#include "msm_sensor.h"
#define SENSOR_NAME "ar0260"
#define PLATFORM_DRIVER_NAME "msm_camera_ar0260"
#define ar0260_obj ar0260_##obj

#ifdef CONFIG_RAWCHIPII
#include "yushanII.h"
#include "ilp0100_ST_api.h"
#include "ilp0100_customer_sensor_config.h"
#endif

#define USE_MCLK_780MHZ 1

DEFINE_MUTEX(ar0260_mut);
DEFINE_MUTEX(ar0260_sensor_init_mut); 
static struct msm_sensor_ctrl_t ar0260_s_ctrl;

static struct msm_camera_i2c_reg_conf ar0260_start_settings[] = {
	{0x3C40, 0xAC34,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
	{0x301C, 0x0102,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},

#if 0
	{0xffff,50},
	{0x3C40, 0xAC36,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},	
	{0x301C, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},

	{0xffff,50},
	{0x3C40, 0xAC34,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
	{0x301C, 0x0102,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},	
#endif
};

static struct msm_camera_i2c_reg_conf ar0260_start_settings_yushanii[] = {
	{0x301C, 0x0102,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
	{0x3C40, 0xAC34,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
};

static struct msm_camera_i2c_reg_conf ar0260_stop_settings[] = {
	{0x301C, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
};

static struct msm_camera_i2c_reg_conf ar0260_stop_settings_yushanii[] = {
	{0x301C, 0x0002,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
	{0x3C40, 0xAC36,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
};

static struct msm_camera_i2c_reg_conf ar0260_groupon_settings[] = {
	{0x3022, 0x0100,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
};

static struct msm_camera_i2c_reg_conf ar0260_groupoff_settings[] = {
	{0x3022, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
};

static struct msm_camera_i2c_reg_conf ar0260_prev_settings[] = {

};

static struct msm_camera_i2c_reg_conf ar0260_snap_settings[] = {

};


#ifdef USE_MCLK_780MHZ



static struct msm_camera_i2c_reg_conf ar0260_recommend_settings[] = {

{0x0018, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_EQUAL,0x4000},

{ 0x001A, 0x0005}, 	
{ 0x001C, 0x000C}, 	
{ 0x001A, 0x0014}, 	
{0x001C, 6<<8,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_NOT_EQUAL,0xFF00},


{ 0x0982, 0x0001}, 	
{ 0x098A, 0x6000}, 	
{ 0xE000, 0xC0F1},
{ 0xE002, 0x0C72},
{ 0xE004, 0x0760},
{ 0xE006, 0xD81D},
{ 0xE008, 0x0ABA},
{ 0xE00A, 0x0780},
{ 0xE00C, 0xE080},
{ 0xE00E, 0x0064},
{ 0xE010, 0x0001},
{ 0xE012, 0x0916},
{ 0xE014, 0x0860},
{ 0xE016, 0xD802},
{ 0xE018, 0xD900},
{ 0xE01A, 0x70CF},
{ 0xE01C, 0xFF00},
{ 0xE01E, 0x31B0},
{ 0xE020, 0xB038},
{ 0xE022, 0x1CFC},
{ 0xE024, 0xB388},
{ 0xE026, 0x76CF},
{ 0xE028, 0xFF00},
{ 0xE02A, 0x33CC},
{ 0xE02C, 0x200A},
{ 0xE02E, 0x0F80},
{ 0xE030, 0xFFFF},
{ 0xE032, 0xE048},
{ 0xE034, 0x1CFC},
{ 0xE036, 0xB008},
{ 0xE038, 0x2022},
{ 0xE03A, 0x0F80},
{ 0xE03C, 0x0000},
{ 0xE03E, 0xFC3C},
{ 0xE040, 0x2020},
{ 0xE042, 0x0F80},
{ 0xE044, 0x0000},
{ 0xE046, 0xEA34},
{ 0xE048, 0x1404},
{ 0xE04A, 0x340E},
{ 0xE04C, 0xD801},
{ 0xE04E, 0x71CF},
{ 0xE050, 0xFF00},
{ 0xE052, 0x33CC},
{ 0xE054, 0x190A},
{ 0xE056, 0x801C},
{ 0xE058, 0x208A},
{ 0xE05A, 0x0004},
{ 0xE05C, 0x1964},
{ 0xE05E, 0x8004},
{ 0xE060, 0x0C12},
{ 0xE062, 0x0760},
{ 0xE064, 0xD83C},
{ 0xE066, 0x0E5E},
{ 0xE068, 0x0880},
{ 0xE06A, 0x216F},
{ 0xE06C, 0x003F},
{ 0xE06E, 0xF1FD},
{ 0xE070, 0x0C02},
{ 0xE072, 0x0760},
{ 0xE074, 0xD81E},
{ 0xE076, 0xC0D1},
{ 0xE078, 0x7EE0},
{ 0xE07A, 0x78E0},
{ 0xE07C, 0xC0F1},
{ 0xE07E, 0xE889},
{ 0xE080, 0x70CF},
{ 0xE082, 0xFF00},
{ 0xE084, 0x0000},
{ 0xE086, 0x900E},
{ 0xE088, 0xB8E7},
{ 0xE08A, 0x0F78},
{ 0xE08C, 0xFFC1},
{ 0xE08E, 0xD800},
{ 0xE090, 0xF1F3},

{ 0x098E, 0x0000}, 	
{ 0x098A, 0x5F38}, 	
{ 0x0990, 0xFFFF},
{ 0x0992, 0xE07C},

{ 0x001C, 0x0600}, 	

