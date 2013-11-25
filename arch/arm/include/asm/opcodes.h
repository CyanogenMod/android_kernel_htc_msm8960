/*
 *  arch/arm/include/asm/opcodes.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARM_OPCODES_H
#define __ASM_ARM_OPCODES_H

#ifndef __ASSEMBLY__
extern asmlinkage unsigned int arm_check_condition(u32 opcode, u32 psr);
#endif

#define ARM_OPCODE_CONDTEST_FAIL   0
#define ARM_OPCODE_CONDTEST_PASS   1
#define ARM_OPCODE_CONDTEST_UNCOND 2



#ifndef __ASSEMBLY__

#include <linux/types.h>
#include <linux/swab.h>

#ifdef CONFIG_CPU_ENDIAN_BE8
#define __opcode_to_mem_arm(x) swab32(x)
#define __opcode_to_mem_thumb16(x) swab16(x)
#define __opcode_to_mem_thumb32(x) swahb32(x)
#else
#define __opcode_to_mem_arm(x) ((u32)(x))
#define __opcode_to_mem_thumb16(x) ((u16)(x))
#define __opcode_to_mem_thumb32(x) swahw32(x)
#endif

#define __mem_to_opcode_arm(x) __opcode_to_mem_arm(x)
#define __mem_to_opcode_thumb16(x) __opcode_to_mem_thumb16(x)
#define __mem_to_opcode_thumb32(x) __opcode_to_mem_thumb32(x)


#define __opcode_is_thumb32(x) ((u32)(x) >= 0xE8000000UL)
#define __opcode_is_thumb16(x) ((u32)(x) < 0xE800UL)

#define __opcode_thumb32_first(x) ((u16)((x) >> 16))
#define __opcode_thumb32_second(x) ((u16)(x))
#define __opcode_thumb32_compose(first, second) \
	(((u32)(u16)(first) << 16) | (u32)(u16)(second))

#endif 

#endif 
