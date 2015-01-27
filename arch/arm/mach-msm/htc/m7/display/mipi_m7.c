/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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

#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <mach/panel_id.h>

#include "../../../../drivers/video/msm/msm_fb.h"
#include "../../../../drivers/video/msm/mipi_dsi.h"

#include "../board-m7.h"

#ifdef MDP_GAMMA
#include "mdp_gamma_jdi.h"
#include "mdp_gamma_renesas.h"
#endif
#include "mipi_m7.h"

static struct dsi_cmd_desc *display_off_cmds = NULL;
static struct dsi_cmd_desc *display_on_cmds = NULL;
static struct dsi_cmd_desc *panel_on_cmds = NULL;
static struct dsi_cmd_desc *backlight_cmds = NULL;
static int display_off_cmds_count = 0;
static int display_on_cmds_count = 0;
static int panel_on_cmds_count = 0;
static int backlight_cmds_count = 0;

static char enter_sleep[2] = {0x10, 0x00};
static char exit_sleep[2] = {0x11, 0x00};
static char display_off[2] = {0x28, 0x00};
static char display_on[2] = {0x29, 0x00};
static char nop[2] = {0x00, 0x00};

static char CABC[2] = {0x55, 0x00};
static char jdi_samsung_CABC[2] = {0x55, 0x03};
static char samsung_password_l2[3] = { 0xF0, 0x5A, 0x5A};
static char samsung_MIE_ctrl1[4] = {0xC0, 0x40, 0x10, 0x80};
static char samsung_MIE_ctrl2[2] = {0xC2, 0x0F};

static char samsung_ctrl_source[17] = {
	0xF2, 0x0C, 0x03, 0x73,
	0x06, 0x04, 0x00, 0x25,
	0x00, 0x26, 0x00, 0xD1,
	0x00, 0xD1, 0x00, 0x00,
	0x00
};
static char samsung_ctrl_bl[4] = {0xC3, 0x63, 0x40, 0x01};
static char samsung_ctrl_positive_gamma[70] = {
	0xFA, 0x00, 0x3F, 0x20,
	0x19, 0x25, 0x24, 0x27,
	0x19, 0x19, 0x13, 0x00,
	0x08, 0x0E, 0x0F, 0x14,
	0x15, 0x1F, 0x25, 0x2A,
	0x2B, 0x2A, 0x20, 0x3C,
	0x00, 0x3F, 0x20, 0x19,
	0x25, 0x24, 0x27, 0x19,
	0x19, 0x13, 0x00, 0x08,
	0x0E, 0x0F, 0x14, 0x15,
	0x1F, 0x25, 0x2A, 0x2B,
	0x2A, 0x20, 0x3C, 0x00,
	0x3F, 0x20, 0x19, 0x25,
	0x24, 0x27, 0x19, 0x19,
	0x13, 0x00, 0x08, 0x0E,
	0x0F, 0x14, 0x15, 0x1F,
	0x25, 0x2A, 0x2B, 0x2A,
	0x20, 0x3C
};
static char samsung_ctrl_negative_gamma[70] = {
	0xFB, 0x00, 0x3F, 0x20,
	0x19, 0x25, 0x24, 0x27,
	0x19, 0x19, 0x13, 0x00,
	0x08, 0x0E, 0x0F, 0x14,
	0x15, 0x1F, 0x25, 0x2A,
	0x2B, 0x2A, 0x20, 0x3C,
	0x00, 0x3F, 0x20, 0x19,
	0x25, 0x24, 0x27, 0x19,
	0x19, 0x13, 0x00, 0x08,
	0x0E, 0x0F, 0x14, 0x15,
	0x1F, 0x25, 0x2A, 0x2B,
	0x2A, 0x20, 0x3C, 0x00,
	0x3F, 0x20, 0x19, 0x25,
	0x24, 0x27, 0x19, 0x19,
	0x13, 0x00, 0x08, 0x0E,
	0x0F, 0x14, 0x15, 0x1F,
	0x25, 0x2A, 0x2B, 0x2A,
	0x20, 0x3C
};
static char samsung_password_l3[3] = { 0xFC, 0x5A, 0x5A };
static char samsung_cmd_test[5] = { 0xFF, 0x00, 0x00, 0x00, 0x20};
static char samsung_panel_exit_sleep[2] = {0x11, 0x00};
#ifdef CONFIG_CABC_DIMMING_SWITCH
static char samsung_bl_ctrl[2] = {0x53, 0x24};
#else
static char samsung_bl_ctrl[2] = {0x53, 0x2C};
#endif
static char samsung_ctrl_brightness[2] = {0x51, 0xFF};
static char samsung_enable_te[2] = {0x35, 0x00};
static char samsung_set_column_address[5] = { 0x2A, 0x00, 0x00, 0x04, 0x37 };
static char samsung_set_page_address[5] = { 0x2B, 0x00, 0x00, 0x07, 0x7F };
static char samsung_panel_display_on[2] = {0x29, 0x00};
static char samsung_display_off[2] = {0x28, 0x00};
static char samsung_enter_sleep[2] = {0x10, 0x00};
static char samsung_deep_standby_off[2] = {0xB0, 0x01};
static char SAE[2] = {0xE9, 0x12};
static char samsung_swwr_mipi_speed[4] = {0xE4, 0x00, 0x04, 0x00};
static char samsung_swwr_kinky_gamma[17] = {0xF2, 0x0C, 0x03, 0x03, 0x06, 0x04, 0x00, 0x25, 0x00, 0x26, 0x00, 0xD1, 0x00, 0xD1, 0x00, 0x0A, 0x00};
static char samsung_password_l2_close[3] = {0xF0, 0xA5, 0xA5};
static char samsung_password_l3_close[3] = {0xFC, 0xA5, 0xA5};
static char Oscillator_Bias_Current[4] = {0xFD, 0x56, 0x08, 0x00};
static char samsung_ctrl_positive_gamma_c2_1[70] = {
	0xFA, 0x1E, 0x38, 0x0C,
	0x0C, 0x12, 0x14, 0x16,
	0x17, 0x1A, 0x1A, 0x19,
	0x14, 0x10, 0x0D, 0x10,
	0x13, 0x1D, 0x20, 0x20,
	0x21, 0x26, 0x27, 0x36,
	0x0F, 0x3C, 0x12, 0x15,
	0x1E, 0x21, 0x24, 0x24,
	0x26, 0x24, 0x23, 0x1C,
	0x15, 0x11, 0x13, 0x16,
	0x1E, 0x21, 0x21, 0x21,
	0x26, 0x27, 0x36, 0x00,
	0x3F, 0x13, 0x18, 0x22,
	0x27, 0x29, 0x2A, 0x2B,
	0x2A, 0x29, 0x23, 0x1B,
	0x16, 0x18, 0x19, 0x1F,
	0x22, 0x23, 0x24, 0x2A,
	0x2D, 0x37
};
static char samsung_ctrl_negative_gamma_c2_1[70] = {
	0xFB, 0x1E, 0x38, 0x0C,
	0x0C, 0x12, 0x14, 0x16,
	0x17, 0x1A, 0x1A, 0x19,
	0x14, 0x10, 0x0D, 0x10,
	0x13, 0x1D, 0x20, 0x20,
	0x21, 0x26, 0x27, 0x36,
	0x0F, 0x3C, 0x12, 0x15,
	0x1E, 0x21, 0x24, 0x24,
	0x26, 0x24, 0x23, 0x1C,
	0x15, 0x11, 0x13, 0x16,
	0x1E, 0x21, 0x21, 0x21,
	0x26, 0x27, 0x36, 0x00,
	0x3F, 0x13, 0x18, 0x22,
	0x27, 0x29, 0x2A, 0x2B,
	0x2A, 0x29, 0x23, 0x1B,
	0x16, 0x18, 0x19, 0x1F,
	0x22, 0x23, 0x24, 0x2A,
	0x2D, 0x37
};
static char BCSAVE[] = {
	0xCD, 0x80, 0xB3, 0x67,
	0x1C, 0x78, 0x37, 0x00,
	0x10, 0x73, 0x41, 0x99,
	0x10, 0x00, 0x00
};
static char TMF[] = {
	0xCE, 0x33, 0x1C, 0x0D,
	0x20, 0x14, 0x00, 0x16,
	0x23, 0x18, 0x2C, 0x16,
	0x00, 0x00
};

