#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/interrupt.h>
#include <linux/cpu.h>
#include <linux/blk-iopoll.h>
#include <linux/delay.h>

#include "blk.h"

int blk_iopoll_enabled = 1;
EXPORT_SYMBOL(blk_iopoll_enabled);

static unsigned int blk_iopoll_budget __read_mostly = 256;

static DEFINE_PER_CPU(struct list_head, blk_cpu_iopoll);

void blk_iopoll_sched(struct blk_iopoll *iop)
{
	unsigned long flags;

	local_irq_save(flags);
	list_add_tail(&iop->list, &__get_cpu_var(blk_cpu_iopoll));
	__raise_softirq_irqoff(BLOCK_IOPOLL_SOFTIRQ);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(blk_iopoll_sched);

void __blk_iopoll_complete(struct blk_iopoll *iop)
{
	list_del(&iop->list);
	smp_mb__before_clear_bit();
	clear_bit_unlock(IOPOLL_F_SCHED, &iop->state);
}
EXPORT_SYMBOL(__blk_iopoll_complete);

void blk_iopoll_complete(struct blk_iopoll *iopoll)
{
	unsigned long flags;

	local_irq_save(flags);
	__blk_iopoll_complete(iopoll);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(blk_iopoll_complete);

static void blk_iopoll_softirq(struct softirq_action *h)
{
	struct list_head *list = &__get_cpu_var(blk_cpu_iopoll);
	int rearm = 0, budget = blk_iopoll_budget;
	unsigned long start_time = jiffies;

	local_irq_disable();

	while (!list_empty(list)) {
		struct blk_iopoll *iop;
		int work, weight;

		if (budget <= 0 || time_after(jiffies, start_time)) {
			rearm = 1;
			break;
		}

		local_irq_enable();

		iop = list_entry(list->next, struct blk_iopoll, list);

		weight = iop->weight;
		work = 0;
		if (test_bit(IOPOLL_F_SCHED, &iop->state))
			work = iop->poll(iop, weight);

		budget -= work;

		local_irq_disable();

		if (work >= weight) {
			if (blk_iopoll_disable_pending(iop))
				__blk_iopoll_complete(iop);
			else
				list_move_tail(&iop->list, list);
		}
	}

	if (rearm)
		__raise_softirq_irqoff(BLOCK_IOPOLL_SOFTIRQ);

	local_irq_enable();
}

void blk_iopoll_disable(struct blk_iopoll *iop)
{
	set_bit(IOPOLL_F_DISABLE, &iop->state);
	while (test_and_set_bit(IOPOLL_F_SCHED, &iop->state))
		msleep(1);
	clear_bit(IOPOLL_F_DISABLE, &iop->state);
}
EXPORT_SYMBOL(blk_iopoll_disable);

void blk_iopoll_enable(struct blk_iopoll *iop)
{
	BUG_ON(!test_bit(IOPOLL_F_SCHED, &iop->state));
	smp_mb__before_clear_bit();
	clear_bit_unlock(IOPOLL_F_SCHED, &iop->state);
}
EXPORT_SYMBOL(blk_iopoll_enable);

void blk_iopoll_init(struct blk_iopoll *iop, int weight, blk_iopoll_fn *poll_fn)
{
	memset(iop, 0, sizeof(*iop));
	INIT_LIST_HEAD(&iop->list);
	iop->weight = weight;
	iop->poll = poll_fn;
	set_bit(IOPOLL_F_SCHED, &iop->state);
}
EXPORT_SYMBOL(blk_iopoll_init);

static int __cpuinit blk_iopoll_cpu_notify(struct notifier_block *self,
					  unsigned long action, void *hcpu)
{
	if (action == CPU_DEAD || action == CPU_DEAD_FROZEN) {
		int cpu = (unsigned long) hcpu;

		local_irq_disable();
		list_splice_init(&per_cpu(blk_cpu_iopoll, cpu),
				 &__get_cpu_var(blk_cpu_iopoll));
		__raise_softirq_irqoff(BLOCK_IOPOLL_SOFTIRQ);
		local_irq_enable();
	}

	return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata blk_iopoll_cpu_notifier = {
	.notifier_call	= blk_iopoll_cpu_notify,
};

static __init int blk_iopoll_setup(void)
{
	int i;

	for_each_possible_cpu(i)
		INIT_LIST_HEAD(&per_cpu(blk_cpu_iopoll, i));

	open_softirq(BLOCK_IOPOLL_SOFTIRQ, blk_iopoll_softirq);
	register_hotcpu_notifier(&blk_iopoll_cpu_notifier);
	return 0;
}
subsys_initcall(blk_iopoll_setup);
