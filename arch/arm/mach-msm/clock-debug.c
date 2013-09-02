/*
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2007-2012, The Linux Foundation. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/clk.h>
#include <linux/list.h>
#include <linux/clkdev.h>
#include <mach/clk-provider.h>

#include "clock.h"

/**
 * HTC clock debugging functions
 */
static int clock_debug_local_get(void *data, u64 *val);
static int clock_debug_enable_get(void *data, u64 *val);
static int clock_debug_rate_get(void *data, u64 *val);
static struct dentry *debugfs_clock_base;
static struct clk_lookup *msm_clocks;
static size_t num_msm_clocks;

static struct clk *clock_debug_parent_get(void *data)
{
	struct clk *clock = data;

	if (clock->ops->get_parent)
		return clock->ops->get_parent(clock);

	return NULL;
}

static int htc_clock_dump(struct clk *clock, struct seq_file *m)
{
	int len = 0;
	u64 value = 0;
	struct clk *parent;
	char nam_buf[20], en_buf[20], hz_buf[20], loc_buf[20], par_buf[20];

	if (!clock)
		return 0;

	memset(nam_buf,  ' ', sizeof(nam_buf));
	nam_buf[19] = 0;
	memset(en_buf, 0, sizeof(en_buf));
	memset(hz_buf, 0, sizeof(hz_buf));
	memset(loc_buf, 0, sizeof(loc_buf));
	memset(par_buf,  ' ', sizeof(par_buf));
	par_buf[19] = 0;

	len = strlen(clock->dbg_name);
	if (len > 19)
		len = 19;
	memcpy(nam_buf, clock->dbg_name, len);

	clock_debug_enable_get(clock, &value);
	if (value)
		sprintf(en_buf, "Y");
	else
		sprintf(en_buf, "N");

	clock_debug_rate_get(clock, &value);
	sprintf(hz_buf, "%llu", value);

	clock_debug_local_get(clock, &value);
	if (value)
		sprintf(loc_buf, "Y");
	else
		sprintf(loc_buf, "N");

	parent = clock_debug_parent_get(clock);
	if (parent) {
		len = strlen(parent->dbg_name);
		if (len > 19)
			len = 19;
		memcpy(par_buf, parent->dbg_name, len);
	} else
		memcpy(par_buf, "NULL", 4);

	if (m)
		seq_printf(m, "%s: [EN]%s, [LOC]%s, [SRC]%s, [FREQ]%s\n", nam_buf, en_buf, loc_buf, par_buf, hz_buf);
	else
		pr_info("%s: [EN]%s, [LOC]%s, [SRC]%s, [FREQ]%s\n", nam_buf, en_buf, loc_buf, par_buf, hz_buf);

	return 0;
}

static int list_clocks_show(struct seq_file *m, void *unused)
{
	int index;
	char *title_msg = "------------ HTC Clock -------------\n";

	seq_printf(m, title_msg);
	for (index = 0; index < num_msm_clocks; index++)
		htc_clock_dump(msm_clocks[index].clk, m);
	return 0;
}

static int list_clocks_open(struct inode *inode, struct file *file)
{
	return single_open(file, list_clocks_show, inode->i_private);
}

