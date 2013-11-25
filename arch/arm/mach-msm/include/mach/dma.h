/* linux/include/asm-arm/arch-msm/dma.h
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2012, Code Aurora Forum. All rights reserved.
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
 */

#ifndef __ASM_ARCH_MSM_DMA_H
#define __ASM_ARCH_MSM_DMA_H

#include <linux/list.h>
#include <mach/msm_iomap.h>

#if defined(CONFIG_ARCH_FSM9XXX)
#include <mach/dma-fsm9xxx.h>
#endif

struct msm_dmov_errdata {
	uint32_t flush[6];
};

struct msm_dmov_cmd {
	struct list_head list;
	unsigned int cmdptr;
	void (*complete_func)(struct msm_dmov_cmd *cmd,
			      unsigned int result,
			      struct msm_dmov_errdata *err);
	void (*exec_func)(struct msm_dmov_cmd *cmd);
	struct work_struct work;
	unsigned id;    
	void *user;	
	u8 toflush;
};

struct msm_dmov_pdata {
	int sd;
	size_t sd_size;
};

void msm_dmov_enqueue_cmd(unsigned id, struct msm_dmov_cmd *cmd);
void msm_dmov_enqueue_cmd_ext(unsigned id, struct msm_dmov_cmd *cmd);
void msm_dmov_flush(unsigned int id, int graceful);
int msm_dmov_exec_cmd(unsigned id, unsigned int cmdptr);

#define DMOV_CRCIS_PER_CONF 10

#define DMOV_ADDR(off, ch) ((off) + ((ch) << 2))

#define DMOV_CMD_PTR(ch)      DMOV_ADDR(0x000, ch)
#define DMOV_CMD_LIST         (0 << 29) 
#define DMOV_CMD_PTR_LIST     (1 << 29) 
#define DMOV_CMD_INPUT_CFG    (2 << 29) 
#define DMOV_CMD_OUTPUT_CFG   (3 << 29) 
#define DMOV_CMD_ADDR(addr)   ((addr) >> 3)

#define DMOV_RSLT(ch)         DMOV_ADDR(0x040, ch)
#define DMOV_RSLT_VALID       (1 << 31) 
#define DMOV_RSLT_ERROR       (1 << 3)
#define DMOV_RSLT_FLUSH       (1 << 2)
#define DMOV_RSLT_DONE        (1 << 1)  
#define DMOV_RSLT_USER        (1 << 0)  

#define DMOV_FLUSH0(ch)       DMOV_ADDR(0x080, ch)
#define DMOV_FLUSH1(ch)       DMOV_ADDR(0x0C0, ch)
#define DMOV_FLUSH2(ch)       DMOV_ADDR(0x100, ch)
#define DMOV_FLUSH3(ch)       DMOV_ADDR(0x140, ch)
#define DMOV_FLUSH4(ch)       DMOV_ADDR(0x180, ch)
#define DMOV_FLUSH5(ch)       DMOV_ADDR(0x1C0, ch)
#define DMOV_FLUSH_TYPE       (1 << 31)

#define DMOV_STATUS(ch)       DMOV_ADDR(0x200, ch)
#define DMOV_STATUS_RSLT_COUNT(n)    (((n) >> 29))
#define DMOV_STATUS_CMD_COUNT(n)     (((n) >> 27) & 3)
#define DMOV_STATUS_RSLT_VALID       (1 << 1)
#define DMOV_STATUS_CMD_PTR_RDY      (1 << 0)

#define DMOV_CONF(ch)         DMOV_ADDR(0x240, ch)
#define DMOV_CONF_SD(sd)      (((sd & 4) << 11) | ((sd & 3) << 4))
#define DMOV_CONF_IRQ_EN             (1 << 6)
#define DMOV_CONF_FORCE_RSLT_EN      (1 << 7)
#define DMOV_CONF_SHADOW_EN          (1 << 12)
#define DMOV_CONF_MPU_DISABLE        (1 << 11)
#define DMOV_CONF_PRIORITY(n)        (n << 0)

#define DMOV_DBG_ERR(ci)      DMOV_ADDR(0x280, ci)

#define DMOV_RSLT_CONF(ch)    DMOV_ADDR(0x300, ch)
#define DMOV_RSLT_CONF_FORCE_TOP_PTR_RSLT (1 << 2)
#define DMOV_RSLT_CONF_FORCE_FLUSH_RSLT   (1 << 1)
#define DMOV_RSLT_CONF_IRQ_EN             (1 << 0)

