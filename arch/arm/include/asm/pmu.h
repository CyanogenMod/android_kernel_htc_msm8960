/*
 *  linux/arch/arm/include/asm/pmu.h
 *
 *  Copyright (C) 2009 picoChip Designs Ltd, Jamie Iles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __ARM_PMU_H__
#define __ARM_PMU_H__

#include <linux/interrupt.h>
#include <linux/perf_event.h>

enum arm_pmu_type {
	ARM_PMU_DEVICE_CPU	= 0,
	ARM_PMU_DEVICE_L2CC	= 1,
	ARM_NUM_PMU_DEVICES,
};

struct arm_pmu_platdata {
	irqreturn_t (*handle_irq)(int irq, void *dev,
				  irq_handler_t pmu_handler);
	void (*enable_irq)(int irq);
	void (*disable_irq)(int irq);
};

#ifdef CONFIG_CPU_HAS_PMU

extern int
reserve_pmu(enum arm_pmu_type type);

extern void
release_pmu(enum arm_pmu_type type);

#else 

#include <linux/err.h>

static inline int
reserve_pmu(enum arm_pmu_type type)
{
	return -ENODEV;
}

static inline void
release_pmu(enum arm_pmu_type type)	{ }

#endif 

#ifdef CONFIG_HW_PERF_EVENTS

struct pmu_hw_events {
	struct perf_event	**events;

	unsigned long           *used_mask;

	raw_spinlock_t		pmu_lock;
};

struct arm_pmu {
	struct pmu	pmu;
	enum arm_perf_pmu_ids id;
	enum arm_pmu_type type;
	cpumask_t	active_irqs;
	const char	*name;
	irqreturn_t	(*handle_irq)(int irq_num, void *dev);
	int     	(*request_pmu_irq)(int irq, irq_handler_t *irq_h);
	void    	(*free_pmu_irq)(int irq);
	void		(*enable)(struct hw_perf_event *evt, int idx, int cpu);
	void		(*disable)(struct hw_perf_event *evt, int idx);
	int		(*get_event_idx)(struct pmu_hw_events *hw_events,
					 struct hw_perf_event *hwc);
	int		(*set_event_filter)(struct hw_perf_event *evt,
					    struct perf_event_attr *attr);
	u32		(*read_counter)(int idx);
	void		(*write_counter)(int idx, u32 val);
	void		(*start)(void);
	void		(*stop)(void);
	void		(*reset)(void *);
	int		(*map_event)(struct perf_event *event);
	int		num_events;
	atomic_t	active_events;
	struct mutex	reserve_mutex;
	u64		max_period;
	struct platform_device	*plat_device;
	struct pmu_hw_events	*(*get_hw_events)(void);
	int	(*test_set_event_constraints)(struct perf_event *event);
	int	(*clear_event_constraints)(struct perf_event *event);
};

#define to_arm_pmu(p) (container_of(p, struct arm_pmu, pmu))

int armpmu_register(struct arm_pmu *armpmu, char *name, int type);

u64 armpmu_event_update(struct perf_event *event,
			struct hw_perf_event *hwc,
			int idx);

int armpmu_event_set_period(struct perf_event *event,
			    struct hw_perf_event *hwc,
			    int idx);

#endif 

#endif 
