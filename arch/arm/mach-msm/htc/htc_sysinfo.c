/*
 * Copyright (C) 2010 HTC, Inc.
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <mach/board.h>

#include <mach/gpio.h>			
#include <linux/mfd/pm8xxx/gpio.h>	
#include <linux/mfd/pm8xxx/mpp.h>	

#include <mach/msm_iomap.h>

#define GPIO_INFO_BUFFER_SIZE	20000
#define REG(x)	{#x, MSM_EBI1_CH0_##x##_ADDR}
#define REGI(x, n)	{#x "_" #n, MSM_EBI1_CH0_##x##_ADDR(n)}

#define CHECK_PROC_ENTRY(name, entry) do { \
				if (entry) { \
					pr_info("Create /proc/%s OK.\n", name); \
				} else { \
					pr_err("Create /proc/%s FAILED.\n", name); \
				} \
			} while (0);


static char gpio_info_buffer[GPIO_INFO_BUFFER_SIZE];

static int sys_gpio_read_proc(char *page, char **start, off_t off,
			int count, int *eof, void *data)
{
	char *p = page;
	uint32_t len = 0;

	pr_info("%s: page=0x%08x, start=0x%08x, off=%d, count=%d, eof=%d, data=0x%08x\n", __func__,
			(uint32_t)page, (uint32_t)*start, (int)off, count, *eof, (uint32_t)data);

	
	*start = page;

	if (!off) {
		
		memset(gpio_info_buffer, 0, sizeof(gpio_info_buffer));
		len = 0;
#if 0 
		len = msm_dump_gpios(NULL, len, gpio_info_buffer);
		len = pm8xxx_dump_gpios(NULL, len, gpio_info_buffer);
		len = pm8xxx_dump_mpp(NULL, len, gpio_info_buffer);
#endif
		pr_info("%s: total bytes = %d.\n", __func__, len);
	}
	len = strlen(gpio_info_buffer + off);

	
	if (len > 0) {
		
		if (len >= count)
			len = count;
		pr_info("%s: transfer %d bytes.", __func__, len);
		memcpy(p, gpio_info_buffer + off, len);
	}

	
	if (len < count) {
		pr_info("%s: end.", __func__);
		*eof = 1;
	}

	return len;
}

extern int board_get_boot_powerkey_debounce_time(void);
int sys_boot_powerkey_debounce_ms(char *page, char **start, off_t off,
			   int count, int *eof, void *data)
{
	char *p = page;

	p += sprintf(p, "%d\n", board_get_boot_powerkey_debounce_time());

	return p - page;
}

static int __init sysinfo_proc_init(void)
{
	struct proc_dir_entry *entry = NULL;

	pr_info("%s: Init HTC system info proc interface.\r\n", __func__);

	
	entry = create_proc_read_entry("emmc", 0, NULL, emmc_partition_read_proc, NULL);
	CHECK_PROC_ENTRY("emmc", entry);

	entry = create_proc_read_entry("gpio_info", 0, NULL, sys_gpio_read_proc, NULL);
	CHECK_PROC_ENTRY("gpio_info", entry);

	entry = create_proc_read_entry("powerkey_debounce_ms", 0, NULL, sys_boot_powerkey_debounce_ms, NULL);
	CHECK_PROC_ENTRY("powerkey_debounce_ms", entry);

	entry = create_proc_read_entry("dying_processes", 0, NULL, dying_processors_read_proc, NULL);
	CHECK_PROC_ENTRY("dying_processes", entry);

	return 0;
}

module_init(sysinfo_proc_init);
MODULE_AUTHOR("Jimmy.CM Chen <jimmy.cm_chen@htc.com>");
MODULE_DESCRIPTION("HTC System Info Interface");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL v2");
