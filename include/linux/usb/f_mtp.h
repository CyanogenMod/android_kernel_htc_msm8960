/*
 * Gadget Function Driver for MTP
 *
 * Copyright (C) 2010 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
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

#ifndef __LINUX_USB_F_MTP_H
#define __LINUX_USB_F_MTP_H

#include <linux/ioctl.h>

#ifdef __KERNEL__

struct mtp_data_header {
	
	uint32_t	length;
	
	uint16_t	type;
	
	uint16_t    command;
	
	uint32_t	transaction_id;
};

#endif 

struct mtp_file_range {
	
	int			fd;
	
	loff_t		offset;
	
	int64_t		length;
	uint16_t	command;
	uint32_t	transaction_id;
};

struct mtp_event {
	
	size_t		length;
	
	void		*data;
};

#define MTP_SEND_FILE              _IOW('M', 0, struct mtp_file_range)
#define MTP_RECEIVE_FILE           _IOW('M', 1, struct mtp_file_range)
#define MTP_SEND_EVENT             _IOW('M', 3, struct mtp_event)
#define MTP_SEND_FILE_WITH_HEADER  _IOW('M', 4, struct mtp_file_range)

#define MTP_SET_CPU_PERF   _IOW('M', 5, int)
#define MTP_THREAD_SUPPORTED	_IOW('M', 64, int)

#endif 
