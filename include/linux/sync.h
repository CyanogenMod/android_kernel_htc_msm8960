/*
 * include/linux/sync.h
 *
 * Copyright (C) 2012 Google, Inc.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_SYNC_H
#define _LINUX_SYNC_H

#include <linux/types.h>
#ifdef __KERNEL__

#include <linux/kref.h>
#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/wait.h>

struct sync_timeline;
struct sync_pt;
struct sync_fence;

struct sync_timeline_ops {
	const char *driver_name;

	
	struct sync_pt *(*dup)(struct sync_pt *pt);

	
	int (*has_signaled)(struct sync_pt *pt);

	
	int (*compare)(struct sync_pt *a, struct sync_pt *b);

	
	void (*free_pt)(struct sync_pt *sync_pt);

	
	void (*release_obj)(struct sync_timeline *sync_timeline);

	
	void (*print_obj)(struct seq_file *s,
			  struct sync_timeline *sync_timeline);

	
	void (*print_pt)(struct seq_file *s, struct sync_pt *sync_pt);

	
	int (*fill_driver_data)(struct sync_pt *syncpt, void *data, int size);

	
	void (*timeline_value_str)(struct sync_timeline *timeline, char *str,
				   int size);

	
	void (*pt_value_str)(struct sync_pt *pt, char *str, int size);
};

struct sync_timeline {
	struct kref		kref;
	const struct sync_timeline_ops	*ops;
	char			name[32];

	
	bool			destroyed;

	struct list_head	child_list_head;
	spinlock_t		child_list_lock;

	struct list_head	active_list_head;
	spinlock_t		active_list_lock;

	struct list_head	sync_timeline_list;
};

struct sync_pt {
	struct sync_timeline		*parent;
	struct list_head	child_list;

	struct list_head	active_list;
	struct list_head	signaled_list;

	struct sync_fence	*fence;
	struct list_head	pt_list;

	
	int			status;

	ktime_t			timestamp;
};

struct sync_fence {
	struct file		*file;
	struct kref		kref;
	char			name[32];

	
	struct list_head	pt_list_head;

	struct list_head	waiter_list_head;
	spinlock_t		waiter_list_lock; 
	int			status;

	wait_queue_head_t	wq;

	struct list_head	sync_fence_list;
};

struct sync_fence_waiter;
typedef void (*sync_callback_t)(struct sync_fence *fence,
				struct sync_fence_waiter *waiter);

struct sync_fence_waiter {
	struct list_head	waiter_list;

	sync_callback_t		callback;
};

static inline void sync_fence_waiter_init(struct sync_fence_waiter *waiter,
					  sync_callback_t callback)
{
	waiter->callback = callback;
}


struct sync_timeline *sync_timeline_create(const struct sync_timeline_ops *ops,
					   int size, const char *name);

void sync_timeline_destroy(struct sync_timeline *obj);

void sync_timeline_signal(struct sync_timeline *obj);

struct sync_pt *sync_pt_create(struct sync_timeline *parent, int size);

void sync_pt_free(struct sync_pt *pt);

struct sync_fence *sync_fence_create(const char *name, struct sync_pt *pt);


struct sync_fence *sync_fence_merge(const char *name,
				    struct sync_fence *a, struct sync_fence *b);

struct sync_fence *sync_fence_fdget(int fd);

void sync_fence_put(struct sync_fence *fence);

void sync_fence_install(struct sync_fence *fence, int fd);

int sync_fence_wait_async(struct sync_fence *fence,
			  struct sync_fence_waiter *waiter);

int sync_fence_cancel_async(struct sync_fence *fence,
			    struct sync_fence_waiter *waiter);

int sync_fence_wait(struct sync_fence *fence, long timeout);

#endif 

struct sync_merge_data {
	__s32	fd2; 
	char	name[32]; 
	__s32	fence; 
};

struct sync_pt_info {
	__u32	len;
	char	obj_name[32];
	char	driver_name[32];
	__s32	status;
	__u64	timestamp_ns;

	__u8	driver_data[0];
};

struct sync_fence_info_data {
	__u32	len;
	char	name[32];
	__s32	status;

	__u8	pt_info[0];
};

#define SYNC_IOC_MAGIC		'>'

#define SYNC_IOC_WAIT		_IOW(SYNC_IOC_MAGIC, 0, __s32)

#define SYNC_IOC_MERGE		_IOWR(SYNC_IOC_MAGIC, 1, struct sync_merge_data)

#define SYNC_IOC_FENCE_INFO	_IOWR(SYNC_IOC_MAGIC, 2,\
	struct sync_fence_info_data)

#endif 
