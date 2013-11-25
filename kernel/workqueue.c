/*
 * kernel/workqueue.c - generic async execution with shared worker pool
 *
 * Copyright (C) 2002		Ingo Molnar
 *
 *   Derived from the taskqueue/keventd code by:
 *     David Woodhouse <dwmw2@infradead.org>
 *     Andrew Morton
 *     Kai Petzke <wpp@marie.physik.tu-berlin.de>
 *     Theodore Ts'o <tytso@mit.edu>
 *
 * Made to use alloc_percpu by Christoph Lameter.
 *
 * Copyright (C) 2010		SUSE Linux Products GmbH
 * Copyright (C) 2010		Tejun Heo <tj@kernel.org>
 *
 * This is the generic async execution mechanism.  Work items as are
 * executed in process context.  The worker pool is shared and
 * automatically managed.  There is one worker pool for each CPU and
 * one extra for works which are better served by workers which are
 * not bound to any specific CPU.
 *
 * Please read Documentation/workqueue.txt for details.
 */

#include <linux/export.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/signal.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/notifier.h>
#include <linux/kthread.h>
#include <linux/hardirq.h>
#include <linux/mempolicy.h>
#include <linux/freezer.h>
#include <linux/kallsyms.h>
#include <linux/debug_locks.h>
#include <linux/lockdep.h>
#include <linux/idr.h>

#include "workqueue_sched.h"

#ifdef CONFIG_TRACING_WORKQUEUE_HISTORY

#define WORKQUEUE_DEBUG	0

#define WQ_NAME			"events"
#define WQ_HIST_LEN		30

#define WORKQUEUE_HISTORY_ELEMENT_SIZE		0x40	

static unsigned int wq_pos = 0;
static unsigned long wq_hist[WQ_HIST_LEN];
extern char *wq_history_flag;

static int store_workqueue(const char *wq_name, unsigned long f_addr)
{
	char func_sym[KSYM_SYMBOL_LEN];
	static unsigned int count = 0;

	if (strcmp(wq_name, WQ_NAME) == 0) {
		wq_hist[wq_pos] = f_addr;

		if (wq_history_flag) {
			sprint_symbol(func_sym, f_addr);
			count++;
			memcpy(&wq_history_flag[(wq_pos * WORKQUEUE_HISTORY_ELEMENT_SIZE)], &count, sizeof(count));
			strlcpy(&wq_history_flag[(wq_pos * WORKQUEUE_HISTORY_ELEMENT_SIZE) + sizeof(count)], func_sym, (WORKQUEUE_HISTORY_ELEMENT_SIZE - sizeof(count)));
		}
#if WORKQUEUE_DEBUG
		if (!wq_history_flag)
			sprint_symbol(func_sym, f_addr);

		printk(KERN_INFO "[wq] %s: %s\n", wq_name, func_sym);
#endif
		if (++wq_pos >= WQ_HIST_LEN)
			wq_pos = 0;
	}

	return 0;
}

int print_workqueue(void)
{
	char func_sym[KSYM_SYMBOL_LEN];
	int i, wq_pos_cur, count = 0;

	i = wq_pos_cur = wq_pos;
	do {
		sprint_symbol(func_sym, wq_hist[i]);
		printk(KERN_INFO "[wq_list] %s[%d]: %s\n", WQ_NAME, count--, func_sym);

		if (--i < 0)
			i = WQ_HIST_LEN - 1;
	} while (wq_pos_cur != i);

	return 0;
}
#endif

enum {
	
	GCWQ_MANAGE_WORKERS	= 1 << 0,	
	GCWQ_MANAGING_WORKERS	= 1 << 1,	
	GCWQ_DISASSOCIATED	= 1 << 2,	
	GCWQ_FREEZING		= 1 << 3,	
	GCWQ_HIGHPRI_PENDING	= 1 << 4,	

	
	WORKER_STARTED		= 1 << 0,	
	WORKER_DIE		= 1 << 1,	
	WORKER_IDLE		= 1 << 2,	
	WORKER_PREP		= 1 << 3,	
	WORKER_ROGUE		= 1 << 4,	
	WORKER_REBIND		= 1 << 5,	
	WORKER_CPU_INTENSIVE	= 1 << 6,	
	WORKER_UNBOUND		= 1 << 7,	

	WORKER_NOT_RUNNING	= WORKER_PREP | WORKER_ROGUE | WORKER_REBIND |
				  WORKER_CPU_INTENSIVE | WORKER_UNBOUND,

	
	TRUSTEE_START		= 0,		
	TRUSTEE_IN_CHARGE	= 1,		
	TRUSTEE_BUTCHER		= 2,		
	TRUSTEE_RELEASE		= 3,		
	TRUSTEE_DONE		= 4,		

	BUSY_WORKER_HASH_ORDER	= 6,		
	BUSY_WORKER_HASH_SIZE	= 1 << BUSY_WORKER_HASH_ORDER,
	BUSY_WORKER_HASH_MASK	= BUSY_WORKER_HASH_SIZE - 1,

	MAX_IDLE_WORKERS_RATIO	= 4,		
	IDLE_WORKER_TIMEOUT	= 300 * HZ,	

	MAYDAY_INITIAL_TIMEOUT  = HZ / 100 >= 2 ? HZ / 100 : 2,
	MAYDAY_INTERVAL		= HZ / 10,	
	CREATE_COOLDOWN		= HZ,		
	TRUSTEE_COOLDOWN	= HZ / 10,	

	RESCUER_NICE_LEVEL	= -20,
};


struct global_cwq;

struct worker {
	
	union {
		struct list_head	entry;	
		struct hlist_node	hentry;	
	};

	struct work_struct	*current_work;	
	struct cpu_workqueue_struct *current_cwq; 
	struct list_head	scheduled;	
	struct task_struct	*task;		
	struct global_cwq	*gcwq;		
	
	unsigned long		last_active;	
	unsigned int		flags;		
	int			id;		
	struct work_struct	rebind_work;	
	struct work_struct	*previous_work;	
};

struct global_cwq {
	spinlock_t		lock;		
	struct list_head	worklist;	
	unsigned int		cpu;		
	unsigned int		flags;		

	int			nr_workers;	
	int			nr_idle;	

	
	struct list_head	idle_list;	
	struct hlist_head	busy_hash[BUSY_WORKER_HASH_SIZE];
						

	struct timer_list	idle_timer;	
	struct timer_list	mayday_timer;	

	struct ida		worker_ida;	

	struct task_struct	*trustee;	
	unsigned int		trustee_state;	
	wait_queue_head_t	trustee_wait;	
	struct worker		*first_idle;	
} ____cacheline_aligned_in_smp;

struct cpu_workqueue_struct {
	struct global_cwq	*gcwq;		
	struct workqueue_struct *wq;		
	int			work_color;	
	int			flush_color;	
	int			nr_in_flight[WORK_NR_COLORS];
						
	int			nr_active;	
	int			max_active;	
	struct list_head	delayed_works;	
};

struct wq_flusher {
	struct list_head	list;		
	int			flush_color;	
	struct completion	done;		
};

#ifdef CONFIG_SMP
typedef cpumask_var_t mayday_mask_t;
#define mayday_test_and_set_cpu(cpu, mask)	\
	cpumask_test_and_set_cpu((cpu), (mask))
#define mayday_clear_cpu(cpu, mask)		cpumask_clear_cpu((cpu), (mask))
#define for_each_mayday_cpu(cpu, mask)		for_each_cpu((cpu), (mask))
#define alloc_mayday_mask(maskp, gfp)		zalloc_cpumask_var((maskp), (gfp))
#define free_mayday_mask(mask)			free_cpumask_var((mask))
#else
typedef unsigned long mayday_mask_t;
#define mayday_test_and_set_cpu(cpu, mask)	test_and_set_bit(0, &(mask))
#define mayday_clear_cpu(cpu, mask)		clear_bit(0, &(mask))
#define for_each_mayday_cpu(cpu, mask)		if ((cpu) = 0, (mask))
#define alloc_mayday_mask(maskp, gfp)		true
#define free_mayday_mask(mask)			do { } while (0)
#endif

struct workqueue_struct {
	unsigned int		flags;		
	union {
		struct cpu_workqueue_struct __percpu	*pcpu;
		struct cpu_workqueue_struct		*single;
		unsigned long				v;
	} cpu_wq;				
	struct list_head	list;		

	struct mutex		flush_mutex;	
	int			work_color;	
	int			flush_color;	
	atomic_t		nr_cwqs_to_flush; 
	struct wq_flusher	*first_flusher;	
	struct list_head	flusher_queue;	
	struct list_head	flusher_overflow; 

	mayday_mask_t		mayday_mask;	
	struct worker		*rescuer;	

	int			nr_drainers;	
	int			saved_max_active; 
#ifdef CONFIG_LOCKDEP
	struct lockdep_map	lockdep_map;
#endif
	char			name[];		
};

struct workqueue_struct *system_wq __read_mostly;
struct workqueue_struct *system_long_wq __read_mostly;
struct workqueue_struct *system_nrt_wq __read_mostly;
struct workqueue_struct *system_unbound_wq __read_mostly;
struct workqueue_struct *system_freezable_wq __read_mostly;
struct workqueue_struct *system_nrt_freezable_wq __read_mostly;
EXPORT_SYMBOL_GPL(system_wq);
EXPORT_SYMBOL_GPL(system_long_wq);
EXPORT_SYMBOL_GPL(system_nrt_wq);
EXPORT_SYMBOL_GPL(system_unbound_wq);
EXPORT_SYMBOL_GPL(system_freezable_wq);
EXPORT_SYMBOL_GPL(system_nrt_freezable_wq);

#define CREATE_TRACE_POINTS
#include <trace/events/workqueue.h>

#define for_each_busy_worker(worker, i, pos, gcwq)			\
	for (i = 0; i < BUSY_WORKER_HASH_SIZE; i++)			\
		hlist_for_each_entry(worker, pos, &gcwq->busy_hash[i], hentry)

static inline int __next_gcwq_cpu(int cpu, const struct cpumask *mask,
				  unsigned int sw)
{
	if (cpu < nr_cpu_ids) {
		if (sw & 1) {
			cpu = cpumask_next(cpu, mask);
			if (cpu < nr_cpu_ids)
				return cpu;
		}
		if (sw & 2)
			return WORK_CPU_UNBOUND;
	}
	return WORK_CPU_NONE;
}

static inline int __next_wq_cpu(int cpu, const struct cpumask *mask,
				struct workqueue_struct *wq)
{
	return __next_gcwq_cpu(cpu, mask, !(wq->flags & WQ_UNBOUND) ? 1 : 2);
}

