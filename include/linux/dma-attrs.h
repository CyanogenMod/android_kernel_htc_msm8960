#ifndef _DMA_ATTR_H
#define _DMA_ATTR_H

#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/bug.h>

enum dma_attr {
	DMA_ATTR_WRITE_BARRIER,
	DMA_ATTR_WEAK_ORDERING,
	DMA_ATTR_WRITE_COMBINE,
	DMA_ATTR_NON_CONSISTENT,
	DMA_ATTR_MAX,
};

#define __DMA_ATTRS_LONGS BITS_TO_LONGS(DMA_ATTR_MAX)

struct dma_attrs {
	unsigned long flags[__DMA_ATTRS_LONGS];
};

#define DEFINE_DMA_ATTRS(x) 					\
	struct dma_attrs x = {					\
		.flags = { [0 ... __DMA_ATTRS_LONGS-1] = 0 },	\
	}

static inline void init_dma_attrs(struct dma_attrs *attrs)
{
	bitmap_zero(attrs->flags, __DMA_ATTRS_LONGS);
}

#ifdef CONFIG_HAVE_DMA_ATTRS
static inline void dma_set_attr(enum dma_attr attr, struct dma_attrs *attrs)
{
	if (attrs == NULL)
		return;
	BUG_ON(attr >= DMA_ATTR_MAX);
	__set_bit(attr, attrs->flags);
}

static inline int dma_get_attr(enum dma_attr attr, struct dma_attrs *attrs)
{
	if (attrs == NULL)
		return 0;
	BUG_ON(attr >= DMA_ATTR_MAX);
	return test_bit(attr, attrs->flags);
}
#else 
static inline void dma_set_attr(enum dma_attr attr, struct dma_attrs *attrs)
{
}

static inline int dma_get_attr(enum dma_attr attr, struct dma_attrs *attrs)
{
	return 0;
}
#endif 
#endif 
