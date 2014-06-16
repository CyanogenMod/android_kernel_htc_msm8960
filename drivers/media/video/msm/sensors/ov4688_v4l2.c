
#include "msm_sensor.h"
#include "../yushanII/ilp0100_ST_definitions.h"
#include "../yushanII/ilp0100_ST_api.h"

#define SENSOR_NAME_I2C_0X20 "ov4688_0x20"
#define SENSOR_NAME_I2C_0X6C "ov4688_0x6c"

#define PLATFORM_DRIVER_NAME "msm_camera_ov4688"
#define ov4688_obj ov4688_##obj

#define OV4688_REG_READ_MODE 0x0101

#define OV4688_REG_FLIP_MODE 0x3820	
#define OV4688_REG_MIRROR_MODE 0x3821	
#define OV4688_REG_NON_MIRROR_FLIP 0x0000
#define OV4688_REG_FLIP 0x6
#define OV4688_REG_MIRROR 0x6

#define OV4688_READ_NORMAL_MODE 0x0000	
#define OV4688_READ_MIRROR 0x0001			
#define OV4688_READ_FLIP 0x0002			
#define OV4688_READ_MIRROR_FLIP 0x0003	

#define REG_DIGITAL_GAIN_GREEN_R 0x020E
#define REG_DIGITAL_GAIN_RED 0x0210
#define REG_DIGITAL_GAIN_BLUE 0x0212
#define REG_DIGITAL_GAIN_GREEN_B 0x0214


DEFINE_MUTEX(ov4688_mut);
DEFINE_MUTEX(ov4688_sensor_init_mut);

struct ov4688_hdr_exp_info_t {
	uint16_t long_coarse_int_time_addr_h;
	uint16_t long_coarse_int_time_addr_m;
	uint16_t long_coarse_int_time_addr_l;
	uint16_t middle_coarse_int_time_addr_h;
	uint16_t middle_coarse_int_time_addr_m;
	uint16_t middle_coarse_int_time_addr_l;
	uint16_t long_gain_addr_h;
	uint16_t long_gain_addr_m;
	uint16_t long_gain_addr_l;
	uint16_t middle_gain_addr_h;
	uint16_t middle_gain_addr_m;
	uint16_t middle_gain_addr_l;
	uint16_t vert_offset;
	uint32_t sensor_max_linecount; 
};

static struct msm_sensor_ctrl_t ov4688_s_ctrl;

static struct msm_camera_i2c_reg_conf ov4688_start_settings[] = {
	{0x0100, 0x01},

	{0x301a, 0xf9},
    {0xffff, 10},
	{0x301a, 0xf1},
	{0x4805, 0x00},
	{0x301a, 0xf0},

};

static struct msm_camera_i2c_reg_conf ov4688_start_settings2[] = {
	{0x3105, 0x11}, 
	{0x301a, 0xf1}, 
	{0x4805, 0x00}, 
    {0x301a, 0xf0},
    {0x3208, 0x00},
	{0x302a, 0x00}, 
	{0x302a, 0x00}, 
	{0x302a, 0x00}, 
	{0x302a, 0x00}, 
	{0x302a, 0x00}, 
	{0x3601, 0x00}, 
	{0x3638, 0x00}, 
	{0x3208, 0x10}, 
	{0x3208, 0xa0}, 
};


static struct msm_camera_i2c_reg_conf ov4688_stop_settings[] = {

	{0x0100, 0x00},
    {0xffff, 100},
	{0x301a, 0xf9},
	{0x4805, 0x03},
};

static struct msm_camera_i2c_reg_conf ov4688_groupon_settings[] = {
	{0x3208, 0x00},
};

static struct msm_camera_i2c_reg_conf ov4688_groupoff_settings[] = {
    {0x3208, 0x10},
	{0x3208, 0xa0},
};

static struct msm_camera_i2c_reg_conf ov4688_groupon_settings_hdr[] = {
	{0x3208, 0x01},
};

static struct msm_camera_i2c_reg_conf ov4688_groupoff_settings_hdr[] = {
    {0x3208, 0x11},
};

 struct msm_camera_i2c_reg_conf ov4688_prev_settings[] = {
    {0xffff, 200},
    
    {0x3841 , 0x02},
    {0x3846 , 0x08},
    {0x3847 , 0x07},
    {0x4800 , 0x04},
    {0x376e , 0x00},

	
	{0x3632, 0x05},
	{0x376b, 0x40},
	{0x3800, 0x00},
	{0x3801, 0x08},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x0a},
	{0x3805, 0x97},
	{0x3806, 0x05},
	{0x3807, 0xff},
	{0x3808, 0x05},
	{0x3809, 0x40},
	{0x380a, 0x02},
	{0x380b, 0xf8+1},
	{0x380c, 0x09}, 
	{0x380d, 0xcd},
	{0x380e, 0x03},
	{0x380f, 0x1d},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x02-1},
	{0x3814, 0x03},
	{0x3815, 0x01},
	{0x3819, 0x01},
	{0x3820, 0x00},
	{0x3821, 0x07},
	{0x3829, 0x00},
	{0x382a, 0x03},
	{0x382b, 0x01},
	{0x382d, 0x7f},
	{0x3830, 0x08},
	{0x3836, 0x02},
	{0x4001, 0x50},
	{0x4022, 0x03},
	{0x4023, 0xe7},
	{0x4024, 0x05},
	{0x4025, 0x14},
	{0x4026, 0x05},
	{0x4027, 0x23},
	{0x4502, 0x44},
	{0x4601, 0x28},
	{0x5050, 0x0c}, 
};

static struct msm_camera_i2c_reg_conf ov4688_video_settings[] = {

    {0xffff, 200},
    {0x3632, 0x00},
    {0x376b, 0x20},

    
    {0x3841 , 0x02},
    {0x3846 , 0x08},
    {0x3847 , 0x07},
    {0x4800 , 0x04},
    {0x376e , 0x00},
    


    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x04},
    {0x3804, 0x0a},
    {0x3805, 0x9f},
    {0x3806, 0x05},
    {0x3807, 0xfb},
    {0x3808, 0x0a},
    {0x3809, 0x80},
    {0x380a, 0x05},
    {0x380b, 0xf0+1}, 
    {0x380c, 0x06},
    {0x380d, 0x48},
    {0x380e, 0x06},
    {0x380f, 0x12},
    {0x3810, 0x00},
    {0x3811, 0x10},
    {0x3812, 0x00},
    {0x3813, 0x04-1}, 
    {0x3814, 0x01},
    {0x3815, 0x01},
    {0x3819, 0x01},
    {0x3820, 0x00},
    {0x3821, 0x06},
    {0x3829, 0x00},
    {0x382a, 0x01},
    {0x382b, 0x01},
    {0x382d, 0x7f},
    {0x3830, 0x04},
    {0x3836, 0x01},
    {0x4001, 0x40},
    {0x4022, 0x07},
    {0x4023, 0xcf},
    {0x4024, 0x09},
    {0x4025, 0x60},
    {0x4026, 0x09},
    {0x4027, 0x6f},
    {0x4502, 0x40},
    {0x4601, 0x04},
    {0x5050, 0x0c},

};

struct msm_camera_i2c_reg_conf ov4688_fast_video_settings[] = {
    
    {0xffff, 200},

    
    {0x3841 , 0x02},
    {0x3846 , 0x08},
    {0x3847 , 0x07},
    {0x4800 , 0x04},
    {0x376e , 0x00},

    {0x3632, 0x05},
    {0x376b, 0x40},
    {0x3800, 0x00},
    {0x3801, 0x08},
    {0x3802, 0x00},
    {0x3803, 0x04},
    {0x3804, 0x0a},
    {0x3805, 0x97},
    {0x3806, 0x05},
    {0x3807, 0xff},
    {0x3808, 0x05},
    {0x3809, 0x40},
    {0x380a, 0x02},
    {0x380b, 0xf8+1},
    {0x380c, 0x05}, 
    {0x380d, 0xe8},
    {0x380e, 0x03},
    {0x380f, 0x1d},
    {0x3810, 0x00},
    {0x3811, 0x04},
    {0x3812, 0x00},
    {0x3813, 0x02-1},
    {0x3814, 0x03},
    {0x3815, 0x01},
    {0x3819, 0x01},
    {0x3820, 0x00},
    {0x3821, 0x07},
    {0x3829, 0x00},
    {0x382a, 0x03},
    {0x382b, 0x01},
    {0x382d, 0x7f},
    {0x3830, 0x08},
    {0x3836, 0x02},
    {0x4001, 0x50},
    {0x4022, 0x03},
    {0x4023, 0xe7},
    {0x4024, 0x05},
    {0x4025, 0x14},
    {0x4026, 0x05},
    {0x4027, 0x23},
    {0x4502, 0x44},
    {0x4601, 0x28},
    {0x5050, 0x0c},

};

struct msm_camera_i2c_reg_conf ov4688_4_3_settings[] = {
    {0xffff, 200},


    {0x3632 , 0x00},
    {0x376b , 0x20},


    
    {0x3841 , 0x02},
    {0x3846 , 0x08},
    {0x3847 , 0x07},
    {0x4800 , 0x04},
    {0x376e , 0x00},
    


