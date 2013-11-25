/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Definitions of the Internet Protocol.
 *
 * Version:	@(#)in.h	1.0.1	04/21/93
 *
 * Authors:	Original taken from the GNU Project <netinet/in.h> file.
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */
#ifndef _LINUX_IN_H
#define _LINUX_IN_H

#include <linux/types.h>
#include <linux/socket.h>

enum {
  IPPROTO_IP = 0,		
  IPPROTO_ICMP = 1,		
  IPPROTO_IGMP = 2,		
  IPPROTO_IPIP = 4,		
  IPPROTO_TCP = 6,		
  IPPROTO_EGP = 8,		
  IPPROTO_PUP = 12,		
  IPPROTO_UDP = 17,		
  IPPROTO_IDP = 22,		
  IPPROTO_DCCP = 33,		
  IPPROTO_RSVP = 46,		
  IPPROTO_GRE = 47,		

  IPPROTO_IPV6	 = 41,		

  IPPROTO_ESP = 50,            
  IPPROTO_AH = 51,             
  IPPROTO_BEETPH = 94,	       
  IPPROTO_PIM    = 103,		

  IPPROTO_COMP   = 108,                
  IPPROTO_SCTP   = 132,		
  IPPROTO_UDPLITE = 136,	

  IPPROTO_RAW	 = 255,		
  IPPROTO_MAX
};


struct in_addr {
	__be32	s_addr;
};

#define IP_TOS		1
#define IP_TTL		2
#define IP_HDRINCL	3
#define IP_OPTIONS	4
#define IP_ROUTER_ALERT	5
#define IP_RECVOPTS	6
#define IP_RETOPTS	7
#define IP_PKTINFO	8
#define IP_PKTOPTIONS	9
#define IP_MTU_DISCOVER	10
#define IP_RECVERR	11
#define IP_RECVTTL	12
#define	IP_RECVTOS	13
#define IP_MTU		14
#define IP_FREEBIND	15
#define IP_IPSEC_POLICY	16
#define IP_XFRM_POLICY	17
#define IP_PASSSEC	18
#define IP_TRANSPARENT	19

#define IP_RECVRETOPTS	IP_RETOPTS

#define IP_ORIGDSTADDR       20
#define IP_RECVORIGDSTADDR   IP_ORIGDSTADDR

#define IP_MINTTL       21
#define IP_NODEFRAG     22

#define IP_PMTUDISC_DONT		0	
#define IP_PMTUDISC_WANT		1	
#define IP_PMTUDISC_DO			2	
#define IP_PMTUDISC_PROBE		3       

#define IP_MULTICAST_IF			32
#define IP_MULTICAST_TTL 		33
#define IP_MULTICAST_LOOP 		34
#define IP_ADD_MEMBERSHIP		35
#define IP_DROP_MEMBERSHIP		36
#define IP_UNBLOCK_SOURCE		37
#define IP_BLOCK_SOURCE			38
#define IP_ADD_SOURCE_MEMBERSHIP	39
#define IP_DROP_SOURCE_MEMBERSHIP	40
#define IP_MSFILTER			41
#define MCAST_JOIN_GROUP		42
#define MCAST_BLOCK_SOURCE		43
#define MCAST_UNBLOCK_SOURCE		44
#define MCAST_LEAVE_GROUP		45
#define MCAST_JOIN_SOURCE_GROUP		46
#define MCAST_LEAVE_SOURCE_GROUP	47
#define MCAST_MSFILTER			48
#define IP_MULTICAST_ALL		49
#define IP_UNICAST_IF			50

#define MCAST_EXCLUDE	0
#define MCAST_INCLUDE	1

#define IP_DEFAULT_MULTICAST_TTL        1
#define IP_DEFAULT_MULTICAST_LOOP       1


struct ip_mreq  {
	struct in_addr imr_multiaddr;	
	struct in_addr imr_interface;	
};

struct ip_mreqn {
	struct in_addr	imr_multiaddr;		
	struct in_addr	imr_address;		
	int		imr_ifindex;		
};

struct ip_mreq_source {
	__be32		imr_multiaddr;
	__be32		imr_interface;
	__be32		imr_sourceaddr;
};

struct ip_msfilter {
	__be32		imsf_multiaddr;
	__be32		imsf_interface;
	__u32		imsf_fmode;
	__u32		imsf_numsrc;
	__be32		imsf_slist[1];
};

#define IP_MSFILTER_SIZE(numsrc) \
	(sizeof(struct ip_msfilter) - sizeof(__u32) \
	+ (numsrc) * sizeof(__u32))

struct group_req {
	__u32				 gr_interface;	
	struct __kernel_sockaddr_storage gr_group;	
};

struct group_source_req {
	__u32				 gsr_interface;	
	struct __kernel_sockaddr_storage gsr_group;	
	struct __kernel_sockaddr_storage gsr_source;	
};

struct group_filter {
	__u32				 gf_interface;	
	struct __kernel_sockaddr_storage gf_group;	
	__u32				 gf_fmode;	
	__u32				 gf_numsrc;	
	struct __kernel_sockaddr_storage gf_slist[1];	
};

#define GROUP_FILTER_SIZE(numsrc) \
	(sizeof(struct group_filter) - sizeof(struct __kernel_sockaddr_storage) \
	+ (numsrc) * sizeof(struct __kernel_sockaddr_storage))

struct in_pktinfo {
	int		ipi_ifindex;
	struct in_addr	ipi_spec_dst;
	struct in_addr	ipi_addr;
};

#define __SOCK_SIZE__	16		
struct sockaddr_in {
  __kernel_sa_family_t	sin_family;	
  __be16		sin_port;	
  struct in_addr	sin_addr;	

  
  unsigned char		__pad[__SOCK_SIZE__ - sizeof(short int) -
			sizeof(unsigned short int) - sizeof(struct in_addr)];
};
#define sin_zero	__pad		


#define	IN_CLASSA(a)		((((long int) (a)) & 0x80000000) == 0)
#define	IN_CLASSA_NET		0xff000000
#define	IN_CLASSA_NSHIFT	24
#define	IN_CLASSA_HOST		(0xffffffff & ~IN_CLASSA_NET)
#define	IN_CLASSA_MAX		128

#define	IN_CLASSB(a)		((((long int) (a)) & 0xc0000000) == 0x80000000)
#define	IN_CLASSB_NET		0xffff0000
#define	IN_CLASSB_NSHIFT	16
#define	IN_CLASSB_HOST		(0xffffffff & ~IN_CLASSB_NET)
#define	IN_CLASSB_MAX		65536

#define	IN_CLASSC(a)		((((long int) (a)) & 0xe0000000) == 0xc0000000)
#define	IN_CLASSC_NET		0xffffff00
#define	IN_CLASSC_NSHIFT	8
#define	IN_CLASSC_HOST		(0xffffffff & ~IN_CLASSC_NET)

#define	IN_CLASSD(a)		((((long int) (a)) & 0xf0000000) == 0xe0000000)
#define	IN_MULTICAST(a)		IN_CLASSD(a)
#define IN_MULTICAST_NET	0xF0000000

#define	IN_EXPERIMENTAL(a)	((((long int) (a)) & 0xf0000000) == 0xf0000000)
#define	IN_BADCLASS(a)		IN_EXPERIMENTAL((a))

#define	INADDR_ANY		((unsigned long int) 0x00000000)

#define	INADDR_BROADCAST	((unsigned long int) 0xffffffff)

#define	INADDR_NONE		((unsigned long int) 0xffffffff)

#define	IN_LOOPBACKNET		127

#define	INADDR_LOOPBACK		0x7f000001	
#define	IN_LOOPBACK(a)		((((long int) (a)) & 0xff000000) == 0x7f000000)

#define INADDR_UNSPEC_GROUP   	0xe0000000U	
#define INADDR_ALLHOSTS_GROUP 	0xe0000001U	
#define INADDR_ALLRTRS_GROUP    0xe0000002U	
#define INADDR_MAX_LOCAL_GROUP  0xe00000ffU	


#include <asm/byteorder.h> 

#ifdef __KERNEL__

#include <linux/errno.h>

static inline int proto_ports_offset(int proto)
{
	switch (proto) {
	case IPPROTO_TCP:
	case IPPROTO_UDP:
	case IPPROTO_DCCP:
	case IPPROTO_ESP:	
	case IPPROTO_SCTP:
	case IPPROTO_UDPLITE:
		return 0;
	case IPPROTO_AH:	
		return 4;
	default:
		return -EINVAL;
	}
}

static inline bool ipv4_is_loopback(__be32 addr)
{
	return (addr & htonl(0xff000000)) == htonl(0x7f000000);
}

static inline bool ipv4_is_multicast(__be32 addr)
{
	return (addr & htonl(0xf0000000)) == htonl(0xe0000000);
}

static inline bool ipv4_is_local_multicast(__be32 addr)
{
	return (addr & htonl(0xffffff00)) == htonl(0xe0000000);
}

static inline bool ipv4_is_lbcast(__be32 addr)
{
	
	return addr == htonl(INADDR_BROADCAST);
}

static inline bool ipv4_is_zeronet(__be32 addr)
{
	return (addr & htonl(0xff000000)) == htonl(0x00000000);
}


static inline bool ipv4_is_private_10(__be32 addr)
{
	return (addr & htonl(0xff000000)) == htonl(0x0a000000);
}

static inline bool ipv4_is_private_172(__be32 addr)
{
	return (addr & htonl(0xfff00000)) == htonl(0xac100000);
}

static inline bool ipv4_is_private_192(__be32 addr)
{
	return (addr & htonl(0xffff0000)) == htonl(0xc0a80000);
}

static inline bool ipv4_is_linklocal_169(__be32 addr)
{
	return (addr & htonl(0xffff0000)) == htonl(0xa9fe0000);
}

static inline bool ipv4_is_anycast_6to4(__be32 addr)
{
	return (addr & htonl(0xffffff00)) == htonl(0xc0586300);
}

static inline bool ipv4_is_test_192(__be32 addr)
{
	return (addr & htonl(0xffffff00)) == htonl(0xc0000200);
}

static inline bool ipv4_is_test_198(__be32 addr)
{
	return (addr & htonl(0xfffe0000)) == htonl(0xc6120000);
}
#endif

#endif	