	{0x001C, 0x3C<<8,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_LESS,0xFF00},

	

{ 0x3E00, 0x042D}, 	
{ 0x3E02, 0x39FF}, 	
{ 0x3E04, 0x49FF}, 	
{ 0x3E06, 0xFFFF}, 	
{ 0x3E08, 0x8071}, 	
{ 0x3E0A, 0x7211}, 	
{ 0x3E0C, 0xE040}, 	
{ 0x3E0E, 0xA840}, 	
{ 0x3E10, 0x4100}, 	
{ 0x3E12, 0x1846}, 	
{ 0x3E14, 0xA547}, 	
{ 0x3E16, 0xAD57}, 	
{ 0x3E18, 0x8149}, 	
{ 0x3E1A, 0x9D49}, 	
{ 0x3E1C, 0x9F46}, 	
{ 0x3E1E, 0x8000}, 	
{ 0x3E20, 0x1842}, 	
{ 0x3E22, 0x4180}, 	
{ 0x3E24, 0x0018}, 	
{ 0x3E26, 0x8149}, 	
{ 0x3E28, 0x9C49}, 	
{ 0x3E2A, 0x9347}, 	
{ 0x3E2C, 0x804D}, 	
{ 0x3E2E, 0x804A}, 	
{ 0x3E30, 0x100C}, 	
{ 0x3E32, 0x8000}, 	
{ 0x3E34, 0x1841}, 	
{ 0x3E36, 0x4280}, 	
{ 0x3E38, 0x0018}, 	
{ 0x3E3A, 0x9710}, 	
{ 0x3E3C, 0x0C80}, 	
{ 0x3E3E, 0x4DA2}, 	
{ 0x3E40, 0x4BA0}, 	
{ 0x3E42, 0x4A00}, 	
{ 0x3E44, 0x1880}, 	
{ 0x3E46, 0x4241}, 	
{ 0x3E48, 0x0018}, 	
{ 0x3E4A, 0xB54B}, 	
{ 0x3E4C, 0x1C00}, 	
{ 0x3E4E, 0x8000}, 	
{ 0x3E50, 0x1C10}, 	
{ 0x3E52, 0x6081}, 	
{ 0x3E54, 0x1580}, 	
{ 0x3E56, 0x7C09}, 	
{ 0x3E58, 0x7000}, 	
{ 0x3E5A, 0x8082}, 	
{ 0x3E5C, 0x7281}, 	
{ 0x3E5E, 0x4C40}, 	
{ 0x3E60, 0x8E4D}, 	
{ 0x3E62, 0x8110}, 	
{ 0x3E64, 0x0CAF}, 	
{ 0x3E66, 0x4D80}, 	
{ 0x3E68, 0x100C}, 	
{ 0x3E6A, 0x8440}, 	
{ 0x3E6C, 0x4C81}, 	
{ 0x3E6E, 0x7C5B}, 	
{ 0x3E70, 0x7000}, 	
{ 0x3E72, 0x8054}, 	
{ 0x3E74, 0x924C}, 	
{ 0x3E76, 0x4078}, 	
{ 0x3E78, 0x4D4F}, 	
{ 0x3E7A, 0x4E98}, 	
{ 0x3E7C, 0x504E}, 	
{ 0x3E7E, 0x4F97}, 	
{ 0x3E80, 0x4F4E}, 	
{ 0x3E82, 0x507C}, 	
{ 0x3E84, 0x7B8D}, 	
{ 0x3E86, 0x4D88}, 	
{ 0x3E88, 0x4E10}, 	
{ 0x3E8A, 0x0940}, 	
{ 0x3E8C, 0x8879}, 	
{ 0x3E8E, 0x5481}, 	
{ 0x3E90, 0x7000}, 	
{ 0x3E92, 0x8082}, 	
{ 0x3E94, 0x7281}, 	
{ 0x3E96, 0x4C40}, 	
{ 0x3E98, 0x8E4D}, 	
{ 0x3E9A, 0x8110}, 	
{ 0x3E9C, 0x0CAF}, 	
{ 0x3E9E, 0x4D80}, 	
{ 0x3EA0, 0x100C}, 	
{ 0x3EA2, 0x8440}, 	
{ 0x3EA4, 0x4C81}, 	
{ 0x3EA6, 0x7C93}, 	
{ 0x3EA8, 0x7000}, 	
{ 0x3EAA, 0x0000}, 	
{ 0x3EAC, 0x0000}, 	
{ 0x3EAE, 0x0000}, 	
{ 0x3EB0, 0x0000}, 	
{ 0x3EB2, 0x0000}, 	
{ 0x3EB4, 0x0000}, 	
{ 0x3EB6, 0x0000}, 	
{ 0x3EB8, 0x0000}, 	
{ 0x3EBA, 0x0000}, 	
{ 0x3EBC, 0x0000}, 	
{ 0x3EBE, 0x0000}, 	
{ 0x3EC0, 0x0000}, 	
{ 0x3EC2, 0x0000}, 	
{ 0x3EC4, 0x0000}, 	
{ 0x3EC6, 0x0000}, 	
{ 0x3EC8, 0x0000}, 	
{ 0x3ECA, 0x0000}, 	

{ 0x30B2, 0xC000}, 	
{ 0x30D4, 0x9400}, 	
{ 0x31C0, 0x0000}, 	
{ 0x316A, 0x8200}, 	
{ 0x316C, 0x8200}, 	
{ 0x3EFE, 0x2808}, 	
{ 0x3EFC, 0x2868}, 	
{ 0x3ED2, 0xD165}, 	
{ 0x3EF2, 0xD165}, 	
{ 0x3ED8, 0x7F1A}, 	
{ 0x3EDA, 0x2828}, 	
{ 0x3EE2, 0x0058}, 	

{ 0x002C, 0x000F}, 	

{ 0x3382, 0x012C}, 	
{ 0x3384, 0x0158}, 	
{ 0x3386, 0x015C}, 	
{ 0x3388, 0x00E6}, 	

{ 0x338A, 0x000F}, 	

{ 0x3276, 0x03FF}, 	
{ 0x32B2, 0x0000}, 	
{ 0x3210, 0x0000}, 	
{ 0x3226, 0x0FFE}, 	
{ 0x3228, 0x0FFF}, 	

{ 0x305E, 0x1a20}, 	
{ 0x32D4, 0x0080}, 	
{ 0x32D6, 0x0080}, 	
{ 0x32D8, 0x0080}, 	
{ 0x32DA, 0x0080}, 	
{ 0x32DC, 0x0080}, 	

{ 0x3C00, 0x4000}, 	
{ 0x3C00, 0x4001}, 	
{ 0x3C40, 0x783C}, 	

{ 0x0032, 0x01D8}, 	


{ 0x301A, 0x10F0}, 	
{0x4334, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_NOT_EQUAL,0xFFFF},

	

{ 0x3C42, 0x1800}, 	
{ 0x3C42, 0x0000}, 	
{ 0x4322, 0xF0D0}, 	
{ 0x3C06, 0x0001}, 	

{ 0x0014, 0x2045}, 	
{ 0x3C46, 0x096A}, 	
{ 0x3C40, 0xAC3C}, 	
{ 0x4312, 0x009A}, 	
{ 0x0012, 0x0090}, 	
{ 0x3C4E, 0x0F00}, 	
{ 0x3C50, 0x0B06}, 	
{ 0x3C52, 0x0D01}, 	
{ 0x3C54, 0x071E}, 	
{ 0x3C56, 0x0006}, 	
{ 0x3C58, 0x0A0C}, 	
{ 0x001A, 0x0014}, 	
{ 0x0010, 0x0341}, 	
{ 0x002A, 0x7F7D}, 	
{ 0x0014, 0x2047}, 	

{0x0014, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_EQUAL,0x8000},


{ 0x0014, 0xA046}, 	

{ 0x3002, 0x001C}, 	
{ 0x3004, 0x0020}, 	
{ 0x3006, 0x045F}, 	
{ 0x3008, 0x07A7}, 	
{ 0x300A, 0x049D}, 	
{ 0x300C, 0x0C48}, 	
{ 0x3010, 0x00D4}, 	
{ 0x3040, 0x8041}, 	
{ 0x3ECE, 0x000A}, 	
{ 0x3256, 0x043F}, 	
{ 0x3254, 0x0002}, 	
{ 0x4304, 0x0788}, 	
{ 0x4306, 0x0440}, 	
{ 0x4300, 0x0001}, 	
{ 0x4302, 0x0210}, 	
{ 0x3C38, 0x0006}, 	
{ 0x3012, 0x049C}, 	
{ 0x3014, 0x04C4}, 	

{ 0x3C40, 0x783E },	
{ 0x3C44, 0x0080}, 	
{ 0x3C44, 0x00C0}, 	
{ 0x3C44, 0x0080}, 	


{ 0x3C40, 0xAC34},	
{0x301C, 0x0102},
{0xffff,50},
#ifndef CONFIG_RAWCHIPII
{ 0x3C40, 0xAC36},	
{0x301C, 0x0},
#else
{0xffff,200},
{0x3C40, 0xAC36},
{0x301C, 0x0002},

{0xffff,300},
{0x3C40, 0xAC34},
{0x301C, 0x0102},

#endif

};

static struct msm_camera_i2c_reg_conf ar0260_recommend_settings_yushanii[] = {

{0x0018, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_EQUAL,0x4000},

{ 0x001A, 0x0005}, 	
{ 0x001C, 0x000C}, 	
{ 0x001A, 0x0014}, 	
{0x001C, 6<<8,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_NOT_EQUAL,0xFF00},


{ 0x0982, 0x0001}, 	
{ 0x098A, 0x6000}, 	
{ 0xE000, 0xC0F1},
{ 0xE002, 0x0C72},
{ 0xE004, 0x0760},
{ 0xE006, 0xD81D},
{ 0xE008, 0x0ABA},
{ 0xE00A, 0x0780},
{ 0xE00C, 0xE080},
{ 0xE00E, 0x0064},
{ 0xE010, 0x0001},
{ 0xE012, 0x0916},
{ 0xE014, 0x0860},
{ 0xE016, 0xD802},
{ 0xE018, 0xD900},
{ 0xE01A, 0x70CF},
{ 0xE01C, 0xFF00},
{ 0xE01E, 0x31B0},
{ 0xE020, 0xB038},
{ 0xE022, 0x1CFC},
{ 0xE024, 0xB388},
{ 0xE026, 0x76CF},
{ 0xE028, 0xFF00},
{ 0xE02A, 0x33CC},
{ 0xE02C, 0x200A},
{ 0xE02E, 0x0F80},
{ 0xE030, 0xFFFF},
{ 0xE032, 0xE048},
{ 0xE034, 0x1CFC},
{ 0xE036, 0xB008},
{ 0xE038, 0x2022},
{ 0xE03A, 0x0F80},
{ 0xE03C, 0x0000},
{ 0xE03E, 0xFC3C},
{ 0xE040, 0x2020},
{ 0xE042, 0x0F80},
{ 0xE044, 0x0000},
{ 0xE046, 0xEA34},
{ 0xE048, 0x1404},
{ 0xE04A, 0x340E},
{ 0xE04C, 0xD801},
{ 0xE04E, 0x71CF},
{ 0xE050, 0xFF00},
{ 0xE052, 0x33CC},
{ 0xE054, 0x190A},
{ 0xE056, 0x801C},
{ 0xE058, 0x208A},
{ 0xE05A, 0x0004},
{ 0xE05C, 0x1964},
{ 0xE05E, 0x8004},
{ 0xE060, 0x0C12},
{ 0xE062, 0x0760},
{ 0xE064, 0xD83C},
{ 0xE066, 0x0E5E},
{ 0xE068, 0x0880},
{ 0xE06A, 0x216F},
{ 0xE06C, 0x003F},
{ 0xE06E, 0xF1FD},
{ 0xE070, 0x0C02},
{ 0xE072, 0x0760},
{ 0xE074, 0xD81E},
{ 0xE076, 0xC0D1},
{ 0xE078, 0x7EE0},
{ 0xE07A, 0x78E0},
{ 0xE07C, 0xC0F1},
{ 0xE07E, 0xE889},
{ 0xE080, 0x70CF},
{ 0xE082, 0xFF00},
{ 0xE084, 0x0000},
{ 0xE086, 0x900E},
{ 0xE088, 0xB8E7},
{ 0xE08A, 0x0F78},
{ 0xE08C, 0xFFC1},
{ 0xE08E, 0xD800},
{ 0xE090, 0xF1F3},

{ 0x098E, 0x0000}, 	
{ 0x098A, 0x5F38}, 	
{ 0x0990, 0xFFFF},
{ 0x0992, 0xE07C},

{ 0x001C, 0x0600}, 	

