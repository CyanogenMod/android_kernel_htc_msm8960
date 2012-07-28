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
#include <linux/workqueue.h>
#include <linux/pm.h>
#include <linux/cpu.h>
#include <linux/interrupt.h>
#include <linux/mfd/pmic8058.h>
#include <linux/mfd/pmic8901.h>
#include <linux/mfd/pm8xxx/misc.h>
#include <linux/sched.h>
#include <linux/console.h>

#include <asm/mach-types.h>

#include <mach/msm_iomap.h>
#include <mach/restart.h>
#include <mach/socinfo.h>
#include <mach/irqs.h>
#include <mach/scm.h>
#include <mach/board_htc.h>
#include <mach/mdm.h>
#include <mach/msm_watchdog.h>
#include <mach/subsystem_restart.h>
#include "timer.h"
#include <mach/pm.h>
#include "smd_private.h"
#if defined(CONFIG_MSM_RMT_STORAGE_CLIENT)
#include <linux/rmt_storage_client.h>
#endif

#define WDT0_RST	0x38
#define WDT0_EN		0x40
#define WDT0_BARK_TIME	0x4C
#define WDT0_BITE_TIME	0x5C

#define PSHOLD_CTL_SU (MSM_TLMM_BASE + 0x820)

#define RESTART_REASON_ADDR 0xF00
#define DLOAD_MODE_ADDR     0x0

#define MSM_REBOOT_REASON_BASE	(MSM_IMEM_BASE + RESTART_REASON_ADDR)
#define SZ_DIAG_ERR_MSG 0xC8

#define SCM_IO_DISABLE_PMIC_ARBITER	1

static int restart_mode;

struct htc_reboot_params {
	unsigned reboot_reason;
	unsigned radio_flag;
	char reserved[256 - SZ_DIAG_ERR_MSG - 8];
	char msg[SZ_DIAG_ERR_MSG];
};

static struct htc_reboot_params *reboot_params;

int pmic_reset_irq;
static void __iomem *msm_tmr0_base;

static int notify_efs_sync = 0;
static int notify_efs_sync_set(const char *val, struct kernel_param *kp);
module_param_call(notify_efs_sync, notify_efs_sync_set, param_get_int,
			&notify_efs_sync, 0644);

static void check_efs_sync_work(struct work_struct *work);
static DECLARE_DELAYED_WORK(checkwork_struct, check_efs_sync_work);

#ifdef CONFIG_MSM_DLOAD_MODE
static int in_panic;
static void *dload_mode_addr;

/* Flag to check if current reboot will enter ramdump */
static int ramdump_reboot;

/* Download mode master kill-switch */
static int dload_set(const char *val, struct kernel_param *kp);
void set_ramdump_reason(const char *msg);
static int download_mode = 1;
module_param_call(download_mode, dload_set, param_get_int,
			&download_mode, 0644);


int check_in_panic(void)
{
	return in_panic;
}

static int panic_prep_restart(struct notifier_block *this,
			      unsigned long event, void *ptr)
{
	/* Prepare a buffer whose size is equal to htc_reboot_params->msg */
	char kernel_panic_msg[SZ_DIAG_ERR_MSG] = "Kernel Panic";
	in_panic = 1;

	/* ptr is a buffer declared in panic function. It's never be NULL.
	   Reserve one space for trailing zero.
	*/
	if (ptr)
		snprintf(kernel_panic_msg, SZ_DIAG_ERR_MSG-1, "KP: %s", (char *)ptr);

	/* Do not set KP restart reason if subsystems have set one already */
	if (!ssr_have_set_restart_reason)
		set_ramdump_reason(kernel_panic_msg);

	return NOTIFY_DONE;
}

static struct notifier_block panic_blk = {
	.notifier_call	= panic_prep_restart,
};

