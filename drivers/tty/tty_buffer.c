
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/module.h>


void tty_buffer_free_all(struct tty_struct *tty)
{
	struct tty_buffer *thead;
	while ((thead = tty->buf.head) != NULL) {
		tty->buf.head = thead->next;
		kfree(thead);
	}
	while ((thead = tty->buf.free) != NULL) {
		tty->buf.free = thead->next;
		kfree(thead);
	}
	tty->buf.tail = NULL;
	tty->buf.memory_used = 0;
}


static struct tty_buffer *tty_buffer_alloc(struct tty_struct *tty, size_t size)
{
	struct tty_buffer *p;
	
	if (tty->buf.memory_used + size > 69632)
		return NULL;
	p = kmalloc(sizeof(struct tty_buffer) + 2 * size, GFP_ATOMIC);
	if (p == NULL)
		return NULL;
	p->used = 0;
	p->size = size;
	p->next = NULL;
	p->commit = 0;
	p->read = 0;
	p->char_buf_ptr = (char *)(p->data);
	p->flag_buf_ptr = (unsigned char *)p->char_buf_ptr + size;
	tty->buf.memory_used += size;
	return p;
}


static void tty_buffer_free(struct tty_struct *tty, struct tty_buffer *b)
{
	
	tty->buf.memory_used -= b->size;
	WARN_ON(tty->buf.memory_used < 0);

	if (b->size >= 512)
		kfree(b);
	else {
		b->next = tty->buf.free;
		tty->buf.free = b;
	}
}


static void __tty_buffer_flush(struct tty_struct *tty)
{
	struct tty_buffer *thead;

	while ((thead = tty->buf.head) != NULL) {
		tty->buf.head = thead->next;
		tty_buffer_free(tty, thead);
	}
	tty->buf.tail = NULL;
}


void tty_buffer_flush(struct tty_struct *tty)
{
	unsigned long flags;
	spin_lock_irqsave(&tty->buf.lock, flags);

	if (test_bit(TTY_FLUSHING, &tty->flags)) {
		set_bit(TTY_FLUSHPENDING, &tty->flags);
		spin_unlock_irqrestore(&tty->buf.lock, flags);
		wait_event(tty->read_wait,
				test_bit(TTY_FLUSHPENDING, &tty->flags) == 0);
		return;
	} else
		__tty_buffer_flush(tty);
	spin_unlock_irqrestore(&tty->buf.lock, flags);
}


static struct tty_buffer *tty_buffer_find(struct tty_struct *tty, size_t size)
{
	struct tty_buffer **tbh = &tty->buf.free;
	while ((*tbh) != NULL) {
		struct tty_buffer *t = *tbh;
		if (t->size >= size) {
			*tbh = t->next;
			t->next = NULL;
			t->used = 0;
			t->commit = 0;
			t->read = 0;
			tty->buf.memory_used += t->size;
			return t;
		}
		tbh = &((*tbh)->next);
	}
	
	size = (size + 0xFF) & ~0xFF;
	return tty_buffer_alloc(tty, size);
}
static int __tty_buffer_request_room(struct tty_struct *tty, size_t size)
{
	struct tty_buffer *b, *n;
	int left;
	if ((b = tty->buf.tail) != NULL)
		left = b->size - b->used;
	else
		left = 0;

	if (left < size) {
		
		if ((n = tty_buffer_find(tty, size)) != NULL) {
			if (b != NULL) {
				b->next = n;
				b->commit = b->used;
			} else
				tty->buf.head = n;
			tty->buf.tail = n;
		} else
			size = left;
	}

	return size;
}


int tty_buffer_request_room(struct tty_struct *tty, size_t size)
{
	unsigned long flags;
	int length;

	spin_lock_irqsave(&tty->buf.lock, flags);
	length = __tty_buffer_request_room(tty, size);
	spin_unlock_irqrestore(&tty->buf.lock, flags);
	return length;
}
EXPORT_SYMBOL_GPL(tty_buffer_request_room);


