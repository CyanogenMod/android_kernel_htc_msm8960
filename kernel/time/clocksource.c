/*
 * linux/kernel/time/clocksource.c
 *
 * This file contains the functions which manage clocksource drivers.
 *
 * Copyright (C) 2004, 2005 IBM, John Stultz (johnstul@us.ibm.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * TODO WishList:
 *   o Allow clocksource drivers to be unregistered
 */

#include <linux/device.h>
#include <linux/clocksource.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h> 
#include <linux/tick.h>
#include <linux/kthread.h>

void timecounter_init(struct timecounter *tc,
		      const struct cyclecounter *cc,
		      u64 start_tstamp)
{
	tc->cc = cc;
	tc->cycle_last = cc->read(cc);
	tc->nsec = start_tstamp;
}
EXPORT_SYMBOL_GPL(timecounter_init);

static u64 timecounter_read_delta(struct timecounter *tc)
{
	cycle_t cycle_now, cycle_delta;
	u64 ns_offset;

	
	cycle_now = tc->cc->read(tc->cc);

	
	cycle_delta = (cycle_now - tc->cycle_last) & tc->cc->mask;

	
	ns_offset = cyclecounter_cyc2ns(tc->cc, cycle_delta);

	
	tc->cycle_last = cycle_now;

	return ns_offset;
}

u64 timecounter_read(struct timecounter *tc)
{
	u64 nsec;

	
	nsec = timecounter_read_delta(tc);
	nsec += tc->nsec;
	tc->nsec = nsec;

	return nsec;
}
EXPORT_SYMBOL_GPL(timecounter_read);

u64 timecounter_cyc2time(struct timecounter *tc,
			 cycle_t cycle_tstamp)
{
	u64 cycle_delta = (cycle_tstamp - tc->cycle_last) & tc->cc->mask;
	u64 nsec;

	if (cycle_delta > tc->cc->mask / 2) {
		cycle_delta = (tc->cycle_last - cycle_tstamp) & tc->cc->mask;
		nsec = tc->nsec - cyclecounter_cyc2ns(tc->cc, cycle_delta);
	} else {
		nsec = cyclecounter_cyc2ns(tc->cc, cycle_delta) + tc->nsec;
	}

	return nsec;
}
EXPORT_SYMBOL_GPL(timecounter_cyc2time);

void
clocks_calc_mult_shift(u32 *mult, u32 *shift, u32 from, u32 to, u32 maxsec)
{
	u64 tmp;
	u32 sft, sftacc= 32;

	tmp = ((u64)maxsec * from) >> 32;
	while (tmp) {
		tmp >>=1;
		sftacc--;
	}

	for (sft = 32; sft > 0; sft--) {
		tmp = (u64) to << sft;
		tmp += from / 2;
		do_div(tmp, from);
		if ((tmp >> sftacc) == 0)
			break;
	}
	*mult = tmp;
	*shift = sft;
}

static struct clocksource *curr_clocksource;
static LIST_HEAD(clocksource_list);
static DEFINE_MUTEX(clocksource_mutex);
static char override_name[32];
static int finished_booting;

#ifdef CONFIG_CLOCKSOURCE_WATCHDOG
static void clocksource_watchdog_work(struct work_struct *work);

static LIST_HEAD(watchdog_list);
static struct clocksource *watchdog;
static struct timer_list watchdog_timer;
static DECLARE_WORK(watchdog_work, clocksource_watchdog_work);
static DEFINE_SPINLOCK(watchdog_lock);
static int watchdog_running;
static atomic_t watchdog_reset_pending;

static int clocksource_watchdog_kthread(void *data);
static void __clocksource_change_rating(struct clocksource *cs, int rating);

#define WATCHDOG_INTERVAL (HZ >> 1)
#define WATCHDOG_THRESHOLD (NSEC_PER_SEC >> 4)

static void clocksource_watchdog_work(struct work_struct *work)
{
	kthread_run(clocksource_watchdog_kthread, NULL, "kwatchdog");
}

static void __clocksource_unstable(struct clocksource *cs)
{
	cs->flags &= ~(CLOCK_SOURCE_VALID_FOR_HRES | CLOCK_SOURCE_WATCHDOG);
	cs->flags |= CLOCK_SOURCE_UNSTABLE;
	if (finished_booting)
		schedule_work(&watchdog_work);
}

