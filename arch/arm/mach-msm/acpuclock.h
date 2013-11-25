/*
 * MSM architecture CPU clock driver header
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2007-2012, Code Aurora Forum. All rights reserved.
 * Author: San Mehat <san@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ARCH_ARM_MACH_MSM_ACPUCLOCK_H
#define __ARCH_ARM_MACH_MSM_ACPUCLOCK_H

enum setrate_reason {
	SETRATE_CPUFREQ = 0,
	SETRATE_SWFI,
	SETRATE_PC,
	SETRATE_HOTPLUG,
	SETRATE_INIT,
};

struct acpuclk_pdata {
	unsigned long max_speed_delta_khz;
	unsigned int max_axi_khz;
};

struct acpuclk_data {
	unsigned long (*get_rate)(int cpu);
	int (*set_rate)(int cpu, unsigned long rate, enum setrate_reason);
	uint32_t switch_time_us;
	unsigned long power_collapse_khz;
	unsigned long wait_for_irq_khz;
};

unsigned long acpuclk_get_rate(int cpu);

int acpuclk_set_rate(int cpu, unsigned long rate, enum setrate_reason);

uint32_t acpuclk_get_switch_time(void);

unsigned long acpuclk_power_collapse(void);

unsigned long acpuclk_wait_for_irq(void);

void acpuclk_register(struct acpuclk_data *data);

#endif
