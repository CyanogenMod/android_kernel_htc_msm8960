/*
 * linux/include/linux/sunrpc/sched.h
 *
 * Scheduling primitives for kernel Sun RPC.
 *
 * Copyright (C) 1996, Olaf Kirch <okir@monad.swb.de>
 */

#ifndef _LINUX_SUNRPC_SCHED_H_
#define _LINUX_SUNRPC_SCHED_H_

#include <linux/timer.h>
#include <linux/ktime.h>
#include <linux/sunrpc/types.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/sunrpc/xdr.h>

struct rpc_procinfo;
struct rpc_message {
	struct rpc_procinfo *	rpc_proc;	
	void *			rpc_argp;	
	void *			rpc_resp;	
	struct rpc_cred *	rpc_cred;	
};

struct rpc_call_ops;
struct rpc_wait_queue;
struct rpc_wait {
	struct list_head	list;		
	struct list_head	links;		
	struct list_head	timer_list;	
	unsigned long		expires;
};

struct rpc_task {
	atomic_t		tk_count;	
	struct list_head	tk_task;	
	struct rpc_clnt *	tk_client;	
	struct rpc_rqst *	tk_rqstp;	

	struct rpc_message	tk_msg;		

	void			(*tk_callback)(struct rpc_task *);
	void			(*tk_action)(struct rpc_task *);
	const struct rpc_call_ops *tk_ops;
	void *			tk_calldata;

	unsigned long		tk_timeout;	
	unsigned long		tk_runstate;	
	struct workqueue_struct	*tk_workqueue;	
	struct rpc_wait_queue 	*tk_waitqueue;	
	union {
		struct work_struct	tk_work;	
		struct rpc_wait		tk_wait;	
	} u;

	ktime_t			tk_start;	

	pid_t			tk_owner;	
	int			tk_status;	
	unsigned short		tk_flags;	
	unsigned short		tk_timeouts;	

#ifdef RPC_DEBUG
	unsigned short		tk_pid;		
#endif
	unsigned char		tk_priority : 2,
				tk_garb_retry : 2,
				tk_cred_retry : 2,
				tk_rebind_retry : 2;
};
#define tk_xprt			tk_client->cl_xprt

#define	task_for_each(task, pos, head) \
	list_for_each(pos, head) \
		if ((task=list_entry(pos, struct rpc_task, u.tk_wait.list)),1)

#define	task_for_first(task, head) \
	if (!list_empty(head) &&  \
	    ((task=list_entry((head)->next, struct rpc_task, u.tk_wait.list)),1))

typedef void			(*rpc_action)(struct rpc_task *);

struct rpc_call_ops {
	void (*rpc_call_prepare)(struct rpc_task *, void *);
	void (*rpc_call_done)(struct rpc_task *, void *);
	void (*rpc_count_stats)(struct rpc_task *, void *);
	void (*rpc_release)(void *);
};

struct rpc_task_setup {
	struct rpc_task *task;
	struct rpc_clnt *rpc_client;
	const struct rpc_message *rpc_message;
	const struct rpc_call_ops *callback_ops;
	void *callback_data;
	struct workqueue_struct *workqueue;
	unsigned short flags;
	signed char priority;
};

#define RPC_TASK_ASYNC		0x0001		
#define RPC_TASK_SWAPPER	0x0002		
#define RPC_CALL_MAJORSEEN	0x0020		
#define RPC_TASK_ROOTCREDS	0x0040		
#define RPC_TASK_DYNAMIC	0x0080		
#define RPC_TASK_KILLED		0x0100		
#define RPC_TASK_SOFT		0x0200		
#define RPC_TASK_SOFTCONN	0x0400		
#define RPC_TASK_SENT		0x0800		
#define RPC_TASK_TIMEOUT	0x1000		

#define RPC_IS_ASYNC(t)		((t)->tk_flags & RPC_TASK_ASYNC)
#define RPC_IS_SWAPPER(t)	((t)->tk_flags & RPC_TASK_SWAPPER)
#define RPC_DO_ROOTOVERRIDE(t)	((t)->tk_flags & RPC_TASK_ROOTCREDS)
#define RPC_ASSASSINATED(t)	((t)->tk_flags & RPC_TASK_KILLED)
#define RPC_IS_SOFT(t)		((t)->tk_flags & (RPC_TASK_SOFT|RPC_TASK_TIMEOUT))
#define RPC_IS_SOFTCONN(t)	((t)->tk_flags & RPC_TASK_SOFTCONN)
#define RPC_WAS_SENT(t)		((t)->tk_flags & RPC_TASK_SENT)

#define RPC_TASK_RUNNING	0
#define RPC_TASK_QUEUED		1
#define RPC_TASK_ACTIVE		2

#define RPC_IS_RUNNING(t)	test_bit(RPC_TASK_RUNNING, &(t)->tk_runstate)
#define rpc_set_running(t)	set_bit(RPC_TASK_RUNNING, &(t)->tk_runstate)
#define rpc_test_and_set_running(t) \
				test_and_set_bit(RPC_TASK_RUNNING, &(t)->tk_runstate)
