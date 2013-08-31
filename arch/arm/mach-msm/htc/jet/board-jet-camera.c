/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm/mach-types.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <mach/board.h>
#include <mach/msm_bus_board.h>
#include <mach/gpiomux.h>
#include <asm/setup.h>

#include "devices.h"
#include "board-jet.h"

#include <linux/spi/spi.h>

#include "board-mahimahi-flashlight.h"
#ifdef CONFIG_MSM_CAMERA_FLASH
#include <linux/htc_flashlight.h>
#endif

#ifdef CONFIG_MSM_CAMERA
#define MSM_8960_GSBI4_QUP_I2C_BUS_ID 4	
static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};

static struct gpiomux_setting cam_settings[11] = {
	{
		.func = GPIOMUX_FUNC_GPIO, 
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},

	{
		.func = GPIOMUX_FUNC_1, 
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, 
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},

	{
		.func = GPIOMUX_FUNC_1, 
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_2, 
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, 
		.drv = GPIOMUX_DRV_4MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},

	{
		.func = GPIOMUX_FUNC_2, 
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, 
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_IN,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, 
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, 
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_HIGH,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, 
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},

};

static struct msm_gpiomux_config jet_cam_configs[] = {
	{
		.gpio = JET_GPIO_CAM_MCLK1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[4], 
			[GPIOMUX_SUSPENDED] = &cam_settings[2], 
		},
	},
	{
		.gpio = JET_GPIO_CAM_MCLK0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[1], 
			[GPIOMUX_SUSPENDED] = &cam_settings[2], 
		},
	},
	{
		.gpio = JET_GPIO_CAM_I2C_DAT,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3], 
			[GPIOMUX_SUSPENDED] = &cam_settings[2],
		},
	},
	{
		.gpio = JET_GPIO_CAM_I2C_CLK,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3], 
			[GPIOMUX_SUSPENDED] = &cam_settings[2],
		},
	},
	{
		.gpio = JET_GPIO_RAW_INTR0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7], 
			[GPIOMUX_SUSPENDED] = &cam_settings[8], 
		},
	},
	{
		.gpio = JET_GPIO_RAW_INTR1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7], 
			[GPIOMUX_SUSPENDED] = &cam_settings[8], 
		},
	},
	
	{
		.gpio      = JET_GPIO_MCAM_SPI_CLK,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[6], 
			[GPIOMUX_SUSPENDED] = &cam_settings[10],
		},
	},
	{
		.gpio      = JET_GPIO_MCAM_SPI_CS0,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[6], 
			[GPIOMUX_SUSPENDED] = &cam_settings[10], 
		},
	},
	{
		.gpio      = JET_GPIO_MCAM_SPI_DI,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[6], 
			[GPIOMUX_SUSPENDED] = &cam_settings[10],
		},
	},
	{
		.gpio      = JET_GPIO_MCAM_SPI_DO,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[6], 
			[GPIOMUX_SUSPENDED] = &cam_settings[10],
		},
	},
};

static struct msm_bus_vectors cam_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_preview_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 96215040,
		.ib  = 378224640,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_video_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 342150912,
		.ib  = 1361968128,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 207747072,
		.ib  = 489756672,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 60318720,
		.ib  = 150796800,
	},
};

static struct msm_bus_vectors cam_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 147045888,
		.ib  = 588183552,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 263678976,
		.ib  = 659197440,
	},
};

static struct msm_bus_vectors cam_zsl_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 319044096,
		.ib  = 1271531520,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 239708160,
		.ib  = 599270400,
	},
};

static struct msm_bus_paths cam_bus_client_config[] = {
	{
		ARRAY_SIZE(cam_init_vectors),
		cam_init_vectors,
	},
	{
		ARRAY_SIZE(cam_preview_vectors),
		cam_preview_vectors,
	},
	{
		ARRAY_SIZE(cam_video_vectors),
		cam_video_vectors,
	},
	{
		ARRAY_SIZE(cam_snapshot_vectors),
		cam_snapshot_vectors,
	},
	{
		ARRAY_SIZE(cam_zsl_vectors),
		cam_zsl_vectors,
	},
};

