/* Copyright (c) 2008-2012, Code Aurora Forum. All rights reserved.
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

#ifndef DIAGCHAR_H
#define DIAGCHAR_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/mempool.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <mach/msm_smd.h>
#include <asm/atomic.h>
#include <mach/usbdiag.h>
#include <asm/mach-types.h>
#include <linux/delay.h>
#define USB_MAX_OUT_BUF 4096
#define APPS_BUF_SIZE	2000
#define IN_BUF_SIZE		16384
#define MAX_IN_BUF_SIZE	32768
#define MAX_SYNC_OBJ_NAME_SIZE	32
#define UINT32_MAX	UINT_MAX
#define HDLC_MAX 4096
#define HDLC_OUT_BUF_SIZE	8192
#define POOL_TYPE_COPY		1
#define POOL_TYPE_HDLC		2
#define POOL_TYPE_WRITE_STRUCT	4
#define POOL_TYPE_HSIC		8
#define POOL_TYPE_HSIC_WRITE	16
#define POOL_TYPE_ALL		7
#define MODEM_DATA		1
#define LPASS_DATA		2
#define APPS_DATA		3
#define SDIO_DATA		4
#define WCNSS_DATA		5
#define HSIC_DATA		6
#define SMUX_DATA		7
#define MODEM_PROC		0
#define APPS_PROC		1
#define LPASS_PROC		2
#define WCNSS_PROC		3
#define MSG_MASK_SIZE 10000
#define LOG_MASK_SIZE 8000
#define EVENT_MASK_SIZE 1000
#define USER_SPACE_DATA 8000
#define PKT_SIZE 4096
#define MAX_EQUIP_ID 15
#define DIAG_CTRL_MSG_LOG_MASK	9
#define DIAG_CTRL_MSG_EVENT_MASK	10
#define DIAG_CTRL_MSG_F3_MASK	11
#define CONTROL_CHAR	0x7E

#define DIAG_CON_APSS (0x0001)	
#define DIAG_CON_MPSS (0x0002)	
#define DIAG_CON_LPASS (0x0004)	
#define DIAG_CON_WCNSS (0x0008)	

#define DIAG_STATUS_OPEN (0x00010000)	
#define DIAG_STATUS_CLOSED (0x00020000)	

extern unsigned int diag_max_reg;
extern unsigned int diag_threshold_reg;

#define APPEND_DEBUG(ch) \
do {							\
	diag_debug_buf[diag_debug_buf_idx] = ch; \
	(diag_debug_buf_idx < 1023) ? \
	(diag_debug_buf_idx++) : (diag_debug_buf_idx = 0); \
} while (0)

struct diag_master_table {
	uint16_t cmd_code;
	uint16_t subsys_id;
	uint32_t client_id;
	uint16_t cmd_code_lo;
	uint16_t cmd_code_hi;
	int process_id;
};

struct bindpkt_params_per_process {
	
	char sync_obj_name[MAX_SYNC_OBJ_NAME_SIZE];
	uint32_t count;	
	struct bindpkt_params *params; 
};

struct bindpkt_params {
	uint16_t cmd_code;
	uint16_t subsys_id;
	uint16_t cmd_code_lo;
	uint16_t cmd_code_hi;
	
	uint16_t proc_id;
	uint32_t event_id;
	uint32_t log_code;
	
	uint32_t client_id;
};

struct diag_write_device {
	void *buf;
	int length;
};

struct diag_client_map {
	char name[20];
	int pid;
	int timeout;
};

#ifndef CONFIG_DIAG_OVER_USB
struct diag_request {
	char *buf;
	int length;
	int actual;
	int status;
	void *context;
};
#endif

struct diagchar_dev {

	
	unsigned int major;
	unsigned int minor_start;
	int num;
	struct cdev *cdev;
	char *name;
	int dropped_count;
	struct class *diagchar_class;
	int ref_count;
	struct mutex diagchar_mutex;
	wait_queue_head_t wait_q;
	wait_queue_head_t smd_wait_q;
	struct diag_client_map *client_map;
	int *data_ready;
	int num_clients;
	int polling_reg_flag;
	struct diag_write_device *buf_tbl;
	int use_device_tree;
	
	struct dci_pkt_req_tracking_tbl *req_tracking_tbl;
	struct diag_dci_client_tbl *dci_client_tbl;
	int dci_tag;
	int dci_client_id;
	struct mutex dci_mutex;
	int num_dci_client;
	unsigned char *apps_dci_buf;
	int dci_state;
	struct workqueue_struct *diag_dci_wq;
#if defined(CONFIG_DIAG_SDIO_PIPE) || defined(CONFIG_DIAGFWD_BRIDGE_CODE)
	struct cdev *cdev_mdm;
	int num_mdmclients;
	struct mutex diagcharmdm_mutex;
	wait_queue_head_t mdmwait_q;
	struct diag_client_map *mdmclient_map;
	int *mdmdata_ready;
	int mdm_logging_process_id;
	unsigned char *user_space_mdm_data;

	struct cdev *cdev_qsc;
	int num_qscclients;
	struct mutex diagcharqsc_mutex;
	wait_queue_head_t qscwait_q;
	struct diag_client_map *qscclient_map;
	int *qscdata_ready;
	int qsc_logging_process_id;
	unsigned char *user_space_qsc_data;
#endif

	
	unsigned int itemsize;
	unsigned int poolsize;
	unsigned int itemsize_hdlc;
	unsigned int poolsize_hdlc;
	unsigned int itemsize_write_struct;
	unsigned int poolsize_write_struct;
	unsigned int debug_flag;
	
	mempool_t *diagpool;
	mempool_t *diag_hdlc_pool;
	mempool_t *diag_write_struct_pool;
	struct mutex diagmem_mutex;
	int count;
	int count_hdlc_pool;
	int count_write_struct_pool;
	int used;
	
	struct mutex diag_cntl_mutex;
	struct diag_ctrl_event_mask *event_mask;
	struct diag_ctrl_log_mask *log_mask;
	struct diag_ctrl_msg_mask *msg_mask;
	
	unsigned char *buf_in_1;
	unsigned char *buf_in_2;
	unsigned char *buf_in_cntl;
	unsigned char *buf_in_lpass_1;
	unsigned char *buf_in_lpass_2;
	unsigned char *buf_in_lpass_cntl;
	unsigned char *buf_in_wcnss_1;
	unsigned char *buf_in_wcnss_2;
	unsigned char *buf_in_wcnss_cntl;
	unsigned char *buf_in_dci;
	unsigned char *usb_buf_out;
	unsigned char *apps_rsp_buf;
	unsigned char *user_space_data;
	
	unsigned char *buf_msg_mask_update;
	unsigned char *buf_log_mask_update;
	unsigned char *buf_event_mask_update;
	smd_channel_t *ch;
	smd_channel_t *ch_cntl;
	smd_channel_t *ch_dci;
	smd_channel_t *chlpass;
	smd_channel_t *chlpass_cntl;
	smd_channel_t *ch_wcnss;
	smd_channel_t *ch_wcnss_cntl;
	int in_busy_1;
	int in_busy_2;
	int in_busy_lpass_1;
	int in_busy_lpass_2;
	int in_busy_wcnss_1;
	int in_busy_wcnss_2;
	int in_busy_dci;
	int read_len_legacy;
	unsigned char *hdlc_buf;
	unsigned hdlc_count;
	unsigned hdlc_escape;
#ifdef CONFIG_DIAG_OVER_USB
	int usb_connected;
	struct usb_diag_ch *legacy_ch;
	struct work_struct diag_proc_hdlc_work;
	struct work_struct diag_read_work;
#endif
	struct workqueue_struct *diag_wq;
	struct wake_lock wake_lock;
	struct work_struct diag_drain_work;
	struct work_struct diag_read_smd_work;
	struct work_struct diag_read_smd_cntl_work;
	struct work_struct diag_read_smd_lpass_work;
	struct work_struct diag_read_smd_lpass_cntl_work;
	struct work_struct diag_read_smd_wcnss_work;
	struct work_struct diag_read_smd_wcnss_cntl_work;
	struct workqueue_struct *diag_cntl_wq;
	struct work_struct diag_modem_mask_update_work;
	struct work_struct diag_lpass_mask_update_work;
	struct work_struct diag_wcnss_mask_update_work;
	struct work_struct diag_read_smd_dci_work;
	struct work_struct diag_update_smd_dci_work;
	struct work_struct diag_clean_modem_reg_work;
	struct work_struct diag_clean_lpass_reg_work;
	struct work_struct diag_clean_wcnss_reg_work;
	uint8_t *msg_masks;
	uint8_t *log_masks;
	int log_masks_length;
	uint8_t *event_masks;
	struct diag_master_table *table;
	uint8_t *pkt_buf;
	int pkt_length;
	struct diag_request *write_ptr_1;
	struct diag_request *write_ptr_2;
	struct diag_request *usb_read_ptr;
	struct diag_request *write_ptr_svc;
	struct diag_request *write_ptr_lpass_1;
	struct diag_request *write_ptr_lpass_2;
	struct diag_request *write_ptr_wcnss_1;
	struct diag_request *write_ptr_wcnss_2;
	struct diag_write_device *write_ptr_dci;
	int logging_mode;
	int mask_check;
	int logging_process_id;
	struct task_struct *socket_process;
	struct task_struct *callback_process;
#if DIAG_XPST
	unsigned char nohdlc;
	unsigned char in_busy_dmrounter;
	struct mutex smd_lock;
	unsigned char init_done;
	unsigned char is2ARM11;
	int debug_dmbytes_recv;
#endif
#ifdef CONFIG_DIAG_SDIO_PIPE
	unsigned char *buf_in_sdio;
	unsigned char *usb_buf_mdm_out;
	struct sdio_channel *sdio_ch;
	int read_len_mdm;
	int in_busy_sdio;
	struct usb_diag_ch *mdm_ch;
	struct work_struct diag_read_mdm_work;
	struct workqueue_struct *diag_sdio_wq;
	struct work_struct diag_read_sdio_work;
	struct work_struct diag_close_sdio_work;
	struct diag_request *usb_read_mdm_ptr;
	struct diag_request *write_ptr_mdm;
#endif
#ifdef CONFIG_DIAGFWD_BRIDGE_CODE
	
	struct work_struct diag_disconnect_work;
	
	int lcid;
	unsigned char *buf_in_smux;
	int in_busy_smux;
	int diag_smux_enabled;
	int smux_connected;
	struct diag_request *write_ptr_mdm;
	
	int hsic_ch;
	int hsic_inited;
	int hsic_device_enabled;
	int hsic_device_opened;
	int hsic_suspend;
	int in_busy_hsic_read_on_device;
	int in_busy_hsic_write;
	struct work_struct diag_read_hsic_work;
	int count_hsic_pool;
	int count_hsic_write_pool;
	unsigned int poolsize_hsic;
	unsigned int poolsize_hsic_write;
	unsigned int itemsize_hsic;
	unsigned int itemsize_hsic_write;
	mempool_t *diag_hsic_pool;
	mempool_t *diag_hsic_write_pool;
	int num_hsic_buf_tbl_entries;
	struct diag_write_device *hsic_buf_tbl;
	spinlock_t hsic_spinlock;
#endif
	int in_busy_hsic_write_wait;
	int qxdm2sd_drop;
	int qxdmusb_drop;
	struct timeval st0;
	struct timeval st1;
};

extern struct diag_bridge_dev *diag_bridge;
extern struct diagchar_dev *driver;

#define EPST_FUN 1
#define HPST_FUN 0

#define DIAG_DBG_READ	1
#define DIAG_DBG_WRITE	2
#define DIAG_DBG_DROP	3

extern unsigned diag7k_debug_mask;
extern unsigned diag9k_debug_mask;
#define DIAGFWD_7K_RAWDATA(buf, src, flag) \
	__diagfwd_dbg_raw_data(buf, src, flag, diag7k_debug_mask)
#define DIAGFWD_9K_RAWDATA(buf, src, flag) \
	__diagfwd_dbg_raw_data(buf, src, flag, diag9k_debug_mask)
void __diagfwd_dbg_raw_data(void *buf, const char* src, unsigned dbg_flag, unsigned mask);
#if defined(CONFIG_ARCH_MSM8X60) || defined(CONFIG_ARCH_MSM8960)
#define	SMDDIAG_NAME "DIAG"
#else
#define	SMDDIAG_NAME "SMD_DIAG"
#endif
extern struct diagchar_dev *driver;
#endif
