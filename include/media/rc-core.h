/*
 * Remote Controller core header
 *
 * Copyright (C) 2009-2010 by Mauro Carvalho Chehab <mchehab@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#ifndef _RC_CORE
#define _RC_CORE

#include <linux/spinlock.h>
#include <linux/kfifo.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <media/rc-map.h>

extern int rc_core_debug;
#define IR_dprintk(level, fmt, ...)				\
do {								\
	if (rc_core_debug >= level)				\
		pr_debug("%s: " fmt, __func__, ##__VA_ARGS__);	\
} while (0)

enum rc_driver_type {
	RC_DRIVER_SCANCODE = 0,	
	RC_DRIVER_IR_RAW,	
};

struct rc_dev {
	struct device			dev;
	const char			*input_name;
	const char			*input_phys;
	struct input_id			input_id;
	char				*driver_name;
	const char			*map_name;
	struct rc_map			rc_map;
	struct mutex			lock;
	unsigned long			devno;
	struct ir_raw_event_ctrl	*raw;
	struct input_dev		*input_dev;
	enum rc_driver_type		driver_type;
	bool				idle;
	u64				allowed_protos;
	u32				scanmask;
	void				*priv;
	spinlock_t			keylock;
	bool				keypressed;
	unsigned long			keyup_jiffies;
	struct timer_list		timer_keyup;
	u32				last_keycode;
	u32				last_scancode;
	u8				last_toggle;
	u32				timeout;
	u32				min_timeout;
	u32				max_timeout;
	u32				rx_resolution;
	u32				tx_resolution;
	int				(*change_protocol)(struct rc_dev *dev, u64 rc_type);
	int				(*open)(struct rc_dev *dev);
	void				(*close)(struct rc_dev *dev);
	int				(*s_tx_mask)(struct rc_dev *dev, u32 mask);
	int				(*s_tx_carrier)(struct rc_dev *dev, u32 carrier);
	int				(*s_tx_duty_cycle)(struct rc_dev *dev, u32 duty_cycle);
	int				(*s_rx_carrier_range)(struct rc_dev *dev, u32 min, u32 max);
	int				(*tx_ir)(struct rc_dev *dev, unsigned *txbuf, unsigned n);
	void				(*s_idle)(struct rc_dev *dev, bool enable);
	int				(*s_learning_mode)(struct rc_dev *dev, int enable);
	int				(*s_carrier_report) (struct rc_dev *dev, int enable);
};

#define to_rc_dev(d) container_of(d, struct rc_dev, dev)


struct rc_dev *rc_allocate_device(void);
void rc_free_device(struct rc_dev *dev);
int rc_register_device(struct rc_dev *dev);
void rc_unregister_device(struct rc_dev *dev);

void rc_repeat(struct rc_dev *dev);
void rc_keydown(struct rc_dev *dev, int scancode, u8 toggle);
void rc_keydown_notimeout(struct rc_dev *dev, int scancode, u8 toggle);
void rc_keyup(struct rc_dev *dev);
u32 rc_g_keycode_from_table(struct rc_dev *dev, u32 scancode);


enum raw_event_type {
	IR_SPACE        = (1 << 0),
	IR_PULSE        = (1 << 1),
	IR_START_EVENT  = (1 << 2),
	IR_STOP_EVENT   = (1 << 3),
};

struct ir_raw_event {
	union {
		u32             duration;

		struct {
			u32     carrier;
			u8      duty_cycle;
		};
	};

	unsigned                pulse:1;
	unsigned                reset:1;
	unsigned                timeout:1;
	unsigned                carrier_report:1;
};

#define DEFINE_IR_RAW_EVENT(event) \
	struct ir_raw_event event = { \
		{ .duration = 0 } , \
		.pulse = 0, \
		.reset = 0, \
		.timeout = 0, \
		.carrier_report = 0 }

static inline void init_ir_raw_event(struct ir_raw_event *ev)
{
	memset(ev, 0, sizeof(*ev));
}

#define IR_MAX_DURATION         0xFFFFFFFF      
#define US_TO_NS(usec)		((usec) * 1000)
#define MS_TO_US(msec)		((msec) * 1000)
#define MS_TO_NS(msec)		((msec) * 1000 * 1000)

void ir_raw_event_handle(struct rc_dev *dev);
int ir_raw_event_store(struct rc_dev *dev, struct ir_raw_event *ev);
int ir_raw_event_store_edge(struct rc_dev *dev, enum raw_event_type type);
int ir_raw_event_store_with_filter(struct rc_dev *dev,
				struct ir_raw_event *ev);
void ir_raw_event_set_idle(struct rc_dev *dev, bool idle);

static inline void ir_raw_event_reset(struct rc_dev *dev)
{
	DEFINE_IR_RAW_EVENT(ev);
	ev.reset = true;

	ir_raw_event_store(dev, &ev);
	ir_raw_event_handle(dev);
}

static inline u32 ir_extract_bits(u32 data, u32 mask)
{
	u32 vbit = 1, value = 0;

	do {
		if (mask & 1) {
			if (data & 1)
				value |= vbit;
			vbit <<= 1;
		}
		data >>= 1;
	} while (mask >>= 1);

	return value;
}

#endif 
