/* SCTP kernel implementation
 * (C) Copyright IBM Corp. 2001, 2004
 * Copyright (c) 1999-2000 Cisco, Inc.
 * Copyright (c) 1999-2001 Motorola, Inc.
 * Copyright (c) 2001-2003 Intel Corp.
 *
 * This file is part of the SCTP kernel implementation
 *
 * The base lksctp header.
 *
 * This SCTP implementation is free software;
 * you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This SCTP implementation is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *                 ************************
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU CC; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Please send any bug reports or fixes you make to the
 * email address(es):
 *    lksctp developers <lksctp-developers@lists.sourceforge.net>
 *
 * Or submit a bug report through the following website:
 *    http://www.sf.net/projects/lksctp
 *
 * Written or modified by:
 *    La Monte H.P. Yarroll <piggy@acm.org>
 *    Xingang Guo           <xingang.guo@intel.com>
 *    Jon Grimm             <jgrimm@us.ibm.com>
 *    Daisy Chang           <daisyc@us.ibm.com>
 *    Sridhar Samudrala     <sri@us.ibm.com>
 *    Ardelle Fan           <ardelle.fan@intel.com>
 *    Ryan Layer            <rmlayer@us.ibm.com>
 *    Kevin Gao             <kevin.gao@intel.com> 
 *
 * Any bugs reported given to us we will try to fix... any fixes shared will
 * be incorporated into the next SCTP release.
 */

#ifndef __net_sctp_h__
#define __net_sctp_h__


#include <linux/types.h>
#include <linux/slab.h>
#include <linux/in.h>
#include <linux/tty.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>
#include <linux/idr.h>

#if IS_ENABLED(CONFIG_IPV6)
#include <net/ipv6.h>
#include <net/ip6_route.h>
#endif

#include <asm/uaccess.h>
#include <asm/page.h>
#include <net/sock.h>
#include <net/snmp.h>
#include <net/sctp/structs.h>
#include <net/sctp/constants.h>


#ifndef SCTP_DEBUG
#ifdef CONFIG_SCTP_DBG_MSG
#define SCTP_DEBUG	1
#else
#define SCTP_DEBUG      0
#endif 
#endif 

#ifdef CONFIG_IP_SCTP_MODULE
#define SCTP_PROTOSW_FLAG 0
#else 
#define SCTP_PROTOSW_FLAG INET_PROTOSW_PERMANENT
#endif


#ifndef SCTP_STATIC
#define SCTP_STATIC static
#endif


extern struct sock *sctp_get_ctl_sock(void);
extern int sctp_copy_local_addr_list(struct sctp_bind_addr *,
				     sctp_scope_t, gfp_t gfp,
				     int flags);
extern struct sctp_pf *sctp_get_pf_specific(sa_family_t family);
extern int sctp_register_pf(struct sctp_pf *, sa_family_t);
extern void sctp_addr_wq_mgmt(struct sctp_sockaddr_entry *, int);

int sctp_backlog_rcv(struct sock *sk, struct sk_buff *skb);
int sctp_inet_listen(struct socket *sock, int backlog);
void sctp_write_space(struct sock *sk);
void sctp_data_ready(struct sock *sk, int len);
unsigned int sctp_poll(struct file *file, struct socket *sock,
		poll_table *wait);
void sctp_sock_rfree(struct sk_buff *skb);
void sctp_copy_sock(struct sock *newsk, struct sock *sk,
		    struct sctp_association *asoc);
extern struct percpu_counter sctp_sockets_allocated;
extern int sctp_asconf_mgmt(struct sctp_sock *, struct sctp_sockaddr_entry *);

int sctp_primitive_ASSOCIATE(struct sctp_association *, void *arg);
int sctp_primitive_SHUTDOWN(struct sctp_association *, void *arg);
int sctp_primitive_ABORT(struct sctp_association *, void *arg);
int sctp_primitive_SEND(struct sctp_association *, void *arg);
int sctp_primitive_REQUESTHEARTBEAT(struct sctp_association *, void *arg);
int sctp_primitive_ASCONF(struct sctp_association *, void *arg);

