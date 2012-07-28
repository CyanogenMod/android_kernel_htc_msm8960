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

#define pr_fmt(fmt) "%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/mfd/pm8xxx/core.h>
#include <linux/debugfs.h>
#ifdef CONFIG_ARCH_MSM8960
#include <linux/seq_file.h>
#endif
#define PM8XXX_DEBUG_DEV_NAME "pm8xxx-debug"

struct pm8xxx_debug_device {
	struct mutex		debug_mutex;
	struct device		*parent;
	struct dentry		*dir;
	int			addr;
};

static bool pm8xxx_debug_addr_is_valid(int addr)
{
	if (addr < 0 || addr > 0x3FF) {
		pr_err("PMIC register address is invalid: %d\n", addr);
		return false;
	}
	return true;
}

static int pm8xxx_debug_data_set(void *data, u64 val)
{
	struct pm8xxx_debug_device *debugdev = data;
	u8 reg = val;
	int rc;

	mutex_lock(&debugdev->debug_mutex);

	if (pm8xxx_debug_addr_is_valid(debugdev->addr)) {
		rc = pm8xxx_writeb(debugdev->parent, debugdev->addr, reg);

		if (rc)
			pr_err("pm8xxx_writeb(0x%03X)=0x%02X failed: rc=%d\n",
				debugdev->addr, reg, rc);
	}

	mutex_unlock(&debugdev->debug_mutex);
	return 0;
}

static int pm8xxx_debug_data_get(void *data, u64 *val)
{
	struct pm8xxx_debug_device *debugdev = data;
	int rc;
	u8 reg;

	mutex_lock(&debugdev->debug_mutex);

	if (pm8xxx_debug_addr_is_valid(debugdev->addr)) {
		rc = pm8xxx_readb(debugdev->parent, debugdev->addr, &reg);

		if (rc)
			pr_err("pm8xxx_readb(0x%03X) failed: rc=%d\n",
				debugdev->addr, rc);
		else
			*val = reg;
	}

	mutex_unlock(&debugdev->debug_mutex);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_data_fops, pm8xxx_debug_data_get,
			pm8xxx_debug_data_set, "0x%02llX\n");

static int pm8xxx_debug_addr_set(void *data, u64 val)
{
	struct pm8xxx_debug_device *debugdev = data;

	if (pm8xxx_debug_addr_is_valid(val)) {
		mutex_lock(&debugdev->debug_mutex);
		debugdev->addr = val;
		mutex_unlock(&debugdev->debug_mutex);
	}

	return 0;
}

static int pm8xxx_debug_addr_get(void *data, u64 *val)
{
	struct pm8xxx_debug_device *debugdev = data;

	mutex_lock(&debugdev->debug_mutex);

	if (pm8xxx_debug_addr_is_valid(debugdev->addr))
		*val = debugdev->addr;

	mutex_unlock(&debugdev->debug_mutex);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_addr_fops, pm8xxx_debug_addr_get,
			pm8xxx_debug_addr_set, "0x%03llX\n");

#ifdef CONFIG_ARCH_MSM8960

/**
 * enum pm8921_vreg_id - PMIC 8921 regulator ID numbers
 */
enum pm8921_vreg_id {
	PM8921_VREG_ID_L1 = 0,
	PM8921_VREG_ID_L2,
	PM8921_VREG_ID_L3,
	PM8921_VREG_ID_L4,
	PM8921_VREG_ID_L5,
	PM8921_VREG_ID_L6,
	PM8921_VREG_ID_L7,
	PM8921_VREG_ID_L8,
	PM8921_VREG_ID_L9,
	PM8921_VREG_ID_L10,
	PM8921_VREG_ID_L11,
	PM8921_VREG_ID_L12,
	PM8921_VREG_ID_L14,
	PM8921_VREG_ID_L15,
	PM8921_VREG_ID_L16,
	PM8921_VREG_ID_L17,
	PM8921_VREG_ID_L18,
	PM8921_VREG_ID_L21,
	PM8921_VREG_ID_L22,
	PM8921_VREG_ID_L23,
	PM8921_VREG_ID_L24,
	PM8921_VREG_ID_L25,
	PM8921_VREG_ID_L26,
	PM8921_VREG_ID_L27,
	PM8921_VREG_ID_L28,
	PM8921_VREG_ID_L29,
	PM8921_VREG_ID_S1,
	PM8921_VREG_ID_S2,
	PM8921_VREG_ID_S3,
	PM8921_VREG_ID_S4,
	PM8921_VREG_ID_S5,
	PM8921_VREG_ID_S6,
	PM8921_VREG_ID_S7,
	PM8921_VREG_ID_S8,
	PM8921_VREG_ID_LVS1,
	PM8921_VREG_ID_LVS2,
	PM8921_VREG_ID_LVS3,
	PM8921_VREG_ID_LVS4,
	PM8921_VREG_ID_LVS5,
	PM8921_VREG_ID_LVS6,
	PM8921_VREG_ID_LVS7,
	PM8921_VREG_ID_USB_OTG,
	PM8921_VREG_ID_HDMI_MVS,
	PM8921_VREG_ID_NCP,
	/* The following are IDs for regulator devices to enable pin control. */
	PM8921_VREG_ID_L1_PC,
	PM8921_VREG_ID_L2_PC,
	PM8921_VREG_ID_L3_PC,
	PM8921_VREG_ID_L4_PC,
	PM8921_VREG_ID_L5_PC,
	PM8921_VREG_ID_L6_PC,
	PM8921_VREG_ID_L7_PC,
	PM8921_VREG_ID_L8_PC,
	PM8921_VREG_ID_L9_PC,
	PM8921_VREG_ID_L10_PC,
	PM8921_VREG_ID_L11_PC,
	PM8921_VREG_ID_L12_PC,
	PM8921_VREG_ID_L14_PC,
	PM8921_VREG_ID_L15_PC,
	PM8921_VREG_ID_L16_PC,
	PM8921_VREG_ID_L17_PC,
	PM8921_VREG_ID_L18_PC,
	PM8921_VREG_ID_L21_PC,
	PM8921_VREG_ID_L22_PC,
	PM8921_VREG_ID_L23_PC,

	PM8921_VREG_ID_L29_PC,
	PM8921_VREG_ID_S1_PC,
	PM8921_VREG_ID_S2_PC,
	PM8921_VREG_ID_S3_PC,
	PM8921_VREG_ID_S4_PC,

	PM8921_VREG_ID_S7_PC,
	PM8921_VREG_ID_S8_PC,
	PM8921_VREG_ID_LVS1_PC,

	PM8921_VREG_ID_LVS3_PC,
	PM8921_VREG_ID_LVS4_PC,
	PM8921_VREG_ID_LVS5_PC,
	PM8921_VREG_ID_LVS6_PC,
	PM8921_VREG_ID_LVS7_PC,

	PM8921_VREG_ID_MAX,
};

/* Pin control input pins. */
#define PM8921_VREG_PIN_CTRL_NONE	0x00
#define PM8921_VREG_PIN_CTRL_D1		0x01
#define PM8921_VREG_PIN_CTRL_A0		0x02
#define PM8921_VREG_PIN_CTRL_A1		0x04
#define PM8921_VREG_PIN_CTRL_A2		0x08

/* Minimum high power mode loads in uA. */
#define PM8921_VREG_LDO_50_HPM_MIN_LOAD		5000
#define PM8921_VREG_LDO_150_HPM_MIN_LOAD	10000
#define PM8921_VREG_LDO_300_HPM_MIN_LOAD	10000
#define PM8921_VREG_LDO_600_HPM_MIN_LOAD	10000
#define PM8921_VREG_LDO_1200_HPM_MIN_LOAD	10000
#define PM8921_VREG_SMPS_1500_HPM_MIN_LOAD	100000
#define PM8921_VREG_SMPS_2000_HPM_MIN_LOAD	100000

