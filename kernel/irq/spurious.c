/*
 * linux/kernel/irq/spurious.c
 *
 * Copyright (C) 1992, 1998-2004 Linus Torvalds, Ingo Molnar
 *
 * This file contains spurious interrupt handling.
 */

#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/interrupt.h>
#include <linux/moduleparam.h>
#include <linux/timer.h>

#include "internals.h"

static int irqfixup __read_mostly;

#define POLL_SPURIOUS_IRQ_INTERVAL (HZ/10)
static void poll_spurious_irqs(unsigned long dummy);
static DEFINE_TIMER(poll_spurious_irq_timer, poll_spurious_irqs, 0, 0);
static int irq_poll_cpu;
static atomic_t irq_poll_active;

bool irq_wait_for_poll(struct irq_desc *desc)
{
	if (WARN_ONCE(irq_poll_cpu == smp_processor_id(),
		      "irq poll in progress on cpu %d for irq %d\n",
		      smp_processor_id(), desc->irq_data.irq))
		return false;

#ifdef CONFIG_SMP
	do {
		raw_spin_unlock(&desc->lock);
		while (irqd_irq_inprogress(&desc->irq_data))
			cpu_relax();
		raw_spin_lock(&desc->lock);
	} while (irqd_irq_inprogress(&desc->irq_data));
	
	return !irqd_irq_disabled(&desc->irq_data) && desc->action;
#else
	return false;
#endif
}


static int try_one_irq(int irq, struct irq_desc *desc, bool force)
{
	irqreturn_t ret = IRQ_NONE;
	struct irqaction *action;

	raw_spin_lock(&desc->lock);

	
	if (irq_settings_is_per_cpu(desc) || irq_settings_is_nested_thread(desc))
		goto out;

	if (irqd_irq_disabled(&desc->irq_data) && !force)
		goto out;

	action = desc->action;
	if (!action || !(action->flags & IRQF_SHARED) ||
	    (action->flags & __IRQF_TIMER) ||
	    (action->handler(irq, action->dev_id) == IRQ_HANDLED) ||
	    !action->next)
		goto out;

	
	if (irqd_irq_inprogress(&desc->irq_data)) {
		desc->istate |= IRQS_PENDING;
		goto out;
	}

	
	desc->istate |= IRQS_POLL_INPROGRESS;
	do {
		if (handle_irq_event(desc) == IRQ_HANDLED)
			ret = IRQ_HANDLED;
		action = desc->action;
	} while ((desc->istate & IRQS_PENDING) && action);
	desc->istate &= ~IRQS_POLL_INPROGRESS;
out:
	raw_spin_unlock(&desc->lock);
	return ret == IRQ_HANDLED;
}

static int misrouted_irq(int irq)
{
	struct irq_desc *desc;
	int i, ok = 0;

	if (atomic_inc_return(&irq_poll_active) != 1)
		goto out;

	irq_poll_cpu = smp_processor_id();

	for_each_irq_desc(i, desc) {
		if (!i)
			 continue;

		if (i == irq)	
			continue;

		if (try_one_irq(i, desc, false))
			ok = 1;
	}
out:
	atomic_dec(&irq_poll_active);
	
	return ok;
}

static void poll_spurious_irqs(unsigned long dummy)
{
	struct irq_desc *desc;
	int i;

	if (atomic_inc_return(&irq_poll_active) != 1)
		goto out;
	irq_poll_cpu = smp_processor_id();

	for_each_irq_desc(i, desc) {
		unsigned int state;

		if (!i)
			 continue;

		
		state = desc->istate;
		barrier();
		if (!(state & IRQS_SPURIOUS_DISABLED))
			continue;

		local_irq_disable();
		try_one_irq(i, desc, true);
		local_irq_enable();
	}
out:
	atomic_dec(&irq_poll_active);
	mod_timer(&poll_spurious_irq_timer,
		  jiffies + POLL_SPURIOUS_IRQ_INTERVAL);
}

static inline int bad_action_ret(irqreturn_t action_ret)
{
	if (likely(action_ret <= (IRQ_HANDLED | IRQ_WAKE_THREAD)))
		return 0;
	return 1;
}

