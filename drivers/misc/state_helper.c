/* drivers/misc/status_helper.c
 *
 * Copyright (C) 2012 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/mfd/pmic8058.h>
#include <linux/pmic8058-xoadc.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <mach/board.h>


static struct class *status_helper;
static struct device *dev1, *dev2;


struct phone_state {
	struct list_head list;
	const char *name;
	void (*func)(void);
};

static LIST_HEAD(g_phone_state_notifier_list);
static LIST_HEAD(g_psensor_state_notifier_list);

static DEFINE_MUTEX(notify_sem);

static void state_helper_send_notify(const char	*name)
{
	static struct phone_state *link;

	mutex_lock(&notify_sem);
	if (!strcmp(name, "phone_event")) {
		list_for_each_entry(link, &g_phone_state_notifier_list, list) {
			if (link->func != NULL) {\
				printk(KERN_INFO"[State Helper] %s: %s call back\n", __func__, link->name);
					link->func();
			}
		}
	} else if (!strcmp(name, "psensor_release_wakelock_event")) {
		list_for_each_entry(link, &g_psensor_state_notifier_list, list) {
			if (link->func != NULL) {\
				printk(KERN_INFO"[State Helper] %s: %s call back\n", __func__, link->name);
					link->func();
			}
		}
	}
	mutex_unlock(&notify_sem);
}

int state_helper_register_notifier(void (*func)(void), const char	*name)
{
	struct phone_state *link;

	mutex_lock(&notify_sem);
	link = kzalloc(sizeof(*link), GFP_KERNEL);
	if (!link) {
		mutex_unlock(&notify_sem);
		return -ENOMEM;
	}

	link->func = func;
	link->name = name;
	if (!strcmp(name, "phone_event")) {
	list_add(&link->list,
		&g_phone_state_notifier_list);
		printk(KERN_INFO"[State Helper] %s: register by %s\n", __func__, name);
	} else if (!strcmp(name, "psensor_release_wakelock_event")) {
	list_add(&link->list,
		&g_psensor_state_notifier_list);
		printk(KERN_INFO"[State Helper] %s: register by %s\n", __func__, name);
	}
	mutex_unlock(&notify_sem);
	return 0;
}


static ssize_t
phone_event_store(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
	state_helper_send_notify("phone_event");
	return count;
}


static ssize_t
psensor_release_wakelock_event_store(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
	printk(KERN_INFO "[State Helper] %s ++\n", __func__);
	state_helper_send_notify("psensor_release_wakelock_event");
	printk(KERN_INFO "[State Helper] %s --\n", __func__);
	return count;
}


static DEVICE_ATTR(phone_event, 0644, NULL, phone_event_store);
static DEVICE_ATTR(psensor_release_wakelock_event, 0644, NULL, psensor_release_wakelock_event_store);

static int __init status_helper_init(void)
{
	int ret;

	status_helper = class_create(THIS_MODULE, "state_helper");
	if (IS_ERR(status_helper))
		return PTR_ERR(status_helper);

	dev1 = device_create(status_helper, NULL, 0, "%s", "proximity");
	if (IS_ERR(status_helper))
		return PTR_ERR(status_helper);
        dev2 = device_create(status_helper, NULL, 0, "%s", "phone");
        if (IS_ERR(status_helper))
                return PTR_ERR(status_helper);

        ret = device_create_file(dev1, &dev_attr_psensor_release_wakelock_event);
	ret = device_create_file(dev2, &dev_attr_phone_event);
	return 0;

}

static void __exit status_helper_exit(void)
{

}

MODULE_DESCRIPTION("state_helper");
MODULE_LICENSE("GPL");

module_init(status_helper_init);
module_exit(status_helper_exit);

