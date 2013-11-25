/* Driver for SanDisk SDDR-09 SmartMedia reader
 *
 *   (c) 2000, 2001 Robert Baruch (autophile@starband.net)
 *   (c) 2002 Andries Brouwer (aeb@cwi.nl)
 * Developed with the assistance of:
 *   (c) 2002 Alan Stern <stern@rowland.org>
 *
 * The SanDisk SDDR-09 SmartMedia reader uses the Shuttle EUSB-01 chip.
 * This chip is a programmable USB controller. In the SDDR-09, it has
 * been programmed to obey a certain limited set of SCSI commands.
 * This driver translates the "real" SCSI commands to the SDDR-09 SCSI
 * commands.
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


#include <linux/errno.h>
#include <linux/module.h>
#include <linux/slab.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>

#include "usb.h"
#include "transport.h"
#include "protocol.h"
#include "debug.h"

MODULE_DESCRIPTION("Driver for SanDisk SDDR-09 SmartMedia reader");
MODULE_AUTHOR("Andries Brouwer <aeb@cwi.nl>, Robert Baruch <autophile@starband.net>");
MODULE_LICENSE("GPL");

static int usb_stor_sddr09_dpcm_init(struct us_data *us);
static int sddr09_transport(struct scsi_cmnd *srb, struct us_data *us);
static int usb_stor_sddr09_init(struct us_data *us);


#define UNUSUAL_DEV(id_vendor, id_product, bcdDeviceMin, bcdDeviceMax, \
		    vendorName, productName, useProtocol, useTransport, \
		    initFunction, flags) \
{ USB_DEVICE_VER(id_vendor, id_product, bcdDeviceMin, bcdDeviceMax), \
  .driver_info = (flags)|(USB_US_TYPE_STOR<<24) }

static struct usb_device_id sddr09_usb_ids[] = {
#	include "unusual_sddr09.h"
	{ }		
};
MODULE_DEVICE_TABLE(usb, sddr09_usb_ids);

#undef UNUSUAL_DEV

#define UNUSUAL_DEV(idVendor, idProduct, bcdDeviceMin, bcdDeviceMax, \
		    vendor_name, product_name, use_protocol, use_transport, \
		    init_function, Flags) \
{ \
	.vendorName = vendor_name,	\
	.productName = product_name,	\
	.useProtocol = use_protocol,	\
	.useTransport = use_transport,	\
	.initFunction = init_function,	\
}

static struct us_unusual_dev sddr09_unusual_dev_list[] = {
#	include "unusual_sddr09.h"
	{ }		
};

#undef UNUSUAL_DEV


#define short_pack(lsb,msb) ( ((u16)(lsb)) | ( ((u16)(msb))<<8 ) )
#define LSB_of(s) ((s)&0xFF)
#define MSB_of(s) ((s)>>8)



struct nand_flash_dev {
	int model_id;
	int chipshift;		
	char pageshift;		
	char blockshift;	
	char zoneshift;		
				
	char pageadrlen;	
};

#define NAND_MFR_AMD		0x01
#define NAND_MFR_NATSEMI	0x8f
#define NAND_MFR_TOSHIBA	0x98
#define NAND_MFR_SAMSUNG	0xec

static inline char *nand_flash_manufacturer(int manuf_id) {
	switch(manuf_id) {
	case NAND_MFR_AMD:
		return "AMD";
	case NAND_MFR_NATSEMI:
		return "NATSEMI";
	case NAND_MFR_TOSHIBA:
		return "Toshiba";
	case NAND_MFR_SAMSUNG:
		return "Samsung";
	default:
		return "unknown";
	}
}


static struct nand_flash_dev nand_flash_ids[] = {
	
	{ 0x6e, 20, 8, 4, 8, 2},	
	{ 0xe8, 20, 8, 4, 8, 2},	
	{ 0xec, 20, 8, 4, 8, 2},	
	{ 0x64, 21, 8, 4, 9, 2}, 	
	{ 0xea, 21, 8, 4, 9, 2},	
	{ 0x6b, 22, 9, 4, 9, 2},	
	{ 0xe3, 22, 9, 4, 9, 2},	
	{ 0xe5, 22, 9, 4, 9, 2},	
	{ 0xe6, 23, 9, 4, 10, 2},	
	{ 0x73, 24, 9, 5, 10, 2},	
	{ 0x75, 25, 9, 5, 10, 2},	
	{ 0x76, 26, 9, 5, 10, 3},	
	{ 0x79, 27, 9, 5, 10, 3},	

	
	{ 0x5d, 21, 9, 4, 8, 2},	
	{ 0xd5, 22, 9, 4, 9, 2},	
	{ 0xd6, 23, 9, 4, 10, 2},	
	{ 0x57, 24, 9, 4, 11, 2},	
	{ 0x58, 25, 9, 4, 12, 2},	
	{ 0,}
};

static struct nand_flash_dev *
nand_find_id(unsigned char id) {
	int i;

	for (i = 0; i < ARRAY_SIZE(nand_flash_ids); i++)
		if (nand_flash_ids[i].model_id == id)
			return &(nand_flash_ids[i]);
	return NULL;
}

static unsigned char parity[256];
static unsigned char ecc2[256];

static void nand_init_ecc(void) {
	int i, j, a;

	parity[0] = 0;
	for (i = 1; i < 256; i++)
		parity[i] = (parity[i&(i-1)] ^ 1);

	for (i = 0; i < 256; i++) {
		a = 0;
		for (j = 0; j < 8; j++) {
			if (i & (1<<j)) {
				if ((j & 1) == 0)
					a ^= 0x04;
				if ((j & 2) == 0)
					a ^= 0x10;
				if ((j & 4) == 0)
					a ^= 0x40;
			}
		}
		ecc2[i] = ~(a ^ (a<<1) ^ (parity[i] ? 0xa8 : 0));
	}
}

static void nand_compute_ecc(unsigned char *data, unsigned char *ecc) {
	int i, j, a;
	unsigned char par, bit, bits[8];

	par = 0;
	for (j = 0; j < 8; j++)
		bits[j] = 0;

	
	for (i = 0; i < 256; i++) {
		par ^= data[i];
		bit = parity[data[i]];
		for (j = 0; j < 8; j++)
			if ((i & (1<<j)) == 0)
				bits[j] ^= bit;
	}

	
	a = (bits[3] << 6) + (bits[2] << 4) + (bits[1] << 2) + bits[0];
	ecc[0] = ~(a ^ (a<<1) ^ (parity[par] ? 0xaa : 0));

	a = (bits[7] << 6) + (bits[6] << 4) + (bits[5] << 2) + bits[4];
	ecc[1] = ~(a ^ (a<<1) ^ (parity[par] ? 0xaa : 0));

	ecc[2] = ecc2[par];
}

static int nand_compare_ecc(unsigned char *data, unsigned char *ecc) {
	return (data[0] == ecc[0] && data[1] == ecc[1] && data[2] == ecc[2]);
}

static void nand_store_ecc(unsigned char *data, unsigned char *ecc) {
	memcpy(data, ecc, 3);
}


struct sddr09_card_info {
	unsigned long	capacity;	
	int		pagesize;	
	int		pageshift;	
	int		blocksize;	
	int		blockshift;	
	int		blockmask;	
	int		*lba_to_pba;	
	int		*pba_to_lba;	
	int		lbact;		
	int		flags;
#define	SDDR09_WP	1		
};

#define CONTROL_SHIFT 6

#define LUN	1
#define	LUNBITS	(LUN << 5)

#define UNDEF    0xffffffff
#define SPARE    0xfffffffe
#define UNUSABLE 0xfffffffd

static const int erase_bad_lba_entries = 0;

static int
sddr09_send_command(struct us_data *us,
		    unsigned char request,
		    unsigned char direction,
		    unsigned char *xfer_data,
		    unsigned int xfer_len) {
	unsigned int pipe;
	unsigned char requesttype = (0x41 | direction);
	int rc;

	

	if (direction == USB_DIR_IN)
		pipe = us->recv_ctrl_pipe;
	else
		pipe = us->send_ctrl_pipe;

	rc = usb_stor_ctrl_transfer(us, pipe, request, requesttype,
				   0, 0, xfer_data, xfer_len);
	switch (rc) {
		case USB_STOR_XFER_GOOD:	return 0;
		case USB_STOR_XFER_STALLED:	return -EPIPE;
		default:			return -EIO;
	}
}

static int
sddr09_send_scsi_command(struct us_data *us,
			 unsigned char *command,
			 unsigned int command_len) {
	return sddr09_send_command(us, 0, USB_DIR_OUT, command, command_len);
}

#if 0
static int
sddr09_test_unit_ready(struct us_data *us) {
	unsigned char *command = us->iobuf;
	int result;

	memset(command, 0, 6);
	command[1] = LUNBITS;

	result = sddr09_send_scsi_command(us, command, 6);

	US_DEBUGP("sddr09_test_unit_ready returns %d\n", result);

	return result;
}
#endif

static int
sddr09_request_sense(struct us_data *us, unsigned char *sensebuf, int buflen) {
	unsigned char *command = us->iobuf;
	int result;

	memset(command, 0, 12);
	command[0] = 0x03;
	command[1] = LUNBITS;
	command[4] = buflen;

	result = sddr09_send_scsi_command(us, command, 12);
	if (result)
		return result;

	result = usb_stor_bulk_transfer_buf(us, us->recv_bulk_pipe,
			sensebuf, buflen, NULL);
	return (result == USB_STOR_XFER_GOOD ? 0 : -EIO);
}


static int
sddr09_readX(struct us_data *us, int x, unsigned long fromaddress,
	     int nr_of_pages, int bulklen, unsigned char *buf,
	     int use_sg) {

	unsigned char *command = us->iobuf;
	int result;

	command[0] = 0xE8;
	command[1] = LUNBITS | x;
	command[2] = MSB_of(fromaddress>>16);
	command[3] = LSB_of(fromaddress>>16); 
	command[4] = MSB_of(fromaddress & 0xFFFF);
	command[5] = LSB_of(fromaddress & 0xFFFF); 
	command[6] = 0;
	command[7] = 0;
	command[8] = 0;
	command[9] = 0;
	command[10] = MSB_of(nr_of_pages);
	command[11] = LSB_of(nr_of_pages);

	result = sddr09_send_scsi_command(us, command, 12);

	if (result) {
		US_DEBUGP("Result for send_control in sddr09_read2%d %d\n",
			  x, result);
		return result;
	}

	result = usb_stor_bulk_transfer_sg(us, us->recv_bulk_pipe,
				       buf, bulklen, use_sg, NULL);

	if (result != USB_STOR_XFER_GOOD) {
		US_DEBUGP("Result for bulk_transfer in sddr09_read2%d %d\n",
			  x, result);
		return -EIO;
	}
	return 0;
}

static int
sddr09_read20(struct us_data *us, unsigned long fromaddress,
	      int nr_of_pages, int pageshift, unsigned char *buf, int use_sg) {
	int bulklen = nr_of_pages << pageshift;

	
	return sddr09_readX(us, 0, fromaddress, nr_of_pages, bulklen,
			    buf, use_sg);
}

static int
sddr09_read21(struct us_data *us, unsigned long fromaddress,
	      int count, int controlshift, unsigned char *buf, int use_sg) {

	int bulklen = (count << controlshift);
	return sddr09_readX(us, 1, fromaddress, count, bulklen,
			    buf, use_sg);
}

static int
sddr09_read22(struct us_data *us, unsigned long fromaddress,
	      int nr_of_pages, int pageshift, unsigned char *buf, int use_sg) {

	int bulklen = (nr_of_pages << pageshift) + (nr_of_pages << CONTROL_SHIFT);
	US_DEBUGP("sddr09_read22: reading %d pages, %d bytes\n",
		  nr_of_pages, bulklen);
	return sddr09_readX(us, 2, fromaddress, nr_of_pages, bulklen,
			    buf, use_sg);
}

#if 0
static int
sddr09_read23(struct us_data *us, unsigned long fromaddress,
	      int count, int controlshift, unsigned char *buf, int use_sg) {

	int bulklen = (count << controlshift);
	return sddr09_readX(us, 3, fromaddress, count, bulklen,
			    buf, use_sg);
}
#endif

static int
sddr09_erase(struct us_data *us, unsigned long Eaddress) {
	unsigned char *command = us->iobuf;
	int result;

	US_DEBUGP("sddr09_erase: erase address %lu\n", Eaddress);

	memset(command, 0, 12);
	command[0] = 0xEA;
	command[1] = LUNBITS;
	command[6] = MSB_of(Eaddress>>16);
	command[7] = LSB_of(Eaddress>>16);
	command[8] = MSB_of(Eaddress & 0xFFFF);
	command[9] = LSB_of(Eaddress & 0xFFFF);

	result = sddr09_send_scsi_command(us, command, 12);

	if (result)
		US_DEBUGP("Result for send_control in sddr09_erase %d\n",
			  result);

	return result;
}


static int
sddr09_writeX(struct us_data *us,
	      unsigned long Waddress, unsigned long Eaddress,
	      int nr_of_pages, int bulklen, unsigned char *buf, int use_sg) {

	unsigned char *command = us->iobuf;
	int result;

	command[0] = 0xE9;
	command[1] = LUNBITS;

	command[2] = MSB_of(Waddress>>16);
	command[3] = LSB_of(Waddress>>16);
	command[4] = MSB_of(Waddress & 0xFFFF);
	command[5] = LSB_of(Waddress & 0xFFFF);

	command[6] = MSB_of(Eaddress>>16);
	command[7] = LSB_of(Eaddress>>16);
	command[8] = MSB_of(Eaddress & 0xFFFF);
	command[9] = LSB_of(Eaddress & 0xFFFF);

	command[10] = MSB_of(nr_of_pages);
	command[11] = LSB_of(nr_of_pages);

	result = sddr09_send_scsi_command(us, command, 12);

	if (result) {
		US_DEBUGP("Result for send_control in sddr09_writeX %d\n",
			  result);
		return result;
	}

	result = usb_stor_bulk_transfer_sg(us, us->send_bulk_pipe,
				       buf, bulklen, use_sg, NULL);

	if (result != USB_STOR_XFER_GOOD) {
		US_DEBUGP("Result for bulk_transfer in sddr09_writeX %d\n",
			  result);
		return -EIO;
	}
	return 0;
}

static int
sddr09_write_inplace(struct us_data *us, unsigned long address,
		     int nr_of_pages, int pageshift, unsigned char *buf,
		     int use_sg) {
	int bulklen = (nr_of_pages << pageshift) + (nr_of_pages << CONTROL_SHIFT);
	return sddr09_writeX(us, address, address, nr_of_pages, bulklen,
			     buf, use_sg);
}

#if 0
static int
sddr09_read_sg_test_only(struct us_data *us) {
	unsigned char *command = us->iobuf;
	int result, bulklen, nsg, ct;
	unsigned char *buf;
	unsigned long address;

	nsg = bulklen = 0;
	command[0] = 0xE7;
	command[1] = LUNBITS;
	command[2] = 0;
	address = 040000; ct = 1;
	nsg++;
	bulklen += (ct << 9);
	command[4*nsg+2] = ct;
	command[4*nsg+1] = ((address >> 9) & 0xFF);
	command[4*nsg+0] = ((address >> 17) & 0xFF);
	command[4*nsg-1] = ((address >> 25) & 0xFF);

	address = 0340000; ct = 1;
	nsg++;
	bulklen += (ct << 9);
	command[4*nsg+2] = ct;
	command[4*nsg+1] = ((address >> 9) & 0xFF);
	command[4*nsg+0] = ((address >> 17) & 0xFF);
	command[4*nsg-1] = ((address >> 25) & 0xFF);

	address = 01000000; ct = 2;
	nsg++;
	bulklen += (ct << 9);
	command[4*nsg+2] = ct;
	command[4*nsg+1] = ((address >> 9) & 0xFF);
	command[4*nsg+0] = ((address >> 17) & 0xFF);
	command[4*nsg-1] = ((address >> 25) & 0xFF);

	command[2] = nsg;

	result = sddr09_send_scsi_command(us, command, 4*nsg+3);

	if (result) {
		US_DEBUGP("Result for send_control in sddr09_read_sg %d\n",
			  result);
		return result;
	}

	buf = kmalloc(bulklen, GFP_NOIO);
	if (!buf)
		return -ENOMEM;

	result = usb_stor_bulk_transfer_buf(us, us->recv_bulk_pipe,
				       buf, bulklen, NULL);
	kfree(buf);
	if (result != USB_STOR_XFER_GOOD) {
		US_DEBUGP("Result for bulk_transfer in sddr09_read_sg %d\n",
			  result);
		return -EIO;
	}

	return 0;
}
#endif


static int
sddr09_read_status(struct us_data *us, unsigned char *status) {

	unsigned char *command = us->iobuf;
	unsigned char *data = us->iobuf;
	int result;

	US_DEBUGP("Reading status...\n");

	memset(command, 0, 12);
	command[0] = 0xEC;
	command[1] = LUNBITS;

	result = sddr09_send_scsi_command(us, command, 12);
	if (result)
		return result;

	result = usb_stor_bulk_transfer_buf(us, us->recv_bulk_pipe,
				       data, 64, NULL);
	*status = data[0];
	return (result == USB_STOR_XFER_GOOD ? 0 : -EIO);
}

static int
sddr09_read_data(struct us_data *us,
		 unsigned long address,
		 unsigned int sectors) {

	struct sddr09_card_info *info = (struct sddr09_card_info *) us->extra;
	unsigned char *buffer;
	unsigned int lba, maxlba, pba;
	unsigned int page, pages;
	unsigned int len, offset;
	struct scatterlist *sg;
	int result;

	
	lba = address >> info->blockshift;
	page = (address & info->blockmask);
	maxlba = info->capacity >> (info->pageshift + info->blockshift);
	if (lba >= maxlba)
		return -EIO;

	
	
	

	len = min(sectors, (unsigned int) info->blocksize) * info->pagesize;
	buffer = kmalloc(len, GFP_NOIO);
	if (buffer == NULL) {
		printk(KERN_WARNING "sddr09_read_data: Out of memory\n");
		return -ENOMEM;
	}

	
	

	result = 0;
	offset = 0;
	sg = NULL;

	while (sectors > 0) {

		
		pages = min(sectors, info->blocksize - page);
		len = pages << info->pageshift;

		
		if (lba >= maxlba) {
			US_DEBUGP("Error: Requested lba %u exceeds "
				  "maximum %u\n", lba, maxlba);
			result = -EIO;
			break;
		}

		
		pba = info->lba_to_pba[lba];

		if (pba == UNDEF) {	/* this lba was never written */

			US_DEBUGP("Read %d zero pages (LBA %d) page %d\n",
				  pages, lba, page);

			/* This is not really an error. It just means
			   that the block has never been written.
			   Instead of returning an error
			   it is better to return all zero data. */

			memset(buffer, 0, len);

		} else {
			US_DEBUGP("Read %d pages, from PBA %d"
				  " (LBA %d) page %d\n",
				  pages, pba, lba, page);

			address = ((pba << info->blockshift) + page) << 
				info->pageshift;

			result = sddr09_read20(us, address>>1,
					pages, info->pageshift, buffer, 0);
			if (result)
				break;
		}

		
		usb_stor_access_xfer_buf(buffer, len, us->srb,
				&sg, &offset, TO_XFER_BUF);

		page = 0;
		lba++;
		sectors -= pages;
	}

	kfree(buffer);
	return result;
}

