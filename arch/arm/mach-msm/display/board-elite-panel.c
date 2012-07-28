/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
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

#include "../../../../drivers/video/msm/msm_fb.h"
#include "../../../../drivers/video/msm/mipi_dsi.h"
#include "../../../../drivers/video/msm/mdp4.h"
#include <mach/gpio.h>
#include <mach/panel_id.h>
#include <mach/debug_display.h>
#include <mach/htc_battery_common.h>
#include "../devices.h"
#include "../board-elite.h"

/* Select panel operate mode : CMD, VIDEO or SWITCH mode */
#define EVA_CMD_MODE_PANEL
#undef EVA_VIDEO_MODE_PANEL
#undef EVA_SWITCH_MODE_PANEL

int bl_level_prevset = 1;
char ptype[60] = "PANEL type = ";

static struct dsi_buf elite_panel_tx_buf;
static struct dsi_buf elite_panel_rx_buf;

static char set_threelane[2] = {0xBA, 0x02}; /* DTYPE_DCS_WRITE-1 */

static char display_mode_video[2] = {0xC2, 0x03}; /* DTYPE_DCS_WRITE-1 */

#ifdef EVA_CMD_MODE_PANEL
static char display_mode_cmd[2] = {0xC2, 0x08}; /* DTYPE_DCS_WRITE-1 */
#endif

static char enter_sleep[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */
static char exit_sleep[2] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */
static char display_on[2] = {0x29, 0x00}; /* DTYPE_DCS_WRITE */
static char display_off[2] = {0x28, 0x00}; /* DTYPE_DCS_WRITE */

static char led_pwm1[2] = {0x51, 0xF0};
static char led_pwm2[2] = {0x53, 0x24};
static char led_pwm3[2] = {0x55, 0x00};
static char sw_reset[2] = {0x01, 0x00}; /* DTYPE_DCS_WRITE */
static char enable_te[2] = {0x35, 0x00};/* DTYPE_DCS_WRITE1 */
static char pwm_freq[] = {0xC9, 0x0F, 0x04, 0x1E, 0x1E,
						  0x00, 0x00, 0x00, 0x10, 0x3E};/* 9.41kHz */
static char swr01[2] = {0x01, 0x33};
static char swr02[2] = {0x02, 0x53};


static struct dsi_cmd_desc sony_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 40, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sharp_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 40, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sony_panel_video_mode_cmds_id18103_ver008[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef EVA_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif

#if 1
	/* vivi color ver 2 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x08}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x18, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x19, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1A, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x26}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x00, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x01, 0x07}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x0B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x11}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x18}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x06, 0x27}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x07, 0x2A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x2F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x2C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0B, 0x24}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0C, 0x1B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0x13}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x0C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0x07}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
#endif

#if 1
	/* gamma 2.2 6b setting start */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr01), swr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr02), swr02},


	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0xA3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0xA8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0xB3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0xBF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0xCA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xD5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xDF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0xE9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0xF3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x17}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0x6A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0xDC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x19}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x19}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0x4E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0x84}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0xDA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x41}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x55}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0x6B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0x84}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xC0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0xE5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0xA3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0xA8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0xB3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0xBF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0xCA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xD5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xDF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0xE9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0xF3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x17}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0x6A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0xDC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x19}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x19}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0x4E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0x84}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0xDA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x41}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x55}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0x6B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0x84}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xC0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEC, 0xE5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xED, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEE, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF0, 0x72}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF2, 0x7F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF4, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF6, 0xA8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF8, 0xBA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFA, 0xCA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x00, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x01, 0xD8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0xE6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0xF2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x06, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x07, 0x1C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x3D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0B, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0x9F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0xE4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x10, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x11, 0x1C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x1D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x14, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x15, 0x4E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x16, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x17, 0x81}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x18, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x19, 0xA1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1A, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0xCC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0xEA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x13}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x24, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x3E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x27, 0x4F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x29, 0x61}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2A, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2B, 0x8F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2F, 0xDA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x30, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x31, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x32, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x33, 0x72}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x34, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x35, 0x7F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x36, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x37, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x38, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0xA8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3A, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3B, 0xBA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3F, 0xCA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x40, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x41, 0xD8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x42, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x43, 0xE6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x44, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x45, 0xF2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x47, 0x1C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x48, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x49, 0x3D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4B, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4D, 0x9F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4F, 0xE4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x50, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x51, 0x1C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x52, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x1D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x54, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55, 0x4E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x56, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x58, 0x81}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x59, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5A, 0xA1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5C, 0xCC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5E, 0xEA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x60, 0x13}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x61, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x62, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x63, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x64, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x65, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x66, 0x3E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x67, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x68, 0x4F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x69, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x61}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6C, 0x8F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6E, 0xDA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x70, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x71, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x72, 0xAC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x73, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x74, 0xB2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0xBF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0xD6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0xE1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0xEB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xF5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xFE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0x1F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0x3C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0xDF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0x1B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0x1D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x52}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x8A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0xD8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0x34}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x5B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x72}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x8C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0xF0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0xAC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0xB2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0xBF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0xD6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0xE1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0xEB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xF5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xFE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0x1F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0x3C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0xDF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0x1B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0x1D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x52}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x8A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0xD8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0x34}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x5B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x72}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x8C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0xF0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xFF}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* gamma 2.2 6b setting end */
#endif

#if 1
#ifdef EVA_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x05} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
#endif
#endif

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep), exit_sleep},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x33} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x04} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x30} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0x34} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x00} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	/* For random dot noise */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	/* Enable CABC setting */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x2D} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0xFF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0xF7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0xEF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x24, 0xE7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0xDF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0xD7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x27, 0xCF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0xC7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x29, 0xBF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2A, 0xB7} },

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(enable_te), enable_te},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5E, 0x06} },

	/* NVT: Enable vivid-color, but disable CABC, please set register(55h) as 0x80  */
	/*{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x80}},*/

	/* NVT: Enable vivid-color, and enable CABC UI-Mode, please set register(55h) as 0x81 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x81}}, */

	/* NVT: Enable vivid-color, and enable CABC Still-Mode, please set register(55h) as 0x82 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55, 0x82} }, */

	/* NVT: Enable vivid-color, and enable CABC Moving-Mode, please set register(55h) as 0x83 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x83}},


	/*	{DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep},*/
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24} },
};


static struct dsi_cmd_desc sony_panel_video_mode_cmds_id28103_ver008[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef EVA_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif

	#if 1
	/* vivi color ver 2 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x08}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x18, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x19, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1A, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x26}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x00, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x01, 0x07}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x0B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x11}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x18}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x06, 0x27}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x07, 0x2A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x2F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x2C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0B, 0x24}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0C, 0x1B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0x13}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x0C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0x07}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
#endif

#if 1
	/* gamma 2.2 6b setting start */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr01), swr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr02), swr02},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0xA3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0xA8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0xB3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0xBF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0xCA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xD5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xDF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0xE9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0xF3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x17}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0x6A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0xDC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x19}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x19}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0x4E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0x84}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0xDA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x41}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x55}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0x6B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0x84}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xC0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0xE5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0xA3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0xA8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0xB3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0xBF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0xCA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xD5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xDF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0xE9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0xF3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x17}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0x6A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0xDC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x19}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x19}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0x4E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0x84}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0xDA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x41}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x55}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0x6B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0x84}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xC0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEC, 0xE5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xED, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEE, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF0, 0x72}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF2, 0x7F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF4, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF6, 0xA8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF8, 0xBA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFA, 0xCA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x00, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x01, 0xD8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0xE6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0xF2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x06, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x07, 0x1C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x3D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0B, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0x9F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0xE4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x10, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x11, 0x1C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x1D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x14, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x15, 0x4E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x16, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x17, 0x81}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x18, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x19, 0xA1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1A, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0xCC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0xEA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x13}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x24, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x3E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x27, 0x4F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x29, 0x61}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2A, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2B, 0x8F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2F, 0xDA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x30, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x31, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x32, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x33, 0x72}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x34, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x35, 0x7F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x36, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x37, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x38, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0xA8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3A, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3B, 0xBA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3F, 0xCA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x40, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x41, 0xD8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x42, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x43, 0xE6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x44, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x45, 0xF2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x47, 0x1C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x48, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x49, 0x3D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4B, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4D, 0x9F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4F, 0xE4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x50, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x51, 0x1C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x52, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x1D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x54, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55, 0x4E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x56, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x58, 0x81}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x59, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5A, 0xA1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5C, 0xCC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5E, 0xEA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x60, 0x13}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x61, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x62, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x63, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x64, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x65, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x66, 0x3E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x67, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x68, 0x4F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x69, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x61}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6C, 0x8F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6E, 0xDA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x70, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x71, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x72, 0xAC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x73, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x74, 0xB2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0xBF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0xD6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0xE1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0xEB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xF5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xFE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0x1F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0x3C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0xDF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0x1B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0x1D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x52}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x8A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0xD8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0x34}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x5B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x72}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x8C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0xF0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0xAC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0xB2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0xBF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0xD6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0xE1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0xEB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xF5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xFE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0x1F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0x3C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0xDF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0x1B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0x1D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x52}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x8A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0xD8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0x34}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x5B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x72}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x8C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0xF0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xFF}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* gamma 2.2 6b setting end */
