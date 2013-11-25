/*
 *  linux/kernel/time/timekeeping.c
 *
 *  Kernel timekeeping code and accessor functions
 *
 *  This code was moved from linux/kernel/timer.c.
 *  Please see that file for copyright and history logs.
 *
 */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/percpu.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/syscore_ops.h>
#include <linux/clocksource.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/tick.h>
#include <linux/stop_machine.h>

struct timekeeper {
	
	struct clocksource *clock;
	
	u32	mult;
	
	int	shift;

	
	cycle_t cycle_interval;
	
	u64	xtime_interval;
	
	s64	xtime_remainder;
	
	u32	raw_interval;

	
	u64	xtime_nsec;
	s64	ntp_error;
	int	ntp_error_shift;

	
	struct timespec xtime;
	struct timespec wall_to_monotonic;
	
	struct timespec total_sleep_time;
	
	struct timespec raw_time;

	
	ktime_t offs_real;

	
	ktime_t offs_boot;

	
	seqlock_t lock;
};

static struct timekeeper timekeeper;

__cacheline_aligned_in_smp DEFINE_SEQLOCK(xtime_lock);


int __read_mostly timekeeping_suspended;



static void timekeeper_setup_internals(struct clocksource *clock)
{
	cycle_t interval;
	u64 tmp, ntpinterval;

	timekeeper.clock = clock;
	clock->cycle_last = clock->read(clock);

	
	tmp = NTP_INTERVAL_LENGTH;
	tmp <<= clock->shift;
	ntpinterval = tmp;
	tmp += clock->mult/2;
	do_div(tmp, clock->mult);
	if (tmp == 0)
		tmp = 1;

	interval = (cycle_t) tmp;
	timekeeper.cycle_interval = interval;

	
	timekeeper.xtime_interval = (u64) interval * clock->mult;
	timekeeper.xtime_remainder = ntpinterval - timekeeper.xtime_interval;
	timekeeper.raw_interval =
		((u64) interval * clock->mult) >> clock->shift;

	timekeeper.xtime_nsec = 0;
	timekeeper.shift = clock->shift;

	timekeeper.ntp_error = 0;
	timekeeper.ntp_error_shift = NTP_SCALE_SHIFT - clock->shift;

	timekeeper.mult = clock->mult;
}

static inline s64 timekeeping_get_ns(void)
{
	cycle_t cycle_now, cycle_delta;
	struct clocksource *clock;

	
	clock = timekeeper.clock;
	cycle_now = clock->read(clock);

	
	cycle_delta = (cycle_now - clock->cycle_last) & clock->mask;

	
	return clocksource_cyc2ns(cycle_delta, timekeeper.mult,
				  timekeeper.shift);
}

static inline s64 timekeeping_get_ns_raw(void)
{
	cycle_t cycle_now, cycle_delta;
	struct clocksource *clock;

	
	clock = timekeeper.clock;
	cycle_now = clock->read(clock);

	
	cycle_delta = (cycle_now - clock->cycle_last) & clock->mask;

	
	return clocksource_cyc2ns(cycle_delta, clock->mult, clock->shift);
}

static void update_rt_offset(void)
{
	struct timespec tmp, *wtm = &timekeeper.wall_to_monotonic;

	set_normalized_timespec(&tmp, -wtm->tv_sec, -wtm->tv_nsec);
	timekeeper.offs_real = timespec_to_ktime(tmp);
}

static void timekeeping_update(bool clearntp)
{
	if (clearntp) {
		timekeeper.ntp_error = 0;
		ntp_clear();
	}
	update_rt_offset();
	update_vsyscall(&timekeeper.xtime, &timekeeper.wall_to_monotonic,
			 timekeeper.clock, timekeeper.mult);
}


