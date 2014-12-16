#include <linux/sched.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/wakelock.h>
#include <linux/time.h>
#include <linux/kernel_stat.h>
#include <linux/rtc.h>
#include <linux/irq.h>
#include <linux/time.h>
#include <linux/wakelock.h>
#include <../clock.h>
#include <../clock-local.h>
#include "pm.h"
#include <linux/vmalloc.h>
#include <mach/board.h>
#include <mach/rpm.h>
#ifdef CONFIG_PERFLOCK
#include <mach/perflock.h>
#endif
#include <mach/rpm-8960.h>
#include <mach/rpm-8064.h>
#include <mach/msm_xo.h>
#include <linux/gpio.h>
#include <asm/system_info.h>
#include <linux/tick.h>
#include "htc_cpu_usage_stats.h"    

#include "board-monarudo.h"
#define HTC_PM_STATSTIC_DELAY			10000

#define USE_STATISTICS_STRATEGY_CONTINUOUS_3    0
#define SEND_KOBJECT_UEVENT_ENV_ENABLED         0

#define NUM_BUSY_THREAD_CHECK                   5
#define HTC_KERNEL_TOP_DELAY                    60000
#define HTC_KERNEL_TOP_CPU_USAGE_THRESHOLD      30
#define BUFFER_WARN_LEN                         64
#define BUFFER_TEMP_LEN                         32
#if USE_STATISTICS_STRATEGY_CONTINUOUS_3
#define MAX_CONSECUTIVE_THRES_TIMES             3
#else 
#define MAX_OVER_THRES_TIMES                    5
#define HTC_KERNEL_TOP_MONITOR_PERIOD           10
#define MAX_PROCESS_MONITOR_ARRAY_FIELDS        (HTC_KERNEL_TOP_MONITOR_PERIOD * NUM_BUSY_THREAD_CHECK)
#define PROCESS_MONITOR_ARRAY_5_IN_10_SIZE      MAX_PROCESS_MONITOR_ARRAY_FIELDS
#define BUFFER_WARN_5_IN_10_SIZE                HTC_KERNEL_TOP_MONITOR_PERIOD
#endif 

#if USE_STATISTICS_STRATEGY_CONTINUOUS_3
struct current_pid_found {
    unsigned char pid_found;
    unsigned char need_to_add;
};
#endif 

extern unsigned int get_tamper_sf(void);
extern unsigned long get_kernel_flag(void);

#define FORCE_CHARGE			(1<<2)
#define Y_CABLE			(1<<26)

struct process_monitor_statistic {
    unsigned int pid;
    char *ppid_name;
    unsigned int cnt;
    unsigned char set_warn;
    unsigned char is_found;
#if SEND_KOBJECT_UEVENT_ENV_ENABLED
    unsigned char sent_uevent;
#endif 
};

int pm_monitor_enabled = 0; 

#if USE_STATISTICS_STRATEGY_CONTINUOUS_3
static struct current_pid_found current_pid_found_array[NUM_BUSY_THREAD_CHECK];
static struct process_monitor_statistic process_monitor_continuous_3_array[NUM_BUSY_THREAD_CHECK];
#define SIZE_OF_CURR_PID_FOUND_ARRAY                    ARRAY_SIZE(current_pid_found_array)
#define SIZE_OF_PROCESS_MONITOR_CONTINUOUS_3_ARRAY      ARRAY_SIZE(process_monitor_continuous_3_array)
#else 
static int statistic_monitor_period = 1;

static struct process_monitor_statistic process_monitor_5_in_10_array[PROCESS_MONITOR_ARRAY_5_IN_10_SIZE];
#define SIZE_OF_PROCESS_MONITOR_5_IN_10_ARRAY           ARRAY_SIZE(process_monitor_5_in_10_array)
#endif 

static void htc_debug_flag_show(void)
{

	unsigned int cfg = 0 ;
    
	if(get_tamper_sf() == 0){
		if((get_kernel_flag()& FORCE_CHARGE) || (get_kernel_flag()& Y_CABLE))
			cfg = 1 ;
		pr_info("[K] CFG:0x%x", cfg);
	 }
}

#ifdef arch_idle_time

static cputime64_t get_idle_time(int cpu)
{
	cputime64_t idle;

	idle = kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
	if (cpu_online(cpu) && !nr_iowait_cpu(cpu))
		idle += arch_idle_time(cpu);
	return idle;
}

static cputime64_t get_iowait_time(int cpu)
{
	cputime64_t iowait;

	iowait = kcpustat_cpu(cpu).cpustat[CPUTIME_IOWAIT];
	if (cpu_online(cpu) && nr_iowait_cpu(cpu))
		iowait += arch_idle_time(cpu);
	return iowait;
}

#else

static u64 get_idle_time(int cpu)
{
	u64 idle, idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		
		idle = kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
	else
		idle = usecs_to_cputime64(idle_time);

	return idle;
}

static u64 get_iowait_time(int cpu)
{
	u64 iowait, iowait_time = get_cpu_iowait_time_us(cpu, NULL);

	if (iowait_time == -1ULL)
		
		iowait = kcpustat_cpu(cpu).cpustat[CPUTIME_IOWAIT];
	else
		iowait = usecs_to_cputime64(iowait_time);

	return iowait;
}

