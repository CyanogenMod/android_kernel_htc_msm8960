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

#include <mach/scm.h>
#include <mach/msm_iomap.h>

#define DEBUG_MAX_RW_BUF 4096

#define TZBSP_CPU_COUNT 0x02
#define TZBSP_DIAG_NUM_OF_VMID 16
#define TZBSP_DIAG_VMID_DESC_LEN 7
#define TZBSP_DIAG_INT_NUM  32
#define TZBSP_MAX_INT_DESC 16
struct tzdbg_vmid_t {
	uint8_t vmid; 
	uint8_t desc[TZBSP_DIAG_VMID_DESC_LEN];	
};
struct tzdbg_boot_info_t {
	uint32_t wb_entry_cnt;	
	uint32_t wb_exit_cnt;	
	uint32_t pc_entry_cnt;	
	uint32_t pc_exit_cnt;	
	uint32_t warm_jmp_addr;	
	uint32_t spare;	
};
struct tzdbg_reset_info_t {
	uint32_t reset_type;	
	uint32_t reset_cnt;	
};
struct tzdbg_int_t {
	uint16_t int_info;
	uint8_t avail;
	uint8_t spare;
	uint32_t int_num;
	uint8_t int_desc[TZBSP_MAX_INT_DESC];
	uint64_t int_count[TZBSP_CPU_COUNT]; 
};
struct tzdbg_t {
	uint32_t magic_num;
	uint32_t version;
	uint32_t cpu_count;
	uint32_t vmid_info_off;
	uint32_t boot_info_off;
	uint32_t reset_info_off;
	uint32_t int_info_off;
	uint32_t ring_off;
	uint32_t ring_len;
	struct tzdbg_vmid_t vmid_info[TZBSP_DIAG_NUM_OF_VMID];
	struct tzdbg_boot_info_t  boot_info[TZBSP_CPU_COUNT];
	struct tzdbg_reset_info_t reset_info[TZBSP_CPU_COUNT];
	uint32_t num_interrupts;
	struct tzdbg_int_t  int_info[TZBSP_DIAG_INT_NUM];
	uint8_t *ring_buffer;	
};

enum tzdbg_stats_type {
	TZDBG_BOOT = 0,
	TZDBG_RESET,
	TZDBG_INTERRUPT,
	TZDBG_VMID,
	TZDBG_GENERAL,
	TZDBG_LOG,
	TZDBG_STATS_MAX,
};

struct tzdbg_stat {
	char *name;
	char *data;
};

struct tzdbg {
	void __iomem *virt_iobase;
	struct tzdbg_t *diag_buf;
	char *disp_buf;
	int debug_tz[TZDBG_STATS_MAX];
	struct tzdbg_stat stat[TZDBG_STATS_MAX];
};

static struct tzdbg tzdbg = {

	.stat[TZDBG_BOOT].name = "boot",
	.stat[TZDBG_RESET].name = "reset",
	.stat[TZDBG_INTERRUPT].name = "interrupt",
	.stat[TZDBG_VMID].name = "vmid",
	.stat[TZDBG_GENERAL].name = "general",
	.stat[TZDBG_LOG].name = "log",
};

#define TZ_SCM_LOG_PHYS		MSM_TZLOG_PHYS
#define TZ_SCM_LOG_SIZE		MSM_TZLOG_SIZE
#define INT_SIZE		4

struct htc_tzlog_dev {
	char *buffer;
	int *pw_cursor;
	int *pr_cursor;
};

struct htc_tzlog_dev *htc_tzlog;


static int _disp_tz_general_stats(void)
{
	int len = 0;

	len += snprintf(tzdbg.disp_buf + len, DEBUG_MAX_RW_BUF - 1,
			"   Version        : 0x%x\n"
			"   Magic Number   : 0x%x\n"
			"   Number of CPU  : %d\n",
			tzdbg.diag_buf->version,
			tzdbg.diag_buf->magic_num,
			tzdbg.diag_buf->cpu_count);
	tzdbg.stat[TZDBG_GENERAL].data = tzdbg.disp_buf;
	return len;
}

static int _disp_tz_vmid_stats(void)
{
	int i, num_vmid;
	int len = 0;
	struct tzdbg_vmid_t *ptr;

	ptr = (struct tzdbg_vmid_t *)((unsigned char *)tzdbg.diag_buf +
					tzdbg.diag_buf->vmid_info_off);
	num_vmid = ((tzdbg.diag_buf->boot_info_off -
				tzdbg.diag_buf->vmid_info_off)/
					(sizeof(struct tzdbg_vmid_t)));

	for (i = 0; i < num_vmid; i++) {
		if (ptr->vmid < 0xFF) {
			len += snprintf(tzdbg.disp_buf + len,
				(DEBUG_MAX_RW_BUF - 1) - len,
				"   0x%x        %s\n",
				(uint32_t)ptr->vmid, (uint8_t *)ptr->desc);
		}
		if (len > (DEBUG_MAX_RW_BUF - 1)) {
			pr_warn("%s: Cannot fit all info into the buffer\n",
								__func__);
			break;
		}
		ptr++;
	}

	tzdbg.stat[TZDBG_VMID].data = tzdbg.disp_buf;
	return len;
}

