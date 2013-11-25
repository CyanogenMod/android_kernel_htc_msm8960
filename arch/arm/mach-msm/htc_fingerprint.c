/* arch/arm/mach-msm/htc_fingerprint.c
 *
 * Copyright (C) 2013 HTC Corporation.
 * Author: Tim_Lee <tim_lee@htc.com>
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
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>

#include <mach/scm.h>

#define DEVICE_NAME "htc_fingerprint"

#define HTC_IOCTL_FPSERVICE 	0x9526
#define HTC_IOCTL_SEC_ATS_GET	0x9528
#define HTC_IOCTL_SEC_ATS_SET	0x9529

#define TAG "[SEC] "
#define HTC_FINGERPRINT_DEBUG	0
#undef PDEBUG
#if HTC_FINGERPRINT_DEBUG
#define PDEBUG(fmt, args...) printk(KERN_INFO TAG "[K] %s(%i, %s): " fmt "\n", \
		__func__, current->pid, current->comm, ## args)
#else
#define PDEBUG(fmt, args...) do {} while (0)
#endif 

#undef PERR
#define PERR(fmt, args...) printk(KERN_ERR TAG "[E] %s(%i, %s): " fmt "\n", \
		__func__, current->pid, current->comm, ## args)

#undef PINFO
#define PINFO(fmt, args...) printk(KERN_INFO TAG "[I] %s(%i, %s): " fmt "\n", \
		__func__, current->pid, current->comm, ## args)

static int htc_fingerprint_major;
static struct class *htc_fingerprint_class;
static const struct file_operations htc_fingerprint_fops;

static unsigned char *htc_fpkey;

typedef struct _htc_fingerprint_msg_s{
	int func;
	int offset;
	unsigned char *req_buf;
	int req_len;
	unsigned char *resp_buf;
	int resp_len;
} htc_fingerprint_msg_s;

typedef struct {
	struct {
		uint8_t func_id;
		uint8_t func_cur_state;
		int8_t  func_return;
		uint8_t func_exec;
	} func_info;
	uint32_t    input[2];
	uint32_t    output[2];
	uint8_t reserve[4];
} htc_sec_ats_t;

void scm_inv_range(unsigned long start, unsigned long end);

static long htc_fingerprint_ioctl(struct file *file, unsigned int command, unsigned long arg)
{
	htc_fingerprint_msg_s hmsg;
	htc_sec_ats_t amsg;
	int ret = 0;

	switch (command) {
	case HTC_IOCTL_FPSERVICE:
		if (copy_from_user(&hmsg, (void __user *)arg, sizeof(hmsg))) {
			PERR("copy_from_user error (msg)");
			return -EFAULT;
		}
		PERR("func = %x", hmsg.func);
		switch (hmsg.func) {
		case ITEM_FP_KEY_ENCRYPT:
			if ((hmsg.req_buf == NULL) || (hmsg.req_len == 0)){
				PERR("invalid arguments (ITEM_FP_KEY_ENCRYPT)");
				return -EFAULT;
			}
			htc_fpkey = kzalloc(hmsg.req_len, GFP_KERNEL);
			if (htc_fpkey == NULL) {
				PERR("allocate the space for data failed (%d)", hmsg.req_len);
				return -EFAULT;
			}
			if (copy_from_user(htc_fpkey, (void __user *)hmsg.req_buf, hmsg.req_len)) {
				PERR("copy_from_user error (fpkey)");
				kfree(htc_fpkey);
				return -EFAULT;
			}

			ret = secure_access_item(0, ITEM_FP_KEY_ENCRYPT, hmsg.req_len, htc_fpkey);

			if (ret)
				PERR("encrypt FP key fail (%d)", ret);

			scm_inv_range((uint32_t)htc_fpkey, (uint32_t)htc_fpkey + hmsg.req_len);
			if (copy_to_user((void __user *)hmsg.resp_buf, htc_fpkey, hmsg.req_len)) {
				PERR("copy_to_user error (fpkey)");
				kfree(htc_fpkey);
				return -EFAULT;
			}

			kfree(htc_fpkey);
			break;

		case ITEM_FP_KEY_DECRYPT:
			if ((hmsg.req_buf == NULL) || (hmsg.req_len == 0)){
				PERR("invalid arguments");
				return -EFAULT;
			}
			htc_fpkey = kzalloc(hmsg.req_len, GFP_KERNEL);
			if (htc_fpkey == NULL) {
				PERR("allocate the space for data failed (%d)", hmsg.req_len);
				return -EFAULT;
			}
			if (copy_from_user(htc_fpkey, (void __user *)hmsg.req_buf, hmsg.req_len)) {
				PERR("copy_from_user error (fpkey)");
				kfree(htc_fpkey);
				return -EFAULT;
			}

			ret = secure_access_item(0, ITEM_FP_KEY_DECRYPT, hmsg.req_len, htc_fpkey);

			if (ret)
				PERR("decrypt FP key fail (%d)", ret);

			scm_inv_range((uint32_t)htc_fpkey, (uint32_t)htc_fpkey + hmsg.req_len);
			if (copy_to_user((void __user *)hmsg.resp_buf, htc_fpkey, hmsg.req_len)) {
				PERR("copy_to_user error (fpkey)");
				kfree(htc_fpkey);
				return -EFAULT;
			}

			kfree(htc_fpkey);
			break;

		default:
			PERR("func error");
			return -EFAULT;
		}
		break;

	case HTC_IOCTL_SEC_ATS_GET:
		if (!arg) {
			PERR("invalid arguments");
			return -ENOMEM;
		}
		ret = secure_access_item(0, ITEM_SEC_ATS, sizeof(htc_sec_ats_t), (unsigned char *)&amsg);
		if (ret) {
			PERR("ATS service fail (%d)", ret);
			return ret;
		}
		scm_inv_range((uint32_t)&amsg, (uint32_t)&amsg + sizeof(htc_sec_ats_t));

		if (copy_to_user((void __user *)arg, &amsg, sizeof(htc_sec_ats_t))) {
			PERR("copy_to_user error (msg)");
			return -EFAULT;
		}
		break;

	case HTC_IOCTL_SEC_ATS_SET:
		if (!arg) {
			PERR("invalid arguments");
			return -ENOMEM;
		}
		if (copy_from_user(&amsg, (void __user *)arg, sizeof(htc_sec_ats_t))) {
			PERR("copy_from_user error (msg)");
			return -EFAULT;
		}
		PDEBUG("func = %x, sizeof htc_sec_ats_t = %d", amsg.func_info.func_id, sizeof(htc_sec_ats_t));
		ret = secure_access_item(1, ITEM_SEC_ATS, sizeof(htc_sec_ats_t), (unsigned char *)&amsg);
		if (ret)
			PERR("ATS service fail (%d)", ret);
		break;

	default:
		PERR("command error");
		return -EFAULT;
	}
	return ret;
}


static int htc_fingerprint_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int htc_fingerprint_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations htc_fingerprint_fops = {
	.unlocked_ioctl = htc_fingerprint_ioctl,
	.open = htc_fingerprint_open,
	.release = htc_fingerprint_release,
	.owner = THIS_MODULE,
};

static int __init htc_fingerprint_init(void)
{
	int ret;

	ret = register_chrdev(0, DEVICE_NAME, &htc_fingerprint_fops);
	if (ret < 0) {
		PERR("register module fail");
		return ret;
	}
	htc_fingerprint_major = ret;

	htc_fingerprint_class = class_create(THIS_MODULE, "htc_fingerprint");
	device_create(htc_fingerprint_class, NULL, MKDEV(htc_fingerprint_major, 0), NULL, DEVICE_NAME);

	PDEBUG("register module ok");
	return 0;
}


static void  __exit htc_fingerprint_exit(void)
{
	device_destroy(htc_fingerprint_class, MKDEV(htc_fingerprint_major, 0));
	class_unregister(htc_fingerprint_class);
	class_destroy(htc_fingerprint_class);
	unregister_chrdev(htc_fingerprint_major, DEVICE_NAME);
	PDEBUG("un-registered module ok");
}

module_init(htc_fingerprint_init);
module_exit(htc_fingerprint_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tim Lee");

