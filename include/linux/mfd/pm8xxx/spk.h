/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __SPK_PM8XXX_H__
#define __SPK_PM8XXX_H__

#define PM8XXX_SPK_DEV_NAME     "pm8xxx-spk"

struct pm8xxx_spk_platform_data {
	bool spk_add_enable;
};

int pm8xxx_spk_mute(bool mute);

int pm8xxx_spk_gain(u8 gain);

int pm8xxx_spk_enable(int enable);

#endif 
