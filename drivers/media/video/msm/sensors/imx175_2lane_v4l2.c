#include "msm_sensor.h"

#ifdef CONFIG_RAWCHIP
#include "rawchip/rawchip.h"
#endif

#define SENSOR_NAME "imx175"
#define PLATFORM_DRIVER_NAME "msm_camera_imx175"
#define imx175_obj imx175_##obj

#define IMX175_REG_SW_RESET 0x0103

#define IMX175_REG_READ_MODE 0x0101
#define IMX175_READ_NORMAL_MODE 0x0000	
#define IMX175_READ_MIRROR 0x0001			
#define IMX175_READ_FLIP 0x0002			
#define IMX175_READ_MIRROR_FLIP 0x0003	

#define REG_DIGITAL_GAIN_GREEN_R 0x020E
#define REG_DIGITAL_GAIN_RED 0x0210
#define REG_DIGITAL_GAIN_BLUE 0x0212
#define REG_DIGITAL_GAIN_GREEN_B 0x0214

DEFINE_MUTEX(imx175_mut);
DEFINE_MUTEX(imx175_sensor_init_mut);
static struct msm_sensor_ctrl_t imx175_s_ctrl;

static struct msm_camera_i2c_reg_conf imx175_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf imx175_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf imx175_groupon_settings[] = {
	{0x104, 0x01},
};

static struct msm_camera_i2c_reg_conf imx175_groupoff_settings[] = {
	{0x104, 0x00},
};

static struct msm_camera_i2c_reg_conf imx175_mipi_settings[] = {
	{0x3020, 0x10},
	{0x302D, 0x03},
	{0x302F, 0x80},
	{0x3032, 0xA3},
	{0x3033, 0x20},
	{0x3034, 0x24},
	{0x3041, 0x15},
	{0x3042, 0x87},
	{0x3050, 0x35},
	{0x3056, 0x57},
	{0x305D, 0x41},
	{0x3097, 0x69},
	{0x3109, 0x41},
	{0x3148, 0x3F},
	{0x330F, 0x07},
	{0x4100, 0x0E},
	{0x4104, 0x32},
	{0x4105, 0x32},
	{0x4108, 0x01},
	{0x4109, 0x7C},
	{0x410A, 0x00},
	{0x410B, 0x00},
};

static struct msm_camera_i2c_reg_conf imx175_pll_settings[] = {
	
	{0x030C, 0x00}, 
	{0x030D, 0xDD}, 
	{0x0301, 0x0A}, 
	{0x0303, 0x01}, 
	{0x0305, 0x06}, 
	{0x0309, 0x0A}, 
	{0x030B, 0x01}, 
	{0x3368, 0x18}, 
	{0x3369, 0x00},
	{0x3344, 0x6F},
	{0x3345, 0x1F},
	{0x3370, 0x7F},
	{0x3371, 0x37},
	{0x3372, 0x67},
	{0x3373, 0x3F},
	{0x3374, 0x3F},
	{0x3375, 0x47},
	{0x3376, 0xCF},
	{0x3377, 0x47},
};

static struct msm_camera_i2c_reg_conf imx175_prev_settings[] = {
	
	{0x030C, 0x00}, 
	{0x030D, 0xA4}, 
	{0x0301, 0x0A}, 
	{0x0303, 0x01}, 
	{0x0305, 0x06}, 
	{0x0309, 0x0A}, 
	{0x030B, 0x01}, 
	{0x3368, 0x18}, 
	{0x3369, 0x00},
	{0x3344, 0x57},
	{0x3345, 0x1F},
	{0x3370, 0x77},
	{0x3371, 0x2F},
	{0x3372, 0x4F},
	{0x3373, 0x2F},
	{0x3374, 0x2F},
	{0x3375, 0x37},
	{0x3376, 0x9F},
	{0x3377, 0x37},
	
	{0x0340, 0x04}, 
	{0x0341, 0xF4},
	{0x0342, 0x0D}, 
	{0x0343, 0x70},
	{0x0344, 0x00}, 
	{0x0345, 0x00},
	{0x0346, 0x00}, 
	{0x0347, 0x00},
	{0x0348, 0x0C}, 
	{0x0349, 0xCF},
	{0x034A, 0x09}, 
	{0x034B, 0x9F},
	{0x034C, 0x06}, 
	{0x034D, 0x68},
	{0x034E, 0x04}, 
	{0x034F, 0xD0},
	{0x33D4, 0x06},
	{0x33D5, 0x68},
	{0x33D6, 0x04},
	{0x33D7, 0xD0},
	{0x3013, 0x04},
	{0x30B0, 0x32},
	{0x30D0, 0x00},
	{0x0390, 0x01}, 
	{0x0401, 0x00}, 
	{0x0405, 0x10}, 
	{0x33C8, 0x00},
	{0x3364, 0x02}, 
	{0x0202, 0x04}, 
	{0x0203, 0xF0},
	{0x0387, 0x01}, 
};

