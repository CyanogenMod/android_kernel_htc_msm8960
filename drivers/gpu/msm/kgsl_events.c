/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
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

#include <linux/slab.h>
#include <linux/list.h>
#include <linux/module.h>
#include <kgsl_device.h>

#include "kgsl_trace.h"

static void _add_event_to_list(struct list_head *head, struct kgsl_event *event)
{
	struct list_head *n;

	for (n = head->next; n != head; n = n->next) {
		struct kgsl_event *e =
			list_entry(n, struct kgsl_event, list);

		if (timestamp_cmp(e->timestamp, event->timestamp) > 0) {
			list_add(&event->list, n->prev);
			break;
		}
	}

	if (n == head)
		list_add_tail(&event->list, head);
}

int kgsl_add_event(struct kgsl_device *device, u32 id, u32 ts,
	void (*cb)(struct kgsl_device *, void *, u32, u32), void *priv,
	void *owner)
{
	struct kgsl_event *event;
	unsigned int cur_ts;
	struct kgsl_context *context = NULL;

	if (cb == NULL)
		return -EINVAL;

	if (id != KGSL_MEMSTORE_GLOBAL) {
		context = idr_find(&device->context_idr, id);
		if (context == NULL)
			return -EINVAL;
	}
	cur_ts = kgsl_readtimestamp(device, context, KGSL_TIMESTAMP_RETIRED);


	if (timestamp_cmp(cur_ts, ts) >= 0) {
		trace_kgsl_fire_event(id, ts, 0);
		cb(device, priv, id, ts);
		return 0;
	}

	event = kzalloc(sizeof(*event), GFP_KERNEL);
	if (event == NULL)
		return -ENOMEM;

	event->context = context;
	event->timestamp = ts;
	event->priv = priv;
	event->func = cb;
	event->owner = owner;
	event->created = jiffies;

	trace_kgsl_register_event(id, ts);

	
	if (context)
		kgsl_context_get(context);

	

	if (context) {
		_add_event_to_list(&context->events, event);


		if (list_empty(&context->events_list))
			list_add_tail(&context->events_list,
				&device->events_pending_list);

	} else
		_add_event_to_list(&device->events, event);


	device->active_cnt++;

	queue_work(device->work_queue, &device->ts_expired_ws);
	return 0;
}
EXPORT_SYMBOL(kgsl_add_event);

void kgsl_cancel_events_ctxt(struct kgsl_device *device,
	struct kgsl_context *context)
{
	struct kgsl_event *event, *event_tmp;
	unsigned int id, cur;

	cur = kgsl_readtimestamp(device, context, KGSL_TIMESTAMP_RETIRED);
	id = context->id;

	list_for_each_entry_safe(event, event_tmp, &context->events, list) {

		trace_kgsl_fire_event(id, cur, jiffies - event->created);

		if (event->func)
			event->func(device, event->priv, id, cur);

		kgsl_context_put(context);
		list_del(&event->list);
		kfree(event);

		kgsl_active_count_put(device);
	}

	
	list_del_init(&context->events_list);
}

void kgsl_cancel_events(struct kgsl_device *device,
	void *owner)
{
	struct kgsl_event *event, *event_tmp;
	unsigned int cur;

	cur = kgsl_readtimestamp(device, NULL, KGSL_TIMESTAMP_RETIRED);

	list_for_each_entry_safe(event, event_tmp, &device->events, list) {
		if (event->owner != owner)
			continue;


		trace_kgsl_fire_event(KGSL_MEMSTORE_GLOBAL, cur,
			jiffies - event->created);

		if (event->func)
			event->func(device, event->priv, KGSL_MEMSTORE_GLOBAL,
				cur);

		if (event->context)
			kgsl_context_put(event->context);

		list_del(&event->list);
		kfree(event);

		kgsl_active_count_put(device);
	}
}
EXPORT_SYMBOL(kgsl_cancel_events);

static void _process_event_list(struct kgsl_device *device,
		struct list_head *head, unsigned int timestamp)
{
	struct kgsl_event *event, *tmp;
	unsigned int id;

	list_for_each_entry_safe(event, tmp, head, list) {
		if (timestamp_cmp(timestamp, event->timestamp) < 0)
			break;

		id = event->context ? event->context->id : KGSL_MEMSTORE_GLOBAL;


		trace_kgsl_fire_event(id, event->timestamp,
			jiffies - event->created);

		if (event->func)
			event->func(device, event->priv, id, event->timestamp);

		if (event->context)
			kgsl_context_put(event->context);

		list_del(&event->list);
		kfree(event);

		kgsl_active_count_put(device);
	}
}

static inline int _mark_next_event(struct kgsl_device *device,
		struct list_head *head)
{
	struct kgsl_event *event;

	if (!list_empty(head)) {
		event = list_first_entry(head, struct kgsl_event, list);


		return device->ftbl->next_event(device, event);
	}

	return 0;
}

static int kgsl_process_context_events(struct kgsl_device *device,
		struct kgsl_context *context)
{
	while (1) {
		unsigned int timestamp = kgsl_readtimestamp(device, context,
			KGSL_TIMESTAMP_RETIRED);

		_process_event_list(device, &context->events, timestamp);


		if (!_mark_next_event(device, &context->events))
			break;
	}


	return list_empty(&context->events) ? 0 : 1;
}

void kgsl_process_events(struct work_struct *work)
{
	struct kgsl_device *device = container_of(work, struct kgsl_device,
		ts_expired_ws);
	struct kgsl_context *context, *tmp;
	uint32_t timestamp;

	mutex_lock(&device->mutex);

	
	timestamp = kgsl_readtimestamp(device, NULL, KGSL_TIMESTAMP_RETIRED);
	_process_event_list(device, &device->events, timestamp);
	_mark_next_event(device, &device->events);

	
	list_for_each_entry_safe(context, tmp, &device->events_pending_list,
		events_list) {


		if (kgsl_process_context_events(device, context) == 0)
			list_del_init(&context->events_list);
	}

	mutex_unlock(&device->mutex);
}
EXPORT_SYMBOL(kgsl_process_events);