static struct msm_bus_scale_pdata cam_bus_client_pdata = {
		cam_bus_client_config,
		ARRAY_SIZE(cam_bus_client_config),
		.name = "msm_camera",
};

static int jet_csi_vreg_on(void);
static int jet_csi_vreg_off(void);

struct msm_camera_device_platform_data msm_camera_csi_device_data[] = {
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 0,
		.camera_csi_on = jet_csi_vreg_on,
		.camera_csi_off = jet_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
		.csid_core = 0,
		.is_csiphy = 1,
		.is_csid   = 1,
		.is_ispif  = 1,
		.is_vpe    = 1,
	},
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 1,
		.camera_csi_on = jet_csi_vreg_on,
		.camera_csi_off = jet_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
		.csid_core = 1,
		.is_csiphy = 1,
		.is_csid   = 1,
		.is_ispif  = 1,
		.is_vpe    = 1,
	},
};

#ifdef CONFIG_MSM_CAMERA_FLASH
int flashlight_control(int mode)
{
pr_info("%s, linear led, mode=%d", __func__, mode);
#ifdef CONFIG_FLASHLIGHT_TPS61310
	return tps61310_flashlight_control(mode);
#else
	return 0;
#endif
}

static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
	.camera_flash = flashlight_control,
};
#endif /* CONFIG_MSM_CAMERA_FLASH */

#ifdef CONFIG_RAWCHIP
static int jet_use_ext_1v2(void)
{
	if (system_rev >= 1) 
		return 1;
	else 
		return 0;
}

