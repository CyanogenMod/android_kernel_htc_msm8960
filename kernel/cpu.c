/* CPU control.
 * (C) 2001, 2002, 2003, 2004 Rusty Russell
 *
 * This code is licenced under the GPL.
 */
#include <linux/proc_fs.h>
#include <linux/smp.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <linux/cpu.h>
#include <linux/export.h>
#include <linux/kthread.h>
#include <linux/stop_machine.h>
#include <linux/mutex.h>
#include <linux/gfp.h>
#include <linux/suspend.h>

#ifdef CONFIG_SMP
static DEFINE_MUTEX(cpu_add_remove_lock);

void cpu_maps_update_begin(void)
{
	mutex_lock(&cpu_add_remove_lock);
}

int cpu_maps_is_updating(void)
{
	return mutex_is_locked(&cpu_add_remove_lock);
}

void cpu_maps_update_done(void)
{
	mutex_unlock(&cpu_add_remove_lock);
}

static RAW_NOTIFIER_HEAD(cpu_chain);

static int cpu_hotplug_disabled;

#ifdef CONFIG_HOTPLUG_CPU

static struct {
	struct task_struct *active_writer;
	struct mutex lock; 
	int refcount;
} cpu_hotplug = {
	.active_writer = NULL,
	.lock = __MUTEX_INITIALIZER(cpu_hotplug.lock),
	.refcount = 0,
};

void get_online_cpus(void)
{
	might_sleep();
	if (cpu_hotplug.active_writer == current)
		return;
	mutex_lock(&cpu_hotplug.lock);
	cpu_hotplug.refcount++;
	mutex_unlock(&cpu_hotplug.lock);

}
EXPORT_SYMBOL_GPL(get_online_cpus);

void put_online_cpus(void)
{
	if (cpu_hotplug.active_writer == current)
		return;
	mutex_lock(&cpu_hotplug.lock);
	if (!--cpu_hotplug.refcount && unlikely(cpu_hotplug.active_writer))
		wake_up_process(cpu_hotplug.active_writer);
	mutex_unlock(&cpu_hotplug.lock);

}
EXPORT_SYMBOL_GPL(put_online_cpus);

static void cpu_hotplug_begin(void)
{
	cpu_hotplug.active_writer = current;

	for (;;) {
		mutex_lock(&cpu_hotplug.lock);
		if (likely(!cpu_hotplug.refcount))
			break;
		__set_current_state(TASK_UNINTERRUPTIBLE);
		mutex_unlock(&cpu_hotplug.lock);
		schedule();
	}
}

static void cpu_hotplug_done(void)
{
	cpu_hotplug.active_writer = NULL;
	mutex_unlock(&cpu_hotplug.lock);
}

#else 
static void cpu_hotplug_begin(void) {}
static void cpu_hotplug_done(void) {}
#endif	

int __ref register_cpu_notifier(struct notifier_block *nb)
{
	int ret;
	cpu_maps_update_begin();
	ret = raw_notifier_chain_register(&cpu_chain, nb);
	cpu_maps_update_done();
	return ret;
}

static int __cpu_notify(unsigned long val, void *v, int nr_to_call,
			int *nr_calls)
{
	int ret;

	ret = __raw_notifier_call_chain(&cpu_chain, val, v, nr_to_call,
					nr_calls);

	return notifier_to_errno(ret);
}

static int cpu_notify(unsigned long val, void *v)
{
	return __cpu_notify(val, v, -1, NULL);
}

#ifdef CONFIG_HOTPLUG_CPU

static void cpu_notify_nofail(unsigned long val, void *v)
{
	BUG_ON(cpu_notify(val, v));
}
EXPORT_SYMBOL(register_cpu_notifier);

void __ref unregister_cpu_notifier(struct notifier_block *nb)
{
	cpu_maps_update_begin();
	raw_notifier_chain_unregister(&cpu_chain, nb);
	cpu_maps_update_done();
}
EXPORT_SYMBOL(unregister_cpu_notifier);

static inline void check_for_tasks(int cpu)
{
	struct task_struct *p;

	write_lock_irq(&tasklist_lock);
	for_each_process(p) {
		if (task_cpu(p) == cpu && p->state == TASK_RUNNING &&
		    (p->utime || p->stime))
			printk(KERN_WARNING "Task %s (pid = %d) is on cpu %d "
				"(state = %ld, flags = %x)\n",
				p->comm, task_pid_nr(p), cpu,
				p->state, p->flags);
	}
	write_unlock_irq(&tasklist_lock);
}

struct take_cpu_down_param {
	unsigned long mod;
	void *hcpu;
};

