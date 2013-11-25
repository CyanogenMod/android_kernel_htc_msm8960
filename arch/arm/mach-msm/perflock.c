/* arch/arm/mach-msm/perflock.c
 *
 * Copyright (C) 2008 HTC Corporation
 * Author: Eiven Peng <eiven_peng@htc.com>
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

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/earlysuspend.h>
#include <linux/cpufreq.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <mach/perflock.h>
#include "acpuclock.h"

#define PERF_LOCK_INITIALIZED	(1U << 0)
#define PERF_LOCK_ACTIVE	(1U << 1)

enum {
	PERF_LOCK_DEBUG = 1U << 0,
	PERF_EXPIRE_DEBUG = 1U << 1,
	PERF_CPUFREQ_NOTIFY_DEBUG = 1U << 2,
	PERF_CPUFREQ_LOCK_DEBUG = 1U << 3,
	PERF_SCREEN_ON_POLICY_DEBUG = 1U << 4,
};

static LIST_HEAD(active_perf_locks);
static LIST_HEAD(inactive_perf_locks);
static LIST_HEAD(active_cpufreq_ceiling_locks);
static LIST_HEAD(inactive_cpufreq_ceiling_locks);
static DEFINE_SPINLOCK(list_lock);
static DEFINE_SPINLOCK(policy_update_lock);
static int initialized;
static int cpufreq_ceiling_initialized;
static unsigned int *perf_acpu_table;
static unsigned int *cpufreq_ceiling_acpu_table;
static unsigned int table_size;
static struct workqueue_struct *perflock_setrate_workqueue;


#ifdef CONFIG_PERF_LOCK_DEBUG
static int debug_mask = PERF_LOCK_DEBUG | PERF_EXPIRE_DEBUG |
	PERF_CPUFREQ_NOTIFY_DEBUG | PERF_CPUFREQ_LOCK_DEBUG;
#else
static int debug_mask = 0;
#endif

static struct kernel_param_ops param_ops_str = {
	.set = param_set_int,
	.get = param_get_int,
};

module_param_cb(debug_mask, &param_ops_str, &debug_mask, S_IWUSR | S_IRUGO);

#ifdef CONFIG_HTC_PNPMGR
static int legacy_mode = 0;
module_param_cb(legacy_mode, &param_ops_str, &legacy_mode, S_IWUSR | S_IRUGO);
extern struct kobject *cpufreq_kobj;
#endif

static unsigned int get_perflock_speed(void);
static unsigned int get_cpufreq_ceiling_speed(void);
static void print_active_locks(void);

#ifdef CONFIG_PERFLOCK_SCREEN_POLICY
static unsigned int screen_off_policy_req;
static unsigned int screen_on_policy_req;
static void perflock_early_suspend(struct early_suspend *handler)
{
	unsigned long irqflags;
	int cpu;

	spin_lock_irqsave(&policy_update_lock, irqflags);
	if (screen_on_policy_req) {
		screen_on_policy_req--;
		spin_unlock_irqrestore(&policy_update_lock, irqflags);
		return;
	}
	screen_off_policy_req++;
	spin_unlock_irqrestore(&policy_update_lock, irqflags);

	for_each_online_cpu(cpu) {
		cpufreq_update_policy(cpu);
	}
}

static void perflock_late_resume(struct early_suspend *handler)
{
	unsigned long irqflags;
	int cpu;

#ifdef CONFIG_MACH_HERO
	unsigned int lock_speed = get_perflock_speed() / 1000;
	if (lock_speed > CONFIG_PERFLOCK_SCREEN_ON_MIN)
		acpuclk_set_rate(lock_speed * 1000, 0);
	else
		acpuclk_set_rate(CONFIG_PERFLOCK_SCREEN_ON_MIN * 1000, 0);
#endif

	spin_lock_irqsave(&policy_update_lock, irqflags);
	if (screen_off_policy_req) {
		screen_off_policy_req--;
		spin_unlock_irqrestore(&policy_update_lock, irqflags);
		return;
	}
	screen_on_policy_req++;
	spin_unlock_irqrestore(&policy_update_lock, irqflags);

	for_each_online_cpu(cpu) {
		cpufreq_update_policy(cpu);
	}
}

static struct early_suspend perflock_power_suspend = {
	.suspend = perflock_early_suspend,
	.resume = perflock_late_resume,
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1,
};

#if defined(CONFIG_HTC_ONMODE_CHARGING) && \
	(defined(CONFIG_ARCH_MSM7225) || \
	defined(CONFIG_ARCH_MSM7227) || \
	defined(CONFIG_ARCH_MSM7201A))
static struct early_suspend perflock_onchg_suspend = {
	.suspend = perflock_early_suspend,
	.resume = perflock_late_resume,
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1,
};
#endif

static int __init perflock_screen_policy_init(void)
{
	int cpu;
	register_early_suspend(&perflock_power_suspend);
#if defined(CONFIG_HTC_ONMODE_CHARGING) && \
	(defined(CONFIG_ARCH_MSM7225) || \
	defined(CONFIG_ARCH_MSM7227) || \
	defined(CONFIG_ARCH_MSM7201A))
	register_onchg_suspend(&perflock_onchg_suspend);
#endif
	screen_on_policy_req++;
	for_each_online_cpu(cpu) {
		cpufreq_update_policy(cpu);
	}

	return 0;
}

late_initcall(perflock_screen_policy_init);
#endif

#if 0
static unsigned int policy_min = CONFIG_MSM_CPU_FREQ_ONDEMAND_MIN;
static unsigned int policy_max = CONFIG_MSM_CPU_FREQ_ONDEMAND_MAX;
#else
static unsigned int policy_min;
static unsigned int policy_max;
#endif
static int param_set_cpu_min_max(const char *val, struct kernel_param *kp)
{
	int ret;
	int cpu;
	int ret2;
	ret = param_set_int(val, kp);
	for_each_online_cpu(cpu) {
	pr_info("%s: cpufreq update cpu:  %d\n", __func__, cpu);
		ret2 = cpufreq_update_policy(cpu);
	pr_info("cpu: %d , ret: %d\n", cpu, ret2);
	}
	return ret;
}

module_param_call(min_cpu_khz, param_set_cpu_min_max, param_get_int,
	&policy_min, S_IWUSR | S_IRUGO);
module_param_call(max_cpu_khz, param_set_cpu_min_max, param_get_int,
	&policy_max, S_IWUSR | S_IRUGO);

static DEFINE_PER_CPU(int, stored_policy_min);
static DEFINE_PER_CPU(int, stored_policy_max);
int perflock_override(const struct cpufreq_policy *policy, const unsigned int new_freq)
{
	unsigned int target_min_freq = 0, target_max_freq = 0;
	unsigned int lock_speed = 0;
	unsigned int cpufreq_ceiling_speed = 0;
	unsigned long irqflags;
	unsigned int policy_min;
	unsigned int policy_max;

#ifdef CONFIG_HTC_PNPMGR
	if (!legacy_mode)
		return 0;
#endif
	if (policy != NULL) {
		
		if (strncmp("ondemand", policy->governor->name, 8) != 0)
			return 0;
		policy_min = policy->min;
		policy_max = policy->max;
		perflock_scaling_min_freq(policy_min, policy->cpu);
		perflock_scaling_max_freq(policy_max, policy->cpu);
	} else {
		policy_min = per_cpu(stored_policy_min, smp_processor_id());
		policy_max = per_cpu(stored_policy_max, smp_processor_id());
	}

	spin_lock_irqsave(&policy_update_lock, irqflags);

	if ((lock_speed = (get_perflock_speed() / 1000))) {
		target_min_freq = lock_speed > policy_min? lock_speed : policy_min;
		target_max_freq = policy_max;
		
		if (target_min_freq > target_max_freq)
			target_min_freq = target_max_freq;
		if (debug_mask & PERF_CPUFREQ_LOCK_DEBUG) {
			pr_info("%s: cpufreq lock speed %d\n",
					__func__, lock_speed);
			print_active_locks();
		}
	} else if ((cpufreq_ceiling_speed = (get_cpufreq_ceiling_speed() / 1000))) {
		target_max_freq = cpufreq_ceiling_speed > policy_max? policy_max : cpufreq_ceiling_speed;
		target_min_freq = policy_min;
		
		if (target_max_freq < target_min_freq)
			target_max_freq = target_min_freq;
		if (debug_mask & PERF_CPUFREQ_LOCK_DEBUG) {
			pr_info("%s: cpufreq_ceiling speed %d\n",
					__func__, cpufreq_ceiling_speed);
			print_active_locks();
		}
	} else {
		
		spin_unlock_irqrestore(&policy_update_lock, irqflags);
		return 0;
	}
	spin_unlock_irqrestore(&policy_update_lock, irqflags);

	if (debug_mask & PERF_CPUFREQ_LOCK_DEBUG)
		pr_info("%s: target freq = %d\n", __func__, new_freq);

	if (new_freq >= target_max_freq)
		return target_max_freq;
	else if (new_freq <= target_min_freq)
		return target_min_freq;
	else
		return new_freq;
	
	return 0;
}

void perflock_scaling_max_freq(unsigned int freq, unsigned int cpu)
{
	if (debug_mask & PERF_LOCK_DEBUG)
		pr_info("%s, freq=%u, cpu%u\n", __func__, freq, cpu);
	per_cpu(stored_policy_max, cpu) = freq;
}

void perflock_scaling_min_freq(unsigned int freq, unsigned int cpu)
{
	if (debug_mask & PERF_LOCK_DEBUG)
		pr_info("%s, freq=%u, cpu%u\n", __func__, freq, cpu);
	per_cpu(stored_policy_min, cpu) = freq;
}

static unsigned int get_perflock_speed(void)
{
	unsigned long irqflags;
	struct perf_lock *lock;
	unsigned int perf_level = 0;

	
	if (list_empty(&active_perf_locks))
		return 0;

	spin_lock_irqsave(&list_lock, irqflags);
	list_for_each_entry(lock, &active_perf_locks, link) {
		if (lock->level > perf_level)
			perf_level = lock->level;
	}
	spin_unlock_irqrestore(&list_lock, irqflags);

	return perf_acpu_table[perf_level];
}

static unsigned int get_cpufreq_ceiling_speed(void)
{
	unsigned long irqflags;
	struct perf_lock *lock;
	unsigned int perf_level = PERF_LOCK_HIGHEST;

	
	if (list_empty(&active_cpufreq_ceiling_locks))
		return 0;

	spin_lock_irqsave(&list_lock, irqflags);
	list_for_each_entry(lock, &active_cpufreq_ceiling_locks, link) {
		if (lock->level < perf_level)
			perf_level = lock->level;
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
	return cpufreq_ceiling_acpu_table[perf_level];
}

static void print_active_locks(void)
{
	unsigned long irqflags;
	struct perf_lock *lock;

	spin_lock_irqsave(&list_lock, irqflags);
	list_for_each_entry(lock, &active_perf_locks, link) {
		pr_info("active perf lock '%s'\n", lock->name);
	}
	list_for_each_entry(lock, &active_cpufreq_ceiling_locks, link) {
		pr_info("active cpufreq_ceiling_locks '%s'\n", lock->name);
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
}

void htc_print_active_perf_locks(void)
{
	unsigned long irqflags;
	struct perf_lock *lock;

	spin_lock_irqsave(&list_lock, irqflags);
	if (!list_empty(&active_perf_locks)) {
		pr_info("perf_lock:");
		list_for_each_entry(lock, &active_perf_locks, link) {
			pr_info(" '%s' ", lock->name);
		}
		pr_info("\n");
	}
	if (!list_empty(&active_cpufreq_ceiling_locks)) {
		printk(KERN_WARNING"ceiling_lock:");
		list_for_each_entry(lock, &active_cpufreq_ceiling_locks, link) {
			printk(KERN_WARNING" '%s' ", lock->name);
		}
		pr_info("\n");
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
}

void perf_lock_init(struct perf_lock *lock, unsigned int type,
			unsigned int level, const char *name)
{
	unsigned long irqflags = 0;

	WARN_ON(!lock);
	WARN_ON(!name);
	WARN_ON(level >= PERF_LOCK_INVALID);
	WARN_ON(lock->flags & PERF_LOCK_INITIALIZED);

	if ((!name) || (level >= PERF_LOCK_INVALID) ||
			(lock->flags & PERF_LOCK_INITIALIZED)) {
		pr_err("%s: ERROR \"%s\" flags %x level %d\n",
			__func__, name, lock->flags, level);
		return;
	}
	lock->name = name;
	lock->flags = PERF_LOCK_INITIALIZED;
	lock->level = level;
	lock->type = type;

	INIT_LIST_HEAD(&lock->link);
	spin_lock_irqsave(&list_lock, irqflags);
	if (lock->type == TYPE_PERF_LOCK)
		list_add(&lock->link, &inactive_perf_locks);
	if (lock->type == TYPE_CPUFREQ_CEILING)
		list_add(&lock->link, &inactive_cpufreq_ceiling_locks);
	spin_unlock_irqrestore(&list_lock, irqflags);
}
EXPORT_SYMBOL(perf_lock_init);

#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND
extern bool is_governor_ondemand(void);
extern bool is_ondemand_locked(void);
#endif
static void do_set_rate_fn(struct work_struct *work)
{
	struct cpufreq_freqs freqs;
	int ret = 0;
#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND
	if(is_governor_ondemand() && is_ondemand_locked()) {
		pr_info("[K] perflock ignore setrate, ondemand governor locked\n");
		return;
	}
#endif
	freqs.new = perflock_override(NULL, 0);
	freqs.cpu = smp_processor_id();
	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
	ret = acpuclk_set_rate(freqs.cpu, freqs.new, SETRATE_CPUFREQ);
	if (!ret)
		cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
}

static DECLARE_WORK(do_setrate_work, do_set_rate_fn);
void perf_lock(struct perf_lock *lock)
{
	unsigned long irqflags;
	int cpu;

	WARN_ON((lock->flags & PERF_LOCK_INITIALIZED) == 0);
	WARN_ON(lock->flags & PERF_LOCK_ACTIVE);
	if (lock->type == TYPE_PERF_LOCK) {
		WARN_ON(!initialized);
		if (!initialized) {
			if (debug_mask & PERF_LOCK_DEBUG)
				pr_info("%s exit because perflock is not initialized\n", __func__);
			return;
		}
	} else if (lock->type == TYPE_CPUFREQ_CEILING) {
		WARN_ON(!cpufreq_ceiling_initialized);
		if (!cpufreq_ceiling_initialized) {
			if (debug_mask & PERF_LOCK_DEBUG)
				pr_info("%s exit because cpufreq_ceiling is not initialized\n", __func__);
			return;
		}
	}

	spin_lock_irqsave(&list_lock, irqflags);
	if (debug_mask & PERF_LOCK_DEBUG)
		pr_info("%s: '%s', flags %d level %d type %u\n",
			__func__, lock->name, lock->flags, lock->level, lock->type);
	if (lock->flags & PERF_LOCK_ACTIVE) {
		pr_err("%s:type(%u) over-locked\n", __func__, lock->type);
		spin_unlock_irqrestore(&list_lock, irqflags);
		return;
	}
	lock->flags |= PERF_LOCK_ACTIVE;
	list_del(&lock->link);
	if (lock->type == TYPE_PERF_LOCK)
		list_add(&lock->link, &active_perf_locks);
	else if (lock->type == TYPE_CPUFREQ_CEILING)
		list_add(&lock->link, &active_cpufreq_ceiling_locks);
	spin_unlock_irqrestore(&list_lock, irqflags);

#ifdef CONFIG_HTC_PNPMGR
	if (!legacy_mode) {
		if (lock->type == TYPE_PERF_LOCK)
			sysfs_notify(cpufreq_kobj, NULL, "perflock_scaling_min");
		else if (lock->type == TYPE_CPUFREQ_CEILING)
			sysfs_notify(cpufreq_kobj, NULL, "perflock_scaling_max");
		return;
	}
#endif
	for_each_online_cpu(cpu) {
		queue_work_on(cpu, perflock_setrate_workqueue, &do_setrate_work);
	}
}
EXPORT_SYMBOL(perf_lock);

void perf_unlock(struct perf_lock *lock)
{
	unsigned long irqflags;

	WARN_ON(!initialized);
	WARN_ON((lock->flags & PERF_LOCK_ACTIVE) == 0);
	if (lock->type == TYPE_PERF_LOCK) {
		WARN_ON(!initialized);
		if (!initialized) {
			if (debug_mask & PERF_LOCK_DEBUG)
				pr_info("%s exit because perflock is not initialized\n", __func__);
			return;
		}
	}
	if (lock->type == TYPE_CPUFREQ_CEILING) {
		WARN_ON(!cpufreq_ceiling_initialized);
		if (!cpufreq_ceiling_initialized) {
			if (debug_mask & PERF_LOCK_DEBUG)
				pr_info("%s exit because cpufreq_ceiling is not initialized\n", __func__);
			return;
		}
	}

	spin_lock_irqsave(&list_lock, irqflags);
	if (debug_mask & PERF_LOCK_DEBUG)
		pr_info("%s: '%s', flags %d level %d\n",
			__func__, lock->name, lock->flags, lock->level);
	if (!(lock->flags & PERF_LOCK_ACTIVE)) {
		pr_err("%s: under-locked\n", __func__);
		spin_unlock_irqrestore(&list_lock, irqflags);
		return;
	}
	lock->flags &= ~PERF_LOCK_ACTIVE;
	list_del(&lock->link);
	if (lock->type == TYPE_PERF_LOCK)
		list_add(&lock->link, &inactive_perf_locks);
	else if (lock->type == TYPE_CPUFREQ_CEILING)
		list_add(&lock->link, &inactive_cpufreq_ceiling_locks);
	spin_unlock_irqrestore(&list_lock, irqflags);
#ifdef CONFIG_HTC_PNPMGR
	if (!legacy_mode) {
		if (lock->type == TYPE_PERF_LOCK)
			sysfs_notify(cpufreq_kobj, NULL, "perflock_scaling_min");
		else if (lock->type == TYPE_CPUFREQ_CEILING)
			sysfs_notify(cpufreq_kobj, NULL, "perflock_scaling_max");
	}
#endif
}
EXPORT_SYMBOL(perf_unlock);

inline int is_perf_lock_active(struct perf_lock *lock)
{
	return (lock->flags & PERF_LOCK_ACTIVE);
}
EXPORT_SYMBOL(is_perf_lock_active);

int is_perf_locked(void)
{
	return (!list_empty(&active_perf_locks));
}
EXPORT_SYMBOL(is_perf_locked);

static struct perf_lock *perflock_find(const char *name)
{
	struct perf_lock *lock;
	unsigned long irqflags;

	spin_lock_irqsave(&list_lock, irqflags);
	list_for_each_entry(lock, &active_perf_locks, link) {
		if(!strcmp(lock->name, name)) {
			spin_unlock_irqrestore(&list_lock, irqflags);
			return lock;
		}
	}
	list_for_each_entry(lock, &inactive_perf_locks, link) {
		if(!strcmp(lock->name, name)) {
			spin_unlock_irqrestore(&list_lock, irqflags);
			return lock;
		}
	}
	list_for_each_entry(lock, &active_cpufreq_ceiling_locks, link) {
		if(!strcmp(lock->name, name)) {
			spin_unlock_irqrestore(&list_lock, irqflags);
			return lock;
		}
	}
	list_for_each_entry(lock, &inactive_cpufreq_ceiling_locks, link) {
		if(!strcmp(lock->name, name)) {
			spin_unlock_irqrestore(&list_lock, irqflags);
			return lock;
		}
	}
	spin_unlock_irqrestore(&list_lock, irqflags);

	return NULL;
}

struct perf_lock *perflock_acquire(const char *name)
{
	struct perf_lock *lock = NULL;

	lock = perflock_find(name);
	if(lock)
		return lock;

	lock = kzalloc(sizeof(struct perf_lock), GFP_KERNEL);
	if(!lock) {
		pr_err("%s: fail to alloc perflock %s\n", __func__, name);
		return NULL; 
	}
	lock->name = name;
	
	lock->flags = 0; 

	return lock;
}
EXPORT_SYMBOL(perflock_acquire);

int perflock_release(const char *name)
{
	struct perf_lock *lock = NULL;
	unsigned long irqflags;

	lock = perflock_find(name);
	if(!lock)
		return -ENODEV;

	
	if(is_perf_lock_active(lock))
		perf_unlock(lock);

	spin_lock_irqsave(&list_lock, irqflags);
	list_del(&lock->link);
	spin_unlock_irqrestore(&list_lock, irqflags);
	kfree(lock);
	return 0;
}
EXPORT_SYMBOL(perflock_release);

#ifdef CONFIG_PERFLOCK_BOOT_LOCK
#define BOOT_LOCK_TIMEOUT	(60 * HZ)
static struct perf_lock boot_perf_lock;

static void do_expire_boot_lock(struct work_struct *work)
{
	if(is_perf_lock_active(&boot_perf_lock)) {
		perf_unlock(&boot_perf_lock);
		pr_info("Release 'boot-time' perf_lock\n");
	}
}
static DECLARE_DELAYED_WORK(work_expire_boot_lock, do_expire_boot_lock);

void release_boot_lock(void)
{
	if(is_perf_lock_active(&boot_perf_lock)) {
		perf_unlock(&boot_perf_lock);
		pr_info("Release 'boot-time' perf_lock before normal release time\n");
	}
}
EXPORT_SYMBOL(release_boot_lock);
#endif

static void perf_acpu_table_fixup(void)
{
	int i;
	for (i = 0; i < table_size; ++i) {
		if (perf_acpu_table[i] > policy_max * 1000)
			perf_acpu_table[i] = policy_max * 1000;
		else if (perf_acpu_table[i] < policy_min * 1000)
			perf_acpu_table[i] = policy_min * 1000;
	}

#ifdef PERFLOCK_FIX_UP
	if (table_size >= 1)
		if (perf_acpu_table[table_size - 1] < policy_max * 1000)
			perf_acpu_table[table_size - 1] = policy_max * 1000;
#endif
}

static void cpufreq_ceiling_acpu_table_fixup(void)
{
	int i;
	for (i = 0; i < table_size; ++i) {
		if (cpufreq_ceiling_acpu_table[i] > policy_max * 1000)
			cpufreq_ceiling_acpu_table[i] = policy_max * 1000;
		else if (cpufreq_ceiling_acpu_table[i] < policy_min * 1000)
			cpufreq_ceiling_acpu_table[i] = policy_min * 1000;
	}
}

static inline void init_local_freq_policy(unsigned int cpu_min
		, unsigned int cpu_max)
{
	int cpu;
	if (unlikely(per_cpu(stored_policy_max, 0) == 0)) {
		if (debug_mask & PERF_LOCK_DEBUG)
			pr_info("%s, initialize max%u\n", __func__, cpu_max);
		for_each_present_cpu(cpu) {
			per_cpu(stored_policy_max, cpu) = policy_max;
		}
	}
	if (unlikely(per_cpu(stored_policy_min, 0) == 0)) {
		if (debug_mask & PERF_LOCK_DEBUG)
			pr_info("%s, initilize min=%u\n", __func__, cpu_min);
		for_each_present_cpu(cpu) {
			per_cpu(stored_policy_min, cpu) = policy_min;
		}
	}
}


static void perflock_floor_init(struct perflock_data *pdata)
{
	struct cpufreq_policy policy;
	struct cpufreq_frequency_table *table =
		cpufreq_frequency_get_table(smp_processor_id());

	BUG_ON(cpufreq_frequency_table_cpuinfo(&policy, table));
	policy_min = policy.cpuinfo.min_freq;
	policy_max = policy.cpuinfo.max_freq;

	if (!pdata)
		goto invalid_config;

	perf_acpu_table = pdata->perf_acpu_table;
	table_size = pdata->table_size;
	if (!perf_acpu_table || !table_size)
		goto invalid_config;
	if (table_size < PERF_LOCK_INVALID)
		goto invalid_config;

	perf_acpu_table_fixup();
	perflock_setrate_workqueue = create_workqueue("perflock_setrate_wq");

	init_local_freq_policy(policy_min, policy_max);
	initialized = 1;
	pr_info("perflock floor init done\n");
#ifdef CONFIG_PERFLOCK_BOOT_LOCK
	
	perf_lock_init(&boot_perf_lock, TYPE_PERF_LOCK, PERF_LOCK_HIGHEST, "boot-time");
	perf_lock(&boot_perf_lock);
	schedule_delayed_work(&work_expire_boot_lock, BOOT_LOCK_TIMEOUT);
	pr_info("Acquire 'boot-time' perf_lock\n");
#endif

	return;

invalid_config:
	pr_err("%s: invalid configuration data, %p %d %d\n", __func__,
		perf_acpu_table, table_size, PERF_LOCK_INVALID);
}

static void cpufreq_ceiling_init(struct perflock_data *pdata)
{
	struct cpufreq_policy policy;
	struct cpufreq_frequency_table *table =
		cpufreq_frequency_get_table(smp_processor_id());

	BUG_ON(cpufreq_frequency_table_cpuinfo(&policy, table));
	policy_min = policy.cpuinfo.min_freq;
	policy_max = policy.cpuinfo.max_freq;

	if (!pdata)
		goto invalid_config;

	cpufreq_ceiling_acpu_table = pdata->perf_acpu_table;
	table_size = pdata->table_size;
	if (!cpufreq_ceiling_acpu_table || !table_size)
		goto invalid_config;
	if (table_size < PERF_LOCK_INVALID)
		goto invalid_config;

	cpufreq_ceiling_acpu_table_fixup();

	init_local_freq_policy(policy_min, policy_max);
	cpufreq_ceiling_initialized = 1;
	pr_info("perflock ceiling init done\n");
	return;

invalid_config:
	pr_err("%s: invalid configuration data, %p %d %d\n", __func__,
		cpufreq_ceiling_acpu_table, table_size, PERF_LOCK_INVALID);
}

static int perf_lock_probe(struct platform_device *pdev)
{
	struct perflock_pdata *pdata = pdev->dev.platform_data;
	pr_info("perflock probe\n");
	if(!pdata->perf_floor && !pdata->perf_ceiling) {
		printk(KERN_INFO "perf_lock Not Initialized\n");
		return -ENODEV;
	}
	if(pdata->perf_floor) {
		perflock_floor_init(pdata->perf_floor);

	}
	if(pdata->perf_ceiling) {
		cpufreq_ceiling_init(pdata->perf_ceiling);
	}
	return 0;
}

static int perf_lock_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver perf_lock_driver = {
	.probe = perf_lock_probe,
	.remove = perf_lock_remove,
	.driver = {
		.name = "perf_lock",
		.owner = THIS_MODULE,
	},
};

static int init_perf_lock(void)
{
	return platform_driver_register(&perf_lock_driver);
}

late_initcall(init_perf_lock);


#ifdef CONFIG_HTC_PNPMGR
ssize_t
perflock_scaling_max_show(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf)
{
	int ret = 0;

	ret = sprintf(buf, "%u", get_cpufreq_ceiling_speed() / 1000);
	return ret;
}
ssize_t
perflock_scaling_max_store(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t n)
{
	return 0;
}
ssize_t
perflock_scaling_min_show(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf)
{
	int ret = 0;

	ret = sprintf(buf, "%u", get_perflock_speed() / 1000);
	return ret;
}

ssize_t
perflock_scaling_min_store(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t n)
{
	return 0;
}
#endif
