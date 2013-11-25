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
#define pr_fmt(fmt)	"[BATT][CHG] " fmt
#define pr_fmt_debug(fmt)    "[BATT][CHG]%s: " fmt, __func__

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/mfd/pm8xxx/pm8921-charger.h>
#include <linux/mfd/pm8xxx/pm8921-bms.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <linux/mfd/pm8xxx/ccadc.h>
#include <linux/mfd/pm8xxx/core.h>
#include <linux/regulator/consumer.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/bitops.h>
#include <linux/workqueue.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/gpio.h>

#include <mach/msm_xo.h>
#ifdef CONFIG_USB_MSM_OTG_72K
#include <mach/msm_hsusb.h>
#else
#include <linux/usb/msm_hsusb.h>
#endif
#include <mach/board_htc.h>

#include <mach/htc_gauge.h>
#include <mach/htc_charger.h>
#include <mach/htc_battery_cell.h>

#include <mach/cable_detect.h>
#include <linux/i2c/smb349.h>

static int ext_usb_temp_condition_old;
struct delayed_work ext_usb_vbat_low_task;
struct delayed_work ext_usb_chgdone_task;
struct delayed_work ext_usb_temp_task;
struct delayed_work ext_usb_bms_notify_task;
struct workqueue_struct *ext_charger_wq;

#if defined(pr_debug)
#undef pr_debug
#endif
#define pr_debug(fmt, ...) do { \
		if (flag_enable_BMS_Charger_log) \
			printk(KERN_INFO pr_fmt_debug(fmt), ##__VA_ARGS__); \
	} while (0)

static bool flag_enable_BMS_Charger_log;
#define BATT_LOG_BUF_LEN (1024)

#define CHG_BUCK_CLOCK_CTRL	0x14

#define PBL_ACCESS1		0x04
#define PBL_ACCESS2		0x05
#define SYS_CONFIG_1		0x06
#define SYS_CONFIG_2		0x07
#define CHG_CNTRL		0x204
#define CHG_IBAT_MAX		0x205
#define CHG_TEST		0x206
#define CHG_BUCK_CTRL_TEST1	0x207
#define CHG_BUCK_CTRL_TEST2	0x208
#define CHG_BUCK_CTRL_TEST3	0x209
#define COMPARATOR_OVERRIDE	0x20A
#define PSI_TXRX_SAMPLE_DATA_0	0x20B
#define PSI_TXRX_SAMPLE_DATA_1	0x20C
#define PSI_TXRX_SAMPLE_DATA_2	0x20D
#define PSI_TXRX_SAMPLE_DATA_3	0x20E
#define PSI_CONFIG_STATUS	0x20F
#define CHG_IBAT_SAFE		0x210
#define CHG_ITRICKLE		0x211
#define CHG_CNTRL_2		0x212
#define CHG_VBAT_DET		0x213
#define CHG_VTRICKLE		0x214
#define CHG_ITERM		0x215
#define CHG_CNTRL_3		0x216
#define CHG_VIN_MIN		0x217
#define CHG_TWDOG		0x218
#define CHG_TTRKL_MAX		0x219
#define CHG_TEMP_THRESH		0x21A
#define CHG_TCHG_MAX		0x21B
#define USB_OVP_CONTROL		0x21C
#define DC_OVP_CONTROL		0x21D
#define USB_OVP_TEST		0x21E
#define DC_OVP_TEST		0x21F
#define CHG_VDD_MAX		0x220
#define CHG_VDD_SAFE		0x221
#define CHG_VBAT_BOOT_THRESH	0x222
#define USB_OVP_TRIM		0x355
#define BUCK_CONTROL_TRIM1	0x356
#define BUCK_CONTROL_TRIM2	0x357
#define BUCK_CONTROL_TRIM3	0x358
#define BUCK_CONTROL_TRIM4	0x359
#define CHG_DEFAULTS_TRIM	0x35A
#define CHG_ITRIM		0x35B
#define CHG_TTRIM		0x35C
#define CHG_COMP_OVR		0x20A

#define EOC_CHECK_PERIOD_MS	10000
#define UNPLUG_CHECK_WAIT_PERIOD_MS 200

#define USB_MA_2	(2)
#define USB_MA_500	(500)
#define USB_MA_900	(900)
#define USB_MA_1100	(1100)

#define SAFETY_TIME_MAX_LIMIT		510
#define SAFETY_TIME_8HR_TWICE		480

enum chg_fsm_state {
	FSM_STATE_OFF_0 = 0,
	FSM_STATE_BATFETDET_START_12 = 12,
	FSM_STATE_BATFETDET_END_16 = 16,
	FSM_STATE_ON_CHG_HIGHI_1 = 1,
	FSM_STATE_ATC_2A = 2,
	FSM_STATE_ATC_2B = 18,
	FSM_STATE_ON_BAT_3 = 3,
	FSM_STATE_ATC_FAIL_4 = 4 ,
	FSM_STATE_DELAY_5 = 5,
	FSM_STATE_ON_CHG_AND_BAT_6 = 6,
	FSM_STATE_FAST_CHG_7 = 7,
	FSM_STATE_TRKL_CHG_8 = 8,
	FSM_STATE_CHG_FAIL_9 = 9,
	FSM_STATE_EOC_10 = 10,
	FSM_STATE_ON_CHG_VREGOK_11 = 11,
	FSM_STATE_ATC_PAUSE_13 = 13,
	FSM_STATE_FAST_CHG_PAUSE_14 = 14,
	FSM_STATE_TRKL_CHG_PAUSE_15 = 15,
	FSM_STATE_START_BOOT = 20,
	FSM_STATE_FLCB_VREGOK = 21,
	FSM_STATE_FLCB = 22,
};

struct fsm_state_to_batt_status {
	enum chg_fsm_state	fsm_state;
	int			batt_state;
};

static struct fsm_state_to_batt_status map[] = {
	{FSM_STATE_OFF_0, POWER_SUPPLY_STATUS_UNKNOWN},
	{FSM_STATE_BATFETDET_START_12, POWER_SUPPLY_STATUS_UNKNOWN},
	{FSM_STATE_BATFETDET_END_16, POWER_SUPPLY_STATUS_UNKNOWN},
	{FSM_STATE_ON_CHG_HIGHI_1, POWER_SUPPLY_STATUS_FULL},
	{FSM_STATE_ATC_2A, POWER_SUPPLY_STATUS_CHARGING},
	{FSM_STATE_ATC_2B, POWER_SUPPLY_STATUS_CHARGING},
	{FSM_STATE_ON_BAT_3, POWER_SUPPLY_STATUS_DISCHARGING},
	{FSM_STATE_ATC_FAIL_4, POWER_SUPPLY_STATUS_DISCHARGING},
	{FSM_STATE_DELAY_5, POWER_SUPPLY_STATUS_UNKNOWN },
	{FSM_STATE_ON_CHG_AND_BAT_6, POWER_SUPPLY_STATUS_CHARGING},
	{FSM_STATE_FAST_CHG_7, POWER_SUPPLY_STATUS_CHARGING},
	{FSM_STATE_TRKL_CHG_8, POWER_SUPPLY_STATUS_CHARGING},
	{FSM_STATE_CHG_FAIL_9, POWER_SUPPLY_STATUS_DISCHARGING},
	{FSM_STATE_EOC_10, POWER_SUPPLY_STATUS_FULL},
	{FSM_STATE_ON_CHG_VREGOK_11, POWER_SUPPLY_STATUS_NOT_CHARGING},
	{FSM_STATE_ATC_PAUSE_13, POWER_SUPPLY_STATUS_NOT_CHARGING},
	{FSM_STATE_FAST_CHG_PAUSE_14, POWER_SUPPLY_STATUS_NOT_CHARGING},
	{FSM_STATE_TRKL_CHG_PAUSE_15, POWER_SUPPLY_STATUS_NOT_CHARGING},
	{FSM_STATE_START_BOOT, POWER_SUPPLY_STATUS_NOT_CHARGING},
	{FSM_STATE_FLCB_VREGOK, POWER_SUPPLY_STATUS_NOT_CHARGING},
	{FSM_STATE_FLCB, POWER_SUPPLY_STATUS_NOT_CHARGING},
};

enum chg_regulation_loop {
	VDD_LOOP = BIT(3),
	BAT_CURRENT_LOOP = BIT(2),
	INPUT_CURRENT_LOOP = BIT(1),
	INPUT_VOLTAGE_LOOP = BIT(0),
	CHG_ALL_LOOPS = VDD_LOOP | BAT_CURRENT_LOOP
			| INPUT_CURRENT_LOOP | INPUT_VOLTAGE_LOOP,
};

enum pmic_chg_interrupts {
	USBIN_VALID_IRQ = 0,
	USBIN_OV_IRQ,
	BATT_INSERTED_IRQ,
	VBATDET_LOW_IRQ,
	USBIN_UV_IRQ,
	VBAT_OV_IRQ,
	CHGWDOG_IRQ,
	VCP_IRQ,
	ATCDONE_IRQ,
	ATCFAIL_IRQ,
	CHGDONE_IRQ,
	CHGFAIL_IRQ,
	CHGSTATE_IRQ,
	LOOP_CHANGE_IRQ,
	FASTCHG_IRQ,
	TRKLCHG_IRQ,
	BATT_REMOVED_IRQ,
	BATTTEMP_HOT_IRQ,
	CHGHOT_IRQ,
	BATTTEMP_COLD_IRQ,
	CHG_GONE_IRQ,
	BAT_TEMP_OK_IRQ,
	COARSE_DET_LOW_IRQ,
	VDD_LOOP_IRQ,
	VREG_OV_IRQ,
	VBATDET_IRQ,
	BATFET_IRQ,
	PSI_IRQ,
	DCIN_VALID_IRQ,
	DCIN_OV_IRQ,
	DCIN_UV_IRQ,
	PM_CHG_MAX_INTS,
};

struct bms_notify {
	int			is_battery_full;
	int			is_charging;
	struct	work_struct	work;
};

struct pm8921_chg_chip {
	struct device			*dev;
	unsigned int			usb_present;
	unsigned int			dc_present;
	unsigned int			usb_charger_current;
	unsigned int			max_bat_chg_current;
	unsigned int			pmic_chg_irq[PM_CHG_MAX_INTS];
	unsigned int			safety_time;
	unsigned int			ttrkl_time;
	unsigned int			update_time;
	unsigned int			max_voltage_mv;
	unsigned int			min_voltage_mv;
	int						cool_temp_dc;
	int						warm_temp_dc;
	unsigned int			temp_check_period;
	unsigned int			cool_bat_chg_current;
	unsigned int			warm_bat_chg_current;
	unsigned int			cool_bat_voltage;
	unsigned int			warm_bat_voltage;
	unsigned int			is_bat_cool;
	unsigned int			is_bat_warm;
	unsigned int			resume_voltage_delta;
	unsigned int			term_current;
	unsigned int			vbat_channel;
	unsigned int			batt_temp_channel;
	unsigned int			batt_id_channel;
	struct dentry			*dent;
	struct bms_notify		bms_notify;
	struct regulator		*vreg_xoadc;
	struct ext_chg_pm8921		*ext;		
	struct ext_usb_chg_pm8921	*ext_usb;	
	bool				keep_btm_on_suspend;
	bool				ext_charging;
	bool				ext_charge_done;
	bool				ext_usb_charging;	
	bool				ext_usb_charge_done;	
	bool				dc_unplug_check;
	bool				disable_reverse_boost_check;
	bool				final_kickstart;
	bool				lockup_lpm_wrkarnd;
	DECLARE_BITMAP(enabled_irqs, PM_CHG_MAX_INTS);
	struct work_struct		battery_id_valid_work;
	int64_t				batt_id_min;
	int64_t				batt_id_max;
	int				trkl_voltage;
	int				weak_voltage;
	int				trkl_current;
	int				weak_current;
	int				vin_min;
	int				vin_min_wlc;
	unsigned int			*thermal_mitigation;
	int				thermal_levels;
	int				mbat_in_gpio;
	int				wlc_tx_gpio;
	int				cable_in_irq;
	int				cable_in_gpio;
	int				is_embeded_batt;
	struct delayed_work		update_heartbeat_work;
	struct delayed_work		eoc_work;
	struct delayed_work		ovp_check_work;
	struct delayed_work		recharge_check_work;
	struct work_struct		unplug_ovp_fet_open_work;
	struct work_struct		chghot_work;
	struct delayed_work		unplug_check_work;
	struct delayed_work		vin_collapse_check_work;
	struct wake_lock		unplug_ovp_fet_open_wake_lock;
	struct wake_lock		eoc_wake_lock;
	struct wake_lock		recharge_check_wake_lock;
	enum pm8921_chg_cold_thr	cold_thr;
	enum pm8921_chg_hot_thr		hot_thr;
	int				rconn_mohm;
	u8				active_path;
};

static bool test_power_monitor = 0;
static bool flag_keep_charge_on;
static bool flag_pa_recharge;
static bool flag_disable_wakelock;
static bool is_batt_full = false;
static bool is_ac_safety_timeout = false;
static bool is_ac_safety_timeout_twice = false; 
static bool is_cable_remove = false;
static bool is_batt_full_eoc_stop = false;

static int usbin_ov_irq_state = 0;
static int usbin_uv_irq_state = 0;
static int ovp = 0;
static int uvp = 0;

enum htc_power_source_type pwr_src;
static unsigned int chg_limit_current; 
static unsigned int usb_max_current;
static int hsml_target_ma;
static int usb_target_ma;
static int charging_disabled; 
static int auto_enable;
static int thermal_mitigation;
static int usb_ovp_disable;
static int bat_temp_ok_prev = -1;
static int eoc_count; 

static int usbin_critical_low_cnt = 0;
static int pwrsrc_under_rating = 0;

static struct pm8921_chg_chip *the_chip;

static struct pm8xxx_adc_arb_btm_param btm_config;
static char batt_log_buf[BATT_LOG_BUF_LEN];

static int get_reg(void *data, u64 *val);
static int get_reg_loop(void *data, u64 * val);
static void dump_reg(void);
static void dump_all(int more);
static void update_ovp_uvp_state(int ov, int v, int uv);
static irqreturn_t usbin_ov_irq_handler(int irq, void *data);

static DEFINE_SPINLOCK(lpm_lock);
#define LPM_ENABLE_BIT	BIT(2)
static int pm8921_chg_set_lpm(struct pm8921_chg_chip *chip, int enable)
{
	int rc;
	u8 reg;

	rc = pm8xxx_readb(chip->dev->parent, CHG_CNTRL, &reg);
	if (rc) {
		pr_err("pm8xxx_readb failed: addr=%03X, rc=%d\n",
				CHG_CNTRL, rc);
		return rc;
	}
	reg &= ~LPM_ENABLE_BIT;
	reg |= (enable ? LPM_ENABLE_BIT : 0);

	rc = pm8xxx_writeb(chip->dev->parent, CHG_CNTRL, reg);
	if (rc) {
		pr_err("pm_chg_write failed: addr=%03X, rc=%d\n",
				CHG_CNTRL, rc);
		return rc;
	}

	return rc;
}

#define VDD_LOOP_ACTIVE_BIT	BIT(3)
#define VDD_MAX_INCREASE_MV	20
static int vdd_max_increase_mv = VDD_MAX_INCREASE_MV;
module_param(vdd_max_increase_mv, int, 0644);

static int ichg_threshold_ua = -200000;
module_param(ichg_threshold_ua, int, 0644);

static int delta_threshold_mv = -5; 
module_param(delta_threshold_mv, int, 0644);
static int last_delta_mv = 0;

static int pm_chg_write(struct pm8921_chg_chip *chip, u16 addr, u8 reg)
{
	int rc;
	unsigned long flags = 0;
	u8 temp;

	
	if (chip->lockup_lpm_wrkarnd) {
		spin_lock_irqsave(&lpm_lock, flags);

		udelay(200);
		
		temp = 0xD1;
		rc = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
		if (rc) {
			pr_err("Error %d writing %d to CHG_TEST\n", rc, temp);
			goto release_lpm_lock;
		}

		
		temp = 0xD3;
		rc = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
		if (rc) {
			pr_err("Error %d writing %d to CHG_TEST\n", rc, temp);
			goto release_lpm_lock;
		}

		rc = pm8xxx_writeb(chip->dev->parent, addr, reg);
		if (rc) {
			pr_err("failed: addr=%03X, rc=%d\n", addr, rc);
			goto release_lpm_lock;
		}

		
		temp = 0xD1;
		rc = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
		if (rc) {
			pr_err("Error %d writing %d to CHG_TEST\n", rc, temp);
			goto release_lpm_lock;
		}

		
		temp = 0xD0;
		rc = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
		if (rc) {
			pr_err("Error %d writing %d to CHG_TEST\n", rc, temp);
			goto release_lpm_lock;
		}

		udelay(200);

release_lpm_lock:
		spin_unlock_irqrestore(&lpm_lock, flags);
	} else {
		rc = pm8xxx_writeb(chip->dev->parent, addr, reg);
		if (rc)
			pr_err("failed: addr=%03X, rc=%d\n", addr, rc);
	}
	return rc;
}

static int pm_chg_masked_write(struct pm8921_chg_chip *chip, u16 addr,
							u8 mask, u8 val)
{
	int rc;
	u8 reg;

	rc = pm8xxx_readb(chip->dev->parent, addr, &reg);
	if (rc) {
		pr_err("pm8xxx_readb failed: addr=%03X, rc=%d\n", addr, rc);
		return rc;
	}
	reg &= ~mask;
	reg |= val & mask;
	rc = pm_chg_write(chip, addr, reg);
	if (rc) {
		pr_err("pm_chg_write failed: addr=%03X, rc=%d\n", addr, rc);
		return rc;
	}
	return 0;
}

static int pm_chg_get_rt_status(struct pm8921_chg_chip *chip, int irq_id)
{
	return pm8xxx_read_irq_stat(chip->dev->parent,
					chip->pmic_chg_irq[irq_id]);
}

static int is_chg_on_bat(struct pm8921_chg_chip *chip)
{
	return !(pm_chg_get_rt_status(chip, DCIN_VALID_IRQ)
			|| pm_chg_get_rt_status(chip, USBIN_VALID_IRQ));
}

static void pm8921_chg_bypass_bat_gone_debounce(struct pm8921_chg_chip *chip,
		int bypass)
{
	int rc;

	rc = pm_chg_write(chip, COMPARATOR_OVERRIDE, bypass ? 0x89 : 0x88);
	if (rc) {
		pr_err("Failed to set bypass bit to %d rc=%d\n", bypass, rc);
	}
}

static int is_usb_chg_plugged_in(struct pm8921_chg_chip *chip)
{
	return pm_chg_get_rt_status(chip, USBIN_VALID_IRQ);
}

static int is_dc_chg_plugged_in(struct pm8921_chg_chip *chip)
{
	return pm_chg_get_rt_status(chip, DCIN_VALID_IRQ);
}

#define CAPTURE_FSM_STATE_CMD	0xC2
#define READ_BANK_7		0x70
#define READ_BANK_4		0x40
static int pm_chg_get_fsm_state(struct pm8921_chg_chip *chip)
{
	u8 temp;
	unsigned long flags = 0;
	int err = 0, ret = 0;

	if (chip->lockup_lpm_wrkarnd) {
		spin_lock_irqsave(&lpm_lock, flags);

		udelay(200);
		
		temp = 0xD1;
		err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
		if (err) {
			pr_err("Error %d writing %d to CHG_TEST\n", err, temp);
			goto err_out;
		}

		
		temp = 0xD3;
		err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
		if (err) {
			pr_err("Error %d writing %d to CHG_TEST\n", err, temp);
			goto err_out;
		}
	}

	temp = CAPTURE_FSM_STATE_CMD;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		goto err_out;
	}

	temp = READ_BANK_7;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		goto err_out;
	}

	err = pm8xxx_readb(chip->dev->parent, CHG_TEST, &temp);
	if (err) {
		pr_err("pm8xxx_readb fail: addr=%03X, rc=%d\n", CHG_TEST, err);
		goto err_out;
	}
	
	ret = temp & 0xF;

	temp = READ_BANK_4;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		goto err_out;
	}

	err = pm8xxx_readb(chip->dev->parent, CHG_TEST, &temp);
	if (err) {
		pr_err("pm8xxx_readb fail: addr=%03X, rc=%d\n", CHG_TEST, err);
		goto err_out;
	}
	
	ret |= (temp & 0x1) << 4;


	if (chip->lockup_lpm_wrkarnd) {
		
		temp = 0xD1;
		err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
		if (err) {
			pr_err("Error %d writing %d to CHG_TEST\n", err, temp);
			goto err_out;
		}

		
		temp = 0xD0;
		err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
		if (err) {
			pr_err("Error %d writing %d to CHG_TEST\n", err, temp);
			goto err_out;
		}

		udelay(200);
	}

err_out:
	if (chip->lockup_lpm_wrkarnd) {
		spin_unlock_irqrestore(&lpm_lock, flags);
	}
	if (err)
		return err;

	return  ret;
}

#define READ_BANK_6		0x60
static int pm_chg_get_regulation_loop(struct pm8921_chg_chip *chip)
{
	u8 temp, data;
	unsigned long flags = 0;
	int err = 0;

	if (chip->lockup_lpm_wrkarnd) {
		spin_lock_irqsave(&lpm_lock, flags);

		udelay(200);
		
		temp = 0xD1;
		err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
		if (err) {
			pr_err("Error %d writing %d to CHG_TEST\n", err, temp);
			goto err_out;
		}

		
		temp = 0xD3;
		err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
		if (err) {
			pr_err("Error %d writing %d to CHG_TEST\n", err, temp);
			goto err_out;
		}
	}

	temp = READ_BANK_6;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		goto err_out;
	}

	err = pm8xxx_readb(chip->dev->parent, CHG_TEST, &data);
	if (err) {
		pr_err("pm8xxx_readb fail: addr=%03X, rc=%d\n", CHG_TEST, err);
		goto err_out;
	}

	if (chip->lockup_lpm_wrkarnd) {
		
		temp = 0xD1;
		err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
		if (err) {
			pr_err("Error %d writing %d to CHG_TEST\n", err, temp);
			goto err_out;
		}

		
		temp = 0xD0;
		err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
		if (err) {
			pr_err("Error %d writing %d to CHG_TEST\n", err, temp);
			goto err_out;
		}

		udelay(200);
	}

err_out:
	if (chip->lockup_lpm_wrkarnd) {
		spin_unlock_irqrestore(&lpm_lock, flags);
	}
	if (err)
		return err;

	
	return data & CHG_ALL_LOOPS;
}

#define CHG_USB_SUSPEND_BIT  BIT(2)
static int pm_chg_usb_suspend_enable(struct pm8921_chg_chip *chip, int enable)
{
	return pm_chg_masked_write(chip, CHG_CNTRL_3, CHG_USB_SUSPEND_BIT,
			enable ? CHG_USB_SUSPEND_BIT : 0);
}

#define CHG_EN_BIT	BIT(7)
static int pm_chg_auto_enable(struct pm8921_chg_chip *chip, int enable)
{
	return pm_chg_masked_write(chip, CHG_CNTRL_3, CHG_EN_BIT,
				enable ? CHG_EN_BIT : 0);
}

#define CHG_FAILED_CLEAR	BIT(0)
#define ATC_FAILED_CLEAR	BIT(1)
static int pm_chg_failed_clear(struct pm8921_chg_chip *chip, int clear)
{
	int rc;

	rc = pm_chg_masked_write(chip, CHG_CNTRL_3, ATC_FAILED_CLEAR,
				clear ? ATC_FAILED_CLEAR : 0);
	rc |= pm_chg_masked_write(chip, CHG_CNTRL_3, CHG_FAILED_CLEAR,
				clear ? CHG_FAILED_CLEAR : 0);
	return rc;
}

#define CHG_CHARGE_DIS_BIT	BIT(1)
static int pm_chg_charge_dis(struct pm8921_chg_chip *chip, int disable)
{
	return pm_chg_masked_write(chip, CHG_CNTRL, CHG_CHARGE_DIS_BIT,
				disable ? CHG_CHARGE_DIS_BIT : 0);
}

static DEFINE_SPINLOCK(pwrsrc_lock);
#define PWRSRC_DISABLED_BIT_KDRV	(1)
#define PWRSRC_DISABLED_BIT_USER	(1<<1)
static int pwrsrc_disabled; 
static int pm_chg_disable_pwrsrc(struct pm8921_chg_chip *chip,
		int disable, int reason)
{
	int rc;
	unsigned long flags;

	spin_lock_irqsave(&pwrsrc_lock, flags);
	if (disable)
		pwrsrc_disabled |= reason;	
	else
		pwrsrc_disabled &= ~reason;	
	rc = pm_chg_charge_dis(chip, pwrsrc_disabled);
	if (rc)
		pr_err("Failed rc=%d\n", rc);
	spin_unlock_irqrestore(&pwrsrc_lock, flags);

	return rc;
}

static DEFINE_SPINLOCK(charger_lock);
static int batt_charging_disabled; 
#define BATT_CHG_DISABLED_BIT_EOC	(1)
#define BATT_CHG_DISABLED_BIT_KDRV	(1<<1)
#define BATT_CHG_DISABLED_BIT_USR1	(1<<2)
#define BATT_CHG_DISABLED_BIT_USR2	(1<<3)
#define BATT_CHG_DISABLED_BIT_BAT	(1<<4)
static int pm_chg_disable_auto_enable(struct pm8921_chg_chip *chip,
		int disable, int reason)
{
	int rc;
	unsigned long flags;

	spin_lock_irqsave(&charger_lock, flags);
	if (disable)
		batt_charging_disabled |= reason;	
	else
		batt_charging_disabled &= ~reason;	
	rc = pm_chg_auto_enable(the_chip, !batt_charging_disabled);
	if (rc)
		pr_err("Failed rc=%d\n", rc);
	spin_unlock_irqrestore(&charger_lock, flags);

	return rc;
}

#define PM8921_CHG_V_MIN_MV	3240
#define PM8921_CHG_V_STEP_MV	20
#define PM8921_CHG_V_STEP_10MV_OFFSET_BIT	BIT(7)
#define PM8921_CHG_VDDMAX_MAX	4500
#define PM8921_CHG_VDDMAX_MIN	3400
#define PM8921_CHG_V_MASK	0x7F
static int __pm_chg_vddmax_set(struct pm8921_chg_chip *chip, int voltage)
{
	int remainder;
	u8 temp = 0;

	if (voltage < PM8921_CHG_VDDMAX_MIN
			|| voltage > PM8921_CHG_VDDMAX_MAX) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}

	temp = (voltage - PM8921_CHG_V_MIN_MV) / PM8921_CHG_V_STEP_MV;

	remainder = voltage % 20;
	if (remainder >= 10) {
		temp |= PM8921_CHG_V_STEP_10MV_OFFSET_BIT;
	}

	pr_debug("voltage=%d setting %02x\n", voltage, temp);
	return pm_chg_write(chip, CHG_VDD_MAX, temp);
}

static int pm_chg_vddmax_get(struct pm8921_chg_chip *chip, int *voltage)
{
	u8 temp;
	int rc;

	rc = pm8xxx_readb(chip->dev->parent, CHG_VDD_MAX, &temp);
	if (rc) {
		pr_err("rc = %d while reading vdd max\n", rc);
		*voltage = 0;
		return rc;
	}
	*voltage = (int)(temp & PM8921_CHG_V_MASK) * PM8921_CHG_V_STEP_MV
							+ PM8921_CHG_V_MIN_MV;
	if (temp & PM8921_CHG_V_STEP_10MV_OFFSET_BIT)
		*voltage =  *voltage + 10;
	return 0;
}