static void timekeeping_forward_now(void)
{
	cycle_t cycle_now, cycle_delta;
	struct clocksource *clock;
	s64 nsec;

	clock = timekeeper.clock;
	cycle_now = clock->read(clock);
	cycle_delta = (cycle_now - clock->cycle_last) & clock->mask;
	clock->cycle_last = cycle_now;

	nsec = clocksource_cyc2ns(cycle_delta, timekeeper.mult,
				  timekeeper.shift);

	
	nsec += arch_gettimeoffset();

	timespec_add_ns(&timekeeper.xtime, nsec);

	nsec = clocksource_cyc2ns(cycle_delta, clock->mult, clock->shift);
	timespec_add_ns(&timekeeper.raw_time, nsec);
}

void getnstimeofday(struct timespec *ts)
{
	unsigned long seq;
	s64 nsecs;

	WARN_ON(timekeeping_suspended);

	do {
		seq = read_seqbegin(&timekeeper.lock);

		*ts = timekeeper.xtime;
		nsecs = timekeeping_get_ns();

		
		nsecs += arch_gettimeoffset();

	} while (read_seqretry(&timekeeper.lock, seq));

	timespec_add_ns(ts, nsecs);
}

EXPORT_SYMBOL(getnstimeofday);

ktime_t ktime_get(void)
{
	unsigned int seq;
	s64 secs, nsecs;

	WARN_ON(timekeeping_suspended);

	do {
		seq = read_seqbegin(&timekeeper.lock);
		secs = timekeeper.xtime.tv_sec +
				timekeeper.wall_to_monotonic.tv_sec;
		nsecs = timekeeper.xtime.tv_nsec +
				timekeeper.wall_to_monotonic.tv_nsec;
		nsecs += timekeeping_get_ns();
		
		nsecs += arch_gettimeoffset();

	} while (read_seqretry(&timekeeper.lock, seq));
	return ktime_add_ns(ktime_set(secs, 0), nsecs);
}
EXPORT_SYMBOL_GPL(ktime_get);

void ktime_get_ts(struct timespec *ts)
{
	struct timespec tomono;
	unsigned int seq;
	s64 nsecs;

	WARN_ON(timekeeping_suspended);

	do {
		seq = read_seqbegin(&timekeeper.lock);
		*ts = timekeeper.xtime;
		tomono = timekeeper.wall_to_monotonic;
		nsecs = timekeeping_get_ns();
		
		nsecs += arch_gettimeoffset();

	} while (read_seqretry(&timekeeper.lock, seq));

	set_normalized_timespec(ts, ts->tv_sec + tomono.tv_sec,
				ts->tv_nsec + tomono.tv_nsec + nsecs);
}
EXPORT_SYMBOL_GPL(ktime_get_ts);

#ifdef CONFIG_NTP_PPS

void getnstime_raw_and_real(struct timespec *ts_raw, struct timespec *ts_real)
{
	unsigned long seq;
	s64 nsecs_raw, nsecs_real;

	WARN_ON_ONCE(timekeeping_suspended);

	do {
		u32 arch_offset;

		seq = read_seqbegin(&timekeeper.lock);

		*ts_raw = timekeeper.raw_time;
		*ts_real = timekeeper.xtime;

		nsecs_raw = timekeeping_get_ns_raw();
		nsecs_real = timekeeping_get_ns();

		
		arch_offset = arch_gettimeoffset();
		nsecs_raw += arch_offset;
		nsecs_real += arch_offset;

	} while (read_seqretry(&timekeeper.lock, seq));

	timespec_add_ns(ts_raw, nsecs_raw);
	timespec_add_ns(ts_real, nsecs_real);
}
EXPORT_SYMBOL(getnstime_raw_and_real);

#endif 

void do_gettimeofday(struct timeval *tv)
{
	struct timespec now;

	getnstimeofday(&now);
	tv->tv_sec = now.tv_sec;
	tv->tv_usec = now.tv_nsec/1000;
}

