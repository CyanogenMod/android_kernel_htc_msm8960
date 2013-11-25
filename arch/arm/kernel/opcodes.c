/*
 *  linux/arch/arm/kernel/opcodes.c
 *
 *  A32 condition code lookup feature moved from nwfpe/fpopcode.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <asm/opcodes.h>

#define ARM_OPCODE_CONDITION_UNCOND 0xf

static const unsigned short cc_map[16] = {
	0xF0F0,			
	0x0F0F,			
	0xCCCC,			
	0x3333,			
	0xFF00,			
	0x00FF,			
	0xAAAA,			
	0x5555,			
	0x0C0C,			
	0xF3F3,			
	0xAA55,			
	0x55AA,			
	0x0A05,			
	0xF5FA,			
	0xFFFF,			
	0			
};

asmlinkage unsigned int arm_check_condition(u32 opcode, u32 psr)
{
	u32 cc_bits  = opcode >> 28;
	u32 psr_cond = psr >> 28;
	unsigned int ret;

	if (cc_bits != ARM_OPCODE_CONDITION_UNCOND) {
		if ((cc_map[cc_bits] >> (psr_cond)) & 1)
			ret = ARM_OPCODE_CONDTEST_PASS;
		else
			ret = ARM_OPCODE_CONDTEST_FAIL;
	} else {
		ret = ARM_OPCODE_CONDTEST_UNCOND;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(arm_check_condition);
