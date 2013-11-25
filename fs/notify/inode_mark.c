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
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>

#include <linux/atomic.h>

#include <linux/fsnotify_backend.h>
#include "fsnotify.h"

#include "../internal.h"

static void fsnotify_recalc_inode_mask_locked(struct inode *inode)
{
	struct fsnotify_mark *mark;
	struct hlist_node *pos;
	__u32 new_mask = 0;

	assert_spin_locked(&inode->i_lock);

	hlist_for_each_entry(mark, pos, &inode->i_fsnotify_marks, i.i_list)
		new_mask |= mark->mask;
	inode->i_fsnotify_mask = new_mask;
}

void fsnotify_recalc_inode_mask(struct inode *inode)
{
	spin_lock(&inode->i_lock);
	fsnotify_recalc_inode_mask_locked(inode);
	spin_unlock(&inode->i_lock);

	__fsnotify_update_child_dentry_flags(inode);
}

void fsnotify_destroy_inode_mark(struct fsnotify_mark *mark)
{
	struct inode *inode = mark->i.inode;

	assert_spin_locked(&mark->lock);
	assert_spin_locked(&mark->group->mark_lock);

	spin_lock(&inode->i_lock);

	hlist_del_init_rcu(&mark->i.i_list);
	mark->i.inode = NULL;

	fsnotify_recalc_inode_mask_locked(inode);

	spin_unlock(&inode->i_lock);
}

void fsnotify_clear_marks_by_inode(struct inode *inode)
{
	struct fsnotify_mark *mark, *lmark;
	struct hlist_node *pos, *n;
	LIST_HEAD(free_list);

	spin_lock(&inode->i_lock);
	hlist_for_each_entry_safe(mark, pos, n, &inode->i_fsnotify_marks, i.i_list) {
		list_add(&mark->i.free_i_list, &free_list);
		hlist_del_init_rcu(&mark->i.i_list);
		fsnotify_get_mark(mark);
	}
	spin_unlock(&inode->i_lock);

	list_for_each_entry_safe(mark, lmark, &free_list, i.free_i_list) {
		fsnotify_destroy_mark(mark);
		fsnotify_put_mark(mark);
	}
}

void fsnotify_clear_inode_marks_by_group(struct fsnotify_group *group)
{
	fsnotify_clear_marks_by_group_flags(group, FSNOTIFY_MARK_FLAG_INODE);
}

struct fsnotify_mark *fsnotify_find_inode_mark_locked(struct fsnotify_group *group,
						      struct inode *inode)
{
	struct fsnotify_mark *mark;
	struct hlist_node *pos;

	assert_spin_locked(&inode->i_lock);

	hlist_for_each_entry(mark, pos, &inode->i_fsnotify_marks, i.i_list) {
		if (mark->group == group) {
			fsnotify_get_mark(mark);
			return mark;
		}
	}
	return NULL;
}

struct fsnotify_mark *fsnotify_find_inode_mark(struct fsnotify_group *group,
					       struct inode *inode)
{
	struct fsnotify_mark *mark;

	spin_lock(&inode->i_lock);
	mark = fsnotify_find_inode_mark_locked(group, inode);
	spin_unlock(&inode->i_lock);

	return mark;
}

void fsnotify_set_inode_mark_mask_locked(struct fsnotify_mark *mark,
					 __u32 mask)
{
	struct inode *inode;

	assert_spin_locked(&mark->lock);

	if (mask &&
	    mark->i.inode &&
	    !(mark->flags & FSNOTIFY_MARK_FLAG_OBJECT_PINNED)) {
		mark->flags |= FSNOTIFY_MARK_FLAG_OBJECT_PINNED;
		inode = igrab(mark->i.inode);
		BUG_ON(!inode);
	}
}

int fsnotify_add_inode_mark(struct fsnotify_mark *mark,
			    struct fsnotify_group *group, struct inode *inode,
			    int allow_dups)
{
	struct fsnotify_mark *lmark;
	struct hlist_node *node, *last = NULL;
	int ret = 0;

	mark->flags |= FSNOTIFY_MARK_FLAG_INODE;

	assert_spin_locked(&mark->lock);
	assert_spin_locked(&group->mark_lock);

	spin_lock(&inode->i_lock);

	mark->i.inode = inode;

	
	if (hlist_empty(&inode->i_fsnotify_marks)) {
		hlist_add_head_rcu(&mark->i.i_list, &inode->i_fsnotify_marks);
		goto out;
	}

	
	hlist_for_each_entry(lmark, node, &inode->i_fsnotify_marks, i.i_list) {
		last = node;

		if ((lmark->group == group) && !allow_dups) {
			ret = -EEXIST;
			goto out;
		}

		if (mark->group->priority < lmark->group->priority)
			continue;

		if ((mark->group->priority == lmark->group->priority) &&
		    (mark->group < lmark->group))
			continue;

		hlist_add_before_rcu(&mark->i.i_list, &lmark->i.i_list);
		goto out;
	}

	BUG_ON(last == NULL);
	
	hlist_add_after_rcu(last, &mark->i.i_list);
out:
	fsnotify_recalc_inode_mask_locked(inode);
	spin_unlock(&inode->i_lock);

	return ret;
}

void fsnotify_unmount_inodes(struct list_head *list)
{
	struct inode *inode, *next_i, *need_iput = NULL;

	spin_lock(&inode_sb_list_lock);
	list_for_each_entry_safe(inode, next_i, list, i_sb_list) {
		struct inode *need_iput_tmp;

		spin_lock(&inode->i_lock);
		if (inode->i_state & (I_FREEING|I_WILL_FREE|I_NEW)) {
			spin_unlock(&inode->i_lock);
			continue;
		}

		if (!atomic_read(&inode->i_count)) {
			spin_unlock(&inode->i_lock);
			continue;
		}

		need_iput_tmp = need_iput;
		need_iput = NULL;

		
		if (inode != need_iput_tmp)
			__iget(inode);
		else
			need_iput_tmp = NULL;
		spin_unlock(&inode->i_lock);

		
		if ((&next_i->i_sb_list != list) &&
		    atomic_read(&next_i->i_count)) {
			spin_lock(&next_i->i_lock);
			if (!(next_i->i_state & (I_FREEING | I_WILL_FREE))) {
				__iget(next_i);
				need_iput = next_i;
			}
			spin_unlock(&next_i->i_lock);
		}

		spin_unlock(&inode_sb_list_lock);

		if (need_iput_tmp)
			iput(need_iput_tmp);

		
		fsnotify(inode, FS_UNMOUNT, inode, FSNOTIFY_EVENT_INODE, NULL, 0);

		fsnotify_inode_delete(inode);

		iput(inode);

		spin_lock(&inode_sb_list_lock);
	}
	spin_unlock(&inode_sb_list_lock);
}
