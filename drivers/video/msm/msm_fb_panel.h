/* Copyright (c) 2008-2012, Code Aurora Forum. All rights reserved.
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

#ifndef MSM_FB_PANEL_H
#define MSM_FB_PANEL_H

#include "msm_fb_def.h"

struct msm_fb_data_type;

typedef void (*msm_fb_vsync_handler_type) (void *arg);

typedef struct panel_id_s {
	uint16 id;
	uint16 type;
} panel_id_type;

#define NO_PANEL		0xffff	
#define MDDI_PANEL		1	
#define EBI2_PANEL		2	
#define LCDC_PANEL		3	
#define EXT_MDDI_PANEL		4	
#define TV_PANEL		5	
#define HDMI_PANEL		6	
#define DTV_PANEL		7	
#define MIPI_VIDEO_PANEL	8	
#define MIPI_CMD_PANEL		9	
#define WRITEBACK_PANEL		10	
#define LVDS_PANEL		11	

typedef enum {
	DISPLAY_LCD = 0,	
	DISPLAY_LCDC,		
	DISPLAY_TV,		
	DISPLAY_EXT_MDDI,	
} DISP_TARGET;

typedef enum {
	DISPLAY_1 = 0,		
	DISPLAY_2,		
	DISPLAY_3,              
	MAX_PHYS_TARGET_NUM,
} DISP_TARGET_PHYS;

struct lcd_panel_info {
	__u32 vsync_enable;
	__u32 refx100;
	__u32 v_back_porch;
	__u32 v_front_porch;
	__u32 v_pulse_width;
	__u32 hw_vsync_mode;
	__u32 vsync_notifier_period;
	__u32 rev;
};

struct lcdc_panel_info {
	__u32 h_back_porch;
	__u32 h_front_porch;
	__u32 h_pulse_width;
	__u32 v_back_porch;
	__u32 v_front_porch;
	__u32 v_pulse_width;
	__u32 border_clr;
	__u32 underflow_clr;
	__u32 hsync_skew;
	
	uint32 xres_pad;
	
	uint32 yres_pad;
	boolean is_sync_active_high;
	
	__u32 no_set_tear;

};

struct mddi_panel_info {
	__u32 vdopkt;
	boolean is_type1;
};

struct mipi_panel_info {
	char mode;		
	char interleave_mode;
	char crc_check;
	char ecc_check;
	char dst_format;	
	char data_lane0;
	char data_lane1;
	char data_lane2;
	char data_lane3;
	char dlane_swap;	
	char rgb_swap;
	char b_sel;
	char g_sel;
	char r_sel;
	char rx_eot_ignore;
	char tx_eot_append;
	char t_clk_post; 
	char t_clk_pre;  
	char vc;	
	struct mipi_dsi_phy_ctrl *dsi_phy_db;
	
	char pulse_mode_hsa_he;
	char hfp_power_stop;
	char hbp_power_stop;
	char hsa_power_stop;
	char eof_bllp_power_stop;
	char bllp_power_stop;
	char traffic_mode;
	char frame_rate;
	
	char interleave_max;
	char insert_dcs_cmd;
	char wr_mem_continue;
	char wr_mem_start;
	char te_sel;
	char stream;	
	char mdp_trigger;
	char dma_trigger;
	uint32 dsi_pclk_rate;
	
	uint32 esc_byte_ratio;
	
	char no_max_pkt_size;
	
	char force_clk_lane_hs;
	
	struct mipi_dsi_reg_set *dsi_reg_db;
	uint32 dsi_reg_db_size;
	
	char force_leave_ulps;
};

enum lvds_mode {
	LVDS_SINGLE_CHANNEL_MODE,
	LVDS_DUAL_CHANNEL_MODE,
};

struct lvds_panel_info {
	enum lvds_mode channel_mode;
	
	char channel_swap;
};

struct msm_panel_info {
	__u32 xres;
	__u32 yres;
	__u32 bpp;
	__u32 mode2_xres;
	__u32 mode2_yres;
	__u32 mode2_bpp;
	__u32 type;
	__u32 wait_cycle;
	DISP_TARGET_PHYS pdest;
	__u32 bl_max;
	__u32 bl_min;
	__u32 fb_num;
	__u32 clk_rate;
	__u32 clk_min;
	__u32 clk_max;
	__u32 frame_count;
	__u32 is_3d_panel;
	__u32 frame_rate;
	__u32 width;
	__u32 height;
	__u32 camera_backlight;
	__u32 read_pointer;

	struct mddi_panel_info mddi;
	struct lcd_panel_info lcd;
	struct lcdc_panel_info lcdc;
	struct mipi_panel_info mipi;
	struct lvds_panel_info lvds;
};

#define MSM_FB_SINGLE_MODE_PANEL(pinfo)		\
	do {					\
		(pinfo)->mode2_xres = 0;	\
		(pinfo)->mode2_yres = 0;	\
		(pinfo)->mode2_bpp = 0;		\
	} while (0)

struct msm_fb_panel_data {
	struct msm_panel_info panel_info;
	void (*set_rect) (int x, int y, int xres, int yres);
	void (*set_vsync_notifier) (msm_fb_vsync_handler_type, void *arg);
	void (*set_backlight) (struct msm_fb_data_type *);

	
	void (*display_on) (struct msm_fb_data_type *);
	void (*display_off) (struct msm_fb_data_type *);
	int (*on) (struct platform_device *pdev);
	int (*off) (struct platform_device *pdev);
	int (*power_ctrl) (boolean enable);
	struct platform_device *next;
	int (*clk_func) (int enable);
#ifdef CONFIG_FB_MSM_CABC
	int (*autobl_enable) (int on, struct msm_fb_data_type *);
	void (*enable_cabc) (int, bool, struct msm_fb_data_type *);
#endif
	void (*color_enhance) (struct msm_fb_data_type *, int on);
	void (*dimming_on) (struct msm_fb_data_type *);
	void (*acl_enable) (int on, struct msm_fb_data_type *);
	void (*set_cabc) (struct msm_fb_data_type *, int mode);
	void (*sre_ctrl) (struct msm_fb_data_type *, unsigned long);
#ifdef CONFIG_FB_MSM_ESD_WORKAROUND
	int esd_workaround;
#endif
};

struct platform_device *msm_fb_device_alloc(struct msm_fb_panel_data *pdata,
						u32 type, u32 id);
int panel_next_on(struct platform_device *pdev);
int panel_next_off(struct platform_device *pdev);

int lcdc_device_register(struct msm_panel_info *pinfo);

int mddi_toshiba_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel);

#endif 
