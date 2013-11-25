/*
 * linux/kernel/irq/autoprobe.c
 *
 * Copyright (C) 1992, 1998-2004 Linus Torvalds, Ingo Molnar
 *
 * This file contains the interrupt probing code and driver APIs.
 */

#include <linux/irq.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/async.h>

#include "internals.h"

static DEFINE_MUTEX(probing_active);

unsigned long probe_irq_on(void)
{
	struct irq_desc *desc;
	unsigned long mask = 0;
	int i;

	async_synchronize_full();
	mutex_lock(&probing_active);
	for_each_irq_desc_reverse(i, desc) {
		raw_spin_lock_irq(&desc->lock);
		if (!desc->action && irq_settings_can_probe(desc)) {
			if (desc->irq_data.chip->irq_set_type)
				desc->irq_data.chip->irq_set_type(&desc->irq_data,
							 IRQ_TYPE_PROBE);
			irq_startup(desc, false);
		}
		raw_spin_unlock_irq(&desc->lock);
	}

	
	msleep(20);

	for_each_irq_desc_reverse(i, desc) {
		raw_spin_lock_irq(&desc->lock);
		if (!desc->action && irq_settings_can_probe(desc)) {
			desc->istate |= IRQS_AUTODETECT | IRQS_WAITING;
			if (irq_startup(desc, false))
				desc->istate |= IRQS_PENDING;
		}
		raw_spin_unlock_irq(&desc->lock);
	}

	msleep(100);

	for_each_irq_desc(i, desc) {
		raw_spin_lock_irq(&desc->lock);

		if (desc->istate & IRQS_AUTODETECT) {
			
			if (!(desc->istate & IRQS_WAITING)) {
				desc->istate &= ~IRQS_AUTODETECT;
				irq_shutdown(desc);
			} else
				if (i < 32)
					mask |= 1 << i;
		}
		raw_spin_unlock_irq(&desc->lock);
	}

	return mask;
}
EXPORT_SYMBOL(probe_irq_on);

unsigned int probe_irq_mask(unsigned long val)
{
	unsigned int mask = 0;
	struct irq_desc *desc;
	int i;

	for_each_irq_desc(i, desc) {
		raw_spin_lock_irq(&desc->lock);
		if (desc->istate & IRQS_AUTODETECT) {
			if (i < 16 && !(desc->istate & IRQS_WAITING))
				mask |= 1 << i;

			desc->istate &= ~IRQS_AUTODETECT;
			irq_shutdown(desc);
		}
		raw_spin_unlock_irq(&desc->lock);
	}
	mutex_unlock(&probing_active);

	return mask & val;
}
EXPORT_SYMBOL(probe_irq_mask);

int probe_irq_off(unsigned long val)
{
	int i, irq_found = 0, nr_of_irqs = 0;
	struct irq_desc *desc;

	for_each_irq_desc(i, desc) {
		raw_spin_lock_irq(&desc->lock);

		if (desc->istate & IRQS_AUTODETECT) {
			if (!(desc->istate & IRQS_WAITING)) {
				if (!nr_of_irqs)
					irq_found = i;
				nr_of_irqs++;
			}
			desc->istate &= ~IRQS_AUTODETECT;
			irq_shutdown(desc);
		}
		raw_spin_unlock_irq(&desc->lock);
	}
	mutex_unlock(&probing_active);

	if (nr_of_irqs > 1)
		irq_found = -irq_found;

	return irq_found;
}
EXPORT_SYMBOL(probe_irq_off);

