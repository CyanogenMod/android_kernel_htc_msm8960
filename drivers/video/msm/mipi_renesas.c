/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_renesas.h"
#include "mdp4.h"
#include <linux/i2c.h>
#include <mach/debug_display.h>
#include <mach/panel_id.h>

static struct mipi_dsi_panel_platform_data *mipi_renesas_pdata;

static struct dsi_buf renesas_tx_buf;
static struct dsi_buf renesas_rx_buf;

static struct dsi_cmd_desc *renesas_video_on_cmds = NULL;
static struct dsi_cmd_desc *renesas_display_off_cmds = NULL;
int renesas_video_on_cmds_count = 0;
int renesas_display_off_cmds_count = 0;

static int mipi_renesas_lcd_init(void);

static char enter_sleep[2] = {0x10, 0x00}; 
static char exit_sleep[2] = {0x11, 0x00}; 
static char display_off[2] = {0x28, 0x00}; 
static char display_on[2] = {0x29, 0x00}; 

static char write_display_brightnes[3]= {0x51, 0x0F, 0xFF};
static char write_control_display[2] = {0x53, 0x24}; 
static struct dsi_cmd_desc renesas_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};
static struct dsi_cmd_desc renesas_cmd_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(write_display_brightnes), write_display_brightnes},
};


static char interface_setting_0[2] = {0xB0, 0x04};
#if 0
static char interface_setting_1[7] = {
	0xB3, 0x14, 0x00, 0x00,
	0x00, 0x00, 0x00};
static char interface_id_setting[3] = {
	0xB4, 0x0C, 0x00};
static char DSI_Control[3] = {
	0xB6, 0x3A, 0xD3};
static char External_Clock_Setting[3] = {
	0xBB, 0x00, 0x10};
static char Slew_rate_adjustment[3] = {
	0xC0, 0x00, 0x00};
static char Display_Setting1common[35] = {
	0xC1, 0x04, 0x60, 0x40,
	0x40, 0x0C, 0x00, 0x50,
	0x02, 0x00, 0x00, 0x00,
	0x5C, 0x63, 0xAC, 0x39,
	0x00, 0x00, 0x50, 0x00,
	0x00, 0x00, 0x86, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x40, 0x00, 0x02,
	0x02, 0x02, 0x00};
static char Display_Setting2[8]= {
	0xC2, 0x32, 0xF7, 0x00,
	0x00, 0x08, 0x00, 0x00};
static char TPC_Sync_Control[4]= {
	0xC3, 0x00, 0x00, 0x00};
static char Source_Timing_Setting[23]= {
	0xC4, 0x70, 0x00, 0x00,
	0x05, 0x00, 0x05, 0x00,
	0x00, 0x00, 0x05, 0x05,
	0x00, 0x00, 0x00, 0x05,
	0x00, 0x05, 0x00, 0x00,
	0x00, 0x05, 0x05};
static char LTPS_Timing_Setting[41]= {
	0xC6, 0x01, 0x71, 0x07,
	0x65, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x09, 0x19, 0x09,
	0x01, 0xE0, 0x01, 0x71,
	0x07, 0x65, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x09, 0x19,
	0x09};
static char Panal_PIN_Control[10]= {
	0xCB, 0x66, 0xE0, 0x87,
	0x61, 0x00, 0x00, 0x00,
	0x00, 0xC0};
static char Panel_Interface_Control[2] = {
	0xCC, 0x03};
static char GPO_Control[6]= {
	0xCF, 0x00, 0x00, 0xC1,
	0x05, 0x3F};
static char Power_Setting_0[15]= {
	0xD0, 0x00, 0x00, 0x19,
	0x18, 0x99, 0x9C, 0x1C,
	0x01, 0x81, 0x00, 0xBB,
	0x56, 0x49, 0x01};
static char Power_Setting_1[30]= {
	0xD1, 0x20, 0x00, 0x00,
	0x04, 0x08, 0x0C, 0x10,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x3C, 0x04, 0x20,
	0x00, 0x00, 0x04, 0x08,
	0x0C, 0x10, 0x00, 0x00,
	0x3C, 0x06, 0x40, 0x00,
	0x32, 0x31};
static char Power_Setting_for_Internal_Power [35]= {
	0xD3, 0x1B, 0x33, 0xBB,
	0xBB, 0xB3, 0x33, 0x33,
	0x33, 0x00, 0x01, 0x00,
	0xA0, 0xD8, 0xA0, 0x00,
	0x3F, 0x33, 0x33, 0x22,
	0x70, 0x02, 0x37, 0x53,
	0x3D, 0xBF, 0x99};