static int _disp_tz_boot_stats(void)
{
	int i;
	int len = 0;
	struct tzdbg_boot_info_t *ptr;

	ptr = (struct tzdbg_boot_info_t *)((unsigned char *)tzdbg.diag_buf +
					tzdbg.diag_buf->boot_info_off);

	for (i = 0; i < tzdbg.diag_buf->cpu_count; i++) {
		len += snprintf(tzdbg.disp_buf + len,
				(DEBUG_MAX_RW_BUF - 1) - len,
				"  CPU #: %d\n"
				"     Warmboot jump address     : 0x%x\n"
				"     Warmboot entry CPU counter: 0x%x\n"
				"     Warmboot exit CPU counter : 0x%x\n"
				"     Power Collapse entry CPU counter: 0x%x\n"
				"     Power Collapse exit CPU counter : 0x%x\n",
				i, ptr->warm_jmp_addr, ptr->wb_entry_cnt,
				ptr->wb_exit_cnt, ptr->pc_entry_cnt,
				ptr->pc_exit_cnt);

		if (len > (DEBUG_MAX_RW_BUF - 1)) {
			pr_warn("%s: Cannot fit all info into the buffer\n",
								__func__);
			break;
		}
		ptr++;
	}
	tzdbg.stat[TZDBG_BOOT].data = tzdbg.disp_buf;
	return len;
}

static int _disp_tz_reset_stats(void)
{
	int i;
	int len = 0;
	struct tzdbg_reset_info_t *ptr;

	ptr = (struct tzdbg_reset_info_t *)((unsigned char *)tzdbg.diag_buf +
					tzdbg.diag_buf->reset_info_off);

	for (i = 0; i < tzdbg.diag_buf->cpu_count; i++) {
		len += snprintf(tzdbg.disp_buf + len,
				(DEBUG_MAX_RW_BUF - 1) - len,
				"  CPU #: %d\n"
				"     Reset Type (reason)       : 0x%x\n"
				"     Reset counter             : 0x%x\n",
				i, ptr->reset_type, ptr->reset_cnt);

		if (len > (DEBUG_MAX_RW_BUF - 1)) {
			pr_warn("%s: Cannot fit all info into the buffer\n",
								__func__);
			break;
		}

		ptr++;
	}
	tzdbg.stat[TZDBG_RESET].data = tzdbg.disp_buf;
	return len;
}

static int _disp_tz_interrupt_stats(void)
{
	int i, j, int_info_size;
	int len = 0;
	int *num_int;
	unsigned char *ptr;
	struct tzdbg_int_t *tzdbg_ptr;

	num_int = (uint32_t *)((unsigned char *)tzdbg.diag_buf +
			(tzdbg.diag_buf->int_info_off - sizeof(uint32_t)));
	ptr = ((unsigned char *)tzdbg.diag_buf +
					tzdbg.diag_buf->int_info_off);
	int_info_size = ((tzdbg.diag_buf->ring_off -
				tzdbg.diag_buf->int_info_off)/(*num_int));

	for (i = 0; i < (*num_int); i++) {
		tzdbg_ptr = (struct tzdbg_int_t *)ptr;
		len += snprintf(tzdbg.disp_buf + len,
				(DEBUG_MAX_RW_BUF - 1) - len,
				"     Interrupt Number          : 0x%x\n"
				"     Type of Interrupt         : 0x%x\n"
				"     Description of interrupt  : %s\n",
				tzdbg_ptr->int_num,
				(uint32_t)tzdbg_ptr->int_info,
				(uint8_t *)tzdbg_ptr->int_desc);
		for (j = 0; j < tzdbg.diag_buf->cpu_count; j++) {
			len += snprintf(tzdbg.disp_buf + len,
				(DEBUG_MAX_RW_BUF - 1) - len,
				"     int_count on CPU # %d      : %u\n",
				(uint32_t)j,
				(uint32_t)tzdbg_ptr->int_count[j]);
		}
		len += snprintf(tzdbg.disp_buf + len, DEBUG_MAX_RW_BUF - 1,
									"\n");

		if (len > (DEBUG_MAX_RW_BUF - 1)) {
			pr_warn("%s: Cannot fit all info into the buffer\n",
								__func__);
			break;
		}

		ptr += int_info_size;
	}
	tzdbg.stat[TZDBG_INTERRUPT].data = tzdbg.disp_buf;
	return len;
}

