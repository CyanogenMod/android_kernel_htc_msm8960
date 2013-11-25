/*
 * videobuf2-memops.c - generic memory handling routines for videobuf2
 *
 * Copyright (C) 2010 Samsung Electronics
 *
 * Author: Pawel Osciak <pawel@osciak.com>
 *	   Marek Szyprowski <m.szyprowski@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/slab.h>

#include <media/videobuf2-core.h>
#include <media/videobuf2-memops.h>

struct vm_area_struct *vb2_get_vma(struct vm_area_struct *vma)
{
	struct vm_area_struct *vma_copy;

	vma_copy = kmalloc(sizeof(*vma_copy), GFP_KERNEL);
	if (vma_copy == NULL)
		return NULL;

	if (vma->vm_ops && vma->vm_ops->open)
		vma->vm_ops->open(vma);

	if (vma->vm_file)
		get_file(vma->vm_file);

	memcpy(vma_copy, vma, sizeof(*vma));

	vma_copy->vm_mm = NULL;
	vma_copy->vm_next = NULL;
	vma_copy->vm_prev = NULL;

	return vma_copy;
}

void vb2_put_vma(struct vm_area_struct *vma)
{
	if (!vma)
		return;

	if (vma->vm_file)
		fput(vma->vm_file);

	if (vma->vm_ops && vma->vm_ops->close)
		vma->vm_ops->close(vma);

	kfree(vma);
}
EXPORT_SYMBOL_GPL(vb2_put_vma);

int vb2_get_contig_userptr(unsigned long vaddr, unsigned long size,
			   struct vm_area_struct **res_vma, dma_addr_t *res_pa)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	unsigned long offset, start, end;
	unsigned long this_pfn, prev_pfn;
	dma_addr_t pa = 0;
	int ret = -EFAULT;

	start = vaddr;
	offset = start & ~PAGE_MASK;
	end = start + size;

	down_read(&mm->mmap_sem);
	vma = find_vma(mm, start);

	if (vma == NULL || vma->vm_end < end)
		goto done;

	for (prev_pfn = 0; start < end; start += PAGE_SIZE) {
		ret = follow_pfn(vma, start, &this_pfn);
		if (ret)
			goto done;

		if (prev_pfn == 0)
			pa = this_pfn << PAGE_SHIFT;
		else if (this_pfn != prev_pfn + 1) {
			ret = -EFAULT;
			goto done;
		}
		prev_pfn = this_pfn;
	}

	*res_vma = vb2_get_vma(vma);
	if (*res_vma == NULL) {
		ret = -ENOMEM;
		goto done;
	}
	*res_pa = pa + offset;
	ret = 0;

done:
	up_read(&mm->mmap_sem);
	return ret;
}
EXPORT_SYMBOL_GPL(vb2_get_contig_userptr);

int vb2_mmap_pfn_range(struct vm_area_struct *vma, unsigned long paddr,
				unsigned long size,
				const struct vm_operations_struct *vm_ops,
				void *priv)
{
	int ret;

	size = min_t(unsigned long, vma->vm_end - vma->vm_start, size);

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	ret = remap_pfn_range(vma, vma->vm_start, paddr >> PAGE_SHIFT,
				size, vma->vm_page_prot);
	if (ret) {
		printk(KERN_ERR "Remapping memory failed, error: %d\n", ret);
		return ret;
	}

	vma->vm_flags		|= VM_DONTEXPAND | VM_RESERVED;
	vma->vm_private_data	= priv;
	vma->vm_ops		= vm_ops;

	vma->vm_ops->open(vma);

	printk(KERN_DEBUG "%s: mapped paddr 0x%08lx at 0x%08lx, size %ld\n",
			__func__, paddr, vma->vm_start, size);

	return 0;
}
EXPORT_SYMBOL_GPL(vb2_mmap_pfn_range);

static void vb2_common_vm_open(struct vm_area_struct *vma)
{
	struct vb2_vmarea_handler *h = vma->vm_private_data;

	printk(KERN_DEBUG "%s: %p, refcount: %d, vma: %08lx-%08lx\n",
	       __func__, h, atomic_read(h->refcount), vma->vm_start,
	       vma->vm_end);

	atomic_inc(h->refcount);
}

static void vb2_common_vm_close(struct vm_area_struct *vma)
{
	struct vb2_vmarea_handler *h = vma->vm_private_data;

	printk(KERN_DEBUG "%s: %p, refcount: %d, vma: %08lx-%08lx\n",
	       __func__, h, atomic_read(h->refcount), vma->vm_start,
	       vma->vm_end);

	h->put(h->arg);
}

const struct vm_operations_struct vb2_common_vm_ops = {
	.open = vb2_common_vm_open,
	.close = vb2_common_vm_close,
};
EXPORT_SYMBOL_GPL(vb2_common_vm_ops);

MODULE_DESCRIPTION("common memory handling routines for videobuf2");
MODULE_AUTHOR("Pawel Osciak <pawel@osciak.com>");
MODULE_LICENSE("GPL");