static struct dsi_cmd_desc samsung_cmd_backlight_cmds_nop[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(samsung_ctrl_brightness), samsung_ctrl_brightness},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc samsung_cmd_backlight_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(samsung_ctrl_brightness), samsung_ctrl_brightness},
};

static struct dsi_cmd_desc samsung_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(samsung_panel_display_on), samsung_panel_display_on},
};

static struct dsi_cmd_desc samsung_jdi_panel_cmd_mode_cmds[] = {
	{DTYPE_DCS_WRITE,  1, 0, 0, 120, sizeof(samsung_panel_exit_sleep), samsung_panel_exit_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl1), samsung_MIE_ctrl1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_bl), samsung_ctrl_bl},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 1, sizeof(SAE),SAE},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_positive_gamma), samsung_ctrl_positive_gamma},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_negative_gamma), samsung_ctrl_negative_gamma},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3), samsung_password_l3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_cmd_test), samsung_cmd_test},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_swwr_mipi_speed), samsung_swwr_mipi_speed},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_swwr_kinky_gamma), samsung_swwr_kinky_gamma},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2_close), samsung_password_l2_close},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3_close), samsung_password_l3_close},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(samsung_set_column_address), samsung_set_column_address},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(samsung_set_page_address), samsung_set_page_address},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(samsung_bl_ctrl), samsung_bl_ctrl},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(CABC), CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(samsung_enable_te), samsung_enable_te},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc samsung_jdi_panel_cmd_mode_cmds_c2_nvm[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3), samsung_password_l3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_cmd_test), samsung_cmd_test},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(samsung_panel_exit_sleep), samsung_panel_exit_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl1), samsung_MIE_ctrl1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_bl), samsung_ctrl_bl},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SAE), SAE},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2_close), samsung_password_l2_close},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3_close), samsung_password_l3_close},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(samsung_bl_ctrl), samsung_bl_ctrl},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(CABC), CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(samsung_enable_te), samsung_enable_te},
};

static struct dsi_cmd_desc samsung_jdi_panel_cmd_mode_cmds_c2_1[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3), samsung_password_l3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_cmd_test), samsung_cmd_test},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Oscillator_Bias_Current), Oscillator_Bias_Current},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(samsung_panel_exit_sleep), samsung_panel_exit_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl1), samsung_MIE_ctrl1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl2), samsung_MIE_ctrl2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_bl), samsung_ctrl_bl},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(BCSAVE), BCSAVE},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(TMF), TMF},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SAE), SAE},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_source), samsung_ctrl_source},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_positive_gamma_c2_1), samsung_ctrl_positive_gamma_c2_1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_negative_gamma_c2_1), samsung_ctrl_negative_gamma_c2_1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2_close), samsung_password_l2_close},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3_close), samsung_password_l3_close},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(samsung_bl_ctrl), samsung_bl_ctrl},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(jdi_samsung_CABC), jdi_samsung_CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(samsung_enable_te), samsung_enable_te},
};

static struct dsi_cmd_desc samsung_jdi_panel_cmd_mode_cmds_c2_2[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3), samsung_password_l3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_cmd_test), samsung_cmd_test},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Oscillator_Bias_Current), Oscillator_Bias_Current},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(samsung_panel_exit_sleep), samsung_panel_exit_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl1), samsung_MIE_ctrl1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl2), samsung_MIE_ctrl2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_bl), samsung_ctrl_bl},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(BCSAVE), BCSAVE},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(TMF), TMF},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SAE), SAE},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_ctrl_source), samsung_ctrl_source},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2_close), samsung_password_l2_close},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l3_close), samsung_password_l3_close},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(samsung_bl_ctrl), samsung_bl_ctrl},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(jdi_samsung_CABC), jdi_samsung_CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(samsung_enable_te), samsung_enable_te},
};

static struct dsi_cmd_desc samsung_display_off_cmds[] = {
	{DTYPE_DCS_WRITE,  1, 0, 0, 0, sizeof(samsung_display_off), samsung_display_off},
	{DTYPE_DCS_WRITE,  1, 0, 0, 48, sizeof(samsung_enter_sleep), samsung_enter_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_WRITE,  1, 0, 0, 0, sizeof(samsung_deep_standby_off), samsung_deep_standby_off},
};

static char write_display_brightness[3]= {0x51, 0x0F, 0xFF};
static char write_control_display[2] = {0x53, 0x24};
static struct dsi_cmd_desc renesas_cmd_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(write_display_brightness), write_display_brightness},
};

static char Color_enhancement[33]= {
	0xCA, 0x01, 0x02, 0xA4,
	0xA4, 0xB8, 0xB4, 0xB0,
	0xA4, 0x3F, 0x28, 0x05,
	0xB9, 0x90, 0x70, 0x01,
	0xFF, 0x05, 0xF8, 0x0C,
	0x0C, 0x0C, 0x0C, 0x13,
	0x13, 0xF0, 0x20, 0x10,
	0x10, 0x10, 0x10, 0x10,
	0x10
};
static char m7_Color_enhancement[33]= {
	0xCA, 0x01, 0x02, 0x9A,
	0xA4, 0xB8, 0xB4, 0xB0,
	0xA4, 0x08, 0x28, 0x05,
	0x87, 0xB0, 0x50, 0x01,
	0xFF, 0x05, 0xF8, 0x0C,
	0x0C, 0x50, 0x40, 0x13,
	0x13, 0xF0, 0x08, 0x10,
	0x10, 0x3F, 0x3F, 0x3F,
	0x3F
};
static char Outline_Sharpening_Control[3]= {
	0xDD, 0x11, 0xA1
};
static char BackLight_Control_6[8]= {
	0xCE, 0x00, 0x07, 0x00,
	0xC1, 0x24, 0xB2, 0x02
};
static char BackLight_Control_6_28kHz[8]= {
	0xCE, 0x00, 0x01, 0x00,
	0xC1, 0xF4, 0xB2, 0x02
};
static char Manufacture_Command_setting[4] = {0xD6, 0x01};
static char hsync_output[4] = {0xC3, 0x01, 0x00, 0x10};
static char protect_on[4] = {0xB0, 0x03};
static char TE_OUT[4] = {0x35, 0x00};
static char deep_standby_off[2] = {0xB1, 0x01};