static struct msm_camera_i2c_reg_conf imx175_video_settings[] = {
	
	{0x030C, 0x00}, 
	{0x030D, 0xDD}, 
	{0x0301, 0x0A}, 
	{0x0303, 0x01}, 
	{0x0305, 0x06}, 
	{0x0309, 0x0A}, 
	{0x030B, 0x01}, 
	{0x3368, 0x18}, 
	{0x3369, 0x00},
	{0x3344, 0x6F},
	{0x3345, 0x1F},
	{0x3370, 0x7F},
	{0x3371, 0x37},
	{0x3372, 0x67},
	{0x3373, 0x3F},
	{0x3374, 0x3F},
	{0x3375, 0x47},
	{0x3376, 0xCF},
	{0x3377, 0x47},
	
	{0x0340, 0x06}, 
	{0x0341, 0xEC},
	{0x0342, 0x0D}, 
	{0x0343, 0x70},
	{0x0344, 0x00}, 
	{0x0345, 0x62},
	{0x0346, 0x01}, 
	{0x0347, 0x6C},
	{0x0348, 0x0C}, 
	{0x0349, 0x6D},
	{0x034A, 0x08}, 
	{0x034B, 0x33},
	{0x034C, 0x0C}, 
	{0x034D, 0x0C},
	{0x034E, 0x06}, 
	{0x034F, 0xC8},
	{0x33D4, 0x0C},
	{0x33D5, 0x0C},
	{0x33D6, 0x06},
	{0x33D7, 0xC8},
	{0x3013, 0x04},
	{0x30B0, 0x32},
	{0x30D0, 0x00},
	{0x0390, 0x00}, 
	{0x0401, 0x00}, 
	{0x0405, 0x10}, 
	{0x33C8, 0x00},
	{0x3364, 0x02}, 
	{0x0202, 0x06}, 
	{0x0203, 0xE8},
	{0x0387, 0x01}, 
};

static struct msm_camera_i2c_reg_conf imx175_fast_video_settings[] = {
	
	{0x030C, 0x00}, 
	{0x030D, 0xDD}, 
	{0x0301, 0x09}, 
	{0x0303, 0x01}, 
	{0x0305, 0x06}, 
	{0x0309, 0x0A}, 
	{0x030B, 0x01}, 
	{0x3368, 0x18}, 
	{0x3369, 0x00},
	{0x3344, 0x6F},
	{0x3345, 0x1F},
	{0x3370, 0x7F},
	{0x3371, 0x37},
	{0x3372, 0x67},
	{0x3373, 0x3F},
	{0x3374, 0x3F},
	{0x3375, 0x47},
	{0x3376, 0xCF},
	{0x3377, 0x47},
	
	{0x0340, 0x02}, 
	{0x0341, 0x22},
	{0x0342, 0x0E}, 
	{0x0343, 0x0C},
	{0x0344, 0x00}, 
	{0x0345, 0x00},
	{0x0346, 0x00}, 
	{0x0347, 0xD4},
	{0x0348, 0x0C}, 
	{0x0349, 0xCF},
	{0x034A, 0x08}, 
	{0x034B, 0xCB},
	{0x034C, 0x06}, 
	{0x034D, 0x68},
	{0x034E, 0x01}, 
	{0x034F, 0xFE},
	{0x33D4, 0x06},
	{0x33D5, 0x68},
	{0x33D6, 0x01},
	{0x33D7, 0xFE},
	{0x3013, 0x04},
	{0x30B0, 0x32},
	{0x30D0, 0x00},
	{0x0390, 0x01}, 
	{0x0401, 0x00}, 
	{0x0405, 0x10}, 
	{0x33C8, 0x00},
	{0x3364, 0x02}, 
	{0x0202, 0x02}, 
	{0x0203, 0x1E},
	{0x0387, 0x03}, 
};

static struct msm_camera_i2c_reg_conf imx175_snap_settings[] = {
	
	{0x030C, 0x00}, 
	{0x030D, 0xDD}, 
	{0x0301, 0x0A}, 
	{0x0303, 0x01}, 
	{0x0305, 0x06}, 
	{0x0309, 0x0A}, 
	{0x030B, 0x01}, 
	{0x3368, 0x18}, 
	{0x3369, 0x00},
	{0x3344, 0x6F},
	{0x3345, 0x1F},
	{0x3370, 0x7F},
	{0x3371, 0x37},
	{0x3372, 0x67},
	{0x3373, 0x3F},
	{0x3374, 0x3F},
	{0x3375, 0x47},
	{0x3376, 0xCF},
	{0x3377, 0x47},
	
