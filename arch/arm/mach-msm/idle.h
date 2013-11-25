/* Copyright (c) 2007-2009,2012 Code Aurora Forum. All rights reserved.
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

#ifndef _ARCH_ARM_MACH_MSM_IDLE_H_
#define _ARCH_ARM_MACH_MSM_IDLE_H_

#ifdef CONFIG_MSM_CPU_AVS
#define CPU_SAVED_STATE_SIZE (4 * 11 + 4 * 10 + 4 * 3)
#else
#define CPU_SAVED_STATE_SIZE (4 * 11 + 4 * 10)
#endif

#define ON	1
#define OFF	0
#define TARGET_IS_8625	1
#define POWER_COLLAPSED 1

#ifndef __ASSEMBLY__

int msm_arch_idle(void);
int msm_pm_collapse(void);
void msm_pm_collapse_exit(void);
extern void *msm_saved_state;
extern unsigned long msm_saved_state_phys;

#ifdef CONFIG_CPU_V7
void msm_pm_boot_entry(void);
void msm_pm_set_l2_flush_flag(unsigned int flag);
void get_pm_boot_vector_symbol_address(unsigned *addr);
extern unsigned long msm_pm_pc_pgd;
extern unsigned long msm_pm_boot_vector[NR_CPUS];
extern uint32_t target_type;
extern uint32_t apps_power_collapse;
extern uint32_t *l2x0_base_addr;
#else
static inline void msm_pm_set_l2_flush_flag(unsigned int flag)
{
	
}
static inline void msm_pm_boot_entry(void)
{
	
}
static inline void msm_pm_write_boot_vector(unsigned int cpu,
						unsigned long address)
{
	
}
#endif
#endif
#endif
