/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2008  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

#ifndef _FS_FUSE_I_H
#define _FS_FUSE_I_H

#include <linux/fuse.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/backing-dev.h>
#include <linux/mutex.h>
#include <linux/rwsem.h>
#include <linux/rbtree.h>
#include <linux/poll.h>
#include <linux/workqueue.h>

#define FUSE_MAX_PAGES_PER_REQ 32

#define FUSE_NOWRITE INT_MIN

#define FUSE_NAME_MAX 1024

#define FUSE_CTL_NUM_DENTRIES 5

#define FUSE_DEFAULT_PERMISSIONS (1 << 0)

#define FUSE_ALLOW_OTHER         (1 << 1)

extern struct list_head fuse_conn_list;

extern struct mutex fuse_mutex;

extern unsigned max_user_bgreq;
extern unsigned max_user_congthresh;

struct fuse_forget_link {
	struct fuse_forget_one forget_one;
	struct fuse_forget_link *next;
};

struct fuse_inode {
	
	struct inode inode;

	u64 nodeid;

	
	u64 nlookup;

	
	struct fuse_forget_link *forget;

	
	u64 i_time;

	umode_t orig_i_mode;

	
	u64 orig_ino;

	
	u64 attr_version;

	
	struct list_head write_files;

	
	struct list_head queued_writes;

	int writectr;

	
	wait_queue_head_t page_waitq;

	
	struct list_head writepages;
};

struct fuse_conn;

struct fuse_file {
	
	struct fuse_conn *fc;

	
	struct fuse_req *reserved_req;

	
	u64 kh;

	
	u64 fh;

	
	u64 nodeid;

	
	atomic_t count;

	
	u32 open_flags;

	
	struct list_head write_entry;

	
	struct rb_node polled_node;

	
	wait_queue_head_t poll_wait;

	
	bool flock:1;
};

struct fuse_in_arg {
	unsigned size;
	const void *value;
};

struct fuse_in {
	
	struct fuse_in_header h;

	
	unsigned argpages:1;

	
	unsigned numargs;

	
	struct fuse_in_arg args[3];
};

struct fuse_arg {
	unsigned size;
	void *value;
};

struct fuse_out {
	
	struct fuse_out_header h;


	unsigned argvar:1;

	
	unsigned argpages:1;

	
	unsigned page_zeroing:1;

	
	unsigned page_replace:1;

	
	unsigned numargs;

	
	struct fuse_arg args[3];
};

enum fuse_req_state {
	FUSE_REQ_INIT = 0,
	FUSE_REQ_PENDING,
	FUSE_REQ_READING,
	FUSE_REQ_SENT,
	FUSE_REQ_WRITING,
	FUSE_REQ_FINISHED
};

struct fuse_req {
	struct list_head list;

	
	struct list_head intr_entry;

	
	atomic_t count;

	
	u64 intr_unique;


	
	unsigned isreply:1;

	
	unsigned force:1;

	
	unsigned aborted:1;

	
	unsigned background:1;

	
	unsigned interrupted:1;

	
	unsigned locked:1;

	
	unsigned waiting:1;

	
	enum fuse_req_state state;

	
	struct fuse_in in;

	
	struct fuse_out out;

	
	wait_queue_head_t waitq;

	
	union {
		struct {
			union {
				struct fuse_release_in in;
				struct work_struct work;
			};
			struct path path;
		} release;
		struct fuse_init_in init_in;
		struct fuse_init_out init_out;
		struct cuse_init_in cuse_init_in;
		struct {
			struct fuse_read_in in;
			u64 attr_ver;
		} read;
		struct {
			struct fuse_write_in in;
			struct fuse_write_out out;
		} write;
		struct fuse_notify_retrieve_in retrieve_in;
		struct fuse_lk_in lk_in;
	} misc;

	
	struct page *pages[FUSE_MAX_PAGES_PER_REQ];

	
	unsigned num_pages;

	
	unsigned page_offset;

	
	struct fuse_file *ff;

	
	struct inode *inode;

	
	struct list_head writepages_entry;

	
	void (*end)(struct fuse_conn *, struct fuse_req *);

	
	struct file *stolen_file;
};