#endif


extern void htc_print_active_wake_locks(int type);
extern void htc_show_interrupts(void);
extern void htc_timer_stats_onoff(char onoff);
extern void htc_timer_stats_show(u16 water_mark);

static int msm_htc_util_delay_time = HTC_PM_STATSTIC_DELAY;
module_param_named(
	delay_time, msm_htc_util_delay_time, int, S_IRUGO | S_IWUSR | S_IWGRP
);

static int msm_htc_util_top_delay_time = HTC_KERNEL_TOP_DELAY;  

static struct workqueue_struct *htc_pm_monitor_wq = NULL;
struct delayed_work htc_pm_delayed_work;

static spinlock_t lock;

static struct workqueue_struct *htc_kernel_top_monitor_wq = NULL;
struct delayed_work htc_kernel_top_delayed_work;

static spinlock_t lock_accu;

void htc_xo_vddmin_stat_show(void);
void htc_kernel_top(void);
#if USE_STATISTICS_STRATEGY_CONTINUOUS_3
void clear_current_pid_found_array(void);
#endif 
void clear_process_monitor_array(struct process_monitor_statistic *pArray, int array_size);
void htc_kernel_top_accumulation(void);

uint32_t previous_xo_count = 0;
uint64_t previous_xo_time = 0;
uint32_t previous_vddmin_count = 0;
uint64_t previous_vddmin_time = 0;

struct st_htc_idle_statistic {
	u32 count;
	u32 time;
};

struct st_htc_idle_statistic htc_idle_Stat[CONFIG_NR_CPUS][3];

void htc_idle_stat_clear(void)
{
	memset(htc_idle_Stat, 0, sizeof(htc_idle_Stat));
}

void htc_idle_stat_add(int sleep_mode, u32 time)
{
	unsigned int cpu = smp_processor_id();

	if (cpu < CONFIG_NR_CPUS) {
		switch (sleep_mode) {
		case MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT:
			htc_idle_Stat[cpu][0].count++;
			htc_idle_Stat[cpu][0].time += time;
			break;
		case MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE:
			htc_idle_Stat[cpu][1].count++;
			htc_idle_Stat[cpu][1].time += time;
			break;
		case MSM_PM_SLEEP_MODE_POWER_COLLAPSE:
			htc_idle_Stat[cpu][2].count++;
			htc_idle_Stat[cpu][2].time += time;
			break;
		default:
			break;
		}
	}
}

void htc_idle_stat_show(u32 total_time)
{
	int i = 0, cpu = 0;
	u32 idle_time = 0;
	total_time *= 1000;

	printk("[K] cpu_id\tcpu_state\tidle_count\tidle_time\n");
	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++) {
		for (i = 0; i < 3 ; i++) {
			if (htc_idle_Stat[cpu][i].count) {
				idle_time += htc_idle_Stat[cpu][i].time;
				printk("[K]\t%d\tC%d\t\t%d\t\t%dms\n"
					,cpu , i, htc_idle_Stat[cpu][i].count, htc_idle_Stat[cpu][i].time / 1000);
			}
		}
	}
	htc_xo_vddmin_stat_show();
	msm_rpm_dump_stat();
}

#if 0
static DECLARE_BITMAP(msm_pm_clocks_no_tcxo_shutdown, MAX_NR_CLKS);
#endif
u32 count_xo_block_clk_array[MAX_NR_CLKS] = {0};
void htc_xo_block_clks_count_clear(void)
{
	memset(count_xo_block_clk_array, 0, sizeof(count_xo_block_clk_array));
}
void htc_xo_block_clks_count(void)
{
#if 0
	int ret, i;
	ret = msm_clock_require_tcxo(msm_pm_clocks_no_tcxo_shutdown, MAX_NR_CLKS);
	if (ret) {
		int blk_xo = 0;
		for_each_set_bit(i, msm_pm_clocks_no_tcxo_shutdown, MAX_NR_CLKS) {
			blk_xo = local_clk_src_xo(i);
			if (blk_xo)
				count_xo_block_clk_array[i]++;
		}
	}
#endif
}

void htc_xo_block_clks_count_show(void)
{
#if 0
	int ret, i;
	ret = msm_clock_require_tcxo(msm_pm_clocks_no_tcxo_shutdown, MAX_NR_CLKS);
	if (ret) {
		char clk_name[20] = "\0";
		for_each_set_bit(i, msm_pm_clocks_no_tcxo_shutdown, MAX_NR_CLKS) {
			if (count_xo_block_clk_array[i] > 0) {
				clk_name[0] = '\0';
				ret = msm_clock_get_name_noirq(i, clk_name, sizeof(clk_name));
				pr_info("%s (id=%d): %d\n", clk_name, ret, count_xo_block_clk_array[i]);
			}
		}
	}
#endif
}

void htc_xo_vddmin_stat_show(void)
{
	uint32_t xo_count = 0;
	uint64_t xo_time = 0;
	uint32_t vddmin_count = 0;
	uint64_t vddmin_time = 0;
	if (htc_get_xo_vdd_min_info(&xo_count, &xo_time, &vddmin_count, &vddmin_time)) {
		if (xo_count > previous_xo_count) {
			printk("[K] XO: %u, %llums\n", xo_count-previous_xo_count, xo_time-previous_xo_time);
			previous_xo_count = xo_count;
			previous_xo_time = xo_time;
		}
		if (vddmin_count > previous_vddmin_count) {
			printk("[K] Vdd-min: %u, %llums\n", vddmin_count-previous_vddmin_count, vddmin_time-previous_vddmin_time);
			previous_vddmin_count = vddmin_count;
			previous_vddmin_time = vddmin_time;
		}
	}
}

