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
 *
 */
#define pr_fmt(fmt)	"[BATT][BMS] " fmt
#define pr_fmt_debug(fmt)    "[BATT][BMS]%s: " fmt, __func__

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/mfd/pm8xxx/pm8921-bms.h>
#include <linux/mfd/pm8xxx/pm8921-charger.h>
#include <linux/mfd/pm8xxx/core.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <linux/mfd/pm8xxx/ccadc.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <mach/board_htc.h>
#include <mach/htc_restart_handler.h>
#include <asm/uaccess.h>

#ifdef CONFIG_HTC_BATT_8960
#include "mach/htc_battery_cell.h"
#endif

#if defined(pr_debug)
#undef pr_debug
#endif
#define pr_debug(fmt, ...) do { \
		if (flag_enable_bms_chg_log) \
			printk(KERN_INFO pr_fmt_debug(fmt), ##__VA_ARGS__); \
	} while (0)
static bool flag_enable_bms_chg_log;
#define BATT_LOG_BUF_LEN (512)

#define BMS_CONTROL		0x224
#define BMS_S1_DELAY	0x225
#define BMS_OUTPUT0		0x230
#define BMS_OUTPUT1		0x231
#define BMS_TOLERANCES		0x232
#define BMS_TEST1		0x237

#define ADC_ARB_SECP_CNTRL	0x190
#define ADC_ARB_SECP_AMUX_CNTRL	0x191
#define ADC_ARB_SECP_ANA_PARAM	0x192
#define ADC_ARB_SECP_DIG_PARAM	0x193
#define ADC_ARB_SECP_RSV	0x194
#define ADC_ARB_SECP_DATA1	0x195
#define ADC_ARB_SECP_DATA0	0x196

#define ADC_ARB_BMS_CNTRL	0x18D
#define AMUX_TRIM_2			0x322
#define TEST_PROGRAM_REV	0x339


#define OCV_UPDATE_STORAGE	0x105
#define OCV_UPDATE_STORAGE_USE_MASK		(1)
#define OCV_HW_RESET_MASK			(1<<1)

#define BMS_STORE_MAGIC_NUM		0xDDAACC00
#define BMS_STORE_MAGIC_OFFSET		1056
#define BMS_STORE_SOC_OFFSET		1060
#define BMS_STORE_OCV_OFFSET		1064
#define BMS_STORE_CC_OFFSET		1068
#define BMS_STORE_CURRTIME_OFFSET		1072

#define BATT_MAX_OCV_UV		5000000
#define BATT_MIN_OCV_UV		0

enum pmic_bms_interrupts {
	PM8921_BMS_SBI_WRITE_OK,
	PM8921_BMS_CC_THR,
	PM8921_BMS_VSENSE_THR,
	PM8921_BMS_VSENSE_FOR_R,
	PM8921_BMS_OCV_FOR_R,
	PM8921_BMS_GOOD_OCV,
	PM8921_BMS_VSENSE_AVG,
	PM_BMS_MAX_INTS,
};

struct pm8921_soc_params {
	uint16_t	last_good_ocv_raw;
	int		cc;

	int		last_good_ocv_uv;
};

struct pm8921_rbatt_params {
	uint16_t	ocv_for_rbatt_raw;
	uint16_t	vsense_for_rbatt_raw;
	uint16_t	vbatt_for_rbatt_raw;

	int		ocv_for_rbatt_uv;
	int		vsense_for_rbatt_uv;
	int		vbatt_for_rbatt_uv;
};

struct pm8921_bms_chip {
	struct device		*dev;
	struct dentry		*dent;
	unsigned int		r_sense;
	unsigned int		i_test;
	unsigned int		v_failure;
	unsigned int		fcc;
	struct single_row_lut	*fcc_temp_lut;
	struct single_row_lut	*fcc_sf_lut;
	struct pc_temp_ocv_lut	*pc_temp_ocv_lut;
	struct sf_lut		*pc_sf_lut;
	struct sf_lut		*rbatt_sf_lut;
	struct sf_lut		*rbatt_est_ocv_lut;
	int			delta_rbatt_mohm;
	struct work_struct	calib_hkadc_work;
	unsigned int		revision;
	unsigned int		version;
	unsigned int		xoadc_v0625_usb_present;
	unsigned int		xoadc_v0625_usb_absent;
	unsigned int		xoadc_v0625;
	unsigned int		xoadc_v125;
	unsigned int		batt_temp_channel;
	unsigned int		vbat_channel;
	unsigned int		ref625mv_channel;
	unsigned int		ref1p25v_channel;
	unsigned int		batt_id_channel;
	unsigned int		pmic_bms_irq[PM_BMS_MAX_INTS];
	DECLARE_BITMAP(enabled_irqs, PM_BMS_MAX_INTS);
	struct mutex		bms_output_lock;
	spinlock_t		bms_100_lock;
	struct single_row_lut	*adjusted_fcc_temp_lut;
	unsigned int		charging_began;
	int					start_percent;
	int					end_percent;

	uint16_t		ocv_reading_at_100;
	int			cc_backup_uv;
	int			ocv_backup_uv;
	int			max_voltage_uv;
	int			batt_temp_suspend;
	int			soc_rbatt_suspend;
	int			default_rbatt_mohm;
	unsigned int	rconn_mohm;
	int			store_batt_data_soc_thre;
	int			amux_2_trim_delta;
	uint16_t		prev_last_good_ocv_raw;
	int			usb_chg_plugged_ready;
	int					level_ocv_update_stop_begin;
	int					level_ocv_update_stop_end;
	unsigned int	criteria_sw_est_ocv;
	unsigned int 	rconn_mohm_sw_est_ocv;
};

static struct pm8921_bms_chip *the_chip;

struct pm8921_bms_debug {
	int rbatt;
	int rbatt_sf;
	int voltage_unusable_uv;
	int pc_unusable;
	int rc_pc;
	int scalefactor;
	int batt_temp;
	int soc_rbatt;
	int last_ocv_raw_uv;
};
static struct pm8921_bms_debug bms_dbg;

struct htc_bms_timer {
	unsigned long batt_system_jiffies;
	unsigned long batt_suspend_ms;
	unsigned long no_ocv_update_period_ms;
};
static struct htc_bms_timer htc_batt_bms_timer;

#define DEFAULT_RBATT_MOHMS		128
#define DEFAULT_OCV_MICROVOLTS		3900000
#define DEFAULT_CHARGE_CYCLES		0

static int last_usb_cal_delta_uv = 1800;
module_param(last_usb_cal_delta_uv, int, 0644);

static int last_chargecycles = DEFAULT_CHARGE_CYCLES;
static int last_charge_increase;
module_param(last_chargecycles, int, 0644);
module_param(last_charge_increase, int, 0644);

static int last_rbatt = -EINVAL;
static int last_ocv_uv = -EINVAL;
static int last_soc = -EINVAL;
static int last_real_fcc_mah = -EINVAL;
static int last_real_fcc_batt_temp = -EINVAL;
static char batt_log_buf[BATT_LOG_BUF_LEN];

static int bms_ops_set(const char *val, const struct kernel_param *kp)
{
	if (*(int *)kp->arg == -EINVAL)
		return param_set_int(val, kp);
	else
		return 0;
}

static struct kernel_param_ops bms_param_ops = {
	.set = bms_ops_set,
	.get = param_get_int,
};

module_param_cb(last_rbatt, &bms_param_ops, &last_rbatt, 0644);
module_param_cb(last_ocv_uv, &bms_param_ops, &last_ocv_uv, 0644);
module_param_cb(last_soc, &bms_param_ops, &last_soc, 0644);

static int bms_fake_battery = -EINVAL;
module_param(bms_fake_battery, int, 0644);

static int bms_start_percent;
static int bms_start_ocv_uv;
static int bms_start_cc_uah;
static int bms_end_percent;
static int bms_end_ocv_uv;
static int bms_end_cc_uah;

static int ocv_update_stop_active_mask = OCV_UPDATE_STOP_BIT_CABLE_IN |
											OCV_UPDATE_STOP_BIT_ATTR_FILE |
											OCV_UPDATE_STOP_BIT_BOOT_UP;
static int ocv_update_stop_reason;
static int level_dropped_after_cable_out = 5;
static int level_dropped_after_boot_up = 5;
static int new_boot_soc;
static int bms_discharge_percent;
static int is_ocv_update_start;
struct mutex ocv_update_lock;

static int bms_ro_ops_set(const char *val, const struct kernel_param *kp)
{
	return -EINVAL;
}

static struct kernel_param_ops bms_ro_param_ops = {
	.set = bms_ro_ops_set,
	.get = param_get_int,
};
module_param_cb(bms_start_percent, &bms_ro_param_ops, &bms_start_percent, 0644);
module_param_cb(bms_start_ocv_uv, &bms_ro_param_ops, &bms_start_ocv_uv, 0644);
module_param_cb(bms_start_cc_uah, &bms_ro_param_ops, &bms_start_cc_uah, 0644);

module_param_cb(bms_end_percent, &bms_ro_param_ops, &bms_end_percent, 0644);
module_param_cb(bms_end_ocv_uv, &bms_ro_param_ops, &bms_end_ocv_uv, 0644);
module_param_cb(bms_end_cc_uah, &bms_ro_param_ops, &bms_end_cc_uah, 0644);

static int dump_cc_uah(void);

static int interpolate_fcc(struct pm8921_bms_chip *chip, int batt_temp);
static void readjust_fcc_table(void)
{
	struct single_row_lut *temp, *old;
	int i, fcc, ratio;

	if (!the_chip->fcc_temp_lut) {
		pr_err("The static fcc lut table is NULL\n");
		return;
	}

	temp = kzalloc(sizeof(struct single_row_lut), GFP_KERNEL);
	if (!temp) {
		pr_err("Cannot allocate memory for adjusted fcc table\n");
		return;
	}

	fcc = interpolate_fcc(the_chip, last_real_fcc_batt_temp);

	temp->cols = the_chip->fcc_temp_lut->cols;
	for (i = 0; i < the_chip->fcc_temp_lut->cols; i++) {
		temp->x[i] = the_chip->fcc_temp_lut->x[i];
		ratio = div_u64(the_chip->fcc_temp_lut->y[i] * 1000, fcc);
		temp->y[i] =  (ratio * last_real_fcc_mah);
		temp->y[i] /= 1000;
		pr_debug("temp=%d, staticfcc=%d, adjfcc=%d, ratio=%d\n",
				temp->x[i], the_chip->fcc_temp_lut->y[i],
				temp->y[i], ratio);
	}

	old = the_chip->adjusted_fcc_temp_lut;
	the_chip->adjusted_fcc_temp_lut = temp;
	kfree(old);
}

static int bms_last_real_fcc_set(const char *val,
				const struct kernel_param *kp)
{
	int rc = 0;

	if (last_real_fcc_mah == -EINVAL)
		rc = param_set_int(val, kp);
	if (rc) {
		pr_err("Failed to set last_real_fcc_mah rc=%d\n", rc);
		return rc;
	}
	if (last_real_fcc_batt_temp != -EINVAL)
		readjust_fcc_table();
	return rc;
}
static struct kernel_param_ops bms_last_real_fcc_param_ops = {
	.set = bms_last_real_fcc_set,
	.get = param_get_int,
};
module_param_cb(last_real_fcc_mah, &bms_last_real_fcc_param_ops,
					&last_real_fcc_mah, 0644);

static int bms_last_real_fcc_batt_temp_set(const char *val,
				const struct kernel_param *kp)
{
	int rc = 0;

	if (last_real_fcc_batt_temp == -EINVAL)
		rc = param_set_int(val, kp);
	if (rc) {
		pr_err("Failed to set last_real_fcc_batt_temp rc=%d\n", rc);
		return rc;
	}
	if (last_real_fcc_mah != -EINVAL)
		readjust_fcc_table();
	return rc;
}

static struct kernel_param_ops bms_last_real_fcc_batt_temp_param_ops = {
	.set = bms_last_real_fcc_batt_temp_set,
	.get = param_get_int,
};
module_param_cb(last_real_fcc_batt_temp, &bms_last_real_fcc_batt_temp_param_ops,
					&last_real_fcc_batt_temp, 0644);

static ssize_t kernel_write(struct file *file, const char *buf,
	size_t count, loff_t pos)
{
	mm_segment_t old_fs;
	ssize_t res;

	old_fs = get_fs();
	set_fs(get_ds());
	
	res = vfs_write(file, (const char __user *)buf, count, &pos);
	set_fs(old_fs);

	return res;
}

int emmc_misc_write(int val, int offset)
{
	char filename[32] = "";
	int w_val = val;
	struct file *filp = NULL;
	ssize_t nread;
	int pnum = get_partition_num_by_name("misc");

	if (pnum < 0) {
		pr_info("%s: unknown partition number for misc partition\n", __func__);
		return 0;
	}
	sprintf(filename, "/dev/block/mmcblk0p%d", pnum);

	filp = filp_open(filename, O_RDWR, 0);
	if (IS_ERR(filp)) {
		pr_info("%s: unable to open file: %s\n", __func__, filename);
		return PTR_ERR(filp);
	}

	filp->f_pos = offset;
	nread = kernel_write(filp, (char *)&w_val, sizeof(int), filp->f_pos);
	pr_info("%s: %X (%d)\n", __func__, w_val, nread);

	if (filp)
		filp_close(filp, NULL);

	return 1;
}

static int pm_bms_get_rt_status(struct pm8921_bms_chip *chip, int irq_id)
{
	return pm8xxx_read_irq_stat(chip->dev->parent,
					chip->pmic_bms_irq[irq_id]);
}

static void pm8921_bms_enable_irq(struct pm8921_bms_chip *chip, int interrupt)
{
	if (!__test_and_set_bit(interrupt, chip->enabled_irqs)) {
		dev_dbg(chip->dev, "%s %d\n", __func__,
						chip->pmic_bms_irq[interrupt]);
		enable_irq(chip->pmic_bms_irq[interrupt]);
	}
}

static void pm8921_bms_disable_irq(struct pm8921_bms_chip *chip, int interrupt)
{
	if (__test_and_clear_bit(interrupt, chip->enabled_irqs)) {
		pr_debug("%d\n", chip->pmic_bms_irq[interrupt]);
		disable_irq_nosync(chip->pmic_bms_irq[interrupt]);
	}
}

static int pm_bms_masked_write(struct pm8921_bms_chip *chip, u16 addr,
							u8 mask, u8 val)
{
	int rc;
	u8 reg;

	rc = pm8xxx_readb(chip->dev->parent, addr, &reg);
	if (rc) {
		pr_err("read failed addr = %03X, rc = %d\n", addr, rc);
		return rc;
	}
	reg &= ~mask;
	reg |= val & mask;
	rc = pm8xxx_writeb(chip->dev->parent, addr, reg);
	if (rc) {
		pr_err("write failed addr = %03X, rc = %d\n", addr, rc);
		return rc;
	}
	return 0;
}

static int usb_chg_plugged_in(void)
{
#if 0
	union power_supply_propval ret = {0,};
	static struct power_supply *psy;
	if (psy == NULL) {
		psy = power_supply_get_by_name("usb");
		if (psy == NULL)
			return 0;
	}

	if (psy->get_property(psy, POWER_SUPPLY_PROP_ONLINE, &ret))
		return 0;
	return ret.intval;
#endif
	int rc = pm8921_is_usb_chg_plugged_in();
	if (rc < 0) {
		return 0;
	}
	the_chip->usb_chg_plugged_ready = 1;
	return rc;
}

#define HOLD_OREG_DATA		BIT(1)
static int pm_bms_lock_output_data(struct pm8921_bms_chip *chip)
{
	int rc;

	rc = pm_bms_masked_write(chip, BMS_CONTROL, HOLD_OREG_DATA,
					HOLD_OREG_DATA);
	if (rc) {
		pr_err("couldnt lock bms output rc = %d\n", rc);
		return rc;
	}
	return 0;
}

