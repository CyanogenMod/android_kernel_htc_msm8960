/*
 * Kernel iptables module to track stats for packets based on user tags.
 *
 * (C) 2011 Google, Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __XT_QTAGUID_INTERNAL_H__
#define __XT_QTAGUID_INTERNAL_H__

#include <linux/types.h>
#include <linux/rbtree.h>
#include <linux/spinlock_types.h>
#include <linux/workqueue.h>

#define IDEBUG_MASK (1<<0)
#define MDEBUG_MASK (1<<1)
#define RDEBUG_MASK (1<<2)
#define CDEBUG_MASK (1<<3)
#define DDEBUG_MASK (1<<4)

#define DEFAULT_DEBUG_MASK 0

#define IDEBUG
#define MDEBUG
#define RDEBUG
#define CDEBUG
#define DDEBUG

#define MSK_DEBUG(mask, ...) do {                           \
		if (unlikely(qtaguid_debug_mask & (mask)))  \
			pr_debug(__VA_ARGS__);              \
	} while (0)
#ifdef IDEBUG
#define IF_DEBUG(...) MSK_DEBUG(IDEBUG_MASK, __VA_ARGS__)
#else
#define IF_DEBUG(...) no_printk(__VA_ARGS__)
#endif
#ifdef MDEBUG
#define MT_DEBUG(...) MSK_DEBUG(MDEBUG_MASK, __VA_ARGS__)
#else
#define MT_DEBUG(...) no_printk(__VA_ARGS__)
#endif
#ifdef RDEBUG
#define RB_DEBUG(...) MSK_DEBUG(RDEBUG_MASK, __VA_ARGS__)
#else
#define RB_DEBUG(...) no_printk(__VA_ARGS__)
#endif
#ifdef CDEBUG
#define CT_DEBUG(...) MSK_DEBUG(CDEBUG_MASK, __VA_ARGS__)
#else
#define CT_DEBUG(...) no_printk(__VA_ARGS__)
#endif
#ifdef DDEBUG
#define DR_DEBUG(...) MSK_DEBUG(DDEBUG_MASK, __VA_ARGS__)
#else
#define DR_DEBUG(...) no_printk(__VA_ARGS__)
#endif

extern uint qtaguid_debug_mask;

typedef uint64_t tag_t;  

#define TAG_UID_MASK 0xFFFFFFFFULL
#define TAG_ACCT_MASK (~0xFFFFFFFFULL)

static inline int tag_compare(tag_t t1, tag_t t2)
{
	return t1 < t2 ? -1 : t1 == t2 ? 0 : 1;
}

static inline tag_t combine_atag_with_uid(tag_t acct_tag, uid_t uid)
{
	return acct_tag | uid;
}
static inline tag_t make_tag_from_uid(uid_t uid)
{
	return uid;
}
static inline uid_t get_uid_from_tag(tag_t tag)
{
	return tag & TAG_UID_MASK;
}
static inline tag_t get_utag_from_tag(tag_t tag)
{
	return tag & TAG_UID_MASK;
}
static inline tag_t get_atag_from_tag(tag_t tag)
{
	return tag & TAG_ACCT_MASK;
}

static inline bool valid_atag(tag_t tag)
{
	return !(tag & TAG_UID_MASK);
}
static inline tag_t make_atag_from_value(uint32_t value)
{
	return (uint64_t)value << 32;
}

#define DEFAULT_MAX_SOCK_TAGS 1024

#define IFS_MAX_COUNTER_SETS 2

enum ifs_tx_rx {
	IFS_TX,
	IFS_RX,
	IFS_MAX_DIRECTIONS
};

enum ifs_proto {
	IFS_TCP,
	IFS_UDP,
	IFS_PROTO_OTHER,
	IFS_MAX_PROTOS
};

struct byte_packet_counters {
	uint64_t bytes;
	uint64_t packets;
};

struct data_counters {
	struct byte_packet_counters bpc[IFS_MAX_COUNTER_SETS][IFS_MAX_DIRECTIONS][IFS_MAX_PROTOS];
};

struct tag_node {
	struct rb_node node;
	tag_t tag;
};

struct tag_stat {
	struct tag_node tn;
	struct data_counters counters;
	struct data_counters *parent_counters;
};

struct iface_stat {
	struct list_head list;  
	char *ifname;
	bool active;
	
	struct net_device *net_dev;

	struct byte_packet_counters totals_via_dev[IFS_MAX_DIRECTIONS];
	struct byte_packet_counters totals_via_skb[IFS_MAX_DIRECTIONS];
	struct byte_packet_counters last_known[IFS_MAX_DIRECTIONS];
	
	bool last_known_valid;

	struct proc_dir_entry *proc_ptr;

	struct rb_root tag_stat_tree;
	spinlock_t tag_stat_list_lock;
};

struct iface_stat_work {
	struct work_struct iface_work;
	struct iface_stat *iface_entry;
};

struct sock_tag {
	struct rb_node sock_node;
	struct sock *sk;  
	
	struct socket *socket;
	
	struct list_head list;   
	pid_t pid;

	tag_t tag;
};

struct qtaguid_event_counts {
	
	atomic64_t sockets_tagged;
	atomic64_t sockets_untagged;
	atomic64_t counter_set_changes;
	atomic64_t delete_cmds;
	atomic64_t iface_events;  

	atomic64_t match_calls;   
	
	atomic64_t match_calls_prepost;
	atomic64_t match_found_sk;   
	
	atomic64_t match_found_sk_in_ct;
	atomic64_t match_found_no_sk_in_ct;
	atomic64_t match_no_sk;
	atomic64_t match_no_sk_file;
};

struct tag_counter_set {
	struct tag_node tn;
	int active_set;
};

struct uid_tag_data {
	struct rb_node node;
	uid_t uid;

	int num_active_tags;
	
	int num_pqd;
	struct rb_root tag_ref_tree;
	
};

struct tag_ref {
	struct tag_node tn;

	int num_sock_tags;
};

struct proc_qtu_data {
	struct rb_node node;
	pid_t pid;

	struct uid_tag_data *parent_tag_data;

	
	struct list_head sock_tag_list;
	
};

#endif  