#define JET_V_RAW_1V8_EN PM8921_GPIO_PM_TO_SYS(JET_GPIO_RAW_1V8_EN)
#define JET_V_RAW_1V2_EN PM8921_GPIO_PM_TO_SYS(JET_GPIO_RAW_1V2_EN)
static int jet_rawchip_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	
	rc = gpio_request(JET_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc) {
		pr_err("rawchip on\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			JET_V_RAW_1V8_EN, rc);
		goto enable_1v8_fail;
	}
	gpio_direction_output(JET_V_RAW_1V8_EN, 1);
	gpio_free(JET_V_RAW_1V8_EN);

	mdelay(5);

	
	if (system_rev < 2) {	
		rc = gpio_request(JET_V_RAW_1V2_EN, "V_RAW_1V2_EN");
		if (rc) {
			pr_err("rawchip on\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			JET_V_RAW_1V2_EN, rc);
			goto enable_1v2_fail;
		}
		gpio_direction_output(JET_V_RAW_1V2_EN, 1);
		gpio_free(JET_V_RAW_1V2_EN);
	}

	if (jet_use_ext_1v2()) { 
		mdelay(1);

		rc = gpio_request(JET_GPIO_V_CAM_D1V2_EN, "rawchip");
		pr_info("rawchip external 1v2 gpio_request,%d\n",
		JET_GPIO_V_CAM_D1V2_EN);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", JET_GPIO_V_CAM_D1V2_EN);
			goto enable_1v2_fail;
		}
		gpio_direction_output(JET_GPIO_V_CAM_D1V2_EN, 1);
		gpio_free(JET_GPIO_V_CAM_D1V2_EN);
		mdelay(1);
	}
	return rc;

enable_1v2_fail:
	rc = gpio_request(JET_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc)
		pr_err("rawchip on\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			JET_V_RAW_1V8_EN, rc);
	gpio_direction_output(JET_V_RAW_1V8_EN, 0);
	gpio_free(JET_V_RAW_1V8_EN);
enable_1v8_fail:
	return rc;
}

static int jet_rawchip_vreg_off(void)
{
	int rc = 0;

	pr_info("%s\n", __func__);

	if (jet_use_ext_1v2()) { 
		rc = gpio_request(JET_GPIO_V_CAM_D1V2_EN, "rawchip");
		if (rc)
			pr_err("rawchip off(\
				\"gpio %d\", 1.2V) FAILED %d\n",
				JET_GPIO_V_CAM_D1V2_EN, rc);
		gpio_direction_output(JET_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(JET_GPIO_V_CAM_D1V2_EN);

		mdelay(1);
	}

	if (system_rev < 2) {	
		rc = gpio_request(JET_V_RAW_1V2_EN, "V_RAW_1V2_EN");
		if (rc)
			pr_err("rawchip off(\
		\"gpio %d\", 1.2V) FAILED %d\n",
		JET_V_RAW_1V2_EN, rc);
		gpio_direction_output(JET_V_RAW_1V2_EN, 0);
		gpio_free(JET_V_RAW_1V2_EN);

		mdelay(5);
	}

	rc = gpio_request(JET_V_RAW_1V8_EN, "V_RAW_1V8_EN");
	if (rc)
		pr_err("rawchip off\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			JET_V_RAW_1V8_EN, rc);
	gpio_direction_output(JET_V_RAW_1V8_EN, 0);
	gpio_free(JET_V_RAW_1V8_EN);

	return rc;
}

static struct msm_camera_rawchip_info msm_rawchip_board_info = {
	.rawchip_reset	= JET_GPIO_RAW_RSTN,
	.rawchip_intr0	= MSM_GPIO_TO_INT(JET_GPIO_RAW_INTR0),
	.rawchip_intr1	= MSM_GPIO_TO_INT(JET_GPIO_RAW_INTR1),
	.rawchip_spi_freq = 27, 
	.rawchip_mclk_freq = 24, 
	.camera_rawchip_power_on = jet_rawchip_vreg_on,
	.camera_rawchip_power_off = jet_rawchip_vreg_off,
	.rawchip_use_ext_1v2 = jet_use_ext_1v2,
};

struct platform_device msm_rawchip_device = {
	.name	= "rawchip",
	.dev	= {
		.platform_data = &msm_rawchip_board_info,
	},
};
#endif /* CONFIG_RAWCHIP */

static uint16_t msm_cam_gpio_tbl[] = {
	JET_GPIO_CAM_MCLK0, 
	JET_GPIO_CAM_MCLK1,
	JET_GPIO_RAW_INTR0,
	JET_GPIO_RAW_INTR1,
	JET_GPIO_MCAM_SPI_CLK,
	JET_GPIO_MCAM_SPI_CS0,
	JET_GPIO_MCAM_SPI_DI,
	JET_GPIO_MCAM_SPI_DO,
};


static struct msm_camera_gpio_conf gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = msm_cam_gpio_tbl,
	.cam_gpio_tbl_size = ARRAY_SIZE(msm_cam_gpio_tbl),
};


static struct regulator *reg_8921_l2;
static struct regulator *reg_8921_l8;
static struct regulator *reg_8921_l9;
static struct regulator *reg_8921_l17;
static struct regulator *reg_8921_lvs6;

static int camera_sensor_power_enable(char *power, unsigned volt, struct regulator **sensor_power)
{
	int rc;

	if (power == NULL)
		return -ENODEV;

	*sensor_power = regulator_get(NULL, power);

	if (IS_ERR(*sensor_power)) {
		pr_info("%s: failed to Unable to get %s\n", __func__, power);
		return -ENODEV;
	}

	if (volt != 1800000) {
		rc = regulator_set_voltage(*sensor_power, volt, volt);
		if (rc < 0) {
			pr_info("%s: failed to unable to set %s voltage to %d rc:%d\n",
					__func__, power, volt, rc);
			regulator_put(*sensor_power);
			*sensor_power = NULL;
			return -ENODEV;
		}
	}

	rc = regulator_enable(*sensor_power);
	if (rc < 0) {
		pr_info("%s: failed to Enable regulator %s failed\n", __func__, power);
		regulator_put(*sensor_power);
		*sensor_power = NULL;
		return -ENODEV;
	}

	return rc;
}

static int camera_sensor_power_disable(struct regulator *sensor_power)
{

	int rc;
	if (sensor_power == NULL)
		return -ENODEV;

	if (IS_ERR(sensor_power)) {
		pr_info("%s: failed to Invalid requlator ptr\n", __func__);
		return -ENODEV;
	}

	rc = regulator_disable(sensor_power);
	if (rc < 0)
		pr_info("%s: disable regulator failed\n", __func__);

	regulator_put(sensor_power);
	sensor_power = NULL;
	return rc;
}

static int jet_csi_vreg_on(void)
{
	pr_info("%s\n", __func__);
	return camera_sensor_power_enable("8921_l2", 1200000, &reg_8921_l2);
}

static int jet_csi_vreg_off(void)
{
	pr_info("%s\n", __func__);
	return camera_sensor_power_disable(reg_8921_l2);
}


#ifdef CONFIG_S5K3H2YX
static int jet_s5k3h2yx_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	
	if (jet_use_ext_1v2())
		rc = camera_sensor_power_enable("8921_l17", 2850000, &reg_8921_l17);
	else
		rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9);

	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);

	
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(1);

	
	rc = gpio_request(JET_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc) {
		pr_err("sensor_power_enable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			JET_GPIO_V_CAM_D1V2_EN, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(JET_GPIO_V_CAM_D1V2_EN, 1);
	gpio_free(JET_GPIO_V_CAM_D1V2_EN);
	mdelay(1);

	
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_lvs6\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}

	return rc;

enable_io_fail:
	rc = gpio_request(JET_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			JET_GPIO_V_CAM_D1V2_EN, rc);
	else {
		gpio_direction_output(JET_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(JET_GPIO_V_CAM_D1V2_EN);
	}
enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_l9);
enable_vcm_fail:
	return rc;
}

