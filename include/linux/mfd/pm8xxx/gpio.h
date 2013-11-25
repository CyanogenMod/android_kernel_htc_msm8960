/*
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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


#ifndef __PM8XXX_GPIO_H
#define __PM8XXX_GPIO_H

#include <linux/errno.h>
#include <linux/seq_file.h>

#define PM8XXX_GPIO_DEV_NAME	"pm8xxx-gpio"

struct pm8xxx_gpio_core_data {
	int	ngpios;
};

struct pm8xxx_gpio_platform_data {
	struct pm8xxx_gpio_core_data	gpio_cdata;
	int				gpio_base;
};

#define	PM_GPIO_DIR_OUT			0x01
#define	PM_GPIO_DIR_IN			0x02
#define	PM_GPIO_DIR_BOTH		(PM_GPIO_DIR_OUT | PM_GPIO_DIR_IN)

#define	PM_GPIO_OUT_BUF_OPEN_DRAIN	1
#define	PM_GPIO_OUT_BUF_CMOS		0

#define	PM_GPIO_PULL_UP_30		0
#define	PM_GPIO_PULL_UP_1P5		1
#define	PM_GPIO_PULL_UP_31P5		2
#define	PM_GPIO_PULL_UP_1P5_30		3
#define	PM_GPIO_PULL_DN			4
#define	PM_GPIO_PULL_NO			5

#define	PM_GPIO_VIN_VPH			0 
#define	PM_GPIO_VIN_BB			1 
#define	PM_GPIO_VIN_S4			2 
#define	PM_GPIO_VIN_L15			3
#define	PM_GPIO_VIN_L4			4
#define	PM_GPIO_VIN_L3			5
#define	PM_GPIO_VIN_L17			6

#define PM8058_GPIO_VIN_VPH		0
#define PM8058_GPIO_VIN_BB		1
#define PM8058_GPIO_VIN_S3		2
#define PM8058_GPIO_VIN_L3		3
#define PM8058_GPIO_VIN_L7		4
#define PM8058_GPIO_VIN_L6		5
#define PM8058_GPIO_VIN_L5		6
#define PM8058_GPIO_VIN_L2		7

#define PM8038_GPIO_VIN_VPH		0
#define PM8038_GPIO_VIN_BB		1
#define PM8038_GPIO_VIN_L11		2
#define PM8038_GPIO_VIN_L15		3
#define PM8038_GPIO_VIN_L4		4
#define PM8038_GPIO_VIN_L3		5
#define PM8038_GPIO_VIN_L17		6

#define PM8018_GPIO_VIN_L4		0
#define PM8018_GPIO_VIN_L14		1
#define PM8018_GPIO_VIN_S3		2
#define PM8018_GPIO_VIN_L6		3
#define PM8018_GPIO_VIN_L2		4
#define PM8018_GPIO_VIN_L5		5
#define PM8018_GPIO_VIN_L8		6
#define PM8018_GPIO_VIN_VPH		7

#define	PM_GPIO_STRENGTH_NO		0
#define	PM_GPIO_STRENGTH_HIGH		1
#define	PM_GPIO_STRENGTH_MED		2
#define	PM_GPIO_STRENGTH_LOW		3

#define	PM_GPIO_FUNC_NORMAL		0
#define	PM_GPIO_FUNC_PAIRED		1
#define	PM_GPIO_FUNC_1			2
#define	PM_GPIO_FUNC_2			3
#define	PM_GPIO_DTEST1			4
#define	PM_GPIO_DTEST2			5
#define	PM_GPIO_DTEST3			6
#define	PM_GPIO_DTEST4			7

struct pm_gpio {
	int		direction;
	int		output_buffer;
	int		output_value;
	int		pull;
	int		vin_sel;
	int		out_strength;
	int		function;
	int		inv_int_pol;
	int		disable_pin;
};

#if defined(CONFIG_GPIO_PM8XXX) || defined(CONFIG_GPIO_PM8XXX_MODULE)
int pm8xxx_gpio_config(int gpio, struct pm_gpio *param);
int pm8xxx_dump_gpios(struct seq_file *m, int curr_len, char *gpio_buffer);
#else
static inline int pm8xxx_gpio_config(int gpio, struct pm_gpio *param)
{
	return -ENXIO;
}
static inline int pm8xxx_dump_gpios(struct seq_file *m, int curr_len, char *gpio_buffer)
{
	return curr_len;
}
#endif

#endif
