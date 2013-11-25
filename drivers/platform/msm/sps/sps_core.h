/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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


#ifndef _SPS_CORE_H_
#define _SPS_CORE_H_

#include <linux/types.h>	
#include <linux/mutex.h>	
#include <linux/list.h>		

#include "spsi.h"
#include "sps_bam.h"

#define SPS_STATE_DEF(x)   ('S' | ('P' << 8) | ('S' << 16) | ((x) << 24))
#define IS_SPS_STATE_OK(x) \
	(((x)->client_state & 0x00ffffff) == SPS_STATE_DEF(0))

#define SPS_CONFIG_SATELLITE  0x11111111

#define SPS_STATE_DISCONNECT  0
#define SPS_STATE_ALLOCATE    SPS_STATE_DEF(1)
#define SPS_STATE_CONNECT     SPS_STATE_DEF(2)
#define SPS_STATE_ENABLE      SPS_STATE_DEF(3)
#define SPS_STATE_DISABLE     SPS_STATE_DEF(4)

struct sps_rm {
	struct list_head connections_q;
	struct mutex lock;
};

struct sps_bam *sps_h2bam(u32 h);

int sps_rm_init(struct sps_rm *rm, u32 options);

void sps_rm_de_init(void);

void sps_rm_config_init(struct sps_connect *connect);

int sps_rm_state_change(struct sps_pipe *pipe, u32 state);

#endif				
