/*
 * SDIO spec header file
 * Protocol and standard (common) device definitions
 *
 * Copyright (C) 1999-2012, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: sdio.h 308973 2012-01-18 04:19:34Z $
 */

#ifndef	_SDIO_H
#define	_SDIO_H


typedef volatile struct {
	uint8	cccr_sdio_rev;		
	uint8	sd_rev;			
	uint8	io_en;			
	uint8	io_rdy;			
	uint8	intr_ctl;		
	uint8	intr_status;		
	uint8	io_abort;		
	uint8	bus_inter;		
	uint8	capability;		

	uint8	cis_base_low;		
	uint8	cis_base_mid;
	uint8	cis_base_high;		

	
	uint8	bus_suspend;		
	uint8	func_select;		
	uint8	exec_flag;		
	uint8	ready_flag;		

	uint8	fn0_blk_size[2];	

	uint8	power_control;		

	uint8	speed_control;		
} sdio_regs_t;

#define SDIOD_CCCR_REV			0x00
#define SDIOD_CCCR_SDREV		0x01
#define SDIOD_CCCR_IOEN			0x02
#define SDIOD_CCCR_IORDY		0x03
#define SDIOD_CCCR_INTEN		0x04
#define SDIOD_CCCR_INTPEND		0x05
#define SDIOD_CCCR_IOABORT		0x06
#define SDIOD_CCCR_BICTRL		0x07
#define SDIOD_CCCR_CAPABLITIES		0x08
#define SDIOD_CCCR_CISPTR_0		0x09
#define SDIOD_CCCR_CISPTR_1		0x0A
#define SDIOD_CCCR_CISPTR_2		0x0B
#define SDIOD_CCCR_BUSSUSP		0x0C
#define SDIOD_CCCR_FUNCSEL		0x0D
#define SDIOD_CCCR_EXECFLAGS		0x0E
#define SDIOD_CCCR_RDYFLAGS		0x0F
#define SDIOD_CCCR_BLKSIZE_0		0x10
#define SDIOD_CCCR_BLKSIZE_1		0x11
#define SDIOD_CCCR_POWER_CONTROL	0x12
#define SDIOD_CCCR_SPEED_CONTROL	0x13
#define SDIOD_CCCR_UHSI_SUPPORT		0x14
#define SDIOD_CCCR_DRIVER_STRENGTH	0x15
#define SDIOD_CCCR_INTR_EXTN		0x16

#define SDIOD_CCCR_BRCM_CARDCAP			0xf0
#define SDIOD_CCCR_BRCM_CARDCAP_CMD14_SUPPORT	0x02
#define SDIOD_CCCR_BRCM_CARDCAP_CMD14_EXT	0x04
#define SDIOD_CCCR_BRCM_CARDCAP_CMD_NODEC	0x08
#define SDIOD_CCCR_BRCM_CARDCTL			0xf1
#define SDIOD_CCCR_BRCM_SEPINT			0xf2

#define SDIO_REV_SDIOID_MASK	0xf0	
#define SDIO_REV_CCCRID_MASK	0x0f	

#define SD_REV_PHY_MASK		0x0f	

#define SDIO_FUNC_ENABLE_1	0x02	
#define SDIO_FUNC_ENABLE_2	0x04	

#define SDIO_FUNC_READY_1	0x02	
#define SDIO_FUNC_READY_2	0x04	

#define INTR_CTL_MASTER_EN	0x1	
#define INTR_CTL_FUNC1_EN	0x2	
#define INTR_CTL_FUNC2_EN	0x4	

#define INTR_STATUS_FUNC1	0x2	
#define INTR_STATUS_FUNC2	0x4	

#define IO_ABORT_RESET_ALL	0x08	
#define IO_ABORT_FUNC_MASK	0x07	

#define BUS_CARD_DETECT_DIS	0x80	
#define BUS_SPI_CONT_INTR_CAP	0x40	
#define BUS_SPI_CONT_INTR_EN	0x20	
#define BUS_SD_DATA_WIDTH_MASK	0x03	
#define BUS_SD_DATA_WIDTH_4BIT	0x02	
#define BUS_SD_DATA_WIDTH_1BIT	0x00	

