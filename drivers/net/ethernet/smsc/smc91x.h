/*------------------------------------------------------------------------
 . smc91x.h - macros for SMSC's 91C9x/91C1xx single-chip Ethernet device.
 .
 . Copyright (C) 1996 by Erik Stahlman
 . Copyright (C) 2001 Standard Microsystems Corporation
 .	Developed by Simple Network Magic Corporation
 . Copyright (C) 2003 Monta Vista Software, Inc.
 .	Unified SMC91x driver by Nicolas Pitre
 .
 . This program is free software; you can redistribute it and/or modify
 . it under the terms of the GNU General Public License as published by
 . the Free Software Foundation; either version 2 of the License, or
 . (at your option) any later version.
 .
 . This program is distributed in the hope that it will be useful,
 . but WITHOUT ANY WARRANTY; without even the implied warranty of
 . MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 . GNU General Public License for more details.
 .
 . You should have received a copy of the GNU General Public License
 . along with this program; if not, write to the Free Software
 . Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 .
 . Information contained in this file was obtained from the LAN91C111
 . manual from SMC.  To get a copy, if you really want one, you can find
 . information under www.smsc.com.
 .
 . Authors
 .	Erik Stahlman		<erik@vt.edu>
 .	Daris A Nevil		<dnevil@snmc.com>
 .	Nicolas Pitre 		<nico@fluxnic.net>
 .
 ---------------------------------------------------------------------------*/
#ifndef _SMC91X_H_
#define _SMC91X_H_

#include <linux/smc91x.h>


#if defined(CONFIG_ARCH_LUBBOCK) ||\
    defined(CONFIG_MACH_MAINSTONE) ||\
    defined(CONFIG_MACH_ZYLONITE) ||\
    defined(CONFIG_MACH_LITTLETON) ||\
    defined(CONFIG_MACH_ZYLONITE2) ||\
    defined(CONFIG_ARCH_VIPER) ||\
    defined(CONFIG_MACH_STARGATE2)

#include <asm/mach-types.h>

#define SMC_CAN_USE_8BIT	1
#define SMC_CAN_USE_16BIT	1
#define SMC_CAN_USE_32BIT	1
#define SMC_NOWAIT		1

#define SMC_IO_SHIFT		(lp->io_shift)

#define SMC_inb(a, r)		readb((a) + (r))
#define SMC_inw(a, r)		readw((a) + (r))
#define SMC_inl(a, r)		readl((a) + (r))
#define SMC_outb(v, a, r)	writeb(v, (a) + (r))
#define SMC_outl(v, a, r)	writel(v, (a) + (r))
#define SMC_insw(a, r, p, l)	readsw((a) + (r), p, l)
#define SMC_outsw(a, r, p, l)	writesw((a) + (r), p, l)
#define SMC_insl(a, r, p, l)	readsl((a) + (r), p, l)
#define SMC_outsl(a, r, p, l)	writesl((a) + (r), p, l)
#define SMC_IRQ_FLAGS		(-1)	

static inline void SMC_outw(u16 val, void __iomem *ioaddr, int reg)
{
	if ((machine_is_mainstone() || machine_is_stargate2()) && reg & 2) {
		unsigned int v = val << 16;
		v |= readl(ioaddr + (reg & ~2)) & 0xffff;
		writel(v, ioaddr + (reg & ~2));
	} else {
		writew(val, ioaddr + reg);
	}
}

#elif defined(CONFIG_SA1100_PLEB)
#define SMC_CAN_USE_8BIT	1
#define SMC_CAN_USE_16BIT	1
#define SMC_CAN_USE_32BIT	0
#define SMC_IO_SHIFT		0
#define SMC_NOWAIT		1

#define SMC_inb(a, r)		readb((a) + (r))
#define SMC_insb(a, r, p, l)	readsb((a) + (r), p, (l))
#define SMC_inw(a, r)		readw((a) + (r))
#define SMC_insw(a, r, p, l)	readsw((a) + (r), p, l)
#define SMC_outb(v, a, r)	writeb(v, (a) + (r))
#define SMC_outsb(a, r, p, l)	writesb((a) + (r), p, (l))
#define SMC_outw(v, a, r)	writew(v, (a) + (r))
#define SMC_outsw(a, r, p, l)	writesw((a) + (r), p, l)

#define SMC_IRQ_FLAGS		(-1)

#elif defined(CONFIG_SA1100_ASSABET)

#include <mach/neponset.h>

#define SMC_CAN_USE_8BIT	1
#define SMC_CAN_USE_16BIT	0
#define SMC_CAN_USE_32BIT	0
#define SMC_NOWAIT		1

#define SMC_IO_SHIFT		2

#define SMC_inb(a, r)		readb((a) + (r))
#define SMC_outb(v, a, r)	writeb(v, (a) + (r))
#define SMC_insb(a, r, p, l)	readsb((a) + (r), p, (l))
#define SMC_outsb(a, r, p, l)	writesb((a) + (r), p, (l))
#define SMC_IRQ_FLAGS		(-1)	

#elif	defined(CONFIG_MACH_LOGICPD_PXA270) ||	\
	defined(CONFIG_MACH_NOMADIK_8815NHK)

#define SMC_CAN_USE_8BIT	0
#define SMC_CAN_USE_16BIT	1
#define SMC_CAN_USE_32BIT	0
#define SMC_IO_SHIFT		0
#define SMC_NOWAIT		1

