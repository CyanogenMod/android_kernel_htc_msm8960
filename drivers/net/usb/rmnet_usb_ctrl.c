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
#include "rmnet_usb.h"
#include <mach/board_htc.h>


static char *rmnet_dev_names[MAX_RMNET_DEVS] = {"hsicctl"};
module_param_array(rmnet_dev_names, charp, NULL, S_IRUGO | S_IWUSR);
#define HTC_HSIC_DEVICE_NAME		"htc_hsicctl"

#define DEFAULT_READ_URB_LENGTH		0x1000
#define UNLINK_TIMEOUT_MS		500 

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
	struct ctrl_pkt *cpkt;
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
					16, 1, buf, 11 , false); \
} while (0)

#define DBG(x...) \
		do { \
			if (ctl_msg_dbg_mask & MSM_USB_CTL_DEBUG) \
				pr_info(x); \
		} while (0)

static int num_devs;
static int insts_per_dev;
struct device			*hsicdevicep;
struct class			*hsicctrldev_classp;
static int				htc_hsicctrldev_major;

static struct rmnet_ctrl_dev **ctrl_devs;
static struct class	*ctrldev_classp[MAX_RMNET_DEVS];
static dev_t		ctrldev_num[MAX_RMNET_DEVS];

struct ctrl_pkt {
	size_t	data_size;
	void	*data;
	void	*ctxt;
};

struct ctrl_pkt_list_elem {
	struct list_head	list;
	struct ctrl_pkt		cpkt;
};

static void resp_avail_cb(struct urb *);

static int rmnet_usb_ctrl_dmux(struct ctrl_pkt_list_elem *clist)
{
	struct mux_hdr	*hdr;
	size_t		pad_len;
	size_t		total_len;
	unsigned int	mux_id;

	hdr = (struct mux_hdr *)clist->cpkt.data;
	pad_len = hdr->padding_info >> MUX_PAD_SHIFT;
	if (pad_len > MAX_PAD_BYTES(4)) {
		pr_err_ratelimited("%s: Invalid pad len %d\n", __func__,
				pad_len);
		return -EINVAL;
	}

	mux_id = hdr->mux_id;

	if (!mux_id || mux_id > insts_per_dev) {
		pr_err_ratelimited("%s: Invalid mux id %d\n", __func__, mux_id);
		return -EINVAL;
	}
	total_len = le16_to_cpu(hdr->pkt_len_w_padding);
	if (!total_len || !(total_len - pad_len)) {
		pr_err_ratelimited("%s: Invalid pkt length %d\n", __func__,
				total_len);
		return -EINVAL;
	}

	clist->cpkt.data_size = total_len - pad_len;

	return mux_id - 1;
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

	
	if ( !dev ) {
		expired_rcvurb = NULL;
		pr_err("[RMNET][%s] dev is NULL\n", __func__);
		return;
	}
	

	dev_err(dev->devicep, "[RMNET][%s] %s rcv_timer expire !!! \n", __func__, dev->name);

	
	#ifdef HTC_LOG_RMNET_USB_CTRL
	if (dev)
		log_rmnet_usb_ctrl_event(dev->intf, "Rx timeout", 0);
	#endif	
	
	if( NULL == expired_rcvurb ) {
		pr_info("[RMNET][%s] Read urb %p expire\n", __func__, expired_rcvurb);
		expired_rcvurb = dev->rcvurb;
		schedule_delayed_work_on(0, &kill_rcv_urb_delay_work, 10);
	}
}
#endif	

static void rmnet_usb_ctrl_mux(unsigned int id, struct ctrl_pkt *cpkt)
{
	struct mux_hdr	*hdr;
	size_t		len;
	size_t		pad_len = 0;

	hdr = (struct mux_hdr *)cpkt->data;
	hdr->mux_id = id + 1;
	len = cpkt->data_size - sizeof(struct mux_hdr) - MAX_PAD_BYTES(4);

	
	pad_len =  ALIGN(len, 4) - len;

	hdr->pkt_len_w_padding = cpu_to_le16(len + pad_len);
	hdr->padding_info = (pad_len << MUX_PAD_SHIFT) | MUX_CTRL_MASK;

	cpkt->data_size = sizeof(struct mux_hdr) + hdr->pkt_len_w_padding;
}

static void get_encap_work(struct work_struct *w)
{
	struct usb_device	*udev;
	struct rmnet_ctrl_dev	*dev =
			container_of(w, struct rmnet_ctrl_dev, get_encap_work);
	int			status;

	if (!test_bit(RMNET_CTRL_DEV_READY, &dev->status))
		return;

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
		if (status != -ENODEV)
			dev_err(dev->devicep,
			"%s: Error submitting Read URB %d\n",
			__func__, status);
		goto resubmit_int_urb;
	}

	return;