EXPORT_SYMBOL(do_gettimeofday);
int do_settimeofday(const struct timespec *tv)
{
	struct timespec ts_delta;
	unsigned long flags;

	if ((unsigned long)tv->tv_nsec >= NSEC_PER_SEC)
		return -EINVAL;

	write_seqlock_irqsave(&timekeeper.lock, flags);

	timekeeping_forward_now();

	ts_delta.tv_sec = tv->tv_sec - timekeeper.xtime.tv_sec;
	ts_delta.tv_nsec = tv->tv_nsec - timekeeper.xtime.tv_nsec;
	timekeeper.wall_to_monotonic =
			timespec_sub(timekeeper.wall_to_monotonic, ts_delta);

	timekeeper.xtime = *tv;
	timekeeping_update(true);

	write_sequnlock_irqrestore(&timekeeper.lock, flags);

	
	clock_was_set();

	return 0;
}

EXPORT_SYMBOL(do_settimeofday);


int timekeeping_inject_offset(struct timespec *ts)
{
	unsigned long flags;

	if ((unsigned long)ts->tv_nsec >= NSEC_PER_SEC)
		return -EINVAL;

	write_seqlock_irqsave(&timekeeper.lock, flags);

	timekeeping_forward_now();

	timekeeper.xtime = timespec_add(timekeeper.xtime, *ts);
	timekeeper.wall_to_monotonic =
				timespec_sub(timekeeper.wall_to_monotonic, *ts);

	timekeeping_update(true);

	write_sequnlock_irqrestore(&timekeeper.lock, flags);

	
	clock_was_set();

	return 0;
}
EXPORT_SYMBOL(timekeeping_inject_offset);

static int change_clocksource(void *data)
{
	struct clocksource *new, *old;
	unsigned long flags;

	new = (struct clocksource *) data;

	write_seqlock_irqsave(&timekeeper.lock, flags);

	timekeeping_forward_now();
	if (!new->enable || new->enable(new) == 0) {
		old = timekeeper.clock;
		timekeeper_setup_internals(new);
		if (old->disable)
			old->disable(old);
	}
	timekeeping_update(true);

	write_sequnlock_irqrestore(&timekeeper.lock, flags);

	return 0;
}

void timekeeping_notify(struct clocksource *clock)
{
	if (timekeeper.clock == clock)
		return;
	stop_machine(change_clocksource, clock, NULL);
	tick_clock_notify();
}

ktime_t ktime_get_real(void)
{
	struct timespec now;

	getnstimeofday(&now);

	return timespec_to_ktime(now);
}
EXPORT_SYMBOL_GPL(ktime_get_real);

void getrawmonotonic(struct timespec *ts)
{
	unsigned long seq;
	s64 nsecs;

	do {
		seq = read_seqbegin(&timekeeper.lock);
		nsecs = timekeeping_get_ns_raw();
		*ts = timekeeper.raw_time;

	} while (read_seqretry(&timekeeper.lock, seq));

	timespec_add_ns(ts, nsecs);
}
EXPORT_SYMBOL(getrawmonotonic);


int timekeeping_valid_for_hres(void)
{
	unsigned long seq;
	int ret;

	do {
		seq = read_seqbegin(&timekeeper.lock);

		ret = timekeeper.clock->flags & CLOCK_SOURCE_VALID_FOR_HRES;

	} while (read_seqretry(&timekeeper.lock, seq));

	return ret;
}

u64 timekeeping_max_deferment(void)
{
	unsigned long seq;
	u64 ret;
	do {
		seq = read_seqbegin(&timekeeper.lock);

		ret = timekeeper.clock->max_idle_ns;

	} while (read_seqretry(&timekeeper.lock, seq));

	return ret;
}

void __attribute__((weak)) read_persistent_clock(struct timespec *ts)
{
	ts->tv_sec = 0;
	ts->tv_nsec = 0;
}

void __attribute__((weak)) read_boot_clock(struct timespec *ts)
{
	ts->tv_sec = 0;
	ts->tv_nsec = 0;
}

