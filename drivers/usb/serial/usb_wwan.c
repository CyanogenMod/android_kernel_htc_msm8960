/*
  USB Driver layer for GSM modems

  Copyright (C) 2005  Matthias Urlichs <smurf@smurf.noris.de>

  This driver is free software; you can redistribute it and/or modify
  it under the terms of Version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  Portions copied from the Keyspan driver by Hugh Blemings <hugh@blemings.org>

  History: see the git log.

  Work sponsored by: Sigos GmbH, Germany <info@sigos.de>

  This driver exists because the "normal" serial driver doesn't work too well
  with GSM modems. Issues:
  - data loss -- one single Receive URB is not nearly enough
  - controlling the baud rate doesn't make sense
*/

#define DRIVER_VERSION "v0.7.2"
#define DRIVER_AUTHOR "Matthias Urlichs <smurf@smurf.noris.de>"
#define DRIVER_DESC "USB Driver for GSM modems"

#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/module.h>
#include <linux/bitops.h>
#include <linux/uaccess.h>
#include <linux/usb.h>
#include <linux/usb/serial.h>
#include <linux/serial.h>
#include "usb-wwan.h"
#include <mach/board_htc.h>

static bool debug;

static bool usb_pm_debug_enabled = false;

#define HTC_USE_WWAN_BUFF_POOL

#ifdef HTC_USE_WWAN_BUFF_POOL
struct wwan_buff_list_elem {
	struct list_head	list;
	u8*		buff;
};

static struct list_head	wwan_in_buf_list;
static struct list_head	wwan_out_buf_list;
static struct mutex wwan_buf_mutex;
static bool wwan_buf_list_init = false;

static bool htc_put_wwan_buffer(u8* buff, struct list_head*	list_ptr)
{
	struct wwan_buff_list_elem	*list_elem = NULL;

	list_elem = kmalloc(sizeof(struct wwan_buff_list_elem), GFP_ATOMIC);
	if (!list_elem) {
		pr_info("[WWAN_POOL] %s(%d): list_elem alloc failed\n", __func__, __LINE__);
		return false;
	}
	list_elem->buff = buff;

	mutex_lock(&wwan_buf_mutex);
	list_add_tail(&list_elem->list, list_ptr);
	pr_info("[WWAN_POOL] %s(%d): list %p put %p\n", __func__, __LINE__, list_ptr, buff);
	mutex_unlock(&wwan_buf_mutex);

	return true;
}

static u8* htc_get_wwan_buffer(struct list_head* list_ptr)
{
	struct wwan_buff_list_elem	*list_elem = NULL;
	u8* buff = NULL;

	mutex_lock(&wwan_buf_mutex);

	if (!list_empty(list_ptr)) {
		list_elem = list_first_entry(list_ptr, struct wwan_buff_list_elem, list);
		buff = list_elem->buff;
		list_del(&list_elem->list);
		kfree(list_elem);

		pr_info("[WWAN_POOL] %s(%d): list %p get %p\n", __func__, __LINE__, list_ptr, buff);
	}
	else {
		pr_info("[WWAN_POOL] %s(%d): list %p empty\n", __func__, __LINE__, list_ptr);
	}

	mutex_unlock(&wwan_buf_mutex);

	return buff;
}
#endif 


void usb_wwan_dtr_rts(struct usb_serial_port *port, int on)
{
	struct usb_serial *serial = port->serial;
	struct usb_wwan_port_private *portdata;

	struct usb_wwan_intf_private *intfdata;

	dbg("%s", __func__);

	intfdata = port->serial->private;

	if (!intfdata->send_setup)
		return;

	portdata = usb_get_serial_port_data(port);
	mutex_lock(&serial->disc_mutex);
	portdata->rts_state = on;
	portdata->dtr_state = on;
	if (serial->dev)
		intfdata->send_setup(port);
	mutex_unlock(&serial->disc_mutex);
}
EXPORT_SYMBOL(usb_wwan_dtr_rts);

void usb_wwan_set_termios(struct tty_struct *tty,
			  struct usb_serial_port *port,
			  struct ktermios *old_termios)
{
	struct usb_wwan_intf_private *intfdata = port->serial->private;

	dbg("%s", __func__);

	
	tty_termios_copy_hw(tty->termios, old_termios);

