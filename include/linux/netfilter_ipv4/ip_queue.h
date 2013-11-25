/*
 * This is a module which is used for queueing IPv4 packets and
 * communicating with userspace via netlink.
 *
 * (C) 2000 James Morris, this code is GPL.
 */
#ifndef _IP_QUEUE_H
#define _IP_QUEUE_H

#ifdef __KERNEL__
#ifdef DEBUG_IPQ
#define QDEBUG(x...) printk(KERN_DEBUG ## x)
#else
#define QDEBUG(x...)
#endif  
#else
#include <net/if.h>
#endif	

typedef struct ipq_packet_msg {
	unsigned long packet_id;	
	unsigned long mark;		
	long timestamp_sec;		
	long timestamp_usec;		
	unsigned int hook;		
	char indev_name[IFNAMSIZ];	
	char outdev_name[IFNAMSIZ];	
	__be16 hw_protocol;		
	unsigned short hw_type;		
	unsigned char hw_addrlen;	
	unsigned char hw_addr[8];	
	size_t data_len;		
	unsigned char payload[0];	
} ipq_packet_msg_t;

typedef struct ipq_mode_msg {
	unsigned char value;		
	size_t range;			
} ipq_mode_msg_t;

typedef struct ipq_verdict_msg {
	unsigned int value;		
	unsigned long id;		
	size_t data_len;		
	unsigned char payload[0];	
} ipq_verdict_msg_t;

typedef struct ipq_peer_msg {
	union {
		ipq_verdict_msg_t verdict;
		ipq_mode_msg_t mode;
	} msg;
} ipq_peer_msg_t;

enum {
	IPQ_COPY_NONE,		
	IPQ_COPY_META,		
	IPQ_COPY_PACKET		
};
#define IPQ_COPY_MAX IPQ_COPY_PACKET

#define IPQM_BASE	0x10	
#define IPQM_MODE	(IPQM_BASE + 1)		
#define IPQM_VERDICT	(IPQM_BASE + 2)		 
#define IPQM_PACKET	(IPQM_BASE + 3)		
#define IPQM_MAX	(IPQM_BASE + 4)

#endif 
