#ifndef __SOUND_CS8427_H
#define __SOUND_CS8427_H

/*
 *  Routines for Cirrus Logic CS8427
 *  Copyright (c) by Jaroslav Kysela <perex@perex.cz>,
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <sound/i2c.h>

#define CS8427_BASE_ADDR	0x10	

#define CS8427_REG_AUTOINC	0x80	
#define CS8427_REG_CONTROL1	0x01
#define CS8427_REG_CONTROL2	0x02
#define CS8427_REG_DATAFLOW	0x03
#define CS8427_REG_CLOCKSOURCE	0x04
#define CS8427_REG_SERIALINPUT	0x05
#define CS8427_REG_SERIALOUTPUT	0x06
#define CS8427_REG_INT1STATUS	0x07
#define CS8427_REG_INT2STATUS	0x08
#define CS8427_REG_INT1MASK	0x09
#define CS8427_REG_INT1MODEMSB	0x0a
#define CS8427_REG_INT1MODELSB	0x0b
#define CS8427_REG_INT2MASK	0x0c
#define CS8427_REG_INT2MODEMSB	0x0d
#define CS8427_REG_INT2MODELSB	0x0e
#define CS8427_REG_RECVCSDATA	0x0f
#define CS8427_REG_RECVERRORS	0x10
#define CS8427_REG_RECVERRMASK	0x11
#define CS8427_REG_CSDATABUF	0x12
#define CS8427_REG_UDATABUF	0x13
#define CS8427_REG_QSUBCODE	0x14	
#define CS8427_REG_OMCKRMCKRATIO 0x1e
#define CS8427_REG_CORU_DATABUF	0x20	
#define CS8427_REG_ID_AND_VER	0x7f

#define CS8427_SWCLK		(1<<7)	
#define CS8427_VSET		(1<<6)	
#define CS8427_MUTESAO		(1<<5)	
#define CS8427_MUTEAES		(1<<4)	
#define CS8427_INTMASK		(3<<1)	
#define CS8427_INTACTHIGH	(0<<1)	
#define CS8427_INTACTLOW	(1<<1)	
#define CS8427_INTOPENDRAIN	(2<<1)	
#define CS8427_TCBLDIR		(1<<0)	

#define CS8427_HOLDMASK		(3<<5)	
#define CS8427_HOLDLASTSAMPLE	(0<<5)	
#define CS8427_HOLDZERO		(1<<5)	
#define CS8427_HOLDNOCHANGE	(2<<5)	
#define CS8427_RMCKF		(1<<4)	
#define CS8427_MMR		(1<<3)	
#define CS8427_MMT		(1<<2)	
#define CS8427_MMTCS		(1<<1)	
#define CS8427_MMTLR		(1<<0)	

#define CS8427_TXOFF		(1<<6)	
#define CS8427_AESBP		(1<<5)	
#define CS8427_TXDMASK		(3<<3)	
#define CS8427_TXDSERIAL	(1<<3)	
#define CS8427_TXAES3DRECEIVER	(2<<3)	
#define CS8427_SPDMASK		(3<<1)	
#define CS8427_SPDSERIAL	(1<<1)	
#define CS8427_SPDAES3RECEIVER	(2<<1)	

#define CS8427_RUN		(1<<6)	
#define CS8427_CLKMASK		(3<<4)	
#define CS8427_CLK256		(0<<4)	
#define CS8427_CLK384		(1<<4)	
#define CS8427_CLK512		(2<<4)	
#define CS8427_OUTC		(1<<3)	
#define CS8427_INC		(1<<2)	
#define CS8427_RXDMASK		(3<<0)	
#define CS8427_RXDILRCK		(0<<0)	
#define CS8427_RXDAES3INPUT	(1<<0)	
#define CS8427_EXTCLOCKRESET	(2<<0)	
#define CS8427_EXTCLOCK		(3<<0)	

#define CS8427_SIMS		(1<<7)	
#define CS8427_SISF		(1<<6)	
#define CS8427_SIRESMASK	(3<<4)	
#define CS8427_SIRES24		(0<<4)	
#define CS8427_SIRES20		(1<<4)	
#define CS8427_SIRES16		(2<<4)	
#define CS8427_SIJUST		(1<<3)	
#define CS8427_SIDEL		(1<<2)	
#define CS8427_SISPOL		(1<<1)	
#define CS8427_SILRPOL		(1<<0)	
#define CS8427_BITWIDTH_MASK	0xCF

#define CS8427_SOMS		(1<<7)	
#define CS8427_SOSF		(1<<6)	
#define CS8427_SORESMASK	(3<<4)	
#define CS8427_SORES24		(0<<4)	
#define CS8427_SORES20		(1<<4)	
#define CS8427_SORES16		(2<<4)	
#define CS8427_SORESDIRECT	(2<<4)	
#define CS8427_SOJUST		(1<<3)	
#define CS8427_SODEL		(1<<2)	
#define CS8427_SOSPOL		(1<<1)	
#define CS8427_SOLRPOL		(1<<0)	

#define CS8427_TSLIP		(1<<7)	
#define CS8427_OSLIP		(1<<6)	
#define CS8427_DETC		(1<<2)	
#define CS8427_EFTC		(1<<1)	
#define CS8427_RERR		(1<<0)	

#define CS8427_DETU		(1<<3)	
#define CS8427_EFTU		(1<<2)	
#define CS8427_QCH		(1<<1)	

#define CS8427_INTMODERISINGMSB	0
#define CS8427_INTMODERESINGLSB	0
#define CS8427_INTMODEFALLINGMSB 0
#define CS8427_INTMODEFALLINGLSB 1
#define CS8427_INTMODELEVELMSB	1
#define CS8427_INTMODELEVELLSB	0

#define CS8427_AUXMASK		(15<<4)	
#define CS8427_AUXSHIFT		4
#define CS8427_PRO		(1<<3)	
#define CS8427_AUDIO		(1<<2)	
#define CS8427_COPY		(1<<1)	/* 0 = copyright asserted, 1 = copyright not asserted */
#define CS8427_ORIG		(1<<0)	

