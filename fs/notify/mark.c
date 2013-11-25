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
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/srcu.h>

#include <linux/atomic.h>

#include <linux/fsnotify_backend.h>
#include "fsnotify.h"

struct srcu_struct fsnotify_mark_srcu;
static DEFINE_SPINLOCK(destroy_lock);
static LIST_HEAD(destroy_list);
static DECLARE_WAIT_QUEUE_HEAD(destroy_waitq);

void fsnotify_get_mark(struct fsnotify_mark *mark)
{
	atomic_inc(&mark->refcnt);
}

void fsnotify_put_mark(struct fsnotify_mark *mark)
{
	if (atomic_dec_and_test(&mark->refcnt))
		mark->free_mark(mark);
}

void fsnotify_destroy_mark(struct fsnotify_mark *mark)
{
	struct fsnotify_group *group;
	struct inode *inode = NULL;

	spin_lock(&mark->lock);

	group = mark->group;

	
	if (!(mark->flags & FSNOTIFY_MARK_FLAG_ALIVE)) {
		spin_unlock(&mark->lock);
		return;
	}

	mark->flags &= ~FSNOTIFY_MARK_FLAG_ALIVE;

	spin_lock(&group->mark_lock);

	if (mark->flags & FSNOTIFY_MARK_FLAG_INODE) {
		inode = mark->i.inode;
		fsnotify_destroy_inode_mark(mark);
	} else if (mark->flags & FSNOTIFY_MARK_FLAG_VFSMOUNT)
		fsnotify_destroy_vfsmount_mark(mark);
	else
		BUG();

	list_del_init(&mark->g_list);

	spin_unlock(&group->mark_lock);
	spin_unlock(&mark->lock);

	spin_lock(&destroy_lock);
	list_add(&mark->destroy_list, &destroy_list);
	spin_unlock(&destroy_lock);
	wake_up(&destroy_waitq);

	if (group->ops->freeing_mark)
		group->ops->freeing_mark(mark, group);


	if (inode && (mark->flags & FSNOTIFY_MARK_FLAG_OBJECT_PINNED))
		iput(inode);


	if (unlikely(atomic_dec_and_test(&group->num_marks)))
		fsnotify_final_destroy_group(group);
}

void fsnotify_set_mark_mask_locked(struct fsnotify_mark *mark, __u32 mask)
{
	assert_spin_locked(&mark->lock);

	mark->mask = mask;

	if (mark->flags & FSNOTIFY_MARK_FLAG_INODE)
		fsnotify_set_inode_mark_mask_locked(mark, mask);
}

void fsnotify_set_mark_ignored_mask_locked(struct fsnotify_mark *mark, __u32 mask)
{
	assert_spin_locked(&mark->lock);

	mark->ignored_mask = mask;
}

int fsnotify_add_mark(struct fsnotify_mark *mark,
		      struct fsnotify_group *group, struct inode *inode,
		      struct vfsmount *mnt, int allow_dups)
{
	int ret = 0;

	BUG_ON(inode && mnt);
	BUG_ON(!inode && !mnt);

	spin_lock(&mark->lock);
	spin_lock(&group->mark_lock);

	mark->flags |= FSNOTIFY_MARK_FLAG_ALIVE;

	mark->group = group;
	list_add(&mark->g_list, &group->marks_list);
	atomic_inc(&group->num_marks);
	fsnotify_get_mark(mark); 

	if (inode) {
		ret = fsnotify_add_inode_mark(mark, group, inode, allow_dups);
		if (ret)
			goto err;
	} else if (mnt) {
		ret = fsnotify_add_vfsmount_mark(mark, group, mnt, allow_dups);
		if (ret)
			goto err;
	} else {
		BUG();
	}

	spin_unlock(&group->mark_lock);

	
	fsnotify_set_mark_mask_locked(mark, mark->mask);

	spin_unlock(&mark->lock);

	if (inode)
		__fsnotify_update_child_dentry_flags(inode);

	return ret;
err:
	mark->flags &= ~FSNOTIFY_MARK_FLAG_ALIVE;
	list_del_init(&mark->g_list);
	mark->group = NULL;
	atomic_dec(&group->num_marks);

	spin_unlock(&group->mark_lock);
	spin_unlock(&mark->lock);

	spin_lock(&destroy_lock);
	list_add(&mark->destroy_list, &destroy_list);
	spin_unlock(&destroy_lock);
	wake_up(&destroy_waitq);

	return ret;
}

void fsnotify_clear_marks_by_group_flags(struct fsnotify_group *group,
					 unsigned int flags)
{
	struct fsnotify_mark *lmark, *mark;
	LIST_HEAD(free_list);

	spin_lock(&group->mark_lock);
	list_for_each_entry_safe(mark, lmark, &group->marks_list, g_list) {
		if (mark->flags & flags) {
			list_add(&mark->free_g_list, &free_list);
			list_del_init(&mark->g_list);
			fsnotify_get_mark(mark);
		}
	}
	spin_unlock(&group->mark_lock);

	list_for_each_entry_safe(mark, lmark, &free_list, free_g_list) {
		fsnotify_destroy_mark(mark);
		fsnotify_put_mark(mark);
	}
}

void fsnotify_clear_marks_by_group(struct fsnotify_group *group)
{
	fsnotify_clear_marks_by_group_flags(group, (unsigned int)-1);
}

void fsnotify_duplicate_mark(struct fsnotify_mark *new, struct fsnotify_mark *old)
{
	assert_spin_locked(&old->lock);
	new->i.inode = old->i.inode;
	new->m.mnt = old->m.mnt;
	new->group = old->group;
	new->mask = old->mask;
	new->free_mark = old->free_mark;
}

void fsnotify_init_mark(struct fsnotify_mark *mark,
			void (*free_mark)(struct fsnotify_mark *mark))
{
	memset(mark, 0, sizeof(*mark));
	spin_lock_init(&mark->lock);
	atomic_set(&mark->refcnt, 1);
	mark->free_mark = free_mark;
}

static int fsnotify_mark_destroy(void *ignored)
{
	struct fsnotify_mark *mark, *next;
	LIST_HEAD(private_destroy_list);

	for (;;) {
		spin_lock(&destroy_lock);
		
		list_replace_init(&destroy_list, &private_destroy_list);
		spin_unlock(&destroy_lock);

		synchronize_srcu(&fsnotify_mark_srcu);

		list_for_each_entry_safe(mark, next, &private_destroy_list, destroy_list) {
			list_del_init(&mark->destroy_list);
			fsnotify_put_mark(mark);
		}

		wait_event_interruptible(destroy_waitq, !list_empty(&destroy_list));
	}

	return 0;
}

static int __init fsnotify_mark_init(void)
{
	struct task_struct *thread;

	thread = kthread_run(fsnotify_mark_destroy, NULL,
			     "fsnotify_mark");
	if (IS_ERR(thread))
		panic("unable to start fsnotify mark destruction thread.");

	return 0;
}
device_initcall(fsnotify_mark_init);
