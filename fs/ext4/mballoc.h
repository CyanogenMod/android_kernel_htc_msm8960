/*
 *  fs/ext4/mballoc.h
 *
 *  Written by: Alex Tomas <alex@clusterfs.com>
 *
 */
#ifndef _EXT4_MBALLOC_H
#define _EXT4_MBALLOC_H

#include <linux/time.h>
#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/quotaops.h>
#include <linux/buffer_head.h>
#include <linux/module.h>
#include <linux/swap.h>
#include <linux/proc_fs.h>
#include <linux/pagemap.h>
#include <linux/seq_file.h>
#include <linux/blkdev.h>
#include <linux/mutex.h>
#include "ext4_jbd2.h"
#include "ext4.h"

#define AGGRESSIVE_CHECK__

#define DOUBLE_CHECK__

#ifdef CONFIG_EXT4_DEBUG
extern u8 mb_enable_debug;

#define mb_debug(n, fmt, a...)	                                        \
	do {								\
		if ((n) <= mb_enable_debug) {		        	\
			printk(KERN_DEBUG "(%s, %d): %s: ",		\
			       __FILE__, __LINE__, __func__);		\
			printk(fmt, ## a);				\
		}							\
	} while (0)
#else
#define mb_debug(n, fmt, a...)
#endif

#define EXT4_MB_HISTORY_ALLOC		1	
#define EXT4_MB_HISTORY_PREALLOC	2	

#define MB_DEFAULT_MAX_TO_SCAN		200

#define MB_DEFAULT_MIN_TO_SCAN		10

#define MB_DEFAULT_MAX_GROUPS_TO_SCAN	5

#define MB_DEFAULT_STATS		0

#define MB_DEFAULT_STREAM_THRESHOLD	16	

#define MB_DEFAULT_ORDER2_REQS		2

#define MB_DEFAULT_GROUP_PREALLOC	512


struct ext4_free_data {
	
	struct ext4_journal_cb_entry	efd_jce;

	

	
	struct rb_node			efd_node;

	
	ext4_group_t			efd_group;

	
	ext4_grpblk_t			efd_start_cluster;
	ext4_grpblk_t			efd_count;

	
	tid_t				efd_tid;
};

struct ext4_prealloc_space {
	struct list_head	pa_inode_list;
	struct list_head	pa_group_list;
	union {
		struct list_head pa_tmp_list;
		struct rcu_head	pa_rcu;
	} u;
	spinlock_t		pa_lock;
	atomic_t		pa_count;
	unsigned		pa_deleted;
	ext4_fsblk_t		pa_pstart;	
	ext4_lblk_t		pa_lstart;	
	ext4_grpblk_t		pa_len;		
	ext4_grpblk_t		pa_free;	
	unsigned short		pa_type;	
	spinlock_t		*pa_obj_lock;
	struct inode		*pa_inode;	
};

enum {
	MB_INODE_PA = 0,
	MB_GROUP_PA = 1
};

struct ext4_free_extent {
	ext4_lblk_t fe_logical;
	ext4_grpblk_t fe_start;	
	ext4_group_t fe_group;
	ext4_grpblk_t fe_len;	
};

#define PREALLOC_TB_SIZE 10
struct ext4_locality_group {
	
	
	struct mutex		lg_mutex;
	
	struct list_head	lg_prealloc_list[PREALLOC_TB_SIZE];
	spinlock_t		lg_prealloc_lock;
};

struct ext4_allocation_context {
	struct inode *ac_inode;
	struct super_block *ac_sb;

	
	struct ext4_free_extent ac_o_ex;

	
	struct ext4_free_extent ac_g_ex;

	
	struct ext4_free_extent ac_b_ex;

	
	struct ext4_free_extent ac_f_ex;

	
	unsigned long ac_ex_scanned;
	__u16 ac_groups_scanned;
	__u16 ac_found;
	__u16 ac_tail;
	__u16 ac_buddy;
	__u16 ac_flags;		
	__u8 ac_status;
	__u8 ac_criteria;
	__u8 ac_2order;		
	__u8 ac_op;		
	struct page *ac_bitmap_page;
	struct page *ac_buddy_page;
	struct ext4_prealloc_space *ac_pa;
	struct ext4_locality_group *ac_lg;
};

#define AC_STATUS_CONTINUE	1
#define AC_STATUS_FOUND		2
#define AC_STATUS_BREAK		3

struct ext4_buddy {
	struct page *bd_buddy_page;
	void *bd_buddy;
	struct page *bd_bitmap_page;
	void *bd_bitmap;
	struct ext4_group_info *bd_info;
	struct super_block *bd_sb;
	__u16 bd_blkbits;
	ext4_group_t bd_group;
};

static inline ext4_fsblk_t ext4_grp_offs_to_block(struct super_block *sb,
					struct ext4_free_extent *fex)
{
	return ext4_group_first_block_no(sb, fex->fe_group) +
		(fex->fe_start << EXT4_SB(sb)->s_cluster_bits);
}
#endif

extern atomic_t vfs_emergency_remount;
