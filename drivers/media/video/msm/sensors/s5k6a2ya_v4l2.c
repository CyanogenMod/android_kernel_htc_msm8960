#include "msm_sensor.h"
#define SENSOR_NAME "s5k6a2ya"
#define PLATFORM_DRIVER_NAME "msm_camera_s5k6a2ya"
#define s5k6a2ya_obj s5k6a2ya_##obj

#ifdef CONFIG_RAWCHIPII
#include "yushanII.h"
#include "ilp0100_ST_api.h"
#include "ilp0100_customer_sensor_config.h"
#endif

#define S5K6A2YA_REG_ORIENTATION 0x0216
#define S5K6A2YA_ORIENTATION_NORMAL_MODE 0x00
#define S5K6A2YA_ORIENTATION_MIRROR 0x01
#define S5K6A2YA_ORIENTATION_FLIP 0x02
#define S5K6A2YA_ORIENTATION_MIRROR_FLIP 0x03


DEFINE_MUTEX(s5k6a2ya_mut);
DEFINE_MUTEX(s5k6a2ya_sensor_init_mut);
static struct msm_sensor_ctrl_t s5k6a2ya_s_ctrl;

static struct msm_camera_i2c_reg_conf s5k6a2ya_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k6a2ya_start_settings_yushanii[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k6a2ya_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k6a2ya_stop_settings_yushanii[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k6a2ya_groupon_settings[] = {
	{0x3E90, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k6a2ya_groupoff_settings[] = {
	{0x3E90, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k6a2ya_prev_settings[] = {
	
	{0x0340, 0x04}, 
	{0x0341, 0xD6}, 
	{0x0342, 0x06}, 
	{0x0343, 0x7E}, 

	
	{0x0344, 0x00}, 
	{0x0345, 0x00}, 
	{0x0346, 0x00}, 
	{0x0347, 0x00}, 
	{0x0348, 0x05}, 
	{0x0349, 0xBF}, 
	{0x034A, 0x04}, 
	{0x034B, 0x4F}, 

	
	{0x034C, 0x05}, 
	{0x034D, 0xC0}, 
	{0x034E, 0x04}, 
	{0x034F, 0x50}, 

	
	{0x0381, 0x01}, 
	{0x0383, 0x01}, 
	{0x0385, 0x01}, 
	{0x0387, 0x01}, 

	
	{0x0204, 0x00}, 
	{0x0205, 0x20}, 

	
	{0x0220, 0x01}, 
	{0x0221, 0xF4},
	{0x0222, 0x02}, 
	{0x0223, 0x6F},
};

static struct msm_camera_i2c_reg_conf s5k6a2ya_snap_settings[] = {
	
	{0x0340, 0x04}, 
	{0x0341, 0xD6}, 
	{0x0342, 0x06}, 
	{0x0343, 0x7E}, 

	
	{0x0344, 0x00}, 
	{0x0345, 0x00}, 
	{0x0346, 0x00}, 
	{0x0347, 0x00}, 
	{0x0348, 0x05}, 
	{0x0349, 0xBF}, 
	{0x034A, 0x04}, 
	{0x034B, 0x4F}, 

	
	{0x034C, 0x05}, 
	{0x034D, 0xC0}, 
	{0x034E, 0x04}, 
	{0x034F, 0x50}, 

	
	{0x0381, 0x01}, 
	{0x0383, 0x01}, 
	{0x0385, 0x01}, 
	{0x0387, 0x01}, 

	
	{0x0204, 0x00}, 
	{0x0205, 0x20}, 

	
	{0x0220, 0x01}, 
	{0x0221, 0xF4},
	{0x0222, 0x02}, 
	{0x0223, 0x6F},
};

static struct msm_camera_i2c_reg_conf s5k6a2ya_recommend_settings[] = {
	
	
	
	{0x303E, 0x20}, 
	{0x303F, 0x10}, 
	{0x3040, 0x40}, 
	{0x3041, 0x10}, 
	{0x3310, 0x0C}, /* [3:2] nop_flob : must be written before streaming on */
	{0x3074, 0x0E}, /* [3] f_lob_read_opt : must be written before streaming on */
	{0x3017, 0x01}, 
	{0x3E33, 0x3C}, 
	{0x3029, 0x0E}, 
	{0x300D, 0x14}, 
	{0x300E, 0x8E}, 
	{0x301B, 0x08}, 
	{0x305B, 0x9C}, 
	{0x3315, 0x5B}, 
	{0x3148, 0x00}, 
	{0x3149, 0x00}, 

	
	{0x3E84, 0x18}, 
	{0x3E85, 0x00},

	
	
	{0x0820, 0x06}, 
	{0x0821, 0x00}, 
	{0x0822, 0x9B}, 
	{0x0823, 0x00}, 

	
	{0x082A, 0x0A}, 
	{0x082B, 0x06}, 

	
	{0x0858, 0x02}, 
	{0x0859, 0x6C}, 
	{0x085A, 0x00}, 
	{0x085B, 0x00}, 

	
	{0x0216, 0x00}, 

	
	{0x0800, 0x00}, 
};

static struct msm_camera_i2c_reg_conf s5k6a2ya_recommend_settings_yushanii[] = {
	
	
	
	{0x303E, 0x20}, 
	{0x303F, 0x10}, 
	{0x3040, 0x40}, 
	{0x3041, 0x10}, 
	{0x3310, 0x0C}, /* [3:2] nop_flob : must be written before streaming on */
	{0x3074, 0x0E}, /* [3] f_lob_read_opt : must be written before streaming on */
	{0x3017, 0x01}, 
	{0x3E33, 0x3C}, 
	{0x3029, 0x0E}, 
	{0x300D, 0x14}, 
	{0x300E, 0x8E}, 
	{0x301B, 0x08}, 
	{0x305B, 0x9C}, 
	{0x3315, 0x5B}, 
	{0x3148, 0x00}, 
	{0x3149, 0x00}, 

	
	{0x3E84, 0x18}, 
	{0x3E85, 0x00},

	
	
	{0x0820, 0x06}, 
	{0x0821, 0x00}, 
	{0x0822, 0x9B}, 
	{0x0823, 0x00}, 

	
	{0x082A, 0x0A}, 
	{0x082B, 0x06}, 

	
	{0x0858, 0x02}, 
	{0x0859, 0x6C}, 
	{0x085A, 0x00}, 
	{0x085B, 0x00}, 

	
	{0x0216, 0x00}, 

	
	{0x0800, 0x00}, 
};

static struct v4l2_subdev_info s5k6a2ya_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	
};

static struct msm_camera_i2c_conf_array s5k6a2ya_init_conf[] = {
	{&s5k6a2ya_recommend_settings[0],
	ARRAY_SIZE(s5k6a2ya_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array s5k6a2ya_init_conf_yushanii[] = {
	{&s5k6a2ya_recommend_settings_yushanii[0],
	ARRAY_SIZE(s5k6a2ya_recommend_settings_yushanii), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array s5k6a2ya_confs[] = {
	{&s5k6a2ya_snap_settings[0],
	ARRAY_SIZE(s5k6a2ya_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k6a2ya_prev_settings[0],
	ARRAY_SIZE(s5k6a2ya_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t s5k6a2ya_dimensions[] = {
	{
		.x_output = 0x5C0,
		.y_output = 0x450,
		.line_length_pclk = 0x067E,
		.frame_length_lines = 0x04D6,
		.vt_pixel_clk = 62000000,
		.op_pixel_clk = 62000000,
		.binning_factor = 1,
		
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x5BF,
		.y_addr_end = 0x44F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{
		.x_output = 0x5C0,
		.y_output = 0x450,
		.line_length_pclk = 0x067E,
		.frame_length_lines = 0x04D6,
		.vt_pixel_clk = 62000000,
		.op_pixel_clk = 62000000,
		.binning_factor = 1,
		
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x5BF,
		.y_addr_end = 0x44F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
};

static struct msm_sensor_output_info_t s5k6a2ya_dimensions_yushanii[] = {
	{
		.x_output = 0x5C0,
		.y_output = 0x450,
		.line_length_pclk = 0x067E,
		.frame_length_lines = 0x04D6,
		.vt_pixel_clk = 62000000,
		.op_pixel_clk = 62000000,
		.binning_factor = 1,
		
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x5BF,
		.y_addr_end = 0x44F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{
		.x_output = 0x5C0,
		.y_output = 0x450,
		.line_length_pclk = 0x067E,
		.frame_length_lines = 0x04D6,
		.vt_pixel_clk = 62000000,
		.op_pixel_clk = 62000000,
		.binning_factor = 1,
		
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x5BF,
		.y_addr_end = 0x44F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
};

static struct msm_camera_csid_vc_cfg s5k6a2ya_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params s5k6a2ya_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 1,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = s5k6a2ya_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 1,
		.settle_cnt = 20,
	},
};

static struct msm_camera_csi2_params *s5k6a2ya_csi_params_array[] = {
	&s5k6a2ya_csi_params,
	&s5k6a2ya_csi_params,
};

static struct msm_sensor_output_reg_addr_t s5k6a2ya_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static struct msm_sensor_id_info_t s5k6a2ya_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x6a20,
};

static struct msm_sensor_exp_gain_info_t s5k6a2ya_exp_gain_info = {
	.coarse_int_time_addr = 0x222,
	.global_gain_addr = 0x204,
	.vert_offset = 4,
	.min_vert = 4,  
};

static int s5k6a2ya_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	if (data->sensor_platform_info)
		s5k6a2ya_s_ctrl.mirror_flip = data->sensor_platform_info->mirror_flip;

	return rc;
}

static int s5k6a2ya_mirror_flip_setting(void)
{
	int rc = 0;
	int value = 0;

	if (s5k6a2ya_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR_FLIP)
		value = S5K6A2YA_ORIENTATION_MIRROR_FLIP;
	else if (s5k6a2ya_s_ctrl.mirror_flip == CAMERA_SENSOR_MIRROR)
		value = S5K6A2YA_ORIENTATION_MIRROR;
	else if (s5k6a2ya_s_ctrl.mirror_flip == CAMERA_SENSOR_FLIP)
		value = S5K6A2YA_ORIENTATION_FLIP;
	else
		value = S5K6A2YA_ORIENTATION_NORMAL_MODE;
	rc = msm_camera_i2c_write(s5k6a2ya_s_ctrl.sensor_i2c_client,
		S5K6A2YA_REG_ORIENTATION, value, MSM_CAMERA_I2C_BYTE_DATA);

	pr_info("%s: mirror_flip: %d, rc = %d\n", __func__, s5k6a2ya_s_ctrl.mirror_flip, rc);
	return rc;
}

static const char *s5k6a2yaVendor = "samsung";
static const char *s5k6a2yaNAME = "s5k6a2ya";
static const char *s5k6a2yaSize = "1.6M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", s5k6a2yaVendor, s5k6a2yaNAME, s5k6a2yaSize);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);

static struct kobject *android_s5k6a2ya;

static int s5k6a2ya_sysfs_init(void)
{
	int ret ;
	pr_info("s5k6a2ya:kobject creat and add\n");
	android_s5k6a2ya = kobject_create_and_add("android_camera2", NULL);
	if (android_s5k6a2ya == NULL) {
		pr_info("s5k6a2ya_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("s5k6a2ya:sysfs_create_file\n");
	ret = sysfs_create_file(android_s5k6a2ya, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("s5k6a2ya_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_s5k6a2ya);
	}

	return 0 ;
}

static struct msm_camera_i2c_client s5k6a2ya_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

int32_t s5k6a2ya_power_up(struct msm_sensor_ctrl_t *s_ctrl)
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

	if (sdata->camera_power_on == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}
	rc = sdata->camera_power_on();
	if (rc < 0) {
		pr_err("%s failed to enable power\n", __func__);
		goto enable_power_on_failed;
	}

#ifndef CONFIG_DISABLE_MCLK_RAWCHIP_TO_MAINCAM
	if (!sdata->use_rawchip && (sdata->htc_image != HTC_CAMERA_IMAGE_YUSHANII_BOARD)) {
		rc = msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
		if (rc < 0) {
			pr_err("%s: msm_camio_clk_enable failed:%d\n",
			 __func__, rc);
			goto enable_mclk_failed;
		}
	}

	rc = msm_sensor_set_power_up(s_ctrl);

	if (rc < 0) {
		pr_err("%s msm_sensor_power_up failed\n", __func__);
		goto set_sensor_power_up_failed;
	}
#else
	rc = msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
	if (rc < 0) {
		pr_err("%s msm_camio_clk_enable failed\n", __func__);
		goto enable_mclk_failed;
	}
#endif

#ifdef CONFIG_RAWCHIPII
	Ilp0100_enableIlp0100SensorClock(SENSOR_1);
	mdelay(35); 
#endif

	s5k6a2ya_sensor_open_init(sdata);
	pr_info("%s end\n", __func__);

	return rc;

#ifndef CONFIG_DISABLE_MCLK_RAWCHIP_TO_MAINCAM
set_sensor_power_up_failed:
	msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
#endif
enable_mclk_failed:
	if (sdata->camera_power_off == NULL)
		pr_err("sensor platform_data didnt register\n");
	else
		sdata->camera_power_off();
enable_power_on_failed:
	return rc;
}

int32_t s5k6a2ya_power_down(struct msm_sensor_ctrl_t *s_ctrl)
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
		pr_err(" sensor platform_data didn't register\n");
		return -EIO;
	}
#ifndef CONFIG_DISABLE_MCLK_RAWCHIP_TO_MAINCAM
	if (!sdata->use_rawchip && (sdata->htc_image != HTC_CAMERA_IMAGE_YUSHANII_BOARD)) {
		rc = msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
		if (rc < 0)
			pr_err("%s: msm_camio_clk_disable failed:%d\n",
				 __func__, rc);
	}
#else
	rc = msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
	if (rc < 0)
		pr_err("%s: msm_camio_clk_disable failed:%d\n", __func__, rc);
#endif

#ifndef CONFIG_DISABLE_MCLK_RAWCHIP_TO_MAINCAM

	rc = msm_sensor_set_power_down(s_ctrl);
	if (rc < 0)
		pr_err("%s msm_sensor_power_down failed\n", __func__);
#endif
	rc = sdata->camera_power_off();
	if (rc < 0) {
		pr_err(" %s failed to disable power\n", __func__);
	}

	return rc;  
}

int32_t s5k6a2ya_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	rc = 0;
	pr_info("%s\n", __func__);
	rc = msm_sensor_i2c_probe(client, id);
	if(rc >= 0)
		s5k6a2ya_sysfs_init();
	pr_info("%s: rc(%d)\n", __func__, rc);
	return rc;
}

static int s5k6a2ya_read_fuseid(struct sensor_cfg_data *cdata,
	struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t  rc;
	unsigned short i, j, value;
	unsigned short  OTP[10] = {0};

	struct msm_camera_i2c_client *s5k6a2ya_msm_camera_i2c_client = s_ctrl->sensor_i2c_client;

	pr_info("%s: sensor OTP information:\n", __func__);

	rc = msm_camera_i2c_write_b(s5k6a2ya_msm_camera_i2c_client, 0x3602, 0x00); 
	if (rc < 0)
		pr_info("%s: i2c_write_b 0x3602 fail\n", __func__);

	rc = msm_camera_i2c_write_b(s5k6a2ya_msm_camera_i2c_client, 0x3600, 0x01); 
	if (rc < 0)
		pr_info("%s: i2c_write_b 0x3600 fail\n", __func__);

	mdelay(5);

	for (i = 0; i < 10; i++) {
		for (j = 0; j < 5; j++) {
			rc = msm_camera_i2c_read_b(s5k6a2ya_msm_camera_i2c_client, 0x3604 + i + (10 * j), &value);
			if (rc < 0)
				pr_info("%s: i2c_read_b %x fail\n", __func__, (0x3604 + i + (10 * j)));
			if (value != 00) {
				OTP[i] = value;
				break;
			}
		}
	}

	rc = msm_camera_i2c_write_b(s5k6a2ya_msm_camera_i2c_client, 0x3600, 0x00); 
	if (rc < 0)
		pr_info("%s: i2c_write_b 0x3600 fail\n", __func__);

	pr_info("%s: Vender ID = 0x%x, Lens ID = 0x%x, Version = 0x%x\n", __func__,
		OTP[0], OTP[1], OTP[2]);
	pr_info("%s: ModuleFuseID= %x%x%x%x%x%x%x\n", __func__,
		OTP[3], OTP[4], OTP[5], OTP[6], OTP[7], OTP[8], OTP[9]);

	cdata->cfg.fuse.fuse_id_word1 = OTP[0] << 8 | OTP[1];
	cdata->cfg.fuse.fuse_id_word2 = OTP[3] << 8 | OTP[4];
	cdata->cfg.fuse.fuse_id_word3 = OTP[5] << 8 | OTP[6];
	cdata->cfg.fuse.fuse_id_word4 = OTP[7] << 8 | OTP[8];

	pr_info("%s: fuse->fuse_id_word1:0x%8x\n",
		__func__, cdata->cfg.fuse.fuse_id_word1);
	pr_info("%s: fuse->fuse_id_word2:0x%8x\n",
		__func__, cdata->cfg.fuse.fuse_id_word2);
	pr_info("%s: fuse->fuse_id_word3:0x%8x\n",
		__func__, cdata->cfg.fuse.fuse_id_word3);
	pr_info("%s: fuse->fuse_id_word4:0x%8x\n",
		__func__, cdata->cfg.fuse.fuse_id_word4);
	return 0;
}


int32_t s5k6a2ya_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res) {

	int rc = 0;
	struct msm_camera_sensor_info *sdata = NULL;
	pr_info("%s\n", __func__);

	if (s_ctrl && s_ctrl->sensordata)
		sdata = s_ctrl->sensordata;
	else {
		pr_err("%s: s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}

	
	rc = msm_sensor_setting_parallel(s_ctrl, update_type, res);

	if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		s5k6a2ya_mirror_flip_setting();
	}

	return rc;
}


static const struct i2c_device_id s5k6a2ya_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&s5k6a2ya_s_ctrl},
	{ }
};

static struct i2c_driver s5k6a2ya_i2c_driver = {
	.id_table = s5k6a2ya_i2c_id,
	.probe  = s5k6a2ya_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static int __init msm_sensor_init_module(void)
{
	pr_info("%s\n", __func__);
	return i2c_add_driver(&s5k6a2ya_i2c_driver);
}

static struct v4l2_subdev_core_ops s5k6a2ya_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops s5k6a2ya_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops s5k6a2ya_subdev_ops = {
	.core = &s5k6a2ya_subdev_core_ops,
	.video  = &s5k6a2ya_subdev_video_ops,
};

static struct msm_sensor_fn_t s5k6a2ya_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain_ex = msm_sensor_write_exp_gain1_ex,
	.sensor_write_snapshot_exp_gain_ex = msm_sensor_write_exp_gain1_ex,
	.sensor_setting = s5k6a2ya_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = s5k6a2ya_power_up,
	.sensor_power_down = s5k6a2ya_power_down,
	.sensor_i2c_read_fuseid = s5k6a2ya_read_fuseid, 
};

static struct msm_sensor_reg_t s5k6a2ya_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = s5k6a2ya_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(s5k6a2ya_start_settings),
	.stop_stream_conf = s5k6a2ya_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(s5k6a2ya_stop_settings),
	.start_stream_conf_yushanii = s5k6a2ya_start_settings_yushanii,
	.start_stream_conf_size_yushanii =
		ARRAY_SIZE(s5k6a2ya_start_settings_yushanii),
	.stop_stream_conf_yushanii = s5k6a2ya_stop_settings_yushanii,
	.stop_stream_conf_size_yushanii =
		ARRAY_SIZE(s5k6a2ya_stop_settings_yushanii),
	.group_hold_on_conf = s5k6a2ya_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(s5k6a2ya_groupon_settings),
	.group_hold_off_conf = s5k6a2ya_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(s5k6a2ya_groupoff_settings),
	.init_settings = &s5k6a2ya_init_conf[0],
	.init_settings_yushanii = &s5k6a2ya_init_conf_yushanii[0],
	.init_size = ARRAY_SIZE(s5k6a2ya_init_conf),
	.init_size_yushanii = ARRAY_SIZE(s5k6a2ya_init_conf_yushanii),
	.mode_settings = &s5k6a2ya_confs[0],
	.output_settings = &s5k6a2ya_dimensions[0],
	.output_settings_yushanii = &s5k6a2ya_dimensions_yushanii[0],
	.num_conf = ARRAY_SIZE(s5k6a2ya_confs),
};

static struct msm_sensor_ctrl_t s5k6a2ya_s_ctrl = {
	.msm_sensor_reg = &s5k6a2ya_regs,
	.sensor_i2c_client = &s5k6a2ya_sensor_i2c_client,
	.sensor_i2c_addr = 0x6C,
	.sensor_output_reg_addr = &s5k6a2ya_reg_addr,
	.sensor_id_info = &s5k6a2ya_id_info,
	.sensor_exp_gain_info = &s5k6a2ya_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &s5k6a2ya_csi_params_array[0],
	.msm_sensor_mutex = &s5k6a2ya_mut,
	.sensor_i2c_driver = &s5k6a2ya_i2c_driver,
	.sensor_v4l2_subdev_info = s5k6a2ya_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k6a2ya_subdev_info),
	.sensor_v4l2_subdev_ops = &s5k6a2ya_subdev_ops,
	.func_tbl = &s5k6a2ya_func_tbl,
	.sensor_first_mutex = &s5k6a2ya_sensor_init_mut,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Samsung 1.6 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
