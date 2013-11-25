/*
 *  linux/include/linux/sunrpc/timer.h
 *
 *  Declarations for the RPC transport timer.
 *
 *  Copyright (C) 2002 Trond Myklebust <trond.myklebust@fys.uio.no>
 */

#ifndef _LINUX_SUNRPC_TIMER_H
#define _LINUX_SUNRPC_TIMER_H

#include <linux/atomic.h>

struct rpc_rtt {
	unsigned long timeo;	
	unsigned long srtt[5];	
	unsigned long sdrtt[5];	
	int ntimeouts[5];	
};


extern void rpc_init_rtt(struct rpc_rtt *rt, unsigned long timeo);
extern void rpc_update_rtt(struct rpc_rtt *rt, unsigned timer, long m);
extern unsigned long rpc_calc_rto(struct rpc_rtt *rt, unsigned timer);

static inline void rpc_set_timeo(struct rpc_rtt *rt, int timer, int ntimeo)
{
	int *t;
	if (!timer)
		return;
	t = &rt->ntimeouts[timer-1];
	if (ntimeo < *t) {
		if (*t > 0)
			(*t)--;
	} else {
		if (ntimeo > 8)
			ntimeo = 8;
		*t = ntimeo;
	}
}

static inline int rpc_ntimeo(struct rpc_rtt *rt, int timer)
{
	if (!timer)
		return 0;
	return rt->ntimeouts[timer-1];
}

#endif 
