/*
 * fs/fs-writeback.c
 *
 * Copyright (C) 2002, Linus Torvalds.
 *
 * Contains all the functions related to writing back and waiting
 * upon dirty inodes against superblocks, and writing back dirty
 * pages against inodes.  ie: data writeback.  Writeout of the
 * inode itself is not handled here.
 *
 * 10Apr2002	Andrew Morton
 *		Split out of fs/inode.c
 *		Additions for address_space-based writeback
 */

#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/writeback.h>
#include <linux/blkdev.h>
#include <linux/backing-dev.h>
#include <linux/tracepoint.h>
#include "internal.h"

#define MIN_WRITEBACK_PAGES	(4096UL >> (PAGE_CACHE_SHIFT - 10))

struct wb_writeback_work {
	long nr_pages;
	struct super_block *sb;
	unsigned long *older_than_this;
	enum writeback_sync_modes sync_mode;
	unsigned int tagged_writepages:1;
	unsigned int for_kupdate:1;
	unsigned int range_cyclic:1;
	unsigned int for_background:1;
	enum wb_reason reason;		

	struct list_head list;		
	struct completion *done;	
};

int nr_pdflush_threads;

int writeback_in_progress(struct backing_dev_info *bdi)
{
	return test_bit(BDI_writeback_running, &bdi->state);
}

static inline struct backing_dev_info *inode_to_bdi(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;

	if (strcmp(sb->s_type->name, "bdev") == 0)
		return inode->i_mapping->backing_dev_info;

	return sb->s_bdi;
}

static inline struct inode *wb_inode(struct list_head *head)
{
	return list_entry(head, struct inode, i_wb_list);
}

#define CREATE_TRACE_POINTS
#include <trace/events/writeback.h>

static void bdi_wakeup_flusher(struct backing_dev_info *bdi)
{
	if (bdi->wb.task) {
		wake_up_process(bdi->wb.task);
	} else {
		wake_up_process(default_backing_dev_info.wb.task);
	}
}

static void bdi_queue_work(struct backing_dev_info *bdi,
			   struct wb_writeback_work *work)
{
	trace_writeback_queue(bdi, work);

	spin_lock_bh(&bdi->wb_lock);
	list_add_tail(&work->list, &bdi->work_list);
	if (!bdi->wb.task)
		trace_writeback_nothread(bdi, work);
	bdi_wakeup_flusher(bdi);
	spin_unlock_bh(&bdi->wb_lock);
}

static void
__bdi_start_writeback(struct backing_dev_info *bdi, long nr_pages,
		      bool range_cyclic, enum wb_reason reason)
{
	struct wb_writeback_work *work;

	work = kzalloc(sizeof(*work), GFP_ATOMIC);
	if (!work) {
		if (bdi->wb.task) {
			trace_writeback_nowork(bdi);
			wake_up_process(bdi->wb.task);
		}
		return;
	}

	work->sync_mode	= WB_SYNC_NONE;
	work->nr_pages	= nr_pages;
	work->range_cyclic = range_cyclic;
	work->reason	= reason;

	bdi_queue_work(bdi, work);
}

void bdi_start_writeback(struct backing_dev_info *bdi, long nr_pages,
			enum wb_reason reason)
{
	__bdi_start_writeback(bdi, nr_pages, true, reason);
}

void bdi_start_background_writeback(struct backing_dev_info *bdi)
{
	trace_writeback_wake_background(bdi);
	spin_lock_bh(&bdi->wb_lock);
	bdi_wakeup_flusher(bdi);
	spin_unlock_bh(&bdi->wb_lock);
}

void inode_wb_list_del(struct inode *inode)
{
	struct backing_dev_info *bdi = inode_to_bdi(inode);

	spin_lock(&bdi->wb.list_lock);
	list_del_init(&inode->i_wb_list);
	spin_unlock(&bdi->wb.list_lock);
}

/*
 * Redirty an inode: set its when-it-was dirtied timestamp and move it to the
 * furthest end of its superblock's dirty-inode list.
 *
 * Before stamping the inode's ->dirtied_when, we check to see whether it is
 * already the most-recently-dirtied inode on the b_dirty list.  If that is
 * the case then the inode must have been redirtied while it was being written
 * out and we don't reset its dirtied_when.
 */