#endif

#if 1
#ifdef EVA_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x05} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
#endif
#endif

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep), exit_sleep},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x33} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x04} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x30} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0x34} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x00} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	/* For random dot noise */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	/* Enable CABC setting */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x2D} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0xFF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0xF7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0xEF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x24, 0xE7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0xDF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0xD7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x27, 0xCF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0xC7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x29, 0xBF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2A, 0xB7} },

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(enable_te), enable_te},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5E, 0x06} },

	/* NVT: Enable vivid-color, but disable CABC, please set register(55h) as 0x80  */
	/*{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x80}},*/

	/* NVT: Enable vivid-color, and enable CABC UI-Mode, please set register(55h) as 0x81 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x81}}, */

	/* NVT: Enable vivid-color, and enable CABC Still-Mode, please set register(55h) as 0x82 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55, 0x82} }, */

	/* NVT: Enable vivid-color, and enable CABC Moving-Mode, please set register(55h) as 0x83 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x83}},


	/*	{DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep},*/
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24} },
};

static struct dsi_cmd_desc sony_panel_video_mode_cmds_c2[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef EVA_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif

#if 1
	/* vivi color ver 2 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x08}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x18, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x19, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1A, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x26}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x00, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x01, 0x07}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x0B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x11}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x18}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x06, 0x27}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x07, 0x2A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x2F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x2C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0B, 0x24}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0C, 0x1B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0x13}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x0C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0x07}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
#endif

#if 1
	/* gamma 2.2 6b setting start */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr01), swr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr02), swr02},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0xA3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0xA8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0xB3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0xBF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0xCA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xD5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xDF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0xE9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0xF3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x17}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0x6A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0xDC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x19}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x19}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0x4E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0x84}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0xDA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x41}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x55}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0x6B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0x84}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xC0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0xE5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0xA3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0xA8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0xB3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0xBF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0xCA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xD5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xDF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0xE9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0xF3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x17}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0x6A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0xDC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x19}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x19}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0x4E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0x84}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0xDA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x41}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x55}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0x6B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0x84}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xC0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEC, 0xE5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xED, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEE, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF0, 0x72}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF2, 0x7F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF4, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF6, 0xA8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF8, 0xBA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFA, 0xCA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x00, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x01, 0xD8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0xE6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0xF2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x06, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x07, 0x1C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x3D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0B, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0x9F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0xE4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x10, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x11, 0x1C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x1D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x14, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x15, 0x4E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x16, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x17, 0x81}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x18, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x19, 0xA1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1A, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0xCC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0xEA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x13}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x24, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x3E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x27, 0x4F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x29, 0x61}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2A, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2B, 0x8F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2F, 0xDA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x30, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x31, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x32, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x33, 0x72}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x34, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x35, 0x7F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x36, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x37, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x38, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0xA8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3A, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3B, 0xBA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3F, 0xCA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x40, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x41, 0xD8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x42, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x43, 0xE6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x44, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x45, 0xF2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x47, 0x1C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x48, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x49, 0x3D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4B, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4D, 0x9F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4F, 0xE4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x50, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x51, 0x1C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x52, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x1D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x54, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55, 0x4E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x56, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x58, 0x81}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x59, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5A, 0xA1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5C, 0xCC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5E, 0xEA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x60, 0x13}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x61, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x62, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x63, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x64, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x65, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x66, 0x3E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x67, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x68, 0x4F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x69, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x61}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6C, 0x8F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6E, 0xDA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x70, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x71, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x72, 0xAC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x73, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x74, 0xB2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0xBF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0xD6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0xE1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0xEB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xF5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xFE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0x1F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0x3C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0xDF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0x1B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0x1D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x52}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x8A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0xD8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0x34}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x5B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x72}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x8C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0xF0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0xAC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0xB2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0xBF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0xD6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0xE1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0xEB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xF5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xFE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0x1F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0x3C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0xDF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0x1B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0x1D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x52}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x8A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0xD8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0x34}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x5B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x72}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x8C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0xF0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xFF}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* gamma 2.2 6b setting end */
#endif

#if 1
#ifdef EVA_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x05}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x8E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x8E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x8E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
#endif
#endif

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep), exit_sleep},

	/* For random dot noise */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	/* Enable CABC setting */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x2D} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0xFF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0xF7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0xEF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x24, 0xE7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0xDF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0xD7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x27, 0xCF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0xC7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x29, 0xBF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2A, 0xB7} },

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(enable_te), enable_te},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5E, 0x06}},

	/* NVT: Enable vivid-color, but disable CABC, please set register(55h) as 0x80  */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x80}}, */

	/* NVT: Enable vivid-color, and enable CABC UI-Mode, please set register(55h) as 0x81 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x81}}, */

	/* NVT: Enable vivid-color, and enable CABC Still-Mode, please set register(55h) as 0x82 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x82}}, */

	/* NVT: Enable vivid-color, and enable CABC Moving-Mode, please set register(55h) as 0x83 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x83}},


	/*	{DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep},*/
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24}},
};

static struct dsi_cmd_desc sony_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 100,
		sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc sony_cmd_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(led_pwm1), led_pwm1}
};

/* himax command begin */
/* himax mipi commands */
static char set_twolane[2] = {0xBA, 0x11}; /* DTYPE_DCS_WRITE-1 */
static char himax_max_pkt_size[2] = {0x03, 0x00};
static char himax_password[4] = {0xB9, 0xFF, 0x83, 0x92}; /* DTYPE_DCS_LWRITE */

static char himax_cc[2] = {0xCC, 0x08}; /* DTYPE_DCS_WRITE-1 */

static char himax_b2[13] = {0xB2, 0x0F, 0xC8, 0x04, 0x0C, 0x04, 0xF4, 0x00,
							0xFF, 0x04, 0x0C, 0x04, 0x20}; /* DTYPE_DCS_LWRITE */ /*Set display related register */
static char himax_b4[21] = {0xB4, 0x12, 0x00, 0x05, 0x00, 0x9A, 0x05, 0x06,
							0x95, 0x00, 0x01, 0x06, 0x00, 0x08, 0x08, 0x00,
							0x1D, 0x08, 0x08, 0x08, 0x00}; /* DTYPE_DCS_LWRITE */ /* MPU/Command CYC */

static char himax_d8[21] = {0xD8, 0x12, 0x00, 0x05, 0x00, 0x9A, 0x05, 0x06,
							0x95, 0x00, 0x01, 0x06, 0x00, 0x08, 0x08, 0x00,
							0x1D, 0x08, 0x08, 0x08, 0x00}; /* DTYPE_DCS_LWRITE */ /* MPU/Command CYC */
static char himax_d4[2] = {0xD4, 0x0C}; /* DTYPE_DCS_WRITE-1 */

static char himax_b1[14] = {0xB1, 0x7C, 0x00, 0x44, 0x76, 0x00, 0x12, 0x12,
							0x2A, 0x25, 0x1E, 0x1E, 0x42, 0x74}; /* DTYPE_DCS_LWRITE */ /* Set Power */
static char himax_bd[4] = {0xBD, 0x00, 0x60, 0xD6}; /* DTYPE_DCS_LWRITE */

/* Gamma */
static char himax_e0[35] = {0xE0, 0x00, 0x1D, 0x27, 0x3D, 0x3C, 0x3F, 0x38,
							0x4F, 0x07, 0x0E, 0x0E, 0x10, 0x17, 0x15, 0x15,
							0x16, 0x1F, 0x00, 0x1D, 0x27, 0x3D, 0x3C, 0x3F,
							0x38, 0x4F, 0x07, 0x0E, 0x0E, 0x10, 0x17, 0x15,
							0x15, 0x16, 0x1F};
static char himax_e1[35] = {0xE1, 0x25, 0x30, 0x33, 0x3B, 0x3A, 0x3F, 0x3B,
							0x50, 0x06, 0x0E, 0x0E, 0x10, 0x14, 0x11, 0x13,
							0x15, 0x1E, 0x25, 0x30, 0x33, 0x3B, 0x3A, 0x3F,
							0x3B, 0x50, 0x06, 0x0E, 0x0E, 0x10, 0x14, 0x11,
							0x13, 0x15, 0x1E};
static char himax_e2[35] = {0xE2, 0x2E, 0x34, 0x33, 0x3A, 0x39, 0x3F, 0x39,
							0x4E, 0x07, 0x0D, 0x0E, 0x10, 0x15, 0x11, 0x15,
							0x15, 0x1E, 0x2E, 0x34, 0x33, 0x3A, 0x39, 0x3F,
							0x39, 0x4E, 0x07, 0x0D, 0x0E, 0x10, 0x15, 0x11,
							0x15, 0x15, 0x1E};

struct dsi_cmd_desc sharp_nt_video_on_cmds_idA1B100[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef EVA_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif
#if 1
        /* vivi color ver 2 */
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x03}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x08}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x18, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x19, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1A, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x66}},

        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x00, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x01, 0x07}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x0B}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x11}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x18}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x20}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x06, 0x27}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x07, 0x2A}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x2E}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x2F}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x2C}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0B, 0x24}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0C, 0x1B}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0x13}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x0C}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0x07}},

        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
