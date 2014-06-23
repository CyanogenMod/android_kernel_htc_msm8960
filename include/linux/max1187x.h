/* include/linux/max1187x.h
 *
 * Copyright (c)2012 Maxim Integrated Products, Inc.
 *
 * Driver Version: 3.0.7
 * Release Date: Feb 22, 2013
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

#ifndef __MAX1187X_H
#define __MAX1187X_H

#define MAX1187X_NAME   "max1187x"
#define MAX1187X_TOUCH  MAX1187X_NAME "_touchscreen_0"
#define MAX1187X_KEY    MAX1187X_NAME "_key_0"
#define MAX1187X_LOG_NAME	"[TP] "

#define MAX_WORDS_COMMAND  9  
#define MAX_WORDS_REPORT   245  
#define MAX_WORDS_COMMAND_ALL  (15 * MAX_WORDS_COMMAND)  

#define MAX1187X_NUM_FW_MAPPINGS_MAX    5
#define MAX1187X_TOUCH_COUNT_MAX        10
#define MAX1187X_TOUCH_REPORT_RAW       0x0800
#define MAX1187X_TOUCH_REPORT_BASIC     0x0801
#define MAX1187X_TOUCH_REPORT_EXTENDED  0x0802
#define MAX_REPORT_READERS		5
#define DEBUG_STRING_LEN_MAX 60
#define MAX_FW_RETRIES 5

#define MAX1187X_PI 205887 

#define MAX1187X_TOUCH_CONFIG_MAX		65
#define MAX1187X_CALIB_TABLE_MAX		74
#define MAX1187X_PRIVATE_CONFIG_MAX		34
#define MAX1187X_LOOKUP_TABLE_MAX		8
#define MAX1187X_IMAGE_FACTOR_MAX		460

#define MAX1187X_NO_BASELINE   0
#define MAX1187X_FIX_BASELINE  1
#define MAX1187X_AUTO_BASELINE 2

struct max1187x_touch_report_header {
	u16 header;
	u16 report_id;
	u16 report_size;
	u16 touch_count:4;
	u16 touch_status:4;
	u16 reserved0:5;
	u16 cycles:1;
	u16 reserved1:2;
	u16 button0:1;
	u16 button1:1;
	u16 button2:1;
	u16 button3:1;
	u16 reserved2:12;
	u16 framecounter;
};

struct max1187x_touch_report_basic {
	u16 finger_id:4;
	u16 reserved0:4;
	u16 finger_status:4;
	u16 reserved1:4;
	u16 x:12;
	u16 reserved2:4;
	u16 y:12;
	u16 reserved3:4;
	u16 z;
};

struct max1187x_touch_report_extended {
	u16 finger_id:4;
	u16 reserved0:4;
	u16 finger_status:4;
	u16 reserved1:4;
	u16 x:12;
	u16 reserved2:4;
	u16 y:12;
	u16 reserved3:4;
	u16 z;
	s16 xspeed;
	s16 yspeed;
	s8 xpixel;
	s8 ypixel;
	u16 area;
	u16 xmin;
	u16 xmax;
	u16 ymin;
	u16 ymax;
};

struct max1187x_board_config {
	u16                         config_id;
	u16                         chip_id;
	u8                          major_ver;
	u8                          minor_ver;
	u8                          protocol_ver;
	u16                         vendor_pin;
	u16                         config_touch[MAX1187X_TOUCH_CONFIG_MAX];
	u16                         config_cal[MAX1187X_CALIB_TABLE_MAX];
	u16                         config_private[MAX1187X_PRIVATE_CONFIG_MAX];
	u16                         config_lin_x[MAX1187X_LOOKUP_TABLE_MAX];
	u16                         config_lin_y[MAX1187X_LOOKUP_TABLE_MAX];
	u16                         config_ifactor[MAX1187X_IMAGE_FACTOR_MAX];
};

struct max1187x_virtual_key {
	int index;
	int keycode;
	int x_position;
	int y_position;
};

struct max1187x_fw_mapping {
	u32				chip_id;
	char			*filename;
	u32				filesize;
	u32				filecrc16;
	u32				file_codesize;
};

struct max1187x_pdata {
	struct max1187x_board_config *fw_config;
	u32			gpio_tirq;
	u32			gpio_reset;
	u32			num_fw_mappings;
	struct max1187x_fw_mapping  fw_mapping[MAX1187X_NUM_FW_MAPPINGS_MAX];
	u32			defaults_allow;
	u32			default_config_id;
	u32			default_chip_id;
	u32			i2c_words;
	#define MAX1187X_REVERSE_X  0x0001
	#define MAX1187X_REVERSE_Y  0x0002
	#define MAX1187X_SWAP_XY    0x0004
	u32			coordinate_settings;
	u32			panel_min_x;
	u32			panel_max_x;
	u32			panel_min_y;
	u32			panel_max_y;
	u32			lcd_x;
	u32			lcd_y;
	u32			num_rows;
	u32			num_cols;
	#define MAX1187X_PROTOCOL_A        0
	#define MAX1187X_PROTOCOL_B        1
	#define MAX1187X_PROTOCOL_CUSTOM1  2
	u16			input_protocol;
	#define MAX1187X_UPDATE_NONE       0
	#define MAX1187X_UPDATE_BIN        1
	#define MAX1187X_UPDATE_CONFIG     2
	#define MAX1187X_UPDATE_BOTH       3
	u8			update_feature;
	u8			support_htc_event;
	u16			tw_mask;
	u32			button_code0;
	u32			button_code1;
	u32			button_code2;
	u32			button_code3;
	#define MAX1187X_REPORT_MODE_BASIC	1
	#define MAX1187X_REPORT_MODE_EXTEND	2
	u8			report_mode;
	struct max1187x_virtual_key *button_data;
};

#endif 

