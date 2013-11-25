/* arch/arm/include/asm/asm_code_footprint.h
 *
 * Copyright (C) 2013 HTC, Inc.
 * Author: jerry_white <jerry_white@htc.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * asm_code_footprint() support.
 * The footprint values are PC Address and Instruction.
 * The footprint values are saved in asm_code_footrpint local stack.
 *
 * Footprint format:
 *   AC AC AC AC <PC ADDR> <ARM CODE> CA CA CA CA
 */

#ifndef __ASM_CODE_FOOTPRINT_H__
#define __ASM_CODE_FOOTPRINT_H__




#ifdef __ASSEMBLY__
	#ifdef CONFIG_ASM_CODE_FOOTPRINT
	#define ASM_CODE_FOOTPRINT      bl asm_code_footprint
	#else
	#define ASM_CODE_FOOTPRINT
	#endif
#else
	#ifdef CONFIG_ASM_CODE_FOOTPRINT
	extern void asm_code_footprint(void);
	#else
	static inline void asm_code_footprint(void){};
	#endif
#endif

#endif 
