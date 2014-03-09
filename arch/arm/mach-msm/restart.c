/* Copyright (c) 2010-2012, The Linux Foundation. All rights reserved.
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
#include <linux/workqueue.h>
#include <linux/pm.h>
#include <linux/cpu.h>
#include <linux/interrupt.h>
#include <linux/mfd/pmic8058.h>
#include <linux/mfd/pmic8901.h>
#include <linux/mfd/pm8xxx/misc.h>
#include <linux/gpio.h>
#include <linux/console.h>

#include <asm/mach-types.h>

#include <mach/msm_iomap.h>
#include <mach/restart.h>
#include <mach/socinfo.h>
#include <mach/irqs.h>
#include <mach/scm.h>
#include <mach/board_htc.h>
#include <mach/htc_restart_handler.h>

#include "msm_watchdog.h"
#include "smd_private.h"
#include "timer.h"

#define WDT0_RST	0x38
#define WDT0_EN		0x40
#define WDT0_BARK_TIME	0x4C
#define WDT0_BITE_TIME	0x5C

#define PSHOLD_CTL_SU (MSM_TLMM_BASE + 0x820)

#define RESTART_REASON_ADDR 0xF00
#define DLOAD_MODE_ADDR     0x0

#define SCM_IO_DISABLE_PMIC_ARBITER	1

#ifdef CONFIG_LGE_CRASH_HANDLER
#define LGE_ERROR_HANDLER_MAGIC_NUM	0xA97F2C46
#define LGE_ERROR_HANDLER_MAGIC_ADDR	0x18
void *lge_error_handler_cookie_addr;
static int ssr_magic_number = 0;
#endif

static int restart_mode;
void *restart_reason;
int ramdump_source=0;

int pmic_reset_irq;
static void __iomem *msm_tmr0_base;

#ifdef CONFIG_MACH_HTC
static int notify_efs_sync = 0;
static int notify_efs_sync_set(const char *val, struct kernel_param *kp);
module_param_call(notify_efs_sync, notify_efs_sync_set, param_get_int,
		&notify_efs_sync, 0644);

static void check_efs_sync_work(struct work_struct *work);
static DECLARE_DELAYED_WORK(checkwork_struct, check_efs_sync_work);
#endif

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

/* Download mode master kill-switch */
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
#ifdef CONFIG_LGE_CRASH_HANDLER
		__raw_writel(on ? LGE_ERROR_HANDLER_MAGIC_NUM : 0,
				lge_error_handler_cookie_addr);
#endif
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

	/* If download_mode is not zero or one, ignore. */
	if (download_mode >> 1) {
		download_mode = old_val;
		return -EINVAL;
	}

	set_dload_mode(download_mode);
#ifdef CONFIG_LGE_CRASH_HANDLER
	ssr_magic_number = 0;
#endif

	return 0;
}
#else
#define set_dload_mode(x) do {} while (0)
#endif

void msm_set_restart_mode(int mode)
{
	restart_mode = mode;
#ifdef CONFIG_LGE_CRASH_HANDLER
	if (download_mode == 1 && (mode & 0xFFFF0000) == 0x6D630000)
		panic("LGE crash handler detected panic");
#endif
}
EXPORT_SYMBOL(msm_set_restart_mode);

#ifdef CONFIG_MACH_HTC
static unsigned ap2mdm_pmic_reset_n_gpio = -1;
void register_ap2mdm_pmic_reset_n_gpio(unsigned gpio)
{
	ap2mdm_pmic_reset_n_gpio = gpio;
}
EXPORT_SYMBOL(register_ap2mdm_pmic_reset_n_gpio);

static void mdm_power_off(void)
{
	int i;

	if (gpio_is_valid(ap2mdm_pmic_reset_n_gpio)) {
		pr_info("%s: Powering off MDM", __func__);
		gpio_direction_output(ap2mdm_pmic_reset_n_gpio, 0);
		for (i = 0; i < 2; i++) {
			pet_watchdog();
			mdelay(1000);
		}
		pr_info("%s: Powered off MDM", __func__);
	}
}

