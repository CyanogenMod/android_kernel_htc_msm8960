/* ir-raw.c - handle IR pulse/space events
 *
 * Copyright (C) 2010 by Mauro Carvalho Chehab <mchehab@redhat.com>
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

#include <linux/export.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/kmod.h>
#include <linux/sched.h>
#include <linux/freezer.h>
#include "rc-core-priv.h"

#define MAX_IR_EVENT_SIZE      512

static LIST_HEAD(ir_raw_client_list);

static DEFINE_MUTEX(ir_raw_handler_lock);
static LIST_HEAD(ir_raw_handler_list);
static u64 available_protocols;

#ifdef MODULE
static struct work_struct wq_load;
#endif

static int ir_raw_event_thread(void *data)
{
	struct ir_raw_event ev;
	struct ir_raw_handler *handler;
	struct ir_raw_event_ctrl *raw = (struct ir_raw_event_ctrl *)data;
	int retval;

	while (!kthread_should_stop()) {

		spin_lock_irq(&raw->lock);
		retval = kfifo_out(&raw->kfifo, &ev, sizeof(ev));

		if (!retval) {
			set_current_state(TASK_INTERRUPTIBLE);

			if (kthread_should_stop())
				set_current_state(TASK_RUNNING);

			spin_unlock_irq(&raw->lock);
			schedule();
			continue;
		}

		spin_unlock_irq(&raw->lock);


		BUG_ON(retval != sizeof(ev));

		mutex_lock(&ir_raw_handler_lock);
		list_for_each_entry(handler, &ir_raw_handler_list, list)
			handler->decode(raw->dev, ev);
		raw->prev_ev = ev;
		mutex_unlock(&ir_raw_handler_lock);
	}

	return 0;
}

int ir_raw_event_store(struct rc_dev *dev, struct ir_raw_event *ev)
{
	if (!dev->raw)
		return -EINVAL;

	IR_dprintk(2, "sample: (%05dus %s)\n",
		   TO_US(ev->duration), TO_STR(ev->pulse));

	if (kfifo_in(&dev->raw->kfifo, ev, sizeof(*ev)) != sizeof(*ev))
		return -ENOMEM;

	return 0;
}
EXPORT_SYMBOL_GPL(ir_raw_event_store);

int ir_raw_event_store_edge(struct rc_dev *dev, enum raw_event_type type)
{
	ktime_t			now;
	s64			delta; 
	DEFINE_IR_RAW_EVENT(ev);
	int			rc = 0;
	int			delay;

	if (!dev->raw)
		return -EINVAL;

	now = ktime_get();
	delta = ktime_to_ns(ktime_sub(now, dev->raw->last_event));
	delay = MS_TO_NS(dev->input_dev->rep[REP_DELAY]);

	if (delta > delay || !dev->raw->last_type)
		type |= IR_START_EVENT;
	else
		ev.duration = delta;

	if (type & IR_START_EVENT)
		ir_raw_event_reset(dev);
	else if (dev->raw->last_type & IR_SPACE) {
		ev.pulse = false;
		rc = ir_raw_event_store(dev, &ev);
	} else if (dev->raw->last_type & IR_PULSE) {
		ev.pulse = true;
		rc = ir_raw_event_store(dev, &ev);
	} else
		return 0;

	dev->raw->last_event = now;
	dev->raw->last_type = type;
	return rc;
}
EXPORT_SYMBOL_GPL(ir_raw_event_store_edge);

int ir_raw_event_store_with_filter(struct rc_dev *dev, struct ir_raw_event *ev)
{
	if (!dev->raw)
		return -EINVAL;

	
	if (dev->idle && !ev->pulse)
		return 0;
	else if (dev->idle)
		ir_raw_event_set_idle(dev, false);

	if (!dev->raw->this_ev.duration)
		dev->raw->this_ev = *ev;
	else if (ev->pulse == dev->raw->this_ev.pulse)
		dev->raw->this_ev.duration += ev->duration;
	else {
		ir_raw_event_store(dev, &dev->raw->this_ev);
		dev->raw->this_ev = *ev;
	}

	
	if (!ev->pulse && dev->timeout &&
	    dev->raw->this_ev.duration >= dev->timeout)
		ir_raw_event_set_idle(dev, true);

	return 0;
}
EXPORT_SYMBOL_GPL(ir_raw_event_store_with_filter);

void ir_raw_event_set_idle(struct rc_dev *dev, bool idle)
{
	if (!dev->raw)
		return;

	IR_dprintk(2, "%s idle mode\n", idle ? "enter" : "leave");

	if (idle) {
		dev->raw->this_ev.timeout = true;
		ir_raw_event_store(dev, &dev->raw->this_ev);
		init_ir_raw_event(&dev->raw->this_ev);
	}

	if (dev->s_idle)
		dev->s_idle(dev, idle);

	dev->idle = idle;
}
EXPORT_SYMBOL_GPL(ir_raw_event_set_idle);

void ir_raw_event_handle(struct rc_dev *dev)
{
	unsigned long flags;

	if (!dev->raw)
		return;

	spin_lock_irqsave(&dev->raw->lock, flags);
	wake_up_process(dev->raw->thread);
	spin_unlock_irqrestore(&dev->raw->lock, flags);
}
EXPORT_SYMBOL_GPL(ir_raw_event_handle);

u64
ir_raw_get_allowed_protocols(void)
{
	u64 protocols;
	mutex_lock(&ir_raw_handler_lock);
	protocols = available_protocols;
	mutex_unlock(&ir_raw_handler_lock);
	return protocols;
}

int ir_raw_event_register(struct rc_dev *dev)
{
	int rc;
	struct ir_raw_handler *handler;

	if (!dev)
		return -EINVAL;

	dev->raw = kzalloc(sizeof(*dev->raw), GFP_KERNEL);
	if (!dev->raw)
		return -ENOMEM;

	dev->raw->dev = dev;
	dev->raw->enabled_protocols = ~0;
	rc = kfifo_alloc(&dev->raw->kfifo,
			 sizeof(struct ir_raw_event) * MAX_IR_EVENT_SIZE,
			 GFP_KERNEL);
	if (rc < 0)
		goto out;

	spin_lock_init(&dev->raw->lock);
	dev->raw->thread = kthread_run(ir_raw_event_thread, dev->raw,
				       "rc%ld", dev->devno);

	if (IS_ERR(dev->raw->thread)) {
		rc = PTR_ERR(dev->raw->thread);
		goto out;
	}

	mutex_lock(&ir_raw_handler_lock);
	list_add_tail(&dev->raw->list, &ir_raw_client_list);
	list_for_each_entry(handler, &ir_raw_handler_list, list)
		if (handler->raw_register)
			handler->raw_register(dev);
	mutex_unlock(&ir_raw_handler_lock);

	return 0;

out:
	kfree(dev->raw);
	dev->raw = NULL;
	return rc;
}

void ir_raw_event_unregister(struct rc_dev *dev)
{
	struct ir_raw_handler *handler;

	if (!dev || !dev->raw)
		return;

	kthread_stop(dev->raw->thread);

	mutex_lock(&ir_raw_handler_lock);
	list_del(&dev->raw->list);
	list_for_each_entry(handler, &ir_raw_handler_list, list)
		if (handler->raw_unregister)
			handler->raw_unregister(dev);
	mutex_unlock(&ir_raw_handler_lock);

	kfifo_free(&dev->raw->kfifo);
	kfree(dev->raw);
	dev->raw = NULL;
}


int ir_raw_handler_register(struct ir_raw_handler *ir_raw_handler)
{
	struct ir_raw_event_ctrl *raw;

	mutex_lock(&ir_raw_handler_lock);
	list_add_tail(&ir_raw_handler->list, &ir_raw_handler_list);
	if (ir_raw_handler->raw_register)
		list_for_each_entry(raw, &ir_raw_client_list, list)
			ir_raw_handler->raw_register(raw->dev);
	available_protocols |= ir_raw_handler->protocols;
	mutex_unlock(&ir_raw_handler_lock);

	return 0;
}
EXPORT_SYMBOL(ir_raw_handler_register);

void ir_raw_handler_unregister(struct ir_raw_handler *ir_raw_handler)
{
	struct ir_raw_event_ctrl *raw;

	mutex_lock(&ir_raw_handler_lock);
	list_del(&ir_raw_handler->list);
	if (ir_raw_handler->raw_unregister)
		list_for_each_entry(raw, &ir_raw_client_list, list)
			ir_raw_handler->raw_unregister(raw->dev);
	available_protocols &= ~ir_raw_handler->protocols;
	mutex_unlock(&ir_raw_handler_lock);
}
EXPORT_SYMBOL(ir_raw_handler_unregister);

#ifdef MODULE
static void init_decoders(struct work_struct *work)
{
	

	load_nec_decode();
	load_rc5_decode();
	load_rc6_decode();
	load_jvc_decode();
	load_sony_decode();
	load_sanyo_decode();
	load_mce_kbd_decode();
	load_lirc_codec();

}
#endif

void ir_raw_init(void)
{
#ifdef MODULE
	INIT_WORK(&wq_load, init_decoders);
	schedule_work(&wq_load);
#endif
}