struct fuse_conn {
	
	spinlock_t lock;

	
	struct mutex inst_mutex;

	
	atomic_t count;

	
	uid_t user_id;

	
	gid_t group_id;

	
	unsigned flags;

	
	unsigned max_read;

	
	unsigned max_write;

	
	wait_queue_head_t waitq;

	
	struct list_head pending;

	
	struct list_head processing;

	
	struct list_head io;

	
	u64 khctr;

	
	struct rb_root polled_files;

	
	unsigned max_background;

	
	unsigned congestion_threshold;

	
	unsigned num_background;

	
	unsigned active_background;

	
	struct list_head bg_queue;

	
	struct list_head interrupts;

	
	struct fuse_forget_link forget_list_head;
	struct fuse_forget_link *forget_list_tail;

	
	int forget_batch;

	int blocked;

	
	wait_queue_head_t blocked_waitq;

	
	wait_queue_head_t reserved_req_waitq;

	
	u64 reqctr;

	unsigned connected;

	unsigned conn_error:1;

	
	unsigned conn_init:1;

	
	unsigned async_read:1;

	
	unsigned atomic_o_trunc:1;

	
	unsigned export_support:1;

	
	unsigned bdi_initialized:1;


	
	unsigned no_fsync:1;

	
	unsigned no_fsyncdir:1;

	
	unsigned no_flush:1;

	
	unsigned no_setxattr:1;

	
	unsigned no_getxattr:1;

	
	unsigned no_listxattr:1;

	
	unsigned no_removexattr:1;

	
	unsigned no_lock:1;

	
	unsigned no_access:1;

	
	unsigned no_create:1;

	
	unsigned no_interrupt:1;

	
	unsigned no_bmap:1;

	
	unsigned no_poll:1;

	
	unsigned big_writes:1;

	
	unsigned dont_mask:1;

	
	unsigned no_flock:1;

	
	atomic_t num_waiting;

	
	unsigned minor;

	
	struct backing_dev_info bdi;

	
	struct list_head entry;

	
	dev_t dev;

	
	struct dentry *ctl_dentry[FUSE_CTL_NUM_DENTRIES];

	
	int ctl_ndents;

	
	struct fasync_struct *fasync;

	
	u32 scramble_key[4];

	
	struct fuse_req *destroy_req;

	
	u64 attr_version;

	
	void (*release)(struct fuse_conn *);

	
	struct super_block *sb;

	
	struct rw_semaphore killsb;
};

static inline struct fuse_conn *get_fuse_conn_super(struct super_block *sb)
{
	return sb->s_fs_info;
}

static inline struct fuse_conn *get_fuse_conn(struct inode *inode)
{
	return get_fuse_conn_super(inode->i_sb);
}

static inline struct fuse_inode *get_fuse_inode(struct inode *inode)
{
	return container_of(inode, struct fuse_inode, inode);
}

static inline u64 get_node_id(struct inode *inode)
{
	return get_fuse_inode(inode)->nodeid;
}

extern const struct file_operations fuse_dev_operations;

extern const struct dentry_operations fuse_dentry_operations;

int fuse_inode_eq(struct inode *inode, void *_nodeidp);

struct inode *fuse_iget(struct super_block *sb, u64 nodeid,
			int generation, struct fuse_attr *attr,
			u64 attr_valid, u64 attr_version);

int fuse_lookup_name(struct super_block *sb, u64 nodeid, struct qstr *name,
		     struct fuse_entry_out *outarg, struct inode **inode);

void fuse_queue_forget(struct fuse_conn *fc, struct fuse_forget_link *forget,
		       u64 nodeid, u64 nlookup);

struct fuse_forget_link *fuse_alloc_forget(void);

void fuse_read_fill(struct fuse_req *req, struct file *file,
		    loff_t pos, size_t count, int opcode);

int fuse_open_common(struct inode *inode, struct file *file, bool isdir);