#if 0
static int _disp_tz_log_stats(void)
{
	int len = 0;
	unsigned char *ptr;

	ptr = (unsigned char *)tzdbg.diag_buf +
					tzdbg.diag_buf->ring_off;
	len += snprintf(tzdbg.disp_buf, (DEBUG_MAX_RW_BUF - 1) - len,
							"%s\n", ptr);

	tzdbg.stat[TZDBG_LOG].data = tzdbg.disp_buf;
	return len;
}
#else
static int _disp_tz_htc_log_stats(char __user *ubuf, size_t count, loff_t *offp)
{
	char *buf = htc_tzlog->buffer;
	int *pw_cursor = htc_tzlog->pw_cursor;
	int *pr_cursor = htc_tzlog->pr_cursor;
	int r_cursor, w_cursor, ret;

	if (buf != 0) {
		
		r_cursor = *pr_cursor;
		w_cursor = *pw_cursor;

		if (r_cursor < w_cursor) {
			if ((w_cursor - r_cursor) > count) {
				ret = copy_to_user(ubuf, buf + r_cursor, count);
				if (ret == count)
					return -EFAULT;

				*pr_cursor = r_cursor + count;
				return count;
			} else {
				ret = copy_to_user(ubuf, buf + r_cursor, (w_cursor - r_cursor));
				if (ret == (w_cursor - r_cursor))
					return -EFAULT;

				*pr_cursor = w_cursor;
				return (w_cursor - r_cursor);
			}
		}

		if (r_cursor > w_cursor) {
			int buf_end = TZ_SCM_LOG_SIZE - 2*INT_SIZE - 1;
			int left_len = buf_end - r_cursor;

			if (left_len > count) {
				ret = copy_to_user(ubuf, buf + r_cursor, count);
				if (ret == count)
					return -EFAULT;

				*pr_cursor = r_cursor + count;
				return count;
			} else {
				ret = copy_to_user(ubuf, buf + r_cursor, left_len);
				if (ret == left_len)
					return -EFAULT;

				*pr_cursor = 0;
				return left_len;
			}
		}

		if (r_cursor == w_cursor) {
			pr_info("No New Trust Zone log\n");
			return 0;
		}
	}

	return 0;
}
#endif

static ssize_t tzdbgfs_read(struct file *file, char __user *buf,
	size_t count, loff_t *offp)
{
	int len = 0;
	int *tz_id =  file->private_data;

	memcpy_fromio((void *)tzdbg.diag_buf, tzdbg.virt_iobase,
						DEBUG_MAX_RW_BUF);
	switch (*tz_id) {
	case TZDBG_BOOT:
		len = _disp_tz_boot_stats();
		break;
	case TZDBG_RESET:
		len = _disp_tz_reset_stats();
		break;
	case TZDBG_INTERRUPT:
		len = _disp_tz_interrupt_stats();
		break;
	case TZDBG_GENERAL:
		len = _disp_tz_general_stats();
		break;
	case TZDBG_VMID:
		len = _disp_tz_vmid_stats();
		break;
	case TZDBG_LOG:
#if 0
		len = _disp_tz_log_stats();
		break;
#else
		return _disp_tz_htc_log_stats(buf, count, offp);
#endif
	default:
		break;
	}

	if (len > count)
		len = count;

	return simple_read_from_buffer(buf, len, offp,
				tzdbg.stat[(*tz_id)].data, len);
}

static int tzdbgfs_open(struct inode *inode, struct file *pfile)
{
	pfile->private_data = inode->i_private;
	return 0;
}

const struct file_operations tzdbg_fops = {
	.owner   = THIS_MODULE,
	.read    = tzdbgfs_read,
	.open    = tzdbgfs_open,
};

static int  tzdbgfs_init(struct platform_device *pdev)
{
	int rc = 0;
	int i;
	struct dentry           *dent_dir;
	struct dentry           *dent;

	dent_dir = debugfs_create_dir("tzdbg", NULL);
	if (dent_dir == NULL) {
		dev_err(&pdev->dev, "tzdbg debugfs_create_dir failed\n");
		return -ENOMEM;
	}

	for (i = 0; i < TZDBG_STATS_MAX; i++) {
		tzdbg.debug_tz[i] = i;
		dent = debugfs_create_file(tzdbg.stat[i].name,
				S_IRUGO, dent_dir,
				&tzdbg.debug_tz[i], &tzdbg_fops);
		if (dent == NULL) {
			dev_err(&pdev->dev, "TZ debugfs_create_file failed\n");
			rc = -ENOMEM;
			goto err;
		}
	}
	tzdbg.disp_buf = kzalloc(DEBUG_MAX_RW_BUF, GFP_KERNEL);
	if (tzdbg.disp_buf == NULL) {
		pr_err("%s: Can't Allocate memory for tzdbg.disp_buf\n",
			__func__);

		goto err;
	}
	platform_set_drvdata(pdev, dent_dir);
	return 0;
err:
	debugfs_remove_recursive(dent_dir);

	return rc;
}

