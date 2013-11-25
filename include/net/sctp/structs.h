/* SCTP kernel implementation
 * (C) Copyright IBM Corp. 2001, 2004
 * Copyright (c) 1999-2000 Cisco, Inc.
 * Copyright (c) 1999-2001 Motorola, Inc.
 * Copyright (c) 2001 Intel Corp.
 *
 * This file is part of the SCTP kernel implementation
 *
 * This SCTP implementation is free software;
 * you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This SCTP implementation is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *		   ************************
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU CC; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Please send any bug reports or fixes you make to the
 * email addresses:
 *    lksctp developers <lksctp-developers@lists.sourceforge.net>
 *
 * Or submit a bug report through the following website:
 *    http://www.sf.net/projects/lksctp
 *
 * Written or modified by:
 *    Randall Stewart	    <randall@sctp.chicago.il.us>
 *    Ken Morneau	    <kmorneau@cisco.com>
 *    Qiaobing Xie	    <qxie1@email.mot.com>
 *    La Monte H.P. Yarroll <piggy@acm.org>
 *    Karl Knutson	    <karl@athena.chicago.il.us>
 *    Jon Grimm		    <jgrimm@us.ibm.com>
 *    Xingang Guo	    <xingang.guo@intel.com>
 *    Hui Huang		    <hui.huang@nokia.com>
 *    Sridhar Samudrala	    <sri@us.ibm.com>
 *    Daisy Chang	    <daisyc@us.ibm.com>
 *    Dajiang Zhang	    <dajiang.zhang@nokia.com>
 *    Ardelle Fan	    <ardelle.fan@intel.com>
 *    Ryan Layer	    <rmlayer@us.ibm.com>
 *    Anup Pemmaiah	    <pemmaiah@cc.usu.edu>
 *    Kevin Gao             <kevin.gao@intel.com>
 *
 * Any bugs reported given to us we will try to fix... any fixes shared will
 * be incorporated into the next SCTP release.
 */

#ifndef __sctp_structs_h__
#define __sctp_structs_h__

#include <linux/time.h>		
#include <linux/socket.h>	
#include <linux/in.h>		
#include <linux/in6.h>		
#include <linux/ipv6.h>
#include <asm/param.h>		
#include <linux/atomic.h>		
#include <linux/skbuff.h>	
#include <linux/workqueue.h>	
#include <linux/sctp.h>		
#include <net/sctp/auth.h>	

union sctp_addr {
	struct sockaddr_in v4;
	struct sockaddr_in6 v6;
	struct sockaddr sa;
};

struct sctp_globals;
struct sctp_endpoint;
struct sctp_association;
struct sctp_transport;
struct sctp_packet;
struct sctp_chunk;
struct sctp_inq;
struct sctp_outq;
struct sctp_bind_addr;
struct sctp_ulpq;
struct sctp_ep_common;
struct sctp_ssnmap;
struct crypto_hash;


#include <net/sctp/tsnmap.h>
#include <net/sctp/ulpevent.h>
#include <net/sctp/ulpqueue.h>


struct sctp_bind_bucket {
	unsigned short	port;
	unsigned short	fastreuse;
	struct hlist_node	node;
	struct hlist_head	owner;
};

struct sctp_bind_hashbucket {
	spinlock_t	lock;
	struct hlist_head	chain;
};

struct sctp_hashbucket {
	rwlock_t	lock;
	struct hlist_head	chain;
} __attribute__((__aligned__(8)));


extern struct sctp_globals {
	unsigned int rto_initial;
	unsigned int rto_min;
	unsigned int rto_max;

	int rto_alpha;
	int rto_beta;

	
	int max_burst;

	
	int cookie_preserve_enable;

	
	unsigned int valid_cookie_life;

	
	unsigned int sack_timeout;

	
	unsigned int hb_interval;

	int max_retrans_association;
	int max_retrans_path;
	int max_retrans_init;

	int sndbuf_policy;

	int rcvbuf_policy;

	

	
	__u16 max_instreams;
	__u16 max_outstreams;

	struct list_head address_families;

	
	int ep_hashsize;
	struct sctp_hashbucket *ep_hashtable;

	
	int assoc_hashsize;
	struct sctp_hashbucket *assoc_hashtable;

	
	int port_hashsize;
	struct sctp_bind_hashbucket *port_hashtable;

	struct list_head local_addr_list;
	int default_auto_asconf;
	struct list_head addr_waitq;
	struct timer_list addr_wq_timer;
	struct list_head auto_asconf_splist;
	spinlock_t addr_wq_lock;

	
	spinlock_t addr_list_lock;
	
	
	int addip_enable;
	int addip_noauth_enable;

	
	int prsctp_enable;

	
	int auth_enable;

	int ipv4_scope_policy;

        bool checksum_disable;

	int rwnd_update_shift;

	
	unsigned long max_autoclose;
} sctp_globals;

