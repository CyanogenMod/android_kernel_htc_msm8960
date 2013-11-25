/*
 *  arch/arm/include/asm/mach/arch.h
 *
 *  Copyright (C) 2000 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASSEMBLY__

struct tag;
struct meminfo;
struct sys_timer;
struct pt_regs;

struct machine_desc {
	unsigned int		nr;		
	const char		*name;		
	unsigned long		atag_offset;	
	const char *const 	*dt_compat;	

	unsigned int		nr_irqs;	

#ifdef CONFIG_ZONE_DMA
	unsigned long		dma_zone_size;	
#endif

	unsigned int		video_start;	
	unsigned int		video_end;	

	unsigned char		reserve_lp0 :1;	
	unsigned char		reserve_lp1 :1;	
	unsigned char		reserve_lp2 :1;	
	char			restart_mode;	
	void			(*fixup)(struct tag *, char **,
					 struct meminfo *);
	void			(*reserve)(void);
	void			(*map_io)(void);
	void			(*init_very_early)(void);
	void			(*init_early)(void);
	void			(*init_irq)(void);
	struct sys_timer	*timer;		
	void			(*init_machine)(void);
#ifdef CONFIG_MULTI_IRQ_HANDLER
	void			(*handle_irq)(struct pt_regs *);
#endif
	void			(*restart)(char, const char *);
};

extern struct machine_desc *machine_desc;

extern struct machine_desc __arch_info_begin[], __arch_info_end[];
#define for_each_machine_desc(p)			\
	for (p = __arch_info_begin; p < __arch_info_end; p++)

#define MACHINE_START(_type,_name)			\
static const struct machine_desc __mach_desc_##_type	\
 __used							\
 __attribute__((__section__(".arch.info.init"))) = {	\
	.nr		= MACH_TYPE_##_type,		\
	.name		= _name,

#define MACHINE_END				\
};

#define DT_MACHINE_START(_name, _namestr)		\
static const struct machine_desc __mach_desc_##_name	\
 __used							\
 __attribute__((__section__(".arch.info.init"))) = {	\
	.nr		= ~0,				\
	.name		= _namestr,

#endif