	if (intfdata->send_setup)
		intfdata->send_setup(port);
}
EXPORT_SYMBOL(usb_wwan_set_termios);

int usb_wwan_tiocmget(struct tty_struct *tty)
{
	struct usb_serial_port *port = tty->driver_data;
	unsigned int value;
	struct usb_wwan_port_private *portdata;

	portdata = usb_get_serial_port_data(port);

	value = ((portdata->rts_state) ? TIOCM_RTS : 0) |
	    ((portdata->dtr_state) ? TIOCM_DTR : 0) |
	    ((portdata->cts_state) ? TIOCM_CTS : 0) |
	    ((portdata->dsr_state) ? TIOCM_DSR : 0) |
	    ((portdata->dcd_state) ? TIOCM_CAR : 0) |
	    ((portdata->ri_state) ? TIOCM_RNG : 0);

	return value;
}
EXPORT_SYMBOL(usb_wwan_tiocmget);

int usb_wwan_tiocmset(struct tty_struct *tty,
		      unsigned int set, unsigned int clear)
{
	struct usb_serial_port *port = tty->driver_data;
	struct usb_wwan_port_private *portdata;
	struct usb_wwan_intf_private *intfdata;

	portdata = usb_get_serial_port_data(port);
	intfdata = port->serial->private;

	if (!intfdata->send_setup)
		return -EINVAL;

	
	if (set & TIOCM_RTS)
		portdata->rts_state = 1;
	if (set & TIOCM_DTR)
		portdata->dtr_state = 1;

	if (clear & TIOCM_RTS)
		portdata->rts_state = 0;
	if (clear & TIOCM_DTR)
		portdata->dtr_state = 0;
	return intfdata->send_setup(port);
}
EXPORT_SYMBOL(usb_wwan_tiocmset);

static int get_serial_info(struct usb_serial_port *port,
			   struct serial_struct __user *retinfo)
{
	struct serial_struct tmp;

	if (!retinfo)
		return -EFAULT;

	memset(&tmp, 0, sizeof(tmp));
	tmp.line            = port->serial->minor;
	tmp.port            = port->number;
	tmp.baud_base       = tty_get_baud_rate(port->port.tty);
	tmp.close_delay	    = port->port.close_delay / 10;
	tmp.closing_wait    = port->port.closing_wait == ASYNC_CLOSING_WAIT_NONE ?
				 ASYNC_CLOSING_WAIT_NONE :
				 port->port.closing_wait / 10;

	if (copy_to_user(retinfo, &tmp, sizeof(*retinfo)))
		return -EFAULT;
	return 0;
}

static int set_serial_info(struct usb_serial_port *port,
			   struct serial_struct __user *newinfo)
{
	struct serial_struct new_serial;
	unsigned int closing_wait, close_delay;
	int retval = 0;

	if (copy_from_user(&new_serial, newinfo, sizeof(new_serial)))
		return -EFAULT;

	close_delay = new_serial.close_delay * 10;
	closing_wait = new_serial.closing_wait == ASYNC_CLOSING_WAIT_NONE ?
			ASYNC_CLOSING_WAIT_NONE : new_serial.closing_wait * 10;

	mutex_lock(&port->port.mutex);

	if (!capable(CAP_SYS_ADMIN)) {
		if ((close_delay != port->port.close_delay) ||
		    (closing_wait != port->port.closing_wait))
			retval = -EPERM;
		else
			retval = -EOPNOTSUPP;
	} else {
		port->port.close_delay  = close_delay;
		port->port.closing_wait = closing_wait;
	}

	mutex_unlock(&port->port.mutex);
	return retval;
}

int usb_wwan_ioctl(struct tty_struct *tty,
		   unsigned int cmd, unsigned long arg)
{
	struct usb_serial_port *port = tty->driver_data;

	dbg("%s cmd 0x%04x", __func__, cmd);

	switch (cmd) {
	case TIOCGSERIAL:
		return get_serial_info(port,
				       (struct serial_struct __user *) arg);
	case TIOCSSERIAL:
		return set_serial_info(port,
				       (struct serial_struct __user *) arg);
	default:
		break;
	}

	dbg("%s arg not supported", __func__);

	return -ENOIOCTLCMD;
}
EXPORT_SYMBOL(usb_wwan_ioctl);