int tty_insert_flip_string_fixed_flag(struct tty_struct *tty,
		const unsigned char *chars, char flag, size_t size)
{
	int copied = 0;
	do {
		int goal = min_t(size_t, size - copied, TTY_BUFFER_PAGE);
		int space;
		unsigned long flags;
		struct tty_buffer *tb;

		spin_lock_irqsave(&tty->buf.lock, flags);
		space = __tty_buffer_request_room(tty, goal);
		tb = tty->buf.tail;
		
		if (unlikely(space == 0)) {
			spin_unlock_irqrestore(&tty->buf.lock, flags);
			break;
		}
		memcpy(tb->char_buf_ptr + tb->used, chars, space);
		memset(tb->flag_buf_ptr + tb->used, flag, space);
		tb->used += space;
		spin_unlock_irqrestore(&tty->buf.lock, flags);
		copied += space;
		chars += space;
	} while (unlikely(size > copied));
	return copied;
}
EXPORT_SYMBOL(tty_insert_flip_string_fixed_flag);


int tty_insert_flip_string_flags(struct tty_struct *tty,
		const unsigned char *chars, const char *flags, size_t size)
{
	int copied = 0;
	do {
		int goal = min_t(size_t, size - copied, TTY_BUFFER_PAGE);
		int space;
		unsigned long __flags;
		struct tty_buffer *tb;

		spin_lock_irqsave(&tty->buf.lock, __flags);
		space = __tty_buffer_request_room(tty, goal);
		tb = tty->buf.tail;
		
		if (unlikely(space == 0)) {
			spin_unlock_irqrestore(&tty->buf.lock, __flags);
			break;
		}
		memcpy(tb->char_buf_ptr + tb->used, chars, space);
		memcpy(tb->flag_buf_ptr + tb->used, flags, space);
		tb->used += space;
		spin_unlock_irqrestore(&tty->buf.lock, __flags);
		copied += space;
		chars += space;
		flags += space;
	} while (unlikely(size > copied));
	return copied;
}
EXPORT_SYMBOL(tty_insert_flip_string_flags);


void tty_schedule_flip(struct tty_struct *tty)
{
	unsigned long flags;
	spin_lock_irqsave(&tty->buf.lock, flags);
	if (tty->buf.tail != NULL)
		tty->buf.tail->commit = tty->buf.tail->used;
	spin_unlock_irqrestore(&tty->buf.lock, flags);
#if defined(CONFIG_MSM_SMD0_WQ)
	if (!strcmp(tty->name, "smd0"))
		queue_work(tty_wq, &tty->buf.work);
	else
#endif
	schedule_work(&tty->buf.work);
}
EXPORT_SYMBOL(tty_schedule_flip);


int tty_prepare_flip_string(struct tty_struct *tty, unsigned char **chars,
								size_t size)
{
	int space;
	unsigned long flags;
	struct tty_buffer *tb;

	spin_lock_irqsave(&tty->buf.lock, flags);
	space = __tty_buffer_request_room(tty, size);

	tb = tty->buf.tail;
	if (likely(space)) {
		*chars = tb->char_buf_ptr + tb->used;
		memset(tb->flag_buf_ptr + tb->used, TTY_NORMAL, space);
		tb->used += space;
	}
	spin_unlock_irqrestore(&tty->buf.lock, flags);
	return space;
}
EXPORT_SYMBOL_GPL(tty_prepare_flip_string);


int tty_prepare_flip_string_flags(struct tty_struct *tty,
			unsigned char **chars, char **flags, size_t size)
{
	int space;
	unsigned long __flags;
	struct tty_buffer *tb;

	spin_lock_irqsave(&tty->buf.lock, __flags);
	space = __tty_buffer_request_room(tty, size);

	tb = tty->buf.tail;
	if (likely(space)) {
		*chars = tb->char_buf_ptr + tb->used;
		*flags = tb->flag_buf_ptr + tb->used;
		tb->used += space;
	}
	spin_unlock_irqrestore(&tty->buf.lock, __flags);
	return space;
}
EXPORT_SYMBOL_GPL(tty_prepare_flip_string_flags);