    {0x3800 , 0x01},
    {0x3801 , 0x34},
    {0x3802 , 0x00},
    {0x3803 , 0x04},
    {0x3804 , 0x09},
    {0x3805 , 0x4f},
    {0x3806 , 0x05},
    {0x3807 , 0xfb},
    {0x3808 , 0x08},
    {0x3809 , 0x04},
    {0x380a , 0x05},
    {0x380b , 0xf0+1},
    {0x380c , 0x05},
    {0x380d , 0xc0},
    {0x380e , 0x06},
    {0x380f , 0x60},
    {0x3810 , 0x00},
    {0x3811 , 0x04},
    {0x3812 , 0x00},
    {0x3813 , 0x04-1},
    {0x3814 , 0x01},
    {0x3815 , 0x01},
    {0x3819 , 0x01},
    {0x3820 , 0x00},
    {0x3821 , 0x06},
    {0x3829 , 0x00},
    {0x382a , 0x01},
    {0x382b , 0x01},
    {0x382d , 0x7f},
    {0x3830 , 0x04},
    {0x3836 , 0x01},

    {0x4001 , 0x40},
    {0x4022 , 0x06},
    {0x4023 , 0x3f},
    {0x4024 , 0x07},
    {0x4025 , 0x6c},
    {0x4026 , 0x07},
    {0x4027 , 0x7b},
    {0x4502 , 0x40},
    {0x4601 , 0x04},
    {0x5050 , 0x0c},
};

static struct msm_camera_i2c_reg_conf ov4688_hdr_settings[] = {
    {0xffff, 200},


   {0x3632 , 0x00},
   {0x376b , 0x20},

   {0x4308 , 0x03}, 

   {0x3800 , 0x01 },
   {0x3801 , 0xb0 },
   {0x3802 , 0x01 },
   {0x3803 , 0x00 },
   {0x3804 , 0x09 },
   {0x3805 , 0x77 },
   {0x3806 , 0x05 },
   {0x3807 , 0x59 },
   {0x3808 , 0x07 },
   {0x3809 , 0xa0 },
   {0x380a , 0x04 },
   {0x380b , 0x41 },
   {0x380c , 0x08 },
   {0x380d , 0xc0 },
   {0x380e , 0x04 },
   {0x380f , 0xc0 },
   {0x3810 , 0x00 },
   {0x3811 , 0x04 },
   {0x3812 , 0x00 },
   {0x3813 , 0x01 },
   {0x3814 , 0x01 },
   {0x3815 , 0x01 },
   {0x3819 , 0x01 },
   {0x3820 , 0x00 },
   {0x3821 , 0x06 },
   {0x3829 , 0x00 },
   {0x382a , 0x01 },
   {0x382b , 0x01 },
   {0x382d , 0x7f },
   {0x3830 , 0x04 },
   {0x3836 , 0x01 },

   {0x4001 , 0x40 },
   {0x4022 , 0x06 },
   {0x4023 , 0x3f },
   {0x4024 , 0x07 },
   {0x4025 , 0x6c },
   {0x4026 , 0x07 },
   {0x4027 , 0x7b },
   {0x4502 , 0x40 },
   {0x4601 , 0x04 },
   {0x5050 , 0x0c },

   {0x3841 , 0x03},
   {0x3846 , 0x08},
   {0x3847 , 0x06},
   {0x4800 , 0x0C},
   {0x376e , 0x01},

};

struct msm_camera_i2c_reg_conf ov4688_16_9_settings_non_hdr[] = {
    {0xffff, 200},

    {0x3632, 0x00},
    {0x376b, 0x20},

    
    {0x3841 , 0x02},
    {0x3846 , 0x08},
    {0x3847 , 0x07},
    {0x4800 , 0x04},
    {0x376e , 0x00},
   

    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x04},
    {0x3804, 0x0a},
    {0x3805, 0x9f},
    {0x3806, 0x05},
    {0x3807, 0xfb},
    {0x3808, 0x0a},
    {0x3809, 0x80},
    {0x380a, 0x05},
    {0x380b, 0xf0|1}, 
    {0x380c, 0x06},
    {0x380d, 0x48},
    {0x380e, 0x06},
    {0x380f, 0x12},
    {0x3810, 0x00},
    {0x3811, 0x10},
    {0x3812, 0x00},
    {0x3813, 0x04-1}, 
    {0x3814, 0x01},
    {0x3815, 0x01},
    {0x3819, 0x01},
    {0x3820, 0x00},
    {0x3821, 0x06},
    {0x3829, 0x00},
    {0x382a, 0x01},
    {0x382b, 0x01},
    {0x382d, 0x7f},
    {0x3830, 0x04},
    {0x3836, 0x01},
    {0x4001, 0x40},
    {0x4022, 0x07},
    {0x4023, 0xcf},
    {0x4024, 0x09},
    {0x4025, 0x60},
    {0x4026, 0x09},
    {0x4027, 0x6f},
    {0x4502, 0x40},
    {0x4601, 0x04},
    {0x5050, 0x0c},
};

static struct msm_camera_i2c_reg_conf ov4688_recommend_settings[] = {

    {0x0103, 0x01},
    {0x3638, 0x00},
    {0x0300, 0x00},
    {0x0302, 0x30}, 
    {0x0303, 0x01}, 
    {0x0304, 0x03},
    {0x030b, 0x00},
    {0x030d, 0x1e},
    {0x030e, 0x04},
    {0x030f, 0x01},
    {0x0312, 0x01},
    {0x031e, 0x00},
    {0x3000, 0x20},
    {0x3002, 0x00},
    {0x3018, 0x72},
    {0x3020, 0x93},
    {0x3021, 0x03}, 
    {0x3022, 0x01},
    {0x3031, 0x0a},
    {0x3305, 0xf1},
    {0x3307, 0x04},
    {0x3309, 0x29},
    {0x3500, 0x00},
    {0x3501, 0x60},
    {0x3502, 0x00},
    {0x3503, 0x04}, 
    {0x3504, 0x00},
    {0x3505, 0x00},
    {0x3506, 0x00},
    {0x3507, 0x00},
    {0x3508, 0x00},
    {0x3509, 0x80},
    {0x350a, 0x00},
    {0x350b, 0x00},
    {0x350c, 0x00},
    {0x350d, 0x00},
    {0x350e, 0x00},
    {0x350f, 0x80},
    {0x3510, 0x00},
    {0x3511, 0x00},
    {0x3512, 0x00},
    {0x3513, 0x00},
    {0x3514, 0x00},
    {0x3515, 0x80},
    {0x3516, 0x00},
    {0x3517, 0x00},
    {0x3518, 0x00},
    {0x3519, 0x00},
    {0x351a, 0x00},
    {0x351b, 0x80},
    {0x351c, 0x00},
    {0x351d, 0x00},
    {0x351e, 0x00},
    {0x351f, 0x00},
    {0x3520, 0x00},
    {0x3521, 0x80},
    {0x3522, 0x08},
    {0x3524, 0x08},
    {0x3526, 0x08},
    {0x3528, 0x08},
    {0x352a, 0x08},
    {0x3602, 0x00},

	{0x3603, 0x01},    

    {0x3604, 0x02},
    {0x3605, 0x00},
    {0x3606, 0x00},
    {0x3607, 0x00},
    {0x3609, 0x12},
    {0x360a, 0x40},
    {0x360c, 0x08},
    {0x360f, 0xe5},
    {0x3608, 0x8f},
    {0x3611, 0x00},
    {0x3613, 0xf7},
    {0x3616, 0x58},
    {0x3619, 0x99},
    {0x361b, 0x60},
    {0x361c, 0x7a},
    {0x361e, 0x79},
    {0x361f, 0x02},
    {0x3633, 0x10},
    {0x3634, 0x10},
    {0x3635, 0x10},
    {0x3636, 0x15},
    {0x3646, 0x86},
    {0x364a, 0x0b},
    {0x3700, 0x17},
    {0x3701, 0x22},
    {0x3703, 0x10},
    {0x370a, 0x37},
    {0x3705, 0x00},
    {0x3706, 0x63},
    {0x3709, 0x3c},
    {0x370b, 0x01},
    {0x370c, 0x30},
    {0x3710, 0x24},
    {0x3711, 0x0c},
    {0x3716, 0x00},
    {0x3720, 0x28},
    {0x3729, 0x7b},
    {0x372a, 0x84},
    {0x372b, 0xbd},
    {0x372c, 0xbc},
    {0x372e, 0x52},
    {0x373c, 0x0e},
    {0x373e, 0x33},
    {0x3743, 0x10},
    {0x3744, 0x88},
    {0x374a, 0x43},
    {0x374c, 0x00},
    {0x374e, 0x23},
    {0x3751, 0x7b},
    {0x3752, 0x84},
    {0x3753, 0xbd},
    {0x3754, 0xbc},
    {0x3756, 0x52},
    {0x375c, 0x00},
    {0x3760, 0x00},
    {0x3761, 0x00},
    {0x3762, 0x00},
    {0x3763, 0x00},
    {0x3764, 0x00},
    {0x3767, 0x04},
    {0x3768, 0x04},
    {0x3769, 0x08},
    {0x376a, 0x08},
    {0x376c, 0x00},
    {0x376d, 0x00},
    {0x376e, 0x00},
    {0x3773, 0x00},
    {0x3774, 0x51},
    {0x3776, 0xbd},
    {0x3777, 0xbd},