int usb_wwan_write(struct tty_struct *tty, struct usb_serial_port *port,
		   const unsigned char *buf, int count)
{
	struct usb_wwan_port_private *portdata;
	struct usb_wwan_intf_private *intfdata;
	int i;
	int left, todo;
	struct urb *this_urb = NULL;	
	int err;
	unsigned long flags;

	portdata = usb_get_serial_port_data(port);
	intfdata = port->serial->private;

	dbg("%s: write (%d chars)", __func__, count);

	i = 0;
	left = count;
	for (i = 0; left > 0 && i < N_OUT_URB; i++) {
		todo = left;
		if (todo > OUT_BUFLEN)
			todo = OUT_BUFLEN;

		this_urb = portdata->out_urbs[i];
		if (test_and_set_bit(i, &portdata->out_busy)) {
			if (time_before(jiffies,
					portdata->tx_start_time[i] + 10 * HZ))
				continue;
			usb_unlink_urb(this_urb);
			continue;
		}
		dbg("%s: endpoint %d buf %d", __func__,
		    usb_pipeendpoint(this_urb->pipe), i);

		err = usb_autopm_get_interface_async(port->serial->interface);
		if (err < 0)
			break;

		
		memcpy(this_urb->transfer_buffer, buf, todo);
		this_urb->transfer_buffer_length = todo;

		spin_lock_irqsave(&intfdata->susp_lock, flags);
		if (intfdata->suspended) {
			usb_anchor_urb(this_urb, &portdata->delayed);
			spin_unlock_irqrestore(&intfdata->susp_lock, flags);
		} else {
			intfdata->in_flight++;
			spin_unlock_irqrestore(&intfdata->susp_lock, flags);
			usb_anchor_urb(this_urb, &portdata->submitted);
			err = usb_submit_urb(this_urb, GFP_ATOMIC);
			if (err) {
				dbg("usb_submit_urb %p (write bulk) failed "
				    "(%d)", this_urb, err);
				usb_unanchor_urb(this_urb);
				clear_bit(i, &portdata->out_busy);
				spin_lock_irqsave(&intfdata->susp_lock, flags);
				intfdata->in_flight--;
				spin_unlock_irqrestore(&intfdata->susp_lock,
						       flags);
				usb_autopm_put_interface_async(port->serial->interface);
				break;
			}
		}

		portdata->tx_start_time[i] = jiffies;
		buf += todo;
		left -= todo;
	}

	count -= left;
	dbg("%s: wrote (did %d)", __func__, count);
	return count;
}
EXPORT_SYMBOL(usb_wwan_write);
static void usb_wwan_indat_callback(struct urb *urb)
{
	int err;
	int endpoint;
	struct usb_wwan_port_private *portdata;
	struct usb_serial_port *port;
	struct tty_struct *tty;
	unsigned char *data = urb->transfer_buffer;
	int status = urb->status;

	dbg("%s: %p", __func__, urb);

	endpoint = usb_pipeendpoint(urb->pipe);
	port = urb->context;
	portdata = usb_get_serial_port_data(port);

	if (status && (status != -ENOENT)) {
		dbg("%s: nonzero status: %d on endpoint %02x.",
		    __func__, status, endpoint);
	} else {
		tty = tty_port_tty_get(&port->port);
		if (tty) {
			if (urb->actual_length) {
				tty_insert_flip_string(tty, data,
						urb->actual_length);
				tty_flip_buffer_push(tty);
			} 
			tty_kref_put(tty);
		} else {
			err("%s: no tty to receive data!!! \n", __func__);
		}

		
		if (status != -ESHUTDOWN) {
			usb_anchor_urb(urb, &portdata->submitted);
			err = usb_submit_urb(urb, GFP_ATOMIC);
			if (err) {
				usb_unanchor_urb(urb);
				if (err != -EPERM) {
					printk(KERN_ERR "%s: resubmit read urb failed. "
						"(%d)", __func__, err);
					
					
#ifdef HTC_PM_DBG
					if (usb_pm_debug_enabled)
						usb_mark_intf_last_busy(port->serial->interface, false);
#endif
					
					usb_mark_last_busy(port->serial->dev);
				}
			} else {
				
#ifdef HTC_PM_DBG
				if (usb_pm_debug_enabled)
					usb_mark_intf_last_busy(port->serial->interface, false);
#endif
				
				usb_mark_last_busy(port->serial->dev);
			}
		}

	}
}