static void redirty_tail(struct inode *inode, struct bdi_writeback *wb)
{
	assert_spin_locked(&wb->list_lock);
	if (!list_empty(&wb->b_dirty)) {
		struct inode *tail;

		tail = wb_inode(wb->b_dirty.next);
		if (time_before(inode->dirtied_when, tail->dirtied_when))
			inode->dirtied_when = jiffies;
	}
	list_move(&inode->i_wb_list, &wb->b_dirty);
}

static void requeue_io(struct inode *inode, struct bdi_writeback *wb)
{
	assert_spin_locked(&wb->list_lock);
	list_move(&inode->i_wb_list, &wb->b_more_io);
}

static void inode_sync_complete(struct inode *inode)
{

	smp_mb();
	wake_up_bit(&inode->i_state, __I_SYNC);
}

static bool inode_dirtied_after(struct inode *inode, unsigned long t)
{
	bool ret = time_after(inode->dirtied_when, t);
#ifndef CONFIG_64BIT
	ret = ret && time_before_eq(inode->dirtied_when, jiffies);
#endif
	return ret;
}

static int move_expired_inodes(struct list_head *delaying_queue,
			       struct list_head *dispatch_queue,
			       struct wb_writeback_work *work)
{
	LIST_HEAD(tmp);
	struct list_head *pos, *node;
	struct super_block *sb = NULL;
	struct inode *inode;
	int do_sb_sort = 0;
	int moved = 0;

	while (!list_empty(delaying_queue)) {
		inode = wb_inode(delaying_queue->prev);
		if (work->older_than_this &&
		    inode_dirtied_after(inode, *work->older_than_this))
			break;
		if (sb && sb != inode->i_sb)
			do_sb_sort = 1;
		sb = inode->i_sb;
		list_move(&inode->i_wb_list, &tmp);
		moved++;
	}

	
	if (!do_sb_sort) {
		list_splice(&tmp, dispatch_queue);
		goto out;
	}

	
	while (!list_empty(&tmp)) {
		sb = wb_inode(tmp.prev)->i_sb;
		list_for_each_prev_safe(pos, node, &tmp) {
			inode = wb_inode(pos);
			if (inode->i_sb == sb)
				list_move(&inode->i_wb_list, dispatch_queue);
		}
	}
out:
	return moved;
}

static void queue_io(struct bdi_writeback *wb, struct wb_writeback_work *work)
{
	int moved;
	assert_spin_locked(&wb->list_lock);
	list_splice_init(&wb->b_more_io, &wb->b_io);
	moved = move_expired_inodes(&wb->b_dirty, &wb->b_io, work);
	trace_writeback_queue_io(wb, work, moved);
}

static int write_inode(struct inode *inode, struct writeback_control *wbc)
{
	if (inode->i_sb->s_op->write_inode && !is_bad_inode(inode))
		return inode->i_sb->s_op->write_inode(inode, wbc);
	return 0;
}

static void inode_wait_for_writeback(struct inode *inode,
				     struct bdi_writeback *wb)
{
	DEFINE_WAIT_BIT(wq, &inode->i_state, __I_SYNC);
	wait_queue_head_t *wqh;

	wqh = bit_waitqueue(&inode->i_state, __I_SYNC);
	while (inode->i_state & I_SYNC) {
		spin_unlock(&inode->i_lock);
		spin_unlock(&wb->list_lock);
		__wait_on_bit(wqh, &wq, inode_wait, TASK_UNINTERRUPTIBLE);
		spin_lock(&wb->list_lock);
		spin_lock(&inode->i_lock);
	}
}

static int
writeback_single_inode(struct inode *inode, struct bdi_writeback *wb,
		       struct writeback_control *wbc)
{
	struct address_space *mapping = inode->i_mapping;
	long nr_to_write = wbc->nr_to_write;
	unsigned dirty;
	int ret;

	assert_spin_locked(&wb->list_lock);
	assert_spin_locked(&inode->i_lock);

	if (!atomic_read(&inode->i_count))
		WARN_ON(!(inode->i_state & (I_WILL_FREE|I_FREEING)));
	else
		WARN_ON(inode->i_state & I_WILL_FREE);

	if (inode->i_state & I_SYNC) {
		if (wbc->sync_mode != WB_SYNC_ALL) {
			requeue_io(inode, wb);
			trace_writeback_single_inode_requeue(inode, wbc,
							     nr_to_write);
			return 0;
		}

		inode_wait_for_writeback(inode, wb);
	}

