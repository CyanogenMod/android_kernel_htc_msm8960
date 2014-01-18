#include <mach/panel_id.h>
#include "../../../drivers/video/msm/msm_fb.h"
#include "../../../drivers/video/msm/mipi_dsi.h"
#include "mipi_ville.h"

static struct mipi_dsi_panel_platform_data *mipi_ville_pdata;
static struct dsi_cmd_desc *display_on_cmds = NULL;
static struct dsi_cmd_desc *display_off_cmds = NULL;
static struct dsi_cmd_desc *cmd_on_cmds = NULL;
static int display_on_cmds_count = 0;
static int display_off_cmds_count = 0;
static int cmd_on_cmds_count = 0;
static int mipi_ville_lcd_init(void);
static void mipi_ville_set_backlight(struct msm_fb_data_type *mfd);
static int cur_bl_level = 0;

static char enter_sleep[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */
static char exit_sleep[2] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */
static char display_off[2] = {0x28, 0x00}; /* DTYPE_DCS_WRITE */
static char display_on[2] = {0x29, 0x00}; /* DTYPE_DCS_WRITE */
static char enable_te[2] = {0x35, 0x00}; /* DTYPE_DCS_WRITE1 */

static char ville_panel_width[] = {0x2A, 0x00, 0x1E, 0x02, 0x39}; /* DTYPE_DCS_LWRITE */
static char ville_panel_height[] = {0x2B, 0x00, 0x00, 0x03, 0xBF}; /* DTYPE_DCS_LWRITE */
static char ville_panel_vinit[] = {0xD1, 0x8A}; /* DTYPE_DCS_WRITE1 */

static char vle_e0[] = {0xF0, 0x5A, 0x5A}; /* DTYPE_DCS_LWRITE */
static char vle_e1[] = {0xF1, 0x5A, 0x5A}; /* DTYPE_DCS_LWRITE */
static char vle_e22[] = {0xFC, 0x5A, 0x5A}; /* DTYPE_DCS_LWRITE */
static char vle_g0[] = {
	0xFA, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; /* DTYPE_DCS_LWRITE */
static char vle_g1[] = {0xFA, 0x03}; /* DTYPE_DCS_WRITE1 */
static char vle_p0[] = {
	0xF8, 0x27, 0x27, 0x08,
	0x08, 0x4E, 0xAA, 0x5E,
	0x8A, 0x10, 0x3F, 0x10, 0x10, 0x00}; /* DTYPE_DCS_LWRITE */
static char vle_e2[] = {0xF6, 0x00, 0x84, 0x09}; /* DTYPE_DCS_LWRITE */
static char vle_e3[] = {0xB0, 0x09}; /* DTYPE_DCS_WRITE1 */
static char vle_e4[] = {0xD5, 0x64}; /* DTYPE_DCS_WRITE1 */
static char vle_e5[] = {0xB0, 0x0B}; /* DTYPE_DCS_WRITE1 */
static char vle_e6[] = {0xD5, 0xA4, 0x7E, 0x20}; /* DTYPE_DCS_LWRITE */
static char vle_e7[] = {0xF7, 0x03}; /* DTYPE_DCS_WRITE1 */
static char vle_e8[] = {0xB0, 0x02}; /* DTYPE_DCS_WRITE1 */
static char vle_e9[] = {0xB3, 0xC3}; /* DTYPE_DCS_WRITE1 */
static char vle_e10[] = {0xB0, 0x08}; /* DTYPE_DCS_WRITE1 */
static char vle_e11[] = {0xFD, 0xF8}; /* DTYPE_DCS_WRITE1 */
static char vle_e12[] = {0xB0, 0x04}; /* DTYPE_DCS_WRITE1 */
static char vle_e13[] = {0xF2, 0x4D}; /* DTYPE_DCS_WRITE1 */
static char vle_e14[] = {0xB0, 0x05}; /* DTYPE_DCS_WRITE1 */
static char vle_e15[] = {0xFD, 0x1F}; /* DTYPE_DCS_WRITE1 */
static char vle_e16[] = {0xB1, 0x01, 0x00, 0x16}; /* DTYPE_DCS_LWRITE */
static char vle_e17[] = {0xB2, 0x10, 0x10, 0x10, 0x10}; /* DTYPE_DCS_LWRITE */
static char vle_e17_C2[] = {0xB2, 0x15, 0x15, 0x15, 0x15}; /* DTYPE_DCS_LWRITE */

static struct dsi_cmd_desc ville_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc ville_cmd_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_e0), vle_e0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_e1), vle_e1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_e22), vle_e22},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_g0), vle_g0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_g1), vle_g1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_p0), vle_p0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_e2), vle_e2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e3), vle_e3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e4), vle_e4},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e5), vle_e5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_e6), vle_e6},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e7), vle_e7},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e8), vle_e8},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e9), vle_e9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e10), vle_e10},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e11), vle_e11},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e12), vle_e12},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e13), vle_e13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e14), vle_e14},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e15), vle_e15},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_e16), vle_e16},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_e17), vle_e17},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(enable_te), enable_te},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(ville_panel_width), ville_panel_width},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(ville_panel_height), ville_panel_height},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(ville_panel_vinit), ville_panel_vinit},
};

