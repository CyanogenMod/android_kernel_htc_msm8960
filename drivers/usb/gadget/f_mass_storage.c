/*
 * f_mass_storage.c -- Mass Storage USB Composite Function
 *
 * Copyright (C) 2003-2008 Alan Stern
 * Copyright (C) 2009 Samsung Electronics
 *                    Author: Michal Nazarewicz <mina86@mina86.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the above-listed copyright holders may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") as published by the Free Software
 * Foundation, either version 2 of that License or (at your option) any
 * later version.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */





#include <linux/blkdev.h>
#include <linux/completion.h>
#include <linux/dcache.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kref.h>
#include <linux/kthread.h>
#include <linux/limits.h>
#include <linux/rwsem.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/freezer.h>
#include <linux/utsname.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>

#include "gadget_chips.h"



#define FSG_DRIVER_DESC		"Mass Storage Function"
#define FSG_DRIVER_VERSION	"2009/09/11"

static const char fsg_string_interface[] = "Mass Storage";

#define FSG_NO_DEVICE_STRINGS    1
#define FSG_NO_OTG               1
#define FSG_NO_INTR_EP           1

#include "storage_common.c"

#ifdef CONFIG_PASCAL_DETECT
extern struct switch_dev kddi_switch;
extern atomic_t pascal_enable;
#endif

#ifdef CONFIG_USB_CSW_HACK
static int write_error_after_csw_sent;
static int csw_hack_sent;
#endif

struct fsg_dev;
struct fsg_common;

struct fsg_operations {
	int (*thread_exits)(struct fsg_common *common);

	int (*pre_eject)(struct fsg_common *common,
			 struct fsg_lun *lun, int num);
	int (*post_eject)(struct fsg_common *common,
			  struct fsg_lun *lun, int num);
};

struct fsg_common {
	struct usb_gadget	*gadget;
	struct usb_composite_dev *cdev;
	struct fsg_dev		*fsg, *new_fsg;
	wait_queue_head_t	fsg_wait;

	
	struct rw_semaphore	filesem;

	
	spinlock_t		lock;

	struct usb_ep		*ep0;		
	struct usb_request	*ep0req;	
	unsigned int		ep0_req_tag;

	struct fsg_buffhd	*next_buffhd_to_fill;
	struct fsg_buffhd	*next_buffhd_to_drain;
	struct fsg_buffhd	*buffhds;

	int			cmnd_size;
	u8			cmnd[MAX_COMMAND_SIZE];

	unsigned int		nluns;
	unsigned int		lun;
	struct fsg_lun		*luns;
	struct fsg_lun		*curlun;

	unsigned int		bulk_out_maxpacket;
	enum fsg_state		state;		
	unsigned int		exception_req_tag;

	enum data_direction	data_dir;
	u32			data_size;
	u32			data_size_from_cmnd;
	u32			tag;
	u32			residue;
	u32			usb_amount_left;

	unsigned int		can_stall:1;
	unsigned int		free_storage_on_release:1;
	unsigned int		phase_error:1;
	unsigned int		short_packet_received:1;
	unsigned int		bad_lun_okay:1;
	unsigned int		running:1;

	int			thread_wakeup_needed;
	struct completion	thread_notifier;
	struct task_struct	*thread_task;

	
	const struct fsg_operations	*ops;
	
	void			*private_data;

	char inquiry_string[8 + 16 + 4 + 1];

	struct kref		ref;
};

struct fsg_config {
	unsigned nluns;
	struct fsg_lun_config {
		const char *filename;
		char ro;
		char removable;
		char cdrom;
		char nofua;
	} luns[FSG_MAX_LUNS];

	const char		*lun_name_format;
	const char		*thread_name;

	
	const struct fsg_operations	*ops;
	
	void			*private_data;

	const char *vendor_name;		
	const char *product_name;		
	u16 release;

	char			can_stall;
};

struct fsg_dev {
	struct usb_function	function;
	struct usb_gadget	*gadget;	
	struct fsg_common	*common;

	u16			interface_number;

	unsigned int		bulk_in_enabled:1;
	unsigned int		bulk_out_enabled:1;

	unsigned long		atomic_bitflags;
#define IGNORE_BULK_OUT		0

	struct usb_ep		*bulk_in;
	struct usb_ep		*bulk_out;
};

static inline int __fsg_is_set(struct fsg_common *common,
			       const char *func, unsigned line)
{
	if (common->fsg)
		return 1;
	ERROR(common, "common->fsg is NULL in %s at %u\n", func, line);
	WARN_ON(1);
	return 0;
}

#define fsg_is_set(common) likely(__fsg_is_set(common, __func__, __LINE__))

static inline struct fsg_dev *fsg_from_func(struct usb_function *f)
{
	return container_of(f, struct fsg_dev, function);
}

typedef void (*fsg_routine_t)(struct fsg_dev *);
static int send_status(struct fsg_common *common);
int android_switch_function(unsigned func);

static int exception_in_progress(struct fsg_common *common)
{
	return common->state > FSG_STATE_IDLE;
}

static void set_bulk_out_req_length(struct fsg_common *common,
				    struct fsg_buffhd *bh, unsigned int length)
{
	unsigned int	rem;

	bh->bulk_out_intended_length = length;
	rem = length % common->bulk_out_maxpacket;
	if (rem > 0)
		length += common->bulk_out_maxpacket - rem;
	bh->outreq->length = length;
}



static int fsg_set_halt(struct fsg_dev *fsg, struct usb_ep *ep)
{
	const char	*name;

	if (ep == fsg->bulk_in)
		name = "bulk-in";
	else if (ep == fsg->bulk_out)
		name = "bulk-out";
	else
		name = ep->name;
	DBG(fsg, "%s set halt\n", name);
	return usb_ep_set_halt(ep);
}




static void wakeup_thread(struct fsg_common *common)
{
	
	common->thread_wakeup_needed = 1;
	if (common->thread_task)
		wake_up_process(common->thread_task);
}

static void raise_exception(struct fsg_common *common, enum fsg_state new_state)
{
	unsigned long		flags;

	spin_lock_irqsave(&common->lock, flags);
	if (common->state <= new_state) {
		common->exception_req_tag = common->ep0_req_tag;
		common->state = new_state;
		if (common->thread_task)
			send_sig_info(SIGUSR1, SEND_SIG_FORCED,
				      common->thread_task);
	}
	spin_unlock_irqrestore(&common->lock, flags);
}



static int ep0_queue(struct fsg_common *common)
{
	int	rc;

	rc = usb_ep_queue(common->ep0, common->ep0req, GFP_ATOMIC);
	common->ep0->driver_data = common;
	if (rc != 0 && rc != -ESHUTDOWN) {
		
		WARNING(common, "error in submission: %s --> %d\n",
			common->ep0->name, rc);
	}
	return rc;
}




static void bulk_in_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct fsg_common	*common = ep->driver_data;
	struct fsg_buffhd	*bh = req->context;

	if (req->status || req->actual != req->length)
		DBG(common, "%s --> %d, %u/%u\n", __func__,
		    req->status, req->actual, req->length);
	if (req->status == -ECONNRESET)		
		usb_ep_fifo_flush(ep);

	
	smp_wmb();
	spin_lock(&common->lock);
	bh->inreq_busy = 0;
	bh->state = BUF_STATE_EMPTY;
	wakeup_thread(common);
	spin_unlock(&common->lock);
}

static void bulk_out_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct fsg_common	*common = ep->driver_data;
	struct fsg_buffhd	*bh = req->context;

	dump_msg(common, "bulk-out", req->buf, req->actual);
	if (req->status || req->actual != bh->bulk_out_intended_length)
		DBG(common, "%s --> %d, %u/%u\n", __func__,
		    req->status, req->actual, bh->bulk_out_intended_length);
	if (req->status == -ECONNRESET)		
		usb_ep_fifo_flush(ep);

	
	smp_wmb();
	spin_lock(&common->lock);
	bh->outreq_busy = 0;
	bh->state = BUF_STATE_FULL;
	wakeup_thread(common);
	spin_unlock(&common->lock);
}

static int fsg_setup(struct usb_function *f,
		     const struct usb_ctrlrequest *ctrl)
{
	struct fsg_dev		*fsg = fsg_from_func(f);
	struct usb_request	*req = fsg->common->ep0req;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);

	if (!fsg_is_set(fsg->common))
		return -EOPNOTSUPP;

	++fsg->common->ep0_req_tag;	
	req->context = NULL;
	req->length = 0;
	dump_msg(fsg, "ep0-setup", (u8 *) ctrl, sizeof(*ctrl));

	switch (ctrl->bRequest) {

	case US_BULK_RESET_REQUEST:
		if (ctrl->bRequestType !=
		    (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE))
			break;
		if (w_index != fsg->interface_number || w_value != 0 ||
				w_length != 0)
			return -EDOM;

		DBG(fsg, "bulk reset request\n");
		raise_exception(fsg->common, FSG_STATE_RESET);
		return DELAYED_STATUS;

	case US_BULK_GET_MAX_LUN:
		if (ctrl->bRequestType !=
		    (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE))
			break;
		if (w_index != fsg->interface_number || w_value != 0 ||
				w_length != 1)
			return -EDOM;
		VDBG(fsg, "get max LUN\n");
		*(u8 *)req->buf = fsg->common->nluns - 1;

		
		req->length = min((u16)1, w_length);
		return ep0_queue(fsg->common);
	}

	VDBG(fsg,
	     "unknown class-specific control req %02x.%02x v%04x i%04x l%u\n",
	     ctrl->bRequestType, ctrl->bRequest,
	     le16_to_cpu(ctrl->wValue), w_index, w_length);
	return -EOPNOTSUPP;
}




static void start_transfer(struct fsg_dev *fsg, struct usb_ep *ep,
			   struct usb_request *req, int *pbusy,
			   enum fsg_buffer_state *state)
{
	int	rc;

	if (ep == fsg->bulk_in)
		dump_msg(fsg, "bulk-in", req->buf, req->length);

	spin_lock_irq(&fsg->common->lock);
	*pbusy = 1;
	*state = BUF_STATE_BUSY;
	spin_unlock_irq(&fsg->common->lock);
	rc = usb_ep_queue(ep, req, GFP_KERNEL);
	if (rc != 0) {
		*pbusy = 0;
		*state = BUF_STATE_EMPTY;

		

		if (rc != -ESHUTDOWN &&
		    !(rc == -EOPNOTSUPP && req->length == 0))
			WARNING(fsg, "error in submission: %s --> %d\n",
				ep->name, rc);
	}
}

static bool start_in_transfer(struct fsg_common *common, struct fsg_buffhd *bh)
{
	if (!fsg_is_set(common))
		return false;
	start_transfer(common->fsg, common->fsg->bulk_in,
		       bh->inreq, &bh->inreq_busy, &bh->state);
	return true;
}

static bool start_out_transfer(struct fsg_common *common, struct fsg_buffhd *bh)
{
	if (!fsg_is_set(common))
		return false;
	start_transfer(common->fsg, common->fsg->bulk_out,
		       bh->outreq, &bh->outreq_busy, &bh->state);
	return true;
}

static int sleep_thread(struct fsg_common *common)
{
	int	rc = 0;

	
	for (;;) {
		try_to_freeze();
		set_current_state(TASK_INTERRUPTIBLE);
		if (signal_pending(current)) {
			rc = -EINTR;
			break;
		}
		if (common->thread_wakeup_needed)
			break;
		schedule();
	}
	__set_current_state(TASK_RUNNING);
	common->thread_wakeup_needed = 0;
	return rc;
}


static void _lba_to_msf(u8 *buf, int lba)
{
    lba += 150;
    buf[0] = (lba / 75) / 60;
    buf[1] = (lba / 75) % 60;
    buf[2] = lba % 75;
}


static int _read_toc_raw(struct fsg_common *common, struct fsg_buffhd *bh)
{
	struct fsg_lun	*curlun = common->curlun;
	int		msf = common->cmnd[1] & 0x02;
	u8		*buf = (u8 *) bh->buf;
	u8		*q;
	int		len;

	q = buf + 2;
	memset(q, 0, 46);
	*q++ = 1; 
	*q++ = 1; 

	*q++ = 1; 
	*q++ = 0x14; 
	*q++ = 0; 
	*q++ = 0xa0; 
	*q++ = 0; 
	*q++ = 0; 
	*q++ = 0; 
	*q++ = 0;
	*q++ = 1; 
	*q++ = 0x00; 
	*q++ = 0x00;

	*q++ = 1; 
	*q++ = 0x14; 
	*q++ = 0; 
	*q++ = 0xa1;
	*q++ = 0; 
	*q++ = 0; 
	*q++ = 0; 
	*q++ = 0;
	*q++ = 1; 
	*q++ = 0x00;
	*q++ = 0x00;

	*q++ = 1; 
	*q++ = 0x14; 
	*q++ = 0; 
	*q++ = 0xa2; 
	*q++ = 0; 
	*q++ = 0; 
	*q++ = 0; 
	if (msf) {
		*q++ = 0; 
		_lba_to_msf(q, curlun->num_sectors);
		q += 3;
	} else {
		put_unaligned_be32(curlun->num_sectors, q);
		q += 4;
	}

	*q++ = 1; 
	*q++ = 0x14; 
	*q++ = 0; 
	*q++ = 1; 
	*q++ = 0; 
	*q++ = 0; 
	*q++ = 0; 
	if (msf) {
		*q++ = 0;
		_lba_to_msf(q, 0);
		q += 3;
	} else {
		memset(q, 0, 4);
		q += 4;
	}

	len = q - buf;
	put_unaligned_be16(len - 2, buf);

	return len;
}


static void cd_data_to_raw(u8 *buf, int lba)
{
	
	buf[0] = 0x00;
	memset(buf + 1, 0xff, 10);
	buf[11] = 0x00;
	buf += 12;

	
	_lba_to_msf(buf, lba);
	buf[3] = 0x01; 
	buf += 4;

	
	buf += 2048;

	
	memset(buf, 0, 288);
}



