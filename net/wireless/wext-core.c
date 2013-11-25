/*
 * This file implement the Wireless Extensions core API.
 *
 * Authors :	Jean Tourrilhes - HPL - <jt@hpl.hp.com>
 * Copyright (c) 1997-2007 Jean Tourrilhes, All Rights Reserved.
 * Copyright	2009 Johannes Berg <johannes@sipsolutions.net>
 *
 * (As all part of the Linux kernel, this file is GPL)
 */
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>
#include <linux/slab.h>
#include <linux/wireless.h>
#include <linux/uaccess.h>
#include <linux/export.h>
#include <net/cfg80211.h>
#include <net/iw_handler.h>
#include <net/netlink.h>
#include <net/wext.h>
#include <net/net_namespace.h>

typedef int (*wext_ioctl_func)(struct net_device *, struct iwreq *,
			       unsigned int, struct iw_request_info *,
			       iw_handler);


static const struct iw_ioctl_description standard_ioctl[] = {
	[IW_IOCTL_IDX(SIOCSIWCOMMIT)] = {
		.header_type	= IW_HEADER_TYPE_NULL,
	},
	[IW_IOCTL_IDX(SIOCGIWNAME)] = {
		.header_type	= IW_HEADER_TYPE_CHAR,
		.flags		= IW_DESCR_FLAG_DUMP,
	},
	[IW_IOCTL_IDX(SIOCSIWNWID)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
		.flags		= IW_DESCR_FLAG_EVENT,
	},
	[IW_IOCTL_IDX(SIOCGIWNWID)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
		.flags		= IW_DESCR_FLAG_DUMP,
	},
	[IW_IOCTL_IDX(SIOCSIWFREQ)] = {
		.header_type	= IW_HEADER_TYPE_FREQ,
		.flags		= IW_DESCR_FLAG_EVENT,
	},
	[IW_IOCTL_IDX(SIOCGIWFREQ)] = {
		.header_type	= IW_HEADER_TYPE_FREQ,
		.flags		= IW_DESCR_FLAG_DUMP,
	},
	[IW_IOCTL_IDX(SIOCSIWMODE)] = {
		.header_type	= IW_HEADER_TYPE_UINT,
		.flags		= IW_DESCR_FLAG_EVENT,
	},
	[IW_IOCTL_IDX(SIOCGIWMODE)] = {
		.header_type	= IW_HEADER_TYPE_UINT,
		.flags		= IW_DESCR_FLAG_DUMP,
	},
	[IW_IOCTL_IDX(SIOCSIWSENS)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
	},
	[IW_IOCTL_IDX(SIOCGIWSENS)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
	},
	[IW_IOCTL_IDX(SIOCSIWRANGE)] = {
		.header_type	= IW_HEADER_TYPE_NULL,
	},
	[IW_IOCTL_IDX(SIOCGIWRANGE)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= sizeof(struct iw_range),
		.flags		= IW_DESCR_FLAG_DUMP,
	},
	[IW_IOCTL_IDX(SIOCSIWPRIV)] = {
		.header_type	= IW_HEADER_TYPE_NULL,
	},
	[IW_IOCTL_IDX(SIOCGIWPRIV)] = { 
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= sizeof(struct iw_priv_args),
		.max_tokens	= 16,
		.flags		= IW_DESCR_FLAG_NOMAX,
	},
	[IW_IOCTL_IDX(SIOCSIWSTATS)] = {
		.header_type	= IW_HEADER_TYPE_NULL,
	},
	[IW_IOCTL_IDX(SIOCGIWSTATS)] = { 
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= sizeof(struct iw_statistics),
		.flags		= IW_DESCR_FLAG_DUMP,
	},
	[IW_IOCTL_IDX(SIOCSIWSPY)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= sizeof(struct sockaddr),
		.max_tokens	= IW_MAX_SPY,
	},
	[IW_IOCTL_IDX(SIOCGIWSPY)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= sizeof(struct sockaddr) +
				  sizeof(struct iw_quality),
		.max_tokens	= IW_MAX_SPY,
	},
	[IW_IOCTL_IDX(SIOCSIWTHRSPY)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= sizeof(struct iw_thrspy),
		.min_tokens	= 1,
		.max_tokens	= 1,
	},
	[IW_IOCTL_IDX(SIOCGIWTHRSPY)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= sizeof(struct iw_thrspy),
		.min_tokens	= 1,
		.max_tokens	= 1,
	},
	[IW_IOCTL_IDX(SIOCSIWAP)] = {
		.header_type	= IW_HEADER_TYPE_ADDR,
	},
	[IW_IOCTL_IDX(SIOCGIWAP)] = {
		.header_type	= IW_HEADER_TYPE_ADDR,
		.flags		= IW_DESCR_FLAG_DUMP,
	},
	[IW_IOCTL_IDX(SIOCSIWMLME)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.min_tokens	= sizeof(struct iw_mlme),
		.max_tokens	= sizeof(struct iw_mlme),
	},
	[IW_IOCTL_IDX(SIOCGIWAPLIST)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= sizeof(struct sockaddr) +
				  sizeof(struct iw_quality),
		.max_tokens	= IW_MAX_AP,
		.flags		= IW_DESCR_FLAG_NOMAX,
	},
	[IW_IOCTL_IDX(SIOCSIWSCAN)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.min_tokens	= 0,
		.max_tokens	= sizeof(struct iw_scan_req),
	},
	[IW_IOCTL_IDX(SIOCGIWSCAN)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= IW_SCAN_MAX_DATA,
		.flags		= IW_DESCR_FLAG_NOMAX,
	},
	[IW_IOCTL_IDX(SIOCSIWESSID)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= IW_ESSID_MAX_SIZE,
		.flags		= IW_DESCR_FLAG_EVENT,
	},
	[IW_IOCTL_IDX(SIOCGIWESSID)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= IW_ESSID_MAX_SIZE,
		.flags		= IW_DESCR_FLAG_DUMP,
	},
	[IW_IOCTL_IDX(SIOCSIWNICKN)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= IW_ESSID_MAX_SIZE,
	},
	[IW_IOCTL_IDX(SIOCGIWNICKN)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= IW_ESSID_MAX_SIZE,
	},
	[IW_IOCTL_IDX(SIOCSIWRATE)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
	},
	[IW_IOCTL_IDX(SIOCGIWRATE)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
	},
	[IW_IOCTL_IDX(SIOCSIWRTS)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
	},
	[IW_IOCTL_IDX(SIOCGIWRTS)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
	},
	[IW_IOCTL_IDX(SIOCSIWFRAG)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
	},
	[IW_IOCTL_IDX(SIOCGIWFRAG)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
	},
	[IW_IOCTL_IDX(SIOCSIWTXPOW)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
	},
	[IW_IOCTL_IDX(SIOCGIWTXPOW)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
	},
	[IW_IOCTL_IDX(SIOCSIWRETRY)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
	},
	[IW_IOCTL_IDX(SIOCGIWRETRY)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
	},
	[IW_IOCTL_IDX(SIOCSIWENCODE)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= IW_ENCODING_TOKEN_MAX,
		.flags		= IW_DESCR_FLAG_EVENT | IW_DESCR_FLAG_RESTRICT,
	},
	[IW_IOCTL_IDX(SIOCGIWENCODE)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= IW_ENCODING_TOKEN_MAX,
		.flags		= IW_DESCR_FLAG_DUMP | IW_DESCR_FLAG_RESTRICT,
	},
	[IW_IOCTL_IDX(SIOCSIWPOWER)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
	},
	[IW_IOCTL_IDX(SIOCGIWPOWER)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
	},
	[IW_IOCTL_IDX(SIOCSIWGENIE)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= IW_GENERIC_IE_MAX,
	},
	[IW_IOCTL_IDX(SIOCGIWGENIE)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= IW_GENERIC_IE_MAX,
	},
	[IW_IOCTL_IDX(SIOCSIWAUTH)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
	},
	[IW_IOCTL_IDX(SIOCGIWAUTH)] = {
		.header_type	= IW_HEADER_TYPE_PARAM,
	},
	[IW_IOCTL_IDX(SIOCSIWENCODEEXT)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.min_tokens	= sizeof(struct iw_encode_ext),
		.max_tokens	= sizeof(struct iw_encode_ext) +
				  IW_ENCODING_TOKEN_MAX,
	},
	[IW_IOCTL_IDX(SIOCGIWENCODEEXT)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.min_tokens	= sizeof(struct iw_encode_ext),
		.max_tokens	= sizeof(struct iw_encode_ext) +
				  IW_ENCODING_TOKEN_MAX,
	},
	[IW_IOCTL_IDX(SIOCSIWPMKSA)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.min_tokens	= sizeof(struct iw_pmksa),
		.max_tokens	= sizeof(struct iw_pmksa),
	},
};
static const unsigned standard_ioctl_num = ARRAY_SIZE(standard_ioctl);

