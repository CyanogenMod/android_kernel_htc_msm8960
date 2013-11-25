/*
 * storage_common.c -- Common definitions for mass storage functionality
 *
 * Copyright (C) 2003-2008 Alan Stern
 * Copyeight (C) 2009 Samsung Electronics
 * Author: Michal Nazarewicz (mina86@mina86.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */







#include <linux/usb/storage.h>
#include <scsi/scsi.h>
#include <asm/unaligned.h>


#define FSG_VENDOR_ID	0x0525	
#define FSG_PRODUCT_ID	0xa4a5	




#ifndef DEBUG
#undef VERBOSE_DEBUG
#undef DUMP_MSGS
#endif 

#ifdef VERBOSE_DEBUG
#define VLDBG	LDBG
#else
#define VLDBG(lun, fmt, args...) do { } while (0)
#endif 

#define LDBG(lun, fmt, args...)   dev_dbg (&(lun)->dev, fmt, ## args)
#define LERROR(lun, fmt, args...) dev_err (&(lun)->dev, fmt, ## args)
#define LWARN(lun, fmt, args...)  dev_warn(&(lun)->dev, fmt, ## args)
#define LINFO(lun, fmt, args...)  dev_info(&(lun)->dev, fmt, ## args)

#define DBG(d, fmt, args...)     dev_dbg(&(d)->gadget->dev , fmt , ## args)
#define VDBG(d, fmt, args...)    dev_vdbg(&(d)->gadget->dev , fmt , ## args)
#define ERROR(d, fmt, args...)   dev_err(&(d)->gadget->dev , fmt , ## args)
#define WARNING(d, fmt, args...) dev_warn(&(d)->gadget->dev , fmt , ## args)
#define INFO(d, fmt, args...)    dev_info(&(d)->gadget->dev , fmt , ## args)



#ifdef DUMP_MSGS

#  define dump_msg(fsg,  label,			\
		    buf,  length) do {	\
	if (length < 512) {						\
		DBG(fsg, "%s, length %u:\n", label, length);		\
		print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_OFFSET,	\
			       16, 1, buf, length, 0);			\
	}								\
} while (0)

#  define dump_cdb(fsg) do { } while (0)

#else

#  define dump_msg(fsg,  label, \
		    buf,  length) do { } while (0)

#  ifdef VERBOSE_DEBUG

#    define dump_cdb(fsg)						\
	print_hex_dump(KERN_DEBUG, "SCSI CDB: ", DUMP_PREFIX_NONE,	\
		       16, 1, (fsg)->cmnd, (fsg)->cmnd_size, 0)		\

#  else

#    define dump_cdb(fsg) do { } while (0)

#  endif 

#endif 


struct interrupt_data {
	u8	bType;
	u8	bValue;
};

#define CBI_INTERRUPT_DATA_LEN		2

#define USB_CBI_ADSC_REQUEST		0x00


#define MAX_COMMAND_SIZE	16

#ifdef CONFIG_PASCAL_DETECT
#define SC_PASCAL_MODE		0xff
#endif
#ifdef CONFIG_LISMO
#define SC_VENDOR_START			0xe4
#define SC_VENDOR_END			0xef
#endif
#define SS_NO_SENSE				0
#define SS_COMMUNICATION_FAILURE		0x040800
#define SS_INVALID_COMMAND			0x052000
#define SS_INVALID_FIELD_IN_CDB			0x052400
#define SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE	0x052100
#define SS_LOGICAL_UNIT_NOT_SUPPORTED		0x052500
#define SS_MEDIUM_NOT_PRESENT			0x023a00
#define SS_MEDIUM_REMOVAL_PREVENTED		0x055302
#define SS_NOT_READY_TO_READY_TRANSITION	0x062800
#define SS_RESET_OCCURRED			0x062900
#define SS_SAVING_PARAMETERS_NOT_SUPPORTED	0x053900
#define SS_UNRECOVERED_READ_ERROR		0x031100
#define SS_WRITE_ERROR				0x030c02
#define SS_WRITE_PROTECTED			0x072700

#define SK(x)		((u8) ((x) >> 16))	
#define ASC(x)		((u8) ((x) >> 8))
#define ASCQ(x)		((u8) (x))

