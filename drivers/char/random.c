/*
 * random.c -- A strong random number generator
 *
 * Copyright Matt Mackall <mpm@selenic.com>, 2003, 2004, 2005
 *
 * Copyright Theodore Ts'o, 1994, 1995, 1996, 1997, 1998, 1999.  All
 * rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * ALTERNATIVELY, this product may be distributed under the terms of
 * the GNU General Public License, in which case the provisions of the GPL are
 * required INSTEAD OF the above restrictions.  (This clause is
 * necessary due to a potential bad interaction between the GPL and
 * the restrictions contained in a BSD-style copyright.)
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ALL OF
 * WHICH ARE HEREBY DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF NOT ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */


#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/percpu.h>
#include <linux/cryptohash.h>
#include <linux/fips.h>
#include <linux/ptrace.h>
#include <linux/kmemcheck.h>

#ifdef CONFIG_GENERIC_HARDIRQS
# include <linux/irq.h>
#endif

#include <asm/processor.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/irq_regs.h>
#include <asm/io.h>

#define CREATE_TRACE_POINTS
#include <trace/events/random.h>

#define INPUT_POOL_WORDS 128
#define OUTPUT_POOL_WORDS 32
#define SEC_XFER_SIZE 512
#define EXTRACT_SIZE 10

#define LONGS(x) (((x) + sizeof(unsigned long) - 1)/sizeof(unsigned long))

static int random_read_wakeup_thresh = 64;

static int random_write_wakeup_thresh = 128;


static int trickle_thresh __read_mostly = INPUT_POOL_WORDS * 28;

static DEFINE_PER_CPU(int, trickle_count);

static struct poolinfo {
	int poolwords;
	int tap1, tap2, tap3, tap4, tap5;
} poolinfo_table[] = {
	
	{ 128,	103,	76,	51,	25,	1 },
	
	{ 32,	26,	20,	14,	7,	1 },
#if 0
	
	{ 2048,	1638,	1231,	819,	411,	1 },

	
	{ 1024,	817,	615,	412,	204,	1 },

	
	{ 1024,	819,	616,	410,	207,	2 },

	
	{ 512,	411,	308,	208,	104,	1 },

	
	{ 512,	409,	307,	206,	102,	2 },
	
	{ 512,	409,	309,	205,	103,	2 },

	
	{ 256,	205,	155,	101,	52,	1 },

	
	{ 128,	103,	78,	51,	27,	2 },

	
	{ 64,	52,	39,	26,	14,	1 },
#endif
};

#define POOLBITS	poolwords*32
#define POOLBYTES	poolwords*4


static DECLARE_WAIT_QUEUE_HEAD(random_read_wait);
static DECLARE_WAIT_QUEUE_HEAD(random_write_wait);
static struct fasync_struct *fasync;