void __init timekeeping_init(void)
{
	struct clocksource *clock;
	unsigned long flags;
	struct timespec now, boot;

	read_persistent_clock(&now);
	read_boot_clock(&boot);

	seqlock_init(&timekeeper.lock);

	ntp_init();

	write_seqlock_irqsave(&timekeeper.lock, flags);
	clock = clocksource_default_clock();
	if (clock->enable)
		clock->enable(clock);
	timekeeper_setup_internals(clock);

	timekeeper.xtime.tv_sec = now.tv_sec;
	timekeeper.xtime.tv_nsec = now.tv_nsec;
	timekeeper.raw_time.tv_sec = 0;
	timekeeper.raw_time.tv_nsec = 0;
	if (boot.tv_sec == 0 && boot.tv_nsec == 0) {
		boot.tv_sec = timekeeper.xtime.tv_sec;
		boot.tv_nsec = timekeeper.xtime.tv_nsec;
	}
	set_normalized_timespec(&timekeeper.wall_to_monotonic,
				-boot.tv_sec, -boot.tv_nsec);
	update_rt_offset();
	timekeeper.total_sleep_time.tv_sec = 0;
	timekeeper.total_sleep_time.tv_nsec = 0;
	write_sequnlock_irqrestore(&timekeeper.lock, flags);
}

static struct timespec timekeeping_suspend_time;

static void update_sleep_time(struct timespec t)
{
	timekeeper.total_sleep_time = t;
	timekeeper.offs_boot = timespec_to_ktime(t);
}

static void __timekeeping_inject_sleeptime(struct timespec *delta)
{
	if (!timespec_valid(delta)) {
		printk(KERN_WARNING "__timekeeping_inject_sleeptime: Invalid "
					"sleep delta value!\n");
		return;
	}

	timekeeper.xtime = timespec_add(timekeeper.xtime, *delta);
	timekeeper.wall_to_monotonic =
			timespec_sub(timekeeper.wall_to_monotonic, *delta);
	update_sleep_time(timespec_add(timekeeper.total_sleep_time, *delta));
}


void timekeeping_inject_sleeptime(struct timespec *delta)
{
	unsigned long flags;
	struct timespec ts;

	
	read_persistent_clock(&ts);
	if (!(ts.tv_sec == 0 && ts.tv_nsec == 0))
		return;

	write_seqlock_irqsave(&timekeeper.lock, flags);

	timekeeping_forward_now();

	__timekeeping_inject_sleeptime(delta);

	timekeeping_update(true);

	write_sequnlock_irqrestore(&timekeeper.lock, flags);

	
	clock_was_set();
}


static void timekeeping_resume(void)
{
	unsigned long flags;
	struct timespec ts;

	read_persistent_clock(&ts);

	clocksource_resume();

	write_seqlock_irqsave(&timekeeper.lock, flags);

	if (timespec_compare(&ts, &timekeeping_suspend_time) > 0) {
		ts = timespec_sub(ts, timekeeping_suspend_time);
		__timekeeping_inject_sleeptime(&ts);
	}
	
	timekeeper.clock->cycle_last = timekeeper.clock->read(timekeeper.clock);
	timekeeper.ntp_error = 0;
	timekeeping_suspended = 0;
	timekeeping_update(false);
	write_sequnlock_irqrestore(&timekeeper.lock, flags);

	touch_softlockup_watchdog();

	clockevents_notify(CLOCK_EVT_NOTIFY_RESUME, NULL);

	
	hrtimers_resume();
}

static int timekeeping_suspend(void)
{
	unsigned long flags;
	struct timespec		delta, delta_delta;
	static struct timespec	old_delta;

	read_persistent_clock(&timekeeping_suspend_time);

	write_seqlock_irqsave(&timekeeper.lock, flags);
	timekeeping_forward_now();
	timekeeping_suspended = 1;

	delta = timespec_sub(timekeeper.xtime, timekeeping_suspend_time);
	delta_delta = timespec_sub(delta, old_delta);
	if (abs(delta_delta.tv_sec)  >= 2) {
		old_delta = delta;
	} else {
		
		timekeeping_suspend_time =
			timespec_add(timekeeping_suspend_time, delta_delta);
	}
	write_sequnlock_irqrestore(&timekeeper.lock, flags);

	clockevents_notify(CLOCK_EVT_NOTIFY_SUSPEND, NULL);
	clocksource_suspend();

	return 0;
}

