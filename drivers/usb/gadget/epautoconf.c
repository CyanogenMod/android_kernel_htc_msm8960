/*
 * epautoconf.c -- endpoint autoconfiguration for usb gadget drivers
 *
 * Copyright (C) 2004 David Brownell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>

#include <linux/ctype.h>
#include <linux/string.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include "gadget_chips.h"


static unsigned epnum;

#ifdef MANY_ENDPOINTS
static unsigned in_epnum;
#endif


static int
ep_matches (
	struct usb_gadget		*gadget,
	struct usb_ep			*ep,
	struct usb_endpoint_descriptor	*desc,
	struct usb_ss_ep_comp_descriptor *ep_comp
)
{
	u8		type;
	const char	*tmp;
	u16		max;

	int		num_req_streams = 0;

	
	if (NULL != ep->driver_data)
		return 0;

	
	type = desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
	if (USB_ENDPOINT_XFER_CONTROL == type)
		return 0;

	
	if ('e' != ep->name[0])
		return 0;

	if ('-' != ep->name[2]) {
		tmp = strrchr (ep->name, '-');
		if (tmp) {
			switch (type) {
			case USB_ENDPOINT_XFER_INT:
				if ('s' == tmp[2])	
					return 0;
				if (gadget_is_pxa (gadget)
						&& 'i' == tmp [1])
					return 0;
				break;
			case USB_ENDPOINT_XFER_BULK:
				if ('b' != tmp[1])	
					return 0;
				break;
			case USB_ENDPOINT_XFER_ISOC:
				if ('s' != tmp[2])	
					return 0;
			}
		} else {
			tmp = ep->name + strlen (ep->name);
		}

		
		tmp--;
		if (!isdigit (*tmp)) {
			if (desc->bEndpointAddress & USB_DIR_IN) {
				if ('n' != *tmp)
					return 0;
			} else {
				if ('t' != *tmp)
					return 0;
			}
		}
	}

	if (usb_endpoint_xfer_bulk(desc)) {
		if (ep_comp && gadget->max_speed >= USB_SPEED_SUPER) {
			num_req_streams = ep_comp->bmAttributes & 0x1f;
			if (num_req_streams > ep->max_streams)
				return 0;
		}

	}

	if (desc->wMaxPacketSize == 0)
		desc->wMaxPacketSize = cpu_to_le16(ep->maxpacket);

	max = 0x7ff & usb_endpoint_maxp(desc);
	switch (type) {
	case USB_ENDPOINT_XFER_INT:
		
		if (!gadget_is_dualspeed(gadget) && max > 64)
			return 0;
		

	case USB_ENDPOINT_XFER_ISOC:
		
		if (ep->maxpacket < max)
			return 0;
		if (!gadget_is_dualspeed(gadget) && max > 1023)
			return 0;

		
		if ((desc->wMaxPacketSize & cpu_to_le16(3<<11))) {
			if (!gadget_is_dualspeed(gadget))
				return 0;
			
		}
		break;
	}

	

	
	desc->bEndpointAddress &= USB_DIR_IN;
	if (isdigit (ep->name [2])) {
		u8	num = simple_strtoul (&ep->name [2], NULL, 10);
		desc->bEndpointAddress |= num;
#ifdef	MANY_ENDPOINTS
	} else if (desc->bEndpointAddress & USB_DIR_IN) {
		if (++in_epnum > 15)
			return 0;
		desc->bEndpointAddress = USB_DIR_IN | in_epnum;
#endif
	} else {
		if (++epnum > 15)
			return 0;
		desc->bEndpointAddress |= epnum;
	}

	
	if ((USB_ENDPOINT_XFER_BULK == type) && !ep_comp) {
		int size = ep->maxpacket;

		
		if (size > 64)
			size = 64;
		desc->wMaxPacketSize = cpu_to_le16(size);
	}
	ep->address = desc->bEndpointAddress;
	return 1;
}

static struct usb_ep *
find_ep (struct usb_gadget *gadget, const char *name)
{
	struct usb_ep	*ep;

	list_for_each_entry (ep, &gadget->ep_list, ep_list) {
		if (0 == strcmp (ep->name, name))
			return ep;
	}
	return NULL;
}

struct usb_ep *usb_ep_autoconfig_ss(
	struct usb_gadget		*gadget,
	struct usb_endpoint_descriptor	*desc,
	struct usb_ss_ep_comp_descriptor *ep_comp
)
{
	struct usb_ep	*ep;
	u8		type;

	type = desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;

	if (gadget_is_net2280 (gadget) && type == USB_ENDPOINT_XFER_INT) {
		
		ep = find_ep (gadget, "ep-e");
		if (ep && ep_matches(gadget, ep, desc, ep_comp))
			goto found_ep;
		ep = find_ep (gadget, "ep-f");
		if (ep && ep_matches(gadget, ep, desc, ep_comp))
			goto found_ep;

	} else if (gadget_is_goku (gadget)) {
		if (USB_ENDPOINT_XFER_INT == type) {
			
			ep = find_ep(gadget, "ep3-bulk");
			if (ep && ep_matches(gadget, ep, desc, ep_comp))
				goto found_ep;
		} else if (USB_ENDPOINT_XFER_BULK == type
				&& (USB_DIR_IN & desc->bEndpointAddress)) {
			
			ep = find_ep(gadget, "ep2-bulk");
			if (ep && ep_matches(gadget, ep, desc,
					      ep_comp))
				goto found_ep;
		}

#ifdef CONFIG_BLACKFIN
	} else if (gadget_is_musbhdrc(gadget)) {
		if ((USB_ENDPOINT_XFER_BULK == type) ||
		    (USB_ENDPOINT_XFER_ISOC == type)) {
			if (USB_DIR_IN & desc->bEndpointAddress)
				ep = find_ep (gadget, "ep5in");
			else
				ep = find_ep (gadget, "ep6out");
		} else if (USB_ENDPOINT_XFER_INT == type) {
			if (USB_DIR_IN & desc->bEndpointAddress)
				ep = find_ep(gadget, "ep1in");
			else
				ep = find_ep(gadget, "ep2out");
		} else
			ep = NULL;
		if (ep && ep_matches(gadget, ep, desc, ep_comp))
			goto found_ep;
#endif
	}

	
	list_for_each_entry (ep, &gadget->ep_list, ep_list) {
		if (ep_matches(gadget, ep, desc, ep_comp))
			goto found_ep;
	}

	
	return NULL;
found_ep:
	ep->desc = NULL;
	ep->comp_desc = NULL;
	return ep;
}

struct usb_ep *usb_ep_autoconfig(
	struct usb_gadget		*gadget,
	struct usb_endpoint_descriptor	*desc
)
{
	return usb_ep_autoconfig_ss(gadget, desc, NULL);
}


void usb_ep_autoconfig_reset (struct usb_gadget *gadget)
{
	struct usb_ep	*ep;

	list_for_each_entry (ep, &gadget->ep_list, ep_list) {
		ep->driver_data = NULL;
	}
#ifdef	MANY_ENDPOINTS
	in_epnum = 0;
#endif
	epnum = 0;
}

