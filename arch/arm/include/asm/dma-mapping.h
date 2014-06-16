#ifndef ASMARM_DMA_MAPPING_H
#define ASMARM_DMA_MAPPING_H

#ifdef __KERNEL__

#include <linux/mm_types.h>
#include <linux/scatterlist.h>
#include <linux/dma-attrs.h>
#include <linux/dma-debug.h>

#include <asm-generic/dma-coherent.h>
#include <asm/memory.h>

#define DMA_ERROR_CODE	(~0)
extern struct dma_map_ops arm_dma_ops;

static inline struct dma_map_ops *get_dma_ops(struct device *dev)
{
	if (dev && dev->archdata.dma_ops)
		return dev->archdata.dma_ops;
	return &arm_dma_ops;
}

static inline void set_dma_ops(struct device *dev, struct dma_map_ops *ops)
{
	BUG_ON(!dev);
	dev->archdata.dma_ops = ops;
}

#include <asm-generic/dma-mapping-common.h>

static inline int dma_set_mask(struct device *dev, u64 mask)
{
	return get_dma_ops(dev)->set_dma_mask(dev, mask);
}

#ifdef __arch_page_to_dma
#error Please update to __arch_pfn_to_dma
#endif

#ifndef __arch_pfn_to_dma
static inline dma_addr_t pfn_to_dma(struct device *dev, unsigned long pfn)
{
	return (dma_addr_t)__pfn_to_bus(pfn);
}

static inline unsigned long dma_to_pfn(struct device *dev, dma_addr_t addr)
{
	return __bus_to_pfn(addr);
}

static inline void *dma_to_virt(struct device *dev, dma_addr_t addr)
{
	return (void *)__bus_to_virt((unsigned long)addr);
}

static inline dma_addr_t virt_to_dma(struct device *dev, void *addr)
{
	return (dma_addr_t)__virt_to_bus((unsigned long)(addr));
}
#else
static inline dma_addr_t pfn_to_dma(struct device *dev, unsigned long pfn)
{
	return __arch_pfn_to_dma(dev, pfn);
}

static inline unsigned long dma_to_pfn(struct device *dev, dma_addr_t addr)
{
	return __arch_dma_to_pfn(dev, addr);
}

static inline void *dma_to_virt(struct device *dev, dma_addr_t addr)
{
	return __arch_dma_to_virt(dev, addr);
}

static inline dma_addr_t virt_to_dma(struct device *dev, void *addr)
{
	return __arch_virt_to_dma(dev, addr);
}
#endif

static inline int dma_mapping_error(struct device *dev, dma_addr_t dma_addr)
{
	return dma_addr == DMA_ERROR_CODE;
}

static inline void *dma_alloc_noncoherent(struct device *dev, size_t size,
		dma_addr_t *handle, gfp_t gfp)
{
	return NULL;
}

static inline void dma_free_noncoherent(struct device *dev, size_t size,
		void *cpu_addr, dma_addr_t handle)
{
}


static inline void dma_coherent_pre_ops(void)
{
#if COHERENT_IS_NORMAL == 1
	dmb();
#else
	if (arch_is_coherent())
		dmb();
	else
		barrier();
#endif
}
static inline void dma_coherent_post_ops(void)
{
#if COHERENT_IS_NORMAL == 1
	dmb();
#else
	if (arch_is_coherent())
		dmb();
	else
		barrier();
#endif
}

extern int dma_supported(struct device *dev, u64 mask);

extern void *arm_dma_alloc(struct device *dev, size_t size, dma_addr_t *handle,
			   gfp_t gfp, struct dma_attrs *attrs);

#define dma_alloc_coherent(d, s, h, f) dma_alloc_attrs(d, s, h, f, NULL)

static inline void *dma_alloc_attrs(struct device *dev, size_t size,
				       dma_addr_t *dma_handle, gfp_t flag,
				       struct dma_attrs *attrs)
{
	struct dma_map_ops *ops = get_dma_ops(dev);
	void *cpu_addr;
	BUG_ON(!ops);

	cpu_addr = ops->alloc(dev, size, dma_handle, flag, attrs);
	debug_dma_alloc_coherent(dev, size, *dma_handle, cpu_addr);
	return cpu_addr;
}

extern void arm_dma_free(struct device *dev, size_t size, void *cpu_addr,
			 dma_addr_t handle, struct dma_attrs *attrs);

#define dma_free_coherent(d, s, c, h) dma_free_attrs(d, s, c, h, NULL)

static inline void dma_free_attrs(struct device *dev, size_t size,
				     void *cpu_addr, dma_addr_t dma_handle,
				     struct dma_attrs *attrs)
{
	struct dma_map_ops *ops = get_dma_ops(dev);
	BUG_ON(!ops);

	debug_dma_free_coherent(dev, size, cpu_addr, dma_handle);
	ops->free(dev, size, cpu_addr, dma_handle, attrs);
}

extern int arm_dma_mmap(struct device *dev, struct vm_area_struct *vma,
			void *cpu_addr, dma_addr_t dma_addr, size_t size,
			struct dma_attrs *attrs);

#define dma_mmap_coherent(d, v, c, h, s) dma_mmap_attrs(d, v, c, h, s, NULL)

