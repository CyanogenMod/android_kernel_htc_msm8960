/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/debugfs.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <mach/msm_iomap.h>
#include <mach/htc_ramdump.h>
#include <linux/miscdevice.h>

#define COPY_LENGTH (1024*1024)

struct ramdump_platform_data *pdata;

int pointer;
char * loc_buf;

static int ramdump_open(struct inode *inode, struct file *file)
{
	pr_info("%s:\n", __func__);
	return 0;
}

static int ramdump_release(struct inode *inode, struct file *file)
{
	pr_info("%s:\n", __func__);
	return 0;
}


static int get_block_count(void)
{
	int i;
	long totalblock;
	i = 0;
	totalblock = 0;
	for (i = 0; i < pdata->count; i++)
		totalblock += (pdata->region[i].size / COPY_LENGTH);

	return totalblock;
}

static unsigned long dump_startaddress(void)
{
	unsigned long dump_address = 0;
	unsigned long tmp_address;
	int i, tmp;
	int current_point = pointer;


	if (pointer < get_block_count()) {
		i = 0;
		for (i = 0; i <  pdata->count; i++) {
			tmp_address = pdata->region[i].start + current_point * COPY_LENGTH;

			
			
			if ((tmp_address - pdata->region[i].start) >= pdata->region[i].size) {
				tmp = pdata->region[i].size/COPY_LENGTH;
				current_point -= tmp;
			} else{
				dump_address = tmp_address;
			}
		}
	}

	return dump_address;
}

static ssize_t htc_ramdump_read(struct file *fp, char __user *buf, size_t count, loff_t *pos)
{
	int ret;
	int length;
	void __iomem *start;
	unsigned long dump_startaddr;


	dump_startaddr = dump_startaddress();

	pr_err("  %lx, pointer %d \n", dump_startaddr, pointer);
	if (dump_startaddr == 0) {
		ret = -1;
		return ret;
	}

	start = ioremap(dump_startaddr, COPY_LENGTH);

	memcpy_fromio(loc_buf, start, COPY_LENGTH);
	if (count > COPY_LENGTH)
		length = COPY_LENGTH;
	else
		length = count;

	ret = copy_to_user(buf, loc_buf, length);
	if (ret) {
		pr_err("copy to user fail %d \n", ret);
		goto copy_to_user_error;
	}

	pointer ++;
copy_to_user_error:
	iounmap(start);
	return length;
}

static long ramdump_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int count;
	switch (cmd) {
		case GET_RAMDUMP_LENGTH:
			count = get_block_count();
			pr_err("total block number %d \n", count);
			if (copy_to_user(argp, &count, sizeof(count)))
				return -EFAULT;
		break;
		case JUMP_RAMUDMP_START:
			pointer = 0;
		break;
	default:
		break;

	}
	return 0;
}

static const struct file_operations ramdump_fops = {
	.owner = THIS_MODULE,
	.open = ramdump_open,
	.read = htc_ramdump_read,
	.release = ramdump_release,
	.compat_ioctl = ramdump_ioctl,
	.unlocked_ioctl = ramdump_ioctl,
};


static struct miscdevice ramdump_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ramdump",
	.fops = &ramdump_fops,
};



static int __devinit htc_ramdump_probe(struct platform_device *pdev)
{
	int err, ret;

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		pr_err("%s: pdata get fail\n", __func__);
		return -ENOSYS;
	}

	err = misc_register(&ramdump_device);
	if (err) {
		pr_err("%s: ramdump device register failed\n", __func__);
		ret = -EFAULT;
		goto exit_misc_device_register_failed;
	}

	loc_buf = kzalloc(COPY_LENGTH, GFP_KERNEL);
	if (loc_buf == NULL) {
		pr_err("%s: kzalloc failed\n", __func__);
		ret = -ENOMEM;
		goto exit_kzalloc_fail;
	}
	pointer = 0;
	return 0;

exit_kzalloc_fail:
	misc_deregister(&ramdump_device);
exit_misc_device_register_failed:
	return ret;
}


static struct platform_driver htc_ramdump_driver = {
	.probe		= htc_ramdump_probe,
	.driver		= {
		.name = "htc_ramdump",
		.owner = THIS_MODULE,
	},
};

static int __init htc_ramdump_init(void)
{
	return platform_driver_register(&htc_ramdump_driver);
}

static void __exit htc_ramdump_exit(void)
{
	platform_driver_unregister(&htc_ramdump_driver);
}

module_init(htc_ramdump_init);
module_exit(htc_ramdump_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("htc ramdump driver");
MODULE_VERSION("1.1");
MODULE_ALIAS("platform:htc_ramdump");
