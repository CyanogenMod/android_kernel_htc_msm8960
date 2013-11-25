#ifndef __SOUND_AC97_CODEC_H
#define __SOUND_AC97_CODEC_H

/*
 *  Copyright (c) by Jaroslav Kysela <perex@perex.cz>
 *  Universal interface for Audio Codec '97
 *
 *  For more details look to AC '97 component specification revision 2.1
 *  by Intel Corporation (http://developer.intel.com).
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

#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include "pcm.h"
#include "control.h"
#include "info.h"

#define	AC97_BUS_MAX_DEVICES	4


#define AC97_RESET		0x00	
#define AC97_MASTER		0x02	
#define AC97_HEADPHONE		0x04	
#define AC97_MASTER_MONO	0x06	
#define AC97_MASTER_TONE	0x08	
#define AC97_PC_BEEP		0x0a	
#define AC97_PHONE		0x0c	
#define AC97_MIC		0x0e	
#define AC97_LINE		0x10	
#define AC97_CD			0x12	
#define AC97_VIDEO		0x14	
#define AC97_AUX		0x16	
#define AC97_PCM		0x18	
#define AC97_REC_SEL		0x1a	
#define AC97_REC_GAIN		0x1c	
#define AC97_REC_GAIN_MIC	0x1e	
#define AC97_GENERAL_PURPOSE	0x20	
#define AC97_3D_CONTROL		0x22	
#define AC97_INT_PAGING		0x24	
#define AC97_POWERDOWN		0x26	
#define AC97_EXTENDED_ID	0x28	
#define AC97_EXTENDED_STATUS	0x2a	
#define AC97_PCM_FRONT_DAC_RATE 0x2c	
#define AC97_PCM_SURR_DAC_RATE	0x2e	
#define AC97_PCM_LFE_DAC_RATE	0x30	
#define AC97_PCM_LR_ADC_RATE	0x32	
#define AC97_PCM_MIC_ADC_RATE	0x34	
#define AC97_CENTER_LFE_MASTER	0x36	
#define AC97_SURROUND_MASTER	0x38	
#define AC97_SPDIF		0x3a	
#define AC97_EXTENDED_MID	0x3c	
#define AC97_EXTENDED_MSTATUS	0x3e	
#define AC97_LINE1_RATE		0x40	
#define AC97_LINE2_RATE		0x42	
#define AC97_HANDSET_RATE	0x44	
#define AC97_LINE1_LEVEL	0x46	
#define AC97_LINE2_LEVEL	0x48	
#define AC97_HANDSET_LEVEL	0x4a	
#define AC97_GPIO_CFG		0x4c	
#define AC97_GPIO_POLARITY	0x4e	
#define AC97_GPIO_STICKY	0x50	
#define AC97_GPIO_WAKEUP	0x52	
#define AC97_GPIO_STATUS	0x54	
#define AC97_MISC_AFE		0x56	
#define AC97_VENDOR_ID1		0x7c	
#define AC97_VENDOR_ID2		0x7e	
#define AC97_CODEC_CLASS_REV	0x60	
#define AC97_PCI_SVID		0x62	
#define AC97_PCI_SID		0x64	
#define AC97_FUNC_SELECT	0x66	
#define AC97_FUNC_INFO		0x68	
#define AC97_SENSE_INFO		0x6a	

#define AC97_MUTE_MASK_MONO	0x8000
#define AC97_MUTE_MASK_STEREO	0x8080

#define AC97_SLOT_TAG		0
#define AC97_SLOT_CMD_ADDR	1
#define AC97_SLOT_CMD_DATA	2
#define AC97_SLOT_PCM_LEFT	3
#define AC97_SLOT_PCM_RIGHT	4
#define AC97_SLOT_MODEM_LINE1	5
#define AC97_SLOT_PCM_CENTER	6
#define AC97_SLOT_MIC		6	
#define AC97_SLOT_SPDIF_LEFT1	6
#define AC97_SLOT_PCM_SLEFT	7	
#define AC97_SLOT_PCM_LEFT_0	7	
#define AC97_SLOT_SPDIF_LEFT	7
#define AC97_SLOT_PCM_SRIGHT	8	
#define AC97_SLOT_PCM_RIGHT_0	8	
#define AC97_SLOT_SPDIF_RIGHT	8
#define AC97_SLOT_LFE		9
#define AC97_SLOT_SPDIF_RIGHT1	9
#define AC97_SLOT_MODEM_LINE2	10
#define AC97_SLOT_PCM_LEFT_1	10	
#define AC97_SLOT_SPDIF_LEFT2	10
#define AC97_SLOT_HANDSET	11	
#define AC97_SLOT_PCM_RIGHT_1	11	
#define AC97_SLOT_SPDIF_RIGHT2	11
#define AC97_SLOT_MODEM_GPIO	12	
#define AC97_SLOT_PCM_CENTER_1	12	

#define AC97_BC_DEDICATED_MIC	0x0001	
#define AC97_BC_RESERVED1	0x0002	
#define AC97_BC_BASS_TREBLE	0x0004	
#define AC97_BC_SIM_STEREO	0x0008	
#define AC97_BC_HEADPHONE	0x0010	
#define AC97_BC_LOUDNESS	0x0020	
#define AC97_BC_16BIT_DAC	0x0000	
#define AC97_BC_18BIT_DAC	0x0040	
#define AC97_BC_20BIT_DAC	0x0080	
#define AC97_BC_DAC_MASK	0x00c0
#define AC97_BC_16BIT_ADC	0x0000	
#define AC97_BC_18BIT_ADC	0x0100	
#define AC97_BC_20BIT_ADC	0x0200	
#define AC97_BC_ADC_MASK	0x0300
#define AC97_BC_3D_TECH_ID_MASK	0x7c00	

#define AC97_GP_DRSS_MASK	0x0c00	
#define AC97_GP_DRSS_1011	0x0000	
#define AC97_GP_DRSS_78		0x0400	

#define AC97_PD_ADC_STATUS	0x0001	
#define AC97_PD_DAC_STATUS	0x0002	
#define AC97_PD_MIXER_STATUS	0x0004	
#define AC97_PD_VREF_STATUS	0x0008	
#define AC97_PD_PR0		0x0100	
#define AC97_PD_PR1		0x0200	
#define AC97_PD_PR2		0x0400	
#define AC97_PD_PR3		0x0800	
#define AC97_PD_PR4		0x1000	
#define AC97_PD_PR5		0x2000	
#define AC97_PD_PR6		0x4000	
#define AC97_PD_EAPD		0x8000	

#define AC97_EI_VRA		0x0001	
#define AC97_EI_DRA		0x0002	
#define AC97_EI_SPDIF		0x0004	
#define AC97_EI_VRM		0x0008	
#define AC97_EI_DACS_SLOT_MASK	0x0030	
#define AC97_EI_DACS_SLOT_SHIFT	4
#define AC97_EI_CDAC		0x0040	
#define AC97_EI_SDAC		0x0080	
#define AC97_EI_LDAC		0x0100	
#define AC97_EI_AMAP		0x0200	
#define AC97_EI_REV_MASK	0x0c00	
#define AC97_EI_REV_22		0x0400	
#define AC97_EI_REV_23		0x0800	
#define AC97_EI_REV_SHIFT	10
#define AC97_EI_ADDR_MASK	0xc000	
#define AC97_EI_ADDR_SHIFT	14

#define AC97_EA_VRA		0x0001	
#define AC97_EA_DRA		0x0002	
#define AC97_EA_SPDIF		0x0004	
#define AC97_EA_VRM		0x0008	
#define AC97_EA_SPSA_SLOT_MASK	0x0030	
#define AC97_EA_SPSA_SLOT_SHIFT 4
#define AC97_EA_SPSA_3_4	0x0000	
#define AC97_EA_SPSA_7_8	0x0010	
#define AC97_EA_SPSA_6_9	0x0020	
#define AC97_EA_SPSA_10_11	0x0030	
#define AC97_EA_CDAC		0x0040	
#define AC97_EA_SDAC		0x0080	
#define AC97_EA_LDAC		0x0100	
#define AC97_EA_MDAC		0x0200	
#define AC97_EA_SPCV		0x0400	
#define AC97_EA_PRI		0x0800	
#define AC97_EA_PRJ		0x1000	
#define AC97_EA_PRK		0x2000	
#define AC97_EA_PRL		0x4000	

#define AC97_SC_PRO		0x0001	
#define AC97_SC_NAUDIO		0x0002	
#define AC97_SC_COPY		0x0004	/* Copyright status */
#define AC97_SC_PRE		0x0008	
#define AC97_SC_CC_MASK		0x07f0	
#define AC97_SC_CC_SHIFT	4
#define AC97_SC_L		0x0800	
#define AC97_SC_SPSR_MASK	0x3000	
#define AC97_SC_SPSR_SHIFT	12
#define AC97_SC_SPSR_44K	0x0000	
#define AC97_SC_SPSR_48K	0x2000	
#define AC97_SC_SPSR_32K	0x3000	
#define AC97_SC_DRS		0x4000	
#define AC97_SC_V		0x8000	

