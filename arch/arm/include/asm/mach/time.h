/*
 * arch/arm/include/asm/mach/time.h
 *
 * Copyright (C) 2004 MontaVista Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ASM_ARM_MACH_TIME_H
#define __ASM_ARM_MACH_TIME_H

struct sys_timer {
	void			(*init)(void);
	void			(*suspend)(void);
	void			(*resume)(void);
#ifdef CONFIG_ARCH_USES_GETTIMEOFFSET
	unsigned long		(*offset)(void);
#endif
};

extern void timer_tick(void);

#endif
