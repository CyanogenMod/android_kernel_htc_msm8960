/*
 *  include/linux/mmc/sdio.h
 *
 *  Copyright 2006-2007 Pierre Ossman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#ifndef LINUX_MMC_SDIO_H
#define LINUX_MMC_SDIO_H

#define SD_IO_SEND_OP_COND          5 
#define SD_IO_RW_DIRECT            52 
#define SD_IO_RW_EXTENDED          53 



#define R4_18V_PRESENT (1<<24)
#define R4_MEMORY_PRESENT (1 << 27)


#define R5_COM_CRC_ERROR	(1 << 15)	
#define R5_ILLEGAL_COMMAND	(1 << 14)	
#define R5_ERROR		(1 << 11)	
#define R5_FUNCTION_NUMBER	(1 << 9)	
#define R5_OUT_OF_RANGE		(1 << 8)	
#define R5_STATUS(x)		(x & 0xCB00)
#define R5_IO_CURRENT_STATE(x)	((x & 0x3000) >> 12) 


#define SDIO_CCCR_CCCR		0x00

#define  SDIO_CCCR_REV_1_00	0	
#define  SDIO_CCCR_REV_1_10	1	
#define  SDIO_CCCR_REV_1_20	2	
#define  SDIO_CCCR_REV_3_00	3	

#define  SDIO_SDIO_REV_1_00	0	
#define  SDIO_SDIO_REV_1_10	1	
#define  SDIO_SDIO_REV_1_20	2	
#define  SDIO_SDIO_REV_2_00	3	
#define  SDIO_SDIO_REV_3_00	4	

#define SDIO_CCCR_SD		0x01

#define  SDIO_SD_REV_1_01	0	
#define  SDIO_SD_REV_1_10	1	
#define  SDIO_SD_REV_2_00	2	
#define  SDIO_SD_REV_3_00	3	

#define SDIO_CCCR_IOEx		0x02
#define SDIO_CCCR_IORx		0x03

#define SDIO_CCCR_IENx		0x04	
#define SDIO_CCCR_INTx		0x05	

#define SDIO_CCCR_ABORT		0x06	

#define SDIO_CCCR_IF		0x07	

#define  SDIO_BUS_WIDTH_1BIT	0x00
#define  SDIO_BUS_WIDTH_4BIT	0x02
#define  SDIO_BUS_WIDTH_8BIT  	0x03
#define  SDIO_BUS_ECSI		0x20	
#define  SDIO_BUS_SCSI		0x40	

#define  SDIO_BUS_ASYNC_INT	0x20

#define  SDIO_BUS_CD_DISABLE     0x80	

#define SDIO_CCCR_CAPS		0x08

#define  SDIO_CCCR_CAP_SDC	0x01	
#define  SDIO_CCCR_CAP_SMB	0x02	
#define  SDIO_CCCR_CAP_SRW	0x04	
#define  SDIO_CCCR_CAP_SBS	0x08	
#define  SDIO_CCCR_CAP_S4MI	0x10	
#define  SDIO_CCCR_CAP_E4MI	0x20	
#define  SDIO_CCCR_CAP_LSC	0x40	
#define  SDIO_CCCR_CAP_4BLS	0x80	

#define SDIO_CCCR_CIS		0x09	

#define SDIO_CCCR_SUSPEND	0x0c
#define SDIO_CCCR_SELx		0x0d
#define SDIO_CCCR_EXECx		0x0e
#define SDIO_CCCR_READYx	0x0f

#define SDIO_CCCR_BLKSIZE	0x10

#define SDIO_CCCR_POWER		0x12

#define  SDIO_POWER_SMPC	0x01	
#define  SDIO_POWER_EMPC	0x02	

#define SDIO_CCCR_SPEED		0x13

#define  SDIO_SPEED_SHS		0x01	
#define  SDIO_SPEED_BSS_SHIFT	1
#define  SDIO_SPEED_BSS_MASK	(7<<SDIO_SPEED_BSS_SHIFT)
#define  SDIO_SPEED_SDR12	(0<<SDIO_SPEED_BSS_SHIFT)
#define  SDIO_SPEED_SDR25	(1<<SDIO_SPEED_BSS_SHIFT)
#define  SDIO_SPEED_SDR50	(2<<SDIO_SPEED_BSS_SHIFT)
#define  SDIO_SPEED_SDR104	(3<<SDIO_SPEED_BSS_SHIFT)
#define  SDIO_SPEED_DDR50	(4<<SDIO_SPEED_BSS_SHIFT)
#define  SDIO_SPEED_EHS		SDIO_SPEED_SDR25	

#define SDIO_CCCR_UHS		0x14
#define  SDIO_UHS_SDR50		0x01
#define  SDIO_UHS_SDR104	0x02
#define  SDIO_UHS_DDR50		0x04

#define SDIO_CCCR_DRIVE_STRENGTH 0x15
#define  SDIO_SDTx_MASK		0x07
#define  SDIO_DRIVE_SDTA	(1<<0)
#define  SDIO_DRIVE_SDTC	(1<<1)
#define  SDIO_DRIVE_SDTD	(1<<2)
#define  SDIO_DRIVE_DTSx_MASK	0x03
#define  SDIO_DRIVE_DTSx_SHIFT	4
#define  SDIO_DTSx_SET_TYPE_B	(0 << SDIO_DRIVE_DTSx_SHIFT)
#define  SDIO_DTSx_SET_TYPE_A	(1 << SDIO_DRIVE_DTSx_SHIFT)
#define  SDIO_DTSx_SET_TYPE_C	(2 << SDIO_DRIVE_DTSx_SHIFT)
#define  SDIO_DTSx_SET_TYPE_D	(3 << SDIO_DRIVE_DTSx_SHIFT)

#define SDIO_FBR_BASE(f)	((f) * 0x100) 

#define SDIO_FBR_STD_IF		0x00

#define  SDIO_FBR_SUPPORTS_CSA	0x40	
#define  SDIO_FBR_ENABLE_CSA	0x80	

#define SDIO_FBR_STD_IF_EXT	0x01

#define SDIO_FBR_POWER		0x02

#define  SDIO_FBR_POWER_SPS	0x01	
#define  SDIO_FBR_POWER_EPS	0x02	

#define SDIO_FBR_CIS		0x09	


#define SDIO_FBR_CSA		0x0C	

#define SDIO_FBR_CSA_DATA	0x0F

#define SDIO_FBR_BLKSIZE	0x10	

#endif 