#define for_each_gcwq_cpu(cpu)						\
	for ((cpu) = __next_gcwq_cpu(-1, cpu_possible_mask, 3);		\
	     (cpu) < WORK_CPU_NONE;					\
	     (cpu) = __next_gcwq_cpu((cpu), cpu_possible_mask, 3))

#define for_each_online_gcwq_cpu(cpu)					\
	for ((cpu) = __next_gcwq_cpu(-1, cpu_online_mask, 3);		\
	     (cpu) < WORK_CPU_NONE;					\
	     (cpu) = __next_gcwq_cpu((cpu), cpu_online_mask, 3))

#define for_each_cwq_cpu(cpu, wq)					\
	for ((cpu) = __next_wq_cpu(-1, cpu_possible_mask, (wq));	\
	     (cpu) < WORK_CPU_NONE;					\
	     (cpu) = __next_wq_cpu((cpu), cpu_possible_mask, (wq)))

#ifdef CONFIG_DEBUG_OBJECTS_WORK

static struct debug_obj_descr work_debug_descr;

static void *work_debug_hint(void *addr)
{
	return ((struct work_struct *) addr)->func;
}

static int work_fixup_init(void *addr, enum debug_obj_state state)
{
	struct work_struct *work = addr;

	switch (state) {
	case ODEBUG_STATE_ACTIVE:
		cancel_work_sync(work);
		debug_object_init(work, &work_debug_descr);
		return 1;
	default:
		return 0;
	}
}

static int work_fixup_activate(void *addr, enum debug_obj_state state)
{
	struct work_struct *work = addr;

	switch (state) {

	case ODEBUG_STATE_NOTAVAILABLE:
		if (test_bit(WORK_STRUCT_STATIC_BIT, work_data_bits(work))) {
			debug_object_init(work, &work_debug_descr);
			debug_object_activate(work, &work_debug_descr);
			return 0;
		}
		WARN_ON_ONCE(1);
		return 0;

	case ODEBUG_STATE_ACTIVE:
		WARN_ON(1);

	default:
		return 0;
	}
}

static int work_fixup_free(void *addr, enum debug_obj_state state)
{
	struct work_struct *work = addr;

	switch (state) {
	case ODEBUG_STATE_ACTIVE:
		cancel_work_sync(work);
		debug_object_free(work, &work_debug_descr);
		return 1;
	default:
		return 0;
	}
}

static struct debug_obj_descr work_debug_descr = {
	.name		= "work_struct",
	.debug_hint	= work_debug_hint,
	.fixup_init	= work_fixup_init,
	.fixup_activate	= work_fixup_activate,
	.fixup_free	= work_fixup_free,
};

static inline void debug_work_activate(struct work_struct *work)
{
	debug_object_activate(work, &work_debug_descr);
}

static inline void debug_work_deactivate(struct work_struct *work)
{
	debug_object_deactivate(work, &work_debug_descr);
}

void __init_work(struct work_struct *work, int onstack)
{
	if (onstack)
		debug_object_init_on_stack(work, &work_debug_descr);
	else
		debug_object_init(work, &work_debug_descr);
}
EXPORT_SYMBOL_GPL(__init_work);

void destroy_work_on_stack(struct work_struct *work)
{
	debug_object_free(work, &work_debug_descr);
}
EXPORT_SYMBOL_GPL(destroy_work_on_stack);

#else
static inline void debug_work_activate(struct work_struct *work) { }
static inline void debug_work_deactivate(struct work_struct *work) { }
#endif

static DEFINE_SPINLOCK(workqueue_lock);
static LIST_HEAD(workqueues);
static bool workqueue_freezing;		

static DEFINE_PER_CPU(struct global_cwq, global_cwq);
static DEFINE_PER_CPU_SHARED_ALIGNED(atomic_t, gcwq_nr_running);

static struct global_cwq unbound_global_cwq;
static atomic_t unbound_gcwq_nr_running = ATOMIC_INIT(0);	

static int worker_thread(void *__worker);

static struct global_cwq *get_gcwq(unsigned int cpu)
{
	if (cpu != WORK_CPU_UNBOUND)
		return &per_cpu(global_cwq, cpu);
	else
		return &unbound_global_cwq;
}

static atomic_t *get_gcwq_nr_running(unsigned int cpu)
{
	if (cpu != WORK_CPU_UNBOUND)
		return &per_cpu(gcwq_nr_running, cpu);
	else
		return &unbound_gcwq_nr_running;
}

static struct cpu_workqueue_struct *get_cwq(unsigned int cpu,
					    struct workqueue_struct *wq)
{
	if (!(wq->flags & WQ_UNBOUND)) {
		if (likely(cpu < nr_cpu_ids))
			return per_cpu_ptr(wq->cpu_wq.pcpu, cpu);
	} else if (likely(cpu == WORK_CPU_UNBOUND))
		return wq->cpu_wq.single;
	return NULL;
}

static unsigned int work_color_to_flags(int color)
{
	return color << WORK_STRUCT_COLOR_SHIFT;
}

static int get_work_color(struct work_struct *work)
{
	return (*work_data_bits(work) >> WORK_STRUCT_COLOR_SHIFT) &
		((1 << WORK_STRUCT_COLOR_BITS) - 1);
}

static int work_next_color(int color)
{
	return (color + 1) % WORK_NR_COLORS;
}

static inline void set_work_data(struct work_struct *work, unsigned long data,
				 unsigned long flags)
{
	BUG_ON(!work_pending(work));
	atomic_long_set(&work->data, data | flags | work_static(work));
}

static void set_work_cwq(struct work_struct *work,
			 struct cpu_workqueue_struct *cwq,
			 unsigned long extra_flags)
{
	set_work_data(work, (unsigned long)cwq,
		      WORK_STRUCT_PENDING | WORK_STRUCT_CWQ | extra_flags);
}

static void set_work_cpu(struct work_struct *work, unsigned int cpu)
{
	set_work_data(work, cpu << WORK_STRUCT_FLAG_BITS, WORK_STRUCT_PENDING);
}

static void clear_work_data(struct work_struct *work)
{
	set_work_data(work, WORK_STRUCT_NO_CPU, 0);
}

static struct cpu_workqueue_struct *get_work_cwq(struct work_struct *work)
{
	unsigned long data = atomic_long_read(&work->data);

	if (data & WORK_STRUCT_CWQ)
		return (void *)(data & WORK_STRUCT_WQ_DATA_MASK);
	else {
		pr_err("%s: return NULL (work = 0x%p, work->entry = 0x%p, work->data = %lu work->function = %pF)\n"
			, __func__, (void *)work, (void *)&work->entry, data, (void*)work->func);
		WARN_ON(1);
		return NULL;
	}
}

static struct global_cwq *get_work_gcwq(struct work_struct *work)
{
	unsigned long data = atomic_long_read(&work->data);
	unsigned int cpu;

	if (data & WORK_STRUCT_CWQ)
		return ((struct cpu_workqueue_struct *)
			(data & WORK_STRUCT_WQ_DATA_MASK))->gcwq;

	cpu = data >> WORK_STRUCT_FLAG_BITS;
	if (cpu == WORK_CPU_NONE)
		return NULL;

	BUG_ON(cpu >= nr_cpu_ids && cpu != WORK_CPU_UNBOUND);
	return get_gcwq(cpu);
}


static bool __need_more_worker(struct global_cwq *gcwq)
{
	return !atomic_read(get_gcwq_nr_running(gcwq->cpu)) ||
		gcwq->flags & GCWQ_HIGHPRI_PENDING;
}

static bool need_more_worker(struct global_cwq *gcwq)
{
	return !list_empty(&gcwq->worklist) && __need_more_worker(gcwq);
}

static bool may_start_working(struct global_cwq *gcwq)
{
	return gcwq->nr_idle;
}

static bool keep_working(struct global_cwq *gcwq)
{
	atomic_t *nr_running = get_gcwq_nr_running(gcwq->cpu);

	return !list_empty(&gcwq->worklist) &&
		(atomic_read(nr_running) <= 1 ||
		 gcwq->flags & GCWQ_HIGHPRI_PENDING);
}

static bool need_to_create_worker(struct global_cwq *gcwq)
{
	return need_more_worker(gcwq) && !may_start_working(gcwq);
}

static bool need_to_manage_workers(struct global_cwq *gcwq)
{
	return need_to_create_worker(gcwq) || gcwq->flags & GCWQ_MANAGE_WORKERS;
}

static bool too_many_workers(struct global_cwq *gcwq)
{
	bool managing = gcwq->flags & GCWQ_MANAGING_WORKERS;
	int nr_idle = gcwq->nr_idle + managing; 
	int nr_busy = gcwq->nr_workers - nr_idle;

	return nr_idle > 2 && (nr_idle - 2) * MAX_IDLE_WORKERS_RATIO >= nr_busy;
}


static struct worker *first_worker(struct global_cwq *gcwq)
{
	if (unlikely(list_empty(&gcwq->idle_list)))
		return NULL;

	return list_first_entry(&gcwq->idle_list, struct worker, entry);
}

static void wake_up_worker(struct global_cwq *gcwq)
{
	struct worker *worker = first_worker(gcwq);

	if (likely(worker))
		wake_up_process(worker->task);
}

void wq_worker_waking_up(struct task_struct *task, unsigned int cpu)
{
	struct worker *worker = kthread_data(task);

	if (!(worker->flags & WORKER_NOT_RUNNING))
		atomic_inc(get_gcwq_nr_running(cpu));
}

struct task_struct *wq_worker_sleeping(struct task_struct *task,
				       unsigned int cpu)
{
	struct worker *worker = kthread_data(task), *to_wakeup = NULL;
	struct global_cwq *gcwq = get_gcwq(cpu);
	atomic_t *nr_running = get_gcwq_nr_running(cpu);

	if (worker->flags & WORKER_NOT_RUNNING)
		return NULL;

	
	BUG_ON(cpu != raw_smp_processor_id());

	if (atomic_dec_and_test(nr_running) && !list_empty(&gcwq->worklist))
		to_wakeup = first_worker(gcwq);
	return to_wakeup ? to_wakeup->task : NULL;
}

static inline void worker_set_flags(struct worker *worker, unsigned int flags,
				    bool wakeup)
{
	struct global_cwq *gcwq = worker->gcwq;

	WARN_ON_ONCE(worker->task != current);

	if ((flags & WORKER_NOT_RUNNING) &&
	    !(worker->flags & WORKER_NOT_RUNNING)) {
		atomic_t *nr_running = get_gcwq_nr_running(gcwq->cpu);

		if (wakeup) {
			if (atomic_dec_and_test(nr_running) &&
			    !list_empty(&gcwq->worklist))
				wake_up_worker(gcwq);
		} else
			atomic_dec(nr_running);
	}

	worker->flags |= flags;
}

