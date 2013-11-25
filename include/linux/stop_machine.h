#ifndef _LINUX_STOP_MACHINE
#define _LINUX_STOP_MACHINE

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/smp.h>
#include <linux/list.h>

typedef int (*cpu_stop_fn_t)(void *arg);

#ifdef CONFIG_SMP

struct cpu_stop_work {
	struct list_head	list;		
	cpu_stop_fn_t		fn;
	void			*arg;
	struct cpu_stop_done	*done;
};

extern struct mutex stop_cpus_mutex;

int stop_one_cpu(unsigned int cpu, cpu_stop_fn_t fn, void *arg);
void stop_one_cpu_nowait(unsigned int cpu, cpu_stop_fn_t fn, void *arg,
			 struct cpu_stop_work *work_buf);
int stop_cpus(const struct cpumask *cpumask, cpu_stop_fn_t fn, void *arg);
int try_stop_cpus(const struct cpumask *cpumask, cpu_stop_fn_t fn, void *arg);

#else	

#include <linux/workqueue.h>

struct cpu_stop_work {
	struct work_struct	work;
	cpu_stop_fn_t		fn;
	void			*arg;
};

static inline int stop_one_cpu(unsigned int cpu, cpu_stop_fn_t fn, void *arg)
{
	int ret = -ENOENT;
	preempt_disable();
	if (cpu == smp_processor_id())
		ret = fn(arg);
	preempt_enable();
	return ret;
}

static void stop_one_cpu_nowait_workfn(struct work_struct *work)
{
	struct cpu_stop_work *stwork =
		container_of(work, struct cpu_stop_work, work);
	preempt_disable();
	stwork->fn(stwork->arg);
	preempt_enable();
}

static inline void stop_one_cpu_nowait(unsigned int cpu,
				       cpu_stop_fn_t fn, void *arg,
				       struct cpu_stop_work *work_buf)
{
	if (cpu == smp_processor_id()) {
		INIT_WORK(&work_buf->work, stop_one_cpu_nowait_workfn);
		work_buf->fn = fn;
		work_buf->arg = arg;
		schedule_work(&work_buf->work);
	}
}

static inline int stop_cpus(const struct cpumask *cpumask,
			    cpu_stop_fn_t fn, void *arg)
{
	if (cpumask_test_cpu(raw_smp_processor_id(), cpumask))
		return stop_one_cpu(raw_smp_processor_id(), fn, arg);
	return -ENOENT;
}

static inline int try_stop_cpus(const struct cpumask *cpumask,
				cpu_stop_fn_t fn, void *arg)
{
	return stop_cpus(cpumask, fn, arg);
}

#endif	

#if defined(CONFIG_STOP_MACHINE) && defined(CONFIG_SMP)

int stop_machine(int (*fn)(void *), void *data, const struct cpumask *cpus);

int __stop_machine(int (*fn)(void *), void *data, const struct cpumask *cpus);

int stop_machine_from_inactive_cpu(int (*fn)(void *), void *data,
				   const struct cpumask *cpus);

#else	 

static inline int __stop_machine(int (*fn)(void *), void *data,
				 const struct cpumask *cpus)
{
	unsigned long flags;
	int ret;
	local_irq_save(flags);
	ret = fn(data);
	local_irq_restore(flags);
	return ret;
}

static inline int stop_machine(int (*fn)(void *), void *data,
			       const struct cpumask *cpus)
{
	return __stop_machine(fn, data, cpus);
}

static inline int stop_machine_from_inactive_cpu(int (*fn)(void *), void *data,
						 const struct cpumask *cpus)
{
	return __stop_machine(fn, data, cpus);
}

#endif	
#endif	
