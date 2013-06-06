/*HTC start Tom Lin 2011/12/19 Sony IMX105 8M driver */

#include "msm_sensor.h"

#ifdef CONFIG_RAWCHIP
#include "rawchip/rawchip.h"
#endif

#define SENSOR_NAME "imx105"
#define PLATFORM_DRIVER_NAME "msm_camera_imx105"
#define imx105_obj imx105_##obj

#define IMX105_REG_READ_MODE 0x0101
#define IMX105_READ_NORMAL_MODE 0x0000	/* without mirror/flip */
#define IMX105_READ_MIRROR 0x0001			/* with mirror */
#define IMX105_READ_FLIP 0x0002			/* with flip */
#define IMX105_READ_MIRROR_FLIP 0x0003/* with mirror/flip */

DEFINE_MUTEX(imx105_mut);
static struct msm_sensor_ctrl_t imx105_s_ctrl;

static struct msm_camera_i2c_reg_conf imx105_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf imx105_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf imx105_groupon_settings[] = {
	{0x104, 0x01},
};

static struct msm_camera_i2c_reg_conf imx105_groupoff_settings[] = {
	{0x104, 0x00},
};

static struct msm_camera_i2c_reg_conf imx105_mipi_settings[] = {
	/* Global Setting V4.1 From SONY */	
	{0x3031, 0x10},
	{0x3064, 0x12},
	{0x3087, 0x57},
	{0x308A, 0x35},
	{0x3091, 0x41},
	{0x3098, 0x03},
	{0x3099, 0xC0},
	{0x309A, 0xA3},
	{0x309D, 0x94},
	{0x30AB, 0x01},
	{0x30AD, 0x00},
	{0x30B1, 0x03},
	{0x30C4, 0x17},
	{0x30F3, 0x03},
	{0x3116, 0x31},
	{0x3117, 0x38},
	{0x3138, 0x28},
	{0x3137, 0x14},
	{0x3139, 0x2E},
	{0x314D, 0x2A},
	{0x328F, 0x01},
	{0x3343, 0x04},
};

static struct msm_camera_i2c_reg_conf imx105_pll_settings[] = {
	/* PLL Setting EXTCLK=24MHz, PLL=672MHz, 15fps */
	{0x0305, 0x01}, /*Pre_PLL_clk_div*/
	{0x0307, 0x1C}, /*PLL_multiplier */
	{0x303C, 0x4B},/*wait time for 24MHz MCLK*/
};


static struct msm_camera_i2c_reg_conf imx105_prev_settings[] = {
	/* PLL Setting EXTCLK=24MHz, PLL=672MHz, 15fps */
	{0x0305, 0x01}, /*Pre_PLL_clk_div*/
	{0x0307, 0x1C}, /*PLL_multiplier */
	{0x303C, 0x4B},/*wait time for 24MHz MCLK*/

	/* Global Setting V4.1 From SONY */	
	{0x3031, 0x10},/*change to 01 for continous mode and 10 for non continous mode*/
	{0x3064, 0x12},
	{0x3087, 0x57},
	{0x308A, 0x35},
	{0x3091, 0x41},
	{0x3098, 0x03},
	{0x3099, 0xC0},
	{0x309A, 0xA3},
	{0x309D, 0x94},
	{0x30AB, 0x01},
	{0x30AD, 0x00},
	{0x30B1, 0x03},
	{0x30C4, 0x17},
	{0x30F3, 0x03},
	{0x3116, 0x31},
	{0x3117, 0x38},
	{0x3138, 0x28},
	{0x3137, 0x14},
	{0x3139, 0x2E},
	{0x314D, 0x2A},
	{0x328F, 0x01},
	{0x3343, 0x04},		

	/*VCM Init*/
	{0x3408, 0x02}, 
	{0x340A, 0x01}, 
	{0x340C, 0x01},
	{0x3081, 0x48},
	{0x3400, 0x01}, /*[0]Damping control 0:Off, 1:On, [1]Independent standby control 0:Operation, 1:Standby*/
	{0x3401, 0x0F},
	{0x3404, 0x17}, 
	{0x3405, 0x00}, 

