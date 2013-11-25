/* Driver for SCM Microsystems (a.k.a. Shuttle) USB-ATAPI cable
 *
 * Current development and maintenance by:
 *   (c) 2000, 2001 Robert Baruch (autophile@starband.net)
 *   (c) 2004, 2005 Daniel Drake <dsd@gentoo.org>
 *
 * Developed with the assistance of:
 *   (c) 2002 Alan Stern <stern@rowland.org>
 *
 * Flash support based on earlier work by:
 *   (c) 2002 Thomas Kreiling <usbdev@sm04.de>
 *
 * Many originally ATAPI devices were slightly modified to meet the USB
 * market by using some kind of translation from ATAPI to USB on the host,
 * and the peripheral would translate from USB back to ATAPI.
 *
 * SCM Microsystems (www.scmmicro.com) makes a device, sold to OEM's only, 
 * which does the USB-to-ATAPI conversion.  By obtaining the data sheet on
 * their device under nondisclosure agreement, I have been able to write
 * this driver for Linux.
 *
 * The chip used in the device can also be used for EPP and ISA translation
 * as well. This driver is only guaranteed to work with the ATAPI
 * translation.
 *
 * See the Kconfig help text for a list of devices known to be supported by
 * this driver.
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
#include <linux/cdrom.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>

#include "usb.h"
#include "transport.h"
#include "protocol.h"
#include "debug.h"

MODULE_DESCRIPTION("Driver for SCM Microsystems (a.k.a. Shuttle) USB-ATAPI cable");
MODULE_AUTHOR("Daniel Drake <dsd@gentoo.org>, Robert Baruch <autophile@starband.net>");
MODULE_LICENSE("GPL");

#define USBAT_DEV_HP8200	0x01
#define USBAT_DEV_FLASH		0x02

#define USBAT_EPP_PORT		0x10
#define USBAT_EPP_REGISTER	0x30
#define USBAT_ATA		0x40
#define USBAT_ISA		0x50

#define USBAT_CMD_READ_REG		0x00
#define USBAT_CMD_WRITE_REG		0x01
#define USBAT_CMD_READ_BLOCK	0x02
#define USBAT_CMD_WRITE_BLOCK	0x03
#define USBAT_CMD_COND_READ_BLOCK	0x04
#define USBAT_CMD_COND_WRITE_BLOCK	0x05
#define USBAT_CMD_WRITE_REGS	0x07

#define USBAT_CMD_EXEC_CMD	0x80
#define USBAT_CMD_SET_FEAT	0x81
#define USBAT_CMD_UIO		0x82

#define USBAT_UIO_READ	1
#define USBAT_UIO_WRITE	0

#define USBAT_QUAL_FCQ	0x20	
#define USBAT_QUAL_ALQ	0x10	

#define USBAT_FLASH_MEDIA_NONE	0
#define USBAT_FLASH_MEDIA_CF	1

#define USBAT_FLASH_MEDIA_SAME	0
#define USBAT_FLASH_MEDIA_CHANGED	1

#define USBAT_ATA_DATA      0x10  
#define USBAT_ATA_FEATURES  0x11  
#define USBAT_ATA_ERROR     0x11  
#define USBAT_ATA_SECCNT    0x12  
#define USBAT_ATA_SECNUM    0x13  
#define USBAT_ATA_LBA_ME    0x14  
#define USBAT_ATA_LBA_HI    0x15  
#define USBAT_ATA_DEVICE    0x16  
#define USBAT_ATA_STATUS    0x17  
#define USBAT_ATA_CMD       0x17  
#define USBAT_ATA_ALTSTATUS 0x0E  

#define USBAT_UIO_EPAD		0x80 
#define USBAT_UIO_CDT		0x40 
				     
#define USBAT_UIO_1		0x20 
#define USBAT_UIO_0		0x10 
#define USBAT_UIO_EPP_ATA	0x08 
#define USBAT_UIO_UI1		0x04 
#define USBAT_UIO_UI0		0x02 
#define USBAT_UIO_INTR_ACK	0x01 

#define USBAT_UIO_DRVRST	0x80 
#define USBAT_UIO_ACKD		0x40 
#define USBAT_UIO_OE1		0x20 
				     
#define USBAT_UIO_OE0		0x10 
#define USBAT_UIO_ADPRST	0x01 

#define USBAT_FEAT_ETEN	0x80	
#define USBAT_FEAT_U1	0x08
#define USBAT_FEAT_U0	0x04
#define USBAT_FEAT_ET1	0x02
#define USBAT_FEAT_ET2	0x01

struct usbat_info {
	int devicetype;

	
	unsigned long sectors;     
	unsigned long ssize;       

	unsigned char sense_key;
	unsigned long sense_asc;   
	unsigned long sense_ascq;  
};

#define short_pack(LSB,MSB) ( ((u16)(LSB)) | ( ((u16)(MSB))<<8 ) )
#define LSB_of(s) ((s)&0xFF)
#define MSB_of(s) ((s)>>8)

static int transferred = 0;

static int usbat_flash_transport(struct scsi_cmnd * srb, struct us_data *us);
static int usbat_hp8200e_transport(struct scsi_cmnd *srb, struct us_data *us);

static int init_usbat_cd(struct us_data *us);
static int init_usbat_flash(struct us_data *us);


#define UNUSUAL_DEV(id_vendor, id_product, bcdDeviceMin, bcdDeviceMax, \
		    vendorName, productName, useProtocol, useTransport, \
		    initFunction, flags) \
{ USB_DEVICE_VER(id_vendor, id_product, bcdDeviceMin, bcdDeviceMax), \
  .driver_info = (flags)|(USB_US_TYPE_STOR<<24) }

static struct usb_device_id usbat_usb_ids[] = {
#	include "unusual_usbat.h"
	{ }		
};
MODULE_DEVICE_TABLE(usb, usbat_usb_ids);

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

static struct us_unusual_dev usbat_unusual_dev_list[] = {
#	include "unusual_usbat.h"
	{ }		
};

#undef UNUSUAL_DEV

static void usbat_pack_ata_sector_cmd(unsigned char *buf,
					unsigned char thistime,
					u32 sector, unsigned char cmd)
{
	buf[0] = 0;
	buf[1] = thistime;
	buf[2] = sector & 0xFF;
	buf[3] = (sector >>  8) & 0xFF;
	buf[4] = (sector >> 16) & 0xFF;
	buf[5] = 0xE0 | ((sector >> 24) & 0x0F);
	buf[6] = cmd;
}

static int usbat_get_device_type(struct us_data *us)
{
	return ((struct usbat_info*)us->extra)->devicetype;
}

static int usbat_read(struct us_data *us,
		      unsigned char access,
		      unsigned char reg,
		      unsigned char *content)
{
	return usb_stor_ctrl_transfer(us,
		us->recv_ctrl_pipe,
		access | USBAT_CMD_READ_REG,
		0xC0,
		(u16)reg,
		0,
		content,
		1);
}

static int usbat_write(struct us_data *us,
		       unsigned char access,
		       unsigned char reg,
		       unsigned char content)
{
	return usb_stor_ctrl_transfer(us,
		us->send_ctrl_pipe,
		access | USBAT_CMD_WRITE_REG,
		0x40,
		short_pack(reg, content),
		0,
		NULL,
		0);
}

static int usbat_bulk_read(struct us_data *us,
			   void* buf,
			   unsigned int len,
			   int use_sg)
{
	if (len == 0)
		return USB_STOR_XFER_GOOD;

	US_DEBUGP("usbat_bulk_read: len = %d\n", len);
	return usb_stor_bulk_transfer_sg(us, us->recv_bulk_pipe, buf, len, use_sg, NULL);
}

static int usbat_bulk_write(struct us_data *us,
			    void* buf,
			    unsigned int len,
			    int use_sg)
{
	if (len == 0)
		return USB_STOR_XFER_GOOD;

	US_DEBUGP("usbat_bulk_write:  len = %d\n", len);
	return usb_stor_bulk_transfer_sg(us, us->send_bulk_pipe, buf, len, use_sg, NULL);
}

static int usbat_execute_command(struct us_data *us,
								 unsigned char *commands,
								 unsigned int len)
{
	return usb_stor_ctrl_transfer(us, us->send_ctrl_pipe,
								  USBAT_CMD_EXEC_CMD, 0x40, 0, 0,
								  commands, len);
}

static int usbat_get_status(struct us_data *us, unsigned char *status)
{
	int rc;
	rc = usbat_read(us, USBAT_ATA, USBAT_ATA_STATUS, status);

	US_DEBUGP("usbat_get_status: 0x%02X\n", (unsigned short) (*status));
	return rc;
}

static int usbat_check_status(struct us_data *us)
{
	unsigned char *reply = us->iobuf;
	int rc;

	rc = usbat_get_status(us, reply);
	if (rc != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_FAILED;

	
	if (*reply & 0x01 && *reply != 0x51)
		return USB_STOR_TRANSPORT_FAILED;

	
	if (*reply & 0x20)
		return USB_STOR_TRANSPORT_FAILED;

	return USB_STOR_TRANSPORT_GOOD;
}

static int usbat_set_shuttle_features(struct us_data *us,
				      unsigned char external_trigger,
				      unsigned char epp_control,
				      unsigned char mask_byte,
				      unsigned char test_pattern,
				      unsigned char subcountH,
				      unsigned char subcountL)
{
	unsigned char *command = us->iobuf;

	command[0] = 0x40;
	command[1] = USBAT_CMD_SET_FEAT;

	command[2] = epp_control;

	command[3] = external_trigger;

	command[4] = test_pattern;

	command[5] = mask_byte;

	command[6] = subcountL;
	command[7] = subcountH;

	return usbat_execute_command(us, command, 8);
}

static int usbat_wait_not_busy(struct us_data *us, int minutes)
{
	int i;
	int result;
	unsigned char *status = us->iobuf;


	for (i=0; i<1200+minutes*60; i++) {

 		result = usbat_get_status(us, status);

		if (result!=USB_STOR_XFER_GOOD)
			return USB_STOR_TRANSPORT_ERROR;
		if (*status & 0x01) { 
			result = usbat_read(us, USBAT_ATA, 0x10, status);
			return USB_STOR_TRANSPORT_FAILED;
		}
		if (*status & 0x20) 
			return USB_STOR_TRANSPORT_FAILED;

		if ((*status & 0x80)==0x00) { 
			US_DEBUGP("Waited not busy for %d steps\n", i);
			return USB_STOR_TRANSPORT_GOOD;
		}

		if (i<500)
			msleep(10); 
		else if (i<700)
			msleep(50); 
		else if (i<1200)
			msleep(100); 
		else
			msleep(1000); 
	}

	US_DEBUGP("Waited not busy for %d minutes, timing out.\n",
		minutes);
	return USB_STOR_TRANSPORT_FAILED;
}

static int usbat_read_block(struct us_data *us,
			    void* buf,
			    unsigned short len,
			    int use_sg)
{
	int result;
	unsigned char *command = us->iobuf;

	if (!len)
		return USB_STOR_TRANSPORT_GOOD;

	command[0] = 0xC0;
	command[1] = USBAT_ATA | USBAT_CMD_READ_BLOCK;
	command[2] = USBAT_ATA_DATA;
	command[3] = 0;
	command[4] = 0;
	command[5] = 0;
	command[6] = LSB_of(len);
	command[7] = MSB_of(len);

	result = usbat_execute_command(us, command, 8);
	if (result != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;

	result = usbat_bulk_read(us, buf, len, use_sg);
	return (result == USB_STOR_XFER_GOOD ?
			USB_STOR_TRANSPORT_GOOD : USB_STOR_TRANSPORT_ERROR);
}

static int usbat_write_block(struct us_data *us,
			     unsigned char access,
			     void* buf,
			     unsigned short len,
			     int minutes,
			     int use_sg)
{
	int result;
	unsigned char *command = us->iobuf;

	if (!len)
		return USB_STOR_TRANSPORT_GOOD;

	command[0] = 0x40;
	command[1] = access | USBAT_CMD_WRITE_BLOCK;
	command[2] = USBAT_ATA_DATA;
	command[3] = 0;
	command[4] = 0;
	command[5] = 0;
	command[6] = LSB_of(len);
	command[7] = MSB_of(len);

	result = usbat_execute_command(us, command, 8);

	if (result != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;

	result = usbat_bulk_write(us, buf, len, use_sg);
	if (result != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;

	return usbat_wait_not_busy(us, minutes);
}

static int usbat_hp8200e_rw_block_test(struct us_data *us,
				       unsigned char access,
				       unsigned char *registers,
				       unsigned char *data_out,
				       unsigned short num_registers,
				       unsigned char data_reg,
				       unsigned char status_reg,
				       unsigned char timeout,
				       unsigned char qualifier,
				       int direction,
				       void *buf,
				       unsigned short len,
				       int use_sg,
				       int minutes)
{
	int result;
	unsigned int pipe = (direction == DMA_FROM_DEVICE) ?
			us->recv_bulk_pipe : us->send_bulk_pipe;

	unsigned char *command = us->iobuf;
	int i, j;
	int cmdlen;
	unsigned char *data = us->iobuf;
	unsigned char *status = us->iobuf;

	BUG_ON(num_registers > US_IOBUF_SIZE/2);

	for (i=0; i<20; i++) {


		if (i==0) {
			cmdlen = 16;
			command[0] = 0x40;
			command[1] = access | USBAT_CMD_WRITE_REGS;
			command[2] = 0x07;
			command[3] = 0x17;
			command[4] = 0xFC;
			command[5] = 0xE7;
			command[6] = LSB_of(num_registers*2);
			command[7] = MSB_of(num_registers*2);
		} else
			cmdlen = 8;

		
		command[cmdlen-8] = (direction==DMA_TO_DEVICE ? 0x40 : 0xC0);
		command[cmdlen-7] = access |
				(direction==DMA_TO_DEVICE ?
				 USBAT_CMD_COND_WRITE_BLOCK : USBAT_CMD_COND_READ_BLOCK);
		command[cmdlen-6] = data_reg;
		command[cmdlen-5] = status_reg;
		command[cmdlen-4] = timeout;
		command[cmdlen-3] = qualifier;
		command[cmdlen-2] = LSB_of(len);
		command[cmdlen-1] = MSB_of(len);

		result = usbat_execute_command(us, command, cmdlen);

		if (result != USB_STOR_XFER_GOOD)
			return USB_STOR_TRANSPORT_ERROR;

		if (i==0) {

			for (j=0; j<num_registers; j++) {
				data[j<<1] = registers[j];
				data[1+(j<<1)] = data_out[j];
			}

			result = usbat_bulk_write(us, data, num_registers*2, 0);
			if (result != USB_STOR_XFER_GOOD)
				return USB_STOR_TRANSPORT_ERROR;

		}

		result = usb_stor_bulk_transfer_sg(us,
			pipe, buf, len, use_sg, NULL);


		if (result == USB_STOR_XFER_SHORT ||
				result == USB_STOR_XFER_STALLED) {


			if (direction==DMA_FROM_DEVICE && i==0) {
				if (usb_stor_clear_halt(us,
						us->send_bulk_pipe) < 0)
					return USB_STOR_TRANSPORT_ERROR;
			}


 			result = usbat_read(us, USBAT_ATA, 
				direction==DMA_TO_DEVICE ?
					USBAT_ATA_STATUS : USBAT_ATA_ALTSTATUS,
				status);

			if (result!=USB_STOR_XFER_GOOD)
				return USB_STOR_TRANSPORT_ERROR;
			if (*status & 0x01) 
				return USB_STOR_TRANSPORT_FAILED;
			if (*status & 0x20) 
				return USB_STOR_TRANSPORT_FAILED;

			US_DEBUGP("Redoing %s\n",
			  direction==DMA_TO_DEVICE ? "write" : "read");

		} else if (result != USB_STOR_XFER_GOOD)
			return USB_STOR_TRANSPORT_ERROR;
		else
			return usbat_wait_not_busy(us, minutes);

	}

	US_DEBUGP("Bummer! %s bulk data 20 times failed.\n",
		direction==DMA_TO_DEVICE ? "Writing" : "Reading");

	return USB_STOR_TRANSPORT_FAILED;
}

/*
 * Write to multiple registers:
 * Allows us to write specific data to any registers. The data to be written
 * gets packed in this sequence: reg0, data0, reg1, data1, ..., regN, dataN
 * which gets sent through bulk out.
 * Not designed for large transfers of data!
 */