#define AC97_PAGE_MASK		0x000f	
#define AC97_PAGE_VENDOR	0	
#define AC97_PAGE_1		1	
#define AC97_INT_ENABLE		0x0800	
#define AC97_INT_SENSE		0x1000	
#define AC97_INT_CAUSE_SENSE	0x2000	
#define AC97_INT_CAUSE_GPIO	0x4000	
#define AC97_INT_STATUS		0x8000	

#define AC97_MEI_LINE1		0x0001	
#define AC97_MEI_LINE2		0x0002	
#define AC97_MEI_HANDSET	0x0004	
#define AC97_MEI_CID1		0x0008	
#define AC97_MEI_CID2		0x0010	
#define AC97_MEI_ADDR_MASK	0xc000	
#define AC97_MEI_ADDR_SHIFT	14

#define AC97_MEA_GPIO		0x0001	
#define AC97_MEA_MREF		0x0002	
#define AC97_MEA_ADC1		0x0004	
#define AC97_MEA_DAC1		0x0008	
#define AC97_MEA_ADC2		0x0010	
#define AC97_MEA_DAC2		0x0020	
#define AC97_MEA_HADC		0x0040	
#define AC97_MEA_HDAC		0x0080	
#define AC97_MEA_PRA		0x0100	
#define AC97_MEA_PRB		0x0200	
#define AC97_MEA_PRC		0x0400	
#define AC97_MEA_PRD		0x0800	
#define AC97_MEA_PRE		0x1000	
#define AC97_MEA_PRF		0x2000	
#define AC97_MEA_PRG		0x4000	
#define AC97_MEA_PRH		0x8000	