static struct dsi_cmd_desc ville_cmd_on_cmds_c2[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_e0), vle_e0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_e1), vle_e1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_e22), vle_e22},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_g0), vle_g0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_g1), vle_g1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_p0), vle_p0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_e2), vle_e2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e3), vle_e3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e4), vle_e4},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e5), vle_e5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_e6), vle_e6},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e7), vle_e7},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e8), vle_e8},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e9), vle_e9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e10), vle_e10},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e11), vle_e11},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e12), vle_e12},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e13), vle_e13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e14), vle_e14},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_e15), vle_e15},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_e16), vle_e16},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vle_e17_C2), vle_e17_C2},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(enable_te), enable_te},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(ville_panel_width), ville_panel_width},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(ville_panel_height), ville_panel_height},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(ville_panel_vinit), ville_panel_vinit},
};


static struct dsi_cmd_desc ville_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(enter_sleep), enter_sleep}
};

/* AUO initial setting command */
static char enable_page_cmd[6] = {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00};
static char enable_page_cmd2[2] = {0xB5, 0x40};
static char enable_page_cmd3[2] = {0xC7, 0x00};
static char enable_page2_cmd[6] = {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x02};
static char eng_setting_cmd[3] = {0xFE, 0x08, 0x10};
static char eng_setting_cmd2[2] = {0xEE, 0x1D};
static char eng_setting_cmd3[3] = {0xC3, 0xF2, 0xD5};
static char enable_page1_cmd[6] = {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x01};
static char avdd_cmd[4] = {0xB0, 0x05, 0x05, 0x05};
static char vghr_cmd[4] = {0xB4, 0x05, 0x05, 0x05};
static char vglr_cmd[4] = {0xB5, 0x08, 0x08, 0x08};
static char avdd_boosting_cmds[4] = {0xB6, 0x44, 0x44, 0x44};
static char vgh_boosting_cmd[4] = {0xB9, 0x04, 0x04, 0x04};
static char vgl_boosting_cmds[4] = {0xBA, 0x14, 0x14, 0x14};
static char vgsp_cmd[4] = {0xBC, 0x00, 0x60, 0x00};
static char vrefn_cmd[4] = {0xBE, 0x22, 0x75, 0x70};
static char hori_flip_cmd[] = {0x36, 0x02};
static char turn_on_peri_cmd[2] = {0x32, 0x00};
static char sleep_out_cmd[2] = {0x11, 0x00};

