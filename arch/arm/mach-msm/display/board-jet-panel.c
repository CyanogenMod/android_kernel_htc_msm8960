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

#include "../devices.h"
#include "../board-jet.h"

/* Select panel operate mode : CMD, VIDEO or SWITCH mode */
#define JEL_CMD_MODE_PANEL
#undef JEL_VIDEO_MODE_PANEL
#undef JEL_SWITCH_MODE_PANEL

static struct dsi_buf jet_panel_tx_buf;
static struct dsi_buf jet_panel_rx_buf;
static struct dsi_cmd_desc *video_on_cmds = NULL;
static struct dsi_cmd_desc *command_on_cmds = NULL;
static struct dsi_cmd_desc *display_off_cmds = NULL;
static struct dsi_cmd_desc *cmd_backlight_cmds = NULL;
#if defined CONFIG_FB_MSM_SELF_REFRESH
static struct dsi_cmd_desc *video_to_cmds = NULL;
static struct dsi_cmd_desc *cmd_to_videos = NULL;
int video_to_cmds_cnt = 0;
int cmd_to_videos_cnt = 0;
#endif
int video_on_cmds_count = 0;
int command_on_cmds_count = 0;
int display_off_cmds_count = 0;
int cmd_backlight_cmds_count = 0;
char ptype[60] = "PANEL type = ";
int bl_level_prevset = 1;

static char enter_sleep[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */
static char exit_sleep[2] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */
static char display_off[2] = {0x28, 0x00}; /* DTYPE_DCS_WRITE */
static char display_on[2] = {0x29, 0x00}; /* DTYPE_DCS_WRITE */

static char led_pwm1[2] = {0x51, 0xFF};	/* DTYPE_DCS_WRITE1 */

static char enable_te[2] = {0x35, 0x00};/* DTYPE_DCS_WRITE1 */

static struct dsi_cmd_desc cmd_bkl_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(led_pwm1), led_pwm1},
};

static char cmd_page_0[2] = {0xff, 0x00};

/*static char set_twolane[2] = {0xBA, 0x01};*/
static char set_threelane[2] = {0xBA, 0x02}; /* DTYPE_DCS_WRITE-1 */

/*static char display_mode_cmd[2] = {0xC2, 0x08};*/
#ifdef JEL_CMD_MODE_PANEL
static char display_mode_cmd[2] = {0xC2, 0x08}; /* DTYPE_DCS_WRITE-1 */
#else
static char display_mode_video[2] = {0xC2, 0x03}; /* DTYPE_DCS_WRITE-1 */
#endif

static char swr01[2] = {0x01, 0x33};
static char swr02[2] = {0x02, 0x53};

static struct dsi_cmd_desc sony_c1_video_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef JEL_CMD_MODE_PANEL
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
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x08}},//PWMDIV=8
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* gamma 2.2 6b setting end */
#endif

#if 1
	/* set Frame rate=60hz */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x05} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
#endif

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep), exit_sleep},

	/* For sony cut1 */
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


	/* {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep}, */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 40, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sony_panel_video_mode_cmds_c2[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef JEL_CMD_MODE_PANEL
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
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x08}},//PWMDIV=8
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* gamma 2.2 6b setting end */
#endif

	/* set Frame rate=60hz */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x05} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },

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
	{DTYPE_DCS_WRITE, 1, 0, 0, 40, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sony_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 100,
		sizeof(enter_sleep), enter_sleep}
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
/* AUO timing fix */
static char auo_fix_000[] = {0xFF, 0x01};
static char auo_fix_001[] = {0xFB, 0x01};
static char auo_fix_002[] = {0x00, 0x4A};
static char auo_fix_003[] = {0x01, 0x44};
static char auo_fix_004[] = {0x02, 0x54};
static char auo_fix_005[] = {0x03, 0x55};
static char auo_fix_006[] = {0x04, 0x55};
static char auo_fix_007[] = {0x05, 0x33};
static char auo_fix_008[] = {0x06, 0x22};
static char auo_fix_009[] = {0x08, 0x56};
static char auo_fix_010[] = {0x09, 0x8F};
static char auo_fix_011[] = {0x0B, 0xAF};
static char auo_fix_012[] = {0x0C, 0xAF};
static char auo_fix_013[] = {0x0D, 0x2F};
static char auo_fix_014[] = {0x0E, 0x1F};
static char auo_fix_015[] = {0x11, 0x84};
static char auo_fix_016[] = {0x12, 0x03};

static char auo_fix_017[] = {0x71, 0x2C};
static char auo_fix_018[] = {0x6F, 0x03};