#define AC97_GPIO_LINE1_OH      0x0001  
#define AC97_GPIO_LINE1_RI      0x0002  
#define AC97_GPIO_LINE1_CID     0x0004  
#define AC97_GPIO_LINE1_LCS     0x0008  
#define AC97_GPIO_LINE1_PULSE   0x0010  
#define AC97_GPIO_LINE1_HL1R    0x0020  
#define AC97_GPIO_LINE1_HOHD    0x0040  
#define AC97_GPIO_LINE12_AC     0x0080  
#define AC97_GPIO_LINE12_DC     0x0100  
#define AC97_GPIO_LINE12_RS     0x0200  
#define AC97_GPIO_LINE2_OH      0x0400  
#define AC97_GPIO_LINE2_RI      0x0800  
#define AC97_GPIO_LINE2_CID     0x1000  
#define AC97_GPIO_LINE2_LCS     0x2000  
#define AC97_GPIO_LINE2_PULSE   0x4000  
#define AC97_GPIO_LINE2_HL1R    0x8000  

#define AC97_SIGMATEL_OUTSEL	0x64	
#define AC97_SIGMATEL_INSEL	0x66	
#define AC97_SIGMATEL_IOMISC	0x68	
#define AC97_SIGMATEL_ANALOG	0x6c	
#define AC97_SIGMATEL_DAC2INVERT 0x6e
#define AC97_SIGMATEL_BIAS1	0x70
#define AC97_SIGMATEL_BIAS2	0x72
#define AC97_SIGMATEL_VARIOUS	0x72	
#define AC97_SIGMATEL_MULTICHN	0x74	
#define AC97_SIGMATEL_CIC1	0x76
#define AC97_SIGMATEL_CIC2	0x78