    {0x3781, 0x18},
    {0x3783, 0x25},
    {0x3841, 0x02},
    {0x3846, 0x08},
    {0x3847, 0x07},
    {0x3d85, 0x36},
    {0x3d8c, 0x71},
    {0x3d8d, 0xcb},
    {0x3f0a, 0x00},
    {0x4000, 0x71},
    {0x4002, 0x04},
    {0x4003, 0x14},

    {0x4004, 0x01},
    {0x4005, 0x00},

    {0x400e, 0x00},
    {0x4011, 0x00},
    {0x401a, 0x00},
    {0x401b, 0x00},
    {0x401c, 0x00},
    {0x401d, 0x00},
    {0x401f, 0x00},
    {0x4020, 0x00},
    {0x4021, 0x10},
    {0x4028, 0x00},
    {0x4029, 0x02},
    {0x402a, 0x06},
    {0x402b, 0x04},
    {0x402c, 0x02},
    {0x402d, 0x02},
    {0x402e, 0x0e},
    {0x402f, 0x04},
    {0x4302, 0xff},
    {0x4303, 0xff},
    {0x4304, 0x00},
    {0x4305, 0x00},
    {0x4306, 0x00},
    {0x4308 , 0x03}, 

    {0x4500, 0x6c},
    {0x4501, 0xc4},
    {0x4503, 0x02},
    {0x4800, 0x04},
    {0x4813, 0x08},
    {0x481f, 0x40},
    {0x4829, 0x78},
    {0x4837, 0x1c},
    {0x4b00, 0x2a},
    {0x4b0d, 0x00},
    {0x4d00, 0x04},
    {0x4d01, 0x42}, 
    {0x4d02, 0xd1},
    {0x4d03, 0x93},
    {0x4d04, 0xf5},
    {0x4d05, 0xc1},
    {0x5000, 0xf3},
    {0x5001, 0x11},
    {0x5004, 0x00},
    {0x500a, 0x00},
    {0x500b, 0x00},
    {0x5032, 0x00},

	
	{0x5500, 0x00},
	{0x5501, 0x10},
	{0x5502, 0x01},
	{0x5503, 0x0f},
	

    {0x5040, 0x00},
    {0x8000, 0x00},
    {0x8001, 0x00},
    {0x8002, 0x00},
    {0x8003, 0x00},
    {0x8004, 0x00},
    {0x8005, 0x00},
    {0x8006, 0x00},
    {0x8007, 0x00},
    {0x8008, 0x00},
    {0x3638, 0x00},
    {0x3105, 0x31},
    {0x301a, 0xf9}, 

	
	{0x484b, 0x05},
	{0x4805, 0x03},
	{0x3508, 0x07},
	{0x3601, 0x01},
	{0x3603, 0x01},
	
};

static struct msm_camera_i2c_reg_conf ov4688_zoe_settings[] = {

    {0xffff, 200},

    {0x3632, 0x00},
    {0x376b, 0x20},

    
    {0x3841 , 0x02},
    {0x3846 , 0x08},
    {0x3847 , 0x07},
    {0x4800 , 0x04},
    {0x376e , 0x00},
    


    {0x3800, 0x00},
    {0x3801, 0x00},
    {0x3802, 0x00},
    {0x3803, 0x04},
    {0x3804, 0x0a},
    {0x3805, 0x9f},
    {0x3806, 0x05},
    {0x3807, 0xfb},
    {0x3808, 0x0a},
    {0x3809, 0x80},
    {0x380a, 0x05},
    {0x380b, 0xf0+1}, 
    {0x380c, 0x09},
    {0x380d, 0x10},
    {0x380e, 0x09},
    {0x380f, 0x2d},

    
    

    {0x3810, 0x00},
    {0x3811, 0x10},
    {0x3812, 0x00},
    {0x3813, 0x04-1}, 
    {0x3814, 0x01},
    {0x3815, 0x01},
    {0x3819, 0x01},
    {0x3820, 0x00},
    {0x3821, 0x06},
    {0x3829, 0x00},
    {0x382a, 0x01},
    {0x382b, 0x01},
    {0x382d, 0x7f},
    {0x3830, 0x04},
    {0x3836, 0x01},
    {0x4001, 0x40},
    {0x4022, 0x07},
    {0x4023, 0xcf},
    {0x4024, 0x09},
    {0x4025, 0x60},
    {0x4026, 0x09},
    {0x4027, 0x6f},
    {0x4502, 0x40},
    {0x4601, 0x04},
    {0x5050, 0x0c},
};

static struct v4l2_subdev_info ov4688_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order  = 0,
	},
	
};