static struct dsi_cmd_desc auo_cmd_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(enable_page_cmd), enable_page_cmd},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(enable_page_cmd2), enable_page_cmd2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(enable_page_cmd3), enable_page_cmd3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(enable_page2_cmd), enable_page2_cmd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(eng_setting_cmd), eng_setting_cmd},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(eng_setting_cmd2), eng_setting_cmd2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(eng_setting_cmd3), eng_setting_cmd3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(enable_page1_cmd), enable_page1_cmd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(avdd_cmd), avdd_cmd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vghr_cmd), vghr_cmd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vglr_cmd), vglr_cmd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(avdd_boosting_cmds), avdd_boosting_cmds},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vgh_boosting_cmd), vgh_boosting_cmd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vgl_boosting_cmds), vgl_boosting_cmds},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vgsp_cmd), vgsp_cmd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,  sizeof(vrefn_cmd), vrefn_cmd},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(hori_flip_cmd), hori_flip_cmd},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(turn_on_peri_cmd), turn_on_peri_cmd},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(sleep_out_cmd), sleep_out_cmd},
};

static struct dsi_cmd_desc auo_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(enter_sleep), enter_sleep}
};

#define AMOLED_NUM_LEVELS 	ARRAY_SIZE(ville_amoled_gamma_table)
#define AMOLED_NUM_LEVELS_C2 	ARRAY_SIZE(ville_amoled_gamma_table_c2)


static const char ville_amoled_gamma_table[][AMOLED_GAMMA_TABLE_SIZE] = {
	/* level 10 */
	{0xFA, 0x02, 0x10, 0x10, 0x10, 0x59, 0x5F, 0x66, 0x81, 0x80,
		0x7F, 0xD9, 0xC3, 0xD5, 0xCD, 0xA3, 0xC7, 0xDB, 0xD2, 0xDD,
		0x00, 0x42, 0x00, 0x2B, 0x00, 0x47},
	/* level 40 */
	{0xFA, 0x02, 0x10, 0x10, 0x10, 0x8B, 0x91, 0x98, 0xD1, 0xA8,
		0xC9, 0xE3, 0xC5, 0xDD, 0xC5, 0xC2, 0xC8, 0xD6, 0xD2, 0xD3,
		0x00, 0x62, 0x00, 0x49, 0x00, 0x60},
	/* level 70 */
	{0xFA, 0x02, 0x10, 0x10, 0x10, 0xAD, 0xA8, 0xB9, 0xD7, 0xAD,
		0xCE, 0xDF, 0xD0, 0xDD, 0xBF, 0xBF, 0xC1, 0xD3, 0xCF, 0xCF,
		0x00, 0x74, 0x00, 0x59, 0x00, 0x82},
	/* level 100 */
	{0xFA, 0x02, 0x10, 0x10, 0x10, 0xBD, 0xAC, 0xC6, 0xD4, 0xAC,
		0xCC, 0xDF, 0xD9, 0xDF, 0xBE, 0xC0, 0xBD, 0xD0, 0xCC, 0xCB,
		0x00, 0x81, 0x00, 0x65, 0x00, 0x92},
	/* level 130 */
	{0xFA, 0x02, 0x10, 0x10, 0x10, 0xC5, 0xAB, 0xCD, 0xD7, 0xB9,
		0xD1, 0xDC, 0xD7, 0xDD, 0xBD, 0xC0, 0xBA, 0xCD, 0xC9, 0xC8,
		0x00, 0x8C, 0x00, 0x6F, 0x00, 0xA0},
	/* level 160 */
	{0xFA, 0x02, 0x10, 0x10, 0x10, 0xD0, 0xB0, 0xD7, 0xD5, 0xBD,
		0xD0, 0xDC, 0xD7, 0xDB, 0xBB, 0xBE, 0xB8, 0xCA, 0xC8, 0xC5,
		0x00, 0x96, 0x00, 0x77, 0x00, 0xAC},
	/* level 200 */
	{0xFA, 0x02, 0x10, 0x10, 0x10, 0xEC, 0xB7, 0xEF, 0xD4, 0xC1,
		0xD0, 0xDC, 0xD8, 0xDB, 0xB9, 0xBC, 0xB5, 0xC8, 0xC8, 0xC3,
		0x00, 0xA1, 0x00, 0x80, 0x00, 0xBA},
	/* level 250 */
	{0xFA, 0x02, 0x10, 0x10, 0x10, 0xE8, 0xB9, 0xEC, 0xD2, 0xC5,
		0xD0, 0xDA, 0xD8, 0xD8, 0xB7, 0xBA, 0xB2, 0xC6, 0xC7, 0xC0,
		0x00, 0xAD, 0x00, 0x8A, 0x00, 0xCA},
	/* level 300 */
	{0xFA, 0x02, 0x10, 0x10, 0x10, 0xEC, 0xB7, 0xEF, 0xD1, 0xCA,
		0xD1, 0xDB, 0xDA, 0xD8, 0xB5, 0xB8, 0xB0, 0xC5, 0xC8, 0xBF,
		0x00, 0xB9, 0x00, 0x93, 0x00, 0xD9},
};

