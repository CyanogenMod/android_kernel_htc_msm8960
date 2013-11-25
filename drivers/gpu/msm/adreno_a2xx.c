/* Copyright (c) 2002,2007-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/sched.h>
#include <mach/socinfo.h>

#include "kgsl.h"
#include "kgsl_sharedmem.h"
#include "kgsl_cffdump.h"
#include "adreno.h"
#include "adreno_a2xx_trace.h"


const unsigned int a200_registers[] = {
	0x0000, 0x0002, 0x0004, 0x000B, 0x003B, 0x003D, 0x0040, 0x0044,
	0x0046, 0x0047, 0x01C0, 0x01C1, 0x01C3, 0x01C8, 0x01D5, 0x01D9,
	0x01DC, 0x01DD, 0x01EA, 0x01EA, 0x01EE, 0x01F3, 0x01F6, 0x01F7,
	0x01FC, 0x01FF, 0x0391, 0x0392, 0x039B, 0x039E, 0x03B2, 0x03B5,
	0x03B7, 0x03B7, 0x03F8, 0x03FB, 0x0440, 0x0440, 0x0443, 0x0444,
	0x044B, 0x044B, 0x044D, 0x044F, 0x0452, 0x0452, 0x0454, 0x045B,
	0x047F, 0x047F, 0x0578, 0x0587, 0x05C9, 0x05C9, 0x05D0, 0x05D0,
	0x0601, 0x0604, 0x0606, 0x0609, 0x060B, 0x060E, 0x0613, 0x0614,
	0x0A29, 0x0A2B, 0x0A2F, 0x0A31, 0x0A40, 0x0A43, 0x0A45, 0x0A45,
	0x0A4E, 0x0A4F, 0x0C2C, 0x0C2C, 0x0C30, 0x0C30, 0x0C38, 0x0C3C,
	0x0C40, 0x0C40, 0x0C44, 0x0C44, 0x0C80, 0x0C86, 0x0C88, 0x0C94,
	0x0C99, 0x0C9A, 0x0CA4, 0x0CA5, 0x0D00, 0x0D03, 0x0D06, 0x0D06,
	0x0D08, 0x0D0B, 0x0D34, 0x0D35, 0x0DAE, 0x0DC1, 0x0DC8, 0x0DD4,
	0x0DD8, 0x0DD9, 0x0E00, 0x0E00, 0x0E02, 0x0E04, 0x0E17, 0x0E1E,
	0x0EC0, 0x0EC9, 0x0ECB, 0x0ECC, 0x0ED0, 0x0ED0, 0x0ED4, 0x0ED7,
	0x0EE0, 0x0EE2, 0x0F01, 0x0F02, 0x0F0C, 0x0F0C, 0x0F0E, 0x0F12,
	0x0F26, 0x0F2A, 0x0F2C, 0x0F2C, 0x2000, 0x2002, 0x2006, 0x200F,
	0x2080, 0x2082, 0x2100, 0x2109, 0x210C, 0x2114, 0x2180, 0x2184,
	0x21F5, 0x21F7, 0x2200, 0x2208, 0x2280, 0x2283, 0x2293, 0x2294,
	0x2300, 0x2308, 0x2312, 0x2312, 0x2316, 0x231D, 0x2324, 0x2326,
	0x2380, 0x2383, 0x2400, 0x2402, 0x2406, 0x240F, 0x2480, 0x2482,
	0x2500, 0x2509, 0x250C, 0x2514, 0x2580, 0x2584, 0x25F5, 0x25F7,
	0x2600, 0x2608, 0x2680, 0x2683, 0x2693, 0x2694, 0x2700, 0x2708,
	0x2712, 0x2712, 0x2716, 0x271D, 0x2724, 0x2726, 0x2780, 0x2783,
	0x4000, 0x4003, 0x4800, 0x4805, 0x4900, 0x4900, 0x4908, 0x4908,
};

const unsigned int a220_registers[] = {
	0x0000, 0x0002, 0x0004, 0x000B, 0x003B, 0x003D, 0x0040, 0x0044,
	0x0046, 0x0047, 0x01C0, 0x01C1, 0x01C3, 0x01C8, 0x01D5, 0x01D9,
	0x01DC, 0x01DD, 0x01EA, 0x01EA, 0x01EE, 0x01F3, 0x01F6, 0x01F7,
	0x01FC, 0x01FF, 0x0391, 0x0392, 0x039B, 0x039E, 0x03B2, 0x03B5,
	0x03B7, 0x03B7, 0x03F8, 0x03FB, 0x0440, 0x0440, 0x0443, 0x0444,
	0x044B, 0x044B, 0x044D, 0x044F, 0x0452, 0x0452, 0x0454, 0x045B,
	0x047F, 0x047F, 0x0578, 0x0587, 0x05C9, 0x05C9, 0x05D0, 0x05D0,
	0x0601, 0x0604, 0x0606, 0x0609, 0x060B, 0x060E, 0x0613, 0x0614,
	0x0A29, 0x0A2B, 0x0A2F, 0x0A31, 0x0A40, 0x0A40, 0x0A42, 0x0A43,
	0x0A45, 0x0A45, 0x0A4E, 0x0A4F, 0x0C30, 0x0C30, 0x0C38, 0x0C39,
	0x0C3C, 0x0C3C, 0x0C80, 0x0C81, 0x0C88, 0x0C93, 0x0D00, 0x0D03,
	0x0D05, 0x0D06, 0x0D08, 0x0D0B, 0x0D34, 0x0D35, 0x0DAE, 0x0DC1,
	0x0DC8, 0x0DD4, 0x0DD8, 0x0DD9, 0x0E00, 0x0E00, 0x0E02, 0x0E04,
	0x0E17, 0x0E1E, 0x0EC0, 0x0EC9, 0x0ECB, 0x0ECC, 0x0ED0, 0x0ED0,
	0x0ED4, 0x0ED7, 0x0EE0, 0x0EE2, 0x0F01, 0x0F02, 0x2000, 0x2002,
	0x2006, 0x200F, 0x2080, 0x2082, 0x2100, 0x2102, 0x2104, 0x2109,
	0x210C, 0x2114, 0x2180, 0x2184, 0x21F5, 0x21F7, 0x2200, 0x2202,
	0x2204, 0x2204, 0x2208, 0x2208, 0x2280, 0x2282, 0x2294, 0x2294,
	0x2300, 0x2308, 0x2309, 0x230A, 0x2312, 0x2312, 0x2316, 0x2316,
	0x2318, 0x231D, 0x2324, 0x2326, 0x2380, 0x2383, 0x2400, 0x2402,
	0x2406, 0x240F, 0x2480, 0x2482, 0x2500, 0x2502, 0x2504, 0x2509,
	0x250C, 0x2514, 0x2580, 0x2584, 0x25F5, 0x25F7, 0x2600, 0x2602,
	0x2604, 0x2606, 0x2608, 0x2608, 0x2680, 0x2682, 0x2694, 0x2694,
	0x2700, 0x2708, 0x2712, 0x2712, 0x2716, 0x2716, 0x2718, 0x271D,
	0x2724, 0x2726, 0x2780, 0x2783, 0x4000, 0x4003, 0x4800, 0x4805,
	0x4900, 0x4900, 0x4908, 0x4908,
};

const unsigned int a225_registers[] = {
	0x0000, 0x0002, 0x0004, 0x000B, 0x003B, 0x003D, 0x0040, 0x0044,
	0x0046, 0x0047, 0x013C, 0x013C, 0x0140, 0x014F, 0x01C0, 0x01C1,
	0x01C3, 0x01C8, 0x01D5, 0x01D9, 0x01DC, 0x01DD, 0x01EA, 0x01EA,
	0x01EE, 0x01F3, 0x01F6, 0x01F7, 0x01FC, 0x01FF, 0x0391, 0x0392,
	0x039B, 0x039E, 0x03B2, 0x03B5, 0x03B7, 0x03B7, 0x03F8, 0x03FB,
	0x0440, 0x0440, 0x0443, 0x0444, 0x044B, 0x044B, 0x044D, 0x044F,
	0x0452, 0x0452, 0x0454, 0x045B, 0x047F, 0x047F, 0x0578, 0x0587,
	0x05C9, 0x05C9, 0x05D0, 0x05D0, 0x0601, 0x0604, 0x0606, 0x0609,
	0x060B, 0x060E, 0x0613, 0x0614, 0x0A29, 0x0A2B, 0x0A2F, 0x0A31,
	0x0A40, 0x0A40, 0x0A42, 0x0A43, 0x0A45, 0x0A45, 0x0A4E, 0x0A4F,
	0x0C01, 0x0C1D, 0x0C30, 0x0C30, 0x0C38, 0x0C39, 0x0C3C, 0x0C3C,
	0x0C80, 0x0C81, 0x0C88, 0x0C93, 0x0D00, 0x0D03, 0x0D05, 0x0D06,
	0x0D08, 0x0D0B, 0x0D34, 0x0D35, 0x0DAE, 0x0DC1, 0x0DC8, 0x0DD4,
	0x0DD8, 0x0DD9, 0x0E00, 0x0E00, 0x0E02, 0x0E04, 0x0E17, 0x0E1E,
	0x0EC0, 0x0EC9, 0x0ECB, 0x0ECC, 0x0ED0, 0x0ED0, 0x0ED4, 0x0ED7,
	0x0EE0, 0x0EE2, 0x0F01, 0x0F02, 0x2000, 0x200F, 0x2080, 0x2082,
	0x2100, 0x2109, 0x210C, 0x2114, 0x2180, 0x2184, 0x21F5, 0x21F7,
	0x2200, 0x2202, 0x2204, 0x2206, 0x2208, 0x2210, 0x2220, 0x2222,
	0x2280, 0x2282, 0x2294, 0x2294, 0x2297, 0x2297, 0x2300, 0x230A,
	0x2312, 0x2312, 0x2315, 0x2316, 0x2318, 0x231D, 0x2324, 0x2326,
	0x2340, 0x2357, 0x2360, 0x2360, 0x2380, 0x2383, 0x2400, 0x240F,
	0x2480, 0x2482, 0x2500, 0x2509, 0x250C, 0x2514, 0x2580, 0x2584,
	0x25F5, 0x25F7, 0x2600, 0x2602, 0x2604, 0x2606, 0x2608, 0x2610,
	0x2620, 0x2622, 0x2680, 0x2682, 0x2694, 0x2694, 0x2697, 0x2697,
	0x2700, 0x270A, 0x2712, 0x2712, 0x2715, 0x2716, 0x2718, 0x271D,
	0x2724, 0x2726, 0x2740, 0x2757, 0x2760, 0x2760, 0x2780, 0x2783,
	0x4000, 0x4003, 0x4800, 0x4806, 0x4808, 0x4808, 0x4900, 0x4900,
	0x4908, 0x4908,
};

const unsigned int a200_registers_count = ARRAY_SIZE(a200_registers) / 2;
const unsigned int a220_registers_count = ARRAY_SIZE(a220_registers) / 2;
const unsigned int a225_registers_count = ARRAY_SIZE(a225_registers) / 2;



#define ALU_CONSTANTS	2048	
#define NUM_REGISTERS	1024	
#ifdef CONFIG_MSM_KGSL_DISABLE_SHADOW_WRITES
#define CMD_BUFFER_LEN	9216	
#else
#define CMD_BUFFER_LEN	3072	
#endif
#define TEX_CONSTANTS		(32*6)	
#define BOOL_CONSTANTS		8	
#define LOOP_CONSTANTS		56	

#define LCC_SHADOW_SIZE		0x2000	

#define ALU_SHADOW_SIZE		LCC_SHADOW_SIZE	
#define REG_SHADOW_SIZE		0x1000	
#ifdef CONFIG_MSM_KGSL_DISABLE_SHADOW_WRITES
#define CMD_BUFFER_SIZE		0x9000	
#else
#define CMD_BUFFER_SIZE		0x3000	
#endif
#define TEX_SHADOW_SIZE		(TEX_CONSTANTS*4)	

#define REG_OFFSET		LCC_SHADOW_SIZE
#define CMD_OFFSET		(REG_OFFSET + REG_SHADOW_SIZE)
#define TEX_OFFSET		(CMD_OFFSET + CMD_BUFFER_SIZE)
#define SHADER_OFFSET		((TEX_OFFSET + TEX_SHADOW_SIZE + 32) & ~31)

static inline int _shader_shadow_size(struct adreno_device *adreno_dev)
{
	return adreno_dev->istore_size *
		(adreno_dev->instruction_size * sizeof(unsigned int));
}

static inline int _context_size(struct adreno_device *adreno_dev)
{
	return SHADER_OFFSET + 3*_shader_shadow_size(adreno_dev);
}


static struct tmp_ctx {
	unsigned int *start;	
	unsigned int *cmd;	

	
	uint32_t bool_shadow;	
	uint32_t loop_shadow;	

	uint32_t shader_shared;	
	uint32_t shader_vertex;	
	uint32_t shader_pixel;	

	uint32_t reg_values[33];
	uint32_t chicken_restore;

	uint32_t gmem_base;	

} tmp_ctx;


#define GMEM2SYS_VTX_PGM_LEN	0x12

static unsigned int gmem2sys_vtx_pgm[GMEM2SYS_VTX_PGM_LEN] = {
	0x00011003, 0x00001000, 0xc2000000,
	0x00001004, 0x00001000, 0xc4000000,
	0x00001005, 0x00002000, 0x00000000,
	0x1cb81000, 0x00398a88, 0x00000003,
	0x140f803e, 0x00000000, 0xe2010100,
	0x14000000, 0x00000000, 0xe2000000
};


#define GMEM2SYS_FRAG_PGM_LEN	0x0c

static unsigned int gmem2sys_frag_pgm[GMEM2SYS_FRAG_PGM_LEN] = {
	0x00000000, 0x1002c400, 0x10000000,
	0x00001003, 0x00002000, 0x00000000,
	0x140f8000, 0x00000000, 0x22000000,
	0x14000000, 0x00000000, 0xe2000000
};


#define SYS2GMEM_VTX_PGM_LEN	0x18

static unsigned int sys2gmem_vtx_pgm[SYS2GMEM_VTX_PGM_LEN] = {
	0x00052003, 0x00001000, 0xc2000000, 0x00001005,
	0x00001000, 0xc4000000, 0x00001006, 0x10071000,
	0x20000000, 0x18981000, 0x0039ba88, 0x00000003,
	0x12982000, 0x40257b08, 0x00000002, 0x140f803e,
	0x00000000, 0xe2010100, 0x140f8000, 0x00000000,
	0xe2020200, 0x14000000, 0x00000000, 0xe2000000
};


#define SYS2GMEM_FRAG_PGM_LEN	0x0f

static unsigned int sys2gmem_frag_pgm[SYS2GMEM_FRAG_PGM_LEN] = {
	0x00011002, 0x00001000, 0xc4000000, 0x00001003,
	0x10041000, 0x20000000, 0x10000001, 0x1ffff688,
	0x00000002, 0x140f8000, 0x00000000, 0xe2000000,
	0x14000000, 0x00000000, 0xe2000000
};

#define SYS2GMEM_TEX_CONST_LEN	6

static unsigned int sys2gmem_tex_const[SYS2GMEM_TEX_CONST_LEN] = {
	0x00000002,		

	0x00000800,		

	
	0,			

	0 << 1 | 1 << 4 | 2 << 7 | 3 << 10 | 2 << 23,

	0,

	1 << 9			
};

#define NUM_COLOR_FORMATS   13

static enum SURFACEFORMAT surface_format_table[NUM_COLOR_FORMATS] = {
	FMT_4_4_4_4,		
	FMT_1_5_5_5,		
	FMT_5_6_5,		
	FMT_8,			
	FMT_8_8,		
	FMT_8_8_8_8,		
	FMT_8_8_8_8,		
	FMT_16_FLOAT,		
	FMT_16_16_FLOAT,	
	FMT_16_16_16_16_FLOAT,	
	FMT_32_FLOAT,		
	FMT_32_32_FLOAT,	
	FMT_32_32_32_32_FLOAT,	
};

static unsigned int format2bytesperpixel[NUM_COLOR_FORMATS] = {
	2,			
	2,			
	2,			
	1,			
	2,			
	4,			
	4,			
	2,			
	4,			
	8,			
	4,			
	8,			
	16,			
};

#define SHADER_CONST_ADDR	(11 * 6 + 3)


static unsigned int *program_shader(unsigned int *cmds, int vtxfrag,
				    unsigned int *shader_pgm, int dwords)
{
	
	*cmds++ = cp_type3_packet(CP_IM_LOAD_IMMEDIATE, 2 + dwords);
	
	*cmds++ = vtxfrag;
	
	*cmds++ = ((0 << 16) | dwords);

	memcpy(cmds, shader_pgm, dwords << 2);
	cmds += dwords;

	return cmds;
}

static unsigned int *reg_to_mem(unsigned int *cmds, uint32_t dst,
				uint32_t src, int dwords)
{
	while (dwords-- > 0) {
		*cmds++ = cp_type3_packet(CP_REG_TO_MEM, 2);
		*cmds++ = src++;
		*cmds++ = dst;
		dst += 4;
	}

	return cmds;
}

#ifdef CONFIG_MSM_KGSL_DISABLE_SHADOW_WRITES

static void build_reg_to_mem_range(unsigned int start, unsigned int end,
				   unsigned int **cmd,
				   struct adreno_context *drawctxt)
{
	unsigned int i = start;

	for (i = start; i <= end; i++) {
		*(*cmd)++ = cp_type3_packet(CP_REG_TO_MEM, 2);
		*(*cmd)++ = i;
		*(*cmd)++ =
		    ((drawctxt->gpustate.gpuaddr + REG_OFFSET) & 0xFFFFE000) +
		    (i - 0x2000) * 4;
	}
}

#endif

static unsigned int *build_chicken_restore_cmds(
					struct adreno_context *drawctxt)
{
	unsigned int *start = tmp_ctx.cmd;
	unsigned int *cmds = start;

	*cmds++ = cp_type3_packet(CP_WAIT_FOR_IDLE, 1);
	*cmds++ = 0;

	*cmds++ = cp_type0_packet(REG_TP0_CHICKEN, 1);
	tmp_ctx.chicken_restore = virt2gpu(cmds, &drawctxt->gpustate);
	*cmds++ = 0x00000000;

	
	create_ib1(drawctxt, drawctxt->chicken_restore, start, cmds);

	return cmds;
}


static const unsigned int register_ranges_a20x[] = {
	REG_RB_SURFACE_INFO, REG_RB_DEPTH_INFO,
	REG_COHER_DEST_BASE_0, REG_PA_SC_SCREEN_SCISSOR_BR,
	REG_PA_SC_WINDOW_OFFSET, REG_PA_SC_WINDOW_SCISSOR_BR,
	REG_RB_STENCILREFMASK_BF, REG_PA_CL_VPORT_ZOFFSET,
	REG_SQ_PROGRAM_CNTL, REG_SQ_WRAPPING_1,
	REG_PA_SC_LINE_CNTL, REG_SQ_PS_CONST,
	REG_PA_SC_AA_MASK, REG_PA_SC_AA_MASK,
	REG_RB_SAMPLE_COUNT_CTL, REG_RB_COLOR_DEST_MASK,
	REG_PA_SU_POLY_OFFSET_FRONT_SCALE, REG_PA_SU_POLY_OFFSET_BACK_OFFSET,
	REG_VGT_MAX_VTX_INDX, REG_RB_FOG_COLOR,
	REG_RB_DEPTHCONTROL, REG_RB_MODECONTROL,
	REG_PA_SU_POINT_SIZE, REG_PA_SC_LINE_STIPPLE,
	REG_PA_SC_VIZ_QUERY, REG_PA_SC_VIZ_QUERY,
	REG_VGT_VERTEX_REUSE_BLOCK_CNTL, REG_RB_DEPTH_CLEAR
};

static const unsigned int register_ranges_a220[] = {
	REG_RB_SURFACE_INFO, REG_RB_DEPTH_INFO,
	REG_COHER_DEST_BASE_0, REG_PA_SC_SCREEN_SCISSOR_BR,
	REG_PA_SC_WINDOW_OFFSET, REG_PA_SC_WINDOW_SCISSOR_BR,
	REG_RB_STENCILREFMASK_BF, REG_PA_CL_VPORT_ZOFFSET,
	REG_SQ_PROGRAM_CNTL, REG_SQ_WRAPPING_1,
	REG_PA_SC_LINE_CNTL, REG_SQ_PS_CONST,
	REG_PA_SC_AA_MASK, REG_PA_SC_AA_MASK,
	REG_RB_SAMPLE_COUNT_CTL, REG_RB_COLOR_DEST_MASK,
	REG_PA_SU_POLY_OFFSET_FRONT_SCALE, REG_PA_SU_POLY_OFFSET_BACK_OFFSET,
	REG_A220_PC_MAX_VTX_INDX, REG_A220_PC_INDX_OFFSET,
	REG_RB_COLOR_MASK, REG_RB_FOG_COLOR,
	REG_RB_DEPTHCONTROL, REG_RB_COLORCONTROL,
	REG_PA_CL_CLIP_CNTL, REG_PA_CL_VTE_CNTL,
	REG_RB_MODECONTROL, REG_RB_SAMPLE_POS,
	REG_PA_SU_POINT_SIZE, REG_PA_SU_LINE_CNTL,
	REG_A220_PC_VERTEX_REUSE_BLOCK_CNTL,
	REG_A220_PC_VERTEX_REUSE_BLOCK_CNTL,
	REG_RB_COPY_CONTROL, REG_RB_DEPTH_CLEAR
};

static const unsigned int register_ranges_a225[] = {
	REG_RB_SURFACE_INFO, REG_A225_RB_COLOR_INFO3,
	REG_COHER_DEST_BASE_0, REG_PA_SC_SCREEN_SCISSOR_BR,
	REG_PA_SC_WINDOW_OFFSET, REG_PA_SC_WINDOW_SCISSOR_BR,
	REG_RB_STENCILREFMASK_BF, REG_PA_CL_VPORT_ZOFFSET,
	REG_SQ_PROGRAM_CNTL, REG_SQ_WRAPPING_1,
	REG_PA_SC_LINE_CNTL, REG_SQ_PS_CONST,
	REG_PA_SC_AA_MASK, REG_PA_SC_AA_MASK,
	REG_RB_SAMPLE_COUNT_CTL, REG_RB_COLOR_DEST_MASK,
	REG_PA_SU_POLY_OFFSET_FRONT_SCALE, REG_PA_SU_POLY_OFFSET_BACK_OFFSET,
	REG_A220_PC_MAX_VTX_INDX, REG_A225_PC_MULTI_PRIM_IB_RESET_INDX,
	REG_RB_COLOR_MASK, REG_RB_FOG_COLOR,
	REG_RB_DEPTHCONTROL, REG_RB_COLORCONTROL,
	REG_PA_CL_CLIP_CNTL, REG_PA_CL_VTE_CNTL,
	REG_RB_MODECONTROL, REG_RB_SAMPLE_POS,
	REG_PA_SU_POINT_SIZE, REG_PA_SU_LINE_CNTL,
	REG_A220_PC_VERTEX_REUSE_BLOCK_CNTL,
	REG_A220_PC_VERTEX_REUSE_BLOCK_CNTL,
	REG_RB_COPY_CONTROL, REG_RB_DEPTH_CLEAR,
	REG_A225_GRAS_UCP0X, REG_A225_GRAS_UCP5W,
	REG_A225_GRAS_UCP_ENABLED, REG_A225_GRAS_UCP_ENABLED
};


static void build_regsave_cmds(struct adreno_device *adreno_dev,
			       struct adreno_context *drawctxt)
{
	unsigned int *start = tmp_ctx.cmd;
	unsigned int *cmd = start;

	*cmd++ = cp_type3_packet(CP_WAIT_FOR_IDLE, 1);
	*cmd++ = 0;

#ifdef CONFIG_MSM_KGSL_DISABLE_SHADOW_WRITES
	*cmd++ = cp_type3_packet(CP_CONTEXT_UPDATE, 1);
	*cmd++ = 0;

	{
		unsigned int i = 0;
		unsigned int reg_array_size = 0;
		const unsigned int *ptr_register_ranges;

		
		if (adreno_is_a220(adreno_dev)) {
			ptr_register_ranges = register_ranges_a220;
			reg_array_size = ARRAY_SIZE(register_ranges_a220);
		} else if (adreno_is_a225(adreno_dev)) {
			ptr_register_ranges = register_ranges_a225;
			reg_array_size = ARRAY_SIZE(register_ranges_a225);
		} else {
			ptr_register_ranges = register_ranges_a20x;
			reg_array_size = ARRAY_SIZE(register_ranges_a20x);
		}


		
		for (i = 0; i < (reg_array_size/2) ; i++) {
			build_reg_to_mem_range(ptr_register_ranges[i*2],
					ptr_register_ranges[i*2+1],
					&cmd, drawctxt);
		}
	}

	
	cmd =
	    reg_to_mem(cmd, (drawctxt->gpustate.gpuaddr) & 0xFFFFE000,
		       REG_SQ_CONSTANT_0, ALU_CONSTANTS);

	
	cmd =
	    reg_to_mem(cmd,
		       (drawctxt->gpustate.gpuaddr + TEX_OFFSET) & 0xFFFFE000,
		       REG_SQ_FETCH_0, TEX_CONSTANTS);
#else

	*cmd++ = cp_type3_packet(CP_WAIT_FOR_IDLE, 1);
	*cmd++ = 0;

	*cmd++ = cp_type3_packet(CP_LOAD_CONSTANT_CONTEXT, 3);
	*cmd++ = (drawctxt->gpustate.gpuaddr + REG_OFFSET) & 0xFFFFE000;
	*cmd++ = 4 << 16;	
	*cmd++ = 0x0;		

	*cmd++ = cp_type3_packet(CP_LOAD_CONSTANT_CONTEXT, 3);
	*cmd++ = drawctxt->gpustate.gpuaddr & 0xFFFFE000;
	*cmd++ = 0 << 16;	
	*cmd++ = 0x0;		

	*cmd++ = cp_type3_packet(CP_LOAD_CONSTANT_CONTEXT, 3);
	*cmd++ = (drawctxt->gpustate.gpuaddr + TEX_OFFSET) & 0xFFFFE000;
	*cmd++ = 1 << 16;	
	*cmd++ = 0x0;		
#endif

	
	*cmd++ = cp_type3_packet(CP_REG_TO_MEM, 2);
	*cmd++ = REG_SQ_GPR_MANAGEMENT;
	*cmd++ = tmp_ctx.reg_values[0];

	*cmd++ = cp_type3_packet(CP_REG_TO_MEM, 2);
	*cmd++ = REG_TP0_CHICKEN;
	*cmd++ = tmp_ctx.reg_values[1];

	if (adreno_is_a22x(adreno_dev)) {
		unsigned int i;
		unsigned int j = 2;
		for (i = REG_A220_VSC_BIN_SIZE; i <=
				REG_A220_VSC_PIPE_DATA_LENGTH_7; i++) {
			*cmd++ = cp_type3_packet(CP_REG_TO_MEM, 2);
			*cmd++ = i;
			*cmd++ = tmp_ctx.reg_values[j];
			j++;
		}
	}

	
	cmd = reg_to_mem(cmd, tmp_ctx.bool_shadow, REG_SQ_CF_BOOLEANS,
			 BOOL_CONSTANTS);

	
	cmd = reg_to_mem(cmd, tmp_ctx.loop_shadow,
		REG_SQ_CF_LOOP, LOOP_CONSTANTS);

	
	create_ib1(drawctxt, drawctxt->reg_save, start, cmd);

	tmp_ctx.cmd = cmd;
}

static unsigned int *build_gmem2sys_cmds(struct adreno_device *adreno_dev,
					 struct adreno_context *drawctxt,
					 struct gmem_shadow_t *shadow)
{
	unsigned int *cmds = shadow->gmem_save_commands;
	unsigned int *start = cmds;
	
	unsigned int bytesperpixel = format2bytesperpixel[shadow->format];
	unsigned int addr = shadow->gmemshadow.gpuaddr;
	unsigned int offset = (addr - (addr & 0xfffff000)) / bytesperpixel;

	if (!(drawctxt->flags & CTXT_FLAGS_PREAMBLE)) {
		
		*cmds++ = cp_type3_packet(CP_REG_TO_MEM, 2);
		*cmds++ = REG_TP0_CHICKEN;

		*cmds++ = tmp_ctx.chicken_restore;

		*cmds++ = cp_type3_packet(CP_WAIT_FOR_IDLE, 1);
		*cmds++ = 0;
	}

	
	*cmds++ = cp_type0_packet(REG_TP0_CHICKEN, 1);
	*cmds++ = 0x00000000;

	
	*cmds++ = cp_type0_packet(REG_PA_SC_AA_CONFIG, 1);
	*cmds++ = 0x00000000;

	

	
	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 4);
	*cmds++ = (0x1 << 16) | SHADER_CONST_ADDR;
	*cmds++ = 0;
	
	*cmds++ = shadow->quad_vertices.gpuaddr | 0x3;
	
	*cmds++ = 0x00000030;

	
	*cmds++ = cp_type0_packet(REG_TC_CNTL_STATUS, 1);
	*cmds++ = 0x1;

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 4);
	*cmds++ = CP_REG(REG_VGT_MAX_VTX_INDX);
	*cmds++ = 0x00ffffff;	
	*cmds++ = 0x0;		
	*cmds++ = 0x00000000;	

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_PA_SC_AA_MASK);
	*cmds++ = 0x0000ffff;	

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_RB_COLORCONTROL);
	*cmds++ = 0x00000c20;

	
	*cmds++ = cp_type0_packet(REG_SQ_INST_STORE_MANAGMENT, 1);
	*cmds++ = adreno_dev->pix_shader_start;

	
	*cmds++ = cp_type3_packet(CP_INVALIDATE_STATE, 1);
	*cmds++ = 0x00003F00;

	*cmds++ = cp_type3_packet(CP_SET_SHADER_BASES, 1);
	*cmds++ = adreno_encode_istore_size(adreno_dev)
		  | adreno_dev->pix_shader_start;

	
	cmds = program_shader(cmds, 0, gmem2sys_vtx_pgm, GMEM2SYS_VTX_PGM_LEN);

	
	cmds =
	    program_shader(cmds, 1, gmem2sys_frag_pgm, GMEM2SYS_FRAG_PGM_LEN);

	
	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 3);
	*cmds++ = CP_REG(REG_SQ_PROGRAM_CNTL);
	if (adreno_is_a22x(adreno_dev))
		*cmds++ = 0x10018001;
	else
		*cmds++ = 0x10010001;
	*cmds++ = 0x00000008;

	

	
	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_PA_CL_VTE_CNTL);
	
	*cmds++ = 0x00000b00;

	
	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 3);
	*cmds++ = CP_REG(REG_RB_SURFACE_INFO);
	*cmds++ = shadow->gmem_pitch;	

	
	BUG_ON(tmp_ctx.gmem_base & 0xFFF);
	*cmds++ =
	    (shadow->
	     format << RB_COLOR_INFO__COLOR_FORMAT__SHIFT) | tmp_ctx.gmem_base;

	
	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_RB_DEPTHCONTROL);
	if (adreno_is_a22x(adreno_dev))
		*cmds++ = 0x08;
	else
		*cmds++ = 0;

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_PA_SU_SC_MODE_CNTL);
	*cmds++ = 0x00080240;

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 3);
	*cmds++ = CP_REG(REG_PA_SC_SCREEN_SCISSOR_TL);
	*cmds++ = (0 << 16) | 0;
	*cmds++ = (0x1fff << 16) | (0x1fff);
	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 3);
	*cmds++ = CP_REG(REG_PA_SC_WINDOW_SCISSOR_TL);
	*cmds++ = (unsigned int)((1U << 31) | (0 << 16) | 0);
	*cmds++ = (0x1fff << 16) | (0x1fff);

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 3);
	*cmds++ = CP_REG(REG_PA_CL_VPORT_ZSCALE);
	*cmds++ = 0xbf800000;	
	*cmds++ = 0x0;

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_RB_COLOR_MASK);
	*cmds++ = 0x0000000f;	

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_RB_COLOR_DEST_MASK);
	*cmds++ = 0xffffffff;

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 3);
	*cmds++ = CP_REG(REG_SQ_WRAPPING_0);
	*cmds++ = 0x00000000;
	*cmds++ = 0x00000000;


	
	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 6);
	*cmds++ = CP_REG(REG_RB_COPY_CONTROL);
	*cmds++ = 0;		
	*cmds++ = addr & 0xfffff000;	
	*cmds++ = shadow->pitch >> 5;	

	*cmds++ = 0x0003c008 |
	    (shadow->format << RB_COPY_DEST_INFO__COPY_DEST_FORMAT__SHIFT);
	
	BUG_ON(offset & 0xfffff000);
	*cmds++ = offset;

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_RB_MODECONTROL);
	*cmds++ = 0x6;		

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_PA_CL_CLIP_CNTL);
	*cmds++ = 0x00010000;

	if (adreno_is_a22x(adreno_dev)) {
		*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
		*cmds++ = CP_REG(REG_A220_RB_LRZ_VSC_CONTROL);
		*cmds++ = 0x0000000;

		*cmds++ = cp_type3_packet(CP_DRAW_INDX, 3);
		*cmds++ = 0;           
		
		*cmds++ = 0x00004088;
		*cmds++ = 3;	       
	} else {
		
		*cmds++ = cp_type3_packet(CP_DRAW_INDX, 2);
		*cmds++ = 0;		
		
		*cmds++ = 0x00030088;
	}

	
	create_ib1(drawctxt, shadow->gmem_save, start, cmds);

	return cmds;
}


static unsigned int *build_sys2gmem_cmds(struct adreno_device *adreno_dev,
					 struct adreno_context *drawctxt,
					 struct gmem_shadow_t *shadow)
{
	unsigned int *cmds = shadow->gmem_restore_commands;
	unsigned int *start = cmds;

	if (!(drawctxt->flags & CTXT_FLAGS_PREAMBLE)) {
		
		*cmds++ = cp_type3_packet(CP_REG_TO_MEM, 2);
		*cmds++ = REG_TP0_CHICKEN;
		*cmds++ = tmp_ctx.chicken_restore;

		*cmds++ = cp_type3_packet(CP_WAIT_FOR_IDLE, 1);
		*cmds++ = 0;
	}

	
	*cmds++ = cp_type0_packet(REG_TP0_CHICKEN, 1);
	*cmds++ = 0x00000000;

	
	*cmds++ = cp_type0_packet(REG_PA_SC_AA_CONFIG, 1);
	*cmds++ = 0x00000000;
	

	
	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 7);

	*cmds++ = (0x1 << 16) | (9 * 6);
	
	*cmds++ = shadow->quad_vertices.gpuaddr | 0x3;
	
	*cmds++ = 0x00000030;
	
	*cmds++ = shadow->quad_texcoords.gpuaddr | 0x3;
	
	*cmds++ = 0x00000020;
	*cmds++ = 0;
	*cmds++ = 0;

	
	*cmds++ = cp_type0_packet(REG_TC_CNTL_STATUS, 1);
	*cmds++ = 0x1;

	cmds = program_shader(cmds, 0, sys2gmem_vtx_pgm, SYS2GMEM_VTX_PGM_LEN);

	
	*cmds++ = cp_type0_packet(REG_SQ_INST_STORE_MANAGMENT, 1);
	*cmds++ = adreno_dev->pix_shader_start;

	
	*cmds++ = cp_type3_packet(CP_INVALIDATE_STATE, 1);
	*cmds++ = 0x00000300; 

	*cmds++ = cp_type3_packet(CP_SET_SHADER_BASES, 1);
	*cmds++ = adreno_encode_istore_size(adreno_dev)
		  | adreno_dev->pix_shader_start;

	
	cmds =
	    program_shader(cmds, 1, sys2gmem_frag_pgm, SYS2GMEM_FRAG_PGM_LEN);

	
	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 3);
	*cmds++ = CP_REG(REG_SQ_PROGRAM_CNTL);
	*cmds++ = 0x10030002;
	*cmds++ = 0x00000008;

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_PA_SC_AA_MASK);
	*cmds++ = 0x0000ffff;	

	if (!adreno_is_a22x(adreno_dev)) {
		
		*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
		*cmds++ = CP_REG(REG_PA_SC_VIZ_QUERY);
		*cmds++ = 0x0;		
	}

	
	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_RB_COLORCONTROL);
	*cmds++ = 0x00000c20;

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 4);
	*cmds++ = CP_REG(REG_VGT_MAX_VTX_INDX);
	*cmds++ = 0x00ffffff;	
	*cmds++ = 0x0;		
	*cmds++ = 0x00000000;	

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 3);
	*cmds++ = CP_REG(REG_VGT_VERTEX_REUSE_BLOCK_CNTL);
	*cmds++ = 0x00000002;	
	*cmds++ = 0x00000002;	

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_SQ_INTERPOLATOR_CNTL);
	*cmds++ = 0xffffffff;	

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_PA_SC_AA_CONFIG);
	*cmds++ = 0x00000000;	

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_PA_SU_SC_MODE_CNTL);
	*cmds++ = 0x00080240;

	
	*cmds++ =
	    cp_type3_packet(CP_SET_CONSTANT, (SYS2GMEM_TEX_CONST_LEN + 1));
	*cmds++ = (0x1 << 16) | (0 * 6);
	memcpy(cmds, sys2gmem_tex_const, SYS2GMEM_TEX_CONST_LEN << 2);
	cmds[0] |= (shadow->pitch >> 5) << 22;
	cmds[1] |=
	    shadow->gmemshadow.gpuaddr | surface_format_table[shadow->format];
	cmds[2] |= (shadow->width - 1) | (shadow->height - 1) << 13;
	cmds += SYS2GMEM_TEX_CONST_LEN;

	
	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 3);
	*cmds++ = CP_REG(REG_RB_SURFACE_INFO);
	*cmds++ = shadow->gmem_pitch;	

	*cmds++ =
	    (shadow->
	     format << RB_COLOR_INFO__COLOR_FORMAT__SHIFT) | tmp_ctx.gmem_base;

	
	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_RB_DEPTHCONTROL);

	if (adreno_is_a22x(adreno_dev))
		*cmds++ = 8;		
	else
		*cmds++ = 0;		

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 3);
	*cmds++ = CP_REG(REG_PA_SC_SCREEN_SCISSOR_TL);
	*cmds++ = (0 << 16) | 0;
	*cmds++ = ((0x1fff) << 16) | 0x1fff;
	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 3);
	*cmds++ = CP_REG(REG_PA_SC_WINDOW_SCISSOR_TL);
	*cmds++ = (unsigned int)((1U << 31) | (0 << 16) | 0);
	*cmds++ = ((0x1fff) << 16) | 0x1fff;

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_PA_CL_VTE_CNTL);
	
	*cmds++ = 0x00000b00;

	
	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 3);
	*cmds++ = CP_REG(REG_PA_CL_VPORT_ZSCALE);
	*cmds++ = 0xbf800000;
	*cmds++ = 0x0;

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_RB_COLOR_MASK);
	*cmds++ = 0x0000000f;	

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_RB_COLOR_DEST_MASK);
	*cmds++ = 0xffffffff;

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 3);
	*cmds++ = CP_REG(REG_SQ_WRAPPING_0);
	*cmds++ = 0x00000000;
	*cmds++ = 0x00000000;

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_RB_MODECONTROL);
	
	*cmds++ = 0x4;

	*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
	*cmds++ = CP_REG(REG_PA_CL_CLIP_CNTL);
	*cmds++ = 0x00010000;

	if (adreno_is_a22x(adreno_dev)) {
		*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
		*cmds++ = CP_REG(REG_A220_RB_LRZ_VSC_CONTROL);
		*cmds++ = 0x0000000;

		*cmds++ = cp_type3_packet(CP_DRAW_INDX, 3);
		*cmds++ = 0;           
		
		*cmds++ = 0x00004088;
		*cmds++ = 3;	       
	} else {
		
		*cmds++ = cp_type3_packet(CP_DRAW_INDX, 2);
		*cmds++ = 0;		
		
		*cmds++ = 0x00030088;
	}

	
	create_ib1(drawctxt, shadow->gmem_restore, start, cmds);

	return cmds;
}

static void build_regrestore_cmds(struct adreno_device *adreno_dev,
				  struct adreno_context *drawctxt)
{
	unsigned int *start = tmp_ctx.cmd;
	unsigned int *cmd = start;

	unsigned int i = 0;
	unsigned int reg_array_size = 0;
	const unsigned int *ptr_register_ranges;

	*cmd++ = cp_type3_packet(CP_WAIT_FOR_IDLE, 1);
	*cmd++ = 0;

	
	
	cmd++;
#ifdef CONFIG_MSM_KGSL_DISABLE_SHADOW_WRITES
	
	*cmd++ = ((drawctxt->gpustate.gpuaddr + REG_OFFSET) & 0xFFFFE000) | 1;
#else
	*cmd++ = (drawctxt->gpustate.gpuaddr + REG_OFFSET) & 0xFFFFE000;
#endif

	
	if (adreno_is_a220(adreno_dev)) {
		ptr_register_ranges = register_ranges_a220;
		reg_array_size = ARRAY_SIZE(register_ranges_a220);
	} else if (adreno_is_a225(adreno_dev)) {
		ptr_register_ranges = register_ranges_a225;
		reg_array_size = ARRAY_SIZE(register_ranges_a225);
	} else {
		ptr_register_ranges = register_ranges_a20x;
		reg_array_size = ARRAY_SIZE(register_ranges_a20x);
	}


	for (i = 0; i < (reg_array_size/2); i++) {
		cmd = reg_range(cmd, ptr_register_ranges[i*2],
				ptr_register_ranges[i*2+1]);
	}

	start[2] =
	    cp_type3_packet(CP_LOAD_CONSTANT_CONTEXT, (cmd - start) - 3);
	
#ifdef CONFIG_MSM_KGSL_DISABLE_SHADOW_WRITES
	start[4] |= (0 << 24) | (4 << 16);	
#else
	start[4] |= (1 << 24) | (4 << 16);
#endif

	
	*cmd++ = cp_type0_packet(REG_SQ_GPR_MANAGEMENT, 1);
	tmp_ctx.reg_values[0] = virt2gpu(cmd, &drawctxt->gpustate);
	*cmd++ = 0x00040400;

	*cmd++ = cp_type3_packet(CP_WAIT_FOR_IDLE, 1);
	*cmd++ = 0;
	*cmd++ = cp_type0_packet(REG_TP0_CHICKEN, 1);
	tmp_ctx.reg_values[1] = virt2gpu(cmd, &drawctxt->gpustate);
	*cmd++ = 0x00000000;

	if (adreno_is_a22x(adreno_dev)) {
		unsigned int i;
		unsigned int j = 2;
		for (i = REG_A220_VSC_BIN_SIZE; i <=
				REG_A220_VSC_PIPE_DATA_LENGTH_7; i++) {
			*cmd++ = cp_type0_packet(i, 1);
			tmp_ctx.reg_values[j] = virt2gpu(cmd,
				&drawctxt->gpustate);
			*cmd++ = 0x00000000;
			j++;
		}
	}

	
	*cmd++ = cp_type3_packet(CP_LOAD_CONSTANT_CONTEXT, 3);
	*cmd++ = drawctxt->gpustate.gpuaddr & 0xFFFFE000;
#ifdef CONFIG_MSM_KGSL_DISABLE_SHADOW_WRITES
	*cmd++ = (0 << 24) | (0 << 16) | 0;	
#else
	*cmd++ = (1 << 24) | (0 << 16) | 0;
#endif
	*cmd++ = ALU_CONSTANTS;

	
	*cmd++ = cp_type3_packet(CP_LOAD_CONSTANT_CONTEXT, 3);
	*cmd++ = (drawctxt->gpustate.gpuaddr + TEX_OFFSET) & 0xFFFFE000;
#ifdef CONFIG_MSM_KGSL_DISABLE_SHADOW_WRITES
	
	*cmd++ = (0 << 24) | (1 << 16) | 0;
#else
	*cmd++ = (1 << 24) | (1 << 16) | 0;
#endif
	*cmd++ = TEX_CONSTANTS;

	
	*cmd++ = cp_type3_packet(CP_SET_CONSTANT, 1 + BOOL_CONSTANTS);
	*cmd++ = (2 << 16) | 0;

	tmp_ctx.bool_shadow = virt2gpu(cmd, &drawctxt->gpustate);
	cmd += BOOL_CONSTANTS;

	
	*cmd++ = cp_type3_packet(CP_SET_CONSTANT, 1 + LOOP_CONSTANTS);
	*cmd++ = (3 << 16) | 0;

	tmp_ctx.loop_shadow = virt2gpu(cmd, &drawctxt->gpustate);
	cmd += LOOP_CONSTANTS;

	
	create_ib1(drawctxt, drawctxt->reg_restore, start, cmd);

	tmp_ctx.cmd = cmd;
}

static void
build_shader_save_restore_cmds(struct adreno_device *adreno_dev,
				struct adreno_context *drawctxt)
{
	unsigned int *cmd = tmp_ctx.cmd;
	unsigned int *save, *restore, *fixup;
	unsigned int *startSizeVtx, *startSizePix, *startSizeShared;
	unsigned int *partition1;
	unsigned int *shaderBases, *partition2;

	
	tmp_ctx.shader_vertex = drawctxt->gpustate.gpuaddr + SHADER_OFFSET;
	tmp_ctx.shader_pixel = tmp_ctx.shader_vertex
				+ _shader_shadow_size(adreno_dev);
	tmp_ctx.shader_shared = tmp_ctx.shader_pixel
				+  _shader_shadow_size(adreno_dev);

	

	restore = cmd;		

	
	*cmd++ = cp_type3_packet(CP_INVALIDATE_STATE, 1);
	*cmd++ = 0x00000300;	

	
	*cmd++ = cp_type3_packet(CP_SET_SHADER_BASES, 1);
	shaderBases = cmd++;	

	
	*cmd++ = cp_type0_packet(REG_SQ_INST_STORE_MANAGMENT, 1);
	partition1 = cmd++;	

	
	*cmd++ = cp_type3_packet(CP_IM_LOAD, 2);
	*cmd++ = tmp_ctx.shader_vertex + 0x0;	
	startSizeVtx = cmd++;	

	
	*cmd++ = cp_type3_packet(CP_IM_LOAD, 2);
	*cmd++ = tmp_ctx.shader_pixel + 0x1;	
	startSizePix = cmd++;	

	
	*cmd++ = cp_type3_packet(CP_IM_LOAD, 2);
	*cmd++ = tmp_ctx.shader_shared + 0x2;	
	startSizeShared = cmd++;	

	
	create_ib1(drawctxt, drawctxt->shader_restore, restore, cmd);

	/*
	 *  fixup SET_SHADER_BASES data
	 *
	 *  since self-modifying PM4 code is being used here, a seperate
	 *  command buffer is used for this fixup operation, to ensure the
	 *  commands are not read by the PM4 engine before the data fields
	 *  have been written.
	 */

	fixup = cmd;		

	
	*cmd++ = cp_type0_packet(REG_SCRATCH_REG2, 1);
	partition2 = cmd++;	

	
	*cmd++ = cp_type3_packet(CP_REG_RMW, 3);
	*cmd++ = REG_SCRATCH_REG2;
	
	*cmd++ = 0x0FFF0FFF;
	
	*cmd++ = adreno_encode_istore_size(adreno_dev);

	
	*cmd++ = cp_type3_packet(CP_REG_TO_MEM, 2);
	*cmd++ = REG_SCRATCH_REG2;
	
	*cmd++ = virt2gpu(shaderBases, &drawctxt->gpustate);

	
	create_ib1(drawctxt, drawctxt->shader_fixup, fixup, cmd);

	

	save = cmd;		

	*cmd++ = cp_type3_packet(CP_WAIT_FOR_IDLE, 1);
	*cmd++ = 0;

	*cmd++ = cp_type3_packet(CP_REG_TO_MEM, 2);
	*cmd++ = REG_SQ_INST_STORE_MANAGMENT;
	
	*cmd++ = virt2gpu(partition1, &drawctxt->gpustate);
	*cmd++ = cp_type3_packet(CP_REG_TO_MEM, 2);
	*cmd++ = REG_SQ_INST_STORE_MANAGMENT;
	
	*cmd++ = virt2gpu(partition2, &drawctxt->gpustate);


	
	*cmd++ = cp_type3_packet(CP_IM_STORE, 2);
	*cmd++ = tmp_ctx.shader_vertex + 0x0;	
	
	*cmd++ = virt2gpu(startSizeVtx, &drawctxt->gpustate);

	
	*cmd++ = cp_type3_packet(CP_IM_STORE, 2);
	*cmd++ = tmp_ctx.shader_pixel + 0x1;	
	
	*cmd++ = virt2gpu(startSizePix, &drawctxt->gpustate);

	

	*cmd++ = cp_type3_packet(CP_IM_STORE, 2);
	*cmd++ = tmp_ctx.shader_shared + 0x2;	
	
	*cmd++ = virt2gpu(startSizeShared, &drawctxt->gpustate);


	*cmd++ = cp_type3_packet(CP_WAIT_FOR_IDLE, 1);
	*cmd++ = 0;

	
	create_ib1(drawctxt, drawctxt->shader_save, save, cmd);

	tmp_ctx.cmd = cmd;
}

