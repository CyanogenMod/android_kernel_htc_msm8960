/*
 * Copyright (C)2003,2004 USAGI/WIDE Project
 *
 * Header for use in defining a given L3 protocol for connection tracking.
 *
 * Author:
 *	Yasuyuki Kozakai @USAGI	<yasuyuki.kozakai@toshiba.co.jp>
 *
 * Derived from include/netfilter_ipv4/ip_conntrack_protocol.h
 */

#ifndef _NF_CONNTRACK_L3PROTO_H
#define _NF_CONNTRACK_L3PROTO_H
#include <linux/netlink.h>
#include <net/netlink.h>
#include <linux/seq_file.h>
#include <net/netfilter/nf_conntrack.h>

struct nf_conntrack_l3proto {
	
	u_int16_t l3proto;

	
	const char *name;

	bool (*pkt_to_tuple)(const struct sk_buff *skb, unsigned int nhoff,
			     struct nf_conntrack_tuple *tuple);

	bool (*invert_tuple)(struct nf_conntrack_tuple *inverse,
			     const struct nf_conntrack_tuple *orig);

	
	int (*print_tuple)(struct seq_file *s,
			   const struct nf_conntrack_tuple *);

	int (*get_l4proto)(const struct sk_buff *skb, unsigned int nhoff,
			   unsigned int *dataoff, u_int8_t *protonum);

	int (*tuple_to_nlattr)(struct sk_buff *skb,
			       const struct nf_conntrack_tuple *t);

	int (*nlattr_tuple_size)(void);

	int (*nlattr_to_tuple)(struct nlattr *tb[],
			       struct nf_conntrack_tuple *t);
	const struct nla_policy *nla_policy;

	size_t nla_size;

#ifdef CONFIG_SYSCTL
	struct ctl_table_header	*ctl_table_header;
	struct ctl_path		*ctl_table_path;
	struct ctl_table	*ctl_table;
#endif 

	
	struct module *me;
};

extern struct nf_conntrack_l3proto __rcu *nf_ct_l3protos[AF_MAX];

extern int nf_conntrack_l3proto_register(struct nf_conntrack_l3proto *proto);
extern void nf_conntrack_l3proto_unregister(struct nf_conntrack_l3proto *proto);
extern struct nf_conntrack_l3proto *nf_ct_l3proto_find_get(u_int16_t l3proto);
extern void nf_ct_l3proto_put(struct nf_conntrack_l3proto *p);

extern struct nf_conntrack_l3proto nf_conntrack_l3proto_generic;

static inline struct nf_conntrack_l3proto *
__nf_ct_l3proto_find(u_int16_t l3proto)
{
	if (unlikely(l3proto >= AF_MAX))
		return &nf_conntrack_l3proto_generic;
	return rcu_dereference(nf_ct_l3protos[l3proto]);
}

#endif 