static int pm_bms_unlock_output_data(struct pm8921_bms_chip *chip)
{
	int rc;

	rc = pm_bms_masked_write(chip, BMS_CONTROL, HOLD_OREG_DATA, 0);
	if (rc) {
		pr_err("fail to unlock BMS_CONTROL rc = %d\n", rc);
		return rc;
	}
	return 0;
}

#define SELECT_OUTPUT_DATA	0x1C
#define SELECT_OUTPUT_TYPE_SHIFT	2
#define OCV_FOR_RBATT		0x0
#define VSENSE_FOR_RBATT	0x1
#define VBATT_FOR_RBATT		0x2
#define CC_MSB			0x3
#define CC_LSB			0x4
#define LAST_GOOD_OCV_VALUE	0x5
#define VSENSE_AVG		0x6
#define VBATT_AVG		0x7

static int pm_bms_read_output_data(struct pm8921_bms_chip *chip, int type,
						int16_t *result)
{
	int rc;
	u8 reg;

	if (!result) {
		pr_err("result pointer null\n");
		return -EINVAL;
	}
	*result = 0;
	if (type < OCV_FOR_RBATT || type > VBATT_AVG) {
		pr_err("invalid type %d asked to read\n", type);
		return -EINVAL;
	}

	rc = pm_bms_masked_write(chip, BMS_CONTROL, SELECT_OUTPUT_DATA,
					type << SELECT_OUTPUT_TYPE_SHIFT);
	if (rc) {
		pr_err("fail to select %d type in BMS_CONTROL rc = %d\n",
						type, rc);
		return rc;
	}

	rc = pm8xxx_readb(chip->dev->parent, BMS_OUTPUT0, &reg);
	if (rc) {
		pr_err("fail to read BMS_OUTPUT0 for type %d rc = %d\n",
			type, rc);
		return rc;
	}
	*result = reg;
	rc = pm8xxx_readb(chip->dev->parent, BMS_OUTPUT1, &reg);
	if (rc) {
		pr_err("fail to read BMS_OUTPUT1 for type %d rc = %d\n",
			type, rc);
		return rc;
	}
	*result |= reg << 8;
	pr_debug("type %d result %x", type, *result);
	return 0;
}

#define V_PER_BIT_MUL_FACTOR	97656
#define V_PER_BIT_DIV_FACTOR	1000
#define XOADC_INTRINSIC_OFFSET	0x6000
static int xoadc_reading_to_microvolt(unsigned int a)
{
	if (a <= XOADC_INTRINSIC_OFFSET)
		return 0;

	return (a - XOADC_INTRINSIC_OFFSET)
			* V_PER_BIT_MUL_FACTOR / V_PER_BIT_DIV_FACTOR;
}

#define XOADC_CALIB_UV		625000
#define VBATT_MUL_FACTOR	3
static int adjust_xo_vbatt_reading(struct pm8921_bms_chip *chip,
					int usb_chg, unsigned int uv)
{
	s64 numerator, denominator;
	int local_delta;

	if (uv == 0)
		return 0;

	
	if (chip->xoadc_v0625 == 0 || chip->xoadc_v125 == 0) {
		pr_debug("No cal yet return %d\n", VBATT_MUL_FACTOR * uv);
		return VBATT_MUL_FACTOR * uv;
	}

	if (usb_chg)
		local_delta = last_usb_cal_delta_uv;
	else
		local_delta = 0;

	pr_debug("using delta = %d\n", local_delta);
	numerator = ((s64)uv - chip->xoadc_v0625 - local_delta)
							* XOADC_CALIB_UV;
	denominator =  (s64)chip->xoadc_v125 - chip->xoadc_v0625 - local_delta;
	if (denominator == 0)
		return uv * VBATT_MUL_FACTOR;
	return (XOADC_CALIB_UV + local_delta + div_s64(numerator, denominator))
						* VBATT_MUL_FACTOR;
}

#define CC_RESOLUTION_N_V1	1085069
#define CC_RESOLUTION_D_V1	100000
#define CC_RESOLUTION_N_V2	868056
#define CC_RESOLUTION_D_V2	10000

static s64 cc_to_microvolt_v1(s64 cc)
{
	return div_s64(cc * CC_RESOLUTION_N_V1, CC_RESOLUTION_D_V1);
}

static s64 cc_to_microvolt_v2(s64 cc)
{
	return div_s64(cc * CC_RESOLUTION_N_V2, CC_RESOLUTION_D_V2);
}

static s64 cc_to_microvolt(struct pm8921_bms_chip *chip, s64 cc)
{
	return (chip->revision < PM8XXX_REVISION_8921_2p0
			&& chip->version == PM8XXX_VERSION_8921 ) ?
				cc_to_microvolt_v1((s64)cc) :
				cc_to_microvolt_v2((s64)cc);
}

#define CC_READING_TICKS	56
#define SLEEP_CLK_HZ		32764
#define SECONDS_PER_HOUR	3600
static s64 ccmicrovolt_to_nvh(s64 cc_uv)
{
	return div_s64(cc_uv * CC_READING_TICKS * 1000,
			SLEEP_CLK_HZ * SECONDS_PER_HOUR);
}

static int read_cc(struct pm8921_bms_chip *chip, int *result)
{
	int rc;
	uint16_t msw, lsw;

	rc = pm_bms_read_output_data(chip, CC_LSB, &lsw);
	if (rc) {
		pr_err("fail to read CC_LSB rc = %d\n", rc);
		return rc;
	}
	rc = pm_bms_read_output_data(chip, CC_MSB, &msw);
	if (rc) {
		pr_err("fail to read CC_MSB rc = %d\n", rc);
		return rc;
	}
	*result = msw << 16 | lsw;
	pr_debug("msw = %04x lsw = %04x cc = %d\n", msw, lsw, *result);
	return 0;
}

static int adjust_xo_vbatt_reading_for_mbg(struct pm8921_bms_chip *chip,
						int result)
{
	int64_t numerator;
	int64_t denominator;

	if (chip->amux_2_trim_delta == 0)
		return result;

	numerator = (s64)result * 1000000;
	denominator = (1000000 + (410 * (s64)chip->amux_2_trim_delta));
	return div_s64(numerator, denominator);
}

static int convert_vbatt_raw_to_uv(struct pm8921_bms_chip *chip,
					int usb_chg,
					uint16_t reading, int *result)
{
	*result = xoadc_reading_to_microvolt(reading);
	pr_debug("raw = %04x vbatt = %u\n", reading, *result);
	*result = adjust_xo_vbatt_reading(chip, usb_chg, *result);
	pr_debug("after adj vbatt = %u\n", *result);
	*result = adjust_xo_vbatt_reading_for_mbg(chip, *result);
	pr_debug("after mbg adj vbatt = %u\n", *result);
	return 0;
}

static int convert_vsense_to_uv(struct pm8921_bms_chip *chip,
					int16_t reading, int *result)
{
	*result = pm8xxx_ccadc_reading_to_microvolt(chip->revision, reading);
	pr_debug("raw = %04x vsense = %d\n", reading, *result);
	*result = pm8xxx_cc_adjust_for_gain(*result);
	pr_debug("after adj vsense = %d\n", *result);
	return 0;
}

static int read_vsense_avg(struct pm8921_bms_chip *chip, int *result)
{
	int rc;
	int16_t reading;

	rc = pm_bms_read_output_data(chip, VSENSE_AVG, &reading);
	if (rc) {
		pr_err("fail to read VSENSE_AVG rc = %d\n", rc);
		return rc;
	}

	convert_vsense_to_uv(chip, reading, result);
	return 0;
}

static int linear_interpolate(int y0, int x0, int y1, int x1, int x)
{
	if (y0 == y1 || x == x0)
		return y0;
	if (x1 == x0 || x == x1)
		return y1;

	return y0 + ((y1 - y0) * (x - x0) / (x1 - x0));
}

static int interpolate_single_lut(struct single_row_lut *lut, int x)
{
	int i, result;

	if (x < lut->x[0]) {
		pr_debug("x %d less than known range return y = %d lut = %pS\n",
							x, lut->y[0], lut);
		return lut->y[0];
	}
	if (x > lut->x[lut->cols - 1]) {
		pr_debug("x %d more than known range return y = %d lut = %pS\n",
						x, lut->y[lut->cols - 1], lut);
		return lut->y[lut->cols - 1];
	}

	for (i = 0; i < lut->cols; i++)
		if (x <= lut->x[i])
			break;
	if (x == lut->x[i]) {
		result = lut->y[i];
	} else {
		result = linear_interpolate(
			lut->y[i - 1],
			lut->x[i - 1],
			lut->y[i],
			lut->x[i],
			x);
	}
	return result;
}

static int interpolate_fcc(struct pm8921_bms_chip *chip, int batt_temp)
{
	
	batt_temp = batt_temp/10;
	return interpolate_single_lut(chip->fcc_temp_lut, batt_temp);
}

static int interpolate_fcc_adjusted(struct pm8921_bms_chip *chip, int batt_temp)
{
	
	batt_temp = batt_temp/10;
	return interpolate_single_lut(chip->adjusted_fcc_temp_lut, batt_temp);
}

static int interpolate_scalingfactor_fcc(struct pm8921_bms_chip *chip,
								int cycles)
{
	if (chip->fcc_sf_lut)
		return interpolate_single_lut(chip->fcc_sf_lut, cycles);
	else
		return 100;
}

static int interpolate_scalingfactor(struct pm8921_bms_chip *chip,
				struct sf_lut *sf_lut,
				int row_entry, int pc)
{
	int i, scalefactorrow1, scalefactorrow2, scalefactor;
	int rows, cols;
	int row1 = 0;
	int row2 = 0;

	if (!sf_lut)
		return 100;

	rows = sf_lut->rows;
	cols = sf_lut->cols;
	if (pc > sf_lut->percent[0]) {
		pr_debug("pc %d greater than known pc ranges for sfd\n", pc);
		row1 = 0;
		row2 = 0;
	}
	if (pc < sf_lut->percent[rows - 1]) {
		pr_debug("pc %d less than known pc ranges for sf", pc);
		row1 = rows - 1;
		row2 = rows - 1;
	}
	for (i = 0; i < rows; i++) {
		if (pc == sf_lut->percent[i]) {
			row1 = i;
			row2 = i;
			break;
		}
		if (pc > sf_lut->percent[i]) {
			row1 = i - 1;
			row2 = i;
			break;
		}
	}

	if (row_entry < sf_lut->row_entries[0])
		row_entry = sf_lut->row_entries[0];
	if (row_entry > sf_lut->row_entries[cols - 1])
		row_entry = sf_lut->row_entries[cols - 1];

	for (i = 0; i < cols; i++)
		if (row_entry <= sf_lut->row_entries[i])
			break;
	if (row_entry == sf_lut->row_entries[i]) {
		scalefactor = linear_interpolate(
				sf_lut->sf[row1][i],
				sf_lut->percent[row1],
				sf_lut->sf[row2][i],
				sf_lut->percent[row2],
				pc);
		return scalefactor;
	}

	scalefactorrow1 = linear_interpolate(
				sf_lut->sf[row1][i - 1],
				sf_lut->row_entries[i - 1],
				sf_lut->sf[row1][i],
				sf_lut->row_entries[i],
				row_entry);

	scalefactorrow2 = linear_interpolate(
				sf_lut->sf[row2][i - 1],
				sf_lut->row_entries[i - 1],
				sf_lut->sf[row2][i],
				sf_lut->row_entries[i],
				row_entry);

	scalefactor = linear_interpolate(
				scalefactorrow1,
				sf_lut->percent[row1],
				scalefactorrow2,
				sf_lut->percent[row2],
				pc);

	return scalefactor;
}

static int is_between(int left, int right, int value)
{
	if (left >= right && left >= value && value >= right)
		return 1;
	if (left <= right && left <= value && value <= right)
		return 1;

	return 0;
}

static int interpolate_pc(struct pm8921_bms_chip *chip,
				int batt_temp, int ocv)
{
	int i, j, pcj, pcj_minus_one, pc;
	int rows = chip->pc_temp_ocv_lut->rows;
	int cols = chip->pc_temp_ocv_lut->cols;

	
	batt_temp = batt_temp/10;

	if (batt_temp < chip->pc_temp_ocv_lut->temp[0]) {
		pr_debug("batt_temp %d < known temp range for pc\n", batt_temp);
		batt_temp = chip->pc_temp_ocv_lut->temp[0];
	}
	if (batt_temp > chip->pc_temp_ocv_lut->temp[cols - 1]) {
		pr_debug("batt_temp %d > known temp range for pc\n", batt_temp);
		batt_temp = chip->pc_temp_ocv_lut->temp[cols - 1];
	}

	for (j = 0; j < cols; j++)
		if (batt_temp <= chip->pc_temp_ocv_lut->temp[j])
			break;
	if (batt_temp == chip->pc_temp_ocv_lut->temp[j]) {
		
		if (ocv >= chip->pc_temp_ocv_lut->ocv[0][j])
			return chip->pc_temp_ocv_lut->percent[0];
		if (ocv <= chip->pc_temp_ocv_lut->ocv[rows - 1][j])
			return chip->pc_temp_ocv_lut->percent[rows - 1];
		for (i = 0; i < rows; i++) {
			if (ocv >= chip->pc_temp_ocv_lut->ocv[i][j]) {
				if (ocv == chip->pc_temp_ocv_lut->ocv[i][j])
					return
					chip->pc_temp_ocv_lut->percent[i];
				pc = linear_interpolate(
					chip->pc_temp_ocv_lut->percent[i],
					chip->pc_temp_ocv_lut->ocv[i][j],
					chip->pc_temp_ocv_lut->percent[i - 1],
					chip->pc_temp_ocv_lut->ocv[i - 1][j],
					ocv);
				return pc;
			}
		}
	}

	if (ocv >= chip->pc_temp_ocv_lut->ocv[0][j])
		return chip->pc_temp_ocv_lut->percent[0];
	if (ocv <= chip->pc_temp_ocv_lut->ocv[rows - 1][j - 1])
		return chip->pc_temp_ocv_lut->percent[rows - 1];

	pcj_minus_one = 0;
	pcj = 0;
	for (i = 0; i < rows-1; i++) {
		if (pcj == 0
			&& is_between(chip->pc_temp_ocv_lut->ocv[i][j],
				chip->pc_temp_ocv_lut->ocv[i+1][j], ocv)) {
			pcj = linear_interpolate(
				chip->pc_temp_ocv_lut->percent[i],
				chip->pc_temp_ocv_lut->ocv[i][j],
				chip->pc_temp_ocv_lut->percent[i + 1],
				chip->pc_temp_ocv_lut->ocv[i+1][j],
				ocv);
		}

		if (pcj_minus_one == 0
			&& is_between(chip->pc_temp_ocv_lut->ocv[i][j-1],
				chip->pc_temp_ocv_lut->ocv[i+1][j-1], ocv)) {

			pcj_minus_one = linear_interpolate(
				chip->pc_temp_ocv_lut->percent[i],
				chip->pc_temp_ocv_lut->ocv[i][j-1],
				chip->pc_temp_ocv_lut->percent[i + 1],
				chip->pc_temp_ocv_lut->ocv[i+1][j-1],
				ocv);
		}

		if (pcj && pcj_minus_one) {
			pc = linear_interpolate(
				pcj_minus_one,
				chip->pc_temp_ocv_lut->temp[j-1],
				pcj,
				chip->pc_temp_ocv_lut->temp[j],
				batt_temp);
			return pc;
		}
	}

	if (pcj)
		return pcj;

	if (pcj_minus_one)
		return pcj_minus_one;

	pr_debug("%d ocv wasn't found for temp %d in the LUT returning 100%%",
							ocv, batt_temp);
	return 100;
}

