/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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

#include <linux/i2c.h>
#include <linux/gpio.h>

#include <asm/mach-types.h>

#include <mach/camera.h>
#include <mach/msm_bus_board.h>
#include <mach/gpiomux.h>

#include "devices.h"
#include "board-t6.h"

#include <mach/board.h>
#include <linux/spi/spi.h>

#include "board-mahimahi-flashlight.h"
#ifdef CONFIG_MSM_CAMERA_FLASH
#include <linux/htc_flashlight.h>
#endif

#ifdef pr_err
#undef pr_err
#endif
#define pr_err(fmt, args...) \
	printk(KERN_ERR "[CAM][ERR] " pr_fmt(fmt), ## args)

#ifdef pr_info
#undef pr_info
#endif
#define pr_info(fmt, args...) \
	printk(KERN_INFO "[CAM] " pr_fmt(fmt), ## args)

#define CAM_PIN_GPIO_RAW_RSTN	RAW_RST
#define CAM_PIN_GPIO_RAW_INTR0	RAW_INT0
#define CAM_PIN_GPIO_RAW_INTR1	RAW_INT1
#define CAM_PIN_GPIO_CAM_MCLK0	CAM_MCLK0
#define CAM_PIN_GPIO_CAM_MCLK1	CAM_MCLK1

#define CAM_PIN_GPIO_CAM_I2C_DAT	CAM_I2C_SDA
#define CAM_PIN_GPIO_CAM_I2C_CLK	CAM_I2C_SCL

#define CAM_PIN_GPIO_FRONT_CAM_I2C_DAT	SR_I2C_SDA
#define CAM_PIN_GPIO_FRONT_CAM_I2C_CLK	SR_I2C_SCL

#define CAM_PIN_GPIO_MCAM_SPI_CLK	MCAM_FP_SPI_CLK
#define CAM_PIN_GPIO_MCAM_SPI_CS0	MCAM_SPI_CS0
#define CAM_PIN_GPIO_MCAM_SPI_DI	MCAM_FP_SPI_DI
#define CAM_PIN_GPIO_MCAM_SPI_DO	MCAM_FP_SPI_DO
#define CAM_PIN_GPIO_CAM_PWDN	PM8921_GPIO_PM_TO_SYS(CAM_PWDN)
#define CAM_PIN_GPIO_CAM_VCM_PD	PM8921_GPIO_PM_TO_SYS(CAM_VCM_PD)
#define CAM_PIN_GPIO_CAM2_RSTz	CAM2_RSTz

#define CAM_PIN_MAIN_CAMERA_ID PM8921_GPIO_PM_TO_SYS(MAIN_CAM_ID)
#define CAM_PIN_FRONT_CAMERA_ID PM8921_GPIO_PM_TO_SYS(CAM2_ID)

#define MSM_8960_GSBI4_QUP_I2C_BUS_ID 4

extern unsigned int system_rev;
extern unsigned int engineerid;

int gpio_get(int gpio, int *value)
{
	int rc = 0;
	rc = gpio_request(gpio, "gpio");
	if (rc < 0) {
		pr_err("get gpio(%d) fail", gpio);
		return rc;
	}
	*value = gpio_get_value(gpio);
	gpio_free(gpio);

	return rc;
}

int gpio_set(int gpio, int enable)
{
	int rc = 0;

	gpio_tlmm_config(GPIO_CFG(
			  gpio, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
			  GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	rc = gpio_request(gpio, "gpio");
	if (rc < 0) {
		pr_err("set gpio(%d) fail", gpio);
		return rc;
	}
	gpio_direction_output(gpio, enable);
	gpio_free(gpio);

	return rc;
}

static struct gpiomux_setting cam_settings[] = {
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

	{
		.func = GPIOMUX_FUNC_1,
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},

	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_4MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},

	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_4MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_HIGH,
	},

	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_6MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},

	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_6MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_HIGH,
	},

	{
		.func = GPIOMUX_FUNC_1,
		.drv = GPIOMUX_DRV_6MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},

	{
		.func = GPIOMUX_FUNC_1,
		.drv = GPIOMUX_DRV_6MA,
		.pull = GPIOMUX_PULL_NONE,
	},

#if defined(CONFIG_MACH_T6_TL) || defined(CONFIG_MACH_T6_DWG) || defined(CONFIG_MACH_T6_DUG)
	{
		.func = GPIOMUX_FUNC_2,
		.drv = GPIOMUX_DRV_16MA,
		.pull = GPIOMUX_PULL_NONE,
	},
#else
	{
		.func = GPIOMUX_FUNC_2,
		.drv = GPIOMUX_DRV_10MA,
		.pull = GPIOMUX_PULL_NONE,
	},
#endif

	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_6MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},

	{
		.func = GPIOMUX_FUNC_1,
		.drv = GPIOMUX_DRV_4MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_2,
		.drv = GPIOMUX_DRV_4MA,
		.pull = GPIOMUX_PULL_NONE,
	},
};

