/*
 * net/core/gen_stats.c
 *
 *             This program is free software; you can redistribute it and/or
 *             modify it under the terms of the GNU General Public License
 *             as published by the Free Software Foundation; either version
 *             2 of the License, or (at your option) any later version.
 *
 * Authors:  Thomas Graf <tgraf@suug.ch>
 *           Jamal Hadi Salim
 *           Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 *
 * See Documentation/networking/gen_stats.txt
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/socket.h>
#include <linux/rtnetlink.h>
#include <linux/gen_stats.h>
#include <net/netlink.h>
#include <net/gen_stats.h>


static inline int
gnet_stats_copy(struct gnet_dump *d, int type, void *buf, int size)
{
	NLA_PUT(d->skb, type, size, buf);
	return 0;

nla_put_failure:
	spin_unlock_bh(d->lock);
	return -1;
}

int
gnet_stats_start_copy_compat(struct sk_buff *skb, int type, int tc_stats_type,
	int xstats_type, spinlock_t *lock, struct gnet_dump *d)
	__acquires(lock)
{
	memset(d, 0, sizeof(*d));

	spin_lock_bh(lock);
	d->lock = lock;
	if (type)
		d->tail = (struct nlattr *)skb_tail_pointer(skb);
	d->skb = skb;
	d->compat_tc_stats = tc_stats_type;
	d->compat_xstats = xstats_type;

	if (d->tail)
		return gnet_stats_copy(d, type, NULL, 0);

	return 0;
}
EXPORT_SYMBOL(gnet_stats_start_copy_compat);

int
gnet_stats_start_copy(struct sk_buff *skb, int type, spinlock_t *lock,
	struct gnet_dump *d)
{
	return gnet_stats_start_copy_compat(skb, type, 0, 0, lock, d);
}
EXPORT_SYMBOL(gnet_stats_start_copy);

int
gnet_stats_copy_basic(struct gnet_dump *d, struct gnet_stats_basic_packed *b)
{
	if (d->compat_tc_stats) {
		d->tc_stats.bytes = b->bytes;
		d->tc_stats.packets = b->packets;
	}

	if (d->tail) {
		struct gnet_stats_basic sb;

		memset(&sb, 0, sizeof(sb));
		sb.bytes = b->bytes;
		sb.packets = b->packets;
		return gnet_stats_copy(d, TCA_STATS_BASIC, &sb, sizeof(sb));
	}
	return 0;
}
EXPORT_SYMBOL(gnet_stats_copy_basic);

int
gnet_stats_copy_rate_est(struct gnet_dump *d,
			 const struct gnet_stats_basic_packed *b,
			 struct gnet_stats_rate_est *r)
{
	if (b && !gen_estimator_active(b, r))
		return 0;

	if (d->compat_tc_stats) {
		d->tc_stats.bps = r->bps;
		d->tc_stats.pps = r->pps;
	}

	if (d->tail)
		return gnet_stats_copy(d, TCA_STATS_RATE_EST, r, sizeof(*r));

	return 0;
}
EXPORT_SYMBOL(gnet_stats_copy_rate_est);

int
gnet_stats_copy_queue(struct gnet_dump *d, struct gnet_stats_queue *q)
{
	if (d->compat_tc_stats) {
		d->tc_stats.drops = q->drops;
		d->tc_stats.qlen = q->qlen;
		d->tc_stats.backlog = q->backlog;
		d->tc_stats.overlimits = q->overlimits;
	}

	if (d->tail)
		return gnet_stats_copy(d, TCA_STATS_QUEUE, q, sizeof(*q));

	return 0;
}
EXPORT_SYMBOL(gnet_stats_copy_queue);

int
gnet_stats_copy_app(struct gnet_dump *d, void *st, int len)
{
	if (d->compat_xstats) {
		d->xstats = st;
		d->xstats_len = len;
	}

	if (d->tail)
		return gnet_stats_copy(d, TCA_STATS_APP, st, len);

	return 0;
}
EXPORT_SYMBOL(gnet_stats_copy_app);

int
gnet_stats_finish_copy(struct gnet_dump *d)
{
	if (d->tail)
		d->tail->nla_len = skb_tail_pointer(d->skb) - (u8 *)d->tail;

	if (d->compat_tc_stats)
		if (gnet_stats_copy(d, d->compat_tc_stats, &d->tc_stats,
			sizeof(d->tc_stats)) < 0)
			return -1;

	if (d->compat_xstats && d->xstats) {
		if (gnet_stats_copy(d, d->compat_xstats, d->xstats,
			d->xstats_len) < 0)
			return -1;
	}

	spin_unlock_bh(d->lock);
	return 0;
}
EXPORT_SYMBOL(gnet_stats_finish_copy);