static int pm_chg_vddmax_set(struct pm8921_chg_chip *chip, int voltage)
{
	int current_mv, ret, steps, i;
	bool increase;

	ret = 0;

	if (voltage < PM8921_CHG_VDDMAX_MIN
		|| voltage > PM8921_CHG_VDDMAX_MAX) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}

	ret = pm_chg_vddmax_get(chip, &current_mv);
	if (ret) {
		pr_err("Failed to read vddmax rc=%d\n", ret);
		return -EINVAL;
	}
	if (current_mv == voltage)
		return 0;

	
	if (is_usb_chg_plugged_in(chip)) {
		if (current_mv < voltage) {
			steps = (voltage - current_mv) / PM8921_CHG_V_STEP_MV;
			increase = true;
		} else {
			steps = (current_mv - voltage) / PM8921_CHG_V_STEP_MV;
			increase = false;
		}
		for (i = 0; i < steps; i++) {
			if (increase)
				current_mv += PM8921_CHG_V_STEP_MV;
			else
				current_mv -= PM8921_CHG_V_STEP_MV;
			ret |= __pm_chg_vddmax_set(chip, current_mv);
		}
	}
	ret |= __pm_chg_vddmax_set(chip, voltage);
	return ret;
}

#define PM8921_CHG_VDDSAFE_MIN	3400
#define PM8921_CHG_VDDSAFE_MAX	4500
static int pm_chg_vddsafe_set(struct pm8921_chg_chip *chip, int voltage)
{
	u8 temp;

	if (voltage < PM8921_CHG_VDDSAFE_MIN
			|| voltage > PM8921_CHG_VDDSAFE_MAX) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}
	temp = (voltage - PM8921_CHG_V_MIN_MV) / PM8921_CHG_V_STEP_MV;
	pr_debug("voltage=%d setting %02x\n", voltage, temp);
	return pm_chg_masked_write(chip, CHG_VDD_SAFE, PM8921_CHG_V_MASK, temp);
}

#define PM8921_CHG_VBATDET_MIN	3240
#define PM8921_CHG_VBATDET_MAX	5780
static int pm_chg_vbatdet_set(struct pm8921_chg_chip *chip, int voltage)
{
	u8 temp;

	if (voltage < PM8921_CHG_VBATDET_MIN
			|| voltage > PM8921_CHG_VBATDET_MAX) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}
	temp = (voltage - PM8921_CHG_V_MIN_MV) / PM8921_CHG_V_STEP_MV;
	pr_debug("voltage=%d setting %02x\n", voltage, temp);
	return pm_chg_masked_write(chip, CHG_VBAT_DET, PM8921_CHG_V_MASK, temp);
}



static int set_appropriate_vbatdet(struct pm8921_chg_chip *chip)
{
	int rc = 0;
	int vbat = 0;

	if (chip->is_bat_cool)
		vbat = chip->cool_bat_voltage - chip->resume_voltage_delta;
	else if (chip->is_bat_warm)
		vbat = chip->warm_bat_voltage - chip->resume_voltage_delta;
	else if (is_batt_full_eoc_stop)
		vbat = chip->max_voltage_mv- chip->resume_voltage_delta;
	else 
		vbat = PM8921_CHG_VBATDET_MAX;

	rc = pm_chg_vbatdet_set(chip, vbat);

	if (rc)
		pr_err("Failed to set vbatdet=%d rc=%d\n", vbat, rc);
	else
		pr_info("%s, vbatdet=%d, is_bat_cool=%d, is_bat_warm=%d\n", __func__, vbat, chip->is_bat_cool, chip->is_bat_warm);

	return rc;
}


#define PM8921_CHG_VINMIN_MIN_MV	3800
#define PM8921_CHG_VINMIN_STEP_MV	100
#define PM8921_CHG_VINMIN_USABLE_MAX	6500
#define PM8921_CHG_VINMIN_USABLE_MIN	4300
#define PM8921_CHG_VINMIN_MASK		0x1F
static int pm_chg_vinmin_set(struct pm8921_chg_chip *chip, int voltage)
{
	u8 temp;

	if (voltage < PM8921_CHG_VINMIN_USABLE_MIN
			|| voltage > PM8921_CHG_VINMIN_USABLE_MAX) {
		pr_err("bad mV=%d asked to set\n", voltage);
		return -EINVAL;
	}
	temp = (voltage - PM8921_CHG_VINMIN_MIN_MV) / PM8921_CHG_VINMIN_STEP_MV;
	pr_debug("voltage=%d setting %02x\n", voltage, temp);
	return pm_chg_masked_write(chip, CHG_VIN_MIN, PM8921_CHG_VINMIN_MASK,
									temp);
}

static int pm_chg_vinmin_get(struct pm8921_chg_chip *chip)
{
	u8 temp;
	int rc, voltage_mv;

	rc = pm8xxx_readb(chip->dev->parent, CHG_VIN_MIN, &temp);
	temp &= PM8921_CHG_VINMIN_MASK;

	voltage_mv = PM8921_CHG_VINMIN_MIN_MV +
			(int)temp * PM8921_CHG_VINMIN_STEP_MV;

	return voltage_mv;
}

#define PM8921_CHG_IBATMAX_MIN	325
#define PM8921_CHG_IBATMAX_MAX	2000
#define PM8921_CHG_I_MIN_MA	225
#define PM8921_CHG_I_STEP_MA	50
#define PM8921_CHG_I_MASK	0x3F
static int pm_chg_ibatmax_set(struct pm8921_chg_chip *chip, int chg_current)
{
	u8 temp;

	if (chg_current < PM8921_CHG_I_MIN_MA
			|| chg_current > PM8921_CHG_IBATMAX_MAX) {
		pr_err("bad mA=%d asked to set\n", chg_current);
		return -EINVAL;
	}
	temp = (chg_current - PM8921_CHG_I_MIN_MA) / PM8921_CHG_I_STEP_MA;
	return pm_chg_masked_write(chip, CHG_IBAT_MAX, PM8921_CHG_I_MASK, temp);
}

#define PM8921_CHG_IBATSAFE_MIN	225
#define PM8921_CHG_IBATSAFE_MAX	3375
static int pm_chg_ibatsafe_set(struct pm8921_chg_chip *chip, int chg_current)
{
	u8 temp;

	if (chg_current < PM8921_CHG_IBATSAFE_MIN
			|| chg_current > PM8921_CHG_IBATSAFE_MAX) {
		pr_err("bad mA=%d asked to set\n", chg_current);
		return -EINVAL;
	}
	temp = (chg_current - PM8921_CHG_I_MIN_MA) / PM8921_CHG_I_STEP_MA;
	return pm_chg_masked_write(chip, CHG_IBAT_SAFE,
						PM8921_CHG_I_MASK, temp);
}

#define PM8921_CHG_ITERM_MIN_MA		50
#define PM8921_CHG_ITERM_MAX_MA		200
#define PM8921_CHG_ITERM_STEP_MA	10
#define PM8921_CHG_ITERM_MASK		0xF
static int pm_chg_iterm_set(struct pm8921_chg_chip *chip, int chg_current)
{
	u8 temp;

	if (chg_current < PM8921_CHG_ITERM_MIN_MA
			|| chg_current > PM8921_CHG_ITERM_MAX_MA) {
		pr_err("bad mA=%d asked to set\n", chg_current);
		return -EINVAL;
	}

	temp = (chg_current - PM8921_CHG_ITERM_MIN_MA)
				/ PM8921_CHG_ITERM_STEP_MA;
	return pm_chg_masked_write(chip, CHG_ITERM, PM8921_CHG_ITERM_MASK,
					 temp);
}

static int pm_chg_iterm_get(struct pm8921_chg_chip *chip, int *chg_current)
{
	u8 temp;
	int rc;

	rc = pm8xxx_readb(chip->dev->parent, CHG_ITERM, &temp);
	if (rc) {
		pr_err("err=%d reading CHG_ITEM\n", rc);
		*chg_current = 0;
		return rc;
	}
	temp &= PM8921_CHG_ITERM_MASK;
	*chg_current = (int)temp * PM8921_CHG_ITERM_STEP_MA
					+ PM8921_CHG_ITERM_MIN_MA;
	return 0;
}

struct usb_ma_limit_entry {
	int usb_ma;
};

static struct usb_ma_limit_entry usb_ma_table[] = {
	{100},
	{500},
	{700},
	{850},
	{900},
	{1100},
	{1300},
	{1500},
};

#if 0
static int get_proper_dc_input_curr_limit_via_hsml(void)
{
	int i = 0;
	int target_ma = 0;
	int masize = ARRAY_SIZE(usb_ma_table);

	if(hsml_target_ma == 0) {
		return 0;
	} else {
		for(i=0; i < masize; i++)
		{
			if(hsml_target_ma > usb_ma_table[i]) continue;
			else break;
		}

		if(i == 0)
			target_ma = usb_ma_table[0];
		else if(i == masize)
			target_ma = usb_ma_table[masize -1];
		else
			target_ma = usb_ma_table[i - 1];

		pr_info("%s, new target_ma: %dmA\n", __func__, target_ma);
		return target_ma;
	}
}
#endif


#define PM8921_CHG_IUSB_MASK 0x1C
#define PM8921_CHG_IUSB_MAX  7
#define PM8921_CHG_IUSB_MIN  0
static int pm_chg_iusbmax_set(struct pm8921_chg_chip *chip, int reg_val)
{
	u8 temp;

	if (reg_val < PM8921_CHG_IUSB_MIN || reg_val > PM8921_CHG_IUSB_MAX) {
		pr_err("bad mA=%d asked to set\n", reg_val);
		return -EINVAL;
	}
	temp = reg_val << 2;
	return pm_chg_masked_write(chip, PBL_ACCESS2, PM8921_CHG_IUSB_MASK,
					 temp);
}

static int pm_chg_iusbmax_get(struct pm8921_chg_chip *chip, int *mA)
{
	u8 temp;
	int rc;

	*mA = 0;
	rc = pm8xxx_readb(chip->dev->parent, PBL_ACCESS2, &temp);
	if (rc) {
		pr_err("err=%d reading PBL_ACCESS2\n", rc);
		return rc;
	}
	temp &= PM8921_CHG_IUSB_MASK;
	temp = temp >> 2;
	*mA = usb_ma_table[temp].usb_ma;
	return rc;
}

#define PM8921_CHG_WD_MASK 0x1F
static int pm_chg_disable_wd(struct pm8921_chg_chip *chip)
{
	
	return pm_chg_masked_write(chip, CHG_TWDOG, PM8921_CHG_WD_MASK, 0);
}

#define PM8921_CHG_TCHG_MASK	0x7F
#define PM8921_CHG_TCHG_MIN	4
#define PM8921_CHG_TCHG_MAX	512
#define PM8921_CHG_TCHG_STEP	4
static int pm_chg_tchg_max_set(struct pm8921_chg_chip *chip, int minutes)
{
	u8 temp;

	if (minutes < PM8921_CHG_TCHG_MIN || minutes > PM8921_CHG_TCHG_MAX) {
		pr_err("bad max minutes =%d asked to set\n", minutes);
		return -EINVAL;
	}

	temp = (minutes - 1)/PM8921_CHG_TCHG_STEP;
	return pm_chg_masked_write(chip, CHG_TCHG_MAX, PM8921_CHG_TCHG_MASK,
					 temp);
}

#define PM8921_CHG_TTRKL_MASK	0x3F
#define PM8921_CHG_TTRKL_MIN	1
#define PM8921_CHG_TTRKL_MAX	64
static int pm_chg_ttrkl_max_set(struct pm8921_chg_chip *chip, int minutes)
{
	u8 temp;

	if (minutes < PM8921_CHG_TTRKL_MIN || minutes > PM8921_CHG_TTRKL_MAX) {
		pr_err("bad max minutes =%d asked to set\n", minutes);
		return -EINVAL;
	}

	temp = minutes - 1;
	return pm_chg_masked_write(chip, CHG_TTRKL_MAX, PM8921_CHG_TTRKL_MASK,
					 temp);
}

#define PM8921_CHG_VTRKL_MIN_MV		2050
#define PM8921_CHG_VTRKL_MAX_MV		2800
#define PM8921_CHG_VTRKL_STEP_MV	50
#define PM8921_CHG_VTRKL_SHIFT		4
#define PM8921_CHG_VTRKL_MASK		0xF0
static int pm_chg_vtrkl_low_set(struct pm8921_chg_chip *chip, int millivolts)
{
	u8 temp;

	if (millivolts < PM8921_CHG_VTRKL_MIN_MV
			|| millivolts > PM8921_CHG_VTRKL_MAX_MV) {
		pr_err("bad voltage = %dmV asked to set\n", millivolts);
		return -EINVAL;
	}

	temp = (millivolts - PM8921_CHG_VTRKL_MIN_MV)/PM8921_CHG_VTRKL_STEP_MV;
	temp = temp << PM8921_CHG_VTRKL_SHIFT;
	return pm_chg_masked_write(chip, CHG_VTRICKLE, PM8921_CHG_VTRKL_MASK,
					 temp);
}

#define PM8921_CHG_VWEAK_MIN_MV		2100
#define PM8921_CHG_VWEAK_MAX_MV		3600
#define PM8921_CHG_VWEAK_STEP_MV	100
#define PM8921_CHG_VWEAK_MASK		0x0F
static int pm_chg_vweak_set(struct pm8921_chg_chip *chip, int millivolts)
{
	u8 temp;

	if (millivolts < PM8921_CHG_VWEAK_MIN_MV
			|| millivolts > PM8921_CHG_VWEAK_MAX_MV) {
		pr_err("bad voltage = %dmV asked to set\n", millivolts);
		return -EINVAL;
	}

	temp = (millivolts - PM8921_CHG_VWEAK_MIN_MV)/PM8921_CHG_VWEAK_STEP_MV;
	return pm_chg_masked_write(chip, CHG_VTRICKLE, PM8921_CHG_VWEAK_MASK,
					 temp);
}

#define PM8921_CHG_ITRKL_MIN_MA		50
#define PM8921_CHG_ITRKL_MAX_MA		200
#define PM8921_CHG_ITRKL_MASK		0x0F
#define PM8921_CHG_ITRKL_STEP_MA	10
static int pm_chg_itrkl_set(struct pm8921_chg_chip *chip, int milliamps)
{
	u8 temp;

	if (milliamps < PM8921_CHG_ITRKL_MIN_MA
		|| milliamps > PM8921_CHG_ITRKL_MAX_MA) {
		pr_err("bad current = %dmA asked to set\n", milliamps);
		return -EINVAL;
	}

	temp = (milliamps - PM8921_CHG_ITRKL_MIN_MA)/PM8921_CHG_ITRKL_STEP_MA;

	return pm_chg_masked_write(chip, CHG_ITRICKLE, PM8921_CHG_ITRKL_MASK,
					 temp);
}

#define PM8921_CHG_IWEAK_MIN_MA		325
#define PM8921_CHG_IWEAK_MAX_MA		525
#define PM8921_CHG_IWEAK_SHIFT		7
#define PM8921_CHG_IWEAK_MASK		0x80
static int pm_chg_iweak_set(struct pm8921_chg_chip *chip, int milliamps)
{
	u8 temp;

	if (milliamps < PM8921_CHG_IWEAK_MIN_MA
		|| milliamps > PM8921_CHG_IWEAK_MAX_MA) {
		pr_err("bad current = %dmA asked to set\n", milliamps);
		return -EINVAL;
	}

	if (milliamps < PM8921_CHG_IWEAK_MAX_MA)
		temp = 0;
	else
		temp = 1;

	temp = temp << PM8921_CHG_IWEAK_SHIFT;
	return pm_chg_masked_write(chip, CHG_ITRICKLE, PM8921_CHG_IWEAK_MASK,
					 temp);
}

#define PM8921_CHG_BATT_TEMP_THR_COLD	BIT(1)
#define PM8921_CHG_BATT_TEMP_THR_COLD_SHIFT	1
static int pm_chg_batt_cold_temp_config(struct pm8921_chg_chip *chip,
					enum pm8921_chg_cold_thr cold_thr)
{
	u8 temp;

	temp = cold_thr << PM8921_CHG_BATT_TEMP_THR_COLD_SHIFT;
	temp = temp & PM8921_CHG_BATT_TEMP_THR_COLD;
	return pm_chg_masked_write(chip, CHG_CNTRL_2,
					PM8921_CHG_BATT_TEMP_THR_COLD,
					 temp);
}

#define PM8921_CHG_BATT_TEMP_THR_HOT		BIT(0)
#define PM8921_CHG_BATT_TEMP_THR_HOT_SHIFT	0
static int pm_chg_batt_hot_temp_config(struct pm8921_chg_chip *chip,
					enum pm8921_chg_hot_thr hot_thr)
{
	u8 temp;

	temp = hot_thr << PM8921_CHG_BATT_TEMP_THR_HOT_SHIFT;
	temp = temp & PM8921_CHG_BATT_TEMP_THR_HOT;
	return pm_chg_masked_write(chip, CHG_CNTRL_2,
					PM8921_CHG_BATT_TEMP_THR_HOT,
					 temp);
}

static void disable_input_voltage_regulation(struct pm8921_chg_chip *chip)
{
	u8 temp;

	pm_chg_write(chip, CHG_BUCK_CTRL_TEST3, 0x70);
	pm8xxx_readb(chip->dev->parent, CHG_BUCK_CTRL_TEST3, &temp);
	
	temp |= 0x81;
	pm_chg_write(chip, CHG_BUCK_CTRL_TEST3, temp);
}

static void enable_input_voltage_regulation(struct pm8921_chg_chip *chip)
{
	u8 temp;

	pm_chg_write(chip, CHG_BUCK_CTRL_TEST3, 0x70);
	pm8xxx_readb(chip->dev->parent, CHG_BUCK_CTRL_TEST3, &temp);
	
	temp &= 0xFE;
	
	temp |= 0x80;
	pm_chg_write(chip, CHG_BUCK_CTRL_TEST3, temp);
}

static int64_t read_battery_id(struct pm8921_chg_chip *chip)
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

static int is_battery_valid(struct pm8921_chg_chip *chip)
{
	int64_t rc;

	if (chip->batt_id_min == 0 && chip->batt_id_max == 0)
		return 1;

	rc = read_battery_id(chip);
	if (rc < 0) {
		pr_err("error reading batt id channel = %d, rc = %lld\n",
					chip->vbat_channel, rc);
		
		return 1;
	}

	if (rc < chip->batt_id_min || rc > chip->batt_id_max) {
		pr_err("batt_id phy =%lld is not valid\n", rc);
		return 0;
	}
	return 1;
}

static void check_battery_valid(struct pm8921_chg_chip *chip)
{
	if (is_battery_valid(chip) == 0) {
		pr_err("batt_id not valid, disbling charging\n");
		pm_chg_disable_auto_enable(chip, 1, BATT_CHG_DISABLED_BIT_BAT);
	} else {
		pm_chg_disable_auto_enable(chip, 0, BATT_CHG_DISABLED_BIT_BAT);
	}
}

static void battery_id_valid(struct work_struct *work)
{
	struct pm8921_chg_chip *chip = container_of(work,
				struct pm8921_chg_chip, battery_id_valid_work);

	check_battery_valid(chip);
}

static void pm8921_chg_enable_irq(struct pm8921_chg_chip *chip, int interrupt)
{
	if (!__test_and_set_bit(interrupt, chip->enabled_irqs)) {
		dev_dbg(chip->dev, "%d\n", chip->pmic_chg_irq[interrupt]);
		enable_irq(chip->pmic_chg_irq[interrupt]);
	}
}

static void pm8921_chg_disable_irq(struct pm8921_chg_chip *chip, int interrupt)
{
	if (__test_and_clear_bit(interrupt, chip->enabled_irqs)) {
		dev_dbg(chip->dev, "%d\n", chip->pmic_chg_irq[interrupt]);
		disable_irq_nosync(chip->pmic_chg_irq[interrupt]);
	}
}

void pm8921_chg_enable_usbin_valid_irq(void)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return;
	}
	pm8921_chg_enable_irq(the_chip, USBIN_VALID_IRQ);
}
EXPORT_SYMBOL(pm8921_chg_enable_usbin_valid_irq);

void pm8921_chg_disable_usbin_valid_irq(void)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return;
	}
	pm8921_chg_disable_irq(the_chip, USBIN_VALID_IRQ);
}
EXPORT_SYMBOL(pm8921_chg_disable_usbin_valid_irq);

static int pm8921_chg_is_enabled(struct pm8921_chg_chip *chip, int interrupt)
{
	return test_bit(interrupt, chip->enabled_irqs);
}

static bool is_using_ext_charger(struct pm8921_chg_chip *chip)
{
	if (chip->ext_usb)
		return true;

	if (chip->ext)
		return true;


	return false;
}

static bool is_ext_charging(struct pm8921_chg_chip *chip)
{
	
	if ((chip->ext == NULL) && (chip->ext_usb == NULL))
		return false;

	if (chip->ext_charging || chip->ext_usb_charging)
		return true;

	return false;
}

static bool is_ext_trickle_charging(struct pm8921_chg_chip *chip)
{
	if (chip->ext == NULL)
		return false;

	if (chip->ext->is_trickle(chip->ext->ctx))
		return true;

	return false;
}

static int is_battery_charging(int fsm_state)
{
	if (is_ext_charging(the_chip))
		return 1;

	switch (fsm_state) {
	case FSM_STATE_ATC_2A:
	case FSM_STATE_ATC_2B:
	case FSM_STATE_ON_CHG_AND_BAT_6:
	case FSM_STATE_FAST_CHG_7:
	case FSM_STATE_TRKL_CHG_8:
		return 1;
	}
	return 0;
}

static void bms_notify(struct work_struct *work)
{
	struct bms_notify *n = container_of(work, struct bms_notify, work);

	if (n->is_charging) {
		pm8921_bms_charging_began();
	} else {
		pm8921_bms_charging_end(n->is_battery_full);
		n->is_battery_full = 0;
	}
}

static void bms_notify_check(struct pm8921_chg_chip *chip)
{
	int fsm_state, new_is_charging;

	if(the_chip->ext_usb)
	{
		queue_delayed_work(ext_charger_wq, &ext_usb_bms_notify_task, 0);
		return;
	}

	fsm_state = pm_chg_get_fsm_state(chip);
	new_is_charging = is_battery_charging(fsm_state);

	if (chip->bms_notify.is_charging ^ new_is_charging) {
		chip->bms_notify.is_charging = new_is_charging;
		schedule_work(&(chip->bms_notify.work));
	}
}

static int get_prop_battery_uvolts(struct pm8921_chg_chip *chip)
{
	int rc;
	struct pm8xxx_adc_chan_result result;

	rc = pm8xxx_adc_read(chip->vbat_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					chip->vbat_channel, rc);
		return rc;
	}
	pr_debug("mvolts phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);
	return (int)result.physical;
}

static int get_prop_dcin_uvolts(struct pm8921_chg_chip *chip)
{
	int rc;
	struct pm8xxx_adc_chan_result result;

	rc = pm8xxx_adc_read(CHANNEL_DCIN, &result);
	if (rc) {
		pr_err("error reading dc_in channel = %d, rc = %d\n",
					CHANNEL_DCIN, rc);
		return rc;
	}
	pr_debug("dc_in uvolts phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);
	return (int)result.physical;
}

static unsigned int voltage_based_capacity(struct pm8921_chg_chip *chip)
{
	unsigned int current_voltage_uv = get_prop_battery_uvolts(chip);
	unsigned int current_voltage_mv = current_voltage_uv / 1000;
	unsigned int low_voltage = chip->min_voltage_mv;
	unsigned int high_voltage = chip->max_voltage_mv;

	if (current_voltage_mv <= low_voltage)
		return 0;
	else if (current_voltage_mv >= high_voltage)
		return 100;
	else
		return (current_voltage_mv - low_voltage) * 100
		    / (high_voltage - low_voltage);
}

static int get_prop_batt_capacity(struct pm8921_chg_chip *chip)
{
	int percent_soc = pm8921_bms_get_percent_charge();

	if (percent_soc == -ENXIO)
		percent_soc = voltage_based_capacity(chip);

	if (percent_soc <= 10)
		pr_warn("low battery charge = %d%%\n", percent_soc);

	
	if (test_power_monitor) {
		pr_info("soc=%d report 77(test_power_monitor)\n",
				percent_soc);
		percent_soc = 77;
	}
	if (percent_soc < 0) {
		pr_warn("soc=%d is out of range!\n", percent_soc);
		percent_soc = 0;
	} else if (100 < percent_soc) {
		pr_warn("soc=%d is out of range!\n", percent_soc);
		percent_soc = 100;
	}

	return percent_soc;
}

static int get_prop_batt_current(struct pm8921_chg_chip *chip)
{
	int result_ua, rc;

	rc = pm8921_bms_get_battery_current(&result_ua);
	if (rc == -ENXIO)
		rc = pm8xxx_ccadc_get_battery_current(&result_ua);

	if (rc) {
		pr_err("unable to get batt current rc = %d\n", rc);
		return rc;
	} else {
		return result_ua;
	}
}

static int get_prop_batt_fcc(struct pm8921_chg_chip *chip)
{
	int rc;

	rc = pm8921_bms_get_fcc();
	if (rc < 0)
		pr_err("unable to get batt fcc rc = %d\n", rc);
	return rc;
}

static int get_prop_batt_health(struct pm8921_chg_chip *chip)
{
	int temp;

	
	if (test_power_monitor) {
		pr_info("report HEALTH_GOOD(test_power_monitor)\n");
		return POWER_SUPPLY_HEALTH_GOOD;
	}

	temp = pm_chg_get_rt_status(chip, BATTTEMP_HOT_IRQ);
	if (temp){
		pr_debug("ABHI bat overheat\n");
		return POWER_SUPPLY_HEALTH_OVERHEAT;
	}

	temp = pm_chg_get_rt_status(chip, BATTTEMP_COLD_IRQ);
	if (temp){
		pr_debug("ABHI bat overcold\n");
		return POWER_SUPPLY_HEALTH_COLD;
	}

	return POWER_SUPPLY_HEALTH_GOOD;
}

static int get_prop_batt_present(struct pm8921_chg_chip *chip)
{
	
	if (test_power_monitor) {
		pr_info("report batt present (test_power_monitor)\n");
		return 1;
	}
	return pm_chg_get_rt_status(chip, BATT_INSERTED_IRQ);
}

static int get_prop_charge_type(struct pm8921_chg_chip *chip)
{
	int temp;

	if (!get_prop_batt_present(chip))
		return POWER_SUPPLY_CHARGE_TYPE_NONE;

	if (is_ext_trickle_charging(chip))
		return POWER_SUPPLY_CHARGE_TYPE_TRICKLE;

	if (is_ext_charging(chip))
		return POWER_SUPPLY_CHARGE_TYPE_FAST;

	temp = pm_chg_get_rt_status(chip, TRKLCHG_IRQ);
	if (temp){
		pr_debug("ABHI %s trickle chg\n", __func__);
		return POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
	}

	temp = pm_chg_get_rt_status(chip, FASTCHG_IRQ);
	if (temp){
		pr_debug("ABHI %s fast chg\n", __func__);
		return POWER_SUPPLY_CHARGE_TYPE_FAST;
	}

	return POWER_SUPPLY_CHARGE_TYPE_NONE;
}

static int get_prop_batt_status(struct pm8921_chg_chip *chip)
{
	int batt_state = POWER_SUPPLY_STATUS_DISCHARGING;
	int fsm_state = pm_chg_get_fsm_state(chip);
	int i;

	if (chip->ext_usb) {
		if (chip->ext_usb_charge_done)
			return POWER_SUPPLY_STATUS_FULL;
		if (chip->ext_usb_charging)
			return POWER_SUPPLY_STATUS_CHARGING;
	}
	if (chip->ext) {
		if (chip->ext_charge_done)
			return POWER_SUPPLY_STATUS_FULL;
		if (chip->ext_charging)
			return POWER_SUPPLY_STATUS_CHARGING;
	}

	for (i = 0; i < ARRAY_SIZE(map); i++)
		if (map[i].fsm_state == fsm_state)
			batt_state = map[i].batt_state;

	if (fsm_state == FSM_STATE_ON_CHG_HIGHI_1) {
		if (!pm_chg_get_rt_status(chip, BATT_INSERTED_IRQ)
			|| !pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ)
			|| pm_chg_get_rt_status(chip, CHGHOT_IRQ)
			|| pm_chg_get_rt_status(chip, VBATDET_LOW_IRQ))

			batt_state = POWER_SUPPLY_STATUS_NOT_CHARGING;
	}
	pr_debug("ABHI %d\n",batt_state);
	return batt_state;
}