static char unlock[] = {0xB0, 0x00};
static char display_brightness[] = {0x51, 0xFF};
#ifdef CONFIG_CABC_DIMMING_SWITCH
static char led_pwm_en[] = {0x53, 0x04};
#else
static char led_pwm_en[] = {0x53, 0x0C};
#endif
static char enable_te[] = {0x35, 0x00};
static char lock[] = {0xB0, 0x03};
static char Write_Content_Adaptive_Brightness_Control[2] = {0x55, 0x42};
static char Source_Timing_Setting[23]= {
	0xC4, 0x70, 0x0C, 0x0C,
	0x55, 0x55, 0x00, 0x00,
	0x00, 0x00, 0x05, 0x05,
	0x00, 0x0C, 0x0C, 0x55,
	0x55, 0x00, 0x00, 0x00,
	0x00, 0x05, 0x05
};
static char common_setting[] = {
	0xCE, 0x6C, 0x40, 0x43,
	0x49, 0x55, 0x62, 0x71,
	0x82, 0x94, 0xA8, 0xB9,
	0xCB, 0xDB, 0xE9, 0xF5,
	0xFC, 0xFF, 0x04, 0xD3,
	0x00, 0x00, 0x54, 0x24
};
static char cabc_still[] = {0xB9, 0x03, 0x82, 0x3C, 0x10, 0x3C, 0x87};
static char cabc_movie[] = {0xBA, 0x03, 0x78, 0x64, 0x10, 0x64, 0xB4};
static char SRE_Manual_0[] = {0xBB, 0x01, 0x00, 0x00};
static char blue_shift_adjust_1[] = {
	0xC7, 0x01, 0x0B, 0x12,
	0x1B, 0x2A, 0x3A, 0x45,
	0x56, 0x3A, 0x42, 0x4E,
	0x5B, 0x64, 0x6C, 0x75,
	0x01, 0x0B, 0x12, 0x1A,
	0x29, 0x37, 0x41, 0x52,
	0x36, 0x3F, 0x4C, 0x59,
	0x62, 0x6A, 0x74
};
static char blue_shift_adjust_2[] = {
	0xC8, 0x01, 0x00, 0xF4,
	0x00, 0x00, 0xFC, 0x00,
	0x00, 0xF7, 0x00, 0x00,
	0xFC, 0x00, 0x00, 0xFF,
	0x00, 0x00, 0xFC, 0x0F
};

static struct dsi_cmd_desc generic_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sharp_cmd_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_brightness), display_brightness},
};

static struct dsi_cmd_desc sharp_renesas_panel_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(common_setting), common_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cabc_still), cabc_still},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cabc_movie), cabc_movie},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(SRE_Manual_0), SRE_Manual_0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(m7_Color_enhancement), m7_Color_enhancement},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(Write_Content_Adaptive_Brightness_Control), Write_Content_Adaptive_Brightness_Control},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(led_pwm_en), led_pwm_en},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(blue_shift_adjust_1), blue_shift_adjust_1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(blue_shift_adjust_2), blue_shift_adjust_2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Source_Timing_Setting), Source_Timing_Setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(lock), lock},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(nop), nop},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(enable_te), enable_te},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
};

static char interface_setting_0[2] = {0xB0, 0x04};
static struct dsi_cmd_desc m7_sharp_panel_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(interface_setting_0), interface_setting_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(nop), nop},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Color_enhancement), Color_enhancement},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Outline_Sharpening_Control), Outline_Sharpening_Control},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(BackLight_Control_6_28kHz), BackLight_Control_6_28kHz},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(CABC), CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(TE_OUT), TE_OUT},
	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc sharp_panel_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(interface_setting_0), interface_setting_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Color_enhancement), Color_enhancement},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Outline_Sharpening_Control), Outline_Sharpening_Control},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(BackLight_Control_6), BackLight_Control_6},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(CABC), CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(TE_OUT), TE_OUT},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc sony_panel_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(interface_setting_0), interface_setting_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hsync_output), hsync_output},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Color_enhancement), Color_enhancement},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Outline_Sharpening_Control), Outline_Sharpening_Control},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(BackLight_Control_6), BackLight_Control_6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(protect_on), protect_on},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(CABC), CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(TE_OUT), TE_OUT},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc sharp_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 20,
		sizeof(display_off), display_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 50,
		sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc sony_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_off), display_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 48, sizeof(enter_sleep), enter_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(interface_setting_0), interface_setting_0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(deep_standby_off), deep_standby_off},
};

static char unlock_command[2] = {0xB0, 0x04};
static char lock_command[2] = {0xB0, 0x03};
static struct dsi_cmd_desc jdi_renesas_panel_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(unlock_command), unlock_command},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(common_setting), common_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cabc_still), cabc_still},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cabc_movie), cabc_movie},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(SRE_Manual_0), SRE_Manual_0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(m7_Color_enhancement), m7_Color_enhancement},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(lock_command), lock_command},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(Write_Content_Adaptive_Brightness_Control), Write_Content_Adaptive_Brightness_Control},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(enable_te), enable_te},
};

static struct dsi_cmd_desc jdi_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 48, sizeof(enter_sleep), enter_sleep},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(unlock_command), unlock_command},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(deep_standby_off), deep_standby_off}
};

static int send_display_cmds(struct dsi_cmd_desc *cmd, int cnt,
		bool clk_ctrl)
{
	int ret = 0;
	struct dcs_cmd_req cmdreq;

	cmdreq.cmds = cmd;
	cmdreq.cmds_cnt = cnt;
	cmdreq.flags = CMD_REQ_COMMIT;
	if (clk_ctrl)
		cmdreq.flags |= CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	ret = mipi_dsi_cmdlist_put(&cmdreq);
	if (ret < 0) {
		pr_err("%s failed (%d)\n", __func__, ret);
	}
	return ret;
}

#ifdef CONFIG_CABC_DIMMING_SWITCH
static char dsi_dim_on[] = {0x53, 0x2C};
static char dsi_dim_off[] = {0x53, 0x24};
static char dsi_sharp_cmd_dim_on[] = {0x53, 0x0C};
static char dsi_sharp_cmd_dim_off[] = {0x53, 0x04};

static struct dsi_cmd_desc jdi_renesas_dim_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_dim_on), dsi_dim_on},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc jdi_renesas_dim_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_dim_off), dsi_dim_off},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc samsung_dim_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_dim_on), dsi_dim_on},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc samsung_dim_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_dim_off), dsi_dim_off},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc renesas_dim_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_dim_on), dsi_dim_on},
};

static struct dsi_cmd_desc renesas_dim_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_dim_off), dsi_dim_off},
};

static struct dsi_cmd_desc sharp_cmd_dim_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_sharp_cmd_dim_on), dsi_sharp_cmd_dim_on},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc sharp_cmd_dim_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_sharp_cmd_dim_off), dsi_sharp_cmd_dim_off},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc *dim_on_cmds = NULL;
static struct dsi_cmd_desc *dim_off_cmds = NULL;
static int dim_on_cmds_count = 0;
static int dim_off_cmds_count = 0;
static atomic_t lcd_backlight_off;