static struct syscore_ops timekeeping_syscore_ops = {
	.resume		= timekeeping_resume,
	.suspend	= timekeeping_suspend,
};

static int __init timekeeping_init_ops(void)
{
	register_syscore_ops(&timekeeping_syscore_ops);
	return 0;
}

device_initcall(timekeeping_init_ops);

static __always_inline int timekeeping_bigadjust(s64 error, s64 *interval,
						 s64 *offset)
{
	s64 tick_error, i;
	u32 look_ahead, adj;
	s32 error2, mult;

	error2 = timekeeper.ntp_error >> (NTP_SCALE_SHIFT + 22 - 2 * SHIFT_HZ);
	error2 = abs(error2);
	for (look_ahead = 0; error2 > 0; look_ahead++)
		error2 >>= 2;

	tick_error = ntp_tick_length() >> (timekeeper.ntp_error_shift + 1);
	tick_error -= timekeeper.xtime_interval >> 1;
	error = ((error - tick_error) >> look_ahead) + tick_error;

	
	i = *interval;
	mult = 1;
	if (error < 0) {
		error = -error;
		*interval = -*interval;
		*offset = -*offset;
		mult = -1;
	}
	for (adj = 0; error > i; adj++)
		error >>= 1;

	*interval <<= adj;
	*offset <<= adj;
	return mult << adj;
}

static void timekeeping_adjust(s64 offset)
{
	s64 error, interval = timekeeper.cycle_interval;
	int adj;

	error = timekeeper.ntp_error >> (timekeeper.ntp_error_shift - 1);
	if (error > interval) {
		error >>= 2;
		if (likely(error <= interval))
			adj = 1;
		else
			adj = timekeeping_bigadjust(error, &interval, &offset);
	} else if (error < -interval) {
		
		error >>= 2;
		if (likely(error >= -interval)) {
			adj = -1;
			interval = -interval;
			offset = -offset;
		} else
			adj = timekeeping_bigadjust(error, &interval, &offset);
	} else 
		return;

	if (unlikely(timekeeper.clock->maxadj &&
			(timekeeper.mult + adj >
			timekeeper.clock->mult + timekeeper.clock->maxadj))) {
		printk_once(KERN_WARNING
			"Adjusting %s more than 11%% (%ld vs %ld)\n",
			timekeeper.clock->name, (long)timekeeper.mult + adj,
			(long)timekeeper.clock->mult +
				timekeeper.clock->maxadj);
	}
	timekeeper.mult += adj;
	timekeeper.xtime_interval += interval;
	timekeeper.xtime_nsec -= offset;
	timekeeper.ntp_error -= (interval - offset) <<
				timekeeper.ntp_error_shift;
}


static cycle_t logarithmic_accumulation(cycle_t offset, int shift)
{
	u64 nsecps = (u64)NSEC_PER_SEC << timekeeper.shift;
	u64 raw_nsecs;

	
	if (offset < timekeeper.cycle_interval<<shift)
		return offset;

	
	offset -= timekeeper.cycle_interval << shift;
	timekeeper.clock->cycle_last += timekeeper.cycle_interval << shift;

	timekeeper.xtime_nsec += timekeeper.xtime_interval << shift;
	while (timekeeper.xtime_nsec >= nsecps) {
		int leap;
		timekeeper.xtime_nsec -= nsecps;
		timekeeper.xtime.tv_sec++;
		leap = second_overflow(timekeeper.xtime.tv_sec);
		timekeeper.xtime.tv_sec += leap;
		timekeeper.wall_to_monotonic.tv_sec -= leap;
		if (leap)
			clock_was_set_delayed();
	}

	
	raw_nsecs = timekeeper.raw_interval << shift;
	raw_nsecs += timekeeper.raw_time.tv_nsec;
	if (raw_nsecs >= NSEC_PER_SEC) {
		u64 raw_secs = raw_nsecs;
		raw_nsecs = do_div(raw_secs, NSEC_PER_SEC);
		timekeeper.raw_time.tv_sec += raw_secs;
	}
	timekeeper.raw_time.tv_nsec = raw_nsecs;

	
	timekeeper.ntp_error += ntp_tick_length() << shift;
	timekeeper.ntp_error -=
	    (timekeeper.xtime_interval + timekeeper.xtime_remainder) <<
				(timekeeper.ntp_error_shift + shift);

	return offset;
}


