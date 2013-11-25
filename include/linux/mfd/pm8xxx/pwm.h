/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#ifndef __PM8XXX_PWM_H__
#define __PM8XXX_PWM_H__

#include <linux/pwm.h>

#define PM8XXX_PWM_DEV_NAME	"pm8xxx-pwm"

#define PM8XXX_PWM_PERIOD_MIN	7 
#define PM8XXX_PWM_PERIOD_MAX	(384 * USEC_PER_SEC) 
#define PM_PWM_LUT_SIZE			64
#define PM_PWM_LUT_DUTY_TIME_MAX	512	
#define PM_PWM_LUT_PAUSE_MAX		(7000 * PM_PWM_LUT_DUTY_TIME_MAX)

#define PM_PWM_LUT_LOOP		0x01
#define PM_PWM_LUT_RAMP_UP	0x02
#define PM_PWM_LUT_REVERSE	0x04
#define PM_PWM_LUT_PAUSE_HI_EN	0x10
#define PM_PWM_LUT_PAUSE_LO_EN	0x20

#define PM_PWM_LUT_NO_TABLE	0x100


enum pm_pwm_size {
	PM_PWM_SIZE_6BIT =	6,
	PM_PWM_SIZE_9BIT =	9,
};

enum pm_pwm_clk {
	PM_PWM_CLK_1KHZ,
	PM_PWM_CLK_32KHZ,
	PM_PWM_CLK_19P2MHZ,
};

enum pm_pwm_pre_div {
	PM_PWM_PDIV_2,
	PM_PWM_PDIV_3,
	PM_PWM_PDIV_5,
	PM_PWM_PDIV_6,
};

struct pm8xxx_pwm_period {
	enum pm_pwm_size	pwm_size;
	enum pm_pwm_clk		clk;
	enum pm_pwm_pre_div	pre_div;
	int			pre_div_exp;
};

struct pm8xxx_pwm_duty_cycles {
	int *duty_pcts;
	int num_duty_pcts;
	int duty_ms;
	int start_idx;
};

struct pm8xxx_pwm_platform_data {
	int dtest_channel;
};

int pm8xxx_pwm_config_period(struct pwm_device *pwm,
			     struct pm8xxx_pwm_period *pwm_p);

int pm8xxx_pwm_config_pwm_value(struct pwm_device *pwm, int pwm_value);

int pm8xxx_pwm_lut_config(struct pwm_device *pwm, int period_us,
			  int duty_pct[], int duty_time_ms, int start_idx,
			  int len, int pause_lo, int pause_hi, int flags);

int pm8xxx_pwm_lut_enable(struct pwm_device *pwm, int start);






#endif 
