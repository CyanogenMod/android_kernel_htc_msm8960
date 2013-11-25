/*
 * Platform data for Android USB
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef	__LINUX_USB_ANDROID_H
#define	__LINUX_USB_ANDROID_H

#include <linux/usb/composite.h>
#include <linux/if_ether.h>

#if 0
struct android_usb_function {
	struct list_head	list;
	char			*name;
	int 			(*bind_config)(struct usb_configuration *c);
};
#endif
struct android_usb_product {
	__u16 vendor_id;

	
	__u16 product_id;

	int num_functions;
	char **functions;
};

struct android_usb_platform_data {
	
	__u16 vendor_id;

	
	__u16 product_id;

	__u16 version;

	char *product_name;
	char *manufacturer_name;
	char *serial_number;

	int num_products;
	struct android_usb_product *products;

	int num_functions;
	char **functions;

	void (*enable_fast_charge)(bool enable);
	bool RndisDisableMPDecision;

	int usb_id_pin_gpio;

	int (*update_pid_and_serial_num)(uint32_t, const char *);

	char *fserial_init_string;

	char *usb_rmnet_interface;
	char *usb_diag_interface;

	unsigned char diag_init:1;
	unsigned char modem_init:1;
	unsigned char rmnet_init:1;
	unsigned char reserved:5;

	

	
	int nluns;
	int cdrom_lun;
	int cdrom_cttype;
	u32 swfi_latency;
	int (*match)(int product_id, int intrsharing);
	u8			usb_core_id;
	int adb_perf_lock_on;
	int mtp_perf_lock_on;
};

struct usb_mass_storage_platform_data {
	
	char *vendor;
	char *product;
	int release;

	char can_stall;
	
	int nluns;
};

struct usb_ether_platform_data {
	u8	ethaddr[ETH_ALEN];
	u32	vendorID;
	const char *vendorDescr;
};
int htc_usb_enable_function(char *name, int ebl);
#if 0
extern void android_register_function(struct android_usb_function *f);

extern int android_enable_function(struct usb_function *f, int enable);
#endif

#endif	