static void m7_dim_on(struct msm_fb_data_type *mfd)
{
	bool clk_ctrl = false;

	if (atomic_read(&lcd_backlight_off)) {
		pr_debug("%s: backlight is off. Skip dimming setting\n", __func__);
		return;
	}

	if (dim_on_cmds == NULL)
		return;

	if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
		clk_ctrl = true;

	send_display_cmds(dim_on_cmds, dim_on_cmds_count, clk_ctrl);
}
#endif

#ifdef COLOR_ENHANCE
static char SAE_on[2] = {0xE9, 0x12};
static char SAE_off[2] = {0xE9, 0x00};
static struct dsi_cmd_desc samsung_color_enhance_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(SAE_on),SAE_on},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(samsung_password_l2_close), samsung_password_l2_close},
};
static struct dsi_cmd_desc samsung_color_enhance_off_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0, sizeof(SAE_off),SAE_off},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(samsung_password_l2_close), samsung_password_l2_close},
};

static char renesas_color_en_on[2]= {0xCA, 0x01};
static char renesas_color_en_off[2]= {0xCA, 0x00};
static struct dsi_cmd_desc sharp_renesas_color_enhance_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(renesas_color_en_on), renesas_color_en_on},
};
static struct dsi_cmd_desc sharp_renesas_color_enhance_off_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(renesas_color_en_off), renesas_color_en_off},
};

static struct dsi_cmd_desc sharp_renesas_c1_color_enhance_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(renesas_color_en_on), renesas_color_en_on},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_c1_color_enhance_off_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(renesas_color_en_off), renesas_color_en_off},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};

static struct dsi_cmd_desc *color_en_on_cmds = NULL;
static struct dsi_cmd_desc *color_en_off_cmds = NULL;
static int color_en_on_cmds_count = 0;
static int color_en_off_cmds_count = 0;

static void m7_color_enhance(struct msm_fb_data_type *mfd, int on)
{
	bool clk_ctrl = false;

	if (color_en_on_cmds == NULL || color_en_off_cmds == NULL)
		return;

	if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
		clk_ctrl = true;

	if (on) {
		send_display_cmds(color_en_on_cmds, color_en_on_cmds_count,
				clk_ctrl);
		pr_info("color enhance on\n");
	} else {
		send_display_cmds(color_en_off_cmds, color_en_off_cmds_count,
				clk_ctrl);
		pr_info("color enhance off\n");
	}
}
#endif

#ifdef CONFIG_SRE_CONTROL
static char SRE_Manual1[] = {0xBB, 0x01, 0x00, 0x00};
static char SRE_Manual2[] = {0xBB, 0x01, 0x03, 0x02};
static char SRE_Manual3[] = {0xBB, 0x01, 0x08, 0x05};
static char SRE_Manual4[] = {0xBB, 0x01, 0x13, 0x08};
static char SRE_Manual5[] = {0xBB, 0x01, 0x1C, 0x0E};
static char SRE_Manual6[] = {0xBB, 0x01, 0x25, 0x10};
static char SRE_Manual7[] = {0xBB, 0x01, 0x38, 0x18};
static char SRE_Manual8[] = {0xBB, 0x01, 0x5D, 0x28};
static char SRE_Manual9[] = {0xBB, 0x01, 0x83, 0x38};
static char SRE_Manual10[] = {0xBB, 0x01, 0xA8, 0x48};

static struct dsi_cmd_desc sharp_renesas_sre1_ctrl_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual1), SRE_Manual1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre2_ctrl_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual2), SRE_Manual2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre3_ctrl_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual3), SRE_Manual3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre4_ctrl_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual4), SRE_Manual4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre5_ctrl_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual5), SRE_Manual5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre6_ctrl_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual6), SRE_Manual6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre7_ctrl_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual7), SRE_Manual7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre8_ctrl_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual8), SRE_Manual8},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre9_ctrl_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual9), SRE_Manual9},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};
static struct dsi_cmd_desc sharp_renesas_sre10_ctrl_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(unlock), unlock},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(SRE_Manual10), SRE_Manual10},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(lock), lock},
};

static struct dsi_cmd_desc *sharp_renesas_sre_ctrl_cmds[10] = {
	sharp_renesas_sre1_ctrl_cmds,
	sharp_renesas_sre2_ctrl_cmds,
	sharp_renesas_sre3_ctrl_cmds,
	sharp_renesas_sre4_ctrl_cmds,
	sharp_renesas_sre5_ctrl_cmds,
	sharp_renesas_sre6_ctrl_cmds,
	sharp_renesas_sre7_ctrl_cmds,
	sharp_renesas_sre8_ctrl_cmds,
	sharp_renesas_sre9_ctrl_cmds,
	sharp_renesas_sre10_ctrl_cmds,
};

static struct dsi_cmd_desc **sre_ctrl_cmds = NULL;
static int sre_ctrl_cmds_count = 0;

static void m7_sre_ctrl(struct msm_fb_data_type *mfd, unsigned long level)
{
	static long prev_level = 0, current_stage = 0, prev_stage = 0, tmp_stage = 0;
	struct dsi_cmd_desc *sre_cmds = NULL;
	bool clk_ctrl = false;

	if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
		clk_ctrl = true;

	if (prev_level != level) {
		prev_level = level;
		if (level >= 0 && level < 8000) {
			current_stage = 1;
		} else if (level >= 8000 && level < 16000) {
			current_stage = 2;
		} else if (level >= 16000 && level < 24000) {
			current_stage = 3;
		} else if (level >= 24000 && level < 32000) {
			current_stage = 4;
		} else if (level >= 32000 && level < 40000) {
			current_stage = 5;
		} else if (level >= 40000 && level < 48000) {
			current_stage = 6;
		} else if (level >= 48000 && level < 56000) {
			current_stage = 7;
		} else if (level >= 56000 && level < 65000) {
			current_stage = 8;
		} else if (level >= 65000 && level < 65500) {
			current_stage = 9;
		} else if (level >= 65500 && level < 65536) {
			current_stage = 10;
		} else {
			current_stage = 11;
			pr_warn("out of range of ADC, set it to 11 as default\n");
		}

		if (prev_stage == current_stage)
			return;
		tmp_stage = prev_stage;
		prev_stage = current_stage;

		if (sre_ctrl_cmds == NULL)
			return;
		if (current_stage >= 1 && current_stage <= 10) {
			sre_cmds = sre_ctrl_cmds[current_stage - 1];
		} else {
			sre_cmds = sre_ctrl_cmds[0];
		}
		send_display_cmds(sre_cmds, sre_ctrl_cmds_count, clk_ctrl);

		pr_debug("SRE level %lu prev_stage %lu current_stage %lu\n",
				level, tmp_stage, current_stage);
	}
}
#endif

#ifdef MDP_GAMMA
struct mdp_reg *mdp_gamma = NULL;
static int mdp_gamma_count = 0;
static int m7_mdp_gamma(void)
{
	if (mdp_gamma == NULL)
		return 0;

	mdp_color_enhancement(mdp_gamma, mdp_gamma_count);
	return 0;
}
#endif

#ifdef CABC_LEVEL_CONTROL
static char sharp_renesas_cabc_UI[2] = {0x55, 0x42};
static char sharp_renesas_cabc_Video[2] = {0x55, 0x43};
static struct dsi_cmd_desc sharp_renesas_set_cabc_UI_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(sharp_renesas_cabc_UI), sharp_renesas_cabc_UI},
};
static struct dsi_cmd_desc sharp_renesas_set_cabc_Video_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(sharp_renesas_cabc_Video), sharp_renesas_cabc_Video},
};