static int do_read(struct fsg_common *common)
{
	struct fsg_lun		*curlun = common->curlun;
	u32			lba;
	struct fsg_buffhd	*bh;
	int			rc;
	u32			amount_left;
	loff_t			file_offset, file_offset_tmp;
	unsigned int		amount;
	ssize_t			nread;
	u32			transfer_request;
#ifdef CONFIG_USB_MSC_PROFILING
	ktime_t			start, diff;
#endif

	if (common->cmnd[0] == READ_CD) {
		if (common->data_size_from_cmnd == 0)
			return 0;
		transfer_request = common->cmnd[9];
	} else
		transfer_request = 0;

	if (common->cmnd[0] == READ_CD)
		lba = get_unaligned_be32(&common->cmnd[2]);
	else if (common->cmnd[0] == READ_6)
		lba = get_unaligned_be24(&common->cmnd[1]);
	else {
		lba = get_unaligned_be32(&common->cmnd[2]);

		if ((common->cmnd[1] & ~0x18) != 0) {
			curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
			return -EINVAL;
		}
	}
	if (lba >= curlun->num_sectors) {
		curlun->sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		return -EINVAL;
	}

	if ((transfer_request & 0xf8) == 0xf8) {
		file_offset = ((loff_t) lba) << 11;

		
		amount_left = 2352;
	} else {
		file_offset = ((loff_t) lba) << curlun->blkbits;

		
		amount_left = common->data_size_from_cmnd;
	}
	if (unlikely(amount_left == 0))
		return -EIO;		

	for (;;) {
		amount = min(amount_left, FSG_BUFLEN);
		amount = min((loff_t)amount,
			     curlun->file_length - file_offset);

		
		bh = common->next_buffhd_to_fill;
		while (bh->state != BUF_STATE_EMPTY) {
			rc = sleep_thread(common);
			if (rc)
				return rc;
		}

		if (amount == 0) {
			curlun->sense_data =
					SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
			curlun->sense_data_info =
					file_offset >> curlun->blkbits;
			curlun->info_valid = 1;
			bh->inreq->length = 0;
			bh->state = BUF_STATE_FULL;
			break;
		}

		
		file_offset_tmp = file_offset;

#ifdef CONFIG_USB_MSC_PROFILING
		start = ktime_get();
#endif
		if ((transfer_request & 0xf8) == 0xf8)
			nread = vfs_read(curlun->filp,
				    ((char __user *)bh->buf) + 16,
				    amount, &file_offset_tmp);
		else
			nread = vfs_read(curlun->filp,
				    (char __user *)bh->buf,
				    amount, &file_offset_tmp);
		VLDBG(curlun, "file read %u @ %llu -> %d\n", amount,
		     (unsigned long long) file_offset, (int) nread);
#ifdef CONFIG_USB_MSC_PROFILING
		diff = ktime_sub(ktime_get(), start);
		curlun->perf.rbytes += nread;
		curlun->perf.rtime = ktime_add(curlun->perf.rtime, diff);
#endif
		if (signal_pending(current))
			return -EINTR;

		if (nread < 0) {
			LDBG(curlun, "error in file read: %d\n", (int)nread);
			nread = 0;
		} else if (nread < amount) {
			LDBG(curlun, "partial file read: %d/%u\n",
			     (int)nread, amount);
			nread = round_down(nread, curlun->blksize);
		}
		file_offset  += nread;
		amount_left  -= nread;
		common->residue -= nread;

		bh->inreq->length = nread;
		bh->state = BUF_STATE_FULL;

		
		if (nread < amount) {
			curlun->sense_data = SS_UNRECOVERED_READ_ERROR;
			curlun->sense_data_info =
					file_offset >> curlun->blkbits;
			curlun->info_valid = 1;
			break;
		}

		if (amount_left == 0)
			break;		

		
		bh->inreq->zero = 0;
		if (!start_in_transfer(common, bh))
			
			return -EIO;
		common->next_buffhd_to_fill = bh->next;
	}

	if ((transfer_request & 0xf8) == 0xf8)
		cd_data_to_raw(bh->buf, lba);

	return -EIO;		
}
#ifdef CONFIG_LISMO
static int do_read_buffer(struct fsg_common *common)
{
	struct fsg_lun		*curlun = common->curlun;
	struct fsg_buffhd	*bh;
	int			rc;
	u32			amount_left;
	loff_t			file_offset;
	unsigned int		amount;
	struct op_desc		*desc = 0;

	file_offset = get_unaligned_be32(&common->cmnd[2]);

	desc = curlun->op_desc[common->cmnd[0]-SC_VENDOR_START];
	if (!desc->buffer){
		printk("%s: cmd=%d not ready\n", __func__, common->cmnd[0]);
		curlun->sense_data =
				SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		curlun->sense_data_info = file_offset;
		curlun->info_valid = 1;
		bh = common->next_buffhd_to_fill;
		bh->inreq->length = 0;
		bh->state = BUF_STATE_FULL;
		return -EIO;		
	}


	
	amount_left = common->data_size_from_cmnd;

	if (file_offset + amount_left > desc->len) {
		printk("[fms_CR7]%s: vendor buffer out of range offset=0x%x read-len=0x%x buf-len=0x%x\n",
		__func__, (unsigned int)file_offset, amount_left, desc->len);
		curlun->sense_data =
				SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		curlun->sense_data_info = file_offset;
		curlun->info_valid = 1;
		bh = common->next_buffhd_to_fill;
		bh->inreq->length = 0;
		bh->state = BUF_STATE_FULL;
		return -EIO;		
	}

	
	if (unlikely(amount_left == 0))
		return -EIO;		

	

	for (;;) {
		

		amount = min(amount_left, FSG_BUFLEN);
		amount = min((loff_t) amount, desc->len - file_offset);
		

		
		bh = common->next_buffhd_to_fill;
		while (bh->state != BUF_STATE_EMPTY) {
			rc = sleep_thread(common);
			if (rc)
				return rc;
		}
		

		if (amount == 0) {
			curlun->sense_data =
					SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
			curlun->sense_data_info = file_offset;
			curlun->info_valid = 1;
			bh->inreq->length = 0;
			bh->state = BUF_STATE_FULL;
			break;
		}

		memcpy((char __user *) bh->buf, desc->buffer + file_offset, amount);

		file_offset  += amount;
		amount_left  -= amount;
		common->residue -= amount;
		bh->inreq->length = amount;
		bh->state = BUF_STATE_FULL;

		if (amount_left == 0)
			break;		

		
#if 0
		START_TRANSFER_OR(common, bulk_in, bh->inreq,
				&bh->inreq_busy, &bh->state)
#else
		bh->inreq->zero = 0;
		if (!start_in_transfer(common, bh))
			
#endif
			return -EIO;
		common->next_buffhd_to_fill = bh->next;
	}
	return -EIO;		
}
#endif

