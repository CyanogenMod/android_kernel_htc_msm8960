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
#include "board-fighter.h"

#include <linux/spi/spi.h>

#include "board-mahimahi-flashlight.h"
#ifdef CONFIG_MSM_CAMERA_FLASH
#include <linux/htc_flashlight.h>
#endif

#ifdef CONFIG_MSM_CAMERA
static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};

static struct gpiomux_setting cam_settings[5] = {
	{
		.func = GPIOMUX_FUNC_GPIO, /*suspend - I(L) 8MA*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},

	{
		.func = GPIOMUX_FUNC_1, /*active 1 - A FUNC1 8MA*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*active 2 - O(L) 8MA*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},

	{
		.func = GPIOMUX_FUNC_1, /*active 3 - A FUNC1 8MA*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_2, /*active 4 - A FUNC2 8MA*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},
};

static struct msm_gpiomux_config fighter_cam_configs[] = {
	{
		.gpio = FIGHTER_GPIO_CAM_MCLK1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[4], /*A FUNC2 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
	{
		.gpio = FIGHTER_GPIO_CAM_MCLK0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[1], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[2], /*O(L) 8MA*/
		},
	},
	{
		.gpio = FIGHTER_GPIO_CAM_I2C_DAT,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = FIGHTER_GPIO_CAM_I2C_CLK,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3], /*A FUNC1 8MA*/
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
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

static int fighter_csi_vreg_on(void);
static int fighter_csi_vreg_off(void);

struct msm_camera_device_platform_data msm_camera_csi_device_data[] = {
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 0,
		.camera_csi_on = fighter_csi_vreg_on,
		.camera_csi_off = fighter_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
		.is_csiphy = 1,
		.is_csid   = 1,
		.is_ispif  = 1,
		.is_vpe    = 1,
	},
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 1,
		.camera_csi_on = fighter_csi_vreg_on,
		.camera_csi_off = fighter_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
		.is_csiphy = 1,
		.is_csid   = 1,
		.is_ispif  = 1,
		.is_vpe    = 1,
	},
};

#ifdef CONFIG_MSM_CAMERA_FLASH
int fighter_flashlight_control(int mode)
{
	pr_debug("%s, linear led, mode=%d", __func__, mode);
#ifdef CONFIG_FLASHLIGHT_TPS61310
	return tps61310_flashlight_control(mode);
#else
	return 0;
#endif
}

static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
	.camera_flash = fighter_flashlight_control,
};
#endif /* CONFIG_MSM_CAMERA_FLASH */

static struct regulator *reg_8921_l2;
static struct regulator *reg_8921_l8;
static struct regulator *reg_8921_l9;
static struct regulator *reg_8921_l12;
static struct regulator *reg_8921_lvs6;

static uint16_t msm_cam_gpio_tbl[] = {
	FIGHTER_GPIO_CAM_MCLK0, /*CAMIF_MCLK*/
	FIGHTER_GPIO_CAM_MCLK1,
};

static struct msm_camera_gpio_conf gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = msm_cam_gpio_tbl,
	.cam_gpio_tbl_size = ARRAY_SIZE(msm_cam_gpio_tbl),
};

static int camera_sensor_power_enable(char *power, unsigned volt, struct regulator **sensor_power)
{
	int rc;

	if (power == NULL)
		return -ENODEV;

	*sensor_power = regulator_get(NULL, power);

	if (IS_ERR(*sensor_power)) {
		pr_err("[CAM] %s: Unable to get %s\n", __func__, power);
		return -ENODEV;
	}

	if (volt != 1800000) {
		rc = regulator_set_voltage(*sensor_power, volt, volt);
		if (rc < 0) {
			pr_err("[CAM] %s: unable to set %s voltage to %d rc:%d\n",
					__func__, power, volt, rc);
			regulator_put(*sensor_power);
			*sensor_power = NULL;
			return -ENODEV;
		}
	}

	rc = regulator_enable(*sensor_power);
	if (rc < 0) {
		pr_err("[CAM] %s: Enable regulator %s failed\n", __func__, power);
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
		pr_err("[CAM] %s: Invalid requlator ptr\n", __func__);
		return -ENODEV;
	}

	rc = regulator_disable(sensor_power);
	if (rc < 0)
		pr_err("[CAM] %s: disable regulator failed\n", __func__);

	regulator_put(sensor_power);
	sensor_power = NULL;
	return rc;
}

