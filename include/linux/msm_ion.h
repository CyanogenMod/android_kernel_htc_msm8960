/*
 *
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_MSM_ION_H
#define _LINUX_MSM_ION_H

#include <linux/ion.h>


enum ion_heap_ids {
	INVALID_HEAP_ID = -1,
	ION_CP_MM_HEAP_ID = 8,
	ION_CP_MFC_HEAP_ID = 12,
	ION_CP_WB_HEAP_ID = 16, 
	ION_CAMERA_HEAP_ID = 20, 
	ION_SYSTEM_CONTIG_HEAP_ID = 21,
	ION_ADSP_HEAP_ID = 22,
	ION_SF_HEAP_ID = 24,
	ION_IOMMU_HEAP_ID = 25,
	ION_QSECOM_HEAP_ID = 27,
	ION_AUDIO_HEAP_ID = 28,

	ION_MM_FIRMWARE_HEAP_ID = 29,
	ION_SYSTEM_HEAP_ID = 30,

	ION_HEAP_ID_RESERVED = 31 
};

enum ion_fixed_position {
	NOT_FIXED,
	FIXED_LOW,
	FIXED_MIDDLE,
	FIXED_HIGH,
};

enum cp_mem_usage {
	VIDEO_BITSTREAM = 0x1,
	VIDEO_PIXEL = 0x2,
	VIDEO_NONPIXEL = 0x3,
	MAX_USAGE = 0x4,
	UNKNOWN = 0x7FFFFFFF,
};

#define ION_HEAP_CP_MASK		(1 << ION_HEAP_TYPE_CP)

#define ION_FLAG_SECURE (1 << ION_HEAP_ID_RESERVED)

#define ION_FLAG_FORCE_CONTIGUOUS (1 << 30)

#define ION_SECURE ION_FLAG_SECURE
#define ION_FORCE_CONTIGUOUS ION_FLAG_FORCE_CONTIGUOUS

#define ION_HEAP(bit) (1 << (bit))

#define ION_ADSP_HEAP_NAME	"adsp"
#define ION_VMALLOC_HEAP_NAME	"vmalloc"
#define ION_KMALLOC_HEAP_NAME	"kmalloc"
#define ION_AUDIO_HEAP_NAME	"audio"
#define ION_SF_HEAP_NAME	"sf"
#define ION_MM_HEAP_NAME	"mm"
#define ION_CAMERA_HEAP_NAME	"camera_preview"
#define ION_IOMMU_HEAP_NAME	"iommu"
#define ION_MFC_HEAP_NAME	"mfc"
#define ION_WB_HEAP_NAME	"wb"
#define ION_MM_FIRMWARE_HEAP_NAME	"mm_fw"
#define ION_QSECOM_HEAP_NAME	"qsecom"
#define ION_FMEM_HEAP_NAME	"fmem"

#define ION_SET_CACHED(__cache)		(__cache | ION_FLAG_CACHED)
#define ION_SET_UNCACHED(__cache)	(__cache & ~ION_FLAG_CACHED)

#define ION_IS_CACHED(__flags)	((__flags) & ION_FLAG_CACHED)

#ifdef __KERNEL__

#define ION_IOMMU_UNMAP_DELAYED 1

struct ion_cp_heap_pdata {
	enum ion_permission_type permission_type;
	unsigned int align;
	ion_phys_addr_t secure_base; 
	size_t secure_size; 
	int reusable;
	int mem_is_fmem;
	int is_cma;
	enum ion_fixed_position fixed_position;
	int iommu_map_all;
	int iommu_2x_map_domain;
	ion_virt_addr_t *virt_addr;
	int (*request_region)(void *);
	int (*release_region)(void *);
	void *(*setup_region)(void);
	enum ion_memory_types memory_type;
	int no_nonsecure_alloc;
};

struct ion_co_heap_pdata {
	int adjacent_mem_id;
	unsigned int align;
	int mem_is_fmem;
	enum ion_fixed_position fixed_position;
	int (*request_region)(void *);
	int (*release_region)(void *);
	void *(*setup_region)(void);
	enum ion_memory_types memory_type;
};

#ifdef CONFIG_ION
int msm_ion_secure_heap(int heap_id);

int msm_ion_unsecure_heap(int heap_id);

int msm_ion_secure_heap_2_0(int heap_id, enum cp_mem_usage usage);

int msm_ion_unsecure_heap_2_0(int heap_id, enum cp_mem_usage usage);
#else
static inline int msm_ion_secure_heap(int heap_id)
{
	return -ENODEV;

}

static inline int msm_ion_unsecure_heap(int heap_id)
{
	return -ENODEV;
}

static inline int msm_ion_secure_heap_2_0(int heap_id, enum cp_mem_usage usage)
{
	return -ENODEV;
}

static inline int msm_ion_unsecure_heap_2_0(int heap_id,
					enum cp_mem_usage usage)
{
	return -ENODEV;
}
#endif 

#endif 

struct ion_flush_data {
	struct ion_handle *handle;
	int fd;
	void *vaddr;
	unsigned int offset;
	unsigned int length;
};

struct ion_flag_data {
	struct ion_handle *handle;
	unsigned long flags;
};

#define ION_IOC_MSM_MAGIC 'M'

#define ION_IOC_CLEAN_CACHES	_IOWR(ION_IOC_MSM_MAGIC, 0, \
						struct ion_flush_data)
#define ION_IOC_INV_CACHES	_IOWR(ION_IOC_MSM_MAGIC, 1, \
						struct ion_flush_data)
#define ION_IOC_CLEAN_INV_CACHES	_IOWR(ION_IOC_MSM_MAGIC, 2, \
						struct ion_flush_data)

#define ION_IOC_GET_FLAGS		_IOWR(ION_IOC_MSM_MAGIC, 3, \
						struct ion_flag_data)

#endif