static int get_prop_batt_temp(struct pm8921_chg_chip *chip)
{
	int rc;
	struct pm8xxx_adc_chan_result result;

	rc = pm8xxx_adc_read(chip->batt_temp_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					chip->vbat_channel, rc);
		return rc;
	}
	pr_debug("batt_temp phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);

	
	if (test_power_monitor) {
		pr_info("temp=%d report 25 C(test_power_monitor)\n",
				(int)result.physical);
		return 250;
	}

	pr_debug("ABHI %d deci Deg Cel\n", (int)result.physical);
	return (int)result.physical;
}

int pm8921_get_batt_voltage(int *result)
{
	int rc;
	struct pm8xxx_adc_chan_result ch_result;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	rc = pm8xxx_adc_read(the_chip->vbat_channel, &ch_result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					the_chip->vbat_channel, rc);
		return rc;
	}
	*result = ((int)ch_result.physical) / 1000;
	pr_debug("batt_voltage phy = %lld meas = 0x%llx\n", ch_result.physical,
						ch_result.measurement);
	return rc;
}

int pm8921_set_chg_ovp(int is_ovp)
{
	pr_info("%s, is_ovp:%d\n", __func__, is_ovp);
	if(is_ovp)
	{
		ovp = 1;
		htc_charger_event_notify(HTC_CHARGER_EVENT_OVP);
	}
	else
	{
		ovp = 0;
		htc_charger_event_notify(HTC_CHARGER_EVENT_OVP_RESOLVE);
	}
	return 0;
}
EXPORT_SYMBOL(pm8921_set_chg_ovp);

int pm8921_get_batt_temperature(int *result)
{
	int rc;
	struct pm8xxx_adc_chan_result ch_result;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	rc = pm8xxx_adc_read(the_chip->batt_temp_channel, &ch_result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					the_chip->batt_temp_channel, rc);
		return rc;
	}
	*result = (int)ch_result.physical;
	if ((*result >= 680) &&
			(flag_keep_charge_on || flag_pa_recharge))
		*result = 650;
	pr_debug("batt_temp phy = %lld meas = 0x%llx\n", ch_result.physical,
						ch_result.measurement);
	return rc;
}

#define PM8921_CHARGER_HTC_FAKE_BATT_ID (1)
int pm8921_get_batt_id(int *result)
{
	int rc = 0;
	struct pm8xxx_adc_chan_result ch_result;
	int id_raw_mv;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	
	if (pm8xxx_get_revision(the_chip->dev->parent) < PM8XXX_REVISION_8921_2p0
		&& pm8xxx_get_version(the_chip->dev->parent) == PM8XXX_VERSION_8921) {
		*result = PM8921_CHARGER_HTC_FAKE_BATT_ID;
	} else {
		rc = pm8xxx_adc_read(the_chip->batt_id_channel, &ch_result);
		if (rc) {
			pr_err("error reading adc channel = %d, rc = %d\n",
					the_chip->batt_id_channel, rc);
			return rc;
		}
		pr_debug("batt_id phy = %lld meas = 0x%llx\n", ch_result.physical,
						ch_result.measurement);
		id_raw_mv = ((int)ch_result.physical) / 1000;
		
		*result = htc_battery_cell_find_and_set_id_auto(id_raw_mv);
	}
	return rc;
}

int pm8921_is_batt_temperature_fault(int *result)
{
	int is_cold, is_warm;
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	is_cold = pm_chg_get_rt_status(the_chip, BATTTEMP_COLD_IRQ);
	is_warm = pm8xxx_adc_btm_is_warm();

	pr_debug("is_cold=%d,is_warm=%d\n", is_cold, is_warm);
	if (is_cold || is_warm)
		*result = 1;
	else
		*result = 0;
	return 0;
}

int pm8921_is_batt_temp_fault_disable_chg(int *result)
{
	int is_cold, is_hot;
	int is_warm, is_vbatdet_low;
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	if(the_chip->ext_usb)
	{
		if(the_chip->ext_usb->ichg->is_batt_temp_fault_disable_chg)
			the_chip->ext_usb->ichg->is_batt_temp_fault_disable_chg(result);
		return 0;
	}


	is_cold = pm_chg_get_rt_status(the_chip, BATTTEMP_COLD_IRQ);
	is_hot = pm_chg_get_rt_status(the_chip, BATTTEMP_HOT_IRQ);
	is_warm = pm8xxx_adc_btm_is_warm();
	is_vbatdet_low = pm_chg_get_rt_status(the_chip, VBATDET_LOW_IRQ);

	pr_debug("is_cold=%d, is_hot=%d, is_warm=%d, is_vbatdet_low=%d\n",
			is_cold, is_hot, is_warm, is_vbatdet_low);
	if ((is_cold || (is_hot && is_warm) || (is_warm && !is_vbatdet_low)) &&
			!flag_keep_charge_on && !flag_pa_recharge)
		*result = 1;
	else
		*result = 0;
	return 0;
}

int pm8921_is_batt_full(int *result)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	*result = is_batt_full;
	return 0;
}

int pm8921_is_pwrsrc_under_rating(int *result)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	*result = pwrsrc_under_rating;
	return 0;
}

int pm8921_is_batt_full_eoc_stop(int *result)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	*result = is_batt_full_eoc_stop;
	return 0;
}

int pm8921_gauge_get_attr_text(char *buf, int size)
{
	int len = 0;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	len += scnprintf(buf + len, size - len,
			"SOC(%%): %d;\n"
			"EOC(bool): %d;\n"
			"OVP(bool): %d;\n"
			"UVP(bool): %d;\n"
			"VBAT(uV): %d;\n"
			"IBAT(uA): %d;\n"
			"ID_RAW(uV): %d;\n"
			"BATT_TEMP(deci-celsius): %d;\n"
			"is_bat_warm(bool): %d\n"
			"is_bat_cool(bool): %d\n"
			"BTM_WARM_IRQ: %d\n"
			"BTM_COOL_IRQ: %d\n"
			"BATT_PRESENT(bool): %d;\n"
			"FCC(uAh): %d;\n"
			"usb_target_ma(mA): %d;\n",
			pm8921_bms_get_percent_charge(),
			is_batt_full,
			ovp,
			uvp,
			get_prop_battery_uvolts(the_chip),
			get_prop_batt_current(the_chip),
			(int)read_battery_id(the_chip),
			get_prop_batt_temp(the_chip),
			the_chip->is_bat_warm,
			the_chip->is_bat_cool,
			pm8xxx_adc_btm_is_warm(),
			pm8xxx_adc_btm_is_cool(),
			get_prop_batt_present(the_chip),
			pm8921_bms_get_fcc(),
			usb_target_ma
			);

	len += pm8921_bms_get_attr_text(buf + len, size - len);

	return len;
}
int pm8921_pwrsrc_enable(bool enable)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	if(the_chip->ext_usb)
	{
		int ret = 0;

		if(the_chip->ext_usb->ichg->set_pwrsrc_enable)
			ret = the_chip->ext_usb->ichg->set_pwrsrc_enable(enable);

		bms_notify_check(the_chip);
		return ret;
	}

	return pm_chg_disable_pwrsrc(the_chip, !enable, PWRSRC_DISABLED_BIT_KDRV);
}
EXPORT_SYMBOL(pm8921_pwrsrc_enable);

int pm8921_get_charging_source(int *result)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	if(the_chip->ext_usb)
	{
		if(the_chip->ext_usb->ichg->get_charging_source)
			the_chip->ext_usb->ichg->get_charging_source(result);
		return 0;
	}

	*result = pwr_src;


	return 0;
}

int pm8921_is_chg_safety_timer_timeout(int *result)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	*result = is_ac_safety_timeout;
	return 0;
}

int pm8921_get_charging_enabled(int *result)
{
	int fsm_state;
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	if(the_chip->ext_usb)
	{
		if(the_chip->ext_usb->ichg->get_charging_enabled)
			return the_chip->ext_usb->ichg->get_charging_enabled(result);
	}

	fsm_state = pm_chg_get_fsm_state(the_chip);
	if (is_battery_charging(fsm_state))
		return pm8921_get_charging_source(result);
	else
		*result = HTC_PWR_SOURCE_TYPE_BATT;
	return 0;
}

int pm8921_is_charger_ovp(int* result)
{
	int ov, uv, v;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	ov = pm_chg_get_rt_status(the_chip, USBIN_OV_IRQ);
	v = pm_chg_get_rt_status(the_chip, USBIN_VALID_IRQ);
	uv = pm_chg_get_rt_status(the_chip, USBIN_UV_IRQ);
	pr_info("usbin_ov_irq_state:%d -> %d [%d,%d,%d]\n",
					usbin_ov_irq_state, ov, ov, v, uv);
	usbin_ov_irq_state = ov;
	update_ovp_uvp_state(ov, v, uv);
	*result = ovp;

	if(the_chip->ext_usb)
	{
		if(the_chip->ext_usb->ichg->is_ovp)
			the_chip->ext_usb->ichg->is_ovp(result);
	}

	return 0;
}

int pm8921_charger_get_attr_text_with_ext_charger(char *buf, int size)
{
	int rc;
	int len = 0;
	struct pm8xxx_adc_chan_result result;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	len += scnprintf(buf + len, size - len,
			"FSM: %d;\n"
			"ATCDONE_IRQ: %d;\n"
			"ATCFAIL_IRQ: %d;\n"
			"BATFET_IRQ: %d;\n"
			"BATTTEMP_COLD_IRQ: %d;\n"
			"BATTTEMP_HOT_IRQ: %d;\n"
			"BATT_INSERTED_IRQ: %d;\n"
			"BATT_REMOVED_IRQ: %d;\n"
			"BAT_TEMP_OK_IRQ: %d;\n"
			"DCIN_UV_IRQ: %d;\n"
			"DCIN_VALID_IRQ: %d;\n"
			"LOOP_CHANGE_IRQ: %d;\n"
			"TRKLCHG_IRQ: %d;\n"
			"USBIN_OV_IRQ: %d;\n"
			"USBIN_UV_IRQ: %d;\n"
			"USBIN_VALID_IRQ: %d;\n"
			"VBATDET_IRQ: %d;\n"
			"VBATDET_LOW_IRQ: %d;\n"
			"VBAT_OV_IRQ: %d;\n"
			"VCP_IRQ: %d;\n"
			"VDD_LOOP_IRQ: %d;\n"
			"VREG_OV_IRQ: %d;\n"
			"REGULATION_LOOP: 0x%02x\n",
			pm_chg_get_fsm_state(the_chip),
			pm_chg_get_rt_status(the_chip, ATCDONE_IRQ),
			pm_chg_get_rt_status(the_chip, ATCFAIL_IRQ),
			pm_chg_get_rt_status(the_chip, BATFET_IRQ),
			pm_chg_get_rt_status(the_chip, BATTTEMP_COLD_IRQ),
			pm_chg_get_rt_status(the_chip, BATTTEMP_HOT_IRQ),
			pm_chg_get_rt_status(the_chip, BATT_INSERTED_IRQ),
			pm_chg_get_rt_status(the_chip, BATT_REMOVED_IRQ),
			pm_chg_get_rt_status(the_chip, BAT_TEMP_OK_IRQ),
			pm_chg_get_rt_status(the_chip, DCIN_UV_IRQ),
			pm_chg_get_rt_status(the_chip, DCIN_VALID_IRQ),
			pm_chg_get_rt_status(the_chip, LOOP_CHANGE_IRQ),
			pm_chg_get_rt_status(the_chip, TRKLCHG_IRQ),
			pm_chg_get_rt_status(the_chip, USBIN_OV_IRQ),
			pm_chg_get_rt_status(the_chip, USBIN_UV_IRQ),
			pm_chg_get_rt_status(the_chip, USBIN_VALID_IRQ),
			pm_chg_get_rt_status(the_chip, VBATDET_IRQ),
			pm_chg_get_rt_status(the_chip, VBATDET_LOW_IRQ),
			pm_chg_get_rt_status(the_chip, VBAT_OV_IRQ),
			pm_chg_get_rt_status(the_chip, VCP_IRQ),
			pm_chg_get_rt_status(the_chip, VDD_LOOP_IRQ),
			pm_chg_get_rt_status(the_chip, VREG_OV_IRQ),
			pm_chg_get_regulation_loop(the_chip)
			);

	rc = pm8xxx_adc_read(CHANNEL_USBIN, &result);
	if (rc) {
		pr_err("error reading i_chg channel = %d, rc = %d\n",
					CHANNEL_USBIN, rc);
	}

	len += scnprintf(buf + len, size - len,
			"USBIN(uV): %d;\n", (int)result.physical);

	pr_info("USBIN(uV): %d;\n", (int)result.physical);

	len += scnprintf(buf + len, size - len,
			"usbin_critical_low_cnt(int): %d;\n", usbin_critical_low_cnt);
	len += scnprintf(buf + len, size - len,
			"pwrsrc_under_rating(bool): %d;\n", pwrsrc_under_rating);

	if(the_chip->ext_usb)
	{
		if(the_chip->ext_usb->ichg->get_attr_text)
			len += the_chip->ext_usb->ichg->get_attr_text(buf+len, size);
	}

	return len;
}