static const char ville_amoled_gamma_table_c2[][AMOLED_GAMMA_TABLE_SIZE] = {
	/* level 10 */
//	{0xFA, 0x02, 0x55, 0x43, 0x58, 0x5A, 0x5A, 0x5A, 0x80, 0x80,
//		0x80, 0xBD, 0xC6, 0xBB, 0x96, 0xA6, 0x93, 0xBD, 0xCD, 0xC6,
//		0x00, 0x4C, 0x00, 0x44, 0x00, 0x57},
	/* level 20 */
	{0xFA, 0x02, 0x55, 0x43, 0x58, 0x63, 0x63, 0x64, 0xA3, 0xA6,
		0xA1, 0xB6, 0xC2, 0xB2, 0x97, 0xAA, 0x9C, 0xBD, 0xC8, 0xC0,
		0x00, 0x5E, 0x00, 0x54, 0x00, 0x69},
	/* level 40 */
	{0xFA, 0x02, 0x55, 0x43, 0x58, 0x8C, 0x8C, 0x8C, 0xA1, 0xAF,
		0x9D, 0xBF, 0xC8, 0xBC, 0x9A, 0xAF, 0xA4, 0xBB, 0xC3, 0xBA,
		0x00, 0x73, 0x00, 0x68, 0x00, 0x81},
	/* level 60 */
	{0xFA, 0x02, 0x55, 0x43, 0x58, 0x91, 0x94, 0x90, 0xA7, 0xB5,
		0xA3, 0xBD, 0xCA, 0xC0, 0x96, 0xA8, 0x9D, 0xBA, 0xC2, 0xB8,
		0x00, 0x83, 0x00, 0x76, 0x00, 0x92},
	/* level 80 */
	{0xFA, 0x02, 0x55, 0x43, 0x58, 0x9B, 0xA0, 0x99, 0xA6, 0xB3,
		0xA2, 0xBD, 0xCC, 0xC4, 0x96, 0xA5, 0x99, 0xBA, 0xC1, 0xB7,
		0x00, 0x8F, 0x00, 0x81, 0x00, 0xA0},
	/* level 100 */
	{0xFA, 0x02, 0x55, 0x43, 0x58, 0xA3, 0xAC, 0xA1, 0xA5, 0xB2,
		0xA2, 0xC0, 0xD0, 0xC8, 0x96, 0xA3, 0x97, 0xB7, 0xBE, 0xB5,
		0x00, 0x9A, 0x00, 0x8B, 0x00, 0xAC},
	/* level 120 */
	{0xFA, 0x02, 0x55, 0x43, 0x58, 0xA6, 0xB1, 0xA3, 0xAA, 0xB8,
		0xA9, 0xBF, 0xCE, 0xC7, 0x95, 0xA1, 0x95, 0xB6, 0xBC, 0xB4,
		0x00, 0xA3, 0x00, 0x94, 0x00, 0xB7},
	/* level 140 */
	{0xFA, 0x02, 0x55, 0x43, 0x58, 0xA9, 0xB6, 0xA6, 0xAE, 0xBC,
		0xB0, 0xBD, 0xCC, 0xC4, 0x97, 0xA2, 0x95, 0xB4, 0xBA, 0xB1,
		0x00, 0xAC, 0x00, 0x9C, 0x00, 0xC1},
	/* level 160 */
	{0xFA, 0x02, 0x55, 0x43, 0x58, 0xA5, 0xB2, 0xA1, 0xAD, 0xBC,
		0xB0, 0xBD, 0xCB, 0xC3, 0x95, 0xA0, 0x93, 0xB1, 0xB7, 0xAE,
		0x00, 0xB5, 0x00, 0xA5, 0x00, 0xCC},
	/* level 180 */
	{0xFA, 0x02, 0x55, 0x43, 0x58, 0xAA, 0xB9, 0xA6, 0xAD, 0xBE,
		0xB2, 0xBE, 0xCA, 0xC3, 0x94, 0x9E, 0x92, 0xB1, 0xB7, 0xAD,
		0x00, 0xBC, 0x00, 0xAC, 0x00, 0xD5},
	/* level 200 */
	{0xFA, 0x02, 0x55, 0x43, 0x58, 0xA6, 0xB6, 0xA2, 0xAC, 0xBD,
		0xB2, 0xBE, 0xC9, 0xC2, 0x93, 0x9E, 0x91, 0xAF, 0xB5, 0xAB,
		0x00, 0xC4, 0x00, 0xB3, 0x00, 0xDD},
	/* level 220 */
	{0xFA, 0x02, 0x55, 0x43, 0x58, 0xAB, 0xBB, 0xA6, 0xAB, 0xBD,
		0xB2, 0xBC, 0xC7, 0xBF, 0x93, 0x9D, 0x90, 0xAF, 0xB4, 0xAB,
		0x00, 0xCA, 0x00, 0xB9, 0x00, 0xE5},
	/* level 240 */
	{0xFA, 0x02, 0x55, 0x43, 0x58, 0xAB, 0xBD, 0xA7, 0xAF, 0xC1,
		0xB7, 0xBC, 0xC6, 0xBE, 0x92, 0x9D, 0x90, 0xAE, 0xB3, 0xA9,
		0x00, 0xD1, 0x00, 0xBF, 0x00, 0xED},
	/* level 260 */
	{0xFA, 0x02, 0x55, 0x43, 0x58, 0xA8, 0xBA, 0xA3, 0xAE, 0xC0,
		0xB7, 0xBC, 0xC5, 0xBD, 0x92, 0x9D, 0x90, 0xAC, 0xB1, 0xA7,
		0x00, 0xD8, 0x00, 0xC5, 0x00, 0xF5},
	/* level 280 */
	{0xFA, 0x02, 0x55, 0x43, 0x58, 0xAC, 0xBF, 0xA7, 0xAC, 0xC0,
		0xB7, 0xBE, 0xC6, 0xBE, 0x90, 0x9A, 0x8D, 0xAC, 0xB1, 0xA7,
		0x00, 0xDE, 0x00, 0xCB, 0x00, 0xFC},
	/* level 300 */
	{0xFA, 0x02, 0x55, 0x43, 0x58, 0xA9, 0xBC, 0xA4, 0xAC, 0xC0,
		0xB7, 0xBE, 0xC6, 0xBE, 0x91, 0x9B, 0x8E, 0xAB, 0xB0, 0xA6,
		0x00, 0xE4, 0x00, 0xD1, 0x01, 0x04},
};

