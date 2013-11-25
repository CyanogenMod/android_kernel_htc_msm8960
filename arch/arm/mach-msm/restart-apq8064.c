/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
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
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/reboot.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/cpu.h>
#include <linux/interrupt.h>
#include <linux/mfd/pmic8058.h>
#include <linux/mfd/pmic8901.h>
#include <linux/mfd/pm8xxx/misc.h>
#include <linux/gpio.h>
#include <linux/console.h>

#include <asm/cacheflush.h>
#include <asm/mach-types.h>

#include <mach/msm_iomap.h>
#include <mach/restart.h>
#include <mach/socinfo.h>
#include <mach/irqs.h>
#include <mach/scm.h>
#include <mach/msm_watchdog.h>
#include <mach/board_htc.h>

#include "timer.h"
#include <mach/htc_restart_handler.h>

#define WDT0_RST	0x38
#define WDT0_EN		0x40
#define WDT0_BARK_TIME	0x4C
#define WDT0_BITE_TIME	0x5C

#define PSHOLD_CTL_SU (MSM_TLMM_BASE + 0x820)

#define RESTART_REASON_ADDR 0xF00
#define DLOAD_MODE_ADDR     0x0

#define SCM_IO_DISABLE_PMIC_ARBITER	1

static int restart_mode;
int ramdump_source=0;

int pmic_reset_irq;
static void __iomem *msm_tmr0_base;

static int in_panic;
static int panic_prep_restart(struct notifier_block *this,
			      unsigned long event, void *ptr)
{
	in_panic = 1;
	return NOTIFY_DONE;
}
static struct notifier_block panic_blk = {
	.notifier_call	= panic_prep_restart,
};
#ifdef CONFIG_MSM_DLOAD_MODE
static void *dload_mode_addr;

static int dload_set(const char *val, struct kernel_param *kp);
static int download_mode = 1;
module_param_call(download_mode, dload_set, param_get_int,
			&download_mode, 0644);

static void set_dload_mode(int on)
{
	if (dload_mode_addr) {
		__raw_writel(on ? 0xE47B337D : 0, dload_mode_addr);
		__raw_writel(on ? 0xCE14091A : 0,
		       dload_mode_addr + sizeof(unsigned int));
		mb();
	}
}

static int dload_set(const char *val, struct kernel_param *kp)
{
	int ret;
	int old_val = download_mode;

	ret = param_set_int(val, kp);

	if (ret)
		return ret;

	
	if (download_mode >> 1) {
		download_mode = old_val;
		return -EINVAL;
	}

	set_dload_mode(download_mode);

	return 0;
}
#else
#define set_dload_mode(x) do {} while (0)
#endif

void msm_set_restart_mode(int mode)
{
	restart_mode = mode;
}
EXPORT_SYMBOL(msm_set_restart_mode);

static unsigned mdm2ap_errfatal_restart;
static unsigned ap2mdm_pmic_reset_n_gpio = -1;
static unsigned ap2qsc_pmic_pwr_en_value;
static unsigned ap2qsc_pmic_pwr_en_gpio = -1;
static unsigned ap2qsc_pmic_soft_reset_gpio = -1;
#ifdef CONFIG_QSC_MODEM
void set_qsc_drv_is_ready(int is_ready);
#endif

void set_mdm2ap_errfatal_restart_flag(unsigned flag)
{
	mdm2ap_errfatal_restart = flag;
}

void register_ap2mdm_pmic_reset_n_gpio(unsigned gpio)
{
	ap2mdm_pmic_reset_n_gpio = gpio;
}

void register_ap2qsc_pmic_pwr_en_gpio(unsigned gpio, unsigned enable_value)
{
	ap2qsc_pmic_pwr_en_gpio = gpio;
	ap2qsc_pmic_pwr_en_value = enable_value;
}

void register_ap2qsc_pmic_soft_reset_gpio(unsigned gpio)
{
	ap2qsc_pmic_soft_reset_gpio = gpio;
}

#define MDM_HOLD_TIME			2000
#define MDM_SELF_REFRESH_TIME		3000
#define MDM_MODEM_DELTA		1000
static void turn_off_mdm_power(void)
{
	int i;

	
	if (gpio_is_valid(ap2mdm_pmic_reset_n_gpio) && !mdm2ap_errfatal_restart) {
		printk(KERN_CRIT "Powering off MDM...\n");
		gpio_direction_output(ap2mdm_pmic_reset_n_gpio, 0);
		for (i = MDM_HOLD_TIME; i > 0; i -= MDM_MODEM_DELTA) {
			pet_watchdog();
			mdelay(MDM_MODEM_DELTA);
		}
		printk(KERN_CRIT "Power off MDM down...\n");
	}
}