static struct msm_camera_i2c_conf_array ov4688_init_conf[] = {
	{&ov4688_recommend_settings[0],
	ARRAY_SIZE(ov4688_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_conf_array ov4688_init_2_conf[] = {

};

static struct msm_camera_i2c_conf_array ov4688_confs[] = {
	{&ov4688_16_9_settings_non_hdr[0],  ARRAY_SIZE(ov4688_16_9_settings_non_hdr),   0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov4688_prev_settings[0],          ARRAY_SIZE(ov4688_prev_settings),           0, MSM_CAMERA_I2C_BYTE_DATA},

	{&ov4688_video_settings[0],         ARRAY_SIZE(ov4688_video_settings),          0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov4688_fast_video_settings[0],    ARRAY_SIZE(ov4688_fast_video_settings),     0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov4688_hdr_settings[0],           ARRAY_SIZE(ov4688_hdr_settings),            0, MSM_CAMERA_I2C_BYTE_DATA},


	{&ov4688_4_3_settings[0],           ARRAY_SIZE(ov4688_4_3_settings),            0, MSM_CAMERA_I2C_BYTE_DATA},

    {&ov4688_fast_video_settings[0],    ARRAY_SIZE(ov4688_fast_video_settings),     0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov4688_16_9_settings_non_hdr[0],  ARRAY_SIZE(ov4688_16_9_settings_non_hdr),   0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov4688_zoe_settings[0],           ARRAY_SIZE(ov4688_zoe_settings),            0, MSM_CAMERA_I2C_BYTE_DATA},
};

#define ADD_FRAME_LENGTH_LINES 0x00
#define ADD_LINE_LENGTH 0x0
#define PIXEL_CLK 230400000 
#define De_flicker_pixel_clk 240000000

static struct msm_sensor_output_info_t ov4688_dimensions[] = {
	{
		.x_addr_start = 0x0,
		.y_addr_start = 0x0,
		.x_output = 0xA80, 
		.y_output = 0x5F0+1, 
		.line_length_pclk = 0x6c0+ADD_LINE_LENGTH,
		.frame_length_lines = 0x612+ADD_FRAME_LENGTH_LINES,
		.vt_pixel_clk = De_flicker_pixel_clk,
		.op_pixel_clk = PIXEL_CLK,
		.binning_factor = 1,
		.is_hdr = 0,
        .yushan_status_line_enable = 1,
        .yushan_status_line = 1,
        .yushan_sensor_status_line = 0,
	},
	{
		.x_output = 0x540, 
		.y_output = 0x2F8+1, 
		.line_length_pclk = 0xa00+ADD_LINE_LENGTH,  
		.frame_length_lines = 0x31d+ADD_FRAME_LENGTH_LINES,
		.vt_pixel_clk = De_flicker_pixel_clk,
		.op_pixel_clk = PIXEL_CLK,
		.binning_factor = 2,
		.is_hdr = 0,
        .yushan_status_line_enable = 1,
        .yushan_status_line = 1,
        .yushan_sensor_status_line = 0,
    },
	{
		.x_addr_start = 0x0,
		.y_addr_start = 0x0,
		.x_output = 0xA80, 
		.y_output = 0x5F0+1, 
		.line_length_pclk = 0x9f0+ADD_LINE_LENGTH,
		.frame_length_lines = 0x612+ADD_FRAME_LENGTH_LINES,
		.vt_pixel_clk = De_flicker_pixel_clk,
		.op_pixel_clk = PIXEL_CLK,
		.binning_factor = 1,
		.is_hdr = 0,
        .yushan_status_line_enable = 1,
        .yushan_status_line = 1,
        .yushan_sensor_status_line = 0,
	},
	{
		.x_output = 0x540, 
		.y_output = 0x2F8+1, 
		.line_length_pclk = 0x600+ADD_LINE_LENGTH,  
		.frame_length_lines = 0x31d+ADD_FRAME_LENGTH_LINES,
		.vt_pixel_clk = De_flicker_pixel_clk,
		.op_pixel_clk = PIXEL_CLK,
		.binning_factor = 2,
		.is_hdr = 0,
        .yushan_status_line_enable = 1,
        .yushan_status_line = 1,
        .yushan_sensor_status_line = 0,
	},
	#if 1
	{
		.x_output = 1952, 
		.y_output = 1089, 
		.line_length_pclk = 0xaaa+ADD_LINE_LENGTH,
		.frame_length_lines = 0x4c0+ADD_FRAME_LENGTH_LINES,
		.vt_pixel_clk = De_flicker_pixel_clk,
		.op_pixel_clk = PIXEL_CLK,
		.binning_factor = 1,
		.is_hdr = 1,
		.yushan_status_line_enable = 1,
		.yushan_status_line = 1,
        .yushan_sensor_status_line = 0,
	},
	#endif
	{
        .x_addr_start = 0x0,
        .y_addr_start = 0x0,
        .x_output = 0x804, 
        .y_output = 0x5F0+1, 
        .line_length_pclk = 0x6c0,
        .frame_length_lines = 0x660+ADD_FRAME_LENGTH_LINES,
        .vt_pixel_clk = De_flicker_pixel_clk,
        .op_pixel_clk = PIXEL_CLK,
        .binning_factor = 1,
        .is_hdr = 0,
        .yushan_status_line_enable = 1,
        .yushan_status_line = 1,
        .yushan_sensor_status_line = 0,
	},
    {
        .x_output = 0x540, 
        .y_output = 0x2F8+1, 
        .line_length_pclk = 0x600+ADD_LINE_LENGTH,  
        .frame_length_lines = 0x31d+ADD_FRAME_LENGTH_LINES,
        .vt_pixel_clk = De_flicker_pixel_clk,
        .op_pixel_clk = PIXEL_CLK,
        .binning_factor = 2,
        .is_hdr = 0,
        .yushan_status_line_enable = 1,
        .yushan_status_line = 1,
        .yushan_sensor_status_line = 0,
    },
	{
		.x_addr_start = 0x0,
		.y_addr_start = 0x0,
		.x_output = 0xA80, 
		.y_output = 0x5F0+1, 
		.line_length_pclk = 0x6c0+ADD_LINE_LENGTH,
		.frame_length_lines = 0x612+ADD_FRAME_LENGTH_LINES,
		.vt_pixel_clk = De_flicker_pixel_clk,
		.op_pixel_clk = PIXEL_CLK,
		.binning_factor = 1,
		.is_hdr = 0,
        .yushan_status_line_enable = 1,
        .yushan_status_line = 1,
        .yushan_sensor_status_line = 0,
	},
	{
		.x_addr_start = 0x0,
		.y_addr_start = 0x0,
		.x_output = 0xA80, 
		.y_output = 0x5F0+1, 
		.line_length_pclk = 0x910,
		.frame_length_lines = 0x92d,
		.vt_pixel_clk = De_flicker_pixel_clk,
		.op_pixel_clk = PIXEL_CLK, 
		.binning_factor = 1,
		.is_hdr = 0,
        .yushan_status_line_enable = 1,
        .yushan_status_line = 1,
        .yushan_sensor_status_line = 0,
	},
};

static struct msm_camera_csid_vc_cfg ov4688_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params ov4688_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 4,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = ov4688_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 4,
		.settle_cnt = 20,
	},
};

static struct msm_camera_csi2_params *ov4688_csi_params_array[] = {
	&ov4688_csi_params,
	&ov4688_csi_params,
	&ov4688_csi_params,
	&ov4688_csi_params,
	&ov4688_csi_params,
	&ov4688_csi_params,
	&ov4688_csi_params,
	&ov4688_csi_params,
	&ov4688_csi_params
};

static struct msm_sensor_output_reg_addr_t ov4688_reg_addr = {
	.x_output = 0x3808,
	.y_output = 0x380A,
	.line_length_pclk = 0x380C,
	.frame_length_lines = 0x380E,
};

static struct msm_sensor_id_info_t ov4688_id_info = {
	.sensor_id_reg_addr = 0x300A,
	.sensor_id = 0x4688,
};
#define SENSOR_REGISTER_MAX_LINECOUNT 0xffff
#define SENSOR_VERT_OFFSET 25

static struct msm_sensor_exp_gain_info_t ov4688_exp_gain_info = {
	.coarse_int_time_addr = 0x3500,
	.global_gain_addr = 0x3508,
	.vert_offset = SENSOR_VERT_OFFSET, 
	.min_vert = 4,  
	.sensor_max_linecount = SENSOR_REGISTER_MAX_LINECOUNT - SENSOR_VERT_OFFSET,  
};
#define SENSOR_VERT_OFFSET_HDR 4

static struct ov4688_hdr_exp_info_t ov4688_hdr_gain_info = {
	.long_coarse_int_time_addr_h = 0x3500,
	.long_coarse_int_time_addr_m = 0x3501,
	.long_coarse_int_time_addr_l = 0x3502,
	.middle_coarse_int_time_addr_h = 0x350A,
	.middle_coarse_int_time_addr_m = 0x350B,
	.middle_coarse_int_time_addr_l = 0x350C,
    .long_gain_addr_h = 0x3507,
	.long_gain_addr_m = 0x3508,
	.long_gain_addr_l = 0x3509,
	.middle_gain_addr_h = 0x350D,
	.middle_gain_addr_m = 0x350E,
	.middle_gain_addr_l = 0x350F,
	.vert_offset = SENSOR_VERT_OFFSET_HDR,
	.sensor_max_linecount = SENSOR_REGISTER_MAX_LINECOUNT-SENSOR_VERT_OFFSET_HDR,  
};

int32_t ov4688_set_dig_gain(struct msm_sensor_ctrl_t *s_ctrl, uint16_t dig_gain)
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

static int ov4688_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	if (data->sensor_platform_info)
		ov4688_s_ctrl.mirror_flip = data->sensor_platform_info->mirror_flip;

	ov4688_s_ctrl.actived_ae = false;	
    pr_info("%s: ov4688_s_ctrl.mirror_flip=%d", __func__, ov4688_s_ctrl.mirror_flip);

	 return rc;
}