static int jet_s5k3h2yx_vreg_off(void)
{
	int rc = 0;

	pr_info("%s\n", __func__);

	
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(1);

	
	rc = gpio_request(JET_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			JET_GPIO_V_CAM_D1V2_EN, rc);
	else {
		gpio_direction_output(JET_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(JET_GPIO_V_CAM_D1V2_EN);
	}
	mdelay(1);

	
	rc = camera_sensor_power_disable(reg_8921_lvs6);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_lvs6\") FAILED %d\n", rc);

	mdelay(1);

	
	if (jet_use_ext_1v2() == 0)
		rc = camera_sensor_power_disable(reg_8921_l9);
	else if (reg_8921_l17 != NULL) { 
		regulator_put(reg_8921_l17);
		reg_8921_l17 = NULL;
	}

	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	return rc;
}

#ifdef CONFIG_S5K3H2YX_ACT
static struct i2c_board_info s5k3h2yx_actuator_i2c_info = {
	I2C_BOARD_INFO("s5k3h2yx_act", 0x11),
};

static struct msm_actuator_info s5k3h2yx_actuator_info = {
	.board_info     = &s5k3h2yx_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = JET_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif

static struct msm_camera_csi_lane_params s5k3h2yx_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_s5k3h2yx_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_MIRROR_FLIP,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= JET_GPIO_CAM_PWDN,
	.vcm_pwd	= JET_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &s5k3h2yx_csi_lane_params,
};

static struct camera_led_est msm_camera_sensor_s5k3h2yx_led_table[] = {
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,
		.min_step = 58,
		.max_step = 256
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 350,
		.min_step = 54,
		.max_step = 57
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 400,
		.lumen_value = 440,
		.min_step = 50,
		.max_step = 53
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 600,
		.lumen_value = 625,
		.min_step = 46,
		.max_step = 49
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,
		.min_step = 0,
		.max_step = 45    
	},

		{
		.enable = 2,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,
		.min_step = 0,
		.max_step = 270
	},
	{
		.enable = 0,
		.led_state = FL_MODE_OFF,
		.current_ma = 0,
		.lumen_value = 0,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 0,
		.led_state = FL_MODE_TORCH,
		.current_ma = 150,
		.lumen_value = 150,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 2,     
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,
		.min_step = 271,
		.max_step = 317    
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL5,
		.current_ma = 500,
		.lumen_value = 500,
		.min_step = 25,
		.max_step = 26
	},
		{
		.enable = 0,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 750,
		.min_step = 271,
		.max_step = 325
	},

	{
		.enable = 0,
		.led_state = FL_MODE_TORCH_LEVEL_2,
		.current_ma = 200,
		.lumen_value = 75,
		.min_step = 0,
		.max_step = 40
	},};

