#include <linux/gpio.h>
#include <mach/gpio.h>
#include <mach/panel_id.h>
#include <mach/htc_battery_common.h>
#include "../../../drivers/video/msm/msm_fb.h"
#include "../../../drivers/video/msm/mipi_dsi.h"
#include "../board-elite.h"
#include "mipi_elite.h"

static struct msm_panel_common_pdata *mipi_elite_pdata;
static int mipi_elite_lcd_init(void);
// Selected codes
static struct dsi_cmd_desc *elite_video_on_cmds = NULL;
static int elite_video_on_cmds_count = 0;
static struct dsi_cmd_desc *elite_display_off_cmds = NULL;
static int elite_display_off_cmds_count = 0;
static struct dsi_cmd_desc *elite_cmd_backlight_cmds = NULL;
static int elite_cmd_backlight_cmds_count = 0;

/* All MIPI codes .. */
static char display_on[2] = {0x29, 0x00}; /* DTYPE_DCS_WRITE */
static char display_off[2] = {0x28, 0x00}; /* DTYPE_DCS_WRITE */
static char enter_sleep[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */

static struct dsi_cmd_desc display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(enter_sleep), enter_sleep}
};

static char led_pwm1[2] = {0x51, 0xF0};

static struct dsi_cmd_desc cmd_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(led_pwm1), led_pwm1}
};

static char set_threelane[2] = {0xBA, 0x02}; /* DTYPE_DCS_WRITE-1 */

static char display_mode_video[2] = {0xC2, 0x03}; /* DTYPE_DCS_WRITE-1 */

#ifdef EVA_CMD_MODE_PANEL
static char display_mode_cmd[2] = {0xC2, 0x08}; /* DTYPE_DCS_WRITE-1 */
#endif

static char exit_sleep[2] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */

static char led_pwm2[2] = {0x53, 0x24};
static char led_pwm3[2] = {0x55, 0x00};
static char enable_te[2] = {0x35, 0x00};/* DTYPE_DCS_WRITE1 */
static char pwm_freq[] = {0xC9, 0x0F, 0x04, 0x1E, 0x1E,
	0x00, 0x00, 0x00, 0x10, 0x3E};/* 9.41kHz */
static char swr01[2] = {0x01, 0x33};
static char swr02[2] = {0x02, 0x53};

static struct dsi_cmd_desc sony_panel_video_mode_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef EVA_CMD_MODE_PANEL
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
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* gamma 2.2 6b setting end */
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
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x80}}, */

	/* NVT: Enable vivid-color, and enable CABC UI-Mode, please set register(55h) as 0x81 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x81}}, */

	/* NVT: Enable vivid-color, and enable CABC Still-Mode, please set register(55h) as 0x82 */
	/* {DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x82}}, */

	/* NVT: Enable vivid-color, and enable CABC Moving-Mode, please set register(55h) as 0x83 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x55,0x83}},


	/* {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep},*/
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24}},

	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sony_panel_video_mode_cmds_id28103[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef EVA_CMD_MODE_PANEL
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
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* gamma 2.2 6b setting end */
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
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(exit_sleep), exit_sleep},

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


	/*    {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep},*/
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x53, 0x24} },

	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

/* himax command begin */
/* himax mipi commands */
static char himax_max_pkt_size[2] = {0x03, 0x00};
static char himax_password[4] = {0xB9, 0xFF, 0x83, 0x92}; /* DTYPE_DCS_LWRITE */

static struct dsi_cmd_desc sharp_nt_video_on_cmds_idA1B100[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef EVA_CMD_MODE_PANEL
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
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x66}},

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

	/* gamma 2.2 6b setting start */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr01), swr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr02), swr02},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* gamma 2.2 6b setting end */

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(exit_sleep), exit_sleep},

	/* For NV1-3 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x05}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x8E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0D, 0x06}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0F, 0x0A}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x10, 0x71}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x23, 0x4D}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6C, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x6D, 0x00}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x37, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x71, 0x2C}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x08, 0x26}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x86}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0E, 0x2B}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFE, 0x01}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x33}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0x34}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},

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

static struct dsi_cmd_desc sharp_nt_video_on_cmds_nv3[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef EVA_CMD_MODE_PANEL
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
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x66}},

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
	/* gamma 2.2 6b setting start */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr01), swr01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(swr02), swr02},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* gamma 2.2 6b setting end */

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(exit_sleep), exit_sleep},

	/* For NV3 */
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0xEE}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x98, 0x73}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x12, 0x33}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x13, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x1B, 0x30}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x39, 0x34}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x46, 0x00}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x05}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x37, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x8E}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00}},

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

