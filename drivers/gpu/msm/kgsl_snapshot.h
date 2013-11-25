/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _KGSL_SNAPSHOT_H_
#define _KGSL_SNAPSHOT_H_

#include <linux/types.h>


#define SNAPSHOT_MAGIC 0x504D0002


struct kgsl_snapshot_header {
	__u32 magic; 
	__u32 gpuid; 
	
	__u32 chipid; 
} __packed;

#define SNAPSHOT_SECTION_MAGIC 0xABCD

struct kgsl_snapshot_section_header {
	__u16 magic; 
	__u16 id;    
	__u32 size;  
} __packed;

#define KGSL_SNAPSHOT_SECTION_OS           0x0101
#define KGSL_SNAPSHOT_SECTION_REGS         0x0201
#define KGSL_SNAPSHOT_SECTION_RB           0x0301
#define KGSL_SNAPSHOT_SECTION_IB           0x0401
#define KGSL_SNAPSHOT_SECTION_INDEXED_REGS 0x0501
#define KGSL_SNAPSHOT_SECTION_ISTORE       0x0801
#define KGSL_SNAPSHOT_SECTION_DEBUG        0x0901
#define KGSL_SNAPSHOT_SECTION_DEBUGBUS     0x0A01
#define KGSL_SNAPSHOT_SECTION_GPU_OBJECT   0x0B01

#define KGSL_SNAPSHOT_SECTION_END          0xFFFF

#define KGSL_SNAPSHOT_OS_LINUX             0x0001


#define SNAPSHOT_STATE_HUNG 0
#define SNAPSHOT_STATE_RUNNING 1

struct kgsl_snapshot_linux {
	int osid;                   
	int state;		    
	__u32 seconds;		    
	__u32 power_flags;            
	__u32 power_level;            
	__u32 power_interval_timeout; 
	__u32 grpclk;                 
	__u32 busclk;		    
	__u32 ptbase;		    
	__u32 pid;		    
	__u32 current_context;	    
	__u32 ctxtcount;	    
	unsigned char release[32];  
	unsigned char version[32];  
	unsigned char comm[16];	    
} __packed;


struct kgsl_snapshot_linux_context {
	__u32 id;			
	__u32 timestamp_queued;		
	__u32 timestamp_retired;	
};

struct kgsl_snapshot_rb {
	int start;  
	int end;    
	int rbsize; 
	int wptr;   
	int rptr;   
	int count;  
} __packed;

struct kgsl_snapshot_ib {
	__u32 gpuaddr; 
	__u32 ptbase;  
	int size;    
} __packed;

struct kgsl_snapshot_regs {
	__u32 count; 
} __packed;

struct kgsl_snapshot_indexed_regs {
	__u32 index_reg; 
	__u32 data_reg;  
	int start;     
	int count;     
} __packed;

struct kgsl_snapshot_istore {
	int count;   
} __packed;


#define SNAPSHOT_DEBUG_SX         1
#define SNAPSHOT_DEBUG_CP         2
#define SNAPSHOT_DEBUG_SQ         3
#define SNAPSHOT_DEBUG_SQTHREAD   4
#define SNAPSHOT_DEBUG_MIU        5

#define SNAPSHOT_DEBUG_VPC_MEMORY 6
#define SNAPSHOT_DEBUG_CP_MEQ     7
#define SNAPSHOT_DEBUG_CP_PM4_RAM 8
#define SNAPSHOT_DEBUG_CP_PFP_RAM 9
#define SNAPSHOT_DEBUG_CP_ROQ     10
#define SNAPSHOT_DEBUG_SHADER_MEMORY 11
#define SNAPSHOT_DEBUG_CP_MERCIU 12

struct kgsl_snapshot_debug {
	int type;    
	int size;   
} __packed;

struct kgsl_snapshot_debugbus {
	int id;	   
	int count; 
} __packed;

#define SNAPSHOT_GPU_OBJECT_SHADER  1
#define SNAPSHOT_GPU_OBJECT_IB      2
#define SNAPSHOT_GPU_OBJECT_GENERIC 3

struct kgsl_snapshot_gpu_object {
	int type;      
	__u32 gpuaddr; 
	__u32 ptbase;  
	int size;    
};

#ifdef __KERNEL__

#define KGSL_SNAPSHOT_MEMSIZE (512 * 1024)

struct kgsl_device;

#define SNAPSHOT_ERR_NOMEM(_d, _s) \
	KGSL_DRV_ERR((_d), \
	"snapshot: not enough snapshot memory for section %s\n", (_s))


static inline void *kgsl_snapshot_add_section(struct kgsl_device *device,
	u16 id, void *snapshot, int *remain,
	int (*func)(struct kgsl_device *, void *, int, void *), void *priv)
{
	struct kgsl_snapshot_section_header *header = snapshot;
	void *data = snapshot + sizeof(*header);
	int ret = 0;


	if (*remain < sizeof(*header))
		return snapshot;

	

	if (func) {
		ret = func(device, data, *remain, priv);


		if (ret == 0)
			return snapshot;
	}

	header->magic = SNAPSHOT_SECTION_MAGIC;
	header->id = id;
	header->size = ret + sizeof(*header);

	
	*remain -= header->size;
	
	return snapshot + header->size;
}


struct kgsl_snapshot_registers {
	unsigned int *regs;  
	int count;	     
};

struct kgsl_snapshot_registers_list {
	
	struct kgsl_snapshot_registers *registers;
	
	int count;
};

int kgsl_snapshot_dump_regs(struct kgsl_device *device, void *snapshot,
	int remain, void *priv);


struct kgsl_snapshot_indexed_registers {
	unsigned int index; 
	unsigned int data;  
	unsigned int start;	
	unsigned int count; 
};


void *kgsl_snapshot_indexed_registers(struct kgsl_device *device,
	void *snapshot, int *remain, unsigned int index,
	unsigned int data, unsigned int start, unsigned int count);

int kgsl_snapshot_get_object(struct kgsl_device *device, unsigned int ptbase,
	unsigned int gpuaddr, unsigned int size, unsigned int type);

int kgsl_snapshot_have_object(struct kgsl_device *device, unsigned int ptbase,
	unsigned int gpuaddr, unsigned int size);

#endif
#endif