#define SMC_inw(a, r)		readw((a) + (r))
#define SMC_outw(v, a, r)	writew(v, (a) + (r))
#define SMC_insw(a, r, p, l)	readsw((a) + (r), p, l)
#define SMC_outsw(a, r, p, l)	writesw((a) + (r), p, l)

#elif	defined(CONFIG_ARCH_INNOKOM) || \
	defined(CONFIG_ARCH_PXA_IDP) || \
	defined(CONFIG_ARCH_RAMSES) || \
	defined(CONFIG_ARCH_PCM027)

#define SMC_CAN_USE_8BIT	1
#define SMC_CAN_USE_16BIT	1
#define SMC_CAN_USE_32BIT	1
#define SMC_IO_SHIFT		0
#define SMC_NOWAIT		1
#define SMC_USE_PXA_DMA		1

#define SMC_inb(a, r)		readb((a) + (r))
#define SMC_inw(a, r)		readw((a) + (r))
#define SMC_inl(a, r)		readl((a) + (r))
#define SMC_outb(v, a, r)	writeb(v, (a) + (r))
#define SMC_outl(v, a, r)	writel(v, (a) + (r))
#define SMC_insl(a, r, p, l)	readsl((a) + (r), p, l)
#define SMC_outsl(a, r, p, l)	writesl((a) + (r), p, l)
#define SMC_IRQ_FLAGS		(-1)	

static inline void
SMC_outw(u16 val, void __iomem *ioaddr, int reg)
{
	if (reg & 2) {
		unsigned int v = val << 16;
		v |= readl(ioaddr + (reg & ~2)) & 0xffff;
		writel(v, ioaddr + (reg & ~2));
	} else {
		writew(val, ioaddr + reg);
	}
}

#elif	defined(CONFIG_SH_SH4202_MICRODEV)

#define SMC_CAN_USE_8BIT	0
#define SMC_CAN_USE_16BIT	1
#define SMC_CAN_USE_32BIT	0

#define SMC_inb(a, r)		inb((a) + (r) - 0xa0000000)
#define SMC_inw(a, r)		inw((a) + (r) - 0xa0000000)
#define SMC_inl(a, r)		inl((a) + (r) - 0xa0000000)
#define SMC_outb(v, a, r)	outb(v, (a) + (r) - 0xa0000000)
#define SMC_outw(v, a, r)	outw(v, (a) + (r) - 0xa0000000)
#define SMC_outl(v, a, r)	outl(v, (a) + (r) - 0xa0000000)
#define SMC_insl(a, r, p, l)	insl((a) + (r) - 0xa0000000, p, l)
#define SMC_outsl(a, r, p, l)	outsl((a) + (r) - 0xa0000000, p, l)
#define SMC_insw(a, r, p, l)	insw((a) + (r) - 0xa0000000, p, l)
#define SMC_outsw(a, r, p, l)	outsw((a) + (r) - 0xa0000000, p, l)

#define SMC_IRQ_FLAGS		(0)

#elif   defined(CONFIG_M32R)

#define SMC_CAN_USE_8BIT	0
#define SMC_CAN_USE_16BIT	1
#define SMC_CAN_USE_32BIT	0

#define SMC_inb(a, r)		inb(((u32)a) + (r))
#define SMC_inw(a, r)		inw(((u32)a) + (r))
#define SMC_outb(v, a, r)	outb(v, ((u32)a) + (r))
#define SMC_outw(v, a, r)	outw(v, ((u32)a) + (r))
#define SMC_insw(a, r, p, l)	insw(((u32)a) + (r), p, l)
#define SMC_outsw(a, r, p, l)	outsw(((u32)a) + (r), p, l)

#define SMC_IRQ_FLAGS		(0)

#define RPC_LSA_DEFAULT		RPC_LED_TX_RX
#define RPC_LSB_DEFAULT		RPC_LED_100_10

#elif	defined(CONFIG_ARCH_VERSATILE)

#define SMC_CAN_USE_8BIT	1
#define SMC_CAN_USE_16BIT	1
#define SMC_CAN_USE_32BIT	1
#define SMC_NOWAIT		1

#define SMC_inb(a, r)		readb((a) + (r))
#define SMC_inw(a, r)		readw((a) + (r))
#define SMC_inl(a, r)		readl((a) + (r))
#define SMC_outb(v, a, r)	writeb(v, (a) + (r))
#define SMC_outw(v, a, r)	writew(v, (a) + (r))
#define SMC_outl(v, a, r)	writel(v, (a) + (r))
#define SMC_insl(a, r, p, l)	readsl((a) + (r), p, l)
#define SMC_outsl(a, r, p, l)	writesl((a) + (r), p, l)
#define SMC_IRQ_FLAGS		(-1)	

#elif defined(CONFIG_ARCH_MSM)

#define SMC_CAN_USE_8BIT	0
#define SMC_CAN_USE_16BIT	1
#define SMC_CAN_USE_32BIT	0
#define SMC_NOWAIT		1

#define SMC_inw(a, r)		readw((a) + (r))
#define SMC_outw(v, a, r)	writew(v, (a) + (r))
#define SMC_insw(a, r, p, l)	readsw((a) + (r), p, l)
#define SMC_outsw(a, r, p, l)	writesw((a) + (r), p, l)

#define SMC_IRQ_FLAGS		IRQF_TRIGGER_HIGH

