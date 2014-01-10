/* linux/arch/arm/mach-msm/board-m7-wifi.h
 *
 * Copyright (C) 2008 HTC Corporation.
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

extern unsigned char *get_wifi_nvs_ram(void);
extern int m7_wifi_power(int on);
extern int m7_wifi_reset(int on);
extern int m7_wifi_set_carddetect(int on);