static char auo_fix_019[] = {0x0F, 0x0A};
static char auo_fix_020[] = {0xFF, 0x05};
static char auo_fix_021[] = {0xFB, 0x01};
static char auo_fix_022[] = {0x01, 0x00};
static char auo_fix_023[] = {0x02, 0x8D};
static char auo_fix_024[] = {0x03, 0x8D};
static char auo_fix_025[] = {0x04, 0x8D};
static char auo_fix_026[] = {0x05, 0x30};
static char auo_fix_027[] = {0x06, 0x33};
static char auo_fix_028[] = {0x07, 0x01};
static char auo_fix_029[] = {0x08, 0x00};
static char auo_fix_030[] = {0x09, 0x30};
static char auo_fix_031[] = {0x0A, 0x30};
static char auo_fix_032[] = {0x0D, 0x11};
static char auo_fix_033[] = {0x0E, 0x1A};
static char auo_fix_034[] = {0x0F, 0x08};
static char auo_fix_035[] = {0x10, 0x53};
static char auo_fix_036[] = {0x11, 0x00};
static char auo_fix_037[] = {0x12, 0x00};
static char auo_fix_038[] = {0x14, 0x01};
static char auo_fix_039[] = {0x15, 0x00};
static char auo_fix_040[] = {0x16, 0x05};
static char auo_fix_041[] = {0x17, 0x08};
static char auo_fix_042[] = {0x19, 0x7F};
static char auo_fix_043[] = {0x1A, 0xFF};
static char auo_fix_044[] = {0x1B, 0x0F};
static char auo_fix_045[] = {0x1C, 0x00};
static char auo_fix_046[] = {0x1D, 0x00};
static char auo_fix_047[] = {0x1E, 0x00};
static char auo_fix_048[] = {0x1F, 0x07};
static char auo_fix_049[] = {0x20, 0x00};
static char auo_fix_050[] = {0x21, 0x00};
static char auo_fix_051[] = {0x22, 0x55};
static char auo_fix_052[] = {0x23, 0x0D};
static char auo_fix_053[] = {0x2D, 0x02};
static char auo_fix_054[] = {0x83, 0x01};
static char auo_fix_055[] = {0x9E, 0x58};
static char auo_fix_056[] = {0x9F, 0x6A};
static char auo_fix_057[] = {0xA0, 0x01};
static char auo_fix_058[] = {0xA2, 0x10};
static char auo_fix_059[] = {0xBB, 0x0A};
static char auo_fix_060[] = {0xBC, 0x0A};

static char auo_fix_061[] = {0x23, 0x4D};
static char auo_fix_062[] = {0x6C, 0x00};
static char auo_fix_063[] = {0x6D, 0x00};

static char auo_fix_064[] = {0x28, 0x00};
static char auo_fix_065[] = {0x2F, 0x00};
static char auo_fix_066[] = {0x32, 0x08};
static char auo_fix_067[] = {0x33, 0xB8};
static char auo_fix_068[] = {0x36, 0x01};
static char auo_fix_069[] = {0x37, 0x00};
static char auo_fix_070[] = {0x43, 0x00};
static char auo_fix_071[] = {0x4B, 0x21};
static char auo_fix_072[] = {0x4C, 0x03};
static char auo_fix_073[] = {0x50, 0x21};
static char auo_fix_074[] = {0x51, 0x03};
static char auo_fix_075[] = {0x58, 0x21};
static char auo_fix_076[] = {0x59, 0x03};
static char auo_fix_077[] = {0x5D, 0x21};
static char auo_fix_078[] = {0x5E, 0x03};

static char sw_wr00[] = {0xFF, 0xEE};
static char sw_wr01[] = {0xFB, 0x01};
static char sw_wr02[] = {0x12, 0x33};
static char sw_wr03[] = {0x13, 0x04};

static struct dsi_cmd_desc auo_panel_video_mode_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_000), auo_fix_000},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_001),	auo_fix_001},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_002),	auo_fix_002},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_003),	auo_fix_003},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_004),	auo_fix_004},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_005),	auo_fix_005},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_006),	auo_fix_006},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_007),	auo_fix_007},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_008),	auo_fix_008},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_009),	auo_fix_009},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_010),	auo_fix_010},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_011),	auo_fix_011},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_012),	auo_fix_012},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_013),	auo_fix_013},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_014),	auo_fix_014},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_015),	auo_fix_015},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_016),	auo_fix_016},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_017),	auo_fix_017},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_018),	auo_fix_018},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_019),	auo_fix_019},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_020),	auo_fix_020},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_021),	auo_fix_021},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_022),	auo_fix_022},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_023),	auo_fix_023},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_024),	auo_fix_024},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_025),	auo_fix_025},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_026),	auo_fix_026},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_027),	auo_fix_027},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_028),	auo_fix_028},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_029),	auo_fix_029},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_030),	auo_fix_030},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_031),	auo_fix_031},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_032),	auo_fix_032},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_033),	auo_fix_033},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_034),	auo_fix_034},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_035),	auo_fix_035},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_036),	auo_fix_036},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_037),	auo_fix_037},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_038),	auo_fix_038},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_039),	auo_fix_039},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_040),	auo_fix_040},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_041),	auo_fix_041},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_042),	auo_fix_042},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_043),	auo_fix_043},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_044),	auo_fix_044},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_045),	auo_fix_045},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_046),	auo_fix_046},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_047),	auo_fix_047},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_048),	auo_fix_048},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_049),	auo_fix_049},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_050),	auo_fix_050},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_051),	auo_fix_051},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_052),	auo_fix_052},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_053),	auo_fix_053},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_054),	auo_fix_054},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_055),	auo_fix_055},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_056),	auo_fix_056},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_057),	auo_fix_057},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_058),	auo_fix_058},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_059),	auo_fix_059},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_060),	auo_fix_060},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_061),	auo_fix_061},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_062),	auo_fix_062},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_063),	auo_fix_063},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_064),	auo_fix_064},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_065),	auo_fix_065},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_066),	auo_fix_066},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_067),	auo_fix_067},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_068),	auo_fix_068},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_069),	auo_fix_069},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_070),	auo_fix_070},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_071),	auo_fix_071},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_072),	auo_fix_072},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_073),	auo_fix_073},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_074),	auo_fix_074},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_075),	auo_fix_075},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_076),	auo_fix_076},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_077),	auo_fix_077},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_078),	auo_fix_078},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd_page_0), cmd_page_0},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef JEL_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif

#if 1
	/* vivi color ver 2 */

	// enable smart color
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x66}},

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

	// add smart color
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x06}},  // add redden
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x00}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
#endif