static void flush_to_ldisc(struct work_struct *work)
{
	struct tty_struct *tty =
		container_of(work, struct tty_struct, buf.work);
	unsigned long 	flags;
	struct tty_ldisc *disc;

	disc = tty_ldisc_ref(tty);
	if (disc == NULL)	
		return;

	spin_lock_irqsave(&tty->buf.lock, flags);

	if (!test_and_set_bit(TTY_FLUSHING, &tty->flags)) {
		struct tty_buffer *head;
		while ((head = tty->buf.head) != NULL) {
			int count;
			char *char_buf;
			unsigned char *flag_buf;
			int yy;
			unsigned int left = 0;
			unsigned int max_space;

			count = head->commit - head->read;
			if (!count) {
				if (head->next == NULL)
					break;
				tty->buf.head = head->next;
				tty_buffer_free(tty, head);
				continue;
			}
			if (test_bit(TTY_FLUSHPENDING, &tty->flags))
				break;

			
			spin_lock(&tty->read_lock);
			if (tty->update_room_in_ldisc) {
				if (tty->read_cnt == 4095 &&
					tty->receive_room == 4095) {
					yy = 1;
					tty->rr_bug++;
				}
				left = N_TTY_BUF_SIZE - tty->read_cnt - 1;
			}
			spin_unlock(&tty->read_lock);

			if (!tty->receive_room)
				break;

			if (tty->update_room_in_ldisc && !left) {
				schedule_work(&tty->buf.work);
				break;
			}

			if (tty->update_room_in_ldisc)
				max_space = min(left, tty->receive_room);
			else
				max_space = tty->receive_room;

			if (count > max_space)
				count = max_space;
			char_buf = head->char_buf_ptr + head->read;
			flag_buf = head->flag_buf_ptr + head->read;
			head->read += count;
			spin_unlock_irqrestore(&tty->buf.lock, flags);
			disc->ops->receive_buf(tty, char_buf,
							flag_buf, count);
			spin_lock_irqsave(&tty->buf.lock, flags);
		}
		clear_bit(TTY_FLUSHING, &tty->flags);
	}

	if (test_bit(TTY_FLUSHPENDING, &tty->flags)) {
		__tty_buffer_flush(tty);
		clear_bit(TTY_FLUSHPENDING, &tty->flags);
		wake_up(&tty->read_wait);
	}
	spin_unlock_irqrestore(&tty->buf.lock, flags);

	tty_ldisc_deref(disc);
}

void tty_flush_to_ldisc(struct tty_struct *tty)
{
	flush_work(&tty->buf.work);
}


void tty_flip_buffer_push(struct tty_struct *tty)
{
	unsigned long flags;
	spin_lock_irqsave(&tty->buf.lock, flags);
	if (tty->buf.tail != NULL)
		tty->buf.tail->commit = tty->buf.tail->used;
	spin_unlock_irqrestore(&tty->buf.lock, flags);

	if (tty->low_latency)
		flush_to_ldisc(&tty->buf.work);
	else {
#if defined(CONFIG_MSM_SMD0_WQ)
		if (!strcmp(tty->name, "smd0"))
			queue_work(tty_wq, &tty->buf.work);
		else
#endif
		schedule_work(&tty->buf.work);
	}
}
EXPORT_SYMBOL(tty_flip_buffer_push);


void tty_buffer_init(struct tty_struct *tty)
{
	spin_lock_init(&tty->buf.lock);
	tty->buf.head = NULL;
	tty->buf.tail = NULL;
	tty->buf.free = NULL;
	tty->buf.memory_used = 0;
	INIT_WORK(&tty->buf.work, flush_to_ldisc);
}