int pm8921_charger_get_attr_text(char *buf, int size)
{
	int rc;
	struct pm8xxx_adc_chan_result result;
	int len = 0;
	u64 val = 0;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	len += scnprintf(buf + len, size - len,
			"FSM: %d;\n"
			"ATCDONE_IRQ: %d;\n"
			"ATCFAIL_IRQ: %d;\n"
			"BATFET_IRQ: %d;\n"
			"BATTTEMP_COLD_IRQ: %d;\n"
			"BATTTEMP_HOT_IRQ: %d;\n"
			"BATT_INSERTED_IRQ: %d;\n"
			"BATT_REMOVED_IRQ: %d;\n"
			"BAT_TEMP_OK_IRQ: %d;\n"
			"CHGDONE_IRQ: %d;\n"
			"CHGFAIL_IRQ: %d;\n"
			"CHGHOT_IRQ: %d;\n"
			"CHGSTATE_IRQ: %d;\n"
			"CHGWDOG_IRQ: %d;\n"
			"CHG_GONE_IRQ: %d;\n"
			"COARSE_DET_LOW_IRQ: %d;\n"
			"DCIN_OV_IRQ: %d;\n"
			"DCIN_UV_IRQ: %d;\n"
			"DCIN_VALID_IRQ: %d;\n"
			"USBIN_OV_IRQ: %d;\n"
			"USBIN_UV_IRQ: %d;\n"
			"USBIN_VALID_IRQ: %d;\n"
			"FASTCHG_IRQ: %d;\n"
			"LOOP_CHANGE_IRQ: %d;\n"
			"TRKLCHG_IRQ: %d;\n"
			"VBATDET_IRQ: %d;\n"
			"VBATDET_LOW_IRQ: %d;\n"
			"VBAT_OV_IRQ: %d;\n"
			"VCP_IRQ: %d;\n"
			"VDD_LOOP_IRQ: %d;\n"
			"VREG_OV_IRQ: %d;\n"
			"REGULATION_LOOP: 0x%02x\n",
			pm_chg_get_fsm_state(the_chip),
			pm_chg_get_rt_status(the_chip, ATCDONE_IRQ),
			pm_chg_get_rt_status(the_chip, ATCFAIL_IRQ),
			pm_chg_get_rt_status(the_chip, BATFET_IRQ),
			pm_chg_get_rt_status(the_chip, BATTTEMP_COLD_IRQ),
			pm_chg_get_rt_status(the_chip, BATTTEMP_HOT_IRQ),
			pm_chg_get_rt_status(the_chip, BATT_INSERTED_IRQ),
			pm_chg_get_rt_status(the_chip, BATT_REMOVED_IRQ),
			pm_chg_get_rt_status(the_chip, BAT_TEMP_OK_IRQ),
			pm_chg_get_rt_status(the_chip, CHGDONE_IRQ),
			pm_chg_get_rt_status(the_chip, CHGFAIL_IRQ),
			pm_chg_get_rt_status(the_chip, CHGHOT_IRQ),
			pm_chg_get_rt_status(the_chip, CHGSTATE_IRQ),
			pm_chg_get_rt_status(the_chip, CHGWDOG_IRQ),
			pm_chg_get_rt_status(the_chip, CHG_GONE_IRQ),
			pm_chg_get_rt_status(the_chip, COARSE_DET_LOW_IRQ),
			pm_chg_get_rt_status(the_chip, DCIN_OV_IRQ),
			pm_chg_get_rt_status(the_chip, DCIN_UV_IRQ),
			pm_chg_get_rt_status(the_chip, DCIN_VALID_IRQ),
			pm_chg_get_rt_status(the_chip, USBIN_OV_IRQ),
			pm_chg_get_rt_status(the_chip, USBIN_UV_IRQ),
			pm_chg_get_rt_status(the_chip, USBIN_VALID_IRQ),
			pm_chg_get_rt_status(the_chip, FASTCHG_IRQ),
			pm_chg_get_rt_status(the_chip, LOOP_CHANGE_IRQ),
			pm_chg_get_rt_status(the_chip, TRKLCHG_IRQ),
			pm_chg_get_rt_status(the_chip, VBATDET_IRQ),
			pm_chg_get_rt_status(the_chip, VBATDET_LOW_IRQ),
			pm_chg_get_rt_status(the_chip, VBAT_OV_IRQ),
			pm_chg_get_rt_status(the_chip, VCP_IRQ),
			pm_chg_get_rt_status(the_chip, VDD_LOOP_IRQ),
			pm_chg_get_rt_status(the_chip, VREG_OV_IRQ),
			pm_chg_get_regulation_loop(the_chip)
			);

	rc = pm8xxx_adc_read(CHANNEL_USBIN, &result);
	if (rc) {
		pr_err("error reading i_chg channel = %d, rc = %d\n",
					CHANNEL_USBIN, rc);
	}
	len += scnprintf(buf + len, size - len,
			"USBIN(uV): %d;\n", (int)result.physical);

	len += scnprintf(buf + len, size - len,
			"DCIN(uV): %d;\n", get_prop_dcin_uvolts(the_chip));

	rc = pm8xxx_adc_read(CHANNEL_VPH_PWR, &result);
	if (rc) {
		pr_err("error reading vph_pwr channel = %d, rc = %d\n",
					CHANNEL_VPH_PWR, rc);
	}
	len += scnprintf(buf + len, size - len,
			"VPH_PWR(uV): %d;\n", (int)result.physical);

	if (the_chip->wlc_tx_gpio)
		len += scnprintf(buf + len, size - len,
				"is_wlc_remove(bool): %d;\n", gpio_get_value(the_chip->wlc_tx_gpio));

	len += scnprintf(buf + len, size - len,
			"AC_SAFETY_TIMEOUT(bool): %d;\n", (int)is_ac_safety_timeout);

	len += scnprintf(buf + len, size - len,
			"AC_SAFETY_TIMEOUT2(bool): %d;\n", (int)is_ac_safety_timeout_twice);

	len += scnprintf(buf + len, size - len,
			"mitigation_level(int): %d;\n", thermal_mitigation);

	len += scnprintf(buf + len, size - len,
			"eoc_count(int): %d;\n", eoc_count);

	len += scnprintf(buf + len, size - len,
			"reverse_boost_disabled(bool): %d;\n", the_chip->disable_reverse_boost_check);

	len += scnprintf(buf + len, size - len,
			"usbin_critical_low_cnt(int): %d;\n", usbin_critical_low_cnt);
	len += scnprintf(buf + len, size - len,
			"pwrsrc_under_rating(bool): %d;\n", pwrsrc_under_rating);

	get_reg((void *)CHG_CNTRL, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_CNTRL: 0x%llx;\n", val);
	get_reg((void *)CHG_CNTRL_2, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_CNTRL_2: 0x%llx;\n", val);
	get_reg((void *)CHG_CNTRL_3, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_CNTRL_3: 0x%llx;\n", val);
	get_reg((void *)PBL_ACCESS1, &val);
	len += scnprintf(buf + len, size - len,
			"PBL_ACCESS1: 0x%llx;\n", val);
	get_reg((void *)PBL_ACCESS2, &val);
	len += scnprintf(buf + len, size - len,
			"PBL_ACCESS2: 0x%llx;\n", val);
	get_reg((void *)SYS_CONFIG_1, &val);
	len += scnprintf(buf + len, size - len,
			"SYS_CONFIG_1: 0x%llx;\n", val);
	get_reg((void *)SYS_CONFIG_2, &val);
	len += scnprintf(buf + len, size - len,
			"SYS_CONFIG_2: 0x%llx;\n", val);
	get_reg((void *)CHG_IBAT_MAX, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_IBAT_MAX: 0x%llx;\n", val);
	get_reg((void *)CHG_IBAT_SAFE, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_IBAT_SAFE: 0x%llx;\n", val);
	get_reg((void *)CHG_VDD_MAX, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_VDD_MAX: 0x%llx;\n", val);
	get_reg((void *)CHG_VDD_SAFE, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_VDD_SAFE: 0x%llx;\n", val);
	len += scnprintf(buf + len, size - len,
			"last_delta_batt_terminal_mv: %d;\n", last_delta_mv);
	get_reg((void *)CHG_VBAT_DET, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_VBAT_DET: 0x%llx;\n", val);
	get_reg((void *)CHG_VIN_MIN, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_VIN_MIN: 0x%llx;\n", val);
	get_reg((void *)CHG_VTRICKLE, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_VTRICKLE: 0x%llx;\n", val);
	get_reg((void *)CHG_ITRICKLE, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_ITRICKLE: 0x%llx;\n", val);
	get_reg((void *)CHG_ITERM, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_ITERM: 0x%llx;\n", val);
	get_reg((void *)CHG_TCHG_MAX, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_TCHG_MAX: 0x%llx;\n", val);
	get_reg((void *)CHG_TWDOG, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_TWDOG: 0x%llx;\n", val);
	get_reg((void *)CHG_TEMP_THRESH, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_TEMP_THRESH: 0x%llx;\n", val);
	get_reg((void *)CHG_COMP_OVR, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_COMP_OVR: 0x%llx;\n", val);
	get_reg((void *)CHG_BUCK_CTRL_TEST1, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_BUCK_CTRL_TEST1: 0x%llx;\n", val);
	get_reg((void *)CHG_BUCK_CTRL_TEST2, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_BUCK_CTRL_TEST2: 0x%llx;\n", val);
	get_reg((void *)CHG_BUCK_CTRL_TEST3, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_BUCK_CTRL_TEST3: 0x%llx;\n", val);
	get_reg((void *)CHG_TEST, &val);
	len += scnprintf(buf + len, size - len,
			"CHG_TEST: 0x%llx;\n", val);
	get_reg((void *)USB_OVP_CONTROL, &val);
	len += scnprintf(buf + len, size - len,
			"USB_OVP_CONTROL: 0x%llx;\n", val);
	get_reg((void *)USB_OVP_TEST, &val);
	len += scnprintf(buf + len, size - len,
			"USB_OVP_TEST: 0x%llx;\n", val);
	get_reg((void *)DC_OVP_CONTROL, &val);
	len += scnprintf(buf + len, size - len,
			"DC_OVP_CONTROL: 0x%llx;\n", val);
	get_reg((void *)DC_OVP_TEST, &val);
	len += scnprintf(buf + len, size - len,
			"DC_OVP_TEST: 0x%llx;\n", val);

	return len;
}

static void (*notify_vbus_state_func_ptr)(int);
static int usb_chg_current;
static DEFINE_SPINLOCK(vbus_lock);

int pm8921_charger_register_vbus_sn(void (*callback)(int))
{
	pr_debug("%p\n", callback);
	notify_vbus_state_func_ptr = callback;
	return 0;
}
EXPORT_SYMBOL_GPL(pm8921_charger_register_vbus_sn);

void pm8921_charger_unregister_vbus_sn(void (*callback)(int))
{
	pr_debug("%p\n", callback);
	notify_vbus_state_func_ptr = NULL;
}
EXPORT_SYMBOL_GPL(pm8921_charger_unregister_vbus_sn);

static void notify_usb_of_the_plugin_event(int plugin)
{
	plugin = !!plugin;
	if (notify_vbus_state_func_ptr) {
		pr_debug("notifying plugin\n");
		(*notify_vbus_state_func_ptr) (plugin);
	} else {
		pr_debug("unable to notify plugin\n");
	}
}

static void __pm8921_charger_vbus_draw(unsigned int mA)
{
	int i, rc;

	if (mA > 0 && mA <= 2) {
		usb_chg_current = 0;
		rc = pm_chg_iusbmax_set(the_chip, 0);
		if (rc)
			pr_err("unable to set iusb to %d rc = %d\n", 0, rc);
		rc = pm_chg_usb_suspend_enable(the_chip, 1);
		if (rc)
			pr_err("fail to set suspend bit rc=%d\n", rc);
	} else {
		rc = pm_chg_usb_suspend_enable(the_chip, 0);
		if (rc)
			pr_err("fail to reset suspend bit rc=%d\n", rc);
		for (i = ARRAY_SIZE(usb_ma_table) - 1; i >= 0; i--) {
			if (usb_ma_table[i].usb_ma <= mA)
				break;
		}
		if (i < 0)
			i = 0;
		rc = pm_chg_iusbmax_set(the_chip, i);
		if (rc)
			pr_err("unable to set iusb to %d rc = %d\n", i, rc);
	}
}

#define USB_WALL_THRESHOLD_MA	(1500)
static void _pm8921_charger_vbus_draw(unsigned int mA)
{
	unsigned long flags;

	if (usb_max_current && mA > usb_max_current) {
		pr_warn("restricting usb current to %d instead of %d\n",
					usb_max_current, mA);
		mA = usb_max_current;
	}
	if (usb_target_ma == 0 && mA > USB_WALL_THRESHOLD_MA)
		usb_target_ma = mA;

	spin_lock_irqsave(&vbus_lock, flags);
	if (the_chip) {
		if (mA > USB_WALL_THRESHOLD_MA)
			__pm8921_charger_vbus_draw(USB_WALL_THRESHOLD_MA);
		else
			__pm8921_charger_vbus_draw(mA);
	} else {
		if (mA > USB_WALL_THRESHOLD_MA)
			usb_chg_current = USB_WALL_THRESHOLD_MA;
		else
			usb_chg_current = mA;
	}
	spin_unlock_irqrestore(&vbus_lock, flags);
}

static int pm8921_apply_19p2mhz_kickstart(struct pm8921_chg_chip *chip)
{
	int err;
	u8 temp;
	unsigned long flags = 0;

	pr_info("%s\n", __func__);
	spin_lock_irqsave(&lpm_lock, flags);
	err = pm8921_chg_set_lpm(chip, 0);
	if (err) {
		pr_err("Error settig LPM rc=%d\n", err);
		goto kick_err;
	}

	temp  = 0xD1;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		goto kick_err;
	}

	temp  = 0xD3;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		goto kick_err;
	}

	temp  = 0xD1;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		goto kick_err;
	}

	temp  = 0xD5;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		goto kick_err;
	}

	udelay(183);

	temp  = 0xD1;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		goto kick_err;
	}

	temp  = 0xD0;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		goto kick_err;
	}

	
	udelay(32);

kick_err:
	err = pm8921_chg_set_lpm(chip, 1);
	if (err)
		pr_err("Error settig LPM rc=%d\n", err);

	spin_unlock_irqrestore(&lpm_lock, flags);

	return err;
}

static void handle_usb_present_change(struct pm8921_chg_chip *chip,
				int usb_present)
{
	int rc = 0;

	if (chip->lockup_lpm_wrkarnd) {
		rc = pm8921_apply_19p2mhz_kickstart(chip);
		if (rc)
			pr_err("Failed to apply kickstart rc=%d\n", rc);
	}

	if (pm_chg_failed_clear(chip, 1))
		pr_err("Failed to write CHG_FAILED_CLEAR bit\n");
	if (chip->usb_present ^ usb_present) {
		pr_info("vbus present change: %d -> %d\n",
		chip->usb_present, usb_present);
		chip->usb_present = usb_present;

		
		if (chip->lockup_lpm_wrkarnd)
			pm8921_chg_bypass_bat_gone_debounce(chip,
				is_chg_on_bat(chip));
		if (usb_present) {
			is_cable_remove = false;
		} else {
			rc = pm_chg_disable_auto_enable(chip, 0, BATT_CHG_DISABLED_BIT_EOC);
			if (rc)
				pr_err("Failed to set auto_enable rc=%d\n", rc);

			is_batt_full = false;
			eoc_count = 0;
			is_ac_safety_timeout = is_ac_safety_timeout_twice = false;
			is_cable_remove = true;
			usbin_critical_low_cnt = 0;
			pwrsrc_under_rating = 0;

			pr_info("Set vbatdet=%d after cable out\n",
					PM8921_CHG_VBATDET_MAX);
			rc = pm_chg_vbatdet_set(chip, PM8921_CHG_VBATDET_MAX);
			if (rc)
				pr_err("Failed to set vbatdet=%d rc=%d\n",
						PM8921_CHG_VBATDET_MAX, rc);
		}
		if (!the_chip->cable_in_irq)
			usbin_ov_irq_handler(chip->pmic_chg_irq[USBIN_OV_IRQ], chip);
		is_batt_full_eoc_stop = false;
	}

	if (usb_present) {
			schedule_delayed_work(&chip->unplug_check_work,
				round_jiffies_relative(msecs_to_jiffies
					(UNPLUG_CHECK_WAIT_PERIOD_MS)));
			pm8921_chg_enable_irq(chip, CHG_GONE_IRQ);
	} else {
			
			usb_target_ma = 0;
			hsml_target_ma = 0;
			pm8921_chg_disable_irq(chip, CHG_GONE_IRQ);
	}
	enable_input_voltage_regulation(chip);

	bms_notify_check(chip);
}

static u32 htc_fake_charger_for_testing(enum htc_power_source_type src)
{
	
	enum htc_power_source_type new_src = HTC_PWR_SOURCE_TYPE_AC;

	if((src > HTC_PWR_SOURCE_TYPE_9VAC) || (src == HTC_PWR_SOURCE_TYPE_BATT))
		return src;

	pr_info("%s(%d -> %d)\n", __func__, src , new_src);
	return new_src;
}


int pm8921_set_pwrsrc_and_charger_enable(enum htc_power_source_type src,
		bool chg_enable, bool pwrsrc_enable)
{
	int mA = 0;
	int rc = 0;

	pr_info("src=%d, chg_enable=%d, pwrsrc_enable=%d\n",
				src, chg_enable, pwrsrc_enable);

	if (get_kernel_flag() & KERNEL_FLAG_ENABLE_FAST_CHARGE)
		src = htc_fake_charger_for_testing(src);


	pwr_src = src;
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	if(the_chip->ext_usb)
	{
		if(the_chip->ext_usb->ichg->set_pwrsrc_and_charger_enable)
			rc = the_chip->ext_usb->ichg->set_pwrsrc_and_charger_enable(src, chg_enable, pwrsrc_enable);

		if(chg_enable)
		{
			if (!flag_disable_wakelock)
				wake_lock(&the_chip->eoc_wake_lock);

			schedule_delayed_work(&the_chip->eoc_work,
			      round_jiffies_relative(msecs_to_jiffies
					(EOC_CHECK_PERIOD_MS)));

			pr_info("schedule_delayed_work(&the_chip->eoc_wora)k\n");

		}

		bms_notify_check(the_chip);

		return rc;
	}

	
	pm_chg_disable_pwrsrc(the_chip, !pwrsrc_enable, PWRSRC_DISABLED_BIT_KDRV);

	
	pm_chg_disable_auto_enable(the_chip, !chg_enable,
								BATT_CHG_DISABLED_BIT_KDRV);

	
	switch (src) {
	case HTC_PWR_SOURCE_TYPE_BATT:
		mA = USB_MA_2;
		break;
	case HTC_PWR_SOURCE_TYPE_WIRELESS:
		if (pm8921_is_dc_chg_plugged_in()) {
			pr_info("Wireless charger is from DC_IN\n");
			mA = USB_MA_1100;
		} else
			mA = USB_MA_500;
		break;
	case HTC_PWR_SOURCE_TYPE_DETECTING:
	case HTC_PWR_SOURCE_TYPE_UNKNOWN_USB:
	case HTC_PWR_SOURCE_TYPE_USB:
		mA = USB_MA_500;
		break;
	case HTC_PWR_SOURCE_TYPE_AC:
	case HTC_PWR_SOURCE_TYPE_9VAC:
	case HTC_PWR_SOURCE_TYPE_MHL_AC:
		mA = USB_MA_1100;
		break;
	default:
		mA = USB_MA_2;
		break;
	}

	
	if (the_chip->vin_min_wlc) {
		if ((HTC_PWR_SOURCE_TYPE_WIRELESS == src) &&
				pm8921_is_dc_chg_plugged_in())
			pm_chg_vinmin_set(the_chip, the_chip->vin_min_wlc);
		else
			pm_chg_vinmin_set(the_chip, the_chip->vin_min);
	}

	_pm8921_charger_vbus_draw(mA);
	if (HTC_PWR_SOURCE_TYPE_BATT == src)
		handle_usb_present_change(the_chip, 0);
	else
		handle_usb_present_change(the_chip, 1);

	return rc;
}
EXPORT_SYMBOL(pm8921_set_pwrsrc_and_charger_enable);

void pm8921_charger_vbus_draw(unsigned int mA)
{
	unsigned long flags;

	pr_info("Enter charge=%d (deprecated api)\n", mA);
	return;

	if (usb_max_current && mA > usb_max_current) {
		pr_warn("restricting usb current to %d instead of %d\n",
					usb_max_current, mA);
		mA = usb_max_current;
	}
	if (usb_target_ma == 0 && mA > USB_WALL_THRESHOLD_MA)
		usb_target_ma = mA;

	spin_lock_irqsave(&vbus_lock, flags);
	if (the_chip) {
		if (mA > USB_WALL_THRESHOLD_MA)
			__pm8921_charger_vbus_draw(USB_WALL_THRESHOLD_MA);
		else
			__pm8921_charger_vbus_draw(mA);
	} else {
		if (mA > USB_WALL_THRESHOLD_MA)
			usb_chg_current = USB_WALL_THRESHOLD_MA;
		else
			usb_chg_current = mA;
	}
	spin_unlock_irqrestore(&vbus_lock, flags);
}
EXPORT_SYMBOL_GPL(pm8921_charger_vbus_draw);

int pm8921_charger_enable(bool enable)
{
	int rc = 0;

	
	if(the_chip->ext_usb)
	{
		if(the_chip->ext_usb->ichg->set_charger_enable)
		{
			rc = the_chip->ext_usb->ichg->set_charger_enable(enable);
			return rc;
		}
	}

	if (the_chip) {
		enable = !!enable;
		rc = pm_chg_disable_auto_enable(the_chip, !enable, BATT_CHG_DISABLED_BIT_KDRV);
	} else {
		pr_err("called before init\n");
		rc = -EINVAL;
	}



	return rc;
}
EXPORT_SYMBOL(pm8921_charger_enable);

int pm8921_is_usb_chg_plugged_in(void)
{
	if (!the_chip) {
		pr_warn("%s: warn: called before init\n", __func__);
		return -EINVAL;
	}
	return is_usb_chg_plugged_in(the_chip);
}
EXPORT_SYMBOL(pm8921_is_usb_chg_plugged_in);

int pm8921_is_dc_chg_plugged_in(void)
{
	if (!the_chip) {
		pr_warn("%s: warn: called before init\n", __func__);
		return -EINVAL;
	}
	return is_dc_chg_plugged_in(the_chip);
}
EXPORT_SYMBOL(pm8921_is_dc_chg_plugged_in);

int pm8921_is_pwr_src_plugged_in(void)
{
	int usb_in, dc_in;

	usb_in = pm8921_is_usb_chg_plugged_in();
	dc_in = pm8921_is_dc_chg_plugged_in();
	pr_info("%s: usb_in=%d, dc_in=%d\n", __func__, usb_in, dc_in);
	if (usb_in ^ dc_in)
		return 1;
	else if (usb_in & dc_in)
		pr_warn("%s: Abnormal interrupt due to usb_in & dc_in"
				" is existed together\n", __func__);
	return 0;
}
EXPORT_SYMBOL(pm8921_is_pwr_src_plugged_in);

int pm8921_is_battery_present(void)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	return get_prop_batt_present(the_chip);
}
EXPORT_SYMBOL(pm8921_is_battery_present);

int pm8921_disable_input_current_limit(bool disable)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	if (disable) {
		pr_warn("Disabling input current limit!\n");

		return pm_chg_write(the_chip,
			 CHG_BUCK_CTRL_TEST3, 0xF2);
	}
	return 0;
}
EXPORT_SYMBOL(pm8921_disable_input_current_limit);

int pm8921_set_max_battery_charge_current(int ma)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	return pm_chg_ibatmax_set(the_chip, ma);
}
EXPORT_SYMBOL(pm8921_set_max_battery_charge_current);

int pm8921_disable_source_current(bool disable)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	if (disable)
		pr_warn("current drawn from chg=0, battery provides current\n");
	return pm_chg_disable_pwrsrc(the_chip, disable, PWRSRC_DISABLED_BIT_KDRV);
}
EXPORT_SYMBOL(pm8921_disable_source_current);

int pm8921_regulate_input_voltage(int voltage)
{
	int rc;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	rc = pm_chg_vinmin_set(the_chip, voltage);

	if (rc == 0)
		the_chip->vin_min = voltage;

	return rc;
}

static void adjust_vdd_max_for_fastchg(struct pm8921_chg_chip *chip)
{
	int ichg_meas_ua, vbat_uv;
	int ichg_meas_ma;
	int adj_vdd_max_mv, programmed_vdd_max;
	int vbat_batt_terminal_uv;
	int vbat_batt_terminal_mv;
	int reg_loop;
	int delta_mv = 0;

	if (chip->rconn_mohm == 0) {
		pr_debug("Exiting as rconn_mohm is 0\n");
		return;
	}
	
	if (chip->is_bat_cool || chip->is_bat_warm) {
		pr_info("Exiting is_bat_cool = %d is_batt_warm = %d\n",
				chip->is_bat_cool, chip->is_bat_warm);
		return;
	}

	reg_loop = pm_chg_get_regulation_loop(chip);
	if (!(reg_loop & VDD_LOOP_ACTIVE_BIT)) {
		pr_debug("Exiting Vdd loop is not active reg loop=0x%x\n",
			reg_loop);
		return;
	}

	pm8921_bms_get_simultaneous_battery_voltage_and_current(&ichg_meas_ua,
								&vbat_uv);
	if (ichg_meas_ua >= 0) {
		pr_debug("Exiting ichg_meas_ua = %d > 0\n", ichg_meas_ua);
		return;
	}
	if (ichg_meas_ua <= ichg_threshold_ua) {
		pr_debug("Exiting ichg_meas_ua = %d < ichg_threshold_ua = %d\n",
					ichg_meas_ua, ichg_threshold_ua);
		return;
	}
	ichg_meas_ma = ichg_meas_ua / 1000;

	
	vbat_batt_terminal_uv = vbat_uv + ichg_meas_ma * chip->rconn_mohm;
	vbat_batt_terminal_mv = vbat_batt_terminal_uv/1000;
	pm_chg_vddmax_get(the_chip, &programmed_vdd_max);

	last_delta_mv = delta_mv =  chip->max_voltage_mv - vbat_batt_terminal_mv;
	pr_info("%s: rconn_mohm=%d, reg_loop=0x%x, vbat_uv=%d, ichg_ma=%d, "
			"vbat_terminal_mv=%d, delta_mv=%d\n",
			__func__, chip->rconn_mohm, reg_loop, vbat_uv, ichg_meas_ma,
			vbat_batt_terminal_mv, delta_mv);
	if (delta_mv > delta_threshold_mv && delta_mv <= 0) {
		pr_debug("skip delta_mv=%d since it is between %d and 0\n",
				delta_mv, delta_threshold_mv);
		return;
	}

	adj_vdd_max_mv = programmed_vdd_max + delta_mv;
	pr_debug("vdd_max needs to be changed by %d mv from %d to %d\n",
			delta_mv,
			programmed_vdd_max,
			adj_vdd_max_mv);

	if (adj_vdd_max_mv < chip->max_voltage_mv) {
		pr_debug("adj vdd_max lower than default max voltage\n");
		return;
	}

	if (adj_vdd_max_mv > (chip->max_voltage_mv + vdd_max_increase_mv))
		adj_vdd_max_mv = chip->max_voltage_mv + vdd_max_increase_mv;

	pr_info("%s: adjusting vdd_max_mv to %d from %d to make "
		"vbat_batt_termial_uv = %d to %d\n",
		__func__, adj_vdd_max_mv, programmed_vdd_max, vbat_batt_terminal_uv,
		chip->max_voltage_mv);
	pm_chg_vddmax_set(chip, adj_vdd_max_mv);
}

#define USB_OV_THRESHOLD_MASK  0x60
#define USB_OV_THRESHOLD_SHIFT  5
int pm8921_usb_ovp_set_threshold(enum pm8921_usb_ov_threshold ov)
{
	u8 temp;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	if (ov > PM_USB_OV_7V) {
		pr_err("limiting to over voltage threshold to 7volts\n");
		ov = PM_USB_OV_7V;
	}

	temp = USB_OV_THRESHOLD_MASK & (ov << USB_OV_THRESHOLD_SHIFT);

	return pm_chg_masked_write(the_chip, USB_OVP_CONTROL,
				USB_OV_THRESHOLD_MASK, temp);
}
EXPORT_SYMBOL(pm8921_usb_ovp_set_threshold);

#define USB_DEBOUNCE_TIME_MASK	0x06
#define USB_DEBOUNCE_TIME_SHIFT 1
int pm8921_usb_ovp_set_hystersis(enum pm8921_usb_debounce_time ms)
{
	u8 temp;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	if (ms > PM_USB_DEBOUNCE_80P5MS) {
		pr_err("limiting debounce to 80.5ms\n");
		ms = PM_USB_DEBOUNCE_80P5MS;
	}

	temp = USB_DEBOUNCE_TIME_MASK & (ms << USB_DEBOUNCE_TIME_SHIFT);

	return pm_chg_masked_write(the_chip, USB_OVP_CONTROL,
				USB_DEBOUNCE_TIME_MASK, temp);
}
EXPORT_SYMBOL(pm8921_usb_ovp_set_hystersis);

#define USB_OVP_DISABLE_MASK	0x80
int pm8921_usb_ovp_disable(int disable)
{
	u8 temp = 0;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	if (disable)
		temp = USB_OVP_DISABLE_MASK;

	return pm_chg_masked_write(the_chip, USB_OVP_CONTROL,
				USB_OVP_DISABLE_MASK, temp);
}

bool pm8921_is_battery_charging(int *source)
{
	int fsm_state, is_charging, dc_present, usb_present;

	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	fsm_state = pm_chg_get_fsm_state(the_chip);
	is_charging = is_battery_charging(fsm_state);
	if (is_charging == 0) {
		*source = PM8921_CHG_SRC_NONE;
		return is_charging;
	}

	if (source == NULL)
		return is_charging;

	
	dc_present = is_dc_chg_plugged_in(the_chip);
	usb_present = is_usb_chg_plugged_in(the_chip);

	if (dc_present && !usb_present)
		*source = PM8921_CHG_SRC_DC;

	if (usb_present && !dc_present)
		*source = PM8921_CHG_SRC_USB;

	if (usb_present && dc_present)
		*source = PM8921_CHG_SRC_DC;

	return is_charging;
}
EXPORT_SYMBOL(pm8921_is_battery_charging);

int pm8921_set_usb_power_supply_type(enum power_supply_type type)
{
#if 1  
	return 0;
#else
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	if (type < POWER_SUPPLY_TYPE_USB)
		return -EINVAL;

	the_chip->usb_psy.type = type;
	power_supply_changed(&the_chip->usb_psy);
	power_supply_changed(&the_chip->dc_psy);
	return 0;
#endif
}
EXPORT_SYMBOL_GPL(pm8921_set_usb_power_supply_type);

int pm8921_batt_temperature(void)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	return get_prop_batt_temp(the_chip);
}

static int is_ac_online(void)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	return is_usb_chg_plugged_in(the_chip) &&
				(pwr_src == HTC_PWR_SOURCE_TYPE_AC ||
							pwr_src == HTC_PWR_SOURCE_TYPE_9VAC ||
							pwr_src == HTC_PWR_SOURCE_TYPE_MHL_AC);
}

static void handle_usb_insertion_removal(struct pm8921_chg_chip *chip)
{
	int usb_present, rc = 0;
	int vbat_programmed = chip->max_voltage_mv;

	if (chip->lockup_lpm_wrkarnd) {
		rc = pm8921_apply_19p2mhz_kickstart(chip);
		if (rc)
			pr_err("Failed to apply kickstart rc=%d\n", rc);
	}

	if (pm_chg_failed_clear(chip, 1))
		pr_err("Failed to write CHG_FAILED_CLEAR bit\n");
	usb_present = is_usb_chg_plugged_in(chip);
	if (chip->usb_present ^ usb_present) {
		pr_info("vbus present change: %d -> %d\n",
		chip->usb_present, usb_present);
		notify_usb_of_the_plugin_event(usb_present);
		chip->usb_present = usb_present;

		
		if (chip->lockup_lpm_wrkarnd)
			pm8921_chg_bypass_bat_gone_debounce(chip, is_chg_on_bat(chip));

		if (usb_present) {
			htc_charger_event_notify(HTC_CHARGER_EVENT_VBUS_IN);
			is_cable_remove = false;
			schedule_delayed_work(&chip->unplug_check_work,
				round_jiffies_relative(msecs_to_jiffies
					(UNPLUG_CHECK_WAIT_PERIOD_MS)));
		} else {
			
			usb_target_ma = 0;
			pm_chg_disable_auto_enable(chip, 0, BATT_CHG_DISABLED_BIT_EOC);
			is_batt_full = false;
			is_ac_safety_timeout = false;
			htc_charger_event_notify(HTC_CHARGER_EVENT_VBUS_OUT);
			is_cable_remove = true;

			rc = pm_chg_vddmax_get(chip, &vbat_programmed);
			if (rc)
				pr_err("couldnt read vddmax rc = %d\n", rc);
			pr_info("Set vbatdet=%d after cable out\n",
					vbat_programmed);
			rc = pm_chg_vbatdet_set(chip, vbat_programmed);
			if (rc)
				pr_err("Failed to set vbatdet=%d rc=%d\n",
					chip->max_voltage_mv, rc);
		}
		is_batt_full_eoc_stop = false;
	}
	bms_notify_check(chip);
}

static void handle_stop_ext_usb_chg(struct pm8921_chg_chip *chip)
{
	if (chip->lockup_lpm_wrkarnd)
		
		pm8921_chg_bypass_bat_gone_debounce(chip, is_chg_on_bat(chip));

	if (chip->ext_usb == NULL) {
		pr_debug("ext_usb charger not registered.\n");
		return;
	}

	if (!chip->ext_usb_charging) {
		pr_debug("ext_usb charger already not charging.\n");
		return;
	}

	if(chip->ext_usb)
	{
		if(chip->ext_usb->stop_charging)
		{
			chip->ext_usb->stop_charging(chip->ext_usb->ctx);
			chip->ext_usb_charging = false;
			chip->ext_usb_charge_done = false;
		}
		return;
	}

}

static void handle_start_ext_usb_chg(struct pm8921_chg_chip *chip)
{
	int usb_present;
	int batt_present;
	int batt_temp_ok;
	int vbat_ov;
	unsigned long delay =
		round_jiffies_relative(msecs_to_jiffies(EOC_CHECK_PERIOD_MS));

	
	if (chip->lockup_lpm_wrkarnd)
		pm8921_chg_bypass_bat_gone_debounce(chip, is_chg_on_bat(chip));

	if (chip->ext_usb == NULL) {
		pr_debug("ext_usb charger not registered.\n");
		return;
	}

	if (chip->ext_usb_charging) {
		pr_debug("ext_usb charger already charging.\n");
		return;
	}

	if(chip->ext_usb)
	{
		if(chip->ext_usb->start_charging)
		{
			chip->ext_usb->start_charging(chip->ext_usb->ctx);
			chip->ext_usb_charging = true;
			chip->ext_usb_charge_done = false;

			
			if (!flag_disable_wakelock)
				wake_lock(&chip->eoc_wake_lock);
			schedule_delayed_work(&chip->eoc_work, delay);
		}
		return;
	}

	usb_present = is_usb_chg_plugged_in(chip);
	batt_present = pm_chg_get_rt_status(chip, BATT_INSERTED_IRQ);
	batt_temp_ok = pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ);
	vbat_ov = pm_chg_get_rt_status(chip, VBAT_OV_IRQ);

	if (!usb_present) {
		pr_warn("%s. usb not present.\n", __func__);
		return;
	}

	if (!batt_present) {
		pr_warn("%s. battery not present.\n", __func__);
		return;
	}
	if (!batt_temp_ok) {
		pr_warn("%s. battery temperature not ok.\n", __func__);
		return;
	}
	if (vbat_ov) {
		pr_warn("%s. battery over voltage.\n", __func__);
		return;
	}

	chip->ext_usb->start_charging(chip->ext_usb->ctx);
	chip->ext_usb_charging = true;
	chip->ext_usb_charge_done = false;
	
	schedule_delayed_work(&chip->eoc_work, delay);
	if (!flag_disable_wakelock)
		wake_lock(&chip->eoc_wake_lock);
}

static void handle_stop_ext_chg(struct pm8921_chg_chip *chip)
{
	if (chip->lockup_lpm_wrkarnd)
		
		pm8921_chg_bypass_bat_gone_debounce(chip, is_chg_on_bat(chip));

	if (chip->ext == NULL) {
		pr_debug("external charger not registered.\n");
		return;
	}

	if (!chip->ext_charging) {
		pr_debug("already not charging.\n");
		return;
	}

	chip->ext->stop_charging(chip->ext->ctx);
	chip->ext_charging = false;
	chip->ext_charge_done = false;
}

static void handle_start_ext_chg(struct pm8921_chg_chip *chip)
{
	int dc_present;
	int batt_present;
	int batt_temp_ok;
	int vbat_ov;
	int batfet;
	unsigned long delay =
		round_jiffies_relative(msecs_to_jiffies(EOC_CHECK_PERIOD_MS));

	
	if (chip->lockup_lpm_wrkarnd)
		pm8921_chg_bypass_bat_gone_debounce(chip, is_chg_on_bat(chip));

	if (chip->ext == NULL) {
		pr_debug("external charger not registered.\n");
		return;
	}

	if (chip->ext_charging) {
		pr_debug("already charging.\n");
		return;
	}

	dc_present = is_dc_chg_plugged_in(chip);
	batt_present = pm_chg_get_rt_status(chip, BATT_INSERTED_IRQ);
	batt_temp_ok = pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ);
	vbat_ov = pm_chg_get_rt_status(chip, VBAT_OV_IRQ);
	batfet = pm_chg_get_rt_status(chip, BATFET_IRQ);

	if (!dc_present) {
		pr_warn("%s. dc not present.\n", __func__);
		return;
	}
	if (!batt_present) {
		pr_warn("%s. battery not present.\n", __func__);
		return;
	}
	if (!batt_temp_ok) {
		pr_warn("%s. battery temperature not ok.\n", __func__);
		return;
	}
	if (vbat_ov) {
		pr_warn("%s. battery over voltage.\n", __func__);
		return;
	}
	if (!batfet) {
		pr_warn("%s. battery FET not closed.\n", __func__);
		return;
	}

	chip->ext->start_charging(chip->ext->ctx);
	chip->ext_charging = true;
	chip->ext_charge_done = false;
	
	schedule_delayed_work(&chip->eoc_work, delay);
	if (!flag_disable_wakelock)
		wake_lock(&chip->eoc_wake_lock);
}

static void handle_dc_removal_insertion(struct pm8921_chg_chip *chip)
{
	int dc_present;

	dc_present = is_dc_chg_plugged_in(chip);
	if (chip->dc_present ^ dc_present) {
		chip->dc_present = dc_present;
		
	}
	bms_notify_check(chip);
}

static int pm_chg_turn_off_ovp_fet(struct pm8921_chg_chip *chip, u16 ovptestreg)
{
	u8 temp;
	int err = 0;
	err = pm_chg_write(chip, ovptestreg, 0x30);
	if (err) {
		pr_err("Error %d writing 0x%2x to addr 0x%2x\n",
				err, 0x30, ovptestreg);
		return err;
	}
	err = pm8xxx_readb(chip->dev->parent, ovptestreg, &temp);
	if (err) {
		pr_err("Error %d reading addr 0x%x\n", err, ovptestreg);
		return err;
	}
	
	temp |= 0x81;
	err = pm_chg_write(chip, ovptestreg, temp);
	if (err) {
		pr_err("Error %d writing 0x%2x to addr 0x%2x\n",
				err, temp, ovptestreg);
		return err;
	}
	return 0;
}

static int pm_chg_turn_on_ovp_fet(struct pm8921_chg_chip *chip, u16 ovptestreg)
{
	u8 temp;
	int err = 0;

	err = pm_chg_write(chip, ovptestreg, 0x30);
	if (err) {
		pr_err("Error %d writing 0x%2x to addr 0x%2x\n",
				err, 0x30, ovptestreg);
		return err;
	}
	err = pm8xxx_readb(chip->dev->parent, ovptestreg, &temp);
	if (err) {
		pr_err("Error %d reading addr 0x%x\n", err, ovptestreg);
		return err;
	}
	
	temp &= 0xFE;
	temp |= 0x80;
	err = pm_chg_write(chip, ovptestreg, temp);
	if (err) {
		pr_err("Error %d writing 0x%2x to addr 0x%2x\n",
				err, temp, ovptestreg);
		return err;
	}

	return 0;
}

static void turn_off_ovp_fet(struct pm8921_chg_chip *chip, u16 ovptestreg)
{
	int i, rc;
	for (i = 0; i < 5; i++) {
		rc = pm_chg_turn_off_ovp_fet(chip, ovptestreg);
		if (!rc)
			break;
	}
	if (i > 0)
		pr_info("%s: retry = %d\n", __func__, i);
	return;
}

static void turn_on_ovp_fet(struct pm8921_chg_chip *chip, u16 ovptestreg)
{
	int i, rc;
	for (i = 0; i < 5; i++) {
		rc = pm_chg_turn_on_ovp_fet(chip, ovptestreg);
		if (!rc)
			break;
	}
	if (i > 0)
		pr_info("%s: retry = %d\n", __func__, i);
	return;
}

static int param_open_ovp_counter = 10;
module_param(param_open_ovp_counter, int, 0644);

#define USB_ACTIVE_BIT BIT(5)
#define DC_ACTIVE_BIT BIT(6)
static int is_active_chg_plugged_in(struct pm8921_chg_chip *chip,
						u8 active_chg_mask)
{
	if (active_chg_mask & USB_ACTIVE_BIT)
		return pm_chg_get_rt_status(chip, USBIN_VALID_IRQ);
	else if (active_chg_mask & DC_ACTIVE_BIT)
		return pm_chg_get_rt_status(chip, DCIN_VALID_IRQ);
	else
		return 0;
}

#define WRITE_BANK_4		0xC0
#define OVP_DEBOUNCE_TIME 0x06
static void unplug_ovp_fet_open_worker(struct work_struct *work)
{
	struct pm8921_chg_chip *chip = container_of(work,
				struct pm8921_chg_chip,
				unplug_ovp_fet_open_work);
	int chg_gone = 0, active_chg_plugged_in = 0, count = 0, is_wlc_remove = 0;
	u8 active_mask = 0;
	u16 ovpreg, ovptestreg;

	if(chip->disable_reverse_boost_check)
		return;

	wake_lock(&chip->unplug_ovp_fet_open_wake_lock);
	pr_info("%s:Start\n", __func__);
	if (chip->wlc_tx_gpio)
		is_wlc_remove = gpio_get_value(chip->wlc_tx_gpio);
	if (is_usb_chg_plugged_in(chip) &&
		(chip->active_path & USB_ACTIVE_BIT)) {
		ovpreg = USB_OVP_CONTROL;
		ovptestreg = USB_OVP_TEST;
		active_mask = USB_ACTIVE_BIT;
	} else if (is_dc_chg_plugged_in(chip) &&
		(chip->active_path & DC_ACTIVE_BIT)) {
		
		if (chip->wlc_tx_gpio && !is_wlc_remove) {
			pr_debug("%s: Skip WA since WLC pad is existed, is_wlc_remove=%d\n",
						__func__, is_wlc_remove);
			goto finish_due_to_no_cable;
		}
		ovpreg = DC_OVP_CONTROL;
		ovptestreg = DC_OVP_TEST;
		active_mask = DC_ACTIVE_BIT;
	} else {
		goto finish_due_to_no_cable;
	}

	while (count++ < param_open_ovp_counter) {
		pm_chg_masked_write(chip, ovpreg, OVP_DEBOUNCE_TIME, 0x0);
		usleep(10);
		active_chg_plugged_in = is_active_chg_plugged_in(chip, active_mask);
		chg_gone = pm_chg_get_rt_status(chip, CHG_GONE_IRQ);
		pr_debug("OVP FET count=%d chg_gone=%d, active_valid=%d\n",
					count, chg_gone, active_chg_plugged_in);
		
		if (chg_gone == 1 && active_chg_plugged_in == 1) {
			pr_debug("since chg_gone = 1 disabling ovp_fet for 20 msecs\n");
			turn_off_ovp_fet(chip, ovptestreg);

			msleep(20);

			turn_on_ovp_fet(chip, ovptestreg);
		} else {
			
			break;
		}
	}
	pm_chg_masked_write(chip, ovpreg, OVP_DEBOUNCE_TIME, 0x2);
finish_due_to_no_cable:
	pr_info("%s:Exiting,count=%d,chg_gone=%d,active_valid=%d\n",
				__func__, count, chg_gone, active_chg_plugged_in);
	wake_unlock(&chip->unplug_ovp_fet_open_wake_lock);
	return;
}

static int find_usb_ma_value(int value)
{
	int i;

	for (i = ARRAY_SIZE(usb_ma_table) - 1; i >= 0; i--) {
		if (value >= usb_ma_table[i].usb_ma)
			break;
	}

	return i;
}

static void decrease_usb_ma_value(int *value)
{
	int i;

	if (value) {
		i = find_usb_ma_value(*value);
		if (i > 0)
			i--;
		if (i >= 0)
			*value = usb_ma_table[i].usb_ma;
	}
}

static void increase_usb_ma_value(int *value)
{
	int i;

	if (value) {
		i = find_usb_ma_value(*value);

		if (i < (ARRAY_SIZE(usb_ma_table) - 1))
			i++;
		*value = usb_ma_table[i].usb_ma;
	}
}

static void vin_collapse_check_worker(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct pm8921_chg_chip *chip = container_of(dwork,
			struct pm8921_chg_chip, vin_collapse_check_work);

	
	if (is_usb_chg_plugged_in(chip) &&
		usb_target_ma > USB_WALL_THRESHOLD_MA) {
		
		decrease_usb_ma_value(&usb_target_ma);
		
		__pm8921_charger_vbus_draw(USB_WALL_THRESHOLD_MA);
		pr_debug("usb_now=%d, usb_target = %d\n",
				USB_WALL_THRESHOLD_MA, usb_target_ma);
	} else {
		cable_detection_vbus_irq_handler();
	}
}

#define VIN_MIN_COLLAPSE_CHECK_MS	50
static irqreturn_t usbin_valid_irq_handler(int irq, void *data)
{
	pr_info("%s: usb_target_ma=%d\n", __func__, usb_target_ma);
	if (usb_target_ma) {
		schedule_delayed_work(&the_chip->vin_collapse_check_work,
				      round_jiffies_relative(msecs_to_jiffies
						(VIN_MIN_COLLAPSE_CHECK_MS)));
	} else {
		
		cable_detection_vbus_irq_handler();
	}

	if (0)
		handle_usb_insertion_removal(data);
	return IRQ_HANDLED;
}

#define CABLE_IN_DETECT_DEBOUNCE_MS	(800)
static irqreturn_t cable_in_handler(int irq, void *data)
{
	if (!the_chip) {
		pr_warn("%s: called before init\n", __func__);
		return IRQ_HANDLED;
	}

	schedule_delayed_work(&the_chip->ovp_check_work,
	      round_jiffies_relative(msecs_to_jiffies
				     (CABLE_IN_DETECT_DEBOUNCE_MS)));
	return IRQ_HANDLED;
}

static void update_ovp_uvp_state_by_cable_irq(int ov, int v, int uv)
{
	int cable_in_irq;

	cable_in_irq = gpio_get_value(the_chip->cable_in_gpio);
	pr_info("%s, ovp=%d, uvp=%d, cable_in_irq=%d, ov=%d, v=%d, uv=%d\n",
		__func__, ovp, uvp, cable_in_irq, ov, v, uv);
	if ( ov && !v && !cable_in_irq) {
		if (!ovp) {
			ovp = 1;
			pr_info("OVP: 0 -> 1, USB_Valid: %d\n", v);
			htc_charger_event_notify(HTC_CHARGER_EVENT_OVP);
		}
	} else if ( !ov && !v && uv && !cable_in_irq) {
		if (!uvp) {
			uvp = 1;
			pr_info("UVP: 0 -> 1, USB_Valid: %d\n", v);
		}
	} else {
		if (ovp) {
			ovp = 0;
			pr_info("OVP: 1 -> 0, USB_Valid: %d\n", v);
			htc_charger_event_notify(HTC_CHARGER_EVENT_OVP_RESOLVE);
		}
		if (uvp) {
			uvp = 0;
			pr_info("UVP: 1 -> 0, USB_Valid: %d\n", v);
		}
	}
}

static void update_ovp_uvp_state(int ov, int v, int uv)
{
	if ( ov && !v && !uv) {
		if (!ovp) {
			ovp = 1;
			pr_info("OVP: 0 -> 1, USB_Valid: %d\n", v);
			htc_charger_event_notify(HTC_CHARGER_EVENT_OVP);
		}
	} else if ( !ov && !v && uv) {
		if (!uvp) {
			uvp = 1;
			pr_info("UVP: 0 -> 1, USB_Valid: %d\n", v);
		}
	} else {
		if (ovp) {
			ovp = 0;
			pr_info("OVP: 1 -> 0, USB_Valid: %d\n", v);
			htc_charger_event_notify(HTC_CHARGER_EVENT_OVP_RESOLVE);
		}
		if (uvp) {
			uvp = 0;
			pr_info("UVP: 1 -> 0, USB_Valid: %d\n", v);
		}
	}
}

static irqreturn_t usbin_ov_irq_handler(int irq, void *data)
{
	int ov, uv, v;
	ov = pm_chg_get_rt_status(the_chip, USBIN_OV_IRQ);
	v = pm_chg_get_rt_status(the_chip, USBIN_VALID_IRQ);
	uv = pm_chg_get_rt_status(the_chip, USBIN_UV_IRQ);
	pr_debug("%d -> %d [%d,%d,%d]\n",
					usbin_ov_irq_state, ov, ov, v, uv);
	usbin_ov_irq_state = ov;

	if (the_chip->cable_in_irq)
		update_ovp_uvp_state_by_cable_irq(ov, v, uv);
	else
		update_ovp_uvp_state(ov, v, uv);
	return IRQ_HANDLED;
}

static irqreturn_t batt_inserted_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	int status;

	status = pm_chg_get_rt_status(chip, BATT_INSERTED_IRQ);
	schedule_work(&chip->battery_id_valid_work);
	handle_start_ext_chg(chip);
	handle_start_ext_usb_chg(chip);
	pr_debug("battery present=%d", status);
	
	return IRQ_HANDLED;
}

static irqreturn_t vbatdet_low_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	int high_transition, rc = 0;


	high_transition = pm_chg_get_rt_status(chip, VBATDET_LOW_IRQ);

	if(chip->ext_usb)
	{
		if (high_transition)
		{
			queue_delayed_work(ext_charger_wq, &ext_usb_vbat_low_task, 0);
		}

		pr_info("%s, high_transition:%d\n", __func__, high_transition);
		return IRQ_HANDLED;
	}

	if (high_transition) {
		handle_start_ext_usb_chg(chip);
		
		pr_info("%s: Set vbatdet=%d before recharge is started\n",
				__func__, PM8921_CHG_VBATDET_MAX);
		rc = pm_chg_vbatdet_set(chip, PM8921_CHG_VBATDET_MAX);
		if (rc)
			pr_err("Failed to set vbatdet=%d rc=%d\n",
					PM8921_CHG_VBATDET_MAX, rc);
		
		pm_chg_disable_auto_enable(chip, 0, BATT_CHG_DISABLED_BIT_EOC);
		pr_info("batt fell below resume voltage %s\n",
			batt_charging_disabled ? "" : "charger enabled (recharging)");
	} else {
		pr_info("vbatdet_low = %d, fsm_state=%d\n", high_transition,
			pm_chg_get_fsm_state(data));
	}

	return IRQ_HANDLED;
}

static irqreturn_t usbin_uv_irq_handler(int irq, void *data)
{
	int ov, uv, v;
	ov = pm_chg_get_rt_status(the_chip, USBIN_OV_IRQ);
	v = pm_chg_get_rt_status(the_chip, USBIN_VALID_IRQ);
	uv = pm_chg_get_rt_status(the_chip, USBIN_UV_IRQ);
	pr_debug("%d -> %d [%d,%d,%d]\n",
					usbin_uv_irq_state, uv, ov, v, uv);
	usbin_uv_irq_state = uv;
	update_ovp_uvp_state(ov, v, uv);
	return IRQ_HANDLED;
}

static irqreturn_t vbat_ov_irq_handler(int irq, void *data)
{
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t chgwdog_irq_handler(int irq, void *data)
{
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t vcp_irq_handler(int irq, void *data)
{
	pr_debug("state_changed_to=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t atcdone_irq_handler(int irq, void *data)
{
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t atcfail_irq_handler(int irq, void *data)
{
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t chgdone_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;

	pr_debug("state_changed_to=%d\n", pm_chg_get_fsm_state(data));


	if(chip->ext_usb)
	{
		queue_delayed_work(ext_charger_wq, &ext_usb_chgdone_task, 0);
		return IRQ_HANDLED;
	}

	handle_stop_ext_chg(chip);


	bms_notify_check(chip);
	htc_gauge_event_notify(HTC_GAUGE_EVENT_EOC_STOP_CHG);

	return IRQ_HANDLED;
}

static irqreturn_t chgfail_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	int ret;

	if (!is_ac_online() || flag_keep_charge_on || flag_pa_recharge) {
		pr_info("%s: write CHG_FAILED_CLEAR bit\n", __func__);
		ret = pm_chg_failed_clear(chip, 1);
		if (ret)
			pr_err("Failed to write CHG_FAILED_CLEAR bit\n");
	} else {
		if ((chip->safety_time > SAFETY_TIME_MAX_LIMIT) &&
				!is_ac_safety_timeout_twice) {
			is_ac_safety_timeout_twice = true;
			pr_info("%s: write CHG_FAILED_CLEAR bit "
					"due to safety time is twice\n", __func__);
			ret = pm_chg_failed_clear(chip, 1);
			if (ret)
				pr_err("Failed to write CHG_FAILED_CLEAR bit\n");
		} else {
			is_ac_safety_timeout = true;
			pr_err("%s: batt_present=%d, batt_temp_ok=%d, state_changed_to=%d\n",
					__func__, get_prop_batt_present(chip),
					pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ),
					pm_chg_get_fsm_state(data));
			htc_charger_event_notify(HTC_CHARGER_EVENT_SAFETY_TIMEOUT);
		}
	}
	return IRQ_HANDLED;
}

static irqreturn_t chgstate_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;

	pr_debug("state_changed_to=%d\n", pm_chg_get_fsm_state(data));
	bms_notify_check(chip);

	return IRQ_HANDLED;
}

static int param_vin_disable_counter = 5;
module_param(param_vin_disable_counter, int, 0644);

static void attempt_reverse_boost_fix(struct pm8921_chg_chip *chip,
							int count, int usb_ma)
{
	if (usb_ma)
		__pm8921_charger_vbus_draw(500);
	pr_debug(" count = %d iusb=500mA\n", count);
	disable_input_voltage_regulation(chip);
	pr_debug(" count = %d disable_input_regulation\n", count);

	msleep(20);

	pr_debug(" count = %d end sleep 20ms chg_gone=%d, usb_valid = %d\n",
								count,
								pm_chg_get_rt_status(chip, CHG_GONE_IRQ),
								is_usb_chg_plugged_in(chip));
	pr_debug(" count = %d restoring input regulation and usb_ma = %d\n",
								count, usb_ma);
	enable_input_voltage_regulation(chip);
	if (usb_ma)
		__pm8921_charger_vbus_draw(usb_ma);
}

#define VIN_ACTIVE_BIT BIT(0)
#define UNPLUG_WRKARND_RESTORE_WAIT_PERIOD_US 200
#define VIN_MIN_INCREASE_MV 100
#define CONSECUTIVE_TRIAL_COUNT_MAX	(20)
static void unplug_check_worker(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct pm8921_chg_chip *chip = container_of(dwork,
				struct pm8921_chg_chip, unplug_check_work);
	u8 reg_loop, active_path;
	int rc, ibat, active_chg_plugged_in, usb_ma;
	int chg_gone = 0, is_wlc_remove = 0;
	static int rb_trial_count = 0;
	static int ovp_trial_count = 0;

	if(chip->disable_reverse_boost_check)
		return;

	reg_loop = 0;
	rc = pm8xxx_readb(chip->dev->parent, PBL_ACCESS1, &active_path);
	if (rc) {
		pr_err("Failed to read PBL_ACCESS1 rc=%d\n", rc);
		return;
	}
	chip->active_path = active_path;

	active_chg_plugged_in = is_active_chg_plugged_in(chip, active_path);
	pr_debug("active_path = 0x%x, active_chg_plugged_in = %d\n",
			active_path, active_chg_plugged_in);
	if (chip->wlc_tx_gpio)
		is_wlc_remove = gpio_get_value(chip->wlc_tx_gpio);
	if (active_path & USB_ACTIVE_BIT) {
		pr_debug("USB charger active\n");

		pm_chg_iusbmax_get(chip, &usb_ma);
		#if 0
		if (usb_ma == 500 && !usb_target_ma) {
			pr_info("Stopping Unplug Check Worker USB == 500mA\n");
			rb_trial_count = ovp_trial_count = 0;
			disable_input_voltage_regulation(chip);
			return;
		}
		#endif

		if (usb_ma <= 100) {
			pr_info(
				"Unenumerated or suspended usb_ma = %d skip\n",
				usb_ma);
			goto check_again_later;
		}
	} else if (active_path & DC_ACTIVE_BIT) {
		pr_debug("DC charger active\n");
		if (!chip->dc_unplug_check)
			return;
		
		if (chip->wlc_tx_gpio && !is_wlc_remove) {
			pr_debug("%s: Skip WA since WLC pad is existed, is_wlc_remove=%d\n",
						__func__, is_wlc_remove);
			goto check_again_later;
		}
	} else {
		
		if (!(is_usb_chg_plugged_in(chip))
				&& !(is_dc_chg_plugged_in(chip))) {
			pr_info("Stopping Unplug Check - chargers are removed"
				"reg_loop = %d, fsm = %d ibat = %d "
				"(rb_trial_count=%d ovp_trial_count=%d)\n",
				pm_chg_get_regulation_loop(chip),
				pm_chg_get_fsm_state(chip),
				get_prop_batt_current(chip),
				rb_trial_count,
				ovp_trial_count
				);
			rb_trial_count = ovp_trial_count = 0;
			if (chip->lockup_lpm_wrkarnd) {
				rc = pm8921_apply_19p2mhz_kickstart(chip);
				if (rc)
					pr_err("Failed kickstart rc=%d\n", rc);

				if (chip->final_kickstart) {
					chip->final_kickstart = false;
					goto check_again_later;
				}
			}
		}
		return;
	}

	chip->final_kickstart = true;

	if (active_path & USB_ACTIVE_BIT) {
		reg_loop = pm_chg_get_regulation_loop(chip);
		pr_debug("reg_loop=0x%x usb_ma = %d\n", reg_loop, usb_ma);
		if ((reg_loop & VIN_ACTIVE_BIT) &&
			(usb_ma > USB_WALL_THRESHOLD_MA)) {
			decrease_usb_ma_value(&usb_ma);
			usb_target_ma = usb_ma;
			
			__pm8921_charger_vbus_draw(usb_ma);
			pr_debug("usb_now=%d, usb_target = %d\n",
				usb_ma, usb_target_ma);
		}
	}

	reg_loop = pm_chg_get_regulation_loop(chip);
#if 0
	pr_debug("reg_loop=0x%x usb_ma = %d\n", reg_loop, usb_ma);
#endif

	ibat = get_prop_batt_current(chip);
	if (reg_loop & VIN_ACTIVE_BIT) {

		pr_debug("ibat = %d fsm = %d reg_loop = 0x%x\n",
				ibat, pm_chg_get_fsm_state(chip), reg_loop);
		if (ibat > 0) {
			int count = 0;
			while (count++ < param_vin_disable_counter
					&& active_chg_plugged_in == 1) {
				if (active_path & USB_ACTIVE_BIT)
					attempt_reverse_boost_fix(chip,
								count, usb_ma);
				else
					attempt_reverse_boost_fix(chip,
								count, 0);
				active_chg_plugged_in
					= is_active_chg_plugged_in(chip,
						active_path);
				pr_debug("active_chg_plugged_in = %d\n",
						active_chg_plugged_in);
				if(!active_chg_plugged_in)
					pr_info("%s: cable out by vin disable, count:%d\n",
							__func__, count);
			}
			rb_trial_count++;
			if (rb_trial_count > CONSECUTIVE_TRIAL_COUNT_MAX) {
				pr_info("too much rb_trial_count=%d\n", rb_trial_count);
				rb_trial_count = 0;
			}
		}
	}

	active_chg_plugged_in = is_active_chg_plugged_in(chip, active_path);
	pr_debug("active_path = 0x%x, active_chg = %d\n",
			active_path, active_chg_plugged_in);
	chg_gone = pm_chg_get_rt_status(chip, CHG_GONE_IRQ);

	if (chg_gone == 1 && active_chg_plugged_in == 1) {
		pr_debug("chg_gone=%d, active_chg_plugged_in = %d\n",
					chg_gone, active_chg_plugged_in);
		ovp_trial_count++;
		if (ovp_trial_count > CONSECUTIVE_TRIAL_COUNT_MAX) {
			pr_info("too much ovp_trial_count=%d\n", ovp_trial_count);
			ovp_trial_count = 0;
		}
		schedule_work(&chip->unplug_ovp_fet_open_work);
	}

	if (!(reg_loop & VIN_ACTIVE_BIT) && (active_path & USB_ACTIVE_BIT) &&
			(usb_target_ma > USB_WALL_THRESHOLD_MA)) {
		
		if (usb_ma < usb_target_ma) {
			increase_usb_ma_value(&usb_ma);
			__pm8921_charger_vbus_draw(usb_ma);
			pr_debug("usb_now=%d, usb_target = %d\n",
					usb_ma, usb_target_ma);
		} else {
			usb_target_ma = usb_ma;
		}
	}

check_again_later:
	
	schedule_delayed_work(&chip->unplug_check_work,
		      round_jiffies_relative(msecs_to_jiffies
				(UNPLUG_CHECK_WAIT_PERIOD_MS)));
}

static irqreturn_t loop_change_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;

	pr_info("fsm_state=%d reg_loop=0x%x\n",
		pm_chg_get_fsm_state(data),
		pm_chg_get_regulation_loop(data));

	schedule_delayed_work(&chip->unplug_check_work,
		round_jiffies_relative(msecs_to_jiffies(0)));
	return IRQ_HANDLED;
}

static irqreturn_t fastchg_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	int high_transition;

	high_transition = pm_chg_get_rt_status(chip, FASTCHG_IRQ);
	if (high_transition && !delayed_work_pending(&chip->eoc_work)) {
		if (!flag_disable_wakelock)
			wake_lock(&chip->eoc_wake_lock);
		schedule_delayed_work(&chip->eoc_work,
				      round_jiffies_relative(msecs_to_jiffies
							(EOC_CHECK_PERIOD_MS)));
	}
	bms_notify_check(chip);
	return IRQ_HANDLED;
}

static irqreturn_t trklchg_irq_handler(int irq, void *data)
{
	
	return IRQ_HANDLED;
}

static irqreturn_t batt_removed_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	int status;

	status = pm_chg_get_rt_status(chip, BATT_REMOVED_IRQ);
	if (chip->is_embeded_batt) {
		pr_info("%s: Skip it due to embeded battery, present=%d\n",
					__func__, !status);
		return IRQ_HANDLED;
	}
	if (chip->mbat_in_gpio
			&& (gpio_get_value(chip->mbat_in_gpio) == 0)) {
		pr_info("%s: Battery is still existed, present=%d\n",
					__func__, !status);
		return IRQ_HANDLED;
	}
	pr_info("%s: battery present=%d FSM=%d", __func__, !status,
					 pm_chg_get_fsm_state(data));
	handle_stop_ext_usb_chg(chip);
	handle_stop_ext_chg(chip);
	
	if (status)
		htc_gauge_event_notify(HTC_GAUGE_EVENT_BATT_REMOVED);
	return IRQ_HANDLED;
}

static irqreturn_t batttemp_hot_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	pr_info("Battery hot\n");

	pr_debug("Batt hot fsm_state=%d\n", pm_chg_get_fsm_state(data));

	handle_stop_ext_chg(chip);

	return IRQ_HANDLED;
}

#define COMP_OVR_CHG_HOT	0x93
#define COMP_OVR_CHG_NOTHOT	0x92
#define MIN_PMIC_DIE_TEMP_FOR_CHGHOT_MILLIDEGC 80000
static void chghot_work(struct work_struct *work)
{
	int rc;
	struct pm8xxx_adc_chan_result result;
	struct pm8921_chg_chip *chip = container_of(work,
				struct pm8921_chg_chip, chghot_work);

	
	rc = pm8xxx_adc_read(CHANNEL_DIE_TEMP, &result);
	if (rc) {
		pr_err("error reading batt id channel = %d, rc = %d\n",
					chip->vbat_channel, rc);
		return;
	}
	pr_info("pmic die phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);

	if (result.physical < MIN_PMIC_DIE_TEMP_FOR_CHGHOT_MILLIDEGC) {
		pr_info("Spurious CHGHOT irq when pmic die = %lld milliDegC\n",
				result.physical);

		rc = pm_chg_write(chip, COMPARATOR_OVERRIDE,
				COMP_OVR_CHG_NOTHOT);
		if (rc < 0) {
			pr_err("Error %d writing %d to addr %d\n", rc,
				COMP_OVR_CHG_NOTHOT,
				COMPARATOR_OVERRIDE);
		}
	}
}

static irqreturn_t chghot_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	pr_info("Chg hot fsm_state=%d\n", pm_chg_get_fsm_state(data));
	schedule_work(&chip->chghot_work);
	return IRQ_HANDLED;
}

static irqreturn_t batttemp_cold_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	pr_info("Battery cold\n");

	pr_debug("Batt cold fsm_state=%d\n", pm_chg_get_fsm_state(data));

	handle_stop_ext_chg(chip);

	return IRQ_HANDLED;
}

