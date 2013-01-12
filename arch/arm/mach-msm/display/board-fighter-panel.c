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
#include "../board-fighter.h"

#if defined (CONFIG_FB_MSM_MDP_ABL)
#include <linux/fb.h>
#endif

static struct dsi_buf fighter_panel_tx_buf;
static struct dsi_buf fighter_panel_rx_buf;


static char sw_reset[2] = {0x01, 0x00}; /* DTYPE_DCS_WRITE */
static char enter_sleep[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */
static char exit_sleep[2] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */
static char display_off[2] = {0x28, 0x00}; /* DTYPE_DCS_WRITE */
static char display_on[2] = {0x29, 0x00}; /* DTYPE_DCS_WRITE */

static char rgb_888[2] = {0x3A, 0x77}; /* DTYPE_DCS_WRITE1 */

#define NOVATEK_TWO_LANE

#if defined(NOVATEK_TWO_LANE)
static char set_num_of_lanes[2] = {0xae, 0x03}; /* DTYPE_DCS_WRITE1 */
#else  /* 1 lane */
static char set_num_of_lanes[2] = {0xae, 0x01}; /* DTYPE_DCS_WRITE1 */
#endif

static char led_pwm1[2] = {0x51, 0xF0};	/* DTYPE_DCS_WRITE1 */
static char led_pwm2[2] = {0x53, 0x24}; /* DTYPE_DCS_WRITE1 */
static char led_pwm3[2] = {0x55, 0x00}; /* DTYPE_DCS_WRITE1 */

static char max_pktsize[2] = {MIPI_DSI_MRPS, 0x00}; /* LSB tx first, 16 bytes */

static char novatek_e0[3] = {0xE0, 0x01, 0x03}; /* DTYPE_DCS_LWRITE */

static char enable_te[2] = {0x35, 0x00};/* DTYPE_DCS_WRITE1 */

static bool isorise;

static struct dsi_cmd_desc novatek_cmd_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(led_pwm1), led_pwm1},
};

#if 0 // gamma 2.5
static char fir_lg_gamma_01_d1[] = {
	0xD1,
	0x01, 0x67, 0x01, 0x6E, 0x01, 0x7F, 0x01, 0x8D, 0x01, 0x93, 0x01,
	0xA4, 0x01, 0xAB, 0x01, 0xC7
};

static char fir_lg_gamma_01_d5[] = {
	0xD5,
	0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 0x10, 0x01, 0x21, 0x01,
	0x44, 0x01, 0x62, 0x01, 0x8D
};

static char fir_lg_gamma_01_d9[] = {
	0xD9,
	0x01, 0x67, 0x01, 0x6E, 0x01, 0x7F, 0x01, 0x8D, 0x01, 0x93, 0x01,
	0xA4, 0x01, 0xAB, 0x01, 0xC7
};

static char fir_lg_gamma_01_e0[] = {
	0xE0,
	0x01, 0x67, 0x01, 0x6E, 0x01, 0x7F, 0x01, 0x8D, 0x01, 0x93, 0x01,
	0xA4, 0x01, 0xAB, 0x01, 0xC7
};

static char fir_lg_gamma_01_e4[] = {
	0xE4,
	0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 0x10, 0x01, 0x21, 0x01,
	0x44, 0x01, 0x62, 0x01, 0x8D
};

static char fir_lg_gamma_01_e8[] = {
	0xE8,
	0x01, 0x67, 0x01, 0x6E, 0x01, 0x7F, 0x01, 0x8D, 0x01, 0x93, 0x01,
	0xA4, 0x01, 0xAB, 0x01, 0xC7
};

static char fir_lg_gamma_02_d2[] = {
	0xD2,
	0x01, 0xD9, 0x01, 0xFE, 0x02, 0x1F, 0x02, 0x58, 0x02, 0x87, 0x02,
	0x8A, 0x02, 0xC0, 0x02, 0xFF
};

static char fir_lg_gamma_02_d6[] = {
	0xD6,
	0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 0x4D, 0x02, 0x82, 0x02,
	0x84, 0x02, 0xB8, 0x02, 0xFA
};

static char fir_lg_gamma_02_dd[] = {
	0xDD,
	0x01, 0xD9, 0x01, 0xFE, 0x02, 0x1F, 0x02, 0x58, 0x02, 0x87, 0x02,
	0x8A, 0x02, 0xC0, 0x02, 0xFF
};

static char fir_lg_gamma_02_e1[] = {
	0xE1,
	0x01, 0xD9, 0x01, 0xFE, 0x02, 0x1F, 0x02, 0x58, 0x02, 0x87, 0x02,
	0x8A, 0x02, 0xC0, 0x02, 0xFF
};

static char fir_lg_gamma_02_e5[] = {
	0xE5,
	0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 0x4D, 0x02, 0x82, 0x02,
	0x84, 0x02, 0xB8, 0x02, 0xFA
};

static char fir_lg_gamma_02_e9[] = {
	0xE9,
	0x01, 0xD9, 0x01, 0xFE, 0x02, 0x1F, 0x02, 0x58, 0x02, 0x87, 0x02,
	0x8A, 0x02, 0xC0, 0x02, 0xFF
};

static char fir_lg_gamma_03_d3[] = {
	0xD3,
	0x03, 0x26, 0x03, 0x52, 0x03, 0x6F, 0x03, 0x96, 0x03, 0xA3, 0x03,
	0xAE, 0x03, 0xB6, 0x03, 0xBC
};

static char fir_lg_gamma_03_d7[] = {
	0xD7,
	0x03, 0x20, 0x03, 0x4C, 0x03, 0x68, 0x03, 0x88, 0x03, 0x90, 0x03,
	0x9E, 0x03, 0xA1, 0x03, 0xAA
};

static char fir_lg_gamma_03_de[] = {
	0xDE,
	0x03, 0x26, 0x03, 0x52, 0x03, 0x6F, 0x03, 0x9B, 0x03, 0xB1, 0x03,
	0xC9, 0x03, 0xDF, 0x03, 0xEF
};

static char fir_lg_gamma_03_e2[] = {
	0xE2,
	0x03, 0x26, 0x03, 0x52, 0x03, 0x6F, 0x03, 0x96, 0x03, 0xA3, 0x03,
	0xAE, 0x03, 0xB6, 0x03, 0xBC
};

static char fir_lg_gamma_03_e6[] = {
	0xE6,
	0x03, 0x20, 0x03, 0x4C, 0x03, 0x68, 0x03, 0x88, 0x03, 0x90, 0x03,
	0x9E, 0x03, 0xA1, 0x03, 0xAA
};

static char fir_lg_gamma_03_ea[] = {
	0xEA,
	0x03, 0x26, 0x03, 0x52, 0x03, 0x6F, 0x03, 0x9B, 0x03, 0xB1, 0x03,
	0xC9, 0x03, 0xDF, 0x03, 0xEF
};

static char fir_lg_gamma_04_d4[] = {
	0xD4, 0x03, 0xBE, 0x03, 0xBF
};

static char fir_lg_gamma_04_d8[] = {
	0xD8, 0x03, 0xAE, 0x03, 0xAF
};

static char fir_lg_gamma_04_df[] = {
	0xDF, 0x03, 0xFF, 0x03, 0xFF
};

static char fir_lg_gamma_04_e3[] = {
	0xE3, 0x03, 0xBE, 0x03, 0xBF
};

static char fir_lg_gamma_04_e7[] = {
	0xE7, 0x03, 0xAE, 0x03, 0xAF
};

static char fir_lg_gamma_04_eb[] = {
	0xEB, 0x03, 0xFF, 0x03, 0xFF
};
#endif

/**gamma 2.2***/
static char fir_lg_gamma22_01_d1[] = {
	0xD1,
//	0x01, 0x67, 0x01, 0x6E, 0x01, 0x7F, 0x01, 0x8D, 0x01, 0x93, 0x01,
//	0xA4, 0x01, 0xAB, 0x01, 0xC7
	0x01, 0x98, 0x01, 0xA7, 0x01, 0xB2, 0x01, 0xB7, 0x01, 0xB8, 0x01,
	0xCD, 0x01, 0xCF, 0x01, 0xE3
};

static char fir_lg_gamma22_01_d5[] = {
	0xD5,
//	0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 0x10, 0x01, 0x21, 0x01,
//	0x44, 0x01, 0x62, 0x01, 0x8D
	0x00, 0xF6, 0x01, 0x08, 0x01, 0x1F, 0x01, 0x37, 0x01, 0x52, 0x01,
	0x70, 0x01, 0x85, 0x01, 0xAA
};

static char fir_lg_gamma22_01_d9[] = {
	0xD9,
//	0x01, 0x67, 0x01, 0x6E, 0x01, 0x7F, 0x01, 0x8D, 0x01, 0x93, 0x01,
//	0xA4, 0x01, 0xAB, 0x01, 0xC7
	0x01, 0x67, 0x01, 0x6E, 0x01, 0x7F, 0x01, 0x8D, 0x01, 0x93, 0x01,
	0xA4, 0x01, 0xAB, 0x01, 0xC7
};

static char fir_lg_gamma22_01_e0[] = {
	0xE0,
	//0x01, 0x67, 0x01, 0x6E, 0x01, 0x7F, 0x01, 0x8D, 0x01, 0x93, 0x01,
	//0xA4, 0x01, 0xAB, 0x01, 0xC7
        0x01, 0x98, 0x01, 0xA7, 0x01, 0xB2, 0x01, 0xB7, 0x01, 0xB8, 0x01,
	0xCD, 0x01, 0xCF, 0x01, 0xE3
};

static char fir_lg_gamma22_01_e4[] = {
	0xE4,
	//0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01, 0x10, 0x01, 0x21, 0x01,
	//0x44, 0x01, 0x62, 0x01, 0x8D
	0x00, 0xF6, 0x01, 0x08, 0x01, 0x1F, 0x01, 0x37, 0x01, 0x52, 0x01,
	0x70, 0x01, 0x85, 0x01, 0xAA
};

static char fir_lg_gamma22_01_e8[] = {
	0xE8,
	//0x01, 0x67, 0x01, 0x6E, 0x01, 0x7F, 0x01, 0x8D, 0x01, 0x93, 0x01,
	//0xA4, 0x01, 0xAB, 0x01, 0xC7
	0x01, 0x67, 0x01, 0x6E, 0x01, 0x7F, 0x01, 0x8D, 0x01, 0x93, 0x01,
	0xA4, 0x01, 0xAB, 0x01, 0xC7
};