#if 1
	/* New gamma setting start */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr01), swr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr02), swr02},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0x4B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0x8C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0x92}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xA5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xB7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0xC6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0xD4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0x29}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0xA2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0xE9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x25}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x27}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0x5D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0x93}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0xB6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0xE6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0x08}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x37}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x5A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0x6A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0x75}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0x8D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xA2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0xB9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0x4B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0x8C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0x92}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xA5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xB7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0xC6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0xD4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0x29}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0xA2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0xE9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x25}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x27}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0x5D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0x93}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0xB6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0xE6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0x08}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x37}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x5A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0x6A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0x75}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0x8D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xA2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEC, 0xB9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xED, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEE, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF0, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF2, 0xD4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF4, 0xE0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF6, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF8, 0xFA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFA, 0x05}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x00, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x01, 0x10}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x1A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x24}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x06, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x07, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x64}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0B, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0xBD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0xFD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x10, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x11, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x14, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x15, 0x61}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x16, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x17, 0x94}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x18, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x19, 0xB5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1A, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0xE2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x2C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x38}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x24, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x56}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x27, 0x66}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x29, 0x7C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2A, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2B, 0x8D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2F, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x30, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x31, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x32, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x33, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x34, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x35, 0xD4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x36, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x37, 0xE0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x38, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3A, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3B, 0xFA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3D, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3F, 0x05}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x40, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x41, 0x10}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x42, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x43, 0x1A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x44, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x45, 0x24}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x47, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x48, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x49, 0x64}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4B, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4D, 0xBD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4F, 0xFD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x50, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x51, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x52, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x54, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55, 0x61}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x56, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x58, 0x94}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x59, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5A, 0xB5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5C, 0xE2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5E, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x60, 0x2C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x61, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x62, 0x38}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x63, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x64, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x65, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x66, 0x56}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x67, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x68, 0x66}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x69, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x7C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6C, 0x8D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6E, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x70, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x71, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x72, 0x9B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x73, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x74, 0xA1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0xB3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0xC1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0xCF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0xDB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0xE7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xF1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0x3F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x81}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0xF0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0x2B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x63}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0xBB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0xE8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0x0B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0x40}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x4C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x53}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x5C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0x5A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0x5F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0x9B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0xA1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0xB3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0xC1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0xCF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0xDB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0xE7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xF1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0x3F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x81}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0xF0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0x2B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x63}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0xBB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0xE8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0x0B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0x40}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x4C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x53}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x5C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0x5A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0x5F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xCB}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x08}},//PWMDIV=8
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* New gamma setting end */

#endif

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep), exit_sleep},

	/* for cut 1 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sw_wr00), sw_wr00},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sw_wr01), sw_wr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sw_wr02), sw_wr02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sw_wr03), sw_wr03},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd_page_0), cmd_page_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x33}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0x34}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x00}},

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


	/* {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep}, */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 40, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc auo_panel_video_mode_cmds_c2[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef JEL_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif

#if 1
	/* vivi color ver 2 */

	// enable smart color
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x66}},

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

	// add smart color
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x06}},  // add redden
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x00}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
#endif

#if 1
	/* New gamma setting start */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr01), swr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr02), swr02},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0x4B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0x8C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0x92}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xA5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xB7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0xC6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0xD4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0x29}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0xA2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0xE9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x25}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x27}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0x5D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0x93}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0xB6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0xE6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0x08}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x37}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x5A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0x6A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0x75}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0x8D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xA2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0xB9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0x4B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0x8C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0x92}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xA5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xB7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0xC6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0xD4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0x29}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0xA2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0xE9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x25}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x27}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0x5D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0x93}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0xB6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0xE6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0x08}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x37}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x5A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0x6A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0x75}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0x8D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xA2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEC, 0xB9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xED, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEE, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF0, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF2, 0xD4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF4, 0xE0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF6, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF8, 0xFA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFA, 0x05}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x00, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x01, 0x10}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x1A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x24}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x06, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x07, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x64}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0B, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0xBD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0xFD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x10, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x11, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x14, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x15, 0x61}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x16, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x17, 0x94}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x18, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x19, 0xB5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1A, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0xE2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x2C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x38}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x24, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x56}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x27, 0x66}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x29, 0x7C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2A, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2B, 0x8D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2F, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x30, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x31, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x32, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x33, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x34, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x35, 0xD4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x36, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x37, 0xE0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x38, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3A, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3B, 0xFA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3D, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3F, 0x05}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x40, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x41, 0x10}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x42, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x43, 0x1A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x44, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x45, 0x24}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x47, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x48, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x49, 0x64}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4B, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4D, 0xBD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4F, 0xFD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x50, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x51, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x52, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x54, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55, 0x61}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x56, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x58, 0x94}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x59, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5A, 0xB5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5C, 0xE2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5E, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x60, 0x2C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x61, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x62, 0x38}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x63, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x64, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x65, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x66, 0x56}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x67, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x68, 0x66}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x69, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x7C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6C, 0x8D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6E, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x70, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x71, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x72, 0x9B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x73, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x74, 0xA1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0xB3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0xC1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0xCF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0xDB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0xE7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xF1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0x3F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x81}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0xF0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0x2B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x63}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0xBB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0xE8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0x0B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0x40}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x4C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x53}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x5C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0x5A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0x5F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0x9B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0xA1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0xB3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0xC1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0xCF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0xDB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0xE7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xF1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0x3F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x81}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0xF0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0x2B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x63}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0xBB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0xE8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0x0B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0x40}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x4C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x53}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x5C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0x5A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0x5F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xCB}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x08}},//PWMDIV=8
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* New gamma setting end */
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
	{DTYPE_DCS_WRITE, 1, 0, 0, 40, sizeof(display_on), display_on},
};
static struct dsi_cmd_desc auo_panel_video_mode_cmds_c3[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef JEL_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif

#if 1
	/* vivi color ver 2 */

	// enable smart color
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x66}},

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

	// add smart color
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x06}},  // add redden
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x00}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
#endif

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x08}},//PWMDIV=8
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},

	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep), exit_sleep},

	/* For random dot noise */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
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
	{DTYPE_DCS_WRITE, 1, 0, 0, 40, sizeof(display_on), display_on},
};
static struct dsi_cmd_desc auo_panel_video_mode_cmds_c3_1[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef JEL_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif

#if 1
	/* vivi color ver 2 */

	// enable smart color
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x66}},

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

	// add smart color
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x06}},  // add redden
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x00}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
#endif