static const struct iw_ioctl_description standard_event[] = {
	[IW_EVENT_IDX(IWEVTXDROP)] = {
		.header_type	= IW_HEADER_TYPE_ADDR,
	},
	[IW_EVENT_IDX(IWEVQUAL)] = {
		.header_type	= IW_HEADER_TYPE_QUAL,
	},
	[IW_EVENT_IDX(IWEVCUSTOM)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= IW_CUSTOM_MAX,
	},
	[IW_EVENT_IDX(IWEVREGISTERED)] = {
		.header_type	= IW_HEADER_TYPE_ADDR,
	},
	[IW_EVENT_IDX(IWEVEXPIRED)] = {
		.header_type	= IW_HEADER_TYPE_ADDR,
	},
	[IW_EVENT_IDX(IWEVGENIE)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= IW_GENERIC_IE_MAX,
	},
	[IW_EVENT_IDX(IWEVMICHAELMICFAILURE)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= sizeof(struct iw_michaelmicfailure),
	},
	[IW_EVENT_IDX(IWEVASSOCREQIE)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= IW_GENERIC_IE_MAX,
	},
	[IW_EVENT_IDX(IWEVASSOCRESPIE)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= IW_GENERIC_IE_MAX,
	},
	[IW_EVENT_IDX(IWEVPMKIDCAND)] = {
		.header_type	= IW_HEADER_TYPE_POINT,
		.token_size	= 1,
		.max_tokens	= sizeof(struct iw_pmkid_cand),
	},
};
static const unsigned standard_event_num = ARRAY_SIZE(standard_event);

