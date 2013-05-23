/* include/linux/a1028.h - a1028 voice processor driver
 *
 * Copyright (C) 2009 HTC Corporation.
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

#ifndef __LINUX_A1028_H
#define __LINUX_A1028_H

#include <linux/ioctl.h>

#define A1028_I2C_NAME "audience_a1028"
#define A1028_MAX_FW_SIZE	(32*1024)
struct a1028img {
	unsigned char *buf;
	unsigned img_size;
};

enum A1028_PathID {
	A1028_PATH_SUSPEND,
	A1028_PATH_INCALL_RECEIVER,
	A1028_PATH_INCALL_HEADSET,
	A1028_PATH_INCALL_SPEAKER,
	A1028_PATH_INCALL_BT,
	A1028_PATH_VR_NO_NS_RECEIVER,
	A1028_PATH_VR_NO_NS_HEADSET,
	A1028_PATH_VR_NO_NS_SPEAKER,
	A1028_PATH_VR_NO_NS_BT,
	A1028_PATH_VR_NS_RECEIVER,
	A1028_PATH_VR_NS_HEADSET,
	A1028_PATH_VR_NS_SPEAKER,
	A1028_PATH_VR_NS_BT,
	A1028_PATH_RECORD_RECEIVER,
	A1028_PATH_RECORD_HEADSET,
	A1028_PATH_RECORD_SPEAKER,
	A1028_PATH_RECORD_BT,
	A1028_PATH_CAMCORDER,
	A1028_PATH_INCALL_TTY,
	A1028_PATH_MFG_LOOPBACK,
	A1028_PATH_MAX
};

struct A1028_config_data {
	unsigned int data_len;
	unsigned int mode_num;
	unsigned char *cmd_data;  
};

enum A1028_NS_states {
	A1028_NS_STATE_AUTO,	
	A1028_NS_STATE_OFF,	
	A1028_NS_STATE_CT,	
	A1028_NS_STATE_FT,	
	A1028_NS_NUM_STATES
};

#define A1028_IOCTL_MAGIC 'u'

#define A1028_BOOTUP_INIT  _IOW(A1028_IOCTL_MAGIC, 0x01, struct a1028img *)
#define A1028_SET_CONFIG   _IOW(A1028_IOCTL_MAGIC, 0x02, enum A1028_PathID)
#define A1028_SET_NS_STATE _IOW(A1028_IOCTL_MAGIC, 0x03, enum A1028_NS_states)
#define A1028_SET_PARAM	   _IOW(A1028_IOCTL_MAGIC, 0x04,  unsigned)
#define A1028_SET_MIC_ONOFF	_IOW(A1028_IOCTL_MAGIC, 0x50, unsigned)
#define A1028_SET_MICSEL_ONOFF	_IOW(A1028_IOCTL_MAGIC, 0x51, unsigned)
#define A1028_READ_DATA		_IOR(A1028_IOCTL_MAGIC, 0x52, unsigned)
#define A1028_WRITE_MSG		_IOW(A1028_IOCTL_MAGIC, 0x53, unsigned)
#define A1028_SYNC_CMD		_IO(A1028_IOCTL_MAGIC, 0x54)
#define A1028_SET_CMD_FILE	_IOW(A1028_IOCTL_MAGIC, 0x55, unsigned)

#ifdef __KERNEL__

#define CtrlMode_LAL		0x0001 
#define CtrlMode_LAH		0x0002 
#define CtrlMode_FE		0x0003 
#define CtrlMode_RE		0x0004 
#define A100_msg_Sync		0x80000000
#define A100_msg_Sync_Ack	0x80000000

#define A100_msg_Reset		0x8002
#define RESET_IMMEDIATE		0x0000
#define RESET_DELAYED		0x0001

#define A100_msg_BootloadInitiate	0x8003
#define A100_msg_GetDeviceParm		0x800B
#define A100_msg_SetDeviceParmID	0x800C
#define A100_msg_SetDeviceParm		0x800D

#define PCM0WordLength		0x0100
#define PCM0DelFromFsTx		0x0101
#define PCM0DelFromFsRx		0x0102
#define PCM0LatchEdge		0x0103
#define PCM0Endianness		0x0105
#define PCM0TristateEnable	0x0107

#define PCM1WordLength		0x0200
#define PCM1DelFromFsTx		0x0201
#define PCM1DelFromFsRx		0x0202
#define PCM1LatchEdge		0x0203
#define PCM1Endianness		0x0205
#define PCM1TristateEnable	0x0207

#define PCMWordLength_16bit	0x10 
#define PCMWordLength_24bit	0x18
#define PCMWordLength_32bit	0x20
#define PCMLatchEdge_Tx_F_Rx_R	0x00 
#define PCMLatchEdge_Tx_R_Rx_F	0x03 
#define PCMEndianness_Little	0x00
#define PCMEndianness_Big	0x01 
#define PCMTristate_Disable	0x00 
#define PCMTristate_Enable	0x01

#define ADC0Gain	0x0300
#define ADC0Rate	0x0301
#define ADC0CutoffFreq	0x0302

#define ADC1Gain	0x0400
#define ADC1Rate	0x0401
#define ADC1CutoffFreq	0x0402

#define ADC_Gain_0db			0x00
#define ADC_Gain_6db			0x01
#define ADC_Gain_12db			0x02
#define ADC_Gain_18db			0x03
#define ADC_Gain_24db			0x04 
#define ADC_Gain_30db			0x05
#define ADC_Rate_8kHz			0x00 
#define ADC_Rate_16kHz			0x01
#define ADC_CutoffFreq_NO_DC_Filter	0x00
#define ADC_CutoffFreq_59p68Hz		0x01 
#define ADC_CutoffFreq_7p46Hz		0x02
#define ADC_CutoffFreq_3p73Hz		0x03

#define A100_msg_Sleep		0x80100001

#define A100_msg_GetAlgorithmParm	0x8016
#define A100_msg_SetAlgorithmParmID	0x8017
#define A100_msg_SetAlgorithmParm	0x8018

#define AIS_Global_Supression_Level	0x0000
#define Mic_Config			0x0002
#define AEC_Mode			0x0003
#define AEC_CNG				0x0023
#define Output_AGC			0x0004
#define Output_AGC_Target_Level		0x0005
#define Output_AGC_Noise_Floor		0x0006
#define Output_AGC_SNR_Improvement	0x0007
#define Comfort_Noise			0x001A
#define Comfort_Noise_Level		0x001B

#define Speaker_Volume			0x0012
#define VEQ_Mode			0x0009
#define VEQ_Max_FarEnd_Limiter_Level	0x000D
#define VEQ_Noise_Estimation_Adj	0x0025
#define Receive_NS			0x000E
#define Receive_NS_Level		0x000F
#define SideTone			0x0015
#define SideTone_Gain			0x0016

#define A100_msg_GetTxDigitalInputGain  0x801A
#define A100_msg_SetTxDigitalInputGain  0x801B

#define A100_msg_GetRcvDigitalInputGain 0x8022
#define A100_msg_SetRcvDigitalInputGain 0x8023

#define A100_msg_GetTxDigitalOutputGain 0x801D
#define A100_msg_SetTxDigitalOutputGain 0x8015

#define A100_msg_Bypass		0x801C 
#define A1028_msg_VP_ON		0x801C0001
#define A1028_msg_VP_OFF	0x801C0000

#define A100_msg_GetMicRMS	0x8013
#define A100_msg_GetMicPeak	0x8014
#define DiagPath_Pri_Input_Mic	0x0000
#define DiagPath_Sec_Input_Mic	0x0001
#define DiagPath_Output_Mic	0x0002
#define DiagPath_Far_End_Input	0x0003
#define DiagPath_Far_End_Output	0x0004
#define A100_msg_SwapInputCh	0x8019
#define A100_msg_OutputKnownSig	0x801E

#define A1028_msg_BOOT		0x0001
#define A1028_msg_BOOT_ACK	0x01

#define TIMEOUT			20 
#define RETRY_CNT		5
#define POLLING_RETRY_CNT	3
#define A1028_ERROR_CODE	0xffff
#define A1028_SLEEP		0
#define A1028_ACTIVE		1
#define A1028_CMD_FIFO_DEPTH	64

enum A1028_config_mode {
	A1028_CONFIG_FULL,
	A1028_CONFIG_VP
};

struct a1028_platform_data {
	uint32_t gpio_a1028_micsel;
	uint32_t gpio_a1028_wakeup;
	uint32_t gpio_a1028_reset;
	uint32_t gpio_a1028_int;
	uint32_t gpio_a1028_clk;
	uint32_t gpio_a1028_micswitch;
};


#endif 
#endif 