#if 1
	/* New gamma setting start */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr01), swr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr02), swr02},


	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0x7D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0x8A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0xB1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0xBF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xCF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xDD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0xE8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0xF2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x1F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0x41}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0x78}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0xA5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x29}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x2A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0x5D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0x93}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0xB8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0xE7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0x07}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x37}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x56}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0x66}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0x7A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0x93}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xA3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0xB4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0x7D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0x8A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0xB1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0xBF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xCF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xDD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0xE8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0xF2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x1F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0x41}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0x78}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0xA5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x29}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x2A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0x5D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0x93}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0xB8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0xE7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0x07}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x37}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x56}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0x66}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0x7A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0x93}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xA3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEC, 0xB4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xED, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEE, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF0, 0xED}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF2, 0xF3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF4, 0xFE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF6, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF8, 0x13}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFA, 0x1D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x00, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x01, 0x26}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x2F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x37}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x06, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x07, 0x56}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0B, 0x9D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0xC2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x10, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x11, 0x31}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x14, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x15, 0x60}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x16, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x17, 0x94}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x18, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x19, 0xB5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1A, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0xE3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x2D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x3A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x48}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x24, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x57}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x27, 0x68}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x29, 0x7B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2A, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2B, 0x90}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2D, 0x03}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2F, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x30, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x31, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x32, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x33, 0xED}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x34, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x35, 0xF3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x36, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x37, 0xFE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x38, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3B, 0x13}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3D, 0x01}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3F, 0x1D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x40, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x41, 0x26}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x42, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x43, 0x2F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x44, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x45, 0x37}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x47, 0x56}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x48, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x49, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4B, 0x9D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4D, 0xC2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4F, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x50, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x51, 0x31}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x52, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x54, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55, 0x60}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x56, 0x02}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x58, 0x94}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x59, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5A, 0xB5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5C, 0xE3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5E, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x60, 0x2D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x61, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x62, 0x3A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x63, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x64, 0x48}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x65, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x66, 0x57}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x67, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x68, 0x68}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x69, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x7B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6C, 0x90}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6E, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x70, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x71, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x72, 0x19}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x73, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x74, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0x55}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0x83}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0x99}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0xA8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xB7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xC5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0xF7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0x1E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x60}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0xE1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0x23}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x59}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x94}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0xB4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0xE1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0x28}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x37}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x3B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x40}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0x50}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0x6D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0x80}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0x19}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0x55}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0x83}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0x99}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0xA8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xB7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xC5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0xF7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0x1E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x60}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0xE1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0x23}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x59}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x94}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0xB4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0xE1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0x28}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x37}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x3B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x40}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0x50}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0x6D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0x80}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xCB}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* New gamma setting end */
#endif
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x08}},//PWMDIV=8
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},

	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep), exit_sleep},

	/* For random dot noise */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
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
	{DTYPE_DCS_WRITE, 1, 0, 0, 40, sizeof(display_on), display_on},
};

#if defined CONFIG_FB_MSM_SELF_REFRESH
#define DSI_VIDEO_BASE	0xE0000
static struct msm_panel_common_pdata *mipi_jet_pdata;
static char display_mode_video_cmd_1[2] = {0xC2, 0x0B}; /* DTYPE_DCS_WRITE */
static char display_mode_video_cmd_2[2] = {0xC2, 0x00}; /* DTYPE_DCS_WRITE */

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

static DECLARE_WAIT_QUEUE_HEAD(jet_vsync_wait);
static unsigned int vsync_irq;
static int wait_vsync = 0;
static int jet_vsync_gpio = 0;
static int jet_irq_cnt = 0;    /* irq trigger trace */

static irqreturn_t jet_vsync_interrupt(int irq, void *data)
{
	jet_irq_cnt++;
	if (wait_vsync) {
		PR_DISP_DEBUG("Wait vsync\n");
		jet_vsync_gpio = 1;
		wake_up(&jet_vsync_wait);
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
	if (ret)
		goto err_request_gpio_failed;

	ret = gpio_direction_input(gpio);
	if (ret)
		goto err_gpio_direction_input_failed;

	ret = vsync_irq = gpio_to_irq(gpio);
	if (ret < 0)
		goto err_get_irq_num_failed;

	ret = request_irq(vsync_irq, jet_vsync_interrupt, IRQF_TRIGGER_RISING,
			  "vsync", pdev);
	if (ret)
		goto err_request_irq_failed;
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
err_request_gpio_failed:
	return ret;
}
#endif
static char disable_dim_cmd[2] = {0x53, 0x24};/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc disable_dim[] = {{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(disable_dim_cmd), disable_dim_cmd},};
#ifdef CONFIG_FB_MSM_CABC
static struct dsi_cmd_desc *dim_cmds = disable_dim; /* default disable dim */
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
		mipi_dsi_mdp_busy_wait();
	}
	/*mipi_dsi_cmds_tx(mfd, &jet_panel_tx_buf, cabc_cmds, ARRAY_SIZE(cabc_on_ui));*/
	mipi_dsi_cmds_tx(&jet_panel_tx_buf, dim_cmds, ARRAY_SIZE(enable_dim));
	PR_DISP_INFO("%s: enable dimming and cabc\n", __func__);

	mutex_unlock(&mfd->dma->ov_mutex);
}
#endif
#define PWM_MIN                   6
#define PWM_DEFAULT               91
#define PWM_MAX                   255

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 143
#define BRI_SETTING_MAX                 255

