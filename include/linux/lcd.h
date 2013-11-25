/*
 * LCD Lowlevel Control Abstraction
 *
 * Copyright (C) 2003,2004 Hewlett-Packard Company
 *
 */

#ifndef _LINUX_LCD_H
#define _LINUX_LCD_H

#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/fb.h>


struct lcd_device;
struct fb_info;

struct lcd_properties {
	
	int max_contrast;
};

struct lcd_ops {
	int (*get_power)(struct lcd_device *);
	
	int (*set_power)(struct lcd_device *, int power);
	
	int (*get_contrast)(struct lcd_device *);
	
        int (*set_contrast)(struct lcd_device *, int contrast);
	
	int (*set_mode)(struct lcd_device *, struct fb_videomode *);
	int (*check_fb)(struct lcd_device *, struct fb_info *);
};

struct lcd_device {
	struct lcd_properties props;
	struct mutex ops_lock;
	
	struct lcd_ops *ops;
	
	struct mutex update_lock;
	
	struct notifier_block fb_notif;

	struct device dev;
};

struct lcd_platform_data {
	
	int (*reset)(struct lcd_device *ld);
	int (*power_on)(struct lcd_device *ld, int enable);

	int lcd_enabled;
	unsigned int reset_delay;
	
	unsigned int power_on_delay;
	
	unsigned int power_off_delay;

	
	void *pdata;
};

static inline void lcd_set_power(struct lcd_device *ld, int power)
{
	mutex_lock(&ld->update_lock);
	if (ld->ops && ld->ops->set_power)
		ld->ops->set_power(ld, power);
	mutex_unlock(&ld->update_lock);
}

extern struct lcd_device *lcd_device_register(const char *name,
	struct device *parent, void *devdata, struct lcd_ops *ops);
extern void lcd_device_unregister(struct lcd_device *ld);

#define to_lcd_device(obj) container_of(obj, struct lcd_device, dev)

static inline void * lcd_get_data(struct lcd_device *ld_dev)
{
	return dev_get_drvdata(&ld_dev->dev);
}


#endif
