/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _LINUX_QUOTA_
#define _LINUX_QUOTA_

#include <linux/errno.h>
#include <linux/types.h>

#define __DQUOT_VERSION__	"dquot_6.5.2"

#define MAXQUOTAS 2
#define USRQUOTA  0		
#define GRPQUOTA  1		

#define INITQFNAMES { \
	"user",     \
	"group",    \
	"undefined", \
};

#define SUBCMDMASK  0x00ff
#define SUBCMDSHIFT 8
#define QCMD(cmd, type)  (((cmd) << SUBCMDSHIFT) | ((type) & SUBCMDMASK))

#define Q_SYNC     0x800001	
#define Q_QUOTAON  0x800002	
#define Q_QUOTAOFF 0x800003	
#define Q_GETFMT   0x800004	
#define Q_GETINFO  0x800005	
#define Q_SETINFO  0x800006	
#define Q_GETQUOTA 0x800007	
#define Q_SETQUOTA 0x800008	

#define	QFMT_VFS_OLD 1
#define	QFMT_VFS_V0 2
#define QFMT_OCFS2 3
#define	QFMT_VFS_V1 4

#define QIF_DQBLKSIZE_BITS 10
#define QIF_DQBLKSIZE (1 << QIF_DQBLKSIZE_BITS)

enum {
	QIF_BLIMITS_B = 0,
	QIF_SPACE_B,
	QIF_ILIMITS_B,
	QIF_INODES_B,
	QIF_BTIME_B,
	QIF_ITIME_B,
};

#define QIF_BLIMITS	(1 << QIF_BLIMITS_B)
#define QIF_SPACE	(1 << QIF_SPACE_B)
#define QIF_ILIMITS	(1 << QIF_ILIMITS_B)
#define QIF_INODES	(1 << QIF_INODES_B)
#define QIF_BTIME	(1 << QIF_BTIME_B)
#define QIF_ITIME	(1 << QIF_ITIME_B)
#define QIF_LIMITS	(QIF_BLIMITS | QIF_ILIMITS)
#define QIF_USAGE	(QIF_SPACE | QIF_INODES)
#define QIF_TIMES	(QIF_BTIME | QIF_ITIME)
#define QIF_ALL		(QIF_LIMITS | QIF_USAGE | QIF_TIMES)

struct if_dqblk {
	__u64 dqb_bhardlimit;
	__u64 dqb_bsoftlimit;
	__u64 dqb_curspace;
	__u64 dqb_ihardlimit;
	__u64 dqb_isoftlimit;
	__u64 dqb_curinodes;
	__u64 dqb_btime;
	__u64 dqb_itime;
	__u32 dqb_valid;
};

#define IIF_BGRACE	1
#define IIF_IGRACE	2
#define IIF_FLAGS	4
#define IIF_ALL		(IIF_BGRACE | IIF_IGRACE | IIF_FLAGS)

struct if_dqinfo {
	__u64 dqi_bgrace;
	__u64 dqi_igrace;
	__u32 dqi_flags;
	__u32 dqi_valid;
};

#define QUOTA_NL_NOWARN 0
#define QUOTA_NL_IHARDWARN 1		
#define QUOTA_NL_ISOFTLONGWARN 2 	
#define QUOTA_NL_ISOFTWARN 3		
#define QUOTA_NL_BHARDWARN 4		
#define QUOTA_NL_BSOFTLONGWARN 5	
#define QUOTA_NL_BSOFTWARN 6		
#define QUOTA_NL_IHARDBELOW 7		
#define QUOTA_NL_ISOFTBELOW 8		
#define QUOTA_NL_BHARDBELOW 9		
#define QUOTA_NL_BSOFTBELOW 10		

enum {
	QUOTA_NL_C_UNSPEC,
	QUOTA_NL_C_WARNING,
	__QUOTA_NL_C_MAX,
};
#define QUOTA_NL_C_MAX (__QUOTA_NL_C_MAX - 1)

enum {
	QUOTA_NL_A_UNSPEC,
	QUOTA_NL_A_QTYPE,
	QUOTA_NL_A_EXCESS_ID,
	QUOTA_NL_A_WARNING,
	QUOTA_NL_A_DEV_MAJOR,
	QUOTA_NL_A_DEV_MINOR,
	QUOTA_NL_A_CAUSED_ID,
	__QUOTA_NL_A_MAX,
};
#define QUOTA_NL_A_MAX (__QUOTA_NL_A_MAX - 1)


