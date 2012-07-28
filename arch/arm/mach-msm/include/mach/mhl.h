/* include/linux/mhl.h
 *
 * Copyright (C) 2010 HTC, Inc.
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

#ifndef _MHL_H_
#define _MHL_H_
//********************************************************************
//  Nested Include Files
//********************************************************************

//********************************************************************
//  Manifest Constants
//********************************************************************
#define MHL_SII9234_I2C_NAME   "sii9234"
#ifdef CONFIG_FB_MSM_HDMI_MHL_SUPERDEMO
#define MHL_SII9234_TOUCH_FINGER_NUM_MAX 2
#endif
//********************************************************************
//  Type Definitions
//********************************************************************
typedef enum {
	E_MHL_FALSE = 0,
	E_MHL_TRUE  = 1,
	E_MHL_OK    = 0,
	E_MHL_FAIL  = 1,
	E_MHL_NA    = 0xFF
} T_MHL_RV;

#ifdef CONFIG_FB_MSM_HDMI_MHL_SUPERDEMO
typedef enum {
	E_MHL_EVT_REMOTE_KEY           = 0x0001,
	E_MHL_EVT_REMOTE_KEY_RELEASE   = 0x0002,
	E_MHL_EVT_REMOTE_TOUCH         = 0x0003,
	E_MHL_EVT_REMOTE_TOUCH_RELEASE = 0x0006,
	E_MHL_EVT_REMOTE_TOUCH_PRESS   = 0x0007,
	E_MHL_EVT_UNKNOWN              = 0xFFFF
} T_MHL_EVT_REMOTE_CNTL;

typedef struct {
	int x;
	int y;
	int z;
} T_MHL_REMOTE_FINGER_DATA;

typedef struct {
	int keyIn;
	int keyCode;
} T_MHL_REMOTE_KEY_DATA;
#endif

typedef struct {
	bool valid;
	uint8_t regA3;
	uint8_t regA6;
} mhl_board_params;

typedef struct {
	int gpio_intr;
	int gpio_reset;
	int ci2ca;
	void (*mhl_usb_switch)(int);
	void (*mhl_1v2_power)(bool enable);
	int (*enable_5v)(int on);
	int (*power)(int);
	int (*mhl_power_vote)(bool enable);
#ifdef CONFIG_FB_MSM_HDMI_MHL_SUPERDEMO
	int abs_x_min;
	int abs_x_max;
	int abs_y_min;
	int abs_y_max;
	int abs_pressure_min;
	int abs_pressure_max;
	int abs_width_min;
	int abs_width_max;
#endif
} T_MHL_PLATFORM_DATA;

//********************************************************************
//  Define & Macro
//********************************************************************
#define M_MHL_SEND_DEBUG(x...) pr_info(x)
//********************************************************************
//  Prototype & Extern variable, function
//********************************************************************
#ifdef CONFIG_FB_MSM_HDMI_MHL
extern void sii9234_change_usb_owner(bool bMHL);
#endif
#endif/*_MHL_H_*/

