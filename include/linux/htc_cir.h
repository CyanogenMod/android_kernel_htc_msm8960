/* include/linux/htc_cir.h
 *
 * Copyright (C) 2011 HTC, Inc.
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

#ifndef __LINUX_HTC_CIR_H
#define __LINUX_HTC_CIR_H

struct cir_platform_data {
	int (*cir_set_path)(int);
	int (*cir_reset)(void);
	int (*cir_power)(int);
};

enum irda_path {
	PATH_NONE = 0,
	PATH_IRDA,
	PATH_CIR,
};

#endif
