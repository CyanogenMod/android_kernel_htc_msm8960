/*
 * Dynamic queue limits (dql) - Definitions
 *
 * Copyright (c) 2011, Tom Herbert <therbert@google.com>
 *
 * This header file contains the definitions for dynamic queue limits (dql).
 * dql would be used in conjunction with a producer/consumer type queue
 * (possibly a HW queue).  Such a queue would have these general properties:
 *
 *   1) Objects are queued up to some limit specified as number of objects.
 *   2) Periodically a completion process executes which retires consumed
 *      objects.
 *   3) Starvation occurs when limit has been reached, all queued data has
 *      actually been consumed, but completion processing has not yet run
 *      so queuing new data is blocked.
 *   4) Minimizing the amount of queued data is desirable.
 *
 * The goal of dql is to calculate the limit as the minimum number of objects
 * needed to prevent starvation.
 *
 * The primary functions of dql are:
 *    dql_queued - called when objects are enqueued to record number of objects
 *    dql_avail - returns how many objects are available to be queued based
 *      on the object limit and how many objects are already enqueued
 *    dql_completed - called at completion time to indicate how many objects
 *      were retired from the queue
 *
 * The dql implementation does not implement any locking for the dql data
 * structures, the higher layer should provide this.  dql_queued should
 * be serialized to prevent concurrent execution of the function; this
 * is also true for  dql_completed.  However, dql_queued and dlq_completed  can
 * be executed concurrently (i.e. they can be protected by different locks).
 */

#ifndef _LINUX_DQL_H
#define _LINUX_DQL_H

#ifdef __KERNEL__

struct dql {
	
	unsigned int	num_queued;		
	unsigned int	adj_limit;		
	unsigned int	last_obj_cnt;		

	

	unsigned int	limit ____cacheline_aligned_in_smp; 
	unsigned int	num_completed;		

	unsigned int	prev_ovlimit;		
	unsigned int	prev_num_queued;	
	unsigned int	prev_last_obj_cnt;	

	unsigned int	lowest_slack;		
	unsigned long	slack_start_time;	

	
	unsigned int	max_limit;		
	unsigned int	min_limit;		
	unsigned int	slack_hold_time;	
};

#define DQL_MAX_OBJECT (UINT_MAX / 16)
#define DQL_MAX_LIMIT ((UINT_MAX / 2) - DQL_MAX_OBJECT)

static inline void dql_queued(struct dql *dql, unsigned int count)
{
	BUG_ON(count > DQL_MAX_OBJECT);

	dql->num_queued += count;
	dql->last_obj_cnt = count;
}

static inline int dql_avail(const struct dql *dql)
{
	return dql->adj_limit - dql->num_queued;
}

void dql_completed(struct dql *dql, unsigned int count);

void dql_reset(struct dql *dql);

int dql_init(struct dql *dql, unsigned hold_time);

#endif 

#endif 