#define sctp_rto_initial		(sctp_globals.rto_initial)
#define sctp_rto_min			(sctp_globals.rto_min)
#define sctp_rto_max			(sctp_globals.rto_max)
#define sctp_rto_alpha			(sctp_globals.rto_alpha)
#define sctp_rto_beta			(sctp_globals.rto_beta)
#define sctp_max_burst			(sctp_globals.max_burst)
#define sctp_valid_cookie_life		(sctp_globals.valid_cookie_life)
#define sctp_cookie_preserve_enable	(sctp_globals.cookie_preserve_enable)
#define sctp_max_retrans_association	(sctp_globals.max_retrans_association)
#define sctp_sndbuf_policy	 	(sctp_globals.sndbuf_policy)
#define sctp_rcvbuf_policy	 	(sctp_globals.rcvbuf_policy)
#define sctp_max_retrans_path		(sctp_globals.max_retrans_path)
#define sctp_max_retrans_init		(sctp_globals.max_retrans_init)
#define sctp_sack_timeout		(sctp_globals.sack_timeout)
#define sctp_hb_interval		(sctp_globals.hb_interval)
#define sctp_max_instreams		(sctp_globals.max_instreams)
#define sctp_max_outstreams		(sctp_globals.max_outstreams)
#define sctp_address_families		(sctp_globals.address_families)
#define sctp_ep_hashsize		(sctp_globals.ep_hashsize)
#define sctp_ep_hashtable		(sctp_globals.ep_hashtable)
#define sctp_assoc_hashsize		(sctp_globals.assoc_hashsize)
#define sctp_assoc_hashtable		(sctp_globals.assoc_hashtable)
#define sctp_port_hashsize		(sctp_globals.port_hashsize)
#define sctp_port_hashtable		(sctp_globals.port_hashtable)
#define sctp_local_addr_list		(sctp_globals.local_addr_list)
#define sctp_local_addr_lock		(sctp_globals.addr_list_lock)
#define sctp_auto_asconf_splist		(sctp_globals.auto_asconf_splist)
#define sctp_addr_waitq			(sctp_globals.addr_waitq)
#define sctp_addr_wq_timer		(sctp_globals.addr_wq_timer)
#define sctp_addr_wq_lock		(sctp_globals.addr_wq_lock)
#define sctp_default_auto_asconf	(sctp_globals.default_auto_asconf)
#define sctp_scope_policy		(sctp_globals.ipv4_scope_policy)
#define sctp_addip_enable		(sctp_globals.addip_enable)
#define sctp_addip_noauth		(sctp_globals.addip_noauth_enable)
#define sctp_prsctp_enable		(sctp_globals.prsctp_enable)
#define sctp_auth_enable		(sctp_globals.auth_enable)
#define sctp_checksum_disable		(sctp_globals.checksum_disable)
#define sctp_rwnd_upd_shift		(sctp_globals.rwnd_update_shift)
#define sctp_max_autoclose		(sctp_globals.max_autoclose)

typedef enum {
	SCTP_SOCKET_UDP = 0,
	SCTP_SOCKET_UDP_HIGH_BANDWIDTH,
	SCTP_SOCKET_TCP
} sctp_socket_type_t;

struct sctp_sock {
	
	struct inet_sock inet;
	
	sctp_socket_type_t type;

	
	struct sctp_pf *pf;

	
	struct crypto_hash *hmac;

	
	struct sctp_endpoint *ep;

	struct sctp_bind_bucket *bind_hash;
	
	__u16 default_stream;
	__u32 default_ppid;
	__u16 default_flags;
	__u32 default_context;
	__u32 default_timetolive;
	__u32 default_rcv_context;
	int max_burst;

	__u32 hbinterval;

	
	__u16 pathmaxrxt;

	
	__u32 pathmtu;

	
	__u32 sackdelay;
	__u32 sackfreq;

	
	__u32 param_flags;

	struct sctp_initmsg initmsg;
	struct sctp_rtoinfo rtoinfo;
	struct sctp_paddrparams paddrparam;
	struct sctp_event_subscribe subscribe;
	struct sctp_assocparams assocparams;
	int user_frag;
	__u32 autoclose;
	__u8 nodelay;
	__u8 disable_fragments;
	__u8 v4mapped;
	__u8 frag_interleave;
	__u32 adaptation_ind;
	__u32 pd_point;

	atomic_t pd_mode;
	
	struct sk_buff_head pd_lobby;
	struct list_head auto_asconf_list;
	int do_auto_asconf;
};

static inline struct sctp_sock *sctp_sk(const struct sock *sk)
{
       return (struct sctp_sock *)sk;
}

static inline struct sock *sctp_opt2sk(const struct sctp_sock *sp)
{
       return (struct sock *)sp;
}

#if IS_ENABLED(CONFIG_IPV6)
struct sctp6_sock {
       struct sctp_sock  sctp;
       struct ipv6_pinfo inet6;
};
#endif 



