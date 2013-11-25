#ifndef ASMARM_DMA_MAPPING_H
#define ASMARM_DMA_MAPPING_H

#ifdef __KERNEL__

#include <linux/mm_types.h>
#include <linux/scatterlist.h>
#include <linux/dma-debug.h>

#include <asm-generic/dma-coherent.h>
#include <asm/memory.h>

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

static inline void __dma_single_cpu_to_dev(const void *kaddr, size_t size,
	enum dma_data_direction dir)
{
	extern void ___dma_single_cpu_to_dev(const void *, size_t,
		enum dma_data_direction);

	if (!arch_is_coherent())
		___dma_single_cpu_to_dev(kaddr, size, dir);
}

static inline void __dma_single_dev_to_cpu(const void *kaddr, size_t size,
	enum dma_data_direction dir)
{
	extern void ___dma_single_dev_to_cpu(const void *, size_t,
		enum dma_data_direction);

	if (!arch_is_coherent())
		___dma_single_dev_to_cpu(kaddr, size, dir);
}

static inline void __dma_page_cpu_to_dev(struct page *page, unsigned long off,
	size_t size, enum dma_data_direction dir)
{
	extern void ___dma_page_cpu_to_dev(struct page *, unsigned long,
		size_t, enum dma_data_direction);

	if (!arch_is_coherent())
		___dma_page_cpu_to_dev(page, off, size, dir);
}

static inline void __dma_page_dev_to_cpu(struct page *page, unsigned long off,
	size_t size, enum dma_data_direction dir)
{
	extern void ___dma_page_dev_to_cpu(struct page *, unsigned long,
		size_t, enum dma_data_direction);

	if (!arch_is_coherent())
		___dma_page_dev_to_cpu(page, off, size, dir);
}

extern int dma_supported(struct device *, u64);
extern int dma_set_mask(struct device *, u64);

