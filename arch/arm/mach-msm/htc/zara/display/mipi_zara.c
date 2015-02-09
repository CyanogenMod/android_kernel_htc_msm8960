/* linux/arch/arm/mach-msm/zara/display/mipi-zara.c
 *
 * Copyright (c) 2013 HTC.
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

#include <linux/gpio.h>

#include <mach/panel_id.h>
#include "../../../drivers/video/msm/msm_fb.h"
#include "../../../drivers/video/msm/mipi_dsi.h"
#include <linux/leds.h>
#include <mach/board.h>
#include "mipi_zara.h"
#include "../board-zara.h"

static struct mipi_dsi_panel_platform_data *mipi_zara_pdata;
static struct dsi_cmd_desc *panel_on_cmds = NULL;
static struct dsi_cmd_desc *display_off_cmds = NULL;
static struct dsi_cmd_desc *backlight_cmds = NULL;
static int panel_on_cmds_count = 0;
static int display_off_cmds_count = 0;
static int backlight_cmds_count = 0;
static atomic_t lcd_power_state;

static char sleep_out[] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */
static char sleep_in[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */

static char display_on[2] = {0x29, 0x00};

#ifdef CONFIG_CABC_DIMMING_SWITCH
static struct dsi_cmd_desc *dim_on_cmds = NULL;
static struct dsi_cmd_desc *dim_off_cmds = NULL;
static int dim_on_cmds_count = 0;
static int dim_off_cmds_count = 0;

static char dsi_novatek_dim_on[] = {0x53, 0x2C};
static char dsi_novatek_dim_off[] = {0x53, 0x24};

static struct dsi_cmd_desc novatek_dim_on_cmds[] = {
        {DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_novatek_dim_on), dsi_novatek_dim_on},
};
static struct dsi_cmd_desc novatek_dim_off_cmds[] = {
        {DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(dsi_novatek_dim_off), dsi_novatek_dim_off},
};
#endif

static char led_pwm[2] = {0x51, 0xF0};
static char display_off[2] = {0x28, 0x00}; /* DTYPE_DCS_WRITE */
static char pwr_ctrl_AVDD[] = {0xB6, 0x34};
static char pwr_ctrl_AVEE[] = {0xB7, 0x34};
static char pwr_ctrl_VCL[] = {0xB8, 0x13};
static char pwr_ctrl_VGH[] = {0xB9, 0x24};
static char pwr_ctrl_VGLX[] = {0xBA, 0x23};
static char set_VGMP_VGSP_vol[] = {0xBC, 0x00, 0x88, 0x00};
static char set_VGMN_VGSN_vol[] = {0xBD, 0x00, 0x84, 0x00};
static char GPO_ctrl[] = {0xC0, 0x04, 0x00};
static char gamma_curve_ctrl[] = {0xCF, 0x04};


static char cmd_set_page1[] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x08, 0x01
};
static char gamma_corr_red1[] = {
	0xD1, 0x00, 0x00, 0x00,
	0x48, 0x00, 0x71, 0x00,
	0x95, 0x00, 0xA4, 0x00,
	0xC1, 0x00, 0xD4, 0x00,
	0xFA
};
static char gamma_corr_red2[] = {
	0xD2, 0x01, 0x22, 0x01,
	0x5F, 0x01, 0x89, 0x01,
	0xCC, 0x02, 0x03, 0x02,
	0x05, 0x02, 0x38, 0x02,
	0x71
};
static char gamma_corr_red3[] = {
	0xD3, 0x02, 0x90, 0x02,
	0xC9, 0x02, 0xF4, 0x03,
	0x1A, 0x03, 0x35, 0x03,
	0x52, 0x03, 0x62, 0x03,
	0x76
};
static char gamma_corr_red4[] = {
	0xD4, 0x03, 0x8F, 0x03,
	0xC0
};
static char normal_display_mode_on[] = {0x13, 0x00};

static char cmd_set_page0[] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x08, 0x00
};
static char disp_opt_ctrl[] = {
	0xB1, 0x68, 0x00, 0x01
};
static char disp_scan_line_ctrl[] = {0xB4, 0x78};
static char eq_ctrl[] = {
	0xB8, 0x01, 0x02, 0x02,
	0x02
};
static char inv_drv_ctrl[] = {0xBC, 0x00, 0x00, 0x00};
static char display_timing_control[] = {
	0xC9, 0x63, 0x06, 0x0D,
	0x1A, 0x17, 0x00
};
static char write_ctrl_display[] = {0x53, 0x24};
static char te_on[] = {0x35, 0x00};
static char pwm_freq_ctrl[] = {0xE0, 0x01, 0x03};
static char pwr_blk_enable[] = {
	0xFF, 0xAA, 0x55, 0x25,
	0x01};