static void update_wall_time(void)
{
	struct clocksource *clock;
	cycle_t offset;
	int shift = 0, maxshift;
	unsigned long flags;

	write_seqlock_irqsave(&timekeeper.lock, flags);

	
	if (unlikely(timekeeping_suspended))
		goto out;

	clock = timekeeper.clock;

#ifdef CONFIG_ARCH_USES_GETTIMEOFFSET
	offset = timekeeper.cycle_interval;
#else
	offset = (clock->read(clock) - clock->cycle_last) & clock->mask;
#endif
	timekeeper.xtime_nsec = (s64)timekeeper.xtime.tv_nsec <<
						timekeeper.shift;

	shift = ilog2(offset) - ilog2(timekeeper.cycle_interval);
	shift = max(0, shift);
	
	maxshift = (64 - (ilog2(ntp_tick_length())+1)) - 1;
	shift = min(shift, maxshift);
	while (offset >= timekeeper.cycle_interval) {
		offset = logarithmic_accumulation(offset, shift);
		if(offset < timekeeper.cycle_interval<<shift)
			shift--;
	}

	
	timekeeping_adjust(offset);

	if (unlikely((s64)timekeeper.xtime_nsec < 0)) {
		s64 neg = -(s64)timekeeper.xtime_nsec;
		timekeeper.xtime_nsec = 0;
		timekeeper.ntp_error += neg << timekeeper.ntp_error_shift;
	}


	timekeeper.xtime.tv_nsec = ((s64)timekeeper.xtime_nsec >>
						timekeeper.shift) + 1;
	timekeeper.xtime_nsec -= (s64)timekeeper.xtime.tv_nsec <<
						timekeeper.shift;
	timekeeper.ntp_error +=	timekeeper.xtime_nsec <<
				timekeeper.ntp_error_shift;

	if (unlikely(timekeeper.xtime.tv_nsec >= NSEC_PER_SEC)) {
		int leap;
		timekeeper.xtime.tv_nsec -= NSEC_PER_SEC;
		timekeeper.xtime.tv_sec++;
		leap = second_overflow(timekeeper.xtime.tv_sec);
		timekeeper.xtime.tv_sec += leap;
		timekeeper.wall_to_monotonic.tv_sec -= leap;
		if (leap)
			clock_was_set_delayed();
	}

	timekeeping_update(false);

out:
	write_sequnlock_irqrestore(&timekeeper.lock, flags);

}

void getboottime(struct timespec *ts)
{
	struct timespec boottime = {
		.tv_sec = timekeeper.wall_to_monotonic.tv_sec +
				timekeeper.total_sleep_time.tv_sec,
		.tv_nsec = timekeeper.wall_to_monotonic.tv_nsec +
				timekeeper.total_sleep_time.tv_nsec
	};

	set_normalized_timespec(ts, -boottime.tv_sec, -boottime.tv_nsec);
}
EXPORT_SYMBOL_GPL(getboottime);