int sctp_rcv(struct sk_buff *skb);
void sctp_v4_err(struct sk_buff *skb, u32 info);
void sctp_hash_established(struct sctp_association *);
void sctp_unhash_established(struct sctp_association *);
void sctp_hash_endpoint(struct sctp_endpoint *);
void sctp_unhash_endpoint(struct sctp_endpoint *);
struct sock *sctp_err_lookup(int family, struct sk_buff *,
			     struct sctphdr *, struct sctp_association **,
			     struct sctp_transport **);
void sctp_err_finish(struct sock *, struct sctp_association *);
void sctp_icmp_frag_needed(struct sock *, struct sctp_association *,
			   struct sctp_transport *t, __u32 pmtu);
void sctp_icmp_proto_unreachable(struct sock *sk,
				 struct sctp_association *asoc,
				 struct sctp_transport *t);
void sctp_backlog_migrate(struct sctp_association *assoc,
			  struct sock *oldsk, struct sock *newsk);

int sctp_snmp_proc_init(void);
void sctp_snmp_proc_exit(void);
int sctp_eps_proc_init(void);
void sctp_eps_proc_exit(void);
int sctp_assocs_proc_init(void);
void sctp_assocs_proc_exit(void);
int sctp_remaddr_proc_init(void);
void sctp_remaddr_proc_exit(void);



extern struct kmem_cache *sctp_chunk_cachep __read_mostly;
extern struct kmem_cache *sctp_bucket_cachep __read_mostly;



#ifdef TEST_FRAME
#include <test_frame.h>
#else

#define sctp_spin_lock_irqsave(lock, flags) spin_lock_irqsave(lock, flags)
#define sctp_spin_unlock_irqrestore(lock, flags)  \
       spin_unlock_irqrestore(lock, flags)
#define sctp_local_bh_disable() local_bh_disable()
#define sctp_local_bh_enable()  local_bh_enable()
#define sctp_spin_lock(lock)    spin_lock(lock)
#define sctp_spin_unlock(lock)  spin_unlock(lock)
#define sctp_write_lock(lock)   write_lock(lock)
#define sctp_write_unlock(lock) write_unlock(lock)
#define sctp_read_lock(lock)    read_lock(lock)
#define sctp_read_unlock(lock)  read_unlock(lock)

#define sctp_lock_sock(sk)       lock_sock(sk)
#define sctp_release_sock(sk)    release_sock(sk)
#define sctp_bh_lock_sock(sk)    bh_lock_sock(sk)
#define sctp_bh_unlock_sock(sk)  bh_unlock_sock(sk)

DECLARE_SNMP_STAT(struct sctp_mib, sctp_statistics);
#define SCTP_INC_STATS(field)      SNMP_INC_STATS(sctp_statistics, field)
#define SCTP_INC_STATS_BH(field)   SNMP_INC_STATS_BH(sctp_statistics, field)
#define SCTP_INC_STATS_USER(field) SNMP_INC_STATS_USER(sctp_statistics, field)
#define SCTP_DEC_STATS(field)      SNMP_DEC_STATS(sctp_statistics, field)

#endif 