static const int event_type_size[] = {
	IW_EV_LCP_LEN,			
	0,
	IW_EV_CHAR_LEN,			
	0,
	IW_EV_UINT_LEN,			
	IW_EV_FREQ_LEN,			
	IW_EV_ADDR_LEN,			
	0,
	IW_EV_POINT_LEN,		
	IW_EV_PARAM_LEN,		
	IW_EV_QUAL_LEN,			
};

#ifdef CONFIG_COMPAT
static const int compat_event_type_size[] = {
	IW_EV_COMPAT_LCP_LEN,		
	0,
	IW_EV_COMPAT_CHAR_LEN,		
	0,
	IW_EV_COMPAT_UINT_LEN,		
	IW_EV_COMPAT_FREQ_LEN,		
	IW_EV_COMPAT_ADDR_LEN,		
	0,
	IW_EV_COMPAT_POINT_LEN,		
	IW_EV_COMPAT_PARAM_LEN,		
	IW_EV_COMPAT_QUAL_LEN,		
};
#endif



static int __net_init wext_pernet_init(struct net *net)
{
	skb_queue_head_init(&net->wext_nlevents);
	return 0;
}

static void __net_exit wext_pernet_exit(struct net *net)
{
	skb_queue_purge(&net->wext_nlevents);
}

static struct pernet_operations wext_pernet_ops = {
	.init = wext_pernet_init,
	.exit = wext_pernet_exit,
};

static int __init wireless_nlevent_init(void)
{
	return register_pernet_subsys(&wext_pernet_ops);
}

subsys_initcall(wireless_nlevent_init);

static void wireless_nlevent_process(struct work_struct *work)
{
	struct sk_buff *skb;
	struct net *net;

	rtnl_lock();

	for_each_net(net) {
		while ((skb = skb_dequeue(&net->wext_nlevents)))
			rtnl_notify(skb, net, 0, RTNLGRP_LINK, NULL,
				    GFP_KERNEL);
	}

	rtnl_unlock();
}

static DECLARE_WORK(wireless_nlevent_work, wireless_nlevent_process);

static struct nlmsghdr *rtnetlink_ifinfo_prep(struct net_device *dev,
					      struct sk_buff *skb)
{
	struct ifinfomsg *r;
	struct nlmsghdr  *nlh;

