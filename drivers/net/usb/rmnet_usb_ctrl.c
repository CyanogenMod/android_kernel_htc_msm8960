/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/termios.h>
#include <linux/poll.h>
#include <linux/ratelimit.h>
#include <linux/debugfs.h>
#include "rmnet_usb_ctrl.h"
#include <mach/board_htc.h>

#define DEVICE_NAME			"hsicctl"
#define HTC_HSIC_DEVICE_NAME		"htc_hsicctl"

#define NUM_CTRL_CHANNELS		4
#define DEFAULT_READ_URB_LENGTH		0x1000

#define HTC_RMNET_IOCTL_DEFINE 0xCC
#define HTC_RMNET_USB_SET_ITC _IOW(HTC_RMNET_IOCTL_DEFINE, 201, int)

#define ACM_CTRL_DTR		BIT(0)
#define ACM_CTRL_RTS		BIT(1)


#define ACM_CTRL_DSR		BIT(0)
#define ACM_CTRL_CTS		BIT(1)
#define ACM_CTRL_RI		BIT(2)
#define ACM_CTRL_CD		BIT(3)

#define HS_INTERVAL		7
#define FS_LS_INTERVAL		3

static bool usb_pm_debug_enabled = false;
static bool enable_ctl_msg_debug = false;


#ifdef HTC_LOG_RMNET_USB_CTRL
#define RMNET_DBG_MSG_LEN   100		
#define RMNET_DBG_MAX_MSG   512		
#define TIME_BUF_LEN  20

static bool enable_dbg_rmnet_usb_ctrl = false;

static struct {
	char     (buf[RMNET_DBG_MAX_MSG])[RMNET_DBG_MSG_LEN];   
	unsigned idx;   
	rwlock_t lck;   
} dbg_rmnet_usb_ctrl = {
	.idx = 0,
	.lck = __RW_LOCK_UNLOCKED(lck)
};

static char *rmnet_usb_ctrl_get_timestamp(char *tbuf)
{
	unsigned long long t;
	unsigned long nanosec_rem;

	t = cpu_clock(smp_processor_id());
	nanosec_rem = do_div(t, 1000000000)/1000;
	scnprintf(tbuf, TIME_BUF_LEN, "[%5lu.%06lu] ", (unsigned long)t,
		nanosec_rem);
	return tbuf;
}

static void rmnet_usb_ctrl_dbg_inc(unsigned* idx)
{
	*idx = (*idx + 1) & (RMNET_DBG_MAX_MSG-1);
}

static void log_rmnet_usb_ctrl_event(struct usb_interface	*intf, char * event, size_t size)
{
	unsigned long flags;
	char tbuf[TIME_BUF_LEN];
	__u8  bInterfaceNumber = 0;

	if (!enable_dbg_rmnet_usb_ctrl)
		return;

	if (intf && intf->cur_altsetting)
		bInterfaceNumber = intf->cur_altsetting->desc.bInterfaceNumber;

	write_lock_irqsave(&dbg_rmnet_usb_ctrl.lck, flags);
	scnprintf(dbg_rmnet_usb_ctrl.buf[dbg_rmnet_usb_ctrl.idx], RMNET_DBG_MSG_LEN,
		"%s:[%u]%s : %u", rmnet_usb_ctrl_get_timestamp(tbuf), bInterfaceNumber, event, size);
	rmnet_usb_ctrl_dbg_inc(&dbg_rmnet_usb_ctrl.idx);
	write_unlock_irqrestore(&dbg_rmnet_usb_ctrl.lck, flags);
}
#endif	

#ifdef HTC_DEBUG_QMI_STUCK
#define QMI_TX_NO_CB_TIMEOUT 5000
#define QMI_RX_NO_CB_TIMEOUT 3000
#include <linux/usb/hcd.h>

struct ctrl_write_context {
	struct timer_list timer;
	unsigned long start_jiffies;
	struct rmnet_ctrl_dev *dev;
};

static void do_mdm_restart_delay_work_fn(struct work_struct *work)
{
	extern struct usb_hcd *mdm_hsic_usb_hcd;

	if (mdm_hsic_usb_hcd)
	{
		if (&mdm_hsic_usb_hcd->ssr_work) {
			pr_info("[%s] queue_work ssr_work !!!\n", __func__);
			queue_work(system_nrt_wq, &mdm_hsic_usb_hcd->ssr_work);
		}
	}
}
static DECLARE_DELAYED_WORK(do_mdm_restart_delay_work, do_mdm_restart_delay_work_fn);
#endif	


static ssize_t modem_wait_store(struct device *d, struct device_attribute *attr,
		const char *buf, size_t n)
{
	unsigned int		mdm_wait;
	struct rmnet_ctrl_dev	*dev = dev_get_drvdata(d);

	if (!dev)
		return -ENODEV;

	sscanf(buf, "%u", &mdm_wait);

	dev->mdm_wait_timeout = mdm_wait;

	return n;
}

static ssize_t modem_wait_show(struct device *d, struct device_attribute *attr,
		char *buf)
{
	struct rmnet_ctrl_dev	*dev = dev_get_drvdata(d);

	if (!dev)
		return -ENODEV;

	return snprintf(buf, PAGE_SIZE, "%u\n", dev->mdm_wait_timeout);
}

static DEVICE_ATTR(modem_wait, 0664, modem_wait_show, modem_wait_store);

static int	ctl_msg_dbg_mask;
module_param_named(dump_ctrl_msg, ctl_msg_dbg_mask, int,
		S_IRUGO | S_IWUSR | S_IWGRP);

enum {
	MSM_USB_CTL_DEBUG = 1U << 0,
	MSM_USB_CTL_DUMP_BUFFER = 1U << 1,
};

#define DUMP_BUFFER(prestr, cnt, buf) \
do { \
	if ((ctl_msg_dbg_mask & MSM_USB_CTL_DUMP_BUFFER) || enable_ctl_msg_debug) \
			print_hex_dump(KERN_INFO, prestr, DUMP_PREFIX_NONE, \
					16, 1, buf, 11, false); \
} while (0)

#define DBG(x...) \
		do { \
			if (ctl_msg_dbg_mask & MSM_USB_CTL_DEBUG) \
				pr_info(x); \
		} while (0)

struct rmnet_ctrl_dev		*ctrl_dev[NUM_CTRL_CHANNELS];
struct class			*ctrldev_classp;
static dev_t			ctrldev_num;
struct device			*hsicdevicep;
struct class			*hsicctrldev_classp;
static int				htc_hsicctrldev_major;

struct ctrl_pkt {
	size_t	data_size;
	void	*data;
};

struct ctrl_pkt_list_elem {
	struct list_head	list;
	struct ctrl_pkt		cpkt;
};

static void resp_avail_cb(struct urb *);

static int is_dev_connected(struct rmnet_ctrl_dev *dev)
{
	if (dev) {
		mutex_lock(&dev->dev_lock);
		if (!dev->is_connected) {
			mutex_unlock(&dev->dev_lock);
			return 0;
		}
		mutex_unlock(&dev->dev_lock);
		return 1;
	}
	return 0;
}

