/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Definitions for the TCP protocol.
 *
 * Version:	@(#)tcp.h	1.0.2	04/28/93
 *
 * Author:	Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
#ifndef _LINUX_TCP_H
#define _LINUX_TCP_H

#include <linux/types.h>
#include <asm/byteorder.h>
#include <linux/socket.h>

struct tcphdr {
	__be16	source;
	__be16	dest;
	__be32	seq;
	__be32	ack_seq;
#if defined(__LITTLE_ENDIAN_BITFIELD)
	__u16	res1:4,
		doff:4,
		fin:1,
		syn:1,
		rst:1,
		psh:1,
		ack:1,
		urg:1,
		ece:1,
		cwr:1;
#elif defined(__BIG_ENDIAN_BITFIELD)
	__u16	doff:4,
		res1:4,
		cwr:1,
		ece:1,
		urg:1,
		ack:1,
		psh:1,
		rst:1,
		syn:1,
		fin:1;
#else
#error	"Adjust your <asm/byteorder.h> defines"
#endif	
	__be16	window;
	__sum16	check;
	__be16	urg_ptr;
};

union tcp_word_hdr { 
	struct tcphdr hdr;
	__be32 		  words[5];
}; 

#define tcp_flag_word(tp) ( ((union tcp_word_hdr *)(tp))->words [3]) 

enum {
	TCP_FLAG_CWR = __constant_htonl(0x00800000),
	TCP_FLAG_ECE = __constant_htonl(0x00400000),
	TCP_FLAG_URG = __constant_htonl(0x00200000),
	TCP_FLAG_ACK = __constant_htonl(0x00100000),
	TCP_FLAG_PSH = __constant_htonl(0x00080000),
	TCP_FLAG_RST = __constant_htonl(0x00040000),
	TCP_FLAG_SYN = __constant_htonl(0x00020000),
	TCP_FLAG_FIN = __constant_htonl(0x00010000),
	TCP_RESERVED_BITS = __constant_htonl(0x0F000000),
	TCP_DATA_OFFSET = __constant_htonl(0xF0000000)
};

#define TCP_MSS_DEFAULT		 536U	
#define TCP_MSS_DESIRED		1220U	

#define TCP_NODELAY		1	
#define TCP_MAXSEG		2	
#define TCP_CORK		3	
#define TCP_KEEPIDLE		4	
#define TCP_KEEPINTVL		5	
#define TCP_KEEPCNT		6	
#define TCP_SYNCNT		7	
#define TCP_LINGER2		8	
#define TCP_DEFER_ACCEPT	9	
#define TCP_WINDOW_CLAMP	10	
#define TCP_INFO		11	
#define TCP_QUICKACK		12	
#define TCP_CONGESTION		13	
#define TCP_MD5SIG		14	
#define TCP_COOKIE_TRANSACTIONS	15	
#define TCP_THIN_LINEAR_TIMEOUTS 16      
#define TCP_THIN_DUPACK         17      
#define TCP_USER_TIMEOUT	18	

#define TCPI_OPT_TIMESTAMPS	1
#define TCPI_OPT_SACK		2
#define TCPI_OPT_WSCALE		4
#define TCPI_OPT_ECN		8 
#define TCPI_OPT_ECN_SEEN	16 

enum tcp_ca_state {
	TCP_CA_Open = 0,
#define TCPF_CA_Open	(1<<TCP_CA_Open)
	TCP_CA_Disorder = 1,
#define TCPF_CA_Disorder (1<<TCP_CA_Disorder)
	TCP_CA_CWR = 2,
#define TCPF_CA_CWR	(1<<TCP_CA_CWR)
	TCP_CA_Recovery = 3,
#define TCPF_CA_Recovery (1<<TCP_CA_Recovery)
	TCP_CA_Loss = 4
#define TCPF_CA_Loss	(1<<TCP_CA_Loss)
};

struct tcp_info {
	__u8	tcpi_state;
	__u8	tcpi_ca_state;
	__u8	tcpi_retransmits;
	__u8	tcpi_probes;
	__u8	tcpi_backoff;
	__u8	tcpi_options;
	__u8	tcpi_snd_wscale : 4, tcpi_rcv_wscale : 4;
	__u8    tcpi_count;

	__u32	tcpi_rto;
	__u32	tcpi_ato;
	__u32	tcpi_snd_mss;
	__u32	tcpi_rcv_mss;

	__u32	tcpi_unacked;
	__u32	tcpi_sacked;
	__u32	tcpi_lost;
	__u32	tcpi_retrans;
	__u32	tcpi_fackets;

	
	__u32	tcpi_last_data_sent;
	__u32	tcpi_last_ack_sent;     
	__u32	tcpi_last_data_recv;
	__u32	tcpi_last_ack_recv;

	
	__u32	tcpi_pmtu;
	__u32	tcpi_rcv_ssthresh;
	__u32	tcpi_rtt;
	__u32	tcpi_rttvar;
	__u32	tcpi_snd_ssthresh;
	__u32	tcpi_snd_cwnd;
	__u32	tcpi_advmss;
	__u32	tcpi_reordering;

	__u32	tcpi_rcv_rtt;
	__u32	tcpi_rcv_space;

	__u32	tcpi_total_retrans;
};

#define TCP_MD5SIG_MAXKEYLEN	80

struct tcp_md5sig {
	struct __kernel_sockaddr_storage tcpm_addr;	
	__u16	__tcpm_pad1;				
	__u16	tcpm_keylen;				
	__u32	__tcpm_pad2;				
	__u8	tcpm_key[TCP_MD5SIG_MAXKEYLEN];		
};

#define TCP_COOKIE_MIN		 8		
#define TCP_COOKIE_MAX		16		
#define TCP_COOKIE_PAIR_SIZE	(2*TCP_COOKIE_MAX)

#define TCP_COOKIE_IN_ALWAYS	(1 << 0)	
#define TCP_COOKIE_OUT_NEVER	(1 << 1)	

#define TCP_S_DATA_IN		(1 << 2)	
#define TCP_S_DATA_OUT		(1 << 3)	

struct tcp_cookie_transactions {
	__u16	tcpct_flags;			
	__u8	__tcpct_pad1;			
	__u8	tcpct_cookie_desired;		
	__u16	tcpct_s_data_desired;		
	__u16	tcpct_used;			
	__u8	tcpct_value[TCP_MSS_DEFAULT];
};

#ifdef __KERNEL__

#include <linux/skbuff.h>
#include <linux/dmaengine.h>
#include <net/sock.h>
#include <net/inet_connection_sock.h>
#include <net/inet_timewait_sock.h>

static inline struct tcphdr *tcp_hdr(const struct sk_buff *skb)
{
	return (struct tcphdr *)skb_transport_header(skb);
}

static inline unsigned int tcp_hdrlen(const struct sk_buff *skb)
{
	return tcp_hdr(skb)->doff * 4;
}

static inline unsigned int tcp_optlen(const struct sk_buff *skb)
{
	return (tcp_hdr(skb)->doff - 5) * 4;
}

struct tcp_sack_block_wire {
	__be32	start_seq;
	__be32	end_seq;
};

struct tcp_sack_block {
	u32	start_seq;
	u32	end_seq;
};

#define TCP_SACK_SEEN     (1 << 0)   
#define TCP_FACK_ENABLED  (1 << 1)   
#define TCP_DSACK_SEEN    (1 << 2)   

struct tcp_options_received {
	long	ts_recent_stamp;
	u32	ts_recent;	
	u32	rcv_tsval;	
	u32	rcv_tsecr;	
	u16 	saw_tstamp : 1,	
		tstamp_ok : 1,	
		dsack : 1,	
		wscale_ok : 1,	
		sack_ok : 4,	
		snd_wscale : 4,	
		rcv_wscale : 4;	
	u8	cookie_plus:6,	
		cookie_out_never:1,
		cookie_in_always:1;
	u8	num_sacks;	
	u16	user_mss;	
	u16	mss_clamp;	
};

static inline void tcp_clear_options(struct tcp_options_received *rx_opt)
{
	rx_opt->tstamp_ok = rx_opt->sack_ok = 0;
	rx_opt->wscale_ok = rx_opt->snd_wscale = 0;
	rx_opt->cookie_plus = 0;
}

#define TCP_NUM_SACKS 4

struct tcp_cookie_values;
struct tcp_request_sock_ops;

struct tcp_request_sock {
	struct inet_request_sock 	req;
#ifdef CONFIG_TCP_MD5SIG
	
	const struct tcp_request_sock_ops *af_specific;
#endif
	u32				rcv_isn;
	u32				snt_isn;
	u32				snt_synack; 
};

static inline struct tcp_request_sock *tcp_rsk(const struct request_sock *req)
{
	return (struct tcp_request_sock *)req;
}

struct tcp_sock {
	
	struct inet_connection_sock	inet_conn;
	u16	tcp_header_len;	
	u16	xmit_size_goal_segs; 

	__be32	pred_flags;

 	u32	rcv_nxt;	
	u32	copied_seq;	
	u32	rcv_wup;	
 	u32	snd_nxt;	

 	u32	snd_una;	
 	u32	snd_sml;	
	u32	rcv_tstamp;	
	u32	lsndtime;	

	
	struct {
		struct sk_buff_head	prequeue;
		struct task_struct	*task;
		struct iovec		*iov;
		int			memory;
		int			len;
#ifdef CONFIG_NET_DMA
		
		struct dma_chan		*dma_chan;
		int			wakeup;
		struct dma_pinned_list	*pinned_list;
		dma_cookie_t		dma_cookie;
#endif
	} ucopy;

	u32	snd_wl1;	
	u32	snd_wnd;	
	u32	max_window;	
	u32	mss_cache;	

	u32	window_clamp;	
	u32	rcv_ssthresh;	

	u32	frto_highmark;	
	u16	advmss;		
	u8	frto_counter;	
	u8	nonagle     : 4,
		thin_lto    : 1,
		thin_dupack : 1,
		unused      : 2;

	u32	srtt;		
	u32	mdev;		
	u32	mdev_max;	
	u32	rttvar;		
	u32	rtt_seq;	

	u32	packets_out;	
	u32	retrans_out;	

	u16	urg_data;	
	u8	ecn_flags;	
	u8	reordering;	
	u32	snd_up;		

	u8	keepalive_probes; 
	struct tcp_options_received rx_opt;

 	u32	snd_ssthresh;	
 	u32	snd_cwnd;	
	u32	snd_cwnd_cnt;	
	u32	snd_cwnd_clamp; 
	u32	snd_cwnd_used;
	u32	snd_cwnd_stamp;
	u32	prior_cwnd;	
	u32	prr_delivered;	
	u32	prr_out;	

 	u32	rcv_wnd;	
	u32	write_seq;	
	u32	pushed_seq;	
	u32	lost_out;	
	u32	sacked_out;	
	u32	fackets_out;	
	u32	tso_deferred;
	u32	bytes_acked;	

	
	struct sk_buff* lost_skb_hint;
	struct sk_buff *scoreboard_skb_hint;
	struct sk_buff *retransmit_skb_hint;

	struct sk_buff_head	out_of_order_queue; 

	
	struct tcp_sack_block duplicate_sack[1]; 
	struct tcp_sack_block selective_acks[4]; 

	struct tcp_sack_block recv_sack_cache[4];

	struct sk_buff *highest_sack;   

	int     lost_cnt_hint;
	u32     retransmit_high;	

	u32	lost_retrans_low;	

	u32	prior_ssthresh; 
	u32	high_seq;	

	u32	retrans_stamp;	
	u32	undo_marker;	
	int	undo_retrans;	
	u32	total_retrans;	

	u32	urg_seq;	
	unsigned int		keepalive_time;	  
	unsigned int		keepalive_intvl;  

	int			linger2;

	struct {
		u32	rtt;
		u32	seq;
		u32	time;
	} rcv_rtt_est;

	struct {
		int	space;
		u32	seq;
		u32	time;
	} rcvq_space;

	struct {
		u32		  probe_seq_start;
		u32		  probe_seq_end;
	} mtu_probe;

#ifdef CONFIG_TCP_MD5SIG
	const struct tcp_sock_af_ops	*af_specific;

	struct tcp_md5sig_info	__rcu *md5sig_info;
#endif

	struct tcp_cookie_values  *cookie_values;
};

static inline struct tcp_sock *tcp_sk(const struct sock *sk)
{
	return (struct tcp_sock *)sk;
}

struct tcp_timewait_sock {
	struct inet_timewait_sock tw_sk;
	u32			  tw_rcv_nxt;
	u32			  tw_snd_nxt;
	u32			  tw_rcv_wnd;
	u32			  tw_ts_recent;
	long			  tw_ts_recent_stamp;
#ifdef CONFIG_TCP_MD5SIG
	struct tcp_md5sig_key	*tw_md5_key;
#endif
	struct tcp_cookie_values  *tw_cookie_values;
};

static inline struct tcp_timewait_sock *tcp_twsk(const struct sock *sk)
{
	return (struct tcp_timewait_sock *)sk;
}

#endif	

#endif	