int ov4688_write_output_settings_specific(struct msm_sensor_ctrl_t *s_ctrl,
       uint16_t res)
{
	int rc = 0;
	int16_t read_data = 0;

    pr_info("%s: ov4688_s_ctrl.mirror_flip=%d", __func__, ov4688_s_ctrl.mirror_flip);

	msm_camera_i2c_read(ov4688_s_ctrl.sensor_i2c_client, OV4688_REG_MIRROR_MODE, &read_data, MSM_CAMERA_I2C_BYTE_DATA);
	read_data = read_data & 0x1;    

	
	if (ov4688_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR_FLIP) {
		msm_camera_i2c_write(ov4688_s_ctrl.sensor_i2c_client, OV4688_REG_FLIP_MODE, OV4688_REG_FLIP, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(ov4688_s_ctrl.sensor_i2c_client, OV4688_REG_MIRROR_MODE, (OV4688_REG_MIRROR | read_data), MSM_CAMERA_I2C_BYTE_DATA);
	}       else if (ov4688_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR) {
		msm_camera_i2c_write(ov4688_s_ctrl.sensor_i2c_client, OV4688_REG_FLIP_MODE, OV4688_REG_NON_MIRROR_FLIP, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(ov4688_s_ctrl.sensor_i2c_client, OV4688_REG_MIRROR_MODE, (OV4688_REG_MIRROR | read_data), MSM_CAMERA_I2C_BYTE_DATA);
	} else if (ov4688_s_ctrl.mirror_flip == CAMERA_SENSOR_FLIP) {
		msm_camera_i2c_write(ov4688_s_ctrl.sensor_i2c_client, OV4688_REG_FLIP_MODE, OV4688_REG_FLIP, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(ov4688_s_ctrl.sensor_i2c_client, OV4688_REG_MIRROR_MODE, (OV4688_REG_NON_MIRROR_FLIP | read_data), MSM_CAMERA_I2C_BYTE_DATA);
	}       else {
		msm_camera_i2c_write(ov4688_s_ctrl.sensor_i2c_client, OV4688_REG_FLIP_MODE, OV4688_REG_NON_MIRROR_FLIP, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(ov4688_s_ctrl.sensor_i2c_client, OV4688_REG_MIRROR_MODE, (OV4688_REG_NON_MIRROR_FLIP | read_data), MSM_CAMERA_I2C_BYTE_DATA);
	}

	return rc;
 }

static const char *ov4688Vendor = "ov";
static const char *ov4688NAME = "ov4688";
static const char *ov4688Size = "4M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	CDBG("%s called\n", __func__);

	sprintf(buf, "%s %s %s\n", ov4688Vendor, ov4688NAME, ov4688Size);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);

static struct kobject *android_ov4688;

static int ov4688_sysfs_init(void)
{
	int ret ;
	pr_info("%s: ov4688:kobject creat and add\n", __func__);

	android_ov4688 = kobject_create_and_add("android_camera", NULL);
	if (android_ov4688 == NULL) {
		pr_info("ov4688_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("ov4688:sysfs_create_file\n");
	ret = sysfs_create_file(android_ov4688, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("ov4688_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_ov4688);
	}

	return 0 ;
}

int32_t ov4688_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	rc = 0;
	pr_info("%s\n", __func__);

	rc = msm_sensor_i2c_probe(client, id);
	if(rc >= 0)
		ov4688_sysfs_init();
	pr_info("%s: rc(%d)\n", __func__, rc);
	return rc;
}

static const struct i2c_device_id ov4688_i2c_0x20_id[] = {
	{SENSOR_NAME_I2C_0X20, (kernel_ulong_t)&ov4688_s_ctrl},
	{ }
};

static struct i2c_driver ov4688_i2c_0x20_driver = {
	.id_table = ov4688_i2c_0x20_id,
	.probe  = ov4688_i2c_probe,
	.driver = {
		.name = SENSOR_NAME_I2C_0X20,
	},
};

static const struct i2c_device_id ov4688_i2c_0x6c_id[] = {
	{SENSOR_NAME_I2C_0X6C, (kernel_ulong_t)&ov4688_s_ctrl},
	{ }
};

static struct i2c_driver ov4688_i2c_0x6c_driver = {
	.id_table = ov4688_i2c_0x6c_id,
	.probe  = ov4688_i2c_probe,
	.driver = {
		.name = SENSOR_NAME_I2C_0X6C,
	},
};


static struct msm_camera_i2c_client ov4688_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};


static int ov4688_read_fuseid(struct sensor_cfg_data *cdata,
	struct msm_sensor_ctrl_t *s_ctrl);
static int power_on=false;
int32_t ov4688_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc;
	struct msm_camera_sensor_info *sdata = NULL;
	struct sensor_cfg_data cdata;
	power_on=true;
	pr_info("%s called\n", __func__);

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_info("%s: failed to s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	if (sdata->camera_power_on == NULL) {
		pr_info("%s: failed to sensor platform_data didnt register\n", __func__);
		return -EIO;
	}

	pr_info("ov4688_power_down,sdata->htc_image=%d",sdata->htc_image);
	if (!sdata->use_rawchip && (sdata->htc_image != HTC_CAMERA_IMAGE_YUSHANII_BOARD)) {
		rc = msm_camio_clk_enable(sdata,CAMIO_CAM_MCLK_CLK);
		if (rc < 0) {
			pr_info("%s: msm_camio_sensor_clk_on failed:%d\n",
			 __func__, rc);
			goto enable_mclk_failed;
		}
	}

	rc = sdata->camera_power_on();
	if (rc < 0) {
		pr_info("%s failed to enable power\n", __func__);
		goto enable_power_on_failed;
	}

	rc = msm_sensor_set_power_up(s_ctrl);
	if (rc < 0) {
		pr_info("%s msm_sensor_power_up failed\n", __func__);
		goto enable_sensor_power_up_failed;
	}

	mdelay(5);

	ov4688_sensor_open_init(sdata);

	ov4688_read_fuseid(&cdata,s_ctrl);
	return rc;

enable_sensor_power_up_failed:
	if (sdata->camera_power_off == NULL)
		pr_info("%s: failed to sensor platform_data didnt register\n", __func__);
	else
		sdata->camera_power_off();
enable_power_on_failed:
	msm_camio_clk_disable(sdata,CAMIO_CAM_MCLK_CLK);
enable_mclk_failed:
	return rc;
}

int32_t ov4688_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc;
	struct msm_camera_sensor_info *sdata = NULL;
	power_on=false;
	pr_info("%s called\n", __func__);

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_info("%s: failed to s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	if (sdata->camera_power_off == NULL) {
		pr_err("%s: failed to sensor platform_data didnt register\n", __func__);
		return -EIO;
	}

	rc = msm_sensor_set_power_down(s_ctrl);
	if (rc < 0)
		pr_info("%s: msm_sensor_power_down failed\n", __func__);

	if (!sdata->use_rawchip && (sdata->htc_image != HTC_CAMERA_IMAGE_YUSHANII_BOARD)) {
		msm_camio_clk_disable(sdata,CAMIO_CAM_MCLK_CLK);
		if (rc < 0)
			pr_info("%s: msm_camio_sensor_clk_off failed:%d\n",
				 __func__, rc);
	}

	rc = sdata->camera_power_off();
	if (rc < 0)
		pr_info("%s: failed to disable power\n", __func__);

	return rc;
}

static int __init msm_sensor_init_module(void)
{
    int rc =0;
	pr_info("ov4688 %s\n", __func__);
	rc = i2c_add_driver(&ov4688_i2c_0x6c_driver);
    if (!rc) {
	    pr_info("ov4688 %s with i2c address 0x6c fail, try 0x20\n", __func__);
        ov4688_s_ctrl.sensor_i2c_addr = 0x20;
        ov4688_s_ctrl.sensor_i2c_driver = &ov4688_i2c_0x20_driver;
        rc = i2c_add_driver(&ov4688_i2c_0x20_driver);
    }
    return rc;
}

static struct v4l2_subdev_core_ops ov4688_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ov4688_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ov4688_subdev_ops = {
	.core = &ov4688_subdev_core_ops,
	.video  = &ov4688_subdev_video_ops,
};


static int ov4688_lookup_actuator(struct msm_sensor_ctrl_t *s_ctrl, char *actuator_name)
{
	int i;
	struct msm_camera_sensor_info *sdata;
	struct msm_actuator_info *actuator_info;
	int actuator_index = -1;

	if ((s_ctrl == NULL) || (s_ctrl->sensordata == NULL))
		return -EFAULT;
	if (actuator_name == NULL)
		return -EFAULT;

	sdata = (struct msm_camera_sensor_info *) s_ctrl->sensordata;

	for(i=0; i<sdata->num_actuator_info_table; i++) {
		actuator_info = &sdata->actuator_info_table[i][0];
		pr_info("index=%d   actuator_info->board_info->type=%s\n", i, actuator_info->board_info->type);
		if(!strncmp(actuator_info->board_info->type, actuator_name, strlen(actuator_name))) {
			actuator_index = i;
			break;
		}
	}

	return actuator_index;
}

#if defined(CONFIG_ACT_OIS_BINDER)
extern void HtcActOisBinder_set_OIS_OTP(uint8_t *otp_data, uint8_t otp_size);

#define OV4688_LITEON_OIS_OTP_SIZE 34
const static short ois_addr[3][OV4688_LITEON_OIS_OTP_SIZE] = {
    
    {0x16A,0x16B,0x16C,0x16D,0x16E,0x16F,0x170,0x171,0x172,0x173,0x174,0x175,0x176,0x177,0x178,0x179,0x17A,0x17B,0x17C,0x17D,0x17E,0x17F,0x180,0x181,   0x124,0x125,0x12B,0x12C,   0x1B2,0x1B3,0x1B4,0x1B5,0x1B6,0x1B7}, 
    {0x182,0x183,0x184,0x185,0x186,0x187,0x188,0x189,0x18A,0x18B,0x18C,0x18D,0x18E,0x18F,0x190,0x191,0x192,0x193,0x194,0x195,0x196,0x197,0x198,0x199,   0x142,0x143,0x149,0x014A,   0x1B8,0x1B9,0x1BA,0x1BB,0x1BC,0x1BD}, 
    {0x19A,0x19B,0x19C,0x19D,0x19E,0x19F,0x1A0,0x1A1,0x1A2,0x1A3,0x1A4,0x1A5,0x1A6,0x1A7,0x1A8,0x1A9,0x1AA,0x1AB,0x1AC,0x1AD,0x1AE,0x1AF,0x1B0,0x1B1,   0x160,0x161,0x167,0x168,   0x1BE,0x1BF,0x1C0,0x1C1,0x1C2,0x1C3}, 
};
#endif

#define OV4688_OTP_ADDRESS_START  (0x7000)
#define OV4688_OTP_ADDRESS_END (0x71FF)
#define OV4688_OTP_SIZE (OV4688_OTP_ADDRESS_END-OV4688_OTP_ADDRESS_START+1)
static uint8_t all_otp_data[OV4688_OTP_SIZE];

static int ov4688_read_fuseid(struct sensor_cfg_data *cdata,
	struct msm_sensor_ctrl_t *s_ctrl)
{
    #define OV4688_LITEON_OTP_SIZE 0x12

    const short addr[3][OV4688_LITEON_OTP_SIZE] = {
        
        {0x126,0x127,0x128,0x129,0x12a,0x110,0x111,0x112,0x12b,0x12c,0x11e,0x11f,0x120,0x121,0x122,0x123,0x124,0x125}, 
        {0x144,0x145,0x146,0x147,0x148,0x12e,0x12f,0x130,0x149,0x14a,0x13c,0x13d,0x13e,0x13f,0x140,0x141,0x142,0x143}, 
        {0x162,0x163,0x164,0x165,0x166,0x14c,0x14d,0x14e,0x167,0x168,0x15a,0x15b,0x15c,0x15d,0x15e,0x15f,0x160,0x161}, 
    };
    static uint8_t otp[OV4688_LITEON_OTP_SIZE];
	static int frist= true;
	uint16_t read_data = 0;

#if defined(CONFIG_ACT_OIS_BINDER)
	int32_t ois_valid_layer=-1;
	static uint8_t ois_otp[OV4688_LITEON_OIS_OTP_SIZE];
#endif

    int32_t i,j;
    int32_t rc = 0;
    const int32_t offset = 0x7000;
    static int32_t valid_layer=-1;
    uint16_t addr_start=0x7100;
    uint16_t addr_end=0x71ca;
    struct msm_camera_sensor_info *sdata = (struct msm_camera_sensor_info *) s_ctrl->sensordata;
    uint8_t driver_ic;
    int actuator_index = -1;

	if (frist) {
	    frist = false;

        rc = msm_camera_i2c_write_tbl (s_ctrl->sensor_i2c_client,
            ov4688_recommend_settings,
            ARRAY_SIZE(ov4688_recommend_settings),MSM_CAMERA_I2C_BYTE_DATA);

        if (rc < 0)
            pr_info("%s: i2c_write recommend settings fail\n", __func__);

        rc = msm_camera_i2c_write_b(s_ctrl->sensor_i2c_client, 0x0100, 0x01);
        if (rc < 0)
            pr_info("%s: i2c_write_b 0x0100 fail\n", __func__);

        rc = msm_camera_i2c_write_b(s_ctrl->sensor_i2c_client, 0x3d84, 0x40);
        if (rc < 0)
            pr_info("%s: i2c_write_b 0x3d84 fail\n", __func__);

        rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d88, addr_start, MSM_CAMERA_I2C_WORD_DATA);
        if (rc < 0)
            pr_info("%s: i2c_write 0x3d88 fail\n", __func__);

        rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3d8a, addr_end, MSM_CAMERA_I2C_WORD_DATA);
        if (rc < 0)
            pr_info("%s: i2c_write 0x3d8a fail\n", __func__);

        rc = msm_camera_i2c_write_b(s_ctrl->sensor_i2c_client, 0x3d81, 0x01);
            if (rc < 0)
                pr_info("%s: i2c_write_b 0x3d81 fail\n", __func__);

        msleep(10);

        
        for (j=2; j>=0; --j) {
            for (i=0; i<OV4688_LITEON_OTP_SIZE; ++i) {
                rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, addr[j][i]+offset, &read_data, MSM_CAMERA_I2C_BYTE_DATA);
                if (rc < 0){
                    pr_err("%s: msm_camera_i2c_read 0x%x failed\n", __func__, addr[j][i]);
                    return rc;
                }
                pr_info("%s: OTP addr 0x%x = 0x%x\n", __func__,  addr[j][i], read_data);

                otp[i]= read_data;

                if (read_data)
                    valid_layer = j;
            }
            if (valid_layer!=-1)
                break;
        }
        pr_info("%s: OTP valid layer = %d\n", __func__,  valid_layer);



#if defined(CONFIG_ACT_OIS_BINDER)
        
        for (j=2; j>=0; --j) {
            for (i=0; i<OV4688_LITEON_OIS_OTP_SIZE; ++i) {
                rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, ois_addr[j][i]+offset, &read_data, MSM_CAMERA_I2C_BYTE_DATA);
                if (rc < 0){
                    pr_err("%s: msm_camera_i2c_read 0x%x failed\n", __func__, ois_addr[j][i]);
                    return rc;
                }
                pr_info("%s: OTP ois_addr 0x%x = 0x%x\n", __func__,  ois_addr[j][i], read_data);

                ois_otp[i]= read_data;

                if (read_data)
                    ois_valid_layer = j;
            }
            if (ois_valid_layer!=-1)
                break;
        }
        pr_info("%s: OTP OIS valid layer = %d\n", __func__,  ois_valid_layer);


	if (ois_valid_layer!=-1) {
		for(i=0; i<OV4688_LITEON_OIS_OTP_SIZE;i ++)
			pr_info("read out OTP OIS data = 0x%x\n", ois_otp[i]);

		HtcActOisBinder_set_OIS_OTP(ois_otp, OV4688_LITEON_OIS_OTP_SIZE);
	}