static void turn_off_qsc_power(void)
{
	if (gpio_is_valid(ap2qsc_pmic_pwr_en_gpio))
	{
		if (gpio_is_valid(ap2qsc_pmic_soft_reset_gpio))
		{
			pr_info("Discharging QSC...\n");
#ifdef CONFIG_QSC_MODEM
			set_qsc_drv_is_ready(0);
#endif
			gpio_direction_output(ap2qsc_pmic_soft_reset_gpio, 1);

			mdelay(500);
		}

		printk(KERN_CRIT "Powering off QSC...\n");
		gpio_direction_output(ap2qsc_pmic_pwr_en_gpio, !ap2qsc_pmic_pwr_en_value);
		pet_watchdog();
		mdelay(1000);
		pet_watchdog();
	}
}

static void wait_mdm_enter_self_refresh(void)
{
	int i;
	printk(KERN_CRIT "Waiting MDM enter self refresh...\n");
	for (i = MDM_SELF_REFRESH_TIME; i > 0; i -= MDM_MODEM_DELTA) {
		pet_watchdog();
		mdelay(MDM_MODEM_DELTA);
	}
}

static void msm_flush_console(void)
{
	unsigned long flags;

	printk("\n");
	printk(KERN_EMERG "[K] Restarting %s\n", linux_banner);
	if (console_trylock()) {
		console_unlock();
		return;
	}

	mdelay(50);

	local_irq_save(flags);

	if (console_trylock())
		printk(KERN_EMERG "[K] restart: Console was locked! Busting\n");
	else
		printk(KERN_EMERG "[K] restart: Console was locked!\n");
	console_unlock();

	local_irq_restore(flags);
}

static void __msm_power_off(int lower_pshold)
{
	turn_off_mdm_power();	
	turn_off_qsc_power();

	printk(KERN_CRIT "Powering off the SoC\n");
#ifdef CONFIG_MSM_DLOAD_MODE
	set_dload_mode(0);
#endif
	pm8xxx_reset_pwr_off(0);

	if (lower_pshold) {
		__raw_writel(0, PSHOLD_CTL_SU);
		mdelay(10000);
		printk(KERN_ERR "Powering off has failed\n");
	}
	return;
}

static void msm_power_off(void)
{
	
	__msm_power_off(1);
}

static void cpu_power_off(void *data)
{
	int rc;

	pr_err("PMIC Initiated shutdown %s cpu=%d\n", __func__,
						smp_processor_id());
	if (smp_processor_id() == 0) {
		__msm_power_off(0);

		pet_watchdog();
		pr_err("Calling scm to disable arbiter\n");
		
		rc = scm_call_atomic1(SCM_SVC_PWR,
						SCM_IO_DISABLE_PMIC_ARBITER, 1);

		pr_err("SCM returned even when asked to busy loop rc=%d\n", rc);
		pr_err("waiting on pmic to shut msm down\n");
	}

	preempt_disable();
	while (1)
		;
}

static irqreturn_t resout_irq_handler(int irq, void *dev_id)
{
	pr_warn("%s PMIC Initiated shutdown\n", __func__);
	oops_in_progress = 1;
	smp_call_function_many(cpu_online_mask, cpu_power_off, NULL, 0);
	if (smp_processor_id() == 0)
		cpu_power_off(NULL);
	preempt_disable();
	while (1)
		;
	return IRQ_HANDLED;
}

