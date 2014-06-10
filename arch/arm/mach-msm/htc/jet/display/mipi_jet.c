#include <linux/gpio.h>
#include <mach/gpio.h>
#include <mach/panel_id.h>
#include "../../../drivers/video/msm/msm_fb.h"
#include "../../../drivers/video/msm/mipi_dsi.h"
#include "../board-jet.h"
#include "mipi_jet.h"

static struct msm_panel_common_pdata *mipi_jet_pdata;
static int mipi_jet_lcd_init(void);
// Selected codes
static struct dsi_cmd_desc *jet_video_on_cmds = NULL;
int jet_video_on_cmds_count = 0;
static struct dsi_cmd_desc *jet_command_on_cmds = NULL;
int jet_command_on_cmds_count = 0;
static struct dsi_cmd_desc *jet_display_off_cmds = NULL;
int jet_display_off_cmds_count = 0;
static struct dsi_cmd_desc *jet_cmd_backlight_cmds = NULL;
int jet_cmd_backlight_cmds_count = 0;
#if defined CONFIG_FB_MSM_SELF_REFRESH
static struct dsi_cmd_desc *jet_video_to_cmd = NULL;
int jet_video_to_cmd_cnt = 0;
static struct dsi_cmd_desc *jet_cmd_to_video = NULL;
int jet_cmd_to_video_cnt = 0;
#endif

static char led_pwm1[2] = {0x51, 0xFF};	/* DTYPE_DCS_WRITE1 */
static char led_pwm2[2] = {0x53, 0x24}; /* DTYPE_DCS_WRITE1 */
static char led_pwm3[2] = {0x55, 0x00}; /* DTYPE_DCS_WRITE1 */
static char display_off[2] = {0x28, 0x00}; /* DTYPE_DCS_WRITE */
static char novatek_e0[3] = {0xE0, 0x01, 0x03}; /* DTYPE_DCS_LWRITE */
static char sw_reset[2] = {0x01, 0x00}; /* DTYPE_DCS_WRITE */
static char enter_sleep[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */
static char exit_sleep[2] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */
static char display_on[2] = {0x29, 0x00}; /* DTYPE_DCS_WRITE */
static char enable_te[2] = {0x35, 0x00};/* DTYPE_DCS_WRITE1 */
static char max_pktsize[2] = {MIPI_DSI_MRPS, 0x00}; /* LSB tx first, 16 bytes */
static char disable_dim_cmd[2] = {0x53, 0x24};/* DTYPE_DCS_WRITE1 */

static struct dsi_cmd_desc sony_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc novatek_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 12,
		sizeof(enter_sleep), enter_sleep}
};

static char fir_lg_gamma_01_d1[] = {
	0xD1, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01,
	0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01,
	0x8D
};

static char fir_lg_gamma_01_d5[] = {
	0xD5, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01,
	0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01,
	0x8D
};

static char fir_lg_gamma_01_d9[] = {
	0xD9, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01,
	0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01,
	0x8D
};

static char fir_lg_gamma_01_e0[] = {
	0xE0, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01,
	0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01,
	0x8D
};

static char fir_lg_gamma_01_e4[] = {
	0xE4, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01,
	0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01,
	0x8D
};

static char fir_lg_gamma_01_e8[] = {
	0xE8, 0x00, 0x70, 0x00, 0xCE, 0x00, 0xF7, 0x01,
	0x10, 0x01, 0x21, 0x01, 0x44, 0x01, 0x62, 0x01,
	0x8D
};

static char fir_lg_gamma_02_d2[] = {
	0xD2, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02,
	0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02,
	0xF0
};

static char fir_lg_gamma_02_d6[] = {
	0xD6, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02,
	0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02,
	0xF0
};

static char fir_lg_gamma_02_dd[] = {
	0xDD, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02,
	0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02,
	0xF0
};

static char fir_lg_gamma_02_e1[] = {
	0xE1, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02,
	0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02,
	0xF0
};

static char fir_lg_gamma_02_e5[] = {
	0xE5, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02,
	0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02,
	0xF0
};

static char fir_lg_gamma_02_e9[] = {
	0xE9, 0x01, 0xAF, 0x01, 0xE4, 0x02, 0x0C, 0x02,
	0x4D, 0x02, 0x82, 0x02, 0x84, 0x02, 0xB8, 0x02,
	0xF0
};

static char fir_lg_gamma_03_d3[] = {
	0xD3, 0x03, 0x14, 0x03, 0x42, 0x03, 0x5E, 0x03,
	0x80, 0x03, 0x97, 0x03, 0xB0, 0x03, 0xC0, 0x03,
	0xDF
};

static char fir_lg_gamma_03_d7[] = {
	0xD7, 0x03, 0x14, 0x03, 0x42, 0x03, 0x5E, 0x03,
	0x80, 0x03, 0x97, 0x03, 0xB0, 0x03, 0xC0, 0x03,
	0xDF
};

static char fir_lg_gamma_03_de[] = {
	0xDE, 0x03, 0x14, 0x03, 0x42, 0x03, 0x5E, 0x03,
	0x80, 0x03, 0x97, 0x03, 0xB0, 0x03, 0xC0, 0x03,
	0xDF
};

