/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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

#ifndef _MIPI_T6_H_
#define _MIPI_T6_H_

#define BRI_SETTING_MIN	30
#define BRI_SETTING_DEF	142
#define BRI_SETTING_MAX	255

int mipi_t6_device_register(struct msm_panel_info *pinfo,
		u32 channel, u32 panel);

#endif /* _MIPI_T6_H_ */