	{0x001C, 0x3C<<8,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_LESS,0xFF00},

	

{ 0x3E00, 0x042D}, 	
{ 0x3E02, 0x39FF}, 	
{ 0x3E04, 0x49FF}, 	
{ 0x3E06, 0xFFFF}, 	
{ 0x3E08, 0x8071}, 	
{ 0x3E0A, 0x7211}, 	
{ 0x3E0C, 0xE040}, 	
{ 0x3E0E, 0xA840}, 	
{ 0x3E10, 0x4100}, 	
{ 0x3E12, 0x1846}, 	
{ 0x3E14, 0xA547}, 	
{ 0x3E16, 0xAD57}, 	
{ 0x3E18, 0x8149}, 	
{ 0x3E1A, 0x9D49}, 	
{ 0x3E1C, 0x9F46}, 	
{ 0x3E1E, 0x8000}, 	
{ 0x3E20, 0x1842}, 	
{ 0x3E22, 0x4180}, 	
{ 0x3E24, 0x0018}, 	
{ 0x3E26, 0x8149}, 	
{ 0x3E28, 0x9C49}, 	
{ 0x3E2A, 0x9347}, 	
{ 0x3E2C, 0x804D}, 	
{ 0x3E2E, 0x804A}, 	
{ 0x3E30, 0x100C}, 	
{ 0x3E32, 0x8000}, 	
{ 0x3E34, 0x1841}, 	
{ 0x3E36, 0x4280}, 	
{ 0x3E38, 0x0018}, 	
{ 0x3E3A, 0x9710}, 	
{ 0x3E3C, 0x0C80}, 	
{ 0x3E3E, 0x4DA2}, 	
{ 0x3E40, 0x4BA0}, 	
{ 0x3E42, 0x4A00}, 	
{ 0x3E44, 0x1880}, 	
{ 0x3E46, 0x4241}, 	
{ 0x3E48, 0x0018}, 	
{ 0x3E4A, 0xB54B}, 	
{ 0x3E4C, 0x1C00}, 	
{ 0x3E4E, 0x8000}, 	
{ 0x3E50, 0x1C10}, 	
{ 0x3E52, 0x6081}, 	
{ 0x3E54, 0x1580}, 	
{ 0x3E56, 0x7C09}, 	
{ 0x3E58, 0x7000}, 	
{ 0x3E5A, 0x8082}, 	
{ 0x3E5C, 0x7281}, 	
{ 0x3E5E, 0x4C40}, 	
{ 0x3E60, 0x8E4D}, 	
{ 0x3E62, 0x8110}, 	
{ 0x3E64, 0x0CAF}, 	
{ 0x3E66, 0x4D80}, 	
{ 0x3E68, 0x100C}, 	
{ 0x3E6A, 0x8440}, 	
{ 0x3E6C, 0x4C81}, 	
{ 0x3E6E, 0x7C5B}, 	
{ 0x3E70, 0x7000}, 	
{ 0x3E72, 0x8054}, 	
{ 0x3E74, 0x924C}, 	
{ 0x3E76, 0x4078}, 	
{ 0x3E78, 0x4D4F}, 	
{ 0x3E7A, 0x4E98}, 	
{ 0x3E7C, 0x504E}, 	
{ 0x3E7E, 0x4F97}, 	
{ 0x3E80, 0x4F4E}, 	
{ 0x3E82, 0x507C}, 	
{ 0x3E84, 0x7B8D}, 	
{ 0x3E86, 0x4D88}, 	
{ 0x3E88, 0x4E10}, 	
{ 0x3E8A, 0x0940}, 	
{ 0x3E8C, 0x8879}, 	
{ 0x3E8E, 0x5481}, 	
{ 0x3E90, 0x7000}, 	
{ 0x3E92, 0x8082}, 	
{ 0x3E94, 0x7281}, 	
{ 0x3E96, 0x4C40}, 	
{ 0x3E98, 0x8E4D}, 	
{ 0x3E9A, 0x8110}, 	
{ 0x3E9C, 0x0CAF}, 	
{ 0x3E9E, 0x4D80}, 	
{ 0x3EA0, 0x100C}, 	
{ 0x3EA2, 0x8440}, 	
{ 0x3EA4, 0x4C81}, 	
{ 0x3EA6, 0x7C93}, 	
{ 0x3EA8, 0x7000}, 	
{ 0x3EAA, 0x0000}, 	
{ 0x3EAC, 0x0000}, 	
{ 0x3EAE, 0x0000}, 	
{ 0x3EB0, 0x0000}, 	
{ 0x3EB2, 0x0000}, 	
{ 0x3EB4, 0x0000}, 	
{ 0x3EB6, 0x0000}, 	
{ 0x3EB8, 0x0000}, 	
{ 0x3EBA, 0x0000}, 	
{ 0x3EBC, 0x0000}, 	
{ 0x3EBE, 0x0000}, 	
{ 0x3EC0, 0x0000}, 	
{ 0x3EC2, 0x0000}, 	
{ 0x3EC4, 0x0000}, 	
{ 0x3EC6, 0x0000}, 	
{ 0x3EC8, 0x0000}, 	
{ 0x3ECA, 0x0000}, 	

{ 0x30B2, 0xC000}, 	
{ 0x30D4, 0x9400}, 	
{ 0x31C0, 0x0000}, 	
{ 0x316A, 0x8200}, 	
{ 0x316C, 0x8200}, 	
{ 0x3EFE, 0x2808}, 	
{ 0x3EFC, 0x2868}, 	
{ 0x3ED2, 0xD165}, 	
{ 0x3EF2, 0xD165}, 	
{ 0x3ED8, 0x7F1A}, 	
{ 0x3EDA, 0x2828}, 	
{ 0x3EE2, 0x0058}, 	

{ 0x002C, 0x000F}, 	

{ 0x3382, 0x012C}, 	
{ 0x3384, 0x0158}, 	
{ 0x3386, 0x015C}, 	
{ 0x3388, 0x00E6}, 	

{ 0x338A, 0x000F}, 	

{ 0x3276, 0x03FF}, 	
{ 0x32B2, 0x0000}, 	
{ 0x3210, 0x0000}, 	
{ 0x3226, 0x0FFE}, 	
{ 0x3228, 0x0FFF}, 	

{ 0x305E, 0x1a20}, 	
{ 0x32D4, 0x0080}, 	
{ 0x32D6, 0x0080}, 	
{ 0x32D8, 0x0080}, 	
{ 0x32DA, 0x0080}, 	
{ 0x32DC, 0x0080}, 	

{ 0x3C00, 0x4000}, 	
{ 0x3C00, 0x4001}, 	
{ 0x3C40, 0x783C}, 	

{ 0x0032, 0x01D8}, 	


{ 0x301A, 0x10F0}, 	
{0x4334, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_NOT_EQUAL,0xFFFF},

	

{ 0x3C42, 0x1800}, 	
{ 0x3C42, 0x0000}, 	
{ 0x4322, 0xF0D0}, 	
{ 0x3C06, 0x0001}, 	

{ 0x0014, 0x2045}, 	
{ 0x3C46, 0x096A}, 	
{ 0x3C40, 0xAC3C}, 	
{ 0x4312, 0x009A}, 	
{ 0x0012, 0x0090}, 	
{ 0x3C4E, 0x0F00}, 	
{ 0x3C50, 0x0B06}, 	
{ 0x3C52, 0x0D01}, 	
{ 0x3C54, 0x071E}, 	
{ 0x3C56, 0x0006}, 	
{ 0x3C58, 0x0A0C}, 	
{ 0x001A, 0x0014}, 	
{ 0x0010, 0x0341}, 	
{ 0x002A, 0x7F7D}, 	
{ 0x0014, 0x2047}, 	

{0x0014, 0,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_POLL_EQUAL,0x8000},


{ 0x0014, 0xA046}, 	

{ 0x3002, 0x001C}, 	
{ 0x3004, 0x0020}, 	
{ 0x3006, 0x045F}, 	
{ 0x3008, 0x07A7}, 	
{ 0x300A, 0x049D}, 	
{ 0x300C, 0x0C48}, 	
{ 0x3010, 0x00D4}, 	
{ 0x3040, 0x8041}, 	
{ 0x3ECE, 0x000A}, 	
{ 0x3256, 0x043F}, 	
{ 0x3254, 0x0002}, 	
{ 0x4304, 0x0788}, 	
{ 0x4306, 0x0440}, 	
{ 0x4300, 0x0001}, 	
{ 0x4302, 0x0210}, 	
{ 0x3C38, 0x0006}, 	
{ 0x3012, 0x0172}, 	
{ 0x3014, 0x04C4}, 	

{ 0x3C44, 0x0080}, 	
{ 0x3C44, 0x00C0}, 	
{ 0x3C44, 0x0080}, 	

#if 1
{0x301C, 0x0102},
{0x3C40, 0xAC34},	
{0xffff,50},
#endif

#if 0
{ 0x3C40, 0xAC36},	
{0x301C, 0x0},
#else
{0x301C, 0x0002},
{0x3C40, 0xAC36},

#if 0
{0xffff,50},
{0x301C, 0x0102},
{0x3C40, 0xAC34},
#endif
#endif

};

static struct msm_sensor_output_info_t ar0260_dimensions[] = {
	{
		.x_output = 0x0788, 
		.y_output = 0x0440, 
		.line_length_pclk = 0x0C48, 
		.frame_length_lines = 0x049D, 
		.vt_pixel_clk = 111428570,
		.op_pixel_clk = 78000000,
		.binning_factor = 1,
		
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x0788-1,
		.y_addr_end = 0x0440-1,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
		.is_hdr = 0,
		
	},
	{
		.x_output = 0x0788,
		.y_output = 0x0440,
		.line_length_pclk = 0x0C48,
		.frame_length_lines = 0x049D,
		.vt_pixel_clk = 111428570,
		.op_pixel_clk = 78000000,
		.binning_factor = 1,

		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x0788-1,
		.y_addr_end = 0x0440-1,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
		.is_hdr = 0,

	},
};

static struct msm_sensor_output_info_t ar0260_dimensions_yushanii[] = {
	{
		.x_output = 0x0788, 
		.y_output = 0x0440, 
		.line_length_pclk = 0x0C48, 
		.frame_length_lines = 0x049D, 
		.vt_pixel_clk = 111428570,
		.op_pixel_clk = 78000000,
		.binning_factor = 1,

		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x0788-1,
		.y_addr_end = 0x0440-1,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
		.is_hdr = 0,

	},
	{
		.x_output = 0x0788,
		.y_output = 0x0440,
		.line_length_pclk = 0x0C48,
		.frame_length_lines = 0x049D,
		.vt_pixel_clk = 111428570,
		.op_pixel_clk = 78000000,
		.binning_factor = 1,

		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x0788-1,
		.y_addr_end = 0x0440-1,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
		.is_hdr = 0,

	},
};

static struct msm_sensor_output_reg_addr_t ar0260_reg_addr = {
	.x_output = 0x4304,
	.y_output = 0x4306,
	.line_length_pclk = 0x300C,
	.frame_length_lines = 0x300A,
};
static struct msm_sensor_exp_gain_info_t ar0260_exp_gain_info = {
	.coarse_int_time_addr = 0x3012,
	.global_gain_addr = 0x305E,
	.vert_offset = 4,
	.min_vert = 4,  
};


#else


static struct msm_camera_i2c_reg_conf ar0260_recommend_settings[] = {
	