#if 0
static bool debug;
module_param(debug, bool, 0644);
#define DEBUG_ENT(fmt, arg...) do { \
	if (debug) \
		printk(KERN_DEBUG "random %04d %04d %04d: " \
		fmt,\
		input_pool.entropy_count,\
		blocking_pool.entropy_count,\
		nonblocking_pool.entropy_count,\
		## arg); } while (0)
#else
#define DEBUG_ENT(fmt, arg...) do {} while (0)
#endif


struct entropy_store;
struct entropy_store {
	
	struct poolinfo *poolinfo;
	__u32 *pool;
	const char *name;
	struct entropy_store *pull;
	int limit;

	
	spinlock_t lock;
	unsigned add_ptr;
	unsigned input_rotate;
	int entropy_count;
	int entropy_total;
	unsigned int initialized:1;
	__u8 last_data[EXTRACT_SIZE];
};

static __u32 input_pool_data[INPUT_POOL_WORDS];
static __u32 blocking_pool_data[OUTPUT_POOL_WORDS];
static __u32 nonblocking_pool_data[OUTPUT_POOL_WORDS];

static struct entropy_store input_pool = {
	.poolinfo = &poolinfo_table[0],
	.name = "input",
	.limit = 1,
	.lock = __SPIN_LOCK_UNLOCKED(&input_pool.lock),
	.pool = input_pool_data
};

static struct entropy_store blocking_pool = {
	.poolinfo = &poolinfo_table[1],
	.name = "blocking",
	.limit = 1,
	.pull = &input_pool,
	.lock = __SPIN_LOCK_UNLOCKED(&blocking_pool.lock),
	.pool = blocking_pool_data
};

static struct entropy_store nonblocking_pool = {
	.poolinfo = &poolinfo_table[1],
	.name = "nonblocking",
	.pull = &input_pool,
	.lock = __SPIN_LOCK_UNLOCKED(&nonblocking_pool.lock),
	.pool = nonblocking_pool_data
};

static __u32 const twist_table[8] = {
	0x00000000, 0x3b6e20c8, 0x76dc4190, 0x4db26158,
	0xedb88320, 0xd6d6a3e8, 0x9b64c2b0, 0xa00ae278 };

static void _mix_pool_bytes(struct entropy_store *r, const void *in,
			    int nbytes, __u8 out[64])
{
	unsigned long i, j, tap1, tap2, tap3, tap4, tap5;
	int input_rotate;
	int wordmask = r->poolinfo->poolwords - 1;
	const char *bytes = in;
	__u32 w;

	tap1 = r->poolinfo->tap1;
	tap2 = r->poolinfo->tap2;
	tap3 = r->poolinfo->tap3;
	tap4 = r->poolinfo->tap4;
	tap5 = r->poolinfo->tap5;

	smp_rmb();
	input_rotate = ACCESS_ONCE(r->input_rotate);
	i = ACCESS_ONCE(r->add_ptr);

	
	while (nbytes--) {
		w = rol32(*bytes++, input_rotate & 31);
		i = (i - 1) & wordmask;

		
		w ^= r->pool[i];
		w ^= r->pool[(i + tap1) & wordmask];
		w ^= r->pool[(i + tap2) & wordmask];
		w ^= r->pool[(i + tap3) & wordmask];
		w ^= r->pool[(i + tap4) & wordmask];
		w ^= r->pool[(i + tap5) & wordmask];

		
		r->pool[i] = (w >> 3) ^ twist_table[w & 7];

		input_rotate += i ? 7 : 14;
	}

	ACCESS_ONCE(r->input_rotate) = input_rotate;
	ACCESS_ONCE(r->add_ptr) = i;
	smp_wmb();

	if (out)
		for (j = 0; j < 16; j++)
			((__u32 *)out)[j] = r->pool[(i - j) & wordmask];
}

static void __mix_pool_bytes(struct entropy_store *r, const void *in,
			     int nbytes, __u8 out[64])
{
	trace_mix_pool_bytes_nolock(r->name, nbytes, _RET_IP_);
	_mix_pool_bytes(r, in, nbytes, out);
}

static void mix_pool_bytes(struct entropy_store *r, const void *in,
			   int nbytes, __u8 out[64])
{
	unsigned long flags;

	trace_mix_pool_bytes(r->name, nbytes, _RET_IP_);
	spin_lock_irqsave(&r->lock, flags);
	_mix_pool_bytes(r, in, nbytes, out);
	spin_unlock_irqrestore(&r->lock, flags);
}

struct fast_pool {
	__u32		pool[4];
	unsigned long	last;
	unsigned short	count;
	unsigned char	rotate;
	unsigned char	last_timer_intr;
};

static void fast_mix(struct fast_pool *f, const void *in, int nbytes)
{
	const char	*bytes = in;
	__u32		w;
	unsigned	i = f->count;
	unsigned	input_rotate = f->rotate;

	while (nbytes--) {
		w = rol32(*bytes++, input_rotate & 31) ^ f->pool[i & 3] ^
			f->pool[(i + 1) & 3];
		f->pool[i & 3] = (w >> 3) ^ twist_table[w & 7];
		input_rotate += (i++ & 3) ? 7 : 14;
	}
	f->count = i;
	f->rotate = input_rotate;
}

static void credit_entropy_bits(struct entropy_store *r, int nbits)
{
	int entropy_count, orig;

	if (!nbits)
		return;

	DEBUG_ENT("added %d entropy credits to %s\n", nbits, r->name);
retry:
	entropy_count = orig = ACCESS_ONCE(r->entropy_count);
	entropy_count += nbits;

	if (entropy_count < 0) {
		DEBUG_ENT("negative entropy/overflow\n");
		entropy_count = 0;
	} else if (entropy_count > r->poolinfo->POOLBITS)
		entropy_count = r->poolinfo->POOLBITS;
	if (cmpxchg(&r->entropy_count, orig, entropy_count) != orig)
		goto retry;

	if (!r->initialized && nbits > 0) {
		r->entropy_total += nbits;
		if (r->entropy_total > 128)
			r->initialized = 1;
	}

	trace_credit_entropy_bits(r->name, nbits, entropy_count,
				  r->entropy_total, _RET_IP_);

	
	if (r == &input_pool && entropy_count >= random_read_wakeup_thresh) {
		wake_up_interruptible(&random_read_wait);
		kill_fasync(&fasync, SIGIO, POLL_IN);
	}
}


struct timer_rand_state {
	cycles_t last_time;
	long last_delta, last_delta2;
	unsigned dont_count_entropy:1;
};

void add_device_randomness(const void *buf, unsigned int size)
{
	unsigned long time = get_cycles() ^ jiffies;

	mix_pool_bytes(&input_pool, buf, size, NULL);
	mix_pool_bytes(&input_pool, &time, sizeof(time), NULL);
	mix_pool_bytes(&nonblocking_pool, buf, size, NULL);
	mix_pool_bytes(&nonblocking_pool, &time, sizeof(time), NULL);
}
EXPORT_SYMBOL(add_device_randomness);

static struct timer_rand_state input_timer_state;

static void add_timer_randomness(struct timer_rand_state *state, unsigned num)
{
	struct {
		long jiffies;
		unsigned cycles;
		unsigned num;
	} sample;
	long delta, delta2, delta3;

	preempt_disable();
	
	if (input_pool.entropy_count > trickle_thresh &&
	    ((__this_cpu_inc_return(trickle_count) - 1) & 0xfff))
		goto out;

	sample.jiffies = jiffies;
	sample.cycles = get_cycles();
	sample.num = num;
	mix_pool_bytes(&input_pool, &sample, sizeof(sample), NULL);


	if (!state->dont_count_entropy) {
		delta = sample.jiffies - state->last_time;
		state->last_time = sample.jiffies;

		delta2 = delta - state->last_delta;
		state->last_delta = delta;

		delta3 = delta2 - state->last_delta2;
		state->last_delta2 = delta2;

		if (delta < 0)
			delta = -delta;
		if (delta2 < 0)
			delta2 = -delta2;
		if (delta3 < 0)
			delta3 = -delta3;
		if (delta > delta2)
			delta = delta2;
		if (delta > delta3)
			delta = delta3;

		credit_entropy_bits(&input_pool,
				    min_t(int, fls(delta>>1), 11));
	}
out:
	preempt_enable();
}

void add_input_randomness(unsigned int type, unsigned int code,
				 unsigned int value)
{
	static unsigned char last_value;

	
	if (value == last_value)
		return;

	DEBUG_ENT("input event\n");
	last_value = value;
	add_timer_randomness(&input_timer_state,
			     (type << 4) ^ code ^ (code >> 4) ^ value);
}
EXPORT_SYMBOL_GPL(add_input_randomness);

static DEFINE_PER_CPU(struct fast_pool, irq_randomness);

void add_interrupt_randomness(int irq, int irq_flags)
{
	struct entropy_store	*r;
	struct fast_pool	*fast_pool = &__get_cpu_var(irq_randomness);
	struct pt_regs		*regs = get_irq_regs();
	unsigned long		now = jiffies;
	__u32			input[4], cycles = get_cycles();

	input[0] = cycles ^ jiffies;
	input[1] = irq;
	if (regs) {
		__u64 ip = instruction_pointer(regs);
		input[2] = ip;
		input[3] = ip >> 32;
	}

	fast_mix(fast_pool, input, sizeof(input));

	if ((fast_pool->count & 1023) &&
	    !time_after(now, fast_pool->last + HZ))
		return;

	fast_pool->last = now;

	r = nonblocking_pool.initialized ? &input_pool : &nonblocking_pool;
	__mix_pool_bytes(r, &fast_pool->pool, sizeof(fast_pool->pool), NULL);
	if (cycles == 0) {
		if (irq_flags & __IRQF_TIMER) {
			if (fast_pool->last_timer_intr)
				return;
			fast_pool->last_timer_intr = 1;
		} else
			fast_pool->last_timer_intr = 0;
	}
	credit_entropy_bits(r, 1);
}

#ifdef CONFIG_BLOCK
void add_disk_randomness(struct gendisk *disk)
{
	if (!disk || !disk->random)
		return;
	
	DEBUG_ENT("disk event %d:%d\n",
		  MAJOR(disk_devt(disk)), MINOR(disk_devt(disk)));

	add_timer_randomness(disk->random, 0x100 + disk_devt(disk));
}
#endif


static ssize_t extract_entropy(struct entropy_store *r, void *buf,
			       size_t nbytes, int min, int rsvd);

static void xfer_secondary_pool(struct entropy_store *r, size_t nbytes)
{
	__u32	tmp[OUTPUT_POOL_WORDS];

	if (r->pull && r->entropy_count < nbytes * 8 &&
	    r->entropy_count < r->poolinfo->POOLBITS) {
		
		int rsvd = r->limit ? 0 : random_read_wakeup_thresh/4;
		int bytes = nbytes;

		
		bytes = max_t(int, bytes, random_read_wakeup_thresh / 8);
		
		bytes = min_t(int, bytes, sizeof(tmp));

		DEBUG_ENT("going to reseed %s with %d bits "
			  "(%d of %d requested)\n",
			  r->name, bytes * 8, nbytes * 8, r->entropy_count);

		bytes = extract_entropy(r->pull, tmp, bytes,
					random_read_wakeup_thresh / 8, rsvd);
		mix_pool_bytes(r, tmp, bytes, NULL);
		credit_entropy_bits(r, bytes*8);
	}
}


static size_t account(struct entropy_store *r, size_t nbytes, int min,
		      int reserved)
{
	unsigned long flags;

	
	spin_lock_irqsave(&r->lock, flags);

	BUG_ON(r->entropy_count > r->poolinfo->POOLBITS);
	DEBUG_ENT("trying to extract %d bits from %s\n",
		  nbytes * 8, r->name);

	
	if (r->entropy_count / 8 < min + reserved) {
		nbytes = 0;
	} else {
		
		if (r->limit && nbytes + reserved >= r->entropy_count / 8)
			nbytes = r->entropy_count/8 - reserved;

		if (r->entropy_count / 8 >= nbytes + reserved)
			r->entropy_count -= nbytes*8;
		else
			r->entropy_count = reserved;

		if (r->entropy_count < random_write_wakeup_thresh) {
			wake_up_interruptible(&random_write_wait);
			kill_fasync(&fasync, SIGIO, POLL_OUT);
		}
	}

	DEBUG_ENT("debiting %d entropy credits from %s%s\n",
		  nbytes * 8, r->name, r->limit ? "" : " (unlimited)");

	spin_unlock_irqrestore(&r->lock, flags);

	return nbytes;
}

static void extract_buf(struct entropy_store *r, __u8 *out)
{
	int i;
	union {
		__u32 w[5];
		unsigned long l[LONGS(EXTRACT_SIZE)];
	} hash;
	__u32 workspace[SHA_WORKSPACE_WORDS];
	__u8 extract[64];
	unsigned long flags;

	
	sha_init(hash.w);
	spin_lock_irqsave(&r->lock, flags);
	for (i = 0; i < r->poolinfo->poolwords; i += 16)
		sha_transform(hash.w, (__u8 *)(r->pool + i), workspace);

	__mix_pool_bytes(r, hash.w, sizeof(hash.w), extract);
	spin_unlock_irqrestore(&r->lock, flags);

	sha_transform(hash.w, extract, workspace);
	memset(extract, 0, sizeof(extract));
	memset(workspace, 0, sizeof(workspace));

	hash.w[0] ^= hash.w[3];
	hash.w[1] ^= hash.w[4];
	hash.w[2] ^= rol32(hash.w[2], 16);

	for (i = 0; i < LONGS(EXTRACT_SIZE); i++) {
		unsigned long v;
		if (!arch_get_random_long(&v))
			break;
		hash.l[i] ^= v;
	}

	memcpy(out, &hash, EXTRACT_SIZE);
	memset(&hash, 0, sizeof(hash));
}

static ssize_t extract_entropy(struct entropy_store *r, void *buf,
				 size_t nbytes, int min, int reserved)
{
	ssize_t ret = 0, i;
	__u8 tmp[EXTRACT_SIZE];

	trace_extract_entropy(r->name, nbytes, r->entropy_count, _RET_IP_);
	xfer_secondary_pool(r, nbytes);
	nbytes = account(r, nbytes, min, reserved);

	while (nbytes) {
		extract_buf(r, tmp);

		if (fips_enabled) {
			unsigned long flags;

			spin_lock_irqsave(&r->lock, flags);
			if (!memcmp(tmp, r->last_data, EXTRACT_SIZE))
				panic("Hardware RNG duplicated output!\n");
			memcpy(r->last_data, tmp, EXTRACT_SIZE);
			spin_unlock_irqrestore(&r->lock, flags);
		}
		i = min_t(int, nbytes, EXTRACT_SIZE);
		memcpy(buf, tmp, i);
		nbytes -= i;
		buf += i;
		ret += i;
	}

	
	memset(tmp, 0, sizeof(tmp));

	return ret;
}

static ssize_t extract_entropy_user(struct entropy_store *r, void __user *buf,
				    size_t nbytes)
{
	ssize_t ret = 0, i;
	__u8 tmp[EXTRACT_SIZE];

	trace_extract_entropy_user(r->name, nbytes, r->entropy_count, _RET_IP_);
	xfer_secondary_pool(r, nbytes);
	nbytes = account(r, nbytes, 0, 0);

	while (nbytes) {
		if (need_resched()) {
			if (signal_pending(current)) {
				if (ret == 0)
					ret = -ERESTARTSYS;
				break;
			}
			schedule();
		}

		extract_buf(r, tmp);
		i = min_t(int, nbytes, EXTRACT_SIZE);
		if (copy_to_user(buf, tmp, i)) {
			ret = -EFAULT;
			break;
		}

		nbytes -= i;
		buf += i;
		ret += i;
	}

	
	memset(tmp, 0, sizeof(tmp));

	return ret;
}

void get_random_bytes(void *buf, int nbytes)
{
	extract_entropy(&nonblocking_pool, buf, nbytes, 0, 0);
}
EXPORT_SYMBOL(get_random_bytes);

void get_random_bytes_arch(void *buf, int nbytes)
{
	char *p = buf;

	trace_get_random_bytes(nbytes, _RET_IP_);
	while (nbytes) {
		unsigned long v;
		int chunk = min(nbytes, (int)sizeof(unsigned long));

		if (!arch_get_random_long(&v))
			break;
		
		memcpy(p, &v, chunk);
		p += chunk;
		nbytes -= chunk;
	}

	if (nbytes)
		extract_entropy(&nonblocking_pool, p, nbytes, 0, 0);
}
EXPORT_SYMBOL(get_random_bytes_arch);


static void init_std_data(struct entropy_store *r)
{
	int i;
	ktime_t now = ktime_get_real();
	unsigned long rv;

	r->entropy_count = 0;
	r->entropy_total = 0;
	mix_pool_bytes(r, &now, sizeof(now), NULL);
	for (i = r->poolinfo->POOLBYTES; i > 0; i -= sizeof(rv)) {
		if (!arch_get_random_long(&rv))
			break;
		mix_pool_bytes(r, &rv, sizeof(rv), NULL);
	}
	mix_pool_bytes(r, utsname(), sizeof(*(utsname())), NULL);
}

static int rand_initialize(void)
{
	init_std_data(&input_pool);
	init_std_data(&blocking_pool);
	init_std_data(&nonblocking_pool);
	return 0;
}
module_init(rand_initialize);

#ifdef CONFIG_BLOCK
void rand_initialize_disk(struct gendisk *disk)
{
	struct timer_rand_state *state;

	state = kzalloc(sizeof(struct timer_rand_state), GFP_KERNEL);
	if (state)
		disk->random = state;
}
#endif

static ssize_t
random_read(struct file *file, char __user *buf, size_t nbytes, loff_t *ppos)
{
	ssize_t n, retval = 0, count = 0;

	if (nbytes == 0)
		return 0;

	while (nbytes > 0) {
		n = nbytes;
		if (n > SEC_XFER_SIZE)
			n = SEC_XFER_SIZE;

		DEBUG_ENT("reading %d bits\n", n*8);

		n = extract_entropy_user(&blocking_pool, buf, n);

		DEBUG_ENT("read got %d bits (%d still needed)\n",
			  n*8, (nbytes-n)*8);

		if (n == 0) {
			if (file->f_flags & O_NONBLOCK) {
				retval = -EAGAIN;
				break;
			}

			DEBUG_ENT("sleeping?\n");

			wait_event_interruptible(random_read_wait,
				input_pool.entropy_count >=
						 random_read_wakeup_thresh);

			DEBUG_ENT("awake\n");

			if (signal_pending(current)) {
				retval = -ERESTARTSYS;
				break;
			}

			continue;
		}

		if (n < 0) {
			retval = n;
			break;
		}
		count += n;
		buf += n;
		nbytes -= n;
		break;		
				
	}

	return (count ? count : retval);
}

static ssize_t
urandom_read(struct file *file, char __user *buf, size_t nbytes, loff_t *ppos)
{
	return extract_entropy_user(&nonblocking_pool, buf, nbytes);
}

static unsigned int
random_poll(struct file *file, poll_table * wait)
{
	unsigned int mask;

	poll_wait(file, &random_read_wait, wait);
	poll_wait(file, &random_write_wait, wait);
	mask = 0;
	if (input_pool.entropy_count >= random_read_wakeup_thresh)
		mask |= POLLIN | POLLRDNORM;
	if (input_pool.entropy_count < random_write_wakeup_thresh)
		mask |= POLLOUT | POLLWRNORM;
	return mask;
}

static int
write_pool(struct entropy_store *r, const char __user *buffer, size_t count)
{
	size_t bytes;
	__u32 buf[16];
	const char __user *p = buffer;

	while (count > 0) {
		bytes = min(count, sizeof(buf));
		if (copy_from_user(&buf, p, bytes))
			return -EFAULT;

		count -= bytes;
		p += bytes;

		mix_pool_bytes(r, buf, bytes, NULL);
		cond_resched();
	}

	return 0;
}

static ssize_t random_write(struct file *file, const char __user *buffer,
			    size_t count, loff_t *ppos)
{
	size_t ret;

	ret = write_pool(&blocking_pool, buffer, count);
	if (ret)
		return ret;
	ret = write_pool(&nonblocking_pool, buffer, count);
	if (ret)
		return ret;

	return (ssize_t)count;
}

static long random_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	int size, ent_count;
	int __user *p = (int __user *)arg;
	int retval;

	switch (cmd) {
	case RNDGETENTCNT:
		
		if (put_user(input_pool.entropy_count, p))
			return -EFAULT;
		return 0;
	case RNDADDTOENTCNT:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		if (get_user(ent_count, p))
			return -EFAULT;
		credit_entropy_bits(&input_pool, ent_count);
		return 0;
	case RNDADDENTROPY:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		if (get_user(ent_count, p++))
			return -EFAULT;
		if (ent_count < 0)
			return -EINVAL;
		if (get_user(size, p++))
			return -EFAULT;
		retval = write_pool(&input_pool, (const char __user *)p,
				    size);
		if (retval < 0)
			return retval;
		credit_entropy_bits(&input_pool, ent_count);
		return 0;
	case RNDZAPENTCNT:
	case RNDCLEARPOOL:
		
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		rand_initialize();
		return 0;
	default:
		return -EINVAL;
	}
}

