/* rc-main.c - Remote Controller core module
 *
 * Copyright (C) 2009-2010 by Mauro Carvalho Chehab <mchehab@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <media/rc-core.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/module.h>
#include "rc-core-priv.h"

#define IR_TAB_MIN_SIZE	256
#define IR_TAB_MAX_SIZE	8192

#define IR_KEYPRESS_TIMEOUT 250

static LIST_HEAD(rc_map_list);
static DEFINE_SPINLOCK(rc_map_lock);

static struct rc_map_list *seek_rc_map(const char *name)
{
	struct rc_map_list *map = NULL;

	spin_lock(&rc_map_lock);
	list_for_each_entry(map, &rc_map_list, list) {
		if (!strcmp(name, map->map.name)) {
			spin_unlock(&rc_map_lock);
			return map;
		}
	}
	spin_unlock(&rc_map_lock);

	return NULL;
}

struct rc_map *rc_map_get(const char *name)
{

	struct rc_map_list *map;

	map = seek_rc_map(name);
#ifdef MODULE
	if (!map) {
		int rc = request_module(name);
		if (rc < 0) {
			printk(KERN_ERR "Couldn't load IR keymap %s\n", name);
			return NULL;
		}
		msleep(20);	

		map = seek_rc_map(name);
	}
#endif
	if (!map) {
		printk(KERN_ERR "IR keymap %s not found\n", name);
		return NULL;
	}

	printk(KERN_INFO "Registered IR keymap %s\n", map->map.name);

	return &map->map;
}
EXPORT_SYMBOL_GPL(rc_map_get);

int rc_map_register(struct rc_map_list *map)
{
	spin_lock(&rc_map_lock);
	list_add_tail(&map->list, &rc_map_list);
	spin_unlock(&rc_map_lock);
	return 0;
}
EXPORT_SYMBOL_GPL(rc_map_register);

void rc_map_unregister(struct rc_map_list *map)
{
	spin_lock(&rc_map_lock);
	list_del(&map->list);
	spin_unlock(&rc_map_lock);
}
EXPORT_SYMBOL_GPL(rc_map_unregister);


static struct rc_map_table empty[] = {
	{ 0x2a, KEY_COFFEE },
};

static struct rc_map_list empty_map = {
	.map = {
		.scan    = empty,
		.size    = ARRAY_SIZE(empty),
		.rc_type = RC_TYPE_UNKNOWN,	
		.name    = RC_MAP_EMPTY,
	}
};

static int ir_create_table(struct rc_map *rc_map,
			   const char *name, u64 rc_type, size_t size)
{
	rc_map->name = name;
	rc_map->rc_type = rc_type;
	rc_map->alloc = roundup_pow_of_two(size * sizeof(struct rc_map_table));
	rc_map->size = rc_map->alloc / sizeof(struct rc_map_table);
	rc_map->scan = kmalloc(rc_map->alloc, GFP_KERNEL);
	if (!rc_map->scan)
		return -ENOMEM;

	IR_dprintk(1, "Allocated space for %u keycode entries (%u bytes)\n",
		   rc_map->size, rc_map->alloc);
	return 0;
}

static void ir_free_table(struct rc_map *rc_map)
{
	rc_map->size = 0;
	kfree(rc_map->scan);
	rc_map->scan = NULL;
}

static int ir_resize_table(struct rc_map *rc_map, gfp_t gfp_flags)
{
	unsigned int oldalloc = rc_map->alloc;
	unsigned int newalloc = oldalloc;
	struct rc_map_table *oldscan = rc_map->scan;
	struct rc_map_table *newscan;

	if (rc_map->size == rc_map->len) {
		
		if (rc_map->alloc >= IR_TAB_MAX_SIZE)
			return -ENOMEM;

		newalloc *= 2;
		IR_dprintk(1, "Growing table to %u bytes\n", newalloc);
	}

	if ((rc_map->len * 3 < rc_map->size) && (oldalloc > IR_TAB_MIN_SIZE)) {
		
		newalloc /= 2;
		IR_dprintk(1, "Shrinking table to %u bytes\n", newalloc);
	}

	if (newalloc == oldalloc)
		return 0;

	newscan = kmalloc(newalloc, gfp_flags);
	if (!newscan) {
		IR_dprintk(1, "Failed to kmalloc %u bytes\n", newalloc);
		return -ENOMEM;
	}

	memcpy(newscan, rc_map->scan, rc_map->len * sizeof(struct rc_map_table));
	rc_map->scan = newscan;
	rc_map->alloc = newalloc;
	rc_map->size = rc_map->alloc / sizeof(struct rc_map_table);
	kfree(oldscan);
	return 0;
}

static unsigned int ir_update_mapping(struct rc_dev *dev,
				      struct rc_map *rc_map,
				      unsigned int index,
				      unsigned int new_keycode)
{
	int old_keycode = rc_map->scan[index].keycode;
	int i;

	
	if (new_keycode == KEY_RESERVED || new_keycode == KEY_UNKNOWN) {
		IR_dprintk(1, "#%d: Deleting scan 0x%04x\n",
			   index, rc_map->scan[index].scancode);
		rc_map->len--;
		memmove(&rc_map->scan[index], &rc_map->scan[index+ 1],
			(rc_map->len - index) * sizeof(struct rc_map_table));
	} else {
		IR_dprintk(1, "#%d: %s scan 0x%04x with key 0x%04x\n",
			   index,
			   old_keycode == KEY_RESERVED ? "New" : "Replacing",
			   rc_map->scan[index].scancode, new_keycode);
		rc_map->scan[index].keycode = new_keycode;
		__set_bit(new_keycode, dev->input_dev->keybit);
	}

	if (old_keycode != KEY_RESERVED) {
		
		__clear_bit(old_keycode, dev->input_dev->keybit);
		
		for (i = 0; i < rc_map->len; i++) {
			if (rc_map->scan[i].keycode == old_keycode) {
				__set_bit(old_keycode, dev->input_dev->keybit);
				break;
			}
		}

		
		ir_resize_table(rc_map, GFP_ATOMIC);
	}

	return old_keycode;
}

static unsigned int ir_establish_scancode(struct rc_dev *dev,
					  struct rc_map *rc_map,
					  unsigned int scancode,
					  bool resize)
{
	unsigned int i;

	if (dev->scanmask)
		scancode &= dev->scanmask;

	
	for (i = 0; i < rc_map->len; i++) {
		if (rc_map->scan[i].scancode == scancode)
			return i;

		
		if (rc_map->scan[i].scancode >= scancode)
			break;
	}

	
	if (rc_map->size == rc_map->len) {
		if (!resize || ir_resize_table(rc_map, GFP_ATOMIC))
			return -1U;
	}

	
	if (i < rc_map->len)
		memmove(&rc_map->scan[i + 1], &rc_map->scan[i],
			(rc_map->len - i) * sizeof(struct rc_map_table));
	rc_map->scan[i].scancode = scancode;
	rc_map->scan[i].keycode = KEY_RESERVED;
	rc_map->len++;

	return i;
}

static int ir_setkeycode(struct input_dev *idev,
			 const struct input_keymap_entry *ke,
			 unsigned int *old_keycode)
{
	struct rc_dev *rdev = input_get_drvdata(idev);
	struct rc_map *rc_map = &rdev->rc_map;
	unsigned int index;
	unsigned int scancode;
	int retval = 0;
	unsigned long flags;

	spin_lock_irqsave(&rc_map->lock, flags);

	if (ke->flags & INPUT_KEYMAP_BY_INDEX) {
		index = ke->index;
		if (index >= rc_map->len) {
			retval = -EINVAL;
			goto out;
		}
	} else {
		retval = input_scancode_to_scalar(ke, &scancode);
		if (retval)
			goto out;

		index = ir_establish_scancode(rdev, rc_map, scancode, true);
		if (index >= rc_map->len) {
			retval = -ENOMEM;
			goto out;
		}
	}

	*old_keycode = ir_update_mapping(rdev, rc_map, index, ke->keycode);

out:
	spin_unlock_irqrestore(&rc_map->lock, flags);
	return retval;
}

static int ir_setkeytable(struct rc_dev *dev,
			  const struct rc_map *from)
{
	struct rc_map *rc_map = &dev->rc_map;
	unsigned int i, index;
	int rc;

	rc = ir_create_table(rc_map, from->name,
			     from->rc_type, from->size);
	if (rc)
		return rc;

	IR_dprintk(1, "Allocated space for %u keycode entries (%u bytes)\n",
		   rc_map->size, rc_map->alloc);

	for (i = 0; i < from->size; i++) {
		index = ir_establish_scancode(dev, rc_map,
					      from->scan[i].scancode, false);
		if (index >= rc_map->len) {
			rc = -ENOMEM;
			break;
		}

		ir_update_mapping(dev, rc_map, index,
				  from->scan[i].keycode);
	}

	if (rc)
		ir_free_table(rc_map);

	return rc;
}

static unsigned int ir_lookup_by_scancode(const struct rc_map *rc_map,
					  unsigned int scancode)
{
	int start = 0;
	int end = rc_map->len - 1;
	int mid;

	while (start <= end) {
		mid = (start + end) / 2;
		if (rc_map->scan[mid].scancode < scancode)
			start = mid + 1;
		else if (rc_map->scan[mid].scancode > scancode)
			end = mid - 1;
		else
			return mid;
	}

	return -1U;
}

static int ir_getkeycode(struct input_dev *idev,
			 struct input_keymap_entry *ke)
{
	struct rc_dev *rdev = input_get_drvdata(idev);
	struct rc_map *rc_map = &rdev->rc_map;
	struct rc_map_table *entry;
	unsigned long flags;
	unsigned int index;
	unsigned int scancode;
	int retval;

	spin_lock_irqsave(&rc_map->lock, flags);

	if (ke->flags & INPUT_KEYMAP_BY_INDEX) {
		index = ke->index;
	} else {
		retval = input_scancode_to_scalar(ke, &scancode);
		if (retval)
			goto out;

		index = ir_lookup_by_scancode(rc_map, scancode);
	}

	if (index < rc_map->len) {
		entry = &rc_map->scan[index];

		ke->index = index;
		ke->keycode = entry->keycode;
		ke->len = sizeof(entry->scancode);
		memcpy(ke->scancode, &entry->scancode, sizeof(entry->scancode));

	} else if (!(ke->flags & INPUT_KEYMAP_BY_INDEX)) {
		ke->index = index;
		ke->keycode = KEY_RESERVED;
	} else {
		retval = -EINVAL;
		goto out;
	}

	retval = 0;

out:
	spin_unlock_irqrestore(&rc_map->lock, flags);
	return retval;
}

u32 rc_g_keycode_from_table(struct rc_dev *dev, u32 scancode)
{
	struct rc_map *rc_map = &dev->rc_map;
	unsigned int keycode;
	unsigned int index;
	unsigned long flags;

	spin_lock_irqsave(&rc_map->lock, flags);

	index = ir_lookup_by_scancode(rc_map, scancode);
	keycode = index < rc_map->len ?
			rc_map->scan[index].keycode : KEY_RESERVED;

	spin_unlock_irqrestore(&rc_map->lock, flags);

	if (keycode != KEY_RESERVED)
		IR_dprintk(1, "%s: scancode 0x%04x keycode 0x%02x\n",
			   dev->input_name, scancode, keycode);

	return keycode;
}
EXPORT_SYMBOL_GPL(rc_g_keycode_from_table);

static void ir_do_keyup(struct rc_dev *dev, bool sync)
{
	if (!dev->keypressed)
		return;

	IR_dprintk(1, "keyup key 0x%04x\n", dev->last_keycode);
	input_report_key(dev->input_dev, dev->last_keycode, 0);
	if (sync)
		input_sync(dev->input_dev);
	dev->keypressed = false;
}

void rc_keyup(struct rc_dev *dev)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->keylock, flags);
	ir_do_keyup(dev, true);
	spin_unlock_irqrestore(&dev->keylock, flags);
}
EXPORT_SYMBOL_GPL(rc_keyup);

static void ir_timer_keyup(unsigned long cookie)
{
	struct rc_dev *dev = (struct rc_dev *)cookie;
	unsigned long flags;

	spin_lock_irqsave(&dev->keylock, flags);
	if (time_is_before_eq_jiffies(dev->keyup_jiffies))
		ir_do_keyup(dev, true);
	spin_unlock_irqrestore(&dev->keylock, flags);
}

void rc_repeat(struct rc_dev *dev)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->keylock, flags);

	input_event(dev->input_dev, EV_MSC, MSC_SCAN, dev->last_scancode);
	input_sync(dev->input_dev);

	if (!dev->keypressed)
		goto out;

	dev->keyup_jiffies = jiffies + msecs_to_jiffies(IR_KEYPRESS_TIMEOUT);
	mod_timer(&dev->timer_keyup, dev->keyup_jiffies);

out:
	spin_unlock_irqrestore(&dev->keylock, flags);
}
EXPORT_SYMBOL_GPL(rc_repeat);

static void ir_do_keydown(struct rc_dev *dev, int scancode,
			  u32 keycode, u8 toggle)
{
	bool new_event = !dev->keypressed ||
			 dev->last_scancode != scancode ||
			 dev->last_toggle != toggle;

	if (new_event && dev->keypressed)
		ir_do_keyup(dev, false);

	input_event(dev->input_dev, EV_MSC, MSC_SCAN, scancode);

	if (new_event && keycode != KEY_RESERVED) {
		
		dev->keypressed = true;
		dev->last_scancode = scancode;
		dev->last_toggle = toggle;
		dev->last_keycode = keycode;

		IR_dprintk(1, "%s: key down event, "
			   "key 0x%04x, scancode 0x%04x\n",
			   dev->input_name, keycode, scancode);
		input_report_key(dev->input_dev, keycode, 1);
	}

	input_sync(dev->input_dev);
}

void rc_keydown(struct rc_dev *dev, int scancode, u8 toggle)
{
	unsigned long flags;
	u32 keycode = rc_g_keycode_from_table(dev, scancode);

	spin_lock_irqsave(&dev->keylock, flags);
	ir_do_keydown(dev, scancode, keycode, toggle);

	if (dev->keypressed) {
		dev->keyup_jiffies = jiffies + msecs_to_jiffies(IR_KEYPRESS_TIMEOUT);
		mod_timer(&dev->timer_keyup, dev->keyup_jiffies);
	}
	spin_unlock_irqrestore(&dev->keylock, flags);
}
EXPORT_SYMBOL_GPL(rc_keydown);

void rc_keydown_notimeout(struct rc_dev *dev, int scancode, u8 toggle)
{
	unsigned long flags;
	u32 keycode = rc_g_keycode_from_table(dev, scancode);

	spin_lock_irqsave(&dev->keylock, flags);
	ir_do_keydown(dev, scancode, keycode, toggle);
	spin_unlock_irqrestore(&dev->keylock, flags);
}
EXPORT_SYMBOL_GPL(rc_keydown_notimeout);

static int ir_open(struct input_dev *idev)
{
	struct rc_dev *rdev = input_get_drvdata(idev);

	return rdev->open(rdev);
}

static void ir_close(struct input_dev *idev)
{
	struct rc_dev *rdev = input_get_drvdata(idev);

	 if (rdev)
		rdev->close(rdev);
}

static char *ir_devnode(struct device *dev, umode_t *mode)
{
	return kasprintf(GFP_KERNEL, "rc/%s", dev_name(dev));
}

static struct class ir_input_class = {
	.name		= "rc",
	.devnode	= ir_devnode,
};

static struct {
	u64	type;
	char	*name;
} proto_names[] = {
	{ RC_TYPE_UNKNOWN,	"unknown"	},
	{ RC_TYPE_RC5,		"rc-5"		},
	{ RC_TYPE_NEC,		"nec"		},
	{ RC_TYPE_RC6,		"rc-6"		},
	{ RC_TYPE_JVC,		"jvc"		},
	{ RC_TYPE_SONY,		"sony"		},
	{ RC_TYPE_RC5_SZ,	"rc-5-sz"	},
	{ RC_TYPE_SANYO,	"sanyo"		},
	{ RC_TYPE_MCE_KBD,	"mce_kbd"	},
	{ RC_TYPE_LIRC,		"lirc"		},
	{ RC_TYPE_OTHER,	"other"		},
};

#define PROTO_NONE	"none"

static ssize_t show_protocols(struct device *device,
			      struct device_attribute *mattr, char *buf)
{
	struct rc_dev *dev = to_rc_dev(device);
	u64 allowed, enabled;
	char *tmp = buf;
	int i;

	
	if (!dev)
		return -EINVAL;

	mutex_lock(&dev->lock);

	if (dev->driver_type == RC_DRIVER_SCANCODE) {
		enabled = dev->rc_map.rc_type;
		allowed = dev->allowed_protos;
	} else {
		enabled = dev->raw->enabled_protocols;
		allowed = ir_raw_get_allowed_protocols();
	}

	IR_dprintk(1, "allowed - 0x%llx, enabled - 0x%llx\n",
		   (long long)allowed,
		   (long long)enabled);

	for (i = 0; i < ARRAY_SIZE(proto_names); i++) {
		if (allowed & enabled & proto_names[i].type)
			tmp += sprintf(tmp, "[%s] ", proto_names[i].name);
		else if (allowed & proto_names[i].type)
			tmp += sprintf(tmp, "%s ", proto_names[i].name);
	}

	if (tmp != buf)
		tmp--;
	*tmp = '\n';

	mutex_unlock(&dev->lock);

	return tmp + 1 - buf;
}

static ssize_t store_protocols(struct device *device,
			       struct device_attribute *mattr,
			       const char *data,
			       size_t len)
{
	struct rc_dev *dev = to_rc_dev(device);
	bool enable, disable;
	const char *tmp;
	u64 type;
	u64 mask;
	int rc, i, count = 0;
	unsigned long flags;
	ssize_t ret;

	
	if (!dev)
		return -EINVAL;

	mutex_lock(&dev->lock);

	if (dev->driver_type == RC_DRIVER_SCANCODE)
		type = dev->rc_map.rc_type;
	else if (dev->raw)
		type = dev->raw->enabled_protocols;
	else {
		IR_dprintk(1, "Protocol switching not supported\n");
		ret = -EINVAL;
		goto out;
	}

	while ((tmp = strsep((char **) &data, " \n")) != NULL) {
		if (!*tmp)
			break;

		if (*tmp == '+') {
			enable = true;
			disable = false;
			tmp++;
		} else if (*tmp == '-') {
			enable = false;
			disable = true;
			tmp++;
		} else {
			enable = false;
			disable = false;
		}

		if (!enable && !disable && !strncasecmp(tmp, PROTO_NONE, sizeof(PROTO_NONE))) {
			tmp += sizeof(PROTO_NONE);
			mask = 0;
			count++;
		} else {
			for (i = 0; i < ARRAY_SIZE(proto_names); i++) {
				if (!strcasecmp(tmp, proto_names[i].name)) {
					tmp += strlen(proto_names[i].name);
					mask = proto_names[i].type;
					break;
				}
			}
			if (i == ARRAY_SIZE(proto_names)) {
				IR_dprintk(1, "Unknown protocol: '%s'\n", tmp);
				ret = -EINVAL;
				goto out;
			}
			count++;
		}

		if (enable)
			type |= mask;
		else if (disable)
			type &= ~mask;
		else
			type = mask;
	}

	if (!count) {
		IR_dprintk(1, "Protocol not specified\n");
		ret = -EINVAL;
		goto out;
	}

	if (dev->change_protocol) {
		rc = dev->change_protocol(dev, type);
		if (rc < 0) {
			IR_dprintk(1, "Error setting protocols to 0x%llx\n",
				   (long long)type);
			ret = -EINVAL;
			goto out;
		}
	}

	if (dev->driver_type == RC_DRIVER_SCANCODE) {
		spin_lock_irqsave(&dev->rc_map.lock, flags);
		dev->rc_map.rc_type = type;
		spin_unlock_irqrestore(&dev->rc_map.lock, flags);
	} else {
		dev->raw->enabled_protocols = type;
	}

	IR_dprintk(1, "Current protocol(s): 0x%llx\n",
		   (long long)type);

	ret = len;

out:
	mutex_unlock(&dev->lock);
	return ret;
}

static void rc_dev_release(struct device *device)
{
}

#define ADD_HOTPLUG_VAR(fmt, val...)					\
	do {								\
		int err = add_uevent_var(env, fmt, val);		\
		if (err)						\
			return err;					\
	} while (0)

static int rc_dev_uevent(struct device *device, struct kobj_uevent_env *env)
{
	struct rc_dev *dev = to_rc_dev(device);

	if (!dev || !dev->input_dev)
		return -ENODEV;

	if (dev->rc_map.name)
		ADD_HOTPLUG_VAR("NAME=%s", dev->rc_map.name);
	if (dev->driver_name)
		ADD_HOTPLUG_VAR("DRV_NAME=%s", dev->driver_name);

	return 0;
}

static DEVICE_ATTR(protocols, S_IRUGO | S_IWUSR,
		   show_protocols, store_protocols);

static struct attribute *rc_dev_attrs[] = {
	&dev_attr_protocols.attr,
	NULL,
};

static struct attribute_group rc_dev_attr_grp = {
	.attrs	= rc_dev_attrs,
};

static const struct attribute_group *rc_dev_attr_groups[] = {
	&rc_dev_attr_grp,
	NULL
};

static struct device_type rc_dev_type = {
	.groups		= rc_dev_attr_groups,
	.release	= rc_dev_release,
	.uevent		= rc_dev_uevent,
};

struct rc_dev *rc_allocate_device(void)
{
	struct rc_dev *dev;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return NULL;

	dev->input_dev = input_allocate_device();
	if (!dev->input_dev) {
		kfree(dev);
		return NULL;
	}

	dev->input_dev->getkeycode = ir_getkeycode;
	dev->input_dev->setkeycode = ir_setkeycode;
	input_set_drvdata(dev->input_dev, dev);

	spin_lock_init(&dev->rc_map.lock);
	spin_lock_init(&dev->keylock);
	mutex_init(&dev->lock);
	setup_timer(&dev->timer_keyup, ir_timer_keyup, (unsigned long)dev);

	dev->dev.type = &rc_dev_type;
	dev->dev.class = &ir_input_class;
	device_initialize(&dev->dev);

	__module_get(THIS_MODULE);
	return dev;
}
EXPORT_SYMBOL_GPL(rc_allocate_device);

void rc_free_device(struct rc_dev *dev)
{
	if (!dev)
		return;

	if (dev->input_dev)
		input_free_device(dev->input_dev);

	put_device(&dev->dev);

	kfree(dev);
	module_put(THIS_MODULE);
}
EXPORT_SYMBOL_GPL(rc_free_device);

int rc_register_device(struct rc_dev *dev)
{
	static bool raw_init = false; 
	static atomic_t devno = ATOMIC_INIT(0);
	struct rc_map *rc_map;
	const char *path;
	int rc;

	if (!dev || !dev->map_name)
		return -EINVAL;

	rc_map = rc_map_get(dev->map_name);
	if (!rc_map)
		rc_map = rc_map_get(RC_MAP_EMPTY);
	if (!rc_map || !rc_map->scan || rc_map->size == 0)
		return -EINVAL;

	set_bit(EV_KEY, dev->input_dev->evbit);
	set_bit(EV_REP, dev->input_dev->evbit);
	set_bit(EV_MSC, dev->input_dev->evbit);
	set_bit(MSC_SCAN, dev->input_dev->mscbit);
	if (dev->open)
		dev->input_dev->open = ir_open;
	if (dev->close)
		dev->input_dev->close = ir_close;

	mutex_lock(&dev->lock);

	dev->devno = (unsigned long)(atomic_inc_return(&devno) - 1);
	dev_set_name(&dev->dev, "rc%ld", dev->devno);
	dev_set_drvdata(&dev->dev, dev);
	rc = device_add(&dev->dev);
	if (rc)
		goto out_unlock;

	rc = ir_setkeytable(dev, rc_map);
	if (rc)
		goto out_dev;

	dev->input_dev->dev.parent = &dev->dev;
	memcpy(&dev->input_dev->id, &dev->input_id, sizeof(dev->input_id));
	dev->input_dev->phys = dev->input_phys;
	dev->input_dev->name = dev->input_name;
	rc = input_register_device(dev->input_dev);
	if (rc)
		goto out_table;

	dev->input_dev->rep[REP_DELAY] = 500;

	dev->input_dev->rep[REP_PERIOD] = 125;

	path = kobject_get_path(&dev->dev.kobj, GFP_KERNEL);
	printk(KERN_INFO "%s: %s as %s\n",
		dev_name(&dev->dev),
		dev->input_name ? dev->input_name : "Unspecified device",
		path ? path : "N/A");
	kfree(path);

	if (dev->driver_type == RC_DRIVER_IR_RAW) {
		
		if (!raw_init) {
			IR_dprintk(1, "Loading raw decoders\n");
			ir_raw_init();
			raw_init = true;
		}
		rc = ir_raw_event_register(dev);
		if (rc < 0)
			goto out_input;
	}

	if (dev->change_protocol) {
		rc = dev->change_protocol(dev, rc_map->rc_type);
		if (rc < 0)
			goto out_raw;
	}

	mutex_unlock(&dev->lock);

	IR_dprintk(1, "Registered rc%ld (driver: %s, remote: %s, mode %s)\n",
		   dev->devno,
		   dev->driver_name ? dev->driver_name : "unknown",
		   rc_map->name ? rc_map->name : "unknown",
		   dev->driver_type == RC_DRIVER_IR_RAW ? "raw" : "cooked");

	return 0;

out_raw:
	if (dev->driver_type == RC_DRIVER_IR_RAW)
		ir_raw_event_unregister(dev);
out_input:
	input_unregister_device(dev->input_dev);
	dev->input_dev = NULL;
out_table:
	ir_free_table(&dev->rc_map);
out_dev:
	device_del(&dev->dev);
out_unlock:
	mutex_unlock(&dev->lock);
	return rc;
}
EXPORT_SYMBOL_GPL(rc_register_device);

void rc_unregister_device(struct rc_dev *dev)
{
	if (!dev)
		return;

	del_timer_sync(&dev->timer_keyup);

	if (dev->driver_type == RC_DRIVER_IR_RAW)
		ir_raw_event_unregister(dev);

	
	ir_free_table(&dev->rc_map);
	IR_dprintk(1, "Freed keycode table\n");

	input_unregister_device(dev->input_dev);
	dev->input_dev = NULL;

	device_del(&dev->dev);

	rc_free_device(dev);
}

EXPORT_SYMBOL_GPL(rc_unregister_device);


static int __init rc_core_init(void)
{
	int rc = class_register(&ir_input_class);
	if (rc) {
		printk(KERN_ERR "rc_core: unable to register rc class\n");
		return rc;
	}

	rc_map_register(&empty_map);

	return 0;
}

static void __exit rc_core_exit(void)
{
	class_unregister(&ir_input_class);
	rc_map_unregister(&empty_map);
}

module_init(rc_core_init);
module_exit(rc_core_exit);

int rc_core_debug;    
EXPORT_SYMBOL_GPL(rc_core_debug);
module_param_named(debug, rc_core_debug, int, 0644);

MODULE_AUTHOR("Mauro Carvalho Chehab <mchehab@redhat.com>");
MODULE_LICENSE("GPL");
