#ifndef __LINUX_IP_NETFILTER_H
#define __LINUX_IP_NETFILTER_H

/* IPv4-specific defines for netfilter. 
 * (C)1998 Rusty Russell -- This code is GPL.
 */

#include <linux/netfilter.h>

#ifndef __KERNEL__

#include <limits.h> 

#define NFC_IP_SRC		0x0001
#define NFC_IP_DST		0x0002
#define NFC_IP_IF_IN		0x0004
#define NFC_IP_IF_OUT		0x0008
#define NFC_IP_TOS		0x0010
#define NFC_IP_PROTO		0x0020
#define NFC_IP_OPTIONS		0x0040
#define NFC_IP_FRAG		0x0080

#define NFC_IP_TCPFLAGS		0x0100
#define NFC_IP_SRC_PT		0x0200
#define NFC_IP_DST_PT		0x0400
#define NFC_IP_PROTO_UNKNOWN	0x2000

#define NF_IP_PRE_ROUTING	0
#define NF_IP_LOCAL_IN		1
#define NF_IP_FORWARD		2
#define NF_IP_LOCAL_OUT		3
#define NF_IP_POST_ROUTING	4
#define NF_IP_NUMHOOKS		5
#endif 

enum nf_ip_hook_priorities {
	NF_IP_PRI_FIRST = INT_MIN,
	NF_IP_PRI_CONNTRACK_DEFRAG = -400,
	NF_IP_PRI_RAW = -300,
	NF_IP_PRI_SELINUX_FIRST = -225,
	NF_IP_PRI_CONNTRACK = -200,
	NF_IP_PRI_MANGLE = -150,
	NF_IP_PRI_NAT_DST = -100,
	NF_IP_PRI_FILTER = 0,
	NF_IP_PRI_SECURITY = 50,
	NF_IP_PRI_NAT_SRC = 100,
	NF_IP_PRI_SELINUX_LAST = 225,
	NF_IP_PRI_CONNTRACK_CONFIRM = INT_MAX,
	NF_IP_PRI_LAST = INT_MAX,
};

#define SO_ORIGINAL_DST 80

#ifdef __KERNEL__
extern int ip_route_me_harder(struct sk_buff *skb, unsigned addr_type);
extern int ip_xfrm_me_harder(struct sk_buff *skb);
extern __sum16 nf_ip_checksum(struct sk_buff *skb, unsigned int hook,
				   unsigned int dataoff, u_int8_t protocol);
#endif 

#endif 