/**
 * enum pm8921_vreg_pin_function - action to perform when pin control is active
 * %PM8921_VREG_PIN_FN_ENABLE:	pin control enables the regulator
 * %PM8921_VREG_PIN_FN_MODE:	pin control changes mode from LPM to HPM
 */
enum pm8921_vreg_pin_function {
	PM8921_VREG_PIN_FN_ENABLE = 0,
	PM8921_VREG_PIN_FN_MODE,
};

#define REGULATOR_TYPE_PLDO		0
#define REGULATOR_TYPE_NLDO		1
#define REGULATOR_TYPE_NLDO1200		2
#define REGULATOR_TYPE_SMPS		3
#define REGULATOR_TYPE_FTSMPS		4
#define REGULATOR_TYPE_VS		5
#define REGULATOR_TYPE_VS300		6
#define REGULATOR_TYPE_NCP		7
/* Common Masks */
#define REGULATOR_ENABLE_MASK		0x80
#define REGULATOR_ENABLE		0x80
#define REGULATOR_DISABLE		0x00

#define REGULATOR_BANK_MASK		0xF0
#define REGULATOR_BANK_SEL(n)		((n) << 4)
#define REGULATOR_BANK_WRITE		0x80

#define LDO_TEST_BANKS			7
#define NLDO1200_TEST_BANKS		5
#define SMPS_TEST_BANKS			8
#define REGULATOR_TEST_BANKS_MAX	SMPS_TEST_BANKS

/*
 * This voltage in uV is returned by get_voltage functions when there is no way
 * to determine the current voltage level.  It is needed because the regulator
 * framework treats a 0 uV voltage as an error.
 */
#define VOLTAGE_UNKNOWN			1

/* LDO masks and values */

/* CTRL register */
#define LDO_ENABLE_MASK			0x80
#define LDO_DISABLE			0x00
#define LDO_ENABLE			0x80
#define LDO_PULL_DOWN_ENABLE_MASK	0x40
#define LDO_PULL_DOWN_ENABLE		0x40

#define LDO_CTRL_PM_MASK		0x20
#define LDO_CTRL_PM_HPM			0x00
#define LDO_CTRL_PM_LPM			0x20

#define LDO_CTRL_VPROG_MASK		0x1F

/* TEST register bank 0 */
#define LDO_TEST_LPM_MASK		0x04
#define LDO_TEST_LPM_SEL_CTRL		0x00
#define LDO_TEST_LPM_SEL_TCXO		0x04

/* TEST register bank 2 */
#define LDO_TEST_VPROG_UPDATE_MASK	0x08
#define LDO_TEST_RANGE_SEL_MASK		0x04
#define LDO_TEST_FINE_STEP_MASK		0x02
#define LDO_TEST_FINE_STEP_SHIFT	1

/* TEST register bank 4 */
#define LDO_TEST_RANGE_EXT_MASK		0x01

/* TEST register bank 5 */
#define LDO_TEST_PIN_CTRL_MASK		0x0F
#define LDO_TEST_PIN_CTRL_EN3		0x08
#define LDO_TEST_PIN_CTRL_EN2		0x04
#define LDO_TEST_PIN_CTRL_EN1		0x02
#define LDO_TEST_PIN_CTRL_EN0		0x01

/* TEST register bank 6 */
#define LDO_TEST_PIN_CTRL_LPM_MASK	0x0F


/*
 * If a given voltage could be output by two ranges, then the preferred one must
 * be determined by the range limits.  Specified voltage ranges should must
 * not overlap.
 *
 * Allowable voltage ranges:
 */
#define PLDO_LOW_UV_MIN			750000
#define PLDO_LOW_UV_MAX			1487500
#define PLDO_LOW_UV_FINE_STEP		12500

#define PLDO_NORM_UV_MIN		1500000
#define PLDO_NORM_UV_MAX		3075000
#define PLDO_NORM_UV_FINE_STEP		25000

#define PLDO_HIGH_UV_MIN		1750000
#define PLDO_HIGH_UV_SET_POINT_MIN	3100000
#define PLDO_HIGH_UV_MAX		4900000
#define PLDO_HIGH_UV_FINE_STEP		50000

#define PLDO_LOW_SET_POINTS		((PLDO_LOW_UV_MAX - PLDO_LOW_UV_MIN) \
						/ PLDO_LOW_UV_FINE_STEP + 1)
#define PLDO_NORM_SET_POINTS		((PLDO_NORM_UV_MAX - PLDO_NORM_UV_MIN) \
						/ PLDO_NORM_UV_FINE_STEP + 1)
#define PLDO_HIGH_SET_POINTS		((PLDO_HIGH_UV_MAX \
						- PLDO_HIGH_UV_SET_POINT_MIN) \
					/ PLDO_HIGH_UV_FINE_STEP + 1)
#define PLDO_SET_POINTS			(PLDO_LOW_SET_POINTS \
						+ PLDO_NORM_SET_POINTS \
						+ PLDO_HIGH_SET_POINTS)

#define NLDO_UV_MIN			750000
#define NLDO_UV_MAX			1537500
#define NLDO_UV_FINE_STEP		12500

#define NLDO_SET_POINTS			((NLDO_UV_MAX - NLDO_UV_MIN) \
						/ NLDO_UV_FINE_STEP + 1)

/* NLDO1200 masks and values */

/* CTRL register */
#define NLDO1200_ENABLE_MASK		0x80
#define NLDO1200_DISABLE		0x00
#define NLDO1200_ENABLE			0x80

/* Legacy mode */
#define NLDO1200_LEGACY_PM_MASK		0x20
#define NLDO1200_LEGACY_PM_HPM		0x00
#define NLDO1200_LEGACY_PM_LPM		0x20

/* Advanced mode */
#define NLDO1200_CTRL_RANGE_MASK	0x40
#define NLDO1200_CTRL_RANGE_HIGH	0x00
#define NLDO1200_CTRL_RANGE_LOW		0x40
#define NLDO1200_CTRL_VPROG_MASK	0x3F

#define NLDO1200_LOW_UV_MIN		375000
#define NLDO1200_LOW_UV_MAX		743750
#define NLDO1200_LOW_UV_STEP		6250

#define NLDO1200_HIGH_UV_MIN		750000
#define NLDO1200_HIGH_UV_MAX		1537500
#define NLDO1200_HIGH_UV_STEP		12500

#define NLDO1200_LOW_SET_POINTS		((NLDO1200_LOW_UV_MAX \
						- NLDO1200_LOW_UV_MIN) \
					/ NLDO1200_LOW_UV_STEP + 1)
#define NLDO1200_HIGH_SET_POINTS	((NLDO1200_HIGH_UV_MAX \
						- NLDO1200_HIGH_UV_MIN) \
					/ NLDO1200_HIGH_UV_STEP + 1)
#define NLDO1200_SET_POINTS		(NLDO1200_LOW_SET_POINTS \
						+ NLDO1200_HIGH_SET_POINTS)

/* TEST register bank 0 */
#define NLDO1200_TEST_LPM_MASK		0x04
#define NLDO1200_TEST_LPM_SEL_CTRL	0x00
#define NLDO1200_TEST_LPM_SEL_TCXO	0x04

/* TEST register bank 1 */
#define NLDO1200_PULL_DOWN_ENABLE_MASK	0x02
#define NLDO1200_PULL_DOWN_ENABLE	0x02

/* TEST register bank 2 */
#define NLDO1200_ADVANCED_MODE_MASK	0x08
#define NLDO1200_ADVANCED_MODE		0x00
#define NLDO1200_LEGACY_MODE		0x08

/* Advanced mode power mode control */
#define NLDO1200_ADVANCED_PM_MASK	0x02
#define NLDO1200_ADVANCED_PM_HPM	0x00
#define NLDO1200_ADVANCED_PM_LPM	0x02

#define NLDO1200_IN_ADVANCED_MODE(vreg) \
	((vreg->test_reg[2] & NLDO1200_ADVANCED_MODE_MASK) \
	 == NLDO1200_ADVANCED_MODE)

/* SMPS masks and values */

/* CTRL register */