static irqreturn_t chg_gone_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;
	u8 reg;
	int rc, chg_gone, usb_chg_plugged_in, dc_chg_plugged_in;

	usb_chg_plugged_in = is_usb_chg_plugged_in(chip);
	dc_chg_plugged_in = is_dc_chg_plugged_in(chip);
	chg_gone = pm_chg_get_rt_status(chip, CHG_GONE_IRQ);

	pr_info("chg_gone=%d, usb_valid=%d, dc_valid=%d, fsm=%d\n",
			chg_gone, usb_chg_plugged_in, dc_chg_plugged_in,
			pm_chg_get_fsm_state(data));
	rc = pm8xxx_readb(chip->dev->parent, CHG_CNTRL_3, &reg);
	if (rc)
		pr_err("Failed to read CHG_CNTRL_3 rc=%d\n", rc);

	if (reg & CHG_USB_SUSPEND_BIT)
		return IRQ_HANDLED;
	schedule_work(&chip->unplug_ovp_fet_open_work);

	return IRQ_HANDLED;
}

static irqreturn_t bat_temp_ok_irq_handler(int irq, void *data)
{
	int bat_temp_ok;
	struct pm8921_chg_chip *chip = data;

	
	if(the_chip->ext_usb)
	{
		pr_info("%s\n", __func__);
		queue_delayed_work(ext_charger_wq, &ext_usb_temp_task, HZ/200);
		return IRQ_HANDLED;
	}

	bat_temp_ok = pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ);
	if (bat_temp_ok_prev == bat_temp_ok) {
		pr_info("batt_temp_ok=%d same as previous one so skip it this time\n",
				 bat_temp_ok);
		return IRQ_HANDLED;
	} else {
		pr_info("batt_temp_ok=%d, bat_temp_ok_prev=%d, FSM=%d\n",
				 bat_temp_ok, bat_temp_ok_prev, pm_chg_get_fsm_state(data));
		bat_temp_ok_prev = bat_temp_ok;
	}

	if (bat_temp_ok) {
		handle_start_ext_chg(chip);
	} else {
		handle_stop_ext_chg(chip);
	}

	bms_notify_check(chip);

	htc_gauge_event_notify(HTC_GAUGE_EVENT_TEMP_ZONE_CHANGE);

	return IRQ_HANDLED;
}

