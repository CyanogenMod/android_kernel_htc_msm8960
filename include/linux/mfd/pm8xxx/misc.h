/*
 * Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#ifndef __MFD_PM8XXX_MISC_H__
#define __MFD_PM8XXX_MISC_H__

#include <linux/err.h>

#define PM8XXX_MISC_DEV_NAME	"pm8xxx-misc"

struct pm8xxx_misc_platform_data {
	int	priority;
};

enum pm8xxx_uart_path_sel {
	UART_NONE,
	UART_TX1_RX1,
	UART_TX2_RX2,
	UART_TX3_RX3,
};

enum pm8xxx_coincell_chg_voltage {
	PM8XXX_COINCELL_VOLTAGE_3p2V = 1,
	PM8XXX_COINCELL_VOLTAGE_3p1V,
	PM8XXX_COINCELL_VOLTAGE_3p0V,
	PM8XXX_COINCELL_VOLTAGE_2p5V = 16
};

enum pm8xxx_coincell_chg_resistor {
	PM8XXX_COINCELL_RESISTOR_2100_OHMS,
	PM8XXX_COINCELL_RESISTOR_1700_OHMS,
	PM8XXX_COINCELL_RESISTOR_1200_OHMS,
	PM8XXX_COINCELL_RESISTOR_800_OHMS
};

enum pm8xxx_coincell_chg_state {
	PM8XXX_COINCELL_CHG_DISABLE,
	PM8XXX_COINCELL_CHG_ENABLE
};

struct pm8xxx_coincell_chg {
	enum pm8xxx_coincell_chg_state		state;
	enum pm8xxx_coincell_chg_voltage	voltage;
	enum pm8xxx_coincell_chg_resistor	resistor;
};

enum pm8xxx_smpl_delay {
	PM8XXX_SMPL_DELAY_0p5,
	PM8XXX_SMPL_DELAY_1p0,
	PM8XXX_SMPL_DELAY_1p5,
	PM8XXX_SMPL_DELAY_2p0,
};

enum pm8xxx_pon_config {
	PM8XXX_DISABLE_HARD_RESET = 0,
	PM8XXX_SHUTDOWN_ON_HARD_RESET,
	PM8XXX_RESTART_ON_HARD_RESET,
};

enum pm8xxx_aux_clk_id {
	CLK_MP3_1,
	CLK_MP3_2,
};

enum pm8xxx_aux_clk_div {
	XO_DIV_NONE,
	XO_DIV_1,
	XO_DIV_2,
	XO_DIV_4,
	XO_DIV_8,
	XO_DIV_16,
	XO_DIV_32,
	XO_DIV_64,
};

enum pm8xxx_hsed_bias {
	PM8XXX_HSED_BIAS0,
	PM8XXX_HSED_BIAS1,
	PM8XXX_HSED_BIAS2,
};

#if defined(CONFIG_MFD_PM8XXX_MISC) || defined(CONFIG_MFD_PM8XXX_MISC_MODULE)

int pm8xxx_reset_pwr_off(int reset);

int pm8xxx_uart_gpio_mux_ctrl(enum pm8xxx_uart_path_sel uart_path_sel);

int pm8xxx_coincell_chg_config(struct pm8xxx_coincell_chg *chg_config);

int pm8xxx_smpl_control(int enable);

int pm8xxx_smpl_set_delay(enum pm8xxx_smpl_delay delay);

int pm8xxx_watchdog_reset_control(int enable);

int pm8xxx_hard_reset_config(enum pm8xxx_pon_config config);

int pm8xxx_stay_on(void);

int pm8xxx_preload_dVdd(void);

int pm8xxx_usb_id_pullup(int enable);

int pm8xxx_aux_clk_control(enum pm8xxx_aux_clk_id clk_id,
				enum pm8xxx_aux_clk_div divider,
				bool enable);

int pm8xxx_hsed_bias_control(enum pm8xxx_hsed_bias bias, bool enable);
#else

static inline int pm8xxx_reset_pwr_off(int reset)
{
	return -ENODEV;
}
static inline int
pm8xxx_uart_gpio_mux_ctrl(enum pm8xxx_uart_path_sel uart_path_sel)
{
	return -ENODEV;
}
static inline int
pm8xxx_coincell_chg_config(struct pm8xxx_coincell_chg *chg_config)
{
	return -ENODEV;
}
static inline int pm8xxx_smpl_set_delay(enum pm8xxx_smpl_delay delay)
{
	return -ENODEV;
}
static inline int pm8xxx_smpl_control(int enable)
{
	return -ENODEV;
}
static inline int pm8xxx_watchdog_reset_control(int enable)
{
	return -ENODEV;
}
static inline int pm8xxx_hard_reset_config(enum pm8xxx_pon_config config)
{
	return -ENODEV;
}
static inline int pm8xxx_stay_on(void)
{
	return -ENODEV;
}
static inline int pm8xxx_preload_dVdd(void)
{
	return -ENODEV;
}
static inline int pm8xxx_usb_id_pullup(int enable)
{
	return -ENODEV;
}
static inline int pm8xxx_aux_clk_control(enum pm8xxx_aux_clk_id clk_id,
			enum pm8xxx_aux_clk_div divider, bool enable)
{
	return -ENODEV;
}
static inline int pm8xxx_hsed_bias_control(enum pm8xxx_hsed_bias bias,
							bool enable)
{
	return -ENODEV;
}

#endif

#endif