/* Legacy mode */
#define SMPS_LEGACY_ENABLE_MASK		0x80
#define SMPS_LEGACY_DISABLE		0x00
#define SMPS_LEGACY_ENABLE		0x80
#define SMPS_LEGACY_PULL_DOWN_ENABLE	0x40
#define SMPS_LEGACY_VREF_SEL_MASK	0x20
#define SMPS_LEGACY_VPROG_MASK		0x1F

/* Advanced mode */
#define SMPS_ADVANCED_BAND_MASK		0xC0
#define SMPS_ADVANCED_BAND_OFF		0x00
#define SMPS_ADVANCED_BAND_1		0x40
#define SMPS_ADVANCED_BAND_2		0x80
#define SMPS_ADVANCED_BAND_3		0xC0
#define SMPS_ADVANCED_VPROG_MASK	0x3F

/* Legacy mode voltage ranges */
#define SMPS_MODE3_UV_MIN		375000
#define SMPS_MODE3_UV_MAX		725000
#define SMPS_MODE3_UV_STEP		25000

#define SMPS_MODE2_UV_MIN		750000
#define SMPS_MODE2_UV_MAX		1475000
#define SMPS_MODE2_UV_STEP		25000

#define SMPS_MODE1_UV_MIN		1500000
#define SMPS_MODE1_UV_MAX		3050000
#define SMPS_MODE1_UV_STEP		50000

#define SMPS_MODE3_SET_POINTS		((SMPS_MODE3_UV_MAX \
						- SMPS_MODE3_UV_MIN) \
					/ SMPS_MODE3_UV_STEP + 1)
#define SMPS_MODE2_SET_POINTS		((SMPS_MODE2_UV_MAX \
						- SMPS_MODE2_UV_MIN) \
					/ SMPS_MODE2_UV_STEP + 1)
#define SMPS_MODE1_SET_POINTS		((SMPS_MODE1_UV_MAX \
						- SMPS_MODE1_UV_MIN) \
					/ SMPS_MODE1_UV_STEP + 1)
#define SMPS_LEGACY_SET_POINTS		(SMPS_MODE3_SET_POINTS \
						+ SMPS_MODE2_SET_POINTS \
						+ SMPS_MODE1_SET_POINTS)

/* Advanced mode voltage ranges */
#define SMPS_BAND1_UV_MIN		375000
#define SMPS_BAND1_UV_MAX		737500
#define SMPS_BAND1_UV_STEP		12500

#define SMPS_BAND2_UV_MIN		750000
#define SMPS_BAND2_UV_MAX		1487500
#define SMPS_BAND2_UV_STEP		12500

#define SMPS_BAND3_UV_MIN		1500000
#define SMPS_BAND3_UV_MAX		3075000
#define SMPS_BAND3_UV_STEP		25000

#define SMPS_BAND1_SET_POINTS		((SMPS_BAND1_UV_MAX \
						- SMPS_BAND1_UV_MIN) \
					/ SMPS_BAND1_UV_STEP + 1)
#define SMPS_BAND2_SET_POINTS		((SMPS_BAND2_UV_MAX \
						- SMPS_BAND2_UV_MIN) \
					/ SMPS_BAND2_UV_STEP + 1)
#define SMPS_BAND3_SET_POINTS		((SMPS_BAND3_UV_MAX \
						- SMPS_BAND3_UV_MIN) \
					/ SMPS_BAND3_UV_STEP + 1)
#define SMPS_ADVANCED_SET_POINTS	(SMPS_BAND1_SET_POINTS \
						+ SMPS_BAND2_SET_POINTS \
						+ SMPS_BAND3_SET_POINTS)

/* Test2 register bank 1 */
#define SMPS_LEGACY_VLOW_SEL_MASK	0x01

/* Test2 register bank 6 */
#define SMPS_ADVANCED_PULL_DOWN_ENABLE	0x08

/* Test2 register bank 7 */
#define SMPS_ADVANCED_MODE_MASK		0x02
#define SMPS_ADVANCED_MODE		0x02
#define SMPS_LEGACY_MODE		0x00

#define SMPS_IN_ADVANCED_MODE(vreg) \
	((vreg->test_reg[7] & SMPS_ADVANCED_MODE_MASK) == SMPS_ADVANCED_MODE)

/* BUCK_SLEEP_CNTRL register */
#define SMPS_PIN_CTRL_MASK		0xF0
#define SMPS_PIN_CTRL_EN3		0x80
#define SMPS_PIN_CTRL_EN2		0x40
#define SMPS_PIN_CTRL_EN1		0x20
#define SMPS_PIN_CTRL_EN0		0x10

#define SMPS_PIN_CTRL_LPM_MASK		0x0F
#define SMPS_PIN_CTRL_LPM_EN3		0x08
#define SMPS_PIN_CTRL_LPM_EN2		0x04
#define SMPS_PIN_CTRL_LPM_EN1		0x02
#define SMPS_PIN_CTRL_LPM_EN0		0x01

/* BUCK_CLOCK_CNTRL register */
#define SMPS_CLK_DIVIDE2		0x40

#define SMPS_CLK_CTRL_MASK		0x30
#define SMPS_CLK_CTRL_FOLLOW_TCXO	0x00
#define SMPS_CLK_CTRL_PWM		0x10
#define SMPS_CLK_CTRL_PFM		0x20

/* FTSMPS masks and values */

/* CTRL register */
#define FTSMPS_VCTRL_BAND_MASK		0xC0
#define FTSMPS_VCTRL_BAND_OFF		0x00
#define FTSMPS_VCTRL_BAND_1		0x40
#define FTSMPS_VCTRL_BAND_2		0x80
#define FTSMPS_VCTRL_BAND_3		0xC0
#define FTSMPS_VCTRL_VPROG_MASK		0x3F

#define FTSMPS_BAND1_UV_MIN		350000
#define FTSMPS_BAND1_UV_MAX		650000
/* 3 LSB's of program voltage must be 0 in band 1. */
/* Logical step size */
#define FTSMPS_BAND1_UV_LOG_STEP	50000
/* Physical step size */
#define FTSMPS_BAND1_UV_PHYS_STEP	6250

#define FTSMPS_BAND2_UV_MIN		700000
#define FTSMPS_BAND2_UV_MAX		1400000
#define FTSMPS_BAND2_UV_STEP		12500

#define FTSMPS_BAND3_UV_MIN		1400000
#define FTSMPS_BAND3_UV_SET_POINT_MIN	1500000
#define FTSMPS_BAND3_UV_MAX		3300000
#define FTSMPS_BAND3_UV_STEP		50000

#define FTSMPS_BAND1_SET_POINTS		((FTSMPS_BAND1_UV_MAX \
						- FTSMPS_BAND1_UV_MIN) \
					/ FTSMPS_BAND1_UV_LOG_STEP + 1)
#define FTSMPS_BAND2_SET_POINTS		((FTSMPS_BAND2_UV_MAX \
						- FTSMPS_BAND2_UV_MIN) \
					/ FTSMPS_BAND2_UV_STEP + 1)
#define FTSMPS_BAND3_SET_POINTS		((FTSMPS_BAND3_UV_MAX \
					  - FTSMPS_BAND3_UV_SET_POINT_MIN) \
					/ FTSMPS_BAND3_UV_STEP + 1)
#define FTSMPS_SET_POINTS		(FTSMPS_BAND1_SET_POINTS \
						+ FTSMPS_BAND2_SET_POINTS \
						+ FTSMPS_BAND3_SET_POINTS)

/* FTS_CNFG1 register bank 0 */
#define FTSMPS_CNFG1_PM_MASK		0x0C
#define FTSMPS_CNFG1_PM_PWM		0x00
#define FTSMPS_CNFG1_PM_PFM		0x08

/* PWR_CNFG register */
#define FTSMPS_PULL_DOWN_ENABLE_MASK	0x40
#define FTSMPS_PULL_DOWN_ENABLE		0x40

/* VS masks and values */

