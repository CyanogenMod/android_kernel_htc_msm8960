/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/reboot.h>
#include <linux/workqueue.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/stringify.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/debugfs.h>

#include <mach/irqs.h>
#include <mach/scm.h>
#include <mach/peripheral-loader.h>
#include <mach/subsystem_restart.h>
#include <mach/subsystem_notif.h>
#include <mach/socinfo.h>
#include <mach/restart.h>
#include <mach/board_htc.h>
#include <mach/system.h>

#include "smd_private.h"
#include "modem_notifier.h"
#include "ramdump.h"

#define MODULE_NAME			"modem_8960"

static int crash_shutdown;
#if defined (CONFIG_MSM_MODEM_SSR_ENABLE)
static int enable_modem_ssr = 1;
#else
static int enable_modem_ssr = 0;
#endif

static struct workqueue_struct *monitor_modem_hung_wq = NULL;
static struct delayed_work monitor_modem_hung_worker;
static void modem_is_hung(struct work_struct *work);
#define HTC_SMEM_PARAM_BASE_ADDR				0x801F0000
#define HTC_ERR_FATAL_HANDLER_MAGIC_OFFSET		0x5C
#define HTC_ERR_FATAL_HANDLER_MAGIC				0x95425012
static uint32_t *htc_err_fatal_handler_magic;

static void modem_sw_fatal_fn(struct work_struct *work)
{
	uint32_t panic_smsm_states = SMSM_RESET | SMSM_SYSTEM_DOWNLOAD;
	uint32_t reset_smsm_states = SMSM_SYSTEM_REBOOT_USR |
					SMSM_SYSTEM_PWRDWN_USR;
	uint32_t modem_state;

	pr_err("Watchdog bite received from modem SW!\n");

	modem_state = smsm_get_state(SMSM_MODEM_STATE);

	if (modem_state & panic_smsm_states) {

		pr_err("Modem SMSM state changed to SMSM_RESET.\n"
			"Probable err_fatal on the modem. "
			"Calling subsystem restart...\n");
		if (get_restart_level() == RESET_SOC)
			ssr_set_restart_reason(
				"modem fatal: Modem SW Watchdog Bite!");
		if (monitor_modem_hung_wq && (*htc_err_fatal_handler_magic == HTC_ERR_FATAL_HANDLER_MAGIC)) {
			pr_info("monitor_modem_hung_worker is un-scheduled and htc_err_fatal_handler_magic is cleared\n");
			*htc_err_fatal_handler_magic = 0;
			cancel_delayed_work(&monitor_modem_hung_worker);
		}
		subsystem_restart("modem");

	} else if (modem_state & reset_smsm_states) {

		pr_err("%s: User-invoked system reset/powerdown. "
			"Resetting the SoC now.\n",
			__func__);
		kernel_restart(NULL);
	} else {
		/* TODO: Bus unlock code/sequence goes _here_ */
		if (get_restart_level() == RESET_SOC)
			ssr_set_restart_reason(
				"modem fatal: Modem SW Watchdog Bite!");
		if (monitor_modem_hung_wq && (*htc_err_fatal_handler_magic == HTC_ERR_FATAL_HANDLER_MAGIC)) {
			pr_info("monitor_modem_hung_worker is un-scheduled and htc_err_fatal_handler_magic is cleared\n");
			*htc_err_fatal_handler_magic = 0;
			cancel_delayed_work(&monitor_modem_hung_worker);
		}
		subsystem_restart("modem");
	}
}

static void modem_fw_fatal_fn(struct work_struct *work)
{
	pr_err("Watchdog bite received from modem FW!\n");
	if (get_restart_level() == RESET_SOC)
		ssr_set_restart_reason(
			"modem fatal: Modem FW Watchdog Bite!");
	if (monitor_modem_hung_wq && (*htc_err_fatal_handler_magic == HTC_ERR_FATAL_HANDLER_MAGIC)) {
		pr_info("monitor_modem_hung_worker is un-scheduled and htc_err_fatal_handler_magic is cleared\n");
		*htc_err_fatal_handler_magic = 0;
		cancel_delayed_work(&monitor_modem_hung_worker);
	}
	subsystem_restart("modem");
}

