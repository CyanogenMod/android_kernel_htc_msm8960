/*
 * include/linux/ion.h
 *
 * Copyright (C) 2011 Google, Inc.
 * Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
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

#ifndef _LINUX_ION_H
#define _LINUX_ION_H

#include <linux/ioctl.h>
#include <linux/types.h>

struct ion_handle;
enum ion_heap_type {
	ION_HEAP_TYPE_SYSTEM,
	ION_HEAP_TYPE_SYSTEM_CONTIG,
	ION_HEAP_TYPE_CARVEOUT,
	ION_HEAP_TYPE_IOMMU,
	ION_HEAP_TYPE_CP,
	ION_HEAP_TYPE_DMA,
	ION_HEAP_TYPE_CUSTOM, 
	ION_NUM_HEAPS,
};

#define ION_HEAP_SYSTEM_MASK		(1 << ION_HEAP_TYPE_SYSTEM)
#define ION_HEAP_SYSTEM_CONTIG_MASK	(1 << ION_HEAP_TYPE_SYSTEM_CONTIG)
#define ION_HEAP_CARVEOUT_MASK		(1 << ION_HEAP_TYPE_CARVEOUT)
#define ION_HEAP_TYPE_DMA_MASK         (1 << ION_HEAP_TYPE_DMA)

#define ION_FLAG_CACHED 1		

#ifdef __KERNEL__
#include <linux/err.h>
#include <mach/ion.h>
struct ion_device;
struct ion_heap;
struct ion_mapper;
struct ion_client;
struct ion_buffer;

#define ion_phys_addr_t unsigned long
#define ion_virt_addr_t unsigned long

struct ion_platform_heap {
	enum ion_heap_type type;
	unsigned int id;
	const char *name;
	ion_phys_addr_t base;
	size_t size;
	enum ion_memory_types memory_type;
	unsigned int has_outer_cache;
	void *extra_data;
	void *priv;
};

struct ion_platform_data {
	unsigned int has_outer_cache;
	int nr;
	int (*request_region)(void *);
	int (*release_region)(void *);
	void *(*setup_region)(void);
	struct ion_platform_heap heaps[];
};

#ifdef CONFIG_ION

void ion_reserve(struct ion_platform_data *data);

struct ion_client *ion_client_create(struct ion_device *dev,
				     unsigned int heap_mask, const char *name);


struct ion_client *msm_ion_client_create(unsigned int heap_mask,
					const char *name);

void ion_client_destroy(struct ion_client *client);

struct ion_handle *ion_alloc(struct ion_client *client, size_t len,
			     size_t align, unsigned int heap_mask,
			     unsigned int flags);

void ion_free(struct ion_client *client, struct ion_handle *handle);

int ion_phys(struct ion_client *client, struct ion_handle *handle,
	     ion_phys_addr_t *addr, size_t *len);

struct sg_table *ion_sg_table(struct ion_client *client,
			      struct ion_handle *handle);

void *ion_map_kernel(struct ion_client *client, struct ion_handle *handle);

void ion_unmap_kernel(struct ion_client *client, struct ion_handle *handle);

int ion_share_dma_buf(struct ion_client *client, struct ion_handle *handle);

struct ion_handle *ion_import_dma_buf(struct ion_client *client, int fd);

int ion_handle_get_flags(struct ion_client *client, struct ion_handle *handle,
				unsigned long *flags);


int ion_map_iommu(struct ion_client *client, struct ion_handle *handle,
			int domain_num, int partition_num, unsigned long align,
			unsigned long iova_length, unsigned long *iova,
			unsigned long *buffer_size,
			unsigned long flags, unsigned long iommu_flags);



int ion_handle_get_size(struct ion_client *client, struct ion_handle *handle,
			unsigned long *size);

void ion_unmap_iommu(struct ion_client *client, struct ion_handle *handle,
			int domain_num, int partition_num);


int ion_secure_heap(struct ion_device *dev, int heap_id, int version,
			void *data);

int ion_unsecure_heap(struct ion_device *dev, int heap_id, int version,
			void *data);

int msm_ion_do_cache_op(struct ion_client *client, struct ion_handle *handle,
			void *vaddr, unsigned long len, unsigned int cmd);

int ion_iommu_heap_dump_size(void);

#else
static inline void ion_reserve(struct ion_platform_data *data)
{

}

static inline struct ion_client *ion_client_create(struct ion_device *dev,
				     unsigned int heap_mask, const char *name)
{
	return ERR_PTR(-ENODEV);
}

static inline struct ion_client *msm_ion_client_create(unsigned int heap_mask,
					const char *name)
{
	return ERR_PTR(-ENODEV);
}

static inline void ion_client_destroy(struct ion_client *client) { }

static inline struct ion_handle *ion_alloc(struct ion_client *client,
					size_t len, size_t align,
					unsigned int heap_mask,
					unsigned int flags)
{
	return ERR_PTR(-ENODEV);
}

static inline void ion_free(struct ion_client *client,
	struct ion_handle *handle) { }


static inline int ion_phys(struct ion_client *client,
	struct ion_handle *handle, ion_phys_addr_t *addr, size_t *len)
{
	return -ENODEV;
}

static inline struct sg_table *ion_sg_table(struct ion_client *client,
			      struct ion_handle *handle)
{
	return ERR_PTR(-ENODEV);
}

static inline void *ion_map_kernel(struct ion_client *client,
	struct ion_handle *handle, unsigned long flags)
{
	return ERR_PTR(-ENODEV);
}

static inline void ion_unmap_kernel(struct ion_client *client,
	struct ion_handle *handle) { }

static inline int ion_share_dma_buf(struct ion_client *client, struct ion_handle *handle)
{
	return -ENODEV;
}

static inline struct ion_handle *ion_import_dma_buf(struct ion_client *client, int fd)
{
	return ERR_PTR(-ENODEV);
}

static inline int ion_handle_get_flags(struct ion_client *client,
	struct ion_handle *handle, unsigned long *flags)
{
	return -ENODEV;
}

static inline int ion_map_iommu(struct ion_client *client,
			struct ion_handle *handle, int domain_num,
			int partition_num, unsigned long align,
			unsigned long iova_length, unsigned long *iova,
			unsigned long *buffer_size,
			unsigned long flags,
			unsigned long iommu_flags)
{
	return -ENODEV;
}

static inline void ion_unmap_iommu(struct ion_client *client,
			struct ion_handle *handle, int domain_num,
			int partition_num)
{
	return;
}

static inline int ion_secure_heap(struct ion_device *dev, int heap_id,
					int version, void *data)
{
	return -ENODEV;

}

static inline int ion_unsecure_heap(struct ion_device *dev, int heap_id,
					int version, void *data)
{
	return -ENODEV;
}

static inline int msm_ion_do_cache_op(struct ion_client *client,
			struct ion_handle *handle, void *vaddr,
			unsigned long len, unsigned int cmd)
{
	return -ENODEV;
}

#endif 
#endif 


struct ion_allocation_data {
	size_t len;
	size_t align;
	unsigned int heap_mask;
	unsigned int flags;
	struct ion_handle *handle;
};

struct ion_allocation_data_old {
	size_t len;
	size_t align;
	unsigned int flags;
	struct ion_handle *handle;
};
struct ion_fd_data {
	struct ion_handle *handle;
	int fd;
};

struct ion_handle_data {
	struct ion_handle *handle;
};

struct ion_custom_data {
	unsigned int cmd;
	unsigned long arg;
};

#define ION_CLIENT_NAME_LENGTH 64

struct ion_client_name_data {
	int len;
	char* name;
};

#define ION_IOC_MAGIC		'I'

#define ION_IOC_ALLOC		_IOWR(ION_IOC_MAGIC, 0, \
				      struct ion_allocation_data)

#define ION_IOC_ALLOC_OLD	_IOWR(ION_IOC_MAGIC, 0, \
				      struct ion_allocation_data_old)

#define ION_IOC_FREE		_IOWR(ION_IOC_MAGIC, 1, struct ion_handle_data)

#define ION_IOC_MAP		_IOWR(ION_IOC_MAGIC, 2, struct ion_fd_data)

#define ION_IOC_SHARE		_IOWR(ION_IOC_MAGIC, 4, struct ion_fd_data)

#define ION_IOC_IMPORT		_IOWR(ION_IOC_MAGIC, 5, struct ion_fd_data)

#define ION_IOC_CUSTOM		_IOWR(ION_IOC_MAGIC, 6, struct ion_custom_data)

#define ION_IOC_CLIENT_RENAME          _IOWR(ION_IOC_MAGIC, 12, \
						struct ion_client_name_data)
#endif 
