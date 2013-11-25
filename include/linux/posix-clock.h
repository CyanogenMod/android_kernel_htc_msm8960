/*
 * posix-clock.h - support for dynamic clock devices
 *
 * Copyright (C) 2010 OMICRON electronics GmbH
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _LINUX_POSIX_CLOCK_H_
#define _LINUX_POSIX_CLOCK_H_

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/posix-timers.h>
#include <linux/rwsem.h>

struct posix_clock;

struct posix_clock_operations {
	struct module *owner;

	int  (*clock_adjtime)(struct posix_clock *pc, struct timex *tx);

	int  (*clock_gettime)(struct posix_clock *pc, struct timespec *ts);

	int  (*clock_getres) (struct posix_clock *pc, struct timespec *ts);

	int  (*clock_settime)(struct posix_clock *pc,
			      const struct timespec *ts);

	int  (*timer_create) (struct posix_clock *pc, struct k_itimer *kit);

	int  (*timer_delete) (struct posix_clock *pc, struct k_itimer *kit);

	void (*timer_gettime)(struct posix_clock *pc,
			      struct k_itimer *kit, struct itimerspec *tsp);

	int  (*timer_settime)(struct posix_clock *pc,
			      struct k_itimer *kit, int flags,
			      struct itimerspec *tsp, struct itimerspec *old);
	int     (*fasync)  (struct posix_clock *pc,
			    int fd, struct file *file, int on);

	long    (*ioctl)   (struct posix_clock *pc,
			    unsigned int cmd, unsigned long arg);

	int     (*mmap)    (struct posix_clock *pc,
			    struct vm_area_struct *vma);

	int     (*open)    (struct posix_clock *pc, fmode_t f_mode);

	uint    (*poll)    (struct posix_clock *pc,
			    struct file *file, poll_table *wait);

	int     (*release) (struct posix_clock *pc);

	ssize_t (*read)    (struct posix_clock *pc,
			    uint flags, char __user *buf, size_t cnt);
};

struct posix_clock {
	struct posix_clock_operations ops;
	struct cdev cdev;
	struct kref kref;
	struct rw_semaphore rwsem;
	bool zombie;
	void (*release)(struct posix_clock *clk);
};

int posix_clock_register(struct posix_clock *clk, dev_t devid);

void posix_clock_unregister(struct posix_clock *clk);

#endif