static int fighter_csi_vreg_on(void)
{
	pr_info("%s\n", __func__);
	return camera_sensor_power_enable("8921_l2", 1200000, &reg_8921_l2);
}

static int fighter_csi_vreg_off(void)
{
	pr_info("%s\n", __func__);
	return camera_sensor_power_disable(reg_8921_l2);
}

#ifdef CONFIG_S5K3H2YX
static int fighter_s5k3h2yx_vreg_on(void)
{
	int rc;
	pr_info("[CAM] %s\n", __func__);

	/* VCM */
	rc = camera_sensor_power_enable("8921_l9", 2850000, &reg_8921_l9); /* maxson should be 2800000 */
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}

	/* IO */
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8921_lvs6\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}

	/* analog */
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}

	udelay(50);

	/* digital */
	if (system_rev >= 2) {	/* fighter use LDO by GPIO95 on XC */
		rc = gpio_request(FIGHTER_GPIO_CAM_EXT_LDO, "CAM_EXT_LDO");
		if (rc < 0)
			pr_err("[CAM] %s: gpio %d request failed, rc=%d\n", __func__,
				FIGHTER_GPIO_CAM_EXT_LDO, rc);
		else {
			gpio_direction_output(FIGHTER_GPIO_CAM_EXT_LDO, 1);
			gpio_free(FIGHTER_GPIO_CAM_EXT_LDO);
		}
	}
	else
		rc = camera_sensor_power_enable("8921_l12", 1200000, &reg_8921_l12);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8921_l12\", 1.5) FAILED %d\n", rc);
		goto enable_digital_fail;
	}

	return rc;

enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_lvs6);
enable_io_fail:
	camera_sensor_power_disable(reg_8921_l9);
enable_vcm_fail:
	return rc;
}

static int fighter_s5k3h2yx_vreg_off(void)
{
	int rc = 0;

	pr_info("[CAM] %s\n", __func__);

	/* VCM */
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable(\"8921_l9\") FAILED %d\n", rc);

	/* digital */
	if (system_rev >= 2) {	/* fighter use LDO by GPIO95 on XC */
		rc = gpio_request(FIGHTER_GPIO_CAM_EXT_LDO, "CAM_EXT_LDO");
		if (rc < 0)
			pr_err("[CAM] %s: gpio %d request failed, rc=%d\n", __func__,
				FIGHTER_GPIO_CAM_EXT_LDO, rc);
		else {
			gpio_direction_output(FIGHTER_GPIO_CAM_EXT_LDO, 0);
			gpio_free(FIGHTER_GPIO_CAM_EXT_LDO);
		}
	}
	else
		rc = camera_sensor_power_disable(reg_8921_l12);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable(\"8921_l12\") FAILED %d\n", rc);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable(\"8921_l8\") FAILED %d\n", rc);

	udelay(50);

	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs6);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable(\"8921_lvs6\") FAILED %d\n", rc);

	return rc;
}

#ifdef CONFIG_S5K3H2YX_ACT
static struct i2c_board_info s5k3h2yx_actuator_i2c_info = {
	I2C_BOARD_INFO("s5k3h2yx_act", 0x11),
};

static struct msm_actuator_info s5k3h2yx_actuator_info = {
	.board_info     = &s5k3h2yx_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = FIGHTER_GPIO_CAM_VCM_PD,
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
	.sensor_pwd	= FIGHTER_GPIO_CAM_PWDN,
	.vcm_pwd	= FIGHTER_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &s5k3h2yx_csi_lane_params,
};