static struct msm_gpiomux_config t6china_cam_common_configs[] = {
	{
		.gpio = CAM_PIN_GPIO_CAM_MCLK0,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[20],
			[GPIOMUX_SUSPENDED] = &cam_settings[14],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_CAM_MCLK1,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[21],
			[GPIOMUX_SUSPENDED] = &cam_settings[14],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_CAM_I2C_DAT,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[17],
			[GPIOMUX_SUSPENDED] = &cam_settings[19],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_CAM_I2C_CLK,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[17],
			[GPIOMUX_SUSPENDED] = &cam_settings[19],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_FRONT_CAM_I2C_DAT,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[17],
			[GPIOMUX_SUSPENDED] = &cam_settings[17],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_FRONT_CAM_I2C_CLK,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[17],
			[GPIOMUX_SUSPENDED] = &cam_settings[17],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_RAW_INTR0,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[7],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_RAW_INTR1,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[7],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_MCAM_SPI_CLK,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[18],
			[GPIOMUX_SUSPENDED] = &cam_settings[18],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_MCAM_SPI_CS0,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[18],
			[GPIOMUX_SUSPENDED] = &cam_settings[19],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_MCAM_SPI_DI,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[18],
			[GPIOMUX_SUSPENDED] = &cam_settings[18],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_MCAM_SPI_DO,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[18],
			[GPIOMUX_SUSPENDED] = &cam_settings[18],
		},
	},
};

#ifdef CONFIG_MSM_CAMERA

static struct msm_bus_vectors cam_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors cam_preview_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 27648000,
		.ib = 110592000,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors cam_video_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 483063040,
		.ib = 1832252160,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 206807040,
		.ib = 488816640,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors cam_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 700000000,
		.ib = 3749488640U,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 200000000,
		.ib = 1351296000,
	},
};

static struct msm_bus_vectors cam_zsl_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 700000000,
		.ib = 3749488640U,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 200000000,
		.ib = 1351296000,
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

static int t6china_csi_vreg_on(void);
static int t6china_csi_vreg_off(void);

struct msm_camera_device_platform_data t6china_msm_camera_csi_device_data[] = {
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate = 228570000,
		.csid_core = 0,
		.camera_csi_on = t6china_csi_vreg_on,
		.camera_csi_off = t6china_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
		.csid_core = 0,
		.is_csiphy = 1,
		.is_csid = 1,
		.is_ispif = 1,
		.is_vpe = 1,
	},
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate = 228570000,
		.csid_core = 1,
		.camera_csi_on = t6china_csi_vreg_on,
		.camera_csi_off = t6china_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
		.csid_core = 1,
		.is_csiphy = 1,
		.is_csid = 1,
		.is_ispif = 1,
		.is_vpe = 1,
	},
};

static struct regulator *reg_8921_lvs4;
static struct regulator *reg_8921_l2;
static struct regulator *reg_8921_l8;
#if defined(CONFIG_VD6869) || defined(CONFIG_OV4688)
static struct regulator *reg_8921_l29;
#endif
static struct regulator *reg_8921_l22;
static struct regulator *reg_8921_l23;

#ifdef CONFIG_MSM_CAMERA_FLASH
#if defined(CONFIG_VD6869) || defined(CONFIG_OV4688)
int t6china_flashlight_control(int mode)
{
	pr_info("%s, linear led, mode=%d", __func__, mode);
#ifdef CONFIG_FLASHLIGHT_TPS61310
	return tps61310_flashlight_control(mode);
#else
	return 0;
#endif
}

