/*
 * Copyright (c) 2012 Code Aurora Forum. All rights reserved.
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

#ifndef _LINUX_IOPOLL_H
#define _LINUX_IOPOLL_H

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <asm-generic/errno.h>
#include <asm/io.h>

#define readl_poll_timeout(addr, val, cond, sleep_us, timeout_us) \
({ \
	unsigned long timeout = jiffies + usecs_to_jiffies(timeout_us); \
	might_sleep_if(timeout_us); \
	for (;;) { \
		(val) = readl(addr); \
		if ((cond) || (timeout_us && time_after(jiffies, timeout))) \
			break; \
		if (sleep_us) \
			usleep_range(DIV_ROUND_UP(sleep_us, 4), sleep_us); \
	} \
	(cond) ? 0 : -ETIMEDOUT; \
})

#define readl_poll(addr, val, cond, sleep_us) \
	readl_poll_timeout(addr, val, cond, sleep_us, 0)

#define readl_tight_poll_timeout(addr, val, cond, timeout_us) \
	readl_poll_timeout(addr, val, cond, 0, timeout_us)

#define readl_tight_poll(addr, val, cond) \
	readl_poll_timeout(addr, val, cond, 0, 0)

#endif 
