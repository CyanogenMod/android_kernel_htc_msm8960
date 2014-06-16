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

#include <linux/export.h>
#include <linux/vmalloc.h>
#include <linux/memory_alloc.h>
#include <asm/cacheflush.h>
#include <linux/slab.h>
#include <linux/kmemleak.h>
#include <linux/highmem.h>

#include "kgsl.h"
#include "kgsl_sharedmem.h"
#include "kgsl_cffdump.h"
#include "kgsl_device.h"

struct ion_client* kgsl_client = NULL;

struct kgsl_mem_entry_attribute {
	struct attribute attr;
	int memtype;
	ssize_t (*show)(struct kgsl_process_private *priv,
		int type, char *buf);
};

#define to_mem_entry_attr(a) \
container_of(a, struct kgsl_mem_entry_attribute, attr)

#define __MEM_ENTRY_ATTR(_type, _name, _show) \
{ \
	.attr = { .name = __stringify(_name), .mode = 0444 }, \
	.memtype = _type, \
	.show = _show, \
}


#ifdef CONFIG_MSM_KGSL_GPU_USAGE
static ssize_t
gpubusy_show(struct kgsl_process_private *priv, int type, char *buf)
{
	char* tmp = buf;
	int i;

	tmp = (char*)((int)tmp + snprintf(tmp, PAGE_SIZE, "%lld %lld", priv->gputime.total, priv->gputime.busy));
	for(i=0;i<KGSL_MAX_PWRLEVELS;i++)
	tmp = (char*)( (int)tmp + snprintf(tmp, PAGE_SIZE - (int)(tmp-buf), " %lld %lld",
				priv->gputime_in_state[i].total, priv->gputime_in_state[i].busy));
	tmp = (char*)((int)tmp + snprintf(tmp, PAGE_SIZE, "\n"));
	return (ssize_t)(tmp - buf);
}

static struct kgsl_mem_entry_attribute gpubusy = __MEM_ENTRY_ATTR(0, gpubusy, gpubusy_show);
#endif


struct mem_entry_stats {
	int memtype;
	struct kgsl_mem_entry_attribute attr;
	struct kgsl_mem_entry_attribute max_attr;
};


#define MEM_ENTRY_STAT(_type, _name) \
{ \
	.memtype = _type, \
	.attr = __MEM_ENTRY_ATTR(_type, _name, mem_entry_show), \
	.max_attr = __MEM_ENTRY_ATTR(_type, _name##_max, \
		mem_entry_max_show), \
}


static struct kgsl_process_private *
_get_priv_from_kobj(struct kobject *kobj)
{
	struct kgsl_process_private *private;
	unsigned long name;

	if (!kobj)
		return NULL;

	if (sscanf(kobj->name, "%ld", &name) != 1)
		return NULL;

	list_for_each_entry(private, &kgsl_driver.process_list, list) {
		if (private->pid == name)
			return private;
	}

	return NULL;
}


static ssize_t
mem_entry_show(struct kgsl_process_private *priv, int type, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", priv->stats[type].cur);
}


static ssize_t
mem_entry_max_show(struct kgsl_process_private *priv, int type, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", priv->stats[type].max);
}


static void mem_entry_sysfs_release(struct kobject *kobj)
{
}

static ssize_t mem_entry_sysfs_show(struct kobject *kobj,
	struct attribute *attr, char *buf)
{
	struct kgsl_mem_entry_attribute *pattr = to_mem_entry_attr(attr);
	struct kgsl_process_private *priv;
	ssize_t ret;

	mutex_lock(&kgsl_driver.process_mutex);
	priv = _get_priv_from_kobj(kobj);

	if (priv && pattr->show)
		ret = pattr->show(priv, pattr->memtype, buf);
	else
		ret = -EIO;

	mutex_unlock(&kgsl_driver.process_mutex);
	return ret;
}

static const struct sysfs_ops mem_entry_sysfs_ops = {
	.show = mem_entry_sysfs_show,
};

static struct kobj_type ktype_mem_entry = {
	.sysfs_ops = &mem_entry_sysfs_ops,
	.default_attrs = NULL,
	.release = mem_entry_sysfs_release
};

static struct mem_entry_stats mem_stats[] = {
	MEM_ENTRY_STAT(KGSL_MEM_ENTRY_KERNEL, kernel),
#ifdef CONFIG_ANDROID_PMEM
	MEM_ENTRY_STAT(KGSL_MEM_ENTRY_PMEM, pmem),
#endif
#ifdef CONFIG_ASHMEM
	MEM_ENTRY_STAT(KGSL_MEM_ENTRY_ASHMEM, ashmem),
#endif
	MEM_ENTRY_STAT(KGSL_MEM_ENTRY_USER, user),
#ifdef CONFIG_ION
	MEM_ENTRY_STAT(KGSL_MEM_ENTRY_ION, ion),
#endif
};