	{ 0x098E, 0xCA12},	
	
	{ 0xCA12, 0x01,MSM_CAMERA_I2C_BYTE_DATA,MSM_CAMERA_I2C_CMD_WRITE},	
	{ 0xCA13, 0x00,MSM_CAMERA_I2C_BYTE_DATA,MSM_CAMERA_I2C_CMD_WRITE},	
	
	{ 0xCA14, 0x0120},	
	{ 0xCA16, 0x1090},	
	{ 0xCA18, 0x7F7D},	
	{ 0xCA1A, 0x000F},	
	
	
	{ 0xC800, 0x001C},	
	{ 0xC802, 0x0020},	
	{ 0xC804, 0x045F},	
	{ 0xC806, 0x07A7},	
	{ 0xC80E, 0x0336},	
	{ 0xC810, 0x0A6E},	
	{ 0xC81E, 0x300A},	
	{ 0xC820, 0x0495},	
	{ 0xC812, 0x0495},	
	{ 0xC814, 0x0C2E},	
	{ 0xC816, 0x00D4},	
	{ 0xC818, 0x043F},	
	{ 0xC830, 0x0000},	
	{ 0xC86C, 0x0788},	
	{ 0xC86E, 0x0440},	
	{ 0xC830, 0x0002},	
	{ 0x3E00, 0x042D},	
	{ 0x3E02, 0x39FF},	
	{ 0x3E04, 0x49FF},	
	{ 0x3E06, 0xFFFF},	
	{ 0x3E08, 0x8071},	
	{ 0x3E0A, 0x7211},	
	{ 0x3E0C, 0xE040},	
	{ 0x3E0E, 0xA840},	
	{ 0x3E10, 0x4100},	
	{ 0x3E12, 0x1846},	
	{ 0x3E14, 0xA547},	
	{ 0x3E16, 0xAD57},	
	{ 0x3E18, 0x8149},	
	{ 0x3E1A, 0x9D49},	
	{ 0x3E1C, 0x9F46},	
	{ 0x3E1E, 0x8000},	
	{ 0x3E20, 0x1842},	
	{ 0x3E22, 0x4180},	
	{ 0x3E24, 0x0018},	
	{ 0x3E26, 0x8149},	
	{ 0x3E28, 0x9C49},	
	{ 0x3E2A, 0x9347},	
	{ 0x3E2C, 0x804D},	
	{ 0x3E2E, 0x804A},	
	{ 0x3E30, 0x100C},	
	{ 0x3E32, 0x8000},	
	{ 0x3E34, 0x1841},	
	{ 0x3E36, 0x4280},	
	{ 0x3E38, 0x0018},	
	{ 0x3E3A, 0x9710},	
	{ 0x3E3C, 0x0C80},	
	{ 0x3E3E, 0x4DA2},	
	{ 0x3E40, 0x4BA0},	
	{ 0x3E42, 0x4A00},	
	{ 0x3E44, 0x1880},	
	{ 0x3E46, 0x4241},	
	{ 0x3E48, 0x0018},	
	{ 0x3E4A, 0xB54B},	
	{ 0x3E4C, 0x1C00},	
	{ 0x3E4E, 0x8000},	
	{ 0x3E50, 0x1C10},	
	{ 0x3E52, 0x6081},	
	{ 0x3E54, 0x1580},	
	{ 0x3E56, 0x7C09},	
	{ 0x3E58, 0x7000},	
	{ 0x3E5A, 0x8082},	
	{ 0x3E5C, 0x7281},	
	{ 0x3E5E, 0x4C40},	
	{ 0x3E60, 0x8E4D},	
	{ 0x3E62, 0x8110},	
	{ 0x3E64, 0x0CAF},	
	{ 0x3E66, 0x4D80},	
	{ 0x3E68, 0x100C},	
	{ 0x3E6A, 0x8440},	
	{ 0x3E6C, 0x4C81},	
	{ 0x3E6E, 0x7C5B},	
	{ 0x3E70, 0x7000},	
	{ 0x3E72, 0x8054},	
	{ 0x3E74, 0x924C},	
	{ 0x3E76, 0x4078},	
	{ 0x3E78, 0x4D4F},	
	{ 0x3E7A, 0x4E98},	
	{ 0x3E7C, 0x504E},	
	{ 0x3E7E, 0x4F97},	
	{ 0x3E80, 0x4F4E},	
	{ 0x3E82, 0x507C},	
	{ 0x3E84, 0x7B8D},	
	{ 0x3E86, 0x4D88},	
	{ 0x3E88, 0x4E10},	
	{ 0x3E8A, 0x0940},	
	{ 0x3E8C, 0x8879},	
	{ 0x3E8E, 0x5481},	
	{ 0x3E90, 0x7000},	
	{ 0x3E92, 0x8082},	
	{ 0x3E94, 0x7281},	
	{ 0x3E96, 0x4C40},	
	{ 0x3E98, 0x8E4D},	
	{ 0x3E9A, 0x8110},	
	{ 0x3E9C, 0x0CAF},	
	{ 0x3E9E, 0x4D80},	
	{ 0x3EA0, 0x100C},	
	{ 0x3EA2, 0x8440},	
	{ 0x3EA4, 0x4C81},	
	{ 0x3EA6, 0x7C93},	
	{ 0x3EA8, 0x7000},	
	{ 0x3EAA, 0x0000},	
	{ 0x3EAC, 0x0000},	
	{ 0x3EAE, 0x0000},	
	{ 0x3EB0, 0x0000},	
	{ 0x3EB2, 0x0000},	
	{ 0x3EB4, 0x0000},	
	{ 0x3EB6, 0x0000},	
	{ 0x3EB8, 0x0000},	
	{ 0x3EBA, 0x0000},	
	{ 0x3EBC, 0x0000},	
	{ 0x3EBE, 0x0000},	
	{ 0x3EC0, 0x0000},	
	{ 0x3EC2, 0x0000},	
	{ 0x3EC4, 0x0000},	
	{ 0x3EC6, 0x0000},	
	{ 0x3EC8, 0x0000},	
	{ 0x3ECA, 0x0000},	
	{ 0x30B2, 0xC000},	
	{ 0x30D4, 0x9400},	
	{ 0x31C0, 0x0000},	
	{ 0x316A, 0x8200},	
	{ 0x316C, 0x8200},	
	{ 0x3EFE, 0x2808},	
	{ 0x3EFC, 0x2868},	
	{ 0x3ED2, 0xD165},	
	{ 0x3EF2, 0xD165},	
	{ 0x3ED8, 0x7F1A},	
	{ 0x3EDA, 0x2828},	
	{ 0x3EE2, 0x0058},	
	{ 0xC984, 0x012C},	
	{ 0xC988, 0x0158},	
	{ 0xC98C, 0x012C},	
	{ 0xC990, 0x0158},	
	{ 0xC994, 0x012C},	
	{ 0xC998, 0x0158},	
	{ 0xC986, 0x015C},	
	{ 0xC98A, 0x00E6},	
	{ 0xC98E, 0x015C},	
	{ 0xC992, 0x00E6},	
	{ 0xC996, 0x015C},	
	{ 0xC99A, 0x00E6},	
	{ 0xC97E, 0x000F},	
	{ 0xC980, 0x000F},	
	{ 0x4300, 0x0000},	
	{ 0xCA1C, 0x8040},	
	{ 0x001E, 0x0777},	
	{ 0xC870, 0x4090},	
	{ 0xC870, 0x4090},	
	{ 0xCA1C, 0x8041},	
	{ 0xDC00, 0x28,MSM_CAMERA_I2C_BYTE_DATA,MSM_CAMERA_I2C_CMD_WRITE},	
	{ 0x0080, 0x8002},	
	
	
	