#define SDIO_CAP_4BLS		0x80	
#define SDIO_CAP_LSC		0x40	
#define SDIO_CAP_E4MI		0x20	
#define SDIO_CAP_S4MI		0x10	
#define SDIO_CAP_SBS		0x08	
#define SDIO_CAP_SRW		0x04	
#define SDIO_CAP_SMB		0x02	
#define SDIO_CAP_SDC		0x01	

#define SDIO_POWER_SMPC		0x01	
#define SDIO_POWER_EMPC		0x02	

#define SDIO_SPEED_SHS		0x01	
#if 0
#define SDIO_SPEED_EHS		0x02	
#else
#define SDIO_SPEED_EHS		SDIO_SPEED_SDR25	
#endif

#define SDIO_BUS_SPEED_UHSISEL_M	BITFIELD_MASK(3)
#define SDIO_BUS_SPEED_UHSISEL_S	1

#define SDIO_BUS_SPEED_UHSICAP_M	BITFIELD_MASK(3)
#define SDIO_BUS_SPEED_UHSICAP_S	0

#define SDIO_BUS_DRVR_TYPE_CAP_M	BITFIELD_MASK(3)
#define SDIO_BUS_DRVR_TYPE_CAP_S	0

#define SDIO_BUS_DRVR_TYPE_SEL_M	BITFIELD_MASK(2)
#define SDIO_BUS_DRVR_TYPE_SEL_S	4

#define SDIO_BUS_ASYNCINT_CAP_M	BITFIELD_MASK(1)
#define SDIO_BUS_ASYNCINT_CAP_S	0

#define SDIO_BUS_ASYNCINT_SEL_M	BITFIELD_MASK(1)
#define SDIO_BUS_ASYNCINT_SEL_S	1

#define SDIO_SEPINT_MASK	0x01	
#define SDIO_SEPINT_OE		0x02	
#define SDIO_SEPINT_ACT_HI	0x04	

typedef volatile struct {
	uint8	devctr;			
	uint8	ext_dev;		
	uint8	pwr_sel;		
	uint8	PAD[6];			

	uint8	cis_low;		
	uint8	cis_mid;
	uint8	cis_high;		
	uint8	csa_low;		
	uint8	csa_mid;
	uint8	csa_high;		
	uint8	csa_dat_win;		

	uint8	fnx_blk_size[2];	
} sdio_fbr_t;

#define SDIOD_MAX_FUNCS			8
#define SDIOD_MAX_IOFUNCS		7

#define SDIOD_FBR_STARTADDR		0x100

#define SDIOD_FBR_SIZE			0x100

#define SDIOD_FBR_BASE(n)		((n) * 0x100)

#define SDIOD_FBR_DEVCTR		0x00	
#define SDIOD_FBR_EXT_DEV		0x01	
#define SDIOD_FBR_PWR_SEL		0x02	

#define SDIOD_FBR_CISPTR_0		0x09
#define SDIOD_FBR_CISPTR_1		0x0A
#define SDIOD_FBR_CISPTR_2		0x0B

#define SDIOD_FBR_CSA_ADDR_0		0x0C
#define SDIOD_FBR_CSA_ADDR_1		0x0D
#define SDIOD_FBR_CSA_ADDR_2		0x0E
#define SDIOD_FBR_CSA_DATA		0x0F

#define SDIOD_FBR_BLKSIZE_0		0x10
#define SDIOD_FBR_BLKSIZE_1		0x11

#define SDIOD_FBR_DEVCTR_DIC	0x0f	
#define SDIOD_FBR_DECVTR_CSA	0x40	
#define SDIOD_FBR_DEVCTR_CSA_EN	0x80	
#define SDIOD_DIC_NONE		0	
#define SDIOD_DIC_UART		1
#define SDIOD_DIC_BLUETOOTH_A	2
#define SDIOD_DIC_BLUETOOTH_B	3
#define SDIOD_DIC_GPS		4
#define SDIOD_DIC_CAMERA	5
#define SDIOD_DIC_PHS		6
#define SDIOD_DIC_WLAN		7
#define SDIOD_DIC_EXT		0xf	

#define SDIOD_PWR_SEL_SPS	0x01	
#define SDIOD_PWR_SEL_EPS	0x02	

#define SDIO_FUNC_0		0
#define SDIO_FUNC_1		1
#define SDIO_FUNC_2		2
#define SDIO_FUNC_3		3
#define SDIO_FUNC_4		4
#define SDIO_FUNC_5		5
#define SDIO_FUNC_6		6
#define SDIO_FUNC_7		7

