/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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

#ifndef _MSM_DSPS_H_
#define _MSM_DSPS_H_

#include <linux/types.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>

#define DSPS_SIGNATURE	0x12345678

struct dsps_clk_info {
	const char *name;
	u32 rate;
	struct clk *clock;
};

struct dsps_gpio_info {
	const char *name;
	int num;
	int on_val;
	int off_val;
	int is_owner;
};

struct dsps_regulator_info {
	const char *name;
	int volt;
	struct regulator *reg;
};

struct msm_dsps_platform_data {
	const char *pil_name;
	struct dsps_clk_info *clks;
	int clks_num;
	struct dsps_gpio_info *gpios;
	int gpios_num;
	struct dsps_regulator_info *regs;
	int regs_num;
	int dsps_pwr_ctl_en;
	void (*init)(struct msm_dsps_platform_data *data);
	int tcm_code_start;
	int tcm_code_size;
	int tcm_buf_start;
	int tcm_buf_size;
	int pipe_start;
	int pipe_size;
	int ddr_start;
	int ddr_size;
	int smem_start;
	int smem_size;
	int ppss_pause_reg;
	u32 signature;
};

#endif 
