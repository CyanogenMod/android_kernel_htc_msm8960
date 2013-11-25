/* Header file for IP tables userspace logging, Version 1.8
 *
 * (C) 2000-2002 by Harald Welte <laforge@gnumonks.org>
 * 
 * Distributed under the terms of GNU GPL */

#ifndef _IPT_ULOG_H
#define _IPT_ULOG_H

#ifndef NETLINK_NFLOG
#define NETLINK_NFLOG 	5
#endif

#define ULOG_DEFAULT_NLGROUP	1
#define ULOG_DEFAULT_QTHRESHOLD	1

#define ULOG_MAC_LEN	80
#define ULOG_PREFIX_LEN	32

#define ULOG_MAX_QLEN	50

struct ipt_ulog_info {
	unsigned int nl_group;
	size_t copy_range;
	size_t qthreshold;
	char prefix[ULOG_PREFIX_LEN];
};

typedef struct ulog_packet_msg {
	unsigned long mark;
	long timestamp_sec;
	long timestamp_usec;
	unsigned int hook;
	char indev_name[IFNAMSIZ];
	char outdev_name[IFNAMSIZ];
	size_t data_len;
	char prefix[ULOG_PREFIX_LEN];
	unsigned char mac_len;
	unsigned char mac[ULOG_MAC_LEN];
	unsigned char payload[0];
} ulog_packet_msg_t;

#endif 