static struct msm_camera_sensor_flash_src msm_camera_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
	.camera_flash = t6china_flashlight_control,
};
#endif
#endif

static int camera_sensor_power_enable(char *power, unsigned volt,
				      struct regulator **sensor_power)
{
	int rc;

	if (power == NULL)
		return -ENODEV;

	*sensor_power = regulator_get(NULL, power);
	if (IS_ERR(*sensor_power)) {
		pr_err("%s: Unable to get %s\n", __func__, power);
		return -ENODEV;
	}
	if (volt != 1800000) {
		rc = regulator_set_voltage(*sensor_power, volt, volt);
		if (rc < 0) {
			pr_err("%s: unable to set %s voltage to %d rc:%d\n",
			       __func__, power, volt, rc);
			regulator_put(*sensor_power);
			*sensor_power = NULL;
			return -ENODEV;
		}
	}
	rc = regulator_enable(*sensor_power);
	if (rc < 0) {
		pr_err("%s: Enable regulator %s failed\n", __func__, power);
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
		pr_err("%s: Invalid requlator ptr\n", __func__);
		return -ENODEV;
	}

	rc = regulator_disable(sensor_power);
	if (rc < 0)
		pr_err("%s: disable regulator failed\n", __func__);

	regulator_put(sensor_power);
	sensor_power = NULL;
	return rc;
}

static int t6china_main_camera_on = 0;
static int t6china_front_camera_on = 0;
static int t6chian_csi_on = 0;
static int t6chian_camera_on = 0;

int t6china_camera_vreg_on(void)
{
	int rc = 0;
	pr_info("%s \n", __func__);
	t6chian_camera_on++;

	if (t6chian_camera_on > 1) {
		pr_info("%s: just return\n", __func__);
		return rc;
	}

	rc = camera_sensor_power_enable("8921_l22", 3000000, &reg_8921_l22);
	if (rc < 0)
		goto VREG_FAIL_D1V8;
	mdelay(125);

	rc = camera_sensor_power_enable("8921_l23", 1800000, &reg_8921_l23);
	if (rc < 0)
		goto VREG_FAIL_IO1V8;
	mdelay(25);

	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	if (rc < 0)
		goto VREG_FAIL_ANALOG2V8;
	mdelay(10);

	rc = gpio_set(PM8921_GPIO_PM_TO_SYS(MCAM_D1V2_EN), 1);
	if (rc < 0)
		goto VREG_FAIL_1V2;
	mdelay(1);

	return rc;

VREG_FAIL_1V2:
	camera_sensor_power_disable(reg_8921_l8);

VREG_FAIL_ANALOG2V8:
	camera_sensor_power_disable(reg_8921_l23);

VREG_FAIL_IO1V8:
	camera_sensor_power_disable(reg_8921_l22);

VREG_FAIL_D1V8:

	return rc;
}

int t6china_camera_vreg_off(void)
{
	pr_info("%s\n", __func__);
	t6chian_camera_on--;

	if (t6chian_camera_on) {
		pr_info("%s: just return\n", __func__);
		return 0;
	}

	gpio_set(PM8921_GPIO_PM_TO_SYS(MCAM_D1V2_EN), 0);
	camera_sensor_power_disable(reg_8921_l8);
	mdelay(25);
	camera_sensor_power_disable(reg_8921_l23);
	mdelay(125);
	camera_sensor_power_disable(reg_8921_l22);

	return 0;
}

int t6china_main_camera_vreg_on(void)
{
	int rc = 0;
	t6china_main_camera_on++;
	pr_info("%s: t6china_main_camera_on=%d\n", __func__,
		t6china_main_camera_on);

	if (t6china_main_camera_on == 1) {

		rc = camera_sensor_power_enable("8921_l29", 2850000,
						&reg_8921_l29);
		if (rc < 0)
			goto FAIL_VCM_POWER;

		rc = gpio_set(PM8921_GPIO_PM_TO_SYS(CAM_VCM_PD), 1);
		if (rc < 0)
			goto FAIL_VCM_PD;
	}
	rc = t6china_camera_vreg_on();

	if (!t6china_front_camera_on) {
		pr_info("%s: front camera enter suspend mode\n", __func__);
		gpio_set(CAM_PIN_GPIO_CAM2_RSTz, 0);
	}

	if (t6china_main_camera_on == 1) {
		pr_info("%s: main camera exit suspend mode\n", __func__);
		gpio_set(CAM_PIN_GPIO_CAM_PWDN, 1);
		gpio_set(PM8921_GPIO_PM_TO_SYS(MCAM_D1V2_EN), 1);
	}
	return rc;

FAIL_VCM_PD:
	camera_sensor_power_disable(reg_8921_l29);

FAIL_VCM_POWER:
	return rc;

}