/* Andrew_Cheng linear led 20111205 MB */
/*
  150 mA FL_MODE_FLASH_LEVEL1
  200 mA FL_MODE_FLASH_LEVEL2
  300 mA FL_MODE_FLASH_LEVEL3
  400 mA FL_MODE_FLASH_LEVEL4
  500 mA FL_MODE_FLASH_LEVEL5
  600 mA FL_MODE_FLASH_LEVEL6
  700 mA FL_MODE_FLASH_LEVEL7
*/
static struct camera_led_est msm_camera_sensor_s5k3h2yx_led_table[] = {
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL1,
		.current_ma = 150,
		.lumen_value = 150,
		.min_step = 50,
		.max_step = 70
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,
		.min_step = 44,
		.max_step = 52
	},
		{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 300,
		.min_step = 29,
		.max_step = 34
	},
		{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 400,
		.lumen_value = 400,
		.min_step = 27,
		.max_step = 28
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
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 600,
		.lumen_value = 600,
		.min_step = 23,
		.max_step = 24
	},
		{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL7,
		.current_ma = 700,
		.lumen_value = 700,
		.min_step = 21,
		.max_step = 22
	},
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 750,
		.min_step = 0,
		.max_step = 43
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
		.lumen_value = 750,
		.min_step = 271,
		.max_step = 325
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
		.enable = 3,
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

static struct camera_led_info msm_camera_sensor_s5k3h2yx_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 800,
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
/* Andrew_Cheng linear led 20111205 ME */

static struct msm_camera_sensor_flash_data flash_s5k3h2yx = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_flash_src,
#endif
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3h2yx_data = {
	.sensor_name	= "s5k3h2yx",
	.camera_power_on = fighter_s5k3h2yx_vreg_on,
	.camera_power_off = fighter_s5k3h2yx_vreg_off,
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_s5k3h2yx,
	.sensor_platform_info = &sensor_s5k3h2yx_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
#ifdef CONFIG_S5K3H2YX_ACT
	.actuator_info = &s5k3h2yx_actuator_info,
#endif
	.use_rawchip = RAWCHIP_DISABLE,
	.flash_cfg = &msm_camera_sensor_s5k3h2yx_flash_cfg, /* Andrew_Cheng linear led 20111205 */
	.camera_type = BACK_CAMERA_2D,
};

struct platform_device fighter_camera_sensor_s5k3h2yx = {
	.name	= "msm_camera_s5k3h2yx",
	.dev	= {
		.platform_data = &msm_camera_sensor_s5k3h2yx_data,
	},
};
#endif /* CONFIG_S5K3H2YX */

#ifdef CONFIG_IMX105
static int fighter_imx105_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	/* VCM */
	rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9); /* based on 8921_l9 definition needs to be between 2.8 to 2.85v */
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}

	pr_info("imx105 VCM on\n");


	/* analog */
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8); /* based on 8921_l9 definition needs to be between 2.8 to 2.85v */
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}

	pr_info("imx105 A on\n");

	udelay(50);

	/* digital */
	if (system_rev >= 2) {	/* fighter use LDO by GPIO95 on XC */
		pr_err("system_rev >=2:vreg on");
		rc = gpio_request(FIGHTER_GPIO_CAM_EXT_LDO, "CAM_EXT_LDO");
		if (rc < 0)
			pr_err(" %s: gpio %d request failed, rc=%d\n", __func__,
					FIGHTER_GPIO_CAM_EXT_LDO, rc);
		else {
			gpio_direction_output(FIGHTER_GPIO_CAM_EXT_LDO, 1);
			gpio_free(FIGHTER_GPIO_CAM_EXT_LDO);
		}
	} else {
		pr_err("system_rev <2:vreg on");
		rc = camera_sensor_power_enable("8921_l12", 1260000, &reg_8921_l12); /*Tom 20120224 increase to 1.26v for 876MHz*/
	}

	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_l12\", 1.2V) FAILED %d\n", rc);
		goto enable_digital_fail;
	}

	pr_info("imx105 D on\n");
	/* IO */
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_lvs6\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}

	pr_info("imx105 IO on\n");

	return rc;

enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l12);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_io_fail:
	camera_sensor_power_disable(reg_8921_lvs6);
enable_vcm_fail:
	camera_sensor_power_disable(reg_8921_l9);
	return rc;
}

static int fighter_imx105_vreg_off(void)
{
	int rc = 0;

	pr_info("%s\n", __func__);

	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs6);
	pr_info("imx105 IO off\n");
	if (rc < 0)
		pr_err("sensor_power_disable(\"8921_lvs6\") FAILED %d\n", rc);

	/* digital */
	if (system_rev >= 2) {	/* fighter use LDO by GPIO95 on XC */
		pr_err("system_rev >=2:vreg off");
		rc = gpio_request(FIGHTER_GPIO_CAM_EXT_LDO, "CAM_EXT_LDO");
		if (rc < 0)
			pr_err("%s: gpio %d request failed, rc=%d\n", __func__,
					FIGHTER_GPIO_CAM_EXT_LDO, rc);
		else {
			gpio_direction_output(FIGHTER_GPIO_CAM_EXT_LDO, 0);
			gpio_free(FIGHTER_GPIO_CAM_EXT_LDO);
		}
	} else {
		pr_err("system_rev <2:vreg off");
		rc = camera_sensor_power_disable(reg_8921_l12);
		pr_info("imx105 D off\n");
	}
	if (rc < 0)
		pr_err("sensor_power_disable(\"8921_l12\") FAILED %d\n", rc);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	pr_info("imx105 A off\n");
	if (rc < 0)
		pr_err("sensor_power_disable(\"8921_l8\") FAILED %d\n", rc);

	udelay(50);

	/* VCM */
	rc = camera_sensor_power_disable(reg_8921_l9);
	pr_info("imx105 VCM off\n");
	if (rc < 0)
		pr_err("sensor_power_disable(\"8921_l9\") FAILED %d\n", rc);

	return rc;
}

#ifdef CONFIG_IMX105_ACT
static struct i2c_board_info imx105_actuator_i2c_info = {
	I2C_BOARD_INFO("imx105_act", 0x1B),
};

static struct msm_actuator_info imx105_actuator_info = {
	.board_info     = &imx105_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = FIGHTER_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif

static struct msm_camera_csi_lane_params imx105_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_imx105_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= FIGHTER_GPIO_CAM_PWDN,
	.vcm_pwd	= FIGHTER_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &imx105_csi_lane_params,
};

/* Andrew_Cheng linear led 20111205 MB */
/*
  150 mA FL_MODE_FLASH_LEVEL1
  200 mA FL_MODE_FLASH_LEVEL2
  300 mA FL_MODE_FLASH_LEVEL3
  400 mA FL_MODE_FLASH_LEVEL4
  500 mA FL_MODE_FLASH_LEVEL5
  600 mA FL_MODE_FLASH_LEVEL6
  700 mA FL_MODE_FLASH_LEVEL7
*/

/* Andrew_Cheng linear led 20111205 MB */
static struct camera_led_est msm_camera_sensor_imx105_led_table[] = {
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
		.enable = 0,
		.led_state = FL_MODE_FLASH,
		.current_ma = 800,
		.lumen_value = 800,
		.min_step = 33,
		.max_step = 42
	},
	{
		.enable = 0,
		.led_state = FL_MODE_PRE_FLASH,
		.current_ma = 100,
		.lumen_value = 100,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH,
		.current_ma = 800,
		.lumen_value = 800,
		.min_step = 40,
		.max_step = 52
	},
	{
		.enable = 0,
		.led_state = FL_MODE_TORCH_LEVEL_2,
		.current_ma = 200,
		.lumen_value = 264,
		.min_step = 0,
		.max_step = 39
	},
};

static struct camera_led_info msm_camera_sensor_imx105_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 800,
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_imx105_led_table),
};