static int __ref take_cpu_down(void *_param)
{
	struct take_cpu_down_param *param = _param;
	int err;

	
	err = __cpu_disable();
	if (err < 0)
		return err;

	cpu_notify(CPU_DYING | param->mod, param->hcpu);
	return 0;
}

int skip_cpu_offline = 0;
static int __ref _cpu_down(unsigned int cpu, int tasks_frozen)
{
	int err, nr_calls = 0;
	void *hcpu = (void *)(long)cpu;
	unsigned long mod = tasks_frozen ? CPU_TASKS_FROZEN : 0;
	struct take_cpu_down_param tcd_param = {
		.mod = mod,
		.hcpu = hcpu,
	};

	if (skip_cpu_offline)
		return -EACCES;

	if (num_online_cpus() == 1)
		return -EBUSY;

	if (!cpu_online(cpu))
		return -EINVAL;

	cpu_hotplug_begin();

	err = __cpu_notify(CPU_DOWN_PREPARE | mod, hcpu, -1, &nr_calls);
	if (err) {
		nr_calls--;
		__cpu_notify(CPU_DOWN_FAILED | mod, hcpu, nr_calls, NULL);
		printk("%s: attempt to take down CPU %u failed\n",
				__func__, cpu);
		goto out_release;
	}

	err = __stop_machine(take_cpu_down, &tcd_param, cpumask_of(cpu));
	if (err) {
		
		cpu_notify_nofail(CPU_DOWN_FAILED | mod, hcpu);

		goto out_release;
	}
	BUG_ON(cpu_online(cpu));

	while (!idle_cpu(cpu))
		cpu_relax();

	
	__cpu_die(cpu);

	
	cpu_notify_nofail(CPU_DEAD | mod, hcpu);

	check_for_tasks(cpu);

out_release:
	cpu_hotplug_done();
	if (!err)
		cpu_notify_nofail(CPU_POST_DEAD | mod, hcpu);
	return err;
}

extern void trace_cpu_down_frequency (unsigned int cpu);
int __ref cpu_down(unsigned int cpu)
{
	int err;

	cpu_maps_update_begin();

	if (cpu_hotplug_disabled) {
		err = -EBUSY;
		goto out;
	}

	err = _cpu_down(cpu, 0);

out:
	cpu_maps_update_done();

	
	if (!err)
		trace_cpu_down_frequency (cpu);

	return err;
}
EXPORT_SYMBOL(cpu_down);
#endif 

static int __cpuinit _cpu_up(unsigned int cpu, int tasks_frozen)
{
	int ret, nr_calls = 0;
	void *hcpu = (void *)(long)cpu;
	unsigned long mod = tasks_frozen ? CPU_TASKS_FROZEN : 0;

	if (cpu_online(cpu) || !cpu_present(cpu))
		return -EINVAL;

	cpu_hotplug_begin();
	ret = __cpu_notify(CPU_UP_PREPARE | mod, hcpu, -1, &nr_calls);
	if (ret) {
		nr_calls--;
		printk(KERN_WARNING "%s: attempt to bring up CPU %u failed\n",
				__func__, cpu);
		goto out_notify;
	}

	
	ret = __cpu_up(cpu);
	if (ret != 0)
		goto out_notify;
	BUG_ON(!cpu_online(cpu));

	
	cpu_notify(CPU_ONLINE | mod, hcpu);

out_notify:
	if (ret != 0)
		__cpu_notify(CPU_UP_CANCELED | mod, hcpu, nr_calls, NULL);
	cpu_hotplug_done();

	return ret;
}

extern void trace_cpu_up_frequency (unsigned int cpu);
int __cpuinit cpu_up(unsigned int cpu)
{
	int err = 0;

#ifdef	CONFIG_MEMORY_HOTPLUG
	int nid;
	pg_data_t	*pgdat;
#endif

	if (!cpu_possible(cpu)) {
		printk(KERN_ERR "can't online cpu %d because it is not "
			"configured as may-hotadd at boot time\n", cpu);
#if defined(CONFIG_IA64)
		printk(KERN_ERR "please check additional_cpus= boot "
				"parameter\n");
#endif
		return -EINVAL;
	}

#ifdef	CONFIG_MEMORY_HOTPLUG
	nid = cpu_to_node(cpu);
	if (!node_online(nid)) {
		err = mem_online_node(nid);
		if (err)
			return err;
	}

	pgdat = NODE_DATA(nid);
	if (!pgdat) {
		printk(KERN_ERR
			"Can't online cpu %d due to NULL pgdat\n", cpu);
		return -ENOMEM;
	}

	if (pgdat->node_zonelists->_zonerefs->zone == NULL) {
		mutex_lock(&zonelists_mutex);
		build_all_zonelists(NULL);
		mutex_unlock(&zonelists_mutex);
	}
#endif

	cpu_maps_update_begin();

	if (cpu_hotplug_disabled) {
		err = -EBUSY;
		goto out;
	}

	err = _cpu_up(cpu, 0);

out:
	cpu_maps_update_done();
	
	if (!err)
		trace_cpu_up_frequency (cpu);
	return err;
}
EXPORT_SYMBOL_GPL(cpu_up);

