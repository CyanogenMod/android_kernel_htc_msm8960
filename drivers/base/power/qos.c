/*
 * Devices PM QoS constraints management
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * This module exposes the interface to kernel space for specifying
 * per-device PM QoS dependencies. It provides infrastructure for registration
 * of:
 *
 * Dependents on a QoS value : register requests
 * Watchers of QoS value : get notified when target QoS value changes
 *
 * This QoS design is best effort based. Dependents register their QoS needs.
 * Watchers register to keep track of the current QoS needs of the system.
 * Watchers can register different types of notification callbacks:
 *  . a per-device notification callback using the dev_pm_qos_*_notifier API.
 *    The notification chain data is stored in the per-device constraint
 *    data struct.
 *  . a system-wide notification callback using the dev_pm_qos_*_global_notifier
 *    API. The notification chain data is stored in a static variable.
 *
 * Note about the per-device constraint data struct allocation:
 * . The per-device constraints data struct ptr is tored into the device
 *    dev_pm_info.
 * . To minimize the data usage by the per-device constraints, the data struct
 *   is only allocated at the first call to dev_pm_qos_add_request.
 * . The data is later free'd when the device is removed from the system.
 *  . A global mutex protects the constraints users from the data being
 *     allocated and free'd.
 */

#include <linux/pm_qos.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/export.h>

#include "power.h"

static DEFINE_MUTEX(dev_pm_qos_mtx);

static BLOCKING_NOTIFIER_HEAD(dev_pm_notifiers);

s32 __dev_pm_qos_read_value(struct device *dev)
{
	struct pm_qos_constraints *c = dev->power.constraints;

	return c ? pm_qos_read_value(c) : 0;
}

s32 dev_pm_qos_read_value(struct device *dev)
{
	unsigned long flags;
	s32 ret;

	spin_lock_irqsave(&dev->power.lock, flags);
	ret = __dev_pm_qos_read_value(dev);
	spin_unlock_irqrestore(&dev->power.lock, flags);

	return ret;
}

static int apply_constraint(struct dev_pm_qos_request *req,
			    enum pm_qos_req_action action, int value)
{
	int ret, curr_value;

	ret = pm_qos_update_target(req->dev->power.constraints,
				   &req->node, action, value);

	if (ret) {
		
		curr_value = pm_qos_read_value(req->dev->power.constraints);
		blocking_notifier_call_chain(&dev_pm_notifiers,
					     (unsigned long)curr_value,
					     req);
	}

	return ret;
}

static int dev_pm_qos_constraints_allocate(struct device *dev)
{
	struct pm_qos_constraints *c;
	struct blocking_notifier_head *n;

	c = kzalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	n = kzalloc(sizeof(*n), GFP_KERNEL);
	if (!n) {
		kfree(c);
		return -ENOMEM;
	}
	BLOCKING_INIT_NOTIFIER_HEAD(n);

	plist_head_init(&c->list);
	c->target_value = PM_QOS_DEV_LAT_DEFAULT_VALUE;
	c->default_value = PM_QOS_DEV_LAT_DEFAULT_VALUE;
	c->type = PM_QOS_MIN;
	c->notifiers = n;

	spin_lock_irq(&dev->power.lock);
	dev->power.constraints = c;
	spin_unlock_irq(&dev->power.lock);

	return 0;
}

void dev_pm_qos_constraints_init(struct device *dev)
{
	mutex_lock(&dev_pm_qos_mtx);
	dev->power.constraints = NULL;
	dev->power.power_state = PMSG_ON;
	mutex_unlock(&dev_pm_qos_mtx);
}

void dev_pm_qos_constraints_destroy(struct device *dev)
{
	struct dev_pm_qos_request *req, *tmp;
	struct pm_qos_constraints *c;

	dev_pm_qos_hide_latency_limit(dev);

	mutex_lock(&dev_pm_qos_mtx);

	dev->power.power_state = PMSG_INVALID;
	c = dev->power.constraints;
	if (!c)
		goto out;

	
	plist_for_each_entry_safe(req, tmp, &c->list, node) {
		apply_constraint(req, PM_QOS_REMOVE_REQ, PM_QOS_DEFAULT_VALUE);
		memset(req, 0, sizeof(*req));
	}

	spin_lock_irq(&dev->power.lock);
	dev->power.constraints = NULL;
	spin_unlock_irq(&dev->power.lock);

	kfree(c->notifiers);
	kfree(c);

 out:
	mutex_unlock(&dev_pm_qos_mtx);
}

int dev_pm_qos_add_request(struct device *dev, struct dev_pm_qos_request *req,
			   s32 value)
{
	int ret = 0;

	if (!dev || !req) 
		return -EINVAL;

	if (WARN(dev_pm_qos_request_active(req),
		 "%s() called for already added request\n", __func__))
		return -EINVAL;

	req->dev = dev;

	mutex_lock(&dev_pm_qos_mtx);

	if (!dev->power.constraints) {
		if (dev->power.power_state.event == PM_EVENT_INVALID) {
			
			req->dev = NULL;
			ret = -ENODEV;
			goto out;
		} else {
			ret = dev_pm_qos_constraints_allocate(dev);
		}
	}

	if (!ret)
		ret = apply_constraint(req, PM_QOS_ADD_REQ, value);

 out:
	mutex_unlock(&dev_pm_qos_mtx);

	return ret;
}
EXPORT_SYMBOL_GPL(dev_pm_qos_add_request);

