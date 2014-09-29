#include <mach/panel_id.h>
#include "../../../drivers/video/msm/msm_fb.h"
#include "../../../drivers/video/msm/mipi_dsi.h"
#include <linux/leds.h>
#include <mach/board.h>
#include "mipi_m4.h"

static struct mipi_dsi_panel_platform_data *mipi_m4_pdata;

static struct dsi_cmd_desc *init_on_cmds = NULL;
static struct dsi_cmd_desc *display_on_cmds = NULL;
static struct dsi_cmd_desc *display_off_cmds = NULL;
static struct dsi_cmd_desc *backlight_cmds = NULL;
static int init_on_cmds_count = 0;
static int display_on_cmds_count = 0;
static int display_off_cmds_count = 0;
static int backlight_cmds_count = 0;
static int wled_trigger_initialized;
static atomic_t lcd_power_state;

static char enter_sleep[2] = {0x10, 0x00};
static char exit_sleep[2] = {0x11, 0x00};
static char display_off[2] = {0x28, 0x00};
static char display_on[2] = {0x29, 0x00};

static char led_pwm1[] = {0x51, 0xff};
static char led_pwm2[] = {0x53, 0x24};
static char pwm_off[]  = {0x51, 0x00};
static char counter_measure_on[] = {0xB4, 0x12};
static char counter_measure_off[] = {0xB4, 0x00};

static char himax_b9[] = {0xB9, 0xFF, 0x83, 0x92};
static char himax_d4[] = {0xD4, 0x00};
static char himax_ba[] = {0xBA, 0x12, 0x83, 0x00, 0xD6, 0xC6, 0x00, 0x0A};
static char himax_c0[] = {0xC0, 0x01, 0x94};
static char himax_c6[] = {0xC6, 0x35, 0x00, 0x00, 0x04};
static char himax_d5[] = {0xD5, 0x00, 0x00, 0x02};
static char himax_bf[] = {0xBF, 0x05, 0x60, 0x02};
static char himax_35[] = {0x35, 0x00};
static char himax_c2[] = {0xC2, 0x08};
static char himax_55[] = {0x55, 0x03};
static char cabc_UI[] = {
	0xCA, 0x2D, 0x27, 0x26,
	0x25, 0x25, 0x25, 0x21,
	0x20, 0x20};
static char himax_e3[] = {0xE3, 0x01};
static char himax_e5[] = {
	0xE5, 0x00, 0x04, 0x0B,
	0x05, 0x05, 0x00, 0x80,
	0x20, 0x80, 0x10, 0x00,
	0x07, 0x07, 0x07, 0x07,
	0x07, 0x80, 0x0A};
static char himax_c9[] = {0xC9, 0x1F, 0x00, 0x1E, 0x3F, 0x00, 0x80};


static char cmd2_page0_0[] = {0xFF, 0x01};
static char cmd2_page0_1[] = {0xFB, 0x01};
static char cmd2_page0_data00[] = {0x00, 0x4A};
static char cmd2_page0_data01[] = {0x01, 0x33};
static char cmd2_page0_data02[] = {0x02, 0x53};
static char cmd2_page0_data03[] = {0x03, 0x55};
static char cmd2_page0_data04[] = {0x04, 0x55};
static char cmd2_page0_data05[] = {0x05, 0x33};
static char cmd2_page0_data06[] = {0x06, 0x22};
static char cmd2_page0_data07[] = {0x08, 0x56};
static char cmd2_page0_data08[] = {0x09, 0x8F};
static char cmd2_page0_data09[] = {0x0B, 0x97};
static char cmd2_page0_data10[] = {0x0C, 0x97};
static char cmd2_page0_data11[] = {0x0D, 0x2F};
static char cmd2_page0_data12[] = {0x0E, 0x24};
static char cmd2_page0_data15[] = {0x36, 0x73};
static char cmd2_page0_data16[] = {0x0F, 0x04};

static char cmd3_0[] = {0xFF, 0xEE};
static char cmd3_1[] = {0xFB, 0x01};
static char cmd3_data00[] = {0x04, 0xAD};
static char cmd3_data01[] = {0xFF, 0x00};

