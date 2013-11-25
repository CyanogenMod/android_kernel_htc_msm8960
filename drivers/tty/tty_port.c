
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/module.h>

void tty_port_init(struct tty_port *port)
{
	memset(port, 0, sizeof(*port));
	init_waitqueue_head(&port->open_wait);
	init_waitqueue_head(&port->close_wait);
	init_waitqueue_head(&port->delta_msr_wait);
	mutex_init(&port->mutex);
	mutex_init(&port->buf_mutex);
	spin_lock_init(&port->lock);
	port->close_delay = (50 * HZ) / 100;
	port->closing_wait = (3000 * HZ) / 100;
	kref_init(&port->kref);
}
EXPORT_SYMBOL(tty_port_init);

int tty_port_alloc_xmit_buf(struct tty_port *port)
{
	
	mutex_lock(&port->buf_mutex);
	if (port->xmit_buf == NULL)
		port->xmit_buf = (unsigned char *)get_zeroed_page(GFP_KERNEL);
	mutex_unlock(&port->buf_mutex);
	if (port->xmit_buf == NULL)
		return -ENOMEM;
	return 0;
}
EXPORT_SYMBOL(tty_port_alloc_xmit_buf);

void tty_port_free_xmit_buf(struct tty_port *port)
{
	mutex_lock(&port->buf_mutex);
	if (port->xmit_buf != NULL) {
		free_page((unsigned long)port->xmit_buf);
		port->xmit_buf = NULL;
	}
	mutex_unlock(&port->buf_mutex);
}
EXPORT_SYMBOL(tty_port_free_xmit_buf);

static void tty_port_destructor(struct kref *kref)
{
	struct tty_port *port = container_of(kref, struct tty_port, kref);
	if (port->xmit_buf)
		free_page((unsigned long)port->xmit_buf);
	if (port->ops->destruct)
		port->ops->destruct(port);
	else
		kfree(port);
}

void tty_port_put(struct tty_port *port)
{
	if (port)
		kref_put(&port->kref, tty_port_destructor);
}
EXPORT_SYMBOL(tty_port_put);


struct tty_struct *tty_port_tty_get(struct tty_port *port)
{
	unsigned long flags;
	struct tty_struct *tty;

	spin_lock_irqsave(&port->lock, flags);
	tty = tty_kref_get(port->tty);
	spin_unlock_irqrestore(&port->lock, flags);
	return tty;
}
EXPORT_SYMBOL(tty_port_tty_get);


void tty_port_tty_set(struct tty_port *port, struct tty_struct *tty)
{
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	if (port->tty)
		tty_kref_put(port->tty);
	port->tty = tty_kref_get(tty);
	spin_unlock_irqrestore(&port->lock, flags);
}
EXPORT_SYMBOL(tty_port_tty_set);

static void tty_port_shutdown(struct tty_port *port)
{
	mutex_lock(&port->mutex);
	if (port->ops->shutdown && !port->console &&
		test_and_clear_bit(ASYNCB_INITIALIZED, &port->flags))
			port->ops->shutdown(port);
	mutex_unlock(&port->mutex);
}


void tty_port_hangup(struct tty_port *port)
{
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	port->count = 0;
	port->flags &= ~ASYNC_NORMAL_ACTIVE;
	if (port->tty) {
		set_bit(TTY_IO_ERROR, &port->tty->flags);
		tty_kref_put(port->tty);
	}
	port->tty = NULL;
	spin_unlock_irqrestore(&port->lock, flags);
	wake_up_interruptible(&port->open_wait);
	wake_up_interruptible(&port->delta_msr_wait);
	tty_port_shutdown(port);
}
EXPORT_SYMBOL(tty_port_hangup);


int tty_port_carrier_raised(struct tty_port *port)
{
	if (port->ops->carrier_raised == NULL)
		return 1;
	return port->ops->carrier_raised(port);
}
EXPORT_SYMBOL(tty_port_carrier_raised);


void tty_port_raise_dtr_rts(struct tty_port *port)
{
	if (port->ops->dtr_rts)
		port->ops->dtr_rts(port, 1);
}
EXPORT_SYMBOL(tty_port_raise_dtr_rts);


void tty_port_lower_dtr_rts(struct tty_port *port)
{
	if (port->ops->dtr_rts)
		port->ops->dtr_rts(port, 0);
}
EXPORT_SYMBOL(tty_port_lower_dtr_rts);


int tty_port_block_til_ready(struct tty_port *port,
				struct tty_struct *tty, struct file *filp)
{
	int do_clocal = 0, retval;
	unsigned long flags;
	DEFINE_WAIT(wait);

	
	if (tty_hung_up_p(filp) || port->flags & ASYNC_CLOSING) {
		wait_event_interruptible_tty(port->close_wait,
				!(port->flags & ASYNC_CLOSING));
		if (port->flags & ASYNC_HUP_NOTIFY)
			return -EAGAIN;
		else
			return -ERESTARTSYS;
	}

	if (tty->flags & (1 << TTY_IO_ERROR)) {
		port->flags |= ASYNC_NORMAL_ACTIVE;
		return 0;
	}
	if (filp->f_flags & O_NONBLOCK) {
		
		if (tty->termios->c_cflag & CBAUD)
			tty_port_raise_dtr_rts(port);
		port->flags |= ASYNC_NORMAL_ACTIVE;
		return 0;
	}

