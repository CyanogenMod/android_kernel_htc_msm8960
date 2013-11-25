#ifndef _NF_CONNTRACK_TCP_H
#define _NF_CONNTRACK_TCP_H

#include <linux/types.h>

enum tcp_conntrack {
	TCP_CONNTRACK_NONE,
	TCP_CONNTRACK_SYN_SENT,
	TCP_CONNTRACK_SYN_RECV,
	TCP_CONNTRACK_ESTABLISHED,
	TCP_CONNTRACK_FIN_WAIT,
	TCP_CONNTRACK_CLOSE_WAIT,
	TCP_CONNTRACK_LAST_ACK,
	TCP_CONNTRACK_TIME_WAIT,
	TCP_CONNTRACK_CLOSE,
	TCP_CONNTRACK_LISTEN,	
#define TCP_CONNTRACK_SYN_SENT2	TCP_CONNTRACK_LISTEN
	TCP_CONNTRACK_MAX,
	TCP_CONNTRACK_IGNORE,
	TCP_CONNTRACK_RETRANS,
	TCP_CONNTRACK_UNACK,
	TCP_CONNTRACK_TIMEOUT_MAX
};

#define IP_CT_TCP_FLAG_WINDOW_SCALE		0x01

#define IP_CT_TCP_FLAG_SACK_PERM		0x02

#define IP_CT_TCP_FLAG_CLOSE_INIT		0x04

#define IP_CT_TCP_FLAG_BE_LIBERAL		0x08

#define IP_CT_TCP_FLAG_DATA_UNACKNOWLEDGED	0x10

#define IP_CT_TCP_FLAG_MAXACK_SET		0x20

struct nf_ct_tcp_flags {
	__u8 flags;
	__u8 mask;
};

#ifdef __KERNEL__

struct ip_ct_tcp_state {
	u_int32_t	td_end;		
	u_int32_t	td_maxend;	
	u_int32_t	td_maxwin;	
	u_int32_t	td_maxack;	
	u_int8_t	td_scale;	
	u_int8_t	flags;		
};

struct ip_ct_tcp {
	struct ip_ct_tcp_state seen[2];	
	u_int8_t	state;		
	
	u_int8_t	last_dir;	
	u_int8_t	retrans;	
	u_int8_t	last_index;	
	u_int32_t	last_seq;	
	u_int32_t	last_ack;	
	u_int32_t	last_end;	
	u_int16_t	last_win;	
	
	u_int8_t	last_wscale;	
	u_int8_t	last_flags;	
};

#endif 

#endif 
