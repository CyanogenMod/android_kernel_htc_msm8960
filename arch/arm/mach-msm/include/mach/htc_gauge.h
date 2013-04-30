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
#ifndef __HTC_GAUGE_H__
#define __HTC_GAUGE_H__

enum htc_gauge_event {
	HTC_GAUGE_EVENT_READY = 0,
	HTC_GAUGE_EVENT_TEMP_ZONE_CHANGE,
	HTC_GAUGE_EVENT_ID_INVALID,
	HTC_GAUGE_EVENT_ID_VALID,
	HTC_GAUGE_EVENT_EOC,
	HTC_GAUGE_EVENT_BATT_REMOVED,
	HTC_GAUGE_EVENT_OVERLOAD,
};

struct htc_gauge {
	const char *name;
	int ready;
	int (*get_battery_voltage)(int *result);
	int (*get_battery_current)(int *result);
	int (*get_battery_temperature)(int *result);
	int (*get_battery_id)(int *result);
	int (*get_battery_soc)(int *result);
	int (*get_battery_cc)(int *result);
	int (*is_battery_temp_fault)(int *result);
	int (*is_battery_full)(int *result);
#if 0
	int (*dump_all)(void);
#endif
	int (*get_attr_text)(char *buf, int size);
	
	int (*register_lower_voltage_alarm_notifier)(void (*callback)(int));
	int (*enable_lower_voltage_alarm)(int enable);
	int (*set_lower_voltage_alarm_threshold)(int thres_mV);
};

int htc_gauge_event_notify(enum htc_gauge_event);
int htc_gauge_get_battery_voltage(int *result);

#endif
