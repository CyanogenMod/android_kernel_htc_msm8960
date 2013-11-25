/*
 * consumer.h -- SoC Regulator consumer support.
 *
 * Copyright (C) 2007, 2008 Wolfson Microelectronics PLC.
 *
 * Author: Liam Girdwood <lrg@slimlogic.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Regulator Consumer Interface.
 *
 * A Power Management Regulator framework for SoC based devices.
 * Features:-
 *   o Voltage and current level control.
 *   o Operating mode control.
 *   o Regulator status.
 *   o sysfs entries for showing client devices and status
 *
 * EXPERIMENTAL FEATURES:
 *   Dynamic Regulator operating Mode Switching (DRMS) - allows regulators
 *   to use most efficient operating mode depending upon voltage and load and
 *   is transparent to client drivers.
 *
 *   e.g. Devices x,y,z share regulator r. Device x and y draw 20mA each during
 *   IO and 1mA at idle. Device z draws 100mA when under load and 5mA when
 *   idling. Regulator r has > 90% efficiency in NORMAL mode at loads > 100mA
 *   but this drops rapidly to 60% when below 100mA. Regulator r has > 90%
 *   efficiency in IDLE mode at loads < 10mA. Thus regulator r will operate
 *   in normal mode for loads > 10mA and in IDLE mode for load <= 10mA.
 *
 */

#ifndef __LINUX_REGULATOR_CONSUMER_H_
#define __LINUX_REGULATOR_CONSUMER_H_

#include <linux/compiler.h>

struct device;
struct notifier_block;


#define REGULATOR_MODE_FAST			0x1
#define REGULATOR_MODE_NORMAL			0x2
#define REGULATOR_MODE_IDLE			0x4
#define REGULATOR_MODE_STANDBY			0x8


#define REGULATOR_EVENT_UNDER_VOLTAGE		0x01
#define REGULATOR_EVENT_OVER_CURRENT		0x02
#define REGULATOR_EVENT_REGULATION_OUT		0x04
#define REGULATOR_EVENT_FAIL			0x08
#define REGULATOR_EVENT_OVER_TEMP		0x10
#define REGULATOR_EVENT_FORCE_DISABLE		0x20
#define REGULATOR_EVENT_VOLTAGE_CHANGE		0x40
#define REGULATOR_EVENT_DISABLE 		0x80

struct regulator;

struct regulator_bulk_data {
	const char *supply;
	struct regulator *consumer;
	int min_uV;
	int max_uV;

	
	int ret;
};

#if defined(CONFIG_REGULATOR)

struct regulator *__must_check regulator_get(struct device *dev,
					     const char *id);
struct regulator *__must_check devm_regulator_get(struct device *dev,
					     const char *id);
struct regulator *__must_check regulator_get_exclusive(struct device *dev,
						       const char *id);
void regulator_put(struct regulator *regulator);
void devm_regulator_put(struct regulator *regulator);

int regulator_enable(struct regulator *regulator);
int regulator_disable(struct regulator *regulator);
int regulator_force_disable(struct regulator *regulator);
int regulator_is_enabled(struct regulator *regulator);
int regulator_disable_deferred(struct regulator *regulator, int ms);

int regulator_bulk_get(struct device *dev, int num_consumers,
		       struct regulator_bulk_data *consumers);
int devm_regulator_bulk_get(struct device *dev, int num_consumers,
			    struct regulator_bulk_data *consumers);
int regulator_bulk_enable(int num_consumers,
			  struct regulator_bulk_data *consumers);
int regulator_bulk_set_voltage(int num_consumers,
			  struct regulator_bulk_data *consumers);
int regulator_bulk_disable(int num_consumers,
			   struct regulator_bulk_data *consumers);
int regulator_bulk_force_disable(int num_consumers,
			   struct regulator_bulk_data *consumers);
void regulator_bulk_free(int num_consumers,
			 struct regulator_bulk_data *consumers);

int regulator_count_voltages(struct regulator *regulator);
int regulator_list_voltage(struct regulator *regulator, unsigned selector);
int regulator_is_supported_voltage(struct regulator *regulator,
				   int min_uV, int max_uV);