static void usb_wwan_outdat_callback(struct urb *urb)
{
	struct usb_serial_port *port;
	struct usb_wwan_port_private *portdata;
	struct usb_wwan_intf_private *intfdata;
	int i;

	dbg("%s", __func__);

	port = urb->context;
	intfdata = port->serial->private;

	usb_serial_port_softint(port);
	usb_autopm_put_interface_async(port->serial->interface);
	portdata = usb_get_serial_port_data(port);
	spin_lock(&intfdata->susp_lock);
	intfdata->in_flight--;
	spin_unlock(&intfdata->susp_lock);

	for (i = 0; i < N_OUT_URB; ++i) {
		if (portdata->out_urbs[i] == urb) {
			smp_mb__before_clear_bit();
			clear_bit(i, &portdata->out_busy);
			break;
		}
	}
}

int usb_wwan_write_room(struct tty_struct *tty)
{
	struct usb_serial_port *port = tty->driver_data;
	struct usb_wwan_port_private *portdata;
	int i;
	int data_len = 0;
	struct urb *this_urb;

	portdata = usb_get_serial_port_data(port);

	for (i = 0; i < N_OUT_URB; i++) {
		this_urb = portdata->out_urbs[i];
		if (this_urb && !test_bit(i, &portdata->out_busy))
			data_len += OUT_BUFLEN;
	}

	dbg("%s: %d", __func__, data_len);
	return data_len;
}
EXPORT_SYMBOL(usb_wwan_write_room);

int usb_wwan_chars_in_buffer(struct tty_struct *tty)
{
	struct usb_serial_port *port = tty->driver_data;
	struct usb_wwan_port_private *portdata;
	int i;
	int data_len = 0;
	struct urb *this_urb;

	portdata = usb_get_serial_port_data(port);

	for (i = 0; i < N_OUT_URB; i++) {
		this_urb = portdata->out_urbs[i];
		if (this_urb && test_bit(i, &portdata->out_busy))
			data_len += this_urb->transfer_buffer_length;
	}
	dbg("%s: %d", __func__, data_len);
	return data_len;
}
EXPORT_SYMBOL(usb_wwan_chars_in_buffer);

void usb_wwan_throttle(struct tty_struct *tty)
{
	struct usb_serial_port *port = tty->driver_data;

	port->throttle_req = true;

	dbg("%s:\n", __func__);
}
EXPORT_SYMBOL(usb_wwan_throttle);

void usb_wwan_unthrottle(struct tty_struct *tty)
{
	struct usb_serial_port *port = tty->driver_data;
	struct usb_wwan_port_private *portdata;

	portdata = usb_get_serial_port_data(port);

	dbg("%s:\n", __func__);
	port->throttle_req = false;
	port->throttled = false;

	
	
	
}
EXPORT_SYMBOL(usb_wwan_unthrottle);

int usb_wwan_open(struct tty_struct *tty, struct usb_serial_port *port)
{
	struct usb_wwan_port_private *portdata;
	struct usb_wwan_intf_private *intfdata;
	struct usb_serial *serial = port->serial;
	int i, err;
	struct urb *urb;

	portdata = usb_get_serial_port_data(port);
	intfdata = serial->private;

	
	tty->raw = 1;
	tty->real_raw = 1;
	tty->update_room_in_ldisc = 1;

	set_bit(TTY_NO_WRITE_SPLIT, &tty->flags);
	pr_info("%s", __func__);

	
	for (i = 0; i < N_IN_URB; i++) {
		urb = portdata->in_urbs[i];
		if (!urb)
			continue;
		usb_anchor_urb(urb, &portdata->submitted);
		err = usb_submit_urb(urb, GFP_KERNEL);
		if (err) {
			usb_unanchor_urb(urb);
			dbg("%s: submit urb %d failed (%d) %d",
			    __func__, i, err, urb->transfer_buffer_length);
		}
	}

	if (intfdata->send_setup)
		intfdata->send_setup(port);

	serial->interface->needs_remote_wakeup = 1;
	spin_lock_irq(&intfdata->susp_lock);
	portdata->opened = 1;
	spin_unlock_irq(&intfdata->susp_lock);
	
	usb_autopm_put_interface(serial->interface);

	return 0;
}
EXPORT_SYMBOL(usb_wwan_open);