#define DMOV_ISR              DMOV_ADDR(0x380, 0)

#define DMOV_CI_CONF(ci)      DMOV_ADDR(0x390, ci)
#define DMOV_CI_CONF_RANGE_END(n)      ((n) << 24)
#define DMOV_CI_CONF_RANGE_START(n)    ((n) << 16)
#define DMOV_CI_CONF_MAX_BURST(n)      ((n) << 0)

#define DMOV_CI_DBG_ERR(ci)   DMOV_ADDR(0x3B0, ci)

#define DMOV_CRCI_CONF0       DMOV_ADDR(0x3D0, 0)
#define DMOV_CRCI_CONF1       DMOV_ADDR(0x3D4, 0)
#define DMOV_CRCI_CONF0_SD(crci, sd) (sd << (crci*3))
#define DMOV_CRCI_CONF1_SD(crci, sd) (sd << ((crci-DMOV_CRCIS_PER_CONF)*3))

#define DMOV_CRCI_CTL(crci)   DMOV_ADDR(0x400, crci)
#define DMOV_CRCI_CTL_BLK_SZ(n)        ((n) << 0)
#define DMOV_CRCI_CTL_RST              (1 << 17)
#define DMOV_CRCI_MUX                  (1 << 18)



#if defined(CONFIG_ARCH_MSM8X60)
#define DMOV_GP_CHAN           15

#define DMOV_NAND_CHAN         17
#define DMOV_NAND_CHAN_MODEM   26
#define DMOV_NAND_CHAN_Q6      27
#define DMOV_NAND_CRCI_CMD     15
#define DMOV_NAND_CRCI_DATA    3

#define DMOV_CE_IN_CHAN        2
#define DMOV_CE_IN_CRCI        4

#define DMOV_CE_OUT_CHAN       3
#define DMOV_CE_OUT_CRCI       5

#define DMOV_CE_HASH_CRCI      15

#define DMOV_SDC1_CHAN         18
#define DMOV_SDC1_CRCI         1

#define DMOV_SDC2_CHAN         19
#define DMOV_SDC2_CRCI         4

#define DMOV_SDC3_CHAN         20
#define DMOV_SDC3_CRCI         2

#define DMOV_SDC4_CHAN         21
#define DMOV_SDC4_CRCI         5

#define DMOV_SDC5_CHAN         21
#define DMOV_SDC5_CRCI         14

#define DMOV_TSIF_CHAN         4
#define DMOV_TSIF_CRCI         6

#define DMOV_HSUART1_TX_CHAN   22
#define DMOV_HSUART1_TX_CRCI   8

#define DMOV_HSUART1_RX_CHAN   23
#define DMOV_HSUART1_RX_CRCI   9

#define DMOV_HSUART2_TX_CHAN   8
#define DMOV_HSUART2_TX_CRCI   13

#define DMOV_HSUART2_RX_CHAN   8
#define DMOV_HSUART2_RX_CRCI   14

#elif defined(CONFIG_ARCH_MSM8960)
#define DMOV_GP_CHAN           9

#define DMOV_CE_IN_CHAN        0
#define DMOV_CE_IN_CRCI        2

#define DMOV_CE_OUT_CHAN       1
#define DMOV_CE_OUT_CRCI       3

#define DMOV_TSIF_CHAN         2
#define DMOV_TSIF_CRCI         1

#define DMOV_HSUART_GSBI4_TX_CHAN	11
#define DMOV_HSUART_GSBI4_TX_CRCI	8

#define DMOV_HSUART_GSBI4_RX_CHAN	10
#define DMOV_HSUART_GSBI4_RX_CRCI	7

#define DMOV_HSUART_GSBI5_TX_CHAN	7
#define DMOV_HSUART_GSBI5_TX_CRCI	10

#define DMOV_HSUART_GSBI5_RX_CHAN	6
#define DMOV_HSUART_GSBI5_RX_CRCI	9

#define DMOV_HSUART_GSBI6_TX_CHAN	7
#define DMOV_HSUART_GSBI6_TX_CRCI	6

#define DMOV_HSUART_GSBI6_RX_CHAN	8
#define DMOV_HSUART_GSBI6_RX_CRCI	11

#define DMOV_HSUART_GSBI9_TX_CHAN	4
#define DMOV_HSUART_GSBI9_TX_CRCI	13

#define DMOV_HSUART_GSBI9_RX_CHAN	3
#define DMOV_HSUART_GSBI9_RX_CRCI	12