static char fir_lg_gamma_03_e2[] = {
	0xE2, 0x03, 0x14, 0x03, 0x42, 0x03, 0x5E, 0x03,
	0x80, 0x03, 0x97, 0x03, 0xB0, 0x03, 0xC0, 0x03,
	0xDF
};

static char fir_lg_gamma_03_e6[] = {
	0xE6, 0x03, 0x14, 0x03, 0x42, 0x03, 0x5E, 0x03,
	0x80, 0x03, 0x97, 0x03, 0xB0, 0x03, 0xC0, 0x03,
	0xDF
};

static char fir_lg_gamma_03_ea[] = {
	0xEA, 0x03, 0x14, 0x03, 0x42, 0x03, 0x5E, 0x03,
	0x80, 0x03, 0x97, 0x03, 0xB0, 0x03, 0xC0, 0x03,
	0xDF
};

static char fir_lg_gamma_04_d4[] = {
	0xD4, 0x03, 0xFD, 0x03, 0xFF
};

static char fir_lg_gamma_04_d8[] = {
	0xD8, 0x03, 0xFD, 0x03, 0xFF
};

static char fir_lg_gamma_04_df[] = {
	0xDF, 0x03, 0xFD, 0x03, 0xFF
};

static char fir_lg_gamma_04_e3[] = {
	0xE3, 0x03, 0xFD, 0x03, 0xFF
};

static char fir_lg_gamma_04_e7[] = {
	0xE7, 0x03, 0xFD, 0x03, 0xFF
};

static char fir_lg_gamma_04_eb[] = {
	0xEB, 0x03, 0xFD, 0x03, 0xFF
};

static char set_threelane[2] = {0xBA, 0x02}; /* DTYPE_DCS_WRITE-1 */

static char swr01[2] = {0x01, 0x33};
static char swr02[2] = {0x02, 0x53};

static char cmd_page_0[2] = {0xff, 0x00};

#ifdef JEL_CMD_MODE_PANEL
static char display_mode_cmd[2] = {0xC2, 0x08}; /* DTYPE_DCS_WRITE-1 */
#else
static char display_mode_video[2] = {0xC2, 0x03}; /* DTYPE_DCS_WRITE-1 */
#endif

static struct dsi_cmd_desc disable_dim[] = {{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(disable_dim_cmd), disable_dim_cmd},};
#ifdef CONFIG_FB_MSM_CABC
static struct dsi_cmd_desc *dim_cmds = disable_dim; /* default disable dim */
#endif

static struct dsi_cmd_desc nvt_LowTemp_wrkr_enter[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, 2, (char[]){0x26, 0x08}},
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}}, */
};

static struct dsi_cmd_desc nvt_LowTemp_wrkr_exit[] = {
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE}}, */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10, 2, (char[]){0xFF, 0x00}},
};

static struct dsi_cmd_desc cmd_bkl_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(led_pwm1), led_pwm1},
};

#ifdef CONFIG_FB_MSM_CABC
/* for cabc enable and disable seletion */
static char cabc_off_cmd[2] = {0x55, 0x80};/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc cabc_off[] = {{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cabc_off_cmd), cabc_off_cmd},};
static char cabc_on_ui_cmd[2] = {0x55, 0x81};/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc cabc_on_ui[] = {{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cabc_on_ui_cmd), cabc_on_ui_cmd},};
static char cabc_on_still_cmd[2] = {0x55, 0x82};/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc cabc_on_still[] = {{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cabc_on_still_cmd), cabc_on_still_cmd},};
static char cabc_on_moving_cmd[2] = {0x55, 0x83};/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc cabc_on_moving[] = {{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cabc_on_moving_cmd), cabc_on_moving_cmd},};
/* for dimming enable and disable selection */
static char enable_dim_cmd[2] = {0x53, 0x2C};/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc enable_dim[] = {{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(enable_dim_cmd), enable_dim_cmd},};
static struct dsi_cmd_desc *cabc_cmds = cabc_off; /* default disable cabc */
#endif

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
		sizeof(fir_lg_gamma_01_d1), fir_lg_gamma_01_d1 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_01_d5), fir_lg_gamma_01_d5 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_01_d9), fir_lg_gamma_01_d9 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_01_e0), fir_lg_gamma_01_e0 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_01_e4), fir_lg_gamma_01_e4 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_01_e8), fir_lg_gamma_01_e8 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_02_d2), fir_lg_gamma_02_d2 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_02_d6), fir_lg_gamma_02_d6 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_02_dd), fir_lg_gamma_02_dd },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_02_e1), fir_lg_gamma_02_e1 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_02_e5), fir_lg_gamma_02_e5 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_02_e9), fir_lg_gamma_02_e9 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_03_d3), fir_lg_gamma_03_d3 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_03_d7), fir_lg_gamma_03_d7 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_03_de), fir_lg_gamma_03_de },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_03_e2), fir_lg_gamma_03_e2 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_03_e6), fir_lg_gamma_03_e6 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_03_ea), fir_lg_gamma_03_ea },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_04_d4), fir_lg_gamma_04_d4 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_04_d8), fir_lg_gamma_04_d8 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_04_df), fir_lg_gamma_04_df },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_04_e3), fir_lg_gamma_04_e3 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_04_e7), fir_lg_gamma_04_e7 },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(fir_lg_gamma_04_eb), fir_lg_gamma_04_eb },
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		6, (char[]){0xF0, 0x55, 0xAA, 0x52, 0x00, 0x00} },/* select page 0 */
	{DTYPE_DCS_WRITE, 1, 0, 0, 12,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(enable_te), enable_te},
	{DTYPE_MAX_PKTSIZE, 1, 0, 0, 0,
		sizeof(max_pktsize), max_pktsize},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm1), led_pwm1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm3), led_pwm3},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sony_c1_video_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef JEL_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif

#if 1
	/* vivi color ver 2 */
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
	/* gamma 2.2 6b setting start */
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
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x08}},//PWMDIV=8
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* gamma 2.2 6b setting end */
#endif

#if 1
	/* set Frame rate=60hz */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x05} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
#endif

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(exit_sleep), exit_sleep},

	/* For sony cut1 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x33} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x04} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x30} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0x34} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x00} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	/* For random dot noise */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	/* Enable CABC setting */
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

	/* NVT: Enable vivid-color, but disable CABC, please set register(55h) as 0x80  */
	/*{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x80}},*/

	/* NVT: Enable vivid-color, and enable CABC UI-Mode, please set register(55h) as 0x81 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x81}}, */

	/* NVT: Enable vivid-color, and enable CABC Still-Mode, please set register(55h) as 0x82 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55, 0x82} }, */

	/* NVT: Enable vivid-color, and enable CABC Moving-Mode, please set register(55h) as 0x83 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x83}},


	/* {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep}, */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24} },

	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sony_panel_video_mode_cmds_c2[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef JEL_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif

#if 1
	/* vivi color ver 2 */
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
	/* gamma 2.2 6b setting start */
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
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x08}},//PWMDIV=8
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* gamma 2.2 6b setting end */
#endif

	/* set Frame rate=60hz */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x05} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x8E} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(exit_sleep), exit_sleep},

	/* For random dot noise */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	/* Enable CABC setting */
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

	/* NVT: Enable vivid-color, but disable CABC, please set register(55h) as 0x80  */
	/*{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x80}},*/

	/* NVT: Enable vivid-color, and enable CABC UI-Mode, please set register(55h) as 0x81 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x81}}, */

	/* NVT: Enable vivid-color, and enable CABC Still-Mode, please set register(55h) as 0x82 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x82}}, */

	/* NVT: Enable vivid-color, and enable CABC Moving-Mode, please set register(55h) as 0x83 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x83}},


	/* {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep}, */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24}},

	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

/* AUO timing fix */
static char auo_fix_000[] = {0xFF, 0x01};
static char auo_fix_001[] = {0xFB, 0x01};
static char auo_fix_002[] = {0x00, 0x4A};
static char auo_fix_003[] = {0x01, 0x44};
static char auo_fix_004[] = {0x02, 0x54};
static char auo_fix_005[] = {0x03, 0x55};
static char auo_fix_006[] = {0x04, 0x55};
static char auo_fix_007[] = {0x05, 0x33};
static char auo_fix_008[] = {0x06, 0x22};
static char auo_fix_009[] = {0x08, 0x56};
static char auo_fix_010[] = {0x09, 0x8F};
static char auo_fix_011[] = {0x0B, 0xAF};
static char auo_fix_012[] = {0x0C, 0xAF};
static char auo_fix_013[] = {0x0D, 0x2F};
static char auo_fix_014[] = {0x0E, 0x1F};
static char auo_fix_015[] = {0x11, 0x84};
static char auo_fix_016[] = {0x12, 0x03};

static char auo_fix_017[] = {0x71, 0x2C};
static char auo_fix_018[] = {0x6F, 0x03};

static char auo_fix_019[] = {0x0F, 0x0A};
static char auo_fix_020[] = {0xFF, 0x05};
static char auo_fix_021[] = {0xFB, 0x01};
static char auo_fix_022[] = {0x01, 0x00};
static char auo_fix_023[] = {0x02, 0x8D};
static char auo_fix_024[] = {0x03, 0x8D};
static char auo_fix_025[] = {0x04, 0x8D};
static char auo_fix_026[] = {0x05, 0x30};
static char auo_fix_027[] = {0x06, 0x33};
static char auo_fix_028[] = {0x07, 0x01};
static char auo_fix_029[] = {0x08, 0x00};
static char auo_fix_030[] = {0x09, 0x30};
static char auo_fix_031[] = {0x0A, 0x30};
static char auo_fix_032[] = {0x0D, 0x11};
static char auo_fix_033[] = {0x0E, 0x1A};
static char auo_fix_034[] = {0x0F, 0x08};
static char auo_fix_035[] = {0x10, 0x53};
static char auo_fix_036[] = {0x11, 0x00};
static char auo_fix_037[] = {0x12, 0x00};
static char auo_fix_038[] = {0x14, 0x01};
static char auo_fix_039[] = {0x15, 0x00};
static char auo_fix_040[] = {0x16, 0x05};
static char auo_fix_041[] = {0x17, 0x08};
static char auo_fix_042[] = {0x19, 0x7F};
static char auo_fix_043[] = {0x1A, 0xFF};
static char auo_fix_044[] = {0x1B, 0x0F};
static char auo_fix_045[] = {0x1C, 0x00};
static char auo_fix_046[] = {0x1D, 0x00};
static char auo_fix_047[] = {0x1E, 0x00};
static char auo_fix_048[] = {0x1F, 0x07};
static char auo_fix_049[] = {0x20, 0x00};
static char auo_fix_050[] = {0x21, 0x00};
static char auo_fix_051[] = {0x22, 0x55};
static char auo_fix_052[] = {0x23, 0x0D};
static char auo_fix_053[] = {0x2D, 0x02};
static char auo_fix_054[] = {0x83, 0x01};
static char auo_fix_055[] = {0x9E, 0x58};
static char auo_fix_056[] = {0x9F, 0x6A};
static char auo_fix_057[] = {0xA0, 0x01};
static char auo_fix_058[] = {0xA2, 0x10};
static char auo_fix_059[] = {0xBB, 0x0A};
static char auo_fix_060[] = {0xBC, 0x0A};