/* CTRL register */
#define VS_ENABLE_MASK			0x80
#define VS_DISABLE			0x00
#define VS_ENABLE			0x80
#define VS_PULL_DOWN_ENABLE_MASK	0x40
#define VS_PULL_DOWN_DISABLE		0x40
#define VS_PULL_DOWN_ENABLE		0x00

#define VS_PIN_CTRL_MASK		0x0F
#define VS_PIN_CTRL_EN0			0x08
#define VS_PIN_CTRL_EN1			0x04
#define VS_PIN_CTRL_EN2			0x02
#define VS_PIN_CTRL_EN3			0x01

/* VS300 masks and values */

/* CTRL register */
#define VS300_CTRL_ENABLE_MASK		0xC0
#define VS300_CTRL_DISABLE		0x00
#define VS300_CTRL_ENABLE		0x40

#define VS300_PULL_DOWN_ENABLE_MASK	0x20
#define VS300_PULL_DOWN_ENABLE		0x20

/* NCP masks and values */

/* CTRL register */
#define NCP_ENABLE_MASK			0x80
#define NCP_DISABLE			0x00
#define NCP_ENABLE			0x80
#define NCP_VPROG_MASK			0x1F

#define NCP_UV_MIN			1500000
#define NCP_UV_MAX			3050000
#define NCP_UV_STEP			50000

#define NCP_SET_POINTS			((NCP_UV_MAX - NCP_UV_MIN) \
						/ NCP_UV_STEP + 1)
#define IS_REAL_REGULATOR(id)		((id) >= 0 && \
					 (id) < PM8921_VREG_ID_L1_PC)

#define MODE_LDO_HPM 0
#define MODE_LDO_LPM 1
#define MODE_NLDO1200_HPM 2
#define MODE_NLDO1200_LPM 3
#define MODE_SMPS_FOLLOW_TCXO 4
#define MODE_SMPS_PWM 5
#define MODE_SMPS_PFM 6
#define MODE_SMPS_AUTO 7
#define MODE_FTSMPS_PWM 8
#define MODE_FTSMPS_1 9
#define MODE_FTSMPS_PFM 10
#define MODE_FTSMPS_3 11

struct pm8921_vreg {
	/* Configuration data */
	const char				*name;
	const int				hpm_min_load;
	const u16				ctrl_addr;
	const u16				test_addr;
	const u16				clk_ctrl_addr;
	const u16				sleep_ctrl_addr;
	const u16				pfm_ctrl_addr;
	const u16				pwr_cnfg_addr;
	const u8				type;
	/* State data */
	u8				test_reg[REGULATOR_TEST_BANKS_MAX];
	u8					ctrl_reg;
	u8					clk_ctrl_reg;
	u8					sleep_ctrl_reg;
	u8					pfm_ctrl_reg;
	u8					pwr_cnfg_reg;
};

#define vreg_err(vreg, fmt, ...) \
	pr_err("%s: " fmt, vreg->name, ##__VA_ARGS__)

#define PLDO(_id, _ctrl_addr, _test_addr, _hpm_min_load, _name) \
	[PM8921_VREG_ID_##_id] = { \
		.type		= REGULATOR_TYPE_PLDO, \
		.ctrl_addr	= _ctrl_addr, \
		.test_addr	= _test_addr, \
		.hpm_min_load	= PM8921_VREG_##_hpm_min_load##_HPM_MIN_LOAD, \
		.name = _name, \
	}

#define NLDO(_id, _ctrl_addr, _test_addr, _hpm_min_load, _name) \
	[PM8921_VREG_ID_##_id] = { \
		.type		= REGULATOR_TYPE_NLDO, \
		.ctrl_addr	= _ctrl_addr, \
		.test_addr	= _test_addr, \
		.hpm_min_load	= PM8921_VREG_##_hpm_min_load##_HPM_MIN_LOAD, \
		.name = _name, \
	}

#define NLDO1200(_id, _ctrl_addr, _test_addr, _hpm_min_load, _name) \
	[PM8921_VREG_ID_##_id] = { \
		.type		= REGULATOR_TYPE_NLDO1200, \
		.ctrl_addr	= _ctrl_addr, \
		.test_addr	= _test_addr, \
		.hpm_min_load	= PM8921_VREG_##_hpm_min_load##_HPM_MIN_LOAD, \
		.name = _name, \
	}

#define SMPS(_id, _ctrl_addr, _test_addr, _clk_ctrl_addr, _sleep_ctrl_addr, \
	     _hpm_min_load, _name) \
	[PM8921_VREG_ID_##_id] = { \
		.type		= REGULATOR_TYPE_SMPS, \
		.ctrl_addr	= _ctrl_addr, \
		.test_addr	= _test_addr, \
		.clk_ctrl_addr	= _clk_ctrl_addr, \
		.sleep_ctrl_addr = _sleep_ctrl_addr, \
		.hpm_min_load	= PM8921_VREG_##_hpm_min_load##_HPM_MIN_LOAD, \
		.name = _name, \
	}

#define FTSMPS(_id, _pwm_ctrl_addr, _fts_cnfg1_addr, _pfm_ctrl_addr, \
	       _pwr_cnfg_addr, _hpm_min_load, _name) \
	[PM8921_VREG_ID_##_id] = { \
		.type		= REGULATOR_TYPE_FTSMPS, \
		.ctrl_addr	= _pwm_ctrl_addr, \
		.test_addr	= _fts_cnfg1_addr, \
		.pfm_ctrl_addr = _pfm_ctrl_addr, \
		.pwr_cnfg_addr = _pwr_cnfg_addr, \
		.hpm_min_load	= PM8921_VREG_##_hpm_min_load##_HPM_MIN_LOAD, \
		.name = _name, \
	}

#define VS(_id, _ctrl_addr, _name) \
	[PM8921_VREG_ID_##_id] = { \
		.type		= REGULATOR_TYPE_VS, \
		.ctrl_addr	= _ctrl_addr, \
		.name = _name, \
	}

#define VS300(_id, _ctrl_addr, _name) \
	[PM8921_VREG_ID_##_id] = { \
		.type		= REGULATOR_TYPE_VS300, \
		.ctrl_addr	= _ctrl_addr, \
		.name = _name, \
	}

#define NCP(_id, _ctrl_addr, _name) \
	[PM8921_VREG_ID_##_id] = { \
		.type		= REGULATOR_TYPE_NCP, \
		.ctrl_addr	= _ctrl_addr, \
		.name = _name, \
	}

static struct pm8921_vreg pm8921_vreg[] = {
	/*  id   ctrl   test   hpm_min */
	NLDO(L1,  0x0AE, 0x0AF, LDO_150, "8921_l1"),
	NLDO(L2,  0x0B0, 0x0B1, LDO_150, "8921_l2"),
	PLDO(L3,  0x0B2, 0x0B3, LDO_150, "8921_l3"),
	PLDO(L4,  0x0B4, 0x0B5, LDO_50, "8921_l4"),
	PLDO(L5,  0x0B6, 0x0B7, LDO_300, "8921_l5"),
	PLDO(L6,  0x0B8, 0x0B9, LDO_600, "8921_l6"),
	PLDO(L7,  0x0BA, 0x0BB, LDO_150, "8921_l7"),
	PLDO(L8,  0x0BC, 0x0BD, LDO_300, "8921_l8"),
	PLDO(L9,  0x0BE, 0x0BF, LDO_300, "8921_l9"),
	PLDO(L10, 0x0C0, 0x0C1, LDO_600, "8921_l10"),
	PLDO(L11, 0x0C2, 0x0C3, LDO_150, "8921_l11"),
	NLDO(L12, 0x0C4, 0x0C5, LDO_150, "8921_l12"),
	PLDO(L14, 0x0C8, 0x0C9, LDO_50, "8921_l14"),
	PLDO(L15, 0x0CA, 0x0CB, LDO_150, "8921_l15"),
	PLDO(L16, 0x0CC, 0x0CD, LDO_300, "8921_l16"),
	PLDO(L17, 0x0CE, 0x0CF, LDO_150, "8921_l17"),
	NLDO(L18, 0x0D0, 0x0D1, LDO_150, "8921_l18"),
	PLDO(L21, 0x0D6, 0x0D7, LDO_150, "8921_l21"),
	PLDO(L22, 0x0D8, 0x0D9, LDO_150, "8921_l22"),
	PLDO(L23, 0x0DA, 0x0DB, LDO_150, "8921_l23"),