void get_monotonic_boottime(struct timespec *ts)
{
	struct timespec tomono, sleep;
	unsigned int seq;
	s64 nsecs;

	WARN_ON(timekeeping_suspended);

	do {
		seq = read_seqbegin(&timekeeper.lock);
		*ts = timekeeper.xtime;
		tomono = timekeeper.wall_to_monotonic;
		sleep = timekeeper.total_sleep_time;
		nsecs = timekeeping_get_ns();

	} while (read_seqretry(&timekeeper.lock, seq));

	set_normalized_timespec(ts, ts->tv_sec + tomono.tv_sec + sleep.tv_sec,
			ts->tv_nsec + tomono.tv_nsec + sleep.tv_nsec + nsecs);
}
EXPORT_SYMBOL_GPL(get_monotonic_boottime);

ktime_t ktime_get_boottime(void)
{
	struct timespec ts;

	get_monotonic_boottime(&ts);
	return timespec_to_ktime(ts);
}
EXPORT_SYMBOL_GPL(ktime_get_boottime);

void monotonic_to_bootbased(struct timespec *ts)
{
	*ts = timespec_add(*ts, timekeeper.total_sleep_time);
}
EXPORT_SYMBOL_GPL(monotonic_to_bootbased);

unsigned long get_seconds(void)
{
	return timekeeper.xtime.tv_sec;
}
EXPORT_SYMBOL(get_seconds);

struct timespec __current_kernel_time(void)
{
	return timekeeper.xtime;
}

struct timespec current_kernel_time(void)
{
	struct timespec now;
	unsigned long seq;

	do {
		seq = read_seqbegin(&timekeeper.lock);

		now = timekeeper.xtime;
	} while (read_seqretry(&timekeeper.lock, seq));

	return now;
}
EXPORT_SYMBOL(current_kernel_time);

struct timespec get_monotonic_coarse(void)
{
	struct timespec now, mono;
	unsigned long seq;

	do {
		seq = read_seqbegin(&timekeeper.lock);

		now = timekeeper.xtime;
		mono = timekeeper.wall_to_monotonic;
	} while (read_seqretry(&timekeeper.lock, seq));

	set_normalized_timespec(&now, now.tv_sec + mono.tv_sec,
				now.tv_nsec + mono.tv_nsec);
	return now;
}

void do_timer(unsigned long ticks)
{
	jiffies_64 += ticks;
	update_wall_time();
	calc_global_load(ticks);
}

void get_xtime_and_monotonic_and_sleep_offset(struct timespec *xtim,
				struct timespec *wtom, struct timespec *sleep)
{
	unsigned long seq;

	do {
		seq = read_seqbegin(&timekeeper.lock);
		*xtim = timekeeper.xtime;
		*wtom = timekeeper.wall_to_monotonic;
		*sleep = timekeeper.total_sleep_time;
	} while (read_seqretry(&timekeeper.lock, seq));
}

#ifdef CONFIG_HIGH_RES_TIMERS
ktime_t ktime_get_update_offsets(ktime_t *offs_real, ktime_t *offs_boot)
{
	ktime_t now;
	unsigned int seq;
	u64 secs, nsecs;

	do {
		seq = read_seqbegin(&timekeeper.lock);

		secs = timekeeper.xtime.tv_sec;
		nsecs = timekeeper.xtime.tv_nsec;
		nsecs += timekeeping_get_ns();
		
		nsecs += arch_gettimeoffset();

		*offs_real = timekeeper.offs_real;
		*offs_boot = timekeeper.offs_boot;
	} while (read_seqretry(&timekeeper.lock, seq));

	now = ktime_add_ns(ktime_set(secs, 0), nsecs);
	now = ktime_sub(now, *offs_real);
	return now;
}
#endif

ktime_t ktime_get_monotonic_offset(void)
{
	unsigned long seq;
	struct timespec wtom;

	do {
		seq = read_seqbegin(&timekeeper.lock);
		wtom = timekeeper.wall_to_monotonic;
	} while (read_seqretry(&timekeeper.lock, seq));

	return timespec_to_ktime(wtom);
}
EXPORT_SYMBOL_GPL(ktime_get_monotonic_offset);


void xtime_update(unsigned long ticks)
{
	write_seqlock(&xtime_lock);
	do_timer(ticks);
	write_sequnlock(&xtime_lock);
}
