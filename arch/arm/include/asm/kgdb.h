/*
 * ARM KGDB support
 *
 * Author: Deepak Saxena <dsaxena@mvista.com>
 *
 * Copyright (C) 2002 MontaVista Software Inc.
 *
 */

#ifndef __ARM_KGDB_H__
#define __ARM_KGDB_H__

#include <linux/ptrace.h>

#define BREAK_INSTR_SIZE	4
#define GDB_BREAKINST		0xef9f0001
#define KGDB_BREAKINST		0xe7ffdefe
#define KGDB_COMPILED_BREAK	0xe7ffdeff
#define CACHE_FLUSH_IS_SAFE	1

#ifndef	__ASSEMBLY__

static inline void arch_kgdb_breakpoint(void)
{
	asm(".word 0xe7ffdeff");
}

extern void kgdb_handle_bus_error(void);
extern int kgdb_fault_expected;

#endif 

#define _GP_REGS		16
#define _FP_REGS		8
#define _EXTRA_REGS		2
#define GDB_MAX_REGS		(_GP_REGS + (_FP_REGS * 3) + _EXTRA_REGS)
#define DBG_MAX_REG_NUM		(_GP_REGS + _FP_REGS + _EXTRA_REGS)

#define KGDB_MAX_NO_CPUS	1
#define BUFMAX			400
#define NUMREGBYTES		(DBG_MAX_REG_NUM << 2)
#define NUMCRITREGBYTES		(32 << 2)

#define _R0			0
#define _R1			1
#define _R2			2
#define _R3			3
#define _R4			4
#define _R5			5
#define _R6			6
#define _R7			7
#define _R8			8
#define _R9			9
#define _R10			10
#define _FP			11
#define _IP			12
#define _SPT			13
#define _LR			14
#define _PC			15
#define _CPSR			(GDB_MAX_REGS - 1)

#define CFI_END_FRAME(func)	__CFI_END_FRAME(_PC, _SPT, func)

#endif 