static int a2xx_create_gpustate_shadow(struct adreno_device *adreno_dev,
			struct adreno_context *drawctxt)
{
	drawctxt->flags |= CTXT_FLAGS_STATE_SHADOW;

	
	build_regrestore_cmds(adreno_dev, drawctxt);
	build_regsave_cmds(adreno_dev, drawctxt);

	build_shader_save_restore_cmds(adreno_dev, drawctxt);

	return 0;
}

static int a2xx_create_gmem_shadow(struct adreno_device *adreno_dev,
			struct adreno_context *drawctxt)
{
	int result;

	calc_gmemsize(&drawctxt->context_gmem_shadow, adreno_dev->gmem_size);
	tmp_ctx.gmem_base = adreno_dev->gmem_base;

	result = kgsl_allocate(&drawctxt->context_gmem_shadow.gmemshadow,
		drawctxt->pagetable, drawctxt->context_gmem_shadow.size);

	if (result)
		return result;

	
	drawctxt->flags |= CTXT_FLAGS_GMEM_SHADOW;

	
	kgsl_sharedmem_set(&drawctxt->context_gmem_shadow.gmemshadow, 0, 0,
			   drawctxt->context_gmem_shadow.size);

	
	build_quad_vtxbuff(drawctxt, &drawctxt->context_gmem_shadow,
		&tmp_ctx.cmd);

	
	if (!(drawctxt->flags & CTXT_FLAGS_PREAMBLE))
		tmp_ctx.cmd = build_chicken_restore_cmds(drawctxt);

	
	drawctxt->context_gmem_shadow.gmem_save_commands = tmp_ctx.cmd;
	tmp_ctx.cmd =
	    build_gmem2sys_cmds(adreno_dev, drawctxt,
				&drawctxt->context_gmem_shadow);
	drawctxt->context_gmem_shadow.gmem_restore_commands = tmp_ctx.cmd;
	tmp_ctx.cmd =
	    build_sys2gmem_cmds(adreno_dev, drawctxt,
				&drawctxt->context_gmem_shadow);