static inline void worker_clr_flags(struct worker *worker, unsigned int flags)
{
	struct global_cwq *gcwq = worker->gcwq;
	unsigned int oflags = worker->flags;

	WARN_ON_ONCE(worker->task != current);

	worker->flags &= ~flags;

	if ((flags & WORKER_NOT_RUNNING) && (oflags & WORKER_NOT_RUNNING))
		if (!(worker->flags & WORKER_NOT_RUNNING))
			atomic_inc(get_gcwq_nr_running(gcwq->cpu));
}

static struct hlist_head *busy_worker_head(struct global_cwq *gcwq,
					   struct work_struct *work)
{
	const int base_shift = ilog2(sizeof(struct work_struct));
	unsigned long v = (unsigned long)work;

	
	v >>= base_shift;
	v += v >> BUSY_WORKER_HASH_ORDER;
	v &= BUSY_WORKER_HASH_MASK;

	return &gcwq->busy_hash[v];
}

static struct worker *__find_worker_executing_work(struct global_cwq *gcwq,
						   struct hlist_head *bwh,
						   struct work_struct *work)
{
	struct worker *worker;
	struct hlist_node *tmp;

	hlist_for_each_entry(worker, tmp, bwh, hentry)
		if (worker->current_work == work)
			return worker;
	return NULL;
}

static struct worker *find_worker_executing_work(struct global_cwq *gcwq,
						 struct work_struct *work)
{
	return __find_worker_executing_work(gcwq, busy_worker_head(gcwq, work),
					    work);
}

static inline struct list_head *gcwq_determine_ins_pos(struct global_cwq *gcwq,
					       struct cpu_workqueue_struct *cwq)
{
	struct work_struct *twork;

	if (likely(!(cwq->wq->flags & WQ_HIGHPRI)))
		return &gcwq->worklist;

	list_for_each_entry(twork, &gcwq->worklist, entry) {
		struct cpu_workqueue_struct *tcwq = get_work_cwq(twork);

		if (tcwq && !(tcwq->wq->flags & WQ_HIGHPRI))
			break;
	}

	gcwq->flags |= GCWQ_HIGHPRI_PENDING;
	return &twork->entry;
}

static void insert_work(struct cpu_workqueue_struct *cwq,
			struct work_struct *work, struct list_head *head,
			unsigned int extra_flags)
{
	struct global_cwq *gcwq = cwq->gcwq;

	
	set_work_cwq(work, cwq, extra_flags);

	smp_wmb();

	list_add_tail(&work->entry, head);

	smp_mb();

	if (__need_more_worker(gcwq))
		wake_up_worker(gcwq);
}

static bool is_chained_work(struct workqueue_struct *wq)
{
	unsigned long flags;
	unsigned int cpu;

	for_each_gcwq_cpu(cpu) {
		struct global_cwq *gcwq = get_gcwq(cpu);
		struct worker *worker;
		struct hlist_node *pos;
		int i;

		spin_lock_irqsave(&gcwq->lock, flags);
		for_each_busy_worker(worker, i, pos, gcwq) {
			if (worker->task != current)
				continue;
			spin_unlock_irqrestore(&gcwq->lock, flags);
			return worker->current_cwq->wq == wq;
		}
		spin_unlock_irqrestore(&gcwq->lock, flags);
	}
	return false;
}

static void __queue_work(unsigned int cpu, struct workqueue_struct *wq,
			 struct work_struct *work)
{
	struct global_cwq *gcwq;
	struct cpu_workqueue_struct *cwq;
	struct list_head *worklist;
	unsigned int work_flags;
	unsigned long flags;

	debug_work_activate(work);

	
	if (unlikely(wq->flags & WQ_DRAINING) &&
	    WARN_ON_ONCE(!is_chained_work(wq)))
		return;

	
	if (!(wq->flags & WQ_UNBOUND)) {
		struct global_cwq *last_gcwq;

		if (unlikely(cpu == WORK_CPU_UNBOUND))
			cpu = raw_smp_processor_id();

		gcwq = get_gcwq(cpu);
		if (wq->flags & WQ_NON_REENTRANT &&
		    (last_gcwq = get_work_gcwq(work)) && last_gcwq != gcwq) {
			struct worker *worker;

			spin_lock_irqsave(&last_gcwq->lock, flags);

			worker = find_worker_executing_work(last_gcwq, work);

			if (worker && worker->current_cwq->wq == wq)
				gcwq = last_gcwq;
			else {
				
				spin_unlock_irqrestore(&last_gcwq->lock, flags);
				spin_lock_irqsave(&gcwq->lock, flags);
			}
		} else
			spin_lock_irqsave(&gcwq->lock, flags);
	} else {
		gcwq = get_gcwq(WORK_CPU_UNBOUND);
		spin_lock_irqsave(&gcwq->lock, flags);
	}

	
	cwq = get_cwq(gcwq->cpu, wq);
	trace_workqueue_queue_work(cpu, cwq, work);

	BUG_ON(!list_empty(&work->entry));

	cwq->nr_in_flight[cwq->work_color]++;
	work_flags = work_color_to_flags(cwq->work_color);

	if (likely(cwq->nr_active < cwq->max_active)) {
		trace_workqueue_activate_work(work);
		cwq->nr_active++;
		worklist = gcwq_determine_ins_pos(gcwq, cwq);
	} else {
		work_flags |= WORK_STRUCT_DELAYED;
		worklist = &cwq->delayed_works;
	}

	insert_work(cwq, work, worklist, work_flags);

	spin_unlock_irqrestore(&gcwq->lock, flags);
}

int queue_work(struct workqueue_struct *wq, struct work_struct *work)
{
	int ret;

	ret = queue_work_on(get_cpu(), wq, work);
	put_cpu();

	return ret;
}
EXPORT_SYMBOL_GPL(queue_work);

int
queue_work_on(int cpu, struct workqueue_struct *wq, struct work_struct *work)
{
	int ret = 0;

	if (!test_and_set_bit(WORK_STRUCT_PENDING_BIT, work_data_bits(work))) {
		__queue_work(cpu, wq, work);
		ret = 1;
	}
	return ret;
}
EXPORT_SYMBOL_GPL(queue_work_on);

static void delayed_work_timer_fn(unsigned long __data)
{
	struct delayed_work *dwork = (struct delayed_work *)__data;
	struct cpu_workqueue_struct *cwq = get_work_cwq(&dwork->work);

	if (unlikely(cwq == NULL)) {
		return;
	} else
		__queue_work(smp_processor_id(), cwq->wq, &dwork->work);
}

int queue_delayed_work(struct workqueue_struct *wq,
			struct delayed_work *dwork, unsigned long delay)
{
	if (delay == 0)
		return queue_work(wq, &dwork->work);

	return queue_delayed_work_on(-1, wq, dwork, delay);
}
EXPORT_SYMBOL_GPL(queue_delayed_work);

int queue_delayed_work_on(int cpu, struct workqueue_struct *wq,
			struct delayed_work *dwork, unsigned long delay)
{
	int ret = 0;
	struct timer_list *timer = &dwork->timer;
	struct work_struct *work = &dwork->work;

	if (!test_and_set_bit(WORK_STRUCT_PENDING_BIT, work_data_bits(work))) {
		unsigned int lcpu;

		BUG_ON(timer_pending(timer));
		BUG_ON(!list_empty(&work->entry));

		timer_stats_timer_set_start_info(&dwork->timer);

		if (!(wq->flags & WQ_UNBOUND)) {
			struct global_cwq *gcwq = get_work_gcwq(work);

			if (gcwq && gcwq->cpu != WORK_CPU_UNBOUND)
				lcpu = gcwq->cpu;
			else
				lcpu = raw_smp_processor_id();
		} else
			lcpu = WORK_CPU_UNBOUND;

		set_work_cwq(work, get_cwq(lcpu, wq), 0);

		timer->expires = jiffies + delay;
		timer->data = (unsigned long)dwork;
		timer->function = delayed_work_timer_fn;

		if (unlikely(cpu >= 0))
			add_timer_on(timer, cpu);
		else
			add_timer(timer);
		ret = 1;
	}
	return ret;
}
EXPORT_SYMBOL_GPL(queue_delayed_work_on);

static void worker_enter_idle(struct worker *worker)
{
	struct global_cwq *gcwq = worker->gcwq;

	BUG_ON(worker->flags & WORKER_IDLE);
	BUG_ON(!list_empty(&worker->entry) &&
	       (worker->hentry.next || worker->hentry.pprev));

	
	worker->flags |= WORKER_IDLE;
	gcwq->nr_idle++;
	worker->last_active = jiffies;

	
	list_add(&worker->entry, &gcwq->idle_list);

	if (likely(!(worker->flags & WORKER_ROGUE))) {
		if (too_many_workers(gcwq) && !timer_pending(&gcwq->idle_timer))
			mod_timer(&gcwq->idle_timer,
				  jiffies + IDLE_WORKER_TIMEOUT);
	} else
		wake_up_all(&gcwq->trustee_wait);

	WARN_ON_ONCE(gcwq->trustee_state == TRUSTEE_DONE &&
		     gcwq->nr_workers == gcwq->nr_idle &&
		     atomic_read(get_gcwq_nr_running(gcwq->cpu)));
}

static void worker_leave_idle(struct worker *worker)
{
	struct global_cwq *gcwq = worker->gcwq;

	BUG_ON(!(worker->flags & WORKER_IDLE));
	worker_clr_flags(worker, WORKER_IDLE);
	gcwq->nr_idle--;
	list_del_init(&worker->entry);
}

static bool worker_maybe_bind_and_lock(struct worker *worker)
__acquires(&gcwq->lock)
{
	struct global_cwq *gcwq = worker->gcwq;
	struct task_struct *task = worker->task;

	while (true) {
		if (!(gcwq->flags & GCWQ_DISASSOCIATED))
			set_cpus_allowed_ptr(task, get_cpu_mask(gcwq->cpu));

		spin_lock_irq(&gcwq->lock);
		if (gcwq->flags & GCWQ_DISASSOCIATED)
			return false;
		if (task_cpu(task) == gcwq->cpu &&
		    cpumask_equal(&current->cpus_allowed,
				  get_cpu_mask(gcwq->cpu)))
			return true;
		spin_unlock_irq(&gcwq->lock);

		cpu_relax();
		cond_resched();
	}
}

static void worker_rebind_fn(struct work_struct *work)
{
	struct worker *worker = container_of(work, struct worker, rebind_work);
	struct global_cwq *gcwq = worker->gcwq;

	if (worker_maybe_bind_and_lock(worker))
		worker_clr_flags(worker, WORKER_REBIND);

	spin_unlock_irq(&gcwq->lock);
}