#ifdef HTC_DEBUG_QMI_STUCK
static struct urb	*expired_rcvurb = NULL;
static void kill_rcv_urb_delay_work_fn(struct work_struct *work)
{
	if (expired_rcvurb) {
		pr_info("[RMNET][%s] +kill urb %p\n", __func__, expired_rcvurb);
		usb_kill_urb(expired_rcvurb);
		pr_info("[RMNET][%s] -kill urb %p\n", __func__, expired_rcvurb);
	}

#if 0
	
	schedule_delayed_work_on(0, &do_mdm_restart_delay_work, msecs_to_jiffies(3000));
#endif
}
static DECLARE_DELAYED_WORK(kill_rcv_urb_delay_work, kill_rcv_urb_delay_work_fn);

static void rmnet_usb_ctrl_read_timer_expire(unsigned long data)
{
	struct rmnet_ctrl_dev *dev = (struct rmnet_ctrl_dev *)data;

	dev_err(dev->devicep, "[RMNET][%s] %s rcv_timer expire !!! \n", __func__, dev->name);

	
	#ifdef HTC_LOG_RMNET_USB_CTRL
	if (dev)
		log_rmnet_usb_ctrl_event(dev->intf, "Rx timeout", 0);
	#endif	
	

	expired_rcvurb = dev->rcvurb;
	schedule_delayed_work_on(0, &kill_rcv_urb_delay_work, 10);
}
#endif	
static void get_encap_work(struct work_struct *w)
{
	struct usb_device	*udev;
	struct rmnet_ctrl_dev	*dev =
			container_of(w, struct rmnet_ctrl_dev, get_encap_work);
	int			status;

	udev = interface_to_usbdev(dev->intf);

	status = usb_autopm_get_interface(dev->intf);
	if (status < 0 && status != -EAGAIN && status != -EACCES) {
		dev->get_encap_failure_cnt++;
		return;
	}

#ifdef HTC_LOG_RMNET_USB_CTRL
	log_rmnet_usb_ctrl_event(dev->intf, "Rx", DEFAULT_READ_URB_LENGTH);
#endif	

#ifdef HTC_DEBUG_QMI_STUCK
	mod_timer(&dev->rcv_timer, jiffies + msecs_to_jiffies(QMI_RX_NO_CB_TIMEOUT));
#endif	

	usb_fill_control_urb(dev->rcvurb, udev,
				usb_rcvctrlpipe(udev, 0),
				(unsigned char *)dev->in_ctlreq,
				dev->rcvbuf,
				DEFAULT_READ_URB_LENGTH,
				resp_avail_cb, dev);


	usb_anchor_urb(dev->rcvurb, &dev->rx_submitted);
	status = usb_submit_urb(dev->rcvurb, GFP_KERNEL);
	if (status) {
#ifdef HTC_DEBUG_QMI_STUCK
		del_timer(&dev->rcv_timer);
#endif	
		dev->get_encap_failure_cnt++;
		usb_unanchor_urb(dev->rcvurb);
		usb_autopm_put_interface(dev->intf);
		dev_err(dev->devicep,
		"%s: Error submitting Read URB %d\n", __func__, status);
		goto resubmit_int_urb;
	}

	return;

resubmit_int_urb:
	
	if (!dev->inturb->anchor) {
		usb_anchor_urb(dev->inturb, &dev->rx_submitted);
		status = usb_submit_urb(dev->inturb, GFP_KERNEL);
		if (status) {
			usb_unanchor_urb(dev->inturb);
				dev_err(dev->devicep, "%s: Error re-submitting Int URB %d\n",
				__func__, status);
		}
	}

}

static void notification_available_cb(struct urb *urb)
{
	int				status;
	struct usb_cdc_notification	*ctrl;
	struct usb_device		*udev;
	struct rmnet_ctrl_dev		*dev = urb->context;

	udev = interface_to_usbdev(dev->intf);

	switch (urb->status) {
	case 0:
	
	case -ENOENT:
		
		break;

	
	case -ESHUTDOWN:
	case -ECONNRESET:
	case -EPROTO:
		return;
	case -EPIPE:
		pr_err_ratelimited("%s: Stall on int endpoint\n", __func__);
		
		return;

	
	case -EOVERFLOW:
		pr_err_ratelimited("%s: Babble error happened\n", __func__);
	default:
		 pr_debug_ratelimited("%s: Non zero urb status = %d\n",
			__func__, urb->status);
		goto resubmit_int_urb;
	}

	if (!urb->actual_length)
		return;

	ctrl = urb->transfer_buffer;

	switch (ctrl->bNotificationType) {
	case USB_CDC_NOTIFY_RESPONSE_AVAILABLE:
		dev->resp_avail_cnt++;
		
#ifdef HTC_PM_DBG
		if (usb_pm_debug_enabled)
			usb_mark_intf_last_busy(dev->intf, false);
#endif
		
		usb_mark_last_busy(udev);
		queue_work(dev->wq, &dev->get_encap_work);

		if (!dev->resp_available) {
			dev->resp_available = true;
			
			if (dev->intf)
				dev_info(&dev->intf->dev, "%s[%d]:dev->resp_available:%d\n", __func__, __LINE__, dev->resp_available);
			wake_up(&dev->open_wait_queue);
		}

		return;
	default:
		 dev_err(dev->devicep,
			"%s:Command not implemented\n", __func__);
	}

resubmit_int_urb:
	
#ifdef HTC_PM_DBG
	if (usb_pm_debug_enabled)
		usb_mark_intf_last_busy(dev->intf, false);
#endif
	
	usb_mark_last_busy(udev);
	usb_anchor_urb(urb, &dev->rx_submitted);
	status = usb_submit_urb(urb, GFP_ATOMIC);
	if (status) {
		usb_unanchor_urb(urb);
		dev_err(dev->devicep, "%s: Error re-submitting Int URB %d\n",
		__func__, status);
	}

	return;
}