	kgsl_cache_range_op(&drawctxt->context_gmem_shadow.gmemshadow,
			    KGSL_CACHE_OP_FLUSH);

	kgsl_cffdump_syncmem(NULL,
			&drawctxt->context_gmem_shadow.gmemshadow,
			drawctxt->context_gmem_shadow.gmemshadow.gpuaddr,
			drawctxt->context_gmem_shadow.gmemshadow.size, false);

	return 0;
}

static int a2xx_drawctxt_create(struct adreno_device *adreno_dev,
	struct adreno_context *drawctxt)
{
	int ret;


	ret = kgsl_allocate(&drawctxt->gpustate,
		drawctxt->pagetable, _context_size(adreno_dev));

	if (ret)
		return ret;

	kgsl_sharedmem_set(&drawctxt->gpustate, 0, 0,
		_context_size(adreno_dev));

	tmp_ctx.cmd = tmp_ctx.start
	    = (unsigned int *)((char *)drawctxt->gpustate.hostptr + CMD_OFFSET);

	if (!(drawctxt->flags & CTXT_FLAGS_PREAMBLE)) {
		ret = a2xx_create_gpustate_shadow(adreno_dev, drawctxt);
		if (ret)
			goto done;

		drawctxt->flags |= CTXT_FLAGS_SHADER_SAVE;
	}