#endif
        if (board_mfg_mode())
            msm_read_all_otp_data (s_ctrl->sensor_i2c_client, OV4688_OTP_ADDRESS_START,all_otp_data,OV4688_OTP_SIZE);

        rc = msm_camera_i2c_write_b(s_ctrl->sensor_i2c_client, 0x0100, 0x00);
        if (rc < 0)
            pr_info("%s: i2c_write_b 0x0100 fail\n", __func__);
    }
    if (board_mfg_mode())
        msm_dump_otp_to_file (PLATFORM_DRIVER_NAME, valid_layer, OV4688_OTP_ADDRESS_START, all_otp_data,OV4688_OTP_SIZE);
    
    cdata->cfg.fuse.fuse_id_word1 = 0;
    cdata->cfg.fuse.fuse_id_word2 = otp[5];
    cdata->cfg.fuse.fuse_id_word3 = otp[6];
    cdata->cfg.fuse.fuse_id_word4 = otp[7];

    
    cdata->af_value.VCM_BIAS = otp[8];
    cdata->af_value.VCM_OFFSET = otp[9];
    cdata->af_value.VCM_BOTTOM_MECH_MSB = otp[0xa];
    cdata->af_value.VCM_BOTTOM_MECH_LSB = otp[0xb];
    cdata->af_value.AF_INF_MSB = otp[0xc];
    cdata->af_value.AF_INF_LSB = otp[0xd];
    cdata->af_value.AF_MACRO_MSB = otp[0xe];
    cdata->af_value.AF_MACRO_LSB = otp[0xf];
    cdata->af_value.VCM_TOP_MECH_MSB = otp[0x10];
    cdata->af_value.VCM_TOP_MECH_LSB = otp[0x11];
    cdata->af_value.VCM_VENDOR_ID_VERSION = otp[4];
    cdata->sensor_ver = 0x00;
    pr_info("%s: OTP Module vendor = 0x%x\n",               __func__,  otp[0]);
    pr_info("%s: OTP LENS = 0x%x\n",                        __func__,  otp[1]);
    pr_info("%s: OTP Sensor Version = 0x%x\n",              __func__,  otp[2]);
    pr_info("%s: OTP Driver IC Vendor & Version = 0x%x\n",  __func__,  otp[3]);
    pr_info("%s: OTP Actuator vender ID & Version = 0x%x\n",__func__,  otp[4]);

    pr_info("%s: OTP fuse 0 = 0x%x\n", __func__,  cdata->cfg.fuse.fuse_id_word1);
    pr_info("%s: OTP fuse 1 = 0x%x\n", __func__,  cdata->cfg.fuse.fuse_id_word2);
    pr_info("%s: OTP fuse 2 = 0x%x\n", __func__,  cdata->cfg.fuse.fuse_id_word3);
    pr_info("%s: OTP fuse 3 = 0x%x\n", __func__,  cdata->cfg.fuse.fuse_id_word4);

    pr_info("%s: OTP BAIS Calibration data = 0x%x\n",           __func__,  cdata->af_value.VCM_BIAS);
    pr_info("%s: OTP OFFSET Calibration data = 0x%x\n",         __func__,  cdata->af_value.VCM_OFFSET);
    pr_info("%s: OTP VCM bottom mech. Limit (MSByte) = 0x%x\n", __func__,  cdata->af_value.VCM_BOTTOM_MECH_MSB);
    pr_info("%s: OTP VCM bottom mech. Limit (LSByte) = 0x%x\n", __func__,  cdata->af_value.VCM_BOTTOM_MECH_LSB);
    pr_info("%s: OTP Infinity position code (MSByte) = 0x%x\n", __func__,  cdata->af_value.AF_INF_MSB);
    pr_info("%s: OTP Infinity position code (LSByte) = 0x%x\n", __func__,  cdata->af_value.AF_INF_LSB);
    pr_info("%s: OTP Macro position code (MSByte) = 0x%x\n",    __func__,  cdata->af_value.AF_MACRO_MSB);
    pr_info("%s: OTP Macro position code (LSByte) = 0x%x\n",    __func__,  cdata->af_value.AF_MACRO_LSB);
    pr_info("%s: OTP VCM top mech. Limit (MSByte) = 0x%x\n",    __func__,  cdata->af_value.VCM_TOP_MECH_MSB);
    pr_info("%s: OTP VCM top mech. Limit (LSByte) = 0x%x\n",    __func__,  cdata->af_value.VCM_TOP_MECH_LSB);


	if (sdata->num_actuator_info_table > 1)
	{
		driver_ic = otp[3];
		pr_info("%s: driver_ic=%d\n", __func__, driver_ic);

		if (driver_ic == 0x21) { 
			actuator_index = ov4688_lookup_actuator(s_ctrl, "ti201_act");
		} else if (driver_ic == 0x11) { 
			actuator_index = ov4688_lookup_actuator(s_ctrl, "lc898212_act");
		} else if (driver_ic == 0x1)  { 
			actuator_index = ov4688_lookup_actuator(s_ctrl, "rumbas_act");
		}

		if (actuator_index >= 0 && actuator_index < sdata->num_actuator_info_table)
			sdata->actuator_info = &sdata->actuator_info_table[actuator_index][0];
		else {
			pr_info("%s: Actuator lookup fail, use the default actuator in board file\n", __func__);
		}

		pr_info("%s: sdata->actuator_info->board_info->type=%s", __func__, sdata->actuator_info->board_info->type);
		pr_info("%s: sdata->actuator_info->board_info->addr=0x%x", __func__, sdata->actuator_info->board_info->addr);
	}

	return 0;
}

