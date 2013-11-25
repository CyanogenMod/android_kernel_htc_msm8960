/*
 * Read-Copy Update mechanism for mutual exclusion (tree-based version)
 * Internal non-public definitions that provide either classic
 * or preemptible semantics.
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
 * Copyright Red Hat, 2009
 * Copyright IBM Corporation, 2009
 *
 * Author: Ingo Molnar <mingo@elte.hu>
 *	   Paul E. McKenney <paulmck@linux.vnet.ibm.com>
 */

#include <linux/delay.h>

#define RCU_KTHREAD_PRIO 1

#ifdef CONFIG_RCU_BOOST
#define RCU_BOOST_PRIO CONFIG_RCU_BOOST_PRIO
#else
#define RCU_BOOST_PRIO RCU_KTHREAD_PRIO
#endif

static void __init rcu_bootup_announce_oddness(void)
{
#ifdef CONFIG_RCU_TRACE
	printk(KERN_INFO "\tRCU debugfs-based tracing is enabled.\n");
#endif
#if (defined(CONFIG_64BIT) && CONFIG_RCU_FANOUT != 64) || (!defined(CONFIG_64BIT) && CONFIG_RCU_FANOUT != 32)
	printk(KERN_INFO "\tCONFIG_RCU_FANOUT set to non-default value of %d\n",
	       CONFIG_RCU_FANOUT);
#endif
#ifdef CONFIG_RCU_FANOUT_EXACT
	printk(KERN_INFO "\tHierarchical RCU autobalancing is disabled.\n");
#endif
#ifdef CONFIG_RCU_FAST_NO_HZ
	printk(KERN_INFO
	       "\tRCU dyntick-idle grace-period acceleration is enabled.\n");
#endif
#ifdef CONFIG_PROVE_RCU
	printk(KERN_INFO "\tRCU lockdep checking is enabled.\n");
#endif
#ifdef CONFIG_RCU_TORTURE_TEST_RUNNABLE
	printk(KERN_INFO "\tRCU torture testing starts during boot.\n");
#endif
#if defined(CONFIG_TREE_PREEMPT_RCU) && !defined(CONFIG_RCU_CPU_STALL_VERBOSE)
	printk(KERN_INFO "\tDump stacks of tasks blocking RCU-preempt GP.\n");
#endif
#if defined(CONFIG_RCU_CPU_STALL_INFO)
	printk(KERN_INFO "\tAdditional per-CPU info printed with stalls.\n");
#endif
#if NUM_RCU_LVL_4 != 0
	printk(KERN_INFO "\tExperimental four-level hierarchy is enabled.\n");
#endif
}

#ifdef CONFIG_TREE_PREEMPT_RCU

struct rcu_state rcu_preempt_state = RCU_STATE_INITIALIZER(rcu_preempt);
DEFINE_PER_CPU(struct rcu_data, rcu_preempt_data);
static struct rcu_state *rcu_state = &rcu_preempt_state;

static void rcu_read_unlock_special(struct task_struct *t);
static int rcu_preempted_readers_exp(struct rcu_node *rnp);

static void __init rcu_bootup_announce(void)
{
	printk(KERN_INFO "Preemptible hierarchical RCU implementation.\n");
	rcu_bootup_announce_oddness();
}

long rcu_batches_completed_preempt(void)
{
	return rcu_preempt_state.completed;
}
EXPORT_SYMBOL_GPL(rcu_batches_completed_preempt);

long rcu_batches_completed(void)
{
	return rcu_batches_completed_preempt();
}
EXPORT_SYMBOL_GPL(rcu_batches_completed);

void rcu_force_quiescent_state(void)
{
	force_quiescent_state(&rcu_preempt_state, 0);
}
EXPORT_SYMBOL_GPL(rcu_force_quiescent_state);

static void rcu_preempt_qs(int cpu)
{
	struct rcu_data *rdp = &per_cpu(rcu_preempt_data, cpu);

	rdp->passed_quiesce_gpnum = rdp->gpnum;
	barrier();
	if (rdp->passed_quiesce == 0)
		trace_rcu_grace_period("rcu_preempt", rdp->gpnum, "cpuqs");
	rdp->passed_quiesce = 1;
	current->rcu_read_unlock_special &= ~RCU_READ_UNLOCK_NEED_QS;
}

static void rcu_preempt_note_context_switch(int cpu)
{
	struct task_struct *t = current;
	unsigned long flags;
	struct rcu_data *rdp;
	struct rcu_node *rnp;

	if (t->rcu_read_lock_nesting > 0 &&
	    (t->rcu_read_unlock_special & RCU_READ_UNLOCK_BLOCKED) == 0) {

		
		rdp = per_cpu_ptr(rcu_preempt_state.rda, cpu);
		rnp = rdp->mynode;
		raw_spin_lock_irqsave(&rnp->lock, flags);
		t->rcu_read_unlock_special |= RCU_READ_UNLOCK_BLOCKED;
		t->rcu_blocked_node = rnp;

		WARN_ON_ONCE((rdp->grpmask & rnp->qsmaskinit) == 0);
		WARN_ON_ONCE(!list_empty(&t->rcu_node_entry));
		if ((rnp->qsmask & rdp->grpmask) && rnp->gp_tasks != NULL) {
			list_add(&t->rcu_node_entry, rnp->gp_tasks->prev);
			rnp->gp_tasks = &t->rcu_node_entry;
#ifdef CONFIG_RCU_BOOST
			if (rnp->boost_tasks != NULL)
				rnp->boost_tasks = rnp->gp_tasks;
#endif 
		} else {
			list_add(&t->rcu_node_entry, &rnp->blkd_tasks);
			if (rnp->qsmask & rdp->grpmask)
				rnp->gp_tasks = &t->rcu_node_entry;
		}
		trace_rcu_preempt_task(rdp->rsp->name,
				       t->pid,
				       (rnp->qsmask & rdp->grpmask)
				       ? rnp->gpnum
				       : rnp->gpnum + 1);
		raw_spin_unlock_irqrestore(&rnp->lock, flags);
	} else if (t->rcu_read_lock_nesting < 0 &&
		   t->rcu_read_unlock_special) {

		rcu_read_unlock_special(t);
	}

	local_irq_save(flags);
	rcu_preempt_qs(cpu);
	local_irq_restore(flags);
}

void __rcu_read_lock(void)
{
	current->rcu_read_lock_nesting++;
	barrier();  
}
EXPORT_SYMBOL_GPL(__rcu_read_lock);

static int rcu_preempt_blocked_readers_cgp(struct rcu_node *rnp)
{
	return rnp->gp_tasks != NULL;
}