static char samsung_MIE_ctrl1_cabc_UI[4] = {0xC0, 0x40, 0x10, 0x80};
static char BCSAVE_cabc_UI[] = {
	0xCD, 0x80, 0xB3, 0x67,
	0x1C, 0x78, 0x37, 0x00,
	0x10, 0x73, 0x41, 0x99,
	0x10, 0x00, 0x00
};
static char TMF_cabc_UI[] = {
	0xCE, 0x33, 0x1C, 0x0D,
	0x20, 0x14, 0x00, 0x16,
	0x23, 0x18, 0x2C, 0x16,
	0x00, 0x00
};
static char samsung_MIE_ctrl1_cabc_Video[4] = {0xC0, 0x80, 0x10, 0x80};
static char BCSAVE_cabc_Video[] = {
	0xCD, 0x80, 0x99, 0x67,
	0x1C, 0x78, 0x37, 0x00,
	0x10, 0x73, 0x41, 0x99,
	0x10, 0x00, 0x00
};
static char TMF_cabc_Video[] = {
	0xCE, 0x2C, 0x1C, 0x0D,
	0x20, 0x14, 0x00, 0x16,
	0x23, 0x18, 0x2C, 0x16,
	0x00, 0x00
};

static struct dsi_cmd_desc jdi_samsung_set_cabc_UI_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl1_cabc_UI), samsung_MIE_ctrl1_cabc_UI},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(BCSAVE_cabc_UI), BCSAVE_cabc_UI},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(TMF_cabc_UI), TMF_cabc_UI},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2_close), samsung_password_l2_close},
};

static struct dsi_cmd_desc jdi_samsung_set_cabc_Video_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2), samsung_password_l2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_MIE_ctrl1_cabc_Video), samsung_MIE_ctrl1_cabc_Video},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(BCSAVE_cabc_Video), BCSAVE_cabc_Video},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(TMF_cabc_Video), TMF_cabc_Video},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(samsung_password_l2_close), samsung_password_l2_close},
};

static struct dsi_cmd_desc *set_cabc_UI_cmds = NULL;
static struct dsi_cmd_desc *set_cabc_Video_cmds = NULL;
static struct dsi_cmd_desc *set_cabc_Camera_cmds = NULL;
static int set_cabc_UI_cmds_count = 0;
static int set_cabc_Video_cmds_count = 0;
static int set_cabc_Camera_cmds_count = 0;

static int cabc_mode = 0;
static int cur_cabc_mode = 0;
static struct mutex set_cabc_mutex;

static void m7_set_cabc(struct msm_fb_data_type *mfd, int mode)
{
	struct dsi_cmd_desc *cabc_cmds = NULL;
	int cabc_cmds_cnt = 0;
	int req_cabc_zoe_mode = 2;
	bool clk_ctrl = false;

	if (set_cabc_UI_cmds == NULL ||
			set_cabc_Video_cmds == NULL ||
			set_cabc_Camera_cmds == NULL)
		return;

	if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
		clk_ctrl = true;

	mutex_lock(&set_cabc_mutex);
	cabc_mode = mode;

	if (mode == 1) {
		send_display_cmds(set_cabc_UI_cmds, set_cabc_UI_cmds_cnt,
				clk_ctrl);
		cur_cabc_mode = mode;
		pr_debug("set_cabc mode = %d\n", mode);
	} else if (mode == 2) {
		send_display_cmds(set_cabc_Video_cmds, set_cabc_Video_cmds_cnt,
				clk_ctrl);
		cur_cabc_mode = mode;
		pr_debug("set_cabc mode = %d\n", mode);
	} else if (mode == 3) {
		if (pwm_value < 168 && cur_cabc_mode == 3) {
			req_cabc_zoe_mode = 2;
			cabc_cmds = set_cabc_Video_cmds;
			cabc_cmds_cnt = set_cabc_Video_cmds_count;
		} else if (pwm_value >= 168 && cur_cabc_mode == 3) {
			req_cabc_zoe_mode = 3;
			cabc_cmds = set_cabc_Camera_cmds;
			cabc_cmds_cnt = set_cabc_Camera_cmds_count;
		} else if (pwm_value == 255) {
			req_cabc_zoe_mode = 3;
			cabc_cmds = set_cabc_Camera_cmds;
			cabc_cmds_cnt = set_cabc_Camera_cmds_count;
		} else {
			req_cabc_zoe_mode = 2;
			cabc_cmds = set_cabc_Video_cmds;
			cabc_cmds_cnt = set_cabc_Video_cmds_count;
		}
		if (cur_cabc_mode != req_cabc_zoe_mode) {
			send_display_cmds(cabc_cmds, cabc_cmds_cnt, clk_ctrl);
			cur_cabc_mode = req_cabc_zoe_mode;
			pr_debug("set_cabc_zoe mode = %d\n", cur_cabc_mode);
		}
	}

	mutex_unlock(&set_cabc_mutex);
}
#endif

static int m7_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	send_display_cmds(panel_on_cmds, panel_on_cmds_count, false);

	return 0;
}

static int resume_blk = 0;

static int m7_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	resume_blk = 1;

	return 0;
}

static int m7_display_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	bool clk_ctrl = false;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
		clk_ctrl = true;

	send_display_cmds(display_on_cmds, display_on_cmds_count, clk_ctrl);

	return 0;
}

static int m7_display_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	bool clk_ctrl = false;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
		clk_ctrl = true;

	send_display_cmds(display_off_cmds, display_off_cmds_count, clk_ctrl);

	return 0;
}

static int pwmic_ver;
static unsigned int pwm_min = 6;
static unsigned int pwm_default = 81 ;
static unsigned int pwm_max = 255;
static unsigned char pwm_value;

static unsigned char m7_shrink_pwm(int val)
{
	unsigned char shrink_br = BRI_SETTING_MAX;
	if (pwmic_ver >= 2)
		pwm_min = 6;
	else
		pwm_min = 11;

	if (val <= 0) {
		shrink_br = 0;
	} else if (val > 0 && (val < BRI_SETTING_MIN)) {
		shrink_br = pwm_min;
	} else if ((val >= BRI_SETTING_MIN) && (val <= BRI_SETTING_DEF)) {
		shrink_br = (val - BRI_SETTING_MIN) * (pwm_default - pwm_min) /
		(BRI_SETTING_DEF - BRI_SETTING_MIN) + pwm_min;
	} else if (val > BRI_SETTING_DEF && val <= BRI_SETTING_MAX) {
		shrink_br = (val - BRI_SETTING_DEF) * (pwm_max - pwm_default) /
		(BRI_SETTING_MAX - BRI_SETTING_DEF) + pwm_default;
	} else if (val > BRI_SETTING_MAX)
		shrink_br = pwm_max;

	pwm_value = shrink_br;

	return shrink_br;
}

static struct i2c_client *blk_pwm_client;

static void pwmic_config(unsigned char *index, unsigned char *value, int count)
{
	int i, rc;

	for(i = 0; i < count; ++i) {
		rc = i2c_smbus_write_byte_data(blk_pwm_client, index[i], value[i]);
		if (rc)
			pr_err("i2c write fail\n");
	}
}