	BUG_ON(inode->i_state & I_SYNC);

	
	inode->i_state |= I_SYNC;
	inode->i_state &= ~I_DIRTY_PAGES;
	spin_unlock(&inode->i_lock);
	spin_unlock(&wb->list_lock);

	ret = do_writepages(mapping, wbc);

	if (wbc->sync_mode == WB_SYNC_ALL) {
		int err = filemap_fdatawait(mapping);
		if (ret == 0)
			ret = err;
	}

	spin_lock(&inode->i_lock);
	dirty = inode->i_state & I_DIRTY;
	inode->i_state &= ~(I_DIRTY_SYNC | I_DIRTY_DATASYNC);
	spin_unlock(&inode->i_lock);
	
	if (dirty & (I_DIRTY_SYNC | I_DIRTY_DATASYNC)) {
		int err = write_inode(inode, wbc);
		if (ret == 0)
			ret = err;
	}

	spin_lock(&wb->list_lock);
	spin_lock(&inode->i_lock);
	inode->i_state &= ~I_SYNC;
	if (!(inode->i_state & I_FREEING)) {
		if ((inode->i_state & I_DIRTY) &&
		    (wbc->sync_mode == WB_SYNC_ALL || wbc->tagged_writepages))
			inode->dirtied_when = jiffies;

		if (mapping_tagged(mapping, PAGECACHE_TAG_DIRTY)) {
			inode->i_state |= I_DIRTY_PAGES;
			if (wbc->nr_to_write <= 0) {
				requeue_io(inode, wb);
			} else {
				redirty_tail(inode, wb);
			}
		} else if (inode->i_state & I_DIRTY) {
			redirty_tail(inode, wb);
		} else {
			list_del_init(&inode->i_wb_list);
		}
	}
	inode_sync_complete(inode);
	trace_writeback_single_inode(inode, wbc, nr_to_write);
	return ret;
}

static long writeback_chunk_size(struct backing_dev_info *bdi,
				 struct wb_writeback_work *work)
{
	long pages;

	if (work->sync_mode == WB_SYNC_ALL || work->tagged_writepages)
		pages = LONG_MAX;
	else {
		pages = min(bdi->avg_write_bandwidth / 2,
			    global_dirty_limit / DIRTY_SCOPE);
		pages = min(pages, work->nr_pages);
		pages = round_down(pages + MIN_WRITEBACK_PAGES,
				   MIN_WRITEBACK_PAGES);
	}

	return pages;
}

/*
 * Write a portion of b_io inodes which belong to @sb.
 *
 * If @only_this_sb is true, then find and write all such
 * inodes. Otherwise write only ones which go sequentially
 * in reverse order.
 *
 * Return the number of pages and/or inodes written.
 */
static long writeback_sb_inodes(struct super_block *sb,
				struct bdi_writeback *wb,
				struct wb_writeback_work *work)
{
	struct writeback_control wbc = {
		.sync_mode		= work->sync_mode,
		.tagged_writepages	= work->tagged_writepages,
		.for_kupdate		= work->for_kupdate,
		.for_background		= work->for_background,
		.range_cyclic		= work->range_cyclic,
		.range_start		= 0,
		.range_end		= LLONG_MAX,
	};
	unsigned long start_time = jiffies;
	long write_chunk;
	long wrote = 0;  

	while (!list_empty(&wb->b_io)) {
		struct inode *inode = wb_inode(wb->b_io.prev);

		if (inode->i_sb != sb) {
			if (work->sb) {
				redirty_tail(inode, wb);
				continue;
			}

			break;
		}

		spin_lock(&inode->i_lock);
		if (inode->i_state & (I_NEW | I_FREEING | I_WILL_FREE)) {
			spin_unlock(&inode->i_lock);
			redirty_tail(inode, wb);
			continue;
		}
		__iget(inode);
		write_chunk = writeback_chunk_size(wb->bdi, work);
		wbc.nr_to_write = write_chunk;
		wbc.pages_skipped = 0;

		writeback_single_inode(inode, wb, &wbc);

		work->nr_pages -= write_chunk - wbc.nr_to_write;
		wrote += write_chunk - wbc.nr_to_write;
		if (!(inode->i_state & I_DIRTY))
			wrote++;
		if (wbc.pages_skipped) {
			redirty_tail(inode, wb);
		}
		spin_unlock(&inode->i_lock);
		spin_unlock(&wb->list_lock);
		iput(inode);
		cond_resched();
		spin_lock(&wb->list_lock);
		if (wrote) {
			if (time_is_before_jiffies(start_time + HZ / 10UL))
				break;
			if (work->nr_pages <= 0)
				break;
		}
	}
	return wrote;
}