int t6china_main_camera_vreg_off(void)
{
	if (t6china_main_camera_on)
		t6china_main_camera_on--;

	if (t6china_main_camera_on == 0) {

		camera_sensor_power_disable(reg_8921_l29);
		gpio_set(PM8921_GPIO_PM_TO_SYS(CAM_VCM_PD), 0);
		gpio_set(CAM_PIN_GPIO_CAM_PWDN, 0);
	}
	return t6china_camera_vreg_off();
}

static int t6china_front_camera_vreg_on(void)
{
	int rc = 0;
	t6china_front_camera_on++;
	pr_info("%s: t6china_front_camera_on=%d\n", __func__,
		t6china_front_camera_on);

	rc = t6china_camera_vreg_on();

	if (!t6china_main_camera_on) {
		pr_info("%s: main camera enter suspend mode\n", __func__);
		gpio_set(CAM_PIN_GPIO_CAM_PWDN, 0);
		gpio_set(PM8921_GPIO_PM_TO_SYS(MCAM_D1V2_EN), 0);
	}
	if (t6china_front_camera_on == 1) {
		pr_info("%s: front camera exit suspend mode\n", __func__);
		gpio_set(CAM_PIN_GPIO_CAM2_RSTz, 1);
	}
	return rc;
}

static int t6china_front_camera_vreg_off(void)
{
	if (t6china_front_camera_on)
		t6china_front_camera_on--;
	if (t6china_front_camera_on == 0) {
		pr_info("%s: front camera enter suspend mode\n", __func__);
		gpio_set(CAM_PIN_GPIO_CAM2_RSTz, 0);
	}

	return t6china_camera_vreg_off();
}

#ifdef CONFIG_RAWCHIPII
static int t6china_use_ext_1v2(void)
{
	return 1;
}

static int t6china_rawchip_vreg_on(void)
{
	int rc = 0;
	pr_info("%s\n", __func__);

	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_lvs4\") FAILED %d\n", rc);
		goto RAW_FAIL_1V8;
	}
	mdelay(5);

	rc = gpio_set(PM8921_GPIO_PM_TO_SYS(V_RAW_1V2_EN), 1);
	if (rc < 0)
		goto RAW_FAIL_1V2;

	return rc;

RAW_FAIL_1V2:
	camera_sensor_power_disable(reg_8921_lvs4);

RAW_FAIL_1V8:
	return rc;
}

static int t6china_rawchip_vreg_off(void)
{
	gpio_set(CAM_PIN_GPIO_CAM_PWDN, 0);
	mdelay(10);

	gpio_set(PM8921_GPIO_PM_TO_SYS(V_RAW_1V2_EN), 0);
	camera_sensor_power_disable(reg_8921_lvs4);

	return 0;
}

static struct msm_camera_rawchip_info t6china_msm_rawchip_board_info = {
	.rawchip_reset = CAM_PIN_GPIO_RAW_RSTN,
	.rawchip_intr0 = MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR0),
	.rawchip_intr1 = MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR1),
	.rawchip_spi_freq = 27,
	.rawchip_mclk_freq = 24,
	.camera_rawchip_power_on = t6china_rawchip_vreg_on,
	.camera_rawchip_power_off = t6china_rawchip_vreg_off,
	.rawchip_use_ext_1v2 = t6china_use_ext_1v2,
};

struct platform_device t6china_msm_rawchip_device = {
	.name = "yushanII",
	.dev = {
		.platform_data = &t6china_msm_rawchip_board_info,
	},
};
#endif

#ifdef CONFIG_AR0260

#endif