static char pwr_blk_disable[] = {
	0xFF, 0xAA, 0x55, 0x25,
	0x00};
static char set_para_idx[] = {0x6F, 0x0A};
static char pwr_blk_sel[] = {0xFA, 0x03};
static char bkl_off[] = {0x53, 0x00};
static char set_cabc_level[] = {0x55, 0x92};
static char vivid_color_setting[] = {
	0xD6, 0x00, 0x05, 0x10,
	0x17, 0x22, 0x26, 0x29,
	0x29, 0x26, 0x23, 0x17,
	0x12, 0x06, 0x02, 0x01,
	0x00};
static char cabc_still[] = {
	0xE3, 0xFF, 0xFB, 0xF3,
	0xEC, 0xE2, 0xCA, 0xC3,
	0xBC, 0xB5, 0xB3
};
static char idx_13[] = {0x6F, 0x13};
static char idx_14[] = {0x6F, 0x14};
static char idx_15[] = {0x6F, 0x15};
static char val_80[] = {0xF5, 0x80};
static char val_FF[] = {0xF5, 0xFF};
static char ctrl_ie_sre[] = {
	0xD4, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00};
static char skin_tone_setting1[] = {
	0xD7, 0x30, 0x30, 0x30,
	0x28, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00};
static char skin_tone_setting2[] = {
	0xD8, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x28,
	0x30, 0x00};

static struct dsi_cmd_desc lg_novatek_video_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(cmd_set_page1), cmd_set_page1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_ctrl_AVDD), pwr_ctrl_AVDD},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_ctrl_AVEE), pwr_ctrl_AVEE},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_ctrl_VCL), pwr_ctrl_VCL},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_ctrl_VGH), pwr_ctrl_VGH},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_ctrl_VGLX), pwr_ctrl_VGLX},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(set_VGMP_VGSP_vol), set_VGMP_VGSP_vol},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(set_VGMN_VGSN_vol), set_VGMN_VGSN_vol},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(GPO_ctrl), GPO_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(gamma_curve_ctrl), gamma_curve_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(gamma_corr_red1), gamma_corr_red1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(gamma_corr_red2), gamma_corr_red2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(gamma_corr_red3), gamma_corr_red3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(gamma_corr_red4), gamma_corr_red4},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out), sleep_out},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(cmd_set_page0), cmd_set_page0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(disp_opt_ctrl), disp_opt_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(disp_scan_line_ctrl), disp_scan_line_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(eq_ctrl), eq_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(inv_drv_ctrl), inv_drv_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(display_timing_control), display_timing_control},
	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(te_on), te_on},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(pwm_freq_ctrl), pwm_freq_ctrl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(cabc_still), cabc_still},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(write_ctrl_display), write_ctrl_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(set_cabc_level), set_cabc_level},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(ctrl_ie_sre), ctrl_ie_sre},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(vivid_color_setting), vivid_color_setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(skin_tone_setting1), skin_tone_setting1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(skin_tone_setting2), skin_tone_setting2},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(pwr_blk_enable), pwr_blk_enable},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(set_para_idx), set_para_idx},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwr_blk_sel), pwr_blk_sel},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_13), idx_13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_80), val_80},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_14), idx_14},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_FF), val_FF},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_15), idx_15},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_FF), val_FF},
	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(normal_display_mode_on), normal_display_mode_on},
	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc lg_novatek_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(bkl_off), bkl_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 45, sizeof(display_off), display_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 130, sizeof(sleep_in), sleep_in}
};


static char RGBCTR[] = {
	0xB0, 0x00, 0x16, 0x14,
	0x34, 0x34};
static char DPRSLCTR[] = {0xB2, 0x54, 0x01, 0x80};
static char SDHDTCTR[] = {0xB6, 0x0A};
static char GSEQCTR[] = {0xB7, 0x00, 0x22};
static char SDEQCTR[] = {
	0xB8, 0x00, 0x00, 0x07,
	0x00};
static char SDVPCTR[] = {0xBA, 0x02};
static char SGOPCTR[] = {0xBB, 0x44, 0x40};
static char DPFRCTR1[] = {
	0xBD, 0x01, 0xD1, 0x16,
	0x14};
static char DPTMCTR10_2[] = {0xC1, 0x03};
static char DPTMCTR10[] = {
	0xCA, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00};
static char PWMFRCTR[] = {0xE0, 0x01, 0x03};
static char SETAVDD[] = {0xB0, 0x07};
static char SETAVEE[] = {0xB1, 0x07};
static char SETVCL[] = {0xB2, 0x00};
static char SETVGH[] = {0xB3, 0x10};
static char SETVGLX[] = {0xB4, 0x0A};
static char BT1CTR[] = {0xB6, 0x34};
static char BT2CTR[] = {0xB7, 0x35};
static char BT3CTR[] = {0xB8, 0x16};
static char BT4CTR[] = {0xB9, 0x33};
static char BT5CTR[] = {0xBA, 0x15};
static char SETVGL_REG[] = {0xC4, 0x05};
static char GSVCTR[] = {0xCA, 0x21};
static char MAUCCTR[] = {
	0xF0, 0x55, 0xAA, 0x52,
	0x00, 0x00};
