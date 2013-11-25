/*
 * PPS API kernel header
 *
 * Copyright (C) 2009   Rodolfo Giometti <giometti@linux.it>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef LINUX_PPS_KERNEL_H
#define LINUX_PPS_KERNEL_H

#include <linux/pps.h>

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/time.h>


struct pps_device;

struct pps_source_info {
	char name[PPS_MAX_NAME_LEN];		
	char path[PPS_MAX_NAME_LEN];		
	int mode;				

	void (*echo)(struct pps_device *pps,
			int event, void *data);	

	struct module *owner;
	struct device *dev;
};

struct pps_event_time {
#ifdef CONFIG_NTP_PPS
	struct timespec ts_raw;
#endif 
	struct timespec ts_real;
};

struct pps_device {
	struct pps_source_info info;		

	struct pps_kparams params;		

	__u32 assert_sequence;			
	__u32 clear_sequence;			
	struct pps_ktime assert_tu;
	struct pps_ktime clear_tu;
	int current_mode;			

	unsigned int last_ev;			
	wait_queue_head_t queue;		

	unsigned int id;			
	struct cdev cdev;
	struct device *dev;
	struct fasync_struct *async_queue;	
	spinlock_t lock;
};


extern struct device_attribute pps_attrs[];


extern struct pps_device *pps_register_source(
		struct pps_source_info *info, int default_params);
extern void pps_unregister_source(struct pps_device *pps);
extern int pps_register_cdev(struct pps_device *pps);
extern void pps_unregister_cdev(struct pps_device *pps);
extern void pps_event(struct pps_device *pps,
		struct pps_event_time *ts, int event, void *data);

static inline void timespec_to_pps_ktime(struct pps_ktime *kt,
		struct timespec ts)
{
	kt->sec = ts.tv_sec;
	kt->nsec = ts.tv_nsec;
}

#ifdef CONFIG_NTP_PPS

static inline void pps_get_ts(struct pps_event_time *ts)
{
	getnstime_raw_and_real(&ts->ts_raw, &ts->ts_real);
}

#else 

static inline void pps_get_ts(struct pps_event_time *ts)
{
	getnstimeofday(&ts->ts_real);
}

#endif 

#endif 