	{ 0x0080, 0x2,MSM_CAMERA_I2C_UNSET_WORD_MASK,MSM_CAMERA_I2C_CMD_POLL},	
	
	{ 0x001A, 0x0000},	
	{ 0x3276, 0x03FF},	
	{ 0x32B2, 0x0000},	
	{ 0x3210, 0x0000},	
	{ 0x3226, 0x0FFE},	
	{ 0x3228, 0x0FFF},	
	{ 0x32D4, 0x0080},	
	{ 0x32D6, 0x0080},	
	{ 0x32D8, 0x0080},	
	{ 0x32DA, 0x0080},	
	{ 0x32DC, 0x0080},	
	{ 0x3330, 0x0100},	
	{ 0x4302, 0x0210},	
	{ 0x4300, 0x0001},	
	{ 0x3012, 0x0DAC},
	{ 0x305E, 0x1820},
	{ 0x3C40, 0xAC34},
	{ 0x3C40, 0xAC36},


};

static struct msm_sensor_output_info_t ar0260_dimensions[] = {
	{
		.x_output = 0x0788,
		.y_output = 0x0440,
		.line_length_pclk = 0x0C2E,
		.frame_length_lines = 0x0495,
		.vt_pixel_clk = 109714286,
		.op_pixel_clk = 76800000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x07A7-0x0020,
		.y_addr_end = 0x0440-1,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
		.is_hdr = 0,
	},
	{
		.x_output = 0x0788,
		.y_output = 0x0440,
		.line_length_pclk = 0x0C2E,
		.frame_length_lines = 0x0495,
		.vt_pixel_clk = 109714286,
		.op_pixel_clk = 76800000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x07A7-0x0020,
		.y_addr_end = 0x0440-1,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
		.is_hdr = 0,
	},
};

static struct msm_sensor_output_reg_addr_t ar0260_reg_addr = {
	.x_output = 0xC86C,
	.y_output = 0xC86E,
	.line_length_pclk = 0xC814,
	.frame_length_lines = 0xC812,
};
static struct msm_sensor_exp_gain_info_t ar0260_exp_gain_info = {
	.coarse_int_time_addr = 0x3012,
	.global_gain_addr = 0x305E,
	.vert_offset = 4,
	.min_vert = 4,  
};
#endif


static struct v4l2_subdev_info ar0260_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	
};

static struct msm_camera_i2c_conf_array ar0260_init_conf[] = {
	{&ar0260_recommend_settings[0],
	ARRAY_SIZE(ar0260_recommend_settings), 0, MSM_CAMERA_I2C_WORD_DATA}
};

static struct msm_camera_i2c_conf_array ar0260_init_conf_yushanii[] = {
	{&ar0260_recommend_settings_yushanii[0],
	ARRAY_SIZE(ar0260_recommend_settings_yushanii), 0, MSM_CAMERA_I2C_WORD_DATA}
};

static struct msm_camera_i2c_conf_array ar0260_confs[] = {
	{&ar0260_snap_settings[0],
	ARRAY_SIZE(ar0260_snap_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
	{&ar0260_prev_settings[0],
	ARRAY_SIZE(ar0260_prev_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
};




static struct msm_camera_csid_vc_cfg ar0260_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params ar0260_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 1,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = ar0260_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 1,
		.settle_cnt = 20,
	},
};

static struct msm_camera_csi2_params *ar0260_csi_params_array[] = {
	&ar0260_csi_params,
	&ar0260_csi_params,
};




static struct msm_sensor_id_info_t ar0260_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x4581,
};



#if 0	

static struct i2c_client *ar0260_client;

#define MAX_I2C_RETRIES 20
#define CHECK_STATE_TIME 100

enum ar0260_width {
	WORD_LEN,
	BYTE_LEN
};


static int i2c_transfer_retry(struct i2c_adapter *adap,
			struct i2c_msg *msgs,
			int len)
{
	int i2c_retry = 0;
	int ns; 

	while (i2c_retry++ < MAX_I2C_RETRIES) {
		ns = i2c_transfer(adap, msgs, len);
		if (ns == len)
			break;
		pr_err("%s: try %d/%d: i2c_transfer sent: %d, len %d\n",
			__func__,
			i2c_retry, MAX_I2C_RETRIES, ns, len);
		msleep(10);
	}

	return ns == len ? 0 : -EIO;
}

static int ar0260_i2c_txdata(unsigned short saddr,
				  unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
		 .addr = saddr,
		 .flags = 0,
		 .len = length,
		 .buf = txdata,
		 },
	};

