/* Copyright (c) 2008-2012, The Linux Foundation. All rights reserved.
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
#ifndef __ADRENO_H
#define __ADRENO_H

#include "kgsl_device.h"
#include "adreno_drawctxt.h"
#include "adreno_ringbuffer.h"
#include "kgsl_iommu.h"
#include <mach/ocmem.h>

#define DEVICE_3D_NAME "kgsl-3d"
#define DEVICE_3D0_NAME "kgsl-3d0"

#define ADRENO_DEVICE(device) \
		KGSL_CONTAINER_OF(device, struct adreno_device, dev)

#define ADRENO_CHIPID_CORE(_id) (((_id) >> 24) & 0xFF)
#define ADRENO_CHIPID_MAJOR(_id) (((_id) >> 16) & 0xFF)
#define ADRENO_CHIPID_MINOR(_id) (((_id) >> 8) & 0xFF)
#define ADRENO_CHIPID_PATCH(_id) ((_id) & 0xFF)

#define KGSL_CMD_FLAGS_NONE             0x00000000
#define KGSL_CMD_FLAGS_PMODE		0x00000001
#define KGSL_CMD_FLAGS_INTERNAL_ISSUE	0x00000002
#define KGSL_CMD_FLAGS_GET_INT		0x00000004
#define KGSL_CMD_FLAGS_EOF	        0x00000100

#define KGSL_CONTEXT_TO_MEM_IDENTIFIER	0x2EADBEEF
#define KGSL_CMD_IDENTIFIER		0x2EEDFACE
#define KGSL_CMD_INTERNAL_IDENTIFIER	0x2EEDD00D
#define KGSL_START_OF_IB_IDENTIFIER	0x2EADEABE
#define KGSL_END_OF_IB_IDENTIFIER	0x2ABEDEAD
#define KGSL_END_OF_FRAME_IDENTIFIER	0x2E0F2E0F
#define KGSL_NOP_IB_IDENTIFIER	        0x20F20F20

#ifdef CONFIG_MSM_SCM
#define ADRENO_DEFAULT_PWRSCALE_POLICY  (&kgsl_pwrscale_policy_tz)
#elif defined CONFIG_MSM_SLEEP_STATS_DEVICE
#define ADRENO_DEFAULT_PWRSCALE_POLICY  (&kgsl_pwrscale_policy_idlestats)
#else
#define ADRENO_DEFAULT_PWRSCALE_POLICY  NULL
#endif

void adreno_debugfs_init(struct kgsl_device *device);

#define ADRENO_ISTORE_START 0x5000 

#define ADRENO_NUM_CTX_SWITCH_ALLOWED_BEFORE_DRAW	50


#define ADRENO_IDLE_TIMEOUT (20 * 1000)

enum adreno_gpurev {
	ADRENO_REV_UNKNOWN = 0,
	ADRENO_REV_A200 = 200,
	ADRENO_REV_A203 = 203,
	ADRENO_REV_A205 = 205,
	ADRENO_REV_A220 = 220,
	ADRENO_REV_A225 = 225,
	ADRENO_REV_A305 = 305,
	ADRENO_REV_A320 = 320,
	ADRENO_REV_A330 = 330,
};

struct adreno_gpudev;

struct adreno_device {
	struct kgsl_device dev;    
	unsigned int chip_id;
	enum adreno_gpurev gpurev;
	unsigned long gmem_base;
	unsigned int gmem_size;
	struct adreno_context *drawctxt_active;
	const char *pfp_fwfile;
	unsigned int *pfp_fw;
	size_t pfp_fw_size;
	unsigned int pfp_fw_version;
	const char *pm4_fwfile;
	unsigned int *pm4_fw;
	size_t pm4_fw_size;
	unsigned int pm4_fw_version;
	struct adreno_ringbuffer ringbuffer;
	unsigned int mharb;
	struct adreno_gpudev *gpudev;
	unsigned int wait_timeout;
	unsigned int istore_size;
	unsigned int pix_shader_start;
	unsigned int instruction_size;
	unsigned int ib_check_level;
	unsigned int fast_hang_detect;
	unsigned int ft_policy;
	unsigned int ft_user_control;
	unsigned int long_ib_detect;
	unsigned int long_ib;
	unsigned int long_ib_ts;
	unsigned int ft_pf_policy;
	unsigned int gpulist_index;
	struct ocmem_buf *ocmem_hdl;
	unsigned int ocmem_base;
	unsigned int gpu_cycles;
};

#define PERFCOUNTER_FLAG_NONE 0x0
#define PERFCOUNTER_FLAG_KERNEL 0x1


struct adreno_perfcount_register {
	unsigned int countable;
	unsigned int refcount;
	unsigned int offset;
	unsigned int flags;
};

struct adreno_perfcount_group {
	struct adreno_perfcount_register *regs;
	unsigned int reg_count;
};

struct adreno_perfcounters {
	struct adreno_perfcount_group *groups;
	unsigned int group_count;
};

struct adreno_gpudev {
	unsigned int reg_rbbm_status;
	unsigned int reg_cp_pfp_ucode_data;
	unsigned int reg_cp_pfp_ucode_addr;
	
	int ctx_switches_since_last_draw;

	struct adreno_perfcounters *perfcounters;

	
	int (*ctxt_create)(struct adreno_device *, struct adreno_context *);
	void (*ctxt_save)(struct adreno_device *, struct adreno_context *);
	void (*ctxt_restore)(struct adreno_device *, struct adreno_context *);
	void (*ctxt_draw_workaround)(struct adreno_device *,
					struct adreno_context *);
	irqreturn_t (*irq_handler)(struct adreno_device *);
	void (*irq_control)(struct adreno_device *, int);
	unsigned int (*irq_pending)(struct adreno_device *);
	void * (*snapshot)(struct adreno_device *, void *, int *, int);
	int (*rb_init)(struct adreno_device *, struct adreno_ringbuffer *);
	void (*perfcounter_init)(struct adreno_device *);
	void (*start)(struct adreno_device *);
	unsigned int (*busy_cycles)(struct adreno_device *);
	void (*perfcounter_enable)(struct adreno_device *, unsigned int group,
		unsigned int counter, unsigned int countable);
	uint64_t (*perfcounter_read)(struct adreno_device *adreno_dev,
		unsigned int group, unsigned int counter,
		unsigned int offset);
};

struct adreno_ft_data {
	unsigned int ib1;
	unsigned int context_id;
	unsigned int global_eop;
	unsigned int *rb_buffer;
	unsigned int rb_size;
	unsigned int *bad_rb_buffer;
	unsigned int bad_rb_size;
	unsigned int *good_rb_buffer;
	unsigned int good_rb_size;
	unsigned int last_valid_ctx_id;
	unsigned int status;
	unsigned int ft_policy;
	unsigned int err_code;
	unsigned int start_of_replay_cmds;
	unsigned int replay_for_snapshot;
};

#define  KGSL_FT_DISABLE                  BIT(0)
#define  KGSL_FT_REPLAY                   BIT(1)
#define  KGSL_FT_SKIPIB                   BIT(2)
#define  KGSL_FT_SKIPFRAME                BIT(3)
#define  KGSL_FT_TEMP_DISABLE             BIT(4)
#define  KGSL_FT_DEFAULT_POLICY           (KGSL_FT_REPLAY + KGSL_FT_SKIPIB)

#define KGSL_FT_PAGEFAULT_INT_ENABLE         0x00000001
#define KGSL_FT_PAGEFAULT_GPUHALT_ENABLE     0x00000002
#define KGSL_FT_PAGEFAULT_LOG_ONE_PER_PAGE   0x00000004
#define KGSL_FT_PAGEFAULT_LOG_ONE_PER_INT    0x00000008
#define KGSL_FT_PAGEFAULT_DEFAULT_POLICY     (KGSL_FT_PAGEFAULT_INT_ENABLE + \
					KGSL_FT_PAGEFAULT_GPUHALT_ENABLE)

extern struct adreno_gpudev adreno_a2xx_gpudev;
extern struct adreno_gpudev adreno_a3xx_gpudev;

extern const unsigned int a200_registers[];
extern const unsigned int a220_registers[];
extern const unsigned int a225_registers[];
extern const unsigned int a200_registers_count;
extern const unsigned int a220_registers_count;
extern const unsigned int a225_registers_count;

extern const unsigned int a3xx_registers[];
extern const unsigned int a3xx_registers_count;

extern const unsigned int a3xx_hlsq_registers[];
extern const unsigned int a3xx_hlsq_registers_count;

extern const unsigned int a330_registers[];
extern const unsigned int a330_registers_count;

extern unsigned int ft_detect_regs[];
extern const unsigned int ft_detect_regs_count;


int adreno_idle(struct kgsl_device *device);
void adreno_regread(struct kgsl_device *device, unsigned int offsetwords,
				unsigned int *value);
void adreno_regwrite(struct kgsl_device *device, unsigned int offsetwords,
				unsigned int value);

int adreno_dump(struct kgsl_device *device, int manual);
unsigned int adreno_a3xx_rbbm_clock_ctl_default(struct adreno_device
							*adreno_dev);

struct kgsl_memdesc *adreno_find_region(struct kgsl_device *device,
						unsigned int pt_base,
						unsigned int gpuaddr,
						unsigned int size);

uint8_t *adreno_convertaddr(struct kgsl_device *device,
	unsigned int pt_base, unsigned int gpuaddr, unsigned int size);

struct kgsl_memdesc *adreno_find_ctxtmem(struct kgsl_device *device,
	unsigned int pt_base, unsigned int gpuaddr, unsigned int size);

void *adreno_snapshot(struct kgsl_device *device, void *snapshot, int *remain,
		int hang);

int adreno_dump_and_exec_ft(struct kgsl_device *device);

void adreno_dump_rb(struct kgsl_device *device, const void *buf,
			 size_t len, int start, int size);

unsigned int adreno_ft_detect(struct kgsl_device *device,
						unsigned int *prev_reg_val);

int adreno_perfcounter_get(struct adreno_device *adreno_dev,
	unsigned int groupid, unsigned int countable, unsigned int *offset,
	unsigned int flags);

int adreno_perfcounter_put(struct adreno_device *adreno_dev,
	unsigned int groupid, unsigned int countable);

static inline int adreno_is_a200(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev == ADRENO_REV_A200);
}

static inline int adreno_is_a203(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev == ADRENO_REV_A203);
}

static inline int adreno_is_a205(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev == ADRENO_REV_A205);
}

static inline int adreno_is_a20x(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev <= 209);
}

static inline int adreno_is_a220(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev == ADRENO_REV_A220);
}

static inline int adreno_is_a225(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev == ADRENO_REV_A225);
}

static inline int adreno_is_a22x(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev  == ADRENO_REV_A220 ||
		adreno_dev->gpurev == ADRENO_REV_A225);
}

static inline int adreno_is_a2xx(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev <= 299);
}

static inline int adreno_is_a3xx(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev >= 300);
}

static inline int adreno_is_a305(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev == ADRENO_REV_A305);
}

static inline int adreno_is_a320(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev == ADRENO_REV_A320);
}

static inline int adreno_is_a330(struct adreno_device *adreno_dev)
{
	return (adreno_dev->gpurev == ADRENO_REV_A330);
}

static inline int adreno_is_a330v2(struct adreno_device *adreno_dev)
{
	return ((adreno_dev->gpurev == ADRENO_REV_A330) &&
		(ADRENO_CHIPID_PATCH(adreno_dev->chip_id) > 0));
}

static inline int adreno_rb_ctxtswitch(unsigned int *cmd)
{
	return (cmd[0] == cp_nop_packet(1) &&
		cmd[1] == KGSL_CONTEXT_TO_MEM_IDENTIFIER);
}

static inline int adreno_context_timestamp(struct kgsl_context *k_ctxt,
		struct adreno_ringbuffer *rb)
{
	struct adreno_context *a_ctxt = NULL;

	if (k_ctxt)
		a_ctxt = k_ctxt->devctxt;

	if (a_ctxt && a_ctxt->flags & CTXT_FLAGS_PER_CONTEXT_TS)
		return a_ctxt->timestamp;

	return rb->global_ts;
}

static inline int adreno_encode_istore_size(struct adreno_device *adreno_dev)
{
	unsigned int size;
	if (adreno_is_a225(adreno_dev))
		size = adreno_dev->istore_size/3;
	else
		size = adreno_dev->istore_size;

	return (ilog2(size) - 5) << 29;
}

static inline int __adreno_add_idle_indirect_cmds(unsigned int *cmds,
						unsigned int nop_gpuaddr)
{
	*cmds++ = CP_HDR_INDIRECT_BUFFER_PFD;
	*cmds++ = nop_gpuaddr;
	*cmds++ = 2;
	*cmds++ = cp_type3_packet(CP_WAIT_FOR_IDLE, 1);
	*cmds++ = 0x00000000;
	return 5;
}

static inline int adreno_add_change_mh_phys_limit_cmds(unsigned int *cmds,
						unsigned int new_phys_limit,
						unsigned int nop_gpuaddr)
{
	unsigned int *start = cmds;

	*cmds++ = cp_type0_packet(MH_MMU_MPU_END, 1);
	*cmds++ = new_phys_limit;
	cmds += __adreno_add_idle_indirect_cmds(cmds, nop_gpuaddr);
	return cmds - start;
}

static inline int adreno_add_bank_change_cmds(unsigned int *cmds,
					int cur_ctx_bank,
					unsigned int nop_gpuaddr)
{
	unsigned int *start = cmds;

	*cmds++ = cp_type0_packet(REG_CP_STATE_DEBUG_INDEX, 1);
	*cmds++ = (cur_ctx_bank ? 0 : 0x20);
	cmds += __adreno_add_idle_indirect_cmds(cmds, nop_gpuaddr);
	return cmds - start;
}

static inline int adreno_add_read_cmds(struct kgsl_device *device,
				unsigned int *cmds, unsigned int addr,
				unsigned int val, unsigned int nop_gpuaddr)
{
	unsigned int *start = cmds;

	*cmds++ = cp_type3_packet(CP_WAIT_REG_MEM, 5);
	
	*cmds++ = 0x13;
	*cmds++ = addr;
	*cmds++ = val;
	*cmds++ = 0xFFFFFFFF;
	*cmds++ = 0xFFFFFFFF;
	cmds += __adreno_add_idle_indirect_cmds(cmds, nop_gpuaddr);
	return cmds - start;
}

static inline int adreno_add_idle_cmds(struct adreno_device *adreno_dev,
							unsigned int *cmds)
{
	unsigned int *start = cmds;

	*cmds++ = cp_type3_packet(CP_WAIT_FOR_IDLE, 1);
	*cmds++ = 0x00000000;

	if ((adreno_dev->gpurev == ADRENO_REV_A305) ||
		(adreno_dev->gpurev == ADRENO_REV_A320)) {
		*cmds++ = cp_type3_packet(CP_WAIT_FOR_ME, 1);
		*cmds++ = 0x00000000;
	}

	return cmds - start;
}

#endif 
