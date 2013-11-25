/*
 * Definitions for ioctls to access DHD iovars.
 * Based on wlioctl.h (for Broadcom 802.11abg driver).
 * (Moves towards generic ioctls for BCM drivers/iovars.)
 *
 * Definitions subject to change without notice.
 *
 * Copyright (C) 1999-2012, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: dhdioctl.h 358413 2012-09-24 04:50:47Z $
 */

#ifndef _dhdioctl_h_
#define	_dhdioctl_h_

#include <typedefs.h>


#define BWL_DEFAULT_PACKING
#include <packed_section_start.h>


typedef struct dhd_ioctl {
	uint cmd;	
	void *buf;	
	uint len;	
	bool set;	
	uint used;	/* bytes read or written (optional) */
	uint needed;	
	uint driver;	
} dhd_ioctl_t;

enum {
	BUS_TYPE_USB = 0, 
	BUS_TYPE_SDIO 
};

#define DHD_IOCTL_MAGIC		0x00444944

#define DHD_IOCTL_VERSION	1

#define	DHD_IOCTL_MAXLEN	8192		
#define	DHD_IOCTL_SMLEN		256		

#define DHD_GET_MAGIC				0
#define DHD_GET_VERSION				1
#define DHD_GET_VAR				2
#define DHD_SET_VAR				3

#define DHD_ERROR_VAL	0x0001
#define DHD_TRACE_VAL	0x0002
#define DHD_INFO_VAL	0x0004
#define DHD_DATA_VAL	0x0008
#define DHD_CTL_VAL	0x0010
#define DHD_TIMER_VAL	0x0020
#define DHD_HDRS_VAL	0x0040
#define DHD_BYTES_VAL	0x0080
#define DHD_INTR_VAL	0x0100
#define DHD_LOG_VAL	0x0200
#define DHD_GLOM_VAL	0x0400
#define DHD_EVENT_VAL	0x0800
#define DHD_BTA_VAL	0x1000
#if 0 && (NDISVER >= 0x0630) && 1
#define DHD_SCAN_VAL	0x2000
#else
#define DHD_ISCAN_VAL	0x2000
#endif
#define DHD_ARPOE_VAL	0x4000
#define DHD_REORDER_VAL	0x8000
#define DHD_WL_VAL		0x10000
#define DHD_NOCHECKDIED_VAL		0x20000 
#define DHD_WL_VAL2		0x40000

#ifdef SDTEST
typedef struct dhd_pktgen {
	uint version;		
	uint freq;		
	uint count;		
	uint print;		
	uint total;		
	uint minlen;		
	uint maxlen;		
	uint numsent;		
	uint numrcvd;		
	uint numfail;		
	uint mode;		
	uint stop;		
} dhd_pktgen_t;

#define DHD_PKTGEN_VERSION 2

#define DHD_PKTGEN_ECHO		1 
#define DHD_PKTGEN_SEND 	2 
#define DHD_PKTGEN_RXBURST	3 
#define DHD_PKTGEN_RECV		4 
#endif 

#define DHD_IDLE_IMMEDIATE	(-1)

#define DHD_IDLE_ACTIVE	0	
#define DHD_IDLE_STOP   (-1)	


#include <packed_section_end.h>

#endif 
