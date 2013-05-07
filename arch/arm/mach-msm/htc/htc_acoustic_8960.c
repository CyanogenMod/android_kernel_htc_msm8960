/* arch/arm/mach-msm/htc_acoustic_8960.c
 *
 * Copyright (C) 2012 HTC Corporation
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

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/mm.h>
#include <linux/gfp.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <mach/htc_acoustic_8960.h>

#define ACOUSTIC_IOCTL_MAGIC 'p'
#define ACOUSTIC_SET_Q6_EFFECT		_IOW(ACOUSTIC_IOCTL_MAGIC, 43, unsigned)
#define ACOUSTIC_GET_HTC_REVISION	_IOW(ACOUSTIC_IOCTL_MAGIC, 44, unsigned)
#define ACOUSTIC_GET_HW_COMPONENT	_IOW(ACOUSTIC_IOCTL_MAGIC, 45, unsigned)
#define ACOUSTIC_GET_DMIC_INFO   	_IOW(ACOUSTIC_IOCTL_MAGIC, 46, unsigned)
#define ACOUSTIC_UPDATE_BEATS_STATUS	_IOW(ACOUSTIC_IOCTL_MAGIC, 47, unsigned)
#define ACOUSTIC_RAMDUMP		_IOW(ACOUSTIC_IOCTL_MAGIC, 99, unsigned)
#define D(fmt, args...) printk(KERN_INFO "[AUD] htc-acoustic: "fmt, ##args)
#define E(fmt, args...) printk(KERN_ERR "[AUD] htc-acoustic: "fmt, ##args)

static struct mutex api_lock;
static struct acoustic_ops default_acoustic_ops;
static struct acoustic_ops *the_ops = &default_acoustic_ops;
static struct switch_dev sdev_beats;

void acoustic_register_ops(struct acoustic_ops *ops)
{
        D("acoustic_register_ops \n");
	the_ops = ops;
}

static int acoustic_open(struct inode *inode, struct file *file)
{
	D("open\n");
	return 0;
}

static int acoustic_release(struct inode *inode, struct file *file)
{
	D("release\n");
	return 0;
}

static long
acoustic_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = 0;
	int hw_rev = 0;
	int mode = -1;
	mutex_lock(&api_lock);
	switch (cmd) {
	case ACOUSTIC_SET_Q6_EFFECT: {
		if (copy_from_user(&mode, (void *)arg, sizeof(mode))) {
			rc = -EFAULT;
			break;
		}
		D("Set Q6 Effect : %d\n", mode);
		if (mode < -1 || mode > 1) {
			E("unsupported Q6 mode: %d\n", mode);
			rc = -EINVAL;
			break;
		}
		if (the_ops->set_q6_effect)
			the_ops->set_q6_effect(mode);
		break;
	}
	case ACOUSTIC_GET_HTC_REVISION:
		if (the_ops->get_htc_revision)
			hw_rev = the_ops->get_htc_revision();
		else
			hw_rev = 1;

		D("Audio HW revision:  %u\n", hw_rev);
		if(copy_to_user((void *)arg, &hw_rev, sizeof(hw_rev))) {
			E("acoustic_ioctl: copy to user failed\n");
			rc = -EINVAL;
		}
		break;
	case ACOUSTIC_GET_HW_COMPONENT:
		if (the_ops->get_hw_component)
			rc = the_ops->get_hw_component();

		D("support components: 0x%x\n", rc);
		if(copy_to_user((void *)arg, &rc, sizeof(rc))) {
			E("acoustic_ioctl: copy to user failed\n");
			rc = -EINVAL;
		}
		break;
       case ACOUSTIC_GET_DMIC_INFO:
		if (the_ops->enable_digital_mic)
			rc = the_ops->enable_digital_mic();

		D("support components: 0x%x\n", rc);
		if(copy_to_user((void *)arg, &rc, sizeof(rc))) {
			E("acoustic_ioctl: copy to user failed\n");
			rc = -EINVAL;
		}
                break;
	case ACOUSTIC_UPDATE_BEATS_STATUS: {
		int new_state = -1;

		if (copy_from_user(&new_state, (void *)arg, sizeof(new_state))) {
			rc = -EFAULT;
			break;
		}
		D("Update Beats Status : %d\n", new_state);
		if (new_state < -1 || new_state > 1) {
			E("Invalid Beats status update");
			rc = -EINVAL;
			break;
		}

		sdev_beats.state = -1;
		switch_set_state(&sdev_beats, new_state);
		break;
	}
       case ACOUSTIC_RAMDUMP:
		pr_err("trigger ramdump by user space\n");
		if (copy_from_user(&mode, (void *)arg, sizeof(mode))) {
			rc = -EFAULT;
			break;
		}
		if (mode >= 4100 && mode <= 4800) {
			dump_stack();
			pr_err("msgid = %d\n", mode);
			BUG();
		}
                break;
	default:
		rc = -EINVAL;
	}
	mutex_unlock(&api_lock);
	return rc;
}

static ssize_t beats_print_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "Beats\n");
}

static struct file_operations acoustic_fops = {
	.owner = THIS_MODULE,
	.open = acoustic_open,
	.release = acoustic_release,
	.unlocked_ioctl = acoustic_ioctl,
};

static struct miscdevice acoustic_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "htc-acoustic",
	.fops = &acoustic_fops,
};

static int __init acoustic_init(void)
{
	int ret = 0;

	mutex_init(&api_lock);
	ret = misc_register(&acoustic_misc);
	if (ret < 0) {
		pr_err("failed to register misc device!\n");
		return ret;
	}

	sdev_beats.name = "Beats";
	sdev_beats.print_name = beats_print_name;

	ret = switch_dev_register(&sdev_beats);
	if (ret < 0) {
		pr_err("failed to register beats switch device!\n");
		return ret;
	}
	return 0;
}

static void __exit acoustic_exit(void)
{
	misc_deregister(&acoustic_misc);
}

module_init(acoustic_init);
module_exit(acoustic_exit);
