/*
 * Copyright (C) 2007 HTC Incorporated
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __HTC_CHARGER_H__
#define __HTC_CHARGER_H__

#include <mach/board.h>

enum htc_charger_event {
	HTC_CHARGER_EVENT_READY = 0,
	HTC_CHARGER_EVENT_VBUS_OUT,
	HTC_CHARGER_EVENT_VBUS_IN,
	HTC_CHARGER_EVENT_SRC_NONE,
	HTC_CHARGER_EVENT_SRC_USB,
	HTC_CHARGER_EVENT_SRC_AC,
	HTC_CHARGER_EVENT_SRC_WIRELESS,
	HTC_CHARGER_EVENT_OVP,
	HTC_CHARGER_EVENT_OVP_RESOLVE,
	HTC_CHARGER_EVENT_SRC_MHL_AC,
	HTC_CHARGER_EVENT_SRC_INTERNAL,
	HTC_CHARGER_EVENT_SRC_CLEAR,
	HTC_CHARGER_EVENT_SRC_DETECTING,
	HTC_CHARGER_EVENT_SRC_UNKNOWN_USB,
	HTC_CHARGER_EVENT_SRC_UNDER_RATING,
	HTC_CHARGER_EVENT_SAFETY_TIMEOUT,
};

enum htc_charging_cfg {
	HTC_CHARGER_CFG_LIMIT = 0,	
	HTC_CHARGER_CFG_SLOW,
	HTC_CHARGER_CFG_FAST,
};

enum htc_power_source_type {
	
	
	HTC_PWR_SOURCE_TYPE_BATT = 0,
	HTC_PWR_SOURCE_TYPE_USB,
	HTC_PWR_SOURCE_TYPE_AC,
	HTC_PWR_SOURCE_TYPE_9VAC,
	HTC_PWR_SOURCE_TYPE_WIRELESS,
	HTC_PWR_SOURCE_TYPE_MHL_AC,
	HTC_PWR_SOURCE_TYPE_DETECTING,
	HTC_PWR_SOURCE_TYPE_UNKNOWN_USB,
	HTC_PWR_SOURCE_TYPE_PQM_FASTCHARGE,
	HTC_PWR_SOURCE_TYPE_MAX = 255,
};

enum htc_extchg_event_type {
	HTC_EXTCHG_EVENT_TYPE_TEMP_NORMAL = 1,
	HTC_EXTCHG_EVENT_TYPE_TEMP_WARM,
	HTC_EXTCHG_EVENT_TYPE_TEMP_COOL,
	HTC_EXTCHG_EVENT_TYPE_TEMP_HOT,
	HTC_EXTCHG_EVENT_TYPE_TEMP_COLD,
	HTC_EXTCHG_EVENT_TYPE_EOC_START_CHARGE,
	HTC_EXTCHG_EVENT_TYPE_EOC_STOP_CHARGE,
	HTC_EXTCHG_EVENT_TYPE_MAX = 255,
};

struct htc_charger {
	const char *name;
	int ready;
	int sw_safetytimer;
	int (*get_charging_source)(int *result);
	int (*get_charging_enabled)(int *result);
	int (*event_notify)(enum htc_extchg_event_type etype);
	int (*set_charger_enable)(bool enable);
	int (*set_pwrsrc_enable)(bool enable);
	int (*set_pwrsrc_and_charger_enable)
			(enum htc_power_source_type src,
			 bool chg_enable, bool pwrsrc_enable);
	int (*set_limit_charge_enable)(bool enable);
	int (*is_batt_charge_enable)(void);
	int (*toggle_charger)(void);
	int (*is_ovp)(int *result);
	int (*is_batt_temp_fault_disable_chg)(int *result);
	int (*charger_change_notifier_register)
			(struct t_cable_status_notifier *notifier);
	int (*max_input_current)(int target_ma);
	int (*dump_all)(void);
	int (*is_charging_enabled)(int *result);
	int (*is_under_rating)(int *result);
	int (*get_attr_text)(char *buf, int size);
	int (*enable_5v_output)(bool enable);
	int (*is_safty_timer_timeout)(int *result);
	int (*is_battery_full_eoc_stop)(int *result);
};

int htc_charger_event_notify(enum htc_charger_event);


#endif
