/***************************************************************************
 * Linux PPP over X - Generic PPP transport layer sockets
 * Linux PPP over Ethernet (PPPoE) Socket Implementation (RFC 2516) 
 *
 * This file supplies definitions required by the PPP over Ethernet driver
 * (pppox.c).  All version information wrt this file is located in pppox.c
 *
 * License:
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 */

#ifndef __LINUX_IF_PPPOX_H
#define __LINUX_IF_PPPOX_H


#include <linux/types.h>
#include <asm/byteorder.h>

#include <linux/socket.h>
#include <linux/if_ether.h>
#ifdef  __KERNEL__
#include <linux/if.h>
#include <linux/netdevice.h>
#include <linux/ppp_channel.h>
#endif 
#include <linux/if_pppol2tp.h>
#include <linux/if_pppolac.h>
#include <linux/if_pppopns.h>

#ifndef AF_PPPOX
#define AF_PPPOX	24
#define PF_PPPOX	AF_PPPOX
#endif 

 
typedef __be16 sid_t;
struct pppoe_addr {
	sid_t         sid;                    
	unsigned char remote[ETH_ALEN];       
	char          dev[IFNAMSIZ];          
}; 
 
struct pptp_addr {
	__be16		call_id;
	struct in_addr	sin_addr;
};

#define PX_PROTO_OE    0 
#define PX_PROTO_OL2TP 1 
#define PX_PROTO_PPTP  2
#define PX_PROTO_OLAC  3
#define PX_PROTO_OPNS  4
#define PX_MAX_PROTO   5

struct sockaddr_pppox {
	__kernel_sa_family_t sa_family;       
	unsigned int    sa_protocol;          
	union {
		struct pppoe_addr  pppoe;
		struct pptp_addr   pptp;
	} sa_addr;
} __attribute__((packed));

struct sockaddr_pppol2tp {
	__kernel_sa_family_t sa_family; 
	unsigned int    sa_protocol;    
	struct pppol2tp_addr pppol2tp;
} __attribute__((packed));

struct sockaddr_pppol2tpv3 {
	__kernel_sa_family_t sa_family; 
	unsigned int    sa_protocol;    
	struct pppol2tpv3_addr pppol2tp;
} __attribute__((packed));


#define PPPOEIOCSFWD	_IOW(0xB1 ,0, size_t)
#define PPPOEIOCDFWD	_IO(0xB1 ,1)

#define PADI_CODE	0x09
#define PADO_CODE	0x07
#define PADR_CODE	0x19
#define PADS_CODE	0x65
#define PADT_CODE	0xa7
struct pppoe_tag {
	__be16 tag_type;
	__be16 tag_len;
	char tag_data[0];
} __attribute__ ((packed));

#define PTT_EOL		__cpu_to_be16(0x0000)
#define PTT_SRV_NAME	__cpu_to_be16(0x0101)
#define PTT_AC_NAME	__cpu_to_be16(0x0102)
#define PTT_HOST_UNIQ	__cpu_to_be16(0x0103)
#define PTT_AC_COOKIE	__cpu_to_be16(0x0104)
#define PTT_VENDOR 	__cpu_to_be16(0x0105)
#define PTT_RELAY_SID	__cpu_to_be16(0x0110)
#define PTT_SRV_ERR     __cpu_to_be16(0x0201)
#define PTT_SYS_ERR  	__cpu_to_be16(0x0202)
#define PTT_GEN_ERR  	__cpu_to_be16(0x0203)

struct pppoe_hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	__u8 ver : 4;
	__u8 type : 4;
#elif defined(__BIG_ENDIAN_BITFIELD)
	__u8 type : 4;
	__u8 ver : 4;
#else
#error	"Please fix <asm/byteorder.h>"
#endif
	__u8 code;
	__be16 sid;
	__be16 length;
	struct pppoe_tag tag[0];
} __attribute__((packed));

#define PPPOE_SES_HLEN	8

#ifdef __KERNEL__
#include <linux/skbuff.h>

static inline struct pppoe_hdr *pppoe_hdr(const struct sk_buff *skb)
{
	return (struct pppoe_hdr *)skb_network_header(skb);
}

struct pppoe_opt {
	struct net_device      *dev;	  
	int			ifindex;  
	struct pppoe_addr	pa;	  
	struct sockaddr_pppox	relay;	  
};

struct pptp_opt {
	struct pptp_addr src_addr;
	struct pptp_addr dst_addr;
	u32 ack_sent, ack_recv;
	u32 seq_sent, seq_recv;
	int ppp_flags;
};

struct pppolac_opt {
	__u32		local;
	__u32		remote;
	__u32		recv_sequence;
	__u32		xmit_sequence;
	atomic_t	sequencing;
	int		(*backlog_rcv)(struct sock *sk_udp, struct sk_buff *skb);
};

struct pppopns_opt {
	__u16		local;
	__u16		remote;
	__u32		recv_sequence;
	__u32		xmit_sequence;
	void		(*data_ready)(struct sock *sk_raw, int length);
	int		(*backlog_rcv)(struct sock *sk_raw, struct sk_buff *skb);
};

#include <net/sock.h>

struct pppox_sock {
	
	struct sock sk;
	struct ppp_channel chan;
	struct pppox_sock	*next;	  
	union {
		struct pppoe_opt pppoe;
		struct pptp_opt  pptp;
		struct pppolac_opt lac;
		struct pppopns_opt pns;
	} proto;
	__be16			num;
};
#define pppoe_dev	proto.pppoe.dev
#define pppoe_ifindex	proto.pppoe.ifindex
#define pppoe_pa	proto.pppoe.pa
#define pppoe_relay	proto.pppoe.relay

static inline struct pppox_sock *pppox_sk(struct sock *sk)
{
	return (struct pppox_sock *)sk;
}

static inline struct sock *sk_pppox(struct pppox_sock *po)
{
	return (struct sock *)po;
}

struct module;

struct pppox_proto {
	int		(*create)(struct net *net, struct socket *sock);
	int		(*ioctl)(struct socket *sock, unsigned int cmd,
				 unsigned long arg);
	struct module	*owner;
};

extern int register_pppox_proto(int proto_num, const struct pppox_proto *pp);
extern void unregister_pppox_proto(int proto_num);
extern void pppox_unbind_sock(struct sock *sk);
extern int pppox_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);

enum {
    PPPOX_NONE		= 0,  
    PPPOX_CONNECTED	= 1,  
    PPPOX_BOUND		= 2,  
    PPPOX_RELAY		= 4,  
    PPPOX_ZOMBIE	= 8,  
    PPPOX_DEAD		= 16  
};

#endif 

#endif 