enum {
	SCTP_MIB_NUM = 0,
	SCTP_MIB_CURRESTAB,			
	SCTP_MIB_ACTIVEESTABS,			
	SCTP_MIB_PASSIVEESTABS,			
	SCTP_MIB_ABORTEDS,			
	SCTP_MIB_SHUTDOWNS,			
	SCTP_MIB_OUTOFBLUES,			
	SCTP_MIB_CHECKSUMERRORS,		
	SCTP_MIB_OUTCTRLCHUNKS,			
	SCTP_MIB_OUTORDERCHUNKS,		
	SCTP_MIB_OUTUNORDERCHUNKS,		
	SCTP_MIB_INCTRLCHUNKS,			
	SCTP_MIB_INORDERCHUNKS,			
	SCTP_MIB_INUNORDERCHUNKS,		
	SCTP_MIB_FRAGUSRMSGS,			
	SCTP_MIB_REASMUSRMSGS,			
	SCTP_MIB_OUTSCTPPACKS,			
	SCTP_MIB_INSCTPPACKS,			
	SCTP_MIB_T1_INIT_EXPIREDS,
	SCTP_MIB_T1_COOKIE_EXPIREDS,
	SCTP_MIB_T2_SHUTDOWN_EXPIREDS,
	SCTP_MIB_T3_RTX_EXPIREDS,
	SCTP_MIB_T4_RTO_EXPIREDS,
	SCTP_MIB_T5_SHUTDOWN_GUARD_EXPIREDS,
	SCTP_MIB_DELAY_SACK_EXPIREDS,
	SCTP_MIB_AUTOCLOSE_EXPIREDS,
	SCTP_MIB_T1_RETRANSMITS,
	SCTP_MIB_T3_RETRANSMITS,
	SCTP_MIB_PMTUD_RETRANSMITS,
	SCTP_MIB_FAST_RETRANSMITS,
	SCTP_MIB_IN_PKT_SOFTIRQ,
	SCTP_MIB_IN_PKT_BACKLOG,
	SCTP_MIB_IN_PKT_DISCARDS,
	SCTP_MIB_IN_DATA_CHUNK_DISCARDS,
	__SCTP_MIB_MAX
};

#define SCTP_MIB_MAX    __SCTP_MIB_MAX
struct sctp_mib {
        unsigned long   mibs[SCTP_MIB_MAX];
};