struct sctp_cookie {

	__u32 my_vtag;

	__u32 peer_vtag;


	
	__u32 my_ttag;

	
	__u32 peer_ttag;

	
	struct timeval expiration;

	__u16 sinit_num_ostreams;
	__u16 sinit_max_instreams;

	
	__u32 initial_tsn;

	
	union sctp_addr peer_addr;

	__u16		my_port;

	__u8 prsctp_capable;

	
	__u8 padding;  		

	__u32 adaptation_ind;

	__u8 auth_random[sizeof(sctp_paramhdr_t) + SCTP_AUTH_RANDOM_LENGTH];
	__u8 auth_hmacs[SCTP_AUTH_NUM_HMACS * sizeof(__u16) + 2];
	__u8 auth_chunks[sizeof(sctp_paramhdr_t) + SCTP_AUTH_MAX_CHUNKS];

	__u32 raw_addr_list_len;
	struct sctp_init_chunk peer_init[0];
};


struct sctp_signed_cookie {
	__u8 signature[SCTP_SECRET_SIZE];
	__u32 __pad;		
	struct sctp_cookie c;
} __packed;

union sctp_addr_param {
	struct sctp_paramhdr p;
	struct sctp_ipv4addr_param v4;
	struct sctp_ipv6addr_param v6;
};

union sctp_params {
	void *v;
	struct sctp_paramhdr *p;
	struct sctp_cookie_preserve_param *life;
	struct sctp_hostname_param *dns;
	struct sctp_cookie_param *cookie;
	struct sctp_supported_addrs_param *sat;
	struct sctp_ipv4addr_param *v4;
	struct sctp_ipv6addr_param *v6;
	union sctp_addr_param *addr;
	struct sctp_adaptation_ind_param *aind;
	struct sctp_supported_ext_param *ext;
	struct sctp_random_param *random;
	struct sctp_chunks_param *chunks;
	struct sctp_hmac_algo_param *hmac_algo;
	struct sctp_addip_param *addip;
};

typedef struct sctp_sender_hb_info {
	struct sctp_paramhdr param_hdr;
	union sctp_addr daddr;
	unsigned long sent_at;
	__u64 hb_nonce;
} __packed sctp_sender_hb_info_t;


struct sctp_stream {
	__u16 *ssn;
	unsigned int len;
};

struct sctp_ssnmap {
	struct sctp_stream in;
	struct sctp_stream out;
	int malloced;
};

struct sctp_ssnmap *sctp_ssnmap_new(__u16 in, __u16 out,
				    gfp_t gfp);
void sctp_ssnmap_free(struct sctp_ssnmap *map);
void sctp_ssnmap_clear(struct sctp_ssnmap *map);

static inline __u16 sctp_ssn_peek(struct sctp_stream *stream, __u16 id)
{
	return stream->ssn[id];
}

static inline __u16 sctp_ssn_next(struct sctp_stream *stream, __u16 id)
{
	return stream->ssn[id]++;
}

static inline void sctp_ssn_skip(struct sctp_stream *stream, __u16 id, 
				 __u16 ssn)
{
	stream->ssn[id] = ssn+1;
}
              
struct sctp_af {
	int		(*sctp_xmit)	(struct sk_buff *skb,
					 struct sctp_transport *);
	int		(*setsockopt)	(struct sock *sk,
					 int level,
					 int optname,
					 char __user *optval,
					 unsigned int optlen);
	int		(*getsockopt)	(struct sock *sk,
					 int level,
					 int optname,
					 char __user *optval,
					 int __user *optlen);
	int		(*compat_setsockopt)	(struct sock *sk,
					 int level,
					 int optname,
					 char __user *optval,
					 unsigned int optlen);
	int		(*compat_getsockopt)	(struct sock *sk,
					 int level,
					 int optname,
					 char __user *optval,
					 int __user *optlen);
	void		(*get_dst)	(struct sctp_transport *t,
					 union sctp_addr *saddr,
					 struct flowi *fl,
					 struct sock *sk);
	void		(*get_saddr)	(struct sctp_sock *sk,
					 struct sctp_transport *t,
					 struct flowi *fl);
	void		(*copy_addrlist) (struct list_head *,
					  struct net_device *);
	int		(*cmp_addr)	(const union sctp_addr *addr1,
					 const union sctp_addr *addr2);
	void		(*addr_copy)	(union sctp_addr *dst,
					 union sctp_addr *src);
	void		(*from_skb)	(union sctp_addr *,
					 struct sk_buff *skb,
					 int saddr);
	void		(*from_sk)	(union sctp_addr *,
					 struct sock *sk);
	void		(*to_sk_saddr)	(union sctp_addr *,
					 struct sock *sk);
	void		(*to_sk_daddr)	(union sctp_addr *,
					 struct sock *sk);
	void		(*from_addr_param) (union sctp_addr *,
					    union sctp_addr_param *,
					    __be16 port, int iif);
	int		(*to_addr_param) (const union sctp_addr *,
					  union sctp_addr_param *); 
	int		(*addr_valid)	(union sctp_addr *,
					 struct sctp_sock *,
					 const struct sk_buff *);
	sctp_scope_t	(*scope) (union sctp_addr *);
	void		(*inaddr_any)	(union sctp_addr *, __be16);
	int		(*is_any)	(const union sctp_addr *);
	int		(*available)	(union sctp_addr *,
					 struct sctp_sock *);
	int		(*skb_iif)	(const struct sk_buff *sk);
	int		(*is_ce)	(const struct sk_buff *sk);
	void		(*seq_dump_addr)(struct seq_file *seq,
					 union sctp_addr *addr);
	void		(*ecn_capable)(struct sock *sk);
	__u16		net_header_len;
	int		sockaddr_len;
	sa_family_t	sa_family;
	struct list_head list;
};