static void clocksource_unstable(struct clocksource *cs, int64_t delta)
{
	printk(KERN_WARNING "Clocksource %s unstable (delta = %Ld ns)\n",
	       cs->name, delta);
	__clocksource_unstable(cs);
}

void clocksource_mark_unstable(struct clocksource *cs)
{
	unsigned long flags;

	spin_lock_irqsave(&watchdog_lock, flags);
	if (!(cs->flags & CLOCK_SOURCE_UNSTABLE)) {
		if (list_empty(&cs->wd_list))
			list_add(&cs->wd_list, &watchdog_list);
		__clocksource_unstable(cs);
	}
	spin_unlock_irqrestore(&watchdog_lock, flags);
}

static void clocksource_watchdog(unsigned long data)
{
	struct clocksource *cs;
	cycle_t csnow, wdnow;
	int64_t wd_nsec, cs_nsec;
	int next_cpu, reset_pending;

	spin_lock(&watchdog_lock);
	if (!watchdog_running)
		goto out;

	reset_pending = atomic_read(&watchdog_reset_pending);

	list_for_each_entry(cs, &watchdog_list, wd_list) {

		
		if (cs->flags & CLOCK_SOURCE_UNSTABLE) {
			if (finished_booting)
				schedule_work(&watchdog_work);
			continue;
		}

		local_irq_disable();
		csnow = cs->read(cs);
		wdnow = watchdog->read(watchdog);
		local_irq_enable();

		
		if (!(cs->flags & CLOCK_SOURCE_WATCHDOG) ||
		    atomic_read(&watchdog_reset_pending)) {
			cs->flags |= CLOCK_SOURCE_WATCHDOG;
			cs->wd_last = wdnow;
			cs->cs_last = csnow;
			continue;
		}

		wd_nsec = clocksource_cyc2ns((wdnow - cs->wd_last) & watchdog->mask,
					     watchdog->mult, watchdog->shift);

		cs_nsec = clocksource_cyc2ns((csnow - cs->cs_last) &
					     cs->mask, cs->mult, cs->shift);
		cs->cs_last = csnow;
		cs->wd_last = wdnow;

		if (atomic_read(&watchdog_reset_pending))
			continue;

		
		if ((abs(cs_nsec - wd_nsec) > WATCHDOG_THRESHOLD)) {
			clocksource_unstable(cs, cs_nsec - wd_nsec);
			continue;
		}

		if (!(cs->flags & CLOCK_SOURCE_VALID_FOR_HRES) &&
		    (cs->flags & CLOCK_SOURCE_IS_CONTINUOUS) &&
		    (watchdog->flags & CLOCK_SOURCE_IS_CONTINUOUS)) {
			cs->flags |= CLOCK_SOURCE_VALID_FOR_HRES;
			tick_clock_notify();
		}
	}

	if (reset_pending)
		atomic_dec(&watchdog_reset_pending);

	next_cpu = cpumask_next(raw_smp_processor_id(), cpu_online_mask);
	if (next_cpu >= nr_cpu_ids)
		next_cpu = cpumask_first(cpu_online_mask);
	watchdog_timer.expires += WATCHDOG_INTERVAL;
	add_timer_on(&watchdog_timer, next_cpu);
out:
	spin_unlock(&watchdog_lock);
}

static inline void clocksource_start_watchdog(void)
{
	if (watchdog_running || !watchdog || list_empty(&watchdog_list))
		return;
	init_timer(&watchdog_timer);
	watchdog_timer.function = clocksource_watchdog;
	watchdog_timer.expires = jiffies + WATCHDOG_INTERVAL;
	add_timer_on(&watchdog_timer, cpumask_first(cpu_online_mask));
	watchdog_running = 1;
}

static inline void clocksource_stop_watchdog(void)
{
	if (!watchdog_running || (watchdog && !list_empty(&watchdog_list)))
		return;
	del_timer(&watchdog_timer);
	watchdog_running = 0;
}