#ifdef __KERNEL__
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/rwsem.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/percpu_counter.h>

#include <linux/dqblk_xfs.h>
#include <linux/dqblk_v1.h>
#include <linux/dqblk_v2.h>

#include <linux/atomic.h>

typedef __kernel_uid32_t qid_t; 
typedef long long qsize_t;	

extern spinlock_t dq_data_lock;

#define DQUOT_INIT_ALLOC max(V1_INIT_ALLOC, V2_INIT_ALLOC)
#define DQUOT_INIT_REWRITE max(V1_INIT_REWRITE, V2_INIT_REWRITE)
#define DQUOT_DEL_ALLOC max(V1_DEL_ALLOC, V2_DEL_ALLOC)
#define DQUOT_DEL_REWRITE max(V1_DEL_REWRITE, V2_DEL_REWRITE)

struct mem_dqblk {
	qsize_t dqb_bhardlimit;	
	qsize_t dqb_bsoftlimit;	
	qsize_t dqb_curspace;	
	qsize_t dqb_rsvspace;   
	qsize_t dqb_ihardlimit;	
	qsize_t dqb_isoftlimit;	
	qsize_t dqb_curinodes;	
	time_t dqb_btime;	
	time_t dqb_itime;	
};

struct quota_format_type;

struct mem_dqinfo {
	struct quota_format_type *dqi_format;
	int dqi_fmt_id;		
	struct list_head dqi_dirty_list;	
	unsigned long dqi_flags;
	unsigned int dqi_bgrace;
	unsigned int dqi_igrace;
	qsize_t dqi_maxblimit;
	qsize_t dqi_maxilimit;
	void *dqi_priv;
};

struct super_block;

#define DQF_MASK 0xffff		
#define DQF_GETINFO_MASK 0x1ffff	
#define DQF_SETINFO_MASK 0xffff		
#define DQF_SYS_FILE_B		16
#define DQF_SYS_FILE (1 << DQF_SYS_FILE_B)	
#define DQF_INFO_DIRTY_B	31
#define DQF_INFO_DIRTY (1 << DQF_INFO_DIRTY_B)	

extern void mark_info_dirty(struct super_block *sb, int type);
static inline int info_dirty(struct mem_dqinfo *info)
{
	return test_bit(DQF_INFO_DIRTY_B, &info->dqi_flags);
}

enum {
	DQST_LOOKUPS,
	DQST_DROPS,
	DQST_READS,
	DQST_WRITES,
	DQST_CACHE_HITS,
	DQST_ALLOC_DQUOTS,
	DQST_FREE_DQUOTS,
	DQST_SYNCS,
	_DQST_DQSTAT_LAST
};

struct dqstats {
	int stat[_DQST_DQSTAT_LAST];
	struct percpu_counter counter[_DQST_DQSTAT_LAST];
};

extern struct dqstats *dqstats_pcpu;
extern struct dqstats dqstats;

static inline void dqstats_inc(unsigned int type)
{
	percpu_counter_inc(&dqstats.counter[type]);
}

static inline void dqstats_dec(unsigned int type)
{
	percpu_counter_dec(&dqstats.counter[type]);
}

#define DQ_MOD_B	0	
#define DQ_BLKS_B	1	
#define DQ_INODES_B	2	
#define DQ_FAKE_B	3	
#define DQ_READ_B	4	
#define DQ_ACTIVE_B	5	
#define DQ_LASTSET_B	6	

struct dquot {
	struct hlist_node dq_hash;	
	struct list_head dq_inuse;	
	struct list_head dq_free;	
	struct list_head dq_dirty;	
	struct mutex dq_lock;		
	atomic_t dq_count;		
	wait_queue_head_t dq_wait_unused;	
	struct super_block *dq_sb;	
	unsigned int dq_id;		
	loff_t dq_off;			
	unsigned long dq_flags;		
	short dq_type;			
	struct mem_dqblk dq_dqb;	
};