#elif defined(CONFIG_MN10300)


#include <unit/smc91111.h>

#elif defined(CONFIG_ARCH_MSM)

#define SMC_CAN_USE_8BIT	0
#define SMC_CAN_USE_16BIT	1
#define SMC_CAN_USE_32BIT	0
#define SMC_NOWAIT		1

#define SMC_inw(a, r)		readw((a) + (r))
#define SMC_outw(v, a, r)	writew(v, (a) + (r))
#define SMC_insw(a, r, p, l)	readsw((a) + (r), p, l)
#define SMC_outsw(a, r, p, l)	writesw((a) + (r), p, l)

#define SMC_IRQ_FLAGS		IRQF_TRIGGER_HIGH

#elif defined(CONFIG_COLDFIRE)

#define SMC_CAN_USE_8BIT	0
#define SMC_CAN_USE_16BIT	1
#define SMC_CAN_USE_32BIT	0
#define SMC_NOWAIT		1

static inline void mcf_insw(void *a, unsigned char *p, int l)
{
	u16 *wp = (u16 *) p;
	while (l-- > 0)
		*wp++ = readw(a);
}

static inline void mcf_outsw(void *a, unsigned char *p, int l)
{
	u16 *wp = (u16 *) p;
	while (l-- > 0)
		writew(*wp++, a);
}

#define SMC_inw(a, r)		_swapw(readw((a) + (r)))
#define SMC_outw(v, a, r)	writew(_swapw(v), (a) + (r))
#define SMC_insw(a, r, p, l)	mcf_insw(a + r, p, l)
#define SMC_outsw(a, r, p, l)	mcf_outsw(a + r, p, l)

#define SMC_IRQ_FLAGS		(IRQF_DISABLED)

#else


#define SMC_CAN_USE_8BIT	1
#define SMC_CAN_USE_16BIT	1
#define SMC_CAN_USE_32BIT	1
#define SMC_NOWAIT		1

#define SMC_IO_SHIFT		(lp->io_shift)

#define SMC_inb(a, r)		readb((a) + (r))
#define SMC_inw(a, r)		readw((a) + (r))
#define SMC_inl(a, r)		readl((a) + (r))
#define SMC_outb(v, a, r)	writeb(v, (a) + (r))
#define SMC_outw(v, a, r)	writew(v, (a) + (r))
#define SMC_outl(v, a, r)	writel(v, (a) + (r))
#define SMC_insw(a, r, p, l)	readsw((a) + (r), p, l)
#define SMC_outsw(a, r, p, l)	writesw((a) + (r), p, l)
#define SMC_insl(a, r, p, l)	readsl((a) + (r), p, l)
#define SMC_outsl(a, r, p, l)	writesl((a) + (r), p, l)

#define RPC_LSA_DEFAULT		RPC_LED_100_10
#define RPC_LSB_DEFAULT		RPC_LED_TX_RX

#endif


struct smc_local {
	struct sk_buff *pending_tx_skb;
	struct tasklet_struct tx_task;

	
	int	version;

	
	int	tcr_cur_mode;

	
	int	rcr_cur_mode;

	
	int	rpc_cur_mode;
	int	ctl_rfduplx;
	int	ctl_rspeed;

	u32	msg_enable;
	u32	phy_type;
	struct mii_if_info mii;

	
	struct work_struct phy_configure;
	struct net_device *dev;
	int	work_pending;

	spinlock_t lock;

#ifdef CONFIG_ARCH_PXA
	
	u_long physaddr;
	struct device *device;
#endif
	void __iomem *base;
	void __iomem *datacs;

	
	int	io_shift;

	struct smc91x_platdata cfg;
};

#define SMC_8BIT(p)	((p)->cfg.flags & SMC91X_USE_8BIT)
#define SMC_16BIT(p)	((p)->cfg.flags & SMC91X_USE_16BIT)
#define SMC_32BIT(p)	((p)->cfg.flags & SMC91X_USE_32BIT)

#ifdef CONFIG_ARCH_PXA
#include <linux/dma-mapping.h>
#include <mach/dma.h>

#ifdef SMC_insl
#undef SMC_insl
#define SMC_insl(a, r, p, l) \
	smc_pxa_dma_insl(a, lp, r, dev->dma, p, l)
static inline void
smc_pxa_dma_insl(void __iomem *ioaddr, struct smc_local *lp, int reg, int dma,
		 u_char *buf, int len)
{
	u_long physaddr = lp->physaddr;
	dma_addr_t dmabuf;

	
	if (dma == (unsigned char)-1) {
		readsl(ioaddr + reg, buf, len);
		return;
	}

	
	if ((long)buf & 4) {
		*((u32 *)buf) = SMC_inl(ioaddr, reg);
		buf += 4;
		len--;
	}

	len *= 4;
	dmabuf = dma_map_single(lp->device, buf, len, DMA_FROM_DEVICE);
	DCSR(dma) = DCSR_NODESC;
	DTADR(dma) = dmabuf;
	DSADR(dma) = physaddr + reg;
	DCMD(dma) = (DCMD_INCTRGADDR | DCMD_BURST32 |
		     DCMD_WIDTH4 | (DCMD_LENGTH & len));
	DCSR(dma) = DCSR_NODESC | DCSR_RUN;
	while (!(DCSR(dma) & DCSR_STOPSTATE))
		cpu_relax();
	DCSR(dma) = 0;
	dma_unmap_single(lp->device, dmabuf, len, DMA_FROM_DEVICE);
}
#endif