#endif

	/* gamma 2.2 6b setting start */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr01), swr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr02), swr02},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* gamma 2.2 6b setting end */

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep), exit_sleep},

	/* For NV1-3 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x05}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x8E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0x06}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0x0A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x10, 0x71}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x4D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6C, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6D, 0x00}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x37, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x71, 0x2C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x26}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x86}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x2B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x33}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0x34}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},

	/* For random dot noise */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	/* Enable CABC setting */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x2D} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0xFF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0xF7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0xEF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x24, 0xE7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0xDF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0xD7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x27, 0xCF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0xC7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x29, 0xBF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2A, 0xB7} },

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(enable_te), enable_te},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5E, 0x06}},

	/* NVT: Enable vivid-color, but disable CABC, please set register(55h) as 0x80  */
	/*{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x80}},*/

	/* NVT: Enable vivid-color, and enable CABC UI-Mode, please set register(55h) as 0x81 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x81}}, */

	/* NVT: Enable vivid-color, and enable CABC Still-Mode, please set register(55h) as 0x82 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x82}}, */

	/* NVT: Enable vivid-color, and enable CABC Moving-Mode, please set register(55h) as 0x83 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x83}},


	/* {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep}, */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24}},
};

struct dsi_cmd_desc sharp_nt_video_on_cmds_nv3[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef EVA_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif
#if 1
        /* vivi color ver 2 */
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x03}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x08}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x18, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x19, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1A, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x66}},

        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x00, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x01, 0x07}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x0B}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x11}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x18}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x20}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x06, 0x27}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x07, 0x2A}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x2E}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x2F}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x2C}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0B, 0x24}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0C, 0x1B}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0x13}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x0C}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0x07}},

        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
#endif
	/* gamma 2.2 6b setting start */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr01), swr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr02), swr02},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* gamma 2.2 6b setting end */

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep), exit_sleep},

	/* For NV3 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x33}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0x34}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x05}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x37, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x8E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},

	/* For random dot noise */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	/* Enable CABC setting */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x2D} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0xFF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0xF7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0xEF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x24, 0xE7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0xDF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0xD7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x27, 0xCF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0xC7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x29, 0xBF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2A, 0xB7} },

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(enable_te), enable_te},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5E, 0x06}},

	/* NVT: Enable vivid-color, but disable CABC, please set register(55h) as 0x80  */
	/*{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x80}},*/

	/* NVT: Enable vivid-color, and enable CABC UI-Mode, please set register(55h) as 0x81 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x81}}, */

	/* NVT: Enable vivid-color, and enable CABC Still-Mode, please set register(55h) as 0x82 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x82}}, */

	/* NVT: Enable vivid-color, and enable CABC Moving-Mode, please set register(55h) as 0x83 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x83}},


	/* {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep}, */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24}},
};

struct dsi_cmd_desc sharp_nt_video_on_cmds_nv4[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef EVA_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif
#if 1
        /* vivi color ver 2 */
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x03}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x08}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x18, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x19, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1A, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x66}},

        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x00, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x01, 0x07}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x0B}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x11}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x18}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x20}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x06, 0x27}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x07, 0x2A}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x2E}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x2F}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x2C}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0B, 0x24}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0C, 0x1B}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0x13}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x0C}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0x07}},

        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
#endif
#if 1
	/* gamma 2.2 6b setting start */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr01), swr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr02), swr02},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* gamma 2.2 6b setting end */
#endif

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep), exit_sleep},

	/* For random dot noise */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	/* Enable CABC setting */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x2D} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0xFF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0xF7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0xEF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x24, 0xE7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0xDF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0xD7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x27, 0xCF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0xC7} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x29, 0xBF} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2A, 0xB7} },

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(enable_te), enable_te},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5E, 0x06}},

	/* NVT: Enable vivid-color, but disable CABC, please set register(55h) as 0x80  */
	/*{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x80}},*/

	/* NVT: Enable vivid-color, and enable CABC UI-Mode, please set register(55h) as 0x81 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x81}}, */

	/* NVT: Enable vivid-color, and enable CABC Still-Mode, please set register(55h) as 0x82 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x82}}, */

	/* NVT: Enable vivid-color, and enable CABC Moving-Mode, please set register(55h) as 0x83 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x83}},


	/* {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep}, */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24}},
};

struct dsi_cmd_desc himax_video_on_cmds_id311100[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10,	sizeof(himax_password), himax_password},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(set_threelane), set_threelane},
	{DTYPE_MAX_PKTSIZE, 1, 0, 0, 0,	sizeof(himax_max_pkt_size), himax_max_pkt_size},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,	sizeof(display_mode_video), display_mode_video},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, 13, (char[]){0xB2, 0x0F, 0xC8, 0x04, 0x0C,
												  0x04, 0xF4, 0x00, 0xFF, 0x04,
												  0x0C, 0x04, 0x20}},
	{DTYPE_DCS_WRITE, 1, 0, 0, 150,	sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(pwm_freq), pwm_freq},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,	sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,	sizeof(led_pwm3), led_pwm3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(enable_te), enable_te},
};

struct dsi_cmd_desc himax_video_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 250, sizeof(sw_reset), sw_reset},
	{DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(himax_password), himax_password},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(himax_d4), himax_d4},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(set_twolane), set_twolane},
	{DTYPE_MAX_PKTSIZE, 1, 0, 0, 0, sizeof(himax_max_pkt_size), himax_max_pkt_size},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(display_mode_video), display_mode_video},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(enable_te), enable_te},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(himax_cc), himax_cc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(himax_b1), himax_b1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(himax_b2), himax_b2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(himax_b4), himax_b4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(himax_bd), himax_bd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(himax_d8), himax_d8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(himax_e0), himax_e0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(himax_e1), himax_e1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(himax_e2), himax_e2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(pwm_freq), pwm_freq},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(led_pwm3), led_pwm3},
};

static struct dsi_cmd_desc himax_display_off_cmds[] = {
		{DTYPE_DCS_WRITE, 1, 0, 0, 0,
			sizeof(display_off), display_off},
		{DTYPE_DCS_WRITE, 1, 0, 0, 100,
			sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc himax_cmd_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(led_pwm1), led_pwm1},
};

static struct dsi_cmd_desc nvt_LowTemp_wrkr_enter[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, 2, (char[]){0x26, 0x08}},
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}}, */
};

static struct dsi_cmd_desc nvt_LowTemp_wrkr_exit[] = {
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE}}, */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10, 2, (char[]){0xFF, 0x00}},
};

/* himax command end */

#if defined CONFIG_FB_MSM_SELF_REFRESH
#define DSI_VIDEO_BASE	0xE0000
static struct msm_panel_common_pdata *mipi_elite_pdata;
static char display_mode_video_cmd_1[2] = {0xC2, 0x0B}; /* DTYPE_DCS_WRITE */
static char display_mode_video_cmd_2[2] = {0xC2, 0x00}; /* DTYPE_DCS_WRITE */

static char himax_vsync_start[2] = {0x00, 0x00}; /* DTYPE_DCS_WRITE */
static char himax_hsync_start[2] = {0x00, 0x00}; /* DTYPE_DCS_WRITE */

static struct dsi_cmd_desc video_to_cmd[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 20,
		sizeof(display_mode_video_cmd_1), display_mode_video_cmd_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(display_mode_video_cmd_2), display_mode_video_cmd_2},
};

static struct dsi_cmd_desc cmd_to_video[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(display_mode_video), display_mode_video},
};

static struct dsi_cmd_desc vsync_hsync_cmds[] = {
	{DTYPE_VSYNC_START, 1, 0, 0, 0, sizeof(himax_vsync_start), himax_vsync_start},
	{DTYPE_HSYNC_START, 1, 0, 0, 0, sizeof(himax_hsync_start), himax_hsync_start},
	{DTYPE_VSYNC_START, 1, 0, 0, 0, sizeof(himax_vsync_start), himax_vsync_start},
	{DTYPE_HSYNC_START, 1, 0, 0, 0, sizeof(himax_hsync_start), himax_hsync_start},
};

static void disable_video_mode_clk(void)
{
	/* Turn off video mode operation */
	MDP_OUTP(MDP_BASE + DSI_VIDEO_BASE, 0);
	MIPI_OUTP(MIPI_DSI_BASE + 0x0000, 0x175);
}

static void enable_video_mode_clk(void)
{
	/* Turn on video mode operation */
	MIPI_OUTP(MIPI_DSI_BASE + 0x0000, 0x173);
	MDP_OUTP(MDP_BASE + DSI_VIDEO_BASE, 1);
}

static DECLARE_WAIT_QUEUE_HEAD(elite_vsync_wait);
static unsigned int vsync_irq;
static int wait_vsync = 0;
static int elite_vsync_gpio = 0;
static int elite_irq_cnt = 0;    /* irq trigger trace */

static irqreturn_t elite_vsync_interrupt(int irq, void *data)
{
	elite_irq_cnt++;
	if (wait_vsync) {
		PR_DISP_DEBUG("Wait vsync\n");
		elite_vsync_gpio = 1;
		wake_up(&elite_vsync_wait);
		wait_vsync = 0;
	}

	return IRQ_HANDLED;
}