static unsigned int
sddr09_find_unused_pba(struct sddr09_card_info *info, unsigned int lba) {
	static unsigned int lastpba = 1;
	int zonestart, end, i;

	zonestart = (lba/1000) << 10;
	end = info->capacity >> (info->blockshift + info->pageshift);
	end -= zonestart;
	if (end > 1024)
		end = 1024;

	for (i = lastpba+1; i < end; i++) {
		if (info->pba_to_lba[zonestart+i] == UNDEF) {
			lastpba = i;
			return zonestart+i;
		}
	}
	for (i = 0; i <= lastpba; i++) {
		if (info->pba_to_lba[zonestart+i] == UNDEF) {
			lastpba = i;
			return zonestart+i;
		}
	}
	return 0;
}

static int
sddr09_write_lba(struct us_data *us, unsigned int lba,
		 unsigned int page, unsigned int pages,
		 unsigned char *ptr, unsigned char *blockbuffer) {

	struct sddr09_card_info *info = (struct sddr09_card_info *) us->extra;
	unsigned long address;
	unsigned int pba, lbap;
	unsigned int pagelen;
	unsigned char *bptr, *cptr, *xptr;
	unsigned char ecc[3];
	int i, result, isnew;

	lbap = ((lba % 1000) << 1) | 0x1000;
	if (parity[MSB_of(lbap) ^ LSB_of(lbap)])
		lbap ^= 1;
	pba = info->lba_to_pba[lba];
	isnew = 0;

	if (pba == UNDEF) {
		pba = sddr09_find_unused_pba(info, lba);
		if (!pba) {
			printk(KERN_WARNING
			       "sddr09_write_lba: Out of unused blocks\n");
			return -ENOSPC;
		}
		info->pba_to_lba[pba] = lba;
		info->lba_to_pba[lba] = pba;
		isnew = 1;
	}

	if (pba == 1) {
		printk(KERN_WARNING "sddr09: avoid writing to pba 1\n");
		return 0;
	}

	pagelen = (1 << info->pageshift) + (1 << CONTROL_SHIFT);

	
	address = (pba << (info->pageshift + info->blockshift));
	result = sddr09_read22(us, address>>1, info->blocksize,
			       info->pageshift, blockbuffer, 0);
	if (result)
		return result;

	
	for (i = 0; i < info->blocksize; i++) {
		bptr = blockbuffer + i*pagelen;
		cptr = bptr + info->pagesize;
		nand_compute_ecc(bptr, ecc);
		if (!nand_compare_ecc(cptr+13, ecc)) {
			US_DEBUGP("Warning: bad ecc in page %d- of pba %d\n",
				  i, pba);
			nand_store_ecc(cptr+13, ecc);
		}
		nand_compute_ecc(bptr+(info->pagesize / 2), ecc);
		if (!nand_compare_ecc(cptr+8, ecc)) {
			US_DEBUGP("Warning: bad ecc in page %d+ of pba %d\n",
				  i, pba);
			nand_store_ecc(cptr+8, ecc);
		}
		cptr[6] = cptr[11] = MSB_of(lbap);
		cptr[7] = cptr[12] = LSB_of(lbap);
	}

	
	xptr = ptr;
	for (i = page; i < page+pages; i++) {
		bptr = blockbuffer + i*pagelen;
		cptr = bptr + info->pagesize;
		memcpy(bptr, xptr, info->pagesize);
		xptr += info->pagesize;
		nand_compute_ecc(bptr, ecc);
		nand_store_ecc(cptr+13, ecc);
		nand_compute_ecc(bptr+(info->pagesize / 2), ecc);
		nand_store_ecc(cptr+8, ecc);
	}

	US_DEBUGP("Rewrite PBA %d (LBA %d)\n", pba, lba);

	result = sddr09_write_inplace(us, address>>1, info->blocksize,
				      info->pageshift, blockbuffer, 0);

	US_DEBUGP("sddr09_write_inplace returns %d\n", result);

#if 0
	{
		unsigned char status = 0;
		int result2 = sddr09_read_status(us, &status);
		if (result2)
			US_DEBUGP("sddr09_write_inplace: cannot read status\n");
		else if (status != 0xc0)
			US_DEBUGP("sddr09_write_inplace: status after write: 0x%x\n",
				  status);
	}
#endif

#if 0
	{
		int result2 = sddr09_test_unit_ready(us);
	}
#endif

	return result;
}