	/*Preview mode*/
	{0x0340, 0x04}, /*frame_length_lines_hi*/
	{0x0341, 0xF2}, /*frame_length_lines_lo*/
	{0x0342, 0x0D}, /*line_length_pclk_hi*/
	{0x0343, 0xD0}, /*line_length_pclk_lo*/
	{0x0346, 0x00}, /*Y_addr_start_hi*/
	{0x0347, 0x24}, /*Y_addr_start_lo*/
	{0x034a, 0x09}, /*y_add_end_hi*/
	{0x034b, 0xC3}, /*y_add_end_lo*/
	{0x034c, 0x06}, /*x_output_size_msb*/
	{0x034d, 0x68}, /*x_output_size_lsb*/
	{0x034e, 0x04}, /*y_output_size_msb*/
	{0x034f, 0xD0}, /*y_output_size_lsb*/
	{0x0381, 0x01}, /*x_even_inc*/
	{0x0383, 0x03}, /*x_odd_inc*/
	{0x0385, 0x01}, /*y_even_inc*/
	{0x0387, 0x03}, /*y_odd_inc*/
	{0x3033, 0x00}, /*HD mode operation setting Default:0x00*/
	{0x3048, 0x01}, /*0x3048[0]:VMODEFDS, 0x3048[1]:VMODEADD, 0x3048[5:7]:VMODEADDJMP, Default:0x00*/
	{0x304C, 0x6F}, /*Pixel readout count number 0x304C:HCNTHALF[0:7], 0x304D:HCNTHALF[8:10], Default:0x304C=0x6F, 0x304D=0x03*/
	{0x304D, 0x03},
	{0x306A, 0xD2}, 
	{0x309B, 0x28}, 
	{0x309C, 0x34}, 
	{0x309E, 0x00}, 
	{0x30AA, 0x03}, 
	{0x30D5, 0x09},
	{0x30D6, 0x01}, 
	{0x30D7, 0x01}, 
	{0x30DE, 0x02},
	{0x3102, 0x08},
	{0x3103, 0x22},
	{0x3104, 0x20},
	{0x3105, 0x00},
	{0x3106, 0x87},
	{0x3107, 0x00},
	{0x315C, 0xA5},
	{0x315D, 0xA4},
	{0x316E, 0xA6},
	{0x316F, 0xA5},
	{0x3318, 0x72},
	{0x0202, 0x04}, /*0x0202-0203, 1 <= Coarse_Integration_time <= frame_length_lines -5*/
	{0x0203, 0xED}
};

static struct msm_camera_i2c_reg_conf imx105_video_settings[] = {

	/* PLL Setting EXTCLK=24MHz, PLL=876MHz */
	{0x0305, 0x04}, /*Pre_PLL_clk_div*/
	{0x0307, 0x92}, /*PLL_multiplier */
	{0x30A4, 0x02}, /*Post_PLL_clk_div*/
	{0x303C, 0x4B},/*wait time for 24MHz MCLK*/

	/* Global Setting V4.1 From SONY */
	{0x3031, 0x10}, /*change to 01 for continous mode and 10 for non continous mode*/
	{0x3064, 0x12},
	{0x3087, 0x57},
	{0x308A, 0x35},
	{0x3091, 0x41},
	{0x3098, 0x03},
	{0x3099, 0xC0},
	{0x309A, 0xA3},
	{0x309D, 0x94},
	{0x30AB, 0x01},
	{0x30AD, 0x08},
	{0x30B1, 0x03},
	{0x30C4, 0x13},
	{0x30F3, 0x03},
	{0x3116, 0x31},
	{0x3117, 0x38},
	{0x3138, 0x28},
	{0x3137, 0x14},
	{0x3139, 0x2E},
	{0x314D, 0x2A},
	{0x3343, 0x04},