static char fir_lg_gamma22_02_d2[] = {
	0xD2,
	//0x01, 0xD9, 0x01, 0xFE, 0x02, 0x1F, 0x02, 0x58, 0x02, 0x87, 0x02,
	//0x8A, 0x02, 0xC0, 0x02, 0xFF
	0x01, 0xF4, 0x02, 0x13, 0x02, 0x31, 0x02, 0x67, 0x02, 0x95, 0x02,
	0x96, 0x02, 0xC8, 0x03, 0x03
};

static char fir_lg_gamma22_02_d6[] = {
	0xD6,
//	0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 0x4D, 0x02, 0x82, 0x02,
//	0x84, 0x02, 0xB8, 0x02, 0xFA
	0x01, 0xC3, 0x01, 0xED, 0x02, 0x14, 0x02, 0x55, 0x02, 0x87, 0x02,
	0x88, 0x02, 0xBD, 0x02, 0xF9
};

static char fir_lg_gamma22_02_dd[] = {
	0xDD,
	0x01, 0xD9, 0x01, 0xFE, 0x02, 0x1F, 0x02, 0x58, 0x02, 0x87, 0x02,
	0x8A, 0x02, 0xC0, 0x02, 0xFF
};

static char fir_lg_gamma22_02_e1[] = {
	0xE1,
	//0x01, 0xD9, 0x01, 0xFE, 0x02, 0x1F, 0x02, 0x58, 0x02, 0x87, 0x02,
	//0x8A, 0x02, 0xC0, 0x02, 0xFF
	0x01, 0xF4, 0x02, 0x13, 0x02, 0x31, 0x02, 0x67, 0x02, 0x95, 0x02,
	0x96, 0x02, 0xC8, 0x03, 0x03
};

static char fir_lg_gamma22_02_e5[] = {
	0xE5,
	//0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02, 0x4D, 0x02, 0x82, 0x02,
	//0x84, 0x02, 0xB8, 0x02, 0xFA
	0x01, 0xC3, 0x01, 0xED, 0x02, 0x14, 0x02, 0x55, 0x02, 0x87, 0x02,
	0x88, 0x02, 0xBD, 0x02, 0xF9
};

static char fir_lg_gamma22_02_e9[] = {
	0xE9,
	0x01, 0xD9, 0x01, 0xFE, 0x02, 0x1F, 0x02, 0x58, 0x02, 0x87, 0x02,
	0x8A, 0x02, 0xC0, 0x02, 0xFF
};

static char fir_lg_gamma22_03_d3[] = {
	0xD3,
	//0x03, 0x26, 0x03, 0x52, 0x03, 0x6F, 0x03, 0x96, 0x03, 0xA3, 0x03,
	//0xAE, 0x03, 0xB6, 0x03, 0xBC
	0x03, 0x2B, 0x03, 0x53, 0x03, 0x6F, 0x03, 0x92, 0x03, 0x9E, 0x03,
	0xAE, 0x03, 0xB3, 0x03, 0xBA
};

static char fir_lg_gamma22_03_d7[] = {
	0xD7,
	//0x03, 0x20, 0x03, 0x4C, 0x03, 0x68, 0x03, 0x88, 0x03, 0x90, 0x03,
	//0x9E, 0x03, 0xA1, 0x03, 0xAA
	0x03, 0x21, 0x03, 0x49, 0x03, 0x64, 0x03, 0x87, 0x03, 0x93, 0x03,
	0xA1, 0x03, 0xA4, 0x03, 0xA5
};

static char fir_lg_gamma22_03_de[] = {
	0xDE,
//	0x03, 0x26, 0x03, 0x52, 0x03, 0x6F, 0x03, 0x9B, 0x03, 0xB1, 0x03,
//	0xC9, 0x03, 0xDF, 0x03, 0xEF
	0x03, 0x26, 0x03, 0x52, 0x03, 0x6F, 0x03, 0x9B, 0x03, 0xB1, 0x03,
	0xC9, 0x03, 0xDF, 0x03, 0xEF
};

static char fir_lg_gamma22_03_e2[] = {
	0xE2,
//	0x03, 0x26, 0x03, 0x52, 0x03, 0x6F, 0x03, 0x96, 0x03, 0xA3, 0x03,
//	0xAE, 0x03, 0xB6, 0x03, 0xBC
	0x03, 0x2B, 0x03, 0x53, 0x03, 0x6F, 0x03, 0x92, 0x03, 0x9E, 0x03,
	0xAE, 0x03, 0xB3, 0x03, 0xBA
};

static char fir_lg_gamma22_03_e6[] = {
	0xE6,
	//0x03, 0x20, 0x03, 0x4C, 0x03, 0x68, 0x03, 0x88, 0x03, 0x90, 0x03,
	//0x9E, 0x03, 0xA1, 0x03, 0xAA
	0x03, 0x21, 0x03, 0x49, 0x03, 0x64, 0x03, 0x87, 0x03, 0x93, 0x03,
	0xA1, 0x03, 0xA4, 0x03, 0xA5
};

static char fir_lg_gamma22_03_ea[] = {
	0xEA,
	//0x03, 0x26, 0x03, 0x52, 0x03, 0x6F, 0x03, 0x9B, 0x03, 0xB1, 0x03,
	//0xC9, 0x03, 0xDF, 0x03, 0xEF
	0x03, 0x26, 0x03, 0x52, 0x03, 0x6F, 0x03, 0x9B, 0x03, 0xB1, 0x03,
	0xC9, 0x03, 0xDF, 0x03, 0xEF
};

static char fir_lg_gamma22_04_d4[] = {
//	0xD4, 0x03, 0xBE, 0x03, 0xBF
	0xD4, 0x03, 0xBC, 0x03, 0xBD
};

static char fir_lg_gamma22_04_d8[] = {
	//0xD8, 0x03, 0xAE, 0x03, 0xAF
	0xD8, 0x03, 0xA6, 0x03, 0xA7
};

static char fir_lg_gamma22_04_df[] = {
	0xDF, 0x03, 0xFF, 0x03, 0xFF
};

static char fir_lg_gamma22_04_e3[] = {
	//0xE3, 0x03, 0xBE, 0x03, 0xBF
	0xE3, 0x03, 0xBC, 0x03, 0xBD
};

static char fir_lg_gamma22_04_e7[] = {
	//0xE7, 0x03, 0xAE, 0x03, 0xAF
	0xE7, 0x03, 0xA6, 0x03, 0xA7
};

static char fir_lg_gamma22_04_eb[] = {
	0xEB, 0x03, 0xFF, 0x03, 0xFF
};

/*   ======== */
/*static char fir_lg_f2[] = {
	0xF2,
	0x00, 0x00, 0x4A, 0x0A, 0xA8, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x40, 0x01, 0x51, 0x00, 0x01, 0x00, 0x01,
};*/

static struct dsi_cmd_desc novatek_video_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 50,
		sizeof(sw_reset), sw_reset},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(display_on), display_on},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,
		sizeof(set_num_of_lanes), set_num_of_lanes},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,
		sizeof(rgb_888), rgb_888},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,
		sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,
		sizeof(led_pwm3), led_pwm3},
};

static struct dsi_cmd_desc novatek_cmd_on_cmds[] = {
	/* added by our own */
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(sw_reset), sw_reset},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xFF, 0xAA, 0x55, 0x25, 0x01} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xFA, 0x00, 0x00, 0x00,
					 0x00, 0x00, 0x00, 0x00,
					 0x00, 0x00, 0x00, 0x00,
					 0x00, 0x03, 0x20, 0x12,
					 0x20, 0xFF, 0xFF, 0xFF} } ,/* 90Hz -> 60Hz */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		8, (char[]){0xF3, 0x02, 0x03, 0x07, 0x44, 0x88, 0xD1, 0x0C} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xFF, 0xAA, 0x55, 0x25, 0x00} } ,

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		6, (char[]){0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		ARRAY_SIZE(novatek_e0), novatek_e0},/* PWM frequency = 13kHz */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		6, (char[]){0xF0, 0x55, 0xAA, 0x52, 0x00, 0x00} } ,

	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(exit_sleep), exit_sleep},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(enable_te), enable_te},
/*
	{DTYPE_DCS_WRITE, 1, 0, 0, 40,
		sizeof(display_on), display_on},
*/
	{DTYPE_MAX_PKTSIZE, 1, 0, 0, 0,
		sizeof(max_pktsize), max_pktsize},
/*
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm1), led_pwm1},
*/
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm3), led_pwm3},
};