static void resp_avail_cb(struct urb *urb)
{
	struct usb_device		*udev;
	struct ctrl_pkt_list_elem	*list_elem = NULL;
	struct rmnet_ctrl_dev		*dev = urb->context;
	void				*cpkt;
	int				status = 0;
	size_t				cpkt_size = 0;

	udev = interface_to_usbdev(dev->intf);

	usb_autopm_put_interface_async(dev->intf);
#ifdef HTC_DEBUG_QMI_STUCK
	del_timer(&dev->rcv_timer);
	if (expired_rcvurb == urb) {
		expired_rcvurb = NULL;
		dev_err(&(dev->intf->dev), "[RMNET] %s(%d) urb->status:%d urb->actual_length:%u !!!\n", __func__, __LINE__, urb->status, urb->actual_length);
		if (urb->status != 0) {
			if (urb->actual_length > 0) {
				dev_err(&(dev->intf->dev), "[RMNET] %s(%d) set urb->status 0 !!!\n", __func__, __LINE__);
				urb->status = 0;
			}
			else {
				dev_err(&(dev->intf->dev), "[RMNET] %s(%d) dev->inturb->anchor(%x) !!!\n", __func__, __LINE__, (dev->inturb) ? (unsigned int)dev->inturb->anchor : (unsigned int)(0xffffffff));
				dev_err(&(dev->intf->dev), "[RMNET] %s(%d) goto resubmit_int_urb !!!\n", __func__, __LINE__);
				goto resubmit_int_urb;
			}
		}
	}
#endif	

	switch (urb->status) {
	case 0:
		
		dev->get_encap_resp_cnt++;
		break;

	
	case -ESHUTDOWN:
	case -ENOENT:
	case -ECONNRESET:
	case -EPROTO:
		return;

	
	case -EOVERFLOW:
		pr_err_ratelimited("%s: Babble error happened\n", __func__);
	default:
		pr_debug_ratelimited("%s: Non zero urb status = %d\n",
				__func__, urb->status);
		goto resubmit_int_urb;
	}

	dev_dbg(dev->devicep, "Read %d bytes for %s\n",
		urb->actual_length, dev->name);

#ifdef HTC_LOG_RMNET_USB_CTRL
	log_rmnet_usb_ctrl_event(dev->intf, "Rx cb", urb->actual_length);
#endif	

	cpkt = urb->transfer_buffer;
	cpkt_size = urb->actual_length;
	if (!cpkt_size) {
		dev->zlp_cnt++;
		dev_dbg(dev->devicep, "%s: zero length pkt received\n",
				__func__);
		goto resubmit_int_urb;
	}

	list_elem = kmalloc(sizeof(struct ctrl_pkt_list_elem), GFP_ATOMIC);
	if (!list_elem) {
		dev_err(dev->devicep, "%s: list_elem alloc failed\n", __func__);
		return;
	}
	list_elem->cpkt.data = kmalloc(cpkt_size, GFP_ATOMIC);
	if (!list_elem->cpkt.data) {
		dev_err(dev->devicep, "%s: list_elem->data alloc failed\n",
			__func__);
		kfree(list_elem);
		return;
	}
	memcpy(list_elem->cpkt.data, cpkt, cpkt_size);
	list_elem->cpkt.data_size = cpkt_size;
	spin_lock(&dev->rx_lock);
	list_add_tail(&list_elem->list, &dev->rx_list);
	spin_unlock(&dev->rx_lock);

	wake_up(&dev->read_wait_queue);

resubmit_int_urb:
	
#ifdef HTC_PM_DBG
	if (usb_pm_debug_enabled)
		usb_mark_intf_last_busy(dev->intf, false);
#endif
	
	
	if (!dev->inturb->anchor) {
		usb_mark_last_busy(udev);
		usb_anchor_urb(dev->inturb, &dev->rx_submitted);
		status = usb_submit_urb(dev->inturb, GFP_ATOMIC);
		if (status) {
			usb_unanchor_urb(dev->inturb);
			dev_err(dev->devicep, "%s: Error re-submitting Int URB %d\n",
				__func__, status);
		}
	}
}

int rmnet_usb_ctrl_start_rx(struct rmnet_ctrl_dev *dev)
{
	int	retval = 0;

	usb_anchor_urb(dev->inturb, &dev->rx_submitted);
	retval = usb_submit_urb(dev->inturb, GFP_KERNEL);
	if (retval < 0) {
		usb_unanchor_urb(dev->inturb);
		dev_err(dev->devicep, "%s Intr submit %d\n", __func__,
				retval);
	}

	return retval;
}

int rmnet_usb_ctrl_suspend(struct rmnet_ctrl_dev *dev)
 {
	int ret = 0;
	ret = work_busy(&dev->get_encap_work);
	if (ret)
		return -EBUSY;

	usb_kill_anchored_urbs(&dev->rx_submitted);
	return 0;
}


static int rmnet_usb_ctrl_alloc_rx(struct rmnet_ctrl_dev *dev)
{
	int	retval = -ENOMEM;

	dev->rcvurb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->rcvurb) {
		pr_err("%s: Error allocating read urb\n", __func__);
		goto nomem;
	}

	dev->rcvbuf = kmalloc(DEFAULT_READ_URB_LENGTH, GFP_KERNEL);
	if (!dev->rcvbuf) {
		pr_err("%s: Error allocating read buffer\n", __func__);
		goto nomem;
	}

	dev->in_ctlreq = kmalloc(sizeof(*dev->in_ctlreq), GFP_KERNEL);
	if (!dev->in_ctlreq) {
		pr_err("%s: Error allocating setup packet buffer\n", __func__);
		goto nomem;
	}

#ifdef HTC_DEBUG_QMI_STUCK
	dev_err(dev->devicep, "%s init_timer rcv_timer\n", dev->name);
	init_timer(&dev->rcv_timer);
	dev->rcv_timer.function = rmnet_usb_ctrl_read_timer_expire;
	dev->rcv_timer.data = (unsigned long)dev;
#endif	

	return 0;

nomem:
	usb_free_urb(dev->rcvurb);
	kfree(dev->rcvbuf);
	kfree(dev->in_ctlreq);

	return retval;

}
static int rmnet_usb_ctrl_write_cmd(struct rmnet_ctrl_dev *dev)
{
	struct usb_device	*udev;

	if (!is_dev_connected(dev))
		return -ENODEV;

	udev = interface_to_usbdev(dev->intf);
	dev->set_ctrl_line_state_cnt++;
	return usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
		USB_CDC_REQ_SET_CONTROL_LINE_STATE,
		(USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE),
		dev->cbits_tomdm,
		dev->intf->cur_altsetting->desc.bInterfaceNumber,
		NULL, 0, USB_CTRL_SET_TIMEOUT);
}

#ifdef HTC_DEBUG_QMI_STUCK

static void rmnet_usb_ctrl_write_timer_expire(unsigned long data)
{
	struct urb *urb = (struct urb *)data;
	struct ctrl_write_context *context = urb->context;
	struct rmnet_ctrl_dev	*dev = context->dev;

	pr_err("[%s] urb %p expired\n", __func__, urb);

	
	#ifdef HTC_LOG_RMNET_USB_CTRL
	if (dev)
		log_rmnet_usb_ctrl_event(dev->intf, "Tx timeout", 0);
	#endif	
	

	
	schedule_delayed_work_on(0, &do_mdm_restart_delay_work, 5);
}
#endif	

static void ctrl_write_callback(struct urb *urb)
{
#ifdef HTC_DEBUG_QMI_STUCK
	struct ctrl_write_context *context = urb->context;
	struct rmnet_ctrl_dev	*dev = context->dev;
#else 
	struct rmnet_ctrl_dev	*dev = urb->context;
#endif 

#ifdef HTC_DEBUG_QMI_STUCK
	del_timer(&context->timer);

	if (unlikely(time_is_before_jiffies(context->start_jiffies + HZ)))
		pr_err("[%s] urb %p takes %d msec to complete.\n", __func__,
			urb, jiffies_to_msecs(jiffies - context->start_jiffies));
#endif	
	if (urb->status) {
		dev->tx_ctrl_err_cnt++;
		pr_debug_ratelimited("Write status/size %d/%d\n",
				urb->status, urb->actual_length);
	}

#ifdef HTC_LOG_RMNET_USB_CTRL
	log_rmnet_usb_ctrl_event(dev->intf, "Tx cb", urb->actual_length);
#endif	

	kfree(urb->setup_packet);
	kfree(urb->transfer_buffer);
	usb_free_urb(urb);
	usb_autopm_put_interface_async(dev->intf);
#ifdef HTC_DEBUG_QMI_STUCK
	kfree(context);
#endif	
}

