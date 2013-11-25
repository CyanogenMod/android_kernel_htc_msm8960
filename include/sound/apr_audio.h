/*
 *
 * Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
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

#ifndef _APR_AUDIO_H_
#define _APR_AUDIO_H_

#include <mach/qdsp6v2/apr.h>


#define PRIMARY_I2S_RX 0		
#define PRIMARY_I2S_TX 1		
#define PCM_RX 2			
#define PCM_TX 3			
#define SECONDARY_I2S_RX 4		
#define SECONDARY_I2S_TX 5		
#define MI2S_RX 6			
#define MI2S_TX 7			
#define HDMI_RX 8			
#define RSVD_2 9			
#define RSVD_3 10			
#define DIGI_MIC_TX 11			
#define VOICE_RECORD_RX 0x8003		
#define VOICE_RECORD_TX 0x8004		
#define VOICE_PLAYBACK_TX 0x8005	

#define SLIMBUS_0_RX		0x4000		
#define SLIMBUS_0_TX		0x4001		
#define SLIMBUS_1_RX		0x4002		
#define SLIMBUS_1_TX		0x4003		
#define SLIMBUS_2_RX		0x4004
#define SLIMBUS_2_TX		0x4005
#define SLIMBUS_3_RX		0x4006
#define SLIMBUS_3_TX		0x4007
#define SLIMBUS_4_RX		0x4008
#define SLIMBUS_4_TX		0x4009		

#define INT_BT_SCO_RX 0x3000		
#define INT_BT_SCO_TX 0x3001		
#define INT_BT_A2DP_RX 0x3002		
#define INT_FM_RX 0x3004		
#define INT_FM_TX 0x3005		
#define RT_PROXY_PORT_001_RX	0x2000    
#define RT_PROXY_PORT_001_TX	0x2001    
#define SECONDARY_PCM_RX 12			
#define SECONDARY_PCM_TX 13			


#define AFE_PORT_INVALID 0xFFFF
#define SLIMBUS_EXTPROC_RX AFE_PORT_INVALID

#define AFE_PORT_CMD_START 0x000100ca

#define AFE_EVENT_RTPORT_START 0
#define AFE_EVENT_RTPORT_STOP 1
#define AFE_EVENT_RTPORT_LOW_WM 2
#define AFE_EVENT_RTPORT_HI_WM 3

struct afe_port_start_command {
	struct apr_hdr hdr;
	u16 port_id;
	u16 gain;		
	u32 sample_rate;	
} __attribute__ ((packed));

#define AFE_PORT_CMD_STOP 0x000100cb
struct afe_port_stop_command {
	struct apr_hdr hdr;
	u16 port_id;
	u16 reserved;
} __attribute__ ((packed));

#define AFE_PORT_CMD_APPLY_GAIN 0x000100cc
struct afe_port_gain_command {
	struct apr_hdr hdr;
	u16 port_id;
	u16	gain;
} __attribute__ ((packed));

#define AFE_PORT_CMD_SIDETONE_CTL 0x000100cd
struct afe_port_sidetone_command {
	struct apr_hdr hdr;
	u16 rx_port_id;		
				
				
				
				
	u16 tx_port_id;		
				
				
				
				
	u16 gain;		
	u16 enable;		
} __attribute__ ((packed));

#define AFE_PORT_CMD_LOOPBACK 0x000100ce
struct afe_loopback_command {
	struct apr_hdr hdr;
	u16 tx_port_id;		
				
				
				
				
	u16 rx_port_id;		
				
				
				
				
	u16 mode;		
	u16 enable;		
} __attribute__ ((packed));

#define AFE_PSEUDOPORT_CMD_START 0x000100cf
struct afe_pseudoport_start_command {
	struct apr_hdr hdr;
	u16 port_id;		
				
				
	u16 timing;		
} __attribute__ ((packed));

#define AFE_PSEUDOPORT_CMD_STOP 0x000100d0
struct afe_pseudoport_stop_command {
	struct apr_hdr hdr;
	u16 port_id;		
				
				
	u16 reserved;
} __attribute__ ((packed));

#define AFE_CMD_GET_ACTIVE_PORTS 0x000100d1


#define AFE_CMD_GET_ACTIVE_HANDLES_FOR_PORT 0x000100d2
struct afe_get_active_handles_command {
	struct apr_hdr hdr;
	u16 port_id;
	u16 reserved;
} __attribute__ ((packed));

#define AFE_PCM_CFG_MODE_PCM			0x0
#define AFE_PCM_CFG_MODE_AUX			0x1
#define AFE_PCM_CFG_SYNC_EXT			0x0
#define AFE_PCM_CFG_SYNC_INT			0x1
#define AFE_PCM_CFG_FRM_8BPF			0x0
#define AFE_PCM_CFG_FRM_16BPF			0x1
#define AFE_PCM_CFG_FRM_32BPF			0x2
#define AFE_PCM_CFG_FRM_64BPF			0x3
#define AFE_PCM_CFG_FRM_128BPF			0x4
#define AFE_PCM_CFG_FRM_256BPF			0x5
#define AFE_PCM_CFG_QUANT_ALAW_NOPAD		0x0
#define AFE_PCM_CFG_QUANT_MULAW_NOPAD		0x1
#define AFE_PCM_CFG_QUANT_LINEAR_NOPAD		0x2
#define AFE_PCM_CFG_QUANT_ALAW_PAD		0x3
#define AFE_PCM_CFG_QUANT_MULAW_PAD		0x4
#define AFE_PCM_CFG_QUANT_LINEAR_PAD		0x5
#define AFE_PCM_CFG_CDATAOE_MASTER		0x0
#define AFE_PCM_CFG_CDATAOE_SHARE		0x1

struct afe_port_pcm_cfg {
	u16	mode;	
	u16	sync;	
	u16	frame;	
			
			
			
			
			
	u16     quant;
	u16	slot;	
	u16	data;	
			
			
	u16	reserved;
} __attribute__ ((packed));

enum {
	AFE_I2S_SD0 = 1,
	AFE_I2S_SD1,
	AFE_I2S_SD2,
	AFE_I2S_SD3,
	AFE_I2S_QUAD01,
	AFE_I2S_QUAD23,
	AFE_I2S_6CHS,
	AFE_I2S_8CHS,
};

#define AFE_MI2S_MONO 0
#define AFE_MI2S_STEREO 3
#define AFE_MI2S_4CHANNELS 4
#define AFE_MI2S_6CHANNELS 6
#define AFE_MI2S_8CHANNELS 8

struct afe_port_mi2s_cfg {
	u16	bitwidth;	
	u16	line;		
				
				
				
				
				
				
				
				
	u16	channel;	
				
				
				
				
	u16	ws;		
				
	u16	format;	
				
} __attribute__ ((packed));

struct afe_port_hdmi_cfg {
	u16	bitwidth;	
	u16	channel_mode;	
				
				
				
	u16	data_type;	
				
} __attribute__ ((packed));


struct afe_port_hdmi_multi_ch_cfg {
	u16	data_type;		
					
	u16	channel_allocation;	
	u16	reserved;		
} __packed;


#define AFE_SLIMBUS_DEVICE_1		0x0
#define AFE_SLIMBUS_DEVICE_2		0x1
#define AFE_PORT_MAX_AUDIO_CHAN_CNT	16

struct afe_port_slimbus_cfg {
	u16	slimbus_dev_id;		

	u16	slave_dev_pgd_la;	
	u16	slave_dev_intfdev_la;	
	u16	bit_width;		

	u16	data_format;		

	u16	num_channels;		

	
	u16	slave_port_mapping[AFE_PORT_MAX_AUDIO_CHAN_CNT];

	u16	reserved;
} __packed;

struct afe_port_slimbus_sch_cfg {
	u16	slimbus_dev_id;		
	u16	bit_width;		
	u16	data_format;		
	u16	num_channels;		
	u16	reserved;
	
	u8	slave_ch_mapping[8];
} __packed;

struct afe_port_rtproxy_cfg {
	u16	bitwidth;	
	u16	interleaved;    
				
	u16	frame_sz;	
	u16	jitter;		
	u16	lw_mark;	
	u16	hw_mark;	
	u16	rsvd;
	int	num_ch;		
} __packed;

#define AFE_PORT_AUDIO_IF_CONFIG 0x000100d3
#define AFE_PORT_AUDIO_SLIM_SCH_CONFIG 0x000100e4
#define AFE_PORT_MULTI_CHAN_HDMI_AUDIO_IF_CONFIG	0x000100D9
#define AFE_PORT_CMD_I2S_CONFIG	0x000100E7

union afe_port_config {
	struct afe_port_pcm_cfg           pcm;
	struct afe_port_mi2s_cfg          mi2s;
	struct afe_port_hdmi_cfg          hdmi;
	struct afe_port_hdmi_multi_ch_cfg hdmi_multi_ch;
	struct afe_port_slimbus_cfg	  slimbus;
	struct afe_port_slimbus_sch_cfg	  slim_sch;
	struct afe_port_rtproxy_cfg       rtproxy;
} __attribute__((packed));

struct afe_audioif_config_command {
	struct apr_hdr hdr;
	u16 port_id;
	union afe_port_config port;
} __attribute__ ((packed));

#define AFE_TEST_CODEC_LOOPBACK_CTL 0x000100d5
struct afe_codec_loopback_command {
	u16	port_inf;	
				
				
				
	u16	enable;		
} __attribute__ ((packed));


#define AFE_PARAM_ID_SIDETONE_GAIN	0x00010300
struct afe_param_sidetone_gain {
	u16 gain;
	u16 reserved;
} __attribute__ ((packed));

#define AFE_PARAM_ID_SAMPLING_RATE	0x00010301
struct afe_param_sampling_rate {
	u32 sampling_rate;
} __attribute__ ((packed));


#define AFE_PARAM_ID_CHANNELS		0x00010302
struct afe_param_channels {
	u16 channels;
	u16 reserved;
} __attribute__ ((packed));


#define AFE_PARAM_ID_LOOPBACK_GAIN	0x00010303
struct afe_param_loopback_gain {
	u16 gain;
	u16 reserved;
} __attribute__ ((packed));

enum {
	
	LB_MODE_DEFAULT = 1,
	
	LB_MODE_SIDETONE,
	
	LB_MODE_EC_REF_VOICE_AUDIO,
	
	LB_MODE_EC_REF_VOICE
};

#define AFE_PARAM_ID_LOOPBACK_CONFIG 0x0001020B
#define AFE_API_VERSION_LOOPBACK_CONFIG 0x1
struct afe_param_loopback_cfg {
	uint32_t loopback_cfg_minor_version;

	
	uint16_t dst_port_id;

	uint16_t routing_mode;

	
	uint16_t enable;

	
	uint16_t reserved;
} __packed;

#define AFE_MODULE_ID_PORT_INFO		0x00010200
#define AFE_MODULE_LOOPBACK           0x00010205
struct afe_param_payload_base {
	u32 module_id;
	u32 param_id;
	u16 param_size;
	u16 reserved;
} __packed;

struct afe_param_payload {
	struct afe_param_payload_base base;
	union {
		struct afe_param_sidetone_gain sidetone_gain;
		struct afe_param_sampling_rate sampling_rate;
		struct afe_param_channels      channels;
		struct afe_param_loopback_gain loopback_gain;
		struct afe_param_loopback_cfg loopback_cfg;
	} __attribute__((packed)) param;
} __attribute__ ((packed));

#define AFE_PORT_CMD_SET_PARAM		0x000100dc

struct afe_port_cmd_set_param {
	struct apr_hdr hdr;
	u16 port_id;
	u16 payload_size;
	u32 payload_address;
	struct afe_param_payload payload;
} __attribute__ ((packed));

struct afe_port_cmd_set_param_no_payload {
	struct apr_hdr hdr;
	u16 port_id;
	u16 payload_size;
	u32 payload_address;
} __packed;

#define AFE_EVENT_GET_ACTIVE_PORTS 0x00010100
struct afe_get_active_ports_rsp {
	u16	num_ports;
	u16	port_id;
} __attribute__ ((packed));


#define AFE_EVENT_GET_ACTIVE_HANDLES 0x00010102
struct afe_get_active_handles_rsp {
	u16	port_id;
	u16	num_handles;
	u16	mode;		
				
				
				
	u16	handle;
} __attribute__ ((packed));

#define AFE_SERVICE_CMD_MEMORY_MAP 0x000100DE
struct afe_cmd_memory_map {
	struct apr_hdr hdr;
	u32 phy_addr;
	u32 mem_sz;
	u16 mem_id;
	u16 rsvd;
} __packed;

#define AFE_SERVICE_CMD_MEMORY_UNMAP 0x000100DF
struct afe_cmd_memory_unmap {
	struct apr_hdr hdr;
	u32 phy_addr;
} __packed;

#define AFE_SERVICE_CMD_REG_RTPORT 0x000100E0
struct afe_cmd_reg_rtport {
	struct apr_hdr hdr;
	u16 port_id;
	u16 rsvd;
} __packed;

#define AFE_SERVICE_CMD_UNREG_RTPORT 0x000100E1
struct afe_cmd_unreg_rtport {
	struct apr_hdr hdr;
	u16 port_id;
	u16 rsvd;
} __packed;

#define AFE_SERVICE_CMD_RTPORT_WR 0x000100E2
struct afe_cmd_rtport_wr {
	struct apr_hdr hdr;
	u16 port_id;
	u16 rsvd;
	u32 buf_addr;
	u32 bytes_avail;
} __packed;

#define AFE_SERVICE_CMD_RTPORT_RD 0x000100E3
struct afe_cmd_rtport_rd {
	struct apr_hdr hdr;
	u16 port_id;
	u16 rsvd;
	u32 buf_addr;
	u32 bytes_avail;
} __packed;

#define AFE_EVENT_RT_PROXY_PORT_STATUS 0x00010105

#define ADM_MAX_COPPS 5

#define ADM_SERVICE_CMD_GET_COPP_HANDLES                 0x00010300
struct adm_get_copp_handles_command {
	struct apr_hdr hdr;
} __attribute__ ((packed));

#define ADM_CMD_MATRIX_MAP_ROUTINGS                      0x00010301
struct adm_routings_session {
	u16 id;
	u16 num_copps;
	u16 copp_id[ADM_MAX_COPPS+1]; 
} __packed;

struct adm_routings_command {
	struct apr_hdr hdr;
	u32 path; 
	u32 num_sessions;
	struct adm_routings_session session[8];
} __attribute__ ((packed));


#define ADM_CMD_MATRIX_RAMP_GAINS                        0x00010302
struct adm_ramp_gain {
	struct apr_hdr hdr;
	u16 session_id;
	u16 copp_id;
	u16 initial_gain;
	u16 gain_increment;
	u16 ramp_duration;
	u16 reserved;
} __attribute__ ((packed));

struct adm_ramp_gains_command {
	struct apr_hdr hdr;
	u32 id;
	u32 num_gains;
	struct adm_ramp_gain gains[ADM_MAX_COPPS];
} __attribute__ ((packed));


#define ADM_CMD_COPP_OPEN                                0x00010304
struct adm_copp_open_command {
	struct apr_hdr hdr;
	u16 flags;
	u16 mode; 
	u16 endpoint_id1;
	u16 endpoint_id2;
	u32 topology_id;
	u16 channel_config;
	u16 reserved;
	u32 rate;
} __attribute__ ((packed));

#define ADM_CMD_COPP_CLOSE                               0x00010305

#define ADM_CMD_MULTI_CHANNEL_COPP_OPEN                  0x00010310
struct adm_multi_ch_copp_open_command {
	struct apr_hdr hdr;
	u16 flags;
	u16 mode; 
	u16 endpoint_id1;
	u16 endpoint_id2;
	u32 topology_id;
	u16 channel_config;
	u16 reserved;
	u32 rate;
	u8 dev_channel_mapping[8];
} __packed;

#define ADM_CMD_MULTI_CHANNEL_COPP_OPEN_V2		 0x00010319

struct adm_multi_ch_copp_open_command_v2 {
	struct apr_hdr hdr;
	u16 flags;
	u16 mode; 
	u16 endpoint_id1;
	u16 endpoint_id2;
	u32 topology_id;
	u16 channel_config;
	u16 bit_width;
	u32 rate;
	u8 dev_channel_mapping[8];
} __packed;


#define ADM_CMD_MEMORY_MAP				0x00010C30
struct adm_cmd_memory_map{
	struct apr_hdr	hdr;
	u32		buf_add;
	u32		buf_size;
	u16		mempool_id;
	u16		reserved;
} __attribute__((packed));

#define ADM_CMD_MEMORY_UNMAP				0x00010C31
struct adm_cmd_memory_unmap{
	struct apr_hdr	hdr;
	u32		buf_add;
} __attribute__((packed));

#define ADM_CMD_MEMORY_MAP_REGIONS			0x00010C47
struct adm_memory_map_regions{
	u32		phys;
	u32		buf_size;
} __attribute__((packed));

struct adm_cmd_memory_map_regions{
	struct apr_hdr	hdr;
	u16		mempool_id;
	u16		nregions;
} __attribute__((packed));

#define ADM_CMD_MEMORY_UNMAP_REGIONS			0x00010C48
struct adm_memory_unmap_regions{
	u32		phys;
} __attribute__((packed));

struct adm_cmd_memory_unmap_regions{
	struct apr_hdr	hdr;
	u16		nregions;
	u16		reserved;
} __attribute__((packed));

#define DEFAULT_COPP_TOPOLOGY				0x00010be3
#define DEFAULT_POPP_TOPOLOGY				0x00010be4
#define VPM_TX_SM_ECNS_COPP_TOPOLOGY			0x00010F71
#define VPM_TX_DM_FLUENCE_COPP_TOPOLOGY			0x00010F72
#define VPM_TX_QMIC_FLUENCE_COPP_TOPOLOGY		0x00010F75
#define HTC_STEREO_RECORD_TOPOLOGY			0x10000000
#define HTC_COPP_TOPOLOGY				0x10000001
#define HTC_POPP_TOPOLOGY				0x10000002

#define SRS_TRUMEDIA_TOPOLOGY_ID			0x00010D90
#define SRS_TRUMEDIA_MODULE_ID				0x10005010
#define SRS_TRUMEDIA_PARAMS				0x10005011
#define SRS_TRUMEDIA_PARAMS_WOWHD			0x10005012
#define SRS_TRUMEDIA_PARAMS_CSHP			0x10005013
#define SRS_TRUMEDIA_PARAMS_HPF				0x10005014
#define SRS_TRUMEDIA_PARAMS_PEQ				0x10005015
#define SRS_TRUMEDIA_PARAMS_HL				0x10005016

#define ASM_MAX_EQ_BANDS 12

struct asm_eq_band {
	u32 band_idx; 
	u32 filter_type; 
	u32 center_freq_hz; 
	u32 filter_gain; 
			
	u32 q_factor;
} __attribute__ ((packed));

struct asm_equalizer_params {
	u32 enable;
	u32 num_bands;
	struct asm_eq_band eq_bands[ASM_MAX_EQ_BANDS];
} __attribute__ ((packed));

struct asm_master_gain_params {
	u16 master_gain;
	u16 padding;
} __attribute__ ((packed));

struct asm_lrchannel_gain_params {
	u16 left_gain;
	u16 right_gain;
} __attribute__ ((packed));

struct asm_mute_params {
	u32 muteflag;
} __attribute__ ((packed));

struct asm_softvolume_params {
	u32 period;
	u32 step;
	u32 rampingcurve;
} __attribute__ ((packed));

struct asm_softpause_params {
	u32 enable;
	u32 period;
	u32 step;
	u32 rampingcurve;
} __packed;

struct asm_pp_param_data_hdr {
	u32 module_id;
	u32 param_id;
	u16 param_size;
	u16 reserved;
} __attribute__ ((packed));

struct asm_pp_params_command {
	struct apr_hdr	hdr;
	u32    *payload;
	u32	payload_size;
	struct  asm_pp_param_data_hdr params;
} __attribute__ ((packed));

#define EQUALIZER_MODULE_ID		0x00010c27
#define EQUALIZER_PARAM_ID		0x00010c28

#define VOLUME_CONTROL_MODULE_ID	0x00010bfe
#define MASTER_GAIN_PARAM_ID		0x00010bff
#define L_R_CHANNEL_GAIN_PARAM_ID	0x00010c00
#define MUTE_CONFIG_PARAM_ID 0x00010c01
#define SOFT_PAUSE_PARAM_ID 0x00010D6A
#define SOFT_VOLUME_PARAM_ID 0x00010C29

#define IIR_FILTER_ENABLE_PARAM_ID 0x00010c03
#define IIR_FILTER_PREGAIN_PARAM_ID 0x00010c04
#define IIR_FILTER_CONFIG_PARAM_ID 0x00010c05

#define MBADRC_MODULE_ID 0x00010c06
#define MBADRC_ENABLE_PARAM_ID 0x00010c07
#define MBADRC_CONFIG_PARAM_ID 0x00010c08


#define ADM_CMD_SET_PARAMS                               0x00010306
#define ADM_CMD_GET_PARAMS                               0x0001030B
#define ADM_CMDRSP_GET_PARAMS                            0x0001030C
struct adm_set_params_command {
	struct apr_hdr		hdr;
	u32			payload;
	u32			payload_size;
} __attribute__ ((packed));


#define ADM_CMD_TAP_COPP_PCM                             0x00010307
struct adm_tap_copp_pcm_command {
	struct apr_hdr hdr;
} __attribute__ ((packed));


#define ADM_SERVICE_CMDRSP_GET_COPP_HANDLES              0x00010308
struct adm_get_copp_handles_respond {
	struct apr_hdr hdr;
	u32 handles;
	u32 copp_id;
} __attribute__ ((packed));

#define ADM_CMDRSP_COPP_OPEN                             0x0001030A
struct adm_copp_open_respond {
	u32 status;
	u16 copp_id;
	u16 reserved;
} __attribute__ ((packed));

#define ADM_CMDRSP_MULTI_CHANNEL_COPP_OPEN               0x00010311
#define ADM_CMDRSP_MULTI_CHANNEL_COPP_OPEN_V2            0x0001031A


#define ASM_STREAM_PRIORITY_NORMAL	0
#define ASM_STREAM_PRIORITY_LOW		1
#define ASM_STREAM_PRIORITY_HIGH	2
#define ASM_STREAM_PRIORITY_RESERVED	3

#define ASM_END_POINT_DEVICE_MATRIX	0
#define ASM_END_POINT_STREAM		1

#define AAC_ENC_MODE_AAC_LC            0x02
#define AAC_ENC_MODE_AAC_P             0x05
#define AAC_ENC_MODE_EAAC_P            0x1D

#define ASM_STREAM_CMD_CLOSE                             0x00010BCD
#define ASM_STREAM_CMD_FLUSH                             0x00010BCE
#define ASM_STREAM_CMD_SET_PP_PARAMS                     0x00010BCF
#define ASM_STREAM_CMD_GET_PP_PARAMS                     0x00010BD0
#define ASM_STREAM_CMDRSP_GET_PP_PARAMS                  0x00010BD1
#define ASM_SESSION_CMD_PAUSE                            0x00010BD3
#define ASM_SESSION_CMD_GET_SESSION_TIME                 0x00010BD4
#define ASM_DATA_CMD_EOS                                 0x00010BDB
#define ASM_DATA_EVENT_EOS                               0x00010BDD

#define ASM_SERVICE_CMD_GET_STREAM_HANDLES               0x00010C0B
#define ASM_STREAM_CMD_FLUSH_READBUFS                    0x00010C09

#define ASM_SESSION_EVENT_RX_UNDERFLOW			 0x00010C17
#define ASM_SESSION_EVENT_TX_OVERFLOW			 0x00010C18
#define ASM_SERVICE_CMD_GET_WALLCLOCK_TIME               0x00010C19
#define ASM_DATA_CMDRSP_EOS                              0x00010C1C


struct asm_pcm_cfg {
	u16 ch_cfg;
	u16 bits_per_sample;
	u32 sample_rate;
	u16 is_signed;
	u16 interleaved;
};

#define PCM_CHANNEL_NULL 0

#define PCM_CHANNEL_FL    1

#define PCM_CHANNEL_FR    2

#define PCM_CHANNEL_FC    3

#define PCM_CHANNEL_LS   4

#define PCM_CHANNEL_RS   5

#define PCM_CHANNEL_LFE  6

#define PCM_CHANNEL_CS   7

#define PCM_CHANNEL_LB   8

#define PCM_CHANNEL_RB   9

#define PCM_CHANNEL_TS   10

#define PCM_CHANNEL_CVH  11

#define PCM_CHANNEL_MS   12

#define PCM_CHANNEL_FLC  13

#define PCM_CHANNEL_FRC  14

#define PCM_CHANNEL_RLC  15

#define PCM_CHANNEL_RRC  16

#define PCM_FORMAT_MAX_NUM_CHANNEL  8

#define MAX_CHAN_MAP_CHANNELS 16
struct asm_multi_channel_pcm_fmt_blk {

	u16 num_channels;	

	u16 bits_per_sample;	

	u32 sample_rate;	

	u16 is_signed;		

	u16 is_interleaved;	

	u8 channel_mapping[8];	
};

struct asm_adpcm_cfg {
	u16 ch_cfg;
	u16 bits_per_sample;
	u32 sample_rate;
	u32 block_size;
};

struct asm_yadpcm_cfg {
	u16 ch_cfg;
	u16 bits_per_sample;
	u32 sample_rate;
};

struct asm_midi_cfg {
	u32 nMode;
};

struct asm_wma_cfg {
	u16 format_tag;
	u16 ch_cfg;
	u32 sample_rate;
	u32 avg_bytes_per_sec;
	u16 block_align;
	u16 valid_bits_per_sample;
	u32 ch_mask;
	u16 encode_opt;
	u16 adv_encode_opt;
	u32 adv_encode_opt2;
	u32 drc_peak_ref;
	u32 drc_peak_target;
	u32 drc_ave_ref;
	u32 drc_ave_target;
};

struct asm_wmapro_cfg {
	u16 format_tag;
	u16 ch_cfg;
	u32 sample_rate;
	u32 avg_bytes_per_sec;
	u16 block_align;
	u16 valid_bits_per_sample;
	u32 ch_mask;
	u16 encode_opt;
	u16 adv_encode_opt;
	u32 adv_encode_opt2;
	u32 drc_peak_ref;
	u32 drc_peak_target;
	u32 drc_ave_ref;
	u32 drc_ave_target;
};

struct asm_aac_cfg {
	u16 format;
	u16 aot;
	u16 ep_config;
	u16 section_data_resilience;
	u16 scalefactor_data_resilience;
	u16 spectral_data_resilience;
	u16 ch_cfg;
	u16 reserved;
	u32 sample_rate;
};

struct asm_flac_cfg {
	u16 stream_info_present;
	u16 min_blk_size;
	u16 max_blk_size;
	u16 ch_cfg;
	u16 sample_size;
	u16 sample_rate;
	u16 md5_sum;
	u32 ext_sample_rate;
	u32 min_frame_size;
	u32 max_frame_size;
};

struct asm_vorbis_cfg {
	u32 ch_cfg;
	u32 bit_rate;
	u32 min_bit_rate;
	u32 max_bit_rate;
	u16 bit_depth_pcm_sample;
	u16 bit_stream_format;
};

struct asm_aac_read_cfg {
	u32 bitrate;
	u32 enc_mode;
	u16 format;
	u16 ch_cfg;
	u32 sample_rate;
};

struct asm_amrnb_read_cfg {
	u16 mode;
	u16 dtx_mode;
};

struct asm_amrwb_read_cfg {
	u16 mode;
	u16 dtx_mode;
};

struct asm_evrc_read_cfg {
	u16 max_rate;
	u16 min_rate;
	u16 rate_modulation_cmd;
	u16 reserved;
};

struct asm_qcelp13_read_cfg {
	u16 max_rate;
	u16 min_rate;
	u16 reduced_rate_level;
	u16 rate_modulation_cmd;
};

struct asm_sbc_read_cfg {
	u32 subband;
	u32 block_len;
	u32 ch_mode;
	u32 alloc_method;
	u32 bit_rate;
	u32 sample_rate;
};

struct asm_sbc_bitrate {
	u32 bitrate;
};

struct asm_immed_decode {
	u32 mode;
};

struct asm_sbr_ps {
	u32 enable;
};

struct asm_dual_mono {
	u16 sce_left;
	u16 sce_right;
};

struct asm_dec_chan_map {
	u32 num_channels;			  

	u8 channel_mapping[MAX_CHAN_MAP_CHANNELS];
};

struct asm_encode_cfg_blk {
	u32 frames_per_buf;
	u32 format_id;
	u32 cfg_size;
	union {
		struct asm_pcm_cfg          pcm;
		struct asm_aac_read_cfg     aac;
		struct asm_amrnb_read_cfg   amrnb;
		struct asm_evrc_read_cfg    evrc;
		struct asm_qcelp13_read_cfg qcelp13;
		struct asm_sbc_read_cfg     sbc;
		struct asm_amrwb_read_cfg   amrwb;
		struct asm_multi_channel_pcm_fmt_blk      mpcm;
	} __attribute__((packed)) cfg;
};

struct asm_frame_meta_info {
	u32 offset_to_frame;
	u32 frame_size;
	u32 encoded_pcm_samples;
	u32 msw_ts;
	u32 lsw_ts;
	u32 nflags;
};

#define ASM_STREAM_CMD_OPEN_READ                         0x00010BCB
struct asm_stream_cmd_open_read {
	struct apr_hdr hdr;
	u32            uMode;
	u32            src_endpoint;
	u32            pre_proc_top;
	u32            format;
} __attribute__((packed));

#define LINEAR_PCM   0x00010BE5
#define DTMF         0x00010BE6
#define ADPCM        0x00010BE7
#define YADPCM       0x00010BE8
#define MP3          0x00010BE9
#define MPEG4_AAC    0x00010BEA
#define AMRNB_FS     0x00010BEB
#define AMRWB_FS     0x00010BEC
#define V13K_FS      0x00010BED
#define EVRC_FS      0x00010BEE
#define EVRCB_FS     0x00010BEF
#define EVRCWB_FS    0x00010BF0
#define MIDI         0x00010BF1
#define SBC          0x00010BF2
#define WMA_V10PRO   0x00010BF3
#define WMA_V9       0x00010BF4
#define AMR_WB_PLUS  0x00010BF5
#define AC3_DECODER  0x00010BF6
#define EAC3_DECODER 0x00010C3C
#define DTS	0x00010D88
#define DTS_LBR	0x00010DBB
#define ATRAC	0x00010D89
#define MAT	0x00010D8A
#define G711_ALAW_FS 0x00010BF7
#define G711_MLAW_FS 0x00010BF8
#define G711_PCM_FS  0x00010BF9
#define MPEG4_MULTI_AAC 0x00010D86
#define US_POINT_EPOS_FORMAT 0x00012310
#define US_RAW_FORMAT        0x0001127C
#define MULTI_CHANNEL_PCM    0x00010C66

#define ASM_ENCDEC_SBCRATE         0x00010C13
#define ASM_ENCDEC_IMMDIATE_DECODE 0x00010C14
#define ASM_ENCDEC_CFG_BLK         0x00010C2C

#define ASM_ENCDEC_SBCRATE         0x00010C13
#define ASM_ENCDEC_IMMDIATE_DECODE 0x00010C14
#define ASM_ENCDEC_CFG_BLK         0x00010C2C

#define ASM_STREAM_CMD_OPEN_READ_COMPRESSED               0x00010D95
struct asm_stream_cmd_open_read_compressed {
	struct apr_hdr hdr;
	u32            uMode;
	u32            frame_per_buf;
} __packed;

#define ASM_STREAM_CMD_OPEN_WRITE                        0x00010BCA
#define ASM_STREAM_CMD_OPEN_WRITE_V2                     0x00010D8F

struct asm_stream_cmd_open_write {
	struct apr_hdr hdr;
	u32            uMode;
	u16            sink_endpoint;
	u16            stream_handle;
	u32            post_proc_top;
	u32            format;
} __attribute__((packed));

struct asm_stream_cmd_open_write_v2 {
	struct apr_hdr hdr;
	u32            uMode;
	u16            sink_endpoint;
	u16            bits_per_sample;
	u32            post_proc_top;
	u32            format;
} __packed;

#define IEC_61937_MASK	0x00000001
#define IEC_60958_MASK	0x00000002

#define ASM_STREAM_CMD_OPEN_WRITE_COMPRESSED	0x00010D84
struct asm_stream_cmd_open_write_compressed {
	struct apr_hdr hdr;
	u32	flags;
	u32	format;
} __packed;

#define ASM_STREAM_CMD_OPEN_READWRITE                    0x00010BCC

struct asm_stream_cmd_open_read_write {
	struct apr_hdr     hdr;
	u32                uMode;
	u32                post_proc_top;
	u32                write_format;
	u32                read_format;
} __attribute__((packed));

#define ADM_CMD_CONNECT_AFE_PORT 0x00010320
#define ADM_CMD_DISCONNECT_AFE_PORT 0x00010321

struct adm_cmd_connect_afe_port {
	struct apr_hdr     hdr;
	u8	mode; 
	u8	session_id; 
	u16	afe_port_id;
} __packed;

#define ADM_CMD_CONNECT_AFE_PORT_V2 0x00010332

struct adm_cmd_connect_afe_port_v2 {
	struct apr_hdr     hdr;
	u8	mode; 
	u8	session_id; 
	u16	afe_port_id;
	u32	num_channels;
	u32	sampleing_rate;
} __packed;

#define ASM_STREAM_CMD_SET_ENCDEC_PARAM                  0x00010C10
#define ASM_STREAM_CMD_GET_ENCDEC_PARAM                  0x00010C11
#define ASM_ENCDEC_CFG_BLK_ID				 0x00010C2C
#define ASM_ENABLE_SBR_PS				 0x00010C63
#define ASM_CONFIGURE_DUAL_MONO			 0x00010C64
struct asm_stream_cmd_encdec_cfg_blk{
	struct apr_hdr              hdr;
	u32                         param_id;
	u32                         param_size;
	struct asm_encode_cfg_blk   enc_blk;
} __attribute__((packed));

struct asm_stream_cmd_encdec_sbc_bitrate{
	struct apr_hdr hdr;
	u32            param_id;
		struct asm_sbc_bitrate      sbc_bitrate;
} __attribute__((packed));

struct asm_stream_cmd_encdec_immed_decode{
	struct apr_hdr hdr;
	u32            param_id;
	u32            param_size;
	struct asm_immed_decode dec;
} __attribute__((packed));

struct asm_stream_cmd_encdec_sbr{
	struct apr_hdr hdr;
	u32            param_id;
	u32            param_size;
	struct asm_sbr_ps sbr_ps;
} __attribute__((packed));

struct asm_stream_cmd_encdec_dualmono {
	struct apr_hdr hdr;
	u32            param_id;
	u32            param_size;
	struct asm_dual_mono channel_map;
} __packed;

#define ASM_ENCDEC_DEC_CHAN_MAP				 0x00010D82
struct asm_stream_cmd_encdec_channelmap {
	struct apr_hdr hdr;
	u32            param_id;
	u32            param_size;
	struct asm_dec_chan_map chan_map;
} __packed;

#define ASM_STREAM _CMD_ADJUST_SAMPLES                   0x00010C0A
struct asm_stream_cmd_adjust_samples{
	struct apr_hdr hdr;
	u16            nsamples;
	u16            reserved;
} __attribute__((packed));

#define ASM_STREAM_CMD_TAP_POPP_PCM                      0x00010BF9
struct asm_stream_cmd_tap_popp_pcm{
	struct apr_hdr hdr;
	u16            enable;
	u16            reserved;
	u32            module_id;
} __attribute__((packed));

#define ASM_SESSION_CMD_MEMORY_MAP			0x00010C32
struct asm_stream_cmd_memory_map{
	struct apr_hdr	hdr;
	u32		buf_add;
	u32		buf_size;
	u16		mempool_id;
	u16		reserved;
} __attribute__((packed));

#define ASM_SESSION_CMD_MEMORY_UNMAP			0x00010C33
struct asm_stream_cmd_memory_unmap{
	struct apr_hdr	hdr;
	u32		buf_add;
} __attribute__((packed));

#define ASM_SESSION_CMD_MEMORY_MAP_REGIONS		0x00010C45
struct asm_memory_map_regions{
	u32		phys;
	u32		buf_size;
} __attribute__((packed));

struct asm_stream_cmd_memory_map_regions{
	struct apr_hdr	hdr;
	u16		mempool_id;
	u16		nregions;
} __attribute__((packed));

#define ASM_SESSION_CMD_MEMORY_UNMAP_REGIONS		0x00010C46
struct asm_memory_unmap_regions{
	u32		phys;
} __attribute__((packed));

struct asm_stream_cmd_memory_unmap_regions{
	struct apr_hdr	hdr;
	u16		nregions;
	u16		reserved;
} __attribute__((packed));

#define ASM_SESSION_CMD_RUN                              0x00010BD2
struct asm_stream_cmd_run{
	struct apr_hdr hdr;
	u32            flags;
	u32            msw_ts;
	u32            lsw_ts;
} __attribute__((packed));

#define ASM_SESSION_CMD_REGISTER_FOR_RX_UNDERFLOW_EVENTS 0x00010BD5
struct asm_stream_cmd_reg_rx_underflow_event{
	struct apr_hdr hdr;
	u16            enable;
	u16            reserved;
} __attribute__((packed));

#define ASM_SESSION_CMD_REGISTER_FOR_TX_OVERFLOW_EVENTS  0x00010BD6
struct asm_stream_cmd_reg_tx_overflow_event{
	struct apr_hdr hdr;
	u16            enable;
	u16            reserved;
} __attribute__((packed));

#define ASM_DATA_CMD_WRITE                               0x00010BD9
struct asm_stream_cmd_write{
	struct apr_hdr     hdr;
	u32	buf_add;
	u32	avail_bytes;
	u32	uid;
	u32	msw_ts;
	u32	lsw_ts;
	u32	uflags;
} __attribute__((packed));

#define ASM_DATA_CMD_READ                                0x00010BDA
struct asm_stream_cmd_read{
	struct apr_hdr     hdr;
	u32	buf_add;
	u32	buf_size;
	u32	uid;
} __attribute__((packed));

#define ASM_DATA_CMD_MEDIA_FORMAT_UPDATE                 0x00010BDC
#define ASM_DATA_EVENT_ENC_SR_CM_NOTIFY                  0x00010BDE
struct asm_stream_media_format_update{
	struct apr_hdr hdr;
	u32            format;
	u32            cfg_size;
	union {
		struct asm_pcm_cfg         pcm_cfg;
		struct asm_adpcm_cfg       adpcm_cfg;
		struct asm_yadpcm_cfg      yadpcm_cfg;
		struct asm_midi_cfg        midi_cfg;
		struct asm_wma_cfg         wma_cfg;
		struct asm_wmapro_cfg      wmapro_cfg;
		struct asm_aac_cfg         aac_cfg;
		struct asm_flac_cfg        flac_cfg;
		struct asm_vorbis_cfg      vorbis_cfg;
		struct asm_multi_channel_pcm_fmt_blk multi_ch_pcm_cfg;
	} __attribute__((packed)) write_cfg;
} __attribute__((packed));


#define ASM_STREAM_CMDRSP_GET_ENCDEC_PARAM               0x00010C12
struct asm_stream_cmdrsp_get_readwrite_param{
	struct apr_hdr hdr;
	u32            status;
	u32            param_id;
	u16            param_size;
	u16            padding;
	union {
		struct asm_sbc_bitrate      sbc_bitrate;
		struct asm_immed_decode aac_dec;
	} __attribute__((packed)) read_write_cfg;
} __attribute__((packed));


#define ASM_SESSION_CMDRSP_GET_SESSION_TIME              0x00010BD8
struct asm_stream_cmdrsp_get_session_time{
	struct apr_hdr hdr;
	u32            status;
	u32            msw_ts;
	u32            lsw_ts;
} __attribute__((packed));

#define ASM_DATA_EVENT_WRITE_DONE                        0x00010BDF
struct asm_data_event_write_done{
	u32	buf_add;
	u32            status;
} __attribute__((packed));

#define ASM_DATA_EVENT_READ_DONE                         0x00010BE0
struct asm_data_event_read_done{
	u32            status;
	u32            buffer_add;
	u32            enc_frame_size;
	u32            offset;
	u32            msw_ts;
	u32            lsw_ts;
	u32            flags;
	u32            num_frames;
	u32            id;
} __attribute__((packed));

#define ASM_DATA_EVENT_SR_CM_CHANGE_NOTIFY               0x00010C65
struct asm_data_event_sr_cm_change_notify {
	u32            sample_rate;
	u16	           no_of_channels;
	u16            reserved;
	u8             channel_map[8];
} __packed;


#define ASM_SERVICE_CMDRSP_GET_STREAM_HANDLES            0x00010C1B
struct asm_svc_cmdrsp_get_strm_handles{
	struct apr_hdr hdr;
	u32            num_handles;
	u32            stream_handles;
} __attribute__((packed));


#define ASM_SERVICE_CMDRSP_GET_WALLCLOCK_TIME            0x00010C1A
struct asm_svc_cmdrsp_get_wallclock_time{
	struct apr_hdr hdr;
	u32            status;
	u32            msw_ts;
	u32            lsw_ts;
} __attribute__((packed));

#define ADSP_EOK          0x00000000 
#define ADSP_EFAILED      0x00000001 
#define ADSP_EBADPARAM    0x00000002 
#define ADSP_EUNSUPPORTED 0x00000003 
#define ADSP_EVERSION     0x00000004 
#define ADSP_EUNEXPECTED  0x00000005 
#define ADSP_EPANIC       0x00000006 
#define ADSP_ENORESOURCE  0x00000007 
#define ADSP_EHANDLE      0x00000008 
#define ADSP_EALREADY     0x00000009 
#define ADSP_ENOTREADY    0x0000000A 
#define ADSP_EPENDING     0x0000000B 
#define ADSP_EBUSY        0x0000000C 
#define ADSP_EABORTED     0x0000000D 
#define ADSP_EPREEMPTED   0x0000000E 
#define ADSP_ECONTINUE    0x0000000F 
#define ADSP_EIMMEDIATE   0x00000010 
#define ADSP_ENOTIMPL     0x00000011 
#define ADSP_ENEEDMORE    0x00000012 

#define SRS_ID_GLOBAL	0x00000001
#define SRS_ID_WOWHD	0x00000002
#define SRS_ID_CSHP	0x00000003
#define SRS_ID_HPF	0x00000004
#define SRS_ID_PEQ	0x00000005
#define SRS_ID_HL	0x00000006

#define SRS_CMD_UPLOAD		0x7FFF0000
#define SRS_PARAM_INDEX_MASK	0x80000000
#define SRS_PARAM_OFFSET_MASK	0x3FFF0000
#define SRS_PARAM_VALUE_MASK	0x0000FFFF

struct srs_trumedia_params_GLOBAL {
	uint8_t                  v1;
	uint8_t                  v2;
	uint8_t                  v3;
	uint8_t                  v4;
	uint8_t                  v5;
	uint8_t                  v6;
	uint8_t                  v7;
	uint8_t                  v8;
} __packed;

struct srs_trumedia_params_WOWHD {
	uint32_t				v1;
	uint16_t				v2;
	uint16_t				v3;
	uint16_t				v4;
	uint16_t				v5;
	uint16_t				v6;
	uint16_t				v7;
	uint16_t				v8;
	uint16_t				v____A1;
	uint32_t				v9;
	uint16_t				v10;
	uint16_t				v11;
	uint32_t				v12[16];
} __packed;

struct srs_trumedia_params_CSHP {
	uint32_t				v1;
	uint16_t				v2;
	uint16_t				v3;
	uint16_t				v4;
	uint16_t				v5;
	uint16_t				v6;
	uint16_t				v____A1;
	uint32_t				v7;
	uint16_t				v8;
	uint16_t				v9;
	uint32_t				v10[16];
} __packed;

struct srs_trumedia_params_HPF {
	uint32_t				v1;
	uint32_t				v2[26];
} __packed;

struct srs_trumedia_params_PEQ {
	uint32_t				v1;
	uint16_t				v2;
	uint16_t				v3;
	uint16_t				v4;
	uint16_t				v____A1;
	uint32_t				v5[26];
	uint32_t				v6[26];
} __packed;

struct srs_trumedia_params_HL {
	uint16_t				v1;
	uint16_t				v2;
	uint16_t				v3;
	uint16_t				v____A1;
	int32_t					v4;
	uint32_t				v5;
	uint16_t				v6;
	uint16_t				v____A2;
	uint32_t				v7;
} __packed;

struct srs_trumedia_params {
	struct srs_trumedia_params_GLOBAL	global;
	struct srs_trumedia_params_WOWHD	wowhd;
	struct srs_trumedia_params_CSHP		cshp;
	struct srs_trumedia_params_HPF		hpf;
	struct srs_trumedia_params_PEQ		peq;
	struct srs_trumedia_params_HL		hl;
} __packed;
int srs_trumedia_open(int port_id, int srs_tech_id, void *srs_params);

#endif 
