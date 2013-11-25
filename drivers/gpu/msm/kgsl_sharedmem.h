/* Copyright (c) 2002,2007-2012, The Linux Foundation. All rights reserved.
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
#ifndef __KGSL_SHAREDMEM_H
#define __KGSL_SHAREDMEM_H

#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include "kgsl_mmu.h"
#include <linux/slab.h>
#include <linux/kmemleak.h>
#include <linux/sched.h>

#include "kgsl_log.h"

struct kgsl_device;
struct kgsl_process_private;

#define KGSL_CACHE_OP_INV       0x01
#define KGSL_CACHE_OP_FLUSH     0x02
#define KGSL_CACHE_OP_CLEAN     0x03

extern struct kgsl_memdesc_ops kgsl_page_alloc_ops;

int kgsl_sharedmem_ion_alloc(struct kgsl_memdesc *memdesc,
				struct kgsl_pagetable *pagetable, size_t size);

int kgsl_sharedmem_ion_alloc_user(struct kgsl_memdesc *memdesc,
				struct kgsl_process_private *private,
				struct kgsl_pagetable *pagetable,
				size_t size);

int kgsl_sharedmem_page_alloc(struct kgsl_memdesc *memdesc,
				struct kgsl_pagetable *pagetable, size_t size);

int kgsl_sharedmem_page_alloc_user(struct kgsl_memdesc *memdesc,
				struct kgsl_process_private *private,
				struct kgsl_pagetable *pagetable,
				size_t size);

int kgsl_sharedmem_alloc_coherent(struct kgsl_memdesc *memdesc, size_t size);

int kgsl_sharedmem_ebimem_user(struct kgsl_memdesc *memdesc,
			     struct kgsl_pagetable *pagetable,
			     size_t size);

int kgsl_sharedmem_ebimem(struct kgsl_memdesc *memdesc,
			struct kgsl_pagetable *pagetable,
			size_t size);

void kgsl_sharedmem_free(struct kgsl_memdesc *memdesc);

int kgsl_sharedmem_readl(const struct kgsl_memdesc *memdesc,
			uint32_t *dst,
			unsigned int offsetbytes);

int kgsl_sharedmem_writel(const struct kgsl_memdesc *memdesc,
			unsigned int offsetbytes,
			uint32_t src);

int kgsl_sharedmem_set(const struct kgsl_memdesc *memdesc,
			unsigned int offsetbytes, unsigned int value,
			unsigned int sizebytes);

void kgsl_cache_range_op(struct kgsl_memdesc *memdesc, int op);

void kgsl_process_init_sysfs(struct kgsl_process_private *private);
void kgsl_process_uninit_sysfs(struct kgsl_process_private *private);

int kgsl_sharedmem_init_sysfs(void);
void kgsl_sharedmem_init_ion(void);
void kgsl_sharedmem_uninit_sysfs(void);

static inline int
kgsl_memdesc_get_align(const struct kgsl_memdesc *memdesc)
{
	return (memdesc->flags & KGSL_MEMALIGN_MASK) >> KGSL_MEMALIGN_SHIFT;
}

static inline int
kgsl_memdesc_set_align(struct kgsl_memdesc *memdesc, unsigned int align)
{
	if (align > 32) {
		KGSL_CORE_ERR("Alignment too big, restricting to 2^32\n");
		align = 32;
	}

	memdesc->flags &= ~KGSL_MEMALIGN_MASK;
	memdesc->flags |= (align << KGSL_MEMALIGN_SHIFT) & KGSL_MEMALIGN_MASK;
	return 0;
}

static inline unsigned int kgsl_get_sg_pa(struct scatterlist *sg)
{
	unsigned int pa = sg_dma_address(sg);
	if (pa == 0)
		pa = sg_phys(sg);
	return pa;
}

int
kgsl_sharedmem_map_vma(struct vm_area_struct *vma,
			const struct kgsl_memdesc *memdesc);


static inline void *kgsl_sg_alloc(unsigned int sglen)
{
	if ((sglen * sizeof(struct scatterlist)) <  PAGE_SIZE)
		return kzalloc(sglen * sizeof(struct scatterlist), GFP_KERNEL);
	else {
		void *ptr = vmalloc(sglen * sizeof(struct scatterlist));
		if (ptr)
			memset(ptr, 0, sglen * sizeof(struct scatterlist));

		return ptr;
	}
}

static inline void kgsl_sg_free(void *ptr, unsigned int sglen)
{
	if ((sglen * sizeof(struct scatterlist)) < PAGE_SIZE)
		kfree(ptr);
	else
		vfree(ptr);
}

static inline int
memdesc_sg_phys(struct kgsl_memdesc *memdesc,
		unsigned int physaddr, unsigned int size)
{
	memdesc->sg = kgsl_sg_alloc(1);
	if (!memdesc->sg)
		return -ENOMEM;

	kmemleak_not_leak(memdesc->sg);

	memdesc->sglen = 1;
	memdesc->sglen_alloc = 1;
	sg_init_table(memdesc->sg, 1);
	memdesc->sg[0].length = size;
	memdesc->sg[0].offset = 0;
	memdesc->sg[0].dma_address = physaddr;
	return 0;
}

static inline int
kgsl_allocate(struct kgsl_memdesc *memdesc,
		struct kgsl_pagetable *pagetable, size_t size)
{
	int ret = 1;

	if (kgsl_mmu_get_mmutype() == KGSL_MMU_TYPE_NONE)
		return kgsl_sharedmem_ebimem(memdesc, pagetable, size);

	memdesc->flags |= (KGSL_MEMTYPE_KERNEL << KGSL_MEMTYPE_SHIFT);
	if (size >= SZ_4M)
		ret = kgsl_sharedmem_ion_alloc(memdesc, pagetable, size);

	if (ret)
		return kgsl_sharedmem_page_alloc(memdesc, pagetable, size);

	return ret;
}

static inline int
kgsl_allocate_user(struct kgsl_memdesc *memdesc,
		struct kgsl_process_private *private,
		struct kgsl_pagetable *pagetable,
		size_t size, unsigned int flags)
{
	int ret = 1;
	char task_comm[TASK_COMM_LEN];

	memdesc->flags = flags;

	if (kgsl_mmu_get_mmutype() == KGSL_MMU_TYPE_NONE)
		ret = kgsl_sharedmem_ebimem_user(memdesc, pagetable, size);
	else {
		if (size >= SZ_4M)
			ret = kgsl_sharedmem_ion_alloc_user(memdesc, private, pagetable, size);
		else if ( size >= SZ_1M && strcmp("om.htc.launcher", get_task_comm(task_comm, current->group_leader)) == 0 )
			ret = kgsl_sharedmem_ion_alloc_user(memdesc, private, pagetable, size);
		if (ret)
			ret = kgsl_sharedmem_page_alloc_user(memdesc, private, pagetable, size);
	}

	return ret;
}

static inline int
kgsl_allocate_contiguous(struct kgsl_memdesc *memdesc, size_t size)
{
	int ret  = kgsl_sharedmem_alloc_coherent(memdesc, size);
	if (!ret && (kgsl_mmu_get_mmutype() == KGSL_MMU_TYPE_NONE))
		memdesc->gpuaddr = memdesc->physaddr;

	memdesc->flags |= (KGSL_MEMTYPE_KERNEL << KGSL_MEMTYPE_SHIFT);
	return ret;
}

static inline int kgsl_sg_size(struct scatterlist *sg, int sglen)
{
	int i, size = 0;
	struct scatterlist *s;

	for_each_sg(sg, s, sglen, i) {
		size += s->length;
	}

	return size;
}
#endif 