struct sctp_af *sctp_get_af_specific(sa_family_t);
int sctp_register_af(struct sctp_af *);

struct sctp_pf {
	void (*event_msgname)(struct sctp_ulpevent *, char *, int *);
	void (*skb_msgname)  (struct sk_buff *, char *, int *);
	int  (*af_supported) (sa_family_t, struct sctp_sock *);
	int  (*cmp_addr) (const union sctp_addr *,
			  const union sctp_addr *,
			  struct sctp_sock *);
	int  (*bind_verify) (struct sctp_sock *, union sctp_addr *);
	int  (*send_verify) (struct sctp_sock *, union sctp_addr *);
	int  (*supported_addrs)(const struct sctp_sock *, __be16 *);
	struct sock *(*create_accept_sk) (struct sock *sk,
					  struct sctp_association *asoc);
	void (*addr_v4map) (struct sctp_sock *, union sctp_addr *);
	struct sctp_af *af;
};


struct sctp_datamsg {
	
	struct list_head chunks;
	
	atomic_t refcnt;
	
	unsigned long expires_at;
	
	int send_error;
	u8 send_failed:1,
	   can_abandon:1,   
	   can_delay;	    
};

struct sctp_datamsg *sctp_datamsg_from_user(struct sctp_association *,
					    struct sctp_sndrcvinfo *,
					    struct msghdr *, int len);
void sctp_datamsg_free(struct sctp_datamsg *);
void sctp_datamsg_put(struct sctp_datamsg *);
void sctp_chunk_fail(struct sctp_chunk *, int error);
int sctp_chunk_abandoned(struct sctp_chunk *);

struct sctp_chunk {
	struct list_head list;

	atomic_t refcnt;

	
	struct list_head transmitted_list;

	struct list_head frag_list;

	
	struct sk_buff *skb;


	
	union sctp_params param_hdr;
	union {
		__u8 *v;
		struct sctp_datahdr *data_hdr;
		struct sctp_inithdr *init_hdr;
		struct sctp_sackhdr *sack_hdr;
		struct sctp_heartbeathdr *hb_hdr;
		struct sctp_sender_hb_info *hbs_hdr;
		struct sctp_shutdownhdr *shutdown_hdr;
		struct sctp_signed_cookie *cookie_hdr;
		struct sctp_ecnehdr *ecne_hdr;
		struct sctp_cwrhdr *ecn_cwr_hdr;
		struct sctp_errhdr *err_hdr;
		struct sctp_addiphdr *addip_hdr;
		struct sctp_fwdtsn_hdr *fwdtsn_hdr;
		struct sctp_authhdr *auth_hdr;
	} subh;

	__u8 *chunk_end;

	struct sctp_chunkhdr *chunk_hdr;
	struct sctphdr *sctp_hdr;

	
	struct sctp_sndrcvinfo sinfo;

	
	struct sctp_association *asoc;

	
	struct sctp_ep_common *rcvr;

	
	unsigned long sent_at;

	
	union sctp_addr source;
	
	union sctp_addr dest;

	
	struct sctp_datamsg *msg;

	struct sctp_transport *transport;

	struct sk_buff *auth_chunk;

#define SCTP_CAN_FRTX 0x0
#define SCTP_NEED_FRTX 0x1
#define SCTP_DONT_FRTX 0x2
	__u16	rtt_in_progress:1,	
		has_tsn:1,		
		has_ssn:1,		
		singleton:1,		
		end_of_packet:1,	
		ecn_ce_done:1,		
		pdiscard:1,		
		tsn_gap_acked:1,	
		data_accepted:1,	
		auth:1,			
		has_asconf:1,		
		tsn_missing_report:2,	
		fast_retransmit:2;	
};

void sctp_chunk_hold(struct sctp_chunk *);
void sctp_chunk_put(struct sctp_chunk *);
int sctp_user_addto_chunk(struct sctp_chunk *chunk, int off, int len,
			  struct iovec *data);