static char VCOM_Setting[8]= {
	0xD5, 0x06, 0x00, 0x00,
	0x01, 0x2D, 0x01, 0x2D};
static char Sequencer_Control[3]= {
	0xD9, 0x20 ,0x00};
static char Panel_syncronous_output_1[3]= {
	0xEC, 0x40, 0x10};
static char Panel_syncronous_output_2[4]= {
	0xED, 0x00, 0x00, 0x00};
static char Panel_syncronous_output_3[3]= {
	0xEE, 0x00, 0x32};
static char Panel_syncronous_output_4[13]= {
	0xEF, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00};
static char Backlght_Control_2[8]= {
	0xB9, 0x0F, 0x18, 0x04,
	0x40, 0x9F, 0x1F, 0x80};
static char BackLight_Control_4[8]= {
	0xBA, 0x0F, 0x18, 0x04,
	0x40, 0x9F, 0x1F, 0xD7};
static char Color_enhancement[33]= {
	0xCA, 0x01, 0x80, 0xDC,
	0xF0, 0xDC, 0xF0, 0xDC,
	0xF0, 0x18, 0x3F, 0x14,
	0x8A, 0x0A, 0x4A, 0x37,
	0xA0, 0x55, 0xF8, 0x0C,
	0x0C, 0x20, 0x10, 0x3F,
	0x3F, 0x00, 0x00, 0x10,
	0x10, 0x3F, 0x3F, 0x3F,
	0x3F};
static char BackLight_Control_6[8]= {
	0xCE, 0x00, 0x07, 0x00,
	0xC1, 0x24, 0xB2, 0x02};
static char ContrastOptimize[7]= {
	0xD8, 0x01, 0x80, 0x80,
	0x40, 0x42, 0x21};
static char Outline_Sharpening_Control[3]= {
	0xDD, 0x31, 0xB3};
static char Test_Image_Generator[7]= {
	0xDE, 0x00, 0xFF, 0x07,
	0x10, 0x00, 0x77};
static char gamma_setting_red[25]= {
	0xC7, 0x01, 0x0A, 0x11,
	0x1A, 0x29, 0x45, 0x3B,
	0x4E, 0x5B, 0x64, 0x6C,
	0x75, 0x01, 0x0A, 0x11,
	0x1A, 0x28, 0x41, 0x38,
	0x4C, 0x59, 0x63, 0x6B,
	0x74};
static char gamma_setting_green[25]= {
	0xC8, 0x01, 0x0A, 0x11,
	0x1A, 0x29, 0x45, 0x3B,
	0x4E, 0x5B, 0x64, 0x6C,
	0x75, 0x01, 0x0A, 0x11,
	0x1A, 0x28, 0x41, 0x38,
	0x4C, 0x59, 0x63, 0x6B,
	0x74};
static char gamma_setting_blue[25]= {
	0xC9, 0x01, 0x0A, 0x11,
	0x1A, 0x29, 0x45, 0x3B,
	0x4E, 0x5B, 0x64, 0x6C,
	0x75, 0x01, 0x0A, 0x11,
	0x1A, 0x28, 0x41, 0x38,
	0x4C, 0x59, 0x63, 0x6B,
	0x74};
#endif
static char BackLight_Control_6[8]= {
	0xCE, 0x00, 0x07, 0x00,
	0xC1, 0x24, 0xB2, 0x02};
static char Manufacture_Command_setting[4] = {0xD6, 0x01};
static char nop[4] = {0x00, 0x00};
static char CABC[2] = {0x55, 0x00};
static char hsync_output[4] = {0xC3, 0x01, 0x00, 0x10};
static char protect_off[4] = {0xB0, 0x03};
static char TE_OUT[4] = {0x35, 0x00};
static char deep_standby_off[2] = {0xB1, 0x01};

