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

#ifndef __LEDS_PM8XXX_H__
#define __LEDS_PM8XXX_H__

#include <linux/kernel.h>
#include <linux/mfd/pm8xxx/pwm.h>

#define PM8XXX_LEDS_DEV_NAME	"pm8xxx-led"

enum pm8xxx_blink_type {
	BLINK_STOP = -1,
	BLINK_UNCHANGE = 0,
	BLINK_64MS_PER_2SEC,
	BLINK_64MS_ON_310MS_PER_2SEC,
	BLINK_64MS_ON_2SEC_PER_2SEC,
	BLINK_1SEC_PER_2SEC,
	key_blink_on,
	key_blink_off,
};

enum pm8xxx_leds {
	PM8XXX_ID_LED_KB_LIGHT = 1,
	PM8XXX_ID_LED_0,
	PM8XXX_ID_LED_1,
	PM8XXX_ID_LED_2,
	PM8XXX_ID_FLASH_LED_0,
	PM8XXX_ID_FLASH_LED_1,
	PM8XXX_ID_WLED,
	PM8XXX_ID_RGB_LED_RED,
	PM8XXX_ID_RGB_LED_GREEN,
	PM8XXX_ID_RGB_LED_BLUE,
	PM8XXX_ID_MAX,
};

enum pm8xxx_led_modes {
	PM8XXX_LED_MODE_MANUAL,
	PM8XXX_LED_MODE_PWM1,
	PM8XXX_LED_MODE_PWM2,
	PM8XXX_LED_MODE_PWM3,
	PM8XXX_LED_MODE_DTEST1,
	PM8XXX_LED_MODE_DTEST2,
	PM8XXX_LED_MODE_DTEST3,
	PM8XXX_LED_MODE_DTEST4
};

enum wled_current_bost_limit {
	WLED_CURR_LIMIT_105mA,
	WLED_CURR_LIMIT_385mA,
	WLED_CURR_LIMIT_525mA,
	WLED_CURR_LIMIT_805mA,
	WLED_CURR_LIMIT_980mA,
	WLED_CURR_LIMIT_1260mA,
	WLED_CURR_LIMIT_1400mA,
	WLED_CURR_LIMIT_1680mA,
};

enum wled_ovp_threshold {
	WLED_OVP_35V,
	WLED_OVP_32V,
	WLED_OVP_29V,
	WLED_OVP_37V,
};

struct wled_config_data {
	u8	num_strings;
	u8	ovp_val;
	u8	boost_curr_lim;
	u8	cp_select;
	u8	ctrl_delay_us;
	bool	dig_mod_gen_en;
	bool	cs_out_en;
	bool	op_fdbck;
};

struct pm8xxx_led_config {
	u8	id;
	u8	mode;
	u16	max_current;
	int	pwm_channel;
	u32	pwm_period_us;
	bool	default_state;
	struct pm8xxx_pwm_duty_cycles *pwm_duty_cycles;
	struct wled_config_data	*wled_cfg;
	int		pwm_coefficient;
	int		lut_coefficient;
};

struct pm8xxx_led_platform_data {
	struct	led_platform_data	*led_core;
	struct	pm8xxx_led_config	*configs;
	u32				num_configs;
};
void pm8xxx_led_current_set_for_key(int brightness_key);

#endif 
