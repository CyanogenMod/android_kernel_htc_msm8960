/*
 * Input Multitouch Library
 *
 * Copyright (c) 2008-2010 Henrik Rydberg
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <linux/input/mt.h>
#include <linux/export.h>
#include <linux/slab.h>

#define TRKID_SGN	((TRKID_MAX + 1) >> 1)

int input_mt_init_slots(struct input_dev *dev, unsigned int num_slots)
{
	int i;

	if (!num_slots)
		return 0;
	if (dev->mt)
		return dev->mtsize != num_slots ? -EINVAL : 0;

	dev->mt = kcalloc(num_slots, sizeof(struct input_mt_slot), GFP_KERNEL);
	if (!dev->mt)
		return -ENOMEM;

	dev->mtsize = num_slots;
	input_set_abs_params(dev, ABS_MT_SLOT, 0, num_slots - 1, 0, 0);
	input_set_abs_params(dev, ABS_MT_TRACKING_ID, 0, TRKID_MAX, 0, 0);
	input_set_events_per_packet(dev, 6 * num_slots);

	
	for (i = 0; i < num_slots; i++)
		input_mt_set_value(&dev->mt[i], ABS_MT_TRACKING_ID, -1);

	return 0;
}
EXPORT_SYMBOL(input_mt_init_slots);

void input_mt_destroy_slots(struct input_dev *dev)
{
	kfree(dev->mt);
	dev->mt = NULL;
	dev->mtsize = 0;
	dev->slot = 0;
	dev->trkid = 0;
}
EXPORT_SYMBOL(input_mt_destroy_slots);

void input_mt_report_slot_state(struct input_dev *dev,
				unsigned int tool_type, bool active)
{
	struct input_mt_slot *mt;
	int id;

	if (!dev->mt || !active) {
		input_event(dev, EV_ABS, ABS_MT_TRACKING_ID, -1);
		return;
	}

	mt = &dev->mt[dev->slot];
	id = input_mt_get_value(mt, ABS_MT_TRACKING_ID);
	if (id < 0 || input_mt_get_value(mt, ABS_MT_TOOL_TYPE) != tool_type)
		id = input_mt_new_trkid(dev);

	input_event(dev, EV_ABS, ABS_MT_TRACKING_ID, id);
	input_event(dev, EV_ABS, ABS_MT_TOOL_TYPE, tool_type);
}
EXPORT_SYMBOL(input_mt_report_slot_state);

void input_mt_report_finger_count(struct input_dev *dev, int count)
{
	input_event(dev, EV_KEY, BTN_TOOL_FINGER, count == 1);
	input_event(dev, EV_KEY, BTN_TOOL_DOUBLETAP, count == 2);
	input_event(dev, EV_KEY, BTN_TOOL_TRIPLETAP, count == 3);
	input_event(dev, EV_KEY, BTN_TOOL_QUADTAP, count == 4);
	input_event(dev, EV_KEY, BTN_TOOL_QUINTTAP, count == 5);
}
EXPORT_SYMBOL(input_mt_report_finger_count);

void input_mt_report_pointer_emulation(struct input_dev *dev, bool use_count)
{
	struct input_mt_slot *oldest = 0;
	int oldid = dev->trkid;
	int count = 0;
	int i;

	for (i = 0; i < dev->mtsize; ++i) {
		struct input_mt_slot *ps = &dev->mt[i];
		int id = input_mt_get_value(ps, ABS_MT_TRACKING_ID);

		if (id < 0)
			continue;
		if ((id - oldid) & TRKID_SGN) {
			oldest = ps;
			oldid = id;
		}
		count++;
	}

	input_event(dev, EV_KEY, BTN_TOUCH, count > 0);
	if (use_count)
		input_mt_report_finger_count(dev, count);

	if (oldest) {
		int x = input_mt_get_value(oldest, ABS_MT_POSITION_X);
		int y = input_mt_get_value(oldest, ABS_MT_POSITION_Y);
		int p = input_mt_get_value(oldest, ABS_MT_PRESSURE);

		input_event(dev, EV_ABS, ABS_X, x);
		input_event(dev, EV_ABS, ABS_Y, y);
		input_event(dev, EV_ABS, ABS_PRESSURE, p);
	} else {
		input_event(dev, EV_ABS, ABS_PRESSURE, 0);
	}
}
EXPORT_SYMBOL(input_mt_report_pointer_emulation);