static int do_write(struct fsg_common *common)
{
	struct fsg_lun		*curlun = common->curlun;
	u32			lba;
	struct fsg_buffhd	*bh;
	int			get_some_more;
	u32			amount_left_to_req, amount_left_to_write;
	loff_t			usb_offset, file_offset, file_offset_tmp;
	unsigned int		amount;
	ssize_t			nwritten;
	int			rc;

#ifdef CONFIG_USB_CSW_HACK
	int			i;
#endif

#ifdef CONFIG_USB_MSC_PROFILING
	ktime_t			start, diff;
#endif
	if (curlun->ro) {
		curlun->sense_data = SS_WRITE_PROTECTED;
		return -EINVAL;
	}
	spin_lock(&curlun->filp->f_lock);
	curlun->filp->f_flags &= ~O_SYNC;	
	spin_unlock(&curlun->filp->f_lock);

	if (common->cmnd[0] == WRITE_6)
		lba = get_unaligned_be24(&common->cmnd[1]);
	else {
		lba = get_unaligned_be32(&common->cmnd[2]);

		if (common->cmnd[1] & ~0x18) {
			curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
			return -EINVAL;
		}
		if (!curlun->nofua && (common->cmnd[1] & 0x08)) { 
			spin_lock(&curlun->filp->f_lock);
			curlun->filp->f_flags |= O_SYNC;
			spin_unlock(&curlun->filp->f_lock);
		}
	}
	if (lba >= curlun->num_sectors) {
		curlun->sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		return -EINVAL;
	}

	
	get_some_more = 1;
	file_offset = usb_offset = ((loff_t) lba) << curlun->blkbits;
	amount_left_to_req = common->data_size_from_cmnd;
	amount_left_to_write = common->data_size_from_cmnd;

	while (amount_left_to_write > 0) {

		
		bh = common->next_buffhd_to_fill;
		if (bh->state == BUF_STATE_EMPTY && get_some_more) {

			amount = min(amount_left_to_req, FSG_BUFLEN);

			
			if (usb_offset >= curlun->file_length) {
				get_some_more = 0;
				curlun->sense_data =
					SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
				curlun->sense_data_info =
					usb_offset >> curlun->blkbits;
				curlun->info_valid = 1;
				continue;
			}

			
			usb_offset += amount;
			common->usb_amount_left -= amount;
			amount_left_to_req -= amount;
			if (amount_left_to_req == 0)
				get_some_more = 0;

			set_bulk_out_req_length(common, bh, amount);
			if (!start_out_transfer(common, bh))
				
				return -EIO;
			common->next_buffhd_to_fill = bh->next;
			continue;
		}

		
		bh = common->next_buffhd_to_drain;
		if (bh->state == BUF_STATE_EMPTY && !get_some_more)
			break;			
#ifdef CONFIG_USB_CSW_HACK
		/*
		 * If the csw packet is already submmitted to the hardware,
		 * by marking the state of buffer as full, then by checking
		 * the residue, we make sure that this csw packet is not
		 * written on to the storage media.
		 */
		if (bh->state == BUF_STATE_FULL && common->residue) {
#else
		if (bh->state == BUF_STATE_FULL) {
#endif
			smp_rmb();
			common->next_buffhd_to_drain = bh->next;
			bh->state = BUF_STATE_EMPTY;

			
			if (bh->outreq->status != 0) {
				curlun->sense_data = SS_COMMUNICATION_FAILURE;
				curlun->sense_data_info =
					file_offset >> curlun->blkbits;
				curlun->info_valid = 1;
				break;
			}

			amount = bh->outreq->actual;
			if (curlun->file_length - file_offset < amount) {
				LERROR(curlun,
				       "write %u @ %llu beyond end %llu\n",
				       amount, (unsigned long long)file_offset,
				       (unsigned long long)curlun->file_length);
				amount = curlun->file_length - file_offset;
			}

			amount = min(amount, bh->bulk_out_intended_length);

			
			amount = round_down(amount, curlun->blksize);
			if (amount == 0)
				goto empty_write;

			
			file_offset_tmp = file_offset;
#ifdef CONFIG_USB_MSC_PROFILING
			start = ktime_get();
#endif
			nwritten = vfs_write(curlun->filp,
					     (char __user *)bh->buf,
					     amount, &file_offset_tmp);
			VLDBG(curlun, "file write %u @ %llu -> %d\n", amount,
			      (unsigned long long)file_offset, (int)nwritten);
#ifdef CONFIG_USB_MSC_PROFILING
			diff = ktime_sub(ktime_get(), start);
			curlun->perf.wbytes += nwritten;
			curlun->perf.wtime =
					ktime_add(curlun->perf.wtime, diff);
#endif
			if (signal_pending(current))
				return -EINTR;		

			if (nwritten < 0) {
				LDBG(curlun, "error in file write: %d\n",
				     (int)nwritten);
				nwritten = 0;
			} else if (nwritten < amount) {
				LDBG(curlun, "partial file write: %d/%u\n",
				     (int)nwritten, amount);
				nwritten = round_down(nwritten, curlun->blksize);
			}
			file_offset += nwritten;
			amount_left_to_write -= nwritten;
			common->residue -= nwritten;

			
			if (nwritten < amount) {
				curlun->sense_data = SS_WRITE_ERROR;
				curlun->sense_data_info =
					file_offset >> curlun->blkbits;
				curlun->info_valid = 1;
#ifdef CONFIG_USB_CSW_HACK
				write_error_after_csw_sent = 1;
				goto write_error;
#endif
				break;
			}

#ifdef CONFIG_USB_CSW_HACK
write_error:
			if ((nwritten == amount) && !csw_hack_sent) {
				if (write_error_after_csw_sent)
					break;
				for (i = 0; i < fsg_num_buffers; i++) {
					if (common->buffhds[i].state ==
							BUF_STATE_BUSY)
						break;
				}
				if (!amount_left_to_req && i == fsg_num_buffers) {
					csw_hack_sent = 1;
					send_status(common);
				}
			}
#endif

 empty_write:
			
			if (bh->outreq->actual < bh->bulk_out_intended_length) {
				common->short_packet_received = 1;
				break;
			}
			continue;
		}

		
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}

	return -EIO;		
}

#ifdef CONFIG_LISMO

static int do_write_buffer(struct fsg_common *common)
{
	struct fsg_lun		*curlun = common->curlun;
	struct fsg_buffhd	*bh;
	int			get_some_more;
	u32			amount_left_to_req, amount_left_to_write;
	loff_t			file_offset;
	unsigned int		amount;
	int			rc;
	struct op_desc		*desc = 0;

	get_some_more = 1;
	file_offset = get_unaligned_be32(&common->cmnd[2]);

	
	desc = curlun->op_desc[common->cmnd[0]-SC_VENDOR_START];
	if (!desc->buffer){
		printk("[fms_CR7]%s: cmd=%d not ready\n", __func__, common->cmnd[0]);
		curlun->sense_data =
				SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		curlun->sense_data_info = file_offset;
		curlun->info_valid = 1;
		return -EIO;		
	}

	amount_left_to_req = amount_left_to_write = common->data_size_from_cmnd;
	
	
	
	if (file_offset + amount_left_to_write > desc->len) {
		printk("[fms_CR7]%s: vendor buffer out of range offset=0x%x write-len=0x%x buf-len=0x%x\n",
			__func__, (unsigned int)file_offset, amount_left_to_req, desc->len);
		curlun->sense_data =
				SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		curlun->sense_data_info = file_offset;
		curlun->info_valid = 1;
		return -EIO;		
	}

	while (amount_left_to_write > 0) {

		
		bh = common->next_buffhd_to_fill;
		if (bh->state == BUF_STATE_EMPTY && get_some_more) {

			amount = min(amount_left_to_req, FSG_BUFLEN);
			

			
			common->usb_amount_left -= amount;
			amount_left_to_req -= amount;
			if (amount_left_to_req == 0)
				get_some_more = 0;

			
			
			

			bh->outreq->length = bh->bulk_out_intended_length =
					amount;
#if 0
			START_TRANSFER_OR(common, bulk_out, bh->outreq,
					&bh->outreq_busy, &bh->state)
#else
			if (!start_out_transfer(common, bh))
				
#endif
				return -EIO;
			common->next_buffhd_to_fill = bh->next;
			continue;
		}

		
		bh = common->next_buffhd_to_drain;
		if (bh->state == BUF_STATE_EMPTY && !get_some_more){
			break;			
		}
		if (bh->state == BUF_STATE_FULL) {
			smp_rmb();
			common->next_buffhd_to_drain = bh->next;
			bh->state = BUF_STATE_EMPTY;

			
			if (bh->outreq->status != 0) {
				curlun->sense_data = SS_COMMUNICATION_FAILURE;
				curlun->sense_data_info = file_offset >> 9;
				curlun->info_valid = 1;
				break;
			}

			amount = bh->outreq->actual;
			if (desc->len - file_offset < amount) {
				LERROR(curlun,
	"write %u @ %llu beyond end %llu\n",
	amount, (unsigned long long) file_offset,
	(unsigned long long) desc->len);
				amount = desc->len - file_offset;
			}

			
			
			memcpy(desc->buffer + file_offset, (char __user *) bh->buf,amount);
			file_offset += amount;
			amount_left_to_write -= amount;
			common->residue -= amount;

#ifdef MAX_UNFLUSHED_BYTES
			curlun->unflushed_bytes += amount;
			if (curlun->unflushed_bytes >= MAX_UNFLUSHED_BYTES) {
				fsync_sub(curlun);
				curlun->unflushed_bytes = 0;
			}
#endif
			
			if (bh->outreq->actual != bh->outreq->length) {
				common->short_packet_received = 1;
				break;
			}
			continue;
		}

		
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}

	return -EIO;		
}
#endif

static int do_synchronize_cache(struct fsg_common *common)
{
	struct fsg_lun	*curlun = common->curlun;
	int		rc;

	rc = fsg_lun_fsync_sub(curlun);
	if (rc)
		curlun->sense_data = SS_WRITE_ERROR;
	return 0;
}



static void invalidate_sub(struct fsg_lun *curlun)
{
	struct file	*filp = curlun->filp;
	struct inode	*inode = filp->f_path.dentry->d_inode;
	unsigned long	rc;

	rc = invalidate_mapping_pages(inode->i_mapping, 0, -1);
	VLDBG(curlun, "invalidate_mapping_pages -> %ld\n", rc);
}

static int do_verify(struct fsg_common *common)
{
	struct fsg_lun		*curlun = common->curlun;
	u32			lba;
	u32			verification_length;
	struct fsg_buffhd	*bh = common->next_buffhd_to_fill;
	loff_t			file_offset, file_offset_tmp;
	u32			amount_left;
	unsigned int		amount;
	ssize_t			nread;

	lba = get_unaligned_be32(&common->cmnd[2]);
	if (lba >= curlun->num_sectors) {
		curlun->sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		return -EINVAL;
	}

	if (common->cmnd[1] & ~0x10) {
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	verification_length = get_unaligned_be16(&common->cmnd[7]);
	if (unlikely(verification_length == 0))
		return -EIO;		

	
	amount_left = verification_length << curlun->blkbits;
	file_offset = ((loff_t) lba) << curlun->blkbits;

	
	fsg_lun_fsync_sub(curlun);
	if (signal_pending(current))
		return -EINTR;

	invalidate_sub(curlun);
	if (signal_pending(current))
		return -EINTR;

	
	while (amount_left > 0) {
		amount = min(amount_left, FSG_BUFLEN);
		amount = min((loff_t)amount,
			     curlun->file_length - file_offset);
		if (amount == 0) {
			curlun->sense_data =
					SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
			curlun->sense_data_info =
				file_offset >> curlun->blkbits;
			curlun->info_valid = 1;
			break;
		}

		
		file_offset_tmp = file_offset;
		nread = vfs_read(curlun->filp,
				(char __user *) bh->buf,
				amount, &file_offset_tmp);
		VLDBG(curlun, "file read %u @ %llu -> %d\n", amount,
				(unsigned long long) file_offset,
				(int) nread);
		if (signal_pending(current))
			return -EINTR;

		if (nread < 0) {
			LDBG(curlun, "error in file verify: %d\n", (int)nread);
			nread = 0;
		} else if (nread < amount) {
			LDBG(curlun, "partial file verify: %d/%u\n",
			     (int)nread, amount);
			nread = round_down(nread, curlun->blksize);
		}
		if (nread == 0) {
			curlun->sense_data = SS_UNRECOVERED_READ_ERROR;
			curlun->sense_data_info =
				file_offset >> curlun->blkbits;
			curlun->info_valid = 1;
			break;
		}
		file_offset += nread;
		amount_left -= nread;
	}
	return 0;
}



static int do_inquiry(struct fsg_common *common, struct fsg_buffhd *bh)
{
	struct fsg_lun *curlun = common->curlun;
	u8	*buf = (u8 *) bh->buf;

	if (!curlun) {		
		common->bad_lun_okay = 1;
		memset(buf, 0, 36);
		buf[0] = 0x7f;		
		buf[4] = 31;		
		return 36;
	}

	buf[0] = curlun->cdrom ? TYPE_ROM : TYPE_DISK;
	buf[1] = curlun->removable ? 0x80 : 0;
	buf[2] = 2;		
	buf[3] = 2;		
#ifdef CONFIG_LISMO
	if ( strcmp(dev_name(&curlun->dev),"lun0") == 0 ){
		buf[4] = 31 + INQUIRY_VENDOR_SPECIFIC_SIZE;		
		buf[5] = 0;		
		buf[6] = 0;
		buf[7] = 0;
		memcpy(buf + 8, common->inquiry_string, sizeof common->inquiry_string);
		memcpy(buf + 8 + sizeof common->inquiry_string - 1,
		   curlun->inquiry_vendor, INQUIRY_VENDOR_SPECIFIC_SIZE);
		return 36 + INQUIRY_VENDOR_SPECIFIC_SIZE;
	} else {
	buf[4] = 31;		
	buf[5] = 0;		
	buf[6] = 0;
	buf[7] = 0;
	memcpy(buf + 8, common->inquiry_string, sizeof common->inquiry_string);
	return 36;
	}
#else

	buf[4] = 31;		
	buf[5] = 0;		
	buf[6] = 0;
	buf[7] = 0;
	memcpy(buf + 8, common->inquiry_string, sizeof common->inquiry_string);
	return 36;
#endif
}

static int do_request_sense(struct fsg_common *common, struct fsg_buffhd *bh)
{
	struct fsg_lun	*curlun = common->curlun;
	u8		*buf = (u8 *) bh->buf;
	u32		sd, sdinfo;
	int		valid;

#if 0
	if (curlun && curlun->unit_attention_data != SS_NO_SENSE) {
		curlun->sense_data = curlun->unit_attention_data;
		curlun->unit_attention_data = SS_NO_SENSE;
	}
#endif

	if (!curlun) {		
		common->bad_lun_okay = 1;
		sd = SS_LOGICAL_UNIT_NOT_SUPPORTED;
		sdinfo = 0;
		valid = 0;
	} else {
		sd = curlun->sense_data;
		sdinfo = curlun->sense_data_info;
		valid = curlun->info_valid << 7;
		curlun->sense_data = SS_NO_SENSE;
		curlun->sense_data_info = 0;
		curlun->info_valid = 0;
	}

	memset(buf, 0, 18);
	buf[0] = valid | 0x70;			
	buf[2] = SK(sd);
	put_unaligned_be32(sdinfo, &buf[3]);	
	buf[7] = 18 - 8;			
	buf[12] = ASC(sd);
	buf[13] = ASCQ(sd);
	return 18;
}

static int do_read_capacity(struct fsg_common *common, struct fsg_buffhd *bh)
{
	struct fsg_lun	*curlun = common->curlun;
	u32		lba = get_unaligned_be32(&common->cmnd[2]);
	int		pmi = common->cmnd[8];
	u8		*buf = (u8 *)bh->buf;

	
	if (pmi > 1 || (pmi == 0 && lba != 0)) {
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	put_unaligned_be32(curlun->num_sectors - 1, &buf[0]);
						
	put_unaligned_be32(curlun->blksize, &buf[4]);
	return 8;
}

static int do_read_header(struct fsg_common *common, struct fsg_buffhd *bh)
{
	struct fsg_lun	*curlun = common->curlun;
	int		msf = common->cmnd[1] & 0x02;
	u32		lba = get_unaligned_be32(&common->cmnd[2]);
	u8		*buf = (u8 *)bh->buf;

	if (common->cmnd[1] & ~0x02) {		
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}
	if (lba >= curlun->num_sectors) {
		curlun->sense_data = SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE;
		return -EINVAL;
	}

	memset(buf, 0, 8);
	buf[0] = 0x01;		
	store_cdrom_address(&buf[4], msf, lba);
	return 8;
}

static int do_read_toc(struct fsg_common *common, struct fsg_buffhd *bh)
{
	struct fsg_lun	*curlun = common->curlun;
	int		msf = common->cmnd[1] & 0x02;
	int		start_track = common->cmnd[6];
	int		format = (common->cmnd[9] & 0xC0) >> 6;
	u8		*buf = (u8 *)bh->buf;

	if ((common->cmnd[1] & ~0x02) != 0 ||	
			start_track > 1) {
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	if (format == 2)
		return _read_toc_raw(common, bh);

	memset(buf, 0, 20);
	buf[1] = (20-2);		
	buf[2] = 1;			
	buf[3] = 1;			
	buf[5] = 0x16;			
	buf[6] = 0x01;			
	store_cdrom_address(&buf[8], msf, 0);

	buf[13] = 0x16;			
	buf[14] = 0xAA;			
	store_cdrom_address(&buf[16], msf, curlun->num_sectors);
	return 20;
}

static int do_mode_sense(struct fsg_common *common, struct fsg_buffhd *bh)
{
	struct fsg_lun	*curlun = common->curlun;
	int		mscmnd = common->cmnd[0];
	u8		*buf = (u8 *) bh->buf;
	u8		*buf0 = buf;
	int		pc, page_code;
	int		changeable_values, all_pages;
	int		valid_page = 0;
	int		len, limit;

	if ((common->cmnd[1] & ~0x08) != 0) {	
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}
	pc = common->cmnd[2] >> 6;
	page_code = common->cmnd[2] & 0x3f;
	if (pc == 3) {
		curlun->sense_data = SS_SAVING_PARAMETERS_NOT_SUPPORTED;
		return -EINVAL;
	}
	changeable_values = (pc == 1);
	all_pages = (page_code == 0x3f);

	memset(buf, 0, 8);
	if (mscmnd == MODE_SENSE) {
		buf[2] = (curlun->ro ? 0x80 : 0x00);		
		buf += 4;
		limit = 255;
	} else {			
		buf[3] = (curlun->ro ? 0x80 : 0x00);		
		buf += 8;
		limit = 65535;		
	}

	

	if (page_code == 0x08 || all_pages) {
		valid_page = 1;
		buf[0] = 0x08;		
		buf[1] = 10;		
		memset(buf+2, 0, 10);	

		if (!changeable_values) {
			buf[2] = 0x00;	
					
					
			put_unaligned_be16(0xffff, &buf[4]);
					
					
			put_unaligned_be16(0xffff, &buf[8]);
					
			put_unaligned_be16(0xffff, &buf[10]);
					
		}
		buf += 12;
	}

	len = buf - buf0;
	if (!valid_page || len > limit) {
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	
	if (mscmnd == MODE_SENSE)
		buf0[0] = len - 1;
	else
		put_unaligned_be16(len - 2, buf0);
	return len;
}

static int do_start_stop(struct fsg_common *common)
{
	struct fsg_lun	*curlun = common->curlun;
	int		loej, start;

	if (!curlun) {
		return -EINVAL;
	} else if (!curlun->removable) {
		curlun->sense_data = SS_INVALID_COMMAND;
		return -EINVAL;
	} else if ((common->cmnd[1] & ~0x01) != 0 || 
		   (common->cmnd[4] & ~0x03) != 0) { 
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}

	loej  = common->cmnd[4] & 0x02;
	start = common->cmnd[4] & 0x01;

	if (start) {
		if (!fsg_lun_is_open(curlun)) {
			curlun->sense_data = SS_MEDIUM_NOT_PRESENT;
			return -EINVAL;
		}
		return 0;
	}

	
	if (curlun->prevent_medium_removal) {
		LDBG(curlun, "unload attempt prevented\n");
		curlun->sense_data = SS_MEDIUM_REMOVAL_PREVENTED;
		return -EINVAL;
	}

	if (!loej)
		return 0;

	
	if (common->ops && common->ops->pre_eject) {
		int r = common->ops->pre_eject(common, curlun,
					       curlun - common->luns);
		if (unlikely(r < 0))
			return r;
		else if (r)
			return 0;
	}

	up_read(&common->filesem);
	down_write(&common->filesem);
	fsg_lun_close(curlun);
	up_write(&common->filesem);
	down_read(&common->filesem);

	return common->ops && common->ops->post_eject
		? min(0, common->ops->post_eject(common, curlun,
						 curlun - common->luns))
		: 0;
}

static int do_prevent_allow(struct fsg_common *common)
{
	struct fsg_lun	*curlun = common->curlun;
	int		prevent;

	if (!common->curlun) {
		return -EINVAL;
	} else if (!common->curlun->removable) {
		common->curlun->sense_data = SS_INVALID_COMMAND;
		return -EINVAL;
	}

	prevent = common->cmnd[4] & 0x01;
	if ((common->cmnd[4] & ~0x01) != 0) {	
		curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
		return -EINVAL;
	}
	if (!curlun->nofua && curlun->prevent_medium_removal && !prevent)
		fsg_lun_fsync_sub(curlun);
	curlun->prevent_medium_removal = prevent;
	return 0;
}

static int do_read_format_capacities(struct fsg_common *common,
			struct fsg_buffhd *bh)
{
	struct fsg_lun	*curlun = common->curlun;
	u8		*buf = (u8 *) bh->buf;

	buf[0] = buf[1] = buf[2] = 0;
	buf[3] = 8;	
	buf += 4;

	put_unaligned_be32(curlun->num_sectors, &buf[0]);
						
	put_unaligned_be32(curlun->blksize, &buf[4]);
	buf[4] = 0x02;				
	return 12;
}

static int do_mode_select(struct fsg_common *common, struct fsg_buffhd *bh)
{
	struct fsg_lun	*curlun = common->curlun;

	
	if (curlun)
		curlun->sense_data = SS_INVALID_COMMAND;
	return -EINVAL;
}

struct work_struct	ums_do_reserve_work;
static char usb_function_ebl;
static void handle_reserve_cmd(struct work_struct *work)
{
	htc_usb_enable_function("adb", usb_function_ebl);
}

static int do_reserve(struct fsg_common *common, struct fsg_buffhd *bh)
{
	int	call_us_ret = -1;
	char *envp[] = {
		"HOME=/",
		"PATH=/sbin:/system/sbin:/system/bin:/system/xbin",
		NULL,
	};
	char *exec_path[2] = {"/system/bin/stop", "/system/bin/start" };
	char *argv_stop[] = { exec_path[0], "adbd", NULL, };
	char *argv_start[] = { exec_path[1], "adbd", NULL, };

	if (common->cmnd[1] == ('h'&0x1f) && common->cmnd[2] == 't'
		&& common->cmnd[3] == 'c') {
		
		switch (common->cmnd[5]) {
		case 0x01: 
			call_us_ret = call_usermodehelper(exec_path[1],
				argv_start, envp, UMH_WAIT_PROC);
			usb_function_ebl = 1;
			schedule_work(&ums_do_reserve_work);
		break;
		case 0x02: 
			call_us_ret = call_usermodehelper(exec_path[0],
				argv_stop, envp, UMH_WAIT_PROC);
			usb_function_ebl = 0;
			schedule_work(&ums_do_reserve_work);
		break;
		default:
			printk(KERN_DEBUG "Unknown hTC specific command..."
					"(0x%2.2X)\n", common->cmnd[5]);
		break;
		}
	}
	printk(KERN_NOTICE "%s adb daemon from mass_storage %s(%d)\n",
		(common->cmnd[5] == 0x01) ? "Enable" :
		(common->cmnd[5] == 0x02) ? "Disable" : "Unknown",
		(call_us_ret == 0) ? "DONE" : "FAIL", call_us_ret);
	return 0;
}


static int halt_bulk_in_endpoint(struct fsg_dev *fsg)
{
	int	rc;

	rc = fsg_set_halt(fsg, fsg->bulk_in);
	if (rc == -EAGAIN)
		VDBG(fsg, "delayed bulk-in endpoint halt\n");
	while (rc != 0) {
		if (rc != -EAGAIN) {
			WARNING(fsg, "usb_ep_set_halt -> %d\n", rc);
			rc = 0;
			break;
		}

		
		if (msleep_interruptible(100) != 0)
			return -EINTR;
		rc = usb_ep_set_halt(fsg->bulk_in);
	}
	return rc;
}

static int wedge_bulk_in_endpoint(struct fsg_dev *fsg)
{
	int	rc;

	DBG(fsg, "bulk-in set wedge\n");
	rc = usb_ep_set_wedge(fsg->bulk_in);
	if (rc == -EAGAIN)
		VDBG(fsg, "delayed bulk-in endpoint wedge\n");
	while (rc != 0) {
		if (rc != -EAGAIN) {
			WARNING(fsg, "usb_ep_set_wedge -> %d\n", rc);
			rc = 0;
			break;
		}

		
		if (msleep_interruptible(100) != 0)
			return -EINTR;
		rc = usb_ep_set_wedge(fsg->bulk_in);
	}
	return rc;
}

static int throw_away_data(struct fsg_common *common)
{
	struct fsg_buffhd	*bh;
	u32			amount;
	int			rc;

	for (bh = common->next_buffhd_to_drain;
	     bh->state != BUF_STATE_EMPTY || common->usb_amount_left > 0;
	     bh = common->next_buffhd_to_drain) {

		
		if (bh->state == BUF_STATE_FULL) {
			smp_rmb();
			bh->state = BUF_STATE_EMPTY;
			common->next_buffhd_to_drain = bh->next;

			
			if (bh->outreq->actual < bh->bulk_out_intended_length ||
			    bh->outreq->status != 0) {
				raise_exception(common,
						FSG_STATE_ABORT_BULK_OUT);
				return -EINTR;
			}
			continue;
		}

		
		bh = common->next_buffhd_to_fill;
		if (bh->state == BUF_STATE_EMPTY
		 && common->usb_amount_left > 0) {
			amount = min(common->usb_amount_left, FSG_BUFLEN);

			set_bulk_out_req_length(common, bh, amount);
			if (!start_out_transfer(common, bh))
				
				return -EIO;
			common->next_buffhd_to_fill = bh->next;
			common->usb_amount_left -= amount;
			continue;
		}

		
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}
	return 0;
}

static int finish_reply(struct fsg_common *common)
{
	struct fsg_buffhd	*bh = common->next_buffhd_to_fill;
	int			rc = 0;

	switch (common->data_dir) {
	case DATA_DIR_NONE:
		break;			

	case DATA_DIR_UNKNOWN:
		if (!common->can_stall) {
			
		} else if (fsg_is_set(common)) {
			fsg_set_halt(common->fsg, common->fsg->bulk_out);
			rc = halt_bulk_in_endpoint(common->fsg);
		} else {
			
			rc = -EIO;
		}
		break;

	
	case DATA_DIR_TO_HOST:
		if (common->data_size == 0) {
			

		
		} else if (!fsg_is_set(common)) {
			rc = -EIO;

		
		} else if (common->residue == 0) {
			bh->inreq->zero = 0;
			if (!start_in_transfer(common, bh))
				return -EIO;
			common->next_buffhd_to_fill = bh->next;

		} else {
			bh->inreq->zero = 1;
			if (!start_in_transfer(common, bh))
				rc = -EIO;
			common->next_buffhd_to_fill = bh->next;
			if (common->can_stall)
				rc = halt_bulk_in_endpoint(common->fsg);
		}
		break;

	case DATA_DIR_FROM_HOST:
		if (common->residue == 0) {
			

		
		} else if (common->short_packet_received) {
			raise_exception(common, FSG_STATE_ABORT_BULK_OUT);
			rc = -EINTR;

#if 0
		} else if (common->can_stall) {
			if (fsg_is_set(common))
				fsg_set_halt(common->fsg,
					     common->fsg->bulk_out);
			raise_exception(common, FSG_STATE_ABORT_BULK_OUT);
			rc = -EINTR;
#endif

		} else {
			rc = throw_away_data(common);
		}
		break;
	}
	return rc;
}

static int send_status(struct fsg_common *common)
{
	struct fsg_lun		*curlun = common->curlun;
	struct fsg_buffhd	*bh;
	struct bulk_cs_wrap	*csw;
	int			rc;
	u8			status = US_BULK_STAT_OK;
	u32			sd, sdinfo = 0;

	
	bh = common->next_buffhd_to_fill;
	while (bh->state != BUF_STATE_EMPTY) {
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}

	if (curlun) {
		sd = curlun->sense_data;
		sdinfo = curlun->sense_data_info;
	} else if (common->bad_lun_okay)
		sd = SS_NO_SENSE;
	else
		sd = SS_LOGICAL_UNIT_NOT_SUPPORTED;

	if (common->phase_error) {
		DBG(common, "sending phase-error status\n");
		status = US_BULK_STAT_PHASE;
		sd = SS_INVALID_COMMAND;
	} else if (sd != SS_NO_SENSE) {
		DBG(common, "sending command-failure status\n");
		status = US_BULK_STAT_FAIL;
		VDBG(common, "  sense data: SK x%02x, ASC x%02x, ASCQ x%02x;"
				"  info x%x\n",
				SK(sd), ASC(sd), ASCQ(sd), sdinfo);
	}

	
	csw = (void *)bh->buf;

	csw->Signature = cpu_to_le32(US_BULK_CS_SIGN);
	csw->Tag = common->tag;
	csw->Residue = cpu_to_le32(common->residue);
#ifdef CONFIG_USB_CSW_HACK
	if (write_error_after_csw_sent) {
		write_error_after_csw_sent = 0;
		csw->Residue = cpu_to_le32(common->residue);
	} else
		csw->Residue = 0;
#else
	csw->Residue = cpu_to_le32(common->residue);
#endif
	csw->Status = status;

	bh->inreq->length = US_BULK_CS_WRAP_LEN;
	bh->inreq->zero = 0;
	if (!start_in_transfer(common, bh))
		
		return -EIO;

	common->next_buffhd_to_fill = bh->next;
	return 0;
}



static int check_command(struct fsg_common *common, int cmnd_size,
			 enum data_direction data_dir, unsigned int mask,
			 int needs_medium, const char *name)
{
	int			i;
	int			lun = common->cmnd[1] >> 5;
	static const char	dirletter[4] = {'u', 'o', 'i', 'n'};
	char			hdlen[20];
	struct fsg_lun		*curlun;

	hdlen[0] = 0;
	if (common->data_dir != DATA_DIR_UNKNOWN)
		sprintf(hdlen, ", H%c=%u", dirletter[(int) common->data_dir],
			common->data_size);
	VDBG(common, "SCSI command: %s;  Dc=%d, D%c=%u;  Hc=%d%s\n",
	     name, cmnd_size, dirletter[(int) data_dir],
	     common->data_size_from_cmnd, common->cmnd_size, hdlen);

	if (common->data_size_from_cmnd == 0)
		data_dir = DATA_DIR_NONE;
	if (common->data_size < common->data_size_from_cmnd) {
		common->data_size_from_cmnd = common->data_size;
		common->phase_error = 1;
	}
	common->residue = common->data_size;
	common->usb_amount_left = common->data_size;

	
	if (common->data_dir != data_dir && common->data_size_from_cmnd > 0) {
		common->phase_error = 1;
		return -EINVAL;
	}

	
	if (cmnd_size != common->cmnd_size) {

		if (cmnd_size <= common->cmnd_size) {
			DBG(common, "%s is buggy! Expected length %d "
			    "but we got %d\n", name,
			    cmnd_size, common->cmnd_size);
			cmnd_size = common->cmnd_size;
		} else if (common->cmnd[0] == RESERVE) {
			cmnd_size = common->cmnd_size;
		} else {
			common->phase_error = 1;
			return -EINVAL;
		}
	}

	
	if (common->lun != lun)
		DBG(common, "using LUN %d from CBW, not LUN %d from CDB\n",
		    common->lun, lun);

	
	curlun = common->curlun;
	if (curlun) {
		if (common->cmnd[0] != REQUEST_SENSE) {
			curlun->sense_data = SS_NO_SENSE;
			curlun->sense_data_info = 0;
			curlun->info_valid = 0;
		}
	} else {
		common->bad_lun_okay = 0;

		if (common->cmnd[0] != INQUIRY &&
		    common->cmnd[0] != REQUEST_SENSE) {
			DBG(common, "unsupported LUN %d\n", common->lun);
			return -EINVAL;
		}
	}

	if (curlun && curlun->unit_attention_data != SS_NO_SENSE &&
	    common->cmnd[0] != INQUIRY &&
	    common->cmnd[0] != REQUEST_SENSE) {
		curlun->sense_data = curlun->unit_attention_data;
		curlun->unit_attention_data = SS_NO_SENSE;
		return -EINVAL;
	}

	
	common->cmnd[1] &= 0x1f;			
	for (i = 1; i < cmnd_size; ++i) {
		if (common->cmnd[i] && !(mask & (1 << i))) {
			if (curlun)
				curlun->sense_data = SS_INVALID_FIELD_IN_CDB;
			return -EINVAL;
		}
	}

	if (curlun && !fsg_lun_is_open(curlun) && needs_medium) {
		curlun->sense_data = SS_MEDIUM_NOT_PRESENT;
		return -EINVAL;
	}

	return 0;
}

static int check_command_size_in_blocks(struct fsg_common *common,
		int cmnd_size, enum data_direction data_dir,
		unsigned int mask, int needs_medium, const char *name)
{
	if (common->curlun)
		common->data_size_from_cmnd <<= common->curlun->blkbits;
	return check_command(common, cmnd_size, data_dir,
			mask, needs_medium, name);
}

static int do_scsi_command(struct fsg_common *common)
{
	struct fsg_buffhd	*bh;
	int			rc;
	int			reply = -EINVAL;
	int			i;
	static char		unknown[16];
#ifdef CONFIG_LISMO
	struct op_desc	*desc;
#endif
	dump_cdb(common);

	
	bh = common->next_buffhd_to_fill;
	common->next_buffhd_to_drain = bh;
	while (bh->state != BUF_STATE_EMPTY) {
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}
	common->phase_error = 0;
	common->short_packet_received = 0;

	down_read(&common->filesem);	
	switch (common->cmnd[0]) {

	case INQUIRY:
		common->data_size_from_cmnd = common->cmnd[4];
		reply = check_command(common, 6, DATA_DIR_TO_HOST,
				      (1<<4), 0,
				      "INQUIRY");
		if (reply == 0)
			reply = do_inquiry(common, bh);
		break;

	case MODE_SELECT:
		common->data_size_from_cmnd = common->cmnd[4];
		reply = check_command(common, 6, DATA_DIR_FROM_HOST,
				      (1<<1) | (1<<4), 0,
				      "MODE SELECT(6)");
		if (reply == 0)
			reply = do_mode_select(common, bh);
		break;

	case MODE_SELECT_10:
		common->data_size_from_cmnd =
			get_unaligned_be16(&common->cmnd[7]);
		reply = check_command(common, 10, DATA_DIR_FROM_HOST,
				      (1<<1) | (3<<7), 0,
				      "MODE SELECT(10)");
		if (reply == 0)
			reply = do_mode_select(common, bh);
		break;

	case MODE_SENSE:
		common->data_size_from_cmnd = common->cmnd[4];
		reply = check_command(common, 6, DATA_DIR_TO_HOST,
				      (1<<1) | (1<<2) | (1<<4), 0,
				      "MODE SENSE(6)");
		if (reply == 0)
			reply = do_mode_sense(common, bh);
		break;

	case MODE_SENSE_10:
		common->data_size_from_cmnd =
			get_unaligned_be16(&common->cmnd[7]);
		reply = check_command(common, 10, DATA_DIR_TO_HOST,
				      (1<<1) | (1<<2) | (3<<7), 0,
				      "MODE SENSE(10)");
		if (reply == 0)
			reply = do_mode_sense(common, bh);
		break;

	case ALLOW_MEDIUM_REMOVAL:
		common->data_size_from_cmnd = 0;
		reply = check_command(common, 6, DATA_DIR_NONE,
				      (1<<4), 0,
				      "PREVENT-ALLOW MEDIUM REMOVAL");
		if (reply == 0)
			reply = do_prevent_allow(common);
		break;

	case READ_6:
		i = common->cmnd[4];
		common->data_size_from_cmnd = (i == 0) ? 256 : i;
		reply = check_command_size_in_blocks(common, 6,
				      DATA_DIR_TO_HOST,
				      (7<<1) | (1<<4), 1,
				      "READ(6)");
		if (reply == 0)
			reply = do_read(common);
		break;

	case READ_10:
		common->data_size_from_cmnd =
				get_unaligned_be16(&common->cmnd[7]);
		reply = check_command_size_in_blocks(common, 10,
				      DATA_DIR_TO_HOST,
				      (1<<1) | (0xf<<2) | (3<<7), 1,
				      "READ(10)");
		if (reply == 0)
			reply = do_read(common);
		break;

	case READ_12:
		common->data_size_from_cmnd =
				get_unaligned_be32(&common->cmnd[6]);
		reply = check_command_size_in_blocks(common, 12,
				      DATA_DIR_TO_HOST,
				      (1<<1) | (0xf<<2) | (0xf<<6), 1,
				      "READ(12)");
		if (reply == 0)
			reply = do_read(common);
		break;

	case READ_CD:
		common->data_size_from_cmnd = ((common->cmnd[6] << 16) |
			    (common->cmnd[7] << 8) | (common->cmnd[8])) << 9;
		reply = check_command(common, 12, DATA_DIR_TO_HOST,
			    (0xf<<2) | (7<<7), 1, "READ CD");

		if (reply == 0)
			reply = do_read(common);
		break;

	case READ_CAPACITY:
		common->data_size_from_cmnd = 8;
		reply = check_command(common, 10, DATA_DIR_TO_HOST,
				      (0xf<<2) | (1<<8), 1,
				      "READ CAPACITY");
		if (reply == 0)
			reply = do_read_capacity(common, bh);
		break;

	case READ_HEADER:
		if (!common->curlun || !common->curlun->cdrom)
			goto unknown_cmnd;
		common->data_size_from_cmnd =
			get_unaligned_be16(&common->cmnd[7]);
		reply = check_command(common, 10, DATA_DIR_TO_HOST,
				      (3<<7) | (0x1f<<1), 1,
				      "READ HEADER");
		if (reply == 0)
			reply = do_read_header(common, bh);
		break;

	case READ_TOC:
		if (!common->curlun || !common->curlun->cdrom)
			goto unknown_cmnd;
		common->data_size_from_cmnd =
			get_unaligned_be16(&common->cmnd[7]);
		reply = check_command(common, 10, DATA_DIR_TO_HOST,
				      (0xf<<6) | (1<<1), 1,
				      "READ TOC");
		if (reply == 0)
			reply = do_read_toc(common, bh);
		break;

	case READ_FORMAT_CAPACITIES:
		common->data_size_from_cmnd =
			get_unaligned_be16(&common->cmnd[7]);
		reply = check_command(common, 10, DATA_DIR_TO_HOST,
				      (3<<7), 1,
				      "READ FORMAT CAPACITIES");
		if (reply == 0)
			reply = do_read_format_capacities(common, bh);
		break;

	case REQUEST_SENSE:
		common->data_size_from_cmnd = common->cmnd[4];
		reply = check_command(common, 6, DATA_DIR_TO_HOST,
				      (1<<4), 0,
				      "REQUEST SENSE");
		if (reply == 0)
			reply = do_request_sense(common, bh);
		break;

	case START_STOP:
		common->data_size_from_cmnd = 0;
		reply = check_command(common, 6, DATA_DIR_NONE,
				      (1<<1) | (1<<4), 0,
				      "START-STOP UNIT");
		if (reply == 0)
			reply = do_start_stop(common);
		break;

	case SYNCHRONIZE_CACHE:
		common->data_size_from_cmnd = 0;
		reply = check_command(common, 10, DATA_DIR_NONE,
				      (0xf<<2) | (3<<7), 1,
				      "SYNCHRONIZE CACHE");
		if (reply == 0)
			reply = do_synchronize_cache(common);
		break;

	case TEST_UNIT_READY:
		common->data_size_from_cmnd = 0;
		reply = check_command(common, 6, DATA_DIR_NONE,
				0, 1,
				"TEST UNIT READY");
		break;
#ifdef CONFIG_PASCAL_DETECT
	case SC_PASCAL_MODE:
		printk(KERN_INFO "SC_PASCAL_MODE\n");
		if (!strncmp("RDEVCHG=PASCAL", (char *)&common->cmnd[1], 14)) {
			printk(KERN_INFO "usb: switch to CDC ACM\n");
			android_switch_function(0x400);
			switch_set_state(&kddi_switch, 1);
			atomic_set(&pascal_enable, 1);
		}
		break;
#endif
#ifdef CONFIG_LISMO
	case SC_VENDOR_START ... SC_VENDOR_END:
		mutex_lock(&sysfs_lock);

		if (common->lun != 0){
			printk("[fms_CR7]%s e4 command receive but not[lun0]! \n", __func__);
			goto cmd_error;
		}
		desc = common->luns[common->lun].op_desc[common->cmnd[0] - SC_VENDOR_START];
		if (!desc){
			printk("[fms_CR7]%s  opcode-%02x not ready! \n", __func__,common->cmnd[0]);
			goto cmd_error;
		}

		common->data_size_from_cmnd = get_unaligned_be32(&common->cmnd[6]);
		if (common->data_size_from_cmnd == 0)
				goto cmd_error;
		if (~common->cmnd[1] & 0x10) {
			if ((reply = check_command(common, 10, DATA_DIR_FROM_HOST,
					(1<<1) | (0xf<<2) | (0xf<<6),
					0, "VENDOR WRITE BUFFER")) == 0) {
				reply = do_write_buffer(common);
				desc->update = jiffies;
				schedule_work(&desc->work);
			} else
				goto cmd_error;
				mutex_unlock(&sysfs_lock);
			break;
		} else {
			if ((reply = check_command(common, 10, DATA_DIR_TO_HOST,
					(1<<1) | (0xf<<2) | (0xf<<6),
					0, "VENDOR READ BUFFER")) == 0)
				reply = do_read_buffer(common);
			else
				goto cmd_error;
				mutex_unlock(&sysfs_lock);
			break;
		}
		cmd_error:
			mutex_unlock(&sysfs_lock);
			common->data_size_from_cmnd = 0;
			sprintf(unknown, "Unknown x%02x", common->cmnd[0]);
			if ((reply = check_command(common, common->cmnd_size,
					DATA_DIR_UNKNOWN, 0x3ff, 0, unknown)) == 0) { 
				common->curlun->sense_data = SS_INVALID_COMMAND;
				reply = -EINVAL;
			}
		break;
#endif
	case VERIFY:
		common->data_size_from_cmnd = 0;
		reply = check_command(common, 10, DATA_DIR_NONE,
				      (1<<1) | (0xf<<2) | (3<<7), 1,
				      "VERIFY");
		if (reply == 0)
			reply = do_verify(common);
		break;

	case WRITE_6:
		i = common->cmnd[4];
		common->data_size_from_cmnd = (i == 0) ? 256 : i;
		reply = check_command_size_in_blocks(common, 6,
				      DATA_DIR_FROM_HOST,
				      (7<<1) | (1<<4), 1,
				      "WRITE(6)");
		if (reply == 0)
			reply = do_write(common);
		break;

	case WRITE_10:
		common->data_size_from_cmnd =
				get_unaligned_be16(&common->cmnd[7]);
		reply = check_command_size_in_blocks(common, 10,
				      DATA_DIR_FROM_HOST,
				      (1<<1) | (0xf<<2) | (3<<7), 1,
				      "WRITE(10)");
		if (reply == 0)
			reply = do_write(common);
		break;

	case WRITE_12:
		common->data_size_from_cmnd =
				get_unaligned_be32(&common->cmnd[6]);
		reply = check_command_size_in_blocks(common, 12,
				      DATA_DIR_FROM_HOST,
				      (1<<1) | (0xf<<2) | (0xf<<6), 1,
				      "WRITE(12)");
		if (reply == 0)
			reply = do_write(common);
		break;

	case RESERVE:
		common->data_size_from_cmnd = common->cmnd[4];
		reply = check_command(common, 10, DATA_DIR_TO_HOST,
				(1<<1) | (0xf<<2) , 0,
				"RESERVE(6)");
		if (reply == 0)
			reply = do_reserve(common, bh);
		break;
	case FORMAT_UNIT:
	case RELEASE:
	case SEND_DIAGNOSTIC:
		

	default:
unknown_cmnd:
		common->data_size_from_cmnd = 0;
		sprintf(unknown, "Unknown x%02x", common->cmnd[0]);
		reply = check_command(common, common->cmnd_size,
				      DATA_DIR_UNKNOWN, ~0, 0, unknown);
		if (reply == 0) {
			common->curlun->sense_data = SS_INVALID_COMMAND;
			reply = -EINVAL;
		}
		break;
	}
	up_read(&common->filesem);

	if (reply == -EINTR || signal_pending(current))
		return -EINTR;

	
	if (reply == -EINVAL)
		reply = 0;		
	if (reply >= 0 && common->data_dir == DATA_DIR_TO_HOST) {
		reply = min((u32)reply, common->data_size_from_cmnd);
		bh->inreq->length = reply;
		bh->state = BUF_STATE_FULL;
		common->residue -= reply;
	}				

	return 0;
}



static int received_cbw(struct fsg_dev *fsg, struct fsg_buffhd *bh)
{
	struct usb_request	*req = bh->outreq;
	struct bulk_cb_wrap	*cbw = req->buf;
	struct fsg_common	*common = fsg->common;

	
	if (req->status || test_bit(IGNORE_BULK_OUT, &fsg->atomic_bitflags))
		return -EINVAL;

	
	if (req->actual != US_BULK_CB_WRAP_LEN ||
			cbw->Signature != cpu_to_le32(
				US_BULK_CB_SIGN)) {
		DBG(fsg, "invalid CBW: len %u sig 0x%x\n",
				req->actual,
				le32_to_cpu(cbw->Signature));

		wedge_bulk_in_endpoint(fsg);
		set_bit(IGNORE_BULK_OUT, &fsg->atomic_bitflags);
		return -EINVAL;
	}

	
	if (cbw->Lun >= FSG_MAX_LUNS || cbw->Flags & ~US_BULK_FLAG_IN ||
			cbw->Length <= 0 || cbw->Length > MAX_COMMAND_SIZE) {
		DBG(fsg, "non-meaningful CBW: lun = %u, flags = 0x%x, "
				"cmdlen %u\n",
				cbw->Lun, cbw->Flags, cbw->Length);

		if (common->can_stall) {
			fsg_set_halt(fsg, fsg->bulk_out);
			halt_bulk_in_endpoint(fsg);
		}
		return -EINVAL;
	}

	
	common->cmnd_size = cbw->Length;
	memcpy(common->cmnd, cbw->CDB, common->cmnd_size);
	if (cbw->Flags & US_BULK_FLAG_IN)
		common->data_dir = DATA_DIR_TO_HOST;
	else
		common->data_dir = DATA_DIR_FROM_HOST;
	common->data_size = le32_to_cpu(cbw->DataTransferLength);
	if (common->data_size == 0)
		common->data_dir = DATA_DIR_NONE;
	common->lun = cbw->Lun;
	if (common->lun >= 0 && common->lun < common->nluns)
		common->curlun = &common->luns[common->lun];
	else
		common->curlun = NULL;
	common->tag = cbw->Tag;
	return 0;
}

static int get_next_command(struct fsg_common *common)
{
	struct fsg_buffhd	*bh;
	int			rc = 0;

	
	bh = common->next_buffhd_to_fill;
	while (bh->state != BUF_STATE_EMPTY) {
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}

	
	set_bulk_out_req_length(common, bh, US_BULK_CB_WRAP_LEN);
	if (!start_out_transfer(common, bh))
		
		return -EIO;


	
	while (bh->state != BUF_STATE_FULL) {
		rc = sleep_thread(common);
		if (rc)
			return rc;
	}
	smp_rmb();
	rc = fsg_is_set(common) ? received_cbw(common->fsg, bh) : -EIO;
	bh->state = BUF_STATE_EMPTY;

	return rc;
}



static int alloc_request(struct fsg_common *common, struct usb_ep *ep,
		struct usb_request **preq)
{
	*preq = usb_ep_alloc_request(ep, GFP_ATOMIC);
	if (*preq)
		return 0;
	ERROR(common, "can't allocate request for %s\n", ep->name);
	return -ENOMEM;
}

static int do_set_interface(struct fsg_common *common, struct fsg_dev *new_fsg)
{
	struct fsg_dev *fsg;
	int i, rc = 0;

	if (common->running)
		DBG(common, "reset interface\n");

reset:
	
	if (common->fsg) {
		fsg = common->fsg;

		for (i = 0; i < fsg_num_buffers; ++i) {
			struct fsg_buffhd *bh = &common->buffhds[i];

			if (bh->inreq) {
				usb_ep_free_request(fsg->bulk_in, bh->inreq);
				bh->inreq = NULL;
			}
			if (bh->outreq) {
				usb_ep_free_request(fsg->bulk_out, bh->outreq);
				bh->outreq = NULL;
			}
		}


		common->fsg = NULL;
		wake_up(&common->fsg_wait);
	}

	common->running = 0;
	if (!new_fsg || rc)
		return rc;

	common->fsg = new_fsg;
	fsg = common->fsg;

	
	for (i = 0; i < fsg_num_buffers; ++i) {
		struct fsg_buffhd	*bh = &common->buffhds[i];

		rc = alloc_request(common, fsg->bulk_in, &bh->inreq);
		if (rc)
			goto reset;
		rc = alloc_request(common, fsg->bulk_out, &bh->outreq);
		if (rc)
			goto reset;
		bh->inreq->buf = bh->outreq->buf = bh->buf;
		bh->inreq->context = bh->outreq->context = bh;
		bh->inreq->complete = bulk_in_complete;
		bh->outreq->complete = bulk_out_complete;
	}

	common->running = 1;
	for (i = 0; i < common->nluns; ++i)
		common->luns[i].unit_attention_data = SS_RESET_OCCURRED;
	return rc;
}



static int fsg_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct fsg_dev *fsg = fsg_from_func(f);
	struct fsg_common *common = fsg->common;
	int rc;

	
	rc = config_ep_by_speed(common->gadget, &(fsg->function), fsg->bulk_in);
	if (rc)
		return rc;
	rc = usb_ep_enable(fsg->bulk_in);
	if (rc)
		return rc;
	fsg->bulk_in->driver_data = common;
	fsg->bulk_in_enabled = 1;

	rc = config_ep_by_speed(common->gadget, &(fsg->function),
				fsg->bulk_out);
	if (rc)
		goto reset_bulk_int;
	rc = usb_ep_enable(fsg->bulk_out);
	if (rc)
		goto reset_bulk_int;
	fsg->bulk_out->driver_data = common;
	fsg->bulk_out_enabled = 1;
	common->bulk_out_maxpacket = le16_to_cpu(fsg->bulk_in->desc->wMaxPacketSize);
	clear_bit(IGNORE_BULK_OUT, &fsg->atomic_bitflags);
	fsg->common->new_fsg = fsg;
	raise_exception(fsg->common, FSG_STATE_CONFIG_CHANGE);
	return USB_GADGET_DELAYED_STATUS;

reset_bulk_int:
	usb_ep_disable(fsg->bulk_in);
	fsg->bulk_in_enabled = 0;
	return rc;
}

static void fsg_disable(struct usb_function *f)
{
	struct fsg_dev *fsg = fsg_from_func(f);

	
	if (fsg->bulk_in_enabled) {
		usb_ep_disable(fsg->bulk_in);
		fsg->bulk_in_enabled = 0;
		fsg->bulk_in->driver_data = NULL;
	}
	if (fsg->bulk_out_enabled) {
		usb_ep_disable(fsg->bulk_out);
		fsg->bulk_out_enabled = 0;
		fsg->bulk_out->driver_data = NULL;
	}
	fsg->common->new_fsg = NULL;
	raise_exception(fsg->common, FSG_STATE_CONFIG_CHANGE);
}



static void handle_exception(struct fsg_common *common)
{
	siginfo_t		info;
	int			i;
	struct fsg_buffhd	*bh;
	enum fsg_state		old_state;
	struct fsg_lun		*curlun;
	unsigned int		exception_req_tag;

	for (;;) {
		int sig =
			dequeue_signal_lock(current, &current->blocked, &info);
		if (!sig)
			break;
		if (sig != SIGUSR1) {
			if (common->state < FSG_STATE_EXIT)
				DBG(common, "Main thread exiting on signal\n");
			raise_exception(common, FSG_STATE_EXIT);
		}
	}

	
	if (likely(common->fsg)) {
		for (i = 0; i < fsg_num_buffers; ++i) {
			bh = &common->buffhds[i];
			if (bh->inreq_busy)
				usb_ep_dequeue(common->fsg->bulk_in, bh->inreq);
			if (bh->outreq_busy)
				usb_ep_dequeue(common->fsg->bulk_out,
					       bh->outreq);
		}

		
		for (;;) {
			int num_active = 0;
			for (i = 0; i < fsg_num_buffers; ++i) {
				bh = &common->buffhds[i];
				num_active += bh->inreq_busy + bh->outreq_busy;
			}
			if (num_active == 0)
				break;
			if (sleep_thread(common))
				return;
		}

		
		if (common->fsg->bulk_in_enabled)
			usb_ep_fifo_flush(common->fsg->bulk_in);
		if (common->fsg->bulk_out_enabled)
			usb_ep_fifo_flush(common->fsg->bulk_out);
	}

	spin_lock_irq(&common->lock);

	for (i = 0; i < fsg_num_buffers; ++i) {
		bh = &common->buffhds[i];
		bh->state = BUF_STATE_EMPTY;
	}
	common->next_buffhd_to_fill = &common->buffhds[0];
	common->next_buffhd_to_drain = &common->buffhds[0];
	exception_req_tag = common->exception_req_tag;
	old_state = common->state;

	if (old_state == FSG_STATE_ABORT_BULK_OUT)
		common->state = FSG_STATE_STATUS_PHASE;
	else {
		for (i = 0; i < common->nluns; ++i) {
			curlun = &common->luns[i];
			curlun->prevent_medium_removal = 0;
			curlun->sense_data = SS_NO_SENSE;
			curlun->unit_attention_data = SS_NO_SENSE;
			curlun->sense_data_info = 0;
			curlun->info_valid = 0;
		}
		common->state = FSG_STATE_IDLE;
	}
	spin_unlock_irq(&common->lock);

	
	switch (old_state) {
	case FSG_STATE_ABORT_BULK_OUT:
		send_status(common);
		spin_lock_irq(&common->lock);
		if (common->state == FSG_STATE_STATUS_PHASE)
			common->state = FSG_STATE_IDLE;
		spin_unlock_irq(&common->lock);
		break;

	case FSG_STATE_RESET:
		if (!fsg_is_set(common))
			break;
		if (test_and_clear_bit(IGNORE_BULK_OUT,
				       &common->fsg->atomic_bitflags))
			usb_ep_clear_halt(common->fsg->bulk_in);

		if (common->ep0_req_tag == exception_req_tag)
			ep0_queue(common);	

		
		
		
		break;

	case FSG_STATE_CONFIG_CHANGE:
		do_set_interface(common, common->new_fsg);
		if (common->new_fsg)
			usb_composite_setup_continue(common->cdev);
		break;

	case FSG_STATE_EXIT:
	case FSG_STATE_TERMINATED:
		do_set_interface(common, NULL);		
		spin_lock_irq(&common->lock);
		common->state = FSG_STATE_TERMINATED;	
		spin_unlock_irq(&common->lock);
		break;

	case FSG_STATE_INTERFACE_CHANGE:
	case FSG_STATE_DISCONNECT:
	case FSG_STATE_COMMAND_PHASE:
	case FSG_STATE_DATA_PHASE:
	case FSG_STATE_STATUS_PHASE:
	case FSG_STATE_IDLE:
		break;
	}
}



static int fsg_main_thread(void *common_)
{
	struct fsg_common	*common = common_;

	allow_signal(SIGINT);
	allow_signal(SIGTERM);
	allow_signal(SIGKILL);
	allow_signal(SIGUSR1);

	
	set_freezable();

	set_fs(get_ds());

	
	while (common->state != FSG_STATE_TERMINATED) {
		if (exception_in_progress(common) || signal_pending(current)) {
			handle_exception(common);
			continue;
		}

		if (!common->running) {
			sleep_thread(common);
			continue;
		}

		if (get_next_command(common))
			continue;

		spin_lock_irq(&common->lock);
		if (!exception_in_progress(common))
			common->state = FSG_STATE_DATA_PHASE;
		spin_unlock_irq(&common->lock);

		if (do_scsi_command(common) || finish_reply(common))
			continue;

		spin_lock_irq(&common->lock);
		if (!exception_in_progress(common))
			common->state = FSG_STATE_STATUS_PHASE;
		spin_unlock_irq(&common->lock);

#ifdef CONFIG_USB_CSW_HACK
		if (csw_hack_sent) {
			csw_hack_sent = 0;
			continue;
		}
#endif
		if (send_status(common))
			continue;

		spin_lock_irq(&common->lock);
		if (!exception_in_progress(common))
			common->state = FSG_STATE_IDLE;
		spin_unlock_irq(&common->lock);
	}

	spin_lock_irq(&common->lock);
	common->thread_task = NULL;
	spin_unlock_irq(&common->lock);

	if (!common->ops || !common->ops->thread_exits
	 || common->ops->thread_exits(common) < 0) {
		struct fsg_lun *curlun = common->luns;
		unsigned i = common->nluns;

		down_write(&common->filesem);
		for (; i--; ++curlun) {
			if (!fsg_lun_is_open(curlun))
				continue;

			fsg_lun_close(curlun);
			curlun->unit_attention_data = SS_MEDIUM_NOT_PRESENT;
		}
		up_write(&common->filesem);
	}

	
	complete_and_exit(&common->thread_notifier, 0);
}



static DEVICE_ATTR(ro, 0644, fsg_show_ro, fsg_store_ro);
static DEVICE_ATTR(nofua, 0644, fsg_show_nofua, fsg_store_nofua);
static DEVICE_ATTR(file, 0644, fsg_show_file, fsg_store_file);
#ifdef CONFIG_USB_MSC_PROFILING
static DEVICE_ATTR(perf, 0644, fsg_show_perf, fsg_store_perf);
#endif

#ifdef CONFIG_LISMO

static void buffer_notify_sysfs(struct work_struct *work)
{
	struct op_desc	*desc;
	
	desc = container_of(work, struct op_desc, work);
	sysfs_notify_dirent(desc->value_sd);
}

static int vendor_cmd_is_valid(unsigned cmd)
{
	if(cmd < SC_VENDOR_START)
		return 0;
	if(cmd > SC_VENDOR_END)
		return 0;
	return 1;
}

static ssize_t
vendor_cmd_read_buffer(struct file* f, struct kobject *kobj, struct bin_attribute *attr,
                char *buf, loff_t off, size_t count)
{
	ssize_t	status;
	struct op_desc	*desc = attr->private;

	
	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else {
		size_t srclen, n;
		void *src;
		size_t nleft = count;
		src = desc->buffer;
		srclen = desc->len;

		if (off < srclen) {
			n = min(nleft, srclen - (size_t) off);
			memcpy(buf, src + off, n);
			nleft -= n;
			buf += n;
			off = 0;
		} else {
			off -= srclen;
		}
		status = count - nleft;
	}

	mutex_unlock(&sysfs_lock);
	return status;
}

static ssize_t
vendor_cmd_write_buffer(struct file* f, struct kobject *kobj, struct bin_attribute *attr,
                char *buf, loff_t off, size_t count)
{
	ssize_t	status;
	struct op_desc	*desc = attr->private;

	
	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else {
		size_t dstlen, n;
		size_t nleft = count;
		void *dst;

		dst = desc->buffer;
		dstlen = desc->len;

		if (off < dstlen) {
			n = min(nleft, dstlen - (size_t) off);
			memcpy(dst + off, buf, n);
			nleft -= n;
			buf += n;
			off = 0;
		} else {
			off -= dstlen;
		}
		status = count - nleft;
	}

	desc->update = jiffies;
	schedule_work(&desc->work);

	mutex_unlock(&sysfs_lock);
	return status;
}

static int
vendor_cmd_mmap_buffer(struct file *f, struct kobject *kobj, struct bin_attribute *attr,
		struct vm_area_struct *vma)
{
        int rc = -EINVAL;
	unsigned long pgoff, delta;
	ssize_t size = vma->vm_end - vma->vm_start;
	struct op_desc	*desc = attr->private;

	printk("[fms_CR7]%s\n", __func__);
	mutex_lock(&sysfs_lock);

	if (vma->vm_pgoff != 0) {
		printk("mmap failed: page offset %lx\n", vma->vm_pgoff);
		goto done;
	}

	pgoff = __pa(desc->buffer);
	delta = PAGE_ALIGN(pgoff) - pgoff;
	printk("[fms_CR7]%s size=%x delta=%lx pgoff=%lx\n", __func__, size, delta, pgoff);

        if (size + delta > desc->len) {
			printk("[fms_CR7]%s mmap failed: size %d\n", __func__, size);
		goto done;
        }

        pgoff += delta;
        vma->vm_flags |= VM_RESERVED;

	rc = io_remap_pfn_range(vma, vma->vm_start, pgoff >> PAGE_SHIFT,
		size, vma->vm_page_prot);

	if (rc < 0)
		printk("[fms_CR7]%s mmap failed: remap error %d\n", __func__, rc);
done:
	mutex_unlock(&sysfs_lock);
	return rc;
}

static ssize_t vendor_size_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct op_desc	*desc = dev_to_desc(dev);
	ssize_t		status;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else
		status = sprintf(buf, "%d\n", desc->len);

	mutex_unlock(&sysfs_lock);
	return status;
}

static ssize_t vendor_size_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	long len;
	char* buffer;
	struct op_desc	*desc = dev_to_desc(dev);
	ssize_t		status;
	long cmd;
	char cmd_buf[16]="0x";
	struct fsg_lun	*curlun = fsg_lun_from_dev(&desc->dev);
	int ret;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else {
		struct bin_attribute* dev_bin_attr_buffer = &desc->dev_bin_attr_buffer;
		status = strict_strtol(buf, 0, &len);
		if (status < 0) {
			status = -EINVAL;
			goto done;
		}
		if ( desc->len == len ) {
			status = 0;
			printk("[fms_CR7]%s already size setting! size not change\n", __func__);
			goto done;
		}

		ret = strict_strtol(strcat(cmd_buf,dev_name(&desc->dev)+7), 0, &cmd);
		printk("[fms_CR7]%s cmd=0x%x old_size=0x%x new_size=0x%x \n", __func__, (unsigned int)cmd, (unsigned int)desc->len, (unsigned int)len);

		if ( cmd-SC_VENDOR_START < ALLOC_CMD_CNT && len == ALLOC_INI_SIZE){
			printk("[fms_CR7]%s buffer alreay malloc \n",__func__);
			buffer = curlun->reserve_buf[cmd-SC_VENDOR_START];
		} else {
			printk("[fms_CR7]%s malloc buffer \n",__func__);
			buffer = kzalloc(len, GFP_KERNEL);
			if(!buffer) {
				status = -ENOMEM;
				goto done;
			}
		}
		if ( cmd-SC_VENDOR_START+1 > ALLOC_CMD_CNT || desc->len != ALLOC_INI_SIZE){
			printk("[fms_CR7]%s free old buffer \n",__func__);
			kfree(desc->buffer);
		}
		desc->len = len;
		desc->buffer = buffer;
		device_remove_bin_file(&desc->dev, dev_bin_attr_buffer);
		dev_bin_attr_buffer->size = len;
		status = device_create_bin_file(&desc->dev, dev_bin_attr_buffer);
	}

done:
	mutex_unlock(&sysfs_lock);
	return status ? : size;
}
static DEVICE_ATTR(size, 0606, vendor_size_show, vendor_size_store); 

static ssize_t vendor_update_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct op_desc	*desc = dev_to_desc(dev);
	ssize_t		status;

	mutex_lock(&sysfs_lock);

	if (!test_bit(FLAG_EXPORT, &desc->flags))
		status = -EIO;
	else
		status = sprintf(buf, "%lu\n", desc->update);

	mutex_unlock(&sysfs_lock);
	return status;
}
static DEVICE_ATTR(update, 0404, vendor_update_show, 0); 

static int vendor_cmd_export(struct device *dev, unsigned cmd, int init)
{
	struct fsg_lun	*curlun = fsg_lun_from_dev(dev);
	struct op_desc	*desc;
	int		status = -EINVAL;
	struct bin_attribute* dev_bin_attr_buffer;

	if (!vendor_cmd_is_valid(cmd)){
		printk("[fms_CR7]%s cmd=%02x cmd is invalid\n", __func__, cmd);
		goto done;
	}

	desc = curlun->op_desc[cmd-SC_VENDOR_START];
	if (!desc) {
		desc = kzalloc(sizeof(struct op_desc), GFP_KERNEL);
		if(!desc) {
			printk("[fms_CR7]%s op_desc alloc failed\n", __func__);
			status = -ENOMEM;
			goto done;
		}
		curlun->op_desc[cmd-SC_VENDOR_START] = desc;
	}

	status = 0;
	if (test_bit(FLAG_EXPORT, &desc->flags)) {
		printk("[fms_CR7]%s already exported\n", __func__);
		goto done;
	}

	if ( cmd-SC_VENDOR_START+1 > ALLOC_CMD_CNT ){
		desc->buffer = kzalloc(2048, GFP_KERNEL);
		printk("[fms_CR7]%s opcode:%02x bufalloc size:%08x \n", __func__, cmd, 2048);
		if(!desc->buffer) {
			printk("[fms_CR7]%s buffer alloc failed\n", __func__);
			status = -ENOMEM;
			goto done;
		}
		desc->len = 2048;
	}else{
		desc->buffer = curlun->reserve_buf[cmd-SC_VENDOR_START];
		printk("[fms_CR7]%s opcode:%02x bufcopy bufsize:%08x \n", __func__, cmd, ALLOC_INI_SIZE);
		desc->len = ALLOC_INI_SIZE;
	}

	dev_bin_attr_buffer = &desc->dev_bin_attr_buffer;
	desc->dev.release = op_release;
	desc->dev.parent = &curlun->dev;
	dev_set_drvdata(&desc->dev, curlun);
	dev_set_name(&desc->dev,"opcode-%02x", cmd);
	status = device_register(&desc->dev);
	if (status != 0) {
		printk("[fms_CR7]%s failed to register opcode%d: %d\n", __func__, cmd, status);
		goto done;
	}

	dev_bin_attr_buffer->attr.name = "buffer";
	if (init)
		dev_bin_attr_buffer->attr.mode = 0660;
	else
		dev_bin_attr_buffer->attr.mode = 0606;
	dev_bin_attr_buffer->read = vendor_cmd_read_buffer;
	dev_bin_attr_buffer->write = vendor_cmd_write_buffer;
	dev_bin_attr_buffer->mmap = vendor_cmd_mmap_buffer;
	if ( cmd-SC_VENDOR_START+1 > ALLOC_CMD_CNT ){
		dev_bin_attr_buffer->size = 2048;
	} else {
		dev_bin_attr_buffer->size = ALLOC_INI_SIZE;
	}
	dev_bin_attr_buffer->private = desc;
	status = device_create_bin_file(&desc->dev, dev_bin_attr_buffer);

	if (status != 0) {
		device_remove_bin_file(&desc->dev, dev_bin_attr_buffer);
		kfree(desc->buffer);
		desc->buffer = 0;
		desc->len = 0;
		device_unregister(&desc->dev);
		goto done;
	}

	if (init){
		dev_attr_size.attr.mode = 0660;
		dev_attr_update.attr.mode = 0440;
	} else {
		dev_attr_size.attr.mode = 0606;
		dev_attr_update.attr.mode = 0404;
	}
	status = device_create_file(&desc->dev, &dev_attr_size);
	if (status == 0)
		status = device_create_file(&desc->dev, &dev_attr_update);
	if (status != 0) {
		printk("[fms_CR7]%s device_create_file failed: %d\n", __func__, status);
		device_remove_file(&desc->dev, &dev_attr_update);
		device_remove_file(&desc->dev, &dev_attr_size);
		device_remove_bin_file(&desc->dev, dev_bin_attr_buffer);
		if ( cmd-SC_VENDOR_START+1 > ALLOC_CMD_CNT )
			kfree(desc->buffer);
		desc->buffer = 0;
		desc->len = 0;
		device_unregister(&desc->dev);
		goto done;
	}

	desc->value_sd = sysfs_get_dirent(desc->dev.kobj.sd, NULL, "update");
	INIT_WORK(&desc->work, buffer_notify_sysfs);

	if (status == 0)
		set_bit(FLAG_EXPORT, &desc->flags);

	desc->update = 0;

done:
	if (status)
		pr_debug("%s: opcode%d status %d\n", __func__, cmd, status);
	return status;
}

static void vendor_cmd_unexport(struct device *dev, unsigned cmd)
{
	struct fsg_lun	*curlun = fsg_lun_from_dev(dev);
	struct op_desc *desc;
	int status = -EINVAL;

	if (!vendor_cmd_is_valid(cmd)){
		printk("[fms_CR7]%s cmd=%02x cmd is invalid\n", __func__, cmd);
		goto done;
	}

	desc = curlun->op_desc[cmd-SC_VENDOR_START];
	if (!desc) {
		printk("[fms_CR7]%s not export\n", __func__);
		status = -ENODEV;
		goto done;
	}

	if (test_bit(FLAG_EXPORT, &desc->flags)) {
		struct bin_attribute* dev_bin_attr_buffer = &desc->dev_bin_attr_buffer;
		clear_bit(FLAG_EXPORT, &desc->flags);
		cancel_work_sync(&desc->work);
		device_remove_file(&desc->dev, &dev_attr_update);
		device_remove_file(&desc->dev, &dev_attr_size);
		device_remove_bin_file(&desc->dev, dev_bin_attr_buffer);
		if ( cmd-SC_VENDOR_START+1 > ALLOC_CMD_CNT || desc->len != ALLOC_INI_SIZE){
			kfree(desc->buffer);
			printk("[fms_CR7]%s opcode:%02x free buff\n", __func__, cmd);
		} else
			printk("[fms_CR7]%s opcode:%02x not free buff\n", __func__, cmd);
		desc->buffer = 0;
		desc->len = 0;
		status = 0;
		device_unregister(&desc->dev);
		kfree(desc);
		curlun->op_desc[cmd-SC_VENDOR_START] = 0;
	} else
		status = -ENODEV;

done:
	if (status)
		pr_debug("%s: opcode%d status %d\n", __func__, cmd, status);
}


static ssize_t vendor_export_store(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t len)
{
	long cmd;
	int status;

	status = strict_strtol(buf, 0, &cmd);
	if (status < 0)
		goto done;

	status = -EINVAL;

	if (!vendor_cmd_is_valid(cmd))
		goto done;

	mutex_lock(&sysfs_lock);

	status = vendor_cmd_export(dev, cmd, 0);
	if (status < 0)
		vendor_cmd_unexport(dev, cmd);

	mutex_unlock(&sysfs_lock);
done:
	if (status)
		pr_debug(KERN_INFO"%s: status %d\n", __func__, status);
	return status ? : len;
}

static DEVICE_ATTR(export, 0220, 0, vendor_export_store); 

static ssize_t vendor_unexport_store(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t len)
{
	long cmd;
	int status;

	status = strict_strtol(buf, 0, &cmd);
	if (status < 0)
		goto done;

	status = -EINVAL;

	if (!vendor_cmd_is_valid(cmd))
		goto done;

	mutex_lock(&sysfs_lock);

	status = 0;
	vendor_cmd_unexport(dev, cmd);

	mutex_unlock(&sysfs_lock);
done:
	if (status)
		pr_debug(KERN_INFO"%s: status %d\n", __func__, status);
	return status ? : len;
}
static DEVICE_ATTR(unexport, 0220, 0, vendor_unexport_store); 

static ssize_t vendor_inquiry_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct fsg_lun *curlun = fsg_lun_from_dev(dev);
	ssize_t status;

	mutex_lock(&sysfs_lock);
	status = sprintf(buf, "\"%s\"\n", curlun->inquiry_vendor);

	mutex_unlock(&sysfs_lock);
	return status;
}