	if (i2c_transfer_retry(ar0260_client->adapter, msg, 1) < 0) {
		pr_err("ar0260_i2c_txdata failed\n");
		return -EIO;
	}

	return 0;
}

static int ar0260_i2c_write(unsigned short saddr,
				 unsigned short waddr, unsigned short wdata,
				 enum ar0260_width width)
{
	int rc = -EIO;
	unsigned char buf[4];
	memset(buf, 0, sizeof(buf));

	switch (width) {
	case WORD_LEN:{
			

			buf[0] = (waddr & 0xFF00) >> 8;
			buf[1] = (waddr & 0x00FF);
			buf[2] = (wdata & 0xFF00) >> 8;
			buf[3] = (wdata & 0x00FF);

			rc = ar0260_i2c_txdata(saddr, buf, 4);
		}
		break;

	case BYTE_LEN:{
			

			buf[0] = waddr;
			buf[1] = wdata;
			rc = ar0260_i2c_txdata(saddr, buf, 2);
		}
		break;

	default:
		break;
	}

	if (rc < 0)
		pr_info("i2c_write failed, addr = 0x%x, val = 0x%x!\n",
		     waddr, wdata);

	return rc;
}

#if 0
static int ar0260_i2c_write_table(struct ar0260_i2c_reg_conf
				       *reg_conf_tbl, int num_of_items_in_table)
{
	int i;
	int rc = -EIO;

	for (i = 0; i < num_of_items_in_table; i++) {
		rc = ar0260_i2c_write(ar0260_client->addr,
				       reg_conf_tbl->waddr, reg_conf_tbl->wdata,
				       reg_conf_tbl->width);
		if (rc < 0) {
		pr_err("%s: num_of_items_in_table=%d\n", __func__,
			num_of_items_in_table);
			break;
		}
		if (reg_conf_tbl->mdelay_time != 0)
			mdelay(reg_conf_tbl->mdelay_time);
		reg_conf_tbl++;
	}

	return rc;
}
#endif

static int ar0260_i2c_rxdata(unsigned short saddr,
			      unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
		{
		 .addr = saddr,
		 .flags = 0,
		 .len = 2,  
		 .buf = rxdata,
		 },
		{
		 .addr = saddr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = rxdata,
		 },
	};

