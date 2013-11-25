#ifndef __LINUX_MROUTE_H
#define __LINUX_MROUTE_H

#include <linux/sockios.h>
#include <linux/types.h>
#ifdef __KERNEL__
#include <linux/in.h>
#endif


#define MRT_BASE	200
#define MRT_INIT	(MRT_BASE)	
#define MRT_DONE	(MRT_BASE+1)	
#define MRT_ADD_VIF	(MRT_BASE+2)	
#define MRT_DEL_VIF	(MRT_BASE+3)	
#define MRT_ADD_MFC	(MRT_BASE+4)	
#define MRT_DEL_MFC	(MRT_BASE+5)	
#define MRT_VERSION	(MRT_BASE+6)	
#define MRT_ASSERT	(MRT_BASE+7)	
#define MRT_PIM		(MRT_BASE+8)	
#define MRT_TABLE	(MRT_BASE+9)	

#define SIOCGETVIFCNT	SIOCPROTOPRIVATE	
#define SIOCGETSGCNT	(SIOCPROTOPRIVATE+1)
#define SIOCGETRPF	(SIOCPROTOPRIVATE+2)

#define MAXVIFS		32	
typedef unsigned long vifbitmap_t;	
typedef unsigned short vifi_t;
#define ALL_VIFS	((vifi_t)(-1))

 
#define VIFM_SET(n,m)	((m)|=(1<<(n)))
#define VIFM_CLR(n,m)	((m)&=~(1<<(n)))
#define VIFM_ISSET(n,m)	((m)&(1<<(n)))
#define VIFM_CLRALL(m)	((m)=0)
#define VIFM_COPY(mfrom,mto)	((mto)=(mfrom))
#define VIFM_SAME(m1,m2)	((m1)==(m2))

 
struct vifctl {
	vifi_t	vifc_vifi;		
	unsigned char vifc_flags;	
	unsigned char vifc_threshold;	
	unsigned int vifc_rate_limit;	
	union {
		struct in_addr vifc_lcl_addr;     
		int            vifc_lcl_ifindex;  
	};
	struct in_addr vifc_rmt_addr;	
};

#define VIFF_TUNNEL		0x1	
#define VIFF_SRCRT		0x2	
#define VIFF_REGISTER		0x4	
#define VIFF_USE_IFINDEX	0x8	

 
struct mfcctl {
	struct in_addr mfcc_origin;		
	struct in_addr mfcc_mcastgrp;		
	vifi_t	mfcc_parent;			
	unsigned char mfcc_ttls[MAXVIFS];	
	unsigned int mfcc_pkt_cnt;		
	unsigned int mfcc_byte_cnt;
	unsigned int mfcc_wrong_if;
	int	     mfcc_expire;
};

 
struct sioc_sg_req {
	struct in_addr src;
	struct in_addr grp;
	unsigned long pktcnt;
	unsigned long bytecnt;
	unsigned long wrong_if;
};


struct sioc_vif_req {
	vifi_t	vifi;		
	unsigned long icount;	
	unsigned long ocount;	
	unsigned long ibytes;	
	unsigned long obytes;	
};

 
struct igmpmsg {
	__u32 unused1,unused2;
	unsigned char im_msgtype;		
	unsigned char im_mbz;			
	unsigned char im_vif;			
	unsigned char unused3;
	struct in_addr im_src,im_dst;
};


#ifdef __KERNEL__
#include <linux/pim.h>
#include <net/sock.h>

#ifdef CONFIG_IP_MROUTE
static inline int ip_mroute_opt(int opt)
{
	return (opt >= MRT_BASE) && (opt <= MRT_BASE + 10);
}
#else
static inline int ip_mroute_opt(int opt)
{
	return 0;
}
#endif

#ifdef CONFIG_IP_MROUTE
extern int ip_mroute_setsockopt(struct sock *, int, char __user *, unsigned int);
extern int ip_mroute_getsockopt(struct sock *, int, char __user *, int __user *);
extern int ipmr_ioctl(struct sock *sk, int cmd, void __user *arg);
extern int ipmr_compat_ioctl(struct sock *sk, unsigned int cmd, void __user *arg);
extern int ip_mr_init(void);
#else
static inline
int ip_mroute_setsockopt(struct sock *sock,
			 int optname, char __user *optval, unsigned int optlen)
{
	return -ENOPROTOOPT;
}

static inline
int ip_mroute_getsockopt(struct sock *sock,
			 int optname, char __user *optval, int __user *optlen)
{
	return -ENOPROTOOPT;
}

static inline
int ipmr_ioctl(struct sock *sk, int cmd, void __user *arg)
{
	return -ENOIOCTLCMD;
}

static inline int ip_mr_init(void)
{
	return 0;
}
#endif

struct vif_device {
	struct net_device 	*dev;			
	unsigned long	bytes_in,bytes_out;
	unsigned long	pkt_in,pkt_out;		
	unsigned long	rate_limit;		
	unsigned char	threshold;		
	unsigned short	flags;			
	__be32		local,remote;		
	int		link;			
};

#define VIFF_STATIC 0x8000

struct mfc_cache {
	struct list_head list;
	__be32 mfc_mcastgrp;			
	__be32 mfc_origin;			
	vifi_t mfc_parent;			
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
			unsigned char ttls[MAXVIFS];	
		} res;
	} mfc_un;
	struct rcu_head	rcu;
};

#define MFC_STATIC		1
#define MFC_NOTIFY		2

#define MFC_LINES		64

#ifdef __BIG_ENDIAN
#define MFC_HASH(a,b)	(((((__force u32)(__be32)a)>>24)^(((__force u32)(__be32)b)>>26))&(MFC_LINES-1))
#else
#define MFC_HASH(a,b)	((((__force u32)(__be32)a)^(((__force u32)(__be32)b)>>2))&(MFC_LINES-1))
#endif		

#endif


#define MFC_ASSERT_THRESH (3*HZ)		


#define IGMPMSG_NOCACHE		1		
#define IGMPMSG_WRONGVIF	2		
#define IGMPMSG_WHOLEPKT	3		

#ifdef __KERNEL__
struct rtmsg;
extern int ipmr_get_route(struct net *net, struct sk_buff *skb,
			  __be32 saddr, __be32 daddr,
			  struct rtmsg *rtm, int nowait);
#endif

#endif