static int rmnet_usb_ctrl_write(struct rmnet_ctrl_dev *dev, char *buf,
		size_t size)
{
	int			result;
	struct urb		*sndurb;
	struct usb_ctrlrequest	*out_ctlreq;
	struct usb_device	*udev;
#ifdef HTC_DEBUG_QMI_STUCK
	struct ctrl_write_context *context;
#endif	

	if (!is_dev_connected(dev))
		return -ENETRESET;

	udev = interface_to_usbdev(dev->intf);

	sndurb = usb_alloc_urb(0, GFP_KERNEL);
	if (!sndurb) {
		dev_err(dev->devicep, "Error allocating read urb\n");
		return -ENOMEM;
	}

	out_ctlreq = kmalloc(sizeof(*out_ctlreq), GFP_KERNEL);
	if (!out_ctlreq) {
		usb_free_urb(sndurb);
		dev_err(dev->devicep, "Error allocating setup packet buffer\n");
		return -ENOMEM;
	}

#ifdef HTC_DEBUG_QMI_STUCK
	context = kmalloc(sizeof(struct ctrl_write_context), GFP_KERNEL);
	if (!context) {
		kfree(out_ctlreq);
		usb_free_urb(sndurb);
		dev_err(dev->devicep, "Error allocating private data\n");
		return -ENOMEM;
	}
	context->dev = dev;
#endif	

	
	out_ctlreq->bRequestType = (USB_DIR_OUT | USB_TYPE_CLASS |
			     USB_RECIP_INTERFACE);
	out_ctlreq->bRequest = USB_CDC_SEND_ENCAPSULATED_COMMAND;
	out_ctlreq->wValue = 0;
	out_ctlreq->wIndex = dev->intf->cur_altsetting->desc.bInterfaceNumber;
	out_ctlreq->wLength = cpu_to_le16(size);

	usb_fill_control_urb(sndurb, udev,
			     usb_sndctrlpipe(udev, 0),
			     (unsigned char *)out_ctlreq, (void *)buf, size,
#ifdef HTC_DEBUG_QMI_STUCK
			     ctrl_write_callback, context);
#else	
			     ctrl_write_callback, dev);
#endif	

	result = usb_autopm_get_interface(dev->intf);
	if (result < 0) {
		dev_err(dev->devicep, "%s: Unable to resume interface: %d\n",
			__func__, result);


		usb_free_urb(sndurb);
		kfree(out_ctlreq);
#ifdef HTC_DEBUG_QMI_STUCK
		kfree(context);
#endif	
		return result;
	}

#ifdef HTC_LOG_RMNET_USB_CTRL
	log_rmnet_usb_ctrl_event(dev->intf, "Tx", size);
#endif	

#ifdef HTC_DEBUG_QMI_STUCK
	init_timer(&context->timer);
	context->timer.function = rmnet_usb_ctrl_write_timer_expire;
	context->timer.data = (unsigned long)sndurb;
	context->start_jiffies = jiffies;
	mod_timer(&context->timer, jiffies + msecs_to_jiffies(QMI_TX_NO_CB_TIMEOUT));
#endif	

	usb_anchor_urb(sndurb, &dev->tx_submitted);
	dev->snd_encap_cmd_cnt++;
	result = usb_submit_urb(sndurb, GFP_KERNEL);
	if (result < 0) {
		dev_err(dev->devicep, "%s: Submit URB error %d\n",
			__func__, result);
		dev->snd_encap_cmd_cnt--;
		usb_autopm_put_interface(dev->intf);
		usb_unanchor_urb(sndurb);
		usb_free_urb(sndurb);
		kfree(out_ctlreq);
#ifdef HTC_DEBUG_QMI_STUCK
		del_timer(&context->timer);
		kfree(context);
#endif	
		return result;
	}

	return size;
}

static int rmnet_ctl_open(struct inode *inode, struct file *file)
{
	int			retval = 0;
	struct rmnet_ctrl_dev	*dev =
		container_of(inode->i_cdev, struct rmnet_ctrl_dev, cdev);

	if (!dev)
		return -ENODEV;

	if (dev->is_opened) {
		file->private_data = dev;
		goto already_opened;
	}

	
	if (dev->is_connected) {
		if (dev->intf)
			dev_info(&dev->intf->dev, "%s[%d]:mdm_wait_timeout:%d resp_available:%d\n", __func__, __LINE__, dev->mdm_wait_timeout, dev->resp_available);
	}

	
	if (dev->mdm_wait_timeout && !dev->resp_available) {
		retval = wait_event_interruptible_timeout(
					dev->open_wait_queue,
					dev->resp_available,
					msecs_to_jiffies(dev->mdm_wait_timeout *
									1000));
		if (retval == 0) {
			dev_err(dev->devicep, "%s: Timeout opening %s\n",
						__func__, dev->name);
			return -ETIMEDOUT;
		} else if (retval < 0) {
			dev_err(dev->devicep, "%s: Error waiting for %s\n",
						__func__, dev->name);
			return retval;
		}
	}

	if (!dev->resp_available) {
		dev_err(dev->devicep, "%s: Connection timedout opening %s\n",
					__func__, dev->name);

#ifdef HTC_MDM_RESTART_IF_RMNET_OPEN_TIMEOUT
		if (dev->is_connected) {
			extern void htc_ehci_trigger_mdm_restart(void);

			if (jiffies_to_msecs(jiffies - dev->connected_jiffies) > RMNET_OPEN_TIMEOUT_MS) {
				dev_err(&dev->intf->dev, "%s[%d]:dev->connected_jiffies:%lu jiffies:%lu\n", __func__, __LINE__, dev->connected_jiffies, jiffies);
				dev_err(&dev->intf->dev, "%s[%d]:htc_ehci_trigger_mdm_restart!!!\n", __func__, __LINE__);
				htc_ehci_trigger_mdm_restart();
				msleep(500);
			}
		}
#endif	

		return -ETIMEDOUT;
	}

	mutex_lock(&dev->dev_lock);
	dev->is_opened = 1;
	mutex_unlock(&dev->dev_lock);

	file->private_data = dev;

already_opened:
	DBG("%s: Open called for %s\n", __func__, dev->name);

	return 0;
}

static int rmnet_ctl_release(struct inode *inode, struct file *file)
{
	struct ctrl_pkt_list_elem	*list_elem = NULL;
	struct rmnet_ctrl_dev		*dev;
	unsigned long			flag;

	dev = file->private_data;
	if (!dev)
		return -ENODEV;

	DBG("%s Called on %s device\n", __func__, dev->name);

	spin_lock_irqsave(&dev->rx_lock, flag);
	while (!list_empty(&dev->rx_list)) {
		list_elem = list_first_entry(
				&dev->rx_list,
				struct ctrl_pkt_list_elem,
				list);
		list_del(&list_elem->list);
		kfree(list_elem->cpkt.data);
		kfree(list_elem);
	}
	spin_unlock_irqrestore(&dev->rx_lock, flag);

	mutex_lock(&dev->dev_lock);
	dev->is_opened = 0;
	mutex_unlock(&dev->dev_lock);

	if (is_dev_connected(dev))
		usb_kill_anchored_urbs(&dev->tx_submitted);

	file->private_data = NULL;

	return 0;
}