int regulator_set_voltage(struct regulator *regulator, int min_uV, int max_uV);
int regulator_set_voltage_time(struct regulator *regulator,
			       int old_uV, int new_uV);
int regulator_get_voltage(struct regulator *regulator);
int regulator_sync_voltage(struct regulator *regulator);
int regulator_set_current_limit(struct regulator *regulator,
			       int min_uA, int max_uA);
int regulator_get_current_limit(struct regulator *regulator);

int regulator_set_mode(struct regulator *regulator, unsigned int mode);
unsigned int regulator_get_mode(struct regulator *regulator);
int regulator_set_optimum_mode(struct regulator *regulator, int load_uA);

int regulator_register_notifier(struct regulator *regulator,
			      struct notifier_block *nb);
int regulator_unregister_notifier(struct regulator *regulator,
				struct notifier_block *nb);

void *regulator_get_drvdata(struct regulator *regulator);
void regulator_set_drvdata(struct regulator *regulator, void *data);

#else

static inline struct regulator *__must_check regulator_get(struct device *dev,
	const char *id)
{
	return NULL;
}

static inline struct regulator *__must_check
devm_regulator_get(struct device *dev, const char *id)
{
	return NULL;
}

static inline void regulator_put(struct regulator *regulator)
{
}

static inline void devm_regulator_put(struct regulator *regulator)
{
}

static inline int regulator_enable(struct regulator *regulator)
{
	return 0;
}

static inline int regulator_disable(struct regulator *regulator)
{
	return 0;
}

static inline int regulator_force_disable(struct regulator *regulator)
{
	return 0;
}

static inline int regulator_disable_deferred(struct regulator *regulator,
					     int ms)
{
	return 0;
}

static inline int regulator_is_enabled(struct regulator *regulator)
{
	return 1;
}

static inline int regulator_bulk_get(struct device *dev,
				     int num_consumers,
				     struct regulator_bulk_data *consumers)
{
	return 0;
}

static inline int devm_regulator_bulk_get(struct device *dev, int num_consumers,
					  struct regulator_bulk_data *consumers)
{
	return 0;
}

static inline int regulator_bulk_enable(int num_consumers,
					struct regulator_bulk_data *consumers)
{
	return 0;
}

static inline int regulator_bulk_disable(int num_consumers,
					 struct regulator_bulk_data *consumers)
{
	return 0;
}

static inline int regulator_bulk_force_disable(int num_consumers,
					struct regulator_bulk_data *consumers)
{
	return 0;
}

static inline void regulator_bulk_free(int num_consumers,
				       struct regulator_bulk_data *consumers)
{
}

static inline int regulator_count_voltages(struct regulator *regulator)
{
	return 0;
}

static inline int regulator_set_voltage(struct regulator *regulator,
					int min_uV, int max_uV)
{
	return 0;
}

static inline int regulator_get_voltage(struct regulator *regulator)
{
	return 0;
}

static inline int regulator_set_current_limit(struct regulator *regulator,
					     int min_uA, int max_uA)
{
	return 0;
}

static inline int regulator_get_current_limit(struct regulator *regulator)
{
	return 0;
}

static inline int regulator_set_mode(struct regulator *regulator,
	unsigned int mode)
{
	return 0;
}

static inline unsigned int regulator_get_mode(struct regulator *regulator)
{
	return REGULATOR_MODE_NORMAL;
}

static inline int regulator_set_optimum_mode(struct regulator *regulator,
					int load_uA)
{
	return REGULATOR_MODE_NORMAL;
}

static inline int regulator_register_notifier(struct regulator *regulator,
			      struct notifier_block *nb)
{
	return 0;
}

static inline int regulator_unregister_notifier(struct regulator *regulator,
				struct notifier_block *nb)
{
	return 0;
}

static inline void *regulator_get_drvdata(struct regulator *regulator)
{
	return NULL;
}

static inline void regulator_set_drvdata(struct regulator *regulator,
	void *data)
{
}

#endif

#endif