static int
sddr09_write_data(struct us_data *us,
		  unsigned long address,
		  unsigned int sectors) {

	struct sddr09_card_info *info = (struct sddr09_card_info *) us->extra;
	unsigned int lba, maxlba, page, pages;
	unsigned int pagelen, blocklen;
	unsigned char *blockbuffer;
	unsigned char *buffer;
	unsigned int len, offset;
	struct scatterlist *sg;
	int result;

	
	lba = address >> info->blockshift;
	page = (address & info->blockmask);
	maxlba = info->capacity >> (info->pageshift + info->blockshift);
	if (lba >= maxlba)
		return -EIO;

	
	


	pagelen = (1 << info->pageshift) + (1 << CONTROL_SHIFT);
	blocklen = (pagelen << info->blockshift);
	blockbuffer = kmalloc(blocklen, GFP_NOIO);
	if (!blockbuffer) {
		printk(KERN_WARNING "sddr09_write_data: Out of memory\n");
		return -ENOMEM;
	}

	
	
	

	len = min(sectors, (unsigned int) info->blocksize) * info->pagesize;
	buffer = kmalloc(len, GFP_NOIO);
	if (buffer == NULL) {
		printk(KERN_WARNING "sddr09_write_data: Out of memory\n");
		kfree(blockbuffer);
		return -ENOMEM;
	}

	result = 0;
	offset = 0;
	sg = NULL;

	while (sectors > 0) {

		

		pages = min(sectors, info->blocksize - page);
		len = (pages << info->pageshift);

		
		if (lba >= maxlba) {
			US_DEBUGP("Error: Requested lba %u exceeds "
				  "maximum %u\n", lba, maxlba);
			result = -EIO;
			break;
		}

		
		usb_stor_access_xfer_buf(buffer, len, us->srb,
				&sg, &offset, FROM_XFER_BUF);

		result = sddr09_write_lba(us, lba, page, pages,
				buffer, blockbuffer);
		if (result)
			break;

		page = 0;
		lba++;
		sectors -= pages;
	}

	kfree(buffer);
	kfree(blockbuffer);

	return result;
}

