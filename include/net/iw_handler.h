/*
 * This file define the new driver API for Wireless Extensions
 *
 * Version :	8	16.3.07
 *
 * Authors :	Jean Tourrilhes - HPL - <jt@hpl.hp.com>
 * Copyright (c) 2001-2007 Jean Tourrilhes, All Rights Reserved.
 */

#ifndef _IW_HANDLER_H
#define _IW_HANDLER_H




#include <linux/wireless.h>		
#include <linux/if_ether.h>

#define IW_HANDLER_VERSION	8



#define IW_WIRELESS_SPY
#define IW_WIRELESS_THRSPY

#define EIWCOMMIT	EINPROGRESS

#define IW_REQUEST_FLAG_COMPAT	0x0001	

#define IW_HEADER_TYPE_NULL	0	
#define IW_HEADER_TYPE_CHAR	2	
#define IW_HEADER_TYPE_UINT	4	
#define IW_HEADER_TYPE_FREQ	5	
#define IW_HEADER_TYPE_ADDR	6	
#define IW_HEADER_TYPE_POINT	8	
#define IW_HEADER_TYPE_PARAM	9	
#define IW_HEADER_TYPE_QUAL	10	

#define IW_DESCR_FLAG_NONE	0x0000	
#define IW_DESCR_FLAG_DUMP	0x0001	
#define IW_DESCR_FLAG_EVENT	0x0002	
#define IW_DESCR_FLAG_RESTRICT	0x0004	
				
#define IW_DESCR_FLAG_NOMAX	0x0008	
#define IW_DESCR_FLAG_WAIT	0x0100	



struct iw_request_info {
	__u16		cmd;		
	__u16		flags;		
};

struct net_device;

typedef int (*iw_handler)(struct net_device *dev, struct iw_request_info *info,
			  union iwreq_data *wrqu, char *extra);

struct iw_handler_def {

	const iw_handler *	standard;
	__u16			num_standard;

#ifdef CONFIG_WEXT_PRIV
	__u16			num_private;
	
	__u16			num_private_args;
	const iw_handler *	private;

	const struct iw_priv_args *	private_args;
#endif

	struct iw_statistics*	(*get_wireless_stats)(struct net_device *dev);
};


struct iw_ioctl_description {
	__u8	header_type;		
	__u8	token_type;		
	__u16	token_size;		
	__u16	min_tokens;		
	__u16	max_tokens;		
	__u32	flags;			
};



struct iw_spy_data {
	
	int			spy_number;
	u_char			spy_address[IW_MAX_SPY][ETH_ALEN];
	struct iw_quality	spy_stat[IW_MAX_SPY];
	
	struct iw_quality	spy_thr_low;	
	struct iw_quality	spy_thr_high;	
	u_char			spy_thr_under[IW_MAX_SPY];
};

struct libipw_device;
struct iw_public_data {
	
	struct iw_spy_data *		spy_data;
	
	struct libipw_device *		libipw;
};



extern int dev_get_wireless_info(char * buffer, char **start, off_t offset,
				 int length);


extern void wireless_send_event(struct net_device *	dev,
				unsigned int		cmd,
				union iwreq_data *	wrqu,
				const char *		extra);


extern int iw_handler_set_spy(struct net_device *	dev,
			      struct iw_request_info *	info,
			      union iwreq_data *	wrqu,
			      char *			extra);
extern int iw_handler_get_spy(struct net_device *	dev,
			      struct iw_request_info *	info,
			      union iwreq_data *	wrqu,
			      char *			extra);
extern int iw_handler_set_thrspy(struct net_device *	dev,
				 struct iw_request_info *info,
				 union iwreq_data *	wrqu,
				 char *			extra);
extern int iw_handler_get_thrspy(struct net_device *	dev,
				 struct iw_request_info *info,
				 union iwreq_data *	wrqu,
				 char *			extra);
extern void wireless_spy_update(struct net_device *	dev,
				unsigned char *		address,
				struct iw_quality *	wstats);


static inline int iwe_stream_lcp_len(struct iw_request_info *info)
{
#ifdef CONFIG_COMPAT
	if (info->flags & IW_REQUEST_FLAG_COMPAT)
		return IW_EV_COMPAT_LCP_LEN;
#endif
	return IW_EV_LCP_LEN;
}

static inline int iwe_stream_point_len(struct iw_request_info *info)
{
#ifdef CONFIG_COMPAT
	if (info->flags & IW_REQUEST_FLAG_COMPAT)
		return IW_EV_COMPAT_POINT_LEN;
#endif
	return IW_EV_POINT_LEN;
}

static inline int iwe_stream_event_len_adjust(struct iw_request_info *info,
					      int event_len)
{
#ifdef CONFIG_COMPAT
	if (info->flags & IW_REQUEST_FLAG_COMPAT) {
		event_len -= IW_EV_LCP_LEN;
		event_len += IW_EV_COMPAT_LCP_LEN;
	}
#endif

	return event_len;
}

static inline char *
iwe_stream_add_event(struct iw_request_info *info, char *stream, char *ends,
		     struct iw_event *iwe, int event_len)
{
	int lcp_len = iwe_stream_lcp_len(info);

	event_len = iwe_stream_event_len_adjust(info, event_len);

	
	if(likely((stream + event_len) < ends)) {
		iwe->len = event_len;
		
		memcpy(stream, (char *) iwe, IW_EV_LCP_PK_LEN);
		memcpy(stream + lcp_len, &iwe->u,
		       event_len - lcp_len);
		stream += event_len;
	}
	return stream;
}

static inline char *
iwe_stream_add_point(struct iw_request_info *info, char *stream, char *ends,
		     struct iw_event *iwe, char *extra)
{
	int event_len = iwe_stream_point_len(info) + iwe->u.data.length;
	int point_len = iwe_stream_point_len(info);
	int lcp_len   = iwe_stream_lcp_len(info);

	
	if(likely((stream + event_len) < ends)) {
		iwe->len = event_len;
		memcpy(stream, (char *) iwe, IW_EV_LCP_PK_LEN);
		memcpy(stream + lcp_len,
		       ((char *) &iwe->u) + IW_EV_POINT_OFF,
		       IW_EV_POINT_PK_LEN - IW_EV_LCP_PK_LEN);
		memcpy(stream + point_len, extra, iwe->u.data.length);
		stream += event_len;
	}
	return stream;
}

static inline char *
iwe_stream_add_value(struct iw_request_info *info, char *event, char *value,
		     char *ends, struct iw_event *iwe, int event_len)
{
	int lcp_len = iwe_stream_lcp_len(info);

	
	event_len -= IW_EV_LCP_LEN;

	
	if(likely((value + event_len) < ends)) {
		
		memcpy(value, &iwe->u, event_len);
		value += event_len;
		
		iwe->len = value - event;
		memcpy(event, (char *) iwe, lcp_len);
	}
	return value;
}

#endif	