static void tzdbgfs_exit(struct platform_device *pdev)
{
	struct dentry           *dent_dir;

	kzfree(tzdbg.disp_buf);
	dent_dir = platform_get_drvdata(pdev);
	debugfs_remove_recursive(dent_dir);
}

static int __devinit tz_log_probe(struct platform_device *pdev)
{
	struct resource *resource;
	void __iomem *virt_iobase;
	uint32_t tzdiag_phy_iobase;
	uint32_t *ptr = NULL;

	resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!resource) {
		dev_err(&pdev->dev,
				"%s: ERROR Missing MEM resource\n", __func__);
		return -ENXIO;
	};
	virt_iobase = devm_ioremap_nocache(&pdev->dev, resource->start,
				resource->end - resource->start + 1);
	if (!virt_iobase) {
		dev_err(&pdev->dev,
			"%s: ERROR could not ioremap: start=%p, len=%u\n",
			__func__, (void *) resource->start,
			(resource->end - resource->start + 1));
		return -ENXIO;
	}
	tzdiag_phy_iobase = readl_relaxed(virt_iobase);

	tzdbg.virt_iobase = devm_ioremap_nocache(&pdev->dev,
				tzdiag_phy_iobase, DEBUG_MAX_RW_BUF);

	if (!tzdbg.virt_iobase) {
		dev_err(&pdev->dev,
			"%s: ERROR could not ioremap: start=%p, len=%u\n",
			__func__, (void *) tzdiag_phy_iobase, DEBUG_MAX_RW_BUF);
		return -ENXIO;
	}

	ptr = kzalloc(DEBUG_MAX_RW_BUF, GFP_KERNEL);
	if (ptr == NULL) {
		pr_err("%s: Can't Allocate memory: ptr\n",
			__func__);
		return -ENXIO;
	}

	tzdbg.diag_buf = (struct tzdbg_t *)ptr;

	htc_tzlog = kzalloc(sizeof(struct htc_tzlog_dev), GFP_KERNEL);
	if (!htc_tzlog) {
		pr_err("%s: Can't Allocate memory: scm_dev\n", __func__);
		return -ENOMEM;
	}

	htc_tzlog->buffer = devm_ioremap_nocache(&pdev->dev,
		TZ_SCM_LOG_PHYS, TZ_SCM_LOG_SIZE);
	if (htc_tzlog->buffer == NULL) {
		pr_err("%s: ioremap fail...\n", __func__);
		kfree(htc_tzlog);
		return -EFAULT;
	}

	htc_tzlog->pr_cursor = (int *)((int)(htc_tzlog->buffer) +
				 TZ_SCM_LOG_SIZE - 2*INT_SIZE);
	htc_tzlog->pw_cursor = (int *)((int)(htc_tzlog->buffer) +
				 TZ_SCM_LOG_SIZE - INT_SIZE);

	pr_info("tzlog buffer address %x\n", TZ_SCM_LOG_PHYS);
	memset(htc_tzlog->buffer, 0, TZ_SCM_LOG_SIZE);

	secure_log_operation(0, 0, TZ_SCM_LOG_PHYS, 32 * 64, 0);

	pr_info("[TZ] ---LOG START---\n");
	pr_info("%s", htc_tzlog->buffer);
	pr_info("[TZ] --- LOG END---\n");

	secure_log_operation(TZ_SCM_LOG_PHYS, TZ_SCM_LOG_SIZE, 0, 0, 0);

	if (tzdbgfs_init(pdev))
		goto err;

	return 0;
err:
	kfree(tzdbg.diag_buf);
	return -ENXIO;
}


static int __devexit tz_log_remove(struct platform_device *pdev)
{
	kzfree(tzdbg.diag_buf);
	tzdbgfs_exit(pdev);

	return 0;
}

static struct of_device_id tzlog_match[] = {
	{	.compatible = "qcom,tz-log",
	},
	{}
};

static struct platform_driver tz_log_driver = {
	.probe		= tz_log_probe,
	.remove		= __devexit_p(tz_log_remove),
	.driver		= {
		.name = "tz_log",
		.owner = THIS_MODULE,
		.of_match_table = tzlog_match,
	},
};

static int __init tz_log_init(void)
{
	return platform_driver_register(&tz_log_driver);
}

static void __exit tz_log_exit(void)
{
	platform_driver_unregister(&tz_log_driver);
}

module_init(tz_log_init);
module_exit(tz_log_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("TZ Log driver");
MODULE_VERSION("1.1");
MODULE_ALIAS("platform:tz_log");