static irqreturn_t coarse_det_low_irq_handler(int irq, void *data)
{
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t vdd_loop_irq_handler(int irq, void *data)
{
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t vreg_ov_irq_handler(int irq, void *data)
{
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t vbatdet_irq_handler(int irq, void *data)
{
	pr_debug("fsm_state=%d\n", pm_chg_get_fsm_state(data));
	return IRQ_HANDLED;
}

static irqreturn_t batfet_irq_handler(int irq, void *data)
{
	pr_debug("vreg ov\n");
	
	return IRQ_HANDLED;
}

static irqreturn_t dcin_valid_irq_handler(int irq, void *data)
{
#if 1
	pr_info("%s\n", __func__);
	cable_detection_vbus_irq_handler();
#else
	struct pm8921_chg_chip *chip = data;

	pm8921_disable_source_current(true); 

	handle_dc_removal_insertion(chip);
	handle_start_ext_chg(chip);
#endif
	return IRQ_HANDLED;
}

static irqreturn_t dcin_ov_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;

	pm8921_disable_source_current(false); 

	handle_dc_removal_insertion(chip);
	handle_stop_ext_chg(chip);
	return IRQ_HANDLED;
}

static irqreturn_t dcin_uv_irq_handler(int irq, void *data)
{
	struct pm8921_chg_chip *chip = data;

	pm8921_disable_source_current(false); 
	handle_stop_ext_chg(chip);

	return IRQ_HANDLED;
}

static void dump_irq_rt_status(void)
{
	pr_info("[irq1] %d%d%d%d %d%d%d%d %d%d%d%d %d%d%d "
			"[irq2] %d%d%d%d %d%d%d%d %d%d%d%d %d%d%d\n",
		
		pm_chg_get_rt_status(the_chip, USBIN_VALID_IRQ),
		pm_chg_get_rt_status(the_chip, USBIN_OV_IRQ),
		pm_chg_get_rt_status(the_chip, USBIN_UV_IRQ),
		pm_chg_get_rt_status(the_chip, VBAT_OV_IRQ),
		
		pm_chg_get_rt_status(the_chip, BATT_INSERTED_IRQ),
		pm_chg_get_rt_status(the_chip, BATT_REMOVED_IRQ),
		pm_chg_get_rt_status(the_chip, VBATDET_LOW_IRQ),
		pm_chg_get_rt_status(the_chip, VBATDET_IRQ),
		
		pm_chg_get_rt_status(the_chip, ATCDONE_IRQ),
		pm_chg_get_rt_status(the_chip, ATCFAIL_IRQ),
		pm_chg_get_rt_status(the_chip, CHGDONE_IRQ),
		pm_chg_get_rt_status(the_chip, CHGFAIL_IRQ),
		
		pm_chg_get_rt_status(the_chip, FASTCHG_IRQ),
		pm_chg_get_rt_status(the_chip, TRKLCHG_IRQ),
		pm_chg_get_rt_status(the_chip, VCP_IRQ),
		
		pm_chg_get_rt_status(the_chip, BATTTEMP_HOT_IRQ),
		pm_chg_get_rt_status(the_chip, BATTTEMP_COLD_IRQ),
		pm_chg_get_rt_status(the_chip, BAT_TEMP_OK_IRQ),
		pm_chg_get_rt_status(the_chip, BATFET_IRQ),
		
		pm_chg_get_rt_status(the_chip, CHGHOT_IRQ),
		pm_chg_get_rt_status(the_chip, CHG_GONE_IRQ),
		pm_chg_get_rt_status(the_chip, CHGSTATE_IRQ),
		pm_chg_get_rt_status(the_chip, CHGWDOG_IRQ),
		
		pm_chg_get_rt_status(the_chip, VDD_LOOP_IRQ),
		pm_chg_get_rt_status(the_chip, VREG_OV_IRQ),
		pm_chg_get_rt_status(the_chip, COARSE_DET_LOW_IRQ),
		pm_chg_get_rt_status(the_chip, LOOP_CHANGE_IRQ),
		
		
		pm_chg_get_rt_status(the_chip, DCIN_VALID_IRQ),
		pm_chg_get_rt_status(the_chip, DCIN_OV_IRQ),
		pm_chg_get_rt_status(the_chip, DCIN_UV_IRQ));
}

static void dump_reg(void)
{
	u64 val;
	unsigned int len =0;

	memset(batt_log_buf, 0, sizeof(BATT_LOG_BUF_LEN));

	get_reg((void *)CHG_CNTRL, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "CNTRL=0x%llx,", val);
	get_reg((void *)CHG_CNTRL_2, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "CNTRL_2=0x%llx,", val);
	get_reg((void *)CHG_CNTRL_3, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "CNTRL_3=0x%llx,", val);
	get_reg((void *)PBL_ACCESS1, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "PBL_ACCESS1=0x%llx,", val);
	get_reg((void *)PBL_ACCESS2, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "PBL_ACCESS2=0x%llx,", val);
	get_reg((void *)SYS_CONFIG_1, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "SYS_CONFIG_1=0x%llx,", val);
	get_reg((void *)SYS_CONFIG_2, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "SYS_CONFIG_2=0x%llx,", val);
	get_reg((void *)CHG_IBAT_SAFE, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "IBAT_SAFE=0x%llx,", val);
	get_reg((void *)CHG_IBAT_MAX, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "IBAT_MAX=0x%llx,", val);
	get_reg((void *)CHG_VBAT_DET, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "VBAT_DET=0x%llx,", val);
	get_reg((void *)CHG_VDD_SAFE, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "VDD_SAFE=0x%llx,", val);
	get_reg((void *)CHG_VDD_MAX, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "VDD_MAX=0x%llx,", val);
	get_reg((void *)CHG_VIN_MIN, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "VIN_MIN=0x%llx,", val);
	get_reg((void *)CHG_VTRICKLE, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "VTRICKLE=0x%llx,", val);
	get_reg((void *)CHG_ITRICKLE, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "ITRICKLE=0x%llx,", val);
	get_reg((void *)CHG_ITERM, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "ITERM=0x%llx,", val);
	get_reg((void *)CHG_TCHG_MAX, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "TCHG_MAX=0x%llx,", val);
	get_reg((void *)CHG_TWDOG, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "TWDOG=0x%llx,", val);
	get_reg((void *)CHG_TEMP_THRESH, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "TEMP_THRESH=0x%llx,", val);
	get_reg((void *)CHG_COMP_OVR, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "COMP_OVR=0x%llx,", val);
	get_reg((void *)CHG_BUCK_CTRL_TEST1, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "BUCK_CTRL_TEST1=0x%llx,", val);
	get_reg((void *)CHG_BUCK_CTRL_TEST2, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "BUCK_CTRL_TEST2=0x%llx,", val);
	get_reg((void *)CHG_BUCK_CTRL_TEST3, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "BUCK_CTRL_TEST3=0x%llx,", val);
	get_reg((void *)CHG_TEST, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "CHG_TEST=0x%llx,", val);
	get_reg((void *)USB_OVP_CONTROL, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "USB_OVP_CONTROL=0x%llx,", val);
	get_reg((void *)USB_OVP_TEST, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "USB_OVP_TEST=0x%llx,", val);
	get_reg((void *)DC_OVP_CONTROL, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "DC_OVP_CONTROL=0x%llx,", val);
	get_reg((void *)DC_OVP_TEST, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "DC_OVP_TEST=0x%llx,", val);
	get_reg_loop((void *)NULL, &val);
	len += scnprintf(batt_log_buf + len, BATT_LOG_BUF_LEN - len, "REGULATION_LOOP_CONTROL=0x%llx", val);

	
	if(BATT_LOG_BUF_LEN - len <= 1)
		pr_info("batt log length maybe out of buffer range!!!");

	pr_info("%s\n", batt_log_buf);
}

static void dump_all(int more)
{
	int rc;
	struct pm8xxx_adc_chan_result result;
	int vbat_mv, ibat_ma, tbat_deg, soc, id_mv, iusb_ma;
	int fcc, health, present, charger_type, status;
	int fsm, ac_online, usb_online, dc_online;
	int ichg = 0, vph_pwr = 0, usbin = 0, dcin = 0;
	int temp_fault = 0, vbatdet_low = 0, is_wlc_remove = 1;

	vbat_mv = get_prop_battery_uvolts(the_chip)/1000;
	soc = get_prop_batt_capacity(the_chip);
	ibat_ma = get_prop_batt_current(the_chip)/1000;
	fcc = get_prop_batt_fcc(the_chip);
	id_mv = (int)read_battery_id(the_chip)/1000;
	health = get_prop_batt_health(the_chip);
	present = get_prop_batt_present(the_chip);
	charger_type = get_prop_charge_type(the_chip);
	status = get_prop_batt_status(the_chip);
	tbat_deg = get_prop_batt_temp(the_chip)/10;
	if (tbat_deg >= 68)
		pr_warn("battery temperature=%d >= 68\n", tbat_deg);

	fsm = pm_chg_get_fsm_state(the_chip);
	ac_online = is_ac_online();
	usb_online = is_usb_chg_plugged_in(the_chip);
	dc_online = is_dc_chg_plugged_in(the_chip);
	pm8921_is_batt_temperature_fault(&temp_fault);
	pm_chg_iusbmax_get(the_chip, &iusb_ma);

	rc = pm8xxx_adc_read(CHANNEL_ICHG, &result);
	if (rc) {
		pr_err("error reading i_chg channel = %d, rc = %d\n",
					CHANNEL_ICHG, rc);
	}
	ichg = (int)result.physical;

	rc = pm8xxx_adc_read(CHANNEL_VPH_PWR, &result);
	if (rc) {
		pr_err("error reading vph_pwr channel = %d, rc = %d\n",
					CHANNEL_VPH_PWR, rc);
	}
	vph_pwr = (int)result.physical;

	rc = pm8xxx_adc_read(CHANNEL_USBIN, &result);
	if (rc) {
		pr_err("error reading i_chg channel = %d, rc = %d\n",
					CHANNEL_USBIN, rc);
	}
	usbin = (int)result.physical;

	dcin = get_prop_dcin_uvolts(the_chip);

	vbatdet_low = pm_chg_get_rt_status(the_chip, VBATDET_LOW_IRQ);
	if (the_chip->wlc_tx_gpio)
		is_wlc_remove = gpio_get_value(the_chip->wlc_tx_gpio);

	pr_info("V=%d mV, I=%d mA, T=%d C, SoC=%d%%, FCC=%d, id=%d mV,"
			" H=%d, P=%d, CHG=%d, S=%d, FSM=%d, AC=%d, USB=%d, DC=%d, WLC=%d"
			" iusb_ma=%d, usb_target_ma=%d, OVP=%d, UVP=%d, TM=%d, eoc_count=%d,"
			" vbatdet_low=%d, is_ac_ST=%d, batfet_dis=0x%x, pwrsrc_dis=0x%x,"
			" is_full=%d, temp_fault=%d, is_bat_warm/cool=%d/%d,"
			" btm_warm/cool=%d/%d, ichg=%d uA, vph_pwr=%d uV, usbin=%d uV,"
			" dcin=%d uV, pwrsrc_under_rating=%d, usbin_critical_low_cnt=%d"
			" disable_reverse_boost_check=%d, flag=%d%d%d%d, hsml_target_ma=%d\n",
			vbat_mv, ibat_ma, tbat_deg, soc, fcc, id_mv,
			health, present, charger_type, status, fsm,
			ac_online, usb_online, dc_online, !is_wlc_remove,
			iusb_ma, usb_target_ma, ovp, uvp, thermal_mitigation, eoc_count,
			vbatdet_low, is_ac_safety_timeout, batt_charging_disabled,
			pwrsrc_disabled, is_batt_full,
			temp_fault, the_chip->is_bat_warm, the_chip->is_bat_cool,
			pm8xxx_adc_btm_is_warm(), pm8xxx_adc_btm_is_cool(), ichg, vph_pwr,
			usbin, dcin, pwrsrc_under_rating, usbin_critical_low_cnt,
			the_chip->disable_reverse_boost_check, test_power_monitor,
			flag_keep_charge_on, flag_pa_recharge, flag_disable_wakelock, hsml_target_ma);
	
	if (more || (fsm == FSM_STATE_OFF_0) || (ibat_ma < -1000) ||
			(tbat_deg == 80) || (4000 < usbin && (!usb_online)) ||
			(2000 < ibat_ma) || is_cable_remove ||
			(0 < ibat_ma && fsm == FSM_STATE_FAST_CHG_7) ||
			(!is_batt_full && usb_online && fsm != FSM_STATE_FAST_CHG_7)) {
		dump_irq_rt_status();
		dump_reg();
		pm8921_bms_dump_all();
		is_cable_remove = false;
	}
}


int pm8921_set_hsml_target_ma(int target_ma)
{
	if (!the_chip) {
		pr_err("%s called before init\n", __func__);
		return -EINVAL;
	}

	pr_info("%s target_ma: %d\n", __func__, target_ma);
	hsml_target_ma = target_ma;

	if((hsml_target_ma != 0) && (pwr_src == HTC_PWR_SOURCE_TYPE_USB)) {
			_pm8921_charger_vbus_draw(hsml_target_ma);
	}

	return 0;
}


inline int pm8921_dump_all(void)
{
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}
	dump_all(0);

	if(the_chip->ext_usb)
	{
		if(the_chip->ext_usb->ichg->dump_all)
			the_chip->ext_usb->ichg->dump_all();
	}

	return 0;
}

static void update_heartbeat(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct pm8921_chg_chip *chip = container_of(dwork,
				struct pm8921_chg_chip, update_heartbeat_work);

	pm_chg_failed_clear(chip, 1);
	
	schedule_delayed_work(&chip->update_heartbeat_work,
			      round_jiffies_relative(msecs_to_jiffies
						     (chip->update_time)));
	dump_all(0);
}

static void recharge_check_worker(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct pm8921_chg_chip *chip = container_of(dwork,
				struct pm8921_chg_chip, recharge_check_work);
	int fast_chg;

	fast_chg = pm_chg_get_rt_status(chip, FASTCHG_IRQ);
	if (!fast_chg) {
		pr_info("check vbatdet_low_irq rt status\n");
		vbatdet_low_irq_handler(chip->pmic_chg_irq[VBATDET_LOW_IRQ], chip);
	}
	wake_unlock(&chip->recharge_check_wake_lock);
}

enum {
	CHG_IN_PROGRESS,
	CHG_NOT_IN_PROGRESS,
	CHG_FINISHED,
};
#define VBAT_TOLERANCE_MV	70
#define CHG_DISABLE_MSLEEP	100
static int is_charging_finished(struct pm8921_chg_chip *chip)
{
	int vbat_meas_uv, vbat_meas_mv, vbat_programmed, vbatdet_low;
	int ichg_meas_ma, iterm_programmed;
	int regulation_loop, fast_chg, vcp;
	int rc;
	int ext_in_charging = 0;

	static int last_vbat_programmed = -EINVAL;

	if (!is_using_ext_charger(chip)) {
		
		fast_chg = pm_chg_get_rt_status(chip, FASTCHG_IRQ);
		pr_debug("fast_chg = %d\n", fast_chg);
		if (fast_chg == 0)
			return CHG_NOT_IN_PROGRESS;

		vcp = pm_chg_get_rt_status(chip, VCP_IRQ);
		pr_debug("vcp = %d\n", vcp);
		if (vcp == 1)
			return CHG_IN_PROGRESS;

		vbatdet_low = pm_chg_get_rt_status(chip, VBATDET_LOW_IRQ);
		pr_debug("vbatdet_low = %d (htc skip this check)\n", vbatdet_low);

		
		rc = pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ);
		pr_debug("batt_temp_ok = %d\n", rc);
		if (rc == 0)
			return CHG_IN_PROGRESS;

		
		vbat_meas_uv = get_prop_battery_uvolts(chip);
		if (vbat_meas_uv < 0)
			return CHG_IN_PROGRESS;
		vbat_meas_mv = vbat_meas_uv / 1000;

		rc = pm_chg_vddmax_get(chip, &vbat_programmed);
		if (rc) {
			pr_err("couldnt read vddmax rc = %d\n", rc);
			return CHG_IN_PROGRESS;
		}
		pr_debug("vddmax = %d vbat_meas_mv=%d\n",
			 vbat_programmed, vbat_meas_mv);
		if (vbat_meas_mv < vbat_programmed - VBAT_TOLERANCE_MV)
			return CHG_IN_PROGRESS;

		if (last_vbat_programmed == -EINVAL)
			last_vbat_programmed = vbat_programmed;
		if (last_vbat_programmed !=  vbat_programmed) {
			
			pr_debug("vddmax = %d last_vdd_max=%d\n",
				 vbat_programmed, last_vbat_programmed);
			last_vbat_programmed = vbat_programmed;
			return CHG_IN_PROGRESS;
		}

		regulation_loop = pm_chg_get_regulation_loop(chip);
		if (regulation_loop < 0) {
			pr_err("couldnt read the regulation loop err=%d\n",
				regulation_loop);
			return CHG_IN_PROGRESS;
		}
		pr_debug("regulation_loop=%d\n", regulation_loop);

		if (regulation_loop != 0 && regulation_loop != VDD_LOOP)
			return CHG_IN_PROGRESS;
	} 
	else if(chip->ext_usb)
	{

		if(chip->ext_usb->ichg->get_charging_enabled)
			chip->ext_usb->ichg->get_charging_enabled(&ext_in_charging);

		if(!ext_in_charging)
			return CHG_NOT_IN_PROGRESS;


		
		rc = pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ);
		pr_debug("batt_temp_ok = %d\n", rc);
		if (rc == 0)
			return CHG_IN_PROGRESS;

		
		vbat_meas_uv = get_prop_battery_uvolts(chip);
		if (vbat_meas_uv < 0)
			return CHG_IN_PROGRESS;
		vbat_meas_mv = vbat_meas_uv / 1000;

		rc = pm_chg_vddmax_get(chip, &vbat_programmed);
		if (rc) {
			pr_err("couldnt read vddmax rc = %d\n", rc);
			return CHG_IN_PROGRESS;
		}

		pr_debug("vddmax = %d vbat_meas_mv=%d\n",
			 vbat_programmed, vbat_meas_mv);
		if (vbat_meas_mv < vbat_programmed - VBAT_TOLERANCE_MV)
			return CHG_IN_PROGRESS;

		if (last_vbat_programmed == -EINVAL)
			last_vbat_programmed = vbat_programmed;
		if (last_vbat_programmed !=  vbat_programmed) {
			
			pr_debug("vddmax = %d last_vdd_max=%d\n",
				 vbat_programmed, last_vbat_programmed);
			last_vbat_programmed = vbat_programmed;
			return CHG_IN_PROGRESS;
		}

		
		rc = pm_chg_iterm_get(chip, &iterm_programmed);

		if (rc) {
			pr_err("couldnt read iterm rc = %d\n", rc);
			return CHG_IN_PROGRESS;
		}

		ichg_meas_ma = (get_prop_batt_current(chip)) / 1000;
		pr_debug("iterm_programmed = %d ichg_meas_ma=%d\n",
					iterm_programmed, ichg_meas_ma);
		if (ichg_meas_ma > 0)
			return CHG_IN_PROGRESS;

		if (ichg_meas_ma * -1 > iterm_programmed)
			return CHG_IN_PROGRESS;

		return CHG_FINISHED;

	}
	else
	{
		
	}


	
	rc = pm_chg_iterm_get(chip, &iterm_programmed);
	if (rc) {
		pr_err("couldnt read iterm rc = %d\n", rc);
		return CHG_IN_PROGRESS;
	}

	ichg_meas_ma = (get_prop_batt_current(chip)) / 1000;
	pr_debug("iterm_programmed = %d ichg_meas_ma=%d\n",
				iterm_programmed, ichg_meas_ma);
	if (ichg_meas_ma > 0)
		return CHG_IN_PROGRESS;

	if (ichg_meas_ma * -1 > iterm_programmed)
		return CHG_IN_PROGRESS;

	return CHG_FINISHED;
}

int pm_chg_program_vbatdet(struct pm8921_chg_chip *chip)
{
	int rc;
	int vbat_programmed;

	rc = pm_chg_vddmax_get(chip, &vbat_programmed);
	if (rc) {
		pr_err("couldnt read vddmax rc = %d\n", rc);
		if (chip->is_bat_warm)
			vbat_programmed = chip->warm_bat_voltage;
		else if (chip->is_bat_cool)
			vbat_programmed = chip->cool_bat_voltage;
		else
			vbat_programmed = chip->max_voltage_mv;
	}
	pr_info("program vbatdet=%d under %s condition\n",
			vbat_programmed - chip->resume_voltage_delta,
			(chip->is_bat_warm|chip->is_bat_cool) ? "warm/cool" : "normal");
	rc = pm_chg_vbatdet_set(chip,
				vbat_programmed - chip->resume_voltage_delta);
	if (rc)
		pr_err("Failed to set vbatdet=%d rc=%d\n",
				vbat_programmed - chip->resume_voltage_delta, rc);
	return rc;
}


#define USBIN_CRITICAL_THRES_UV	(4450 * 1000)
#define USBIN_CRITICAL_LOW_CNT_MAX	(6)
void pwrsrc_under_rating_check(void)
{
	int prev_under_rating;
	int usbin;
	int rc;
	struct pm8xxx_adc_chan_result result;

	if (!is_usb_chg_plugged_in(the_chip))
		return;

	rc = pm8xxx_adc_read(CHANNEL_USBIN, &result);
	if (rc) {
		pr_err("error reading usbin channel = %d, rc = %d\n",
					CHANNEL_USBIN, rc);
		return;
	}
	usbin = (int)result.physical;

	if (usbin < USBIN_CRITICAL_THRES_UV) {
		if ( usbin_critical_low_cnt < USBIN_CRITICAL_LOW_CNT_MAX)
			usbin_critical_low_cnt++;
	} else
		usbin_critical_low_cnt = 0;

	prev_under_rating = pwrsrc_under_rating;
	if (usbin_critical_low_cnt == USBIN_CRITICAL_LOW_CNT_MAX)
		pwrsrc_under_rating = 1;
	else
		pwrsrc_under_rating = 0;

	if (prev_under_rating != pwrsrc_under_rating)
		htc_charger_event_notify(HTC_CHARGER_EVENT_SRC_UNDER_RATING);
}

static int rconn_mohm;
static int set_rconn_mohm(const char *val, struct kernel_param *kp)
{
	int ret;
	struct pm8921_chg_chip *chip = the_chip;

	ret = param_set_int(val, kp);
	if (ret) {
		pr_err("error setting value %d\n", ret);
		return ret;
	}
	if (chip)
		chip->rconn_mohm = rconn_mohm;
	return 0;
}
module_param_call(rconn_mohm, set_rconn_mohm, param_get_uint,
					&rconn_mohm, 0644);
#define CONSECUTIVE_COUNT	3
#define EOC_STOP_CHG_COUNT	(CONSECUTIVE_COUNT + 180)
#define CLEAR_FULL_STATE_BY_LEVEL_THR		90
static void eoc_worker(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct pm8921_chg_chip *chip = container_of(dwork,
				struct pm8921_chg_chip, eoc_work);
	int end, soc = 0;

	if (!is_ac_safety_timeout)
		pm_chg_failed_clear(chip, 1);
	end = is_charging_finished(chip);

	if (end == CHG_NOT_IN_PROGRESS) {
		
		pr_info("%s: End due to fast_chg=%d\n",
				__func__, pm_chg_get_rt_status(chip, FASTCHG_IRQ));
		is_batt_full = false;
		eoc_count = 0;
		is_ac_safety_timeout_twice = false;
		if (!flag_disable_wakelock)
			wake_unlock(&chip->eoc_wake_lock);
		is_batt_full_eoc_stop = false;
		return;
	}

	if (!chip->bms_notify.is_charging)
		bms_notify_check(chip);

	if (CONSECUTIVE_COUNT <= eoc_count)
		end = CHG_FINISHED;

	if (end == CHG_FINISHED) {
		eoc_count++;
	} else {
		eoc_count = 0;
	}

	if (EOC_STOP_CHG_COUNT == eoc_count) {
		eoc_count = 0;
		is_ac_safety_timeout_twice = false;

		if (is_ext_charging(chip))
			chip->ext_charge_done = true;

		if (chip->is_bat_warm || chip->is_bat_cool) {
			pr_info("exit EXT-EOC-CHARGING phase at %s condition.\n",
								(chip->is_bat_warm) ? "warm" : "cool");
			is_batt_full = false;
			is_batt_full_eoc_stop = false;
		} else {
			pr_info("EXT-EOC-CHARGING phase done\n");
			is_batt_full_eoc_stop = true;
		}

		set_appropriate_vbatdet(chip);
		pm_chg_disable_auto_enable(chip, 1, BATT_CHG_DISABLED_BIT_EOC);

		pr_info("vbatdet_low_irq=%d\n",
						pm_chg_get_rt_status(chip, VBATDET_LOW_IRQ));
		
		chgdone_irq_handler(chip->pmic_chg_irq[CHGDONE_IRQ], chip);
		
		schedule_delayed_work(&chip->recharge_check_work,
			      round_jiffies_relative(msecs_to_jiffies
						     (EOC_CHECK_PERIOD_MS)));
		wake_lock(&chip->recharge_check_wake_lock);

		if (!flag_disable_wakelock)
			wake_unlock(&chip->eoc_wake_lock);
		return;
	} else if (CONSECUTIVE_COUNT == eoc_count) {
		if (chip->is_bat_warm || chip->is_bat_cool) {
			pr_info("meet %s EOC condition.\n",
								(chip->is_bat_warm) ? "warm" : "cool");
			set_appropriate_vbatdet(chip);
			pm_chg_disable_auto_enable(chip, 1, BATT_CHG_DISABLED_BIT_EOC);
			is_batt_full = false;
			
			chgdone_irq_handler(chip->pmic_chg_irq[CHGDONE_IRQ], chip);
			if (!flag_disable_wakelock)
				wake_unlock(&chip->eoc_wake_lock);
		} else {
			pr_info("EXT-EOC-CHARGING phase start\n");
			is_batt_full = true;
			pm8921_bms_charging_end(1);
			htc_gauge_event_notify(HTC_GAUGE_EVENT_EOC);
		}
	} else if (0 == eoc_count) {
		is_batt_full_eoc_stop = false;
	}

	if (is_batt_full) {
		soc = get_prop_batt_capacity(chip);
		if (soc < CLEAR_FULL_STATE_BY_LEVEL_THR) {
			is_batt_full = false;
			eoc_count = 0;
			pr_info("%s: Clear is_batt_full & eoc_count due to"
						" Overloading happened, soc=%d\n",
						__func__, soc);
			htc_gauge_event_notify(HTC_GAUGE_EVENT_EOC);
		}
	}

	adjust_vdd_max_for_fastchg(chip);
	pr_debug("EOC count = %d\n", eoc_count);
	schedule_delayed_work(&chip->eoc_work,
		      round_jiffies_relative(msecs_to_jiffies
					     (EOC_CHECK_PERIOD_MS)));

	pwrsrc_under_rating_check();
}

static void ovp_check_worker(struct work_struct *work)
{
	usbin_ov_irq_handler(the_chip->pmic_chg_irq[USBIN_OV_IRQ], the_chip);
}

static void btm_configure_work(struct work_struct *work)
{
	int rc;

	rc = pm8xxx_adc_btm_configure(&btm_config);
	if (rc)
		pr_err("failed to configure btm rc=%d", rc);
}

DECLARE_WORK(btm_config_work, btm_configure_work);