static const char black_gamma[AMOLED_GAMMA_TABLE_SIZE] = {
	0xFA, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static char set_gamma[AMOLED_GAMMA_TABLE_SIZE] = {
	0xFA, 0x02, 0x10, 0x10, 0x10, 0xC5, 0xAB, 0xCD, 0xD7, 0xB9,
	0xD1, 0xDC, 0xD7, 0xDD, 0xBD, 0xC0, 0xBA, 0xCD, 0xC9, 0xC8,
	0x00, 0x8C, 0x00, 0x6F, 0x00, 0xA0};

static struct dsi_cmd_desc ville_cmd_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(set_gamma), set_gamma},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,  sizeof(vle_g1), vle_g1},
};

#if defined (CONFIG_FB_MSM_MDP_ABL)
static struct gamma_curvy smd_gamma_tbl = {
	.gamma_len = 33,
	.bl_len = 8,
	.ref_y_gamma = {0, 1, 2, 4, 8, 16, 24, 33, 45, 59, 74, 94,
		112, 133, 157, 183, 212, 244, 275, 314, 348,
		392, 439, 487, 537, 591, 645, 706, 764, 831,
		896, 963, 1024},
	.ref_y_shade = {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352,
		384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704,
		736, 768, 800, 832, 864, 896, 928, 960, 992, 1024},
	.ref_bl_lvl = {0, 141, 233, 317, 462, 659, 843, 1024},
	.ref_y_lvl = {0, 138, 218, 298, 424, 601, 818, 1024},
};
#endif

