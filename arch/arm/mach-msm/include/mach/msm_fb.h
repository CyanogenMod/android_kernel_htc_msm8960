/* arch/arm/mach-msm/include/mach/msm_fb.h
 *
 * Internal shared definitions for various MSM framebuffer parts.
 *
 * Copyright (C) 2007 Google Incorporated
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _MSM_FB_H_
#define _MSM_FB_H_

#include <linux/device.h>

struct mddi_info;

#define MSM_MDP_OUT_IF_FMT_RGB565 0
#define MSM_MDP_OUT_IF_FMT_RGB666 1

struct msm_fb_data {
	int xres;	
	int yres;	
	int width;	
	int height;	
	unsigned output_format;
};

struct msmfb_callback {
	void (*func)(struct msmfb_callback *);
};

enum {
	MSM_MDDI_PMDH_INTERFACE = 0,
	MSM_MDDI_EMDH_INTERFACE,
	MSM_EBI2_INTERFACE,
	MSM_LCDC_INTERFACE,

	MSM_MDP_NUM_INTERFACES = MSM_LCDC_INTERFACE + 1,
};

#define MSMFB_CAP_PARTIAL_UPDATES	(1 << 0)

struct msm_panel_data {
	
	int (*suspend)(struct msm_panel_data *);
	
	int (*resume)(struct msm_panel_data *);
	
	int (*blank)(struct msm_panel_data *);
	
	int (*unblank)(struct msm_panel_data *);
	void (*wait_vsync)(struct msm_panel_data *);
	void (*request_vsync)(struct msm_panel_data *, struct msmfb_callback *);
	void (*clear_vsync)(struct msm_panel_data *);
	
	unsigned interface_type;
	
	struct msm_fb_data *fb_data;

	
	uint32_t caps;
};

struct msm_mddi_client_data {
	void (*suspend)(struct msm_mddi_client_data *);
	void (*resume)(struct msm_mddi_client_data *);
	void (*activate_link)(struct msm_mddi_client_data *);
	void (*remote_write)(struct msm_mddi_client_data *, uint32_t val,
			     uint32_t reg);
	uint32_t (*remote_read)(struct msm_mddi_client_data *, uint32_t reg);
	void (*auto_hibernate)(struct msm_mddi_client_data *, int);
	void *private_client_data;
	struct resource *fb_resource;
	
	unsigned interface_type;
};

struct msm_mddi_platform_data {
	unsigned int clk_rate;
	void (*power_client)(struct msm_mddi_client_data *, int on);

	
	void (*fixup)(uint16_t *mfr_name, uint16_t *product_id);

	int vsync_irq;

	struct resource *fb_resource; 
	
	int num_clients;
	
	struct {
		unsigned product_id; 
		char *name;	
		unsigned id;	
		void *client_data;	
		unsigned int clk_rate;	
	} client_platform_data[];
};

struct msm_lcdc_timing {
	unsigned int clk_rate;		
	unsigned int hsync_pulse_width;	
	unsigned int hsync_back_porch;	
	unsigned int hsync_front_porch;	
	unsigned int hsync_skew;	
	unsigned int vsync_pulse_width;	
	unsigned int vsync_back_porch;	
	unsigned int vsync_front_porch;	

	
	unsigned int vsync_act_low:1;
	unsigned int hsync_act_low:1;
	unsigned int den_act_low:1;
};

struct msm_lcdc_panel_ops {
	int	(*init)(struct msm_lcdc_panel_ops *);
	int	(*uninit)(struct msm_lcdc_panel_ops *);
	int	(*blank)(struct msm_lcdc_panel_ops *);
	int	(*unblank)(struct msm_lcdc_panel_ops *);
};

struct msm_lcdc_platform_data {
	struct msm_lcdc_panel_ops	*panel_ops;
	struct msm_lcdc_timing		*timing;
	int				fb_id;
	struct msm_fb_data		*fb_data;
	struct resource			*fb_resource;
};

struct mdp_blit_req;
struct fb_info;
struct mdp_device {
	struct device dev;
	void (*dma)(struct mdp_device *mdp, uint32_t addr,
		    uint32_t stride, uint32_t w, uint32_t h, uint32_t x,
		    uint32_t y, struct msmfb_callback *callback, int interface);
	void (*dma_wait)(struct mdp_device *mdp, int interface);
	int (*blit)(struct mdp_device *mdp, struct fb_info *fb,
		    struct mdp_blit_req *req);
	void (*set_grp_disp)(struct mdp_device *mdp, uint32_t disp_id);
	int (*check_output_format)(struct mdp_device *mdp, int bpp);
	int (*set_output_format)(struct mdp_device *mdp, int bpp);
};

struct class_interface;
int register_mdp_client(struct class_interface *class_intf);


struct msm_mddi_bridge_platform_data {
	
	int (*init)(struct msm_mddi_bridge_platform_data *,
		    struct msm_mddi_client_data *);
	int (*uninit)(struct msm_mddi_bridge_platform_data *,
		      struct msm_mddi_client_data *);
	
	int (*blank)(struct msm_mddi_bridge_platform_data *,
		     struct msm_mddi_client_data *);
	int (*unblank)(struct msm_mddi_bridge_platform_data *,
		       struct msm_mddi_client_data *);
	struct msm_fb_data fb_data;

	
	uint32_t panel_caps;
};


struct mdp_v4l2_req;
int msm_fb_v4l2_enable(struct mdp_overlay *req, bool enable, void **par);
int msm_fb_v4l2_update(void *par,
	unsigned long srcp0_addr, unsigned long srcp0_size,
	unsigned long srcp1_addr, unsigned long srcp1_size,
	unsigned long srcp2_addr, unsigned long srcp2_size);

#endif
