/*
 * Read-Copy Update mechanism for mutual exclusion
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright IBM Corporation, 2001
 *
 * Author: Dipankar Sarma <dipankar@in.ibm.com>
 *
 * Based on the original work by Paul McKenney <paulmck@us.ibm.com>
 * and inputs from Rusty Russell, Andrea Arcangeli and Andi Kleen.
 * Papers:
 * http://www.rdrop.com/users/paulmck/paper/rclockpdcsproof.pdf
 * http://lse.sourceforge.net/locking/rclock_OLS.2001.05.01c.sc.pdf (OLS2001)
 *
 * For detailed explanation of Read-Copy Update mechanism see -
 *		http://lse.sourceforge.net/locking/rcupdate.html
 *
 */

#ifndef __LINUX_RCUPDATE_H
#define __LINUX_RCUPDATE_H

#include <linux/types.h>
#include <linux/cache.h>
#include <linux/spinlock.h>
#include <linux/threads.h>
#include <linux/cpumask.h>
#include <linux/seqlock.h>
#include <linux/lockdep.h>
#include <linux/completion.h>
#include <linux/debugobjects.h>
#include <linux/bug.h>
#include <linux/compiler.h>

#ifdef CONFIG_RCU_TORTURE_TEST
extern int rcutorture_runnable; 
#endif 

#if defined(CONFIG_TREE_RCU) || defined(CONFIG_TREE_PREEMPT_RCU)
extern void rcutorture_record_test_transition(void);
extern void rcutorture_record_progress(unsigned long vernum);
extern void do_trace_rcu_torture_read(char *rcutorturename,
				      struct rcu_head *rhp);
#else
static inline void rcutorture_record_test_transition(void)
{
}
static inline void rcutorture_record_progress(unsigned long vernum)
{
}
#ifdef CONFIG_RCU_TRACE
extern void do_trace_rcu_torture_read(char *rcutorturename,
				      struct rcu_head *rhp);
#else
#define do_trace_rcu_torture_read(rcutorturename, rhp) do { } while (0)
#endif
#endif

#define UINT_CMP_GE(a, b)	(UINT_MAX / 2 >= (a) - (b))
#define UINT_CMP_LT(a, b)	(UINT_MAX / 2 < (a) - (b))
#define ULONG_CMP_GE(a, b)	(ULONG_MAX / 2 >= (a) - (b))
#define ULONG_CMP_LT(a, b)	(ULONG_MAX / 2 < (a) - (b))


#ifdef CONFIG_PREEMPT_RCU

extern void call_rcu(struct rcu_head *head,
			      void (*func)(struct rcu_head *head));

#else 

#define	call_rcu	call_rcu_sched

#endif 

extern void call_rcu_bh(struct rcu_head *head,
			void (*func)(struct rcu_head *head));

extern void call_rcu_sched(struct rcu_head *head,
			   void (*func)(struct rcu_head *rcu));

extern void synchronize_sched(void);

#ifdef CONFIG_PREEMPT_RCU

extern void __rcu_read_lock(void);
extern void __rcu_read_unlock(void);
void synchronize_rcu(void);

#define rcu_preempt_depth() (current->rcu_read_lock_nesting)

#else 

static inline void __rcu_read_lock(void)
{
	preempt_disable();
}

static inline void __rcu_read_unlock(void)
{
	preempt_enable();
}

static inline void synchronize_rcu(void)
{
	synchronize_sched();
}

static inline int rcu_preempt_depth(void)
{
	return 0;
}

#endif 

extern void rcu_sched_qs(int cpu);
extern void rcu_bh_qs(int cpu);
extern void rcu_check_callbacks(int cpu, int user);
struct notifier_block;
extern void rcu_idle_enter(void);
extern void rcu_idle_exit(void);
extern void rcu_irq_enter(void);
extern void rcu_irq_exit(void);

#define RCU_NONIDLE(a) \
	do { \
		rcu_idle_exit(); \
		do { a; } while (0); \
		rcu_idle_enter(); \
	} while (0)


typedef void call_rcu_func_t(struct rcu_head *head,
			     void (*func)(struct rcu_head *head));
void wait_rcu_gp(call_rcu_func_t crf);

#if defined(CONFIG_TREE_RCU) || defined(CONFIG_TREE_PREEMPT_RCU)
#include <linux/rcutree.h>
#elif defined(CONFIG_TINY_RCU) || defined(CONFIG_TINY_PREEMPT_RCU)
#include <linux/rcutiny.h>
#else
#error "Unknown RCU implementation specified to kernel configuration"
#endif

#ifdef CONFIG_DEBUG_OBJECTS_RCU_HEAD
extern void init_rcu_head_on_stack(struct rcu_head *head);
extern void destroy_rcu_head_on_stack(struct rcu_head *head);
#else 
static inline void init_rcu_head_on_stack(struct rcu_head *head)
{
}

static inline void destroy_rcu_head_on_stack(struct rcu_head *head)
{
}
#endif	