	{0x0340, 0x06}, /*frame_length_lines_hi*/
	{0x0341, 0xDE}, /*frame_length_lines_lo*/
	{0x0342, 0x0D}, /*line_length_pclk_hi*/
	{0x0343, 0x48}, /*line_length_pclk_lo*/
	{0x0344, 0x00}, /*0x0344-0345 x_address_Start 0x0066=102*/
	{0x0345, 0x66},  /*x_addr_start*/
	{0x0346, 0x01}, /*0x0346-0347 Y_address_Start 0x0190=400*/
	{0x0347, 0x90}, /*y_addr_start*/
	{0x0348, 0x0C},/*0x0348-0349 x_address_End 0x0c71=3185*/
	{0x0349, 0x71}, /*x_add_end*/
	{0x034A, 0x08}, /*0x034A-034B Y_address_End 0x0857=2135*/
	{0x034B, 0x57}, /*y_add_end*/
	{0x034C, 0x0C}, /*x_output_size_msb*/
	{0x034D, 0x0C}, /*x_output_size_lsb*/
	{0x034E, 0x06}, /*y_output_size_msb*/
	{0x034F, 0xC8}, /*y_output_size_lsb*/
	{0x0381, 0x01}, /*x_even_inc*/
	{0x0383, 0x01}, /*x_odd_inc*/
	{0x0385, 0x01}, /*y_even_inc*/
	{0x0387, 0x01}, /*y_odd_inc*/
	{0x3033, 0x00},
	{0x303D, 0x70},
	{0x303E, 0x41},
	{0x3040, 0x08},
	{0x3041, 0x89},
	{0x3048, 0x00},
	{0x304C, 0x50},
	{0x304D, 0x03},
	{0x306A, 0xD2},
	{0x309B, 0x00},
	{0x309C, 0x34},
	{0x309E, 0x00},
	{0x30AA, 0x02},
	{0x30D5, 0x00},
	{0x30D6, 0x85},
	{0x30D7, 0x2A},
	{0x30D8, 0x64},
	{0x30D9, 0x89},
	{0x30DE, 0x00},
	{0x3102, 0x10},
	{0x3103, 0x44},
	{0x3104, 0x40},
	{0x3105, 0x00},
	{0x3106, 0x10},
	{0x3107, 0x01},
	{0x315C, 0x76},
	{0x315D, 0x75},
	{0x316E, 0x77},
	{0x316F, 0x76},
	{0x3301, 0x00},
	{0x3318, 0x61},
	{0x0202, 0x04}, /*0x0202-0203, 1 <= Coarse_Integration_time <= frame_length_lines -5*/
	{0x0203, 0xED}
};


static struct msm_camera_i2c_reg_conf imx105_fast_video_settings[] = {

	/* 1640x510 > 100 fps*/
	/* PLL Setting EXTCLK=24MHz, PLL=672MHz */
	{0x0305, 0x04}, /*Pre_PLL_clk_div*/
	{0x0307, 0x70}, /*PLL_multiplier */
	{0x30A4, 0x02}, /*Post_PLL_clk_div*/
	{0x303C, 0x4B},/*wait time for 24MHz MCLK*/

	/* Global Setting V4.1 From SONY */
	{0x3031, 0x10}, /*change to 01 for continous mode and 10 for non continous mode*/
	{0x3064, 0x12},
	{0x3087, 0x57},
	{0x308A, 0x35},
	{0x3091, 0x41},
	{0x3098, 0x03},
	{0x3099, 0xC0},
	{0x309A, 0xA3},
	{0x309D, 0x94},
	{0x30AB, 0x01},
	{0x30AD, 0x08},
	{0x30B1, 0x03},
	{0x30C4, 0x13},
	{0x30F3, 0x03},
	{0x3116, 0x31},
	{0x3117, 0x38},
	{0x3138, 0x28},
	{0x3137, 0x14},
	{0x3139, 0x2E},
	{0x314D, 0x2A},
	{0x3343, 0x04},