void htc_print_vddmin_gpio_status(void)
{
	if (system_rev == XB || system_rev == XC)
		printk(KERN_INFO "[K] AP2MDM_VDDMIN: %d, MDM2AP_VDDMIN: %d. \n", gpio_get_value(AP2MDM_VDDMIN), gpio_get_value(MDM2AP_VDDMIN));
}

void htc_pm_monitor_work(struct work_struct *work)
{
	struct timespec ts;
	struct rtc_time tm;

	if (htc_pm_monitor_wq == NULL){
		printk(KERN_INFO "[K] hTc PM Statistic is NILL.\n");
		return;
	}

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec - (sys_tz.tz_minuteswest * 60), &tm);
	printk("[K] [PM] hTC PM Statistic start (%02d-%02d %02d:%02d:%02d) \n",
		tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	htc_show_interrupts();
	htc_xo_block_clks_count_show();
	htc_xo_block_clks_count_clear();
	msm_xo_print_voters();
	htc_idle_stat_show(msm_htc_util_delay_time);
	htc_idle_stat_clear();
	htc_timer_stats_onoff('0');
	htc_timer_stats_show(300);
	htc_timer_stats_onoff('1');
#ifdef CONFIG_PERFLOCK
	htc_print_active_perf_locks();
#endif

	
	htc_debug_flag_show();

	
	
	htc_print_active_wake_locks(WAKE_LOCK_SUSPEND);
	htc_print_vddmin_gpio_status();

	queue_delayed_work(htc_pm_monitor_wq, &htc_pm_delayed_work, msecs_to_jiffies(msm_htc_util_delay_time));
	htc_kernel_top();
	printk("[K] [PM] hTC PM Statistic done\n");
}

void htc_kernel_top_accumulation_monitor_work(struct work_struct *work)
{
	struct timespec ts;
	struct rtc_time tm;

	if (htc_kernel_top_monitor_wq == NULL){
		if (pm_monitor_enabled)
			printk(KERN_INFO "[K] hTc Kernel Top statistic is NILL.\n");
		return;
	}

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec - (sys_tz.tz_minuteswest * 60), &tm);
	if (pm_monitor_enabled)
		printk("[K] [KTOP] hTC Kernel Top Statistic start (%02d-%02d %02d:%02d:%02d) \n",
			tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	if (!pm_monitor_enabled) {
		htc_show_interrupts();
		htc_print_active_wake_locks(WAKE_LOCK_SUSPEND);
	}

	queue_delayed_work(htc_kernel_top_monitor_wq, &htc_kernel_top_delayed_work, msecs_to_jiffies(msm_htc_util_top_delay_time));
	htc_kernel_top_accumulation();
	if (pm_monitor_enabled)
		printk("[K] [KTOP] hTC Kernel Top Statistic done\n");
} 

static u32 full_loading_counter = 0;

#define MAX_PID 32768
 
unsigned int *prev_proc_stat = NULL;
int *curr_proc_delta = NULL;
int *curr_proc_pid = NULL;  
struct task_struct **task_ptr_array = NULL;
struct kernel_cpustat new_cpu_stat, old_cpu_stat;
unsigned int *prev_proc_stat_accu = NULL;
int *curr_proc_delta_accu = NULL;
int *curr_proc_pid_accu = NULL;
struct task_struct **task_ptr_array_accu = NULL;
struct kernel_cpustat new_cpu_stat_accu, old_cpu_stat_accu;

int findBiggestInRange(int *array, int max_limit_idx)
{
	int largest_idx = 0, i;

	for (i = 0 ; i < MAX_PID ; i++) {
		if (array[i] > array[largest_idx] && (max_limit_idx == -1 || array[i] < array[max_limit_idx]))
			largest_idx = i;
	}

	return largest_idx;
}

void sorting(int *source, int *output)
{
	int i;
	for (i = 0 ; i < NUM_BUSY_THREAD_CHECK ; i++) {
		if (i == 0)
			output[i] = findBiggestInRange(source, -1);
		else
			output[i] = findBiggestInRange(source, output[i-1]);
	}
}

void sorting_pid(int *source, int *pid_pos, int pid_cnt, int *output)
{
    int i=0, j=0, k=0, l=0;
    int pid_found = 0;

    for (i = 0; i < NUM_BUSY_THREAD_CHECK; i++) {
        output[i] = 0;
        if (i == 0) {
            for (j = 0; j < pid_cnt; j++) {
                k = pid_pos[j];
                
                if(source[output[i]] < source[k]) {
                    output[i] = k;
                }
            }
        }
        else {
            for (j = 0; j < pid_cnt; j++) {
                k = pid_pos[j];
                
                for (l = 0; l < i; l++) {
                    
                    if (output[l] == k) {
                        pid_found = 1;
                        break;
                    }
                }
                
                if (pid_found) {
                    pid_found = 0;
                    continue;
                }

                
                if(source[output[i]] < source[k]) {
                    output[i] = k;
                }
            }
        }
    }
} 