static int setup_vsync(struct platform_device *pdev, int init)
{
	int ret;
	int gpio = 0;
	PR_DISP_INFO("%s+++\n", __func__);

	if (!init) {
		ret = 0;
		goto uninit;
	}

	ret = gpio_request(gpio, "vsync");
	if (ret) {
		PR_DISP_ERR("[%s] request vsync failed", __func__);
		/* goto err_request_gpio_failed; */
	}

	ret = gpio_direction_input(gpio);
	if (ret) {
		PR_DISP_ERR("[%s] gpio_direction_input failed", __func__);
		goto err_gpio_direction_input_failed;
	}

	ret = vsync_irq = gpio_to_irq(gpio);
	if (ret < 0) {
		PR_DISP_ERR("[%s] gpio_to_irq failed", __func__);
		goto err_get_irq_num_failed;
	}

	ret = request_irq(vsync_irq, elite_vsync_interrupt, IRQF_TRIGGER_RISING,
			  "vsync", pdev);
	if (ret) {
		PR_DISP_ERR("[%s] request_irq failed", __func__);
		goto err_request_irq_failed;
	}
	PR_DISP_INFO("vsync on gpio %d now %d\n",
	       gpio, gpio_get_value(gpio));
	disable_irq(vsync_irq);

	PR_DISP_INFO("%s---\n", __func__);
	return 0;

uninit:
	free_irq(gpio_to_irq(gpio), pdev);
err_request_irq_failed:
err_get_irq_num_failed:
err_gpio_direction_input_failed:
	gpio_free(gpio);
/* err_request_gpio_failed: */
	return ret;
}
#endif

static char disable_dim_cmd[2] = {0x53, 0x24};/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc disable_dim[] = {{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(disable_dim_cmd), disable_dim_cmd},};
static struct dsi_cmd_desc *dim_cmds = disable_dim; /* default disable dim */
#ifdef CONFIG_FB_MSM_CABC
/* for cabc enable and disable seletion */
static char cabc_off_cmd[2] = {0x55, 0x80};/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc cabc_off[] = {{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cabc_off_cmd), cabc_off_cmd},};
static char cabc_on_ui_cmd[2] = {0x55, 0x81};/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc cabc_on_ui[] = {{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cabc_on_ui_cmd), cabc_on_ui_cmd},};
static char cabc_on_still_cmd[2] = {0x55, 0x82};/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc cabc_on_still[] = {{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cabc_on_still_cmd), cabc_on_still_cmd},};
static char cabc_on_moving_cmd[2] = {0x55, 0x83};/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc cabc_on_moving[] = {{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cabc_on_moving_cmd), cabc_on_moving_cmd},};
/* for dimming enable and disable selection */
static char enable_dim_cmd[2] = {0x53, 0x2C};/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc enable_dim[] = {{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(enable_dim_cmd), enable_dim_cmd},};
static struct dsi_cmd_desc *cabc_cmds = cabc_off; /* default disable cabc */

void enable_ic_cabc(int cabc, bool dim_on, struct msm_fb_data_type *mfd)
{
	if (dim_on)
		dim_cmds = enable_dim;
	if (cabc == 1)
		cabc_cmds = cabc_on_ui;
	else if (cabc == 2)
		cabc_cmds = cabc_on_still;
	else if (cabc == 3)
		cabc_cmds = cabc_on_moving;
	mutex_lock(&mfd->dma->ov_mutex);

	if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL) {
		mdp4_dsi_cmd_dma_busy_wait(mfd);
		mdp4_dsi_blt_dmap_busy_wait(mfd);
		mipi_dsi_mdp_busy_wait(mfd);
	}
	/*mipi_dsi_cmds_tx(mfd, &elite_panel_tx_buf, cabc_cmds, ARRAY_SIZE(cabc_on_ui));*/
	mipi_dsi_cmds_tx(mfd, &elite_panel_tx_buf, dim_cmds, ARRAY_SIZE(enable_dim));
	PR_DISP_INFO("%s: enable dimming and cabc\n", __func__);

	mutex_unlock(&mfd->dma->ov_mutex);
}
#endif

static char manufacture_id[2] = {0x04, 0x00}; 		/* DTYPE_DCS_READ */

static struct dsi_cmd_desc elite_manufacture_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id), manufacture_id};

static uint32 mipi_elite_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	uint32 *lp;

	tp = &elite_panel_tx_buf;
	rp = &elite_panel_rx_buf;
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);

	cmd = &elite_manufacture_id_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 3);
	lp = (uint32 *)rp->data;
	PR_DISP_INFO("%s: manufacture_id=%x\n", __func__, *lp);

	return *lp;
}

static struct dsi_cmd_desc *elite_video_on_cmds = NULL;
static struct dsi_cmd_desc *elite_display_off_cmds = NULL;
static struct dsi_cmd_desc *elite_cmd_backlight_cmds = NULL;
#if defined CONFIG_FB_MSM_SELF_REFRESH
static struct dsi_cmd_desc *elite_video_to_cmd = NULL;
static struct dsi_cmd_desc *elite_cmd_to_video = NULL;
int elite_video_to_cmd_cnt = 0;
int elite_cmd_to_video_cnt = 0;
#endif
int elite_video_on_cmds_count = 0;
int elite_display_off_cmds_count = 0;
int elite_cmd_backlight_cmds_count = 0;


#define PWM_MIN                   6
#define PWM_DEFAULT               91
#define PWM_MAX                   255

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 143
#define BRI_SETTING_MAX                 255


static unsigned char elite_shrink_pwm(int val)
{
	unsigned char shrink_br = BRI_SETTING_MAX;

	if (val <= 0) {
		shrink_br = 0;
	} else if (val > 0 && (val < BRI_SETTING_MIN)) {
			shrink_br = PWM_MIN;
	} else if ((val >= BRI_SETTING_MIN) && (val <= BRI_SETTING_DEF)) {
			shrink_br = (val - BRI_SETTING_MIN) * (PWM_DEFAULT - PWM_MIN) /
		(BRI_SETTING_DEF - BRI_SETTING_MIN) + PWM_MIN;
	} else if (val > BRI_SETTING_DEF && val <= BRI_SETTING_MAX) {
			shrink_br = (val - BRI_SETTING_DEF) * (PWM_MAX - PWM_DEFAULT) /
		(BRI_SETTING_MAX - BRI_SETTING_DEF) + PWM_DEFAULT;
	} else if (val > BRI_SETTING_MAX)
			shrink_br = PWM_MAX;

	PR_DISP_INFO("brightness orig=%d, transformed=%d\n", val, shrink_br);

	return shrink_br;
}

#if defined CONFIG_FB_MSM_SELF_REFRESH
static void elite_self_refresh_switch(int on)
{
	int vsync_timeout;

	if (on) {
		PR_DISP_DEBUG("[SR] %s on\n", __func__);
		mipi_set_tx_power_mode(0);
		mipi_dsi_cmds_tx(0, &elite_panel_tx_buf, video_to_cmd,
			ARRAY_SIZE(video_to_cmd));
		mipi_set_tx_power_mode(1);
		/*! himax still have problem, temporary avoid video mode clk disable.*/
		if (panel_type != PANEL_ID_ELITE_SHARP_HX)
			disable_video_mode_clk();
	} else {
		int lost_vsync_count = 0;

		lost_vsync_count = 0;
		PR_DISP_DEBUG("[SR] %s off\n", __func__);
		elite_irq_cnt = 0;
		mipi_set_tx_power_mode(0);
		enable_irq(vsync_irq);
		if (panel_type != PANEL_ID_ELITE_SHARP_HX) {
			wait_vsync = 1;
			vsync_timeout = wait_event_timeout(elite_vsync_wait, elite_vsync_gpio ||
						gpio_get_value(0), HZ/2);
			mipi_dsi_cmds_tx(0, &elite_panel_tx_buf, cmd_to_video,
				ARRAY_SIZE(cmd_to_video));
			elite_vsync_gpio = 0;
			wait_vsync = 1;
			vsync_timeout = wait_event_timeout(elite_vsync_wait, elite_vsync_gpio ||
						gpio_get_value(0), HZ/2);
			if (elite_irq_cnt > 2)
				PR_DISP_DEBUG("[SR] vsync_count:%d\n", elite_irq_cnt);
			mipi_dsi_sw_reset();
			enable_video_mode_clk();
			disable_irq(vsync_irq);
			elite_vsync_gpio = 0;
			if (vsync_timeout == 0)
				PR_DISP_DEBUG("Lost vsync! non-HX\n");
		} else {
			/*! himax still have problem, temporary marked video mode clk.*/
			mipi_dsi_cmds_tx(0, &elite_panel_tx_buf, cmd_to_video,
				ARRAY_SIZE(cmd_to_video));
			wait_vsync = 1;
			udelay(300);
			vsync_timeout = wait_event_timeout(elite_vsync_wait, elite_vsync_gpio ||
				gpio_get_value(0), HZ/2);
			disable_irq(vsync_irq);
			wait_vsync = 0;
			elite_vsync_gpio = 0;
			if (vsync_timeout == 0)
				PR_DISP_DEBUG("Lost vsync! HX\n");

			udelay(300);
			mipi_dsi_cmds_tx(0, &elite_panel_tx_buf, vsync_hsync_cmds,
				ARRAY_SIZE(vsync_hsync_cmds));
			/*mipi_dsi_sw_reset();
			enable_video_mode_clk();*/
		}
	}
}
#endif