#if defined(CONFIG_HOTPLUG_CPU) && defined(CONFIG_PROVE_RCU)
bool rcu_lockdep_current_cpu_online(void);
#else 
static inline bool rcu_lockdep_current_cpu_online(void)
{
	return 1;
}
#endif 

#ifdef CONFIG_DEBUG_LOCK_ALLOC

#ifdef CONFIG_PROVE_RCU
extern int rcu_is_cpu_idle(void);
#else 
static inline int rcu_is_cpu_idle(void)
{
	return 0;
}
#endif 

static inline void rcu_lock_acquire(struct lockdep_map *map)
{
	lock_acquire(map, 0, 0, 2, 1, NULL, _THIS_IP_);
}

static inline void rcu_lock_release(struct lockdep_map *map)
{
	lock_release(map, 1, _THIS_IP_);
}

extern struct lockdep_map rcu_lock_map;
extern struct lockdep_map rcu_bh_lock_map;
extern struct lockdep_map rcu_sched_lock_map;
extern int debug_lockdep_rcu_enabled(void);

static inline int rcu_read_lock_held(void)
{
	if (!debug_lockdep_rcu_enabled())
		return 1;
	if (rcu_is_cpu_idle())
		return 0;
	if (!rcu_lockdep_current_cpu_online())
		return 0;
	return lock_is_held(&rcu_lock_map);
}

extern int rcu_read_lock_bh_held(void);

#ifdef CONFIG_PREEMPT_COUNT
static inline int rcu_read_lock_sched_held(void)
{
	int lockdep_opinion = 0;

	if (!debug_lockdep_rcu_enabled())
		return 1;
	if (rcu_is_cpu_idle())
		return 0;
	if (!rcu_lockdep_current_cpu_online())
		return 0;
	if (debug_locks)
		lockdep_opinion = lock_is_held(&rcu_sched_lock_map);
	return lockdep_opinion || preempt_count() != 0 || irqs_disabled();
}
#else 
static inline int rcu_read_lock_sched_held(void)
{
	return 1;
}
#endif 

#else 

# define rcu_lock_acquire(a)		do { } while (0)
# define rcu_lock_release(a)		do { } while (0)

static inline int rcu_read_lock_held(void)
{
	return 1;
}

static inline int rcu_read_lock_bh_held(void)
{
	return 1;
}

#ifdef CONFIG_PREEMPT_COUNT
static inline int rcu_read_lock_sched_held(void)
{
	return preempt_count() != 0 || irqs_disabled();
}
#else 
static inline int rcu_read_lock_sched_held(void)
{
	return 1;
}
#endif 

#endif 

#ifdef CONFIG_PROVE_RCU

extern int rcu_my_thread_group_empty(void);

#define rcu_lockdep_assert(c, s)					\
	do {								\
		static bool __section(.data.unlikely) __warned;		\
		if (debug_lockdep_rcu_enabled() && !__warned && !(c)) {	\
			__warned = true;				\
			lockdep_rcu_suspicious(__FILE__, __LINE__, s);	\
		}							\
	} while (0)

#if defined(CONFIG_PROVE_RCU) && !defined(CONFIG_PREEMPT_RCU)
static inline void rcu_preempt_sleep_check(void)
{
	rcu_lockdep_assert(!lock_is_held(&rcu_lock_map),
			   "Illegal context switch in RCU read-side "
			   "critical section");
}
#else 
static inline void rcu_preempt_sleep_check(void)
{
}
#endif 

#define rcu_sleep_check()						\
	do {								\
		rcu_preempt_sleep_check();				\
		rcu_lockdep_assert(!lock_is_held(&rcu_bh_lock_map),	\
				   "Illegal context switch in RCU-bh"	\
				   " read-side critical section");	\
		rcu_lockdep_assert(!lock_is_held(&rcu_sched_lock_map),	\
				   "Illegal context switch in RCU-sched"\
				   " read-side critical section");	\
	} while (0)

#else 

#define rcu_lockdep_assert(c, s) do { } while (0)
#define rcu_sleep_check() do { } while (0)

#endif 


#ifdef __CHECKER__
#define rcu_dereference_sparse(p, space) \
	((void)(((typeof(*p) space *)p) == p))
#else 
#define rcu_dereference_sparse(p, space)
#endif 

#define __rcu_access_pointer(p, space) \
	({ \
		typeof(*p) *_________p1 = (typeof(*p)*__force )ACCESS_ONCE(p); \
		rcu_dereference_sparse(p, space); \
		((typeof(*p) __force __kernel *)(_________p1)); \
	})
#define __rcu_dereference_check(p, c, space) \
	({ \
		typeof(*p) *_________p1 = (typeof(*p)*__force )ACCESS_ONCE(p); \
		rcu_lockdep_assert(c, "suspicious rcu_dereference_check()" \
				      " usage"); \
		rcu_dereference_sparse(p, space); \
		smp_read_barrier_depends(); \
		((typeof(*p) __force __kernel *)(_________p1)); \
	})