static void rcu_report_unblock_qs_rnp(struct rcu_node *rnp, unsigned long flags)
	__releases(rnp->lock)
{
	unsigned long mask;
	struct rcu_node *rnp_p;

	if (rnp->qsmask != 0 || rcu_preempt_blocked_readers_cgp(rnp)) {
		raw_spin_unlock_irqrestore(&rnp->lock, flags);
		return;  
	}

	rnp_p = rnp->parent;
	if (rnp_p == NULL) {
		rcu_report_qs_rsp(&rcu_preempt_state, flags);
		return;
	}

	
	mask = rnp->grpmask;
	raw_spin_unlock(&rnp->lock);	
	raw_spin_lock(&rnp_p->lock);	
	rcu_report_qs_rnp(mask, &rcu_preempt_state, rnp_p, flags);
}

static struct list_head *rcu_next_node_entry(struct task_struct *t,
					     struct rcu_node *rnp)
{
	struct list_head *np;

	np = t->rcu_node_entry.next;
	if (np == &rnp->blkd_tasks)
		np = NULL;
	return np;
}

static noinline void rcu_read_unlock_special(struct task_struct *t)
{
	int empty;
	int empty_exp;
	int empty_exp_now;
	unsigned long flags;
	struct list_head *np;
#ifdef CONFIG_RCU_BOOST
	struct rt_mutex *rbmp = NULL;
#endif 
	struct rcu_node *rnp;
	int special;

	
	if (in_nmi())
		return;

	local_irq_save(flags);

	special = t->rcu_read_unlock_special;
	if (special & RCU_READ_UNLOCK_NEED_QS) {
		rcu_preempt_qs(smp_processor_id());
	}

	
	if (in_irq() || in_serving_softirq()) {
		local_irq_restore(flags);
		return;
	}

	
	if (special & RCU_READ_UNLOCK_BLOCKED) {
		t->rcu_read_unlock_special &= ~RCU_READ_UNLOCK_BLOCKED;

		for (;;) {
			rnp = t->rcu_blocked_node;
			raw_spin_lock(&rnp->lock);  
			if (rnp == t->rcu_blocked_node)
				break;
			raw_spin_unlock(&rnp->lock); 
		}
		empty = !rcu_preempt_blocked_readers_cgp(rnp);
		empty_exp = !rcu_preempted_readers_exp(rnp);
		smp_mb(); 
		np = rcu_next_node_entry(t, rnp);
		list_del_init(&t->rcu_node_entry);
		t->rcu_blocked_node = NULL;
		trace_rcu_unlock_preempted_task("rcu_preempt",
						rnp->gpnum, t->pid);
		if (&t->rcu_node_entry == rnp->gp_tasks)
			rnp->gp_tasks = np;
		if (&t->rcu_node_entry == rnp->exp_tasks)
			rnp->exp_tasks = np;
#ifdef CONFIG_RCU_BOOST
		if (&t->rcu_node_entry == rnp->boost_tasks)
			rnp->boost_tasks = np;
		
		if (t->rcu_boost_mutex) {
			rbmp = t->rcu_boost_mutex;
			t->rcu_boost_mutex = NULL;
		}
#endif 

		empty_exp_now = !rcu_preempted_readers_exp(rnp);
		if (!empty && !rcu_preempt_blocked_readers_cgp(rnp)) {
			trace_rcu_quiescent_state_report("preempt_rcu",
							 rnp->gpnum,
							 0, rnp->qsmask,
							 rnp->level,
							 rnp->grplo,
							 rnp->grphi,
							 !!rnp->gp_tasks);
			rcu_report_unblock_qs_rnp(rnp, flags);
		} else
			raw_spin_unlock_irqrestore(&rnp->lock, flags);

#ifdef CONFIG_RCU_BOOST
		
		if (rbmp)
			rt_mutex_unlock(rbmp);
#endif 

		if (!empty_exp && empty_exp_now)
			rcu_report_exp_rnp(&rcu_preempt_state, rnp, true);
	} else {
		local_irq_restore(flags);
	}
}

void __rcu_read_unlock(void)
{
	struct task_struct *t = current;

	if (t->rcu_read_lock_nesting != 1)
		--t->rcu_read_lock_nesting;
	else {
		barrier();  
		t->rcu_read_lock_nesting = INT_MIN;
		barrier();  
		if (unlikely(ACCESS_ONCE(t->rcu_read_unlock_special)))
			rcu_read_unlock_special(t);
		barrier();  
		t->rcu_read_lock_nesting = 0;
	}
#ifdef CONFIG_PROVE_LOCKING
	{
		int rrln = ACCESS_ONCE(t->rcu_read_lock_nesting);

		WARN_ON_ONCE(rrln < 0 && rrln > INT_MIN / 2);
	}
#endif 
}
EXPORT_SYMBOL_GPL(__rcu_read_unlock);

#ifdef CONFIG_RCU_CPU_STALL_VERBOSE

static void rcu_print_detail_task_stall_rnp(struct rcu_node *rnp)
{
	unsigned long flags;
	struct task_struct *t;

	if (!rcu_preempt_blocked_readers_cgp(rnp))
		return;
	raw_spin_lock_irqsave(&rnp->lock, flags);
	t = list_entry(rnp->gp_tasks,
		       struct task_struct, rcu_node_entry);
	list_for_each_entry_continue(t, &rnp->blkd_tasks, rcu_node_entry)
		sched_show_task(t);
	raw_spin_unlock_irqrestore(&rnp->lock, flags);
}

static void rcu_print_detail_task_stall(struct rcu_state *rsp)
{
	struct rcu_node *rnp = rcu_get_root(rsp);

	rcu_print_detail_task_stall_rnp(rnp);
	rcu_for_each_leaf_node(rsp, rnp)
		rcu_print_detail_task_stall_rnp(rnp);
}

#else 

static void rcu_print_detail_task_stall(struct rcu_state *rsp)
{
}

#endif 

#ifdef CONFIG_RCU_CPU_STALL_INFO

static void rcu_print_task_stall_begin(struct rcu_node *rnp)
{
	printk(KERN_ERR "\tTasks blocked on level-%d rcu_node (CPUs %d-%d):",
	       rnp->level, rnp->grplo, rnp->grphi);
}

static void rcu_print_task_stall_end(void)
{
	printk(KERN_CONT "\n");
}

#else 

static void rcu_print_task_stall_begin(struct rcu_node *rnp)
{
}

static void rcu_print_task_stall_end(void)
{
}

#endif 

static int rcu_print_task_stall(struct rcu_node *rnp)
{
	struct task_struct *t;
	int ndetected = 0;

	if (!rcu_preempt_blocked_readers_cgp(rnp))
		return 0;
	rcu_print_task_stall_begin(rnp);
	t = list_entry(rnp->gp_tasks,
		       struct task_struct, rcu_node_entry);
	list_for_each_entry_continue(t, &rnp->blkd_tasks, rcu_node_entry) {
		printk(KERN_CONT " P%d", t->pid);
		ndetected++;
	}
	rcu_print_task_stall_end();
	return ndetected;
}