static struct worker *alloc_worker(void)
{
	struct worker *worker;

	worker = kzalloc(sizeof(*worker), GFP_KERNEL);
	if (worker) {
		INIT_LIST_HEAD(&worker->entry);
		INIT_LIST_HEAD(&worker->scheduled);
		INIT_WORK(&worker->rebind_work, worker_rebind_fn);
		
		worker->flags = WORKER_PREP;
	}
	return worker;
}

static struct worker *create_worker(struct global_cwq *gcwq, bool bind)
{
	bool on_unbound_cpu = gcwq->cpu == WORK_CPU_UNBOUND;
	struct worker *worker = NULL;
	int id = -1;

	spin_lock_irq(&gcwq->lock);
	while (ida_get_new(&gcwq->worker_ida, &id)) {
		spin_unlock_irq(&gcwq->lock);
		if (!ida_pre_get(&gcwq->worker_ida, GFP_KERNEL))
			goto fail;
		spin_lock_irq(&gcwq->lock);
	}
	spin_unlock_irq(&gcwq->lock);

	worker = alloc_worker();
	if (!worker)
		goto fail;

	worker->gcwq = gcwq;
	worker->id = id;

	if (!on_unbound_cpu)
		worker->task = kthread_create_on_node(worker_thread,
						      worker,
						      cpu_to_node(gcwq->cpu),
						      "kworker/%u:%d", gcwq->cpu, id);
	else
		worker->task = kthread_create(worker_thread, worker,
					      "kworker/u:%d", id);
	if (IS_ERR(worker->task))
		goto fail;

	if (bind && !on_unbound_cpu)
		kthread_bind(worker->task, gcwq->cpu);
	else {
		worker->task->flags |= PF_THREAD_BOUND;
		if (on_unbound_cpu)
			worker->flags |= WORKER_UNBOUND;
	}

	return worker;
fail:
	if (id >= 0) {
		spin_lock_irq(&gcwq->lock);
		ida_remove(&gcwq->worker_ida, id);
		spin_unlock_irq(&gcwq->lock);
	}
	kfree(worker);
	return NULL;
}

static void start_worker(struct worker *worker)
{
	worker->flags |= WORKER_STARTED;
	worker->gcwq->nr_workers++;
	worker_enter_idle(worker);
	wake_up_process(worker->task);
}

static void destroy_worker(struct worker *worker)
{
	struct global_cwq *gcwq = worker->gcwq;
	int id = worker->id;

	
	BUG_ON(worker->current_work);
	BUG_ON(!list_empty(&worker->scheduled));

	if (worker->flags & WORKER_STARTED)
		gcwq->nr_workers--;
	if (worker->flags & WORKER_IDLE)
		gcwq->nr_idle--;

	list_del_init(&worker->entry);
	worker->flags |= WORKER_DIE;

	spin_unlock_irq(&gcwq->lock);

	kthread_stop(worker->task);
	kfree(worker);

	spin_lock_irq(&gcwq->lock);
	ida_remove(&gcwq->worker_ida, id);
}

static void idle_worker_timeout(unsigned long __gcwq)
{
	struct global_cwq *gcwq = (void *)__gcwq;

	spin_lock_irq(&gcwq->lock);

	if (too_many_workers(gcwq)) {
		struct worker *worker;
		unsigned long expires;

		
		worker = list_entry(gcwq->idle_list.prev, struct worker, entry);
		expires = worker->last_active + IDLE_WORKER_TIMEOUT;

		if (time_before(jiffies, expires))
			mod_timer(&gcwq->idle_timer, expires);
		else {
			
			gcwq->flags |= GCWQ_MANAGE_WORKERS;
			wake_up_worker(gcwq);
		}
	}

	spin_unlock_irq(&gcwq->lock);
}

static bool send_mayday(struct work_struct *work)
{
	struct cpu_workqueue_struct *cwq = get_work_cwq(work);
	struct workqueue_struct *wq = cwq->wq;
	unsigned int cpu;

	if (!(wq->flags & WQ_RESCUER))
		return false;

	
	cpu = cwq->gcwq->cpu;
	
	if (cpu == WORK_CPU_UNBOUND)
		cpu = 0;
	if (!mayday_test_and_set_cpu(cpu, wq->mayday_mask))
		wake_up_process(wq->rescuer->task);
	return true;
}

static void gcwq_mayday_timeout(unsigned long __gcwq)
{
	struct global_cwq *gcwq = (void *)__gcwq;
	struct work_struct *work;

	spin_lock_irq(&gcwq->lock);

	if (need_to_create_worker(gcwq)) {
		list_for_each_entry(work, &gcwq->worklist, entry)
			send_mayday(work);
	}

	spin_unlock_irq(&gcwq->lock);

	mod_timer(&gcwq->mayday_timer, jiffies + MAYDAY_INTERVAL);
}

static bool maybe_create_worker(struct global_cwq *gcwq)
__releases(&gcwq->lock)
__acquires(&gcwq->lock)
{
	if (!need_to_create_worker(gcwq))
		return false;
restart:
	spin_unlock_irq(&gcwq->lock);

	
	mod_timer(&gcwq->mayday_timer, jiffies + MAYDAY_INITIAL_TIMEOUT);

	while (true) {
		struct worker *worker;

		worker = create_worker(gcwq, true);
		if (worker) {
			del_timer_sync(&gcwq->mayday_timer);
			spin_lock_irq(&gcwq->lock);
			start_worker(worker);
			BUG_ON(need_to_create_worker(gcwq));
			return true;
		}

		if (!need_to_create_worker(gcwq))
			break;

		__set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(CREATE_COOLDOWN);

		if (!need_to_create_worker(gcwq))
			break;
	}

	del_timer_sync(&gcwq->mayday_timer);
	spin_lock_irq(&gcwq->lock);
	if (need_to_create_worker(gcwq))
		goto restart;
	return true;
}

static bool maybe_destroy_workers(struct global_cwq *gcwq)
{
	bool ret = false;

	while (too_many_workers(gcwq)) {
		struct worker *worker;
		unsigned long expires;

		worker = list_entry(gcwq->idle_list.prev, struct worker, entry);
		expires = worker->last_active + IDLE_WORKER_TIMEOUT;

		if (time_before(jiffies, expires)) {
			mod_timer(&gcwq->idle_timer, expires);
			break;
		}

		destroy_worker(worker);
		ret = true;
	}

	return ret;
}

static bool manage_workers(struct worker *worker)
{
	struct global_cwq *gcwq = worker->gcwq;
	bool ret = false;

	if (gcwq->flags & GCWQ_MANAGING_WORKERS)
		return ret;

	gcwq->flags &= ~GCWQ_MANAGE_WORKERS;
	gcwq->flags |= GCWQ_MANAGING_WORKERS;

	ret |= maybe_destroy_workers(gcwq);
	ret |= maybe_create_worker(gcwq);

	gcwq->flags &= ~GCWQ_MANAGING_WORKERS;

	if (unlikely(gcwq->trustee))
		wake_up_all(&gcwq->trustee_wait);

	return ret;
}

static void move_linked_works(struct work_struct *work, struct list_head *head,
			      struct work_struct **nextp)
{
	struct work_struct *n;

	list_for_each_entry_safe_from(work, n, NULL, entry) {
		list_move_tail(&work->entry, head);
		if (!(*work_data_bits(work) & WORK_STRUCT_LINKED))
			break;
	}

	if (nextp)
		*nextp = n;
}

static void cwq_activate_first_delayed(struct cpu_workqueue_struct *cwq)
{
	struct work_struct *work = list_first_entry(&cwq->delayed_works,
						    struct work_struct, entry);
	struct list_head *pos = gcwq_determine_ins_pos(cwq->gcwq, cwq);

	trace_workqueue_activate_work(work);
	move_linked_works(work, pos, NULL);
	__clear_bit(WORK_STRUCT_DELAYED_BIT, work_data_bits(work));
	cwq->nr_active++;
}

static void cwq_dec_nr_in_flight(struct cpu_workqueue_struct *cwq, int color,
				 bool delayed)
{
	
	if (color == WORK_NO_COLOR)
		return;

	cwq->nr_in_flight[color]--;

	if (!delayed) {
		cwq->nr_active--;
		if (!list_empty(&cwq->delayed_works)) {
			
			if (cwq->nr_active < cwq->max_active)
				cwq_activate_first_delayed(cwq);
		}
	}

	
	if (likely(cwq->flush_color != color))
		return;

	
	if (cwq->nr_in_flight[color])
		return;

	
	cwq->flush_color = -1;

	if (atomic_dec_and_test(&cwq->wq->nr_cwqs_to_flush))
		complete(&cwq->wq->first_flusher->done);
}

static void process_one_work(struct worker *worker, struct work_struct *work)
__releases(&gcwq->lock)
__acquires(&gcwq->lock)
{
	struct cpu_workqueue_struct *cwq = get_work_cwq(work);
	struct global_cwq *gcwq = cwq->gcwq;
	struct hlist_head *bwh = busy_worker_head(gcwq, work);
	bool cpu_intensive = cwq->wq->flags & WQ_CPU_INTENSIVE;
	work_func_t f = work->func;
	int work_color;
	struct worker *collision;
#ifdef CONFIG_LOCKDEP
	struct lockdep_map lockdep_map = work->lockdep_map;
#endif
	collision = __find_worker_executing_work(gcwq, bwh, work);
	if (unlikely(collision)) {
		move_linked_works(work, &collision->scheduled, NULL);
		return;
	}

	
	debug_work_deactivate(work);
	hlist_add_head(&worker->hentry, bwh);
	worker->current_work = work;
	worker->current_cwq = cwq;
	work_color = get_work_color(work);

	
	set_work_cpu(work, gcwq->cpu);
	list_del_init(&work->entry);

	if (unlikely(gcwq->flags & GCWQ_HIGHPRI_PENDING)) {
		struct work_struct *nwork = list_first_entry(&gcwq->worklist,
						struct work_struct, entry);

		if (!list_empty(&gcwq->worklist) &&
		    get_work_cwq(nwork)->wq->flags & WQ_HIGHPRI)
			wake_up_worker(gcwq);
		else
			gcwq->flags &= ~GCWQ_HIGHPRI_PENDING;
	}

	if (unlikely(cpu_intensive))
		worker_set_flags(worker, WORKER_CPU_INTENSIVE, true);

#ifdef CONFIG_TRACING_WORKQUEUE_HISTORY
	store_workqueue(cwq->wq->name, (unsigned long)f);
#endif

	spin_unlock_irq(&gcwq->lock);

	work_clear_pending(work);
	lock_map_acquire_read(&cwq->wq->lockdep_map);
	lock_map_acquire(&lockdep_map);
	trace_workqueue_execute_start(work);

	f(work);
	trace_workqueue_execute_end(work);
	lock_map_release(&lockdep_map);
	lock_map_release(&cwq->wq->lockdep_map);

	if (unlikely(in_atomic() || lockdep_depth(current) > 0)) {
		printk(KERN_ERR "BUG: workqueue leaked lock or atomic: "
		       "%s/0x%08x/%d\n",
		       current->comm, preempt_count(), task_pid_nr(current));
		printk(KERN_ERR "    last function: ");
		print_symbol("%s\n", (unsigned long)f);
		debug_show_held_locks(current);
		dump_stack();
	}

	spin_lock_irq(&gcwq->lock);

	
	if (unlikely(cpu_intensive))
		worker_clr_flags(worker, WORKER_CPU_INTENSIVE);

	
	hlist_del_init(&worker->hentry);
	worker->current_work = NULL;
	worker->previous_work = work;
	worker->current_cwq = NULL;
	cwq_dec_nr_in_flight(cwq, work_color, false);
}

