/*
 * DCCP connection tracking protocol helper
 *
 * Copyright (c) 2005, 2006, 2008 Patrick McHardy <kaber@trash.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sysctl.h>
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <linux/dccp.h>
#include <linux/slab.h>

#include <net/net_namespace.h>
#include <net/netns/generic.h>

#include <linux/netfilter/nfnetlink_conntrack.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_conntrack_ecache.h>
#include <net/netfilter/nf_log.h>


#define DCCP_MSL (2 * 60 * HZ)

static const char * const dccp_state_names[] = {
	[CT_DCCP_NONE]		= "NONE",
	[CT_DCCP_REQUEST]	= "REQUEST",
	[CT_DCCP_RESPOND]	= "RESPOND",
	[CT_DCCP_PARTOPEN]	= "PARTOPEN",
	[CT_DCCP_OPEN]		= "OPEN",
	[CT_DCCP_CLOSEREQ]	= "CLOSEREQ",
	[CT_DCCP_CLOSING]	= "CLOSING",
	[CT_DCCP_TIMEWAIT]	= "TIMEWAIT",
	[CT_DCCP_IGNORE]	= "IGNORE",
	[CT_DCCP_INVALID]	= "INVALID",
};

#define sNO	CT_DCCP_NONE
#define sRQ	CT_DCCP_REQUEST
#define sRS	CT_DCCP_RESPOND
#define sPO	CT_DCCP_PARTOPEN
#define sOP	CT_DCCP_OPEN
#define sCR	CT_DCCP_CLOSEREQ
#define sCG	CT_DCCP_CLOSING
#define sTW	CT_DCCP_TIMEWAIT
#define sIG	CT_DCCP_IGNORE
#define sIV	CT_DCCP_INVALID

static const u_int8_t
dccp_state_table[CT_DCCP_ROLE_MAX + 1][DCCP_PKT_SYNCACK + 1][CT_DCCP_MAX + 1] = {
	[CT_DCCP_ROLE_CLIENT] = {
		[DCCP_PKT_REQUEST] = {
			sRQ, sRQ, sRS, sIG, sIG, sIG, sIG, sRQ,
		},
		[DCCP_PKT_RESPONSE] = {
			sIV, sIG, sIG, sIG, sIG, sIG, sIG, sIV,
		},
		[DCCP_PKT_ACK] = {
			sIV, sIV, sPO, sPO, sOP, sCR, sCG, sIV
		},
		[DCCP_PKT_DATA] = {
			sIV, sIV, sIV, sIV, sOP, sCR, sCG, sIV,
		},
		[DCCP_PKT_DATAACK] = {
			sIV, sIV, sPO, sPO, sOP, sCR, sCG, sIV
		},
		[DCCP_PKT_CLOSEREQ] = {
			sIV, sIV, sIV, sIV, sIV, sIV, sIV, sIV
		},
		[DCCP_PKT_CLOSE] = {
			sIV, sIV, sIV, sCG, sCG, sCG, sIV, sIV
		},
		[DCCP_PKT_RESET] = {
			sIV, sTW, sTW, sTW, sTW, sTW, sTW, sIG
		},
		[DCCP_PKT_SYNC] = {
			sIG, sIG, sIG, sIG, sIG, sIG, sIG, sIG,
		},
		[DCCP_PKT_SYNCACK] = {
			sIG, sIG, sIG, sIG, sIG, sIG, sIG, sIG,
		},
	},
	[CT_DCCP_ROLE_SERVER] = {
		[DCCP_PKT_REQUEST] = {
			sIV, sIG, sIG, sIG, sIG, sIG, sIG, sRQ
		},
		[DCCP_PKT_RESPONSE] = {
			sIV, sRS, sRS, sIG, sIG, sIG, sIG, sIV
		},
		[DCCP_PKT_ACK] = {
			sIV, sIV, sIV, sOP, sOP, sIV, sCG, sIV
		},
		[DCCP_PKT_DATA] = {
			sIV, sIV, sIV, sOP, sOP, sIV, sCG, sIV
		},
		[DCCP_PKT_DATAACK] = {
			sIV, sIV, sIV, sOP, sOP, sIV, sCG, sIV
		},
		[DCCP_PKT_CLOSEREQ] = {
			sIV, sIV, sIV, sCR, sCR, sCR, sCR, sIV
		},
		[DCCP_PKT_CLOSE] = {
			sIV, sIV, sIV, sCG, sCG, sIV, sCG, sIV
		},
		[DCCP_PKT_RESET] = {
			sIV, sTW, sTW, sTW, sTW, sTW, sTW, sTW, sIG
		},
		[DCCP_PKT_SYNC] = {
			sIG, sIG, sIG, sIG, sIG, sIG, sIG, sIG,
		},
		[DCCP_PKT_SYNCACK] = {
			sIG, sIG, sIG, sIG, sIG, sIG, sIG, sIG,
		},
	},
};

static int dccp_net_id __read_mostly;
struct dccp_net {
	int dccp_loose;
	unsigned int dccp_timeout[CT_DCCP_MAX + 1];
#ifdef CONFIG_SYSCTL
	struct ctl_table_header *sysctl_header;
	struct ctl_table *sysctl_table;
#endif
};

static inline struct dccp_net *dccp_pernet(struct net *net)
{
	return net_generic(net, dccp_net_id);
}

static bool dccp_pkt_to_tuple(const struct sk_buff *skb, unsigned int dataoff,
			      struct nf_conntrack_tuple *tuple)
{
	struct dccp_hdr _hdr, *dh;

	dh = skb_header_pointer(skb, dataoff, sizeof(_hdr), &_hdr);
	if (dh == NULL)
		return false;

	tuple->src.u.dccp.port = dh->dccph_sport;
	tuple->dst.u.dccp.port = dh->dccph_dport;
	return true;
}

static bool dccp_invert_tuple(struct nf_conntrack_tuple *inv,
			      const struct nf_conntrack_tuple *tuple)
{
	inv->src.u.dccp.port = tuple->dst.u.dccp.port;
	inv->dst.u.dccp.port = tuple->src.u.dccp.port;
	return true;
}

static bool dccp_new(struct nf_conn *ct, const struct sk_buff *skb,
		     unsigned int dataoff, unsigned int *timeouts)
{
	struct net *net = nf_ct_net(ct);
	struct dccp_net *dn;
	struct dccp_hdr _dh, *dh;
	const char *msg;
	u_int8_t state;

	dh = skb_header_pointer(skb, dataoff, sizeof(_dh), &dh);
	BUG_ON(dh == NULL);

	state = dccp_state_table[CT_DCCP_ROLE_CLIENT][dh->dccph_type][CT_DCCP_NONE];
	switch (state) {
	default:
		dn = dccp_pernet(net);
		if (dn->dccp_loose == 0) {
			msg = "nf_ct_dccp: not picking up existing connection ";
			goto out_invalid;
		}
	case CT_DCCP_REQUEST:
		break;
	case CT_DCCP_INVALID:
		msg = "nf_ct_dccp: invalid state transition ";
		goto out_invalid;
	}

	ct->proto.dccp.role[IP_CT_DIR_ORIGINAL] = CT_DCCP_ROLE_CLIENT;
	ct->proto.dccp.role[IP_CT_DIR_REPLY] = CT_DCCP_ROLE_SERVER;
	ct->proto.dccp.state = CT_DCCP_NONE;
	ct->proto.dccp.last_pkt = DCCP_PKT_REQUEST;
	ct->proto.dccp.last_dir = IP_CT_DIR_ORIGINAL;
	ct->proto.dccp.handshake_seq = 0;
	return true;

out_invalid:
	if (LOG_INVALID(net, IPPROTO_DCCP))
		nf_log_packet(nf_ct_l3num(ct), 0, skb, NULL, NULL, NULL, msg);
	return false;
}

static u64 dccp_ack_seq(const struct dccp_hdr *dh)
{
	const struct dccp_hdr_ack_bits *dhack;

	dhack = (void *)dh + __dccp_basic_hdr_len(dh);
	return ((u64)ntohs(dhack->dccph_ack_nr_high) << 32) +
		     ntohl(dhack->dccph_ack_nr_low);
}

static unsigned int *dccp_get_timeouts(struct net *net)
{
	return dccp_pernet(net)->dccp_timeout;
}

static int dccp_packet(struct nf_conn *ct, const struct sk_buff *skb,
		       unsigned int dataoff, enum ip_conntrack_info ctinfo,
		       u_int8_t pf, unsigned int hooknum,
		       unsigned int *timeouts)
{
	struct net *net = nf_ct_net(ct);
	enum ip_conntrack_dir dir = CTINFO2DIR(ctinfo);
	struct dccp_hdr _dh, *dh;
	u_int8_t type, old_state, new_state;
	enum ct_dccp_roles role;

	dh = skb_header_pointer(skb, dataoff, sizeof(_dh), &dh);
	BUG_ON(dh == NULL);
	type = dh->dccph_type;

	if (type == DCCP_PKT_RESET &&
	    !test_bit(IPS_SEEN_REPLY_BIT, &ct->status)) {
		
		nf_ct_kill_acct(ct, ctinfo, skb);
		return NF_ACCEPT;
	}

	spin_lock_bh(&ct->lock);

	role = ct->proto.dccp.role[dir];
	old_state = ct->proto.dccp.state;
	new_state = dccp_state_table[role][type][old_state];

	switch (new_state) {
	case CT_DCCP_REQUEST:
		if (old_state == CT_DCCP_TIMEWAIT &&
		    role == CT_DCCP_ROLE_SERVER) {
			ct->proto.dccp.role[dir] = CT_DCCP_ROLE_CLIENT;
			ct->proto.dccp.role[!dir] = CT_DCCP_ROLE_SERVER;
		}
		break;
	case CT_DCCP_RESPOND:
		if (old_state == CT_DCCP_REQUEST)
			ct->proto.dccp.handshake_seq = dccp_hdr_seq(dh);
		break;
	case CT_DCCP_PARTOPEN:
		if (old_state == CT_DCCP_RESPOND &&
		    type == DCCP_PKT_ACK &&
		    dccp_ack_seq(dh) == ct->proto.dccp.handshake_seq)
			set_bit(IPS_ASSURED_BIT, &ct->status);
		break;
	case CT_DCCP_IGNORE:
		if (ct->proto.dccp.last_dir == !dir &&
		    ct->proto.dccp.last_pkt == DCCP_PKT_REQUEST &&
		    type == DCCP_PKT_RESPONSE) {
			ct->proto.dccp.role[!dir] = CT_DCCP_ROLE_CLIENT;
			ct->proto.dccp.role[dir] = CT_DCCP_ROLE_SERVER;
			ct->proto.dccp.handshake_seq = dccp_hdr_seq(dh);
			new_state = CT_DCCP_RESPOND;
			break;
		}
		ct->proto.dccp.last_dir = dir;
		ct->proto.dccp.last_pkt = type;

		spin_unlock_bh(&ct->lock);
		if (LOG_INVALID(net, IPPROTO_DCCP))
			nf_log_packet(pf, 0, skb, NULL, NULL, NULL,
				      "nf_ct_dccp: invalid packet ignored ");
		return NF_ACCEPT;
	case CT_DCCP_INVALID:
		spin_unlock_bh(&ct->lock);
		if (LOG_INVALID(net, IPPROTO_DCCP))
			nf_log_packet(pf, 0, skb, NULL, NULL, NULL,
				      "nf_ct_dccp: invalid state transition ");
		return -NF_ACCEPT;
	}

	ct->proto.dccp.last_dir = dir;
	ct->proto.dccp.last_pkt = type;
	ct->proto.dccp.state = new_state;
	spin_unlock_bh(&ct->lock);

	if (new_state != old_state)
		nf_conntrack_event_cache(IPCT_PROTOINFO, ct);

	nf_ct_refresh_acct(ct, ctinfo, skb, timeouts[new_state]);

	return NF_ACCEPT;
}

static int dccp_error(struct net *net, struct nf_conn *tmpl,
		      struct sk_buff *skb, unsigned int dataoff,
		      enum ip_conntrack_info *ctinfo,
		      u_int8_t pf, unsigned int hooknum)
{
	struct dccp_hdr _dh, *dh;
	unsigned int dccp_len = skb->len - dataoff;
	unsigned int cscov;
	const char *msg;

	dh = skb_header_pointer(skb, dataoff, sizeof(_dh), &dh);
	if (dh == NULL) {
		msg = "nf_ct_dccp: short packet ";
		goto out_invalid;
	}

	if (dh->dccph_doff * 4 < sizeof(struct dccp_hdr) ||
	    dh->dccph_doff * 4 > dccp_len) {
		msg = "nf_ct_dccp: truncated/malformed packet ";
		goto out_invalid;
	}

	cscov = dccp_len;
	if (dh->dccph_cscov) {
		cscov = (dh->dccph_cscov - 1) * 4;
		if (cscov > dccp_len) {
			msg = "nf_ct_dccp: bad checksum coverage ";
			goto out_invalid;
		}
	}

	if (net->ct.sysctl_checksum && hooknum == NF_INET_PRE_ROUTING &&
	    nf_checksum_partial(skb, hooknum, dataoff, cscov, IPPROTO_DCCP,
				pf)) {
		msg = "nf_ct_dccp: bad checksum ";
		goto out_invalid;
	}

	if (dh->dccph_type >= DCCP_PKT_INVALID) {
		msg = "nf_ct_dccp: reserved packet type ";
		goto out_invalid;
	}

	return NF_ACCEPT;

out_invalid:
	if (LOG_INVALID(net, IPPROTO_DCCP))
		nf_log_packet(pf, 0, skb, NULL, NULL, NULL, msg);
	return -NF_ACCEPT;
}

static int dccp_print_tuple(struct seq_file *s,
			    const struct nf_conntrack_tuple *tuple)
{
	return seq_printf(s, "sport=%hu dport=%hu ",
			  ntohs(tuple->src.u.dccp.port),
			  ntohs(tuple->dst.u.dccp.port));
}

static int dccp_print_conntrack(struct seq_file *s, struct nf_conn *ct)
{
	return seq_printf(s, "%s ", dccp_state_names[ct->proto.dccp.state]);
}

#if IS_ENABLED(CONFIG_NF_CT_NETLINK)
static int dccp_to_nlattr(struct sk_buff *skb, struct nlattr *nla,
			  struct nf_conn *ct)
{
	struct nlattr *nest_parms;

	spin_lock_bh(&ct->lock);
	nest_parms = nla_nest_start(skb, CTA_PROTOINFO_DCCP | NLA_F_NESTED);
	if (!nest_parms)
		goto nla_put_failure;
	NLA_PUT_U8(skb, CTA_PROTOINFO_DCCP_STATE, ct->proto.dccp.state);
	NLA_PUT_U8(skb, CTA_PROTOINFO_DCCP_ROLE,
		   ct->proto.dccp.role[IP_CT_DIR_ORIGINAL]);
	NLA_PUT_BE64(skb, CTA_PROTOINFO_DCCP_HANDSHAKE_SEQ,
		     cpu_to_be64(ct->proto.dccp.handshake_seq));
	nla_nest_end(skb, nest_parms);
	spin_unlock_bh(&ct->lock);
	return 0;

nla_put_failure:
	spin_unlock_bh(&ct->lock);
	return -1;
}

static const struct nla_policy dccp_nla_policy[CTA_PROTOINFO_DCCP_MAX + 1] = {
	[CTA_PROTOINFO_DCCP_STATE]	= { .type = NLA_U8 },
	[CTA_PROTOINFO_DCCP_ROLE]	= { .type = NLA_U8 },
	[CTA_PROTOINFO_DCCP_HANDSHAKE_SEQ] = { .type = NLA_U64 },
};

static int nlattr_to_dccp(struct nlattr *cda[], struct nf_conn *ct)
{
	struct nlattr *attr = cda[CTA_PROTOINFO_DCCP];
	struct nlattr *tb[CTA_PROTOINFO_DCCP_MAX + 1];
	int err;

	if (!attr)
		return 0;

	err = nla_parse_nested(tb, CTA_PROTOINFO_DCCP_MAX, attr,
			       dccp_nla_policy);
	if (err < 0)
		return err;

	if (!tb[CTA_PROTOINFO_DCCP_STATE] ||
	    !tb[CTA_PROTOINFO_DCCP_ROLE] ||
	    nla_get_u8(tb[CTA_PROTOINFO_DCCP_ROLE]) > CT_DCCP_ROLE_MAX ||
	    nla_get_u8(tb[CTA_PROTOINFO_DCCP_STATE]) >= CT_DCCP_IGNORE) {
		return -EINVAL;
	}

	spin_lock_bh(&ct->lock);
	ct->proto.dccp.state = nla_get_u8(tb[CTA_PROTOINFO_DCCP_STATE]);
	if (nla_get_u8(tb[CTA_PROTOINFO_DCCP_ROLE]) == CT_DCCP_ROLE_CLIENT) {
		ct->proto.dccp.role[IP_CT_DIR_ORIGINAL] = CT_DCCP_ROLE_CLIENT;
		ct->proto.dccp.role[IP_CT_DIR_REPLY] = CT_DCCP_ROLE_SERVER;
	} else {
		ct->proto.dccp.role[IP_CT_DIR_ORIGINAL] = CT_DCCP_ROLE_SERVER;
		ct->proto.dccp.role[IP_CT_DIR_REPLY] = CT_DCCP_ROLE_CLIENT;
	}
	if (tb[CTA_PROTOINFO_DCCP_HANDSHAKE_SEQ]) {
		ct->proto.dccp.handshake_seq =
		be64_to_cpu(nla_get_be64(tb[CTA_PROTOINFO_DCCP_HANDSHAKE_SEQ]));
	}
	spin_unlock_bh(&ct->lock);
	return 0;
}

static int dccp_nlattr_size(void)
{
	return nla_total_size(0)	
		+ nla_policy_len(dccp_nla_policy, CTA_PROTOINFO_DCCP_MAX + 1);
}

#endif

#if IS_ENABLED(CONFIG_NF_CT_NETLINK_TIMEOUT)

#include <linux/netfilter/nfnetlink.h>
#include <linux/netfilter/nfnetlink_cttimeout.h>

static int dccp_timeout_nlattr_to_obj(struct nlattr *tb[], void *data)
{
	struct dccp_net *dn = dccp_pernet(&init_net);
	unsigned int *timeouts = data;
	int i;

	
	for (i=0; i<CT_DCCP_MAX; i++)
		timeouts[i] = dn->dccp_timeout[i];

	
	for (i=CTA_TIMEOUT_DCCP_UNSPEC+1; i<CTA_TIMEOUT_DCCP_MAX+1; i++) {
		if (tb[i]) {
			timeouts[i] = ntohl(nla_get_be32(tb[i])) * HZ;
		}
	}
	return 0;
}

static int
dccp_timeout_obj_to_nlattr(struct sk_buff *skb, const void *data)
{
        const unsigned int *timeouts = data;
	int i;

	for (i=CTA_TIMEOUT_DCCP_UNSPEC+1; i<CTA_TIMEOUT_DCCP_MAX+1; i++)
		NLA_PUT_BE32(skb, i, htonl(timeouts[i] / HZ));

	return 0;

nla_put_failure:
	return -ENOSPC;
}

static const struct nla_policy
dccp_timeout_nla_policy[CTA_TIMEOUT_DCCP_MAX+1] = {
	[CTA_TIMEOUT_DCCP_REQUEST]	= { .type = NLA_U32 },
	[CTA_TIMEOUT_DCCP_RESPOND]	= { .type = NLA_U32 },
	[CTA_TIMEOUT_DCCP_PARTOPEN]	= { .type = NLA_U32 },
	[CTA_TIMEOUT_DCCP_OPEN]		= { .type = NLA_U32 },
	[CTA_TIMEOUT_DCCP_CLOSEREQ]	= { .type = NLA_U32 },
	[CTA_TIMEOUT_DCCP_CLOSING]	= { .type = NLA_U32 },
	[CTA_TIMEOUT_DCCP_TIMEWAIT]	= { .type = NLA_U32 },
};
#endif 

#ifdef CONFIG_SYSCTL
static struct ctl_table dccp_sysctl_table[] = {
	{
		.procname	= "nf_conntrack_dccp_timeout_request",
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_jiffies,
	},
	{
		.procname	= "nf_conntrack_dccp_timeout_respond",
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_jiffies,
	},
	{
		.procname	= "nf_conntrack_dccp_timeout_partopen",
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_jiffies,
	},
	{
		.procname	= "nf_conntrack_dccp_timeout_open",
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_jiffies,
	},
	{
		.procname	= "nf_conntrack_dccp_timeout_closereq",
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_jiffies,
	},
	{
		.procname	= "nf_conntrack_dccp_timeout_closing",
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_jiffies,
	},
	{
		.procname	= "nf_conntrack_dccp_timeout_timewait",
		.maxlen		= sizeof(unsigned int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_jiffies,
	},
	{
		.procname	= "nf_conntrack_dccp_loose",
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
	{ }
};
#endif 

static struct nf_conntrack_l4proto dccp_proto4 __read_mostly = {
	.l3proto		= AF_INET,
	.l4proto		= IPPROTO_DCCP,
	.name			= "dccp",
	.pkt_to_tuple		= dccp_pkt_to_tuple,
	.invert_tuple		= dccp_invert_tuple,
	.new			= dccp_new,
	.packet			= dccp_packet,
	.get_timeouts		= dccp_get_timeouts,
	.error			= dccp_error,
	.print_tuple		= dccp_print_tuple,
	.print_conntrack	= dccp_print_conntrack,
#if IS_ENABLED(CONFIG_NF_CT_NETLINK)
	.to_nlattr		= dccp_to_nlattr,
	.nlattr_size		= dccp_nlattr_size,
	.from_nlattr		= nlattr_to_dccp,
	.tuple_to_nlattr	= nf_ct_port_tuple_to_nlattr,
	.nlattr_tuple_size	= nf_ct_port_nlattr_tuple_size,
	.nlattr_to_tuple	= nf_ct_port_nlattr_to_tuple,
	.nla_policy		= nf_ct_port_nla_policy,
#endif
#if IS_ENABLED(CONFIG_NF_CT_NETLINK_TIMEOUT)
	.ctnl_timeout		= {
		.nlattr_to_obj	= dccp_timeout_nlattr_to_obj,
		.obj_to_nlattr	= dccp_timeout_obj_to_nlattr,
		.nlattr_max	= CTA_TIMEOUT_DCCP_MAX,
		.obj_size	= sizeof(unsigned int) * CT_DCCP_MAX,
		.nla_policy	= dccp_timeout_nla_policy,
	},
#endif 
};

static struct nf_conntrack_l4proto dccp_proto6 __read_mostly = {
	.l3proto		= AF_INET6,
	.l4proto		= IPPROTO_DCCP,
	.name			= "dccp",
	.pkt_to_tuple		= dccp_pkt_to_tuple,
	.invert_tuple		= dccp_invert_tuple,
	.new			= dccp_new,
	.packet			= dccp_packet,
	.get_timeouts		= dccp_get_timeouts,
	.error			= dccp_error,
	.print_tuple		= dccp_print_tuple,
	.print_conntrack	= dccp_print_conntrack,
#if IS_ENABLED(CONFIG_NF_CT_NETLINK)
	.to_nlattr		= dccp_to_nlattr,
	.nlattr_size		= dccp_nlattr_size,
	.from_nlattr		= nlattr_to_dccp,
	.tuple_to_nlattr	= nf_ct_port_tuple_to_nlattr,
	.nlattr_tuple_size	= nf_ct_port_nlattr_tuple_size,
	.nlattr_to_tuple	= nf_ct_port_nlattr_to_tuple,
	.nla_policy		= nf_ct_port_nla_policy,
#endif
#if IS_ENABLED(CONFIG_NF_CT_NETLINK_TIMEOUT)
	.ctnl_timeout		= {
		.nlattr_to_obj	= dccp_timeout_nlattr_to_obj,
		.obj_to_nlattr	= dccp_timeout_obj_to_nlattr,
		.nlattr_max	= CTA_TIMEOUT_DCCP_MAX,
		.obj_size	= sizeof(unsigned int) * CT_DCCP_MAX,
		.nla_policy	= dccp_timeout_nla_policy,
	},
#endif 
};

static __net_init int dccp_net_init(struct net *net)
{
	struct dccp_net *dn = dccp_pernet(net);

	
	dn->dccp_loose = 1;
	dn->dccp_timeout[CT_DCCP_REQUEST]	= 2 * DCCP_MSL;
	dn->dccp_timeout[CT_DCCP_RESPOND]	= 4 * DCCP_MSL;
	dn->dccp_timeout[CT_DCCP_PARTOPEN]	= 4 * DCCP_MSL;
	dn->dccp_timeout[CT_DCCP_OPEN]		= 12 * 3600 * HZ;
	dn->dccp_timeout[CT_DCCP_CLOSEREQ]	= 64 * HZ;
	dn->dccp_timeout[CT_DCCP_CLOSING]	= 64 * HZ;
	dn->dccp_timeout[CT_DCCP_TIMEWAIT]	= 2 * DCCP_MSL;

#ifdef CONFIG_SYSCTL
	dn->sysctl_table = kmemdup(dccp_sysctl_table,
			sizeof(dccp_sysctl_table), GFP_KERNEL);
	if (!dn->sysctl_table)
		return -ENOMEM;

	dn->sysctl_table[0].data = &dn->dccp_timeout[CT_DCCP_REQUEST];
	dn->sysctl_table[1].data = &dn->dccp_timeout[CT_DCCP_RESPOND];
	dn->sysctl_table[2].data = &dn->dccp_timeout[CT_DCCP_PARTOPEN];
	dn->sysctl_table[3].data = &dn->dccp_timeout[CT_DCCP_OPEN];
	dn->sysctl_table[4].data = &dn->dccp_timeout[CT_DCCP_CLOSEREQ];
	dn->sysctl_table[5].data = &dn->dccp_timeout[CT_DCCP_CLOSING];
	dn->sysctl_table[6].data = &dn->dccp_timeout[CT_DCCP_TIMEWAIT];
	dn->sysctl_table[7].data = &dn->dccp_loose;

	dn->sysctl_header = register_net_sysctl_table(net,
			nf_net_netfilter_sysctl_path, dn->sysctl_table);
	if (!dn->sysctl_header) {
		kfree(dn->sysctl_table);
		return -ENOMEM;
	}
#endif

	return 0;
}

static __net_exit void dccp_net_exit(struct net *net)
{
	struct dccp_net *dn = dccp_pernet(net);
#ifdef CONFIG_SYSCTL
	unregister_net_sysctl_table(dn->sysctl_header);
	kfree(dn->sysctl_table);
#endif
}

static struct pernet_operations dccp_net_ops = {
	.init = dccp_net_init,
	.exit = dccp_net_exit,
	.id   = &dccp_net_id,
	.size = sizeof(struct dccp_net),
};

static int __init nf_conntrack_proto_dccp_init(void)
{
	int err;

	err = register_pernet_subsys(&dccp_net_ops);
	if (err < 0)
		goto err1;

	err = nf_conntrack_l4proto_register(&dccp_proto4);
	if (err < 0)
		goto err2;

	err = nf_conntrack_l4proto_register(&dccp_proto6);
	if (err < 0)
		goto err3;
	return 0;

err3:
	nf_conntrack_l4proto_unregister(&dccp_proto4);
err2:
	unregister_pernet_subsys(&dccp_net_ops);
err1:
	return err;
}

static void __exit nf_conntrack_proto_dccp_fini(void)
{
	unregister_pernet_subsys(&dccp_net_ops);
	nf_conntrack_l4proto_unregister(&dccp_proto6);
	nf_conntrack_l4proto_unregister(&dccp_proto4);
}

module_init(nf_conntrack_proto_dccp_init);
module_exit(nf_conntrack_proto_dccp_fini);

MODULE_AUTHOR("Patrick McHardy <kaber@trash.net>");
MODULE_DESCRIPTION("DCCP connection tracking protocol helper");
MODULE_LICENSE("GPL");