static void rcu_preempt_stall_reset(void)
{
	rcu_preempt_state.jiffies_stall = jiffies + ULONG_MAX / 2;
}

static void rcu_preempt_check_blocked_tasks(struct rcu_node *rnp)
{
	WARN_ON_ONCE(rcu_preempt_blocked_readers_cgp(rnp));
	if (!list_empty(&rnp->blkd_tasks))
		rnp->gp_tasks = rnp->blkd_tasks.next;
	WARN_ON_ONCE(rnp->qsmask);
}

#ifdef CONFIG_HOTPLUG_CPU

static int rcu_preempt_offline_tasks(struct rcu_state *rsp,
				     struct rcu_node *rnp,
				     struct rcu_data *rdp)
{
	struct list_head *lp;
	struct list_head *lp_root;
	int retval = 0;
	struct rcu_node *rnp_root = rcu_get_root(rsp);
	struct task_struct *t;

	if (rnp == rnp_root) {
		WARN_ONCE(1, "Last CPU thought to be offlined?");
		return 0;  
	}

	
	WARN_ON_ONCE(rnp != rdp->mynode);

	if (rcu_preempt_blocked_readers_cgp(rnp) && rnp->qsmask == 0)
		retval |= RCU_OFL_TASKS_NORM_GP;
	if (rcu_preempted_readers_exp(rnp))
		retval |= RCU_OFL_TASKS_EXP_GP;
	lp = &rnp->blkd_tasks;
	lp_root = &rnp_root->blkd_tasks;
	while (!list_empty(lp)) {
		t = list_entry(lp->next, typeof(*t), rcu_node_entry);
		raw_spin_lock(&rnp_root->lock); 
		list_del(&t->rcu_node_entry);
		t->rcu_blocked_node = rnp_root;
		list_add(&t->rcu_node_entry, lp_root);
		if (&t->rcu_node_entry == rnp->gp_tasks)
			rnp_root->gp_tasks = rnp->gp_tasks;
		if (&t->rcu_node_entry == rnp->exp_tasks)
			rnp_root->exp_tasks = rnp->exp_tasks;
#ifdef CONFIG_RCU_BOOST
		if (&t->rcu_node_entry == rnp->boost_tasks)
			rnp_root->boost_tasks = rnp->boost_tasks;
#endif 
		raw_spin_unlock(&rnp_root->lock); 
	}

#ifdef CONFIG_RCU_BOOST
	
	raw_spin_lock(&rnp_root->lock); 
	if (rnp_root->boost_tasks != NULL &&
	    rnp_root->boost_tasks != rnp_root->gp_tasks)
		rnp_root->boost_tasks = rnp_root->gp_tasks;
	raw_spin_unlock(&rnp_root->lock); 
#endif 

	rnp->gp_tasks = NULL;
	rnp->exp_tasks = NULL;
	return retval;
}

#endif 

static void rcu_preempt_cleanup_dead_cpu(int cpu)
{
	rcu_cleanup_dead_cpu(cpu, &rcu_preempt_state);
}

static void rcu_preempt_check_callbacks(int cpu)
{
	struct task_struct *t = current;

	if (t->rcu_read_lock_nesting == 0) {
		rcu_preempt_qs(cpu);
		return;
	}
	if (t->rcu_read_lock_nesting > 0 &&
	    per_cpu(rcu_preempt_data, cpu).qs_pending)
		t->rcu_read_unlock_special |= RCU_READ_UNLOCK_NEED_QS;
}

static void rcu_preempt_process_callbacks(void)
{
	__rcu_process_callbacks(&rcu_preempt_state,
				&__get_cpu_var(rcu_preempt_data));
}

#ifdef CONFIG_RCU_BOOST

static void rcu_preempt_do_callbacks(void)
{
	rcu_do_batch(&rcu_preempt_state, &__get_cpu_var(rcu_preempt_data));
}

#endif 

void call_rcu(struct rcu_head *head, void (*func)(struct rcu_head *rcu))
{
	__call_rcu(head, func, &rcu_preempt_state, 0);
}
EXPORT_SYMBOL_GPL(call_rcu);

void kfree_call_rcu(struct rcu_head *head,
		    void (*func)(struct rcu_head *rcu))
{
	__call_rcu(head, func, &rcu_preempt_state, 1);
}
EXPORT_SYMBOL_GPL(kfree_call_rcu);

void synchronize_rcu(void)
{
	rcu_lockdep_assert(!lock_is_held(&rcu_bh_lock_map) &&
			   !lock_is_held(&rcu_lock_map) &&
			   !lock_is_held(&rcu_sched_lock_map),
			   "Illegal synchronize_rcu() in RCU read-side critical section");
	if (!rcu_scheduler_active)
		return;
	wait_rcu_gp(call_rcu);
}
EXPORT_SYMBOL_GPL(synchronize_rcu);

static DECLARE_WAIT_QUEUE_HEAD(sync_rcu_preempt_exp_wq);
static long sync_rcu_preempt_exp_count;
static DEFINE_MUTEX(sync_rcu_preempt_exp_mutex);

static int rcu_preempted_readers_exp(struct rcu_node *rnp)
{
	return rnp->exp_tasks != NULL;
}

static int sync_rcu_preempt_exp_done(struct rcu_node *rnp)
{
	return !rcu_preempted_readers_exp(rnp) &&
	       ACCESS_ONCE(rnp->expmask) == 0;
}

static void rcu_report_exp_rnp(struct rcu_state *rsp, struct rcu_node *rnp,
			       bool wake)
{
	unsigned long flags;
	unsigned long mask;

	raw_spin_lock_irqsave(&rnp->lock, flags);
	for (;;) {
		if (!sync_rcu_preempt_exp_done(rnp)) {
			raw_spin_unlock_irqrestore(&rnp->lock, flags);
			break;
		}
		if (rnp->parent == NULL) {
			raw_spin_unlock_irqrestore(&rnp->lock, flags);
			if (wake)
				wake_up(&sync_rcu_preempt_exp_wq);
			break;
		}
		mask = rnp->grpmask;
		raw_spin_unlock(&rnp->lock); 
		rnp = rnp->parent;
		raw_spin_lock(&rnp->lock); 
		rnp->expmask &= ~mask;
	}
}

static void
sync_rcu_preempt_exp_init(struct rcu_state *rsp, struct rcu_node *rnp)
{
	unsigned long flags;
	int must_wait = 0;

	raw_spin_lock_irqsave(&rnp->lock, flags);
	if (list_empty(&rnp->blkd_tasks))
		raw_spin_unlock_irqrestore(&rnp->lock, flags);
	else {
		rnp->exp_tasks = rnp->blkd_tasks.next;
		rcu_initiate_boost(rnp, flags);  
		must_wait = 1;
	}
	if (!must_wait)
		rcu_report_exp_rnp(rsp, rnp, false); 
}

