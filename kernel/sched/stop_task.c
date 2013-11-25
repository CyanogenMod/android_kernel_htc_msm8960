#include "sched.h"


#ifdef CONFIG_SMP
static int
select_task_rq_stop(struct task_struct *p, int sd_flag, int flags)
{
	return task_cpu(p); 
}
#endif 

static void
check_preempt_curr_stop(struct rq *rq, struct task_struct *p, int flags)
{
	
}

static struct task_struct *pick_next_task_stop(struct rq *rq)
{
	struct task_struct *stop = rq->stop;

	if (stop && stop->on_rq) {
		stop->se.exec_start = rq->clock_task;
		return stop;
	}

	return NULL;
}

static void
enqueue_task_stop(struct rq *rq, struct task_struct *p, int flags)
{
	inc_nr_running(rq);
}

static void
dequeue_task_stop(struct rq *rq, struct task_struct *p, int flags)
{
	dec_nr_running(rq);
}

static void yield_task_stop(struct rq *rq)
{
	BUG(); 
}

static void put_prev_task_stop(struct rq *rq, struct task_struct *prev)
{
	struct task_struct *curr = rq->curr;
	u64 delta_exec;

	delta_exec = rq->clock_task - curr->se.exec_start;
	if (unlikely((s64)delta_exec < 0))
		delta_exec = 0;

	schedstat_set(curr->se.statistics.exec_max,
			max(curr->se.statistics.exec_max, delta_exec));

	curr->se.sum_exec_runtime += delta_exec;
	account_group_exec_runtime(curr, delta_exec);

	curr->se.exec_start = rq->clock_task;
	cpuacct_charge(curr, delta_exec);
}

static void task_tick_stop(struct rq *rq, struct task_struct *curr, int queued)
{
}

static void set_curr_task_stop(struct rq *rq)
{
	struct task_struct *stop = rq->stop;

	stop->se.exec_start = rq->clock_task;
}

static void switched_to_stop(struct rq *rq, struct task_struct *p)
{
	BUG(); 
}

static void
prio_changed_stop(struct rq *rq, struct task_struct *p, int oldprio)
{
	BUG(); 
}

static unsigned int
get_rr_interval_stop(struct rq *rq, struct task_struct *task)
{
	return 0;
}

const struct sched_class stop_sched_class = {
	.next			= &rt_sched_class,

	.enqueue_task		= enqueue_task_stop,
	.dequeue_task		= dequeue_task_stop,
	.yield_task		= yield_task_stop,

	.check_preempt_curr	= check_preempt_curr_stop,

	.pick_next_task		= pick_next_task_stop,
	.put_prev_task		= put_prev_task_stop,

#ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_stop,
#endif

	.set_curr_task          = set_curr_task_stop,
	.task_tick		= task_tick_stop,

	.get_rr_interval	= get_rr_interval_stop,

	.prio_changed		= prio_changed_stop,
	.switched_to		= switched_to_stop,
};