static int
sddr09_read_control(struct us_data *us,
		unsigned long address,
		unsigned int blocks,
		unsigned char *content,
		int use_sg) {

	US_DEBUGP("Read control address %lu, blocks %d\n",
		address, blocks);

	return sddr09_read21(us, address, blocks,
			     CONTROL_SHIFT, content, use_sg);
}

static int
sddr09_read_deviceID(struct us_data *us, unsigned char *deviceID) {
	unsigned char *command = us->iobuf;
	unsigned char *content = us->iobuf;
	int result, i;

	memset(command, 0, 12);
	command[0] = 0xED;
	command[1] = LUNBITS;

	result = sddr09_send_scsi_command(us, command, 12);
	if (result)
		return result;

	result = usb_stor_bulk_transfer_buf(us, us->recv_bulk_pipe,
			content, 64, NULL);

	for (i = 0; i < 4; i++)
		deviceID[i] = content[i];

	return (result == USB_STOR_XFER_GOOD ? 0 : -EIO);
}

static int
sddr09_get_wp(struct us_data *us, struct sddr09_card_info *info) {
	int result;
	unsigned char status;

	result = sddr09_read_status(us, &status);
	if (result) {
		US_DEBUGP("sddr09_get_wp: read_status fails\n");
		return result;
	}
	US_DEBUGP("sddr09_get_wp: status 0x%02X", status);
	if ((status & 0x80) == 0) {
		info->flags |= SDDR09_WP;	
		US_DEBUGP(" WP");
	}
	if (status & 0x40)
		US_DEBUGP(" Ready");
	if (status & LUNBITS)
		US_DEBUGP(" Suspended");
	if (status & 0x1)
		US_DEBUGP(" Error");
	US_DEBUGP("\n");
	return 0;
}