	/*       id   ctrl   test   hpm_min */
	NLDO1200(L24, 0x0DC, 0x0DD, LDO_1200, "8921_l24"),
	NLDO1200(L25, 0x0DE, 0x0DF, LDO_1200, "8921_l25"),
	NLDO1200(L26, 0x0E0, 0x0E1, LDO_1200, "8921_l26"),
	NLDO1200(L27, 0x0E2, 0x0E3, LDO_1200, "8921_l27"),
	NLDO1200(L28, 0x0E4, 0x0E5, LDO_1200, "8921_l28"),

	/*  id   ctrl   test   hpm_min */
	PLDO(L29, 0x0E6, 0x0E7, LDO_150, "8921_l29"),

	/*   id  ctrl   test2  clk    sleep  hpm_min */
	SMPS(S1, 0x1D0, 0x1D5, 0x009, 0x1D2, SMPS_1500, "8921_s1"),
	SMPS(S2, 0x1D8, 0x1DD, 0x00A, 0x1DA, SMPS_1500, "8921_s2"),
	SMPS(S3, 0x1E0, 0x1E5, 0x00B, 0x1E2, SMPS_1500, "8921_s3"),
	SMPS(S4, 0x1E8, 0x1ED, 0x011, 0x1EA, SMPS_1500, "8921_s4"),

	/*     id  ctrl fts_cnfg1 pfm  pwr_cnfg  hpm_min */
	FTSMPS(S5, 0x025, 0x02E, 0x026, 0x032, SMPS_2000, "8921_s5"),
	FTSMPS(S6, 0x036, 0x03F, 0x037, 0x043, SMPS_2000, "8921_s6"),

	/*   id  ctrl   test2  clk    sleep  hpm_min */
	SMPS(S7, 0x1F0, 0x1F5, 0x012, 0x1F2, SMPS_1500, "8921_s7"),
	SMPS(S8, 0x1F8, 0x1FD, 0x013, 0x1FA, SMPS_1500, "8921_s8"),

	/* id		ctrl */
	VS(LVS1,	0x060, "8921_lvs1"),
	VS300(LVS2,     0x062, "8921_lvs2"),
	VS(LVS3,	0x064, "8921_lvs3"),
	VS(LVS4,	0x066, "8921_lvs4"),
	VS(LVS5,	0x068, "8921_lvs5"),
	VS(LVS6,	0x06A, "8921_lvs6"),
	VS(LVS7,	0x06C, "8921_lvs7"),
	VS300(USB_OTG,  0x06E, "8921_usb_otg"),
	VS300(HDMI_MVS, 0x070, "8921_hdmi_mvs"),

	/*  id   ctrl */
	NCP(NCP, 0x090, "8921_ncp"),
};

static struct dentry *debugfs_base;
static struct pm8xxx_debug_device *gdebugdev;

/* Returns the physical enable state of the regulator. */
static int pm8921_vreg_is_enabled(struct pm8921_vreg *vreg)
{
	int rc = 0;

	/*
	 * All regulator types except advanced mode SMPS, FTSMPS, and VS300 have
	 * enable bit in bit 7 of the control register.
	 */
	switch (vreg->type) {
	case REGULATOR_TYPE_FTSMPS:
		if ((vreg->ctrl_reg & FTSMPS_VCTRL_BAND_MASK)
		    != FTSMPS_VCTRL_BAND_OFF)
			rc = 1;
		break;
	case REGULATOR_TYPE_VS300:
		if ((vreg->ctrl_reg & VS300_CTRL_ENABLE_MASK)
		    != VS300_CTRL_DISABLE)
			rc = 1;
		break;
	case REGULATOR_TYPE_SMPS:
		if (SMPS_IN_ADVANCED_MODE(vreg)) {
			if ((vreg->ctrl_reg & SMPS_ADVANCED_BAND_MASK)
			    != SMPS_ADVANCED_BAND_OFF)
				rc = 1;
			break;
		}
		/* Fall through for legacy mode SMPS. */
	default:
		if ((vreg->ctrl_reg & REGULATOR_ENABLE_MASK)
		    == REGULATOR_ENABLE)
			rc = 1;
	}

	return rc;
}
/* Returns the physical enable state of the regulator. */
static int pm8921_vreg_is_pulldown(struct pm8921_vreg *vreg)
{
	int rc = 0;

	switch (vreg->type) {
	case REGULATOR_TYPE_PLDO:
	case REGULATOR_TYPE_NLDO:
		return (vreg->ctrl_reg & LDO_PULL_DOWN_ENABLE_MASK)?1:0;

	case REGULATOR_TYPE_NLDO1200:
		return (vreg->test_reg[1] & NLDO1200_PULL_DOWN_ENABLE_MASK)?1:0;

	case REGULATOR_TYPE_SMPS:
		if (!SMPS_IN_ADVANCED_MODE(vreg))
			return  (vreg->ctrl_reg & SMPS_LEGACY_PULL_DOWN_ENABLE)?1:0;
		else
			return (vreg->test_reg[6] & SMPS_ADVANCED_PULL_DOWN_ENABLE)?1:0;

	case REGULATOR_TYPE_FTSMPS:
		return (vreg->pwr_cnfg_reg & FTSMPS_PULL_DOWN_ENABLE_MASK)?1:0;

	case REGULATOR_TYPE_VS:
		return (vreg->ctrl_reg & VS_PULL_DOWN_DISABLE)?0:1;

	case REGULATOR_TYPE_VS300:
		return  (vreg->ctrl_reg & VS300_PULL_DOWN_ENABLE)?1:0;

	default:
		return rc = -EINVAL;
	}

	return rc;
}
static int _pm8921_nldo_get_voltage(struct pm8921_vreg *vreg)
{
	u8 vprog, fine_step_reg;

	fine_step_reg = vreg->test_reg[2] & LDO_TEST_FINE_STEP_MASK;
	vprog = vreg->ctrl_reg & LDO_CTRL_VPROG_MASK;

	vprog = (vprog << 1) | (fine_step_reg >> LDO_TEST_FINE_STEP_SHIFT);

	return NLDO_UV_FINE_STEP * vprog + NLDO_UV_MIN;
}

static int _pm8921_pldo_get_voltage(struct pm8921_vreg *vreg)
{
	int vmin, fine_step;
	u8 range_ext, range_sel, vprog, fine_step_reg;

	fine_step_reg = vreg->test_reg[2] & LDO_TEST_FINE_STEP_MASK;
	range_sel = vreg->test_reg[2] & LDO_TEST_RANGE_SEL_MASK;
	range_ext = vreg->test_reg[4] & LDO_TEST_RANGE_EXT_MASK;
	vprog = vreg->ctrl_reg & LDO_CTRL_VPROG_MASK;

	vprog = (vprog << 1) | (fine_step_reg >> LDO_TEST_FINE_STEP_SHIFT);

	if (range_sel) {
		/* low range mode */
		fine_step = PLDO_LOW_UV_FINE_STEP;
		vmin = PLDO_LOW_UV_MIN;
	} else if (!range_ext) {
		/* normal mode */
		fine_step = PLDO_NORM_UV_FINE_STEP;
		vmin = PLDO_NORM_UV_MIN;
	} else {
		/* high range mode */
		fine_step = PLDO_HIGH_UV_FINE_STEP;
		vmin = PLDO_HIGH_UV_MIN;
	}

	return fine_step * vprog + vmin;
}