#ifdef SMC_insw
#undef SMC_insw
#define SMC_insw(a, r, p, l) \
	smc_pxa_dma_insw(a, lp, r, dev->dma, p, l)
static inline void
smc_pxa_dma_insw(void __iomem *ioaddr, struct smc_local *lp, int reg, int dma,
		 u_char *buf, int len)
{
	u_long physaddr = lp->physaddr;
	dma_addr_t dmabuf;

	
	if (dma == (unsigned char)-1) {
		readsw(ioaddr + reg, buf, len);
		return;
	}

	
	while ((long)buf & 6) {
		*((u16 *)buf) = SMC_inw(ioaddr, reg);
		buf += 2;
		len--;
	}

	len *= 2;
	dmabuf = dma_map_single(lp->device, buf, len, DMA_FROM_DEVICE);
	DCSR(dma) = DCSR_NODESC;
	DTADR(dma) = dmabuf;
	DSADR(dma) = physaddr + reg;
	DCMD(dma) = (DCMD_INCTRGADDR | DCMD_BURST32 |
		     DCMD_WIDTH2 | (DCMD_LENGTH & len));
	DCSR(dma) = DCSR_NODESC | DCSR_RUN;
	while (!(DCSR(dma) & DCSR_STOPSTATE))
		cpu_relax();
	DCSR(dma) = 0;
	dma_unmap_single(lp->device, dmabuf, len, DMA_FROM_DEVICE);
}
#endif

static void
smc_pxa_dma_irq(int dma, void *dummy)
{
	DCSR(dma) = 0;
}
#endif  



#if ! SMC_CAN_USE_32BIT
#define SMC_inl(ioaddr, reg)		({ BUG(); 0; })
#define SMC_outl(x, ioaddr, reg)	BUG()
#define SMC_insl(a, r, p, l)		BUG()
#define SMC_outsl(a, r, p, l)		BUG()
#endif

#if !defined(SMC_insl) || !defined(SMC_outsl)
#define SMC_insl(a, r, p, l)		BUG()
#define SMC_outsl(a, r, p, l)		BUG()
#endif

#if ! SMC_CAN_USE_16BIT

#define SMC_outw(x, ioaddr, reg)					\
	do {								\
		unsigned int __val16 = (x);				\
		SMC_outb( __val16, ioaddr, reg );			\
		SMC_outb( __val16 >> 8, ioaddr, reg + (1 << SMC_IO_SHIFT));\
	} while (0)
#define SMC_inw(ioaddr, reg)						\
	({								\
		unsigned int __val16;					\
		__val16 =  SMC_inb( ioaddr, reg );			\
		__val16 |= SMC_inb( ioaddr, reg + (1 << SMC_IO_SHIFT)) << 8; \
		__val16;						\
	})

#define SMC_insw(a, r, p, l)		BUG()
#define SMC_outsw(a, r, p, l)		BUG()

#endif

#if !defined(SMC_insw) || !defined(SMC_outsw)
#define SMC_insw(a, r, p, l)		BUG()
#define SMC_outsw(a, r, p, l)		BUG()
#endif

#if ! SMC_CAN_USE_8BIT
#define SMC_inb(ioaddr, reg)		({ BUG(); 0; })
#define SMC_outb(x, ioaddr, reg)	BUG()
#define SMC_insb(a, r, p, l)		BUG()
#define SMC_outsb(a, r, p, l)		BUG()
#endif

#if !defined(SMC_insb) || !defined(SMC_outsb)
#define SMC_insb(a, r, p, l)		BUG()
#define SMC_outsb(a, r, p, l)		BUG()
#endif

#ifndef SMC_CAN_USE_DATACS
#define SMC_CAN_USE_DATACS	0
#endif

#ifndef SMC_IO_SHIFT
#define SMC_IO_SHIFT	0
#endif

#ifndef	SMC_IRQ_FLAGS
#define	SMC_IRQ_FLAGS		IRQF_TRIGGER_RISING
#endif

#ifndef SMC_INTERRUPT_PREAMBLE
#define SMC_INTERRUPT_PREAMBLE
#endif


#define SMC_IO_EXTENT	(16 << SMC_IO_SHIFT)
#define SMC_DATA_EXTENT (4)

#define BANK_SELECT		(14 << SMC_IO_SHIFT)


#define TCR_REG(lp) 	SMC_REG(lp, 0x0000, 0)
#define TCR_ENABLE	0x0001	
#define TCR_LOOP	0x0002	
#define TCR_FORCOL	0x0004	
#define TCR_PAD_EN	0x0080	
#define TCR_NOCRC	0x0100	
#define TCR_MON_CSN	0x0400	
#define TCR_FDUPLX    	0x0800  
#define TCR_STP_SQET	0x1000	
#define TCR_EPH_LOOP	0x2000	
#define TCR_SWFDUP	0x8000	

#define TCR_CLEAR	0	
#define TCR_DEFAULT	(TCR_ENABLE | TCR_PAD_EN)