#if 0
static int
sddr09_reset(struct us_data *us) {

	unsigned char *command = us->iobuf;

	memset(command, 0, 12);
	command[0] = 0xEB;
	command[1] = LUNBITS;

	return sddr09_send_scsi_command(us, command, 12);
}
#endif

static struct nand_flash_dev *
sddr09_get_cardinfo(struct us_data *us, unsigned char flags) {
	struct nand_flash_dev *cardinfo;
	unsigned char deviceID[4];
	char blurbtxt[256];
	int result;

	US_DEBUGP("Reading capacity...\n");

	result = sddr09_read_deviceID(us, deviceID);

	if (result) {
		US_DEBUGP("Result of read_deviceID is %d\n", result);
		printk(KERN_WARNING "sddr09: could not read card info\n");
		return NULL;
	}

	sprintf(blurbtxt, "sddr09: Found Flash card, ID = %02X %02X %02X %02X",
		deviceID[0], deviceID[1], deviceID[2], deviceID[3]);

	
	sprintf(blurbtxt + strlen(blurbtxt),
		": Manuf. %s",
		nand_flash_manufacturer(deviceID[0]));

	
	cardinfo = nand_find_id(deviceID[1]);
	if (cardinfo) {
		sprintf(blurbtxt + strlen(blurbtxt),
			", %d MB", 1<<(cardinfo->chipshift - 20));
	} else {
		sprintf(blurbtxt + strlen(blurbtxt),
			", type unrecognized");
	}

	
	if (deviceID[2] == 0xa5) {
		sprintf(blurbtxt + strlen(blurbtxt),
			", 128-bit ID");
	}

	
	if (deviceID[3] == 0xc0) {
		sprintf(blurbtxt + strlen(blurbtxt),
			", extra cmd");
	}

	if (flags & SDDR09_WP)
		sprintf(blurbtxt + strlen(blurbtxt),
			", WP");

	printk(KERN_WARNING "%s\n", blurbtxt);

	return cardinfo;
}

