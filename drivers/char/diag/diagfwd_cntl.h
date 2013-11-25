/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#ifndef DIAGFWD_CNTL_H
#define DIAGFWD_CNTL_H

#define DIAG_CTRL_MSG_REG		1
#define DIAG_CTRL_MSG_DTR		2
#define DIAG_CTRL_MSG_DIAGMODE		3
#define DIAG_CTRL_MSG_DIAGDATA		4
#define DIAG_CTRL_MSG_FEATURE		8
#define DIAG_CTRL_MSG_EQUIP_LOG_MASK	9
#define DIAG_CTRL_MSG_EVENT_MASK_V2	10
#define DIAG_CTRL_MSG_F3_MASK_V2	11

struct cmd_code_range {
	uint16_t cmd_code_lo;
	uint16_t cmd_code_hi;
	uint32_t data;
};

struct diag_ctrl_msg {
	uint32_t version;
	uint16_t cmd_code;
	uint16_t subsysid;
	uint16_t count_entries;
	uint16_t port;
};

struct diag_ctrl_event_mask {
	uint32_t cmd_type;
	uint32_t data_len;
	uint8_t stream_id;
	uint8_t status;
	uint8_t event_config;
	uint32_t event_mask_size;
	
} __packed;

struct diag_ctrl_log_mask {
	uint32_t cmd_type;
	uint32_t data_len;
	uint8_t stream_id;
	uint8_t status;
	uint8_t equip_id;
	uint32_t num_items; 
	uint32_t log_mask_size; 
	
} __packed;

struct diag_ctrl_msg_mask {
	uint32_t cmd_type;
	uint32_t data_len;
	uint8_t stream_id;
	uint8_t status;
	uint8_t msg_mode;
	uint16_t ssid_first; 
	uint16_t ssid_last; 
	uint32_t msg_mask_size; 
	
} __packed;

void diagfwd_cntl_init(void);
void diagfwd_cntl_exit(void);
void diag_read_smd_cntl_work_fn(struct work_struct *);
void diag_read_smd_lpass_cntl_work_fn(struct work_struct *);
void diag_read_smd_wcnss_cntl_work_fn(struct work_struct *);
void diag_smd_cntl_notify(void *ctxt, unsigned event);
void diag_smd_lpass_cntl_notify(void *ctxt, unsigned event);
void diag_smd_wcnss_cntl_notify(void *ctxt, unsigned event);
void diag_clean_modem_reg_fn(struct work_struct *);
void diag_clean_lpass_reg_fn(struct work_struct *);
void diag_clean_wcnss_reg_fn(struct work_struct *);

#endif