static int random_fasync(int fd, struct file *filp, int on)
{
	return fasync_helper(fd, filp, on, &fasync);
}

const struct file_operations random_fops = {
	.read  = random_read,
	.write = random_write,
	.poll  = random_poll,
	.unlocked_ioctl = random_ioctl,
	.fasync = random_fasync,
	.llseek = noop_llseek,
};

const struct file_operations urandom_fops = {
	.read  = urandom_read,
	.write = random_write,
	.unlocked_ioctl = random_ioctl,
	.fasync = random_fasync,
	.llseek = noop_llseek,
};


void generate_random_uuid(unsigned char uuid_out[16])
{
	get_random_bytes(uuid_out, 16);
	
	uuid_out[6] = (uuid_out[6] & 0x0F) | 0x40;
	
	uuid_out[8] = (uuid_out[8] & 0x3F) | 0x80;
}
EXPORT_SYMBOL(generate_random_uuid);


#ifdef CONFIG_SYSCTL

#include <linux/sysctl.h>

static int min_read_thresh = 8, min_write_thresh;
static int max_read_thresh = INPUT_POOL_WORDS * 32;
static int max_write_thresh = INPUT_POOL_WORDS * 32;
static char sysctl_bootid[16];

static int proc_do_uuid(ctl_table *table, int write,
			void __user *buffer, size_t *lenp, loff_t *ppos)
{
	ctl_table fake_table;
	unsigned char buf[64], tmp_uuid[16], *uuid;

	uuid = table->data;
	if (!uuid) {
		uuid = tmp_uuid;
		generate_random_uuid(uuid);
	} else {
		static DEFINE_SPINLOCK(bootid_spinlock);

		spin_lock(&bootid_spinlock);
		if (!uuid[8])
			generate_random_uuid(uuid);
		spin_unlock(&bootid_spinlock);
	}

	sprintf(buf, "%pU", uuid);

	fake_table.data = buf;
	fake_table.maxlen = sizeof(buf);

	return proc_dostring(&fake_table, write, buffer, lenp, ppos);
}

