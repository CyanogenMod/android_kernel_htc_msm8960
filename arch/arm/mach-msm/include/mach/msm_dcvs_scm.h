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
#ifndef _ARCH_ARM_MACH_MSM_MSM_DCVS_SCM_H
#define _ARCH_ARM_MACH_MSM_MSM_DCVS_SCM_H

enum msm_dcvs_scm_event {
	MSM_DCVS_SCM_IDLE_ENTER,
	MSM_DCVS_SCM_IDLE_EXIT,
	MSM_DCVS_SCM_QOS_TIMER_EXPIRED,
	MSM_DCVS_SCM_CLOCK_FREQ_UPDATE,
	MSM_DCVS_SCM_ENABLE_CORE,
	MSM_DCVS_SCM_RESET_CORE,
};

struct msm_dcvs_algo_param {
	uint32_t slack_time_us;
	uint32_t scale_slack_time;
	uint32_t scale_slack_time_pct;
	uint32_t disable_pc_threshold;
	uint32_t em_window_size;
	uint32_t em_max_util_pct;
	uint32_t ss_window_size;
	uint32_t ss_util_pct;
	uint32_t ss_iobusy_conv;
};

struct msm_dcvs_freq_entry {
	uint32_t freq; 
	uint32_t idle_energy;
	uint32_t active_energy;
};

struct msm_dcvs_core_param {
	uint32_t max_time_us;
	uint32_t num_freq; 
};


#ifdef CONFIG_MSM_DCVS
extern int msm_dcvs_scm_init(size_t size);

extern int msm_dcvs_scm_create_group(uint32_t id);

extern int msm_dcvs_scm_register_core(uint32_t core_id, uint32_t group_id,
		struct msm_dcvs_core_param *param,
		struct msm_dcvs_freq_entry *freq);

extern int msm_dcvs_scm_set_algo_params(uint32_t core_id,
		struct msm_dcvs_algo_param *param);

extern int msm_dcvs_scm_event(uint32_t core_id,
		enum msm_dcvs_scm_event event_id,
		uint32_t param0, uint32_t param1,
		uint32_t *ret0, uint32_t *ret1);

#else
static inline int msm_dcvs_scm_init(uint32_t phy, size_t bytes)
{ return -ENOSYS; }
static inline int msm_dcvs_scm_create_group(uint32_t id)
{ return -ENOSYS; }
static inline int msm_dcvs_scm_register_core(uint32_t core_id,
		uint32_t group_id,
		struct msm_dcvs_core_param *param,
		struct msm_dcvs_freq_entry *freq)
{ return -ENOSYS; }
static inline int msm_dcvs_scm_set_algo_params(uint32_t core_id,
		struct msm_dcvs_algo_param *param)
{ return -ENOSYS; }
static inline int msm_dcvs_scm_event(uint32_t core_id,
		enum msm_dcvs_scm_event event_id,
		uint32_t param0, uint32_t param1,
		uint32_t *ret0, uint32_t *ret1)
{ return -ENOSYS; }
#endif

#endif
