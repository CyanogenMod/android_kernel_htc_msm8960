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

#ifndef __PM8XXX_CHARGER_H
#define __PM8XXX_CHARGER_H

#include <linux/errno.h>
#include <linux/power_supply.h>
#ifdef CONFIG_HTC_BATT_8960
#include <mach/htc_charger.h>
#endif

#define PM8921_CHARGER_DEV_NAME	"pm8921-charger"

struct pm8xxx_charger_core_data {
	unsigned int	vbat_channel;
	unsigned int	batt_temp_channel;
	unsigned int	batt_id_channel;
};

enum pm8921_chg_cold_thr {
	PM_SMBC_BATT_TEMP_COLD_THR__LOW,
	PM_SMBC_BATT_TEMP_COLD_THR__HIGH
};

enum pm8921_chg_hot_thr	{
	PM_SMBC_BATT_TEMP_HOT_THR__LOW,
	PM_SMBC_BATT_TEMP_HOT_THR__HIGH
};

enum pm8921_usb_ov_threshold {
	PM_USB_OV_5P5V,
	PM_USB_OV_6V,
	PM_USB_OV_6P5V,
	PM_USB_OV_7V,
};

enum pm8921_usb_debounce_time {
	PM_USB_BYPASS_DEBOUNCER,
	PM_USB_DEBOUNCE_20P5MS,
	PM_USB_DEBOUNCE_40P5MS,
	PM_USB_DEBOUNCE_80P5MS,
};

enum pm8921_chg_led_src_config {
	LED_SRC_GND,
	LED_SRC_VPH_PWR,
	LED_SRC_5V,
	LED_SRC_MIN_VPH_5V,
	LED_SRC_BYPASS,
};


struct ext_usb_chg_pm8921 {
	const char	*name;
	void		*ctx;
	int		(*start_charging) (void *ctx);
	int		(*stop_charging) (void *ctx);
	bool		(*is_trickle) (void *ctx);
	struct htc_charger	*ichg;
};


struct pm8921_charger_platform_data {
	struct pm8xxx_charger_core_data	charger_cdata;
	unsigned int			safety_time;
	unsigned int			ttrkl_time;
	unsigned int			update_time;
	unsigned int			max_voltage;
	unsigned int			min_voltage;
	unsigned int			resume_voltage_delta;
	unsigned int			term_current;
	int				cool_temp;
	int				warm_temp;
	unsigned int			temp_check_period;
	unsigned int			max_bat_chg_current;
	unsigned int			cool_bat_chg_current;
	unsigned int			warm_bat_chg_current;
	unsigned int			cool_bat_voltage;
	unsigned int			warm_bat_voltage;
	unsigned int			(*get_batt_capacity_percent) (void);
	int64_t				batt_id_min;
	int64_t				batt_id_max;
	bool				keep_btm_on_suspend;
	bool				dc_unplug_check;
	int				trkl_voltage;
	int				weak_voltage;
	int				trkl_current;
	int				weak_current;
	int				vin_min;
	int				*thermal_mitigation;
	int				thermal_levels;
	int				mbat_in_gpio;
	int				is_embeded_batt;
	enum pm8921_chg_cold_thr	cold_thr;
	enum pm8921_chg_hot_thr		hot_thr;
	int				rconn_mohm;
	enum pm8921_chg_led_src_config	led_src_config;
	struct ext_usb_chg_pm8921	*ext_usb;
};

enum pm8921_charger_source {
	PM8921_CHG_SRC_NONE,
	PM8921_CHG_SRC_USB,
	PM8921_CHG_SRC_DC,
};

struct ext_chg_pm8921 {
	const char	*name;
	void		*ctx;
	int		(*start_charging) (void *ctx);
	int		(*stop_charging) (void *ctx);
	bool		(*is_trickle) (void *ctx);
};

#ifdef CONFIG_HTC_BATT_8960
struct pm8921_charger_batt_param {
	unsigned int max_voltage;
	unsigned int cool_bat_voltage;
	unsigned int warm_bat_voltage;
};
#endif 

#if defined(CONFIG_PM8921_CHARGER) || defined(CONFIG_PM8921_CHARGER_MODULE)
void pm8921_charger_vbus_draw(unsigned int mA);
int pm8921_charger_register_vbus_sn(void (*callback)(int));
void pm8921_charger_unregister_vbus_sn(void (*callback)(int));
int pm8921_charger_enable(bool enable);

int pm8921_is_usb_chg_plugged_in(void);

int pm8921_is_dc_chg_plugged_in(void);

int pm8921_is_pwr_src_plugged_in(void);

int pm8921_is_battery_present(void);

int pm8921_set_max_battery_charge_current(int ma);

int pm8921_disable_input_current_limit(bool disable);


int pm8921_set_usb_power_supply_type(enum power_supply_type type);

int pm8921_disable_source_current(bool disable);

int pm8921_regulate_input_voltage(int voltage);
bool pm8921_is_battery_charging(int *source);

int pm8921_batt_temperature(void);
int register_external_dc_charger(struct ext_chg_pm8921 *ext);

void unregister_external_dc_charger(struct ext_chg_pm8921 *ext);

int pm8921_usb_ovp_set_threshold(enum pm8921_usb_ov_threshold ov);

