/* cgroupstats.h - exporting per-cgroup statistics
 *
 * Copyright IBM Corporation, 2007
 * Author Balbir Singh <balbir@linux.vnet.ibm.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _LINUX_CGROUPSTATS_H
#define _LINUX_CGROUPSTATS_H

#include <linux/types.h>
#include <linux/taskstats.h>

struct cgroupstats {
	__u64	nr_sleeping;		
	__u64	nr_running;		
	__u64	nr_stopped;		
	__u64	nr_uninterruptible;	
					
	__u64	nr_io_wait;		
};


enum {
	CGROUPSTATS_CMD_UNSPEC = __TASKSTATS_CMD_MAX,	
	CGROUPSTATS_CMD_GET,		
	CGROUPSTATS_CMD_NEW,		
	__CGROUPSTATS_CMD_MAX,
};

#define CGROUPSTATS_CMD_MAX (__CGROUPSTATS_CMD_MAX - 1)

enum {
	CGROUPSTATS_TYPE_UNSPEC = 0,	
	CGROUPSTATS_TYPE_CGROUP_STATS,	
	__CGROUPSTATS_TYPE_MAX,
};

#define CGROUPSTATS_TYPE_MAX (__CGROUPSTATS_TYPE_MAX - 1)

enum {
	CGROUPSTATS_CMD_ATTR_UNSPEC = 0,
	CGROUPSTATS_CMD_ATTR_FD,
	__CGROUPSTATS_CMD_ATTR_MAX,
};

#define CGROUPSTATS_CMD_ATTR_MAX (__CGROUPSTATS_CMD_ATTR_MAX - 1)

#endif 
