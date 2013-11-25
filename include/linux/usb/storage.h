#ifndef __LINUX_USB_STORAGE_H
#define __LINUX_USB_STORAGE_H

/*
 * linux/usb/storage.h
 *
 * Copyright Matthew Wilcox for Intel Corp, 2010
 *
 * This file contains definitions taken from the
 * USB Mass Storage Class Specification Overview
 *
 * Distributed under the terms of the GNU GPL, version two.
 */


#define USB_SC_RBC	0x01		
#define USB_SC_8020	0x02		
#define USB_SC_QIC	0x03		
#define USB_SC_UFI	0x04		
#define USB_SC_8070	0x05		
#define USB_SC_SCSI	0x06		
#define USB_SC_LOCKABLE	0x07		

#define USB_SC_ISD200	0xf0		
#define USB_SC_CYP_ATACB	0xf1	
#define USB_SC_DEVICE	0xff		


#define USB_PR_CBI	0x00		
#define USB_PR_CB	0x01		
#define USB_PR_BULK	0x50		
#define USB_PR_UAS	0x62		

#define USB_PR_USBAT	0x80		
#define USB_PR_EUSB_SDDR09	0x81	
#define USB_PR_SDDR55	0x82		
#define USB_PR_DPCM_USB	0xf0		
#define USB_PR_FREECOM	0xf1		
#define USB_PR_DATAFAB	0xf2		
#define USB_PR_JUMPSHOT	0xf3		
#define USB_PR_ALAUDA	0xf4		
#define USB_PR_KARMA	0xf5		

#define USB_PR_DEVICE	0xff		


struct bulk_cb_wrap {
	__le32	Signature;		
	__u32	Tag;			
	__le32	DataTransferLength;	
	__u8	Flags;			
	__u8	Lun;			
	__u8	Length;			
	__u8	CDB[16];		
};

#define US_BULK_CB_WRAP_LEN	31
#define US_BULK_CB_SIGN		0x43425355	
#define US_BULK_FLAG_IN		(1 << 7)
#define US_BULK_FLAG_OUT	0

struct bulk_cs_wrap {
	__le32	Signature;	
	__u32	Tag;		
	__le32	Residue;	
	__u8	Status;		
};

#define US_BULK_CS_WRAP_LEN	13
#define US_BULK_CS_SIGN		0x53425355      
#define US_BULK_STAT_OK		0
#define US_BULK_STAT_FAIL	1
#define US_BULK_STAT_PHASE	2

#define US_BULK_RESET_REQUEST   0xff
#define US_BULK_GET_MAX_LUN     0xfe

#endif
