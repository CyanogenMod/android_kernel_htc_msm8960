/*
 * drivers/gpio/devres.c - managed gpio resources
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This file is based on kernel/irq/devres.c
 *
 * Copyright (c) 2011 John Crispin <blogic@openwrt.org>
 */

#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/gfp.h>

static void devm_gpio_release(struct device *dev, void *res)
{
	unsigned *gpio = res;

	gpio_free(*gpio);
}

static int devm_gpio_match(struct device *dev, void *res, void *data)
{
	unsigned *this = res, *gpio = data;

	return *this == *gpio;
}


int devm_gpio_request(struct device *dev, unsigned gpio, const char *label)
{
	unsigned *dr;
	int rc;

	dr = devres_alloc(devm_gpio_release, sizeof(unsigned), GFP_KERNEL);
	if (!dr)
		return -ENOMEM;

	rc = gpio_request(gpio, label);
	if (rc) {
		devres_free(dr);
		return rc;
	}

	*dr = gpio;
	devres_add(dev, dr);

	return 0;
}
EXPORT_SYMBOL(devm_gpio_request);

void devm_gpio_free(struct device *dev, unsigned int gpio)
{

	WARN_ON(devres_destroy(dev, devm_gpio_release, devm_gpio_match,
		&gpio));
	gpio_free(gpio);
}
EXPORT_SYMBOL(devm_gpio_free);
