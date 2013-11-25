/*
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 */

#ifndef __F_CCID_H
#define __F_CCID_H

#define PROTOCOL_TO 0x01
#define PROTOCOL_T1 0x02
#define ABDATA_SIZE 512

#define CCID_FEATURES_NADA       0x00000000
#define CCID_FEATURES_AUTO_PCONF 0x00000002
#define CCID_FEATURES_AUTO_ACTIV 0x00000004
#define CCID_FEATURES_AUTO_VOLT  0x00000008
#define CCID_FEATURES_AUTO_CLOCK 0x00000010
#define CCID_FEATURES_AUTO_BAUD  0x00000020
#define CCID_FEATURES_AUTO_PNEGO 0x00000040
#define CCID_FEATURES_AUTO_PPS   0x00000080
#define CCID_FEATURES_ICCSTOP    0x00000100
#define CCID_FEATURES_NAD        0x00000200
#define CCID_FEATURES_AUTO_IFSD  0x00000400
#define CCID_FEATURES_EXC_TPDU   0x00010000
#define CCID_FEATURES_EXC_SAPDU  0x00020000
#define CCID_FEATURES_EXC_APDU   0x00040000
#define CCID_FEATURES_WAKEUP     0x00100000

#define CCID_NOTIFY_CARD	_IOW('C', 1, struct usb_ccid_notification)
#define CCID_NOTIFY_HWERROR	_IOW('C', 2, struct usb_ccid_notification)
#define CCID_READ_DTR		_IOR('C', 3, int)

struct usb_ccid_notification {
	unsigned char buf[4];
} __packed;

struct ccid_bulk_in_header {
	unsigned char bMessageType;
	unsigned long wLength;
	unsigned char bSlot;
	unsigned char bSeq;
	unsigned char bStatus;
	unsigned char bError;
	unsigned char bSpecific;
	unsigned char abData[ABDATA_SIZE];
	unsigned char bSizeToSend;
} __packed;

struct ccid_bulk_out_header {
	unsigned char bMessageType;
	unsigned long wLength;
	unsigned char bSlot;
	unsigned char bSeq;
	unsigned char bSpecific_0;
	unsigned char bSpecific_1;
	unsigned char bSpecific_2;
	unsigned char APDU[ABDATA_SIZE];
} __packed;
#endif
