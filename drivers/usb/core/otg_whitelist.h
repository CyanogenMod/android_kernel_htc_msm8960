/*
 * drivers/usb/core/otg_whitelist.h
 *
 * Copyright (C) 2004 Texas Instruments
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


static struct usb_device_id whitelist_table [] = {

{ USB_DEVICE_INFO(USB_CLASS_HUB, 0, 0), },
{ USB_DEVICE_INFO(USB_CLASS_HUB, 0, 1), },

#ifdef	CONFIG_USB_PRINTER		
{ USB_DEVICE_INFO(7, 1, 1) },
{ USB_DEVICE_INFO(7, 1, 2) },
{ USB_DEVICE_INFO(7, 1, 3) },
#endif

#ifdef	CONFIG_USB_NET_CDCETHER
{ USB_DEVICE(0x0525, 0xa4a1), },
{ USB_DEVICE(0x0525, 0xa4a2), },
#endif

#if	defined(CONFIG_USB_TEST) || defined(CONFIG_USB_TEST_MODULE)
{ USB_DEVICE(0x0525, 0xa4a0), },
#endif
{ USB_DEVICE_CLASS_INFO(USB_CLASS_AUDIO), },
{ USB_INTERFACE_CLASS_INFO(USB_CLASS_AUDIO), },
{ USB_DEVICE_CLASS_INFO(USB_CLASS_HID), },
{ USB_INTERFACE_CLASS_INFO(USB_CLASS_HID), },
{ USB_DEVICE_CLASS_INFO(USB_CLASS_MASS_STORAGE), },
{ USB_INTERFACE_CLASS_INFO(USB_CLASS_MASS_STORAGE), },
{ USB_DEVICE_CLASS_INFO(USB_CLASS_HUB), },
{ }	
};

static int is_targeted(struct usb_device *dev)
{
	int loop,pass_flag;
	struct usb_device_id	*id = whitelist_table;

	
	if (!dev->bus->otg_port)
		return 1;

	
	if ((le16_to_cpu(dev->descriptor.idVendor) == 0x1a0a &&
	     le16_to_cpu(dev->descriptor.idProduct) == 0xbadd))
		return 0;

	
	if ((le16_to_cpu(dev->descriptor.idVendor) == 0x1a0a &&
	     le16_to_cpu(dev->descriptor.idProduct) == 0x0200))
		return 1;
	
	for (loop = 0; loop < dev->config->desc.bNumInterfaces;loop++)
		printk("[USB] USB client interface %d class %d\n",loop,dev->config->intf_cache[loop]->altsetting[0].desc.bInterfaceClass);

	for (id = whitelist_table; id->match_flags; id++) {
		if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
		    id->idVendor != le16_to_cpu(dev->descriptor.idVendor))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
		    id->idProduct != le16_to_cpu(dev->descriptor.idProduct))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_LO) &&
		    (id->bcdDevice_lo > le16_to_cpu(dev->descriptor.bcdDevice)))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_HI) &&
		    (id->bcdDevice_hi < le16_to_cpu(dev->descriptor.bcdDevice)))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS) &&
		    (id->bDeviceClass != dev->descriptor.bDeviceClass))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS) &&
		    (id->bDeviceSubClass != dev->descriptor.bDeviceSubClass))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL) &&
		    (id->bDeviceProtocol != dev->descriptor.bDeviceProtocol))
			continue;
#if defined(CONFIG_USB_PEHCI_HCD) || defined(CONFIG_USB_PEHCI_HCD_MODULE)
		
		if (USB_CLASS_HUB == dev->descriptor.bDeviceClass) {
			
			unsigned char tier = 0;
			struct usb_device *root_hub;

			root_hub = dev->bus->root_hub;
			while ((dev->parent != NULL) && 
				(dev->parent != root_hub) &&
				(tier != 6))  {
				tier++;
				dev = dev->parent;
			}

			if (tier == 6) {
				dev_err(&dev->dev, "5 tier of hubs reached,"
					" newly added hub will not be"
					" supported!\n");
				hub_tier = 1;
				return 0;
			}
		}
#endif

		if (id->match_flags & USB_DEVICE_ID_MATCH_INT_CLASS) {
			pass_flag = 0;
			for (loop = 0; loop < dev->config->desc.bNumInterfaces;loop++) {
				if(dev->config->intf_cache[loop]->altsetting[0].desc.bInterfaceClass == id->bInterfaceClass ) {
					printk("interface class match 0x%x\n",id->bInterfaceClass);
					pass_flag = 1;
					break;
				}
			}
			if (pass_flag == 0)
				continue;
		}

		return 1;
	}

	


#ifdef	CONFIG_USB_OTG_WHITELIST
	
	dev_err(&dev->dev, "device v%04x p%04x is not supported\n",
		le16_to_cpu(dev->descriptor.idVendor),
		le16_to_cpu(dev->descriptor.idProduct));
	return 0;
#else
	return 1;
#endif
}

