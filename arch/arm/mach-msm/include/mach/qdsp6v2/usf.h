/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#ifndef __USF_H__
#define __USF_H__

#include <linux/types.h>
#include <linux/ioctl.h>

#define USF_IOCTL_MAGIC 'U'

#define US_SET_TX_INFO   _IOW(USF_IOCTL_MAGIC, 0, \
				struct us_tx_info_type)
#define US_START_TX      _IO(USF_IOCTL_MAGIC, 1)
#define US_GET_TX_UPDATE _IOWR(USF_IOCTL_MAGIC, 2, \
				struct us_tx_update_info_type)
#define US_SET_RX_INFO   _IOW(USF_IOCTL_MAGIC, 3, \
				struct us_rx_info_type)
#define US_SET_RX_UPDATE _IOWR(USF_IOCTL_MAGIC, 4, \
				struct us_rx_update_info_type)
#define US_START_RX      _IO(USF_IOCTL_MAGIC, 5)

#define US_STOP_TX      _IO(USF_IOCTL_MAGIC, 6)
#define US_STOP_RX      _IO(USF_IOCTL_MAGIC, 7)

#define US_SET_DETECTION _IOWR(USF_IOCTL_MAGIC, 8, \
				struct us_detect_info_type)

#define US_GET_VERSION  _IOWR(USF_IOCTL_MAGIC, 9, \
				struct us_version_info_type)

#define USF_NO_WAIT_TIMEOUT	0x00000000
#define USF_INFINITIVE_TIMEOUT	0xffffffff
#define USF_DEFAULT_TIMEOUT	0xfffffffe

enum us_detect_place_enum {
	US_DETECT_HW,
	US_DETECT_FW
};

enum us_detect_mode_enum {
	US_DETECT_DISABLED_MODE,
	US_DETECT_CONTINUE_MODE,
	US_DETECT_SHOT_MODE
};

#define USF_POINT_EPOS_FORMAT	0
#define USF_RAW_FORMAT		1

#define USF_TSC_EVENT_IND      0
#define USF_TSC_PTR_EVENT_IND  1
#define USF_MOUSE_EVENT_IND    2
#define USF_KEYBOARD_EVENT_IND 3
#define USF_MAX_EVENT_IND      4

#define USF_NO_EVENT 0
#define USF_TSC_EVENT      (1 << USF_TSC_EVENT_IND)
#define USF_TSC_PTR_EVENT  (1 << USF_TSC_PTR_EVENT_IND)
#define USF_MOUSE_EVENT    (1 << USF_MOUSE_EVENT_IND)
#define USF_KEYBOARD_EVENT (1 << USF_KEYBOARD_EVENT_IND)
#define USF_ALL_EVENTS         (USF_TSC_EVENT |\
				USF_TSC_PTR_EVENT |\
				USF_MOUSE_EVENT |\
				USF_KEYBOARD_EVENT)

#define MIN_MAX_DIM 2

#define COORDINATES_DIM 3

#define TILTS_DIM 2

#define USF_MAX_CLIENT_NAME_SIZE	20
struct us_xx_info_type {
	const char *client_name;
	uint32_t dev_id;
	uint32_t stream_format;
	uint32_t sample_rate;
	uint32_t buf_size;
	uint16_t buf_num;
	uint16_t port_cnt;
	uint8_t  port_id[4];
	uint16_t bits_per_sample;
	uint16_t params_data_size;
	uint8_t *params_data;
};

enum us_input_event_src_type {
	US_INPUT_SRC_PEN,
	US_INPUT_SRC_FINGER,
	US_INPUT_SRC_UNDEF
};

struct us_input_info_type {
	
	int tsc_x_dim[MIN_MAX_DIM];
	int tsc_y_dim[MIN_MAX_DIM];
	int tsc_z_dim[MIN_MAX_DIM];
	
	int tsc_x_tilt[MIN_MAX_DIM];
	int tsc_y_tilt[MIN_MAX_DIM];
	
	int tsc_pressure[MIN_MAX_DIM];
	
	uint16_t event_types;
	
	enum us_input_event_src_type event_src;
	
	uint16_t conflicting_event_types;
};

struct us_tx_info_type {
	
	struct us_xx_info_type us_xx_info;
	
	struct us_input_info_type input_info;
};

struct us_rx_info_type {
	
	struct us_xx_info_type us_xx_info;
	
};

struct point_event_type {
	int coordinates[COORDINATES_DIM];
	
	int inclinations[TILTS_DIM];
	uint32_t pressure;
};

#define USF_BUTTON_LEFT_MASK   1
#define USF_BUTTON_MIDDLE_MASK 2
#define USF_BUTTON_RIGHT_MASK  4
struct mouse_event_type {
	int rels[COORDINATES_DIM];
	uint16_t buttons_states;
};

struct key_event_type {
	uint32_t key;
	uint8_t key_state;
};

struct usf_event_type {
	uint32_t seq_num;
	uint32_t timestamp;
	uint16_t event_type_ind;
	union {
		struct point_event_type point_event;
		struct mouse_event_type mouse_event;
		struct key_event_type   key_event;
	} event_data;
};

struct us_tx_update_info_type {
	uint16_t event_counter;
	struct usf_event_type *event;
	uint32_t free_region;
	uint32_t timeout;
	uint16_t event_filters;

	uint16_t params_data_size;
	uint8_t *params_data;
	uint32_t ready_region;
};

struct us_rx_update_info_type {
	uint32_t ready_region;
	uint16_t params_data_size;
	uint8_t *params_data;
	uint32_t free_region;
};

struct us_detect_info_type {
	enum us_detect_place_enum us_detector;
	enum us_detect_mode_enum  us_detect_mode;
	uint32_t skip_time;
	uint16_t params_data_size;
	uint8_t *params_data;
	uint32_t detect_timeout;
	bool is_us;
};

struct us_version_info_type {
	uint16_t buf_size;
	char *pbuf;
};

#endif 
