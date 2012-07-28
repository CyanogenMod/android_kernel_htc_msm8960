/* kernel/power/powerstate.c
 *
 * Copyright (C) 2005-2008 Google, Inc.
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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/powerstate.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>

enum {
	DEBUG_POWER_STATE = 1U << 0,
};
static int debug_mask;
module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

#define POWER_STATE_TYPE_MASK              (0x0f)
#define POWER_STATE_INITIALIZED            (1U << 8)
#define POWER_STATE_ACTIVE                 (1U << 9)

static DEFINE_SPINLOCK(list_lock);
static LIST_HEAD(inactive_states);
static struct list_head active_power_states[POWER_STATE_TYPE_COUNT];

static int print_state_stat(struct seq_file *m, struct power_state *state)
{
	int state_count = state->stat.count;
	ktime_t active_time = ktime_set(0, 0);
	ktime_t total_time = state->stat.total_time;
	ktime_t max_time = state->stat.max_time;

	if (state->flags & POWER_STATE_ACTIVE) {
		ktime_t now, add_time;
		now = ktime_get();
		add_time = ktime_sub(now, state->stat.last_time);
		state_count++;
		active_time = add_time;
		total_time = ktime_add(total_time, add_time);
		if (add_time.tv64 > max_time.tv64)
			max_time = add_time;
	}
	return seq_printf(m,
		     "\"%s\"\t%d\t%lld\t%lld\t%lld\t%lld\n",
		     state->name,
		     state->stat.count, ktime_to_ns(active_time),
		     ktime_to_ns(total_time),
		     ktime_to_ns(max_time),
		     ktime_to_ns(state->stat.last_time));
}

static int powerstate_stats_show(struct seq_file *m, void *unused)
{
	unsigned long irqflags;
	struct power_state *state;
	int ret;
	int type;

	spin_lock_irqsave(&list_lock, irqflags);

	ret = seq_puts(m, "name\tcount\tactive_since"
			"\ttotal_time\tmax_time\tlast_change\n");
	list_for_each_entry(state, &inactive_states, link)
		ret = print_state_stat(m, state);
	for (type = 0; type < POWER_STATE_TYPE_COUNT; type++) {
		list_for_each_entry(state, &active_power_states[type], link)
			ret = print_state_stat(m, state);
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
	return 0;
}

static void power_state_end_stat_locked(struct power_state *state, int expired)
{
	ktime_t duration;
	ktime_t now;
	if (!(state->flags & POWER_STATE_ACTIVE))
		return;
	now = ktime_get();
	state->stat.count++;
	duration = ktime_sub(now, state->stat.last_time);
	state->stat.total_time = ktime_add(state->stat.total_time, duration);
	if (ktime_to_ns(duration) > ktime_to_ns(state->stat.max_time))
		state->stat.max_time = duration;
	state->stat.last_time = ktime_get();
}

void power_state_init_internal(struct power_state *state, int type, const char *name)
{
	unsigned long irqflags = 0;
	int ascii = 0;

	if (name)
		state->name = name;
	BUG_ON(!state->name);

	if (strlen(state->name) > 3)
		ascii = isascii(state->name[0]) & isascii(state->name[1]) & isascii(state->name[2]);
	else
		ascii = isascii(state->name[0]);

	if (!ascii)
		dump_stack();

	if (debug_mask & DEBUG_POWER_STATE)
		pr_info("power_state_init name=%s\n", state->name);

	state->stat.count = 0;
	state->stat.total_time = ktime_set(0, 0);
	state->stat.max_time = ktime_set(0, 0);
	state->stat.last_time = ktime_set(0, 0);
	state->flags = (type & POWER_STATE_TYPE_MASK) | POWER_STATE_INITIALIZED;

	INIT_LIST_HEAD(&state->link);
	spin_lock_irqsave(&list_lock, irqflags);
	list_add(&state->link, &inactive_states);
	spin_unlock_irqrestore(&list_lock, irqflags);
}

void power_state_init(struct power_state *state, const char *name)
{
	power_state_init_internal(state, POWER_STATE_STATE, name);
}

void power_state_destroy(struct power_state *state)
{
	unsigned long irqflags;
	if (debug_mask & DEBUG_POWER_STATE)
		pr_info("power_state_destroy name=%s\n", state->name);
	spin_lock_irqsave(&list_lock, irqflags);
	state->flags &= ~POWER_STATE_INITIALIZED;
	list_del(&state->link);
	spin_unlock_irqrestore(&list_lock, irqflags);
}

void power_state_start(struct power_state *state)
{
	int type;
	unsigned long irqflags;

	spin_lock_irqsave(&list_lock, irqflags);
	type = state->flags & POWER_STATE_TYPE_MASK;
	BUG_ON(type >= POWER_STATE_TYPE_COUNT);
	BUG_ON(!(state->flags & POWER_STATE_INITIALIZED));

	if (!(state->flags & POWER_STATE_ACTIVE)) {
		state->flags |= POWER_STATE_ACTIVE;
		state->stat.last_time = ktime_get();
	}
	list_del(&state->link);

	if (debug_mask & DEBUG_POWER_STATE)
		pr_info("power_state_start: %s, type %d\n", state->name, type);

	list_add(&state->link, &active_power_states[type]);
	spin_unlock_irqrestore(&list_lock, irqflags);
}

void power_state_trigger(struct power_state *state)
{
	state->stat.count++;
}

void power_state_end(struct power_state *state)
{
	int type;
	unsigned long irqflags;
	spin_lock_irqsave(&list_lock, irqflags);
	type = state->flags & POWER_STATE_TYPE_MASK;

	power_state_end_stat_locked(state, 0);

	if (debug_mask & DEBUG_POWER_STATE)
		pr_info("power_state_end: %s\n", state->name);
	state->flags &= ~POWER_STATE_ACTIVE;
	list_del(&state->link);
	list_add(&state->link, &inactive_states);

	spin_unlock_irqrestore(&list_lock, irqflags);
}

int power_state_active(struct power_state *state)
{
	return !!(state->flags & POWER_STATE_ACTIVE);
}

static int powerstate_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, powerstate_stats_show, NULL);
}

static const struct file_operations powerstate_stats_fops = {
	.owner = THIS_MODULE,
	.open = powerstate_stats_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init powerstate_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(active_power_states); i++)
		INIT_LIST_HEAD(&active_power_states[i]);

	proc_create("powerstate", S_IRUGO, NULL, &powerstate_stats_fops);

	return 0;
}

static void  __exit powerstate_exit(void)
{
	remove_proc_entry("powerstate", NULL);
}

core_initcall(powerstate_init);
module_exit(powerstate_exit);