static const struct file_operations list_clocks_fops = {
	.open		= list_clocks_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static int htc_clock_status_debug_init(void)
{
	debugfs_clock_base = debugfs_create_dir("htc_clock", NULL);
	if (!debugfs_clock_base)
		return -ENOMEM;

	if (!debugfs_create_file("list_clocks", S_IRUGO, debugfs_clock_base,
				&msm_clocks, &list_clocks_fops))
		return -ENOMEM;

	return 0;
}
/* END HTC clock debugging */

static int clock_debug_rate_set(void *data, u64 val)
{
	struct clk *clock = data;
	int ret;

	/* Only increases to max rate will succeed, but that's actually good
	 * for debugging purposes so we don't check for error. */
	if (clock->flags & CLKFLAG_MAX)
		clk_set_max_rate(clock, val);
	ret = clk_set_rate(clock, val);
	if (ret)
		pr_err("clk_set_rate(%s, %lu) failed (%d)\n", clock->dbg_name,
				(unsigned long)val, ret);

	return ret;
}

static int clock_debug_rate_get(void *data, u64 *val)
{
	struct clk *clock = data;
	*val = clk_get_rate(clock);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(clock_rate_fops, clock_debug_rate_get,
			clock_debug_rate_set, "%llu\n");

static struct clk *measure;

static int clock_debug_measure_get(void *data, u64 *val)
{
	struct clk *clock = data;
	int ret, is_hw_gated;

	/* Check to see if the clock is in hardware gating mode */
	if (clock->ops->in_hwcg_mode)
		is_hw_gated = clock->ops->in_hwcg_mode(clock);
	else
		is_hw_gated = 0;

	ret = clk_set_parent(measure, clock);
	if (!ret) {
		/*
		 * Disable hw gating to get accurate rate measurements. Only do
		 * this if the clock is explictly enabled by software. This
		 * allows us to detect errors where clocks are on even though
		 * software is not requesting them to be on due to broken
		 * hardware gating signals.
		 */
		if (is_hw_gated && clock->count)
			clock->ops->disable_hwcg(clock);
		*val = clk_get_rate(measure);
		/* Reenable hwgating if it was disabled */
		if (is_hw_gated && clock->count)
			clock->ops->enable_hwcg(clock);
	}

	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(clock_measure_fops, clock_debug_measure_get,
			NULL, "%lld\n");

static int clock_debug_enable_set(void *data, u64 val)
{
	struct clk *clock = data;
	int rc = 0;

	if (val)
		rc = clk_prepare_enable(clock);
	else
		clk_disable_unprepare(clock);

	return rc;
}

static int clock_debug_enable_get(void *data, u64 *val)
{
	struct clk *clock = data;
	int enabled;

	if (clock->ops->is_enabled)
		enabled = clock->ops->is_enabled(clock);
	else
		enabled = !!(clock->count);

	*val = enabled;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(clock_enable_fops, clock_debug_enable_get,
			clock_debug_enable_set, "%lld\n");

static int clock_debug_local_get(void *data, u64 *val)
{
	struct clk *clock = data;

	if (!clock->ops->is_local)
		*val = true;
	else
		*val = clock->ops->is_local(clock);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(clock_local_fops, clock_debug_local_get,
			NULL, "%llu\n");

static int clock_debug_hwcg_get(void *data, u64 *val)
{
	struct clk *clock = data;
	if (clock->ops->in_hwcg_mode)
		*val = !!clock->ops->in_hwcg_mode(clock);
	else
		*val = 0;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(clock_hwcg_fops, clock_debug_hwcg_get,
			NULL, "%llu\n");

static int fmax_rates_show(struct seq_file *m, void *unused)
{
	struct clk *clock = m->private;
	int level = 0;

	int vdd_level = find_vdd_level(clock, clock->rate);
	if (vdd_level < 0) {
		seq_printf(m, "could not find_vdd_level for %s, %ld\n",
			   clock->dbg_name, clock->rate);
		return 0;
	}
	for (level = 0; level < clock->num_fmax; level++) {
		if (vdd_level == level)
			seq_printf(m, "[%lu] ", clock->fmax[level]);
		else
			seq_printf(m, "%lu ", clock->fmax[level]);
	}
	seq_printf(m, "\n");

	return 0;
}

static int fmax_rates_open(struct inode *inode, struct file *file)
{
	return single_open(file, fmax_rates_show, inode->i_private);
}

static const struct file_operations fmax_rates_fops = {
	.open		= fmax_rates_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static int list_rates_show(struct seq_file *m, void *unused)
{
	struct clk *clock = m->private;
	int rate, level, fmax = 0, i = 0;

	/* Find max frequency supported within voltage constraints. */
	if (!clock->vdd_class) {
		fmax = INT_MAX;
	} else {
		for (level = 0; level < clock->num_fmax; level++)
			if (clock->fmax[level])
				fmax = clock->fmax[level];
	}

	/*
	 * List supported frequencies <= fmax. Higher frequencies may appear in
	 * the frequency table, but are not valid and should not be listed.
	 */
	while ((rate = clock->ops->list_rate(clock, i++)) >= 0) {
		if (rate <= fmax)
			seq_printf(m, "%u\n", rate);
	}

	return 0;
}

static int list_rates_open(struct inode *inode, struct file *file)
{
	return single_open(file, list_rates_show, inode->i_private);
}

static const struct file_operations list_rates_fops = {
	.open		= list_rates_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static struct dentry *debugfs_base;
static u32 debug_suspend;

struct clk_table {
	struct list_head node;
	struct clk_lookup *clocks;
	size_t num_clocks;
};

static int clock_debug_add(struct clk *clock)
{
	char temp[50], *ptr;
	struct dentry *clk_dir;

	if (!debugfs_base)
		return -ENOMEM;

	strlcpy(temp, clock->dbg_name, ARRAY_SIZE(temp));
	for (ptr = temp; *ptr; ptr++)
		*ptr = tolower(*ptr);

	clk_dir = debugfs_create_dir(temp, debugfs_base);
	if (!clk_dir)
		return -ENOMEM;

	if (!debugfs_create_file("rate", S_IRUGO | S_IWUSR, clk_dir,
				clock, &clock_rate_fops))
		goto error;

	if (!debugfs_create_file("enable", S_IRUGO | S_IWUSR, clk_dir,
				clock, &clock_enable_fops))
		goto error;

	if (!debugfs_create_file("is_local", S_IRUGO, clk_dir, clock,
				&clock_local_fops))
		goto error;

	if (!debugfs_create_file("has_hw_gating", S_IRUGO, clk_dir, clock,
				&clock_hwcg_fops))
		goto error;

	if (measure &&
	    !clk_set_parent(measure, clock) &&
	    !debugfs_create_file("measure", S_IRUGO, clk_dir, clock,
				&clock_measure_fops))
		goto error;

	if (clock->ops->list_rate)
		if (!debugfs_create_file("list_rates",
				S_IRUGO, clk_dir, clock, &list_rates_fops))
			goto error;

	if (clock->vdd_class && !debugfs_create_file("fmax_rates",
				S_IRUGO, clk_dir, clock, &fmax_rates_fops))
			goto error;

	return 0;
error:
	debugfs_remove_recursive(clk_dir);
	return -ENOMEM;
}
static LIST_HEAD(clk_list);
static DEFINE_SPINLOCK(clk_list_lock);

/**
 * clock_debug_register() - Add additional clocks to clock debugfs hierarchy
 * @table: Table of clocks to create debugfs nodes for
 * @size: Size of @table
 *
 * Use this function to register additional clocks in debugfs. The clock debugfs
 * hierarchy must have already been initialized with clock_debug_init() prior to
 * calling this function. Unlike clock_debug_init(), this may be called multiple
 * times with different clock lists and can be used after the kernel has
 * finished booting.
 */
int clock_debug_register(struct clk_lookup *table, size_t size)
{
	struct clk_table *clk_table;
	unsigned long flags;
	int i;

	clk_table = kmalloc(sizeof(*clk_table), GFP_KERNEL);
	if (!clk_table)
		return -ENOMEM;

	clk_table->clocks = table;
	clk_table->num_clocks = size;

	spin_lock_irqsave(&clk_list_lock, flags);
	list_add_tail(&clk_table->node, &clk_list);
	spin_unlock_irqrestore(&clk_list_lock, flags);

	for (i = 0; i < size; i++)
		clock_debug_add(table[i].clk);

	/* HTC clock debugging */
	msm_clocks = table;
	num_msm_clocks = size;

	return 0;
}

/**
 * clock_debug_init() - Initialize clock debugfs
 */
int __init clock_debug_init(void)
{
	debugfs_base = debugfs_create_dir("clk", NULL);
	if (!debugfs_base)
		return -ENOMEM;
	if (!debugfs_create_u32("debug_suspend", S_IRUGO | S_IWUSR,
				debugfs_base, &debug_suspend)) {
		debugfs_remove_recursive(debugfs_base);
		return -ENOMEM;
	}

	measure = clk_get_sys("debug", "measure");
	if (IS_ERR(measure))
		measure = NULL;

	/* HTC clock debugging */
	htc_clock_status_debug_init();

	return 0;
}

static int clock_debug_print_clock(struct clk *c)
{
	char *start = "";

	if (!c || !c->prepare_count)
		return 0;

	pr_info("\t");
	do {
		if (c->vdd_class)
			pr_cont("%s%s:%u:%u [%ld, %lu]", start, c->dbg_name,
				c->prepare_count, c->count, c->rate,
				c->vdd_class->cur_level);
		else
			pr_cont("%s%s:%u:%u [%ld]", start, c->dbg_name,
				c->prepare_count, c->count, c->rate);
		start = " -> ";
	} while ((c = clk_get_parent(c)));

	pr_cont("\n");

	return 1;
}

/**
 * clock_debug_print_enabled() - Print names of enabled clocks for suspend debug
 *
 * Print the names of enabled clocks and their parents if debug_suspend is set
 */
void clock_debug_print_enabled(void)
{
	struct clk_table *table;
	unsigned long flags;
	int i, cnt = 0;

	if (likely(!debug_suspend))
		return;

	pr_info("Enabled clocks:\n");
	spin_lock_irqsave(&clk_list_lock, flags);
	list_for_each_entry(table, &clk_list, node) {
		for (i = 0; i < table->num_clocks; i++)
			cnt += clock_debug_print_clock(table->clocks[i].clk);
	}
	spin_unlock_irqrestore(&clk_list_lock, flags);

	if (cnt)
		pr_info("Enabled clock count: %d\n", cnt);
	else
		pr_info("No clocks enabled.\n");

}