static char auo_fix_061[] = {0x23, 0x4D};
static char auo_fix_062[] = {0x6C, 0x00};
static char auo_fix_063[] = {0x6D, 0x00};

static char auo_fix_064[] = {0x28, 0x00};
static char auo_fix_065[] = {0x2F, 0x00};
static char auo_fix_066[] = {0x32, 0x08};
static char auo_fix_067[] = {0x33, 0xB8};
static char auo_fix_068[] = {0x36, 0x01};
static char auo_fix_069[] = {0x37, 0x00};
static char auo_fix_070[] = {0x43, 0x00};
static char auo_fix_071[] = {0x4B, 0x21};
static char auo_fix_072[] = {0x4C, 0x03};
static char auo_fix_073[] = {0x50, 0x21};
static char auo_fix_074[] = {0x51, 0x03};
static char auo_fix_075[] = {0x58, 0x21};
static char auo_fix_076[] = {0x59, 0x03};
static char auo_fix_077[] = {0x5D, 0x21};
static char auo_fix_078[] = {0x5E, 0x03};

static char sw_wr00[] = {0xFF, 0xEE};
static char sw_wr01[] = {0xFB, 0x01};
static char sw_wr02[] = {0x12, 0x33};
static char sw_wr03[] = {0x13, 0x04};

static struct dsi_cmd_desc auo_panel_video_mode_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_000), auo_fix_000},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_001),	auo_fix_001},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_002),	auo_fix_002},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_003),	auo_fix_003},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_004),	auo_fix_004},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_005),	auo_fix_005},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_006),	auo_fix_006},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_007),	auo_fix_007},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_008),	auo_fix_008},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_009),	auo_fix_009},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_010),	auo_fix_010},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_011),	auo_fix_011},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_012),	auo_fix_012},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_013),	auo_fix_013},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_014),	auo_fix_014},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_015),	auo_fix_015},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_016),	auo_fix_016},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_017),	auo_fix_017},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_018),	auo_fix_018},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_019),	auo_fix_019},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_020),	auo_fix_020},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_021),	auo_fix_021},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_022),	auo_fix_022},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_023),	auo_fix_023},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_024),	auo_fix_024},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_025),	auo_fix_025},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_026),	auo_fix_026},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_027),	auo_fix_027},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_028),	auo_fix_028},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_029),	auo_fix_029},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_030),	auo_fix_030},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_031),	auo_fix_031},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_032),	auo_fix_032},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_033),	auo_fix_033},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_034),	auo_fix_034},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_035),	auo_fix_035},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_036),	auo_fix_036},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_037),	auo_fix_037},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_038),	auo_fix_038},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_039),	auo_fix_039},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_040),	auo_fix_040},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_041),	auo_fix_041},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_042),	auo_fix_042},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_043),	auo_fix_043},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_044),	auo_fix_044},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_045),	auo_fix_045},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_046),	auo_fix_046},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_047),	auo_fix_047},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_048),	auo_fix_048},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_049),	auo_fix_049},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_050),	auo_fix_050},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_051),	auo_fix_051},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_052),	auo_fix_052},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_053),	auo_fix_053},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_054),	auo_fix_054},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_055),	auo_fix_055},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_056),	auo_fix_056},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_057),	auo_fix_057},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_058),	auo_fix_058},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_059),	auo_fix_059},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_060),	auo_fix_060},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_061),	auo_fix_061},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_062),	auo_fix_062},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_063),	auo_fix_063},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_064),	auo_fix_064},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_065),	auo_fix_065},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_066),	auo_fix_066},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_067),	auo_fix_067},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_068),	auo_fix_068},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_069),	auo_fix_069},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_070),	auo_fix_070},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_071),	auo_fix_071},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_072),	auo_fix_072},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_073),	auo_fix_073},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_074),	auo_fix_074},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_075),	auo_fix_075},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_076),	auo_fix_076},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_077),	auo_fix_077},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_fix_078),	auo_fix_078},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd_page_0), cmd_page_0},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef JEL_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif

#if 1
	/* vivi color ver 2 */

	// enable smart color
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x66}},

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

	// add smart color
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x06}},  // add redden
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x00}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
#endif