static struct dsi_cmd_desc novatek_c2_cmd_on_cmds[] = {
	/* added by our own */
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(sw_reset), sw_reset},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xFF, 0xAA, 0x55, 0x25, 0x01} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		8, (char[]){0xF3, 0x02, 0x03, 0x07, 0x15, 0x88, 0xD1, 0x0C} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xFF, 0xAA, 0x55, 0x25, 0x00} } ,

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		6, (char[]){0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xB8, 0x01, 0x03, 0x03, 0x03} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		ARRAY_SIZE(novatek_e0), novatek_e0},/* PWM frequency = 13kHz */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		6, (char[]){0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xD0, 0x13, 0x11, 0x10, 0x10} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xD1, 0x00, 0x8F, 0x00, 0xC1, 0x00, 0xF0, 0x01,
					 0x0D, 0x01, 0x24, 0x01, 0x41, 0x01, 0x63, 0x01,
					 0x91} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xD2, 0x01, 0xB4, 0x01, 0xE9, 0x02, 0x12, 0x02,
					 0x57, 0x02, 0x8B, 0x02, 0x8D, 0x02, 0xBD, 0x02,
					 0xF5} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xD3, 0x03, 0x19, 0x03, 0x46, 0x03, 0x63, 0x03,
					 0x88, 0x03, 0x9C, 0x03, 0xB6, 0x03, 0xC8, 0x03,
					 0xD5} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xD4, 0x03, 0xDC, 0x03, 0xFF} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xD5, 0x00, 0x8F, 0x00, 0xC1, 0x00, 0xF0, 0x01,
					 0x0D, 0x01, 0x24, 0x01, 0x41, 0x01, 0x63, 0x01,
					 0x91} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xD6, 0x01, 0xB4, 0x01, 0xE9, 0x02, 0x12, 0x02,
					 0x57, 0x02, 0x8B, 0x02, 0x8D, 0x02, 0xBD, 0x02,
					 0xF5} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xD7, 0x03, 0x19, 0x03, 0x46, 0x03, 0x63, 0x03,
					 0x88, 0x03, 0x9C, 0x03, 0xB6, 0x03, 0xC8, 0x03,
					 0xD5} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xD8, 0x03, 0xDC, 0x03, 0xFF} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xD9, 0x00, 0x8F, 0x00, 0xC1, 0x00, 0xF0, 0x01,
					 0x0D, 0x01, 0x24, 0x01, 0x41, 0x01, 0x63, 0x01,
					 0x91} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xDD, 0x01, 0xB4, 0x01, 0xE9, 0x02, 0x12, 0x02,
					 0x57, 0x02, 0x8B, 0x02, 0x8D, 0x02, 0xBD, 0x02,
					 0xF5} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xDE, 0x03, 0x19, 0x03, 0x46, 0x03, 0x63, 0x03,
					 0x88, 0x03, 0x9C, 0x03, 0xB6, 0x03, 0xC8, 0x03,
					 0xD5} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xDF, 0x03, 0xDC, 0x03, 0xFF} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xE0, 0x00, 0x8F, 0x00, 0xC1, 0x00, 0xF0, 0x01,
					 0x0D, 0x01, 0x24, 0x01, 0x41, 0x01, 0x63, 0x01,
					 0x91} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xE1, 0x01, 0xB4, 0x01, 0xE9, 0x02, 0x12, 0x02,
					 0x57, 0x02, 0x8B, 0x02, 0x8D, 0x02, 0xBD, 0x02,
					 0xF5} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xE2, 0x03, 0x19, 0x03, 0x46, 0x03, 0x63, 0x03,
					 0x88, 0x03, 0x9C, 0x03, 0xB6, 0x03, 0xC8, 0x03,
					 0xD5} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xE3, 0x03, 0xDC, 0x03, 0xFF} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xE4, 0x00, 0x8F, 0x00, 0xC1, 0x00, 0xF0, 0x01,
					 0x0D, 0x01, 0x24, 0x01, 0x41, 0x01, 0x63, 0x01,
					 0x91} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xE5, 0x01, 0xB4, 0x01, 0xE9, 0x02, 0x12, 0x02,
					 0x57, 0x02, 0x8B, 0x02, 0x8D, 0x02, 0xBD, 0x02,
					 0xF5} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xE6, 0x03, 0x19, 0x03, 0x46, 0x03, 0x63, 0x03,
					 0x88, 0x03, 0x9C, 0x03, 0xB6, 0x03, 0xC8, 0x03,
					 0xD5} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xE7, 0x03, 0xDC, 0x03, 0xFF} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xE8, 0x00, 0x8F, 0x00, 0xC1, 0x00, 0xF0, 0x01,
					 0x0D, 0x01, 0x24, 0x01, 0x41, 0x01, 0x63, 0x01,
					 0x91} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xE9, 0x01, 0xB4, 0x01, 0xE9, 0x02, 0x12, 0x02,
					 0x57, 0x02, 0x8B, 0x02, 0x8D, 0x02, 0xBD, 0x02,
					 0xF5} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		17, (char[]){0xEA, 0x03, 0x19, 0x03, 0x46, 0x03, 0x63, 0x03,
					 0x88, 0x03, 0x9C, 0x03, 0xB6, 0x03, 0xC8, 0x03,
					 0xD5} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 5,
		5, (char[]){0xEB, 0x03, 0xDC, 0x03, 0xFF} } ,

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		6, (char[]){0xF0, 0x55, 0xAA, 0x52, 0x00, 0x00} } ,

	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(exit_sleep), exit_sleep},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(enable_te), enable_te},
/*
	{DTYPE_DCS_WRITE, 1, 0, 0, 40,
		sizeof(display_on), display_on},
*/
	{DTYPE_MAX_PKTSIZE, 1, 0, 0, 0,
		sizeof(max_pktsize), max_pktsize},
/*
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm1), led_pwm1},
*/
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm3), led_pwm3},
};

static struct dsi_cmd_desc novatek_c3_cmd_on_cmds[] = {
	/* added by our own */
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(sw_reset), sw_reset},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		6, (char[]){0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		ARRAY_SIZE(novatek_e0), novatek_e0},/* PWM frequency = 13kHz */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		6, (char[]){0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01} } ,

	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(exit_sleep), exit_sleep},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(enable_te), enable_te},
/*
	{DTYPE_DCS_WRITE, 1, 0, 0, 40,
		sizeof(display_on), display_on},
*/
	{DTYPE_MAX_PKTSIZE, 1, 0, 0, 0,
		sizeof(max_pktsize), max_pktsize},
/*
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm1), led_pwm1},
*/
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm3), led_pwm3},
};

static struct dsi_cmd_desc lg_novatek_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 30,
		sizeof(sw_reset), sw_reset},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xFF, 0xAA, 0x55, 0x25, 0x01} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		8, (char[]){0xF3, 0x02, 0x03, 0x07,
					 0x15, 0x88, 0xD1, 0x0D} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xFF, 0xAA, 0x55, 0x25, 0x00} } ,
	/* page 0 */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		6, (char[]){0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xB8, 0x01, 0x02, 0x02, 0x02} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		7, (char[]){0xC9, 0x63, 0x06, 0x0D, 0x1A, 0x17, 0x00} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		ARRAY_SIZE(novatek_e0), novatek_e0},/* PWM frequency = 13kHz */
	/* page 1 */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,
		6, (char[]){0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01} },/* select page 1 */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		4, (char[]){0xB0, 0x05, 0x05, 0x05} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		4, (char[]){0xB1, 0x05, 0x05, 0x05} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		4, (char[]){0xB2, 0x01, 0x01, 0x01} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		4, (char[]){0xB3, 0x0E, 0x0E, 0x0E} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		4, (char[]){0xB4, 0x08, 0x08, 0x08} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		4, (char[]){0xB6, 0x44, 0x44, 0x44} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		4, (char[]){0xB7, 0x34, 0x34, 0x34} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		4, (char[]){0xB8, 0x10, 0x10, 0x10} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		4, (char[]){0xB9, 0x26, 0x26, 0x26} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		4, (char[]){0xBA, 0x34, 0x34, 0x34} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		4, (char[]){0xBC, 0x00, 0xC8, 0x00} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		4, (char[]){0xBD, 0x00, 0xC8, 0x00} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		3, (char[]){0xC0, 0x01, 0x03} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		4, (char[]){0xCA, 0x00, 0x15, 0x80} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xD0, 0x0A, 0x10, 0x0D, 0x0F} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_d1), fir_lg_gamma22_01_d1 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_d5), fir_lg_gamma22_01_d5 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_d9), fir_lg_gamma22_01_d9 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_e0), fir_lg_gamma22_01_e0 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_e4), fir_lg_gamma22_01_e4 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_e8), fir_lg_gamma22_01_e8 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_d2), fir_lg_gamma22_02_d2 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_d6), fir_lg_gamma22_02_d6 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_dd), fir_lg_gamma22_02_dd },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_e1), fir_lg_gamma22_02_e1 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_e5), fir_lg_gamma22_02_e5 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_e9), fir_lg_gamma22_02_e9 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_d3), fir_lg_gamma22_03_d3 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_d7), fir_lg_gamma22_03_d7 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_de), fir_lg_gamma22_03_de },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_e2), fir_lg_gamma22_03_e2 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_e6), fir_lg_gamma22_03_e6 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_ea), fir_lg_gamma22_03_ea },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_d4), fir_lg_gamma22_04_d4 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_d8), fir_lg_gamma22_04_d8 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_df), fir_lg_gamma22_04_df },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_e3), fir_lg_gamma22_04_e3 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_e7), fir_lg_gamma22_04_e7 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_eb), fir_lg_gamma22_04_eb },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		6, (char[]){0xF0, 0x55, 0xAA, 0x52, 0x00, 0x00} },/* select page 0 */
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(enable_te), enable_te},
/*
	{DTYPE_DCS_WRITE, 1, 0, 0, 40,
		sizeof(display_on), display_on},
*/
	{DTYPE_MAX_PKTSIZE, 1, 0, 0, 0,
		sizeof(max_pktsize), max_pktsize},
/*
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm1), led_pwm1},
*/
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm3), led_pwm3},
};

static struct dsi_cmd_desc lg_novatek_c2_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 30,
		sizeof(sw_reset), sw_reset},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xFF, 0xAA, 0x55, 0x25, 0x01} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		8, (char[]){0xF3, 0x02, 0x03, 0x07,
					 0x45, 0x88, 0xD1, 0x0D} } ,
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		5, (char[]){0xD0, 0x0A, 0x10, 0x0D, 0x0F} },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_d1), fir_lg_gamma22_01_d1 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_d5), fir_lg_gamma22_01_d5 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_d9), fir_lg_gamma22_01_d9 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_e0), fir_lg_gamma22_01_e0 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_e4), fir_lg_gamma22_01_e4 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_e8), fir_lg_gamma22_01_e8 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_d2), fir_lg_gamma22_02_d2 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_d6), fir_lg_gamma22_02_d6 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_dd), fir_lg_gamma22_02_dd },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_e1), fir_lg_gamma22_02_e1 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_e5), fir_lg_gamma22_02_e5 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_e9), fir_lg_gamma22_02_e9 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_d3), fir_lg_gamma22_03_d3 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_d7), fir_lg_gamma22_03_d7 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_de), fir_lg_gamma22_03_de },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_e2), fir_lg_gamma22_03_e2 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_e6), fir_lg_gamma22_03_e6 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_ea), fir_lg_gamma22_03_ea },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_d4), fir_lg_gamma22_04_d4 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_d8), fir_lg_gamma22_04_d8 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_df), fir_lg_gamma22_04_df },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_e3), fir_lg_gamma22_04_e3 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_e7), fir_lg_gamma22_04_e7 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_eb), fir_lg_gamma22_04_eb },
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(enable_te), enable_te},
/*	{DTYPE_DCS_WRITE, 1, 0, 0, 40,
		sizeof(display_on), display_on},*/
	{DTYPE_MAX_PKTSIZE, 1, 0, 0, 0,
		sizeof(max_pktsize), max_pktsize},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm3), led_pwm3},
};