	if (!(drawctxt->flags & CTXT_FLAGS_NOGMEMALLOC)) {
		ret = a2xx_create_gmem_shadow(adreno_dev, drawctxt);
		if (ret)
			goto done;
	}

	

	kgsl_cache_range_op(&drawctxt->gpustate,
			    KGSL_CACHE_OP_FLUSH);

	kgsl_cffdump_syncmem(NULL, &drawctxt->gpustate,
			drawctxt->gpustate.gpuaddr,
			drawctxt->gpustate.size, false);

done:
	if (ret)
		kgsl_sharedmem_free(&drawctxt->gpustate);

	return ret;
}

static void a2xx_drawctxt_draw_workaround(struct adreno_device *adreno_dev,
					struct adreno_context *context)
{
	struct kgsl_device *device = &adreno_dev->dev;
	unsigned int cmd[11];
	unsigned int *cmds = &cmd[0];

	if (adreno_is_a225(adreno_dev)) {
		adreno_dev->gpudev->ctx_switches_since_last_draw++;
		if (adreno_dev->gpudev->ctx_switches_since_last_draw >
				ADRENO_NUM_CTX_SWITCH_ALLOWED_BEFORE_DRAW)
			adreno_dev->gpudev->ctx_switches_since_last_draw = 0;
		else
			return;
		*cmds++ = cp_type3_packet(CP_SET_CONSTANT, 2);
		*cmds++ = (0x4 << 16) | (REG_PA_SU_SC_MODE_CNTL - 0x2000);
		*cmds++ = 0;
		*cmds++ = cp_type3_packet(CP_DRAW_INDX, 5);
		*cmds++ = 0;
		*cmds++ = 1<<14;
		*cmds++ = 0;
		*cmds++ = device->mmu.setstate_memory.gpuaddr;
		*cmds++ = 0;
		*cmds++ = cp_type3_packet(CP_WAIT_FOR_IDLE, 1);
		*cmds++ = 0x00000000;
	} else {
		*cmds++ = cp_type3_packet(CP_SET_SHADER_BASES, 1);
		*cmds++ = adreno_encode_istore_size(adreno_dev)
					| adreno_dev->pix_shader_start;
	}