static DECLARE_WORK(modem_sw_fatal_work, modem_sw_fatal_fn);
static DECLARE_WORK(modem_fw_fatal_work, modem_fw_fatal_fn);

static void modem_is_hung(struct work_struct *work)
{
	if(get_radio_flag() & 0x8)
	{
		pr_info("Modem is hung and trigger oem-94\n");
		arm_pm_restart(RESTART_MODE_LEGECY, "oem-94");
	}
	else
	{
		pr_info("Modem is hung and trigger modem SSR\n");
		subsystem_restart("modem");
	}
}

static void smsm_state_cb(void *data, uint32_t old_state, uint32_t new_state)
{
	/* Ignore if we're the one that set SMSM_RESET */
	if (crash_shutdown)
		return;

	if (new_state & SMSM_RESET) {
		pr_err("Modem SMSM state changed to SMSM_RESET.\n"
			"Probable err_fatal on the modem. "
			"Calling subsystem restart...\n");
		if (monitor_modem_hung_wq && (*htc_err_fatal_handler_magic == HTC_ERR_FATAL_HANDLER_MAGIC)) {
			pr_info("smsm_state_cb 0x%X -> 0x%X\n"
				"monitor_modem_hung_worker is un-scheduled and htc_err_fatal_handler_magic is cleared\n", old_state, new_state);
			*htc_err_fatal_handler_magic = 0;
			cancel_delayed_work(&monitor_modem_hung_worker);
		}

		if (smd_smsm_erase_efs()) {
			pr_err("Unrecoverable efs, need to reboot and erase"
				"modem_st1/st2 partitions...\n");
			arm_pm_restart(RESTART_MODE_ERASE_EFS, "force-hard");
		} else {
			subsystem_restart("modem");
		}
	}
}

static void smsm_state_cb1(void *data, uint32_t old_state, uint32_t new_state)
{
	pr_info("smsm_state_cb1 0x%X -> 0x%X\n", old_state, new_state);

	if(new_state & SMSM_PRE_RESET) {
		pr_err("Modem SMSM state changed to SMSM_PRE_RESET but clear modem SMSM_PRE_RESET first.\n");
		smsm_change_state_ssr(SMSM_MODEM_STATE, SMSM_PRE_RESET, 0, KERNEL_FLAG_ENABLE_SSR_MODEM);
		if (monitor_modem_hung_wq && (*htc_err_fatal_handler_magic == HTC_ERR_FATAL_HANDLER_MAGIC)) {
			if(get_radio_flag() & 0x8)
				pr_info("Wait for 3s to trigger oem-94 if modem is hung in err_fatal_handler\n"
					"monitor_modem_hung_worker is scheduled and htc_err_fatal_handler_magic = 0x%X\n", *htc_err_fatal_handler_magic);
			else
				pr_info("Wait for 3s to trigger modem SSR if modem is hung in err_fatal_handler\n"
					"monitor_modem_hung_worker is scheduled and htc_err_fatal_handler_magic = 0x%X\n", *htc_err_fatal_handler_magic);
			queue_delayed_work(monitor_modem_hung_wq, &monitor_modem_hung_worker, msecs_to_jiffies(3000));
		}
	}
}

#define Q6_FW_WDOG_ENABLE		0x08882024
#define Q6_SW_WDOG_ENABLE		0x08982024
static int modem_shutdown(const struct subsys_data *subsys)
{
	void __iomem *q6_fw_wdog_addr;
	void __iomem *q6_sw_wdog_addr;

	/*
	 * Disable the modem watchdog since it keeps running even after the
	 * modem is shutdown.
	 */
	q6_fw_wdog_addr = ioremap_nocache(Q6_FW_WDOG_ENABLE, 4);
	if (!q6_fw_wdog_addr)
		return -ENOMEM;

	q6_sw_wdog_addr = ioremap_nocache(Q6_SW_WDOG_ENABLE, 4);
	if (!q6_sw_wdog_addr) {
		iounmap(q6_fw_wdog_addr);
		return -ENOMEM;
	}

	writel_relaxed(0x0, q6_fw_wdog_addr);
	writel_relaxed(0x0, q6_sw_wdog_addr);
	mb();
	iounmap(q6_sw_wdog_addr);
	iounmap(q6_fw_wdog_addr);

	pil_force_shutdown("modem");
	pil_force_shutdown("modem_fw");
	disable_irq_nosync(Q6FW_WDOG_EXPIRED_IRQ);
	disable_irq_nosync(Q6SW_WDOG_EXPIRED_IRQ);

	return 0;
}