static int
sddr09_read_map(struct us_data *us) {

	struct sddr09_card_info *info = (struct sddr09_card_info *) us->extra;
	int numblocks, alloc_len, alloc_blocks;
	int i, j, result;
	unsigned char *buffer, *buffer_end, *ptr;
	unsigned int lba, lbact;

	if (!info->capacity)
		return -1;

	
	

	numblocks = info->capacity >> (info->blockshift + info->pageshift);

	
	
	
#define SDDR09_READ_MAP_BUFSZ 65536

	alloc_blocks = min(numblocks, SDDR09_READ_MAP_BUFSZ >> CONTROL_SHIFT);
	alloc_len = (alloc_blocks << CONTROL_SHIFT);
	buffer = kmalloc(alloc_len, GFP_NOIO);
	if (buffer == NULL) {
		printk(KERN_WARNING "sddr09_read_map: out of memory\n");
		result = -1;
		goto done;
	}
	buffer_end = buffer + alloc_len;

#undef SDDR09_READ_MAP_BUFSZ

	kfree(info->lba_to_pba);
	kfree(info->pba_to_lba);
	info->lba_to_pba = kmalloc(numblocks*sizeof(int), GFP_NOIO);
	info->pba_to_lba = kmalloc(numblocks*sizeof(int), GFP_NOIO);

	if (info->lba_to_pba == NULL || info->pba_to_lba == NULL) {
		printk(KERN_WARNING "sddr09_read_map: out of memory\n");
		result = -1;
		goto done;
	}

	for (i = 0; i < numblocks; i++)
		info->lba_to_pba[i] = info->pba_to_lba[i] = UNDEF;


	ptr = buffer_end;
	for (i = 0; i < numblocks; i++) {
		ptr += (1 << CONTROL_SHIFT);
		if (ptr >= buffer_end) {
			unsigned long address;

			address = i << (info->pageshift + info->blockshift);
			result = sddr09_read_control(
				us, address>>1,
				min(alloc_blocks, numblocks - i),
				buffer, 0);
			if (result) {
				result = -1;
				goto done;
			}
			ptr = buffer;
		}

		if (i == 0 || i == 1) {
			info->pba_to_lba[i] = UNUSABLE;
			continue;
		}

		
		for (j = 0; j < 16; j++)
			if (ptr[j] != 0)
				goto nonz;
		info->pba_to_lba[i] = UNUSABLE;
		printk(KERN_WARNING "sddr09: PBA %d has no logical mapping\n",
		       i);
		continue;

	nonz:
		/* unwritten PBAs have control field FF^16 */
		for (j = 0; j < 16; j++)
			if (ptr[j] != 0xff)
				goto nonff;
		continue;

	nonff:
		
		if (j < 6) {
			printk(KERN_WARNING
			       "sddr09: PBA %d has no logical mapping: "
			       "reserved area = %02X%02X%02X%02X "
			       "data status %02X block status %02X\n",
			       i, ptr[0], ptr[1], ptr[2], ptr[3],
			       ptr[4], ptr[5]);
			info->pba_to_lba[i] = UNUSABLE;
			continue;
		}

		if ((ptr[6] >> 4) != 0x01) {
			printk(KERN_WARNING
			       "sddr09: PBA %d has invalid address field "
			       "%02X%02X/%02X%02X\n",
			       i, ptr[6], ptr[7], ptr[11], ptr[12]);
			info->pba_to_lba[i] = UNUSABLE;
			continue;
		}

		
		if (parity[ptr[6] ^ ptr[7]]) {
			printk(KERN_WARNING
			       "sddr09: Bad parity in LBA for block %d"
			       " (%02X %02X)\n", i, ptr[6], ptr[7]);
			info->pba_to_lba[i] = UNUSABLE;
			continue;
		}

		lba = short_pack(ptr[7], ptr[6]);
		lba = (lba & 0x07FF) >> 1;


		if (lba >= 1000) {
			printk(KERN_WARNING
			       "sddr09: Bad low LBA %d for block %d\n",
			       lba, i);
			goto possibly_erase;
		}

		lba += 1000*(i/0x400);

		if (info->lba_to_pba[lba] != UNDEF) {
			printk(KERN_WARNING
			       "sddr09: LBA %d seen for PBA %d and %d\n",
			       lba, info->lba_to_pba[lba], i);
			goto possibly_erase;
		}

		info->pba_to_lba[i] = lba;
		info->lba_to_pba[lba] = i;
		continue;

	possibly_erase:
		if (erase_bad_lba_entries) {
			unsigned long address;

			address = (i << (info->pageshift + info->blockshift));
			sddr09_erase(us, address>>1);
			info->pba_to_lba[i] = UNDEF;
		} else
			info->pba_to_lba[i] = UNUSABLE;
	}

	lbact = 0;
	for (i = 0; i < numblocks; i += 1024) {
		int ct = 0;

		for (j = 0; j < 1024 && i+j < numblocks; j++) {
			if (info->pba_to_lba[i+j] != UNUSABLE) {
				if (ct >= 1000)
					info->pba_to_lba[i+j] = SPARE;
				else
					ct++;
			}
		}
		lbact += ct;
	}
	info->lbact = lbact;
	US_DEBUGP("Found %d LBA's\n", lbact);
	result = 0;

 done:
	if (result != 0) {
		kfree(info->lba_to_pba);
		kfree(info->pba_to_lba);
		info->lba_to_pba = NULL;
		info->pba_to_lba = NULL;
	}
	kfree(buffer);
	return result;
}