void synchronize_rcu_expedited(void)
{
	unsigned long flags;
	struct rcu_node *rnp;
	struct rcu_state *rsp = &rcu_preempt_state;
	long snap;
	int trycount = 0;

	smp_mb(); 
	snap = ACCESS_ONCE(sync_rcu_preempt_exp_count) + 1;
	smp_mb(); 

	while (!mutex_trylock(&sync_rcu_preempt_exp_mutex)) {
		if (trycount++ < 10)
			udelay(trycount * num_online_cpus());
		else {
			synchronize_rcu();
			return;
		}
		if ((ACCESS_ONCE(sync_rcu_preempt_exp_count) - snap) > 0)
			goto mb_ret; 
	}
	if ((ACCESS_ONCE(sync_rcu_preempt_exp_count) - snap) > 0)
		goto unlock_mb_ret; 

	
	synchronize_sched_expedited();

	raw_spin_lock_irqsave(&rsp->onofflock, flags);

	
	rcu_for_each_nonleaf_node_breadth_first(rsp, rnp) {
		raw_spin_lock(&rnp->lock); 
		rnp->expmask = rnp->qsmaskinit;
		raw_spin_unlock(&rnp->lock); 
	}

	
	rcu_for_each_leaf_node(rsp, rnp)
		sync_rcu_preempt_exp_init(rsp, rnp);
	if (NUM_RCU_NODES > 1)
		sync_rcu_preempt_exp_init(rsp, rcu_get_root(rsp));

	raw_spin_unlock_irqrestore(&rsp->onofflock, flags);

	
	rnp = rcu_get_root(rsp);
	wait_event(sync_rcu_preempt_exp_wq,
		   sync_rcu_preempt_exp_done(rnp));

	
	smp_mb(); 
	ACCESS_ONCE(sync_rcu_preempt_exp_count)++;
unlock_mb_ret:
	mutex_unlock(&sync_rcu_preempt_exp_mutex);
mb_ret:
	smp_mb(); 
}
EXPORT_SYMBOL_GPL(synchronize_rcu_expedited);

static int rcu_preempt_pending(int cpu)
{
	return __rcu_pending(&rcu_preempt_state,
			     &per_cpu(rcu_preempt_data, cpu));
}

static int rcu_preempt_cpu_has_callbacks(int cpu)
{
	return !!per_cpu(rcu_preempt_data, cpu).nxtlist;
}

void rcu_barrier(void)
{
	_rcu_barrier(&rcu_preempt_state, call_rcu);
}
EXPORT_SYMBOL_GPL(rcu_barrier);

static void __cpuinit rcu_preempt_init_percpu_data(int cpu)
{
	rcu_init_percpu_data(cpu, &rcu_preempt_state, 1);
}

static void rcu_preempt_cleanup_dying_cpu(void)
{
	rcu_cleanup_dying_cpu(&rcu_preempt_state);
}

static void __init __rcu_init_preempt(void)
{
	rcu_init_one(&rcu_preempt_state, &rcu_preempt_data);
}

void exit_rcu(void)
{
	struct task_struct *t = current;

	if (t->rcu_read_lock_nesting == 0)
		return;
	t->rcu_read_lock_nesting = 1;
	__rcu_read_unlock();
}

#else 

static struct rcu_state *rcu_state = &rcu_sched_state;

static void __init rcu_bootup_announce(void)
{
	printk(KERN_INFO "Hierarchical RCU implementation.\n");
	rcu_bootup_announce_oddness();
}

long rcu_batches_completed(void)
{
	return rcu_batches_completed_sched();
}
EXPORT_SYMBOL_GPL(rcu_batches_completed);

void rcu_force_quiescent_state(void)
{
	rcu_sched_force_quiescent_state();
}
EXPORT_SYMBOL_GPL(rcu_force_quiescent_state);

static void rcu_preempt_note_context_switch(int cpu)
{
}

static int rcu_preempt_blocked_readers_cgp(struct rcu_node *rnp)
{
	return 0;
}

#ifdef CONFIG_HOTPLUG_CPU

static void rcu_report_unblock_qs_rnp(struct rcu_node *rnp, unsigned long flags)
{
	raw_spin_unlock_irqrestore(&rnp->lock, flags);
}

#endif 

static void rcu_print_detail_task_stall(struct rcu_state *rsp)
{
}

static int rcu_print_task_stall(struct rcu_node *rnp)
{
	return 0;
}

static void rcu_preempt_stall_reset(void)
{
}

static void rcu_preempt_check_blocked_tasks(struct rcu_node *rnp)
{
	WARN_ON_ONCE(rnp->qsmask);
}

#ifdef CONFIG_HOTPLUG_CPU

static int rcu_preempt_offline_tasks(struct rcu_state *rsp,
				     struct rcu_node *rnp,
				     struct rcu_data *rdp)
{
	return 0;
}

#endif 

static void rcu_preempt_cleanup_dead_cpu(int cpu)
{
}

static void rcu_preempt_check_callbacks(int cpu)
{
}

static void rcu_preempt_process_callbacks(void)
{
}

void kfree_call_rcu(struct rcu_head *head,
		    void (*func)(struct rcu_head *rcu))
{
	__call_rcu(head, func, &rcu_sched_state, 1);
}
EXPORT_SYMBOL_GPL(kfree_call_rcu);

void synchronize_rcu_expedited(void)
{
	synchronize_sched_expedited();
}
EXPORT_SYMBOL_GPL(synchronize_rcu_expedited);

#ifdef CONFIG_HOTPLUG_CPU

static void rcu_report_exp_rnp(struct rcu_state *rsp, struct rcu_node *rnp,
			       bool wake)
{
}

#endif 

static int rcu_preempt_pending(int cpu)
{
	return 0;
}

static int rcu_preempt_cpu_has_callbacks(int cpu)
{
	return 0;
}

void rcu_barrier(void)
{
	rcu_barrier_sched();
}
EXPORT_SYMBOL_GPL(rcu_barrier);

static void __cpuinit rcu_preempt_init_percpu_data(int cpu)
{
}

static void rcu_preempt_cleanup_dying_cpu(void)
{
}

static void __init __rcu_init_preempt(void)
{
}

#endif 

#ifdef CONFIG_RCU_BOOST

#include "rtmutex_common.h"

#ifdef CONFIG_RCU_TRACE

