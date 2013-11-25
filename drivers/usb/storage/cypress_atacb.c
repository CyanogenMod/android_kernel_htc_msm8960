/*
 * Support for emulating SAT (ata pass through) on devices based
 *       on the Cypress USB/ATA bridge supporting ATACB.
 *
 * Copyright (c) 2008 Matthieu Castet (castet.matthieu@free.fr)
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

#include <linux/module.h>
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_eh.h>
#include <linux/ata.h>

#include "usb.h"
#include "protocol.h"
#include "scsiglue.h"
#include "debug.h"

MODULE_DESCRIPTION("SAT support for Cypress USB/ATA bridges with ATACB");
MODULE_AUTHOR("Matthieu Castet <castet.matthieu@free.fr>");
MODULE_LICENSE("GPL");

#define UNUSUAL_DEV(id_vendor, id_product, bcdDeviceMin, bcdDeviceMax, \
		    vendorName, productName, useProtocol, useTransport, \
		    initFunction, flags) \
{ USB_DEVICE_VER(id_vendor, id_product, bcdDeviceMin, bcdDeviceMax), \
  .driver_info = (flags)|(USB_US_TYPE_STOR<<24) }

static struct usb_device_id cypress_usb_ids[] = {
#	include "unusual_cypress.h"
	{ }		
};
MODULE_DEVICE_TABLE(usb, cypress_usb_ids);

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

static struct us_unusual_dev cypress_unusual_dev_list[] = {
#	include "unusual_cypress.h"
	{ }		
};

#undef UNUSUAL_DEV


static void cypress_atacb_passthrough(struct scsi_cmnd *srb, struct us_data *us)
{
	unsigned char save_cmnd[MAX_COMMAND_SIZE];

	if (likely(srb->cmnd[0] != ATA_16 && srb->cmnd[0] != ATA_12)) {
		usb_stor_transparent_scsi_command(srb, us);
		return;
	}

	memcpy(save_cmnd, srb->cmnd, sizeof(save_cmnd));
	memset(srb->cmnd, 0, MAX_COMMAND_SIZE);

	
	if (save_cmnd[1] >> 5) 
		goto invalid_fld;
	
	switch((save_cmnd[1] >> 1) & 0xf) {
		case 3: 
		case 4: 
		case 5: 
			break;
		default:
			goto invalid_fld;
	}

	
	srb->cmd_len = 16;

	srb->cmnd[0] = 0x24; 
	srb->cmnd[1] = 0x24; 

	srb->cmnd[3] = 0xff - 1; 
	srb->cmnd[4] = 1; 

	if (save_cmnd[0] == ATA_16) {
		srb->cmnd[ 6] = save_cmnd[ 4]; 
		srb->cmnd[ 7] = save_cmnd[ 6]; 
		srb->cmnd[ 8] = save_cmnd[ 8]; 
		srb->cmnd[ 9] = save_cmnd[10]; 
		srb->cmnd[10] = save_cmnd[12]; 
		srb->cmnd[11] = save_cmnd[13]; 
		srb->cmnd[12] = save_cmnd[14]; 

		if (save_cmnd[1] & 0x01) {
			
			if (save_cmnd[3] || save_cmnd[5] || save_cmnd[7] || save_cmnd[9]
					|| save_cmnd[11])
				goto invalid_fld;
		}
	}
	else { 
		srb->cmnd[ 6] = save_cmnd[3]; 
		srb->cmnd[ 7] = save_cmnd[4]; 
		srb->cmnd[ 8] = save_cmnd[5]; 
		srb->cmnd[ 9] = save_cmnd[6]; 
		srb->cmnd[10] = save_cmnd[7]; 
		srb->cmnd[11] = save_cmnd[8]; 
		srb->cmnd[12] = save_cmnd[9]; 

	}
	
	if ((srb->cmnd[12] == ATA_CMD_SET_FEATURES)
			&& (srb->cmnd[6] == SETFEATURES_XFER))
		goto invalid_fld;

	if (srb->cmnd[12] == ATA_CMD_ID_ATA || srb->cmnd[12] == ATA_CMD_ID_ATAPI)
		srb->cmnd[2] |= (1<<7); 


	usb_stor_transparent_scsi_command(srb, us);

	if (srb->result == SAM_STAT_CHECK_CONDITION &&
			memcmp(srb->sense_buffer, usb_stor_sense_invalidCDB,
				sizeof(usb_stor_sense_invalidCDB)) == 0) {
		US_DEBUGP("cypress atacb not supported ???\n");
		goto end;
	}

	if ((srb->result != (DID_ERROR << 16) &&
				srb->result != (DID_ABORT << 16)) &&
			save_cmnd[2] & 0x20) {
		struct scsi_eh_save ses;
		unsigned char regs[8];
		unsigned char *sb = srb->sense_buffer;
		unsigned char *desc = sb + 8;
		int tmp_result;

		scsi_eh_prep_cmnd(srb, &ses, NULL, 0, sizeof(regs));

		srb->cmd_len = 16;
		srb->cmnd = ses.cmnd;
		srb->cmnd[2] = 1;

		usb_stor_transparent_scsi_command(srb, us);
		memcpy(regs, srb->sense_buffer, sizeof(regs));
		tmp_result = srb->result;
		scsi_eh_restore_cmnd(srb, &ses);
		
		if (tmp_result != SAM_STAT_GOOD)
			goto invalid_fld;

		
		memset(sb, 0, SCSI_SENSE_BUFFERSIZE);

		
		sb[1] = RECOVERED_ERROR;
		sb[2] = 0; 
		sb[3] = 0x1D;


		
		sb[0] = 0x72;
		desc[0] = 0x09; 

		
		sb[7] = 14;
		desc[1] = 12;

		
		desc[ 2] = 0x00;
		desc[ 3] = regs[1];  
		desc[ 5] = regs[2];  
		desc[ 7] = regs[3];  
		desc[ 9] = regs[4];  
		desc[11] = regs[5];  
		desc[12] = regs[6];  
		desc[13] = regs[7];  

		srb->result = (DRIVER_SENSE << 24) | SAM_STAT_CHECK_CONDITION;
	}
	goto end;
invalid_fld:
	srb->result = (DRIVER_SENSE << 24) | SAM_STAT_CHECK_CONDITION;

	memcpy(srb->sense_buffer,
			usb_stor_sense_invalidCDB,
			sizeof(usb_stor_sense_invalidCDB));
end:
	memcpy(srb->cmnd, save_cmnd, sizeof(save_cmnd));
	if (srb->cmnd[0] == ATA_12)
		srb->cmd_len = 12;
}


static int cypress_probe(struct usb_interface *intf,
			 const struct usb_device_id *id)
{
	struct us_data *us;
	int result;

	result = usb_stor_probe1(&us, intf, id,
			(id - cypress_usb_ids) + cypress_unusual_dev_list);
	if (result)
		return result;

	us->protocol_name = "Transparent SCSI with Cypress ATACB";
	us->proto_handler = cypress_atacb_passthrough;

	result = usb_stor_probe2(us);
	return result;
}

static struct usb_driver cypress_driver = {
	.name =		"ums-cypress",
	.probe =	cypress_probe,
	.disconnect =	usb_stor_disconnect,
	.suspend =	usb_stor_suspend,
	.resume =	usb_stor_resume,
	.reset_resume =	usb_stor_reset_resume,
	.pre_reset =	usb_stor_pre_reset,
	.post_reset =	usb_stor_post_reset,
	.id_table =	cypress_usb_ids,
	.soft_unbind =	1,
	.no_dynamic_id = 1,
};

module_usb_driver(cypress_driver);