#if SCTP_DEBUG
extern int sctp_debug_flag;
#define SCTP_DEBUG_PRINTK(fmt, args...)			\
do {							\
	if (sctp_debug_flag)				\
		printk(KERN_DEBUG pr_fmt(fmt), ##args);	\
} while (0)
#define SCTP_DEBUG_PRINTK_CONT(fmt, args...)		\
do {							\
	if (sctp_debug_flag)				\
		pr_cont(fmt, ##args);			\
} while (0)
#define SCTP_DEBUG_PRINTK_IPADDR(fmt_lead, fmt_trail,			\
				 args_lead, addr, args_trail...)	\
do {									\
	const union sctp_addr *_addr = (addr);				\
	if (sctp_debug_flag) {						\
		if (_addr->sa.sa_family == AF_INET6) {			\
			printk(KERN_DEBUG				\
			       pr_fmt(fmt_lead "%pI6" fmt_trail),	\
			       args_lead,				\
			       &_addr->v6.sin6_addr,			\
			       args_trail);				\
		} else {						\
			printk(KERN_DEBUG				\
			       pr_fmt(fmt_lead "%pI4" fmt_trail),	\
			       args_lead,				\
			       &_addr->v4.sin_addr.s_addr,		\
			       args_trail);				\
		}							\
	}								\
} while (0)
#define SCTP_ENABLE_DEBUG { sctp_debug_flag = 1; }
#define SCTP_DISABLE_DEBUG { sctp_debug_flag = 0; }

#define SCTP_ASSERT(expr, str, func) \
	if (!(expr)) { \
		SCTP_DEBUG_PRINTK("Assertion Failed: %s(%s) at %s:%s:%d\n", \
			str, (#expr), __FILE__, __func__, __LINE__); \
		func; \
	}

#else	

#define SCTP_DEBUG_PRINTK(whatever...)
#define SCTP_DEBUG_PRINTK_CONT(fmt, args...)
#define SCTP_DEBUG_PRINTK_IPADDR(whatever...)
#define SCTP_ENABLE_DEBUG
#define SCTP_DISABLE_DEBUG
#define SCTP_ASSERT(expr, str, func)

#endif 


#ifdef CONFIG_SCTP_DBG_OBJCNT

extern atomic_t sctp_dbg_objcnt_sock;
extern atomic_t sctp_dbg_objcnt_ep;
extern atomic_t sctp_dbg_objcnt_assoc;
extern atomic_t sctp_dbg_objcnt_transport;
extern atomic_t sctp_dbg_objcnt_chunk;
extern atomic_t sctp_dbg_objcnt_bind_addr;
extern atomic_t sctp_dbg_objcnt_bind_bucket;
extern atomic_t sctp_dbg_objcnt_addr;
extern atomic_t sctp_dbg_objcnt_ssnmap;
extern atomic_t sctp_dbg_objcnt_datamsg;
extern atomic_t sctp_dbg_objcnt_keys;

#define SCTP_DBG_OBJCNT_INC(name) \
atomic_inc(&sctp_dbg_objcnt_## name)
#define SCTP_DBG_OBJCNT_DEC(name) \
atomic_dec(&sctp_dbg_objcnt_## name)
#define SCTP_DBG_OBJCNT(name) \
atomic_t sctp_dbg_objcnt_## name = ATOMIC_INIT(0)

#define SCTP_DBG_OBJCNT_ENTRY(name) \
{.label= #name, .counter= &sctp_dbg_objcnt_## name}

void sctp_dbg_objcnt_init(void);
void sctp_dbg_objcnt_exit(void);

#else

#define SCTP_DBG_OBJCNT_INC(name)
#define SCTP_DBG_OBJCNT_DEC(name)

static inline void sctp_dbg_objcnt_init(void) { return; }
static inline void sctp_dbg_objcnt_exit(void) { return; }

#endif 

#if defined CONFIG_SYSCTL
void sctp_sysctl_register(void);
void sctp_sysctl_unregister(void);
#else
static inline void sctp_sysctl_register(void) { return; }
static inline void sctp_sysctl_unregister(void) { return; }
#endif

#define SCTP_SAT_LEN(x) (sizeof(struct sctp_paramhdr) + (x) * sizeof(__u16))

#if IS_ENABLED(CONFIG_IPV6)

void sctp_v6_pf_init(void);
void sctp_v6_pf_exit(void);
int sctp_v6_protosw_init(void);
void sctp_v6_protosw_exit(void);
int sctp_v6_add_protocol(void);
void sctp_v6_del_protocol(void);

#else 

static inline void sctp_v6_pf_init(void) { return; }
static inline void sctp_v6_pf_exit(void) { return; }
static inline int sctp_v6_protosw_init(void) { return 0; }
static inline void sctp_v6_protosw_exit(void) { return; }
static inline int sctp_v6_add_protocol(void) { return 0; }
static inline void sctp_v6_del_protocol(void) { return; }

#endif 


static inline sctp_assoc_t sctp_assoc2id(const struct sctp_association *asoc)
{
	return asoc ? asoc->assoc_id : 0;
}

struct sctp_association *sctp_id2assoc(struct sock *sk, sctp_assoc_t id);

int sctp_do_peeloff(struct sock *sk, sctp_assoc_t id, struct socket **sockp);

#define sctp_skb_for_each(pos, head, tmp) \
	skb_queue_walk_safe(head, pos, tmp)

static inline void sctp_skb_list_tail(struct sk_buff_head *list,
				      struct sk_buff_head *head)
{
	unsigned long flags;

	sctp_spin_lock_irqsave(&head->lock, flags);
	sctp_spin_lock(&list->lock);

	skb_queue_splice_tail_init(list, head);

	sctp_spin_unlock(&list->lock);
	sctp_spin_unlock_irqrestore(&head->lock, flags);
}


static inline struct list_head *sctp_list_dequeue(struct list_head *list)
{
	struct list_head *result = NULL;

	if (list->next != list) {
		result = list->next;
		list->next = result->next;
		list->next->prev = list;
		INIT_LIST_HEAD(result);
	}
	return result;
}

static inline void sctp_skb_set_owner_r(struct sk_buff *skb, struct sock *sk)
{
	struct sctp_ulpevent *event = sctp_skb2event(skb);

	skb_orphan(skb);
	skb->sk = sk;
	skb->destructor = sctp_sock_rfree;
	atomic_add(event->rmem_len, &sk->sk_rmem_alloc);
	sk->sk_forward_alloc -= event->rmem_len;
}

static inline int sctp_list_single_entry(struct list_head *head)
{
	return (head->next != head) && (head->next == head->prev);
}

static inline __s32 sctp_jitter(__u32 rto)
{
	static __u32 sctp_rand;
	__s32 ret;

	
	if (!rto)
		rto = 1;

	sctp_rand += jiffies;
	sctp_rand ^= (sctp_rand << 12);
	sctp_rand ^= (sctp_rand >> 20);

	ret = sctp_rand % rto - (rto >> 1);
	return ret;
}

static inline int sctp_frag_point(const struct sctp_association *asoc, int pmtu)
{
	struct sctp_sock *sp = sctp_sk(asoc->base.sk);
	int frag = pmtu;

	frag -= sp->pf->af->net_header_len;
	frag -= sizeof(struct sctphdr) + sizeof(struct sctp_data_chunk);

	if (asoc->user_frag)
		frag = min_t(int, frag, asoc->user_frag);

	frag = min_t(int, frag, SCTP_MAX_CHUNK_LEN);

	return frag;
}

static inline void sctp_assoc_pending_pmtu(struct sctp_association *asoc)
{

	sctp_assoc_sync_pmtu(asoc);
	asoc->pmtu_pending = 0;
}

#define sctp_walk_params(pos, chunk, member)\
_sctp_walk_params((pos), (chunk), ntohs((chunk)->chunk_hdr.length), member)

#define _sctp_walk_params(pos, chunk, end, member)\
for (pos.v = chunk->member;\
     pos.v <= (void *)chunk + end - ntohs(pos.p->length) &&\
     ntohs(pos.p->length) >= sizeof(sctp_paramhdr_t);\
     pos.v += WORD_ROUND(ntohs(pos.p->length)))

#define sctp_walk_errors(err, chunk_hdr)\
_sctp_walk_errors((err), (chunk_hdr), ntohs((chunk_hdr)->length))

#define _sctp_walk_errors(err, chunk_hdr, end)\
for (err = (sctp_errhdr_t *)((void *)chunk_hdr + \
	    sizeof(sctp_chunkhdr_t));\
     (void *)err <= (void *)chunk_hdr + end - ntohs(err->length) &&\
     ntohs(err->length) >= sizeof(sctp_errhdr_t); \
     err = (sctp_errhdr_t *)((void *)err + WORD_ROUND(ntohs(err->length))))

#define sctp_walk_fwdtsn(pos, chunk)\
_sctp_walk_fwdtsn((pos), (chunk), ntohs((chunk)->chunk_hdr->length) - sizeof(struct sctp_fwdtsn_chunk))

#define _sctp_walk_fwdtsn(pos, chunk, end)\
for (pos = chunk->subh.fwdtsn_hdr->skip;\
     (void *)pos <= (void *)chunk->subh.fwdtsn_hdr->skip + end - sizeof(struct sctp_fwdtsn_skip);\
     pos++)

#define WORD_ROUND(s) (((s)+3)&~3)

#define t_new(type, flags)	(type *)kzalloc(sizeof(type), flags)

#define tv_lt(s, t) \
   (s.tv_sec < t.tv_sec || (s.tv_sec == t.tv_sec && s.tv_usec < t.tv_usec))

#define TIMEVAL_ADD(tv1, tv2) \
({ \
        suseconds_t usecs = (tv2).tv_usec + (tv1).tv_usec; \
        time_t secs = (tv2).tv_sec + (tv1).tv_sec; \
\
        if (usecs >= 1000000) { \
                usecs -= 1000000; \
                secs++; \
        } \
        (tv2).tv_sec = secs; \
        (tv2).tv_usec = usecs; \
})


extern struct proto sctp_prot;
extern struct proto sctpv6_prot;
extern struct proc_dir_entry *proc_net_sctp;
void sctp_put_port(struct sock *sk);

extern struct idr sctp_assocs_id;
extern spinlock_t sctp_assocs_id_lock;


static inline int ipver2af(__u8 ipver)
{
	switch (ipver) {
	case 4:
	        return  AF_INET;
	case 6:
		return AF_INET6;
	default:
		return 0;
	}
}

static inline int param_type2af(__be16 type)
{
	switch (type) {
	case SCTP_PARAM_IPV4_ADDRESS:
	        return  AF_INET;
	case SCTP_PARAM_IPV6_ADDRESS:
		return AF_INET6;
	default:
		return 0;
	}
}

static inline int sctp_sanity_check(void)
{
	SCTP_ASSERT(sizeof(struct sctp_ulpevent) <=
		    sizeof(((struct sk_buff *)0)->cb),
		    "SCTP: ulpevent does not fit in skb!\n", return 0);

	return 1;
}

static inline int sctp_phashfn(__u16 lport)
{
	return lport & (sctp_port_hashsize - 1);
}

static inline int sctp_ep_hashfn(__u16 lport)
{
	return lport & (sctp_ep_hashsize - 1);
}

static inline int sctp_assoc_hashfn(__u16 lport, __u16 rport)
{
	int h = (lport << 16) + rport;
	h ^= h>>8;
	return h & (sctp_assoc_hashsize - 1);
}

static inline int sctp_vtag_hashfn(__u16 lport, __u16 rport, __u32 vtag)
{
	int h = (lport << 16) + rport;
	h ^= vtag;
	return h & (sctp_assoc_hashsize - 1);
}

#define sctp_for_each_hentry(epb, node, head) \
	hlist_for_each_entry(epb, node, head, node)

#define sctp_style(sk, style) __sctp_style((sk), (SCTP_SOCKET_##style))
static inline int __sctp_style(const struct sock *sk, sctp_socket_type_t style)
{
	return sctp_sk(sk)->type == style;
}

#define sctp_state(asoc, state) __sctp_state((asoc), (SCTP_STATE_##state))
static inline int __sctp_state(const struct sctp_association *asoc,
			       sctp_state_t state)
{
	return asoc->state == state;
}

#define sctp_sstate(sk, state) __sctp_sstate((sk), (SCTP_SS_##state))
static inline int __sctp_sstate(const struct sock *sk, sctp_sock_state_t state)
{
	return sk->sk_state == state;
}

static inline void sctp_v6_map_v4(union sctp_addr *addr)
{
	addr->v4.sin_family = AF_INET;
	addr->v4.sin_port = addr->v6.sin6_port;
	addr->v4.sin_addr.s_addr = addr->v6.sin6_addr.s6_addr32[3];
}

static inline void sctp_v4_map_v6(union sctp_addr *addr)
{
	addr->v6.sin6_family = AF_INET6;
	addr->v6.sin6_port = addr->v4.sin_port;
	addr->v6.sin6_addr.s6_addr32[3] = addr->v4.sin_addr.s_addr;
	addr->v6.sin6_addr.s6_addr32[0] = 0;
	addr->v6.sin6_addr.s6_addr32[1] = 0;
	addr->v6.sin6_addr.s6_addr32[2] = htonl(0x0000ffff);
}

static inline struct dst_entry *sctp_transport_dst_check(struct sctp_transport *t)
{
	if (t->dst && !dst_check(t->dst, 0)) {
		dst_release(t->dst);
		t->dst = NULL;
	}

	return t->dst;
}

#endif 