	nlh = nlmsg_put(skb, 0, 0, RTM_NEWLINK, sizeof(*r), 0);
	if (!nlh)
		return NULL;

	r = nlmsg_data(nlh);
	r->ifi_family = AF_UNSPEC;
	r->__ifi_pad = 0;
	r->ifi_type = dev->type;
	r->ifi_index = dev->ifindex;
	r->ifi_flags = dev_get_flags(dev);
	r->ifi_change = 0;	

	NLA_PUT_STRING(skb, IFLA_IFNAME, dev->name);

	return nlh;
 nla_put_failure:
	nlmsg_cancel(skb, nlh);
	return NULL;
}


void wireless_send_event(struct net_device *	dev,
			 unsigned int		cmd,
			 union iwreq_data *	wrqu,
			 const char *		extra)
{
	const struct iw_ioctl_description *	descr = NULL;
	int extra_len = 0;
	struct iw_event  *event;		
	int event_len;				
	int hdr_len;				
	int wrqu_off = 0;			
	
	unsigned	cmd_index;		
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	struct nlattr *nla;
#ifdef CONFIG_COMPAT
	struct __compat_iw_event *compat_event;
	struct compat_iw_point compat_wrqu;
	struct sk_buff *compskb;
#endif

	if (WARN_ON(cmd == SIOCGIWSCAN && extra))
		extra = NULL;

	
	if (cmd <= SIOCIWLAST) {
		cmd_index = IW_IOCTL_IDX(cmd);
		if (cmd_index < standard_ioctl_num)
			descr = &(standard_ioctl[cmd_index]);
	} else {
		cmd_index = IW_EVENT_IDX(cmd);
		if (cmd_index < standard_event_num)
			descr = &(standard_event[cmd_index]);
	}
	
	if (descr == NULL) {
		netdev_err(dev, "(WE) : Invalid/Unknown Wireless Event (0x%04X)\n",
			   cmd);
		return;
	}

	
	if (descr->header_type == IW_HEADER_TYPE_POINT) {
		
		if (wrqu->data.length > descr->max_tokens) {
			netdev_err(dev, "(WE) : Wireless Event too big (%d)\n",
				   wrqu->data.length);
			return;
		}
		if (wrqu->data.length < descr->min_tokens) {
			netdev_err(dev, "(WE) : Wireless Event too small (%d)\n",
				   wrqu->data.length);
			return;
		}
		
		if (extra != NULL)
			extra_len = wrqu->data.length * descr->token_size;
		
		wrqu_off = IW_EV_POINT_OFF;
	}

	
	hdr_len = event_type_size[descr->header_type];
	event_len = hdr_len + extra_len;


	skb = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_ATOMIC);
	if (!skb)
		return;

	
	nlh = rtnetlink_ifinfo_prep(dev, skb);
	if (WARN_ON(!nlh)) {
		kfree_skb(skb);
		return;
	}

	
	nla = nla_reserve(skb, IFLA_WIRELESS, event_len);
	if (!nla) {
		kfree_skb(skb);
		return;
	}
	event = nla_data(nla);

	
	memset(event, 0, hdr_len);
	event->len = event_len;
	event->cmd = cmd;
	memcpy(&event->u, ((char *) wrqu) + wrqu_off, hdr_len - IW_EV_LCP_LEN);
	if (extra_len)
		memcpy(((char *) event) + hdr_len, extra, extra_len);

	nlmsg_end(skb, nlh);
#ifdef CONFIG_COMPAT
	hdr_len = compat_event_type_size[descr->header_type];
	event_len = hdr_len + extra_len;

	compskb = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_ATOMIC);
	if (!compskb) {
		kfree_skb(skb);
		return;
	}

	
	nlh = rtnetlink_ifinfo_prep(dev, compskb);
	if (WARN_ON(!nlh)) {
		kfree_skb(skb);
		kfree_skb(compskb);
		return;
	}

	
	nla = nla_reserve(compskb, IFLA_WIRELESS, event_len);
	if (!nla) {
		kfree_skb(skb);
		kfree_skb(compskb);
		return;
	}
	compat_event = nla_data(nla);

	compat_event->len = event_len;
	compat_event->cmd = cmd;
	if (descr->header_type == IW_HEADER_TYPE_POINT) {
		compat_wrqu.length = wrqu->data.length;
		compat_wrqu.flags = wrqu->data.flags;
		memcpy(&compat_event->pointer,
			((char *) &compat_wrqu) + IW_EV_COMPAT_POINT_OFF,
			hdr_len - IW_EV_COMPAT_LCP_LEN);
		if (extra_len)
			memcpy(((char *) compat_event) + hdr_len,
				extra, extra_len);
	} else {
		
		memcpy(&compat_event->pointer, wrqu,
			hdr_len - IW_EV_COMPAT_LCP_LEN);
	}

	nlmsg_end(compskb, nlh);

	skb_shinfo(skb)->frag_list = compskb;