void
kgsl_process_uninit_sysfs(struct kgsl_process_private *private)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mem_stats); i++) {
		sysfs_remove_file(&private->kobj, &mem_stats[i].attr.attr);
		sysfs_remove_file(&private->kobj,
			&mem_stats[i].max_attr.attr);
	}

#ifdef CONFIG_MSM_KGSL_GPU_USAGE
	sysfs_remove_file(&private->kobj, &gpubusy.attr);
#endif

	kobject_put(&private->kobj);
}

void
kgsl_process_init_sysfs(struct kgsl_process_private *private)
{
	unsigned char name[16];
	int i, ret;

	snprintf(name, sizeof(name), "%d", private->pid);

	if (kobject_init_and_add(&private->kobj, &ktype_mem_entry,
		kgsl_driver.prockobj, name))
		return;

	for (i = 0; i < ARRAY_SIZE(mem_stats); i++) {

		ret = sysfs_create_file(&private->kobj,
			&mem_stats[i].attr.attr);
		ret = sysfs_create_file(&private->kobj,
			&mem_stats[i].max_attr.attr);
	}
#ifdef CONFIG_MSM_KGSL_GPU_USAGE
ret = sysfs_create_file(&private->kobj, &gpubusy.attr);
#endif
}

static int kgsl_drv_memstat_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	unsigned int val = 0;

	if (!strncmp(attr->attr.name, "vmalloc", 7))
		val = kgsl_driver.stats.vmalloc;
	else if (!strncmp(attr->attr.name, "vmalloc_max", 11))
		val = kgsl_driver.stats.vmalloc_max;
	else if (!strncmp(attr->attr.name, "page_alloc", 10))
		val = kgsl_driver.stats.page_alloc;
	else if (!strncmp(attr->attr.name, "page_alloc_max", 14))
		val = kgsl_driver.stats.page_alloc_max;
	else if (!strncmp(attr->attr.name, "coherent", 8))
		val = kgsl_driver.stats.coherent;
	else if (!strncmp(attr->attr.name, "coherent_max", 12))
		val = kgsl_driver.stats.coherent_max;
	else if (!strncmp(attr->attr.name, "mapped", 6))
		val = kgsl_driver.stats.mapped;
	else if (!strncmp(attr->attr.name, "mapped_max", 10))
		val = kgsl_driver.stats.mapped_max;

	return snprintf(buf, PAGE_SIZE, "%u\n", val);
}

static int kgsl_drv_histogram_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int len = 0;
	int i;

	for (i = 0; i < 16; i++)
		len += snprintf(buf + len, PAGE_SIZE - len, "%d ",
			kgsl_driver.stats.histogram[i]);

	len += snprintf(buf + len, PAGE_SIZE - len, "\n");
	return len;
}

DEVICE_ATTR(vmalloc, 0444, kgsl_drv_memstat_show, NULL);
DEVICE_ATTR(vmalloc_max, 0444, kgsl_drv_memstat_show, NULL);
DEVICE_ATTR(page_alloc, 0444, kgsl_drv_memstat_show, NULL);
DEVICE_ATTR(page_alloc_max, 0444, kgsl_drv_memstat_show, NULL);
DEVICE_ATTR(coherent, 0444, kgsl_drv_memstat_show, NULL);
DEVICE_ATTR(coherent_max, 0444, kgsl_drv_memstat_show, NULL);
DEVICE_ATTR(mapped, 0444, kgsl_drv_memstat_show, NULL);
DEVICE_ATTR(mapped_max, 0444, kgsl_drv_memstat_show, NULL);
DEVICE_ATTR(histogram, 0444, kgsl_drv_histogram_show, NULL);

static const struct device_attribute *drv_attr_list[] = {
	&dev_attr_vmalloc,
	&dev_attr_vmalloc_max,
	&dev_attr_page_alloc,
	&dev_attr_page_alloc_max,
	&dev_attr_coherent,
	&dev_attr_coherent_max,
	&dev_attr_mapped,
	&dev_attr_mapped_max,
	&dev_attr_histogram,
	NULL
};

void
kgsl_sharedmem_uninit_sysfs(void)
{
	kgsl_remove_device_sysfs_files(&kgsl_driver.virtdev, drv_attr_list);
}

int
kgsl_sharedmem_init_sysfs(void)
{
	return kgsl_create_device_sysfs_files(&kgsl_driver.virtdev,
		drv_attr_list);
}

#ifdef CONFIG_OUTER_CACHE
static void _outer_cache_range_op(int op, unsigned long addr, size_t size)
{
	switch (op) {
	case KGSL_CACHE_OP_FLUSH:
		outer_flush_range(addr, addr + size);
		break;
	case KGSL_CACHE_OP_CLEAN:
		outer_clean_range(addr, addr + size);
		break;
	case KGSL_CACHE_OP_INV:
		outer_inv_range(addr, addr + size);
		break;
	}
}