static char cmd2_page4_0[] = {0xFF, 0x05};
static char cmd2_page4_1[] = {0xFB, 0x01};
static char cmd2_page4_data00[] = {0x01, 0x00};
static char cmd2_page4_data01[] = {0x02, 0x82};
static char cmd2_page4_data02[] = {0x03, 0x82};
static char cmd2_page4_data03[] = {0x04, 0x82};
static char cmd2_page4_data04[] = {0x05, 0x30};
static char cmd2_page4_data05[] = {0x06, 0x33};
static char cmd2_page4_data06[] = {0x07, 0x01};
static char cmd2_page4_data07[] = {0x08, 0x00};
static char cmd2_page4_data08[] = {0x09, 0x46};
static char cmd2_page4_data09[] = {0x0A, 0x46};
static char cmd2_page4_data10[] = {0x0D, 0x0B};
static char cmd2_page4_data11[] = {0x0E, 0x1D};
static char cmd2_page4_data12[] = {0x0F, 0x08};
static char cmd2_page4_data13[] = {0x10, 0x53};
static char cmd2_page4_data14[] = {0x11, 0x00};
static char cmd2_page4_data15[] = {0x12, 0x00};
static char cmd2_page4_data16[] = {0x14, 0x01};
static char cmd2_page4_data17[] = {0x15, 0x00};
static char cmd2_page4_data18[] = {0x16, 0x05};
static char cmd2_page4_data19[] = {0x17, 0x00};
static char cmd2_page4_data20[] = {0x19, 0x7F};
static char cmd2_page4_data21[] = {0x1A, 0xFF};
static char cmd2_page4_data22[] = {0x1B, 0x0F};
static char cmd2_page4_data23[] = {0x1C, 0x00};
static char cmd2_page4_data24[] = {0x1D, 0x00};
static char cmd2_page4_data25[] = {0x1E, 0x00};
static char cmd2_page4_data26[] = {0x1F, 0x07};
static char cmd2_page4_data27[] = {0x20, 0x00};
static char cmd2_page4_data28[] = {0x21, 0x00};
static char cmd2_page4_data29[] = {0x22, 0x55};
static char cmd2_page4_data30[] = {0x23, 0x4D};
static char cmd2_page4_data31[] = {0x6C, 0x00};
static char cmd2_page4_data32[] = {0x6D, 0x00};
static char cmd2_page4_data33[] = {0x2D, 0x02};
static char cmd2_page4_data34[] = {0x83, 0x02};
static char cmd2_page4_data35[] = {0x9E, 0x58};
static char cmd2_page4_data36[] = {0x9F, 0x6A};
static char cmd2_page4_data37[] = {0xA0, 0x41};
static char cmd2_page4_data38[] = {0xA2, 0x10};
static char cmd2_page4_data39[] = {0xBB, 0x0A};
static char cmd2_page4_data40[] = {0xBC, 0x0A};
static char cmd2_page4_data41[] = {0x28, 0x01};
static char cmd2_page4_data42[] = {0x2F, 0x02};
static char cmd2_page4_data43[] = {0x32, 0x08};
static char cmd2_page4_data44[] = {0x33, 0xB8};
static char cmd2_page4_data45[] = {0x36, 0x02};
static char cmd2_page4_data46[] = {0x37, 0x00};
static char cmd2_page4_data47[] = {0x43, 0x00};
static char cmd2_page4_data48[] = {0x4B, 0x21};
static char cmd2_page4_data49[] = {0x4C, 0x03};
static char cmd2_page4_data50[] = {0x50, 0x21};
static char cmd2_page4_data51[] = {0x51, 0x03};
static char cmd2_page4_data52[] = {0x58, 0x21};
static char cmd2_page4_data53[] = {0x59, 0x03};
static char cmd2_page4_data54[] = {0x5D, 0x21};
static char cmd2_page4_data55[] = {0x5E, 0x03};

static char cmd2_page2_0[] = {0xFF, 0x03};
static char cmd2_page2_1[] = {0xFE, 0x08};
static char cmd2_page2_data00[] = {0x25, 0x26};
static char cmd2_page2_data01[] = {0x00, 0x00};
static char cmd2_page2_data02[] = {0x01, 0x05};
static char cmd2_page2_data03[] = {0x02, 0x10};
static char cmd2_page2_data04[] = {0x03, 0x14};
static char cmd2_page2_data05[] = {0x04, 0x16};
static char cmd2_page2_data06[] = {0x05, 0x18};
static char cmd2_page2_data07[] = {0x06, 0x20};
static char cmd2_page2_data08[] = {0x07, 0x20};
static char cmd2_page2_data09[] = {0x08, 0x18};
static char cmd2_page2_data10[] = {0x09, 0x16};
static char cmd2_page2_data11[] = {0x0A, 0x14};
static char cmd2_page2_data12[] = {0x0B, 0x12};
static char cmd2_page2_data13[] = {0x0C, 0x06};
static char cmd2_page2_data14[] = {0x0D, 0x02};
static char cmd2_page2_data15[] = {0x0E, 0x01};
static char cmd2_page2_data16[] = {0x0F, 0x00};
static char cmd2_page2_data17[] = {0xFB, 0x01};
static char cmd2_page2_data18[] = {0xFF, 0x00};
static char cmd2_page2_data19[] = {0xFE, 0x01};

