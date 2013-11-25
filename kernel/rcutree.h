/*
 * Read-Copy Update mechanism for mutual exclusion (tree-based version)
 * Internal non-public definitions.
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
 * Copyright IBM Corporation, 2008
 *
 * Author: Ingo Molnar <mingo@elte.hu>
 *	   Paul E. McKenney <paulmck@linux.vnet.ibm.com>
 */

#include <linux/cache.h>
#include <linux/spinlock.h>
#include <linux/threads.h>
#include <linux/cpumask.h>
#include <linux/seqlock.h>

#define MAX_RCU_LVLS 4
#if CONFIG_RCU_FANOUT > 16
#define RCU_FANOUT_LEAF       16
#else 
#define RCU_FANOUT_LEAF       (CONFIG_RCU_FANOUT)
#endif 
#define RCU_FANOUT_1	      (RCU_FANOUT_LEAF)
#define RCU_FANOUT_2	      (RCU_FANOUT_1 * CONFIG_RCU_FANOUT)
#define RCU_FANOUT_3	      (RCU_FANOUT_2 * CONFIG_RCU_FANOUT)
#define RCU_FANOUT_4	      (RCU_FANOUT_3 * CONFIG_RCU_FANOUT)

#if NR_CPUS <= RCU_FANOUT_1
#  define NUM_RCU_LVLS	      1
#  define NUM_RCU_LVL_0	      1
#  define NUM_RCU_LVL_1	      (NR_CPUS)
#  define NUM_RCU_LVL_2	      0
#  define NUM_RCU_LVL_3	      0
#  define NUM_RCU_LVL_4	      0
#elif NR_CPUS <= RCU_FANOUT_2
#  define NUM_RCU_LVLS	      2
#  define NUM_RCU_LVL_0	      1
#  define NUM_RCU_LVL_1	      DIV_ROUND_UP(NR_CPUS, RCU_FANOUT_1)
#  define NUM_RCU_LVL_2	      (NR_CPUS)
#  define NUM_RCU_LVL_3	      0
#  define NUM_RCU_LVL_4	      0
#elif NR_CPUS <= RCU_FANOUT_3
#  define NUM_RCU_LVLS	      3
#  define NUM_RCU_LVL_0	      1
#  define NUM_RCU_LVL_1	      DIV_ROUND_UP(NR_CPUS, RCU_FANOUT_2)
#  define NUM_RCU_LVL_2	      DIV_ROUND_UP(NR_CPUS, RCU_FANOUT_1)
#  define NUM_RCU_LVL_3	      (NR_CPUS)
#  define NUM_RCU_LVL_4	      0
#elif NR_CPUS <= RCU_FANOUT_4
#  define NUM_RCU_LVLS	      4
#  define NUM_RCU_LVL_0	      1
#  define NUM_RCU_LVL_1	      DIV_ROUND_UP(NR_CPUS, RCU_FANOUT_3)
#  define NUM_RCU_LVL_2	      DIV_ROUND_UP(NR_CPUS, RCU_FANOUT_2)
#  define NUM_RCU_LVL_3	      DIV_ROUND_UP(NR_CPUS, RCU_FANOUT_1)
#  define NUM_RCU_LVL_4	      (NR_CPUS)
#else
# error "CONFIG_RCU_FANOUT insufficient for NR_CPUS"
#endif 

#define RCU_SUM (NUM_RCU_LVL_0 + NUM_RCU_LVL_1 + NUM_RCU_LVL_2 + NUM_RCU_LVL_3 + NUM_RCU_LVL_4)
#define NUM_RCU_NODES (RCU_SUM - NR_CPUS)

struct rcu_dynticks {
	long long dynticks_nesting; 
				    
	int dynticks_nmi_nesting;   
	atomic_t dynticks;	    
};

#define RCU_KTHREAD_STOPPED  0
#define RCU_KTHREAD_RUNNING  1
#define RCU_KTHREAD_WAITING  2
#define RCU_KTHREAD_OFFCPU   3
#define RCU_KTHREAD_YIELDING 4
#define RCU_KTHREAD_MAX      4

struct rcu_node {
	raw_spinlock_t lock;	
				
	unsigned long gpnum;	
				
				
	unsigned long completed; 
				
				
	unsigned long qsmask;	
				
				
				
				
				
	unsigned long expmask;	
				
				
				
	atomic_t wakemask;	
				
				
	unsigned long qsmaskinit;
				
	unsigned long grpmask;	
				
	int	grplo;		
	int	grphi;		
	u8	grpnum;		
	u8	level;		
	struct rcu_node *parent;
	struct list_head blkd_tasks;
				
				
				
	struct list_head *gp_tasks;
				
				
				
