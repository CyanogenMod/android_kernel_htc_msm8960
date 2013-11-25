#ifndef _SLHC_H
#define _SLHC_H
/*
 * Definitions for tcp compression routines.
 *
 * $Header: slcompress.h,v 1.10 89/12/31 08:53:02 van Exp $
 *
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	Van Jacobson (van@helios.ee.lbl.gov), Dec 31, 1989:
 *	- Initial distribution.
 *
 *
 * modified for KA9Q Internet Software Package by
 * Katie Stevens (dkstevens@ucdavis.edu)
 * University of California, Davis
 * Computing Services
 *	- 01-31-90	initial adaptation
 *
 *	- Feb 1991	Bill_Simpson@um.cc.umich.edu
 *			variable number of conversation slots
 *			allow zero or one slots
 *			separate routines
 *			status display
 */




#include <linux/ip.h>
#include <linux/tcp.h>

#define SL_TYPE_IP 0x40
#define SL_TYPE_UNCOMPRESSED_TCP 0x70
#define SL_TYPE_COMPRESSED_TCP 0x80
#define SL_TYPE_ERROR 0x00

#define NEW_C	0x40	
#define NEW_I	0x20
#define NEW_S	0x08
#define NEW_A	0x04
#define NEW_W	0x02
#define NEW_U	0x01

#define SPECIAL_I (NEW_S|NEW_W|NEW_U)		
#define SPECIAL_D (NEW_S|NEW_A|NEW_W|NEW_U)	
#define SPECIALS_MASK (NEW_S|NEW_A|NEW_W|NEW_U)

#define TCP_PUSH_BIT 0x10


typedef __u8 byte_t;
typedef __u32 int32;

struct cstate {
	byte_t	cs_this;	
	struct cstate *next;	
	struct iphdr cs_ip;	
	struct tcphdr cs_tcp;
	unsigned char cs_ipopt[64];
	unsigned char cs_tcpopt[64];
	int cs_hsize;
};
#define NULLSLSTATE	(struct cstate *)0

struct slcompress {
	struct cstate *tstate;	
	struct cstate *rstate;	

	byte_t tslot_limit;	
	byte_t rslot_limit;	

	byte_t xmit_oldest;	
	byte_t xmit_current;	
	byte_t recv_current;	

	byte_t flags;
#define SLF_TOSS	0x01	

	int32 sls_o_nontcp;	
	int32 sls_o_tcp;	
	int32 sls_o_uncompressed;	
	int32 sls_o_compressed;	
	int32 sls_o_searches;	
	int32 sls_o_misses;	

	int32 sls_i_uncompressed;	
	int32 sls_i_compressed;	
	int32 sls_i_error;	
	int32 sls_i_tossed;	

	int32 sls_i_runt;
	int32 sls_i_badcheck;
};
#define NULLSLCOMPR	(struct slcompress *)0

struct slcompress *slhc_init(int rslots, int tslots);
void slhc_free(struct slcompress *comp);

int slhc_compress(struct slcompress *comp, unsigned char *icp, int isize,
		  unsigned char *ocp, unsigned char **cpp, int compress_cid);
int slhc_uncompress(struct slcompress *comp, unsigned char *icp, int isize);
int slhc_remember(struct slcompress *comp, unsigned char *icp, int isize);
int slhc_toss(struct slcompress *comp);

#endif	