static unsigned char jet_shrink_pwm(int val)
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
static void jet_self_refresh_switch(int on)
{
	int vsync_timeout;

	if (on) {
		PR_DISP_DEBUG("[SR] %s on\n", __func__);
		mipi_set_tx_power_mode(0);
		mipi_dsi_cmds_tx(&jet_panel_tx_buf, video_to_cmds,
			video_to_cmds_cnt);
		mipi_set_tx_power_mode(1);
		disable_video_mode_clk();
	} else {
		PR_DISP_DEBUG("[SR] %s off\n", __func__);
		jet_irq_cnt = 0;
		mipi_set_tx_power_mode(0);
		enable_irq(vsync_irq);
		wait_vsync = 1;
		vsync_timeout = wait_event_timeout(jet_vsync_wait, jet_vsync_gpio ||
					gpio_get_value(0), HZ/2);

		jet_vsync_gpio = 0;
		mipi_dsi_cmds_tx(&jet_panel_tx_buf, cmd_to_videos,
			cmd_to_videos_cnt);
		wait_vsync = 1;
		vsync_timeout = wait_event_timeout(jet_vsync_wait, jet_vsync_gpio ||
					gpio_get_value(0), HZ/2);
		if (jet_irq_cnt > 2)
			PR_DISP_DEBUG("[SR] vsync_count:%d\n", jet_irq_cnt);
		mipi_dsi_sw_reset();
		enable_video_mode_clk();
		disable_irq(vsync_irq);
		jet_vsync_gpio = 0;
		if (vsync_timeout == 0)
			PR_DISP_DEBUG("Lost vsync!\n");
	}
}
#endif

static int mipi_lcd_on = 1;

static int jet_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;

	if (!mipi_lcd_on) {
		mipi_dsi_cmds_tx(&jet_panel_tx_buf, nvt_LowTemp_wrkr_enter,
			ARRAY_SIZE(nvt_LowTemp_wrkr_enter));

		mipi_dsi_cmds_tx(&jet_panel_tx_buf, nvt_LowTemp_wrkr_exit,
			ARRAY_SIZE(nvt_LowTemp_wrkr_exit));

		gpio_set_value(JET_LCD_RSTz, 0);
		hr_msleep(1);
		gpio_set_value(JET_LCD_RSTz, 1);
		hr_msleep(20);
	}

	if (mipi->mode == DSI_VIDEO_MODE) {
		if (!mipi_lcd_on) {
			if (panel_type != PANEL_ID_NONE) {
				PR_DISP_INFO("%s: %s\n", __func__, ptype);
				mipi_dsi_cmds_tx(&jet_panel_tx_buf, video_on_cmds,
					video_on_cmds_count);
			} else
				PR_DISP_ERR("%s: panel_type is not supported!(%d)", __func__, panel_type);
		}
	} else {
		if (!mipi_lcd_on) {
			if (panel_type != PANEL_ID_NONE) {
				PR_DISP_INFO("%s: %s\n", __func__, ptype);
				mipi_dsi_cmds_tx(&jet_panel_tx_buf, command_on_cmds,
					command_on_cmds_count);
			} else
				PR_DISP_ERR("%s: panel_type is not supported!(%d)", __func__, panel_type);
		}
	}

	mipi_lcd_on = 1;

	return 0;
}

static int jet_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (!mipi_lcd_on)
		return 0;

	/* Remove mutex temporarily*/
	/*mutex_lock(&mfd->dma->ov_mutex);*/
	if (panel_type != PANEL_ID_NONE) {
		PR_DISP_INFO("%s: %s\n", __func__, ptype);
		mipi_dsi_cmds_tx(&jet_panel_tx_buf, display_off_cmds,
			display_off_cmds_count);
	} else
		PR_DISP_ERR("%s: panel_type is not supported!(%d)", __func__, panel_type);

	/*mutex_unlock(&mfd->dma->ov_mutex);*/

	bl_level_prevset = 0;
	mipi_lcd_on = 0;

	return 0;
}

static void jet_set_backlight(struct msm_fb_data_type *mfd)
{
	struct mipi_panel_info *mipi;

	if (!mfd->panel_power_on)
		return;

	mipi  = &mfd->panel_info.mipi;

	if (bl_level_prevset == mfd->bl_level)
		return;

	mutex_lock(&mfd->dma->ov_mutex);

	led_pwm1[1] = jet_shrink_pwm(mfd->bl_level);

	mipi_dsi_mdp_busy_wait();

	if (mfd->bl_level == 0) {
		mipi_dsi_cmds_tx(&jet_panel_tx_buf, disable_dim,
				ARRAY_SIZE(disable_dim));
	}
	mipi_dsi_cmds_tx(&jet_panel_tx_buf, cmd_backlight_cmds,
			cmd_backlight_cmds_count);

	mutex_unlock(&mfd->dma->ov_mutex);

	bl_level_prevset = mfd->bl_level;
}