static ssize_t vendor_inquiry_store(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t len)
{
	struct fsg_lun *curlun = fsg_lun_from_dev(dev);

	mutex_lock(&sysfs_lock);
	strncpy(curlun->inquiry_vendor, buf,
					sizeof curlun->inquiry_vendor);
	curlun->inquiry_vendor[sizeof curlun->inquiry_vendor - 1] = '\0';

	mutex_unlock(&sysfs_lock);
	return 0;
}

static DEVICE_ATTR(inquiry, 0660, vendor_inquiry_show, vendor_inquiry_store); 

static void op_release(struct device *dev)
{
}
#endif


static int string_id;
static void fsg_update_mode(int _linux_fsg_mode)
{
	if (_linux_fsg_mode) {
		fsg_intf_desc.bInterfaceClass =
			USB_CLASS_VENDOR_SPEC;
		fsg_intf_desc.bInterfaceSubClass =
			USB_SUBCLASS_VENDOR_SPEC;
		fsg_intf_desc.bInterfaceProtocol =
			0x0;
		fsg_intf_desc.iInterface = 0;
	} else {
		fsg_intf_desc.bInterfaceClass =
			USB_CLASS_MASS_STORAGE;
		fsg_intf_desc.bInterfaceSubClass =
			USB_SC_SCSI;
		fsg_intf_desc.bInterfaceProtocol =
			USB_PR_BULK;
		fsg_intf_desc.iInterface = string_id;
	}
}