#endif
	skb_queue_tail(&dev_net(dev)->wext_nlevents, skb);
	schedule_work(&wireless_nlevent_work);
}
EXPORT_SYMBOL(wireless_send_event);




struct iw_statistics *get_wireless_stats(struct net_device *dev)
{
#ifdef CONFIG_WIRELESS_EXT
	if ((dev->wireless_handlers != NULL) &&
	   (dev->wireless_handlers->get_wireless_stats != NULL))
		return dev->wireless_handlers->get_wireless_stats(dev);
#endif

#ifdef CONFIG_CFG80211_WEXT
	if (dev->ieee80211_ptr &&
	    dev->ieee80211_ptr->wiphy &&
	    dev->ieee80211_ptr->wiphy->wext &&
	    dev->ieee80211_ptr->wiphy->wext->get_wireless_stats)
		return dev->ieee80211_ptr->wiphy->wext->get_wireless_stats(dev);
#endif

	
	return NULL;
}

static int iw_handler_get_iwstats(struct net_device *		dev,
				  struct iw_request_info *	info,
				  union iwreq_data *		wrqu,
				  char *			extra)
{
	
	struct iw_statistics *stats;

	stats = get_wireless_stats(dev);
	if (stats) {
		
		memcpy(extra, stats, sizeof(struct iw_statistics));
		wrqu->data.length = sizeof(struct iw_statistics);

		
		if (wrqu->data.flags != 0)
			stats->qual.updated &= ~IW_QUAL_ALL_UPDATED;
		return 0;
	} else
		return -EOPNOTSUPP;
}

static iw_handler get_handler(struct net_device *dev, unsigned int cmd)
{
	
	unsigned int	index;		
	const struct iw_handler_def *handlers = NULL;

#ifdef CONFIG_CFG80211_WEXT
	if (dev->ieee80211_ptr && dev->ieee80211_ptr->wiphy)
		handlers = dev->ieee80211_ptr->wiphy->wext;
#endif
#ifdef CONFIG_WIRELESS_EXT
	if (dev->wireless_handlers)
		handlers = dev->wireless_handlers;
#endif

	if (!handlers)
		return NULL;

	
	index = IW_IOCTL_IDX(cmd);
	if (index < handlers->num_standard)
		return handlers->standard[index];

#ifdef CONFIG_WEXT_PRIV
	
	index = cmd - SIOCIWFIRSTPRIV;
	if (index < handlers->num_private)
		return handlers->private[index];
#endif

	
	return NULL;
}

static int ioctl_standard_iw_point(struct iw_point *iwp, unsigned int cmd,
				   const struct iw_ioctl_description *descr,
				   iw_handler handler, struct net_device *dev,
				   struct iw_request_info *info)
{
	int err, extra_size, user_length = 0, essid_compat = 0;
	char *extra;

	extra_size = descr->max_tokens * descr->token_size;

	
	switch (cmd) {
	case SIOCSIWESSID:
	case SIOCGIWESSID:
	case SIOCSIWNICKN:
	case SIOCGIWNICKN:
		if (iwp->length == descr->max_tokens + 1)
			essid_compat = 1;
		else if (IW_IS_SET(cmd) && (iwp->length != 0)) {
			char essid[IW_ESSID_MAX_SIZE + 1];
			unsigned int len;
			len = iwp->length * descr->token_size;

			if (len > IW_ESSID_MAX_SIZE)
				return -EFAULT;

			err = copy_from_user(essid, iwp->pointer, len);
			if (err)
				return -EFAULT;

			if (essid[iwp->length - 1] == '\0')
				essid_compat = 1;
		}
		break;
	default:
		break;
	}