static struct dsi_cmd_desc lg_novatek_mp_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 30,
		sizeof(sw_reset), sw_reset},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(enable_te), enable_te},
	/* page 1 */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,
		6, (char[]){0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01} },/* select page 1 */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,
		5, (char[]){0xD0, 0x09, 0x10, 0x0C, 0x0F} },/* Grandient Control */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_d1), fir_lg_gamma22_01_d1 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_d5), fir_lg_gamma22_01_d5 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_d9), fir_lg_gamma22_01_d9 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_e0), fir_lg_gamma22_01_e0 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_e4), fir_lg_gamma22_01_e4 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_01_e8), fir_lg_gamma22_01_e8 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_d2), fir_lg_gamma22_02_d2 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_d6), fir_lg_gamma22_02_d6 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_dd), fir_lg_gamma22_02_dd },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_e1), fir_lg_gamma22_02_e1 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_e5), fir_lg_gamma22_02_e5 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_02_e9), fir_lg_gamma22_02_e9 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_d3), fir_lg_gamma22_03_d3 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_d7), fir_lg_gamma22_03_d7 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_de), fir_lg_gamma22_03_de },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_e2), fir_lg_gamma22_03_e2 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_e6), fir_lg_gamma22_03_e6 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_03_ea), fir_lg_gamma22_03_ea },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_d4), fir_lg_gamma22_04_d4 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_d8), fir_lg_gamma22_04_d8 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_df), fir_lg_gamma22_04_df },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_e3), fir_lg_gamma22_04_e3 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_e7), fir_lg_gamma22_04_e7 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma22_04_eb), fir_lg_gamma22_04_eb },

	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(exit_sleep), exit_sleep},
/*
	{DTYPE_DCS_WRITE, 1, 0, 0, 40,
		sizeof(display_on), display_on},
*/
	{DTYPE_MAX_PKTSIZE, 1, 0, 0, 0,
		sizeof(max_pktsize), max_pktsize},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm3), led_pwm3},
};

static unsigned char pwm_freq_sel_cmds1[] = {0x00, 0xB4}; /* address shift to pwm_freq_sel */
static unsigned char pwm_freq_sel_cmds2[] = {0xC6, 0x00}; /* CABC command with parameter 0 */

static unsigned char pwm_dbf_cmds1[] = {0x00, 0xB1}; /* address shift to PWM DBF */
static unsigned char pwm_dbf_cmds2[] = {0xC6, 0x04}; /* CABC command-- DBF: [2:1], force duty: [0] */

/* disable video mdoe */
static unsigned char no_video_mode1[] = {0x00, 0x93};
static unsigned char no_video_mode2[] = {0xB0, 0xB7};
/* disable TE wait VSYNC */
static unsigned char no_wait_te1[] = {0x00, 0xA0};
static unsigned char no_wait_te2[] = {0xC1, 0x00};

static char set_tear_line[] = {0x44, 0x01, 0x68};/* DTYPE_DCS_LWRITE 0x01, 0x68: 3/8 of QHD screen*/