static void
__report_bad_irq(unsigned int irq, struct irq_desc *desc,
		 irqreturn_t action_ret)
{
	struct irqaction *action;
	unsigned long flags;

	if (bad_action_ret(action_ret)) {
		printk(KERN_ERR "irq event %d: bogus return value %x\n",
				irq, action_ret);
	} else {
		printk(KERN_ERR "irq %d: nobody cared (try booting with "
				"the \"irqpoll\" option)\n", irq);
	}
	dump_stack();
	printk(KERN_ERR "handlers:\n");

	raw_spin_lock_irqsave(&desc->lock, flags);
	action = desc->action;
	while (action) {
		printk(KERN_ERR "[<%p>] %pf", action->handler, action->handler);
		if (action->thread_fn)
			printk(KERN_CONT " threaded [<%p>] %pf",
					action->thread_fn, action->thread_fn);
		printk(KERN_CONT "\n");
		action = action->next;
	}
	raw_spin_unlock_irqrestore(&desc->lock, flags);
}

static void
report_bad_irq(unsigned int irq, struct irq_desc *desc, irqreturn_t action_ret)
{
	static int count = 100;

	if (count > 0) {
		count--;
		__report_bad_irq(irq, desc, action_ret);
	}
}

static inline int
try_misrouted_irq(unsigned int irq, struct irq_desc *desc,
		  irqreturn_t action_ret)
{
	struct irqaction *action;

	if (!irqfixup)
		return 0;

	
	if (action_ret == IRQ_NONE)
		return 1;

	if (irqfixup < 2)
		return 0;

	if (!irq)
		return 1;

	action = desc->action;
	barrier();
	return action && (action->flags & IRQF_IRQPOLL);
}

void note_interrupt(unsigned int irq, struct irq_desc *desc,
		    irqreturn_t action_ret)
{
	if (desc->istate & IRQS_POLL_INPROGRESS)
		return;

	
	if (action_ret == IRQ_WAKE_THREAD)
		return;

	if (bad_action_ret(action_ret)) {
		report_bad_irq(irq, desc, action_ret);
		return;
	}

	if (unlikely(action_ret == IRQ_NONE)) {
		if (time_after(jiffies, desc->last_unhandled + HZ/10))
			desc->irqs_unhandled = 1;
		else
			desc->irqs_unhandled++;
		desc->last_unhandled = jiffies;
	}

	if (unlikely(try_misrouted_irq(irq, desc, action_ret))) {
		int ok = misrouted_irq(irq);
		if (action_ret == IRQ_NONE)
			desc->irqs_unhandled -= ok;
	}

	desc->irq_count++;
	if (likely(desc->irq_count < 100000))
		return;

	desc->irq_count = 0;
	if (unlikely(desc->irqs_unhandled > 99900)) {
		__report_bad_irq(irq, desc, action_ret);
		printk(KERN_EMERG "Disabling IRQ #%d\n", irq);
		desc->istate |= IRQS_SPURIOUS_DISABLED;
		desc->depth++;
		irq_disable(desc);

		mod_timer(&poll_spurious_irq_timer,
			  jiffies + POLL_SPURIOUS_IRQ_INTERVAL);
	}
	desc->irqs_unhandled = 0;
}

bool noirqdebug __read_mostly;

int noirqdebug_setup(char *str)
{
	noirqdebug = 1;
	printk(KERN_INFO "IRQ lockup detection disabled\n");

	return 1;
}

__setup("noirqdebug", noirqdebug_setup);
module_param(noirqdebug, bool, 0644);
MODULE_PARM_DESC(noirqdebug, "Disable irq lockup detection when true");

static int __init irqfixup_setup(char *str)
{
	irqfixup = 1;
	printk(KERN_WARNING "Misrouted IRQ fixup support enabled.\n");
	printk(KERN_WARNING "This may impact system performance.\n");

	return 1;
}

__setup("irqfixup", irqfixup_setup);
module_param(irqfixup, int, 0644);

static int __init irqpoll_setup(char *str)
{
	irqfixup = 2;
	printk(KERN_WARNING "Misrouted IRQ fixup and polling support "
				"enabled\n");
	printk(KERN_WARNING "This may significantly impact system "
				"performance\n");
	return 1;
}

__setup("irqpoll", irqpoll_setup);
