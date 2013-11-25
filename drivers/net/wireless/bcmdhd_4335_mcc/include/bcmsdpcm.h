/*
 * Broadcom SDIO/PCMCIA
 * Software-specific definitions shared between device and host side
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
 * $Id: bcmsdpcm.h 364353 2012-10-23 20:31:46Z $
 */

#ifndef	_bcmsdpcm_h_
#define	_bcmsdpcm_h_


#define I_SMB_NAK	I_SMB_SW0	
#define I_SMB_INT_ACK	I_SMB_SW1	
#define I_SMB_USE_OOB	I_SMB_SW2	
#define I_SMB_DEV_INT	I_SMB_SW3	

#define I_TOSBMAIL      (I_SMB_NAK | I_SMB_INT_ACK | I_SMB_USE_OOB | I_SMB_DEV_INT)

#define SMB_NAK		(1 << 0)	
#define SMB_INT_ACK	(1 << 1)	
#define SMB_USE_OOB	(1 << 2)	
#define SMB_DEV_INT	(1 << 3)	
#define SMB_MASK	0x0000000f	

#define SMB_DATA_VERSION_MASK	0x00ff0000	
#define SMB_DATA_VERSION_SHIFT	16		


#define I_HMB_FC_STATE	I_HMB_SW0	
#define I_HMB_FC_CHANGE	I_HMB_SW1	
#define I_HMB_FRAME_IND	I_HMB_SW2	
#define I_HMB_HOST_INT	I_HMB_SW3	

#define I_TOHOSTMAIL    (I_HMB_FC_CHANGE | I_HMB_FRAME_IND | I_HMB_HOST_INT)

#define HMB_FC_ON	(1 << 0)	
#define HMB_FC_CHANGE	(1 << 1)	
#define HMB_FRAME_IND	(1 << 2)	
#define HMB_HOST_INT	(1 << 3)	
#define HMB_MASK	0x0000000f	

#define HMB_DATA_NAKHANDLED	0x01	
#define HMB_DATA_DEVREADY	0x02	
#define HMB_DATA_FC		0x04	
#define HMB_DATA_FWREADY	0x08	
#define HMB_DATA_FWHALT		0x10	

#define HMB_DATA_FCDATA_MASK	0xff000000	
#define HMB_DATA_FCDATA_SHIFT	24		

#define HMB_DATA_VERSION_MASK	0x00ff0000	
#define HMB_DATA_VERSION_SHIFT	16		


#define SDPCM_PROT_VERSION	4

#define SDPCM_SEQUENCE_MASK		0x000000ff	
#define SDPCM_PACKET_SEQUENCE(p) (((uint8 *)p)[0] & 0xff) 

#define SDPCM_CHANNEL_MASK		0x00000f00	
#define SDPCM_CHANNEL_SHIFT		8		
#define SDPCM_PACKET_CHANNEL(p) (((uint8 *)p)[1] & 0x0f) 

#define SDPCM_FLAGS_MASK		0x0000f000	
#define SDPCM_FLAGS_SHIFT		12		
#define SDPCM_PACKET_FLAGS(p) ((((uint8 *)p)[1] & 0xf0) >> 4) 

#define SDPCM_NEXTLEN_MASK		0x00ff0000	
#define SDPCM_NEXTLEN_SHIFT		16		
#define SDPCM_NEXTLEN_VALUE(p) ((((uint8 *)p)[2] & 0xff) << 4) 
#define SDPCM_NEXTLEN_OFFSET		2

#define SDPCM_DOFFSET_OFFSET		3		
#define SDPCM_DOFFSET_VALUE(p) 		(((uint8 *)p)[SDPCM_DOFFSET_OFFSET] & 0xff)
#define SDPCM_DOFFSET_MASK		0xff000000
#define SDPCM_DOFFSET_SHIFT		24

#define SDPCM_FCMASK_OFFSET		4		
#define SDPCM_FCMASK_VALUE(p)		(((uint8 *)p)[SDPCM_FCMASK_OFFSET ] & 0xff)
#define SDPCM_WINDOW_OFFSET		5		
#define SDPCM_WINDOW_VALUE(p)		(((uint8 *)p)[SDPCM_WINDOW_OFFSET] & 0xff)
#define SDPCM_VERSION_OFFSET		6		
#define SDPCM_VERSION_VALUE(p)		(((uint8 *)p)[SDPCM_VERSION_OFFSET] & 0xff)
#define SDPCM_UNUSED_OFFSET		7		
#define SDPCM_UNUSED_VALUE(p)		(((uint8 *)p)[SDPCM_UNUSED_OFFSET] & 0xff)

#define SDPCM_SWHEADER_LEN	8	

#define SDPCM_CONTROL_CHANNEL	0	
#define SDPCM_EVENT_CHANNEL	1	
#define SDPCM_DATA_CHANNEL	2	
#define SDPCM_GLOM_CHANNEL	3	
#define SDPCM_TEST_CHANNEL	15	
#define SDPCM_MAX_CHANNEL	15

#define SDPCM_SEQUENCE_WRAP	256	

#define SDPCM_FLAG_RESVD0	0x01
#define SDPCM_FLAG_RESVD1	0x02
#define SDPCM_FLAG_GSPI_TXENAB	0x04
#define SDPCM_FLAG_GLOMDESC	0x08	