static void outer_cache_range_op_sg(struct scatterlist *sg, int sglen, int op)
{
	struct scatterlist *s;
	int i;

	for_each_sg(sg, s, sglen, i) {
		unsigned int paddr = kgsl_get_sg_pa(s);
		_outer_cache_range_op(op, paddr, s->length);
	}
}

#else
static void outer_cache_range_op_sg(struct scatterlist *sg, int sglen, int op)
{
}
#endif

static int kgsl_ion_alloc_vmfault(struct kgsl_memdesc *memdesc,
				struct vm_area_struct *vma,
				struct vm_fault *vmf)
{
	unsigned long offset, pfn;
	int ret;

	offset = ((unsigned long) vmf->virtual_address - vma->vm_start) >> PAGE_SHIFT;

	pfn = (memdesc->sg[0].dma_address >> PAGE_SHIFT) + offset;
	ret = vm_insert_pfn(vma, (unsigned long) vmf->virtual_address, pfn);

	if (ret == -ENOMEM || ret == -EAGAIN)
		return VM_FAULT_OOM;
	else if (ret == -EFAULT)
		return VM_FAULT_SIGBUS;

	return 0;
}

static int kgsl_ion_alloc_vmflags(struct kgsl_memdesc *memdesc)
{
	return VM_RESERVED | VM_DONTEXPAND;
}

static void kgsl_ion_alloc_free(struct kgsl_memdesc *memdesc)
{
	kgsl_driver.stats.pre_alloc -= memdesc->size;
	if (memdesc->handle)
		ion_free(kgsl_client, memdesc->handle);

	if (memdesc->hostptr) {
		iounmap(memdesc->hostptr);
		kgsl_driver.stats.pre_alloc_kernel -= memdesc->size;
		kgsl_driver.stats.vmalloc -= memdesc->size;
	}

	if (memdesc->private)
		kgsl_process_sub_stats(memdesc->private, KGSL_MEM_ENTRY_PRE_ALLOC, memdesc->size);
}

static int kgsl_ion_alloc_map_kernel(struct kgsl_memdesc *memdesc)
{
	if (!memdesc->hostptr) {
		memdesc->hostptr = ioremap(memdesc->sg[0].dma_address, memdesc->sg[0].length);
		if (IS_ERR_OR_NULL(memdesc->hostptr)) {
			printk("[kgsl] ion ioremap failed\n");
			return -ENOMEM;
		}
		KGSL_STATS_ADD(memdesc->size, kgsl_driver.stats.vmalloc,
			kgsl_driver.stats.vmalloc_max);
		kgsl_driver.stats.pre_alloc_kernel += memdesc->size;
	}

	return 0;
}

static int kgsl_page_alloc_vmfault(struct kgsl_memdesc *memdesc,
				struct vm_area_struct *vma,
				struct vm_fault *vmf)
{
	int i, pgoff;
	struct scatterlist *s = memdesc->sg;
	unsigned int offset;

	offset = ((unsigned long) vmf->virtual_address - vma->vm_start);

	if (offset >= memdesc->size)
		return VM_FAULT_SIGBUS;

	pgoff = offset >> PAGE_SHIFT;


	for (i = 0; i < memdesc->sglen; i++) {
		int npages = s->length >> PAGE_SHIFT;

		if (pgoff < npages) {
			struct page *page = sg_page(s);

			page = nth_page(page, pgoff);

			get_page(page);
			vmf->page = page;

			return 0;
		}

		pgoff -= npages;
		s = sg_next(s);
	}

	return VM_FAULT_SIGBUS;
}

static int kgsl_page_alloc_vmflags(struct kgsl_memdesc *memdesc)
{
	return VM_RESERVED | VM_DONTEXPAND;
}

static void kgsl_page_alloc_free(struct kgsl_memdesc *memdesc)
{
	int i = 0, j, size;
	struct scatterlist *sg;
	int sglen = memdesc->sglen;

	kgsl_driver.stats.page_alloc -= memdesc->size;

	if (memdesc->hostptr) {
		vunmap(memdesc->hostptr);
		kgsl_driver.stats.vmalloc -= memdesc->size;
		kgsl_driver.stats.page_alloc_kernel -= memdesc->size;
	}
	if (memdesc->sg)
		for_each_sg(memdesc->sg, sg, sglen, i){
			if (sg->length == 0)
				break;
			size = 1 << get_order(sg->length);
			for (j = 0; j < size; j++)
				ClearPageKgsl(nth_page(sg_page(sg), j));
			__free_pages(sg_page(sg), get_order(sg->length));
		}
	if (memdesc->private)
		kgsl_process_sub_stats(memdesc->private, KGSL_MEM_ENTRY_PAGE_ALLOC, memdesc->size);
}