#define EPH_STATUS_REG(lp)	SMC_REG(lp, 0x0002, 0)
#define ES_TX_SUC	0x0001	
#define ES_SNGL_COL	0x0002	
#define ES_MUL_COL	0x0004	
#define ES_LTX_MULT	0x0008	
#define ES_16COL	0x0010	
#define ES_SQET		0x0020	
#define ES_LTXBRD	0x0040	
#define ES_TXDEFR	0x0080	
#define ES_LATCOL	0x0200	
#define ES_LOSTCARR	0x0400	
#define ES_EXC_DEF	0x0800	
#define ES_CTR_ROL	0x1000	
#define ES_LINK_OK	0x4000	
#define ES_TXUNRN	0x8000	


#define RCR_REG(lp)		SMC_REG(lp, 0x0004, 0)
#define RCR_RX_ABORT	0x0001	
#define RCR_PRMS	0x0002	
#define RCR_ALMUL	0x0004	
#define RCR_RXEN	0x0100	
#define RCR_STRIP_CRC	0x0200	
#define RCR_ABORT_ENB	0x0200	
#define RCR_FILT_CAR	0x0400	
#define RCR_SOFTRST	0x8000 	

#define RCR_DEFAULT	(RCR_STRIP_CRC | RCR_RXEN)
#define RCR_CLEAR	0x0	


#define COUNTER_REG(lp)	SMC_REG(lp, 0x0006, 0)


#define MIR_REG(lp)		SMC_REG(lp, 0x0008, 0)


#define RPC_REG(lp)		SMC_REG(lp, 0x000A, 0)
#define RPC_SPEED	0x2000	
#define RPC_DPLX	0x1000	
#define RPC_ANEG	0x0800	
#define RPC_LSXA_SHFT	5	
#define RPC_LSXB_SHFT	2	

#ifndef RPC_LSA_DEFAULT
#define RPC_LSA_DEFAULT	RPC_LED_100
#endif
#ifndef RPC_LSB_DEFAULT
#define RPC_LSB_DEFAULT RPC_LED_FD
#endif

#define RPC_DEFAULT (RPC_ANEG | RPC_SPEED | RPC_DPLX)



#define BSR_REG		0x000E


#define CONFIG_REG(lp)	SMC_REG(lp, 0x0000,	1)
#define CONFIG_EXT_PHY	0x0200	
#define CONFIG_GPCNTRL	0x0400	
#define CONFIG_NO_WAIT	0x1000	
#define CONFIG_EPH_POWER_EN 0x8000 

#define CONFIG_DEFAULT	(CONFIG_EPH_POWER_EN)


#define BASE_REG(lp)	SMC_REG(lp, 0x0002, 1)


#define ADDR0_REG(lp)	SMC_REG(lp, 0x0004, 1)
#define ADDR1_REG(lp)	SMC_REG(lp, 0x0006, 1)
#define ADDR2_REG(lp)	SMC_REG(lp, 0x0008, 1)


#define GP_REG(lp)		SMC_REG(lp, 0x000A, 1)


#define CTL_REG(lp)		SMC_REG(lp, 0x000C, 1)
#define CTL_RCV_BAD	0x4000 
#define CTL_AUTO_RELEASE 0x0800 
#define CTL_LE_ENABLE	0x0080 
#define CTL_CR_ENABLE	0x0040 
#define CTL_TE_ENABLE	0x0020 
#define CTL_EEPROM_SELECT 0x0004 
#define CTL_RELOAD	0x0002 
#define CTL_STORE	0x0001 


#define MMU_CMD_REG(lp)	SMC_REG(lp, 0x0000, 2)
#define MC_BUSY		1	
#define MC_NOP		(0<<5)	
#define MC_ALLOC	(1<<5) 	
#define MC_RESET	(2<<5)	
#define MC_REMOVE	(3<<5) 	
#define MC_RELEASE  	(4<<5) 	
#define MC_FREEPKT  	(5<<5) 	
#define MC_ENQUEUE	(6<<5)	
#define MC_RSTTXFIFO	(7<<5)	


#define PN_REG(lp)		SMC_REG(lp, 0x0002, 2)


#define AR_REG(lp)		SMC_REG(lp, 0x0003, 2)
#define AR_FAILED	0x80	


#define TXFIFO_REG(lp)	SMC_REG(lp, 0x0004, 2)
#define TXFIFO_TEMPTY	0x80	

#define RXFIFO_REG(lp)	SMC_REG(lp, 0x0005, 2)
#define RXFIFO_REMPTY	0x80	

#define FIFO_REG(lp)	SMC_REG(lp, 0x0004, 2)

#define PTR_REG(lp)		SMC_REG(lp, 0x0006, 2)
#define PTR_RCV		0x8000 
#define PTR_AUTOINC 	0x4000 
#define PTR_READ	0x2000 


#define DATA_REG(lp)	SMC_REG(lp, 0x0008, 2)


#define INT_REG(lp)		SMC_REG(lp, 0x000C, 2)


#define IM_REG(lp)		SMC_REG(lp, 0x000D, 2)
#define IM_MDINT	0x80 
#define IM_ERCV_INT	0x40 
#define IM_EPH_INT	0x20 
#define IM_RX_OVRN_INT	0x10 
#define IM_ALLOC_INT	0x08 
#define IM_TX_EMPTY_INT	0x04 
#define IM_TX_INT	0x02 
#define IM_RCV_INT	0x01 


#define MCAST_REG1(lp)	SMC_REG(lp, 0x0000, 3)
#define MCAST_REG2(lp)	SMC_REG(lp, 0x0002, 3)
#define MCAST_REG3(lp)	SMC_REG(lp, 0x0004, 3)
#define MCAST_REG4(lp)	SMC_REG(lp, 0x0006, 3)