#define BMS_MODE_BIT			BIT(6)
#define EN_VBAT_BIT				BIT(5)
#define OVERRIDE_MODE_DELAY_MS	20
int pm8921_bms_get_simultaneous_battery_voltage_and_current(int *ibat_ua,
								int *vbat_uv)
{
	int16_t vsense_raw;
	int16_t vbat_raw;
	int vsense_uv, usb_chg;

	if (the_chip == NULL) {
		pr_err("Called to early\n");
		return -EINVAL;
	}

	mutex_lock(&the_chip->bms_output_lock);

	pm8xxx_writeb(the_chip->dev->parent, BMS_S1_DELAY, 0x00);
	pm_bms_masked_write(the_chip, BMS_CONTROL,
			BMS_MODE_BIT | EN_VBAT_BIT, BMS_MODE_BIT | EN_VBAT_BIT);

	msleep(OVERRIDE_MODE_DELAY_MS);

	pm_bms_lock_output_data(the_chip);
	pm_bms_read_output_data(the_chip, VSENSE_AVG, &vsense_raw);
	pm_bms_read_output_data(the_chip, VBATT_AVG, &vbat_raw);
	pm_bms_unlock_output_data(the_chip);
	pm_bms_masked_write(the_chip, BMS_CONTROL,
			BMS_MODE_BIT | EN_VBAT_BIT, 0);

	pm8xxx_writeb(the_chip->dev->parent, BMS_S1_DELAY, 0x0B);

	mutex_unlock(&the_chip->bms_output_lock);

	usb_chg = usb_chg_plugged_in();
	convert_vbatt_raw_to_uv(the_chip, usb_chg, vbat_raw, vbat_uv);
	convert_vsense_to_uv(the_chip, vsense_raw, &vsense_uv);
	*ibat_ua = vsense_uv * 1000 / (int)the_chip->r_sense;

	pr_debug("vsense_raw = 0x%x vbat_raw = 0x%x"
			" ibat_ua = %d vbat_uv = %d\n",
			(uint16_t)vsense_raw, (uint16_t)vbat_raw,
			*ibat_ua, *vbat_uv);
	return 0;
}
EXPORT_SYMBOL(pm8921_bms_get_simultaneous_battery_voltage_and_current);

static int read_rbatt_params_raw(struct pm8921_bms_chip *chip,
				struct pm8921_rbatt_params *raw)
{
	int usb_chg;

	mutex_lock(&chip->bms_output_lock);
	pm_bms_lock_output_data(chip);

	pm_bms_read_output_data(chip,
			OCV_FOR_RBATT, &raw->ocv_for_rbatt_raw);
	pm_bms_read_output_data(chip,
			VBATT_FOR_RBATT, &raw->vbatt_for_rbatt_raw);
	pm_bms_read_output_data(chip,
			VSENSE_FOR_RBATT, &raw->vsense_for_rbatt_raw);

	pm_bms_unlock_output_data(chip);
	mutex_unlock(&chip->bms_output_lock);

	usb_chg = usb_chg_plugged_in();
	convert_vbatt_raw_to_uv(chip, usb_chg,
			raw->vbatt_for_rbatt_raw, &raw->vbatt_for_rbatt_uv);
	convert_vbatt_raw_to_uv(chip, usb_chg,
			raw->ocv_for_rbatt_raw, &raw->ocv_for_rbatt_uv);
	convert_vsense_to_uv(chip,
			raw->vsense_for_rbatt_raw, &raw->vsense_for_rbatt_uv);

	pr_debug("vbatt_for_rbatt_raw = 0x%x, vbatt_for_rbatt= %duV\n",
			raw->vbatt_for_rbatt_raw, raw->vbatt_for_rbatt_uv);
	pr_debug("ocv_for_rbatt_raw = 0x%x, ocv_for_rbatt= %duV\n",
			raw->ocv_for_rbatt_raw, raw->ocv_for_rbatt_uv);
	pr_debug("vsense_for_rbatt_raw = 0x%x, vsense_for_rbatt= %duV\n",
			raw->vsense_for_rbatt_raw, raw->vsense_for_rbatt_uv);
	return 0;
}

#define MBG_TRANSIENT_ERROR_UV 15000
static void adjust_pon_ocv(struct pm8921_bms_chip *chip, int *uv)
{
	if (*uv >= MBG_TRANSIENT_ERROR_UV)
		*uv -= MBG_TRANSIENT_ERROR_UV;
}

int pm8921_store_hw_reset_reason(int is_hw_reset)
{
	int rc = 0;
	u8 reset = 0;
	u8 ocv_hw_reset_old = 0, ocv_hw_reset = 0;

	if (!the_chip) {
		pr_err("%s called before initialization\n", __func__);
		return -EINVAL;
	}

	if (is_hw_reset)
		reset = BIT(1);
	else
		reset = 0;

	
	rc = pm8xxx_readb(the_chip->dev->parent, OCV_UPDATE_STORAGE, &ocv_hw_reset_old);
	if (rc) {
		pr_err("%s: failed to read addr = %d, rc=%d\n",
				__func__, OCV_UPDATE_STORAGE, rc);
	}

	pm_bms_masked_write(the_chip, OCV_UPDATE_STORAGE, OCV_HW_RESET_MASK, reset);

	rc = pm8xxx_readb(the_chip->dev->parent, OCV_UPDATE_STORAGE, &ocv_hw_reset);
	if (rc) {
		pr_err("%s: failed to read addr = %d, rc=%d\n",
				__func__, OCV_UPDATE_STORAGE, rc);
	}

	pr_info("%s OCV_UPDATE_STORAGE=0x%x->0x%x\n", __func__, ocv_hw_reset_old, ocv_hw_reset);

	return (int)ocv_hw_reset;
}

EXPORT_SYMBOL(pm8921_store_hw_reset_reason);

static int read_soc_params_raw(struct pm8921_bms_chip *chip,
				struct pm8921_soc_params *raw)
{
	int usb_chg, rc;
	u8 ocv_updated_flag = 0;

	rc = pm8xxx_readb(chip->dev->parent, OCV_UPDATE_STORAGE, &ocv_updated_flag);
	if (rc) {
		pr_err("%s: failed to read addr = %d, rc=%d\n",
				__func__, OCV_UPDATE_STORAGE, rc);
	} else {
		ocv_updated_flag &= OCV_UPDATE_STORAGE_USE_MASK;
		pr_debug("%s: OCV_UPDATE_STORAGE = 0x%x\n", __func__, ocv_updated_flag);
	}

	mutex_lock(&chip->bms_output_lock);
	pm_bms_lock_output_data(chip);

	pm_bms_read_output_data(chip,
			LAST_GOOD_OCV_VALUE, &raw->last_good_ocv_raw);
	read_cc(chip, &raw->cc);

	pm_bms_unlock_output_data(chip);
	mutex_unlock(&chip->bms_output_lock);

	usb_chg =  usb_chg_plugged_in();
	pr_debug("%s: usb_chg=%d, usb_chg_plugged_ready=%d,"
			"prev_last_good_ocv_raw=0x%x, last_good_ocv_raw=0x%x\n",
			__func__, usb_chg, chip->usb_chg_plugged_ready,
			chip->prev_last_good_ocv_raw, raw->last_good_ocv_raw);
	if (chip->prev_last_good_ocv_raw == 0) {
		if (chip->usb_chg_plugged_ready == 1)
			chip->prev_last_good_ocv_raw = raw->last_good_ocv_raw;
		convert_vbatt_raw_to_uv(chip, usb_chg,
			raw->last_good_ocv_raw, &raw->last_good_ocv_uv);
		bms_dbg.last_ocv_raw_uv = raw->last_good_ocv_uv;
		if (!ocv_updated_flag)
			adjust_pon_ocv(chip, &raw->last_good_ocv_uv);
		else
			pr_info("%s: Skip adjust_pon_ocv_raw due to ocv_updated_flag=0x%x\n",
					__func__, ocv_updated_flag);
		if(read_backup_cc_uv() != 0) {
			chip->cc_backup_uv = read_backup_cc_uv();
			chip->ocv_reading_at_100 = read_backup_ocv_at_100();
			chip->ocv_backup_uv = read_backup_ocv_uv();
			if (chip->ocv_reading_at_100 > 0)
				last_ocv_uv = chip->max_voltage_uv;
			else
				raw->last_good_ocv_uv = last_ocv_uv = chip->ocv_backup_uv;
		} else {
			last_ocv_uv = raw->last_good_ocv_uv;
		}
		pr_info("%s: last_good_ocv_raw=0x%x, last_good_ocv_uv/ori=%duV/%duV"
				"ocv_reading_at_100=%x, cc_backup_uv=%d, ocv_backup_uv=%d, last_ocv_uv=%d\n",
				__func__, raw->last_good_ocv_raw, raw->last_good_ocv_uv,
				bms_dbg.last_ocv_raw_uv, chip->ocv_reading_at_100,
				chip->cc_backup_uv, chip->ocv_backup_uv, last_ocv_uv);
	} else if (chip->prev_last_good_ocv_raw != raw->last_good_ocv_raw) {
		convert_vbatt_raw_to_uv(chip, usb_chg,
			raw->last_good_ocv_raw, &raw->last_good_ocv_uv);
		
		if (raw->last_good_ocv_uv <= BATT_MIN_OCV_UV
			|| raw->last_good_ocv_uv > BATT_MAX_OCV_UV) {
			pr_info("%s: abnormal hw ocv_raw=%x, ocv_uv=%duV, raw.cc=%x",
				__func__, raw->last_good_ocv_raw, raw->last_good_ocv_uv, raw->cc);
			raw->last_good_ocv_raw = chip->prev_last_good_ocv_raw;
			convert_vbatt_raw_to_uv(chip, usb_chg,
				raw->last_good_ocv_raw, &raw->last_good_ocv_uv);
			return 0;
		} else
			chip->prev_last_good_ocv_raw = raw->last_good_ocv_raw;

		bms_dbg.last_ocv_raw_uv = last_ocv_uv = raw->last_good_ocv_uv;

		
		chip->ocv_reading_at_100 = 0;
		chip->cc_backup_uv = 0;
		chip->ocv_backup_uv = 0;
		write_backup_cc_uv(chip->cc_backup_uv);
		write_backup_ocv_at_100(chip->ocv_reading_at_100);
		write_backup_ocv_uv(chip->ocv_backup_uv);
		htc_batt_bms_timer.no_ocv_update_period_ms = 0;

		pm_bms_masked_write(chip, OCV_UPDATE_STORAGE,
							OCV_UPDATE_STORAGE_USE_MASK, 0x1);
		rc = pm8xxx_readb(chip->dev->parent, OCV_UPDATE_STORAGE, &ocv_updated_flag);
		if (rc) {
			pr_err("%s: failed to read addr = %d, rc=%d\n",
					__func__, OCV_UPDATE_STORAGE, rc);
		} else {
			ocv_updated_flag &= OCV_UPDATE_STORAGE_USE_MASK;
		}
		pr_info("%s: last_good_ocv_raw/uv=0x%x/%duV, ocv_updated_flag=0x%x\n",
				__func__, raw->last_good_ocv_raw, raw->last_good_ocv_uv,
				ocv_updated_flag);
	} else {
		raw->last_good_ocv_uv = last_ocv_uv;
	}

	pr_debug("0p625 = %duV\n", chip->xoadc_v0625);
	pr_debug("1p25 = %duV\n", chip->xoadc_v125);
	pr_debug("last_good_ocv_raw= 0x%x, last_good_ocv_uv= %duV\n",
			raw->last_good_ocv_raw, raw->last_good_ocv_uv);
	pr_debug("cc_raw= 0x%x\n", raw->cc);
	return 0;
}

static int get_rbatt(struct pm8921_bms_chip *chip, int soc_rbatt, int batt_temp)
{
	int rbatt, scalefactor;

	rbatt = (last_rbatt < 0) ? chip->default_rbatt_mohm : last_rbatt;
	pr_debug("rbatt before scaling = %d\n", rbatt);
	if (chip->rbatt_sf_lut == NULL)  {
		pr_debug("RBATT = %d\n", rbatt);
		return rbatt;
	}

	
	batt_temp = batt_temp / 10;
	scalefactor = interpolate_scalingfactor(chip, chip->rbatt_sf_lut,
							batt_temp, soc_rbatt);
	bms_dbg.rbatt_sf = scalefactor;
	bms_dbg.soc_rbatt = soc_rbatt;
	pr_debug("rbatt sf = %d for batt_temp = %d, soc_rbatt = %d\n",
				scalefactor, batt_temp, soc_rbatt);
	rbatt = (rbatt * scalefactor) / 100;

	rbatt += the_chip->rconn_mohm;
	pr_debug("adding rconn_mohm = %d rbatt = %d\n",
				the_chip->rconn_mohm, rbatt);

	if (is_between(20, 10, soc_rbatt))
		rbatt = rbatt
			+ ((20 - soc_rbatt) * chip->delta_rbatt_mohm) / 10;
	else
		if (is_between(10, 0, soc_rbatt))
			rbatt = rbatt + chip->delta_rbatt_mohm;

	pr_debug("RBATT = %d\n", rbatt);
	return rbatt;
}

static int calculate_rbatt_resume(struct pm8921_bms_chip *chip,
				struct pm8921_rbatt_params *raw)
{
	unsigned int  r_batt;

	if (raw->ocv_for_rbatt_uv <= 0
		|| raw->ocv_for_rbatt_uv <= raw->vbatt_for_rbatt_uv
		|| raw->vsense_for_rbatt_raw <= 0) {
		pr_debug("rbatt readings unavailable ocv = %d, vbatt = %d,"
					"vsen = %d\n",
					raw->ocv_for_rbatt_uv,
					raw->vbatt_for_rbatt_uv,
					raw->vsense_for_rbatt_raw);
		return -EINVAL;
	}
	r_batt = ((raw->ocv_for_rbatt_uv - raw->vbatt_for_rbatt_uv)
			* chip->r_sense) / raw->vsense_for_rbatt_uv;
	pr_debug("r_batt = %umilliOhms", r_batt);
	return r_batt;
}

static int calculate_fcc_uah(struct pm8921_bms_chip *chip, int batt_temp,
							int chargecycles)
{
	int initfcc, result, scalefactor = 0;

	if (chip->adjusted_fcc_temp_lut == NULL) {
		initfcc = interpolate_fcc(chip, batt_temp);

		scalefactor = interpolate_scalingfactor_fcc(chip, chargecycles);

		
		result = (initfcc * scalefactor * 1000) / 100;
		pr_debug("fcc = %d uAh\n", result);
		return result;
	} else {
		return 1000 * interpolate_fcc_adjusted(chip, batt_temp);
	}
}

static int get_battery_uvolts(struct pm8921_bms_chip *chip, int *uvolts)
{
	int rc;
	struct pm8xxx_adc_chan_result result;

	rc = pm8xxx_adc_read(chip->vbat_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					chip->vbat_channel, rc);
		return rc;
	}
	pr_debug("mvolts phy = %lld meas = 0x%llx", result.physical,
						result.measurement);
	*uvolts = (int)result.physical;
	return 0;
}

static int adc_based_ocv(struct pm8921_bms_chip *chip, int *ocv)
{
	int vbatt, rbatt, ibatt_ua, rc;

	rc = get_battery_uvolts(chip, &vbatt);
	if (rc) {
		pr_err("failed to read vbatt from adc rc = %d\n", rc);
		return rc;
	}

	rc =  pm8921_bms_get_battery_current(&ibatt_ua);
	if (rc) {
		pr_err("failed to read batt current rc = %d\n", rc);
		return rc;
	}

	rbatt = (last_rbatt < 0) ? chip->default_rbatt_mohm : last_rbatt;
	*ocv = vbatt + (ibatt_ua * rbatt)/1000;
	return 0;
}

