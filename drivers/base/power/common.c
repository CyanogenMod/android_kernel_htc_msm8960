/*
 * drivers/base/power/common.c - Common device power management code.
 *
 * Copyright (C) 2011 Rafael J. Wysocki <rjw@sisk.pl>, Renesas Electronics Corp.
 *
 * This file is released under the GPLv2.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/export.h>
#include <linux/slab.h>
#include <linux/pm_clock.h>

int dev_pm_get_subsys_data(struct device *dev)
{
	struct pm_subsys_data *psd;
	int ret = 0;

	psd = kzalloc(sizeof(*psd), GFP_KERNEL);
	if (!psd)
		return -ENOMEM;

	spin_lock_irq(&dev->power.lock);

	if (dev->power.subsys_data) {
		dev->power.subsys_data->refcount++;
	} else {
		spin_lock_init(&psd->lock);
		psd->refcount = 1;
		dev->power.subsys_data = psd;
		pm_clk_init(dev);
		psd = NULL;
		ret = 1;
	}

	spin_unlock_irq(&dev->power.lock);

	
	kfree(psd);

	return ret;
}
EXPORT_SYMBOL_GPL(dev_pm_get_subsys_data);

int dev_pm_put_subsys_data(struct device *dev)
{
	struct pm_subsys_data *psd;
	int ret = 0;

	spin_lock_irq(&dev->power.lock);

	psd = dev_to_psd(dev);
	if (!psd) {
		ret = -EINVAL;
		goto out;
	}

	if (--psd->refcount == 0) {
		dev->power.subsys_data = NULL;
		kfree(psd);
		ret = 1;
	}

 out:
	spin_unlock_irq(&dev->power.lock);

	return ret;
}
EXPORT_SYMBOL_GPL(dev_pm_put_subsys_data);
