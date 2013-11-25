/* Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/completion.h>
#include <linux/cpuidle.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ktime.h>
#include <linux/pm.h>
#include <linux/pm_qos.h>
#include <linux/smp.h>
#include <linux/suspend.h>
#include <linux/tick.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <mach/msm_iomap.h>
#include <mach/socinfo.h>
#include <mach/system.h>
#include <mach/rpm.h>
#include <mach/htc_util.h>
#include <mach/board_htc.h>
#include <asm/system.h>
#include <asm/cacheflush.h>
#include <asm/hardware/gic.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/hardware/cache-l2x0.h>
#ifdef CONFIG_VFP
#include <asm/vfp.h>
#endif

#include "acpuclock.h"
#include "clock.h"
#include "avs.h"
#include <mach/cpuidle.h>
#include "idle.h"
#include "pm.h"
#include "rpm_resources.h"
#include "scm-boot.h"
#include "spm.h"
#include "timer.h"
#include "pm-boot.h"
#ifdef CONFIG_TRACING_IRQ_PWR
#include "../../../drivers/gpio/gpio-msm-common.h"
#define PWR_KEY_MSMz 26
#endif

#ifdef CONFIG_MSM_WATCHDOG
#include <mach/msm_watchdog.h>
#endif
#include <mach/msm_xo.h>
#include <linux/wakelock.h>
#include <linux/cpufreq.h>

#ifdef CONFIG_GPIO_MSM_V2
#include <linux/console.h>
#include "../../../drivers/gpio/gpio-msm-common.h"
#endif

#ifdef pr_err
#undef pr_err
#endif
#define pr_err(fmt, args...) \
	printk(KERN_ERR "[PM] " pr_fmt(fmt), ## args)

#ifdef pr_warn
#undef pr_warn
#endif
#define pr_warn(fmt, args...) \
	printk(KERN_WARNING "[PM] " pr_fmt(fmt), ## args)

#ifdef pr_info
#undef pr_info
#endif
#define pr_info(fmt, args...) \
	printk(KERN_INFO "[PM] " pr_fmt(fmt), ## args)

#if defined(DEBUG)
#ifdef pr_debug
#undef pr_debug
#endif
#define pr_debug(fmt, args...) \
	printk(KERN_DEBUG "[PM] " pr_fmt(fmt), ## args)
#endif


enum {
	MSM_PM_DEBUG_SUSPEND = BIT(0),
	MSM_PM_DEBUG_POWER_COLLAPSE = BIT(1),
	MSM_PM_DEBUG_SUSPEND_LIMITS = BIT(2),
	MSM_PM_DEBUG_CLOCK = BIT(3),
	MSM_PM_DEBUG_RESET_VECTOR = BIT(4),
	MSM_PM_DEBUG_IDLE_CLK = BIT(5),
	MSM_PM_DEBUG_IDLE = BIT(6),
	MSM_PM_DEBUG_IDLE_LIMITS = BIT(7),
	MSM_PM_DEBUG_HOTPLUG = BIT(8),
	MSM_PM_DEBUG_GPIO = BIT(9),
	MSM_PM_DEBUG_RPM_TIMESTAMP = BIT(10),
	MSM_PM_DEBUG_IDLE_CLOCK = BIT(11),
	MSM_PM_DEBUG_RPM_STAT = BIT(12),
	MSM_PM_DEBUG_VREG = BIT(13),
};

static int msm_pm_debug_mask = MSM_PM_DEBUG_SUSPEND | MSM_PM_DEBUG_SUSPEND_LIMITS
	| MSM_PM_DEBUG_RPM_STAT | MSM_PM_DEBUG_CLOCK | MSM_PM_DEBUG_RPM_TIMESTAMP;

module_param_named(
	debug_mask, msm_pm_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP
);

extern int board_mfg_mode(void);
#ifdef CONFIG_APQ8064_ONLY 
extern unsigned long acpuclk_krait_power_collapse(void);
#endif
#if defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY)
extern int dlp_ext_buck_en(int);
#endif
#define CPU_FOOT_PRINT_MAGIC				0xACBDFE00
#define CPU_FOOT_PRINT_MAGIC_SPC			0xACBDAA00
#define CPU_FOOT_PRINT_BASE_CPU0_VIRT		(CPU_FOOT_PRINT_BASE + 0x0)

static void init_cpu_foot_print(unsigned cpu, bool notify_rpm)
{
	unsigned *status = (unsigned *)CPU_FOOT_PRINT_BASE_CPU0_VIRT + cpu;
	*status = (notify_rpm) ? CPU_FOOT_PRINT_MAGIC : CPU_FOOT_PRINT_MAGIC_SPC;
	mb();
}

static void set_cpu_foot_print(unsigned cpu, unsigned state)
{
	unsigned *status = (unsigned *)CPU_FOOT_PRINT_BASE_CPU0_VIRT + cpu;
	*(unsigned char *)status = (unsigned char)state;
	mb();
}

#define RESET_VECTOR_CLEAN_MAGIC		0xDCBAABCD
#define CPU_RESET_VECTOR_CPU0_BASE	(CPU_FOOT_PRINT_BASE + 0x28)
static void clean_reset_vector_debug_info(unsigned cpu)
{
	unsigned *reset_vector = (unsigned *)CPU_RESET_VECTOR_CPU0_BASE;
	reset_vector[cpu] = RESET_VECTOR_CLEAN_MAGIC;
	mb();
}

#define SAVE_MSM_PM_BOOT_ENTRY_BASE		(CPU_FOOT_PRINT_BASE + 0x20)
static void store_pm_boot_entry_addr(void)
{
	unsigned *addr;
	addr = (unsigned *)SAVE_MSM_PM_BOOT_ENTRY_BASE;
	*addr = (unsigned)virt_to_phys(msm_pm_boot_entry);
	mb();
}

#define SAVE_MSM_PM_BOOT_VECTOR_BASE			(CPU_FOOT_PRINT_BASE + 0x24)
static void store_pm_boot_vector_addr(unsigned value)
{
	unsigned *addr;
	addr = (unsigned *)SAVE_MSM_PM_BOOT_VECTOR_BASE;
	*addr = (unsigned)value;
	mb();
}

enum {
	MSM_PM_MODE_ATTR_SUSPEND,
	MSM_PM_MODE_ATTR_IDLE,
	MSM_PM_MODE_ATTR_NR,
};

static char *msm_pm_mode_attr_labels[MSM_PM_MODE_ATTR_NR] = {
	[MSM_PM_MODE_ATTR_SUSPEND] = "suspend_enabled",
	[MSM_PM_MODE_ATTR_IDLE] = "idle_enabled",
};

struct msm_pm_kobj_attribute {
	unsigned int cpu;
	struct kobj_attribute ka;
};

#define GET_CPU_OF_ATTR(attr) \
	(container_of(attr, struct msm_pm_kobj_attribute, ka)->cpu)

struct msm_pm_sysfs_sleep_mode {
	struct kobject *kobj;
	struct attribute_group attr_group;
	struct attribute *attrs[MSM_PM_MODE_ATTR_NR + 1];
	struct msm_pm_kobj_attribute kas[MSM_PM_MODE_ATTR_NR];
};

static char *msm_pm_sleep_mode_labels[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE] = "power_collapse",
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT] = "wfi",
	[MSM_PM_SLEEP_MODE_RETENTION] = "retention",
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE] =
		"standalone_power_collapse",
};

static struct msm_pm_sleep_ops pm_sleep_ops;
static ssize_t msm_pm_mode_attr_show(
	struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int ret = -EINVAL;
	int i;

	for (i = 0; i < MSM_PM_SLEEP_MODE_NR; i++) {
		struct kernel_param kp;
		unsigned int cpu;
		struct msm_pm_platform_data *mode;

		if (msm_pm_sleep_mode_labels[i] == NULL)
			continue;

		if (strcmp(kobj->name, msm_pm_sleep_mode_labels[i]))
			continue;

		cpu = GET_CPU_OF_ATTR(attr);
		mode = &msm_pm_sleep_modes[MSM_PM_MODE(cpu, i)];

		if (!strcmp(attr->attr.name,
			msm_pm_mode_attr_labels[MSM_PM_MODE_ATTR_SUSPEND])) {
			u32 arg = mode->suspend_enabled;
			kp.arg = &arg;
			ret = param_get_ulong(buf, &kp);
		} else if (!strcmp(attr->attr.name,
			msm_pm_mode_attr_labels[MSM_PM_MODE_ATTR_IDLE])) {
			u32 arg = mode->idle_enabled;
			kp.arg = &arg;
			ret = param_get_ulong(buf, &kp);
		}

		break;
	}

	if (ret > 0) {
		strlcat(buf, "\n", PAGE_SIZE);
		ret++;
	}

	return ret;
}

static ssize_t msm_pm_mode_attr_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret = -EINVAL;
	int i;

	for (i = 0; i < MSM_PM_SLEEP_MODE_NR; i++) {
		struct kernel_param kp;
		unsigned int cpu;
		struct msm_pm_platform_data *mode;

		if (msm_pm_sleep_mode_labels[i] == NULL)
			continue;

		if (strcmp(kobj->name, msm_pm_sleep_mode_labels[i]))
			continue;

		cpu = GET_CPU_OF_ATTR(attr);
		mode = &msm_pm_sleep_modes[MSM_PM_MODE(cpu, i)];

		if (!strcmp(attr->attr.name,
			msm_pm_mode_attr_labels[MSM_PM_MODE_ATTR_SUSPEND])) {
			kp.arg = &mode->suspend_enabled;
			ret = param_set_byte(buf, &kp);
		} else if (!strcmp(attr->attr.name,
			msm_pm_mode_attr_labels[MSM_PM_MODE_ATTR_IDLE])) {
			kp.arg = &mode->idle_enabled;
			ret = param_set_byte(buf, &kp);
		}

		break;
	}

	return ret ? ret : count;
}

static int __init msm_pm_mode_sysfs_add_cpu(
	unsigned int cpu, struct kobject *modes_kobj)
{
	char cpu_name[8];
	struct kobject *cpu_kobj;
	struct msm_pm_sysfs_sleep_mode *mode = NULL;
	int i, j, k;
	int ret;

	snprintf(cpu_name, sizeof(cpu_name), "cpu%u", cpu);
	cpu_kobj = kobject_create_and_add(cpu_name, modes_kobj);
	if (!cpu_kobj) {
		pr_err("%s: cannot create %s kobject\n", __func__, cpu_name);
		ret = -ENOMEM;
		goto mode_sysfs_add_cpu_exit;
	}

	for (i = 0; i < MSM_PM_SLEEP_MODE_NR; i++) {
		int idx = MSM_PM_MODE(cpu, i);

		if ((!msm_pm_sleep_modes[idx].suspend_supported)
			&& (!msm_pm_sleep_modes[idx].idle_supported))
			continue;

		if (!msm_pm_sleep_mode_labels[i] ||
				!msm_pm_sleep_mode_labels[i][0])
			continue;

		mode = kzalloc(sizeof(*mode), GFP_KERNEL);
		if (!mode) {
			pr_err("%s: cannot allocate memory for attributes\n",
				__func__);
			ret = -ENOMEM;
			goto mode_sysfs_add_cpu_exit;
		}

		mode->kobj = kobject_create_and_add(
				msm_pm_sleep_mode_labels[i], cpu_kobj);
		if (!mode->kobj) {
			pr_err("%s: cannot create kobject\n", __func__);
			ret = -ENOMEM;
			goto mode_sysfs_add_cpu_exit;
		}

		for (k = 0, j = 0; k < MSM_PM_MODE_ATTR_NR; k++) {
			if ((k == MSM_PM_MODE_ATTR_IDLE) &&
				!msm_pm_sleep_modes[idx].idle_supported)
				continue;
			if ((k == MSM_PM_MODE_ATTR_SUSPEND) &&
			     !msm_pm_sleep_modes[idx].suspend_supported)
				continue;
			sysfs_attr_init(&mode->kas[j].ka.attr);
			mode->kas[j].cpu = cpu;
			mode->kas[j].ka.attr.mode = 0644;
			mode->kas[j].ka.show = msm_pm_mode_attr_show;
			mode->kas[j].ka.store = msm_pm_mode_attr_store;
			mode->kas[j].ka.attr.name = msm_pm_mode_attr_labels[k];
			mode->attrs[j] = &mode->kas[j].ka.attr;
			j++;
		}
		mode->attrs[j] = NULL;

		mode->attr_group.attrs = mode->attrs;
		ret = sysfs_create_group(mode->kobj, &mode->attr_group);
		if (ret) {
			pr_err("%s: cannot create kobject attribute group\n",
				__func__);
			goto mode_sysfs_add_cpu_exit;
		}
	}

	ret = 0;

mode_sysfs_add_cpu_exit:
	if (ret) {
		if (mode && mode->kobj)
			kobject_del(mode->kobj);
		kfree(mode);
	}

	return ret;
}

static int __init msm_pm_mode_sysfs_add(void)
{
	struct kobject *module_kobj;
	struct kobject *modes_kobj;
	unsigned int cpu;
	int ret;

	module_kobj = kset_find_obj(module_kset, KBUILD_MODNAME);
	if (!module_kobj) {
		pr_err("%s: cannot find kobject for module %s\n",
			__func__, KBUILD_MODNAME);
		ret = -ENOENT;
		goto mode_sysfs_add_exit;
	}

	modes_kobj = kobject_create_and_add("modes", module_kobj);
	if (!modes_kobj) {
		pr_err("%s: cannot create modes kobject\n", __func__);
		ret = -ENOMEM;
		goto mode_sysfs_add_exit;
	}

	for_each_possible_cpu(cpu) {
		ret = msm_pm_mode_sysfs_add_cpu(cpu, modes_kobj);
		if (ret)
			goto mode_sysfs_add_exit;
	}

	ret = 0;

mode_sysfs_add_exit:
	return ret;
}


static void msm_pm_config_hw_before_power_down(void)
{
	return;
}

static void msm_pm_config_hw_after_power_up(void)
{
	return;
}

static void msm_pm_config_hw_before_swfi(void)
{
	return;
}



#ifdef CONFIG_MSM_SLEEP_TIME_OVERRIDE
static int msm_pm_sleep_time_override;
module_param_named(sleep_time_override,
	msm_pm_sleep_time_override, int, S_IRUGO | S_IWUSR | S_IWGRP);
#endif

#define SCLK_HZ (32768)
#define MSM_PM_SLEEP_TICK_LIMIT (0x6DDD000)

static uint32_t msm_pm_max_sleep_time;

static int64_t msm_pm_convert_and_cap_time(int64_t time_ns, int64_t limit)
{
	do_div(time_ns, NSEC_PER_SEC / SCLK_HZ);
	return (time_ns > limit) ? limit : time_ns;
}

void msm_pm_set_max_sleep_time(int64_t max_sleep_time_ns)
{
	if (max_sleep_time_ns == 0) {
		msm_pm_max_sleep_time = 0;
	} else {
		msm_pm_max_sleep_time = (uint32_t)msm_pm_convert_and_cap_time(
			max_sleep_time_ns, MSM_PM_SLEEP_TICK_LIMIT);

		if (msm_pm_max_sleep_time == 0)
			msm_pm_max_sleep_time = 1;
	}

	if (msm_pm_debug_mask & MSM_PM_DEBUG_SUSPEND)
		pr_info("%s: Requested %lld ns Giving %u sclk ticks\n",
			__func__, max_sleep_time_ns, msm_pm_max_sleep_time);
}
EXPORT_SYMBOL(msm_pm_set_max_sleep_time);

static unsigned int *radio_info_addr;

void msm_pm_radio_info_init(unsigned int *addr)
{
	radio_info_addr = addr;
}
EXPORT_SYMBOL(msm_pm_radio_info_init);

void radio_stat_dump(void)
{
	int garbage = 0;
	int syncack = 0;

	if (radio_info_addr) {
		garbage = *(radio_info_addr+0x30);
		syncack = *(radio_info_addr+0x53);
	}
	printk(KERN_INFO "radio_info_stat: %d, %d\n", garbage, syncack);
}


static struct msm_rpmrs_limits *msm_pm_idle_rs_limits;
static bool msm_pm_use_qtimer;

static void msm_pm_swfi(void)
{
	msm_pm_config_hw_before_swfi();
	msm_arch_idle();
}


static void msm_pm_retention(void)
{
	int ret = 0;

	msm_pm_config_hw_before_swfi();
	ret = msm_spm_set_low_power_mode(MSM_SPM_MODE_POWER_RETENTION, false);
	WARN_ON(ret);
	msm_arch_idle();
	ret = msm_spm_set_low_power_mode(MSM_SPM_MODE_CLOCK_GATING, false);
	WARN_ON(ret);
}

#ifdef CONFIG_CACHE_L2X0
static inline bool msm_pm_l2x0_power_collapse(void)
{
	bool collapsed = 0;

	l2cc_suspend();
	collapsed = msm_pm_collapse();
	l2cc_resume();

	return collapsed;
}
#else
static inline bool msm_pm_l2x0_power_collapse(void)
{
	return msm_pm_collapse();
}
#endif

#ifdef CONFIG_APQ8064_ONLY 
extern uint32_t mpm_get_timetick(void);
extern uint32_t htc_dump_vdd_min_time(uint32_t suspend_start, uint32_t resume_start);

volatile uint32_t g_suspend_start_time = 0;
volatile uint32_t g_resume_start_time= 0;
#endif

static bool __ref msm_pm_spm_power_collapse(
	unsigned int cpu, bool from_idle, bool notify_rpm)
{
	void *entry;
	bool collapsed = 0;
	int ret;
	unsigned int saved_gic_cpu_ctrl;

	saved_gic_cpu_ctrl = readl_relaxed(MSM_QGIC_CPU_BASE + GIC_CPU_CTRL);
	mb();

	if (MSM_PM_DEBUG_POWER_COLLAPSE & msm_pm_debug_mask)
		pr_info("CPU%u: %s: notify_rpm %d\n",
			cpu, __func__, (int) notify_rpm);

	ret = msm_spm_set_low_power_mode(
			MSM_SPM_MODE_POWER_COLLAPSE, notify_rpm);
	WARN_ON(ret);

	entry = (!cpu || from_idle) ?
		msm_pm_collapse_exit : msm_secondary_startup;
	msm_pm_boot_config_before_pc(cpu, virt_to_phys(entry));

	if (MSM_PM_DEBUG_RESET_VECTOR & msm_pm_debug_mask)
		pr_info("CPU%u: %s: program vector to %p\n",
			cpu, __func__, entry);

#ifdef CONFIG_VFP
	vfp_pm_suspend();
#endif

	if (!from_idle && smp_processor_id() == 0) {
		if (suspend_console_deferred)
			suspend_console();

		msm_rpm_set_suspend_flag(true);

#ifdef CONFIG_MSM_WATCHDOG
		msm_watchdog_suspend(NULL);
#endif
		printk(KERN_INFO "[R] suspend end\n");
#ifdef CONFIG_APQ8064_ONLY 
		printk(KERN_INFO "[R][htc] START =======================");
		g_suspend_start_time = mpm_get_timetick();
		printk(KERN_INFO "suspend end at %d ticks\n", g_suspend_start_time);
#endif
	}

	init_cpu_foot_print(cpu, notify_rpm);

	collapsed = msm_pm_l2x0_power_collapse();

	set_cpu_foot_print(cpu, 0xa);
	clean_reset_vector_debug_info(cpu);
	if (!from_idle && smp_processor_id() == 0) {
#ifdef CONFIG_APQ8064_ONLY 
		volatile uint32_t sw_done_exit;

		g_resume_start_time = mpm_get_timetick();

		sw_done_exit = htc_dump_vdd_min_time(g_suspend_start_time, g_resume_start_time);
		printk(KERN_INFO "resume start at %d ticks\n", g_resume_start_time);

		printk(KERN_INFO "Sleep for %d ticks\n",
			g_resume_start_time - g_suspend_start_time);

		printk(KERN_INFO "************************************");
		printk(KERN_INFO "From \"rpm sw_done exit\" To \"krait resume\"=%d ticks\n",
			g_resume_start_time - sw_done_exit);
		printk(KERN_INFO "************************************");
		printk(KERN_INFO "[R][htc] END	=======================");
#endif
		printk(KERN_INFO "[R] resume start\n");
#ifdef CONFIG_TRACING_IRQ_PWR
		printk(KERN_INFO "%s : PWR KEY INT ENABLE : %d\n", __func__,  (__msm_gpio_get_intr_config(PWR_KEY_MSMz)&0x1));
#endif

#ifdef CONFIG_MSM_WATCHDOG
		msm_watchdog_resume(NULL);
#endif

		msm_rpm_set_suspend_flag(false);

		if (suspend_console_deferred)
			resume_console();
	}

	msm_pm_boot_config_after_pc(cpu);
	set_cpu_foot_print(cpu, 0xb);

	if (collapsed) {
#ifdef CONFIG_VFP
		vfp_pm_resume();
#endif
		cpu_init();
		writel(0xF0, MSM_QGIC_CPU_BASE + GIC_CPU_PRIMASK);
		writel_relaxed(saved_gic_cpu_ctrl,
				MSM_QGIC_CPU_BASE + GIC_CPU_CTRL);
		mb();
		local_fiq_enable();
	}

	if (MSM_PM_DEBUG_POWER_COLLAPSE & msm_pm_debug_mask)
		pr_info("CPU%u: %s: msm_pm_collapse returned, collapsed %d\n",
			cpu, __func__, collapsed);

	if (!from_idle && (MSM_PM_DEBUG_RPM_TIMESTAMP & msm_pm_debug_mask))
		msm_rpm_print_sleep_tick();

	ret = msm_spm_set_low_power_mode(MSM_SPM_MODE_CLOCK_GATING, false);
	WARN_ON(ret);
	return collapsed;
}

static bool msm_pm_power_collapse_standalone(bool from_idle)
{
	unsigned int cpu = smp_processor_id();
	unsigned int avsdscr_setting;
	bool collapsed;

	avsdscr_setting = avs_get_avsdscr();
	avs_disable();
	collapsed = msm_pm_spm_power_collapse(cpu, from_idle, false);
	avs_reset_delays(avsdscr_setting);
	return collapsed;
}

static bool msm_pm_power_collapse(bool from_idle)
{
	unsigned int cpu = smp_processor_id();
	unsigned long saved_acpuclk_rate;
	unsigned int avsdscr_setting;
	bool collapsed;
	struct clk *clk, *parent_clk;
	int blk_xo_vddmin_count = 0;

	if(board_mfg_mode() == 9)
		return true;

	if (MSM_PM_DEBUG_POWER_COLLAPSE & msm_pm_debug_mask)
		pr_info("CPU%u: %s: idle %d\n",
			cpu, __func__, (int)from_idle);

	if (smp_processor_id() == 0) {
		if (((!from_idle) && (MSM_PM_DEBUG_CLOCK & msm_pm_debug_mask)) ||
			((from_idle) && (MSM_PM_DEBUG_IDLE_CLOCK & msm_pm_debug_mask))) {
			spin_lock(&clk_enable_list_lock);
			list_for_each_entry(clk, &clk_enable_list, enable_list) {
				if (is_xo_src(clk)) {
					blk_xo_vddmin_count++;
					parent_clk = clk->ops->get_parent?clk->ops->get_parent(clk):NULL;
					if (clk->vdd_class)
						pr_info("%s not off block xo vdig level %ld, parent clk: %s\n",
								clk->dbg_name, clk->vdd_class->cur_level,
								parent_clk?parent_clk->dbg_name:"none");
					else
						pr_info("%s not off block xo vdig level (none), parent clk: %s\n",
								clk->dbg_name, parent_clk?parent_clk->dbg_name:"none");
				}
			}
			spin_unlock(&clk_enable_list_lock);
			if (blk_xo_vddmin_count)
				pr_info("%d clks are on that block xo or vddmin\n", blk_xo_vddmin_count);
		}
		if ((!from_idle) && (MSM_PM_DEBUG_RPM_STAT & msm_pm_debug_mask)){
			msm_xo_print_voters_suspend();
			msm_rpm_dump_stat();
		}
		if (!from_idle)
			radio_stat_dump();

	}

	msm_pm_config_hw_before_power_down();
	if (MSM_PM_DEBUG_POWER_COLLAPSE & msm_pm_debug_mask)
		pr_info("CPU%u: %s: pre power down\n", cpu, __func__);

	avsdscr_setting = avs_get_avsdscr();
	avs_disable();

	if (cpu_online(cpu))
		saved_acpuclk_rate = acpuclk_power_collapse();
	else {
		saved_acpuclk_rate = 0;
	}

	if ((!from_idle) && (MSM_PM_DEBUG_CLOCK & msm_pm_debug_mask))
		pr_info("CPU%u: %s: change clock rate (old rate = %lu)\n",
			cpu, __func__, saved_acpuclk_rate);

	collapsed = msm_pm_spm_power_collapse(cpu, from_idle, true);

	if (cpu_online(cpu)) {
		if ((!from_idle) && (MSM_PM_DEBUG_RPM_STAT & msm_pm_debug_mask))
			msm_rpm_dump_stat();
		if (!from_idle)
			radio_stat_dump();
		if ((!from_idle) && (MSM_PM_DEBUG_CLOCK & msm_pm_debug_mask))
			pr_info("CPU%u: %s: restore clock rate to %lu\n",
				cpu, __func__, saved_acpuclk_rate);
		if (acpuclk_set_rate(cpu, saved_acpuclk_rate, SETRATE_PC) < 0)
			pr_warn("CPU%u: %s: failed to restore clock rate(%lu)\n",
				cpu, __func__, saved_acpuclk_rate);
	} else {
		unsigned int gic_dist_enabled;
		unsigned int gic_dist_pending;
		gic_dist_enabled = readl_relaxed(
				MSM_QGIC_DIST_BASE + GIC_DIST_ENABLE_CLEAR);
		gic_dist_pending = readl_relaxed(
				MSM_QGIC_DIST_BASE + GIC_DIST_PENDING_SET);
		mb();
		gic_dist_pending &= gic_dist_enabled;

		if (gic_dist_pending)
			pr_err("CPU %d interrupted during hotplug.Pending int 0x%x\n",
					cpu, gic_dist_pending);
	}


	avs_reset_delays(avsdscr_setting);
	msm_pm_config_hw_after_power_up();
	if (MSM_PM_DEBUG_POWER_COLLAPSE & msm_pm_debug_mask)
		pr_info("CPU%u: %s: post power up\n", cpu, __func__);

	if (MSM_PM_DEBUG_POWER_COLLAPSE & msm_pm_debug_mask)
		pr_info("CPU%u: %s: return\n", cpu, __func__);
	return collapsed;
}

static void msm_pm_qtimer_available(void)
{
	if (machine_is_msm8974())
		msm_pm_use_qtimer = true;
}

static int64_t msm_pm_timer_enter_idle(void)
{
	if (msm_pm_use_qtimer)
		return ktime_to_ns(tick_nohz_get_sleep_length());

	return msm_timer_enter_idle();
}

static void msm_pm_timer_exit_idle(bool timer_halted)
{
	if (msm_pm_use_qtimer)
		return;

	msm_timer_exit_idle((int) timer_halted);
}

static int64_t msm_pm_timer_enter_suspend(int64_t *period)
{
	int time = 0;

	if (msm_pm_use_qtimer)
		return sched_clock();

	time = msm_timer_get_sclk_time(period);
	if (!time)
		pr_err("%s: Unable to read sclk.\n", __func__);

	return time;
}

static int64_t msm_pm_timer_exit_suspend(int64_t time, int64_t period)
{
	if (msm_pm_use_qtimer)
		return sched_clock() - time;

	if (time != 0) {
		int64_t end_time = msm_timer_get_sclk_time(NULL);
		if (end_time != 0) {
			time = end_time - time;
			if (time < 0)
				time += period;
		} else
			time = 0;
	}

	return time;
}


void arch_idle(void)
{
	return;
}

int msm_pm_idle_prepare(struct cpuidle_device *dev,
		struct cpuidle_driver *drv, int index)
{
	uint32_t latency_us;
	uint32_t sleep_us;
	int i;
	unsigned int power_usage = -1;
	int ret = MSM_PM_SLEEP_MODE_NOT_SELECTED;

	latency_us = (uint32_t) pm_qos_request(PM_QOS_CPU_DMA_LATENCY);
	sleep_us = (uint32_t) ktime_to_ns(tick_nohz_get_sleep_length());
	sleep_us = DIV_ROUND_UP(sleep_us, 1000);

	for (i = 0; i < dev->state_count; i++) {
		struct cpuidle_state *state = &drv->states[i];
		struct cpuidle_state_usage *st_usage = &dev->states_usage[i];
		enum msm_pm_sleep_mode mode;
		bool allow;
		struct msm_rpmrs_limits *rs_limits = NULL;
		uint32_t power;
		int idx;

		mode = (enum msm_pm_sleep_mode) cpuidle_get_statedata(st_usage);
		idx = MSM_PM_MODE(dev->cpu, mode);

		allow = msm_pm_sleep_modes[idx].idle_enabled &&
				msm_pm_sleep_modes[idx].idle_supported;

		switch (mode) {
		case MSM_PM_SLEEP_MODE_POWER_COLLAPSE:
		case MSM_PM_SLEEP_MODE_RETENTION:
			if (!allow)
				break;

			if (num_online_cpus() > 1) {
				allow = false;
				break;
			}
			

		case MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE:
			if (!allow)
				break;

			if (!dev->cpu &&
				msm_rpm_local_request_is_outstanding()) {
				allow = false;
				break;
			}
			

		case MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT:
			if (!allow)
				break;

			if (pm_sleep_ops.lowest_limits)
				rs_limits = pm_sleep_ops.lowest_limits(true,
						mode, latency_us, sleep_us,
						&power);

			if (MSM_PM_DEBUG_IDLE & msm_pm_debug_mask)
				pr_info("CPU%u: %s: %s, latency %uus, "
					"sleep %uus, limit %p\n",
					dev->cpu, __func__, state->desc,
					latency_us, sleep_us, rs_limits);

			if ((MSM_PM_DEBUG_IDLE_LIMITS & msm_pm_debug_mask) &&
					rs_limits)
				pr_info("CPU%u: %s: limit %p: "
					"pxo %d, l2_cache %d, "
					"vdd_mem %d, vdd_dig %d\n",
					dev->cpu, __func__, rs_limits,
					rs_limits->pxo,
					rs_limits->l2_cache,
					rs_limits->vdd_mem,
					rs_limits->vdd_dig);

			if (!rs_limits)
				allow = false;
			break;

		default:
			allow = false;
			break;
		}

		if (MSM_PM_DEBUG_IDLE & msm_pm_debug_mask)
			pr_info("CPU%u: %s: allow %s: %d\n",
				dev->cpu, __func__, state->desc, (int)allow);

		if (allow) {
			if (power < power_usage) {
				power_usage = power;
				ret = mode;
			}

			if (MSM_PM_SLEEP_MODE_POWER_COLLAPSE == mode)
				msm_pm_idle_rs_limits = rs_limits;
		}
	}

	return ret;
}

static char *gpio_sleep_status_info;

int print_gpio_buffer(struct seq_file *m)
{
	if (gpio_sleep_status_info)
		seq_printf(m, gpio_sleep_status_info);
	else
		seq_printf(m, "Device haven't suspended yet!\n");

	return 0;
}
EXPORT_SYMBOL(print_gpio_buffer);

int free_gpio_buffer()
{
	kfree(gpio_sleep_status_info);
	gpio_sleep_status_info = NULL;

	return 0;
}
EXPORT_SYMBOL(free_gpio_buffer);

static char *vreg_sleep_status_info;

int print_vreg_buffer(struct seq_file *m)
{
	if (vreg_sleep_status_info)
		seq_printf(m, vreg_sleep_status_info);
	else
		seq_printf(m, "Device haven't suspended yet!\n");

	return 0;
}
EXPORT_SYMBOL(print_vreg_buffer);

int free_vreg_buffer(void)
{
	kfree(vreg_sleep_status_info);
	vreg_sleep_status_info = NULL;

	return 0;
}
EXPORT_SYMBOL(free_vreg_buffer);

static char *pmic_reg_sleep_status_info;

int print_pmic_reg_buffer(struct seq_file *m)
{
	if (pmic_reg_sleep_status_info)
		seq_printf(m, pmic_reg_sleep_status_info);
	else
		seq_printf(m, "Device haven't suspended yet!\n");

	return 0;
}
EXPORT_SYMBOL(print_pmic_reg_buffer);

int free_pmic_reg_buffer(void)
{
	kfree(pmic_reg_sleep_status_info);
	pmic_reg_sleep_status_info = NULL;

	return 0;
}
EXPORT_SYMBOL(free_pmic_reg_buffer);

int msm_pm_idle_enter(enum msm_pm_sleep_mode sleep_mode)
{
	int64_t time;
	int exit_stat;
	uint64_t xo_shutdown_time, vdd_min_time;

	if (MSM_PM_DEBUG_IDLE & msm_pm_debug_mask)
		pr_info("CPU%u: %s: mode %d\n",
			smp_processor_id(), __func__, sleep_mode);

	time = ktime_to_ns(ktime_get());

	switch (sleep_mode) {
	case MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT:
		msm_pm_swfi();
		exit_stat = MSM_PM_STAT_IDLE_WFI;
		break;

	case MSM_PM_SLEEP_MODE_RETENTION:
		msm_pm_retention();
		exit_stat = MSM_PM_STAT_RETENTION;
		break;

	case MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE:
		msm_pm_power_collapse_standalone(true);
		exit_stat = MSM_PM_STAT_IDLE_STANDALONE_POWER_COLLAPSE;
		break;

	case MSM_PM_SLEEP_MODE_POWER_COLLAPSE: {
		int64_t timer_expiration = 0;
		bool timer_halted = false;
		uint32_t sleep_delay;
		int ret = -ENODEV;
		int notify_rpm =
			(sleep_mode == MSM_PM_SLEEP_MODE_POWER_COLLAPSE);
		int collapsed;

		timer_expiration = msm_pm_timer_enter_idle();

		sleep_delay = (uint32_t) msm_pm_convert_and_cap_time(
			timer_expiration, MSM_PM_SLEEP_TICK_LIMIT);
		if (sleep_delay == 0) 
			sleep_delay = 1;

		if (MSM_PM_DEBUG_IDLE_CLK & msm_pm_debug_mask)
			clock_debug_print_enabled();

		if (pm_sleep_ops.enter_sleep)
			ret = pm_sleep_ops.enter_sleep(sleep_delay,
					msm_pm_idle_rs_limits,
					true, notify_rpm);
		if (!ret) {
			xo_shutdown_time = msm_rpm_get_xo_time();
			vdd_min_time = msm_rpm_get_vdd_time();

			collapsed = msm_pm_power_collapse(true);
			timer_halted = true;

			if (pm_sleep_ops.exit_sleep)
				pm_sleep_ops.exit_sleep(msm_pm_idle_rs_limits,
						true, notify_rpm, collapsed);

			xo_shutdown_time = msm_rpm_get_xo_time() - xo_shutdown_time;
			if (xo_shutdown_time > 0)
				msm_pm_add_stat(MSM_PM_STAT_IDLE_POWER_COLLAPSE_XO_SHUTDOWN, xo_shutdown_time);
			vdd_min_time = msm_rpm_get_vdd_time() - vdd_min_time;
			if (vdd_min_time > 0)
				msm_pm_add_stat(MSM_PM_STAT_IDLE_POWER_COLLAPSE_VDD_MIN, vdd_min_time);
		}
		msm_pm_timer_exit_idle(timer_halted);
		exit_stat = MSM_PM_STAT_IDLE_POWER_COLLAPSE;
		break;
	}

	case MSM_PM_SLEEP_MODE_NOT_SELECTED:
		goto cpuidle_enter_bail;
		break;

	default:
		__WARN();
		goto cpuidle_enter_bail;
		break;
	}

	time = ktime_to_ns(ktime_get()) - time;
	msm_pm_add_stat(exit_stat, time);
	do_div(time, 1000);
	if ((get_kernel_flag() & KERNEL_FLAG_PM_MONITOR) ||
		(!(get_kernel_flag() & KERNEL_FLAG_TEST_PWR_SUPPLY) && (!get_tamper_sf())))
		htc_idle_stat_add(sleep_mode, (u32)time);

	return (int) time;

cpuidle_enter_bail:
	return 0;
}

static struct msm_pm_sleep_status_data *msm_pm_slp_sts;

static DEFINE_PER_CPU_SHARED_ALIGNED(enum msm_pm_sleep_mode,
		msm_pm_last_slp_mode);

bool msm_pm_verify_cpu_pc(unsigned int cpu)
{
	enum msm_pm_sleep_mode mode = per_cpu(msm_pm_last_slp_mode, cpu);

#ifndef CONFIG_ARCH_MSM8X60
	if (msm_pm_slp_sts) {
		int acc_sts = __raw_readl(msm_pm_slp_sts->base_addr
						+ cpu * msm_pm_slp_sts->cpu_offset);

		if ((acc_sts & msm_pm_slp_sts->mask) &&
			((mode == MSM_PM_SLEEP_MODE_POWER_COLLAPSE) ||
			(mode == MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)))
			return true;
	}
#else
	if (msm_pm_slp_sts)
		if ((mode == MSM_PM_SLEEP_MODE_POWER_COLLAPSE) ||
			(mode == MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE))
				return false;
#endif

	return false;
}

void msm_pm_cpu_enter_lowpower(unsigned int cpu)
{
	int i;
	bool allow[MSM_PM_SLEEP_MODE_NR];

	for (i = 0; i < MSM_PM_SLEEP_MODE_NR; i++) {
		struct msm_pm_platform_data *mode;

		mode = &msm_pm_sleep_modes[MSM_PM_MODE(cpu, i)];
		allow[i] = mode->suspend_supported && mode->suspend_enabled;
	}

	if (MSM_PM_DEBUG_HOTPLUG & msm_pm_debug_mask)
		pr_notice("CPU%u: %s: shutting down cpu\n", cpu, __func__);

	if (allow[MSM_PM_SLEEP_MODE_POWER_COLLAPSE]) {
		per_cpu(msm_pm_last_slp_mode, cpu)
			= MSM_PM_SLEEP_MODE_POWER_COLLAPSE;
		msm_pm_power_collapse(false);
	} else if (allow[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE]) {
		per_cpu(msm_pm_last_slp_mode, cpu)
			= MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE;
		msm_pm_power_collapse_standalone(false);
	} else if (allow[MSM_PM_SLEEP_MODE_RETENTION]) {
		per_cpu(msm_pm_last_slp_mode, cpu)
			= MSM_PM_SLEEP_MODE_RETENTION;
		msm_pm_retention();
	} else {
		per_cpu(msm_pm_last_slp_mode, cpu)
			= MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT;
		msm_pm_swfi();
	}
}

int msm_pm_wait_cpu_shutdown(unsigned int cpu)
{

	int timeout = 10;

	if (!msm_pm_slp_sts)
		return 0;

	while (timeout--) {


		int acc_sts = __raw_readl(msm_pm_slp_sts->base_addr
					+ cpu * msm_pm_slp_sts->cpu_offset);
		mb();

		if (acc_sts & msm_pm_slp_sts->mask)
			return 0;

		usleep(100);
	}
	pr_warn("%s(): Timed out waiting for CPU %u SPM to enter sleep state",
			__func__, cpu);
	return -EBUSY;
}

static int msm_pm_enter(suspend_state_t state)
{
	bool allow[MSM_PM_SLEEP_MODE_NR];
	int i;
	int64_t period = 0;
	int64_t time = msm_pm_timer_enter_suspend(&period);
	int curr_len = 0;
	uint64_t xo_shutdown_time, vdd_min_time;
	int collapsed;

#if defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY)
	dlp_ext_buck_en(0);
#endif
	if (MSM_PM_DEBUG_SUSPEND & msm_pm_debug_mask)
		pr_info("%s\n", __func__);

	if (MSM_PM_DEBUG_GPIO & msm_pm_debug_mask) {
		if (gpio_sleep_status_info) {
			memset(gpio_sleep_status_info, 0,
				sizeof(gpio_sleep_status_info));
		} else {
			gpio_sleep_status_info = kmalloc(25000, GFP_ATOMIC);
			if (!gpio_sleep_status_info) {
				pr_err("[PM] kmalloc memory failed in %s\n",
					__func__);

			}
		}

		curr_len = msm_dump_gpios(NULL, curr_len,
						gpio_sleep_status_info);
		curr_len = pm8xxx_dump_gpios(NULL, curr_len,
						gpio_sleep_status_info);
		curr_len = pm8xxx_dump_mpp(NULL, curr_len,
						gpio_sleep_status_info);

	}

	if (MSM_PM_DEBUG_VREG & msm_pm_debug_mask) {
		curr_len = 0;
		if (vreg_sleep_status_info) {
			memset(vreg_sleep_status_info, 0,
				sizeof(vreg_sleep_status_info));
		} else {
			vreg_sleep_status_info = kmalloc(25000, GFP_ATOMIC);
			if (!vreg_sleep_status_info) {
				pr_err("kmalloc memory failed in %s\n",
					__func__);

			}
		}
		curr_len = pmic_vreg_dump(vreg_sleep_status_info, curr_len);
	}

	
	if (MSM_PM_DEBUG_VREG & msm_pm_debug_mask) {
		curr_len = 0;
		if (pmic_reg_sleep_status_info) {
			memset(pmic_reg_sleep_status_info, 0,
				sizeof(pmic_reg_sleep_status_info));
		} else {
			pmic_reg_sleep_status_info = kmalloc(75000, GFP_ATOMIC);
			if (!pmic_reg_sleep_status_info) {
				pr_err("kmalloc memory failed in %s\n",
					__func__);

			}
		}
		curr_len = pmic_suspend_reg_dump(pmic_reg_sleep_status_info, curr_len);
	}
	

	if (smp_processor_id()) {
		__WARN();
		goto enter_exit;
	}


	for (i = 0; i < MSM_PM_SLEEP_MODE_NR; i++) {
		struct msm_pm_platform_data *mode;

		mode = &msm_pm_sleep_modes[MSM_PM_MODE(0, i)];
		allow[i] = mode->suspend_supported && mode->suspend_enabled;
	}

	if (allow[MSM_PM_SLEEP_MODE_POWER_COLLAPSE]) {
		struct msm_rpmrs_limits *rs_limits = NULL;
		int ret = -ENODEV;
		uint32_t power;

		if (MSM_PM_DEBUG_SUSPEND & msm_pm_debug_mask)
			pr_info("%s: power collapse\n", __func__);

		clock_debug_print_enabled();

		
		keep_dig_voltage_low_in_idle(false);

#ifdef CONFIG_MSM_SLEEP_TIME_OVERRIDE
		if (msm_pm_sleep_time_override > 0) {
			int64_t ns = NSEC_PER_SEC *
				(int64_t) msm_pm_sleep_time_override;
			msm_pm_set_max_sleep_time(ns);
			msm_pm_sleep_time_override = 0;
		}
#endif 
		if (pm_sleep_ops.lowest_limits)
			rs_limits = pm_sleep_ops.lowest_limits(false,
					MSM_PM_SLEEP_MODE_POWER_COLLAPSE, -1,
					-1, &power);

		if ((MSM_PM_DEBUG_SUSPEND_LIMITS & msm_pm_debug_mask) &&
				rs_limits)
			pr_info("%s: limit %p: pxo %d, l2_cache %d, "
				"vdd_mem %d, vdd_dig %d\n",
				__func__, rs_limits,
				rs_limits->pxo, rs_limits->l2_cache,
				rs_limits->vdd_mem, rs_limits->vdd_dig);

		if (rs_limits) {
			if (pm_sleep_ops.enter_sleep)
				ret = pm_sleep_ops.enter_sleep(
						msm_pm_max_sleep_time,
						rs_limits, false, true);
			if (!ret) {
				xo_shutdown_time = msm_rpm_get_xo_time();
				vdd_min_time = msm_rpm_get_vdd_time();

				collapsed = msm_pm_power_collapse(false);
				if (pm_sleep_ops.exit_sleep) {
					pm_sleep_ops.exit_sleep(rs_limits,
						false, true, collapsed);
				}

				xo_shutdown_time = msm_rpm_get_xo_time() - xo_shutdown_time;
				if (xo_shutdown_time > 0)
					msm_pm_add_stat(MSM_PM_STAT_SUSPEND_XO_SHUTDOWN, xo_shutdown_time);

				vdd_min_time = msm_rpm_get_vdd_time() - vdd_min_time;
				if (vdd_min_time > 0)
					msm_pm_add_stat(MSM_PM_STAT_SUSPEND_VDD_MIN, vdd_min_time);
			}
		} else {
			pr_err("%s: cannot find the lowest power limit\n",
				__func__);
		}

		
		keep_dig_voltage_low_in_idle(true);

		time = msm_pm_timer_exit_suspend(time, period);
		msm_pm_add_stat(MSM_PM_STAT_SUSPEND, time);
	} else if (allow[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE]) {
		if (MSM_PM_DEBUG_SUSPEND & msm_pm_debug_mask)
			pr_info("%s: standalone power collapse\n", __func__);
		msm_pm_power_collapse_standalone(false);
	} else if (allow[MSM_PM_SLEEP_MODE_RETENTION]) {
		if (MSM_PM_DEBUG_SUSPEND & msm_pm_debug_mask)
			pr_info("%s: retention\n", __func__);
		msm_pm_retention();
	} else if (allow[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT]) {
		if (MSM_PM_DEBUG_SUSPEND & msm_pm_debug_mask)
			pr_info("%s: swfi\n", __func__);
		if (suspend_console_deferred)
			suspend_console();

#ifdef CONFIG_MSM_WATCHDOG
		msm_watchdog_suspend(NULL);
#endif

		printk(KERN_INFO "[R] suspend end\n");
		msm_pm_swfi();
		printk(KERN_INFO "[R] resume start\n");

#ifdef CONFIG_MSM_WATCHDOG
		msm_watchdog_resume(NULL);
#endif

		if (suspend_console_deferred)
			resume_console();

		if (MSM_PM_DEBUG_RPM_TIMESTAMP & msm_pm_debug_mask)
			msm_rpm_print_sleep_tick();
	}


enter_exit:

#if defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY)
	dlp_ext_buck_en(1);
#endif
	if (MSM_PM_DEBUG_SUSPEND & msm_pm_debug_mask)
		pr_info("%s: return\n", __func__);

	return 0;
}

static struct platform_suspend_ops msm_pm_ops = {
	.enter = msm_pm_enter,
	.valid = suspend_valid_only_mem,
};


#define BOOT_LOCK_TIMEOUT_NORMAL      (60 * HZ)
#define BOOT_LOCK_TIMEOUT_SHORT      (10 * HZ)
static void do_expire_boot_lock(struct work_struct *work)
{
	enable_hlt();
	pr_info("Release 'boot-time' no_halt_lock\n");
}
static DECLARE_DELAYED_WORK(work_expire_boot_lock, do_expire_boot_lock);

static void __init boot_lock_nohalt(void)
{
	int nohalt_timeout;

	
	switch (board_mfg_mode()) {
	case 0:  
	case 1:  
	case 2:  
		nohalt_timeout = BOOT_LOCK_TIMEOUT_NORMAL;
		break;
	case 3:  
	case 4:  
	case 5:  
	default:
		nohalt_timeout = BOOT_LOCK_TIMEOUT_SHORT;
		break;
	}
	disable_hlt();
	schedule_delayed_work(&work_expire_boot_lock, nohalt_timeout);
	pr_info("Acquire 'boot-time' no_halt_lock %ds\n", nohalt_timeout / HZ);
}

void __init msm_pm_init_sleep_status_data(
		struct msm_pm_sleep_status_data *data)
{
	msm_pm_slp_sts = data;
}

void msm_pm_set_sleep_ops(struct msm_pm_sleep_ops *ops)
{
	if (ops)
		pm_sleep_ops = *ops;
}

static int __init msm_pm_init(void)
{
	pgd_t *pc_pgd;
	pmd_t *pmd;
	unsigned long pmdval;
	enum msm_pm_time_stats_id enable_stats[] = {
		MSM_PM_STAT_IDLE_WFI,
		MSM_PM_STAT_RETENTION,
		MSM_PM_STAT_IDLE_STANDALONE_POWER_COLLAPSE,
		MSM_PM_STAT_IDLE_POWER_COLLAPSE,
		MSM_PM_STAT_IDLE_POWER_COLLAPSE_XO_SHUTDOWN,
		MSM_PM_STAT_IDLE_POWER_COLLAPSE_VDD_MIN,
		MSM_PM_STAT_SUSPEND,
		MSM_PM_STAT_SUSPEND_XO_SHUTDOWN,
		MSM_PM_STAT_SUSPEND_VDD_MIN,
	};
	unsigned long exit_phys;
	unsigned int addr;

	
	pc_pgd = pgd_alloc(&init_mm);
	if (!pc_pgd)
		return -ENOMEM;

	exit_phys = virt_to_phys(msm_pm_collapse_exit);

	pmd = pmd_offset(pud_offset(pc_pgd + pgd_index(exit_phys),exit_phys),
					exit_phys);
	pmdval = (exit_phys & PGDIR_MASK) |
		     PMD_TYPE_SECT | PMD_SECT_AP_WRITE;
	pmd[0] = __pmd(pmdval);
	pmd[1] = __pmd(pmdval + (1 << (PGDIR_SHIFT - 1)));

	msm_saved_state_phys =
		allocate_contiguous_ebi_nomap(CPU_SAVED_STATE_SIZE *
					      num_possible_cpus(), 4);
	if (!msm_saved_state_phys)
		return -ENOMEM;
	msm_saved_state = ioremap_nocache(msm_saved_state_phys,
					  CPU_SAVED_STATE_SIZE *
					  num_possible_cpus());
	if (!msm_saved_state)
		return -ENOMEM;

	pmd[2] = __pmd(pmdval + (2 << (PGDIR_SHIFT - 1)));
	flush_pmd_entry(pmd);
	msm_pm_pc_pgd = virt_to_phys(pc_pgd);
	clean_caches((unsigned long)&msm_pm_pc_pgd, sizeof(msm_pm_pc_pgd),
		     virt_to_phys(&msm_pm_pc_pgd));

	msm_pm_mode_sysfs_add();
	msm_pm_add_stats(enable_stats, ARRAY_SIZE(enable_stats));

	suspend_set_ops(&msm_pm_ops);
	boot_lock_nohalt();
	msm_pm_qtimer_available();
	msm_cpuidle_init();
	if (get_kernel_flag() & KERNEL_FLAG_GPIO_DUMP) {
		suspend_console_deferred = 1;
		console_suspend_enabled = 0;
		msm_pm_debug_mask |= (MSM_PM_DEBUG_GPIO | MSM_PM_DEBUG_VREG);
	}

	store_pm_boot_entry_addr();
	get_pm_boot_vector_symbol_address(&addr);
	pr_info("%s: msm_pm_boot_vector 0x%x", __func__, addr);
	store_pm_boot_vector_addr(addr);

	keep_dig_voltage_low_in_idle(true);

	if(board_mfg_mode() == 6 || board_mfg_mode() == 8) {
		static struct pm_qos_request pm_qos_req_dma;

		pm_qos_add_request(&pm_qos_req_dma,
			PM_QOS_CPU_DMA_LATENCY, 100);
	}

	return 0;
}

late_initcall(msm_pm_init);
