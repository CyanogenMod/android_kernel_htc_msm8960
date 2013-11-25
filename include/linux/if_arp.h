/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Global definitions for the ARP (RFC 826) protocol.
 *
 * Version:	@(#)if_arp.h	1.0.1	04/16/93
 *
 * Authors:	Original taken from Berkeley UNIX 4.3, (c) UCB 1986-1988
 *		Portions taken from the KA9Q/NOS (v2.00m PA0GRI) source.
 *		Ross Biro
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *		Florian La Roche,
 *		Jonathan Layes <layes@loran.com>
 *		Arnaldo Carvalho de Melo <acme@conectiva.com.br> ARPHRD_HWX25
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
#ifndef _LINUX_IF_ARP_H
#define _LINUX_IF_ARP_H

#include <linux/netdevice.h>

#define ARPHRD_NETROM	0		
#define ARPHRD_ETHER 	1		
#define	ARPHRD_EETHER	2		
#define	ARPHRD_AX25	3		
#define	ARPHRD_PRONET	4		
#define	ARPHRD_CHAOS	5		
#define	ARPHRD_IEEE802	6		
#define	ARPHRD_ARCNET	7		
#define	ARPHRD_APPLETLK	8		
#define ARPHRD_DLCI	15		
#define ARPHRD_ATM	19		
#define ARPHRD_METRICOM	23		
#define	ARPHRD_IEEE1394	24		
#define ARPHRD_EUI64	27		
#define ARPHRD_INFINIBAND 32		

#define ARPHRD_SLIP	256
#define ARPHRD_CSLIP	257
#define ARPHRD_SLIP6	258
#define ARPHRD_CSLIP6	259
#define ARPHRD_RSRVD	260		
#define ARPHRD_ADAPT	264
#define ARPHRD_ROSE	270
#define ARPHRD_X25	271		
#define ARPHRD_HWX25	272		
#define ARPHRD_CAN	280		
#define ARPHRD_PPP	512
#define ARPHRD_CISCO	513		
#define ARPHRD_HDLC	ARPHRD_CISCO
#define ARPHRD_LAPB	516		
#define ARPHRD_DDCMP    517		
#define ARPHRD_RAWHDLC	518		
#define ARPHRD_RAWIP	530	        

#define ARPHRD_TUNNEL	768		
#define ARPHRD_TUNNEL6	769		
#define ARPHRD_FRAD	770             
#define ARPHRD_SKIP	771		
#define ARPHRD_LOOPBACK	772		
#define ARPHRD_LOCALTLK 773		
#define ARPHRD_FDDI	774		
#define ARPHRD_BIF      775             
#define ARPHRD_SIT	776		
#define ARPHRD_IPDDP	777		
#define ARPHRD_IPGRE	778		
#define ARPHRD_PIMREG	779		
#define ARPHRD_HIPPI	780		
#define ARPHRD_ASH	781		
#define ARPHRD_ECONET	782		
#define ARPHRD_IRDA 	783		
#define ARPHRD_FCPP	784		
#define ARPHRD_FCAL	785		
#define ARPHRD_FCPL	786		
#define ARPHRD_FCFABRIC	787		
	
#define ARPHRD_IEEE802_TR 800		
#define ARPHRD_IEEE80211 801		
#define ARPHRD_IEEE80211_PRISM 802	
#define ARPHRD_IEEE80211_RADIOTAP 803	
#define ARPHRD_IEEE802154	  804

#define ARPHRD_PHONET	820		
#define ARPHRD_PHONET_PIPE 821		
#define ARPHRD_CAIF	822		

#define ARPHRD_VOID	  0xFFFF	
#define ARPHRD_NONE	  0xFFFE	

#define	ARPOP_REQUEST	1		
#define	ARPOP_REPLY	2		
#define	ARPOP_RREQUEST	3		
#define	ARPOP_RREPLY	4		
#define	ARPOP_InREQUEST	8		
#define	ARPOP_InREPLY	9		
#define	ARPOP_NAK	10		


struct arpreq {
  struct sockaddr	arp_pa;		
  struct sockaddr	arp_ha;		
  int			arp_flags;	
  struct sockaddr       arp_netmask;    
  char			arp_dev[16];
};

struct arpreq_old {
  struct sockaddr	arp_pa;		
  struct sockaddr	arp_ha;		
  int			arp_flags;	
  struct sockaddr       arp_netmask;    
};

#define ATF_COM		0x02		
#define	ATF_PERM	0x04		
#define	ATF_PUBL	0x08		
#define	ATF_USETRAILERS	0x10		
#define ATF_NETMASK     0x20            
#define ATF_DONTPUB	0x40		


struct arphdr {
	__be16		ar_hrd;		
	__be16		ar_pro;		
	unsigned char	ar_hln;		
	unsigned char	ar_pln;		
	__be16		ar_op;		

#if 0
	unsigned char		ar_sha[ETH_ALEN];	
	unsigned char		ar_sip[4];		
	unsigned char		ar_tha[ETH_ALEN];	
	unsigned char		ar_tip[4];		
#endif

};

#ifdef __KERNEL__
#include <linux/skbuff.h>

static inline struct arphdr *arp_hdr(const struct sk_buff *skb)
{
	return (struct arphdr *)skb_network_header(skb);
}

static inline int arp_hdr_len(struct net_device *dev)
{
	
	return sizeof(struct arphdr) + (dev->addr_len + sizeof(u32)) * 2;
}
#endif

#endif	