#define __rcu_dereference_protected(p, c, space) \
	({ \
		rcu_lockdep_assert(c, "suspicious rcu_dereference_protected()" \
				      " usage"); \
		rcu_dereference_sparse(p, space); \
		((typeof(*p) __force __kernel *)(p)); \
	})

#define __rcu_access_index(p, space) \
	({ \
		typeof(p) _________p1 = ACCESS_ONCE(p); \
		rcu_dereference_sparse(p, space); \
		(_________p1); \
	})
#define __rcu_dereference_index_check(p, c) \
	({ \
		typeof(p) _________p1 = ACCESS_ONCE(p); \
		rcu_lockdep_assert(c, \
				   "suspicious rcu_dereference_index_check()" \
				   " usage"); \
		smp_read_barrier_depends(); \
		(_________p1); \
	})
#define __rcu_assign_pointer(p, v, space) \
	({ \
		smp_wmb(); \
		(p) = (typeof(*v) __force space *)(v); \
	})


#define rcu_access_pointer(p) __rcu_access_pointer((p), __rcu)

#define rcu_dereference_check(p, c) \
	__rcu_dereference_check((p), rcu_read_lock_held() || (c), __rcu)

#define rcu_dereference_bh_check(p, c) \
	__rcu_dereference_check((p), rcu_read_lock_bh_held() || (c), __rcu)

#define rcu_dereference_sched_check(p, c) \
	__rcu_dereference_check((p), rcu_read_lock_sched_held() || (c), \
				__rcu)

#define rcu_dereference_raw(p) rcu_dereference_check(p, 1) 

#define rcu_access_index(p) __rcu_access_index((p), __rcu)

#define rcu_dereference_index_check(p, c) \
	__rcu_dereference_index_check((p), (c))

#define rcu_dereference_protected(p, c) \
	__rcu_dereference_protected((p), (c), __rcu)


#define rcu_dereference(p) rcu_dereference_check(p, 0)

#define rcu_dereference_bh(p) rcu_dereference_bh_check(p, 0)

#define rcu_dereference_sched(p) rcu_dereference_sched_check(p, 0)

static inline void rcu_read_lock(void)
{
	__rcu_read_lock();
	__acquire(RCU);
	rcu_lock_acquire(&rcu_lock_map);
	rcu_lockdep_assert(!rcu_is_cpu_idle(),
			   "rcu_read_lock() used illegally while idle");
}


static inline void rcu_read_unlock(void)
{
	rcu_lockdep_assert(!rcu_is_cpu_idle(),
			   "rcu_read_unlock() used illegally while idle");
	rcu_lock_release(&rcu_lock_map);
	__release(RCU);
	__rcu_read_unlock();
}

static inline void rcu_read_lock_bh(void)
{
	local_bh_disable();
	__acquire(RCU_BH);
	rcu_lock_acquire(&rcu_bh_lock_map);
	rcu_lockdep_assert(!rcu_is_cpu_idle(),
			   "rcu_read_lock_bh() used illegally while idle");
}

static inline void rcu_read_unlock_bh(void)
{
	rcu_lockdep_assert(!rcu_is_cpu_idle(),
			   "rcu_read_unlock_bh() used illegally while idle");
	rcu_lock_release(&rcu_bh_lock_map);
	__release(RCU_BH);
	local_bh_enable();
}

static inline void rcu_read_lock_sched(void)
{
	preempt_disable();
	__acquire(RCU_SCHED);
	rcu_lock_acquire(&rcu_sched_lock_map);
	rcu_lockdep_assert(!rcu_is_cpu_idle(),
			   "rcu_read_lock_sched() used illegally while idle");
}

static inline notrace void rcu_read_lock_sched_notrace(void)
{
	preempt_disable_notrace();
	__acquire(RCU_SCHED);
}

static inline void rcu_read_unlock_sched(void)
{
	rcu_lockdep_assert(!rcu_is_cpu_idle(),
			   "rcu_read_unlock_sched() used illegally while idle");
	rcu_lock_release(&rcu_sched_lock_map);
	__release(RCU_SCHED);
	preempt_enable();
}

static inline notrace void rcu_read_unlock_sched_notrace(void)
{
	__release(RCU_SCHED);
	preempt_enable_notrace();
}

#define rcu_assign_pointer(p, v) \
	__rcu_assign_pointer((p), (v), __rcu)

#define RCU_INIT_POINTER(p, v) \
		p = (typeof(*v) __force __rcu *)(v)

static __always_inline bool __is_kfree_rcu_offset(unsigned long offset)
{
	return offset < 4096;
}

static __always_inline
void __kfree_rcu(struct rcu_head *head, unsigned long offset)
{
	typedef void (*rcu_callback)(struct rcu_head *);

	BUILD_BUG_ON(!__builtin_constant_p(offset));

	
	BUILD_BUG_ON(!__is_kfree_rcu_offset(offset));

	kfree_call_rcu(head, (rcu_callback)offset);
}

#define kfree_rcu(ptr, rcu_head)					\
	__kfree_rcu(&((ptr)->rcu_head), offsetof(typeof(*(ptr)), rcu_head))

#endif 
