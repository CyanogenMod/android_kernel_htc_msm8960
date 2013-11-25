#ifndef __USBHID_H
#define __USBHID_H

/*
 *  Copyright (c) 1999 Andreas Gal
 *  Copyright (c) 2000-2001 Vojtech Pavlik
 *  Copyright (c) 2006 Jiri Kosina
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/input.h>

int usbhid_wait_io(struct hid_device* hid);
void usbhid_close(struct hid_device *hid);
int usbhid_open(struct hid_device *hid);
void usbhid_init_reports(struct hid_device *hid);
void usbhid_submit_report
(struct hid_device *hid, struct hid_report *report, unsigned char dir);
int usbhid_get_power(struct hid_device *hid);
void usbhid_put_power(struct hid_device *hid);
struct usb_interface *usbhid_find_interface(int minor);

#define HID_CTRL_RUNNING	1
#define HID_OUT_RUNNING		2
#define HID_IN_RUNNING		3
#define HID_RESET_PENDING	4
#define HID_SUSPENDED		5
#define HID_CLEAR_HALT		6
#define HID_DISCONNECTED	7
#define HID_STARTED		8
#define HID_REPORTED_IDLE	9
#define HID_KEYS_PRESSED	10


struct usbhid_device {
	struct hid_device *hid;						

	struct usb_interface *intf;                                     
	int ifnum;                                                      

	unsigned int bufsize;                                           

	struct urb *urbin;                                              
	char *inbuf;                                                    
	dma_addr_t inbuf_dma;                                           

	struct urb *urbctrl;                                            
	struct usb_ctrlrequest *cr;                                     
	struct hid_control_fifo ctrl[HID_CONTROL_FIFO_SIZE];  		
	unsigned char ctrlhead, ctrltail;                               
	char *ctrlbuf;                                                  
	dma_addr_t ctrlbuf_dma;                                         
	unsigned long last_ctrl;						

	struct urb *urbout;                                             
	struct hid_output_fifo out[HID_CONTROL_FIFO_SIZE];              
	unsigned char outhead, outtail;                                 
	char *outbuf;                                                   
	dma_addr_t outbuf_dma;                                          
	unsigned long last_out;							

	spinlock_t lock;						
	unsigned long iofl;                                             
	struct timer_list io_retry;                                     
	unsigned long stop_retry;                                       
	unsigned int retry_delay;                                       
	struct work_struct reset_work;                                  
	wait_queue_head_t wait;						
	int ledcount;							

	struct work_struct led_work;					
};

#define	hid_to_usb_dev(hid_dev) \
	container_of(hid_dev->dev.parent->parent, struct usb_device, dev)

#endif