static int __devinit jet_lcd_probe(struct platform_device *pdev)
{
#if defined CONFIG_FB_MSM_SELF_REFRESH
	int ret;
	mipi_jet_pdata = pdev->dev.platform_data;

	ret = setup_vsync(pdev, 1);
	if (ret) {
		dev_err(&pdev->dev, "mipi_jet_setup_vsync failed\n");
		return ret;
	}
#endif
	msm_fb_add_device(pdev);

	return 0;
}

static int mipi_dsi_panel_power(int on);

static void jet_lcd_shutdown(struct platform_device *pdev)
{
	mipi_dsi_panel_power(0);
}

static struct platform_driver this_driver = {
	.probe  = jet_lcd_probe,
	.shutdown = jet_lcd_shutdown,
	.driver = {
		.name   = "mipi_novatek",
	},
};

static struct msm_fb_panel_data jet_panel_data = {
	.on		= jet_lcd_on,
	.off		= jet_lcd_off,
	.set_backlight = jet_set_backlight,
#if defined CONFIG_FB_MSM_SELF_REFRESH
	.self_refresh_switch = jet_self_refresh_switch,
#endif
#ifdef CONFIG_FB_MSM_CABC
	.enable_cabc = enable_ic_cabc,
#endif
};

static int ch_used[3];

int mipi_jet_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_novatek", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	jet_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &jet_panel_data,
		sizeof(jet_panel_data));
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

static struct mipi_dsi_phy_ctrl nova_dsi_video_mode_phy_db = {
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
};

static int mipi_video_auo_hd720p_init(void)
{
	int ret;
#ifdef JEL_CMD_MODE_PANEL
	PR_DISP_INFO("%s: CMD mode (AL)\n", __func__);
	pinfo.type = MIPI_CMD_PANEL;
	pinfo.mipi.mode = DSI_CMD_MODE;
	pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
	/*pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;*/
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
#ifdef CONFIG_FB_MSM_SELF_REFRESH
	jet_panel_data.self_refresh_switch = NULL; /* CMD or VIDEO mode only */
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
	pinfo.type = MIPI_VIDEO_PANEL;/*MIPI_VIDEO_PANEL;*/
	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
#ifdef CONFIG_FB_MSM_SELF_REFRESH
	PR_DISP_INFO("%s: VIDEO mode (AL)\n", __func__);
	jet_panel_data.self_refresh_switch = NULL; /* CMD or VIDEO mode only */
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

	pinfo.lcdc.h_back_porch = 104;	/* 660Mhz: 116 */
	pinfo.lcdc.h_front_porch = 95;	/* 660Mhz: 184 */
	pinfo.lcdc.h_pulse_width = 1;	/* 660Mhz: 24 */
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
	/*pinfo.clk_rate = 742500000;*/
	/*pinfo.clk_rate = 482000000;*/
	pinfo.clk_rate = 569000000;

	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x10;	/* 660Mhz: 10 */
	pinfo.mipi.t_clk_pre = 0x21;	/* 660Mhz: 30 */
	pinfo.mipi.stream = 0;	/* dma_p */
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &nova_dsi_video_mode_phy_db;
	pinfo.mipi.esc_byte_ratio = 4;

	ret = mipi_jet_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_WVGA_PT);
	if (ret)
		PR_DISP_ERR("%s: failed to register device!\n", __func__);

	if (panel_type == PANEL_ID_JET_AUO_NT) {
		strcat(ptype, "PANEL_ID_JET_AUO_NT");
		PR_DISP_INFO("%s: %s", __func__, ptype);
		video_on_cmds = auo_panel_video_mode_cmds;
		video_on_cmds_count = ARRAY_SIZE(auo_panel_video_mode_cmds);
		command_on_cmds = auo_panel_video_mode_cmds;
		command_on_cmds_count = ARRAY_SIZE(auo_panel_video_mode_cmds);
		display_off_cmds	= sony_display_off_cmds;
		display_off_cmds_count = ARRAY_SIZE(sony_display_off_cmds);
	} else if (panel_type == PANEL_ID_JET_AUO_NT_C2) {
		strcat(ptype, "PANEL_ID_JET_AUO_NT_C2");
		PR_DISP_INFO("%s: %s", __func__, ptype);
		video_on_cmds = auo_panel_video_mode_cmds_c2;
		video_on_cmds_count = ARRAY_SIZE(auo_panel_video_mode_cmds_c2);
		command_on_cmds = auo_panel_video_mode_cmds_c2;
		command_on_cmds_count = ARRAY_SIZE(auo_panel_video_mode_cmds_c2);
		display_off_cmds	= sony_display_off_cmds;
		display_off_cmds_count = ARRAY_SIZE(sony_display_off_cmds);
	} else if (panel_type == PANEL_ID_JET_AUO_NT_C3) {
		strcat(ptype, "PANEL_ID_JET_AUO_NT_C3");
		PR_DISP_INFO("%s: %s", __func__, ptype);
		video_on_cmds = auo_panel_video_mode_cmds_c3;
		video_on_cmds_count = ARRAY_SIZE(auo_panel_video_mode_cmds_c3);
		command_on_cmds = auo_panel_video_mode_cmds_c3;
		command_on_cmds_count = ARRAY_SIZE(auo_panel_video_mode_cmds_c3);
		display_off_cmds	= sony_display_off_cmds;
		display_off_cmds_count = ARRAY_SIZE(sony_display_off_cmds);
	} else if (panel_type == PANEL_ID_JET_AUO_NT_C3_1) {
		strcat(ptype, "PANEL_ID_JET_AUO_NT_C3_1");
		PR_DISP_INFO("%s: %s", __func__, ptype);
		video_on_cmds = auo_panel_video_mode_cmds_c3_1;
		video_on_cmds_count = ARRAY_SIZE(auo_panel_video_mode_cmds_c3_1);
		command_on_cmds = auo_panel_video_mode_cmds_c3_1;
		command_on_cmds_count = ARRAY_SIZE(auo_panel_video_mode_cmds_c3_1);
		display_off_cmds	= sony_display_off_cmds;
		display_off_cmds_count = ARRAY_SIZE(sony_display_off_cmds);
	}


	cmd_backlight_cmds = cmd_bkl_cmds;
	cmd_backlight_cmds_count = ARRAY_SIZE(cmd_bkl_cmds);