#ifdef CONFIG_VD6869
static uint16_t vd6869_back_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0,
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
	CAM_PIN_GPIO_RAW_INTR0,
	CAM_PIN_GPIO_RAW_INTR1,
};
#endif

#ifdef CONFIG_OV4688
static uint16_t ov4688_back_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0,
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
	CAM_PIN_GPIO_RAW_INTR0,
	CAM_PIN_GPIO_RAW_INTR1,
};
#endif

#ifdef CONFIG_AR0260

#endif

static int t6china_csi_vreg_on(void)
{
	pr_info("%s\n", __func__);
	t6chian_csi_on++;
	if (t6chian_csi_on == 1)
		return camera_sensor_power_enable("8921_l2", 1200000,
						  &reg_8921_l2);

	return 0;
}

static int t6china_csi_vreg_off(void)
{
	pr_info("%s\n", __func__);

	t6chian_csi_on--;
	if (t6chian_csi_on) {
		return 0;
	}
	return camera_sensor_power_disable(reg_8921_l2);
}

#ifdef CONFIG_LC898212_ACT
static struct i2c_board_info lc898212_actuator_i2c_info = {
	I2C_BOARD_INFO("lc898212_act", 0x11),
};

static struct msm_actuator_info lc898212_actuator_info = {
	.board_info = &lc898212_actuator_i2c_info,
	.cam_name = MSM_ACTUATOR_MAIN_CAM_1,
	.bus_id = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable = 1,
};
#endif

#ifdef CONFIG_VD6869

static struct msm_camera_gpio_conf vd6869_back_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = vd6869_back_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(vd6869_back_cam_gpio),
};

static struct msm_camera_sensor_info msm_camera_sensor_vd6869_data;
void t6china_yushanii_probed_vd6869(enum htc_camera_image_type_board htc_image)
{
	pr_info("%s htc_image %d", __func__, htc_image);
	msm_camera_sensor_vd6869_data.htc_image = htc_image;
	msm_camera_sensor_vd6869_data.video_hdr_capability &=
	    msm_camera_sensor_vd6869_data.htc_image;
}

static struct msm_camera_csi_lane_params vd6869_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_vd6869_board_info = {
	.mount_angle = 90,
	.pixel_order_default = MSM_CAMERA_PIXEL_ORDER_GR,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.board_control_reset_pin = 1,
	.sensor_reset_enable = 1,
	.sensor_reset = CAM_PIN_GPIO_CAM_PWDN,
	.sensor_pwd = 0,
	.vcm_pwd = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable = 1,
	.csi_lane_params = &vd6869_csi_lane_params,
	.sensor_mount_angle = ANGLE_90,
};

static struct camera_led_est msm_camera_sensor_vd6869_led_table[] = {
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 1500,
		.lumen_value = 1500,
		.min_step = 9,
		.max_step = 28
	},
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 300,
		.min_step = 0,
		.max_step = 8
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 800,
		.lumen_value = 880,
		.min_step = 25,
		.max_step = 26
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 1200,
		.lumen_value = 1250,
		.min_step = 23,
		.max_step = 24
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH,
		.current_ma = 1500,
		.lumen_value = 1450,
		.min_step = 0,
		.max_step = 22
	},

	{
		.enable = 0,
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
		.enable = 0,
		.led_state = FL_MODE_FLASH,
		.current_ma = 1500,
		.lumen_value = 1450,
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

static struct camera_led_info msm_camera_sensor_vd6869_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 1500,
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_vd6869_led_table),
};

static struct camera_flash_info msm_camera_sensor_vd6869_flash_info = {
	.led_info = &msm_camera_sensor_vd6869_led_info,
	.led_est_table = msm_camera_sensor_vd6869_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_vd6869_flash_cfg = {
	.low_temp_limit = 5,
	.low_cap_limit = 14,
	.low_cap_limit_dual = 0,
	.flash_info = &msm_camera_sensor_vd6869_flash_info,
};

static struct msm_camera_sensor_flash_data flash_vd6869 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src = &msm_camera_flash_src,
#endif
};