static int modem_powerup(const struct subsys_data *subsys)
{
	pil_force_boot("modem_fw");
	pil_force_boot("modem");
	enable_irq(Q6FW_WDOG_EXPIRED_IRQ);
	enable_irq(Q6SW_WDOG_EXPIRED_IRQ);
	return 0;
}

void modem_crash_shutdown(const struct subsys_data *subsys)
{
	crash_shutdown = 1;
	smsm_reset_modem(SMSM_RESET);
}

/* FIXME: Get address, size from PIL */
static struct ramdump_segment modemsw_segments[] = {
	{0x89000000, 0x8D400000 - 0x89000000},
};

static struct ramdump_segment modemfw_segments[] = {
	{0x8D400000, 0x8DA00000 - 0x8D400000},
};

static struct ramdump_segment smem_segments[] = {
	{0x80000000, 0x00200000},
};

static void *modemfw_ramdump_dev;
static void *modemsw_ramdump_dev;
static void *smem_ramdump_dev;

static int modem_ramdump(int enable,
				const struct subsys_data *crashed_subsys)
{
	int ret = 0;

	if (enable) {
		ret = do_ramdump(modemsw_ramdump_dev, modemsw_segments,
			ARRAY_SIZE(modemsw_segments));

		if (ret < 0) {
			pr_err("Unable to dump modem sw memory (rc = %d).\n",
			       ret);
			goto out;
		}

		ret = do_ramdump(modemfw_ramdump_dev, modemfw_segments,
			ARRAY_SIZE(modemfw_segments));

		if (ret < 0) {
			pr_err("Unable to dump modem fw memory (rc = %d).\n",
				ret);
			goto out;
		}

		ret = do_ramdump(smem_ramdump_dev, smem_segments,
			ARRAY_SIZE(smem_segments));

		if (ret < 0) {
			pr_err("Unable to dump smem memory (rc = %d).\n", ret);
			goto out;
		}
	}

out:
	return ret;
}

static irqreturn_t modem_wdog_bite_irq(int irq, void *dev_id)
{
	int ret;

	switch (irq) {

	case Q6SW_WDOG_EXPIRED_IRQ:
		ret = schedule_work(&modem_sw_fatal_work);
		disable_irq_nosync(Q6SW_WDOG_EXPIRED_IRQ);
		disable_irq_nosync(Q6FW_WDOG_EXPIRED_IRQ);
		break;
	case Q6FW_WDOG_EXPIRED_IRQ:
		ret = schedule_work(&modem_fw_fatal_work);
		disable_irq_nosync(Q6SW_WDOG_EXPIRED_IRQ);
		disable_irq_nosync(Q6FW_WDOG_EXPIRED_IRQ);
		break;
	break;

	default:
		pr_err("%s: Unknown IRQ!\n", __func__);
	}

	return IRQ_HANDLED;
}

static struct subsys_data modem_8960 = {
	.name = "modem",
	.shutdown = modem_shutdown,
	.powerup = modem_powerup,
	.ramdump = modem_ramdump,
	.crash_shutdown = modem_crash_shutdown,
	.enable_ssr = 0
};

static int enable_modem_ssr_set(const char *val, struct kernel_param *kp)
{
	int ret;

	ret = param_set_int(val, kp);
	if (ret)
		return ret;

	if (enable_modem_ssr)
		pr_info(MODULE_NAME ": Subsystem restart activated for Modem.\n");

	modem_8960.enable_ssr = enable_modem_ssr;

	return 0;
}

module_param_call(enable_modem_ssr, enable_modem_ssr_set, param_get_int,
			&enable_modem_ssr, S_IRUGO | S_IWUSR);

static int modem_subsystem_restart_init(void)
{
	modem_8960.enable_ssr = enable_modem_ssr;

	return ssr_register_subsystem(&modem_8960);
}

static int modem_debug_set(void *data, u64 val)
{
	if (val == 1)
		subsystem_restart("modem");

	return 0;
}