#if defined CONFIG_FB_MSM_SELF_REFRESH
	video_to_cmds = video_to_cmd;
	cmd_to_videos = cmd_to_video;
	video_to_cmds_cnt	= ARRAY_SIZE(video_to_cmd);
	cmd_to_videos_cnt = ARRAY_SIZE(cmd_to_video);
#endif

	return ret;
}

static int mipi_video_sony_hd720p_init(void)
{
	int ret;

	/* 1:VIDEO MODE, 0:CMD MODE */
#ifdef JEL_CMD_MODE_PANEL
	PR_DISP_INFO("%s: CMD mode (AL)\n", __func__);
	pinfo.type = MIPI_CMD_PANEL;
	pinfo.mipi.mode = DSI_CMD_MODE;
	pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
	/*pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;*/
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
#ifdef CONFIG_FB_MSM_SELF_REFRESH
	jet_panel_data.self_refresh_switch = NULL; /* CMD or VIDEO mode only */
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
	jet_panel_data.self_refresh_switch = NULL; /* CMD or VIDEO mode only */
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
	pinfo.mipi.dsi_phy_db = &nova_dsi_video_mode_phy_db;
	pinfo.mipi.esc_byte_ratio = 4;

	ret = mipi_jet_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_WVGA_PT);
	if (ret)
		PR_DISP_ERR("%s: failed to register device!\n", __func__);

	if (panel_type == PANEL_ID_JET_SONY_NT) {
		strcat(ptype, "PANEL_ID_JET_SONY_NT");
		PR_DISP_INFO("%s: %s", __func__, ptype);
		video_on_cmds = sony_c1_video_on_cmds;
		video_on_cmds_count = ARRAY_SIZE(sony_c1_video_on_cmds);
		command_on_cmds = sony_c1_video_on_cmds;
		command_on_cmds_count = ARRAY_SIZE(sony_c1_video_on_cmds);
		display_off_cmds	= sony_display_off_cmds;
		display_off_cmds_count = ARRAY_SIZE(sony_display_off_cmds);
	} else if (panel_type == PANEL_ID_JET_SONY_NT_C1) {
		strcat(ptype, "PANEL_ID_JET_SONY_NT_C1");
		PR_DISP_INFO("%s: %s", __func__, ptype);
		video_on_cmds = sony_c1_video_on_cmds;
		video_on_cmds_count = ARRAY_SIZE(sony_c1_video_on_cmds);
		command_on_cmds = sony_c1_video_on_cmds;
		command_on_cmds_count = ARRAY_SIZE(sony_c1_video_on_cmds);
		display_off_cmds	= sony_display_off_cmds;
		display_off_cmds_count = ARRAY_SIZE(sony_display_off_cmds);
	} else if (panel_type == PANEL_ID_JET_SONY_NT_C2) {
		strcat(ptype, "PANEL_ID_JET_SONY_NT_C2");
		PR_DISP_INFO("%s: %s", __func__, ptype);
		video_on_cmds = sony_panel_video_mode_cmds_c2;
		video_on_cmds_count = ARRAY_SIZE(sony_panel_video_mode_cmds_c2);
		command_on_cmds = sony_panel_video_mode_cmds_c2;
		command_on_cmds_count = ARRAY_SIZE(sony_panel_video_mode_cmds_c2);
		display_off_cmds	= sony_display_off_cmds;
		display_off_cmds_count = ARRAY_SIZE(sony_display_off_cmds);
	}

	cmd_backlight_cmds = cmd_bkl_cmds;
	cmd_backlight_cmds_count = ARRAY_SIZE(cmd_bkl_cmds);

#if defined CONFIG_FB_MSM_SELF_REFRESH
	video_to_cmds = video_to_cmd;
	cmd_to_videos = cmd_to_video;
	video_to_cmds_cnt	= ARRAY_SIZE(video_to_cmd);
	cmd_to_videos_cnt = ARRAY_SIZE(cmd_to_video);
#endif

	return ret;
}

static bool dsi_power_on;