void usb_wwan_close(struct usb_serial_port *port)
{
	int i;
	struct usb_serial *serial = port->serial;
	struct usb_wwan_port_private *portdata;
	struct usb_wwan_intf_private *intfdata = port->serial->private;

	pr_info("%s", __func__);
	portdata = usb_get_serial_port_data(port);

	if (serial->dev) {
		
		spin_lock_irq(&intfdata->susp_lock);
		portdata->opened = 0;
		spin_unlock_irq(&intfdata->susp_lock);

		for (i = 0; i < N_IN_URB; i++)
			usb_kill_urb(portdata->in_urbs[i]);
		for (i = 0; i < N_OUT_URB; i++)
			usb_kill_urb(portdata->out_urbs[i]);
		
		usb_autopm_get_interface_no_resume(serial->interface);
		
		
	}
}
EXPORT_SYMBOL(usb_wwan_close);

static struct urb *usb_wwan_setup_urb(struct usb_serial *serial, int endpoint,
				      int dir, void *ctx, char *buf, int len,
				      void (*callback) (struct urb *))
{
	struct urb *urb;

	if (endpoint == -1)
		return NULL;	

	urb = usb_alloc_urb(0, GFP_KERNEL);	
	if (urb == NULL) {
		dbg("%s: alloc for endpoint %d failed.", __func__, endpoint);
		return NULL;
	}

	
	usb_fill_bulk_urb(urb, serial->dev,
			  usb_sndbulkpipe(serial->dev, endpoint) | dir,
			  buf, len, callback, ctx);

	return urb;
}

static void usb_wwan_setup_urbs(struct usb_serial *serial)
{
	int i, j;
	struct usb_serial_port *port;
	struct usb_wwan_port_private *portdata;

	dbg("%s", __func__);

	for (i = 0; i < serial->num_ports; i++) {
		port = serial->port[i];
		portdata = usb_get_serial_port_data(port);

		
		for (j = 0; j < N_IN_URB; ++j) {
			portdata->in_urbs[j] = usb_wwan_setup_urb(serial,
								  port->
								  bulk_in_endpointAddress,
								  USB_DIR_IN,
								  port,
								  portdata->
								  in_buffer[j],
								  IN_BUFLEN,
								  usb_wwan_indat_callback);
		}

		
		for (j = 0; j < N_OUT_URB; ++j) {
			portdata->out_urbs[j] = usb_wwan_setup_urb(serial,
								   port->
								   bulk_out_endpointAddress,
								   USB_DIR_OUT,
								   port,
								   portdata->
								   out_buffer
								   [j],
								   OUT_BUFLEN,
								   usb_wwan_outdat_callback);
		}
	}
}

