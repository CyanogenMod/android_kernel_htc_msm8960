/*
 * This file implement the Wireless Extensions priv API.
 *
 * Authors :	Jean Tourrilhes - HPL - <jt@hpl.hp.com>
 * Copyright (c) 1997-2007 Jean Tourrilhes, All Rights Reserved.
 * Copyright	2009 Johannes Berg <johannes@sipsolutions.net>
 *
 * (As all part of the Linux kernel, this file is GPL)
 */
#include <linux/slab.h>
#include <linux/wireless.h>
#include <linux/netdevice.h>
#include <net/iw_handler.h>
#include <net/wext.h>

int iw_handler_get_private(struct net_device *		dev,
			   struct iw_request_info *	info,
			   union iwreq_data *		wrqu,
			   char *			extra)
{
	
	if ((dev->wireless_handlers->num_private_args == 0) ||
	   (dev->wireless_handlers->private_args == NULL))
		return -EOPNOTSUPP;

	
	if (wrqu->data.length < dev->wireless_handlers->num_private_args) {
		wrqu->data.length = dev->wireless_handlers->num_private_args;
		return -E2BIG;
	}

	
	wrqu->data.length = dev->wireless_handlers->num_private_args;

	
	memcpy(extra, dev->wireless_handlers->private_args,
	       sizeof(struct iw_priv_args) * wrqu->data.length);

	return 0;
}

static const char iw_priv_type_size[] = {
	0,				
	1,				
	1,				
	0,				
	sizeof(__u32),			
	sizeof(struct iw_freq),		
	sizeof(struct sockaddr),	
	0,				
};

static int get_priv_size(__u16 args)
{
	int	num = args & IW_PRIV_SIZE_MASK;
	int	type = (args & IW_PRIV_TYPE_MASK) >> 12;

	return num * iw_priv_type_size[type];
}

static int adjust_priv_size(__u16 args, struct iw_point *iwp)
{
	int	num = iwp->length;
	int	max = args & IW_PRIV_SIZE_MASK;
	int	type = (args & IW_PRIV_TYPE_MASK) >> 12;

	
	if (max < num)
		num = max;

	return num * iw_priv_type_size[type];
}

static int get_priv_descr_and_size(struct net_device *dev, unsigned int cmd,
				   const struct iw_priv_args **descrp)
{
	const struct iw_priv_args *descr;
	int i, extra_size;

	descr = NULL;
	for (i = 0; i < dev->wireless_handlers->num_private_args; i++) {
		if (cmd == dev->wireless_handlers->private_args[i].cmd) {
			descr = &dev->wireless_handlers->private_args[i];
			break;
		}
	}

	extra_size = 0;
	if (descr) {
		if (IW_IS_SET(cmd)) {
			int	offset = 0;	
			
			if (descr->name[0] == '\0')
				
				offset = sizeof(__u32);

			
			extra_size = get_priv_size(descr->set_args);

			
			if ((descr->set_args & IW_PRIV_SIZE_FIXED) &&
			   ((extra_size + offset) <= IFNAMSIZ))
				extra_size = 0;
		} else {
			
			extra_size = get_priv_size(descr->get_args);

			
			if ((descr->get_args & IW_PRIV_SIZE_FIXED) &&
			   (extra_size <= IFNAMSIZ))
				extra_size = 0;
		}
	}
	*descrp = descr;
	return extra_size;
}

static int ioctl_private_iw_point(struct iw_point *iwp, unsigned int cmd,
				  const struct iw_priv_args *descr,
				  iw_handler handler, struct net_device *dev,
				  struct iw_request_info *info, int extra_size)
{
	char *extra;
	int err;

	
	if (IW_IS_SET(cmd)) {
		if (!iwp->pointer && iwp->length != 0)
			return -EFAULT;

		if (iwp->length > (descr->set_args & IW_PRIV_SIZE_MASK))
			return -E2BIG;
	} else if (!iwp->pointer)
		return -EFAULT;

	extra = kzalloc(extra_size, GFP_KERNEL);
	if (!extra)
		return -ENOMEM;

	
	if (IW_IS_SET(cmd) && (iwp->length != 0)) {
		if (copy_from_user(extra, iwp->pointer, extra_size)) {
			err = -EFAULT;
			goto out;
		}
	}

	
	err = handler(dev, info, (union iwreq_data *) iwp, extra);

	
	if (!err && IW_IS_GET(cmd)) {
		if (!(descr->get_args & IW_PRIV_SIZE_FIXED))
			extra_size = adjust_priv_size(descr->get_args, iwp);

		if (copy_to_user(iwp->pointer, extra, extra_size))
			err =  -EFAULT;
	}

out:
	kfree(extra);
	return err;
}

int ioctl_private_call(struct net_device *dev, struct iwreq *iwr,
		       unsigned int cmd, struct iw_request_info *info,
		       iw_handler handler)
{
	int extra_size = 0, ret = -EINVAL;
	const struct iw_priv_args *descr;

	extra_size = get_priv_descr_and_size(dev, cmd, &descr);

	
	if (extra_size == 0) {
		
		ret = handler(dev, info, &(iwr->u), (char *) &(iwr->u));
	} else {
		ret = ioctl_private_iw_point(&iwr->u.data, cmd, descr,
					     handler, dev, info, extra_size);
	}

	
	if (ret == -EIWCOMMIT)
		ret = call_commit_handler(dev);

	return ret;
}

#ifdef CONFIG_COMPAT
int compat_private_call(struct net_device *dev, struct iwreq *iwr,
			unsigned int cmd, struct iw_request_info *info,
			iw_handler handler)
{
	const struct iw_priv_args *descr;
	int ret, extra_size;

	extra_size = get_priv_descr_and_size(dev, cmd, &descr);

	
	if (extra_size == 0) {
		
		ret = handler(dev, info, &(iwr->u), (char *) &(iwr->u));
	} else {
		struct compat_iw_point *iwp_compat;
		struct iw_point iwp;

		iwp_compat = (struct compat_iw_point *) &iwr->u.data;
		iwp.pointer = compat_ptr(iwp_compat->pointer);
		iwp.length = iwp_compat->length;
		iwp.flags = iwp_compat->flags;

		ret = ioctl_private_iw_point(&iwp, cmd, descr,
					     handler, dev, info, extra_size);

		iwp_compat->pointer = ptr_to_compat(iwp.pointer);
		iwp_compat->length = iwp.length;
		iwp_compat->flags = iwp.flags;
	}

	
	if (ret == -EIWCOMMIT)
		ret = call_commit_handler(dev);

	return ret;
}
#endif
