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
#ifndef __ISL9519_H__
#define __ISL9519_H__

struct isl_platform_data {
	int chgcurrent;
	int valid_n_gpio;
	int (*chg_detection_config) (void);
	int max_system_voltage;
	int min_system_voltage;
	int term_current;
	int input_current;
};

#endif
