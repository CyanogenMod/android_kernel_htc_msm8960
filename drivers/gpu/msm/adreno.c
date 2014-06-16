/* Copyright (c) 2002,2007-2013, The Linux Foundation. All rights reserved.
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
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/ioctl.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/msm_kgsl.h>
#include <linux/delay.h>

#include <mach/socinfo.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_bus.h>
#include <mach/msm_dcvs.h>
#include <mach/msm_dcvs_scm.h>
#include <mach/board.h>

#include "kgsl.h"
#include "kgsl_pwrscale.h"
#include "kgsl_cffdump.h"
#include "kgsl_sharedmem.h"
#include "kgsl_iommu.h"

#include "adreno.h"
#include "adreno_pm4types.h"

#include "a2xx_reg.h"
#include "a3xx_reg.h"

#define DRIVER_VERSION_MAJOR   3
#define DRIVER_VERSION_MINOR   1

#define ADRENO_CFG_MHARB \
	(0x10 \
		| (0 << MH_ARBITER_CONFIG__SAME_PAGE_GRANULARITY__SHIFT) \
		| (1 << MH_ARBITER_CONFIG__L1_ARB_ENABLE__SHIFT) \
		| (1 << MH_ARBITER_CONFIG__L1_ARB_HOLD_ENABLE__SHIFT) \
		| (0 << MH_ARBITER_CONFIG__L2_ARB_CONTROL__SHIFT) \
		| (1 << MH_ARBITER_CONFIG__PAGE_SIZE__SHIFT) \
		| (1 << MH_ARBITER_CONFIG__TC_REORDER_ENABLE__SHIFT) \
		| (1 << MH_ARBITER_CONFIG__TC_ARB_HOLD_ENABLE__SHIFT) \
		| (0 << MH_ARBITER_CONFIG__IN_FLIGHT_LIMIT_ENABLE__SHIFT) \
		| (0x8 << MH_ARBITER_CONFIG__IN_FLIGHT_LIMIT__SHIFT) \
		| (1 << MH_ARBITER_CONFIG__CP_CLNT_ENABLE__SHIFT) \
		| (1 << MH_ARBITER_CONFIG__VGT_CLNT_ENABLE__SHIFT) \
		| (1 << MH_ARBITER_CONFIG__TC_CLNT_ENABLE__SHIFT) \
		| (1 << MH_ARBITER_CONFIG__RB_CLNT_ENABLE__SHIFT) \
		| (1 << MH_ARBITER_CONFIG__PA_CLNT_ENABLE__SHIFT))

#define ADRENO_MMU_CONFIG						\
	(0x01								\
	 | (MMU_CONFIG << MH_MMU_CONFIG__RB_W_CLNT_BEHAVIOR__SHIFT)	\
	 | (MMU_CONFIG << MH_MMU_CONFIG__CP_W_CLNT_BEHAVIOR__SHIFT)	\
	 | (MMU_CONFIG << MH_MMU_CONFIG__CP_R0_CLNT_BEHAVIOR__SHIFT)	\
	 | (MMU_CONFIG << MH_MMU_CONFIG__CP_R1_CLNT_BEHAVIOR__SHIFT)	\
	 | (MMU_CONFIG << MH_MMU_CONFIG__CP_R2_CLNT_BEHAVIOR__SHIFT)	\
	 | (MMU_CONFIG << MH_MMU_CONFIG__CP_R3_CLNT_BEHAVIOR__SHIFT)	\
	 | (MMU_CONFIG << MH_MMU_CONFIG__CP_R4_CLNT_BEHAVIOR__SHIFT)	\
	 | (MMU_CONFIG << MH_MMU_CONFIG__VGT_R0_CLNT_BEHAVIOR__SHIFT)	\
	 | (MMU_CONFIG << MH_MMU_CONFIG__VGT_R1_CLNT_BEHAVIOR__SHIFT)	\
	 | (MMU_CONFIG << MH_MMU_CONFIG__TC_R_CLNT_BEHAVIOR__SHIFT)	\
	 | (MMU_CONFIG << MH_MMU_CONFIG__PA_W_CLNT_BEHAVIOR__SHIFT))

#define KGSL_LOG_LEVEL_DEFAULT 3

#ifndef CONFIG_DEBUG_FS
unsigned int kgsl_cff_dump_enable;
#endif

static const struct kgsl_functable adreno_functable;

struct kgsl_process_name {
	char name[TASK_COMM_LEN+1];
};

static const struct kgsl_process_name kgsl_blocking_process_tbl[] = {
	{"SurfaceFlinger"},
	{"surfaceflinger"},
	{"ndroid.systemui"},
	{"droid.htcdialer"},
	{"m.android.phone"},
	{"mediaserver"},
};

static struct adreno_device device_3d0 = {
	.dev = {
		KGSL_DEVICE_COMMON_INIT(device_3d0.dev),
		.name = DEVICE_3D0_NAME,
		.id = KGSL_DEVICE_3D0,
		.mh = {
			.mharb  = ADRENO_CFG_MHARB,
			.mh_intf_cfg1 = 0x00032f07,
			.mpu_base = 0x00000000,
			.mpu_range =  0xFFFFF000,
		},
		.mmu = {
			.config = ADRENO_MMU_CONFIG,
		},
		.pwrctrl = {
			.irq_name = KGSL_3D0_IRQ,
		},
		.iomemname = KGSL_3D0_REG_MEMORY,
		.ftbl = &adreno_functable,
#ifdef CONFIG_HAS_EARLYSUSPEND
		.display_off = {
			.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING,
			.suspend = kgsl_early_suspend_driver,
			.resume = kgsl_late_resume_driver,
		},
#endif
		.cmd_log = KGSL_LOG_LEVEL_DEFAULT,
		.ctxt_log = KGSL_LOG_LEVEL_DEFAULT,
		.drv_log = KGSL_LOG_LEVEL_DEFAULT,
		.mem_log = KGSL_LOG_LEVEL_DEFAULT,
		.pwr_log = KGSL_LOG_LEVEL_DEFAULT,
		.ft_log = KGSL_LOG_LEVEL_DEFAULT,
		.pm_dump_enable = 0,
	},
	.gmem_base = 0,
	.gmem_size = SZ_256K,
	.pfp_fw = NULL,
	.pm4_fw = NULL,
	.wait_timeout = 0, 
	.ib_check_level = 0,
	.ft_policy = KGSL_FT_DEFAULT_POLICY,
	.ft_pf_policy = KGSL_FT_PAGEFAULT_DEFAULT_POLICY,
	.fast_hang_detect = 1,
	.long_ib_detect = 1,
};

#define LONG_IB_DETECT_REG_INDEX_START 1
#define LONG_IB_DETECT_REG_INDEX_END 5

unsigned int ft_detect_regs[FT_DETECT_REGS_COUNT] = {
	A3XX_RBBM_STATUS,
	REG_CP_RB_RPTR,   
	REG_CP_IB1_BASE,
	REG_CP_IB1_BUFSZ,
	REG_CP_IB2_BASE,
	REG_CP_IB2_BUFSZ, 
	0,
	0,
	0,
	0,
	0,
	0
};


#define ANY_ID (~0)
#define NO_VER (~0)

static const struct {
	enum adreno_gpurev gpurev;
	unsigned int core, major, minor, patchid;
	const char *pm4fw;
	const char *pfpfw;
	struct adreno_gpudev *gpudev;
	unsigned int istore_size;
	unsigned int pix_shader_start;
	
	unsigned int instruction_size;
	
	unsigned int gmem_size;
	unsigned int sync_lock_pm4_ver;
	unsigned int sync_lock_pfp_ver;
} adreno_gpulist[] = {
	{ ADRENO_REV_A200, 0, 2, ANY_ID, ANY_ID,
		"yamato_pm4.fw", "yamato_pfp.fw", &adreno_a2xx_gpudev,
		512, 384, 3, SZ_256K, NO_VER, NO_VER },
	{ ADRENO_REV_A203, 0, 1, 1, ANY_ID,
		"yamato_pm4.fw", "yamato_pfp.fw", &adreno_a2xx_gpudev,
		512, 384, 3, SZ_256K, NO_VER, NO_VER },
	{ ADRENO_REV_A205, 0, 1, 0, ANY_ID,
		"yamato_pm4.fw", "yamato_pfp.fw", &adreno_a2xx_gpudev,
		512, 384, 3, SZ_256K, NO_VER, NO_VER },
	{ ADRENO_REV_A220, 2, 1, ANY_ID, ANY_ID,
		"leia_pm4_470.fw", "leia_pfp_470.fw", &adreno_a2xx_gpudev,
		512, 384, 3, SZ_512K, NO_VER, NO_VER },
	{ ADRENO_REV_A225, 2, 2, 0, 5,
		"a225p5_pm4.fw", "a225_pfp.fw", &adreno_a2xx_gpudev,
		1536, 768, 3, SZ_512K, NO_VER, NO_VER },
	{ ADRENO_REV_A225, 2, 2, 0, 6,
		"a225_pm4.fw", "a225_pfp.fw", &adreno_a2xx_gpudev,
		1536, 768, 3, SZ_512K, 0x225011, 0x225002 },
	{ ADRENO_REV_A225, 2, 2, ANY_ID, ANY_ID,
		"a225_pm4.fw", "a225_pfp.fw", &adreno_a2xx_gpudev,
		1536, 768, 3, SZ_512K, 0x225011, 0x225002 },
	
	{ ADRENO_REV_A305, 3, 0, 5, ANY_ID,
		"a300_pm4.fw", "a300_pfp.fw", &adreno_a3xx_gpudev,
		512, 0, 2, SZ_256K, 0x3FF037, 0x3FF016 },
	
	{ ADRENO_REV_A320, 3, 2, ANY_ID, ANY_ID,
		"a300_pm4.fw", "a300_pfp.fw", &adreno_a3xx_gpudev,
		512, 0, 2, SZ_512K, 0x3FF037, 0x3FF016 },
	{ ADRENO_REV_A330, 3, 3, 0, ANY_ID,
		"a330_pm4.fw", "a330_pfp.fw", &adreno_a3xx_gpudev,
		512, 0, 2, SZ_1M, NO_VER, NO_VER },
};

static unsigned int adreno_isidle(struct kgsl_device *device);


static void adreno_perfcounter_init(struct kgsl_device *device)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

	if (adreno_dev->gpudev->perfcounter_init)
		adreno_dev->gpudev->perfcounter_init(adreno_dev);
};


static void adreno_perfcounter_start(struct adreno_device *adreno_dev)
{
	struct adreno_perfcounters *counters = adreno_dev->gpudev->perfcounters;
	struct adreno_perfcount_group *group;
	unsigned int i, j;

	
	for (i = 0; i < counters->group_count; i++) {
		group = &(counters->groups[i]);

		
		for (j = 0; j < group->reg_count; j++) {
			if (group->regs[j].countable ==
					KGSL_PERFCOUNTER_NOT_USED)
				continue;

			if (adreno_dev->gpudev->perfcounter_enable)
				adreno_dev->gpudev->perfcounter_enable(
					adreno_dev, i, j,
					group->regs[j].countable);
		}
	}
}


int adreno_perfcounter_read_group(struct adreno_device *adreno_dev,
	struct kgsl_perfcounter_read_group __user *reads, unsigned int count)
{
	struct adreno_perfcounters *counters = adreno_dev->gpudev->perfcounters;
	struct adreno_perfcount_group *group;
	struct kgsl_perfcounter_read_group *list = NULL;
	unsigned int i, j;
	int ret = 0;

	
	if (adreno_is_a2xx(adreno_dev))
		return -EINVAL;

	
	if (!adreno_dev->gpudev->perfcounter_read)
		return -EINVAL;

	
	if (reads == NULL || count == 0 || count > 100)
		return -EINVAL;

	list = kmalloc(sizeof(struct kgsl_perfcounter_read_group) * count,
			GFP_KERNEL);
	if (!list)
		return -ENOMEM;

	if (copy_from_user(list, reads,
			sizeof(struct kgsl_perfcounter_read_group) * count)) {
		ret = -EFAULT;
		goto done;
	}

	
	for (j = 0; j < count; j++) {
		list[j].value = 0;
		
		if (list[j].groupid >= counters->group_count) {
		ret = -EINVAL;
		goto done;
		}

		group = &(counters->groups[list[j].groupid]);

		
		for (i = 0; i < group->reg_count; i++) {
			if (group->regs[i].countable == list[j].countable) {
				list[j].value =
					adreno_dev->gpudev->perfcounter_read(
					adreno_dev, list[j].groupid,
					i, group->regs[i].offset);
				break;
			}
		}
	}

	
	if (copy_to_user(reads, list,
			sizeof(struct kgsl_perfcounter_read_group) *
			count) != 0)
		ret = -EFAULT;

done:
	kfree(list);
	return ret;
}


int adreno_perfcounter_query_group(struct adreno_device *adreno_dev,
	unsigned int groupid, unsigned int *countables, unsigned int count,
	unsigned int *max_counters)
{
	struct adreno_perfcounters *counters = adreno_dev->gpudev->perfcounters;
	struct adreno_perfcount_group *group;
	unsigned int i;

	*max_counters = 0;

	
	if (adreno_is_a2xx(adreno_dev))
		return -EINVAL;

	if (groupid >= counters->group_count)
		return -EINVAL;

	group = &(counters->groups[groupid]);
	*max_counters = group->reg_count;

	if (countables == NULL || count == 0)
		return 0;

	for (i = 0; i < group->reg_count && i < count; i++) {
		if (copy_to_user(&countables[i], &(group->regs[i].countable),
				sizeof(unsigned int)) != 0)
			return -EFAULT;
	}

	return 0;
}


int adreno_perfcounter_get(struct adreno_device *adreno_dev,
	unsigned int groupid, unsigned int countable, unsigned int *offset,
	unsigned int flags)
{
	struct adreno_perfcounters *counters = adreno_dev->gpudev->perfcounters;
	struct adreno_perfcount_group *group;
	unsigned int i, empty = -1;

	
	if (offset)
		*offset = 0;

	
	if (adreno_is_a2xx(adreno_dev))
		return -EINVAL;

	if (groupid >= counters->group_count)
		return -EINVAL;

	group = &(counters->groups[groupid]);

	for (i = 0; i < group->reg_count; i++) {
		if (group->regs[i].countable == countable) {
			
			group->regs[i].refcount++;
			group->regs[i].flags |= flags;
			if (offset)
				*offset = group->regs[i].offset;
			return 0;
		} else if (group->regs[i].countable ==
			KGSL_PERFCOUNTER_NOT_USED) {
			
			empty = i;
		}
	}

	
	if (empty == -1)
		return -EBUSY;

	
	group->regs[empty].countable = countable;
	group->regs[empty].refcount = 1;

	
	adreno_dev->gpudev->perfcounter_enable(adreno_dev, groupid, empty,
		countable);

	group->regs[empty].flags = flags;

	if (offset)
		*offset = group->regs[empty].offset;

	return 0;
}


int adreno_perfcounter_put(struct adreno_device *adreno_dev,
	unsigned int groupid, unsigned int countable)
{
	struct adreno_perfcounters *counters = adreno_dev->gpudev->perfcounters;
	struct adreno_perfcount_group *group;

	unsigned int i;

	
	if (adreno_is_a2xx(adreno_dev))
		return -EINVAL;

	if (groupid >= counters->group_count)
		return -EINVAL;

	group = &(counters->groups[groupid]);

	for (i = 0; i < group->reg_count; i++) {
		if (group->regs[i].countable == countable) {
			if (group->regs[i].refcount > 0) {
				group->regs[i].refcount--;

				if (group->regs[i].flags &&
					group->regs[i].refcount == 0)
					group->regs[i].refcount++;

				
				if (group->regs[i].refcount == 0)
					group->regs[i].countable =
						KGSL_PERFCOUNTER_NOT_USED;
			}

			return 0;
		}
	}

	return -EINVAL;
}

static irqreturn_t adreno_irq_handler(struct kgsl_device *device)
{
	irqreturn_t result;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

	result = adreno_dev->gpudev->irq_handler(adreno_dev);

	if (device->requested_state == KGSL_STATE_NONE) {
		if (device->pwrctrl.nap_allowed == true) {
			kgsl_pwrctrl_request_state(device, KGSL_STATE_NAP);
			queue_work(device->work_queue, &device->idle_check_ws);
		} else if (device->pwrscale.policy != NULL) {
			queue_work(device->work_queue, &device->idle_check_ws);
		}
	}

	
	mod_timer_pending(&device->idle_timer,
		jiffies + device->pwrctrl.interval_timeout);
	mod_timer_pending(&device->hang_timer,
		(jiffies + msecs_to_jiffies(KGSL_TIMEOUT_PART)));
	return result;
}

static void adreno_cleanup_pt(struct kgsl_device *device,
			struct kgsl_pagetable *pagetable)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_ringbuffer *rb = &adreno_dev->ringbuffer;

	kgsl_mmu_unmap(pagetable, &rb->buffer_desc);

	kgsl_mmu_unmap(pagetable, &rb->memptrs_desc);

	kgsl_mmu_unmap(pagetable, &device->memstore);

	kgsl_mmu_unmap(pagetable, &adreno_dev->pwron_fixup);

	kgsl_mmu_unmap(pagetable, &device->mmu.setstate_memory);
}

static int adreno_setup_pt(struct kgsl_device *device,
			struct kgsl_pagetable *pagetable)
{
	int result;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_ringbuffer *rb = &adreno_dev->ringbuffer;

	result = kgsl_mmu_map_global(pagetable, &rb->buffer_desc);
	if (result)
		goto error;

	result = kgsl_mmu_map_global(pagetable, &rb->memptrs_desc);
	if (result)
		goto unmap_buffer_desc;

	result = kgsl_mmu_map_global(pagetable, &device->memstore);
	if (result)
		goto unmap_memptrs_desc;

	result = kgsl_mmu_map_global(pagetable, &adreno_dev->pwron_fixup);
	if (result)
		goto unmap_memstore_desc;

	result = kgsl_mmu_map_global(pagetable, &device->mmu.setstate_memory);
	if (result)
		goto unmap_pwron_fixup_desc;

	device->mh.mpu_range = device->mmu.setstate_memory.gpuaddr +
				device->mmu.setstate_memory.size;
	return result;

unmap_pwron_fixup_desc:
	kgsl_mmu_unmap(pagetable, &adreno_dev->pwron_fixup);

unmap_memstore_desc:
	kgsl_mmu_unmap(pagetable, &device->memstore);

unmap_memptrs_desc:
	kgsl_mmu_unmap(pagetable, &rb->memptrs_desc);

unmap_buffer_desc:
	kgsl_mmu_unmap(pagetable, &rb->buffer_desc);

error:
	return result;
}

static bool adreno_use_default_setstate(struct adreno_device *adreno_dev)
{
	return (adreno_isidle(&adreno_dev->dev) ||
			KGSL_STATE_ACTIVE != adreno_dev->dev.state ||
			adreno_dev->dev.active_cnt == 0 ||
			kgsl_cff_dump_enable);
}

static void adreno_iommu_setstate(struct kgsl_device *device,
					unsigned int context_id,
					uint32_t flags)
{
	unsigned int pt_val, reg_pt_val;
	unsigned int link[250];
	unsigned int *cmds = &link[0];
	int sizedwords = 0;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	int num_iommu_units, i;
	struct kgsl_context *context;
	struct adreno_context *adreno_ctx = NULL;

	 if (adreno_use_default_setstate(adreno_dev))
		return kgsl_mmu_device_setstate(&device->mmu, flags);

	num_iommu_units = kgsl_mmu_get_num_iommu_units(&device->mmu);

	context = kgsl_context_get(device, context_id);

	if (context == NULL)
		return;
	adreno_ctx = context->devctxt;

	if (kgsl_mmu_enable_clk(&device->mmu,
				KGSL_IOMMU_CONTEXT_USER))
		goto done;

	cmds += __adreno_add_idle_indirect_cmds(cmds,
		device->mmu.setstate_memory.gpuaddr +
		KGSL_IOMMU_SETSTATE_NOP_OFFSET);

	if (cpu_is_msm8960())
		cmds += adreno_add_change_mh_phys_limit_cmds(cmds, 0xFFFFF000,
					device->mmu.setstate_memory.gpuaddr +
					KGSL_IOMMU_SETSTATE_NOP_OFFSET);
	else
		cmds += adreno_add_bank_change_cmds(cmds,
					KGSL_IOMMU_CONTEXT_USER,
					device->mmu.setstate_memory.gpuaddr +
					KGSL_IOMMU_SETSTATE_NOP_OFFSET);

	cmds += adreno_add_idle_cmds(adreno_dev, cmds);

	
	cmds += kgsl_mmu_sync_lock(&device->mmu, cmds);

	pt_val = kgsl_mmu_get_pt_base_addr(&device->mmu,
					device->mmu.hwpagetable);
	if (flags & KGSL_MMUFLAGS_PTUPDATE) {
		for (i = 0; i < num_iommu_units; i++) {
			reg_pt_val = (pt_val + kgsl_mmu_get_pt_lsb(&device->mmu,
						i, KGSL_IOMMU_CONTEXT_USER));
			*cmds++ = cp_type3_packet(CP_MEM_WRITE, 2);
			*cmds++ = kgsl_mmu_get_reg_gpuaddr(&device->mmu, i,
				KGSL_IOMMU_CONTEXT_USER, KGSL_IOMMU_CTX_TTBR0);
			*cmds++ = reg_pt_val;
			*cmds++ = cp_type3_packet(CP_WAIT_FOR_IDLE, 1);
			*cmds++ = 0x00000000;

			cmds += adreno_add_read_cmds(device, cmds,
				kgsl_mmu_get_reg_gpuaddr(&device->mmu, i,
				KGSL_IOMMU_CONTEXT_USER, KGSL_IOMMU_CTX_TTBR0),
				reg_pt_val,
				device->mmu.setstate_memory.gpuaddr +
				KGSL_IOMMU_SETSTATE_NOP_OFFSET);
		}
	}
	if (flags & KGSL_MMUFLAGS_TLBFLUSH) {
		for (i = 0; i < num_iommu_units; i++) {
			reg_pt_val = (pt_val + kgsl_mmu_get_pt_lsb(&device->mmu,
						i, KGSL_IOMMU_CONTEXT_USER));

			*cmds++ = cp_type3_packet(CP_MEM_WRITE, 2);
			*cmds++ = kgsl_mmu_get_reg_gpuaddr(&device->mmu, i,
				KGSL_IOMMU_CONTEXT_USER,
				KGSL_IOMMU_CTX_TLBIALL);
			*cmds++ = 1;

			cmds += __adreno_add_idle_indirect_cmds(cmds,
			device->mmu.setstate_memory.gpuaddr +
			KGSL_IOMMU_SETSTATE_NOP_OFFSET);

			cmds += adreno_add_read_cmds(device, cmds,
				kgsl_mmu_get_reg_gpuaddr(&device->mmu, i,
					KGSL_IOMMU_CONTEXT_USER,
					KGSL_IOMMU_CTX_TTBR0),
				reg_pt_val,
				device->mmu.setstate_memory.gpuaddr +
				KGSL_IOMMU_SETSTATE_NOP_OFFSET);
		}
	}

	
	cmds += kgsl_mmu_sync_unlock(&device->mmu, cmds);

	if (cpu_is_msm8960())
		cmds += adreno_add_change_mh_phys_limit_cmds(cmds,
			kgsl_mmu_get_reg_gpuaddr(&device->mmu, 0,
						0, KGSL_IOMMU_GLOBAL_BASE),
			device->mmu.setstate_memory.gpuaddr +
			KGSL_IOMMU_SETSTATE_NOP_OFFSET);
	else
		cmds += adreno_add_bank_change_cmds(cmds,
			KGSL_IOMMU_CONTEXT_PRIV,
			device->mmu.setstate_memory.gpuaddr +
			KGSL_IOMMU_SETSTATE_NOP_OFFSET);

	cmds += adreno_add_idle_cmds(adreno_dev, cmds);

	sizedwords += (cmds - &link[0]);
	if (sizedwords) {
		
		*cmds++ = cp_type3_packet(CP_INVALIDATE_STATE, 1);
		*cmds++ = 0x7fff;
		sizedwords += 2;
		adreno_ringbuffer_issuecmds(device, adreno_ctx,
			KGSL_CMD_FLAGS_PMODE,
			&link[0], sizedwords);
		kgsl_mmu_disable_clk_on_ts(&device->mmu,
				adreno_dev->ringbuffer.global_ts, true);
	}

	if (sizedwords > (sizeof(link)/sizeof(unsigned int))) {
		KGSL_DRV_ERR(device, "Temp command buffer overflow\n");
		BUG();
	}
done:
	kgsl_context_put(context);
}

static void adreno_gpummu_setstate(struct kgsl_device *device,
					unsigned int context_id,
					uint32_t flags)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	unsigned int link[32];
	unsigned int *cmds = &link[0];
	int sizedwords = 0;
	unsigned int mh_mmu_invalidate = 0x00000003; 
	struct kgsl_context *context;
	struct adreno_context *adreno_ctx = NULL;

	if (adreno_is_a20x(adreno_dev))
		flags |= KGSL_MMUFLAGS_TLBFLUSH;
	if (!adreno_use_default_setstate(adreno_dev)) {
		context = kgsl_context_get(device, context_id);
		if (context == NULL)
			return;
		adreno_ctx = context->devctxt;

		if (flags & KGSL_MMUFLAGS_PTUPDATE) {
			
			*cmds++ = cp_type3_packet(CP_WAIT_FOR_IDLE, 1);
			*cmds++ = 0x00000000;

			
			*cmds++ = cp_type0_packet(MH_MMU_PT_BASE, 1);
			*cmds++ = kgsl_mmu_get_pt_base_addr(&device->mmu,
					device->mmu.hwpagetable);
			sizedwords += 4;
		}

		if (flags & KGSL_MMUFLAGS_TLBFLUSH) {
			if (!(flags & KGSL_MMUFLAGS_PTUPDATE)) {
				*cmds++ = cp_type3_packet(CP_WAIT_FOR_IDLE,
								1);
				*cmds++ = 0x00000000;
				sizedwords += 2;
			}
			*cmds++ = cp_type0_packet(MH_MMU_INVALIDATE, 1);
			*cmds++ = mh_mmu_invalidate;
			sizedwords += 2;
		}

		if (flags & KGSL_MMUFLAGS_PTUPDATE &&
			adreno_is_a20x(adreno_dev)) {
			*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
			*cmds++ = (0x4 << 16) |
				(REG_PA_SU_SC_MODE_CNTL - 0x2000);
			*cmds++ = 0;	  
			*cmds++ = cp_type3_packet(CP_SET_BIN_BASE_OFFSET, 1);
			*cmds++ = device->mmu.setstate_memory.gpuaddr;
			*cmds++ = cp_type3_packet(CP_DRAW_INDX_BIN, 6);
			*cmds++ = 0;	  
			*cmds++ = 0x0003C004; 
			*cmds++ = 0;	  
			*cmds++ = 3;	  
			*cmds++ =
			device->mmu.setstate_memory.gpuaddr; 
			*cmds++ = 6;	  
			*cmds++ = cp_type3_packet(CP_DRAW_INDX_BIN, 6);
			*cmds++ = 0;	  
			*cmds++ = 0x0003C004; 
			*cmds++ = 0;	  
			*cmds++ = 3;	  
			
			*cmds++ = device->mmu.setstate_memory.gpuaddr;
			*cmds++ = 6;	  
			*cmds++ = cp_type3_packet(CP_WAIT_FOR_IDLE, 1);
			*cmds++ = 0x00000000;
			sizedwords += 21;
		}


		if (flags & (KGSL_MMUFLAGS_PTUPDATE | KGSL_MMUFLAGS_TLBFLUSH)) {
			*cmds++ = cp_type3_packet(CP_INVALIDATE_STATE, 1);
			*cmds++ = 0x7fff; 
			sizedwords += 2;
		}

		adreno_ringbuffer_issuecmds(device, adreno_ctx,
					KGSL_CMD_FLAGS_PMODE,
					&link[0], sizedwords);

		kgsl_context_put(context);
	} else {
		kgsl_mmu_device_setstate(&device->mmu, flags);
	}
}

static void adreno_setstate(struct kgsl_device *device,
			unsigned int context_id,
			uint32_t flags)
{
	
	if (KGSL_MMU_TYPE_GPU == kgsl_mmu_get_mmutype())
		return adreno_gpummu_setstate(device, context_id, flags);
	else if (KGSL_MMU_TYPE_IOMMU == kgsl_mmu_get_mmutype())
		return adreno_iommu_setstate(device, context_id, flags);
}

static unsigned int
a3xx_getchipid(struct kgsl_device *device)
{
	struct kgsl_device_platform_data *pdata =
		kgsl_device_get_drvdata(device);


	return pdata->chipid;
}

static unsigned int
a2xx_getchipid(struct kgsl_device *device)
{
	unsigned int chipid = 0;
	unsigned int coreid, majorid, minorid, patchid, revid;
	struct kgsl_device_platform_data *pdata =
		kgsl_device_get_drvdata(device);

	

	if (pdata->chipid != 0)
		return pdata->chipid;

	adreno_regread(device, REG_RBBM_PERIPHID1, &coreid);
	adreno_regread(device, REG_RBBM_PERIPHID2, &majorid);
	adreno_regread(device, REG_RBBM_PATCH_RELEASE, &revid);

	if (cpu_is_msm8x60())
		chipid = 2 << 24;
	else
		chipid = (coreid & 0xF) << 24;

	chipid |= ((majorid >> 4) & 0xF) << 16;

	minorid = ((revid >> 0)  & 0xFF);

	patchid = ((revid >> 16) & 0xFF);

	
	
	if (cpu_is_qsd8x50())
		patchid = 1;
	else if (cpu_is_msm8625() && minorid == 0)
		minorid = 1;

	chipid |= (minorid << 8) | patchid;

	return chipid;
}

static unsigned int
adreno_getchipid(struct kgsl_device *device)
{
	struct kgsl_device_platform_data *pdata =
		kgsl_device_get_drvdata(device);


	if (pdata->chipid == 0 || ADRENO_CHIPID_MAJOR(pdata->chipid) == 2)
		return a2xx_getchipid(device);
	else
		return a3xx_getchipid(device);
}

static inline bool _rev_match(unsigned int id, unsigned int entry)
{
	return (entry == ANY_ID || entry == id);
}

static void
adreno_identify_gpu(struct adreno_device *adreno_dev)
{
	unsigned int i, core, major, minor, patchid;

	adreno_dev->chip_id = adreno_getchipid(&adreno_dev->dev);

	core = ADRENO_CHIPID_CORE(adreno_dev->chip_id);
	major = ADRENO_CHIPID_MAJOR(adreno_dev->chip_id);
	minor = ADRENO_CHIPID_MINOR(adreno_dev->chip_id);
	patchid = ADRENO_CHIPID_PATCH(adreno_dev->chip_id);

	for (i = 0; i < ARRAY_SIZE(adreno_gpulist); i++) {
		if (core == adreno_gpulist[i].core &&
		    _rev_match(major, adreno_gpulist[i].major) &&
		    _rev_match(minor, adreno_gpulist[i].minor) &&
		    _rev_match(patchid, adreno_gpulist[i].patchid))
			break;
	}

	if (i == ARRAY_SIZE(adreno_gpulist)) {
		adreno_dev->gpurev = ADRENO_REV_UNKNOWN;
		return;
	}

	adreno_dev->gpurev = adreno_gpulist[i].gpurev;
	adreno_dev->gpudev = adreno_gpulist[i].gpudev;
	adreno_dev->pfp_fwfile = adreno_gpulist[i].pfpfw;
	adreno_dev->pm4_fwfile = adreno_gpulist[i].pm4fw;
	adreno_dev->istore_size = adreno_gpulist[i].istore_size;
	adreno_dev->pix_shader_start = adreno_gpulist[i].pix_shader_start;
	adreno_dev->instruction_size = adreno_gpulist[i].instruction_size;
	adreno_dev->gmem_size = adreno_gpulist[i].gmem_size;
	adreno_dev->gpulist_index = i;

}

static struct platform_device_id adreno_id_table[] = {
	{ DEVICE_3D0_NAME, (kernel_ulong_t)&device_3d0.dev, },
	{},
};

MODULE_DEVICE_TABLE(platform, adreno_id_table);

static struct of_device_id adreno_match_table[] = {
	{ .compatible = "qcom,kgsl-3d0", },
	{}
};

#if 0
static inline int adreno_of_read_property(struct device_node *node,
	const char *prop, unsigned int *ptr)
{
	int ret = of_property_read_u32(node, prop, ptr);
	if (ret)
		KGSL_CORE_ERR("Unable to read '%s'\n", prop);
	return ret;
}

static struct device_node *adreno_of_find_subnode(struct device_node *parent,
	const char *name)
{
	struct device_node *child;

	for_each_child_of_node(parent, child) {
		if (of_device_is_compatible(child, name))
			return child;
	}

	return NULL;
}

static int adreno_of_get_pwrlevels(struct device_node *parent,
	struct kgsl_device_platform_data *pdata)
{
	struct device_node *node, *child;
	int ret = -EINVAL;

	node = adreno_of_find_subnode(parent, "qcom,gpu-pwrlevels");

	if (node == NULL) {
		KGSL_CORE_ERR("Unable to find 'qcom,gpu-pwrlevels'\n");
		return -EINVAL;
	}

	pdata->num_levels = 0;

	for_each_child_of_node(node, child) {
		unsigned int index;
		struct kgsl_pwrlevel *level;

		if (adreno_of_read_property(child, "reg", &index))
			goto done;

		if (index >= KGSL_MAX_PWRLEVELS) {
			KGSL_CORE_ERR("Pwrlevel index %d is out of range\n",
				index);
			continue;
		}

		if (index >= pdata->num_levels)
			pdata->num_levels = index + 1;

		level = &pdata->pwrlevel[index];

		if (adreno_of_read_property(child, "qcom,gpu-freq",
			&level->gpu_freq))
			goto done;

		if (adreno_of_read_property(child, "qcom,bus-freq",
			&level->bus_freq))
			goto done;

		if (adreno_of_read_property(child, "qcom,io-fraction",
			&level->io_fraction))
			level->io_fraction = 0;
	}

	if (adreno_of_read_property(parent, "qcom,initial-pwrlevel",
		&pdata->init_level))
		pdata->init_level = 1;

	if (pdata->init_level < 0 || pdata->init_level > pdata->num_levels) {
		KGSL_CORE_ERR("Initial power level out of range\n");
		pdata->init_level = 1;
	}

	ret = 0;
done:
	return ret;

}

static struct msm_dcvs_core_info *adreno_of_get_dcvs(struct device_node *parent)
{
	struct device_node *node, *child;
	struct msm_dcvs_core_info *info = NULL;
	int count = 0;
	int ret = -EINVAL;

	node = adreno_of_find_subnode(parent, "qcom,dcvs-core-info");
	if (node == NULL)
		return ERR_PTR(-EINVAL);

	info = kzalloc(sizeof(*info), GFP_KERNEL);

	if (info == NULL) {
		KGSL_CORE_ERR("kzalloc(%d) failed\n", sizeof(*info));
		ret = -ENOMEM;
		goto err;
	}

	for_each_child_of_node(node, child)
		count++;

	info->power_param.num_freq = count;

	info->freq_tbl = kzalloc(info->power_param.num_freq *
			sizeof(struct msm_dcvs_freq_entry),
			GFP_KERNEL);

	if (info->freq_tbl == NULL) {
		KGSL_CORE_ERR("kzalloc(%d) failed\n",
			info->power_param.num_freq *
			sizeof(struct msm_dcvs_freq_entry));
		ret = -ENOMEM;
		goto err;
	}

	for_each_child_of_node(node, child) {
		unsigned int index;

		if (adreno_of_read_property(child, "reg", &index))
			goto err;

		if (index >= info->power_param.num_freq) {
			KGSL_CORE_ERR("DCVS freq entry %d is out of range\n",
				index);
			continue;
		}

		if (adreno_of_read_property(child, "qcom,freq",
			&info->freq_tbl[index].freq))
			goto err;

		if (adreno_of_read_property(child, "qcom,voltage",
			&info->freq_tbl[index].voltage))
			info->freq_tbl[index].voltage = 0;

		if (adreno_of_read_property(child, "qcom,is_trans_level",
			&info->freq_tbl[index].is_trans_level))
			info->freq_tbl[index].is_trans_level = 0;

		if (adreno_of_read_property(child, "qcom,active-energy-offset",
			&info->freq_tbl[index].active_energy_offset))
			info->freq_tbl[index].active_energy_offset = 0;

		if (adreno_of_read_property(child, "qcom,leakage-energy-offset",
			&info->freq_tbl[index].leakage_energy_offset))
			info->freq_tbl[index].leakage_energy_offset = 0;
	}

	if (adreno_of_read_property(node, "qcom,num-cores", &info->num_cores))
		goto err;

	info->sensors = kzalloc(info->num_cores *
			sizeof(int),
			GFP_KERNEL);

	for (count = 0; count < info->num_cores; count++) {
		if (adreno_of_read_property(node, "qcom,sensors",
			&(info->sensors[count])))
			goto err;
	}

	if (adreno_of_read_property(node, "qcom,core-core-type",
		&info->core_param.core_type))
		goto err;

	if (adreno_of_read_property(node, "qcom,algo-disable-pc-threshold",
		&info->algo_param.disable_pc_threshold))
		goto err;
	if (adreno_of_read_property(node, "qcom,algo-em-win-size-min-us",
		&info->algo_param.em_win_size_min_us))
		goto err;
	if (adreno_of_read_property(node, "qcom,algo-em-win-size-max-us",
		&info->algo_param.em_win_size_max_us))
		goto err;
	if (adreno_of_read_property(node, "qcom,algo-em-max-util-pct",
		&info->algo_param.em_max_util_pct))
		goto err;
	if (adreno_of_read_property(node, "qcom,algo-group-id",
		&info->algo_param.group_id))
		goto err;
	if (adreno_of_read_property(node, "qcom,algo-max-freq-chg-time-us",
		&info->algo_param.max_freq_chg_time_us))
		goto err;
	if (adreno_of_read_property(node, "qcom,algo-slack-mode-dynamic",
		&info->algo_param.slack_mode_dynamic))
		goto err;
	if (adreno_of_read_property(node, "qcom,algo-slack-weight-thresh-pct",
		&info->algo_param.slack_weight_thresh_pct))
		goto err;
	if (adreno_of_read_property(node, "qcom,algo-slack-time-min-us",
		&info->algo_param.slack_time_min_us))
		goto err;
	if (adreno_of_read_property(node, "qcom,algo-slack-time-max-us",
		&info->algo_param.slack_time_max_us))
		goto err;
	if (adreno_of_read_property(node, "qcom,algo-ss-win-size-min-us",
		&info->algo_param.ss_win_size_min_us))
		goto err;
	if (adreno_of_read_property(node, "qcom,algo-ss-win-size-max-us",
		&info->algo_param.ss_win_size_max_us))
		goto err;
	if (adreno_of_read_property(node, "qcom,algo-ss-util-pct",
		&info->algo_param.ss_util_pct))
		goto err;
	if (adreno_of_read_property(node, "qcom,algo-ss-no-corr-below-freq",
		&info->algo_param.ss_no_corr_below_freq))
		goto err;

	if (adreno_of_read_property(node, "qcom,energy-active-coeff-a",
		&info->energy_coeffs.active_coeff_a))
		goto err;
	if (adreno_of_read_property(node, "qcom,energy-active-coeff-b",
		&info->energy_coeffs.active_coeff_b))
		goto err;
	if (adreno_of_read_property(node, "qcom,energy-active-coeff-c",
		&info->energy_coeffs.active_coeff_c))
		goto err;
	if (adreno_of_read_property(node, "qcom,energy-leakage-coeff-a",
		&info->energy_coeffs.leakage_coeff_a))
		goto err;
	if (adreno_of_read_property(node, "qcom,energy-leakage-coeff-b",
		&info->energy_coeffs.leakage_coeff_b))
		goto err;
	if (adreno_of_read_property(node, "qcom,energy-leakage-coeff-c",
		&info->energy_coeffs.leakage_coeff_c))
		goto err;
	if (adreno_of_read_property(node, "qcom,energy-leakage-coeff-d",
		&info->energy_coeffs.leakage_coeff_d))
		goto err;

	if (adreno_of_read_property(node, "qcom,power-current-temp",
		&info->power_param.current_temp))
		goto err;

	return info;

err:
	if (info)
		kfree(info->freq_tbl);

	kfree(info);

	return ERR_PTR(ret);
}

static int adreno_of_get_iommu(struct device_node *parent,
	struct kgsl_device_platform_data *pdata)
{
	struct device_node *node, *child;
	struct kgsl_device_iommu_data *data = NULL;
	struct kgsl_iommu_ctx *ctxs = NULL;
	u32 reg_val[2];
	int ctx_index = 0;

	node = of_parse_phandle(parent, "iommu", 0);
	if (node == NULL)
		return -EINVAL;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (data == NULL) {
		KGSL_CORE_ERR("kzalloc(%d) failed\n", sizeof(*data));
		goto err;
	}

	if (of_property_read_u32_array(node, "reg", reg_val, 2))
		goto err;

	data->physstart = reg_val[0];
	data->physend = data->physstart + reg_val[1] - 1;

	data->iommu_ctx_count = 0;

	for_each_child_of_node(node, child)
		data->iommu_ctx_count++;

	ctxs = kzalloc(data->iommu_ctx_count * sizeof(struct kgsl_iommu_ctx),
		GFP_KERNEL);

	if (ctxs == NULL) {
		KGSL_CORE_ERR("kzalloc(%d) failed\n",
			data->iommu_ctx_count * sizeof(struct kgsl_iommu_ctx));
		goto err;
	}

	for_each_child_of_node(node, child) {
		int ret = of_property_read_string(child, "label",
				&ctxs[ctx_index].iommu_ctx_name);

		if (ret) {
			KGSL_CORE_ERR("Unable to read KGSL IOMMU 'label'\n");
			goto err;
		}

		if (adreno_of_read_property(child, "qcom,iommu-ctx-sids",
			&ctxs[ctx_index].ctx_id))
			goto err;

		ctx_index++;
	}

	data->iommu_ctxs = ctxs;

	pdata->iommu_data = data;
	pdata->iommu_count = 1;

	return 0;

err:
	kfree(ctxs);
	kfree(data);

	return -EINVAL;
}

static int adreno_of_get_pdata(struct platform_device *pdev)
{
	struct kgsl_device_platform_data *pdata = NULL;
	struct kgsl_device *device;
	int ret = -EINVAL;

	pdev->id_entry = adreno_id_table;

	pdata = pdev->dev.platform_data;
	if (pdata)
		return 0;

	if (of_property_read_string(pdev->dev.of_node, "label", &pdev->name)) {
		KGSL_CORE_ERR("Unable to read 'label'\n");
		goto err;
	}

	if (adreno_of_read_property(pdev->dev.of_node, "qcom,id", &pdev->id))
		goto err;

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (pdata == NULL) {
		KGSL_CORE_ERR("kzalloc(%d) failed\n", sizeof(*pdata));
		ret = -ENOMEM;
		goto err;
	}

	if (adreno_of_read_property(pdev->dev.of_node, "qcom,chipid",
		&pdata->chipid))
		goto err;

	
	ret = adreno_of_get_pwrlevels(pdev->dev.of_node, pdata);
	if (ret)
		goto err;

	
	if (adreno_of_read_property(pdev->dev.of_node, "qcom,idle-timeout",
		&pdata->idle_timeout))
		pdata->idle_timeout = 83;

	if (adreno_of_read_property(pdev->dev.of_node, "qcom,nap-allowed",
		&pdata->nap_allowed))
		pdata->nap_allowed = 1;

	if (adreno_of_read_property(pdev->dev.of_node, "qcom,clk-map",
		&pdata->clk_map))
		goto err;

	device = (struct kgsl_device *)pdev->id_entry->driver_data;

	if (device->id != KGSL_DEVICE_3D0)
		goto err;

	

	pdata->bus_scale_table = msm_bus_cl_get_pdata(pdev);
	if (IS_ERR_OR_NULL(pdata->bus_scale_table)) {
		ret = PTR_ERR(pdata->bus_scale_table);
		goto err;
	}

	pdata->core_info = adreno_of_get_dcvs(pdev->dev.of_node);
	if (IS_ERR_OR_NULL(pdata->core_info)) {
		ret = PTR_ERR(pdata->core_info);
		goto err;
	}

	ret = adreno_of_get_iommu(pdev->dev.of_node, pdata);
	if (ret)
		goto err;

	pdev->dev.platform_data = pdata;
	return 0;

err:
	if (pdata) {
		if (pdata->core_info)
			kfree(pdata->core_info->freq_tbl);
		kfree(pdata->core_info);

		if (pdata->iommu_data)
			kfree(pdata->iommu_data->iommu_ctxs);

		kfree(pdata->iommu_data);
	}

	kfree(pdata);

	return ret;
}
#endif

#ifdef CONFIG_MSM_OCMEM
static int
adreno_ocmem_gmem_malloc(struct adreno_device *adreno_dev)
{
	if (!adreno_is_a330(adreno_dev))
		return 0;

	
	if (adreno_dev->ocmem_hdl != NULL)
		return 0;

	adreno_dev->ocmem_hdl =
		ocmem_allocate(OCMEM_GRAPHICS, adreno_dev->gmem_size);
	if (adreno_dev->ocmem_hdl == NULL)
		return -ENOMEM;

	adreno_dev->gmem_size = adreno_dev->ocmem_hdl->len;
	adreno_dev->ocmem_base = adreno_dev->ocmem_hdl->addr;

	return 0;
}

static void
adreno_ocmem_gmem_free(struct adreno_device *adreno_dev)
{
	if (!adreno_is_a330(adreno_dev))
		return;

	if (adreno_dev->ocmem_hdl == NULL)
		return;

	ocmem_free(OCMEM_GRAPHICS, adreno_dev->ocmem_hdl);
	adreno_dev->ocmem_hdl = NULL;
}
#else
static int
adreno_ocmem_gmem_malloc(struct adreno_device *adreno_dev)
{
	return 0;
}

static void
adreno_ocmem_gmem_free(struct adreno_device *adreno_dev)
{
}
#endif

static int __devinit
adreno_probe(struct platform_device *pdev)
{
	struct kgsl_device *device;
	struct adreno_device *adreno_dev;
	int status = -EINVAL;
#if 0
	bool is_dt;

	is_dt = of_match_device(adreno_match_table, &pdev->dev);

	if (is_dt && pdev->dev.of_node) {
		status = adreno_of_get_pdata(pdev);
		if (status)
			goto error_return;
	}
#endif

	device = (struct kgsl_device *)pdev->id_entry->driver_data;
	adreno_dev = ADRENO_DEVICE(device);
	device->parentdev = &pdev->dev;

	status = adreno_ringbuffer_init(device);
	if (status != 0)
		goto error;

	status = kgsl_device_platform_probe(device);
	if (status)
		goto error_close_rb;

	adreno_debugfs_init(device);

	adreno_ft_init_sysfs(device);

	kgsl_pwrscale_init(device);
	kgsl_pwrscale_attach_policy(device, ADRENO_DEFAULT_PWRSCALE_POLICY);

	device->flags &= ~KGSL_FLAGS_SOFT_RESET;
	return 0;

error_close_rb:
	adreno_ringbuffer_close(&adreno_dev->ringbuffer);
error:
	device->parentdev = NULL;
	return status;
}

static int __devexit adreno_remove(struct platform_device *pdev)
{
	struct kgsl_device *device;
	struct adreno_device *adreno_dev;

	device = (struct kgsl_device *)pdev->id_entry->driver_data;
	adreno_dev = ADRENO_DEVICE(device);

	kgsl_pwrscale_detach_policy(device);
	kgsl_pwrscale_close(device);

	adreno_ringbuffer_close(&adreno_dev->ringbuffer);
	kgsl_device_platform_remove(device);

	return 0;
}

static int adreno_init(struct kgsl_device *device)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_ringbuffer *rb = &adreno_dev->ringbuffer;

	if (KGSL_STATE_DUMP_AND_FT != device->state)
		kgsl_pwrctrl_set_state(device, KGSL_STATE_INIT);

	
	kgsl_pwrctrl_enable(device);

	
	adreno_identify_gpu(adreno_dev);

	if (adreno_ringbuffer_read_pm4_ucode(device)) {
		KGSL_DRV_ERR(device, "Reading pm4 microcode failed %s\n",
			adreno_dev->pm4_fwfile);
		BUG_ON(1);
	}

	if (adreno_ringbuffer_read_pfp_ucode(device)) {
		KGSL_DRV_ERR(device, "Reading pfp microcode failed %s\n",
			adreno_dev->pfp_fwfile);
		BUG_ON(1);
	}

	if (adreno_dev->gpurev == ADRENO_REV_UNKNOWN) {
		KGSL_DRV_ERR(device, "Unknown chip ID %x\n",
			adreno_dev->chip_id);
		BUG_ON(1);
	}


	if ((adreno_dev->pm4_fw_version >=
		adreno_gpulist[adreno_dev->gpulist_index].sync_lock_pm4_ver) &&
		(adreno_dev->pfp_fw_version >=
		adreno_gpulist[adreno_dev->gpulist_index].sync_lock_pfp_ver))
		device->mmu.flags |= KGSL_MMU_FLAGS_IOMMU_SYNC;

	rb->global_ts = 0;

	ft_detect_regs[0] = adreno_dev->gpudev->reg_rbbm_status;

	if (!adreno_is_a2xx(adreno_dev))
		adreno_perfcounter_init(device);

	
	kgsl_pwrctrl_disable(device);

	return 0;
}

static int adreno_start(struct kgsl_device *device)
{
	int status = -EINVAL;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	unsigned int state = device->state;

	kgsl_cffdump_open(device);

	if (KGSL_STATE_DUMP_AND_FT != device->state)
		kgsl_pwrctrl_set_state(device, KGSL_STATE_INIT);

	
	kgsl_pwrctrl_enable(device);

	

	
	set_bit(ADRENO_DEVICE_PWRON, &adreno_dev->priv);

	
	if (adreno_is_a2xx(adreno_dev)) {
		if (adreno_is_a20x(adreno_dev)) {
			device->mh.mh_intf_cfg1 = 0;
			device->mh.mh_intf_cfg2 = 0;
		}

		kgsl_mh_start(device);
	}

	ft_detect_regs[0] = adreno_dev->gpudev->reg_rbbm_status;

	
	if (adreno_is_a3xx(adreno_dev)) {
		ft_detect_regs[6] = A3XX_RBBM_PERFCTR_SP_7_LO;
		ft_detect_regs[7] = A3XX_RBBM_PERFCTR_SP_7_HI;
		ft_detect_regs[8] = A3XX_RBBM_PERFCTR_SP_6_LO;
		ft_detect_regs[9] = A3XX_RBBM_PERFCTR_SP_6_HI;
		ft_detect_regs[10] = A3XX_RBBM_PERFCTR_SP_5_LO;
		ft_detect_regs[11] = A3XX_RBBM_PERFCTR_SP_5_HI;
	}

	status = kgsl_mmu_start(device);
	if (status)
		goto error_clk_off;

	status = adreno_ocmem_gmem_malloc(adreno_dev);
	if (status) {
		KGSL_DRV_ERR(device, "OCMEM malloc failed\n");
		goto error_mmu_off;
	}

	
	adreno_dev->gpudev->start(adreno_dev);

	kgsl_pwrctrl_irq(device, KGSL_PWRFLAGS_ON);
	device->ftbl->irqctrl(device, 1);

	status = adreno_ringbuffer_start(&adreno_dev->ringbuffer);
	if (status)
		goto error_irq_off;


	if (KGSL_STATE_DUMP_AND_FT != device->state)
		mod_timer(&device->idle_timer, jiffies + FIRST_TIMEOUT);

	if (!adreno_is_a2xx(adreno_dev))
		adreno_perfcounter_start(adreno_dev);
	else {
		unsigned int reg;

		kgsl_regread(device, REG_RBBM_PM_OVERRIDE2, &reg);
		kgsl_regwrite(device, REG_RBBM_PM_OVERRIDE2, (reg | 0x40));

		kgsl_regwrite(device, REG_SQ_PERFCOUNTER3_SELECT, 0x85);

		kgsl_regwrite(device, REG_CP_PERFMON_CNTL,
			REG_PERF_MODE_CNT | REG_PERF_STATE_ENABLE);

		ft_detect_regs[6] = REG_SQ_PERFCOUNTER3_LO;
		ft_detect_regs[7] = REG_SQ_PERFCOUNTER3_HI;
	}

        mod_timer(&device->hang_timer,
		(jiffies + msecs_to_jiffies(KGSL_TIMEOUT_PART)));

	device->reset_counter++;

	return 0;

error_irq_off:
	kgsl_pwrctrl_irq(device, KGSL_PWRFLAGS_OFF);

error_mmu_off:
	kgsl_mmu_stop(&device->mmu);

error_clk_off:
	if (KGSL_STATE_DUMP_AND_FT != device->state) {
		kgsl_pwrctrl_disable(device);
		
		kgsl_pwrctrl_set_state(device, state);
	}

	return status;
}

static int adreno_stop(struct kgsl_device *device)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

	adreno_dev->drawctxt_active = NULL;

	adreno_ringbuffer_stop(&adreno_dev->ringbuffer);

	kgsl_mmu_stop(&device->mmu);

	device->ftbl->irqctrl(device, 0);
	kgsl_pwrctrl_irq(device, KGSL_PWRFLAGS_OFF);
	del_timer_sync(&device->idle_timer);
	del_timer_sync(&device->hang_timer);

	adreno_ocmem_gmem_free(adreno_dev);

	
	kgsl_pwrctrl_disable(device);

	kgsl_cffdump_close(device->id);

	return 0;
}


static int _mark_context_status(int id, void *ptr, void *data)
{
	unsigned int ft_status = *((unsigned int *) data);
	struct kgsl_context *context = ptr;
	struct adreno_context *adreno_context = context->devctxt;

	if (ft_status) {
		context->reset_status =
				KGSL_CTX_STAT_GUILTY_CONTEXT_RESET_EXT;
		adreno_context->flags |= CTXT_FLAGS_GPU_HANG;
	} else if (KGSL_CTX_STAT_GUILTY_CONTEXT_RESET_EXT !=
		context->reset_status) {
		if (adreno_context->flags & (CTXT_FLAGS_GPU_HANG |
			CTXT_FLAGS_GPU_HANG_FT))
			context->reset_status =
			KGSL_CTX_STAT_GUILTY_CONTEXT_RESET_EXT;
		else
			context->reset_status =
			KGSL_CTX_STAT_INNOCENT_CONTEXT_RESET_EXT;
	}

	return 0;
}

static void adreno_mark_context_status(struct kgsl_device *device,
					int ft_status)
{
	

	read_lock(&device->context_lock);
	idr_for_each(&device->context_idr, _mark_context_status, &ft_status);
	read_unlock(&device->context_lock);
}


static int _set_max_ts(int id, void *ptr, void *data)
{
	struct kgsl_device *device = data;
	struct kgsl_context *context = ptr;
	struct adreno_context *drawctxt = context->devctxt;

	if (drawctxt->flags & CTXT_FLAGS_GPU_HANG) {
		kgsl_sharedmem_writel(&device->memstore,
			KGSL_MEMSTORE_OFFSET(context->id,
			soptimestamp),
			drawctxt->timestamp);
		kgsl_sharedmem_writel(&device->memstore,
			KGSL_MEMSTORE_OFFSET(context->id,
			eoptimestamp),
			drawctxt->timestamp);
	}

	return 0;
}

static void adreno_set_max_ts_for_bad_ctxs(struct kgsl_device *device)
{
	read_lock(&device->context_lock);
	idr_for_each(&device->context_idr, _set_max_ts, device);
	read_unlock(&device->context_lock);
}

static void adreno_destroy_ft_data(struct adreno_ft_data *ft_data)
{
	vfree(ft_data->rb_buffer);
	vfree(ft_data->bad_rb_buffer);
	vfree(ft_data->good_rb_buffer);
}

static int _find_start_of_cmd_seq(struct adreno_ringbuffer *rb,
					unsigned int *ptr,
					bool inc)
{
	int status = -EINVAL;
	unsigned int val1;
	unsigned int size = rb->buffer_desc.size;
	unsigned int start_ptr = *ptr;

	while ((start_ptr / sizeof(unsigned int)) != rb->wptr) {
		if (inc)
			start_ptr = adreno_ringbuffer_inc_wrapped(start_ptr,
									size);
		else
			start_ptr = adreno_ringbuffer_dec_wrapped(start_ptr,
									size);
		kgsl_sharedmem_readl(&rb->buffer_desc, &val1, start_ptr);
		
		rmb();
		if (KGSL_CMD_IDENTIFIER == val1) {
			if ((start_ptr / sizeof(unsigned int)) != rb->wptr)
				start_ptr = adreno_ringbuffer_dec_wrapped(
							start_ptr, size);
				*ptr = start_ptr;
				status = 0;
				break;
		}
	}
	return status;
}

static int _find_cmd_seq_after_eop_ts(struct adreno_ringbuffer *rb,
					unsigned int *rb_rptr,
					unsigned int global_eop,
					bool inc)
{
	int status = -EINVAL;
	unsigned int temp_rb_rptr = *rb_rptr;
	unsigned int size = rb->buffer_desc.size;
	unsigned int val[3];
	int i = 0;
	bool check = false;

	if (inc && temp_rb_rptr / sizeof(unsigned int) != rb->wptr)
		return status;

	do {
		if (!inc)
			temp_rb_rptr = adreno_ringbuffer_dec_wrapped(
					temp_rb_rptr, size);
		kgsl_sharedmem_readl(&rb->buffer_desc, &val[i],
					temp_rb_rptr);
		
		rmb();

		if (check && ((inc && val[i] == global_eop) ||
			(!inc && (val[i] ==
			cp_type3_packet(CP_MEM_WRITE, 2) ||
			val[i] == CACHE_FLUSH_TS)))) {
			i = (i + 2) % 3;
			if (val[i] == rb->device->memstore.gpuaddr +
				KGSL_MEMSTORE_OFFSET(KGSL_MEMSTORE_GLOBAL,
						eoptimestamp)) {
				int j = ((i + 2) % 3);
				if ((inc && (val[j] == CACHE_FLUSH_TS ||
						val[j] == cp_type3_packet(
							CP_MEM_WRITE, 2))) ||
					(!inc && val[j] == global_eop)) {
						
						status = 0;
						break;
				}
			}
			i = (i + 1) % 3;
		}
		if (inc)
			temp_rb_rptr = adreno_ringbuffer_inc_wrapped(
						temp_rb_rptr, size);

		i = (i + 1) % 3;
		if (2 == i)
			check = true;
	} while (temp_rb_rptr / sizeof(unsigned int) != rb->wptr);
	if (!status) {
		status = _find_start_of_cmd_seq(rb, &temp_rb_rptr, false);
		if (!status) {
			*rb_rptr = temp_rb_rptr;
			KGSL_FT_INFO(rb->device,
			"Offset of cmd sequence after eop timestamp: 0x%x\n",
			temp_rb_rptr / sizeof(unsigned int));
		}
	}
	if (status)
		KGSL_FT_ERR(rb->device,
		"Failed to find the command sequence after eop timestamp %x\n",
		global_eop);
	return status;
}

static int _find_hanging_ib_sequence(struct adreno_ringbuffer *rb,
				unsigned int *rb_rptr,
				unsigned int ib1)
{
	int status = -EINVAL;
	unsigned int temp_rb_rptr = *rb_rptr;
	unsigned int size = rb->buffer_desc.size;
	unsigned int val[2];
	int i = 0;
	bool check = false;
	bool ctx_switch = false;

	while (temp_rb_rptr / sizeof(unsigned int) != rb->wptr) {
		kgsl_sharedmem_readl(&rb->buffer_desc, &val[i], temp_rb_rptr);
		
		rmb();

		if (check && val[i] == ib1) {
			
			i = (i + 1) % 2;
			if (adreno_cmd_is_ib(val[i])) {
				
				status = _find_start_of_cmd_seq(rb,
						&temp_rb_rptr, false);

				KGSL_FT_INFO(rb->device,
				"Found the hanging IB at offset 0x%x\n",
				temp_rb_rptr / sizeof(unsigned int));
				break;
			}
			i = (i + 1) % 2;
		}
		if (val[i] == KGSL_CONTEXT_TO_MEM_IDENTIFIER) {
			if (ctx_switch) {
				KGSL_FT_ERR(rb->device,
				"Context switch encountered before bad "
				"IB found\n");
				break;
			}
			ctx_switch = true;
		}
		i = (i + 1) % 2;
		if (1 == i)
			check = true;
		temp_rb_rptr = adreno_ringbuffer_inc_wrapped(temp_rb_rptr,
								size);
	}
	if  (!status)
		*rb_rptr = temp_rb_rptr;
	return status;
}

static void adreno_setup_ft_data(struct kgsl_device *device,
					struct adreno_ft_data *ft_data)
{
	int ret = 0;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_ringbuffer *rb = &adreno_dev->ringbuffer;
	struct kgsl_context *context;
	struct adreno_context *adreno_context;
	unsigned int rb_rptr = rb->wptr * sizeof(unsigned int);

	memset(ft_data, 0, sizeof(*ft_data));
	ft_data->start_of_replay_cmds = 0xFFFFFFFF;
	ft_data->replay_for_snapshot = 0xFFFFFFFF;

	adreno_regread(device, REG_CP_IB1_BASE, &ft_data->ib1);

	kgsl_sharedmem_readl(&device->memstore, &ft_data->context_id,
			KGSL_MEMSTORE_OFFSET(KGSL_MEMSTORE_GLOBAL,
			current_context));

	kgsl_sharedmem_readl(&device->memstore,
			&ft_data->global_eop,
			KGSL_MEMSTORE_OFFSET(KGSL_MEMSTORE_GLOBAL,
			eoptimestamp));

	
	rmb();

	ft_data->rb_buffer = vmalloc(rb->buffer_desc.size);
	if (!ft_data->rb_buffer) {
		KGSL_MEM_ERR(device, "vmalloc(%d) failed\n",
				rb->buffer_desc.size);
		return;
	}

	ft_data->bad_rb_buffer = vmalloc(rb->buffer_desc.size);
	if (!ft_data->bad_rb_buffer) {
		KGSL_MEM_ERR(device, "vmalloc(%d) failed\n",
				rb->buffer_desc.size);
		return;
	}

	ft_data->good_rb_buffer = vmalloc(rb->buffer_desc.size);
	if (!ft_data->good_rb_buffer) {
		KGSL_MEM_ERR(device, "vmalloc(%d) failed\n",
				rb->buffer_desc.size);
		return;
	}

	ft_data->status = 0;

	
	context = kgsl_context_get(device, ft_data->context_id);

	ft_data->ft_policy = adreno_dev->ft_policy;

	if (!adreno_dev->ft_policy)
		adreno_dev->ft_policy = KGSL_FT_DEFAULT_POLICY;

	
	ret = _find_cmd_seq_after_eop_ts(rb, &rb_rptr,
					ft_data->global_eop + 1, false);
	if (ret) {
		ft_data->ft_policy |= KGSL_FT_TEMP_DISABLE;
		goto done;
	} else {
		ft_data->start_of_replay_cmds = rb_rptr;
		ft_data->ft_policy &= ~KGSL_FT_TEMP_DISABLE;
	}

	if (context) {
		adreno_context = context->devctxt;
		if (adreno_context->flags & CTXT_FLAGS_PREAMBLE) {
			if (ft_data->ib1) {
				ret = _find_hanging_ib_sequence(rb,
						&rb_rptr, ft_data->ib1);
				if (ret) {
					KGSL_FT_ERR(device,
					"Start not found for replay IB seq\n");
					goto done;
				}
				ft_data->start_of_replay_cmds = rb_rptr;
				ft_data->replay_for_snapshot = rb_rptr;
			}
		}
	}

done:
	kgsl_context_put(context);
}

static int
_adreno_check_long_ib(struct kgsl_device *device)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	unsigned int curr_global_ts = 0;

	
	kgsl_sharedmem_readl(&device->memstore,
			&curr_global_ts,
			KGSL_MEMSTORE_OFFSET(KGSL_MEMSTORE_GLOBAL,
			eoptimestamp));
	
	rmb();

	
	adreno_dev->long_ib = 0;

	if (curr_global_ts == adreno_dev->long_ib_ts) {
		KGSL_FT_ERR(device,
			"IB ran too long, invalidate ctxt\n");
		return 1;
	} else {
		
		KGSL_FT_INFO(device, "false long ib detection return\n");
		return 0;
	}
}

static int
_adreno_ft_restart_device(struct kgsl_device *device,
		   struct kgsl_context *context)
{
	struct adreno_context *adreno_context = NULL;

	
	if (adreno_stop(device)) {
		KGSL_FT_ERR(device, "Device stop failed\n");
		return 1;
	}
	
	if (adreno_init(device)) {
		KGSL_FT_ERR(device, "Device start failed\n");
		return 1;
	}

	if (adreno_start(device)) {
		KGSL_FT_ERR(device, "Device start failed\n");
		return 1;
	}

	if ((context != NULL) && (context->devctxt != NULL)) {
		adreno_context = context->devctxt;
		kgsl_mmu_setstate(&device->mmu, adreno_context->pagetable,
			KGSL_MEMSTORE_GLOBAL);
	}

	if (KGSL_MMU_TYPE_IOMMU == kgsl_mmu_get_mmutype()) {
		if (kgsl_mmu_enable_clk(&device->mmu,
				KGSL_IOMMU_CONTEXT_USER))
			return 1;
	}

	return 0;
}

static inline void
_adreno_debug_ft_info(struct kgsl_device *device,
			struct adreno_ft_data *ft_data)
{

	if (device->ft_log >= 7)  {

		
		KGSL_FT_INFO(device, "Temp RB buffer size 0x%X\n",
			ft_data->rb_size);
		adreno_dump_rb(device, ft_data->rb_buffer,
			ft_data->rb_size<<2, 0, ft_data->rb_size);

		KGSL_FT_INFO(device, "Bad RB buffer size 0x%X\n",
			ft_data->bad_rb_size);
		adreno_dump_rb(device, ft_data->bad_rb_buffer,
			ft_data->bad_rb_size<<2, 0, ft_data->bad_rb_size);

		KGSL_FT_INFO(device, "Good RB buffer size 0x%X\n",
			ft_data->good_rb_size);
		adreno_dump_rb(device, ft_data->good_rb_buffer,
			ft_data->good_rb_size<<2, 0, ft_data->good_rb_size);

	}
}

static int
_adreno_ft_resubmit_rb(struct kgsl_device *device,
			struct adreno_ringbuffer *rb,
			struct kgsl_context *context,
			struct adreno_ft_data *ft_data,
			unsigned int *buff, unsigned int size)
{
	unsigned int ret = 0;
	unsigned int retry_num = 0;

	_adreno_debug_ft_info(device, ft_data);

	do {
		ret = _adreno_ft_restart_device(device, context);
		if (ret == 0)
			break;
		msleep(20);
		KGSL_FT_ERR(device, "Retry device restart %d\n", retry_num);
		retry_num++;
	} while (retry_num < 4);

	if (ret) {
		KGSL_FT_ERR(device, "Device restart failed\n");
		BUG_ON(1);
		goto done;
	}

	if (size) {

		
		adreno_ringbuffer_restore(rb, buff, size);

		ret = adreno_idle(device);
	}

done:
	return ret;
}

static int
_adreno_ft(struct kgsl_device *device,
			struct adreno_ft_data *ft_data)
{
	int ret = 0, i;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_ringbuffer *rb = &adreno_dev->ringbuffer;
	struct kgsl_context *context;
	struct adreno_context *adreno_context = NULL;
	struct adreno_context *last_active_ctx = adreno_dev->drawctxt_active;
	unsigned int long_ib = 0;
	static int no_context_ft;

	context = kgsl_context_get(device, ft_data->context_id);

	if (context == NULL) {
		KGSL_FT_ERR(device, "Last context unknown id:%d\n",
			ft_data->context_id);
		if (no_context_ft) {
			no_context_ft = 0;
			goto play_good_cmds;
		}
	} else {
		no_context_ft = 0;
		adreno_context = context->devctxt;
		adreno_context->flags |= CTXT_FLAGS_GPU_HANG;
		context->wait_on_invalid_ts = false;

		if (!(adreno_context->flags & CTXT_FLAGS_PER_CONTEXT_TS)) {
			ft_data->status = 1;
			KGSL_FT_ERR(device, "Fault tolerance not supported\n");
			goto play_good_cmds;
		}

		if (adreno_context->flags & CTXT_FLAGS_NO_FAULT_TOLERANCE) {
			ft_data->status = 1;
			KGSL_FT_ERR(device,
			"No FT set for this context play good cmds\n");
			goto play_good_cmds;
		}

	}

	if ((adreno_context) && (adreno_dev->long_ib)) {
		long_ib = _adreno_check_long_ib(device);
		if (!long_ib) {
			adreno_context->flags &= ~CTXT_FLAGS_GPU_HANG;
			return 0;
		}
	}

	adreno_ringbuffer_extract(rb, ft_data);

	
	if (long_ib) {
		ft_data->status = 1;
		_adreno_debug_ft_info(device, ft_data);
		goto play_good_cmds;
	}

	if ((ft_data->ft_policy & KGSL_FT_DISABLE) ||
		(ft_data->ft_policy & KGSL_FT_TEMP_DISABLE)) {
		KGSL_FT_ERR(device, "NO FT policy play only good cmds\n");
		ft_data->status = 1;
		goto play_good_cmds;
	}

	
	if (adreno_context && adreno_context->pagefault) {
		if ((ft_data->context_id == adreno_context->id) &&
			(ft_data->global_eop == adreno_context->pagefault_ts)) {
			ft_data->ft_policy &= ~KGSL_FT_REPLAY;
			KGSL_FT_ERR(device, "MMU fault skipping replay\n");
		}

		adreno_context->pagefault = 0;
	}

	if (ft_data->ft_policy & KGSL_FT_REPLAY) {
		ret = _adreno_ft_resubmit_rb(device, rb, context, ft_data,
				ft_data->bad_rb_buffer, ft_data->bad_rb_size);

		if (ret) {
			KGSL_FT_ERR(device, "Replay status: 1\n");
			ft_data->status = 1;
		} else
			goto play_good_cmds;
	}

	if (ft_data->ft_policy & KGSL_FT_SKIPIB) {
		for (i = 0; i < ft_data->bad_rb_size; i++) {
			if ((ft_data->bad_rb_buffer[i] ==
					CP_HDR_INDIRECT_BUFFER_PFD) &&
				(ft_data->bad_rb_buffer[i+1] == ft_data->ib1)) {

				ft_data->bad_rb_buffer[i] = cp_nop_packet(2);
				ft_data->bad_rb_buffer[i+1] =
							KGSL_NOP_IB_IDENTIFIER;
				ft_data->bad_rb_buffer[i+2] =
							KGSL_NOP_IB_IDENTIFIER;
				break;
			}
		}

		if ((i == (ft_data->bad_rb_size)) || (!ft_data->ib1)) {
			KGSL_FT_ERR(device, "Bad IB to NOP not found\n");
			ft_data->status = 1;
			goto play_good_cmds;
		}

		ret = _adreno_ft_resubmit_rb(device, rb, context, ft_data,
				ft_data->bad_rb_buffer, ft_data->bad_rb_size);

		if (ret) {
			KGSL_FT_ERR(device, "NOP faulty IB status: 1\n");
			ft_data->status = 1;
		} else {
			ft_data->status = 0;
			goto play_good_cmds;
		}
	}

	if (ft_data->ft_policy & KGSL_FT_SKIPFRAME) {
		for (i = 0; i < ft_data->bad_rb_size; i++) {
			if (ft_data->bad_rb_buffer[i] ==
					KGSL_END_OF_FRAME_IDENTIFIER) {
				ft_data->bad_rb_buffer[0] = cp_nop_packet(i);
				break;
			}
		}

		if (adreno_context && (i == ft_data->bad_rb_size)) {
			adreno_context->flags |= CTXT_FLAGS_SKIP_EOF;
			KGSL_FT_INFO(device,
			"EOF not found in RB, skip next issueib till EOF\n");
			ft_data->bad_rb_buffer[0] = cp_nop_packet(i);
		}

		ret = _adreno_ft_resubmit_rb(device, rb, context, ft_data,
				ft_data->bad_rb_buffer, ft_data->bad_rb_size);

		if (ret) {
			KGSL_FT_ERR(device, "Skip EOF status: 1\n");
			ft_data->status = 1;
		} else {
			ft_data->status = 0;
			goto play_good_cmds;
		}
	}

play_good_cmds:

	if (ft_data->status)
		KGSL_FT_ERR(device, "Bad context commands failed\n");
	else {
		KGSL_FT_INFO(device, "Bad context commands success\n");

		if (adreno_context) {
			adreno_context->flags = (adreno_context->flags &
				~CTXT_FLAGS_GPU_HANG) | CTXT_FLAGS_GPU_HANG_FT;
		}
		adreno_dev->drawctxt_active = last_active_ctx;
	}

	ret = _adreno_ft_resubmit_rb(device, rb, context, ft_data,
			ft_data->good_rb_buffer, ft_data->good_rb_size);

	if (ret) {
		if (!context)
			no_context_ft = 1;
		ret = -EAGAIN;
		KGSL_FT_ERR(device, "Playing good commands unsuccessful\n");
		goto done;
	} else
		KGSL_FT_INFO(device, "Playing good commands successful\n");

	if (ft_data->last_valid_ctx_id) {
		struct kgsl_context *last_ctx = kgsl_context_get(device,
			ft_data->last_valid_ctx_id);

		if (last_ctx)
			adreno_dev->drawctxt_active = last_ctx->devctxt;

		kgsl_context_put(last_ctx);
	}

done:
	
	if (KGSL_MMU_TYPE_IOMMU == kgsl_mmu_get_mmutype())
		kgsl_mmu_disable_clk_on_ts(&device->mmu, 0, false);

	kgsl_context_put(context);
	return ret;
}

static int
adreno_ft(struct kgsl_device *device,
			struct adreno_ft_data *ft_data)
{
	int ret = 0;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_ringbuffer *rb = &adreno_dev->ringbuffer;

	KGSL_FT_INFO(device,
	"Start Parameters: IB1: 0x%X, "
	"Bad context_id: %u, global_eop: 0x%x\n",
	ft_data->ib1, ft_data->context_id, ft_data->global_eop);

	KGSL_FT_INFO(device, "Last issued global timestamp: %x\n",
			rb->global_ts);

	while (true) {

		ret = _adreno_ft(device, ft_data);

		if (-EAGAIN == ret) {
			adreno_destroy_ft_data(ft_data);
			adreno_setup_ft_data(device, ft_data);
			KGSL_FT_INFO(device,
			"Retry. Parameters: "
			"IB1: 0x%X, Bad context_id: %u, global_eop: 0x%x\n",
			ft_data->ib1, ft_data->context_id,
			ft_data->global_eop);
		} else {
			break;
		}
	}

	if (ret)
		goto done;

	
	if (adreno_dev->drawctxt_active)
		kgsl_mmu_setstate(&device->mmu,
			adreno_dev->drawctxt_active->pagetable,
			adreno_dev->drawctxt_active->id);
	else
		kgsl_mmu_setstate(&device->mmu,
			device->mmu.defaultpagetable, KGSL_MEMSTORE_GLOBAL);
	kgsl_sharedmem_writel(&device->memstore,
			KGSL_MEMSTORE_OFFSET(KGSL_MEMSTORE_GLOBAL,
			eoptimestamp), rb->global_ts);

	
	if (adreno_dev->drawctxt_active != NULL)
		adreno_drawctxt_switch(adreno_dev, NULL, 0);
#ifdef CONFIG_MSM_KGSL_GPU_USAGE
		device->current_process_priv = NULL;
#endif

done:
	adreno_set_max_ts_for_bad_ctxs(device);
	adreno_mark_context_status(device, ret);
	KGSL_FT_ERR(device, "policy 0x%X status 0x%x\n",
			ft_data->ft_policy, ret);
	return ret;
}

static int adreno_kill_suspect(struct kgsl_device *device, int pid)
{
	int ret = 1;
#ifdef CONFIG_MSM_KGSL_KILL_HANG_PROCESS
	int cankill = 1;
	char suspect_task_comm[TASK_COMM_LEN+1];
	char suspect_task_parent_comm[TASK_COMM_LEN+1];
	int suspect_tgid;
	struct task_struct *suspect_task = find_task_by_pid_ns(pid, &init_pid_ns);
	struct task_struct *suspect_parent_task;
	int i = 0;

	if (suspect_task) {
		suspect_parent_task = suspect_task->group_leader;
	} else {
		KGSL_DRV_ERR(device, "Cannot find task! suspect_task is NULL!\n");
		return -EINVAL;
	}

	suspect_tgid = task_tgid_nr(suspect_task);
	get_task_comm(suspect_task_comm, suspect_task);

	if (suspect_parent_task)
		get_task_comm(suspect_task_parent_comm, suspect_parent_task);
	else
		suspect_task_parent_comm[0] = '\0';

	

	for (i = 0; i < ARRAY_SIZE(kgsl_blocking_process_tbl); i++) {
		if (!((strncmp(suspect_task_comm,
			kgsl_blocking_process_tbl[i].name, TASK_COMM_LEN)) &&
					(strncmp(suspect_task_parent_comm,
					kgsl_blocking_process_tbl[i].name, TASK_COMM_LEN)))) {
				cankill=0;
				break;
			}
	}

	if (cankill) {
		KGSL_DRV_ERR(device, "We need to kill suspect process "
			"causing gpu hung, tgid=%d, name=%s, pname=%s\n",
			suspect_tgid, suspect_task_comm, suspect_task_parent_comm);

		do_send_sig_info(SIGKILL,
		SEND_SIG_FORCED, suspect_task, true);
		ret = 0;
	} else {
		KGSL_DRV_ERR(device, "We can't kill suspect process "
			"causing gpu hung due to stability, tgid=%d, name=%s, pname=%s\n",
			suspect_tgid, suspect_task_comm, suspect_task_parent_comm);
	}
#endif
	return ret;
}

int
adreno_dump_and_exec_ft(struct kgsl_device *device)
{
	int result = -ETIMEDOUT;
	struct adreno_ft_data ft_data;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	unsigned int curr_pwrlevel;

	struct kgsl_context *context;
	unsigned int context_id;
	pid_t gpu_hung_pid;

	if (device->state == KGSL_STATE_HUNG)
		goto done;
	if (device->state == KGSL_STATE_DUMP_AND_FT) {
		mutex_unlock(&device->mutex);
		wait_for_completion(&device->ft_gate);
		mutex_lock(&device->mutex);
		if (device->state != KGSL_STATE_HUNG)
			result = 0;
	} else {
		kgsl_pwrctrl_set_state(device, KGSL_STATE_DUMP_AND_FT);
		INIT_COMPLETION(device->ft_gate);
		

		
		kgsl_sharedmem_readl(&device->memstore,
			(unsigned int *) &context_id,
			KGSL_MEMSTORE_OFFSET(KGSL_MEMSTORE_GLOBAL,
			current_context));
		read_lock(&device->context_lock);
		context = idr_find(&device->context_idr, context_id);
		gpu_hung_pid = context->dev_priv->process_priv->pid;
		read_unlock(&device->context_lock);

		
		curr_pwrlevel = pwr->active_pwrlevel;
		kgsl_pwrctrl_pwrlevel_change(device, pwr->max_pwrlevel);

		
		adreno_setup_ft_data(device, &ft_data);

		if (!adreno_dev->long_ib) {
			kgsl_postmortem_dump(device, 0);

			kgsl_device_snapshot(device, 1);
		}

		result = adreno_ft(device, &ft_data);
		adreno_destroy_ft_data(&ft_data);

		
		kgsl_pwrctrl_pwrlevel_change(device, curr_pwrlevel);

		if (result) {
			kgsl_pwrctrl_set_state(device, KGSL_STATE_HUNG);
		} else {
			kgsl_pwrctrl_set_state(device, KGSL_STATE_ACTIVE);
			mod_timer(&device->idle_timer, jiffies + FIRST_TIMEOUT);
			mod_timer(&device->hang_timer,
				(jiffies +
				msecs_to_jiffies(KGSL_TIMEOUT_PART)));
		}
		complete_all(&device->ft_gate);
		
		if (!device->snapshot_no_panic) {
			if (result) {
				msleep(10000);
				panic("GPU Hang");
			} else {
				if (board_mfg_mode() || adreno_kill_suspect(device, gpu_hung_pid)) {
					msleep(10000);
					panic("Recoverable GPU Hang");
				}
			}
		}
	}
done:
	return result;
}
EXPORT_SYMBOL(adreno_dump_and_exec_ft);

static int _ft_sysfs_store(const char *buf, size_t count, unsigned int *ptr)
{
	char temp[20];
	unsigned long val;
	int rc;

	snprintf(temp, sizeof(temp), "%.*s",
			 (int)min(count, sizeof(temp) - 1), buf);
	rc = kstrtoul(temp, 0, &val);
	if (rc)
		return rc;

	*ptr = val;

	return count;
}

struct adreno_device *_get_adreno_dev(struct device *dev)
{
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	return device ? ADRENO_DEVICE(device) : NULL;
}

static int _ft_policy_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct adreno_device *adreno_dev = _get_adreno_dev(dev);
	int ret;
	if (adreno_dev == NULL)
		return 0;

	mutex_lock(&adreno_dev->dev.mutex);
	ret = _ft_sysfs_store(buf, count, &adreno_dev->ft_policy);
	mutex_unlock(&adreno_dev->dev.mutex);

	return ret;
}

static int _ft_policy_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct adreno_device *adreno_dev = _get_adreno_dev(dev);
	if (adreno_dev == NULL)
		return 0;
	return snprintf(buf, PAGE_SIZE, "0x%X\n", adreno_dev->ft_policy);
}

static int _ft_pagefault_policy_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct adreno_device *adreno_dev = _get_adreno_dev(dev);
	int ret;
	if (adreno_dev == NULL)
		return 0;

	mutex_lock(&adreno_dev->dev.mutex);
	ret = _ft_sysfs_store(buf, count, &adreno_dev->ft_pf_policy);
	mutex_unlock(&adreno_dev->dev.mutex);

	return ret;
}

static int _ft_pagefault_policy_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct adreno_device *adreno_dev = _get_adreno_dev(dev);
	if (adreno_dev == NULL)
		return 0;
	return snprintf(buf, PAGE_SIZE, "0x%X\n", adreno_dev->ft_pf_policy);
}

static int _ft_fast_hang_detect_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct adreno_device *adreno_dev = _get_adreno_dev(dev);
	int ret;
	if (adreno_dev == NULL)
		return 0;

	mutex_lock(&adreno_dev->dev.mutex);
	ret = _ft_sysfs_store(buf, count, &adreno_dev->fast_hang_detect);
	mutex_unlock(&adreno_dev->dev.mutex);

	return ret;

}

static int _ft_fast_hang_detect_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct adreno_device *adreno_dev = _get_adreno_dev(dev);
	if (adreno_dev == NULL)
		return 0;
	return snprintf(buf, PAGE_SIZE, "%d\n",
				(adreno_dev->fast_hang_detect ? 1 : 0));
}

static int _ft_long_ib_detect_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct adreno_device *adreno_dev = _get_adreno_dev(dev);
	int ret;
	if (adreno_dev == NULL)
		return 0;

	mutex_lock(&adreno_dev->dev.mutex);
	ret = _ft_sysfs_store(buf, count, &adreno_dev->long_ib_detect);
	mutex_unlock(&adreno_dev->dev.mutex);

	return ret;

}

static int _ft_long_ib_detect_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct adreno_device *adreno_dev = _get_adreno_dev(dev);
	if (adreno_dev == NULL)
		return 0;
	return snprintf(buf, PAGE_SIZE, "%d\n",
				(adreno_dev->long_ib_detect ? 1 : 0));
}


#define FT_DEVICE_ATTR(name) \
	DEVICE_ATTR(name, 0644,	_ ## name ## _show, _ ## name ## _store);

FT_DEVICE_ATTR(ft_policy);
FT_DEVICE_ATTR(ft_pagefault_policy);
FT_DEVICE_ATTR(ft_fast_hang_detect);
FT_DEVICE_ATTR(ft_long_ib_detect);


const struct device_attribute *ft_attr_list[] = {
	&dev_attr_ft_policy,
	&dev_attr_ft_pagefault_policy,
	&dev_attr_ft_fast_hang_detect,
	&dev_attr_ft_long_ib_detect,
	NULL,
};

int adreno_ft_init_sysfs(struct kgsl_device *device)
{
	return kgsl_create_device_sysfs_files(device->dev, ft_attr_list);
}

void adreno_ft_uninit_sysfs(struct kgsl_device *device)
{
	kgsl_remove_device_sysfs_files(device->dev, ft_attr_list);
}

static int adreno_getproperty(struct kgsl_device *device,
				enum kgsl_property_type type,
				void *value,
				unsigned int sizebytes)
{
	int status = -EINVAL;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

	switch (type) {
	case KGSL_PROP_DEVICE_INFO:
		{
			struct kgsl_devinfo devinfo;

			if (sizebytes != sizeof(devinfo)) {
				status = -EINVAL;
				break;
			}

			memset(&devinfo, 0, sizeof(devinfo));
			devinfo.device_id = device->id+1;
			devinfo.chip_id = adreno_dev->chip_id;
			devinfo.mmu_enabled = kgsl_mmu_enabled();
			devinfo.gpu_id = adreno_dev->gpurev;
			devinfo.gmem_gpubaseaddr = adreno_dev->gmem_base;
			devinfo.gmem_sizebytes = adreno_dev->gmem_size;

			if (copy_to_user(value, &devinfo, sizeof(devinfo)) !=
					0) {
				status = -EFAULT;
				break;
			}
			status = 0;
		}
		break;
	case KGSL_PROP_DEVICE_SHADOW:
		{
			struct kgsl_shadowprop shadowprop;

			if (sizebytes != sizeof(shadowprop)) {
				status = -EINVAL;
				break;
			}
			memset(&shadowprop, 0, sizeof(shadowprop));
			if (device->memstore.hostptr) {
				shadowprop.gpuaddr = device->memstore.gpuaddr;
				shadowprop.size = device->memstore.size;
				shadowprop.flags = KGSL_FLAGS_INITIALIZED |
					KGSL_FLAGS_PER_CONTEXT_TIMESTAMPS;
			}
			if (copy_to_user(value, &shadowprop,
				sizeof(shadowprop))) {
				status = -EFAULT;
				break;
			}
			status = 0;
		}
		break;
	case KGSL_PROP_MMU_ENABLE:
		{
			int mmu_prop = kgsl_mmu_enabled();

			if (sizebytes != sizeof(int)) {
				status = -EINVAL;
				break;
			}
			if (copy_to_user(value, &mmu_prop, sizeof(mmu_prop))) {
				status = -EFAULT;
				break;
			}
			status = 0;
		}
		break;
	case KGSL_PROP_INTERRUPT_WAITS:
		{
			int int_waits = 1;
			if (sizebytes != sizeof(int)) {
				status = -EINVAL;
				break;
			}
			if (copy_to_user(value, &int_waits, sizeof(int))) {
				status = -EFAULT;
				break;
			}
			status = 0;
		}
		break;
	default:
		status = -EINVAL;
	}

	return status;
}

static int adreno_setproperty(struct kgsl_device *device,
				enum kgsl_property_type type,
				void *value,
				unsigned int sizebytes)
{
	int status = -EINVAL;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

	switch (type) {
	case KGSL_PROP_PWRCTRL: {
			unsigned int enable;
			struct kgsl_device_platform_data *pdata =
				kgsl_device_get_drvdata(device);

			if (sizebytes != sizeof(enable))
				break;

			if (copy_from_user(&enable, (void __user *) value,
				sizeof(enable))) {
				status = -EFAULT;
				break;
			}

			if (enable) {
				if (pdata->nap_allowed)
					device->pwrctrl.nap_allowed = true;
				adreno_dev->fast_hang_detect = 1;
				kgsl_pwrscale_enable(device);
			} else {
				device->pwrctrl.nap_allowed = false;
				adreno_dev->fast_hang_detect = 0;
				kgsl_pwrscale_disable(device);
			}

			status = 0;
		}
		break;
	default:
		break;
	}

	return status;
}

static int adreno_ringbuffer_drain(struct kgsl_device *device,
	unsigned int *regs)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_ringbuffer *rb = &adreno_dev->ringbuffer;
	unsigned long wait;
	unsigned long timeout = jiffies + msecs_to_jiffies(ADRENO_IDLE_TIMEOUT);


	wait = jiffies + msecs_to_jiffies(100);

	do {
		if (time_after(jiffies, wait)) {
			
			if (adreno_ft_detect(device, regs))
				return -ETIMEDOUT;

			wait = jiffies + msecs_to_jiffies(KGSL_TIMEOUT_PART);
		}
		GSL_RB_GET_READPTR(rb, &rb->rptr);

		if (time_after(jiffies, timeout)) {
			KGSL_DRV_ERR(device, "rptr: %x, wptr: %x\n",
				rb->rptr, rb->wptr);
			return -ETIMEDOUT;
		}
	} while (rb->rptr != rb->wptr);

	return 0;
}

int adreno_idle(struct kgsl_device *device)
{
	unsigned long wait_time;
	unsigned long wait_time_part;
	unsigned int prev_reg_val[FT_DETECT_REGS_COUNT];


	memset(prev_reg_val, 0, sizeof(prev_reg_val));

	kgsl_cffdump_regpoll(device->id,
		adreno_dev->gpudev->reg_rbbm_status << 2,
		0x00000000, 0x80000000);

retry:
	
	if (adreno_ringbuffer_drain(device, prev_reg_val))
		goto err;

	
	wait_time = jiffies + msecs_to_jiffies(ADRENO_IDLE_TIMEOUT);
	wait_time_part = jiffies + msecs_to_jiffies(KGSL_TIMEOUT_PART);

	while (time_before(jiffies, wait_time)) {
		if (adreno_isidle(device))
			return 0;

		
		if (time_after(jiffies, wait_time_part)) {
			wait_time_part = jiffies +
				msecs_to_jiffies(KGSL_TIMEOUT_PART);
			if ((adreno_ft_detect(device, prev_reg_val)))
				goto err;
		}

	}

err:
	KGSL_DRV_ERR(device, "spun too long waiting for RB to idle\n");
	if (KGSL_STATE_DUMP_AND_FT != device->state &&
		!adreno_dump_and_exec_ft(device)) {
		wait_time = jiffies + msecs_to_jiffies(ADRENO_IDLE_TIMEOUT);
		goto retry;
	}
	return -ETIMEDOUT;
}

static bool is_adreno_rbbm_status_idle(struct kgsl_device *device)
{
	unsigned int reg_rbbm_status;
	bool status = false;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

	
	adreno_regread(device,
		adreno_dev->gpudev->reg_rbbm_status,
		&reg_rbbm_status);

	if (adreno_is_a2xx(adreno_dev)) {
		if (reg_rbbm_status == 0x110)
			status = true;
	} else {
		if (!(reg_rbbm_status & 0x80000000))
			status = true;
	}
	return status;
}

static unsigned int adreno_isidle(struct kgsl_device *device)
{
	int status = false;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_ringbuffer *rb = &adreno_dev->ringbuffer;

	
	if (kgsl_pwrctrl_isenabled(device)) {
		
		GSL_RB_GET_READPTR(rb, &rb->rptr);
		if (rb->rptr == rb->wptr) {

			if (!adreno_dev->gpudev->irq_pending(adreno_dev)) {
				
				status = is_adreno_rbbm_status_idle(device);
			}
		}
	} else {
		status = true;
	}
	return status;
}

static int adreno_suspend_context(struct kgsl_device *device)
{
	int status = 0;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

	
	if (adreno_dev->drawctxt_active != NULL) {
		adreno_drawctxt_switch(adreno_dev, NULL, 0);
		status = adreno_idle(device);
	}

	return status;
}


struct kgsl_memdesc *adreno_find_ctxtmem(struct kgsl_device *device,
	unsigned int pt_base, unsigned int gpuaddr, unsigned int size)
{
	struct kgsl_context *context;
	struct adreno_context *adreno_context = NULL;
	int next = 0;
	struct kgsl_memdesc *desc = NULL;


	read_lock(&device->context_lock);
	while (1) {
		context = idr_get_next(&device->context_idr, &next);
		if (context == NULL)
			break;

		adreno_context = (struct adreno_context *)context->devctxt;

		if (kgsl_mmu_pt_equal(&device->mmu, adreno_context->pagetable,
					pt_base)) {
			desc = &adreno_context->gpustate;
			if (kgsl_gpuaddr_in_memdesc(desc, gpuaddr, size))
				break;

			desc = &adreno_context->context_gmem_shadow.gmemshadow;
			if (kgsl_gpuaddr_in_memdesc(desc, gpuaddr, size))
				break;
		}
		next = next + 1;
		desc = NULL;
	}
	read_unlock(&device->context_lock);
	return desc;
}

struct kgsl_memdesc *adreno_find_region(struct kgsl_device *device,
						unsigned int pt_base,
						unsigned int gpuaddr,
						unsigned int size)
{
	struct kgsl_mem_entry *entry;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_ringbuffer *ringbuffer = &adreno_dev->ringbuffer;

	if (kgsl_gpuaddr_in_memdesc(&ringbuffer->buffer_desc, gpuaddr, size))
		return &ringbuffer->buffer_desc;

	if (kgsl_gpuaddr_in_memdesc(&ringbuffer->memptrs_desc, gpuaddr, size))
		return &ringbuffer->memptrs_desc;

	if (kgsl_gpuaddr_in_memdesc(&device->memstore, gpuaddr, size))
		return &device->memstore;

	if (kgsl_gpuaddr_in_memdesc(&adreno_dev->pwron_fixup, gpuaddr, size))
		return &adreno_dev->pwron_fixup;

	if (kgsl_gpuaddr_in_memdesc(&device->mmu.setstate_memory, gpuaddr,
					size))
		return &device->mmu.setstate_memory;

	entry = kgsl_get_mem_entry(device, pt_base, gpuaddr, size);

	if (entry)
		return &entry->memdesc;

	return adreno_find_ctxtmem(device, pt_base, gpuaddr, size);
}

uint8_t *adreno_convertaddr(struct kgsl_device *device, unsigned int pt_base,
			    unsigned int gpuaddr, unsigned int size)
{
	struct kgsl_memdesc *memdesc;

	memdesc = adreno_find_region(device, pt_base, gpuaddr, size);

	return memdesc ? kgsl_gpuaddr_to_vaddr(memdesc, gpuaddr) : NULL;
}

void adreno_regread(struct kgsl_device *device, unsigned int offsetwords,
				unsigned int *value)
{
	unsigned int *reg;
	BUG_ON(offsetwords*sizeof(uint32_t) >= device->reg_len);
	reg = (unsigned int *)(device->reg_virt + (offsetwords << 2));

	if (!in_interrupt())
		kgsl_pre_hwaccess(device);

	*value = __raw_readl(reg);
	rmb();
}

void adreno_regwrite(struct kgsl_device *device, unsigned int offsetwords,
				unsigned int value)
{
	unsigned int *reg;

	BUG_ON(offsetwords*sizeof(uint32_t) >= device->reg_len);

	if (!in_interrupt())
		kgsl_pre_hwaccess(device);

	kgsl_trace_regwrite(device, offsetwords, value);

	kgsl_cffdump_regwrite(device->id, offsetwords << 2, value);
	reg = (unsigned int *)(device->reg_virt + (offsetwords << 2));

	wmb();
	__raw_writel(value, reg);
}

static unsigned int _get_context_id(struct kgsl_context *k_ctxt)
{
	unsigned int context_id = KGSL_MEMSTORE_GLOBAL;
	if (k_ctxt != NULL) {
		struct adreno_context *a_ctxt = k_ctxt->devctxt;
		if (k_ctxt->id == KGSL_CONTEXT_INVALID || a_ctxt == NULL)
			context_id = KGSL_CONTEXT_INVALID;
		else if (a_ctxt->flags & CTXT_FLAGS_PER_CONTEXT_TS)
			context_id = k_ctxt->id;
	}

	return context_id;
}

static unsigned int adreno_check_hw_ts(struct kgsl_device *device,
		struct kgsl_context *context, unsigned int timestamp)
{
	int status = 0;
	unsigned int ref_ts, enableflag;
	unsigned int context_id = _get_context_id(context);

	if (context_id == KGSL_CONTEXT_INVALID) {
		KGSL_DRV_WARN(device, "context was detached");
		return -EINVAL;
	}

	status = kgsl_check_timestamp(device, context, timestamp);
	if (status)
		return status;

	kgsl_sharedmem_readl(&device->memstore, &enableflag,
			KGSL_MEMSTORE_OFFSET(context_id, ts_cmp_enable));

	mb();

	if (enableflag) {
		kgsl_sharedmem_readl(&device->memstore, &ref_ts,
				KGSL_MEMSTORE_OFFSET(context_id,
					ref_wait_ts));

		
		mb();
		if (timestamp_cmp(ref_ts, timestamp) >= 0) {
			kgsl_sharedmem_writel(&device->memstore,
					KGSL_MEMSTORE_OFFSET(context_id,
						ref_wait_ts), timestamp);
			
			wmb();
		}
	} else {
		kgsl_sharedmem_writel(&device->memstore,
				KGSL_MEMSTORE_OFFSET(context_id,
					ref_wait_ts), timestamp);
		enableflag = 1;
		kgsl_sharedmem_writel(&device->memstore,
				KGSL_MEMSTORE_OFFSET(context_id,
					ts_cmp_enable), enableflag);
		
		wmb();


		if (context && device->state != KGSL_STATE_SLUMBER)
			adreno_ringbuffer_issuecmds(device, context->devctxt,
					KGSL_CMD_FLAGS_GET_INT, NULL, 0);
	}

	return 0;
}

static int adreno_next_event(struct kgsl_device *device,
		struct kgsl_event *event)
{
	return adreno_check_hw_ts(device, event->context, event->timestamp);
}

static int adreno_check_interrupt_timestamp(struct kgsl_device *device,
		struct kgsl_context *context, unsigned int timestamp)
{
	int status;

	mutex_lock(&device->mutex);
	status = adreno_check_hw_ts(device, context, timestamp);
	mutex_unlock(&device->mutex);

	return status;
}

#define kgsl_wait_event_interruptible_timeout(wq, condition, timeout, io)\
({									\
	long __ret = timeout;						\
	if (io)						\
		__wait_io_event_interruptible_timeout(wq, condition, __ret);\
	else						\
		__wait_event_interruptible_timeout(wq, condition, __ret);\
	__ret;								\
})



unsigned int adreno_ft_detect(struct kgsl_device *device,
						unsigned int *prev_reg_val)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_ringbuffer *rb = &adreno_dev->ringbuffer;
	unsigned int curr_reg_val[FT_DETECT_REGS_COUNT] = {0};
	unsigned int fast_hang_detected = 1;
	unsigned int long_ib_detected = 1;
	unsigned int i;
	static unsigned long next_hang_detect_time;
	static unsigned int prev_global_ts;
	unsigned int curr_global_ts = 0;
	unsigned int curr_context_id = 0;
	static struct adreno_context *curr_context;
	static struct kgsl_context *context;

	if (!adreno_dev->fast_hang_detect)
		fast_hang_detected = 0;

	if (!adreno_dev->long_ib_detect)
		long_ib_detected = 0;

	if (!(adreno_dev->ringbuffer.flags & KGSL_FLAGS_STARTED))
		return 0;

	if (is_adreno_rbbm_status_idle(device) &&
		(kgsl_readtimestamp(device, NULL, KGSL_TIMESTAMP_RETIRED)
		== rb->global_ts)) {


		if (adreno_is_a2xx(adreno_dev)) {
			unsigned int rptr;
			adreno_regread(device, REG_CP_RB_RPTR, &rptr);
			if (rptr != adreno_dev->ringbuffer.wptr)
				adreno_regwrite(device, REG_CP_RB_WPTR,
					adreno_dev->ringbuffer.wptr);
		}

		return 0;
	}

	if ((next_hang_detect_time) &&
		(time_before(jiffies, next_hang_detect_time)))
			return 0;
	else
		next_hang_detect_time = (jiffies +
			msecs_to_jiffies(KGSL_TIMEOUT_PART-1));

	
	for (i = 0; i < FT_DETECT_REGS_COUNT; i++) {
		if (ft_detect_regs[i] == 0)
			continue;
		adreno_regread(device, ft_detect_regs[i],
			&curr_reg_val[i]);
	}

	
	kgsl_sharedmem_readl(&device->memstore,
			&curr_global_ts,
			KGSL_MEMSTORE_OFFSET(KGSL_MEMSTORE_GLOBAL,
			eoptimestamp));

	mb();

	if (curr_global_ts == prev_global_ts) {

		
		if (context == NULL) {
			kgsl_sharedmem_readl(&device->memstore,
				&curr_context_id,
				KGSL_MEMSTORE_OFFSET(KGSL_MEMSTORE_GLOBAL,
				current_context));
			
			mb();

			context = kgsl_context_get(device, curr_context_id);
			if (context != NULL) {
				curr_context = context->devctxt;
				curr_context->ib_gpu_time_used = 0;
			} else {
				KGSL_DRV_ERR(device,
					"Fault tolerance no context found\n");
			}
		}

		mb();

		for (i = 0; i < FT_DETECT_REGS_COUNT; i++) {
			if (curr_reg_val[i] != prev_reg_val[i]) {
				fast_hang_detected = 0;

				
				if ((i >=
					LONG_IB_DETECT_REG_INDEX_START)
					&&
					(i <=
					LONG_IB_DETECT_REG_INDEX_END))
					long_ib_detected = 0;
			}
		}

		if (fast_hang_detected) {
			KGSL_FT_ERR(device,
				"Proc %s, ctxt_id %d ts %d triggered fault tolerance"
				" on global ts %d\n",
				curr_context ? curr_context->pid_name : "",
				curr_context ? curr_context->id : 0,
				(kgsl_readtimestamp(device, context,
				KGSL_TIMESTAMP_RETIRED) + 1),
				curr_global_ts + 1);
			return 1;
		}

		if (curr_context != NULL) {

			curr_context->ib_gpu_time_used += KGSL_TIMEOUT_PART;
			KGSL_FT_INFO(device,
			"Proc %s used GPU Time %d ms on timestamp 0x%X\n",
			curr_context->pid_name, curr_context->ib_gpu_time_used,
			curr_global_ts+1);

			if ((long_ib_detected) &&
				(!(curr_context->flags &
				 CTXT_FLAGS_NO_FAULT_TOLERANCE))) {
				if (curr_context->ib_gpu_time_used >
					KGSL_TIMEOUT_LONG_IB_DETECTION) {
					if (adreno_dev->long_ib_ts !=
						curr_global_ts) {
						KGSL_FT_ERR(device,
						"Proc %s, ctxt_id %d ts %d"
						"used GPU for %d ms long ib "
						"detected on global ts %d\n",
						curr_context->pid_name,
						curr_context->id,
						(kgsl_readtimestamp(device,
						context,
						KGSL_TIMESTAMP_RETIRED)+1),
						curr_context->ib_gpu_time_used,
						curr_global_ts+1);
						adreno_dev->long_ib = 1;
						adreno_dev->long_ib_ts =
								curr_global_ts;
						curr_context->ib_gpu_time_used =
								0;
						kgsl_context_put(context);
						return 1;
					}
				}
			}
		}
	} else {
		
		prev_global_ts = curr_global_ts;
		kgsl_context_put(context);
		context = NULL;
		curr_context = NULL;
		adreno_dev->long_ib = 0;
		adreno_dev->long_ib_ts = 0;
	}


	for (i = 0; i < FT_DETECT_REGS_COUNT; i++)
			prev_reg_val[i] = curr_reg_val[i];
	return 0;
}

static int _check_pending_timestamp(struct kgsl_device *device,
		struct kgsl_context *context, unsigned int timestamp)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	unsigned int context_id = _get_context_id(context);
	unsigned int ts_issued;

	if (context_id == KGSL_CONTEXT_INVALID)
		return -EINVAL;

	ts_issued = adreno_context_timestamp(context, &adreno_dev->ringbuffer);

	if (timestamp_cmp(timestamp, ts_issued) <= 0)
		return 0;

	if (context && !context->wait_on_invalid_ts) {
		KGSL_DRV_ERR(device, "Cannot wait for invalid ts <%d:0x%x>, last issued ts <%d:0x%x>\n",
			context_id, timestamp, context_id, ts_issued);

			
			context->wait_on_invalid_ts = true;
	}

	return -EINVAL;
}

static int adreno_waittimestamp(struct kgsl_device *device,
				struct kgsl_context *context,
				unsigned int timestamp,
				unsigned int msecs)
{
	static unsigned int io_cnt;
	struct adreno_context *adreno_ctx = context ? context->devctxt : NULL;
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	unsigned int context_id = _get_context_id(context);
	unsigned int time_elapsed = 0;
	unsigned int wait;
	int ts_compare = 1;
	int io, ret = -ETIMEDOUT;

	if (context_id == KGSL_CONTEXT_INVALID) {
		KGSL_DRV_WARN(device, "context was detached");
		return -EINVAL;
	}


	if (adreno_ctx && !(adreno_ctx->flags & CTXT_FLAGS_USER_GENERATED_TS)) {
		if (_check_pending_timestamp(device, context, timestamp))
			return -EINVAL;

		
		context->wait_on_invalid_ts = false;
	}


	wait = 100;

	do {
		long status;


		if (kgsl_check_timestamp(device, context, timestamp)) {
			queue_work(device->work_queue, &device->ts_expired_ws);
			ret = 0;
			break;
		}


		io_cnt = (io_cnt + 1) % 100;
		io = (io_cnt < pwr->pwrlevels[pwr->active_pwrlevel].io_fraction)
			? 0 : 1;

		mutex_unlock(&device->mutex);

		
		status = kgsl_wait_event_interruptible_timeout(
			device->wait_queue,
			adreno_check_interrupt_timestamp(device, context,
				timestamp), msecs_to_jiffies(wait), io);

		mutex_lock(&device->mutex);


		if (status != 0) {
			ret = (status > 0) ? 0 : (int) status;
			break;
		}
		time_elapsed += wait;



		if (ts_compare && (adreno_ctx &&
			(adreno_ctx->flags & CTXT_FLAGS_USER_GENERATED_TS))) {
			if (time_elapsed > KGSL_SYNCOBJ_SERVER_TIMEOUT) {
				ret = _check_pending_timestamp(device, context,
					timestamp);
				if (ret)
					break;

				
				ts_compare = 0;


				context->wait_on_invalid_ts = false;
			}
		}


		if (KGSL_TIMEOUT_PART < (msecs - time_elapsed))
			wait = KGSL_TIMEOUT_PART;
		else
			wait = (msecs - time_elapsed);

	} while (!msecs || time_elapsed < msecs);

	return ret;
}

static unsigned int adreno_readtimestamp(struct kgsl_device *device,
		struct kgsl_context *context, enum kgsl_timestamp_type type)
{
	unsigned int timestamp = 0;
	unsigned int context_id = _get_context_id(context);

	if (context_id == KGSL_CONTEXT_INVALID) {
		KGSL_DRV_WARN(device, "context was detached");
		return timestamp;
	}
	switch (type) {
	case KGSL_TIMESTAMP_QUEUED: {
		struct adreno_device *adreno_dev = ADRENO_DEVICE(device);

		timestamp = adreno_context_timestamp(context,
				&adreno_dev->ringbuffer);
		break;
	}
	case KGSL_TIMESTAMP_CONSUMED:
		kgsl_sharedmem_readl(&device->memstore, &timestamp,
			KGSL_MEMSTORE_OFFSET(context_id, soptimestamp));
		break;
	case KGSL_TIMESTAMP_RETIRED:
		kgsl_sharedmem_readl(&device->memstore, &timestamp,
			KGSL_MEMSTORE_OFFSET(context_id, eoptimestamp));
		break;
	}

	rmb();

	return timestamp;
}

static long adreno_ioctl(struct kgsl_device_private *dev_priv,
			      unsigned int cmd, void *data)
{
	struct kgsl_device *device = dev_priv->device;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	int result = 0;

	switch (cmd) {
	case IOCTL_KGSL_DRAWCTXT_SET_BIN_BASE_OFFSET: {
		struct kgsl_drawctxt_set_bin_base_offset *binbase = data;
		struct kgsl_context *context;

		binbase = data;

		context = kgsl_context_get_owner(dev_priv,
			binbase->drawctxt_id);
		if (context) {
			adreno_drawctxt_set_bin_base_offset(
				device, context, binbase->offset);
		} else {
			result = -EINVAL;
			KGSL_DRV_ERR(device,
				"invalid drawctxt drawctxt_id %d "
				"device_id=%d\n",
				binbase->drawctxt_id, device->id);
		}

		kgsl_context_put(context);
		break;
	}
	case IOCTL_KGSL_PERFCOUNTER_GET: {
		struct kgsl_perfcounter_get *get = data;
		result = adreno_perfcounter_get(adreno_dev, get->groupid,
			get->countable, &get->offset, PERFCOUNTER_FLAG_NONE);
		break;
	}
	case IOCTL_KGSL_PERFCOUNTER_PUT: {
		struct kgsl_perfcounter_put *put = data;
		result = adreno_perfcounter_put(adreno_dev, put->groupid,
			put->countable);
		break;
	}
	case IOCTL_KGSL_PERFCOUNTER_QUERY: {
		struct kgsl_perfcounter_query *query = data;
		result = adreno_perfcounter_query_group(adreno_dev,
			query->groupid, query->countables,
			query->count, &query->max_counters);
		break;
	}
	case IOCTL_KGSL_PERFCOUNTER_READ: {
		struct kgsl_perfcounter_read *read = data;
		result = adreno_perfcounter_read_group(adreno_dev,
			read->reads, read->count);
		break;
	}
	default:
		KGSL_DRV_INFO(dev_priv->device,
			"invalid ioctl code %08x\n", cmd);
		result = -ENOIOCTLCMD;
		break;
	}
	return result;

}

static inline s64 adreno_ticks_to_us(u32 ticks, u32 gpu_freq)
{
	gpu_freq /= 1000000;
	return ticks / gpu_freq;
}

static void adreno_power_stats(struct kgsl_device *device,
				struct kgsl_power_stats *stats)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	unsigned int cycles = 0;

	if (device->state == KGSL_STATE_ACTIVE)
		cycles = adreno_dev->gpudev->busy_cycles(adreno_dev);

	if (pwr->time != 0) {
		s64 tmp = ktime_to_us(ktime_get());
		stats->total_time = tmp - pwr->time;
		pwr->time = tmp;
		stats->busy_time = adreno_ticks_to_us(cycles, device->pwrctrl.
				pwrlevels[device->pwrctrl.active_pwrlevel].
				gpu_freq);
		
		stats->busy_time = (stats->busy_time > stats->total_time) ? stats->total_time : stats->busy_time;
		device->gputime.total = device->gputime.total + stats->total_time;
		device->gputime.busy = device->gputime.busy + stats->busy_time;
		device->gputime_in_state[device->pwrctrl.active_pwrlevel].total
			= device->gputime_in_state[device->pwrctrl.active_pwrlevel].total + stats->total_time;
		device->gputime_in_state[device->pwrctrl.active_pwrlevel].busy
			= device->gputime_in_state[device->pwrctrl.active_pwrlevel].busy + stats->busy_time;

#ifdef CONFIG_MSM_KGSL_GPU_USAGE
		if(device->current_process_priv != NULL) {
			device->current_process_priv->gputime.total
				= device->current_process_priv->gputime.total + stats->total_time;
			device->current_process_priv->gputime.busy
				= device->current_process_priv->gputime.busy + stats->busy_time;
			device->current_process_priv->gputime_in_state[device->pwrctrl.active_pwrlevel].total
				= device->current_process_priv->gputime_in_state[device->pwrctrl.active_pwrlevel].total + stats->total_time;
			device->current_process_priv->gputime_in_state[device->pwrctrl.active_pwrlevel].busy
				= device->current_process_priv->gputime_in_state[device->pwrctrl.active_pwrlevel].busy + stats->busy_time;
		} else
			printk("curent_process_pirv = NULL, skip gpu usage recorde.\n");
#endif
	} else {
		stats->total_time = 0;
		stats->busy_time = 0;
		pwr->time = ktime_to_us(ktime_get());
	}
}

void adreno_irqctrl(struct kgsl_device *device, int state)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	adreno_dev->gpudev->irq_control(adreno_dev, state);
}

static unsigned int adreno_gpuid(struct kgsl_device *device,
	unsigned int *chipid)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);


	if (chipid != NULL)
		*chipid = adreno_dev->chip_id;


	return (0x0003 << 16) | ((int) adreno_dev->gpurev);
}

static const struct kgsl_functable adreno_functable = {
	
	.regread = adreno_regread,
	.regwrite = adreno_regwrite,
	.idle = adreno_idle,
	.isidle = adreno_isidle,
	.suspend_context = adreno_suspend_context,
	.init = adreno_init,
	.start = adreno_start,
	.stop = adreno_stop,
	.getproperty = adreno_getproperty,
	.waittimestamp = adreno_waittimestamp,
	.readtimestamp = adreno_readtimestamp,
	.issueibcmds = adreno_ringbuffer_issueibcmds,
	.ioctl = adreno_ioctl,
	.setup_pt = adreno_setup_pt,
	.cleanup_pt = adreno_cleanup_pt,
	.power_stats = adreno_power_stats,
	.irqctrl = adreno_irqctrl,
	.gpuid = adreno_gpuid,
	.snapshot = adreno_snapshot,
	.irq_handler = adreno_irq_handler,
	
	.setstate = adreno_setstate,
	.drawctxt_create = adreno_drawctxt_create,
	.drawctxt_destroy = adreno_drawctxt_destroy,
	.setproperty = adreno_setproperty,
	.postmortem_dump = adreno_dump,
	.next_event = adreno_next_event,
};

static struct platform_driver adreno_platform_driver = {
	.probe = adreno_probe,
	.remove = __devexit_p(adreno_remove),
	.suspend = kgsl_suspend_driver,
	.resume = kgsl_resume_driver,
	.id_table = adreno_id_table,
	.driver = {
		.owner = THIS_MODULE,
		.name = DEVICE_3D_NAME,
		.pm = &kgsl_pm_ops,
		.of_match_table = adreno_match_table,
	}
};

static int __init kgsl_3d_init(void)
{
	return platform_driver_register(&adreno_platform_driver);
}

static void __exit kgsl_3d_exit(void)
{
	platform_driver_unregister(&adreno_platform_driver);
}

module_init(kgsl_3d_init);
module_exit(kgsl_3d_exit);

MODULE_DESCRIPTION("3D Graphics driver");
MODULE_VERSION("1.2");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:kgsl_3d");