static int ville_send_display_cmds(struct dsi_cmd_desc *cmd, int cnt,
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
static int mipi_ville_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi = &mfd->panel_info.mipi;
	if (mipi->mode == DSI_VIDEO_MODE) {
		pr_err("%s: does not support video mode\n", __func__);
		return -EINVAL;
	}

	switch (panel_type) {
	case PANEL_ID_VILLE_SAMSUNG_SG:
	case PANEL_ID_VILLE_SAMSUNG_SG_C2:
		ville_send_display_cmds(cmd_on_cmds, cmd_on_cmds_count, false);
		break;
	case PANEL_ID_VILLE_AUO:
		ville_send_display_cmds(cmd_on_cmds, cmd_on_cmds_count, true);
		break;
	default:
		pr_err("%s: panel_type is not supported!(%d)\n",
			__func__, panel_type);
		return -EINVAL;
	}

	return 0;
}

static int mipi_ville_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (!mipi_lcd_on)
		return 0;

	mipi_lcd_on = 0;

	return 0;
}

static int mipi_ville_display_on(struct platform_device *pdev)
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
		pr_err("%s: does not support video mode\n", __func__);
		return -EINVAL;
	}

	switch (panel_type) {
	case PANEL_ID_VILLE_SAMSUNG_SG:
	case PANEL_ID_VILLE_SAMSUNG_SG_C2:
		ville_send_display_cmds(display_on_cmds,
				display_on_cmds_count, false);
		break;
	case PANEL_ID_VILLE_AUO:
		ville_send_display_cmds(display_on_cmds,
				display_on_cmds_count, true);
		break;
	default:
		pr_err("%s: panel_type is not supported!(%d)\n",
			__func__, panel_type);
		return -EINVAL;
	}
	mipi_lcd_on = 1;

	return 0;
}

static int mipi_ville_display_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (panel_type != PANEL_ID_NONE)
		ville_send_display_cmds(display_off_cmds,
				display_off_cmds_count, false);

	return 0;
}