static void elite_display_on(struct msm_fb_data_type *mfd)
{
	mutex_lock(&mfd->dma->ov_mutex);

	if (mfd->panel_info.type == MIPI_CMD_PANEL) {
		mdp4_dsi_cmd_dma_busy_wait(mfd);
		mdp4_dsi_blt_dmap_busy_wait(mfd);
		mipi_dsi_mdp_busy_wait(mfd);
	}

	if (panel_type == PANEL_ID_ELITE_SONY_NT
			|| panel_type == PANEL_ID_ELITE_SONY_NT_C1
			|| panel_type == PANEL_ID_ELITE_SONY_NT_C2)
		mipi_dsi_cmds_tx(mfd, &elite_panel_tx_buf, sony_display_on_cmds,
			ARRAY_SIZE(sony_display_on_cmds));
	else
		mipi_dsi_cmds_tx(mfd, &elite_panel_tx_buf, sharp_display_on_cmds,
			ARRAY_SIZE(sharp_display_on_cmds));

	mutex_unlock(&mfd->dma->ov_mutex);
}

static void mipi_dsi_set_backlight(struct msm_fb_data_type *mfd)
{
	struct mipi_panel_info *mipi;

	mipi  = &mfd->panel_info.mipi;

	PR_DISP_DEBUG("%s+:bl=%d\n", __func__, mfd->bl_level);
	if (bl_level_prevset == mfd->bl_level)
		return;

	/* mdp4_dsi_cmd_busy_wait: will turn on dsi clock also */

	led_pwm1[1] = elite_shrink_pwm(mfd->bl_level);

	mutex_lock(&mfd->dma->ov_mutex);

	if (mfd->bl_level == 0 ||
		board_mfg_mode() == 4 ||
		(board_mfg_mode() == 5 && !(htc_battery_get_zcharge_mode()%2))) {
		led_pwm1[1] = 0;
	}

/* Remove the check first for impact MFG test. Command by adb to set backlight not work */
#if 0
	if (mdp4_overlay_dsi_state_get() <= ST_DSI_SUSPEND) {
		mutex_unlock(&mfd->dma->ov_mutex);
		return;
	}
#endif

	mdp4_dsi_cmd_dma_busy_wait(mfd);
	mdp4_dsi_blt_dmap_busy_wait(mfd);
	mipi_dsi_mdp_busy_wait(mfd);

	if (mfd->bl_level == 0) {
		mipi_dsi_cmds_tx(mfd, &elite_panel_tx_buf, disable_dim, ARRAY_SIZE(disable_dim));
	}
	mipi_dsi_cmds_tx(mfd, &elite_panel_tx_buf, elite_cmd_backlight_cmds,
			elite_cmd_backlight_cmds_count);

	bl_level_prevset = mfd->bl_level;
	mutex_unlock(&mfd->dma->ov_mutex);

	return;
}

static int mipi_lcd_on = 1;

static int elite_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;

	if (panel_type != PANEL_ID_ELITE_SHARP_HX) {
		if (!mipi_lcd_on) {
			mipi_dsi_cmds_tx(mfd, &elite_panel_tx_buf, nvt_LowTemp_wrkr_enter,
				ARRAY_SIZE(nvt_LowTemp_wrkr_enter));

			mipi_dsi_cmds_tx(mfd, &elite_panel_tx_buf, nvt_LowTemp_wrkr_exit,
				ARRAY_SIZE(nvt_LowTemp_wrkr_exit));

			gpio_set_value(ELITE_GPIO_LCD_RSTz, 0);
			hr_msleep(1);
			gpio_set_value(ELITE_GPIO_LCD_RSTz, 1);
			hr_msleep(20);
		}
	}

	if (mipi->mode == DSI_VIDEO_MODE) {
		if(!mipi_lcd_on) {
			if (panel_type == PANEL_ID_ELITE_SONY_NT
					|| panel_type == PANEL_ID_ELITE_SONY_NT_C1
					|| panel_type == PANEL_ID_ELITE_SONY_NT_C2
					|| panel_type == PANEL_ID_ELITE_SHARP_NT
					|| panel_type == PANEL_ID_ELITE_SHARP_NT_C1
					|| panel_type == PANEL_ID_ELITE_SHARP_NT_C2
					|| panel_type == PANEL_ID_ELITE_SHARP_HX) {
				mipi_dsi_cmds_tx(mfd, &elite_panel_tx_buf, elite_video_on_cmds, elite_video_on_cmds_count);
				PR_DISP_INFO("%s: panel_type video mode (%d), %s", __func__, panel_type, ptype);
			} else {
				PR_DISP_INFO("%s: panel_type video mode is not supported!(%d), %s", __func__, panel_type, ptype);
			}
		}
	} else {
		if (!mipi_lcd_on) {
			mipi_dsi_cmd_bta_sw_trigger(); /* clean up ack_err_status */
			if (panel_type == PANEL_ID_ELITE_SONY_NT 
					|| panel_type == PANEL_ID_ELITE_SONY_NT_C1
					|| panel_type == PANEL_ID_ELITE_SONY_NT_C2
					|| panel_type == PANEL_ID_ELITE_SHARP_NT
					|| panel_type == PANEL_ID_ELITE_SHARP_NT_C1
					|| panel_type == PANEL_ID_ELITE_SHARP_NT_C2
					|| panel_type == PANEL_ID_ELITE_SHARP_HX) {
				mipi_dsi_cmds_tx(mfd, &elite_panel_tx_buf, elite_video_on_cmds, elite_video_on_cmds_count);
				PR_DISP_INFO("%s, panel_type command mode (%d)", ptype, panel_type);
			} else {
				PR_DISP_INFO("%s: panel_type command mode is not supported!(%d)", __func__, panel_type);
			}
		}
		mipi_dsi_cmd_bta_sw_trigger(); /* clean up ack_err_status */
		mipi_elite_manufacture_id(mfd);
	}

	mipi_lcd_on = 1;

	return 0;
}


static int elite_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (!mipi_lcd_on)
		return 0;

	if (panel_type != PANEL_ID_NONE) {
		PR_DISP_INFO("%s\n", ptype);
		mipi_dsi_cmds_tx(mfd, &elite_panel_tx_buf, elite_display_off_cmds,
			elite_display_off_cmds_count);
	}

	bl_level_prevset = 0;
	mipi_lcd_on = 0;

	return 0;
}



static void elite_set_backlight(struct msm_fb_data_type *mfd)
{

	int bl_level;

	bl_level = mfd->bl_level;
	if (!mfd->panel_power_on)
		return;

	mipi_dsi_set_backlight(mfd);

}

static int __devinit elite_lcd_probe(struct platform_device *pdev)
{
#if defined CONFIG_FB_MSM_SELF_REFRESH
	int ret;
	PR_DISP_DEBUG("[SR]%s pdev->id=%d\n", __func__, pdev->id);
	mipi_elite_pdata = pdev->dev.platform_data;

	ret = setup_vsync(pdev, 1);
	if (ret) {
		dev_err(&pdev->dev, "mipi_elite_setup_vsync failed\n");
		return ret;
	}
#endif
	msm_fb_add_device(pdev);

	return 0;
}


static int mipi_dsi_panel_power(int on);

static void elite_lcd_shutdown(struct platform_device *pdev)
{
	mipi_dsi_panel_power(0);
}


static struct platform_driver this_driver = {
	.probe  = elite_lcd_probe,
	.shutdown = elite_lcd_shutdown,
	.driver = {
		.name   = "mipi_elite",
	},
};

static struct msm_fb_panel_data elite_panel_data = {
	.on		= elite_lcd_on,
	.off		= elite_lcd_off,
	.set_backlight = elite_set_backlight,
#if defined CONFIG_FB_MSM_SELF_REFRESH
	.self_refresh_switch = elite_self_refresh_switch,
#endif
	.display_on = elite_display_on,
#ifdef CONFIG_FB_MSM_CABC
	.enable_cabc = enable_ic_cabc,
#endif
};

static int ch_used[3];

