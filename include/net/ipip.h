#ifndef __NET_IPIP_H
#define __NET_IPIP_H 1

#include <linux/if_tunnel.h>
#include <net/ip.h>

#define IPTUNNEL_ERR_TIMEO	(30*HZ)

struct ip_tunnel_6rd_parm {
	struct in6_addr		prefix;
	__be32			relay_prefix;
	u16			prefixlen;
	u16			relay_prefixlen;
};

struct ip_tunnel {
	struct ip_tunnel __rcu	*next;
	struct net_device	*dev;

	int			err_count;	
	unsigned long		err_time;	

	
	__u32			i_seqno;	
	__u32			o_seqno;	
	int			hlen;		
	int			mlink;

	struct ip_tunnel_parm	parms;

	
#ifdef CONFIG_IPV6_SIT_6RD
	struct ip_tunnel_6rd_parm	ip6rd;
#endif
	struct ip_tunnel_prl_entry __rcu *prl;		
	unsigned int			prl_count;	
};

struct ip_tunnel_prl_entry {
	struct ip_tunnel_prl_entry __rcu *next;
	__be32				addr;
	u16				flags;
	struct rcu_head			rcu_head;
};

#define __IPTUNNEL_XMIT(stats1, stats2) do {				\
	int err;							\
	int pkt_len = skb->len - skb_transport_offset(skb);		\
									\
	skb->ip_summed = CHECKSUM_NONE;					\
	ip_select_ident(iph, &rt->dst, NULL);				\
									\
	err = ip_local_out(skb);					\
	if (likely(net_xmit_eval(err) == 0)) {				\
		(stats1)->tx_bytes += pkt_len;				\
		(stats1)->tx_packets++;					\
	} else {							\
		(stats2)->tx_errors++;					\
		(stats2)->tx_aborted_errors++;				\
	}								\
} while (0)

#define IPTUNNEL_XMIT() __IPTUNNEL_XMIT(txq, stats)

#endif
