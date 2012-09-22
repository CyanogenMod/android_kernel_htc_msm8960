/*
 *
 * Copyright (C) 2012 HTC, Inc.
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

#include <linux/slab.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <asm/cacheflush.h>
#include <asm/uaccess.h>
#include <mach/scm.h>

char local_buf[128];
unsigned int mem_addr, mem_len, mem_chk_cln;

static ssize_t scm_memchk_write(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	mem_addr = 0;
	mem_len = 0;
	mem_chk_cln = 0;

	sscanf(buf, "%x %x %x", &mem_addr, &mem_len, &mem_chk_cln);

	return count;
}

static ssize_t scm_memchk_read(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	mem_chk_t *mem_chk;
	unsigned int addr, chk_cln = 0;
	unsigned int *rbuf, buf_len;
	int len, i, ret, prt_len = 0;

	if (mem_addr == 0 && mem_len == 0)
		return 0;

	/* Don't change the output format
	    (or MPU verification tool need to be modified accordingly) */
	addr = mem_addr;
	len = mem_len;
	chk_cln = mem_chk_cln;

	if (chk_cln)
		buf_len = sizeof(mem_chk_t);
	else
		buf_len = ((len<<2) > sizeof(mem_chk_t))? (len<<2): sizeof(mem_chk_t);

	/* buffer allocate for read memory */
	rbuf = (unsigned int *)kzalloc(buf_len, GFP_KERNEL);
	if (rbuf == NULL) {
		pr_err("[MEMCHK] Out of mem...\n");
		return -1;
	}

	mem_chk = (mem_chk_t *)rbuf;
	mem_chk->enable = 1;
	mem_chk->chk_cln = chk_cln;
	mem_chk->addr = addr;
	mem_chk->len = len;

	ret = secure_access_item(1, ITEM_READ_MEM, sizeof(mem_chk_t), (unsigned char *)mem_chk);
	if (ret) {
		pr_err("Access Error\n");
		kfree(rbuf);
		return -1;
	}

	ret = secure_access_item(0, ITEM_READ_MEM, buf_len, (unsigned char *)rbuf);
	if (ret) {
		pr_err("Access Error\n");
		kfree(rbuf);
		return -1;
	}

	if (chk_cln) {
		if (rbuf[0])
			prt_len += sprintf(buf + prt_len, "[%X]: %X\n", rbuf[0], rbuf[1]);
		else
			prt_len += sprintf(buf + prt_len, "Memory Region Clean\n");
	} else {
		for (i = 0; i < len; i++)
			prt_len += sprintf(buf + prt_len, "[%X]: %X\n", addr + (i<<2), rbuf[i]);
	}

	kfree(rbuf);
	return prt_len;
}
static DEVICE_ATTR(memchk, 0660, scm_memchk_read, scm_memchk_write);

static int scm_memchk_probe(struct platform_device *pdev)
{
	int ret;

	ret = device_create_file(&pdev->dev, &dev_attr_memchk);
	if (ret < 0)
		pr_err("%s: create device attribute fail \n", __func__);

	return ret;
}

static struct platform_driver scm_memchk_driver = {
	.probe = scm_memchk_probe,
	.driver = { .name = "scm-memchk", },
};


static int __init scm_memchk_device_init(void)
{
	return platform_driver_register(&scm_memchk_driver);
}

static void __exit scm_memchk_device_exit(void)
{
	platform_driver_unregister(&scm_memchk_driver);
}

module_init(scm_memchk_device_init);
module_exit(scm_memchk_device_exit);
