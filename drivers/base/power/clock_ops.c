/*
 * drivers/base/power/clock_ops.c - Generic clock manipulation PM callbacks
 *
 * Copyright (c) 2011 Rafael J. Wysocki <rjw@sisk.pl>, Renesas Electronics Corp.
 *
 * This file is released under the GPLv2.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/pm.h>
#include <linux/pm_clock.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/err.h>

#ifdef CONFIG_PM

enum pce_status {
	PCE_STATUS_NONE = 0,
	PCE_STATUS_ACQUIRED,
	PCE_STATUS_ENABLED,
	PCE_STATUS_ERROR,
};

struct pm_clock_entry {
	struct list_head node;
	char *con_id;
	struct clk *clk;
	enum pce_status status;
};

static void pm_clk_acquire(struct device *dev, struct pm_clock_entry *ce)
{
	ce->clk = clk_get(dev, ce->con_id);
	if (IS_ERR(ce->clk)) {
		ce->status = PCE_STATUS_ERROR;
	} else {
		ce->status = PCE_STATUS_ACQUIRED;
		dev_dbg(dev, "Clock %s managed by runtime PM.\n", ce->con_id);
	}
}

int pm_clk_add(struct device *dev, const char *con_id)
{
	struct pm_subsys_data *psd = dev_to_psd(dev);
	struct pm_clock_entry *ce;

	if (!psd)
		return -EINVAL;

	ce = kzalloc(sizeof(*ce), GFP_KERNEL);
	if (!ce) {
		dev_err(dev, "Not enough memory for clock entry.\n");
		return -ENOMEM;
	}

	if (con_id) {
		ce->con_id = kstrdup(con_id, GFP_KERNEL);
		if (!ce->con_id) {
			dev_err(dev,
				"Not enough memory for clock connection ID.\n");
			kfree(ce);
			return -ENOMEM;
		}
	}

	pm_clk_acquire(dev, ce);

	spin_lock_irq(&psd->lock);
	list_add_tail(&ce->node, &psd->clock_list);
	spin_unlock_irq(&psd->lock);
	return 0;
}

static void __pm_clk_remove(struct pm_clock_entry *ce)
{
	if (!ce)
		return;

	if (ce->status < PCE_STATUS_ERROR) {
		if (ce->status == PCE_STATUS_ENABLED)
			clk_disable(ce->clk);

		if (ce->status >= PCE_STATUS_ACQUIRED)
			clk_put(ce->clk);
	}

	kfree(ce->con_id);
	kfree(ce);
}

void pm_clk_remove(struct device *dev, const char *con_id)
{
	struct pm_subsys_data *psd = dev_to_psd(dev);
	struct pm_clock_entry *ce;

	if (!psd)
		return;

	spin_lock_irq(&psd->lock);

	list_for_each_entry(ce, &psd->clock_list, node) {
		if (!con_id && !ce->con_id)
			goto remove;
		else if (!con_id || !ce->con_id)
			continue;
		else if (!strcmp(con_id, ce->con_id))
			goto remove;
	}

	spin_unlock_irq(&psd->lock);
	return;

 remove:
	list_del(&ce->node);
	spin_unlock_irq(&psd->lock);

	__pm_clk_remove(ce);
}

void pm_clk_init(struct device *dev)
{
	struct pm_subsys_data *psd = dev_to_psd(dev);
	if (psd)
		INIT_LIST_HEAD(&psd->clock_list);
}

int pm_clk_create(struct device *dev)
{
	int ret = dev_pm_get_subsys_data(dev);
	return ret < 0 ? ret : 0;
}

void pm_clk_destroy(struct device *dev)
{
	struct pm_subsys_data *psd = dev_to_psd(dev);
	struct pm_clock_entry *ce, *c;
	struct list_head list;

	if (!psd)
		return;

	INIT_LIST_HEAD(&list);

	spin_lock_irq(&psd->lock);

	list_for_each_entry_safe_reverse(ce, c, &psd->clock_list, node)
		list_move(&ce->node, &list);

	spin_unlock_irq(&psd->lock);

	dev_pm_put_subsys_data(dev);

	list_for_each_entry_safe_reverse(ce, c, &list, node) {
		list_del(&ce->node);
		__pm_clk_remove(ce);
	}
}

#endif 

#ifdef CONFIG_PM_RUNTIME

int pm_clk_suspend(struct device *dev)
{
	struct pm_subsys_data *psd = dev_to_psd(dev);
	struct pm_clock_entry *ce;
	unsigned long flags;

	dev_dbg(dev, "%s()\n", __func__);

	if (!psd)
		return 0;

	spin_lock_irqsave(&psd->lock, flags);

	list_for_each_entry_reverse(ce, &psd->clock_list, node) {
		if (ce->status < PCE_STATUS_ERROR) {
			if (ce->status == PCE_STATUS_ENABLED)
				clk_disable(ce->clk);
			ce->status = PCE_STATUS_ACQUIRED;
		}
	}

	spin_unlock_irqrestore(&psd->lock, flags);

	return 0;
}

int pm_clk_resume(struct device *dev)
{
	struct pm_subsys_data *psd = dev_to_psd(dev);
	struct pm_clock_entry *ce;
	unsigned long flags;

	dev_dbg(dev, "%s()\n", __func__);

	if (!psd)
		return 0;

	spin_lock_irqsave(&psd->lock, flags);

	list_for_each_entry(ce, &psd->clock_list, node) {
		if (ce->status < PCE_STATUS_ERROR) {
			clk_enable(ce->clk);
			ce->status = PCE_STATUS_ENABLED;
		}
	}

	spin_unlock_irqrestore(&psd->lock, flags);

	return 0;
}

static int pm_clk_notify(struct notifier_block *nb,
				 unsigned long action, void *data)
{
	struct pm_clk_notifier_block *clknb;
	struct device *dev = data;
	char **con_id;
	int error;

	dev_dbg(dev, "%s() %ld\n", __func__, action);

	clknb = container_of(nb, struct pm_clk_notifier_block, nb);

	switch (action) {
	case BUS_NOTIFY_ADD_DEVICE:
		if (dev->pm_domain)
			break;

		error = pm_clk_create(dev);
		if (error)
			break;

		dev->pm_domain = clknb->pm_domain;
		if (clknb->con_ids[0]) {
			for (con_id = clknb->con_ids; *con_id; con_id++)
				pm_clk_add(dev, *con_id);
		} else {
			pm_clk_add(dev, NULL);
		}

		break;
	case BUS_NOTIFY_DEL_DEVICE:
		if (dev->pm_domain != clknb->pm_domain)
			break;

		dev->pm_domain = NULL;
		pm_clk_destroy(dev);
		break;
	}

	return 0;
}

#else 

#ifdef CONFIG_PM

int pm_clk_suspend(struct device *dev)
{
	struct pm_subsys_data *psd = dev_to_psd(dev);
	struct pm_clock_entry *ce;
	unsigned long flags;

	dev_dbg(dev, "%s()\n", __func__);

	
	if (!psd || !dev->driver)
		return 0;

	spin_lock_irqsave(&psd->lock, flags);

	list_for_each_entry_reverse(ce, &psd->clock_list, node)
		clk_disable(ce->clk);

	spin_unlock_irqrestore(&psd->lock, flags);

	return 0;
}

int pm_clk_resume(struct device *dev)
{
	struct pm_subsys_data *psd = dev_to_psd(dev);
	struct pm_clock_entry *ce;
	unsigned long flags;

	dev_dbg(dev, "%s()\n", __func__);

	
	if (!psd || !dev->driver)
		return 0;

	spin_lock_irqsave(&psd->lock, flags);

	list_for_each_entry(ce, &psd->clock_list, node)
		clk_enable(ce->clk);

	spin_unlock_irqrestore(&psd->lock, flags);

	return 0;
}

#endif 

static void enable_clock(struct device *dev, const char *con_id)
{
	struct clk *clk;

	clk = clk_get(dev, con_id);
	if (!IS_ERR(clk)) {
		clk_enable(clk);
		clk_put(clk);
		dev_info(dev, "Runtime PM disabled, clock forced on.\n");
	}
}

static void disable_clock(struct device *dev, const char *con_id)
{
	struct clk *clk;

	clk = clk_get(dev, con_id);
	if (!IS_ERR(clk)) {
		clk_disable(clk);
		clk_put(clk);
		dev_info(dev, "Runtime PM disabled, clock forced off.\n");
	}
}

static int pm_clk_notify(struct notifier_block *nb,
				 unsigned long action, void *data)
{
	struct pm_clk_notifier_block *clknb;
	struct device *dev = data;
	char **con_id;

	dev_dbg(dev, "%s() %ld\n", __func__, action);

	clknb = container_of(nb, struct pm_clk_notifier_block, nb);

	switch (action) {
	case BUS_NOTIFY_BIND_DRIVER:
		if (clknb->con_ids[0]) {
			for (con_id = clknb->con_ids; *con_id; con_id++)
				enable_clock(dev, *con_id);
		} else {
			enable_clock(dev, NULL);
		}
		break;
	case BUS_NOTIFY_UNBOUND_DRIVER:
		if (clknb->con_ids[0]) {
			for (con_id = clknb->con_ids; *con_id; con_id++)
				disable_clock(dev, *con_id);
		} else {
			disable_clock(dev, NULL);
		}
		break;
	}

	return 0;
}

#endif 

void pm_clk_add_notifier(struct bus_type *bus,
				 struct pm_clk_notifier_block *clknb)
{
	if (!bus || !clknb)
		return;

	clknb->nb.notifier_call = pm_clk_notify;
	bus_register_notifier(bus, &clknb->nb);
}