	{0x0340, 0x09}, 
	{0x0341, 0xC4},
	{0x0342, 0x0D}, 
	{0x0343, 0x70},
	{0x0344, 0x00}, 
	{0x0345, 0x00},
	{0x0346, 0x00}, 
	{0x0347, 0x00},
	{0x0348, 0x0C}, 
	{0x0349, 0xCF},
	{0x034A, 0x09}, 
	{0x034B, 0x9F},
	{0x034C, 0x0C}, 
	{0x034D, 0xD0},
	{0x034E, 0x09}, 
	{0x034F, 0xA0},
	{0x33D4, 0x0C},
	{0x33D5, 0xD0},
	{0x33D6, 0x09},
	{0x33D7, 0xA0},
	{0x3013, 0x04},
	{0x30B0, 0x32},
	{0x30D0, 0x00},
	{0x0390, 0x00}, 
	{0x0401, 0x00}, 
	{0x0405, 0x10}, 
	{0x33C8, 0x00},
	{0x3364, 0x02}, 
	{0x0202, 0x09}, 
	{0x0203, 0xC0},
	{0x0387, 0x01}, 
};

static struct msm_camera_i2c_reg_conf imx175_4_3_settings[] = {
	
	{0x030C, 0x00}, 
	{0x030D, 0xDD}, 
	{0x0301, 0x0A}, 
	{0x0303, 0x01}, 
	{0x0305, 0x06}, 
	{0x0309, 0x0A}, 
	{0x030B, 0x01}, 
	{0x3368, 0x18}, 
	{0x3369, 0x00},
	{0x3344, 0x6F},
	{0x3345, 0x1F},
	{0x3370, 0x7F},
	{0x3371, 0x37},
	{0x3372, 0x67},
	{0x3373, 0x3F},
	{0x3374, 0x3F},
	{0x3375, 0x47},
	{0x3376, 0xCF},
	{0x3377, 0x47},
	
	{0x0340, 0x09}, 
	{0x0341, 0xC4},
	{0x0342, 0x0D}, 
	{0x0343, 0x70},
	{0x0344, 0x00}, 
	{0x0345, 0x00},
	{0x0346, 0x00}, 
	{0x0347, 0x00},
	{0x0348, 0x0C}, 
	{0x0349, 0xCF},
	{0x034A, 0x09}, 
	{0x034B, 0x9F},
	{0x034C, 0x0C}, 
	{0x034D, 0xD0},
	{0x034E, 0x09}, 
	{0x034F, 0xA0},
	{0x33D4, 0x0C},
	{0x33D5, 0xD0},
	{0x33D6, 0x09},
	{0x33D7, 0xA0},
	{0x3013, 0x04},
	{0x30B0, 0x32},
	{0x30D0, 0x00},
	{0x0390, 0x00}, 
	{0x0401, 0x00}, 
	{0x0405, 0x10}, 
	{0x33C8, 0x00},
	{0x3364, 0x02}, 
	{0x0202, 0x09}, 
	{0x0203, 0xC0},
	{0x0387, 0x01}, 
};

static struct msm_camera_i2c_reg_conf imx175_snap_wide_settings[] = {
	
	{0x030C, 0x00}, 
	{0x030D, 0xB4}, 
	{0x0301, 0x0A}, 
	{0x0303, 0x01}, 
	{0x0305, 0x06}, 
	{0x0309, 0x0A}, 
	{0x030B, 0x01}, 
	{0x3368, 0x18}, 
	{0x3369, 0x00},
	{0x3344, 0x5F},
	{0x3345, 0x1F},
	{0x3370, 0x77},
	{0x3371, 0x2F},
	{0x3372, 0x5F},
	{0x3373, 0x37},
	{0x3374, 0x37},
	{0x3375, 0x37},
	{0x3376, 0xB7},
	{0x3377, 0x3F},
	
