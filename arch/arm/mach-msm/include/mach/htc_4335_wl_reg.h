/* arch/arm/mach-msm/include/mach/htc_BCM4335_wl_reg.c
 *
 * Copyright (C) 2012 HTC, Inc.
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

#ifndef __ASM_ARCH_HTC_4335_WL_REG_H
#define __ASM_ARCH_HTC_4335_WL_REG_H

#define ID_WIFI	(0)
#define ID_BT		(1)
#define BCM4335_WL_REG_OFF	(0)
#define BCM4335_WL_REG_ON	(1)

int htc_BCM4335_wl_reg_init(int );
int htc_BCM4335_wl_reg_ctl(int on, int id);

#endif
