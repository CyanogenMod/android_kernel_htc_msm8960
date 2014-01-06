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

#define RESTART_REASON_BOOT_BASE        0x77665500
#define RESTART_REASON_BOOTLOADER       (RESTART_REASON_BOOT_BASE | 0x00)
#define RESTART_REASON_REBOOT           (RESTART_REASON_BOOT_BASE | 0x01)
#define RESTART_REASON_RECOVERY         (RESTART_REASON_BOOT_BASE | 0x02)
#define RESTART_REASON_ERASE_EFS        (RESTART_REASON_BOOT_BASE | 0x03)
#define RESTART_REASON_RAMDUMP          (RESTART_REASON_BOOT_BASE | 0xAA)
#define RESTART_REASON_POWEROFF         (RESTART_REASON_BOOT_BASE | 0xBB)
#define RESTART_REASON_ERASE_FLASH      (RESTART_REASON_BOOT_BASE | 0xEF)

#define RESTART_REASON_OEM_BASE         0x6f656d00
#define RESTART_REASON_RIL_FATAL        (RESTART_REASON_OEM_BASE | 0x99)

int read_backup_cc_uv(void);
void write_backup_cc_uv(int cc_reading);
uint16_t read_backup_ocv_at_100(void);
void write_backup_ocv_at_100(uint16_t ocv_reading);
int read_backup_ocv_uv(void);
void write_backup_ocv_uv(int ocv_backup);
int set_restart_action(unsigned int reason, const char *msg);
int set_restart_to_oem(unsigned int code, const char *msg);
int set_restart_to_ramdump(const char *msg);
int htc_restart_handler_init(void);
unsigned get_restart_reason(void);
