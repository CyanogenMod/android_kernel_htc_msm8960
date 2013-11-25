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
#include <linux/mount.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>

#include <linux/atomic.h>

#include <linux/fsnotify_backend.h>
#include "fsnotify.h"
#include "../mount.h"

void fsnotify_clear_marks_by_mount(struct vfsmount *mnt)
{
	struct fsnotify_mark *mark, *lmark;
	struct hlist_node *pos, *n;
	struct mount *m = real_mount(mnt);
	LIST_HEAD(free_list);

	spin_lock(&mnt->mnt_root->d_lock);
	hlist_for_each_entry_safe(mark, pos, n, &m->mnt_fsnotify_marks, m.m_list) {
		list_add(&mark->m.free_m_list, &free_list);
		hlist_del_init_rcu(&mark->m.m_list);
		fsnotify_get_mark(mark);
	}
	spin_unlock(&mnt->mnt_root->d_lock);

	list_for_each_entry_safe(mark, lmark, &free_list, m.free_m_list) {
		fsnotify_destroy_mark(mark);
		fsnotify_put_mark(mark);
	}
}

void fsnotify_clear_vfsmount_marks_by_group(struct fsnotify_group *group)
{
	fsnotify_clear_marks_by_group_flags(group, FSNOTIFY_MARK_FLAG_VFSMOUNT);
}

static void fsnotify_recalc_vfsmount_mask_locked(struct vfsmount *mnt)
{
	struct mount *m = real_mount(mnt);
	struct fsnotify_mark *mark;
	struct hlist_node *pos;
	__u32 new_mask = 0;

	assert_spin_locked(&mnt->mnt_root->d_lock);

	hlist_for_each_entry(mark, pos, &m->mnt_fsnotify_marks, m.m_list)
		new_mask |= mark->mask;
	m->mnt_fsnotify_mask = new_mask;
}

void fsnotify_recalc_vfsmount_mask(struct vfsmount *mnt)
{
	spin_lock(&mnt->mnt_root->d_lock);
	fsnotify_recalc_vfsmount_mask_locked(mnt);
	spin_unlock(&mnt->mnt_root->d_lock);
}

void fsnotify_destroy_vfsmount_mark(struct fsnotify_mark *mark)
{
	struct vfsmount *mnt = mark->m.mnt;

	assert_spin_locked(&mark->lock);
	assert_spin_locked(&mark->group->mark_lock);

	spin_lock(&mnt->mnt_root->d_lock);

	hlist_del_init_rcu(&mark->m.m_list);
	mark->m.mnt = NULL;

	fsnotify_recalc_vfsmount_mask_locked(mnt);

	spin_unlock(&mnt->mnt_root->d_lock);
}

static struct fsnotify_mark *fsnotify_find_vfsmount_mark_locked(struct fsnotify_group *group,
								struct vfsmount *mnt)
{
	struct mount *m = real_mount(mnt);
	struct fsnotify_mark *mark;
	struct hlist_node *pos;

	assert_spin_locked(&mnt->mnt_root->d_lock);

	hlist_for_each_entry(mark, pos, &m->mnt_fsnotify_marks, m.m_list) {
		if (mark->group == group) {
			fsnotify_get_mark(mark);
			return mark;
		}
	}
	return NULL;
}

struct fsnotify_mark *fsnotify_find_vfsmount_mark(struct fsnotify_group *group,
						  struct vfsmount *mnt)
{
	struct fsnotify_mark *mark;

	spin_lock(&mnt->mnt_root->d_lock);
	mark = fsnotify_find_vfsmount_mark_locked(group, mnt);
	spin_unlock(&mnt->mnt_root->d_lock);

	return mark;
}

int fsnotify_add_vfsmount_mark(struct fsnotify_mark *mark,
			       struct fsnotify_group *group, struct vfsmount *mnt,
			       int allow_dups)
{
	struct mount *m = real_mount(mnt);
	struct fsnotify_mark *lmark;
	struct hlist_node *node, *last = NULL;
	int ret = 0;

	mark->flags |= FSNOTIFY_MARK_FLAG_VFSMOUNT;

	assert_spin_locked(&mark->lock);
	assert_spin_locked(&group->mark_lock);

	spin_lock(&mnt->mnt_root->d_lock);

	mark->m.mnt = mnt;

	
	if (hlist_empty(&m->mnt_fsnotify_marks)) {
		hlist_add_head_rcu(&mark->m.m_list, &m->mnt_fsnotify_marks);
		goto out;
	}

	
	hlist_for_each_entry(lmark, node, &m->mnt_fsnotify_marks, m.m_list) {
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

		hlist_add_before_rcu(&mark->m.m_list, &lmark->m.m_list);
		goto out;
	}

	BUG_ON(last == NULL);
	
	hlist_add_after_rcu(last, &mark->m.m_list);
out:
	fsnotify_recalc_vfsmount_mask_locked(mnt);
	spin_unlock(&mnt->mnt_root->d_lock);

	return ret;
}