static struct dsi_cmd_desc sharp_nt_video_on_cmds_nv4[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
#ifdef EVA_CMD_MODE_PANEL
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
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x25, 0x66}},

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

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x02}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x04}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x09, 0x20}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x0A, 0x09}},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01}},
	/* gamma 2.2 6b setting end */
#endif

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x00} },

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFF, 0x05} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0xFB, 0x01} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x28, 0x01} },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, 2, (char[]){0x2F, 0x02} },
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

static struct dsi_cmd_desc himax_video_on_cmds_id311100[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 10, sizeof(himax_password), himax_password},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_threelane), set_threelane},
	{DTYPE_MAX_PKTSIZE, 1, 0, 0, 0, sizeof(himax_max_pkt_size), himax_max_pkt_size},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(display_mode_video), display_mode_video},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, 13, (char[]){0xB2, 0x0F, 0xC8, 0x04, 0x0C,
							   0x04, 0xF4, 0x00, 0xFF, 0x04,
							   0x0C, 0x04, 0x20}},
	{DTYPE_DCS_WRITE, 1, 0, 0, 15, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(pwm_freq), pwm_freq},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(led_pwm3), led_pwm3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(enable_te), enable_te},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

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

/* himax command end */
static char disable_dim_cmd[2] = {0x53, 0x24};/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc disable_dim[] = {{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(disable_dim_cmd), disable_dim_cmd},};
#ifdef CONFIG_FB_MSM_CABC
static struct dsi_cmd_desc *dim_cmds = disable_dim; /* default disable dim */
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

static int elite_send_display_cmds(struct dsi_cmd_desc *cmd, int cnt,
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
static int mipi_elite_lcd_on(struct platform_device *pdev)
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

	if (mipi->mode == DSI_VIDEO_MODE) {
		pr_err("%s: does not support video mode\n",
				__func__);
		return -EINVAL;
	}

	if (panel_type != PANEL_ID_ELITE_SHARP_HX) {
		elite_send_display_cmds(nvt_LowTemp_wrkr_enter,
				ARRAY_SIZE(nvt_LowTemp_wrkr_enter), false);

		elite_send_display_cmds(nvt_LowTemp_wrkr_exit,
				ARRAY_SIZE(nvt_LowTemp_wrkr_exit), false);

		gpio_set_value(ELITE_GPIO_LCD_RSTz, 0);
		hr_msleep(1);
		gpio_set_value(ELITE_GPIO_LCD_RSTz, 1);
		hr_msleep(10);
	}

	if (panel_type == PANEL_ID_ELITE_SONY_NT ||
			panel_type == PANEL_ID_ELITE_SONY_NT_C1 ||
			panel_type == PANEL_ID_ELITE_SONY_NT_C2 ||
			panel_type == PANEL_ID_ELITE_SHARP_NT ||
			panel_type == PANEL_ID_ELITE_SHARP_NT_C1 ||
			panel_type == PANEL_ID_ELITE_SHARP_NT_C2 ||
			panel_type == PANEL_ID_ELITE_SHARP_HX) {
		elite_send_display_cmds(elite_video_on_cmds,
				elite_video_on_cmds_count, false);
		pr_info("%s: panel_type (%d)", __func__, panel_type);
	} else
		pr_err("%s: panel_type is not supported!(%d)", __func__, panel_type);

	mipi_lcd_on = 1;
	return 0;
}

static int mipi_elite_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (!mipi_lcd_on)
		return 0;

	if (panel_type != PANEL_ID_NONE)
		elite_send_display_cmds(elite_display_off_cmds,
				elite_display_off_cmds_count, false);

	mipi_lcd_on = 0;

	return 0;
}

static unsigned char elite_shrink_pwm(int val)
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

static void mipi_elite_set_backlight(struct msm_fb_data_type *mfd)
{
	led_pwm1[1] = elite_shrink_pwm(mfd->bl_level);

	if (mfd->bl_level == 0 || board_mfg_mode() == 4 ||
			(board_mfg_mode() == 5 && !(htc_battery_get_zcharge_mode()%2))) {
		led_pwm1[1] = 0;
	}

	if (mfd->bl_level == 0) {
		elite_send_display_cmds(disable_dim, ARRAY_SIZE(disable_dim),
				false);
	}

	elite_send_display_cmds(elite_cmd_backlight_cmds,
			elite_cmd_backlight_cmds_count, true);
}