#define SD_CARD_TYPE_UNKNOWN	0	
#define SD_CARD_TYPE_IO		1	
#define SD_CARD_TYPE_MEMORY	2	
#define SD_CARD_TYPE_COMBO	3	

#define SDIO_MAX_BLOCK_SIZE	2048	
#define SDIO_MIN_BLOCK_SIZE	1	

#define CARDREG_STATUS_BIT_OUTOFRANGE		31
#define CARDREG_STATUS_BIT_COMCRCERROR		23
#define CARDREG_STATUS_BIT_ILLEGALCOMMAND	22
#define CARDREG_STATUS_BIT_ERROR		19
#define CARDREG_STATUS_BIT_IOCURRENTSTATE3	12
#define CARDREG_STATUS_BIT_IOCURRENTSTATE2	11
#define CARDREG_STATUS_BIT_IOCURRENTSTATE1	10
#define CARDREG_STATUS_BIT_IOCURRENTSTATE0	9
#define CARDREG_STATUS_BIT_FUN_NUM_ERROR	4



#define SD_CMD_GO_IDLE_STATE		0	
#define SD_CMD_SEND_OPCOND		1
#define SD_CMD_MMC_SET_RCA		3
#define SD_CMD_IO_SEND_OP_COND		5	
#define SD_CMD_SELECT_DESELECT_CARD	7
#define SD_CMD_SEND_CSD			9
#define SD_CMD_SEND_CID			10
#define SD_CMD_STOP_TRANSMISSION	12
#define SD_CMD_SEND_STATUS		13
#define SD_CMD_GO_INACTIVE_STATE	15
#define SD_CMD_SET_BLOCKLEN		16
#define SD_CMD_READ_SINGLE_BLOCK	17
#define SD_CMD_READ_MULTIPLE_BLOCK	18
#define SD_CMD_WRITE_BLOCK		24
#define SD_CMD_WRITE_MULTIPLE_BLOCK	25
#define SD_CMD_PROGRAM_CSD		27
#define SD_CMD_SET_WRITE_PROT		28
#define SD_CMD_CLR_WRITE_PROT		29
#define SD_CMD_SEND_WRITE_PROT		30
#define SD_CMD_ERASE_WR_BLK_START	32
#define SD_CMD_ERASE_WR_BLK_END		33
#define SD_CMD_ERASE			38
#define SD_CMD_LOCK_UNLOCK		42
#define SD_CMD_IO_RW_DIRECT		52	
#define SD_CMD_IO_RW_EXTENDED		53	
#define SD_CMD_APP_CMD			55
#define SD_CMD_GEN_CMD			56
#define SD_CMD_READ_OCR			58
#define SD_CMD_CRC_ON_OFF		59	
#define SD_ACMD_SD_STATUS		13
#define SD_ACMD_SEND_NUM_WR_BLOCKS	22
#define SD_ACMD_SET_WR_BLOCK_ERASE_CNT	23
#define SD_ACMD_SD_SEND_OP_COND		41
#define SD_ACMD_SET_CLR_CARD_DETECT	42
#define SD_ACMD_SEND_SCR		51

#define SD_IO_OP_READ		0   
#define SD_IO_OP_WRITE		1   
#define SD_IO_RW_NORMAL		0   
#define SD_IO_RW_RAW		1   
#define SD_IO_BYTE_MODE		0   
#define SD_IO_BLOCK_MODE	1   
#define SD_IO_FIXED_ADDRESS	0   
#define SD_IO_INCREMENT_ADDRESS	1   

#define SDIO_IO_RW_DIRECT_ARG(rw, raw, func, addr, data) \
	((((rw) & 1) << 31) | (((func) & 0x7) << 28) | (((raw) & 1) << 27) | \
	 (((addr) & 0x1FFFF) << 9) | ((data) & 0xFF))

#define SDIO_IO_RW_EXTENDED_ARG(rw, blk, func, addr, inc_addr, count) \
	((((rw) & 1) << 31) | (((func) & 0x7) << 28) | (((blk) & 1) << 27) | \
	 (((inc_addr) & 1) << 26) | (((addr) & 0x1FFFF) << 9) | ((count) & 0x1FF))