static int ov4688_read_VCM_driver_IC_info(	struct msm_sensor_ctrl_t *s_ctrl)
{
#if 0
	int32_t  rc;
	int page = 0;
	unsigned short info_value = 0, info_index = 0;
	unsigned short  OTP = 0;
	struct msm_camera_i2c_client *msm_camera_i2c_client = s_ctrl->sensor_i2c_client;
	struct msm_camera_sensor_info *sdata;

	pr_info("%s: sensor OTP information:\n", __func__);

	s_ctrl = get_sctrl(&s_ctrl->sensor_v4l2_subdev);
	sdata = (struct msm_camera_sensor_info *) s_ctrl->sensordata;

	if ((sdata->actuator_info_table == NULL) || (sdata->num_actuator_info_table <= 1))
	{
		pr_info("%s: failed to actuator_info_table == NULL or num_actuator_info_table <= 1 return 0\n", __func__);
		return 0;
	}

	
	rc = msm_camera_i2c_write_b(msm_camera_i2c_client, 0x0100, 0x00);
	if (rc < 0)
		pr_info("%s: i2c_write_b 0x0100 fail\n", __func__);

	
	rc = msm_camera_i2c_write_b(msm_camera_i2c_client, 0x3368, 0x18);
	if (rc < 0)
		pr_info("%s: i2c_write_b 0x3368 fail\n", __func__);

	rc = msm_camera_i2c_write_b(msm_camera_i2c_client, 0x3369, 0x00);
	if (rc < 0)
		pr_info("%s: i2c_write_b 0x3369 fail\n", __func__);

	
	rc = msm_camera_i2c_write_b(msm_camera_i2c_client, 0x3400, 0x01);
	if (rc < 0)
		pr_info("%s: i2c_write_b 0x3400 fail\n", __func__);

	mdelay(4);

	
	info_index = 10;
	
	for (page = 3; page >= 0; page--) {
		
		rc = msm_camera_i2c_write_b(msm_camera_i2c_client, 0x3402, page);
		if (rc < 0)
			pr_info("%s: i2c_write_b 0x3402 (select page %d) fail\n", __func__, page);

		
		
		rc = msm_camera_i2c_read_b(msm_camera_i2c_client, (0x3410 + info_index), &info_value);
		if (rc < 0)
			pr_info("%s: i2c_read_b 0x%x fail\n", __func__, (0x3410 + info_index));

		
		if (((info_value & 0x0F) != 0) || page < 0)
			break;

		
		rc = msm_camera_i2c_read_b(msm_camera_i2c_client, (0x3404 + info_index), &info_value);
		if (rc < 0)
			pr_info("%s: i2c_read_b 0x%x fail\n", __func__, (0x3404 + info_index));

		
		if (((info_value & 0x0F) != 0) || page < 0)
			break;

	}

	OTP = (short)(info_value&0x0F);
	pr_info("%s: OTP=%d\n", __func__, OTP);

	if (sdata->num_actuator_info_table > 1)
	{
		if (OTP == 1) 
			sdata->actuator_info = &sdata->actuator_info_table[2][0];
		else if (OTP == 2) 
			sdata->actuator_info = &sdata->actuator_info_table[1][0];

		pr_info("%s: sdata->actuator_info->board_info->type=%s", __func__, sdata->actuator_info->board_info->type);
		pr_info("%s: sdata->actuator_info->board_info->addr=0x%x", __func__, sdata->actuator_info->board_info->addr);
	}

	
#endif
	return 0;
}


int ov4688_write_hdr_exp_gain1_ex(struct msm_sensor_ctrl_t *s_ctrl,
		int mode, uint16_t gain, uint16_t long_dig_gain, uint16_t short_dig_gain, uint32_t long_line, uint32_t short_line)
{
	uint32_t fl_lines;
	uint8_t offset;

	uint32_t fps_divider = Q10;
	pr_info("[CAM]%s: called\n", __func__);

	if (mode == SENSOR_PREVIEW_MODE)
		fps_divider = s_ctrl->fps_divider;

    if(long_line > s_ctrl->sensor_exp_gain_info->sensor_max_linecount)
        long_line = s_ctrl->sensor_exp_gain_info->sensor_max_linecount;
    if(short_line > s_ctrl->sensor_exp_gain_info->sensor_max_linecount)
        short_line = s_ctrl->sensor_exp_gain_info->sensor_max_linecount;

	fl_lines = s_ctrl->curr_frame_length_lines;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (long_line * Q10 > (fl_lines - offset) * fps_divider)
		fl_lines = long_line + offset;
	else
		fl_lines = (fl_lines * fps_divider) / Q10;
	if (short_line * Q10 > (fl_lines - offset) * fps_divider)
		fl_lines = short_line + offset;
	else
		fl_lines = (fl_lines * fps_divider) / Q10;

	if(short_line > 198)
		short_line = 198;
	if(short_line < 2)	
		short_line = 2;

	long_line = short_line*4;

	if(long_line > 1008)	
		long_line = 1008;
	if(long_line < 8)
		long_line = 8;

	pr_info("[CAM] gain:%x long/short exposure:0x%x/0x%x\n",
		gain,
		long_line,
		short_line);

	if (s_ctrl->func_tbl->sensor_group_hold_on_hdr)
		s_ctrl->func_tbl->sensor_group_hold_on_hdr(s_ctrl);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines, fl_lines,
		MSM_CAMERA_I2C_WORD_DATA);

	
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.long_coarse_int_time_addr_h, long_line>>12, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.long_coarse_int_time_addr_m, (long_line>>4)&0xff, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.long_coarse_int_time_addr_l, (long_line<<4)&0xff, MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		ov4688_hdr_gain_info.long_gain_addr_m, gain,
		MSM_CAMERA_I2C_WORD_DATA);

	
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.middle_coarse_int_time_addr_h, short_line>>12, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.middle_coarse_int_time_addr_m, (short_line>>4)&0xff, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.middle_coarse_int_time_addr_l, (short_line<<4)&0xff, MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		ov4688_hdr_gain_info.middle_gain_addr_m, gain,
		MSM_CAMERA_I2C_WORD_DATA);

	if (s_ctrl->func_tbl->sensor_group_hold_off_hdr)
		s_ctrl->func_tbl->sensor_group_hold_off_hdr(s_ctrl);

	ov4688_s_ctrl.actived_ae = false;

	return 0;

}


int ov4688_write_non_hdr_exp_gain1_ex(struct msm_sensor_ctrl_t *s_ctrl,
		int mode, uint16_t gain, uint16_t dig_gain, uint32_t line)
{
	uint32_t fl_lines;
	uint8_t offset;

	uint32_t fps_divider = Q10;
	pr_info("[CAM]%s: called\n", __func__);
	
	if (mode == SENSOR_PREVIEW_MODE)
		fps_divider = s_ctrl->fps_divider;

	if(line > s_ctrl->sensor_exp_gain_info->sensor_max_linecount)
		line = s_ctrl->sensor_exp_gain_info->sensor_max_linecount;

	fl_lines = s_ctrl->curr_frame_length_lines;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line * Q10 > (fl_lines - offset) * fps_divider)
		fl_lines = line + offset;
	else
		fl_lines = (fl_lines * fps_divider) / Q10;

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines, fl_lines,
		MSM_CAMERA_I2C_WORD_DATA);

	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.long_coarse_int_time_addr_h, line>>12, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.long_coarse_int_time_addr_m, (line>>4)&0xff, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.long_coarse_int_time_addr_l, (line<<4)&0xff, MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain,
		MSM_CAMERA_I2C_WORD_DATA);

	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}


int ov4688_write_exp_gain1_ex(struct msm_sensor_ctrl_t *s_ctrl,
		int mode, uint16_t gain, uint16_t dig_gain, uint32_t line)
{

	return ov4688_write_non_hdr_exp_gain1_ex(s_ctrl,mode,gain,dig_gain,line);

}

void ov4688_start_stream_hdr(struct msm_sensor_ctrl_t *s_ctrl){

	pr_info("becker  1031,ov4688_start_stream,HDR");

	msm_camera_i2c_write_tbl(
		s_ctrl->sensor_i2c_client,
		s_ctrl->msm_sensor_reg->start_stream_conf,
		s_ctrl->msm_sensor_reg->start_stream_conf_size,
		s_ctrl->msm_sensor_reg->default_data_type);
	mdelay(50);

#if 0
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x30CC, 0x0,
		MSM_CAMERA_I2C_BYTE_DATA);
	mdelay(50);

	
	msm_camera_i2c_read(s_ctrl->sensor_i2c_client,
		0x3300, &read_data,
		MSM_CAMERA_I2C_BYTE_DATA);
	read_data &= 0xEF;
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x3300, read_data,
		MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x4424, 0x07,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x30CC, 0x3,
		MSM_CAMERA_I2C_BYTE_DATA);
#endif

}

void ov4688_start_stream_non_hdr(struct msm_sensor_ctrl_t *s_ctrl){


	pr_info("ov4688_start_stream,non-HDR");

	msm_camera_i2c_write_tbl(
    		s_ctrl->sensor_i2c_client,
    		ov4688_start_settings,
    		ARRAY_SIZE(ov4688_start_settings),
    		MSM_CAMERA_I2C_BYTE_DATA);

	mdelay(10);

    if (power_on) {
    	msm_camera_i2c_write_tbl(
    		s_ctrl->sensor_i2c_client,
    		ov4688_start_settings2,
    		ARRAY_SIZE(ov4688_start_settings2),
    		MSM_CAMERA_I2C_BYTE_DATA);
           power_on=0;
    }
}

void ov4688_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	CDBG("%s: called\n", __func__);

	pr_info("[CAM] %s, hdr_mode = %d\n", __func__, s_ctrl->sensordata->hdr_mode);
	if(s_ctrl->sensordata->hdr_mode){
		ov4688_start_stream_hdr(s_ctrl);
	}else{
		ov4688_start_stream_non_hdr(s_ctrl);
	}
}