static struct msm_camera_sensor_info msm_camera_sensor_vd6869_data = {
	.sensor_name = "vd6869",
	.camera_power_on = t6china_main_camera_vreg_on,
	.camera_power_off = t6china_main_camera_vreg_off,
	.camera_yushanii_probed = t6china_yushanii_probed_vd6869,
	.pdata = &t6china_msm_camera_csi_device_data[0],
	.flash_data = &flash_vd6869,
	.sensor_platform_info = &sensor_vd6869_board_info,
	.gpio_conf = &vd6869_back_cam_gpio_conf,
	.csi_if = 1,
	.camera_type = BACK_CAMERA_2D,
#if defined(CONFIG_LC898212_ACT)
	.actuator_info = &lc898212_actuator_info,
#endif
	.use_rawchip = RAWCHIP_DISABLE,
	.htc_image = HTC_CAMERA_IMAGE_YUSHANII_BOARD,
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = HDR_MODE,
	.flash_cfg = &msm_camera_sensor_vd6869_flash_cfg,
	.dual_camera =1,
};

#endif

#ifdef CONFIG_AR0260

static uint16_t ar0260_front_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK1,
};

static struct msm_camera_gpio_conf ar0260_front_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = ar0260_front_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(ar0260_front_cam_gpio),
};

static struct msm_camera_sensor_info msm_camera_sensor_ar0260_data;

void t6china_yushanii_probed_ar0260(enum htc_camera_image_type_board htc_image)
{
	pr_info("%s htc_image %d", __func__, htc_image);
	msm_camera_sensor_ar0260_data.htc_image = htc_image;
	msm_camera_sensor_ar0260_data.video_hdr_capability &=
	    msm_camera_sensor_ar0260_data.htc_image;
}

static struct msm_camera_csi_lane_params ar0260_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_ar0260_board_info = {
	.mount_angle = 270,
	.mirror_flip = CAMERA_SENSOR_MIRROR,
	.sensor_reset_enable = 1,
	.sensor_reset = CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd = 0,
	.vcm_pwd = 0,
	.vcm_enable = 0,
	.csi_lane_params = &ar0260_csi_lane_params,
};

static struct msm_camera_sensor_flash_data flash_ar0260 = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_ar0260_data = {
	.sensor_name = "ar0260",
	.sensor_reset = CAM_PIN_GPIO_CAM2_RSTz,
	.camera_yushanii_probed = t6china_yushanii_probed_ar0260,
	.sensor_pwd = 0,
	.vcm_pwd = 0,
	.vcm_enable = 0,
	.camera_power_on = t6china_front_camera_vreg_on,
	.camera_power_off = t6china_front_camera_vreg_off,
	.pdata = &t6china_msm_camera_csi_device_data[1],
	.flash_data = &flash_ar0260,
	.sensor_platform_info = &sensor_ar0260_board_info,
	.gpio_conf = &ar0260_front_cam_gpio_conf,
	.csi_if = 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_DISABLE,
	.htc_image = HTC_CAMERA_IMAGE_NONE_BOARD,
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = NON_HDR_MODE,
};

#endif

#ifdef CONFIG_AS0260

static uint16_t as0260_front_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK1,
};

static struct msm_camera_gpio_conf as0260_front_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = as0260_front_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(as0260_front_cam_gpio),
};

static struct msm_camera_sensor_info msm_camera_sensor_as0260_data;

void t6china_yushanii_probed_as0260(enum htc_camera_image_type_board htc_image)
{
	pr_info("%s htc_image %d", __func__, htc_image);
	msm_camera_sensor_as0260_data.htc_image = htc_image;
	msm_camera_sensor_as0260_data.video_hdr_capability &=
	    msm_camera_sensor_as0260_data.htc_image;
}

static struct msm_camera_csi_lane_params as0260_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_as0260_board_info = {
	.mount_angle = 270,
	.mirror_flip = CAMERA_SENSOR_MIRROR,
	.sensor_reset_enable = 1,
	.sensor_reset = CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd = 0,
	.vcm_pwd = 0,
	.vcm_enable = 0,
	.csi_lane_params = &as0260_csi_lane_params,
};

