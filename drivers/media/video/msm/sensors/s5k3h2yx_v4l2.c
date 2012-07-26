#include "msm_sensor.h"

#ifdef CONFIG_RAWCHIP
#include "rawchip/rawchip.h"
#endif

#define SENSOR_NAME "s5k3h2yx"
#define PLATFORM_DRIVER_NAME "msm_camera_s5k3h2yx"
#define s5k3h2yx_obj s5k3h2yx_##obj

#define S5K3H2YX_REG_READ_MODE 0x0101
#define S5K3H2YX_READ_NORMAL_MODE 0x0000	/* without mirror/flip */
#define S5K3H2YX_READ_MIRROR 0x0001			/* with mirror */
#define S5K3H2YX_READ_FLIP 0x0002			/* with flip */
#define S5K3H2YX_READ_MIRROR_FLIP 0x0003	/* with mirror/flip */

#define DEFAULT_VCM_MAX 73
#define DEFAULT_VCM_MED 35
#define DEFAULT_VCM_MIN 8


DEFINE_MUTEX(s5k3h2yx_mut);
static struct msm_sensor_ctrl_t s5k3h2yx_s_ctrl;

static struct msm_camera_i2c_reg_conf s5k3h2yx_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_groupon_settings[] = {
	{0x104, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_groupoff_settings[] = {
	{0x104, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_mipi_settings[] = {
/*	{0x0101, 0x00},*/
	{0x3065, 0x35},
	{0x310E, 0x00},
	{0x3098, 0xAB},
	{0x30C7, 0x0A},
	{0x309A, 0x01},
	{0x310D, 0xC6},
	{0x30C3, 0x40},
	{0x30BB, 0x02},
	{0x30BC, 0x38},
	{0x30BD, 0x40},
	{0x3110, 0x70},
	{0x3111, 0x80},
	{0x3112, 0x7B},
	{0x3113, 0xC0},
	{0x30C7, 0x1A},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_pll_settings[] = {
	{0x0305, 0x04},/*pre_pll_clk_div = 4*/
	{0x0306, 0x00},/*pll_multiplier*/
	{0x0307, 0x98},/*pll_multiplier  = 152*/
	{0x0303, 0x01},/*vt_sys_clk_div = 1*/
	{0x0301, 0x05},/*vt_pix_clk_div = 5*/
	{0x030B, 0x01},/*op_sys_clk_div = 1*/
	{0x0309, 0x05},/*op_pix_clk_div = 5*/
	{0x30CC, 0xE0},/*DPHY_band_ctrl 870 MHz ~ 950 MHz*/
	{0x31A1, 0x5A},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_prev_settings[] = {
	/* PLL setting*/
	{0x0305, 0x04},/*PRE_PLL_CLK_DIV*/
	{0x0306, 0x00},/*PLL_MULTIPLIER*/
	{0x0307, 0x6C},/*PLL_MULTIPLIER*/
	{0x0303, 0x01},/*VT_SYS_CLK_DIV*/
	{0x0301, 0x05},/*VT_PIX_CLK_DIV*/
	{0x030B, 0x01},/*OP_SYS_CLK_DIV*/
	{0x0309, 0x05},/*OP_PIX_CLK_DIV*/
	{0x30CC, 0xB0},/*DPHY_BAND_CTRL*/
	{0x31A1, 0x56},/*BINNING*/

	/*Timing configuration*/
	{0x0200, 0x02},/*FINE_INTEGRATION_TIME_*/
	{0x0201, 0x50},
	{0x0202, 0x04},/*COARSE_INTEGRATION_TIME*/
	{0x0203, 0xDB},
	{0x0204, 0x00},/*ANALOG_GAIN*/
	{0x0205, 0x20},
	{0x0342, 0x0D},/*LINE_LENGTH_PCK*/
	{0x0343, 0x8E},
#ifdef CONFIG_RAWCHIP
	{0x0340, 0x04},/*FRAME_LENGTH_LINES 1268*/
	{0x0341, 0xF4},
#else
	{0x0340, 0x04},/*FRAME_LENGTH_LINES*/
	{0x0341, 0xE0},
#endif
	/*Output Size (1640x1232)*/
	{0x0344, 0x00},/*X_ADDR_START*/
	{0x0345, 0x00},
	{0x0346, 0x00},/*Y_ADDR_START*/
	{0x0347, 0x00},
	{0x0348, 0x0C},/*X_ADDR_END*/
	{0x0349, 0xCD},
	{0x034A, 0x09},/*Y_ADDR_END*/
	{0x034B, 0x9F},
	{0x0381, 0x01},/*X_EVEN_INC*/
	{0x0383, 0x03},/*X_ODD_INC*/
	{0x0385, 0x01},/*Y_EVEN_INC*/
	{0x0387, 0x03},/*Y_ODD_INC*/
	{0x0401, 0x00},/*DERATING_EN*/
	{0x0405, 0x10},
	{0x0700, 0x05},/*FIFO_WATER_MARK_PIXELS*/
	{0x0701, 0x30},
	{0x034C, 0x06},/*X_OUTPUT_SIZE*/
	{0x034D, 0x68},
	{0x034E, 0x04},/*Y_OUTPUT_SIZE*/
	{0x034F, 0xD0},
	/*Manufacture Setting*/
	{0x300E, 0xED},
	{0x301D, 0x80},
	{0x301A, 0x77},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_video_settings[] = {
	/* PLL setting*/
	{0x0305, 0x04},/*pre_pll_clk_div = 4*/
	{0x0306, 0x00},/*pll_multiplier*/
	{0x0307, 0x98},/*pll_multiplier  = 152*/
	{0x0303, 0x01},/*vt_sys_clk_div = 1*/
	{0x0301, 0x05},/*vt_pix_clk_div = 5*/
	{0x030B, 0x01},/*op_sys_clk_div = 1*/
	{0x0309, 0x05},/*op_pix_clk_div = 5*/
	{0x30CC, 0xE0},/*DPHY_band_ctrl 870 MHz ~ 950 MHz*/
	{0x31A1, 0x5A},

	{ 0x0344 , 0x00 }, /* X addr start 98d */
	{ 0x0345 , 0x62 },
	{ 0x0346 , 0x01 }, /* Y addr start 364d */
	{ 0x0347 , 0x6C },
	{ 0x0348 , 0x0C }, /* X addr end 3181d */
	{ 0x0349 , 0x6D },
	{ 0x034A , 0x08 }, /* Y addr end 2099d */
	{ 0x034B , 0x33 },
	{ 0x0381 , 0x01 }, /* x_even_inc = 1 */
	{ 0x0383 , 0x01 }, /* x_odd_inc = 1 */
	{ 0x0385 , 0x01 }, /* y_even_inc = 1 */
	{ 0x0387 , 0x01 }, /* y_odd_inc = 1 */
	{ 0x0105 , 0x01 }, /* skip corrupted frame - for preview flash when doing hjr af */
	{ 0x0401 , 0x00 }, /* Derating_en  = 0 (disable) */
	{ 0x0405 , 0x10 },
	{ 0x0700 , 0x05 }, /* fifo_water_mark_pixels = 1328 */
	{ 0x0701 , 0x30 },
	{ 0x034C , 0x0C }, /* x_output_size = 3084 */
	{ 0x034D , 0x0C },
	{ 0x034E , 0x06 }, /* y_output_size = 1736 */
	{ 0x034F , 0xC8 },
	{ 0x0200 , 0x02 }, /* fine integration time */
	{ 0x0201 , 0x50 },
	{ 0x0202 , 0x04 }, /* Coarse integration time */
	{ 0x0203 , 0xDB },
	{ 0x0204 , 0x00 }, /* Analog gain */
	{ 0x0205 , 0x20 },
	{ 0x0342 , 0x0D }, /* Line_length_pck 3470d */
	{ 0x0343 , 0x8E },
#ifdef CONFIG_RAWCHIP
	{ 0x0340 , 0x06 }, /* Frame_length_lines 1772d */
	{ 0x0341 , 0xEC },
#else
	{ 0x0340 , 0x06 }, /* Frame_length_lines 1752d */
	{ 0x0341 , 0xD8 },
#endif

	/*Manufacture Setting*/
	{ 0x300E , 0x29 }, /* Reserved  For 912Mbps */
	{ 0x31A3 , 0x00 }, /* Reserved  For 912Mbps */
	{ 0x301A , 0x77 }, /* Reserved */
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_fast_video_settings[] = {
#if 0
  {0x0305, 0x04}, /*pre_pll_clk_div = 4*/
  {0x0306, 0x00}, /*pll_multiplier*/
  {0x0307, 0x98}, /*pll_multiplier  = 152*/
  {0x0303, 0x01}, /*vt_sys_clk_div = 1*/
  {0x0301, 0x05}, /*vt_pix_clk_div = 5*/
  {0x030B, 0x01}, /*op_sys_clk_div = 1*/
  {0x0309, 0x05}, /*op_pix_clk_div = 5*/
  {0x30CC, 0xE0}, /*DPHY_band_ctrl 870 MHz ~ 950 MHz*/
  {0x31A1, 0x5A}, /*"DBR_CLK = PLL_CLK / DIV_DBR(0x31A1[3:0]= 912*/
  /*Readout*/
  /*Address Data  Comment*/
  {0x0344, 0x00}, /*X addr start 112d*/
  {0x0345, 0x70},
  {0x0346, 0x01}, /*Y addr start 372d*/
  {0x0347, 0x74},
  {0x0348, 0x0C}, /*X addr end 3165d*/
  {0x0349, 0x5D},
  {0x034A, 0x08}, /*Y addr end 2091d*/
  {0x034B, 0x2B},

  {0x0381, 0x01}, /*x_even_inc = 1*/
  {0x0383, 0x03}, /*x_odd_inc = 3*/
  {0x0385, 0x01}, /*y_even_inc = 1*/
  {0x0387, 0x03}, /*y_odd_inc = 3*/

  {0x0401, 0x00}, /*Derating_en  = 0 (disable)*/
  {0x0405, 0x10},
  {0x0700, 0x05}, /*fifo_water_mark_pixels = 1328*/
  {0x0701, 0x30},

  {0x034C, 0x05}, /*x_output_size = 1528*/
  {0x034D, 0xF8},
  {0x034E, 0x03}, /*y_output_size = 860*/
  {0x034F, 0x5C},

  {0x0200, 0x02}, /*fine integration time*/
  {0x0201, 0x50},
  {0x0202, 0x02}, /*Coarse integration time*/
  {0x0203, 0x5c},
  {0x0204, 0x00}, /*Analog gain*/
  {0x0205, 0x20},
  {0x0342, 0x0D}, /*Line_length_pck 3470d*/
  {0x0343, 0x8E},
#ifdef CONFIG_RAWCHIP
  /*Frame_length_lines 960d*/
  {0x0340, 0x03},
  {0x0341, 0xC0},
#else
  /*Frame_length_lines 876d*/
  {0x0340, 0x03},
  {0x0341, 0x6C},
#endif

  /*Manufacture Setting*/
  /*Address Data  Comment*/
  {0x300E, 0x2D},
  {0x31A3, 0x40},
  {0x301A, 0xA7},
  {0x3053, 0xCB}, /*CF for full ,CB for preview/HD/FHD/QVGA120fps*/
 #else
//100fps
{0x0305, 0x04},	//pre_pll_clk_div = 4
{0x0306, 0x00},	//pll_multiplier
{0x0307, 0x98},	//pll_multiplier  = 152
{0x0303, 0x01},	//vt_sys_clk_div = 1
{0x0301, 0x05},	//vt_pix_clk_div = 5
{0x030B, 0x01},	//op_sys_clk_div = 1
{0x0309, 0x05},	//op_pix_clk_div = 5
{0x30CC, 0xE0},	//DPHY_band_ctrl 870 MHz ~ 950 MHz
{0x31A1, 0x5A},	//"DBR_CLK = PLL_CLK / DIV_DBR(0x31A1[3:0]= 912Mhz / 10 = 91.2Mhz[7:4]

{0x0344, 0x00},	//X addr start 0d
{0x0345, 0x00},
{0x0346, 0x00},	//Y addr start 212d
{0x0347, 0xD4},
{0x0348, 0x0C},	//X addr end 3277d
{0x0349, 0xCD},
{0x034A, 0x08},	//Y addr end 2251d
{0x034B, 0xCB},

{0x0381, 0x01},	//x_even_inc = 1
{0x0383, 0x03},	//x_odd_inc = 3
{0x0385, 0x01},	//y_even_inc = 1
{0x0387, 0x07},	//y_odd_inc = 7

{0x0401, 0x00},	//Derating_en  = 0 (disable)
{0x0405, 0x10},
{0x0700, 0x05},	//fifo_water_mark_pixels = 1328
{0x0701, 0x30},

{0x034C, 0x06},	//x_output_size = 1640
{0x034D, 0x68},
{0x034E, 0x01},	//y_output_size = 510
{0x034F, 0xFE},

{0x0200, 0x02},	//fine integration time
{0x0201, 0x50},
{0x0202, 0x01},	//Coarse integration time
{0x0203, 0x39},
{0x0204, 0x00},	//Analog gain
{0x0205, 0x20},
{0x0342, 0x0D},	//Line_length_pck 3470d
{0x0343, 0x8E},
#ifdef CONFIG_RAWCHIP
  {0x0340, 0x02},	//Frame_length_lines 324d
  {0x0341, 0x22},
#else
  {0x0340, 0x02},	//Frame_length_lines 324d
  {0x0341, 0x0E},
#endif

{0x300E, 0x2D},
{0x31A3, 0x40},
{0x301A, 0xA7},
{0x3053, 0xCB}, //CF for full ,CB for preview/HD/FHD/QVGA120fps
#endif //ori
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_snap_settings[] = {
	/* PLL setting*/
	{0x0305, 0x04},/*pre_pll_clk_div = 4*/
	{0x0306, 0x00},/*pll_multiplier*/
	{0x0307, 0x98},/*pll_multiplier  = 152*/
	{0x0303, 0x01},/*vt_sys_clk_div = 1*/
	{0x0301, 0x05},/*vt_pix_clk_div = 5*/
	{0x030B, 0x01},/*op_sys_clk_div = 1*/
	{0x0309, 0x05},/*op_pix_clk_div = 5*/
	{0x30CC, 0xE0},/*DPHY_band_ctrl 870 MHz ~ 950 MHz*/
	{0x31A1, 0x5A},

	/*Timing configuration*/
	{0x0200, 0x02},/*FINE_INTEGRATION_TIME_*/
	{0x0201, 0x50},
	{0x0202, 0x04},/*COARSE_INTEGRATION_TIME*/
	{0x0203, 0xE7},
	{0x0204, 0x00},/*ANALOG_GAIN*/
	{0x0205, 0x20},
	{0x0342, 0x0D},/*LINE_LENGTH_PCK*/
	{0x0343, 0x8E},
#ifdef CONFIG_RAWCHIP
	{0x0340, 0x09},/*FRAME_LENGTH_LINES*/
	{0x0341, 0xC4},
#else
	{0x0340, 0x09},/*FRAME_LENGTH_LINES*/
	{0x0341, 0xC0},
#endif
	/*Output Size (3280x2464)*/
	{0x0344, 0x00},/*X_ADDR_START*/
	{0x0345, 0x00},
	{0x0346, 0x00},/*Y_ADDR_START*/
	{0x0347, 0x00},
	{0x0348, 0x0C},/*X_ADDR_END*/
	{0x0349, 0xCF},
	{0x034A, 0x09},/*Y_ADDR_END*/
	{0x034B, 0x9F},
	{0x0381, 0x01},/*X_EVEN_INC*/
	{0x0383, 0x01},/*X_ODD_INC*/
	{0x0385, 0x01},/*Y_EVEN_INC*/
	{0x0387, 0x01},/*Y_ODD_INC*/
	{0x0401, 0x00},/*DERATING_EN*/
	{0x0405, 0x10},
	{0x0700, 0x05},/*FIFO_WATER_MARK_PIXELS*/
	{0x0701, 0x30},
	{0x034C, 0x0C},/*X_OUTPUT_SIZE*/
	{0x034D, 0xD0},
	{0x034E, 0x09},/*Y_OUTPUT_SIZE*/
	{0x034F, 0xA0},

	/*Manufacture Setting*/
	{ 0x300E , 0x29 }, /* Reserved  For 912Mbps */
	{ 0x31A3 , 0x00 }, /* Reserved  For 912Mbps */
	{ 0x301A , 0x77 }, /* Reserved */
};


static struct msm_camera_i2c_reg_conf s5k3h2yx_snap_wide_settings[] = {

	{0x0305, 0x04},	//pre_pll_clk_div = 4
	{0x0306, 0x00},	//pll_multiplier
	{0x0307, 0x98},	//pll_multiplier  = 152
	{0x0303, 0x01},	//vt_sys_clk_div = 1
	{0x0301, 0x05},	//vt_pix_clk_div = 5
	{0x030B, 0x01},	//op_sys_clk_div = 1
	{0x0309, 0x05},	//op_pix_clk_div = 5
	{0x30CC, 0xE0},	//DPHY_band_ctrl 870 MHz ~ 950 MHz
	{0x31A1, 0x5A},	//"DBR_CLK = PLL_CLK / DIV_DBR(0x31A1[3:0])

	{0x0344, 0x00},	//X addr start 0d
	{0x0345, 0x00},
	{0x0346, 0x01},	//Y addr start 304d
	{0x0347, 0x30},
	{0x0348, 0x0C},	//X addr end 3279d
	{0x0349, 0xCF},
	{0x034A, 0x08},	//Y addr end 2159d
	{0x034B, 0x6F},

	{0x0381, 0x01},	//x_even_inc = 1
	{0x0383, 0x01},	//x_odd_inc = 1
	{0x0385, 0x01},	//y_even_inc = 1
	{0x0387, 0x01},	//y_odd_inc = 1

	{0x0401, 0x00},	//Derating_en  = 0 (disable)
	{0x0405, 0x10},
	{0x0700, 0x05},	//fifo_water_mark_pixels = 1328
	{0x0701, 0x30},

	{0x034C, 0x0C},	//x_output_size = 3280
	{0x034D, 0xD0},
	{0x034E, 0x07},	//y_output_size = 1856
	{0x034F, 0x40},

	{0x0200, 0x02},	//fine integration time
	{0x0201, 0x50},
	{0x0202, 0x04},	//Coarse integration time
	{0x0203, 0xDB},
	{0x0204, 0x00},	//Analog gain
	{0x0205, 0x20},
	{0x0342, 0x0D},	//Line_length_pck 3470d
	{0x0343, 0x8E},
#if 0
	{0x0340, 0x07},	//Frame_length_lines 1872d
	{0x0341, 0x50},
#else
	{0x0340, 0x09},	//Frame_length_lines 1872d
	{0x0341, 0x60},
#endif

	{0x300E, 0x29},
	{0x31A3, 0x00},
	{0x301A, 0xA7},
	{0x3053, 0xCB},	//CF for full/preview/ ,CB for HD/FHD/QVGA120fps
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_night_settings[] = {
  {0x0305, 0x04},	//pre_pll_clk_div = 4
  {0x0306, 0x00},	//pll_multiplier
  {0x0307, 0x98},	//pll_multiplier  = 152
  {0x0303, 0x01},	//vt_sys_clk_div = 1
  {0x0301, 0x05},	//vt_pix_clk_div = 5
  {0x030B, 0x01},	//op_sys_clk_div = 1
  {0x0309, 0x05},	//op_pix_clk_div = 5
  {0x30CC, 0xE0},	//DPHY_band_ctrl 870 MHz ~ 950 MHz
  {0x31A1, 0x5A},	//"DBR_CLK = PLL_CLK / DIV_DBR(s31A1[3:0]) = 912Mhz / 10 = 91.2Mhz[7:4] must be same as vt_pix_clk_div (s0301)"

  {0x0344, 0x00},	//X addr start 0d
  {0x0345, 0x00},	//
  {0x0346, 0x00},	//Y addr start 0d
  {0x0347, 0x00},	//
  {0x0348, 0x0C},	//X addr end 3279d
  {0x0349, 0xCD},	//
  {0x034A, 0x09},	//Y addr end 2463d
  {0x034B, 0x9F},	//

  {0x0381, 0x01},	//x_even_inc = 1
  {0x0383, 0x03},	//x_odd_inc = 3
  {0x0385, 0x01},	//y_even_inc = 1
  {0x0387, 0x03},	//y_odd_inc = 3

  {0x0401, 0x00},	//Derating_en  = 0 (disable)
  {0x0405, 0x10},	//
  {0x0700, 0x05},	//fifo_water_mark_pixels = 1328
  {0x0701, 0x30},	//

  {0x034C, 0x06},	//x_output_size = 1640
  {0x034D, 0x68},	//
  {0x034E, 0x04},	//y_output_size = 1232
  {0x034F, 0xD0},	//

  {0x0200, 0x02},	//fine integration time
  {0x0201, 0x50},	//
  {0x0202, 0x04},	//Coarse integration time
  {0x0203, 0xDB},	//
  {0x0204, 0x00},	//Analog gain
  {0x0205, 0x20},	//
  {0x0342, 0x0D},	//Line_length_pck 3470d
  {0x0343, 0x8E},	//
  {0x0340, 0x04},	//Frame_length_lines 1248d
  {0x0341, 0xE0},

#ifdef CONFIG_RAWCHIP
  {0x0340, 0x04},	//Frame_length_lines 1248d
  {0x0341, 0xF4},
#else
  {0x0340, 0x04},	//Frame_length_lines 1248d
  {0x0341, 0xE0},
#endif


  {0x300E, 0x2D},	//Hbinnning[2] : 1b enale / 0b disable
  {0x31A3, 0x40},	//Vbinning enable[6] : 1b enale / 0b disable
  {0x301A, 0xA7},	//"In case of using the Vt_Pix_Clk more than 137Mhz, sA7h should be adopted! "
  {0x3053, 0xCF},
};


static struct msm_camera_i2c_reg_conf s5k3h2yx_recommend_settings[] = {
	{0x3000, 0x08},
	{0x3001, 0x05},
	{0x3002, 0x0D},
	{0x3003, 0x21},
	{0x3004, 0x62},
	{0x3005, 0x0B},
	{0x3006, 0x6D},
	{0x3007, 0x02},
	{0x3008, 0x62},
	{0x3009, 0x62},
	{0x300A, 0x41},
	{0x300B, 0x10},
	{0x300C, 0x21},
	{0x300D, 0x04},
	{0x307E, 0x03},
	{0x307F, 0xA5},
	{0x3080, 0x04},
	{0x3081, 0x29},
	{0x3082, 0x03},
	{0x3083, 0x21},
	{0x3011, 0x5F},
	{0x3156, 0xE2},
	{0x3027, 0x0E},
	{0x300f, 0x02},
	{0x3010, 0x10},
	{0x3017, 0x74},
	{0x3018, 0x00},
	{0x3020, 0x02},
	{0x3021, 0x24},
	{0x3023, 0x80},
	{0x3024, 0x04},
	{0x3025, 0x08},
	{0x301C, 0xD4},
	{0x315D, 0x00},
	/*Manufacture Setting*/
	{0x300E, 0x29},
	{0x31A3, 0x00},
	{0x301A, 0xA7},
	{0x3053, 0xCF},
	{0x3054, 0x00},
	{0x3055, 0x35},
	{0x3062, 0x04},
	{0x3063, 0x38},
	{0x31A4, 0x04},
	{0x3016, 0x54},
	{0x3157, 0x02},
	{0x3158, 0x00},
	{0x315B, 0x02},
	{0x315C, 0x00},
	{0x301B, 0x05},
	{0x3028, 0x41},
	{0x302A, 0x00},
	{0x3060, 0x01},
	{0x302D, 0x19},
	{0x302B, 0x04},
	{0x3072, 0x13},
	{0x3073, 0x21},
	{0x3074, 0x82},
	{0x3075, 0x20},
	{0x3076, 0xA2},
	{0x3077, 0x02},
	{0x3078, 0x91},
	{0x3079, 0x91},
	{0x307A, 0x61},
	{0x307B, 0x28},
	{0x307C, 0x31},
};

static struct v4l2_subdev_info s5k3h2yx_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array s5k3h2yx_init_conf[] = {
	{&s5k3h2yx_mipi_settings[0],
	ARRAY_SIZE(s5k3h2yx_mipi_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_recommend_settings[0],
	ARRAY_SIZE(s5k3h2yx_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_pll_settings[0],
	ARRAY_SIZE(s5k3h2yx_pll_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_conf_array s5k3h2yx_confs[] = {
	{&s5k3h2yx_snap_settings[0],
	ARRAY_SIZE(s5k3h2yx_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_prev_settings[0],
	ARRAY_SIZE(s5k3h2yx_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_video_settings[0],
	ARRAY_SIZE(s5k3h2yx_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_fast_video_settings[0],
	ARRAY_SIZE(s5k3h2yx_fast_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_night_settings[0],
	ARRAY_SIZE(s5k3h2yx_night_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_snap_wide_settings[0],
	ARRAY_SIZE(s5k3h2yx_snap_wide_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t s5k3h2yx_dimensions[] = {
	{/*full size*/
		.x_output = 0xCD0,
		.y_output = 0x9A0,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x9C4,
#else
		.frame_length_lines = 0x9C0,
#endif
		.vt_pixel_clk = 182400000,
		.op_pixel_clk = 182400000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x99F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{/*Q size*/
		.x_output = 0x668,
		.y_output = 0x4D0,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x4F4,
#else
		.frame_length_lines = 0x4E0,
#endif
		.vt_pixel_clk = 129600000,
		.op_pixel_clk = 129600000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xCCD,
		.y_addr_end = 0x99F,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
	{/*video size*/
		.x_output = 0xC0C,
		.y_output = 0x6C8,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x6EC,
#else
		.frame_length_lines = 0x6D8,
#endif
		.vt_pixel_clk = 182400000,
		.op_pixel_clk = 182400000,
		.binning_factor = 1,
		.x_addr_start = 0x062,
		.y_addr_start = 0x16C,
		.x_addr_end = 0xC6D,
		.y_addr_end = 0x833,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{/*fast video size*/
#if 0
		.x_output = 0x5F8,
		.y_output = 0x35C,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x3C0,
#else
		.frame_length_lines = 0x36C,
#endif
		.vt_pixel_clk = 182400000,
		.op_pixel_clk = 182400000,
		.binning_factor = 1,
		.x_addr_start = 0x070,
		.y_addr_start = 0x174,
		.x_addr_end = 0xC5D,
		.y_addr_end = 0x82B,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
#else
//100 fps
		.x_output = 0x668,
		.y_output = 0x1FE,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x222,
#else
		.frame_length_lines = 0x20E,
#endif
		.vt_pixel_clk = 182400000,
		.op_pixel_clk = 182400000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0x0D4,
		.x_addr_end = 0xCCD,
		.y_addr_end = 0x8CB,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 7,
		.binning_rawchip = 0x22,
#endif //ori
	},
	{/*night mode size*/
		.x_output = 0x668,
		.y_output = 0x4D0,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x4F4,
#else
		.frame_length_lines = 0x4E0,
#endif
		.vt_pixel_clk = 182400000,
		.op_pixel_clk = 182400000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xCCD,
		.y_addr_end = 0x99F,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
	{/*wide full size*/
		.x_output = 0xCD0,
		.y_output = 0x740,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x960,  //0x764
#else
		.frame_length_lines = 0x960,  //0x750
#endif
		.vt_pixel_clk = 182400000,
		.op_pixel_clk = 182400000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x99F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
};

static struct msm_camera_csid_vc_cfg s5k3h2yx_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params s5k3h2yx_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 2,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = s5k3h2yx_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 2,
		.settle_cnt = 0x1B,
	},
};

static struct msm_camera_csi2_params *s5k3h2yx_csi_params_array[] = {
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params
};

static struct msm_sensor_output_reg_addr_t s5k3h2yx_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static struct msm_sensor_id_info_t s5k3h2yx_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x382B,
};

static struct msm_sensor_exp_gain_info_t s5k3h2yx_exp_gain_info = {
	.coarse_int_time_addr = 0x202,
	.global_gain_addr = 0x204,
	.vert_offset = 4,
	.min_vert = 4, /* min coarse integration time */ /* HTC Angie 20111019 - Fix FPS */
};

static uint32_t vcm_clib;
static uint16_t vcm_clib_min,vcm_clib_med,vcm_clib_max;

static void s5k3h2yx_read_vcm_clib(struct msm_sensor_ctrl_t *s_ctrl)
{
	uint8_t rc=0;
	unsigned short info_value = 0;

	struct msm_camera_i2c_client *s5k3h2yx_msm_camera_i2c_client = s_ctrl->sensor_i2c_client;

	vcm_clib =0;
	vcm_clib_min = 0;
	vcm_clib_med = 0;
	vcm_clib_max = 0;


	pr_info("%s: sensor OTP information:\n", __func__);

	/* testmode disable */
	rc = msm_camera_i2c_write_b(s5k3h2yx_msm_camera_i2c_client, 0x3A1C, 0x00);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x3A1C fail\n", __func__);

	/* Initialize */
	rc = msm_camera_i2c_write_b(s5k3h2yx_msm_camera_i2c_client, 0x0A00, 0x04);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x0A00 (Start) fail\n", __func__);

	mdelay(4);

	vcm_clib =0;
	rc = msm_camera_i2c_write_b(s5k3h2yx_msm_camera_i2c_client, 0x0A02, 5);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x0A02 (select page %d) fail\n", __func__, 5);
			/* Set Read Mode */
	rc = msm_camera_i2c_write_b(s5k3h2yx_msm_camera_i2c_client, 0x0A00, 0x01);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x0A00: Set read mode fail\n", __func__);

 	rc = msm_camera_i2c_read_b(s5k3h2yx_msm_camera_i2c_client, (0x0A04), &info_value);
	if (rc < 0)
		pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A04));
	else
		{
		pr_info("%s: i2c_read_b 0x%x\n", __func__, info_value);
		vcm_clib = (vcm_clib << 8) | info_value;
		}
 	rc = msm_camera_i2c_read_b(s5k3h2yx_msm_camera_i2c_client, (0x0A05), &info_value);
	if (rc < 0)
		pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A05));
	else
		{
		pr_info("%s: i2c_read_b 0x%x\n", __func__, info_value);
		vcm_clib = (vcm_clib << 8) | info_value;
		}

	//parsing into min/med/max
	if(vcm_clib >> 8 == 0x03)//SHARP
		{
		  uint32_t p;

		  rc = msm_camera_i2c_read_b(s5k3h2yx_msm_camera_i2c_client, (0x0A0C), &info_value);
		  if (rc < 0)
			  pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A0C));
		  else
			  {
			  pr_info("%s: i2c_read_b 0x%x\n", __func__, info_value);
			  vcm_clib = (vcm_clib << 8) | info_value;
			  }
		  rc = msm_camera_i2c_read_b(s5k3h2yx_msm_camera_i2c_client, (0x0A0D), &info_value);
		  if (rc < 0)
			  pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A0D));
		  else
			  {
			  pr_info("%s: i2c_read_b 0x%x\n", __func__, info_value);
			  vcm_clib = (vcm_clib << 8) | info_value;
			  }

		  p=((vcm_clib & 0x0000FFFF) ) >> 3 ;
		  vcm_clib_min= p - 20;
		  vcm_clib_max= p + 26;
		  vcm_clib_med= (vcm_clib_max + vcm_clib_min)/2 -26/4;
		  pr_info("%s: VCM clib=0x%x, [Sharp] (p=%d) min/med/max=%d %d %d\n"
			  , __func__, vcm_clib, p, vcm_clib_min, vcm_clib_med, vcm_clib_max);
		}
	else
		{
    		vcm_clib =0;
    		rc = msm_camera_i2c_write_b(s5k3h2yx_msm_camera_i2c_client, 0x0A02, 16);
    		if (rc < 0)
    			pr_err("%s: i2c_write_b 0x0A02 (select page %d) fail\n", __func__, 16);
    				/* Set Read Mode */
    		rc = msm_camera_i2c_write_b(s5k3h2yx_msm_camera_i2c_client, 0x0A00, 0x01);
    		if (rc < 0)
    			pr_err("%s: i2c_write_b 0x0A00: Set read mode fail\n", __func__);
    		
    		rc = msm_camera_i2c_read_b(s5k3h2yx_msm_camera_i2c_client, (0x0A04), &info_value);
    		if (rc < 0)
    			pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A04));
    		else
    			{
    			pr_info("%s: i2c_read_b 0x%x\n", __func__, info_value);
    			vcm_clib = (vcm_clib << 8) | info_value;
    			}
    		rc = msm_camera_i2c_read_b(s5k3h2yx_msm_camera_i2c_client, (0x0A05), &info_value);
    		if (rc < 0)
    			pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A05));
    		else
    			{
    			pr_info("%s: i2c_read_b 0x%x\n", __func__, info_value);
    			vcm_clib = (vcm_clib << 8) | info_value;
    			}
    
    		if(vcm_clib >> 8 == 0x04)//Lite-On
    		{
    		  uint32_t p;
    
    		  rc = msm_camera_i2c_read_b(s5k3h2yx_msm_camera_i2c_client, (0x0A0E), &info_value);
    		  if (rc < 0)
    			  pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A0E));
    		  else
    			  {
    			  pr_info("%s: i2c_read_b 0x%x\n", __func__, info_value);
    			  vcm_clib = (vcm_clib << 16) | info_value;
    			  }
    
    		  p=((vcm_clib & 0x000000FF) + 0x00000080) >> 3 ;
    		  vcm_clib_min= p - 20;
    		  vcm_clib_max= p + 26;
    		  vcm_clib_med= (vcm_clib_max + vcm_clib_min)/2 -26/4;
			  pr_info("%s: VCM clib=0x%x, [Lite-On] (p=%d) min/med/max=%d %d %d\n"
				  , __func__, vcm_clib, p, vcm_clib_min, vcm_clib_med, vcm_clib_max);
    		}
		}
	if(((vcm_clib & 0x0000FFFF) == 0x0000) || (vcm_clib_min==0 && vcm_clib_med==0 && vcm_clib_max==0)
		||(//protect vcm range within theoratical
		     (DEFAULT_VCM_MAX < vcm_clib_max) || (DEFAULT_VCM_MAX < vcm_clib_med) || (DEFAULT_VCM_MAX < vcm_clib_min)
		  || (DEFAULT_VCM_MIN > vcm_clib_max) || (DEFAULT_VCM_MIN > vcm_clib_med) || (DEFAULT_VCM_MIN > vcm_clib_min)
		  || ((vcm_clib_med < vcm_clib_min) || (vcm_clib_med > vcm_clib_max))
		))
		{
		  vcm_clib_min=DEFAULT_VCM_MIN;
		  vcm_clib_med=DEFAULT_VCM_MED;
		  vcm_clib_max=DEFAULT_VCM_MAX;
		}


	pr_info("%s: VCM clib=0x%x, min/med/max=%d %d %d\n"
		, __func__, vcm_clib, vcm_clib_min, vcm_clib_med, vcm_clib_max);

	return;

}


static int lens_info;	//	IR: 5;	BG: 6;

static int s5k3h2yx_read_lens_info(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t  rc;
	int page = 0;
	unsigned short info_value = 0, info_index = 0;
	unsigned short  OTP[10] = {0};
	struct msm_camera_i2c_client *s5k4e5yx_msm_camera_i2c_client = s_ctrl->sensor_i2c_client;

	lens_info = 6;	//default: BG

	pr_info("[CAM]%s\n", __func__);
	pr_info("%s: sensor OTP information:\n", __func__);

	/* testmode disable */
	rc = msm_camera_i2c_write_b(s5k4e5yx_msm_camera_i2c_client, 0x3A1C, 0x00);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x3A1C fail\n", __func__);

	/* Initialize */
	rc = msm_camera_i2c_write_b(s5k4e5yx_msm_camera_i2c_client, 0x0A00, 0x04);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x0A00 (Start) fail\n", __func__);

	mdelay(4);

	/*Read Page 20 to Page 16*/
	info_index = 1;
	info_value = 0;
	memset(OTP, 0, sizeof(OTP));

	for (page = 20; page >= 16; page--) {
		rc = msm_camera_i2c_write_b(s5k4e5yx_msm_camera_i2c_client, 0x0A02, page);
		if (rc < 0)
			pr_err("%s: i2c_write_b 0x0A02 (select page %d) fail\n", __func__, page);

		/* Set Read Mode */
		rc = msm_camera_i2c_write_b(s5k4e5yx_msm_camera_i2c_client, 0x0A00, 0x01);
		if (rc < 0)
			pr_err("%s: i2c_write_b 0x0A00: Set read mode fail\n", __func__);

		/* 0x0A04~0x0A0D: read Information 0~9 according to SPEC*/
		rc = msm_camera_i2c_read_b(s5k4e5yx_msm_camera_i2c_client, (0x0A04 + info_index), &info_value);
		if (rc < 0)
			pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A04 + info_index));

		 /* some values of fuseid are maybe zero */
		if (((info_value&0x0F) != 0) || page == 0)
			break;
	}
	OTP[info_index] = (short)(info_value&0x0F);

	if (OTP[1] != 0) {
		pr_info("Get Fuseid from Page20 to Page16\n");
		goto get_done;
	}

	/*Read Page 4 to Page 0*/
	info_index = 1;
	info_value = 0;
	memset(OTP, 0, sizeof(OTP));

	for (page = 4; page >= 0; page--) {
		rc = msm_camera_i2c_write_b(s5k4e5yx_msm_camera_i2c_client, 0x0A02, page);
		if (rc < 0)
			pr_err("%s: i2c_write_b 0x0A02 (select page %d) fail\n", __func__, page);

		/* Set Read Mode */
		rc = msm_camera_i2c_write_b(s5k4e5yx_msm_camera_i2c_client, 0x0A00, 0x01);
		if (rc < 0)
			pr_err("%s: i2c_write_b 0x0A00: Set read mode fail\n", __func__);

		/* 0x0A04~0x0A0D: read Information 0~9 according to SPEC*/
		rc = msm_camera_i2c_read_b(s5k4e5yx_msm_camera_i2c_client, (0x0A04 + info_index), &info_value);
		if (rc < 0)
			pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A04 + info_index));

		 /* some values of fuseid are maybe zero */
		if (((info_value & 0x0F) != 0) || page == 0)
			break;
	}
	OTP[info_index] = (short)(info_value&0x0F);

get_done:
	/* interface disable */
	rc = msm_camera_i2c_write_b(s5k4e5yx_msm_camera_i2c_client, 0x0A00, 0x00);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x0A00 (Stop) fail\n", __func__);

	pr_info("%s: LensID=%x\n", __func__, OTP[1]);

	if (OTP[1] == 5)	// IR
		lens_info = OTP[1];

	return lens_info;
}

static int s5k3h2yx_sensor_config(void __user *argp)
{
	return msm_sensor_config(&s5k3h2yx_s_ctrl, argp);
}

static int s5k3h2yx_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int rc;
	uint16_t value = 0;

	if (data->sensor_platform_info)
		s5k3h2yx_s_ctrl.mirror_flip = data->sensor_platform_info->mirror_flip;

	rc = msm_sensor_open_init(&s5k3h2yx_s_ctrl, data);
	if (rc < 0) {
		pr_err("%s failed to sensor open init\n", __func__);
		return rc;
	}

	/* Apply sensor mirror/flip */
	if (s5k3h2yx_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR_FLIP)
		value = S5K3H2YX_READ_MIRROR_FLIP;
	else if (s5k3h2yx_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR)
		value = S5K3H2YX_READ_MIRROR;
	else if (s5k3h2yx_s_ctrl.mirror_flip == CAMERA_SENSOR_FLIP)
		value = S5K3H2YX_READ_FLIP;
	else
		value = S5K3H2YX_READ_NORMAL_MODE;
	msm_camera_i2c_write(s5k3h2yx_s_ctrl.sensor_i2c_client,
		S5K3H2YX_REG_READ_MODE, value, MSM_CAMERA_I2C_BYTE_DATA);

	s5k3h2yx_read_lens_info(&s5k3h2yx_s_ctrl);
	s5k3h2yx_read_vcm_clib(&s5k3h2yx_s_ctrl);

	return rc;
}

static int s5k3h2yx_sensor_release(void)
{
	int	rc = 0;
	rc = msm_sensor_release(&s5k3h2yx_s_ctrl);
	return rc;
}

static const char *s5k3h2yxVendor = "samsung";
static const char *s5k3h2yxNAME = "s5k3h2yx";
static const char *s5k3h2yxSize = "8M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", s5k3h2yxVendor, s5k3h2yxNAME, s5k3h2yxSize);
	ret = strlen(buf) + 1;

	return ret;
}

static ssize_t lens_info_show(struct device *dev,
  struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%d\n", lens_info);
	ret = strlen(buf) + 1;

	return ret;
}

static ssize_t vcm_clib_show(struct device *dev,
  struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%d\n", vcm_clib);
	ret = strlen(buf) + 1;

	return ret;
}

static ssize_t vcm_clib_min_show(struct device *dev,
  struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%d\n", vcm_clib_min);
	ret = strlen(buf) + 1;

	return ret;
}

static ssize_t vcm_clib_med_show(struct device *dev,
  struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%d\n", vcm_clib_med);
	ret = strlen(buf) + 1;

	return ret;
}

static ssize_t vcm_clib_max_show(struct device *dev,
  struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%d\n", vcm_clib_max);
	ret = strlen(buf) + 1;

	return ret;
}
static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);
static DEVICE_ATTR(lensinfo, 0444, lens_info_show, NULL);
static DEVICE_ATTR(vcmclib, 0444, vcm_clib_show, NULL);
static DEVICE_ATTR(vcmclibmin, 0444, vcm_clib_min_show, NULL);
static DEVICE_ATTR(vcmclibmed, 0444, vcm_clib_med_show, NULL);
static DEVICE_ATTR(vcmclibmax, 0444, vcm_clib_max_show, NULL);

static struct kobject *android_s5k3h2yx;

static int s5k3h2yx_sysfs_init(void)
{
	int ret ;
	pr_info("s5k3h2yx:kobject creat and add\n");
	android_s5k3h2yx = kobject_create_and_add("android_camera", NULL);
	if (android_s5k3h2yx == NULL) {
		pr_info("s5k3h2yx_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("s5k3h2yx:sysfs_create_file\n");
	ret = sysfs_create_file(android_s5k3h2yx, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("s5k3h2yx_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_s5k3h2yx);
	}
	pr_info("s5k3h2yx:sysfs_create_file lensinfo\n");
	ret = sysfs_create_file(android_s5k3h2yx, &dev_attr_lensinfo.attr);
	if (ret) {
		pr_info("s5k3h2yx_sysfs_init: dev_attr_lensinfo failed\n");
		kobject_del(android_s5k3h2yx);
	}
	pr_info("s5k3h2yx:sysfs_create_file vcmclib\n");
	ret = sysfs_create_file(android_s5k3h2yx, &dev_attr_vcmclib.attr);
	if (ret) {
		pr_info("s5k3h2yx_sysfs_init: dev_attr_vcmclib failed\n");
		kobject_del(android_s5k3h2yx);
	}
	pr_info("s5k3h2yx:sysfs_create_file vcmclibmin\n");
	ret = sysfs_create_file(android_s5k3h2yx, &dev_attr_vcmclibmin.attr);
	if (ret) {
		pr_info("s5k3h2yx_sysfs_init: dev_attr_vcmclibmin failed\n");
		kobject_del(android_s5k3h2yx);
	}
	pr_info("s5k3h2yx:sysfs_create_file vcmclibmed\n");
	ret = sysfs_create_file(android_s5k3h2yx, &dev_attr_vcmclibmed.attr);
	if (ret) {
		pr_info("s5k3h2yx_sysfs_init: dev_attr_vcmclibmed failed\n");
		kobject_del(android_s5k3h2yx);
	}
	pr_info("s5k3h2yx:sysfs_create_file vcmclibmax\n");
	ret = sysfs_create_file(android_s5k3h2yx, &dev_attr_vcmclibmax.attr);
	if (ret) {
		pr_info("s5k3h2yx_sysfs_init: dev_attr_vcmclibmax failed\n");
		kobject_del(android_s5k3h2yx);
	}

	return 0 ;
}

static const struct i2c_device_id s5k3h2yx_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&s5k3h2yx_s_ctrl},
	{ }
};

static struct i2c_driver s5k3h2yx_i2c_driver = {
	.id_table = s5k3h2yx_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client s5k3h2yx_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int s5k3h2yx_sensor_v4l2_probe(const struct msm_camera_sensor_info *info,
	struct v4l2_subdev *sdev, struct msm_sensor_ctrl *s)
{
	int rc = -EINVAL;

	rc = msm_sensor_v4l2_probe(&s5k3h2yx_s_ctrl, info, sdev, s);

	return rc;
}

int32_t s5k3h2yx_power_up(const struct msm_camera_sensor_info *sdata)
{
	int rc;
	pr_info("%s\n", __func__);

	if (sdata->camera_power_on == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}

	if (!sdata->use_rawchip) {
		rc = msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
		if (rc < 0) {
			pr_err("%s: msm_camio_sensor_clk_on failed:%d\n",
			 __func__, rc);
			goto enable_mclk_failed;
		}
	}

	rc = sdata->camera_power_on();
	if (rc < 0) {
		pr_err("%s failed to enable power\n", __func__);
		goto enable_power_on_failed;
	}

	rc = msm_sensor_power_up(sdata);
	if (rc < 0) {
		pr_err("%s msm_sensor_power_up failed\n", __func__);
		goto enable_sensor_power_up_failed;
	}

	return rc;

enable_sensor_power_up_failed:
	if (sdata->camera_power_off == NULL)
		pr_err("sensor platform_data didnt register\n");
	else
		sdata->camera_power_off();
enable_power_on_failed:
	msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
enable_mclk_failed:
	return rc;
}

int32_t s5k3h2yx_power_down(const struct msm_camera_sensor_info *sdata)
{
	int rc;
	pr_info("%s\n", __func__);

	if (sdata->camera_power_off == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}

	rc = sdata->camera_power_off();
	if (rc < 0)
		pr_err("%s failed to disable power\n", __func__);

	rc = msm_sensor_power_down(sdata);
	if (rc < 0)
		pr_err("%s msm_sensor_power_down failed\n", __func__);

	if (!sdata->use_rawchip) {
		msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
		if (rc < 0)
			pr_err("%s: msm_camio_sensor_clk_off failed:%d\n",
				 __func__, rc);
	}

	return rc;
}

static int s5k3h2yx_probe(struct platform_device *pdev)
{
	int	rc = 0;

	pr_info("%s\n", __func__);

	rc = msm_sensor_register(pdev, s5k3h2yx_sensor_v4l2_probe);
	if(rc >= 0)
		s5k3h2yx_sysfs_init();
	return rc;
}

struct platform_driver s5k3h2yx_driver = {
	.probe = s5k3h2yx_probe,
	.driver = {
		.name = PLATFORM_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init msm_sensor_init_module(void)
{
	pr_info("%s\n", __func__);
	return platform_driver_register(&s5k3h2yx_driver);
}

static struct v4l2_subdev_core_ops s5k3h2yx_subdev_core_ops;
static struct v4l2_subdev_video_ops s5k3h2yx_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops s5k3h2yx_subdev_ops = {
	.core = &s5k3h2yx_subdev_core_ops,
	.video  = &s5k3h2yx_subdev_video_ops,
};

/*HTC_START*/
static int s5k3h2yx_read_fuseid(struct sensor_cfg_data *cdata,
	struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t  rc;
	int page = 0;
	unsigned short info_value = 0, info_index = 0;
	unsigned short  OTP[10] = {0};

	struct msm_camera_i2c_client *s5k4e5yx_msm_camera_i2c_client = s_ctrl->sensor_i2c_client;

	pr_info("%s: sensor OTP information:\n", __func__);

	/* testmode disable */
	rc = msm_camera_i2c_write_b(s5k4e5yx_msm_camera_i2c_client, 0x3A1C, 0x00);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x3A1C fail\n", __func__);

	/* Initialize */
	rc = msm_camera_i2c_write_b(s5k4e5yx_msm_camera_i2c_client, 0x0A00, 0x04);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x0A00 (Start) fail\n", __func__);

	mdelay(4);

	/*Read Page 20 to Page 16*/
	for (info_index = 0; info_index < 10; info_index++) {
		for (page = 20; page >= 16; page--) {
			rc = msm_camera_i2c_write_b(s5k4e5yx_msm_camera_i2c_client, 0x0A02, page);
			if (rc < 0)
				pr_err("%s: i2c_write_b 0x0A02 (select page %d) fail\n", __func__, page);

			/* Set Read Mode */
			rc = msm_camera_i2c_write_b(s5k4e5yx_msm_camera_i2c_client, 0x0A00, 0x01);
			if (rc < 0)
				pr_err("%s: i2c_write_b 0x0A00: Set read mode fail\n", __func__);

			/* 0x0A04~0x0A0D: read Information 0~9 according to SPEC*/
			rc = msm_camera_i2c_read_b(s5k4e5yx_msm_camera_i2c_client, (0x0A04 + info_index), &info_value);
			if (rc < 0)
				pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A04 + info_index));

			 /* some values of fuseid are maybe zero */
			if (((info_value&0x0F) != 0) || page == 0)
				break;
		}
		OTP[info_index] = (short)(info_value&0x0F);
		info_value = 0;
	}

	if (OTP[0] != 0 && OTP[1] != 0) {
		pr_info("Get Fuseid from Page20 to Page16\n");
		goto get_done;
	}

	/*Read Page 4 to Page 0*/
	memset(OTP, 0, sizeof(OTP));
	for (info_index = 0; info_index < 10; info_index++) {
		for (page = 4; page >= 0; page--) {
			rc = msm_camera_i2c_write_b(s5k4e5yx_msm_camera_i2c_client, 0x0A02, page);
			if (rc < 0)
				pr_err("%s: i2c_write_b 0x0A02 (select page %d) fail\n", __func__, page);

			/* Set Read Mode */
			rc = msm_camera_i2c_write_b(s5k4e5yx_msm_camera_i2c_client, 0x0A00, 0x01);
			if (rc < 0)
				pr_err("%s: i2c_write_b 0x0A00: Set read mode fail\n", __func__);

			/* 0x0A04~0x0A0D: read Information 0~9 according to SPEC*/
			rc = msm_camera_i2c_read_b(s5k4e5yx_msm_camera_i2c_client, (0x0A04 + info_index), &info_value);
			if (rc < 0)
				pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A04 + info_index));

			 /* some values of fuseid are maybe zero */
			if (((info_value & 0x0F) != 0) || page == 0)
				break;
		}
		OTP[info_index] = (short)(info_value&0x0F);
		info_value = 0;
	}

get_done:
	/* interface disable */
	rc = msm_camera_i2c_write_b(s5k4e5yx_msm_camera_i2c_client, 0x0A00, 0x00);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x0A00 (Stop) fail\n", __func__);

	pr_info("%s: VenderID=%x,LensID=%x,SensorID=%x%x\n", __func__,
		OTP[0], OTP[1], OTP[2], OTP[3]);
	pr_info("%s: ModuleFuseID= %x%x%x%x%x%x\n", __func__,
		OTP[4], OTP[5], OTP[6], OTP[7], OTP[8], OTP[9]);

    cdata->cfg.fuse.fuse_id_word1 = 0;
    cdata->cfg.fuse.fuse_id_word2 = 0;
	cdata->cfg.fuse.fuse_id_word3 = (OTP[0]);
	cdata->cfg.fuse.fuse_id_word4 =
		(OTP[4]<<20) |
		(OTP[5]<<16) |
		(OTP[6]<<12) |
		(OTP[7]<<8) |
		(OTP[8]<<4) |
		(OTP[9]);

	pr_info("s5k3h2yx: fuse->fuse_id_word1:%d\n",
		cdata->cfg.fuse.fuse_id_word1);
	pr_info("s5k3h2yx: fuse->fuse_id_word2:%d\n",
		cdata->cfg.fuse.fuse_id_word2);
	pr_info("s5k3h2yx: fuse->fuse_id_word3:0x%08x\n",
		cdata->cfg.fuse.fuse_id_word3);
	pr_info("s5k3h2yx: fuse->fuse_id_word4:0x%08x\n",
		cdata->cfg.fuse.fuse_id_word4);
	return 0;

}
/* HTC_END*/
static struct msm_sensor_fn_t s5k3h2yx_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_write_snapshot_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_setting = msm_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = s5k3h2yx_sensor_config,
	.sensor_open_init = s5k3h2yx_sensor_open_init,
	.sensor_release = s5k3h2yx_sensor_release,
	.sensor_power_up = s5k3h2yx_power_up,
	.sensor_power_down = s5k3h2yx_power_down,
	.sensor_probe = msm_sensor_probe,
	.sensor_read_lens_info = s5k3h2yx_read_lens_info,
	.sensor_i2c_read_fuseid = s5k3h2yx_read_fuseid,
};

static struct msm_sensor_reg_t s5k3h2yx_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = s5k3h2yx_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(s5k3h2yx_start_settings),
	.stop_stream_conf = s5k3h2yx_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(s5k3h2yx_stop_settings),
	.group_hold_on_conf = s5k3h2yx_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(s5k3h2yx_groupon_settings),
	.group_hold_off_conf = s5k3h2yx_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(s5k3h2yx_groupoff_settings),
	.init_settings = &s5k3h2yx_init_conf[0],
	.init_size = ARRAY_SIZE(s5k3h2yx_init_conf),
	.mode_settings = &s5k3h2yx_confs[0],
	.output_settings = &s5k3h2yx_dimensions[0],
	.num_conf = ARRAY_SIZE(s5k3h2yx_confs),
};

static struct msm_sensor_ctrl_t s5k3h2yx_s_ctrl = {
	.msm_sensor_reg = &s5k3h2yx_regs,
	.sensor_i2c_client = &s5k3h2yx_sensor_i2c_client,
	.sensor_i2c_addr = 0x20,
	.sensor_output_reg_addr = &s5k3h2yx_reg_addr,
	.sensor_id_info = &s5k3h2yx_id_info,
	.sensor_exp_gain_info = &s5k3h2yx_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &s5k3h2yx_csi_params_array[0],
	.msm_sensor_mutex = &s5k3h2yx_mut,
	.sensor_i2c_driver = &s5k3h2yx_i2c_driver,
	.sensor_v4l2_subdev_info = s5k3h2yx_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k3h2yx_subdev_info),
	.sensor_v4l2_subdev_ops = &s5k3h2yx_subdev_ops,
	.func_tbl = &s5k3h2yx_func_tbl,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Samsung 8 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