#ifdef CONFIG_LISMO
#define VENDOR_CMD_NR	(SC_VENDOR_END - SC_VENDOR_START + 1)
#define INQUIRY_VENDOR_INIT	"LISMOSC1"

#define INQUIRY_VENDOR_SPECIFIC_SIZE 20 

#define ALLOC_INI_SIZE  0x101000
#define ALLOC_CMD_CNT   1


struct op_desc {
	struct device	dev;
	unsigned long	flags;
#define FLAG_IS_READ	0
#define FLAG_IS_WRITE	1
#define FLAG_EXPORT	2	

	char			*buffer;
	size_t			len;
	struct bin_attribute	dev_bin_attr_buffer;
	unsigned long 		update;
	struct work_struct	work;
	struct sysfs_dirent	*value_sd;
};
static void op_release(struct device *dev);

static DEFINE_MUTEX(sysfs_lock);
#endif



struct fsg_lun {
	struct file	*filp;
	loff_t		file_length;
	loff_t		num_sectors;

	unsigned int	initially_ro:1;
	unsigned int	ro:1;
	unsigned int	removable:1;
	unsigned int	cdrom:1;
	unsigned int	prevent_medium_removal:1;
	unsigned int	registered:1;
	unsigned int	info_valid:1;
	unsigned int	nofua:1;

	u32		sense_data;
	u32		sense_data_info;
	u32		unit_attention_data;

	unsigned int	blkbits;	
	unsigned int	blksize;	
	struct device	dev;
#ifdef CONFIG_USB_MSC_PROFILING
	spinlock_t	lock;
	struct {

		unsigned long rbytes;
		unsigned long wbytes;
		ktime_t rtime;
		ktime_t wtime;
	} perf;

#endif
#ifdef CONFIG_LISMO
	struct op_desc *op_desc[VENDOR_CMD_NR];

	
	char inquiry_vendor[INQUIRY_VENDOR_SPECIFIC_SIZE + 1];
	char   *reserve_buf[VENDOR_CMD_NR];
#endif
};

#define fsg_lun_is_open(curlun)	((curlun)->filp != NULL)

static struct fsg_lun *fsg_lun_from_dev(struct device *dev)
{
	return container_of(dev, struct fsg_lun, dev);
}

#ifdef CONFIG_LISMO
static struct op_desc *dev_to_desc(struct device *dev)
{
	return container_of(dev, struct op_desc, dev);
}
#endif


#define EP0_BUFSIZE	256
#define DELAYED_STATUS	(EP0_BUFSIZE + 999)	

#ifdef CONFIG_USB_CSW_HACK
#define fsg_num_buffers		4
#else
#ifdef CONFIG_USB_GADGET_DEBUG_FILES

static unsigned int fsg_num_buffers = CONFIG_USB_GADGET_STORAGE_NUM_BUFFERS;
module_param_named(num_buffers, fsg_num_buffers, uint, S_IRUGO);
MODULE_PARM_DESC(num_buffers, "Number of pipeline buffers");

#else

#define fsg_num_buffers	CONFIG_USB_GADGET_STORAGE_NUM_BUFFERS

#endif 
#endif 

static inline int fsg_num_buffers_validate(void)
{
	if (fsg_num_buffers >= 2 && fsg_num_buffers <= 4)
		return 0;
	pr_err("fsg_num_buffers %u is out of range (%d to %d)\n",
	       fsg_num_buffers, 2 ,4);
	return -EINVAL;
}

#define FSG_BUFLEN	((u32)16384)

#define FSG_MAX_LUNS	8

enum fsg_buffer_state {
	BUF_STATE_EMPTY = 0,
	BUF_STATE_FULL,
	BUF_STATE_BUSY
};

struct fsg_buffhd {
#ifdef FSG_BUFFHD_STATIC_BUFFER
	char				buf[FSG_BUFLEN];
#else
	void				*buf;
#endif
	enum fsg_buffer_state		state;
	struct fsg_buffhd		*next;

	unsigned int			bulk_out_intended_length;

	struct usb_request		*inreq;
	int				inreq_busy;
	struct usb_request		*outreq;
	int				outreq_busy;
};

enum fsg_state {
	
	FSG_STATE_COMMAND_PHASE = -10,
	FSG_STATE_DATA_PHASE,
	FSG_STATE_STATUS_PHASE,