static DEFINE_SPINLOCK(set_current_lock);
static void set_appropriate_battery_current(struct pm8921_chg_chip *chip)
{
	unsigned int chg_current = chip->max_bat_chg_current;
	unsigned long flags;

	spin_lock_irqsave(&set_current_lock, flags);
	if (chip->is_bat_cool)
		chg_current = min(chg_current, chip->cool_bat_chg_current);

	if (chip->is_bat_warm)
		chg_current = min(chg_current, chip->warm_bat_chg_current);

	if (thermal_mitigation != 0 && chip->thermal_mitigation)
		chg_current = min(chg_current,
				chip->thermal_mitigation[thermal_mitigation]);

	if (chg_limit_current != 0)
		chg_current = min(chg_current, chg_limit_current);

	pm_chg_ibatmax_set(the_chip, chg_current);
	spin_unlock_irqrestore(&set_current_lock, flags);
}

int pm8921_limit_charge_enable(bool enable)
{
	pr_info("limit_charge=%d\n", enable);
	if (!the_chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	if (enable)
		chg_limit_current = PM8921_CHG_I_MIN_MA;
	else
		chg_limit_current = 0;

	set_appropriate_battery_current(the_chip);
	return 0;
}

#define TEMP_HYSTERISIS_DECIDEGC 20
static void battery_cool(bool enter)
{
	static int prev_is_cold;
	int is_cold = pm_chg_get_rt_status(the_chip, BATTTEMP_COLD_IRQ);

	pr_info("%s:enter=%d, is_cold=%d\n", __func__, enter, is_cold);

	if (enter == the_chip->is_bat_cool)
		return;
	the_chip->is_bat_cool = enter;
	if (enter) {
		btm_config.low_thr_temp =
			the_chip->cool_temp_dc + TEMP_HYSTERISIS_DECIDEGC;
		set_appropriate_battery_current(the_chip);
		pm_chg_vddmax_set(the_chip, the_chip->cool_bat_voltage);
	} else {
		btm_config.low_thr_temp = the_chip->cool_temp_dc;
		set_appropriate_battery_current(the_chip);
		pm_chg_vddmax_set(the_chip, the_chip->max_voltage_mv);
	}

	set_appropriate_vbatdet(the_chip);

	if(is_cold != prev_is_cold)
	{
		htc_gauge_event_notify(HTC_GAUGE_EVENT_TEMP_ZONE_CHANGE);

		
		if(the_chip->ext_usb)
		{
			queue_delayed_work(ext_charger_wq, &ext_usb_temp_task, HZ/200);
		}

		prev_is_cold = is_cold;
	}

	schedule_work(&btm_config_work);
}

static void battery_warm(bool enter)
{
	pr_info("%s:enter=%d\n", __func__, enter);
	if (enter == the_chip->is_bat_warm)
		return;
	the_chip->is_bat_warm = enter;
	if (enter) {
		btm_config.high_thr_temp =
			the_chip->warm_temp_dc - TEMP_HYSTERISIS_DECIDEGC;
		set_appropriate_battery_current(the_chip);
		pm_chg_vddmax_set(the_chip, the_chip->warm_bat_voltage);
		htc_gauge_event_notify(HTC_GAUGE_EVENT_TEMP_ZONE_CHANGE);
	} else {
		btm_config.high_thr_temp = the_chip->warm_temp_dc;
		set_appropriate_battery_current(the_chip);
		pm_chg_vddmax_set(the_chip, the_chip->max_voltage_mv);
		htc_gauge_event_notify(HTC_GAUGE_EVENT_TEMP_ZONE_CHANGE);
	}

	set_appropriate_vbatdet(the_chip);
	
	if(the_chip->ext_usb)
	{
		queue_delayed_work(ext_charger_wq, &ext_usb_temp_task, HZ/200);
	}

	schedule_work(&btm_config_work);
}

static int configure_btm(struct pm8921_chg_chip *chip)
{
	int rc;

	if (chip->warm_temp_dc != INT_MIN)
		btm_config.btm_warm_fn = battery_warm;
	else
		btm_config.btm_warm_fn = NULL;

	if (chip->cool_temp_dc != INT_MIN)
		btm_config.btm_cool_fn = battery_cool;
	else
		btm_config.btm_cool_fn = NULL;

	btm_config.low_thr_temp = chip->cool_temp_dc;
	btm_config.high_thr_temp = chip->warm_temp_dc;
	btm_config.interval = chip->temp_check_period;
	rc = pm8xxx_adc_btm_configure(&btm_config);
	if (rc)
		pr_err("failed to configure btm rc = %d\n", rc);
	rc = pm8xxx_adc_btm_start();
	if (rc)
		pr_err("failed to start btm rc = %d\n", rc);

	return rc;
}

int register_external_dc_charger(struct ext_chg_pm8921 *ext)
{
	if (the_chip == NULL) {
		pr_err("called too early\n");
		return -EINVAL;
	}
	
	the_chip->ext = ext;
	the_chip->ext_charging = false;

	if (is_dc_chg_plugged_in(the_chip))
		pm8921_disable_source_current(true); 

	handle_start_ext_chg(the_chip);

	return 0;
}
EXPORT_SYMBOL(register_external_dc_charger);

void unregister_external_dc_charger(struct ext_chg_pm8921 *ext)
{
	if (the_chip == NULL) {
		pr_err("called too early\n");
		return;
	}
	handle_stop_ext_chg(the_chip);
	the_chip->ext = NULL;
}
EXPORT_SYMBOL(unregister_external_dc_charger);

static int set_usb_ovp_disable_param(const char *val, struct kernel_param *kp)
{
	int ret;

	ret = param_set_int(val, kp);
	if (ret) {
		pr_err("set_usb_ovp_disable_param: error setting value %d\n", ret);
		return ret;
	}
	pr_info("set_usb_ovp_disable_param: usb_ovp_disable=%d\n", usb_ovp_disable);
	pm8921_usb_ovp_disable(usb_ovp_disable);
	return 0;
}
module_param_call(ovp_disable, set_usb_ovp_disable_param, param_get_uint,
					&usb_ovp_disable, 0644);

static int set_disable_status_param(const char *val, struct kernel_param *kp)
{
	int ret;
	struct pm8921_chg_chip *chip = the_chip;

	ret = param_set_int(val, kp);
	if (ret) {
		pr_err("error setting value %d\n", ret);
		return ret;
	}
	pr_info("factory set disable param to %d\n", charging_disabled);
	if (chip) {
		pm_chg_disable_auto_enable(chip, charging_disabled, BATT_CHG_DISABLED_BIT_USR1);
		pm_chg_disable_pwrsrc(chip, charging_disabled, PWRSRC_DISABLED_BIT_USER);
	}
	return 0;
}
module_param_call(disabled, set_disable_status_param, param_get_uint,
					&charging_disabled, 0644);

static int set_auto_enable_param(const char *val, struct kernel_param *kp)
{
	int ret;
	struct pm8921_chg_chip *chip = the_chip;

	ret = param_set_int(val, kp);
	if (ret) {
		pr_err("error setting value %d\n", ret);
		return ret;
	}
	pr_info("factory set auto enable param to %d\n", auto_enable);
	if (chip)
		pm_chg_disable_auto_enable(chip, !auto_enable, BATT_CHG_DISABLED_BIT_USR2);

	return 0;
}
module_param_call(auto_enable, set_auto_enable_param, param_get_uint,
					&auto_enable, 0644);

static int set_therm_mitigation_level(const char *val, struct kernel_param *kp)
{
	int ret;
	struct pm8921_chg_chip *chip = the_chip;

	ret = param_set_int(val, kp);
	if (ret) {
		pr_err("error setting value %d\n", ret);
		return ret;
	}

	if (!chip) {
		pr_err("called before init\n");
		return -EINVAL;
	}

	if (!chip->thermal_mitigation) {
		pr_err("no thermal mitigation\n");
		return -EINVAL;
	}

	if (thermal_mitigation < 0
		|| thermal_mitigation >= chip->thermal_levels) {
		pr_err("out of bound level selected\n");
		return -EINVAL;
	}

	pr_info("set mitigation level(%d) current=%d\n",
			thermal_mitigation, chip->thermal_mitigation[thermal_mitigation]);
	set_appropriate_battery_current(chip);
	return ret;
}
module_param_call(thermal_mitigation, set_therm_mitigation_level,
					param_get_uint,
					&thermal_mitigation, 0644);

static int set_usb_max_current(const char *val, struct kernel_param *kp)
{
	int ret, mA;
	struct pm8921_chg_chip *chip = the_chip;

	ret = param_set_int(val, kp);
	if (ret) {
		pr_err("error setting value %d\n", ret);
		return ret;
	}
	if (chip) {
		pr_warn("setting current max to %d\n", usb_max_current);
		pm_chg_iusbmax_get(chip, &mA);
		if (mA > usb_max_current)
			pm8921_charger_vbus_draw(usb_max_current);
		return 0;
	}
	return -EINVAL;
}
module_param_call(usb_max_current, set_usb_max_current, param_get_uint,
					&usb_max_current, 0644);

static void free_irqs(struct pm8921_chg_chip *chip)
{
	int i;

	for (i = 0; i < PM_CHG_MAX_INTS; i++)
		if (chip->pmic_chg_irq[i]) {
			free_irq(chip->pmic_chg_irq[i], chip);
			chip->pmic_chg_irq[i] = 0;
		}
}

static void __devinit determine_initial_state(struct pm8921_chg_chip *chip)
{
	unsigned long flags;
	int fsm_state = 0;
	int usb_present = 0;
	int cable_in_irq = 0;

	chip->dc_present = !!is_dc_chg_plugged_in(chip);
	usb_present = !!is_usb_chg_plugged_in(chip);
	usbin_ov_irq_state = pm_chg_get_rt_status(the_chip, USBIN_OV_IRQ);
	usbin_uv_irq_state = pm_chg_get_rt_status(the_chip, USBIN_UV_IRQ);

	if (chip->cable_in_irq) {
		cable_in_irq = gpio_get_value(chip->cable_in_gpio);
		if ( usbin_ov_irq_state && !usb_present && !cable_in_irq) {
			if (!ovp) {
				ovp = 1;
				pr_info("init OVP: 0 -> 1 by cable in irq\n");
			}
		} else if ( !usbin_ov_irq_state && !usb_present
				&& usbin_uv_irq_state && !cable_in_irq) {
			if (!uvp) {
				uvp = 1;
				pr_info("init UVP: 0 -> 1 by cable in irq\n");
			}
		}
	} else {
		if ( usbin_ov_irq_state && !usb_present && !usbin_uv_irq_state) {
			if (!ovp) {
				ovp = 1;
				pr_info("init OVP: 0 -> 1\n");
			}
		} else if ( !usbin_ov_irq_state && !usb_present && usbin_uv_irq_state) {
			if (!uvp) {
				uvp = 1;
				pr_info("init UVP: 0 -> 1\n");
			}
		}
	}

	bat_temp_ok_prev = pm_chg_get_rt_status(chip, BAT_TEMP_OK_IRQ);

	pm8921_chg_enable_irq(chip, DCIN_VALID_IRQ);
	pm8921_chg_enable_irq(chip, USBIN_VALID_IRQ);
	if (!chip->is_embeded_batt) {
		pm8921_chg_enable_irq(chip, BATT_REMOVED_IRQ);
		pm8921_chg_enable_irq(chip, BATT_INSERTED_IRQ);
	}
	
	
	pm8921_chg_enable_irq(chip, DCIN_OV_IRQ);
	pm8921_chg_enable_irq(chip, DCIN_UV_IRQ);
	pm8921_chg_enable_irq(chip, CHGFAIL_IRQ);
	pm8921_chg_enable_irq(chip, FASTCHG_IRQ);
	pm8921_chg_enable_irq(chip, VBATDET_LOW_IRQ);
	pm8921_chg_enable_irq(chip, BATTTEMP_HOT_IRQ);
	pm8921_chg_enable_irq(chip, BATTTEMP_COLD_IRQ);
	pm8921_chg_enable_irq(chip, BAT_TEMP_OK_IRQ);
	pm8921_chg_enable_irq(chip, CHGHOT_IRQ);

	spin_lock_irqsave(&vbus_lock, flags);
	
	cable_detection_vbus_irq_handler();
	fastchg_irq_handler(chip->pmic_chg_irq[FASTCHG_IRQ], chip);
#if 0
	if (usb_chg_current) {
		
		handle_usb_insertion_removal(chip);
		fastchg_irq_handler(chip->pmic_chg_irq[FASTCHG_IRQ], chip);
	}
#endif
	spin_unlock_irqrestore(&vbus_lock, flags);

	
	if(chip->ext_usb)
	{
		int result = 0;
		if(chip->ext_usb->ichg->is_charging_enabled)
			chip->ext_usb->ichg->is_charging_enabled(&result);

		if(result)
		{
			chip->bms_notify.is_charging = 1;
			pm8921_bms_charging_began();
		}
	}
	else		
	{
		fsm_state = pm_chg_get_fsm_state(chip);
		if (is_battery_charging(fsm_state)) {
			chip->bms_notify.is_charging = 1;
			pm8921_bms_charging_began();
		}
	}

	check_battery_valid(chip);

	pr_debug("usb = %d, dc = %d  batt = %d state=%d\n",
			usb_present,
			chip->dc_present,
			get_prop_batt_present(chip),
			fsm_state);
}

struct pm_chg_irq_init_data {
	unsigned int	irq_id;
	char		*name;
	unsigned long	flags;
	irqreturn_t	(*handler)(int, void *);
};

#define CHG_IRQ(_id, _flags, _handler) \
{ \
	.irq_id		= _id, \
	.name		= #_id, \
	.flags		= _flags, \
	.handler	= _handler, \
}
struct pm_chg_irq_init_data chg_irq_data[] = {
	CHG_IRQ(USBIN_VALID_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						usbin_valid_irq_handler),
	CHG_IRQ(USBIN_OV_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						usbin_ov_irq_handler),
	CHG_IRQ(BATT_INSERTED_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						batt_inserted_irq_handler),
	CHG_IRQ(VBATDET_LOW_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						vbatdet_low_irq_handler),
	CHG_IRQ(USBIN_UV_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						usbin_uv_irq_handler),
	CHG_IRQ(VBAT_OV_IRQ, IRQF_TRIGGER_RISING, vbat_ov_irq_handler),
	CHG_IRQ(CHGWDOG_IRQ, IRQF_TRIGGER_RISING, chgwdog_irq_handler),
	CHG_IRQ(VCP_IRQ, IRQF_TRIGGER_RISING, vcp_irq_handler),
	CHG_IRQ(ATCDONE_IRQ, IRQF_TRIGGER_RISING, atcdone_irq_handler),
	CHG_IRQ(ATCFAIL_IRQ, IRQF_TRIGGER_RISING, atcfail_irq_handler),
	CHG_IRQ(CHGDONE_IRQ, IRQF_TRIGGER_RISING, chgdone_irq_handler),
	CHG_IRQ(CHGFAIL_IRQ, IRQF_TRIGGER_RISING, chgfail_irq_handler),
	CHG_IRQ(CHGSTATE_IRQ, IRQF_TRIGGER_RISING, chgstate_irq_handler),
	CHG_IRQ(LOOP_CHANGE_IRQ, IRQF_TRIGGER_RISING, loop_change_irq_handler),
	CHG_IRQ(FASTCHG_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						fastchg_irq_handler),
	CHG_IRQ(TRKLCHG_IRQ, IRQF_TRIGGER_RISING, trklchg_irq_handler),
	CHG_IRQ(BATT_REMOVED_IRQ, IRQF_TRIGGER_RISING,
						batt_removed_irq_handler),
	CHG_IRQ(BATTTEMP_HOT_IRQ, IRQF_TRIGGER_RISING,
						batttemp_hot_irq_handler),
	CHG_IRQ(CHGHOT_IRQ, IRQF_TRIGGER_RISING, chghot_irq_handler),
	CHG_IRQ(BATTTEMP_COLD_IRQ, IRQF_TRIGGER_RISING,
						batttemp_cold_irq_handler),
	CHG_IRQ(CHG_GONE_IRQ, IRQF_TRIGGER_RISING |IRQF_TRIGGER_FALLING , chg_gone_irq_handler),
	CHG_IRQ(BAT_TEMP_OK_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						bat_temp_ok_irq_handler),
	CHG_IRQ(COARSE_DET_LOW_IRQ, IRQF_TRIGGER_RISING,
						coarse_det_low_irq_handler),
	CHG_IRQ(VDD_LOOP_IRQ, IRQF_TRIGGER_RISING, vdd_loop_irq_handler),
	CHG_IRQ(VREG_OV_IRQ, IRQF_TRIGGER_RISING, vreg_ov_irq_handler),
	CHG_IRQ(VBATDET_IRQ, IRQF_TRIGGER_RISING, vbatdet_irq_handler),
	CHG_IRQ(BATFET_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						batfet_irq_handler),
	CHG_IRQ(DCIN_VALID_IRQ, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						dcin_valid_irq_handler),
	CHG_IRQ(DCIN_OV_IRQ, IRQF_TRIGGER_RISING, dcin_ov_irq_handler),
	CHG_IRQ(DCIN_UV_IRQ, IRQF_TRIGGER_RISING, dcin_uv_irq_handler),
};

static int __devinit request_irqs(struct pm8921_chg_chip *chip,
					struct platform_device *pdev)
{
	struct resource *res;
	int ret, i;

	ret = 0;
	bitmap_fill(chip->enabled_irqs, PM_CHG_MAX_INTS);

	for (i = 0; i < ARRAY_SIZE(chg_irq_data); i++) {
		res = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
				chg_irq_data[i].name);
		if (res == NULL) {
			pr_err("couldn't find %s\n", chg_irq_data[i].name);
			goto err_out;
		}
		chip->pmic_chg_irq[chg_irq_data[i].irq_id] = res->start;
		ret = request_irq(res->start, chg_irq_data[i].handler,
			chg_irq_data[i].flags,
			chg_irq_data[i].name, chip);
		if (ret < 0) {
			pr_err("couldn't request %d (%s) %d\n", res->start,
					chg_irq_data[i].name, ret);
			chip->pmic_chg_irq[chg_irq_data[i].irq_id] = 0;
			goto err_out;
		}
		pm8921_chg_disable_irq(chip, chg_irq_data[i].irq_id);
	}
	return 0;

err_out:
	free_irqs(chip);
	return -EINVAL;
}

static void pm8921_chg_force_19p2mhz_clk(struct pm8921_chg_chip *chip)
{
	int err;
	u8 temp;

	temp  = 0xD1;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}

	temp  = 0xD3;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}

	temp  = 0xD1;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}

	temp  = 0xD5;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}

	udelay(183);

	temp  = 0xD1;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}

	temp  = 0xD0;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}
	udelay(32);

	temp  = 0xD1;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}

	temp  = 0xD3;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}
}

static void pm8921_chg_set_hw_clk_switching(struct pm8921_chg_chip *chip)
{
	int err;
	u8 temp;

	temp  = 0xD1;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}

	temp  = 0xD0;
	err = pm8xxx_writeb(chip->dev->parent, CHG_TEST, temp);
	if (err) {
		pr_err("Error %d writing %d to addr %d\n", err, temp, CHG_TEST);
		return;
	}
}

#define ENUM_TIMER_STOP_BIT	BIT(1)
#define BOOT_DONE_BIT		BIT(6)
#define CHG_BATFET_ON_BIT	BIT(3)
#define CHG_VCP_EN		BIT(0)
#define CHG_BAT_TEMP_DIS_BIT	BIT(2)
#define SAFE_CURRENT_MA		1500
#define VREF_BATT_THERM_FORCE_ON	BIT(7)
#define PM_SUB_REV		0x001
static int __devinit pm8921_chg_hw_init(struct pm8921_chg_chip *chip)
{
	int rc, vdd_safe;
	unsigned long flags;
	u8 subrev;

	if (pm8xxx_get_version(chip->dev->parent) == PM8XXX_VERSION_8921) {
		
		chip->lockup_lpm_wrkarnd = true;
	}

	if (chip->lockup_lpm_wrkarnd) {
		rc = pm8921_apply_19p2mhz_kickstart(chip);
		if (rc) {
			pr_err("Failed to apply kickstart rc=%d\n", rc);
			return rc;
		}
	} else
		pm8921_chg_force_19p2mhz_clk(chip);

	rc = pm_chg_masked_write(chip, SYS_CONFIG_2,
					BOOT_DONE_BIT, BOOT_DONE_BIT);
	if (rc) {
		pr_err("Failed to set BOOT_DONE_BIT rc=%d\n", rc);
		return rc;
	}

	vdd_safe = chip->max_voltage_mv + VDD_MAX_INCREASE_MV;

	if (vdd_safe > PM8921_CHG_VDDSAFE_MAX)
		vdd_safe = PM8921_CHG_VDDSAFE_MAX;

	rc = pm_chg_vddsafe_set(chip, vdd_safe);

	if (rc) {
		pr_err("Failed to set safe voltage to %d rc=%d\n",
						chip->max_voltage_mv, rc);
		return rc;
	}

	pr_info("Init: Set vbatdet=%d\n", PM8921_CHG_VBATDET_MAX);
	rc = pm_chg_vbatdet_set(chip, PM8921_CHG_VBATDET_MAX);
	if (rc) {
		pr_err("Failed to set vbatdet comprator voltage to %d rc=%d\n",
			chip->max_voltage_mv, rc);
		return rc;
	}

	rc = pm_chg_vddmax_set(chip, chip->max_voltage_mv);
	if (rc) {
		pr_err("Failed to set max voltage to %d rc=%d\n",
						chip->max_voltage_mv, rc);
		return rc;
	}
	rc = pm_chg_ibatsafe_set(chip, SAFE_CURRENT_MA);
	if (rc) {
		pr_err("Failed to set max voltage to %d rc=%d\n",
						SAFE_CURRENT_MA, rc);
		return rc;
	}

	rc = pm_chg_ibatmax_set(chip, chip->max_bat_chg_current);
	if (rc) {
		pr_err("Failed to set max current to 400 rc=%d\n", rc);
		return rc;
	}

	rc = pm_chg_iterm_set(chip, chip->term_current);
	if (rc) {
		pr_err("Failed to set term current to %d rc=%d\n",
						chip->term_current, rc);
		return rc;
	}

	
	rc = pm_chg_masked_write(chip, PBL_ACCESS2, ENUM_TIMER_STOP_BIT,
			ENUM_TIMER_STOP_BIT);
	if (rc) {
		pr_err("Failed to set enum timer stop rc=%d\n", rc);
		return rc;
	}

	if (chip->safety_time != 0) {
		if (chip->safety_time > SAFETY_TIME_MAX_LIMIT) {
			rc = pm_chg_tchg_max_set(chip, SAFETY_TIME_8HR_TWICE);
		} else {
			rc = pm_chg_tchg_max_set(chip, chip->safety_time);
		}
		if (rc) {
			pr_err("Failed to set max time to %d minutes rc=%d\n",
							chip->safety_time, rc);
			return rc;
		}
	}

	if (chip->ttrkl_time != 0) {
		rc = pm_chg_ttrkl_max_set(chip, chip->ttrkl_time);
		if (rc) {
			pr_err("Failed to set trkl time to %d minutes rc=%d\n",
							chip->ttrkl_time, rc);
			return rc;
		}
	}

	if (chip->vin_min != 0) {
		rc = pm_chg_vinmin_set(chip, chip->vin_min);
		if (rc) {
			pr_err("Failed to set vin min to %d mV rc=%d\n",
							chip->vin_min, rc);
			return rc;
		}
	} else
		chip->vin_min = pm_chg_vinmin_get(chip);

	rc = pm_chg_disable_wd(chip);
	if (rc) {
		pr_err("Failed to disable wd rc=%d\n", rc);
		return rc;
	}

	if (flag_keep_charge_on || flag_pa_recharge)
		rc = pm_chg_masked_write(chip, CHG_CNTRL_2,
					CHG_BAT_TEMP_DIS_BIT,
					CHG_BAT_TEMP_DIS_BIT); 
	else
		rc = pm_chg_masked_write(chip, CHG_CNTRL_2,
					CHG_BAT_TEMP_DIS_BIT, 0); 
	if (rc) {
		pr_err("Failed to enable/disable temp control chg rc=%d\n", rc);
		return rc;
	}

	
	rc = pm_chg_write(chip, CHG_BUCK_CLOCK_CTRL, 0x15);
	if (rc) {
		pr_err("Failed to switch buck clk rc=%d\n", rc);
		return rc;
	}

	if (chip->trkl_voltage != 0) {
		rc = pm_chg_vtrkl_low_set(chip, chip->trkl_voltage);
		if (rc) {
			pr_err("Failed to set trkl voltage to %dmv  rc=%d\n",
							chip->trkl_voltage, rc);
			return rc;
		}
	}

	if (chip->weak_voltage != 0) {
		rc = pm_chg_vweak_set(chip, chip->weak_voltage);
		if (rc) {
			pr_err("Failed to set weak voltage to %dmv  rc=%d\n",
							chip->weak_voltage, rc);
			return rc;
		}
	}

	if (chip->trkl_current != 0) {
		rc = pm_chg_itrkl_set(chip, chip->trkl_current);
		if (rc) {
			pr_err("Failed to set trkl current to %dmA  rc=%d\n",
							chip->trkl_voltage, rc);
			return rc;
		}
	}

	if (chip->weak_current != 0) {
		rc = pm_chg_iweak_set(chip, chip->weak_current);
		if (rc) {
			pr_err("Failed to set weak current to %dmA  rc=%d\n",
							chip->weak_current, rc);
			return rc;
		}
	}

	rc = pm_chg_batt_cold_temp_config(chip, chip->cold_thr);
	if (rc) {
		pr_err("Failed to set cold config %d  rc=%d\n",
						chip->cold_thr, rc);
	}

	rc = pm_chg_batt_hot_temp_config(chip, chip->hot_thr);
	if (rc) {
		pr_err("Failed to set hot config %d  rc=%d\n",
						chip->hot_thr, rc);
	}

	
	if (pm8xxx_get_revision(chip->dev->parent) < PM8XXX_REVISION_8921_2p0
		&& pm8xxx_get_version(chip->dev->parent) == PM8XXX_VERSION_8921) {
		pm_chg_write(chip, CHG_BUCK_CTRL_TEST2, 0xF1);
		pm_chg_write(chip, CHG_BUCK_CTRL_TEST3, 0xCE);
		pm_chg_write(chip, CHG_BUCK_CTRL_TEST3, 0xD8);

		
		pm_chg_write(chip, PSI_TXRX_SAMPLE_DATA_0, 0xFF);
		pm_chg_write(chip, PSI_TXRX_SAMPLE_DATA_1, 0xFF);
		pm_chg_write(chip, PSI_TXRX_SAMPLE_DATA_2, 0xFF);
		pm_chg_write(chip, PSI_TXRX_SAMPLE_DATA_3, 0xFF);
		pm_chg_write(chip, PSI_CONFIG_STATUS, 0x0D);
		udelay(100);
		pm_chg_write(chip, PSI_CONFIG_STATUS, 0x0C);
	}

	
	if (pm8xxx_get_revision(chip->dev->parent) == PM8XXX_REVISION_8921_3p0
		&& pm8xxx_get_version(chip->dev->parent) == PM8XXX_VERSION_8921) {
		rc = pm8xxx_readb(chip->dev->parent, PM_SUB_REV, &subrev);
		if (rc) {
			pr_err("read failed: addr=%03X, rc=%d\n",
				PM_SUB_REV, rc);
			return rc;
		}
		
		if (subrev & 0x1) {
			pm8xxx_writeb(chip->dev->parent,
				CHG_BUCK_CTRL_TEST3, 0xA4);
			pr_info("%s: write 0x%x as A4 for PMIC 3.0.1",
					__func__, CHG_BUCK_CTRL_TEST3);
		} else {
			pm8xxx_writeb(chip->dev->parent,
				CHG_BUCK_CTRL_TEST3, 0xAC);
			pr_info("%s: write 0x%x as AC for PMIC 3.0",
					__func__, CHG_BUCK_CTRL_TEST3);
		}
	}

	pm_chg_write(chip, CHG_BUCK_CTRL_TEST3, 0xD9);

	
	pm_chg_write(chip, CHG_BUCK_CTRL_TEST3, 0x91);

	rc = pm_chg_masked_write(chip, CHG_CNTRL, VREF_BATT_THERM_FORCE_ON,
						VREF_BATT_THERM_FORCE_ON);
	if (rc)
		pr_err("Failed to Force Vref therm rc=%d\n", rc);

	spin_lock_irqsave(&pwrsrc_lock, flags);
	rc = pm_chg_charge_dis(chip, charging_disabled);
	spin_unlock_irqrestore(&pwrsrc_lock, flags);
	if (rc) {
		pr_err("Failed to disable CHG_CHARGE_DIS bit rc=%d\n", rc);
		return rc;
	}

	spin_lock_irqsave(&charger_lock, flags);
	rc = pm_chg_auto_enable(chip, !batt_charging_disabled);
	spin_unlock_irqrestore(&charger_lock, flags);
	if (rc) {
		pr_err("Failed to enable charging rc=%d\n", rc);
		return rc;
	}

	if (pm8xxx_get_version(chip->dev->parent) == PM8XXX_VERSION_8921) {
		
		rc = pm8xxx_writeb(chip->dev->parent, CHG_TEST, 0xD0);
		if (rc) {
			pr_err("Failed to clear kickstart rc=%d\n", rc);
			return rc;
		}

		
		pm8921_chg_set_lpm(chip, 1);
	}

	if (chip->lockup_lpm_wrkarnd) {
		chip->vreg_xoadc = regulator_get(chip->dev, "vreg_xoadc");
		if (IS_ERR(chip->vreg_xoadc))
			return -ENODEV;

		rc = regulator_set_optimum_mode(chip->vreg_xoadc, 10000);
		if (rc < 0) {
			pr_err("Failed to set configure HPM rc=%d\n", rc);
			return rc;
		}

		rc = regulator_set_voltage(chip->vreg_xoadc, 1800000, 1800000);
		if (rc) {
			pr_err("Failed to set L14 voltage rc=%d\n", rc);
			return rc;
		}

		rc = regulator_enable(chip->vreg_xoadc);
		if (rc) {
			pr_err("Failed to enable L14 rc=%d\n", rc);
			return rc;
		}
	}

	return 0;
}