static int calculate_pc(struct pm8921_bms_chip *chip, int ocv_uv, int batt_temp,
							int chargecycles)
{
	int pc, scalefactor;

	pc = interpolate_pc(chip, batt_temp, ocv_uv / 1000);
	pr_debug("pc = %u for ocv = %dmicroVolts batt_temp = %d\n",
					pc, ocv_uv, batt_temp);

	scalefactor = interpolate_scalingfactor(chip,
					chip->pc_sf_lut, chargecycles, pc);
	pr_debug("scalefactor = %u batt_temp = %d\n", scalefactor, batt_temp);

	bms_dbg.scalefactor = scalefactor;
	
	pc = (pc * scalefactor) / 100;
	return pc;
}

static void calculate_cc_uah(struct pm8921_bms_chip *chip, int cc, int *val)
{
	int64_t cc_voltage_uv, cc_nvh, cc_uah;

	cc_voltage_uv = cc;
	cc_voltage_uv -= chip->cc_backup_uv;
	pr_debug("cc = %d. after subtracting %d cc = %lld\n",
					cc, chip->cc_backup_uv,
					cc_voltage_uv);
	cc_voltage_uv = cc_to_microvolt(chip, cc_voltage_uv);
	cc_voltage_uv = pm8xxx_cc_adjust_for_gain(cc_voltage_uv);
	pr_debug("cc_voltage_uv = %lld microvolts\n", cc_voltage_uv);
	cc_nvh = ccmicrovolt_to_nvh(cc_voltage_uv);
	pr_debug("cc_nvh = %lld nano_volt_hour\n", cc_nvh);
	cc_uah = div_s64(cc_nvh, chip->r_sense);
	*val = cc_uah;
}

static int calculate_unusable_charge_uah(struct pm8921_bms_chip *chip,
				int rbatt, int fcc_uah,
				int batt_temp, int chargecycles)
{
	int voltage_unusable_uv, pc_unusable;

	
	voltage_unusable_uv = (rbatt * chip->i_test)
						+ (chip->v_failure * 1000);
	pc_unusable = calculate_pc(chip, voltage_unusable_uv,
						batt_temp, chargecycles);
	pr_debug("rbatt = %umilliOhms unusable_v =%d unusable_pc = %d\n",
			rbatt, voltage_unusable_uv, pc_unusable);
	bms_dbg.rbatt = rbatt;
	bms_dbg.voltage_unusable_uv = voltage_unusable_uv;
	bms_dbg.pc_unusable = pc_unusable;
	return (fcc_uah * pc_unusable) / 100;
}

static int calculate_remaining_charge_uah(struct pm8921_bms_chip *chip,
						struct pm8921_soc_params *raw,
						int fcc_uah, int batt_temp,
						int chargecycles)
{
	int ocv, pc;

	
	ocv = 0;
	if (chip->ocv_reading_at_100 != raw->last_good_ocv_raw) {
		ocv = raw->last_good_ocv_uv;
	} else {
		ocv = chip->max_voltage_uv;
	}

	if (ocv == 0) {
		ocv = last_ocv_uv;
		pr_debug("ocv not available using last_ocv_uv=%d\n", ocv);
	}

	pc = calculate_pc(chip, ocv, batt_temp, chargecycles);
	bms_dbg.rc_pc = pc;
	pr_debug("ocv = %d pc = %d\n", ocv, pc);
	return (fcc_uah * pc) / 100;
}

static void calculate_soc_params(struct pm8921_bms_chip *chip,
						struct pm8921_soc_params *raw,
						int batt_temp, int chargecycles,
						int *fcc_uah,
						int *unusable_charge_uah,
						int *remaining_charge_uah,
						int *cc_uah,
						int *rbatt)
{
	unsigned long flags;
	int soc_rbatt;

	*fcc_uah = calculate_fcc_uah(chip, batt_temp, chargecycles);
	pr_debug("FCC = %uuAh batt_temp = %d, cycles = %d\n",
					*fcc_uah, batt_temp, chargecycles);

	spin_lock_irqsave(&chip->bms_100_lock, flags);
	
	*remaining_charge_uah = calculate_remaining_charge_uah(chip, raw,
					*fcc_uah, batt_temp, chargecycles);
	pr_debug("RC = %uuAh\n", *remaining_charge_uah);

	
	calculate_cc_uah(chip, raw->cc, cc_uah);
	pr_debug("cc_uah = %duAh raw->cc = %x cc = %lld after subtracting %d\n",
				*cc_uah, raw->cc,
				(int64_t)raw->cc - chip->cc_backup_uv,
				chip->cc_backup_uv);
	spin_unlock_irqrestore(&chip->bms_100_lock, flags);

	soc_rbatt = ((*remaining_charge_uah - *cc_uah) * 100) / *fcc_uah;
	if (soc_rbatt < 0)
		soc_rbatt = 0;
	*rbatt = get_rbatt(chip, soc_rbatt, batt_temp);

	*unusable_charge_uah = calculate_unusable_charge_uah(chip, *rbatt,
					*fcc_uah, batt_temp, chargecycles);
	pr_debug("UUC = %uuAh\n", *unusable_charge_uah);
}

static int calculate_real_fcc_uah(struct pm8921_bms_chip *chip,
				struct pm8921_soc_params *raw,
				int batt_temp, int chargecycles,
				int *ret_fcc_uah)
{
	int fcc_uah, unusable_charge_uah;
	int remaining_charge_uah;
	int cc_uah;
	int real_fcc_uah;
	int rbatt;

	calculate_soc_params(chip, raw, batt_temp, chargecycles,
						&fcc_uah,
						&unusable_charge_uah,
						&remaining_charge_uah,
						&cc_uah,
						&rbatt);

	real_fcc_uah = remaining_charge_uah - cc_uah;
	*ret_fcc_uah = fcc_uah;
	pr_debug("real_fcc = %d, RC = %d CC = %d fcc = %d\n",
			real_fcc_uah, remaining_charge_uah, cc_uah, fcc_uah);
	return real_fcc_uah;
}
static int calculate_state_of_charge(struct pm8921_bms_chip *chip,
					struct pm8921_soc_params *raw,
					int batt_temp, int chargecycles, int verbol)
{
	int remaining_usable_charge_uah, fcc_uah, unusable_charge_uah;
	int remaining_charge_uah, soc, soc_remainder = 0;
	int update_userspace = 1;
	int cc_uah;
	int rbatt;

	calculate_soc_params(chip, raw, batt_temp, chargecycles,
						&fcc_uah,
						&unusable_charge_uah,
						&remaining_charge_uah,
						&cc_uah,
						&rbatt);
	bms_dbg.batt_temp = batt_temp;

	
	remaining_usable_charge_uah = remaining_charge_uah
					- cc_uah
					- unusable_charge_uah;

	pr_debug("RUC = %duAh\n", remaining_usable_charge_uah);
	if (fcc_uah - unusable_charge_uah <= 0) {
		pr_warn("FCC = %duAh, UUC = %duAh forcing soc = 0\n",
						fcc_uah, unusable_charge_uah);
		soc = 0;
	} else {
		soc = (remaining_usable_charge_uah * 100)
			/ (fcc_uah - unusable_charge_uah);
		soc_remainder = (remaining_usable_charge_uah * 100)
			% (fcc_uah - unusable_charge_uah);
		
		if (soc >= 0 && soc_remainder > 0)
			soc += 1;
	}

	if (verbol) {
		pr_info("FCC=%d,UC=%d,RC=%d,CC=%d,CC_reset=%d,RUC=%d,SOC=%d,"
			       "SOC_R=%d,start_percent=%d,end_percent=%d,OCV=%d,OCV_raw=%d,"
			       "rbatt=%d,rbatt_sf=%d,batt_temp=%d,soc_rbatt=%d,last_rbatt=%d,"
			       "V_unusable_uv=%d,pc_unusable=%d,rc_pc=%d,scalefactor=%d,"
			       "no_ocv_update_ms=%lu\n",
				fcc_uah, unusable_charge_uah, remaining_charge_uah,
				cc_uah, chip->cc_backup_uv, remaining_usable_charge_uah, soc,
				soc_remainder, chip->start_percent, chip->end_percent,
				raw->last_good_ocv_uv, bms_dbg.last_ocv_raw_uv,
				bms_dbg.rbatt, bms_dbg.rbatt_sf, bms_dbg.batt_temp,
				bms_dbg.soc_rbatt, last_rbatt, bms_dbg.voltage_unusable_uv,
				bms_dbg.pc_unusable, bms_dbg.rc_pc, bms_dbg.scalefactor,
				htc_batt_bms_timer.no_ocv_update_period_ms);
	}

	if (soc > 100)
		soc = 100;
	pr_debug("SOC = %u%%\n", soc);

	if (bms_fake_battery != -EINVAL) {
		pr_debug("Returning Fake SOC = %d%%\n", bms_fake_battery);
		return bms_fake_battery;
	}

	if (soc < 0) {
		pr_err("bad rem_usb_chg = %d rem_chg %d,"
				"cc_uah %d, unusb_chg %d\n",
				remaining_usable_charge_uah,
				remaining_charge_uah,
				cc_uah, unusable_charge_uah);

		pr_err("for bad rem_usb_chg last_ocv_uv = %d"
				"chargecycles = %d, batt_temp = %d"
				"fcc = %d soc =%d\n",
				last_ocv_uv, chargecycles, batt_temp,
				fcc_uah, soc);
		update_userspace = 0;
		soc = 0;
	}

	if (last_soc == -EINVAL || soc <= last_soc) {
		last_soc = update_userspace ? soc : last_soc;
		return soc;
	}

	if (the_chip->start_percent != -EINVAL) {
		last_soc = soc;
	} else {
		pr_info("soc = %d reporting last_soc = %d\n", soc, last_soc);
		soc = last_soc;
	}


	return soc;
}

#define MIN_DELTA_625_UV	1000
static void calib_hkadc(struct pm8921_bms_chip *chip)
{
	int voltage, rc;
	struct pm8xxx_adc_chan_result result;
	int usb_chg;
	int this_delta;

	rc = pm8xxx_adc_read(the_chip->ref1p25v_channel, &result);
	if (rc) {
		pr_err("ADC failed for 1.25volts rc = %d\n", rc);
		return;
	}
	voltage = xoadc_reading_to_microvolt(result.adc_code);

	pr_debug("result 1.25v = 0x%x, voltage = %duV adc_meas = %lld\n",
				result.adc_code, voltage, result.measurement);

	chip->xoadc_v125 = voltage;

	rc = pm8xxx_adc_read(the_chip->ref625mv_channel, &result);
	if (rc) {
		pr_err("ADC failed for 1.25volts rc = %d\n", rc);
		return;
	}
	voltage = xoadc_reading_to_microvolt(result.adc_code);

	usb_chg = usb_chg_plugged_in();
	pr_debug("result 0.625V = 0x%x, voltage = %duV adc_meas = %lld "
				"usb_chg = %d\n",
				result.adc_code, voltage, result.measurement,
				usb_chg);

	if (usb_chg)
		chip->xoadc_v0625_usb_present = voltage;
	else
		chip->xoadc_v0625_usb_absent = voltage;

	chip->xoadc_v0625 = voltage;
	if (chip->xoadc_v0625_usb_present && chip->xoadc_v0625_usb_absent) {
		this_delta = chip->xoadc_v0625_usb_present
						- chip->xoadc_v0625_usb_absent;
		pr_debug("this_delta= %duV\n", this_delta);
		if (this_delta > MIN_DELTA_625_UV)
			last_usb_cal_delta_uv = this_delta;
		pr_debug("625V_present= %d, 625V_absent= %d, delta = %duV\n",
			chip->xoadc_v0625_usb_present,
			chip->xoadc_v0625_usb_absent,
			last_usb_cal_delta_uv);
	}
}

static void calibrate_hkadc_work(struct work_struct *work)
{
	struct pm8921_bms_chip *chip = container_of(work,
				struct pm8921_bms_chip, calib_hkadc_work);

	calib_hkadc(chip);
}

void pm8921_bms_calibrate_hkadc(void)
{
	schedule_work(&the_chip->calib_hkadc_work);
}

int pm8921_bms_get_vsense_avg(int *result)
{
	int rc = -EINVAL;

	if (the_chip) {
		mutex_lock(&the_chip->bms_output_lock);
		pm_bms_lock_output_data(the_chip);
		rc = read_vsense_avg(the_chip, result);
		pm_bms_unlock_output_data(the_chip);
		mutex_unlock(&the_chip->bms_output_lock);
	} else
		pr_err("called before initialization\n");
	return rc;
}
EXPORT_SYMBOL(pm8921_bms_get_vsense_avg);

int pm8921_bms_get_battery_current(int *result_ua)
{
	int vsense;

	if (!the_chip) {
		pr_err("called before initialization\n");
		return -EINVAL;
	}
	if (the_chip->r_sense == 0) {
		pr_err("r_sense is zero\n");
		return -EINVAL;
	}

	mutex_lock(&the_chip->bms_output_lock);
	pm_bms_lock_output_data(the_chip);
	read_vsense_avg(the_chip, &vsense);
	pm_bms_unlock_output_data(the_chip);
	mutex_unlock(&the_chip->bms_output_lock);
	pr_debug("vsense=%duV\n", vsense);
	
	*result_ua = vsense * 1000 / (int)the_chip->r_sense;
	pr_debug("ibat=%duA\n", *result_ua);
	return 0;
}
EXPORT_SYMBOL(pm8921_bms_get_battery_current);

int pm8921_bms_get_percent_charge(void)
{
	int batt_temp, rc;
	struct pm8xxx_adc_chan_result result;
	struct pm8921_soc_params raw;

	if (!the_chip) {
		pr_err("called before initialization\n");
		return -EINVAL;
	}

	rc = pm8xxx_adc_read(the_chip->batt_temp_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					the_chip->batt_temp_channel, rc);
		return rc;
	}
	pr_debug("batt_temp phy = %lld meas = 0x%llx", result.physical,
						result.measurement);
	batt_temp = (int)result.physical;

	read_soc_params_raw(the_chip, &raw);

	return calculate_state_of_charge(the_chip, &raw,
					batt_temp, last_chargecycles, 0);
}
EXPORT_SYMBOL_GPL(pm8921_bms_get_percent_charge);

int pm8921_bms_get_rbatt(void)
{
	int batt_temp, rc;
	struct pm8xxx_adc_chan_result result;
	struct pm8921_soc_params raw;
	int fcc_uah;
	int unusable_charge_uah;
	int remaining_charge_uah;
	int cc_uah;
	int rbatt;

	if (!the_chip) {
		pr_err("called before initialization\n");
		return -EINVAL;
	}

	rc = pm8xxx_adc_read(the_chip->batt_temp_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					the_chip->batt_temp_channel, rc);
		return rc;
	}
	pr_debug("batt_temp phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);
	batt_temp = (int)result.physical;

	read_soc_params_raw(the_chip, &raw);

	calculate_soc_params(the_chip, &raw, batt_temp, last_chargecycles,
						&fcc_uah,
						&unusable_charge_uah,
						&remaining_charge_uah,
						&cc_uah,
						&rbatt);
	return rbatt;
}
EXPORT_SYMBOL_GPL(pm8921_bms_get_rbatt);

int pm8921_bms_get_fcc(void)
{
	int batt_temp, rc;
	struct pm8xxx_adc_chan_result result;

	if (!the_chip) {
		pr_err("called before initialization\n");
		return -EINVAL;
	}

	rc = pm8xxx_adc_read(the_chip->batt_temp_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					the_chip->batt_temp_channel, rc);
		return rc;
	}
	pr_debug("batt_temp phy = %lld meas = 0x%llx", result.physical,
						result.measurement);
	batt_temp = (int)result.physical;
	return calculate_fcc_uah(the_chip, batt_temp, last_chargecycles);
}
EXPORT_SYMBOL_GPL(pm8921_bms_get_fcc);