static struct camera_led_info msm_camera_sensor_s5k3h2yx_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 750,  
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_s5k3h2yx_led_table),
};

static struct camera_flash_info msm_camera_sensor_s5k3h2yx_flash_info = {
	.led_info = &msm_camera_sensor_s5k3h2yx_led_info,
	.led_est_table = msm_camera_sensor_s5k3h2yx_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_s5k3h2yx_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 14,
	.low_cap_limit_dual	= 0,
	.flash_info             = &msm_camera_sensor_s5k3h2yx_flash_info,
};

static struct msm_camera_sensor_flash_data flash_s5k3h2yx = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_flash_src,
#endif
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3h2yx_data = {
	.sensor_name	= "s5k3h2yx",
	.camera_power_on = jet_s5k3h2yx_vreg_on,
	.camera_power_off = jet_s5k3h2yx_vreg_off,
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_s5k3h2yx,
	.sensor_platform_info = &sensor_s5k3h2yx_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
#ifdef CONFIG_S5K3H2YX_ACT
	.actuator_info = &s5k3h2yx_actuator_info,
#endif
	.use_rawchip = RAWCHIP_ENABLE,
	.flash_cfg = &msm_camera_sensor_s5k3h2yx_flash_cfg, 
};
#endif /* CONFIG_S5K3H2YX */

#ifdef CONFIG_IMX175_2LANE
static int jet_imx175_vreg_on(void)
{
	int rc;
	pr_info("[CAM] %s\n", __func__);

	
	if (jet_use_ext_1v2())
		rc = camera_sensor_power_enable("8921_l17", 2850000, &reg_8921_l17);
	else
		rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9);

	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable\
			(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);

	
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable\
			(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_vana_fail;
	}
	mdelay(1);

	
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable\
			(\"8921_lvs6\", 1.8V) FAILED %d\n", rc);
		goto enable_vdig_fail;
	}

	return rc;

enable_vdig_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_vana_fail:
	if (jet_use_ext_1v2() == 0)
		camera_sensor_power_disable(reg_8921_l9);
enable_vcm_fail:
	return rc;
}

static int jet_imx175_vreg_off(void)
{
	int rc = 0;

	pr_info("[CAM] %s\n", __func__);

	
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(1);

	
	rc = camera_sensor_power_disable(reg_8921_lvs6);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable\
			(\"8921_lvs6\") FAILED %d\n", rc);

	mdelay(1);

	
	if (jet_use_ext_1v2() == 0)
		rc = camera_sensor_power_disable(reg_8921_l9);

	if (rc < 0)
		pr_err("[CAM] sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	return rc;
}

#ifdef CONFIG_IMX175_ACT
static struct i2c_board_info imx175_actuator_i2c_info = {
	I2C_BOARD_INFO("imx175_act", 0x11),
};

static struct msm_actuator_info imx175_actuator_info = {
	.board_info     = &imx175_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = JET_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif

static struct msm_camera_csi_lane_params imx175_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_imx175_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= JET_GPIO_CAM_PWDN,
	.vcm_pwd	= JET_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &imx175_csi_lane_params,
};

static struct camera_led_est msm_camera_sensor_imx175_led_table[] = {
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,
		.min_step = 58,
		.max_step = 256
	},
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 350,
		.min_step = 54,
		.max_step = 57
	},
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 400,
		.lumen_value = 440,
		.min_step = 50,
		.max_step = 53
	},
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 600,
		.lumen_value = 625,
		.min_step = 46,
		.max_step = 49
	},
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,
		.min_step = 0,
		.max_step = 45    
	},
	{
		.enable = 2,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,
		.min_step = 0,
		.max_step = 270
	},
	{
		.enable = 0,
		.led_state = FL_MODE_OFF,
		.current_ma = 0,
		.lumen_value = 0,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 0,
		.led_state = FL_MODE_TORCH,
		.current_ma = 150,
		.lumen_value = 150,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 2,     
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,
		.min_step = 271,
		.max_step = 317    
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL5,
		.current_ma = 500,
		.lumen_value = 500,
		.min_step = 25,
		.max_step = 26
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 750,
		.min_step = 271,
		.max_step = 325
	},
	{
		.enable = 0,
		.led_state = FL_MODE_TORCH_LEVEL_2,
		.current_ma = 200,
		.lumen_value = 75,
		.min_step = 0,
		.max_step = 40
	},
};