static char SETPARIDX[] = {0x6F, 0x01};
static char skew_delay_en[] = {0xF8, 0x24};
static char skew_delay[] = {0xFC, 0x41};

static struct dsi_cmd_desc jdi_novatek_video_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cmd_set_page0), cmd_set_page0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(RGBCTR), RGBCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(DPRSLCTR), DPRSLCTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SDHDTCTR), SDHDTCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(GSEQCTR), GSEQCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(SDEQCTR), SDEQCTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SDVPCTR), SDVPCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(SGOPCTR), SGOPCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(DPFRCTR1), DPFRCTR1},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(DPTMCTR10_2), DPTMCTR10_2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(DPTMCTR10), DPTMCTR10},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(PWMFRCTR), PWMFRCTR},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(cabc_still), cabc_still},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(write_ctrl_display), write_ctrl_display},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(set_cabc_level), set_cabc_level},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(ctrl_ie_sre), ctrl_ie_sre},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(vivid_color_setting), vivid_color_setting},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(skin_tone_setting1), skin_tone_setting1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(skin_tone_setting2), skin_tone_setting2},

	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(pwr_blk_enable), pwr_blk_enable},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_13), idx_13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_80), val_80},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_14), idx_14},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_FF), val_FF},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(idx_15), idx_15},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(val_FF), val_FF},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETPARIDX), SETPARIDX},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(skew_delay_en), skew_delay_en},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETPARIDX), SETPARIDX},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(skew_delay), skew_delay},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(pwr_blk_disable), pwr_blk_disable},

	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(cmd_set_page1), cmd_set_page1},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETAVDD), SETAVDD},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETAVEE), SETAVEE},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETVCL), SETVCL},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETVGH), SETVGH},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETVGLX), SETVGLX},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT1CTR), BT1CTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT2CTR), BT2CTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT3CTR), BT3CTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT4CTR), BT4CTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(BT5CTR), BT5CTR},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(SETVGL_REG), SETVGL_REG},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 1, sizeof(GSVCTR), GSVCTR},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(MAUCCTR), MAUCCTR},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out), sleep_out},
	{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc lg_novatek_cmd_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(led_pwm), led_pwm},
};

static struct dsi_cmd_desc jdi_novatek_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 40, sizeof(display_off), display_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(sleep_in), sleep_in}
};

static int zara_send_display_cmds(struct dsi_cmd_desc *cmd, int cnt)
{
	int ret = 0;
	struct dcs_cmd_req cmdreq;

	cmdreq.cmds = cmd;
	cmdreq.cmds_cnt = cnt;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	ret = mipi_dsi_cmdlist_put(&cmdreq);
	if (ret < 0)
		pr_err("%s failed (%d)\n", __func__, ret);
	return ret;
}

static int mipi_zara_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (panel_type == PANEL_ID_CANIS_LG_NOVATEK) {
		gpio_set_value(MSM_LCD_RSTz, 1);
		hr_msleep(5);
		gpio_set_value(MSM_LCD_RSTz, 0);
		hr_msleep(5);
		gpio_set_value(MSM_LCD_RSTz, 1);
		hr_msleep(30);
	}

	zara_send_display_cmds(panel_on_cmds, panel_on_cmds_count);

	atomic_set(&lcd_power_state, 1);

	return 0;
}

static int mipi_zara_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	atomic_set(&lcd_power_state, 0);

	return 0;
}

static int mipi_zara_display_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	zara_send_display_cmds(display_off_cmds, display_off_cmds_count);

	return 0;
}

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 143
#define BRI_SETTING_MAX                 255

static unsigned char zara_shrink_pwm(int val)
{
	unsigned int pwm_min, pwm_default, pwm_max;
	unsigned char shrink_br = BRI_SETTING_MAX;

	pwm_min = 7;
	pwm_default = 86;
	pwm_max = 255;

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

	pr_info("brightness orig=%d, transformed=%d\n", val, shrink_br);

	return shrink_br;
}

static void zara_set_backlight(struct msm_fb_data_type *mfd)
{
	led_pwm[1] = zara_shrink_pwm((unsigned char)(mfd->bl_level));

	if (atomic_read(&lcd_power_state) == 0) {
		pr_debug("%s: LCD is off. Skip backlight setting\n", __func__);
		return;
	}

#ifdef CONFIG_CABC_DIMMING_SWITCH
	if (led_pwm[1] == 0)
	zara_send_display_cmds(dim_off_cmds, dim_off_cmds_count);
#endif
	zara_send_display_cmds(backlight_cmds, backlight_cmds_count);
}

