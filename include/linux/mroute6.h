#ifndef __LINUX_MROUTE6_H
#define __LINUX_MROUTE6_H

#include <linux/types.h>
#include <linux/sockios.h>


#define MRT6_BASE	200
#define MRT6_INIT	(MRT6_BASE)	
#define MRT6_DONE	(MRT6_BASE+1)	
#define MRT6_ADD_MIF	(MRT6_BASE+2)	
#define MRT6_DEL_MIF	(MRT6_BASE+3)	
#define MRT6_ADD_MFC	(MRT6_BASE+4)	
#define MRT6_DEL_MFC	(MRT6_BASE+5)	
#define MRT6_VERSION	(MRT6_BASE+6)	
#define MRT6_ASSERT	(MRT6_BASE+7)	
#define MRT6_PIM	(MRT6_BASE+8)	
#define MRT6_TABLE	(MRT6_BASE+9)	

#define SIOCGETMIFCNT_IN6	SIOCPROTOPRIVATE	
#define SIOCGETSGCNT_IN6	(SIOCPROTOPRIVATE+1)
#define SIOCGETRPF	(SIOCPROTOPRIVATE+2)

#define MAXMIFS		32
typedef unsigned long mifbitmap_t;	
typedef unsigned short mifi_t;
#define ALL_MIFS	((mifi_t)(-1))

#ifndef IF_SETSIZE
#define IF_SETSIZE	256
#endif

typedef	__u32		if_mask;
#define NIFBITS (sizeof(if_mask) * 8)        

#if !defined(__KERNEL__)
#if !defined(DIV_ROUND_UP)
#define	DIV_ROUND_UP(x,y)	(((x) + ((y) - 1)) / (y))
#endif
#endif

typedef struct if_set {
	if_mask ifs_bits[DIV_ROUND_UP(IF_SETSIZE, NIFBITS)];
} if_set;

#define IF_SET(n, p)    ((p)->ifs_bits[(n)/NIFBITS] |= (1 << ((n) % NIFBITS)))
#define IF_CLR(n, p)    ((p)->ifs_bits[(n)/NIFBITS] &= ~(1 << ((n) % NIFBITS)))
#define IF_ISSET(n, p)  ((p)->ifs_bits[(n)/NIFBITS] & (1 << ((n) % NIFBITS)))
#define IF_COPY(f, t)   bcopy(f, t, sizeof(*(f)))
#define IF_ZERO(p)      bzero(p, sizeof(*(p)))


struct mif6ctl {
	mifi_t	mif6c_mifi;		
	unsigned char mif6c_flags;	
	unsigned char vifc_threshold;	
	__u16	 mif6c_pifi;		
	unsigned int vifc_rate_limit;	
};

#define MIFF_REGISTER	0x1	


struct mf6cctl {
	struct sockaddr_in6 mf6cc_origin;		
	struct sockaddr_in6 mf6cc_mcastgrp;		
	mifi_t	mf6cc_parent;			
	struct if_set mf6cc_ifset;		
};


struct sioc_sg_req6 {
	struct sockaddr_in6 src;
	struct sockaddr_in6 grp;
	unsigned long pktcnt;
	unsigned long bytecnt;
	unsigned long wrong_if;
};


struct sioc_mif_req6 {
	mifi_t	mifi;		
	unsigned long icount;	
	unsigned long ocount;	
	unsigned long ibytes;	
	unsigned long obytes;	
};


#ifdef __KERNEL__

#include <linux/pim.h>
#include <linux/skbuff.h>	
#include <net/net_namespace.h>

#ifdef CONFIG_IPV6_MROUTE
static inline int ip6_mroute_opt(int opt)
{
	return (opt >= MRT6_BASE) && (opt <= MRT6_BASE + 10);
}
#else
static inline int ip6_mroute_opt(int opt)
{
	return 0;
}
#endif

struct sock;

#ifdef CONFIG_IPV6_MROUTE
extern int ip6_mroute_setsockopt(struct sock *, int, char __user *, unsigned int);
extern int ip6_mroute_getsockopt(struct sock *, int, char __user *, int __user *);
extern int ip6_mr_input(struct sk_buff *skb);
extern int ip6mr_ioctl(struct sock *sk, int cmd, void __user *arg);
extern int ip6mr_compat_ioctl(struct sock *sk, unsigned int cmd, void __user *arg);
extern int ip6_mr_init(void);
extern void ip6_mr_cleanup(void);
#else
static inline
int ip6_mroute_setsockopt(struct sock *sock,
			  int optname, char __user *optval, unsigned int optlen)
{
	return -ENOPROTOOPT;
}

static inline
int ip6_mroute_getsockopt(struct sock *sock,
			  int optname, char __user *optval, int __user *optlen)
{
	return -ENOPROTOOPT;
}

static inline
int ip6mr_ioctl(struct sock *sk, int cmd, void __user *arg)
{
	return -ENOIOCTLCMD;
}

static inline int ip6_mr_init(void)
{
	return 0;
}

static inline void ip6_mr_cleanup(void)
{
	return;
}
#endif

struct mif_device {
	struct net_device 	*dev;			
	unsigned long	bytes_in,bytes_out;
	unsigned long	pkt_in,pkt_out;		
	unsigned long	rate_limit;		
	unsigned char	threshold;		
	unsigned short	flags;			
	int		link;			
};

#define VIFF_STATIC 0x8000

struct mfc6_cache {
	struct list_head list;
	struct in6_addr mf6c_mcastgrp;			
	struct in6_addr mf6c_origin;			
	mifi_t mf6c_parent;			
	int mfc_flags;				

	union {
		struct {
			unsigned long expires;
			struct sk_buff_head unresolved;	
		} unres;
		struct {
			unsigned long last_assert;
			int minvif;
			int maxvif;
			unsigned long bytes;
			unsigned long pkt;
			unsigned long wrong_if;
			unsigned char ttls[MAXMIFS];	
		} res;
	} mfc_un;
};

#define MFC_STATIC		1
#define MFC_NOTIFY		2

#define MFC6_LINES		64

#define MFC6_HASH(a, g) (((__force u32)(a)->s6_addr32[0] ^ \
			  (__force u32)(a)->s6_addr32[1] ^ \
			  (__force u32)(a)->s6_addr32[2] ^ \
			  (__force u32)(a)->s6_addr32[3] ^ \
			  (__force u32)(g)->s6_addr32[0] ^ \
			  (__force u32)(g)->s6_addr32[1] ^ \
			  (__force u32)(g)->s6_addr32[2] ^ \
			  (__force u32)(g)->s6_addr32[3]) % MFC6_LINES)

#define MFC_ASSERT_THRESH (3*HZ)		

#endif

#ifdef __KERNEL__
struct rtmsg;
extern int ip6mr_get_route(struct net *net, struct sk_buff *skb,
			   struct rtmsg *rtm, int nowait);

#ifdef CONFIG_IPV6_MROUTE
extern struct sock *mroute6_socket(struct net *net, struct sk_buff *skb);
extern int ip6mr_sk_done(struct sock *sk);
#else
static inline struct sock *mroute6_socket(struct net *net, struct sk_buff *skb)
{
	return NULL;
}
static inline int ip6mr_sk_done(struct sock *sk)
{
	return 0;
}
#endif
#endif


struct mrt6msg {
#define MRT6MSG_NOCACHE		1
#define MRT6MSG_WRONGMIF	2
#define MRT6MSG_WHOLEPKT	3		
	__u8		im6_mbz;		
	__u8		im6_msgtype;		
	__u16		im6_mif;		
	__u32		im6_pad;		
	struct in6_addr	im6_src, im6_dst;
};

#endif