static char cmd2_page3_0[] = {0xFF, 0x04};
static char cmd2_page3_1[] = {0xFB, 0x01};
static char cmd2_page3_data00[] = {0x0A, 0x03};
static char cmd2_page3_data01[] = {0x05, 0x2D};
static char cmd2_page3_data02[] = {0x21, 0xFF};
static char cmd2_page3_data03[] = {0x22, 0xF7};
static char cmd2_page3_data04[] = {0x23, 0xEF};
static char cmd2_page3_data05[] = {0x24, 0xE7};
static char cmd2_page3_data06[] = {0x25, 0xDF};
static char cmd2_page3_data07[] = {0x26, 0xD7};
static char cmd2_page3_data08[] = {0x27, 0xCF};
static char cmd2_page3_data09[] = {0x28, 0xC7};
static char cmd2_page3_data10[] = {0x29, 0xBF};
static char cmd2_page3_data11[] = {0x2A, 0xB7};

static char cmd1_0[] = {0xFF, 0x00};
static char cmd1_1[] = {0xFB, 0x01};
static char pwm_duty[] = {0x51, 0x07};
static char bl_ctl[4] = {0x53, 0x24};
static char power_save[4] = {0x55, 0x83};
static char power_save2[4] = {0x5E, 0x06};
static char set_display_mode[4] = {0xC2, 0x08};
static char set_mipi_lane[4] = {0xBA, 0x02};
static char set_te_on[4] = {0x35, 0x00};

static struct dsi_cmd_desc sharp_hx_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_b9), himax_b9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,  sizeof(himax_d4), himax_d4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_ba), himax_ba},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_c0), himax_c0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_c6), himax_c6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_d5), himax_d5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_bf), himax_bf},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_c2), himax_c2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 5,  sizeof(himax_e3), himax_e3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_e5), himax_e5},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,  sizeof(himax_35), himax_35},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,  sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 5,  sizeof(himax_55), himax_55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(cabc_UI), cabc_UI},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,  sizeof(himax_c9), himax_c9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,  sizeof(counter_measure_off), counter_measure_off},
};