static unsigned int rmnet_ctl_poll(struct file *file, poll_table *wait)
{
	unsigned int		mask = 0;
	struct rmnet_ctrl_dev	*dev;

	dev = file->private_data;
	if (!dev)
		return POLLERR;

	poll_wait(file, &dev->read_wait_queue, wait);
	if (!is_dev_connected(dev)) {
		dev_dbg(dev->devicep, "%s: Device not connected\n",
			__func__);
		return POLLERR;
	}

	if (!list_empty(&dev->rx_list))
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static ssize_t rmnet_ctl_read(struct file *file, char __user *buf, size_t count,
		loff_t *ppos)
{
	int				retval = 0;
	int				bytes_to_read;
	struct rmnet_ctrl_dev		*dev;
	struct ctrl_pkt_list_elem	*list_elem = NULL;
	unsigned long			flags;

	dev = file->private_data;
	if (!dev)
		return -ENODEV;

	DBG("%s: Read from %s\n", __func__, dev->name);

ctrl_read:
	if (!is_dev_connected(dev)) {
		dev_dbg(dev->devicep, "%s: Device not connected\n",
			__func__);
		return -ENETRESET;
	}
	spin_lock_irqsave(&dev->rx_lock, flags);
	if (list_empty(&dev->rx_list)) {
		spin_unlock_irqrestore(&dev->rx_lock, flags);

		retval = wait_event_interruptible(dev->read_wait_queue,
					!list_empty(&dev->rx_list) ||
					!is_dev_connected(dev));
		if (retval < 0)
			return retval;

		goto ctrl_read;
	}

	list_elem = list_first_entry(&dev->rx_list,
				     struct ctrl_pkt_list_elem, list);
	bytes_to_read = (uint32_t)(list_elem->cpkt.data_size);
	if (bytes_to_read > count) {
		spin_unlock_irqrestore(&dev->rx_lock, flags);
		dev_err(dev->devicep, "%s: Packet size %d > buf size %d\n",
			__func__, bytes_to_read, count);
		return -ENOMEM;
	}
	spin_unlock_irqrestore(&dev->rx_lock, flags);

	if (copy_to_user(buf, list_elem->cpkt.data, bytes_to_read)) {
			dev_err(dev->devicep,
				"%s: copy_to_user failed for %s\n",
				__func__, dev->name);
		return -EFAULT;
	}
	spin_lock_irqsave(&dev->rx_lock, flags);
	list_del(&list_elem->list);
	spin_unlock_irqrestore(&dev->rx_lock, flags);

	kfree(list_elem->cpkt.data);
	kfree(list_elem);
	DBG("%s: Returning %d bytes to %s\n", __func__, bytes_to_read,
			dev->name);
	DUMP_BUFFER("Read: ", bytes_to_read, buf);

#ifdef HTC_LOG_RMNET_USB_CTRL
	log_rmnet_usb_ctrl_event(dev->intf, "Read", bytes_to_read);
#endif	

	return bytes_to_read;
}

static ssize_t rmnet_ctl_write(struct file *file, const char __user * buf,
		size_t size, loff_t *pos)
{
	int			status;
	void			*wbuf;
	struct rmnet_ctrl_dev	*dev = file->private_data;

	if (!dev)
		return -ENODEV;

	if (size <= 0)
		return -EINVAL;

	if (!is_dev_connected(dev))
		return -ENETRESET;

	DBG("%s: Writing %i bytes on %s\n", __func__, size, dev->name);

	wbuf = kmalloc(size , GFP_KERNEL);
	if (!wbuf) {
		dev_err(dev->devicep,
			"%s(%d): kmalloc failed !!!\n",
			__func__, __LINE__);
		return -ENOMEM;
	}

	status = copy_from_user(wbuf , buf, size);
	if (status) {
		dev_err(dev->devicep,
		"%s: Unable to copy data from userspace %d\n",
		__func__, status);
		kfree(wbuf);
		return status;
	}
	DUMP_BUFFER("Write: ", size, buf);

	status = rmnet_usb_ctrl_write(dev, wbuf, size);
	if (status == size)
		return size;

	return status;
}

static int rmnet_ctrl_tiocmset(struct rmnet_ctrl_dev *dev, unsigned int set,
		unsigned int clear)
{
	int retval;

	mutex_lock(&dev->dev_lock);
	if (set & TIOCM_DTR)
		dev->cbits_tomdm |= ACM_CTRL_DTR;


	if (clear & TIOCM_DTR)
		dev->cbits_tomdm &= ~ACM_CTRL_DTR;


	mutex_unlock(&dev->dev_lock);

	retval = usb_autopm_get_interface(dev->intf);
	if (retval < 0) {
		dev_err(dev->devicep, "%s: Unable to resume interface: %d\n",
			__func__, retval);
		return retval;
	}

	retval = rmnet_usb_ctrl_write_cmd(dev);

	usb_autopm_put_interface(dev->intf);
	return retval;
}

static int rmnet_ctrl_tiocmget(struct rmnet_ctrl_dev *dev)
{
	int	ret;

	mutex_lock(&dev->dev_lock);
	ret =
	(dev->cbits_tolocal & ACM_CTRL_CD ? TIOCM_CD : 0) |
	(dev->cbits_tomdm & ACM_CTRL_DTR ? TIOCM_DTR : 0);
	mutex_unlock(&dev->dev_lock);

	return ret;
}

static long rmnet_ctrl_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	int			ret;
	struct rmnet_ctrl_dev	*dev;

	dev = file->private_data;
	if (!dev)
		return -ENODEV;

	switch (cmd) {
	case TIOCMGET:

		ret = rmnet_ctrl_tiocmget(dev);
		break;
	case TIOCMSET:
		ret = rmnet_ctrl_tiocmset(dev, arg, ~arg);
		break;
	case HTC_RMNET_USB_SET_ITC:
		{
			ret = 0;
		}
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static const struct file_operations ctrldev_fops = {
	.owner = THIS_MODULE,
	.read  = rmnet_ctl_read,
	.write = rmnet_ctl_write,
	.unlocked_ioctl = rmnet_ctrl_ioctl,
	.open  = rmnet_ctl_open,
	.release = rmnet_ctl_release,
	.poll = rmnet_ctl_poll,
};

#define HSIC_FAST_INTERRUPT_LATENCY 1
#define HSIC_SLOW_INTERRUPT_LATENCY 6

static int enable_shorten_itc_count = 0;
static int rmnet_ctrl_set_itc( struct rmnet_ctrl_dev *dev, int value ) {
	int ret = 0;
	int enable = 0;
	struct usb_device	*udev;

	if (!dev) {
		pr_info("[%s] dev is null\n", __func__);
		return -ENODEV;
	}

	if (!dev->intf) {
		pr_info("[%s] intf is null\n", __func__);
		return -ENODEV;
	}

	udev = interface_to_usbdev(dev->intf);

	switch(value) {
		case 1://enable
			enable = 1;
			break;
		case 0://disable
			enable = 0;
			break;
		default://other
			pr_info("[%s][%s] value=[%d]\n", __func__, dev->name, value);
			return -ENODEV;
	}
	

	pr_info("[%s][%s] mutex_lock\n", __func__, dev->name);
	mutex_lock(&dev->dev_lock);

	if ( ! ( dev->is_opened && dev->resp_available ) ) {
		pr_err("[%s] is_opened=[%d], resp_available=[%d]\n", __func__, dev->is_opened, dev->resp_available);
		mutex_unlock(&dev->dev_lock);
		return -ENODEV;
	}
	pr_info("[%s] is_opened=[%d], resp_available=[%d]\n", __func__, dev->is_opened, dev->resp_available);
	pr_info("[%s] enable_shorten_itc_count:%d enable:%d\n", __func__, enable_shorten_itc_count, enable);
	if ( enable ) {
		if (enable_shorten_itc_count == 0) {
			
			ret = usb_autopm_get_interface(dev->intf);
			if (ret < 0) {
				pr_info("[%s] Unable to resume interface: %d\n", __func__, ret);
				mutex_unlock(&dev->dev_lock);
				return -ENODEV;
			}

			pr_info("[%s][%s] usb_set_interrupt_latency(1)+\n", __func__, dev->name);
			ret = usb_set_interrupt_latency(udev, HSIC_FAST_INTERRUPT_LATENCY);
			pr_info("[%s][%s] usb_set_interrupt_latency-\n", __func__, dev->name);
			if ( dev->intf )
				usb_autopm_put_interface(dev->intf);
			else
				pr_err("[%s]dev->intf=NULL\n", __func__);
		}
		enable_shorten_itc_count++;
	} else {
		if ( enable_shorten_itc_count < 1 ) {
			
			pr_info("[%s][%s] set_shorten_interrupt_latency_count is %d\n", __func__, dev->name, enable_shorten_itc_count);
		} else {
			enable_shorten_itc_count--;
			if (enable_shorten_itc_count == 0) {
				ret = usb_autopm_get_interface(dev->intf);
				if (ret < 0) {
					pr_info("[%s] Unable to resume interface: %d\n", __func__, ret);
					mutex_unlock(&dev->dev_lock);
					return -ENODEV;
				}
				pr_info("[%s][%s] usb_set_interrupt_latency(6)+\n", __func__, dev->name);
				ret = usb_set_interrupt_latency(udev, HSIC_SLOW_INTERRUPT_LATENCY);
				pr_info("[%s][%s] usb_set_interrupt_latency-\n", __func__, dev->name);
				if ( dev->intf )
					usb_autopm_put_interface(dev->intf);
				else
					pr_err("[%s]dev->intf=NULL\n", __func__);
			}
		}
	}
	mutex_unlock(&dev->dev_lock);
	pr_info("[%s][%s] mutex_unlock\n", __func__, dev->name);

	return ret;
}

static ssize_t rmnet_hsic_ctl_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	return -ENODEV;
}

static ssize_t rmnet_hsic_ctl_write(struct file *file, const char __user * buf, size_t size, loff_t *pos)
{
	return -ENODEV;
}

static long rmnet_hsic_ctrl_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int			ret;
	struct rmnet_ctrl_dev	*dev;

	dev = file->private_data;
	if (!dev)
		return -ENODEV;

	switch (cmd) {
	case HTC_RMNET_USB_SET_ITC:
		{
			int value = 0;
			get_user(value, (unsigned long __user *)arg);
			ret = rmnet_ctrl_set_itc ( dev, value );
		}
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int rmnet_hsic_ctl_open(struct inode *inode, struct file *file)
{
	int			retval = 0;
	struct rmnet_ctrl_dev *dev = NULL;
	dev = ctrl_dev[0];

	if (!dev || !file)
		return -ENODEV;

	if (dev->is_connected) {
		if (dev->intf)
			dev_info(&dev->intf->dev, "%s[%d]:mdm_wait_timeout:%d resp_available:%d\n", __func__, __LINE__, dev->mdm_wait_timeout, dev->resp_available);
	}

	
	if (dev->mdm_wait_timeout && !dev->resp_available) {
		retval = wait_event_interruptible_timeout(
					dev->open_wait_queue,
					dev->resp_available,
					msecs_to_jiffies(dev->mdm_wait_timeout *
									1000));
		if (retval == 0) {
			dev_err(dev->devicep, "%s: Timeout opening %s\n",
						__func__, dev->name);
			return -ETIMEDOUT;
		} else if (retval < 0) {
			dev_err(dev->devicep, "%s: Error waiting for %s\n",
						__func__, dev->name);
			return retval;
		}
	}

	if (!dev->resp_available) {
		dev_err(dev->devicep, "%s: Connection timedout opening %s\n",
					__func__, dev->name);

		return -ETIMEDOUT;
	}

	file->private_data = dev;

	return 0;
}

static int rmnet_hsic_ctl_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static unsigned int rmnet_hsic_ctl_poll(struct file *file, poll_table *wait)
{
	return POLLERR;
}

static const struct file_operations hsicctrldev_fops = {
	.owner = THIS_MODULE,
	.read  = rmnet_hsic_ctl_read,
	.write = rmnet_hsic_ctl_write,
	.unlocked_ioctl = rmnet_hsic_ctrl_ioctl,
	.open  = rmnet_hsic_ctl_open,
	.release = rmnet_hsic_ctl_release,
	.poll = rmnet_hsic_ctl_poll,
};

int rmnet_usb_ctrl_probe(struct usb_interface *intf,
		struct usb_host_endpoint *int_in, struct rmnet_ctrl_dev *dev)
{
	u16				wMaxPacketSize;
	struct usb_endpoint_descriptor	*ep;
	struct usb_device		*udev;
	int				interval;
	int				ret = 0;

	udev = interface_to_usbdev(intf);

	if (!dev) {
		pr_err("%s: Ctrl device not found\n", __func__);
		return -ENODEV;
	}
	dev->int_pipe = usb_rcvintpipe(udev,
		int_in->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);

	mutex_lock(&dev->dev_lock);
	dev->intf = intf;

	
	dev->cbits_tolocal = ACM_CTRL_CD;

	
	dev->cbits_tomdm = ACM_CTRL_DTR;
	mutex_unlock(&dev->dev_lock);

	dev->resp_available = false;
	dev->snd_encap_cmd_cnt = 0;
	dev->get_encap_resp_cnt = 0;
	dev->resp_avail_cnt = 0;
	dev->tx_ctrl_err_cnt = 0;
	dev->set_ctrl_line_state_cnt = 0;

	ret = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
			USB_CDC_REQ_SET_CONTROL_LINE_STATE,
			(USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE),
			dev->cbits_tomdm,
			dev->intf->cur_altsetting->desc.bInterfaceNumber,
			NULL, 0, USB_CTRL_SET_TIMEOUT);
	if (ret < 0)
		return ret;

	dev->set_ctrl_line_state_cnt++;

	dev->inturb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->inturb) {
		dev_err(dev->devicep, "Error allocating int urb\n");
		return -ENOMEM;
	}

	
	ep = &dev->intf->cur_altsetting->endpoint[0].desc;
	wMaxPacketSize = le16_to_cpu(ep->wMaxPacketSize);

	dev->intbuf = kmalloc(wMaxPacketSize, GFP_KERNEL);
	if (!dev->intbuf) {
		usb_free_urb(dev->inturb);
		dev_err(dev->devicep, "Error allocating int buffer\n");
		return -ENOMEM;
	}

	dev->in_ctlreq->bRequestType =
		(USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE);
	dev->in_ctlreq->bRequest  = USB_CDC_GET_ENCAPSULATED_RESPONSE;
	dev->in_ctlreq->wValue = 0;
	dev->in_ctlreq->wIndex =
		dev->intf->cur_altsetting->desc.bInterfaceNumber;
	dev->in_ctlreq->wLength = cpu_to_le16(DEFAULT_READ_URB_LENGTH);

	interval = max((int)int_in->desc.bInterval,
			(udev->speed == USB_SPEED_HIGH) ? HS_INTERVAL
							: FS_LS_INTERVAL);

	usb_fill_int_urb(dev->inturb, udev,
			 dev->int_pipe,
			 dev->intbuf, wMaxPacketSize,
			 notification_available_cb, dev, interval);

	
#ifdef HTC_PM_DBG
	if (usb_pm_debug_enabled)
		usb_mark_intf_last_busy(dev->intf, false);
#endif
	
	usb_mark_last_busy(udev);

	
	dev_info(&intf->dev, "%s[%d]:rmnet_usb_ctrl_start_rx dev->resp_available:%d\n", __func__, __LINE__, dev->resp_available);

	ret = rmnet_usb_ctrl_start_rx(dev);

#ifdef HTC_MDM_RESTART_IF_RMNET_OPEN_TIMEOUT
	if (!ret)
		dev->connected_jiffies = jiffies;
#endif	

	if (!ret)
		dev->is_connected = true;

	return ret;
}