	{0x0340, 0x07}, 
	{0x0341, 0x64},
	{0x0342, 0x0D}, 
	{0x0343, 0x70},
	{0x0344, 0x00}, 
	{0x0345, 0x00},
	{0x0346, 0x01}, 
	{0x0347, 0x30},
	{0x0348, 0x0C}, 
	{0x0349, 0xCF},
	{0x034A, 0x08}, 
	{0x034B, 0x6F},
	{0x034C, 0x0C}, 
	{0x034D, 0xD0},
	{0x034E, 0x07}, 
	{0x034F, 0x40},
	{0x33D4, 0x0C},
	{0x33D5, 0xD0},
	{0x33D6, 0x07},
	{0x33D7, 0x40},
	{0x3013, 0x04},
	{0x30B0, 0x32},
	{0x30D0, 0x00},
	{0x0390, 0x00}, 
	{0x0401, 0x00}, 
	{0x0405, 0x10}, 
	{0x33C8, 0x00},
	{0x3364, 0x02}, 
	{0x0202, 0x07}, 
	{0x0203, 0x60},
	{0x0387, 0x01}, 
};

 
 static struct msm_camera_i2c_reg_conf imx175_night_settings[] = {
	
	{0x030C, 0x00}, 
	{0x030D, 0xDD}, 
	{0x0301, 0x0A}, 
	{0x0303, 0x01}, 
	{0x0305, 0x06}, 
	{0x0309, 0x0A}, 
	{0x030B, 0x01}, 
	{0x3368, 0x18}, 
	{0x3369, 0x00},
	{0x3344, 0x6F},
	{0x3345, 0x1F},
	{0x3370, 0x7F},
	{0x3371, 0x37},
	{0x3372, 0x67},
	{0x3373, 0x3F},
	{0x3374, 0x3F},
	{0x3375, 0x47},
	{0x3376, 0xCF},
	{0x3377, 0x47},
	
	{0x0340, 0x09}, 
	{0x0341, 0xC4},
	{0x0342, 0x0D}, 
	{0x0343, 0x70},
	{0x0344, 0x00}, 
	{0x0345, 0x00},
	{0x0346, 0x00}, 
	{0x0347, 0x00},
	{0x0348, 0x0C}, 
	{0x0349, 0xCF},
	{0x034A, 0x09}, 
	{0x034B, 0x9F},
	{0x034C, 0x0C}, 
	{0x034D, 0xD0},
	{0x034E, 0x09}, 
	{0x034F, 0xA0},
	{0x33D4, 0x0C},
	{0x33D5, 0xD0},
	{0x33D6, 0x09},
	{0x33D7, 0xA0},
	{0x3013, 0x04},
	{0x30B0, 0x32},
	{0x30D0, 0x00},
	{0x0390, 0x00}, 
	{0x0401, 0x00}, 
	{0x0405, 0x10}, 
	{0x33C8, 0x00},
	{0x3364, 0x02}, 
	{0x0202, 0x09}, 
	{0x0203, 0xC0},
	{0x0387, 0x01}, 
};

static struct msm_camera_i2c_reg_conf imx175_recommend_settings[] = {
};

static struct v4l2_subdev_info imx175_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	
};

