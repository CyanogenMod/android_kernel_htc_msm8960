/* (C) 1999 Jérôme de Vivie <devivie@info.enserb.u-bordeaux.fr>
 * (C) 1999 Hervé Eychenne <eychenne@info.enserb.u-bordeaux.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>

#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_limit.h>

struct xt_limit_priv {
	unsigned long prev;
	uint32_t credit;
};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Herve Eychenne <rv@wallfire.org>");
MODULE_DESCRIPTION("Xtables: rate-limit match");
MODULE_ALIAS("ipt_limit");
MODULE_ALIAS("ip6t_limit");


static DEFINE_SPINLOCK(limit_lock);

#define MAX_CPJ (0xFFFFFFFF / (HZ*60*60*24))

#define _POW2_BELOW2(x) ((x)|((x)>>1))
#define _POW2_BELOW4(x) (_POW2_BELOW2(x)|_POW2_BELOW2((x)>>2))
#define _POW2_BELOW8(x) (_POW2_BELOW4(x)|_POW2_BELOW4((x)>>4))
#define _POW2_BELOW16(x) (_POW2_BELOW8(x)|_POW2_BELOW8((x)>>8))
#define _POW2_BELOW32(x) (_POW2_BELOW16(x)|_POW2_BELOW16((x)>>16))
#define POW2_BELOW32(x) ((_POW2_BELOW32(x)>>1) + 1)

#define CREDITS_PER_JIFFY POW2_BELOW32(MAX_CPJ)

static bool
limit_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_rateinfo *r = par->matchinfo;
	struct xt_limit_priv *priv = r->master;
	unsigned long now = jiffies;

	spin_lock_bh(&limit_lock);
	priv->credit += (now - xchg(&priv->prev, now)) * CREDITS_PER_JIFFY;
	if (priv->credit > r->credit_cap)
		priv->credit = r->credit_cap;

	if (priv->credit >= r->cost) {
		
		priv->credit -= r->cost;
		spin_unlock_bh(&limit_lock);
		return true;
	}

	spin_unlock_bh(&limit_lock);
	return false;
}

static u_int32_t
user2credits(u_int32_t user)
{
	
	if (user > 0xFFFFFFFF / (HZ*CREDITS_PER_JIFFY))
		
		return (user / XT_LIMIT_SCALE) * HZ * CREDITS_PER_JIFFY;

	return (user * HZ * CREDITS_PER_JIFFY) / XT_LIMIT_SCALE;
}

static int limit_mt_check(const struct xt_mtchk_param *par)
{
	struct xt_rateinfo *r = par->matchinfo;
	struct xt_limit_priv *priv;

	
	if (r->burst == 0
	    || user2credits(r->avg * r->burst) < user2credits(r->avg)) {
		pr_info("Overflow, try lower: %u/%u\n",
			r->avg, r->burst);
		return -ERANGE;
	}

	priv = kmalloc(sizeof(*priv), GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;

	
	r->master = priv;
	if (r->cost == 0) {
		priv->prev = jiffies;
		priv->credit = user2credits(r->avg * r->burst); 
		r->credit_cap = user2credits(r->avg * r->burst); 
		r->cost = user2credits(r->avg);
	}
	return 0;
}

static void limit_mt_destroy(const struct xt_mtdtor_param *par)
{
	const struct xt_rateinfo *info = par->matchinfo;

	kfree(info->master);
}

#ifdef CONFIG_COMPAT
struct compat_xt_rateinfo {
	u_int32_t avg;
	u_int32_t burst;

	compat_ulong_t prev;
	u_int32_t credit;
	u_int32_t credit_cap, cost;

	u_int32_t master;
};

static void limit_mt_compat_from_user(void *dst, const void *src)
{
	const struct compat_xt_rateinfo *cm = src;
	struct xt_rateinfo m = {
		.avg		= cm->avg,
		.burst		= cm->burst,
		.prev		= cm->prev | (unsigned long)cm->master << 32,
		.credit		= cm->credit,
		.credit_cap	= cm->credit_cap,
		.cost		= cm->cost,
	};
	memcpy(dst, &m, sizeof(m));
}

static int limit_mt_compat_to_user(void __user *dst, const void *src)
{
	const struct xt_rateinfo *m = src;
	struct compat_xt_rateinfo cm = {
		.avg		= m->avg,
		.burst		= m->burst,
		.prev		= m->prev,
		.credit		= m->credit,
		.credit_cap	= m->credit_cap,
		.cost		= m->cost,
		.master		= m->prev >> 32,
	};
	return copy_to_user(dst, &cm, sizeof(cm)) ? -EFAULT : 0;
}
#endif 

static struct xt_match limit_mt_reg __read_mostly = {
	.name             = "limit",
	.revision         = 0,
	.family           = NFPROTO_UNSPEC,
	.match            = limit_mt,
	.checkentry       = limit_mt_check,
	.destroy          = limit_mt_destroy,
	.matchsize        = sizeof(struct xt_rateinfo),
#ifdef CONFIG_COMPAT
	.compatsize       = sizeof(struct compat_xt_rateinfo),
	.compat_from_user = limit_mt_compat_from_user,
	.compat_to_user   = limit_mt_compat_to_user,
#endif
	.me               = THIS_MODULE,
};

static int __init limit_mt_init(void)
{
	return xt_register_match(&limit_mt_reg);
}

static void __exit limit_mt_exit(void)
{
	xt_unregister_match(&limit_mt_reg);
}

module_init(limit_mt_init);
module_exit(limit_mt_exit);
