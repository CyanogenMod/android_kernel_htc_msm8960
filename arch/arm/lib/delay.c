/*
 *  Originally from linux/arch/arm/lib/delay.S
 *
 *  Copyright (C) 1995, 1996 Russell King
 *  Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *  Copyright (C) 1993 Linus Torvalds
 *  Copyright (C) 1997 Martin Mares <mj@atrey.karlin.mff.cuni.cz>
 *  Copyright (C) 2005-2006 Atmel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/timex.h>

void delay_loop(unsigned long loops)
{
	asm volatile(
	"1:	subs %0, %0, #1 \n"
	"	bhi 1b		\n"
	: 
	: "r" (loops)
	);
}

#ifdef ARCH_HAS_READ_CURRENT_TIMER
void read_current_timer_delay_loop(unsigned long loops)
{
	unsigned long bclock, now;

	read_current_timer(&bclock);
	do {
		read_current_timer(&now);
	} while ((now - bclock) < loops);
}
#endif

static void (*delay_fn)(unsigned long) = delay_loop;

void set_delay_fn(void (*fn)(unsigned long))
{
	delay_fn = fn;
}

void __delay(unsigned long loops)
{
	delay_fn(loops);
}
EXPORT_SYMBOL(__delay);

void __const_udelay(unsigned long xloops)
{
	unsigned long lpj;
	unsigned long loops;

	xloops >>= 14;			
	lpj = loops_per_jiffy >> 10;	
	loops = lpj * xloops;		
	loops >>= 6;			

	if (loops)
		__delay(loops);
}
EXPORT_SYMBOL(__const_udelay);

void __udelay(unsigned long usecs)
{
	__const_udelay(usecs * ((2199023UL*HZ)>>11));
}
EXPORT_SYMBOL(__udelay);
