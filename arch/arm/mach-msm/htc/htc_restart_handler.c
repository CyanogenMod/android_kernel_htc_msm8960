/* linux/arch/arm/mach-msm/htc_restart_handler.c
 *
 * Copyright (C) 2012 HTC Corporation.
 * Author: Jimmy.CM Chen <jimmy.cm_chen@htc.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/reboot.h>
#include <linux/io.h>

#include <mach/msm_iomap.h>
#include <mach/restart.h>
#include <mach/scm.h>
#include <mach/board_htc.h>
#include <mach/htc_restart_handler.h>

#define RESTART_REASON_ADDR	0xF00
#define MSM_REBOOT_REASON_BASE	(MSM_IMEM_BASE + RESTART_REASON_ADDR)
#define SZ_DIAG_ERR_MSG 	0xC8

struct htc_reboot_params {
	unsigned reboot_reason;
	unsigned radio_flag;
	int 	batt_magic;
	char reserved[256 - SZ_DIAG_ERR_MSG - 22];
	char msg[SZ_DIAG_ERR_MSG];
};

static struct htc_reboot_params *reboot_params;
static atomic_t restart_counter = ATOMIC_INIT(0);

static inline void set_restart_msg(const char *msg)
{
	if (msg) {
		pr_info("%s: set restart msg = `%s'\r\n", __func__, msg);
		strncpy(reboot_params->msg, msg, sizeof(reboot_params->msg)-1);
	}
	else {
		strncpy(reboot_params->msg, "", sizeof(reboot_params->msg)-1);
	}
	mb();
}

unsigned get_restart_reason(void)
{
	return reboot_params->reboot_reason;
}
EXPORT_SYMBOL(get_restart_reason);

static inline void set_restart_reason(unsigned int reason)
{
	pr_info("%s: set restart reason = %08x\r\n", __func__, reason);
	reboot_params->reboot_reason = reason;
	mb();
}

static int panic_restart_action(struct notifier_block *this, unsigned long event, void *ptr)
{
	char kernel_panic_msg[SZ_DIAG_ERR_MSG] = "Kernel Panic";
	if (ptr)
		snprintf(kernel_panic_msg, SZ_DIAG_ERR_MSG-1, "KP: %s", (char *)ptr);
	set_restart_to_ramdump(kernel_panic_msg);

	return NOTIFY_DONE;
}

static struct notifier_block panic_blk = {
	.notifier_call  = panic_restart_action,
};

int set_restart_action(unsigned int reason, const char *msg)
{
	
	if (atomic_read(&restart_counter) != 0) {
		pr_warn("%s: someone call this function before\r\n", __func__);
		return 1;
	}

	atomic_set(&restart_counter, 1);

	set_restart_reason(reason);
	set_restart_msg(msg? msg: "");
	return 0;
}
EXPORT_SYMBOL(set_restart_action);

#ifdef CONFIG_QSC_MODEM
void dump_uart_ringbuffer(void);
#endif

int set_restart_to_oem(unsigned int code, const char *msg)
{
	char oem_msg[SZ_DIAG_ERR_MSG] = "";

	if (msg == NULL)
		sprintf(oem_msg, "oem-%x", code);
	else
		strncpy(oem_msg, msg, (strlen(msg) >= SZ_DIAG_ERR_MSG)? (SZ_DIAG_ERR_MSG - 1): strlen(msg));

#ifdef CONFIG_QSC_MODEM
	if (code == 0x93)
	{
		dump_uart_ringbuffer();
	}
#endif

	
	if ((code >= 0x93) && (code <= 0x98))
		code = 0x99;

	return set_restart_action(RESTART_REASON_OEM_BASE | code, oem_msg);
}
int set_restart_to_ramdump(const char *msg)
{
	return set_restart_action(RESTART_REASON_RAMDUMP, msg);
}
EXPORT_SYMBOL(set_restart_to_ramdump);

int htc_restart_handler_init(void)
{
	reboot_params = (void *)MSM_REBOOT_REASON_BASE;
	reboot_params->radio_flag = get_radio_flag();
	set_restart_reason(RESTART_REASON_RAMDUMP);
	set_restart_msg("Unknown");

	atomic_notifier_chain_register(&panic_notifier_list, &panic_blk);

	return 0;
}
EXPORT_SYMBOL(htc_restart_handler_init);

