/*
 * DLCI/FRAD	Definitions for Frame Relay Access Devices.  DLCI devices are
 *		created for each DLCI associated with a FRAD.  The FRAD driver
 *		is not truly a network device, but the lower level device
 *		handler.  This allows other FRAD manufacturers to use the DLCI
 *		code, including its RFC1490 encapsulation alongside the current
 *		implementation for the Sangoma cards.
 *
 * Version:	@(#)if_ifrad.h	0.15	31 Mar 96
 *
 * Author:	Mike McLagan <mike.mclagan@linux.org>
 *
 * Changes:
 *		0.15	Mike McLagan	changed structure defs (packed)
 *					re-arranged flags
 *					added DLCI_RET vars
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */

#ifndef _FRAD_H_
#define _FRAD_H_

#include <linux/if.h>


struct dlci_add
{
   char  devname[IFNAMSIZ];
   short dlci;
};

#define DLCI_GET_CONF	(SIOCDEVPRIVATE + 2)
#define DLCI_SET_CONF	(SIOCDEVPRIVATE + 3)

struct dlci_conf {
   short flags;
   short CIR_fwd;
   short Bc_fwd;
   short Be_fwd;
   short CIR_bwd;
   short Bc_bwd;
   short Be_bwd; 

   short Tc_fwd;
   short Tc_bwd;
   short Tf_max;
   short Tb_max;

};

#define DLCI_GET_SLAVE	(SIOCDEVPRIVATE + 4)

#define DLCI_IGNORE_CIR_OUT	0x0001
#define DLCI_ACCOUNT_CIR_IN	0x0002
#define DLCI_BUFFER_IF		0x0008

#define DLCI_VALID_FLAGS	0x000B

#define FRAD_GET_CONF	(SIOCDEVPRIVATE)
#define FRAD_SET_CONF	(SIOCDEVPRIVATE + 1)

#define FRAD_LAST_IOCTL	FRAD_SET_CONF

struct frad_conf 
{
   short station;
   short flags;
   short kbaud;
   short clocking;
   short mtu;
   short T391;
   short T392;
   short N391;
   short N392;
   short N393;
   short CIR_fwd;
   short Bc_fwd;
   short Be_fwd;
   short CIR_bwd;
   short Bc_bwd;
   short Be_bwd;


};

#define FRAD_STATION_CPE	0x0000
#define FRAD_STATION_NODE	0x0001

#define FRAD_TX_IGNORE_CIR	0x0001
#define FRAD_RX_ACCOUNT_CIR	0x0002
#define FRAD_DROP_ABORTED	0x0004
#define FRAD_BUFFERIF		0x0008
#define FRAD_STATS		0x0010
#define FRAD_MCI		0x0100
#define FRAD_AUTODLCI		0x8000
#define FRAD_VALID_FLAGS	0x811F

#define FRAD_CLOCK_INT		0x0001
#define FRAD_CLOCK_EXT		0x0000

#ifdef __KERNEL__

#if defined(CONFIG_DLCI) || defined(CONFIG_DLCI_MODULE)

struct frhdr
{
   unsigned char  control;

   
   unsigned char  pad;

   unsigned char  NLPID;
   unsigned char  OUI[3];
   __be16 PID;

#define IP_NLPID pad 
} __packed;

#define FRAD_I_UI		0x03

#define FRAD_P_PADDING		0x00
#define FRAD_P_Q933		0x08
#define FRAD_P_SNAP		0x80
#define FRAD_P_CLNP		0x81
#define FRAD_P_IP		0xCC

struct dlci_local
{
   struct net_device      *master;
   struct net_device      *slave;
   struct dlci_conf       config;
   int                    configured;
   struct list_head	  list;

   
   void              (*receive)(struct sk_buff *skb, struct net_device *);
};

struct frad_local
{
   struct net_device_stats stats;

   
   struct net_device     *master[CONFIG_DLCI_MAX];
   short             dlci[CONFIG_DLCI_MAX];

   struct frad_conf  config;
   int               configured;	
   int               initialized;	

   
   int               (*activate)(struct net_device *, struct net_device *);
   int               (*deactivate)(struct net_device *, struct net_device *);
   int               (*assoc)(struct net_device *, struct net_device *);
   int               (*deassoc)(struct net_device *, struct net_device *);
   int               (*dlci_conf)(struct net_device *, struct net_device *, int get);

   
   struct timer_list timer;
   int               type;		
   int               state;		
   int               buffer;		
};

#endif 

extern void dlci_ioctl_set(int (*hook)(unsigned int, void __user *));

#endif 

#endif