#ifdef CONFIG_CABC_DIMMING_SWITCH
static void zara_dim_on(struct msm_fb_data_type *mfd)
{
	zara_send_display_cmds(dim_on_cmds, dim_on_cmds_count);
}
#endif

static void lg_novatek_panel_init(void)
{
	panel_on_cmds = lg_novatek_video_on_cmds;
	panel_on_cmds_count = ARRAY_SIZE(lg_novatek_video_on_cmds);
	display_off_cmds = lg_novatek_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(lg_novatek_display_off_cmds);
	backlight_cmds = lg_novatek_cmd_backlight_cmds;
	backlight_cmds_count = ARRAY_SIZE(lg_novatek_cmd_backlight_cmds);
#ifdef CONFIG_CABC_DIMMING_SWITCH
	dim_on_cmds = novatek_dim_on_cmds;
	dim_on_cmds_count = ARRAY_SIZE(novatek_dim_on_cmds);
	dim_off_cmds = novatek_dim_off_cmds;
	dim_off_cmds_count = ARRAY_SIZE(novatek_dim_off_cmds);
#endif
}

static void jdi_novatek_panel_init(void)
{
	panel_on_cmds = jdi_novatek_video_on_cmds;
	panel_on_cmds_count = ARRAY_SIZE(jdi_novatek_video_on_cmds);
	display_off_cmds = jdi_novatek_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(jdi_novatek_display_off_cmds);
	backlight_cmds = lg_novatek_cmd_backlight_cmds;
	backlight_cmds_count = ARRAY_SIZE(lg_novatek_cmd_backlight_cmds);
#ifdef CONFIG_CABC_DIMMING_SWITCH
	dim_on_cmds = novatek_dim_on_cmds;
	dim_on_cmds_count = ARRAY_SIZE(novatek_dim_on_cmds);
	dim_off_cmds = novatek_dim_off_cmds;
	dim_off_cmds_count = ARRAY_SIZE(novatek_dim_off_cmds);
#endif
}

static int __devinit mipi_zara_lcd_probe(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	struct platform_device *current_pdev;
	static struct mipi_dsi_phy_ctrl *phy_settings;
	static char dlane_swap;

	if (panel_type == PANEL_ID_CANIS_LG_NOVATEK) {
		lg_novatek_panel_init();
		pr_info("%s: panel_type=PANEL_ID_CANIS_LG_NOVATEK\n", __func__);
	} else if (panel_type == PANEL_ID_CANIS_JDI_NOVATEK) {
		jdi_novatek_panel_init();
		pr_info("%s: panel_type=PANEL_ID_CANIS_JDI_NOVATEK\n", __func__);
	} else {
		pr_err("%s: Unsupported panel type: %d",
				__func__, panel_type);
	}

	atomic_set(&lcd_power_state, 1);

	if (pdev->id == 0) {
		mipi_zara_pdata = pdev->dev.platform_data;

		if (mipi_zara_pdata
			&& mipi_zara_pdata->phy_ctrl_settings) {
			phy_settings = (mipi_zara_pdata->phy_ctrl_settings);
		}

		if (mipi_zara_pdata
			&& mipi_zara_pdata->dlane_swap) {
			dlane_swap = (mipi_zara_pdata->dlane_swap);
		}

		return 0;
	}

	current_pdev = msm_fb_add_device(pdev);

	if (current_pdev) {
		mfd = platform_get_drvdata(current_pdev);
		if (!mfd)
			return -ENODEV;
		if (mfd->key != MFD_KEY)
			return -EINVAL;

		mipi = &mfd->panel_info.mipi;

		if (phy_settings != NULL)
			mipi->dsi_phy_db = phy_settings;

		if (dlane_swap)
			mipi->dlane_swap = dlane_swap;
	}
	return 0;
}

/* HTC specific functions */

static struct platform_driver this_driver = {
	.probe  = mipi_zara_lcd_probe,
	.driver = {
		.name   = "mipi_zara",
	},
};

static struct msm_fb_panel_data zara_panel_data = {
	.on		= mipi_zara_lcd_on,
	.off		= mipi_zara_lcd_off,
	.set_backlight  = zara_set_backlight,
	.early_off = mipi_zara_display_off,
#ifdef CONFIG_CABC_DIMMING_SWITCH
	.dimming_on	= zara_dim_on,
#endif
};

static int ch_used[3];

int mipi_zara_device_register(struct msm_panel_info *pinfo,
		u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = platform_driver_register(&this_driver);
	if (ret) {
		pr_err("platform_driver_register() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_zara", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	zara_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &zara_panel_data,
		sizeof(zara_panel_data));
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