	struct list_head *exp_tasks;
				
				
				
				
				
#ifdef CONFIG_RCU_BOOST
	struct list_head *boost_tasks;
				
				
				
				
				
				
				
	unsigned long boost_time;
				
	struct task_struct *boost_kthread_task;
				
				
	unsigned int boost_kthread_status;
				
	unsigned long n_tasks_boosted;
				
	unsigned long n_exp_boosts;
				
	unsigned long n_normal_boosts;
				
	unsigned long n_balk_blkd_tasks;
				
	unsigned long n_balk_exp_gp_tasks;
				
	unsigned long n_balk_boost_tasks;
				
	unsigned long n_balk_notblocked;
				
	unsigned long n_balk_notyet;
				
	unsigned long n_balk_nos;
				
				
#endif 
	struct task_struct *node_kthread_task;
				
				
				
	unsigned int node_kthread_status;
				
} ____cacheline_internodealigned_in_smp;

#define rcu_for_each_node_breadth_first(rsp, rnp) \
	for ((rnp) = &(rsp)->node[0]; \
	     (rnp) < &(rsp)->node[NUM_RCU_NODES]; (rnp)++)

#define rcu_for_each_nonleaf_node_breadth_first(rsp, rnp) \
	for ((rnp) = &(rsp)->node[0]; \
	     (rnp) < (rsp)->level[NUM_RCU_LVLS - 1]; (rnp)++)

#define rcu_for_each_leaf_node(rsp, rnp) \
	for ((rnp) = (rsp)->level[NUM_RCU_LVLS - 1]; \
	     (rnp) < &(rsp)->node[NUM_RCU_NODES]; (rnp)++)

#define RCU_DONE_TAIL		0	
#define RCU_WAIT_TAIL		1	
#define RCU_NEXT_READY_TAIL	2	
#define RCU_NEXT_TAIL		3
#define RCU_NEXT_SIZE		4

struct rcu_data {
	
	unsigned long	completed;	
					
	unsigned long	gpnum;		
					
	unsigned long	passed_quiesce_gpnum;
					
	bool		passed_quiesce;	
	bool		qs_pending;	
	bool		beenonline;	
	bool		preemptible;	
	struct rcu_node *mynode;	
	unsigned long grpmask;		
#ifdef CONFIG_RCU_CPU_STALL_INFO
	unsigned long	ticks_this_gp;	
					
					
					
#endif 

	
	struct rcu_head *nxtlist;
	struct rcu_head **nxttail[RCU_NEXT_SIZE];
	long		qlen_lazy;	
	long		qlen;		
	long		qlen_last_fqs_check;
					
	unsigned long	n_cbs_invoked;	
	unsigned long   n_cbs_orphaned; 
	unsigned long   n_cbs_adopted;  
	unsigned long	n_force_qs_snap;
					
	long		blimit;		

	
	struct rcu_dynticks *dynticks;	
	int dynticks_snap;		

	
	unsigned long dynticks_fqs;	
	unsigned long offline_fqs;	

	
	unsigned long n_rcu_pending;	
	unsigned long n_rp_qs_pending;
	unsigned long n_rp_report_qs;
	unsigned long n_rp_cb_ready;
	unsigned long n_rp_cpu_needs_gp;
	unsigned long n_rp_gp_completed;
	unsigned long n_rp_gp_started;
	unsigned long n_rp_need_fqs;
	unsigned long n_rp_need_nothing;

	int cpu;
	struct rcu_state *rsp;
};

#define RCU_GP_IDLE		0	
#define RCU_GP_INIT		1	
#define RCU_SAVE_DYNTICK	2	
#define RCU_FORCE_QS		3	
#define RCU_SIGNAL_INIT		RCU_SAVE_DYNTICK

#define RCU_JIFFIES_TILL_FORCE_QS	 3	

#ifdef CONFIG_PROVE_RCU
#define RCU_STALL_DELAY_DELTA	       (5 * HZ)
#else
#define RCU_STALL_DELAY_DELTA	       0
#endif
#define RCU_STALL_RAT_DELAY		2	
						
						
						

#define rcu_wait(cond)							\
do {									\
	for (;;) {							\
		set_current_state(TASK_INTERRUPTIBLE);			\
		if (cond)						\
			break;						\
		schedule();						\
	}								\
	__set_current_state(TASK_RUNNING);				\
} while (0)

struct rcu_state {
	struct rcu_node node[NUM_RCU_NODES];	
	struct rcu_node *level[NUM_RCU_LVLS];	
	u32 levelcnt[MAX_RCU_LVLS + 1];		
	u8 levelspread[NUM_RCU_LVLS];		
	struct rcu_data __percpu *rda;		

	

	u8	fqs_state ____cacheline_internodealigned_in_smp;
						
	u8	fqs_active;			
						