static struct msm_camera_i2c_conf_array imx175_init_conf[] = {
	{&imx175_mipi_settings[0],
	ARRAY_SIZE(imx175_mipi_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_recommend_settings[0],
	ARRAY_SIZE(imx175_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_pll_settings[0],
	ARRAY_SIZE(imx175_pll_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_conf_array imx175_confs[] = {
	{&imx175_snap_settings[0],
	ARRAY_SIZE(imx175_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_prev_settings[0],
	ARRAY_SIZE(imx175_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_video_settings[0],
	ARRAY_SIZE(imx175_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_fast_video_settings[0],
	ARRAY_SIZE(imx175_fast_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_snap_wide_settings[0],
	ARRAY_SIZE(imx175_snap_wide_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_4_3_settings[0],
	ARRAY_SIZE(imx175_4_3_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx175_night_settings[0],
	ARRAY_SIZE(imx175_night_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t imx175_dimensions[] = {
	{
		.x_output = 0xCD0,
		.y_output = 0x9A0,
		.line_length_pclk = 0xD70,
		.frame_length_lines = 0x9C4,
		.vt_pixel_clk = 176800000,
		.op_pixel_clk = 176800000,
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
	{
		.x_output = 0x668,
		.y_output = 0x4D0,
		.line_length_pclk = 0xD70,
		.frame_length_lines = 0x4F4,
		.vt_pixel_clk = 131200000,
		.op_pixel_clk = 131200000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x99F,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
	{
		.x_output = 0xC0C,
		.y_output = 0x6C8,
		.line_length_pclk = 0xD70,
		.frame_length_lines = 0x6EC,
		.vt_pixel_clk = 176800000,
		.op_pixel_clk = 176800000,
		.binning_factor = 1,
		.x_addr_start = 0x62,
		.y_addr_start = 0x16C,
		.x_addr_end = 0xC6D,
		.y_addr_end = 0x833,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{
		.x_output = 0x668,
		.y_output = 0x1FE,
		.line_length_pclk = 0xE0C,
		.frame_length_lines = 0x222,
		.vt_pixel_clk = 176800000,
		.op_pixel_clk = 176800000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0xD4,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x8CB,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 7,
		.binning_rawchip = 0x22, 
	},
	{
		.x_output = 0xCD0,
		.y_output = 0x740,
		.line_length_pclk = 0xD70,
		.frame_length_lines = 0x764,
		.vt_pixel_clk = 144000000,
		.op_pixel_clk = 144000000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0x130,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x86F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{
		.x_output = 0xCD0,
		.y_output = 0x9A0,
		.line_length_pclk = 0xD70,
		.frame_length_lines = 0x9C4,
		.vt_pixel_clk = 176800000,
		.op_pixel_clk = 176800000,
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
	{ 
		.x_output = 0xCD0,
		.y_output = 0x9A0,
		.line_length_pclk = 0xD70,
		.frame_length_lines = 0x9C4,
		.vt_pixel_clk = 176800000,
		.op_pixel_clk = 176800000,
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

static struct msm_camera_csid_vc_cfg imx175_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params imx175_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 2,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = imx175_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 2,
		.settle_cnt = 0x1B,
	},
};

static struct msm_camera_csi2_params *imx175_csi_params_array[] = {
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params,
	&imx175_csi_params
};

static struct msm_sensor_output_reg_addr_t imx175_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static struct msm_sensor_id_info_t imx175_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x0175,
};

static struct msm_sensor_exp_gain_info_t imx175_exp_gain_info = {
	.coarse_int_time_addr = 0x202,
	.global_gain_addr = 0x204,
	.vert_offset = 4,
	.min_vert = 4,  
	.sensor_max_linecount = 65531,  
};

int32_t imx175_set_dig_gain(struct msm_sensor_ctrl_t *s_ctrl, uint16_t dig_gain)
{
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		REG_DIGITAL_GAIN_GREEN_R, dig_gain,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		REG_DIGITAL_GAIN_RED, dig_gain,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		REG_DIGITAL_GAIN_BLUE, dig_gain,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		REG_DIGITAL_GAIN_GREEN_B, dig_gain,
		MSM_CAMERA_I2C_WORD_DATA);

	return 0;
}

#if 0
static uint32_t vcm_clib;
static uint16_t vcm_clib_min,vcm_clib_med,vcm_clib_max;

static int imx175_read_vcm_clib(struct sensor_cfg_data *cdata, struct msm_sensor_ctrl_t *s_ctrl)
{
	uint8_t rc=0;
	unsigned short info_value = 0;

	struct msm_camera_i2c_client *imx175_msm_camera_i2c_client = s_ctrl->sensor_i2c_client;

	vcm_clib =0;
	vcm_clib_min = 0;
	vcm_clib_med = 0;
	vcm_clib_max = 0;

	pr_info("%s: sensor OTP information:\n", __func__);

	
	rc = msm_camera_i2c_write_b(imx175_msm_camera_i2c_client, 0x3A1C, 0x00);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x3A1C fail\n", __func__);

	
	rc = msm_camera_i2c_write_b(imx175_msm_camera_i2c_client, 0x0A00, 0x04);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x0A00 (Start) fail\n", __func__);

	mdelay(4);

	vcm_clib =0;
	rc = msm_camera_i2c_write_b(imx175_msm_camera_i2c_client, 0x0A02, 5);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x0A02 (select page %d) fail\n", __func__, 5);
			
	rc = msm_camera_i2c_write_b(imx175_msm_camera_i2c_client, 0x0A00, 0x01);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x0A00: Set read mode fail\n", __func__);

 	rc = msm_camera_i2c_read_b(imx175_msm_camera_i2c_client, (0x0A04), &info_value);
	if (rc < 0)
		pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A04));
	else
		{
		pr_info("%s: i2c_read_b 0x%x\n", __func__, info_value);
		vcm_clib = (vcm_clib << 8) | info_value;
		}
 	rc = msm_camera_i2c_read_b(imx175_msm_camera_i2c_client, (0x0A05), &info_value);
	if (rc < 0)
		pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A05));
	else
		{
		pr_info("%s: i2c_read_b 0x%x\n", __func__, info_value);
		vcm_clib = (vcm_clib << 8) | info_value;
		}

	
	if(vcm_clib >> 8 == 0x03)
		{
		  uint32_t p;

		  rc = msm_camera_i2c_read_b(imx175_msm_camera_i2c_client, (0x0A0C), &info_value);
		  if (rc < 0)
			  pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A0C));
		  else
			  {
			  pr_info("%s: i2c_read_b 0x%x\n", __func__, info_value);
			  vcm_clib = (vcm_clib << 8) | info_value;
			  }
		  rc = msm_camera_i2c_read_b(imx175_msm_camera_i2c_client, (0x0A0D), &info_value);
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
    		rc = msm_camera_i2c_write_b(imx175_msm_camera_i2c_client, 0x0A02, 16);
    		if (rc < 0)
    			pr_err("%s: i2c_write_b 0x0A02 (select page %d) fail\n", __func__, 16);
    				
    		rc = msm_camera_i2c_write_b(imx175_msm_camera_i2c_client, 0x0A00, 0x01);
    		if (rc < 0)
    			pr_err("%s: i2c_write_b 0x0A00: Set read mode fail\n", __func__);
    		
    		rc = msm_camera_i2c_read_b(imx175_msm_camera_i2c_client, (0x0A04), &info_value);
    		if (rc < 0)
    			pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A04));
    		else
    			{
    			pr_info("%s: i2c_read_b 0x%x\n", __func__, info_value);
    			vcm_clib = (vcm_clib << 8) | info_value;
    			}
    		rc = msm_camera_i2c_read_b(imx175_msm_camera_i2c_client, (0x0A05), &info_value);
    		if (rc < 0)
    			pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A05));
    		else
    			{
    			pr_info("%s: i2c_read_b 0x%x\n", __func__, info_value);
    			vcm_clib = (vcm_clib << 8) | info_value;
    			}
    
    		if(vcm_clib >> 8 == 0x04)
    		{
    		  uint32_t p;
    
    		  rc = msm_camera_i2c_read_b(imx175_msm_camera_i2c_client, (0x0A0E), &info_value);
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
		||(
		     (DEFAULT_VCM_MAX < vcm_clib_max) || (DEFAULT_VCM_MAX < vcm_clib_med) || (DEFAULT_VCM_MAX < vcm_clib_min)
		  || (DEFAULT_VCM_MIN > vcm_clib_max) || (DEFAULT_VCM_MIN > vcm_clib_med) || (DEFAULT_VCM_MIN > vcm_clib_min)
		  || ((vcm_clib_med < vcm_clib_min) || (vcm_clib_med > vcm_clib_max))
		))
		{
		  vcm_clib_min=DEFAULT_VCM_MIN;
		  vcm_clib_med=DEFAULT_VCM_MED;
		  vcm_clib_max=DEFAULT_VCM_MAX;
		}

    cdata->cfg.calib_vcm_pos.min=vcm_clib_min;
    cdata->cfg.calib_vcm_pos.med=vcm_clib_med;
    cdata->cfg.calib_vcm_pos.max=vcm_clib_max;

	pr_info("%s: VCM clib=0x%x, min/med/max=%d %d %d\n"
		, __func__, vcm_clib, cdata->cfg.calib_vcm_pos.min, cdata->cfg.calib_vcm_pos.med, cdata->cfg.calib_vcm_pos.max);

	return 0;

}
#endif

static int lens_info;	

static void imx175_read_lens_info(struct msm_sensor_ctrl_t *s_ctrl)
{
	lens_info = 6;	

#if 0
	int32_t  rc;
	int page = 0;
	unsigned short info_value = 0, info_index = 0;
	unsigned short  OTP[10] = {0};
	struct msm_camera_i2c_client *imx175_msm_camera_i2c_client = s_ctrl->sensor_i2c_client;

	lens_info = 6;	

	pr_info("[CAM]%s\n", __func__);
	pr_info("%s: sensor OTP information:\n", __func__);

	
	rc = msm_camera_i2c_write_b(imx175_msm_camera_i2c_client, 0x3A1C, 0x00);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x3A1C fail\n", __func__);

	
	rc = msm_camera_i2c_write_b(imx175_msm_camera_i2c_client, 0x0A00, 0x04);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x0A00 (Start) fail\n", __func__);

	mdelay(4);

	
	info_index = 1;
	info_value = 0;
	memset(OTP, 0, sizeof(OTP));

	for (page = 20; page >= 16; page--) {
		rc = msm_camera_i2c_write_b(imx175_msm_camera_i2c_client, 0x0A02, page);
		if (rc < 0)
			pr_err("%s: i2c_write_b 0x0A02 (select page %d) fail\n", __func__, page);

		
		rc = msm_camera_i2c_write_b(imx175_msm_camera_i2c_client, 0x0A00, 0x01);
		if (rc < 0)
			pr_err("%s: i2c_write_b 0x0A00: Set read mode fail\n", __func__);

		
		rc = msm_camera_i2c_read_b(imx175_msm_camera_i2c_client, (0x0A04 + info_index), &info_value);
		if (rc < 0)
			pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A04 + info_index));

		 
		if (((info_value&0x0F) != 0) || page == 0)
			break;
	}
	OTP[info_index] = (short)(info_value&0x0F);

	if (OTP[1] != 0) {
		pr_info("Get Fuseid from Page20 to Page16\n");
		goto get_done;
	}

	
	info_index = 1;
	info_value = 0;
	memset(OTP, 0, sizeof(OTP));

	for (page = 4; page >= 0; page--) {
		rc = msm_camera_i2c_write_b(imx175_msm_camera_i2c_client, 0x0A02, page);
		if (rc < 0)
			pr_err("%s: i2c_write_b 0x0A02 (select page %d) fail\n", __func__, page);

		
		rc = msm_camera_i2c_write_b(imx175_msm_camera_i2c_client, 0x0A00, 0x01);
		if (rc < 0)
			pr_err("%s: i2c_write_b 0x0A00: Set read mode fail\n", __func__);

		
		rc = msm_camera_i2c_read_b(imx175_msm_camera_i2c_client, (0x0A04 + info_index), &info_value);
		if (rc < 0)
			pr_err("%s: i2c_read_b 0x%x fail\n", __func__, (0x0A04 + info_index));

		 
		if (((info_value & 0x0F) != 0) || page == 0)
			break;
	}
	OTP[info_index] = (short)(info_value&0x0F);

get_done:
	
	rc = msm_camera_i2c_write_b(imx175_msm_camera_i2c_client, 0x0A00, 0x00);
	if (rc < 0)
		pr_err("%s: i2c_write_b 0x0A00 (Stop) fail\n", __func__);

	pr_info("%s: LensID=%x\n", __func__, OTP[1]);

	if (OTP[1] == 5)	
		lens_info = OTP[1];
#endif

	return;
}


static int imx175_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;
	pr_info("%s called\n", __func__);

	if (data->sensor_platform_info)
		imx175_s_ctrl.mirror_flip = data->sensor_platform_info->mirror_flip;

	imx175_read_lens_info(&imx175_s_ctrl);

	return rc;
}

static const char *imx175Vendor = "sony";
static const char *imx175NAME = "imx175";
static const char *imx175Size = "8M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", imx175Vendor, imx175NAME, imx175Size);
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

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);
static DEVICE_ATTR(lensinfo, 0444, lens_info_show, NULL);

static struct kobject *android_imx175;

static int imx175_sysfs_init(void)
{
	int ret ;
	pr_info("imx175:kobject creat and add\n");
	android_imx175 = kobject_create_and_add("android_camera", NULL);
	if (android_imx175 == NULL) {
		pr_info("imx175_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("imx175:sysfs_create_file\n");
	ret = sysfs_create_file(android_imx175, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("imx175_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_imx175);
	}
	pr_info("imx175:sysfs_create_file lensinfo\n");
	ret = sysfs_create_file(android_imx175, &dev_attr_lensinfo.attr);
	if (ret) {
		pr_info("imx175_sysfs_init: dev_attr_lensinfo failed\n");
		kobject_del(android_imx175);
	}
	return 0 ;
}

int32_t imx175_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	rc = 0;
	pr_info("%s\n", __func__);

	rc = msm_sensor_i2c_probe(client, id);
	if(rc >= 0)
		imx175_sysfs_init();
	pr_info("%s: rc(%d)\n", __func__, rc);
	return rc;
}

static const struct i2c_device_id imx175_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&imx175_s_ctrl},
	{ }
};

static struct i2c_driver imx175_i2c_driver = {
	.id_table = imx175_i2c_id,
	.probe  = imx175_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client imx175_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

int32_t imx175_power_up(struct msm_sensor_ctrl_t *s_ctrl)
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

	rc = msm_sensor_set_power_up(s_ctrl);
	if (rc < 0) {
		pr_err("%s msm_sensor_power_up failed\n", __func__);
		goto enable_sensor_power_up_failed;
	}

	imx175_sensor_open_init(sdata);
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

int32_t imx175_power_down(struct msm_sensor_ctrl_t *s_ctrl)
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
	if (rc < 0)
		pr_err("%s failed to disable power\n", __func__);

	rc = msm_sensor_set_power_down(s_ctrl);
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

static int __init msm_sensor_init_module(void)
{
	pr_info("imx175 %s\n", __func__);
	return i2c_add_driver(&imx175_i2c_driver);
}

static struct v4l2_subdev_core_ops imx175_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops imx175_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops imx175_subdev_ops = {
	.core = &imx175_subdev_core_ops,
	.video  = &imx175_subdev_video_ops,
};

static int imx175_read_fuseid(struct sensor_cfg_data *cdata,
	struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	int i;
	uint16_t read_data = 0;
	uint8_t OTP[10] = {0};

	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3400, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0)
		pr_err("%s: msm_camera_i2c_write 0x3400 failed\n", __func__);

	
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3402, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0)
		pr_err("%s: msm_camera_i2c_write 0x3402 failed\n", __func__);

	for (i = 0; i < 10; i++) {
		rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, (0x3404 + i), &read_data, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0)
			pr_err("%s: msm_camera_i2c_read 0x%x failed\n", __func__, (0x3404 + i));

		OTP[i] = (uint8_t)(read_data&0x00FF);
		read_data = 0;
	}