static void
sddr09_card_info_destructor(void *extra) {
	struct sddr09_card_info *info = (struct sddr09_card_info *)extra;

	if (!info)
		return;

	kfree(info->lba_to_pba);
	kfree(info->pba_to_lba);
}

static int
sddr09_common_init(struct us_data *us) {
	int result;

	
	if (us->pusb_dev->actconfig->desc.bConfigurationValue != 1) {
		US_DEBUGP("active config #%d != 1 ??\n", us->pusb_dev
				->actconfig->desc.bConfigurationValue);
		return -EINVAL;
	}

	result = usb_reset_configuration(us->pusb_dev);
	US_DEBUGP("Result of usb_reset_configuration is %d\n", result);
	if (result == -EPIPE) {
		US_DEBUGP("-- stall on control interface\n");
	} else if (result != 0) {
		
		US_DEBUGP("-- Unknown error.  Rejecting device\n");
		return -EINVAL;
	}

	us->extra = kzalloc(sizeof(struct sddr09_card_info), GFP_NOIO);
	if (!us->extra)
		return -ENOMEM;
	us->extra_destructor = sddr09_card_info_destructor;

	nand_init_ecc();
	return 0;
}


static int
usb_stor_sddr09_dpcm_init(struct us_data *us) {
	int result;
	unsigned char *data = us->iobuf;

	result = sddr09_common_init(us);
	if (result)
		return result;

	result = sddr09_send_command(us, 0x01, USB_DIR_IN, data, 2);
	if (result) {
		US_DEBUGP("sddr09_init: send_command fails\n");
		return result;
	}

	US_DEBUGP("SDDR09init: %02X %02X\n", data[0], data[1]);
	

	result = sddr09_send_command(us, 0x08, USB_DIR_IN, data, 2);
	if (result) {
		US_DEBUGP("sddr09_init: 2nd send_command fails\n");
		return result;
	}

	US_DEBUGP("SDDR09init: %02X %02X\n", data[0], data[1]);
	

	result = sddr09_request_sense(us, data, 18);
	if (result == 0 && data[2] != 0) {
		int j;
		for (j=0; j<18; j++)
			printk(" %02X", data[j]);
		printk("\n");
		
		
		
		
		
		
		
	}

	

	return 0;		
}

static int dpcm_transport(struct scsi_cmnd *srb, struct us_data *us)
{
	int ret;

	US_DEBUGP("dpcm_transport: LUN=%d\n", srb->device->lun);

	switch (srb->device->lun) {
	case 0:

		ret = usb_stor_CB_transport(srb, us);
		break;

	case 1:


		srb->device->lun = 0;
		ret = sddr09_transport(srb, us);
		srb->device->lun = 1;
		break;

	default:
		US_DEBUGP("dpcm_transport: Invalid LUN %d\n",
				srb->device->lun);
		ret = USB_STOR_TRANSPORT_ERROR;
		break;
	}
	return ret;
}