static int kgsl_contiguous_vmflags(struct kgsl_memdesc *memdesc)
{
	return VM_RESERVED | VM_IO | VM_PFNMAP | VM_DONTEXPAND;
}

static int kgsl_page_alloc_map_kernel(struct kgsl_memdesc *memdesc)
{
	if (!memdesc->hostptr) {
		pgprot_t page_prot = pgprot_writecombine(PAGE_KERNEL);
		struct page **pages = NULL;
		struct scatterlist *sg;
		int npages = PAGE_ALIGN(memdesc->size) >> PAGE_SHIFT;
		int sglen = memdesc->sglen;
		int i, count = 0;

		
		pages = vmalloc(npages * sizeof(struct page *));
		if (!pages) {
			KGSL_CORE_ERR("vmalloc(%d) failed\n",
				npages * sizeof(struct page *));
			return -ENOMEM;
		}

		for_each_sg(memdesc->sg, sg, sglen, i) {
			struct page *page = sg_page(sg);
			int j;

			for (j = 0; j < sg->length >> PAGE_SHIFT; j++)
				pages[count++] = page++;
		}


		memdesc->hostptr = vmap(pages, count,
					VM_IOREMAP, page_prot);
		KGSL_STATS_ADD(memdesc->size, kgsl_driver.stats.vmalloc,
				kgsl_driver.stats.vmalloc_max);
		kgsl_driver.stats.page_alloc_kernel += memdesc->size;
		vfree(pages);
	}
	if (!memdesc->hostptr)
		return -ENOMEM;

	return 0;
}

static int kgsl_contiguous_vmfault(struct kgsl_memdesc *memdesc,
				struct vm_area_struct *vma,
				struct vm_fault *vmf)
{
	unsigned long offset, pfn;
	int ret;

	offset = ((unsigned long) vmf->virtual_address - vma->vm_start) >>
		PAGE_SHIFT;

	pfn = (memdesc->physaddr >> PAGE_SHIFT) + offset;
	ret = vm_insert_pfn(vma, (unsigned long) vmf->virtual_address, pfn);

	if (ret == -ENOMEM || ret == -EAGAIN)
		return VM_FAULT_OOM;
	else if (ret == -EFAULT)
		return VM_FAULT_SIGBUS;

	return VM_FAULT_NOPAGE;
}

static void kgsl_ebimem_free(struct kgsl_memdesc *memdesc)

{
	kgsl_driver.stats.coherent -= memdesc->size;
	if (memdesc->hostptr)
		iounmap(memdesc->hostptr);

	free_contiguous_memory_by_paddr(memdesc->physaddr);
}

static int kgsl_ebimem_map_kernel(struct kgsl_memdesc *memdesc)
{
	if (!memdesc->hostptr) {
		memdesc->hostptr = ioremap(memdesc->physaddr, memdesc->size);
		if (!memdesc->hostptr) {
			KGSL_CORE_ERR("ioremap failed, addr:0x%p, size:0x%x\n",
				memdesc->hostptr, memdesc->size);
			return -ENOMEM;
		}
	}

	return 0;
}

static void kgsl_coherent_free(struct kgsl_memdesc *memdesc)
{
	kgsl_driver.stats.coherent -= memdesc->size;
	dma_free_coherent(NULL, memdesc->size,
			  memdesc->hostptr, memdesc->physaddr);
}

struct kgsl_memdesc_ops kgsl_page_alloc_ops = {
	.free = kgsl_page_alloc_free,
	.vmflags = kgsl_page_alloc_vmflags,
	.vmfault = kgsl_page_alloc_vmfault,
	.map_kernel_mem = kgsl_page_alloc_map_kernel,
};
EXPORT_SYMBOL(kgsl_page_alloc_ops);

struct kgsl_memdesc_ops kgsl_ion_alloc_ops = {
	.free = kgsl_ion_alloc_free,
	.vmflags = kgsl_ion_alloc_vmflags,
	.vmfault = kgsl_ion_alloc_vmfault,
	.map_kernel_mem = kgsl_ion_alloc_map_kernel,
};
EXPORT_SYMBOL(kgsl_ion_alloc_ops);

static struct kgsl_memdesc_ops kgsl_ebimem_ops = {
	.free = kgsl_ebimem_free,
	.vmflags = kgsl_contiguous_vmflags,
	.vmfault = kgsl_contiguous_vmfault,
	.map_kernel_mem = kgsl_ebimem_map_kernel,
};

static struct kgsl_memdesc_ops kgsl_coherent_ops = {
	.free = kgsl_coherent_free,
};