#define SD_RSP_NO_NONE			0
#define SD_RSP_NO_1			1
#define SD_RSP_NO_2			2
#define SD_RSP_NO_3			3
#define SD_RSP_NO_4			4
#define SD_RSP_NO_5			5
#define SD_RSP_NO_6			6

	
#define SD_RSP_MR6_COM_CRC_ERROR	0x8000
#define SD_RSP_MR6_ILLEGAL_COMMAND	0x4000
#define SD_RSP_MR6_ERROR		0x2000

	
#define SD_RSP_MR1_SBIT			0x80
#define SD_RSP_MR1_PARAMETER_ERROR	0x40
#define SD_RSP_MR1_RFU5			0x20
#define SD_RSP_MR1_FUNC_NUM_ERROR	0x10
#define SD_RSP_MR1_COM_CRC_ERROR	0x08
#define SD_RSP_MR1_ILLEGAL_COMMAND	0x04
#define SD_RSP_MR1_RFU1			0x02
#define SD_RSP_MR1_IDLE_STATE		0x01

	
#define SD_RSP_R5_COM_CRC_ERROR		0x80
#define SD_RSP_R5_ILLEGAL_COMMAND	0x40
#define SD_RSP_R5_IO_CURRENTSTATE1	0x20
#define SD_RSP_R5_IO_CURRENTSTATE0	0x10
#define SD_RSP_R5_ERROR			0x08
#define SD_RSP_R5_RFU			0x04
#define SD_RSP_R5_FUNC_NUM_ERROR	0x02
#define SD_RSP_R5_OUT_OF_RANGE		0x01

#define SD_RSP_R5_ERRBITS		0xCB



#define SDIOH_CMD_0		0
#define SDIOH_CMD_3		3
#define SDIOH_CMD_5		5
#define SDIOH_CMD_7		7
#define SDIOH_CMD_11		11
#define SDIOH_CMD_14		14
#define SDIOH_CMD_15		15
#define SDIOH_CMD_19		19
#define SDIOH_CMD_52		52
#define SDIOH_CMD_53		53
#define SDIOH_CMD_59		59

#define SDIOH_RSP_NONE		0
#define SDIOH_RSP_R1		1
#define SDIOH_RSP_R2		2
#define SDIOH_RSP_R3		3
#define SDIOH_RSP_R4		4
#define SDIOH_RSP_R5		5
#define SDIOH_RSP_R6		6

#define SDIOH_RSP5_ERROR_FLAGS	0xCB


#define CMD5_OCR_M		BITFIELD_MASK(24)
#define CMD5_OCR_S		0

#define CMD5_S18R_M		BITFIELD_MASK(1)
#define CMD5_S18R_S		24

#define CMD7_RCA_M		BITFIELD_MASK(16)
#define CMD7_RCA_S		16

#define CMD14_RCA_M		BITFIELD_MASK(16)
#define CMD14_RCA_S		16
#define CMD14_SLEEP_M		BITFIELD_MASK(1)
#define CMD14_SLEEP_S		15

#define CMD_15_RCA_M		BITFIELD_MASK(16)
#define CMD_15_RCA_S		16

#define CMD52_DATA_M		BITFIELD_MASK(8)  
#define CMD52_DATA_S		0
#define CMD52_REG_ADDR_M	BITFIELD_MASK(17) 
#define CMD52_REG_ADDR_S	9
#define CMD52_RAW_M		BITFIELD_MASK(1)  
#define CMD52_RAW_S		27
#define CMD52_FUNCTION_M	BITFIELD_MASK(3)  
#define CMD52_FUNCTION_S	28
#define CMD52_RW_FLAG_M		BITFIELD_MASK(1)  
#define CMD52_RW_FLAG_S		31


#define CMD53_BYTE_BLK_CNT_M	BITFIELD_MASK(9) 
#define CMD53_BYTE_BLK_CNT_S	0
#define CMD53_REG_ADDR_M	BITFIELD_MASK(17) 
#define CMD53_REG_ADDR_S	9
#define CMD53_OP_CODE_M		BITFIELD_MASK(1)  
#define CMD53_OP_CODE_S		26
#define CMD53_BLK_MODE_M	BITFIELD_MASK(1)  
#define CMD53_BLK_MODE_S	27
#define CMD53_FUNCTION_M	BITFIELD_MASK(3)  
#define CMD53_FUNCTION_S	28
#define CMD53_RW_FLAG_M		BITFIELD_MASK(1)  
#define CMD53_RW_FLAG_S		31