#if 1
	/* New gamma setting start */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr01), swr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr02), swr02},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0x4B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0x8C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0x92}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xA5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xB7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0xC6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0xD4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0x29}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0xA2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0xE9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x25}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x27}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0x5D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0x93}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0xB6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0xE6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0x08}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x37}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x5A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0x6A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0x75}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0x8D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xA2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0xB9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0x4B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0x8C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0x92}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xA5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xB7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0xC6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0xD4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0x29}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0xA2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0xE9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x25}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x27}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0x5D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0x93}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0xB6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0xE6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0x08}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x37}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x5A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0x6A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0x75}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0x8D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xA2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEC, 0xB9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xED, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEE, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF0, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF2, 0xD4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF4, 0xE0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF6, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF8, 0xFA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFA, 0x05}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x00, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x01, 0x10}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x1A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x24}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x06, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x07, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x64}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0B, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0xBD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0xFD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x10, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x11, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x14, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x15, 0x61}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x16, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x17, 0x94}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x18, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x19, 0xB5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1A, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0xE2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x2C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x38}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x24, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x56}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x27, 0x66}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x29, 0x7C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2A, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2B, 0x8D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2F, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x30, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x31, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x32, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x33, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x34, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x35, 0xD4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x36, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x37, 0xE0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x38, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3A, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3B, 0xFA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3D, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3F, 0x05}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x40, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x41, 0x10}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x42, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x43, 0x1A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x44, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x45, 0x24}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x47, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x48, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x49, 0x64}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4B, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4D, 0xBD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4F, 0xFD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x50, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x51, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x52, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x54, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55, 0x61}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x56, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x58, 0x94}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x59, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5A, 0xB5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5C, 0xE2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5E, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x60, 0x2C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x61, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x62, 0x38}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x63, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x64, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x65, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x66, 0x56}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x67, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x68, 0x66}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x69, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x7C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6C, 0x8D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6E, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x70, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x71, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x72, 0x9B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x73, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x74, 0xA1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0xB3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0xC1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0xCF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0xDB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0xE7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xF1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0x3F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x81}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0xF0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0x2B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x63}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0xBB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0xE8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0x0B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0x40}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x4C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x53}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x5C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0x5A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0x5F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0x9B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0xA1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0xB3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0xC1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0xCF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0xDB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0xE7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xF1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0x3F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x81}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0xF0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0x2B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x63}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0xBB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0xE8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0x0B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0x40}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x4C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x53}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x5C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0x5A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0x5F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xCB}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x08}},//PWMDIV=8
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* New gamma setting end */

#endif

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(exit_sleep), exit_sleep},

	/* for cut 1 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sw_wr00), sw_wr00},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sw_wr01), sw_wr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sw_wr02), sw_wr02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sw_wr03), sw_wr03},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmd_page_0), cmd_page_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x33}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0x34}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x00}},

	/* For random dot noise */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	/* Enable CABC setting */
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

	/* NVT: Enable vivid-color, but disable CABC, please set register(55h) as 0x80  */
	/*{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x80}},*/

	/* NVT: Enable vivid-color, and enable CABC UI-Mode, please set register(55h) as 0x81 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x81}}, */

	/* NVT: Enable vivid-color, and enable CABC Still-Mode, please set register(55h) as 0x82 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55, 0x82} }, */

	/* NVT: Enable vivid-color, and enable CABC Moving-Mode, please set register(55h) as 0x83 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x83}},


	/* {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep}, */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24} },

	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc auo_panel_video_mode_cmds_c2[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef JEL_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif

#if 1
	/* vivi color ver 2 */

	// enable smart color
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x66}},

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

	// add smart color
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x06}},  // add redden
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x00}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
#endif