	FSG_STATE_IDLE = 0,
	FSG_STATE_ABORT_BULK_OUT,
	FSG_STATE_RESET,
	FSG_STATE_INTERFACE_CHANGE,
	FSG_STATE_CONFIG_CHANGE,
	FSG_STATE_DISCONNECT,
	FSG_STATE_EXIT,
	FSG_STATE_TERMINATED
};

enum data_direction {
	DATA_DIR_UNKNOWN = 0,
	DATA_DIR_FROM_HOST,
	DATA_DIR_TO_HOST,
	DATA_DIR_NONE
};




static inline u32 get_unaligned_be24(u8 *buf)
{
	return 0xffffff & (u32) get_unaligned_be32(buf - 1);
}




enum {
#ifndef FSG_NO_DEVICE_STRINGS
	FSG_STRING_MANUFACTURER	= 1,
	FSG_STRING_PRODUCT,
	FSG_STRING_SERIAL,
	FSG_STRING_CONFIG,
#endif
	FSG_STRING_INTERFACE
};


#ifndef FSG_NO_OTG
static struct usb_otg_descriptor
fsg_otg_desc = {
	.bLength =		sizeof fsg_otg_desc,
	.bDescriptorType =	USB_DT_OTG,

	.bmAttributes =		USB_OTG_SRP,
};
#endif


static struct usb_interface_descriptor
fsg_intf_desc = {
	.bLength =		sizeof fsg_intf_desc,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bNumEndpoints =	2,		
	.bInterfaceClass =	USB_CLASS_MASS_STORAGE,
	.bInterfaceSubClass =	USB_SC_SCSI,	
	.bInterfaceProtocol =	USB_PR_BULK,	
	.iInterface =		FSG_STRING_INTERFACE,
};


static struct usb_endpoint_descriptor
fsg_fs_bulk_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	
};

static struct usb_endpoint_descriptor
fsg_fs_bulk_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	
};

#ifndef FSG_NO_INTR_EP

static struct usb_endpoint_descriptor
fsg_fs_intr_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(2),
	.bInterval =		32,	
};

#ifndef FSG_NO_OTG
#  define FSG_FS_FUNCTION_PRE_EP_ENTRIES	2
#else
#  define FSG_FS_FUNCTION_PRE_EP_ENTRIES	1
#endif

#endif

static struct usb_descriptor_header *fsg_fs_function[] = {
#ifndef FSG_NO_OTG
	(struct usb_descriptor_header *) &fsg_otg_desc,
#endif
	(struct usb_descriptor_header *) &fsg_intf_desc,
	(struct usb_descriptor_header *) &fsg_fs_bulk_in_desc,
	(struct usb_descriptor_header *) &fsg_fs_bulk_out_desc,
#ifndef FSG_NO_INTR_EP
	(struct usb_descriptor_header *) &fsg_fs_intr_in_desc,
#endif
	NULL,
};


static struct usb_endpoint_descriptor
fsg_hs_bulk_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_endpoint_descriptor
fsg_hs_bulk_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
	.bInterval =		1,	
};

#ifndef FSG_NO_INTR_EP

static struct usb_endpoint_descriptor
fsg_hs_intr_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(2),
	.bInterval =		9,	
};

#ifndef FSG_NO_OTG
#  define FSG_HS_FUNCTION_PRE_EP_ENTRIES	2
#else
#  define FSG_HS_FUNCTION_PRE_EP_ENTRIES	1
#endif

#endif

static struct usb_descriptor_header *fsg_hs_function[] = {
#ifndef FSG_NO_OTG
	(struct usb_descriptor_header *) &fsg_otg_desc,
#endif
	(struct usb_descriptor_header *) &fsg_intf_desc,
	(struct usb_descriptor_header *) &fsg_hs_bulk_in_desc,
	(struct usb_descriptor_header *) &fsg_hs_bulk_out_desc,
#ifndef FSG_NO_INTR_EP
	(struct usb_descriptor_header *) &fsg_hs_intr_in_desc,
#endif
	NULL,
};

static struct usb_endpoint_descriptor
fsg_ss_bulk_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor fsg_ss_bulk_in_comp_desc = {
	.bLength =		sizeof(fsg_ss_bulk_in_comp_desc),
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,

	
};