static unsigned char sony_orise9608a_001[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 : address shift*/
static unsigned char sony_orise9608a_002[] = {
	0xFF, 0x96, 0x08, 0x01,
}; /* DTYPE_DCS_LWRITE*/

static unsigned char sony_orise9608a_003[] = {0x00, 0x80}; /* DTYPE_DCS_WRITE1 : address shift*/
static unsigned char sony_orise9608a_004[] = {
	0xFF, 0x96, 0x08,
}; /* DTYPE_DCS_LWRITE */

static unsigned char sony_orise9608a_005[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_006[] = {0xA0, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_007[] = {0x00, 0x80}; /* DTYPE_DCS_WRITE1 : address shift*/
static unsigned char sony_orise9608a_008[] = {
	0xB3, 0x00, 0x00, 0x00,
	0x21, 0x00,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_009[] = {0x00, 0x92}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_010[] = {0xB3, 0x01}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_011[] = {0x00, 0xC0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_012[] = {0xB3, 0x11}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_013[] = {0x00, 0x80}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_014[] = {
	0xC0, 0x00, 0x4A, 0x00,
	0x0A, 0x0A, 0x00, 0x4A,
	0x0A, 0x0A,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_015[] = {0x00, 0x92}; /* DTYPE_DCS_WRITE1 : address shift*/
static unsigned char sony_orise9608a_016[] = {
	0xC0, 0x00, 0x06, 0x00,
	0x09,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_017[] = {0x00, 0xA2}; /* DTYPE_DCS_WRITE1 : address shift*/
static unsigned char sony_orise9608a_018[] = {
	0xC0, 0x00, 0x10, 0x00,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_019[] = {0x00, 0xB3}; /* DTYPE_DCS_WRITE1 : address shift*/
static unsigned char sony_orise9608a_020[] = {
	0xC0, 0x00, 0x50,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_021[] = {0x00, 0x81}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_022[] = {0xC1, 0x55}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_023[] = {0x00, 0x80}; /* DTYPE_DCS_WRITE1 : address shift*/
static unsigned char sony_orise9608a_024[] = {
	0xC4, 0x00, 0x87,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_025[] = {0x00, 0xA0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_026[] = {
	0xC4, 0x33, 0x09, 0x90,
	0x2B, 0x33, 0x09, 0x90,
	0x54,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_027[] = {0x00, 0x80}; /* DTYPE_DCS_WRITE1 : address shift*/
static unsigned char sony_orise9608a_028[] = {
	0xC5, 0x08, 0x00, 0xA0,
	0x11,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_029[] = {0x00, 0x90}; /* DTYPE_DCS_WRITE1 : address shift*/
static unsigned char sony_orise9608a_030[] = {
	0xC5, 0x96, 0x81, 0x06,
	0x81, 0x33, 0x33, 0x34,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_031[] = {0x00, 0xA0}; /* DTYPE_DCS_WRITE1 : address shift*/
static unsigned char sony_orise9608a_032[] = {
	0xC5, 0x96, 0x81, 0x06,
	0x81, 0x33, 0x33, 0x34,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_033[] = {0x00, 0xB0}; /* DTYPE_DCS_WRITE1 : address shift*/
static unsigned char sony_orise9608a_034[] = {
	0xC5, 0x04, 0xA8,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_035[] = {0x00, 0x80}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_036[] = {0xC6, 0x64}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_037[] = {0x00, 0xB0}; /* DTYPE_DCS_WRITE1 : address shift*/
static unsigned char sony_orise9608a_038[] = {
	0xC6, 0x03, 0x09, 0x00,
	0x1F, 0x12,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_039[] = {0x00, 0xE1}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_040[] = {0xC0, 0x96}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_041[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_042[] = {0xD0, 0x02}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_043[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_044[] = {
	0xD1, 0x01, 0x01,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_045[] = {0x00, 0xB7}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_046[] = {0xB0, 0x10}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_047[] = {0x00, 0xC0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_048[] = {0xB0, 0x55}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_049[] = {0x00, 0xB1}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_050[] = {0xB0, 0x03}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_051[] = {0x00, 0x80}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_052[] = {
	0xCB, 0xAA, 0xAA, 0xAA,
	0xAA, 0xAA, 0xAA, 0xAA,
	0xAA, 0xAA, 0xAA,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_053[] = {0x00, 0x90}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_054[] = {
	0xCB, 0xAA, 0xAA, 0xAA,
	0xAA, 0xAA, 0xAA, 0xAA,
	0xAA, 0xAA, 0xAA, 0xAA,
	0xAA, 0xAA, 0xAA, 0xAA,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_055[] = {0x00, 0xA0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_056[] = {
	0xCB, 0xAA, 0xAA, 0xAA,
	0xAA, 0xAA, 0xAA, 0xAA,
	0xAA, 0xAA, 0xAA, 0xAA,
	0xAA, 0xAA, 0xAA, 0xAA,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_057[] = {0x00, 0xB0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_058[] = {
	0xCB, 0xAA, 0xAA, 0xAA,
	0xAA, 0xAA, 0xAA, 0xAA,
	0xAA, 0xAA, 0xAA,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_059[] = {0x00, 0xC0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_060[] = {
	0xCB, 0xA6, 0xA6, 0xA6,
	0xA6, 0xA6, 0xA6, 0xAA,
	0xAA, 0xA6, 0xA6, 0xA6,
	0xA6, 0xA2, 0xA2, 0xA2,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_061[] = {0x00, 0xD0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_062[] = {
	0xCB, 0xA2, 0xAA, 0xAA,
	0xAA, 0xAA, 0xA6, 0xA6,
	0xA6, 0xA6, 0xA6, 0xA6,
	0xAA, 0xAA, 0xA6, 0xA6,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_063[] = {0x00, 0xE0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_064[] = {
	0xCB, 0xA6, 0xA6, 0xA2,
	0xA2, 0xA2, 0xA2, 0xAA,
	0xAA, 0xAA, 0xAA,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_065[] = {0x00, 0xF0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_066[] = {
	0xCB, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_067[] = {0x00, 0x80}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_068[] = {
	0xCC, 0x00, 0x00, 0x25,
	0x26, 0x02, 0x06, 0x00,
	0x00, 0x0A, 0x0C,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_069[] = {0x00, 0x90}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_070[] = {
	0xCC, 0x0E, 0x10, 0x12,
	0x14, 0x16, 0x18, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x25, 0x26, 0x01,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_071[] = {0x00, 0xA0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_072[] = {
	0xCC, 0x05, 0x00, 0x00,
	0x09, 0x0B, 0x0D, 0x0F,
	0x11, 0x13, 0x15, 0x17,
	0x00, 0x00, 0x00, 0x00,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_073[] = {0x00, 0xB0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_074[] = {
	0xCC, 0x00, 0x00, 0x25,
	0x26, 0x05, 0x01, 0x00,
	0x00, 0x0F, 0x0D,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_075[] = {0x00, 0xC0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_076[] = {
	0xCC, 0x0B, 0x09, 0x17,
	0x15, 0x13, 0x11, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x25, 0x26, 0x06,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_077[] = {0x00, 0xD0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_078[] = {
	0xCC, 0x02, 0x00, 0x00,
	0x10, 0x0E, 0x0C, 0x0A,
	0x18, 0x16, 0x14, 0x12,
	0x00, 0x00, 0x00, 0x00,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_079[] = {0x00, 0x80}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_080[] = {
	0xCE, 0x86, 0x03, 0x18,
	0x85, 0x03, 0x18, 0x00,
	0x0F, 0x00, 0x00, 0x0F,
	0x00,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_081[] = {0x00, 0x90}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_082[] = {
	0xCE, 0x33, 0xBF, 0x18,
	0x33, 0xC0, 0x18, 0xF0,
	0x00, 0x00, 0xF0, 0x00,
	0x00, 0x00, 0x00,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_083[] = {0x00, 0xA0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_084[] = {
	0xCE, 0x38, 0x02, 0x83,
	0xC1, 0x86, 0x18, 0x00,
	0x38, 0x01, 0x83, 0xC2,
	0x85, 0x18, 0x00,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_085[] = {0x00, 0xB0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_086[] = {
	0xCE, 0x30, 0x01, 0x83,
	0xC5, 0x86, 0x18, 0x00,
	0x30, 0x02, 0x83, 0xC6,
	0x85, 0x18, 0x00,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_087[] = {0x00, 0xC0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_088[] = {
	0xCE, 0x38, 0x00, 0x83,
	0xC3, 0x86, 0x18, 0x00,
	0x30, 0x00, 0x83, 0xC4,
	0x85, 0x18, 0x00,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_089[] = {0x00, 0xD0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_090[] = {
	0xCE, 0x30, 0x03, 0x83,
	0xC7, 0x86, 0x18, 0x00,
	0x30, 0x04, 0x83, 0xC8,
	0x85, 0x18, 0x00,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_091[] = {0x00, 0x80}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_092[] = {
	0xCF, 0xF0, 0x00, 0x00,
	0x10, 0x00, 0x00, 0x00,
	0xF0, 0x00, 0x00, 0x10,
	0x00, 0x00, 0x00,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_093[] = {0x00, 0x90}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_094[] = {
	0xCF, 0xF0, 0x00, 0x00,
	0x10, 0x00, 0x00, 0x00,
	0xF0, 0x00, 0x00, 0x10,
	0x00, 0x00, 0x00,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_095[] = {0x00, 0xA0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_096[] = {
	0xCF, 0xF0, 0x00, 0x00,
	0x10, 0x00, 0x00, 0x00,
	0xF0, 0x00, 0x00, 0x10,
	0x00, 0x00, 0x00,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_097[] = {0x00, 0xB0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_098[] = {
	0xCF, 0xF0, 0x00, 0x00,
	0x10, 0x00, 0x00, 0x00,
	0xF0, 0x00, 0x00, 0x10,
	0x00, 0x00, 0x00,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_099[] = {0x00, 0xC0}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_100[] = {
	0xCF, 0x01, 0x01, 0x10,
	0x10, 0x00, 0x00, 0x02,
	0x01, 0x00,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_101[] = {0x00, 0x80}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_102[] = {0xD6, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_103[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 : address shift*/
static unsigned char sony_orise9608a_104[] = {
	0xD7, 0x00, 0xD8, 0x1F,
	0x1F, 0xD9, 0x24,
}; /* DTYPE_DCS_LWRITE */

#if 0
static unsigned char sony_gamma25_00[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_gamma25_01[] = {
	0xe1, 0x09, 0x11, 0x17,
	0x0d, 0x06, 0x0e, 0x0a,
	0x08, 0x05, 0x09, 0x0d,
	0x07, 0x0e, 0x0e, 0x0a,
	0x08,
}; /* DTYPE_DCS_LWRITE :0xE100:0x11, 0xE101:0x19, 0xE102: 0x1e, ..., 0xff are padding for 4 bytes*/

static unsigned char sony_gamma25_02[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_gamma25_03[] = {
	0xe2, 0x09, 0x11, 0x17,
	0x0d, 0x06, 0x0e, 0x0a,
	0x08, 0x05, 0x09, 0x0d,
	0x07, 0x0e, 0x0e, 0x0a,
	0x08,
}; /* DTYPE_DCS_LWRITE :0xE200:0x11, 0xE201:0x19, 0xE202: 0x1e, ..., 0xff are padding for 4 bytes*/

static unsigned char sony_gamma25_04[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_gamma25_05[] = {
	0xec, 0x40, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x00,
}; /* DTYPE_DCS_LWRITE */

static unsigned char sony_gamma25_06[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_gamma25_07[] = {
	0xed, 0x40, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x00,
}; /* DTYPE_DCS_LWRITE :0xE200:0x11, 0xE201:0x19, 0xE202: 0x1e, ..., 0xff are padding for 4 bytes*/

static unsigned char sony_gamma25_08[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_gamma25_09[] = {
	0xee, 0x40, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x54,
	0x44, 0x44, 0x44, 0x45,
	0x44, 0x44, 0x45, 0x44,
	0x44, 0x44, 0x44, 0x54,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x34, 0x44,
	0x44, 0x34, 0x44, 0x34,
	0x34, 0x44, 0x00,
}; /* DTYPE_DCS_LWRITE :0xE200:0x11, 0xE201:0x19, 0xE202: 0x1e, ..., 0xff are padding for 4 bytes*/
#endif

static unsigned char sony_gamma22_00[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_gamma22_01[] = {
	0xe1, 0x09, 0x10, 0x15,
	0x0E, 0x07, 0x0E, 0x0B,
	0x09, 0x04, 0x08, 0x0D,
	0x08, 0x0E, 0x13, 0x0D,
	0x08,
/*	0xe1, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0x08,*/
}; /* DTYPE_DCS_LWRITE :0xE100:0x11, 0xE101:0x19, 0xE102: 0x1e, ..., 0xff are padding for 4 bytes*/

static unsigned char sony_gamma22_02[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_gamma22_03[] = {
	0xe2, 0x09, 0x10, 0x15,
	0x0E, 0x07, 0x0E, 0x0B,
	0x09, 0x04, 0x08, 0x0D,
	0x08, 0x0E, 0x13, 0x0D,
	0x08,
}; /* DTYPE_DCS_LWRITE :0xE200:0x11, 0xE201:0x19, 0xE202: 0x1e, ..., 0xff are padding for 4 bytes*/

static unsigned char sony_gamma22_04[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_gamma22_05[] = {
	0xec, 0x40, 0x43, 0x43,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x43, 0x34,
	0x44, 0x44, 0x44, 0x34,
	0x44, 0x34, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x34, 0x44, 0x43, 0x44,
	0x44, 0x44, 0x44, 0x34,
	0x44, 0x44, 0x00,
}; /* DTYPE_DCS_LWRITE */

static unsigned char sony_gamma22_06[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_gamma22_07[] = {
	0xed, 0x40, 0x43, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x43, 0x34,
	0x44, 0x44, 0x44, 0x34,
	0x44, 0x34, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x44,
	0x34, 0x44, 0x43, 0x44,
	0x44, 0x44, 0x44, 0x34,
	0x44, 0x44, 0x00,
}; /* DTYPE_DCS_LWRITE :0xE200:0x11, 0xE201:0x19, 0xE202: 0x1e, ..., 0xff are padding for 4 bytes*/

static unsigned char sony_gamma22_08[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_gamma22_09[] = {
	0xee, 0x40, 0x43, 0x44,
	0x44, 0x54, 0x44, 0x45,
	0x44, 0x45, 0x44, 0x44,
	0x44, 0x44, 0x44, 0x45,
	0x44, 0x44, 0x44, 0x44,
	0x54, 0x44, 0x44, 0x44,
	0x43, 0x44, 0x44, 0x44,
	0x44, 0x44, 0x43, 0x34,
	0x34, 0x44, 0x00,
}; /* DTYPE_DCS_LWRITE :0xE200:0x11, 0xE201:0x19, 0xE202: 0x1e, ..., 0xff are padding for 4 bytes*/

static unsigned char sony_cabc_00[] = {0x00, 0x80}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_cabc_01[] = {
	0xca, 0x01, 0x8e, 0x95,
	0x9d, 0xa4, 0xab, 0xb2,
	0xba, 0xc1, 0xc8, 0xcf,
	0xd7, 0xde, 0xe5, 0xec,
	0xec, 0xe8, 0xff, 0x87,
	0xff, 0x87, 0xff, 0x05,
	0x03, 0x05, 0x03, 0x05,
	0x03,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_cabc_02[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_03[] = {0xC7, 0x10}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_04[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_cabc_05[] = {
	0xc8, 0x90, 0xa9, 0xaa,
	0xaa, 0xaa, 0xaa, 0xaa,
	0xaa, 0x99, 0x88, 0x88,
	0x88, 0x77, 0x66, 0x55,
	0x55, 0x55, 0x55,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_cabc_06[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_07[] = {0xC7, 0x11}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_08[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_cabc_09[] = {
	0xc8, 0x90, 0x99, 0xaa,
	0xaa, 0xaa, 0xaa, 0xaa,
	0x9a, 0x99, 0x88, 0x88,
	0x88, 0x77, 0x66, 0x66,
	0x55, 0x55, 0x55,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_cabc_10[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_11[] = {0xC7, 0x12}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_12[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_cabc_13[] = {
	0xc8, 0x90, 0x99, 0xa9,
	0xaa, 0xaa, 0xaa, 0xaa,
	0x99, 0x99, 0x88, 0x88,
	0x88, 0x77, 0x66, 0x66,
	0x66, 0x55, 0x55,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_cabc_14[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_15[] = {0xC7, 0x13}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_16[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_cabc_17[] = {
	0xc8, 0x90, 0x99, 0x99,
	0xaa, 0xaa, 0xaa, 0x9a,
	0x99, 0x99, 0x88, 0x88,
	0x88, 0x77, 0x66, 0x66,
	0x66, 0x66, 0x55,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_cabc_18[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_19[] = {0xC7, 0x14}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_20[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_cabc_21[] = {
	0xc8, 0x90, 0x99, 0x99,
	0xa9, 0xaa, 0xaa, 0x99,
	0x99, 0x99, 0x88, 0x88,
	0x88, 0x77, 0x66, 0x66,
	0x66, 0x66, 0x66,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_cabc_22[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_23[] = {0xC7, 0x15}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_24[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_cabc_25[] = {
	0xc8, 0x90, 0x99, 0x99,
	0x99, 0xaa, 0x9a, 0x99,
	0x99, 0x99, 0x88, 0x88,
	0x88, 0x77, 0x77, 0x66,
	0x66, 0x66, 0x66,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_cabc_26[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_27[] = {0xC7, 0x16}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_28[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_cabc_29[] = {
	0xc8, 0x90, 0x99, 0x99,
	0x99, 0xa9, 0x99, 0x99,
	0x99, 0x99, 0x88, 0x88,
	0x88, 0x77, 0x77, 0x77,
	0x66, 0x66, 0x66,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_cabc_30[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_31[] = {0xC7, 0x17}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_32[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_cabc_33[] = {
	0xc8, 0x80, 0x99, 0x99,
	0x99, 0x99, 0x99, 0x99,
	0x99, 0x99, 0x88, 0x88,
	0x88, 0x77, 0x77, 0x77,
	0x77, 0x66, 0x66,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_cabc_34[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_35[] = {0xC7, 0x18}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_36[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_cabc_37[] = {
	0xc8, 0x80, 0x98, 0x99,
	0x99, 0x99, 0x98, 0x99,
	0x99, 0x99, 0x88, 0x88,
	0x88, 0x77, 0x77, 0x77,
	0x77, 0x77, 0x66,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_cabc_38[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_39[] = {0xC7, 0x19}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_40[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_cabc_41[] = {
	0xc8, 0x80, 0x88, 0x99,
	0x99, 0x99, 0x88, 0x99,
	0x99, 0x99, 0x88, 0x88,
	0x88, 0x77, 0x77, 0x77,
	0x77, 0x77, 0x77,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_cabc_42[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_43[] = {0xC7, 0x1A}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_44[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_cabc_45[] = {
	0xc8, 0x80, 0x88, 0x98,
	0x99, 0x99, 0x88, 0x98,
	0x99, 0x99, 0x88, 0x88,
	0x88, 0x88, 0x77, 0x77,
	0x77, 0x77, 0x77,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_cabc_46[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_47[] = {0xC7, 0x1B}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_48[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_cabc_49[] = {
	0xc8, 0x80, 0x88, 0x88,
	0x99, 0x99, 0x88, 0x88,
	0x99, 0x99, 0x88, 0x88,
	0x88, 0x88, 0x88, 0x77,
	0x77, 0x77, 0x77,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_cabc_50[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_51[] = {0xC7, 0x1C}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_52[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_cabc_53[] = {
	0xc8, 0x80, 0x88, 0x88,
	0x98, 0x99, 0x88, 0x88,
	0x98, 0x99, 0x88, 0x88,
	0x88, 0x88, 0x88, 0x88,
	0x77, 0x77, 0x77,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_cabc_54[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_55[] = {0xC7, 0x1D}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_56[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 :address shift*/
static unsigned char sony_cabc_57[] = {
	0xc8, 0x80, 0x88, 0x88,
	0x88, 0x99, 0x88, 0x88,
	0x88, 0x99, 0x88, 0x88,
	0x88, 0x88, 0x88, 0x88,
	0x88, 0x77, 0x77,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_cabc_58[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_cabc_59[] = {0xC7, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_105[] = {0x00, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_106[] = {
	0xFF, 0xFF, 0xFF, 0xFF,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_107[] = {
	0x2A, 0x00, 0x00, 0x02,
	0x1B,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_108[] = {
	0x2B, 0x00, 0x00, 0x03,
	0xBF,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_109[] = {0x36, 0x00}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_110[] = {0x3A, 0x77}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_111[] = {
	0x44, 0x01, 0x68,
}; /* DTYPE_DCS_LWRITE */
static unsigned char sony_orise9608a_112[] = {0x35, 0x00};

static unsigned char sony_orise9608a_eot_eotp_1[] = {0x00, 0xB7}; /* DTYPE_DCS_WRITE1 */
static unsigned char sony_orise9608a_eot_eotp_2[] = {0xB0, 0x10}; /* DTYPE_DCS_WRITE1 */

static struct dsi_cmd_desc sony_orise9608a_mp_panel_cmd_mode_cmds[] = {

	/* set driver ic to organize both EOT and EOTP */
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_002), sony_orise9608a_002},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_003), sony_orise9608a_003},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_004), sony_orise9608a_004},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_eot_eotp_1), sony_orise9608a_eot_eotp_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_eot_eotp_2), sony_orise9608a_eot_eotp_2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_105), sony_orise9608a_105},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_106), sony_orise9608a_106},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_107), sony_orise9608a_107},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_108), sony_orise9608a_108},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_109), sony_orise9608a_109},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_110), sony_orise9608a_110},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_111), sony_orise9608a_111},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(sony_orise9608a_112), sony_orise9608a_112},
	{DTYPE_DCS_WRITE, 1, 0, 0, 100,
		sizeof(exit_sleep), exit_sleep},
/*	{DTYPE_DCS_WRITE, 1, 0, 0, 40,
		sizeof(display_on), display_on},*/
	{DTYPE_MAX_PKTSIZE, 1, 0, 0, 0,
		sizeof(max_pktsize), max_pktsize},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm3), led_pwm3},
};

static struct dsi_cmd_desc sony_orise9608a_c1_1_panel_cmd_mode_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_002), sony_orise9608a_002},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_003), sony_orise9608a_003},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_004), sony_orise9608a_004},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_eot_eotp_1), sony_orise9608a_eot_eotp_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_eot_eotp_2), sony_orise9608a_eot_eotp_2},

        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma22_00), sony_gamma22_00},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma22_01), sony_gamma22_01},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma22_02), sony_gamma22_02},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma22_03), sony_gamma22_03},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma22_04), sony_gamma22_04},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma22_05), sony_gamma22_05},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma22_06), sony_gamma22_06},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma22_07), sony_gamma22_07},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma22_08), sony_gamma22_08},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma22_09), sony_gamma22_09},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_105), sony_orise9608a_105},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_106), sony_orise9608a_106},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_107), sony_orise9608a_107},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_108), sony_orise9608a_108},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_109), sony_orise9608a_109},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_110), sony_orise9608a_110},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_111), sony_orise9608a_111},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(sony_orise9608a_112), sony_orise9608a_112},
	{DTYPE_DCS_WRITE, 1, 0, 0, 100,
		sizeof(exit_sleep), exit_sleep},
/*
	{DTYPE_DCS_WRITE, 1, 0, 0, 40,
		sizeof(display_on), display_on},
*/
	{DTYPE_MAX_PKTSIZE, 1, 0, 0, 0,
		sizeof(max_pktsize), max_pktsize},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm3), led_pwm3},
};

static struct dsi_cmd_desc sony_orise9608a_panel_cmd_mode_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_001), sony_orise9608a_001},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_002), sony_orise9608a_002},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_003), sony_orise9608a_003},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_004), sony_orise9608a_004},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_005), sony_orise9608a_005},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_006), sony_orise9608a_006},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_007), sony_orise9608a_007},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_008), sony_orise9608a_008},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_009), sony_orise9608a_009},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_010), sony_orise9608a_010},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_011), sony_orise9608a_011},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_012), sony_orise9608a_012},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_013), sony_orise9608a_013},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_014), sony_orise9608a_014},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_015), sony_orise9608a_015},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_016), sony_orise9608a_016},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_017), sony_orise9608a_017},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_018), sony_orise9608a_018},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_019), sony_orise9608a_019},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_020), sony_orise9608a_020},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_021), sony_orise9608a_021},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_022), sony_orise9608a_022},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_023), sony_orise9608a_023},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_024), sony_orise9608a_024},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_025), sony_orise9608a_025},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_026), sony_orise9608a_026},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_027), sony_orise9608a_027},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_028), sony_orise9608a_028},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_029), sony_orise9608a_029},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_030), sony_orise9608a_030},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_031), sony_orise9608a_031},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_032), sony_orise9608a_032},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_033), sony_orise9608a_033},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_034), sony_orise9608a_034},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_035), sony_orise9608a_035},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_036), sony_orise9608a_036},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_037), sony_orise9608a_037},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_038), sony_orise9608a_038},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_039), sony_orise9608a_039},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_040), sony_orise9608a_040},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_041), sony_orise9608a_041},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_042), sony_orise9608a_042},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_043), sony_orise9608a_043},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_044), sony_orise9608a_044},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_045), sony_orise9608a_045},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_046), sony_orise9608a_046},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_047), sony_orise9608a_047},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_048), sony_orise9608a_048},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_049), sony_orise9608a_049},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_050), sony_orise9608a_050},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_051), sony_orise9608a_051},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_052), sony_orise9608a_052},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_053), sony_orise9608a_053},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_054), sony_orise9608a_054},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_055), sony_orise9608a_055},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_056), sony_orise9608a_056},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_057), sony_orise9608a_057},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_058), sony_orise9608a_058},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_059), sony_orise9608a_059},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_060), sony_orise9608a_060},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_061), sony_orise9608a_061},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_062), sony_orise9608a_062},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_063), sony_orise9608a_063},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_064), sony_orise9608a_064},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_065), sony_orise9608a_065},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_066), sony_orise9608a_066},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_067), sony_orise9608a_067},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_068), sony_orise9608a_068},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_069), sony_orise9608a_069},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_070), sony_orise9608a_070},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_071), sony_orise9608a_071},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_072), sony_orise9608a_072},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_073), sony_orise9608a_073},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_074), sony_orise9608a_074},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_075), sony_orise9608a_075},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_076), sony_orise9608a_076},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_077), sony_orise9608a_077},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_078), sony_orise9608a_078},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_079), sony_orise9608a_079},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_080), sony_orise9608a_080},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_081), sony_orise9608a_081},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_082), sony_orise9608a_082},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_083), sony_orise9608a_083},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_084), sony_orise9608a_084},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_085), sony_orise9608a_085},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_086), sony_orise9608a_086},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_087), sony_orise9608a_087},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_088), sony_orise9608a_088},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_089), sony_orise9608a_089},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_090), sony_orise9608a_090},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_091), sony_orise9608a_091},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_092), sony_orise9608a_092},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_093), sony_orise9608a_093},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_094), sony_orise9608a_094},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_095), sony_orise9608a_095},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_096), sony_orise9608a_096},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_097), sony_orise9608a_097},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_098), sony_orise9608a_098},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_099), sony_orise9608a_099},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_100), sony_orise9608a_100},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_101), sony_orise9608a_101},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_102), sony_orise9608a_102},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_103), sony_orise9608a_103},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_104), sony_orise9608a_104},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma22_00), sony_gamma22_00},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma22_01), sony_gamma22_01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma22_02), sony_gamma22_02},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma22_03), sony_gamma22_03},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma22_04), sony_gamma22_04},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma22_05), sony_gamma22_05},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma22_06), sony_gamma22_06},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma22_07), sony_gamma22_07},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_gamma22_08), sony_gamma22_08},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_gamma22_09), sony_gamma22_09},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_00), sony_cabc_00},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_cabc_01), sony_cabc_01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_02), sony_cabc_02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_03), sony_cabc_03},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_04), sony_cabc_04},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_cabc_05), sony_cabc_05},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_06), sony_cabc_06},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_07), sony_cabc_07},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_08), sony_cabc_08},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_cabc_09), sony_cabc_09},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_10), sony_cabc_10},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_11), sony_cabc_11},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_12), sony_cabc_12},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_cabc_13), sony_cabc_13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_14), sony_cabc_14},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_15), sony_cabc_15},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_16), sony_cabc_16},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_cabc_17), sony_cabc_17},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_18), sony_cabc_18},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_19), sony_cabc_19},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_20), sony_cabc_20},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_cabc_21), sony_cabc_21},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_22), sony_cabc_22},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_23), sony_cabc_23},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_24), sony_cabc_24},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_cabc_25), sony_cabc_25},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_26), sony_cabc_26},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_27), sony_cabc_27},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_28), sony_cabc_28},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_cabc_29), sony_cabc_29},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_30), sony_cabc_30},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_31), sony_cabc_31},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_32), sony_cabc_32},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_cabc_33), sony_cabc_33},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_34), sony_cabc_34},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_35), sony_cabc_35},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_36), sony_cabc_36},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_cabc_37), sony_cabc_37},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_38), sony_cabc_38},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_39), sony_cabc_39},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_40), sony_cabc_40},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_cabc_41), sony_cabc_41},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_42), sony_cabc_42},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_43), sony_cabc_43},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_44), sony_cabc_44},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_cabc_45), sony_cabc_45},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_46), sony_cabc_46},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_47), sony_cabc_47},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_48), sony_cabc_48},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_cabc_49), sony_cabc_49},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_50), sony_cabc_50},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_51), sony_cabc_51},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_52), sony_cabc_52},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_cabc_53), sony_cabc_53},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_54), sony_cabc_54},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_55), sony_cabc_55},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_56), sony_cabc_56},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_cabc_57), sony_cabc_57},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_58), sony_cabc_58},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_cabc_59), sony_cabc_59},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_105), sony_orise9608a_105},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_106), sony_orise9608a_106},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_107), sony_orise9608a_107},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sony_orise9608a_108), sony_orise9608a_108},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_109), sony_orise9608a_109},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sony_orise9608a_110), sony_orise9608a_110},

	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep), exit_sleep},
	/* {DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on}, */

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pwm_freq_sel_cmds1), pwm_freq_sel_cmds1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pwm_freq_sel_cmds2), pwm_freq_sel_cmds2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pwm_dbf_cmds1), pwm_dbf_cmds1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(pwm_dbf_cmds2), pwm_dbf_cmds2},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(enable_te), enable_te},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(set_tear_line), set_tear_line},

	{DTYPE_MAX_PKTSIZE, 1, 0, 0, 0, sizeof(max_pktsize), max_pktsize},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(no_video_mode1), no_video_mode1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(no_video_mode2), no_video_mode2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(no_wait_te1), no_wait_te1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(no_wait_te2), no_wait_te2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(led_pwm3), led_pwm3},
};