#define SDPCM_GLOMDESC_FLAG	(SDPCM_FLAG_GLOMDESC << SDPCM_FLAGS_SHIFT)

#define SDPCM_GLOMDESC(p)	(((uint8 *)p)[1] & 0x80)

#define SDPCM_TEST_HDRLEN		4	
#define SDPCM_TEST_PKT_CNT_FLD_LEN	4	
#define SDPCM_TEST_DISCARD		0x01	
#define SDPCM_TEST_ECHOREQ		0x02	
#define SDPCM_TEST_ECHORSP		0x03	
#define SDPCM_TEST_BURST		0x04	
#define SDPCM_TEST_SEND			0x05	

#define SDPCM_TEST_FILL(byteno, id)	((uint8)(id + byteno))


typedef volatile struct {
	uint32 cmd52rd;		
	uint32 cmd52wr;		
	uint32 cmd53rd;		
	uint32 cmd53wr;		
	uint32 abort;		
	uint32 datacrcerror;	
	uint32 rdoutofsync;	
	uint32 wroutofsync;	
	uint32 writebusy;	
	uint32 readwait;	
	uint32 readterm;	
	uint32 writeterm;	
	uint32 rxdescuflo;	
	uint32 rxfifooflo;	
	uint32 txfifouflo;	
	uint32 runt;		
	uint32 badlen;		
	uint32 badcksum;	
	uint32 seqbreak;	
	uint32 rxfcrc;		
	uint32 rxfwoos;		
	uint32 rxfwft;		
	uint32 rxfabort;	
	uint32 woosint;		
	uint32 roosint;		
	uint32 rftermint;	
	uint32 wftermint;	
} sdpcmd_cnt_t;


#define SDIODREV_IS(var, val)	((var) == (val))
#define SDIODREV_GE(var, val)	((var) >= (val))
#define SDIODREV_GT(var, val)	((var) > (val))
#define SDIODREV_LT(var, val)	((var) < (val))
#define SDIODREV_LE(var, val)	((var) <= (val))

#define SDIODDMAREG32(h, dir, chnl) \
	((dir) == DMA_TX ? \
	 (void *)(uintptr)&((h)->regs->dma.sdiod32.dma32regs[chnl].xmt) : \
	 (void *)(uintptr)&((h)->regs->dma.sdiod32.dma32regs[chnl].rcv))

#define SDIODDMAREG64(h, dir, chnl) \
	((dir) == DMA_TX ? \
	 (void *)(uintptr)&((h)->regs->dma.sdiod64.dma64regs[chnl].xmt) : \
	 (void *)(uintptr)&((h)->regs->dma.sdiod64.dma64regs[chnl].rcv))

#define SDIODDMAREG(h, dir, chnl) \
	(SDIODREV_LT((h)->corerev, 1) ? \
	 SDIODDMAREG32((h), (dir), (chnl)) : \
	 SDIODDMAREG64((h), (dir), (chnl)))

#define PCMDDMAREG(h, dir, chnl) \
	((dir) == DMA_TX ? \
	 (void *)(uintptr)&((h)->regs->dma.pcm32.dmaregs.xmt) : \
	 (void *)(uintptr)&((h)->regs->dma.pcm32.dmaregs.rcv))

#define SDPCMDMAREG(h, dir, chnl, coreid) \
	((coreid) == SDIOD_CORE_ID ? \
	 SDIODDMAREG(h, dir, chnl) : \
	 PCMDDMAREG(h, dir, chnl))

#define SDIODFIFOREG(h, corerev) \
	(SDIODREV_LT((corerev), 1) ? \
	 ((dma32diag_t *)(uintptr)&((h)->regs->dma.sdiod32.dmafifo)) : \
	 ((dma32diag_t *)(uintptr)&((h)->regs->dma.sdiod64.dmafifo)))

#define PCMDFIFOREG(h) \
	((dma32diag_t *)(uintptr)&((h)->regs->dma.pcm32.dmafifo))

#define SDPCMFIFOREG(h, coreid, corerev) \
	((coreid) == SDIOD_CORE_ID ? \
	 SDIODFIFOREG(h, corerev) : \
	 PCMDFIFOREG(h))

#define SDPCM_SHARED_VERSION       0x0001
#define SDPCM_SHARED_VERSION_MASK  0x00FF
#define SDPCM_SHARED_ASSERT_BUILT  0x0100
#define SDPCM_SHARED_ASSERT        0x0200
#define SDPCM_SHARED_TRAP          0x0400
#define SDPCM_SHARED_IN_BRPT       0x0800
#define SDPCM_SHARED_SET_BRPT      0x1000
#define SDPCM_SHARED_PENDING_BRPT  0x2000

typedef struct {
	uint32	flags;
	uint32  trap_addr;
	uint32  assert_exp_addr;
	uint32  assert_file_addr;
	uint32  assert_line;
	uint32	console_addr;		
	uint32  msgtrace_addr;
	uint32  brpt_addr;
} sdpcm_shared_t;

extern sdpcm_shared_t sdpcm_shared;

extern void sdpcmd_fwhalt(void);

#endif	