int pm8921_usb_ovp_set_hystersis(enum pm8921_usb_debounce_time ms);

int pm8921_usb_ovp_disable(int disable);

#ifdef CONFIG_HTC_BATT_8960
int pm8921_get_batt_voltage(int *result);

int pm8921_get_batt_temperature(int *result);

int pm8921_get_batt_id(int *result);

int pm8921_is_batt_temperature_fault(int *result);

int pm8921_is_batt_temp_fault_disable_chg(int *result);

int pm8921_is_batt_full(int *result);

int pm8921_get_charging_source(int *result);

int pm8921_get_charging_enabled(int *result);

int pm8921_pwrsrc_enable(bool enable);

int pm8921_set_pwrsrc_and_charger_enable(enum htc_power_source_type src,
		bool chg_enable, bool pwrsrc_enable);

int pm8921_limit_charge_enable(bool enable);

int pm8921_is_charger_ovp(int *result);

int pm8921_dump_all(void);

int pm8921_charger_get_attr_text(char *buf, int size);

int pm8921_charger_get_attr_text_with_ext_charger(char *buf, int size);

int pm8921_gauge_get_attr_text(char *buf, int size);
#endif 
void pm8921_chg_disable_usbin_valid_irq(void);
void pm8921_chg_enable_usbin_valid_irq(void);
#else
static inline void pm8921_charger_vbus_draw(unsigned int mA)
{
}
static inline int pm8921_charger_register_vbus_sn(void (*callback)(int))
{
	return -ENXIO;
}
static inline void pm8921_charger_unregister_vbus_sn(void (*callback)(int))
{
}
static inline int pm8921_charger_enable(bool enable)
{
	return -ENXIO;
}
static inline int pm8921_is_usb_chg_plugged_in(void)
{
	return -ENXIO;
}
static inline int pm8921_is_dc_chg_plugged_in(void)
{
	return -ENXIO;
}
static inline int pm8921_is_pwr_src_plugged_in(void)
{
	return -ENXIO;
}
static inline int pm8921_is_battery_present(void)
{
	return -ENXIO;
}
static inline int pm8921_disable_input_current_limit(bool disable)
{
	return -ENXIO;
}
static inline int pm8921_set_usb_power_supply_type(enum power_supply_type type)
{
	return -ENXIO;
}
static inline int pm8921_set_max_battery_charge_current(int ma)
{
	return -ENXIO;
}
static inline int pm8921_disable_source_current(bool disable)
{
	return -ENXIO;
}
static inline int pm8921_regulate_input_voltage(int voltage)
{
	return -ENXIO;
}
static inline bool pm8921_is_battery_charging(int *source)
{
	*source = PM8921_CHG_SRC_NONE;
	return 0;
}
static inline int pm8921_batt_temperature(void)
{
	return -ENXIO;
}
static inline int register_external_dc_charger(struct ext_chg_pm8921 *ext)
{
	pr_err("%s.not implemented.\n", __func__);
	return -ENODEV;
}
static inline void unregister_external_dc_charger(struct ext_chg_pm8921 *ext)
{
	pr_err("%s.not implemented.\n", __func__);
}
static inline int pm8921_usb_ovp_set_threshold(enum pm8921_usb_ov_threshold ov)
{
	return -ENXIO;
}
static inline int pm8921_usb_ovp_set_hystersis(enum pm8921_usb_debounce_time ms)
{
	return -ENXIO;
}
static inline int pm8921_usb_ovp_disable(int disable)
{
	return -ENXIO;
}
#ifdef CONFIG_HTC_BATT_8960
static inline int pm8921_get_batt_voltage(int *result)
{
	return -ENXIO;
}
static inline int pm8921_get_batt_temperature(int *result)
{
	return -ENXIO;
}
static inline int pm8921_get_batt_id(int *result)
{
	return -ENXIO;
}
static inline int pm8921_is_batt_temperature_fault(int *result)
{
	return -ENXIO;
}
static inline int pm8921_is_batt_temp_fault_disable_chg(int *result)
{
	return -ENXIO;
}
static inline int pm8921_is_batt_full(int *result)
{
	return -ENXIO;
}
static inline int pm8921_get_charging_source(int *result)
{
	return -ENXIO;
}
static inline int pm8921_get_charging_enabled(int *result)
{
	return -ENXIO;
}
static inline int pm8921_pwrsrc_enable(bool enable)
{
	return -ENXIO;
}
static inline int pm8921_set_pwrsrc_and_charger_enable(enum htc_power_source_type src,
		bool chg_enable, bool pwrsrc_enable)
{
	return -ENXIO;
}
static inline int pm8921_limit_charge_enable(bool enable)
{
	return -ENXIO;
}
static inline int pm8921_is_charger_ovp(int *result)
{
	return -ENXIO;
}
static inline int pm8921_charger_get_attr_text(char *buf, int size)
{
	return -ENXIO;
}
static inline int pm8921_gauge_get_attr_text(char *buf, int size)
{
	return -ENXIO;
}
#endif 
static inline void pm8921_chg_disable_usbin_valid_irq(void)
{
}
static inline void pm8921_chg_enable_usbin_valid_irq(void)
{
}
#endif

#endif