#define AC97_AD_TEST		0x5a	
#define AC97_AD_TEST2		0x5c	
#define AC97_AD_HPFD_SHIFT	12	
#define AC97_AD_CODEC_CFG	0x70	
#define AC97_AD_JACK_SPDIF	0x72	
#define AC97_AD_SERIAL_CFG	0x74	
#define AC97_AD_MISC		0x76	
#define AC97_AD_VREFD_SHIFT	2	

#define AC97_CSR_ACMODE		0x5e	
#define AC97_CSR_MISC_CRYSTAL	0x60	
#define AC97_CSR_SPDIF		0x68	
#define AC97_CSR_SERIAL		0x6a	
#define AC97_CSR_SPECF_ADDR	0x6c	
#define AC97_CSR_SPECF_DATA	0x6e	
#define AC97_CSR_BDI_STATUS	0x7a	

#define AC97_CXR_AUDIO_MISC	0x5c
#define AC97_CXR_SPDIFEN	(1<<3)
#define AC97_CXR_COPYRGT	(1<<2)
#define AC97_CXR_SPDIF_MASK	(3<<0)
#define AC97_CXR_SPDIF_PCM	0x0
#define AC97_CXR_SPDIF_AC3	0x2

#define AC97_ALC650_SPDIF_INPUT_STATUS1	0x60
#define AC97_ALC650_PRO             0x0001  
#define AC97_ALC650_NAUDIO          0x0002  
#define AC97_ALC650_COPY            0x0004  /* Copyright status */
#define AC97_ALC650_PRE             0x0038  
#define AC97_ALC650_PRE_SHIFT       3
#define AC97_ALC650_MODE            0x00C0  
#define AC97_ALC650_MODE_SHIFT      6
#define AC97_ALC650_CC_MASK         0x7f00  
#define AC97_ALC650_CC_SHIFT        8
#define AC97_ALC650_L               0x8000  

#define AC97_ALC650_SPDIF_INPUT_STATUS2	0x62
#define AC97_ALC650_SOUCE_MASK      0x000f  
#define AC97_ALC650_CHANNEL_MASK    0x00f0  
#define AC97_ALC650_CHANNEL_SHIFT   4 
#define AC97_ALC650_SPSR_MASK       0x0f00  
#define AC97_ALC650_SPSR_SHIFT      8
#define AC97_ALC650_SPSR_44K        0x0000  
#define AC97_ALC650_SPSR_48K        0x0200  
#define AC97_ALC650_SPSR_32K        0x0300  
#define AC97_ALC650_CLOCK_ACCURACY  0x3000  
#define AC97_ALC650_CLOCK_SHIFT     12
#define AC97_ALC650_CLOCK_LOCK      0x4000  
#define AC97_ALC650_V               0x8000  

#define AC97_ALC650_SURR_DAC_VOL	0x64
#define AC97_ALC650_LFE_DAC_VOL		0x66
#define AC97_ALC650_UNKNOWN1		0x68
#define AC97_ALC650_MULTICH		0x6a
#define AC97_ALC650_UNKNOWN2		0x6c
#define AC97_ALC650_REVISION		0x6e
#define AC97_ALC650_UNKNOWN3		0x70
#define AC97_ALC650_UNKNOWN4		0x72
#define AC97_ALC650_MISC		0x74
#define AC97_ALC650_GPIO_SETUP		0x76
#define AC97_ALC650_GPIO_STATUS		0x78
#define AC97_ALC650_CLOCK		0x7a

