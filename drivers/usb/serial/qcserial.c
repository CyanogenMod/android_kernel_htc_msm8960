/*
 * Qualcomm Serial USB driver
 *
 *	Copyright (c) 2008, 2012 Code Aurora Forum. All rights reserved.
 *	Copyright (c) 2009 Greg Kroah-Hartman <gregkh@suse.de>
 *	Copyright (c) 2009 Novell Inc.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License version
 *	2 as published by the Free Software Foundation.
 *
 */

#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/usb/serial.h>
#include <linux/slab.h>
#include "usb-wwan.h"
#include <mach/board_htc.h>

#define DRIVER_AUTHOR "Qualcomm Inc"
#define DRIVER_DESC "Qualcomm USB Serial driver"

static bool debug;

#define DEVICE_G1K(v, p) \
	USB_DEVICE(v, p), .driver_info = 1

static const struct usb_device_id id_table[] = {
	
	{DEVICE_G1K(0x05c6, 0x9211)},	
	{DEVICE_G1K(0x05c6, 0x9212)},	
	{DEVICE_G1K(0x03f0, 0x1f1d)},	
	{DEVICE_G1K(0x03f0, 0x201d)},	
	{DEVICE_G1K(0x04da, 0x250d)},	
	{DEVICE_G1K(0x04da, 0x250c)},	
	{DEVICE_G1K(0x413c, 0x8172)},	
	{DEVICE_G1K(0x413c, 0x8171)},	
	{DEVICE_G1K(0x1410, 0xa001)},	
	{DEVICE_G1K(0x1410, 0xa008)},	
	{DEVICE_G1K(0x0b05, 0x1776)},	
	{DEVICE_G1K(0x0b05, 0x1774)},	
	{DEVICE_G1K(0x19d2, 0xfff3)},	
	{DEVICE_G1K(0x19d2, 0xfff2)},	
	{DEVICE_G1K(0x1557, 0x0a80)},	
	{DEVICE_G1K(0x05c6, 0x9001)},   
	{DEVICE_G1K(0x05c6, 0x9002)},	
	{DEVICE_G1K(0x05c6, 0x9202)},	
	{DEVICE_G1K(0x05c6, 0x9203)},	
	{DEVICE_G1K(0x05c6, 0x9222)},	
	{DEVICE_G1K(0x05c6, 0x9008)},	
	{DEVICE_G1K(0x05c6, 0x9009)},	
	{DEVICE_G1K(0x05c6, 0x9201)},	
	{DEVICE_G1K(0x05c6, 0x9221)},	
	{DEVICE_G1K(0x05c6, 0x9231)},	
	{DEVICE_G1K(0x1f45, 0x0001)},	

	
	{USB_DEVICE(0x1410, 0xa010)},	
	{USB_DEVICE(0x1410, 0xa011)},	
	{USB_DEVICE(0x1410, 0xa012)},	
	{USB_DEVICE(0x1410, 0xa013)},	
	{USB_DEVICE(0x1410, 0xa014)},	
	{USB_DEVICE(0x413c, 0x8185)},	
	{USB_DEVICE(0x413c, 0x8186)},	
	{USB_DEVICE(0x05c6, 0x9208)},	
	{USB_DEVICE(0x05c6, 0x920b)},	
	{USB_DEVICE(0x05c6, 0x9224)},	
	{USB_DEVICE(0x05c6, 0x9225)},	
	{USB_DEVICE(0x05c6, 0x9244)},	
	{USB_DEVICE(0x05c6, 0x9245)},	
	{USB_DEVICE(0x03f0, 0x241d)},	
	{USB_DEVICE(0x03f0, 0x251d)},	
	{USB_DEVICE(0x05c6, 0x9214)},	
	{USB_DEVICE(0x05c6, 0x9215)},	
	{USB_DEVICE(0x05c6, 0x9264)},	
	{USB_DEVICE(0x05c6, 0x9265)},	
	{USB_DEVICE(0x05c6, 0x9234)},	
	{USB_DEVICE(0x05c6, 0x9235)},	
	{USB_DEVICE(0x05c6, 0x9274)},	
	{USB_DEVICE(0x05c6, 0x9275)},	
	{USB_DEVICE(0x1199, 0x9000)},	
	{USB_DEVICE(0x1199, 0x9001)},	
	{USB_DEVICE(0x1199, 0x9002)},	
	{USB_DEVICE(0x1199, 0x9003)},	
	{USB_DEVICE(0x1199, 0x9004)},	
	{USB_DEVICE(0x1199, 0x9005)},	
	{USB_DEVICE(0x1199, 0x9006)},	
	{USB_DEVICE(0x1199, 0x9007)},	
	{USB_DEVICE(0x1199, 0x9008)},	
	{USB_DEVICE(0x1199, 0x9009)},	
	{USB_DEVICE(0x1199, 0x900a)},	
	{USB_DEVICE(0x1199, 0x9011)},   
	{USB_DEVICE(0x16d8, 0x8001)},	
	{USB_DEVICE(0x16d8, 0x8002)},	
	{USB_DEVICE(0x05c6, 0x9204)},	
	{USB_DEVICE(0x05c6, 0x9205)},	

	
	{USB_DEVICE(0x03f0, 0x371d)},	
	{USB_DEVICE(0x05c6, 0x920c)},	
	{USB_DEVICE(0x05c6, 0x920d)},	
	{USB_DEVICE(0x1410, 0xa020)},   
	{USB_DEVICE(0x1410, 0xa021)},	
	{USB_DEVICE(0x413c, 0x8193)},	
	{USB_DEVICE(0x413c, 0x8194)},	
	{USB_DEVICE(0x1199, 0x9013)},	
	{USB_DEVICE(0x12D1, 0x14F0)},	
	{USB_DEVICE(0x12D1, 0x14F1)},	
	{USB_DEVICE(0x05c6, 0x9048)},	
	{USB_DEVICE(0x05c6, 0x908a)},   
	{USB_DEVICE(0x05c6, 0x904C)},	
	{ }				
};
MODULE_DEVICE_TABLE(usb, id_table);

