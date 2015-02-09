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


#include "../../../drivers/video/msm/msm_fb.h"
#include "../../../drivers/video/msm/mipi_dsi.h"
#include "mipi_zara.h"
#include <mach/panel_id.h>

static struct msm_panel_info pinfo;

static struct mipi_dsi_phy_ctrl mipi_dsi_lg_novatek_phy_ctrl = {
	/* DSI_BIT_CLK at 482MHz, 2 lane, RGB888 */
	/* regulator */
	{0x09, 0x08, 0x05, 0x00, 0x20},
	/* timing */
	/* clk_rate:482MHz */
	{0x7F, 0x1C, 0x13, 0x00, 0x41, 0x49, 0x17,
	0x1F, 0x20, 0x03, 0x04, 0xa0},
	/* phy ctrl */
	{0x5F, 0x00, 0x00, 0x10},
	/* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
	{0x00, 0x52, 0x30, 0xc4, 0x00, 0x10, 0x07, 0x62,
	0x71, 0x88, 0x99,
	0x0, 0x14, 0x03, 0x0, 0x2, 0x0e, 0x01, 0x0, 0x01, 0x0},
};

static int __init mipi_cmd_lg_novatek_init(void)
{
	int ret;

	pinfo.xres = 540;
	pinfo.yres = 960;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 56;
	pinfo.height = 99;

	pinfo.lcdc.h_back_porch = 60;
	pinfo.lcdc.h_front_porch = 20;
	pinfo.lcdc.h_pulse_width = 4;
	pinfo.lcdc.v_back_porch = 20;
	pinfo.lcdc.v_front_porch = 20;
	pinfo.lcdc.v_pulse_width = 2;
	pinfo.clk_rate = 463000000;

	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = FALSE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = FALSE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_BURST_MODE;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;

	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x04;
	pinfo.mipi.t_clk_pre = 0x1B;
	pinfo.mipi.stream = 0;	/* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;

	pinfo.mipi.dsi_phy_db = &mipi_dsi_lg_novatek_phy_ctrl;

	ret = mipi_zara_device_register(&pinfo, MIPI_DSI_PRIM,
		MIPI_DSI_PANEL_QHD_PT);

	if (ret)
		pr_err("%s: failed to register device!\n", __func__);

	pr_info("%s: panel_type=PANEL_ID_CANIS_LG_NOVATEK\n", __func__);

	return ret;
}

static int __init mipi_cmd_jdi_novatek_init(void)
{
	int ret;

	pinfo.xres = 540;
	pinfo.yres = 960;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.width = 56;
	pinfo.height = 99;

	pinfo.lcdc.h_back_porch = 60;
	pinfo.lcdc.h_front_porch = 20;
	pinfo.lcdc.h_pulse_width = 4;
	pinfo.lcdc.v_back_porch = 20;
	pinfo.lcdc.v_front_porch = 20;
	pinfo.lcdc.v_pulse_width = 2;
	pinfo.clk_rate = 463000000;

	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = FALSE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = FALSE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_BURST_MODE;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;

	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post = 0x04;
	pinfo.mipi.t_clk_pre = 0x1B;
	pinfo.mipi.stream = 0;	/* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;

	pinfo.mipi.dsi_phy_db = &mipi_dsi_lg_novatek_phy_ctrl;

	ret = mipi_zara_device_register(&pinfo, MIPI_DSI_PRIM,
		MIPI_DSI_PANEL_QHD_PT);

	if (ret)
		pr_err("%s: failed to register device!\n", __func__);

	return ret;
}

static int __init zara_init_panel(void)
{
	if(panel_type == PANEL_ID_NONE) {
		pr_info("%s panel ID = PANEL_ID_NONE\n", __func__);
		return -ENODEV;
	}

	if (panel_type == PANEL_ID_CANIS_LG_NOVATEK)
		mipi_cmd_lg_novatek_init();
	else if (panel_type == PANEL_ID_CANIS_JDI_NOVATEK)
		mipi_cmd_jdi_novatek_init();

	return 0;
}

late_initcall(zara_init_panel);