int dev_pm_qos_update_request(struct dev_pm_qos_request *req,
			      s32 new_value)
{
	int ret = 0;

	if (!req) 
		return -EINVAL;

	if (WARN(!dev_pm_qos_request_active(req),
		 "%s() called for unknown object\n", __func__))
		return -EINVAL;

	mutex_lock(&dev_pm_qos_mtx);

	if (req->dev->power.constraints) {
		if (new_value != req->node.prio)
			ret = apply_constraint(req, PM_QOS_UPDATE_REQ,
					       new_value);
	} else {
		
		ret = -ENODEV;
	}

	mutex_unlock(&dev_pm_qos_mtx);
	return ret;
}
EXPORT_SYMBOL_GPL(dev_pm_qos_update_request);

int dev_pm_qos_remove_request(struct dev_pm_qos_request *req)
{
	int ret = 0;

	if (!req) 
		return -EINVAL;

	if (WARN(!dev_pm_qos_request_active(req),
		 "%s() called for unknown object\n", __func__))
		return -EINVAL;

	mutex_lock(&dev_pm_qos_mtx);

	if (req->dev->power.constraints) {
		ret = apply_constraint(req, PM_QOS_REMOVE_REQ,
				       PM_QOS_DEFAULT_VALUE);
		memset(req, 0, sizeof(*req));
	} else {
		
		ret = -ENODEV;
	}

	mutex_unlock(&dev_pm_qos_mtx);
	return ret;
}
EXPORT_SYMBOL_GPL(dev_pm_qos_remove_request);

int dev_pm_qos_add_notifier(struct device *dev, struct notifier_block *notifier)
{
	int retval = 0;

	mutex_lock(&dev_pm_qos_mtx);

	
	if (dev->power.constraints)
		retval = blocking_notifier_chain_register(
				dev->power.constraints->notifiers,
				notifier);

	mutex_unlock(&dev_pm_qos_mtx);
	return retval;
}
EXPORT_SYMBOL_GPL(dev_pm_qos_add_notifier);

int dev_pm_qos_remove_notifier(struct device *dev,
			       struct notifier_block *notifier)
{
	int retval = 0;

	mutex_lock(&dev_pm_qos_mtx);

	
	if (dev->power.constraints)
		retval = blocking_notifier_chain_unregister(
				dev->power.constraints->notifiers,
				notifier);

	mutex_unlock(&dev_pm_qos_mtx);
	return retval;
}
EXPORT_SYMBOL_GPL(dev_pm_qos_remove_notifier);

int dev_pm_qos_add_global_notifier(struct notifier_block *notifier)
{
	return blocking_notifier_chain_register(&dev_pm_notifiers, notifier);
}
EXPORT_SYMBOL_GPL(dev_pm_qos_add_global_notifier);

int dev_pm_qos_remove_global_notifier(struct notifier_block *notifier)
{
	return blocking_notifier_chain_unregister(&dev_pm_notifiers, notifier);
}
EXPORT_SYMBOL_GPL(dev_pm_qos_remove_global_notifier);

int dev_pm_qos_add_ancestor_request(struct device *dev,
				    struct dev_pm_qos_request *req, s32 value)
{
	struct device *ancestor = dev->parent;
	int error = -ENODEV;

	while (ancestor && !ancestor->power.ignore_children)
		ancestor = ancestor->parent;

	if (ancestor)
		error = dev_pm_qos_add_request(ancestor, req, value);

	if (error)
		req->dev = NULL;

	return error;
}
EXPORT_SYMBOL_GPL(dev_pm_qos_add_ancestor_request);

#ifdef CONFIG_PM_RUNTIME
static void __dev_pm_qos_drop_user_request(struct device *dev)
{
	dev_pm_qos_remove_request(dev->power.pq_req);
	dev->power.pq_req = 0;
}

int dev_pm_qos_expose_latency_limit(struct device *dev, s32 value)
{
	struct dev_pm_qos_request *req;
	int ret;

	if (!device_is_registered(dev) || value < 0)
		return -EINVAL;

	if (dev->power.pq_req)
		return -EEXIST;

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	ret = dev_pm_qos_add_request(dev, req, value);
	if (ret < 0)
		return ret;

	dev->power.pq_req = req;
	ret = pm_qos_sysfs_add(dev);
	if (ret)
		__dev_pm_qos_drop_user_request(dev);

	return ret;
}
EXPORT_SYMBOL_GPL(dev_pm_qos_expose_latency_limit);

void dev_pm_qos_hide_latency_limit(struct device *dev)
{
	if (dev->power.pq_req) {
		pm_qos_sysfs_remove(dev);
		__dev_pm_qos_drop_user_request(dev);
	}
}
EXPORT_SYMBOL_GPL(dev_pm_qos_hide_latency_limit);
#endif 