static struct usb_endpoint_descriptor
fsg_ss_bulk_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor fsg_ss_bulk_out_comp_desc = {
	.bLength =		sizeof(fsg_ss_bulk_in_comp_desc),
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,

	
};

#ifndef FSG_NO_INTR_EP

static struct usb_endpoint_descriptor
fsg_ss_intr_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(2),
	.bInterval =		9,	
};

static struct usb_ss_ep_comp_descriptor fsg_ss_intr_in_comp_desc = {
	.bLength =		sizeof(fsg_ss_bulk_in_comp_desc),
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,

	.wBytesPerInterval =	cpu_to_le16(2),
};

#ifndef FSG_NO_OTG
#  define FSG_SS_FUNCTION_PRE_EP_ENTRIES	2
#else
#  define FSG_SS_FUNCTION_PRE_EP_ENTRIES	1
#endif

#endif

static __maybe_unused struct usb_ext_cap_descriptor fsg_ext_cap_desc = {
	.bLength =		USB_DT_USB_EXT_CAP_SIZE,
	.bDescriptorType =	USB_DT_DEVICE_CAPABILITY,
	.bDevCapabilityType =	USB_CAP_TYPE_EXT,

	.bmAttributes =		cpu_to_le32(USB_LPM_SUPPORT),
};

static __maybe_unused struct usb_ss_cap_descriptor fsg_ss_cap_desc = {
	.bLength =		USB_DT_USB_SS_CAP_SIZE,
	.bDescriptorType =	USB_DT_DEVICE_CAPABILITY,
	.bDevCapabilityType =	USB_SS_CAP_TYPE,

	

	.wSpeedSupported =	cpu_to_le16(USB_LOW_SPEED_OPERATION
		| USB_FULL_SPEED_OPERATION
		| USB_HIGH_SPEED_OPERATION
		| USB_5GBPS_OPERATION),
	.bFunctionalitySupport = USB_LOW_SPEED_OPERATION,
	.bU1devExitLat =	USB_DEFAULT_U1_DEV_EXIT_LAT,
	.bU2DevExitLat =	cpu_to_le16(USB_DEFAULT_U2_DEV_EXIT_LAT),
};

static __maybe_unused struct usb_bos_descriptor fsg_bos_desc = {
	.bLength =		USB_DT_BOS_SIZE,
	.bDescriptorType =	USB_DT_BOS,

	.wTotalLength =		cpu_to_le16(USB_DT_BOS_SIZE
				+ USB_DT_USB_EXT_CAP_SIZE
				+ USB_DT_USB_SS_CAP_SIZE),

	.bNumDeviceCaps =	2,
};

static struct usb_descriptor_header *fsg_ss_function[] = {
#ifndef FSG_NO_OTG
	(struct usb_descriptor_header *) &fsg_otg_desc,
#endif
	(struct usb_descriptor_header *) &fsg_intf_desc,
	(struct usb_descriptor_header *) &fsg_ss_bulk_in_desc,
	(struct usb_descriptor_header *) &fsg_ss_bulk_in_comp_desc,
	(struct usb_descriptor_header *) &fsg_ss_bulk_out_desc,
	(struct usb_descriptor_header *) &fsg_ss_bulk_out_comp_desc,
#ifndef FSG_NO_INTR_EP
	(struct usb_descriptor_header *) &fsg_ss_intr_in_desc,
	(struct usb_descriptor_header *) &fsg_ss_intr_in_comp_desc,
#endif
	NULL,
};

static __maybe_unused struct usb_endpoint_descriptor *
fsg_ep_desc(struct usb_gadget *g, struct usb_endpoint_descriptor *fs,
		struct usb_endpoint_descriptor *hs,
		struct usb_endpoint_descriptor *ss)
{
	if (gadget_is_superspeed(g) && g->speed == USB_SPEED_SUPER)
		return ss;
	else if (gadget_is_dualspeed(g) && g->speed == USB_SPEED_HIGH)
		return hs;
	return fs;
}


static struct usb_string		fsg_strings[] = {
#ifndef FSG_NO_DEVICE_STRINGS
	{FSG_STRING_MANUFACTURER,	fsg_string_manufacturer},
	{FSG_STRING_PRODUCT,		fsg_string_product},
	{FSG_STRING_SERIAL,		""},
	{FSG_STRING_CONFIG,		fsg_string_config},
#endif
	{FSG_STRING_INTERFACE,		fsg_string_interface},
	{}
};

