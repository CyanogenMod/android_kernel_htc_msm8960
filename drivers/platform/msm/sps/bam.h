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


#ifndef _BAM_H_
#define _BAM_H_

#include <linux/types.h>	
#include <linux/io.h>		
#include <linux/bitops.h>	
#include "spsi.h"

enum bam_pipe_mode {
	BAM_PIPE_MODE_BAM2BAM = 0,	
	BAM_PIPE_MODE_SYSTEM = 1,	
};

enum bam_pipe_dir {
	
	BAM_PIPE_CONSUMER = 0,
	
	BAM_PIPE_PRODUCER = 1,
};

enum bam_stream_mode {
	BAM_STREAM_MODE_DISABLE = 0,
	BAM_STREAM_MODE_ENABLE = 1,
};

/* NWD written Type */
enum bam_write_nwd {
	BAM_WRITE_NWD_DISABLE = 0,
	BAM_WRITE_NWD_ENABLE = 1,
};


enum bam_enable {
	BAM_DISABLE = 0,
	BAM_ENABLE = 1,
};

enum bam_pipe_timer_mode {
	BAM_PIPE_TIMER_ONESHOT = 0,
	BAM_PIPE_TIMER_PERIODIC = 1,
};

struct transfer_descriptor {
	u32 addr;	
	u32 size:16;	
	u32 flags:16;	
}  __packed;

struct bam_pipe_parameters {
	u16 event_threshold;
	u32 pipe_irq_mask;
	enum bam_pipe_dir dir;
	enum bam_pipe_mode mode;
	enum bam_write_nwd write_nwd;
	u32 desc_base;	
	u32 desc_size;	
	u32 lock_group;	
	enum bam_stream_mode stream_mode;
	u32 ee;		

	
	u32 peer_phys_addr;
	u32 peer_pipe;
	u32 data_base;	
	u32 data_size;	
};

int bam_init(void *base,
		u32 ee,
		u16 summing_threshold,
		u32 irq_mask, u32 *version, u32 *num_pipes);

int bam_security_init(void *base, u32 ee, u32 vmid, u32 pipe_mask);

int bam_check(void *base, u32 *version, u32 *num_pipes);

void bam_exit(void *base, u32 ee);

u32 bam_check_irq_source(void *base, u32 ee, u32 mask,
				enum sps_callback_case *cb_case);

int bam_pipe_init(void *base, u32 pipe, struct bam_pipe_parameters *param,
					u32 ee);

void bam_pipe_exit(void *base, u32 pipe, u32 ee);

void bam_pipe_enable(void *base, u32 pipe);

void bam_pipe_disable(void *base, u32 pipe);

int bam_pipe_is_enabled(void *base, u32 pipe);

void bam_pipe_set_irq(void *base, u32 pipe, enum bam_enable irq_en,
		      u32 src_mask, u32 ee);

/**
 * Configure a BAM pipe for satellite MTI use
 *
 * This function configures a BAM pipe for satellite MTI use.
 *
 * @base - BAM virtual base address.
 *
 * @pipe - pipe index
 *
 * @irq_gen_addr - physical address written to generate MTI
 *
 * @ee - BAM execution environment index
 *
 */
void bam_pipe_satellite_mti(void *base, u32 pipe, u32 irq_gen_addr, u32 ee);

/**
 * Configure MTI for a BAM pipe
 *
 * This function configures the interrupt for a BAM pipe.
 *
 * @base - BAM virtual base address.
 *
 * @pipe - pipe index
 *
 * @irq_en - enable or disable interrupt
 *
 * @src_mask - interrupt source mask, set regardless of whether
 *    interrupt is disabled
 *
 * @irq_gen_addr - physical address written to generate MTI
 *
 */
void bam_pipe_set_mti(void *base, u32 pipe, enum bam_enable irq_en,
		      u32 src_mask, u32 irq_gen_addr);

u32 bam_pipe_get_and_clear_irq_status(void *base, u32 pipe);

void bam_pipe_set_desc_write_offset(void *base, u32 pipe, u32 next_write);

u32 bam_pipe_get_desc_write_offset(void *base, u32 pipe);

u32 bam_pipe_get_desc_read_offset(void *base, u32 pipe);

void bam_pipe_timer_config(void *base, u32 pipe,
			   enum bam_pipe_timer_mode mode,
			   u32 timeout_count);

void bam_pipe_timer_reset(void *base, u32 pipe);

u32 bam_pipe_timer_get_count(void *base, u32 pipe);

#endif				