static int _pm8921_nldo1200_get_voltage(struct pm8921_vreg *vreg)
{
	int uV = 0;
	int vprog;

	if (!NLDO1200_IN_ADVANCED_MODE(vreg)) {
		pr_warn("%s: currently in legacy mode; voltage unknown.\n",
			vreg->name);
		return  -EINVAL;
	}

	vprog = vreg->ctrl_reg & NLDO1200_CTRL_VPROG_MASK;

	if ((vreg->ctrl_reg & NLDO1200_CTRL_RANGE_MASK)
	    == NLDO1200_CTRL_RANGE_LOW)
		uV = vprog * NLDO1200_LOW_UV_STEP + NLDO1200_LOW_UV_MIN;
	else
		uV = vprog * NLDO1200_HIGH_UV_STEP + NLDO1200_HIGH_UV_MIN;

	return uV;
}
static int pm8921_smps_get_voltage_advanced(struct pm8921_vreg *vreg)
{
	u8 vprog, band;
	int uV = 0;

	vprog = vreg->ctrl_reg & SMPS_ADVANCED_VPROG_MASK;
	band = vreg->ctrl_reg & SMPS_ADVANCED_BAND_MASK;

	if (band == SMPS_ADVANCED_BAND_1)
		uV = vprog * SMPS_BAND1_UV_STEP + SMPS_BAND1_UV_MIN;
	else if (band == SMPS_ADVANCED_BAND_2)
		uV = vprog * SMPS_BAND2_UV_STEP + SMPS_BAND2_UV_MIN;
	else if (band == SMPS_ADVANCED_BAND_3)
		uV = vprog * SMPS_BAND3_UV_STEP + SMPS_BAND3_UV_MIN;
	else
		uV = - EINVAL;

	return uV;
}

static int pm8921_smps_get_voltage_legacy(struct pm8921_vreg *vreg)
{
	u8 vlow, vref, vprog;
	int uV;

	vlow = vreg->test_reg[1] & SMPS_LEGACY_VLOW_SEL_MASK;
	vref = vreg->ctrl_reg & SMPS_LEGACY_VREF_SEL_MASK;
	vprog = vreg->ctrl_reg & SMPS_LEGACY_VPROG_MASK;

	if (vlow && vref) {
		/* mode 3 */
		uV = vprog * SMPS_MODE3_UV_STEP + SMPS_MODE3_UV_MIN;
	} else if (vref) {
		/* mode 2 */
		uV = vprog * SMPS_MODE2_UV_STEP + SMPS_MODE2_UV_MIN;
	} else {
		/* mode 1 */
		uV = vprog * SMPS_MODE1_UV_STEP + SMPS_MODE1_UV_MIN;
	}

	return uV;
}

static int _pm8921_smps_get_voltage(struct pm8921_vreg *vreg)
{
	if (SMPS_IN_ADVANCED_MODE(vreg))
		return pm8921_smps_get_voltage_advanced(vreg);

	return pm8921_smps_get_voltage_legacy(vreg);
}

static int _pm8921_ftsmps_get_voltage(struct pm8921_vreg *vreg)
{
	u8 vprog, band;
	int uV = 0;

	if ((vreg->test_reg[0] & FTSMPS_CNFG1_PM_MASK) == FTSMPS_CNFG1_PM_PFM) {
		vprog = vreg->pfm_ctrl_reg & FTSMPS_VCTRL_VPROG_MASK;
		band = vreg->pfm_ctrl_reg & FTSMPS_VCTRL_BAND_MASK;
		if (band == FTSMPS_VCTRL_BAND_OFF && vprog == 0) {
			/* PWM_VCTRL overrides PFM_VCTRL */
			vprog = vreg->ctrl_reg & FTSMPS_VCTRL_VPROG_MASK;
			band = vreg->ctrl_reg & FTSMPS_VCTRL_BAND_MASK;
		}
	} else {
		vprog = vreg->ctrl_reg & FTSMPS_VCTRL_VPROG_MASK;
		band = vreg->ctrl_reg & FTSMPS_VCTRL_BAND_MASK;
	}

	if (band == FTSMPS_VCTRL_BAND_1)
		uV = vprog * FTSMPS_BAND1_UV_PHYS_STEP + FTSMPS_BAND1_UV_MIN;
	else if (band == FTSMPS_VCTRL_BAND_2)
		uV = vprog * FTSMPS_BAND2_UV_STEP + FTSMPS_BAND2_UV_MIN;
	else if (band == FTSMPS_VCTRL_BAND_3)
		uV = vprog * FTSMPS_BAND3_UV_STEP + FTSMPS_BAND3_UV_MIN;
	else
		uV = -EINVAL;

	return uV;
}

static int pm8921_vreg_get_voltage(struct pm8921_vreg *vreg)
{
	int rc = 0;

	switch (vreg->type) {
	case REGULATOR_TYPE_PLDO:
		return _pm8921_pldo_get_voltage(vreg);

	case REGULATOR_TYPE_NLDO:
		return _pm8921_nldo_get_voltage(vreg);

	case REGULATOR_TYPE_NLDO1200:
		return _pm8921_nldo1200_get_voltage(vreg);

	case REGULATOR_TYPE_SMPS:
		return _pm8921_smps_get_voltage(vreg);

	case REGULATOR_TYPE_FTSMPS:
		 return _pm8921_ftsmps_get_voltage(vreg);

	default:
		return rc = -EINVAL;
	}

	return rc;
}

static int pm8921_vreg_get_mode(struct pm8921_vreg *vreg)
{
	int rc = 0;
	unsigned int mode = 0;

	switch (vreg->type) {
	case REGULATOR_TYPE_PLDO:
	case REGULATOR_TYPE_NLDO:
		return ((vreg->ctrl_reg & LDO_CTRL_PM_MASK)
					== LDO_CTRL_PM_LPM ?
				MODE_LDO_LPM: MODE_LDO_HPM);

	case REGULATOR_TYPE_NLDO1200:
		if (NLDO1200_IN_ADVANCED_MODE(vreg)) {
			/* Advanced mode */
			if ((vreg->test_reg[2] & NLDO1200_ADVANCED_PM_MASK)
			    == NLDO1200_ADVANCED_PM_LPM)
				mode = MODE_NLDO1200_LPM;
			else
				mode = MODE_NLDO1200_HPM;
		} else {
			/* Legacy mode */
			if ((vreg->ctrl_reg & NLDO1200_LEGACY_PM_MASK)
			    == NLDO1200_LEGACY_PM_LPM)
				mode = MODE_NLDO1200_LPM;
			else
				mode = MODE_NLDO1200_HPM;
		}
		return mode;

	case REGULATOR_TYPE_SMPS:
		switch (vreg->clk_ctrl_reg & SMPS_CLK_CTRL_MASK) {
			case 0x0:
				mode = MODE_SMPS_FOLLOW_TCXO;
				break;
			case 0x10:
				mode = MODE_SMPS_PWM;
				break;
			case 0x20:
				mode = MODE_SMPS_PFM;
				break;
			case 0x30:
				mode = MODE_SMPS_AUTO;
				break;
		}
		return mode;
	case REGULATOR_TYPE_FTSMPS:
		switch (vreg->test_reg[0] & FTSMPS_CNFG1_PM_MASK) {
			case 0x0:
				mode = MODE_FTSMPS_PWM;
				break;
			case 0x40:
				mode = MODE_FTSMPS_1;
				break;
			case 0x80:
				mode = MODE_FTSMPS_PFM;
				break;
			case 0xC0:
				mode = MODE_FTSMPS_3;
				break;
		}
		return mode;

	default:
		return rc = -EINVAL;
	}

	return rc;
}

static int pm8921_init_ldo(struct pm8921_vreg *vreg, bool is_real)
{
	int rc = 0;
	int i;
	u8 bank;

	/* Save the current control register state. */
	rc = pm8xxx_readb(gdebugdev->parent, vreg->ctrl_addr, &vreg->ctrl_reg);
	if (rc)
		goto bail;

	/* Save the current test register state. */
	for (i = 0; i < LDO_TEST_BANKS; i++) {
		bank = REGULATOR_BANK_SEL(i);
		rc = pm8xxx_writeb(gdebugdev->parent, vreg->test_addr, bank);
		if (rc)
			goto bail;

		rc = pm8xxx_readb(gdebugdev->parent, vreg->test_addr,
				  &vreg->test_reg[i]);
		if (rc)
			goto bail;
		vreg->test_reg[i] |= REGULATOR_BANK_WRITE;
	}

bail:
	if (rc)
		vreg_err(vreg, "pm8xxx_readb/writeb failed, rc=%d\n", rc);

	return rc;
}