static long __writeback_inodes_wb(struct bdi_writeback *wb,
				  struct wb_writeback_work *work)
{
	unsigned long start_time = jiffies;
	long wrote = 0;

	while (!list_empty(&wb->b_io)) {
		struct inode *inode = wb_inode(wb->b_io.prev);
		struct super_block *sb = inode->i_sb;

		if (!grab_super_passive(sb)) {
			redirty_tail(inode, wb);
			continue;
		}
		wrote += writeback_sb_inodes(sb, wb, work);
		drop_super(sb);

		
		if (wrote) {
			if (time_is_before_jiffies(start_time + HZ / 10UL))
				break;
			if (work->nr_pages <= 0)
				break;
		}
	}
	/* Leave any unwritten inodes on b_io */
	return wrote;
}

long writeback_inodes_wb(struct bdi_writeback *wb, long nr_pages,
				enum wb_reason reason)
{
	struct wb_writeback_work work = {
		.nr_pages	= nr_pages,
		.sync_mode	= WB_SYNC_NONE,
		.range_cyclic	= 1,
		.reason		= reason,
	};

	spin_lock(&wb->list_lock);
	if (list_empty(&wb->b_io))
		queue_io(wb, &work);
	__writeback_inodes_wb(wb, &work);
	spin_unlock(&wb->list_lock);

	return nr_pages - work.nr_pages;
}

static bool over_bground_thresh(struct backing_dev_info *bdi)
{
	unsigned long background_thresh, dirty_thresh;

	global_dirty_limits(&background_thresh, &dirty_thresh);

	if (global_page_state(NR_FILE_DIRTY) +
	    global_page_state(NR_UNSTABLE_NFS) > background_thresh)
		return true;

	if (bdi_stat(bdi, BDI_RECLAIMABLE) >
				bdi_dirty_limit(bdi, background_thresh))
		return true;

	return false;
}

static void wb_update_bandwidth(struct bdi_writeback *wb,
				unsigned long start_time)
{
	__bdi_update_bandwidth(wb->bdi, 0, 0, 0, 0, 0, start_time);
}

static long wb_writeback(struct bdi_writeback *wb,
			 struct wb_writeback_work *work)
{
	unsigned long wb_start = jiffies;
	long nr_pages = work->nr_pages;
	unsigned long oldest_jif;
	struct inode *inode;
	long progress;

	oldest_jif = jiffies;
	work->older_than_this = &oldest_jif;

	spin_lock(&wb->list_lock);
	for (;;) {
		if (work->nr_pages <= 0)
			break;

		if ((work->for_background || work->for_kupdate) &&
		    !list_empty(&wb->bdi->work_list))
			break;

		if (work->for_background && !over_bground_thresh(wb->bdi))
			break;

		if (work->for_kupdate) {
			oldest_jif = jiffies -
				msecs_to_jiffies(dirty_expire_interval * 10);
		} else if (work->for_background)
			oldest_jif = jiffies;

		trace_writeback_start(wb->bdi, work);
		if (list_empty(&wb->b_io))
			queue_io(wb, work);
		if (work->sb)
			progress = writeback_sb_inodes(work->sb, wb, work);
		else
			progress = __writeback_inodes_wb(wb, work);
		trace_writeback_written(wb->bdi, work);

		wb_update_bandwidth(wb, wb_start);

		if (progress)
			continue;
		if (list_empty(&wb->b_more_io))
			break;
		/*
		 * Nothing written. Wait for some inode to
		 * become available for writeback. Otherwise
		 * we'll just busyloop.
		 */
		if (!list_empty(&wb->b_more_io))  {
			trace_writeback_wait(wb->bdi, work);
			inode = wb_inode(wb->b_more_io.prev);
			spin_lock(&inode->i_lock);
			inode_wait_for_writeback(inode, wb);
			spin_unlock(&inode->i_lock);
		}
	}
	spin_unlock(&wb->list_lock);