static void rcu_initiate_boost_trace(struct rcu_node *rnp)
{
	if (list_empty(&rnp->blkd_tasks))
		rnp->n_balk_blkd_tasks++;
	else if (rnp->exp_tasks == NULL && rnp->gp_tasks == NULL)
		rnp->n_balk_exp_gp_tasks++;
	else if (rnp->gp_tasks != NULL && rnp->boost_tasks != NULL)
		rnp->n_balk_boost_tasks++;
	else if (rnp->gp_tasks != NULL && rnp->qsmask != 0)
		rnp->n_balk_notblocked++;
	else if (rnp->gp_tasks != NULL &&
		 ULONG_CMP_LT(jiffies, rnp->boost_time))
		rnp->n_balk_notyet++;
	else
		rnp->n_balk_nos++;
}

#else 

static void rcu_initiate_boost_trace(struct rcu_node *rnp)
{
}

#endif 

static int rcu_boost(struct rcu_node *rnp)
{
	unsigned long flags;
	struct rt_mutex mtx;
	struct task_struct *t;
	struct list_head *tb;

	if (rnp->exp_tasks == NULL && rnp->boost_tasks == NULL)
		return 0;  

	raw_spin_lock_irqsave(&rnp->lock, flags);

	if (rnp->exp_tasks == NULL && rnp->boost_tasks == NULL) {
		raw_spin_unlock_irqrestore(&rnp->lock, flags);
		return 0;
	}

	if (rnp->exp_tasks != NULL) {
		tb = rnp->exp_tasks;
		rnp->n_exp_boosts++;
	} else {
		tb = rnp->boost_tasks;
		rnp->n_normal_boosts++;
	}
	rnp->n_tasks_boosted++;

	t = container_of(tb, struct task_struct, rcu_node_entry);
	rt_mutex_init_proxy_locked(&mtx, t);
	t->rcu_boost_mutex = &mtx;
	raw_spin_unlock_irqrestore(&rnp->lock, flags);
	rt_mutex_lock(&mtx);  
	rt_mutex_unlock(&mtx);  

	return ACCESS_ONCE(rnp->exp_tasks) != NULL ||
	       ACCESS_ONCE(rnp->boost_tasks) != NULL;
}

static void rcu_boost_kthread_timer(unsigned long arg)
{
	invoke_rcu_node_kthread((struct rcu_node *)arg);
}

static int rcu_boost_kthread(void *arg)
{
	struct rcu_node *rnp = (struct rcu_node *)arg;
	int spincnt = 0;
	int more2boost;

	trace_rcu_utilization("Start boost kthread@init");
	for (;;) {
		rnp->boost_kthread_status = RCU_KTHREAD_WAITING;
		trace_rcu_utilization("End boost kthread@rcu_wait");
		rcu_wait(rnp->boost_tasks || rnp->exp_tasks);
		trace_rcu_utilization("Start boost kthread@rcu_wait");
		rnp->boost_kthread_status = RCU_KTHREAD_RUNNING;
		more2boost = rcu_boost(rnp);
		if (more2boost)
			spincnt++;
		else
			spincnt = 0;
		if (spincnt > 10) {
			trace_rcu_utilization("End boost kthread@rcu_yield");
			rcu_yield(rcu_boost_kthread_timer, (unsigned long)rnp);
			trace_rcu_utilization("Start boost kthread@rcu_yield");
			spincnt = 0;
		}
	}
	
	trace_rcu_utilization("End boost kthread@notreached");
	return 0;
}

static void rcu_initiate_boost(struct rcu_node *rnp, unsigned long flags)
{
	struct task_struct *t;

	if (!rcu_preempt_blocked_readers_cgp(rnp) && rnp->exp_tasks == NULL) {
		rnp->n_balk_exp_gp_tasks++;
		raw_spin_unlock_irqrestore(&rnp->lock, flags);
		return;
	}
	if (rnp->exp_tasks != NULL ||
	    (rnp->gp_tasks != NULL &&
	     rnp->boost_tasks == NULL &&
	     rnp->qsmask == 0 &&
	     ULONG_CMP_GE(jiffies, rnp->boost_time))) {
		if (rnp->exp_tasks == NULL)
			rnp->boost_tasks = rnp->gp_tasks;
		raw_spin_unlock_irqrestore(&rnp->lock, flags);
		t = rnp->boost_kthread_task;
		if (t != NULL)
			wake_up_process(t);
	} else {
		rcu_initiate_boost_trace(rnp);
		raw_spin_unlock_irqrestore(&rnp->lock, flags);
	}
}

static void invoke_rcu_callbacks_kthread(void)
{
	unsigned long flags;

	local_irq_save(flags);
	__this_cpu_write(rcu_cpu_has_work, 1);
	if (__this_cpu_read(rcu_cpu_kthread_task) != NULL &&
	    current != __this_cpu_read(rcu_cpu_kthread_task))
		wake_up_process(__this_cpu_read(rcu_cpu_kthread_task));
	local_irq_restore(flags);
}

static bool rcu_is_callbacks_kthread(void)
{
	return __get_cpu_var(rcu_cpu_kthread_task) == current;
}

static void rcu_boost_kthread_setaffinity(struct rcu_node *rnp,
					  cpumask_var_t cm)
{
	struct task_struct *t;

	t = rnp->boost_kthread_task;
	if (t != NULL)
		set_cpus_allowed_ptr(rnp->boost_kthread_task, cm);
}

#define RCU_BOOST_DELAY_JIFFIES DIV_ROUND_UP(CONFIG_RCU_BOOST_DELAY * HZ, 1000)

static void rcu_preempt_boost_start_gp(struct rcu_node *rnp)
{
	rnp->boost_time = jiffies + RCU_BOOST_DELAY_JIFFIES;
}

static int __cpuinit rcu_spawn_one_boost_kthread(struct rcu_state *rsp,
						 struct rcu_node *rnp,
						 int rnp_index)
{
	unsigned long flags;
	struct sched_param sp;
	struct task_struct *t;

	if (&rcu_preempt_state != rsp)
		return 0;
	rsp->boost = 1;
	if (rnp->boost_kthread_task != NULL)
		return 0;
	t = kthread_create(rcu_boost_kthread, (void *)rnp,
			   "rcub/%d", rnp_index);
	if (IS_ERR(t))
		return PTR_ERR(t);
	raw_spin_lock_irqsave(&rnp->lock, flags);
	rnp->boost_kthread_task = t;
	raw_spin_unlock_irqrestore(&rnp->lock, flags);
	sp.sched_priority = RCU_BOOST_PRIO;
	sched_setscheduler_nocheck(t, SCHED_FIFO, &sp);
	wake_up_process(t); 
	return 0;
}

#ifdef CONFIG_HOTPLUG_CPU

static void rcu_stop_cpu_kthread(int cpu)
{
	struct task_struct *t;

	
	t = per_cpu(rcu_cpu_kthread_task, cpu);
	if (t != NULL) {
		per_cpu(rcu_cpu_kthread_task, cpu) = NULL;
		kthread_stop(t);
	}
}

#endif 