#if 1
	/* New gamma setting start */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr01), swr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr02), swr02},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0x4B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0x8C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0x92}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xA5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xB7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0xC6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0xD4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0x29}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0xA2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0xE9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x25}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x27}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0x5D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0x93}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0xB6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0xE6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0x08}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x37}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x5A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0x6A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0x75}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0x8D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xA2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0xB9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0x4B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0x8C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0x92}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xA5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xB7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0xC6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0xD4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0x29}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0xA2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0xE9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x25}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x27}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0x5D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0x93}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0xB6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0xE6}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0x08}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x37}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x5A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0x6A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0x75}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0x8D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xA2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEC, 0xB9}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xED, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEE, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF0, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF2, 0xD4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF4, 0xE0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF6, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF8, 0xFA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFA, 0x05}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x00, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x01, 0x10}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x1A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x24}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x06, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x07, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x64}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0B, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0xBD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0xFD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x10, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x11, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x14, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x15, 0x61}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x16, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x17, 0x94}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x18, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x19, 0xB5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1A, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0xE2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x2C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x38}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x24, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x56}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x27, 0x66}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x29, 0x7C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2A, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2B, 0x8D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2F, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x30, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x31, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x32, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x33, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x34, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x35, 0xD4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x36, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x37, 0xE0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x38, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3A, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3B, 0xFA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3D, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3F, 0x05}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x40, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x41, 0x10}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x42, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x43, 0x1A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x44, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x45, 0x24}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x47, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x48, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x49, 0x64}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4B, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4D, 0xBD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4F, 0xFD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x50, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x51, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x52, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x54, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55, 0x61}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x56, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x58, 0x94}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x59, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5A, 0xB5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5C, 0xE2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5E, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x60, 0x2C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x61, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x62, 0x38}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x63, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x64, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x65, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x66, 0x56}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x67, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x68, 0x66}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x69, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x7C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6C, 0x8D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6E, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x70, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x71, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x72, 0x9B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x73, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x74, 0xA1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0xB3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0xC1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0xCF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0xDB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0xE7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xF1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0x3F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x81}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0xF0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0x2B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x63}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0xBB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0xE8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0x0B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0x40}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x4C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x53}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x5C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0x5A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0x5F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0x9B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0xA1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0xB3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0xC1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0xCF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0xDB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0xE7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xF1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xFC}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0x3F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x81}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0xAA}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0xF0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0x2B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0x2E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x63}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0xBB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0xE8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0x0B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0x40}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x4C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x53}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x5C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0x5A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0x5F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xCB}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x08}},//PWMDIV=8
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* New gamma setting end */
#endif

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(exit_sleep), exit_sleep},

	/* For random dot noise */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	/* Enable CABC setting */
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

	/* NVT: Enable vivid-color, but disable CABC, please set register(55h) as 0x80  */
	/*{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x80}},*/

	/* NVT: Enable vivid-color, and enable CABC UI-Mode, please set register(55h) as 0x81 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x81}}, */

	/* NVT: Enable vivid-color, and enable CABC Still-Mode, please set register(55h) as 0x82 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x82}}, */

	/* NVT: Enable vivid-color, and enable CABC Moving-Mode, please set register(55h) as 0x83 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x83}},


	/* {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep}, */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24}},

	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc auo_panel_video_mode_cmds_c3[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef JEL_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif

#if 1
	/* vivi color ver 2 */

	// enable smart color
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x66}},

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

	// add smart color
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x06}},  // add redden
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x00}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
#endif

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x08}},//PWMDIV=8
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},

	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(exit_sleep), exit_sleep},

	/* For random dot noise */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	/* Enable CABC setting */
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

	/* NVT: Enable vivid-color, but disable CABC, please set register(55h) as 0x80  */
	/*{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x80}},*/

	/* NVT: Enable vivid-color, and enable CABC UI-Mode, please set register(55h) as 0x81 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x81}}, */

	/* NVT: Enable vivid-color, and enable CABC Still-Mode, please set register(55h) as 0x82 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x82}}, */

	/* NVT: Enable vivid-color, and enable CABC Moving-Mode, please set register(55h) as 0x83 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x83}},


	/* {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep}, */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24}},

	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc auo_panel_video_mode_cmds_c3_1[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef JEL_CMD_MODE_PANEL
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_cmd), display_mode_cmd},
#else
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
#endif

#if 1
	/* vivi color ver 2 */

	// enable smart color
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x66}},

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

	// add smart color
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x06}},  // add redden
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x00}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
#endif

#if 1
	/* New gamma setting start */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr01), swr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr02), swr02},


	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0x7D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0x8A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0xB1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0xBF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xCF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xDD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0xE8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0xF2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x1F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0x41}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0x78}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0xA5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x29}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x2A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0x5D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0x93}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0xB8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0xE7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0x07}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x37}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x56}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0x66}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0x7A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0x93}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xA3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0xB4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0x7D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0x8A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0x9C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0xB1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0xBF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xCF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xDD}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0xE8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0xF2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x1F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0x41}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0x78}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0xA5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x29}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x2A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0x5D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0x93}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0xB8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0xE7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0x07}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x37}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x46}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x56}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0x66}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0x7A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0x93}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xA3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEC, 0xB4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xED, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEE, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF0, 0xED}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF2, 0xF3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF4, 0xFE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF6, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF8, 0x13}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xF9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFA, 0x1D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x00, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x01, 0x26}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x02, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x03, 0x2F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x04, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x05, 0x37}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x06, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x07, 0x56}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0B, 0x9D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0xC2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x10, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x11, 0x31}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x14, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x15, 0x60}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x16, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x17, 0x94}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x18, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x19, 0xB5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1A, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0xE3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1C, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1E, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1F, 0x2D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x20, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x21, 0x3A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x22, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x48}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x24, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x57}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x26, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x27, 0x68}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x29, 0x7B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2A, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2B, 0x90}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2D, 0x03}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2F, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x30, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x31, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x32, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x33, 0xED}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x34, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x35, 0xF3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x36, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x37, 0xFE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x38, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3B, 0x13}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3D, 0x01}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x3F, 0x1D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x40, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x41, 0x26}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x42, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x43, 0x2F}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x44, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x45, 0x37}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x47, 0x56}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x48, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x49, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4B, 0x9D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4C, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4D, 0xC2}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4E, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x4F, 0xFF}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x50, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x51, 0x31}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x52, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x32}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x54, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55, 0x60}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x56, 0x02}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x58, 0x94}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x59, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5A, 0xB5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5B, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5C, 0xE3}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5E, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x5F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x60, 0x2D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x61, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x62, 0x3A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x63, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x64, 0x48}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x65, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x66, 0x57}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x67, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x68, 0x68}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x69, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x7B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6C, 0x90}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6E, 0xA0}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x70, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x71, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x72, 0x19}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x73, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x74, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x75, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x76, 0x55}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x77, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x78, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x79, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7A, 0x83}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7B, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7C, 0x99}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7D, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7E, 0xA8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x7F, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x80, 0xB7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x81, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x82, 0xC5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x83, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x84, 0xF7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x85, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x86, 0x1E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x87, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x88, 0x60}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x89, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8A, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8B, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8C, 0xE1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8D, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8E, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x8F, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x90, 0x23}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x91, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x92, 0x59}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x93, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x94, 0x94}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x95, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x96, 0xB4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x97, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0xE1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x99, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9A, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9B, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9C, 0x28}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9D, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9E, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x9F, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA0, 0x37}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA2, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA3, 0x3B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA4, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA5, 0x40}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA6, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA7, 0x50}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xA9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAA, 0x6D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAC, 0x80}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAE, 0xCB}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xAF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB0, 0x19}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB2, 0x36}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB3, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB4, 0x55}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB5, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB6, 0x70}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB7, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB8, 0x83}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xB9, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBA, 0x99}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBB, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBC, 0xA8}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBD, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBE, 0xB7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xBF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC0, 0xC5}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC1, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC2, 0xF7}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC3, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC4, 0x1E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC5, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC6, 0x60}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC7, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC8, 0x95}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xC9, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCA, 0xE1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCB, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCC, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCD, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCE, 0x23}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xCF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD0, 0x59}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD1, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD2, 0x94}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD3, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD4, 0xB4}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD5, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD6, 0xE1}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD8, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xD9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDA, 0x28}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDB, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDC, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDD, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDE, 0x37}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xDF, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE0, 0x3B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE1, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE2, 0x40}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE3, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE4, 0x50}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE5, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE6, 0x6D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE7, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE8, 0x80}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xE9, 0x03}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xEA, 0xCB}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* New gamma setting end */