void kgsl_cache_range_op(struct kgsl_memdesc *memdesc, int op)
{

	void *addr = (memdesc->hostptr) ?
		memdesc->hostptr : (void *) memdesc->useraddr;

	int size = memdesc->size;

	if (addr !=  NULL) {
		switch (op) {
		case KGSL_CACHE_OP_FLUSH:
			dmac_flush_range(addr, addr + size);
			break;
		case KGSL_CACHE_OP_CLEAN:
			dmac_clean_range(addr, addr + size);
			break;
		case KGSL_CACHE_OP_INV:
			dmac_inv_range(addr, addr + size);
			break;
		}
	}
	outer_cache_range_op_sg(memdesc->sg, memdesc->sglen, op);
}
EXPORT_SYMBOL(kgsl_cache_range_op);

static int
_kgsl_sharedmem_page_alloc(struct kgsl_memdesc *memdesc,
			struct kgsl_pagetable *pagetable,
			size_t size)
{
	int pcount = 0, order, ret = 0;
	int j, len, page_size, sglen_alloc, sglen = 0;
	struct page **pages = NULL;
	pgprot_t page_prot = pgprot_writecombine(PAGE_KERNEL);
	void *ptr;
	unsigned int align;

	align = (memdesc->flags & KGSL_MEMALIGN_MASK) >> KGSL_MEMALIGN_SHIFT;

	page_size = (align >= ilog2(SZ_64K) && size >= SZ_64K)
			? SZ_64K : PAGE_SIZE;
	
	if (page_size != PAGE_SIZE)
		kgsl_memdesc_set_align(memdesc, ilog2(page_size));


	sglen_alloc = PAGE_ALIGN(size) >> PAGE_SHIFT;

	memdesc->pagetable = pagetable;
	memdesc->ops = &kgsl_page_alloc_ops;

	memdesc->sglen_alloc = sglen_alloc;
	memdesc->sg = kgsl_sg_alloc(memdesc->sglen_alloc);

	if (memdesc->sg == NULL) {
		ret = -ENOMEM;
		goto done;
	}


	if ((memdesc->sglen_alloc * sizeof(struct page *)) > PAGE_SIZE)
		pages = vmalloc(memdesc->sglen_alloc * sizeof(struct page *));
	else
		pages = kmalloc(PAGE_SIZE, GFP_KERNEL);

	if (pages == NULL) {
		ret = -ENOMEM;
		goto done;
	}

	kmemleak_not_leak(memdesc->sg);

	sg_init_table(memdesc->sg, memdesc->sglen_alloc);

	len = size;

	while (len > 0) {
		struct page *page;
		unsigned int gfp_mask = __GFP_HIGHMEM;
		int j;

		
		if (len < page_size)
			page_size = PAGE_SIZE;

		if (page_size != PAGE_SIZE)
			gfp_mask |= __GFP_COMP | __GFP_NORETRY |
				__GFP_NO_KSWAPD | __GFP_NOWARN;
		else
			gfp_mask |= GFP_KERNEL;

		page = alloc_pages(gfp_mask, get_order(page_size));

		if (page == NULL) {
			if (page_size != PAGE_SIZE) {
				page_size = PAGE_SIZE;
				continue;
			}
			memdesc->sglen = sglen;
			memdesc->size = (size - len);

			KGSL_CORE_ERR(
				"Out of memory: only allocated %dKB of %dKB requested\n",
				(size - len) >> 10, size >> 10);

			ret = -ENOMEM;
			goto done;
		}

		for (j = 0; j < page_size >> PAGE_SHIFT; j++) {
			pages[pcount++] = nth_page(page, j);
			SetPageKgsl(nth_page(page, j));
		}

		sg_set_page(&memdesc->sg[sglen++], page, page_size, 0);
		len -= page_size;
	}

	memdesc->sglen = sglen;
	memdesc->size = size;

	ptr = vmap(pages, pcount, VM_IOREMAP, page_prot);

	if (ptr != NULL) {
		memset(ptr, 0, memdesc->size);
		dmac_flush_range(ptr, ptr + memdesc->size);
		vunmap(ptr);
	} else {
		

		for (j = 0; j < pcount; j++) {
			ptr = kmap_atomic(pages[j]);
			memset(ptr, 0, PAGE_SIZE);
			dmac_flush_range(ptr, ptr + PAGE_SIZE);
			kunmap_atomic(ptr);
		}
	}

	outer_cache_range_op_sg(memdesc->sg, memdesc->sglen,
				KGSL_CACHE_OP_FLUSH);

	order = get_order(size);

	if (order < 16)
		kgsl_driver.stats.histogram[order]++;

done:
	KGSL_STATS_ADD(memdesc->size, kgsl_driver.stats.page_alloc,
	kgsl_driver.stats.page_alloc_max);
	if ((memdesc->sglen_alloc * sizeof(struct page *)) > PAGE_SIZE)
		vfree(pages);
	else
		kfree(pages);

	if (ret)
		kgsl_sharedmem_free(memdesc);

	return ret;
}