	return nr_pages - work->nr_pages;
}

static struct wb_writeback_work *
get_next_work_item(struct backing_dev_info *bdi)
{
	struct wb_writeback_work *work = NULL;

	spin_lock_bh(&bdi->wb_lock);
	if (!list_empty(&bdi->work_list)) {
		work = list_entry(bdi->work_list.next,
				  struct wb_writeback_work, list);
		list_del_init(&work->list);
	}
	spin_unlock_bh(&bdi->wb_lock);
	return work;
}

static unsigned long get_nr_dirty_pages(void)
{
	return global_page_state(NR_FILE_DIRTY) +
		global_page_state(NR_UNSTABLE_NFS) +
		get_nr_dirty_inodes();
}

static long wb_check_background_flush(struct bdi_writeback *wb)
{
	if (over_bground_thresh(wb->bdi)) {

		struct wb_writeback_work work = {
			.nr_pages	= LONG_MAX,
			.sync_mode	= WB_SYNC_NONE,
			.for_background	= 1,
			.range_cyclic	= 1,
			.reason		= WB_REASON_BACKGROUND,
		};

		return wb_writeback(wb, &work);
	}

	return 0;
}

static long wb_check_old_data_flush(struct bdi_writeback *wb)
{
	unsigned long expired;
	long nr_pages;

	if (!dirty_writeback_interval)
		return 0;

	expired = wb->last_old_flush +
			msecs_to_jiffies(dirty_writeback_interval * 10);
	if (time_before(jiffies, expired))
		return 0;

	wb->last_old_flush = jiffies;
	nr_pages = get_nr_dirty_pages();

	if (nr_pages) {
		struct wb_writeback_work work = {
			.nr_pages	= nr_pages,
			.sync_mode	= WB_SYNC_NONE,
			.for_kupdate	= 1,
			.range_cyclic	= 1,
			.reason		= WB_REASON_PERIODIC,
		};

		return wb_writeback(wb, &work);
	}

	return 0;
}

long wb_do_writeback(struct bdi_writeback *wb, int force_wait)
{
	struct backing_dev_info *bdi = wb->bdi;
	struct wb_writeback_work *work;
	long wrote = 0;

	set_bit(BDI_writeback_running, &wb->bdi->state);
	while ((work = get_next_work_item(bdi)) != NULL) {
		if (force_wait)
			work->sync_mode = WB_SYNC_ALL;

		trace_writeback_exec(bdi, work);

		wrote += wb_writeback(wb, work);

		if (work->done)
			complete(work->done);
		else
			kfree(work);
	}

	wrote += wb_check_old_data_flush(wb);
	wrote += wb_check_background_flush(wb);
	clear_bit(BDI_writeback_running, &wb->bdi->state);

	return wrote;
}

int bdi_writeback_thread(void *data)
{
	struct bdi_writeback *wb = data;
	struct backing_dev_info *bdi = wb->bdi;
	long pages_written;

	current->flags |= PF_SWAPWRITE;
	set_freezable();
	wb->last_active = jiffies;

	set_user_nice(current, 0);

	trace_writeback_thread_start(bdi);

	while (!kthread_freezable_should_stop(NULL)) {
		del_timer(&wb->wakeup_timer);

		pages_written = wb_do_writeback(wb, 0);

		trace_writeback_pages_written(pages_written);

		if (pages_written)
			wb->last_active = jiffies;

		set_current_state(TASK_INTERRUPTIBLE);
		if (!list_empty(&bdi->work_list) || kthread_should_stop()) {
			__set_current_state(TASK_RUNNING);
			continue;
		}

		if (wb_has_dirty_io(wb) && dirty_writeback_interval)
			schedule_timeout(msecs_to_jiffies(dirty_writeback_interval * 10));
		else {
			schedule();
		}
	}

	
	if (!list_empty(&bdi->work_list))
		wb_do_writeback(wb, 1);

	trace_writeback_thread_stop(bdi);
	return 0;
}


void wakeup_flusher_threads(long nr_pages, enum wb_reason reason)
{
	struct backing_dev_info *bdi;

	if (!nr_pages) {
		nr_pages = global_page_state(NR_FILE_DIRTY) +
				global_page_state(NR_UNSTABLE_NFS);
	}

	rcu_read_lock();
	list_for_each_entry_rcu(bdi, &bdi_list, bdi_list) {
		if (!bdi_has_dirty_io(bdi))
			continue;
		__bdi_start_writeback(bdi, nr_pages, false, reason);
	}
	rcu_read_unlock();
}