static int mipi_dsi_panel_power(int on)
{
	static struct regulator *v_lcm, *v_lcmio, *v_dsivdd;
	static bool bPanelPowerOn = false;
	int rc;

	char *lcm_str = "8921_l11";
	char *lcmio_str = "8921_lvs5";
	char *dsivdd_str = "8921_l2";

	PR_DISP_INFO("%s: state : %d\n", __func__, on);

	if (!dsi_power_on) {

		v_lcm = regulator_get(&msm_mipi_dsi1_device.dev,
				lcm_str);
		if (IS_ERR(v_lcm)) {
			PR_DISP_ERR("could not get %s, rc = %ld\n",
				lcm_str, PTR_ERR(v_lcm));
			return -ENODEV;
		}

		v_lcmio = regulator_get(&msm_mipi_dsi1_device.dev,
				lcmio_str);
		if (IS_ERR(v_lcmio)) {
			PR_DISP_ERR("could not get %s, rc = %ld\n",
				lcmio_str, PTR_ERR(v_lcmio));
			return -ENODEV;
		}

		v_dsivdd = regulator_get(&msm_mipi_dsi1_device.dev,
				dsivdd_str);
		if (IS_ERR(v_dsivdd)) {
			PR_DISP_ERR("could not get %s, rc = %ld\n",
				dsivdd_str, PTR_ERR(v_dsivdd));
			return -ENODEV;
		}
		if (system_rev >= 2) /* XC */
			rc = regulator_set_voltage(v_lcm, 3000000, 3000000);
		else
			rc = regulator_set_voltage(v_lcm, 3200000, 3200000);
		if (rc) {
			PR_DISP_ERR("%s#%d: set_voltage %s failed, rc=%d\n", __func__, __LINE__, lcm_str, rc);
			return -EINVAL;
		}

		rc = regulator_set_voltage(v_dsivdd, 1200000, 1200000);
		if (rc) {
			PR_DISP_ERR("%s#%d: set_voltage %s failed, rc=%d\n", __func__, __LINE__, dsivdd_str, rc);
			return -EINVAL;
		}

		rc = gpio_request(JET_LCD_RSTz, "LCM_RST_N");
		if (rc) {
			PR_DISP_ERR("%s:LCM gpio %d request failed, rc=%d\n", __func__,  JET_LCD_RSTz, rc);
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

		rc = regulator_enable(v_lcm);
		if (rc) {
			PR_DISP_ERR("enable regulator %s failed, rc=%d\n", lcm_str, rc);
			return -ENODEV;
		}

		rc = regulator_enable(v_dsivdd);
		if (rc) {
			PR_DISP_ERR("enable regulator %s failed, rc=%d\n", dsivdd_str, rc);
			return -ENODEV;
		}
		rc = regulator_enable(v_lcmio);
		if (rc) {
			PR_DISP_ERR("enable regulator %s failed, rc=%d\n", lcmio_str, rc);
			return -ENODEV;
		}

		jet_lcd_id_power(PM_GPIO_PULL_NO);

		if (!mipi_lcd_on) {
			hr_msleep(20);
			gpio_set_value(JET_LCD_RSTz, 1);
			hr_msleep(1);
		}

		bPanelPowerOn = true;

	} else {
		PR_DISP_INFO("%s: off\n", __func__);
		if (!bPanelPowerOn) return 0;
		jet_lcd_id_power(PM_GPIO_PULL_DN);

		gpio_set_value(JET_LCD_RSTz, 0);
		hr_msleep(10);

		if (regulator_disable(v_lcmio)) {
			PR_DISP_ERR("%s: Unable to enable the regulator: %s\n", __func__, lcmio_str);
			return -EINVAL;
		}

		if (regulator_disable(v_dsivdd)) {
			PR_DISP_ERR("%s: Unable to enable the regulator: %s\n", __func__, dsivdd_str);
			return -EINVAL;
		}

		if (regulator_disable(v_lcm)) {
			PR_DISP_ERR("%s: Unable to enable the regulator: %s\n", __func__, lcm_str);
			return -EINVAL;
		}

		rc = regulator_set_optimum_mode(v_dsivdd, 100);
		if (rc < 0) {
			PR_DISP_ERR("%s: Unable to enable the regulator: %s\n", __func__, dsivdd_str);
			return -EINVAL;
		}

		bPanelPowerOn = false;
	}
	return 0;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio = JET_LCD_TE,
	.dsi_power_save = mipi_dsi_panel_power,
};

static int __init jet_panel_init(void)
{
	 printk(KERN_INFO "%s: panel type = 0x%x, hboot type = 0x%x, board_mfg_mode = 0x%x\n", __func__, panel_type, board_build_flag(), board_mfg_mode());

	if (panel_type == PANEL_ID_NONE) {
		if (board_mfg_mode() == 8 || board_mfg_mode() == 4) {
			PR_DISP_INFO("%s: enter mfgkernel/powettest \n", __func__);
			return 0;
		} else if (board_build_flag() == SHIP_BUILD) {
			/* For shipping produce, the main source panel can cover the most device */
			PR_DISP_INFO("%s: Use PANEL_ID_JET_SONY_NT_C2 be default panel!\n", __func__);
			panel_type = PANEL_ID_JET_SONY_NT_C2;
		} else
			panic("[DISP] PANEL_ID_NONE! panic");
	}

	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);

	return 0;
}

static int __init jet_panel_late_init(void)
{
	mipi_dsi_buf_alloc(&jet_panel_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&jet_panel_rx_buf, DSI_BUF_SIZE);

	if(panel_type == PANEL_ID_NONE)	{
		PR_DISP_INFO("%s panel ID = PANEL_ID_NONE\n", __func__);
		return 0;
	}

	if (panel_type == PANEL_ID_JET_SONY_NT || panel_type == PANEL_ID_JET_SONY_NT_C1 ||
		panel_type == PANEL_ID_JET_SONY_NT_C2)
		mipi_video_sony_hd720p_init();
	else if (panel_type == PANEL_ID_JET_AUO_NT || panel_type ==
			PANEL_ID_JET_AUO_NT_C2 || panel_type == PANEL_ID_JET_AUO_NT_C3 ||
			panel_type == PANEL_ID_JET_AUO_NT_C3_1)
			mipi_video_auo_hd720p_init();

	return platform_driver_register(&this_driver);
}

module_init(jet_panel_init);
late_initcall(jet_panel_late_init);
