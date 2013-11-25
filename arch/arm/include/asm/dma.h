#ifndef __ASM_ARM_DMA_H
#define __ASM_ARM_DMA_H

#ifndef CONFIG_ZONE_DMA
#define MAX_DMA_ADDRESS	0xffffffffUL
#else
#define MAX_DMA_ADDRESS	({ \
	extern unsigned long arm_dma_zone_size; \
	arm_dma_zone_size ? \
		(PAGE_OFFSET + arm_dma_zone_size) : 0xffffffffUL; })
#endif

#ifdef CONFIG_ISA_DMA_API
/*
 * This is used to support drivers written for the x86 ISA DMA API.
 * It should not be re-used except for that purpose.
 */
#include <linux/spinlock.h>
#include <asm/scatterlist.h>

#include <mach/isa-dma.h>

#define DMA_MODE_MASK	 0xcc

#define DMA_MODE_READ	 0x44
#define DMA_MODE_WRITE	 0x48
#define DMA_MODE_CASCADE 0xc0
#define DMA_AUTOINIT	 0x10

extern raw_spinlock_t  dma_spin_lock;

static inline unsigned long claim_dma_lock(void)
{
	unsigned long flags;
	raw_spin_lock_irqsave(&dma_spin_lock, flags);
	return flags;
}

static inline void release_dma_lock(unsigned long flags)
{
	raw_spin_unlock_irqrestore(&dma_spin_lock, flags);
}

#define clear_dma_ff(chan)

extern void set_dma_page(unsigned int chan, char pagenr);

extern int  request_dma(unsigned int chan, const char * device_id);

extern void free_dma(unsigned int chan);

extern void enable_dma(unsigned int chan);

extern void disable_dma(unsigned int chan);

extern int dma_channel_active(unsigned int chan);

extern void set_dma_sg(unsigned int chan, struct scatterlist *sg, int nr_sg);

extern void __set_dma_addr(unsigned int chan, void *addr);
#define set_dma_addr(chan, addr)				\
	__set_dma_addr(chan, bus_to_virt(addr))

extern void set_dma_count(unsigned int chan, unsigned long count);

extern void set_dma_mode(unsigned int chan, unsigned int mode);

extern void set_dma_speed(unsigned int chan, int cycle_ns);

extern int  get_dma_residue(unsigned int chan);

#ifndef NO_DMA
#define NO_DMA	255
#endif

#endif 

#ifdef CONFIG_PCI
extern int isa_dma_bridge_buggy;
#else
#define isa_dma_bridge_buggy    (0)
#endif

#endif 