static int get_rt_status(void *data, u64 *val)
{
	int i = (int)data;
	int ret;

	
	ret = pm_chg_get_rt_status(the_chip, i);
	*val = ret;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(rt_fops, get_rt_status, NULL, "%llu\n");

static int get_fsm_status(void *data, u64 *val)
{
	u8 temp;

	temp = pm_chg_get_fsm_state(the_chip);
	*val = temp;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(fsm_fops, get_fsm_status, NULL, "%llu\n");

static int get_reg_loop(void *data, u64 *val)
{
	u8 temp;

	if (!the_chip) {
		pr_err("%s called before init\n", __func__);
		return -EINVAL;
	}
	temp = pm_chg_get_regulation_loop(the_chip);
	*val = temp;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(reg_loop_fops, get_reg_loop, NULL, "0x%02llx\n");

static int get_reg(void *data, u64 *val)
{
	int addr = (int)data;
	int ret;
	u8 temp;

	ret = pm8xxx_readb(the_chip->dev->parent, addr, &temp);
	if (ret) {
		pr_err("pm8xxx_readb to %x value =%d errored = %d\n",
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
	ret = pm_chg_write(the_chip, addr, temp);
	if (ret) {
		pr_err("pm_chg_write to %x value =%d errored = %d\n",
			addr, temp, ret);
		return -EAGAIN;
	}
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(reg_fops, get_reg, set_reg, "0x%02llx\n");

enum {
	BAT_WARM_ZONE,
	BAT_COOL_ZONE,
};
static int get_warm_cool(void *data, u64 * val)
{
	if (!the_chip) {
		pr_err("%s called before init\n", __func__);
		return -EINVAL;
	}
	if ((int)data == BAT_WARM_ZONE)
		*val = the_chip->is_bat_warm;
	if ((int)data == BAT_COOL_ZONE)
		*val = the_chip->is_bat_cool;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(warm_cool_fops, get_warm_cool, NULL, "0x%lld\n");

static void create_debugfs_entries(struct pm8921_chg_chip *chip)
{
	int i;

	chip->dent = debugfs_create_dir("pm8921_chg", NULL);

	if (IS_ERR(chip->dent)) {
		pr_err("pmic charger couldnt create debugfs dir\n");
		return;
	}

	debugfs_create_file("CHG_CNTRL", 0644, chip->dent,
			    (void *)CHG_CNTRL, &reg_fops);
	debugfs_create_file("CHG_CNTRL_2", 0644, chip->dent,
			    (void *)CHG_CNTRL_2, &reg_fops);
	debugfs_create_file("CHG_CNTRL_3", 0644, chip->dent,
			    (void *)CHG_CNTRL_3, &reg_fops);
	debugfs_create_file("PBL_ACCESS1", 0644, chip->dent,
			    (void *)PBL_ACCESS1, &reg_fops);
	debugfs_create_file("PBL_ACCESS2", 0644, chip->dent,
			    (void *)PBL_ACCESS2, &reg_fops);
	debugfs_create_file("SYS_CONFIG_1", 0644, chip->dent,
			    (void *)SYS_CONFIG_1, &reg_fops);
	debugfs_create_file("SYS_CONFIG_2", 0644, chip->dent,
			    (void *)SYS_CONFIG_2, &reg_fops);
	debugfs_create_file("CHG_VDD_MAX", 0644, chip->dent,
			    (void *)CHG_VDD_MAX, &reg_fops);
	debugfs_create_file("CHG_VDD_SAFE", 0644, chip->dent,
			    (void *)CHG_VDD_SAFE, &reg_fops);
	debugfs_create_file("CHG_VBAT_DET", 0644, chip->dent,
			    (void *)CHG_VBAT_DET, &reg_fops);
	debugfs_create_file("CHG_IBAT_MAX", 0644, chip->dent,
			    (void *)CHG_IBAT_MAX, &reg_fops);
	debugfs_create_file("CHG_IBAT_SAFE", 0644, chip->dent,
			    (void *)CHG_IBAT_SAFE, &reg_fops);
	debugfs_create_file("CHG_VIN_MIN", 0644, chip->dent,
			    (void *)CHG_VIN_MIN, &reg_fops);
	debugfs_create_file("CHG_VTRICKLE", 0644, chip->dent,
			    (void *)CHG_VTRICKLE, &reg_fops);
	debugfs_create_file("CHG_ITRICKLE", 0644, chip->dent,
			    (void *)CHG_ITRICKLE, &reg_fops);
	debugfs_create_file("CHG_ITERM", 0644, chip->dent,
			    (void *)CHG_ITERM, &reg_fops);
	debugfs_create_file("CHG_TCHG_MAX", 0644, chip->dent,
			    (void *)CHG_TCHG_MAX, &reg_fops);
	debugfs_create_file("CHG_TWDOG", 0644, chip->dent,
			    (void *)CHG_TWDOG, &reg_fops);
	debugfs_create_file("CHG_TEMP_THRESH", 0644, chip->dent,
			    (void *)CHG_TEMP_THRESH, &reg_fops);
	debugfs_create_file("CHG_COMP_OVR", 0644, chip->dent,
			    (void *)CHG_COMP_OVR, &reg_fops);
	debugfs_create_file("CHG_BUCK_CTRL_TEST1", 0644, chip->dent,
			    (void *)CHG_BUCK_CTRL_TEST1, &reg_fops);
	debugfs_create_file("CHG_BUCK_CTRL_TEST2", 0644, chip->dent,
			    (void *)CHG_BUCK_CTRL_TEST2, &reg_fops);
	debugfs_create_file("CHG_BUCK_CTRL_TEST3", 0644, chip->dent,
			    (void *)CHG_BUCK_CTRL_TEST3, &reg_fops);
	debugfs_create_file("CHG_TEST", 0644, chip->dent,
			    (void *)CHG_TEST, &reg_fops);
	debugfs_create_file("USB_OVP_CONTROL", 0644, chip->dent,
			    (void *)USB_OVP_CONTROL, &reg_fops);

	debugfs_create_file("FSM_STATE", 0644, chip->dent, NULL,
			    &fsm_fops);

	debugfs_create_file("REGULATION_LOOP_CONTROL", 0644, chip->dent, NULL,
			    &reg_loop_fops);

	debugfs_create_file("BAT_WARM_ZONE", 0644, chip->dent,
				(void *)BAT_WARM_ZONE, &warm_cool_fops);
	debugfs_create_file("BAT_COOL_ZONE", 0644, chip->dent,
				(void *)BAT_COOL_ZONE, &warm_cool_fops);

	for (i = 0; i < ARRAY_SIZE(chg_irq_data); i++) {
		if (chip->pmic_chg_irq[chg_irq_data[i].irq_id])
			debugfs_create_file(chg_irq_data[i].name, 0444,
				chip->dent,
				(void *)chg_irq_data[i].irq_id,
				&rt_fops);
	}
}

static void pm_chg_reset_warm_cool_state(struct pm8921_chg_chip *chip)
{
	pr_debug("chip->is_bat_warm=%d, chip->is_bat_cool=%d\n",
						chip->is_bat_warm, chip->is_bat_cool);

	if (chip->is_bat_cool)
			battery_cool(0);
	if (chip->is_bat_warm)
			battery_warm(0);
}

static int pm8921_charger_suspend_noirq(struct device *dev)
{
	int rc;
	struct pm8921_chg_chip *chip = dev_get_drvdata(dev);

	if (chip->lockup_lpm_wrkarnd) {
		rc = regulator_disable(chip->vreg_xoadc);
		if (rc)
			pr_err("Failed to disable L14 rc=%d\n", rc);

		rc = pm8921_apply_19p2mhz_kickstart(chip);
		if (rc)
			pr_err("Failed to apply kickstart rc=%d\n", rc);
	}

	rc = pm_chg_masked_write(chip, CHG_CNTRL, VREF_BATT_THERM_FORCE_ON, 0);
	if (rc)
		pr_err("Failed to Force Vref therm off rc=%d\n", rc);

	if (!chip->lockup_lpm_wrkarnd)
		pm8921_chg_set_hw_clk_switching(chip);

	return 0;
}

static int pm8921_charger_resume_noirq(struct device *dev)
{
	int rc;
	struct pm8921_chg_chip *chip = dev_get_drvdata(dev);

	if (chip->lockup_lpm_wrkarnd) {
		rc = regulator_enable(chip->vreg_xoadc);
		if (rc)
			pr_err("Failed to enable L14 rc=%d\n", rc);
		rc = pm8921_apply_19p2mhz_kickstart(chip);
		if (rc)
			pr_err("Failed to apply kickstart rc=%d\n", rc);
	} else
		pm8921_chg_force_19p2mhz_clk(chip);

	rc = pm_chg_masked_write(chip, CHG_CNTRL, VREF_BATT_THERM_FORCE_ON,
						VREF_BATT_THERM_FORCE_ON);
	if (rc)
		pr_err("Failed to Force Vref therm on rc=%d\n", rc);
	return 0;
}

static int pm8921_charger_resume(struct device *dev)
{
	int rc;
	struct pm8921_chg_chip *chip = dev_get_drvdata(dev);

	if (!(chip->cool_temp_dc == INT_MIN && chip->warm_temp_dc == INT_MIN)
		&& !(chip->keep_btm_on_suspend)) {
		rc = pm8xxx_adc_btm_configure(&btm_config);
		if (rc)
			pr_err("couldn't reconfigure btm rc=%d\n", rc);

		pm_chg_reset_warm_cool_state(chip);

		rc = pm8xxx_adc_btm_start();
		if (rc)
			pr_err("couldn't restart btm rc=%d\n", rc);
	}
	if (pm8921_chg_is_enabled(chip, LOOP_CHANGE_IRQ)) {
		disable_irq_wake(chip->pmic_chg_irq[LOOP_CHANGE_IRQ]);
		pm8921_chg_disable_irq(chip, LOOP_CHANGE_IRQ);
	}
	return 0;
}

static int pm8921_charger_suspend(struct device *dev)
{
	int rc;
	struct pm8921_chg_chip *chip = dev_get_drvdata(dev);

	if (!(chip->cool_temp_dc == INT_MIN && chip->warm_temp_dc == INT_MIN)
		&& !(chip->keep_btm_on_suspend)) {
		rc = pm8xxx_adc_btm_end();
		if (rc)
			pr_err("Failed to disable BTM on suspend rc=%d\n", rc);
	}
	if (is_usb_chg_plugged_in(chip)) {
		pm8921_chg_enable_irq(chip, LOOP_CHANGE_IRQ);
		enable_irq_wake(chip->pmic_chg_irq[LOOP_CHANGE_IRQ]);
	}
	return 0;
}

static const struct dev_pm_ops pm8921_charger_pm_ops = {
	.suspend	= pm8921_charger_suspend,
	.suspend_noirq  = pm8921_charger_suspend_noirq,
	.resume_noirq   = pm8921_charger_resume_noirq,
	.resume		= pm8921_charger_resume,
};

static void ext_usb_vbatdet_irq_handler(struct work_struct *w)
{
	int result;

	pm8921_get_batt_voltage(&result);

	pr_info("%s, vol:%d\n", __func__, result);

	

	if(!(the_chip->ext_usb->ichg->event_notify))
	{
		pr_err("%s event_notify api error!\n", __func__);
		return;
	}

	the_chip->ext_usb->ichg->event_notify(HTC_EXTCHG_EVENT_TYPE_EOC_START_CHARGE);

	
	if (!flag_disable_wakelock)
		wake_lock(&the_chip->eoc_wake_lock);
	schedule_delayed_work(&the_chip->eoc_work,
		round_jiffies_relative(msecs_to_jiffies
					     (EOC_CHECK_PERIOD_MS)));

	return;
}


static void ext_usb_chgdone_irq_handler(struct work_struct *w)
{
	int result;

	pm8921_get_batt_voltage(&result);

	pr_info("%s, vol:%d\n", __func__, result);

	if(!(the_chip->ext_usb->ichg->event_notify))
	{
		pr_err("%s event_notify api error!\n", __func__);
		return;
	}

	
	the_chip->ext_usb->ichg->event_notify(HTC_EXTCHG_EVENT_TYPE_EOC_STOP_CHARGE);

	bms_notify_check(the_chip);
	htc_gauge_event_notify(HTC_GAUGE_EVENT_EOC_STOP_CHG);

	return;
}


static void ext_usb_temp_irq_handler(struct work_struct *w)
{
	int new_temp;
	int hot_irq;
	int cold_irq;

	cold_irq = pm_chg_get_rt_status(the_chip, BATTTEMP_COLD_IRQ);
	hot_irq = pm_chg_get_rt_status(the_chip, BATTTEMP_HOT_IRQ);

	pr_info("%s, cold_irq:%d, hot_irq:%d, is_bat_warm:%d, is_bat_cool:%d,\n",
		__func__, cold_irq, hot_irq, the_chip->is_bat_warm, the_chip->is_bat_cool);


	if(!(the_chip->ext_usb->ichg->event_notify))
	{
		pr_err("%s event_notify api error!\n", __func__);
		return;
	}

	
	if(the_chip->is_bat_warm)
	{
		if(hot_irq)
			new_temp = HTC_EXTCHG_EVENT_TYPE_TEMP_HOT;
		else
			new_temp = HTC_EXTCHG_EVENT_TYPE_TEMP_WARM;
	}
	else if((cold_irq) || (the_chip->is_bat_cool))
	{
		if(cold_irq)
			new_temp = HTC_EXTCHG_EVENT_TYPE_TEMP_COLD;
		else
			new_temp = HTC_EXTCHG_EVENT_TYPE_TEMP_COOL;
	}
	else
		new_temp = HTC_EXTCHG_EVENT_TYPE_TEMP_NORMAL;


	
	if(ext_usb_temp_condition_old == new_temp)
		return;

	
	the_chip->ext_usb->ichg->event_notify(new_temp);

	ext_usb_temp_condition_old = new_temp;

	bms_notify_check(the_chip);
	htc_gauge_event_notify(HTC_GAUGE_EVENT_TEMP_ZONE_CHANGE);
}


static void ext_usb_bms_notify_check_handler(struct work_struct *w)
{
	int new_is_charging = 0;
	int ret = 0;

	pr_info("%s, \n",	__func__);

	if(the_chip->ext_usb->ichg->is_charging_enabled)
	{
		ret = the_chip->ext_usb->ichg->is_charging_enabled(&new_is_charging);

		if(ret)
			pr_info("%s fail to get is_charging_enabled error\n", __func__);
	}


	if (the_chip->bms_notify.is_charging ^ new_is_charging) {
		the_chip->bms_notify.is_charging = new_is_charging;
		schedule_work(&(the_chip->bms_notify.work));
	}
}

static int __devinit pm8921_charger_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct pm8921_chg_chip *chip;
	const struct pm8921_charger_platform_data *pdata
				= pdev->dev.platform_data;
	const struct pm8921_charger_batt_param *chg_batt_param;


	if (!pdata) {
		pr_err("missing platform data\n");
		return -EINVAL;
	}

	chip = kzalloc(sizeof(struct pm8921_chg_chip),
					GFP_KERNEL);
	if (!chip) {
		pr_err("Cannot allocate pm_chg_chip\n");
		return -ENOMEM;
	}

	chip->dev = &pdev->dev;
	chip->safety_time = pdata->safety_time;
	chip->ttrkl_time = pdata->ttrkl_time;
	chip->update_time = pdata->update_time;
	
	chg_batt_param = htc_battery_cell_get_cur_cell_charger_cdata();
	if (!chg_batt_param) {
		chip->max_voltage_mv = pdata->max_voltage;
		chip->cool_bat_voltage = pdata->cool_bat_voltage;
		chip->warm_bat_voltage = pdata->warm_bat_voltage;
	} else {
		chip->max_voltage_mv = chg_batt_param->max_voltage;
		chip->cool_bat_voltage = chg_batt_param->cool_bat_voltage;
		chip->warm_bat_voltage = chg_batt_param->warm_bat_voltage;
	}
	chip->min_voltage_mv = pdata->min_voltage;
	chip->resume_voltage_delta = pdata->resume_voltage_delta;
	chip->term_current = pdata->term_current;
	chip->vbat_channel = pdata->charger_cdata.vbat_channel;
	chip->batt_temp_channel = pdata->charger_cdata.batt_temp_channel;
	chip->batt_id_channel = pdata->charger_cdata.batt_id_channel;
	chip->batt_id_min = pdata->batt_id_min;
	chip->batt_id_max = pdata->batt_id_max;
	if (pdata->cool_temp != INT_MIN)
		chip->cool_temp_dc = pdata->cool_temp * 10;
	else
		chip->cool_temp_dc = INT_MIN;

	if (pdata->warm_temp != INT_MIN)
		chip->warm_temp_dc = pdata->warm_temp * 10;
	else
		chip->warm_temp_dc = INT_MIN;

	chip->temp_check_period = pdata->temp_check_period;
	chip->dc_unplug_check = pdata->dc_unplug_check;
	chip->max_bat_chg_current = pdata->max_bat_chg_current;
	chip->cool_bat_chg_current = pdata->cool_bat_chg_current;
	chip->warm_bat_chg_current = pdata->warm_bat_chg_current;
	chip->keep_btm_on_suspend = pdata->keep_btm_on_suspend;
	chip->trkl_voltage = pdata->trkl_voltage;
	chip->weak_voltage = pdata->weak_voltage;
	chip->trkl_current = pdata->trkl_current;
	chip->weak_current = pdata->weak_current;
	chip->vin_min = pdata->vin_min;
	chip->vin_min_wlc= pdata->vin_min_wlc;
	chip->thermal_mitigation = pdata->thermal_mitigation;
	chip->thermal_levels = pdata->thermal_levels;
	chip->mbat_in_gpio = pdata->mbat_in_gpio;
	if (pdata->wlc_tx_gpio)
		chip->wlc_tx_gpio = pdata->wlc_tx_gpio;
	else
		chip->wlc_tx_gpio = 0;
	chip->is_embeded_batt = pdata->is_embeded_batt;
	chip->cold_thr = pdata->cold_thr;
	chip->hot_thr = pdata->hot_thr;
	chip->rconn_mohm = pdata->rconn_mohm;

	chip->ext_usb = pdata->ext_usb;
	chip->disable_reverse_boost_check = pdata->disable_reverse_boost_check;

	if (pdata->cable_in_irq) {
		chip->cable_in_irq = pdata->cable_in_irq;
		chip->cable_in_gpio = pdata->cable_in_gpio;
	}

	rc = pm8921_chg_hw_init(chip);
	if (rc) {
		pr_err("couldn't init hardware rc=%d\n", rc);
		goto free_chip;
	}

	platform_set_drvdata(pdev, chip);
	the_chip = chip;

	wake_lock_init(&chip->eoc_wake_lock, WAKE_LOCK_SUSPEND, "pm8921_eoc");
	wake_lock_init(&chip->recharge_check_wake_lock, WAKE_LOCK_SUSPEND,
												"pm8921_recharge_check");
	wake_lock_init(&chip->unplug_ovp_fet_open_wake_lock,
			WAKE_LOCK_SUSPEND, "pm8921_unplug_ovp_fet_open_wake_lock");
	INIT_DELAYED_WORK(&chip->eoc_work, eoc_worker);
	INIT_DELAYED_WORK(&chip->recharge_check_work, recharge_check_worker);
	INIT_DELAYED_WORK(&chip->vin_collapse_check_work,
						vin_collapse_check_worker);
	INIT_WORK(&chip->unplug_ovp_fet_open_work,
					unplug_ovp_fet_open_worker);
	INIT_DELAYED_WORK(&chip->unplug_check_work, unplug_check_worker);

	INIT_DELAYED_WORK(&ext_usb_vbat_low_task, ext_usb_vbatdet_irq_handler);
	INIT_DELAYED_WORK(&ext_usb_chgdone_task, ext_usb_chgdone_irq_handler);
	INIT_DELAYED_WORK(&ext_usb_temp_task, ext_usb_temp_irq_handler);
	INIT_DELAYED_WORK(&ext_usb_bms_notify_task, ext_usb_bms_notify_check_handler);
	if (chip->cable_in_irq)
		INIT_DELAYED_WORK(&chip->ovp_check_work, ovp_check_worker);

	ext_charger_wq = create_singlethread_workqueue("ext_charger_wq");

	if (!ext_charger_wq) {
		pr_err("Failed to create ext_charger_wq workqueue.");
		goto free_chip;
	}

	rc = request_irqs(chip, pdev);
	if (rc) {
		pr_err("couldn't register interrupts rc=%d\n", rc);
		goto free_chip;
	}

	if (chip->cable_in_irq) {
		rc = request_any_context_irq(
				chip->cable_in_irq,
				cable_in_handler,
				IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING,
				"cable_in_irq", NULL);
		if (rc < 0) {
			pr_err("request cable_in_irq=%d failed!\n",
					chip->cable_in_irq);
			goto free_cable_in_irq;
		}
	}

	enable_irq_wake(chip->pmic_chg_irq[USBIN_VALID_IRQ]);
	enable_irq_wake(chip->pmic_chg_irq[DCIN_VALID_IRQ]);
	enable_irq_wake(chip->pmic_chg_irq[VBATDET_LOW_IRQ]);
	enable_irq_wake(chip->pmic_chg_irq[FASTCHG_IRQ]);
	if (!chip->is_embeded_batt)
		enable_irq_wake(chip->pmic_chg_irq[BATT_REMOVED_IRQ]);
	enable_irq_wake(chip->pmic_chg_irq[CHGHOT_IRQ]);
	if (chip->cable_in_irq)
		enable_irq_wake(chip->cable_in_irq);

	if (!(chip->cool_temp_dc == INT_MIN && chip->warm_temp_dc == INT_MIN) &&
		!flag_keep_charge_on && !flag_pa_recharge) {
		rc = configure_btm(chip);
		if (rc) {
			pr_err("couldn't register with btm rc=%d\n", rc);
			goto free_irq;
		}
	}

	create_debugfs_entries(chip);

	INIT_WORK(&chip->bms_notify.work, bms_notify);
	INIT_WORK(&chip->battery_id_valid_work, battery_id_valid);
	INIT_WORK(&chip->chghot_work, chghot_work);

	
	determine_initial_state(chip);

	if (0) {
		INIT_DELAYED_WORK(&chip->update_heartbeat_work,
							update_heartbeat);
		schedule_delayed_work(&chip->update_heartbeat_work,
				      round_jiffies_relative(msecs_to_jiffies
							(chip->update_time)));
	}
	htc_charger_event_notify(HTC_CHARGER_EVENT_READY);
	htc_gauge_event_notify(HTC_GAUGE_EVENT_READY);
	if (ovp)
		htc_charger_event_notify(HTC_CHARGER_EVENT_OVP);

	pr_info("%s: max_vbat=%u, cool_vbat=%u, warm_vbat=%u,"
			"wlc_tx_gpio=%d, vin_min=%d, vin_min_wlc=%d\n", __func__,
			chip->max_voltage_mv, chip->cool_bat_voltage, chip->warm_bat_voltage,
			chip->wlc_tx_gpio, chip->vin_min, chip->vin_min_wlc);
	return 0;

free_cable_in_irq:
	if (chip->cable_in_irq)
		free_irq(chip->cable_in_irq, 0);
free_irq:
	free_irqs(chip);
free_chip:
	kfree(chip);
	return rc;
}

static int __devexit pm8921_charger_remove(struct platform_device *pdev)
{
	struct pm8921_chg_chip *chip = platform_get_drvdata(pdev);

	if (chip->lockup_lpm_wrkarnd)
		regulator_put(chip->vreg_xoadc);
	free_irqs(chip);
	platform_set_drvdata(pdev, NULL);
	the_chip = NULL;
	kfree(chip);
	return 0;
}

static struct platform_driver pm8921_charger_driver = {
	.probe	= pm8921_charger_probe,
	.remove	= __devexit_p(pm8921_charger_remove),
	.driver	= {
		.name	= PM8921_CHARGER_DEV_NAME,
		.owner	= THIS_MODULE,
		.pm = &pm8921_charger_pm_ops,
	},
};

static int __init pm8921_charger_init(void)
{
	test_power_monitor =
		(get_kernel_flag() & KERNEL_FLAG_TEST_PWR_SUPPLY) ? 1 : 0;
	flag_keep_charge_on =
		(get_kernel_flag() & KERNEL_FLAG_KEEP_CHARG_ON) ? 1 : 0;
	flag_pa_recharge =
		(get_kernel_flag() & KERNEL_FLAG_PA_RECHARG_TEST) ? 1 : 0;
	flag_disable_wakelock =
		(get_kernel_flag() & KERNEL_FLAG_DISABLE_WAKELOCK) ? 1 : 0;
	flag_enable_BMS_Charger_log =
               (get_kernel_flag() & KERNEL_FLAG_ENABLE_BMS_CHARGER_LOG) ? 1 : 0;
	return platform_driver_register(&pm8921_charger_driver);
}

static void __exit pm8921_charger_exit(void)
{
	platform_driver_unregister(&pm8921_charger_driver);
}

late_initcall(pm8921_charger_init);
module_exit(pm8921_charger_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("PMIC8921 charger/battery driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:" PM8921_CHARGER_DEV_NAME);
