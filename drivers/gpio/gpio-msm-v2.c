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
 *
 */
#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/irq.h>


#include <mach/msm_iomap.h>
#include <mach/gpiomux.h>
#include "gpio-msm-common.h"

enum {
	GPIO_IN_BIT  = 0,
	GPIO_OUT_BIT = 1
};

enum {
	INTR_STATUS_BIT = 0,
};

enum {
	GPIO_OE_BIT = 9,
};

enum {
	INTR_ENABLE_BIT        = 0,
	INTR_POL_CTL_BIT       = 1,
	INTR_DECT_CTL_BIT      = 2,
	INTR_RAW_STATUS_EN_BIT = 3,
};

enum {
	TARGET_PROC_SCORPION = 4,
	TARGET_PROC_NONE     = 7,
};

enum {
	DC_POLARITY_HI	= BIT(11),
	DC_IRQ_ENABLE	= BIT(3),
};

#define INTR_RAW_STATUS_EN BIT(INTR_RAW_STATUS_EN_BIT)
#define INTR_ENABLE        BIT(INTR_ENABLE_BIT)
#define INTR_DECT_CTL_EDGE BIT(INTR_DECT_CTL_BIT)
#define INTR_POL_CTL_HI    BIT(INTR_POL_CTL_BIT)

#define GPIO_INTR_CFG_SU(gpio)    (MSM_TLMM_BASE + 0x0400 + (0x04 * (gpio)))
#define DIR_CONN_INTR_CFG_SU(irq) (MSM_TLMM_BASE + 0x0700 + (0x04 * (irq)))
#define GPIO_CONFIG(gpio)         (MSM_TLMM_BASE + 0x1000 + (0x10 * (gpio)))
#define GPIO_IN_OUT(gpio)         (MSM_TLMM_BASE + 0x1004 + (0x10 * (gpio)))
#define GPIO_INTR_CFG(gpio)       (MSM_TLMM_BASE + 0x1008 + (0x10 * (gpio)))
#define GPIO_INTR_STATUS(gpio)    (MSM_TLMM_BASE + 0x100c + (0x10 * (gpio)))

static inline void set_gpio_bits(unsigned n, void __iomem *reg)
{
	__raw_writel(__raw_readl(reg) | n, reg);
}

static inline void clr_gpio_bits(unsigned n, void __iomem *reg)
{
	__raw_writel(__raw_readl(reg) & ~n, reg);
}

unsigned __msm_gpio_get_inout(unsigned gpio)
{
	return __raw_readl(GPIO_IN_OUT(gpio)) & BIT(GPIO_IN_BIT);
}

void __msm_gpio_set_inout(unsigned gpio, unsigned val)
{
	__raw_writel(val ? BIT(GPIO_OUT_BIT) : 0, GPIO_IN_OUT(gpio));
}

void __msm_gpio_set_config_direction(unsigned gpio, int input, int val)
{
	if (input)
		clr_gpio_bits(BIT(GPIO_OE_BIT), GPIO_CONFIG(gpio));
	else {
		__msm_gpio_set_inout(gpio, val);
		set_gpio_bits(BIT(GPIO_OE_BIT), GPIO_CONFIG(gpio));
	}
}

void __msm_gpio_set_polarity(unsigned gpio, unsigned val)
{
	if (val)
		clr_gpio_bits(INTR_POL_CTL_HI, GPIO_INTR_CFG(gpio));
	else
		set_gpio_bits(INTR_POL_CTL_HI, GPIO_INTR_CFG(gpio));
}

unsigned __msm_gpio_get_intr_status(unsigned gpio)
{
	return __raw_readl(GPIO_INTR_STATUS(gpio)) &
					BIT(INTR_STATUS_BIT);
}

void __msm_gpio_set_intr_status(unsigned gpio)
{
	__raw_writel(BIT(INTR_STATUS_BIT), GPIO_INTR_STATUS(gpio));
}

unsigned __msm_gpio_get_intr_config(unsigned gpio)
{
	return __raw_readl(GPIO_INTR_CFG(gpio));
}

void __msm_gpio_set_intr_cfg_enable(unsigned gpio, unsigned val)
{
	if (val) {
		set_gpio_bits(INTR_ENABLE, GPIO_INTR_CFG(gpio));

	} else {
		clr_gpio_bits(INTR_ENABLE, GPIO_INTR_CFG(gpio));
	}
}

unsigned  __msm_gpio_get_intr_cfg_enable(unsigned gpio)
{
	return 	__msm_gpio_get_intr_config(gpio) & INTR_ENABLE;
}

void __msm_gpio_set_intr_cfg_type(unsigned gpio, unsigned type)
{
	unsigned cfg;

	cfg  = __msm_gpio_get_intr_config(gpio);
	cfg |= INTR_RAW_STATUS_EN;
	__raw_writel(cfg, GPIO_INTR_CFG(gpio));
	__raw_writel(TARGET_PROC_SCORPION, GPIO_INTR_CFG_SU(gpio));

	cfg  = __msm_gpio_get_intr_config(gpio);
	if (type & IRQ_TYPE_EDGE_BOTH)
		cfg |= INTR_DECT_CTL_EDGE;
	else
		cfg &= ~INTR_DECT_CTL_EDGE;

	if (type & (IRQ_TYPE_EDGE_RISING | IRQ_TYPE_LEVEL_HIGH))
		cfg |= INTR_POL_CTL_HI;
	else
		cfg &= ~INTR_POL_CTL_HI;

	__raw_writel(cfg, GPIO_INTR_CFG(gpio));
	udelay(5);
}

