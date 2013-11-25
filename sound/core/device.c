/*
 *  Device management routines
 *  Copyright (c) by Jaroslav Kysela <perex@perex.cz>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/slab.h>
#include <linux/time.h>
#include <linux/export.h>
#include <linux/errno.h>
#include <sound/core.h>

int snd_device_new(struct snd_card *card, snd_device_type_t type,
		   void *device_data, struct snd_device_ops *ops)
{
	struct snd_device *dev;

	if (snd_BUG_ON(!card || !device_data || !ops))
		return -ENXIO;
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (dev == NULL) {
		snd_printk(KERN_ERR "Cannot allocate device\n");
		return -ENOMEM;
	}
	dev->card = card;
	dev->type = type;
	dev->state = SNDRV_DEV_BUILD;
	dev->device_data = device_data;
	dev->ops = ops;
	list_add(&dev->list, &card->devices);	
	return 0;
}

EXPORT_SYMBOL(snd_device_new);

int snd_device_free(struct snd_card *card, void *device_data)
{
	struct snd_device *dev;
	
	if (snd_BUG_ON(!card || !device_data))
		return -ENXIO;
	list_for_each_entry(dev, &card->devices, list) {
		if (dev->device_data != device_data)
			continue;
		
		list_del(&dev->list);
		if (dev->state == SNDRV_DEV_REGISTERED &&
		    dev->ops->dev_disconnect)
			if (dev->ops->dev_disconnect(dev))
				snd_printk(KERN_ERR
					   "device disconnect failure\n");
		if (dev->ops->dev_free) {
			if (dev->ops->dev_free(dev))
				snd_printk(KERN_ERR "device free failure\n");
		}
		kfree(dev);
		return 0;
	}
	snd_printd("device free %p (from %pF), not found\n", device_data,
		   __builtin_return_address(0));
	return -ENXIO;
}

EXPORT_SYMBOL(snd_device_free);

int snd_device_disconnect(struct snd_card *card, void *device_data)
{
	struct snd_device *dev;

	if (snd_BUG_ON(!card || !device_data))
		return -ENXIO;
	list_for_each_entry(dev, &card->devices, list) {
		if (dev->device_data != device_data)
			continue;
		if (dev->state == SNDRV_DEV_REGISTERED &&
		    dev->ops->dev_disconnect) {
			if (dev->ops->dev_disconnect(dev))
				snd_printk(KERN_ERR "device disconnect failure\n");
			dev->state = SNDRV_DEV_DISCONNECTED;
		}
		return 0;
	}
	snd_printd("device disconnect %p (from %pF), not found\n", device_data,
		   __builtin_return_address(0));
	return -ENXIO;
}

int snd_device_register(struct snd_card *card, void *device_data)
{
	struct snd_device *dev;
	int err;

	if (snd_BUG_ON(!card || !device_data))
		return -ENXIO;
	list_for_each_entry(dev, &card->devices, list) {
		if (dev->device_data != device_data)
			continue;
		if (dev->state == SNDRV_DEV_BUILD && dev->ops->dev_register) {
			if ((err = dev->ops->dev_register(dev)) < 0)
				return err;
			dev->state = SNDRV_DEV_REGISTERED;
			return 0;
		}
		snd_printd("snd_device_register busy\n");
		return -EBUSY;
	}
	snd_BUG();
	return -ENXIO;
}

EXPORT_SYMBOL(snd_device_register);

int snd_device_register_all(struct snd_card *card)
{
	struct snd_device *dev;
	int err;
	
	if (snd_BUG_ON(!card))
		return -ENXIO;
	list_for_each_entry(dev, &card->devices, list) {
		if (dev->state == SNDRV_DEV_BUILD && dev->ops->dev_register) {
			if ((err = dev->ops->dev_register(dev)) < 0)
				return err;
			dev->state = SNDRV_DEV_REGISTERED;
		}
	}
	return 0;
}

int snd_device_disconnect_all(struct snd_card *card)
{
	struct snd_device *dev;
	int err = 0;

	if (snd_BUG_ON(!card))
		return -ENXIO;
	list_for_each_entry(dev, &card->devices, list) {
		if (snd_device_disconnect(card, dev->device_data) < 0)
			err = -ENXIO;
	}
	return err;
}

int snd_device_free_all(struct snd_card *card, snd_device_cmd_t cmd)
{
	struct snd_device *dev;
	int err;
	unsigned int range_low, range_high, type;

	if (snd_BUG_ON(!card))
		return -ENXIO;
	range_low = (__force unsigned int)cmd * SNDRV_DEV_TYPE_RANGE_SIZE;
	range_high = range_low + SNDRV_DEV_TYPE_RANGE_SIZE - 1;
      __again:
	list_for_each_entry(dev, &card->devices, list) {
		type = (__force unsigned int)dev->type;
		if (type >= range_low && type <= range_high) {
			if ((err = snd_device_free(card, dev->device_data)) < 0)
				return err;
			goto __again;
		}
	}
	return 0;
}