static inline int dma_mapping_error(struct device *dev, dma_addr_t dma_addr)
{
	return dma_addr == ~0;
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

extern void *dma_alloc_coherent(struct device *, size_t, dma_addr_t *, gfp_t);

extern void dma_free_coherent(struct device *, size_t, void *, dma_addr_t);

int dma_mmap_coherent(struct device *, struct vm_area_struct *,
		void *, dma_addr_t, size_t);


extern void *dma_alloc_writecombine(struct device *, size_t, dma_addr_t *,
		gfp_t);

#define dma_free_writecombine(dev,size,cpu_addr,handle) \
	dma_free_coherent(dev,size,cpu_addr,handle)

int dma_mmap_writecombine(struct device *, struct vm_area_struct *,
		void *, dma_addr_t, size_t);

extern void __init init_consistent_dma_size(unsigned long size);


#ifdef CONFIG_DMABOUNCE

extern int dmabounce_register_dev(struct device *, unsigned long,
		unsigned long, int (*)(struct device *, dma_addr_t, size_t));

extern void dmabounce_unregister_dev(struct device *);

extern dma_addr_t __dma_map_page(struct device *, struct page *,
		unsigned long, size_t, enum dma_data_direction);
extern void __dma_unmap_page(struct device *, dma_addr_t, size_t,
		enum dma_data_direction);

int dmabounce_sync_for_cpu(struct device *, dma_addr_t, unsigned long,
		size_t, enum dma_data_direction);
int dmabounce_sync_for_device(struct device *, dma_addr_t, unsigned long,
		size_t, enum dma_data_direction);
#else
static inline int dmabounce_sync_for_cpu(struct device *d, dma_addr_t addr,
	unsigned long offset, size_t size, enum dma_data_direction dir)
{
	return 1;
}

static inline int dmabounce_sync_for_device(struct device *d, dma_addr_t addr,
	unsigned long offset, size_t size, enum dma_data_direction dir)
{
	return 1;
}


static inline dma_addr_t __dma_map_page(struct device *dev, struct page *page,
	     unsigned long offset, size_t size, enum dma_data_direction dir)
{
	__dma_page_cpu_to_dev(page, offset, size, dir);
	return pfn_to_dma(dev, page_to_pfn(page)) + offset;
}

static inline void __dma_unmap_page(struct device *dev, dma_addr_t handle,
		size_t size, enum dma_data_direction dir)
{
	__dma_page_dev_to_cpu(pfn_to_page(dma_to_pfn(dev, handle)),
		handle & ~PAGE_MASK, size, dir);
}
#endif 

/**
 * dma_map_single - map a single buffer for streaming DMA
 * @dev: valid struct device pointer, or NULL for ISA and EISA-like devices
 * @cpu_addr: CPU direct mapped address of buffer
 * @size: size of buffer to map
 * @dir: DMA transfer direction
 *
 * Ensure that any data held in the cache is appropriately discarded
 * or written back.
 *
 * The device owns this memory once this call has completed.  The CPU
 * can regain ownership by calling dma_unmap_single() or
 * dma_sync_single_for_cpu().
 */
static inline dma_addr_t dma_map_single(struct device *dev, void *cpu_addr,
		size_t size, enum dma_data_direction dir)
{
	unsigned long offset;
	struct page *page;
	dma_addr_t addr;

	BUG_ON(!virt_addr_valid(cpu_addr));
	BUG_ON(!virt_addr_valid(cpu_addr + size - 1));
	BUG_ON(!valid_dma_direction(dir));

	page = virt_to_page(cpu_addr);
	offset = (unsigned long)cpu_addr & ~PAGE_MASK;
	addr = __dma_map_page(dev, page, offset, size, dir);
	debug_dma_map_page(dev, page, offset, size, dir, addr, true);

	return addr;
}

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

/**
 * dma_map_page - map a portion of a page for streaming DMA
 * @dev: valid struct device pointer, or NULL for ISA and EISA-like devices
 * @page: page that buffer resides in
 * @offset: offset into page for start of buffer
 * @size: size of buffer to map
 * @dir: DMA transfer direction
 *
 * Ensure that any data held in the cache is appropriately discarded
 * or written back.
 *
 * The device owns this memory once this call has completed.  The CPU
 * can regain ownership by calling dma_unmap_page().
 */
static inline dma_addr_t dma_map_page(struct device *dev, struct page *page,
	     unsigned long offset, size_t size, enum dma_data_direction dir)
{
	dma_addr_t addr;

	BUG_ON(!valid_dma_direction(dir));

	addr = __dma_map_page(dev, page, offset, size, dir);
	debug_dma_map_page(dev, page, offset, size, dir, addr, false);

	return addr;
}

static inline void dma_unmap_single(struct device *dev, dma_addr_t handle,
		size_t size, enum dma_data_direction dir)
{
	debug_dma_unmap_page(dev, handle, size, dir, true);
	__dma_unmap_page(dev, handle, size, dir);
}

static inline void dma_unmap_page(struct device *dev, dma_addr_t handle,
		size_t size, enum dma_data_direction dir)
{
	debug_dma_unmap_page(dev, handle, size, dir, false);
	__dma_unmap_page(dev, handle, size, dir);
}

static inline void dma_sync_single_range_for_cpu(struct device *dev,
		dma_addr_t handle, unsigned long offset, size_t size,
		enum dma_data_direction dir)
{
	BUG_ON(!valid_dma_direction(dir));

	debug_dma_sync_single_for_cpu(dev, handle + offset, size, dir);

	if (!dmabounce_sync_for_cpu(dev, handle, offset, size, dir))
		return;

	__dma_single_dev_to_cpu(dma_to_virt(dev, handle) + offset, size, dir);
}

static inline void dma_sync_single_range_for_device(struct device *dev,
		dma_addr_t handle, unsigned long offset, size_t size,
		enum dma_data_direction dir)
{
	BUG_ON(!valid_dma_direction(dir));

	debug_dma_sync_single_for_device(dev, handle + offset, size, dir);

	if (!dmabounce_sync_for_device(dev, handle, offset, size, dir))
		return;

	__dma_single_cpu_to_dev(dma_to_virt(dev, handle) + offset, size, dir);
}

static inline void dma_sync_single_for_cpu(struct device *dev,
		dma_addr_t handle, size_t size, enum dma_data_direction dir)
{
	dma_sync_single_range_for_cpu(dev, handle, 0, size, dir);
}

static inline void dma_sync_single_for_device(struct device *dev,
		dma_addr_t handle, size_t size, enum dma_data_direction dir)
{
	dma_sync_single_range_for_device(dev, handle, 0, size, dir);
}

extern int dma_map_sg(struct device *, struct scatterlist *, int,
		enum dma_data_direction);
extern void dma_unmap_sg(struct device *, struct scatterlist *, int,
		enum dma_data_direction);
extern void dma_sync_sg_for_cpu(struct device *, struct scatterlist *, int,
		enum dma_data_direction);
extern void dma_sync_sg_for_device(struct device *, struct scatterlist *, int,
		enum dma_data_direction);


#endif 
#endif