	{0x0340, 0x02}, /*0x0340-0341 Frame_length_lines*/
	{0x0341, 0x78},
	{0x0342, 0x06}, /*0x0342-0343 Line_length_pck*/
	{0x0343, 0xE8},
	{0x0344, 0x00},/*x_addr_start*/
	{0x0345, 0x04},
	{0x0346, 0x00},/*Y_addr_Start*/
	{0x0347, 0xF8},
	{0x0348, 0x0C},/*0x0348-0349 x_address_end*/
	{0x0349, 0xD3},
	{0x034A, 0x08}, /*0x034A-034B y_address_end*/
	{0x034B, 0xEF},
	{0x034C, 0x06}, /*0x034C-034D x_output_size 0x0668*/
	{0x034D, 0x68},
	{0x034E, 0x01}, /*0x034E-034F y_output_size 0x01FE*/
	{0x034F, 0xFE},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x05},
	{0x0387, 0x03},
	{0x3033, 0x84},
	{0x303D, 0x70},
	{0x303E, 0x40},
	{0x3040, 0x08},
	{0x3041, 0x97},
	{0x3048, 0x01},
	{0x304C, 0xB7},
	{0x304D, 0x01},
	{0x306A, 0xD2},
	{0x309B, 0x08},
	{0x309C, 0x33},
	{0x309E, 0x04},
	{0x30AA, 0x00},
	{0x30D5, 0x04},
	{0x30D6, 0x85},
	{0x30D7, 0x2A},
	{0x30D8, 0x64},
	{0x30D9, 0x89},
	{0x30DE, 0x00},
	{0x3102, 0x05},
	{0x3103, 0x12},
	{0x3104, 0x12},
	{0x3105, 0x00},
	{0x3106, 0x46},
	{0x3107, 0x00},
	{0x315C, 0x4A},
	{0x315D, 0x49},
	{0x316E, 0x4B},
	{0x316F, 0x4A},
	{0x3301, 0x00},
	{0x3318, 0x62},
	{0x0202, 0x04}, /*0x0202-0203, 1 <= Coarse_Integration_time <= frame_length_lines -5*/
	{0x0203, 0xED}
};


static struct msm_camera_i2c_reg_conf imx105_snap_settings[] = {
	/* PLL Setting EXTCLK=24MHz, PLL=876MHz */
	{0x0305, 0x04}, /*Pre_PLL_clk_div*/
	{0x0307, 0x92}, /*PLL_multiplier */
	{0x30A4, 0x02}, /*Post_PLL_clk_div*/
	{0x303C, 0x4B},/*wait time for 24MHz MCLK*/

	/* Global Setting V4.1 From SONY */
	{0x3031, 0x10}, /*change to 01 for continous mode and 10 for non continous mode*/
	{0x3064, 0x12},
	{0x3087, 0x57},
	{0x308A, 0x35},
	{0x3091, 0x41},
	{0x3098, 0x03},
	{0x3099, 0xC0},
	{0x309A, 0xA3},
	{0x309D, 0x94},
	{0x30AB, 0x01},
	{0x30AD, 0x08},
	{0x30B1, 0x03},
	{0x30C4, 0x13},
	{0x30F3, 0x03},
	{0x3116, 0x31},
	{0x3117, 0x38},
	{0x3138, 0x28},
	{0x3137, 0x14},
	{0x3139, 0x2E},
	{0x314D, 0x2A},
	{0x3343, 0x04},

	{0x0340, 0x09}, /*frame_length_lines_hi*/
	{0x0341, 0xE6}, /*frame_length_lines_lo*/
	{0x0342, 0x0D}, /*line_length_pclk_hi*/
	{0x0343, 0xD0}, /*line_length_pclk_lo*/
	{0x0344, 0x00},
	{0x0345, 0x04},
	{0x0346, 0x00},
	{0x0347, 0x24},
	{0x0348, 0x0C},
	{0x0349, 0xD3},
	{0x034A, 0x09},
	{0x034B, 0xC3},
	{0x034C, 0x0C}, /*0x034C-034D X_output_size 0x0CD0=3280*/
	{0x034D, 0xD0},
	{0x034E, 0x09}, /*0x034E-034F Y_output_size 0x09A0=2464*/
	{0x034F, 0xA0},
	{0x0381, 0x01}, /*x_even_inc*/
	{0x0383, 0x01}, /*x_odd_inc*/
	{0x0385, 0x01}, /*y_even_inc*/
	{0x0387, 0x01}, /*y_odd_inc*/
	{0x3033, 0x00},
	{0x303D, 0x70},
	{0x303E, 0x41},
	{0x3040, 0x08},
	{0x3041, 0x89},
	{0x3048, 0x00},
	{0x304C, 0x50},
	{0x304D, 0x03},
	{0x306A, 0xD2},
	{0x309B, 0x00},
	{0x309C, 0x34},
	{0x309E, 0x00},
	{0x30AA, 0x02},
	{0x30D5, 0x00},
	{0x30D6, 0x85},
	{0x30D7, 0x2A},
	{0x30D8, 0x64},
	{0x30D9, 0x89},
	{0x30DE, 0x00},
	{0x3102, 0x10},
	{0x3103, 0x44},
	{0x3104, 0x40},
	{0x3105, 0x00},
	{0x3106, 0x10},
	{0x3107, 0x01},
	{0x315C, 0x76},
	{0x315D, 0x75},
	{0x316E, 0x77},
	{0x316F, 0x76},
	{0x3301, 0x00},
	{0x3318, 0x61},
	{0x0202, 0x04}, /*0x0202-0203, 1 <= Coarse_Integration_time <= frame_length_lines -5*/
	{0x0203, 0xED},
};