static struct camera_led_info msm_camera_sensor_imx175_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 750,  
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_imx175_led_table),
};

static struct camera_flash_info msm_camera_sensor_imx175_flash_info = {
	.led_info = &msm_camera_sensor_imx175_led_info,
	.led_est_table = msm_camera_sensor_imx175_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_imx175_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 14,
	.low_cap_limit_dual	= 0,
	.flash_info             = &msm_camera_sensor_imx175_flash_info,
};

static struct msm_camera_sensor_flash_data flash_imx175 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_flash_src,
#endif
};

static struct msm_camera_sensor_info msm_camera_sensor_imx175_data = {
	.sensor_name	= "imx175",
	.camera_power_on = jet_imx175_vreg_on,
	.camera_power_off = jet_imx175_vreg_off,
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx175,
	.sensor_platform_info = &sensor_imx175_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
#ifdef CONFIG_IMX175_ACT
	.actuator_info = &imx175_actuator_info,
#endif
	.use_rawchip = RAWCHIP_ENABLE,
	.flash_cfg = &msm_camera_sensor_imx175_flash_cfg, 
};
#endif /* CONFIG_IMX175_2LANE */

#ifdef CONFIG_S5K6A1GX
static int jet_s5k6a1gx_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	pr_info("sensor_power_enable(\"8921_l8\", 2.8V) == %d\n", rc);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	udelay(50);

	
	if (jet_use_ext_1v2()) { 
		rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
		if (rc < 0) {
			pr_err("sensor_power_enable\
				(\"8921_lvs6\", 1.8V) FAILED %d\n", rc);
			goto enable_io_fail;
		}
	} else {
		rc = gpio_request(JET_GPIO_V_CAM2_D1V8_EN, "s5k6a1gx");
		pr_info("power IO gpio_request,%d\n", JET_GPIO_V_CAM2_D1V8_EN);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", JET_GPIO_V_CAM2_D1V8_EN);
			goto enable_io_fail;
		}
		gpio_direction_output(JET_GPIO_V_CAM2_D1V8_EN, 1);
		gpio_free(JET_GPIO_V_CAM2_D1V8_EN);
		udelay(50);

		
		rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
		if (rc < 0) {
			pr_err("sensor_power_enable\
				(\"8921_lvs6\", 1.8V) FAILED %d\n", rc);
			goto enable_i2c_pullup_fail;
		}
	}
	udelay(50);
	
	rc = gpio_request(JET_GPIO_CAM2_RSTz, "s5k6a1gx");
	pr_info("reset pin gpio_request,%d\n", JET_GPIO_CAM2_RSTz);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", JET_GPIO_CAM2_RSTz);
		goto enable_rst_fail;
	}
	gpio_direction_output(JET_GPIO_CAM2_RSTz, 1);
	gpio_free(JET_GPIO_CAM2_RSTz);
	udelay(50);

	
	if (jet_use_ext_1v2()) { 
		rc = gpio_request(JET_GPIO_V_CAM2_D1V8_EN, "s5k6a1gx");
		pr_info("digital gpio_request,%d\n", JET_GPIO_V_CAM2_D1V8_EN);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", JET_GPIO_V_CAM2_D1V8_EN);
			goto enable_digital_fail;
		}
		gpio_direction_output(JET_GPIO_V_CAM2_D1V8_EN, 1);
		gpio_free(JET_GPIO_V_CAM2_D1V8_EN);
		udelay(50);
	} else {
		rc = gpio_request(JET_GPIO_V_CAM_D1V2_EN, "s5k6a1gx");
		pr_info("digital gpio_request,%d\n", JET_GPIO_V_CAM_D1V2_EN);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", JET_GPIO_V_CAM_D1V2_EN);
			goto enable_digital_fail;
		}
		gpio_direction_output(JET_GPIO_V_CAM_D1V2_EN, 1);
		gpio_free(JET_GPIO_V_CAM_D1V2_EN);
	}
	udelay(50);

	return rc;