static int sddr09_transport(struct scsi_cmnd *srb, struct us_data *us)
{
	static unsigned char sensekey = 0, sensecode = 0;
	static unsigned char havefakesense = 0;
	int result, i;
	unsigned char *ptr = us->iobuf;
	unsigned long capacity;
	unsigned int page, pages;

	struct sddr09_card_info *info;

	static unsigned char inquiry_response[8] = {
		0x00, 0x80, 0x00, 0x02, 0x1F, 0x00, 0x00, 0x00
	};

	
	static unsigned char mode_page_01[19] = {
		0x00, 0x0F, 0x00, 0x0, 0x0, 0x0, 0x00,
		0x01, 0x0A,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	info = (struct sddr09_card_info *)us->extra;

	if (srb->cmnd[0] == REQUEST_SENSE && havefakesense) {
		
		memset(ptr, 0, 18);
		ptr[0] = 0x70;
		ptr[2] = sensekey;
		ptr[7] = 11;
		ptr[12] = sensecode;
		usb_stor_set_xfer_buf(ptr, 18, srb);
		sensekey = sensecode = havefakesense = 0;
		return USB_STOR_TRANSPORT_GOOD;
	}

	havefakesense = 1;


	if (srb->cmnd[0] == INQUIRY) {
		memcpy(ptr, inquiry_response, 8);
		fill_inquiry_response(us, ptr, 36);
		return USB_STOR_TRANSPORT_GOOD;
	}

	if (srb->cmnd[0] == READ_CAPACITY) {
		struct nand_flash_dev *cardinfo;

		sddr09_get_wp(us, info);	

		cardinfo = sddr09_get_cardinfo(us, info->flags);
		if (!cardinfo) {
			
		init_error:
			sensekey = 0x02;	
			sensecode = 0x3a;	
			return USB_STOR_TRANSPORT_FAILED;
		}

		info->capacity = (1 << cardinfo->chipshift);
		info->pageshift = cardinfo->pageshift;
		info->pagesize = (1 << info->pageshift);
		info->blockshift = cardinfo->blockshift;
		info->blocksize = (1 << info->blockshift);
		info->blockmask = info->blocksize - 1;

		
		if (sddr09_read_map(us)) {
			
			goto init_error;
		}

		

		capacity = (info->lbact << info->blockshift) - 1;

		((__be32 *) ptr)[0] = cpu_to_be32(capacity);

		

		((__be32 *) ptr)[1] = cpu_to_be32(info->pagesize);
		usb_stor_set_xfer_buf(ptr, 8, srb);

		return USB_STOR_TRANSPORT_GOOD;
	}

	if (srb->cmnd[0] == MODE_SENSE_10) {
		int modepage = (srb->cmnd[2] & 0x3F);

		
		if (modepage == 0x01 || modepage == 0x3F) {
			US_DEBUGP("SDDR09: Dummy up request for "
				  "mode page 0x%x\n", modepage);

			memcpy(ptr, mode_page_01, sizeof(mode_page_01));
			((__be16*)ptr)[0] = cpu_to_be16(sizeof(mode_page_01) - 2);
			ptr[3] = (info->flags & SDDR09_WP) ? 0x80 : 0;
			usb_stor_set_xfer_buf(ptr, sizeof(mode_page_01), srb);
			return USB_STOR_TRANSPORT_GOOD;
		}

		sensekey = 0x05;	
		sensecode = 0x24;	
		return USB_STOR_TRANSPORT_FAILED;
	}

	if (srb->cmnd[0] == ALLOW_MEDIUM_REMOVAL)
		return USB_STOR_TRANSPORT_GOOD;

	havefakesense = 0;

	if (srb->cmnd[0] == READ_10) {

		page = short_pack(srb->cmnd[3], srb->cmnd[2]);
		page <<= 16;
		page |= short_pack(srb->cmnd[5], srb->cmnd[4]);
		pages = short_pack(srb->cmnd[8], srb->cmnd[7]);

		US_DEBUGP("READ_10: read page %d pagect %d\n",
			  page, pages);

		result = sddr09_read_data(us, page, pages);
		return (result == 0 ? USB_STOR_TRANSPORT_GOOD :
				USB_STOR_TRANSPORT_ERROR);
	}

	if (srb->cmnd[0] == WRITE_10) {

		page = short_pack(srb->cmnd[3], srb->cmnd[2]);
		page <<= 16;
		page |= short_pack(srb->cmnd[5], srb->cmnd[4]);
		pages = short_pack(srb->cmnd[8], srb->cmnd[7]);

		US_DEBUGP("WRITE_10: write page %d pagect %d\n",
			  page, pages);

		result = sddr09_write_data(us, page, pages);
		return (result == 0 ? USB_STOR_TRANSPORT_GOOD :
				USB_STOR_TRANSPORT_ERROR);
	}

	if (srb->cmnd[0] != TEST_UNIT_READY &&
	    srb->cmnd[0] != REQUEST_SENSE) {
		sensekey = 0x05;	
		sensecode = 0x20;	
		havefakesense = 1;
		return USB_STOR_TRANSPORT_FAILED;
	}

	for (; srb->cmd_len<12; srb->cmd_len++)
		srb->cmnd[srb->cmd_len] = 0;

	srb->cmnd[1] = LUNBITS;

	ptr[0] = 0;
	for (i=0; i<12; i++)
		sprintf(ptr+strlen(ptr), "%02X ", srb->cmnd[i]);

	US_DEBUGP("SDDR09: Send control for command %s\n", ptr);

	result = sddr09_send_scsi_command(us, srb->cmnd, 12);
	if (result) {
		US_DEBUGP("sddr09_transport: sddr09_send_scsi_command "
			  "returns %d\n", result);
		return USB_STOR_TRANSPORT_ERROR;
	}

	if (scsi_bufflen(srb) == 0)
		return USB_STOR_TRANSPORT_GOOD;

	if (srb->sc_data_direction == DMA_TO_DEVICE ||
	    srb->sc_data_direction == DMA_FROM_DEVICE) {
		unsigned int pipe = (srb->sc_data_direction == DMA_TO_DEVICE)
				? us->send_bulk_pipe : us->recv_bulk_pipe;

		US_DEBUGP("SDDR09: %s %d bytes\n",
			  (srb->sc_data_direction == DMA_TO_DEVICE) ?
			  "sending" : "receiving",
			  scsi_bufflen(srb));

		result = usb_stor_bulk_srb(us, pipe, srb);

		return (result == USB_STOR_XFER_GOOD ?
			USB_STOR_TRANSPORT_GOOD : USB_STOR_TRANSPORT_ERROR);
	} 

	return USB_STOR_TRANSPORT_GOOD;
}

static int
usb_stor_sddr09_init(struct us_data *us) {
	return sddr09_common_init(us);
}

static int sddr09_probe(struct usb_interface *intf,
			 const struct usb_device_id *id)
{
	struct us_data *us;
	int result;

	result = usb_stor_probe1(&us, intf, id,
			(id - sddr09_usb_ids) + sddr09_unusual_dev_list);
	if (result)
		return result;

	if (us->protocol == USB_PR_DPCM_USB) {
		us->transport_name = "Control/Bulk-EUSB/SDDR09";
		us->transport = dpcm_transport;
		us->transport_reset = usb_stor_CB_reset;
		us->max_lun = 1;
	} else {
		us->transport_name = "EUSB/SDDR09";
		us->transport = sddr09_transport;
		us->transport_reset = usb_stor_CB_reset;
		us->max_lun = 0;
	}

	result = usb_stor_probe2(us);
	return result;
}

static struct usb_driver sddr09_driver = {
	.name =		"ums-sddr09",
	.probe =	sddr09_probe,
	.disconnect =	usb_stor_disconnect,
	.suspend =	usb_stor_suspend,
	.resume =	usb_stor_resume,
	.reset_resume =	usb_stor_reset_resume,
	.pre_reset =	usb_stor_pre_reset,
	.post_reset =	usb_stor_post_reset,
	.id_table =	sddr09_usb_ids,
	.soft_unbind =	1,
	.no_dynamic_id = 1,
};

module_usb_driver(sddr09_driver);