#define MII_REG(lp)		SMC_REG(lp, 0x0008, 3)
#define MII_MSK_CRS100	0x4000 
#define MII_MDOE	0x0008 
#define MII_MCLK	0x0004 
#define MII_MDI		0x0002 
#define MII_MDO		0x0001 


#define REV_REG(lp)		SMC_REG(lp, 0x000A, 3)


#define ERCV_REG(lp)	SMC_REG(lp, 0x000C, 3)
#define ERCV_RCV_DISCRD	0x0080 
#define ERCV_THRESHOLD	0x001F 


#define EXT_REG(lp)		SMC_REG(lp, 0x0000, 7)


#define CHIP_9192	3
#define CHIP_9194	4
#define CHIP_9195	5
#define CHIP_9196	6
#define CHIP_91100	7
#define CHIP_91100FD	8
#define CHIP_91111FD	9

static const char * chip_ids[ 16 ] =  {
	NULL, NULL, NULL,
	 "SMC91C90/91C92",
	 "SMC91C94",
	 "SMC91C95",
	 "SMC91C96",
	 "SMC91C100",
	 "SMC91C100FD",
	 "SMC91C11xFD",
	NULL, NULL, NULL,
	NULL, NULL, NULL};


#define RS_ALGNERR	0x8000
#define RS_BRODCAST	0x4000
#define RS_BADCRC	0x2000
#define RS_ODDFRAME	0x1000
#define RS_TOOLONG	0x0800
#define RS_TOOSHORT	0x0400
#define RS_MULTICAST	0x0001
#define RS_ERRORS	(RS_ALGNERR | RS_BADCRC | RS_TOOLONG | RS_TOOSHORT)


#define PHY_LAN83C183	0x0016f840
#define PHY_LAN83C180	0x02821c50


#define PHY_CFG1_REG		0x10
#define PHY_CFG1_LNKDIS		0x8000	
#define PHY_CFG1_XMTDIS		0x4000	
#define PHY_CFG1_XMTPDN		0x2000	
#define PHY_CFG1_BYPSCR		0x0400	
#define PHY_CFG1_UNSCDS		0x0200	
#define PHY_CFG1_EQLZR		0x0100	
#define PHY_CFG1_CABLE		0x0080	
#define PHY_CFG1_RLVL0		0x0040	
#define PHY_CFG1_TLVL_SHIFT	2	
#define PHY_CFG1_TLVL_MASK	0x003C
#define PHY_CFG1_TRF_MASK	0x0003	


#define PHY_CFG2_REG		0x11
#define PHY_CFG2_APOLDIS	0x0020	
#define PHY_CFG2_JABDIS		0x0010	
#define PHY_CFG2_MREG		0x0008	
#define PHY_CFG2_INTMDIO	0x0004	

#define PHY_INT_REG		0x12	
#define PHY_INT_INT		0x8000	
#define PHY_INT_LNKFAIL		0x4000	
#define PHY_INT_LOSSSYNC	0x2000	
#define PHY_INT_CWRD		0x1000	
#define PHY_INT_SSD		0x0800	
#define PHY_INT_ESD		0x0400	
#define PHY_INT_RPOL		0x0200	
#define PHY_INT_JAB		0x0100	
#define PHY_INT_SPDDET		0x0080	
#define PHY_INT_DPLXDET		0x0040	

#define PHY_MASK_REG		0x13	


#define ECOR			0x8000
#define ECOR_RESET		0x80
#define ECOR_LEVEL_IRQ		0x40
#define ECOR_WR_ATTRIB		0x04
#define ECOR_ENABLE		0x01

#define ECSR			0x8002
#define ECSR_IOIS8		0x20
#define ECSR_PWRDWN		0x04
#define ECSR_INT		0x02

#define ATTRIB_SIZE		((64*1024) << SMC_IO_SHIFT)



#if SMC_DEBUG > 0
#define SMC_REG(lp, reg, bank)					\
	({								\
		int __b = SMC_CURRENT_BANK(lp);			\
		if (unlikely((__b & ~0xf0) != (0x3300 | bank))) {	\
			printk( "%s: bank reg screwed (0x%04x)\n",	\
				CARDNAME, __b );			\
			BUG();						\
		}							\
		reg<<SMC_IO_SHIFT;					\
	})
#else
#define SMC_REG(lp, reg, bank)	(reg<<SMC_IO_SHIFT)
#endif

#define SMC_MUST_ALIGN_WRITE(lp)	SMC_32BIT(lp)

#define SMC_GET_PN(lp)						\
	(SMC_8BIT(lp)	? (SMC_inb(ioaddr, PN_REG(lp)))	\
				: (SMC_inw(ioaddr, PN_REG(lp)) & 0xFF))

#define SMC_SET_PN(lp, x)						\
	do {								\
		if (SMC_MUST_ALIGN_WRITE(lp))				\
			SMC_outl((x)<<16, ioaddr, SMC_REG(lp, 0, 2));	\
		else if (SMC_8BIT(lp))				\
			SMC_outb(x, ioaddr, PN_REG(lp));		\
		else							\
			SMC_outw(x, ioaddr, PN_REG(lp));		\
	} while (0)

#define SMC_GET_AR(lp)						\
	(SMC_8BIT(lp)	? (SMC_inb(ioaddr, AR_REG(lp)))	\
				: (SMC_inw(ioaddr, PN_REG(lp)) >> 8))

