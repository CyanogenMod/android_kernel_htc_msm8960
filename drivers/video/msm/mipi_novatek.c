/* Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
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

#ifdef CONFIG_SPI_QUP
#include <linux/spi/spi.h>
#endif
#include <linux/leds.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_novatek.h"
#include "mdp4.h"

#include <mach/panel_id.h>
#include <mach/debug_display.h>

#define EVA_CMD_MODE_PANEL
#undef EVA_VIDEO_MODE_PANEL
#undef EVA_SWITCH_MODE_PANEL

static struct mipi_dsi_panel_platform_data *mipi_novatek_pdata;

static struct dsi_buf novatek_tx_buf;
static struct dsi_buf novatek_rx_buf;
static int mipi_novatek_lcd_init(void);

struct dsi_cmd_desc *novatek_display_on_cmds = NULL;
struct dsi_cmd_desc *novatek_display_off_cmds = NULL;
int novatek_display_on_cmds_size = 0;
int novatek_display_off_cmds_size = 0;
char ptype[60] = "Panel Type = ";

static int wled_trigger_initialized;

#define MIPI_DSI_NOVATEK_SPI_DEVICE_NAME	"dsi_novatek_3d_panel_spi"
#define HPCI_FPGA_READ_CMD	0x84
#define HPCI_FPGA_WRITE_CMD	0x04
#ifdef CONFIG_SPI_QUP
#undef CONFIG_SPI_QUP
#endif
#ifdef CONFIG_SPI_QUP
static struct spi_device *panel_3d_spi_client;

static void novatek_fpga_write(uint8 addr, uint16 value)
{
	char tx_buf[32];
	int  rc;
	struct spi_message  m;
	struct spi_transfer t;
	u8 data[4] = {0x0, 0x0, 0x0, 0x0};

	if (!panel_3d_spi_client) {
		pr_err("%s panel_3d_spi_client is NULL\n", __func__);
		return;
	}
	data[0] = HPCI_FPGA_WRITE_CMD;
	data[1] = addr;
	data[2] = ((value >> 8) & 0xFF);
	data[3] = (value & 0xFF);

	memset(&t, 0, sizeof t);
	memset(tx_buf, 0, sizeof tx_buf);
	t.tx_buf = data;
	t.len = 4;
	spi_setup(panel_3d_spi_client);
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	rc = spi_sync(panel_3d_spi_client, &m);
	if (rc)
		pr_err("%s: SPI transfer failed\n", __func__);

	return;
}

static void novatek_fpga_read(uint8 addr)
{
	char tx_buf[32];
	int  rc;
	struct spi_message  m;
	struct spi_transfer t;
	struct spi_transfer rx;
	char rx_value[2];
	u8 data[4] = {0x0, 0x0};

	if (!panel_3d_spi_client) {
		pr_err("%s panel_3d_spi_client is NULL\n", __func__);
		return;
	}

	data[0] = HPCI_FPGA_READ_CMD;
	data[1] = addr;

	memset(&t, 0, sizeof t);
	memset(tx_buf, 0, sizeof tx_buf);
	memset(&rx, 0, sizeof rx);
	memset(rx_value, 0, sizeof rx_value);
	t.tx_buf = data;
	t.len = 2;
	rx.rx_buf = rx_value;
	rx.len = 2;
	spi_setup(panel_3d_spi_client);
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	spi_message_add_tail(&rx, &m);

	rc = spi_sync(panel_3d_spi_client, &m);
	if (rc)
		pr_err("%s: SPI transfer failed\n", __func__);
	else
		pr_info("%s: rx_value = 0x%x, 0x%x\n", __func__,
						rx_value[0], rx_value[1]);

	return;
}

static int __devinit panel_3d_spi_probe(struct spi_device *spi)
{
	panel_3d_spi_client = spi;
	return 0;
}
static int __devexit panel_3d_spi_remove(struct spi_device *spi)
{
	panel_3d_spi_client = NULL;
	return 0;
}
static struct spi_driver panel_3d_spi_driver = {
	.probe         = panel_3d_spi_probe,
	.remove        = __devexit_p(panel_3d_spi_remove),
	.driver		   = {
		.name = "dsi_novatek_3d_panel_spi",
		.owner  = THIS_MODULE,
	}
};

#else

static void novatek_fpga_write(uint8 addr, uint16 value)
{
	return;
}

static void novatek_fpga_read(uint8 addr)
{
	return;
}

#endif



#ifdef NOVETAK_COMMANDS_UNUSED
static char display_config_cmd_mode1[] = {
	
	0x2A, 0x00, 0x00, 0x01,
	0x3F, 0xFF, 0xFF, 0xFF
};

static char display_config_cmd_mode2[] = {
	
	0x2B, 0x00, 0x00, 0x01,
	0xDF, 0xFF, 0xFF, 0xFF
};

static char display_config_cmd_mode3_666[] = {
	
	0x3A, 0x66, 0x15, 0x80 
};

static char display_config_cmd_mode3_565[] = {
	
	0x3A, 0x55, 0x15, 0x80 
};

static char display_config_321[] = {
	
	0x66, 0x2e, 0x15, 0x00 
};

static char display_config_323[] = {
	
	0x13, 0x00, 0x05, 0x00 
};

static char display_config_2lan[] = {
	
	0x61, 0x01, 0x02, 0xff 
};

static char display_config_exit_sleep[] = {
	
	0x11, 0x00, 0x05, 0x80 
};

static char display_config_TE_ON[] = {
	
	0x35, 0x00, 0x15, 0x80
};

static char display_config_39H[] = {
	
	0x39, 0x00, 0x05, 0x80
};

static char display_config_set_tear_scanline[] = {
	
	0x44, 0x00, 0x00, 0xff
};

static char display_config_set_twolane[] = {
	
	0xae, 0x03, 0x15, 0x80
};

static char display_config_set_threelane[] = {
	
	0xae, 0x05, 0x15, 0x80
};

#else
#if 0
static char sw_reset[2] = {0x01, 0x00}; 
static char enter_sleep[2] = {0x10, 0x00}; 
static char exit_sleep[2] = {0x11, 0x00}; 
static char display_off[2] = {0x28, 0x00}; 
static char display_on[2] = {0x29, 0x00}; 



static char rgb_888[2] = {0x3A, 0x77}; 

#if defined(NOVATEK_TWO_LANE)
static char set_num_of_lanes[2] = {0xae, 0x03}; 
#else  
static char set_num_of_lanes[2] = {0xae, 0x01}; 
#endif
static char novatek_f4[2] = {0xf4, 0x55}; 
static char novatek_8c[16] = { 
	0x8C, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x08, 0x08, 0x00, 0x30, 0xC0, 0xB7, 0x37};
static char novatek_ff[2] = {0xff, 0x55 }; 

static char set_width[5] = { 
	0x2A, 0x00, 0x00, 0x02, 0x1B}; 
static char set_height[5] = { 
	0x2B, 0x00, 0x00, 0x03, 0xBF}; 
#endif
#endif
static char led_pwm2[2] = {0x53, 0x24}; 

#if 0
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
	{DTYPE_DCS_WRITE, 1, 0, 0, 50,
		sizeof(sw_reset), sw_reset},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(display_on), display_on},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 50,
		sizeof(novatek_f4), novatek_f4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 50,
		sizeof(novatek_8c), novatek_8c},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 50,
		sizeof(novatek_ff), novatek_ff},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,
		sizeof(set_num_of_lanes), set_num_of_lanes},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 50,
		sizeof(set_width), set_width},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 50,
		sizeof(set_height), set_height},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,
		sizeof(rgb_888), rgb_888},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(led_pwm3), led_pwm3},
};

static struct dsi_cmd_desc novatek_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(enter_sleep), enter_sleep}
};
#endif
static char k2_f0_1[] = {
    0xF0, 0x55, 0xAA, 0x52,
    0x08, 0x01}; 
static char k2_b0_1[] = {
    0xB0, 0x12}; 
static char k2_b1_1[] = {
    0xB1, 0x12}; 
static char k2_b2[] = {
    0xB2, 0x00}; 
static char k2_b3[] = {
    0xB3, 0x07}; 
static char k2_b6_1[] = {
    0xB6, 0x14}; 
static char k2_b7_1[] = {
    0xB7, 0x15}; 
static char k2_b8_1[] = {
    0xB8, 0x24}; 
static char k2_b9[] = {
    0xB9, 0x24}; 
static char k2_ba[] = {
    0xBA, 0x14}; 
static char k2_bf[] = {
    0xBF, 0x01}; 
static char k2_c3[] = {
    0xC3, 0x06}; 
static char k2_c2[] = {
    0xC2, 0x00}; 
static char k2_c0[] = {
    0xC0, 0x00, 0x00}; 
static char k2_bc_1[] = {
    0xBC, 0x00, 0x80, 0x00}; 
static char k2_bd[] = {
    0xBD, 0x00, 0x80, 0x00}; 

static char k2_d1[] = {
    0xD1, 0x00, 0x58, 0x00, 0x64, 0x00, 0x76, 0x00, 0x88, 0x00,
    0x99, 0x00, 0xB8, 0x00, 0xCD, 0x00, 0xFA, 0x01, 0x1A, 0x01,
    0x4F, 0x01, 0x7D, 0x01, 0xC3, 0x01, 0xFF, 0x02, 0x01, 0x02,
    0x3A, 0x02, 0x77, 0x02, 0x9E, 0x02, 0XD8, 0X02, 0xF6, 0x03,
    0x23, 0x03, 0x53, 0x03, 0x79, 0x03, 0x91, 0x03, 0xB7, 0x03,
    0xC3, 0x03, 0XF9};
static char k2_d2[] = {
    0xD2, 0x00, 0xB3, 0x00, 0xBA, 0x00, 0xC7, 0x00, 0xD3, 0x00,
    0xDF, 0x00, 0xF5, 0x01, 0x01, 0x01, 0x24, 0x01, 0x3F, 0x01,
    0x6B, 0x01, 0x92, 0x01, 0xD2, 0x02, 0x0B, 0x02, 0x0D, 0x02,
    0x42, 0x02, 0x7E, 0x02, 0xA5, 0x02, 0xE0, 0x02, 0xFD, 0x03,
    0x2B, 0x03, 0x58, 0x03, 0x7A, 0x03, 0x90, 0x03, 0xA8, 0x03,
    0xC2, 0x03, 0xF8};
static char k2_d3[] = {
    0xD3, 0x00, 0x58, 0x00, 0x63, 0x00, 0x75, 0x00, 0x87, 0x00,
    0x98, 0x00, 0xB7, 0x00, 0xCB, 0x00, 0xF8, 0x01, 0x18, 0x01,
    0x4D, 0x01, 0x7A, 0x01, 0xC0, 0X01, 0xFD, 0x01, 0xFF, 0x02,
    0x38, 0x02, 0x75, 0x02, 0x9D, 0x02, 0xDB, 0x02, 0xFC, 0x03,
    0x30, 0x03, 0x63, 0x03, 0x8E, 0x03, 0xA6, 0x03, 0xCC, 0x03,
    0XD8, 0x03, 0xF9};
static char k2_d4[] = {
    0xD4, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x1E, 0x00, 0x30, 0x00,
    0x41, 0x00, 0x60, 0x00, 0x83, 0x00, 0xB0, 0x00, 0xDA, 0x01,
    0x21, 0x01, 0x5B, 0x01, 0xB7, 0x01, 0xFF, 0x02, 0x01, 0x02,
    0x46, 0x02, 0x89, 0x02, 0xB4, 0x02, 0xEE, 0x03, 0x12, 0x03,
    0x45, 0x03, 0x53, 0x03, 0x79, 0x03, 0x91, 0x03, 0xB7, 0x03,
    0xC3, 0x03, 0xF9};
static char k2_d5[] = {
    0xD5, 0x00, 0x5B, 0x00, 0x62, 0x00, 0x6F, 0x00, 0x78, 0x00,
    0x87, 0x00, 0x9D, 0x00, 0xB7, 0x00, 0xDA, 0x00, 0xFF, 0x01,
    0x3D, 0x01, 0x70, 0x01, 0xC6, 0x02, 0x0B, 0x02, 0x0D, 0x02,
    0x4E, 0x02, 0x90, 0x02, 0xBB, 0x02, 0xF6, 0x03, 0x19, 0x03,
    0x4D, 0x03, 0x58, 0x03, 0x7A, 0x03, 0x90, 0X03, 0xA9, 0x03,
    0xC2, 0x03, 0xF8};
static char k2_d6[] = {
    0xD6, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x1D, 0x00, 0x2F, 0x00,
    0x40, 0x00, 0x5F, 0x00, 0x81, 0x00, 0xAE, 0x00, 0xDB, 0x01,
    0x1F, 0x01, 0x58, 0x01, 0xB4, 0x01, 0xFD, 0x01, 0xFF, 0x02,
    0x44, 0x02, 0x87, 0x02, 0xB3, 0x02, 0xF1, 0x03, 0x18, 0x03,
    0x52, 0x03, 0x63, 0x03, 0x8E, 0x03, 0xA6, 0x03, 0xCC, 0x03,
    0xD8, 0x03, 0xF9};

static char k2_f0_2[] = {
    0xF0, 0x55, 0xAA, 0x52,
    0x08, 0x00}; 
static char k2_b6_2[] = {
    0xB6, 0x03}; 
static char k2_b7_2[] = {
    0xB7, 0x70, 0x70}; 
static char k2_b8_2[] = {
    0xB8, 0x01, 0x06, 0x06,
    0x06}; 
static char k2_bc_2[] = {
    0xBC, 0x00}; 
static char k2_b0_2[] = {
    0xB0, 0x00, 0x0A, 0x0E,
    0x09, 0x04}; 
static char k2_b1_2[] = {
    0xB1, 0x60, 0x00, 0x01}; 
static char k2_b4_2[] = {
    0xB4, 0x10}; 

static char k2_ff_1[] = {
    0xFF, 0xAA, 0x55, 0xA5,
    0x80}; 
static char k2_f7[] = {
    0xF7, 0x63, 0x40, 0x00,
    0x00, 0x00, 0x01, 0xC4,
    0xA2, 0x00, 0x02, 0x64,
    0x54, 0x48, 0x00, 0xD0}; 
static char k2_f8[] = {
    0xF8, 0x00, 0x00, 0x33,
    0x0F, 0x0F, 0x20, 0x00,
    0x01, 0x00, 0x00, 0x20,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00}; 
static char k2_ff_2[] = {
    0xFF, 0xAA, 0x55, 0xA5,
    0x00}; 

static char k2_b7_3[] = {
    0xB7, 0x02, 0x50}; 
static char k2_bd_3[] = {
    0xBD, 0x00, 0x00}; 
static char k2_bc_3[] = {
    0xBC, 0x00, 0x00}; 

static char k2_peripheral_on[] = {0x00, 0x00}; 
static char k2_peripheral_off[] = {0x00, 0x00}; 

static struct dsi_cmd_desc k2_auo_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_f0_1), k2_f0_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(k2_b0_1), k2_b0_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(k2_b1_1), k2_b1_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(k2_b2), k2_b2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(k2_b3), k2_b3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(k2_b6_1), k2_b6_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(k2_b7_1), k2_b7_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(k2_b8_1), k2_b8_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(k2_b9), k2_b9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(k2_ba), k2_ba},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(k2_bf), k2_bf},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(k2_c3), k2_c3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(k2_c2), k2_c2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_c0), k2_c0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_bc_1), k2_bc_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_bd), k2_bd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_d1), k2_d1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_d2), k2_d2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_d3), k2_d3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_d4), k2_d4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_d5), k2_d5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_d6), k2_d6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_f0_2), k2_f0_2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(k2_b6_2), k2_b6_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_b7_2), k2_b7_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_b8_2), k2_b8_2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(k2_bc_2), k2_bc_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_b0_2), k2_b0_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_b1_2), k2_b1_2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(k2_b4_2), k2_b4_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_ff_1), k2_ff_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_f7), k2_f7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_f8), k2_f8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_ff_2), k2_ff_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_b7_3), k2_b7_3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_bd_3), k2_bd_3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(k2_bc_3), k2_bc_3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(led_pwm2), led_pwm2},
	{DTYPE_PERIPHERAL_ON, 1, 0, 1, 120, sizeof(k2_peripheral_on), k2_peripheral_on},
};

static struct dsi_cmd_desc k2_auo_display_off_cmds[] = {
	{DTYPE_PERIPHERAL_OFF, 1, 0, 1, 70, sizeof(k2_peripheral_off), k2_peripheral_off},
};

static char set_threelane[2] = {0xBA, 0x02}; 

#ifdef EVA_CMD_MODE_PANEL
static char display_mode_cmd[2] = {0xC2, 0x08}; 
#else
static char display_mode_video[2] = {0xC2, 0x03}; 
#endif

static char enter_sleep[2] = {0x10, 0x00}; 
static char exit_sleep[2] = {0x11, 0x00}; 
static char display_on[2] = {0x29, 0x00}; 
static char display_off[2] = {0x28, 0x00}; 

static char enable_te[2] = {0x35, 0x00};

static char swr01[2] = {0x01, 0x33};
static char swr02[2] = {0x02, 0x53};

static struct dsi_cmd_desc sony_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 40, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sony_panel_video_mode_cmds_id28103_ver008[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef EVA_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif

#if 1
	
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
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	
#endif

#if 1
#ifdef EVA_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x05} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
#endif
#endif

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep), exit_sleep},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x33} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x04} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x30} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0x34} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x00} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	
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

	
	

	
	

	
	

	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x83}},


	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24} },
};

static struct dsi_cmd_desc sony_panel_video_mode_cmds_c2[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef EVA_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif

#if 1
	
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
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	
#endif

#if 1
#ifdef EVA_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x05}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x8E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x8E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x8E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
#endif
#endif

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep), exit_sleep},

	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	
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

	
	

	
	

	
	

	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x83}},


	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24}},
};

static struct dsi_cmd_desc sony_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 100,
		sizeof(enter_sleep), enter_sleep}
};


static char manufacture_id[2] = {0x04, 0x00}; 

static struct dsi_cmd_desc novatek_manufacture_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id), manufacture_id};

static struct dcs_cmd_req cmdreq;
static u32 manu_id;

static void mipi_novatek_manufature_cb(u32 data)
{
	manu_id = data;
	pr_info("%s: manufature_id=%x\n", __func__, manu_id);
}

static uint32 mipi_novatek_manufacture_id(struct msm_fb_data_type *mfd)
{
	cmdreq.cmds = &novatek_manufacture_id_cmd;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_RX | CMD_REQ_COMMIT;
	cmdreq.rlen = 3;
	cmdreq.cb = mipi_novatek_manufature_cb;
	mipi_dsi_cmdlist_put(&cmdreq);

	return manu_id;
}

static int fpga_addr;
static int fpga_access_mode;
static bool support_3d;

static void mipi_novatek_3d_init(int addr, int mode)
{
	fpga_addr = addr;
	fpga_access_mode = mode;
}

static void mipi_dsi_enable_3d_barrier(int mode)
{
	void __iomem *fpga_ptr;
	uint32_t ptr_value = 0;

	if (!fpga_addr && support_3d) {
		pr_err("%s: fpga_addr not set. Failed to enable 3D barrier\n",
					__func__);
		return;
	}

	if (fpga_access_mode == FPGA_SPI_INTF) {
		if (mode == LANDSCAPE)
			novatek_fpga_write(fpga_addr, 1);
		else if (mode == PORTRAIT)
			novatek_fpga_write(fpga_addr, 3);
		else
			novatek_fpga_write(fpga_addr, 0);

		mb();
		novatek_fpga_read(fpga_addr);
	} else if (fpga_access_mode == FPGA_EBI2_INTF) {
		fpga_ptr = ioremap_nocache(fpga_addr, sizeof(uint32_t));
		if (!fpga_ptr) {
			pr_err("%s: FPGA ioremap failed."
				"Failed to enable 3D barrier\n",
						__func__);
			return;
		}

		ptr_value = readl_relaxed(fpga_ptr);
		if (mode == LANDSCAPE)
			writel_relaxed(((0xFFFF0000 & ptr_value) | 1),
								fpga_ptr);
		else if (mode == PORTRAIT)
			writel_relaxed(((0xFFFF0000 & ptr_value) | 3),
								fpga_ptr);
		else
			writel_relaxed((0xFFFF0000 & ptr_value),
								fpga_ptr);

		mb();
		iounmap(fpga_ptr);
	} else
		pr_err("%s: 3D barrier not configured correctly\n",
					__func__);
}

static void mipi_novatek_panel_type_detect(struct mipi_panel_info *mipi)
{
	if (panel_type == PANEL_ID_K2_WL_AUO) {
		PR_DISP_INFO("%s: panel_type=PANEL_ID_K2_WL_AUO\n", __func__);
		strcat(ptype, "PANEL_ID_K2_WL_AUO");
		if (mipi->mode == DSI_VIDEO_MODE) {
			novatek_display_on_cmds = k2_auo_display_on_cmds;
			novatek_display_on_cmds_size = ARRAY_SIZE(k2_auo_display_on_cmds);
			novatek_display_off_cmds = k2_auo_display_off_cmds;
			novatek_display_off_cmds_size = ARRAY_SIZE(k2_auo_display_off_cmds);
		}
	} else if (panel_type == PANEL_ID_ELITE_SONY_NT_C2) {
		strcat(ptype, "PANEL_ID_ELITE_SONY_NT_C2");
		PR_DISP_INFO("%s: assign initial setting for SONY_NT Cut2, %s\n", __func__, ptype);
		novatek_display_on_cmds = sony_panel_video_mode_cmds_c2;
		novatek_display_on_cmds_size = ARRAY_SIZE(sony_panel_video_mode_cmds_c2);
		novatek_display_off_cmds = sony_display_off_cmds;
		novatek_display_off_cmds_size = ARRAY_SIZE(sony_display_off_cmds);
	} else if (panel_type == PANEL_ID_ELITE_SONY_NT_C1) {
		strcat(ptype, "PANEL_ID_ELITE_SONY_NT_C1");
		PR_DISP_INFO("%s: assign initial setting for SONY_NT id 0x28103 Cut1, %s\n", __func__, ptype);
		novatek_display_on_cmds = sony_panel_video_mode_cmds_id28103_ver008;
		novatek_display_on_cmds_size = ARRAY_SIZE(sony_panel_video_mode_cmds_id28103_ver008);
		novatek_display_off_cmds = sony_display_off_cmds;
		novatek_display_off_cmds_size = ARRAY_SIZE(sony_display_off_cmds);
	}
}

static int mipi_novatek_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	struct msm_panel_info *pinfo;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	pinfo = &mfd->panel_info;
	if (pinfo->is_3d_panel)
		support_3d = TRUE;

	mipi  = &mfd->panel_info.mipi;

	if (mfd->init_mipi_lcd == 0) {
		PR_DISP_DEBUG("Display On - 1st time\n");

		mipi_novatek_panel_type_detect(mipi);

		mfd->init_mipi_lcd = 1;
		return 0;
	}

	PR_DISP_INFO("Display On. (%s)\n", ptype);

	if (mipi->mode == DSI_VIDEO_MODE) {
	} else {
		cmdreq.cmds = novatek_display_on_cmds;
		cmdreq.cmds_cnt = novatek_display_on_cmds_size;
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);

		mipi_dsi_cmd_bta_sw_trigger(); 

		mipi_novatek_manufacture_id(mfd);
	}

	PR_DISP_DEBUG("Init done!\n");

	return 0;
}

static int mipi_novatek_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	cmdreq.cmds = novatek_display_off_cmds;
	cmdreq.cmds_cnt = novatek_display_off_cmds_size;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

	return 0;
}

static void mipi_novatek_display_on(struct msm_fb_data_type *mfd)
{
	mutex_lock(&mfd->dma->ov_mutex);

	if (panel_type == PANEL_ID_ELITE_SONY_NT_C1
			|| panel_type == PANEL_ID_ELITE_SONY_NT_C2)
		mipi_dsi_cmds_tx(&novatek_tx_buf, sony_display_on_cmds,
			ARRAY_SIZE(sony_display_on_cmds));

	mutex_unlock(&mfd->dma->ov_mutex);
}

DEFINE_LED_TRIGGER(bkl_led_trigger);

static char led_pwm1[2] = {0x51, 0xF0};	
static struct dsi_cmd_desc backlight_cmd[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(led_pwm1), led_pwm1},
};

static void mipi_novatek_set_backlight(struct msm_fb_data_type *mfd)
{
	if ((mipi_novatek_pdata && mipi_novatek_pdata->enable_wled_bl_ctrl)
	    && (wled_trigger_initialized)) {
		led_trigger_event(bkl_led_trigger, mfd->bl_level);
		return;
	}

	
	if (mipi_novatek_pdata && mipi_novatek_pdata->shrink_pwm)
		led_pwm1[1] = mipi_novatek_pdata->shrink_pwm(mfd->bl_level);
	else
		led_pwm1[1] = (unsigned char)(mfd->bl_level);

	cmdreq.cmds = (struct dsi_cmd_desc*)&backlight_cmd;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);
}

static int mipi_dsi_3d_barrier_sysfs_register(struct device *dev);
static int barrier_mode;

static int __devinit mipi_novatek_lcd_probe(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	struct platform_device *current_pdev;
	static struct mipi_dsi_phy_ctrl *phy_settings;
	static char dlane_swap;

	if (pdev->id == 0) {
		mipi_novatek_pdata = pdev->dev.platform_data;

		if (mipi_novatek_pdata
			&& mipi_novatek_pdata->phy_ctrl_settings) {
			phy_settings = (mipi_novatek_pdata->phy_ctrl_settings);
		}

		if (mipi_novatek_pdata
			&& mipi_novatek_pdata->dlane_swap) {
			dlane_swap = (mipi_novatek_pdata->dlane_swap);
		}

		if (mipi_novatek_pdata
			 && mipi_novatek_pdata->fpga_3d_config_addr)
			mipi_novatek_3d_init(mipi_novatek_pdata
	->fpga_3d_config_addr, mipi_novatek_pdata->fpga_ctrl_mode);

		
		if (mipi_dsi_3d_barrier_sysfs_register(&pdev->dev)) {
			pr_err("%s: Failed to register 3d Barrier sysfs\n",
						__func__);
			return -ENODEV;
		}
		barrier_mode = 0;

		return 0;
	}

	current_pdev = msm_fb_add_device(pdev);

	if (current_pdev) {
		mfd = platform_get_drvdata(current_pdev);
		if (!mfd)
			return -ENODEV;
		if (mfd->key != MFD_KEY)
			return -EINVAL;

		mipi  = &mfd->panel_info.mipi;

		if (phy_settings != NULL)
			mipi->dsi_phy_db = phy_settings;

		if (dlane_swap)
			mipi->dlane_swap = dlane_swap;
	}
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_novatek_lcd_probe,
	.driver = {
		.name   = "mipi_novatek",
	},
};

static struct msm_fb_panel_data novatek_panel_data = {
	.on		= mipi_novatek_lcd_on,
	.off		= mipi_novatek_lcd_off,
	.display_on = mipi_novatek_display_on,
	.set_backlight = mipi_novatek_set_backlight,
};

static ssize_t mipi_dsi_3d_barrier_read(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return snprintf((char *)buf, sizeof(buf), "%u\n", barrier_mode);
}

static ssize_t mipi_dsi_3d_barrier_write(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	int ret = -1;
	u32 data = 0;

	if (sscanf((char *)buf, "%u", &data) != 1) {
		dev_err(dev, "%s\n", __func__);
		ret = -EINVAL;
	} else {
		barrier_mode = data;
		if (data == 1)
			mipi_dsi_enable_3d_barrier(LANDSCAPE);
		else if (data == 2)
			mipi_dsi_enable_3d_barrier(PORTRAIT);
		else
			mipi_dsi_enable_3d_barrier(0);
	}

	return count;
}

static struct device_attribute mipi_dsi_3d_barrier_attributes[] = {
	__ATTR(enable_3d_barrier, 0664, mipi_dsi_3d_barrier_read,
					 mipi_dsi_3d_barrier_write),
};

static int mipi_dsi_3d_barrier_sysfs_register(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mipi_dsi_3d_barrier_attributes); i++)
		if (device_create_file(dev, mipi_dsi_3d_barrier_attributes + i))
			goto error;

	return 0;

error:
	for (; i >= 0 ; i--)
		device_remove_file(dev, mipi_dsi_3d_barrier_attributes + i);
	pr_err("%s: Unable to create interface\n", __func__);

	return -ENODEV;
}

static int ch_used[3];

int mipi_novatek_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_novatek_lcd_init();
	if (ret) {
		pr_err("mipi_novatek_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_novatek", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	novatek_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &novatek_panel_data,
		sizeof(novatek_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int mipi_novatek_lcd_init(void)
{
#ifdef CONFIG_SPI_QUP
	int ret;
	ret = spi_register_driver(&panel_3d_spi_driver);

	if (ret) {
		pr_err("%s: spi register failed: rc=%d\n", __func__, ret);
		platform_driver_unregister(&this_driver);
	} else
		pr_info("%s: SUCCESS (SPI)\n", __func__);
#endif

	led_trigger_register_simple("bkl_trigger", &bkl_led_trigger);
	pr_info("%s: SUCCESS (WLED TRIGGER)\n", __func__);
	wled_trigger_initialized = 1;

	mipi_dsi_buf_alloc(&novatek_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&novatek_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}