static void get_all_cpu_stat(struct kernel_cpustat *cpu_stat)
{
	int i;

	if (!cpu_stat)
		return;
	memset(cpu_stat, 0, sizeof(struct kernel_cpustat));

	for_each_possible_cpu(i) {
		cpu_stat->cpustat[CPUTIME_USER] += kcpustat_cpu(i).cpustat[CPUTIME_USER];
		cpu_stat->cpustat[CPUTIME_NICE] += kcpustat_cpu(i).cpustat[CPUTIME_NICE];
		cpu_stat->cpustat[CPUTIME_SYSTEM] += kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM];
		cpu_stat->cpustat[CPUTIME_IDLE] += get_idle_time(i);
		cpu_stat->cpustat[CPUTIME_IOWAIT] += get_iowait_time(i);
		cpu_stat->cpustat[CPUTIME_IRQ] += kcpustat_cpu(i).cpustat[CPUTIME_IRQ];
		cpu_stat->cpustat[CPUTIME_SOFTIRQ] += kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ];
		cpu_stat->cpustat[CPUTIME_STEAL] += kcpustat_cpu(i).cpustat[CPUTIME_STEAL];
		cpu_stat->cpustat[CPUTIME_GUEST] += kcpustat_cpu(i).cpustat[CPUTIME_GUEST];
		cpu_stat->cpustat[CPUTIME_GUEST_NICE] += kcpustat_cpu(i).cpustat[CPUTIME_GUEST_NICE];
	}
}

void clear_process_monitor_array(struct process_monitor_statistic *pArray, int array_size)
{
    int j;

    for (j = 0; j < array_size; j++)
    {
        (pArray + j)->pid = 0;
        (pArray + j)->ppid_name = NULL;
        (pArray + j)->cnt = 0;
        (pArray + j)->set_warn = 0;
        (pArray + j)->is_found = 0;
#if SEND_KOBJECT_UEVENT_ENV_ENABLED
        (pArray + j)->sent_uevent = 0;
#endif 
    }
} 

#if USE_STATISTICS_STRATEGY_CONTINUOUS_3
void clear_current_pid_found_array(void)
{
    int i;

    for (i = 0; i < SIZE_OF_CURR_PID_FOUND_ARRAY; i++)
    {
        current_pid_found_array[i].pid_found = 0;
        current_pid_found_array[i].need_to_add = 0;
    }
} 

int htc_kernel_top_statistics_continuous_3(unsigned long delta_time, int *ptr_top_loading)
{
    int rtn = 0;
    int i, j, cpu_usage;
#if SEND_KOBJECT_UEVENT_ENV_ENABLED
    int ok_to_send_uevent = 0;
    char buf_warn[SIZE_OF_PROCESS_MONITOR_CONTINUOUS_3_ARRAY * BUFFER_WARN_LEN];
    char buf_temp[BUFFER_TEMP_LEN];
#endif 

    for (i = 0 ; i < SIZE_OF_CURR_PID_FOUND_ARRAY ; i++)
    {
        cpu_usage = curr_proc_delta_accu[*(ptr_top_loading + i)] * 100 / delta_time;
        
        if (cpu_usage >= HTC_KERNEL_TOP_CPU_USAGE_THRESHOLD)
        {
            
            for (j = 0; j < SIZE_OF_PROCESS_MONITOR_CONTINUOUS_3_ARRAY; j++)
            {
                
                if (process_monitor_continuous_3_array[j].pid == *(ptr_top_loading + i))
                {
                    
                    process_monitor_continuous_3_array[j].cnt++;
                    process_monitor_continuous_3_array[j].is_found = 1;
                    
                    current_pid_found_array[i].pid_found = 1;
                    if ((process_monitor_continuous_3_array[j].cnt >= MAX_CONSECUTIVE_THRES_TIMES) && (!process_monitor_continuous_3_array[j].set_warn))
                    {
                        process_monitor_continuous_3_array[j].set_warn = 1;
                        printk(KERN_ERR "[K] CPU_Sniffer: PID=[%d], name=[%s], over-cpu-usage-threshold.\n", 
                            process_monitor_continuous_3_array[j].pid, process_monitor_continuous_3_array[j].ppid_name);
                    }
                    break;
                }
            }
            if (!current_pid_found_array[i].pid_found)
            {
                current_pid_found_array[i].need_to_add = 1;
            }
        }
    }

#if SEND_KOBJECT_UEVENT_ENV_ENABLED
    
    memset(buf_warn, 0x0, sizeof(buf_warn));
    strcpy(buf_warn, "");
    for (j = 0; j < SIZE_OF_PROCESS_MONITOR_CONTINUOUS_3_ARRAY; j++)
    {
        if ((process_monitor_continuous_3_array[j].set_warn == 1) && (process_monitor_continuous_3_array[j].sent_uevent == 0))
        {
            strcat(buf_warn, "PID=");
            sprintf(buf_temp, "%d", process_monitor_continuous_3_array[j].pid);
            strcat(buf_warn, buf_temp);
            strcat(buf_warn, ",0,0,0;");
            process_monitor_continuous_3_array[j].sent_uevent = 1;
            ok_to_send_uevent++;
        }
    }

    
    if (ok_to_send_uevent)
    {
        
        strcat(buf_warn, "PID=0,0,0,0;");
        strcat(buf_warn, "#");
        send_cpu_usage_stats_kobject_uevent(&buf_warn[0]);
    }
#endif 

    
    for (j = 0; j < SIZE_OF_PROCESS_MONITOR_CONTINUOUS_3_ARRAY; j++)
    {
        if (!process_monitor_continuous_3_array[j].is_found)
        {
            
            process_monitor_continuous_3_array[j].pid = 0;
            process_monitor_continuous_3_array[j].ppid_name = NULL;
            process_monitor_continuous_3_array[j].cnt = 0;
            process_monitor_continuous_3_array[j].set_warn = 0;
#if SEND_KOBJECT_UEVENT_ENV_ENABLED
            process_monitor_continuous_3_array[j].sent_uevent = 0;
#endif 
        }
        
        process_monitor_continuous_3_array[j].is_found = 0;
    }

    
    for (i = 0 ; i < SIZE_OF_CURR_PID_FOUND_ARRAY ; i++)
    {
        
        for (j = 0; j < SIZE_OF_PROCESS_MONITOR_CONTINUOUS_3_ARRAY; j++)
        {
            if (current_pid_found_array[i].need_to_add && !process_monitor_continuous_3_array[j].pid)
            {
                process_monitor_continuous_3_array[j].pid = *(ptr_top_loading + i);
                process_monitor_continuous_3_array[j].ppid_name = task_ptr_array_accu[*(ptr_top_loading + i)]->comm;
                process_monitor_continuous_3_array[j].cnt++;
                current_pid_found_array[i].need_to_add = 0;
                break;
            }
        }
    }
    clear_current_pid_found_array();

    return rtn;
} 
#else 