static int sysctl_poolsize = INPUT_POOL_WORDS * 32;
ctl_table random_table[] = {
	{
		.procname	= "poolsize",
		.data		= &sysctl_poolsize,
		.maxlen		= sizeof(int),
		.mode		= 0444,
		.proc_handler	= proc_dointvec,
	},
	{
		.procname	= "entropy_avail",
		.maxlen		= sizeof(int),
		.mode		= 0444,
		.proc_handler	= proc_dointvec,
		.data		= &input_pool.entropy_count,
	},
	{
		.procname	= "read_wakeup_threshold",
		.data		= &random_read_wakeup_thresh,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_minmax,
		.extra1		= &min_read_thresh,
		.extra2		= &max_read_thresh,
	},
	{
		.procname	= "write_wakeup_threshold",
		.data		= &random_write_wakeup_thresh,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_minmax,
		.extra1		= &min_write_thresh,
		.extra2		= &max_write_thresh,
	},
	{
		.procname	= "boot_id",
		.data		= &sysctl_bootid,
		.maxlen		= 16,
		.mode		= 0444,
		.proc_handler	= proc_do_uuid,
	},
	{
		.procname	= "uuid",
		.maxlen		= 16,
		.mode		= 0444,
		.proc_handler	= proc_do_uuid,
	},
	{ }
};
#endif 	

static u32 random_int_secret[MD5_MESSAGE_BYTES / 4] ____cacheline_aligned;

static int __init random_int_secret_init(void)
{
	get_random_bytes(random_int_secret, sizeof(random_int_secret));
	return 0;
}
late_initcall(random_int_secret_init);

DEFINE_PER_CPU(__u32 [MD5_DIGEST_WORDS], get_random_int_hash);
unsigned int get_random_int(void)
{
	__u32 *hash;
	unsigned int ret;

	if (arch_get_random_int(&ret))
		return ret;

	hash = get_cpu_var(get_random_int_hash);

	hash[0] += current->pid + jiffies + get_cycles();
	md5_transform(hash, random_int_secret);
	ret = hash[0];
	put_cpu_var(get_random_int_hash);

	return ret;
}

unsigned long
randomize_range(unsigned long start, unsigned long end, unsigned long len)
{
	unsigned long range = end - len - start;

	if (end <= start + len)
		return 0;
	return PAGE_ALIGN(get_random_int() % range + start);
}
