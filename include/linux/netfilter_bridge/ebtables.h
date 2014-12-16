/*
 *  ebtables
 *
 *	Authors:
 *	Bart De Schuymer		<bdschuym@pandora.be>
 *
 *  ebtables.c,v 2.0, April, 2002
 *
 *  This code is stongly inspired on the iptables code which is
 *  Copyright (C) 1999 Paul `Rusty' Russell & Michael J. Neuling
 */

#ifndef __LINUX_BRIDGE_EFF_H
#define __LINUX_BRIDGE_EFF_H
#include <linux/if.h>
#include <linux/netfilter_bridge.h>
#include <linux/if_ether.h>

#define EBT_TABLE_MAXNAMELEN 32
#define EBT_CHAIN_MAXNAMELEN EBT_TABLE_MAXNAMELEN
#define EBT_FUNCTION_MAXNAMELEN EBT_TABLE_MAXNAMELEN

#define EBT_ACCEPT   -1
#define EBT_DROP     -2
#define EBT_CONTINUE -3
#define EBT_RETURN   -4
#define NUM_STANDARD_TARGETS   4
#define EBT_VERDICT_BITS 0x0000000F

struct xt_match;
struct xt_target;

struct ebt_counter {
	uint64_t pcnt;
	uint64_t bcnt;
};

struct ebt_replace {
	char name[EBT_TABLE_MAXNAMELEN];
	unsigned int valid_hooks;
	
	unsigned int nentries;
	
	unsigned int entries_size;
	
	struct ebt_entries __user *hook_entry[NF_BR_NUMHOOKS];
	
	unsigned int num_counters;
	
	struct ebt_counter __user *counters;
	char __user *entries;
};

struct ebt_replace_kernel {
	char name[EBT_TABLE_MAXNAMELEN];
	unsigned int valid_hooks;
	
	unsigned int nentries;
	
	unsigned int entries_size;
	
	struct ebt_entries *hook_entry[NF_BR_NUMHOOKS];
	
	unsigned int num_counters;
	
	struct ebt_counter *counters;
	char *entries;
};

struct ebt_entries {
	unsigned int distinguisher;
	
	char name[EBT_CHAIN_MAXNAMELEN];
	
	unsigned int counter_offset;
	
	int policy;
	
	unsigned int nentries;
	
	char data[0] __attribute__ ((aligned (__alignof__(struct ebt_replace))));
};


#define EBT_ENTRY_OR_ENTRIES 0x01
#define EBT_NOPROTO 0x02
#define EBT_802_3 0x04
#define EBT_SOURCEMAC 0x08
#define EBT_DESTMAC 0x10
#define EBT_F_MASK (EBT_NOPROTO | EBT_802_3 | EBT_SOURCEMAC | EBT_DESTMAC \
   | EBT_ENTRY_OR_ENTRIES)

#define EBT_IPROTO 0x01
#define EBT_IIN 0x02
#define EBT_IOUT 0x04
#define EBT_ISOURCE 0x8
#define EBT_IDEST 0x10
#define EBT_ILOGICALIN 0x20
#define EBT_ILOGICALOUT 0x40
#define EBT_INV_MASK (EBT_IPROTO | EBT_IIN | EBT_IOUT | EBT_ILOGICALIN \
   | EBT_ILOGICALOUT | EBT_ISOURCE | EBT_IDEST)

struct ebt_entry_match {
	union {
		char name[EBT_FUNCTION_MAXNAMELEN];
		struct xt_match *match;
	} u;
	
	unsigned int match_size;
	unsigned char data[0] __attribute__ ((aligned (__alignof__(struct ebt_replace))));
};

struct ebt_entry_watcher {
	union {
		char name[EBT_FUNCTION_MAXNAMELEN];
		struct xt_target *watcher;
	} u;
	
	unsigned int watcher_size;
	unsigned char data[0] __attribute__ ((aligned (__alignof__(struct ebt_replace))));
};