int htc_kernel_top_statistics_5_in_10(unsigned long delta_time, int *ptr_top_loading)
{
    int rtn = 0;
    int i, j, cpu_usage;
#if SEND_KOBJECT_UEVENT_ENV_ENABLED
    int ok_to_send_uevent = 0;
    char buf_warn[BUFFER_WARN_5_IN_10_SIZE * BUFFER_WARN_LEN];
    char buf_temp[BUFFER_TEMP_LEN];
#endif 

    for (i = 0 ; i < NUM_BUSY_THREAD_CHECK ; i++)
    {
        cpu_usage = curr_proc_delta_accu[*(ptr_top_loading + i)] * 100 / delta_time;
        
        if (cpu_usage >= HTC_KERNEL_TOP_CPU_USAGE_THRESHOLD)
        {
            
            for (j = 0; j < SIZE_OF_PROCESS_MONITOR_5_IN_10_ARRAY; j++)
            {
                
                if (process_monitor_5_in_10_array[j].pid == *(ptr_top_loading + i))
                {
                    
                    process_monitor_5_in_10_array[j].cnt++;
                    if ((process_monitor_5_in_10_array[j].cnt >= MAX_OVER_THRES_TIMES) && (process_monitor_5_in_10_array[j].set_warn == 0))
                    {
                        process_monitor_5_in_10_array[j].set_warn = 1;
                        process_monitor_5_in_10_array[j].ppid_name = task_ptr_array_accu[*(ptr_top_loading + i)]->comm;
                    }
                    break;
                }
                
                else if (process_monitor_5_in_10_array[j].pid == 0)
                {
                    process_monitor_5_in_10_array[j].pid = *(ptr_top_loading + i);
                    process_monitor_5_in_10_array[j].cnt++;
                    break;
                }
            }
        }
    }

    if (statistic_monitor_period < HTC_KERNEL_TOP_MONITOR_PERIOD)
    {
        
        statistic_monitor_period++;
    }
    else
    {
        
#if SEND_KOBJECT_UEVENT_ENV_ENABLED
        
        memset(buf_warn, 0x0, sizeof(buf_warn));
        strcpy(buf_warn, "");
#endif 
        for (j = 0; j < SIZE_OF_PROCESS_MONITOR_5_IN_10_ARRAY; j++)
        {
            if (process_monitor_5_in_10_array[j].set_warn == 1)
            {
#if SEND_KOBJECT_UEVENT_ENV_ENABLED
                strcat(buf_warn, "PID=");
                sprintf(buf_temp, "%d", process_monitor_5_in_10_array[j].pid);
                strcat(buf_warn, buf_temp);
                strcat(buf_warn, ",0,0,0;");
#endif 
                printk(KERN_ERR "[K] CPU_Sniffer: PID=[%d], name=[%s], over-cpu-usage-threshold.\n", 
                    process_monitor_5_in_10_array[j].pid, process_monitor_5_in_10_array[j].ppid_name);
#if SEND_KOBJECT_UEVENT_ENV_ENABLED
                process_monitor_5_in_10_array[j].sent_uevent = 1;
                ok_to_send_uevent++;
#endif 
                rtn = 1;
            }
        }

#if SEND_KOBJECT_UEVENT_ENV_ENABLED
        
        if (ok_to_send_uevent)
        {
            
            strcat(buf_warn, "PID=0,0,0,0;");
            strcat(buf_warn, "#");
            send_cpu_usage_stats_kobject_uevent(&buf_warn[0]);
        }
#endif 

        if (pm_monitor_enabled)
            printk("[K] [KTOP] Reach the number of statistics monitor period.\n");
        statistic_monitor_period = 1;
        clear_process_monitor_array(&process_monitor_5_in_10_array[0], SIZE_OF_PROCESS_MONITOR_5_IN_10_ARRAY);
    }

    return rtn;
} 
#endif 

