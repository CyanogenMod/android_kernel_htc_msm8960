/* kernel/power/userpowerstate.c
 *
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

#include <linux/ctype.h>
#include <linux/module.h>
#include <linux/powerstate.h>
#include <linux/slab.h>

#include "power.h"

enum {
	DEBUG_FAILURE	= BIT(0),
	DEBUG_ERROR	= BIT(1),
	DEBUG_NEW	= BIT(2),
	DEBUG_ACCESS	= BIT(3),
	DEBUG_LOOKUP	= BIT(4),
};
static int debug_mask = DEBUG_FAILURE;
module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

static DEFINE_MUTEX(tree_lock);

struct user_power_state {
	struct rb_node		node;
	struct power_state	power_state;
	char			name[0];
};
struct rb_root user_power_states;

static struct user_power_state *lookup_power_state_name(
	const char *buf, int allocate)
{
	struct rb_node **p = &user_power_states.rb_node;
	struct rb_node *parent = NULL;
	struct user_power_state *l;
	int diff;
	int name_len;
	const char *arg;

	/* Find length of lock name and start of optional timeout string */
	arg = buf;
	while (*arg && !isspace(*arg))
		arg++;
	name_len = arg - buf;
	if (!name_len)
		goto bad_arg;
	while (isspace(*arg))
		arg++;

	/* Lookup power state in rbtree */
	while (*p) {
		parent = *p;
		l = rb_entry(parent, struct user_power_state, node);
		diff = strncmp(buf, l->name, name_len);
		if (!diff && l->name[name_len])
			diff = -1;
		if (debug_mask & DEBUG_ERROR)
			pr_info("lookup_power_state_name: compare %.*s %s %d\n",
				name_len, buf, l->name, diff);

		if (diff < 0)
			p = &(*p)->rb_left;
		else if (diff > 0)
			p = &(*p)->rb_right;
		else
			return l;
	}

	/* Allocate and add new powerstate to rbtree */
	if (!allocate) {
		if (debug_mask & DEBUG_ERROR)
			pr_info("lookup_power_state_name: %.*s not found\n",
				name_len, buf);
		return ERR_PTR(-EINVAL);
	}
	l = kzalloc(sizeof(*l) + name_len + 1, GFP_KERNEL);
	if (l == NULL) {
		if (debug_mask & DEBUG_FAILURE)
			pr_err("lookup_power_state_name: failed to allocate "
				"memory for %.*s\n", name_len, buf);
		return ERR_PTR(-ENOMEM);
	}
	memcpy(l->name, buf, name_len);
	if (debug_mask & DEBUG_NEW)
		pr_info("lookup_power_state_name: new power state %s\n", l->name);
	power_state_init(&l->power_state, l->name);
	rb_link_node(&l->node, parent, p);
	rb_insert_color(&l->node, &user_power_states);
	return l;

bad_arg:
	if (debug_mask & DEBUG_ERROR)
		pr_info("lookup_power_state_name: power state, %.*s, bad arg, %s\n",
			name_len, buf, arg);
	return ERR_PTR(-EINVAL);
}

ssize_t power_state_start_show(
	struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	char *s = buf;
	char *end = buf + PAGE_SIZE;
	struct rb_node *n;
	struct user_power_state *l;

	mutex_lock(&tree_lock);

	for (n = rb_first(&user_power_states); n != NULL; n = rb_next(n)) {
		l = rb_entry(n, struct user_power_state, node);
		if (power_state_active(&l->power_state))
			s += scnprintf(s, end - s, "%s ", l->name);
	}
	s += scnprintf(s, end - s, "\n");

	mutex_unlock(&tree_lock);
	return (s - buf);
}

ssize_t power_state_start_store(
	struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t n)
{
	struct user_power_state *l;

	mutex_lock(&tree_lock);
	l = lookup_power_state_name(buf, 1);
	if (IS_ERR(l)) {
		n = PTR_ERR(l);
		goto bad_name;
	}

	if (debug_mask & DEBUG_ACCESS)
		pr_info("power_state_store: %s\n", l->name);

	power_state_start(&l->power_state);
bad_name:
	mutex_unlock(&tree_lock);
	return n;
}


ssize_t power_state_end_show(
	struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	char *s = buf;
	char *end = buf + PAGE_SIZE;
	struct rb_node *n;
	struct user_power_state *l;

	mutex_lock(&tree_lock);

	for (n = rb_first(&user_power_states); n != NULL; n = rb_next(n)) {
		l = rb_entry(n, struct user_power_state, node);
		if (!power_state_active(&l->power_state))
			s += scnprintf(s, end - s, "%s ", l->name);
	}
	s += scnprintf(s, end - s, "\n");

	mutex_unlock(&tree_lock);
	return (s - buf);
}

ssize_t power_state_end_store(
	struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t n)
{
	struct user_power_state *l;

	mutex_lock(&tree_lock);
	l = lookup_power_state_name(buf, 0);
	if (IS_ERR(l)) {
		n = PTR_ERR(l);
		goto not_found;
	}

	if (debug_mask & DEBUG_ACCESS)
		pr_info("power_state_end_store: %s\n", l->name);

	power_state_end(&l->power_state);
not_found:
	mutex_unlock(&tree_lock);
	return n;
}

ssize_t power_state_trigger_show(
	struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return 0;
}

ssize_t power_state_trigger_store(
	struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t n)
{
	struct user_power_state *l;

	mutex_lock(&tree_lock);
	l = lookup_power_state_name(buf, 1);
	if (IS_ERR(l)) {
		n = PTR_ERR(l);
		goto not_found;
	}

	if (debug_mask & DEBUG_ACCESS)
		pr_info("power_state_end_store: %s\n", l->name);

	power_state_trigger(&l->power_state);
not_found:
	mutex_unlock(&tree_lock);
	return n;
}