struct ebt_entry_target {
	union {
		char name[EBT_FUNCTION_MAXNAMELEN];
		struct xt_target *target;
	} u;
	
	unsigned int target_size;
	unsigned char data[0] __attribute__ ((aligned (__alignof__(struct ebt_replace))));
};

#define EBT_STANDARD_TARGET "standard"
struct ebt_standard_target {
	struct ebt_entry_target target;
	int verdict;
};

struct ebt_entry {
	
	unsigned int bitmask;
	unsigned int invflags;
	__be16 ethproto;
	
	char in[IFNAMSIZ];
	
	char logical_in[IFNAMSIZ];
	
	char out[IFNAMSIZ];
	
	char logical_out[IFNAMSIZ];
	unsigned char sourcemac[ETH_ALEN];
	unsigned char sourcemsk[ETH_ALEN];
	unsigned char destmac[ETH_ALEN];
	unsigned char destmsk[ETH_ALEN];
	
	unsigned int watchers_offset;
	
	unsigned int target_offset;
	
	unsigned int next_offset;
	unsigned char elems[0] __attribute__ ((aligned (__alignof__(struct ebt_replace))));
};

#define EBT_BASE_CTL            128

#define EBT_SO_SET_ENTRIES      (EBT_BASE_CTL)
#define EBT_SO_SET_COUNTERS     (EBT_SO_SET_ENTRIES+1)
#define EBT_SO_SET_MAX          (EBT_SO_SET_COUNTERS+1)

#define EBT_SO_GET_INFO         (EBT_BASE_CTL)
#define EBT_SO_GET_ENTRIES      (EBT_SO_GET_INFO+1)
#define EBT_SO_GET_INIT_INFO    (EBT_SO_GET_ENTRIES+1)
#define EBT_SO_GET_INIT_ENTRIES (EBT_SO_GET_INIT_INFO+1)
#define EBT_SO_GET_MAX          (EBT_SO_GET_INIT_ENTRIES+1)

#ifdef __KERNEL__

#define EBT_MATCH 0
#define EBT_NOMATCH 1

struct ebt_match {
	struct list_head list;
	const char name[EBT_FUNCTION_MAXNAMELEN];
	bool (*match)(const struct sk_buff *skb, const struct net_device *in,
		const struct net_device *out, const struct xt_match *match,
		const void *matchinfo, int offset, unsigned int protoff,
		bool *hotdrop);
	bool (*checkentry)(const char *table, const void *entry,
		const struct xt_match *match, void *matchinfo,
		unsigned int hook_mask);
	void (*destroy)(const struct xt_match *match, void *matchinfo);
	unsigned int matchsize;
	u_int8_t revision;
	u_int8_t family;
	struct module *me;
};

struct ebt_watcher {
	struct list_head list;
	const char name[EBT_FUNCTION_MAXNAMELEN];
	unsigned int (*target)(struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
		unsigned int hook_num, const struct xt_target *target,
		const void *targinfo);
	bool (*checkentry)(const char *table, const void *entry,
		const struct xt_target *target, void *targinfo,
		unsigned int hook_mask);
	void (*destroy)(const struct xt_target *target, void *targinfo);
	unsigned int targetsize;
	u_int8_t revision;
	u_int8_t family;
	struct module *me;
};

struct ebt_target {
	struct list_head list;
	const char name[EBT_FUNCTION_MAXNAMELEN];
	
	unsigned int (*target)(struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
		unsigned int hook_num, const struct xt_target *target,
		const void *targinfo);
	bool (*checkentry)(const char *table, const void *entry,
		const struct xt_target *target, void *targinfo,
		unsigned int hook_mask);
	void (*destroy)(const struct xt_target *target, void *targinfo);
	unsigned int targetsize;
	u_int8_t revision;
	u_int8_t family;
	struct module *me;
};

struct ebt_chainstack {
	struct ebt_entries *chaininfo; 
	struct ebt_entry *e; 
	unsigned int n; 
};

struct ebt_table_info {
	