static int usbat_multiple_write(struct us_data *us,
				unsigned char *registers,
				unsigned char *data_out,
				unsigned short num_registers)
{
	int i, result;
	unsigned char *data = us->iobuf;
	unsigned char *command = us->iobuf;

	BUG_ON(num_registers > US_IOBUF_SIZE/2);

	
	command[0] = 0x40;
	command[1] = USBAT_ATA | USBAT_CMD_WRITE_REGS;

	
	command[2] = 0;
	command[3] = 0;
	command[4] = 0;
	command[5] = 0;

	
	command[6] = LSB_of(num_registers*2);
	command[7] = MSB_of(num_registers*2);

	
	result = usbat_execute_command(us, command, 8);
	if (result != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;

	
	for (i=0; i<num_registers; i++) {
		data[i<<1] = registers[i];
		data[1+(i<<1)] = data_out[i];
	}

	
	result = usbat_bulk_write(us, data, num_registers*2, 0);
	if (result != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;

	if (usbat_get_device_type(us) == USBAT_DEV_HP8200)
		return usbat_wait_not_busy(us, 0);
	else
		return USB_STOR_TRANSPORT_GOOD;
}

static int usbat_read_blocks(struct us_data *us,
			     void* buffer,
			     int len,
			     int use_sg)
{
	int result;
	unsigned char *command = us->iobuf;

	command[0] = 0xC0;
	command[1] = USBAT_ATA | USBAT_CMD_COND_READ_BLOCK;
	command[2] = USBAT_ATA_DATA;
	command[3] = USBAT_ATA_STATUS;
	command[4] = 0xFD; 
	command[5] = USBAT_QUAL_FCQ;
	command[6] = LSB_of(len);
	command[7] = MSB_of(len);

	
	result = usbat_execute_command(us, command, 8);
	if (result != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_FAILED;
	
	
	result = usbat_bulk_read(us, buffer, len, use_sg);
	if (result != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_FAILED;

	return USB_STOR_TRANSPORT_GOOD;
}

static int usbat_write_blocks(struct us_data *us,
			      void* buffer,
			      int len,
			      int use_sg)
{
	int result;
	unsigned char *command = us->iobuf;

	command[0] = 0x40;
	command[1] = USBAT_ATA | USBAT_CMD_COND_WRITE_BLOCK;
	command[2] = USBAT_ATA_DATA;
	command[3] = USBAT_ATA_STATUS;
	command[4] = 0xFD; 
	command[5] = USBAT_QUAL_FCQ;
	command[6] = LSB_of(len);
	command[7] = MSB_of(len);

	
	result = usbat_execute_command(us, command, 8);
	if (result != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_FAILED;
	
	
	result = usbat_bulk_write(us, buffer, len, use_sg);
	if (result != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_FAILED;

	return USB_STOR_TRANSPORT_GOOD;
}

static int usbat_read_user_io(struct us_data *us, unsigned char *data_flags)
{
	int result;

	result = usb_stor_ctrl_transfer(us,
		us->recv_ctrl_pipe,
		USBAT_CMD_UIO,
		0xC0,
		0,
		0,
		data_flags,
		USBAT_UIO_READ);

	US_DEBUGP("usbat_read_user_io: UIO register reads %02X\n", (unsigned short) (*data_flags));

	return result;
}

static int usbat_write_user_io(struct us_data *us,
			       unsigned char enable_flags,
			       unsigned char data_flags)
{
	return usb_stor_ctrl_transfer(us,
		us->send_ctrl_pipe,
		USBAT_CMD_UIO,
		0x40,
		short_pack(enable_flags, data_flags),
		0,
		NULL,
		USBAT_UIO_WRITE);
}

static int usbat_device_reset(struct us_data *us)
{
	int rc;

	rc = usbat_write_user_io(us,
							 USBAT_UIO_DRVRST | USBAT_UIO_OE1 | USBAT_UIO_OE0,
							 USBAT_UIO_EPAD | USBAT_UIO_1);
	if (rc != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;
			
	rc = usbat_write_user_io(us,
							 USBAT_UIO_OE1  | USBAT_UIO_OE0,
							 USBAT_UIO_EPAD | USBAT_UIO_1);
	if (rc != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;

	return USB_STOR_TRANSPORT_GOOD;
}

static int usbat_device_enable_cdt(struct us_data *us)
{
	int rc;

	
	rc = usbat_write_user_io(us,
							 USBAT_UIO_ACKD | USBAT_UIO_OE1  | USBAT_UIO_OE0,
							 USBAT_UIO_EPAD | USBAT_UIO_1);
	if (rc != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;

	return USB_STOR_TRANSPORT_GOOD;
}

static int usbat_flash_check_media_present(unsigned char *uio)
{
	if (*uio & USBAT_UIO_UI0) {
		US_DEBUGP("usbat_flash_check_media_present: no media detected\n");
		return USBAT_FLASH_MEDIA_NONE;
	}

	return USBAT_FLASH_MEDIA_CF;
}

static int usbat_flash_check_media_changed(unsigned char *uio)
{
	if (*uio & USBAT_UIO_0) {
		US_DEBUGP("usbat_flash_check_media_changed: media change detected\n");
		return USBAT_FLASH_MEDIA_CHANGED;
	}

	return USBAT_FLASH_MEDIA_SAME;
}

static int usbat_flash_check_media(struct us_data *us,
				   struct usbat_info *info)
{
	int rc;
	unsigned char *uio = us->iobuf;

	rc = usbat_read_user_io(us, uio);
	if (rc != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;

	
	rc = usbat_flash_check_media_present(uio);
	if (rc == USBAT_FLASH_MEDIA_NONE) {
		info->sense_key = 0x02;
		info->sense_asc = 0x3A;
		info->sense_ascq = 0x00;
		return USB_STOR_TRANSPORT_FAILED;
	}

	
	rc = usbat_flash_check_media_changed(uio);
	if (rc == USBAT_FLASH_MEDIA_CHANGED) {

		
		rc = usbat_device_reset(us);
		if (rc != USB_STOR_TRANSPORT_GOOD)
			return rc;
		rc = usbat_device_enable_cdt(us);
		if (rc != USB_STOR_TRANSPORT_GOOD)
			return rc;

		msleep(50);

		rc = usbat_read_user_io(us, uio);
		if (rc != USB_STOR_XFER_GOOD)
			return USB_STOR_TRANSPORT_ERROR;
		
		info->sense_key = UNIT_ATTENTION;
		info->sense_asc = 0x28;
		info->sense_ascq = 0x00;
		return USB_STOR_TRANSPORT_FAILED;
	}

	return USB_STOR_TRANSPORT_GOOD;
}

static int usbat_identify_device(struct us_data *us,
				 struct usbat_info *info)
{
	int rc;
	unsigned char status;

	if (!us || !info)
		return USB_STOR_TRANSPORT_ERROR;

	rc = usbat_device_reset(us);
	if (rc != USB_STOR_TRANSPORT_GOOD)
		return rc;
	msleep(500);

	rc = usbat_write(us, USBAT_ATA, USBAT_ATA_CMD, 0xA1);
 	if (rc != USB_STOR_XFER_GOOD)
 		return USB_STOR_TRANSPORT_ERROR;

	rc = usbat_get_status(us, &status);
 	if (rc != USB_STOR_XFER_GOOD)
 		return USB_STOR_TRANSPORT_ERROR;

	
	if (status == 0xA1 || !(status & 0x01)) {
		
		US_DEBUGP("usbat_identify_device: Detected HP8200 CDRW\n");
		info->devicetype = USBAT_DEV_HP8200;
	} else {
		
		US_DEBUGP("usbat_identify_device: Detected Flash reader/writer\n");
		info->devicetype = USBAT_DEV_FLASH;
	}

	return USB_STOR_TRANSPORT_GOOD;
}

static int usbat_set_transport(struct us_data *us,
			       struct usbat_info *info,
			       int devicetype)
{

	if (!info->devicetype)
		info->devicetype = devicetype;

	if (!info->devicetype)
		usbat_identify_device(us, info);

	switch (info->devicetype) {
	default:
		return USB_STOR_TRANSPORT_ERROR;

	case  USBAT_DEV_HP8200:
		us->transport = usbat_hp8200e_transport;
		break;

	case USBAT_DEV_FLASH:
		us->transport = usbat_flash_transport;
		break;
	}

	return 0;
}

static int usbat_flash_get_sector_count(struct us_data *us,
					struct usbat_info *info)
{
	unsigned char registers[3] = {
		USBAT_ATA_SECCNT,
		USBAT_ATA_DEVICE,
		USBAT_ATA_CMD,
	};
	unsigned char  command[3] = { 0x01, 0xA0, 0xEC };
	unsigned char *reply;
	unsigned char status;
	int rc;

	if (!us || !info)
		return USB_STOR_TRANSPORT_ERROR;

	reply = kmalloc(512, GFP_NOIO);
	if (!reply)
		return USB_STOR_TRANSPORT_ERROR;

	
	rc = usbat_multiple_write(us, registers, command, 3);
	if (rc != USB_STOR_XFER_GOOD) {
		US_DEBUGP("usbat_flash_get_sector_count: Gah! identify_device failed\n");
		rc = USB_STOR_TRANSPORT_ERROR;
		goto leave;
	}

	
	if (usbat_get_status(us, &status) != USB_STOR_XFER_GOOD) {
		rc = USB_STOR_TRANSPORT_ERROR;
		goto leave;
	}

	msleep(100);

	
	rc = usbat_read_block(us, reply, 512, 0);
	if (rc != USB_STOR_TRANSPORT_GOOD)
		goto leave;

	info->sectors = ((u32)(reply[117]) << 24) |
		((u32)(reply[116]) << 16) |
		((u32)(reply[115]) <<  8) |
		((u32)(reply[114])      );

	rc = USB_STOR_TRANSPORT_GOOD;

 leave:
	kfree(reply);
	return rc;
}

static int usbat_flash_read_data(struct us_data *us,
								 struct usbat_info *info,
								 u32 sector,
								 u32 sectors)
{
	unsigned char registers[7] = {
		USBAT_ATA_FEATURES,
		USBAT_ATA_SECCNT,
		USBAT_ATA_SECNUM,
		USBAT_ATA_LBA_ME,
		USBAT_ATA_LBA_HI,
		USBAT_ATA_DEVICE,
		USBAT_ATA_STATUS,
	};
	unsigned char command[7];
	unsigned char *buffer;
	unsigned char  thistime;
	unsigned int totallen, alloclen;
	int len, result;
	unsigned int sg_offset = 0;
	struct scatterlist *sg = NULL;

	result = usbat_flash_check_media(us, info);
	if (result != USB_STOR_TRANSPORT_GOOD)
		return result;


	if (sector > 0x0FFFFFFF)
		return USB_STOR_TRANSPORT_ERROR;

	totallen = sectors * info->ssize;


	alloclen = min(totallen, 65536u);
	buffer = kmalloc(alloclen, GFP_NOIO);
	if (buffer == NULL)
		return USB_STOR_TRANSPORT_ERROR;

	do {
		len = min(totallen, alloclen);
		thistime = (len / info->ssize) & 0xff;
 
		
		usbat_pack_ata_sector_cmd(command, thistime, sector, 0x20);

		
		result = usbat_multiple_write(us, registers, command, 7);
		if (result != USB_STOR_TRANSPORT_GOOD)
			goto leave;

		
		result = usbat_read_blocks(us, buffer, len, 0);
		if (result != USB_STOR_TRANSPORT_GOOD)
			goto leave;
  	 
		US_DEBUGP("usbat_flash_read_data:  %d bytes\n", len);
	
		
		usb_stor_access_xfer_buf(buffer, len, us->srb,
					 &sg, &sg_offset, TO_XFER_BUF);

		sector += thistime;
		totallen -= len;
	} while (totallen > 0);

	kfree(buffer);
	return USB_STOR_TRANSPORT_GOOD;

leave:
	kfree(buffer);
	return USB_STOR_TRANSPORT_ERROR;
}

static int usbat_flash_write_data(struct us_data *us,
								  struct usbat_info *info,
								  u32 sector,
								  u32 sectors)
{
	unsigned char registers[7] = {
		USBAT_ATA_FEATURES,
		USBAT_ATA_SECCNT,
		USBAT_ATA_SECNUM,
		USBAT_ATA_LBA_ME,
		USBAT_ATA_LBA_HI,
		USBAT_ATA_DEVICE,
		USBAT_ATA_STATUS,
	};
	unsigned char command[7];
	unsigned char *buffer;
	unsigned char  thistime;
	unsigned int totallen, alloclen;
	int len, result;
	unsigned int sg_offset = 0;
	struct scatterlist *sg = NULL;

	result = usbat_flash_check_media(us, info);
	if (result != USB_STOR_TRANSPORT_GOOD)
		return result;


	if (sector > 0x0FFFFFFF)
		return USB_STOR_TRANSPORT_ERROR;

	totallen = sectors * info->ssize;


	alloclen = min(totallen, 65536u);
	buffer = kmalloc(alloclen, GFP_NOIO);
	if (buffer == NULL)
		return USB_STOR_TRANSPORT_ERROR;

	do {
		len = min(totallen, alloclen);
		thistime = (len / info->ssize) & 0xff;

		
		usb_stor_access_xfer_buf(buffer, len, us->srb,
					 &sg, &sg_offset, FROM_XFER_BUF);

		
		usbat_pack_ata_sector_cmd(command, thistime, sector, 0x30);

		
		result = usbat_multiple_write(us, registers, command, 7);
		if (result != USB_STOR_TRANSPORT_GOOD)
			goto leave;

		
		result = usbat_write_blocks(us, buffer, len, 0);
		if (result != USB_STOR_TRANSPORT_GOOD)
			goto leave;

		sector += thistime;
		totallen -= len;
	} while (totallen > 0);

	kfree(buffer);
	return result;

leave:
	kfree(buffer);
	return USB_STOR_TRANSPORT_ERROR;
}

static int usbat_hp8200e_handle_read10(struct us_data *us,
				       unsigned char *registers,
				       unsigned char *data,
				       struct scsi_cmnd *srb)
{
	int result = USB_STOR_TRANSPORT_GOOD;
	unsigned char *buffer;
	unsigned int len;
	unsigned int sector;
	unsigned int sg_offset = 0;
	struct scatterlist *sg = NULL;

	US_DEBUGP("handle_read10: transfersize %d\n",
		srb->transfersize);

	if (scsi_bufflen(srb) < 0x10000) {

		result = usbat_hp8200e_rw_block_test(us, USBAT_ATA, 
			registers, data, 19,
			USBAT_ATA_DATA, USBAT_ATA_STATUS, 0xFD,
			(USBAT_QUAL_FCQ | USBAT_QUAL_ALQ),
			DMA_FROM_DEVICE,
			scsi_sglist(srb),
			scsi_bufflen(srb), scsi_sg_count(srb), 1);

		return result;
	}


	if (data[7+0] == GPCMD_READ_CD) {
		len = short_pack(data[7+9], data[7+8]);
		len <<= 16;
		len |= data[7+7];
		US_DEBUGP("handle_read10: GPCMD_READ_CD: len %d\n", len);
		srb->transfersize = scsi_bufflen(srb)/len;
	}

	if (!srb->transfersize)  {
		srb->transfersize = 2048; 
		US_DEBUGP("handle_read10: transfersize 0, forcing %d\n",
			srb->transfersize);
	}


	len = (65535/srb->transfersize) * srb->transfersize;
	US_DEBUGP("Max read is %d bytes\n", len);
	len = min(len, scsi_bufflen(srb));
	buffer = kmalloc(len, GFP_NOIO);
	if (buffer == NULL) 
		return USB_STOR_TRANSPORT_FAILED;
	sector = short_pack(data[7+3], data[7+2]);
	sector <<= 16;
	sector |= short_pack(data[7+5], data[7+4]);
	transferred = 0;

	while (transferred != scsi_bufflen(srb)) {

		if (len > scsi_bufflen(srb) - transferred)
			len = scsi_bufflen(srb) - transferred;

		data[3] = len&0xFF; 	  
		data[4] = (len>>8)&0xFF;  

		

		data[7+2] = MSB_of(sector>>16); 
		data[7+3] = LSB_of(sector>>16);
		data[7+4] = MSB_of(sector&0xFFFF);
		data[7+5] = LSB_of(sector&0xFFFF);
		if (data[7+0] == GPCMD_READ_CD)
			data[7+6] = 0;
		data[7+7] = MSB_of(len / srb->transfersize); 
		data[7+8] = LSB_of(len / srb->transfersize); 

		result = usbat_hp8200e_rw_block_test(us, USBAT_ATA, 
			registers, data, 19,
			USBAT_ATA_DATA, USBAT_ATA_STATUS, 0xFD, 
			(USBAT_QUAL_FCQ | USBAT_QUAL_ALQ),
			DMA_FROM_DEVICE,
			buffer,
			len, 0, 1);

		if (result != USB_STOR_TRANSPORT_GOOD)
			break;

		
		usb_stor_access_xfer_buf(buffer, len, srb,
				 &sg, &sg_offset, TO_XFER_BUF);

		

		transferred += len;
		sector += len / srb->transfersize;

	} 

	kfree(buffer);
	return result;
}

static int usbat_select_and_test_registers(struct us_data *us)
{
	int selector;
	unsigned char *status = us->iobuf;

	
	for (selector = 0xA0; selector <= 0xB0; selector += 0x10) {
		if (usbat_write(us, USBAT_ATA, USBAT_ATA_DEVICE, selector) !=
				USB_STOR_XFER_GOOD)
			return USB_STOR_TRANSPORT_ERROR;

		if (usbat_read(us, USBAT_ATA, USBAT_ATA_STATUS, status) != 
				USB_STOR_XFER_GOOD)
			return USB_STOR_TRANSPORT_ERROR;

		if (usbat_read(us, USBAT_ATA, USBAT_ATA_DEVICE, status) != 
				USB_STOR_XFER_GOOD)
			return USB_STOR_TRANSPORT_ERROR;

		if (usbat_read(us, USBAT_ATA, USBAT_ATA_LBA_ME, status) != 
				USB_STOR_XFER_GOOD)
			return USB_STOR_TRANSPORT_ERROR;

		if (usbat_read(us, USBAT_ATA, USBAT_ATA_LBA_HI, status) != 
				USB_STOR_XFER_GOOD)
			return USB_STOR_TRANSPORT_ERROR;

		if (usbat_write(us, USBAT_ATA, USBAT_ATA_LBA_ME, 0x55) != 
				USB_STOR_XFER_GOOD)
			return USB_STOR_TRANSPORT_ERROR;

		if (usbat_write(us, USBAT_ATA, USBAT_ATA_LBA_HI, 0xAA) != 
				USB_STOR_XFER_GOOD)
			return USB_STOR_TRANSPORT_ERROR;

		if (usbat_read(us, USBAT_ATA, USBAT_ATA_LBA_ME, status) != 
				USB_STOR_XFER_GOOD)
			return USB_STOR_TRANSPORT_ERROR;

		if (usbat_read(us, USBAT_ATA, USBAT_ATA_LBA_ME, status) != 
				USB_STOR_XFER_GOOD)
			return USB_STOR_TRANSPORT_ERROR;
	}

	return USB_STOR_TRANSPORT_GOOD;
}

static int init_usbat(struct us_data *us, int devicetype)
{
	int rc;
	struct usbat_info *info;
	unsigned char subcountH = USBAT_ATA_LBA_HI;
	unsigned char subcountL = USBAT_ATA_LBA_ME;
	unsigned char *status = us->iobuf;

	us->extra = kzalloc(sizeof(struct usbat_info), GFP_NOIO);
	if (!us->extra) {
		US_DEBUGP("init_usbat: Gah! Can't allocate storage for usbat info struct!\n");
		return 1;
	}
	info = (struct usbat_info *) (us->extra);

	
	rc = usbat_write_user_io(us,
				 USBAT_UIO_OE1 | USBAT_UIO_OE0,
				 USBAT_UIO_EPAD | USBAT_UIO_1);
	if (rc != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;

	US_DEBUGP("INIT 1\n");

	msleep(2000);

	rc = usbat_read_user_io(us, status);
	if (rc != USB_STOR_TRANSPORT_GOOD)
		return rc;

	US_DEBUGP("INIT 2\n");

	rc = usbat_read_user_io(us, status);
	if (rc != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;

	rc = usbat_read_user_io(us, status);
	if (rc != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;

	US_DEBUGP("INIT 3\n");

	rc = usbat_select_and_test_registers(us);
	if (rc != USB_STOR_TRANSPORT_GOOD)
		return rc;

	US_DEBUGP("INIT 4\n");

	rc = usbat_read_user_io(us, status);
	if (rc != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;

	US_DEBUGP("INIT 5\n");

	
	rc = usbat_device_enable_cdt(us);
	if (rc != USB_STOR_TRANSPORT_GOOD)
		return rc;

	US_DEBUGP("INIT 6\n");

	rc = usbat_read_user_io(us, status);
	if (rc != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;

	US_DEBUGP("INIT 7\n");

	msleep(1400);

	rc = usbat_read_user_io(us, status);
	if (rc != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;

	US_DEBUGP("INIT 8\n");

	rc = usbat_select_and_test_registers(us);
	if (rc != USB_STOR_TRANSPORT_GOOD)
		return rc;

	US_DEBUGP("INIT 9\n");

	
	if (usbat_set_transport(us, info, devicetype))
		return USB_STOR_TRANSPORT_ERROR;

	US_DEBUGP("INIT 10\n");

	if (usbat_get_device_type(us) == USBAT_DEV_FLASH) { 
		subcountH = 0x02;
		subcountL = 0x00;
	}
	rc = usbat_set_shuttle_features(us, (USBAT_FEAT_ETEN | USBAT_FEAT_ET2 | USBAT_FEAT_ET1),
									0x00, 0x88, 0x08, subcountH, subcountL);
	if (rc != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;

	US_DEBUGP("INIT 11\n");

	return USB_STOR_TRANSPORT_GOOD;
}

static int usbat_hp8200e_transport(struct scsi_cmnd *srb, struct us_data *us)
{
	int result;
	unsigned char *status = us->iobuf;
	unsigned char registers[32];
	unsigned char data[32];
	unsigned int len;
	int i;

	len = scsi_bufflen(srb);


	registers[0] = USBAT_ATA_FEATURES;
	registers[1] = USBAT_ATA_SECCNT;
	registers[2] = USBAT_ATA_SECNUM;
	registers[3] = USBAT_ATA_LBA_ME;
	registers[4] = USBAT_ATA_LBA_HI;
	registers[5] = USBAT_ATA_DEVICE;
	registers[6] = USBAT_ATA_CMD;
	data[0] = 0x00;
	data[1] = 0x00;
	data[2] = 0x00;
	data[3] = len&0xFF; 		
	data[4] = (len>>8)&0xFF; 	
	data[5] = 0xB0; 		
	data[6] = 0xA0; 		

	for (i=7; i<19; i++) {
		registers[i] = 0x10;
		data[i] = (i-7 >= srb->cmd_len) ? 0 : srb->cmnd[i-7];
	}

	result = usbat_get_status(us, status);
	US_DEBUGP("Status = %02X\n", *status);
	if (result != USB_STOR_XFER_GOOD)
		return USB_STOR_TRANSPORT_ERROR;
	if (srb->cmnd[0] == TEST_UNIT_READY)
		transferred = 0;

	if (srb->sc_data_direction == DMA_TO_DEVICE) {

		result = usbat_hp8200e_rw_block_test(us, USBAT_ATA, 
			registers, data, 19,
			USBAT_ATA_DATA, USBAT_ATA_STATUS, 0xFD,
			(USBAT_QUAL_FCQ | USBAT_QUAL_ALQ),
			DMA_TO_DEVICE,
			scsi_sglist(srb),
			len, scsi_sg_count(srb), 10);

		if (result == USB_STOR_TRANSPORT_GOOD) {
			transferred += len;
			US_DEBUGP("Wrote %08X bytes\n", transferred);
		}

		return result;

	} else if (srb->cmnd[0] == READ_10 ||
		   srb->cmnd[0] == GPCMD_READ_CD) {

		return usbat_hp8200e_handle_read10(us, registers, data, srb);

	}

	if (len > 0xFFFF) {
		US_DEBUGP("Error: len = %08X... what do I do now?\n",
			len);
		return USB_STOR_TRANSPORT_ERROR;
	}

	result = usbat_multiple_write(us, registers, data, 7);

	if (result != USB_STOR_TRANSPORT_GOOD)
		return result;


	result = usbat_write_block(us, USBAT_ATA, srb->cmnd, 12,
				   srb->cmnd[0] == GPCMD_BLANK ? 75 : 10, 0);

	if (result != USB_STOR_TRANSPORT_GOOD)
		return result;

	

	if (len != 0 && (srb->sc_data_direction == DMA_FROM_DEVICE)) {

		

		if (usbat_read(us, USBAT_ATA, USBAT_ATA_LBA_ME, status) != 
		    	USB_STOR_XFER_GOOD) {
			return USB_STOR_TRANSPORT_ERROR;
		}

		if (len > 0xFF) { 
			len = *status;
			if (usbat_read(us, USBAT_ATA, USBAT_ATA_LBA_HI, status) !=
				    USB_STOR_XFER_GOOD) {
				return USB_STOR_TRANSPORT_ERROR;
			}
			len += ((unsigned int) *status)<<8;
		}
		else
			len = *status;


		result = usbat_read_block(us, scsi_sglist(srb), len,
			                                   scsi_sg_count(srb));
	}

	return result;
}

static int usbat_flash_transport(struct scsi_cmnd * srb, struct us_data *us)
{
	int rc;
	struct usbat_info *info = (struct usbat_info *) (us->extra);
	unsigned long block, blocks;
	unsigned char *ptr = us->iobuf;
	static unsigned char inquiry_response[36] = {
		0x00, 0x80, 0x00, 0x01, 0x1F, 0x00, 0x00, 0x00
	};

	if (srb->cmnd[0] == INQUIRY) {
		US_DEBUGP("usbat_flash_transport: INQUIRY. Returning bogus response.\n");
		memcpy(ptr, inquiry_response, sizeof(inquiry_response));
		fill_inquiry_response(us, ptr, 36);
		return USB_STOR_TRANSPORT_GOOD;
	}

	if (srb->cmnd[0] == READ_CAPACITY) {
		rc = usbat_flash_check_media(us, info);
		if (rc != USB_STOR_TRANSPORT_GOOD)
			return rc;

		rc = usbat_flash_get_sector_count(us, info);
		if (rc != USB_STOR_TRANSPORT_GOOD)
			return rc;

		
		info->ssize = 0x200;
		US_DEBUGP("usbat_flash_transport: READ_CAPACITY: %ld sectors, %ld bytes per sector\n",
			  info->sectors, info->ssize);

		((__be32 *) ptr)[0] = cpu_to_be32(info->sectors - 1);
		((__be32 *) ptr)[1] = cpu_to_be32(info->ssize);
		usb_stor_set_xfer_buf(ptr, 8, srb);

		return USB_STOR_TRANSPORT_GOOD;
	}

	if (srb->cmnd[0] == MODE_SELECT_10) {
		US_DEBUGP("usbat_flash_transport:  Gah! MODE_SELECT_10.\n");
		return USB_STOR_TRANSPORT_ERROR;
	}

	if (srb->cmnd[0] == READ_10) {
		block = ((u32)(srb->cmnd[2]) << 24) | ((u32)(srb->cmnd[3]) << 16) |
				((u32)(srb->cmnd[4]) <<  8) | ((u32)(srb->cmnd[5]));

		blocks = ((u32)(srb->cmnd[7]) << 8) | ((u32)(srb->cmnd[8]));

		US_DEBUGP("usbat_flash_transport:  READ_10: read block 0x%04lx  count %ld\n", block, blocks);
		return usbat_flash_read_data(us, info, block, blocks);
	}

	if (srb->cmnd[0] == READ_12) {
		block = ((u32)(srb->cmnd[2]) << 24) | ((u32)(srb->cmnd[3]) << 16) |
		        ((u32)(srb->cmnd[4]) <<  8) | ((u32)(srb->cmnd[5]));

		blocks = ((u32)(srb->cmnd[6]) << 24) | ((u32)(srb->cmnd[7]) << 16) |
		         ((u32)(srb->cmnd[8]) <<  8) | ((u32)(srb->cmnd[9]));

		US_DEBUGP("usbat_flash_transport: READ_12: read block 0x%04lx  count %ld\n", block, blocks);
		return usbat_flash_read_data(us, info, block, blocks);
	}

	if (srb->cmnd[0] == WRITE_10) {
		block = ((u32)(srb->cmnd[2]) << 24) | ((u32)(srb->cmnd[3]) << 16) |
		        ((u32)(srb->cmnd[4]) <<  8) | ((u32)(srb->cmnd[5]));

		blocks = ((u32)(srb->cmnd[7]) << 8) | ((u32)(srb->cmnd[8]));

		US_DEBUGP("usbat_flash_transport: WRITE_10: write block 0x%04lx  count %ld\n", block, blocks);
		return usbat_flash_write_data(us, info, block, blocks);
	}

	if (srb->cmnd[0] == WRITE_12) {
		block = ((u32)(srb->cmnd[2]) << 24) | ((u32)(srb->cmnd[3]) << 16) |
		        ((u32)(srb->cmnd[4]) <<  8) | ((u32)(srb->cmnd[5]));

		blocks = ((u32)(srb->cmnd[6]) << 24) | ((u32)(srb->cmnd[7]) << 16) |
		         ((u32)(srb->cmnd[8]) <<  8) | ((u32)(srb->cmnd[9]));

		US_DEBUGP("usbat_flash_transport: WRITE_12: write block 0x%04lx  count %ld\n", block, blocks);
		return usbat_flash_write_data(us, info, block, blocks);
	}


	if (srb->cmnd[0] == TEST_UNIT_READY) {
		US_DEBUGP("usbat_flash_transport: TEST_UNIT_READY.\n");

		rc = usbat_flash_check_media(us, info);
		if (rc != USB_STOR_TRANSPORT_GOOD)
			return rc;

		return usbat_check_status(us);
	}

	if (srb->cmnd[0] == REQUEST_SENSE) {
		US_DEBUGP("usbat_flash_transport: REQUEST_SENSE.\n");

		memset(ptr, 0, 18);
		ptr[0] = 0xF0;
		ptr[2] = info->sense_key;
		ptr[7] = 11;
		ptr[12] = info->sense_asc;
		ptr[13] = info->sense_ascq;
		usb_stor_set_xfer_buf(ptr, 18, srb);

		return USB_STOR_TRANSPORT_GOOD;
	}

	if (srb->cmnd[0] == ALLOW_MEDIUM_REMOVAL) {
		return USB_STOR_TRANSPORT_GOOD;
	}

	US_DEBUGP("usbat_flash_transport: Gah! Unknown command: %d (0x%x)\n",
			  srb->cmnd[0], srb->cmnd[0]);
	info->sense_key = 0x05;
	info->sense_asc = 0x20;
	info->sense_ascq = 0x00;
	return USB_STOR_TRANSPORT_FAILED;
}

static int init_usbat_cd(struct us_data *us)
{
	return init_usbat(us, USBAT_DEV_HP8200);
}

static int init_usbat_flash(struct us_data *us)
{
	return init_usbat(us, USBAT_DEV_FLASH);
}

static int usbat_probe(struct usb_interface *intf,
			 const struct usb_device_id *id)
{
	struct us_data *us;
	int result;

	result = usb_stor_probe1(&us, intf, id,
			(id - usbat_usb_ids) + usbat_unusual_dev_list);
	if (result)
		return result;

	us->transport_name = "Shuttle USBAT";
	us->transport = usbat_flash_transport;
	us->transport_reset = usb_stor_CB_reset;
	us->max_lun = 1;

	result = usb_stor_probe2(us);
	return result;
}

static struct usb_driver usbat_driver = {
	.name =		"ums-usbat",
	.probe =	usbat_probe,
	.disconnect =	usb_stor_disconnect,
	.suspend =	usb_stor_suspend,
	.resume =	usb_stor_resume,
	.reset_resume =	usb_stor_reset_resume,
	.pre_reset =	usb_stor_pre_reset,
	.post_reset =	usb_stor_post_reset,
	.id_table =	usbat_usb_ids,
	.soft_unbind =	1,
	.no_dynamic_id = 1,
};

module_usb_driver(usbat_driver);
