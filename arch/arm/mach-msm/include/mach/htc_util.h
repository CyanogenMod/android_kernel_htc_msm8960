#ifndef __ARCH_ARM_MACH_MSM_HTC_UTIL_H
#define __ARCH_ARM_MACH_MSM_HTC_UTIL_H
extern int pm_monitor_enabled;  

void htc_pm_monitor_init(void);
void htc_monitor_init(void);
void htc_kernel_top_monitor_init(void);
void htc_top_monitor_init(void);
void htc_idle_stat_add(int sleep_mode, u32 time);
void htc_xo_block_clks_count(void);

#endif
