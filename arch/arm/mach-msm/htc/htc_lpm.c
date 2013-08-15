/*
 * Copyright (C) 2013 The CyanogenMod Project
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/sysfs.h>

#include <mach/board_htc.h>

static struct kobject *htc_lpm_kobj;

static ssize_t lpm_mode_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf);
static ssize_t lpm_mode_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count);

static struct kobj_attribute lpm_mode =
		__ATTR(lpm_mode, 0660, lpm_mode_show, lpm_mode_store);

static ssize_t lpm_mode_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	char ret;

	ret = (board_mfg_mode() == MFG_MODE_OFFMODE_CHARGING) ? '1' : '0';
	memcpy(buf, &ret, 1);
	return 1;
}

static ssize_t lpm_mode_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	return -ENOTSUPP;
}

static int __init lpm_mode_init(void)
{
	int ret;

	htc_lpm_kobj = kobject_create_and_add("htc_lpm", NULL);

	ret = sysfs_create_file(htc_lpm_kobj,
			(const struct attribute *) &lpm_mode);
	if (ret) {
		printk(KERN_ERR "Failed to add sysfs %d\n", ret);
		return ret;
	}

	return 0;
}

static void __exit lpm_mode_exit(void)
{
	sysfs_remove_file(htc_lpm_kobj, (const struct attribute *) &lpm_mode);
	kobject_del(htc_lpm_kobj);
}

module_init(lpm_mode_init);
module_exit(lpm_mode_exit);
MODULE_DESCRIPTION("Low power mode attribute for HTC M7");
MODULE_AUTHOR("The CyanogenMod Project");
MODULE_LICENSE("GPL");
