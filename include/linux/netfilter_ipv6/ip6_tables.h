

#ifndef _IP6_TABLES_H
#define _IP6_TABLES_H

#ifdef __KERNEL__
#include <linux/if.h>
#include <linux/in6.h>
#include <linux/ipv6.h>
#include <linux/skbuff.h>
#endif
#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/netfilter_ipv6.h>

#include <linux/netfilter/x_tables.h>

#ifndef __KERNEL__
#define IP6T_FUNCTION_MAXNAMELEN XT_FUNCTION_MAXNAMELEN
#define IP6T_TABLE_MAXNAMELEN XT_TABLE_MAXNAMELEN
#define ip6t_match xt_match
#define ip6t_target xt_target
#define ip6t_table xt_table
#define ip6t_get_revision xt_get_revision
#define ip6t_entry_match xt_entry_match
#define ip6t_entry_target xt_entry_target
#define ip6t_standard_target xt_standard_target
#define ip6t_error_target xt_error_target
#define ip6t_counters xt_counters
#define IP6T_CONTINUE XT_CONTINUE
#define IP6T_RETURN XT_RETURN

#include <linux/netfilter/xt_tcpudp.h>
#define ip6t_tcp xt_tcp
#define ip6t_udp xt_udp
#define IP6T_TCP_INV_SRCPT	XT_TCP_INV_SRCPT
#define IP6T_TCP_INV_DSTPT	XT_TCP_INV_DSTPT
#define IP6T_TCP_INV_FLAGS	XT_TCP_INV_FLAGS
#define IP6T_TCP_INV_OPTION	XT_TCP_INV_OPTION
#define IP6T_TCP_INV_MASK	XT_TCP_INV_MASK
#define IP6T_UDP_INV_SRCPT	XT_UDP_INV_SRCPT
#define IP6T_UDP_INV_DSTPT	XT_UDP_INV_DSTPT
#define IP6T_UDP_INV_MASK	XT_UDP_INV_MASK