enable_digital_fail:
	rc = gpio_request(JET_GPIO_CAM2_RSTz, "s5k6a1gx");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", JET_GPIO_CAM2_RSTz);
	else {
		gpio_direction_output(JET_GPIO_CAM2_RSTz, 0);
		gpio_free(JET_GPIO_CAM2_RSTz);
	}
enable_rst_fail:
	camera_sensor_power_disable(reg_8921_lvs6);
enable_i2c_pullup_fail:
	rc = gpio_request(JET_GPIO_V_CAM2_D1V8_EN, "s5k6a1gx");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", JET_GPIO_V_CAM2_D1V8_EN);
	else {
		gpio_direction_output(JET_GPIO_V_CAM2_D1V8_EN, 0);
		gpio_free(JET_GPIO_V_CAM2_D1V8_EN);
	}
enable_io_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	return rc;
}

static int jet_s5k6a1gx_vreg_off(void)
{
	int rc;
	pr_info("%s\n", __func__);

	
	if (jet_use_ext_1v2()) { 
		rc = gpio_request(JET_GPIO_V_CAM2_D1V8_EN, "s5k6a1gx");
		pr_info("digital gpio_request,%d\n", JET_GPIO_V_CAM2_D1V8_EN);
		if (rc < 0)
			pr_err("GPIO(%d) request failed", JET_GPIO_V_CAM2_D1V8_EN);
		else {
			gpio_direction_output(JET_GPIO_V_CAM2_D1V8_EN, 0);
			gpio_free(JET_GPIO_V_CAM2_D1V8_EN);
		}

	} else {
		rc = gpio_request(JET_GPIO_V_CAM_D1V2_EN, "s5k6a1gx");
		pr_info("digital gpio_request,%d\n", JET_GPIO_V_CAM_D1V2_EN);
		if (rc < 0)
			pr_err("GPIO(%d) request failed", JET_GPIO_V_CAM_D1V2_EN);
		else {
			gpio_direction_output(JET_GPIO_V_CAM_D1V2_EN, 0);
			gpio_free(JET_GPIO_V_CAM_D1V2_EN);
		}
	}
	udelay(50);

	
	rc = gpio_request(JET_GPIO_CAM2_RSTz, "s5k6a1gx");
	pr_info("reset pin gpio_request,%d\n", JET_GPIO_CAM2_RSTz);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", JET_GPIO_CAM2_RSTz);
	else {
		gpio_direction_output(JET_GPIO_CAM2_RSTz, 0);
		gpio_free(JET_GPIO_CAM2_RSTz);
	}
	udelay(50);

	
	if (jet_use_ext_1v2()) { 
		rc = camera_sensor_power_disable(reg_8921_lvs6);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_lvs6\") FAILED %d\n", rc);
	} else {
		rc = gpio_request(JET_GPIO_V_CAM2_D1V8_EN, "s5k6a1gx");
		pr_info("power io gpio_request,%d\n", JET_GPIO_V_CAM2_D1V8_EN);
		if (rc < 0)
			pr_err("GPIO(%d) request failed", JET_GPIO_V_CAM2_D1V8_EN);
		else {
			gpio_direction_output(JET_GPIO_V_CAM2_D1V8_EN, 0);
			gpio_free(JET_GPIO_V_CAM2_D1V8_EN);
		}
	}
	udelay(50);

	
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable(\"8921_l8\") FAILED %d\n", rc);
	udelay(50);

	return rc;
}

static struct msm_camera_csi_lane_params s5k6a1gx_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_s5k6a1gx_board_info = {
	.mount_angle = 270,
	.mirror_flip = CAMERA_SENSOR_MIRROR,
	.sensor_reset_enable = 0,
	.sensor_reset	= JET_GPIO_CAM2_RSTz,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.csi_lane_params = &s5k6a1gx_csi_lane_params,
};