#define RSP4_IO_OCR_M		BITFIELD_MASK(24) 
#define RSP4_IO_OCR_S		0

#define RSP4_S18A_M			BITFIELD_MASK(1) 
#define RSP4_S18A_S			24

#define RSP4_STUFF_M		BITFIELD_MASK(3)  
#define RSP4_STUFF_S		24
#define RSP4_MEM_PRESENT_M	BITFIELD_MASK(1)  
#define RSP4_MEM_PRESENT_S	27
#define RSP4_NUM_FUNCS_M	BITFIELD_MASK(3)  
#define RSP4_NUM_FUNCS_S	28
#define RSP4_CARD_READY_M	BITFIELD_MASK(1)  
#define RSP4_CARD_READY_S	31

#define RSP6_STATUS_M		BITFIELD_MASK(16) 
#define RSP6_STATUS_S		0
#define RSP6_IO_RCA_M		BITFIELD_MASK(16) 
#define RSP6_IO_RCA_S		16

#define RSP1_AKE_SEQ_ERROR_M	BITFIELD_MASK(1)  
#define RSP1_AKE_SEQ_ERROR_S	3
#define RSP1_APP_CMD_M		BITFIELD_MASK(1)  
#define RSP1_APP_CMD_S		5
#define RSP1_READY_FOR_DATA_M	BITFIELD_MASK(1)  
#define RSP1_READY_FOR_DATA_S	8
#define RSP1_CURR_STATE_M	BITFIELD_MASK(4)  
#define RSP1_CURR_STATE_S	9
#define RSP1_EARSE_RESET_M	BITFIELD_MASK(1)  
#define RSP1_EARSE_RESET_S	13
#define RSP1_CARD_ECC_DISABLE_M	BITFIELD_MASK(1)  
#define RSP1_CARD_ECC_DISABLE_S	14
#define RSP1_WP_ERASE_SKIP_M	BITFIELD_MASK(1)  
#define RSP1_WP_ERASE_SKIP_S	15
#define RSP1_CID_CSD_OVERW_M	BITFIELD_MASK(1)  
#define RSP1_CID_CSD_OVERW_S	16
#define RSP1_ERROR_M		BITFIELD_MASK(1)  
#define RSP1_ERROR_S		19
#define RSP1_CC_ERROR_M		BITFIELD_MASK(1)  
#define RSP1_CC_ERROR_S		20
#define RSP1_CARD_ECC_FAILED_M	BITFIELD_MASK(1)  
#define RSP1_CARD_ECC_FAILED_S	21
#define RSP1_ILLEGAL_CMD_M	BITFIELD_MASK(1)  
#define RSP1_ILLEGAL_CMD_S	22
#define RSP1_COM_CRC_ERROR_M	BITFIELD_MASK(1)  
#define RSP1_COM_CRC_ERROR_S	23
#define RSP1_LOCK_UNLOCK_FAIL_M	BITFIELD_MASK(1)  
#define RSP1_LOCK_UNLOCK_FAIL_S	24
#define RSP1_CARD_LOCKED_M	BITFIELD_MASK(1)  
#define RSP1_CARD_LOCKED_S	25
#define RSP1_WP_VIOLATION_M	BITFIELD_MASK(1)  
#define RSP1_WP_VIOLATION_S	26
#define RSP1_ERASE_PARAM_M	BITFIELD_MASK(1)  
#define RSP1_ERASE_PARAM_S	27
#define RSP1_ERASE_SEQ_ERR_M	BITFIELD_MASK(1)  
#define RSP1_ERASE_SEQ_ERR_S	28
#define RSP1_BLK_LEN_ERR_M	BITFIELD_MASK(1)  
#define RSP1_BLK_LEN_ERR_S	29
#define RSP1_ADDR_ERR_M		BITFIELD_MASK(1)  
#define RSP1_ADDR_ERR_S		30
#define RSP1_OUT_OF_RANGE_M	BITFIELD_MASK(1)  
#define RSP1_OUT_OF_RANGE_S	31