static void process_scheduled_works(struct worker *worker)
{
	while (!list_empty(&worker->scheduled)) {
		struct work_struct *work = list_first_entry(&worker->scheduled,
						struct work_struct, entry);
		process_one_work(worker, work);
	}
}

static int worker_thread(void *__worker)
{
	struct worker *worker = __worker;
	struct global_cwq *gcwq = worker->gcwq;

	
	worker->task->flags |= PF_WQ_WORKER;
woke_up:
	spin_lock_irq(&gcwq->lock);

	
	if (worker->flags & WORKER_DIE) {
		spin_unlock_irq(&gcwq->lock);
		worker->task->flags &= ~PF_WQ_WORKER;
		return 0;
	}

	worker_leave_idle(worker);
recheck:
	
	if (!need_more_worker(gcwq))
		goto sleep;

	
	if (unlikely(!may_start_working(gcwq)) && manage_workers(worker))
		goto recheck;

	BUG_ON(!list_empty(&worker->scheduled));

	worker_clr_flags(worker, WORKER_PREP);

	do {
		struct work_struct *work =
			list_first_entry(&gcwq->worklist,
					 struct work_struct, entry);

		if (likely(!(*work_data_bits(work) & WORK_STRUCT_LINKED))) {
			
			process_one_work(worker, work);
			if (unlikely(!list_empty(&worker->scheduled)))
				process_scheduled_works(worker);
		} else {
			move_linked_works(work, &worker->scheduled, NULL);
			process_scheduled_works(worker);
		}
	} while (keep_working(gcwq));

	worker_set_flags(worker, WORKER_PREP, false);
sleep:
	if (unlikely(need_to_manage_workers(gcwq)) && manage_workers(worker))
		goto recheck;

	worker_enter_idle(worker);
	__set_current_state(TASK_INTERRUPTIBLE);
	spin_unlock_irq(&gcwq->lock);
	schedule();
	goto woke_up;
}

static int rescuer_thread(void *__wq)
{
	struct workqueue_struct *wq = __wq;
	struct worker *rescuer = wq->rescuer;
	struct list_head *scheduled = &rescuer->scheduled;
	bool is_unbound = wq->flags & WQ_UNBOUND;
	unsigned int cpu;

	set_user_nice(current, RESCUER_NICE_LEVEL);
repeat:
	set_current_state(TASK_INTERRUPTIBLE);

	if (kthread_should_stop())
		return 0;

	for_each_mayday_cpu(cpu, wq->mayday_mask) {
		unsigned int tcpu = is_unbound ? WORK_CPU_UNBOUND : cpu;
		struct cpu_workqueue_struct *cwq = get_cwq(tcpu, wq);
		struct global_cwq *gcwq = cwq->gcwq;
		struct work_struct *work, *n;

		__set_current_state(TASK_RUNNING);
		mayday_clear_cpu(cpu, wq->mayday_mask);

		
		rescuer->gcwq = gcwq;
		worker_maybe_bind_and_lock(rescuer);

		BUG_ON(!list_empty(&rescuer->scheduled));
		list_for_each_entry_safe(work, n, &gcwq->worklist, entry)
			if (get_work_cwq(work) == cwq)
				move_linked_works(work, scheduled, &n);

		process_scheduled_works(rescuer);

		if (keep_working(gcwq))
			wake_up_worker(gcwq);

		spin_unlock_irq(&gcwq->lock);
	}

	schedule();
	goto repeat;
}

struct wq_barrier {
	struct work_struct	work;
	struct completion	done;
};

static void wq_barrier_func(struct work_struct *work)
{
	struct wq_barrier *barr = container_of(work, struct wq_barrier, work);
	complete(&barr->done);
}

static void insert_wq_barrier(struct cpu_workqueue_struct *cwq,
			      struct wq_barrier *barr,
			      struct work_struct *target, struct worker *worker)
{
	struct list_head *head;
	unsigned int linked = 0;

	INIT_WORK_ONSTACK(&barr->work, wq_barrier_func);
	__set_bit(WORK_STRUCT_PENDING_BIT, work_data_bits(&barr->work));
	init_completion(&barr->done);

	if (worker)
		head = worker->scheduled.next;
	else {
		unsigned long *bits = work_data_bits(target);

		head = target->entry.next;
		
		linked = *bits & WORK_STRUCT_LINKED;
		__set_bit(WORK_STRUCT_LINKED_BIT, bits);
	}

	debug_work_activate(&barr->work);
	insert_work(cwq, &barr->work, head,
		    work_color_to_flags(WORK_NO_COLOR) | linked);
}

static bool flush_workqueue_prep_cwqs(struct workqueue_struct *wq,
				      int flush_color, int work_color)
{
	bool wait = false;
	unsigned int cpu;

	if (flush_color >= 0) {
		BUG_ON(atomic_read(&wq->nr_cwqs_to_flush));
		atomic_set(&wq->nr_cwqs_to_flush, 1);
	}

	for_each_cwq_cpu(cpu, wq) {
		struct cpu_workqueue_struct *cwq = get_cwq(cpu, wq);
		struct global_cwq *gcwq = cwq->gcwq;

		spin_lock_irq(&gcwq->lock);

		if (flush_color >= 0) {
			BUG_ON(cwq->flush_color != -1);

			if (cwq->nr_in_flight[flush_color]) {
				cwq->flush_color = flush_color;
				atomic_inc(&wq->nr_cwqs_to_flush);
				wait = true;
			}
		}

		if (work_color >= 0) {
			BUG_ON(work_color != work_next_color(cwq->work_color));
			cwq->work_color = work_color;
		}

		spin_unlock_irq(&gcwq->lock);
	}

	if (flush_color >= 0 && atomic_dec_and_test(&wq->nr_cwqs_to_flush))
		complete(&wq->first_flusher->done);

	return wait;
}

void flush_workqueue(struct workqueue_struct *wq)
{
	struct wq_flusher this_flusher = {
		.list = LIST_HEAD_INIT(this_flusher.list),
		.flush_color = -1,
		.done = COMPLETION_INITIALIZER_ONSTACK(this_flusher.done),
	};
	int next_color;

	lock_map_acquire(&wq->lockdep_map);
	lock_map_release(&wq->lockdep_map);

	mutex_lock(&wq->flush_mutex);

	next_color = work_next_color(wq->work_color);

	if (next_color != wq->flush_color) {
		BUG_ON(!list_empty(&wq->flusher_overflow));
		this_flusher.flush_color = wq->work_color;
		wq->work_color = next_color;

		if (!wq->first_flusher) {
			
			BUG_ON(wq->flush_color != this_flusher.flush_color);

			wq->first_flusher = &this_flusher;

			if (!flush_workqueue_prep_cwqs(wq, wq->flush_color,
						       wq->work_color)) {
				
				wq->flush_color = next_color;
				wq->first_flusher = NULL;
				goto out_unlock;
			}
		} else {
			
			BUG_ON(wq->flush_color == this_flusher.flush_color);
			list_add_tail(&this_flusher.list, &wq->flusher_queue);
			flush_workqueue_prep_cwqs(wq, -1, wq->work_color);
		}
	} else {
		list_add_tail(&this_flusher.list, &wq->flusher_overflow);
	}

	mutex_unlock(&wq->flush_mutex);

	wait_for_completion(&this_flusher.done);

	if (wq->first_flusher != &this_flusher)
		return;

	mutex_lock(&wq->flush_mutex);

	
	if (wq->first_flusher != &this_flusher)
		goto out_unlock;

	wq->first_flusher = NULL;

	BUG_ON(!list_empty(&this_flusher.list));
	BUG_ON(wq->flush_color != this_flusher.flush_color);

	while (true) {
		struct wq_flusher *next, *tmp;

		
		list_for_each_entry_safe(next, tmp, &wq->flusher_queue, list) {
			if (next->flush_color != wq->flush_color)
				break;
			list_del_init(&next->list);
			complete(&next->done);
		}

		BUG_ON(!list_empty(&wq->flusher_overflow) &&
		       wq->flush_color != work_next_color(wq->work_color));

		
		wq->flush_color = work_next_color(wq->flush_color);

		
		if (!list_empty(&wq->flusher_overflow)) {
			list_for_each_entry(tmp, &wq->flusher_overflow, list)
				tmp->flush_color = wq->work_color;

			wq->work_color = work_next_color(wq->work_color);

			list_splice_tail_init(&wq->flusher_overflow,
					      &wq->flusher_queue);
			flush_workqueue_prep_cwqs(wq, -1, wq->work_color);
		}

		if (list_empty(&wq->flusher_queue)) {
			BUG_ON(wq->flush_color != wq->work_color);
			break;
		}

		BUG_ON(wq->flush_color == wq->work_color);
		BUG_ON(wq->flush_color != next->flush_color);

		list_del_init(&next->list);
		wq->first_flusher = next;

		if (flush_workqueue_prep_cwqs(wq, wq->flush_color, -1))
			break;

		wq->first_flusher = NULL;
	}

out_unlock:
	mutex_unlock(&wq->flush_mutex);
}
EXPORT_SYMBOL_GPL(flush_workqueue);