#define AC97_YMF7X3_DIT_CTRL	0x66	
#define AC97_YMF7X3_3D_MODE_SEL	0x68	

#define AC97_CM9738_VENDOR_CTRL	0x5a
#define AC97_CM9739_MULTI_CHAN	0x64
#define AC97_CM9739_SPDIF_IN_STATUS	0x68 
#define AC97_CM9739_SPDIF_CTRL	0x6c

#define AC97_WM97XX_FMIXER_VOL  0x72
#define AC97_WM9704_RMIXER_VOL  0x74
#define AC97_WM9704_TEST        0x5a
#define AC97_WM9704_RPCM_VOL    0x70
#define AC97_WM9711_OUT3VOL     0x16


#define AC97_SCAP_AUDIO		(1<<0)	
#define AC97_SCAP_MODEM		(1<<1)	
#define AC97_SCAP_SURROUND_DAC	(1<<2)	
#define AC97_SCAP_CENTER_LFE_DAC (1<<3)	
#define AC97_SCAP_SKIP_AUDIO	(1<<4)	
#define AC97_SCAP_SKIP_MODEM	(1<<5)	
#define AC97_SCAP_INDEP_SDIN	(1<<6)	
#define AC97_SCAP_INV_EAPD	(1<<7)	
#define AC97_SCAP_DETECT_BY_VENDOR (1<<8) 
#define AC97_SCAP_NO_SPDIF	(1<<9)	
#define AC97_SCAP_EAPD_LED	(1<<10)	
#define AC97_SCAP_POWER_SAVE	(1<<11)	

#define AC97_HAS_PC_BEEP	(1<<0)	
#define AC97_AD_MULTI		(1<<1)	
#define AC97_CS_SPDIF		(1<<2)	
#define AC97_CX_SPDIF		(1<<3)	
#define AC97_STEREO_MUTES	(1<<4)	
#define AC97_DOUBLE_RATE	(1<<5)	
#define AC97_HAS_NO_MASTER_VOL	(1<<6)	
#define AC97_HAS_NO_PCM_VOL	(1<<7)	
#define AC97_DEFAULT_POWER_OFF	(1<<8)	
#define AC97_MODEM_PATCH	(1<<9)	
#define AC97_HAS_NO_REC_GAIN	(1<<10) 
#define AC97_HAS_NO_PHONE	(1<<11) 
#define AC97_HAS_NO_PC_BEEP	(1<<12) 
#define AC97_HAS_NO_VIDEO	(1<<13) 
#define AC97_HAS_NO_CD		(1<<14) 
#define AC97_HAS_NO_MIC	(1<<15) 
#define AC97_HAS_NO_TONE	(1<<16) 
#define AC97_HAS_NO_STD_PCM	(1<<17)	
#define AC97_HAS_NO_AUX		(1<<18) 
#define AC97_HAS_8CH		(1<<19) 

#define AC97_RATES_FRONT_DAC	0
#define AC97_RATES_SURR_DAC	1
#define AC97_RATES_LFE_DAC	2
#define AC97_RATES_ADC		3
#define AC97_RATES_MIC_ADC	4
#define AC97_RATES_SPDIF	5


struct snd_ac97;

struct snd_ac97_build_ops {
	int (*build_3d) (struct snd_ac97 *ac97);
	int (*build_specific) (struct snd_ac97 *ac97);
	int (*build_spdif) (struct snd_ac97 *ac97);
	int (*build_post_spdif) (struct snd_ac97 *ac97);
#ifdef CONFIG_PM
	void (*suspend) (struct snd_ac97 *ac97);
	void (*resume) (struct snd_ac97 *ac97);
#endif
	void (*update_jacks) (struct snd_ac97 *ac97);	
};

