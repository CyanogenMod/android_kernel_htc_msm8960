/*
 * Broadcom HND chip & on-chip-interconnect-related definitions.
 *
 * Copyright (C) 1999-2013, Broadcom Corporation
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
 * $Id: hndsoc.h 365041 2012-10-26 09:10:35Z $
 */

#ifndef	_HNDSOC_H
#define	_HNDSOC_H

#include <sbconfig.h>
#include <aidmp.h>

#define SI_SDRAM_BASE		0x00000000	
#define SI_PCI_MEM		0x08000000	
#define SI_PCI_MEM_SZ		(64 * 1024 * 1024)
#define SI_PCI_CFG		0x0c000000	
#define	SI_SDRAM_SWAPPED	0x10000000	
#define SI_SDRAM_R2		0x80000000	

#define SI_ENUM_BASE    	0x18000000	

#define SI_WRAP_BASE    	0x18100000	
#define SI_CORE_SIZE    	0x1000		

#define	SI_MAXCORES		32		

#define	SI_FASTRAM		0x19000000	
#define	SI_FASTRAM_SWAPPED	0x19800000

#define	SI_FLASH2		0x1c000000	
#define	SI_FLASH2_SZ		0x02000000	
#define	SI_ARMCM3_ROM		0x1e000000	
#define	SI_FLASH1		0x1fc00000	
#define	SI_FLASH1_SZ		0x00400000	
#define	SI_FLASH_WINDOW		0x01000000	

#define SI_NS_NANDFLASH		0x1c000000	
#define SI_NS_NORFLASH		0x1e000000	
#define SI_NS_ROM		0xfffd0000	
#define	SI_NS_FLASH_WINDOW	0x02000000	

#define	SI_ARM7S_ROM		0x20000000	
#define	SI_ARMCR4_ROM		0x000f0000	
#define	SI_ARMCM3_SRAM2		0x60000000	
#define	SI_ARM7S_SRAM2		0x80000000	
#define	SI_ARM_FLASH1		0xffff0000	
#define	SI_ARM_FLASH1_SZ	0x00010000	

#define SI_PCI_DMA		0x40000000	
#define SI_PCI_DMA2		0x80000000	
#define SI_PCI_DMA_SZ		0x40000000	
#define SI_PCIE_DMA_L32		0x00000000	
#define SI_PCIE_DMA_H32		0x80000000	

#define	NODEV_CORE_ID		0x700		
#define	CC_CORE_ID		0x800		
#define	ILINE20_CORE_ID		0x801		
#define	SRAM_CORE_ID		0x802		
#define	SDRAM_CORE_ID		0x803		
#define	PCI_CORE_ID		0x804		
#define	MIPS_CORE_ID		0x805		
#define	ENET_CORE_ID		0x806		
#define	CODEC_CORE_ID		0x807		
#define	USB_CORE_ID		0x808		
#define	ADSL_CORE_ID		0x809		
#define	ILINE100_CORE_ID	0x80a		
#define	IPSEC_CORE_ID		0x80b		
#define	UTOPIA_CORE_ID		0x80c		
#define	PCMCIA_CORE_ID		0x80d		
#define	SOCRAM_CORE_ID		0x80e		
#define	MEMC_CORE_ID		0x80f		
#define	OFDM_CORE_ID		0x810		
#define	EXTIF_CORE_ID		0x811		
#define	D11_CORE_ID		0x812		
#define	APHY_CORE_ID		0x813		
#define	BPHY_CORE_ID		0x814		
#define	GPHY_CORE_ID		0x815		
#define	MIPS33_CORE_ID		0x816		
#define	USB11H_CORE_ID		0x817		
#define	USB11D_CORE_ID		0x818		
#define	USB20H_CORE_ID		0x819		
#define	USB20D_CORE_ID		0x81a		
#define	SDIOH_CORE_ID		0x81b		
#define	ROBO_CORE_ID		0x81c		
#define	ATA100_CORE_ID		0x81d		
#define	SATAXOR_CORE_ID		0x81e		
#define	GIGETH_CORE_ID		0x81f		
#define	PCIE_CORE_ID		0x820		
#define	NPHY_CORE_ID		0x821		
#define	SRAMC_CORE_ID		0x822		
#define	MINIMAC_CORE_ID		0x823		
#define	ARM11_CORE_ID		0x824		
#define	ARM7S_CORE_ID		0x825		
#define	LPPHY_CORE_ID		0x826		
#define	PMU_CORE_ID		0x827		
#define	SSNPHY_CORE_ID		0x828		
#define	SDIOD_CORE_ID		0x829		
#define	ARMCM3_CORE_ID		0x82a		
#define	HTPHY_CORE_ID		0x82b		
#define	MIPS74K_CORE_ID		0x82c		
#define	GMAC_CORE_ID		0x82d		
#define	DMEMC_CORE_ID		0x82e		
#define	PCIERC_CORE_ID		0x82f		
#define	OCP_CORE_ID		0x830		
#define	SC_CORE_ID		0x831		
#define	AHB_CORE_ID		0x832		
#define	SPIH_CORE_ID		0x833		
#define	I2S_CORE_ID		0x834		
#define	DMEMS_CORE_ID		0x835		
#define	DEF_SHIM_COMP		0x837		