	unsigned int entries_size;
	unsigned int nentries;
	
	struct ebt_entries *hook_entry[NF_BR_NUMHOOKS];
	
	struct ebt_chainstack **chainstack;
	char *entries;
	struct ebt_counter counters[0] ____cacheline_aligned;
};

struct ebt_table {
	struct list_head list;
	char name[EBT_TABLE_MAXNAMELEN];
	struct ebt_replace_kernel *table;
	unsigned int valid_hooks;
	rwlock_t lock;
	int (*check)(const struct ebt_table_info *info,
	   unsigned int valid_hooks);
	
	struct ebt_table_info *private;
	struct module *me;
};

#define EBT_ALIGN(s) (((s) + (__alignof__(struct _xt_align)-1)) & \
		     ~(__alignof__(struct _xt_align)-1))
extern struct ebt_table *ebt_register_table(struct net *net,
					    const struct ebt_table *table);
extern void ebt_unregister_table(struct net *net, struct ebt_table *table);
extern unsigned int ebt_do_table(unsigned int hook, struct sk_buff *skb,
   const struct net_device *in, const struct net_device *out,
   struct ebt_table *table);

#define FWINV(bool,invflg) ((bool) ^ !!(info->invflags & invflg))
#define BASE_CHAIN (par->hook_mask & (1 << NF_BR_NUMHOOKS))
#define CLEAR_BASE_CHAIN_BIT (par->hook_mask &= ~(1 << NF_BR_NUMHOOKS))
#define INVALID_TARGET (info->target < -NUM_STANDARD_TARGETS || info->target >= 0)

#endif 

#define EBT_MATCH_ITERATE(e, fn, args...)                   \
({                                                          \
	unsigned int __i;                                   \
	int __ret = 0;                                      \
	struct ebt_entry_match *__match;                    \
	                                                    \
	for (__i = sizeof(struct ebt_entry);                \
	     __i < (e)->watchers_offset;                    \
	     __i += __match->match_size +                   \
	     sizeof(struct ebt_entry_match)) {              \
		__match = (void *)(e) + __i;                \
		                                            \
		__ret = fn(__match , ## args);              \
		if (__ret != 0)                             \
			break;                              \
	}                                                   \
	if (__ret == 0) {                                   \
		if (__i != (e)->watchers_offset)            \
			__ret = -EINVAL;                    \
	}                                                   \
	__ret;                                              \
})

#define EBT_WATCHER_ITERATE(e, fn, args...)                 \
({                                                          \
	unsigned int __i;                                   \
	int __ret = 0;                                      \
	struct ebt_entry_watcher *__watcher;                \
	                                                    \
	for (__i = e->watchers_offset;                      \
	     __i < (e)->target_offset;                      \
	     __i += __watcher->watcher_size +               \
	     sizeof(struct ebt_entry_watcher)) {            \
		__watcher = (void *)(e) + __i;              \
		                                            \
		__ret = fn(__watcher , ## args);            \
		if (__ret != 0)                             \
			break;                              \
	}                                                   \
	if (__ret == 0) {                                   \
		if (__i != (e)->target_offset)              \
			__ret = -EINVAL;                    \
	}                                                   \
	__ret;                                              \
})

#define EBT_ENTRY_ITERATE(entries, size, fn, args...)       \
({                                                          \
	unsigned int __i;                                   \
	int __ret = 0;                                      \
	struct ebt_entry *__entry;                          \
	                                                    \
	for (__i = 0; __i < (size);) {                      \
		__entry = (void *)(entries) + __i;          \
		__ret = fn(__entry , ## args);              \
		if (__ret != 0)                             \
			break;                              \
		if (__entry->bitmask != 0)                  \
			__i += __entry->next_offset;        \
		else                                        \
			__i += sizeof(struct ebt_entries);  \
	}                                                   \
	if (__ret == 0) {                                   \
		if (__i != (size))                          \
			__ret = -EINVAL;                    \
	}                                                   \
	__ret;                                              \
})

#endif