struct snd_ac97_bus_ops {
	void (*reset) (struct snd_ac97 *ac97);
	void (*warm_reset)(struct snd_ac97 *ac97);
	void (*write) (struct snd_ac97 *ac97, unsigned short reg, unsigned short val);
	unsigned short (*read) (struct snd_ac97 *ac97, unsigned short reg);
	void (*wait) (struct snd_ac97 *ac97);
	void (*init) (struct snd_ac97 *ac97);
};

struct snd_ac97_bus {
	
	struct snd_ac97_bus_ops *ops;
	void *private_data;
	void (*private_free) (struct snd_ac97_bus *bus);
	
	struct snd_card *card;
	unsigned short num;	
	unsigned short no_vra: 1, 
		       dra: 1,	
		       isdin: 1;
	unsigned int clock;	
	spinlock_t bus_lock;	
	unsigned short used_slots[2][4]; 
	unsigned short pcms_count; 
	struct ac97_pcm *pcms;
	struct snd_ac97 *codec[4];
	struct snd_info_entry *proc;
};

struct snd_ac97_res_table {
	unsigned short reg;	
	unsigned short bits;	
};

struct snd_ac97_template {
	void *private_data;
	void (*private_free) (struct snd_ac97 *ac97);
	struct pci_dev *pci;	
	unsigned short num;	
	unsigned short addr;	
	unsigned int scaps;	
	const struct snd_ac97_res_table *res_table;	
};

struct snd_ac97 {
	
	const struct snd_ac97_build_ops *build_ops;
	void *private_data;
	void (*private_free) (struct snd_ac97 *ac97);
	
	struct snd_ac97_bus *bus;
	struct pci_dev *pci;	
	struct snd_info_entry *proc;
	struct snd_info_entry *proc_regs;
	unsigned short subsystem_vendor;
	unsigned short subsystem_device;
	struct mutex reg_mutex;
	struct mutex page_mutex;	
	unsigned short num;	
	unsigned short addr;	
	unsigned int id;	
	unsigned short caps;	
	unsigned short ext_id;	
	unsigned short ext_mid;	
	const struct snd_ac97_res_table *res_table;	
	unsigned int scaps;	
	unsigned int flags;	
	unsigned int rates[6];	
	unsigned int spdif_status;
	unsigned short regs[0x80]; 
	DECLARE_BITMAP(reg_accessed, 0x80); 
	union {			
		struct {
			unsigned short unchained[3];	
			unsigned short chained[3];	
			unsigned short id[3];		
			unsigned short pcmreg[3];	
			unsigned short codec_cfg[3];	
			unsigned char swap_mic_linein;	
			unsigned char lo_as_master;	
		} ad18xx;
		unsigned int dev_flags;		
	} spec;
	
	unsigned char indep_surround;
	unsigned char channel_mode;

#ifdef CONFIG_SND_AC97_POWER_SAVE
	unsigned int power_up;	
	struct delayed_work power_work;
#endif
	struct device dev;
};

#define to_ac97_t(d) container_of(d, struct snd_ac97, dev)

static inline int ac97_is_audio(struct snd_ac97 * ac97)
{
	return (ac97->scaps & AC97_SCAP_AUDIO);
}
static inline int ac97_is_modem(struct snd_ac97 * ac97)
{
	return (ac97->scaps & AC97_SCAP_MODEM);
}
static inline int ac97_is_rev22(struct snd_ac97 * ac97)
{
	return (ac97->ext_id & AC97_EI_REV_MASK) >= AC97_EI_REV_22;
}
static inline int ac97_can_amap(struct snd_ac97 * ac97)
{
	return (ac97->ext_id & AC97_EI_AMAP) != 0;
}
static inline int ac97_can_spdif(struct snd_ac97 * ac97)
{
	return (ac97->ext_id & AC97_EI_SPDIF) != 0;
}