static struct dsi_cmd_desc sony_orise9608a_panel_display_on[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};
static struct dsi_cmd_desc novatek_panel_display_on[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 40, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc novatek_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 1,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 100,
		sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc novatek_display_off_lg_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 40,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(enter_sleep), enter_sleep}
};

static char manufacture_id[2] = {0x04, 0x00}; /* DTYPE_DCS_READ */

static struct dsi_cmd_desc novatek_manufacture_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id), manufacture_id};

static uint32 fighter_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	uint32 *lp;

	tp = &fighter_panel_tx_buf;
	rp = &fighter_panel_rx_buf;
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);

	cmd = &novatek_manufacture_id_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 3);
	lp = (uint32 *)rp->data;
	PR_DISP_INFO("%s: manufacture_id=%x\n", __func__, *lp);
	return *lp;
}

#define PWM_MIN                   7
#define PWM_DEFAULT               91
#define PWM_MAX                   255

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 142
#define BRI_SETTING_MAX                 255


static unsigned char fighter_shrink_pwm(int val)
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

static int bl_level_old;

static void mipi_dsi_set_backlight(struct msm_fb_data_type *mfd)
{
	struct mipi_panel_info *mipi;

	mipi  = &mfd->panel_info.mipi;
	if (bl_level_old == mfd->bl_level)
		return;

	mutex_lock(&mfd->dma->ov_mutex);
	/* mdp4_dsi_cmd_busy_wait: will turn on dsi clock also */

	led_pwm1[1] = fighter_shrink_pwm(mfd->bl_level);

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
	mipi_dsi_cmds_tx(mfd, &fighter_panel_tx_buf, novatek_cmd_backlight_cmds,
			ARRAY_SIZE(novatek_cmd_backlight_cmds));
	bl_level_old = mfd->bl_level;
	mutex_unlock(&mfd->dma->ov_mutex);
	return;
}