#define SMC_GET_TXFIFO(lp)						\
	(SMC_8BIT(lp)	? (SMC_inb(ioaddr, TXFIFO_REG(lp)))	\
				: (SMC_inw(ioaddr, TXFIFO_REG(lp)) & 0xFF))

#define SMC_GET_RXFIFO(lp)						\
	(SMC_8BIT(lp)	? (SMC_inb(ioaddr, RXFIFO_REG(lp)))	\
				: (SMC_inw(ioaddr, TXFIFO_REG(lp)) >> 8))

#define SMC_GET_INT(lp)						\
	(SMC_8BIT(lp)	? (SMC_inb(ioaddr, INT_REG(lp)))	\
				: (SMC_inw(ioaddr, INT_REG(lp)) & 0xFF))

#define SMC_ACK_INT(lp, x)						\
	do {								\
		if (SMC_8BIT(lp))					\
			SMC_outb(x, ioaddr, INT_REG(lp));		\
		else {							\
			unsigned long __flags;				\
			int __mask;					\
			local_irq_save(__flags);			\
			__mask = SMC_inw(ioaddr, INT_REG(lp)) & ~0xff; \
			SMC_outw(__mask | (x), ioaddr, INT_REG(lp));	\
			local_irq_restore(__flags);			\
		}							\
	} while (0)

#define SMC_GET_INT_MASK(lp)						\
	(SMC_8BIT(lp)	? (SMC_inb(ioaddr, IM_REG(lp)))	\
				: (SMC_inw(ioaddr, INT_REG(lp)) >> 8))

#define SMC_SET_INT_MASK(lp, x)					\
	do {								\
		if (SMC_8BIT(lp))					\
			SMC_outb(x, ioaddr, IM_REG(lp));		\
		else							\
			SMC_outw((x) << 8, ioaddr, INT_REG(lp));	\
	} while (0)

#define SMC_CURRENT_BANK(lp)	SMC_inw(ioaddr, BANK_SELECT)

#define SMC_SELECT_BANK(lp, x)					\
	do {								\
		if (SMC_MUST_ALIGN_WRITE(lp))				\
			SMC_outl((x)<<16, ioaddr, 12<<SMC_IO_SHIFT);	\
		else							\
			SMC_outw(x, ioaddr, BANK_SELECT);		\
	} while (0)

#define SMC_GET_BASE(lp)		SMC_inw(ioaddr, BASE_REG(lp))

#define SMC_SET_BASE(lp, x)		SMC_outw(x, ioaddr, BASE_REG(lp))

#define SMC_GET_CONFIG(lp)	SMC_inw(ioaddr, CONFIG_REG(lp))

#define SMC_SET_CONFIG(lp, x)	SMC_outw(x, ioaddr, CONFIG_REG(lp))

#define SMC_GET_COUNTER(lp)	SMC_inw(ioaddr, COUNTER_REG(lp))

#define SMC_GET_CTL(lp)		SMC_inw(ioaddr, CTL_REG(lp))

#define SMC_SET_CTL(lp, x)		SMC_outw(x, ioaddr, CTL_REG(lp))

#define SMC_GET_MII(lp)		SMC_inw(ioaddr, MII_REG(lp))

#define SMC_GET_GP(lp)		SMC_inw(ioaddr, GP_REG(lp))

#define SMC_SET_GP(lp, x)						\
	do {								\
		if (SMC_MUST_ALIGN_WRITE(lp))				\
			SMC_outl((x)<<16, ioaddr, SMC_REG(lp, 8, 1));	\
		else							\
			SMC_outw(x, ioaddr, GP_REG(lp));		\
	} while (0)

#define SMC_SET_MII(lp, x)		SMC_outw(x, ioaddr, MII_REG(lp))

#define SMC_GET_MIR(lp)		SMC_inw(ioaddr, MIR_REG(lp))

#define SMC_SET_MIR(lp, x)		SMC_outw(x, ioaddr, MIR_REG(lp))

#define SMC_GET_MMU_CMD(lp)	SMC_inw(ioaddr, MMU_CMD_REG(lp))

#define SMC_SET_MMU_CMD(lp, x)	SMC_outw(x, ioaddr, MMU_CMD_REG(lp))

#define SMC_GET_FIFO(lp)		SMC_inw(ioaddr, FIFO_REG(lp))

#define SMC_GET_PTR(lp)		SMC_inw(ioaddr, PTR_REG(lp))

#define SMC_SET_PTR(lp, x)						\
	do {								\
		if (SMC_MUST_ALIGN_WRITE(lp))				\
			SMC_outl((x)<<16, ioaddr, SMC_REG(lp, 4, 2));	\
		else							\
			SMC_outw(x, ioaddr, PTR_REG(lp));		\
	} while (0)

#define SMC_GET_EPH_STATUS(lp)	SMC_inw(ioaddr, EPH_STATUS_REG(lp))

#define SMC_GET_RCR(lp)		SMC_inw(ioaddr, RCR_REG(lp))

#define SMC_SET_RCR(lp, x)		SMC_outw(x, ioaddr, RCR_REG(lp))

#define SMC_GET_REV(lp)		SMC_inw(ioaddr, REV_REG(lp))

#define SMC_GET_RPC(lp)		SMC_inw(ioaddr, RPC_REG(lp))