static struct msm_camera_i2c_reg_conf imx105_recommend_settings[] = {
	/* PLL Setting EXTCLK=24MHz, PLL=672MHz, 15fps */
	{0x0305, 0x01}, /*Pre_PLL_clk_div*/
	{0x0307, 0x1C}, /*PLL_multiplier */
	{0x303C, 0x4B},/*wait time for 24MHz MCLK*/

	/* Global Setting V4.1 From SONY */	
	{0x3031, 0x10}, /*change to 01 for continous mode and 10 for non continous mode*/
	{0x3064, 0x12},
	{0x3087, 0x57},
	{0x308A, 0x35},
	{0x3091, 0x41},
	{0x3098, 0x03},
	{0x3099, 0xC0},
	{0x309A, 0xA3},
	{0x309D, 0x94},
	{0x30AB, 0x01},
	{0x30AD, 0x00},
	{0x30B1, 0x03},
	{0x30C4, 0x17},
	{0x30F3, 0x03},
	{0x3116, 0x31},
	{0x3117, 0x38},
	{0x3138, 0x28},
	{0x3137, 0x14},
	{0x3139, 0x2E},
	{0x314D, 0x2A},
	{0x328F, 0x01},
	{0x3343, 0x04},

	/*VCM Init*/
	{0x3408, 0x02}, 
	{0x340A, 0x01},
	{0x340C, 0x01},
	{0x3081, 0x48},
	{0x3400, 0x01}, 
	{0x3401, 0x0F},
	{0x3404, 0x17}, 
	{0x3405, 0x00},  

	/* Black level Setting */
	{0x3032, 0x40}
};