unsigned char idx[5] = {0x50, 0x01, 0x02, 0x05, 0x00};
unsigned char val[5] = {0x02, 0x09, 0x78, 0x14, 0x04};
unsigned char idx0[1] = {0x03};
unsigned char val0[1] = {0xFF};
unsigned char idx1[5] = {0x00, 0x01, 0x02, 0x03, 0x05};
unsigned char val1[5] = {0x04, 0x09, 0x78, 0xff, 0x14};
unsigned char val2[5] = {0x14, 0x08, 0x78, 0xff, 0x14};
unsigned char idx2[6] = {0x00, 0x01, 0x03, 0x03, 0x03, 0x03};
unsigned char idx3[6] = {0x00, 0x03, 0x03, 0x03, 0x03, 0x01};
unsigned char val3[6] = {0x14, 0x09, 0x50, 0xA0, 0xE0, 0xFF};
unsigned char val4[6] = {0x14, 0xF0, 0xA0, 0x50, 0x11, 0x08};

static void m7_set_backlight(struct msm_fb_data_type *mfd)
{
	static int blk_low = 0;
	bool clk_ctrl = false;
	int rc;

	if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
		clk_ctrl = true;

	switch (panel_type) {
	case PANEL_ID_M7_JDI_SAMSUNG:
	case PANEL_ID_M7_JDI_SAMSUNG_C2:
	case PANEL_ID_M7_JDI_SAMSUNG_C2_1:
	case PANEL_ID_M7_JDI_SAMSUNG_C2_2:
	case PANEL_ID_M7_JDI_RENESAS:
		samsung_ctrl_brightness[1] = m7_shrink_pwm((unsigned char)(mfd->bl_level));
		break;
	case PANEL_ID_M7_SHARP_RENESAS_C1:
		display_brightness[1] = m7_shrink_pwm((unsigned char)(mfd->bl_level));
		break;
	default:
		write_display_brightness[2] = m7_shrink_pwm((unsigned char)(mfd->bl_level));
		break;
	}

	if (pwmic_ver >= 2) {
		if (resume_blk) {
			resume_blk = 0;
			gpio_tlmm_config(GPIO_CFG(BL_HW_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(BL_HW_EN, 1);
			hr_msleep(4);
			pwmic_config(idx, val, sizeof(idx));
			hr_msleep(12);
			pwmic_config(idx0, val0, sizeof(idx0));
		}
	} else {
		if (resume_blk) {
			resume_blk = 0;
			gpio_tlmm_config(GPIO_CFG(BL_HW_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(BL_HW_EN, 1);
			hr_msleep(1);
			if (pwm_value >= 21 ) {
				pwmic_config(idx1, val1, sizeof(idx1));
				blk_low = 0;
			} else {
				val2[3] = pwm_value-5;
				pwmic_config(idx1, val2, sizeof(idx1));
				blk_low = 1;
			}
		}
		if (pwm_value >= 21 ) {
			if (blk_low == 1) {
				pwmic_config(idx2, val3, sizeof(idx2));
				blk_low = 0;
				pr_debug("bl >= 21\n");
			}
		} else if ((pwm_value > 0)&&(pwm_value < 21)) {
			if (blk_low == 0) {
				pwmic_config(idx3, val4, sizeof(idx3));
				blk_low = 1;
				pr_debug("bl < 21\n");
			}
			rc = i2c_smbus_write_byte_data(blk_pwm_client,
					0x03, (pwm_value - 5));
			if (rc)
				pr_err("i2c write fail\n");
		}
	}

#ifdef CONFIG_CABC_DIMMING_SWITCH
	if (samsung_ctrl_brightness[1] == 0 ||
			display_brightness[1] == 0 ||
			write_display_brightness[2] == 0) {
		atomic_set(&lcd_backlight_off, 1);
		send_display_cmds(dim_off_cmds, dim_off_cmds_count, clk_ctrl);
	} else {
		atomic_set(&lcd_backlight_off, 0);
	}
#endif

	send_display_cmds(backlight_cmds, backlight_cmds_count, clk_ctrl);

#ifdef CABC_LEVEL_CONTROL
	if (cabc_mode == 3)
		m7_set_cabc(mfd, cabc_mode);
#endif

	if ((mfd->bl_level) == 0) {
		gpio_tlmm_config(GPIO_CFG(BL_HW_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_set_value(BL_HW_EN, 0);
		resume_blk = 1;
	}
}

static void sharp_renesas_panel_init(void)
{
	if (PANEL_ID_DLXJ_SHARP_RENESAS) {
		panel_on_cmds = sharp_panel_on_cmds;
		panel_on_cmds_count = ARRAY_SIZE(sharp_panel_on_cmds);
	} else if (PANEL_ID_M7_SHARP_RENESAS) {
		panel_on_cmds = m7_sharp_panel_on_cmds;
		panel_on_cmds_count = ARRAY_SIZE(m7_sharp_panel_on_cmds);
	}
	display_on_cmds = generic_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(generic_display_on_cmds);
	display_off_cmds = sharp_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(sharp_display_off_cmds);
	backlight_cmds = renesas_cmd_backlight_cmds;
	backlight_cmds_count = ARRAY_SIZE(renesas_cmd_backlight_cmds);
#ifdef CONFIG_CABC_DIMMING_SWITCH
	dim_on_cmds = renesas_dim_on_cmds;
	dim_on_cmds_count = ARRAY_SIZE(renesas_dim_on_cmds);
	dim_off_cmds = renesas_dim_off_cmds;
	dim_off_cmds_count = ARRAY_SIZE(renesas_dim_off_cmds);
#endif
#ifdef COLOR_ENHANCE
	color_en_on_cmds = sharp_renesas_color_enhance_on_cmds;
	color_en_on_cmds_count = ARRAY_SIZE(sharp_renesas_color_enhance_on_cmds);
	color_en_off_cmds = sharp_renesas_color_enhance_off_cmds;
	color_en_off_cmds_count = ARRAY_SIZE(sharp_renesas_color_enhance_off_cmds);
#endif
	pwm_min = 13;
	pwm_default = 82;
	pwm_max = 255;
}

static void sony_panel_init(void)
{
	panel_on_cmds = sony_panel_on_cmds;
	panel_on_cmds_count = ARRAY_SIZE(sony_panel_on_cmds);
	display_on_cmds = generic_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(generic_display_on_cmds);
	display_off_cmds = sony_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(sony_display_off_cmds);
	backlight_cmds = renesas_cmd_backlight_cmds;
	backlight_cmds_count = ARRAY_SIZE(renesas_cmd_backlight_cmds);
#ifdef CONFIG_CABC_DIMMING_SWITCH
	dim_on_cmds = renesas_dim_on_cmds;
	dim_on_cmds_count = ARRAY_SIZE(renesas_dim_on_cmds);
	dim_off_cmds = renesas_dim_off_cmds;
	dim_off_cmds_count = ARRAY_SIZE(renesas_dim_off_cmds);
#endif
	pwm_min = 13;
	pwm_default = 82;
	pwm_max = 255;
}

static void samsung_panel_init(void)
{
	switch (panel_type) {
	case PANEL_ID_M7_JDI_SAMSUNG:
		panel_on_cmds = samsung_jdi_panel_cmd_mode_cmds;
		panel_on_cmds_count = ARRAY_SIZE(samsung_jdi_panel_cmd_mode_cmds);
		backlight_cmds = samsung_cmd_backlight_cmds_nop;
		backlight_cmds_count = ARRAY_SIZE(samsung_cmd_backlight_cmds_nop);
#ifdef MDP_GAMMA
		mdp_gamma = mdp_gamma_jdi;
		mdp_gamma_count = ARRAY_SIZE(mdp_gamma_jdi);
#endif
		break;
	case PANEL_ID_M7_JDI_SAMSUNG_C2:
		panel_on_cmds = samsung_jdi_panel_cmd_mode_cmds_c2_nvm;
		panel_on_cmds_count = ARRAY_SIZE(samsung_jdi_panel_cmd_mode_cmds_c2_nvm);
		backlight_cmds = samsung_cmd_backlight_cmds;
		backlight_cmds_count = ARRAY_SIZE(samsung_cmd_backlight_cmds);
#ifdef MDP_GAMMA
		mdp_gamma = mdp_gamma_jdi;
		mdp_gamma_count = ARRAY_SIZE(mdp_gamma_jdi);
#endif
		break;
	case PANEL_ID_M7_JDI_SAMSUNG_C2_1:
		panel_on_cmds = samsung_jdi_panel_cmd_mode_cmds_c2_1;
		panel_on_cmds_count = ARRAY_SIZE(samsung_jdi_panel_cmd_mode_cmds_c2_1);
		backlight_cmds = samsung_cmd_backlight_cmds;
		backlight_cmds_count = ARRAY_SIZE(samsung_cmd_backlight_cmds);
#ifdef CABC_LEVEL_CONTROL
		set_cabc_UI_cmds = jdi_samsung_set_cabc_UI_cmds;
		set_cabc_UI_cmds_count = ARRAY_SIZE(jdi_samsung_set_cabc_UI_cmds);
		set_cabc_Video_cmds = jdi_samsung_set_cabc_UI_cmds;
		set_cabc_Video_cmds_count = ARRAY_SIZE(jdi_samsung_set_cabc_UI_cmds);
		set_cabc_Camera_cmds = jdi_samsung_set_cabc_Video_cmds;
		set_cabc_Camera_cmds_count = ARRAY_SIZE(jdi_samsung_set_cabc_Video_cmds);
#endif
		break;
	case PANEL_ID_M7_JDI_SAMSUNG_C2_2:
		panel_on_cmds = samsung_jdi_panel_cmd_mode_cmds_c2_2;
		panel_on_cmds_count = ARRAY_SIZE(samsung_jdi_panel_cmd_mode_cmds_c2_2);
		backlight_cmds = samsung_cmd_backlight_cmds;
		backlight_cmds_count = ARRAY_SIZE(samsung_cmd_backlight_cmds);
#ifdef CABC_LEVEL_CONTROL
		set_cabc_UI_cmds = jdi_samsung_set_cabc_UI_cmds;
		set_cabc_UI_cmds_count = ARRAY_SIZE(jdi_samsung_set_cabc_UI_cmds);
		set_cabc_Video_cmds = jdi_samsung_set_cabc_UI_cmds;
		set_cabc_Video_cmds_count = ARRAY_SIZE(jdi_samsung_set_cabc_UI_cmds);
		set_cabc_Camera_cmds = jdi_samsung_set_cabc_Video_cmds;
		set_cabc_Camera_cmds_count = ARRAY_SIZE(jdi_samsung_set_cabc_Video_cmds);
		break;
#endif
	}
	display_on_cmds = samsung_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(samsung_display_on_cmds);
	display_off_cmds = samsung_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(samsung_display_off_cmds);
#ifdef CONFIG_CABC_DIMMING_SWITCH
	dim_on_cmds = samsung_dim_on_cmds;
	dim_on_cmds_count = ARRAY_SIZE(samsung_dim_on_cmds);
	dim_off_cmds = samsung_dim_off_cmds;
	dim_off_cmds_count = ARRAY_SIZE(samsung_dim_off_cmds);
#endif
#ifdef COLOR_ENHANCE
	color_en_on_cmds = samsung_color_enhance_on_cmds;
	color_en_on_cmds_count = ARRAY_SIZE(samsung_color_enhance_on_cmds);
	color_en_off_cmds = samsung_color_enhance_off_cmds;
	color_en_off_cmds_count = ARRAY_SIZE(samsung_color_enhance_on_cmds);
#endif
	pwm_min = 6;
	pwm_default = 69;
	pwm_max = 255;
}

static void sharp_panel_init(void)
{
	panel_on_cmds = sharp_renesas_panel_on_cmds;
	panel_on_cmds_count = ARRAY_SIZE(sharp_renesas_panel_on_cmds);
	display_on_cmds = generic_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(generic_display_on_cmds);
	display_off_cmds = sharp_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(sharp_display_off_cmds);
	backlight_cmds = sharp_cmd_backlight_cmds;
	backlight_cmds_count = ARRAY_SIZE(sharp_cmd_backlight_cmds);
#ifdef CONFIG_CABC_DIMMING_SWITCH
	dim_on_cmds = sharp_cmd_dim_on_cmds;
	dim_on_cmds_count = ARRAY_SIZE(sharp_cmd_dim_on_cmds);
	dim_off_cmds = sharp_cmd_dim_off_cmds;
	dim_off_cmds_count = ARRAY_SIZE(sharp_cmd_dim_off_cmds);
#endif
#ifdef COLOR_ENHANCE
	color_en_on_cmds = sharp_renesas_c1_color_enhance_on_cmds;
	color_en_on_cmds_count = ARRAY_SIZE(sharp_renesas_c1_color_enhance_on_cmds);
	color_en_off_cmds = sharp_renesas_c1_color_enhance_off_cmds;
	color_en_off_cmds_count = ARRAY_SIZE(sharp_renesas_c1_color_enhance_off_cmds);
#endif
#ifdef CONFIG_SRE_CONTROL
	sre_ctrl_cmds = sharp_renesas_sre_ctrl_cmds;
	sre_ctrl_cmds_count = ARRAY_SIZE(sharp_renesas_sre1_ctrl_cmds);
#endif
#ifdef CABC_LEVEL_CONTROL
	set_cabc_UI_cmds = sharp_renesas_set_cabc_UI_cmds;
	set_cabc_UI_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_UI_cmds);
	set_cabc_Video_cmds = sharp_renesas_set_cabc_Video_cmds;
	set_cabc_Video_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_Video_cmds);
	set_cabc_Camera_cmds = sharp_renesas_set_cabc_Video_cmds;
	set_cabc_Camera_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_Video_cmds);
#endif
#ifdef MDP_GAMMA
	mdp_gamma = mdp_gamma_renesas;
	mdp_gamma_count = ARRAY_SIZE(mdp_gamma_renesas);
#endif
	pwm_min = 6;
	pwm_default = 69;
	pwm_max = 255;
}

static void jdi_renesas_panel_init(void)
{
	panel_on_cmds = jdi_renesas_panel_on_cmds;
	panel_on_cmds_count = ARRAY_SIZE(jdi_renesas_panel_on_cmds);
	display_on_cmds = generic_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(generic_display_on_cmds);
	display_off_cmds = jdi_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(jdi_display_off_cmds);
	backlight_cmds = samsung_cmd_backlight_cmds;
	backlight_cmds_count = ARRAY_SIZE(samsung_cmd_backlight_cmds);
#ifdef CONFIG_CABC_DIMMING_SWITCH
	dim_on_cmds = jdi_renesas_dim_on_cmds;
	dim_on_cmds_count = ARRAY_SIZE(jdi_renesas_dim_on_cmds);
	dim_off_cmds = jdi_renesas_dim_off_cmds;
	dim_off_cmds_count = ARRAY_SIZE(jdi_renesas_dim_off_cmds);
#endif
#ifdef COLOR_ENHANCE
	color_en_on_cmds = sharp_renesas_c1_color_enhance_on_cmds;
	color_en_on_cmds_count = ARRAY_SIZE(sharp_renesas_c1_color_enhance_on_cmds);
	color_en_off_cmds = sharp_renesas_c1_color_enhance_off_cmds;
	color_en_off_cmds_count = ARRAY_SIZE(sharp_renesas_c1_color_enhance_off_cmds);
#endif
#ifdef CONFIG_SRE_CONTROL
	sre_ctrl_cmds = sharp_renesas_sre_ctrl_cmds;
	sre_ctrl_cmds_count = ARRAY_SIZE(sharp_renesas_sre1_ctrl_cmds);
#endif
#ifdef CABC_LEVEL_CONTROL
	set_cabc_UI_cmds = sharp_renesas_set_cabc_UI_cmds;
	set_cabc_UI_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_UI_cmds);
	set_cabc_Video_cmds = sharp_renesas_set_cabc_Video_cmds;
	set_cabc_Video_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_Video_cmds);
	set_cabc_Camera_cmds = sharp_renesas_set_cabc_Video_cmds;
	set_cabc_Camera_cmds_count = ARRAY_SIZE(sharp_renesas_set_cabc_Video_cmds);
#endif
#ifdef MDP_GAMMA
	mdp_gamma = mdp_gamma_renesas;
	mdp_gamma_count = ARRAY_SIZE(mdp_gamma_renesas);
#endif
	pwm_min = 6;
	pwm_default = 69;
	pwm_max = 255;
}

static struct mipi_dsi_panel_platform_data *mipi_m7_pdata;

static int __devinit mipi_m7_lcd_probe(struct platform_device *pdev)
{
	switch (panel_type) {
	case PANEL_ID_DLXJ_SHARP_RENESAS:
		pr_info("%s: panel type: %s\n", __func__, "PANEL_ID_DLXJ_SHARP_RENESAS");
		sharp_renesas_panel_init();
		break;
	case PANEL_ID_M7_SHARP_RENESAS:
		pr_info("%s: panel type: %s\n", __func__, "PANEL_ID_M7_SHARP_RENESAS");
		sharp_renesas_panel_init();
		break;
	case PANEL_ID_DLXJ_SONY_RENESAS:
		pr_info("%s: panel type: %s\n", __func__, "PANEL_ID_DLXJ_SONY_RENESAS");
		sony_panel_init();
		break;
	case PANEL_ID_M7_JDI_SAMSUNG:
		pr_info("%s: panel type: %s\n", __func__, "PANEL_ID_M7_JDI_SAMSUNG");
		samsung_panel_init();
		break;
	case PANEL_ID_M7_JDI_SAMSUNG_C2:
		pr_info("%s: panel type: %s\n", __func__, "PANEL_ID_M7_JDI_SAMSUNG_C2");
		samsung_panel_init();
		break;
	case PANEL_ID_M7_JDI_SAMSUNG_C2_1:
		pr_info("%s: panel type: %s\n", __func__, "PANEL_ID_M7_JDI_SAMSUNG_C2_1");
		samsung_panel_init();
		break;
	case PANEL_ID_M7_JDI_SAMSUNG_C2_2:
		pr_info("%s: panel type: %s\n", __func__, "PANEL_ID_M7_JDI_SAMSUNG_C2_2");
		samsung_panel_init();
		break;
	case PANEL_ID_M7_SHARP_RENESAS_C1:
		pr_info("%s: panel type: %s\n", __func__, "PANEL_ID_M7_SHARP_RENESAS_C1");
		sharp_panel_init();
		break;
	case PANEL_ID_M7_JDI_RENESAS:
		pr_info("%s: panel type: %s\n", __func__, "PANEL_ID_M7_JDI_RENESAS");
		jdi_renesas_panel_init();
		break;
	default:
		pr_err("%s: Unsupported panel type: %d",
				__func__, panel_type);
	}

	if (pdev->id == 0) {
		mipi_m7_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);
	return 0;
}

static struct platform_driver this_driver = {
	.probe = mipi_m7_lcd_probe,
	.driver = {
		.name = "mipi_m7",
	},
};

static struct msm_fb_panel_data m7_panel_data = {
	.on = m7_lcd_on,
	.off = m7_lcd_off,
	.set_backlight = m7_set_backlight,
	.late_init = m7_display_on,
	.early_off = m7_display_off,
#ifdef COLOR_ENHANCE
	.color_enhance = m7_color_enhance,
#endif
#ifdef CONFIG_CABC_DIMMING_SWITCH
	.dimming_on = m7_dim_on,
#endif
#ifdef CABC_LEVEL_CONTROL
	.set_cabc = m7_set_cabc,
#endif
#ifdef CONFIG_SRE_CONTROL
	.sre_ctrl = m7_sre_ctrl,
#endif
#ifdef MDP_GAMMA
	.mdp_gamma = m7_mdp_gamma,
#endif
};

static int pwm_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id);
static void pwm_i2c_remove(void);