static void set_dload_mode(int on)
{
/* Disable QCT dload mode as HTC has our own dload mechanism. */
#if 0
	if (dload_mode_addr) {
		__raw_writel(on ? 0xE47B337D : 0, dload_mode_addr);
		__raw_writel(on ? 0xCE14091A : 0,
		       dload_mode_addr + sizeof(unsigned int));
		mb();
	}
#endif
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
static atomic_t restart_counter = ATOMIC_INIT(0);
static int modem_cache_flush_done;

inline void notify_modem_cache_flush_done(void)
{
	modem_cache_flush_done = 1;
}

 inline unsigned get_restart_reason(void)
{
	return reboot_params->reboot_reason;
}
EXPORT_SYMBOL(get_restart_reason);

/*
   This function should not be called outside
   to ensure that others do not change restart reason.
   Use mode & cmd to set reason & msg in arch_reset().
*/
static inline void set_restart_reason(unsigned int reason)
{
	reboot_params->reboot_reason = reason;
}

/*
   This function should not be called outsite
   to ensure that others do no change restart reason.
   Use mode & cmd to set reason & msg in arch_reset().
*/
static inline void set_restart_msg(const char *msg)
{
	strncpy(reboot_params->msg, msg, sizeof(reboot_params->msg)-1);
}

static bool console_flushed;

static void msm_pm_flush_console(void)
{
	if (console_flushed)
		return;
	console_flushed = true;

	printk("\n");
	printk(KERN_EMERG "Restarting %s\n", linux_banner);
	if (!console_trylock()) {
		console_unlock();
		return;
	}

	mdelay(50);

	local_irq_disable();
	if (console_trylock())
		printk(KERN_EMERG "restart: Console was locked! Busting\n");
	else
		printk(KERN_EMERG "restart: Console was locked!\n");
	console_unlock();
}

/* This function expose others to restart message for entering ramdump mode. */
void set_ramdump_reason(const char *msg)
{
	/* Only allow write msg before entering arch_rest, unless
	 * device met fatal while performing normal reboot.
	 */
	if (atomic_read(&restart_counter) != 0 &&
		ramdump_reboot)
		return;

	set_restart_reason(RESTART_REASON_RAMDUMP);
	set_restart_msg(msg? msg: "");
	/* Ramdump reason has been set, disallow any subsequent setting */
	ramdump_reboot = 1;
}

static void set_modem_efs_sync(void)
{
	smsm_change_state(SMSM_APPS_STATE, SMSM_APPS_REBOOT, SMSM_APPS_REBOOT);
	printk(KERN_INFO "%s: wait for modem efs_sync\n", __func__);
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
		/* Kick watchdog.
		   Do not assume efs sync will be executed.
		   Assume watchdog timeout is default 4 seconds. */
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

	/* Only allow one instace of efs_sync at the same time */
	if (old_val == 1)
		return -EINVAL;

	ret = param_set_int(val, kp);

	if (ret)
		return ret;

	/* If notify_efs_sync is not one, ignore. */
	if (notify_efs_sync != 1) {
		notify_efs_sync = 0;
		return -EINVAL;
	}

	notify_modem_efs_sync_schedule(10);

	return 0;
}

static void __msm_power_off(int lower_pshold)
{
	printk(KERN_CRIT "Powering off the SoC\n");
#ifdef CONFIG_MSM_DLOAD_MODE
	set_dload_mode(0);
#endif

	/* Check if userspace initiated efs_sync is done */
	if (notify_efs_sync) {
		/* Cancel check worker */
		pr_info("%s: userspace initiated efs_sync not finished...\n", __func__);
		cancel_delayed_work(&checkwork_struct);
		/* Change to busy wait for whichever seconds left */
		check_modem_efs_sync_timeout(efs_sync_work_timout);
	}

	if (cpu_is_msm8x60()) {
		pm8058_reset_pwr_off(0);
		pm8901_reset_pwr_off(0);
	}

#ifdef CONFIG_ARCH_MSM8960
	if (pm8xxx_reset_pwr_off(0))
		panic("pm8xxx_reset_pwr_off fail");
#else
	pm8xxx_reset_pwr_off(0);
#endif
	if (lower_pshold) {
		__raw_writel(0, PSHOLD_CTL_SU);
		mdelay(10000);
		printk(KERN_ERR "Powering off has failed\n");
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

inline void soc_restart(char mode, const char *cmd)
{
	arm_pm_restart(mode, cmd);
}

void arch_reset(char mode, const char *cmd)
{
	unsigned long oem_code = 0;

	/* arch_reset should only enter once*/
	if (atomic_add_return(1, &restart_counter) != 1)
		return;

#ifdef CONFIG_MSM_DLOAD_MODE

	/* This looks like a normal reboot at this point. */
	set_dload_mode(0);

	/* Write download mode flags if we're panic'ing */
	set_dload_mode(in_panic);

	/* Write download mode flags if restart_mode says so */
	if (restart_mode == RESTART_DLOAD)
		set_dload_mode(1);

	/* Kill download mode if master-kill switch is set */
	if (!download_mode)
		set_dload_mode(0);
#endif

	printk(KERN_NOTICE "Going down for restart now\n");
	printk(KERN_NOTICE "%s: mode %d\n", __func__, mode);
	if (cmd) {
		printk(KERN_NOTICE "%s: restart command `%s'.\n", __func__, cmd);
		/* XXX: modem will set msg itself.
			Dying msg should be passed to this function directly. */
		if (mode != RESTART_MODE_MODEM_CRASH)
			set_restart_msg(cmd);
	} else
		printk(KERN_NOTICE "%s: no command restart.\n", __func__);

	if (cpu_is_msm8x60())
		pm8058_reset_pwr_off(1);
#ifdef CONFIG_ARCH_MSM8960
	if (pm8xxx_reset_pwr_off(1)) {
		in_panic = 1;
	}
#else
	pm8xxx_reset_pwr_off(1);
#endif

	if (in_panic) {
		/* Reason & message are set in panic notifier, panic_prep_restart.
		   Kernel panic is a top priority.
		   Keep this condition to avoid the reason and message being modified. */
	} else if (!cmd) {
		set_restart_reason(RESTART_REASON_REBOOT);
	} else if (!strncmp(cmd, "bootloader", 10)) {
		set_restart_reason(RESTART_REASON_BOOTLOADER);
	} else if (!strncmp(cmd, "recovery", 8)) {
		set_restart_reason(RESTART_REASON_RECOVERY);
	} else if (!strcmp(cmd, "eraseflash")) {
		set_restart_reason(RESTART_REASON_ERASE_FLASH);
	} else if (!strncmp(cmd, "oem-", 4)) {
		oem_code = simple_strtoul(cmd + 4, 0, 16) & 0xff;

		/* oem-96, 97, 98, 99 are RIL fatal */
		if ((oem_code == 0x96) || (oem_code == 0x97) || (oem_code == 0x98)) {
			oem_code = 0x99;
			/* RIL fatals will enter ramdump */
			ramdump_reboot = 1;
		}

		set_restart_reason(RESTART_REASON_OEM_BASE | oem_code);
	} else if (!strcmp(cmd, "force-hard") ||
			(RESTART_MODE_LEGECY < mode && mode < RESTART_MODE_MAX)
		) {
		/* The only situation modem user triggers reset is NV restore after erasing EFS. */
		if (mode == RESTART_MODE_MODEM_USER_INVOKED)
			set_restart_reason(RESTART_REASON_REBOOT);
		else if (mode == RESTART_MODE_ERASE_EFS)
			set_restart_reason(RESTART_REASON_ERASE_EFS);
		else {
			set_restart_reason(RESTART_REASON_RAMDUMP);
			/* Force hards will also enter ramdump */
			ramdump_reboot = 1;
		}
	} else {
		/* unknown command */
		set_restart_reason(RESTART_REASON_REBOOT);
	}

	/* boot mode: 0 - normal, 1 - factory2, 2 - recovery */
	if (mode != RESTART_MODE_MODEM_CRASH && board_mfg_mode() <= 2) {
		/* oem-11 is userspace-triggered reboot */
		if (oem_code != 0x11) {
			set_modem_efs_sync();
			check_modem_efs_sync_timeout(10);
		}
	}

	/* Check if userspace initiated efs_sync is done */
	if (notify_efs_sync) {
		/* Cancel check worker */
		pr_info("%s: userspace initiated efs_sync not finished...\n", __func__);
		cancel_delayed_work(&checkwork_struct);
		/* Change to busy wait for whichever seconds left */
		check_modem_efs_sync_timeout(efs_sync_work_timout - 1);
	}

	msm_pm_flush_console();

	__raw_writel(0, msm_tmr0_base + WDT0_EN);
	if (!(machine_is_msm8x60_fusion() || machine_is_msm8x60_fusn_ffa())) {
		mb();
		__raw_writel(0, PSHOLD_CTL_SU); /* Actually reset the chip */
		mdelay(5000);
		pr_notice("PS_HOLD didn't work, falling back to watchdog\n");
	}

	__raw_writel(1, msm_tmr0_base + WDT0_RST);
	__raw_writel(5*0x31F3, msm_tmr0_base + WDT0_BARK_TIME);
	__raw_writel(0x31F3, msm_tmr0_base + WDT0_BITE_TIME);
	__raw_writel(1, msm_tmr0_base + WDT0_EN);

	mdelay(10000);
	printk(KERN_ERR "Restarting has failed\n");
}


static int __init msm_restart_init(void)
{
	int rc;
	arm_pm_restart = arch_reset;

#ifdef CONFIG_MSM_DLOAD_MODE
	atomic_notifier_chain_register(&panic_notifier_list, &panic_blk);
	dload_mode_addr = MSM_IMEM_BASE + DLOAD_MODE_ADDR;

	/* Reset detection is switched on below.*/
	set_dload_mode(1);
#endif
	msm_tmr0_base = msm_timer_get_timer0_base();
	reboot_params = (void *)MSM_REBOOT_REASON_BASE;
	memset(reboot_params, 0x0, sizeof(struct htc_reboot_params));
	set_restart_reason(RESTART_REASON_RAMDUMP);
	set_restart_msg("Unknown");
	reboot_params->radio_flag = get_radio_flag();

	if (pmic_reset_irq != 0) {
		rc = request_any_context_irq(pmic_reset_irq,
					resout_irq_handler, IRQF_TRIGGER_HIGH,
					"restart_from_pmic", NULL);
		if (rc < 0)
			pr_err("pmic restart irq fail rc = %d\n", rc);
	} else {
		pr_warn("no pmic restart interrupt specified\n");
	}

	pm_power_off = msm_power_off;
	return 0;
}

late_initcall(msm_restart_init);
