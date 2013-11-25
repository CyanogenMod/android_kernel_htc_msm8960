/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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
#ifndef __MFD_PM8XXX_BATT_ALARM_H__
#define __MFD_PM8XXX_BATT_ALARM_H__

#include <linux/bitops.h>
#include <linux/errno.h>
#include <linux/notifier.h>

#define PM8XXX_BATT_ALARM_DEV_NAME	"pm8xxx-batt-alarm"

struct pm8xxx_batt_alarm_core_data {
	char	*irq_name;
	u16	reg_addr_threshold;
	u16	reg_addr_ctrl1;
	u16	reg_addr_ctrl2;
	u16	reg_addr_pwm_ctrl;
};

enum pm8xxx_batt_alarm_comparator {
	PM8XXX_BATT_ALARM_LOWER_COMPARATOR,
	PM8XXX_BATT_ALARM_UPPER_COMPARATOR,
};

enum pm8xxx_batt_alarm_hold_time {
	PM8XXX_BATT_ALARM_HOLD_TIME_0p125_MS = 0,
	PM8XXX_BATT_ALARM_HOLD_TIME_0p25_MS,
	PM8XXX_BATT_ALARM_HOLD_TIME_0p5_MS,
	PM8XXX_BATT_ALARM_HOLD_TIME_1_MS,
	PM8XXX_BATT_ALARM_HOLD_TIME_2_MS,
	PM8XXX_BATT_ALARM_HOLD_TIME_4_MS,
	PM8XXX_BATT_ALARM_HOLD_TIME_8_MS,
	PM8XXX_BATT_ALARM_HOLD_TIME_16_MS,
};

#define PM8XXX_BATT_ALARM_STATUS_BELOW_LOWER	BIT(0)
#define PM8XXX_BATT_ALARM_STATUS_ABOVE_UPPER	BIT(1)

#if defined(CONFIG_MFD_PM8XXX_BATT_ALARM) \
	|| defined(CONFIG_MFD_PM8XXX_BATT_ALARM_MODULE)

int pm8xxx_batt_alarm_enable(enum pm8xxx_batt_alarm_comparator comparator);

int pm8xxx_batt_alarm_disable(enum pm8xxx_batt_alarm_comparator comparator);

int pm8xxx_batt_alarm_state_set(int enable_lower_comparator,
				int enable_upper_comparator);

int pm8xxx_batt_alarm_threshold_set(
	enum pm8xxx_batt_alarm_comparator comparator, int threshold_mV);

int pm8xxx_batt_alarm_status_read(void);

int pm8xxx_batt_alarm_register_notifier(struct notifier_block *nb);

int pm8xxx_batt_alarm_unregister_notifier(struct notifier_block *nb);

int pm8xxx_batt_alarm_hold_time_set(enum pm8xxx_batt_alarm_hold_time hold_time);

int pm8xxx_batt_alarm_pwm_rate_set(int use_pwm, int clock_scaler,
				   int clock_divider);

int pm8xxx_batt_lower_alarm_register_notifier(void (*callback)(int));

int pm8xxx_batt_lower_alarm_enable(int enable);

int pm8xxx_batt_lower_alarm_threshold_set(int threshold_mV);
#else

static inline int
pm8xxx_batt_alarm_enable(enum pm8xxx_batt_alarm_comparator comparator)
{ return -ENODEV; }

static inline int
pm8xxx_batt_alarm_disable(enum pm8xxx_batt_alarm_comparator comparator)
{ return -ENODEV; }

static inline int
pm8xxx_batt_alarm_threshold_set(enum pm8xxx_batt_alarm_comparator comparator,
				int threshold_mV)
{ return -ENODEV; }

static inline int pm8xxx_batt_alarm_status_read(void)
{ return -ENODEV; }

static inline int pm8xxx_batt_alarm_register_notifier(struct notifier_block *nb)
{ return -ENODEV; }

static inline int
pm8xxx_batt_alarm_unregister_notifier(struct notifier_block *nb)
{ return -ENODEV; }

static inline int
pm8xxx_batt_alarm_hold_time_set(enum pm8xxx_batt_alarm_hold_time hold_time)
{ return -ENODEV; }

static inline int
pm8xxx_batt_alarm_pwm_rate_set(int use_pwm, int clock_scaler, int clock_divider)
{ return -ENODEV; }

static inline int pm8xxx_batt_lower_alarm_register_notifier(
				void (*callback)(int))
{ return -ENODEV; }

static inline int pm8xxx_batt_lower_alarm_enable(int enable)
{ return -ENODEV; }

static inline int pm8xxx_batt_lower_alarm_threshold_set(int threshold_mV)
{ return -ENODEV; }
#endif


#endif 