struct quota_format_ops {
	int (*check_quota_file)(struct super_block *sb, int type);	
	int (*read_file_info)(struct super_block *sb, int type);	
	int (*write_file_info)(struct super_block *sb, int type);	
	int (*free_file_info)(struct super_block *sb, int type);	
	int (*read_dqblk)(struct dquot *dquot);		
	int (*commit_dqblk)(struct dquot *dquot);	
	int (*release_dqblk)(struct dquot *dquot);	
};

struct dquot_operations {
	int (*write_dquot) (struct dquot *);		
	struct dquot *(*alloc_dquot)(struct super_block *, int);	
	void (*destroy_dquot)(struct dquot *);		
	int (*acquire_dquot) (struct dquot *);		
	int (*release_dquot) (struct dquot *);		
	int (*mark_dirty) (struct dquot *);		
	int (*write_info) (struct super_block *, int);	
	qsize_t *(*get_reserved_space) (struct inode *);
};

struct path;

struct quotactl_ops {
	int (*quota_on)(struct super_block *, int, int, struct path *);
	int (*quota_on_meta)(struct super_block *, int, int);
	int (*quota_off)(struct super_block *, int);
	int (*quota_sync)(struct super_block *, int, int);
	int (*get_info)(struct super_block *, int, struct if_dqinfo *);
	int (*set_info)(struct super_block *, int, struct if_dqinfo *);
	int (*get_dqblk)(struct super_block *, int, qid_t, struct fs_disk_quota *);
	int (*set_dqblk)(struct super_block *, int, qid_t, struct fs_disk_quota *);
	int (*get_xstate)(struct super_block *, struct fs_quota_stat *);
	int (*set_xstate)(struct super_block *, unsigned int, int);
};

struct quota_format_type {
	int qf_fmt_id;	
	const struct quota_format_ops *qf_ops;	
	struct module *qf_owner;		
	struct quota_format_type *qf_next;
};

enum {
	_DQUOT_USAGE_ENABLED = 0,		
	_DQUOT_LIMITS_ENABLED,			
	_DQUOT_SUSPENDED,			
	_DQUOT_STATE_FLAGS
};
#define DQUOT_USAGE_ENABLED	(1 << _DQUOT_USAGE_ENABLED)
#define DQUOT_LIMITS_ENABLED	(1 << _DQUOT_LIMITS_ENABLED)
#define DQUOT_SUSPENDED		(1 << _DQUOT_SUSPENDED)
#define DQUOT_STATE_FLAGS	(DQUOT_USAGE_ENABLED | DQUOT_LIMITS_ENABLED | \
				 DQUOT_SUSPENDED)
#define DQUOT_STATE_LAST	(_DQUOT_STATE_FLAGS * MAXQUOTAS)
#define DQUOT_QUOTA_SYS_FILE	(1 << DQUOT_STATE_LAST)
#define DQUOT_NEGATIVE_USAGE	(1 << (DQUOT_STATE_LAST + 1))
					       

static inline unsigned int dquot_state_flag(unsigned int flags, int type)
{
	return flags << _DQUOT_STATE_FLAGS * type;
}

static inline unsigned int dquot_generic_flag(unsigned int flags, int type)
{
	return (flags >> _DQUOT_STATE_FLAGS * type) & DQUOT_STATE_FLAGS;
}

#ifdef CONFIG_QUOTA_NETLINK_INTERFACE
extern void quota_send_warning(short type, unsigned int id, dev_t dev,
			       const char warntype);
#else
static inline void quota_send_warning(short type, unsigned int id, dev_t dev,
				      const char warntype)
{
	return;
}
#endif 

struct quota_info {
	unsigned int flags;			
	struct mutex dqio_mutex;		
	struct mutex dqonoff_mutex;		
	struct rw_semaphore dqptr_sem;		
	struct inode *files[MAXQUOTAS];		
	struct mem_dqinfo info[MAXQUOTAS];	
	const struct quota_format_ops *ops[MAXQUOTAS];	
};

int register_quota_format(struct quota_format_type *fmt);
void unregister_quota_format(struct quota_format_type *fmt);

struct quota_module_name {
	int qm_fmt_id;
	char *qm_mod_name;
};

#define INIT_QUOTA_MODULE_NAMES {\
	{QFMT_VFS_OLD, "quota_v1"},\
	{QFMT_VFS_V0, "quota_v2"},\
	{0, NULL}}

#endif 
#endif 