static struct msm_camera_sensor_flash_data flash_s5k6a1gx = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k6a1gx_data = {
	.sensor_name	= "s5k6a1gx",
	.sensor_reset	= JET_GPIO_CAM2_RSTz,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.camera_power_on = jet_s5k6a1gx_vreg_on,
	.camera_power_off = jet_s5k6a1gx_vreg_off,
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_s5k6a1gx,
	.sensor_platform_info = &sensor_s5k6a1gx_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_DISABLE,
};
#endif /* CONFIG_S5K6A1GX */

static void config_cam_id(int status)
{
	static uint32_t cam_id_gpio_start[] = {
		GPIO_CFG(JET_GPIO_CAM_ID, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	};

	static uint32_t cam_id_gpio_end[] = {
		GPIO_CFG(JET_GPIO_CAM_ID, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	};
	pr_info("config_cam_id(): status=%d\n",status);
	if(status)
		gpio_tlmm_config(cam_id_gpio_start[0], GPIO_CFG_ENABLE);
	else
		gpio_tlmm_config(cam_id_gpio_end[0], GPIO_CFG_ENABLE);
}

static struct i2c_board_info msm_camera_boardinfo[] = {
#ifdef CONFIG_S5K3H2YX
	{
	I2C_BOARD_INFO("s5k3h2yx", 0x20 >> 1),
	.platform_data = &msm_camera_sensor_s5k3h2yx_data,
	},
#endif
#ifdef CONFIG_S5K6A1GX
	{
	I2C_BOARD_INFO("s5k6a1gx", 0x6C >> 1),
	.platform_data = &msm_camera_sensor_s5k6a1gx_data,
	},
#endif
};

struct msm_camera_board_info jet_camera_board_info = {
	.board_info = msm_camera_boardinfo,
	.num_i2c_board_info = ARRAY_SIZE(msm_camera_boardinfo),
};

static struct i2c_board_info msm_camera_boardinfo_2nd[] = {
#ifdef CONFIG_IMX175_2LANE
	{
	I2C_BOARD_INFO("imx175", 0x20 >> 1),
	.platform_data = &msm_camera_sensor_imx175_data,
	},
#endif
#ifdef CONFIG_S5K6A1GX
	{
	I2C_BOARD_INFO("s5k6a1gx", 0x6C >> 1),
	.platform_data = &msm_camera_sensor_s5k6a1gx_data,
	},
#endif
};

struct msm_camera_board_info jet_camera_board_info_2nd = {
	.board_info = msm_camera_boardinfo_2nd,
	.num_i2c_board_info = ARRAY_SIZE(msm_camera_boardinfo_2nd),
};
#endif /* CONFIG_MSM_CAMERA */

void __init jet_init_camera(void)
{
#ifdef CONFIG_MSM_CAMERA
	pr_info("%s", __func__);

	msm_gpiomux_install(jet_cam_configs,
			ARRAY_SIZE(jet_cam_configs));

	config_cam_id(1); 
	msleep(2);

	if (gpio_get_value(JET_GPIO_CAM_ID) == 0){
		i2c_register_board_info(MSM_8960_GSBI4_QUP_I2C_BUS_ID,
			jet_camera_board_info.board_info,
			jet_camera_board_info.num_i2c_board_info);
	}else{ 
		i2c_register_board_info(MSM_8960_GSBI4_QUP_I2C_BUS_ID,
			jet_camera_board_info_2nd.board_info,
			jet_camera_board_info_2nd.num_i2c_board_info);
	}
	config_cam_id(0); 

	platform_device_register(&msm_rawchip_device);

	platform_device_register(&msm_camera_server);
	platform_device_register(&msm8960_device_i2c_mux_gsbi4);
	platform_device_register(&msm8960_device_csiphy0);
	platform_device_register(&msm8960_device_csiphy1);
	platform_device_register(&msm8960_device_csid0);
	platform_device_register(&msm8960_device_csid1);
	platform_device_register(&msm8960_device_ispif);
	platform_device_register(&msm8960_device_vfe);
	platform_device_register(&msm8960_device_vpe);
#endif /* CONFIG_MSM_CAMERA */
}
