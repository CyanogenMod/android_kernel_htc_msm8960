#include <linux/rcupdate.h>
#include <linux/rculist.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/percpu.h>
#include <linux/init.h>
#include <linux/gfp.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#ifdef CONFIG_DEBUG_CSD_LOCK
#include <linux/reboot.h>
#endif

#ifdef CONFIG_USE_GENERIC_SMP_HELPERS
static struct {
	struct list_head	queue;
	raw_spinlock_t		lock;
} call_function __cacheline_aligned_in_smp =
	{
		.queue		= LIST_HEAD_INIT(call_function.queue),
		.lock		= __RAW_SPIN_LOCK_UNLOCKED(call_function.lock),
	};

enum {
	CSD_FLAG_LOCK		= 0x01,
};

struct call_function_data {
	struct call_single_data	csd;
	atomic_t		refs;
	cpumask_var_t		cpumask;
#ifdef CONFIG_DEBUG_CSD_LOCK
	cpumask_var_t		cpumask_run;
#endif
};

static DEFINE_PER_CPU_SHARED_ALIGNED(struct call_function_data, cfd_data);

struct call_single_queue {
	struct list_head	list;
	raw_spinlock_t		lock;
};

static DEFINE_PER_CPU_SHARED_ALIGNED(struct call_single_queue, call_single_queue);

