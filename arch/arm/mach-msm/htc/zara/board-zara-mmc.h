/* linux/arch/arm/mach-msm/board-zara-mmc.h
 *
 * Copyright (C) 2011 HTC Corporation.
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

struct mmc_platform_data;

int msm_add_sdcc(unsigned int controller, struct mmc_platform_data *plat);
int zara_wifi_power(int on);
int zara_wifi_set_carddetect(int on);