void __gpio_tlmm_config(unsigned config)
{
	uint32_t flags;
	unsigned gpio = GPIO_PIN(config);

	flags = ((GPIO_DIR(config) << 9) & (0x1 << 9)) |
		((GPIO_DRVSTR(config) << 6) & (0x7 << 6)) |
		((GPIO_FUNC(config) << 2) & (0xf << 2)) |
		((GPIO_PULL(config) & 0x3));
	__raw_writel(flags, GPIO_CONFIG(gpio));
}

void __msm_gpio_install_direct_irq(unsigned gpio, unsigned irq,
					unsigned int input_polarity)
{
	uint32_t bits;

	__raw_writel(__raw_readl(GPIO_CONFIG(gpio)) | BIT(GPIO_OE_BIT),
		GPIO_CONFIG(gpio));
	__raw_writel(__raw_readl(GPIO_INTR_CFG(gpio)) &
		~(INTR_RAW_STATUS_EN | INTR_ENABLE),
		GPIO_INTR_CFG(gpio));
	__raw_writel(DC_IRQ_ENABLE | TARGET_PROC_NONE,
		GPIO_INTR_CFG_SU(gpio));

	bits = TARGET_PROC_SCORPION | (gpio << 3);
	if (input_polarity)
		bits |= DC_POLARITY_HI;
	__raw_writel(bits, DIR_CONN_INTR_CFG_SU(irq));
}

#if defined(CONFIG_DEBUG_FS)
#define GPIO_FUNC_SEL_BIT 2
#define GPIO_DRV_BIT 6

int _gpio_debug_direction_set(void *data, u64 val)
{
	int *id = data;

	if (val)
		clr_gpio_bits(BIT(GPIO_OE_BIT), GPIO_CONFIG(*id));
	else
		set_gpio_bits(BIT(GPIO_OE_BIT), GPIO_CONFIG(*id));

	return 0;
}

int _gpio_debug_direction_get(void *data, u64 *val)
{
	int *id = data;

	*val = ((readl(GPIO_CONFIG(*id)) & BIT(GPIO_OE_BIT))>>GPIO_OE_BIT)? 0 : 1;

	return 0;
}

int _gpio_debug_level_set(void *data, u64 val)
{
	int *id = data;

	writel(val ? BIT(GPIO_OUT_BIT) : 0, GPIO_IN_OUT(*id));

	return 0;
}

int _gpio_debug_level_get(void *data, u64 *val)
{
	int *id = data;
	int dir = (readl(GPIO_CONFIG(*id)) & BIT(GPIO_OE_BIT))>>GPIO_OE_BIT;

	if (dir)
		*val = (readl(GPIO_IN_OUT(*id)) & BIT(GPIO_OUT_BIT))>>GPIO_OUT_BIT;
	else
		*val = readl(GPIO_IN_OUT(*id)) & BIT(GPIO_IN_BIT);

	return 0;
}

int _gpio_debug_drv_set(void *data, u64 val)
{
	int *id = data;

	set_gpio_bits((val << GPIO_DRV_BIT), GPIO_CONFIG(*id));

	return 0;
}

int _gpio_debug_drv_get(void *data, u64 *val)
{
	int *id = data;

	*val = (readl(GPIO_CONFIG(*id)) >> GPIO_DRV_BIT) & 0x7;

	return 0;
}

int _gpio_debug_func_sel_set(void *data, u64 val)
{
	int *id = data;

	set_gpio_bits((val << GPIO_FUNC_SEL_BIT), GPIO_CONFIG(*id));

	return 0;
}

int _gpio_debug_func_sel_get(void *data, u64 *val)
{
	int *id = data;

	*val = (readl(GPIO_CONFIG(*id)) >> GPIO_FUNC_SEL_BIT) & 0x7;

	return 0;
}

int _gpio_debug_pull_set(void *data, u64 val)
{
	int *id = data;

	set_gpio_bits(val, GPIO_CONFIG(*id));

	return 0;
}

int _gpio_debug_pull_get(void *data, u64 *val)
{
	int *id = data;

	*val = readl(GPIO_CONFIG(*id)) & 0x3;

	return 0;
}

int _gpio_debug_int_enable_get(void *data, u64 *val)
{
	int *id = data;

	*val = readl(GPIO_INTR_CFG(*id)) & 0x1;

	return 0;
}

int _gpio_debug_int_owner_set(void *data, u64 val)
{
	int *id = data;

	if (val)
		writel(TARGET_PROC_SCORPION, GPIO_INTR_CFG_SU(*id));
	else
		writel(TARGET_PROC_NONE, GPIO_INTR_CFG_SU(*id));

	return 0;
}

int _gpio_debug_int_owner_get(void *data, u64 *val)
{
	int *id = data;

	*val = readl(GPIO_INTR_CFG_SU(*id)) & 0x7;

	return 0;
}

int _gpio_debug_int_type_get(void *data, u64 *val)
{
	int *id = data;

	*val = (readl(GPIO_INTR_CFG(*id))>>0x1) & 0x3;

	return 0;
}

#endif