	adreno_ringbuffer_issuecmds(device, context, KGSL_CMD_FLAGS_PMODE,
			&cmd[0], cmds - cmd);
}

static void a2xx_drawctxt_save(struct adreno_device *adreno_dev,
			struct adreno_context *context)
{
	struct kgsl_device *device = &adreno_dev->dev;

	if (context == NULL || (context->flags & CTXT_FLAGS_BEING_DESTROYED))
		return;

	if (context->flags & CTXT_FLAGS_GPU_HANG)
		KGSL_CTXT_WARN(device,
			"Current active context has caused gpu hang\n");

	if (!(context->flags & CTXT_FLAGS_PREAMBLE)) {
		kgsl_cffdump_syncmem(NULL, &context->gpustate,
			context->reg_save[1],
			context->reg_save[2] << 2, true);
		
		adreno_ringbuffer_issuecmds(device, context,
			KGSL_CMD_FLAGS_NONE,
			context->reg_save, 3);

		if (context->flags & CTXT_FLAGS_SHADER_SAVE) {
			kgsl_cffdump_syncmem(NULL, &context->gpustate,
				context->shader_save[1],
				context->shader_save[2] << 2, true);
			
			adreno_ringbuffer_issuecmds(device, context,
				KGSL_CMD_FLAGS_PMODE,
				context->shader_save, 3);

			kgsl_cffdump_syncmem(NULL, &context->gpustate,
				context->shader_fixup[1],
				context->shader_fixup[2] << 2, true);
			adreno_ringbuffer_issuecmds(device, context,
				KGSL_CMD_FLAGS_NONE,
				context->shader_fixup, 3);

			context->flags |= CTXT_FLAGS_SHADER_RESTORE;
		}
	}

