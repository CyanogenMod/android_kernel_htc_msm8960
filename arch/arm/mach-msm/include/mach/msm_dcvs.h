/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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
#ifndef _ARCH_ARM_MACH_MSM_MSM_DCVS_H
#define _ARCH_ARM_MACH_MSM_MSM_DCVS_H

#include <mach/msm_dcvs_scm.h>

#define CORE_NAME_MAX (32)
#define CORES_MAX (10)

enum msm_core_idle_state {
	MSM_DCVS_IDLE_ENTER,
	MSM_DCVS_IDLE_EXIT,
};

enum msm_core_control_event {
	MSM_DCVS_ENABLE_IDLE_PULSE,
	MSM_DCVS_DISABLE_IDLE_PULSE,
	MSM_DCVS_ENABLE_HIGH_LATENCY_MODES,
	MSM_DCVS_DISABLE_HIGH_LATENCY_MODES,
};

struct msm_dcvs_idle {
	const char *core_name;
	
	int (*enable)(struct msm_dcvs_idle *self,
			enum msm_core_control_event event);
};

extern int msm_dcvs_idle_source_register(struct msm_dcvs_idle *drv);

extern int msm_dcvs_idle_source_unregister(struct msm_dcvs_idle *drv);

int msm_dcvs_idle(int handle, enum msm_core_idle_state state,
		uint32_t iowaited);

struct msm_dcvs_core_info {
	struct msm_dcvs_freq_entry *freq_tbl;
	struct msm_dcvs_core_param core_param;
	struct msm_dcvs_algo_param algo_param;
};

extern int msm_dcvs_register_core(const char *core_name, uint32_t group_id,
		struct msm_dcvs_core_info *info);

struct msm_dcvs_freq {
	const char *core_name;
	
	int (*set_frequency)(struct msm_dcvs_freq *self,
			unsigned int freq);
	unsigned int (*get_frequency)(struct msm_dcvs_freq *self);
};

extern int msm_dcvs_freq_sink_register(struct msm_dcvs_freq *drv);

extern int msm_dcvs_freq_sink_unregister(struct msm_dcvs_freq *drv);

#endif