#define CS8427_QCRC		(1<<6)	
#define CS8427_CCRC		(1<<5)	
#define CS8427_UNLOCK		(1<<4)	
#define CS8427_V		(1<<3)	
#define CS8427_CONF		(1<<2)	
#define CS8427_BIP		(1<<1)	
#define CS8427_PAR		(1<<0)	

#define CS8427_BSEL		(1<<5)	
#define CS8427_CBMR		(1<<4)	
#define CS8427_DETCI		(1<<3)	
#define CS8427_EFTCI		(1<<2)	
#define CS8427_CAM		(1<<1)	
#define CS8427_CHS		(1<<0)	

#define CS8427_UD		(1<<4)	
#define CS8427_UBMMASK		(3<<2)	
#define CS8427_UBMZEROS		(0<<2)	
#define CS8427_UBMBLOCK		(1<<2)	
#define CS8427_DETUI		(1<<1)	
#define CS8427_EFTUI		(1<<1)	

#define CS8427_IDMASK		(15<<4)
#define CS8427_IDSHIFT		4
#define CS8427_VERMASK		(15<<0)
#define CS8427_VERSHIFT		0
#define CS8427_VER8427A		0x71

#define CS8427_ADDR0 0x10
#define CS8427_ADDR1 0x11
#define CS8427_ADDR2 0x12
#define CS8427_ADDR3 0x13
#define CS8427_ADDR4 0x14
#define CS8427_ADDR5 0x15
#define CS8427_ADDR6 0x16
#define CS8427_ADDR7 0x17

#define CHANNEL_STATUS_SIZE	24

struct cs8427_platform_data {
	int irq;
	int irq_base;
	int num_irqs;
	int reset_gpio;
	int (*enable) (int enable);
};

struct snd_pcm_substream;

int snd_cs8427_create(struct snd_i2c_bus *bus, unsigned char addr,
		      unsigned int reset_timeout, struct snd_i2c_device **r_cs8427);
int snd_cs8427_reg_write(struct snd_i2c_device *device, unsigned char reg,
			 unsigned char val);
int snd_cs8427_iec958_build(struct snd_i2c_device *cs8427,
			    struct snd_pcm_substream *playback_substream,
			    struct snd_pcm_substream *capture_substream);
int snd_cs8427_iec958_active(struct snd_i2c_device *cs8427, int active);
int snd_cs8427_iec958_pcm(struct snd_i2c_device *cs8427, unsigned int rate);
#endif 