void sctp_chunk_free(struct sctp_chunk *);
void  *sctp_addto_chunk(struct sctp_chunk *, int len, const void *data);
void  *sctp_addto_chunk_fixed(struct sctp_chunk *, int len, const void *data);
struct sctp_chunk *sctp_chunkify(struct sk_buff *,
				 const struct sctp_association *,
				 struct sock *);
void sctp_init_addrs(struct sctp_chunk *, union sctp_addr *,
		     union sctp_addr *);
const union sctp_addr *sctp_source(const struct sctp_chunk *chunk);

enum {
	SCTP_ADDR_NEW,		
	SCTP_ADDR_SRC,		
	SCTP_ADDR_DEL,		
};

struct sctp_sockaddr_entry {
	struct list_head list;
	struct rcu_head	rcu;
	union sctp_addr a;
	__u8 state;
	__u8 valid;
};

#define SCTP_ADDRESS_TICK_DELAY	500

typedef struct sctp_chunk *(sctp_packet_phandler_t)(struct sctp_association *);

struct sctp_packet {
	
	__u16 source_port;
	__u16 destination_port;
	__u32 vtag;

	
	struct list_head chunk_list;

	
	size_t overhead;
	
	size_t size;

	struct sctp_transport *transport;

	
	struct sctp_chunk *auth;

	u8  has_cookie_echo:1,	
	    has_sack:1,		
	    has_auth:1,		
	    has_data:1,		
	    ipfragok:1,		
	    malloced:1;		
};

struct sctp_packet *sctp_packet_init(struct sctp_packet *,
				     struct sctp_transport *,
				     __u16 sport, __u16 dport);
struct sctp_packet *sctp_packet_config(struct sctp_packet *, __u32 vtag, int);
sctp_xmit_t sctp_packet_transmit_chunk(struct sctp_packet *,
                                       struct sctp_chunk *, int);
sctp_xmit_t sctp_packet_append_chunk(struct sctp_packet *,
                                     struct sctp_chunk *);
int sctp_packet_transmit(struct sctp_packet *);
void sctp_packet_free(struct sctp_packet *);

static inline int sctp_packet_empty(struct sctp_packet *packet)
{
	return packet->size == packet->overhead;
}

struct sctp_transport {
	
	struct list_head transports;

	
	atomic_t refcnt;
	__u32	 dead:1,
		 rto_pending:1,

		hb_sent:1,

		
		pmtu_pending:1,

		
		malloced:1;

	struct flowi fl;

	
	union sctp_addr ipaddr;

	
	struct sctp_af *af_specific;

	
	struct sctp_association *asoc;

	
	unsigned long rto;

	__u32 rtt;		

	
	__u32 rttvar;

	
	__u32 srtt;

	
	__u32 cwnd;		  

	
	__u32 ssthresh;

	__u32 partial_bytes_acked;

	
	__u32 flight_size;

	__u32 burst_limited;	

	
	struct dst_entry *dst;
	
	union sctp_addr saddr;

	unsigned long hbinterval;

	
	unsigned long sackdelay;
	__u32 sackfreq;

	unsigned long last_time_heard;

	unsigned long last_time_ecne_reduced;

	__u16 pathmaxrxt;

	
	__u32 pathmtu;

	
	__u32 param_flags;

	
	int init_sent_count;

	int state;

	

	
	unsigned short error_count;

	struct timer_list T3_rtx_timer;

	
	struct timer_list hb_timer;

	
	struct timer_list proto_unreach_timer;

	struct list_head transmitted;

	
	struct sctp_packet packet;

	
	struct list_head send_ready;

	struct {
		__u32 next_tsn_at_change;

		
		char changeover_active;

		char cycling_changeover;

		char cacc_saw_newack;
	} cacc;

	
	__u64 hb_nonce;
};

struct sctp_transport *sctp_transport_new(const union sctp_addr *,
					  gfp_t);
void sctp_transport_set_owner(struct sctp_transport *,
			      struct sctp_association *);
void sctp_transport_route(struct sctp_transport *, union sctp_addr *,
			  struct sctp_sock *);
void sctp_transport_pmtu(struct sctp_transport *, struct sock *sk);
void sctp_transport_free(struct sctp_transport *);
void sctp_transport_reset_timers(struct sctp_transport *);
void sctp_transport_hold(struct sctp_transport *);
void sctp_transport_put(struct sctp_transport *);
void sctp_transport_update_rto(struct sctp_transport *, __u32);
void sctp_transport_raise_cwnd(struct sctp_transport *, __u32, __u32);
void sctp_transport_lower_cwnd(struct sctp_transport *, sctp_lower_cwnd_t);
void sctp_transport_burst_limited(struct sctp_transport *);
void sctp_transport_burst_reset(struct sctp_transport *);
unsigned long sctp_transport_timeout(struct sctp_transport *);
void sctp_transport_reset(struct sctp_transport *);
void sctp_transport_update_pmtu(struct sctp_transport *, u32);
void sctp_transport_immediate_rtx(struct sctp_transport *);