#define ACPHY_CORE_ID		0x83b		
#define PCIE2_CORE_ID		0x83c		
#define USB30D_CORE_ID		0x83d		
#define ARMCR4_CORE_ID		0x83e		
#define APB_BRIDGE_CORE_ID	0x135		
#define AXI_CORE_ID		0x301		
#define EROM_CORE_ID		0x366		
#define OOB_ROUTER_CORE_ID	0x367		
#define DEF_AI_COMP		0xfff		

#define CC_4706_CORE_ID		0x500		
#define NS_PCIEG2_CORE_ID	0x501		
#define NS_DMA_CORE_ID		0x502		
#define NS_SDIO3_CORE_ID	0x503		
#define NS_USB20_CORE_ID	0x504		
#define NS_USB30_CORE_ID	0x505		
#define NS_A9JTAG_CORE_ID	0x506		
#define NS_DDR23_CORE_ID	0x507		
#define NS_ROM_CORE_ID		0x508		
#define NS_NAND_CORE_ID		0x509		
#define NS_QSPI_CORE_ID		0x50a		
#define NS_CCB_CORE_ID		0x50b		
#define SOCRAM_4706_CORE_ID	0x50e		
#define NS_SOCRAM_CORE_ID	SOCRAM_4706_CORE_ID
#define	ARMCA9_CORE_ID		0x510		
#define	NS_IHOST_CORE_ID	ARMCA9_CORE_ID	
#define GMAC_COMMON_4706_CORE_ID	0x5dc		
#define GMAC_4706_CORE_ID	0x52d		
#define AMEMC_CORE_ID		0x52e		
#define ALTA_CORE_ID		0x534		
#define DDR23_PHY_CORE_ID	0x5dd

#define SI_PCI1_MEM     0x40000000  
#define SI_PCI1_CFG     0x44000000  
#define SI_PCIE1_DMA_H32		0xc0000000	
#define CC_4706B0_CORE_REV	0x8000001f		
#define SOCRAM_4706B0_CORE_REV	0x80000005		
#define GMAC_4706B0_CORE_REV	0x80000000		

#define	SI_CC_IDX		0

#define	SOCI_SB			0
#define	SOCI_AI			1
#define	SOCI_UBUS		2
#define	SOCI_NAI		3

#define	SICF_BIST_EN		0x8000
#define	SICF_PME_EN		0x4000
#define	SICF_CORE_BITS		0x3ffc
#define	SICF_FGC		0x0002
#define	SICF_CLOCK_EN		0x0001

#define	SISF_BIST_DONE		0x8000
#define	SISF_BIST_ERROR		0x4000
#define	SISF_GATED_CLK		0x2000
#define	SISF_DMA64		0x1000
#define	SISF_CORE_BITS		0x0fff

#define SISF_NS_BOOTDEV_MASK	0x0003	
#define SISF_NS_BOOTDEV_NOR	0x0000	
#define SISF_NS_BOOTDEV_NAND	0x0001	
#define SISF_NS_BOOTDEV_ROM	0x0002	
#define SISF_NS_BOOTDEV_OFFLOAD	0x0003	
#define SISF_NS_SKUVEC_MASK	0x000c	

#define SI_CLK_CTL_ST		0x1e0		

#define	CCS_FORCEALP		0x00000001	
#define	CCS_FORCEHT		0x00000002	
#define	CCS_FORCEILP		0x00000004	
#define	CCS_ALPAREQ		0x00000008	
#define	CCS_HTAREQ		0x00000010	
#define	CCS_FORCEHWREQOFF	0x00000020	
#define CCS_HQCLKREQ		0x00000040	
#define CCS_USBCLKREQ		0x00000100	
#define CCS_ERSRC_REQ_MASK	0x00000700	
#define CCS_ERSRC_REQ_SHIFT	8
#define	CCS_ALPAVAIL		0x00010000	
#define	CCS_HTAVAIL		0x00020000	
#define CCS_BP_ON_APL		0x00040000	
#define CCS_BP_ON_HT		0x00080000	
#define CCS_ERSRC_STS_MASK	0x07000000	
#define CCS_ERSRC_STS_SHIFT	24

#define	CCS0_HTAVAIL		0x00010000	
#define	CCS0_ALPAVAIL		0x00020000	


#define FLASH_MIN		0x00020000	

#define	BISZ_OFFSET		0x3e0		
#define	BISZ_MAGIC		0x4249535a	
#define	BISZ_MAGIC_IDX		0		
#define	BISZ_TXTST_IDX		1		
#define	BISZ_TXTEND_IDX		2		
#define	BISZ_DATAST_IDX		3		
#define	BISZ_DATAEND_IDX	4		
#define	BISZ_BSSST_IDX		5		
#define	BISZ_BSSEND_IDX		6		
#define BISZ_SIZE		7		

#define	SOC_BOOTDEV_ROM		0x00000001
#define	SOC_BOOTDEV_PFLASH	0x00000002
#define	SOC_BOOTDEV_SFLASH	0x00000004
#define	SOC_BOOTDEV_NANDFLASH	0x00000008

#define	SOC_KNLDEV_NORFLASH	0x00000002
#define	SOC_KNLDEV_NANDFLASH	0x00000004

#ifndef _LANGUAGE_ASSEMBLY
int soc_boot_dev(void *sih);
int soc_knl_dev(void *sih);
#endif	

#endif 