static struct v4l2_subdev_info imx105_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array imx105_init_conf[] = {
	{&imx105_mipi_settings[0],
		ARRAY_SIZE(imx105_mipi_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx105_recommend_settings[0],
		ARRAY_SIZE(imx105_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx105_pll_settings[0],
		ARRAY_SIZE(imx105_pll_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},	
};

static struct msm_camera_i2c_conf_array imx105_confs[] = {
	{&imx105_snap_settings[0],
		ARRAY_SIZE(imx105_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx105_prev_settings[0],
		ARRAY_SIZE(imx105_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx105_video_settings[0],
		ARRAY_SIZE(imx105_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx105_fast_video_settings[0],
		ARRAY_SIZE(imx105_fast_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},	
};

static struct msm_sensor_output_info_t imx105_dimensions[] = {
	{/*full size 3280 x 2464 at 20 fps*/
		.x_output = 0xCD0, 
		.y_output = 0x9A0, 
		.line_length_pclk =  0xDD0,
		.frame_length_lines = 0x9E6,
		.vt_pixel_clk = 175200000,/*24Mhzx1(PreDivider)x146(PLL)/4(PostDivider)=Pixel CLK /10(vt_pixel_clk_divider)* 1(vt_sys_clk_div) = 87.6MHZx2(lanes)*/
		.op_pixel_clk = 175200000,
		.binning_factor = 1,
	},
	{/*Q size 1640 x 1232 at 30 fps*/
		.x_output = 0x668, 
		.y_output = 0x4D0,
		.line_length_pclk = 0xDD0,
		.frame_length_lines = 0x4F2,
		.vt_pixel_clk = 134400000,
		.op_pixel_clk = 134400000,
		.binning_factor = 1,
	},
	{/*video size 3084x1736 at 30 fps*/
		.x_output = 0xC0C,//3
		.y_output = 0x6C8,
		.line_length_pclk = 0xD48,
		.frame_length_lines = 0x6DE, /* Tom 20120215 changed to fix video frame not in sync issue*/
		.vt_pixel_clk = 175200000,
		.op_pixel_clk = 175200000,
		.binning_factor = 1,
	},		
	{/*video size 1640 X 510 at 100 fps*/
		.x_output = 0x668,
		.y_output = 0x1FE,
		.line_length_pclk = 0x6E8,
		.frame_length_lines = 0x278,
		.vt_pixel_clk = 134400000,
		.op_pixel_clk = 134400000,
		.binning_factor = 1,
	},
};

static struct msm_camera_csid_vc_cfg imx105_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params imx105_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 2,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = imx105_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 2,
		/*.settle_cnt = 0x1B,*/
		.settle_cnt = 0x15,/* Tom 20120224 changed for 876 MHz*/
	},
};

static struct msm_camera_csi2_params *imx105_csi_params_array[] = {
	&imx105_csi_params,
	&imx105_csi_params,
	&imx105_csi_params,
	&imx105_csi_params
};

static struct msm_sensor_output_reg_addr_t imx105_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static struct msm_sensor_id_info_t imx105_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x0105,
};

static struct msm_sensor_exp_gain_info_t imx105_exp_gain_info = {
	.coarse_int_time_addr = 0x202,
	.global_gain_addr = 0x204,
	.vert_offset = 5, /* sony imx105 sensor limitation */ /* HTC Tom 20120210 - Fix black screen */
	.min_vert = 4, /* min coarse integration time */ /* HTC Angie 20111019 - Fix FPS */	
};

static int imx105_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;
	uint16_t value = 0;

	pr_info("%s\n", __func__);

	if (data->sensor_platform_info)
		imx105_s_ctrl.mirror_flip = data->sensor_platform_info->mirror_flip;

	/* Apply sensor mirror/flip */
	if (imx105_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR_FLIP)
		value = IMX105_READ_MIRROR_FLIP;
	else if (imx105_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR)
		value = IMX105_READ_MIRROR;
	else if (imx105_s_ctrl.mirror_flip == CAMERA_SENSOR_FLIP)
		value = IMX105_READ_FLIP;
	else	
		value = IMX105_READ_NORMAL_MODE;

	pr_info("[CAM]:imx105 %s :IMX105_READ Value = %d\n", __func__,value);


	msm_camera_i2c_write(imx105_s_ctrl.sensor_i2c_client,
			IMX105_REG_READ_MODE, value, MSM_CAMERA_I2C_BYTE_DATA);

	return rc;
}

static const char *imx105Vendor = "sony";
static const char *imx105NAME = "imx105";
static const char *imx105Size = "8M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", imx105Vendor, imx105NAME, imx105Size);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);

static struct kobject *android_imx105;

static int imx105_sysfs_init(void)
{
	int ret ;
	pr_info("imx105:kobject creat and add\n");
	android_imx105 = kobject_create_and_add("android_camera", NULL);
	if (android_imx105 == NULL) {
		pr_info("imx105_sysfs_init: subsystem_register " \
				"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("imx105:sysfs_create_file\n");
	ret = sysfs_create_file(android_imx105, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("imx105_sysfs_init: sysfs_create_file " \
				"failed\n");
		kobject_del(android_imx105);
	}

	return 0 ;
}

int32_t imx105_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int	rc = 0;
	pr_info("%s\n", __func__);
	rc = msm_sensor_i2c_probe(client, id);
	if(rc >= 0)
		imx105_sysfs_init();
	pr_info("%s: rc(%d)\n", __func__, rc);
	return rc;
}

static const struct i2c_device_id imx105_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&imx105_s_ctrl},
	{ }
};

static struct i2c_driver imx105_i2c_driver = {
	.id_table = imx105_i2c_id,
	.probe  = imx105_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client imx105_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

/*Tom start 20120215 to fix AF issue by using sensor i2c driver client to write next lens position*/

int32_t msm_camera_i2c_write_lens_position(int16_t lens_position)
{
	struct msm_sensor_ctrl_t *s_ctrl = &imx105_s_ctrl;
	int rc = 0;

	pr_info("%s lens_position %d\n", __func__, lens_position);


	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);

	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			0x3403,
			((lens_position & 0x0300) >> 8),
			MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s VCM_CODE_MSB i2c write failed (%d)\n", __func__, rc);
		return rc;
	}

	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			0x3402,
			(lens_position & 0x00FF),
			MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s VCM_CODE_LSB i2c write failed (%d)\n", __func__, rc);
		return rc;
	}

	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);

	return rc;
}