#define EFS_SYNC_IFC_NUM	2
#define DUN_IFC_NUM 3
static bool usb_diag_enable = false;
#ifdef CONFIG_BUILD_EDIAG
#define SYSMON_IFC_NUM 1
#endif
int usb_serial_reset_resume(struct usb_interface *intf)
{
	pr_info("%s intf %p\n", __func__, intf);
	return usb_serial_resume(intf);
}
static struct usb_driver qcdriver = {
	.name			= "qcserial",
	.probe			= usb_serial_probe,
	.disconnect		= usb_serial_disconnect,
	.id_table		= id_table,
	.suspend		= usb_serial_suspend,
	.resume			= usb_serial_resume,
	.reset_resume = usb_serial_reset_resume,
	.supports_autosuspend	= true,
};

static int qcprobe(struct usb_serial *serial, const struct usb_device_id *id)
{
	struct usb_wwan_intf_private *data;
	struct usb_host_interface *intf = serial->interface->cur_altsetting;
	int retval = -ENODEV;
	__u8 nintf;
	__u8 ifnum;
	bool is_gobi1k = id->driver_info ? true : false;

	dbg("%s", __func__);
	dbg("Is Gobi 1000 = %d", is_gobi1k);

	nintf = serial->dev->actconfig->desc.bNumInterfaces;
	dbg("Num Interfaces = %d", nintf);
	ifnum = intf->desc.bInterfaceNumber;
	dbg("This Interface = %d", ifnum);

	data = kzalloc(sizeof(struct usb_wwan_intf_private),
					 GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	spin_lock_init(&data->susp_lock);

	
	if (!(board_mfg_mode() == 8 || board_mfg_mode() == 6 || board_mfg_mode() == 2))
	
		usb_enable_autosuspend(serial->dev);

	switch (nintf) {
	case 1:
		
		
		if (serial->interface->num_altsetting == 2)
			intf = &serial->interface->altsetting[1];
		else if (serial->interface->num_altsetting > 2)
			break;

		if (intf->desc.bNumEndpoints == 2 &&
		    usb_endpoint_is_bulk_in(&intf->endpoint[0].desc) &&
		    usb_endpoint_is_bulk_out(&intf->endpoint[1].desc)) {
			dbg("QDL port found");

			if (serial->interface->num_altsetting == 1) {
				retval = 0; 
				break;
			}

			retval = usb_set_interface(serial->dev, ifnum, 1);
			if (retval < 0) {
				dev_err(&serial->dev->dev,
					"Could not set interface, error %d\n",
					retval);
				retval = -ENODEV;
				kfree(data);
			}
		}
		break;

	case 3:
	case 4:


		if (ifnum == 1 && !is_gobi1k) {
			dbg("Gobi 2K+ DM/DIAG interface found");
			retval = usb_set_interface(serial->dev, ifnum, 0);
			if (retval < 0) {
				dev_err(&serial->dev->dev,
					"Could not set interface, error %d\n",
					retval);
				retval = -ENODEV;
				kfree(data);
			}
		} else if (ifnum == 2) {
			dbg("Modem port found");
			retval = usb_set_interface(serial->dev, ifnum, 0);
			if (retval < 0) {
				dev_err(&serial->dev->dev,
					"Could not set interface, error %d\n",
					retval);
				retval = -ENODEV;
				kfree(data);
			}
		} else if (ifnum==3 && !is_gobi1k) {
			dbg("Gobi 2K+ NMEA GPS interface found");
			retval = usb_set_interface(serial->dev, ifnum, 0);
			if (retval < 0) {
				dev_err(&serial->dev->dev,
					"Could not set interface, error %d\n",
					retval);
				retval = -ENODEV;
				kfree(data);
			}
		}
		break;
	case 8:
	case 9:
		
		if (get_radio_flag() & 0x20000)
		usb_diag_enable = true;
		
		if (ifnum != EFS_SYNC_IFC_NUM && !(!usb_diag_enable && ifnum == DUN_IFC_NUM && (board_mfg_mode() == 8 || board_mfg_mode() == 6 || board_mfg_mode() == 2))) { 
#ifdef CONFIG_BUILD_EDIAG
			if (ifnum != SYSMON_IFC_NUM) {
#endif
				kfree(data);
				break;
#ifdef CONFIG_BUILD_EDIAG
			}
#endif
		}
#ifdef CONFIG_BUILD_EDIAG
		if (ifnum == SYSMON_IFC_NUM)
			dev_info(&serial->dev->dev, "SYSMON device is created as TTY device\n");
#endif
		retval = 0;
		break;
	default:
		dev_err(&serial->dev->dev,
			"unknown number of interfaces: %d\n", nintf);
		kfree(data);
		retval = -ENODEV;
	}

	
	if (retval != -ENODEV)
		usb_set_serial_data(serial, data);
	return retval;
}

static void qc_release(struct usb_serial *serial)
{
	struct usb_wwan_intf_private *priv = usb_get_serial_data(serial);

	dbg("%s", __func__);

	
	usb_wwan_release(serial);
	usb_set_serial_data(serial, NULL);
	kfree(priv);
}

static struct usb_serial_driver qcdevice = {
	.driver = {
		.owner     = THIS_MODULE,
		.name      = "qcserial",
	},
	.description         = "Qualcomm USB modem",
	.id_table            = id_table,
	.num_ports           = 1,
	.probe               = qcprobe,
	.open		     = usb_wwan_open,
	.close		     = usb_wwan_close,
	.write		     = usb_wwan_write,
	.write_room	     = usb_wwan_write_room,
	.chars_in_buffer     = usb_wwan_chars_in_buffer,
	.throttle            = usb_wwan_throttle,
	.unthrottle          = usb_wwan_unthrottle,
	.attach		     = usb_wwan_startup,
	.disconnect	     = usb_wwan_disconnect,
	.release	     = qc_release,
#ifdef CONFIG_PM
	.suspend	     = usb_wwan_suspend,
	.resume		     = usb_wwan_resume,
#endif
};

static struct usb_serial_driver * const serial_drivers[] = {
	&qcdevice, NULL
};

module_usb_serial_driver(qcdriver, serial_drivers);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL v2");

module_param(debug, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "Debug enabled or not");
