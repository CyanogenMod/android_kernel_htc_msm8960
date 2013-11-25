/*
 * (C) Copyright Linus Torvalds 1999
 * (C) Copyright Johannes Erdfelt 1999-2001
 * (C) Copyright Andreas Gal 1999
 * (C) Copyright Gregory P. Smith 1999
 * (C) Copyright Deti Fliegl 1999
 * (C) Copyright Randy Dunlap 2000
 * (C) Copyright David Brownell 2000-2002
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <linux/utsname.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <asm/irq.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>

#include <linux/usb.h>
#include <linux/usb/hcd.h>

#include "usb.h"

#include <mach/board_htc.h>




unsigned long usb_hcds_loaded;
EXPORT_SYMBOL_GPL(usb_hcds_loaded);

LIST_HEAD (usb_bus_list);
EXPORT_SYMBOL_GPL (usb_bus_list);

#define USB_MAXBUS		64
struct usb_busmap {
	unsigned long busmap [USB_MAXBUS / (8*sizeof (unsigned long))];
};
static struct usb_busmap busmap;

DEFINE_MUTEX(usb_bus_list_lock);	
EXPORT_SYMBOL_GPL (usb_bus_list_lock);

static DEFINE_SPINLOCK(hcd_root_hub_lock);

static DEFINE_SPINLOCK(hcd_urb_list_lock);

static DEFINE_SPINLOCK(hcd_urb_unlink_lock);

DECLARE_WAIT_QUEUE_HEAD(usb_kill_urb_queue);

static inline int is_root_hub(struct usb_device *udev)
{
	return (udev->parent == NULL);
}




#define KERNEL_REL	((LINUX_VERSION_CODE >> 16) & 0x0ff)
#define KERNEL_VER	((LINUX_VERSION_CODE >> 8) & 0x0ff)

static const u8 usb3_rh_dev_descriptor[18] = {
	0x12,       
	0x01,       
	0x00, 0x03, 

	0x09,	    
	0x00,	    
	0x03,       
	0x09,       

	0x6b, 0x1d, 
	0x03, 0x00, 
	KERNEL_VER, KERNEL_REL, 

	0x03,       
	0x02,       
	0x01,       
	0x01        
};

static const u8 usb2_rh_dev_descriptor [18] = {
	0x12,       
	0x01,       
	0x00, 0x02, 

	0x09,	    
	0x00,	    
	0x00,       
	0x40,       

	0x6b, 0x1d, 
	0x02, 0x00, 
	KERNEL_VER, KERNEL_REL, 

	0x03,       
	0x02,       
	0x01,       
	0x01        
};


static const u8 usb11_rh_dev_descriptor [18] = {
	0x12,       
	0x01,       
	0x10, 0x01, 

	0x09,	    
	0x00,	    
	0x00,       
	0x40,       

	0x6b, 0x1d, 
	0x01, 0x00, 
	KERNEL_VER, KERNEL_REL, 

	0x03,       
	0x02,       
	0x01,       
	0x01        
};




static const u8 fs_rh_config_descriptor [] = {

	
	0x09,       
	0x02,       
	0x19, 0x00, 
	0x01,       
	0x01,       
	0x00,       
	0xc0,       
	0x00,       
      

	
	0x09,       
	0x04,       
	0x00,       
	0x00,       
	0x01,       
	0x09,       
	0x00,       
	0x00,       
	0x00,       
     
	
	0x07,       
	0x05,       
	0x81,       
 	0x03,       
 	0x02, 0x00, 
	0xff        
};

static const u8 hs_rh_config_descriptor [] = {

	
	0x09,       
	0x02,       
	0x19, 0x00, 
	0x01,       
	0x01,       
	0x00,       
	0xc0,       
	0x00,       
      

	
	0x09,       
	0x04,       
	0x00,       
	0x00,       
	0x01,       
	0x09,       
	0x00,       
	0x00,       
	0x00,       
     
	
	0x07,       
	0x05,       
	0x81,       
 	0x03,       
	(USB_MAXCHILDREN + 1 + 7) / 8, 0x00,
	0x0c        
};

static const u8 ss_rh_config_descriptor[] = {
	
	0x09,       
	0x02,       
	0x1f, 0x00, 
	0x01,       
	0x01,       
	0x00,       
	0xc0,       
	0x00,       

	
	0x09,       
	0x04,       
	0x00,       
	0x00,       
	0x01,       
	0x09,       
	0x00,       
	0x00,       
	0x00,       

	
	0x07,       
	0x05,       
	0x81,       
	0x03,       
	(USB_MAXCHILDREN + 1 + 7) / 8, 0x00,
	0x0c,       

	
	0x06,        
	0x30,        
	0x00,        
	0x00,        
	0x02, 0x00   
};

static int authorized_default = -1;
module_param(authorized_default, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(authorized_default,
		"Default USB device authorization: 0 is not authorized, 1 is "
		"authorized, -1 is authorized except for wireless USB (default, "
		"old behaviour");

static unsigned
ascii2desc(char const *s, u8 *buf, unsigned len)
{
	unsigned n, t = 2 + 2*strlen(s);

	if (t > 254)
		t = 254;	
	if (len > t)
		len = t;

	t += USB_DT_STRING << 8;	

	n = len;
	while (n--) {
		*buf++ = t;
		if (!n--)
			break;
		*buf++ = t >> 8;
		t = (unsigned char)*s++;
	}
	return len;
}

static unsigned
rh_string(int id, struct usb_hcd const *hcd, u8 *data, unsigned len)
{
	char buf[100];
	char const *s;
	static char const langids[4] = {4, USB_DT_STRING, 0x09, 0x04};

	
	switch (id) {
	case 0:
		
		
		if (len > 4)
			len = 4;
		memcpy(data, langids, len);
		return len;
	case 1:
		
		s = hcd->self.bus_name;
		break;
	case 2:
		
		s = hcd->product_desc;
		break;
	case 3:
		
		snprintf (buf, sizeof buf, "%s %s %s", init_utsname()->sysname,
			init_utsname()->release, hcd->driver->description);
		s = buf;
		break;
	default:
		
		return 0;
	}

	return ascii2desc(s, data, len);
}


static int rh_call_control (struct usb_hcd *hcd, struct urb *urb)
{
	struct usb_ctrlrequest *cmd;
 	u16		typeReq, wValue, wIndex, wLength;
	u8		*ubuf = urb->transfer_buffer;
	u8		tbuf[USB_DT_BOS_SIZE + USB_DT_USB_SS_CAP_SIZE]
		__attribute__((aligned(4)));
	const u8	*bufp = tbuf;
	unsigned	len = 0;
	int		status;
	u8		patch_wakeup = 0;
	u8		patch_protocol = 0;

	might_sleep();

	spin_lock_irq(&hcd_root_hub_lock);
	status = usb_hcd_link_urb_to_ep(hcd, urb);
	spin_unlock_irq(&hcd_root_hub_lock);
	if (status)
		return status;
	urb->hcpriv = hcd;	

	cmd = (struct usb_ctrlrequest *) urb->setup_packet;
	typeReq  = (cmd->bRequestType << 8) | cmd->bRequest;
	wValue   = le16_to_cpu (cmd->wValue);
	wIndex   = le16_to_cpu (cmd->wIndex);
	wLength  = le16_to_cpu (cmd->wLength);

	if (wLength > urb->transfer_buffer_length)
		goto error;

	urb->actual_length = 0;
	switch (typeReq) {

	


	case DeviceRequest | USB_REQ_GET_STATUS:
		tbuf [0] = (device_may_wakeup(&hcd->self.root_hub->dev)
					<< USB_DEVICE_REMOTE_WAKEUP)
				| (1 << USB_DEVICE_SELF_POWERED);
		tbuf [1] = 0;
		len = 2;
		break;
	case DeviceOutRequest | USB_REQ_CLEAR_FEATURE:
		if (wValue == USB_DEVICE_REMOTE_WAKEUP)
			device_set_wakeup_enable(&hcd->self.root_hub->dev, 0);
		else
			goto error;
		break;
	case DeviceOutRequest | USB_REQ_SET_FEATURE:
		if (device_can_wakeup(&hcd->self.root_hub->dev)
				&& wValue == USB_DEVICE_REMOTE_WAKEUP)
			device_set_wakeup_enable(&hcd->self.root_hub->dev, 1);
		else
			goto error;
		break;
	case DeviceRequest | USB_REQ_GET_CONFIGURATION:
		tbuf [0] = 1;
		len = 1;
			
	case DeviceOutRequest | USB_REQ_SET_CONFIGURATION:
		break;
	case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
		switch (wValue & 0xff00) {
		case USB_DT_DEVICE << 8:
			switch (hcd->speed) {
			case HCD_USB3:
				bufp = usb3_rh_dev_descriptor;
				break;
			case HCD_USB2:
				bufp = usb2_rh_dev_descriptor;
				break;
			case HCD_USB11:
				bufp = usb11_rh_dev_descriptor;
				break;
			default:
				goto error;
			}
			len = 18;
			if (hcd->has_tt)
				patch_protocol = 1;
			break;
		case USB_DT_CONFIG << 8:
			switch (hcd->speed) {
			case HCD_USB3:
				bufp = ss_rh_config_descriptor;
				len = sizeof ss_rh_config_descriptor;
				break;
			case HCD_USB2:
				bufp = hs_rh_config_descriptor;
				len = sizeof hs_rh_config_descriptor;
				break;
			case HCD_USB11:
				bufp = fs_rh_config_descriptor;
				len = sizeof fs_rh_config_descriptor;
				break;
			default:
				goto error;
			}
			if (device_can_wakeup(&hcd->self.root_hub->dev))
				patch_wakeup = 1;
			break;
		case USB_DT_STRING << 8:
			if ((wValue & 0xff) < 4)
				urb->actual_length = rh_string(wValue & 0xff,
						hcd, ubuf, wLength);
			else 
				goto error;
			break;
		case USB_DT_BOS << 8:
			goto nongeneric;
		default:
			goto error;
		}
		break;
	case DeviceRequest | USB_REQ_GET_INTERFACE:
		tbuf [0] = 0;
		len = 1;
			
	case DeviceOutRequest | USB_REQ_SET_INTERFACE:
		break;
	case DeviceOutRequest | USB_REQ_SET_ADDRESS:
		
		dev_dbg (hcd->self.controller, "root hub device address %d\n",
			wValue);
		break;

	

	

	case EndpointRequest | USB_REQ_GET_STATUS:
		
		tbuf [0] = 0;
		tbuf [1] = 0;
		len = 2;
			
	case EndpointOutRequest | USB_REQ_CLEAR_FEATURE:
	case EndpointOutRequest | USB_REQ_SET_FEATURE:
		dev_dbg (hcd->self.controller, "no endpoint features yet\n");
		break;

	

	default:
nongeneric:
		
		switch (typeReq) {
		case GetHubStatus:
		case GetPortStatus:
			len = 4;
			break;
		case GetHubDescriptor:
			len = sizeof (struct usb_hub_descriptor);
			break;
		case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
			
			break;
		}
		status = hcd->driver->hub_control (hcd,
			typeReq, wValue, wIndex,
			tbuf, wLength);
		break;
error:
		
		status = -EPIPE;
	}

	if (status < 0) {
		len = 0;
		if (status != -EPIPE) {
			dev_dbg (hcd->self.controller,
				"CTRL: TypeReq=0x%x val=0x%x "
				"idx=0x%x len=%d ==> %d\n",
				typeReq, wValue, wIndex,
				wLength, status);
		}
	} else if (status > 0) {
		
		len = status;
		status = 0;
	}
	if (len) {
		if (urb->transfer_buffer_length < len)
			len = urb->transfer_buffer_length;
		urb->actual_length = len;
		
		memcpy (ubuf, bufp, len);

		
		if (patch_wakeup &&
				len > offsetof (struct usb_config_descriptor,
						bmAttributes))
			((struct usb_config_descriptor *)ubuf)->bmAttributes
				|= USB_CONFIG_ATT_WAKEUP;

		
		if (patch_protocol &&
				len > offsetof(struct usb_device_descriptor,
						bDeviceProtocol))
			((struct usb_device_descriptor *) ubuf)->
				bDeviceProtocol = USB_HUB_PR_HS_SINGLE_TT;
	}

	
	spin_lock_irq(&hcd_root_hub_lock);
	usb_hcd_unlink_urb_from_ep(hcd, urb);

	spin_unlock(&hcd_root_hub_lock);
	usb_hcd_giveback_urb(hcd, urb, status);
	spin_lock(&hcd_root_hub_lock);

	spin_unlock_irq(&hcd_root_hub_lock);
	return 0;
}


void usb_hcd_poll_rh_status(struct usb_hcd *hcd)
{
	struct urb	*urb;
	int		length;
	unsigned long	flags;
	char		buffer[6];	

	if (unlikely(!hcd->rh_pollable))
		return;
	if (!hcd->uses_new_polling && !hcd->status_urb)
		return;

	length = hcd->driver->hub_status_data(hcd, buffer);
	if (length > 0) {

		
		spin_lock_irqsave(&hcd_root_hub_lock, flags);
		urb = hcd->status_urb;
		if (urb) {
			clear_bit(HCD_FLAG_POLL_PENDING, &hcd->flags);
			hcd->status_urb = NULL;
			urb->actual_length = length;
			memcpy(urb->transfer_buffer, buffer, length);

			usb_hcd_unlink_urb_from_ep(hcd, urb);
			spin_unlock(&hcd_root_hub_lock);
			usb_hcd_giveback_urb(hcd, urb, 0);
			spin_lock(&hcd_root_hub_lock);
		} else {
			length = 0;
			set_bit(HCD_FLAG_POLL_PENDING, &hcd->flags);
		}
		spin_unlock_irqrestore(&hcd_root_hub_lock, flags);
	}

	if (hcd->uses_new_polling ? HCD_POLL_RH(hcd) :
			(length == 0 && hcd->status_urb != NULL))
		mod_timer (&hcd->rh_timer, (jiffies/(HZ/4) + 1) * (HZ/4));
}
EXPORT_SYMBOL_GPL(usb_hcd_poll_rh_status);

static void rh_timer_func (unsigned long _hcd)
{
	usb_hcd_poll_rh_status((struct usb_hcd *) _hcd);
}


static int rh_queue_status (struct usb_hcd *hcd, struct urb *urb)
{
	int		retval;
	unsigned long	flags;
	unsigned	len = 1 + (urb->dev->maxchild / 8);

	spin_lock_irqsave (&hcd_root_hub_lock, flags);
	if (hcd->status_urb || urb->transfer_buffer_length < len) {
		dev_dbg (hcd->self.controller, "not queuing rh status urb\n");
		retval = -EINVAL;
		goto done;
	}

	retval = usb_hcd_link_urb_to_ep(hcd, urb);
	if (retval)
		goto done;

	hcd->status_urb = urb;
	urb->hcpriv = hcd;	
	if (!hcd->uses_new_polling)
		mod_timer(&hcd->rh_timer, (jiffies/(HZ/4) + 1) * (HZ/4));

	
	else if (HCD_POLL_PENDING(hcd))
		mod_timer(&hcd->rh_timer, jiffies);
	retval = 0;
 done:
	spin_unlock_irqrestore (&hcd_root_hub_lock, flags);
	return retval;
}

static int rh_urb_enqueue (struct usb_hcd *hcd, struct urb *urb)
{
	if (usb_endpoint_xfer_int(&urb->ep->desc))
		return rh_queue_status (hcd, urb);
	if (usb_endpoint_xfer_control(&urb->ep->desc))
		return rh_call_control (hcd, urb);
	return -EINVAL;
}


static int usb_rh_urb_dequeue(struct usb_hcd *hcd, struct urb *urb, int status)
{
	unsigned long	flags;
	int		rc;

	spin_lock_irqsave(&hcd_root_hub_lock, flags);
	rc = usb_hcd_check_unlink_urb(hcd, urb, status);
	if (rc)
		goto done;

	if (usb_endpoint_num(&urb->ep->desc) == 0) {	
		;	

	} else {				
		if (!hcd->uses_new_polling)
			del_timer (&hcd->rh_timer);
		if (urb == hcd->status_urb) {
			hcd->status_urb = NULL;
			usb_hcd_unlink_urb_from_ep(hcd, urb);

			spin_unlock(&hcd_root_hub_lock);
			usb_hcd_giveback_urb(hcd, urb, status);
			spin_lock(&hcd_root_hub_lock);
		}
	}
 done:
	spin_unlock_irqrestore(&hcd_root_hub_lock, flags);
	return rc;
}



static ssize_t usb_host_authorized_default_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct usb_device *rh_usb_dev = to_usb_device(dev);
	struct usb_bus *usb_bus = rh_usb_dev->bus;
	struct usb_hcd *usb_hcd;

	if (usb_bus == NULL)	
		return -ENODEV;
	usb_hcd = bus_to_hcd(usb_bus);
	return snprintf(buf, PAGE_SIZE, "%u\n", usb_hcd->authorized_default);
}

static ssize_t usb_host_authorized_default_store(struct device *dev,
						 struct device_attribute *attr,
						 const char *buf, size_t size)
{
	ssize_t result;
	unsigned val;
	struct usb_device *rh_usb_dev = to_usb_device(dev);
	struct usb_bus *usb_bus = rh_usb_dev->bus;
	struct usb_hcd *usb_hcd;

	if (usb_bus == NULL)	
		return -ENODEV;
	usb_hcd = bus_to_hcd(usb_bus);
	result = sscanf(buf, "%u\n", &val);
	if (result == 1) {
		usb_hcd->authorized_default = val? 1 : 0;
		result = size;
	}
	else
		result = -EINVAL;
	return result;
}

static DEVICE_ATTR(authorized_default, 0644,
	    usb_host_authorized_default_show,
	    usb_host_authorized_default_store);


static struct attribute *usb_bus_attrs[] = {
		&dev_attr_authorized_default.attr,
		NULL,
};

static struct attribute_group usb_bus_attr_group = {
	.name = NULL,	
	.attrs = usb_bus_attrs,
};




static void usb_bus_init (struct usb_bus *bus)
{
	memset (&bus->devmap, 0, sizeof(struct usb_devmap));

	bus->devnum_next = 1;

	bus->root_hub = NULL;
	bus->busnum = -1;
	bus->bandwidth_allocated = 0;
	bus->bandwidth_int_reqs  = 0;
	bus->bandwidth_isoc_reqs = 0;

	INIT_LIST_HEAD (&bus->bus_list);
#ifdef CONFIG_USB_OTG
	INIT_DELAYED_WORK(&bus->hnp_polling, usb_hnp_polling_work);
#endif
}


static int usb_register_bus(struct usb_bus *bus)
{
	int result = -E2BIG;
	int busnum;

	mutex_lock(&usb_bus_list_lock);
	busnum = find_next_zero_bit (busmap.busmap, USB_MAXBUS, 1);
	if (busnum >= USB_MAXBUS) {
		printk (KERN_ERR "%s: too many buses\n", usbcore_name);
		goto error_find_busnum;
	}
	set_bit (busnum, busmap.busmap);
	bus->busnum = busnum;

	
	list_add (&bus->bus_list, &usb_bus_list);
	mutex_unlock(&usb_bus_list_lock);
#ifdef CONFIG_USB_OTG
	
	if (bus->is_b_host)
		bus->hnp_support = 1;
#endif

	usb_notify_add_bus(bus);

	dev_info (bus->controller, "new USB bus registered, assigned bus "
		  "number %d\n", bus->busnum);
	return 0;

error_find_busnum:
	mutex_unlock(&usb_bus_list_lock);
	return result;
}

static void usb_deregister_bus (struct usb_bus *bus)
{
	dev_info (bus->controller, "USB bus %d deregistered\n", bus->busnum);

	mutex_lock(&usb_bus_list_lock);
	list_del (&bus->bus_list);
	mutex_unlock(&usb_bus_list_lock);

	usb_notify_remove_bus(bus);

	clear_bit (bus->busnum, busmap.busmap);
}

static int register_root_hub(struct usb_hcd *hcd)
{
	struct device *parent_dev = hcd->self.controller;
	struct usb_device *usb_dev = hcd->self.root_hub;
	const int devnum = 1;
	int retval;

	usb_dev->devnum = devnum;
	usb_dev->bus->devnum_next = devnum + 1;
	memset (&usb_dev->bus->devmap.devicemap, 0,
			sizeof usb_dev->bus->devmap.devicemap);
	set_bit (devnum, usb_dev->bus->devmap.devicemap);
	usb_set_device_state(usb_dev, USB_STATE_ADDRESS);

	mutex_lock(&usb_bus_list_lock);

	usb_dev->ep0.desc.wMaxPacketSize = cpu_to_le16(64);
	retval = usb_get_device_descriptor(usb_dev, USB_DT_DEVICE_SIZE);
	if (retval != sizeof usb_dev->descriptor) {
		mutex_unlock(&usb_bus_list_lock);
		dev_dbg (parent_dev, "can't read %s device descriptor %d\n",
				dev_name(&usb_dev->dev), retval);
		return (retval < 0) ? retval : -EMSGSIZE;
	}

	retval = usb_new_device (usb_dev);
	if (retval) {
		dev_err (parent_dev, "can't register root hub for %s, %d\n",
				dev_name(&usb_dev->dev), retval);
	}
	mutex_unlock(&usb_bus_list_lock);

	if (retval == 0) {
		spin_lock_irq (&hcd_root_hub_lock);
		hcd->rh_registered = 1;
		spin_unlock_irq (&hcd_root_hub_lock);

		
		if (HCD_DEAD(hcd))
			usb_hc_died (hcd);	
	}

	return retval;
}



long usb_calc_bus_time (int speed, int is_input, int isoc, int bytecount)
{
	unsigned long	tmp;

	switch (speed) {
	case USB_SPEED_LOW: 	
		if (is_input) {
			tmp = (67667L * (31L + 10L * BitTime (bytecount))) / 1000L;
			return (64060L + (2 * BW_HUB_LS_SETUP) + BW_HOST_DELAY + tmp);
		} else {
			tmp = (66700L * (31L + 10L * BitTime (bytecount))) / 1000L;
			return (64107L + (2 * BW_HUB_LS_SETUP) + BW_HOST_DELAY + tmp);
		}
	case USB_SPEED_FULL:	
		if (isoc) {
			tmp = (8354L * (31L + 10L * BitTime (bytecount))) / 1000L;
			return (((is_input) ? 7268L : 6265L) + BW_HOST_DELAY + tmp);
		} else {
			tmp = (8354L * (31L + 10L * BitTime (bytecount))) / 1000L;
			return (9107L + BW_HOST_DELAY + tmp);
		}
	case USB_SPEED_HIGH:	
		
		if (isoc)
			tmp = HS_NSECS_ISO (bytecount);
		else
			tmp = HS_NSECS (bytecount);
		return tmp;
	default:
		pr_debug ("%s: bogus device speed!\n", usbcore_name);
		return -1;
	}
}
EXPORT_SYMBOL_GPL(usb_calc_bus_time);





int usb_hcd_link_urb_to_ep(struct usb_hcd *hcd, struct urb *urb)
{
	int		rc = 0;

	spin_lock(&hcd_urb_list_lock);

	
	if (unlikely(atomic_read(&urb->reject))) {
		rc = -EPERM;
		goto done;
	}

	if (unlikely(!urb->ep->enabled)) {
		rc = -ENOENT;
		goto done;
	}

	if (unlikely(!urb->dev->can_submit)) {
		rc = -EHOSTUNREACH;
		goto done;
	}

	if (HCD_RH_RUNNING(hcd)) {
		urb->unlinked = 0;
		list_add_tail(&urb->urb_list, &urb->ep->urb_list);
	} else {
		rc = -ESHUTDOWN;
		goto done;
	}
 done:
	spin_unlock(&hcd_urb_list_lock);
	return rc;
}
EXPORT_SYMBOL_GPL(usb_hcd_link_urb_to_ep);

int usb_hcd_check_unlink_urb(struct usb_hcd *hcd, struct urb *urb,
		int status)
{
	struct list_head	*tmp;

	
	list_for_each(tmp, &urb->ep->urb_list) {
		if (tmp == &urb->urb_list)
			break;
	}
	if (tmp != &urb->urb_list)
		return -EIDRM;

	if (urb->unlinked)
		return -EBUSY;
	urb->unlinked = status;
	return 0;
}
EXPORT_SYMBOL_GPL(usb_hcd_check_unlink_urb);

void usb_hcd_unlink_urb_from_ep(struct usb_hcd *hcd, struct urb *urb)
{
	
	spin_lock(&hcd_urb_list_lock);
	list_del_init(&urb->urb_list);
	spin_unlock(&hcd_urb_list_lock);
}
EXPORT_SYMBOL_GPL(usb_hcd_unlink_urb_from_ep);


static int hcd_alloc_coherent(struct usb_bus *bus,
			      gfp_t mem_flags, dma_addr_t *dma_handle,
			      void **vaddr_handle, size_t size,
			      enum dma_data_direction dir)
{
	unsigned char *vaddr;

	if (*vaddr_handle == NULL) {
		WARN_ON_ONCE(1);
		return -EFAULT;
	}

	vaddr = hcd_buffer_alloc(bus, size + sizeof(vaddr),
				 mem_flags, dma_handle);
	if (!vaddr)
		return -ENOMEM;

	put_unaligned((unsigned long)*vaddr_handle,
		      (unsigned long *)(vaddr + size));

	if (dir == DMA_TO_DEVICE)
		memcpy(vaddr, *vaddr_handle, size);

	*vaddr_handle = vaddr;
	return 0;
}

static void hcd_free_coherent(struct usb_bus *bus, dma_addr_t *dma_handle,
			      void **vaddr_handle, size_t size,
			      enum dma_data_direction dir)
{
	unsigned char *vaddr = *vaddr_handle;

	vaddr = (void *)get_unaligned((unsigned long *)(vaddr + size));

	if (dir == DMA_FROM_DEVICE)
		memcpy(vaddr, *vaddr_handle, size);

	hcd_buffer_free(bus, size + sizeof(vaddr), *vaddr_handle, *dma_handle);

	*vaddr_handle = vaddr;
	*dma_handle = 0;
}

void usb_hcd_unmap_urb_setup_for_dma(struct usb_hcd *hcd, struct urb *urb)
{
	if (urb->transfer_flags & URB_SETUP_MAP_SINGLE)
		dma_unmap_single(hcd->self.controller,
				urb->setup_dma,
				sizeof(struct usb_ctrlrequest),
				DMA_TO_DEVICE);
	else if (urb->transfer_flags & URB_SETUP_MAP_LOCAL)
		hcd_free_coherent(urb->dev->bus,
				&urb->setup_dma,
				(void **) &urb->setup_packet,
				sizeof(struct usb_ctrlrequest),
				DMA_TO_DEVICE);

	
	urb->transfer_flags &= ~(URB_SETUP_MAP_SINGLE | URB_SETUP_MAP_LOCAL);
}
EXPORT_SYMBOL_GPL(usb_hcd_unmap_urb_setup_for_dma);

static void unmap_urb_for_dma(struct usb_hcd *hcd, struct urb *urb)
{
	if (hcd->driver->unmap_urb_for_dma)
		hcd->driver->unmap_urb_for_dma(hcd, urb);
	else
		usb_hcd_unmap_urb_for_dma(hcd, urb);
}

void usb_hcd_unmap_urb_for_dma(struct usb_hcd *hcd, struct urb *urb)
{
	enum dma_data_direction dir;

	usb_hcd_unmap_urb_setup_for_dma(hcd, urb);

	dir = usb_urb_dir_in(urb) ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
	if (urb->transfer_flags & URB_DMA_MAP_SG)
		dma_unmap_sg(hcd->self.controller,
				urb->sg,
				urb->num_sgs,
				dir);
	else if (urb->transfer_flags & URB_DMA_MAP_PAGE)
		dma_unmap_page(hcd->self.controller,
				urb->transfer_dma,
				urb->transfer_buffer_length,
				dir);
	else if (urb->transfer_flags & URB_DMA_MAP_SINGLE)
		dma_unmap_single(hcd->self.controller,
				urb->transfer_dma,
				urb->transfer_buffer_length,
				dir);
	else if (urb->transfer_flags & URB_MAP_LOCAL)
		hcd_free_coherent(urb->dev->bus,
				&urb->transfer_dma,
				&urb->transfer_buffer,
				urb->transfer_buffer_length,
				dir);

	
	urb->transfer_flags &= ~(URB_DMA_MAP_SG | URB_DMA_MAP_PAGE |
			URB_DMA_MAP_SINGLE | URB_MAP_LOCAL);
}
EXPORT_SYMBOL_GPL(usb_hcd_unmap_urb_for_dma);

static int map_urb_for_dma(struct usb_hcd *hcd, struct urb *urb,
			   gfp_t mem_flags)
{
	if (hcd->driver->map_urb_for_dma)
		return hcd->driver->map_urb_for_dma(hcd, urb, mem_flags);
	else
		return usb_hcd_map_urb_for_dma(hcd, urb, mem_flags);
}

int usb_hcd_map_urb_for_dma(struct usb_hcd *hcd, struct urb *urb,
			    gfp_t mem_flags)
{
	enum dma_data_direction dir;
	int ret = 0;


	if (usb_endpoint_xfer_control(&urb->ep->desc)) {
		if (hcd->self.uses_pio_for_control)
			return ret;
		if (hcd->self.uses_dma) {
			urb->setup_dma = dma_map_single(
					hcd->self.controller,
					urb->setup_packet,
					sizeof(struct usb_ctrlrequest),
					DMA_TO_DEVICE);
			if (dma_mapping_error(hcd->self.controller,
						urb->setup_dma))
				return -EAGAIN;
			urb->transfer_flags |= URB_SETUP_MAP_SINGLE;
		} else if (hcd->driver->flags & HCD_LOCAL_MEM) {
			ret = hcd_alloc_coherent(
					urb->dev->bus, mem_flags,
					&urb->setup_dma,
					(void **)&urb->setup_packet,
					sizeof(struct usb_ctrlrequest),
					DMA_TO_DEVICE);
			if (ret)
				return ret;
			urb->transfer_flags |= URB_SETUP_MAP_LOCAL;
		}
	}

	dir = usb_urb_dir_in(urb) ? DMA_FROM_DEVICE : DMA_TO_DEVICE;
	if (urb->transfer_buffer_length != 0
	    && !(urb->transfer_flags & URB_NO_TRANSFER_DMA_MAP)) {
		if (hcd->self.uses_dma) {
			if (urb->num_sgs) {
				int n = dma_map_sg(
						hcd->self.controller,
						urb->sg,
						urb->num_sgs,
						dir);
				if (n <= 0)
					ret = -EAGAIN;
				else
					urb->transfer_flags |= URB_DMA_MAP_SG;
				urb->num_mapped_sgs = n;
				if (n != urb->num_sgs)
					urb->transfer_flags |=
							URB_DMA_SG_COMBINED;
			} else if (urb->sg) {
				struct scatterlist *sg = urb->sg;
				urb->transfer_dma = dma_map_page(
						hcd->self.controller,
						sg_page(sg),
						sg->offset,
						urb->transfer_buffer_length,
						dir);
				if (dma_mapping_error(hcd->self.controller,
						urb->transfer_dma))
					ret = -EAGAIN;
				else
					urb->transfer_flags |= URB_DMA_MAP_PAGE;
			} else {
				urb->transfer_dma = dma_map_single(
						hcd->self.controller,
						urb->transfer_buffer,
						urb->transfer_buffer_length,
						dir);
				if (dma_mapping_error(hcd->self.controller,
						urb->transfer_dma))
					ret = -EAGAIN;
				else
					urb->transfer_flags |= URB_DMA_MAP_SINGLE;
			}
		} else if (hcd->driver->flags & HCD_LOCAL_MEM) {
			ret = hcd_alloc_coherent(
					urb->dev->bus, mem_flags,
					&urb->transfer_dma,
					&urb->transfer_buffer,
					urb->transfer_buffer_length,
					dir);
			if (ret == 0)
				urb->transfer_flags |= URB_MAP_LOCAL;
		}
		if (ret && (urb->transfer_flags & (URB_SETUP_MAP_SINGLE |
				URB_SETUP_MAP_LOCAL)))
			usb_hcd_unmap_urb_for_dma(hcd, urb);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(usb_hcd_map_urb_for_dma);


int usb_hcd_submit_urb (struct urb *urb, gfp_t mem_flags)
{
	int			status;
	struct usb_hcd		*hcd = bus_to_hcd(urb->dev->bus);

	usb_get_urb(urb);
	atomic_inc(&urb->use_count);
	atomic_inc(&urb->dev->urbnum);
	usbmon_urb_submit(&hcd->self, urb);


	if (is_root_hub(urb->dev)) {
		status = rh_urb_enqueue(hcd, urb);
	} else {
		status = map_urb_for_dma(hcd, urb, mem_flags);
		if (likely(status == 0)) {
			status = hcd->driver->urb_enqueue(hcd, urb, mem_flags);
			if (unlikely(status))
				unmap_urb_for_dma(hcd, urb);
		}
	}

	if (unlikely(status)) {
		usbmon_urb_submit_error(&hcd->self, urb, status);
		urb->hcpriv = NULL;
		INIT_LIST_HEAD(&urb->urb_list);
		atomic_dec(&urb->use_count);
		atomic_dec(&urb->dev->urbnum);
		if (atomic_read(&urb->reject))
			wake_up(&usb_kill_urb_queue);
		usb_put_urb(urb);
	}
	return status;
}


static int unlink1(struct usb_hcd *hcd, struct urb *urb, int status)
{
	int		value;

	if (is_root_hub(urb->dev))
		value = usb_rh_urb_dequeue(hcd, urb, status);
	else {

		value = hcd->driver->urb_dequeue(hcd, urb, status);
	}
	return value;
}

int usb_hcd_unlink_urb (struct urb *urb, int status)
{
	struct usb_hcd		*hcd;
	int			retval = -EIDRM;
	unsigned long		flags;

	spin_lock_irqsave(&hcd_urb_unlink_lock, flags);
	if (atomic_read(&urb->use_count) > 0) {
		retval = 0;
		usb_get_dev(urb->dev);
	}
	spin_unlock_irqrestore(&hcd_urb_unlink_lock, flags);
	if (retval == 0) {
		hcd = bus_to_hcd(urb->dev->bus);
		retval = unlink1(hcd, urb, status);
		usb_put_dev(urb->dev);
	}

	if (retval == 0)
		retval = -EINPROGRESS;
	else if (retval != -EIDRM && retval != -EBUSY)
		dev_dbg(&urb->dev->dev, "hcd_unlink_urb %p fail %d\n",
				urb, retval);
	return retval;
}


void usb_hcd_giveback_urb(struct usb_hcd *hcd, struct urb *urb, int status)
{
	urb->hcpriv = NULL;
	if (unlikely(urb->unlinked))
		status = urb->unlinked;
	else if (unlikely((urb->transfer_flags & URB_SHORT_NOT_OK) &&
			urb->actual_length < urb->transfer_buffer_length &&
			!status))
		status = -EREMOTEIO;

	unmap_urb_for_dma(hcd, urb);
	usbmon_urb_complete(&hcd->self, urb, status);
	usb_unanchor_urb(urb);

	if (hcd->driver->log_urb_complete)
		hcd->driver->log_urb_complete(urb, "C", status);

	
	urb->status = status;
	urb->complete (urb);
	atomic_dec (&urb->use_count);
	if (unlikely(atomic_read(&urb->reject)))
		wake_up (&usb_kill_urb_queue);
	usb_put_urb (urb);
}
EXPORT_SYMBOL_GPL(usb_hcd_giveback_urb);


void usb_hcd_flush_endpoint(struct usb_device *udev,
		struct usb_host_endpoint *ep)
{
	struct usb_hcd		*hcd;
	struct urb		*urb;

	if (!ep)
		return;
	might_sleep();
	hcd = bus_to_hcd(udev->bus);

	
	spin_lock_irq(&hcd_urb_list_lock);
rescan:
	list_for_each_entry (urb, &ep->urb_list, urb_list) {
		int	is_in;

		if (urb->unlinked)
			continue;
		usb_get_urb (urb);
		is_in = usb_urb_dir_in(urb);
		spin_unlock(&hcd_urb_list_lock);

		
		unlink1(hcd, urb, -ESHUTDOWN);
		dev_dbg (hcd->self.controller,
			"shutdown urb %p ep%d%s%s\n",
			urb, usb_endpoint_num(&ep->desc),
			is_in ? "in" : "out",
			({	char *s;

				 switch (usb_endpoint_type(&ep->desc)) {
				 case USB_ENDPOINT_XFER_CONTROL:
					s = ""; break;
				 case USB_ENDPOINT_XFER_BULK:
					s = "-bulk"; break;
				 case USB_ENDPOINT_XFER_INT:
					s = "-intr"; break;
				 default:
			 		s = "-iso"; break;
				};
				s;
			}));
		usb_put_urb (urb);

		
		spin_lock(&hcd_urb_list_lock);
		goto rescan;
	}
	spin_unlock_irq(&hcd_urb_list_lock);

	
	while (!list_empty (&ep->urb_list)) {
		spin_lock_irq(&hcd_urb_list_lock);

		
		urb = NULL;
		if (!list_empty (&ep->urb_list)) {
			urb = list_entry (ep->urb_list.prev, struct urb,
					urb_list);
			usb_get_urb (urb);
		}
		spin_unlock_irq(&hcd_urb_list_lock);

		if (urb) {
			usb_kill_urb (urb);
			usb_put_urb (urb);
		}
	}
}

int usb_hcd_alloc_bandwidth(struct usb_device *udev,
		struct usb_host_config *new_config,
		struct usb_host_interface *cur_alt,
		struct usb_host_interface *new_alt)
{
	int num_intfs, i, j;
	struct usb_host_interface *alt = NULL;
	int ret = 0;
	struct usb_hcd *hcd;
	struct usb_host_endpoint *ep;

	hcd = bus_to_hcd(udev->bus);
	if (!hcd->driver->check_bandwidth)
		return 0;

	
	if (!new_config && !cur_alt) {
		for (i = 1; i < 16; ++i) {
			ep = udev->ep_out[i];
			if (ep)
				hcd->driver->drop_endpoint(hcd, udev, ep);
			ep = udev->ep_in[i];
			if (ep)
				hcd->driver->drop_endpoint(hcd, udev, ep);
		}
		hcd->driver->check_bandwidth(hcd, udev);
		return 0;
	}
	if (new_config) {
		num_intfs = new_config->desc.bNumInterfaces;
		for (i = 1; i < 16; ++i) {
			ep = udev->ep_out[i];
			if (ep) {
				ret = hcd->driver->drop_endpoint(hcd, udev, ep);
				if (ret < 0)
					goto reset;
			}
			ep = udev->ep_in[i];
			if (ep) {
				ret = hcd->driver->drop_endpoint(hcd, udev, ep);
				if (ret < 0)
					goto reset;
			}
		}
		for (i = 0; i < num_intfs; ++i) {
			struct usb_host_interface *first_alt;
			int iface_num;

			first_alt = &new_config->intf_cache[i]->altsetting[0];
			iface_num = first_alt->desc.bInterfaceNumber;
			
			alt = usb_find_alt_setting(new_config, iface_num, 0);
			if (!alt)
				
				alt = first_alt;

			for (j = 0; j < alt->desc.bNumEndpoints; j++) {
				ret = hcd->driver->add_endpoint(hcd, udev, &alt->endpoint[j]);
				if (ret < 0)
					goto reset;
			}
		}
	}
	if (cur_alt && new_alt) {
		struct usb_interface *iface = usb_ifnum_to_if(udev,
				cur_alt->desc.bInterfaceNumber);

		if (!iface)
			return -EINVAL;
		if (iface->resetting_device) {
			cur_alt = usb_altnum_to_altsetting(iface, 0);
			if (!cur_alt)
				cur_alt = &iface->altsetting[0];
		}

		
		for (i = 0; i < cur_alt->desc.bNumEndpoints; i++) {
			ret = hcd->driver->drop_endpoint(hcd, udev,
					&cur_alt->endpoint[i]);
			if (ret < 0)
				goto reset;
		}
		
		for (i = 0; i < new_alt->desc.bNumEndpoints; i++) {
			ret = hcd->driver->add_endpoint(hcd, udev,
					&new_alt->endpoint[i]);
			if (ret < 0)
				goto reset;
		}
	}
	ret = hcd->driver->check_bandwidth(hcd, udev);
reset:
	if (ret < 0)
		hcd->driver->reset_bandwidth(hcd, udev);
	return ret;
}

void usb_hcd_disable_endpoint(struct usb_device *udev,
		struct usb_host_endpoint *ep)
{
	struct usb_hcd		*hcd;

	might_sleep();
	hcd = bus_to_hcd(udev->bus);
	if (hcd->driver->endpoint_disable)
		hcd->driver->endpoint_disable(hcd, ep);
}

void usb_hcd_reset_endpoint(struct usb_device *udev,
			    struct usb_host_endpoint *ep)
{
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);

	if (hcd->driver->endpoint_reset)
		hcd->driver->endpoint_reset(hcd, ep);
	else {
		int epnum = usb_endpoint_num(&ep->desc);
		int is_out = usb_endpoint_dir_out(&ep->desc);
		int is_control = usb_endpoint_xfer_control(&ep->desc);

		usb_settoggle(udev, epnum, is_out, 0);
		if (is_control)
			usb_settoggle(udev, epnum, !is_out, 0);
	}
}

int usb_alloc_streams(struct usb_interface *interface,
		struct usb_host_endpoint **eps, unsigned int num_eps,
		unsigned int num_streams, gfp_t mem_flags)
{
	struct usb_hcd *hcd;
	struct usb_device *dev;
	int i;

	dev = interface_to_usbdev(interface);
	hcd = bus_to_hcd(dev->bus);
	if (!hcd->driver->alloc_streams || !hcd->driver->free_streams)
		return -EINVAL;
	if (dev->speed != USB_SPEED_SUPER)
		return -EINVAL;

	
	for (i = 0; i < num_eps; i++)
		if (!usb_endpoint_xfer_bulk(&eps[i]->desc))
			return -EINVAL;

	return hcd->driver->alloc_streams(hcd, dev, eps, num_eps,
			num_streams, mem_flags);
}
EXPORT_SYMBOL_GPL(usb_alloc_streams);

void usb_free_streams(struct usb_interface *interface,
		struct usb_host_endpoint **eps, unsigned int num_eps,
		gfp_t mem_flags)
{
	struct usb_hcd *hcd;
	struct usb_device *dev;
	int i;

	dev = interface_to_usbdev(interface);
	hcd = bus_to_hcd(dev->bus);
	if (dev->speed != USB_SPEED_SUPER)
		return;

	
	for (i = 0; i < num_eps; i++)
		if (!eps[i] || !usb_endpoint_xfer_bulk(&eps[i]->desc))
			return;

	hcd->driver->free_streams(hcd, dev, eps, num_eps, mem_flags);
}
EXPORT_SYMBOL_GPL(usb_free_streams);

void usb_hcd_synchronize_unlinks(struct usb_device *udev)
{
	spin_lock_irq(&hcd_urb_unlink_lock);
	spin_unlock_irq(&hcd_urb_unlink_lock);
}


int usb_hcd_get_frame_number (struct usb_device *udev)
{
	struct usb_hcd	*hcd = bus_to_hcd(udev->bus);

	if (!HCD_RH_RUNNING(hcd))
		return -ESHUTDOWN;
	return hcd->driver->get_frame_number (hcd);
}


#ifdef	CONFIG_PM

int hcd_bus_suspend(struct usb_device *rhdev, pm_message_t msg)
{
	struct usb_hcd	*hcd = container_of(rhdev->bus, struct usb_hcd, self);
	int		status;
	int		old_state = hcd->state;

	dev_dbg(&rhdev->dev, "bus %ssuspend, wakeup %d\n",
			(PMSG_IS_AUTO(msg) ? "auto-" : ""),
			rhdev->do_remote_wakeup);
	if (HCD_DEAD(hcd)) {
		dev_dbg(&rhdev->dev, "skipped %s of dead bus\n", "suspend");
		return 0;
	}

	if (!hcd->driver->bus_suspend) {
		status = -ENOENT;
	} else {
		clear_bit(HCD_FLAG_RH_RUNNING, &hcd->flags);
		hcd->state = HC_STATE_QUIESCING;
		status = hcd->driver->bus_suspend(hcd);
	}
	if (status == 0) {
		usb_set_device_state(rhdev, USB_STATE_SUSPENDED);
		hcd->state = HC_STATE_SUSPENDED;

		
		if (rhdev->do_remote_wakeup) {
			char	buffer[6];

			status = hcd->driver->hub_status_data(hcd, buffer);
			if (status != 0) {
				dev_dbg(&rhdev->dev, "suspend raced with wakeup event\n");
				hcd_bus_resume(rhdev, PMSG_AUTO_RESUME);
				status = -EBUSY;
			}
		}
	} else {
		spin_lock_irq(&hcd_root_hub_lock);
		if (!HCD_DEAD(hcd)) {
			set_bit(HCD_FLAG_RH_RUNNING, &hcd->flags);
			hcd->state = old_state;
		}
		spin_unlock_irq(&hcd_root_hub_lock);
		dev_dbg(&rhdev->dev, "bus %s fail, err %d\n",
				"suspend", status);
	}
	return status;
}

int hcd_bus_resume(struct usb_device *rhdev, pm_message_t msg)
{
	struct usb_hcd	*hcd = container_of(rhdev->bus, struct usb_hcd, self);
	int		status;
	int		old_state = hcd->state;

	dev_dbg(&rhdev->dev, "usb %sresume\n",
			(PMSG_IS_AUTO(msg) ? "auto-" : ""));
	if (HCD_DEAD(hcd)) {
		dev_dbg(&rhdev->dev, "skipped %s of dead bus\n", "resume");
		return 0;
	}
	if (!hcd->driver->bus_resume)
		return -ENOENT;
	if (HCD_RH_RUNNING(hcd))
		return 0;

	hcd->state = HC_STATE_RESUMING;
	status = hcd->driver->bus_resume(hcd);
	clear_bit(HCD_FLAG_WAKEUP_PENDING, &hcd->flags);
	if (status == 0) {
		
		msleep(10);
		spin_lock_irq(&hcd_root_hub_lock);
		if (!HCD_DEAD(hcd)) {
			usb_set_device_state(rhdev, rhdev->actconfig
					? USB_STATE_CONFIGURED
					: USB_STATE_ADDRESS);
			set_bit(HCD_FLAG_RH_RUNNING, &hcd->flags);
			hcd->state = HC_STATE_RUNNING;
		}
		spin_unlock_irq(&hcd_root_hub_lock);
	} else {
		hcd->state = old_state;
		dev_dbg(&rhdev->dev, "bus %s fail, err %d\n",
				"resume", status);
		if (status != -ESHUTDOWN)
			usb_hc_died(hcd);
	}
	return status;
}

#endif	

#ifdef	CONFIG_USB_SUSPEND

static void hcd_resume_work(struct work_struct *work)
{
	struct usb_hcd *hcd = container_of(work, struct usb_hcd, wakeup_work);
	struct usb_device *udev = hcd->self.root_hub;

	usb_lock_device(udev);
	usb_remote_wakeup(udev);
	usb_unlock_device(udev);
}

void usb_hcd_resume_root_hub (struct usb_hcd *hcd)
{
	unsigned long flags;

	spin_lock_irqsave (&hcd_root_hub_lock, flags);
	if (hcd->rh_registered) {
		set_bit(HCD_FLAG_WAKEUP_PENDING, &hcd->flags);
		
		if (hcd->product_desc && !strncmp(hcd->product_desc, "Qualcomm EHCI Host Controller using HSIC", 40)) {
			if (get_radio_flag() & RADIO_FLAG_USB_UPLOAD)
				pr_info("%s[%d]: Queue hcd->wakeup_work\n", __FUNCTION__, __LINE__);
			queue_work(pm_rt_wq, &hcd->wakeup_work);
		} else
		
			queue_work(pm_wq, &hcd->wakeup_work);
	}
	spin_unlock_irqrestore (&hcd_root_hub_lock, flags);
}
EXPORT_SYMBOL_GPL(usb_hcd_resume_root_hub);

#endif	


#ifdef	CONFIG_USB_OTG

int usb_bus_start_enum(struct usb_bus *bus, unsigned port_num)
{
	struct usb_hcd		*hcd;
	int			status = -EOPNOTSUPP;

	hcd = container_of (bus, struct usb_hcd, self);
	if (port_num && hcd->driver->start_port_reset)
		status = hcd->driver->start_port_reset(hcd, port_num);

	if (status == 0)
		mod_timer(&hcd->rh_timer, jiffies + msecs_to_jiffies(10));
	return status;
}
EXPORT_SYMBOL_GPL(usb_bus_start_enum);

#endif


irqreturn_t usb_hcd_irq (int irq, void *__hcd)
{
	struct usb_hcd		*hcd = __hcd;
	unsigned long		flags;
	irqreturn_t		rc;

	local_irq_save(flags);

	if (unlikely(HCD_DEAD(hcd) || !HCD_HW_ACCESSIBLE(hcd)))
		rc = IRQ_NONE;
	else if (hcd->driver->irq(hcd) == IRQ_NONE)
		rc = IRQ_NONE;
	else
		rc = IRQ_HANDLED;

	local_irq_restore(flags);
	return rc;
}
EXPORT_SYMBOL_GPL(usb_hcd_irq);


void usb_hc_died (struct usb_hcd *hcd)
{
	unsigned long flags;

	dev_err (hcd->self.controller, "HC died; cleaning up\n");

	spin_lock_irqsave (&hcd_root_hub_lock, flags);
	clear_bit(HCD_FLAG_RH_RUNNING, &hcd->flags);
	set_bit(HCD_FLAG_DEAD, &hcd->flags);
	if (hcd->rh_registered) {
		clear_bit(HCD_FLAG_POLL_RH, &hcd->flags);

		
		usb_set_device_state (hcd->self.root_hub,
				USB_STATE_NOTATTACHED);
		usb_kick_khubd (hcd->self.root_hub);
	}
	if (usb_hcd_is_primary_hcd(hcd) && hcd->shared_hcd) {
		hcd = hcd->shared_hcd;
		if (hcd->rh_registered) {
			clear_bit(HCD_FLAG_POLL_RH, &hcd->flags);

			
			usb_set_device_state(hcd->self.root_hub,
					USB_STATE_NOTATTACHED);
			usb_kick_khubd(hcd->self.root_hub);
		}
	}
	spin_unlock_irqrestore (&hcd_root_hub_lock, flags);
	
}
EXPORT_SYMBOL_GPL (usb_hc_died);


struct usb_hcd *usb_create_shared_hcd(const struct hc_driver *driver,
		struct device *dev, const char *bus_name,
		struct usb_hcd *primary_hcd)
{
	struct usb_hcd *hcd;

	hcd = kzalloc(sizeof(*hcd) + driver->hcd_priv_size, GFP_KERNEL);
	if (!hcd) {
		dev_dbg (dev, "hcd alloc failed\n");
		return NULL;
	}
	if (primary_hcd == NULL) {
		hcd->bandwidth_mutex = kmalloc(sizeof(*hcd->bandwidth_mutex),
				GFP_KERNEL);
		if (!hcd->bandwidth_mutex) {
			kfree(hcd);
			dev_dbg(dev, "hcd bandwidth mutex alloc failed\n");
			return NULL;
		}
		mutex_init(hcd->bandwidth_mutex);
		dev_set_drvdata(dev, hcd);
	} else {
		hcd->bandwidth_mutex = primary_hcd->bandwidth_mutex;
		hcd->primary_hcd = primary_hcd;
		primary_hcd->primary_hcd = primary_hcd;
		hcd->shared_hcd = primary_hcd;
		primary_hcd->shared_hcd = hcd;
	}

	kref_init(&hcd->kref);

	usb_bus_init(&hcd->self);
	hcd->self.controller = dev;
	hcd->self.bus_name = bus_name;
	hcd->self.uses_dma = (dev->dma_mask != NULL);

	init_timer(&hcd->rh_timer);
	hcd->rh_timer.function = rh_timer_func;
	hcd->rh_timer.data = (unsigned long) hcd;
#ifdef CONFIG_USB_SUSPEND
	INIT_WORK(&hcd->wakeup_work, hcd_resume_work);
#endif

	hcd->driver = driver;
	hcd->speed = driver->flags & HCD_MASK;
	hcd->product_desc = (driver->product_desc) ? driver->product_desc :
			"USB Host Controller";
	return hcd;
}
EXPORT_SYMBOL_GPL(usb_create_shared_hcd);

struct usb_hcd *usb_create_hcd(const struct hc_driver *driver,
		struct device *dev, const char *bus_name)
{
	return usb_create_shared_hcd(driver, dev, bus_name, NULL);
}
EXPORT_SYMBOL_GPL(usb_create_hcd);

static void hcd_release (struct kref *kref)
{
	struct usb_hcd *hcd = container_of (kref, struct usb_hcd, kref);

	if (usb_hcd_is_primary_hcd(hcd))
		kfree(hcd->bandwidth_mutex);
	else
		hcd->shared_hcd->shared_hcd = NULL;
	kfree(hcd);
}

struct usb_hcd *usb_get_hcd (struct usb_hcd *hcd)
{
	if (hcd)
		kref_get (&hcd->kref);
	return hcd;
}
EXPORT_SYMBOL_GPL(usb_get_hcd);

void usb_put_hcd (struct usb_hcd *hcd)
{
	if (hcd)
		kref_put (&hcd->kref, hcd_release);
}
EXPORT_SYMBOL_GPL(usb_put_hcd);

int usb_hcd_is_primary_hcd(struct usb_hcd *hcd)
{
	if (!hcd->primary_hcd)
		return 1;
	return hcd == hcd->primary_hcd;
}
EXPORT_SYMBOL_GPL(usb_hcd_is_primary_hcd);

static int usb_hcd_request_irqs(struct usb_hcd *hcd,
		unsigned int irqnum, unsigned long irqflags)
{
	int retval;

	if (hcd->driver->irq) {

		if (irqflags & IRQF_SHARED)
			irqflags &= ~IRQF_DISABLED;

		snprintf(hcd->irq_descr, sizeof(hcd->irq_descr), "%s:usb%d",
				hcd->driver->description, hcd->self.busnum);
		retval = request_irq(irqnum, &usb_hcd_irq, irqflags,
				hcd->irq_descr, hcd);
		if (retval != 0) {
			dev_err(hcd->self.controller,
					"request interrupt %d failed\n",
					irqnum);
			return retval;
		}
		hcd->irq = irqnum;
		dev_info(hcd->self.controller, "irq %d, %s 0x%08llx\n", irqnum,
				(hcd->driver->flags & HCD_MEMORY) ?
					"io mem" : "io base",
					(unsigned long long)hcd->rsrc_start);
	} else {
		hcd->irq = 0;
		if (hcd->rsrc_start)
			dev_info(hcd->self.controller, "%s 0x%08llx\n",
					(hcd->driver->flags & HCD_MEMORY) ?
					"io mem" : "io base",
					(unsigned long long)hcd->rsrc_start);
	}
	return 0;
}

int usb_add_hcd(struct usb_hcd *hcd,
		unsigned int irqnum, unsigned long irqflags)
{
	int retval;
	struct usb_device *rhdev;

	dev_info(hcd->self.controller, "%s\n", hcd->product_desc);

	
	if (authorized_default < 0 || authorized_default > 1)
		hcd->authorized_default = hcd->wireless? 0 : 1;
	else
		hcd->authorized_default = authorized_default;
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	if ((retval = hcd_buffer_create(hcd)) != 0) {
		dev_dbg(hcd->self.controller, "pool alloc failed\n");
		return retval;
	}

	if ((retval = usb_register_bus(&hcd->self)) < 0)
		goto err_register_bus;

	if ((rhdev = usb_alloc_dev(NULL, &hcd->self, 0)) == NULL) {
		dev_err(hcd->self.controller, "unable to allocate root hub\n");
		retval = -ENOMEM;
		goto err_allocate_root_hub;
	}
	hcd->self.root_hub = rhdev;

	switch (hcd->speed) {
	case HCD_USB11:
		rhdev->speed = USB_SPEED_FULL;
		break;
	case HCD_USB2:
		rhdev->speed = USB_SPEED_HIGH;
		break;
	case HCD_USB3:
		rhdev->speed = USB_SPEED_SUPER;
		break;
	default:
		retval = -EINVAL;
		goto err_set_rh_speed;
	}

	device_set_wakeup_capable(&rhdev->dev, 1);

	set_bit(HCD_FLAG_RH_RUNNING, &hcd->flags);

	if (hcd->driver->reset && (retval = hcd->driver->reset(hcd)) < 0) {
		dev_err(hcd->self.controller, "can't setup\n");
		goto err_hcd_driver_setup;
	}
	hcd->rh_pollable = 1;

	
	if (device_can_wakeup(hcd->self.controller)
			&& device_can_wakeup(&hcd->self.root_hub->dev))
		dev_dbg(hcd->self.controller, "supports USB remote wakeup\n");

	if (usb_hcd_is_primary_hcd(hcd) && irqnum) {
		retval = usb_hcd_request_irqs(hcd, irqnum, irqflags);
		if (retval)
			goto err_request_irq;
	}

	hcd->state = HC_STATE_RUNNING;
	retval = hcd->driver->start(hcd);
	if (retval < 0) {
		dev_err(hcd->self.controller, "startup error %d\n", retval);
		goto err_hcd_driver_start;
	}

	
	rhdev->bus_mA = min(500u, hcd->power_budget);
	if ((retval = register_root_hub(hcd)) != 0)
		goto err_register_root_hub;

	retval = sysfs_create_group(&rhdev->dev.kobj, &usb_bus_attr_group);
	if (retval < 0) {
		printk(KERN_ERR "Cannot register USB bus sysfs attributes: %d\n",
		       retval);
		goto error_create_attr_group;
	}
	if (hcd->uses_new_polling && HCD_POLL_RH(hcd))
		usb_hcd_poll_rh_status(hcd);

	device_wakeup_enable(hcd->self.controller);
	return retval;

error_create_attr_group:
	clear_bit(HCD_FLAG_RH_RUNNING, &hcd->flags);
	if (HC_IS_RUNNING(hcd->state))
		hcd->state = HC_STATE_QUIESCING;
	spin_lock_irq(&hcd_root_hub_lock);
	hcd->rh_registered = 0;
	spin_unlock_irq(&hcd_root_hub_lock);

#ifdef CONFIG_USB_SUSPEND
	cancel_work_sync(&hcd->wakeup_work);
#endif
	mutex_lock(&usb_bus_list_lock);
	usb_disconnect(&rhdev);		
	mutex_unlock(&usb_bus_list_lock);
err_register_root_hub:
	hcd->rh_pollable = 0;
	clear_bit(HCD_FLAG_POLL_RH, &hcd->flags);
	del_timer_sync(&hcd->rh_timer);
	hcd->driver->stop(hcd);
	hcd->state = HC_STATE_HALT;
	clear_bit(HCD_FLAG_POLL_RH, &hcd->flags);
	del_timer_sync(&hcd->rh_timer);
err_hcd_driver_start:
	if (usb_hcd_is_primary_hcd(hcd) && hcd->irq > 0)
		free_irq(irqnum, hcd);
err_request_irq:
err_hcd_driver_setup:
err_set_rh_speed:
	usb_put_dev(hcd->self.root_hub);
err_allocate_root_hub:
	usb_deregister_bus(&hcd->self);
err_register_bus:
	hcd_buffer_destroy(hcd);
	return retval;
} 
EXPORT_SYMBOL_GPL(usb_add_hcd);

void usb_remove_hcd(struct usb_hcd *hcd)
{
	struct usb_device *rhdev = hcd->self.root_hub;

	dev_info(hcd->self.controller, "remove, state %x\n", hcd->state);

	usb_get_dev(rhdev);
	sysfs_remove_group(&rhdev->dev.kobj, &usb_bus_attr_group);

	clear_bit(HCD_FLAG_RH_RUNNING, &hcd->flags);
	if (HC_IS_RUNNING (hcd->state))
		hcd->state = HC_STATE_QUIESCING;

	dev_dbg(hcd->self.controller, "roothub graceful disconnect\n");
	spin_lock_irq (&hcd_root_hub_lock);
	hcd->rh_registered = 0;
	spin_unlock_irq (&hcd_root_hub_lock);

#ifdef CONFIG_USB_SUSPEND
	cancel_work_sync(&hcd->wakeup_work);
#endif

	mutex_lock(&usb_bus_list_lock);
	usb_disconnect(&rhdev);		
	mutex_unlock(&usb_bus_list_lock);

	hcd->rh_pollable = 0;
	clear_bit(HCD_FLAG_POLL_RH, &hcd->flags);
	del_timer_sync(&hcd->rh_timer);

	hcd->driver->stop(hcd);
	hcd->state = HC_STATE_HALT;

	
	clear_bit(HCD_FLAG_POLL_RH, &hcd->flags);
	del_timer_sync(&hcd->rh_timer);

	if (usb_hcd_is_primary_hcd(hcd)) {
		if (hcd->irq > 0)
			free_irq(hcd->irq, hcd);
	}

	usb_put_dev(hcd->self.root_hub);
	usb_deregister_bus(&hcd->self);
	hcd_buffer_destroy(hcd);
}
EXPORT_SYMBOL_GPL(usb_remove_hcd);

void
usb_hcd_platform_shutdown(struct platform_device* dev)
{
	struct usb_hcd *hcd = platform_get_drvdata(dev);

	if (hcd->driver->shutdown)
		hcd->driver->shutdown(hcd);
}
EXPORT_SYMBOL_GPL(usb_hcd_platform_shutdown);


#if defined(CONFIG_USB_MON) || defined(CONFIG_USB_MON_MODULE)

struct usb_mon_operations *mon_ops;

 
int usb_mon_register (struct usb_mon_operations *ops)
{

	if (mon_ops)
		return -EBUSY;

	mon_ops = ops;
	mb();
	return 0;
}
EXPORT_SYMBOL_GPL (usb_mon_register);

void usb_mon_deregister (void)
{

	if (mon_ops == NULL) {
		printk(KERN_ERR "USB: monitor was not registered\n");
		return;
	}
	mon_ops = NULL;
	mb();
}
EXPORT_SYMBOL_GPL (usb_mon_deregister);

#endif 