static void mdm_wait_self_refresh(void)
{
	int i;

	for (i = 0; i < 2; i++) {
		pet_watchdog();
		mdelay(1000);
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

static void set_modem_efs_sync(void)
{
	smsm_change_state(SMSM_APPS_STATE, SMSM_APPS_REBOOT, SMSM_APPS_REBOOT);
	printk(KERN_INFO "[K] %s: wait for modem efs_sync\n", __func__);
}

static int check_modem_efs_sync(void)
{
	return (smsm_get_state(SMSM_MODEM_STATE) & SMSM_SYSTEM_PWRDWN_USR);
}

static int efs_sync_work_timout;
static void check_efs_sync_work(struct work_struct *work)
{
	if (--efs_sync_work_timout > 0 && !check_modem_efs_sync()) {
		schedule_delayed_work(&checkwork_struct, msecs_to_jiffies(1000));
	} else {
		notify_efs_sync = 0;
		if (efs_sync_work_timout <= 0)
			pr_notice("%s: modem efs_sync timeout.\n", __func__);
		else
			pr_info("%s: modem efs_sync done.\n", __func__);
	}
}

static void notify_modem_efs_sync_schedule(unsigned timeout)
{
	efs_sync_work_timout = timeout;
	set_modem_efs_sync();
	schedule_delayed_work(&checkwork_struct, msecs_to_jiffies(1000));
}

static void check_modem_efs_sync_timeout(unsigned timeout)
{
	while (timeout > 0 && !check_modem_efs_sync()) {
		writel(1, msm_tmr0_base + WDT0_RST);
		mdelay(1000);
		timeout--;
	}
	if (timeout <= 0)
		pr_notice("%s: modem efs_sync timeout.\n", __func__);
	else
		pr_info("%s: modem efs_sync done.\n", __func__);
}

static int notify_efs_sync_set(const char *val, struct kernel_param *kp)
{
	int ret;
	int old_val = notify_efs_sync;

	if (old_val == 1)
		return -EINVAL;

	ret = param_set_int(val, kp);

	if (ret)
		return ret;

	if (notify_efs_sync != 1) {
		notify_efs_sync = 0;
		return -EINVAL;
	}

	notify_modem_efs_sync_schedule(10);

	return 0;
}

static int notify_efs_sync_call
	(struct notifier_block *this, unsigned long code, void *_cmd)
{
	unsigned long oem_code = 0;

	switch (code) {
		case SYS_RESTART:
			if (_cmd && !strncmp(_cmd, "oem-", 4)) {
				oem_code = simple_strtoul(_cmd + 4, 0, 16) & 0xff;
			}

			if (board_mfg_mode() <= 2) {
				if (oem_code != 0x11) {
					set_modem_efs_sync();
					check_modem_efs_sync_timeout(10);
				}
			}

		case SYS_POWER_OFF:
			if (notify_efs_sync) {
				pr_info("%s: userspace initiated efs_sync not finished...\n", __func__);
				cancel_delayed_work(&checkwork_struct);
				check_modem_efs_sync_timeout(efs_sync_work_timout - 1);
			}
			break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block notify_efs_sync_notifier = {
   .notifier_call = notify_efs_sync_call,
};
#endif

static void __msm_power_off(int lower_pshold)
{
	printk(KERN_CRIT "[K] Powering off the SoC\n");
#ifdef CONFIG_MACH_HTC
	mdm_power_off();
#endif
#ifdef CONFIG_MSM_DLOAD_MODE
	set_dload_mode(0);
#endif
	pm8xxx_reset_pwr_off(0);

	if (lower_pshold) {
		__raw_writel(0, PSHOLD_CTL_SU);
		mdelay(10000);
		printk(KERN_ERR "[K] Powering off has failed\n");
	}
	return;
}

static void msm_power_off(void)
{
	/* MSM initiated power off, lower ps_hold */
	__msm_power_off(1);
}

static void cpu_power_off(void *data)
{
	int rc;

	pr_err("PMIC Initiated shutdown %s cpu=%d\n", __func__,
						smp_processor_id());
	if (smp_processor_id() == 0) {
		/*
		 * PMIC initiated power off, do not lower ps_hold, pmic will
		 * shut msm down
		 */
		__msm_power_off(0);

		pet_watchdog();
		pr_err("Calling scm to disable arbiter\n");
		/* call secure manager to disable arbiter and never return */
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

#ifdef CONFIG_LGE_CRASH_HANDLER
#define SUBSYS_NAME_MAX_LENGTH	40

int get_ssr_magic_number(void)
{
	return ssr_magic_number;
}

void set_ssr_magic_number(const char* subsys_name)
{
	int i;
	const char *subsys_list[] = {
		"modem", "riva", "dsps", "lpass",
		"external_modem", "gss",
	};

	ssr_magic_number = (0x6d630000 | 0x0000f000);

	for (i=0; i < ARRAY_SIZE(subsys_list); i++) {
		if (!strncmp(subsys_list[i], subsys_name,
					SUBSYS_NAME_MAX_LENGTH)) {
			ssr_magic_number = (0x6d630000 | ((i+1)<<12));
			break;
		}
	}
}

void set_kernel_crash_magic_number(void)
{
	pet_watchdog();
	if (ssr_magic_number == 0)
		__raw_writel(0x6d630100, restart_reason);
	else
		__raw_writel(restart_mode, restart_reason);
}
#endif /* CONFIG_LGE_CRASH_HANDLER */

void msm_restart(char mode, const char *cmd)
{
#ifdef CONFIG_MACH_HTC
	unsigned long oem_code = 0;
#endif

#ifdef CONFIG_MSM_DLOAD_MODE
	/* This looks like a normal reboot at this point. */
	set_dload_mode(0);

	/* Write download mode flags if we're panic'ing */
	set_dload_mode(in_panic);

	/* Write download mode flags if restart_mode says so */
	if (restart_mode == RESTART_DLOAD) {
		set_dload_mode(1);
#ifdef CONFIG_LGE_CRASH_HANDLER
		writel(0x6d63c421, restart_reason);
		goto reset;
#endif
	}

	/* Kill download mode if master-kill switch is set */
	if (!download_mode)
		set_dload_mode(0);
#endif

	printk(KERN_NOTICE "[K] Going down for restart now\n");

	printk(KERN_NOTICE "%s: Kernel command line: %s\n", __func__, saved_command_line);

	pm8xxx_reset_pwr_off(1);

	pr_info("[K] %s: restart by command: [%s]\r\n", __func__, (cmd) ? cmd : "");

#ifdef CONFIG_MACH_HTC
	if (in_panic) {

	} else if (!cmd) {
		set_restart_action(RESTART_REASON_REBOOT, NULL);
	} else if (!strncmp(cmd, "bootloader", 10)) {
		set_restart_action(RESTART_REASON_BOOTLOADER, NULL);
	} else if (!strncmp(cmd, "recovery", 8)) {
		set_restart_action(RESTART_REASON_RECOVERY, NULL);
	} else if (!strcmp(cmd, "eraseflash")) {
		set_restart_action(RESTART_REASON_ERASE_FLASH, NULL);
	} else if (!strncmp(cmd, "oem-", 4)) {
		oem_code = simple_strtoul(cmd + 4, NULL, 16) & 0xff;
		set_restart_to_oem(oem_code, NULL);
	} else if (!strcmp(cmd, "force-hard") ||
			(RESTART_MODE_LEGACY < mode && mode < RESTART_MODE_MAX)
			) {
		if (mode == RESTART_MODE_MODEM_USER_INVOKED)
			set_restart_action(RESTART_REASON_REBOOT, NULL);
		else {
			set_restart_action(RESTART_REASON_RAMDUMP, cmd);
		}
	} else {
		set_restart_action(RESTART_REASON_REBOOT, NULL);
	}
#else
	if (cmd != NULL) {
		if (!strncmp(cmd, "bootloader", 10)) {
			__raw_writel(0x77665500, restart_reason);
		} else if (!strncmp(cmd, "recovery", 8)) {
			__raw_writel(0x77665502, restart_reason);
		} else if (!strncmp(cmd, "oem-", 4)) {
			unsigned long code;
			code = simple_strtoul(cmd + 4, NULL, 16) & 0xff;
			__raw_writel(0x6f656d00 | code, restart_reason);
		} else {
			__raw_writel(0x77665501, restart_reason);
		}
	} else {
		__raw_writel(0x77665501, restart_reason);
	}
#endif
#ifdef CONFIG_LGE_CRASH_HANDLER
	if (in_panic == 1)
		set_kernel_crash_magic_number();
reset:
#endif /* CONFIG_LGE_CRASH_HANDLER */

#ifdef CONFIG_MACH_HTC
	if (!(get_radio_flag() & RADIO_FLAG_USB_UPLOAD) ||
			((get_restart_reason() != RESTART_REASON_RAMDUMP) &&
			 (get_restart_reason() != (RESTART_REASON_OEM_BASE | 0x99))))
		mdm_power_off();
	else
		mdm_wait_self_refresh();

	msm_flush_console();
#endif

	__raw_writel(0, msm_tmr0_base + WDT0_EN);
	if (!(machine_is_msm8x60_fusion() || machine_is_msm8x60_fusn_ffa())) {
		mb();
		__raw_writel(0, PSHOLD_CTL_SU); /* Actually reset the chip */
		mdelay(5000);
		pr_notice("[K] PS_HOLD didn't work, falling back to watchdog\n");
	}

	pr_info("[K] %s: Restarting by watchdog\r\n", __func__);

	__raw_writel(1, msm_tmr0_base + WDT0_RST);
	__raw_writel(5*0x31F3, msm_tmr0_base + WDT0_BARK_TIME);
	__raw_writel(0x31F3, msm_tmr0_base + WDT0_BITE_TIME);
	__raw_writel(1, msm_tmr0_base + WDT0_EN);

	mdelay(10000);
	printk(KERN_ERR "[K] Restarting has failed\n");
}

static int __init msm_pmic_restart_init(void)
{
	int rc;

#ifdef CONFIG_MACH_HTC
	htc_restart_handler_init();
	register_reboot_notifier(&notify_efs_sync_notifier);
#endif

	if (pmic_reset_irq != 0) {
		rc = request_any_context_irq(pmic_reset_irq,
					resout_irq_handler, IRQF_TRIGGER_HIGH,
					"restart_from_pmic", NULL);
		if (rc < 0)
			pr_err("pmic restart irq fail rc = %d\n", rc);
	} else {
		pr_warn("no pmic restart interrupt specified\n");
	}

#ifdef CONFIG_LGE_CRASH_HANDLER
	__raw_writel(0x6d63ad00, restart_reason);
#endif

	return 0;
}

late_initcall(msm_pmic_restart_init);

static int __init msm_restart_init(void)
{
	atomic_notifier_chain_register(&panic_notifier_list, &panic_blk);
#ifdef CONFIG_MSM_DLOAD_MODE
	dload_mode_addr = MSM_IMEM_BASE + DLOAD_MODE_ADDR;
#ifdef CONFIG_LGE_CRASH_HANDLER
	lge_error_handler_cookie_addr = MSM_IMEM_BASE +
		LGE_ERROR_HANDLER_MAGIC_ADDR;
#endif
	set_dload_mode(download_mode);
#endif
	msm_tmr0_base = msm_timer_get_timer0_base();
	pm_power_off = msm_power_off;

	return 0;
}
early_initcall(msm_restart_init);
