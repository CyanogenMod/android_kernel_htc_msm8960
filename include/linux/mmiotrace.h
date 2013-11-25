#ifndef _LINUX_MMIOTRACE_H
#define _LINUX_MMIOTRACE_H

#include <linux/types.h>
#include <linux/list.h>

struct kmmio_probe;
struct pt_regs;

typedef void (*kmmio_pre_handler_t)(struct kmmio_probe *,
				struct pt_regs *, unsigned long addr);
typedef void (*kmmio_post_handler_t)(struct kmmio_probe *,
				unsigned long condition, struct pt_regs *);

struct kmmio_probe {
	
	struct list_head	list;
	
	unsigned long		addr;
	
	unsigned long		len;
	
	kmmio_pre_handler_t	pre_handler;
	
	kmmio_post_handler_t	post_handler;
	void			*private;
};

extern unsigned int kmmio_count;

extern int register_kmmio_probe(struct kmmio_probe *p);
extern void unregister_kmmio_probe(struct kmmio_probe *p);
extern int kmmio_init(void);
extern void kmmio_cleanup(void);

#ifdef CONFIG_MMIOTRACE
static inline int is_kmmio_active(void)
{
	return kmmio_count;
}

extern int kmmio_handler(struct pt_regs *regs, unsigned long addr);

extern void mmiotrace_ioremap(resource_size_t offset, unsigned long size,
							void __iomem *addr);
extern void mmiotrace_iounmap(volatile void __iomem *addr);

extern __printf(1, 2) int mmiotrace_printk(const char *fmt, ...);
#else 
static inline int is_kmmio_active(void)
{
	return 0;
}

static inline int kmmio_handler(struct pt_regs *regs, unsigned long addr)
{
	return 0;
}

static inline void mmiotrace_ioremap(resource_size_t offset,
					unsigned long size, void __iomem *addr)
{
}

static inline void mmiotrace_iounmap(volatile void __iomem *addr)
{
}

static inline __printf(1, 2) int mmiotrace_printk(const char *fmt, ...)
{
	return 0;
}
#endif 

enum mm_io_opcode {
	MMIO_READ	= 0x1,	
	MMIO_WRITE	= 0x2,	
	MMIO_PROBE	= 0x3,	
	MMIO_UNPROBE	= 0x4,	
	MMIO_UNKNOWN_OP = 0x5,	
};

struct mmiotrace_rw {
	resource_size_t	phys;	
	unsigned long	value;
	unsigned long	pc;	
	int		map_id;
	unsigned char	opcode;	
	unsigned char	width;	
};

struct mmiotrace_map {
	resource_size_t	phys;	
	unsigned long	virt;	
	unsigned long	len;	
	int		map_id;
	unsigned char	opcode;	
};

extern void enable_mmiotrace(void);
extern void disable_mmiotrace(void);
extern void mmio_trace_rw(struct mmiotrace_rw *rw);
extern void mmio_trace_mapping(struct mmiotrace_map *map);
extern int mmio_trace_printk(const char *fmt, va_list args);

#endif 
