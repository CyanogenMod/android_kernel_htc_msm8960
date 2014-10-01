/* arch/arm/mach-msm/include/mach/htc_fmtx_rfkill.h
 *
 * Copyright (C) 2010 HTC, Inc.
 * Author: assd bt <assd_bt@htc.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ASM_ARCH_HTC_SLEEP_CLK_H
#define __ASM_ARCH_HTC_SLEEP_CLK_H

#define ID_WIFI	(0)
#define ID_BT	(1)
#define CLK_OFF	(0)
#define CLK_ON	(1)

#define M4_SLEEP_CLK_PIN 8

int htc_wifi_bt_sleep_clk_ctl(int on, int id);

#endif
