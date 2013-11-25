/*
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
* $Id: wlfc_proto.h 358262 2012-09-21 21:39:29Z $
*
*/
#ifndef __wlfc_proto_definitions_h__
#define __wlfc_proto_definitions_h__


#define WLFC_CTL_TYPE_MAC_OPEN			1
#define WLFC_CTL_TYPE_MAC_CLOSE			2
#define WLFC_CTL_TYPE_MAC_REQUEST_CREDIT	3
#define WLFC_CTL_TYPE_TXSTATUS			4
#define WLFC_CTL_TYPE_PKTTAG			5

#define WLFC_CTL_TYPE_MACDESC_ADD		6
#define WLFC_CTL_TYPE_MACDESC_DEL		7
#define WLFC_CTL_TYPE_RSSI			8

#define WLFC_CTL_TYPE_INTERFACE_OPEN		9
#define WLFC_CTL_TYPE_INTERFACE_CLOSE		10

#define WLFC_CTL_TYPE_FIFO_CREDITBACK		11

#define WLFC_CTL_TYPE_PENDING_TRAFFIC_BMP	12
#define WLFC_CTL_TYPE_MAC_REQUEST_PACKET	13
#define WLFC_CTL_TYPE_HOST_REORDER_RXPKTS	14

#define WLFC_CTL_TYPE_TRANS_ID			18
#define WLFC_CTL_TYPE_COMP_TXSTATUS		19


#define WLFC_CTL_TYPE_FILLER			255

#define WLFC_CTL_VALUE_LEN_MACDESC		8	

#define WLFC_CTL_VALUE_LEN_MAC			1	
#define WLFC_CTL_VALUE_LEN_RSSI			1

#define WLFC_CTL_VALUE_LEN_INTERFACE		1
#define WLFC_CTL_VALUE_LEN_PENDING_TRAFFIC_BMP	2

#define WLFC_CTL_VALUE_LEN_TXSTATUS		4
#define WLFC_CTL_VALUE_LEN_PKTTAG		4

#define WLFC_CTL_VALUE_LEN_FIFO_CREDITBACK	6

#define WLFC_CTL_VALUE_LEN_REQUEST_CREDIT	3	
#define WLFC_CTL_VALUE_LEN_REQUEST_PACKET	3	


#define WLFC_PKTID_GEN_MASK		0x80000000
#define WLFC_PKTID_GEN_SHIFT	31

#define WLFC_PKTID_GEN(x)	(((x) & WLFC_PKTID_GEN_MASK) >> WLFC_PKTID_GEN_SHIFT)
#define WLFC_PKTID_SETGEN(x, gen)	(x) = ((x) & ~WLFC_PKTID_GEN_MASK) | \
	(((gen) << WLFC_PKTID_GEN_SHIFT) & WLFC_PKTID_GEN_MASK)

#define WLFC_PKTFLAG_PKTFROMHOST	0x01
#define WLFC_PKTFLAG_PKT_REQUESTED	0x02

#define WL_TXSTATUS_FLAGS_MASK			0xf 
#define WL_TXSTATUS_FLAGS_SHIFT			27

#define WL_TXSTATUS_SET_FLAGS(x, flags)	((x)  = \
	((x) & ~(WL_TXSTATUS_FLAGS_MASK << WL_TXSTATUS_FLAGS_SHIFT)) | \
	(((flags) & WL_TXSTATUS_FLAGS_MASK) << WL_TXSTATUS_FLAGS_SHIFT))
#define WL_TXSTATUS_GET_FLAGS(x)		(((x) >> WL_TXSTATUS_FLAGS_SHIFT) & \
	WL_TXSTATUS_FLAGS_MASK)

#define WL_TXSTATUS_FIFO_MASK			0x7 
#define WL_TXSTATUS_FIFO_SHIFT			24

#define WL_TXSTATUS_SET_FIFO(x, flags)	((x)  = \
	((x) & ~(WL_TXSTATUS_FIFO_MASK << WL_TXSTATUS_FIFO_SHIFT)) | \
	(((flags) & WL_TXSTATUS_FIFO_MASK) << WL_TXSTATUS_FIFO_SHIFT))
#define WL_TXSTATUS_GET_FIFO(x)		(((x) >> WL_TXSTATUS_FIFO_SHIFT) & WL_TXSTATUS_FIFO_MASK)

