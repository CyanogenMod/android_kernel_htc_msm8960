/* Copyright (c) 2011-2013, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __QDSP6VOICE_H__
#define __QDSP6VOICE_H__

#include <mach/qdsp6v2/apr.h>
#include <linux/ion.h>

#define MAX_VOC_PKT_SIZE 642
#define SESSION_NAME_LEN 21

#define VOC_REC_UPLINK		0x00
#define VOC_REC_DOWNLINK	0x01
#define VOC_REC_BOTH		0x02

struct voice_header {
	uint32_t id;
	uint32_t data_len;
};

struct voice_init {
	struct voice_header hdr;
	void *cb_handle;
};


struct device_data {
	uint32_t volume; 
	uint32_t mute;
	uint32_t sample;
	uint32_t enabled;
	uint32_t dev_id;
	uint32_t port_id;
};

struct voice_dev_route_state {
	u16 rx_route_flag;
	u16 tx_route_flag;
};

struct voice_rec_route_state {
	u16 ul_flag;
	u16 dl_flag;
};

enum {
	VOC_INIT = 0,
	VOC_RUN,
	VOC_CHANGE,
	VOC_RELEASE,
	VOC_STANDBY,
};

#define VSS_ICOMMON_CMD_SET_UI_PROPERTY 0x00011103
#define VSS_ICOMMON_CMD_MAP_MEMORY   0x00011025
#define VSS_ICOMMON_CMD_UNMAP_MEMORY 0x00011026
#define VSS_ICOMMON_MAP_MEMORY_SHMEM8_4K_POOL  3

struct vss_icommon_cmd_map_memory_t {
	uint32_t phys_addr;

	uint32_t mem_size;
	

	uint16_t mem_pool_id;
} __packed;

struct vss_icommon_cmd_unmap_memory_t {
	uint32_t phys_addr;
} __packed;

struct vss_map_memory_cmd {
	struct apr_hdr hdr;
	struct vss_icommon_cmd_map_memory_t vss_map_mem;
} __packed;

struct vss_unmap_memory_cmd {
	struct apr_hdr hdr;
	struct vss_icommon_cmd_unmap_memory_t vss_unmap_mem;
} __packed;

#define VSS_IMVM_CMD_CREATE_PASSIVE_CONTROL_SESSION	0x000110FF

#define VSS_IMVM_CMD_SET_POLICY_DUAL_CONTROL	0x00011327

#define VSS_IMVM_CMD_CREATE_FULL_CONTROL_SESSION	0x000110FE

#define APRV2_IBASIC_CMD_DESTROY_SESSION		0x0001003C

#define VSS_IMVM_CMD_ATTACH_STREAM			0x0001123C

#define VSS_IMVM_CMD_DETACH_STREAM			0x0001123D

#define VSS_IMVM_CMD_ATTACH_VOCPROC		       0x0001123E

#define VSS_IMVM_CMD_DETACH_VOCPROC			0x0001123F

#define VSS_IMVM_CMD_START_VOICE			0x00011190

#define VSS_IMVM_CMD_STANDBY_VOICE	0x00011191

#define VSS_IMVM_CMD_STOP_VOICE				0x00011192

#define VSS_ISTREAM_CMD_ATTACH_VOCPROC			0x000110F8

#define VSS_ISTREAM_CMD_DETACH_VOCPROC			0x000110F9


#define VSS_ISTREAM_CMD_SET_TTY_MODE			0x00011196

#define VSS_ICOMMON_CMD_SET_NETWORK			0x0001119C

#define VSS_ICOMMON_CMD_SET_VOICE_TIMING		0x000111E0

#define VSS_IWIDEVOICE_CMD_SET_WIDEVOICE                0x00011243

enum msm_audio_voc_rate {
		VOC_0_RATE, 
		VOC_8_RATE, 
		VOC_4_RATE, 
		VOC_2_RATE, 
		VOC_1_RATE  
};

struct vss_istream_cmd_set_tty_mode_t {
	uint32_t mode;
} __packed;

struct vss_istream_cmd_attach_vocproc_t {
	uint16_t handle;
	
} __packed;

struct vss_istream_cmd_detach_vocproc_t {
	uint16_t handle;
	
} __packed;

struct vss_imvm_cmd_attach_stream_t {
	uint16_t handle;
	
} __packed;

struct vss_imvm_cmd_detach_stream_t {
	uint16_t handle;
	
} __packed;

struct vss_icommon_cmd_set_network_t {
	uint32_t network_id;
	
} __packed;

struct vss_icommon_cmd_set_voice_timing_t {
	uint16_t mode;
	uint16_t enc_offset;
	uint16_t dec_req_offset;
	uint16_t dec_offset;
} __packed;

struct vss_imvm_cmd_create_control_session_t {
	char name[SESSION_NAME_LEN];
} __packed;


struct vss_imvm_cmd_set_policy_dual_control_t {
	bool enable_flag;
	
} __packed;

struct vss_iwidevoice_cmd_set_widevoice_t {
	uint32_t enable;
} __packed;

struct mvm_attach_vocproc_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_attach_vocproc_t mvm_attach_cvp_handle;
} __packed;

struct mvm_detach_vocproc_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_detach_vocproc_t mvm_detach_cvp_handle;
} __packed;

struct mvm_create_ctl_session_cmd {
	struct apr_hdr hdr;
	struct vss_imvm_cmd_create_control_session_t mvm_session;
} __packed;

struct mvm_modem_dual_control_session_cmd {
	struct apr_hdr hdr;
	struct vss_imvm_cmd_set_policy_dual_control_t voice_ctl;
} __packed;

struct mvm_set_tty_mode_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_set_tty_mode_t tty_mode;
} __packed;

struct mvm_attach_stream_cmd {
	struct apr_hdr hdr;
	struct vss_imvm_cmd_attach_stream_t attach_stream;
} __packed;

struct mvm_detach_stream_cmd {
	struct apr_hdr hdr;
	struct vss_imvm_cmd_detach_stream_t detach_stream;
} __packed;

struct mvm_set_network_cmd {
	struct apr_hdr hdr;
	struct vss_icommon_cmd_set_network_t network;
} __packed;

struct mvm_set_voice_timing_cmd {
	struct apr_hdr hdr;
	struct vss_icommon_cmd_set_voice_timing_t timing;
} __packed;

struct mvm_set_widevoice_enable_cmd {
	struct apr_hdr hdr;
	struct vss_iwidevoice_cmd_set_widevoice_t vss_set_wv;
} __packed;

#define VSS_ISTREAM_CMD_CREATE_PASSIVE_CONTROL_SESSION	0x00011140

#define VSS_ISTREAM_CMD_CREATE_FULL_CONTROL_SESSION	0x000110F7

#define APRV2_IBASIC_CMD_DESTROY_SESSION		0x0001003C

#define VSS_ISTREAM_CMD_SET_MUTE			0x00011022

#define VSS_ISTREAM_CMD_REGISTER_CALIBRATION_DATA	0x00011279

#define VSS_ISTREAM_CMD_DEREGISTER_CALIBRATION_DATA     0x0001127A

#define VSS_ISTREAM_CMD_SET_MEDIA_TYPE			0x00011186

#define VSS_ISTREAM_EVT_SEND_ENC_BUFFER			0x00011015

#define VSS_ISTREAM_EVT_REQUEST_DEC_BUFFER		0x00011017

#define VSS_ISTREAM_EVT_SEND_DEC_BUFFER			0x00011016

#define VSS_ISTREAM_CMD_VOC_AMR_SET_ENC_RATE		0x0001113E

#define VSS_ISTREAM_CMD_VOC_AMRWB_SET_ENC_RATE		0x0001113F

#define VSS_ISTREAM_CMD_CDMA_SET_ENC_MINMAX_RATE	0x00011019

#define VSS_ISTREAM_CMD_SET_ENC_DTX_MODE		0x0001101D

#define MODULE_ID_VOICE_MODULE_FENS			0x00010EEB
#define MODULE_ID_VOICE_MODULE_ST			0x00010EE3
#define VOICE_PARAM_MOD_ENABLE				0x00010E00
#define MOD_ENABLE_PARAM_LEN				4

#define VSS_ISTREAM_CMD_START_PLAYBACK                  0x00011238

#define VSS_ISTREAM_CMD_STOP_PLAYBACK                   0x00011239

#define VSS_ISTREAM_CMD_START_RECORD                    0x00011236
#define VSS_ISTREAM_CMD_STOP_RECORD                     0x00011237

#define VSS_TAP_POINT_NONE                              0x00010F78

#define VSS_TAP_POINT_STREAM_END                        0x00010F79

struct vss_istream_cmd_start_record_t {
	uint32_t rx_tap_point;
	uint32_t tx_tap_point;
} __packed;

struct vss_istream_cmd_create_passive_control_session_t {
	char name[SESSION_NAME_LEN];
} __packed;

struct vss_istream_cmd_set_mute_t {
	uint16_t direction;
	uint16_t mute_flag;
} __packed;

struct vss_istream_cmd_create_full_control_session_t {
	uint16_t direction;
	uint32_t enc_media_type;
	
	uint32_t dec_media_type;
	
	uint32_t network_id;
	
	char name[SESSION_NAME_LEN];
} __packed;

struct vss_istream_cmd_set_media_type_t {
	uint32_t rx_media_id;
	
	uint32_t tx_media_id;
	
} __packed;

struct vss_istream_evt_send_enc_buffer_t {
	uint32_t media_id;
      
	uint8_t packet_data[MAX_VOC_PKT_SIZE];
      
} __packed;

struct vss_istream_evt_send_dec_buffer_t {
	uint32_t media_id;
      
	uint8_t packet_data[MAX_VOC_PKT_SIZE];
      
} __packed;

struct vss_istream_cmd_voc_amr_set_enc_rate_t {
	uint32_t mode;
} __packed;

struct vss_istream_cmd_voc_amrwb_set_enc_rate_t {
	uint32_t mode;
} __packed;

struct vss_istream_cmd_cdma_set_enc_minmax_rate_t {
	uint16_t min_rate;
	uint16_t max_rate;
} __packed;

struct vss_istream_cmd_set_enc_dtx_mode_t {
	uint32_t enable;
} __packed;

struct vss_istream_cmd_register_calibration_data_t {
	uint32_t phys_addr;
	uint32_t mem_size;
	
};

struct vss_icommon_cmd_set_ui_property_enable_t {
	uint32_t module_id;
	
	uint32_t param_id;
	
	uint16_t param_size;
	
	uint16_t reserved;
	
	uint16_t enable;
	uint16_t reserved_field;
	
};

struct cvs_create_passive_ctl_session_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_create_passive_control_session_t cvs_session;
} __packed;

struct cvs_create_full_ctl_session_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_create_full_control_session_t cvs_session;
} __packed;

struct cvs_destroy_session_cmd {
	struct apr_hdr hdr;
} __packed;

struct cvs_set_mute_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_set_mute_t cvs_set_mute;
} __packed;

struct cvs_set_media_type_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_set_media_type_t media_type;
} __packed;

struct cvs_send_dec_buf_cmd {
	struct apr_hdr hdr;
	struct vss_istream_evt_send_dec_buffer_t dec_buf;
} __packed;

struct cvs_set_amr_enc_rate_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_voc_amr_set_enc_rate_t amr_rate;
} __packed;

struct cvs_set_amrwb_enc_rate_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_voc_amrwb_set_enc_rate_t amrwb_rate;
} __packed;

struct cvs_set_cdma_enc_minmax_rate_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_cdma_set_enc_minmax_rate_t cdma_rate;
} __packed;

struct cvs_set_enc_dtx_mode_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_set_enc_dtx_mode_t dtx_mode;
} __packed;

struct cvs_register_cal_data_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_register_calibration_data_t cvs_cal_data;
} __packed;

struct cvs_deregister_cal_data_cmd {
	struct apr_hdr hdr;
} __packed;

struct cvs_set_pp_enable_cmd {
	struct apr_hdr hdr;
	struct vss_icommon_cmd_set_ui_property_enable_t vss_set_pp;
} __packed;
struct cvs_start_record_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_start_record_t rec_mode;
} __packed;


#define VSS_IVOCPROC_CMD_CREATE_FULL_CONTROL_SESSION	0x000100C3

#define APRV2_IBASIC_CMD_DESTROY_SESSION		0x0001003C

#define VSS_IVOCPROC_CMD_SET_DEVICE			0x000100C4

#define VSS_IVOCPROC_CMD_SET_DEVICE_V2		0x000112C6

#define VSS_IVOCPROC_CMD_SET_VP3_DATA			0x000110EB

#define VSS_IVOCPROC_CMD_SET_RX_VOLUME_INDEX		0x000110EE

#define VSS_IVOCPROC_CMD_ENABLE				0x000100C6

#define VSS_IVOCPROC_CMD_DISABLE			0x000110E1

#define VSS_IVOCPROC_CMD_REGISTER_CALIBRATION_DATA	0x00011275
#define VSS_IVOCPROC_CMD_DEREGISTER_CALIBRATION_DATA    0x00011276

#define VSS_IVOCPROC_CMD_REGISTER_VOLUME_CAL_TABLE      0x00011277
#define VSS_IVOCPROC_CMD_DEREGISTER_VOLUME_CAL_TABLE    0x00011278

#define VSS_IVOCPROC_TOPOLOGY_ID_NONE			0x00010F70
#define VSS_IVOCPROC_TOPOLOGY_ID_TX_SM_ECNS		0x00010F71
#define VSS_IVOCPROC_TOPOLOGY_ID_TX_DM_FLUENCE		0x00010F72

#define VSS_IVOCPROC_TOPOLOGY_ID_RX_DEFAULT		0x00010F77

#define VSS_NETWORK_ID_DEFAULT				0x00010037
#define VSS_NETWORK_ID_VOIP_NB				0x00011240
#define VSS_NETWORK_ID_VOIP_WB				0x00011241
#define VSS_NETWORK_ID_VOIP_WV				0x00011242

#define VSS_MEDIA_ID_EVRC_MODEM		0x00010FC2
#define VSS_MEDIA_ID_AMR_NB_MODEM	0x00010FC6
#define VSS_MEDIA_ID_AMR_WB_MODEM	0x00010FC7
#define VSS_MEDIA_ID_PCM_NB		0x00010FCB
#define VSS_MEDIA_ID_PCM_WB		0x00010FCC
#define VSS_MEDIA_ID_G711_ALAW		0x00010FCD
#define VSS_MEDIA_ID_G711_MULAW		0x00010FCE
#define VSS_MEDIA_ID_G729		0x00010FD0
#define VSS_MEDIA_ID_4GV_NB_MODEM	0x00010FC3
#define VSS_MEDIA_ID_4GV_WB_MODEM	0x00010FC4

#define VSS_IVOCPROC_CMD_SET_MUTE			0x000110EF

#define VOICE_CMD_SET_PARAM				0x00011006
#define VOICE_CMD_GET_PARAM				0x00011007
#define VOICE_EVT_GET_PARAM_ACK				0x00011008

#define VSS_IVOCPROC_PORT_ID_NONE			0xFFFF

struct vss_ivocproc_cmd_create_full_control_session_t {
	uint16_t direction;
	uint32_t tx_port_id;
	uint32_t tx_topology_id;
	uint32_t rx_port_id;
	uint32_t rx_topology_id;
	int32_t network_id;
} __packed;

struct vss_ivocproc_cmd_set_volume_index_t {
	uint16_t vol_index;
} __packed;

struct vss_ivocproc_cmd_set_device_t {
	uint32_t tx_port_id;
	uint32_t tx_topology_id;
	int32_t rx_port_id;
	uint32_t rx_topology_id;
} __packed;

#define VSS_IVOCPROC_VOCPROC_MODE_EC_INT_MIXING 0x00010F7C

#define VSS_IVOCPROC_VOCPROC_MODE_EC_EXT_MIXING 0x00010F7D

struct vss_ivocproc_cmd_set_device_v2_t {
	uint16_t tx_port_id;
	
	uint32_t tx_topology_id;
	
	uint16_t rx_port_id;
	
	uint32_t rx_topology_id;
	
	uint32_t vocproc_mode;
	uint16_t ec_ref_port_id;
} __packed;

struct vss_ivocproc_cmd_register_calibration_data_t {
	uint32_t phys_addr;
	uint32_t mem_size;
	
} __packed;

struct vss_ivocproc_cmd_register_volume_cal_table_t {
	uint32_t phys_addr;

	uint32_t mem_size;
	
} __packed;

struct vss_ivocproc_cmd_set_mute_t {
	uint16_t direction;
	uint16_t mute_flag;
} __packed;

struct cvp_create_full_ctl_session_cmd {
	struct apr_hdr hdr;
	struct vss_ivocproc_cmd_create_full_control_session_t cvp_session;
} __packed;

struct cvp_command {
	struct apr_hdr hdr;
} __packed;

struct cvp_set_device_cmd {
	struct apr_hdr hdr;
	struct vss_ivocproc_cmd_set_device_t cvp_set_device;
} __packed;

struct cvp_set_device_cmd_v2 {
	struct apr_hdr hdr;
	struct vss_ivocproc_cmd_set_device_v2_t cvp_set_device_v2;
} __packed;

struct cvp_set_vp3_data_cmd {
	struct apr_hdr hdr;
} __packed;

struct cvp_set_rx_volume_index_cmd {
	struct apr_hdr hdr;
	struct vss_ivocproc_cmd_set_volume_index_t cvp_set_vol_idx;
} __packed;

struct cvp_register_cal_data_cmd {
	struct apr_hdr hdr;
	struct vss_ivocproc_cmd_register_calibration_data_t cvp_cal_data;
} __packed;

struct cvp_deregister_cal_data_cmd {
	struct apr_hdr hdr;
} __packed;

struct cvp_register_vol_cal_table_cmd {
	struct apr_hdr hdr;
	struct vss_ivocproc_cmd_register_volume_cal_table_t cvp_vol_cal_tbl;
} __packed;

struct cvp_deregister_vol_cal_table_cmd {
	struct apr_hdr hdr;
} __packed;

struct cvp_set_mute_cmd {
	struct apr_hdr hdr;
	struct vss_ivocproc_cmd_set_mute_t cvp_set_mute;
} __packed;

typedef void (*ul_cb_fn)(uint8_t *voc_pkt,
			 uint32_t pkt_len,
			 void *private_data);

typedef void (*dl_cb_fn)(uint8_t *voc_pkt,
			 uint32_t *pkt_len,
			 void *private_data);


struct mvs_driver_info {
	uint32_t media_type;
	uint32_t rate;
	uint32_t network_type;
	uint32_t dtx_mode;
	ul_cb_fn ul_cb;
	dl_cb_fn dl_cb;
	void *private_data;
};

struct incall_rec_info {
	uint32_t rec_enable;
	uint32_t rec_mode;
	uint32_t recording;
};

struct incall_music_info {
	uint32_t play_enable;
	uint32_t playing;
	int count;
	int force;
};

struct voice_data {
	int voc_state;

	wait_queue_head_t mvm_wait;
	wait_queue_head_t cvs_wait;
	wait_queue_head_t cvp_wait;

	
	struct device_data dev_rx;
	struct device_data dev_tx;

	u32 mvm_state;
	u32 cvs_state;
	u32 cvp_state;

	
	u16 mvm_handle;
	
	u16 cvs_handle;
	
	u16 cvp_handle;

	struct mutex lock;

	uint16_t sidetone_gain;
	uint8_t tty_mode;
	
	uint8_t wv_enable;
	
	uint32_t st_enable;
	
	uint32_t fens_enable;

	struct voice_dev_route_state voc_route_state;

	u16 session_id;

	struct incall_rec_info rec_info;

	struct incall_music_info music_info;

	struct voice_rec_route_state rec_route_state;
};

struct cal_mem {
	struct ion_handle *handle;
	uint32_t phy;
	void *buf;
};

#define MAX_VOC_SESSIONS 4
#define SESSION_ID_BASE 0xFFF0

struct common_data {
	
	uint32_t default_mute_val;
	uint32_t default_vol_val;
	uint32_t default_sample_val;
	bool ec_ref_ext;
	uint16_t ec_port_id;

	
	void *apr_q6_mvm;
	
	void *apr_q6_cvs;
	
	void *apr_q6_cvp;

	struct ion_client *client;
	struct cal_mem cvp_cal;
	struct cal_mem cvs_cal;

	struct mutex common_lock;

	struct mvs_driver_info mvs_info;

	struct voice_data voice[MAX_VOC_SESSIONS];
};

void voc_register_mvs_cb(ul_cb_fn ul_cb,
			dl_cb_fn dl_cb,
			void *private_data);

void voc_config_vocoder(uint32_t media_type,
			uint32_t rate,
			uint32_t network_type,
			uint32_t dtx_mode);

enum {
	DEV_RX = 0,
	DEV_TX,
};

enum {
	RX_PATH = 0,
	TX_PATH,
};

int voc_set_pp_enable(uint16_t session_id, uint32_t module_id, uint32_t enable);
int voc_get_pp_enable(uint16_t session_id, uint32_t module_id);
int voc_set_widevoice_enable(uint16_t session_id, uint32_t wv_enable);
uint32_t voc_get_widevoice_enable(uint16_t session_id);
uint8_t voc_get_tty_mode(uint16_t session_id);
int voc_set_tty_mode(uint16_t session_id, uint8_t tty_mode);
int voc_start_voice_call(uint16_t session_id);
int voc_standby_voice_call(uint16_t session_id);
int voc_resume_voice_call(uint16_t session_id);
int voc_end_voice_call(uint16_t session_id);
int voc_set_rxtx_port(uint16_t session_id,
		      uint32_t dev_port_id,
		      uint32_t dev_type);
int voc_set_rx_vol_index(uint16_t session_id, uint32_t dir, uint32_t voc_idx);
int voc_set_tx_mute(uint16_t session_id, uint32_t dir, uint32_t mute);
int voc_set_rx_device_mute(uint16_t session_id, uint32_t mute);
int voc_get_rx_device_mute(uint16_t session_id);
int voc_disable_cvp(uint16_t session_id);
int voc_enable_cvp(uint16_t session_id);
int voc_set_route_flag(uint16_t session_id, uint8_t path_dir, uint8_t set);
uint8_t voc_get_route_flag(uint16_t session_id, uint8_t path_dir);

#define VOICE_SESSION_NAME "Voice session"
#define VOIP_SESSION_NAME "VoIP session"
#define VOLTE_SESSION_NAME "VoLTE session"
#define VOICE2_SESSION_NAME "Voice2 session"

uint16_t voc_get_session_id(char *name);

int voc_start_playback(uint32_t set);
int voc_start_record(uint32_t port_id, uint32_t set);
int voc_set_ext_ec_ref(uint16_t port_id, bool state);

#endif