struct sctp_inq {
	struct list_head in_chunk_list;
	struct sctp_chunk *in_progress;

	struct work_struct immediate;

	int malloced;	     
};

void sctp_inq_init(struct sctp_inq *);
void sctp_inq_free(struct sctp_inq *);
void sctp_inq_push(struct sctp_inq *, struct sctp_chunk *packet);
struct sctp_chunk *sctp_inq_pop(struct sctp_inq *);
struct sctp_chunkhdr *sctp_inq_peek(struct sctp_inq *);
void sctp_inq_set_th_handler(struct sctp_inq *, work_func_t);

struct sctp_outq {
	struct sctp_association *asoc;

	
	struct list_head out_chunk_list;

	unsigned out_qlen;	

	
	unsigned error;

	
	struct list_head control_chunk_list;

	struct list_head sacked;

	struct list_head retransmit;

	struct list_head abandoned;

	
	__u32 outstanding_bytes;

	
	char fast_rtx;

	
	char cork;

	
	char empty;

	
	char malloced;
};

void sctp_outq_init(struct sctp_association *, struct sctp_outq *);
void sctp_outq_teardown(struct sctp_outq *);
void sctp_outq_free(struct sctp_outq*);
int sctp_outq_tail(struct sctp_outq *, struct sctp_chunk *chunk);
int sctp_outq_sack(struct sctp_outq *, struct sctp_sackhdr *);
int sctp_outq_is_empty(const struct sctp_outq *);
void sctp_outq_restart(struct sctp_outq *);

void sctp_retransmit(struct sctp_outq *, struct sctp_transport *,
		     sctp_retransmit_reason_t);
void sctp_retransmit_mark(struct sctp_outq *, struct sctp_transport *, __u8);
int sctp_outq_uncork(struct sctp_outq *);
static inline void sctp_outq_cork(struct sctp_outq *q)
{
	q->cork = 1;
}

struct sctp_bind_addr {

	__u16 port;

	struct list_head address_list;

	int malloced;	     
};

void sctp_bind_addr_init(struct sctp_bind_addr *, __u16 port);
void sctp_bind_addr_free(struct sctp_bind_addr *);
int sctp_bind_addr_copy(struct sctp_bind_addr *dest,
			const struct sctp_bind_addr *src,
			sctp_scope_t scope, gfp_t gfp,
			int flags);
int sctp_bind_addr_dup(struct sctp_bind_addr *dest,
			const struct sctp_bind_addr *src,
			gfp_t gfp);
int sctp_add_bind_addr(struct sctp_bind_addr *, union sctp_addr *,
		       __u8 addr_state, gfp_t gfp);
int sctp_del_bind_addr(struct sctp_bind_addr *, union sctp_addr *);
int sctp_bind_addr_match(struct sctp_bind_addr *, const union sctp_addr *,
			 struct sctp_sock *);
int sctp_bind_addr_conflict(struct sctp_bind_addr *, const union sctp_addr *,
			 struct sctp_sock *, struct sctp_sock *);
int sctp_bind_addr_state(const struct sctp_bind_addr *bp,
			 const union sctp_addr *addr);
union sctp_addr *sctp_find_unmatch_addr(struct sctp_bind_addr	*bp,
					const union sctp_addr	*addrs,
					int			addrcnt,
					struct sctp_sock	*opt);
union sctp_params sctp_bind_addrs_to_raw(const struct sctp_bind_addr *bp,
					 int *addrs_len,
					 gfp_t gfp);
int sctp_raw_to_bind_addrs(struct sctp_bind_addr *bp, __u8 *raw, int len,
			   __u16 port, gfp_t gfp);

sctp_scope_t sctp_scope(const union sctp_addr *);
int sctp_in_scope(const union sctp_addr *addr, const sctp_scope_t scope);
int sctp_is_any(struct sock *sk, const union sctp_addr *addr);
int sctp_addr_is_valid(const union sctp_addr *addr);
int sctp_is_ep_boundall(struct sock *sk);


typedef enum {
	SCTP_EP_TYPE_SOCKET,
	SCTP_EP_TYPE_ASSOCIATION,
} sctp_endpoint_type_t;


struct sctp_ep_common {
	
	struct hlist_node node;
	int hashent;

	
	sctp_endpoint_type_t type;

	atomic_t    refcnt;
	char	    dead;
	char	    malloced;

	
	struct sock *sk;

	
	struct sctp_inq	  inqueue;

	struct sctp_bind_addr bind_addr;
};



struct sctp_endpoint {
	
	struct sctp_ep_common base;

	
	struct list_head asocs;

	__u8 secret_key[SCTP_HOW_MANY_SECRETS][SCTP_SECRET_SIZE];
	int current_key;
	int last_key;
	int key_changed_at;