static void fsg_common_release(struct kref *ref);

static void fsg_lun_release(struct device *dev)
{
	
}

static inline void fsg_common_get(struct fsg_common *common)
{
	kref_get(&common->ref);
}

static inline void fsg_common_put(struct fsg_common *common)
{
	kref_put(&common->ref, fsg_common_release);
}

static struct fsg_common *fsg_common_init(struct fsg_common *common,
					  struct usb_composite_dev *cdev,
					  struct fsg_config *cfg)
{
	struct usb_gadget *gadget = cdev->gadget;
	struct fsg_buffhd *bh;
	struct fsg_lun *curlun;
	struct fsg_lun_config *lcfg;
	int nluns, i, rc;
#ifdef CONFIG_LISMO
	int j;
#endif
	char *pathbuf;

	rc = fsg_num_buffers_validate();
	if (rc != 0)
		return ERR_PTR(rc);

	
	nluns = cfg->nluns;
	if (nluns < 1 || nluns > FSG_MAX_LUNS) {
		dev_err(&gadget->dev, "invalid number of LUNs: %u\n", nluns);
		return ERR_PTR(-EINVAL);
	}

	
	if (!common) {
		common = kzalloc(sizeof *common, GFP_KERNEL);
		if (!common)
			return ERR_PTR(-ENOMEM);
		common->free_storage_on_release = 1;
	} else {
		memset(common, 0, sizeof *common);
		common->free_storage_on_release = 0;
	}