#define rpc_clear_running(t)	\
	do { \
		smp_mb__before_clear_bit(); \
		clear_bit(RPC_TASK_RUNNING, &(t)->tk_runstate); \
		smp_mb__after_clear_bit(); \
	} while (0)

#define RPC_IS_QUEUED(t)	test_bit(RPC_TASK_QUEUED, &(t)->tk_runstate)
#define rpc_set_queued(t)	set_bit(RPC_TASK_QUEUED, &(t)->tk_runstate)
#define rpc_clear_queued(t)	\
	do { \
		smp_mb__before_clear_bit(); \
		clear_bit(RPC_TASK_QUEUED, &(t)->tk_runstate); \
		smp_mb__after_clear_bit(); \
	} while (0)

#define RPC_IS_ACTIVATED(t)	test_bit(RPC_TASK_ACTIVE, &(t)->tk_runstate)

#define RPC_PRIORITY_LOW	(-1)
#define RPC_PRIORITY_NORMAL	(0)
#define RPC_PRIORITY_HIGH	(1)
#define RPC_PRIORITY_PRIVILEGED	(2)
#define RPC_NR_PRIORITY		(1 + RPC_PRIORITY_PRIVILEGED - RPC_PRIORITY_LOW)

struct rpc_timer {
	struct timer_list timer;
	struct list_head list;
	unsigned long expires;
};

struct rpc_wait_queue {
	spinlock_t		lock;
	struct list_head	tasks[RPC_NR_PRIORITY];	
	pid_t			owner;			
	unsigned char		maxpriority;		
	unsigned char		priority;		
	unsigned char		count;			
	unsigned char		nr;			
	unsigned short		qlen;			
	struct rpc_timer	timer_list;
#if defined(RPC_DEBUG) || defined(RPC_TRACEPOINTS)
	const char *		name;
#endif
};

#define RPC_BATCH_COUNT			16
#define RPC_IS_PRIORITY(q)		((q)->maxpriority > 0)

struct rpc_task *rpc_new_task(const struct rpc_task_setup *);
struct rpc_task *rpc_run_task(const struct rpc_task_setup *);
struct rpc_task *rpc_run_bc_task(struct rpc_rqst *req,
				const struct rpc_call_ops *ops);
void		rpc_put_task(struct rpc_task *);
void		rpc_put_task_async(struct rpc_task *);
void		rpc_exit_task(struct rpc_task *);
void		rpc_exit(struct rpc_task *, int);
void		rpc_release_calldata(const struct rpc_call_ops *, void *);
void		rpc_killall_tasks(struct rpc_clnt *);
void		rpc_execute(struct rpc_task *);
void		rpc_init_priority_wait_queue(struct rpc_wait_queue *, const char *);
void		rpc_init_wait_queue(struct rpc_wait_queue *, const char *);
void		rpc_destroy_wait_queue(struct rpc_wait_queue *);
void		rpc_sleep_on(struct rpc_wait_queue *, struct rpc_task *,
					rpc_action action);
void		rpc_sleep_on_priority(struct rpc_wait_queue *,
					struct rpc_task *,
					rpc_action action,
					int priority);
void		rpc_wake_up_queued_task(struct rpc_wait_queue *,
					struct rpc_task *);
void		rpc_wake_up(struct rpc_wait_queue *);
struct rpc_task *rpc_wake_up_next(struct rpc_wait_queue *);
struct rpc_task *rpc_wake_up_first(struct rpc_wait_queue *,
					bool (*)(struct rpc_task *, void *),
					void *);
void		rpc_wake_up_status(struct rpc_wait_queue *, int);
int		rpc_queue_empty(struct rpc_wait_queue *);
void		rpc_delay(struct rpc_task *, unsigned long);
void *		rpc_malloc(struct rpc_task *, size_t);
void		rpc_free(void *);
int		rpciod_up(void);
void		rpciod_down(void);
int		__rpc_wait_for_completion_task(struct rpc_task *task, int (*)(void *));
#ifdef RPC_DEBUG
struct net;
void		rpc_show_tasks(struct net *);
#endif
int		rpc_init_mempool(void);
void		rpc_destroy_mempool(void);
extern struct workqueue_struct *rpciod_workqueue;
void		rpc_prepare_task(struct rpc_task *task);

static inline int rpc_wait_for_completion_task(struct rpc_task *task)
{
	return __rpc_wait_for_completion_task(task, NULL);
}

static inline void rpc_task_set_priority(struct rpc_task *task, unsigned char prio)
{
	task->tk_priority = prio - RPC_PRIORITY_LOW;
}

static inline int rpc_task_has_priority(struct rpc_task *task, unsigned char prio)
{
	return (task->tk_priority + RPC_PRIORITY_LOW == prio);
}

#if defined(RPC_DEBUG) || defined (RPC_TRACEPOINTS)
static inline const char * rpc_qname(const struct rpc_wait_queue *q)
{
	return ((q && q->name) ? q->name : "unknown");
}

static inline void rpc_assign_waitqueue_name(struct rpc_wait_queue *q,
		const char *name)
{
	q->name = name;
}
#else
static inline void rpc_assign_waitqueue_name(struct rpc_wait_queue *q,
		const char *name)
{
}
#endif

#endif 
