/***************************************************************************
 * Linux PPP over L2TP (PPPoL2TP) Socket Implementation (RFC 2661)
 *
 * This file supplies definitions required by the PPP over L2TP driver
 * (l2tp_ppp.c).  All version information wrt this file is located in l2tp_ppp.c
 *
 * License:
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 */

#ifndef __LINUX_IF_PPPOL2TP_H
#define __LINUX_IF_PPPOL2TP_H

#include <linux/types.h>

#ifdef __KERNEL__
#include <linux/in.h>
#endif

struct pppol2tp_addr {
	__kernel_pid_t	pid;		
	int	fd;			

	struct sockaddr_in addr;	

	__u16 s_tunnel, s_session;	
	__u16 d_tunnel, d_session;	
};

struct pppol2tpv3_addr {
	__kernel_pid_t	pid;		
	int	fd;			

	struct sockaddr_in addr;	

	__u32 s_tunnel, s_session;	
	__u32 d_tunnel, d_session;	
};

enum {
	PPPOL2TP_SO_DEBUG	= 1,
	PPPOL2TP_SO_RECVSEQ	= 2,
	PPPOL2TP_SO_SENDSEQ	= 3,
	PPPOL2TP_SO_LNSMODE	= 4,
	PPPOL2TP_SO_REORDERTO	= 5,
};

enum {
	PPPOL2TP_MSG_DEBUG	= (1 << 0),	
	PPPOL2TP_MSG_CONTROL	= (1 << 1),	
	PPPOL2TP_MSG_SEQ	= (1 << 2),	
	PPPOL2TP_MSG_DATA	= (1 << 3),	
};



#endif