#define WL_TXSTATUS_PKTID_MASK			0xffffff 
#define WL_TXSTATUS_SET_PKTID(x, num)	((x) = \
	((x) & ~WL_TXSTATUS_PKTID_MASK) | (num))
#define WL_TXSTATUS_GET_PKTID(x)		((x) & WL_TXSTATUS_PKTID_MASK)

#define WLFC_MAC_DESC_TABLE_SIZE	32
#define WLFC_MAX_IFNUM				16
#define WLFC_MAC_DESC_ID_INVALID	0xff

#define WLFC_MAC_DESC_GET_LOOKUP_INDEX(x) ((x) & 0x1f)

#define WLFC_PKTFLAG_SET_PKTREQUESTED(x)	(x) |= \
	(WLFC_PKTFLAG_PKT_REQUESTED << WL_TXSTATUS_FLAGS_SHIFT)

#define WLFC_PKTFLAG_CLR_PKTREQUESTED(x)	(x) &= \
	~(WLFC_PKTFLAG_PKT_REQUESTED << WL_TXSTATUS_FLAGS_SHIFT)

#define WL_TXSTATUS_GENERATION_MASK			1
#define WL_TXSTATUS_GENERATION_SHIFT		31

#define WLFC_PKTFLAG_SET_GENERATION(x, gen)	((x) = \
	((x) & ~(WL_TXSTATUS_GENERATION_MASK << WL_TXSTATUS_GENERATION_SHIFT)) | \
	(((gen) & WL_TXSTATUS_GENERATION_MASK) << WL_TXSTATUS_GENERATION_SHIFT))

#define WLFC_PKTFLAG_GENERATION(x)	(((x) >> WL_TXSTATUS_GENERATION_SHIFT) & \
	WL_TXSTATUS_GENERATION_MASK)

#define WLFC_MAX_PENDING_DATALEN	120

#define WLFC_CTL_PKTFLAG_DISCARD		0
#define WLFC_CTL_PKTFLAG_D11SUPPRESS	1
#define WLFC_CTL_PKTFLAG_WLSUPPRESS		2
#define WLFC_CTL_PKTFLAG_TOSSED_BYWLC	3

#define WLFC_D11_STATUS_INTERPRET(txs)	\
	(((txs)->status.suppr_ind != 0) ? WLFC_CTL_PKTFLAG_D11SUPPRESS : WLFC_CTL_PKTFLAG_DISCARD)

#ifdef PROP_TXSTATUS_DEBUG
#define WLFC_DBGMESG(x) printf x
#define WLFC_BREADCRUMB(x) do {if ((x) == NULL) \
	{printf("WLFC: %s():%d:caller:%p\n", \
	__FUNCTION__, __LINE__, __builtin_return_address(0));}} while (0)
#define WLFC_PRINTMAC(banner, ea) do {printf("%s MAC: [%02x:%02x:%02x:%02x:%02x:%02x]\n", \
	banner, ea[0], 	ea[1], 	ea[2], 	ea[3], 	ea[4], 	ea[5]); } while (0)
#define WLFC_WHEREIS(s) printf("WLFC: at %s():%d, %s\n", __FUNCTION__, __LINE__, (s))
#else
#define WLFC_DBGMESG(x)
#define WLFC_BREADCRUMB(x)
#define WLFC_PRINTMAC(banner, ea)
#define WLFC_WHEREIS(s)
#endif

#define WLHOST_REORDERDATA_MAXFLOWS		256
#define WLHOST_REORDERDATA_LEN		 10
#define WLHOST_REORDERDATA_TOTLEN	(WLHOST_REORDERDATA_LEN + 1 + 1) 

#define WLHOST_REORDERDATA_FLOWID_OFFSET		0
#define WLHOST_REORDERDATA_MAXIDX_OFFSET		2
#define WLHOST_REORDERDATA_FLAGS_OFFSET			4
#define WLHOST_REORDERDATA_CURIDX_OFFSET		6
#define WLHOST_REORDERDATA_EXPIDX_OFFSET		8

#define WLHOST_REORDERDATA_DEL_FLOW		0x01
#define WLHOST_REORDERDATA_FLUSH_ALL		0x02
#define WLHOST_REORDERDATA_CURIDX_VALID		0x04
#define WLHOST_REORDERDATA_EXPIDX_VALID		0x08
#define WLHOST_REORDERDATA_NEW_HOLE		0x10

#define WLFC_CTL_TRANS_ID_LEN			6

#endif 