static void mipi_elite_per_panel_fcts_init(void)
{
	/* Common parts */

	elite_display_off_cmds = display_off_cmds;
	elite_display_off_cmds_count = ARRAY_SIZE(display_off_cmds);

	elite_cmd_backlight_cmds = cmd_backlight_cmds;
	elite_cmd_backlight_cmds_count = ARRAY_SIZE(cmd_backlight_cmds);

	if (panel_type == PANEL_ID_ELITE_SONY_NT
			|| panel_type == PANEL_ID_ELITE_SONY_NT_C2) {
		pr_info("%s: assign initial setting for SONY_NT*\n",
				__func__);
		elite_video_on_cmds = sony_panel_video_mode_cmds;
		elite_video_on_cmds_count = ARRAY_SIZE(sony_panel_video_mode_cmds);
	} else if (panel_type == PANEL_ID_ELITE_SONY_NT_C1) {
		pr_info("%s: assign initial setting for SONY_NT id 0x28103 Cut1, %s\n",
				__func__, "PANEL_ID_ELITE_SONY_NT_C1");
		elite_video_on_cmds = sony_panel_video_mode_cmds_id28103;
		elite_video_on_cmds_count = ARRAY_SIZE(sony_panel_video_mode_cmds_id28103);
	} else if (panel_type == PANEL_ID_ELITE_SHARP_HX) {
		pr_info("%s: assign initial setting for SHARP_HX, %s\n",
				__func__, "PANEL_ID_ELITE_SHARP_HX");
		elite_video_on_cmds = himax_video_on_cmds_id311100;
		elite_video_on_cmds_count = ARRAY_SIZE(himax_video_on_cmds_id311100);
	} else if (panel_type == PANEL_ID_ELITE_SHARP_NT) {
		pr_info("%s: assign initial setting(NV1-3) for SHARP_NT id 0xA1B100, %s\n",
				__func__, "PANEL_ID_ELITE_SHARP_NT");
		elite_video_on_cmds = sharp_nt_video_on_cmds_idA1B100;
		elite_video_on_cmds_count = ARRAY_SIZE(sharp_nt_video_on_cmds_idA1B100);
	} else if (panel_type == PANEL_ID_ELITE_SHARP_NT_C1) {
		pr_info("%s: assign initial setting(NV3) for SHARP_NT, %s\n",
				__func__, "PANEL_ID_ELITE_SHARP_NT_C1");
		elite_video_on_cmds = sharp_nt_video_on_cmds_nv3;
		elite_video_on_cmds_count = ARRAY_SIZE(sharp_nt_video_on_cmds_nv3);
	} else if (panel_type == PANEL_ID_ELITE_SHARP_NT_C2) {
		pr_info("%s: assign initial setting(NV4) for SHARP_NT Cut2, %s\n",
				__func__, "PANEL_ID_ELITE_SHARP_NT_C2");
		elite_video_on_cmds = sharp_nt_video_on_cmds_nv4;
		elite_video_on_cmds_count = ARRAY_SIZE(sharp_nt_video_on_cmds_nv4);
	}
}

static int __devinit mipi_elite_lcd_probe(struct platform_device *pdev)
{
	mipi_elite_per_panel_fcts_init();

	if (pdev->id == 0) {
		mipi_elite_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);
	return 0;
}

/* HTC specific functionality */
#ifdef CONFIG_FB_MSM_CABC
void mipi_elite_enable_ic_cabc(int cabc, bool dim_on,
		struct msm_fb_data_type *mfd)
{
	if (dim_on)
		cabc_cmds = enable_dim;
	if (cabc == 1)
		cabc_cmds = cabc_on_ui;
	else if (cabc == 2)
		cabc_cmds = cabc_on_still;
	else if (cabc == 3)
		cabc_cmds = cabc_on_moving;

	elite_send_display_cmds(cabc_cmds, ARRAY_SIZE(cabc_cmds), false);
}
#endif

static struct platform_driver this_driver = {
	.probe  = mipi_elite_lcd_probe,
	.driver = {
		.name   = "mipi_elite",
	},
};

static struct msm_fb_panel_data elite_panel_data = {
	.on = mipi_elite_lcd_on,
	.off = mipi_elite_lcd_off,
	.set_backlight = mipi_elite_set_backlight,
#ifdef CONFIG_FB_MSM_CABC
	.enable_cabc = mipi_elite_enable_ic_cabc,
#endif
};

static int ch_used[3];

int mipi_elite_device_register(struct msm_panel_info *pinfo,
		u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_elite_lcd_init();
	if (ret) {
		pr_err("mipi_elite_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_elite", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	elite_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &elite_panel_data,
			sizeof(elite_panel_data));
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

static int mipi_elite_lcd_init(void)
{
	return platform_driver_register(&this_driver);
}
