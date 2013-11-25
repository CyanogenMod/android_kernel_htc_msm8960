/* Copyright (c) 2002,2007-2012, The Linux Foundation. All rights reserved.
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
#ifndef __ADRENO_PM4TYPES_H
#define __ADRENO_PM4TYPES_H


#define CP_PKT_MASK	0xc0000000

#define CP_TYPE0_PKT	((unsigned int)0 << 30)
#define CP_TYPE1_PKT	((unsigned int)1 << 30)
#define CP_TYPE2_PKT	((unsigned int)2 << 30)
#define CP_TYPE3_PKT	((unsigned int)3 << 30)


#define CP_ME_INIT		0x48

#define CP_NOP			0x10

#define CP_INDIRECT_BUFFER_PFD	0x37

#define CP_WAIT_FOR_IDLE	0x26

#define CP_WAIT_REG_MEM	0x3c

#define CP_WAIT_REG_EQ		0x52

#define CP_WAT_REG_GTE		0x53

#define CP_WAIT_UNTIL_READ	0x5c

#define CP_WAIT_IB_PFD_COMPLETE 0x5d

#define CP_REG_RMW		0x21

#define CP_SET_BIN_DATA             0x2f

#define CP_REG_TO_MEM		0x3e

#define CP_MEM_WRITE		0x3d

#define CP_MEM_WRITE_CNTR	0x4f

#define CP_COND_EXEC		0x44

#define CP_COND_WRITE		0x45

#define CP_EVENT_WRITE		0x46

#define CP_EVENT_WRITE_SHD	0x58

#define CP_EVENT_WRITE_CFL	0x59

#define CP_EVENT_WRITE_ZPD	0x5b


#define CP_DRAW_INDX		0x22

#define CP_DRAW_INDX_2		0x36

#define CP_DRAW_INDX_BIN	0x34

#define CP_DRAW_INDX_2_BIN	0x35


#define CP_VIZ_QUERY		0x23

#define CP_SET_STATE		0x25

#define CP_SET_CONSTANT	0x2d

#define CP_IM_LOAD		0x27

#define CP_IM_LOAD_IMMEDIATE	0x2b

#define CP_LOAD_CONSTANT_CONTEXT 0x2e

#define CP_SET_BIN_DATA             0x2f

#define CP_INVALIDATE_STATE	0x3b


#define CP_SET_SHADER_BASES	0x4A

#define CP_SET_BIN_MASK	0x50

#define CP_SET_BIN_SELECT	0x51


#define CP_CONTEXT_UPDATE	0x5e

#define CP_INTERRUPT		0x40


#define CP_IM_STORE            0x2c

#define CP_TEST_TWO_MEMS    0x71

#define CP_WAIT_FOR_ME      0x13

#define CP_SET_BIN_BASE_OFFSET     0x4B

#define CP_SET_DRAW_INIT_FLAGS      0x4B

#define CP_SET_PROTECTED_MODE  0x5f 


#define CP_LOAD_STATE 0x30 

#define CP_COND_INDIRECT_BUFFER_PFE 0x3A 
#define CP_COND_INDIRECT_BUFFER_PFD 0x32 

#define CP_INDIRECT_BUFFER_PFE 0x3F

#define CP_LOADSTATE_DSTOFFSET_SHIFT 0x00000000
#define CP_LOADSTATE_STATESRC_SHIFT 0x00000010
#define CP_LOADSTATE_STATEBLOCKID_SHIFT 0x00000013
#define CP_LOADSTATE_NUMOFUNITS_SHIFT 0x00000016
#define CP_LOADSTATE_STATETYPE_SHIFT 0x00000000
#define CP_LOADSTATE_EXTSRCADDR_SHIFT 0x00000002

#define cp_type0_packet(regindx, cnt) \
	(CP_TYPE0_PKT | (((cnt)-1) << 16) | ((regindx) & 0x7FFF))

#define cp_type0_packet_for_sameregister(regindx, cnt) \
	((CP_TYPE0_PKT | (((cnt)-1) << 16) | ((1 << 15) | \
		((regindx) & 0x7FFF)))

#define cp_type1_packet(reg0, reg1) \
	 (CP_TYPE1_PKT | ((reg1) << 12) | (reg0))

#define cp_type3_packet(opcode, cnt) \
	 (CP_TYPE3_PKT | (((cnt)-1) << 16) | (((opcode) & 0xFF) << 8))

#define cp_predicated_type3_packet(opcode, cnt) \
	 (CP_TYPE3_PKT | (((cnt)-1) << 16) | (((opcode) & 0xFF) << 8) | 0x1)

#define cp_nop_packet(cnt) \
	 (CP_TYPE3_PKT | (((cnt)-1) << 16) | (CP_NOP << 8))

#define pkt_is_type0(pkt) (((pkt) & 0XC0000000) == CP_TYPE0_PKT)

#define type0_pkt_size(pkt) ((((pkt) >> 16) & 0x3FFF) + 1)
#define type0_pkt_offset(pkt) ((pkt) & 0x7FFF)


#define pkt_is_type3(pkt) \
	((((pkt) & 0xC0000000) == CP_TYPE3_PKT) && \
	 (((pkt) & 0x80FE) == 0))

#define cp_type3_opcode(pkt) (((pkt) >> 8) & 0xFF)
#define type3_pkt_size(pkt) ((((pkt) >> 16) & 0x3FFF) + 1)

#define CP_HDR_ME_INIT	cp_type3_packet(CP_ME_INIT, 18)
#define CP_HDR_INDIRECT_BUFFER_PFD cp_type3_packet(CP_INDIRECT_BUFFER_PFD, 2)
#define CP_HDR_INDIRECT_BUFFER_PFE cp_type3_packet(CP_INDIRECT_BUFFER_PFE, 2)

#define SUBBLOCK_OFFSET(reg) ((unsigned int)((reg) - (0x2000)))

#define CP_REG(reg) ((0x4 << 16) | (SUBBLOCK_OFFSET(reg)))


static inline int adreno_cmd_is_ib(unsigned int cmd)
{
	return (cmd == cp_type3_packet(CP_INDIRECT_BUFFER_PFE, 2) ||
		cmd == cp_type3_packet(CP_INDIRECT_BUFFER_PFD, 2) ||
		cmd == cp_type3_packet(CP_COND_INDIRECT_BUFFER_PFE, 2) ||
		cmd == cp_type3_packet(CP_COND_INDIRECT_BUFFER_PFD, 2));
}

#endif	