static struct usb_gadget_strings	fsg_stringtab = {
	.language	= 0x0409,		
	.strings	= fsg_strings,
};


 


static int fsg_lun_open(struct fsg_lun *curlun, const char *filename)
{
	int				ro;
	struct file			*filp = NULL;
	int				rc = -EINVAL;
	struct inode			*inode = NULL;
	loff_t				size;
	loff_t				num_sectors;
	loff_t				min_sectors;

	
	ro = curlun->initially_ro;
	if (!ro) {
		filp = filp_open(filename, O_RDWR | O_LARGEFILE, 0);
		if (PTR_ERR(filp) == -EROFS || PTR_ERR(filp) == -EACCES)
			ro = 1;
	}
	if (ro)
		filp = filp_open(filename, O_RDONLY | O_LARGEFILE, 0);
	if (IS_ERR(filp)) {
		LINFO(curlun, "unable to open backing file: %s\n", filename);
		return PTR_ERR(filp);
	}

	if (!(filp->f_mode & FMODE_WRITE))
		ro = 1;

	if (filp->f_path.dentry)
		inode = filp->f_path.dentry->d_inode;
	if (!inode || (!S_ISREG(inode->i_mode) && !S_ISBLK(inode->i_mode))) {
		LINFO(curlun, "invalid file type: %s\n", filename);
		goto out;
	}

	if (!filp->f_op || !(filp->f_op->read || filp->f_op->aio_read)) {
		LINFO(curlun, "file not readable: %s\n", filename);
		goto out;
	}
	if (!(filp->f_op->write || filp->f_op->aio_write))
		ro = 1;

	size = i_size_read(inode->i_mapping->host);
	if (size < 0) {
		LINFO(curlun, "unable to find file size: %s\n", filename);
		rc = (int) size;
		goto out;
	}

	if (curlun->cdrom) {
		curlun->blksize = 2048;
		curlun->blkbits = 11;
	} else if (inode->i_bdev) {
		curlun->blksize = bdev_logical_block_size(inode->i_bdev);
		curlun->blkbits = blksize_bits(curlun->blksize);
	} else {
		curlun->blksize = 512;
		curlun->blkbits = 9;
	}

	num_sectors = size >> curlun->blkbits; 
	min_sectors = 1;
	if (curlun->cdrom) {
		min_sectors = 300;	
		if (num_sectors >= 256*60*75) {
			num_sectors = 256*60*75 - 1;
			LINFO(curlun, "file too big: %s\n", filename);
			LINFO(curlun, "using only first %d blocks\n",
					(int) num_sectors);
		}
	}
	if (num_sectors < min_sectors) {
		LINFO(curlun, "file too small: %s\n", filename);
		rc = -ETOOSMALL;
		goto out;
	}

	get_file(filp);
	curlun->ro = ro;
	curlun->filp = filp;
	curlun->file_length = size;
	curlun->num_sectors = num_sectors;
	LDBG(curlun, "open backing file: %s\n", filename);
	rc = 0;

out:
	filp_close(filp, current->files);
	return rc;
}


static void fsg_lun_close(struct fsg_lun *curlun)
{
	if (curlun->filp) {
		LDBG(curlun, "close backing file\n");
		fput(curlun->filp);
		curlun->filp = NULL;
	}
}



static int fsg_lun_fsync_sub(struct fsg_lun *curlun)
{
	struct file	*filp = curlun->filp;

	if (curlun->ro || !filp)
		return 0;
	return vfs_fsync(filp, 1);
}

static void store_cdrom_address(u8 *dest, int msf, u32 addr)
{
	if (msf) {
		
		addr >>= 2;		
		addr += 2*75;		
		dest[3] = addr % 75;	
		addr /= 75;
		dest[2] = addr % 60;	
		addr /= 60;
		dest[1] = addr;		
		dest[0] = 0;		
	} else {
		
		put_unaligned_be32(addr, dest);
	}
}




static ssize_t fsg_show_ro(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct fsg_lun	*curlun = fsg_lun_from_dev(dev);

	return sprintf(buf, "%d\n", fsg_lun_is_open(curlun)
				  ? curlun->ro
				  : curlun->initially_ro);
}

