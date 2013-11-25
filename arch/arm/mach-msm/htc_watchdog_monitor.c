/* arch/arm/mach-msm/htc_watchdog_monitor.c
 *
 * Copyright (C) 2011 HTC Corporation
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
#include <linux/kernel_stat.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include "htc_watchdog_monitor.h"

#define MAX_PID 32768
#define NUM_BUSY_THREAD_CHECK 5

#ifndef arch_idle_time
#define arch_idle_time(cpu) 0
#endif

static unsigned int *prev_proc_stat;
static int *curr_proc_delta;
static struct task_struct **task_ptr_array;
static struct htc_cpu_usage_stat  old_cpu_stat;
static spinlock_t lock;

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
static void get_all_cpu_stat(struct htc_cpu_usage_stat *cpu_stat)
{
	int i;
	u64 user, nice, system, idle, iowait, irq, softirq, steal;
	u64 guest, guest_nice;

	if (!cpu_stat)
		return;

	user = nice = system = idle = iowait =
		irq = softirq = steal = 0;
	guest = guest_nice = 0;

	for_each_possible_cpu(i) {
		user += kcpustat_cpu(i).cpustat[CPUTIME_USER];
		nice += kcpustat_cpu(i).cpustat[CPUTIME_NICE];
		system += kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM];
		idle += get_idle_time(i);
		iowait += get_iowait_time(i);
		irq += kcpustat_cpu(i).cpustat[CPUTIME_IRQ];
		softirq += kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ];
		steal += kcpustat_cpu(i).cpustat[CPUTIME_STEAL];
		guest += kcpustat_cpu(i).cpustat[CPUTIME_GUEST];
		guest_nice += kcpustat_cpu(i).cpustat[CPUTIME_GUEST_NICE];
	
	
	}
	cpu_stat->user = user;
	cpu_stat->nice = nice;
	cpu_stat->system = system;
	cpu_stat->softirq = softirq;
	cpu_stat->irq = irq;
	cpu_stat->idle = idle;
	cpu_stat->iowait = iowait;
	cpu_stat->steal = steal;
	cpu_stat->guest = guest;
	cpu_stat->guest_nice = guest_nice;
}

void htc_watchdog_monitor_init(void)
{
	spin_lock_init(&lock);
	prev_proc_stat = vmalloc(sizeof(int) * MAX_PID);
	curr_proc_delta = vmalloc(sizeof(int) * MAX_PID);
	task_ptr_array = vmalloc(sizeof(int) * MAX_PID);
	if (prev_proc_stat)
		memset(prev_proc_stat, 0, sizeof(int) * MAX_PID);
	if (curr_proc_delta)
		memset(curr_proc_delta, 0, sizeof(int) * MAX_PID);
	if (task_ptr_array)
		memset(task_ptr_array, 0, sizeof(int) * MAX_PID);

	get_all_cpu_stat(&old_cpu_stat);
}

static int findBiggestInRange(int *array, int max_limit_idx)
{
	int largest_idx = 0, i;

	for (i = 0; i < MAX_PID; i++) {
		if (array[i] > array[largest_idx] &&
		(max_limit_idx == -1 || array[i] < array[max_limit_idx]))
			largest_idx = i;
	}

	return largest_idx;
}

static void sorting(int *source, int *output)
{
	int i;
	for (i = 0; i < NUM_BUSY_THREAD_CHECK; i++) {
		if (i == 0)
			output[i] = findBiggestInRange(source, -1);
		else
			output[i] = findBiggestInRange(source, output[i-1]);
	}
}

	
void htc_watchdog_top_stat(void)
{
	struct task_struct *p;
	int top_loading[NUM_BUSY_THREAD_CHECK], i;
	unsigned long user_time, system_time, io_time;
	unsigned long irq_time, idle_time, delta_time;
	ulong flags;
	struct task_cputime cputime;
	struct htc_cpu_usage_stat new_cpu_stat;

	if (task_ptr_array == NULL ||
			curr_proc_delta == NULL ||
			prev_proc_stat == NULL)
		return;

	memset(curr_proc_delta, 0, sizeof(int) * MAX_PID);
	memset(task_ptr_array, 0, sizeof(int) * MAX_PID);

	printk(KERN_ERR"\n\n[%s] Start to dump:\n", __func__);

	spin_lock_irqsave(&lock, flags);
	get_all_cpu_stat(&new_cpu_stat);

	
	for_each_process(p) {
		thread_group_cputime(p, &cputime);

		if (p->pid < MAX_PID) {
			curr_proc_delta[p->pid] =
				(cputime.utime + cputime.stime)
				- (prev_proc_stat[p->pid]);
			task_ptr_array[p->pid] = p;
		}
	}

	
	sorting(curr_proc_delta, top_loading);

	
	user_time = (unsigned long)((new_cpu_stat.user + new_cpu_stat.nice)
			- (old_cpu_stat.user + old_cpu_stat.nice));
	system_time = (unsigned long)(new_cpu_stat.system - old_cpu_stat.system);
	io_time = (unsigned long)(new_cpu_stat.iowait - old_cpu_stat.iowait);
	irq_time = (unsigned long)((new_cpu_stat.irq + new_cpu_stat.softirq)
			- (old_cpu_stat.irq + old_cpu_stat.softirq));
	idle_time = (unsigned long)
	((new_cpu_stat.idle + new_cpu_stat.steal + new_cpu_stat.guest)
	- (old_cpu_stat.idle + old_cpu_stat.steal + old_cpu_stat.guest));
	delta_time = user_time + system_time + io_time + irq_time + idle_time;

	
	printk(KERN_ERR"CPU\t\tPID\t\tName\n");
	for (i = 0; i < NUM_BUSY_THREAD_CHECK; i++) {
		printk(KERN_ERR "%lu%%\t\t%d\t\t%s\n",
			curr_proc_delta[top_loading[i]] * 100 / delta_time,
			top_loading[i],
			task_ptr_array[top_loading[i]]->comm);
	}
	spin_unlock_irqrestore(&lock, flags);
	printk(KERN_ERR "\n");
}
