/*
 * FLoating proportions
 *
 *  Copyright (C) 2007 Red Hat, Inc., Peter Zijlstra <pzijlstr@redhat.com>
 *
 * This file contains the public data structure and API definitions.
 */

#ifndef _LINUX_PROPORTIONS_H
#define _LINUX_PROPORTIONS_H

#include <linux/percpu_counter.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>

struct prop_global {
	int shift;
	struct percpu_counter events;
};

struct prop_descriptor {
	int index;
	struct prop_global pg[2];
	struct mutex mutex;		
};

int prop_descriptor_init(struct prop_descriptor *pd, int shift);
void prop_change_shift(struct prop_descriptor *pd, int new_shift);


struct prop_local_percpu {
	struct percpu_counter events;

	int shift;
	unsigned long period;
	raw_spinlock_t lock;		
};

int prop_local_init_percpu(struct prop_local_percpu *pl);
void prop_local_destroy_percpu(struct prop_local_percpu *pl);
void __prop_inc_percpu(struct prop_descriptor *pd, struct prop_local_percpu *pl);
void prop_fraction_percpu(struct prop_descriptor *pd, struct prop_local_percpu *pl,
		long *numerator, long *denominator);

static inline
void prop_inc_percpu(struct prop_descriptor *pd, struct prop_local_percpu *pl)
{
	unsigned long flags;

	local_irq_save(flags);
	__prop_inc_percpu(pd, pl);
	local_irq_restore(flags);
}

#if BITS_PER_LONG == 32
#define PROP_MAX_SHIFT (3*BITS_PER_LONG/4)
#else
#define PROP_MAX_SHIFT (BITS_PER_LONG/2)
#endif

#define PROP_FRAC_SHIFT		(BITS_PER_LONG - PROP_MAX_SHIFT - 1)
#define PROP_FRAC_BASE		(1UL << PROP_FRAC_SHIFT)

void __prop_inc_percpu_max(struct prop_descriptor *pd,
			   struct prop_local_percpu *pl, long frac);



struct prop_local_single {
	unsigned long events;

	unsigned long period;
	int shift;
	raw_spinlock_t lock;		
};

#define INIT_PROP_LOCAL_SINGLE(name)			\
{	.lock = __RAW_SPIN_LOCK_UNLOCKED(name.lock),	\
}

int prop_local_init_single(struct prop_local_single *pl);
void prop_local_destroy_single(struct prop_local_single *pl);
void __prop_inc_single(struct prop_descriptor *pd, struct prop_local_single *pl);
void prop_fraction_single(struct prop_descriptor *pd, struct prop_local_single *pl,
		long *numerator, long *denominator);

static inline
void prop_inc_single(struct prop_descriptor *pd, struct prop_local_single *pl)
{
	unsigned long flags;

	local_irq_save(flags);
	__prop_inc_single(pd, pl);
	local_irq_restore(flags);
}

#endif 