int
kgsl_sharedmem_page_alloc(struct kgsl_memdesc *memdesc,
		       struct kgsl_pagetable *pagetable, size_t size)
{
	int ret = 0;
	BUG_ON(size == 0);

	size = ALIGN(size, PAGE_SIZE * 2);

	ret =  _kgsl_sharedmem_page_alloc(memdesc, pagetable, size);
	if (!ret)
		ret = kgsl_page_alloc_map_kernel(memdesc);
	if (ret)
		kgsl_sharedmem_free(memdesc);
	return ret;
}
EXPORT_SYMBOL(kgsl_sharedmem_page_alloc);

int
kgsl_sharedmem_page_alloc_user(struct kgsl_memdesc *memdesc,
				struct kgsl_process_private *private,
			    struct kgsl_pagetable *pagetable,
			    size_t size)
{
	int ret = 0;
	ret = _kgsl_sharedmem_page_alloc(memdesc, pagetable, PAGE_ALIGN(size));
	if (ret == 0 && private)
		kgsl_process_add_stats(private, KGSL_MEM_ENTRY_PAGE_ALLOC, size);

	return ret;
}
EXPORT_SYMBOL(kgsl_sharedmem_page_alloc_user);

static int
_kgsl_sharedmem_ion_alloc(struct kgsl_memdesc *memdesc,
				struct kgsl_pagetable *pagetable,
				size_t size, unsigned int protflags)
{
	int order, ret = 0;
	void *ptr;
	struct ion_handle *handle = NULL;
	ion_phys_addr_t pa = 0;
	size_t len = 0;


	
		

	memdesc->size = size;
	memdesc->pagetable = pagetable;
	memdesc->ops = &kgsl_ion_alloc_ops;

	if (kgsl_client == NULL) {
		KGSL_CORE_ERR("kgsl_client is not initialized\n");
		ret = -ENOMEM;
		goto done;
	}

	handle = ion_alloc(kgsl_client, size, SZ_4K, 0x1 << ION_SF_HEAP_ID,0);
	if (IS_ERR_OR_NULL(handle)) {
		ret = -ENOMEM;
		goto done;
	}

	memdesc->handle = handle;

	if (ion_phys(kgsl_client, handle, &pa, &len)) {
		printk("kgsl: ion_phys() failed\n");
		ret = -ENOMEM;
		goto done;
	}

	ret = memdesc_sg_phys(memdesc, pa, memdesc->size);
	if (ret)
		goto done;

	
	


	ptr = ioremap(pa, memdesc->size);

	if (ptr != NULL) {
		memset(ptr, 0, memdesc->size);
		dmac_flush_range(ptr, ptr + memdesc->size);
		iounmap(ptr);
	}
	outer_cache_range_op_sg(memdesc->sg, memdesc->sglen, KGSL_CACHE_OP_FLUSH);

	if (ret) {
		printk("[kgsl] kgsl_mmu_map failed\n");
		ret = -ENOMEM;
		goto done;
	}

	order = get_order(size);

	if (order < 16)
		kgsl_driver.stats.histogram[order]++;

done:
	KGSL_STATS_ADD(size, kgsl_driver.stats.pre_alloc, kgsl_driver.stats.pre_alloc_max);
	if (ret)
		kgsl_sharedmem_free(memdesc);

	return ret;
}

int
kgsl_sharedmem_ion_alloc(struct kgsl_memdesc *memdesc,
				struct kgsl_pagetable *pagetable,
				size_t size)
{
	int ret;

	BUG_ON(size == 0);
	size = PAGE_ALIGN(size);

	ret = _kgsl_sharedmem_ion_alloc(memdesc, pagetable, size,
		GSL_PT_PAGE_RV | GSL_PT_PAGE_WV);
	if (!ret)
		ret = kgsl_ion_alloc_map_kernel(memdesc);

	if (ret)
		kgsl_sharedmem_free(memdesc);

	return ret;
}
EXPORT_SYMBOL(kgsl_sharedmem_ion_alloc);

int
kgsl_sharedmem_ion_alloc_user(struct kgsl_memdesc *memdesc,
				struct kgsl_process_private *private,
				struct kgsl_pagetable *pagetable,
				size_t size)
{
	unsigned int protflags;
	int ret = 0;

	BUG_ON(size == 0);

	size = PAGE_ALIGN(size);

	protflags = GSL_PT_PAGE_RV;
	if (!(memdesc->flags & KGSL_MEMFLAGS_GPUREADONLY))
		protflags |= GSL_PT_PAGE_WV;

	ret = _kgsl_sharedmem_ion_alloc(memdesc, pagetable, size,
		protflags);

	if (ret == 0 && private)
		kgsl_process_add_stats(private, KGSL_MEM_ENTRY_PRE_ALLOC, size);

	return ret;
}
EXPORT_SYMBOL(kgsl_sharedmem_ion_alloc_user);