	iwp->length -= essid_compat;

	
	if (IW_IS_SET(cmd)) {
		
		if (!iwp->pointer && iwp->length != 0)
			return -EFAULT;
		
		if (iwp->length > descr->max_tokens)
			return -E2BIG;
		if (iwp->length < descr->min_tokens)
			return -EINVAL;
	} else {
		
		if (!iwp->pointer)
			return -EFAULT;
		
		user_length = iwp->length;


		
		if ((descr->flags & IW_DESCR_FLAG_NOMAX) &&
		    (user_length > descr->max_tokens)) {
			extra_size = user_length * descr->token_size;

		}
	}

	
	extra = kzalloc(extra_size, GFP_KERNEL);
	if (!extra)
		return -ENOMEM;

	
	if (IW_IS_SET(cmd) && (iwp->length != 0)) {
		if (copy_from_user(extra, iwp->pointer,
				   iwp->length *
				   descr->token_size)) {
			err = -EFAULT;
			goto out;
		}

		if (cmd == SIOCSIWENCODEEXT) {
			struct iw_encode_ext *ee = (void *) extra;

			if (iwp->length < sizeof(*ee) + ee->key_len) {
				err = -EFAULT;
				goto out;
			}
		}
	}

	if (IW_IS_GET(cmd) && !(descr->flags & IW_DESCR_FLAG_NOMAX)) {
		iwp->length = descr->max_tokens;
	}

	err = handler(dev, info, (union iwreq_data *) iwp, extra);

	iwp->length += essid_compat;

	
	if (!err && IW_IS_GET(cmd)) {
		
		if (user_length < iwp->length) {
			err = -E2BIG;
			goto out;
		}

		if (copy_to_user(iwp->pointer, extra,
				 iwp->length *
				 descr->token_size)) {
			err = -EFAULT;
			goto out;
		}
	}

	
	if ((descr->flags & IW_DESCR_FLAG_EVENT) &&
	    ((err == 0) || (err == -EIWCOMMIT))) {
		union iwreq_data *data = (union iwreq_data *) iwp;

		if (descr->flags & IW_DESCR_FLAG_RESTRICT)
			wireless_send_event(dev, cmd, data, NULL);
		else
			wireless_send_event(dev, cmd, data, extra);
	}

out:
	kfree(extra);
	return err;
}

int call_commit_handler(struct net_device *dev)
{
#ifdef CONFIG_WIRELESS_EXT
	if ((netif_running(dev)) &&
	   (dev->wireless_handlers->standard[0] != NULL))
		
		return dev->wireless_handlers->standard[0](dev, NULL,
							   NULL, NULL);
	else
		return 0;		
#else
	
	return 0;
#endif
}

static int wireless_process_ioctl(struct net *net, struct ifreq *ifr,
				  unsigned int cmd,
				  struct iw_request_info *info,
				  wext_ioctl_func standard,
				  wext_ioctl_func private)
{
	struct iwreq *iwr = (struct iwreq *) ifr;
	struct net_device *dev;
	iw_handler	handler;


	
	if ((dev = __dev_get_by_name(net, ifr->ifr_name)) == NULL)
		return -ENODEV;

	if (cmd == SIOCGIWSTATS)
		return standard(dev, iwr, cmd, info,
				&iw_handler_get_iwstats);

#ifdef CONFIG_WEXT_PRIV
	if (cmd == SIOCGIWPRIV && dev->wireless_handlers)
		return standard(dev, iwr, cmd, info,
				iw_handler_get_private);
#endif

	
	if (!netif_device_present(dev))
		return -ENODEV;

	
	handler = get_handler(dev, cmd);
	if (handler) {
		
		if (cmd < SIOCIWFIRSTPRIV)
			return standard(dev, iwr, cmd, info, handler);
		else if (private)
			return private(dev, iwr, cmd, info, handler);
	}
	
	if (dev->netdev_ops->ndo_do_ioctl)
		return dev->netdev_ops->ndo_do_ioctl(dev, ifr, cmd);
	return -EOPNOTSUPP;
}

static int wext_permission_check(unsigned int cmd)
{
	if ((IW_IS_SET(cmd) || cmd == SIOCGIWENCODE ||
	     cmd == SIOCGIWENCODEEXT) &&
	    !capable(CAP_NET_ADMIN))
		return -EPERM;

	return 0;
}