	if ((context->flags & CTXT_FLAGS_GMEM_SAVE) &&
	    (context->flags & CTXT_FLAGS_GMEM_SHADOW)) {
		kgsl_cffdump_syncmem(NULL, &context->gpustate,
			context->context_gmem_shadow.gmem_save[1],
			context->context_gmem_shadow.gmem_save[2] << 2, true);
		adreno_ringbuffer_issuecmds(device, context,
			KGSL_CMD_FLAGS_PMODE,
			context->context_gmem_shadow.gmem_save, 3);

		kgsl_cffdump_syncmem(NULL, &context->gpustate,
			context->chicken_restore[1],
			context->chicken_restore[2] << 2, true);

		
		if (!(context->flags & CTXT_FLAGS_PREAMBLE)) {
			adreno_ringbuffer_issuecmds(device, context,
				KGSL_CMD_FLAGS_NONE,
				context->chicken_restore, 3);
		}
		adreno_dev->gpudev->ctx_switches_since_last_draw = 0;

		context->flags |= CTXT_FLAGS_GMEM_RESTORE;
	} else if (adreno_is_a2xx(adreno_dev))
		a2xx_drawctxt_draw_workaround(adreno_dev, context);
}

static void a2xx_drawctxt_restore(struct adreno_device *adreno_dev,
			struct adreno_context *context)
{
	struct kgsl_device *device = &adreno_dev->dev;
	unsigned int cmds[5];