void rmnet_usb_ctrl_disconnect(struct rmnet_ctrl_dev *dev)
{

	mutex_lock(&dev->dev_lock);

	
	dev->cbits_tolocal = ~ACM_CTRL_CD;

	dev->cbits_tomdm = ~ACM_CTRL_DTR;
	dev->is_connected = false;
	mutex_unlock(&dev->dev_lock);

	wake_up(&dev->read_wait_queue);

	cancel_work_sync(&dev->get_encap_work);

	usb_kill_anchored_urbs(&dev->tx_submitted);
	usb_kill_anchored_urbs(&dev->rx_submitted);

	usb_free_urb(dev->inturb);
	dev->inturb = NULL;

	kfree(dev->intbuf);
	dev->intbuf = NULL;

	
	dev_info(&dev->intf->dev, "%s[%d]:dev->resp_available:%d\n", __func__, __LINE__, dev->resp_available);
}

#if defined(CONFIG_DEBUG_FS)
#define DEBUG_BUF_SIZE	4096
static ssize_t rmnet_usb_ctrl_read_stats(struct file *file, char __user *ubuf,
		size_t count, loff_t *ppos)
{
	struct rmnet_ctrl_dev	*dev;
	char			*buf;
	int			ret;
	int			i;
	int			temp = 0;

	buf = kzalloc(sizeof(char) * DEBUG_BUF_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	for (i = 0; i < NUM_CTRL_CHANNELS; i++) {
		dev = ctrl_dev[i];
		if (!dev)
			continue;

		temp += scnprintf(buf + temp, DEBUG_BUF_SIZE - temp,
				"\n#ctrl_dev: %p     Name: %s#\n"
				"snd encap cmd cnt         %u\n"
				"resp avail cnt:           %u\n"
				"get encap resp cnt:       %u\n"
				"set ctrl line state cnt:  %u\n"
				"tx_err_cnt:               %u\n"
				"cbits_tolocal:            %d\n"
				"cbits_tomdm:              %d\n"
				"mdm_wait_timeout:         %u\n"
				"zlp_cnt:                  %u\n"
				"get_encap_failure_cnt     %u\n"
				"dev opened:               %s\n",
				dev, dev->name,
				dev->snd_encap_cmd_cnt,
				dev->resp_avail_cnt,
				dev->get_encap_resp_cnt,
				dev->set_ctrl_line_state_cnt,
				dev->tx_ctrl_err_cnt,
				dev->cbits_tolocal,
				dev->cbits_tomdm,
				dev->mdm_wait_timeout,
				dev->zlp_cnt,
				dev->get_encap_failure_cnt,
				dev->is_opened ? "OPEN" : "CLOSE");

	}

	ret = simple_read_from_buffer(ubuf, count, ppos, buf, temp);

	kfree(buf);

	return ret;
}

static ssize_t rmnet_usb_ctrl_reset_stats(struct file *file, const char __user *
		buf, size_t count, loff_t *ppos)
{
	struct rmnet_ctrl_dev	*dev;
	int			i;

	for (i = 0; i < NUM_CTRL_CHANNELS; i++) {
		dev = ctrl_dev[i];
		if (!dev)
			continue;

		dev->snd_encap_cmd_cnt = 0;
		dev->resp_avail_cnt = 0;
		dev->get_encap_resp_cnt = 0;
		dev->set_ctrl_line_state_cnt = 0;
		dev->tx_ctrl_err_cnt = 0;
		dev->zlp_cnt = 0;
	}
	return count;
}

const struct file_operations rmnet_usb_ctrl_stats_ops = {
	.read = rmnet_usb_ctrl_read_stats,
	.write = rmnet_usb_ctrl_reset_stats,
};

struct dentry	*usb_ctrl_dent;
struct dentry	*usb_ctrl_dfile;
static void rmnet_usb_ctrl_debugfs_init(void)
{
	usb_ctrl_dent = debugfs_create_dir("rmnet_usb_ctrl", 0);
	if (IS_ERR(usb_ctrl_dent))
		return;

	usb_ctrl_dfile = debugfs_create_file("status", 0644, usb_ctrl_dent, 0,
			&rmnet_usb_ctrl_stats_ops);
	if (!usb_ctrl_dfile || IS_ERR(usb_ctrl_dfile))
		debugfs_remove(usb_ctrl_dent);
}

static void rmnet_usb_ctrl_debugfs_exit(void)
{
	debugfs_remove(usb_ctrl_dfile);
	debugfs_remove(usb_ctrl_dent);
}

#else
static void rmnet_usb_ctrl_debugfs_init(void) { }
static void rmnet_usb_ctrl_debugfs_exit(void) { }
#endif

int rmnet_usb_ctrl_init(void)
{
	struct rmnet_ctrl_dev	*dev;
	int			n;
	int			status;
	int			ret = 0;

	
	if (get_radio_flag() & 0x0001)
		usb_pm_debug_enabled  = true;

	if (get_radio_flag() & 0x0002)
		enable_ctl_msg_debug = true;
	

#ifdef HTC_LOG_RMNET_USB_CTRL
	if (get_radio_flag() & 0x0008)
		enable_dbg_rmnet_usb_ctrl = true;
#endif	

	for (n = 0; n < NUM_CTRL_CHANNELS; ++n) {

		dev = kzalloc(sizeof(*dev), GFP_KERNEL);
		if (!dev) {
			status = -ENOMEM;
			goto error0;
		}
		
		snprintf(dev->name, CTRL_DEV_MAX_LEN, "hsicctl%d", n);

		dev->wq = create_singlethread_workqueue(dev->name);
		if (!dev->wq) {
			pr_err("unable to allocate workqueue");
			kfree(dev);
			goto error0;
		}

		mutex_init(&dev->dev_lock);
		spin_lock_init(&dev->rx_lock);
		init_waitqueue_head(&dev->read_wait_queue);
		init_waitqueue_head(&dev->open_wait_queue);
		INIT_LIST_HEAD(&dev->rx_list);
		init_usb_anchor(&dev->tx_submitted);
		init_usb_anchor(&dev->rx_submitted);
		INIT_WORK(&dev->get_encap_work, get_encap_work);

		status = rmnet_usb_ctrl_alloc_rx(dev);
		if (status < 0) {
			kfree(dev);
			goto error0;
		}

		ctrl_dev[n] = dev;
	}

	status = alloc_chrdev_region(&ctrldev_num, 0, NUM_CTRL_CHANNELS,
			DEVICE_NAME);
	if (IS_ERR_VALUE(status)) {
		pr_err("ERROR:%s: alloc_chrdev_region() ret %i.\n",
		       __func__, status);
		goto error0;
	}

	ctrldev_classp = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(ctrldev_classp)) {
		pr_err("ERROR:%s: class_create() ENOMEM\n", __func__);
		status = -ENOMEM;
		goto error1;
	}
	for (n = 0; n < NUM_CTRL_CHANNELS; ++n) {
		cdev_init(&ctrl_dev[n]->cdev, &ctrldev_fops);
		ctrl_dev[n]->cdev.owner = THIS_MODULE;

		status = cdev_add(&ctrl_dev[n]->cdev, (ctrldev_num + n), 1);

		if (IS_ERR_VALUE(status)) {
			pr_err("%s: cdev_add() ret %i\n", __func__, status);
			kfree(ctrl_dev[n]);
			goto error2;
		}

		ctrl_dev[n]->devicep =
				device_create(ctrldev_classp, NULL,
				(ctrldev_num + n), NULL,
				DEVICE_NAME "%d", n);

		if (IS_ERR(ctrl_dev[n]->devicep)) {
			pr_err("%s: device_create() ENOMEM\n", __func__);
			status = -ENOMEM;
			cdev_del(&ctrl_dev[n]->cdev);
			kfree(ctrl_dev[n]);
			goto error2;
		}
		
		status = device_create_file(ctrl_dev[n]->devicep,
					&dev_attr_modem_wait);
		if (status) {
			device_destroy(ctrldev_classp,
				MKDEV(MAJOR(ctrldev_num), n));
			cdev_del(&ctrl_dev[n]->cdev);
			kfree(ctrl_dev[n]);
			goto error2;
		}

		dev_set_drvdata(ctrl_dev[n]->devicep, ctrl_dev[n]);
	}

	rmnet_usb_ctrl_debugfs_init();
	do {
		pr_info("%s: register_chrdev\n", __func__);
		ret = register_chrdev(0, HTC_HSIC_DEVICE_NAME, &hsicctrldev_fops);
		if ( ret < 0 ) {
			pr_err("%s: register_chrdev, ret=[%d]\n", __func__, ret);
			break;
		}
		pr_info("%s: register_chrdev, ret=[%d]\n", __func__, ret);
		htc_hsicctrldev_major = ret;
		pr_info("%s: class_create\n", __func__);
		hsicctrldev_classp = class_create(THIS_MODULE, HTC_HSIC_DEVICE_NAME);
		if (IS_ERR(hsicctrldev_classp)) {
			pr_err("%s: class_create() ENOMEM\n", __func__);
			unregister_chrdev( htc_hsicctrldev_major, HTC_HSIC_DEVICE_NAME );
			break;
		}
		pr_info("%s: device_create\n", __func__);
		hsicdevicep = device_create(hsicctrldev_classp, NULL, MKDEV(htc_hsicctrldev_major, 0), NULL, HTC_HSIC_DEVICE_NAME);
		if (IS_ERR(hsicdevicep)) {
			pr_err("%s: device_create() ENOMEM\n", __func__);
			class_destroy(hsicctrldev_classp);
			hsicctrldev_classp = NULL;
			hsicdevicep = NULL;
			break;
		}
	}
	while ( 0 );

	pr_info("rmnet usb ctrl Initialized.\n");
	return 0;
error2:
		while (--n >= 0) {
			cdev_del(&ctrl_dev[n]->cdev);
			device_destroy(ctrldev_classp,
				MKDEV(MAJOR(ctrldev_num), n));
		}

		class_destroy(ctrldev_classp);
		n = NUM_CTRL_CHANNELS;
error1:
	unregister_chrdev_region(MAJOR(ctrldev_num), NUM_CTRL_CHANNELS);
error0:
	while (--n >= 0)
		kfree(ctrl_dev[n]);

	return status;
}