static int wext_ioctl_dispatch(struct net *net, struct ifreq *ifr,
			       unsigned int cmd, struct iw_request_info *info,
			       wext_ioctl_func standard,
			       wext_ioctl_func private)
{
	int ret = wext_permission_check(cmd);

	if (ret)
		return ret;

	dev_load(net, ifr->ifr_name);
	rtnl_lock();
	ret = wireless_process_ioctl(net, ifr, cmd, info, standard, private);
	rtnl_unlock();

	return ret;
}

static int ioctl_standard_call(struct net_device *	dev,
			       struct iwreq		*iwr,
			       unsigned int		cmd,
			       struct iw_request_info	*info,
			       iw_handler		handler)
{
	const struct iw_ioctl_description *	descr;
	int					ret = -EINVAL;

	
	if (IW_IOCTL_IDX(cmd) >= standard_ioctl_num)
		return -EOPNOTSUPP;
	descr = &(standard_ioctl[IW_IOCTL_IDX(cmd)]);

	
	if (descr->header_type != IW_HEADER_TYPE_POINT) {

		
		ret = handler(dev, info, &(iwr->u), NULL);

		
		if ((descr->flags & IW_DESCR_FLAG_EVENT) &&
		   ((ret == 0) || (ret == -EIWCOMMIT)))
			wireless_send_event(dev, cmd, &(iwr->u), NULL);
	} else {
		ret = ioctl_standard_iw_point(&iwr->u.data, cmd, descr,
					      handler, dev, info);
	}

	
	if (ret == -EIWCOMMIT)
		ret = call_commit_handler(dev);

	

	return ret;
}


int wext_handle_ioctl(struct net *net, struct ifreq *ifr, unsigned int cmd,
		      void __user *arg)
{
	struct iw_request_info info = { .cmd = cmd, .flags = 0 };
	int ret;

	ret = wext_ioctl_dispatch(net, ifr, cmd, &info,
				  ioctl_standard_call,
				  ioctl_private_call);
	if (ret >= 0 &&
	    IW_IS_GET(cmd) &&
	    copy_to_user(arg, ifr, sizeof(struct iwreq)))
		return -EFAULT;

	return ret;
}

#ifdef CONFIG_COMPAT
static int compat_standard_call(struct net_device	*dev,
				struct iwreq		*iwr,
				unsigned int		cmd,
				struct iw_request_info	*info,
				iw_handler		handler)
{
	const struct iw_ioctl_description *descr;
	struct compat_iw_point *iwp_compat;
	struct iw_point iwp;
	int err;

	descr = standard_ioctl + IW_IOCTL_IDX(cmd);

	if (descr->header_type != IW_HEADER_TYPE_POINT)
		return ioctl_standard_call(dev, iwr, cmd, info, handler);

	iwp_compat = (struct compat_iw_point *) &iwr->u.data;
	iwp.pointer = compat_ptr(iwp_compat->pointer);
	iwp.length = iwp_compat->length;
	iwp.flags = iwp_compat->flags;

	err = ioctl_standard_iw_point(&iwp, cmd, descr, handler, dev, info);

	iwp_compat->pointer = ptr_to_compat(iwp.pointer);
	iwp_compat->length = iwp.length;
	iwp_compat->flags = iwp.flags;

	return err;
}

int compat_wext_handle_ioctl(struct net *net, unsigned int cmd,
			     unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct iw_request_info info;
	struct iwreq iwr;
	char *colon;
	int ret;

	if (copy_from_user(&iwr, argp, sizeof(struct iwreq)))
		return -EFAULT;

	iwr.ifr_name[IFNAMSIZ-1] = 0;
	colon = strchr(iwr.ifr_name, ':');
	if (colon)
		*colon = 0;

	info.cmd = cmd;
	info.flags = IW_REQUEST_FLAG_COMPAT;

	ret = wext_ioctl_dispatch(net, (struct ifreq *) &iwr, cmd, &info,
				  compat_standard_call,
				  compat_private_call);

	if (ret >= 0 &&
	    IW_IS_GET(cmd) &&
	    copy_to_user(argp, &iwr, sizeof(struct iwreq)))
		return -EFAULT;

	return ret;
}
#endif
