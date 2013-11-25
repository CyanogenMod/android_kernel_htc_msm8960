/* Driver for USB Mass Storage compliant devices
 * Main Header File
 *
 * Current development and maintenance by:
 *   (c) 1999-2002 Matthew Dharm (mdharm-usb@one-eyed-alien.net)
 *
 * Initial work by:
 *   (c) 1999 Michael Gee (michael@linuxspecific.com)
 *
 * This driver is based on the 'USB Mass Storage Class' document. This
 * describes in detail the protocol used to communicate with such
 * devices.  Clearly, the designers had SCSI and ATAPI commands in
 * mind when they created this document.  The commands are all very
 * similar to commands in the SCSI-II and ATAPI specifications.
 *
 * It is important to note that in a number of cases this class
 * exhibits class-specific exemptions from the USB specification.
 * Notably the usage of NAK, STALL and ACK differs from the norm, in
 * that they are used to communicate wait, failed and OK on commands.
 *
 * Also, for certain devices, the interrupt endpoint is used to convey
 * status of a command.
 *
 * Please see http://www.one-eyed-alien.net/~mdharm/linux-usb for more
 * information about this driver.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _USB_H_
#define _USB_H_

#include <linux/usb.h>
#include <linux/usb_usual.h>
#include <linux/blkdev.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <scsi/scsi_host.h>

struct us_data;
struct scsi_cmnd;


struct us_unusual_dev {
	const char* vendorName;
	const char* productName;
	__u8  useProtocol;
	__u8  useTransport;
	int (*initFunction)(struct us_data *);
};


#define US_FLIDX_URB_ACTIVE	0	
#define US_FLIDX_SG_ACTIVE	1	
#define US_FLIDX_ABORTING	2	
#define US_FLIDX_DISCONNECTING	3	
#define US_FLIDX_RESETTING	4	
#define US_FLIDX_TIMED_OUT	5	
#define US_FLIDX_SCAN_PENDING	6	
#define US_FLIDX_REDO_READ10	7	
#define US_FLIDX_READ10_WORKED	8	

#define USB_STOR_STRING_LEN 32


#define US_IOBUF_SIZE		64	
#define US_SENSE_SIZE		18	

typedef int (*trans_cmnd)(struct scsi_cmnd *, struct us_data*);
typedef int (*trans_reset)(struct us_data*);
typedef void (*proto_cmnd)(struct scsi_cmnd*, struct us_data*);
typedef void (*extra_data_destructor)(void *);	
typedef void (*pm_hook)(struct us_data *, int);	

#define US_SUSPEND	0
#define US_RESUME	1

struct us_data {
	struct mutex		dev_mutex;	 
	struct usb_device	*pusb_dev;	 
	struct usb_interface	*pusb_intf;	 
	struct us_unusual_dev   *unusual_dev;	 
	unsigned long		fflags;		 
	unsigned long		dflags;		 
	unsigned int		send_bulk_pipe;	 
	unsigned int		recv_bulk_pipe;
	unsigned int		send_ctrl_pipe;
	unsigned int		recv_ctrl_pipe;
	unsigned int		recv_intr_pipe;

	
	char			*transport_name;
	char			*protocol_name;
	__le32			bcs_signature;
	u8			subclass;
	u8			protocol;
	u8			max_lun;

	u8			ifnum;		 
	u8			ep_bInterval;	  

	
	trans_cmnd		transport;	 
	trans_reset		transport_reset; 
	proto_cmnd		proto_handler;	 

	
	struct scsi_cmnd	*srb;		 
	unsigned int		tag;		 
	char			scsi_name[32];	 

	
	struct urb		*current_urb;	 
	struct usb_ctrlrequest	*cr;		 
	struct usb_sg_request	current_sg;	 
	unsigned char		*iobuf;		 
	dma_addr_t		iobuf_dma;	 
	struct task_struct	*ctl_thread;	 

	
	struct completion	cmnd_ready;	 
	struct completion	notify;		 
	wait_queue_head_t	delay_wait;	 
	struct delayed_work	scan_dwork;	 

	
	void			*extra;		 
	extra_data_destructor	extra_destructor;
#ifdef CONFIG_PM
	pm_hook			suspend_resume_hook;
#endif

	
	int			use_last_sector_hacks;
	int			last_sector_retries;
};

static inline struct Scsi_Host *us_to_host(struct us_data *us) {
	return container_of((void *) us, struct Scsi_Host, hostdata);
}
static inline struct us_data *host_to_us(struct Scsi_Host *host) {
	return (struct us_data *) host->hostdata;
}

extern void fill_inquiry_response(struct us_data *us,
	unsigned char *data, unsigned int data_len);

#define scsi_unlock(host)	spin_unlock_irq(host->host_lock)
#define scsi_lock(host)		spin_lock_irq(host->host_lock)

#ifdef CONFIG_PM
extern int usb_stor_suspend(struct usb_interface *iface, pm_message_t message);
extern int usb_stor_resume(struct usb_interface *iface);
extern int usb_stor_reset_resume(struct usb_interface *iface);
#else
#define usb_stor_suspend	NULL
#define usb_stor_resume		NULL
#define usb_stor_reset_resume	NULL
#endif

extern int usb_stor_pre_reset(struct usb_interface *iface);
extern int usb_stor_post_reset(struct usb_interface *iface);

extern int usb_stor_probe1(struct us_data **pus,
		struct usb_interface *intf,
		const struct usb_device_id *id,
		struct us_unusual_dev *unusual_dev);
extern int usb_stor_probe2(struct us_data *us);
extern void usb_stor_disconnect(struct usb_interface *intf);

#endif
