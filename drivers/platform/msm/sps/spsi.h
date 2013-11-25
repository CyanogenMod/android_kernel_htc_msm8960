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


#ifndef _SPSI_H_
#define _SPSI_H_

#include <linux/types.h>	
#include <linux/list.h>		
#include <linux/kernel.h>	
#include <linux/compiler.h>
#include <linux/ratelimit.h>

#include <mach/sps.h>

#include "sps_map.h"

#define BAM_MAX_PIPES              31
#define BAM_MAX_P_LOCK_GROUP_NUM   31

#define SPS_EVENT_INDEX(e)    ((e) - 1)
#define SPS_ERROR -1

#define BAM_ID(dev)       ((dev)->props.phys_addr)

#define SPSRM_CLEAR     0xcccccccc

extern u32 d_type;

#ifdef CONFIG_DEBUG_FS
extern u8 debugfs_record_enabled;
extern u8 logging_option;
extern u8 debug_level_option;
extern u8 print_limit_option;

#define MAX_MSG_LEN 80
#define SPS_DEBUGFS(msg, args...) do {					\
		char buf[MAX_MSG_LEN];		\
		snprintf(buf, MAX_MSG_LEN, msg"\n", ##args);	\
		sps_debugfs_record(buf);	\
	} while (0)
#define SPS_ERR(msg, args...) do {					\
		if (unlikely(print_limit_option > 2))	\
			pr_err_ratelimited(msg, ##args);	\
		else	\
			pr_err(msg, ##args);	\
		if (unlikely(debugfs_record_enabled))	\
			SPS_DEBUGFS(msg, ##args);	\
	} while (0)
#define SPS_INFO(msg, args...) do {					\
		if (unlikely(print_limit_option > 1))	\
			pr_info_ratelimited(msg, ##args);	\
		else	\
			pr_info(msg, ##args);	\
		if (unlikely(debugfs_record_enabled))	\
			SPS_DEBUGFS(msg, ##args);	\
	} while (0)
#define SPS_DBG(msg, args...) do {					\
		if ((unlikely(logging_option > 1))	\
			&& (unlikely(debug_level_option > 3))) {\
			if (unlikely(print_limit_option > 0))	\
				pr_info_ratelimited(msg, ##args);	\
			else	\
				pr_info(msg, ##args);	\
		} else	\
			pr_debug(msg, ##args);	\
		if (unlikely(debugfs_record_enabled))	\
			SPS_DEBUGFS(msg, ##args);	\
	} while (0)
#define SPS_DBG1(msg, args...) do {					\
		if ((unlikely(logging_option > 1))	\
			&& (unlikely(debug_level_option > 2))) {\
			if (unlikely(print_limit_option > 0))	\
				pr_info_ratelimited(msg, ##args);	\
			else	\
				pr_info(msg, ##args);	\
		} else	\
			pr_debug(msg, ##args);	\
		if (unlikely(debugfs_record_enabled))	\
			SPS_DEBUGFS(msg, ##args);	\
	} while (0)
#define SPS_DBG2(msg, args...) do {					\
		if ((unlikely(logging_option > 1))	\
			&& (unlikely(debug_level_option > 1))) {\
			if (unlikely(print_limit_option > 0))	\
				pr_info_ratelimited(msg, ##args);	\
			else	\
				pr_info(msg, ##args);	\
		} else	\
			pr_debug(msg, ##args);	\
		if (unlikely(debugfs_record_enabled))	\
			SPS_DEBUGFS(msg, ##args);	\
	} while (0)
#define SPS_DBG3(msg, args...) do {					\
		if ((unlikely(logging_option > 1))	\
			&& (unlikely(debug_level_option > 0))) {\
			if (unlikely(print_limit_option > 0))	\
				pr_info_ratelimited(msg, ##args);	\
			else	\
				pr_info(msg, ##args);	\
		} else	\
			pr_debug(msg, ##args);	\
		if (unlikely(debugfs_record_enabled))	\
			SPS_DEBUGFS(msg, ##args);	\
	} while (0)
#else
#define	SPS_DBG3(x...)		pr_debug(x)
#define	SPS_DBG2(x...)		pr_debug(x)
#define	SPS_DBG1(x...)		pr_debug(x)
#define	SPS_DBG(x...)		pr_debug(x)
#define	SPS_INFO(x...)		pr_info(x)
#define	SPS_ERR(x...)		pr_err(x)
#endif

struct sps_conn_end_pt {
	u32 dev;		
	u32 bam_phys;		
	u32 pipe_index;		
	u32 event_threshold;	
	u32 lock_group;	
	void *bam;
};

struct sps_connection {
	struct list_head list;

	
	struct sps_conn_end_pt src;

	
	struct sps_conn_end_pt dest;

	
	struct sps_mem_buffer desc;	
	struct sps_mem_buffer data;	
	u32 config;		

	
	void *client_src;
	void *client_dest;
	int refs;		

	
	u32 alloc_src_pipe;	
	u32 alloc_dest_pipe;	
	u32 alloc_desc_base;	
	u32 alloc_data_base;	
};

struct sps_q_event {
	struct list_head list;
	
	struct sps_event_notify notify;
};

struct sps_mem_stats {
	u32 base_addr;
	u32 size;
	u32 blocks_used;
	u32 bytes_used;
	u32 max_bytes_used;
};

#ifdef CONFIG_DEBUG_FS
void sps_debugfs_record(const char *);
#endif

void print_bam_reg(void *);

void print_bam_pipe_reg(void *, u32);

void print_bam_selected_reg(void *);

void print_bam_pipe_selected_reg(void *, u32);

void print_bam_pipe_desc_fifo(void *, u32);

void print_bam_pipe_desc_fifo(void *, u32);

void *spsi_get_mem_ptr(u32 phys_addr);

u32 sps_mem_alloc_io(u32 bytes);

void sps_mem_free_io(u32 phys_addr, u32 bytes);

int sps_map_find(struct sps_connect *connect);

int sps_dma_pipe_alloc(void *bam, u32 pipe_index, enum sps_mode dir);

int sps_dma_pipe_enable(void *bam, u32 pipe_index);

int sps_dma_pipe_free(void *bam, u32 pipe_index);

int sps_mem_init(u32 pipemem_phys_base, u32 pipemem_size);

int sps_mem_de_init(void);

int sps_dma_init(const struct sps_bam_props *bam_props);

void sps_dma_de_init(void);

int sps_dma_device_init(u32 h);

int sps_dma_device_de_init(u32 h);


int sps_map_init(const struct sps_map *map_props, u32 options);

void sps_map_de_init(void);

#endif	