static noinline void block_dump___mark_inode_dirty(struct inode *inode)
{
	if (inode->i_ino || strcmp(inode->i_sb->s_id, "bdev")) {
		struct dentry *dentry;
		const char *name = "?";

		dentry = d_find_alias(inode);
		if (dentry) {
			spin_lock(&dentry->d_lock);
			name = (const char *) dentry->d_name.name;
		}
		printk(KERN_DEBUG
		       "%s(%d): dirtied inode %lu (%s) on %s\n",
		       current->comm, task_pid_nr(current), inode->i_ino,
		       name, inode->i_sb->s_id);
		if (dentry) {
			spin_unlock(&dentry->d_lock);
			dput(dentry);
		}
	}
}

void __mark_inode_dirty(struct inode *inode, int flags)
{
	struct super_block *sb = inode->i_sb;
	struct backing_dev_info *bdi = NULL;

	if (flags & (I_DIRTY_SYNC | I_DIRTY_DATASYNC)) {
		if (sb->s_op->dirty_inode)
			sb->s_op->dirty_inode(inode, flags);
	}

	smp_mb();

	
	if ((inode->i_state & flags) == flags)
		return;

	if (unlikely(block_dump > 1))
		block_dump___mark_inode_dirty(inode);

	spin_lock(&inode->i_lock);
	if ((inode->i_state & flags) != flags) {
		const int was_dirty = inode->i_state & I_DIRTY;

		inode->i_state |= flags;

		if (inode->i_state & I_SYNC)
			goto out_unlock_inode;

		if (!S_ISBLK(inode->i_mode)) {
			if (inode_unhashed(inode))
				goto out_unlock_inode;
		}
		if (inode->i_state & I_FREEING)
			goto out_unlock_inode;

		if (!was_dirty) {
			bool wakeup_bdi = false;
			bdi = inode_to_bdi(inode);

			if (bdi_cap_writeback_dirty(bdi)) {
				WARN(!test_bit(BDI_registered, &bdi->state),
				     "bdi-%s not registered\n", bdi->name);

				if (!wb_has_dirty_io(&bdi->wb))
					wakeup_bdi = true;
			}

			spin_unlock(&inode->i_lock);
			spin_lock(&bdi->wb.list_lock);
			inode->dirtied_when = jiffies;
			list_move(&inode->i_wb_list, &bdi->wb.b_dirty);
			spin_unlock(&bdi->wb.list_lock);

			if (wakeup_bdi)
				bdi_wakeup_thread_delayed(bdi);
			return;
		}
	}
out_unlock_inode:
	spin_unlock(&inode->i_lock);

}
EXPORT_SYMBOL(__mark_inode_dirty);

static void wait_sb_inodes(struct super_block *sb)
{
	struct inode *inode, *old_inode = NULL;

	WARN_ON(!rwsem_is_locked(&sb->s_umount));

	spin_lock(&inode_sb_list_lock);

	list_for_each_entry(inode, &sb->s_inodes, i_sb_list) {
		struct address_space *mapping = inode->i_mapping;

		spin_lock(&inode->i_lock);
		if ((inode->i_state & (I_FREEING|I_WILL_FREE|I_NEW)) ||
		    (mapping->nrpages == 0)) {
			spin_unlock(&inode->i_lock);
			continue;
		}
		__iget(inode);
		spin_unlock(&inode->i_lock);
		spin_unlock(&inode_sb_list_lock);

		iput(old_inode);
		old_inode = inode;

		filemap_fdatawait(mapping);

		cond_resched();

		spin_lock(&inode_sb_list_lock);
	}
	spin_unlock(&inode_sb_list_lock);
	iput(old_inode);
}

/**
 * writeback_inodes_sb_nr -	writeback dirty inodes from given super_block
 * @sb: the superblock
 * @nr: the number of pages to write
 * @reason: reason why some writeback work initiated
 *
 * Start writeback on some inodes on this super_block. No guarantees are made
 * on how many (if any) will be written, and this function does not wait
 * for IO completion of submitted IO.
 */