int usb_wwan_startup(struct usb_serial *serial)
{
	int i, j, err;
	struct usb_serial_port *port;
	struct usb_wwan_port_private *portdata;
	u8 *buffer;

	dbg("%s", __func__);

#ifdef HTC_USE_WWAN_BUFF_POOL
	if (!wwan_buf_list_init)
	{
		wwan_buf_list_init = true;
		mutex_init(&wwan_buf_mutex);
		pr_info("[WWAN_POOL] init wwan_in_buf_list:%p\n", &wwan_in_buf_list);
		INIT_LIST_HEAD(&wwan_in_buf_list);
		pr_info("[WWAN_POOL] init wwan_out_buf_list:%p\n", &wwan_out_buf_list);
		INIT_LIST_HEAD(&wwan_out_buf_list);
	}
#endif 

	if (get_radio_flag() & 0x0001)
		usb_pm_debug_enabled = true;

	
	for (i = 0; i < serial->num_ports; i++) {
		port = serial->port[i];
		portdata = kzalloc(sizeof(*portdata), GFP_KERNEL);
		if (!portdata) {
			dbg("%s: kmalloc for usb_wwan_port_private (%d) failed!.",
			    __func__, i);
			return 1;
		}
		init_usb_anchor(&portdata->delayed);
		init_usb_anchor(&portdata->submitted);
		
		
		
		INIT_LIST_HEAD(&portdata->in_urb_list);
		spin_lock_init(&portdata->in_lock);

		for (j = 0; j < N_IN_URB; j++) {
#ifdef HTC_USE_WWAN_BUFF_POOL
			buffer = htc_get_wwan_buffer(&wwan_in_buf_list);
			if (!buffer) {
				buffer = kmalloc(IN_BUFLEN, GFP_KERNEL);
				pr_info("[WWAN_POOL] alloc in_buffer:%p size:%d\n", buffer, IN_BUFLEN);
			}
#else 
			buffer = kmalloc(IN_BUFLEN, GFP_KERNEL);
#endif 
			if (!buffer)
				goto bail_out_error;
			portdata->in_buffer[j] = buffer;
		}

		for (j = 0; j < N_OUT_URB; j++) {
#ifdef HTC_USE_WWAN_BUFF_POOL
			buffer = htc_get_wwan_buffer(&wwan_out_buf_list);
			if (!buffer) {
				buffer = kmalloc(OUT_BUFLEN, GFP_KERNEL);
				pr_info("[WWAN_POOL] alloc out_buffer:%p size:%d\n", buffer, OUT_BUFLEN);
			}
#else 
			buffer = kmalloc(OUT_BUFLEN, GFP_KERNEL);
#endif 
			if (!buffer)
				goto bail_out_error2;
			portdata->out_buffer[j] = buffer;
		}

		usb_set_serial_port_data(port, portdata);

		if (!port->interrupt_in_urb)
			continue;
		err = usb_submit_urb(port->interrupt_in_urb, GFP_KERNEL);
		if (err)
			dbg("%s: submit irq_in urb failed %d", __func__, err);
	}
	usb_wwan_setup_urbs(serial);

	
	serial->interface->needs_remote_wakeup = 1;
	

	return 0;

bail_out_error2:
	for (j = 0; j < N_OUT_URB; j++)
		kfree(portdata->out_buffer[j]);
bail_out_error:
	for (j = 0; j < N_IN_URB; j++)
		kfree(portdata->in_buffer[j]);
	kfree(portdata);
	return 1;
}
EXPORT_SYMBOL(usb_wwan_startup);

static void stop_read_write_urbs(struct usb_serial *serial)
{
	int i;
	struct usb_serial_port *port;
	struct usb_wwan_port_private *portdata;

	
	for (i = 0; i < serial->num_ports; ++i) {
		port = serial->port[i];
		portdata = usb_get_serial_port_data(port);
		usb_kill_anchored_urbs(&portdata->submitted);
	}
}

void usb_wwan_disconnect(struct usb_serial *serial)
{
	dbg("%s", __func__);

	stop_read_write_urbs(serial);
}
EXPORT_SYMBOL(usb_wwan_disconnect);

void usb_wwan_release(struct usb_serial *serial)
{
	int i, j;
	struct usb_serial_port *port;
	struct usb_wwan_port_private *portdata;
	struct urb *urb;
	struct list_head *q;
	unsigned long flags;

	
	for (i = 0; i < serial->num_ports; ++i) {
		port = serial->port[i];
		portdata = usb_get_serial_port_data(port);
		
		
		
		
		spin_lock_irqsave(&portdata->in_lock, flags);
		q = &portdata->in_urb_list;
		while (!list_empty(q)) {
			urb = list_first_entry(q, struct urb, urb_list);
			list_del_init(&urb->urb_list);
		}
		spin_unlock_irqrestore(&portdata->in_lock, flags);

		for (j = 0; j < N_IN_URB; j++) {
			usb_free_urb(portdata->in_urbs[j]);
#ifdef HTC_USE_WWAN_BUFF_POOL
			if (!htc_put_wwan_buffer(portdata->in_buffer[j], &wwan_in_buf_list)) {
				pr_info("[WWAN_POOL] free in_buffer:%p\n", portdata->in_buffer[j]);
				kfree(portdata->in_buffer[j]);
			}
#else 
			kfree(portdata->in_buffer[j]);
#endif 
			portdata->in_urbs[j] = NULL;
		}
		for (j = 0; j < N_OUT_URB; j++) {
			usb_free_urb(portdata->out_urbs[j]);
#ifdef HTC_USE_WWAN_BUFF_POOL
			if (!htc_put_wwan_buffer(portdata->out_buffer[j], &wwan_out_buf_list)) {
				pr_info("[WWAN_POOL] free out_buffer:%p\n", portdata->out_buffer[j]);
				kfree(portdata->out_buffer[j]);
			}
#else 
			kfree(portdata->out_buffer[j]);
#endif 
			portdata->out_urbs[j] = NULL;
		}
	}

	
	for (i = 0; i < serial->num_ports; i++) {
		port = serial->port[i];
		kfree(usb_get_serial_port_data(port));
	}
}
EXPORT_SYMBOL(usb_wwan_release);