static int pm8921_init_nldo1200(struct pm8921_vreg *vreg)
{
	int rc = 0;
	int i;
	u8 bank;

	/* Save the current control register state. */
	rc = pm8xxx_readb(gdebugdev->parent, vreg->ctrl_addr, &vreg->ctrl_reg);
	if (rc)
		goto bail;

	/* Save the current test register state. */
	for (i = 0; i < LDO_TEST_BANKS; i++) {
		bank = REGULATOR_BANK_SEL(i);
		rc = pm8xxx_writeb(gdebugdev->parent, vreg->test_addr, bank);
		if (rc)
			goto bail;

		rc = pm8xxx_readb(gdebugdev->parent, vreg->test_addr,
				  &vreg->test_reg[i]);
		if (rc)
			goto bail;
		vreg->test_reg[i] |= REGULATOR_BANK_WRITE;
	}
bail:
	if (rc)
		vreg_err(vreg, "pm8xxx_readb/writeb failed, rc=%d\n", rc);

	return rc;
}

static int pm8921_init_smps(struct pm8921_vreg *vreg, bool is_real)
{
	int rc = 0;
	int i;
	u8 bank;

	/* Save the current control register state. */
	rc = pm8xxx_readb(gdebugdev->parent, vreg->ctrl_addr, &vreg->ctrl_reg);
	if (rc)
		goto bail;

	/* Save the current test2 register state. */
	for (i = 0; i < SMPS_TEST_BANKS; i++) {
		bank = REGULATOR_BANK_SEL(i);
		rc = pm8xxx_writeb(gdebugdev->parent, vreg->test_addr, bank);
		if (rc)
			goto bail;

		rc = pm8xxx_readb(gdebugdev->parent, vreg->test_addr,
				  &vreg->test_reg[i]);
		if (rc)
			goto bail;
		vreg->test_reg[i] |= REGULATOR_BANK_WRITE;
	}

	/* Save the current clock control register state. */
	rc = pm8xxx_readb(gdebugdev->parent, vreg->clk_ctrl_addr,
			  &vreg->clk_ctrl_reg);
	if (rc)
		goto bail;

	/* Save the current sleep control register state. */
	rc = pm8xxx_readb(gdebugdev->parent, vreg->sleep_ctrl_addr,
			  &vreg->sleep_ctrl_reg);
	if (rc)
		goto bail;

bail:
	if (rc)
		vreg_err(vreg, "pm8xxx_readb/writeb failed, rc=%d\n", rc);

	return rc;
}

static int pm8921_init_ftsmps(struct pm8921_vreg *vreg)
{
	int rc, i;
	u8 bank;

	/* Save the current control register state. */
	rc = pm8xxx_readb(gdebugdev->parent, vreg->ctrl_addr, &vreg->ctrl_reg);
	if (rc)
		goto bail;

	/* Store current regulator register values. */
	rc = pm8xxx_readb(gdebugdev->parent, vreg->pfm_ctrl_addr,
			  &vreg->pfm_ctrl_reg);
	if (rc)
		goto bail;

	rc = pm8xxx_readb(gdebugdev->parent, vreg->pwr_cnfg_addr,
			  &vreg->pwr_cnfg_reg);
	if (rc)
		goto bail;

	/* Save the current fts_cnfg1 register state (uses 'test' member). */
	for (i = 0; i < SMPS_TEST_BANKS; i++) {
		bank = REGULATOR_BANK_SEL(i);
		rc = pm8xxx_writeb(gdebugdev->parent, vreg->test_addr, bank);
		if (rc)
			goto bail;

		rc = pm8xxx_readb(gdebugdev->parent, vreg->test_addr,
				  &vreg->test_reg[i]);
		if (rc)
			goto bail;
		vreg->test_reg[i] |= REGULATOR_BANK_WRITE;
	}

bail:
	if (rc)
		vreg_err(vreg, "pm8xxx_readb/writeb failed, rc=%d\n", rc);

	return rc;
}

static int pm8921_init_vs(struct pm8921_vreg *vreg, bool is_real)
{
	int rc = 0;

	/* Save the current control register state. */
	rc = pm8xxx_readb(gdebugdev->parent, vreg->ctrl_addr, &vreg->ctrl_reg);
	if (rc) {
		vreg_err(vreg, "pm8xxx_readb failed, rc=%d\n", rc);
		return rc;
	}

	return rc;
}

static int pm8921_init_vs300(struct pm8921_vreg *vreg)
{
	int rc;

	/* Save the current control register state. */
	rc = pm8xxx_readb(gdebugdev->parent, vreg->ctrl_addr, &vreg->ctrl_reg);
	if (rc) {
		vreg_err(vreg, "pm8xxx_readb failed, rc=%d\n", rc);
		return rc;
	}

	return rc;
}

static int pm8921_init_ncp(struct pm8921_vreg *vreg)
{
	int rc;

	/* Save the current control register state. */
	rc = pm8xxx_readb(gdebugdev->parent, vreg->ctrl_addr, &vreg->ctrl_reg);
	if (rc) {
		vreg_err(vreg, "pm8xxx_readb failed, rc=%d\n", rc);
		return rc;
	}

	return rc;
}
static int pm8921_init_vreg(struct pm8921_vreg *vreg)
{
	int rc = 0;

	switch (vreg->type) {
	case REGULATOR_TYPE_PLDO:
	case REGULATOR_TYPE_NLDO:
		rc = pm8921_init_ldo(vreg, true);
		break;
	case REGULATOR_TYPE_NLDO1200:
		rc = pm8921_init_nldo1200(vreg);
		break;
	case REGULATOR_TYPE_SMPS:
		rc = pm8921_init_smps(vreg, true);
		break;
	case REGULATOR_TYPE_FTSMPS:
		rc = pm8921_init_ftsmps(vreg);
		break;
	case REGULATOR_TYPE_VS:
		rc = pm8921_init_vs(vreg, true);
		break;
	case REGULATOR_TYPE_VS300:
		rc = pm8921_init_vs300(vreg);
		break;
	case REGULATOR_TYPE_NCP:
		rc = pm8921_init_ncp(vreg);
		break;
	}
	return rc;
}
int pm8921_vreg_dump(int id, struct seq_file *m, char *vreg_buffer, int curr_len)
{
	int rc = 0;
	int len = 0;
	int mode = 0;
	int enable = 0;
	int voltage = 0;
	int pd = 0;
	char nam_buf[20];
	char en_buf[10];
	char mod_buf[10];
	char vol_buf[20];
	char pd_buf[10];
	char vreg_buf[128];

	struct pm8921_vreg *vreg;

	if (!IS_REAL_REGULATOR(id) || id == PM8921_VREG_ID_L5)
		return curr_len;

	memset(nam_buf,  ' ', sizeof(nam_buf));
	nam_buf[19] = 0;
	memset(en_buf, 0, sizeof(en_buf));
	memset(mod_buf, 0, sizeof(mod_buf));
	memset(vol_buf, 0, sizeof(vol_buf));
	memset(pd_buf, 0, sizeof(pd_buf));
	memset(vreg_buf, 0, sizeof(vreg_buf));

	vreg = &pm8921_vreg[id];

	rc = pm8921_init_vreg(vreg);
	if (rc)
		return curr_len;

	len = strlen(vreg->name);
	if (len > 19)
		len = 19;
	memcpy(nam_buf, vreg->name, len);

	enable =  pm8921_vreg_is_enabled(vreg);
	if (enable == -EINVAL)
		sprintf(en_buf, "NULL");
	else if (enable)
		sprintf(en_buf, "YES ");
	else
		sprintf(en_buf, "NO  ");

	mode = pm8921_vreg_get_mode(vreg);
	switch (mode) {
		case MODE_LDO_HPM:
		case MODE_NLDO1200_HPM:
			sprintf(mod_buf, "HPM ");
			break;
		case MODE_NLDO1200_LPM:
		case MODE_LDO_LPM:
			sprintf(mod_buf, "LPM ");
			break;
		case MODE_SMPS_PWM:
		case MODE_FTSMPS_PWM:
			sprintf(mod_buf, "PWM ");
			break;
		case MODE_SMPS_PFM:
		case MODE_FTSMPS_PFM:
			sprintf(mod_buf, "PFM ");
			break;
		case MODE_SMPS_FOLLOW_TCXO:
			sprintf(mod_buf, "TCXO");
			break;
		case MODE_SMPS_AUTO:
			sprintf(mod_buf, "AUTO");
			break;
		case MODE_FTSMPS_1:
			sprintf(mod_buf, "0x40");
			break;
		case MODE_FTSMPS_3:
			sprintf(mod_buf, "0x80");
			break;
		default:
			sprintf(mod_buf, "NULL");
	}

	voltage = pm8921_vreg_get_voltage(vreg);
	if (voltage == -EINVAL)
		sprintf(vol_buf, "NULL");
	else
		sprintf(vol_buf, "%d uV", voltage);

	pd = pm8921_vreg_is_pulldown(vreg);
	if (pd == -EINVAL)
		sprintf(pd_buf, "NULL");
	else if (pd)
		sprintf(pd_buf, "YES ");
	else
		sprintf(pd_buf, "NO  ");

	if (m)
		seq_printf(m, "VREG %s: [Enable]%s, [Mode]%s, [PD]%s, [Vol]%s\n", nam_buf, en_buf,  mod_buf, pd_buf, vol_buf);
	else
		pr_info("VREG %s: [Enable]%s, [Mode]%s, [PD]%s, [Vol]%s\n", nam_buf, en_buf,  mod_buf, pd_buf, vol_buf);

	if (vreg_buffer)
	{
		sprintf(vreg_buf, "VREG %s: [Enable]%s, [Mode]%s, [PD]%s, [Vol]%s\n", nam_buf, en_buf,  mod_buf, pd_buf, vol_buf);
		vreg_buf[127] = '\0';
		curr_len += sprintf(vreg_buffer + curr_len, vreg_buf);
	}
	return curr_len;
}