	pr_info("%s: VenderID=%x,LensID=%x,SensorID=%02x%02x\n", __func__,
		OTP[0], OTP[1], OTP[2], OTP[3]);
	pr_info("%s: ModuleFuseID= %02x%02x%02x%02x%02x%02x\n", __func__,
		OTP[4], OTP[5], OTP[6], OTP[7], OTP[8], OTP[9]);

	cdata->cfg.fuse.fuse_id_word1 = 0;
	cdata->cfg.fuse.fuse_id_word2 = (OTP[0]);
	cdata->cfg.fuse.fuse_id_word3 =
		(OTP[4]<<24) |
		(OTP[5]<<16) |
		(OTP[6]<<8) |
		(OTP[7]);
	cdata->cfg.fuse.fuse_id_word4 =
		(OTP[8]<<8) |
		(OTP[9]);

	pr_info("imx175: fuse->fuse_id_word1:%d\n",
		cdata->cfg.fuse.fuse_id_word1);
	pr_info("imx175: fuse->fuse_id_word2:%d\n",
		cdata->cfg.fuse.fuse_id_word2);
	pr_info("imx175: fuse->fuse_id_word3:0x%08x\n",
		cdata->cfg.fuse.fuse_id_word3);
	pr_info("imx175: fuse->fuse_id_word4:0x%08x\n",
		cdata->cfg.fuse.fuse_id_word4);
	return 0;

}

int imx175_write_output_settings_specific(struct msm_sensor_ctrl_t *s_ctrl,
	uint16_t res)
{
	int rc = 0;
	uint16_t value = 0;

	
	if (imx175_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR_FLIP)
		value = IMX175_READ_MIRROR_FLIP;
	else if (imx175_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR)
		value = IMX175_READ_MIRROR;
	else if (imx175_s_ctrl.mirror_flip == CAMERA_SENSOR_FLIP)
		value = IMX175_READ_FLIP;
	else
		value = IMX175_READ_NORMAL_MODE;
	rc = msm_camera_i2c_write(imx175_s_ctrl.sensor_i2c_client,
		IMX175_REG_READ_MODE, value, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s set mirror_flip failed\n", __func__);
		return rc;
	}

	return rc;
}

static struct msm_sensor_fn_t imx175_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain_ex = msm_sensor_write_exp_gain1_ex,
	.sensor_write_snapshot_exp_gain_ex = msm_sensor_write_exp_gain1_ex,
	.sensor_set_dig_gain = imx175_set_dig_gain,
	.sensor_write_snapshot_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_setting = msm_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = imx175_power_up,
	.sensor_power_down = imx175_power_down,
	.sensor_i2c_read_fuseid = imx175_read_fuseid,
#if 0
	