static unsigned char ville_shrink_pwm(int val)
{
	int i;
	int level, frac, shrink_br = 255;
	unsigned int prev_gamma, next_gammma, interpolate_gamma;

	if(val == 0) {
		for (i = 0; i < AMOLED_GAMMA_TABLE_SIZE; ++i) {
			interpolate_gamma = black_gamma[i];
			set_gamma[i] = (char) interpolate_gamma;
		}
		return val;
	}

	val = clamp(val, BRI_SETTING_MIN, BRI_SETTING_MAX);

	if ((val >= BRI_SETTING_MIN) && (val <= BRI_SETTING_DEF)) {
			shrink_br = (val - BRI_SETTING_MIN) * (PWM_DEFAULT - PWM_MIN) /
		(BRI_SETTING_DEF - BRI_SETTING_MIN) + PWM_MIN;
	} else if (val > BRI_SETTING_DEF && val <= BRI_SETTING_MAX) {
			shrink_br = (val - BRI_SETTING_DEF) * (PWM_MAX - PWM_DEFAULT) /
		(BRI_SETTING_MAX - BRI_SETTING_DEF) + PWM_DEFAULT;
	} else if (val > BRI_SETTING_MAX)
			shrink_br = PWM_MAX;

	level = (shrink_br - AMOLED_MIN_VAL) / AMOLED_LEVEL_STEP;
	frac = (shrink_br - AMOLED_MIN_VAL) % AMOLED_LEVEL_STEP;

	for (i = 0 ; i < AMOLED_GAMMA_TABLE_SIZE ; ++i) {
		if (frac == 0 || level == 8) {
			interpolate_gamma = ville_amoled_gamma_table[level][i];
		} else {
			prev_gamma = ville_amoled_gamma_table[level][i];
			next_gammma = ville_amoled_gamma_table[level+1][i];
			interpolate_gamma = (prev_gamma * (AMOLED_LEVEL_STEP -
									frac) + next_gammma * frac) /
									AMOLED_LEVEL_STEP;
		}
		set_gamma[i] = (char) interpolate_gamma;
	}

	return val;
}

static unsigned char ville_shrink_pwm_c2(int val)
{
	int i;
	int level, frac, shrink_br = 255;
	unsigned int prev_gamma, next_gammma, interpolate_gamma;

	if(val == 0) {
		for (i = 0; i < AMOLED_GAMMA_TABLE_SIZE; ++i) {
			interpolate_gamma = black_gamma[i];
			set_gamma[i] = (char) interpolate_gamma;
		}
		return val;
	}

	val = clamp(val, BRI_SETTING_MIN, BRI_SETTING_MAX);

	if ((val >= BRI_SETTING_MIN) && (val <= BRI_SETTING_DEF)) {
			shrink_br = (val - BRI_SETTING_MIN) * (PWM_DEFAULT - PWM_MIN) /
		(BRI_SETTING_DEF - BRI_SETTING_MIN) + PWM_MIN;
	} else if (val > BRI_SETTING_DEF && val <= BRI_SETTING_MAX) {
			shrink_br = (val - BRI_SETTING_DEF) * (PWM_MAX - PWM_DEFAULT) /
		(BRI_SETTING_MAX - BRI_SETTING_DEF) + PWM_DEFAULT;
	} else if (val > BRI_SETTING_MAX)
			shrink_br = PWM_MAX;

	level = (shrink_br - AMOLED_MIN_VAL) / AMOLED_LEVEL_STEP_C2;
	frac = (shrink_br - AMOLED_MIN_VAL) % AMOLED_LEVEL_STEP_C2;

	for (i = 0; i < AMOLED_GAMMA_TABLE_SIZE - 2; ++i) {
		if (frac == 0 || level == 14) {
			interpolate_gamma = ville_amoled_gamma_table_c2[level][i];
		} else {
			prev_gamma = ville_amoled_gamma_table_c2[level][i];
			next_gammma = ville_amoled_gamma_table_c2[level+1][i];
			interpolate_gamma = (prev_gamma * (AMOLED_LEVEL_STEP_C2 -
									frac) + next_gammma * frac) /
									AMOLED_LEVEL_STEP_C2;
		}
		set_gamma[i] = (char) interpolate_gamma;
	}

	/* special case for SMD gamma setting  */
	if(frac == 0 || level == 14) {
		set_gamma[24] = (char)(ville_amoled_gamma_table_c2[level][24]);
		set_gamma[25] = (char)(ville_amoled_gamma_table_c2[level][25]);
	} else {
		prev_gamma = ville_amoled_gamma_table_c2[level][24] * 256 + ville_amoled_gamma_table_c2[level][25];
		next_gammma = ville_amoled_gamma_table_c2[level+1][24] * 256 + ville_amoled_gamma_table_c2[level+1][25];
		interpolate_gamma = (prev_gamma * (AMOLED_LEVEL_STEP_C2 -
						frac) + next_gammma * frac) /
						AMOLED_LEVEL_STEP_C2;
		set_gamma[24] = interpolate_gamma / 256;
		set_gamma[25] = interpolate_gamma % 256;
	}

	return val;
}