#ifdef CONFIG_PM
int usb_wwan_suspend(struct usb_serial *serial, pm_message_t message)
{
	struct usb_wwan_intf_private *intfdata = serial->private;
	int b;

	dbg("%s entered", __func__);

	if (PMSG_IS_AUTO(message)) {
		spin_lock_irq(&intfdata->susp_lock);
		b = intfdata->in_flight;
		spin_unlock_irq(&intfdata->susp_lock);

		if (b)
			return -EBUSY;
	}

	spin_lock_irq(&intfdata->susp_lock);
	intfdata->suspended = 1;
	spin_unlock_irq(&intfdata->susp_lock);
	stop_read_write_urbs(serial);

	return 0;
}
EXPORT_SYMBOL(usb_wwan_suspend);

static void unbusy_queued_urb(struct urb *urb, struct usb_wwan_port_private *portdata)
{
	int i;

	for (i = 0; i < N_OUT_URB; i++) {
		if (urb == portdata->out_urbs[i]) {
			clear_bit(i, &portdata->out_busy);
			break;
		}
	}
}

static void play_delayed(struct usb_serial_port *port)
{
	struct usb_wwan_intf_private *data;
	struct usb_wwan_port_private *portdata;
	struct urb *urb;
	int err;

	portdata = usb_get_serial_port_data(port);
	data = port->serial->private;
	while ((urb = usb_get_from_anchor(&portdata->delayed))) {
		usb_anchor_urb(urb, &portdata->submitted);
		err = usb_submit_urb(urb, GFP_ATOMIC);
		if (!err) {
			data->in_flight++;
		} else {
			usb_unanchor_urb(urb);
			
			do {
				unbusy_queued_urb(urb, portdata);
				usb_autopm_put_interface_no_suspend(port->serial->interface);
			} while ((urb = usb_get_from_anchor(&portdata->delayed)));
			break;
		}
	}
}

int usb_wwan_resume(struct usb_serial *serial)
{
	int i, j;
	struct usb_serial_port *port;
	struct usb_wwan_intf_private *intfdata = serial->private;
	struct usb_wwan_port_private *portdata;
	struct urb *urb;
	int err = 0;

	dbg("%s entered", __func__);
	
	for (i = 0; i < serial->num_ports; i++) {
		port = serial->port[i];
		if (!port->interrupt_in_urb) {
			dbg("%s: No interrupt URB for port %d", __func__, i);
			continue;
		}
		err = usb_submit_urb(port->interrupt_in_urb, GFP_NOIO);
		dbg("Submitted interrupt URB for port %d (result %d)", i, err);
		if (err < 0) {
			err("%s: Error %d for interrupt URB of port%d",
			    __func__, err, i);
			goto err_out;
		}
	}

	spin_lock_irq(&intfdata->susp_lock);
	intfdata->suspended = 0;
	for (i = 0; i < serial->num_ports; i++) {
		
		port = serial->port[i];
		portdata = usb_get_serial_port_data(port);

		
		if (!portdata->opened)
			continue;

		for (j = 0; j < N_IN_URB; j++) {
			urb = portdata->in_urbs[j];
			usb_anchor_urb(urb, &portdata->submitted);
			err = usb_submit_urb(urb, GFP_ATOMIC);
			if (err < 0) {
				err("%s: Error %d for bulk URB %d",
				    __func__, err, i);
				usb_unanchor_urb(urb);
				intfdata->suspended = 1;
				spin_unlock_irq(&intfdata->susp_lock);
				goto err_out;
			}
		}
		play_delayed(port);
	}
	spin_unlock_irq(&intfdata->susp_lock);

err_out:
	return err;
}
EXPORT_SYMBOL(usb_wwan_resume);
#endif

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");

module_param(debug, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "Debug messages");
