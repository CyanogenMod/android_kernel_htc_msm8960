/* Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
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

#ifndef __PMIC8XXX_CCADC_H__
#define __PMIC8XXX_CCADC_H__

#include <linux/mfd/pm8xxx/core.h>

#define PM8XXX_CCADC_DEV_NAME "pm8xxx-ccadc"

struct pm8xxx_ccadc_platform_data {
	int		r_sense;
	unsigned int	calib_delay_ms;
};

#define CCADC_READING_RESOLUTION_N	542535
#define CCADC_READING_RESOLUTION_D	100000

static inline s64 pm8xxx_ccadc_reading_to_microvolt(int revision, s64 cc)
{
	return div_s64(cc * CCADC_READING_RESOLUTION_N,
					CCADC_READING_RESOLUTION_D);
}

#if defined(CONFIG_PM8XXX_CCADC) || defined(CONFIG_PM8XXX_CCADC_MODULE)
s64 pm8xxx_cc_adjust_for_gain(s64 uv);

void pm8xxx_calib_ccadc(void);

int pm8xxx_ccadc_get_battery_current(int *bat_current);
int pm8xxx_ccadc_dump_all(void);
int pm8xxx_ccadc_get_attr_text(char *buf, int size);
#else
static inline s64 pm8xxx_cc_adjust_for_gain(s64 uv)
{
	return -ENXIO;
}
static inline void pm8xxx_calib_ccadc(void)
{
}
static inline int pm8xxx_ccadc_get_battery_current(int *bat_current)
{
	return -ENXIO;
}
static inline int pm8xxx_ccadc_dump_all(void)
{
	return -ENXIO;
}
static inline int pm8xxx_ccadc_get_attr_text(char *buf, int size)
{
	return 0;
}
#endif

#endif 
