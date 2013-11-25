/* Copyright (c) 2012 Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful;
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __SMB349_H__
#define __SMB349_H__

#ifdef CONFIG_HTC_BATT_8960
#include <mach/htc_charger.h>
#endif

#define SMB_CHG_CURR_REG			0x00
#define CHG_OTHER_CURRENT_REG		0x01
#define VAR_FUNC_REG			0x02
#define FLOAT_VOLTAGE_REG		0x03
#define CHG_CTRL_REG			0x04
#define STAT_TIMER_REG			0x05
#define PIN_ENABLE_CTRL_REG		0x06
#define THERM_CTRL_A_REG		0x07
#define SYSOK_USB3_SEL_REG		0x08
#define CTRL_FUNC_REG		0x09
#define OTG_TLIM_THERM_CNTRL_REG	0x0A
#define LIMIT_CELL_TEMP_MONI_REG 0x0B
#define FAULT_IRQ_REG				0x0C
#define STATUS_IRQ_REG				0x0D
#define SYSOK_REG				0x0E
#define AUTO_INPUT_VOL_DET_REG		0x10
#define I2C_BUS_REG				0x12
#define CMD_A_REG				0x30
#define CMD_B_REG				0x31
#define CMD_C_REG				0x33
#define IRQ_A_REG				0x35
#define IRQ_B_REG				0x36
#define IRQ_C_REG				0x37
#define IRQ_D_REG				0x38
#define IRQ_E_REG				0x39
#define IRQ_F_REG				0x3A
#define STATUS_A_REG				0x3B
#define STATUS_B_REG				0x3C
#define STATUS_C_REG				0x3D
#define STATUS_D_REG				0x3E
#define STATUS_E_REG				0x3F

#define CHG_STATUS_MASK		SMB349_MASK(2, 1)
#define CHG_ENABLE_STATUS_BIT		BIT(0)

#define FAST_CHG_CURRENT_MASK			SMB349_MASK(4, 4)


#define SUSPEND_MODE_MASK				SMB349_MASK(1, 2)

#define CHARGING_ENABLE_MASK				SMB349_MASK(1, 1)
#define VOLIATILE_WRITE_PERMISSIOIN_MASK		SMB349_MASK(1, 7)
#define CURRENT_TERMINATION_MASK			SMB349_MASK(1, 6)

#define OTG_ENABLE_MASK				SMB349_MASK(1, 4)
#define AICL_MASK				SMB349_MASK(1, 4)
#define USBCS_MASK				SMB349_MASK(1, 4)

#define AC_INPUT_CURRENT_LIMIT_MASK		SMB349_MASK(4, 0)
#define DC_INPUT_CURRENT_LIMIT_MASK		SMB349_MASK(4, 0)

#define PRE_CHG_CURRENT_MASK			SMB349_MASK(3, 5)

#define USB_HC_MODE_MASK			SMB349_MASK(1, 0)
#define TERMINATION_CURRENT_MASK		SMB349_MASK(3, 2)

#define USB_1_5_MODE_MASK			SMB349_MASK(1, 1)

#define IS_SUSPEND_MODE_MASK			SMB349_MASK(1, 7)

#define OTG_CURRENT_MASK			SMB349_MASK(2, 2)


#define PIN_CONTROL_ACTIVE_MASK			SMB349_MASK(2, 5)

#define PRE_CHG_TO_FAST_CHG_THRESH_MASK		SMB349_MASK(2, 6)
#define SWITCH_FREQ_MASK			SMB349_MASK(2, 6)
#define OTG_I2C_PIN_MASK			SMB349_MASK(2, 6)

#define PRE_CHG_TO_FAST_CHG_ENABLE_MASK		SMB349_MASK(1, 1)
#define FLOAT_VOLTAGE_MASK			SMB349_MASK(6, 0)


#define SUSPEND_MODE_BIT			BIT(2)
#define CHG_ENABLE_BIT				BIT(1)
#define VOLATILE_W_PERM_BIT			BIT(7)
#define USB_SELECTION_BIT			BIT(1)
#define SYSTEM_FET_ENABLE_BIT			BIT(7)
#define AICL_COMPLETE_STATUS_BIT		BIT(4)
#define AUTOMATIC_INPUT_CURR_LIMIT_BIT		BIT(4)
#define I2C_CONTROL_CHARGER_BIT			BIT(5)
#define AUTOMATIC_POWER_SOURCE_DETECTION_BIT	BIT(2)
#define BATT_OV_END_CHG_BIT			BIT(1)
#define VCHG_FUNCTION				BIT(0)
#define USB_1_5_MODE				BIT(1)
#define USB_HC_MODE				BIT(0)
#define POWER_OK_STATUS				BIT(0)
#define USBCS_PIN_MODE				BIT(4)
#define USBCS_REGISTER_MODE			0
#define CURR_TERM_END_CHG_BIT			BIT(6)

#define SMB349_HOT_TEMPERATURE_HARD_LIMIT_IRQ			7
#define SMB349_HOT_TEMPERATURE_HARD_LIMIT_STATUS                6
#define SMB349_COLD_TEMPERATURE_HARD_LIMIT_IRQ                  5
#define SMB349_COLD_TEMPERATURE_HARD_LIMIT_STATUS               4
#define SMB349_HOT_TEMPERATURE_SOFT_LIMIT_IRQ                   3
#define SMB349_HOT_TEMPERATURE_SOFT_LIMIT_STATUS                2
#define SMB349_COLD_TEMPERATURE_SOFT_LIMIT_IRQ                  1
#define SMB349_COLD_TEMPERATURE_SOFT_LIMIT_STATUS               0

#define SMB349_BATTERY_OVER_VOLTAGE_CONDITION_IRQ		7
#define SMB349_BATTERY_OVER_VOLTAGE_STATUS			6
#define SMB349_MISSING_BATTER_IRQ                               5
#define SMB349_MISSING_BATTERY_STATUS                           4
#define SMB349_LOW_BATTERY_VOLTAGE_IRQ                          3
#define SMB349_LOW_BATTERY_VOLTAGE_STATUS                       2
#define SMB349_PRE_TO_FAST_CHARGE_BATTERY_VOLTAGE_IRQ           1
#define SMB349_PRE_TO_FAST_CHARGE_BATTERY_VOLTAGE_STATUS        0

#define SMB349_INTERNAL_TEMPERATURE_LIMIT_IRQ			7
#define SMB349_INTERNAL_TEMPERATURE_LIMIT_STATUS                6
#define SMB349_RE_CHARGE_BATTERY_THRESHOLD_IRQ                  5
#define SMB349_RE_CHARGE_BATTERY_THRESHOLD_STATUS               4
#define SMB349_TAPER_CHARGING_MODE_IRQ                          3
#define SMB349_TAPER_CHARGING_MODE_STATUS                       2
#define SMB349_TERMINATION_CHARGING_CURRENT_HIT_IRQ             1
#define SMB349_TERMINATION_CHARGING_CURRENT_HIT_STATUS          0

#define SMB349_APSD_COMPLETED_IRQ				7
#define SMB349_APSD_COMPLETED_STATUS                            6
#define SMB349_AICL_COMPLETE_IRQ                                5
#define SMB349_AICL_COMPLETE_STATUS                             4
#define SMB349_RESERVED_3                                         3
#define SMB349_RESERVED_2                                         2
#define SMB349_CHARGE_TIMEOUT_IRQ                               1
#define SMB349_CHARGE_TIMEOUT_STATUS                            0

#define SMB349_DCIN_OVER_VOLTAGE_STATUS				2

#define SMB349_OTG_OVER_CURRENT_LIMIT_IRQ			7
#define SMB349_OTG_OVER_CURRENT_LIMIT_STATUS            	6
#define SMB349_OTG_BATTERY_UNDER_VOLTAGE_IRQ            	5
#define SMB349_OTG_BATTERY_UNDER_VOLTAGE_STATUS         	4
#define SMB349_OTG_DETECTION_IRQ                        	3
#define SMB349_OTG_DETECTION_STATUS                     	2
#define SMB349_POWER_OK_IRQ					1
#define SMB349_POWER_OK_STATUS					0


#define SMB349_FAST_CHG_MIN_MA	1000
#define SMB349_FAST_CHG_STEP_MA	200
#define SMB349_FAST_CHG_MAX_MA	4000
#define SMB349_FAST_CHG_SHIFT	4
#define SMB349_PRE_CHG_SHIFT	5

#define SMB349_OTG_I2C_PIN_SHIFT	6

#define SMB349_OTG_I2C_CONTROL		0x0
#define SMB349_OTG_PIN_CONTROL		0x1

#define SMB34X_OTG_CURR_LIMIT_SHIFT	2
#define SMB349_SWITCH_FREQ_SHIFT	6

#define SMB349_PIN_CONTROL_SHIFT	5
#define SMB349_USBCS_REGISTER_CTRL	0
#define SMB349_USBCS_PIN_CTRL		1

#define SMB349_NO_CHARGING				0
#define SMB349_PRE_CHARGING				1
#define SMB349_FAST_CHARGING				2
#define SMB349_TAPER_CHARGING				3



#define	SMB349_FLOAT_VOL_4000_MV	0x1B
#define	SMB349_FLOAT_VOL_4200_MV	0x25
#define	SMB349_FLOAT_VOL_4220_MV	0x26
#define	SMB349_FLOAT_VOL_4240_MV	0x27
#define	SMB349_FLOAT_VOL_4260_MV	0x28
#define	SMB349_FLOAT_VOL_4280_MV	0x29
#define	SMB349_FLOAT_VOL_4300_MV	0x2A
#define	SMB349_FLOAT_VOL_4320_MV	0x2B
#define	SMB349_FLOAT_VOL_4340_MV	0x2C
#define	SMB349_FLOAT_VOL_4350_MV	0x2D


#define SMB_OTG_CURR_LIMIT_250MA		0x0
#define SMB_OTG_CURR_LIMIT_500MA		0x1
#define SMB_OTG_CURR_LIMIT_750MA		0x2
#define SMB_OTG_CURR_LIMIT_1000MA		0x3


#define SMB349_I2C_CONTROL_ACTIVE_HIGH		0x0
#define SMB349_I2C_CONTROL_ACTIVE_LOW		0x1
#define SMB349_PIN_CONTROL_ACTIVE_HIGH		0x2
#define SMB349_PIN_CONTROL_ACTIVE_LOW		0x3

#define		AICL_RESULT_500MA	0x0
#define		AICL_RESULT_900MA       0x1
#define		AICL_RESULT_1000MA      0x2
#define		AICL_RESULT_1100MA      0x3
#define		AICL_RESULT_1200MA      0x4
#define		AICL_RESULT_1300MA      0x5
#define		AICL_RESULT_1500MA      0x6
#define		AICL_RESULT_1600MA      0x7
#define		AICL_RESULT_1700MA      0x8
#define		AICL_RESULT_1800MA      0x9
#define		AICL_RESULT_2000MA      0xa
#define		AICL_RESULT_2200MA      0xb
#define		AICL_RESULT_2400MA      0xc
#define		AICL_RESULT_2500MA      0xd
#define		AICL_RESULT_3000MA      0xe
#define		AICL_RESULT_3500MA      0xf

#define		SWITCH_FREQ_750KHZ	0x0
#define		SWITCH_FREQ_1MHZ       	0x1
#define		SWITCH_FREQ_1D5MHZ     	0x2
#define		SWITCH_FREQ_3MHZ       	0x3

#define SMB349_USB_MODE		0
#define SMB349_HC_MODE		1

#define AICL_ENABLE			1
#define AICL_DISABLE		0


#define		FAST_CHARGE_500MA	0x0
#define		FAST_CHARGE_600MA       0x1
#define		FAST_CHARGE_700MA       0x2
#define		FAST_CHARGE_800MA       0x3
#define		FAST_CHARGE_900MA       0x4
#define		FAST_CHARGE_1000MA      0x5
#define		FAST_CHARGE_1100MA      0x6
#define		FAST_CHARGE_1200MA      0x7
#define		FAST_CHARGE_1300MA      0x8
#define		FAST_CHARGE_1400MA      0x9
#define		FAST_CHARGE_1500MA      0xa
#define		FAST_CHARGE_1600MA      0xb
#define		FAST_CHARGE_1700MA      0xc
#define		FAST_CHARGE_1800MA      0xd
#define		FAST_CHARGE_1900MA      0xe
#define		FAST_CHARGE_2000MA      0xf


#define         SMB340_FASTCHG_1000MA      0x0
#define         SMB340_FASTCHG_1200MA      0x1
#define         SMB340_FASTCHG_1400MA      0x2
#define         SMB340_FASTCHG_1600MA      0x3
#define         SMB340_FASTCHG_1800MA      0x4
#define         SMB340_FASTCHG_2000MA      0x5
#define         SMB340_FASTCHG_2200MA      0x6
#define         SMB340_FASTCHG_2400MA      0x7
#define         SMB340_FASTCHG_2600MA      0x8
#define         SMB340_FASTCHG_2800MA      0x9
#define         SMB340_FASTCHG_3000MA      0xa
#define         SMB340_FASTCHG_3200MA      0xb
#define         SMB340_FASTCHG_3400MA      0xc
#define         SMB340_FASTCHG_3600MA      0xd
#define         SMB340_FASTCHG_3800MA      0xe
#define         SMB340_FASTCHG_4000MA      0xf



#define		DC_INPUT_500MA		0x0
#define		DC_INPUT_900MA		0x1
#define		DC_INPUT_1000MA		0x2
#define		DC_INPUT_1100MA		0x3
#define		DC_INPUT_1200MA		0x4
#define		DC_INPUT_1300MA		0x5
#define		DC_INPUT_1500MA		0x6
#define		DC_INPUT_1600MA		0x7
#define		DC_INPUT_1700MA		0x8
#define		DC_INPUT_1800MA		0x9
#define		DC_INPUT_2000MA		0xa
#define		DC_INPUT_2200MA		0xb
#define		DC_INPUT_2400MA		0xc
#define		DC_INPUT_2500MA		0xd
#define		DC_INPUT_3000MA		0xe
#define		DC_INPUT_3500MA		0xf

#define		PRECHG_CURR_100MA		0x0
#define		PRECHG_CURR_150MA		0x1
#define		PRECHG_CURR_200MA		0x2
#define		PRECHG_CURR_250MA		0x3
#define		PRECHG_CURR_300MA		0x4
#define		PRECHG_CURR_350MA		0x5
#define		PRECHG_CURR_50MA		0x6

#define         SMB340_PRECHG_CURR_200MA	0x0
#define         SMB340_PRECHG_CURR_300MA	0x1
#define         SMB340_PRECHG_CURR_400MA	0x2
#define         SMB340_PRECHG_CURR_500MA	0x3
#define         SMB340_PRECHG_CURR_600MA	0x4
#define         SMB340_PRECHG_CURR_700MA	0x5
#define         SMB340_PRECHG_CURR_100MA	0x6


#define		SMB_349		1
#define		SMB_340		2

enum smb349_thermal_state {
	STATE_HI_V_SCRN_ON = 1,
	STATE_LO_V_SCRN_ON,
	STATE_HI_V_SCRN_OFF,
	STATE_LO_V_SCRN_OFF,
};

struct smb349_chg_int_notifier {
	struct list_head notifier_link;
	const char *name;
	void (*func)(int int_reg, int value);
};

struct smb349_platform_data {
	int chg_susp_gpio;
	int chg_stat_gpio;
	int chg_current_ma;
	int chip_rev;
#ifdef CONFIG_SUPPORT_DQ_BATTERY
	int dq_result;
#endif
	int aicl_result_threshold;
	int dc_input_max;
	int aicl_on;
};

struct smb349_charger_batt_param {
	int max_voltage;
	int cool_bat_voltage;
	int warm_bat_voltage;
};
int smb349_set_AICL_mode(unsigned int enable);
int smb349_allow_fast_charging_setting(void);
int smb349_allow_volatile_wrtting(void);
int smb349_enable_charging(bool enable);
int smb349_eoc_notify(enum htc_extchg_event_type main_event);
int smb349_event_notify(enum htc_extchg_event_type extchg_event);
int smb349_temp_notify(enum htc_extchg_event_type main_event);
int smb349_enable_5v_output(bool mhl_in);
int smb349_config(void);
void smb349_dbg(void);
int  smb349_dump_reg(u8 reg);
void smb349_partial_reg_dump(void);
int smb349_dump_all(void);
int smb349_is_AICL_complete(void);
int smb349_is_AICL_enabled(void);
int smb349_is_charger_bit_low_active(void);
int smb349_is_charger_error(void);
int smb349_adjust_max_chg_vol(enum htc_extchg_event_type main_event);
int smb349_is_charging_enabled(int *result);
int smb349_is_batt_temp_fault_disable_chg(int *result);
int smb349_get_i2c_slave_id(void);
int smb349_is_hc_mode(void);
int smb349_is_usbcs_register_mode(void);
int smb349_masked_write(int reg, u8 mask, u8 val);
int smb349_not_allow_charging_cycle_end(void);
int smb349_enable_pwrsrc(bool enable);
int smb349_set_hsml_target_ma(int target_ma);
int smb349_set_pwrsrc_and_charger_enable(enum htc_power_source_type src, bool chg_enable, bool pwrsrc_enable);
int smb349_set_hc_mode(unsigned int enable);
int smb349_switch_usbcs_mode(int mode);
int smb349_limit_charge_enable(bool enable);
int smb349_is_batt_charge_enable(void);
int smb349_get_charging_src(int *result);
int smb349_get_charging_enabled(int *result);
int smb349_is_charger_overvoltage(int* result);
int smb349_charger_get_attr_text(char *buf, int size);
int smb349_start_charging(void *ctx);
int smb349_stop_charging(void *ctx);
bool smb349_is_trickle_charging(void *ctx);
#endif		
