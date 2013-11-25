/*
 * Routines to compress and uncompress tcp packets (for transmission
 * over low speed serial lines).
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
 *	- 01-31-90	initial adaptation (from 1.19)
 *	PPP.05	02-15-90 [ks]
 *	PPP.08	05-02-90 [ks]	use PPP protocol field to signal compression
 *	PPP.15	09-90	 [ks]	improve mbuf handling
 *	PPP.16	11-02	 [karn]	substantially rewritten to use NOS facilities
 *
 *	- Feb 1991	Bill_Simpson@um.cc.umich.edu
 *			variable number of conversation slots
 *			allow zero or one slots
 *			separate routines
 *			status display
 *	- Jul 1994	Dmitry Gorodchanin
 *			Fixes for memory leaks.
 *      - Oct 1994      Dmitry Gorodchanin
 *                      Modularization.
 *	- Jan 1995	Bjorn Ekwall
 *			Use ip_fast_csum from ip.h
 *	- July 1995	Christos A. Polyzols
 *			Spotted bug in tcp option checking
 *
 *
 *	This module is a difficult issue. It's clearly inet code but it's also clearly
 *	driver code belonging close to PPP and SLIP
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <net/slhc_vj.h>

#ifdef CONFIG_INET
#include <linux/mm.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/termios.h>
#include <linux/in.h>
#include <linux/fcntl.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <net/ip.h>
#include <net/protocol.h>
#include <net/icmp.h>
#include <net/tcp.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/timer.h>
#include <asm/uaccess.h>
#include <net/checksum.h>
#include <asm/unaligned.h>

static unsigned char *encode(unsigned char *cp, unsigned short n);
static long decode(unsigned char **cpp);
static unsigned char * put16(unsigned char *cp, unsigned short x);
static unsigned short pull16(unsigned char **cpp);

struct slcompress *
slhc_init(int rslots, int tslots)
{
	register short i;
	register struct cstate *ts;
	struct slcompress *comp;

	comp = kzalloc(sizeof(struct slcompress), GFP_KERNEL);
	if (! comp)
		goto out_fail;

	if ( rslots > 0  &&  rslots < 256 ) {
		size_t rsize = rslots * sizeof(struct cstate);
		comp->rstate = kzalloc(rsize, GFP_KERNEL);
		if (! comp->rstate)
			goto out_free;
		comp->rslot_limit = rslots - 1;
	}

	if ( tslots > 0  &&  tslots < 256 ) {
		size_t tsize = tslots * sizeof(struct cstate);
		comp->tstate = kzalloc(tsize, GFP_KERNEL);
		if (! comp->tstate)
			goto out_free2;
		comp->tslot_limit = tslots - 1;
	}

	comp->xmit_oldest = 0;
	comp->xmit_current = 255;
	comp->recv_current = 255;
	comp->flags |= SLF_TOSS;

	if ( tslots > 0 ) {
		ts = comp->tstate;
		for(i = comp->tslot_limit; i > 0; --i){
			ts[i].cs_this = i;
			ts[i].next = &(ts[i - 1]);
		}
		ts[0].next = &(ts[comp->tslot_limit]);
		ts[0].cs_this = 0;
	}
	return comp;

out_free2:
	kfree(comp->rstate);
out_free:
	kfree(comp);
out_fail:
	return NULL;
}


void
slhc_free(struct slcompress *comp)
{
	if ( comp == NULLSLCOMPR )
		return;

	if ( comp->tstate != NULLSLSTATE )
		kfree( comp->tstate );

	if ( comp->rstate != NULLSLSTATE )
		kfree( comp->rstate );

	kfree( comp );
}


static inline unsigned char *
put16(unsigned char *cp, unsigned short x)
{
	*cp++ = x >> 8;
	*cp++ = x;

	return cp;
}


static unsigned char *
encode(unsigned char *cp, unsigned short n)
{
	if(n >= 256 || n == 0){
		*cp++ = 0;
		cp = put16(cp,n);
	} else {
		*cp++ = n;
	}
	return cp;
}

static unsigned short
pull16(unsigned char **cpp)
{
	short rval;

	rval = *(*cpp)++;
	rval <<= 8;
	rval |= *(*cpp)++;
	return rval;
}

static long
decode(unsigned char **cpp)
{
	register int x;

	x = *(*cpp)++;
	if(x == 0){
		return pull16(cpp) & 0xffff;	
	} else {
		return x & 0xff;		
	}
}


int
slhc_compress(struct slcompress *comp, unsigned char *icp, int isize,
	unsigned char *ocp, unsigned char **cpp, int compress_cid)
{
	register struct cstate *ocs = &(comp->tstate[comp->xmit_oldest]);
	register struct cstate *lcs = ocs;
	register struct cstate *cs = lcs->next;
	register unsigned long deltaS, deltaA;
	register short changes = 0;
	int hlen;
	unsigned char new_seq[16];
	register unsigned char *cp = new_seq;
	struct iphdr *ip;
	struct tcphdr *th, *oth;
	__sum16 csum;



	if(isize<sizeof(struct iphdr))
		return isize;

	ip = (struct iphdr *) icp;

	
	if (ip->protocol != IPPROTO_TCP || (ntohs(ip->frag_off) & 0x3fff)) {
		
		if(ip->protocol != IPPROTO_TCP)
			comp->sls_o_nontcp++;
		else
			comp->sls_o_tcp++;
		return isize;
	}
	

	th = (struct tcphdr *)(((unsigned char *)ip) + ip->ihl*4);
	hlen = ip->ihl*4 + th->doff*4;

	if(hlen > isize || th->syn || th->fin || th->rst ||
	    ! (th->ack)){
		
		comp->sls_o_tcp++;
		return isize;
	}
	for ( ; ; ) {
		if( ip->saddr == cs->cs_ip.saddr
		 && ip->daddr == cs->cs_ip.daddr
		 && th->source == cs->cs_tcp.source
		 && th->dest == cs->cs_tcp.dest)
			goto found;

		
		if ( cs == ocs )
			break;
		lcs = cs;
		cs = cs->next;
		comp->sls_o_searches++;
	}
	comp->sls_o_misses++;
	comp->xmit_oldest = lcs->cs_this;
	goto uncompressed;

found:
	if(lcs == ocs) {
 		
	} else if (cs == ocs) {
		
		comp->xmit_oldest = lcs->cs_this;
	} else {
		
		lcs->next = cs->next;
		cs->next = ocs->next;
		ocs->next = cs;
	}

	oth = &cs->cs_tcp;

	if(ip->version != cs->cs_ip.version || ip->ihl != cs->cs_ip.ihl
	 || ip->tos != cs->cs_ip.tos
	 || (ip->frag_off & htons(0x4000)) != (cs->cs_ip.frag_off & htons(0x4000))
	 || ip->ttl != cs->cs_ip.ttl
	 || th->doff != cs->cs_tcp.doff
	 || (ip->ihl > 5 && memcmp(ip+1,cs->cs_ipopt,((ip->ihl)-5)*4) != 0)
	 || (th->doff > 5 && memcmp(th+1,cs->cs_tcpopt,((th->doff)-5)*4) != 0)){
		goto uncompressed;
	}

	if(th->urg){
		deltaS = ntohs(th->urg_ptr);
		cp = encode(cp,deltaS);
		changes |= NEW_U;
	} else if(th->urg_ptr != oth->urg_ptr){
		goto uncompressed;
	}
	if((deltaS = ntohs(th->window) - ntohs(oth->window)) != 0){
		cp = encode(cp,deltaS);
		changes |= NEW_W;
	}
	if((deltaA = ntohl(th->ack_seq) - ntohl(oth->ack_seq)) != 0L){
		if(deltaA > 0x0000ffff)
			goto uncompressed;
		cp = encode(cp,deltaA);
		changes |= NEW_A;
	}
	if((deltaS = ntohl(th->seq) - ntohl(oth->seq)) != 0L){
		if(deltaS > 0x0000ffff)
			goto uncompressed;
		cp = encode(cp,deltaS);
		changes |= NEW_S;
	}

	switch(changes){
	case 0:	
		if(ip->tot_len != cs->cs_ip.tot_len &&
		   ntohs(cs->cs_ip.tot_len) == hlen)
			break;
		goto uncompressed;
		break;
	case SPECIAL_I:
	case SPECIAL_D:
		goto uncompressed;
	case NEW_S|NEW_A:
		if(deltaS == deltaA &&
		    deltaS == ntohs(cs->cs_ip.tot_len) - hlen){
			
			changes = SPECIAL_I;
			cp = new_seq;
		}
		break;
	case NEW_S:
		if(deltaS == ntohs(cs->cs_ip.tot_len) - hlen){
			
			changes = SPECIAL_D;
			cp = new_seq;
		}
		break;
	}
	deltaS = ntohs(ip->id) - ntohs(cs->cs_ip.id);
	if(deltaS != 1){
		cp = encode(cp,deltaS);
		changes |= NEW_I;
	}
	if(th->psh)
		changes |= TCP_PUSH_BIT;
	csum = th->check;
	memcpy(&cs->cs_ip,ip,20);
	memcpy(&cs->cs_tcp,th,20);
	deltaS = cp - new_seq;
	if(compress_cid == 0 || comp->xmit_current != cs->cs_this){
		cp = ocp;
		*cpp = ocp;
		*cp++ = changes | NEW_C;
		*cp++ = cs->cs_this;
		comp->xmit_current = cs->cs_this;
	} else {
		cp = ocp;
		*cpp = ocp;
		*cp++ = changes;
	}
	*(__sum16 *)cp = csum;
	cp += 2;
	memcpy(cp,new_seq,deltaS);	
	memcpy(cp+deltaS,icp+hlen,isize-hlen);
	comp->sls_o_compressed++;
	ocp[0] |= SL_TYPE_COMPRESSED_TCP;
	return isize - hlen + deltaS + (cp - ocp);

uncompressed:
	memcpy(&cs->cs_ip,ip,20);
	memcpy(&cs->cs_tcp,th,20);
	if (ip->ihl > 5)
	  memcpy(cs->cs_ipopt, ip+1, ((ip->ihl) - 5) * 4);
	if (th->doff > 5)
	  memcpy(cs->cs_tcpopt, th+1, ((th->doff) - 5) * 4);
	comp->xmit_current = cs->cs_this;
	comp->sls_o_uncompressed++;
	memcpy(ocp, icp, isize);
	*cpp = ocp;
	ocp[9] = cs->cs_this;
	ocp[0] |= SL_TYPE_UNCOMPRESSED_TCP;
	return isize;
}


int
slhc_uncompress(struct slcompress *comp, unsigned char *icp, int isize)
{
	register int changes;
	long x;
	register struct tcphdr *thp;
	register struct iphdr *ip;
	register struct cstate *cs;
	int len, hdrlen;
	unsigned char *cp = icp;

	
	comp->sls_i_compressed++;
	if(isize < 3){
		comp->sls_i_error++;
		return 0;
	}
	changes = *cp++;
	if(changes & NEW_C){
		x = *cp++;	
		if(x < 0 || x > comp->rslot_limit)
			goto bad;

		comp->flags &=~ SLF_TOSS;
		comp->recv_current = x;
	} else {
		if(comp->flags & SLF_TOSS){
			comp->sls_i_tossed++;
			return 0;
		}
	}
	cs = &comp->rstate[comp->recv_current];
	thp = &cs->cs_tcp;
	ip = &cs->cs_ip;

	thp->check = *(__sum16 *)cp;
	cp += 2;

	thp->psh = (changes & TCP_PUSH_BIT) ? 1 : 0;

	hdrlen = ip->ihl * 4 + thp->doff * 4;

	switch(changes & SPECIALS_MASK){
	case SPECIAL_I:		
		{
		register short i;
		i = ntohs(ip->tot_len) - hdrlen;
		thp->ack_seq = htonl( ntohl(thp->ack_seq) + i);
		thp->seq = htonl( ntohl(thp->seq) + i);
		}
		break;

	case SPECIAL_D:			
		thp->seq = htonl( ntohl(thp->seq) +
				  ntohs(ip->tot_len) - hdrlen);
		break;

	default:
		if(changes & NEW_U){
			thp->urg = 1;
			if((x = decode(&cp)) == -1) {
				goto bad;
			}
			thp->urg_ptr = htons(x);
		} else
			thp->urg = 0;
		if(changes & NEW_W){
			if((x = decode(&cp)) == -1) {
				goto bad;
			}
			thp->window = htons( ntohs(thp->window) + x);
		}
		if(changes & NEW_A){
			if((x = decode(&cp)) == -1) {
				goto bad;
			}
			thp->ack_seq = htonl( ntohl(thp->ack_seq) + x);
		}
		if(changes & NEW_S){
			if((x = decode(&cp)) == -1) {
				goto bad;
			}
			thp->seq = htonl( ntohl(thp->seq) + x);
		}
		break;
	}
	if(changes & NEW_I){
		if((x = decode(&cp)) == -1) {
			goto bad;
		}
		ip->id = htons (ntohs (ip->id) + x);
	} else
		ip->id = htons (ntohs (ip->id) + 1);


	len = isize - (cp - icp);
	if (len < 0)
		goto bad;
	len += hdrlen;
	ip->tot_len = htons(len);
	ip->check = 0;

	memmove(icp + hdrlen, cp, len - hdrlen);

	cp = icp;
	memcpy(cp, ip, 20);
	cp += 20;

	if (ip->ihl > 5) {
	  memcpy(cp, cs->cs_ipopt, (ip->ihl - 5) * 4);
	  cp += (ip->ihl - 5) * 4;
	}

	put_unaligned(ip_fast_csum(icp, ip->ihl),
		      &((struct iphdr *)icp)->check);

	memcpy(cp, thp, 20);
	cp += 20;

	if (thp->doff > 5) {
	  memcpy(cp, cs->cs_tcpopt, ((thp->doff) - 5) * 4);
	  cp += ((thp->doff) - 5) * 4;
	}

	return len;
bad:
	comp->sls_i_error++;
	return slhc_toss( comp );
}


int
slhc_remember(struct slcompress *comp, unsigned char *icp, int isize)
{
	register struct cstate *cs;
	unsigned ihl;

	unsigned char index;

	if(isize < 20) {
		
		comp->sls_i_runt++;
		return slhc_toss( comp );
	}
	
	ihl = icp[0] & 0xf;
	if(ihl < 20 / 4){
		
		comp->sls_i_runt++;
		return slhc_toss( comp );
	}
	index = icp[9];
	icp[9] = IPPROTO_TCP;

	if (ip_fast_csum(icp, ihl)) {
		
		comp->sls_i_badcheck++;
		return slhc_toss( comp );
	}
	if(index > comp->rslot_limit) {
		comp->sls_i_error++;
		return slhc_toss(comp);
	}

	
	cs = &comp->rstate[comp->recv_current = index];
	comp->flags &=~ SLF_TOSS;
	memcpy(&cs->cs_ip,icp,20);
	memcpy(&cs->cs_tcp,icp + ihl*4,20);
	if (ihl > 5)
	  memcpy(cs->cs_ipopt, icp + sizeof(struct iphdr), (ihl - 5) * 4);
	if (cs->cs_tcp.doff > 5)
	  memcpy(cs->cs_tcpopt, icp + ihl*4 + sizeof(struct tcphdr), (cs->cs_tcp.doff - 5) * 4);
	cs->cs_hsize = ihl*2 + cs->cs_tcp.doff*2;
	comp->sls_i_uncompressed++;
	return isize;
}

int
slhc_toss(struct slcompress *comp)
{
	if ( comp == NULLSLCOMPR )
		return 0;

	comp->flags |= SLF_TOSS;
	return 0;
}

#else 

int
slhc_toss(struct slcompress *comp)
{
  printk(KERN_DEBUG "Called IP function on non IP-system: slhc_toss");
  return -EINVAL;
}
int
slhc_uncompress(struct slcompress *comp, unsigned char *icp, int isize)
{
  printk(KERN_DEBUG "Called IP function on non IP-system: slhc_uncompress");
  return -EINVAL;
}
int
slhc_compress(struct slcompress *comp, unsigned char *icp, int isize,
	unsigned char *ocp, unsigned char **cpp, int compress_cid)
{
  printk(KERN_DEBUG "Called IP function on non IP-system: slhc_compress");
  return -EINVAL;
}

int
slhc_remember(struct slcompress *comp, unsigned char *icp, int isize)
{
  printk(KERN_DEBUG "Called IP function on non IP-system: slhc_remember");
  return -EINVAL;
}

void
slhc_free(struct slcompress *comp)
{
  printk(KERN_DEBUG "Called IP function on non IP-system: slhc_free");
}
struct slcompress *
slhc_init(int rslots, int tslots)
{
  printk(KERN_DEBUG "Called IP function on non IP-system: slhc_init");
  return NULL;
}

#endif 

EXPORT_SYMBOL(slhc_init);
EXPORT_SYMBOL(slhc_free);
EXPORT_SYMBOL(slhc_remember);
EXPORT_SYMBOL(slhc_compress);
EXPORT_SYMBOL(slhc_uncompress);
EXPORT_SYMBOL(slhc_toss);

MODULE_LICENSE("Dual BSD/GPL");