void htc_kernel_top(void)
{
	struct task_struct *p;
	int top_loading[NUM_BUSY_THREAD_CHECK], i;
	unsigned long user_time, system_time, io_time;
	unsigned long irq_time, idle_time, delta_time;
	ulong flags;
	struct task_cputime cputime;
	int dump_top_stack = 0;
	int pid_cnt = 0;    
#ifdef CONFIG_DEBUG_KSWAPD
	struct task_struct *kswapd_t = NULL;
#endif

	if (task_ptr_array == NULL ||
			curr_proc_delta == NULL ||
			curr_proc_pid == NULL ||    
			prev_proc_stat == NULL)
		return;

	spin_lock_irqsave(&lock, flags);
	get_all_cpu_stat(&new_cpu_stat);

	
	for_each_process(p) {
		thread_group_cputime(p, &cputime);

		if (p->pid < MAX_PID) {
			curr_proc_delta[p->pid] =
				(cputime.utime + cputime.stime)
				- (prev_proc_stat[p->pid]);
			task_ptr_array[p->pid] = p;
			
			if (curr_proc_delta[p->pid] > 0) {
				curr_proc_pid[pid_cnt] = p->pid;
				pid_cnt++;
			}
			
		}
	}

	
	
	
	sorting_pid(curr_proc_delta, curr_proc_pid, pid_cnt, top_loading);
	

	
	user_time = (unsigned long)((new_cpu_stat.cpustat[CPUTIME_USER] + new_cpu_stat.cpustat[CPUTIME_NICE])
			- (old_cpu_stat.cpustat[CPUTIME_USER] + old_cpu_stat.cpustat[CPUTIME_NICE]));
	system_time = (unsigned long)(new_cpu_stat.cpustat[CPUTIME_SYSTEM] - old_cpu_stat.cpustat[CPUTIME_SYSTEM]);
	io_time = (unsigned long)(new_cpu_stat.cpustat[CPUTIME_IOWAIT] - old_cpu_stat.cpustat[CPUTIME_IOWAIT]);
	irq_time = (unsigned long)((new_cpu_stat.cpustat[CPUTIME_IRQ] + new_cpu_stat.cpustat[CPUTIME_SOFTIRQ])
			- (old_cpu_stat.cpustat[CPUTIME_IRQ] + old_cpu_stat.cpustat[CPUTIME_SOFTIRQ]));


    

    idle_time = (unsigned long)((new_cpu_stat.cpustat[CPUTIME_IDLE] > old_cpu_stat.cpustat[CPUTIME_IDLE]) 
            ? new_cpu_stat.cpustat[CPUTIME_IDLE] - old_cpu_stat.cpustat[CPUTIME_IDLE] : 0);
    idle_time += (unsigned long)((new_cpu_stat.cpustat[CPUTIME_STEAL] + new_cpu_stat.cpustat[CPUTIME_GUEST]) 
                - (old_cpu_stat.cpustat[CPUTIME_STEAL] + old_cpu_stat.cpustat[CPUTIME_GUEST]));
    

	delta_time = user_time + system_time + io_time + irq_time + idle_time;

	if ((full_loading_counter >= 9) && (full_loading_counter % 3 == 0))
		 dump_top_stack = 1;

	
	printk(KERN_INFO "[K] CPU Usage\t\tPID\t\tName\n");
	for (i = 0 ; i < NUM_BUSY_THREAD_CHECK ; i++) {
		printk(KERN_INFO "[K] %8lu%%\t\t%d\t\t%s\t\t%d\n",
				curr_proc_delta[top_loading[i]] * 100 / delta_time,
				top_loading[i],
				task_ptr_array[top_loading[i]]->comm,
				curr_proc_delta[top_loading[i]]);
#ifdef CONFIG_DEBUG_KSWAPD
		if (task_ptr_array[top_loading[i]] && task_ptr_array[top_loading[i]]->flags & PF_KSWAPD)
			kswapd_t = task_ptr_array[top_loading[i]];
#endif
	}

#ifdef CONFIG_DEBUG_KSWAPD
	if (kswapd_t) {
		printk("\n[K][DEBUG] ###pid:%d name:%s state:%lu ppid:%d stime:%lu utime:%lu\n",
				kswapd_t->pid, kswapd_t->comm, kswapd_t->state, kswapd_t->real_parent->pid, kswapd_t->stime, kswapd_t->utime);
		show_stack(kswapd_t, NULL);
	}
#endif

	
	if (dump_top_stack) {
	   struct task_struct *t;
	   for (i = 0 ; i < NUM_BUSY_THREAD_CHECK ; i++) {
		if (task_ptr_array[top_loading[i]] != NULL && task_ptr_array[top_loading[i]]->stime > 0) {
			t = task_ptr_array[top_loading[i]];
			
			do {
				printk("\n[K] ###pid:%d name:%s state:%lu ppid:%d stime:%lu utime:%lu\n",
				t->pid, t->comm, t->state, t->real_parent->pid, t->stime, t->utime);
				show_stack(t, t->stack);
				t = next_thread(t);
			} while (t != task_ptr_array[top_loading[i]]);
		}
	   }
	}
	
	for_each_process(p) {
		if (p->pid < MAX_PID) {
			thread_group_cputime(p, &cputime);
			prev_proc_stat[p->pid] = cputime.stime + cputime.utime;
		}
	}

	old_cpu_stat = new_cpu_stat;

	spin_unlock_irqrestore(&lock, flags);

	memset(curr_proc_delta, 0, sizeof(int) * MAX_PID);
	memset(task_ptr_array, 0, sizeof(int) * MAX_PID);
	memset(curr_proc_pid, 0, sizeof(int) * MAX_PID);    
}