int32_t imx105_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc;
	struct msm_camera_sensor_info *sdata = NULL;
	pr_info("%s\n", __func__);

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_info("%s: failed to s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	if (sdata->camera_power_on == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}

	rc = sdata->camera_power_on();
	if (rc < 0) {
		pr_err("%s failed to enable power\n", __func__);
		goto enable_power_on_failed;
	}

	if (!sdata->use_rawchip) {
		rc = msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
		if (rc < 0) {
			pr_info("%s: msm_camio_sensor_clk_on failed:%d\n",
					__func__, rc);
			goto enable_mclk_failed;
		}
	}

	rc = msm_sensor_set_power_up(s_ctrl);
	if (rc < 0) {
		pr_info("%s msm_sensor_power_up failed\n", __func__);
		goto enable_sensor_power_up_failed;
	}

	imx105_sensor_open_init(sdata);
	pr_info("%s end\n", __func__);

	return rc;

enable_sensor_power_up_failed:
	if (sdata->camera_power_off == NULL)
		pr_info("%s: failed to sensor platform_data didnt register\n", __func__);
	else
		sdata->camera_power_off();
enable_power_on_failed:
	msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
enable_mclk_failed:
	return rc;
}

int32_t imx105_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc;
	struct msm_camera_sensor_info *sdata = NULL;
	pr_info("%s\n", __func__);

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_info("%s: failed to s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	if (sdata->camera_power_off == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}

	rc = sdata->camera_power_off();
	if (rc < 0) {
		pr_err("%s failed to disable power\n", __func__);
		return rc;
	}

	mdelay(1);
	if (!sdata->use_rawchip) {
		rc = msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
		if (rc < 0)
			pr_info("%s: msm_camio_sensor_clk_off failed:%d\n",
					__func__, rc);
	}
	mdelay(1);

	rc = msm_sensor_set_power_down(s_ctrl);
	if (rc < 0)
		pr_info("%s: msm_sensor_power_down failed\n", __func__);
	mdelay(1);

	return rc;
}

static int __init msm_sensor_init_module(void)
{
	pr_info("[CAM]:imx105 %s\n", __func__);
	return i2c_add_driver(&imx105_i2c_driver);
}

static struct v4l2_subdev_core_ops imx105_subdev_core_ops;
static struct v4l2_subdev_video_ops imx105_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops imx105_subdev_ops = {
	.core = &imx105_subdev_core_ops,
	.video  = &imx105_subdev_video_ops,
};

/*HTC_START*/
static int imx105_read_fuseid(struct sensor_cfg_data *cdata,
		struct msm_sensor_ctrl_t *s_ctrl)
{


	int32_t  rc;
	uint16_t info_value = 0;
	unsigned short info_index = 0;
	unsigned short  OTP[10] = {0};

	struct msm_camera_i2c_client *imx105_msm_camera_i2c_client = s_ctrl->sensor_i2c_client;

	pr_info("%s: sensor OTP information: E \n", __func__);


	for (info_index = 0; info_index < 10; info_index++) {
		rc = msm_camera_i2c_write_b(imx105_msm_camera_i2c_client ,0x34C9, info_index);
		if (rc < 0)
			pr_err("[CAM]%s: i2c_write_b 0x34C9 (select info_index %d) fail\n", __func__, info_index);

		/*HTC start Tom change back to read word*/
		/* read Information 0~9 according to SPEC*/
		rc = msm_camera_i2c_read(imx105_msm_camera_i2c_client,0x3500, &info_value,2);
		if (rc < 0)
			pr_err("[CAM]%s: i2c_read 0x3500 fail\n", __func__);

		OTP[info_index] = (unsigned short)((info_value & 0xFF00) >>8);
		info_index++;
		OTP[info_index] = (unsigned short)(info_value & 0x00FF);
		info_value = 0;
		/*HTC end Tom change back to read word*/
	}