static inline void clocksource_reset_watchdog(void)
{
	struct clocksource *cs;

	list_for_each_entry(cs, &watchdog_list, wd_list)
		cs->flags &= ~CLOCK_SOURCE_WATCHDOG;
}

static void clocksource_resume_watchdog(void)
{
	atomic_inc(&watchdog_reset_pending);
}

static void clocksource_enqueue_watchdog(struct clocksource *cs)
{
	unsigned long flags;

	spin_lock_irqsave(&watchdog_lock, flags);
	if (cs->flags & CLOCK_SOURCE_MUST_VERIFY) {
		
		list_add(&cs->wd_list, &watchdog_list);
		cs->flags &= ~CLOCK_SOURCE_WATCHDOG;
	} else {
		
		if (cs->flags & CLOCK_SOURCE_IS_CONTINUOUS)
			cs->flags |= CLOCK_SOURCE_VALID_FOR_HRES;
		
		if (!watchdog || cs->rating > watchdog->rating) {
			watchdog = cs;
			
			clocksource_reset_watchdog();
		}
	}
	
	clocksource_start_watchdog();
	spin_unlock_irqrestore(&watchdog_lock, flags);
}

static void clocksource_dequeue_watchdog(struct clocksource *cs)
{
	struct clocksource *tmp;
	unsigned long flags;

	spin_lock_irqsave(&watchdog_lock, flags);
	if (cs->flags & CLOCK_SOURCE_MUST_VERIFY) {
		
		list_del_init(&cs->wd_list);
	} else if (cs == watchdog) {
		
		clocksource_reset_watchdog();
		
		watchdog = NULL;
		list_for_each_entry(tmp, &clocksource_list, list) {
			if (tmp == cs || tmp->flags & CLOCK_SOURCE_MUST_VERIFY)
				continue;
			if (!watchdog || tmp->rating > watchdog->rating)
				watchdog = tmp;
		}
	}
	cs->flags &= ~CLOCK_SOURCE_WATCHDOG;
	
	clocksource_stop_watchdog();
	spin_unlock_irqrestore(&watchdog_lock, flags);
}

static int clocksource_watchdog_kthread(void *data)
{
	struct clocksource *cs, *tmp;
	unsigned long flags;
	LIST_HEAD(unstable);

	mutex_lock(&clocksource_mutex);
	spin_lock_irqsave(&watchdog_lock, flags);
	list_for_each_entry_safe(cs, tmp, &watchdog_list, wd_list)
		if (cs->flags & CLOCK_SOURCE_UNSTABLE) {
			list_del_init(&cs->wd_list);
			list_add(&cs->wd_list, &unstable);
		}
	
	clocksource_stop_watchdog();
	spin_unlock_irqrestore(&watchdog_lock, flags);

	
	list_for_each_entry_safe(cs, tmp, &unstable, wd_list) {
		list_del_init(&cs->wd_list);
		__clocksource_change_rating(cs, 0);
	}
	mutex_unlock(&clocksource_mutex);
	return 0;
}

#else 

static void clocksource_enqueue_watchdog(struct clocksource *cs)
{
	if (cs->flags & CLOCK_SOURCE_IS_CONTINUOUS)
		cs->flags |= CLOCK_SOURCE_VALID_FOR_HRES;
}

static inline void clocksource_dequeue_watchdog(struct clocksource *cs) { }
static inline void clocksource_resume_watchdog(void) { }
static inline int clocksource_watchdog_kthread(void *data) { return 0; }

#endif 

void clocksource_suspend(void)
{
	struct clocksource *cs;

	list_for_each_entry_reverse(cs, &clocksource_list, list)
		if (cs->suspend)
			cs->suspend(cs);
}

void clocksource_resume(void)
{
	struct clocksource *cs;

	list_for_each_entry(cs, &clocksource_list, list)
		if (cs->resume)
			cs->resume(cs);

	clocksource_resume_watchdog();
}

void clocksource_touch_watchdog(void)
{
	clocksource_resume_watchdog();
}

static u32 clocksource_max_adjustment(struct clocksource *cs)
{
	u64 ret;
	ret = (u64)cs->mult * 11;
	do_div(ret,100);
	return (u32)ret;
}