void htc_kernel_top_accumulation(void)
{
	struct task_struct *p;
	int top_loading_accu[NUM_BUSY_THREAD_CHECK], i;
	unsigned long user_time, system_time, io_time;
	unsigned long irq_time, idle_time, delta_time;
	ulong flags;
	struct task_cputime cputime;
	int dump_top_stack = 0;
	int pid_cnt = 0;

	if (task_ptr_array_accu == NULL ||
			curr_proc_delta_accu == NULL ||
			curr_proc_pid_accu == NULL ||
			prev_proc_stat_accu == NULL)
		return;

	spin_lock_irqsave(&lock_accu, flags);
	get_all_cpu_stat(&new_cpu_stat_accu);

	
	for_each_process(p) {
		thread_group_cputime(p, &cputime);

		if (p->pid < MAX_PID) {
			curr_proc_delta_accu[p->pid] =
				(cputime.utime + cputime.stime)
				- (prev_proc_stat_accu[p->pid]);
			task_ptr_array_accu[p->pid] = p;
			if (curr_proc_delta_accu[p->pid] > 0) {
				curr_proc_pid_accu[pid_cnt] = p->pid;
				pid_cnt++;
			}
		}
	}

	
	
	sorting_pid(curr_proc_delta_accu, curr_proc_pid_accu, pid_cnt, top_loading_accu);

	
	user_time = (unsigned long)((new_cpu_stat_accu.cpustat[CPUTIME_USER] + new_cpu_stat_accu.cpustat[CPUTIME_NICE])
			- (old_cpu_stat_accu.cpustat[CPUTIME_USER] + old_cpu_stat_accu.cpustat[CPUTIME_NICE]));
	system_time = (unsigned long)(new_cpu_stat_accu.cpustat[CPUTIME_SYSTEM] - old_cpu_stat_accu.cpustat[CPUTIME_SYSTEM]);
	io_time = (unsigned long)(new_cpu_stat_accu.cpustat[CPUTIME_IOWAIT] - old_cpu_stat_accu.cpustat[CPUTIME_IOWAIT]);
	irq_time = (unsigned long)((new_cpu_stat_accu.cpustat[CPUTIME_IRQ] + new_cpu_stat_accu.cpustat[CPUTIME_SOFTIRQ])
			- (old_cpu_stat_accu.cpustat[CPUTIME_IRQ] + old_cpu_stat_accu.cpustat[CPUTIME_SOFTIRQ]));

    

    idle_time = (unsigned long)((new_cpu_stat_accu.cpustat[CPUTIME_IDLE] > old_cpu_stat_accu.cpustat[CPUTIME_IDLE]) 
            ? new_cpu_stat_accu.cpustat[CPUTIME_IDLE] - old_cpu_stat_accu.cpustat[CPUTIME_IDLE] : 0);
    idle_time += (unsigned long)((new_cpu_stat_accu.cpustat[CPUTIME_STEAL] + new_cpu_stat_accu.cpustat[CPUTIME_GUEST]) 
                - (old_cpu_stat_accu.cpustat[CPUTIME_STEAL] + old_cpu_stat_accu.cpustat[CPUTIME_GUEST]));
    

	delta_time = user_time + system_time + io_time + irq_time + idle_time;

	if ((full_loading_counter >= 9) && (full_loading_counter % 3 == 0))
		 dump_top_stack = 1;

	
	if (pm_monitor_enabled) {
		printk("[K] [KTOP] CPU Usage\t\tPID\t\tName\n");
		for (i = 0 ; i < NUM_BUSY_THREAD_CHECK ; i++) {
			printk("[K] [KTOP] %8lu%%\t\t%d\t\t%s\t\t%d\n",
					curr_proc_delta_accu[top_loading_accu[i]] * 100 / delta_time,
					top_loading_accu[i],
					task_ptr_array_accu[top_loading_accu[i]]->comm,
					curr_proc_delta_accu[top_loading_accu[i]]);
		}
	}

#if USE_STATISTICS_STRATEGY_CONTINUOUS_3
	htc_kernel_top_statistics_continuous_3(delta_time, &top_loading_accu[0]);
#else 
	htc_kernel_top_statistics_5_in_10(delta_time, &top_loading_accu[0]);
#endif 

	
	if (dump_top_stack) {
	   struct task_struct *t;
	   for (i = 0 ; i < NUM_BUSY_THREAD_CHECK ; i++) {
		if (task_ptr_array_accu[top_loading_accu[i]] != NULL && task_ptr_array_accu[top_loading_accu[i]]->stime > 0) {
			t = task_ptr_array_accu[top_loading_accu[i]];
			
			do {
				if (pm_monitor_enabled)
					printk("\n[K] [KTOP] ###pid:%d name:%s state:%lu ppid:%d stime:%lu utime:%lu\n",
						t->pid, t->comm, t->state, t->real_parent->pid, t->stime, t->utime);
				show_stack(t, t->stack);
				t = next_thread(t);
			} while (t != task_ptr_array_accu[top_loading_accu[i]]);
		}
	   }
	}
	
	for_each_process(p) {
		if (p->pid < MAX_PID) {
			thread_group_cputime(p, &cputime);
			prev_proc_stat_accu[p->pid] = cputime.stime + cputime.utime;
		}
	}

	old_cpu_stat_accu = new_cpu_stat_accu;

	spin_unlock_irqrestore(&lock_accu, flags);

	memset(curr_proc_delta_accu, 0, sizeof(int) * MAX_PID);
	memset(task_ptr_array_accu, 0, sizeof(int) * MAX_PID);
	memset(curr_proc_pid_accu, 0, sizeof(int) * MAX_PID);
} 

