/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ARCH_ARM_MACH_MSM_EVENT_TIMER_H
#define __ARCH_ARM_MACH_MSM_EVENT_TIMER_H

#include <linux/hrtimer.h>

struct event_timer_info;

#ifdef CONFIG_MSM_EVENT_TIMER
struct event_timer_info *add_event_timer(void (*function)(void *), void *data);

void activate_event_timer(struct event_timer_info *event, ktime_t event_time);

void deactivate_event_timer(struct event_timer_info *event);

void destroy_event_timer(struct event_timer_info *event);

ktime_t get_next_event_time(void);
#else
static inline void *add_event_timer(void (*function)(void *), void *data)
{
	return NULL;
}

static inline void activate_event_timer(void *event, ktime_t event_time) {}

static inline void deactivate_event_timer(void *event) {}

static inline void destroy_event_timer(void *event) {}

static inline ktime_t get_next_event_time(void)
{
	return ns_to_ktime(0);
}

#endif 
#endif 