static void disable_ocv_update_with_reason(bool disable, int reason)
{
	int prev_ocv_update_stop_reason;
	mutex_lock(&ocv_update_lock);
	prev_ocv_update_stop_reason = ocv_update_stop_reason;
	if (ocv_update_stop_active_mask & reason) {
		if (disable)
			ocv_update_stop_reason |= reason;
		else
			ocv_update_stop_reason &= ~reason;

		if (prev_ocv_update_stop_reason ^ ocv_update_stop_reason) {
			pr_info("ocv_update_stop_reason:0x%x->0x%d\n",
							prev_ocv_update_stop_reason, ocv_update_stop_reason);
			if (!!prev_ocv_update_stop_reason != !!ocv_update_stop_reason) {
				if (!!ocv_update_stop_reason)
					pm8921_bms_stop_ocv_updates();
				else
					pm8921_bms_start_ocv_updates();
			}
		}
	}
	mutex_unlock(&ocv_update_lock);
}

static int get_rbatt_for_estimate_ocv(struct sf_lut *rbatt_lut, int temp)
{
	int x, y, rows, cols;

	rows = rbatt_lut->rows;
	cols = rbatt_lut->cols;
	for (x= 0; x < rows; x++) {
		for (y= 0; y < cols; y++) {
			if (temp < rbatt_lut->row_entries[y])
				return rbatt_lut->sf[x][y];
		}
	}
	return rbatt_lut->sf[rows-1][cols-1];
}

static int estimate_ocv(struct pm8921_bms_chip *chip, int ibatt_ua, int vbat_uv)
{
	int ocv_est_uv, batt_temp, rc;
	int rbatt_mohm, rbatt_for_estimated_ocv;
	struct pm8xxx_adc_chan_result result;

	if (!chip) {
		pr_info("%s: called before initialization\n", __func__);
		return -EINVAL;
	}
	
	rc = pm8xxx_adc_read(the_chip->batt_temp_channel, &result);
	if (rc) {
		pr_info("error reading adc channel = %d, rc = %d\n",
					the_chip->batt_temp_channel, rc);
		return rc;
	}
	pr_debug("batt_temp phy = %lld meas = 0x%llx\n", result.physical, result.measurement);
	batt_temp = (int)result.physical;

	
	if (batt_temp <= 0) {
		pr_info("%s: batt_temp=%d , return!\n", __func__, batt_temp);
		return 0;
	}
	
	if (chip->rbatt_est_ocv_lut == NULL)  {
		pr_info("%s: rbatt_est_ocv_lut is NULL\n", __func__);
		return 0;
	}
	rbatt_for_estimated_ocv = get_rbatt_for_estimate_ocv(chip->rbatt_est_ocv_lut,
		batt_temp/10);
	rbatt_mohm = rbatt_for_estimated_ocv + chip->rconn_mohm_sw_est_ocv;

	ocv_est_uv = vbat_uv + (ibatt_ua * rbatt_mohm) / 1000;
	pr_info("estimated ocv=%d, rbatt=%d, rconn=%d, ibatt_ua=%d, vbat_uv=%d, "
			"last_ocv_uv=%d, no_ocv_update_ms=%lu\n",
			ocv_est_uv, rbatt_for_estimated_ocv, chip->rconn_mohm_sw_est_ocv,
			ibatt_ua, vbat_uv, last_ocv_uv,
			htc_batt_bms_timer.no_ocv_update_period_ms);
	return ocv_est_uv;
}

static int pm8921_bms_estimate_ocv(void)
{
	int	rc;
	int	estimated_ocv_uv = 0;
	int 	ibatt_ua, vbat_uv;
	struct pm8921_soc_params raw;

	if (!the_chip) {
		pr_err("called before initialization\n");
		return -EINVAL;
	}

	
	rc = pm8921_bms_get_simultaneous_battery_voltage_and_current(
							&ibatt_ua, &vbat_uv);
	if (rc) {
		pr_err("%s, simultaneous failed rc = %d\n",__func__, rc);
		return rc;
	}

	
	if (ibatt_ua > 50000) {
		pr_info("%s: ibatt_ua=%d uA exceed 50mA, "
			       "no_ocv_update_ms=%lu, return!\n",
				__func__, ibatt_ua,
				htc_batt_bms_timer.no_ocv_update_period_ms);
		return 0;
	}

	mutex_lock(&the_chip->bms_output_lock);
	pm_bms_lock_output_data(the_chip);

	pm_bms_read_output_data(the_chip,
			LAST_GOOD_OCV_VALUE, &raw.last_good_ocv_raw);
	read_cc(the_chip, &raw.cc);

	pm_bms_unlock_output_data(the_chip);
	mutex_unlock(&the_chip->bms_output_lock);

	if (the_chip->prev_last_good_ocv_raw != raw.last_good_ocv_raw) {
		pr_info("ocv is updated by hw, pre_ocv_raw=%x, last_ocv_raw=%x, "
				"no_ocv_update_ms=%lu\n",
				the_chip->prev_last_good_ocv_raw, raw.last_good_ocv_raw,
				htc_batt_bms_timer.no_ocv_update_period_ms);
		return 0;
	}

	estimated_ocv_uv = estimate_ocv(the_chip, ibatt_ua, vbat_uv);

	
	if (estimated_ocv_uv > 0) {
		last_ocv_uv = estimated_ocv_uv;
		the_chip->cc_backup_uv = raw.cc;
		the_chip->ocv_backup_uv = last_ocv_uv;
		the_chip->ocv_reading_at_100 = 0;
		write_backup_cc_uv(the_chip->cc_backup_uv);
		write_backup_ocv_at_100(the_chip->ocv_reading_at_100);
		write_backup_ocv_uv(the_chip->ocv_backup_uv);
		htc_batt_bms_timer.no_ocv_update_period_ms = 0; 
	}
	return estimated_ocv_uv;
}

#ifdef CONFIG_HTC_BATT_8960
int pm8921_bms_get_batt_current(int *result)
{
	return pm8921_bms_get_battery_current(result);
}

int pm8921_bms_get_batt_soc(int *result)
{
	int batt_temp, rc;
	struct pm8xxx_adc_chan_result temp_result;
	struct pm8921_soc_params raw;
	unsigned long time_since_last_update_ms, cur_jiffies;

	if (!the_chip) {
		pr_err("called before initialization\n");
		return -EINVAL;
	}

	
	if (the_chip->criteria_sw_est_ocv > 0) {
		cur_jiffies = jiffies;
		time_since_last_update_ms =
			(cur_jiffies - htc_batt_bms_timer.batt_system_jiffies) * MSEC_PER_SEC / HZ;
		htc_batt_bms_timer.no_ocv_update_period_ms += time_since_last_update_ms;
		htc_batt_bms_timer.batt_system_jiffies = cur_jiffies;
	}

	rc = pm8xxx_adc_read(the_chip->batt_temp_channel, &temp_result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					the_chip->batt_temp_channel, rc);
		return rc;
	}
	pr_debug("batt_temp phy = %lld meas = 0x%llx", temp_result.physical,
						temp_result.measurement);
	batt_temp = (int)temp_result.physical;

	read_soc_params_raw(the_chip, &raw);

	*result = calculate_state_of_charge(the_chip, &raw,
					batt_temp, last_chargecycles, 1);
	if (bms_discharge_percent &&
			((bms_discharge_percent - *result) >=
				level_dropped_after_cable_out)) {
		pr_info("OCV can be update due to %d - %d >= %d\n",
				bms_discharge_percent, *result,
				level_dropped_after_cable_out);
		bms_discharge_percent = 0;
		disable_ocv_update_with_reason(false, OCV_UPDATE_STOP_BIT_CABLE_IN);
	}
	if (new_boot_soc &&
			((new_boot_soc - *result) >=
				level_dropped_after_boot_up)) {
		pr_info("OCV can be update due to %d - %d >= %d\n",
				new_boot_soc, *result,
				level_dropped_after_boot_up);
		new_boot_soc = 0;
		disable_ocv_update_with_reason(false, OCV_UPDATE_STOP_BIT_BOOT_UP);
	}
	if (the_chip->level_ocv_update_stop_begin &&
			the_chip->level_ocv_update_stop_end) {
		if (*result >= the_chip->level_ocv_update_stop_begin &&
				*result <= the_chip->level_ocv_update_stop_end)
			disable_ocv_update_with_reason(true, OCV_UPDATE_STOP_BIT_BATT_LEVEL);
		else
			disable_ocv_update_with_reason(false, OCV_UPDATE_STOP_BIT_BATT_LEVEL);
	}

	return 0;
}

int pm8921_bms_get_batt_cc(int *result)
{
	*result = dump_cc_uah();

	return 0;
}
#endif 

#define IBAT_TOL_MASK		0x0F
#define OCV_TOL		0xF0
#define OCV_TOL_MASK			0xF0
#define IBAT_TOL_DEFAULT	0x03
#define IBAT_TOL_NOCHG		0x0F
#define OCV_TOL_DEFAULT		0x20
#define OCV_TOL_NO_OCV		0x00
int pm8921_bms_charging_began(void)
{
	int batt_temp, rc = 0;
	struct pm8xxx_adc_chan_result result;
	struct pm8921_soc_params raw;

	if (!the_chip) {
		pr_err("called before initialization\n");
		return -EINVAL;
	}

	rc = pm8xxx_adc_read(the_chip->batt_temp_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
				the_chip->batt_temp_channel, rc);
		return rc;
	}
	pr_debug("batt_temp phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);
	batt_temp = (int)result.physical;

	read_soc_params_raw(the_chip, &raw);

	the_chip->start_percent = calculate_state_of_charge(the_chip, &raw,
					batt_temp, last_chargecycles, 0);
	bms_start_percent = the_chip->start_percent;
	bms_start_ocv_uv = raw.last_good_ocv_uv;
	calculate_cc_uah(the_chip, raw.cc, &bms_start_cc_uah);
	pm_bms_masked_write(the_chip, BMS_TOLERANCES,
			IBAT_TOL_MASK, IBAT_TOL_DEFAULT);
	pr_info("start_percent = %d%%\n", the_chip->start_percent);
	bms_discharge_percent = 0;
	disable_ocv_update_with_reason(true, OCV_UPDATE_STOP_BIT_CABLE_IN);

	return rc;
}
EXPORT_SYMBOL_GPL(pm8921_bms_charging_began);

#define DELTA_FCC_PERCENT	3
#define MIN_START_PERCENT_FOR_LEARNING	(-30)
void pm8921_bms_charging_end(int is_battery_full)
{
	int batt_temp, rc;
	struct pm8xxx_adc_chan_result result;
	struct pm8921_soc_params raw;
	struct timespec xtime;
	unsigned long currtime_ms;

	xtime = CURRENT_TIME;
	currtime_ms = xtime.tv_sec * MSEC_PER_SEC + xtime.tv_nsec / NSEC_PER_MSEC;

	if (the_chip == NULL)
		return;

	rc = pm8xxx_adc_read(the_chip->batt_temp_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
				the_chip->batt_temp_channel, rc);
		return;
	}
	pr_debug("batt_temp phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);
	batt_temp = (int)result.physical;

	read_soc_params_raw(the_chip, &raw);

	calculate_cc_uah(the_chip, raw.cc, &bms_end_cc_uah);

	if (is_battery_full
		&& the_chip->start_percent <= MIN_START_PERCENT_FOR_LEARNING) {
		int fcc_uah, new_fcc_uah, delta_fcc_uah;

		new_fcc_uah = calculate_real_fcc_uah(the_chip, &raw,
						batt_temp, last_chargecycles,
						&fcc_uah);
		delta_fcc_uah = new_fcc_uah - fcc_uah;
		if (delta_fcc_uah < 0)
			delta_fcc_uah = -delta_fcc_uah;

		if (delta_fcc_uah * 100  > (DELTA_FCC_PERCENT * fcc_uah)) {
			
			if (new_fcc_uah > fcc_uah)
				new_fcc_uah
				= (fcc_uah +
						(DELTA_FCC_PERCENT * fcc_uah) / 100);
			else
				new_fcc_uah
				= (fcc_uah -
						(DELTA_FCC_PERCENT * fcc_uah) / 100);
			pr_info("delta_fcc=%d > %d percent of fcc=%d"
					"restring it to %d\n",
					delta_fcc_uah, DELTA_FCC_PERCENT,
					fcc_uah, new_fcc_uah);
		}
		last_real_fcc_mah = new_fcc_uah/1000;
		last_real_fcc_batt_temp = batt_temp;
		readjust_fcc_table();
		pr_info("learnt fcc = %d batt_temp = %d\n",
					last_real_fcc_mah, last_real_fcc_batt_temp);
	}

	
	if (is_battery_full) {
		unsigned long flags;
		spin_lock_irqsave(&the_chip->bms_100_lock, flags);
		htc_batt_bms_timer.no_ocv_update_period_ms = 0;
		the_chip->ocv_reading_at_100 = raw.last_good_ocv_raw;
		the_chip->cc_backup_uv = raw.cc;
		the_chip->ocv_backup_uv = 0;
		write_backup_cc_uv(the_chip->cc_backup_uv);
		write_backup_ocv_at_100(the_chip->ocv_reading_at_100);
		write_backup_ocv_uv(the_chip->ocv_backup_uv);
		spin_unlock_irqrestore(&the_chip->bms_100_lock, flags);
		pr_info("EOC ocv_reading = 0x%x cc = %d\n",
				the_chip->ocv_reading_at_100,
				the_chip->cc_backup_uv);
		disable_ocv_update_with_reason(false, OCV_UPDATE_STOP_BIT_CABLE_IN);
	}

	the_chip->end_percent = calculate_state_of_charge(the_chip, &raw,
					batt_temp, last_chargecycles, 0);

	bms_end_percent = the_chip->end_percent;
	if (!is_battery_full)
		bms_discharge_percent = the_chip->end_percent;
	else
		bms_discharge_percent = 0;
	bms_end_ocv_uv = raw.last_good_ocv_uv;

	if (the_chip->end_percent > the_chip->start_percent) {
		last_charge_increase +=
			the_chip->end_percent - the_chip->start_percent;
		if (last_charge_increase > 100) {
			last_chargecycles++;
			last_charge_increase = last_charge_increase % 100;
		}
	}

	
	if (the_chip->store_batt_data_soc_thre > 0
			&& !usb_chg_plugged_in()
			&& bms_end_percent < the_chip->store_batt_data_soc_thre
			&& board_mfg_mode() == 5 ) {
		emmc_misc_write(BMS_STORE_MAGIC_NUM, BMS_STORE_MAGIC_OFFSET);
		emmc_misc_write(bms_end_percent, BMS_STORE_SOC_OFFSET);
		emmc_misc_write(raw.last_good_ocv_uv, BMS_STORE_OCV_OFFSET);
		emmc_misc_write(raw.cc, BMS_STORE_CC_OFFSET);
		emmc_misc_write(currtime_ms, BMS_STORE_CURRTIME_OFFSET);
	}

	pr_info("end_percent=%d%%, last_charge_increase=%d, last_chargecycles=%d, "
			"board_mfg_mode=%d, bms_end_percent=%d, last_good_ocv_uv=%d, raw.cc=%x, "
			"currtime_ms=%ld\n",
			the_chip->end_percent, last_charge_increase, last_chargecycles,
			board_mfg_mode(), bms_end_percent, raw.last_good_ocv_uv, raw.cc, currtime_ms);
	the_chip->start_percent = -EINVAL;
	the_chip->end_percent = -EINVAL;
	pm_bms_masked_write(the_chip, BMS_TOLERANCES,
				IBAT_TOL_MASK, IBAT_TOL_NOCHG);
}
EXPORT_SYMBOL_GPL(pm8921_bms_charging_end);