	pr_info("[CAM]%s: VenderID=%x,LensID=%x,SensorID=%x%x\n", __func__,
			OTP[0], OTP[1], OTP[2], OTP[3]);
	pr_info("[CAM]%s: ModuleFuseID= %x%x%x%x%x%x\n", __func__,
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
	/*
	   pr_info("[CAM]imx105: fuse->fuse_id_word1:%d\n",
	   cdata->cfg.fuse.fuse_id_word1);
	   pr_info("[CAM]imx105: fuse->fuse_id_word2:%d\n",
	   cdata->cfg.fuse.fuse_id_word2);
	   pr_info("[CAM]imx105: fuse->fuse_id_word3:0x%08x\n",
	   cdata->cfg.fuse.fuse_id_word3);
	   pr_info("[CAM]imx105: fuse->fuse_id_word4:0x%08x\n",
	   cdata->cfg.fuse.fuse_id_word4);
	   */		
	pr_info("%s: sensor OTP information: X \n", __func__);


	return 0;
}

/* HTC_END*/

int32_t imx105_sensor_write_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line) /* HTC Angie 20111019 - Fix FPS */
{
	uint32_t fl_lines;
	uint8_t offset;
	/* HTC_START Angie 20111019 - Fix FPS */
	uint32_t fps_divider = Q10;

	if (s_ctrl->cam_mode == SENSOR_PREVIEW_MODE)
		fps_divider = s_ctrl->fps_divider;
	fl_lines = s_ctrl->curr_frame_length_lines;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line * Q10 > (fl_lines - offset) * fps_divider)
		fl_lines = line + offset;
	else
		fl_lines = (fl_lines * fps_divider) / Q10;
	/* HTC_END */

	/*HTC start Tom 20120209  - fix black screen caused by line count > frame length lines -5*/
	if (line > fl_lines -offset)
		line = fl_lines -offset;
	/*HTC end*/

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_output_reg_addr->frame_length_lines, fl_lines,
			MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, line,
			MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->global_gain_addr, gain,
			MSM_CAMERA_I2C_WORD_DATA);
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);

	pr_info("%s write fl_lines : %d ; write line_cnt : %d ; write gain : %d \n", __func__, fl_lines,  line, gain);

	return 0;
}

static struct msm_sensor_fn_t imx105_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = imx105_sensor_write_exp_gain,
	.sensor_write_snapshot_exp_gain = imx105_sensor_write_exp_gain,
	.sensor_setting = msm_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = imx105_power_up,
	.sensor_power_down = imx105_power_down,
	.sensor_i2c_read_fuseid = imx105_read_fuseid,
};

static struct msm_sensor_reg_t imx105_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = imx105_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(imx105_start_settings),
	.stop_stream_conf = imx105_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(imx105_stop_settings),
	.group_hold_on_conf = imx105_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(imx105_groupon_settings),
	.group_hold_off_conf = imx105_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(imx105_groupoff_settings),
	.init_settings = &imx105_init_conf[0],
	.init_size = ARRAY_SIZE(imx105_init_conf),
	.mode_settings = &imx105_confs[0],
	.output_settings = &imx105_dimensions[0],
	.num_conf = ARRAY_SIZE(imx105_confs),
};

static struct msm_sensor_ctrl_t imx105_s_ctrl = {
	.msm_sensor_reg = &imx105_regs,
	.sensor_i2c_client = &imx105_sensor_i2c_client,
	.sensor_i2c_addr = 0x34,
	.sensor_output_reg_addr = &imx105_reg_addr,
	.sensor_id_info = &imx105_id_info,
	.sensor_exp_gain_info = &imx105_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &imx105_csi_params_array[0],
	.msm_sensor_mutex = &imx105_mut,
	.sensor_i2c_driver = &imx105_i2c_driver,
	.sensor_v4l2_subdev_info = imx105_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(imx105_subdev_info),
	.sensor_v4l2_subdev_ops = &imx105_subdev_ops,
	.func_tbl = &imx105_func_tbl,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Sony 8MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");

/*HTC end Tom Lin 2011/12/19 Sony IMX105 8M driver*/