static struct msm_camera_sensor_flash_data flash_as0260 = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_as0260_data = {
	.sensor_name = "as0260",
	.sensor_reset = CAM_PIN_GPIO_CAM2_RSTz,
	.camera_yushanii_probed = t6china_yushanii_probed_as0260,
	.sensor_pwd = 0,
	.vcm_pwd = 0,
	.vcm_enable = 0,
	.camera_power_on = t6china_front_camera_vreg_on,
	.camera_power_off = t6china_front_camera_vreg_off,
	.pdata = &t6china_msm_camera_csi_device_data[1],
	.flash_data = &flash_as0260,
	.sensor_platform_info = &sensor_as0260_board_info,
	.gpio_conf = &as0260_front_cam_gpio_conf,
	.csi_if = 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_DISABLE,
	.htc_image = HTC_CAMERA_IMAGE_NONE_BOARD,
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = NON_HDR_MODE,
	.dual_camera = 1,
};

#endif

#ifdef CONFIG_OV4688

static struct msm_camera_gpio_conf ov4688_back_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = ov4688_back_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(ov4688_back_cam_gpio),
};

static struct msm_camera_sensor_info msm_camera_sensor_ov4688_data;
void t6china_yushanii_probed_ov4688(enum htc_camera_image_type_board htc_image)
{
	pr_info("%s htc_image %d", __func__, htc_image);
	msm_camera_sensor_ov4688_data.htc_image = htc_image;
	msm_camera_sensor_ov4688_data.video_hdr_capability &=
	    msm_camera_sensor_ov4688_data.htc_image;
}

static struct msm_camera_csi_lane_params ov4688_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_ov4688_board_info = {
	.mount_angle = 90,
	.pixel_order_default = MSM_CAMERA_PIXEL_ORDER_GR,
	.mirror_flip = CAMERA_SENSOR_MIRROR,
	.board_control_reset_pin = 1,
	.sensor_reset_enable = 1,
	.sensor_reset = CAM_PIN_GPIO_CAM_PWDN,
	.sensor_pwd = 0,
	.vcm_pwd = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable = 1,
	.csi_lane_params = &ov4688_csi_lane_params,
	.sensor_mount_angle = ANGLE_90,
};

static struct camera_led_est msm_camera_sensor_ov4688_led_table[] = {
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 1500,
		.lumen_value = 1500,
		.min_step = 9,
		.max_step = 28
	},
	{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 300,
		.min_step = 0,
		.max_step = 8
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 800,
		.lumen_value = 880,
		.min_step = 25,
		.max_step = 26
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 1200,
		.lumen_value = 1250,
		.min_step = 23,
		.max_step = 24
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH,
		.current_ma = 1500,
		.lumen_value = 1450,
		.min_step = 0,
		.max_step = 22
	},

	{
		.enable = 0,
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
		.enable = 0,
		.led_state = FL_MODE_FLASH,
		.current_ma = 1500,
		.lumen_value = 1450,
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

static struct camera_led_info msm_camera_sensor_ov4688_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 1500,
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_ov4688_led_table),
};

static struct camera_flash_info msm_camera_sensor_ov4688_flash_info = {
	.led_info = &msm_camera_sensor_ov4688_led_info,
	.led_est_table = msm_camera_sensor_ov4688_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_ov4688_flash_cfg = {
	.low_temp_limit = 5,
	.low_cap_limit = 14,
	.low_cap_limit_dual = 0,
	.flash_info = &msm_camera_sensor_ov4688_flash_info,
};

static struct msm_camera_sensor_flash_data flash_ov4688 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src = &msm_camera_flash_src,
#endif
};

static struct msm_camera_sensor_info msm_camera_sensor_ov4688_data = {
	.sensor_name = "ov4688",
	.camera_power_on = t6china_main_camera_vreg_on,
	.camera_power_off = t6china_main_camera_vreg_off,
	.camera_yushanii_probed = t6china_yushanii_probed_ov4688,
	.pdata = &t6china_msm_camera_csi_device_data[0],
	.flash_data = &flash_ov4688,
	.sensor_platform_info = &sensor_ov4688_board_info,
	.gpio_conf = &ov4688_back_cam_gpio_conf,
	.csi_if = 1,
	.camera_type = BACK_CAMERA_2D,
#ifdef CONFIG_LC898212_ACT
	.actuator_info = &lc898212_actuator_info,
#endif
	.use_rawchip = RAWCHIP_DISABLE,
	.htc_image = HTC_CAMERA_IMAGE_YUSHANII_BOARD,
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = HDR_MODE,
	.flash_cfg = &msm_camera_sensor_ov4688_flash_cfg,
	.dual_camera = 1,
};