static int modem_debug_get(void *data, u64 *val)
{
	*val = 0;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(modem_debug_fops, modem_debug_get, modem_debug_set,
				"%llu\n");

static int modem_debugfs_init(void)
{
	struct dentry *dent;
	dent = debugfs_create_dir("modem_debug", 0);

	if (IS_ERR(dent))
		return PTR_ERR(dent);

	debugfs_create_file("reset_modem", 0644, dent, NULL,
		&modem_debug_fops);
	return 0;
}

static int __init modem_8960_init(void)
{
	int ret;

	if (!cpu_is_msm8960() && !cpu_is_msm8930() && !cpu_is_msm9615())
		return -ENODEV;

	ret = smsm_state_cb_register(SMSM_MODEM_STATE, SMSM_RESET,
		smsm_state_cb, 0);

	if (ret < 0)
		pr_err("%s: Unable to register SMSM callback! (%d)\n",
				__func__, ret);

	ret = smsm_state_cb_register(SMSM_MODEM_STATE, SMSM_PRE_RESET,
		smsm_state_cb1, 0);

	if (ret < 0)
		pr_err("%s: Unable to register SMSM callback1! (%d)\n",
				__func__, ret);

	ret = request_irq(Q6FW_WDOG_EXPIRED_IRQ, modem_wdog_bite_irq,
			IRQF_TRIGGER_RISING, "modem_wdog_fw", NULL);

	if (ret < 0) {
		pr_err("%s: Unable to request q6fw watchdog IRQ. (%d)\n",
				__func__, ret);
		goto out;
	}

	ret = request_irq(Q6SW_WDOG_EXPIRED_IRQ, modem_wdog_bite_irq,
			IRQF_TRIGGER_RISING, "modem_wdog_sw", NULL);

	if (ret < 0) {
		pr_err("%s: Unable to request q6sw watchdog IRQ. (%d)\n",
				__func__, ret);
		disable_irq_nosync(Q6FW_WDOG_EXPIRED_IRQ);
		goto out;
	}

       if (get_kernel_flag() & KERNEL_FLAG_ENABLE_SSR_MODEM)
		enable_modem_ssr = 1;

	pr_info("%s: enable_modem_ssr set to %d\n", __func__, enable_modem_ssr);

	ret = modem_subsystem_restart_init();

	if (ret < 0) {
		pr_err("%s: Unable to reg with subsystem restart. (%d)\n",
				__func__, ret);
		goto out;
	}

	modemfw_ramdump_dev = create_ramdump_device("modem_fw");

	if (!modemfw_ramdump_dev) {
		pr_err("%s: Unable to create modem fw ramdump device. (%d)\n",
				__func__, -ENOMEM);
		ret = -ENOMEM;
		goto out;
	}

	modemsw_ramdump_dev = create_ramdump_device("modem_sw");

	if (!modemsw_ramdump_dev) {
		pr_err("%s: Unable to create modem sw ramdump device. (%d)\n",
				__func__, -ENOMEM);
		ret = -ENOMEM;
		goto out;
	}

	smem_ramdump_dev = create_ramdump_device("smem");

	if (!smem_ramdump_dev) {
		pr_err("%s: Unable to create smem ramdump device. (%d)\n",
				__func__, -ENOMEM);
		ret = -ENOMEM;
		goto out;
	}

	ret = modem_debugfs_init();

	pr_info("%s: modem fatal driver init'ed.\n", __func__);

	if (monitor_modem_hung_wq == NULL) {
		/* Create private workqueue... */
		monitor_modem_hung_wq = create_workqueue("monitor_modem_hung_wq");
		printk(KERN_INFO "Create monitor modem hung workqueue(0x%x)...\n", (unsigned int)monitor_modem_hung_wq);
	}

	if (monitor_modem_hung_wq) {
		pr_info("%s: INIT_DELAYED_WORK: monitor_modem_hung_worker\n", __func__);
		INIT_DELAYED_WORK(&monitor_modem_hung_worker, modem_is_hung);
	}

	htc_err_fatal_handler_magic = (uint32_t *)ioremap(HTC_SMEM_PARAM_BASE_ADDR + HTC_ERR_FATAL_HANDLER_MAGIC_OFFSET, sizeof(uint32_t));
out:
	return ret;
}

module_init(modem_8960_init);