	if (i2c_transfer_retry(ar0260_client->adapter, msgs, 2) < 0) {
		pr_err("ar0260_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int32_t ar0260_i2c_read_w(unsigned short saddr, unsigned short raddr,
	unsigned short *rdata)
{
	int32_t rc = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	buf[0] = (raddr & 0xFF00)>>8;
	buf[1] = (raddr & 0x00FF);

	rc = ar0260_i2c_rxdata(saddr, buf, 2);
	if (rc < 0)
		return rc;

	*rdata = buf[0] << 8 | buf[1];

	if (rc < 0)
		CDBG("ar0260_i2c_read_w failed!\n");

	return rc;
}

#if 0
static int ar0260_i2c_read(unsigned short saddr,
				unsigned short raddr, unsigned char *rdata)
{
	int rc = 0;
	unsigned char buf[1];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));
	buf[0] = raddr;
	rc = ar0260_i2c_rxdata(saddr, buf, 1);
	if (rc < 0)
		return rc;
	*rdata = buf[0];
	if (rc < 0)
		pr_info("ar0260_i2c_read failed!\n");

	return rc;
}
#endif
static int ar0260_i2c_write_bit(unsigned short saddr, unsigned short raddr,
unsigned short bit, unsigned short state)
{
	int rc;
	unsigned short check_value;
	unsigned short check_bit;

	if (state)
		check_bit = 0x0001 << bit;
	else
		check_bit = 0xFFFF & (~(0x0001 << bit));
	pr_info("ar0260_i2c_write_bit check_bit:0x%4x", check_bit);
	rc = ar0260_i2c_read_w(saddr, raddr, &check_value);
	if (rc < 0)
	  return rc;

	pr_info("%s: ar0260: 0x%4x reg value = 0x%4x\n", __func__,
		raddr, check_value);
	if (state)
		check_value = (check_value | check_bit);
	else
		check_value = (check_value & check_bit);

	pr_info("%s: ar0260: Set to 0x%4x reg value = 0x%4x\n", __func__,
		raddr, check_value);

	rc = ar0260_i2c_write(saddr, raddr, check_value,
		WORD_LEN);
	return rc;
}

static int ar0260_i2c_check_bit(unsigned short saddr, unsigned short raddr,
unsigned short bit, int check_state)
{
	int k;
	unsigned short check_value;
	unsigned short check_bit;
	check_bit = 0x0001 << bit;
	for (k = 0; k < CHECK_STATE_TIME; k++) {
		ar0260_i2c_read_w(ar0260_client->addr,
			      raddr, &check_value);
		if (check_state) {
			if ((check_value & check_bit))
			break;
		} else {
			if (!(check_value & check_bit))
			break;
		}
		msleep(1);
	}
	if (k == CHECK_STATE_TIME) {
		pr_err("%s failed addr:0x%2x data check_bit:0x%2x",
			__func__, raddr, check_bit);
		return -1;
	}
	return 1;
}


#endif	

static int ar0260_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	if (data->sensor_platform_info)
		ar0260_s_ctrl.mirror_flip = data->sensor_platform_info->mirror_flip;

	return 0;

#if 0	
	if (ar0260_s_ctrl.sensor_i2c_client && ar0260_s_ctrl.sensor_i2c_client->client)
		ar0260_client = ar0260_s_ctrl.sensor_i2c_client->client;
	#if 1	
	ar0260_i2c_check_bit(0, 0, 0, 0);
	ar0260_i2c_write_bit(0, 0, 0, 0);
	#endif
#endif	
}

static const char *ar0260Vendor = "aptina";
static const char *ar0260NAME = "ar0260";
static const char *ar0260Size = "2.1M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", ar0260Vendor, ar0260NAME, ar0260Size);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);

static struct kobject *android_ar0260;

static int ar0260_sysfs_init(void)
{
	int ret ;
	pr_info("ar0260:kobject creat and add\n");
	android_ar0260 = kobject_create_and_add("android_camera2", NULL);
	if (android_ar0260 == NULL) {
		pr_info("ar0260_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("ar0260:sysfs_create_file\n");
	ret = sysfs_create_file(android_ar0260, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("ar0260_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_ar0260);
	}

	return 0 ;
}

static struct msm_camera_i2c_client ar0260_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

#define CAM2_RSTz       (2)
#define CAM_PIN_GPIO_CAM2_RSTz	CAM2_RSTz

int32_t ar0260_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc;
	struct sensor_cfg_data cdata;  
	struct msm_camera_sensor_info *sdata = NULL;
	pr_info("%s\n", __func__);


#if 1		
		rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "ar0260");
		if (rc < 0)
			pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
		else {
			gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
			gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
		}
		mdelay(10); 
#endif

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_err("%s: s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	if (sdata->camera_power_on == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}

	rc = sdata->camera_power_on();
	if (rc < 0) {
		pr_err("%s failed to enable power\n", __func__);
		return rc;
	}

	if (!sdata->use_rawchip && (sdata->htc_image != HTC_CAMERA_IMAGE_YUSHANII_BOARD)) {
		rc = msm_camio_clk_enable(sdata,CAMIO_CAM_MCLK_CLK);
		if (rc < 0) {
			return rc;
		}
	}

#ifdef CONFIG_RAWCHIPII
    if (sdata->htc_image == HTC_CAMERA_IMAGE_YUSHANII_BOARD) {
    	Ilp0100_enableIlp0100SensorClock(SENSOR_1);
        mdelay(35);	
	}
#endif

#if 0	
	mdelay(10);	
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "ar0260");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	else {
		gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 1);
		gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	}

	mdelay(50);	
#else

	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "ar0260");
	pr_info("reset pin gpio_request,%d\n", CAM_PIN_GPIO_CAM2_RSTz);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
		
	}
	mdelay(1); 
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 1);
	mdelay(1); 
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);


	
	gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	mdelay(25); 
#endif

	ar0260_sensor_open_init(sdata);
	if (s_ctrl->func_tbl->sensor_i2c_read_fuseid == NULL) {
		rc = -EFAULT;
		return rc;
	}
	rc = s_ctrl->func_tbl->sensor_i2c_read_fuseid(&cdata, s_ctrl);
	if (rc < 0) {
		return rc;
	}
	pr_info("%s end\n", __func__);

	return 0;  
}

int32_t ar0260_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc;
	struct msm_camera_sensor_info *sdata = NULL;
	pr_info("%s\n", __func__);

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_err("%s: s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	if (sdata->camera_power_off == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}

#if 0	
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "ar0260");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	else {
		gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
		gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	}
	mdelay(10); 
