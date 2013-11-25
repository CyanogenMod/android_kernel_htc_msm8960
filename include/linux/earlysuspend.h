/* include/linux/earlysuspend.h
 *
 * Copyright (C) 2007-2008 Google, Inc.
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

#ifndef _LINUX_EARLYSUSPEND_H
#define _LINUX_EARLYSUSPEND_H

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/list.h>
#endif

enum {
	EARLY_SUSPEND_LEVEL_BLANK_SCREEN = 50,
	EARLY_SUSPEND_LEVEL_STOP_DRAWING = 100,
	EARLY_SUSPEND_LEVEL_DISABLE_FB = 150,
};
struct early_suspend {
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct list_head link;
	int level;
	void (*suspend)(struct early_suspend *h);
	void (*resume)(struct early_suspend *h);
#endif
};

#ifdef CONFIG_HAS_EARLYSUSPEND
void register_early_suspend(struct early_suspend *handler);
void unregister_early_suspend(struct early_suspend *handler);
#ifdef CONFIG_HTC_ONMODE_CHARGING
void register_onchg_suspend(struct early_suspend *handler);
void unregister_onchg_suspend(struct early_suspend *handler);
#endif
#else
#define register_early_suspend(handler) do { } while (0)
#define unregister_early_suspend(handler) do { } while (0)
#endif

#endif