static ssize_t fsg_show_nofua(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct fsg_lun	*curlun = fsg_lun_from_dev(dev);

	return sprintf(buf, "%u\n", curlun->nofua);
}

#ifdef CONFIG_USB_MSC_PROFILING
static ssize_t fsg_show_perf(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct fsg_lun	*curlun = fsg_lun_from_dev(dev);
	unsigned long rbytes, wbytes;
	int64_t rtime, wtime;

	spin_lock(&curlun->lock);
	rbytes = curlun->perf.rbytes;
	wbytes = curlun->perf.wbytes;
	rtime = ktime_to_us(curlun->perf.rtime);
	wtime = ktime_to_us(curlun->perf.wtime);
	spin_unlock(&curlun->lock);

	return snprintf(buf, PAGE_SIZE, "Write performance :"
					"%lu bytes in %lld microseconds\n"
					"Read performance :"
					"%lu bytes in %lld microseconds\n",
					wbytes, wtime, rbytes, rtime);
}
static ssize_t fsg_store_perf(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct fsg_lun	*curlun = fsg_lun_from_dev(dev);
	int value;

	sscanf(buf, "%d", &value);
	if (!value) {
		spin_lock(&curlun->lock);
		memset(&curlun->perf, 0, sizeof(curlun->perf));
		spin_unlock(&curlun->lock);
	}

	return count;
}
#endif
static ssize_t fsg_show_file(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct fsg_lun	*curlun = fsg_lun_from_dev(dev);
	struct rw_semaphore	*filesem = dev_get_drvdata(dev);
	char		*p;
	ssize_t		rc;

	down_read(filesem);
	if (fsg_lun_is_open(curlun)) {	
		p = d_path(&curlun->filp->f_path, buf, PAGE_SIZE - 1);
		if (IS_ERR(p))
			rc = PTR_ERR(p);
		else {
			rc = strlen(p);
			memmove(buf, p, rc);
			buf[rc] = '\n';		
			buf[++rc] = 0;
		}
	} else {				
		*buf = 0;
		rc = 0;
	}
	up_read(filesem);
	return rc;
}


static ssize_t fsg_store_ro(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	ssize_t		rc;
	struct fsg_lun	*curlun = fsg_lun_from_dev(dev);
	struct rw_semaphore	*filesem = dev_get_drvdata(dev);
	unsigned	ro;

	rc = kstrtouint(buf, 2, &ro);
	if (rc)
		return rc;

	down_read(filesem);
	if (fsg_lun_is_open(curlun)) {
		LDBG(curlun, "read-only status change prevented\n");
		rc = -EBUSY;
	} else {
		curlun->ro = ro;
		curlun->initially_ro = ro;
		LDBG(curlun, "read-only status set to %d\n", curlun->ro);
		rc = count;
	}
	up_read(filesem);
	return rc;
}

static ssize_t fsg_store_nofua(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct fsg_lun	*curlun = fsg_lun_from_dev(dev);
	unsigned	nofua;
	int		ret;

	ret = kstrtouint(buf, 2, &nofua);
	if (ret)
		return ret;

	
	if (!nofua && curlun->nofua)
		fsg_lun_fsync_sub(curlun);

	curlun->nofua = nofua;

	return count;
}

static ssize_t fsg_store_file(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct fsg_lun	*curlun = fsg_lun_from_dev(dev);
	struct rw_semaphore	*filesem = dev_get_drvdata(dev);
	int		rc = 0;


#ifndef CONFIG_USB_ANDROID_MASS_STORAGE
	if (curlun->prevent_medium_removal && fsg_lun_is_open(curlun)) {
		LDBG(curlun, "eject attempt prevented\n");
		return -EBUSY;				
	}
#endif

	
	if (count > 0 && buf[count-1] == '\n')
		((char *) buf)[count-1] = 0;		

	
	down_write(filesem);
	if (fsg_lun_is_open(curlun)) {
		fsg_lun_close(curlun);
		curlun->unit_attention_data = SS_MEDIUM_NOT_PRESENT;
	}

	
	if (count > 0 && buf[0]) {
		rc = fsg_lun_open(curlun, buf);
		if (rc == 0)
			curlun->unit_attention_data =
					SS_NOT_READY_TO_READY_TRANSITION;
	}
	up_write(filesem);
	return (rc < 0 ? rc : count);
}
