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

/* for cable_detect struct */
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
};

enum htc_charging_cfg {
	HTC_CHARGER_CFG_LIMIT = 0,	/* cdma talking */
	HTC_CHARGER_CFG_SLOW,
	HTC_CHARGER_CFG_FAST,
/*	HTC_CHARGER_CFG_9VFAST,
	HTC_CHARGER_CFG_WARM,
	HTC_CHARGER_CFG_COOL,*/
};

enum htc_power_source_type {
	/* HTC_PWR_SOURCE_TYPE_DRAIN, */
	/* HTC_PWR_SOURCE_TYPE_UNKNOWN, */
	HTC_PWR_SOURCE_TYPE_BATT = 0,
	HTC_PWR_SOURCE_TYPE_USB,
	HTC_PWR_SOURCE_TYPE_AC,
	HTC_PWR_SOURCE_TYPE_9VAC,
	HTC_PWR_SOURCE_TYPE_WIRELESS,
};

struct htc_charger {
	const char *name;
	int ready;
	int (*get_charging_source)(int *result);
	int (*get_charging_enabled)(int *result);
	int (*set_charger_enable)(bool enable);
	int (*set_pwrsrc_enable)(bool enable);
	int (*set_pwrsrc_and_charger_enable)
			(enum htc_power_source_type src,
			 bool chg_enable, bool pwrsrc_enable);
	int (*set_limit_charge_enable)(bool enable);
	int (*toggle_charger)(void);
	int (*is_ovp)(int *result);
	int (*is_batt_temp_fault_disable_chg)(int *result);
	int (*charger_change_notifier_register)
			(struct t_cable_status_notifier *notifier);
	int (*dump_all)(void);
	int (*get_attr_text)(char *buf, int size);
};

/* let driver including this .h can notify event to htc battery */
int htc_charger_event_notify(enum htc_charger_event);


#endif