 	__u8 *digest;
 
	
	__u32 sndbuf_policy;

	
	__u32 rcvbuf_policy;

	struct crypto_hash **auth_hmacs;

	
	 struct sctp_hmac_algo_param *auth_hmacs_list;

	
	struct sctp_chunks_param *auth_chunk_list;

	
	struct list_head endpoint_shared_keys;
	__u16 active_key_id;
};

static inline struct sctp_endpoint *sctp_ep(struct sctp_ep_common *base)
{
	struct sctp_endpoint *ep;

	ep = container_of(base, struct sctp_endpoint, base);
	return ep;
}

struct sctp_endpoint *sctp_endpoint_new(struct sock *, gfp_t);
void sctp_endpoint_free(struct sctp_endpoint *);
void sctp_endpoint_put(struct sctp_endpoint *);
void sctp_endpoint_hold(struct sctp_endpoint *);
void sctp_endpoint_add_asoc(struct sctp_endpoint *, struct sctp_association *);
struct sctp_association *sctp_endpoint_lookup_assoc(
	const struct sctp_endpoint *ep,
	const union sctp_addr *paddr,
	struct sctp_transport **);
int sctp_endpoint_is_peeled_off(struct sctp_endpoint *,
				const union sctp_addr *);
struct sctp_endpoint *sctp_endpoint_is_match(struct sctp_endpoint *,
					const union sctp_addr *);
int sctp_has_association(const union sctp_addr *laddr,
			 const union sctp_addr *paddr);

int sctp_verify_init(const struct sctp_association *asoc, sctp_cid_t,
		     sctp_init_chunk_t *peer_init, struct sctp_chunk *chunk,
		     struct sctp_chunk **err_chunk);
int sctp_process_init(struct sctp_association *, struct sctp_chunk *chunk,
		      const union sctp_addr *peer,
		      sctp_init_chunk_t *init, gfp_t gfp);
__u32 sctp_generate_tag(const struct sctp_endpoint *);
__u32 sctp_generate_tsn(const struct sctp_endpoint *);

struct sctp_inithdr_host {
	__u32 init_tag;
	__u32 a_rwnd;
	__u16 num_outbound_streams;
	__u16 num_inbound_streams;
	__u32 initial_tsn;
};



struct sctp_association {

	struct sctp_ep_common base;

	
	struct list_head asocs;

	
	sctp_assoc_t assoc_id;

	
	struct sctp_endpoint *ep;

	
	struct sctp_cookie c;

	
	struct {
		__u32 rwnd;

		struct list_head transport_addr_list;

		__u16 transport_count;

		__u16 port;

		struct sctp_transport *primary_path;

		union sctp_addr primary_addr;

		struct sctp_transport *active_path;

		struct sctp_transport *retran_path;

		
		struct sctp_transport *last_sent_to;

		
		struct sctp_transport *last_data_from;

		struct sctp_tsnmap tsn_map;

		__u8    sack_needed;     
		__u32	sack_cnt;

		
		__u8	ecn_capable:1,	    
			ipv4_address:1,	    
			ipv6_address:1,	    
			hostname_address:1, 
			asconf_capable:1,   
			prsctp_capable:1,   
			auth_capable:1;	    

		__u32   adaptation_ind;	 

		__be16 addip_disabled_mask;

		struct sctp_inithdr_host i;
		int cookie_len;
		void *cookie;

		__u32 addip_serial;

		sctp_random_param_t *peer_random;
		sctp_chunks_param_t *peer_chunks;
		sctp_hmac_algo_param_t *peer_hmacs;
	} peer;

	sctp_state_t state;

	
	struct timeval cookie_life;

	int overall_error_count;

	unsigned long rto_initial;
	unsigned long rto_max;
	unsigned long rto_min;

	
	int max_burst;

	int max_retrans;

	
	__u16 max_init_attempts;

	
	__u16 init_retries;

	
	unsigned long max_init_timeo;

	unsigned long hbinterval;

	__u16 pathmaxrxt;

	
	__u8   pmtu_pending;

	__u32 pathmtu;

	
	__u32 param_flags;

	
	unsigned long sackdelay;
	__u32 sackfreq;


	unsigned long timeouts[SCTP_NUM_TIMEOUT_TYPES];
	struct timer_list timers[SCTP_NUM_TIMEOUT_TYPES];

	
	struct sctp_transport *shutdown_last_sent_to;

	
	int shutdown_retries;

	
	struct sctp_transport *init_last_sent_to;

	__u32 next_tsn;


	__u32 ctsn_ack_point;

	
	__u32 adv_peer_ack_point;

	
	__u32 highest_sacked;

	
	__u32 fast_recovery_exit;

	
	__u8 fast_recovery;

	__u16 unack_data;

	__u32 rtx_data_chunks;

	__u32 rwnd;

	
	__u32 a_rwnd;