void kgsl_sharedmem_init_ion(void)
{
	if (kgsl_client == NULL)
		kgsl_client = msm_ion_client_create(-1, "KGSL");
}
EXPORT_SYMBOL(kgsl_sharedmem_init_ion);


int
kgsl_sharedmem_alloc_coherent(struct kgsl_memdesc *memdesc, size_t size)
{
	int result = 0;

	size = ALIGN(size, PAGE_SIZE);

	memdesc->size = size;
	memdesc->ops = &kgsl_coherent_ops;

	memdesc->hostptr = dma_alloc_coherent(NULL, size, &memdesc->physaddr,
					      GFP_KERNEL);
	if (memdesc->hostptr == NULL) {
		KGSL_CORE_ERR("dma_alloc_coherent(%d) failed\n", size);
		result = -ENOMEM;
		goto err;
	}

	result = memdesc_sg_phys(memdesc, memdesc->physaddr, size);
	if (result)
		goto err;

	

	KGSL_STATS_ADD(size, kgsl_driver.stats.coherent,
		       kgsl_driver.stats.coherent_max);

err:
	if (result)
		kgsl_sharedmem_free(memdesc);

	return result;
}
EXPORT_SYMBOL(kgsl_sharedmem_alloc_coherent);

void kgsl_sharedmem_free(struct kgsl_memdesc *memdesc)
{
	if (memdesc == NULL || memdesc->size == 0)
		return;

	if (memdesc->gpuaddr) {
		kgsl_mmu_unmap(memdesc->pagetable, memdesc);
		kgsl_mmu_put_gpuaddr(memdesc->pagetable, memdesc);
	}

	if (memdesc->ops && memdesc->ops->free)
		memdesc->ops->free(memdesc);

	kgsl_sg_free(memdesc->sg, memdesc->sglen_alloc);

	memset(memdesc, 0, sizeof(*memdesc));
}
EXPORT_SYMBOL(kgsl_sharedmem_free);

static int
_kgsl_sharedmem_ebimem(struct kgsl_memdesc *memdesc,
			struct kgsl_pagetable *pagetable, size_t size)
{
	int result = 0;

	memdesc->size = size;
	memdesc->pagetable = pagetable;
	memdesc->ops = &kgsl_ebimem_ops;
	memdesc->physaddr = allocate_contiguous_ebi_nomap(size, SZ_8K);

	if (memdesc->physaddr == 0) {
		KGSL_CORE_ERR("allocate_contiguous_ebi_nomap(%d) failed\n",
			size);
		return -ENOMEM;
	}

	result = memdesc_sg_phys(memdesc, memdesc->physaddr, size);

	if (result)
		goto err;

	KGSL_STATS_ADD(size, kgsl_driver.stats.coherent,
		kgsl_driver.stats.coherent_max);

err:
	if (result)
		kgsl_sharedmem_free(memdesc);

	return result;
}

int
kgsl_sharedmem_ebimem_user(struct kgsl_memdesc *memdesc,
			struct kgsl_pagetable *pagetable,
			size_t size)
{
	size = ALIGN(size, PAGE_SIZE);
	return _kgsl_sharedmem_ebimem(memdesc, pagetable, size);
}
EXPORT_SYMBOL(kgsl_sharedmem_ebimem_user);

int
kgsl_sharedmem_ebimem(struct kgsl_memdesc *memdesc,
		struct kgsl_pagetable *pagetable, size_t size)
{
	int result;
	size = ALIGN(size, 8192);
	result = _kgsl_sharedmem_ebimem(memdesc, pagetable, size);

	if (result)
		return result;

	memdesc->hostptr = ioremap(memdesc->physaddr, size);

	if (memdesc->hostptr == NULL) {
		KGSL_CORE_ERR("ioremap failed\n");
		kgsl_sharedmem_free(memdesc);
		return -ENOMEM;
	}

	return 0;
}
EXPORT_SYMBOL(kgsl_sharedmem_ebimem);

int
kgsl_sharedmem_readl(const struct kgsl_memdesc *memdesc,
			uint32_t *dst,
			unsigned int offsetbytes)
{
	uint32_t *src;
	BUG_ON(memdesc == NULL || memdesc->hostptr == NULL || dst == NULL);
	WARN_ON(offsetbytes % sizeof(uint32_t) != 0);
	if (offsetbytes % sizeof(uint32_t) != 0)
		return -EINVAL;

	WARN_ON(offsetbytes + sizeof(uint32_t) > memdesc->size);
	if (offsetbytes + sizeof(uint32_t) > memdesc->size)
		return -ERANGE;
	src = (uint32_t *)(memdesc->hostptr + offsetbytes);
	*dst = *src;
	return 0;
}
EXPORT_SYMBOL(kgsl_sharedmem_readl);

