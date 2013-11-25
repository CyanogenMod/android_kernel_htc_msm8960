/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Global definitions for the IP router interface.
 *
 * Version:	@(#)route.h	1.0.3	05/27/93
 *
 * Authors:	Original taken from Berkeley UNIX 4.3, (c) UCB 1986-1988
 *		for the purposes of compatibility only.
 *
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *
 * Changes:
 *              Mike McLagan    :       Routing by source
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
#ifndef _LINUX_ROUTE_H
#define _LINUX_ROUTE_H

#include <linux/if.h>
#include <linux/compiler.h>

struct rtentry {
	unsigned long	rt_pad1;
	struct sockaddr	rt_dst;		
	struct sockaddr	rt_gateway;	
	struct sockaddr	rt_genmask;	
	unsigned short	rt_flags;
	short		rt_pad2;
	unsigned long	rt_pad3;
	void		*rt_pad4;
	short		rt_metric;	
	char __user	*rt_dev;	
	unsigned long	rt_mtu;		
#ifndef __KERNEL__
#define rt_mss	rt_mtu			
#endif
	unsigned long	rt_window;	
	unsigned short	rt_irtt;	
};


#define	RTF_UP		0x0001		
#define	RTF_GATEWAY	0x0002		
#define	RTF_HOST	0x0004		
#define RTF_REINSTATE	0x0008		
#define	RTF_DYNAMIC	0x0010		
#define	RTF_MODIFIED	0x0020		
#define RTF_MTU		0x0040		
#define RTF_MSS		RTF_MTU		
#define RTF_WINDOW	0x0080		
#define RTF_IRTT	0x0100		
#define RTF_REJECT	0x0200		




#endif	