void msm_restart(char mode, const char *cmd)
{

#ifdef CONFIG_MSM_DLOAD_MODE

	
	set_dload_mode(0);

	
	set_dload_mode(in_panic);

	
	if (restart_mode == RESTART_DLOAD)
		set_dload_mode(1);

	
	if (!download_mode)
		set_dload_mode(0);
#endif

	printk(KERN_NOTICE "Going down for restart now\n");

	printk(KERN_NOTICE "%s: Kernel command line: %s\n", __func__, hashed_command_line);

	pm8xxx_reset_pwr_off(1);

	pr_info("%s: restart by command: [%s]\r\n", __func__, (cmd) ? cmd : "");
	
	if (in_panic) {
		
	} else if (!cmd) {
		set_restart_action(RESTART_REASON_REBOOT, NULL);
	} else if (!strncmp(cmd, "bootloader", 10)) {
		set_restart_action(RESTART_REASON_BOOTLOADER, NULL);
	} else if (!strncmp(cmd, "recovery", 8)) {
		set_restart_action(RESTART_REASON_RECOVERY, NULL);
	} else if (!strncmp(cmd, "oem-", 4)) {
		unsigned long code;
		code = simple_strtoul(cmd + 4, NULL, 16) & 0xff;
		set_restart_to_oem(code, NULL);
	} else if (!strncmp(cmd, "force-dog-bark", 14)) {
		set_restart_to_ramdump("force-dog-bark");
	} else if (!strncmp(cmd, "force-hard", 10) ||
			(RESTART_MODE_LEGECY < mode && mode < RESTART_MODE_MAX)
			) {
		
		if (mode == RESTART_MODE_MODEM_USER_INVOKED)
			set_restart_action(RESTART_REASON_REBOOT, NULL);
		else if (mode == RESTART_MODE_ERASE_EFS)
			set_restart_action(RESTART_REASON_ERASE_EFS, NULL);
		else {
			set_restart_action(RESTART_REASON_RAMDUMP, cmd);
		}
	} else {
		set_restart_action(RESTART_REASON_REBOOT, NULL);
	}

	
	if (!(get_radio_flag() & RADIO_FLAG_USB_UPLOAD) || ((get_restart_reason() != RESTART_REASON_RAMDUMP) && (get_restart_reason() != (RESTART_REASON_OEM_BASE | 0x99))))
	{
		turn_off_mdm_power();
		turn_off_qsc_power();
	}
	else
		wait_mdm_enter_self_refresh();

	msm_flush_console();
	flush_cache_all();

	__raw_writel(0, msm_tmr0_base + WDT0_EN);

	if (cmd && !strncmp(cmd, "force-dog-bark", 14)) {
		pr_info("%s: Force dog bark!\r\n", __func__);

		__raw_writel(1, msm_tmr0_base + WDT0_RST);
		__raw_writel(0x31F3, msm_tmr0_base + WDT0_BARK_TIME);
		__raw_writel(5*0x31F3, msm_tmr0_base + WDT0_BITE_TIME);
		__raw_writel(1, msm_tmr0_base + WDT0_EN);

		mdelay(10000);

		pr_info("%s: Force Watchdog bark does not work, falling back to normal process.\r\n", __func__);
	}

#ifdef CONFIG_ARCH_APQ8064
	pr_info("%s: PS_HOLD to restart\r\n", __func__);

	mb();
	__raw_writel(0, PSHOLD_CTL_SU); 
	mdelay(5000);

	pr_info("%s: PS_HOLD didn't work, falling back to watchdog\r\n", __func__);
	pr_info("%s: Restarting by watchdog\r\n", __func__);

	__raw_writel(1, msm_tmr0_base + WDT0_RST);
	__raw_writel(5*0x31F3, msm_tmr0_base + WDT0_BARK_TIME);
	__raw_writel(0x31F3, msm_tmr0_base + WDT0_BITE_TIME);
	__raw_writel(1, msm_tmr0_base + WDT0_EN);

	mdelay(10000);
#else
	if (!(machine_is_msm8x60_fusion() || machine_is_msm8x60_fusn_ffa())) {
		mb();
		__raw_writel(0, PSHOLD_CTL_SU); 
		mdelay(5000);
		pr_notice("PS_HOLD didn't work, falling back to watchdog\n");
	}

	pr_info("%s: Restarting by watchdog\r\n", __func__);

	__raw_writel(1, msm_tmr0_base + WDT0_RST);
	__raw_writel(5*0x31F3, msm_tmr0_base + WDT0_BARK_TIME);
	__raw_writel(0x31F3, msm_tmr0_base + WDT0_BITE_TIME);
	__raw_writel(1, msm_tmr0_base + WDT0_EN);

	mdelay(10000);
#endif
	printk(KERN_ERR "Restarting has failed\n");
}

static int __init msm_restart_init(void)
{
	int rc;

	htc_restart_handler_init();

	atomic_notifier_chain_register(&panic_notifier_list, &panic_blk);
#ifdef CONFIG_MSM_DLOAD_MODE
	dload_mode_addr = MSM_IMEM_BASE + DLOAD_MODE_ADDR;

	
	set_dload_mode(1);
#endif
	msm_tmr0_base = msm_timer_get_timer0_base();
	pm_power_off = msm_power_off;

	if (pmic_reset_irq != 0) {
		rc = request_any_context_irq(pmic_reset_irq,
					resout_irq_handler, IRQF_TRIGGER_HIGH,
					"restart_from_pmic", NULL);
		if (rc < 0)
			pr_err("pmic restart irq fail rc = %d\n", rc);
	} else {
		pr_warn("no pmic restart interrupt specified\n");
	}

	return 0;
}

late_initcall(msm_restart_init);