static int mipi_lcd_on = 1;

static int fighter_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;

	if (mipi->mode == DSI_VIDEO_MODE) {
		mipi_dsi_cmds_tx(mfd, &fighter_panel_tx_buf, novatek_video_on_cmds,
			ARRAY_SIZE(novatek_video_on_cmds));
	} else {
		if (!mipi_lcd_on) {
			mipi_dsi_cmd_bta_sw_trigger(); /* clean up ack_err_status */
			if (panel_type == PANEL_ID_FIGHTER_SAMSUNG_NT) {
				PR_DISP_INFO("fighter_lcd_on PANEL_ID_FIGHTER_SAMSUNG_NT\n");
				mipi_dsi_cmds_tx(mfd, &fighter_panel_tx_buf, novatek_cmd_on_cmds,
					ARRAY_SIZE(novatek_cmd_on_cmds));
			} else if (panel_type == PANEL_ID_FIGHTER_SAMSUNG_NT_C2) {
				PR_DISP_INFO("fighter_lcd_on PANEL_ID_FIGHTER_SAMSUNG_NT_C2\n");
				mipi_dsi_cmds_tx(mfd, &fighter_panel_tx_buf, novatek_c2_cmd_on_cmds,
					ARRAY_SIZE(novatek_c2_cmd_on_cmds));
			} else if (panel_type == PANEL_ID_FIGHTER_SAMSUNG_NT_C3) {
				PR_DISP_INFO("fighter_lcd_on PANEL_ID_FIGHTER_SAMSUNG_NT_C3\n");
				mipi_dsi_cmds_tx(mfd, &fighter_panel_tx_buf, novatek_c3_cmd_on_cmds,
					ARRAY_SIZE(novatek_c3_cmd_on_cmds));
			} else if (panel_type == PANEL_ID_FIGHTER_LG_NT) {
				PR_DISP_INFO("fighter_lcd_on PANEL_ID_FIGHTER_LG_NT\n");
				mipi_dsi_cmds_tx(mfd, &fighter_panel_tx_buf, lg_novatek_cmd_on_cmds,
					ARRAY_SIZE(lg_novatek_cmd_on_cmds));
			} else if (panel_type == PANEL_ID_FIGHTER_LG_NT_C2) {
				PR_DISP_INFO("fighter_lcd_on PANEL_ID_FIGHTER_LG_NT_C2\n");
				mipi_dsi_cmds_tx(mfd, &fighter_panel_tx_buf, lg_novatek_c2_cmd_on_cmds,
					ARRAY_SIZE(lg_novatek_c2_cmd_on_cmds));
			} else if (panel_type == PANEL_ID_FIGHTER_LG_NT_MP) {
				PR_DISP_INFO("fighter_lcd_on PANEL_ID_FIGHTER_LG_NT_MP\n");
				mipi_dsi_cmds_tx(mfd, &fighter_panel_tx_buf, lg_novatek_mp_cmd_on_cmds,
					ARRAY_SIZE(lg_novatek_mp_cmd_on_cmds));
			} else if (panel_type == PANEL_ID_FIGHTER_SONY_OTM) {
				PR_DISP_INFO("fighter_lcd_on PANEL_ID_FIGHTER_SONY_OTM\n");
				mipi_dsi_cmds_tx(mfd, &fighter_panel_tx_buf, sony_orise9608a_panel_cmd_mode_cmds,
					ARRAY_SIZE(sony_orise9608a_panel_cmd_mode_cmds));
			} else if (panel_type == PANEL_ID_FIGHTER_SONY_OTM_C1_1) {
				PR_DISP_INFO("fighter_lcd_on PANEL_ID_FIGHTER_SONY_OTM_C1_1\n");
				mipi_dsi_cmds_tx(mfd, &fighter_panel_tx_buf, sony_orise9608a_c1_1_panel_cmd_mode_cmds,
					ARRAY_SIZE(sony_orise9608a_c1_1_panel_cmd_mode_cmds));
			} else if (panel_type == PANEL_ID_FIGHTER_SONY_OTM_MP) {
				PR_DISP_INFO("fighter_lcd_on PANEL_ID_FIGHTER_SONY_OTM_MP\n");
				mipi_dsi_cmds_tx(mfd, &fighter_panel_tx_buf, sony_orise9608a_mp_panel_cmd_mode_cmds,
					ARRAY_SIZE(sony_orise9608a_mp_panel_cmd_mode_cmds));
			}
		}
		mipi_dsi_cmd_bta_sw_trigger(); /* clean up ack_err_status */

		fighter_manufacture_id(mfd);
	}
	mipi_lcd_on = 1;

	return 0;
}