#ifdef CONFIG_PM_SLEEP_SMP
static cpumask_var_t frozen_cpus;

void __weak arch_disable_nonboot_cpus_begin(void)
{
}

void __weak arch_disable_nonboot_cpus_end(void)
{
}

int disable_nonboot_cpus(void)
{
	int cpu, first_cpu, error = 0;

	cpu_maps_update_begin();
	first_cpu = cpumask_first(cpu_online_mask);
	cpumask_clear(frozen_cpus);
	arch_disable_nonboot_cpus_begin();

	printk("Disabling non-boot CPUs ...\n");
	for_each_online_cpu(cpu) {
		if (cpu == first_cpu)
			continue;
		error = _cpu_down(cpu, 1);
		if (!error)
			cpumask_set_cpu(cpu, frozen_cpus);
		else {
			printk(KERN_ERR "Error taking CPU%d down: %d\n",
				cpu, error);
			break;
		}
	}

	arch_disable_nonboot_cpus_end();

	if (!error) {
		BUG_ON(num_online_cpus() > 1);
		
		cpu_hotplug_disabled = 1;
	} else {
		printk(KERN_ERR "Non-boot CPUs are not disabled\n");
	}
	cpu_maps_update_done();
	return error;
}

void __weak arch_enable_nonboot_cpus_begin(void)
{
}

void __weak arch_enable_nonboot_cpus_end(void)
{
}

void __ref enable_nonboot_cpus(void)
{
	int cpu, error;

	
	cpu_maps_update_begin();
	cpu_hotplug_disabled = 0;
	if (cpumask_empty(frozen_cpus))
		goto out;

	printk(KERN_INFO "Enabling non-boot CPUs ...\n");

	arch_enable_nonboot_cpus_begin();

	for_each_cpu(cpu, frozen_cpus) {
		error = _cpu_up(cpu, 1);
		if (!error) {
			printk(KERN_INFO "CPU%d is up\n", cpu);
			continue;
		}
		printk(KERN_WARNING "Error taking CPU%d up: %d\n", cpu, error);
	}

	arch_enable_nonboot_cpus_end();

	cpumask_clear(frozen_cpus);
out:
	cpu_maps_update_done();
}

static int __init alloc_frozen_cpus(void)
{
	if (!alloc_cpumask_var(&frozen_cpus, GFP_KERNEL|__GFP_ZERO))
		return -ENOMEM;
	return 0;
}
core_initcall(alloc_frozen_cpus);

void cpu_hotplug_disable_before_freeze(void)
{
	cpu_maps_update_begin();
	cpu_hotplug_disabled = 1;
	cpu_maps_update_done();
}


void cpu_hotplug_enable_after_thaw(void)
{
	cpu_maps_update_begin();
	cpu_hotplug_disabled = 0;
	cpu_maps_update_done();
}