static void rcu_kthread_do_work(void)
{
	rcu_do_batch(&rcu_sched_state, &__get_cpu_var(rcu_sched_data));
	rcu_do_batch(&rcu_bh_state, &__get_cpu_var(rcu_bh_data));
	rcu_preempt_do_callbacks();
}

static void invoke_rcu_node_kthread(struct rcu_node *rnp)
{
	struct task_struct *t;

	t = rnp->node_kthread_task;
	if (t != NULL)
		wake_up_process(t);
}

static void rcu_cpu_kthread_setrt(int cpu, int to_rt)
{
	int policy;
	struct sched_param sp;
	struct task_struct *t;

	t = per_cpu(rcu_cpu_kthread_task, cpu);
	if (t == NULL)
		return;
	if (to_rt) {
		policy = SCHED_FIFO;
		sp.sched_priority = RCU_KTHREAD_PRIO;
	} else {
		policy = SCHED_NORMAL;
		sp.sched_priority = 0;
	}
	sched_setscheduler_nocheck(t, policy, &sp);
}

static void rcu_cpu_kthread_timer(unsigned long arg)
{
	struct rcu_data *rdp = per_cpu_ptr(rcu_state->rda, arg);
	struct rcu_node *rnp = rdp->mynode;

	atomic_or(rdp->grpmask, &rnp->wakemask);
	invoke_rcu_node_kthread(rnp);
}

static void rcu_yield(void (*f)(unsigned long), unsigned long arg)
{
	struct sched_param sp;
	struct timer_list yield_timer;
	int prio = current->rt_priority;

	setup_timer_on_stack(&yield_timer, f, arg);
	mod_timer(&yield_timer, jiffies + 2);
	sp.sched_priority = 0;
	sched_setscheduler_nocheck(current, SCHED_NORMAL, &sp);
	set_user_nice(current, 19);
	schedule();
	set_user_nice(current, 0);
	sp.sched_priority = prio;
	sched_setscheduler_nocheck(current, SCHED_FIFO, &sp);
	del_timer(&yield_timer);
}

static int rcu_cpu_kthread_should_stop(int cpu)
{
	while (cpu_is_offline(cpu) ||
	       !cpumask_equal(&current->cpus_allowed, cpumask_of(cpu)) ||
	       smp_processor_id() != cpu) {
		if (kthread_should_stop())
			return 1;
		per_cpu(rcu_cpu_kthread_status, cpu) = RCU_KTHREAD_OFFCPU;
		per_cpu(rcu_cpu_kthread_cpu, cpu) = raw_smp_processor_id();
		local_bh_enable();
		schedule_timeout_uninterruptible(1);
		if (!cpumask_equal(&current->cpus_allowed, cpumask_of(cpu)))
			set_cpus_allowed_ptr(current, cpumask_of(cpu));
		local_bh_disable();
	}
	per_cpu(rcu_cpu_kthread_cpu, cpu) = cpu;
	return 0;
}

static int rcu_cpu_kthread(void *arg)
{
	int cpu = (int)(long)arg;
	unsigned long flags;
	int spincnt = 0;
	unsigned int *statusp = &per_cpu(rcu_cpu_kthread_status, cpu);
	char work;
	char *workp = &per_cpu(rcu_cpu_has_work, cpu);

	trace_rcu_utilization("Start CPU kthread@init");
	for (;;) {
		*statusp = RCU_KTHREAD_WAITING;
		trace_rcu_utilization("End CPU kthread@rcu_wait");
		rcu_wait(*workp != 0 || kthread_should_stop());
		trace_rcu_utilization("Start CPU kthread@rcu_wait");
		local_bh_disable();
		if (rcu_cpu_kthread_should_stop(cpu)) {
			local_bh_enable();
			break;
		}
		*statusp = RCU_KTHREAD_RUNNING;
		per_cpu(rcu_cpu_kthread_loops, cpu)++;
		local_irq_save(flags);
		work = *workp;
		*workp = 0;
		local_irq_restore(flags);
		if (work)
			rcu_kthread_do_work();
		local_bh_enable();
		if (*workp != 0)
			spincnt++;
		else
			spincnt = 0;
		if (spincnt > 10) {
			*statusp = RCU_KTHREAD_YIELDING;
			trace_rcu_utilization("End CPU kthread@rcu_yield");
			rcu_yield(rcu_cpu_kthread_timer, (unsigned long)cpu);
			trace_rcu_utilization("Start CPU kthread@rcu_yield");
			spincnt = 0;
		}
	}
	*statusp = RCU_KTHREAD_STOPPED;
	trace_rcu_utilization("End CPU kthread@term");
	return 0;
}

static int __cpuinit rcu_spawn_one_cpu_kthread(int cpu)
{
	struct sched_param sp;
	struct task_struct *t;

	if (!rcu_scheduler_fully_active ||
	    per_cpu(rcu_cpu_kthread_task, cpu) != NULL)
		return 0;
	t = kthread_create_on_node(rcu_cpu_kthread,
				   (void *)(long)cpu,
				   cpu_to_node(cpu),
				   "rcuc/%d", cpu);
	if (IS_ERR(t))
		return PTR_ERR(t);
	if (cpu_online(cpu))
		kthread_bind(t, cpu);
	per_cpu(rcu_cpu_kthread_cpu, cpu) = cpu;
	WARN_ON_ONCE(per_cpu(rcu_cpu_kthread_task, cpu) != NULL);
	sp.sched_priority = RCU_KTHREAD_PRIO;
	sched_setscheduler_nocheck(t, SCHED_FIFO, &sp);
	per_cpu(rcu_cpu_kthread_task, cpu) = t;
	wake_up_process(t); 
	return 0;
}

static int rcu_node_kthread(void *arg)
{
	int cpu;
	unsigned long flags;
	unsigned long mask;
	struct rcu_node *rnp = (struct rcu_node *)arg;
	struct sched_param sp;
	struct task_struct *t;

	for (;;) {
		rnp->node_kthread_status = RCU_KTHREAD_WAITING;
		rcu_wait(atomic_read(&rnp->wakemask) != 0);
		rnp->node_kthread_status = RCU_KTHREAD_RUNNING;
		raw_spin_lock_irqsave(&rnp->lock, flags);
		mask = atomic_xchg(&rnp->wakemask, 0);
		rcu_initiate_boost(rnp, flags); 
		for (cpu = rnp->grplo; cpu <= rnp->grphi; cpu++, mask >>= 1) {
			if ((mask & 0x1) == 0)
				continue;
			preempt_disable();
			t = per_cpu(rcu_cpu_kthread_task, cpu);
			if (!cpu_online(cpu) || t == NULL) {
				preempt_enable();
				continue;
			}
			per_cpu(rcu_cpu_has_work, cpu) = 1;
			sp.sched_priority = RCU_KTHREAD_PRIO;
			sched_setscheduler_nocheck(t, SCHED_FIFO, &sp);
			preempt_enable();
		}
	}
	
	rnp->node_kthread_status = RCU_KTHREAD_STOPPED;
	return 0;
}