static void fighter_display_on(struct msm_fb_data_type *mfd)
{
	/* The Orise-Sony panel need to set display on after first frame sent */
	/*
	if (panel_type != PANEL_ID_FIGHTER_SONY_OTM && panel_type != PANEL_ID_FIGHTER_SONY_OTM_C1_1)
		return;
	*/
	mutex_lock(&mfd->dma->ov_mutex);

	if (mfd->panel_info.type == MIPI_CMD_PANEL) {
		mdp4_dsi_cmd_dma_busy_wait(mfd);
		mdp4_dsi_blt_dmap_busy_wait(mfd);
		mipi_dsi_mdp_busy_wait(mfd);
	}
	if (isorise) {
		mipi_dsi_cmds_tx(mfd, &fighter_panel_tx_buf, sony_orise9608a_panel_display_on,
			ARRAY_SIZE(sony_orise9608a_panel_display_on));
	} else {
		mipi_dsi_cmds_tx(mfd, &fighter_panel_tx_buf, novatek_panel_display_on,
			ARRAY_SIZE(novatek_panel_display_on));
	}

	mutex_unlock(&mfd->dma->ov_mutex);
}

static int fighter_lcd_off(struct platform_device *pdev)
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
	if (isorise) {
		mipi_dsi_cmds_tx(mfd, &fighter_panel_tx_buf, novatek_display_off_cmds,
				ARRAY_SIZE(novatek_display_off_cmds));
	} else {
		mipi_dsi_cmds_tx(mfd, &fighter_panel_tx_buf, novatek_display_off_lg_cmds,
				ARRAY_SIZE(novatek_display_off_lg_cmds));
	}

	/*mutex_unlock(&mfd->dma->ov_mutex);*/
	bl_level_old = 0;
	mipi_lcd_on = 0;

	return 0;
}



static void fighter_set_backlight(struct msm_fb_data_type *mfd)
{
	if (!mfd->panel_power_on)
		return;

	mipi_dsi_set_backlight(mfd);
}

static int __devinit fighter_lcd_probe(struct platform_device *pdev)
{
	msm_fb_add_device(pdev);

	return 0;
}

static int mipi_dsi_panel_power(int on);

static void fighter_lcd_shutdown(struct platform_device *pdev)
{
	mipi_dsi_panel_power(0);
}

static struct platform_driver this_driver = {
	.probe  = fighter_lcd_probe,
	.shutdown = fighter_lcd_shutdown,
	.driver = {
		.name   = "mipi_fighter",
	},
};

static struct msm_fb_panel_data novatek_panel_data = {
	.on		= fighter_lcd_on,
	.off		= fighter_lcd_off,
	.set_backlight = fighter_set_backlight,
	.display_on = fighter_display_on,
};

static int ch_used[3];

int mipi_fighter_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_fighter", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	novatek_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &novatek_panel_data,
		sizeof(novatek_panel_data));
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

static struct mipi_dsi_phy_ctrl dsi_cmd_mode_phy_db = {

/* DSI_BIT_CLK at 482MHz, 2 lane, RGB888 */
		/* regulator */
		{0x03, 0x0a, 0x04, 0x00, 0x20},
		/* timing */
		/* clk_rate:482MHz */
		{0xb7, 0x28, 0x1f, 0x00, 0x22, 0x95, 0x22, 0x28, 0x22,
		0x03, 0x04, 0xa0},
		/* phy ctrl */
		{0x5f, 0x00, 0x00, 0x10},
		/* strength */
		{0xff, 0x00, 0x06, 0x00},
		/* pll control */
		{0x0, 0xf9, 0xb0, 0xda, 0x00, 0x50, 0x48, 0x63,
		0x41, 0x0f, 0x01,
		0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
};

#if defined (CONFIG_FB_MSM_MDP_ABL)
static struct gamma_curvy lg_ws2_gamma_tbl = {
       .gamma_len = 33,
       .bl_len = 8,
       .ref_y_gamma = {0, 1, 1, 2, 3, 5, 10, 15, 20, 28, 39, 51,
                       66, 84, 104, 127, 151, 178, 207, 240, 279,
                       319, 358, 417, 473, 527, 583, 650, 719, 779,
                       853, 943, 1024},
       .ref_y_shade = {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352,
                       384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704,
                       736, 768, 800, 832, 864, 896, 928, 960, 992, 1024},
       .ref_bl_lvl = {0, 50, 102, 152, 237, 371, 703, 1024},
       .ref_y_lvl = {0, 138, 218, 298, 424, 601, 818, 1024},
};
#endif

static int mipi_cmd_novatek_blue_qhd_pt_init(void)
{
	int ret;

#if defined (CONFIG_FB_MSM_MDP_ABL)
	pinfo.panel_char = lg_ws2_gamma_tbl;
#endif
	pinfo.xres = 540;
	pinfo.yres = 960;
	pinfo.type = MIPI_CMD_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 49;
	pinfo.height = 87;

	pinfo.lcdc.h_back_porch = 64;
	pinfo.lcdc.h_front_porch = 96;
	pinfo.lcdc.h_pulse_width = 32;
	pinfo.lcdc.v_back_porch = 16;
	pinfo.lcdc.v_front_porch = 16;
	pinfo.lcdc.v_pulse_width = 4;
	pinfo.lcd.v_back_porch = 16;
	pinfo.lcd.v_front_porch = 16;
	pinfo.lcd.v_pulse_width = 4;
	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.clk_rate = 482000000;
	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 6096; /* adjust refx100 to prevent tearing */

	pinfo.mipi.mode = DSI_CMD_MODE;
	pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.t_clk_post = 0x0a;
	pinfo.mipi.t_clk_pre = 0x20;
	pinfo.mipi.stream = 0;	/* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.te_sel = 1; /* TE from vsycn gpio */
	pinfo.mipi.interleave_max = 1;
	pinfo.mipi.insert_dcs_cmd = TRUE;
	pinfo.mipi.wr_mem_continue = 0x3c;
	pinfo.mipi.wr_mem_start = 0x2c;
	pinfo.mipi.dsi_phy_db = &dsi_cmd_mode_phy_db;

	ret = mipi_fighter_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_WVGA_PT);
	if (ret)
		PR_DISP_ERR("%s: failed to register device!\n", __func__);

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

		rc = gpio_request(FIGHTER_LCD_RSTz, "LCM_RST_N");
		if (rc) {
			PR_DISP_ERR("%s:LCM gpio %d request failed, rc=%d\n", __func__,  FIGHTER_LCD_RSTz, rc);
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
		if (isorise) {
			rc = regulator_enable(v_lcmio);
			if (rc) {
				PR_DISP_ERR("enable regulator %s failed, rc=%d\n", lcmio_str, rc);
				return -ENODEV;
			}
		} else {
			rc = regulator_enable(v_lcm);
			if (rc) {
				PR_DISP_ERR("enable regulator %s failed, rc=%d\n", lcm_str, rc);
				return -ENODEV;
			}
		}
		rc = regulator_enable(v_dsivdd);
		if (rc) {
			PR_DISP_ERR("enable regulator %s failed, rc=%d\n", dsivdd_str, rc);
			return -ENODEV;
		}
		if (isorise) {
			rc = regulator_enable(v_lcm);
			if (rc) {
				PR_DISP_ERR("enable regulator %s failed, rc=%d\n", lcm_str, rc);
				return -ENODEV;
			}
		} else {
			rc = regulator_enable(v_lcmio);
			if (rc) {
				PR_DISP_ERR("enable regulator %s failed, rc=%d\n", lcmio_str, rc);
				return -ENODEV;
			}
		}

		if (!mipi_lcd_on) {
			if (isorise)
				hr_msleep(1);
			else
				hr_msleep(10);
			gpio_set_value(FIGHTER_LCD_RSTz, 1);
			if (isorise)
				hr_msleep(10);
			else
				hr_msleep(5);
			gpio_set_value(FIGHTER_LCD_RSTz, 0);
			if (isorise)
				hr_msleep(10);
			else
				hr_msleep(30);
			gpio_set_value(FIGHTER_LCD_RSTz, 1);
		}
		if (isorise)
			hr_msleep(10);
		else
			hr_msleep(30);

		bPanelPowerOn = true;

	} else {
		PR_DISP_INFO("%s: off\n", __func__);
		if (!bPanelPowerOn) return 0;
		if (isorise)
			hr_msleep(120);
		gpio_set_value(FIGHTER_LCD_RSTz, 0);
		if (isorise)
			hr_msleep(120);
		else
			hr_msleep(5);
		if (isorise) {
			if (regulator_disable(v_lcm)) {
				PR_DISP_ERR("%s: Unable to enable the regulator: %s\n", __func__, lcm_str);
				return -EINVAL;
			}
		} else {
			if (regulator_disable(v_lcmio)) {
				PR_DISP_ERR("%s: Unable to enable the regulator: %s\n", __func__, lcmio_str);
				return -EINVAL;
			}
			hr_msleep(5);
		}
		if (regulator_disable(v_dsivdd)) {
			PR_DISP_ERR("%s: Unable to enable the regulator: %s\n", __func__, dsivdd_str);
			return -EINVAL;
		}
		if (isorise) {
			if (regulator_disable(v_lcmio)) {
				PR_DISP_ERR("%s: Unable to enable the regulator: %s\n", __func__, lcmio_str);
				return -EINVAL;
			}
		} else {
			if (regulator_disable(v_lcm)) {
				PR_DISP_ERR("%s: Unable to enable the regulator: %s\n", __func__, lcm_str);
				return -EINVAL;
			}
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
	.vsync_gpio = FIGHTER_LCD_TE,
	.dsi_power_save = mipi_dsi_panel_power,
};

static int __init fighter_panel_init(void)
{
	if(panel_type != PANEL_ID_NONE)
		msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
	else
		printk(KERN_INFO "[DISP]panel ID= NONE\n");
	if (panel_type == PANEL_ID_FIGHTER_SONY_OTM || panel_type == PANEL_ID_FIGHTER_SONY_OTM_C1_1 || panel_type == PANEL_ID_FIGHTER_SONY_OTM_MP)
		isorise = 1;
	return 0;
}

static int __init fighter_panel_late_init(void)
{
	mipi_dsi_buf_alloc(&fighter_panel_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&fighter_panel_rx_buf, DSI_BUF_SIZE);

	mipi_cmd_novatek_blue_qhd_pt_init();

	return platform_driver_register(&this_driver);
}

module_init(fighter_panel_init);
late_initcall(fighter_panel_late_init);
