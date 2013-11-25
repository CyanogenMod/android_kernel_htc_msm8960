/*
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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
#ifndef __ARCH_MACH_MSM_SUBSYSTEM_MAP_H
#define __ARCH_MACH_MSM_SUBSYSTEM_MAP_H

#include <linux/iommu.h>
#include <mach/iommu_domains.h>

#define MSM_SUBSYSTEM_MAP_KADDR		0x1
#define MSM_SUBSYSTEM_MAP_IOVA		0x2
#define	MSM_SUBSYSTEM_MAP_CACHED	0x4
#define MSM_SUBSYSTEM_MAP_UNCACHED	0x8
#define MSM_SUBSYSTEM_MAP_IOMMU_2X 0x10

#define	MSM_SUBSYSTEM_ALIGN_IOVA_8K	SZ_8K
#define MSM_SUBSYSTEM_ALIGN_IOVA_1M	SZ_1M


enum msm_subsystem_id {
	INVALID_SUBSYS_ID = -1,
	MSM_SUBSYSTEM_VIDEO,
	MSM_SUBSYSTEM_VIDEO_FWARE,
	MSM_SUBSYSTEM_CAMERA,
	MSM_SUBSYSTEM_DISPLAY,
	MSM_SUBSYSTEM_ROTATOR,
	MAX_SUBSYSTEM_ID
};

static inline int msm_subsystem_check_id(int subsys_id)
{
	return subsys_id > INVALID_SUBSYS_ID && subsys_id < MAX_SUBSYSTEM_ID;
}

struct msm_mapped_buffer {
	void *vaddr;
	unsigned long *iova;
};

extern struct msm_mapped_buffer *msm_subsystem_map_buffer(
				unsigned long phys,
				unsigned int length,
				unsigned int flags,
				int *subsys_ids,
				unsigned int nsubsys);

extern int msm_subsystem_unmap_buffer(struct msm_mapped_buffer *buf);

extern phys_addr_t msm_subsystem_check_iova_mapping(int subsys_id,
						unsigned long iova);

#endif 