#endif

	if (!sdata->use_rawchip && (sdata->htc_image != HTC_CAMERA_IMAGE_YUSHANII_BOARD)) {
		msm_camio_clk_disable(sdata,CAMIO_CAM_MCLK_CLK);
	}
	rc = sdata->camera_power_off();
	if (rc < 0) {
		pr_err("%s failed to disable power\n", __func__);
		return rc;
	}
	return 0;  
}

int32_t ar0260_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	rc = 0;
	
	
	int max_probe_count = 1;
	int probe_count = 0;

	pr_info("%s\n", __func__);

sensor_probe_retry:
	rc = msm_sensor_i2c_probe(client, id);
	if(rc >= 0)
		ar0260_sysfs_init();
	else {
		
		probe_count++;
		if(probe_count < max_probe_count) {
			pr_info("%s  apply sensor probe retry mechanism , probe_count=%d\n", __func__, probe_count);
			goto sensor_probe_retry;
		}
	}

	pr_info("%s: rc(%d)\n", __func__, rc);
	return rc;
}

static const struct i2c_device_id ar0260_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ar0260_s_ctrl},
	{ }
};

static struct i2c_driver ar0260_i2c_driver = {
	.id_table = ar0260_i2c_id,
	.probe  = ar0260_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static int ar0260_read_fuseid(struct sensor_cfg_data *cdata,
	struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	int i;
	uint16_t read_data = 0;
	uint16_t id_addr[2] = {0x31F6,0x31F4};
	static uint16_t id_data[2] = {0,0};
	static int first=true;
	
	struct msm_camera_i2c_reg_conf cmd[]= {
		{0x301A, 0x10D8,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
		{0x3134, 0xCD95,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
		{0x304C, 0x3000,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
		{0x304A, 0x0200,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},
		{0x304A, 0x0210,MSM_CAMERA_I2C_WORD_DATA,MSM_CAMERA_I2C_CMD_WRITE},

		
	};
	
	pr_info("%s called\n", __func__);


	if (!first) {
		cdata->cfg.fuse.fuse_id_word1 = 0;
		cdata->cfg.fuse.fuse_id_word2 = 0;
		cdata->cfg.fuse.fuse_id_word3 = id_data[0];
		cdata->cfg.fuse.fuse_id_word4 = id_data[1];

		pr_info("ar0260: catched fuse->fuse_id : 0x%x 0x%x 0x%x 0x%x\n", 
			cdata->cfg.fuse.fuse_id_word1,
			cdata->cfg.fuse.fuse_id_word2,
			cdata->cfg.fuse.fuse_id_word3,
			cdata->cfg.fuse.fuse_id_word4);		
		return 0;	
	}
	first=false;

	rc = msm_camera_i2c_write_tbl(s_ctrl->sensor_i2c_client, cmd, ARRAY_SIZE (cmd), 0);
	if (rc < 0) {
		pr_err("%s: msm_camera_i2c_write failed\n", __func__);
		return 0;
	}
	msleep(50);
	
	for (i = 0; i < ARRAY_SIZE(id_addr); i++) {
		rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, id_addr[i], &read_data, MSM_CAMERA_I2C_WORD_DATA);
		if (rc < 0) {
			pr_err("%s: msm_camera_i2c_read 0x%x failed\n", __func__, id_addr[i]);
			break;
		}
		id_data[i] = read_data;
	}
	
	if (i==ARRAY_SIZE(id_data)) {
		cdata->cfg.fuse.fuse_id_word1 = 0;
		cdata->cfg.fuse.fuse_id_word2 = 0;
		cdata->cfg.fuse.fuse_id_word3 = id_data[0];
		cdata->cfg.fuse.fuse_id_word4 = id_data[1];
	
		pr_info("ar0260: fuse->fuse_id : 0x%x 0x%x 0x%x 0x%x\n", 
			cdata->cfg.fuse.fuse_id_word1,
			cdata->cfg.fuse.fuse_id_word2,
			cdata->cfg.fuse.fuse_id_word3,
			cdata->cfg.fuse.fuse_id_word4);
	}
	return 0;

}

int32_t aptina_write_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		int mode, uint16_t gain, uint16_t dig_gain, uint32_t line) 
{
	CDBG("%s: called\n", __func__);
	if(line > s_ctrl->sensor_exp_gain_info->sensor_max_linecount)
		line = s_ctrl->sensor_exp_gain_info->sensor_max_linecount;

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, line,
		MSM_CAMERA_I2C_WORD_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain,
		MSM_CAMERA_I2C_WORD_DATA);
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}

static int __init msm_sensor_init_module(void)
{
	pr_info("%s\n", __func__);
	return i2c_add_driver(&ar0260_i2c_driver);
}

static struct v4l2_subdev_core_ops ar0260_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ar0260_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ar0260_subdev_ops = {
	.core = &ar0260_subdev_core_ops,
	.video  = &ar0260_subdev_video_ops,
};

static struct msm_sensor_fn_t ar0260_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain_ex = aptina_write_exp_gain, 
	.sensor_write_snapshot_exp_gain_ex = aptina_write_exp_gain, 
	.sensor_setting = msm_sensor_setting_parallel,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = ar0260_power_up,
	.sensor_power_down = ar0260_power_down,
	.sensor_i2c_read_fuseid = ar0260_read_fuseid,
};

static struct msm_sensor_reg_t ar0260_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = ar0260_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ar0260_start_settings),
	.stop_stream_conf = ar0260_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ar0260_stop_settings),


	.start_stream_conf_yushanii = ar0260_start_settings_yushanii,
	.start_stream_conf_size_yushanii = ARRAY_SIZE(ar0260_start_settings_yushanii),
	.stop_stream_conf_yushanii = ar0260_stop_settings_yushanii,
	.stop_stream_conf_size_yushanii = ARRAY_SIZE(ar0260_stop_settings_yushanii),

	.group_hold_on_conf = ar0260_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(ar0260_groupon_settings),
	.group_hold_off_conf = ar0260_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(ar0260_groupoff_settings),
	.init_settings = &ar0260_init_conf[0],
	.init_size = ARRAY_SIZE(ar0260_init_conf),

	.init_settings_yushanii = &ar0260_init_conf_yushanii[0],
	.init_size_yushanii = ARRAY_SIZE(ar0260_init_conf_yushanii),

	.mode_settings = &ar0260_confs[0],
	.output_settings = &ar0260_dimensions[0],

	.output_settings_yushanii = &ar0260_dimensions_yushanii[0],


	.num_conf = ARRAY_SIZE(ar0260_confs),
};

static struct msm_sensor_ctrl_t ar0260_s_ctrl = {
	.msm_sensor_reg = &ar0260_regs,
	.sensor_i2c_client = &ar0260_sensor_i2c_client,
	.sensor_i2c_addr = 0x90,
	.sensor_output_reg_addr = &ar0260_reg_addr,
	.sensor_id_info = &ar0260_id_info,
	.sensor_exp_gain_info = &ar0260_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &ar0260_csi_params_array[0],
	.msm_sensor_mutex = &ar0260_mut,
	.sensor_i2c_driver = &ar0260_i2c_driver,
	.sensor_v4l2_subdev_info = ar0260_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ar0260_subdev_info),
	.sensor_v4l2_subdev_ops = &ar0260_subdev_ops,
	.func_tbl = &ar0260_func_tbl,
	.sensor_first_mutex = &ar0260_sensor_init_mut,  
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Aptina 2.1 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