int pm8921_bms_stop_ocv_updates(void)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	if (!is_ocv_update_start) {
		pr_info("ocv updates is already stopped");
		return -EINVAL;
	}
	is_ocv_update_start = 0;
	pr_info("stopping ocv updates, is_ocv_update_start=%d", is_ocv_update_start);
	return pm_bms_masked_write(the_chip, BMS_TOLERANCES,
			OCV_TOL_MASK, OCV_TOL_NO_OCV);
}
EXPORT_SYMBOL_GPL(pm8921_bms_stop_ocv_updates);

int pm8921_bms_start_ocv_updates(void)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	if (is_ocv_update_start) {
		pr_info("ocv updates is already started");
		return -EINVAL;
	}
	is_ocv_update_start = 1;
	pr_info("starting ocv updates, is_ocv_update_start=%d", is_ocv_update_start);
	return pm_bms_masked_write(the_chip, BMS_TOLERANCES,
			OCV_TOL_MASK, OCV_TOL_DEFAULT);
}
EXPORT_SYMBOL_GPL(pm8921_bms_start_ocv_updates);

static irqreturn_t pm8921_bms_sbi_write_ok_handler(int irq, void *data)
{
	pr_debug("irq = %d triggered", irq);
	return IRQ_HANDLED;
}

static irqreturn_t pm8921_bms_cc_thr_handler(int irq, void *data)
{
	pr_debug("irq = %d triggered", irq);
	return IRQ_HANDLED;
}

static irqreturn_t pm8921_bms_vsense_thr_handler(int irq, void *data)
{
	pr_debug("irq = %d triggered", irq);
	return IRQ_HANDLED;
}

static irqreturn_t pm8921_bms_vsense_for_r_handler(int irq, void *data)
{
	pr_debug("irq = %d triggered", irq);
	return IRQ_HANDLED;
}

static irqreturn_t pm8921_bms_ocv_for_r_handler(int irq, void *data)
{
	struct pm8921_bms_chip *chip = data;

	pr_debug("irq = %d triggered", irq);
	schedule_work(&chip->calib_hkadc_work);
	return IRQ_HANDLED;
}

static irqreturn_t pm8921_bms_good_ocv_handler(int irq, void *data)
{
	struct pm8921_bms_chip *chip = data;

	pr_debug("irq = %d triggered", irq);
	schedule_work(&chip->calib_hkadc_work);
	return IRQ_HANDLED;
}

static irqreturn_t pm8921_bms_vsense_avg_handler(int irq, void *data)
{
	pr_debug("irq = %d triggered", irq);
	return IRQ_HANDLED;
}

struct pm_bms_irq_init_data {
	unsigned int	irq_id;
	char		*name;
	unsigned long	flags;
	irqreturn_t	(*handler)(int, void *);
};

#define BMS_IRQ(_id, _flags, _handler) \
{ \
	.irq_id		= _id, \
	.name		= #_id, \
	.flags		= _flags, \
	.handler	= _handler, \
}

struct pm_bms_irq_init_data bms_irq_data[] = {
	BMS_IRQ(PM8921_BMS_SBI_WRITE_OK, IRQF_TRIGGER_RISING,
				pm8921_bms_sbi_write_ok_handler),
	BMS_IRQ(PM8921_BMS_CC_THR, IRQF_TRIGGER_RISING,
				pm8921_bms_cc_thr_handler),
	BMS_IRQ(PM8921_BMS_VSENSE_THR, IRQF_TRIGGER_RISING,
				pm8921_bms_vsense_thr_handler),
	BMS_IRQ(PM8921_BMS_VSENSE_FOR_R, IRQF_TRIGGER_RISING,
				pm8921_bms_vsense_for_r_handler),
	BMS_IRQ(PM8921_BMS_OCV_FOR_R, IRQF_TRIGGER_RISING,
				pm8921_bms_ocv_for_r_handler),
	BMS_IRQ(PM8921_BMS_GOOD_OCV, IRQF_TRIGGER_RISING,
				pm8921_bms_good_ocv_handler),
	BMS_IRQ(PM8921_BMS_VSENSE_AVG, IRQF_TRIGGER_RISING,
				pm8921_bms_vsense_avg_handler),
};

static void free_irqs(struct pm8921_bms_chip *chip)
{
	int i;

	for (i = 0; i < PM_BMS_MAX_INTS; i++)
		if (chip->pmic_bms_irq[i]) {
			free_irq(chip->pmic_bms_irq[i], NULL);
			chip->pmic_bms_irq[i] = 0;
		}
}

static int __devinit request_irqs(struct pm8921_bms_chip *chip,
					struct platform_device *pdev)
{
	struct resource *res;
	int ret, i;

	ret = 0;
	bitmap_fill(chip->enabled_irqs, PM_BMS_MAX_INTS);

	for (i = 0; i < ARRAY_SIZE(bms_irq_data); i++) {
		res = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
				bms_irq_data[i].name);
		if (res == NULL) {
			pr_err("couldn't find %s\n", bms_irq_data[i].name);
			goto err_out;
		}
		ret = request_irq(res->start, bms_irq_data[i].handler,
			bms_irq_data[i].flags,
			bms_irq_data[i].name, chip);
		if (ret < 0) {
			pr_err("couldn't request %d (%s) %d\n", res->start,
					bms_irq_data[i].name, ret);
			goto err_out;
		}
		chip->pmic_bms_irq[bms_irq_data[i].irq_id] = res->start;
		pm8921_bms_disable_irq(chip, bms_irq_data[i].irq_id);
	}
	return 0;

err_out:
	free_irqs(chip);
	return -EINVAL;
}

#define EN_BMS_BIT	BIT(7)
#define EN_PON_HS_BIT	BIT(0)
static int __devinit pm8921_bms_hw_init(struct pm8921_bms_chip *chip)
{
	int rc;

	rc = pm_bms_masked_write(chip, BMS_CONTROL,
			EN_BMS_BIT | EN_PON_HS_BIT, EN_BMS_BIT | EN_PON_HS_BIT);
	if (rc) {
		pr_err("failed to enable pon and bms addr = %d %d",
				BMS_CONTROL, rc);
	}

	
	pm_bms_masked_write(chip, BMS_TOLERANCES,
				IBAT_TOL_MASK, IBAT_TOL_NOCHG);

	is_ocv_update_start = 1;
	pm_bms_masked_write(chip, BMS_TOLERANCES,
				OCV_TOL_MASK, OCV_TOL_DEFAULT);
	return 0;
}

static int bms_disabled;
static int set_disable_bms_param(const char *val, struct kernel_param *kp)
{
	u8 data;
	int rc;
	struct pm8921_bms_chip *chip = the_chip;

	rc = param_set_int(val, kp);

	if (rc) {
		pr_err("error setting value %d\n", rc);
		return rc;
	}

	if (bms_disabled)
		data = 0;
	else
		data = EN_BMS_BIT | EN_PON_HS_BIT;

	pr_info("set bms_disabled =%d\n", bms_disabled);
	rc = pm_bms_masked_write(chip, BMS_CONTROL,
			EN_BMS_BIT | EN_PON_HS_BIT, data);
	if (rc) {
		pr_err("failed to enable pon and bms addr = %d %d",
				BMS_CONTROL, rc);
	}

	return 0;
}
module_param_call(disabled, set_disable_bms_param, param_get_uint,
					&bms_disabled, 0644);

static void check_initial_ocv(struct pm8921_bms_chip *chip)
{
	int ocv_uv, rc;
	int16_t ocv_raw;
	int usb_chg;

	ocv_uv = 0;
	pm_bms_read_output_data(chip, LAST_GOOD_OCV_VALUE, &ocv_raw);
	usb_chg = usb_chg_plugged_in();
	rc = convert_vbatt_raw_to_uv(chip, usb_chg, ocv_raw, &ocv_uv);
	if (rc || ocv_uv == 0) {
		rc = adc_based_ocv(chip, &ocv_uv);
		if (rc) {
			pr_err("failed to read adc based ocv_uv rc = %d\n", rc);
			ocv_uv = DEFAULT_OCV_MICROVOLTS;
		}
		last_ocv_uv = ocv_uv;
	}
	pr_debug("ocv_uv = %d last_ocv_uv = %d\n", ocv_uv, last_ocv_uv);
}