void drain_workqueue(struct workqueue_struct *wq)
{
	unsigned int flush_cnt = 0;
	unsigned int cpu;

	spin_lock(&workqueue_lock);
	if (!wq->nr_drainers++)
		wq->flags |= WQ_DRAINING;
	spin_unlock(&workqueue_lock);
reflush:
	flush_workqueue(wq);

	for_each_cwq_cpu(cpu, wq) {
		struct cpu_workqueue_struct *cwq = get_cwq(cpu, wq);
		bool drained;

		spin_lock_irq(&cwq->gcwq->lock);
		drained = !cwq->nr_active && list_empty(&cwq->delayed_works);
		spin_unlock_irq(&cwq->gcwq->lock);

		if (drained)
			continue;

		if (++flush_cnt == 10 ||
		    (flush_cnt % 100 == 0 && flush_cnt <= 1000))
			pr_warning("workqueue %s: flush on destruction isn't complete after %u tries\n",
				   wq->name, flush_cnt);
		goto reflush;
	}

	spin_lock(&workqueue_lock);
	if (!--wq->nr_drainers)
		wq->flags &= ~WQ_DRAINING;
	spin_unlock(&workqueue_lock);
}
EXPORT_SYMBOL_GPL(drain_workqueue);

static bool start_flush_work(struct work_struct *work, struct wq_barrier *barr,
			     bool wait_executing)
{
	struct worker *worker = NULL;
	struct global_cwq *gcwq;
	struct cpu_workqueue_struct *cwq;

	might_sleep();
	gcwq = get_work_gcwq(work);
	if (!gcwq)
		return false;

	spin_lock_irq(&gcwq->lock);
	if (!list_empty(&work->entry)) {
		smp_rmb();
		cwq = get_work_cwq(work);
		if (unlikely(!cwq || gcwq != cwq->gcwq))
			goto already_gone;
	} else if (wait_executing) {
		worker = find_worker_executing_work(gcwq, work);
		if (!worker)
			goto already_gone;
		cwq = worker->current_cwq;
	} else
		goto already_gone;

	insert_wq_barrier(cwq, barr, work, worker);
	spin_unlock_irq(&gcwq->lock);

	if (cwq->wq->saved_max_active == 1 || cwq->wq->flags & WQ_RESCUER)
		lock_map_acquire(&cwq->wq->lockdep_map);
	else
		lock_map_acquire_read(&cwq->wq->lockdep_map);
	lock_map_release(&cwq->wq->lockdep_map);

	return true;
already_gone:
	spin_unlock_irq(&gcwq->lock);
	return false;
}

bool flush_work(struct work_struct *work)
{
	struct wq_barrier barr;

	if (start_flush_work(work, &barr, true)) {
		wait_for_completion(&barr.done);
		destroy_work_on_stack(&barr.work);
		return true;
	} else
		return false;
}
EXPORT_SYMBOL_GPL(flush_work);

static bool wait_on_cpu_work(struct global_cwq *gcwq, struct work_struct *work)
{
	struct wq_barrier barr;
	struct worker *worker;

	spin_lock_irq(&gcwq->lock);

	worker = find_worker_executing_work(gcwq, work);
	if (unlikely(worker))
		insert_wq_barrier(worker->current_cwq, &barr, work, worker);

	spin_unlock_irq(&gcwq->lock);

	if (unlikely(worker)) {
		wait_for_completion(&barr.done);
		destroy_work_on_stack(&barr.work);
		return true;
	} else
		return false;
}

static bool wait_on_work(struct work_struct *work)
{
	bool ret = false;
	int cpu;

	might_sleep();

	lock_map_acquire(&work->lockdep_map);
	lock_map_release(&work->lockdep_map);

	for_each_gcwq_cpu(cpu)
		ret |= wait_on_cpu_work(get_gcwq(cpu), work);
	return ret;
}

bool flush_work_sync(struct work_struct *work)
{
	struct wq_barrier barr;
	bool pending, waited;

	
	pending = start_flush_work(work, &barr, false);

	
	waited = wait_on_work(work);

	
	if (pending) {
		wait_for_completion(&barr.done);
		destroy_work_on_stack(&barr.work);
	}

	return pending || waited;
}
EXPORT_SYMBOL_GPL(flush_work_sync);

static int try_to_grab_pending(struct work_struct *work)
{
	struct global_cwq *gcwq;
	int ret = -1;

	if (!test_and_set_bit(WORK_STRUCT_PENDING_BIT, work_data_bits(work)))
		return 0;

	gcwq = get_work_gcwq(work);
	if (!gcwq)
		return ret;

	spin_lock_irq(&gcwq->lock);
	if (!list_empty(&work->entry)) {
		smp_rmb();
		if (gcwq == get_work_gcwq(work)) {
			debug_work_deactivate(work);
			list_del_init(&work->entry);
			cwq_dec_nr_in_flight(get_work_cwq(work),
				get_work_color(work),
				*work_data_bits(work) & WORK_STRUCT_DELAYED);
			ret = 1;
		}
	}
	spin_unlock_irq(&gcwq->lock);

	return ret;
}

static bool __cancel_work_timer(struct work_struct *work,
				struct timer_list* timer)
{
	int ret;

	do {
		ret = (timer && likely(del_timer(timer)));
		if (!ret)
			ret = try_to_grab_pending(work);
		wait_on_work(work);
	} while (unlikely(ret < 0));

	clear_work_data(work);
	return ret;
}

bool cancel_work_sync(struct work_struct *work)
{
	return __cancel_work_timer(work, NULL);
}
EXPORT_SYMBOL_GPL(cancel_work_sync);

bool flush_delayed_work(struct delayed_work *dwork)
{
	if (del_timer_sync(&dwork->timer))
		__queue_work(raw_smp_processor_id(),
			     get_work_cwq(&dwork->work)->wq, &dwork->work);
	return flush_work(&dwork->work);
}
EXPORT_SYMBOL(flush_delayed_work);

bool flush_delayed_work_sync(struct delayed_work *dwork)
{
	if (del_timer_sync(&dwork->timer))
		__queue_work(raw_smp_processor_id(),
			     get_work_cwq(&dwork->work)->wq, &dwork->work);
	return flush_work_sync(&dwork->work);
}
EXPORT_SYMBOL(flush_delayed_work_sync);

bool cancel_delayed_work_sync(struct delayed_work *dwork)
{
	return __cancel_work_timer(&dwork->work, &dwork->timer);
}
EXPORT_SYMBOL(cancel_delayed_work_sync);

int schedule_work(struct work_struct *work)
{
	return queue_work(system_wq, work);
}
EXPORT_SYMBOL(schedule_work);

int schedule_work_on(int cpu, struct work_struct *work)
{
	return queue_work_on(cpu, system_wq, work);
}
EXPORT_SYMBOL(schedule_work_on);

int schedule_delayed_work(struct delayed_work *dwork,
					unsigned long delay)
{
	return queue_delayed_work(system_wq, dwork, delay);
}
EXPORT_SYMBOL(schedule_delayed_work);

int schedule_delayed_work_on(int cpu,
			struct delayed_work *dwork, unsigned long delay)
{
	return queue_delayed_work_on(cpu, system_wq, dwork, delay);
}
EXPORT_SYMBOL(schedule_delayed_work_on);

int schedule_on_each_cpu(work_func_t func)
{
	int cpu;
	struct work_struct __percpu *works;

	works = alloc_percpu(struct work_struct);
	if (!works)
		return -ENOMEM;

	get_online_cpus();

	for_each_online_cpu(cpu) {
		struct work_struct *work = per_cpu_ptr(works, cpu);

		INIT_WORK(work, func);
		schedule_work_on(cpu, work);
	}

	for_each_online_cpu(cpu)
		flush_work(per_cpu_ptr(works, cpu));

	put_online_cpus();
	free_percpu(works);
	return 0;
}

void flush_scheduled_work(void)
{
	flush_workqueue(system_wq);
}
EXPORT_SYMBOL(flush_scheduled_work);

int execute_in_process_context(work_func_t fn, struct execute_work *ew)
{
	if (!in_interrupt()) {
		fn(&ew->work);
		return 0;
	}

	INIT_WORK(&ew->work, fn);
	schedule_work(&ew->work);

	return 1;
}
EXPORT_SYMBOL_GPL(execute_in_process_context);

int keventd_up(void)
{
	return system_wq != NULL;
}

static int alloc_cwqs(struct workqueue_struct *wq)
{
	const size_t size = sizeof(struct cpu_workqueue_struct);
	const size_t align = max_t(size_t, 1 << WORK_STRUCT_FLAG_BITS,
				   __alignof__(unsigned long long));

	if (!(wq->flags & WQ_UNBOUND))
		wq->cpu_wq.pcpu = __alloc_percpu(size, align);
	else {
		void *ptr;

		ptr = kzalloc(size + align + sizeof(void *), GFP_KERNEL);
		if (ptr) {
			wq->cpu_wq.single = PTR_ALIGN(ptr, align);
			*(void **)(wq->cpu_wq.single + 1) = ptr;
		}
	}

	
	BUG_ON(!IS_ALIGNED(wq->cpu_wq.v, align));
	return wq->cpu_wq.v ? 0 : -ENOMEM;
}

static void free_cwqs(struct workqueue_struct *wq)
{
	if (!(wq->flags & WQ_UNBOUND))
		free_percpu(wq->cpu_wq.pcpu);
	else if (wq->cpu_wq.single) {
		
		kfree(*(void **)(wq->cpu_wq.single + 1));
	}
}

static int wq_clamp_max_active(int max_active, unsigned int flags,
			       const char *name)
{
	int lim = flags & WQ_UNBOUND ? WQ_UNBOUND_MAX_ACTIVE : WQ_MAX_ACTIVE;

	if (max_active < 1 || max_active > lim)
		printk(KERN_WARNING "workqueue: max_active %d requested for %s "
		       "is out of range, clamping between %d and %d\n",
		       max_active, name, 1, lim);

	return clamp_val(max_active, 1, lim);
}