	common->buffhds = kcalloc(fsg_num_buffers,
				  sizeof *(common->buffhds), GFP_KERNEL);
	if (!common->buffhds) {
		if (common->free_storage_on_release)
			kfree(common);
		return ERR_PTR(-ENOMEM);
	}

	common->ops = cfg->ops;
	common->private_data = cfg->private_data;

	common->gadget = gadget;
	common->ep0 = gadget->ep0;
	common->ep0req = cdev->req;
	common->cdev = cdev;

	
	if (fsg_strings[FSG_STRING_INTERFACE].id == 0) {
		rc = usb_string_id(cdev);
		if (unlikely(rc < 0))
			goto error_release;
		fsg_strings[FSG_STRING_INTERFACE].id = rc;
		fsg_intf_desc.iInterface = string_id = rc;
	}

	curlun = kcalloc(nluns, sizeof(*curlun), GFP_KERNEL);
	if (unlikely(!curlun)) {
		rc = -ENOMEM;
		goto error_release;
	}
	common->luns = curlun;

	init_rwsem(&common->filesem);

	for (i = 0, lcfg = cfg->luns; i < nluns; ++i, ++curlun, ++lcfg) {
		curlun->cdrom = !!lcfg->cdrom;
		curlun->ro = lcfg->cdrom || lcfg->ro;
		curlun->initially_ro = curlun->ro;
		curlun->removable = lcfg->removable;
		curlun->nofua = lcfg->nofua;
		curlun->dev.release = fsg_lun_release;
		curlun->dev.parent = &gadget->dev;
		
		dev_set_drvdata(&curlun->dev, &common->filesem);
		dev_set_name(&curlun->dev,
			     cfg->lun_name_format
			   ? cfg->lun_name_format
			   : "lun%d",
			     i);

		rc = device_register(&curlun->dev);
		if (rc) {
			INFO(common, "failed to register LUN%d: %d\n", i, rc);
			common->nluns = i;
			put_device(&curlun->dev);
			goto error_release;
		}

		rc = device_create_file(&curlun->dev, &dev_attr_ro);
		if (rc)
			goto error_luns;
		rc = device_create_file(&curlun->dev, &dev_attr_file);
		if (rc)
			goto error_luns;
		rc = device_create_file(&curlun->dev, &dev_attr_nofua);
		if (rc)
			goto error_luns;
#ifdef CONFIG_USB_MSC_PROFILING
		rc = device_create_file(&curlun->dev, &dev_attr_perf);
		if (rc)
			dev_err(&gadget->dev, "failed to create sysfs entry:"
				"(dev_attr_perf) error: %d\n", rc);
#endif
#ifdef CONFIG_LISMO
		if ( i==0 ){
			rc = device_create_file(&curlun->dev, &dev_attr_export);
			if (rc)
				goto error_luns;
			rc = device_create_file(&curlun->dev, &dev_attr_unexport);
			if (rc)
				goto error_luns;
			rc = device_create_file(&curlun->dev, &dev_attr_inquiry);
			if (rc)
				goto error_luns;
			memset(curlun->inquiry_vendor, 0, sizeof curlun->inquiry_vendor);
			strcpy(curlun->inquiry_vendor, INQUIRY_VENDOR_INIT);

			for (j=0; j < ALLOC_CMD_CNT; j++){
				curlun->reserve_buf[j] = kzalloc(ALLOC_INI_SIZE, GFP_KERNEL);
				printk("[fms_CR7]%s alloc buf[%d]\n", __func__,j);
				if(!curlun->reserve_buf[j]){
					printk("[fms_CR7]%s Error : buffer malloc fail! cmd_idx=%d \n", __func__, j);
					rc = -ENOMEM;
					goto error_release;
				}
			}

			rc = vendor_cmd_export(&curlun->dev, 0xe4, 1);
			if (rc < 0){
				vendor_cmd_unexport(&curlun->dev, 0xe4);
				goto error_release;
			}

		}
#endif
		if (lcfg->filename) {
			rc = fsg_lun_open(curlun, lcfg->filename);
			if (rc)
				goto error_luns;
		} else if (!curlun->removable) {
			ERROR(common, "no file given for LUN%d\n", i);
			rc = -EINVAL;
			goto error_luns;
		}
	}
	common->nluns = nluns;

	
	bh = common->buffhds;
	i = fsg_num_buffers;
	goto buffhds_first_it;
	do {
		bh->next = bh + 1;
		++bh;
buffhds_first_it:
		bh->buf = kmalloc(FSG_BUFLEN, GFP_KERNEL);
		if (unlikely(!bh->buf)) {
			rc = -ENOMEM;
			goto error_release;
		}
	} while (--i);
	bh->next = common->buffhds;

	
	if (cfg->release != 0xffff) {
		i = cfg->release;
	} else {
		i = usb_gadget_controller_number(gadget);
		if (i >= 0) {
			i = 0x0300 + i;
		} else {
			WARNING(common, "controller '%s' not recognized\n",
				gadget->name);
			i = 0x0399;
		}
	}
	snprintf(common->inquiry_string, sizeof common->inquiry_string,
		 "%-8s%-16s%04x", cfg->vendor_name ?: "Linux",
		 
		 cfg->product_name ?: (common->luns->cdrom
				     ? "File-Stor Gadget"
				     : "File-CD Gadget"),
		 i);

