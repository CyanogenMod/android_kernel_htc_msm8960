#ifndef IOCONTEXT_H
#define IOCONTEXT_H

#include <linux/radix-tree.h>
#include <linux/rcupdate.h>
#include <linux/workqueue.h>

enum {
	ICQ_IOPRIO_CHANGED	= 1 << 0,
	ICQ_CGROUP_CHANGED	= 1 << 1,
	ICQ_EXITED		= 1 << 2,

	ICQ_CHANGED_MASK	= ICQ_IOPRIO_CHANGED | ICQ_CGROUP_CHANGED,
};

struct io_cq {
	struct request_queue	*q;
	struct io_context	*ioc;

	union {
		struct list_head	q_node;
		struct kmem_cache	*__rcu_icq_cache;
	};
	union {
		struct hlist_node	ioc_node;
		struct rcu_head		__rcu_head;
	};

	unsigned int		flags;
};

struct io_context {
	atomic_long_t refcount;
	atomic_t nr_tasks;

	
	spinlock_t lock;

	unsigned short ioprio;

	int nr_batch_requests;     
	unsigned long last_waited; 

	struct radix_tree_root	icq_tree;
	struct io_cq __rcu	*icq_hint;
	struct hlist_head	icq_list;

	struct work_struct release_work;
};

static inline struct io_context *ioc_task_link(struct io_context *ioc)
{
	if (ioc && atomic_long_inc_not_zero(&ioc->refcount)) {
		atomic_inc(&ioc->nr_tasks);
		return ioc;
	}

	return NULL;
}

struct task_struct;
#ifdef CONFIG_BLOCK
void put_io_context(struct io_context *ioc);
void exit_io_context(struct task_struct *task);
struct io_context *get_task_io_context(struct task_struct *task,
				       gfp_t gfp_flags, int node);
void ioc_ioprio_changed(struct io_context *ioc, int ioprio);
void ioc_cgroup_changed(struct io_context *ioc);
unsigned int icq_get_changed(struct io_cq *icq);
#else
struct io_context;
static inline void put_io_context(struct io_context *ioc) { }
static inline void exit_io_context(struct task_struct *task) { }
#endif

#endif
