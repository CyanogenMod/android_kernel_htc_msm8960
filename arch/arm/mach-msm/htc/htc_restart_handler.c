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
#define MAGIC_NUM_FOR_BATT_SAVE		0xFEDCBA00 
#define HTC_BATT_SAVE_OCV_RAW		(1)
#define HTC_BATT_SAVE_CC		(1<<1)
#define HTC_BATT_SAVE_OCV_UV		(1<<2)

#define BATT_SAVE_MASK		(HTC_BATT_SAVE_OCV_RAW|HTC_BATT_SAVE_CC|HTC_BATT_SAVE_OCV_UV)

struct htc_reboot_params {
	unsigned reboot_reason;
	unsigned radio_flag;
	int 	batt_magic;
	int	cc_backup_uv;
	int 	ocv_backup_uv;
	uint16_t ocv_reading_at_100;
	char reserved[256 - SZ_DIAG_ERR_MSG - 22];
	char msg[SZ_DIAG_ERR_MSG];
};

static struct htc_reboot_params *reboot_params;
static atomic_t restart_counter = ATOMIC_INIT(0);

int read_backup_cc_uv(void)
{
	pr_info("%s: cc_backup_uv=%d, magic=%x\n", __func__,
		reboot_params->cc_backup_uv, reboot_params->batt_magic);
	if((reboot_params->batt_magic & MAGIC_NUM_FOR_BATT_SAVE)
		== MAGIC_NUM_FOR_BATT_SAVE) {
		if ((reboot_params->batt_magic & BATT_SAVE_MASK) <= BATT_SAVE_MASK) {
			if ((reboot_params->batt_magic & HTC_BATT_SAVE_CC)
				== HTC_BATT_SAVE_CC)
				return reboot_params->cc_backup_uv;
		}
	}
	return 0;
}
EXPORT_SYMBOL(read_backup_cc_uv);

void write_backup_cc_uv(int cc_reading)
{
	pr_info("%s: ori cc_backup_uv= %d, cc_reading=%d\n", __func__,
		reboot_params->cc_backup_uv, cc_reading);
	if ((reboot_params->batt_magic & MAGIC_NUM_FOR_BATT_SAVE)
		!= MAGIC_NUM_FOR_BATT_SAVE)
		reboot_params->batt_magic = MAGIC_NUM_FOR_BATT_SAVE;
	reboot_params->batt_magic |= HTC_BATT_SAVE_CC;

	reboot_params->cc_backup_uv = cc_reading;
	mb();
}
EXPORT_SYMBOL(write_backup_cc_uv);

uint16_t read_backup_ocv_at_100(void)
{
	pr_info("%s: ocv_at_100=%x, magic=%x\n", __func__,
		reboot_params->ocv_reading_at_100, reboot_params->batt_magic);
	if((reboot_params->batt_magic & MAGIC_NUM_FOR_BATT_SAVE)
		== MAGIC_NUM_FOR_BATT_SAVE) {
		if ((reboot_params->batt_magic & BATT_SAVE_MASK) <= BATT_SAVE_MASK) {
			if ((reboot_params->batt_magic & HTC_BATT_SAVE_OCV_RAW)
				== HTC_BATT_SAVE_OCV_RAW)
				return reboot_params->ocv_reading_at_100;
		}
	}
	return 0;
}
EXPORT_SYMBOL(read_backup_ocv_at_100);

void write_backup_ocv_at_100(uint16_t ocv_reading)
{
	pr_info("%s: ori ocv_at_100=%x, ocv_reading=%x\n", __func__,
		reboot_params->ocv_reading_at_100, ocv_reading);
	if((reboot_params->batt_magic & MAGIC_NUM_FOR_BATT_SAVE)
		!= MAGIC_NUM_FOR_BATT_SAVE)
		reboot_params->batt_magic = MAGIC_NUM_FOR_BATT_SAVE;
	reboot_params->batt_magic |= HTC_BATT_SAVE_OCV_RAW;

	reboot_params->ocv_reading_at_100 = ocv_reading;
	mb();
}
EXPORT_SYMBOL(write_backup_ocv_at_100);

int read_backup_ocv_uv(void)
{
	pr_info("%s: ocv_backup_uv=%d, magic=%x\n", __func__,
		reboot_params->ocv_backup_uv, reboot_params->batt_magic);
	if((reboot_params->batt_magic & MAGIC_NUM_FOR_BATT_SAVE)
		== MAGIC_NUM_FOR_BATT_SAVE) {
		if ((reboot_params->batt_magic & BATT_SAVE_MASK) <= BATT_SAVE_MASK) {
			if ((reboot_params->batt_magic & HTC_BATT_SAVE_OCV_UV)
				== HTC_BATT_SAVE_OCV_UV)
				return reboot_params->ocv_backup_uv;
		}
	}
	return 0;
}
EXPORT_SYMBOL(read_backup_ocv_uv);

void write_backup_ocv_uv(int ocv_backup)
{
	pr_info("%s: ori ocv_backup_uv=%d, ocv_backup=%d\n", __func__,
		reboot_params->ocv_backup_uv, ocv_backup);
	if((reboot_params->batt_magic & MAGIC_NUM_FOR_BATT_SAVE)
		!= MAGIC_NUM_FOR_BATT_SAVE)
		reboot_params->batt_magic = MAGIC_NUM_FOR_BATT_SAVE;
	reboot_params->batt_magic |= HTC_BATT_SAVE_OCV_UV;

	reboot_params->ocv_backup_uv = ocv_backup;
	mb();
}
EXPORT_SYMBOL(write_backup_ocv_uv);

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