int
kgsl_sharedmem_writel(const struct kgsl_memdesc *memdesc,
			unsigned int offsetbytes,
			uint32_t src)
{
	uint32_t *dst;
	BUG_ON(memdesc == NULL || memdesc->hostptr == NULL);
	WARN_ON(offsetbytes % sizeof(uint32_t) != 0);
	if (offsetbytes % sizeof(uint32_t) != 0)
		return -EINVAL;

	WARN_ON(offsetbytes + sizeof(uint32_t) > memdesc->size);
	if (offsetbytes + sizeof(uint32_t) > memdesc->size)
		return -ERANGE;
	kgsl_cffdump_setmem(memdesc->gpuaddr + offsetbytes,
		src, sizeof(uint32_t));
	dst = (uint32_t *)(memdesc->hostptr + offsetbytes);
	*dst = src;
	return 0;
}
EXPORT_SYMBOL(kgsl_sharedmem_writel);

int
kgsl_sharedmem_set(const struct kgsl_memdesc *memdesc, unsigned int offsetbytes,
			unsigned int value, unsigned int sizebytes)
{
	BUG_ON(memdesc == NULL || memdesc->hostptr == NULL);
	BUG_ON(offsetbytes + sizebytes > memdesc->size);

	kgsl_cffdump_setmem(memdesc->gpuaddr + offsetbytes, value,
			    sizebytes);
	memset(memdesc->hostptr + offsetbytes, value, sizebytes);
	return 0;
}
EXPORT_SYMBOL(kgsl_sharedmem_set);

int
kgsl_sharedmem_map_vma(struct vm_area_struct *vma,
			const struct kgsl_memdesc *memdesc)
{
	unsigned long addr = vma->vm_start;
	unsigned long size = vma->vm_end - vma->vm_start;
	int ret, i = 0;

	if (!memdesc->sg || (size != memdesc->size) ||
		(memdesc->sglen != (size / PAGE_SIZE)))
		return -EINVAL;

	for (; addr < vma->vm_end; addr += PAGE_SIZE, i++) {
		ret = vm_insert_page(vma, addr, sg_page(&memdesc->sg[i]));
		if (ret)
			return ret;
	}
	return 0;
}
EXPORT_SYMBOL(kgsl_sharedmem_map_vma);

static const char * const memtype_str[] = {
	[KGSL_MEMTYPE_OBJECTANY] = "any(0)",
	[KGSL_MEMTYPE_FRAMEBUFFER] = "framebuffer",
	[KGSL_MEMTYPE_RENDERBUFFER] = "renderbuffer",
	[KGSL_MEMTYPE_ARRAYBUFFER] = "arraybuffer",
	[KGSL_MEMTYPE_ELEMENTARRAYBUFFER] = "elementarraybuffer",
	[KGSL_MEMTYPE_VERTEXARRAYBUFFER] = "vertexarraybuffer",
	[KGSL_MEMTYPE_TEXTURE] = "texture",
	[KGSL_MEMTYPE_SURFACE] = "surface",
	[KGSL_MEMTYPE_EGL_SURFACE] = "egl_surface",
	[KGSL_MEMTYPE_GL] = "gl",
	[KGSL_MEMTYPE_CL] = "cl",
	[KGSL_MEMTYPE_CL_BUFFER_MAP] = "cl_buffer_map",
	[KGSL_MEMTYPE_CL_BUFFER_NOMAP] = "cl_buffer_nomap",
	[KGSL_MEMTYPE_CL_IMAGE_MAP] = "cl_image_map",
	[KGSL_MEMTYPE_CL_IMAGE_NOMAP] = "cl_image_nomap",
	[KGSL_MEMTYPE_CL_KERNEL_STACK] = "cl_kernel_stack",
	[KGSL_MEMTYPE_COMMAND] = "command",
	[KGSL_MEMTYPE_2D] = "2d",
	[KGSL_MEMTYPE_EGL_IMAGE] = "egl_image",
	[KGSL_MEMTYPE_EGL_SHADOW] = "egl_shadow",
	[KGSL_MEMTYPE_MULTISAMPLE] = "egl_multisample",
	
};

void kgsl_get_memory_usage(char *name, size_t name_size, unsigned int memflags)
{
	unsigned char type;

	type = (memflags & KGSL_MEMTYPE_MASK) >> KGSL_MEMTYPE_SHIFT;
	if (type == KGSL_MEMTYPE_KERNEL)
		strlcpy(name, "kernel", name_size);
	else if (type < ARRAY_SIZE(memtype_str) && memtype_str[type] != NULL)
		strlcpy(name, memtype_str[type], name_size);
	else
		snprintf(name, name_size, "unknown(%3d)", type);
}
EXPORT_SYMBOL(kgsl_get_memory_usage);