#define ip6t_counters_info xt_counters_info
#define IP6T_STANDARD_TARGET XT_STANDARD_TARGET
#define IP6T_ERROR_TARGET XT_ERROR_TARGET
#define IP6T_MATCH_ITERATE(e, fn, args...) \
	XT_MATCH_ITERATE(struct ip6t_entry, e, fn, ## args)
#define IP6T_ENTRY_ITERATE(entries, size, fn, args...) \
	XT_ENTRY_ITERATE(struct ip6t_entry, entries, size, fn, ## args)
#endif

struct ip6t_ip6 {
	
	struct in6_addr src, dst;		
	
	struct in6_addr smsk, dmsk;
	char iniface[IFNAMSIZ], outiface[IFNAMSIZ];
	unsigned char iniface_mask[IFNAMSIZ], outiface_mask[IFNAMSIZ];

	__u16 proto;
	
	__u8 tos;

	
	__u8 flags;
	
	__u8 invflags;
};

#define IP6T_F_PROTO		0x01	
#define IP6T_F_TOS		0x02	
#define IP6T_F_GOTO		0x04	
#define IP6T_F_MASK		0x07	

#define IP6T_INV_VIA_IN		0x01	
#define IP6T_INV_VIA_OUT		0x02	
#define IP6T_INV_TOS		0x04	
#define IP6T_INV_SRCIP		0x08	
#define IP6T_INV_DSTIP		0x10	
#define IP6T_INV_FRAG		0x20	
#define IP6T_INV_PROTO		XT_INV_PROTO
#define IP6T_INV_MASK		0x7F	

struct ip6t_entry {
	struct ip6t_ip6 ipv6;

	
	unsigned int nfcache;

	
	__u16 target_offset;
	
	__u16 next_offset;

	
	unsigned int comefrom;

	
	struct xt_counters counters;

	
	unsigned char elems[0];
};

struct ip6t_standard {
	struct ip6t_entry entry;
	struct xt_standard_target target;
};

struct ip6t_error {
	struct ip6t_entry entry;
	struct xt_error_target target;
};

#define IP6T_ENTRY_INIT(__size)						       \
{									       \
	.target_offset	= sizeof(struct ip6t_entry),			       \
	.next_offset	= (__size),					       \
}

#define IP6T_STANDARD_INIT(__verdict)					       \
{									       \
	.entry		= IP6T_ENTRY_INIT(sizeof(struct ip6t_standard)),       \
	.target		= XT_TARGET_INIT(XT_STANDARD_TARGET,		       \
					 sizeof(struct xt_standard_target)),   \
	.target.verdict	= -(__verdict) - 1,				       \
}

#define IP6T_ERROR_INIT							       \
{									       \
	.entry		= IP6T_ENTRY_INIT(sizeof(struct ip6t_error)),	       \
	.target		= XT_TARGET_INIT(XT_ERROR_TARGET,		       \
					 sizeof(struct xt_error_target)),      \
	.target.errorname = "ERROR",					       \
}

#define IP6T_BASE_CTL			64

#define IP6T_SO_SET_REPLACE		(IP6T_BASE_CTL)
#define IP6T_SO_SET_ADD_COUNTERS	(IP6T_BASE_CTL + 1)
#define IP6T_SO_SET_MAX			IP6T_SO_SET_ADD_COUNTERS

#define IP6T_SO_GET_INFO		(IP6T_BASE_CTL)
#define IP6T_SO_GET_ENTRIES		(IP6T_BASE_CTL + 1)
#define IP6T_SO_GET_REVISION_MATCH	(IP6T_BASE_CTL + 4)
#define IP6T_SO_GET_REVISION_TARGET	(IP6T_BASE_CTL + 5)
#define IP6T_SO_GET_MAX			IP6T_SO_GET_REVISION_TARGET

struct ip6t_icmp {
	__u8 type;				
	__u8 code[2];				
	__u8 invflags;				
};

#define IP6T_ICMP_INV	0x01	

struct ip6t_getinfo {
	
	char name[XT_TABLE_MAXNAMELEN];

	
	
	unsigned int valid_hooks;

	
	unsigned int hook_entry[NF_INET_NUMHOOKS];

	
	unsigned int underflow[NF_INET_NUMHOOKS];

	
	unsigned int num_entries;

	
	unsigned int size;
};

struct ip6t_replace {
	
	char name[XT_TABLE_MAXNAMELEN];

	unsigned int valid_hooks;

	
	unsigned int num_entries;

	
	unsigned int size;

	
	unsigned int hook_entry[NF_INET_NUMHOOKS];

	
	unsigned int underflow[NF_INET_NUMHOOKS];

	
	
	unsigned int num_counters;
	
	struct xt_counters __user *counters;

	
	struct ip6t_entry entries[0];
};

struct ip6t_get_entries {
	
	char name[XT_TABLE_MAXNAMELEN];

	
	unsigned int size;

	
	struct ip6t_entry entrytable[0];
};

static __inline__ struct xt_entry_target *
ip6t_get_target(struct ip6t_entry *e)
{
	return (void *)e + e->target_offset;
}


#ifdef __KERNEL__

#include <linux/init.h>
extern void ip6t_init(void) __init;

extern void *ip6t_alloc_initial_table(const struct xt_table *);
extern struct xt_table *ip6t_register_table(struct net *net,
					    const struct xt_table *table,
					    const struct ip6t_replace *repl);
extern void ip6t_unregister_table(struct net *net, struct xt_table *table);
extern unsigned int ip6t_do_table(struct sk_buff *skb,
				  unsigned int hook,
				  const struct net_device *in,
				  const struct net_device *out,
				  struct xt_table *table);

static inline int
ip6t_ext_hdr(u8 nexthdr)
{	return (nexthdr == IPPROTO_HOPOPTS) ||
	       (nexthdr == IPPROTO_ROUTING) ||
	       (nexthdr == IPPROTO_FRAGMENT) ||
	       (nexthdr == IPPROTO_ESP) ||
	       (nexthdr == IPPROTO_AH) ||
	       (nexthdr == IPPROTO_NONE) ||
	       (nexthdr == IPPROTO_DSTOPTS);
}

extern int ipv6_find_hdr(const struct sk_buff *skb, unsigned int *offset,
			 int target, unsigned short *fragoff);

#ifdef CONFIG_COMPAT
#include <net/compat.h>

struct compat_ip6t_entry {
	struct ip6t_ip6 ipv6;
	compat_uint_t nfcache;
	__u16 target_offset;
	__u16 next_offset;
	compat_uint_t comefrom;
	struct compat_xt_counters counters;
	unsigned char elems[0];
};

static inline struct xt_entry_target *
compat_ip6t_get_target(struct compat_ip6t_entry *e)
{
	return (void *)e + e->target_offset;
}

#endif 
#endif 
#endif 
