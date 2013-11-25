

#ifndef _IPTABLES_H
#define _IPTABLES_H

#ifdef __KERNEL__
#include <linux/if.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/skbuff.h>
#endif
#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/netfilter_ipv4.h>

#include <linux/netfilter/x_tables.h>

#ifndef __KERNEL__
#define IPT_FUNCTION_MAXNAMELEN XT_FUNCTION_MAXNAMELEN
#define IPT_TABLE_MAXNAMELEN XT_TABLE_MAXNAMELEN
#define ipt_match xt_match
#define ipt_target xt_target
#define ipt_table xt_table
#define ipt_get_revision xt_get_revision
#define ipt_entry_match xt_entry_match
#define ipt_entry_target xt_entry_target
#define ipt_standard_target xt_standard_target
#define ipt_error_target xt_error_target
#define ipt_counters xt_counters
#define IPT_CONTINUE XT_CONTINUE
#define IPT_RETURN XT_RETURN

#include <linux/netfilter/xt_tcpudp.h>
#define ipt_udp xt_udp
#define ipt_tcp xt_tcp
#define IPT_TCP_INV_SRCPT	XT_TCP_INV_SRCPT
#define IPT_TCP_INV_DSTPT	XT_TCP_INV_DSTPT
#define IPT_TCP_INV_FLAGS	XT_TCP_INV_FLAGS
#define IPT_TCP_INV_OPTION	XT_TCP_INV_OPTION
#define IPT_TCP_INV_MASK	XT_TCP_INV_MASK
#define IPT_UDP_INV_SRCPT	XT_UDP_INV_SRCPT
#define IPT_UDP_INV_DSTPT	XT_UDP_INV_DSTPT
#define IPT_UDP_INV_MASK	XT_UDP_INV_MASK

#define ipt_counters_info xt_counters_info
#define IPT_STANDARD_TARGET XT_STANDARD_TARGET
#define IPT_ERROR_TARGET XT_ERROR_TARGET

#define IPT_MATCH_ITERATE(e, fn, args...) \
	XT_MATCH_ITERATE(struct ipt_entry, e, fn, ## args)

#define IPT_ENTRY_ITERATE(entries, size, fn, args...) \
	XT_ENTRY_ITERATE(struct ipt_entry, entries, size, fn, ## args)
#endif

struct ipt_ip {
	
	struct in_addr src, dst;
	
	struct in_addr smsk, dmsk;
	char iniface[IFNAMSIZ], outiface[IFNAMSIZ];
	unsigned char iniface_mask[IFNAMSIZ], outiface_mask[IFNAMSIZ];

	
	__u16 proto;

	
	__u8 flags;
	
	__u8 invflags;
};

#define IPT_F_FRAG		0x01	
#define IPT_F_GOTO		0x02	
#define IPT_F_MASK		0x03	

#define IPT_INV_VIA_IN		0x01	
#define IPT_INV_VIA_OUT		0x02	
#define IPT_INV_TOS		0x04	
#define IPT_INV_SRCIP		0x08	
#define IPT_INV_DSTIP		0x10	
#define IPT_INV_FRAG		0x20	
#define IPT_INV_PROTO		XT_INV_PROTO
#define IPT_INV_MASK		0x7F	

struct ipt_entry {
	struct ipt_ip ip;

	
	unsigned int nfcache;

	
	__u16 target_offset;
	
	__u16 next_offset;

	
	unsigned int comefrom;

	
	struct xt_counters counters;

	
	unsigned char elems[0];
};

#define IPT_BASE_CTL		64

#define IPT_SO_SET_REPLACE	(IPT_BASE_CTL)
#define IPT_SO_SET_ADD_COUNTERS	(IPT_BASE_CTL + 1)
#define IPT_SO_SET_MAX		IPT_SO_SET_ADD_COUNTERS

#define IPT_SO_GET_INFO			(IPT_BASE_CTL)
#define IPT_SO_GET_ENTRIES		(IPT_BASE_CTL + 1)
#define IPT_SO_GET_REVISION_MATCH	(IPT_BASE_CTL + 2)
#define IPT_SO_GET_REVISION_TARGET	(IPT_BASE_CTL + 3)
#define IPT_SO_GET_MAX			IPT_SO_GET_REVISION_TARGET

struct ipt_icmp {
	__u8 type;				
	__u8 code[2];				
	__u8 invflags;				
};

#define IPT_ICMP_INV	0x01	

struct ipt_getinfo {
	
	char name[XT_TABLE_MAXNAMELEN];

	
	
	unsigned int valid_hooks;

	
	unsigned int hook_entry[NF_INET_NUMHOOKS];

	
	unsigned int underflow[NF_INET_NUMHOOKS];

	
	unsigned int num_entries;

	
	unsigned int size;
};

struct ipt_replace {
	
	char name[XT_TABLE_MAXNAMELEN];

	unsigned int valid_hooks;

	
	unsigned int num_entries;

	
	unsigned int size;

	
	unsigned int hook_entry[NF_INET_NUMHOOKS];

	
	unsigned int underflow[NF_INET_NUMHOOKS];

	
	
	unsigned int num_counters;
	
	struct xt_counters __user *counters;

	
	struct ipt_entry entries[0];
};

struct ipt_get_entries {
	
	char name[XT_TABLE_MAXNAMELEN];

	
	unsigned int size;

	
	struct ipt_entry entrytable[0];
};

static __inline__ struct xt_entry_target *
ipt_get_target(struct ipt_entry *e)
{
	return (void *)e + e->target_offset;
}

#ifdef __KERNEL__

#include <linux/init.h>
extern void ipt_init(void) __init;

extern struct xt_table *ipt_register_table(struct net *net,
					   const struct xt_table *table,
					   const struct ipt_replace *repl);
extern void ipt_unregister_table(struct net *net, struct xt_table *table);

struct ipt_standard {
	struct ipt_entry entry;
	struct xt_standard_target target;
};

struct ipt_error {
	struct ipt_entry entry;
	struct xt_error_target target;
};

#define IPT_ENTRY_INIT(__size)						       \
{									       \
	.target_offset	= sizeof(struct ipt_entry),			       \
	.next_offset	= (__size),					       \
}

#define IPT_STANDARD_INIT(__verdict)					       \
{									       \
	.entry		= IPT_ENTRY_INIT(sizeof(struct ipt_standard)),	       \
	.target		= XT_TARGET_INIT(XT_STANDARD_TARGET,		       \
					 sizeof(struct xt_standard_target)),   \
	.target.verdict	= -(__verdict) - 1,				       \
}

#define IPT_ERROR_INIT							       \
{									       \
	.entry		= IPT_ENTRY_INIT(sizeof(struct ipt_error)),	       \
	.target		= XT_TARGET_INIT(XT_ERROR_TARGET,		       \
					 sizeof(struct xt_error_target)),      \
	.target.errorname = "ERROR",					       \
}

extern void *ipt_alloc_initial_table(const struct xt_table *);
extern unsigned int ipt_do_table(struct sk_buff *skb,
				 unsigned int hook,
				 const struct net_device *in,
				 const struct net_device *out,
				 struct xt_table *table);

#ifdef CONFIG_COMPAT
#include <net/compat.h>

struct compat_ipt_entry {
	struct ipt_ip ip;
	compat_uint_t nfcache;
	__u16 target_offset;
	__u16 next_offset;
	compat_uint_t comefrom;
	struct compat_xt_counters counters;
	unsigned char elems[0];
};

static inline struct xt_entry_target *
compat_ipt_get_target(struct compat_ipt_entry *e)
{
	return (void *)e + e->target_offset;
}

#endif 
#endif 
#endif 