static inline int dma_mmap_attrs(struct device *dev, struct vm_area_struct *vma,
				  void *cpu_addr, dma_addr_t dma_addr,
				  size_t size, struct dma_attrs *attrs)
{
	struct dma_map_ops *ops = get_dma_ops(dev);
	BUG_ON(!ops);
	return ops->mmap(dev, vma, cpu_addr, dma_addr, size, attrs);
}

static inline void *dma_alloc_writecombine(struct device *dev, size_t size,
				       dma_addr_t *dma_handle, gfp_t flag)
{
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE, &attrs);
	return dma_alloc_attrs(dev, size, dma_handle, flag, &attrs);
}

static inline void dma_free_writecombine(struct device *dev, size_t size,
				     void *cpu_addr, dma_addr_t dma_handle)
{
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE, &attrs);
	return dma_free_attrs(dev, size, cpu_addr, dma_handle, &attrs);
}

static inline int dma_mmap_writecombine(struct device *dev, struct vm_area_struct *vma,
		      void *cpu_addr, dma_addr_t dma_addr, size_t size)
{
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_WRITE_COMBINE, &attrs);
	return dma_mmap_attrs(dev, vma, cpu_addr, dma_addr, size, &attrs);
}

static inline void *dma_alloc_stronglyordered(struct device *dev, size_t size,
				       dma_addr_t *dma_handle, gfp_t flag)
{
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_STRONGLY_ORDERED, &attrs);
	return dma_alloc_attrs(dev, size, dma_handle, flag, &attrs);
}

static inline void dma_free_stronglyordered(struct device *dev, size_t size,
				     void *cpu_addr, dma_addr_t dma_handle)
{
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_STRONGLY_ORDERED, &attrs);
	return dma_free_attrs(dev, size, cpu_addr, dma_handle, &attrs);
}

static inline int dma_mmap_stronglyordered(struct device *dev,
		struct vm_area_struct *vma, void *cpu_addr,
		dma_addr_t dma_addr, size_t size)
{
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_STRONGLY_ORDERED, &attrs);
	return dma_mmap_attrs(dev, vma, cpu_addr, dma_addr, size, &attrs);
}

static inline void *dma_alloc_nonconsistent(struct device *dev, size_t size,
				       dma_addr_t *dma_handle, gfp_t flag)
{
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_NON_CONSISTENT, &attrs);
	return dma_alloc_attrs(dev, size, dma_handle, flag, &attrs);
}

static inline void dma_free_nonconsistent(struct device *dev, size_t size,
				     void *cpu_addr, dma_addr_t dma_handle)
{
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_NON_CONSISTENT, &attrs);
	return dma_free_attrs(dev, size, cpu_addr, dma_handle, &attrs);
}

static inline int dma_mmap_nonconsistent(struct device *dev,
		struct vm_area_struct *vma, void *cpu_addr,
		dma_addr_t dma_addr, size_t size)
{
	DEFINE_DMA_ATTRS(attrs);
	dma_set_attr(DMA_ATTR_NON_CONSISTENT, &attrs);
	return dma_mmap_attrs(dev, vma, cpu_addr, dma_addr, size, &attrs);
}



extern void __init init_consistent_dma_size(unsigned long size);


extern int dmabounce_register_dev(struct device *, unsigned long,
		unsigned long, int (*)(struct device *, dma_addr_t, size_t));

extern void dmabounce_unregister_dev(struct device *);



/**
 * dma_cache_pre_ops - clean or invalidate cache before dma transfer is
 *                     initiated and perform a barrier operation.
 * @virtual_addr: A kernel logical or kernel virtual address
 * @size: size of buffer to map
 * @dir: DMA transfer direction
 *
 * Ensure that any data held in the cache is appropriately discarded
 * or written back.
 *
 */
static inline void dma_cache_pre_ops(void *virtual_addr,
		size_t size, enum dma_data_direction dir)
{
	extern void ___dma_single_cpu_to_dev(const void *, size_t,
		enum dma_data_direction);

	BUG_ON(!valid_dma_direction(dir));

	if (!arch_is_coherent())
		___dma_single_cpu_to_dev(virtual_addr, size, dir);
}

/**
 * dma_cache_post_ops - clean or invalidate cache after dma transfer is
 *                     initiated and perform a barrier operation.
 * @virtual_addr: A kernel logical or kernel virtual address
 * @size: size of buffer to map
 * @dir: DMA transfer direction
 *
 * Ensure that any data held in the cache is appropriately discarded
 * or written back.
 *
 */
static inline void dma_cache_post_ops(void *virtual_addr,
		size_t size, enum dma_data_direction dir)
{
	extern void ___dma_single_cpu_to_dev(const void *, size_t,
		enum dma_data_direction);

	BUG_ON(!valid_dma_direction(dir));

	if (arch_has_speculative_dfetch() && !arch_is_coherent()
	 && dir != DMA_TO_DEVICE)
		___dma_single_cpu_to_dev(virtual_addr,
					 size, DMA_FROM_DEVICE);
}
extern int arm_dma_map_sg(struct device *, struct scatterlist *, int,
		enum dma_data_direction, struct dma_attrs *attrs);
extern void arm_dma_unmap_sg(struct device *, struct scatterlist *, int,
		enum dma_data_direction, struct dma_attrs *attrs);
extern void arm_dma_sync_sg_for_cpu(struct device *, struct scatterlist *, int,
		enum dma_data_direction);
extern void arm_dma_sync_sg_for_device(struct device *, struct scatterlist *, int,
		enum dma_data_direction);

#endif 
#endif