static void rcu_node_kthread_setaffinity(struct rcu_node *rnp, int outgoingcpu)
{
	cpumask_var_t cm;
	int cpu;
	unsigned long mask = rnp->qsmaskinit;

	if (rnp->node_kthread_task == NULL)
		return;
	if (!alloc_cpumask_var(&cm, GFP_KERNEL))
		return;
	cpumask_clear(cm);
	for (cpu = rnp->grplo; cpu <= rnp->grphi; cpu++, mask >>= 1)
		if ((mask & 0x1) && cpu != outgoingcpu)
			cpumask_set_cpu(cpu, cm);
	if (cpumask_weight(cm) == 0) {
		cpumask_setall(cm);
		for (cpu = rnp->grplo; cpu <= rnp->grphi; cpu++)
			cpumask_clear_cpu(cpu, cm);
		WARN_ON_ONCE(cpumask_weight(cm) == 0);
	}
	set_cpus_allowed_ptr(rnp->node_kthread_task, cm);
	rcu_boost_kthread_setaffinity(rnp, cm);
	free_cpumask_var(cm);
}

static int __cpuinit rcu_spawn_one_node_kthread(struct rcu_state *rsp,
						struct rcu_node *rnp)
{
	unsigned long flags;
	int rnp_index = rnp - &rsp->node[0];
	struct sched_param sp;
	struct task_struct *t;

	if (!rcu_scheduler_fully_active ||
	    rnp->qsmaskinit == 0)
		return 0;
	if (rnp->node_kthread_task == NULL) {
		t = kthread_create(rcu_node_kthread, (void *)rnp,
				   "rcun/%d", rnp_index);
		if (IS_ERR(t))
			return PTR_ERR(t);
		raw_spin_lock_irqsave(&rnp->lock, flags);
		rnp->node_kthread_task = t;
		raw_spin_unlock_irqrestore(&rnp->lock, flags);
		sp.sched_priority = 99;
		sched_setscheduler_nocheck(t, SCHED_FIFO, &sp);
		wake_up_process(t); 
	}
	return rcu_spawn_one_boost_kthread(rsp, rnp, rnp_index);
}

static int __init rcu_spawn_kthreads(void)
{
	int cpu;
	struct rcu_node *rnp;

	rcu_scheduler_fully_active = 1;
	for_each_possible_cpu(cpu) {
		per_cpu(rcu_cpu_has_work, cpu) = 0;
		if (cpu_online(cpu))
			(void)rcu_spawn_one_cpu_kthread(cpu);
	}
	rnp = rcu_get_root(rcu_state);
	(void)rcu_spawn_one_node_kthread(rcu_state, rnp);
	if (NUM_RCU_NODES > 1) {
		rcu_for_each_leaf_node(rcu_state, rnp)
			(void)rcu_spawn_one_node_kthread(rcu_state, rnp);
	}
	return 0;
}
early_initcall(rcu_spawn_kthreads);

static void __cpuinit rcu_prepare_kthreads(int cpu)
{
	struct rcu_data *rdp = per_cpu_ptr(rcu_state->rda, cpu);
	struct rcu_node *rnp = rdp->mynode;

	
	if (rcu_scheduler_fully_active) {
		(void)rcu_spawn_one_cpu_kthread(cpu);
		if (rnp->node_kthread_task == NULL)
			(void)rcu_spawn_one_node_kthread(rcu_state, rnp);
	}
}

#else 

static void rcu_initiate_boost(struct rcu_node *rnp, unsigned long flags)
{
	raw_spin_unlock_irqrestore(&rnp->lock, flags);
}

static void invoke_rcu_callbacks_kthread(void)
{
	WARN_ON_ONCE(1);
}

static bool rcu_is_callbacks_kthread(void)
{
	return false;
}

static void rcu_preempt_boost_start_gp(struct rcu_node *rnp)
{
}

#ifdef CONFIG_HOTPLUG_CPU

static void rcu_stop_cpu_kthread(int cpu)
{
}

#endif 

static void rcu_node_kthread_setaffinity(struct rcu_node *rnp, int outgoingcpu)
{
}

static void rcu_cpu_kthread_setrt(int cpu, int to_rt)
{
}

static int __init rcu_scheduler_really_started(void)
{
	rcu_scheduler_fully_active = 1;
	return 0;
}
early_initcall(rcu_scheduler_really_started);

static void __cpuinit rcu_prepare_kthreads(int cpu)
{
}

#endif 

#if !defined(CONFIG_RCU_FAST_NO_HZ)

int rcu_needs_cpu(int cpu)
{
	return rcu_cpu_has_callbacks(cpu);
}

static void rcu_prepare_for_idle_init(int cpu)
{
}

static void rcu_cleanup_after_idle(int cpu)
{
}

static void rcu_prepare_for_idle(int cpu)
{
}

#else 

#define RCU_IDLE_FLUSHES 5		
#define RCU_IDLE_OPT_FLUSHES 3		
#define RCU_IDLE_GP_DELAY 6		
#define RCU_IDLE_LAZY_GP_DELAY (6 * HZ)	

static DEFINE_PER_CPU(int, rcu_dyntick_drain);
static DEFINE_PER_CPU(unsigned long, rcu_dyntick_holdoff);
static DEFINE_PER_CPU(struct hrtimer, rcu_idle_gp_timer);
static ktime_t rcu_idle_gp_wait;	
static ktime_t rcu_idle_lazy_gp_wait;	

int rcu_needs_cpu(int cpu)
{
	
	if (!rcu_cpu_has_callbacks(cpu))
		return 0;
	
	return per_cpu(rcu_dyntick_holdoff, cpu) == jiffies;
}

static bool __rcu_cpu_has_nonlazy_callbacks(struct rcu_data *rdp)
{
	return rdp->qlen != rdp->qlen_lazy;
}

#ifdef CONFIG_TREE_PREEMPT_RCU

static bool rcu_preempt_cpu_has_nonlazy_callbacks(int cpu)
{
	struct rcu_data *rdp = &per_cpu(rcu_preempt_data, cpu);

	return __rcu_cpu_has_nonlazy_callbacks(rdp);
}

#else 

static bool rcu_preempt_cpu_has_nonlazy_callbacks(int cpu)
{
	return 0;
}

#endif 

static bool rcu_cpu_has_nonlazy_callbacks(int cpu)
{
	return __rcu_cpu_has_nonlazy_callbacks(&per_cpu(rcu_sched_data, cpu)) ||
	       __rcu_cpu_has_nonlazy_callbacks(&per_cpu(rcu_bh_data, cpu)) ||
	       rcu_preempt_cpu_has_nonlazy_callbacks(cpu);
}