int snd_ac97_bus(struct snd_card *card, int num, struct snd_ac97_bus_ops *ops,
		 void *private_data, struct snd_ac97_bus **rbus);
int snd_ac97_mixer(struct snd_ac97_bus *bus, struct snd_ac97_template *template,
		   struct snd_ac97 **rac97);
const char *snd_ac97_get_short_name(struct snd_ac97 *ac97);

void snd_ac97_write(struct snd_ac97 *ac97, unsigned short reg, unsigned short value);
unsigned short snd_ac97_read(struct snd_ac97 *ac97, unsigned short reg);
void snd_ac97_write_cache(struct snd_ac97 *ac97, unsigned short reg, unsigned short value);
int snd_ac97_update(struct snd_ac97 *ac97, unsigned short reg, unsigned short value);
int snd_ac97_update_bits(struct snd_ac97 *ac97, unsigned short reg, unsigned short mask, unsigned short value);
#ifdef CONFIG_SND_AC97_POWER_SAVE
int snd_ac97_update_power(struct snd_ac97 *ac97, int reg, int powerup);
#else
static inline int snd_ac97_update_power(struct snd_ac97 *ac97, int reg,
					int powerup)
{
	return 0;
}
#endif
#ifdef CONFIG_PM
void snd_ac97_suspend(struct snd_ac97 *ac97);
void snd_ac97_resume(struct snd_ac97 *ac97);
#endif

enum {
	AC97_TUNE_DEFAULT = -1,	
	AC97_TUNE_NONE = 0,	
	AC97_TUNE_HP_ONLY,	
	AC97_TUNE_SWAP_HP,	
	AC97_TUNE_SWAP_SURROUND, 
	AC97_TUNE_AD_SHARING,	
	AC97_TUNE_ALC_JACK,	
	AC97_TUNE_INV_EAPD,	
	AC97_TUNE_MUTE_LED,	
	AC97_TUNE_HP_MUTE_LED,  
};

struct ac97_quirk {
	unsigned short subvendor; 
	unsigned short subdevice; 
	unsigned short mask;	
	unsigned int codec_id;	
	const char *name;	
	int type;		
};

int snd_ac97_tune_hardware(struct snd_ac97 *ac97, struct ac97_quirk *quirk, const char *override);
int snd_ac97_set_rate(struct snd_ac97 *ac97, int reg, unsigned int rate);


enum ac97_pcm_cfg {
	AC97_PCM_CFG_FRONT = 2,
	AC97_PCM_CFG_REAR = 10,		
	AC97_PCM_CFG_LFE = 11,		
	AC97_PCM_CFG_40 = 4,		
	AC97_PCM_CFG_51 = 6,		
	AC97_PCM_CFG_SPDIF = 20
};

struct ac97_pcm {
	struct snd_ac97_bus *bus;
	unsigned int stream: 1,	   	   
		     exclusive: 1,	   
		     copy_flag: 1,	   
		     spdif: 1;		   
	unsigned short aslots;		   
	unsigned short cur_dbl;		   
	unsigned int rates;		   
	struct {
		unsigned short slots;	   
		unsigned short rslots[4];  
		unsigned char rate_table[4];
		struct snd_ac97 *codec[4];	   
	} r[2];				   
	unsigned long private_value;	   
};

int snd_ac97_pcm_assign(struct snd_ac97_bus *ac97,
			unsigned short pcms_count,
			const struct ac97_pcm *pcms);
int snd_ac97_pcm_open(struct ac97_pcm *pcm, unsigned int rate,
		      enum ac97_pcm_cfg cfg, unsigned short slots);
int snd_ac97_pcm_close(struct ac97_pcm *pcm);
int snd_ac97_pcm_double_rate_rules(struct snd_pcm_runtime *runtime);

extern struct bus_type ac97_bus_type;

static inline void snd_ac97_dev_add_pdata(struct snd_ac97 *ac97, void *data)
{
	ac97->dev.platform_data = data;
}

#endif 