int mipi_elite_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_elite", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	elite_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &elite_panel_data,
		sizeof(elite_panel_data));
	if (ret) {
		PR_DISP_ERR("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		PR_DISP_ERR("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}


static struct msm_panel_info pinfo;

static struct mipi_dsi_phy_ctrl mipi_dsi_sony_panel_id28103_phy_ctrl_720p = {
	/* DSI_BIT_CLK at 569MHz, 3 lane, RGB888 */
	/* regulator *//* off=0x0500 */
	{0x03, 0x08, 0x05, 0x00, 0x20},
	/* timing *//* off=0x0440 */
	{0x9B, 0x38, 0x18, 0x00, 0x4B, 0x51, 0x1C,
	0x3B, 0x29, 0x03, 0x04, 0xA0},
	/* phy ctrl *//* off=0x0470 */
	{0x5F, 0x00, 0x00, 0x10},
	/* strength *//* off=0x0480 */
	{0xFF, 0x00, 0x06, 0x00},
	/* pll control *//* off=0x0204 */
	{0x0, 0x38, 0x32, 0xDA, 0x00, 0x10, 0x0F, 0x61,
	0x41, 0x0F, 0x01,
	0x00, 0x1A, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x02 },

#if 0
	/* DSI_BIT_CLK at 660MHz, 3 lane, RGB888 */
	/* regulator *//* off=0x0500 */
	{0x03, 0x0A, 0x04, 0x00, 0x20},
	/* timing *//* off=0x0440 */
	{0xAF, 0x3C, 0x1C, 0x00, 0x54, 0x5E, 0x21,
	0x40, 0x2F, 0x03, 0x04, 0xA0},
	/* phy ctrl *//* off=0x0470 */
	{0x5f, 0x00, 0x00, 0x10},
	/* strength *//* off=0x0480 */
	{0xFF, 0x00, 0x06, 0x00},
	/* pll control *//* off=0x0204 */
	{0x0, 0x49, 0xB1, 0xDA, 0x00, 0x50, 0x48, 0x63,
	0x40, 0x07, 0x00,
	0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
#endif
};


static struct mipi_dsi_phy_ctrl mipi_dsi_sharp_panel_idA1B100_phy_ctrl_720p = {
	/* DSI_BIT_CLK at 569MHz, 3 lane, RGB888 */
	/* regulator *//* off=0x0500 */
	{0x03, 0x08, 0x05, 0x00, 0x20},
	/* timing *//* off=0x0440 */
	{0x9B, 0x38, 0x18, 0x00, 0x4B, 0x51, 0x1C,
	0x3B, 0x29, 0x03, 0x04, 0xA0},
	/* phy ctrl *//* off=0x0470 */
	{0x5F, 0x00, 0x00, 0x10},
	/* strength *//* off=0x0480 */
	{0xFF, 0x00, 0x06, 0x00},
	/* pll control *//* off=0x0204 */
	{0x0, 0x38, 0x32, 0xDA, 0x00, 0x10, 0x0F, 0x61,
	0x41, 0x0F, 0x01,
	0x00, 0x1A, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x02 },

#if 0
	/* DSI_BIT_CLK at 660MHz, 3 lane, RGB888 */
	/* regulator *//* off=0x0500 */
	{0x03, 0x0A, 0x04, 0x00, 0x20},
	/* timing *//* off=0x0440 */
	{0xAF, 0x3C, 0x1C, 0x00, 0x54, 0x5E, 0x21,
	0x40, 0x2F, 0x03, 0x04, 0xA0},
	/* phy ctrl *//* off=0x0470 */
	{0x5f, 0x00, 0x00, 0x10},
	/* strength *//* off=0x0480 */
	{0xFF, 0x00, 0x06, 0x00},
	/* pll control *//* off=0x0204 */
	{0x0, 0x49, 0xB1, 0xDA, 0x00, 0x50, 0x48, 0x63,
	0x40, 0x07, 0x00,
	0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
#endif
};

static struct mipi_dsi_phy_ctrl phy_ctrl_720p_id311100 = {
	/* DSI_BIT_CLK at 548MHz, 3 lane, RGB888 */
	/* regulator *//* off=0x0500 */
	{0x03, 0x0A, 0x04, 0x00, 0x20},
	/* timing *//* off=0x0440 */
	{0x96, 0x36, 0x17, 0x00, 0x4A, 0x54, 0x1B,
	0x39, 0x27, 0x03, 0x04, 0xA0},
	/* phy ctrl *//* off=0x0470 */
	{0x5f, 0x00, 0x00, 0x10},
	/* strength *//* off=0x0480 */
	{0xFF, 0x00, 0x06, 0x00},
	/* pll control *//* off=0x0204 */
	{0x0, 0x11, 0xB1, 0xDA, 0x00, 0x50, 0x48, 0x63,
	0x40, 0x07, 0x00,
	0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
};

#if 0
static struct mipi_dsi_phy_ctrl mipi_dsi_sharp_panel_video_mode_two_lane__phy_ctrl_720p = {
		/* regulator */
		{0x03, 0x0a, 0x04, 0x00, 0x20},
		/* timing */
		{0xab, 0x8a, 0x18, 0x00, 0x92,
		0x97, 0x1b, 0x8c, 0x0c,	0x03,
		0x04, 0xa0},
		/* phy ctrl */
		{0x5f, 0x00, 0x00, 0x10},
		/* strength */
		{0xff, 0x00, 0x06, 0x00},
		/* pll control */
		{0x00, 0xf9, 0xb0, 0xda, 0x00,
		0x50, 0x48, 0x63,
		0x30, 0x07, 0x03,
		0x00, 0x14, 0x03, 0x00, 0x02,
		0x00, 0x20, 0x00, 0x01},
};
#endif
static int mipi_video_sony_hd720p_init(void)
{
	int ret;

	PR_DISP_INFO("%s: enter mipi_video_sony init.\n", __func__);

	/* 1:VIDEO MODE, 0:CMD MODE */
#ifdef EVA_CMD_MODE_PANEL
		PR_DISP_INFO("%s: CMD mode (AL)\n", __func__);
		pinfo.type = MIPI_CMD_PANEL;
		pinfo.mipi.mode = DSI_CMD_MODE;
		pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
		/*pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;*/
		pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
#ifdef CONFIG_FB_MSM_SELF_REFRESH
		elite_panel_data.self_refresh_switch = NULL; /* CMD or VIDEO mode only */
#endif
		pinfo.lcd.vsync_enable = TRUE;
		pinfo.lcd.hw_vsync_mode = TRUE;
		pinfo.lcd.refx100 = 6096; /* adjust refx100 to prevent tearing */
		pinfo.mipi.te_sel = 1; /* TE from vsycn gpio */
		pinfo.mipi.interleave_max = 1;
		pinfo.mipi.insert_dcs_cmd = TRUE;
		pinfo.mipi.wr_mem_continue = 0x3c;
		pinfo.mipi.wr_mem_start = 0x2c;

#else
		pinfo.type = MIPI_VIDEO_PANEL;
		pinfo.mipi.mode = DSI_VIDEO_MODE;
		pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
		pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
#ifdef CONFIG_FB_MSM_SELF_REFRESH
		PR_DISP_INFO("%s: VIDEO mode (AL)\n", __func__);
		elite_panel_data.self_refresh_switch = NULL; /* CMD or VIDEO mode only */
#else
		PR_DISP_INFO("%s: SWITCH mode (AL)\n", __func__);
#endif

		pinfo.mipi.pulse_mode_hsa_he = TRUE;
		pinfo.mipi.hfp_power_stop = TRUE;
		pinfo.mipi.hbp_power_stop = TRUE;
		pinfo.mipi.hsa_power_stop = TRUE;
		pinfo.mipi.eof_bllp_power_stop = TRUE;
		pinfo.mipi.bllp_power_stop = TRUE;
		pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_PULSE;

#endif


	pinfo.xres = 720;
	pinfo.yres = 1280;

	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 58;
	pinfo.height = 103;

	pinfo.lcdc.h_back_porch = 104;
	pinfo.lcdc.h_front_porch = 95;
	pinfo.lcdc.h_pulse_width = 1;
	pinfo.lcdc.v_back_porch = 2;
	pinfo.lcdc.v_front_porch = 6;
	pinfo.lcdc.v_pulse_width = 1;

	pinfo.lcd.v_back_porch = 2;
	pinfo.lcd.v_front_porch = 6;
	pinfo.lcd.v_pulse_width = 1;

	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;

	pinfo.clk_rate = 569000000;

	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x10;
	pinfo.mipi.t_clk_pre = 0x21;
	pinfo.mipi.stream = 0;	/* dma_p */

	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &mipi_dsi_sony_panel_id28103_phy_ctrl_720p;

	ret = mipi_elite_device_register(&pinfo, MIPI_DSI_PRIM, MIPI_DSI_PANEL_WVGA_PT);
	if (ret)
		PR_DISP_ERR("%s: failed to register device!\n", __func__);

	if (panel_type == PANEL_ID_ELITE_SONY_NT) {
		strcat(ptype, "PANEL_ID_ELITE_SONY_NT");
		PR_DISP_INFO("%s: assign initial setting for SONY_NT id 0x18103 Cut1, %s\n", __func__, ptype);
		elite_video_on_cmds = sony_panel_video_mode_cmds_id18103_ver008;
		elite_video_on_cmds_count = ARRAY_SIZE(sony_panel_video_mode_cmds_id18103_ver008);
	} else if (panel_type == PANEL_ID_ELITE_SONY_NT_C1) {
		strcat(ptype, "PANEL_ID_ELITE_SONY_NT_C1");
		PR_DISP_INFO("%s: assign initial setting for SONY_NT id 0x28103 Cut1, %s\n", __func__, ptype);
		elite_video_on_cmds = sony_panel_video_mode_cmds_id28103_ver008;
		elite_video_on_cmds_count = ARRAY_SIZE(sony_panel_video_mode_cmds_id28103_ver008);
	} else if (panel_type == PANEL_ID_ELITE_SONY_NT_C2) {
		strcat(ptype, "PANEL_ID_ELITE_SONY_NT_C2");
		PR_DISP_INFO("%s: assign initial setting for SONY_NT Cut2, %s\n", __func__, ptype);
		elite_video_on_cmds = sony_panel_video_mode_cmds_c2;
		elite_video_on_cmds_count = ARRAY_SIZE(sony_panel_video_mode_cmds_c2);
	}

	elite_display_off_cmds	= sony_display_off_cmds;
	elite_cmd_backlight_cmds = sony_cmd_backlight_cmds;
#if defined CONFIG_FB_MSM_SELF_REFRESH
	elite_video_to_cmd = video_to_cmd;
	elite_cmd_to_video = cmd_to_video;
#endif

	elite_display_off_cmds_count = ARRAY_SIZE(sony_display_off_cmds);
	elite_cmd_backlight_cmds_count = ARRAY_SIZE(sony_cmd_backlight_cmds);
#if defined CONFIG_FB_MSM_SELF_REFRESH
	elite_video_to_cmd_cnt	= ARRAY_SIZE(video_to_cmd);
	elite_cmd_to_video_cnt = ARRAY_SIZE(cmd_to_video);
#endif
	return ret;
}

static int __init mipi_video_himax_720p_pt_init(void)
{
	int ret;

	pinfo.xres = 720;
	pinfo.yres = 1280;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 58;
	pinfo.height = 103;

	pinfo.lcdc.h_back_porch = 116;
	pinfo.lcdc.h_front_porch = 62;
	pinfo.lcdc.h_pulse_width = 24;
	pinfo.lcdc.v_back_porch = 4;
	pinfo.lcdc.v_front_porch = 14;
	pinfo.lcdc.v_pulse_width = 2;

	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;

	pinfo.clk_rate = 548000000;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = TRUE;
	pinfo.mipi.hfp_power_stop = TRUE;
	pinfo.mipi.hbp_power_stop = TRUE;
	pinfo.mipi.hsa_power_stop = TRUE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_PULSE;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 10;
	pinfo.mipi.t_clk_pre = 164;
	pinfo.mipi.stream = 0; /* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 57;
	pinfo.mipi.dsi_phy_db = &phy_ctrl_720p_id311100;
#ifdef CONFIG_FB_MSM_SELF_REFRESH
	elite_panel_data.self_refresh_switch = NULL; /* CMD or VIDEO mode only */
#endif

	ret = mipi_elite_device_register(&pinfo, MIPI_DSI_PRIM, MIPI_DSI_PANEL_WVGA_PT);

	if (ret)
		PR_DISP_ERR("%s: failed to register device!\n", __func__);

	strcat(ptype, "PANEL_ID_ELITE_SHARP_HX");
	PR_DISP_INFO("%s: assign initial setting for SHARP_HX, %s\n", __func__, ptype);

	if(0)
	elite_video_on_cmds = himax_video_on_cmds;
	elite_video_on_cmds = himax_video_on_cmds_id311100;
	elite_display_off_cmds	= himax_display_off_cmds;
	elite_cmd_backlight_cmds = himax_cmd_backlight_cmds;
#if defined CONFIG_FB_MSM_SELF_REFRESH
	elite_video_to_cmd = video_to_cmd;
	elite_cmd_to_video = cmd_to_video;
#endif

	if(0)
	elite_video_on_cmds_count = ARRAY_SIZE(himax_video_on_cmds);
	elite_video_on_cmds_count = ARRAY_SIZE(himax_video_on_cmds_id311100);
	elite_display_off_cmds_count = ARRAY_SIZE(himax_display_off_cmds);
	elite_cmd_backlight_cmds_count = ARRAY_SIZE(himax_cmd_backlight_cmds);
#if defined CONFIG_FB_MSM_SELF_REFRESH
	elite_video_to_cmd_cnt	= ARRAY_SIZE(video_to_cmd);
	elite_cmd_to_video_cnt = ARRAY_SIZE(cmd_to_video);
#endif
	return ret;
}


static int __init mipi_video_sharp_nt_720p_pt_init(void)
{
	int ret;

	PR_DISP_INFO("%s: enter mipi_video_sharp_nt init.\n", __func__);

	/* 1:VIDEO MODE, 0:CMD MODE */
#ifdef EVA_CMD_MODE_PANEL
		PR_DISP_INFO("%s: CMD mode (AL)\n", __func__);
		pinfo.type = MIPI_CMD_PANEL;
		pinfo.mipi.mode = DSI_CMD_MODE;
		pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
		/*pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;*/
		pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
#ifdef CONFIG_FB_MSM_SELF_REFRESH
		elite_panel_data.self_refresh_switch = NULL; /* CMD or VIDEO mode only */
#endif
		pinfo.lcd.vsync_enable = TRUE;
		pinfo.lcd.hw_vsync_mode = TRUE;
		pinfo.lcd.refx100 = 6096; /* adjust refx100 to prevent tearing */
		pinfo.mipi.te_sel = 1; /* TE from vsycn gpio */
		pinfo.mipi.interleave_max = 1;
		pinfo.mipi.insert_dcs_cmd = TRUE;
		pinfo.mipi.wr_mem_continue = 0x3c;
		pinfo.mipi.wr_mem_start = 0x2c;

#else
		pinfo.type = MIPI_VIDEO_PANEL;
		pinfo.mipi.mode = DSI_VIDEO_MODE;
		pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
		pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
#ifdef CONFIG_FB_MSM_SELF_REFRESH
		PR_DISP_INFO("%s: VIDEO mode (AL)\n", __func__);
		elite_panel_data.self_refresh_switch = NULL; /* CMD or VIDEO mode only */
#else
		PR_DISP_INFO("%s: SWITCH mode (AL)\n", __func__);
#endif
		pinfo.mipi.pulse_mode_hsa_he = TRUE;
		pinfo.mipi.hfp_power_stop = TRUE;
		pinfo.mipi.hbp_power_stop = TRUE;
		pinfo.mipi.hsa_power_stop = TRUE;
		pinfo.mipi.eof_bllp_power_stop = TRUE;
		pinfo.mipi.bllp_power_stop = TRUE;
		pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_PULSE;

#endif

	pinfo.xres = 720;
	pinfo.yres = 1280;

	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 58;
	pinfo.height = 103;

	pinfo.lcdc.h_back_porch = 125;
	pinfo.lcdc.h_front_porch = 122;
	pinfo.lcdc.h_pulse_width = 1;
	pinfo.lcdc.v_back_porch = 2;
	pinfo.lcdc.v_front_porch = 6;
	pinfo.lcdc.v_pulse_width = 1;

	pinfo.lcd.v_back_porch = 2;
	pinfo.lcd.v_front_porch = 6;
	pinfo.lcd.v_pulse_width = 1;

	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;

	pinfo.clk_rate = 569000000;

	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x10;
	pinfo.mipi.t_clk_pre = 0x21;
	pinfo.mipi.stream = 0; /* dma_p */

	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 57;
	pinfo.mipi.dsi_phy_db = &mipi_dsi_sharp_panel_idA1B100_phy_ctrl_720p;

	ret = mipi_elite_device_register(&pinfo, MIPI_DSI_PRIM, MIPI_DSI_PANEL_WVGA_PT);

	if (ret)
		PR_DISP_ERR("%s: failed to register device!\n", __func__);

	if (panel_type == PANEL_ID_ELITE_SHARP_NT) {
		strcat(ptype, "PANEL_ID_ELITE_SHARP_NT");
		PR_DISP_INFO("%s: assign initial setting(NV1-3) for SHARP_NT id 0xA1B100, %s\n", __func__, ptype);
		elite_video_on_cmds = sharp_nt_video_on_cmds_idA1B100;
		elite_video_on_cmds_count = ARRAY_SIZE(sharp_nt_video_on_cmds_idA1B100);
	} else if (panel_type == PANEL_ID_ELITE_SHARP_NT_C1) {
		strcat(ptype, "PANEL_ID_ELITE_SHARP_NT_C1");
		PR_DISP_INFO("%s: assign initial setting(NV3) for SHARP_NT, %s\n", __func__, ptype);
		elite_video_on_cmds = sharp_nt_video_on_cmds_nv3;
		elite_video_on_cmds_count = ARRAY_SIZE(sharp_nt_video_on_cmds_nv3);
	} else if (panel_type == PANEL_ID_ELITE_SHARP_NT_C2) {
		strcat(ptype, "PANEL_ID_ELITE_SHARP_NT_C2");
		PR_DISP_INFO("%s: assign initial setting(NV4) for SHARP_NT Cut2, %s\n", __func__, ptype);
		elite_video_on_cmds = sharp_nt_video_on_cmds_nv4;
		elite_video_on_cmds_count = ARRAY_SIZE(sharp_nt_video_on_cmds_nv4);
	}

	elite_display_off_cmds	= sony_display_off_cmds;
	elite_cmd_backlight_cmds = sony_cmd_backlight_cmds;
#if defined CONFIG_FB_MSM_SELF_REFRESH
	elite_video_to_cmd = video_to_cmd;
	elite_cmd_to_video = cmd_to_video;
#endif

	elite_display_off_cmds_count = ARRAY_SIZE(sony_display_off_cmds);
	elite_cmd_backlight_cmds_count = ARRAY_SIZE(sony_cmd_backlight_cmds);
#if defined CONFIG_FB_MSM_SELF_REFRESH
	elite_video_to_cmd_cnt	= ARRAY_SIZE(video_to_cmd);
	elite_cmd_to_video_cnt = ARRAY_SIZE(cmd_to_video);
#endif
	return ret;
}

static bool dsi_power_on;

uint32_t cfg_panel_te_active[] = {GPIO_CFG(ELITE_GPIO_LCD_TE, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA)};
uint32_t cfg_panel_te_sleep[] = {GPIO_CFG(ELITE_GPIO_LCD_TE, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA)};


static int mipi_dsi_panel_power(int on)
{
	static struct regulator *v_lcmio, *v_lcm, *v_dsivdd;
	static bool bPanelPowerOn = false;
	int rc;

	char *lcm_str = "8921_l11";
	char *lcmio_str = "8921_lvs5";
	char *dsivdd_str = "8921_l2";

	PR_DISP_INFO("%s: state : %d\n", __func__, on);

	if (!dsi_power_on) {

		v_lcm = regulator_get(&msm_mipi_dsi1_device.dev, lcm_str);
		if (IS_ERR(v_lcm)) {
			PR_DISP_ERR("could not get %s, rc = %ld\n", lcm_str, PTR_ERR(v_lcm));
			return -ENODEV;
		}

		v_dsivdd = regulator_get(&msm_mipi_dsi1_device.dev, dsivdd_str);
		if (IS_ERR(v_dsivdd)) {
			PR_DISP_ERR("could not get %s, rc = %ld\n", dsivdd_str, PTR_ERR(v_dsivdd));
			return -ENODEV;
		}

		if (system_rev == 0) { /*for XA*/
			rc = gpio_request(ELITE_GPIO_V_LCMIO_1V8_EN, "LCMIO_1V8_EN");
			if (rc) {
				PR_DISP_ERR("%s:ELITE_GPIO_V_LCMIO_1V8_EN gpio %d request failed, rc=%d\n", __func__, ELITE_GPIO_V_LCMIO_1V8_EN, rc);
				return -ENODEV;
			}
			gpio_direction_output(ELITE_GPIO_V_LCMIO_1V8_EN, 0);
			/*gpio_free(ELITE_GPIO_V_LCMIO_1V8_EN);*/
		} else {
			v_lcmio = regulator_get(&msm_mipi_dsi1_device.dev, lcmio_str);
			if (IS_ERR(v_lcmio)) {
				PR_DISP_ERR("could not get %s, rc = %ld\n", lcmio_str, PTR_ERR(v_lcmio));
				return -ENODEV;
			}
		}

		if (panel_type == PANEL_ID_ELITE_SHARP_HX)
			rc = regulator_set_voltage(v_lcm, 3200000, 3200000);
		else
			rc = regulator_set_voltage(v_lcm, 3000000, 3000000);
		if (rc) {
			PR_DISP_ERR("%s#%d: set_voltage %s failed, rc=%d\n", __func__, __LINE__, lcm_str, rc);
			return -EINVAL;
		}

		rc = regulator_set_voltage(v_dsivdd, 1200000, 1200000);
		if (rc) {
			PR_DISP_ERR("%s#%d: set_voltage %s failed, rc=%d\n", __func__, __LINE__, dsivdd_str, rc);
			return -EINVAL;
		}

		rc = gpio_request(ELITE_GPIO_LCD_RSTz, "LCM_RST_N");
		if (rc) {
			PR_DISP_ERR("%s:LCM gpio %d request failed, rc=%d\n", __func__,  ELITE_GPIO_LCD_RSTz, rc);
			return -EINVAL;
		}

		dsi_power_on = true;
	}

	if (on) {
		PR_DISP_INFO("%s: on\n", __func__);
		rc = regulator_set_optimum_mode(v_lcm, 100000);
		if (rc < 0) {
			PR_DISP_ERR("set_optimum_mode %s failed, rc=%d\n", lcm_str, rc);
			return -EINVAL;
		}

		rc = regulator_set_optimum_mode(v_dsivdd, 100000);
		if (rc < 0) {
			PR_DISP_ERR("set_optimum_mode %s failed, rc=%d\n", dsivdd_str, rc);
			return -EINVAL;
		}

		rc = regulator_enable(v_dsivdd);
		if (rc) {
			PR_DISP_ERR("enable regulator %s failed, rc=%d\n", dsivdd_str, rc);
			return -ENODEV;
		}

		if (system_rev == 0) { /*for XA*/
			gpio_set_value(ELITE_GPIO_V_LCMIO_1V8_EN, 1);
		} else {
			rc = regulator_enable(v_lcmio);
			if (rc) {
				PR_DISP_ERR("enable regulator %s failed, rc=%d\n", lcmio_str, rc);
				return -ENODEV;
			}
		}

		rc = regulator_enable(v_lcm);
		if (rc) {
			PR_DISP_ERR("enable regulator %s failed, rc=%d\n", lcm_str, rc);
			return -ENODEV;
		}

		elite_lcd_id_power(PM_GPIO_PULL_NO);

		rc = gpio_tlmm_config(cfg_panel_te_active[0], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n", __func__,
					cfg_panel_te_active[0], rc);
		}

		if (panel_type != PANEL_ID_ELITE_SHARP_HX) {
			if (!mipi_lcd_on) {
				hr_msleep(20);
				gpio_set_value(ELITE_GPIO_LCD_RSTz, 1);
				hr_msleep(1);
			}
		} else {
			if (!mipi_lcd_on) {
				hr_msleep(20);
				gpio_set_value(ELITE_GPIO_LCD_RSTz, 1);
				hr_msleep(1);
				gpio_set_value(ELITE_GPIO_LCD_RSTz, 0);
				hr_msleep(1);
				gpio_set_value(ELITE_GPIO_LCD_RSTz, 1);
			}
			hr_msleep(20);
		}

		bPanelPowerOn = true;

	} else {
		PR_DISP_INFO("%s: off\n", __func__);
		if (!bPanelPowerOn) return 0;

		elite_lcd_id_power(PM_GPIO_PULL_DN);

		rc = gpio_tlmm_config(cfg_panel_te_sleep[0], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n", __func__,
					cfg_panel_te_sleep[0], rc);
		}

		gpio_set_value(ELITE_GPIO_LCD_RSTz, 0);
		hr_msleep(10);

		if (regulator_disable(v_lcm)) {
			PR_DISP_ERR("%s: Unable to disable the regulator: %s\n", __func__, lcm_str);
			return -EINVAL;
		}

		if (system_rev == 0) { /*for XA*/
			gpio_set_value(ELITE_GPIO_V_LCMIO_1V8_EN, 0);
		} else {
			if (regulator_disable(v_lcmio)) {
				PR_DISP_ERR("%s: Unable to enable the regulator: %s\n", __func__, lcmio_str);
				return -EINVAL;
			}
		}

		if (regulator_disable(v_dsivdd)) {
			PR_DISP_ERR("%s: Unable to enable the regulator: %s\n", __func__, dsivdd_str);
			return -EINVAL;
		}

		rc = regulator_set_optimum_mode(v_dsivdd, 100);
		if (rc < 0) {
			PR_DISP_ERR("%s: Unable to disable the regulator: %s\n", __func__, dsivdd_str);
			return -EINVAL;
		}

		bPanelPowerOn = false;
	}
	return 0;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio = ELITE_GPIO_LCD_TE,
	.dsi_power_save = mipi_dsi_panel_power,
};

static int __init elite_panel_init(void)
{
	if(panel_type != PANEL_ID_NONE)
		msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
	else
		printk(KERN_INFO "[DISP]panel ID= NONE\n");
	return 0;
}

static int __init elite_panel_late_init(void)
{
	mipi_dsi_buf_alloc(&elite_panel_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&elite_panel_rx_buf, DSI_BUF_SIZE);

	PR_DISP_INFO("%s: enter 0x%x\n", __func__, panel_type);

	if (panel_type == PANEL_ID_ELITE_SONY_NT 
			|| panel_type == PANEL_ID_ELITE_SONY_NT_C1
			|| panel_type == PANEL_ID_ELITE_SONY_NT_C2) {
		mipi_video_sony_hd720p_init();
		PR_DISP_INFO("match PANEL_ID_ELITE_SONY_NT panel_type\n");
	} else if (panel_type == PANEL_ID_ELITE_SHARP_HX) {
		mipi_video_himax_720p_pt_init();
		PR_DISP_INFO("match PANEL_ID_ELITE_SHARP_HX panel_type\n");
	} else if (panel_type == PANEL_ID_ELITE_SHARP_NT
			|| panel_type == PANEL_ID_ELITE_SHARP_NT_C1
			|| panel_type == PANEL_ID_ELITE_SHARP_NT_C2) {
		PR_DISP_INFO("match PANEL_ID_ELITE_SHARP_NT panel_type\n");
		mipi_video_sharp_nt_720p_pt_init();
	} else
		PR_DISP_INFO("Mis-match panel_type\n");
	return platform_driver_register(&this_driver);
}

module_init(elite_panel_init);
late_initcall(elite_panel_late_init);