static enum hrtimer_restart rcu_idle_gp_timer_func(struct hrtimer *hrtp)
{
	trace_rcu_prep_idle("Timer");
	return HRTIMER_NORESTART;
}

static void rcu_prepare_for_idle_init(int cpu)
{
	static int firsttime = 1;
	struct hrtimer *hrtp = &per_cpu(rcu_idle_gp_timer, cpu);

	hrtimer_init(hrtp, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtp->function = rcu_idle_gp_timer_func;
	if (firsttime) {
		unsigned int upj = jiffies_to_usecs(RCU_IDLE_GP_DELAY);

		rcu_idle_gp_wait = ns_to_ktime(upj * (u64)1000);
		upj = jiffies_to_usecs(RCU_IDLE_LAZY_GP_DELAY);
		rcu_idle_lazy_gp_wait = ns_to_ktime(upj * (u64)1000);
		firsttime = 0;
	}
}

static void rcu_cleanup_after_idle(int cpu)
{
	hrtimer_cancel(&per_cpu(rcu_idle_gp_timer, cpu));
}

static void rcu_prepare_for_idle(int cpu)
{
	if (!rcu_cpu_has_callbacks(cpu)) {
		per_cpu(rcu_dyntick_holdoff, cpu) = jiffies - 1;
		per_cpu(rcu_dyntick_drain, cpu) = 0;
		trace_rcu_prep_idle("No callbacks");
		return;
	}

	if (per_cpu(rcu_dyntick_holdoff, cpu) == jiffies) {
		trace_rcu_prep_idle("In holdoff");
		return;
	}

	
	if (per_cpu(rcu_dyntick_drain, cpu) <= 0) {
		
		per_cpu(rcu_dyntick_drain, cpu) = RCU_IDLE_FLUSHES;
	} else if (per_cpu(rcu_dyntick_drain, cpu) <= RCU_IDLE_OPT_FLUSHES &&
		   !rcu_pending(cpu) &&
		   !local_softirq_pending()) {
		
		trace_rcu_prep_idle("Dyntick with callbacks");
		per_cpu(rcu_dyntick_drain, cpu) = 0;
		per_cpu(rcu_dyntick_holdoff, cpu) = jiffies;
		if (rcu_cpu_has_nonlazy_callbacks(cpu))
			hrtimer_start(&per_cpu(rcu_idle_gp_timer, cpu),
				      rcu_idle_gp_wait, HRTIMER_MODE_REL);
		else
			hrtimer_start(&per_cpu(rcu_idle_gp_timer, cpu),
				      rcu_idle_lazy_gp_wait, HRTIMER_MODE_REL);
		return; 
	} else if (--per_cpu(rcu_dyntick_drain, cpu) <= 0) {
		
		per_cpu(rcu_dyntick_holdoff, cpu) = jiffies;
		trace_rcu_prep_idle("Begin holdoff");
		invoke_rcu_core();  
		return;
	}

#ifdef CONFIG_TREE_PREEMPT_RCU
	if (per_cpu(rcu_preempt_data, cpu).nxtlist) {
		rcu_preempt_qs(cpu);
		force_quiescent_state(&rcu_preempt_state, 0);
	}
#endif 
	if (per_cpu(rcu_sched_data, cpu).nxtlist) {
		rcu_sched_qs(cpu);
		force_quiescent_state(&rcu_sched_state, 0);
	}
	if (per_cpu(rcu_bh_data, cpu).nxtlist) {
		rcu_bh_qs(cpu);
		force_quiescent_state(&rcu_bh_state, 0);
	}

	if (rcu_cpu_has_callbacks(cpu)) {
		trace_rcu_prep_idle("More callbacks");
		invoke_rcu_core();
	} else
		trace_rcu_prep_idle("Callbacks drained");
}

#endif 

#ifdef CONFIG_RCU_CPU_STALL_INFO

#ifdef CONFIG_RCU_FAST_NO_HZ

static void print_cpu_stall_fast_no_hz(char *cp, int cpu)
{
	struct hrtimer *hrtp = &per_cpu(rcu_idle_gp_timer, cpu);

	sprintf(cp, "drain=%d %c timer=%lld",
		per_cpu(rcu_dyntick_drain, cpu),
		per_cpu(rcu_dyntick_holdoff, cpu) == jiffies ? 'H' : '.',
		hrtimer_active(hrtp)
			? ktime_to_us(hrtimer_get_remaining(hrtp))
			: -1);
}

#else 

static void print_cpu_stall_fast_no_hz(char *cp, int cpu)
{
}

#endif 

static void print_cpu_stall_info_begin(void)
{
	printk(KERN_CONT "\n");
}

static void print_cpu_stall_info(struct rcu_state *rsp, int cpu)
{
	char fast_no_hz[72];
	struct rcu_data *rdp = per_cpu_ptr(rsp->rda, cpu);
	struct rcu_dynticks *rdtp = rdp->dynticks;
	char *ticks_title;
	unsigned long ticks_value;

	if (rsp->gpnum == rdp->gpnum) {
		ticks_title = "ticks this GP";
		ticks_value = rdp->ticks_this_gp;
	} else {
		ticks_title = "GPs behind";
		ticks_value = rsp->gpnum - rdp->gpnum;
	}
	print_cpu_stall_fast_no_hz(fast_no_hz, cpu);
	printk(KERN_ERR "\t%d: (%lu %s) idle=%03x/%llx/%d %s\n",
	       cpu, ticks_value, ticks_title,
	       atomic_read(&rdtp->dynticks) & 0xfff,
	       rdtp->dynticks_nesting, rdtp->dynticks_nmi_nesting,
	       fast_no_hz);
}

static void print_cpu_stall_info_end(void)
{
	printk(KERN_ERR "\t");
}

static void zero_cpu_stall_ticks(struct rcu_data *rdp)
{
	rdp->ticks_this_gp = 0;
}

static void increment_cpu_stall_ticks(void)
{
	__get_cpu_var(rcu_sched_data).ticks_this_gp++;
	__get_cpu_var(rcu_bh_data).ticks_this_gp++;
#ifdef CONFIG_TREE_PREEMPT_RCU
	__get_cpu_var(rcu_preempt_data).ticks_this_gp++;
#endif 
}

#else 

static void print_cpu_stall_info_begin(void)
{
	printk(KERN_CONT " {");
}

static void print_cpu_stall_info(struct rcu_state *rsp, int cpu)
{
	printk(KERN_CONT " %d", cpu);
}

static void print_cpu_stall_info_end(void)
{
	printk(KERN_CONT "} ");
}

static void zero_cpu_stall_ticks(struct rcu_data *rdp)
{
}

static void increment_cpu_stall_ticks(void)
{
}

#endif 
