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

#include "../board-t6.h"

#ifdef MDP_GAMMA
#include "mdp_gamma_renesas.h"
#include "mdp_gamma_renesas_c3.h"
#endif
#include "mipi_t6.h"

static  struct pm8xxx_mpp_config_data MPP_ENABLE = {
	.type           = PM8XXX_MPP_TYPE_D_OUTPUT,
	.level          = PM8921_MPP_DIG_LEVEL_S4,
	.control		= PM8XXX_MPP_DOUT_CTRL_HIGH,
};

static  struct pm8xxx_mpp_config_data MPP_DISABLE = {
	.type           = PM8XXX_MPP_TYPE_D_OUTPUT,
	.level          = PM8921_MPP_DIG_LEVEL_S4,
	.control		= PM8XXX_MPP_DOUT_CTRL_LOW,
};

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
static char unlock_command[2] = {0xB0, 0x04};
static char lock_command[2] = {0xB0, 0x03};

static char samsung_ctrl_brightness[2] = {0x51, 0xFF};
static char write_display_brightness[3]= {0x51, 0x0F, 0xFF};
static char write_control_display[2] = {0x53, 0x24};

static struct dsi_cmd_desc renesas_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc samsung_cmd_backlight_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(samsung_ctrl_brightness), samsung_ctrl_brightness},
};

static char t6_Color_enhancement[33]= {
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

static char gamma_setting_red[31]= {
	0xc7, 0x00, 0x12, 0x18,
	0x20, 0x2C, 0x39, 0x42,
	0x51, 0x35, 0x3D, 0x48,
	0x56, 0x63, 0x6E, 0x7F,
	0x00, 0x12, 0x18, 0x20,
	0x2C, 0x39, 0x42, 0x51,
	0x35, 0x3D, 0x48, 0x56,
	0x63, 0x6E, 0x7F
};

static char gamma_setting_green[25]= {
	0xC8, 0x01, 0x00, 0x00,
	0xFD, 0x03, 0xFC, 0xF1,
	0x00, 0xFA, 0xFC, 0xF6,
	0xD6, 0xF1, 0x00, 0x00,
	0x02, 0x06, 0xF1, 0xE0
};

static char Manufacture_Command_setting[4] = {0xD6, 0x01};
static char deep_standby_off[2] = {0xB1, 0x01};

static char unlock[] = {0xB0, 0x00};
static char display_brightness[] = {0x51, 0xFF};
static char enable_te[] = {0x35, 0x00};
static char lock[] = {0xB0, 0x03};
static char Write_Content_Adaptive_Brightness_Control[2] = {0x55, 0x42};

static char common_setting[] = {
	0xCE, 0x6C, 0x40, 0x43,
	0x49, 0x55, 0x62, 0x71,
	0x82, 0x94, 0xA8, 0xB9,
	0xCB, 0xDB, 0xE9, 0xF5,
	0xFC, 0xFF, 0x01, 0x5A,
	0x00, 0x00, 0x54, 0x20
};
static char cabc_still[] = {0xB9, 0x03, 0x82, 0x3C, 0x10, 0x3C, 0x87};
static char cabc_movie[] = {0xBA, 0x03, 0x78, 0x64, 0x10, 0x64, 0xB4};
static char cabc_bl_limit[] = {0x5E, 0x1E};
static char SRE_Manual_0[] = {0xBB, 0x01, 0x00, 0x00};

static struct dsi_cmd_desc jdi_renesas_cmd_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(unlock_command), unlock_command},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(common_setting), common_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cabc_still), cabc_still},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cabc_movie), cabc_movie},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(SRE_Manual_0), SRE_Manual_0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(t6_Color_enhancement), t6_Color_enhancement},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gamma_setting_red), gamma_setting_red},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(gamma_setting_green), gamma_setting_green},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(lock_command), lock_command},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(Write_Content_Adaptive_Brightness_Control), Write_Content_Adaptive_Brightness_Control},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cabc_bl_limit), cabc_bl_limit},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(enable_te), enable_te},
};

static struct dsi_cmd_desc jdi_renesas_cmd_on_cmds_c3[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(unlock_command), unlock_command},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(common_setting), common_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cabc_still), cabc_still},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cabc_movie), cabc_movie},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(SRE_Manual_0), SRE_Manual_0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(t6_Color_enhancement), t6_Color_enhancement},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(lock_command), lock_command},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(Write_Content_Adaptive_Brightness_Control), Write_Content_Adaptive_Brightness_Control},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cabc_bl_limit), cabc_bl_limit},
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

static struct dsi_cmd_desc jdi_renesas_dim_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_dim_on), dsi_dim_on},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc jdi_renesas_dim_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_dim_off), dsi_dim_off},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
};

static struct dsi_cmd_desc *dim_on_cmds = NULL;
static struct dsi_cmd_desc *dim_off_cmds = NULL;
static int dim_on_cmds_count = 0;
static int dim_off_cmds_count = 0;
static atomic_t lcd_backlight_off;

static void t6_dim_on(struct msm_fb_data_type *mfd)
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
static char renesas_color_en_on[2]= {0xCA, 0x01};
static char renesas_color_en_off[2]= {0xCA, 0x00};

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

static void t6_color_enhance(struct msm_fb_data_type *mfd, int on)
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

static void t6_sre_ctrl(struct msm_fb_data_type *mfd, unsigned long level)
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
static int t6_mdp_gamma(void)
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

