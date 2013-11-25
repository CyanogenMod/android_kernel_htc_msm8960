/*
 * Backlight Lowlevel Control Abstraction
 *
 * Copyright (C) 2003,2004 Hewlett-Packard Company
 *
 */

#ifndef _LINUX_BACKLIGHT_H
#define _LINUX_BACKLIGHT_H

#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/notifier.h>


enum backlight_update_reason {
	BACKLIGHT_UPDATE_HOTKEY,
	BACKLIGHT_UPDATE_SYSFS,
};

enum backlight_type {
	BACKLIGHT_RAW = 1,
	BACKLIGHT_PLATFORM,
	BACKLIGHT_FIRMWARE,
	BACKLIGHT_TYPE_MAX,
};

struct backlight_device;
struct fb_info;

struct backlight_ops {
	unsigned int options;

#define BL_CORE_SUSPENDRESUME	(1 << 0)

	
	int (*update_status)(struct backlight_device *);
	int (*get_brightness)(struct backlight_device *);
	int (*check_fb)(struct backlight_device *, struct fb_info *);
};

struct backlight_properties {
	
	int brightness;
	
	int max_brightness;
	int power;
	
	
	int fb_blank;
	
	enum backlight_type type;
	
	
	unsigned int state;

#define BL_CORE_SUSPENDED	(1 << 0)	
#define BL_CORE_FBBLANK		(1 << 1)	
#define BL_CORE_DRIVER4		(1 << 28)	
#define BL_CORE_DRIVER3		(1 << 29)	
#define BL_CORE_DRIVER2		(1 << 30)	
#define BL_CORE_DRIVER1		(1 << 31)	

};

struct backlight_device {
	
	struct backlight_properties props;

	
	struct mutex update_lock;

	struct mutex ops_lock;
	const struct backlight_ops *ops;

	
	struct notifier_block fb_notif;

	struct device dev;
};

static inline void backlight_update_status(struct backlight_device *bd)
{
	mutex_lock(&bd->update_lock);
	if (bd->ops && bd->ops->update_status)
		bd->ops->update_status(bd);
	mutex_unlock(&bd->update_lock);
}

extern struct backlight_device *backlight_device_register(const char *name,
	struct device *dev, void *devdata, const struct backlight_ops *ops,
	const struct backlight_properties *props);
extern void backlight_device_unregister(struct backlight_device *bd);
extern void backlight_force_update(struct backlight_device *bd,
				   enum backlight_update_reason reason);

#define to_backlight_device(obj) container_of(obj, struct backlight_device, dev)

static inline void * bl_get_data(struct backlight_device *bl_dev)
{
	return dev_get_drvdata(&bl_dev->dev);
}

struct generic_bl_info {
	const char *name;
	int max_intensity;
	int default_intensity;
	int limit_mask;
	void (*set_bl_intensity)(int intensity);
	void (*kick_battery)(void);
};

#endif
