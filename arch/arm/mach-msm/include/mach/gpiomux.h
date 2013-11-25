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
 */
#ifndef __ARCH_ARM_MACH_MSM_GPIOMUX_H
#define __ARCH_ARM_MACH_MSM_GPIOMUX_H

#include <linux/bitops.h>
#include <linux/errno.h>

enum msm_gpiomux_setting {
	GPIOMUX_ACTIVE = 0,
	GPIOMUX_SUSPENDED,
	GPIOMUX_NSETTINGS
};

enum gpiomux_drv {
	GPIOMUX_DRV_2MA = 0,
	GPIOMUX_DRV_4MA,
	GPIOMUX_DRV_6MA,
	GPIOMUX_DRV_8MA,
	GPIOMUX_DRV_10MA,
	GPIOMUX_DRV_12MA,
	GPIOMUX_DRV_14MA,
	GPIOMUX_DRV_16MA,
};

enum gpiomux_func {
	GPIOMUX_FUNC_GPIO = 0,
	GPIOMUX_FUNC_1,
	GPIOMUX_FUNC_2,
	GPIOMUX_FUNC_3,
	GPIOMUX_FUNC_4,
	GPIOMUX_FUNC_5,
	GPIOMUX_FUNC_6,
	GPIOMUX_FUNC_7,
	GPIOMUX_FUNC_8,
	GPIOMUX_FUNC_9,
	GPIOMUX_FUNC_A,
	GPIOMUX_FUNC_B,
	GPIOMUX_FUNC_C,
	GPIOMUX_FUNC_D,
	GPIOMUX_FUNC_E,
	GPIOMUX_FUNC_F,
};

enum gpiomux_pull {
	GPIOMUX_PULL_NONE = 0,
	GPIOMUX_PULL_DOWN,
	GPIOMUX_PULL_KEEPER,
	GPIOMUX_PULL_UP,
};

enum gpiomux_dir {
	GPIOMUX_IN = 0,
	GPIOMUX_OUT_HIGH,
	GPIOMUX_OUT_LOW,
};

struct gpiomux_setting {
	enum gpiomux_func func;
	enum gpiomux_drv  drv;
	enum gpiomux_pull pull;
	enum gpiomux_dir  dir;
};

struct msm_gpiomux_config {
	unsigned gpio;
	struct gpiomux_setting *settings[GPIOMUX_NSETTINGS];
};

struct msm_gpiomux_configs {
	struct msm_gpiomux_config *cfg;
	size_t                     ncfg;
};

#ifdef CONFIG_MSM_GPIOMUX

int msm_gpiomux_init(size_t ngpio);

void msm_gpiomux_install(struct msm_gpiomux_config *configs, unsigned nconfigs);

int __must_check msm_gpiomux_get(unsigned gpio);

int msm_gpiomux_put(unsigned gpio);

/* Install a new setting in a gpio.  To erase a slot, use NULL.
 * The old setting that was overwritten can be passed back to the caller
 * old_setting can be NULL if the caller is not interested in the previous
 * setting
 * If a previous setting was not available to return (NULL configuration)
 * - the function returns 1
 * else function returns 0
 */
int msm_gpiomux_write(unsigned gpio, enum msm_gpiomux_setting which,
	struct gpiomux_setting *setting, struct gpiomux_setting *old_setting);

void __msm_gpiomux_write(unsigned gpio, struct gpiomux_setting val);
#else
static inline int msm_gpiomux_init(size_t ngpio)
{
	return -ENOSYS;
}

static inline void
msm_gpiomux_install(struct msm_gpiomux_config *configs, unsigned nconfigs) {}

static inline int __must_check msm_gpiomux_get(unsigned gpio)
{
	return -ENOSYS;
}

static inline int msm_gpiomux_put(unsigned gpio)
{
	return -ENOSYS;
}

static inline int msm_gpiomux_write(unsigned gpio,
	enum msm_gpiomux_setting which, struct gpiomux_setting *setting,
	struct gpiomux_setting *old_setting)
{
	return -ENOSYS;
}
#endif
#endif
