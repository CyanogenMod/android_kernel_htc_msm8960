/*
 *  Copyright (C) 2008 Red Hat, Inc., Eric Paris <eparis@redhat.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mount.h>
#include <linux/mutex.h>
#include <linux/namei.h>
#include <linux/path.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include <linux/atomic.h>

#include <linux/fsnotify_backend.h>
#include "fsnotify.h"

static struct kmem_cache *fsnotify_event_cachep;
static struct kmem_cache *fsnotify_event_holder_cachep;
static struct fsnotify_event *q_overflow_event;
static atomic_t fsnotify_sync_cookie = ATOMIC_INIT(0);

u32 fsnotify_get_cookie(void)
{
	return atomic_inc_return(&fsnotify_sync_cookie);
}
EXPORT_SYMBOL_GPL(fsnotify_get_cookie);

bool fsnotify_notify_queue_is_empty(struct fsnotify_group *group)
{
	BUG_ON(!mutex_is_locked(&group->notification_mutex));
	return list_empty(&group->notification_list) ? true : false;
}

void fsnotify_get_event(struct fsnotify_event *event)
{
	atomic_inc(&event->refcnt);
}

void fsnotify_put_event(struct fsnotify_event *event)
{
	if (!event)
		return;

	if (atomic_dec_and_test(&event->refcnt)) {
		pr_debug("%s: event=%p\n", __func__, event);

		if (event->data_type == FSNOTIFY_EVENT_PATH)
			path_put(&event->path);

		BUG_ON(!list_empty(&event->private_data_list));

		kfree(event->file_name);
		put_pid(event->tgid);
		kmem_cache_free(fsnotify_event_cachep, event);
	}
}

struct fsnotify_event_holder *fsnotify_alloc_event_holder(void)
{
	return kmem_cache_alloc(fsnotify_event_holder_cachep, GFP_KERNEL);
}

void fsnotify_destroy_event_holder(struct fsnotify_event_holder *holder)
{
	if (holder)
		kmem_cache_free(fsnotify_event_holder_cachep, holder);
}

struct fsnotify_event_private_data *fsnotify_remove_priv_from_event(struct fsnotify_group *group, struct fsnotify_event *event)
{
	struct fsnotify_event_private_data *lpriv;
	struct fsnotify_event_private_data *priv = NULL;

	assert_spin_locked(&event->lock);

	list_for_each_entry(lpriv, &event->private_data_list, event_list) {
		if (lpriv->group == group) {
			priv = lpriv;
			list_del(&priv->event_list);
			break;
		}
	}
	return priv;
}

struct fsnotify_event *fsnotify_add_notify_event(struct fsnotify_group *group, struct fsnotify_event *event,
						 struct fsnotify_event_private_data *priv,
						 struct fsnotify_event *(*merge)(struct list_head *,
										 struct fsnotify_event *))
{
	struct fsnotify_event *return_event = NULL;
	struct fsnotify_event_holder *holder = NULL;
	struct list_head *list = &group->notification_list;

	pr_debug("%s: group=%p event=%p priv=%p\n", __func__, group, event, priv);

	if (!list_empty(&event->holder.event_list)) {
alloc_holder:
		holder = fsnotify_alloc_event_holder();
		if (!holder)
			return ERR_PTR(-ENOMEM);
	}

	mutex_lock(&group->notification_mutex);

	if (group->q_len >= group->max_events) {
		event = q_overflow_event;

		fsnotify_get_event(event);
		return_event = event;

		
		priv = NULL;
	}

	if (!list_empty(list) && merge) {
		struct fsnotify_event *tmp;

		tmp = merge(list, event);
		if (tmp) {
			mutex_unlock(&group->notification_mutex);

			if (return_event)
				fsnotify_put_event(return_event);
			if (holder != &event->holder)
				fsnotify_destroy_event_holder(holder);
			return tmp;
		}
	}

	spin_lock(&event->lock);

	if (list_empty(&event->holder.event_list)) {
		if (unlikely(holder))
			fsnotify_destroy_event_holder(holder);
		holder = &event->holder;
	} else if (unlikely(!holder)) {
		spin_unlock(&event->lock);
		mutex_unlock(&group->notification_mutex);

		if (return_event) {
			fsnotify_put_event(return_event);
			return_event = NULL;
		}

		goto alloc_holder;
	}

	group->q_len++;
	holder->event = event;

	fsnotify_get_event(event);
	list_add_tail(&holder->event_list, list);
	if (priv)
		list_add_tail(&priv->event_list, &event->private_data_list);
	spin_unlock(&event->lock);
	mutex_unlock(&group->notification_mutex);

	wake_up(&group->notification_waitq);
	return return_event;
}

struct fsnotify_event *fsnotify_remove_notify_event(struct fsnotify_group *group)
{
	struct fsnotify_event *event;
	struct fsnotify_event_holder *holder;

	BUG_ON(!mutex_is_locked(&group->notification_mutex));

	pr_debug("%s: group=%p\n", __func__, group);

	holder = list_first_entry(&group->notification_list, struct fsnotify_event_holder, event_list);

	event = holder->event;

	spin_lock(&event->lock);
	holder->event = NULL;
	list_del_init(&holder->event_list);
	spin_unlock(&event->lock);

	
	if (holder != &event->holder)
		fsnotify_destroy_event_holder(holder);

	group->q_len--;

	return event;
}

struct fsnotify_event *fsnotify_peek_notify_event(struct fsnotify_group *group)
{
	struct fsnotify_event *event;
	struct fsnotify_event_holder *holder;

	BUG_ON(!mutex_is_locked(&group->notification_mutex));

	holder = list_first_entry(&group->notification_list, struct fsnotify_event_holder, event_list);
	event = holder->event;

	return event;
}

void fsnotify_flush_notify(struct fsnotify_group *group)
{
	struct fsnotify_event *event;
	struct fsnotify_event_private_data *priv;

	mutex_lock(&group->notification_mutex);
	while (!fsnotify_notify_queue_is_empty(group)) {
		event = fsnotify_remove_notify_event(group);
		
		if (group->ops->free_event_priv) {
			spin_lock(&event->lock);
			priv = fsnotify_remove_priv_from_event(group, event);
			spin_unlock(&event->lock);
			if (priv)
				group->ops->free_event_priv(priv);
		}
		fsnotify_put_event(event); 
	}
	mutex_unlock(&group->notification_mutex);
}

static void initialize_event(struct fsnotify_event *event)
{
	INIT_LIST_HEAD(&event->holder.event_list);
	atomic_set(&event->refcnt, 1);

	spin_lock_init(&event->lock);

	INIT_LIST_HEAD(&event->private_data_list);
}

int fsnotify_replace_event(struct fsnotify_event_holder *old_holder,
			   struct fsnotify_event *new_event)
{
	struct fsnotify_event *old_event = old_holder->event;
	struct fsnotify_event_holder *new_holder = &new_event->holder;

	enum event_spinlock_class {
		SPINLOCK_OLD,
		SPINLOCK_NEW,
	};

	pr_debug("%s: old_event=%p new_event=%p\n", __func__, old_event, new_event);

	BUG_ON(!list_empty(&new_holder->event_list));

	spin_lock_nested(&old_event->lock, SPINLOCK_OLD);
	spin_lock_nested(&new_event->lock, SPINLOCK_NEW);

	new_holder->event = new_event;
	list_replace_init(&old_holder->event_list, &new_holder->event_list);

	spin_unlock(&new_event->lock);
	spin_unlock(&old_event->lock);

	
	if (old_holder != &old_event->holder)
		fsnotify_destroy_event_holder(old_holder);

	fsnotify_get_event(new_event); 
	fsnotify_put_event(old_event); 

	return 0;
}

struct fsnotify_event *fsnotify_clone_event(struct fsnotify_event *old_event)
{
	struct fsnotify_event *event;

	event = kmem_cache_alloc(fsnotify_event_cachep, GFP_KERNEL);
	if (!event)
		return NULL;

	pr_debug("%s: old_event=%p new_event=%p\n", __func__, old_event, event);

	memcpy(event, old_event, sizeof(*event));
	initialize_event(event);

	if (event->name_len) {
		event->file_name = kstrdup(old_event->file_name, GFP_KERNEL);
		if (!event->file_name) {
			kmem_cache_free(fsnotify_event_cachep, event);
			return NULL;
		}
	}
	event->tgid = get_pid(old_event->tgid);
	if (event->data_type == FSNOTIFY_EVENT_PATH)
		path_get(&event->path);

	return event;
}

struct fsnotify_event *fsnotify_create_event(struct inode *to_tell, __u32 mask, void *data,
					     int data_type, const unsigned char *name,
					     u32 cookie, gfp_t gfp)
{
	struct fsnotify_event *event;

	event = kmem_cache_zalloc(fsnotify_event_cachep, gfp);
	if (!event)
		return NULL;

	pr_debug("%s: event=%p to_tell=%p mask=%x data=%p data_type=%d\n",
		 __func__, event, to_tell, mask, data, data_type);

	initialize_event(event);

	if (name) {
		event->file_name = kstrdup(name, gfp);
		if (!event->file_name) {
			kmem_cache_free(fsnotify_event_cachep, event);
			return NULL;
		}
		event->name_len = strlen(event->file_name);
	}

	event->tgid = get_pid(task_tgid(current));
	event->sync_cookie = cookie;
	event->to_tell = to_tell;
	event->data_type = data_type;

	switch (data_type) {
	case FSNOTIFY_EVENT_PATH: {
		struct path *path = data;
		event->path.dentry = path->dentry;
		event->path.mnt = path->mnt;
		path_get(&event->path);
		break;
	}
	case FSNOTIFY_EVENT_INODE:
		event->inode = data;
		break;
	case FSNOTIFY_EVENT_NONE:
		event->inode = NULL;
		event->path.dentry = NULL;
		event->path.mnt = NULL;
		break;
	default:
		BUG();
	}

	event->mask = mask;

	return event;
}

static __init int fsnotify_notification_init(void)
{
	fsnotify_event_cachep = KMEM_CACHE(fsnotify_event, SLAB_PANIC);
	fsnotify_event_holder_cachep = KMEM_CACHE(fsnotify_event_holder, SLAB_PANIC);

	q_overflow_event = fsnotify_create_event(NULL, FS_Q_OVERFLOW, NULL,
						 FSNOTIFY_EVENT_NONE, NULL, 0,
						 GFP_KERNEL);
	if (!q_overflow_event)
		panic("unable to allocate fsnotify q_overflow_event\n");

	return 0;
}
subsys_initcall(fsnotify_notification_init);