	if (context == NULL) {
		
		kgsl_mmu_setstate(&device->mmu, device->mmu.defaultpagetable,
				adreno_dev->drawctxt_active->id);
		return;
	}

	KGSL_CTXT_INFO(device, "context flags %08x\n", context->flags);

	cmds[0] = cp_nop_packet(1);
	cmds[1] = KGSL_CONTEXT_TO_MEM_IDENTIFIER;
	cmds[2] = cp_type3_packet(CP_MEM_WRITE, 2);
	cmds[3] = device->memstore.gpuaddr +
		KGSL_MEMSTORE_OFFSET(KGSL_MEMSTORE_GLOBAL, current_context);
	cmds[4] = context->id;
	adreno_ringbuffer_issuecmds(device, context, KGSL_CMD_FLAGS_NONE,
					cmds, 5);
	kgsl_mmu_setstate(&device->mmu, context->pagetable, context->id);

	if (context->flags & CTXT_FLAGS_GMEM_RESTORE) {
		kgsl_cffdump_syncmem(NULL, &context->gpustate,
			context->context_gmem_shadow.gmem_restore[1],
			context->context_gmem_shadow.gmem_restore[2] << 2,
			true);

		adreno_ringbuffer_issuecmds(device, context,
			KGSL_CMD_FLAGS_PMODE,
			context->context_gmem_shadow.gmem_restore, 3);

		if (!(context->flags & CTXT_FLAGS_PREAMBLE)) {
			kgsl_cffdump_syncmem(NULL, &context->gpustate,
				context->chicken_restore[1],
				context->chicken_restore[2] << 2, true);

			
			adreno_ringbuffer_issuecmds(device, context,
				KGSL_CMD_FLAGS_NONE,
				context->chicken_restore, 3);
		}

		context->flags &= ~CTXT_FLAGS_GMEM_RESTORE;
	}

	if (!(context->flags & CTXT_FLAGS_PREAMBLE)) {
		kgsl_cffdump_syncmem(NULL, &context->gpustate,
			context->reg_restore[1],
			context->reg_restore[2] << 2, true);

		
		adreno_ringbuffer_issuecmds(device, context,
			KGSL_CMD_FLAGS_NONE, context->reg_restore, 3);

		
		if (context->flags & CTXT_FLAGS_SHADER_RESTORE) {
			kgsl_cffdump_syncmem(NULL, &context->gpustate,
				context->shader_restore[1],
				context->shader_restore[2] << 2, true);

			adreno_ringbuffer_issuecmds(device, context,
				KGSL_CMD_FLAGS_NONE,
				context->shader_restore, 3);
		}
	}

	if (adreno_is_a20x(adreno_dev)) {
		cmds[0] = cp_type3_packet(CP_SET_BIN_BASE_OFFSET, 1);
		cmds[1] = context->bin_base_offset;
		adreno_ringbuffer_issuecmds(device, context,
			KGSL_CMD_FLAGS_NONE, cmds, 2);
	}
}


#define RBBM_INT_MASK RBBM_INT_CNTL__RDERR_INT_MASK

#define CP_INT_MASK \
	(CP_INT_CNTL__T0_PACKET_IN_IB_MASK | \
	CP_INT_CNTL__OPCODE_ERROR_MASK | \
	CP_INT_CNTL__PROTECTED_MODE_ERROR_MASK | \
	CP_INT_CNTL__RESERVED_BIT_ERROR_MASK | \
	CP_INT_CNTL__IB_ERROR_MASK | \
	CP_INT_CNTL__IB1_INT_MASK | \
	CP_INT_CNTL__RB_INT_MASK)

#define VALID_STATUS_COUNT_MAX	10

static struct {
	unsigned int mask;
	const char *message;
} kgsl_cp_error_irqs[] = {
	{ CP_INT_CNTL__T0_PACKET_IN_IB_MASK,
		"ringbuffer TO packet in IB interrupt" },
	{ CP_INT_CNTL__OPCODE_ERROR_MASK,
		"ringbuffer opcode error interrupt" },
	{ CP_INT_CNTL__PROTECTED_MODE_ERROR_MASK,
		"ringbuffer protected mode error interrupt" },
	{ CP_INT_CNTL__RESERVED_BIT_ERROR_MASK,
		"ringbuffer reserved bit error interrupt" },
	{ CP_INT_CNTL__IB_ERROR_MASK,
		"ringbuffer IB error interrupt" },
};

static void a2xx_cp_intrcallback(struct kgsl_device *device)
{
	unsigned int status = 0, num_reads = 0, master_status = 0;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_ringbuffer *rb = &adreno_dev->ringbuffer;
	int i;

	adreno_regread(device, REG_MASTER_INT_SIGNAL, &master_status);
	while (!status && (num_reads < VALID_STATUS_COUNT_MAX) &&
		(master_status & MASTER_INT_SIGNAL__CP_INT_STAT)) {
		adreno_regread(device, REG_CP_INT_STATUS, &status);
		adreno_regread(device, REG_MASTER_INT_SIGNAL,
					&master_status);
		num_reads++;
	}
	if (num_reads > 1)
		KGSL_DRV_WARN(device,
			"Looped %d times to read REG_CP_INT_STATUS\n",
			num_reads);

	trace_kgsl_a2xx_irq_status(device, master_status, status);

	if (!status) {
		if (master_status & MASTER_INT_SIGNAL__CP_INT_STAT) {
			KGSL_DRV_WARN(device, "Unable to read CP_INT_STATUS\n");
			wake_up_interruptible_all(&device->wait_queue);
		} else
			KGSL_DRV_WARN(device, "Spurious interrput detected\n");
		return;
	}

	for (i = 0; i < ARRAY_SIZE(kgsl_cp_error_irqs); i++) {
		if (status & kgsl_cp_error_irqs[i].mask) {
			KGSL_CMD_CRIT(rb->device, "%s\n",
				 kgsl_cp_error_irqs[i].message);

			kgsl_pwrctrl_irq(rb->device, KGSL_PWRFLAGS_OFF);
		}
	}

	
	status &= CP_INT_MASK;
	adreno_regwrite(device, REG_CP_INT_ACK, status);

	if (status & (CP_INT_CNTL__IB1_INT_MASK | CP_INT_CNTL__RB_INT_MASK)) {
		KGSL_CMD_WARN(rb->device, "ringbuffer ib1/rb interrupt\n");
		queue_work(device->work_queue, &device->ts_expired_ws);
		wake_up_interruptible_all(&device->wait_queue);
	}
}

static void a2xx_rbbm_intrcallback(struct kgsl_device *device)
{
	unsigned int status = 0;
	unsigned int rderr = 0;
	unsigned int addr = 0;
	const char *source;

	adreno_regread(device, REG_RBBM_INT_STATUS, &status);

	if (status & RBBM_INT_CNTL__RDERR_INT_MASK) {
		adreno_regread(device, REG_RBBM_READ_ERROR, &rderr);
		source = (rderr & RBBM_READ_ERROR_REQUESTER)
			 ? "host" : "cp";
		
		addr = (rderr & RBBM_READ_ERROR_ADDRESS_MASK) >> 2;

		if (addr == REG_CP_INT_STATUS &&
			rderr & RBBM_READ_ERROR_ERROR &&
			rderr & RBBM_READ_ERROR_REQUESTER)
			KGSL_DRV_WARN(device,
				"rbbm read error interrupt: %s reg: %04X\n",
				source, addr);
		else
			KGSL_DRV_CRIT(device,
				"rbbm read error interrupt: %s reg: %04X\n",
				source, addr);
	}

	status &= RBBM_INT_MASK;
	adreno_regwrite(device, REG_RBBM_INT_ACK, status);
}

irqreturn_t a2xx_irq_handler(struct adreno_device *adreno_dev)
{
	struct kgsl_device *device = &adreno_dev->dev;
	irqreturn_t result = IRQ_NONE;
	unsigned int status;

	adreno_regread(device, REG_MASTER_INT_SIGNAL, &status);

	if (status & MASTER_INT_SIGNAL__MH_INT_STAT) {
		kgsl_mh_intrcallback(device);
		result = IRQ_HANDLED;
	}

	if (status & MASTER_INT_SIGNAL__CP_INT_STAT) {
		a2xx_cp_intrcallback(device);
		result = IRQ_HANDLED;
	}

	if (status & MASTER_INT_SIGNAL__RBBM_INT_STAT) {
		a2xx_rbbm_intrcallback(device);
		result = IRQ_HANDLED;
	}

	return result;
}