#define RSP5_DATA_M		BITFIELD_MASK(8)  
#define RSP5_DATA_S		0
#define RSP5_FLAGS_M		BITFIELD_MASK(8)  
#define RSP5_FLAGS_S		8
#define RSP5_STUFF_M		BITFIELD_MASK(16) 
#define RSP5_STUFF_S		16

#define SPIRSP4_IO_OCR_M	BITFIELD_MASK(16) 
#define SPIRSP4_IO_OCR_S	0
#define SPIRSP4_STUFF_M		BITFIELD_MASK(3)  
#define SPIRSP4_STUFF_S		16
#define SPIRSP4_MEM_PRESENT_M	BITFIELD_MASK(1)  
#define SPIRSP4_MEM_PRESENT_S	19
#define SPIRSP4_NUM_FUNCS_M	BITFIELD_MASK(3)  
#define SPIRSP4_NUM_FUNCS_S	20
#define SPIRSP4_CARD_READY_M	BITFIELD_MASK(1)  
#define SPIRSP4_CARD_READY_S	23
#define SPIRSP4_IDLE_STATE_M	BITFIELD_MASK(1)  
#define SPIRSP4_IDLE_STATE_S	24
#define SPIRSP4_ILLEGAL_CMD_M	BITFIELD_MASK(1)  
#define SPIRSP4_ILLEGAL_CMD_S	26
#define SPIRSP4_COM_CRC_ERROR_M	BITFIELD_MASK(1)  
#define SPIRSP4_COM_CRC_ERROR_S	27
#define SPIRSP4_FUNC_NUM_ERROR_M	BITFIELD_MASK(1)  
#define SPIRSP4_FUNC_NUM_ERROR_S	28
#define SPIRSP4_PARAM_ERROR_M	BITFIELD_MASK(1)  
#define SPIRSP4_PARAM_ERROR_S	30
#define SPIRSP4_START_BIT_M	BITFIELD_MASK(1)  
#define SPIRSP4_START_BIT_S	31

#define SPIRSP5_DATA_M			BITFIELD_MASK(8)  
#define SPIRSP5_DATA_S			16
#define SPIRSP5_IDLE_STATE_M		BITFIELD_MASK(1)  
#define SPIRSP5_IDLE_STATE_S		24
#define SPIRSP5_ILLEGAL_CMD_M		BITFIELD_MASK(1)  
#define SPIRSP5_ILLEGAL_CMD_S		26
#define SPIRSP5_COM_CRC_ERROR_M		BITFIELD_MASK(1)  
#define SPIRSP5_COM_CRC_ERROR_S		27
#define SPIRSP5_FUNC_NUM_ERROR_M	BITFIELD_MASK(1)  
#define SPIRSP5_FUNC_NUM_ERROR_S	28
#define SPIRSP5_PARAM_ERROR_M		BITFIELD_MASK(1)  
#define SPIRSP5_PARAM_ERROR_S		30
#define SPIRSP5_START_BIT_M		BITFIELD_MASK(1)  
#define SPIRSP5_START_BIT_S		31

#define RSP6STAT_AKE_SEQ_ERROR_M	BITFIELD_MASK(1)  
#define RSP6STAT_AKE_SEQ_ERROR_S	3
#define RSP6STAT_APP_CMD_M		BITFIELD_MASK(1)  
#define RSP6STAT_APP_CMD_S		5
#define RSP6STAT_READY_FOR_DATA_M	BITFIELD_MASK(1)  
#define RSP6STAT_READY_FOR_DATA_S	8
#define RSP6STAT_CURR_STATE_M		BITFIELD_MASK(4)  
#define RSP6STAT_CURR_STATE_S		9
#define RSP6STAT_ERROR_M		BITFIELD_MASK(1)  
#define RSP6STAT_ERROR_S		13
#define RSP6STAT_ILLEGAL_CMD_M		BITFIELD_MASK(1)  
#define RSP6STAT_ILLEGAL_CMD_S		14
#define RSP6STAT_COM_CRC_ERROR_M	BITFIELD_MASK(1)  
#define RSP6STAT_COM_CRC_ERROR_S	15

#define SDIOH_XFER_TYPE_READ    SD_IO_OP_READ
#define SDIOH_XFER_TYPE_WRITE   SD_IO_OP_WRITE

#define CMD_OPTION_DEFAULT	0
#define CMD_OPTION_TUNING	1
#endif 
