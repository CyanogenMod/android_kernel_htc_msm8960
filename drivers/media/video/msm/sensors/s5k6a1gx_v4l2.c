#include "msm_sensor.h"
#define SENSOR_NAME "s5k6a1gx"
#define PLATFORM_DRIVER_NAME "msm_camera_s5k6a1gx"
#define s5k6a1gx_obj s5k6a1gx_##obj

DEFINE_MUTEX(s5k6a1gx_mut);
static struct msm_sensor_ctrl_t s5k6a1gx_s_ctrl;

static struct msm_camera_i2c_reg_conf s5k6a1gx_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k6a1gx_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k6a1gx_groupon_settings[] = {
	{0x104, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k6a1gx_groupoff_settings[] = {
	{0x104, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k6a1gx_prev_settings[] = {
	/*AE setting*/
	{0x0202, 0x01}, /*coarse int time*/
	{0x0203, 0x4C},
	{0x0204, 0x00}, /*analog gain*/
	{0x0205, 0x20},
	{0x0342, 0x05}, /* Min Line Length*/
	{0x0343, 0xCE},
	{0x0340, 0x04}, /*frame length line*/
	{0x0341, 0x22},

	/*size setting*/
	{0x0344, 0x00}, /*x_addr_start_H*/
	{0x0345, 0x00}, /*x_addr_start_L*/
	{0x0346, 0x00}, /*y_addr_start_H*/
	{0x0347, 0x00}, /*y_addr_start_L*/

	{0x0348, 0x05}, /*y_addr_end_H*/
	{0x0349, 0x0F}, /*y_addr_end_L*/

	{0x034A, 0x04}, /*y_addr_end_H*/
	{0x034B, 0x0F}, /*y_addr_end_L*/

	{0x034C, 0x05}, /*x_size_ouput 1280*/
	{0x034D, 0x10},

	{0x034E, 0x04}, /*y_size_ouput 720*/
	{0x034F, 0x10},
};

static struct msm_camera_i2c_reg_conf s5k6a1gx_snap_settings[] = {
	/*AE setting*/
	{0x0202, 0x01}, /*coarse int time*/
	{0x0203, 0x4C},
	{0x0204, 0x00}, /*analog gain*/
	{0x0205, 0x20},
	{0x0342, 0x05}, /* Min Line Length*/
	{0x0343, 0xCE},
	{0x0340, 0x04}, /*frame length line*/
	{0x0341, 0x22},

	/*size setting*/
	{0x0344, 0x00}, /*x_addr_start_H*/
	{0x0345, 0x00}, /*x_addr_start_L*/
	{0x0346, 0x00}, /*y_addr_start_H*/
	{0x0347, 0x00}, /*y_addr_start_L*/

	{0x0348, 0x05}, /*y_addr_end_H*/
	{0x0349, 0x0F}, /*y_addr_end_L*/

	{0x034A, 0x04}, /*y_addr_end_H*/
	{0x034B, 0x0F}, /*y_addr_end_L*/

	{0x034C, 0x05}, /*x_size_ouput 1280*/
	{0x034D, 0x10},

	{0x034E, 0x04}, /*y_size_ouput 720*/
	{0x034F, 0x10},
};

static struct msm_camera_i2c_reg_conf s5k6a1gx_recommend_settings[] = {
	{0x0103, 0x01}, /*sw reset*/
	{0x301C, 0x35}, /*APS*/
	{0x3016, 0x05}, /*Analog*/
	{0x3034, 0x73}, /*Analog*/
	{0x3037, 0x01}, /*Analog*/
	{0x3035, 0x05}, /*Analog*/
	{0x301E, 0x09}, /*Analog*/
	{0x301B, 0xC0}, /*Analog*/
	{0x3013, 0x28}, /*Analog*/
	{0x3042, 0x01}, /*Analog*/
	{0x303C, 0x01}, /*Analog*/

	/*Mclk=24Mhz*/
	{0x30BC, 0x38}, /*outif_mld_ulpm_rxinit_limit[15:8] */
	{0x30BD, 0x40}, /*outif_mld_ulpm_rxinit_limit[7:0] */
	{0x3110, 0x70}, /*outif_enable_time[15:8] */
	{0x3111, 0x80}, /*outif_enable_time[7:0] */
	{0x3112, 0x7B}, /*streaming_enable_time[15:8] */
	{0x3113, 0xC0}, /*streaming_enable_time[7:0] */
	{0x30C7, 0x1A}, /*[5:4]esc_ref_div, [3] dphy_ulps_auto, [1]dphy_enable */

	/* 480Mbps */
	{0x0305, 0x04}, /*pll_p*/
	{0x0306, 0x00}, /*pll_m*/
	{0x0307, 0xA0},
	{0x308D, 0x01}, /*pll_s*/
	{0x0301, 0x0A}, /*vt_pix_clk_div*/
	{0x0303, 0x01}, /*vt_sys_clk_div*/

	/*PLL*/
	{0x0101, 0x00}, /*Vfilp + H mirror  0x03*/
};

static struct v4l2_subdev_info s5k6a1gx_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array s5k6a1gx_init_conf[] = {
	{&s5k6a1gx_recommend_settings[0],
	ARRAY_SIZE(s5k6a1gx_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array s5k6a1gx_confs[] = {
	{&s5k6a1gx_snap_settings[0],
	ARRAY_SIZE(s5k6a1gx_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k6a1gx_prev_settings[0],
	ARRAY_SIZE(s5k6a1gx_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t s5k6a1gx_dimensions[] = {
	{
		.x_output = 0x510,
		.y_output = 0x410,
		.line_length_pclk = 0x5CE,
		.frame_length_lines = 0x422,
		.vt_pixel_clk = 48000000,
		.op_pixel_clk = 48000000,
		.binning_factor = 1,
	},
	{
		.x_output = 0x510,
		.y_output = 0x410,
		.line_length_pclk = 0x5CE,
		.frame_length_lines = 0x422,
		.vt_pixel_clk = 48000000,
		.op_pixel_clk = 48000000,
		.binning_factor = 1,
	},
};

static struct msm_camera_csid_vc_cfg s5k6a1gx_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params s5k6a1gx_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 1,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = s5k6a1gx_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 1,
		.settle_cnt = 20,
	},
};

static struct msm_camera_csi2_params *s5k6a1gx_csi_params_array[] = {
	&s5k6a1gx_csi_params,
	&s5k6a1gx_csi_params,
};

static struct msm_sensor_output_reg_addr_t s5k6a1gx_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static struct msm_sensor_id_info_t s5k6a1gx_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x6a10,
};

static struct msm_sensor_exp_gain_info_t s5k6a1gx_exp_gain_info = {
	.coarse_int_time_addr = 0x202,
	.global_gain_addr = 0x204,
	.vert_offset = 4,
	.min_vert = 4, /* min coarse integration time */ /* HTC Angie 20111019 - Fix FPS */
};

static int s5k6a1gx_sensor_config(void __user *argp)
{
	return msm_sensor_config(&s5k6a1gx_s_ctrl, argp);
}

static int s5k6a1gx_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	return msm_sensor_open_init(&s5k6a1gx_s_ctrl, data);
}

static int s5k6a1gx_sensor_release(void)
{
	int	rc = 0;

	rc = msm_sensor_release(&s5k6a1gx_s_ctrl);

	return rc;
}

static const char *s5k6a1gxVendor = "samsung";
static const char *s5k6a1gxNAME = "s5k6a1gx";
static const char *s5k6a1gxSize = "1.3M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", s5k6a1gxVendor, s5k6a1gxNAME, s5k6a1gxSize);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);

static struct kobject *android_s5k6a1gx;

static int s5k6a1gx_sysfs_init(void)
{
	int ret ;
	pr_info("s5k6a1gx:kobject creat and add\n");
	android_s5k6a1gx = kobject_create_and_add("android_camera2", NULL);
	if (android_s5k6a1gx == NULL) {
		pr_info("s5k6a1gx_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("s5k6a1gx:sysfs_create_file\n");
	ret = sysfs_create_file(android_s5k6a1gx, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("s5k6a1gx_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_s5k6a1gx);
	}

	return 0 ;
}

static const struct i2c_device_id s5k6a1gx_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&s5k6a1gx_s_ctrl},
	{ }
};

static struct i2c_driver s5k6a1gx_i2c_driver = {
	.id_table = s5k6a1gx_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client s5k6a1gx_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int s5k6a1gx_sensor_v4l2_probe(const struct msm_camera_sensor_info *info,
	struct v4l2_subdev *sdev, struct msm_sensor_ctrl *s)
{
	int rc = -EINVAL;

	rc = msm_sensor_v4l2_probe(&s5k6a1gx_s_ctrl, info, sdev, s);

	return rc;
}

int32_t s5k6a1gx_power_up(const struct msm_camera_sensor_info *sdata)
{
	int rc;
	pr_info("%s\n", __func__);

	if (sdata->camera_power_on == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}

	rc = sdata->camera_power_on();
	if (rc < 0) {
		pr_err("%s failed to enable power\n", __func__);
		return rc;
	}

	rc = msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
	if (rc < 0) {
		return rc;
	}

	return 0;  /*msm_sensor_power_up(sdata)*/
}

int32_t s5k6a1gx_power_down(const struct msm_camera_sensor_info *sdata)
{
	int rc;
	pr_info("%s\n", __func__);

	if (sdata->camera_power_off == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}

	msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);

	rc = sdata->camera_power_off();
	if (rc < 0) {
		pr_err("%s failed to disable power\n", __func__);
		return rc;
	}
	return 0;  /*msm_sensor_power_down(sdata);*/
}

static int s5k6a1gx_probe(struct platform_device *pdev)
{
	int	rc = 0;

	pr_info("%s\n", __func__);

	rc = msm_sensor_register(pdev, s5k6a1gx_sensor_v4l2_probe);
	if(rc >= 0)
		s5k6a1gx_sysfs_init();
	return rc;
}

struct platform_driver s5k6a1gx_driver = {
	.probe = s5k6a1gx_probe,
	.driver = {
		.name = PLATFORM_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init msm_sensor_init_module(void)
{
	pr_info("sss%s\n", __func__);
	return platform_driver_register(&s5k6a1gx_driver);
}

static struct v4l2_subdev_core_ops s5k6a1gx_subdev_core_ops;
static struct v4l2_subdev_video_ops s5k6a1gx_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops s5k6a1gx_subdev_ops = {
	.core = &s5k6a1gx_subdev_core_ops,
	.video  = &s5k6a1gx_subdev_video_ops,
};

static struct msm_sensor_fn_t s5k6a1gx_func_tbl = {
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
	.sensor_config = s5k6a1gx_sensor_config,
	.sensor_open_init = s5k6a1gx_sensor_open_init,
	.sensor_release = s5k6a1gx_sensor_release,
	.sensor_power_up = s5k6a1gx_power_up,
	.sensor_power_down = s5k6a1gx_power_down,
	.sensor_probe = msm_sensor_probe,
};

static struct msm_sensor_reg_t s5k6a1gx_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = s5k6a1gx_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(s5k6a1gx_start_settings),
	.stop_stream_conf = s5k6a1gx_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(s5k6a1gx_stop_settings),
	.group_hold_on_conf = s5k6a1gx_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(s5k6a1gx_groupon_settings),
	.group_hold_off_conf = s5k6a1gx_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(s5k6a1gx_groupoff_settings),
	.init_settings = &s5k6a1gx_init_conf[0],
	.init_size = ARRAY_SIZE(s5k6a1gx_init_conf),
	.mode_settings = &s5k6a1gx_confs[0],
	.output_settings = &s5k6a1gx_dimensions[0],
	.num_conf = ARRAY_SIZE(s5k6a1gx_confs),
};

static struct msm_sensor_ctrl_t s5k6a1gx_s_ctrl = {
	.msm_sensor_reg = &s5k6a1gx_regs,
	.sensor_i2c_client = &s5k6a1gx_sensor_i2c_client,
	.sensor_i2c_addr = 0x6C,
	.sensor_output_reg_addr = &s5k6a1gx_reg_addr,
	.sensor_id_info = &s5k6a1gx_id_info,
	.sensor_exp_gain_info = &s5k6a1gx_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &s5k6a1gx_csi_params_array[0],
	.msm_sensor_mutex = &s5k6a1gx_mut,
	.sensor_i2c_driver = &s5k6a1gx_i2c_driver,
	.sensor_v4l2_subdev_info = s5k6a1gx_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k6a1gx_subdev_info),
	.sensor_v4l2_subdev_ops = &s5k6a1gx_subdev_ops,
	.func_tbl = &s5k6a1gx_func_tbl,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Samsung 1.3 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