static u64 clocksource_max_deferment(struct clocksource *cs)
{
	u64 max_nsecs, max_cycles;

	max_cycles = 1ULL << (63 - (ilog2(cs->mult + cs->maxadj) + 1));

	max_cycles = min_t(u64, max_cycles, (u64) cs->mask);
	max_nsecs = clocksource_cyc2ns(max_cycles, cs->mult - cs->maxadj,
					cs->shift);

	return max_nsecs - (max_nsecs >> 3);
}

#ifndef CONFIG_ARCH_USES_GETTIMEOFFSET

static void clocksource_select(void)
{
	struct clocksource *best, *cs;

	if (!finished_booting || list_empty(&clocksource_list))
		return;
	
	best = list_first_entry(&clocksource_list, struct clocksource, list);
	
	list_for_each_entry(cs, &clocksource_list, list) {
		if (strcmp(cs->name, override_name) != 0)
			continue;
		if (!(cs->flags & CLOCK_SOURCE_VALID_FOR_HRES) &&
		    tick_oneshot_mode_active()) {
			
			printk(KERN_WARNING "Override clocksource %s is not "
			       "HRT compatible. Cannot switch while in "
			       "HRT/NOHZ mode\n", cs->name);
			override_name[0] = 0;
		} else
			
			best = cs;
		break;
	}
	if (curr_clocksource != best) {
		printk(KERN_INFO "Switching to clocksource %s\n", best->name);
		curr_clocksource = best;
		timekeeping_notify(curr_clocksource);
	}
}

#else 

static inline void clocksource_select(void) { }

#endif

static int __init clocksource_done_booting(void)
{
	mutex_lock(&clocksource_mutex);
	curr_clocksource = clocksource_default_clock();
	mutex_unlock(&clocksource_mutex);

	finished_booting = 1;

	clocksource_watchdog_kthread(NULL);

	mutex_lock(&clocksource_mutex);
	clocksource_select();
	mutex_unlock(&clocksource_mutex);
	return 0;
}
fs_initcall(clocksource_done_booting);

static void clocksource_enqueue(struct clocksource *cs)
{
	struct list_head *entry = &clocksource_list;
	struct clocksource *tmp;

	list_for_each_entry(tmp, &clocksource_list, list)
		
		if (tmp->rating >= cs->rating)
			entry = &tmp->list;
	list_add(&cs->list, entry);
}

void __clocksource_updatefreq_scale(struct clocksource *cs, u32 scale, u32 freq)
{
	u64 sec;
	sec = (cs->mask - (cs->mask >> 3));
	do_div(sec, freq);
	do_div(sec, scale);
	if (!sec)
		sec = 1;
	else if (sec > 600 && cs->mask > UINT_MAX)
		sec = 600;

	clocks_calc_mult_shift(&cs->mult, &cs->shift, freq,
			       NSEC_PER_SEC / scale, sec * scale);

	cs->maxadj = clocksource_max_adjustment(cs);
	while ((cs->mult + cs->maxadj < cs->mult)
		|| (cs->mult - cs->maxadj > cs->mult)) {
		cs->mult >>= 1;
		cs->shift--;
		cs->maxadj = clocksource_max_adjustment(cs);
	}

	cs->max_idle_ns = clocksource_max_deferment(cs);
}
EXPORT_SYMBOL_GPL(__clocksource_updatefreq_scale);

int __clocksource_register_scale(struct clocksource *cs, u32 scale, u32 freq)
{

	
	__clocksource_updatefreq_scale(cs, scale, freq);

	
	mutex_lock(&clocksource_mutex);
	clocksource_enqueue(cs);
	clocksource_enqueue_watchdog(cs);
	clocksource_select();
	mutex_unlock(&clocksource_mutex);
	return 0;
}
EXPORT_SYMBOL_GPL(__clocksource_register_scale);


int clocksource_register(struct clocksource *cs)
{
	
	cs->maxadj = clocksource_max_adjustment(cs);
	WARN_ONCE(cs->mult + cs->maxadj < cs->mult,
		"Clocksource %s might overflow on 11%% adjustment\n",
		cs->name);

	
	cs->max_idle_ns = clocksource_max_deferment(cs);

	mutex_lock(&clocksource_mutex);
	clocksource_enqueue(cs);
	clocksource_enqueue_watchdog(cs);
	clocksource_select();
	mutex_unlock(&clocksource_mutex);
	return 0;
}
EXPORT_SYMBOL(clocksource_register);