	common->can_stall = cfg->can_stall &&
		!(gadget_is_at91(common->gadget));

	spin_lock_init(&common->lock);
	kref_init(&common->ref);

	
	common->thread_task =
		kthread_create(fsg_main_thread, common,
			       cfg->thread_name ?: "file-storage");
	if (IS_ERR(common->thread_task)) {
		rc = PTR_ERR(common->thread_task);
		goto error_release;
	}
	init_completion(&common->thread_notifier);
	init_waitqueue_head(&common->fsg_wait);

	INIT_WORK(&ums_do_reserve_work, handle_reserve_cmd);

	
	INFO(common, FSG_DRIVER_DESC ", version: " FSG_DRIVER_VERSION "\n");
	INFO(common, "Number of LUNs=%d\n", common->nluns);

	pathbuf = kmalloc(PATH_MAX, GFP_KERNEL);
	for (i = 0, nluns = common->nluns, curlun = common->luns;
	     i < nluns;
	     ++curlun, ++i) {
		char *p = "(no medium)";
		if (fsg_lun_is_open(curlun)) {
			p = "(error)";
			if (pathbuf) {
				p = d_path(&curlun->filp->f_path,
					   pathbuf, PATH_MAX);
				if (IS_ERR(p))
					p = "(error)";
			}
		}
		LINFO(curlun, "LUN: %s%s%sfile: %s\n",
		      curlun->removable ? "removable " : "",
		      curlun->ro ? "read only " : "",
		      curlun->cdrom ? "CD-ROM " : "",
		      p);
	}
	kfree(pathbuf);