	.sensor_i2c_read_vcm_clib = imx175_read_vcm_clib,
	
#endif
	.sensor_write_output_settings_specific = imx175_write_output_settings_specific, 
};

static struct msm_sensor_reg_t imx175_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = imx175_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(imx175_start_settings),
	.stop_stream_conf = imx175_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(imx175_stop_settings),
	.group_hold_on_conf = imx175_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(imx175_groupon_settings),
	.group_hold_off_conf = imx175_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(imx175_groupoff_settings),
	.init_settings = &imx175_init_conf[0],
	.init_size = ARRAY_SIZE(imx175_init_conf),
	.mode_settings = &imx175_confs[0],
	.output_settings = &imx175_dimensions[0],
	.num_conf = ARRAY_SIZE(imx175_confs),
};

static struct msm_sensor_ctrl_t imx175_s_ctrl = {
	.msm_sensor_reg = &imx175_regs,
	.sensor_i2c_client = &imx175_sensor_i2c_client,
	.sensor_i2c_addr = 0x20,
	.sensor_output_reg_addr = &imx175_reg_addr,
	.sensor_id_info = &imx175_id_info,
	.sensor_exp_gain_info = &imx175_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &imx175_csi_params_array[0],
	.msm_sensor_mutex = &imx175_mut,
	.sensor_i2c_driver = &imx175_i2c_driver,
	.sensor_v4l2_subdev_info = imx175_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(imx175_subdev_info),
	.sensor_v4l2_subdev_ops = &imx175_subdev_ops,
	.func_tbl = &imx175_func_tbl,
	.sensor_first_mutex = &imx175_sensor_init_mut, 
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Sony 8 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