#define SMC_SET_RPC(lp, x)						\
	do {								\
		if (SMC_MUST_ALIGN_WRITE(lp))				\
			SMC_outl((x)<<16, ioaddr, SMC_REG(lp, 8, 0));	\
		else							\
			SMC_outw(x, ioaddr, RPC_REG(lp));		\
	} while (0)

#define SMC_GET_TCR(lp)		SMC_inw(ioaddr, TCR_REG(lp))

#define SMC_SET_TCR(lp, x)		SMC_outw(x, ioaddr, TCR_REG(lp))

#ifndef SMC_GET_MAC_ADDR
#define SMC_GET_MAC_ADDR(lp, addr)					\
	do {								\
		unsigned int __v;					\
		__v = SMC_inw(ioaddr, ADDR0_REG(lp));			\
		addr[0] = __v; addr[1] = __v >> 8;			\
		__v = SMC_inw(ioaddr, ADDR1_REG(lp));			\
		addr[2] = __v; addr[3] = __v >> 8;			\
		__v = SMC_inw(ioaddr, ADDR2_REG(lp));			\
		addr[4] = __v; addr[5] = __v >> 8;			\
	} while (0)
#endif

#define SMC_SET_MAC_ADDR(lp, addr)					\
	do {								\
		SMC_outw(addr[0]|(addr[1] << 8), ioaddr, ADDR0_REG(lp)); \
		SMC_outw(addr[2]|(addr[3] << 8), ioaddr, ADDR1_REG(lp)); \
		SMC_outw(addr[4]|(addr[5] << 8), ioaddr, ADDR2_REG(lp)); \
	} while (0)

#define SMC_SET_MCAST(lp, x)						\
	do {								\
		const unsigned char *mt = (x);				\
		SMC_outw(mt[0] | (mt[1] << 8), ioaddr, MCAST_REG1(lp)); \
		SMC_outw(mt[2] | (mt[3] << 8), ioaddr, MCAST_REG2(lp)); \
		SMC_outw(mt[4] | (mt[5] << 8), ioaddr, MCAST_REG3(lp)); \
		SMC_outw(mt[6] | (mt[7] << 8), ioaddr, MCAST_REG4(lp)); \
	} while (0)

#define SMC_PUT_PKT_HDR(lp, status, length)				\
	do {								\
		if (SMC_32BIT(lp))					\
			SMC_outl((status) | (length)<<16, ioaddr,	\
				 DATA_REG(lp));			\
		else {							\
			SMC_outw(status, ioaddr, DATA_REG(lp));	\
			SMC_outw(length, ioaddr, DATA_REG(lp));	\
		}							\
	} while (0)

#define SMC_GET_PKT_HDR(lp, status, length)				\
	do {								\
		if (SMC_32BIT(lp)) {				\
			unsigned int __val = SMC_inl(ioaddr, DATA_REG(lp)); \
			(status) = __val & 0xffff;			\
			(length) = __val >> 16;				\
		} else {						\
			(status) = SMC_inw(ioaddr, DATA_REG(lp));	\
			(length) = SMC_inw(ioaddr, DATA_REG(lp));	\
		}							\
	} while (0)

#define SMC_PUSH_DATA(lp, p, l)					\
	do {								\
		if (SMC_32BIT(lp)) {				\
			void *__ptr = (p);				\
			int __len = (l);				\
			void __iomem *__ioaddr = ioaddr;		\
			if (__len >= 2 && (unsigned long)__ptr & 2) {	\
				__len -= 2;				\
				SMC_outw(*(u16 *)__ptr, ioaddr,		\
					DATA_REG(lp));		\
				__ptr += 2;				\
			}						\
			if (SMC_CAN_USE_DATACS && lp->datacs)		\
				__ioaddr = lp->datacs;			\
			SMC_outsl(__ioaddr, DATA_REG(lp), __ptr, __len>>2); \
			if (__len & 2) {				\
				__ptr += (__len & ~3);			\
				SMC_outw(*((u16 *)__ptr), ioaddr,	\
					 DATA_REG(lp));		\
			}						\
		} else if (SMC_16BIT(lp))				\
			SMC_outsw(ioaddr, DATA_REG(lp), p, (l) >> 1);	\
		else if (SMC_8BIT(lp))				\
			SMC_outsb(ioaddr, DATA_REG(lp), p, l);	\
	} while (0)

#define SMC_PULL_DATA(lp, p, l)					\
	do {								\
		if (SMC_32BIT(lp)) {				\
			void *__ptr = (p);				\
			int __len = (l);				\
			void __iomem *__ioaddr = ioaddr;		\
			if ((unsigned long)__ptr & 2) {			\
					\
				__ptr -= 2;				\
				__len += 2;				\
				SMC_SET_PTR(lp,			\
					2|PTR_READ|PTR_RCV|PTR_AUTOINC); \
			}						\
			if (SMC_CAN_USE_DATACS && lp->datacs)		\
				__ioaddr = lp->datacs;			\
			__len += 2;					\
			SMC_insl(__ioaddr, DATA_REG(lp), __ptr, __len>>2); \
		} else if (SMC_16BIT(lp))				\
			SMC_insw(ioaddr, DATA_REG(lp), p, (l) >> 1);	\
		else if (SMC_8BIT(lp))				\
			SMC_insb(ioaddr, DATA_REG(lp), p, l);		\
	} while (0)

#endif  