static struct dsi_cmd_desc auo_nt_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_0), cmd2_page0_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_1), cmd2_page0_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data00), cmd2_page0_data00},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data01), cmd2_page0_data01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data02), cmd2_page0_data02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data03), cmd2_page0_data03},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data04), cmd2_page0_data04},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data05), cmd2_page0_data05},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data06), cmd2_page0_data06},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data07), cmd2_page0_data07},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data08), cmd2_page0_data08},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data09), cmd2_page0_data09},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data10), cmd2_page0_data10},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data11), cmd2_page0_data11},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data12), cmd2_page0_data12},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data15), cmd2_page0_data15},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page0_data16), cmd2_page0_data16},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd3_0), cmd3_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd3_1), cmd3_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd3_data00), cmd3_data00},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd3_data01), cmd3_data01},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_0), cmd2_page4_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_1), cmd2_page4_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data00), cmd2_page4_data00},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data01), cmd2_page4_data01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data02), cmd2_page4_data02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data03), cmd2_page4_data03},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data04), cmd2_page4_data04},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data05), cmd2_page4_data05},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data06), cmd2_page4_data06},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data07), cmd2_page4_data07},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data08), cmd2_page4_data08},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data09), cmd2_page4_data09},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data10), cmd2_page4_data10},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data11), cmd2_page4_data11},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data12), cmd2_page4_data12},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data13), cmd2_page4_data13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data14), cmd2_page4_data14},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data15), cmd2_page4_data15},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data16), cmd2_page4_data16},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data17), cmd2_page4_data17},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data18), cmd2_page4_data18},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data19), cmd2_page4_data19},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data20), cmd2_page4_data20},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data21), cmd2_page4_data21},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data22), cmd2_page4_data22},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data23), cmd2_page4_data23},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data24), cmd2_page4_data24},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data25), cmd2_page4_data25},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data26), cmd2_page4_data26},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data27), cmd2_page4_data27},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data28), cmd2_page4_data28},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data29), cmd2_page4_data29},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data30), cmd2_page4_data30},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data31), cmd2_page4_data31},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data32), cmd2_page4_data32},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data33), cmd2_page4_data33},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data34), cmd2_page4_data34},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data35), cmd2_page4_data35},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data36), cmd2_page4_data36},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data37), cmd2_page4_data37},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data38), cmd2_page4_data38},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data39), cmd2_page4_data39},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data40), cmd2_page4_data40},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data41), cmd2_page4_data41},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data42), cmd2_page4_data42},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data43), cmd2_page4_data43},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data44), cmd2_page4_data44},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data45), cmd2_page4_data45},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data46), cmd2_page4_data46},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data47), cmd2_page4_data47},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data48), cmd2_page4_data48},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data49), cmd2_page4_data49},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data50), cmd2_page4_data50},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data51), cmd2_page4_data51},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data52), cmd2_page4_data52},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data53), cmd2_page4_data53},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data54), cmd2_page4_data54},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page4_data55), cmd2_page4_data55},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_0), cmd2_page2_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_1), cmd2_page2_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data00), cmd2_page2_data00},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data01), cmd2_page2_data01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data02), cmd2_page2_data02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data03), cmd2_page2_data03},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data04), cmd2_page2_data04},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data05), cmd2_page2_data05},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data06), cmd2_page2_data06},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data07), cmd2_page2_data07},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data08), cmd2_page2_data08},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data09), cmd2_page2_data09},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data10), cmd2_page2_data10},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data11), cmd2_page2_data11},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data12), cmd2_page2_data12},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data13), cmd2_page2_data13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data14), cmd2_page2_data14},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data15), cmd2_page2_data15},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data16), cmd2_page2_data16},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data17), cmd2_page2_data17},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data18), cmd2_page2_data18},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page2_data19), cmd2_page2_data19},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_0), cmd2_page3_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_1), cmd2_page3_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data00), cmd2_page3_data00},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data01), cmd2_page3_data01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data02), cmd2_page3_data02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data03), cmd2_page3_data03},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data04), cmd2_page3_data04},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data05), cmd2_page3_data05},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data06), cmd2_page3_data06},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data07), cmd2_page3_data07},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data08), cmd2_page3_data08},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data09), cmd2_page3_data09},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data10), cmd2_page3_data10},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd2_page3_data11), cmd2_page3_data11},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd1_0), cmd1_0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(cmd1_1), cmd1_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(pwm_duty), pwm_duty},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(bl_ctl), bl_ctl},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(power_save), power_save},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(power_save2), power_save2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(set_display_mode), set_display_mode},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(set_mipi_lane), set_mipi_lane},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(set_te_on), set_te_on},
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc sharp_display_off_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(pwm_off), pwm_off},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(counter_measure_on), counter_measure_on},
	{DTYPE_DCS_WRITE, 1, 0, 0, 1,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 130,
		sizeof(enter_sleep), enter_sleep},
};

static struct dsi_cmd_desc sharp_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 40, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sharp_hx_cmd_backlight_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(led_pwm1), led_pwm1},
};

static int m4_send_display_cmds(struct dsi_cmd_desc *cmd, int cnt, bool clk_ctrl)
{
	int ret = 0;
	struct dcs_cmd_req cmdreq;

	cmdreq.cmds = cmd;
	cmdreq.cmds_cnt = cnt;
	cmdreq.flags = CMD_REQ_COMMIT;
	if(clk_ctrl)
		cmdreq.flags |= CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	ret = mipi_dsi_cmdlist_put(&cmdreq);
	if (ret < 0)
		pr_err("%s failed (%d)\n", __func__, ret);
	return ret;
}

static int mipi_m4_lcd_on(struct platform_device *pdev)
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
	mipi  = &mfd->panel_info.mipi;

	m4_send_display_cmds(init_on_cmds, init_on_cmds_count, false);

	atomic_set(&lcd_power_state, 1);

	pr_debug("Init done\n");

	return 0;
}

static int mipi_m4_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	return 0;
}

static int mipi_m4_display_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	bool clk_ctrl;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_op_mode_config(DSI_CMD_MODE);

	clk_ctrl = (mfd->panel_info.type == MIPI_CMD_PANEL);
	m4_send_display_cmds(display_on_cmds, display_on_cmds_count, clk_ctrl);

	return 0;
}

DEFINE_LED_TRIGGER(bkl_led_trigger);