static struct camera_flash_info msm_camera_sensor_imx105_flash_info = {
	.led_info = &msm_camera_sensor_imx105_led_info,
	.led_est_table = msm_camera_sensor_imx105_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_imx105_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 14,
	.low_cap_limit_dual	= 0,
	.flash_info		= &msm_camera_sensor_imx105_flash_info,
};
/* Andrew_Cheng linear led 20111205 ME */

static struct msm_camera_sensor_flash_data flash_imx105 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_flash_src,
#endif
};

static struct msm_camera_sensor_info msm_camera_sensor_imx105_data = {
	.sensor_name		= "imx105",
	.camera_power_on	= fighter_imx105_vreg_on,
	.camera_power_off	= fighter_imx105_vreg_off,
	.pdata			= &msm_camera_csi_device_data[0],
	.flash_data		= &flash_imx105,
	.sensor_platform_info	= &sensor_imx105_board_info,
	.gpio_conf		= &gpio_conf,
	.csi_if			= 1,
#ifdef CONFIG_IMX105_ACT
	.actuator_info		= &imx105_actuator_info,
#endif
	.use_rawchip		= RAWCHIP_DISABLE,
	.camera_type		= BACK_CAMERA_2D,
	.flash_cfg		= &msm_camera_sensor_imx105_flash_cfg,
	/* Andrew_Cheng linear led 20111205 */
};

struct platform_device fighter_camera_sensor_imx105 = {
	.name	= "msm_camera_imx105",
	.dev	= {
		.platform_data = &msm_camera_sensor_imx105_data,
	},
};
#endif /* CONFIG_IMX105 */

#ifdef CONFIG_MT9V113
static int fighter_mt9v113_vreg_on(void)
{
	int rc;

	pr_info("[CAM] %s\n", __func__);

	/* IO */
	rc = gpio_request(FIGHTER_GPIO_V_CAM2_D1V8_EN, "CAM_D1V8_EN");
	pr_info("[CAM] fighter_mt9v113_vreg_on %d 1v8\n", FIGHTER_GPIO_V_CAM2_D1V8_EN);
	if (rc) {
		pr_err("[CAM] %s:GPIO_CAM_D1V8_EN gpio %d request failed, rc=%d\n", __func__,  FIGHTER_GPIO_V_CAM2_D1V8_EN, rc);
		goto init_fail;
	}
	gpio_direction_output(FIGHTER_GPIO_V_CAM2_D1V8_EN, 1);
	gpio_free(FIGHTER_GPIO_V_CAM2_D1V8_EN);

	udelay(50);

	/* Reset */
	rc = gpio_request(FIGHTER_GPIO_CAM2_RSTz, "mt9v113");
	if (!rc) {
		gpio_direction_output(FIGHTER_GPIO_CAM2_RSTz, 1);
		msleep(2);
	}	else
		pr_err("[CAM] %s:FIGHTER_GPIO_CAM2_RSTz gpio %d request failed, rc=%d\n", __func__,  FIGHTER_GPIO_CAM2_RSTz, rc);
	gpio_free(FIGHTER_GPIO_CAM2_RSTz);

	udelay(50);

	/* analog */
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	pr_info("[CAM] sensor_power_enable(\"8921_l8\", 2.8V) == %d\n", rc);

	if (rc < 0)
		goto init_fail;

	udelay(50);

	/* digital */
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
	pr_info("[CAM] sensor_power_enable(\"8921_lvs6\", 1.8V) == %d\n", rc);

	if (rc < 0)
		goto init_fail;

	udelay(50);

init_fail:
	return rc;
}