	__u32 rwnd_over;

	__u32 rwnd_press;

	int sndbuf_used;

	atomic_t rmem_alloc;

	wait_queue_head_t	wait;

	
	__u32 frag_point;
	__u32 user_frag;

	
	int init_err_counter;

	
	int init_cycle;

	
	__u16 default_stream;
	__u16 default_flags;
	__u32 default_ppid;
	__u32 default_context;
	__u32 default_timetolive;

	
	__u32 default_rcv_context;

	
	struct sctp_ssnmap *ssnmap;

	
	struct sctp_outq outqueue;

	struct sctp_ulpq ulpq;

	
	__u32 last_ecne_tsn;

	
	__u32 last_cwr_tsn;

	
	int numduptsns;

	__u32 autoclose;




	struct sctp_chunk *addip_last_asconf;

	struct list_head asconf_ack_list;

	struct list_head addip_chunk_list;

	__u32 addip_serial;
	union sctp_addr *asconf_addr_del_pending;
	int src_out_of_asoc_ok;
	struct sctp_transport *new_transport;

	struct list_head endpoint_shared_keys;

	struct sctp_auth_bytes *asoc_shared_key;

	__u16 default_hmac_id;

	__u16 active_key_id;

	__u8 need_ecne:1,	
	     temp:1;		
};


enum {
	SCTP_ASSOC_EYECATCHER = 0xa550c123,
};

static inline struct sctp_association *sctp_assoc(struct sctp_ep_common *base)
{
	struct sctp_association *asoc;

	asoc = container_of(base, struct sctp_association, base);
	return asoc;
}



struct sctp_association *
sctp_association_new(const struct sctp_endpoint *, const struct sock *,
		     sctp_scope_t scope, gfp_t gfp);
void sctp_association_free(struct sctp_association *);
void sctp_association_put(struct sctp_association *);
void sctp_association_hold(struct sctp_association *);

struct sctp_transport *sctp_assoc_choose_alter_transport(
	struct sctp_association *, struct sctp_transport *);
void sctp_assoc_update_retran_path(struct sctp_association *);
struct sctp_transport *sctp_assoc_lookup_paddr(const struct sctp_association *,
					  const union sctp_addr *);
int sctp_assoc_lookup_laddr(struct sctp_association *asoc,
			    const union sctp_addr *laddr);
struct sctp_transport *sctp_assoc_add_peer(struct sctp_association *,
				     const union sctp_addr *address,
				     const gfp_t gfp,
				     const int peer_state);
void sctp_assoc_del_peer(struct sctp_association *asoc,
			 const union sctp_addr *addr);
void sctp_assoc_rm_peer(struct sctp_association *asoc,
			 struct sctp_transport *peer);
void sctp_assoc_control_transport(struct sctp_association *,
				  struct sctp_transport *,
				  sctp_transport_cmd_t, sctp_sn_error_t);
struct sctp_transport *sctp_assoc_lookup_tsn(struct sctp_association *, __u32);
struct sctp_transport *sctp_assoc_is_match(struct sctp_association *,
					   const union sctp_addr *,
					   const union sctp_addr *);
void sctp_assoc_migrate(struct sctp_association *, struct sock *);
void sctp_assoc_update(struct sctp_association *old,
		       struct sctp_association *new);

__u32 sctp_association_get_next_tsn(struct sctp_association *);

void sctp_assoc_sync_pmtu(struct sctp_association *);
void sctp_assoc_rwnd_increase(struct sctp_association *, unsigned);
void sctp_assoc_rwnd_decrease(struct sctp_association *, unsigned);
void sctp_assoc_set_primary(struct sctp_association *,
			    struct sctp_transport *);
void sctp_assoc_del_nonprimary_peers(struct sctp_association *,
				    struct sctp_transport *);
int sctp_assoc_set_bind_addr_from_ep(struct sctp_association *,
				     sctp_scope_t, gfp_t);
int sctp_assoc_set_bind_addr_from_cookie(struct sctp_association *,
					 struct sctp_cookie*,
					 gfp_t gfp);
int sctp_assoc_set_id(struct sctp_association *, gfp_t);
void sctp_assoc_clean_asconf_ack_cache(const struct sctp_association *asoc);
struct sctp_chunk *sctp_assoc_lookup_asconf_ack(
					const struct sctp_association *asoc,
					__be32 serial);
void sctp_asconf_queue_teardown(struct sctp_association *asoc);

int sctp_cmp_addr_exact(const union sctp_addr *ss1,
			const union sctp_addr *ss2);
struct sctp_chunk *sctp_get_ecne_prepend(struct sctp_association *asoc);

typedef struct sctp_cmsgs {
	struct sctp_initmsg *init;
	struct sctp_sndrcvinfo *info;
} sctp_cmsgs_t;

typedef struct {
	char *label;
	atomic_t *counter;
} sctp_dbg_objcnt_entry_t;

#endif 