struct workqueue_struct *__alloc_workqueue_key(const char *fmt,
					       unsigned int flags,
					       int max_active,
					       struct lock_class_key *key,
					       const char *lock_name, ...)
{
	va_list args, args1;
	struct workqueue_struct *wq;
	unsigned int cpu;
	size_t namelen;

	
	va_start(args, lock_name);
	va_copy(args1, args);
	namelen = vsnprintf(NULL, 0, fmt, args) + 1;

	wq = kzalloc(sizeof(*wq) + namelen, GFP_KERNEL);
	if (!wq)
		goto err;

	vsnprintf(wq->name, namelen, fmt, args1);
	va_end(args);
	va_end(args1);

	if (flags & WQ_MEM_RECLAIM)
		flags |= WQ_RESCUER;

	if (flags & WQ_UNBOUND)
		flags |= WQ_HIGHPRI;

	max_active = max_active ?: WQ_DFL_ACTIVE;
	max_active = wq_clamp_max_active(max_active, flags, wq->name);

	
	wq->flags = flags;
	wq->saved_max_active = max_active;
	mutex_init(&wq->flush_mutex);
	atomic_set(&wq->nr_cwqs_to_flush, 0);
	INIT_LIST_HEAD(&wq->flusher_queue);
	INIT_LIST_HEAD(&wq->flusher_overflow);

	lockdep_init_map(&wq->lockdep_map, lock_name, key, 0);
	INIT_LIST_HEAD(&wq->list);

	if (alloc_cwqs(wq) < 0)
		goto err;

	for_each_cwq_cpu(cpu, wq) {
		struct cpu_workqueue_struct *cwq = get_cwq(cpu, wq);
		struct global_cwq *gcwq = get_gcwq(cpu);

		BUG_ON((unsigned long)cwq & WORK_STRUCT_FLAG_MASK);
		cwq->gcwq = gcwq;
		cwq->wq = wq;
		cwq->flush_color = -1;
		cwq->max_active = max_active;
		INIT_LIST_HEAD(&cwq->delayed_works);
	}

	if (flags & WQ_RESCUER) {
		struct worker *rescuer;

		if (!alloc_mayday_mask(&wq->mayday_mask, GFP_KERNEL))
			goto err;

		wq->rescuer = rescuer = alloc_worker();
		if (!rescuer)
			goto err;

		rescuer->task = kthread_create(rescuer_thread, wq, "%s",
					       wq->name);
		if (IS_ERR(rescuer->task))
			goto err;

		rescuer->task->flags |= PF_THREAD_BOUND;
		wake_up_process(rescuer->task);
	}

	spin_lock(&workqueue_lock);

	if (workqueue_freezing && wq->flags & WQ_FREEZABLE)
		for_each_cwq_cpu(cpu, wq)
			get_cwq(cpu, wq)->max_active = 0;

	list_add(&wq->list, &workqueues);

	spin_unlock(&workqueue_lock);

	return wq;
err:
	if (wq) {
		free_cwqs(wq);
		free_mayday_mask(wq->mayday_mask);
		kfree(wq->rescuer);
		kfree(wq);
	}
	return NULL;
}
EXPORT_SYMBOL_GPL(__alloc_workqueue_key);

void destroy_workqueue(struct workqueue_struct *wq)
{
	unsigned int cpu;

	
	drain_workqueue(wq);

	spin_lock(&workqueue_lock);
	list_del(&wq->list);
	spin_unlock(&workqueue_lock);

	
	for_each_cwq_cpu(cpu, wq) {
		struct cpu_workqueue_struct *cwq = get_cwq(cpu, wq);
		int i;

		for (i = 0; i < WORK_NR_COLORS; i++)
			BUG_ON(cwq->nr_in_flight[i]);
		BUG_ON(cwq->nr_active);
		BUG_ON(!list_empty(&cwq->delayed_works));
	}

	if (wq->flags & WQ_RESCUER) {
		kthread_stop(wq->rescuer->task);
		free_mayday_mask(wq->mayday_mask);
		kfree(wq->rescuer);
	}

	free_cwqs(wq);
	kfree(wq);
}
EXPORT_SYMBOL_GPL(destroy_workqueue);

void workqueue_set_max_active(struct workqueue_struct *wq, int max_active)
{
	unsigned int cpu;

	max_active = wq_clamp_max_active(max_active, wq->flags, wq->name);

	spin_lock(&workqueue_lock);

	wq->saved_max_active = max_active;

	for_each_cwq_cpu(cpu, wq) {
		struct global_cwq *gcwq = get_gcwq(cpu);

		spin_lock_irq(&gcwq->lock);

		if (!(wq->flags & WQ_FREEZABLE) ||
		    !(gcwq->flags & GCWQ_FREEZING))
			get_cwq(gcwq->cpu, wq)->max_active = max_active;

		spin_unlock_irq(&gcwq->lock);
	}

	spin_unlock(&workqueue_lock);
}
EXPORT_SYMBOL_GPL(workqueue_set_max_active);

bool workqueue_congested(unsigned int cpu, struct workqueue_struct *wq)
{
	struct cpu_workqueue_struct *cwq = get_cwq(cpu, wq);

	return !list_empty(&cwq->delayed_works);
}
EXPORT_SYMBOL_GPL(workqueue_congested);

unsigned int work_cpu(struct work_struct *work)
{
	struct global_cwq *gcwq = get_work_gcwq(work);

	return gcwq ? gcwq->cpu : WORK_CPU_NONE;
}
EXPORT_SYMBOL_GPL(work_cpu);

unsigned int work_busy(struct work_struct *work)
{
	struct global_cwq *gcwq = get_work_gcwq(work);
	unsigned long flags;
	unsigned int ret = 0;

	if (!gcwq)
		return false;

	spin_lock_irqsave(&gcwq->lock, flags);

	if (work_pending(work))
		ret |= WORK_BUSY_PENDING;
	if (find_worker_executing_work(gcwq, work))
		ret |= WORK_BUSY_RUNNING;

	spin_unlock_irqrestore(&gcwq->lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(work_busy);


#define trustee_wait_event_timeout(cond, timeout) ({			\
	long __ret = (timeout);						\
	while (!((cond) || (gcwq->trustee_state == TRUSTEE_RELEASE)) &&	\
	       __ret) {							\
		spin_unlock_irq(&gcwq->lock);				\
		__wait_event_timeout(gcwq->trustee_wait, (cond) ||	\
			(gcwq->trustee_state == TRUSTEE_RELEASE),	\
			__ret);						\
		spin_lock_irq(&gcwq->lock);				\
	}								\
	gcwq->trustee_state == TRUSTEE_RELEASE ? -1 : (__ret);		\
})

#define trustee_wait_event(cond) ({					\
	long __ret1;							\
	__ret1 = trustee_wait_event_timeout(cond, MAX_SCHEDULE_TIMEOUT);\
	__ret1 < 0 ? -1 : 0;						\
})

static int __cpuinit trustee_thread(void *__gcwq)
{
	struct global_cwq *gcwq = __gcwq;
	struct worker *worker;
	struct work_struct *work;
	struct hlist_node *pos;
	long rc;
	int i;

	BUG_ON(gcwq->cpu != smp_processor_id());

	spin_lock_irq(&gcwq->lock);
	BUG_ON(gcwq->cpu != smp_processor_id());
	rc = trustee_wait_event(!(gcwq->flags & GCWQ_MANAGING_WORKERS));
	BUG_ON(rc < 0);

	gcwq->flags |= GCWQ_MANAGING_WORKERS;

	list_for_each_entry(worker, &gcwq->idle_list, entry)
		worker->flags |= WORKER_ROGUE;

	for_each_busy_worker(worker, i, pos, gcwq)
		worker->flags |= WORKER_ROGUE;

	spin_unlock_irq(&gcwq->lock);
	schedule();
	spin_lock_irq(&gcwq->lock);

	atomic_set(get_gcwq_nr_running(gcwq->cpu), 0);

	spin_unlock_irq(&gcwq->lock);
	del_timer_sync(&gcwq->idle_timer);
	spin_lock_irq(&gcwq->lock);

	gcwq->trustee_state = TRUSTEE_IN_CHARGE;
	wake_up_all(&gcwq->trustee_wait);

	while (gcwq->nr_workers != gcwq->nr_idle ||
	       gcwq->flags & GCWQ_FREEZING ||
	       gcwq->trustee_state == TRUSTEE_IN_CHARGE) {
		int nr_works = 0;

		list_for_each_entry(work, &gcwq->worklist, entry) {
			send_mayday(work);
			nr_works++;
		}

		list_for_each_entry(worker, &gcwq->idle_list, entry) {
			if (!nr_works--)
				break;
			wake_up_process(worker->task);
		}

		if (need_to_create_worker(gcwq)) {
			spin_unlock_irq(&gcwq->lock);
			worker = create_worker(gcwq, false);
			spin_lock_irq(&gcwq->lock);
			if (worker) {
				worker->flags |= WORKER_ROGUE;
				start_worker(worker);
			}
		}

		
		if (trustee_wait_event_timeout(false, TRUSTEE_COOLDOWN) < 0)
			break;
	}

	do {
		rc = trustee_wait_event(!list_empty(&gcwq->idle_list));
		while (!list_empty(&gcwq->idle_list))
			destroy_worker(list_first_entry(&gcwq->idle_list,
							struct worker, entry));
	} while (gcwq->nr_workers && rc >= 0);

	WARN_ON(!list_empty(&gcwq->idle_list));

	for_each_busy_worker(worker, i, pos, gcwq) {
		struct work_struct *rebind_work = &worker->rebind_work;

		worker->flags |= WORKER_REBIND;
		worker->flags &= ~WORKER_ROGUE;

		
		if (test_and_set_bit(WORK_STRUCT_PENDING_BIT,
				     work_data_bits(rebind_work)))
			continue;

		debug_work_activate(rebind_work);
		insert_work(get_cwq(gcwq->cpu, system_wq), rebind_work,
			    worker->scheduled.next,
			    work_color_to_flags(WORK_NO_COLOR));
	}

	
	gcwq->flags &= ~GCWQ_MANAGING_WORKERS;

	
	gcwq->trustee = NULL;
	gcwq->trustee_state = TRUSTEE_DONE;
	wake_up_all(&gcwq->trustee_wait);
	spin_unlock_irq(&gcwq->lock);
	return 0;
}

static void __cpuinit wait_trustee_state(struct global_cwq *gcwq, int state)
__releases(&gcwq->lock)
__acquires(&gcwq->lock)
{
	if (!(gcwq->trustee_state == state ||
	      gcwq->trustee_state == TRUSTEE_DONE)) {
		spin_unlock_irq(&gcwq->lock);
		__wait_event(gcwq->trustee_wait,
			     gcwq->trustee_state == state ||
			     gcwq->trustee_state == TRUSTEE_DONE);
		spin_lock_irq(&gcwq->lock);
	}
}