static void a2xx_irq_control(struct adreno_device *adreno_dev, int state)
{
	struct kgsl_device *device = &adreno_dev->dev;

	if (state) {
		adreno_regwrite(device, REG_RBBM_INT_CNTL, RBBM_INT_MASK);
		adreno_regwrite(device, REG_CP_INT_CNTL, CP_INT_MASK);
		adreno_regwrite(device, MH_INTERRUPT_MASK,
			kgsl_mmu_get_int_mask());
	} else {
		adreno_regwrite(device, REG_RBBM_INT_CNTL, 0);
		adreno_regwrite(device, REG_CP_INT_CNTL, 0);
		adreno_regwrite(device, MH_INTERRUPT_MASK, 0);
	}

	
	wmb();
}

static unsigned int a2xx_irq_pending(struct adreno_device *adreno_dev)
{
	struct kgsl_device *device = &adreno_dev->dev;
	unsigned int status;

	adreno_regread(device, REG_MASTER_INT_SIGNAL, &status);

	return (status &
		(MASTER_INT_SIGNAL__MH_INT_STAT |
		 MASTER_INT_SIGNAL__CP_INT_STAT |
		 MASTER_INT_SIGNAL__RBBM_INT_STAT)) ? 1 : 0;
}

static int a2xx_rb_init(struct adreno_device *adreno_dev,
			struct adreno_ringbuffer *rb)
{
	unsigned int *cmds, cmds_gpu;

	
	cmds = adreno_ringbuffer_allocspace(rb, NULL, 19);
	if (cmds == NULL)
		return -ENOMEM;

	cmds_gpu = rb->buffer_desc.gpuaddr + sizeof(uint)*(rb->wptr-19);

	GSL_RB_WRITE(cmds, cmds_gpu, cp_type3_packet(CP_ME_INIT, 18));
	
	GSL_RB_WRITE(cmds, cmds_gpu, 0x000003ff);
	
	GSL_RB_WRITE(cmds, cmds_gpu, 0x00000000);
	
	GSL_RB_WRITE(cmds, cmds_gpu, 0x00000000);

	GSL_RB_WRITE(cmds, cmds_gpu,
		SUBBLOCK_OFFSET(REG_RB_SURFACE_INFO));
	GSL_RB_WRITE(cmds, cmds_gpu,
		SUBBLOCK_OFFSET(REG_PA_SC_WINDOW_OFFSET));
	GSL_RB_WRITE(cmds, cmds_gpu,
		SUBBLOCK_OFFSET(REG_VGT_MAX_VTX_INDX));
	GSL_RB_WRITE(cmds, cmds_gpu,
		SUBBLOCK_OFFSET(REG_SQ_PROGRAM_CNTL));
	GSL_RB_WRITE(cmds, cmds_gpu,
		SUBBLOCK_OFFSET(REG_RB_DEPTHCONTROL));
	GSL_RB_WRITE(cmds, cmds_gpu,
		SUBBLOCK_OFFSET(REG_PA_SU_POINT_SIZE));
	GSL_RB_WRITE(cmds, cmds_gpu,
		SUBBLOCK_OFFSET(REG_PA_SC_LINE_CNTL));
	GSL_RB_WRITE(cmds, cmds_gpu,
		SUBBLOCK_OFFSET(REG_PA_SU_POLY_OFFSET_FRONT_SCALE));

	
	GSL_RB_WRITE(cmds, cmds_gpu,
		(adreno_encode_istore_size(adreno_dev)
		| adreno_dev->pix_shader_start));
	
	GSL_RB_WRITE(cmds, cmds_gpu, 0x00000001);
	GSL_RB_WRITE(cmds, cmds_gpu, 0x00000000);

	
	GSL_RB_WRITE(cmds, cmds_gpu, 0x00000000);
	if (KGSL_MMU_TYPE_IOMMU == kgsl_mmu_get_mmutype())
		GSL_RB_WRITE(cmds, cmds_gpu, 0);
	else
		GSL_RB_WRITE(cmds, cmds_gpu, GSL_RB_PROTECTED_MODE_CONTROL);
	
	GSL_RB_WRITE(cmds, cmds_gpu, 0x00000000);
	
	GSL_RB_WRITE(cmds, cmds_gpu, 0x00000000);

	adreno_ringbuffer_submit(rb);

	return 0;
}

static unsigned int a2xx_busy_cycles(struct adreno_device *adreno_dev)
{
	struct kgsl_device *device = &adreno_dev->dev;
	unsigned int reg, val;

	
	adreno_regwrite(device, REG_CP_PERFMON_CNTL,
		REG_PERF_MODE_CNT | REG_PERF_STATE_FREEZE);

	
	adreno_regread(device, REG_RBBM_PERFCOUNTER1_LO, &val);

	
	adreno_regwrite(device, REG_CP_PERFMON_CNTL,
		REG_PERF_MODE_CNT | REG_PERF_STATE_RESET);

	
	adreno_regread(device, REG_RBBM_PM_OVERRIDE2, &reg);
	adreno_regwrite(device, REG_RBBM_PM_OVERRIDE2, (reg | 0x40));
	adreno_regwrite(device, REG_RBBM_PERFCOUNTER1_SELECT, 0x1);
	adreno_regwrite(device, REG_CP_PERFMON_CNTL,
		REG_PERF_MODE_CNT | REG_PERF_STATE_ENABLE);

	return val;
}

static void a2xx_gmeminit(struct adreno_device *adreno_dev)
{
	struct kgsl_device *device = &adreno_dev->dev;
	union reg_rb_edram_info rb_edram_info;
	unsigned int gmem_size;
	unsigned int edram_value = 0;

	
	gmem_size = (adreno_dev->gmem_size >> 14);
	while (gmem_size >>= 1)
		edram_value++;

	rb_edram_info.val = 0;

	rb_edram_info.f.edram_size = edram_value;
	rb_edram_info.f.edram_mapping_mode = 0; 

	
	rb_edram_info.f.edram_range = (adreno_dev->gmem_base >> 14);

	adreno_regwrite(device, REG_RB_EDRAM_INFO, rb_edram_info.val);
}

static void a2xx_start(struct adreno_device *adreno_dev)
{
	struct kgsl_device *device = &adreno_dev->dev;

	adreno_regwrite(device, REG_RBBM_PM_OVERRIDE1, 0xfffffffe);
	adreno_regwrite(device, REG_RBBM_PM_OVERRIDE2, 0xffffffff);

	if (!(device->flags & KGSL_FLAGS_SOFT_RESET) ||
		!adreno_is_a22x(adreno_dev)) {
		adreno_regwrite(device, REG_RBBM_SOFT_RESET,
			0xFFFFFFFF);
		device->flags |= KGSL_FLAGS_SOFT_RESET;
	} else {
		adreno_regwrite(device, REG_RBBM_SOFT_RESET,
			0x00000001);
	}
	msleep(30);

	adreno_regwrite(device, REG_RBBM_SOFT_RESET, 0x00000000);

	if (adreno_is_a225(adreno_dev)) {
		
		adreno_regwrite(device, REG_SQ_FLOW_CONTROL,
			0x18000000);
	}

	if (adreno_is_a20x(adreno_dev))
		adreno_regwrite(device, REG_RBBM_CNTL, 0x0000FFFF);
	else
		adreno_regwrite(device, REG_RBBM_CNTL, 0x00004442);

	adreno_regwrite(device, REG_SQ_VS_PROGRAM, 0x00000000);
	adreno_regwrite(device, REG_SQ_PS_PROGRAM, 0x00000000);

	if (cpu_is_msm8960())
		adreno_regwrite(device, REG_RBBM_PM_OVERRIDE1, 0x200);
	else
		adreno_regwrite(device, REG_RBBM_PM_OVERRIDE1, 0);

	if (!adreno_is_a22x(adreno_dev))
		adreno_regwrite(device, REG_RBBM_PM_OVERRIDE2, 0);
	else
		adreno_regwrite(device, REG_RBBM_PM_OVERRIDE2, 0x80);

	adreno_regwrite(device, REG_RBBM_DEBUG, 0x00080000);

	
	adreno_regwrite(device, REG_RBBM_INT_CNTL, 0);
	adreno_regwrite(device, REG_CP_INT_CNTL, 0);
	adreno_regwrite(device, REG_SQ_INT_CNTL, 0);

	a2xx_gmeminit(adreno_dev);
}

void *a2xx_snapshot(struct adreno_device *adreno_dev, void *snapshot,
	int *remain, int hang);

struct adreno_gpudev adreno_a2xx_gpudev = {
	.reg_rbbm_status = REG_RBBM_STATUS,
	.reg_cp_pfp_ucode_addr = REG_CP_PFP_UCODE_ADDR,
	.reg_cp_pfp_ucode_data = REG_CP_PFP_UCODE_DATA,

	.ctxt_create = a2xx_drawctxt_create,
	.ctxt_save = a2xx_drawctxt_save,
	.ctxt_restore = a2xx_drawctxt_restore,
	.ctxt_draw_workaround = a2xx_drawctxt_draw_workaround,
	.irq_handler = a2xx_irq_handler,
	.irq_control = a2xx_irq_control,
	.irq_pending = a2xx_irq_pending,
	.snapshot = a2xx_snapshot,
	.rb_init = a2xx_rb_init,
	.busy_cycles = a2xx_busy_cycles,
	.start = a2xx_start,
};
