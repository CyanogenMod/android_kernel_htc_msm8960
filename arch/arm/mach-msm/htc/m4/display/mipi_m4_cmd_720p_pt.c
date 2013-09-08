#include "../../../drivers/video/msm/msm_fb.h"
#include "../../../drivers/video/msm/mipi_dsi.h"
#include "mipi_m4.h"
#include <mach/panel_id.h>

static struct msm_panel_info pinfo;

static struct mipi_dsi_phy_ctrl dsi_sharp_cmd_mode_phy_db = {
	{0x09, 0x08, 0x05, 0x00, 0x20},

	{0xb9, 0x2a, 0x20, 0x00, 0x24, 0x50, 0x1d, 0x2a, 0x24,
	0x03, 0x04, 0xa0},

	{0x5f, 0x00, 0x00, 0x10},

	{0xff, 0x00, 0x06, 0x00},

	{0x0, 0xdf, 0xb1, 0xda, 0x00, 0x50, 0x48, 0x63,
	0x41, 0x0f, 0x01,
	0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01},
};

#if 0
static struct mipi_dsi_reg_set dsi_sharp_cmd_mode_reg_db[] = {
	{0x0300, 0xC0},
	{0x0304, 0xEF},
	{0x0318, 0x00},
	{0x0340, 0xC0},
	{0x0344, 0xEF},
	{0x0358, 0x00},
	{0x0380, 0xC0},
	{0x0384, 0xEF},
	{0x0398, 0x00},
	{0x0400, 0xC0},
	{0x0404, 0xEF},
	{0x0418, 0x88},
};
#endif

static int __init mipi_cmd_m4_720p_pt_init(void)
{
	int ret;

	pinfo.xres = 720;
	pinfo.yres = 1280;
	pinfo.type = MIPI_CMD_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;

	pinfo.width = 53;
	pinfo.height = 94;

	if (panel_type == PANEL_ID_KIWI_SHARP_HX) {
		pinfo.lcdc.h_back_porch = 29;
		pinfo.lcdc.h_front_porch = 55;
		pinfo.lcdc.h_pulse_width = 16;
		pinfo.lcdc.v_back_porch = 1;
		pinfo.lcdc.v_front_porch = 2;
		pinfo.lcdc.v_pulse_width = 1;
	} else if (panel_type == PANEL_ID_KIWI_AUO_NT) {
		pinfo.lcdc.h_back_porch = 5;
		pinfo.lcdc.h_front_porch = 3;
		pinfo.lcdc.h_pulse_width = 70;
		pinfo.lcdc.v_back_porch = 1;
		pinfo.lcdc.v_front_porch = 1;
		pinfo.lcdc.v_pulse_width = 1;
	}

	pinfo.lcd.primary_vsync_init = pinfo.yres;
	pinfo.lcd.primary_rdptr_irq = 0;
	pinfo.lcd.primary_start_pos = pinfo.yres +
		pinfo.lcd.v_back_porch + pinfo.lcd.v_front_porch - 1;

	pinfo.lcdc.border_clr = 0;
	pinfo.lcdc.underflow_clr = 0xff;
	pinfo.lcdc.hsync_skew = 0;

	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	pinfo.clk_rate = 514000000;
	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 6000;

	pinfo.lcd.v_back_porch = 1;
	pinfo.lcd.v_front_porch = 2;
	pinfo.lcd.v_pulse_width = 1;

	pinfo.mipi.mode = DSI_CMD_MODE;
	pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.esc_byte_ratio = 4;

	pinfo.mipi.t_clk_post = 0x04;
	pinfo.mipi.t_clk_pre = 0x1e;
	pinfo.mipi.stream = 0;
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.te_sel = 1;
	pinfo.mipi.interleave_max = 1;
	pinfo.mipi.insert_dcs_cmd = TRUE;
	pinfo.mipi.wr_mem_continue = 0x3c;
	pinfo.mipi.wr_mem_start = 0x2c;
	pinfo.mipi.tx_eot_append = 1;
	pinfo.mipi.dsi_phy_db = &dsi_sharp_cmd_mode_phy_db;
#if 0
	pinfo.mipi.dsi_reg_db = dsi_sharp_cmd_mode_reg_db;
	pinfo.mipi.dsi_reg_db_size = ARRAY_SIZE(dsi_sharp_cmd_mode_reg_db);
#endif
	pinfo.mipi.dlane_swap = 0x00;

	ret = mipi_m4_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_720P_PT);

	if (ret)
		pr_err("%s: failed to register device!\n", __func__);


	return ret;
}

late_initcall(mipi_cmd_m4_720p_pt_init);