#endif
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x08}},//PWMDIV=8
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},

	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(exit_sleep), exit_sleep},

	/* For random dot noise */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x50} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x02} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6A, 0x60} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	/* Enable CABC setting */
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

	/* NVT: Enable vivid-color, but disable CABC, please set register(55h) as 0x80  */
	/*{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x80}},*/

	/* NVT: Enable vivid-color, and enable CABC UI-Mode, please set register(55h) as 0x81 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x81}}, */

	/* NVT: Enable vivid-color, and enable CABC Still-Mode, please set register(55h) as 0x82 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x82}}, */

	/* NVT: Enable vivid-color, and enable CABC Moving-Mode, please set register(55h) as 0x83 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x83}},


	/* {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep}, */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24}},

	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static int jet_send_display_cmds(struct dsi_cmd_desc *cmd, int cnt,
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
	if (ret < 0)
		pr_err("%s failed (%d)\n", __func__, ret);
	return ret;
}

int mipi_lcd_on = 1;
static int mipi_jet_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (mipi_lcd_on)
		return 0;

	mipi = &mfd->panel_info.mipi;

	jet_send_display_cmds(nvt_LowTemp_wrkr_enter,
			ARRAY_SIZE(nvt_LowTemp_wrkr_enter), false);

	jet_send_display_cmds(nvt_LowTemp_wrkr_exit,
			ARRAY_SIZE(nvt_LowTemp_wrkr_exit), false);

	gpio_set_value(JET_GPIO_LCD_RSTz, 0);
	msleep(1);
	gpio_set_value(JET_GPIO_LCD_RSTz, 1);
	msleep(20);

	if (panel_type != PANEL_ID_NONE) {
		if (mipi->mode == DSI_VIDEO_MODE) {
			jet_send_display_cmds(jet_video_on_cmds,
					jet_video_on_cmds_count, false);
			pr_info("%s: panel_type video mode (%d)", __func__, panel_type);
		} else {
			jet_send_display_cmds(jet_command_on_cmds,
					jet_command_on_cmds_count, false);
			pr_info("%s: panel_type command mode (%d)", __func__, panel_type);
		}
	} else
		pr_info("%s: panel_type not supported!(%d)", __func__, panel_type);
	mipi_lcd_on = 1;

	return 0;
}

static int mipi_jet_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (!mipi_lcd_on)
		return 0;

	if (panel_type != PANEL_ID_NONE) {
		pr_info("%s\n", __func__);
		jet_send_display_cmds(jet_display_off_cmds,
				jet_display_off_cmds_count, false);
	}

	mipi_lcd_on = 0;

	return 0;
}

static unsigned char jet_shrink_pwm(int val)
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

	return shrink_br;
}

static void mipi_jet_set_backlight(struct msm_fb_data_type *mfd)
{
	led_pwm1[1] = jet_shrink_pwm(mfd->bl_level);

	if (mfd->bl_level == 0)
		jet_send_display_cmds(disable_dim,
				ARRAY_SIZE(disable_dim), false);

	jet_send_display_cmds(jet_cmd_backlight_cmds,
			jet_cmd_backlight_cmds_count, true);
}

