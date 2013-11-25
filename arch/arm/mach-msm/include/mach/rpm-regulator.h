/* Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
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

#ifndef __ARCH_ARM_MACH_MSM_INCLUDE_MACH_RPM_REGULATOR_H
#define __ARCH_ARM_MACH_MSM_INCLUDE_MACH_RPM_REGULATOR_H

#include <linux/regulator/machine.h>

#define RPM_REGULATOR_DEV_NAME "rpm-regulator"

#include <mach/rpm-regulator-8660.h>
#include <mach/rpm-regulator-8960.h>
#include <mach/rpm-regulator-9615.h>
#include <mach/rpm-regulator-8974.h>
#include <mach/rpm-regulator-8930.h>

enum rpm_vreg_version {
	RPM_VREG_VERSION_8660,
	RPM_VREG_VERSION_8960,
	RPM_VREG_VERSION_9615,
	RPM_VREG_VERSION_8930,
	RPM_VREG_VERSION_MAX = RPM_VREG_VERSION_8930,
};

#define RPM_VREG_PIN_CTRL_NONE		0x00

enum rpm_vreg_state {
	RPM_VREG_STATE_OFF,
	RPM_VREG_STATE_ON,
};

enum rpm_vreg_freq {
	RPM_VREG_FREQ_NONE,
	RPM_VREG_FREQ_19p20,
	RPM_VREG_FREQ_9p60,
	RPM_VREG_FREQ_6p40,
	RPM_VREG_FREQ_4p80,
	RPM_VREG_FREQ_3p84,
	RPM_VREG_FREQ_3p20,
	RPM_VREG_FREQ_2p74,
	RPM_VREG_FREQ_2p40,
	RPM_VREG_FREQ_2p13,
	RPM_VREG_FREQ_1p92,
	RPM_VREG_FREQ_1p75,
	RPM_VREG_FREQ_1p60,
	RPM_VREG_FREQ_1p48,
	RPM_VREG_FREQ_1p37,
	RPM_VREG_FREQ_1p28,
	RPM_VREG_FREQ_1p20,
};

enum rpm_vreg_voltage_corner {
	RPM_VREG_CORNER_NONE = 1,
	RPM_VREG_CORNER_LOW,
	RPM_VREG_CORNER_NOMINAL,
	RPM_VREG_CORNER_HIGH,
};

enum rpm_vreg_voter {
	RPM_VREG_VOTER_REG_FRAMEWORK,	
	RPM_VREG_VOTER1,		
	RPM_VREG_VOTER2,		
	RPM_VREG_VOTER3,		
	RPM_VREG_VOTER4,		
	RPM_VREG_VOTER5,		
	RPM_VREG_VOTER6,		
	RPM_VREG_VOTER_COUNT,
};

struct rpm_regulator_init_data {
	struct regulator_init_data	init_data;
	int				id;
	int				sleep_selectable;
	int				system_uA;
	int				enable_time;
	unsigned			pull_down_enable;
	enum rpm_vreg_freq		freq;
	unsigned			pin_ctrl;
	int				pin_fn;
	int				force_mode;
	int				sleep_set_force_mode;
	int				power_mode;
	int				default_uV;
	unsigned			peak_uA;
	unsigned			avg_uA;
	enum rpm_vreg_state		state;
};

struct rpm_regulator_consumer_mapping {
	const char		*dev_name;
	const char		*supply;
	int			vreg_id;
	enum rpm_vreg_voter	voter;
	int			sleep_also;
};

struct rpm_regulator_platform_data {
	struct rpm_regulator_init_data		*init_data;
	int					num_regulators;
	enum rpm_vreg_version			version;
	int					vreg_id_vdd_mem;
	int					vreg_id_vdd_dig;
	bool					requires_tcxo_workaround;
	struct rpm_regulator_consumer_mapping	*consumer_map;
	int					consumer_map_len;
};

#ifdef CONFIG_MSM_RPM_REGULATOR
int rpm_vreg_set_voltage(int vreg_id, enum rpm_vreg_voter voter, int min_uV,
			 int max_uV, int sleep_also);

int rpm_vreg_set_frequency(int vreg_id, enum rpm_vreg_freq freq);

#else

static inline int rpm_vreg_set_voltage(int vreg_id, enum rpm_vreg_voter voter,
				       int min_uV, int max_uV, int sleep_also)
{
	return 0;
}

static inline int rpm_vreg_set_frequency(int vreg_id, enum rpm_vreg_freq freq)
{
	return 0;
}

#endif 

#endif