static int
hotplug_cfd(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
	long cpu = (long)hcpu;
	struct call_function_data *cfd = &per_cpu(cfd_data, cpu);

	switch (action) {
	case CPU_UP_PREPARE:
	case CPU_UP_PREPARE_FROZEN:
		if (!zalloc_cpumask_var_node(&cfd->cpumask, GFP_KERNEL,
				cpu_to_node(cpu)))
			return notifier_from_errno(-ENOMEM);
		break;

#ifdef CONFIG_HOTPLUG_CPU
	case CPU_UP_CANCELED:
	case CPU_UP_CANCELED_FROZEN:

	case CPU_DEAD:
	case CPU_DEAD_FROZEN:
		free_cpumask_var(cfd->cpumask);
		break;
#endif
	};

	return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata hotplug_cfd_notifier = {
	.notifier_call		= hotplug_cfd,
};

void __init call_function_init(void)
{
	void *cpu = (void *)(long)smp_processor_id();
	int i;

	for_each_possible_cpu(i) {
		struct call_single_queue *q = &per_cpu(call_single_queue, i);

		raw_spin_lock_init(&q->lock);
		INIT_LIST_HEAD(&q->list);
	}

	hotplug_cfd(&hotplug_cfd_notifier, CPU_UP_PREPARE, cpu);
	register_cpu_notifier(&hotplug_cfd_notifier);
}

#ifdef CONFIG_DEBUG_CSD_LOCK
#define CSD_LOCK_WAIT_TIMEOUT_MS	(CONFIG_DEBUG_CSD_LOCK_TIMEOUT_MS)
static void csd_info_dump(void)
{
	struct call_function_data *data;

	char buf_cpu_run[16] = {0};
	char buf_cpu_wait[16] = {0};
	smp_call_func_t func;

	
	pr_info("%s: CSD Queue:\n", __func__);
	list_for_each_entry_rcu(data, &call_function.queue, csd.list) {
		func = data->csd.func;

		cpulist_scnprintf(buf_cpu_run, sizeof(buf_cpu_run), data->cpumask_run);
		cpulist_scnprintf(buf_cpu_wait, sizeof(buf_cpu_wait), data->cpumask);
		pr_info("Entry: Func=[<%08lx>] (%pS), Ref=%d, CpuRun=%s, CpuWait=%s\n",
			(unsigned long) func, (void *)func,
			atomic_read(&data->refs),
			buf_cpu_run, buf_cpu_wait);
	}
}
#endif

static void csd_lock_wait(struct call_single_data *data)
{
#ifdef CONFIG_DEBUG_CSD_LOCK
	unsigned long start = jiffies;
	unsigned long now;
#endif

	while (data->flags & CSD_FLAG_LOCK) {
		cpu_relax();

#ifdef CONFIG_DEBUG_CSD_LOCK
		now = jiffies;
		
		if (now < start)
			start = now;

		
		if (((jiffies_to_msecs(now - start)) > CSD_LOCK_WAIT_TIMEOUT_MS)) {
			WARN(1, "%s: CSD lock waiting time exceeds %d miliseconds.\n", __func__, CSD_LOCK_WAIT_TIMEOUT_MS);
			csd_info_dump();
			kernel_restart("force-dog-bark");
		}
#endif
	}
}

static void csd_lock(struct call_single_data *data)
{
	csd_lock_wait(data);
	data->flags = CSD_FLAG_LOCK;

	smp_mb();
}

static void csd_unlock(struct call_single_data *data)
{
	WARN_ON(!(data->flags & CSD_FLAG_LOCK));

	smp_mb();

	data->flags &= ~CSD_FLAG_LOCK;
}

static
void generic_exec_single(int cpu, struct call_single_data *data, int wait)
{
	struct call_single_queue *dst = &per_cpu(call_single_queue, cpu);
	unsigned long flags;
	int ipi;

	raw_spin_lock_irqsave(&dst->lock, flags);
	ipi = list_empty(&dst->list);
	list_add_tail(&data->list, &dst->list);
	raw_spin_unlock_irqrestore(&dst->lock, flags);


	smp_mb();

	if (ipi || wait)
		arch_send_call_function_single_ipi(cpu);

	if (wait)
		csd_lock_wait(data);
}

void generic_smp_call_function_interrupt(void)
{
	struct call_function_data *data;
	int cpu = smp_processor_id();

	WARN_ON_ONCE(!cpu_online(cpu));

	smp_mb();

	list_for_each_entry_rcu(data, &call_function.queue, csd.list) {
		int refs;
		smp_call_func_t func;


		if (!cpumask_test_cpu(cpu, data->cpumask))
			continue;

		smp_rmb();

		if (atomic_read(&data->refs) == 0)
			continue;

#ifdef CONFIG_DEBUG_CSD_LOCK
		
		cpumask_set_cpu(cpu, data->cpumask_run);
#endif

		func = data->csd.func;		
		func(data->csd.info);

		if (!cpumask_test_and_clear_cpu(cpu, data->cpumask)) {
			WARN(1, "%pf enabled interrupts and double executed\n", func);
			continue;
		}

		refs = atomic_dec_return(&data->refs);
		WARN_ON(refs < 0);

		if (refs)
			continue;

		WARN_ON(!cpumask_empty(data->cpumask));

		raw_spin_lock(&call_function.lock);
		list_del_rcu(&data->csd.list);
		raw_spin_unlock(&call_function.lock);

		csd_unlock(&data->csd);
	}

}

void generic_smp_call_function_single_interrupt(void)
{
	struct call_single_queue *q = &__get_cpu_var(call_single_queue);
	unsigned int data_flags;
	LIST_HEAD(list);

	WARN_ON_ONCE(!cpu_online(smp_processor_id()));

	raw_spin_lock(&q->lock);
	list_replace_init(&q->list, &list);
	raw_spin_unlock(&q->lock);

	while (!list_empty(&list)) {
		struct call_single_data *data;

		data = list_entry(list.next, struct call_single_data, list);
		list_del(&data->list);

		data_flags = data->flags;

		data->func(data->info);

		if (data_flags & CSD_FLAG_LOCK)
			csd_unlock(data);
	}
}

static DEFINE_PER_CPU_SHARED_ALIGNED(struct call_single_data, csd_data);

int smp_call_function_single(int cpu, smp_call_func_t func, void *info,
			     int wait)
{
	struct call_single_data d = {
		.flags = 0,
	};
	unsigned long flags;
	int this_cpu;
	int err = 0;

	this_cpu = get_cpu();

	WARN_ON_ONCE(cpu_online(this_cpu) && irqs_disabled()
		     && !oops_in_progress);

	if (cpu == this_cpu) {
		local_irq_save(flags);
		func(info);
		local_irq_restore(flags);
	} else {
		if ((unsigned)cpu < nr_cpu_ids && cpu_online(cpu)) {
			struct call_single_data *data = &d;

			if (!wait)
				data = &__get_cpu_var(csd_data);

			csd_lock(data);

			data->func = func;
			data->info = info;
			generic_exec_single(cpu, data, wait);
		} else {
			err = -ENXIO;	
		}
	}

	put_cpu();

	return err;
}
EXPORT_SYMBOL(smp_call_function_single);

int smp_call_function_any(const struct cpumask *mask,
			  smp_call_func_t func, void *info, int wait)
{
	unsigned int cpu;
	const struct cpumask *nodemask;
	int ret;

	
	cpu = get_cpu();
	if (cpumask_test_cpu(cpu, mask))
		goto call;

	
	nodemask = cpumask_of_node(cpu_to_node(cpu));
	for (cpu = cpumask_first_and(nodemask, mask); cpu < nr_cpu_ids;
	     cpu = cpumask_next_and(cpu, nodemask, mask)) {
		if (cpu_online(cpu))
			goto call;
	}

	
	cpu = cpumask_any_and(mask, cpu_online_mask);
call:
	ret = smp_call_function_single(cpu, func, info, wait);
	put_cpu();
	return ret;
}
EXPORT_SYMBOL_GPL(smp_call_function_any);

void __smp_call_function_single(int cpu, struct call_single_data *data,
				int wait)
{
	unsigned int this_cpu;
	unsigned long flags;

	this_cpu = get_cpu();
	WARN_ON_ONCE(cpu_online(smp_processor_id()) && wait && irqs_disabled()
		     && !oops_in_progress);

	if (cpu == this_cpu) {
		local_irq_save(flags);
		data->func(data->info);
		local_irq_restore(flags);
	} else {
		csd_lock(data);
		generic_exec_single(cpu, data, wait);
	}
	put_cpu();
}

void smp_call_function_many(const struct cpumask *mask,
			    smp_call_func_t func, void *info, bool wait)
{
	struct call_function_data *data;
	unsigned long flags;
	int refs, cpu, next_cpu, this_cpu = smp_processor_id();

	WARN_ON_ONCE(cpu_online(this_cpu) && irqs_disabled()
		     && !oops_in_progress && !early_boot_irqs_disabled);

	
	cpu = cpumask_first_and(mask, cpu_online_mask);
	if (cpu == this_cpu)
		cpu = cpumask_next_and(cpu, mask, cpu_online_mask);

	
	if (cpu >= nr_cpu_ids)
		return;

	
	next_cpu = cpumask_next_and(cpu, mask, cpu_online_mask);
	if (next_cpu == this_cpu)
		next_cpu = cpumask_next_and(next_cpu, mask, cpu_online_mask);

	
	if (next_cpu >= nr_cpu_ids) {
		smp_call_function_single(cpu, func, info, wait);
		return;
	}

	data = &__get_cpu_var(cfd_data);
	csd_lock(&data->csd);

	
	BUG_ON(atomic_read(&data->refs) || !cpumask_empty(data->cpumask));

	atomic_set(&data->refs, 0);	

	data->csd.func = func;
	data->csd.info = info;

	
	smp_wmb();

#ifdef CONFIG_DEBUG_CSD_LOCK
	
	cpumask_clear(data->cpumask_run);
#endif

	
	cpumask_and(data->cpumask, mask, cpu_online_mask);
	cpumask_clear_cpu(this_cpu, data->cpumask);
	refs = cpumask_weight(data->cpumask);

	
	if (unlikely(!refs)) {
		csd_unlock(&data->csd);
		return;
	}

	raw_spin_lock_irqsave(&call_function.lock, flags);
	list_add_rcu(&data->csd.list, &call_function.queue);
	atomic_set(&data->refs, refs);
	raw_spin_unlock_irqrestore(&call_function.lock, flags);

	smp_mb();

	
	arch_send_call_function_ipi_mask(data->cpumask);

	
	if (wait)
		csd_lock_wait(&data->csd);
}
EXPORT_SYMBOL(smp_call_function_many);

int smp_call_function(smp_call_func_t func, void *info, int wait)
{
	preempt_disable();
	smp_call_function_many(cpu_online_mask, func, info, wait);
	preempt_enable();

	return 0;
}
EXPORT_SYMBOL(smp_call_function);

void ipi_call_lock(void)
{
	raw_spin_lock(&call_function.lock);
}

void ipi_call_unlock(void)
{
	raw_spin_unlock(&call_function.lock);
}

void ipi_call_lock_irq(void)
{
	raw_spin_lock_irq(&call_function.lock);
}

void ipi_call_unlock_irq(void)
{
	raw_spin_unlock_irq(&call_function.lock);
}
#endif 

unsigned int setup_max_cpus = NR_CPUS;
EXPORT_SYMBOL(setup_max_cpus);



void __weak arch_disable_smp_support(void) { }

static int __init nosmp(char *str)
{
	setup_max_cpus = 0;
	arch_disable_smp_support();

	return 0;
}

early_param("nosmp", nosmp);

static int __init nrcpus(char *str)
{
	int nr_cpus;

	get_option(&str, &nr_cpus);
	if (nr_cpus > 0 && nr_cpus < nr_cpu_ids)
		nr_cpu_ids = nr_cpus;

	return 0;
}

early_param("nr_cpus", nrcpus);

static int __init maxcpus(char *str)
{
	get_option(&str, &setup_max_cpus);
	if (setup_max_cpus == 0)
		arch_disable_smp_support();

	return 0;
}

early_param("maxcpus", maxcpus);

int nr_cpu_ids __read_mostly = NR_CPUS;
EXPORT_SYMBOL(nr_cpu_ids);

void __init setup_nr_cpu_ids(void)
{
	nr_cpu_ids = find_last_bit(cpumask_bits(cpu_possible_mask),NR_CPUS) + 1;
}

void __init smp_init(void)
{
	unsigned int cpu;

	
	for_each_present_cpu(cpu) {
		if (num_online_cpus() >= setup_max_cpus)
			break;
		if (!cpu_online(cpu))
			cpu_up(cpu);
	}

	
	printk(KERN_INFO "Brought up %ld CPUs\n", (long)num_online_cpus());
	smp_cpus_done(setup_max_cpus);
}

int on_each_cpu(void (*func) (void *info), void *info, int wait)
{
	unsigned long flags;
	int ret = 0;

	preempt_disable();
	ret = smp_call_function(func, info, wait);
	local_irq_save(flags);
	func(info);
	local_irq_restore(flags);
	preempt_enable();
	return ret;
}
EXPORT_SYMBOL(on_each_cpu);

void on_each_cpu_mask(const struct cpumask *mask, smp_call_func_t func,
			void *info, bool wait)
{
	int cpu = get_cpu();

	smp_call_function_many(mask, func, info, wait);
	if (cpumask_test_cpu(cpu, mask)) {
		local_irq_disable();
		func(info);
		local_irq_enable();
	}
	put_cpu();
}
EXPORT_SYMBOL(on_each_cpu_mask);

void on_each_cpu_cond(bool (*cond_func)(int cpu, void *info),
			smp_call_func_t func, void *info, bool wait,
			gfp_t gfp_flags)
{
	cpumask_var_t cpus;
	int cpu, ret;

	might_sleep_if(gfp_flags & __GFP_WAIT);

	if (likely(zalloc_cpumask_var(&cpus, (gfp_flags|__GFP_NOWARN)))) {
		preempt_disable();
		for_each_online_cpu(cpu)
			if (cond_func(cpu, info))
				cpumask_set_cpu(cpu, cpus);
		on_each_cpu_mask(cpus, func, info, wait);
		preempt_enable();
		free_cpumask_var(cpus);
	} else {
		preempt_disable();
		for_each_online_cpu(cpu)
			if (cond_func(cpu, info)) {
				ret = smp_call_function_single(cpu, func,
								info, wait);
				WARN_ON_ONCE(!ret);
			}
		preempt_enable();
	}
}
EXPORT_SYMBOL(on_each_cpu_cond);