static int
cpu_hotplug_pm_callback(struct notifier_block *nb,
			unsigned long action, void *ptr)
{
	switch (action) {

	case PM_SUSPEND_PREPARE:
	case PM_HIBERNATION_PREPARE:
		cpu_hotplug_disable_before_freeze();
		break;

	case PM_POST_SUSPEND:
	case PM_POST_HIBERNATION:
		cpu_hotplug_enable_after_thaw();
		break;

	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}


static int __init cpu_hotplug_pm_sync_init(void)
{
	pm_notifier(cpu_hotplug_pm_callback, 0);
	return 0;
}
core_initcall(cpu_hotplug_pm_sync_init);

#endif 

void __cpuinit notify_cpu_starting(unsigned int cpu)
{
	unsigned long val = CPU_STARTING;

#ifdef CONFIG_PM_SLEEP_SMP
	if (frozen_cpus != NULL && cpumask_test_cpu(cpu, frozen_cpus))
		val = CPU_STARTING_FROZEN;
#endif 
	cpu_notify(val, (void *)(long)cpu);
}

#endif 


#define MASK_DECLARE_1(x)	[x+1][0] = (1UL << (x))
#define MASK_DECLARE_2(x)	MASK_DECLARE_1(x), MASK_DECLARE_1(x+1)
#define MASK_DECLARE_4(x)	MASK_DECLARE_2(x), MASK_DECLARE_2(x+2)
#define MASK_DECLARE_8(x)	MASK_DECLARE_4(x), MASK_DECLARE_4(x+4)

const unsigned long cpu_bit_bitmap[BITS_PER_LONG+1][BITS_TO_LONGS(NR_CPUS)] = {

	MASK_DECLARE_8(0),	MASK_DECLARE_8(8),
	MASK_DECLARE_8(16),	MASK_DECLARE_8(24),
#if BITS_PER_LONG > 32
	MASK_DECLARE_8(32),	MASK_DECLARE_8(40),
	MASK_DECLARE_8(48),	MASK_DECLARE_8(56),
#endif
};
EXPORT_SYMBOL_GPL(cpu_bit_bitmap);

const DECLARE_BITMAP(cpu_all_bits, NR_CPUS) = CPU_BITS_ALL;
EXPORT_SYMBOL(cpu_all_bits);

#ifdef CONFIG_INIT_ALL_POSSIBLE
static DECLARE_BITMAP(cpu_possible_bits, CONFIG_NR_CPUS) __read_mostly
	= CPU_BITS_ALL;
#else
static DECLARE_BITMAP(cpu_possible_bits, CONFIG_NR_CPUS) __read_mostly;
#endif
const struct cpumask *const cpu_possible_mask = to_cpumask(cpu_possible_bits);
EXPORT_SYMBOL(cpu_possible_mask);

static DECLARE_BITMAP(cpu_online_bits, CONFIG_NR_CPUS) __read_mostly;
const struct cpumask *const cpu_online_mask = to_cpumask(cpu_online_bits);
EXPORT_SYMBOL(cpu_online_mask);

static DECLARE_BITMAP(cpu_present_bits, CONFIG_NR_CPUS) __read_mostly;
const struct cpumask *const cpu_present_mask = to_cpumask(cpu_present_bits);
EXPORT_SYMBOL(cpu_present_mask);

static DECLARE_BITMAP(cpu_active_bits, CONFIG_NR_CPUS) __read_mostly;
const struct cpumask *const cpu_active_mask = to_cpumask(cpu_active_bits);
EXPORT_SYMBOL(cpu_active_mask);

void set_cpu_possible(unsigned int cpu, bool possible)
{
	if (possible)
		cpumask_set_cpu(cpu, to_cpumask(cpu_possible_bits));
	else
		cpumask_clear_cpu(cpu, to_cpumask(cpu_possible_bits));
}

void set_cpu_present(unsigned int cpu, bool present)
{
	if (present)
		cpumask_set_cpu(cpu, to_cpumask(cpu_present_bits));
	else
		cpumask_clear_cpu(cpu, to_cpumask(cpu_present_bits));
}

void set_cpu_online(unsigned int cpu, bool online)
{
	if (online)
		cpumask_set_cpu(cpu, to_cpumask(cpu_online_bits));
	else
		cpumask_clear_cpu(cpu, to_cpumask(cpu_online_bits));
}

void set_cpu_active(unsigned int cpu, bool active)
{
	if (active)
		cpumask_set_cpu(cpu, to_cpumask(cpu_active_bits));
	else
		cpumask_clear_cpu(cpu, to_cpumask(cpu_active_bits));
}

void init_cpu_present(const struct cpumask *src)
{
	cpumask_copy(to_cpumask(cpu_present_bits), src);
}

void init_cpu_possible(const struct cpumask *src)
{
	cpumask_copy(to_cpumask(cpu_possible_bits), src);
}

void init_cpu_online(const struct cpumask *src)
{
	cpumask_copy(to_cpumask(cpu_online_bits), src);
}

static ATOMIC_NOTIFIER_HEAD(idle_notifier);

void idle_notifier_register(struct notifier_block *n)
{
	atomic_notifier_chain_register(&idle_notifier, n);
}
EXPORT_SYMBOL_GPL(idle_notifier_register);

void idle_notifier_unregister(struct notifier_block *n)
{
	atomic_notifier_chain_unregister(&idle_notifier, n);
}
EXPORT_SYMBOL_GPL(idle_notifier_unregister);

void idle_notifier_call_chain(unsigned long val)
{
	atomic_notifier_call_chain(&idle_notifier, val, NULL);
}
EXPORT_SYMBOL_GPL(idle_notifier_call_chain);
