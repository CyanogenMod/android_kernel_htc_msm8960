/*
 * Copyright (c) 2001-2005 Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write the Free Software Foundation,
 * Inc.,  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef __XFS_SYSCTL_H__
#define __XFS_SYSCTL_H__

#include <linux/sysctl.h>


typedef struct xfs_sysctl_val {
	int min;
	int val;
	int max;
} xfs_sysctl_val_t;

typedef struct xfs_param {
	xfs_sysctl_val_t sgid_inherit;	
	xfs_sysctl_val_t symlink_mode;	
	xfs_sysctl_val_t panic_mask;	
	xfs_sysctl_val_t error_level;	
	xfs_sysctl_val_t syncd_timer;	
	xfs_sysctl_val_t stats_clear;	
	xfs_sysctl_val_t inherit_sync;	
	xfs_sysctl_val_t inherit_nodump;
	xfs_sysctl_val_t inherit_noatim;
	xfs_sysctl_val_t xfs_buf_timer;	
	xfs_sysctl_val_t xfs_buf_age;	
	xfs_sysctl_val_t inherit_nosym;	
	xfs_sysctl_val_t rotorstep;	
	xfs_sysctl_val_t inherit_nodfrg;
	xfs_sysctl_val_t fstrm_timer;	
} xfs_param_t;


enum {
	
	
	
	XFS_SGID_INHERIT = 4,
	XFS_SYMLINK_MODE = 5,
	XFS_PANIC_MASK = 6,
	XFS_ERRLEVEL = 7,
	XFS_SYNCD_TIMER = 8,
	
	
	
	XFS_STATS_CLEAR = 12,
	XFS_INHERIT_SYNC = 13,
	XFS_INHERIT_NODUMP = 14,
	XFS_INHERIT_NOATIME = 15,
	XFS_BUF_TIMER = 16,
	XFS_BUF_AGE = 17,
	
	XFS_INHERIT_NOSYM = 19,
	XFS_ROTORSTEP = 20,
	XFS_INHERIT_NODFRG = 21,
	XFS_FILESTREAM_TIMER = 22,
};

extern xfs_param_t	xfs_params;

#ifdef CONFIG_SYSCTL
extern int xfs_sysctl_register(void);
extern void xfs_sysctl_unregister(void);
#else
# define xfs_sysctl_register()		(0)
# define xfs_sysctl_unregister()	do { } while (0)
#endif 

#endif 