static int __devinit workqueue_cpu_callback(struct notifier_block *nfb,
						unsigned long action,
						void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	struct global_cwq *gcwq = get_gcwq(cpu);
	struct task_struct *new_trustee = NULL;
	struct worker *uninitialized_var(new_worker);
	unsigned long flags;

	action &= ~CPU_TASKS_FROZEN;

	switch (action) {
	case CPU_DOWN_PREPARE:
		new_trustee = kthread_create(trustee_thread, gcwq,
					     "workqueue_trustee/%d\n", cpu);
		if (IS_ERR(new_trustee))
			return notifier_from_errno(PTR_ERR(new_trustee));
		kthread_bind(new_trustee, cpu);
		
	case CPU_UP_PREPARE:
		BUG_ON(gcwq->first_idle);
		new_worker = create_worker(gcwq, false);
		if (!new_worker) {
			if (new_trustee)
				kthread_stop(new_trustee);
			return NOTIFY_BAD;
		}
	}

	
	spin_lock_irqsave(&gcwq->lock, flags);

	switch (action) {
	case CPU_DOWN_PREPARE:
		
		BUG_ON(gcwq->trustee || gcwq->trustee_state != TRUSTEE_DONE);
		gcwq->trustee = new_trustee;
		gcwq->trustee_state = TRUSTEE_START;
		wake_up_process(gcwq->trustee);
		wait_trustee_state(gcwq, TRUSTEE_IN_CHARGE);
		
	case CPU_UP_PREPARE:
		BUG_ON(gcwq->first_idle);
		gcwq->first_idle = new_worker;
		break;

	case CPU_DYING:
		gcwq->flags |= GCWQ_DISASSOCIATED;
		break;

	case CPU_POST_DEAD:
		gcwq->trustee_state = TRUSTEE_BUTCHER;
		
	case CPU_UP_CANCELED:
		destroy_worker(gcwq->first_idle);
		gcwq->first_idle = NULL;
		break;

	case CPU_DOWN_FAILED:
	case CPU_ONLINE:
		gcwq->flags &= ~GCWQ_DISASSOCIATED;
		if (gcwq->trustee_state != TRUSTEE_DONE) {
			gcwq->trustee_state = TRUSTEE_RELEASE;
			wake_up_process(gcwq->trustee);
			wait_trustee_state(gcwq, TRUSTEE_DONE);
		}

		spin_unlock_irq(&gcwq->lock);
		kthread_bind(gcwq->first_idle->task, cpu);
		spin_lock_irq(&gcwq->lock);
		gcwq->flags |= GCWQ_MANAGE_WORKERS;
		start_worker(gcwq->first_idle);
		gcwq->first_idle = NULL;
		break;
	}

	spin_unlock_irqrestore(&gcwq->lock, flags);

	return notifier_from_errno(0);
}

static int __devinit workqueue_cpu_up_callback(struct notifier_block *nfb,
					       unsigned long action,
					       void *hcpu)
{
	switch (action & ~CPU_TASKS_FROZEN) {
	case CPU_UP_PREPARE:
	case CPU_UP_CANCELED:
	case CPU_DOWN_FAILED:
	case CPU_ONLINE:
		return workqueue_cpu_callback(nfb, action, hcpu);
	}
	return NOTIFY_OK;
}

static int __devinit workqueue_cpu_down_callback(struct notifier_block *nfb,
						 unsigned long action,
						 void *hcpu)
{
	switch (action & ~CPU_TASKS_FROZEN) {
	case CPU_DOWN_PREPARE:
	case CPU_DYING:
	case CPU_POST_DEAD:
		return workqueue_cpu_callback(nfb, action, hcpu);
	}
	return NOTIFY_OK;
}

#ifdef CONFIG_SMP

struct work_for_cpu {
	struct completion completion;
	long (*fn)(void *);
	void *arg;
	long ret;
};

static int do_work_for_cpu(void *_wfc)
{
	struct work_for_cpu *wfc = _wfc;
	wfc->ret = wfc->fn(wfc->arg);
	complete(&wfc->completion);
	return 0;
}

long work_on_cpu(unsigned int cpu, long (*fn)(void *), void *arg)
{
	struct task_struct *sub_thread;
	struct work_for_cpu wfc = {
		.completion = COMPLETION_INITIALIZER_ONSTACK(wfc.completion),
		.fn = fn,
		.arg = arg,
	};

	sub_thread = kthread_create(do_work_for_cpu, &wfc, "work_for_cpu");
	if (IS_ERR(sub_thread))
		return PTR_ERR(sub_thread);
	kthread_bind(sub_thread, cpu);
	wake_up_process(sub_thread);
	wait_for_completion(&wfc.completion);
	return wfc.ret;
}
EXPORT_SYMBOL_GPL(work_on_cpu);
#endif 

#ifdef CONFIG_FREEZER

void freeze_workqueues_begin(void)
{
	unsigned int cpu;

	spin_lock(&workqueue_lock);

	BUG_ON(workqueue_freezing);
	workqueue_freezing = true;

	for_each_gcwq_cpu(cpu) {
		struct global_cwq *gcwq = get_gcwq(cpu);
		struct workqueue_struct *wq;

		spin_lock_irq(&gcwq->lock);

		BUG_ON(gcwq->flags & GCWQ_FREEZING);
		gcwq->flags |= GCWQ_FREEZING;

		list_for_each_entry(wq, &workqueues, list) {
			struct cpu_workqueue_struct *cwq = get_cwq(cpu, wq);

			if (cwq && wq->flags & WQ_FREEZABLE)
				cwq->max_active = 0;
		}

		spin_unlock_irq(&gcwq->lock);
	}

	spin_unlock(&workqueue_lock);
}

bool freeze_workqueues_busy(void)
{
	unsigned int cpu;
	bool busy = false;

	spin_lock(&workqueue_lock);

	BUG_ON(!workqueue_freezing);

	for_each_gcwq_cpu(cpu) {
		struct workqueue_struct *wq;
		list_for_each_entry(wq, &workqueues, list) {
			struct cpu_workqueue_struct *cwq = get_cwq(cpu, wq);

			if (!cwq || !(wq->flags & WQ_FREEZABLE))
				continue;

			BUG_ON(cwq->nr_active < 0);
			if (cwq->nr_active) {
				busy = true;
				goto out_unlock;
			}
		}
	}
out_unlock:
	spin_unlock(&workqueue_lock);
	return busy;
}

void thaw_workqueues(void)
{
	unsigned int cpu;

	spin_lock(&workqueue_lock);

	if (!workqueue_freezing)
		goto out_unlock;

	for_each_gcwq_cpu(cpu) {
		struct global_cwq *gcwq = get_gcwq(cpu);
		struct workqueue_struct *wq;

		spin_lock_irq(&gcwq->lock);

		BUG_ON(!(gcwq->flags & GCWQ_FREEZING));
		gcwq->flags &= ~GCWQ_FREEZING;

		list_for_each_entry(wq, &workqueues, list) {
			struct cpu_workqueue_struct *cwq = get_cwq(cpu, wq);

			if (!cwq || !(wq->flags & WQ_FREEZABLE))
				continue;

			
			cwq->max_active = wq->saved_max_active;

			while (!list_empty(&cwq->delayed_works) &&
			       cwq->nr_active < cwq->max_active)
				cwq_activate_first_delayed(cwq);
		}

		wake_up_worker(gcwq);

		spin_unlock_irq(&gcwq->lock);
	}

	workqueue_freezing = false;
out_unlock:
	spin_unlock(&workqueue_lock);
}
#endif 

unsigned long get_work_func_of_task_struct(struct task_struct *tsk)
{
	unsigned int cpu;

	for_each_gcwq_cpu(cpu) {
		struct global_cwq *gcwq = get_gcwq(cpu);
		struct worker *worker;
		struct hlist_node *pos;
		int i;

		for_each_busy_worker(worker, i, pos, gcwq) {
			if (worker->task == tsk)
				return (unsigned long)worker->current_work->func;
		}
	}
	return 0;
}

void show_pending_work_on_gcwq(void)
{
	struct work_struct *work;
	unsigned int cpu;

	for_each_gcwq_cpu(cpu) {
		struct global_cwq *gcwq = get_gcwq(cpu);

		list_for_each_entry(work, &gcwq->worklist, entry) {
			printk("CPU%d pending work : %pf\n", cpu, work->func);
		}
	}
}
EXPORT_SYMBOL(show_pending_work_on_gcwq);

static int __init init_workqueues(void)
{
	unsigned int cpu;
	int i;

	cpu_notifier(workqueue_cpu_up_callback, CPU_PRI_WORKQUEUE_UP);
	cpu_notifier(workqueue_cpu_down_callback, CPU_PRI_WORKQUEUE_DOWN);

	
	for_each_gcwq_cpu(cpu) {
		struct global_cwq *gcwq = get_gcwq(cpu);

		spin_lock_init(&gcwq->lock);
		INIT_LIST_HEAD(&gcwq->worklist);
		gcwq->cpu = cpu;
		gcwq->flags |= GCWQ_DISASSOCIATED;

		INIT_LIST_HEAD(&gcwq->idle_list);
		for (i = 0; i < BUSY_WORKER_HASH_SIZE; i++)
			INIT_HLIST_HEAD(&gcwq->busy_hash[i]);

		init_timer_deferrable(&gcwq->idle_timer);
		gcwq->idle_timer.function = idle_worker_timeout;
		gcwq->idle_timer.data = (unsigned long)gcwq;

		setup_timer(&gcwq->mayday_timer, gcwq_mayday_timeout,
			    (unsigned long)gcwq);

		ida_init(&gcwq->worker_ida);

		gcwq->trustee_state = TRUSTEE_DONE;
		init_waitqueue_head(&gcwq->trustee_wait);
	}

	
	for_each_online_gcwq_cpu(cpu) {
		struct global_cwq *gcwq = get_gcwq(cpu);
		struct worker *worker;

		if (cpu != WORK_CPU_UNBOUND)
			gcwq->flags &= ~GCWQ_DISASSOCIATED;
		worker = create_worker(gcwq, true);
		BUG_ON(!worker);
		spin_lock_irq(&gcwq->lock);
		start_worker(worker);
		spin_unlock_irq(&gcwq->lock);
	}


#ifdef CONFIG_TRACING_WORKQUEUE_HISTORY
	if (wq_history_flag)
		memset(wq_history_flag, 0, (WORKQUEUE_HISTORY_ELEMENT_SIZE * WQ_HIST_LEN));
#endif

	system_wq = alloc_workqueue("events", 0, 0);
	system_long_wq = alloc_workqueue("events_long", 0, 0);
	system_nrt_wq = alloc_workqueue("events_nrt", WQ_NON_REENTRANT, 0);
	system_unbound_wq = alloc_workqueue("events_unbound", WQ_UNBOUND,
					    WQ_UNBOUND_MAX_ACTIVE);
	system_freezable_wq = alloc_workqueue("events_freezable",
					      WQ_FREEZABLE, 0);
	system_nrt_freezable_wq = alloc_workqueue("events_nrt_freezable",
			WQ_NON_REENTRANT | WQ_FREEZABLE, 0);
	BUG_ON(!system_wq || !system_long_wq || !system_nrt_wq ||
	       !system_unbound_wq || !system_freezable_wq ||
		!system_nrt_freezable_wq);
	return 0;
}
early_initcall(init_workqueues);