void writeback_inodes_sb_nr(struct super_block *sb,
			    unsigned long nr,
			    enum wb_reason reason)
{
	DECLARE_COMPLETION_ONSTACK(done);
	struct wb_writeback_work work = {
		.sb			= sb,
		.sync_mode		= WB_SYNC_NONE,
		.tagged_writepages	= 1,
		.done			= &done,
		.nr_pages		= nr,
		.reason			= reason,
	};

	WARN_ON(!rwsem_is_locked(&sb->s_umount));
	bdi_queue_work(sb->s_bdi, &work);
	wait_for_completion(&done);
}
EXPORT_SYMBOL(writeback_inodes_sb_nr);

/**
 * writeback_inodes_sb	-	writeback dirty inodes from given super_block
 * @sb: the superblock
 * @reason: reason why some writeback work was initiated
 *
 * Start writeback on some inodes on this super_block. No guarantees are made
 * on how many (if any) will be written, and this function does not wait
 * for IO completion of submitted IO.
 */
void writeback_inodes_sb(struct super_block *sb, enum wb_reason reason)
{
	return writeback_inodes_sb_nr(sb, get_nr_dirty_pages(), reason);
}
EXPORT_SYMBOL(writeback_inodes_sb);

int writeback_inodes_sb_if_idle(struct super_block *sb, enum wb_reason reason)
{
	if (!writeback_in_progress(sb->s_bdi)) {
		down_read(&sb->s_umount);
		writeback_inodes_sb(sb, reason);
		up_read(&sb->s_umount);
		return 1;
	} else
		return 0;
}
EXPORT_SYMBOL(writeback_inodes_sb_if_idle);

int writeback_inodes_sb_nr_if_idle(struct super_block *sb,
				   unsigned long nr,
				   enum wb_reason reason)
{
	if (!writeback_in_progress(sb->s_bdi)) {
		down_read(&sb->s_umount);
		writeback_inodes_sb_nr(sb, nr, reason);
		up_read(&sb->s_umount);
		return 1;
	} else
		return 0;
}
EXPORT_SYMBOL(writeback_inodes_sb_nr_if_idle);

void sync_inodes_sb(struct super_block *sb)
{
	DECLARE_COMPLETION_ONSTACK(done);
	struct wb_writeback_work work = {
		.sb		= sb,
		.sync_mode	= WB_SYNC_ALL,
		.nr_pages	= LONG_MAX,
		.range_cyclic	= 0,
		.done		= &done,
		.reason		= WB_REASON_SYNC,
	};

	WARN_ON(!rwsem_is_locked(&sb->s_umount));

	bdi_queue_work(sb->s_bdi, &work);
	wait_for_completion(&done);

	wait_sb_inodes(sb);
}
EXPORT_SYMBOL(sync_inodes_sb);

int write_inode_now(struct inode *inode, int sync)
{
	struct bdi_writeback *wb = &inode_to_bdi(inode)->wb;
	int ret;
	struct writeback_control wbc = {
		.nr_to_write = LONG_MAX,
		.sync_mode = sync ? WB_SYNC_ALL : WB_SYNC_NONE,
		.range_start = 0,
		.range_end = LLONG_MAX,
	};

	if (!mapping_cap_writeback_dirty(inode->i_mapping))
		wbc.nr_to_write = 0;

	might_sleep();
	spin_lock(&wb->list_lock);
	spin_lock(&inode->i_lock);
	ret = writeback_single_inode(inode, wb, &wbc);
	spin_unlock(&inode->i_lock);
	spin_unlock(&wb->list_lock);
	return ret;
}
EXPORT_SYMBOL(write_inode_now);

int sync_inode(struct inode *inode, struct writeback_control *wbc)
{
	struct bdi_writeback *wb = &inode_to_bdi(inode)->wb;
	int ret;

	spin_lock(&wb->list_lock);
	spin_lock(&inode->i_lock);
	ret = writeback_single_inode(inode, wb, wbc);
	spin_unlock(&inode->i_lock);
	spin_unlock(&wb->list_lock);
	return ret;
}
EXPORT_SYMBOL(sync_inode);

int sync_inode_metadata(struct inode *inode, int wait)
{
	struct writeback_control wbc = {
		.sync_mode = wait ? WB_SYNC_ALL : WB_SYNC_NONE,
		.nr_to_write = 0, 
	};

	return sync_inode(inode, &wbc);
}
EXPORT_SYMBOL(sync_inode_metadata);