static int fighter_mt9v113_vreg_off(void)
{
	int rc;

	pr_info("[CAM] %s\n", __func__);

	/* digital */
	rc = camera_sensor_power_disable(reg_8921_lvs6);
	pr_info("[CAM] camera_sensor_power_disable(\"8921_lvs6\", 1.8V) == %d\n", rc);

	if (rc < 0)
		goto init_fail;

	udelay(50);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	pr_info("[CAM] camera_sensor_power_disable(\"8921_l8\", 2.8V) == %d\n", rc);

	if (rc < 0)
		goto init_fail;

	udelay(50);

	/* IO */
	rc = gpio_request(FIGHTER_GPIO_V_CAM2_D1V8_EN, "CAM_D1V8_EN");
	pr_info("[CAM] fighter_mt9v113_vreg_off %d 1v8\n", FIGHTER_GPIO_V_CAM2_D1V8_EN);
	if (rc) {
		pr_err("[CAM] %s:GPIO_CAM_D1V8_EN gpio %d request failed, rc=%d\n", __func__,  FIGHTER_GPIO_V_CAM2_D1V8_EN, rc);
		goto init_fail;
	}
	gpio_direction_output(FIGHTER_GPIO_V_CAM2_D1V8_EN, 0);
	gpio_free(FIGHTER_GPIO_V_CAM2_D1V8_EN);

	udelay(50);

	/* Reset */
	rc = gpio_request(FIGHTER_GPIO_CAM2_RSTz, "mt9v113");
	if (!rc) {
		gpio_direction_output(FIGHTER_GPIO_CAM2_RSTz, 0);
		msleep(2);
	} else
		pr_err("[CAM] %s:FIGHTER_GPIO_CAM2_RSTz gpio %d request failed, rc=%d\n", __func__,	FIGHTER_GPIO_CAM2_RSTz, rc);
	gpio_free(FIGHTER_GPIO_CAM2_RSTz);

	udelay(50);

init_fail:
		return rc;
}

static struct msm_camera_csi_lane_params mt9v113_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_mt9v113_board_info = {
	.mount_angle = 270,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= FIGHTER_GPIO_CAM2_RSTz,
	.sensor_pwd	= FIGHTER_GPIO_CAM_PWDN,
	.vcm_pwd	= 0,
	.vcm_enable	= 1,
	.csi_lane_params = &mt9v113_csi_lane_params,
};

static struct msm_camera_sensor_flash_data flash_mt9v113 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9v113_data = {
	.sensor_name	= "mt9v113",
	.sensor_reset	= FIGHTER_GPIO_CAM2_RSTz,
	.sensor_pwd	= FIGHTER_GPIO_CAM_PWDN,
	.vcm_pwd	= 0,
	.vcm_enable	= 1,
	.camera_power_on = fighter_mt9v113_vreg_on,
	.camera_power_off = fighter_mt9v113_vreg_off,
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_mt9v113,
	.sensor_platform_info = &sensor_mt9v113_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_DISABLE,
};

struct platform_device fighter_camera_sensor_mt9v113 = {
	.name	= "msm_camera_mt9v113",
	.dev	= {
		.platform_data = &msm_camera_sensor_mt9v113_data,
	},
};
#endif /* CONFIG_MT9V113 */

struct i2c_board_info fighter_camera_i2c_boardinfo[] = {
#ifdef CONFIG_S5K3H2YX
	{
		I2C_BOARD_INFO("s5k3h2yx", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_s5k3h2yx_data,
	},
#endif
#ifdef CONFIG_IMX105
	{
		I2C_BOARD_INFO("imx105", 0x1A),
		.platform_data = &msm_camera_sensor_imx105_data,
	},
#endif
#ifdef CONFIG_MT9V113
	{
		I2C_BOARD_INFO("mt9v113", 0x3C),
		.platform_data = &msm_camera_sensor_mt9v113_data,
	},
#endif
};

struct msm_camera_board_info fighter_camera_board_info = {
	.board_info = fighter_camera_i2c_boardinfo,
	.num_i2c_board_info = ARRAY_SIZE(fighter_camera_i2c_boardinfo),
};
#endif /* CONFIG_MSM_CAMERA */

void __init fighter_init_camera(void)
{
#ifdef CONFIG_MSM_CAMERA
	msm_gpiomux_install(fighter_cam_configs,
			ARRAY_SIZE(fighter_cam_configs));

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