	DBG(common, "I/O thread pid: %d\n", task_pid_nr(common->thread_task));

	wake_up_process(common->thread_task);

	return common;

error_luns:
	common->nluns = i + 1;
error_release:
	common->state = FSG_STATE_TERMINATED;	
	
	fsg_common_release(&common->ref);
	return ERR_PTR(rc);
}

static void fsg_common_release(struct kref *ref)
{
	struct fsg_common *common = container_of(ref, struct fsg_common, ref);

	
	if (common->state != FSG_STATE_TERMINATED) {
		raise_exception(common, FSG_STATE_EXIT);
		wait_for_completion(&common->thread_notifier);
	}

	if (likely(common->luns)) {
		struct fsg_lun *lun = common->luns;
		unsigned i = common->nluns;
#ifdef CONFIG_LISMO
		unsigned j;
#endif
		
		for (; i; --i, ++lun) {
#ifdef CONFIG_USB_MSC_PROFILING
			device_remove_file(&lun->dev, &dev_attr_perf);
#endif
#ifdef CONFIG_LISMO
			if (i == common->nluns){
				for (j=SC_VENDOR_START; j < SC_VENDOR_END + 1; j++) {
					vendor_cmd_unexport(&lun->dev, j);
					if ( j-SC_VENDOR_START < ALLOC_CMD_CNT ){
						printk("[fms_CR7]%s kfree buf[%d]\n", __func__,j-SC_VENDOR_START);
						kfree(lun->reserve_buf[j-SC_VENDOR_START]);
					}
				}
				device_remove_file(&lun->dev, &dev_attr_export);
				device_remove_file(&lun->dev, &dev_attr_unexport);
				device_remove_file(&lun->dev, &dev_attr_inquiry);
			}
#endif
			device_remove_file(&lun->dev, &dev_attr_nofua);
			device_remove_file(&lun->dev, &dev_attr_ro);
			device_remove_file(&lun->dev, &dev_attr_file);
			fsg_lun_close(lun);
			device_unregister(&lun->dev);
		}

		kfree(common->luns);
	}

	{
		struct fsg_buffhd *bh = common->buffhds;
		unsigned i = fsg_num_buffers;
		do {
			kfree(bh->buf);
		} while (++bh, --i);
	}

	kfree(common->buffhds);
	if (common->free_storage_on_release)
		kfree(common);
}



static void fsg_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct fsg_dev		*fsg = fsg_from_func(f);
	struct fsg_common	*common = fsg->common;

	DBG(fsg, "unbind\n");
	if (fsg->common->fsg == fsg) {
		fsg->common->new_fsg = NULL;
		raise_exception(fsg->common, FSG_STATE_CONFIG_CHANGE);
		
		wait_event(common->fsg_wait, common->fsg != fsg);
	}

	fsg_common_put(common);
	kfree(fsg);
}

static int fsg_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct fsg_dev		*fsg = fsg_from_func(f);
	struct usb_gadget	*gadget = c->cdev->gadget;
	int			i;
	struct usb_ep		*ep;

	fsg->gadget = gadget;

	
	i = usb_interface_id(c, f);
	if (i < 0)
		return i;
	fsg_intf_desc.bInterfaceNumber = i;
	fsg->interface_number = i;

	
	ep = usb_ep_autoconfig(gadget, &fsg_fs_bulk_in_desc);
	if (!ep)
		goto autoconf_fail;
	ep->driver_data = fsg->common;	
	fsg->bulk_in = ep;

	ep = usb_ep_autoconfig(gadget, &fsg_fs_bulk_out_desc);
	if (!ep)
		goto autoconf_fail;
	ep->driver_data = fsg->common;	
	fsg->bulk_out = ep;

	f->descriptors = fsg_fs_function;

	if (gadget_is_dualspeed(gadget)) {
		
		fsg_hs_bulk_in_desc.bEndpointAddress =
			fsg_fs_bulk_in_desc.bEndpointAddress;
		fsg_hs_bulk_out_desc.bEndpointAddress =
			fsg_fs_bulk_out_desc.bEndpointAddress;
		f->hs_descriptors = fsg_hs_function;
	}

	if (gadget_is_superspeed(gadget)) {
		unsigned	max_burst;

		
		max_burst = min_t(unsigned, FSG_BUFLEN / 1024, 15);

		fsg_ss_bulk_in_desc.bEndpointAddress =
			fsg_fs_bulk_in_desc.bEndpointAddress;
		fsg_ss_bulk_in_comp_desc.bMaxBurst = max_burst;

		fsg_ss_bulk_out_desc.bEndpointAddress =
			fsg_fs_bulk_out_desc.bEndpointAddress;
		fsg_ss_bulk_out_comp_desc.bMaxBurst = max_burst;

		f->ss_descriptors = fsg_ss_function;
	}

	return 0;

autoconf_fail:
	ERROR(fsg, "unable to autoconfigure all endpoints\n");
	return -ENOTSUPP;
}



static struct usb_gadget_strings *fsg_strings_array[] = {
	&fsg_stringtab,
	NULL,
};

static int fsg_bind_config(struct usb_composite_dev *cdev,
			   struct usb_configuration *c,
			   struct fsg_common *common)
{
	struct fsg_dev *fsg;
	int rc;

	fsg = kzalloc(sizeof *fsg, GFP_KERNEL);
	if (unlikely(!fsg))
		return -ENOMEM;

	fsg->function.name        = "mass_storage";
	fsg->function.strings     = fsg_strings_array;
	fsg->function.bind        = fsg_bind;
	fsg->function.unbind      = fsg_unbind;
	fsg->function.setup       = fsg_setup;
	fsg->function.set_alt     = fsg_set_alt;
	fsg->function.disable     = fsg_disable;

	fsg->common               = common;

	rc = usb_add_function(c, &fsg->function);
	if (unlikely(rc))
		kfree(fsg);
	else
		fsg_common_get(fsg->common);
	return rc;
}

static inline int __deprecated __maybe_unused
fsg_add(struct usb_composite_dev *cdev, struct usb_configuration *c,
	struct fsg_common *common)
{
	return fsg_bind_config(cdev, c, common);
}



struct fsg_module_parameters {
	char		*file[FSG_MAX_LUNS];
	bool		ro[FSG_MAX_LUNS];
	bool		removable[FSG_MAX_LUNS];
	bool		cdrom[FSG_MAX_LUNS];
	bool		nofua[FSG_MAX_LUNS];

	unsigned int	file_count, ro_count, removable_count, cdrom_count;
	unsigned int	nofua_count;
	unsigned int	luns;	
	bool		stall;	
};

#define _FSG_MODULE_PARAM_ARRAY(prefix, params, name, type, desc)	\
	module_param_array_named(prefix ## name, params.name, type,	\
				 &prefix ## params.name ## _count,	\
				 S_IRUGO);				\
	MODULE_PARM_DESC(prefix ## name, desc)

#define _FSG_MODULE_PARAM(prefix, params, name, type, desc)		\
	module_param_named(prefix ## name, params.name, type,		\
			   S_IRUGO);					\
	MODULE_PARM_DESC(prefix ## name, desc)

#define FSG_MODULE_PARAMETERS(prefix, params)				\
	_FSG_MODULE_PARAM_ARRAY(prefix, params, file, charp,		\
				"names of backing files or devices");	\
	_FSG_MODULE_PARAM_ARRAY(prefix, params, ro, bool,		\
				"true to force read-only");		\
	_FSG_MODULE_PARAM_ARRAY(prefix, params, removable, bool,	\
				"true to simulate removable media");	\
	_FSG_MODULE_PARAM_ARRAY(prefix, params, cdrom, bool,		\
				"true to simulate CD-ROM instead of disk"); \
	_FSG_MODULE_PARAM_ARRAY(prefix, params, nofua, bool,		\
				"true to ignore SCSI WRITE(10,12) FUA bit"); \
	_FSG_MODULE_PARAM(prefix, params, luns, uint,			\
			  "number of LUNs");				\
	_FSG_MODULE_PARAM(prefix, params, stall, bool,			\
			  "false to prevent bulk stalls")

static void
fsg_config_from_params(struct fsg_config *cfg,
		       const struct fsg_module_parameters *params)
{
	struct fsg_lun_config *lun;
	unsigned i;

	
	cfg->nluns =
		min(params->luns ?: (params->file_count ?: 1u),
		    (unsigned)FSG_MAX_LUNS);
	for (i = 0, lun = cfg->luns; i < cfg->nluns; ++i, ++lun) {
		lun->ro = !!params->ro[i];
		lun->cdrom = !!params->cdrom[i];
		lun->removable = 
			params->removable_count <= i || params->removable[i];
		lun->filename =
			params->file_count > i && params->file[i][0]
			? params->file[i]
			: 0;
	}

	
	cfg->lun_name_format = 0;
	cfg->thread_name = 0;
	cfg->vendor_name = 0;
	cfg->product_name = 0;
	cfg->release = 0xffff;

	cfg->ops = NULL;
	cfg->private_data = NULL;

	
	cfg->can_stall = params->stall;
}

static inline struct fsg_common *
fsg_common_from_params(struct fsg_common *common,
		       struct usb_composite_dev *cdev,
		       const struct fsg_module_parameters *params)
	__attribute__((unused));
static inline struct fsg_common *
fsg_common_from_params(struct fsg_common *common,
		       struct usb_composite_dev *cdev,
		       const struct fsg_module_parameters *params)
{
	struct fsg_config cfg;
	fsg_config_from_params(&cfg, params);
	return fsg_common_init(common, cdev, &cfg);
}