int ov4688_write_hdr_outdoor_flag(struct msm_sensor_ctrl_t *s_ctrl, uint8_t is_outdoor)
{
    int indoor_line_length, outdoor_line_length;
    indoor_line_length = 3000;
    outdoor_line_length = 1760;

    if (is_outdoor)
        msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
            s_ctrl->sensor_output_reg_addr->line_length_pclk, outdoor_line_length,
            MSM_CAMERA_I2C_WORD_DATA);
    else
        msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
            s_ctrl->sensor_output_reg_addr->line_length_pclk, indoor_line_length,
            MSM_CAMERA_I2C_WORD_DATA);
    pr_info("[CAM] ov4688_write_hdr_outdoor_flag is outdoor = %d", is_outdoor);
    return 0;
}

void ov4688_yushanII_set_output_format(struct msm_sensor_ctrl_t *sensor,int res, Ilp0100_structFrameFormat *output_format)
{
	pr_info("[CAM]%s",__func__);
	output_format->ActiveLineLengthPixels = sensor->msm_sensor_reg->output_settings[res].x_output;
	output_format->ActiveFrameLengthLines =	sensor->msm_sensor_reg->output_settings[res].y_output;
	output_format->LineLengthPixels = sensor->msm_sensor_reg->output_settings[res].line_length_pclk * 4;
	output_format->FrameLengthLines = sensor->msm_sensor_reg->output_settings[res].frame_length_lines;

	
	
	output_format->StatusLinesOutputted = sensor->msm_sensor_reg->output_settings[res].yushan_status_line_enable;
	output_format->StatusNbLines = sensor->msm_sensor_reg->output_settings[res].yushan_status_line;
	output_format->ImageOrientation = sensor->sensordata->sensor_platform_info->mirror_flip;
	output_format->StatusLineLengthPixels =
		sensor->msm_sensor_reg->output_settings[res].x_output;
	

	output_format->MinInterframe =
		(output_format->FrameLengthLines -
		output_format->ActiveFrameLengthLines -
		output_format->StatusNbLines);
	output_format->AutomaticFrameParamsUpdate = TRUE;

	if(sensor->sensordata->hdr_mode)
		output_format->HDRMode = STAGGERED;
	else
		output_format->HDRMode = HDR_NONE;

	output_format->Hoffset =
			sensor->msm_sensor_reg->output_settings[res].x_addr_start;
	output_format->Voffset =
			sensor->msm_sensor_reg->output_settings[res].y_addr_start;

	
	output_format->HScaling = 1;
	output_format->VScaling = 1;

	if(sensor->msm_sensor_reg->output_settings[res].binning_factor == 2){
		output_format->Binning = 0x22;
	}else{
		output_format->Binning = 0x11;
	}

}

void ov4688_yushanII_set_parm(struct msm_sensor_ctrl_t *sensor, int res,Ilp0100_structSensorParams *YushanII_sensor)
{
	
	YushanII_sensor->StatusNbLines = sensor->msm_sensor_reg->output_settings[res].yushan_sensor_status_line;
    
	YushanII_sensor->FullActiveLines =
		sensor->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].y_output;
	
	YushanII_sensor->FullActivePixels =
		sensor->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].x_output;
	
	YushanII_sensor->MinLineLength =
		sensor->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].line_length_pclk * 4;
}

void ov4688_yushanII_set_IQ(struct msm_sensor_ctrl_t *sensor,int* channel_offset,int* tone_map,int* disable_defcor,struct yushanii_cls* cls)
{
    *channel_offset = 70;
}

void ov4688_yushanII_active_hold(void)
{
	pr_info("[CAM]%s,actived_ae=%d", __func__, ov4688_s_ctrl.actived_ae);
	if (!ov4688_s_ctrl.actived_ae) {
		msm_camera_i2c_write(ov4688_s_ctrl.sensor_i2c_client, 0x3208, 0xe1, MSM_CAMERA_I2C_BYTE_DATA);
		ov4688_s_ctrl.actived_ae = true;
	}
}

int ov4688_yushanII_ae_updated(void)
{
	return ov4688_s_ctrl.actived_ae;
}

void ov4688_yushanII_set_default_ae(struct msm_sensor_ctrl_t *s_ctrl, uint8_t res)
{
	uint32_t long_line =0x1d4;	
	uint32_t short_line = 0x75;
	uint16_t gain = 0x3f7;

	pr_info("[CAM]%s, res=%d", __func__, res);

	if (res == 4) {	
		
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.long_coarse_int_time_addr_h, long_line>>12, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.long_coarse_int_time_addr_m, (long_line>>4)&0xff, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.long_coarse_int_time_addr_l, (long_line<<4)&0xff, MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			ov4688_hdr_gain_info.long_gain_addr_m, gain,
			MSM_CAMERA_I2C_WORD_DATA);

		
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.middle_coarse_int_time_addr_h, short_line>>12, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.middle_coarse_int_time_addr_m, (short_line>>4)&0xff, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.middle_coarse_int_time_addr_l, (short_line<<4)&0xff, MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			ov4688_hdr_gain_info.middle_gain_addr_m, gain,
			MSM_CAMERA_I2C_WORD_DATA);
	} else {
		long_line = s_ctrl->msm_sensor_reg->output_settings[res].frame_length_lines/2;	
		gain = 0xc0;	

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.long_coarse_int_time_addr_h, long_line>>12, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.long_coarse_int_time_addr_m, (long_line>>4)&0xff, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, ov4688_hdr_gain_info.long_coarse_int_time_addr_l, (long_line<<4)&0xff, MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			ov4688_hdr_gain_info.long_gain_addr_m, gain,
			MSM_CAMERA_I2C_WORD_DATA);
	}

}

static struct msm_sensor_fn_t ov4688_func_tbl = {
	.sensor_start_stream = ov4688_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_group_hold_on_hdr = msm_sensor_group_hold_on_hdr,
	.sensor_group_hold_off_hdr = msm_sensor_group_hold_off_hdr,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain_ex = ov4688_write_exp_gain1_ex,
	.sensor_write_hdr_exp_gain_ex = ov4688_write_hdr_exp_gain1_ex,
	
	.sensor_write_snapshot_exp_gain_ex = ov4688_write_exp_gain1_ex,
	.sensor_write_snapshot_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_setting = msm_sensor_setting_parallel,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = ov4688_power_up,
	.sensor_power_down = ov4688_power_down,
	.sensor_i2c_read_fuseid = ov4688_read_fuseid,
	.sensor_i2c_read_vcm_driver_ic = ov4688_read_VCM_driver_IC_info,
    .sensor_write_hdr_outdoor_flag = ov4688_write_hdr_outdoor_flag,
	.sensor_yushanII_set_output_format = ov4688_yushanII_set_output_format,
	.sensor_yushanII_set_parm = ov4688_yushanII_set_parm,
	.sensor_yushanII_set_IQ = ov4688_yushanII_set_IQ,
	.sensor_write_output_settings_specific = ov4688_write_output_settings_specific,	
	.sensor_yushanII_active_hold = ov4688_yushanII_active_hold,
	.sensor_yushanII_ae_updated = ov4688_yushanII_ae_updated,
	.sensor_yushanII_set_default_ae = ov4688_yushanII_set_default_ae,
};

static struct msm_sensor_reg_t ov4688_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = ov4688_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ov4688_start_settings),
	.stop_stream_conf = ov4688_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ov4688_stop_settings),
	.group_hold_on_conf = ov4688_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(ov4688_groupon_settings),
	.group_hold_off_conf = ov4688_groupoff_settings,
	.group_hold_off_conf_size = ARRAY_SIZE(ov4688_groupoff_settings),
	.group_hold_on_conf_hdr = ov4688_groupon_settings_hdr,
	.group_hold_on_conf_size_hdr = ARRAY_SIZE(ov4688_groupon_settings_hdr),
	.group_hold_off_conf_hdr = ov4688_groupoff_settings_hdr,
	.group_hold_off_conf_size_hdr = ARRAY_SIZE(ov4688_groupoff_settings_hdr),

	.init_settings = &ov4688_init_conf[0],
	.init_size = ARRAY_SIZE(ov4688_init_conf),

	.init_settings_2 = &ov4688_init_2_conf[0],
	.init_size_2 = ARRAY_SIZE(ov4688_init_2_conf),

	.mode_settings = &ov4688_confs[0],
	.output_settings = &ov4688_dimensions[0],
	.num_conf = ARRAY_SIZE(ov4688_confs),
};

static struct msm_sensor_ctrl_t ov4688_s_ctrl = {
	.msm_sensor_reg = &ov4688_regs,
	.sensor_i2c_client = &ov4688_sensor_i2c_client,
	.sensor_i2c_addr = 0x6c,
	.sensor_output_reg_addr = &ov4688_reg_addr,
	.sensor_id_info = &ov4688_id_info,
	.sensor_exp_gain_info = &ov4688_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &ov4688_csi_params_array[0],
	.msm_sensor_mutex = &ov4688_mut,
	.sensor_i2c_driver = &ov4688_i2c_0x6c_driver,
	.sensor_v4l2_subdev_info = ov4688_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ov4688_subdev_info),
	.sensor_v4l2_subdev_ops = &ov4688_subdev_ops,
	.func_tbl = &ov4688_func_tbl,
	.sensor_first_mutex = &ov4688_sensor_init_mut, 
	.yushanII_switch_virtual_channel = 1,
	.adjust_y_output_size = 1,
	.adjust_frame_length_line = 1,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("OV 4MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