static struct dsi_cmd_desc sharp_video_on_cmds[] = {
#if 0
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(interface_setting_0), interface_setting_0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(interface_setting_1), interface_setting_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(interface_id_setting), interface_id_setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(DSI_Control), DSI_Control},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(External_Clock_Setting), External_Clock_Setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Slew_rate_adjustment), Slew_rate_adjustment},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Display_Setting1common), Display_Setting1common},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Display_Setting2), Display_Setting2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(TPC_Sync_Control), TPC_Sync_Control},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Source_Timing_Setting), Source_Timing_Setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(LTPS_Timing_Setting), LTPS_Timing_Setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Panal_PIN_Control), Panal_PIN_Control},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Panel_Interface_Control), Panel_Interface_Control},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(GPO_Control), GPO_Control},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Power_Setting_0), Power_Setting_0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Power_Setting_1), Power_Setting_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Power_Setting_for_Internal_Power), Power_Setting_for_Internal_Power},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(VCOM_Setting), VCOM_Setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Sequencer_Control), Sequencer_Control},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Panel_syncronous_output_1), Panel_syncronous_output_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Panel_syncronous_output_2), Panel_syncronous_output_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Panel_syncronous_output_3), Panel_syncronous_output_3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Panel_syncronous_output_4), Panel_syncronous_output_4},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Backlght_Control_2), Backlght_Control_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(BackLight_Control_4), BackLight_Control_4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Color_enhancement), Color_enhancement},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(BackLight_Control_6), BackLight_Control_6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(ContrastOptimize), ContrastOptimize},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Outline_Sharpening_Control), Outline_Sharpening_Control},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Test_Image_Generator), Test_Image_Generator},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_setting_red), gamma_setting_red},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_setting_green), gamma_setting_green},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_setting_blue), gamma_setting_blue},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(write_display_brightnes), write_display_brightnes},
#endif
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(interface_setting_0), interface_setting_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(BackLight_Control_6), BackLight_Control_6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(CABC), CABC},

        
        {DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(exit_sleep), exit_sleep},

};
static struct dsi_cmd_desc sony_video_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(interface_setting_0), interface_setting_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hsync_output), hsync_output},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(BackLight_Control_6), BackLight_Control_6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(Manufacture_Command_setting), Manufacture_Command_setting},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(protect_off), protect_off},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nop), nop},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(CABC), CABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(write_control_display), write_control_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(TE_OUT), TE_OUT},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc sharp_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 16,
		sizeof(display_off), display_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 64,
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
#if 0
static char manufacture_id[2] = {0x04, 0x00}; 

static struct dsi_cmd_desc renesas_manufacture_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id), manufacture_id};

static uint32 mipi_renesas_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	uint32 *lp;

	tp = &renesas_tx_buf;
	rp = &renesas_rx_buf;
	cmd = &renesas_manufacture_id_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 3);
	lp = (uint32 *)rp->data;
	pr_info("%s: manufacture_id=%x", __func__, *lp);
	return *lp;
}
#endif
static int resume_blk = 1;
static int first_init = 1;
struct i2c_client *pwm_client;
static int mipi_renesas_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;

	if (first_init)
		first_init = 0;
	else {
		if (mipi->mode == DSI_VIDEO_MODE) {
			mipi_dsi_cmds_tx(&renesas_tx_buf, renesas_video_on_cmds,
				renesas_video_on_cmds_count);
		}
	}

	
	PR_DISP_INFO("mipi_renesas_lcd_on\n");
	return 0;
}

static int mipi_renesas_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	resume_blk = 1;
	PR_DISP_INFO("mipi_renesas_lcd_off\n");
	return 0;
}

static int __devinit mipi_renesas_lcd_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		mipi_renesas_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	return 0;
}

static void renesas_display_on(struct msm_fb_data_type *mfd)
{
	
	msleep(120);

	mutex_lock(&mfd->dma->ov_mutex);
	mipi_dsi_cmds_tx(&renesas_tx_buf, renesas_display_on_cmds,
			ARRAY_SIZE(renesas_display_on_cmds));
	mutex_unlock(&mfd->dma->ov_mutex);
	PR_DISP_INFO("renesas_display_on\n");
}

static void renesas_display_off(struct msm_fb_data_type *mfd)
{
	mutex_lock(&mfd->dma->ov_mutex);
	mipi_dsi_cmds_tx(&renesas_tx_buf, renesas_display_off_cmds,
			renesas_display_off_cmds_count);

	mutex_unlock(&mfd->dma->ov_mutex);
	PR_DISP_INFO("renesas_display_off");
}

#define PWM_MIN                   6
#define PWM_DEFAULT               102
#define PWM_MAX                   255

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 143
#define BRI_SETTING_MAX                 255

static unsigned char renesas_shrink_pwm(int val)
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

