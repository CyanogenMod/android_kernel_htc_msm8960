/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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
#ifndef __KGSL_IOMMU_H
#define __KGSL_IOMMU_H

#include <mach/iommu.h>

#define KGSL_IOMMU_CTX_OFFSET_V1	0
#define KGSL_IOMMU_CTX_OFFSET_V2	0x8000
#define KGSL_IOMMU_CTX_SHIFT		12

#define KGSL_IOMMU_TLBLKCR_LKE_MASK		0x00000001
#define KGSL_IOMMU_TLBLKCR_LKE_SHIFT		0
#define KGSL_IOMMU_TLBLKCR_TLBIALLCFG_MASK	0x00000001
#define KGSL_IOMMU_TLBLKCR_TLBIALLCFG_SHIFT	1
#define KGSL_IOMMU_TLBLKCR_TLBIASIDCFG_MASK	0x00000001
#define KGSL_IOMMU_TLBLKCR_TLBIASIDCFG_SHIFT	2
#define KGSL_IOMMU_TLBLKCR_TLBIVAACFG_MASK	0x00000001
#define KGSL_IOMMU_TLBLKCR_TLBIVAACFG_SHIFT	3
#define KGSL_IOMMU_TLBLKCR_FLOOR_MASK		0x000000FF
#define KGSL_IOMMU_TLBLKCR_FLOOR_SHIFT		8
#define KGSL_IOMMU_TLBLKCR_VICTIM_MASK		0x000000FF
#define KGSL_IOMMU_TLBLKCR_VICTIM_SHIFT		16

#define KGSL_IOMMU_V2PXX_INDEX_MASK		0x000000FF
#define KGSL_IOMMU_V2PXX_INDEX_SHIFT		0
#define KGSL_IOMMU_V2PXX_VA_MASK		0x000FFFFF
#define KGSL_IOMMU_V2PXX_VA_SHIFT		12

#define KGSL_IOMMU_FSYNR1_AWRITE_MASK		0x00000001
#define KGSL_IOMMU_FSYNR1_AWRITE_SHIFT		8
#define KGSL_IOMMU_V1_FSYNR0_WNR_MASK		0x00000001
#define KGSL_IOMMU_V1_FSYNR0_WNR_SHIFT		4

enum kgsl_iommu_reg_map {
	KGSL_IOMMU_GLOBAL_BASE = 0,
	KGSL_IOMMU_CTX_TTBR0,
	KGSL_IOMMU_CTX_TTBR1,
	KGSL_IOMMU_CTX_FSR,
	KGSL_IOMMU_CTX_TLBIALL,
	KGSL_IOMMU_CTX_RESUME,
	KGSL_IOMMU_CTX_TLBLKCR,
	KGSL_IOMMU_CTX_V2PUR,
	KGSL_IOMMU_CTX_FSYNR0,
	KGSL_IOMMU_CTX_FSYNR1,
	KGSL_IOMMU_REG_MAX
};

struct kgsl_iommu_register_list {
	unsigned int reg_offset;
	unsigned int reg_mask;
	unsigned int reg_shift;
};

#define KGSL_IOMMU_MAX_UNITS 2

#define KGSL_IOMMU_MAX_DEVS_PER_UNIT 2

#define KGSL_IOMMU_SET_CTX_REG(iommu, iommu_unit, ctx, REG, val)	\
		writel_relaxed(val,					\
		iommu_unit->reg_map.hostptr +				\
		iommu->iommu_reg_list[KGSL_IOMMU_CTX_##REG].reg_offset +\
		(ctx << KGSL_IOMMU_CTX_SHIFT) +				\
		iommu->ctx_offset)

#define KGSL_IOMMU_GET_CTX_REG(iommu, iommu_unit, ctx, REG)		\
		readl_relaxed(						\
		iommu_unit->reg_map.hostptr +				\
		iommu->iommu_reg_list[KGSL_IOMMU_CTX_##REG].reg_offset +\
		(ctx << KGSL_IOMMU_CTX_SHIFT) +				\
		iommu->ctx_offset)

#define KGSL_IOMMMU_PT_LSB(iommu, pt_val) \
	(pt_val &							\
	~(iommu->iommu_reg_list[KGSL_IOMMU_CTX_TTBR0].reg_mask <<	\
	iommu->iommu_reg_list[KGSL_IOMMU_CTX_TTBR0].reg_shift))

#define KGSL_IOMMU_SETSTATE_NOP_OFFSET	1024

struct kgsl_iommu_device {
	struct device *dev;
	bool attached;
	unsigned int pt_lsb;
	enum kgsl_iommu_context_id ctx_id;
	bool clk_enabled;
	struct kgsl_device *kgsldev;
	int fault;
};

struct kgsl_iommu_unit {
	struct kgsl_iommu_device dev[KGSL_IOMMU_MAX_DEVS_PER_UNIT];
	unsigned int dev_count;
	struct kgsl_memdesc reg_map;
};

struct kgsl_iommu {
	struct kgsl_iommu_unit iommu_units[KGSL_IOMMU_MAX_UNITS];
	unsigned int unit_count;
	unsigned int iommu_last_cmd_ts;
	bool clk_event_queued;
	struct kgsl_device *device;
	unsigned int ctx_offset;
	struct kgsl_iommu_register_list *iommu_reg_list;
	struct remote_iommu_petersons_spinlock *sync_lock_vars;
	struct kgsl_memdesc sync_lock_desc;
	unsigned int sync_lock_offset;
	bool sync_lock_initialized;
};

struct kgsl_iommu_pt {
	struct iommu_domain *domain;
	struct kgsl_iommu *iommu;
};

#endif
