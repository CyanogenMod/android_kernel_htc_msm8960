/* kernel/power/earlysuspend.c
 *
 * Copyright (C) 2005-2008 Google, Inc.
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

#include <linux/earlysuspend.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/rtc.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>

#include "power.h"
#ifdef CONFIG_TRACING_IRQ_PWR
#include "../drivers/gpio/gpio-msm-common.h"
#define PWR_KEY_MSMz           26
#endif

enum {
	DEBUG_USER_STATE = 1U << 0,
	DEBUG_SUSPEND = 1U << 2,
	DEBUG_VERBOSE = 1U << 3,
	DEBUG_NO_SUSPEND = 1U << 4,
};

#ifdef CONFIG_NO_SUSPEND
static int debug_mask = DEBUG_USER_STATE;
#else
static int debug_mask = DEBUG_USER_STATE;
#endif

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

static DEFINE_MUTEX(early_suspend_lock);
static LIST_HEAD(early_suspend_handlers);
static void early_suspend(struct work_struct *work);
static void late_resume(struct work_struct *work);
static DECLARE_WORK(early_suspend_work, early_suspend);
static DECLARE_WORK(late_resume_work, late_resume);
static DEFINE_SPINLOCK(state_lock);
enum {
	SUSPEND_REQUESTED = 0x1,
	SUSPENDED = 0x2,
	SUSPEND_REQUESTED_AND_SUSPENDED = SUSPEND_REQUESTED | SUSPENDED,
};
static int state;
#ifdef CONFIG_HTC_ONMODE_CHARGING
static LIST_HEAD(onchg_suspend_handlers);
static void onchg_suspend(struct work_struct *work);
static void onchg_resume(struct work_struct *work);

static DECLARE_WORK(onchg_suspend_work, onchg_suspend);
static DECLARE_WORK(onchg_resume_work, onchg_resume);

static int state_onchg;
#endif

#ifdef CONFIG_EARLYSUSPEND_BOOST_CPU_SPEED

extern int skip_cpu_offline;
int has_boost_cpu_func = 0;

static void __ref boost_cpu_speed(int boost)
{
	unsigned long max_wait;
	unsigned int cpu = 0, isfound = 0;

	if (!has_boost_cpu_func)
		return;

	if (boost) {
		skip_cpu_offline = 1;

		for(cpu = 1; cpu < NR_CPUS; cpu++) {
			if (cpu_online(cpu)) {
				isfound = 1;
				break;
			}
		}
		cpu = isfound ? cpu : 1;

		if (!isfound) {
			max_wait = jiffies + msecs_to_jiffies(50);
			cpu_hotplug_driver_lock();
			cpu_up(cpu);
			cpu_hotplug_driver_unlock();
			while (!cpu_active(cpu) && jiffies < max_wait)
				;
		}
#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_ONDEMAND
		ondemand_boost_cpu(1);
#endif

	} else {

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_ONDEMAND
		ondemand_boost_cpu(0);
#endif
		skip_cpu_offline = 0;
	}
}
#else
static void boost_cpu_speed(int boost) { return; }
#endif

void register_early_suspend(struct early_suspend *handler)
{
	struct list_head *pos;

	mutex_lock(&early_suspend_lock);
	list_for_each(pos, &early_suspend_handlers) {
		struct early_suspend *e;
		e = list_entry(pos, struct early_suspend, link);
		if (e->level > handler->level)
			break;
	}
	list_add_tail(&handler->link, pos);
	if ((state & SUSPENDED) && handler->suspend)
		handler->suspend(handler);
	mutex_unlock(&early_suspend_lock);
}
EXPORT_SYMBOL(register_early_suspend);

void unregister_early_suspend(struct early_suspend *handler)
{
	mutex_lock(&early_suspend_lock);
	list_del(&handler->link);
	mutex_unlock(&early_suspend_lock);
}
EXPORT_SYMBOL(unregister_early_suspend);

#define EARLY_SUSPEND_TIMEOUT_VALUE 5
static void early_suspend_handlers_timeout(unsigned long data)
{
	printk(KERN_EMERG "**** early_suspend_handlers %d secs timeout: %pf ****\n", \
		EARLY_SUSPEND_TIMEOUT_VALUE, (void *)data);
	pr_info("### Show Blocked State in ###\n");
	show_state_filter(TASK_UNINTERRUPTIBLE);
	BUG();
}

static void early_suspend(struct work_struct *work)
{
	struct early_suspend *pos;
	struct timer_list timer;
	unsigned long irqflags;
	int abort = 0;

	pr_info("[R] early_suspend start\n");
	mutex_lock(&early_suspend_lock);
	spin_lock_irqsave(&state_lock, irqflags);
	if (state == SUSPEND_REQUESTED) {
		state |= SUSPENDED;
#ifdef CONFIG_HTC_ONMODE_CHARGING
		state_onchg = SUSPEND_REQUESTED_AND_SUSPENDED;
#endif
	}
	else
		abort = 1;
	spin_unlock_irqrestore(&state_lock, irqflags);

	if (abort) {
		if (debug_mask & DEBUG_SUSPEND)
			pr_info("early_suspend: abort, state %d\n", state);
		mutex_unlock(&early_suspend_lock);
		goto abort;
	}

	boost_cpu_speed(1);

	init_timer_on_stack(&timer);
	timer.function = early_suspend_handlers_timeout;

	if (debug_mask & DEBUG_SUSPEND)
		pr_info("early_suspend: call handlers\n");
	list_for_each_entry(pos, &early_suspend_handlers, link) {
		if (pos->suspend != NULL) {
			timer.expires = jiffies + HZ * EARLY_SUSPEND_TIMEOUT_VALUE;
			timer.data = (unsigned long)pos->suspend;
			add_timer(&timer);

			if (debug_mask & DEBUG_VERBOSE)
				pr_info("early_suspend: calling %pf\n", pos->suspend);

			pos->suspend(pos);
			del_timer_sync(&timer);
		}
	}

	destroy_timer_on_stack(&timer);
	boost_cpu_speed(0);
	mutex_unlock(&early_suspend_lock);

	if (debug_mask & DEBUG_SUSPEND)
		pr_info("early_suspend: sync\n");

	suspend_sys_sync_queue();

	if (debug_mask & DEBUG_NO_SUSPEND) {
		pr_info("DEBUG_NO_SUSPEND set, will not suspend\n");
		wake_lock(&no_suspend_wake_lock);
	}

abort:
	spin_lock_irqsave(&state_lock, irqflags);
	if (state == SUSPEND_REQUESTED_AND_SUSPENDED)
		wake_unlock(&main_wake_lock);
	spin_unlock_irqrestore(&state_lock, irqflags);
	pr_info("[R] early_suspend end\n");
}

static void late_resume(struct work_struct *work)
{
	struct early_suspend *pos;
	struct timer_list timer;
	unsigned long irqflags;
	int abort = 0;

	pr_info("[R] late_resume start\n");
	mutex_lock(&early_suspend_lock);
	spin_lock_irqsave(&state_lock, irqflags);
	if (state == SUSPENDED) {
		state &= ~SUSPENDED;
#ifdef CONFIG_HTC_ONMODE_CHARGING
		state_onchg &= ~SUSPEND_REQUESTED_AND_SUSPENDED;
#endif
	}
	else
		abort = 1;
	spin_unlock_irqrestore(&state_lock, irqflags);

	if (abort) {
		if (debug_mask & DEBUG_SUSPEND)
			pr_info("late_resume: abort, state %d\n", state);
		goto abort;
	}

	boost_cpu_speed(1);
	init_timer_on_stack(&timer);
	timer.function = early_suspend_handlers_timeout;

	if (debug_mask & DEBUG_SUSPEND)
		pr_info("late_resume: call handlers\n");
	list_for_each_entry_reverse(pos, &early_suspend_handlers, link) {
		if (pos->resume != NULL) {
			timer.expires = jiffies + HZ * EARLY_SUSPEND_TIMEOUT_VALUE;
			timer.data = (unsigned long)pos->suspend;
			add_timer(&timer);

			if (debug_mask & DEBUG_VERBOSE)
				pr_info("late_resume: calling %pf\n", pos->resume);

			pos->resume(pos);
			del_timer_sync(&timer);
		}
	}

	destroy_timer_on_stack(&timer);
	boost_cpu_speed(0);

	if (debug_mask & DEBUG_SUSPEND)
		pr_info("late_resume: done\n");

	if (debug_mask & DEBUG_NO_SUSPEND)
		wake_unlock(&no_suspend_wake_lock);

abort:
	mutex_unlock(&early_suspend_lock);
	pr_info("[R] late_resume end\n");
}
#ifdef CONFIG_HTC_ONMODE_CHARGING
void register_onchg_suspend(struct early_suspend *handler)
{
	struct list_head *pos;
	mutex_lock(&early_suspend_lock);
	list_for_each(pos, &onchg_suspend_handlers) {
		struct early_suspend *e;
		e = list_entry(pos, struct early_suspend, link);
		if (e->level > handler->level)
			break;
	}
	list_add_tail(&handler->link, pos);
	mutex_unlock(&early_suspend_lock);
}
EXPORT_SYMBOL(register_onchg_suspend);

void unregister_onchg_suspend(struct early_suspend *handler)
{
	mutex_lock(&early_suspend_lock);
	list_del(&handler->link);
	mutex_unlock(&early_suspend_lock);
}
EXPORT_SYMBOL(unregister_onchg_suspend);


static void onchg_suspend(struct work_struct *work)
{
	struct early_suspend *pos;
	unsigned long irqflags;
	int abort = 0;
	pr_info("[R] onchg_suspend start\n");
	mutex_lock(&early_suspend_lock);
	spin_lock_irqsave(&state_lock, irqflags);
	if (state == SUSPEND_REQUESTED_AND_SUSPENDED &&
	    state_onchg == SUSPEND_REQUESTED)
		state_onchg |= SUSPENDED;
	else
		abort = 1;
	spin_unlock_irqrestore(&state_lock, irqflags);

	if (abort) {
		if (debug_mask & DEBUG_SUSPEND)
			pr_info("onchg_suspend: abort, state %d, state_onchg: %d\n", state, state_onchg);
		mutex_unlock(&early_suspend_lock);
		goto abort;
	}

	if (debug_mask & DEBUG_SUSPEND)
		pr_info("onchg_suspend: call handlers\n");

	list_for_each_entry(pos, &onchg_suspend_handlers, link) {
		if (pos->suspend != NULL)
			pos->suspend(pos);
	}
	mutex_unlock(&early_suspend_lock);

abort:
	pr_info("[R] onchg_suspend end\n");
}

static void onchg_resume(struct work_struct *work)
{
	struct early_suspend *pos;
	unsigned long irqflags;
	int abort = 0;
	pr_info("[R] onchg_resume start\n");
	mutex_lock(&early_suspend_lock);
	spin_lock_irqsave(&state_lock, irqflags);
	if (state == SUSPEND_REQUESTED_AND_SUSPENDED &&
	     state_onchg == SUSPENDED)
		state_onchg &= ~SUSPENDED;
	else
		abort = 1;
	spin_unlock_irqrestore(&state_lock, irqflags);

	if (abort) {
		if (debug_mask & DEBUG_SUSPEND)
			pr_info("onchg_resume: abort, state %d, state_onchg: %d\n", state, state_onchg);
		goto abort;
	}
	if (debug_mask & DEBUG_SUSPEND)
		pr_info("onchg_resume: call handlers\n");
	list_for_each_entry_reverse(pos, &onchg_suspend_handlers, link)
		if (pos->resume != NULL)
			pos->resume(pos);
	if (debug_mask & DEBUG_SUSPEND)
		pr_info("onchg_resume: done\n");
abort:
	mutex_unlock(&early_suspend_lock);
	pr_info("[R] onchg_resume end\n");
}

void request_onchg_state(int on)
{
	unsigned long irqflags;
	int old_sleep;
	spin_lock_irqsave(&state_lock, irqflags);
	if (debug_mask & DEBUG_USER_STATE) {
		struct timespec ts;
		struct rtc_time tm;
		getnstimeofday(&ts);
		rtc_time_to_tm(ts.tv_sec, &tm);
		pr_info("request_onchg_state: %s (%d.%d)->%d at %lld "
			"(%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n",
			on == 1 ? "on" : "off",
			state,
			!(state_onchg & SUSPEND_REQUESTED),
			on,
			ktime_to_ns(ktime_get()),
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
	}
	if (state == SUSPEND_REQUESTED_AND_SUSPENDED) {
		old_sleep = state_onchg & SUSPEND_REQUESTED;
		if (!old_sleep && on == 0) {
			state_onchg |= SUSPEND_REQUESTED;
			queue_work(suspend_work_queue, &onchg_suspend_work);
		} else if (old_sleep && on == 1) {
			state_onchg &= ~SUSPEND_REQUESTED;
			queue_work(suspend_work_queue, &onchg_resume_work);
		}
	}
	spin_unlock_irqrestore(&state_lock, irqflags);
}

int get_onchg_state(void)
{
	return state_onchg;
}
#endif

void request_suspend_state(suspend_state_t new_state)
{
	unsigned long irqflags;
	int old_sleep;

	spin_lock_irqsave(&state_lock, irqflags);
	old_sleep = state & SUSPEND_REQUESTED;
	if (debug_mask & DEBUG_USER_STATE) {
		struct timespec ts;
		struct rtc_time tm;
		getnstimeofday(&ts);
		rtc_time_to_tm(ts.tv_sec, &tm);
		pr_info("request_suspend_state: %s (%d->%d) at %lld "
			"(%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n",
			new_state != PM_SUSPEND_ON ? "sleep" : "wakeup",
			requested_suspend_state, new_state,
			ktime_to_ns(ktime_get()),
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
	}
	if (!old_sleep && new_state != PM_SUSPEND_ON) {
		state |= SUSPEND_REQUESTED;
		queue_work(suspend_work_queue, &early_suspend_work);
	} else if (old_sleep && new_state == PM_SUSPEND_ON) {
		state &= ~SUSPEND_REQUESTED;
		wake_lock(&main_wake_lock);
#ifdef CONFIG_TRACING_IRQ_PWR
		pr_info("%s : PWR KEY INT ENABLE : %d\n", __func__,  (__msm_gpio_get_intr_config(PWR_KEY_MSMz)&0x1));
#endif
		queue_work(suspend_work_queue, &late_resume_work);
	}
	requested_suspend_state = new_state;
	spin_unlock_irqrestore(&state_lock, irqflags);
}

suspend_state_t get_suspend_state(void)
{
	return requested_suspend_state;
}