static struct dsi_cmd_desc *set_cabc_UI_cmds = NULL;
static struct dsi_cmd_desc *set_cabc_Video_cmds = NULL;
static struct dsi_cmd_desc *set_cabc_Camera_cmds = NULL;
static int set_cabc_UI_cmds_count = 0;
static int set_cabc_Video_cmds_count = 0;
static int set_cabc_Camera_cmds_count = 0;

static int cabc_mode = 0;
static int cur_cabc_mode = 0;
static struct mutex set_cabc_mutex;

static void t6_set_cabc(struct msm_fb_data_type *mfd, int mode)
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

static int t6_lcd_on(struct platform_device *pdev)
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

static int resume_blk = 1;

static int t6_lcd_off(struct platform_device *pdev)
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

static int t6_display_on(struct platform_device *pdev)
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

static int t6_display_off(struct platform_device *pdev)
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

static unsigned int pwm_min = 6;
static unsigned int pwm_default = 81 ;
static unsigned int pwm_max = 255;
static unsigned char pwm_value;

static unsigned char t6_shrink_pwm(int val)
{
	unsigned char shrink_br = BRI_SETTING_MAX;

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

static void t6_set_backlight(struct msm_fb_data_type *mfd)
{
	bool clk_ctrl = false;
	int rc;

	if (mfd && mfd->panel_info.type == MIPI_CMD_PANEL)
		clk_ctrl = true;

    samsung_ctrl_brightness[1] = t6_shrink_pwm((unsigned char)(mfd->bl_level));

	if (resume_blk) {
		resume_blk = 0;

		rc = pm8xxx_mpp_config(BL_HW_EN_MPP_8, &MPP_ENABLE);
		if (rc < 0) {
			pr_err("enable bl_hw failed, rc=%d\n", rc);
			return;
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
		t6_set_cabc(mfd, cabc_mode);
#endif

	if ((mfd->bl_level) == 0) {
		rc = pm8xxx_mpp_config(BL_HW_EN_MPP_8, &MPP_DISABLE);
		if (rc < 0) {
			pr_err("disable bl_hw failed, rc=%d\n", rc);
			return;
		}
		resume_blk = 1;
	}
}

static void scorpius_jdi_renesas_panel_init(void)
{
	panel_on_cmds = jdi_renesas_cmd_on_cmds;
	panel_on_cmds_count = ARRAY_SIZE(jdi_renesas_cmd_on_cmds);
	display_on_cmds = renesas_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(renesas_display_on_cmds);
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
	pwm_default = 81;
	pwm_max = 255;
}

char *board_cid(void);
static void scorpius_jdi_panel_init_c3(void)
{
	panel_on_cmds = jdi_renesas_cmd_on_cmds_c3;
	panel_on_cmds_count = ARRAY_SIZE(jdi_renesas_cmd_on_cmds_c3);
	display_on_cmds = renesas_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(renesas_display_on_cmds);
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
	if (!strncmp(board_cid(), "CWS__001", 8) ||
		!strncmp(board_cid(), "VZW__001", 8) ||
		!strncmp(board_cid(), "SPCS_001", 8)) {
		mdp_gamma = mdp_gamma_renesas;
		mdp_gamma_count = ARRAY_SIZE(mdp_gamma_renesas);
	} else {
		mdp_gamma = mdp_gamma_renesas_c3;
		mdp_gamma_count = ARRAY_SIZE(mdp_gamma_renesas_c3);
	}
#endif
	pwm_min = 6;
	pwm_default = 81;
	pwm_max = 255;
}

static struct mipi_dsi_panel_platform_data *mipi_t6_pdata;

static int __devinit mipi_t6_lcd_probe(struct platform_device *pdev)
{
	switch (panel_type) {
	case PANEL_ID_SCORPIUS_JDI_RENESAS:
		scorpius_jdi_renesas_panel_init();
		break;
	case PANEL_ID_SCORPIUS_JDI_RENESAS_C3:
		scorpius_jdi_panel_init_c3();
		break;
	default:
		pr_err("%s: Unsupported panel type: %d",
				__func__, panel_type);
	}

	if (pdev->id == 0) {
		mipi_t6_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);
	return 0;
}

static struct platform_driver this_driver = {
	.probe = mipi_t6_lcd_probe,
	.driver = {
		.name = "mipi_t6",
	},
};

static struct msm_fb_panel_data t6_panel_data = {
	.on = t6_lcd_on,
	.off = t6_lcd_off,
	.set_backlight = t6_set_backlight,
	.late_init = t6_display_on,
	.early_off = t6_display_off,
#ifdef COLOR_ENHANCE
	.color_enhance = t6_color_enhance,
#endif
#ifdef CONFIG_CABC_DIMMING_SWITCH
	.dimming_on = t6_dim_on,
#endif
#ifdef CABC_LEVEL_CONTROL
	.set_cabc = t6_set_cabc,
#endif
#ifdef CONFIG_SRE_CONTROL
	.sre_ctrl = t6_sre_ctrl,
#endif
#ifdef MDP_GAMMA
	.mdp_gamma = t6_mdp_gamma,
#endif
};

int mipi_t6_device_register(struct msm_panel_info *pinfo,
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

	pdev = platform_device_alloc("mipi_t6", (panel << 8) | channel);
	if (!pdev)
		return -ENOMEM;

	t6_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &t6_panel_data,
		sizeof(t6_panel_data));
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