struct fuse_file *fuse_file_alloc(struct fuse_conn *fc);
struct fuse_file *fuse_file_get(struct fuse_file *ff);
void fuse_file_free(struct fuse_file *ff);
void fuse_finish_open(struct inode *inode, struct file *file);

void fuse_sync_release(struct fuse_file *ff, int flags);

void fuse_release_common(struct file *file, int opcode);

int fuse_fsync_common(struct file *file, loff_t start, loff_t end,
		      int datasync, int isdir);

int fuse_notify_poll_wakeup(struct fuse_conn *fc,
			    struct fuse_notify_poll_wakeup_out *outarg);

void fuse_init_file_inode(struct inode *inode);

void fuse_init_common(struct inode *inode);

void fuse_init_dir(struct inode *inode);

void fuse_init_symlink(struct inode *inode);

void fuse_change_attributes(struct inode *inode, struct fuse_attr *attr,
			    u64 attr_valid, u64 attr_version);

void fuse_change_attributes_common(struct inode *inode, struct fuse_attr *attr,
				   u64 attr_valid);

int fuse_dev_init(void);

void fuse_dev_cleanup(void);

int fuse_ctl_init(void);
void fuse_ctl_cleanup(void);

struct fuse_req *fuse_request_alloc(void);

struct fuse_req *fuse_request_alloc_nofs(void);

void fuse_request_free(struct fuse_req *req);

struct fuse_req *fuse_get_req(struct fuse_conn *fc);

struct fuse_req *fuse_get_req_nofail(struct fuse_conn *fc, struct file *file);

void fuse_put_request(struct fuse_conn *fc, struct fuse_req *req);

void fuse_request_send(struct fuse_conn *fc, struct fuse_req *req);

void fuse_request_send_background(struct fuse_conn *fc, struct fuse_req *req);

void fuse_request_send_background_locked(struct fuse_conn *fc,
					 struct fuse_req *req);

void fuse_abort_conn(struct fuse_conn *fc);

void fuse_invalidate_attr(struct inode *inode);

void fuse_invalidate_entry_cache(struct dentry *entry);

struct fuse_conn *fuse_conn_get(struct fuse_conn *fc);

void fuse_conn_kill(struct fuse_conn *fc);

void fuse_conn_init(struct fuse_conn *fc);

void fuse_conn_put(struct fuse_conn *fc);

int fuse_ctl_add_conn(struct fuse_conn *fc);

void fuse_ctl_remove_conn(struct fuse_conn *fc);

int fuse_valid_type(int m);

int fuse_allow_task(struct fuse_conn *fc, struct task_struct *task);

u64 fuse_lock_owner_id(struct fuse_conn *fc, fl_owner_t id);

int fuse_update_attributes(struct inode *inode, struct kstat *stat,
			   struct file *file, bool *refreshed);

void fuse_flush_writepages(struct inode *inode);

void fuse_set_nowrite(struct inode *inode);
void fuse_release_nowrite(struct inode *inode);

u64 fuse_get_attr_version(struct fuse_conn *fc);

int fuse_reverse_inval_inode(struct super_block *sb, u64 nodeid,
			     loff_t offset, loff_t len);

int fuse_reverse_inval_entry(struct super_block *sb, u64 parent_nodeid,
			     u64 child_nodeid, struct qstr *name);

int fuse_do_open(struct fuse_conn *fc, u64 nodeid, struct file *file,
		 bool isdir);
ssize_t fuse_direct_io(struct file *file, const char __user *buf,
		       size_t count, loff_t *ppos, int write);
long fuse_do_ioctl(struct file *file, unsigned int cmd, unsigned long arg,
		   unsigned int flags);
long fuse_ioctl_common(struct file *file, unsigned int cmd,
		       unsigned long arg, unsigned int flags);
unsigned fuse_file_poll(struct file *file, poll_table *wait);
int fuse_dev_release(struct inode *inode, struct file *file);

void fuse_write_update_size(struct inode *inode, loff_t pos);

#endif 