int pmic_vreg_dump(char *vreg_buffer, int curr_len)
{
	int i;
	char *title_msg = "------------ PMIC VREG -------------\n";

	if (vreg_buffer)
		curr_len += sprintf(vreg_buffer + curr_len,
			"%s\n", title_msg);

	pr_info("%s", title_msg);

	for (i = 0; i < PM8921_VREG_ID_MAX; i++)
		curr_len = pm8921_vreg_dump(i, NULL, vreg_buffer, curr_len);

	return curr_len;
}
static int list_vregs_show(struct seq_file *m, void *unused)
{
	int i;
	char *title_msg = "------------ PMIC VREG -------------\n";

	if (m)
		seq_printf(m, title_msg);

	for (i = 0; i < PM8921_VREG_ID_MAX; i++)
		pm8921_vreg_dump(i, m, NULL, 0);

	return 0;
}

extern int print_vreg_buffer(struct seq_file *m);
extern int free_vreg_buffer(void);

static int list_sleep_vregs_show(struct seq_file *m, void *unused)
{
	print_vreg_buffer(m);
	return 0;
}

static int list_vregs_open(struct inode *inode, struct file *file)
{
	return single_open(file, list_vregs_show, inode->i_private);
}

static int list_sleep_vregs_open(struct inode *inode, struct file *file)
{
	return single_open(file, list_sleep_vregs_show, inode->i_private);
}

static int list_sleep_vregs_release(struct inode *inode, struct file *file)
{
	free_vreg_buffer();
	return single_release(inode, file);
}

static const struct file_operations list_vregs_fops = {
	.open		= list_vregs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static const struct file_operations list_sleep_vregs_fops = {
	.open		= list_sleep_vregs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= list_sleep_vregs_release,
};
int __init pm8921_vreg_status_init(struct pm8xxx_debug_device *dev)
{
	int err = 0;

	debugfs_base = debugfs_create_dir("pm8921-vreg", NULL);
	if (!debugfs_base)
		return -ENOMEM;

	if (!debugfs_create_file("list_vregs", S_IRUGO, debugfs_base,
				&pm8921_vreg, &list_vregs_fops))
		return -ENOMEM;

	if (!debugfs_create_file("list_sleep_vregs", S_IRUGO, debugfs_base,
				&pm8921_vreg, &list_sleep_vregs_fops))
		return -ENOMEM;

	gdebugdev = dev;
	return err;
}
#else
int pmic_vreg_dump(char *vreg_buffer, int curr_len)
{
	return 0;
}
#endif
static int __devinit pm8xxx_debug_probe(struct platform_device *pdev)
{
	char *name = pdev->dev.platform_data;
	struct pm8xxx_debug_device *debugdev;
	struct dentry *dir;
	struct dentry *temp;
	int rc;

	if (name == NULL) {
		pr_err("debugfs directory name must be specified in "
			"platform_data pointer\n");
		return -EINVAL;
	}

	debugdev = kzalloc(sizeof(struct pm8xxx_debug_device), GFP_KERNEL);
	if (debugdev == NULL) {
		pr_err("kzalloc failed\n");
		return -ENOMEM;
	}

	debugdev->parent = pdev->dev.parent;
	debugdev->addr = -1;

	dir = debugfs_create_dir(name, NULL);
	if (dir == NULL || IS_ERR(dir)) {
		pr_err("debugfs_create_dir failed: rc=%ld\n", PTR_ERR(dir));
		rc = PTR_ERR(dir);
		goto dir_error;
	}

	temp = debugfs_create_file("addr", S_IRUSR | S_IWUSR, dir, debugdev,
				   &debug_addr_fops);
	if (temp == NULL || IS_ERR(temp)) {
		pr_err("debugfs_create_file failed: rc=%ld\n", PTR_ERR(temp));
		rc = PTR_ERR(temp);
		goto file_error;
	}

	temp = debugfs_create_file("data", S_IRUSR | S_IWUSR, dir, debugdev,
				   &debug_data_fops);
	if (temp == NULL || IS_ERR(temp)) {
		pr_err("debugfs_create_file failed: rc=%ld\n", PTR_ERR(temp));
		rc = PTR_ERR(temp);
		goto file_error;
	}
#ifdef CONFIG_ARCH_MSM8960
	pm8921_vreg_status_init(debugdev);
#endif
	mutex_init(&debugdev->debug_mutex);

	debugdev->dir = dir;
	platform_set_drvdata(pdev, debugdev);

	return 0;

file_error:
	debugfs_remove_recursive(dir);
dir_error:
	kfree(debugdev);

	return rc;
}

static int __devexit pm8xxx_debug_remove(struct platform_device *pdev)
{
	struct pm8xxx_debug_device *debugdev = platform_get_drvdata(pdev);

	if (debugdev) {
		debugfs_remove_recursive(debugdev->dir);
		mutex_destroy(&debugdev->debug_mutex);
		kfree(debugdev);
	}

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver pm8xxx_debug_driver = {
	.probe		= pm8xxx_debug_probe,
	.remove		= __devexit_p(pm8xxx_debug_remove),
	.driver		= {
		.name	= PM8XXX_DEBUG_DEV_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init pm8xxx_debug_init(void)
{
	return platform_driver_register(&pm8xxx_debug_driver);
}
subsys_initcall(pm8xxx_debug_init);

static void __exit pm8xxx_debug_exit(void)
{
	platform_driver_unregister(&pm8xxx_debug_driver);
}
module_exit(pm8xxx_debug_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("PM8XXX Debug driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:" PM8XXX_DEBUG_DEV_NAME);
