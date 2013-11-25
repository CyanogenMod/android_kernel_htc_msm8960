/*
 * Copyright (C) 2013, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * Fundamental constants relating to 802.3
 *
 * $Id: 802.3.h 417942 2013-08-13 07:53:57Z $
 */

#ifndef _802_3_h_
#define _802_3_h_

#include <packed_section_start.h>

#define SNAP_HDR_LEN	6	
#define DOT3_OUI_LEN	3	

BWL_PRE_PACKED_STRUCT struct dot3_mac_llc_snap_header {
	uint8	ether_dhost[ETHER_ADDR_LEN];	
	uint8	ether_shost[ETHER_ADDR_LEN];	
	uint16	length;				
	uint8	dsap;				
	uint8	ssap;				
	uint8	ctl;				
	uint8	oui[DOT3_OUI_LEN];		
	uint16	type;				
} BWL_POST_PACKED_STRUCT;

#include <packed_section_end.h>

#endif	
