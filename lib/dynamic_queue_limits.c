/*
 * Dynamic byte queue limits.  See include/linux/dynamic_queue_limits.h
 *
 * Copyright (c) 2011, Tom Herbert <therbert@google.com>
 */
#include <linux/module.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/dynamic_queue_limits.h>

#define POSDIFF(A, B) ((int)((A) - (B)) > 0 ? (A) - (B) : 0)
#define AFTER_EQ(A, B) ((int)((A) - (B)) >= 0)

void dql_completed(struct dql *dql, unsigned int count)
{
	unsigned int inprogress, prev_inprogress, limit;
	unsigned int ovlimit, completed, num_queued;
	bool all_prev_completed;

	num_queued = ACCESS_ONCE(dql->num_queued);

	
	BUG_ON(count > num_queued - dql->num_completed);

	completed = dql->num_completed + count;
	limit = dql->limit;
	ovlimit = POSDIFF(num_queued - dql->num_completed, limit);
	inprogress = num_queued - completed;
	prev_inprogress = dql->prev_num_queued - dql->num_completed;
	all_prev_completed = AFTER_EQ(completed, dql->prev_num_queued);

	if ((ovlimit && !inprogress) ||
	    (dql->prev_ovlimit && all_prev_completed)) {
		limit += POSDIFF(completed, dql->prev_num_queued) +
		     dql->prev_ovlimit;
		dql->slack_start_time = jiffies;
		dql->lowest_slack = UINT_MAX;
	} else if (inprogress && prev_inprogress && !all_prev_completed) {
		unsigned int slack, slack_last_objs;

		slack = POSDIFF(limit + dql->prev_ovlimit,
		    2 * (completed - dql->num_completed));
		slack_last_objs = dql->prev_ovlimit ?
		    POSDIFF(dql->prev_last_obj_cnt, dql->prev_ovlimit) : 0;

		slack = max(slack, slack_last_objs);

		if (slack < dql->lowest_slack)
			dql->lowest_slack = slack;

		if (time_after(jiffies,
			       dql->slack_start_time + dql->slack_hold_time)) {
			limit = POSDIFF(limit, dql->lowest_slack);
			dql->slack_start_time = jiffies;
			dql->lowest_slack = UINT_MAX;
		}
	}

	
	limit = clamp(limit, dql->min_limit, dql->max_limit);

	if (limit != dql->limit) {
		dql->limit = limit;
		ovlimit = 0;
	}

	dql->adj_limit = limit + completed;
	dql->prev_ovlimit = ovlimit;
	dql->prev_last_obj_cnt = dql->last_obj_cnt;
	dql->num_completed = completed;
	dql->prev_num_queued = num_queued;
}
EXPORT_SYMBOL(dql_completed);

void dql_reset(struct dql *dql)
{
	
	dql->limit = 0;
	dql->num_queued = 0;
	dql->num_completed = 0;
	dql->last_obj_cnt = 0;
	dql->prev_num_queued = 0;
	dql->prev_last_obj_cnt = 0;
	dql->prev_ovlimit = 0;
	dql->lowest_slack = UINT_MAX;
	dql->slack_start_time = jiffies;
}
EXPORT_SYMBOL(dql_reset);

int dql_init(struct dql *dql, unsigned hold_time)
{
	dql->max_limit = DQL_MAX_LIMIT;
	dql->min_limit = 0;
	dql->slack_hold_time = hold_time;
	dql_reset(dql);
	return 0;
}
EXPORT_SYMBOL(dql_init);