#elif defined(CONFIG_ARCH_MSM9615)

#define DMOV_GP_CHAN          4

#define DMOV_CE_IN_CHAN       0
#define DMOV_CE_IN_CRCI       12

#define DMOV_CE_OUT_CHAN      1
#define DMOV_CE_OUT_CRCI      13

#define DMOV_NAND_CHAN        3
#define DMOV_NAND_CRCI_CMD    15
#define DMOV_NAND_CRCI_DATA   3

#elif defined(CONFIG_ARCH_FSM9XXX)

#else
#define DMOV_GP_CHAN          4

#define DMOV_CE_IN_CHAN       5
#define DMOV_CE_IN_CRCI       1

#define DMOV_CE_OUT_CHAN      6
#define DMOV_CE_OUT_CRCI      2

#define DMOV_CE_HASH_CRCI     3

#define DMOV_NAND_CHAN        7
#define DMOV_NAND_CRCI_CMD    5
#define DMOV_NAND_CRCI_DATA   4

#define DMOV_SDC1_CHAN        8
#define DMOV_SDC1_CRCI        6

#define DMOV_SDC2_CHAN        8
#define DMOV_SDC2_CRCI        7

#define DMOV_SDC3_CHAN        8
#define DMOV_SDC3_CRCI        12

#define DMOV_SDC4_CHAN        8
#define DMOV_SDC4_CRCI        13

#define DMOV_TSIF_CHAN        10
#define DMOV_TSIF_CRCI        10

#define DMOV_USB_CHAN         11

#define DMOV_HSUART1_TX_CHAN   4
#define DMOV_HSUART1_TX_CRCI   8

#define DMOV_HSUART1_RX_CHAN   9
#define DMOV_HSUART1_RX_CRCI   9

#define DMOV_HSUART2_TX_CHAN   4
#define DMOV_HSUART2_TX_CRCI   14

#define DMOV_HSUART2_RX_CHAN   11
#define DMOV_HSUART2_RX_CRCI   15
#endif

#define DMOV8064_CE_IN_CHAN        0
#define DMOV8064_CE_IN_CRCI       14

#define DMOV8064_CE_OUT_CHAN       1
#define DMOV8064_CE_OUT_CRCI       15

#define DMOV_APQ8064_HSUART_GSBI1_TX_CHAN	3
#define DMOV_APQ8064_HSUART_GSBI1_TX_CRCI	13

#define DMOV_APQ8064_HSUART_GSBI1_RX_CHAN	2
#define DMOV_APQ8064_HSUART_GSBI1_RX_CRCI	12

#define DMOV_NONE_CRCI        0


#define CMD_PTR_ADDR(addr)  ((addr) >> 3)
#define CMD_PTR_LP          (1 << 31) 
#define CMD_PTR_PT          (3 << 29) 

typedef struct {
	unsigned cmd;
	unsigned src;
	unsigned dst;
	unsigned len;
} dmov_s;

typedef struct {
	unsigned cmd;
	unsigned src_dscr;
	unsigned dst_dscr;
	unsigned _reserved;
} dmov_sg;

typedef struct {
	uint32_t cmd;
	uint32_t src_row_addr;
	uint32_t dst_row_addr;
	uint32_t src_dst_len;
	uint32_t num_rows;
	uint32_t row_offset;
} dmov_box;


#define CMD_LC      (1 << 31)  
#define CMD_FR      (1 << 22)  
#define CMD_OCU     (1 << 21)  
#define CMD_OCB     (1 << 20)  
#define CMD_TCB     (1 << 19)  
#define CMD_DAH     (1 << 18)  
#define CMD_SAH     (1 << 17)  

#define CMD_MODE_SINGLE     (0 << 0) 
#define CMD_MODE_SG         (1 << 0) 
#define CMD_MODE_IND_SG     (2 << 0) 
#define CMD_MODE_BOX        (3 << 0) 

#define CMD_DST_SWAP_BYTES  (1 << 14) 
#define CMD_DST_SWAP_SHORTS (1 << 15) 
#define CMD_DST_SWAP_WORDS  (1 << 16) 

#define CMD_SRC_SWAP_BYTES  (1 << 11) 
#define CMD_SRC_SWAP_SHORTS (1 << 12) 
#define CMD_SRC_SWAP_WORDS  (1 << 13) 

#define CMD_DST_CRCI(n)     (((n) & 15) << 7)
#define CMD_SRC_CRCI(n)     (((n) & 15) << 3)

#endif