inline void mipi_dsi_set_backlight(struct msm_fb_data_type *mfd, int level)
{
	struct mipi_panel_info *mipi;

	mipi  = &mfd->panel_info.mipi;

	switch (panel_type) {
	case PANEL_ID_VILLE_SAMSUNG_SG:
		ville_shrink_pwm(mfd->bl_level);
		ville_send_display_cmds(ville_cmd_backlight_cmds,
				ARRAY_SIZE(ville_cmd_backlight_cmds), true);
		break;
	case PANEL_ID_VILLE_SAMSUNG_SG_C2:
		ville_shrink_pwm_c2(mfd->bl_level);
		ville_send_display_cmds(ville_cmd_backlight_cmds,
				ARRAY_SIZE(ville_cmd_backlight_cmds), true);
		break;
	default:
		break;
	}
}

static void mipi_ville_set_backlight(struct msm_fb_data_type *mfd)
{
	mipi_dsi_set_backlight(mfd, mfd->bl_level);

	cur_bl_level = mfd->bl_level;
}

static int __devinit mipi_ville_lcd_probe(struct platform_device *pdev)
{
	switch (panel_type) {
	case PANEL_ID_VILLE_SAMSUNG_SG:
		display_off_cmds = ville_display_off_cmds;
		display_off_cmds_count = ARRAY_SIZE(ville_display_off_cmds);
		cmd_on_cmds = ville_cmd_on_cmds;
		cmd_on_cmds_count = ARRAY_SIZE(ville_cmd_on_cmds);
		break;
	case PANEL_ID_VILLE_SAMSUNG_SG_C2:
		display_off_cmds = ville_display_off_cmds;
		display_off_cmds_count = ARRAY_SIZE(ville_display_off_cmds);
		cmd_on_cmds = ville_cmd_on_cmds_c2;
		cmd_on_cmds_count = ARRAY_SIZE(ville_cmd_on_cmds_c2);
		break;
	case PANEL_ID_VILLE_AUO:
		display_off_cmds = auo_display_off_cmds;
		display_off_cmds_count = ARRAY_SIZE(auo_display_off_cmds);
		cmd_on_cmds = auo_cmd_on_cmds;
		cmd_on_cmds_count = ARRAY_SIZE(auo_cmd_on_cmds);
		break;
	default:
		break;
	}
	display_on_cmds = ville_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(ville_display_on_cmds);

	if (pdev->id == 0) {
		mipi_ville_pdata = pdev->dev.platform_data;
		return 0;
	}
	msm_fb_add_device(pdev);
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_ville_lcd_probe,
	.driver = {
		.name = "mipi_ville",
	},
};

static struct msm_fb_panel_data ville_panel_data = {
	.on		= mipi_ville_lcd_on,
	.off		= mipi_ville_lcd_off,
	.set_backlight	= mipi_ville_set_backlight,
	.late_init	= mipi_ville_display_on,
	.early_off	= mipi_ville_display_off,
};

static int ch_used[3];

int mipi_ville_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_ville_lcd_init();
	if (ret) {
		pr_err("mipi_ville_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_ville", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	ville_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &ville_panel_data,
		sizeof(ville_panel_data));
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

static int mipi_ville_lcd_init(void)
{
	return platform_driver_register(&this_driver);
}