void htc_pm_monitor_init(void)
{
	if (htc_pm_monitor_wq == NULL)
		return;

	queue_delayed_work(htc_pm_monitor_wq, &htc_pm_delayed_work, msecs_to_jiffies(msm_htc_util_delay_time));

	spin_lock_init(&lock);

	prev_proc_stat = vmalloc(sizeof(int) * MAX_PID);
	curr_proc_delta = vmalloc(sizeof(int) * MAX_PID);
	task_ptr_array = vmalloc(sizeof(int) * MAX_PID);
	curr_proc_pid = vmalloc(sizeof(int) * MAX_PID);     

	memset(prev_proc_stat, 0, sizeof(int) * MAX_PID);
	memset(curr_proc_delta, 0, sizeof(int) * MAX_PID);
	memset(task_ptr_array, 0, sizeof(int) * MAX_PID);
	memset(curr_proc_pid, 0, sizeof(int) * MAX_PID);    

	get_all_cpu_stat(&new_cpu_stat);
	get_all_cpu_stat(&old_cpu_stat);

}

void htc_kernel_top_monitor_init(void)
{
	if (htc_kernel_top_monitor_wq == NULL)
		return;

	queue_delayed_work(htc_kernel_top_monitor_wq, &htc_kernel_top_delayed_work, msecs_to_jiffies(msm_htc_util_top_delay_time));

	spin_lock_init(&lock_accu);

	prev_proc_stat_accu = vmalloc(sizeof(int) * MAX_PID);
	curr_proc_delta_accu = vmalloc(sizeof(int) * MAX_PID);
	task_ptr_array_accu = vmalloc(sizeof(int) * MAX_PID);
	curr_proc_pid_accu = vmalloc(sizeof(int) * MAX_PID);

	memset(prev_proc_stat_accu, 0, sizeof(int) * MAX_PID);
	memset(curr_proc_delta_accu, 0, sizeof(int) * MAX_PID);
	memset(task_ptr_array_accu, 0, sizeof(int) * MAX_PID);
	memset(curr_proc_pid_accu, 0, sizeof(int) * MAX_PID);

	get_all_cpu_stat(&new_cpu_stat_accu);
	get_all_cpu_stat(&old_cpu_stat_accu);

} 


void htc_monitor_init(void)
{
	if (htc_pm_monitor_wq == NULL) {
		
		htc_pm_monitor_wq = create_workqueue("htc_pm_monitor_wq");
		printk(KERN_INFO "[K] Create HTC private workqueue(0x%x)...\n", (unsigned int)htc_pm_monitor_wq);
	}

	if (htc_pm_monitor_wq){
		printk(KERN_INFO "[K] htc_pm_monitor_wq is not NULL.\n");
		INIT_DELAYED_WORK(&htc_pm_delayed_work, htc_pm_monitor_work);
	}
}

void htc_top_monitor_init(void)
{
	if (htc_kernel_top_monitor_wq == NULL) {
		
		htc_kernel_top_monitor_wq = create_workqueue("htc_kernel_top_monitor_wq");
		printk(KERN_INFO "[K] [KTOP] Create HTC private workqueue(0x%x)...\n", (unsigned int)htc_kernel_top_monitor_wq);
	}

	if (htc_kernel_top_monitor_wq) {
		printk(KERN_INFO "[K] [KTOP] htc_kernel_top_monitor_wq is not NULL.\n");
#if USE_STATISTICS_STRATEGY_CONTINUOUS_3
		clear_current_pid_found_array();
		clear_process_monitor_array(&process_monitor_continuous_3_array[0], SIZE_OF_PROCESS_MONITOR_CONTINUOUS_3_ARRAY);
#else 
		clear_process_monitor_array(&process_monitor_5_in_10_array[0], SIZE_OF_PROCESS_MONITOR_5_IN_10_ARRAY);
#endif 
		INIT_DELAYED_WORK(&htc_kernel_top_delayed_work, htc_kernel_top_accumulation_monitor_work);
	}
} 

