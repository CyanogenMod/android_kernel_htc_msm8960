/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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
 *
 * Subsystem restart notifier API header
 *
 */

#ifndef _SUBSYS_NOTIFIER_H
#define _SUBSYS_NOTIFIER_H

#include <linux/notifier.h>

enum subsys_notif_type {
	SUBSYS_BEFORE_SHUTDOWN,
	SUBSYS_AFTER_SHUTDOWN,
	SUBSYS_BEFORE_POWERUP,
	SUBSYS_AFTER_POWERUP,
	SUBSYS_NOTIF_TYPE_COUNT
};

#if defined(CONFIG_MSM_SUBSYSTEM_RESTART)
void *subsys_notif_register_notifier(
			const char *subsys_name, struct notifier_block *nb);
int subsys_notif_unregister_notifier(void *subsys_handle,
				struct notifier_block *nb);

void *subsys_notif_add_subsys(const char *);
int subsys_notif_queue_notification(void *subsys_handle,
					enum subsys_notif_type notif_type);
#else

static inline void *subsys_notif_register_notifier(
			const char *subsys_name, struct notifier_block *nb)
{
	return NULL;
}

static inline int subsys_notif_unregister_notifier(void *subsys_handle,
					struct notifier_block *nb)
{
	return 0;
}

static inline void *subsys_notif_add_subsys(const char *subsys_name)
{
	return NULL;
}

static inline int subsys_notif_queue_notification(void *subsys_handle,
					enum subsys_notif_type notif_type)
{
	return 0;
}
#endif 

#endif