	u8	fqs_need_gp;			
						
						
						
						
	u8	boost;				
	unsigned long gpnum;			
	unsigned long completed;		

	

	raw_spinlock_t onofflock;		
						
	raw_spinlock_t fqslock;			
						
	unsigned long jiffies_force_qs;		
						
	unsigned long n_force_qs;		
						
	unsigned long n_force_qs_lh;		
						
	unsigned long n_force_qs_ngp;		
						
	unsigned long gp_start;			
						
	unsigned long jiffies_stall;		
						
	unsigned long gp_max;			
						
	char *name;				
};


#define RCU_OFL_TASKS_NORM_GP	0x1		
						
#define RCU_OFL_TASKS_EXP_GP	0x2		
						

extern struct rcu_state rcu_sched_state;
DECLARE_PER_CPU(struct rcu_data, rcu_sched_data);

extern struct rcu_state rcu_bh_state;
DECLARE_PER_CPU(struct rcu_data, rcu_bh_data);

#ifdef CONFIG_TREE_PREEMPT_RCU
extern struct rcu_state rcu_preempt_state;
DECLARE_PER_CPU(struct rcu_data, rcu_preempt_data);
#endif 

#ifdef CONFIG_RCU_BOOST
DECLARE_PER_CPU(unsigned int, rcu_cpu_kthread_status);
DECLARE_PER_CPU(int, rcu_cpu_kthread_cpu);
DECLARE_PER_CPU(unsigned int, rcu_cpu_kthread_loops);
DECLARE_PER_CPU(char, rcu_cpu_has_work);
#endif 

#ifndef RCU_TREE_NONCORE

static void rcu_bootup_announce(void);
long rcu_batches_completed(void);
static void rcu_preempt_note_context_switch(int cpu);
static int rcu_preempt_blocked_readers_cgp(struct rcu_node *rnp);
#ifdef CONFIG_HOTPLUG_CPU
static void rcu_report_unblock_qs_rnp(struct rcu_node *rnp,
				      unsigned long flags);
static void rcu_stop_cpu_kthread(int cpu);
#endif 
static void rcu_print_detail_task_stall(struct rcu_state *rsp);
static int rcu_print_task_stall(struct rcu_node *rnp);
static void rcu_preempt_stall_reset(void);
static void rcu_preempt_check_blocked_tasks(struct rcu_node *rnp);
#ifdef CONFIG_HOTPLUG_CPU
static int rcu_preempt_offline_tasks(struct rcu_state *rsp,
				     struct rcu_node *rnp,
				     struct rcu_data *rdp);
#endif 
static void rcu_preempt_cleanup_dead_cpu(int cpu);
static void rcu_preempt_check_callbacks(int cpu);
static void rcu_preempt_process_callbacks(void);
void call_rcu(struct rcu_head *head, void (*func)(struct rcu_head *rcu));
#if defined(CONFIG_HOTPLUG_CPU) || defined(CONFIG_TREE_PREEMPT_RCU)
static void rcu_report_exp_rnp(struct rcu_state *rsp, struct rcu_node *rnp,
			       bool wake);
#endif 
static int rcu_preempt_pending(int cpu);
static int rcu_preempt_cpu_has_callbacks(int cpu);
static void __cpuinit rcu_preempt_init_percpu_data(int cpu);
static void rcu_preempt_cleanup_dying_cpu(void);
static void __init __rcu_init_preempt(void);
static void rcu_initiate_boost(struct rcu_node *rnp, unsigned long flags);
static void rcu_preempt_boost_start_gp(struct rcu_node *rnp);
static void invoke_rcu_callbacks_kthread(void);
static bool rcu_is_callbacks_kthread(void);
#ifdef CONFIG_RCU_BOOST
static void rcu_preempt_do_callbacks(void);
static void rcu_boost_kthread_setaffinity(struct rcu_node *rnp,
					  cpumask_var_t cm);
static int __cpuinit rcu_spawn_one_boost_kthread(struct rcu_state *rsp,
						 struct rcu_node *rnp,
						 int rnp_index);
static void invoke_rcu_node_kthread(struct rcu_node *rnp);
static void rcu_yield(void (*f)(unsigned long), unsigned long arg);
#endif 
static void rcu_cpu_kthread_setrt(int cpu, int to_rt);
static void __cpuinit rcu_prepare_kthreads(int cpu);
static void rcu_prepare_for_idle_init(int cpu);
static void rcu_cleanup_after_idle(int cpu);
static void rcu_prepare_for_idle(int cpu);
static void print_cpu_stall_info_begin(void);
static void print_cpu_stall_info(struct rcu_state *rsp, int cpu);
static void print_cpu_stall_info_end(void);
static void zero_cpu_stall_ticks(struct rcu_data *rdp);
static void increment_cpu_stall_ticks(void);

#endif 