static void mipi_renesas_set_backlight(struct msm_fb_data_type *mfd)
{
	struct mipi_panel_info *mipi;
	int rc;

	mipi  = &mfd->panel_info.mipi;

	mutex_lock(&mfd->dma->ov_mutex);
	if (mdp4_overlay_dsi_state_get() <= ST_DSI_SUSPEND) {
		mutex_unlock(&mfd->dma->ov_mutex);
		return;
	 }

	write_display_brightnes[2] = renesas_shrink_pwm(mfd->bl_level);

	if (resume_blk) {
		resume_blk = 0;
		rc = i2c_smbus_write_byte_data(pwm_client, 0x10, 0xC5);
		if (rc)
			pr_err("i2c write fail\n");
		rc = i2c_smbus_write_byte_data(pwm_client, 0x19, 0x13);
		if (rc)
			pr_err("i2c write fail\n");
		rc = i2c_smbus_write_byte_data(pwm_client, 0x14, 0xC2);
		if (rc)
			pr_err("i2c write fail\n");
		rc = i2c_smbus_write_byte_data(pwm_client, 0x79, 0xFF);
		if (rc)
			pr_err("i2c write fail\n");
		rc = i2c_smbus_write_byte_data(pwm_client, 0x1D, 0xFA);
		if (rc)
			pr_err("i2c write fail\n");
	}

	mipi_dsi_cmds_tx(&renesas_tx_buf, renesas_cmd_backlight_cmds,
			ARRAY_SIZE(renesas_cmd_backlight_cmds));

	mutex_unlock(&mfd->dma->ov_mutex);
	return;
}
static struct platform_driver this_driver = {
	.probe  = mipi_renesas_lcd_probe,
	.driver = {
		.name   = "mipi_renesas",
	},
};

static struct msm_fb_panel_data renesas_panel_data = {
	.on		= mipi_renesas_lcd_on,
	.off	= mipi_renesas_lcd_off,
	.set_backlight = mipi_renesas_set_backlight,
	.display_on = renesas_display_on,
	.display_off = renesas_display_off,
};


static int ch_used[3];

int mipi_renesas_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_renesas_lcd_init();
	if (ret) {
		pr_err("mipi_renesas_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_renesas", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	renesas_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &renesas_panel_data,
		sizeof(renesas_panel_data));
	if (ret) {
		pr_err("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_err("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}
static const struct i2c_device_id pwm_i2c_id[] = {
	{ "pwm_i2c", 0 },
	{ }
};

static int pwm_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int rc;

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_BYTE | I2C_FUNC_I2C))
		return -ENODEV;
	pwm_client = client;

	return rc;
}

static struct i2c_driver pwm_i2c_driver = {
	.driver = {
		.name = "pwm_i2c",
		.owner = THIS_MODULE,
	},
	.probe = pwm_i2c_probe,
	.remove =  __exit_p( pwm_i2c_remove),
	.id_table =  pwm_i2c_id,
};
static void __exit pwm_i2c_remove(void)
{
	i2c_del_driver(&pwm_i2c_driver);
}
static int mipi_renesas_lcd_init(void)
{
	int ret;

	mipi_dsi_buf_alloc(&renesas_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&renesas_rx_buf, DSI_BUF_SIZE);
	ret = i2c_add_driver(&pwm_i2c_driver);

	if (ret)
		pr_err(KERN_ERR "%s: failed to add i2c driver\n", __func__);

	if (panel_type == PANEL_ID_DLX_SHARP_RENESAS ) {
		renesas_video_on_cmds = sharp_video_on_cmds;
		renesas_display_off_cmds = sharp_display_off_cmds;
		renesas_video_on_cmds_count = ARRAY_SIZE(sharp_video_on_cmds);
		renesas_display_off_cmds_count =ARRAY_SIZE(sharp_display_off_cmds);
		PR_DISP_INFO("panel_type = PANEL_ID_DLX_SHARP_RENESAS");
	} else if (panel_type == PANEL_ID_DLX_SONY_RENESAS ) {
		renesas_video_on_cmds = sony_video_on_cmds;
		renesas_display_off_cmds = sony_display_off_cmds;
		renesas_video_on_cmds_count = ARRAY_SIZE(sony_video_on_cmds);
		renesas_display_off_cmds_count =ARRAY_SIZE(sony_display_off_cmds);
		PR_DISP_INFO("panel_type = PANEL_ID_DLX_SONY_RENESAS");
	} else {
		renesas_video_on_cmds = sharp_video_on_cmds;
		renesas_display_off_cmds = sharp_display_off_cmds;
		renesas_video_on_cmds_count = ARRAY_SIZE(sharp_video_on_cmds);
		renesas_display_off_cmds_count =ARRAY_SIZE(sharp_display_off_cmds);
		PR_DISP_INFO("Default panel_type = PANEL_ID_DLX_SHARP_RENESAS");
	}

	return platform_driver_register(&this_driver);
}