static void __clocksource_change_rating(struct clocksource *cs, int rating)
{
	list_del(&cs->list);
	cs->rating = rating;
	clocksource_enqueue(cs);
	clocksource_select();
}

void clocksource_change_rating(struct clocksource *cs, int rating)
{
	mutex_lock(&clocksource_mutex);
	__clocksource_change_rating(cs, rating);
	mutex_unlock(&clocksource_mutex);
}
EXPORT_SYMBOL(clocksource_change_rating);

void clocksource_unregister(struct clocksource *cs)
{
	mutex_lock(&clocksource_mutex);
	clocksource_dequeue_watchdog(cs);
	list_del(&cs->list);
	clocksource_select();
	mutex_unlock(&clocksource_mutex);
}
EXPORT_SYMBOL(clocksource_unregister);

#ifdef CONFIG_SYSFS
static ssize_t
sysfs_show_current_clocksources(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	mutex_lock(&clocksource_mutex);
	count = snprintf(buf, PAGE_SIZE, "%s\n", curr_clocksource->name);
	mutex_unlock(&clocksource_mutex);

	return count;
}

static ssize_t sysfs_override_clocksource(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	size_t ret = count;

	
	if (count >= sizeof(override_name))
		return -EINVAL;

	
	if (buf[count-1] == '\n')
		count--;

	mutex_lock(&clocksource_mutex);

	if (count > 0)
		memcpy(override_name, buf, count);
	override_name[count] = 0;
	clocksource_select();

	mutex_unlock(&clocksource_mutex);

	return ret;
}

static ssize_t
sysfs_show_available_clocksources(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct clocksource *src;
	ssize_t count = 0;

	mutex_lock(&clocksource_mutex);
	list_for_each_entry(src, &clocksource_list, list) {
		if (!tick_oneshot_mode_active() ||
		    (src->flags & CLOCK_SOURCE_VALID_FOR_HRES))
			count += snprintf(buf + count,
				  max((ssize_t)PAGE_SIZE - count, (ssize_t)0),
				  "%s ", src->name);
	}
	mutex_unlock(&clocksource_mutex);

	count += snprintf(buf + count,
			  max((ssize_t)PAGE_SIZE - count, (ssize_t)0), "\n");

	return count;
}

static DEVICE_ATTR(current_clocksource, 0644, sysfs_show_current_clocksources,
		   sysfs_override_clocksource);

static DEVICE_ATTR(available_clocksource, 0444,
		   sysfs_show_available_clocksources, NULL);

static struct bus_type clocksource_subsys = {
	.name = "clocksource",
	.dev_name = "clocksource",
};

static struct device device_clocksource = {
	.id	= 0,
	.bus	= &clocksource_subsys,
};

static int __init init_clocksource_sysfs(void)
{
	int error = subsys_system_register(&clocksource_subsys, NULL);

	if (!error)
		error = device_register(&device_clocksource);
	if (!error)
		error = device_create_file(
				&device_clocksource,
				&dev_attr_current_clocksource);
	if (!error)
		error = device_create_file(
				&device_clocksource,
				&dev_attr_available_clocksource);
	return error;
}

device_initcall(init_clocksource_sysfs);
#endif 

static int __init boot_override_clocksource(char* str)
{
	mutex_lock(&clocksource_mutex);
	if (str)
		strlcpy(override_name, str, sizeof(override_name));
	mutex_unlock(&clocksource_mutex);
	return 1;
}

__setup("clocksource=", boot_override_clocksource);

static int __init boot_override_clock(char* str)
{
	if (!strcmp(str, "pmtmr")) {
		printk("Warning: clock=pmtmr is deprecated. "
			"Use clocksource=acpi_pm.\n");
		return boot_override_clocksource("acpi_pm");
	}
	printk("Warning! clock= boot option is deprecated. "
		"Use clocksource=xyz\n");
	return boot_override_clocksource(str);
}

__setup("clock=", boot_override_clock);
