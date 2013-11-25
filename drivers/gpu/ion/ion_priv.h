/*
 * drivers/gpu/ion/ion_priv.h
 *
 * Copyright (C) 2011 Google, Inc.
 * Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#ifndef _ION_PRIV_H
#define _ION_PRIV_H

#include <linux/kref.h>
#include <linux/mm_types.h>
#include <linux/mutex.h>
#include <linux/rbtree.h>
#include <linux/ion.h>
#include <linux/iommu.h>
#include <linux/seq_file.h>

enum {
	DI_PARTITION_NUM = 0,
	DI_DOMAIN_NUM = 1,
	DI_MAX,
};

struct ion_iommu_map {
	unsigned long iova_addr;
	struct rb_node node;
	union {
		int domain_info[DI_MAX];
		uint64_t key;
	};
	struct ion_buffer *buffer;
	struct kref ref;
	int mapped_size;
	unsigned long flags;
};

struct ion_buffer *ion_handle_buffer(struct ion_handle *handle);

struct ion_buffer {
	struct kref ref;
	struct rb_node node;
	struct ion_device *dev;
	struct ion_heap *heap;
	struct ion_client *creator;
	unsigned long flags;
	size_t size;
	union {
		void *priv_virt;
		ion_phys_addr_t priv_phys;
	};
	struct mutex lock;
	int kmap_cnt;
	void *vaddr;
	int dmap_cnt;
	struct sg_table *sg_table;
	int umap_cnt;
	unsigned int iommu_map_cnt;
	struct rb_root iommu_maps;
	int marked;
};

struct ion_heap_ops {
	int (*allocate) (struct ion_heap *heap,
			 struct ion_buffer *buffer, unsigned long len,
			 unsigned long align, unsigned long flags);
	void (*free) (struct ion_buffer *buffer);
	int (*phys) (struct ion_heap *heap, struct ion_buffer *buffer,
		     ion_phys_addr_t *addr, size_t *len);
	struct sg_table *(*map_dma) (struct ion_heap *heap,
					struct ion_buffer *buffer);
	void (*unmap_dma) (struct ion_heap *heap, struct ion_buffer *buffer);
	void * (*map_kernel) (struct ion_heap *heap, struct ion_buffer *buffer);
	void (*unmap_kernel) (struct ion_heap *heap, struct ion_buffer *buffer);
	int (*map_user) (struct ion_heap *mapper, struct ion_buffer *buffer,
			 struct vm_area_struct *vma);
	void (*unmap_user) (struct ion_heap *mapper, struct ion_buffer *buffer);
	int (*cache_op)(struct ion_heap *heap, struct ion_buffer *buffer,
			void *vaddr, unsigned int offset,
			unsigned int length, unsigned int cmd);
	int (*map_iommu)(struct ion_buffer *buffer,
				struct ion_iommu_map *map_data,
				unsigned int domain_num,
				unsigned int partition_num,
				unsigned long align,
				unsigned long iova_length,
				unsigned long flags);
	void (*unmap_iommu)(struct ion_iommu_map *data);
	int (*print_debug)(struct ion_heap *heap, struct seq_file *s,
			   const struct rb_root *mem_map);
	int (*secure_heap)(struct ion_heap *heap, int version, void *data);
	int (*unsecure_heap)(struct ion_heap *heap, int version, void *data);
};

struct ion_heap {
	struct rb_node node;
	struct ion_device *dev;
	enum ion_heap_type type;
	struct ion_heap_ops *ops;
	int id;
	const char *name;
};

struct mem_map_data {
	struct rb_node node;
	unsigned long addr;
	unsigned long addr_end;
	unsigned long size;
	const char *client_name;
	const char *creator_name;
};

#define iommu_map_domain(__m)		((__m)->domain_info[1])
#define iommu_map_partition(__m)	((__m)->domain_info[0])

struct ion_device *ion_device_create(long (*custom_ioctl)
				     (struct ion_client *client,
				      unsigned int cmd,
				      unsigned long arg));

void ion_device_destroy(struct ion_device *dev);

void ion_device_add_heap(struct ion_device *dev, struct ion_heap *heap);


struct ion_heap *ion_heap_create(struct ion_platform_heap *);
void ion_heap_destroy(struct ion_heap *);

struct ion_heap *ion_system_heap_create(struct ion_platform_heap *);
void ion_system_heap_destroy(struct ion_heap *);

struct ion_heap *ion_system_contig_heap_create(struct ion_platform_heap *);
void ion_system_contig_heap_destroy(struct ion_heap *);

struct ion_heap *ion_carveout_heap_create(struct ion_platform_heap *);
void ion_carveout_heap_destroy(struct ion_heap *);

struct ion_heap *ion_iommu_heap_create(struct ion_platform_heap *);
void ion_iommu_heap_destroy(struct ion_heap *);

struct ion_heap *ion_cp_heap_create(struct ion_platform_heap *);
void ion_cp_heap_destroy(struct ion_heap *);

struct ion_heap *ion_reusable_heap_create(struct ion_platform_heap *);
void ion_reusable_heap_destroy(struct ion_heap *);

ion_phys_addr_t ion_carveout_allocate(struct ion_heap *heap, unsigned long size,
				      unsigned long align);
void ion_carveout_free(struct ion_heap *heap, ion_phys_addr_t addr,
		       unsigned long size);


struct ion_heap *msm_get_contiguous_heap(void);
#define ION_CARVEOUT_ALLOCATE_FAIL -1
#define ION_CP_ALLOCATE_FAIL -1

#define ION_RESERVED_ALLOCATE_FAIL -1

void *ion_map_fmem_buffer(struct ion_buffer *buffer, unsigned long phys_base,
				void *virt_base, unsigned long flags);

int ion_do_cache_op(struct ion_client *client, struct ion_handle *handle,
			void *uaddr, unsigned long offset, unsigned long len,
			unsigned int cmd);

void ion_cp_heap_get_base(struct ion_heap *heap, unsigned long *base,
			unsigned long *size);

void ion_mem_map_show(struct ion_heap *heap);

#endif 