static const struct i2c_device_id pwm_i2c_id[] = {
	{ "pwm_i2c", 0 },
	{ }
};

static struct i2c_driver pwm_i2c_driver = {
	.driver = {
		.name = "pwm_i2c",
		.owner = THIS_MODULE,
	},
	.probe = pwm_i2c_probe,
	.remove =  __exit_p(pwm_i2c_remove),
	.id_table =  pwm_i2c_id,
};

static int pwm_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_BYTE | I2C_FUNC_I2C))
		return -ENODEV;

	blk_pwm_client = client;

	return 0;
}

static void __exit pwm_i2c_remove(void)
{
	i2c_del_driver(&pwm_i2c_driver);
}

static void pwm_i2c_init(void)
{
	int ret;

	ret = i2c_add_driver(&pwm_i2c_driver);
	if (ret) {
		pr_err("%s: i2c_add_driver failed!\n", __func__);
		return;
	}

	pwmic_ver = i2c_smbus_read_byte_data(blk_pwm_client, 0x1f);
	pr_info("%s: PWM IC version %d\n", __func__, pwmic_ver);
}

int mipi_m7_device_register(struct msm_panel_info *pinfo,
		u32 channel, u32 panel)
{
	static int ch_used[3] = {0};
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

#ifdef CABC_LEVEL_CONTROL
	mutex_init(&set_cabc_mutex);
#endif

	pwm_i2c_init();

	pdev = platform_device_alloc("mipi_m7", (panel << 8) | channel);
	if (!pdev)
		return -ENOMEM;

	m7_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &m7_panel_data,
		sizeof(m7_panel_data));
	if (ret) {
		pr_err("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_err("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}
	return platform_driver_register(&this_driver);

err_device_put:
	platform_device_put(pdev);
	return ret;
}