	if (C_CLOCAL(tty))
		do_clocal = 1;


	retval = 0;

	
	spin_lock_irqsave(&port->lock, flags);
	if (!tty_hung_up_p(filp))
		port->count--;
	port->blocked_open++;
	spin_unlock_irqrestore(&port->lock, flags);

	while (1) {
		
		if (tty->termios->c_cflag & CBAUD)
			tty_port_raise_dtr_rts(port);

		prepare_to_wait(&port->open_wait, &wait, TASK_INTERRUPTIBLE);
		if (tty_hung_up_p(filp) || !(port->flags & ASYNC_INITIALIZED)) {
			if (port->flags & ASYNC_HUP_NOTIFY)
				retval = -EAGAIN;
			else
				retval = -ERESTARTSYS;
			break;
		}
		if (!(port->flags & ASYNC_CLOSING) &&
				(do_clocal || tty_port_carrier_raised(port)))
			break;
		if (signal_pending(current)) {
			retval = -ERESTARTSYS;
			break;
		}
		tty_unlock();
		schedule();
		tty_lock();
	}
	finish_wait(&port->open_wait, &wait);

	spin_lock_irqsave(&port->lock, flags);
	if (!tty_hung_up_p(filp))
		port->count++;
	port->blocked_open--;
	if (retval == 0)
		port->flags |= ASYNC_NORMAL_ACTIVE;
	spin_unlock_irqrestore(&port->lock, flags);
	return retval;
}
EXPORT_SYMBOL(tty_port_block_til_ready);

int tty_port_close_start(struct tty_port *port,
				struct tty_struct *tty, struct file *filp)
{
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	if (tty_hung_up_p(filp)) {
		spin_unlock_irqrestore(&port->lock, flags);
		return 0;
	}

	if (tty->count == 1 && port->count != 1) {
		printk(KERN_WARNING
		    "tty_port_close_start: tty->count = 1 port count = %d.\n",
								port->count);
		port->count = 1;
	}
	if (--port->count < 0) {
		printk(KERN_WARNING "tty_port_close_start: count = %d\n",
								port->count);
		port->count = 0;
	}

	if (port->count) {
		spin_unlock_irqrestore(&port->lock, flags);
		if (port->ops->drop)
			port->ops->drop(port);
		return 0;
	}
	set_bit(ASYNCB_CLOSING, &port->flags);
	tty->closing = 1;
	spin_unlock_irqrestore(&port->lock, flags);
	
	if (tty->flow_stopped)
		tty_driver_flush_buffer(tty);
	if (test_bit(ASYNCB_INITIALIZED, &port->flags) &&
			port->closing_wait != ASYNC_CLOSING_WAIT_NONE)
		tty_wait_until_sent_from_close(tty, port->closing_wait);
	if (port->drain_delay) {
		unsigned int bps = tty_get_baud_rate(tty);
		long timeout;

		if (bps > 1200)
			timeout = max_t(long,
				(HZ * 10 * port->drain_delay) / bps, HZ / 10);
		else
			timeout = 2 * HZ;
		schedule_timeout_interruptible(timeout);
	}
	
	tty_ldisc_flush(tty);

	if (tty->termios->c_cflag & HUPCL)
		tty_port_lower_dtr_rts(port);

	return 1;
}
EXPORT_SYMBOL(tty_port_close_start);

void tty_port_close_end(struct tty_port *port, struct tty_struct *tty)
{
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	tty->closing = 0;

	if (port->blocked_open) {
		spin_unlock_irqrestore(&port->lock, flags);
		if (port->close_delay) {
			msleep_interruptible(
				jiffies_to_msecs(port->close_delay));
		}
		spin_lock_irqsave(&port->lock, flags);
		wake_up_interruptible(&port->open_wait);
	}
	port->flags &= ~(ASYNC_NORMAL_ACTIVE | ASYNC_CLOSING);
	wake_up_interruptible(&port->close_wait);
	spin_unlock_irqrestore(&port->lock, flags);
}
EXPORT_SYMBOL(tty_port_close_end);

void tty_port_close(struct tty_port *port, struct tty_struct *tty,
							struct file *filp)
{
	if (tty_port_close_start(port, tty, filp) == 0)
		return;
	tty_port_shutdown(port);
	set_bit(TTY_IO_ERROR, &tty->flags);
	tty_port_close_end(port, tty);
	tty_port_tty_set(port, NULL);
}
EXPORT_SYMBOL(tty_port_close);

int tty_port_open(struct tty_port *port, struct tty_struct *tty,
							struct file *filp)
{
	spin_lock_irq(&port->lock);
	if (!tty_hung_up_p(filp))
		++port->count;
	spin_unlock_irq(&port->lock);
	tty_port_tty_set(port, tty);


	mutex_lock(&port->mutex);

	if (!test_bit(ASYNCB_INITIALIZED, &port->flags)) {
		clear_bit(TTY_IO_ERROR, &tty->flags);
		if (port->ops->activate) {
			int retval = port->ops->activate(port, tty);
			if (retval) {
				mutex_unlock(&port->mutex);
				return retval;
			}
		}
		set_bit(ASYNCB_INITIALIZED, &port->flags);
	}
	mutex_unlock(&port->mutex);
	return tty_port_block_til_ready(port, tty, filp);
}

EXPORT_SYMBOL(tty_port_open);