static int mipi_m4_display_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	bool clk_ctrl;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	clk_ctrl = (mfd->panel_info.type == MIPI_CMD_PANEL);
	m4_send_display_cmds(display_off_cmds, display_off_cmds_count, clk_ctrl);

	if (wled_trigger_initialized)
		led_trigger_event(bkl_led_trigger, 0);

	else
		pr_err("%s: wled trigger is not initialized!\n", __func__);

	atomic_set(&lcd_power_state, 0);

	pr_err("%s\n", __func__);

	return 0;
}

#define BRI_SETTING_MIN                 30
#define BRI_SETTING_DEF                 142
#define BRI_SETTING_MAX                 255

static unsigned char m4_shrink_pwm(int val)
{
	unsigned int pwm_min, pwm_default, pwm_max;
	unsigned char shrink_br = BRI_SETTING_MAX;

	pwm_min = 12;
	if (panel_type == PANEL_ID_KIWI_AUO_NT)
		pwm_default = 76;
	else
		pwm_default = 68;
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

	return shrink_br;
}

static void m4_set_backlight(struct msm_fb_data_type *mfd)
{
	struct mipi_panel_info *mipi;
	bool clk_ctrl = (mfd && mfd->panel_info.type == MIPI_CMD_PANEL);

	led_pwm1[1] = m4_shrink_pwm((unsigned char)(mfd->bl_level));

	if (mipi_m4_pdata && (mipi_m4_pdata->enable_wled_bl_ctrl)
	    && (wled_trigger_initialized)) {
		led_trigger_event(bkl_led_trigger, led_pwm1[1]);
		return;
	}

	mipi  = &mfd->panel_info.mipi;
	pr_debug("%s+:bl=%d \n", __func__, mfd->bl_level);

	if (atomic_read(&lcd_power_state) == 0) {
		pr_debug("%s: LCD is off. Skip backlight setting\n", __func__);
		return;
	}
	
	if (mipi->mode == DSI_VIDEO_MODE) {
		return;
	}

	if (mipi->mode == DSI_CMD_MODE) {
		mipi_dsi_op_mode_config(DSI_CMD_MODE);
	}

	m4_send_display_cmds(backlight_cmds, backlight_cmds_count, clk_ctrl);

#ifdef CONFIG_BACKLIGHT_WLED_CABC
	if (wled_trigger_initialized) {
		led_trigger_event(bkl_led_trigger, mfd->bl_level);
	}
#endif
	return;
}

static int __devinit mipi_m4_lcd_probe(struct platform_device *pdev)
{
	if (panel_type == PANEL_ID_KIWI_SHARP_HX) {
		init_on_cmds = sharp_hx_cmd_on_cmds;
		init_on_cmds_count = ARRAY_SIZE(sharp_hx_cmd_on_cmds);
	} else if (panel_type == PANEL_ID_KIWI_AUO_NT) {
		init_on_cmds = auo_nt_cmd_on_cmds;
		init_on_cmds_count = ARRAY_SIZE(auo_nt_cmd_on_cmds);
	}

	display_on_cmds = sharp_display_on_cmds;
	display_on_cmds_count = ARRAY_SIZE(sharp_display_on_cmds);
	display_off_cmds = sharp_display_off_cmds;
	display_off_cmds_count = ARRAY_SIZE(sharp_display_off_cmds);
	backlight_cmds = sharp_hx_cmd_backlight_cmds;
	backlight_cmds_count = ARRAY_SIZE(sharp_hx_cmd_backlight_cmds);

	if (pdev->id == 0) {
		mipi_m4_pdata = pdev->dev.platform_data;
	}
	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_m4_lcd_probe,
	.driver = {
		.name   = "mipi_m4",
	},
};

static struct msm_fb_panel_data m4_panel_data = {
	.on		= mipi_m4_lcd_on,
	.off		= mipi_m4_lcd_off,
	.set_backlight  = m4_set_backlight,
	.late_init = mipi_m4_display_on,
	.early_off = mipi_m4_display_off,
};

static int ch_used[3];

int mipi_m4_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	led_trigger_register_simple("bkl_trigger", &bkl_led_trigger);
	pr_info("%s: SUCCESS (WLED TRIGGER)\n", __func__);
	wled_trigger_initialized = 1;
	atomic_set(&lcd_power_state, 1);

	ret = platform_driver_register(&this_driver);
	if (ret) {
		pr_err("platform_driver_register() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_m4", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	m4_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &m4_panel_data,
		sizeof(m4_panel_data));
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
