/* Qualcomm Secure Execution Environment Communicator (QSEECOM) driver
 *
 * Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __QSEECOM_LEGACY_H_
#define __QSEECOM_LEGACY_H_

#include <linux/types.h>

#define TZ_SCHED_CMD_ID_REGISTER_LISTENER    0x04

enum tz_sched_cmd_type {
	TZ_SCHED_CMD_INVALID = 0,
	TZ_SCHED_CMD_NEW,      
	TZ_SCHED_CMD_PENDING,  
	TZ_SCHED_CMD_COMPLETE, 
	TZ_SCHED_CMD_MAX     = 0x7FFFFFFF
};

enum tz_sched_cmd_status {
	TZ_SCHED_STATUS_INCOMPLETE = 0,
	TZ_SCHED_STATUS_COMPLETE,
	TZ_SCHED_STATUS_MAX  = 0x7FFFFFFF
};
__packed struct qse_pr_init_sb_req_s {
	
	uint32_t                  pr_cmd;
	
	uint32_t                  sb_ptr;
	
	uint32_t                  sb_len;
	uint32_t                  listener_id;
};

__packed struct qse_pr_init_sb_rsp_s {
	
	uint32_t                  pr_cmd;
	
	int32_t                   ret;
};

__packed struct qseecom_command {
	uint32_t               cmd_type;
	uint8_t                *sb_in_cmd_addr;
	uint32_t               sb_in_cmd_len;
};

__packed struct qseecom_response {
	uint32_t                 cmd_status;
	uint8_t                  *sb_in_rsp_addr;
	uint32_t                 sb_in_rsp_len;
};

#endif 