void rmnet_usb_ctrl_exit(void)
{
	int	i;

	for (i = 0; i < NUM_CTRL_CHANNELS; ++i) {
		if (!ctrl_dev[i])
			return;

		kfree(ctrl_dev[i]->in_ctlreq);
		kfree(ctrl_dev[i]->rcvbuf);
		kfree(ctrl_dev[i]->intbuf);
		usb_free_urb(ctrl_dev[i]->rcvurb);
		usb_free_urb(ctrl_dev[i]->inturb);
#if defined(DEBUG)
		device_remove_file(ctrl_dev[i]->devicep, &dev_attr_modem_wait);
#endif
		cdev_del(&ctrl_dev[i]->cdev);
		destroy_workqueue(ctrl_dev[i]->wq);
		kfree(ctrl_dev[i]);
		ctrl_dev[i] = NULL;
		device_destroy(ctrldev_classp, MKDEV(MAJOR(ctrldev_num), i));
	}

	class_destroy(ctrldev_classp);
	unregister_chrdev_region(MAJOR(ctrldev_num), NUM_CTRL_CHANNELS);

	if ( hsicctrldev_classp != NULL ) {
		pr_info("%s: device_destroy\n", __func__);
		device_destroy(hsicctrldev_classp, MKDEV(htc_hsicctrldev_major, 0));
		pr_info("%s: class_unregister\n", __func__);
		class_unregister(hsicctrldev_classp);
		pr_info("%s: class_destroy\n", __func__);
		class_destroy(hsicctrldev_classp);
		pr_info("%s: unregister_chrdev\n", __func__);
		unregister_chrdev(htc_hsicctrldev_major, HTC_HSIC_DEVICE_NAME);
		hsicctrldev_classp = NULL;
		hsicdevicep = NULL;
		enable_shorten_itc_count = 0;
	}

	rmnet_usb_ctrl_debugfs_exit();
}