static void mipi_jet_per_panel_fcts_init(void)
{
	/* Common parts */
	jet_cmd_backlight_cmds = cmd_bkl_cmds;
	jet_cmd_backlight_cmds_count = ARRAY_SIZE(cmd_bkl_cmds);

	jet_display_off_cmds = sony_display_off_cmds;
	jet_display_off_cmds_count = ARRAY_SIZE(sony_display_off_cmds);

	if (panel_type == PANEL_ID_FIGHTER_LG_NT) {
		jet_display_off_cmds = novatek_display_off_cmds;
		jet_display_off_cmds_count = ARRAY_SIZE(novatek_display_off_cmds);

		jet_command_on_cmds = lg_novatek_cmd_on_cmds;
		jet_command_on_cmds_count = ARRAY_SIZE(lg_novatek_cmd_on_cmds);
	}
	if (panel_type == PANEL_ID_JET_SONY_NT) {
		jet_video_on_cmds = sony_c1_video_on_cmds;
		jet_video_on_cmds_count = ARRAY_SIZE(sony_c1_video_on_cmds);

		jet_command_on_cmds = sony_c1_video_on_cmds;
		jet_command_on_cmds_count = ARRAY_SIZE(sony_c1_video_on_cmds);
	}
	else if (panel_type == PANEL_ID_JET_SONY_NT_C1) {
		jet_video_on_cmds = sony_c1_video_on_cmds;
		jet_video_on_cmds_count = ARRAY_SIZE(sony_c1_video_on_cmds);

		jet_command_on_cmds = sony_c1_video_on_cmds;
		jet_command_on_cmds_count = ARRAY_SIZE(sony_c1_video_on_cmds);
	}
	else if (panel_type == PANEL_ID_JET_SONY_NT_C2) {
		jet_video_on_cmds = sony_panel_video_mode_cmds_c2;
		jet_video_on_cmds_count = ARRAY_SIZE(sony_panel_video_mode_cmds_c2);

		jet_command_on_cmds = sony_panel_video_mode_cmds_c2;
		jet_command_on_cmds_count = ARRAY_SIZE(sony_panel_video_mode_cmds_c2);
	}
	else if (panel_type == PANEL_ID_JET_AUO_NT) {
		jet_video_on_cmds = auo_panel_video_mode_cmds;
		jet_video_on_cmds_count = ARRAY_SIZE(auo_panel_video_mode_cmds);

		jet_command_on_cmds = auo_panel_video_mode_cmds;
		jet_command_on_cmds_count = ARRAY_SIZE(auo_panel_video_mode_cmds);
	}
	else if (panel_type == PANEL_ID_JET_AUO_NT_C2) {
		jet_video_on_cmds = auo_panel_video_mode_cmds_c2;
		jet_video_on_cmds_count = ARRAY_SIZE(auo_panel_video_mode_cmds_c2);

		jet_command_on_cmds = auo_panel_video_mode_cmds_c2;
		jet_command_on_cmds_count = ARRAY_SIZE(auo_panel_video_mode_cmds_c2);
	}
	else if (panel_type == PANEL_ID_JET_AUO_NT_C3) {
		jet_video_on_cmds = auo_panel_video_mode_cmds_c3;
		jet_video_on_cmds_count = ARRAY_SIZE(auo_panel_video_mode_cmds_c3);

		jet_command_on_cmds = auo_panel_video_mode_cmds_c3;
		jet_command_on_cmds_count = ARRAY_SIZE(auo_panel_video_mode_cmds_c3);
	}
	else if (panel_type == PANEL_ID_JET_AUO_NT_C3_1) {
		jet_video_on_cmds = auo_panel_video_mode_cmds_c3_1;
		jet_video_on_cmds_count = ARRAY_SIZE(auo_panel_video_mode_cmds_c3_1);

		jet_command_on_cmds = auo_panel_video_mode_cmds_c3_1;
		jet_command_on_cmds_count = ARRAY_SIZE(auo_panel_video_mode_cmds_c3_1);
	}
}

static int __devinit mipi_jet_lcd_probe(struct platform_device *pdev)
{
	mipi_jet_per_panel_fcts_init();

	if (pdev->id == 0) {
		mipi_jet_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);
	return 0;
}

/* HTC specific functions */
#ifdef CONFIG_FB_MSM_CABC
void mipi_jet_enable_ic_cabc(int cabc, bool dim_on, struct msm_fb_data_type *mfd)
{
	if (dim_on)
		dim_cmds = enable_dim;
	if (cabc == 1)
		cabc_cmds = cabc_on_ui;
	else if (cabc == 2)
		cabc_cmds = cabc_on_still;
	else if (cabc == 3)
		cabc_cmds = cabc_on_moving;

	jet_send_display_cmds(dim_cmds, ARRAY_SIZE(dim_cmds), false);
}
#endif

static struct platform_driver this_driver = {
	.probe  = mipi_jet_lcd_probe,
	.driver = {
		.name   = "mipi_jet",
	},
};

static struct msm_fb_panel_data jet_panel_data = {
	.on = mipi_jet_lcd_on,
	.off = mipi_jet_lcd_off,
	.set_backlight = mipi_jet_set_backlight,
#ifdef CONFIG_FB_MSM_CABC
	.enable_cabc = mipi_jet_enable_ic_cabc,
#endif
};

static int ch_used[3];

int mipi_jet_device_register(struct msm_panel_info *pinfo,
		u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_jet_lcd_init();
	if (ret) {
		pr_err("mipi_jet_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_jet", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	jet_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &jet_panel_data,
			sizeof(jet_panel_data));
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

static int mipi_jet_lcd_init(void)
{
	return platform_driver_register(&this_driver);
}
