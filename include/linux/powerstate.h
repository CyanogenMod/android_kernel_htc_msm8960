/* include/linux/powerstate.h
 *
 * Copyright (C) 2007-2008 Google, Inc.
 * Copyright (C) 2011 HTC Corporation
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_POWERSTATE_H
#define _LINUX_POWERSTATE_H

#include <linux/list.h>
#include <linux/ktime.h>

enum {
	POWER_STATE_STATE,
	POWER_STATE_TRIGGER,
	POWER_STATE_TYPE_COUNT
};

struct power_state {
#ifdef CONFIG_POWERSTATE
	struct list_head    link;
	int                 flags;
	const char         *name;
	struct {
		int             count;
		ktime_t         total_time;
		ktime_t         max_time;
		ktime_t         last_time;
	} stat;
#endif
};

#ifdef CONFIG_POWERSTATE

void power_state_init(struct power_state *state, const char *name);
void power_state_destroy(struct power_state *state);
void power_state_start(struct power_state *state);
void power_state_trigger(struct power_state *state);
void power_state_end(struct power_state *state);
int power_state_active(struct power_state *state);

#else

static inline void power_state_init(struct power_state *state, const char *name) {}
static inline void power_state_destroy(struct power_state *state) {}
static inline void power_state_start(struct power_state *state) {}
static inline void power_state_trigger(struct power_state *state) {}
static inline void power_state_end(struct power_state *state) {}
static inline int power_state_active(struct power_state *state) { return 0; }

#endif

#endif