static int64_t read_battery_id(struct pm8921_bms_chip *chip)
{
	int rc;
	struct pm8xxx_adc_chan_result result;

	rc = pm8xxx_adc_read(chip->batt_id_channel, &result);
	if (rc) {
		pr_err("error reading batt id channel = %d, rc = %d\n",
					chip->vbat_channel, rc);
		return rc;
	}
	pr_debug("batt_id phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);
	return result.physical;
}

#ifdef CONFIG_HTC_BATT_8960
#define PM8921_BMS_HTC_FAKE_BATT_ID (1)
static int set_battery_data(struct pm8921_bms_chip *chip)
{
	int battery_id_mv, batt_id;
	struct pm8921_bms_battery_data* bms_battery_data;

	
	
	if (pm8xxx_get_revision(chip->dev->parent) < PM8XXX_REVISION_8921_2p0
		&& pm8xxx_get_version(chip->dev->parent) == PM8XXX_VERSION_8921) {
		batt_id = PM8921_BMS_HTC_FAKE_BATT_ID;
		htc_battery_cell_set_cur_cell_by_id(batt_id);
	} else {
		battery_id_mv = (int)read_battery_id(chip) / 1000;
		
		batt_id = htc_battery_cell_find_and_set_id_auto(battery_id_mv);
	}

	
	bms_battery_data = htc_battery_cell_get_cur_cell_gauge_cdata();

	if (bms_battery_data) {
		pr_info("set bms_battery_data (cell_id=%d).\n",
					batt_id);
		chip->fcc = bms_battery_data->fcc;
		chip->fcc_temp_lut = bms_battery_data->fcc_temp_lut;
		chip->fcc_sf_lut = bms_battery_data->fcc_sf_lut;
		chip->pc_temp_ocv_lut = bms_battery_data->pc_temp_ocv_lut;
		chip->pc_sf_lut = bms_battery_data->pc_sf_lut;
		chip->rbatt_sf_lut = bms_battery_data->rbatt_sf_lut;
		chip->rbatt_est_ocv_lut = bms_battery_data->rbatt_est_ocv_lut;
		chip->default_rbatt_mohm
				= bms_battery_data->default_rbatt_mohm;
		chip->delta_rbatt_mohm
				= bms_battery_data->delta_rbatt_mohm;
		if (bms_battery_data->level_ocv_update_stop_begin
			&& bms_battery_data->level_ocv_update_stop_end) {
			chip->level_ocv_update_stop_begin = bms_battery_data->level_ocv_update_stop_begin;
			chip->level_ocv_update_stop_end = bms_battery_data->level_ocv_update_stop_end;
			ocv_update_stop_active_mask = ocv_update_stop_active_mask |
											OCV_UPDATE_STOP_BIT_BATT_LEVEL;
		}
	} else {
		pr_err("bms_battery_data doesn't exist (id=%d)\n",
					batt_id);
		chip->fcc = palladium_1500_data.fcc;
		chip->fcc_temp_lut = palladium_1500_data.fcc_temp_lut;
		chip->fcc_sf_lut = palladium_1500_data.fcc_sf_lut;
		chip->pc_temp_ocv_lut = palladium_1500_data.pc_temp_ocv_lut;
		chip->pc_sf_lut = palladium_1500_data.pc_sf_lut;
		chip->rbatt_sf_lut = palladium_1500_data.rbatt_sf_lut;
		chip->default_rbatt_mohm
				= palladium_1500_data.default_rbatt_mohm;
		chip->delta_rbatt_mohm
				= palladium_1500_data.delta_rbatt_mohm;
	}
	return 0;
}
#else
#define PALLADIUM_ID_MIN	0x7F40
#define PALLADIUM_ID_MAX	0x7F5A
#define DESAY_5200_ID_MIN	0x7F7F
#define DESAY_5200_ID_MAX	0x802F
static int set_battery_data(struct pm8921_bms_chip *chip)
{
	int64_t battery_id;

	battery_id = read_battery_id(chip);

	if (battery_id < 0) {
		pr_err("cannot read battery id err = %lld\n", battery_id);
		return battery_id;
	}

	if (is_between(PALLADIUM_ID_MIN, PALLADIUM_ID_MAX, battery_id)) {
		chip->fcc = palladium_1500_data.fcc;
		chip->fcc_temp_lut = palladium_1500_data.fcc_temp_lut;
		chip->fcc_sf_lut = palladium_1500_data.fcc_sf_lut;
		chip->pc_temp_ocv_lut = palladium_1500_data.pc_temp_ocv_lut;
		chip->pc_sf_lut = palladium_1500_data.pc_sf_lut;
		chip->rbatt_sf_lut = palladium_1500_data.rbatt_sf_lut;
		chip->default_rbatt_mohm
				= palladium_1500_data.default_rbatt_mohm;
		chip->delta_rbatt_mohm
				= palladium_1500_data.delta_rbatt_mohm;
		return 0;
	} else if (is_between(DESAY_5200_ID_MIN, DESAY_5200_ID_MAX,
				battery_id)) {
		chip->fcc = desay_5200_data.fcc;
		chip->fcc_temp_lut = desay_5200_data.fcc_temp_lut;
		chip->fcc_sf_lut = desay_5200_data.fcc_sf_lut;
		chip->pc_temp_ocv_lut = desay_5200_data.pc_temp_ocv_lut;
		chip->pc_sf_lut = desay_5200_data.pc_sf_lut;
		chip->rbatt_sf_lut = desay_5200_data.rbatt_sf_lut;
		chip->default_rbatt_mohm = desay_5200_data.default_rbatt_mohm;
		chip->delta_rbatt_mohm = desay_5200_data.delta_rbatt_mohm;
		return 0;
	} else {
		pr_warn("invalid battery id, palladium 1500 assumed batt_id %llx\n",
				battery_id);
		chip->fcc = palladium_1500_data.fcc;
		chip->fcc_temp_lut = palladium_1500_data.fcc_temp_lut;
		chip->fcc_sf_lut = palladium_1500_data.fcc_sf_lut;
		chip->pc_temp_ocv_lut = palladium_1500_data.pc_temp_ocv_lut;
		chip->pc_sf_lut = palladium_1500_data.pc_sf_lut;
		chip->rbatt_sf_lut = palladium_1500_data.rbatt_sf_lut;
		chip->default_rbatt_mohm
				= palladium_1500_data.default_rbatt_mohm;
		chip->delta_rbatt_mohm
				= palladium_1500_data.delta_rbatt_mohm;
		return 0;
	}
}
#endif 

enum bms_request_operation {
	CALC_RBATT,
	CALC_FCC,
	CALC_PC,
	CALC_SOC,
	CALIB_HKADC,
	CALIB_CCADC,
	STOP_OCV,
	START_OCV,
	GET_VBAT_VSENSE_SIMULTANEOUS,
};

static int test_batt_temp = 5;
static int test_chargecycle = 150;
static int test_ocv = 3900000;
enum {
	TEST_BATT_TEMP,
	TEST_CHARGE_CYCLE,
	TEST_OCV,
};
static int get_test_param(void *data, u64 * val)
{
	switch ((int)data) {
	case TEST_BATT_TEMP:
		*val = test_batt_temp;
		break;
	case TEST_CHARGE_CYCLE:
		*val = test_chargecycle;
		break;
	case TEST_OCV:
		*val = test_ocv;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
static int set_test_param(void *data, u64  val)
{
	switch ((int)data) {
	case TEST_BATT_TEMP:
		test_batt_temp = (int)val;
		break;
	case TEST_CHARGE_CYCLE:
		test_chargecycle = (int)val;
		break;
	case TEST_OCV:
		test_ocv = (int)val;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(temp_fops, get_test_param, set_test_param, "%llu\n");

static int get_calc(void *data, u64 * val)
{
	int param = (int)data;
	int ret = 0;
	int ibat_ua, vbat_uv;
	struct pm8921_soc_params raw;
	struct pm8921_rbatt_params rraw;

	read_soc_params_raw(the_chip, &raw);
	read_rbatt_params_raw(the_chip, &rraw);

	*val = 0;

	
	switch (param) {
	case CALC_RBATT:
		*val = calculate_rbatt_resume(the_chip, &rraw);
		break;
	case CALC_FCC:
		*val = calculate_fcc_uah(the_chip, test_batt_temp,
							test_chargecycle);
		break;
	case CALC_PC:
		*val = calculate_pc(the_chip, test_ocv, test_batt_temp,
							test_chargecycle);
		break;
	case CALC_SOC:
		*val = calculate_state_of_charge(the_chip, &raw,
					test_batt_temp, test_chargecycle, 0);
		break;
	case CALIB_HKADC:
		
		*val = 0;
		calib_hkadc(the_chip);
		break;
	case CALIB_CCADC:
		
		*val = 0;
		pm8xxx_calib_ccadc();
		break;
	case GET_VBAT_VSENSE_SIMULTANEOUS:
		
		*val =
		pm8921_bms_get_simultaneous_battery_voltage_and_current(
			&ibat_ua,
			&vbat_uv);
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int set_calc(void *data, u64 val)
{
	int param = (int)data;
	int ret = 0;

	switch (param) {
	case STOP_OCV:
		disable_ocv_update_with_reason(true, OCV_UPDATE_STOP_BIT_ATTR_FILE);
		break;
	case START_OCV:
		disable_ocv_update_with_reason(false, OCV_UPDATE_STOP_BIT_ATTR_FILE);
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}
DEFINE_SIMPLE_ATTRIBUTE(calc_fops, get_calc, set_calc, "%llu\n");

static int get_reading(void *data, u64 * val)
{
	int param = (int)data;
	int ret = 0;
	struct pm8921_soc_params raw;
	struct pm8921_rbatt_params rraw;

	read_soc_params_raw(the_chip, &raw);
	read_rbatt_params_raw(the_chip, &rraw);

	*val = 0;

	switch (param) {
	case CC_MSB:
	case CC_LSB:
		*val = raw.cc;
		break;
	case LAST_GOOD_OCV_VALUE:
		*val = raw.last_good_ocv_uv;
		break;
	case VBATT_FOR_RBATT:
		*val = rraw.vbatt_for_rbatt_uv;
		break;
	case VSENSE_FOR_RBATT:
		*val = rraw.vsense_for_rbatt_uv;
		break;
	case OCV_FOR_RBATT:
		*val = rraw.ocv_for_rbatt_uv;
		break;
	case VSENSE_AVG:
		read_vsense_avg(the_chip, (uint *)val);
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}
DEFINE_SIMPLE_ATTRIBUTE(reading_fops, get_reading, NULL, "%lld\n");

static int get_rt_status(void *data, u64 * val)
{
	int i = (int)data;
	int ret;

	
	ret = pm_bms_get_rt_status(the_chip, i);
	*val = ret;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(rt_fops, get_rt_status, NULL, "%llu\n");

static int get_reg(void *data, u64 * val)
{
	int addr = (int)data;
	int ret;
	u8 temp;

	ret = pm8xxx_readb(the_chip->dev->parent, addr, &temp);
	if (ret) {
		pr_err("pm8xxx_readb to %x value = %d errored = %d\n",
			addr, temp, ret);
		return -EAGAIN;
	}
	*val = temp;
	return 0;
}

static int set_reg(void *data, u64 val)
{
	int addr = (int)data;
	int ret;
	u8 temp;

	temp = (u8) val;
	ret = pm8xxx_writeb(the_chip->dev->parent, addr, temp);
	if (ret) {
		pr_err("pm8xxx_writeb to %x value = %d errored = %d\n",
			addr, temp, ret);
		return -EAGAIN;
	}
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(reg_fops, get_reg, set_reg, "0x%02llx\n");

static void dump_all(void)
{
	u64 val;
	unsigned int len =0;

	memset(batt_log_buf, 0, sizeof(BATT_LOG_BUF_LEN));

	
	get_reg((void *)BMS_CONTROL, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "CONTROL=0x%02llx,", val);
	get_reg((void *)BMS_OUTPUT0, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "OUTPUT0=0x%02llx,", val);
	get_reg((void *)BMS_OUTPUT1, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "OUTPUT1=0x%02llx,", val);
	get_reg((void *)BMS_TOLERANCES, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "TOLERANCES=0x%02llx,", val);
	get_reg((void *)BMS_TEST1, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "BMS_TEST1=0x%02llx,", val);
	get_reg((void *)OCV_UPDATE_STORAGE, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "OCV_UPDATE_STORAGE=0x%02llx,", val);

	
	get_reading((void *)CC_MSB, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "read_cc=0x%lld,", val);
	get_reading((void *)LAST_GOOD_OCV_VALUE, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "last_good_ocv=0x%lld,", val);
	get_reading((void *)VBATT_FOR_RBATT, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "vbatt_for_rbatt=0x%lld,", val);
	get_reading((void *)VSENSE_FOR_RBATT, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "vsense_for_rbatt=0x%lld,", val);
	get_reading((void *)OCV_FOR_RBATT, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "ocv_for_rbatt=0x%lld,", val);
	get_reading((void *)VSENSE_AVG, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "vsense_avg=0x%lld, ", val);

	
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "[irq]%d%d%d%d%d%d%d",
		pm_bms_get_rt_status(the_chip, PM8921_BMS_SBI_WRITE_OK),
		pm_bms_get_rt_status(the_chip, PM8921_BMS_CC_THR),
		pm_bms_get_rt_status(the_chip, PM8921_BMS_VSENSE_THR),
		pm_bms_get_rt_status(the_chip, PM8921_BMS_VSENSE_FOR_R),
		pm_bms_get_rt_status(the_chip, PM8921_BMS_OCV_FOR_R),
		pm_bms_get_rt_status(the_chip, PM8921_BMS_GOOD_OCV),
		pm_bms_get_rt_status(the_chip, PM8921_BMS_VSENSE_AVG));

	
	if(BATT_LOG_BUF_LEN - len <= 1)
		pr_info("batt log length maybe out of buffer range!!!");

	pr_info("%s\n", batt_log_buf);
	pm8xxx_ccadc_dump_all();
}

inline int pm8921_bms_dump_all(void)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	dump_all();
	return 0;
}
EXPORT_SYMBOL(pm8921_bms_dump_all);

int pm8921_bms_get_attr_text(char *buf, int size)
{
	struct pm8921_soc_params raw;
	struct pm8921_rbatt_params rraw;
	unsigned long flags;
	int len = 0;
	u64 val = 0;
	int cc_uah, fcc_uah, unusable_charge_uah, remaining_charge_uah;
	int chargecycles;
	int soc_rbatt, rbatt;
	int batt_temp, rc;
	struct pm8xxx_adc_chan_result result;

	if (!the_chip) {
		pr_err("driver not initialized\n");
		return 0;
	}

	get_reg((void *)BMS_CONTROL, &val);
	len += scnprintf(buf + len, size - len,
			"BMS_CONTROL: 0x%02llx;\n", val);
	get_reg((void *)BMS_OUTPUT0, &val);
	len += scnprintf(buf + len, size - len,
			"BMS_OUTPUT0: 0x%02llx;\n", val);
	get_reg((void *)BMS_OUTPUT1, &val);
	len += scnprintf(buf + len, size - len,
			"BMS_OUTPUT1: 0x%02llx;\n", val);
	get_reg((void *)BMS_TOLERANCES, &val);
	len += scnprintf(buf + len, size - len,
			"BMS_TOLERANCES: 0x%02llx;\n", val);
	get_reg((void *)BMS_TEST1, &val);
	len += scnprintf(buf + len, size - len,
			"BMS_TEST1: 0x%02llx;\n", val);
	get_reg((void *)OCV_UPDATE_STORAGE, &val);
	len += scnprintf(buf + len, size - len,
			"OCV_UPDATE_STORAGE: 0x%02llx;\n", val);

	len += scnprintf(buf + len, size - len,
			"bms_discharge_soc: %d;\n", bms_discharge_percent);
	len += scnprintf(buf + len, size - len,
			"is_ocv_update_start: %d;\n", is_ocv_update_start);
	len += scnprintf(buf + len, size - len,
			"ocv_update_stop_active_mask: 0x%x;\n", ocv_update_stop_active_mask);
	len += scnprintf(buf + len, size - len,
			"ocv_update_stop_reason: 0x%x;\n", ocv_update_stop_reason);

	read_soc_params_raw(the_chip, &raw);
	read_rbatt_params_raw(the_chip, &rraw);

	len += scnprintf(buf + len, size - len,
			"OCV_FOR_RBATT_RAW: 0x%x;\n", rraw.ocv_for_rbatt_raw);
	len += scnprintf(buf + len, size - len,
			"VBATT_FOR_RBATT_RAW: 0x%x;\n", rraw.vbatt_for_rbatt_raw);
	len += scnprintf(buf + len, size - len,
			"VSENSE_FOR_RBATT_RAW: 0x%x;\n", rraw.vsense_for_rbatt_raw);
	len += scnprintf(buf + len, size - len,
			"LAST_GOOD_OCV_RAW: 0x%x;\n", raw.last_good_ocv_raw);
	len += scnprintf(buf + len, size - len,
			"CC_RAW: 0x%x;\n", raw.cc);

	len += scnprintf(buf + len, size - len,
			"ocv_for_rbatt_uv: %d;\n", rraw.ocv_for_rbatt_uv);
	len += scnprintf(buf + len, size - len,
			"vbatt_for_rbatt_uv: %d;\n", rraw.vbatt_for_rbatt_uv);
	len += scnprintf(buf + len, size - len,
			"vsense_for_rbatt_uv: %d;\n", rraw.vsense_for_rbatt_uv);
	len += scnprintf(buf + len, size - len,
			"last_good_ocv_uv: %d;\n", raw.last_good_ocv_uv);
	len += scnprintf(buf + len, size - len,
			"last_ocv_raw_uv: %d;\n", bms_dbg.last_ocv_raw_uv);
	rc = pm8xxx_adc_read(the_chip->batt_temp_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
				the_chip->batt_temp_channel, rc);
		return len;
	}
	batt_temp = (int)result.physical;
	chargecycles = last_chargecycles;
	fcc_uah = calculate_fcc_uah(the_chip, batt_temp, chargecycles);
	remaining_charge_uah = calculate_remaining_charge_uah(the_chip, &raw,
					fcc_uah, batt_temp, chargecycles);
	spin_lock_irqsave(&the_chip->bms_100_lock, flags);
	calculate_cc_uah(the_chip, raw.cc, &cc_uah);
	spin_unlock_irqrestore(&the_chip->bms_100_lock, flags);
	soc_rbatt = ((remaining_charge_uah - cc_uah) * 100) / fcc_uah;
	if (soc_rbatt < 0)
		soc_rbatt = 0;
	rbatt = get_rbatt(the_chip, soc_rbatt, batt_temp);
	unusable_charge_uah = calculate_unusable_charge_uah(the_chip, rbatt,
					fcc_uah, batt_temp, chargecycles);
	len += scnprintf(buf + len, size - len,
			"rbatt(milliOhms): %d;\n", bms_dbg.rbatt);
	len += scnprintf(buf + len, size - len,
			"rbatt_scalefactor: %d;\n", bms_dbg.rbatt_sf);
	len += scnprintf(buf + len, size - len,
			"soc_rbatt(%%): %d;\n", bms_dbg.soc_rbatt);
	len += scnprintf(buf + len, size - len,
			"last_rbatt(%%): %d;\n", last_rbatt);
	len += scnprintf(buf + len, size - len,
			"voltage_unusable_uv(uV): %d;\n", bms_dbg.voltage_unusable_uv);
	len += scnprintf(buf + len, size - len,
			"pc_unusable(%%): %d;\n", bms_dbg.pc_unusable);
	len += scnprintf(buf + len, size - len,
			"rc_pc(%%): %d;\n", bms_dbg.rc_pc);
	len += scnprintf(buf + len, size - len,
			"scalefactor(): %d;\n", bms_dbg.scalefactor);
	len += scnprintf(buf + len, size - len,
			"fcc(uAh): %d;\n", fcc_uah);
	len += scnprintf(buf + len, size - len,
			"unusable_charge(uAh): %d;\n", unusable_charge_uah);
	len += scnprintf(buf + len, size - len,
			"remaining_charge(uAh): %d;\n", remaining_charge_uah);
	len += scnprintf(buf + len, size - len,
			"cc(uAh): %d;\n", cc_uah);
	len += scnprintf(buf + len, size - len,
			"chargecycles: %d;\n", chargecycles);
	len += scnprintf(buf + len, size - len,
			"start_percent: %d;\n", the_chip->start_percent);
	len += scnprintf(buf + len, size - len,
			"end_percent: %d;\n", the_chip->end_percent);

	
	len += pm8xxx_ccadc_get_attr_text(buf + len, size - len);

	return len;
}
EXPORT_SYMBOL(pm8921_bms_get_attr_text);

static void create_debugfs_entries(struct pm8921_bms_chip *chip)
{
	int i;

	chip->dent = debugfs_create_dir("pm8921-bms", NULL);

	if (IS_ERR(chip->dent)) {
		pr_err("pmic bms couldnt create debugfs dir\n");
		return;
	}

	debugfs_create_file("BMS_CONTROL", 0644, chip->dent,
			(void *)BMS_CONTROL, &reg_fops);
	debugfs_create_file("BMS_OUTPUT0", 0644, chip->dent,
			(void *)BMS_OUTPUT0, &reg_fops);
	debugfs_create_file("BMS_OUTPUT1", 0644, chip->dent,
			(void *)BMS_OUTPUT1, &reg_fops);
	debugfs_create_file("BMS_TEST1", 0644, chip->dent,
			(void *)BMS_TEST1, &reg_fops);

	debugfs_create_file("test_batt_temp", 0644, chip->dent,
				(void *)TEST_BATT_TEMP, &temp_fops);
	debugfs_create_file("test_chargecycle", 0644, chip->dent,
				(void *)TEST_CHARGE_CYCLE, &temp_fops);
	debugfs_create_file("test_ocv", 0644, chip->dent,
				(void *)TEST_OCV, &temp_fops);

	debugfs_create_file("read_cc", 0644, chip->dent,
				(void *)CC_MSB, &reading_fops);
	debugfs_create_file("read_last_good_ocv", 0644, chip->dent,
				(void *)LAST_GOOD_OCV_VALUE, &reading_fops);
	debugfs_create_file("read_vbatt_for_rbatt", 0644, chip->dent,
				(void *)VBATT_FOR_RBATT, &reading_fops);
	debugfs_create_file("read_vsense_for_rbatt", 0644, chip->dent,
				(void *)VSENSE_FOR_RBATT, &reading_fops);
	debugfs_create_file("read_ocv_for_rbatt", 0644, chip->dent,
				(void *)OCV_FOR_RBATT, &reading_fops);
	debugfs_create_file("read_vsense_avg", 0644, chip->dent,
				(void *)VSENSE_AVG, &reading_fops);

	debugfs_create_file("show_rbatt", 0644, chip->dent,
				(void *)CALC_RBATT, &calc_fops);
	debugfs_create_file("show_fcc", 0644, chip->dent,
				(void *)CALC_FCC, &calc_fops);
	debugfs_create_file("show_pc", 0644, chip->dent,
				(void *)CALC_PC, &calc_fops);
	debugfs_create_file("show_soc", 0644, chip->dent,
				(void *)CALC_SOC, &calc_fops);
	debugfs_create_file("calib_hkadc", 0644, chip->dent,
				(void *)CALIB_HKADC, &calc_fops);
	debugfs_create_file("calib_ccadc", 0644, chip->dent,
				(void *)CALIB_CCADC, &calc_fops);
	debugfs_create_file("stop_ocv", 0644, chip->dent,
				(void *)STOP_OCV, &calc_fops);
	debugfs_create_file("start_ocv", 0644, chip->dent,
				(void *)START_OCV, &calc_fops);

	debugfs_create_file("simultaneous", 0644, chip->dent,
			(void *)GET_VBAT_VSENSE_SIMULTANEOUS, &calc_fops);

	for (i = 0; i < ARRAY_SIZE(bms_irq_data); i++) {
		if (chip->pmic_bms_irq[bms_irq_data[i].irq_id])
			debugfs_create_file(bms_irq_data[i].name, 0444,
				chip->dent,
				(void *)bms_irq_data[i].irq_id,
				&rt_fops);
	}
}

static int dump_cc_uah(void)
{
	unsigned long flags;
	struct pm8921_soc_params raw;
	int cc_uah;

	if (!the_chip) {
		pr_err("driver not initialized\n");
		return 0;
	}
	read_soc_params_raw(the_chip, &raw);

	spin_lock_irqsave(&the_chip->bms_100_lock, flags);
	
	calculate_cc_uah(the_chip, raw.cc, &cc_uah);
	pr_info("cc_uah = %duAh, raw->cc = %x,"
			" cc = %lld after subtracting %d\n",
				cc_uah, raw.cc,
				(int64_t)raw.cc - the_chip->cc_backup_uv,
				the_chip->cc_backup_uv);
	spin_unlock_irqrestore(&the_chip->bms_100_lock, flags);
	return cc_uah;
}

int prev_cc_uah = 0;
static int pm8921_bms_suspend(struct device *dev)
{
	u64 val;
#if 0 
	int rc;
	struct pm8xxx_adc_chan_result result;
	struct pm8921_bms_chip *chip = dev_get_drvdata(dev);
	struct pm8921_soc_params raw;
	int fcc_uah;
	int remaining_charge_uah;
	int cc_uah;

	chip->batt_temp_suspend = 0;
	rc = pm8xxx_adc_read(chip->batt_temp_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					chip->batt_temp_channel, rc);
	}
	chip->batt_temp_suspend = (int)result.physical;
	read_soc_params_raw(chip, &raw);

	fcc_uah = calculate_fcc_uah(chip,
			chip->batt_temp_suspend, last_chargecycles);
	pr_debug("FCC = %uuAh batt_temp = %d, cycles = %d\n",
			fcc_uah, chip->batt_temp_suspend, last_chargecycles);
	
	remaining_charge_uah = calculate_remaining_charge_uah(chip, &raw,
					fcc_uah, chip->batt_temp_suspend,
					last_chargecycles);
	pr_debug("RC = %uuAh\n", remaining_charge_uah);

	
	calculate_cc_uah(chip, raw.cc, &cc_uah);
	pr_debug("cc_uah = %duAh raw->cc = %x cc = %lld after subtracting %d\n",
				cc_uah, raw.cc,
				(int64_t)raw.cc - chip->cc_reading_at_100,
				chip->cc_reading_at_100);
	chip->soc_rbatt_suspend = ((remaining_charge_uah - cc_uah) * 100)
						/ fcc_uah;
#endif

	dump_cc_uah();
	get_reg((void *)BMS_TOLERANCES, &val);
	pr_info("%s: BMS_TOLERANCES=0x%02llx\n", __func__, val);
	return 0;
}

#define DELTA_RBATT_PERCENT	10
static int pm8921_bms_resume(struct device *dev)
{
	u64 val;
#if 0 
	struct pm8921_rbatt_params raw;
	struct pm8921_bms_chip *chip = dev_get_drvdata(dev);
	int rbatt;
	int expected_rbatt;
	int scalefactor;
	int delta_rbatt;

	read_rbatt_params_raw(chip, &raw);
	rbatt = calculate_rbatt_resume(chip, &raw);

	if (rbatt < 0)
		return 0;

	expected_rbatt
		= (last_rbatt < 0) ? chip->default_rbatt_mohm : last_rbatt;

	if (chip->rbatt_sf_lut) {
		scalefactor = interpolate_scalingfactor(chip,
						chip->rbatt_sf_lut,
						chip->batt_temp_suspend / 10,
						chip->soc_rbatt_suspend);
		rbatt = rbatt * 100 / scalefactor;
	}

	delta_rbatt = expected_rbatt - rbatt;
	if (delta_rbatt)
		delta_rbatt = -delta_rbatt;
	if (delta_rbatt * 100 <= DELTA_RBATT_PERCENT * expected_rbatt)
		last_rbatt = rbatt;
#endif

	dump_cc_uah();
	get_reg((void *)BMS_TOLERANCES, &val);
	pr_info("%s: BMS_TOLERANCES=0x%02llx\n", __func__, val);
	return 0;
}

static int pm8921_bms_prepare(struct device *dev)
{
	unsigned long time_since_last_update_ms, cur_jiffies;
	struct timespec xtime;

	if (the_chip->criteria_sw_est_ocv <= 0)
		return 0;

	cur_jiffies = jiffies;
	time_since_last_update_ms =
		(cur_jiffies - htc_batt_bms_timer.batt_system_jiffies) * MSEC_PER_SEC / HZ;
	htc_batt_bms_timer.no_ocv_update_period_ms += time_since_last_update_ms;
	htc_batt_bms_timer.batt_system_jiffies = cur_jiffies;
	xtime = CURRENT_TIME;
	htc_batt_bms_timer.batt_suspend_ms = xtime.tv_sec * MSEC_PER_SEC +
		xtime.tv_nsec / NSEC_PER_MSEC;

	return 0;
}

static void pm8921_bms_complete(struct device *dev)
{
	struct timespec xtime;
	unsigned long resume_ms, sr_time_period_ms;

	if (the_chip->criteria_sw_est_ocv <= 0)
		return;

	xtime = CURRENT_TIME;
	htc_batt_bms_timer.batt_system_jiffies = jiffies;
	resume_ms = xtime.tv_sec * MSEC_PER_SEC + xtime.tv_nsec / NSEC_PER_MSEC;
	sr_time_period_ms = resume_ms - htc_batt_bms_timer.batt_suspend_ms;
	htc_batt_bms_timer.no_ocv_update_period_ms += sr_time_period_ms;

	if (htc_batt_bms_timer.no_ocv_update_period_ms > the_chip->criteria_sw_est_ocv
		&& !(!!ocv_update_stop_reason))
		pm8921_bms_estimate_ocv();
}

static const struct dev_pm_ops pm8921_bms_pm_ops = {
	.prepare = pm8921_bms_prepare,
	.complete = pm8921_bms_complete,
	.suspend	= pm8921_bms_suspend,
	.resume	= pm8921_bms_resume,
};

#define REG_SBI_CONFIG		0x04F
#define PAGE3_ENABLE_MASK	0x6
#define PROGRAM_REV_MASK	0x0F
#define PROGRAM_REV		0x9
static int read_ocv_trim(struct pm8921_bms_chip *chip)
{
	int rc;
	u8 reg, sbi_config;

	rc = pm8xxx_readb(chip->dev->parent, REG_SBI_CONFIG, &sbi_config);
	if (rc) {
		pr_err("error = %d reading sbi config reg\n", rc);
		return rc;
	}

	reg = sbi_config | PAGE3_ENABLE_MASK;
	rc = pm8xxx_writeb(chip->dev->parent, REG_SBI_CONFIG, reg);
	if (rc) {
		pr_err("error = %d writing sbi config reg\n", rc);
		return rc;
	}

	rc = pm8xxx_readb(chip->dev->parent, TEST_PROGRAM_REV, &reg);
	if (rc)
		pr_err("Error %d reading %d addr %d\n",
			rc, reg, TEST_PROGRAM_REV);
	pr_info("program rev reg is 0x%x\n", reg);
	reg &= PROGRAM_REV_MASK;

	
	if (reg >= PROGRAM_REV) {
		chip->amux_2_trim_delta = 0;
		goto restore_sbi_config;
	}

	rc = pm8xxx_readb(chip->dev->parent, AMUX_TRIM_2, &reg);
	if (rc) {
		pr_err("error = %d reading trim reg\n", rc);
		return rc;
	}

	chip->amux_2_trim_delta = abs(0x49 - reg);
	pr_info("trim reg=0x%x, trim delta=%d\n", reg, chip->amux_2_trim_delta);

restore_sbi_config:
	rc = pm8xxx_writeb(chip->dev->parent, REG_SBI_CONFIG, sbi_config);
	if (rc) {
		pr_err("error = %d writing sbi config reg\n", rc);
		return rc;
	}

	return 0;
}

static int __devinit pm8921_bms_probe(struct platform_device *pdev)
{
	int rc = 0;
	int vbatt, curr_soc;
	struct pm8921_bms_chip *chip;
	const struct pm8921_bms_platform_data *pdata
				= pdev->dev.platform_data;
	struct pm8921_soc_params raw;
	struct timespec xtime;
	unsigned long currtime_ms;
#ifdef CONFIG_HTC_BATT_8960
	const struct pm8921_charger_batt_param *chg_batt_param;
#endif

	pr_info("%s\n", __func__);

	if (!pdata) {
		pr_err("missing platform data\n");
		return -EINVAL;
	}

	xtime = CURRENT_TIME;
	currtime_ms = xtime.tv_sec * MSEC_PER_SEC + xtime.tv_nsec / NSEC_PER_MSEC;

	chip = kzalloc(sizeof(struct pm8921_bms_chip), GFP_KERNEL);
	if (!chip) {
		pr_err("Cannot allocate pm_bms_chip\n");
		return -ENOMEM;
	}
	mutex_init(&chip->bms_output_lock);
	mutex_init(&ocv_update_lock);
	spin_lock_init(&chip->bms_100_lock);
	chip->dev = &pdev->dev;
	chip->r_sense = pdata->r_sense;
	chip->i_test = pdata->i_test;
	chip->v_failure = pdata->v_failure;
	chip->rconn_mohm = pdata->rconn_mohm;
	chip->store_batt_data_soc_thre = pdata->store_batt_data_soc_thre;
	chip->criteria_sw_est_ocv = pdata->criteria_sw_est_ocv;
	chip->rconn_mohm_sw_est_ocv = pdata->rconn_mohm_sw_est_ocv;
	chip->cc_backup_uv = 0;
	chip->ocv_reading_at_100 = 0;
	chip->ocv_backup_uv = 0;
	chip->start_percent = -EINVAL;
	chip->end_percent = -EINVAL;
	chip->batt_temp_channel = pdata->bms_cdata.batt_temp_channel;
	chip->vbat_channel = pdata->bms_cdata.vbat_channel;
	chip->ref625mv_channel = pdata->bms_cdata.ref625mv_channel;
	chip->ref1p25v_channel = pdata->bms_cdata.ref1p25v_channel;
	chip->batt_id_channel = pdata->bms_cdata.batt_id_channel;
	chip->revision = pm8xxx_get_revision(chip->dev->parent);
	chip->version = pm8xxx_get_version(chip->dev->parent);
	INIT_WORK(&chip->calib_hkadc_work, calibrate_hkadc_work);
	htc_batt_bms_timer.batt_system_jiffies = jiffies;
	rc = set_battery_data(chip);
	if (rc) {
		pr_err("%s bad battery data %d\n", __func__, rc);
		goto free_chip;
	}
#ifdef CONFIG_HTC_BATT_8960
	
	chg_batt_param = htc_battery_cell_get_cur_cell_charger_cdata();
	if (!chg_batt_param) {
		chip->max_voltage_uv = pdata->max_voltage_uv;
	} else {
		chip->max_voltage_uv = chg_batt_param->max_voltage * 1000;
	}
#else
	chip->max_voltage_uv = pdata->max_voltage_uv;
#endif 

	if (chip->pc_temp_ocv_lut == NULL) {
		pr_err("temp ocv lut table is NULL\n");
		rc = -EINVAL;
		goto free_chip;
	}

	
	if (chip->default_rbatt_mohm <= 0)
		chip->default_rbatt_mohm = DEFAULT_RBATT_MOHMS;


	rc = request_irqs(chip, pdev);
	if (rc) {
		pr_err("couldn't register interrupts rc = %d\n", rc);
		goto free_chip;
	}

	rc = pm8921_bms_hw_init(chip);
	if (rc) {
		pr_err("couldn't init hardware rc = %d\n", rc);
		goto free_irqs;
	}

	platform_set_drvdata(pdev, chip);
	the_chip = chip;
	create_debugfs_entries(chip);

	rc = read_ocv_trim(chip);
	if (rc) {
		pr_err("couldn't adjust ocv_trim rc= %d\n", rc);
		goto free_irqs;
	}
	check_initial_ocv(chip);

	
	schedule_work(&chip->calib_hkadc_work);
	
	pm8921_bms_enable_irq(chip, PM8921_BMS_GOOD_OCV);
	pm8921_bms_enable_irq(chip, PM8921_BMS_OCV_FOR_R);

	get_battery_uvolts(chip, &vbatt);
	curr_soc = pm8921_bms_get_percent_charge();

	
	if (batt_stored_magic_num == BMS_STORE_MAGIC_NUM
			&& the_chip->store_batt_data_soc_thre > 0
			&& (curr_soc - batt_stored_soc) > 5
			&& (currtime_ms - batt_stored_time_ms) < 3600000 ) {
		read_soc_params_raw(the_chip, &raw);
		chip->cc_backup_uv = raw.cc - batt_stored_cc_uv;
		chip->ocv_backup_uv = last_ocv_uv = batt_stored_ocv_uv;
		chip->ocv_reading_at_100 = 0;
		write_backup_cc_uv(chip->cc_backup_uv);
		write_backup_ocv_at_100(chip->ocv_reading_at_100);
		write_backup_ocv_uv(chip->ocv_backup_uv);

		new_boot_soc = pm8921_bms_get_percent_charge();
		
		disable_ocv_update_with_reason(true, OCV_UPDATE_STOP_BIT_BOOT_UP);
	}
	
	pm8921_store_hw_reset_reason(0);

	pr_info("OK battery_capacity_at_boot=%d, new_boot_soc=%d, volt=%d, "
			"ocv=%d, batt_magic_num=%x, stored_soc=%d, curr_time=%ld, "
			"stored_time=%ld\n",
				curr_soc, new_boot_soc, vbatt, last_ocv_uv,
				batt_stored_magic_num, batt_stored_soc,
				currtime_ms, batt_stored_time_ms);
	pr_info("r_sense=%u,i_test=%u,v_failure=%u,default_rbatt_mohm=%d\n",
			chip->r_sense, chip->i_test, chip->v_failure,
			chip->default_rbatt_mohm);

	return 0;

free_irqs:
	free_irqs(chip);
free_chip:
	kfree(chip);
	return rc;
}

static int __devexit pm8921_bms_remove(struct platform_device *pdev)
{
	struct pm8921_bms_chip *chip = platform_get_drvdata(pdev);

	free_irqs(chip);
	kfree(chip->adjusted_fcc_temp_lut);
	platform_set_drvdata(pdev, NULL);
	the_chip = NULL;
	kfree(chip);
	return 0;
}

static struct platform_driver pm8921_bms_driver = {
	.probe	= pm8921_bms_probe,
	.remove	= __devexit_p(pm8921_bms_remove),
	.driver	= {
		.name	= PM8921_BMS_DEV_NAME,
		.owner	= THIS_MODULE,
		.pm = &pm8921_bms_pm_ops,
	},
};

static int __init pm8921_bms_init(void)
{
	flag_enable_bms_chg_log =
		(get_kernel_flag() & KERNEL_FLAG_ENABLE_BMS_CHARGER_LOG) ? 1 : 0;
	return platform_driver_register(&pm8921_bms_driver);
}

static void __exit pm8921_bms_exit(void)
{
	platform_driver_unregister(&pm8921_bms_driver);
}

late_initcall(pm8921_bms_init);
module_exit(pm8921_bms_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("PMIC8921 bms driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:" PM8921_BMS_DEV_NAME);