#endif

static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};

#ifdef CONFIG_I2C

struct i2c_board_info t6china_camera_i2c_boardinfo_ar0260[] = {
#ifdef CONFIG_AR0260
	{
		I2C_BOARD_INFO("ar0260", 0x90 >> 1),
		.platform_data = &msm_camera_sensor_ar0260_data,
	},
#endif
};

struct i2c_board_info t6china_camera_i2c_boardinfo_as0260[] = {
#ifdef CONFIG_AS0260
	{
		I2C_BOARD_INFO("as0260", 0x90 >> 1),
		.platform_data = &msm_camera_sensor_as0260_data,
	},
#endif
};

struct i2c_board_info t6china_camera_i2c_boardinfo_vd6869[] = {
#ifdef CONFIG_VD6869
	{
		I2C_BOARD_INFO("vd6869", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_vd6869_data,
	},
#endif
};

struct i2c_board_info t6china_camera_i2c_boardinfo_ov4688_0x20[] = {
#ifdef CONFIG_OV4688
	{
		I2C_BOARD_INFO("ov4688_0x20", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_ov4688_data,
	},
#endif
};

#endif

int main_camera_id(int *id)
{
	int rc = 0;

	rc = camera_sensor_power_enable("8921_l22", 3000000, &reg_8921_l22);
	if (rc < 0) {
		goto CLEANUP;
	}
	rc = camera_sensor_power_enable("8921_l23", 1800000, &reg_8921_l23);
	if (rc < 0) {
		goto CLEANUP;
	}
	msleep(100);
	rc = gpio_get(CAM_PIN_MAIN_CAMERA_ID, id);

CLEANUP:
	rc = camera_sensor_power_disable(reg_8921_l23);
	rc = camera_sensor_power_disable(reg_8921_l22);

	return rc;
}

void __init t6china_init_cam(void)
{
	int main_id = 0;
	int rc = 0;
	pr_info("%s", __func__);
	msm_gpiomux_install(t6china_cam_common_configs,
			    ARRAY_SIZE(t6china_cam_common_configs));
	platform_device_register(&msm_camera_server);

	platform_device_register(&msm8960_device_i2c_mux_gsbi4);
	platform_device_register(&msm8960_device_csiphy0);
	platform_device_register(&msm8960_device_csiphy1);
	platform_device_register(&msm8960_device_csid0);
	platform_device_register(&msm8960_device_csid1);
	platform_device_register(&msm8960_device_ispif);
	platform_device_register(&msm8960_device_vfe);
	platform_device_register(&msm8960_device_vpe);

	rc = main_camera_id(&main_id);
	if (rc < 0) {
		pr_err("%s can't get main camera id", __func__);
	}
	pr_info("main camera id=%d system_rev=%d engineerid=%d\n", main_id,
		system_rev, engineerid);
#ifdef CONFIG_I2C

	if (main_id) {
		i2c_register_board_info(APQ_8064_GSBI4_QUP_I2C_BUS_ID,
					t6china_camera_i2c_boardinfo_ov4688_0x20,
					ARRAY_SIZE
					(t6china_camera_i2c_boardinfo_ov4688_0x20));
	} else {
		i2c_register_board_info(APQ_8064_GSBI4_QUP_I2C_BUS_ID,
					t6china_camera_i2c_boardinfo_vd6869,
					ARRAY_SIZE
					(t6china_camera_i2c_boardinfo_vd6869));
	}

#ifdef CONFIG_AR0260
	i2c_register_board_info(APQ_8064_GSBI2_QUP_I2C_BUS_ID,
				t6china_camera_i2c_boardinfo_ar0260,
				ARRAY_SIZE
				(t6china_camera_i2c_boardinfo_ar0260));
#endif

#ifdef CONFIG_AS0260
	i2c_register_board_info(APQ_8064_GSBI2_QUP_I2C_BUS_ID,
				t6china_camera_i2c_boardinfo_as0260,
				ARRAY_SIZE
				(t6china_camera_i2c_boardinfo_as0260));
#endif

#endif
}

#endif