resubmit_int_urb:
	
	if (!dev->inturb->anchor) {
		usb_anchor_urb(dev->inturb, &dev->rx_submitted);
		status = usb_submit_urb(dev->inturb, GFP_KERNEL);
		if (status) {
			usb_unanchor_urb(dev->inturb);
			if (status != -ENODEV)
				dev_err(dev->devicep,
				"%s: Error re-submitting Int URB %d\n",
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
		
		if (!test_bit(RMNET_CTRL_DEV_READY, &dev->status)) {
			set_bit(RMNET_CTRL_DEV_READY, &dev->status);
			wake_up(&dev->open_wait_queue);
		}

		usb_mark_last_busy(udev);
		queue_work(dev->wq, &dev->get_encap_work);

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
	
	usb_anchor_urb(urb, &dev->rx_submitted);
	status = usb_submit_urb(urb, GFP_ATOMIC);
	if (status) {
		usb_unanchor_urb(urb);
		if (status != -ENODEV)
			dev_err(dev->devicep,
			"%s: Error re-submitting Int URB %d\n",
			__func__, status);
	}

	return;
}

static void resp_avail_cb(struct urb *urb)
{
	struct usb_device		*udev;
	struct ctrl_pkt_list_elem	*list_elem = NULL;
	struct rmnet_ctrl_dev		*rx_dev, *dev = urb->context;
	void				*cpkt;
	int				ch_id, status = 0;
	size_t				cpkt_size = 0;

	udev = interface_to_usbdev(dev->intf);

	usb_autopm_put_interface_async(dev->intf);
#ifdef HTC_DEBUG_QMI_STUCK
	del_timer(&dev->rcv_timer);
	if (expired_rcvurb == urb) {
		expired_rcvurb = NULL;
		dev_err(dev->devicep, "[RMNET] %s(%d) urb->status:%d urb->actual_length:%u !!!\n", __func__, __LINE__, urb->status, urb->actual_length);
		if (urb->status != 0) {
			if (urb->actual_length > 0) {
				dev_err(dev->devicep, "[RMNET] %s(%d) set urb->status 0 !!!\n", __func__, __LINE__);
				urb->status = 0;
			}
			else {
				dev_err(dev->devicep, "[RMNET] %s(%d) dev->inturb->anchor(%x) !!!\n", __func__, __LINE__, (dev->inturb) ? (unsigned int)dev->inturb->anchor : (unsigned int)(0xffffffff));
				dev_err(dev->devicep, "[RMNET] %s(%d) goto resubmit_int_urb !!!\n", __func__, __LINE__);
				goto resubmit_int_urb;
			}
		}
	}
#endif	

	switch (urb->status) {
	case 0:
		
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

	rx_dev = dev;

	if (test_bit(RMNET_CTRL_DEV_MUX_EN, &dev->status)) {
		ch_id = rmnet_usb_ctrl_dmux(list_elem);
		if (ch_id < 0) {
			kfree(list_elem->cpkt.data);
			kfree(list_elem);
			goto resubmit_int_urb;
		}

		rx_dev = &ctrl_devs[dev->id][ch_id];
	}

	rx_dev->get_encap_resp_cnt++;

	spin_lock(&rx_dev->rx_lock);
	list_add_tail(&list_elem->list, &rx_dev->rx_list);
	spin_unlock(&rx_dev->rx_lock);

	wake_up(&rx_dev->read_wait_queue);

resubmit_int_urb:
	
#ifdef HTC_PM_DBG
	if (usb_pm_debug_enabled)
		usb_mark_intf_last_busy(dev->intf, false);
#endif
	
	
	
	
	
	if ( dev->inturb ) {
		if (!dev->inturb->anchor) {
			usb_mark_last_busy(udev);
			usb_anchor_urb(dev->inturb, &dev->rx_submitted);
			status = usb_submit_urb(dev->inturb, GFP_ATOMIC);
			if (status) {
				usb_unanchor_urb(dev->inturb);
				if (status != -ENODEV)
					dev_err(dev->devicep,
					"%s: Error re-submitting Int URB %d\n",
					__func__, status);
			}
		}
	} else {
		dev_err(dev->devicep, "%s: dev->inturb is NULL\n", __func__);
	}
	
}

int rmnet_usb_ctrl_start_rx(struct rmnet_ctrl_dev *dev)
{
	int	retval = 0;

	usb_anchor_urb(dev->inturb, &dev->rx_submitted);
	retval = usb_submit_urb(dev->inturb, GFP_KERNEL);
	if (retval < 0) {
		usb_unanchor_urb(dev->inturb);
		if (retval != -ENODEV)
			dev_err(dev->devicep,
			"%s Intr submit %d\n", __func__, retval);
	}

	return retval;
}

static int rmnet_usb_ctrl_alloc_rx(struct rmnet_ctrl_dev *dev)
{
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
	dev_info(dev->devicep, "%s init_timer rcv_timer\n", dev->name);
	init_timer(&dev->rcv_timer);
	dev->rcv_timer.function = rmnet_usb_ctrl_read_timer_expire;
	dev->rcv_timer.data = (unsigned long)dev;
#endif	

	return 0;

nomem:
	usb_free_urb(dev->rcvurb);
	kfree(dev->rcvbuf);
	kfree(dev->in_ctlreq);

	return -ENOMEM;

}
static int rmnet_usb_ctrl_write_cmd(struct rmnet_ctrl_dev *dev)
{
	struct usb_device	*udev;

	if (!test_bit(RMNET_CTRL_DEV_READY, &dev->status))
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
	struct ctrl_pkt		*cpkt = context->cpkt;
	struct rmnet_ctrl_dev	*dev = cpkt->ctxt;

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
	struct ctrl_pkt		*cpkt = context->cpkt;
#else 
	struct ctrl_pkt		*cpkt = urb->context;
#endif 
	struct rmnet_ctrl_dev	*dev = cpkt->ctxt;

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
	kfree(cpkt);
#ifdef HTC_DEBUG_QMI_STUCK
	kfree(context);
#endif	
}

static int rmnet_usb_ctrl_write(struct rmnet_ctrl_dev *dev,
		struct ctrl_pkt *cpkt, size_t size)
{
	int			result;
	struct urb		*sndurb;
	struct usb_ctrlrequest	*out_ctlreq;
	struct usb_device	*udev;
#ifdef HTC_DEBUG_QMI_STUCK
	struct ctrl_write_context *context;
#endif	

	if (!test_bit(RMNET_CTRL_DEV_READY, &dev->status))
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
	context->cpkt = cpkt;
#endif	

	
	out_ctlreq->bRequestType = (USB_DIR_OUT | USB_TYPE_CLASS |
			     USB_RECIP_INTERFACE);
	out_ctlreq->bRequest = USB_CDC_SEND_ENCAPSULATED_COMMAND;
	out_ctlreq->wValue = 0;
	out_ctlreq->wIndex = dev->intf->cur_altsetting->desc.bInterfaceNumber;
	out_ctlreq->wLength = cpu_to_le16(cpkt->data_size);

	usb_fill_control_urb(sndurb, udev,
			     usb_sndctrlpipe(udev, 0),
			     (unsigned char *)out_ctlreq, (void *)cpkt->data,
			     cpkt->data_size,
#ifdef HTC_DEBUG_QMI_STUCK
			     ctrl_write_callback, context);
#else	
			     ctrl_write_callback, cpkt);
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
		if (result != -ENODEV)
			dev_err(dev->devicep,
			"%s: Submit URB error %d\n",
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
#ifdef HTC_MDM_RESTART_IF_RMNET_OPEN_TIMEOUT
		extern void htc_ehci_trigger_mdm_restart(void);
#endif
	if (!dev)
		return -ENODEV;

	if (test_bit(RMNET_CTRL_DEV_OPEN, &dev->status))
		goto already_opened;

	if (dev->mdm_wait_timeout &&
			!test_bit(RMNET_CTRL_DEV_READY, &dev->status)) {
		retval = wait_event_interruptible_timeout(
				dev->open_wait_queue,
				test_bit(RMNET_CTRL_DEV_READY, &dev->status),
				msecs_to_jiffies(dev->mdm_wait_timeout * 1000));
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

	if (!test_bit(RMNET_CTRL_DEV_READY, &dev->status)) {
		dev_err(dev->devicep, "%s: Connection timedout opening %s\n",
					__func__, dev->name);

#ifdef HTC_MDM_RESTART_IF_RMNET_OPEN_TIMEOUT
		if ((dev->claimed) && jiffies_to_msecs(jiffies - dev->connected_jiffies) > RMNET_OPEN_TIMEOUT_MS) {
			dev_err(dev->devicep, "%s[%d]:dev->connected_jiffies:%lu jiffies:%lu\n", __func__, __LINE__, dev->connected_jiffies, jiffies);
			dev_err(dev->devicep, "%s[%d]:htc_ehci_trigger_mdm_restart!!!\n", __func__, __LINE__);
			htc_ehci_trigger_mdm_restart();
			msleep(500);
		}
#endif	

		return -ETIMEDOUT;
	}

	set_bit(RMNET_CTRL_DEV_OPEN, &dev->status);
	dev_err(dev->devicep, "%s: Connection opened %s\n",
					__func__, dev->name);
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
	int				time;

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

	clear_bit(RMNET_CTRL_DEV_OPEN, &dev->status);

	time = usb_wait_anchor_empty_timeout(&dev->tx_submitted,
			UNLINK_TIMEOUT_MS);
	if (!time)
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
	if (!test_bit(RMNET_CTRL_DEV_READY, &dev->status)) {
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
	unsigned int			hdr_len = 0;
	struct rmnet_ctrl_dev		*dev;
	struct ctrl_pkt_list_elem	*list_elem = NULL;
	unsigned long			flags;

	dev = file->private_data;
	if (!dev)
		return -ENODEV;

	DBG("%s: Read from %s\n", __func__, dev->name);

ctrl_read:
	if (!test_bit(RMNET_CTRL_DEV_READY, &dev->status)) {
		dev_dbg(dev->devicep, "%s: Device not connected\n",
			__func__);
		return -ENETRESET;
	}
	spin_lock_irqsave(&dev->rx_lock, flags);
	if (list_empty(&dev->rx_list)) {
		spin_unlock_irqrestore(&dev->rx_lock, flags);

		retval = wait_event_interruptible(dev->read_wait_queue,
				!list_empty(&dev->rx_list) ||
				!test_bit(RMNET_CTRL_DEV_READY, &dev->status));
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

	if (test_bit(RMNET_CTRL_DEV_MUX_EN, &dev->status))
		hdr_len = sizeof(struct mux_hdr);

	if (copy_to_user(buf, list_elem->cpkt.data + hdr_len, bytes_to_read)) {
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
	size_t			total_len;
	void			*wbuf;
	void			*actual_data;
	struct ctrl_pkt		*cpkt;
	struct rmnet_ctrl_dev	*dev = file->private_data;

	if (!dev)
		return -ENODEV;

	if (size <= 0)
		return -EINVAL;

	if (!test_bit(RMNET_CTRL_DEV_READY, &dev->status))
		return -ENETRESET;

	DBG("%s: Writing %i bytes on %s\n", __func__, size, dev->name);

	total_len = size;

	if (test_bit(RMNET_CTRL_DEV_MUX_EN, &dev->status))
		total_len += sizeof(struct mux_hdr) + MAX_PAD_BYTES(4);

	wbuf = kmalloc(total_len , GFP_KERNEL);
	if (!wbuf)
		return -ENOMEM;

	cpkt = kmalloc(sizeof(struct ctrl_pkt), GFP_KERNEL);
	if (!cpkt) {
		kfree(wbuf);
		return -ENOMEM;
	}
	actual_data = cpkt->data = wbuf;
	cpkt->data_size = total_len;
	cpkt->ctxt = dev;

	if (test_bit(RMNET_CTRL_DEV_MUX_EN, &dev->status)) {
		actual_data = wbuf + sizeof(struct mux_hdr);
		rmnet_usb_ctrl_mux(dev->ch_id, cpkt);
	}

	status = copy_from_user(actual_data, buf, size);
	if (status) {
		dev_err(dev->devicep,
		"%s: Unable to copy data from userspace %d\n",
		__func__, status);
		kfree(wbuf);
		kfree(cpkt);
		return status;
	}
	DUMP_BUFFER("Write: ", size, buf);

	status = rmnet_usb_ctrl_write(dev, cpkt, size);
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

#define ITC_BIT(nr)   (1UL << (nr))
#define RMNET_CTRL_ITC_DISABLE 0
#define RMNET_CTRL_ITC_ENABLE ITC_BIT(0)
#define RMNET_CTRL_ITC_INIT ITC_BIT(1) 
#define RMNET_CTRL_ITC_AP_START ITC_BIT(2) 
#define RMNET_CTRL_ITC_AP_AUDIO ITC_BIT(2) 
#define RMNET_CTRL_ITC_AP_ISIS ITC_BIT(3) 
#define RMNET_CTRL_ITC_AP_VT ITC_BIT(4) 
#define RMNET_CTRL_ITC_AP_MAX ITC_BIT(4) 
#define RMNET_CTRL_ITC_AP_COUNT 3

#define RMNET_CTRL_ITC_AUDIO_INIT RMNET_CTRL_ITC_AP_AUDIO | RMNET_CTRL_ITC_INIT
#define RMNET_CTRL_ITC_AUDIO_ENABLE RMNET_CTRL_ITC_AP_AUDIO | RMNET_CTRL_ITC_ENABLE
#define RMNET_CTRL_ITC_AUDIO_DISABLE RMNET_CTRL_ITC_AP_AUDIO | RMNET_CTRL_ITC_DISABLE
#define RMNET_CTRL_ITC_ISIS_INIT RMNET_CTRL_ITC_AP_ISIS | RMNET_CTRL_ITC_INIT
#define RMNET_CTRL_ITC_ISIS_ENABLE RMNET_CTRL_ITC_AP_ISIS | RMNET_CTRL_ITC_ENABLE
#define RMNET_CTRL_ITC_ISIS_DISABLE RMNET_CTRL_ITC_AP_ISIS | RMNET_CTRL_ITC_DISABLE
#define RMNET_CTRL_ITC_VT_INIT RMNET_CTRL_ITC_AP_VT | RMNET_CTRL_ITC_INIT
#define RMNET_CTRL_ITC_VT_ENABLE RMNET_CTRL_ITC_AP_VT | RMNET_CTRL_ITC_ENABLE
#define RMNET_CTRL_ITC_VT_DISABLE RMNET_CTRL_ITC_AP_VT | RMNET_CTRL_ITC_DISABLE
unsigned int rmnet_ctrl_itc_catch = 0;

inline void set_itc_bit(unsigned int* value, unsigned int bit)
{
	if (!value)
	{
		pr_err("[%s] value is null\n", __func__);
		return;
	}

	(*value) |= bit;
	return;
}

inline void clear_itc_bit(unsigned int* value, unsigned int bit)
{
	if (!value)
	{
		pr_err("[%s] value is null\n", __func__);
		return;
	}

	if ( (*value) & bit )
	{
		(*value) &= ~bit;
	}

	return;
}

int rmnet_ctrl_set_itc_value_check(int value, int* p_enable, int* p_need_set)
{
	int ret = 0;
	int enable = 0;
	int is_init = 0;
	int ap_bit = 0;
	int need_set = 1;
	int has_ap_in_catch = 0;
	char ap_name[128] = {0};
	int i = 0;

	enable = (value & RMNET_CTRL_ITC_ENABLE) ? 1 : 0;
	is_init = (value & RMNET_CTRL_ITC_INIT) ? 1 : 0;

	for ( i = 0; i < RMNET_CTRL_ITC_AP_COUNT; i++)
	{
		if (value & (RMNET_CTRL_ITC_AP_START << i)) {
			ap_bit = (RMNET_CTRL_ITC_AP_START << i);
			break;
		}
	}

	if ( !ap_bit ) 
	{
		
		pr_err("[%s] can't find ap, value=[0x%x]\n", __func__, value);
		enable = 0;
		need_set = 0;
		ret = -1;
		goto end;
	}

	switch (ap_bit)
	{
		case RMNET_CTRL_ITC_AP_AUDIO:
			sprintf( ap_name, "%s", "audio");
			break;
		case RMNET_CTRL_ITC_AP_ISIS:
			sprintf( ap_name, "%s", "isis");
			break;
		case RMNET_CTRL_ITC_AP_VT:
			sprintf( ap_name, "%s", "vt");
			break;
		default:
			break;
	}

	pr_info("[%s] value=[0x%x], ap=[%s(0x%x)], enable=[%d], is_init=[%d], rmnet_ctrl_itc_catch=[0x%x]\n", __func__, value, ap_name, ap_bit, enable, is_init, rmnet_ctrl_itc_catch);

	if ( is_init ) 
	{
		
		
		clear_itc_bit ( &rmnet_ctrl_itc_catch, ap_bit );
		goto check_if_need_set_itc;
	}

	if (enable)
	{
		
		set_itc_bit ( &rmnet_ctrl_itc_catch, ap_bit );
	}
	else
	{
		
		
		clear_itc_bit ( &rmnet_ctrl_itc_catch, ap_bit );
	}


check_if_need_set_itc:
	
	for ( i = 0; i < RMNET_CTRL_ITC_AP_COUNT; i++)
	{
		if (rmnet_ctrl_itc_catch & (RMNET_CTRL_ITC_AP_START << i)) {
			has_ap_in_catch = (RMNET_CTRL_ITC_AP_START << i);
			break;
		}
	}

	if ( has_ap_in_catch )
	{
		enable = 1;
	}
	else
	{
		enable = 0;
	}

	

	
	if ( enable != ((rmnet_ctrl_itc_catch & RMNET_CTRL_ITC_ENABLE) ? 1 : 0 ) )
	{
		need_set = 1;
		
		if ( enable )
		{
			set_itc_bit ( &rmnet_ctrl_itc_catch, RMNET_CTRL_ITC_ENABLE );
		}
		else
		{
			
			clear_itc_bit ( &rmnet_ctrl_itc_catch, RMNET_CTRL_ITC_ENABLE );
		}
	}
	else
	{
		need_set = 0;
	}

	pr_info("[%s] need_set: value=[0x%x], ap=[%s(0x%x)], enable=[%d], is_init=[%d], rmnet_ctrl_itc_catch=[0x%x], has_ap_in_catch=[%d], need_set=[%d]\n", __func__, value, ap_name, ap_bit, enable, is_init, rmnet_ctrl_itc_catch, has_ap_in_catch, need_set);
end:
	*p_enable = enable;
	*p_need_set = need_set;
	return ret;
}


static int enable_shorten_itc_count = 0;
static int rmnet_ctrl_set_itc( struct rmnet_ctrl_dev *dev, int value ) {
	int ret = 0;
	int enable = 0;
	int is_oldversion = 0;
	int check_itc_err = 0;
	int need_set_itc = 0;
	struct usb_device	*udev;

	if (!dev) {
		pr_info("[%s] dev is null\n", __func__);
		return -ENODEV;
	}

	if (!dev->intf) {
		pr_info("[%s] intf is null\n", __func__);
		return -ENODEV;
	}

	
	mutex_lock(&dev->dev_lock);

	udev = interface_to_usbdev(dev->intf);

	if ( value <= 1 ) {
		is_oldversion = 1;
	}

	if ( is_oldversion ) {
		switch(value) {
			case 1://enable
				enable = 1;
				break;
			case 0://disable
				enable = 0;
				break;
			default://other
				pr_info("[%s][%s] value=[%d]\n", __func__, dev->name, value);
				mutex_unlock(&dev->dev_lock);
				return -ENODEV;
		}
	} else {
		check_itc_err = rmnet_ctrl_set_itc_value_check( value, &enable, &need_set_itc );
		pr_info("[%s][%s] value=[%d], check_itc_err=[%d], enable=[%d], need_set_itc=[%d]\n", __func__, dev->name, value, check_itc_err, enable, need_set_itc);
		if ( check_itc_err ) {
			mutex_unlock(&dev->dev_lock);
			return -ENODEV;
		}
		if ( need_set_itc == 0 ) {
			mutex_unlock(&dev->dev_lock);
			return ret;
		}
	}

	

	if (!(test_bit(RMNET_CTRL_DEV_OPEN, &dev->status) && test_bit(RMNET_CTRL_DEV_READY, &dev->status))) {
		pr_err("[%s] is_opened=[%d], resp_available=[%d]\n", __func__, test_bit(RMNET_CTRL_DEV_OPEN, &dev->status), test_bit(RMNET_CTRL_DEV_READY, &dev->status));
		mutex_unlock(&dev->dev_lock);
		return -ENODEV;
	}
	pr_info("[%s] is_opened=[%d], resp_available=[%d], enable_shorten_itc_count=[%d], enable=[%d], is_oldversion=[%d]\n", __func__, test_bit(RMNET_CTRL_DEV_OPEN, &dev->status), test_bit(RMNET_CTRL_DEV_READY, &dev->status), enable_shorten_itc_count, enable, is_oldversion);
	
	if ( enable ) {
		if (enable_shorten_itc_count == 0) {
			
			ret = usb_autopm_get_interface(dev->intf);
			if (ret < 0) {
				pr_info("[%s] Unable to resume interface: %d\n", __func__, ret);
				mutex_unlock(&dev->dev_lock);
				return -ENODEV;
			}

			pr_info("[%s][%s] usb_set_interrupt_latency(1)\n", __func__, dev->name);
			
			ret = usb_set_interrupt_latency(udev, HSIC_FAST_INTERRUPT_LATENCY);
			
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
				pr_info("[%s][%s] usb_set_interrupt_latency(6)\n", __func__, dev->name);
				
				ret = usb_set_interrupt_latency(udev, HSIC_SLOW_INTERRUPT_LATENCY);
				
				if ( dev->intf )
					usb_autopm_put_interface(dev->intf);
				else
					pr_err("[%s]dev->intf=NULL\n", __func__);
			}
		}
	}
	mutex_unlock(&dev->dev_lock);
	

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

	if (&ctrl_devs[0][0])
		dev = &ctrl_devs[0][0];
	else
		return -ENODEV;

	if (!dev || !file)
		return -ENODEV;

	if (test_bit(RMNET_CTRL_DEV_OPEN, &dev->status)) {
		if (dev->intf)
			dev_info(&dev->intf->dev, "%s[%d]:mdm_wait_timeout:%d resp_available:%d\n", __func__, __LINE__, dev->mdm_wait_timeout, test_bit(RMNET_CTRL_DEV_READY, &dev->status));
	}

	
	if (dev->mdm_wait_timeout && !test_bit(RMNET_CTRL_DEV_READY, &dev->status)) {
		retval = wait_event_interruptible_timeout(
					dev->open_wait_queue,
					test_bit(RMNET_CTRL_DEV_READY, &dev->status),
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

	if (!test_bit(RMNET_CTRL_DEV_READY, &dev->status)) {
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
			 struct usb_host_endpoint *int_in,
			 unsigned long rmnet_devnum,
			 unsigned long *data)
{
	struct rmnet_ctrl_dev		*dev = NULL;
	u16				wMaxPacketSize;
	struct usb_endpoint_descriptor	*ep;
	struct usb_device		*udev = interface_to_usbdev(intf);
	int				interval;
	int				ret = 0, n;

	
	for (n = 0; n < insts_per_dev; n++) {
		dev = &ctrl_devs[rmnet_devnum][n];
		if (!dev->claimed)
			break;
	}

	if (!dev || n == insts_per_dev) {
		pr_err("%s: No available ctrl devices for %lu\n", __func__,
			rmnet_devnum);
		return -ENODEV;
	}

	dev->int_pipe = usb_rcvintpipe(udev,
		int_in->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);

	dev->intf = intf;

	dev->id = rmnet_devnum;

	dev->snd_encap_cmd_cnt = 0;
	dev->get_encap_resp_cnt = 0;
	dev->resp_avail_cnt = 0;
	dev->tx_ctrl_err_cnt = 0;
	dev->set_ctrl_line_state_cnt = 0;

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
	ret = rmnet_usb_ctrl_start_rx(dev);
	if (ret) {
		usb_free_urb(dev->inturb);
		kfree(dev->intbuf);
		return ret;
	}

#ifdef HTC_MDM_RESTART_IF_RMNET_OPEN_TIMEOUT
	if (!ret)
		dev->connected_jiffies = jiffies;
#endif	

	dev->claimed = true;

	
	if (*data)
		set_bit(RMNET_CTRL_DEV_MUX_EN, &dev->status);

	*data = (unsigned long)dev;

	
	if (test_bit(RMNET_CTRL_DEV_MUX_EN, &dev->status)) {
		set_bit(RMNET_CTRL_DEV_READY, &dev->status);
		wake_up(&dev->open_wait_queue);
	}

	return 0;
}

void rmnet_usb_ctrl_disconnect(struct rmnet_ctrl_dev *dev)
{
	dev->claimed = false;

	clear_bit(RMNET_CTRL_DEV_READY, &dev->status);

	mutex_lock(&dev->dev_lock);
	
	dev->cbits_tolocal = ~ACM_CTRL_CD;
	dev->cbits_tomdm = ~ACM_CTRL_DTR;
	mutex_unlock(&dev->dev_lock);

	wake_up(&dev->read_wait_queue);

	cancel_work_sync(&dev->get_encap_work);

	usb_kill_anchored_urbs(&dev->tx_submitted);
	usb_kill_anchored_urbs(&dev->rx_submitted);

	usb_free_urb(dev->inturb);
	dev->inturb = NULL;

	kfree(dev->intbuf);
	dev->intbuf = NULL;
	
	dev_info(&dev->intf->dev, "%s[%d]:dev->resp_available:%d\n", __func__, __LINE__, test_bit(RMNET_CTRL_DEV_READY, &dev->status));
}

#if defined(CONFIG_DEBUG_FS)
#define DEBUG_BUF_SIZE	4096
static ssize_t rmnet_usb_ctrl_read_stats(struct file *file, char __user *ubuf,
		size_t count, loff_t *ppos)
{
	struct rmnet_ctrl_dev	*dev;
	char			*buf;
	int			ret;
	int			i, n;
	int			temp = 0;

	buf = kzalloc(sizeof(char) * DEBUG_BUF_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	for (i = 0; i < num_devs; i++) {
		for (n = 0; n < insts_per_dev; n++) {
			dev = &ctrl_devs[i][n];
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
					"RMNET_CTRL_DEV_MUX_EN:    %d\n"
					"RMNET_CTRL_DEV_OPEN:      %d\n"
					"RMNET_CTRL_DEV_READY:     %d\n",
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
					test_bit(RMNET_CTRL_DEV_MUX_EN,
							&dev->status),
					test_bit(RMNET_CTRL_DEV_OPEN,
							&dev->status),
					test_bit(RMNET_CTRL_DEV_READY,
							&dev->status));
		}
	}

	ret = simple_read_from_buffer(ubuf, count, ppos, buf, temp);
	kfree(buf);
	return ret;
}

static ssize_t rmnet_usb_ctrl_reset_stats(struct file *file, const char __user *
		buf, size_t count, loff_t *ppos)
{
	struct rmnet_ctrl_dev	*dev;
	int			i, n;

	for (i = 0; i < num_devs; i++) {
		for (n = 0; n < insts_per_dev; n++) {
			dev = &ctrl_devs[i][n];

			dev->snd_encap_cmd_cnt = 0;
			dev->resp_avail_cnt = 0;
			dev->get_encap_resp_cnt = 0;
			dev->set_ctrl_line_state_cnt = 0;
			dev->tx_ctrl_err_cnt = 0;
			dev->zlp_cnt = 0;
		}
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

int rmnet_usb_ctrl_init(int no_rmnet_devs, int no_rmnet_insts_per_dev)
{
	struct rmnet_ctrl_dev	*dev;
	int			i, n;
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
	num_devs = no_rmnet_devs;
	insts_per_dev = no_rmnet_insts_per_dev;

	ctrl_devs = kzalloc(num_devs * sizeof(*ctrl_devs), GFP_KERNEL);
	if (!ctrl_devs)
		return -ENOMEM;

	for (i = 0; i < num_devs; i++) {
		ctrl_devs[i] = kzalloc(insts_per_dev * sizeof(*ctrl_devs[i]),
				       GFP_KERNEL);
		if (!ctrl_devs[i])
			return -ENOMEM;

		status = alloc_chrdev_region(&ctrldev_num[i], 0, insts_per_dev,
					     rmnet_dev_names[i]);
		if (IS_ERR_VALUE(status)) {
			pr_err("ERROR:%s: alloc_chrdev_region() ret %i.\n",
				__func__, status);
			return status;
		}

		ctrldev_classp[i] = class_create(THIS_MODULE,
						 rmnet_dev_names[i]);
		if (IS_ERR(ctrldev_classp[i])) {
			pr_err("ERROR:%s: class_create() ENOMEM\n", __func__);
			status = PTR_ERR(ctrldev_classp[i]);
			return status;
		}

		for (n = 0; n < insts_per_dev; n++) {
			dev = &ctrl_devs[i][n];

			
			snprintf(dev->name, CTRL_DEV_MAX_LEN, "%s%d",
				 rmnet_dev_names[i], n);

			dev->wq = create_singlethread_workqueue(dev->name);
			if (!dev->wq) {
				pr_err("unable to allocate workqueue");
				kfree(dev);
				return -ENOMEM;
			}

			dev->ch_id = n;

			mutex_init(&dev->dev_lock);
			spin_lock_init(&dev->rx_lock);
			init_waitqueue_head(&dev->read_wait_queue);
			init_waitqueue_head(&dev->open_wait_queue);
			INIT_LIST_HEAD(&dev->rx_list);
			init_usb_anchor(&dev->tx_submitted);
			init_usb_anchor(&dev->rx_submitted);
			INIT_WORK(&dev->get_encap_work, get_encap_work);

			cdev_init(&dev->cdev, &ctrldev_fops);
			dev->cdev.owner = THIS_MODULE;

			status = cdev_add(&dev->cdev, (ctrldev_num[i] + n), 1);
			if (status) {
				pr_err("%s: cdev_add() ret %i\n", __func__,
					status);
				destroy_workqueue(dev->wq);
				kfree(dev);
				return status;
			}

			dev->devicep = device_create(ctrldev_classp[i], NULL,
						     (ctrldev_num[i] + n), NULL,
						     "%s%d", rmnet_dev_names[i],
						     n);
			if (IS_ERR(dev->devicep)) {
				pr_err("%s: device_create() returned %ld\n",
					__func__, PTR_ERR(dev->devicep));
				cdev_del(&dev->cdev);
				destroy_workqueue(dev->wq);
				kfree(dev);
				return PTR_ERR(dev->devicep);
			}

			
			status = device_create_file(dev->devicep,
						    &dev_attr_modem_wait);
			if (status) {
				device_destroy(dev->devicep->class,
					       dev->devicep->devt);
				cdev_del(&dev->cdev);
				destroy_workqueue(dev->wq);
				kfree(dev);
				return status;
			}
			dev_set_drvdata(dev->devicep, dev);

			status = rmnet_usb_ctrl_alloc_rx(dev);
			if (status) {
				device_remove_file(dev->devicep,
						   &dev_attr_modem_wait);
				device_destroy(dev->devicep->class,
					       dev->devicep->devt);
				cdev_del(&dev->cdev);
				destroy_workqueue(dev->wq);
				kfree(dev);
				return status;
			}
		}
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
}

static void free_rmnet_ctrl_dev(struct rmnet_ctrl_dev *dev)
{
	kfree(dev->in_ctlreq);
	kfree(dev->rcvbuf);
	kfree(dev->intbuf);
	usb_free_urb(dev->rcvurb);
	usb_free_urb(dev->inturb);
	device_remove_file(dev->devicep, &dev_attr_modem_wait);
	cdev_del(&dev->cdev);
	destroy_workqueue(dev->wq);
	device_destroy(dev->devicep->class,
		       dev->devicep->devt);
}

void rmnet_usb_ctrl_exit(int no_rmnet_devs, int no_rmnet_insts_per_dev)
{
	int i, n;

	for (i = 0; i < no_rmnet_devs; i++) {
		for (n = 0; n < no_rmnet_insts_per_dev; n++)
			free_rmnet_ctrl_dev(&ctrl_devs[i][n]);

		kfree(ctrl_devs[i]);

		class_destroy(ctrldev_classp[i]);
		if (ctrldev_num[i])
			unregister_chrdev_region(ctrldev_num[i], insts_per_dev);
	}

	kfree(ctrl_devs);

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
