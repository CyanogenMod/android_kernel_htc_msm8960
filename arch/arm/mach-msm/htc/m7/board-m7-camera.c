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

#include "devices.h"
#include "board-m7.h"

#include <linux/spi/spi.h>

#include "board-mahimahi-flashlight.h"
#ifdef CONFIG_MSM_CAMERA_FLASH
#include <linux/htc_flashlight.h>
#endif

#if defined(CONFIG_RUMBAS_ACT)
void m7_vcm_vreg_on(void);
void m7_vcm_vreg_off(void);
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

#define CAM_PIN_PMGPIO_V_RAW_1V2_EN	0
#define CAM_PIN_GPIO_V_CAM_D1V2_EN_XA	V_CAM_D1V2_EN_XA
#define CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB	PM8921_GPIO_PM_TO_SYS(V_CAM_D1V2_EN_XB)
#define CAM_PIN_GPIO_MCAM_D1V2_EN	PM8921_GPIO_PM_TO_SYS(MCAM_D1V2_EN)
#define CAM_PIN_GPIO_V_CAM_1V8_EN	PM8921_GPIO_PM_TO_SYS(V_CAM_1V8_EN)

#define CAM_PIN_GPIO_V_RAW_1V8_EN	PM8921_GPIO_PM_TO_SYS(V_RAW_1V8_EN)
#define CAM_PIN_GPIO_RAW_RSTN	RAW_RST
#define CAM_PIN_GPIO_RAW_INTR0	RAW_INT0
#define CAM_PIN_GPIO_RAW_INTR1	RAW_INT1_XB
#define CAM_PIN_GPIO_CAM_MCLK0	CAM_MCLK0	
#define CAM_PIN_GPIO_CAM_SEL	CAM_SEL	

#define CAM_PIN_GPIO_CAM_I2C_DAT	I2C4_DATA_CAM	
#define CAM_PIN_GPIO_CAM_I2C_CLK	I2C4_CLK_CAM	

#define CAM_PIN_GPIO_MCAM_SPI_CLK	MCAM_SPI_CLK
#define CAM_PIN_GPIO_MCAM_SPI_CS0	MCAM_SPI_CS0
#define CAM_PIN_GPIO_MCAM_SPI_DI	MCAM_SPI_DI
#define CAM_PIN_GPIO_MCAM_SPI_DO	MCAM_SPI_DO
#define CAM_PIN_GPIO_CAM_PWDN	PM8921_GPIO_PM_TO_SYS(CAM1_PWDN)	
#define CAM_PIN_GPIO_CAM_VCM_PD	PM8921_GPIO_PM_TO_SYS(CAM_VCM_PD)	
#define CAM_PIN_GPIO_CAM2_RSTz	CAM2_RSTz
#define CAM_PIN_GPIO_CAM2_STANDBY	0

#define CAM_PIN_CAMERA_ID PM8921_GPIO_PM_TO_SYS(MAIN_CAM_ID)
#define MSM_8960_GSBI4_QUP_I2C_BUS_ID 4	

extern unsigned int system_rev;
extern unsigned int engineerid; 

#if defined(CONFIG_ACT_OIS_BINDER)
extern void HtcActOisBinder_i2c_add_driver(void* i2c_client);
extern void HtcActOisBinder_open_init(void);
extern void HtcActOisBinder_power_down(void);
extern int32_t HtcActOisBinder_act_set_ois_mode(int ois_mode);
extern int32_t HtcActOisBinder_mappingTbl_i2c_write(int startup_mode, void* sensor_actuator_info);
#endif


#if defined(CONFIG_VD6869)
static struct msm_camera_sensor_info msm_camera_sensor_vd6869_data;
#endif
#if defined(CONFIG_IMX135)
static struct msm_camera_sensor_info msm_camera_sensor_imx135_data;
#endif
#if defined(CONFIG_AR0260)
static struct msm_camera_sensor_info msm_camera_sensor_ar0260_data;
#endif
#if defined(CONFIG_OV2722)
static struct msm_camera_sensor_info msm_camera_sensor_ov2722_data;
#endif
#if defined(CONFIG_OV4688)
static struct msm_camera_sensor_info msm_camera_sensor_ov4688_data;
#endif

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

	{
		.func = GPIOMUX_FUNC_2, 
		.drv = GPIOMUX_DRV_6MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, 
		.drv = GPIOMUX_DRV_6MA,
		.pull = GPIOMUX_PULL_DOWN,
		.dir = GPIOMUX_IN,
	},
};

static struct msm_gpiomux_config m7_cam_common_configs[] = {
	{
		.gpio = CAM_PIN_GPIO_CAM_MCLK0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[17], 
			[GPIOMUX_SUSPENDED] = &cam_settings[14], 
		},
	},
	{
		.gpio = CAM_PIN_GPIO_CAM_I2C_DAT,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[17], 
			[GPIOMUX_SUSPENDED] = &cam_settings[16],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_CAM_I2C_CLK,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[17], 
			[GPIOMUX_SUSPENDED] = &cam_settings[16],
		},
	},
	{
		.gpio = CAM_PIN_GPIO_RAW_INTR0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7], 
			[GPIOMUX_SUSPENDED] = &cam_settings[8], 
		},
	},
	{
		.gpio = CAM_PIN_GPIO_RAW_INTR1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[7], 
			[GPIOMUX_SUSPENDED] = &cam_settings[8], 
		},
	},
	
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_CLK,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[18], 
			[GPIOMUX_SUSPENDED] = &cam_settings[14], 
		},
	},
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_CS0,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[18], 
			[GPIOMUX_SUSPENDED] = &cam_settings[14], 
		},
	},
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_DI,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[18], 
			[GPIOMUX_SUSPENDED] = &cam_settings[19], 
		},
	},
	{
		.gpio      = CAM_PIN_GPIO_MCAM_SPI_DO,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_settings[18], 
			[GPIOMUX_SUSPENDED] = &cam_settings[14], 
		},
	},
};

#ifdef CONFIG_MSM_CAMERA

#if 1	

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
		.ab  = 27648000,
		.ib  = 110592000,
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
		.ab  = 483063040,
		.ib  = 1832252160,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 206807040,
		.ib  = 488816640,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 274423680,
		.ib  = 1097694720,
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
		.ab  = 540000000,
		.ib  = 1350000000,
	},
};

static struct msm_bus_vectors cam_zsl_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 700000000, 
		.ib  = 3749488640U, 
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
		.ab  = 200000000, 
		.ib  = 1351296000,
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


#if 1	

static int m7_csi_vreg_on(void);
static int m7_csi_vreg_off(void);

struct msm_camera_device_platform_data m7_msm_camera_csi_device_data[] = {
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 0,
		.camera_csi_on = m7_csi_vreg_on,
		.camera_csi_off = m7_csi_vreg_off,
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
		.camera_csi_on = m7_csi_vreg_on,
		.camera_csi_off = m7_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
		.csid_core = 1,
		.is_csiphy = 1,
		.is_csid   = 1,
		.is_ispif  = 1,
		.is_vpe    = 1,
	},
};

#ifdef CONFIG_MSM_CAMERA_FLASH
#if defined(CONFIG_IMX175) || defined(CONFIG_IMX135) || defined(CONFIG_VD6869) || defined(CONFIG_IMX091) || defined(CONFIG_S5K3H2YX) || defined(CONFIG_OV4688)
int m7_flashlight_control(int mode)
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
	.camera_flash = m7_flashlight_control,
};
#endif
#endif


static struct regulator *reg_8921_lvs1;
#if defined(CONFIG_IMX135) || defined(CONFIG_VD6869) || defined(CONFIG_AR0260) || defined(CONFIG_OV2722) || defined(CONFIG_OV4688)
static struct regulator *reg_8921_lvs4;
#endif

static struct regulator *reg_8921_l2;
#if defined(CONFIG_IMX135) || defined(CONFIG_VD6869) || defined(CONFIG_AR0260) || defined(CONFIG_OV2722) || defined(CONFIG_RAWCHIPII)|| defined(CONFIG_OV4688)
static struct regulator *reg_8921_l8;
static struct regulator *reg_8921_l9;
static struct regulator *reg_8921_l12;
static struct regulator *reg_8921_s2;

#endif

int gpio_set(int gpio,int enable)
{
	int rc = 0;
	gpio_tlmm_config(GPIO_CFG(gpio, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	rc = gpio_request(gpio, "gpio");
	if (rc < 0) {
		pr_err("set gpio(%d) fail", gpio);
	    return rc;
	}
	gpio_direction_output(gpio, enable);
	gpio_free(gpio);

	return rc;
}

static int camera_sensor_power_enable(char *power, unsigned volt, struct regulator **sensor_power)
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


int check_yushanII_flag(void)
{
#if defined(CONFIG_VD6869)
	if (msm_camera_sensor_vd6869_data.htc_image == HTC_CAMERA_IMAGE_NONE_BOARD) {
		pr_info("check_yushanII_flag() , NO yushanII , VD6869 htc_image=%d\n", msm_camera_sensor_vd6869_data.htc_image);
		return 0;
	}
#endif
#if defined(CONFIG_IMX135)
	if (msm_camera_sensor_imx135_data.htc_image == HTC_CAMERA_IMAGE_NONE_BOARD) {
		pr_info("check_yushanII_flag() , NO yushanII , IMX135 htc_image=%d\n", msm_camera_sensor_imx135_data.htc_image);
		return 0;
	}
#endif

#if defined(CONFIG_OV4688)
	if (msm_camera_sensor_ov4688_data.htc_image == HTC_CAMERA_IMAGE_NONE_BOARD) {
		pr_info("check_yushanII_flag() , NO yushanII , ov4688 htc_image=%d\n", msm_camera_sensor_ov4688_data.htc_image);
		return 0;
	}
#endif
	pr_info("check_yushanII_flag() , With yushanII\n");
	return 1;
}


#ifdef CONFIG_RAWCHIPII

static int m7_use_ext_1v2(void)
{
	return 1;
}

static int m7_rawchip_vreg_on(void)
{
	int rc;
	int gpio_cam_d1v2_en=0;

	pr_info("%s\n", __func__);

	if (system_rev <= 1) { 
		
		rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
		pr_info("rawchip external 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		if (rc) {
			pr_err("rawchip on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
			goto enable_1v8_fail;
		}
		gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 1);
		gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	} else { 
		pr_info("%s: 8921_lvs1 1800000\n", __func__);
		rc = camera_sensor_power_enable("8921_lvs1", 1800000, &reg_8921_lvs1);
		pr_info("%s: 8921_lvs1 1800000 (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("sensor_power_enable\
				(\"8921_lvs1\", 1.8V) FAILED %d\n", rc);
			goto enable_1v8_fail;
		}
	}
	mdelay(5);
	
	if (system_rev > 1){
	    pr_info("%s: 8921_s2 1200000\n", __func__);
		rc = camera_sensor_power_enable("8921_s2", 1200000, &reg_8921_s2);
		pr_info("%s: 8921_s2 1200000 (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("sensor_power_enable\
				(\"8921_s2\", 1.2V) FAILED %d\n", rc);
			goto enable_v_raw_1v2_fail;
		}
		mdelay(5);
	}
	

#if 0	
	if (system_rev >= 1) { 
		if (m7_use_ext_1v2()) { 
#else
	{
		{
#endif	
			mdelay(1);

			if (system_rev == 0) { 
				gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM_D1V2_EN_XA;
			} else  { 
				gpio_cam_d1v2_en = CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB;
			}
			rc = gpio_request(gpio_cam_d1v2_en, "rawchip");
			pr_info("rawchip external 1v2 gpio_request,%d rc(%d)\n", gpio_cam_d1v2_en, rc);
			if (rc < 0) {
				pr_err("GPIO(%d) request failed", gpio_cam_d1v2_en);
				goto enable_ext_1v2_fail;
			}
			gpio_direction_output(gpio_cam_d1v2_en, 1);
			gpio_free(gpio_cam_d1v2_en);
		}
	}

	return rc;


enable_ext_1v2_fail://HTC_CAM_Start chuck
	
		if (system_rev > 1){ 
			rc = camera_sensor_power_disable(reg_8921_s2);
			if (rc < 0)
				pr_err("sensor_power_disable\
					(\"8921_s2\") FAILED %d\n", rc);
			mdelay(1);
		}
	

enable_v_raw_1v2_fail://END
#if 0	

	if (system_rev >= 0 && system_rev <= 3) { 
	rc = gpio_request(CAM_PIN_V_RAW_1V2_EN, "V_RAW_1V2_EN");
	if (rc)
		pr_err("rawchip on\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_V_RAW_1V2_EN, rc);
	gpio_direction_output(CAM_PIN_V_RAW_1V2_EN, 0);
	gpio_free(CAM_PIN_V_RAW_1V2_EN);
	}
enable_1v2_fail:
#endif	

	if (system_rev <= 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
		if (rc)
			pr_err("rawchip on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	} else { 
		rc = camera_sensor_power_disable(reg_8921_lvs1);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_lvs1\") FAILED %d\n", rc);
	}

enable_1v8_fail:
	return rc;
}

static int m7_rawchip_vreg_off(void)
{
	int rc = 0;
	int gpio_cam_d1v2_en=0;

	pr_info("%s\n", __func__);
#if 0	
	if (system_rev >= 1) { 
		if (m7_use_ext_1v2()) { 
#else
	{
		{
#endif	
			if (system_rev == 0) { 
				gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM_D1V2_EN_XA;
			} else  { 
				gpio_cam_d1v2_en = CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB;
			}
			rc = gpio_request(gpio_cam_d1v2_en, "rawchip");
			if (rc)
				pr_err("rawchip off(\
					\"gpio %d\", 1.2V) FAILED %d\n",
					gpio_cam_d1v2_en, rc);
			gpio_direction_output(gpio_cam_d1v2_en, 0);
			gpio_free(gpio_cam_d1v2_en);
			mdelay(1);
		}
	}

	if (system_rev > 1){ 
		rc = camera_sensor_power_disable(reg_8921_s2);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_s2\") FAILED %d\n", rc);
		mdelay(1);
	}


	if (system_rev <= 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
		if (rc)
			pr_err("rawchip off\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	} else { 
	    pr_info("%s: 8921_lvs1 1800000-off\n", __func__);
		rc = camera_sensor_power_disable(reg_8921_lvs1);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_lvs1\") FAILED %d\n", rc);
	}

	return rc;
}

static struct msm_camera_rawchip_info m7_msm_rawchip_board_info = {
	.rawchip_reset	= CAM_PIN_GPIO_RAW_RSTN,
	.rawchip_intr0	= MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR0),
	.rawchip_intr1	= MSM_GPIO_TO_INT(CAM_PIN_GPIO_RAW_INTR1),
	.rawchip_spi_freq = 27, 
	.rawchip_mclk_freq = 24, 
	.camera_rawchip_power_on = m7_rawchip_vreg_on,
	.camera_rawchip_power_off = m7_rawchip_vreg_off,
	.rawchip_use_ext_1v2 = m7_use_ext_1v2,
};

struct platform_device m7_msm_rawchip_device = {
	.name	= "yushanII",
	.dev	= {
		.platform_data = &m7_msm_rawchip_board_info,
	},
};
#endif


#if defined(CONFIG_IMX091) || defined(CONFIG_S5K3H2YX) || defined(CONFIG_S5K6A1GX)
static uint16_t msm_cam_gpio_tbl[] = {
	CAM_PIN_GPIO_CAM_MCLK0, 
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};
#endif

#ifdef CONFIG_AR0260
static uint16_t ar0260_front_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0, 
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};
#endif

#ifdef CONFIG_OV2722
static uint16_t ov2722_front_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0, 
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};
#endif

#ifdef CONFIG_IMX175
static uint16_t imx175_back_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0, 
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};
#endif

#ifdef CONFIG_IMX135
static uint16_t imx135_back_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0, 
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};
#endif

#ifdef CONFIG_VD6869
static uint16_t vd6869_back_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0, 
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
	CAM_PIN_GPIO_RAW_INTR0,
	
};
#endif

#ifdef CONFIG_OV4688
static uint16_t ov4688_back_cam_gpio[] = {
	CAM_PIN_GPIO_CAM_MCLK0, 
	CAM_PIN_GPIO_RAW_INTR0,
	CAM_PIN_GPIO_RAW_INTR1,
	CAM_PIN_GPIO_MCAM_SPI_CLK,
	CAM_PIN_GPIO_MCAM_SPI_CS0,
	CAM_PIN_GPIO_MCAM_SPI_DI,
	CAM_PIN_GPIO_MCAM_SPI_DO,
};

#endif

#if defined(CONFIG_IMX091) || defined(CONFIG_S5K3H2YX) || defined(CONFIG_S5K6A1GX)
static struct msm_camera_gpio_conf gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = msm_cam_gpio_tbl,
	.cam_gpio_tbl_size = ARRAY_SIZE(msm_cam_gpio_tbl),
};
#endif

#ifdef CONFIG_AR0260
static struct msm_camera_gpio_conf ar0260_front_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = ar0260_front_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(ar0260_front_cam_gpio),
};
#endif

#ifdef CONFIG_OV2722
static struct msm_camera_gpio_conf ov2722_front_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = ov2722_front_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(ov2722_front_cam_gpio),
};
#endif

#ifdef CONFIG_IMX175
static struct msm_camera_gpio_conf imx175_back_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = imx175_back_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(imx175_back_cam_gpio),
};
#endif

#ifdef CONFIG_IMX135
static struct msm_camera_gpio_conf imx135_back_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = imx135_back_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(imx135_back_cam_gpio),
};
#endif

#ifdef CONFIG_VD6869
static struct msm_camera_gpio_conf vd6869_back_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = vd6869_back_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(vd6869_back_cam_gpio),
};
#endif

#ifdef CONFIG_OV4688
static struct msm_camera_gpio_conf ov4688_back_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = ov4688_back_cam_gpio,
	.cam_gpio_tbl_size = ARRAY_SIZE(ov4688_back_cam_gpio),
};
#endif

static int m7_csi_vreg_on(void)
{
	pr_info("%s\n", __func__);
	return camera_sensor_power_enable("8921_l2", 1200000, &reg_8921_l2);
}

static int m7_csi_vreg_off(void)
{
	pr_info("%s\n", __func__);
	return camera_sensor_power_disable(reg_8921_l2);
}

static void update_yushanII_flag(enum htc_camera_image_type_board htc_image)
{
	pr_info("update_yushanII_flag() , htc_image=%d\n", htc_image);

#if defined(CONFIG_OV4688)
	msm_camera_sensor_ov4688_data.htc_image = htc_image;
	msm_camera_sensor_ov4688_data.video_hdr_capability &= msm_camera_sensor_ov4688_data.htc_image;
#endif

#if defined(CONFIG_VD6869)
	msm_camera_sensor_vd6869_data.htc_image = htc_image;
	msm_camera_sensor_vd6869_data.video_hdr_capability &= msm_camera_sensor_vd6869_data.htc_image;
#endif
#if defined(CONFIG_IMX135)
	msm_camera_sensor_imx135_data.htc_image = htc_image;
	msm_camera_sensor_imx135_data.video_hdr_capability &= msm_camera_sensor_imx135_data.htc_image;
#endif
#if defined(CONFIG_AR0260)
	msm_camera_sensor_ar0260_data.htc_image = htc_image;
	msm_camera_sensor_ar0260_data.video_hdr_capability &= msm_camera_sensor_ar0260_data.htc_image;
#endif
#if defined(CONFIG_OV2722)
	msm_camera_sensor_ov2722_data.htc_image = htc_image;
	msm_camera_sensor_ov2722_data.video_hdr_capability &= msm_camera_sensor_ov2722_data.htc_image;
#endif

	return;
}

static void m7_yushanii_probed(enum htc_camera_image_type_board htc_image)
{
	pr_info("%s htc_image %d", __func__, htc_image);
	update_yushanII_flag(htc_image);
}

#if defined(CONFIG_AD5823_ACT)
#if (defined(CONFIG_IMX175) || defined(CONFIG_IMX091))
static struct i2c_board_info ad5823_actuator_i2c_info = {
	I2C_BOARD_INFO("ad5823_act", 0x1C),
};

static struct msm_actuator_info ad5823_actuator_info = {
	.board_info     = &ad5823_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif
#endif

#if defined(CONFIG_TI201_ACT)
#if (defined(CONFIG_IMX175) || defined(CONFIG_IMX091) || defined(CONFIG_VD6869))
static struct i2c_board_info ti201_actuator_i2c_info = {
	I2C_BOARD_INFO("ti201_act", 0x1C),
};

static struct msm_actuator_info ti201_actuator_info = {
	.board_info     = &ti201_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
#if defined(CONFIG_ACT_OIS_BINDER)
	.oisbinder_i2c_add_driver = HtcActOisBinder_i2c_add_driver,
	.oisbinder_open_init = HtcActOisBinder_open_init,
	.oisbinder_power_down = HtcActOisBinder_open_init,
	.oisbinder_act_set_ois_mode = HtcActOisBinder_act_set_ois_mode,
	.oisbinder_mappingTbl_i2c_write = HtcActOisBinder_mappingTbl_i2c_write,
#endif
};
#endif
#endif

#if defined(CONFIG_AD5816_ACT)
#if (defined(CONFIG_IMX175) || defined(CONFIG_IMX091))
static struct i2c_board_info ad5816_actuator_i2c_info = {
	I2C_BOARD_INFO("ad5816_act", 0x1C),
};

static struct msm_actuator_info ad5816_actuator_info = {
	.board_info     = &ad5816_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif
#endif

#if defined(CONFIG_RUMBAS_ACT)
static struct i2c_board_info rumbas_actuator_i2c_info = {
	I2C_BOARD_INFO("rumbas_act", 0x32),
};

static struct msm_actuator_info rumbas_actuator_info = {
	.board_info     = &rumbas_actuator_i2c_info,
	.bus_id         = APQ_8064_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
	.otp_diviation	= 85,
#if defined(CONFIG_RUMBAS_ACT)
	.vcm_wa_vreg_on = m7_vcm_vreg_on,
	.vcm_wa_vreg_off = m7_vcm_vreg_off,
#endif
};
#endif

#ifdef CONFIG_LC898212_ACT
static struct i2c_board_info lc898212_actuator_i2c_info = {
	I2C_BOARD_INFO("lc898212_act", 0x11),
};

static struct msm_actuator_info lc898212_actuator_info = {
	.board_info     = &lc898212_actuator_i2c_info,
	.bus_id         = APQ_8064_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif

#ifdef CONFIG_IMX175
static int m7_imx175_vreg_on(void)
{
	int rc;
	unsigned gpio_cam_d1v2_en = 0;
	pr_info("%s\n", __func__);

	
	pr_info("%s: 8921_l9 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9);
	pr_info("%s: 8921_l9 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);

#if 0	
	if (system_rev == 0 || !m7_use_ext_1v2()) { 
#else
	if (1) {
#endif	

	
	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN\n", __func__);
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
	mdelay(1);
	}

	pr_info("%s: CAM_PIN_GPIO_MCAM_D1V2_EN\n", __func__);
	rc = gpio_request(CAM_PIN_GPIO_MCAM_D1V2_EN, "MCAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_MCAM_D1V2_EN (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_MCAM_D1V2_EN, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_MCAM_D1V2_EN, 1);
	gpio_free(CAM_PIN_GPIO_MCAM_D1V2_EN);
	mdelay(1);

	
	gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V8_EN;

	rc = gpio_request(gpio_cam_d1v2_en, "CAM2_IO_D1V8_EN");
	pr_info("digital gpio_request,%d\n", gpio_cam_d1v2_en);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", gpio_cam_d1v2_en);
		goto enable_digital_fail;
	}
	gpio_direction_output(gpio_cam_d1v2_en, 1);
	gpio_free(gpio_cam_d1v2_en);
	mdelay(1);

	
#if 0	
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
#else
	pr_info("%s: 8921_lvs4 1800000\n", __func__);
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);	
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
#endif	
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}
	mdelay(1);

	
	pr_info("%s: 8921_l8 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	pr_info("%s: 8921_l8 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(1);

	
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "CAM2_RST");
	pr_info("reset pin gpio_request,%d\n", CAM_PIN_GPIO_CAM2_RSTz);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 1);
	gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	mdelay(1);

	return rc;

enable_io_fail:
#if 0	
	if (system_rev == 0 || !m7_use_ext_1v2()) { 
#else
	{
#endif	
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
	}
	}
enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_l9);
enable_vcm_fail:
	return rc;
}

static int m7_imx175_vreg_off(void)
{
	int rc = 0;
	unsigned gpio_cam_d1v2_en = 0;

	pr_info("%s\n", __func__);

	
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(1);

	gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM2_D1V8_EN;
	rc = gpio_request(gpio_cam_d1v2_en, "CAM_D1V2_EN");
	pr_info("digital gpio_request,%d\n", gpio_cam_d1v2_en);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", gpio_cam_d1v2_en);
	else {
		gpio_direction_output(gpio_cam_d1v2_en, 0);
		gpio_free(gpio_cam_d1v2_en);
	}
	mdelay(1);

	rc = gpio_request(CAM_PIN_GPIO_MCAM_D1V2_EN, "MCAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_MCAM_D1V2_EN (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_disabled\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_MCAM_D1V2_EN, rc);
	}
	gpio_direction_output(CAM_PIN_GPIO_MCAM_D1V2_EN, 0);
	gpio_free(CAM_PIN_GPIO_MCAM_D1V2_EN);
	mdelay(1);

#if 0	
	if (system_rev == 0 || !m7_use_ext_1v2()) { 
#else
	if (1) {
#endif	
	
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
	}
	mdelay(1);
	}

	
#if 0	
	rc = camera_sensor_power_disable(reg_8921_lvs6);
#else
	rc = camera_sensor_power_disable(reg_8921_lvs4);
#endif	
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_lvs6\") FAILED %d\n", rc);

	mdelay(1);

	
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "CAM_RST");
	pr_info("reset pin gpio_request,%d\n", CAM_PIN_GPIO_CAM2_RSTz);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	else {
		gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
		gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	}
	mdelay(1);

	
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	return rc;
}

static struct msm_camera_csi_lane_params imx175_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_imx175_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= CAM_PIN_GPIO_CAM_PWDN,
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &imx175_csi_lane_params,
};

static struct camera_led_est msm_camera_sensor_imx175_led_table[] = {
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,
		.min_step = 29,
		.max_step = 128
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 350,
		.min_step = 27,
		.max_step = 28
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 400,
		.lumen_value = 440,
		.min_step = 25,
		.max_step = 26
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 600,
		.lumen_value = 625,
		.min_step = 23,
		.max_step = 24
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,
		.min_step = 0,
		.max_step = 22    
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
	.low_cap_limit_dual = 0,
	.flash_info             = &msm_camera_sensor_imx175_flash_info,
};


static struct msm_camera_sensor_flash_data flash_imx175 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_camera_flash_src,
#endif

};

#if defined(CONFIG_AD5823_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_AD5816_ACT)
static struct msm_actuator_info *imx175_actuator_table[] = {
#if defined(CONFIG_AD5823_ACT)
    &ad5823_actuator_info,
#endif
#if defined(CONFIG_TI201_ACT)
    &ti201_actuator_info,
#endif
#if defined(CONFIG_AD5816_ACT)
    &ad5816_actuator_info,
#endif
};
#endif


static struct msm_camera_sensor_info msm_camera_sensor_imx175_data = {
	.sensor_name	= "imx175",
	.camera_power_on = m7_imx175_vreg_on,
	.camera_power_off = m7_imx175_vreg_off,
	.pdata	= &m7_msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx175,
	.sensor_platform_info = &sensor_imx175_board_info,
	.gpio_conf = &imx175_back_cam_gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
#if defined(CONFIG_AD5823_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_AD5816_ACT)
	.num_actuator_info_table = ARRAY_SIZE(imx175_actuator_table),
	.actuator_info_table = &imx175_actuator_table[0],
#endif
#ifdef CONFIG_AD5823_ACT
	.actuator_info = &ti201_actuator_info,
#endif
	.use_rawchip = RAWCHIP_ENABLE,
	.video_hdr_capability = NON_HDR_MODE,
	.flash_cfg = &msm_camera_sensor_imx175_flash_cfg, 
};

#endif	


#ifdef CONFIG_IMX135
static int m7_imx135_vreg_on(void)
{
	int rc;
	int gpio_cam_d1v2_en=0;
	pr_info("%s\n", __func__);

	
	rc = gpio_request(CAM_PIN_GPIO_CAM_SEL, "CAM_SEL");
	pr_info("%s: CAM_PIN_GPIO_CAM_SEL (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable(\"gpio %d\") FAILED %d\n",CAM_PIN_GPIO_CAM_SEL, rc);
		goto enable_mclk_switch_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM_SEL, 0);
	gpio_free(CAM_PIN_GPIO_CAM_SEL);
	mdelay(1);

	
	pr_info("%s: 8921_l9 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9);
	pr_info("%s: 8921_l9 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);

	
	if (system_rev <= 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
			goto enable_io_fail;
		}
		gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 1);
		gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	} else { 
		if (check_yushanII_flag() == 0) {
			pr_info("%s: 8921_lvs1 1800000\n", __func__);
			rc = camera_sensor_power_enable("8921_lvs1", 1800000, &reg_8921_lvs1);
			pr_info("%s: 8921_lvs1 1800000 (%d)\n", __func__, rc);
			if (rc < 0) {
				pr_err("sensor_power_enable\
					(\"8921_lvs1\", 1.8V) FAILED %d\n", rc);
				goto enable_io_fail;
			}
		}
	}
	mdelay(5);

	if (system_rev == 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_1V8_EN, "V_CAM_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
			goto enable_io_fail_2;
		}
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_1V8_EN, 1);
		gpio_free(CAM_PIN_GPIO_V_CAM_1V8_EN);
		mdelay(5);
	} else if (system_rev >= 2) { 
		pr_info("%s: 8921_lvs4 1800000\n", __func__);
		rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
		pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("sensor_power_enable\
				(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
			goto enable_io_fail_2;
		}
		mdelay(5);
	}

	
	pr_info("%s: 8921_l8 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	pr_info("%s: 8921_l8 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(1);

	
	if (system_rev == 0) { 
		gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM_D1V2_EN_XA;
	} else  { 
		gpio_cam_d1v2_en = CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB;
	}
	pr_info("%s: gpio_cam_d1v2_en\n", __func__);
	rc = gpio_request(gpio_cam_d1v2_en, "CAM_D1V2_EN");
	pr_info("%s: gpio_cam_d1v2_en (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable\
			(\"gpio %d\", 1.05V) FAILED %d\n",
			gpio_cam_d1v2_en, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(gpio_cam_d1v2_en, 1);
	gpio_free(gpio_cam_d1v2_en);
	mdelay(5);

	return rc;


enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	if (system_rev == 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_1V8_EN, "V_CAM_1V8_EN");
		if (rc < 0) {
			pr_err("I/O 1v8 off\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		} else {
			gpio_direction_output(CAM_PIN_GPIO_V_CAM_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_CAM_1V8_EN);
		}
	} else if (system_rev >= 2) { 
		rc = camera_sensor_power_disable(reg_8921_lvs4);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_lvs4\") FAILED %d\n", rc);
	}
enable_io_fail_2:
	if (system_rev <= 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
		if (rc < 0) {
			pr_err("I/O 1v8 off\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		} else {
			gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
		}
	} else { 
		if (check_yushanII_flag() == 0) {
			rc = camera_sensor_power_disable(reg_8921_lvs1);
			if (rc < 0)
				pr_err("sensor_power_disable\
					(\"8921_lvs1\") FAILED %d\n", rc);
		}
	}
enable_io_fail:
	camera_sensor_power_disable(reg_8921_l9);
enable_vcm_fail:
enable_mclk_switch_fail:

	return rc;
}

static int m7_imx135_vreg_off(void)
{
	int rc = 0;
	int gpio_cam_d1v2_en=0;
	pr_info("%s\n", __func__);

	
	if (system_rev == 0) { 
		gpio_cam_d1v2_en = CAM_PIN_GPIO_V_CAM_D1V2_EN_XA;
	} else  { 
		gpio_cam_d1v2_en = CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB;
	}
	rc = gpio_request(gpio_cam_d1v2_en, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.05V) FAILED %d\n",
			gpio_cam_d1v2_en, rc);
	else {
		gpio_direction_output(gpio_cam_d1v2_en, 0);
		gpio_free(gpio_cam_d1v2_en);
	}
	mdelay(10);

	
	pr_info("%s: 8921_l8 off\n", __func__);
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(10);

	
	if (system_rev == 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_1V8_EN, "V_CAM_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.2V) FAILED %d\n",
				CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		}
		else
		{
			gpio_direction_output(CAM_PIN_GPIO_V_CAM_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_CAM_1V8_EN);
		}
		mdelay(20);
	} else if (system_rev >= 2) { 
		rc = camera_sensor_power_disable(reg_8921_lvs4);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_lvs4\") FAILED %d\n", rc);
		mdelay(20);
	}

	if (system_rev <= 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.2V) FAILED %d\n",
				CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		}
		else
		{
			gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
		}
	} else { 
		if (check_yushanII_flag() == 0) {
			rc = camera_sensor_power_disable(reg_8921_lvs1);
			if (rc < 0)
				pr_err("sensor_power_disable\
					(\"8921_lvs1\") FAILED %d\n", rc);
		}
	}
	mdelay(10);


	
	pr_info("%s: 8921_l9 off\n", __func__);
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	
	rc = gpio_request(CAM_PIN_GPIO_CAM_SEL, "CAM_SEL");
	pr_info("%s: CAM_PIN_GPIO_CAM_SEL (%d)\n", __func__, rc);
	if (rc>=0) {
		gpio_direction_output(CAM_PIN_GPIO_CAM_SEL, 0);
		gpio_free(CAM_PIN_GPIO_CAM_SEL);
	}

	return rc;
}

static struct msm_camera_csi_lane_params imx135_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_imx135_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= CAM_PIN_GPIO_CAM_PWDN,
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &imx135_csi_lane_params,
};

static struct camera_led_est msm_camera_sensor_imx135_led_table[] = {
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,
		.min_step = 29,
		.max_step = 128
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 350,
		.min_step = 27,
		.max_step = 28
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 400,
		.lumen_value = 440,
		.min_step = 25,
		.max_step = 26
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 600,
		.lumen_value = 625,
		.min_step = 23,
		.max_step = 24
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,
		.min_step = 0,
		.max_step = 22    
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

static struct camera_led_info msm_camera_sensor_imx135_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 750,  
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_imx135_led_table),
};

static struct camera_flash_info msm_camera_sensor_imx135_flash_info = {
	.led_info = &msm_camera_sensor_imx135_led_info,
	.led_est_table = msm_camera_sensor_imx135_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_imx135_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 14,
	.low_cap_limit_dual = 0,
	.flash_info             = &msm_camera_sensor_imx135_flash_info,
};


static struct msm_camera_sensor_flash_data flash_imx135 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_camera_flash_src,
#endif

};


static struct msm_camera_sensor_info msm_camera_sensor_imx135_data = {
	.sensor_name	= "imx135",
	.camera_power_on = m7_imx135_vreg_on,
	.camera_power_off = m7_imx135_vreg_off,
	.camera_yushanii_probed = m7_yushanii_probed,
	.pdata	= &m7_msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx135,
	.sensor_platform_info = &sensor_imx135_board_info,
	.gpio_conf = &imx135_back_cam_gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
#if defined(CONFIG_RUMBAS_ACT)
	.actuator_info = &rumbas_actuator_info,
#endif
	.use_rawchip = RAWCHIP_DISABLE,
	.htc_image = HTC_CAMERA_IMAGE_NONE_BOARD,
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = NON_HDR_MODE,
	.flash_cfg = &msm_camera_sensor_imx135_flash_cfg, 
};

#endif	

#ifdef CONFIG_VD6869
static int m7_vd6869_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	
	if (system_rev <= 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
			goto enable_io_fail;
		}
		gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 1);
		gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	} else { 
		if (check_yushanII_flag() == 0) {
			pr_info("%s: 8921_lvs1 1800000\n", __func__);
			rc = camera_sensor_power_enable("8921_lvs1", 1800000, &reg_8921_lvs1);
			pr_info("%s: 8921_lvs1 1800000 (%d)\n", __func__, rc);
			if (rc < 0) {
				pr_err("sensor_power_enable\
					(\"8921_lvs1\", 1.8V) FAILED %d\n", rc);
				goto enable_io_fail;
			}
		}
	}
	mdelay(5);

	if (system_rev == 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_1V8_EN, "V_CAM_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
			goto enable_io_fail_2;
		}
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_1V8_EN, 1);
		gpio_free(CAM_PIN_GPIO_V_CAM_1V8_EN);
		mdelay(5);
	} else if (system_rev >= 2) { 
		pr_info("%s: 8921_lvs4 1800000\n", __func__);
		rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
		pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("sensor_power_enable\
				(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
			goto enable_io_fail_2;
		}
		mdelay(5);
	}

	
	pr_info("%s: 8921_l8 2900000\n", __func__);
	rc = camera_sensor_power_enable("8921_l8", 2900000, &reg_8921_l8);
	pr_info("%s: 8921_l8 2900000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l8\", 2.9V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(5);

	
	pr_info("%s: 8921_l9 3100000\n", __func__);
	rc = camera_sensor_power_enable("8921_l9", 2900000, &reg_8921_l9);
	pr_info("%s: 8921_l9 3100000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l9\", 3.1V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);

	

	
	if (system_rev >= 1) { 
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD	
		pr_info("%s: CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB\n", __func__);
		rc = gpio_request(CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB, "CAM_D1V2_EN");
		pr_info("%s: CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB (%d)\n", __func__, rc);
		if (rc) {
			pr_err("sensor_power_enable\
				(\"gpio %d\", 1.2V) FAILED %d\n",
				CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB, rc);
			goto enable_digital_fail;
		}
		gpio_direction_output(CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB, 1);
		gpio_free(CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB);
#else
		pr_info("[CAM]%s: CAM_PIN_GPIO_MCAM_D1V2_EN\n", __func__);
		rc = gpio_request(CAM_PIN_GPIO_MCAM_D1V2_EN, "CAM_D1V2_EN");
		pr_info("[CAM]%s: CAM_PIN_GPIO_MCAM_D1V2_EN (%d)\n", __func__, rc);
		if (rc) {
			pr_err("[CAM]sensor_power_enable\
				(\"gpio %d\", 1.05V) FAILED %d\n",
				CAM_PIN_GPIO_MCAM_D1V2_EN, rc);
			goto enable_digital_fail;
		}
		gpio_direction_output(CAM_PIN_GPIO_MCAM_D1V2_EN, 1);
		gpio_free(CAM_PIN_GPIO_MCAM_D1V2_EN);
#endif
	} else { 
		pr_info("%s: 8921_l12 1200000\n", __func__);
		rc = camera_sensor_power_enable("8921_l12", 1200000, &reg_8921_l12);
		pr_info("%s: 8921_l12 1200000 (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("sensor_power_enable\
				(\"8921_l12\", 1.2V) FAILED %d\n", rc);
			goto enable_digital_fail;
		}
	}
	mdelay(2);
	
		rc = gpio_request(CAM_PIN_GPIO_CAM_SEL, "CAM_SEL");
		pr_info("%s: CAM_PIN_GPIO_CAM_SEL (%d)\n", __func__, rc);
		if (rc) {
			pr_err("sensor_power_enable(\"gpio %d\") FAILED %d\n",CAM_PIN_GPIO_CAM_SEL, rc);
			goto enable_mclk_switch_fail;
		}
		gpio_direction_output(CAM_PIN_GPIO_CAM_SEL, 0);
		gpio_free(CAM_PIN_GPIO_CAM_SEL);
		mdelay(1);

	return rc;

enable_digital_fail:	
		camera_sensor_power_disable(reg_8921_l9);
enable_analog_fail:
	if (system_rev == 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_1V8_EN, "V_CAM_1V8_EN");
		if (rc < 0) {
			pr_err("I/O 1v8 off\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		} else {
			gpio_direction_output(CAM_PIN_GPIO_V_CAM_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_CAM_1V8_EN);
		}
	} else if (system_rev >= 2) { 
		rc = camera_sensor_power_disable(reg_8921_lvs4);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_lvs4\") FAILED %d\n", rc);
	}
enable_io_fail_2:
	if (system_rev <= 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
		if (rc < 0) {
			pr_err("I/O 1v8 off\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		} else {
			gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
		}
	} else { 
		if (check_yushanII_flag() == 0) {
			rc = camera_sensor_power_disable(reg_8921_lvs1);
			if (rc < 0)
				pr_err("sensor_power_disable\
					(\"8921_lvs1\") FAILED %d\n", rc);
		}
	}
enable_io_fail:
	
enable_vcm_fail:
		if(system_rev > 0){
					camera_sensor_power_disable(reg_8921_l8);
		}

enable_mclk_switch_fail:
		
		if (system_rev >= 1) { 
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD	
			rc = gpio_request(CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB, "CAM_D1V2_EN");
			if (rc < 0)
				pr_err("sensor_power_disable\
					(\"gpio %d\", 1.05V) FAILED %d\n",
					CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB, rc);
			else {
				gpio_direction_output(CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB, 0);
				gpio_free(CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB);
			}
#else
			rc = gpio_request(CAM_PIN_GPIO_MCAM_D1V2_EN, "MCAM_D1V2_EN");
			pr_info("I/O 1v2 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_MCAM_D1V2_EN, rc);
			if (rc < 0)
				pr_err("sensor_power_disable\
					(\"gpio %d\", 1.05V) FAILED %d\n",
					CAM_PIN_GPIO_MCAM_D1V2_EN, rc);
			else {
				gpio_direction_output(CAM_PIN_GPIO_MCAM_D1V2_EN, 0);
				gpio_free(CAM_PIN_GPIO_MCAM_D1V2_EN);
			}
#endif

		} else { 
			pr_info("%s: 8921_l12 off\n", __func__);
			rc = camera_sensor_power_disable(reg_8921_l12);
			if (rc < 0)
				pr_err("sensor_power_disable\
					(\"8921_l12\") FAILED %d\n", rc);
		}
		mdelay(10);

	return rc;
}

static int m7_vd6869_vreg_off(void)
{
	int rc = 0;
	pr_info("%s\n", __func__);

	
		rc = gpio_request(CAM_PIN_GPIO_CAM_SEL, "CAM_SEL");
		pr_info("%s: CAM_PIN_GPIO_CAM_SEL (%d)\n", __func__, rc);
		if (rc>=0) {
			gpio_direction_output(CAM_PIN_GPIO_CAM_SEL, 0);
			gpio_free(CAM_PIN_GPIO_CAM_SEL);
		}

	
	if (system_rev >= 1) { 
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD	
		rc = gpio_request(CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB, "CAM_D1V2_EN");
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"gpio %d\", 1.05V) FAILED %d\n",
				CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB, rc);
		else {
			gpio_direction_output(CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB, 0);
			gpio_free(CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB);
		}
#else
		rc = gpio_request(CAM_PIN_GPIO_MCAM_D1V2_EN, "MCAM_D1V2_EN");
		pr_info("I/O 1v2 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_MCAM_D1V2_EN, rc);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"gpio %d\", 1.05V) FAILED %d\n",
				CAM_PIN_GPIO_MCAM_D1V2_EN, rc);
		else {
			gpio_direction_output(CAM_PIN_GPIO_MCAM_D1V2_EN, 0);
			gpio_free(CAM_PIN_GPIO_MCAM_D1V2_EN);
		}
#endif

	} else { 
		pr_info("%s: 8921_l12 off\n", __func__);
		rc = camera_sensor_power_disable(reg_8921_l12);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_l12\") FAILED %d\n", rc);
	}
	mdelay(10);

	
	pr_info("%s: 8921_l9 off\n", __func__);
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	
	pr_info("%s: 8921_l8 off\n", __func__);
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(10);


	
	if (system_rev == 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_1V8_EN, "V_CAM_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		}
		else
		{
			gpio_direction_output(CAM_PIN_GPIO_V_CAM_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_CAM_1V8_EN);
		}
		mdelay(20);
	} else if (system_rev >= 2) { 
		rc = camera_sensor_power_disable(reg_8921_lvs4);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_lvs4\") FAILED %d\n", rc);
		mdelay(20);
	}

	if (system_rev <= 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		}
		else
		{
			gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
		}
	} else { 
		if (check_yushanII_flag() == 0) {
			rc = camera_sensor_power_disable(reg_8921_lvs1);
			if (rc < 0)
				pr_err("sensor_power_disable\
					(\"8921_lvs1\") FAILED %d\n", rc);
		}
	}
	mdelay(10);

	return rc;
}

static struct msm_camera_csi_lane_params vd6869_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_vd6869_board_info = {
	.mount_angle = 90,
	.pixel_order_default = MSM_CAMERA_PIXEL_ORDER_GR,	
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD
	.mirror_flip = CAMERA_SENSOR_MIRROR_FLIP,
#else
	.mirror_flip = CAMERA_SENSOR_MIRROR_FLIP,
#endif
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= CAM_PIN_GPIO_CAM_PWDN,
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &vd6869_csi_lane_params,
	.sensor_mount_angle = ANGLE_90, 
	.ews_enable = false,
};

static struct camera_led_est msm_camera_sensor_vd6869_led_table[] = {
                {
                .enable = 1,
                .led_state = FL_MODE_FLASH,
                .current_ma = 1500,
                .lumen_value = 1500,
                .min_step = 20,
                .max_step = 28
        },
                {
                .enable = 1,
                .led_state = FL_MODE_FLASH_LEVEL3,
                .current_ma = 300,
                .lumen_value = 300,
                .min_step = 0,
                .max_step = 19
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
                .enable =0,
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
        },};


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
	.low_temp_limit		= 5,
	.low_cap_limit		= 14,
	.flash_info             = &msm_camera_sensor_vd6869_flash_info,
};


static struct msm_camera_sensor_flash_data flash_vd6869 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_camera_flash_src,
#endif

};

#if defined(CONFIG_RUMBAS_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_LC898212_ACT)
static struct msm_actuator_info *vd6869_actuator_table[] = {
#if defined(CONFIG_RUMBAS_ACT)
    &rumbas_actuator_info,
#endif
#if defined(CONFIG_TI201_ACT)
    &ti201_actuator_info,
#endif
#if defined(CONFIG_LC898212_ACT)
    &lc898212_actuator_info,
#endif
};
#endif

static struct msm_camera_sensor_info msm_camera_sensor_vd6869_data = {
	.sensor_name	= "vd6869",
	.camera_power_on = m7_vd6869_vreg_on,
	.camera_power_off = m7_vd6869_vreg_off,
	.camera_yushanii_probed = m7_yushanii_probed,
	.pdata	= &m7_msm_camera_csi_device_data[0],
	.flash_data	= &flash_vd6869,
	.sensor_platform_info = &sensor_vd6869_board_info,
	.gpio_conf = &vd6869_back_cam_gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,

#if defined(CONFIG_RUMBAS_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_LC898212_ACT)
	.num_actuator_info_table = ARRAY_SIZE(vd6869_actuator_table),
	.actuator_info_table = &vd6869_actuator_table[0],
#endif

#if defined(CONFIG_RUMBAS_ACT)
	.actuator_info = &rumbas_actuator_info,
#endif
	.use_rawchip = RAWCHIP_DISABLE,
	.htc_image = HTC_CAMERA_IMAGE_YUSHANII_BOARD,
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = HDR_MODE,
	.flash_cfg = &msm_camera_sensor_vd6869_flash_cfg, 
};

#endif	

#ifdef CONFIG_OV4688

static int m7_ov4688_vreg_on(void)
{
	int rc =0;
	pr_info("%s\n", __func__);


	
	pr_info("%s: 8921_lvs4 1800000\n", __func__);
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_ov4688_io1v8_fail;
	}
	mdelay(5);

	
	rc = camera_sensor_power_enable("8921_l8", 2900000, &reg_8921_l8);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8038_l8\", 2.9V) FAILED %d\n", rc);
		goto enable_ov4688_l8_fail;
	}
	mdelay(5);

	
	rc = gpio_set (CAM_PIN_GPIO_MCAM_D1V2_EN,1);
	if (rc < 0)
		goto enable_ov4688_d1v2_fail;
	mdelay(2);

	
	rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_ov4688_vcm_fail;
	}
	mdelay(1);

	
	rc = gpio_set (CAM_PIN_GPIO_CAM_VCM_PD,1);
	if (rc < 0) {
		goto enable_ov4688_vcm_pd_fail;
	}

	return rc;

enable_ov4688_vcm_pd_fail:
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

enable_ov4688_vcm_fail:
	rc= gpio_set (CAM_PIN_GPIO_MCAM_D1V2_EN,0);
	if (rc < 0)
		pr_err("Set D1V2 fail\n");

enable_ov4688_d1v2_fail:
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);

enable_ov4688_l8_fail:
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_lvs4\") FAILED %d\n", rc);

enable_ov4688_io1v8_fail:

	return rc;
}

static int m7_ov4688_vreg_off(void)
{
	int rc = 0;
	pr_info("%s\n", __func__);

	
	rc = gpio_set (CAM_PIN_GPIO_CAM_VCM_PD,0);
	if (rc < 0)
		pr_err("Set VCM PD fail\n");
	mdelay(10);

	
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	
	rc = gpio_set (CAM_PIN_GPIO_MCAM_D1V2_EN,0);
	if (rc < 0)
		pr_err("sensor_power_disable\
				(\"CAM_PIN_GPIO_MCAM_D1V2_EN\") FAILED %d\n", rc);
	mdelay(10);

	
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(10);

	
	rc = camera_sensor_power_disable(reg_8921_lvs4);
	if (rc < 0)
		pr_err("sensor_power_disable\
				(\"8921_lvs4\") FAILED %d\n", rc);
	mdelay(20);

	return rc;
}

static struct msm_camera_csi_lane_params ov4688_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_ov4688_board_info = {
	.mount_angle = 90,
	.pixel_order_default = MSM_CAMERA_PIXEL_ORDER_GR,	
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD
	.mirror_flip = CAMERA_SENSOR_MIRROR_FLIP,
#else
	.mirror_flip = CAMERA_SENSOR_FLIP, 
#endif
	.sensor_reset_enable = 0,
	.sensor_reset = 0,
	.sensor_pwd	= CAM_PIN_GPIO_CAM_PWDN,
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &ov4688_csi_lane_params,
};

static struct camera_led_est msm_camera_sensor_ov4688_led_table[] = {
                {
                .enable = 1,
                .led_state = FL_MODE_FLASH,
                .current_ma = 1500,
                .lumen_value = 1500,
                .min_step = 20,
                .max_step = 28
        },
                {
                .enable = 1,
                .led_state = FL_MODE_FLASH_LEVEL3,
                .current_ma = 300,
                .lumen_value = 300,
                .min_step = 0,
                .max_step = 19
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
                .enable =0,
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
        },};


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
	.low_temp_limit		= 5,
	.low_cap_limit		= 15,
	.flash_info             = &msm_camera_sensor_ov4688_flash_info,
};

static struct msm_camera_sensor_flash_data flash_ov4688 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_camera_flash_src,
#endif

};


#if defined(CONFIG_RUMBAS_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_LC898212_ACT)
static struct msm_actuator_info *ov4688_actuator_table[] = {
#if defined(CONFIG_RUMBAS_ACT)
    &rumbas_actuator_info,
#endif
#if defined(CONFIG_TI201_ACT)
    &ti201_actuator_info,
#endif
#if defined(CONFIG_LC898212_ACT)
    &lc898212_actuator_info,
#endif
};
#endif

static struct msm_camera_sensor_info msm_camera_sensor_ov4688_data = {
	.sensor_name	= "ov4688",
	.camera_power_on = m7_ov4688_vreg_on,
	.camera_power_off = m7_ov4688_vreg_off,
	.camera_yushanii_probed = m7_yushanii_probed,
	.pdata	= &m7_msm_camera_csi_device_data[0],
	.flash_data	= &flash_ov4688,
	.sensor_platform_info = &sensor_ov4688_board_info,
	.gpio_conf = &ov4688_back_cam_gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,

#if defined(CONFIG_RUMBAS_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_LC898212_ACT)
	.num_actuator_info_table = ARRAY_SIZE(ov4688_actuator_table),
	.actuator_info_table = &ov4688_actuator_table[0],
#endif

#if defined(CONFIG_RUMBAS_ACT)
	.actuator_info = &rumbas_actuator_info,
#endif
#ifdef CONFIG_LC898212_ACT
	.actuator_info = &lc898212_actuator_info,
#endif
	.use_rawchip = RAWCHIP_DISABLE,
	.htc_image = HTC_CAMERA_IMAGE_YUSHANII_BOARD,
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = HDR_MODE,
	.flash_cfg = &msm_camera_sensor_ov4688_flash_cfg, 
};

#endif

#ifdef CONFIG_IMX091
static int m7_imx091_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	
	pr_info("%s: 8921_l9 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9);
	pr_info("%s: 8921_l9 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);

	
	pr_info("%s: 8921_l8 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	pr_info("%s: 8921_l8 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(1);

#if 0	
	if (system_rev == 0 || !m7_use_ext_1v2()) { 
#else
	if (1) {
#endif	
	
	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN\n", __func__);
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
	mdelay(1);
	}

	
#if 0	
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
#else
	pr_info("%s: 8921_lvs4 1800000\n", __func__);
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);	
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
#endif	
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}

	return rc;

enable_io_fail:
#if 0	
	if (system_rev == 0 || !m7_use_ext_1v2()) { 
#else
	{
#endif	
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
	}
	}
enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_l9);
enable_vcm_fail:
	return rc;
}

static int m7_imx091_vreg_off(void)
{
	int rc = 0;

	pr_info("%s\n", __func__);

	
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(1);

#if 0	
	if (system_rev == 0 || !m7_use_ext_1v2()) { 
#else
	if (1) {
#endif	
	
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
	}
	mdelay(1);
	}

	
#if 0	
	rc = camera_sensor_power_disable(reg_8921_lvs6);
#else
	rc = camera_sensor_power_disable(reg_8921_lvs4);
#endif	
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_lvs6\") FAILED %d\n", rc);

	mdelay(1);

	
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	return rc;
}

#ifdef CONFIG_IMX091_ACT
static struct i2c_board_info imx091_actuator_i2c_info = {
	I2C_BOARD_INFO("imx091_act", 0x11),
};

static struct msm_actuator_info imx091_actuator_info = {
	.board_info     = &imx091_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif

static struct msm_camera_csi_lane_params imx091_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_imx091_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= CAM_PIN_GPIO_CAM_PWDN,
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &imx091_csi_lane_params,
};

static struct camera_led_est msm_camera_sensor_imx091_led_table[] = {
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,
		.min_step = 29,
		.max_step = 128
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 350,
		.min_step = 27,
		.max_step = 28
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 400,
		.lumen_value = 440,
		.min_step = 25,
		.max_step = 26
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 600,
		.lumen_value = 625,
		.min_step = 23,
		.max_step = 24
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,
		.min_step = 0,
		.max_step = 22    
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

static struct camera_led_info msm_camera_sensor_imx091_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 750,  
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_imx091_led_table),
};

static struct camera_flash_info msm_camera_sensor_imx091_flash_info = {
	.led_info = &msm_camera_sensor_imx091_led_info,
	.led_est_table = msm_camera_sensor_imx091_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_imx091_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 14,
	.low_cap_limit_dual = 0,
	.flash_info             = &msm_camera_sensor_imx091_flash_info,
};


static struct msm_camera_sensor_flash_data flash_imx091 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_camera_flash_src,
#endif

};

#ifdef CONFIG_IMX091
#if defined(CONFIG_AD5823_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_AD5816_ACT)
static struct msm_actuator_info *imx091_actuator_table[] = {
#if defined(CONFIG_AD5823_ACT)
    &ad5823_actuator_info,
#endif
#if defined(CONFIG_TI201_ACT)
    &ti201_actuator_info,
#endif
#if defined(CONFIG_AD5816_ACT)
    &ad5816_actuator_info,
#endif
};
#endif
#endif

static struct msm_camera_sensor_info msm_camera_sensor_imx091_data = {
	.sensor_name	= "imx091",
	.camera_power_on = m7_imx091_vreg_on,
	.camera_power_off = m7_imx091_vreg_off,
	.pdata	= &m7_msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx091,
	.sensor_platform_info = &sensor_imx091_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
#if defined(CONFIG_AD5823_ACT) || defined(CONFIG_TI201_ACT) || defined(CONFIG_AD5816_ACT)
	.num_actuator_info_table = ARRAY_SIZE(imx091_actuator_table),
	.actuator_info_table = &imx091_actuator_table[0],
#endif
#if defined(CONFIG_AD5823_ACT)
	.actuator_info = &ti201_actuator_info,
#endif
	.use_rawchip = RAWCHIP_ENABLE,
	.video_hdr_capability = NON_HDR_MODE,
	.flash_cfg = &msm_camera_sensor_imx091_flash_cfg, 
};

#endif	


#ifdef CONFIG_S5K3H2YX
static int m7_s5k3h2yx_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	
	pr_info("%s: 8921_l9 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9);
	pr_info("%s: 8921_l9 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);

	
	pr_info("%s: 8921_l8 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	pr_info("%s: 8921_l8 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(1);

#if 0	
	if (system_rev == 0 || !m7_use_ext_1v2()) { 
#else
	if (1) {
#endif	
	
	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN\n", __func__);
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	pr_info("%s: CAM_PIN_GPIO_V_CAM_D1V2_EN (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
		goto enable_digital_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 1);
	gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
	mdelay(1);
	}

	
#if 0	
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
#else
	pr_info("%s: 8921_lvs4 1800000\n", __func__);
	rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);	
	pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
#endif	
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}

	return rc;

enable_io_fail:
#if 0	
	if (system_rev == 0 || !m7_use_ext_1v2()) { 
#else
	{
#endif	
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
	}
	}
enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_l9);
enable_vcm_fail:
	return rc;
}

static int m7_s5k3h2yx_vreg_off(void)
{
	int rc = 0;

	pr_info("%s\n", __func__);

	
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(1);

#if 0	
	if (system_rev == 0 || !m7_use_ext_1v2()) { 
#else
	if (1) {
#endif	
	
	rc = gpio_request(CAM_PIN_GPIO_V_CAM_D1V2_EN, "CAM_D1V2_EN");
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"gpio %d\", 1.2V) FAILED %d\n",
			CAM_PIN_GPIO_V_CAM_D1V2_EN, rc);
	else {
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_D1V2_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_CAM_D1V2_EN);
	}
	mdelay(1);
	}

	
#if 0	
	rc = camera_sensor_power_disable(reg_8921_lvs6);
#else
	rc = camera_sensor_power_disable(reg_8921_lvs4);
#endif	
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_lvs6\") FAILED %d\n", rc);

	mdelay(1);

	
	rc = camera_sensor_power_disable(reg_8921_l9);
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
	.vcm_pwd        = CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif

static struct msm_camera_csi_lane_params s5k3h2yx_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_s5k3h2yx_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= CAM_PIN_GPIO_CAM_PWDN,
	.vcm_pwd	= CAM_PIN_GPIO_CAM_VCM_PD,
	.vcm_enable	= 1,
	.csi_lane_params = &s5k3h2yx_csi_lane_params,
};

static struct camera_led_est msm_camera_sensor_s5k3h2yx_led_table[] = {
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,
		.min_step = 29,
		.max_step = 128
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 350,
		.min_step = 27,
		.max_step = 28
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 400,
		.lumen_value = 440,
		.min_step = 25,
		.max_step = 26
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 600,
		.lumen_value = 625,
		.min_step = 23,
		.max_step = 24
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 745,
		.min_step = 0,
		.max_step = 22    
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
	.low_cap_limit_dual = 0,
	.flash_info             = &msm_camera_sensor_s5k3h2yx_flash_info,
};


static struct msm_camera_sensor_flash_data flash_s5k3h2yx = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_camera_flash_src,
#endif

};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3h2yx_data = {
	.sensor_name	= "s5k3h2yx",
	.camera_power_on = m7_s5k3h2yx_vreg_on,
	.camera_power_off = m7_s5k3h2yx_vreg_off,
	.pdata	= &m7_msm_camera_csi_device_data[0],
	.flash_data	= &flash_s5k3h2yx,
	.sensor_platform_info = &sensor_s5k3h2yx_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
#ifdef CONFIG_S5K3H2YX_ACT
	.actuator_info = &s5k3h2yx_actuator_info,
#endif
	.use_rawchip = RAWCHIP_ENABLE,
	.video_hdr_capability = NON_HDR_MODE,
	.flash_cfg = &msm_camera_sensor_s5k3h2yx_flash_cfg, 
};

#endif	

#ifdef CONFIG_S5K6A1GX
static int m7_s5k6a1gx_vreg_on(void)
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

	
#if 0
	if (system_rev >= 1) { 
		rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
		if (rc < 0) {
			pr_err("sensor_power_enable\
				(\"8921_lvs6\", 1.8V) FAILED %d\n", rc);
			goto enable_io_fail;
		}
	} else { 
#endif
		rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
		pr_info("sensor_power_enable(\"8921_lvs4\", 1.8V) == %d\n", rc);
		if (rc < 0) {
			pr_err("sensor_power_enable(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
			goto enable_io_fail;
		}
	udelay(50);

#if 0	
	if (system_rev == 0) { 
		
		rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
		if (rc < 0) {
			pr_err("sensor_power_enable\
				(\"8921_lvs6\", 1.8V) FAILED %d\n", rc);
			goto enable_i2c_pullup_fail;
		}
	}
#endif

	
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "s5k6a1gx");
	pr_info("reset pin gpio_request,%d\n", CAM_PIN_GPIO_CAM2_RSTz);
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
		goto enable_rst_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 1);
	gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	udelay(50);

	
#if 0
	if (system_rev >= 1) { 
		rc = gpio_request(CAM_PIN_XB_GPIO_V_CAM2_D1V2_EN, "s5k6a1gx");
		pr_info("reset pin gpio_request,%d\n", CAM_PIN_XB_GPIO_V_CAM2_D1V2_EN);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_XB_GPIO_V_CAM2_D1V2_EN);
			goto enable_digital_fail;
		}
		gpio_direction_output(CAM_PIN_XB_GPIO_V_CAM2_D1V2_EN, 1);
		gpio_free(CAM_PIN_XB_GPIO_V_CAM2_D1V2_EN);
	} else { 
#endif
		rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V2_EN, "s5k6a1gx");
		pr_info("digital gpio_request,%d\n", CAM_PIN_GPIO_V_CAM2_D1V2_EN);
		if (rc < 0) {
			pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V2_EN);
			goto enable_digital_fail;
		}
		gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V2_EN, 1);
		gpio_free(CAM_PIN_GPIO_V_CAM2_D1V2_EN);
	udelay(50);

	return rc;

enable_digital_fail:
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "s5k6a1gx");
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	else {
		gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
		gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	}
enable_rst_fail:
#if 0
	camera_sensor_power_disable(reg_8921_lvs6);
enable_i2c_pullup_fail:
	if (system_rev < 1) 
#endif
	camera_sensor_power_disable(reg_8921_lvs4);
enable_io_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	return rc;
}

static int m7_s5k6a1gx_vreg_off(void)
{
	int rc;
	pr_info("%s\n", __func__);

	
#if 0
	if (system_rev >= 1) { 
		rc = gpio_request(CAM_PIN_XB_GPIO_V_CAM2_D1V2_EN, "s5k6a1gx");
		if (rc < 0)
			pr_err("GPIO(%d) request failed", CAM_PIN_XB_GPIO_V_CAM2_D1V2_EN);
		else {
			gpio_direction_output(CAM_PIN_XB_GPIO_V_CAM2_D1V2_EN, 0);
			gpio_free(CAM_PIN_XB_GPIO_V_CAM2_D1V2_EN);
		}
	} else { 

		rc = gpio_request(CAM_PIN_GPIO_V_CAM2_D1V2_EN, "s5k6a1gx");
		pr_info("digital gpio_request,%d\n", CAM_PIN_GPIO_V_CAM2_D1V2_EN);
		if (rc < 0)
			pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_V_CAM2_D1V2_EN);
		else {
			gpio_direction_output(CAM_PIN_GPIO_V_CAM2_D1V2_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_CAM2_D1V2_EN);
		}
	}
#endif
	udelay(50);

	
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "s5k6a1gx");
	pr_info("reset pin gpio_request,%d\n", CAM_PIN_GPIO_CAM2_RSTz);
	if (rc < 0)
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
	else {
		gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
		gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	}
	udelay(50);

#if 0
	if (system_rev == 0) { 
		
		rc = camera_sensor_power_disable(reg_8921_lvs6);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_lvs6\") FAILED %d\n", rc);
		mdelay(1);
	}

	
	if (system_rev >= 1) { 
		rc = camera_sensor_power_disable(reg_8921_lvs6);
		if (rc < 0)
			pr_err("sensor_power_disable(\"8921_lvs6\") FAILED %d\n", rc);
	} else { 
#endif
		rc = camera_sensor_power_disable(reg_8921_lvs4);
		if (rc < 0)
			pr_err("sensor_power_disable(\"8921_lvs4\") FAILED %d\n", rc);
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
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.csi_lane_params = &s5k6a1gx_csi_lane_params,
};

static struct msm_camera_sensor_flash_data flash_s5k6a1gx = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k6a1gx_data = {
	.sensor_name	= "s5k6a1gx",
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.camera_power_on = m7_s5k6a1gx_vreg_on,
	.camera_power_off = m7_s5k6a1gx_vreg_off,
	.pdata	= &m7_msm_camera_csi_device_data[1],
	.flash_data	= &flash_s5k6a1gx,
	.sensor_platform_info = &sensor_s5k6a1gx_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_DISABLE,
	.video_hdr_capability = NON_HDR_MODE,
};

#endif	

#ifdef CONFIG_AR0260
static int m7_ar0260_vreg_on(void)
{
	int rc;

	pr_info("%s\n", __func__);

	
	if (system_rev <= 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
			goto enable_io_fail;
		}
		gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 1);
		gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	} else { 
		if (check_yushanII_flag() == 0) {
			pr_info("%s: 8921_lvs1 1800000\n", __func__);
			rc = camera_sensor_power_enable("8921_lvs1", 1800000, &reg_8921_lvs1);
			pr_info("%s: 8921_lvs1 1800000 (%d)\n", __func__, rc);
			if (rc < 0) {
				pr_err("sensor_power_enable\
					(\"8921_lvs1\", 1.8V) FAILED %d\n", rc);
				goto enable_io_fail;
			}
		}
	}
	mdelay(1);

	if (system_rev == 1) { 
		mdelay(50);
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_1V8_EN, "V_CAM_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
			goto enable_io_fail_2;
		}
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_1V8_EN, 1);
		gpio_free(CAM_PIN_GPIO_V_CAM_1V8_EN);
		mdelay(5);
	} else if (system_rev >= 2) { 
		mdelay(50);
		pr_info("%s: 8921_lvs4 1800000\n", __func__);
		rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
		pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("sensor_power_enable\
				(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
			goto enable_io_fail_2;
		}
		mdelay(5);
	}

	
	pr_info("%s: 8921_l8 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	pr_info("%s: 8921_l8 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(5);

	
	rc = gpio_request(CAM_PIN_GPIO_CAM_SEL, "CAM_SEL");
	pr_info("%s: CAM_PIN_GPIO_CAM_SEL (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable(\"gpio %d\") FAILED %d\n",CAM_PIN_GPIO_CAM_SEL, rc);
		goto enable_mclk_switch_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM_SEL, 1);
	gpio_free(CAM_PIN_GPIO_CAM_SEL);
	mdelay(1);

	return rc;


enable_analog_fail:
	if (system_rev == 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_1V8_EN, "V_CAM_1V8_EN");
		if (rc < 0) {
			pr_err("I/O 1v8 off\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		} else {
			gpio_direction_output(CAM_PIN_GPIO_V_CAM_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_CAM_1V8_EN);
		}
	} else if (system_rev >= 2) { 
		rc = camera_sensor_power_disable(reg_8921_lvs4);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_lvs4\") FAILED %d\n", rc);
	}
enable_io_fail_2:
	if (system_rev <= 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
		if (rc < 0) {
			pr_err("I/O 1v8 off\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		} else {
			gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
		}
	} else { 
		if (check_yushanII_flag() == 0) {
			rc = camera_sensor_power_disable(reg_8921_lvs1);
			if (rc < 0)
				pr_err("sensor_power_disable\
					(\"8921_lvs1\") FAILED %d\n", rc);
		}
	}
enable_io_fail:
enable_mclk_switch_fail:

	return rc;
}

static int m7_ar0260_vreg_off(void)
{
	int rc = 0;
	pr_info("%s\n", __func__);

	
	mdelay(3);
	rc = gpio_request(CAM_PIN_GPIO_CAM_SEL, "CAM_SEL");
	pr_info("%s: CAM_PIN_GPIO_CAM_SEL (%d)\n", __func__, rc);
	if (rc>=0) {
		gpio_direction_output(CAM_PIN_GPIO_CAM_SEL, 0);
		gpio_free(CAM_PIN_GPIO_CAM_SEL);
	}
	mdelay(3);

	
	pr_info("%s: 8921_l8 off\n", __func__);
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(10);

	
	if (system_rev == 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_1V8_EN, "V_CAM_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		}
		else
		{
			gpio_direction_output(CAM_PIN_GPIO_V_CAM_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_CAM_1V8_EN);
		}
		mdelay(20);
	} else if (system_rev >= 2) { 
		rc = camera_sensor_power_disable(reg_8921_lvs4);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_lvs4\") FAILED %d\n", rc);
		mdelay(20);
	}

	if (system_rev <= 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		}
		else
		{
			gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
		}
	} else { 
		if (check_yushanII_flag() == 0) {
			rc = camera_sensor_power_disable(reg_8921_lvs1);
			if (rc < 0)
				pr_err("sensor_power_disable\
					(\"8921_lvs1\") FAILED %d\n", rc);
		}
	}
	mdelay(1);

	return rc;
}

static struct msm_camera_csi_lane_params ar0260_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_ar0260_board_info = {
	.mount_angle = 270,
	.pixel_order_default = MSM_CAMERA_PIXEL_ORDER_GR,	
	.mirror_flip = CAMERA_SENSOR_FLIP,
	.sensor_reset_enable = 0,
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.csi_lane_params = &ar0260_csi_lane_params,
};

static struct msm_camera_sensor_flash_data flash_ar0260 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_ar0260_data = {
	.sensor_name	= "ar0260",
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.camera_power_on = m7_ar0260_vreg_on,
	.camera_power_off = m7_ar0260_vreg_off,
	.camera_yushanii_probed = m7_yushanii_probed,
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD
	.pdata	= &m7_msm_camera_csi_device_data[1],
#else
	.pdata	= &m7_msm_camera_csi_device_data[0],
#endif
	.flash_data	= &flash_ar0260,
	.sensor_platform_info = &sensor_ar0260_board_info,
	.gpio_conf = &ar0260_front_cam_gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_DISABLE,
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD
	.htc_image = HTC_CAMERA_IMAGE_NONE_BOARD,
#else
	.htc_image = HTC_CAMERA_IMAGE_YUSHANII_BOARD,
#endif
	.hdr_mode = NON_HDR_MODE,
	.video_hdr_capability = NON_HDR_MODE,
};

#endif	


#ifdef CONFIG_OV2722
static int m7_ov2722_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "ov2722");
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
		goto reset_high_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 1);
	gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	mdelay(2);

	
	if (system_rev <= 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
			goto enable_io_fail;
		}
		gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 1);
		gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	} else { 
		if (check_yushanII_flag() == 0) {
			pr_info("%s: 8921_lvs1 1800000\n", __func__);
			rc = camera_sensor_power_enable("8921_lvs1", 1800000, &reg_8921_lvs1);
			pr_info("%s: 8921_lvs1 1800000 (%d)\n", __func__, rc);
			if (rc < 0) {
				pr_err("sensor_power_enable\
					(\"8921_lvs1\", 1.8V) FAILED %d\n", rc);
				goto enable_io_fail;
			}
		}
	}
	mdelay(1);

	if (system_rev == 1) { 
		mdelay(50);
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_1V8_EN, "V_CAM_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
			goto enable_io_fail_2;
		}
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_1V8_EN, 1);
		gpio_free(CAM_PIN_GPIO_V_CAM_1V8_EN);
		mdelay(5);
	} else if (system_rev >= 2) { 
		mdelay(50);
		pr_info("%s: 8921_lvs4 1800000\n", __func__);
		rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
		pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("sensor_power_enable\
				(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
			goto enable_io_fail_2;
		}
		mdelay(5);
	}

	
	pr_info("%s: 8921_l8 2800000\n", __func__);
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	pr_info("%s: 8921_l8 2800000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(5);

	
	rc = gpio_request(CAM_PIN_GPIO_CAM_SEL, "CAM_SEL");
	pr_info("%s: CAM_PIN_GPIO_CAM_SEL (%d)\n", __func__, rc);
	if (rc) {
		pr_err("sensor_power_enable(\"gpio %d\") FAILED %d\n",CAM_PIN_GPIO_CAM_SEL, rc);
		goto enable_mclk_switch_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM_SEL, 1);
	gpio_free(CAM_PIN_GPIO_CAM_SEL);
	mdelay(1);

	
	rc = gpio_request(CAM_PIN_GPIO_CAM2_RSTz, "ov2722");
	if (rc < 0) {
		pr_err("GPIO(%d) request failed", CAM_PIN_GPIO_CAM2_RSTz);
		goto reset_low_fail;
	}
	gpio_direction_output(CAM_PIN_GPIO_CAM2_RSTz, 0);
	gpio_free(CAM_PIN_GPIO_CAM2_RSTz);
	mdelay(5);

	return rc;


reset_low_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	if (system_rev == 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_1V8_EN, "V_CAM_1V8_EN");
		if (rc < 0) {
			pr_err("I/O 1v8 off\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		} else {
			gpio_direction_output(CAM_PIN_GPIO_V_CAM_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_CAM_1V8_EN);
		}
	} else if (system_rev >= 2) { 
		rc = camera_sensor_power_disable(reg_8921_lvs4);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_lvs4\") FAILED %d\n", rc);
	}
enable_io_fail_2:
	if (system_rev <= 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
		if (rc < 0) {
			pr_err("I/O 1v8 off\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		} else {
			gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
		}
	} else { 
		if (check_yushanII_flag() == 0) {
			rc = camera_sensor_power_disable(reg_8921_lvs1);
			if (rc < 0)
				pr_err("sensor_power_disable\
					(\"8921_lvs1\") FAILED %d\n", rc);
		}
	}
enable_io_fail:
reset_high_fail:
enable_mclk_switch_fail:

	return rc;
}

static int m7_ov2722_vreg_off(void)
{
	int rc = 0;
	pr_info("%s\n", __func__);

	
	mdelay(3);
	rc = gpio_request(CAM_PIN_GPIO_CAM_SEL, "CAM_SEL");
	pr_info("%s: CAM_PIN_GPIO_CAM_SEL (%d)\n", __func__, rc);
	if (rc>=0) {
		gpio_direction_output(CAM_PIN_GPIO_CAM_SEL, 0);
		gpio_free(CAM_PIN_GPIO_CAM_SEL);
	}
	mdelay(3);

	
	pr_info("%s: 8921_l8 off\n", __func__);
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(10);

	
	if (system_rev == 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_1V8_EN, "V_CAM_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		}
		else
		{
			gpio_direction_output(CAM_PIN_GPIO_V_CAM_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_CAM_1V8_EN);
		}
		mdelay(20);
	} else if (system_rev >= 2) { 
		rc = camera_sensor_power_disable(reg_8921_lvs4);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_lvs4\") FAILED %d\n", rc);
		mdelay(20);
	}

	if (system_rev <= 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		}
		else
		{
			gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
		}
	} else { 
		if (check_yushanII_flag() == 0) {
			rc = camera_sensor_power_disable(reg_8921_lvs1);
			if (rc < 0)
				pr_err("sensor_power_disable\
					(\"8921_lvs1\") FAILED %d\n", rc);
		}
	}
	mdelay(1);

	return rc;
}

static struct msm_camera_csi_lane_params ov2722_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_ov2722_board_info = {
	.mount_angle = 270,
	.pixel_order_default = MSM_CAMERA_PIXEL_ORDER_BG,	
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 1,
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.csi_lane_params = &ov2722_csi_lane_params,
};

static struct msm_camera_sensor_flash_data flash_ov2722 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov2722_data = {
	.sensor_name	= "ov2722",
	.sensor_reset	= CAM_PIN_GPIO_CAM2_RSTz,
	.sensor_pwd	= CAM_PIN_GPIO_CAM2_STANDBY,
	.vcm_pwd	= 0,
	.vcm_enable	= 0,
	.camera_power_on = m7_ov2722_vreg_on,
	.camera_power_off = m7_ov2722_vreg_off,
	.camera_yushanii_probed = m7_yushanii_probed,
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD
	.pdata	= &m7_msm_camera_csi_device_data[1],
#else
	.pdata	= &m7_msm_camera_csi_device_data[0],
#endif
	.flash_data	= &flash_ov2722,
	.sensor_platform_info = &sensor_ov2722_board_info,
	.gpio_conf = &ov2722_front_cam_gpio_conf,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.use_rawchip = RAWCHIP_DISABLE,

	
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD
	.htc_image = HTC_CAMERA_IMAGE_NONE_BOARD,
#else
	.htc_image = HTC_CAMERA_IMAGE_YUSHANII_BOARD,
#endif
	.hdr_mode = NON_HDR_MODE,
	
	.video_hdr_capability = NON_HDR_MODE,
};
#endif	


#endif	

#endif	
static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};


#ifdef CONFIG_I2C

static struct i2c_board_info m7_camera_i2c_boardinfo_imx135_ar0260[] = {
#ifdef CONFIG_IMX135
		{
		I2C_BOARD_INFO("imx135", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_imx135_data,
		},
#endif
#ifdef CONFIG_AR0260
		{
		I2C_BOARD_INFO("ar0260", 0x90 >> 1),
		.platform_data = &msm_camera_sensor_ar0260_data,
		},
#endif
};

static struct i2c_board_info m7_camera_i2c_boardinfo_imx135_ov2722[] = {
#ifdef CONFIG_IMX135
		{
		I2C_BOARD_INFO("imx135", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_imx135_data,
		},
#endif
#ifdef CONFIG_OV2722
		{
		I2C_BOARD_INFO("ov2722", 0x6c >> 1),
		.platform_data = &msm_camera_sensor_ov2722_data,
		},
#endif
};

static struct i2c_board_info m7_camera_i2c_boardinfo_vd6869_ar0260[] = {
#ifdef CONFIG_VD6869
		{
		I2C_BOARD_INFO("vd6869", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_vd6869_data,
		},
#endif
#ifdef CONFIG_AR0260
		{
		I2C_BOARD_INFO("ar0260", 0x90 >> 1),
		.platform_data = &msm_camera_sensor_ar0260_data,
		},
#endif
};

static struct i2c_board_info m7_camera_i2c_boardinfo_vd6869_ov2722[] = {
#ifdef CONFIG_VD6869
		{
		I2C_BOARD_INFO("vd6869", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_vd6869_data,
		},
#endif
#ifdef CONFIG_OV2722
		{
		I2C_BOARD_INFO("ov2722", 0x6c >> 1),
		.platform_data = &msm_camera_sensor_ov2722_data,
		},
#endif
};

struct i2c_board_info m7_camera_i2c_boardinfo_ov4688_0x6c_ov2722[] = {

#ifdef CONFIG_OV4688
		{
		I2C_BOARD_INFO("ov4688_0x6c", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_ov4688_data,
		},
#endif
#ifdef CONFIG_OV2722
		{
		I2C_BOARD_INFO("ov2722", 0x6c >> 1),
		.platform_data = &msm_camera_sensor_ov2722_data,
		}
#endif
};

struct i2c_board_info m7_camera_i2c_boardinfo_ov4688_0x20_ov2722[] = {

#ifdef CONFIG_OV4688
		{
		I2C_BOARD_INFO("ov4688_0x20", 0x20 >> 1),
		.platform_data = &msm_camera_sensor_ov4688_data,
		},
#endif
#ifdef CONFIG_OV2722
		{
		I2C_BOARD_INFO("ov2722", 0x6c >> 1),
		.platform_data = &msm_camera_sensor_ov2722_data,
		}
#endif
};

#endif

enum camera_sensor_id {
	CAMERA_SENSOR_ID_ST_4M,
	CAMERA_SENSOR_ID_OV_4M,
	CAMERA_SENSOR_ID_SONY_13M,
};

int m7_main_camera_id(void)
{
	int rc = 0;
	int main_camera_id = 0;
	int pull_high_value = 1;
	int pull_low_value = 0;

	struct pm_gpio cam_id_pmic_gpio_start = {
		.direction      = PM_GPIO_DIR_IN,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.pull      = PM_GPIO_PULL_UP_30,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
	};

	struct pm_gpio cam_id_pmic_gpio_end = {
		.direction      = PM_GPIO_DIR_IN,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.pull      = PM_GPIO_PULL_DN,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
	};

	struct pm_gpio cam_id_pmic_gpio_release = {
		.direction      = PM_GPIO_DIR_IN,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.pull      = PM_GPIO_PULL_DN,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_NO,
		.function       = PM_GPIO_FUNC_NORMAL,
	};

	if (system_rev <= 1) { 
		
		rc = gpio_request(CAM_PIN_GPIO_V_RAW_1V8_EN, "V_RAW_1V8_EN");
		pr_info("rawchip external 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
		if (rc) {
			pr_err("rawchip on\
				(\"gpio %d\", 1.2V) FAILED %d\n",
				CAM_PIN_GPIO_V_RAW_1V8_EN, rc);
			goto enable_io_failed;
		}
		gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 1);
	} else { 
		pr_info("%s: 8921_lvs1 1800000\n", __func__);
		rc = camera_sensor_power_enable("8921_lvs1", 1800000, &reg_8921_lvs1);
		pr_info("%s: 8921_lvs1 1800000 (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("sensor_power_enable\
				(\"8921_lvs1\", 1.8V) FAILED %d\n", rc);
			goto enable_io_failed;
		}
		#if defined(CONFIG_OV4688)
			 
			pr_info("%s: 8921_lvs4 1800000\n", __func__);
			rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
			pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
			if (rc < 0) {
				pr_err("sensor_power_enable\
					(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
				goto enable_io_2_failed;
			}
		#endif
	}
	mdelay(1);

	rc = gpio_request(CAM_PIN_CAMERA_ID, "CAM_PIN_CAMERA_ID");
	if (rc) {
		pr_err("request camera_id fail %d\n", rc);
		goto request_camid_failed;
	}
	rc = pm8xxx_gpio_config(CAM_PIN_CAMERA_ID, &cam_id_pmic_gpio_start);
	if (rc) {
		pr_err("%s: cam_id_pmic_gpio_start=%d\n", __func__, rc);
		goto config_camid_failed;
	}
	mdelay(1);
	pull_high_value = gpio_get_value(CAM_PIN_CAMERA_ID);

	rc = pm8xxx_gpio_config(CAM_PIN_CAMERA_ID, &cam_id_pmic_gpio_end);
	if (rc) {
		pr_err("%s: cam_id_pmic_gpio_end=%d\n", __func__, rc);
		goto config_camid_failed;
	}
	mdelay(1);
	pull_low_value = gpio_get_value(CAM_PIN_CAMERA_ID);

	if (pull_high_value == 0 && pull_low_value == 0)
		main_camera_id = CAMERA_SENSOR_ID_ST_4M;
	else if (pull_high_value == 1 && pull_low_value == 1)
		main_camera_id = CAMERA_SENSOR_ID_OV_4M;
	else
		main_camera_id = CAMERA_SENSOR_ID_SONY_13M;
	pr_info("pull_high_value = %d pull_low_value = %d main_camera id = %d\n",
		pull_high_value, pull_low_value, main_camera_id);

	rc = pm8xxx_gpio_config(CAM_PIN_CAMERA_ID, &cam_id_pmic_gpio_release);
	if (rc) {
		pr_err("%s: cam_id_pmic_gpio_release=%d\n", __func__, rc);
		goto config_camid_failed;
	}

config_camid_failed:
	gpio_free(CAM_PIN_CAMERA_ID);

#if defined(CONFIG_OV4688)
request_camid_failed:
	if (system_rev <= 1) { 
		gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	} else { 
		camera_sensor_power_disable(reg_8921_lvs4);
		pr_info("sensor_power_disable\
					(\"8921_lvs4\", 1.8V) \n");
	}
enable_io_2_failed:
	if (system_rev > 1) { 
		camera_sensor_power_disable(reg_8921_lvs1);
		pr_info("sensor_power_disable\
					(\"8921_lvs1\", 1.8V) \n");
	}
#else
request_camid_failed:
	if (system_rev <= 1) { 
		gpio_direction_output(CAM_PIN_GPIO_V_RAW_1V8_EN, 0);
		gpio_free(CAM_PIN_GPIO_V_RAW_1V8_EN);
	} else { 
		camera_sensor_power_disable(reg_8921_lvs1);
	}
#endif

enable_io_failed:

	return main_camera_id;
}

#if defined(CONFIG_RUMBAS_ACT)
void m7_vcm_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	rc = m7_rawchip_vreg_on();
	if (rc < 0) {
		pr_err("%s m7_rawchip_vreg_on failed\n", __func__);
		return;
	}

	
	if (system_rev == 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_1V8_EN, "V_CAM_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
			goto enable_io_fail;
		}
		gpio_direction_output(CAM_PIN_GPIO_V_CAM_1V8_EN, 1);
		gpio_free(CAM_PIN_GPIO_V_CAM_1V8_EN);
		mdelay(5);
	} else if (system_rev >= 2) { 
		pr_info("%s: 8921_lvs4 1800000\n", __func__);
		rc = camera_sensor_power_enable("8921_lvs4", 1800000, &reg_8921_lvs4);
		pr_info("%s: 8921_lvs4 1800000 (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("sensor_power_enable\
				(\"8921_lvs4\", 1.8V) FAILED %d\n", rc);
			goto enable_io_fail;
		}
		mdelay(5);
	}

	
	pr_info("%s: 8921_l8 2900000\n", __func__);
	rc = camera_sensor_power_enable("8921_l8", 2900000, &reg_8921_l8);
	pr_info("%s: 8921_l8 2900000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l8\", 2.9V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}
	mdelay(5);

	
	pr_info("%s: 8921_l9 3100000\n", __func__);
	rc = camera_sensor_power_enable("8921_l9", 3100000, &reg_8921_l9);
	pr_info("%s: 8921_l9 3100000 (%d)\n", __func__, rc);
	if (rc < 0) {
		pr_err("sensor_power_enable\
			(\"8921_l9\", 3.1V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}
	mdelay(1);

	
	
	if (system_rev >= 1) { 
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD	
		pr_info("%s: CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB\n", __func__);
		rc = gpio_request(CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB, "CAM_D1V2_EN");
		pr_info("%s: CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB (%d)\n", __func__, rc);
		if (rc) {
			pr_err("sensor_power_enable\
				(\"gpio %d\", 1.2V) FAILED %d\n",
				CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB, rc);
			goto enable_digital_fail;
		}
		gpio_direction_output(CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB, 1);
		gpio_free(CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB);
#else
		pr_info("[CAM]%s: CAM_PIN_GPIO_MCAM_D1V2_EN\n", __func__);
		rc = gpio_request(CAM_PIN_GPIO_MCAM_D1V2_EN, "CAM_D1V2_EN");
		pr_info("[CAM]%s: CAM_PIN_GPIO_MCAM_D1V2_EN (%d)\n", __func__, rc);
		if (rc) {
			pr_err("[CAM]sensor_power_enable\
				(\"gpio %d\", 1.05V) FAILED %d\n",
				CAM_PIN_GPIO_MCAM_D1V2_EN, rc);
			goto enable_digital_fail;
		}
		gpio_direction_output(CAM_PIN_GPIO_MCAM_D1V2_EN, 1);
		gpio_free(CAM_PIN_GPIO_MCAM_D1V2_EN);
#endif
	} else { 
		pr_info("%s: 8921_l12 1200000\n", __func__);
		rc = camera_sensor_power_enable("8921_l12", 1200000, &reg_8921_l12);
		pr_info("%s: 8921_l12 1200000 (%d)\n", __func__, rc);
		if (rc < 0) {
			pr_err("sensor_power_enable\
				(\"8921_l12\", 1.2V) FAILED %d\n", rc);
			goto enable_digital_fail;
		}
	}
	mdelay(2);
	return;
enable_digital_fail:
	
	camera_sensor_power_disable(reg_8921_l9);
enable_vcm_fail:
	
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	
	if (system_rev == 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_1V8_EN, "V_CAM_1V8_EN");
		if (rc < 0) {
			pr_err("I/O 1v8 off\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		} else {
			gpio_direction_output(CAM_PIN_GPIO_V_CAM_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_CAM_1V8_EN);
		}
	} else if (system_rev >= 2) { 
		rc = camera_sensor_power_disable(reg_8921_lvs4);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_lvs4\") FAILED %d\n", rc);
	}
enable_io_fail:
	return;
}

void m7_vcm_vreg_off(void)
{
	int rc;
	pr_info("%s\n", __func__);

	
	if (system_rev >= 1) { 
#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD	
		rc = gpio_request(CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB, "CAM_D1V2_EN");
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"gpio %d\", 1.05V) FAILED %d\n",
				CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB, rc);
		else {
			gpio_direction_output(CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB, 0);
			gpio_free(CAM_PIN_PMGPIO_V_CAM_D1V2_EN_XB);
		}
#else
		rc = gpio_request(CAM_PIN_GPIO_MCAM_D1V2_EN, "MCAM_D1V2_EN");
		pr_info("I/O 1v2 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_MCAM_D1V2_EN, rc);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"gpio %d\", 1.05V) FAILED %d\n",
				CAM_PIN_GPIO_MCAM_D1V2_EN, rc);
		else {
			gpio_direction_output(CAM_PIN_GPIO_MCAM_D1V2_EN, 0);
			gpio_free(CAM_PIN_GPIO_MCAM_D1V2_EN);
		}
#endif

	} else { 
		pr_info("%s: 8921_l12 off\n", __func__);
		rc = camera_sensor_power_disable(reg_8921_l12);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_l12\") FAILED %d\n", rc);
	}
	mdelay(10);

	
	pr_info("%s: 8921_l9 off\n", __func__);
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l9\") FAILED %d\n", rc);

	
	pr_info("%s: 8921_l8 off\n", __func__);
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("sensor_power_disable\
			(\"8921_l8\") FAILED %d\n", rc);
	mdelay(10);

	
	if (system_rev == 1) { 
		rc = gpio_request(CAM_PIN_GPIO_V_CAM_1V8_EN, "V_CAM_1V8_EN");
		pr_info("I/O 1v8 gpio_request,%d rc(%d)\n", CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		if (rc) {
			pr_err("I/O 1v8 on\
				(\"gpio %d\", 1.8V) FAILED %d\n",
				CAM_PIN_GPIO_V_CAM_1V8_EN, rc);
		}
		else
		{
			gpio_direction_output(CAM_PIN_GPIO_V_CAM_1V8_EN, 0);
			gpio_free(CAM_PIN_GPIO_V_CAM_1V8_EN);
		}
		mdelay(20);
	} else if (system_rev >= 2) { 
		rc = camera_sensor_power_disable(reg_8921_lvs4);
		if (rc < 0)
			pr_err("sensor_power_disable\
				(\"8921_lvs4\") FAILED %d\n", rc);
		mdelay(20);
	}

	m7_rawchip_vreg_off();
}
#endif

void __init m7_init_cam(void)
{
	int main_camera_id = CAMERA_SENSOR_ID_SONY_13M;
	pr_info("%s", __func__);
	msm_gpiomux_install(m7_cam_common_configs,
			ARRAY_SIZE(m7_cam_common_configs));
	platform_device_register(&msm_camera_server);

	platform_device_register(&msm8960_device_i2c_mux_gsbi4);
	platform_device_register(&msm8960_device_csiphy0);
	platform_device_register(&msm8960_device_csiphy1);
	platform_device_register(&msm8960_device_csid0);
	platform_device_register(&msm8960_device_csid1);
	platform_device_register(&msm8960_device_ispif);
	platform_device_register(&msm8960_device_vfe);
	platform_device_register(&msm8960_device_vpe);

#ifdef CONFIG_CAMERA_IMAGE_NONE_BOARD
	pr_info("CAMERA_IMAGE_NONE_BOARD is defined\n");
#else
	pr_info("CAMERA_IMAGE_NONE_BOARD is not defined\n");
#endif

	if (system_rev > 0) 
		main_camera_id = m7_main_camera_id();
	pr_info("main_camera id = %d\n", main_camera_id);

	pr_info("engineerid=%d\n",engineerid);
	pr_info("system_rev=%d\n",system_rev);
#ifdef CONFIG_I2C
	if ( (((engineerid&1)==0) && (system_rev < 2)) ||
	     (((engineerid&1)==1) && (system_rev >= 2)) ) {
		
		
		if (main_camera_id == CAMERA_SENSOR_ID_ST_4M) {
			i2c_register_board_info(APQ_8064_GSBI4_QUP_I2C_BUS_ID,
				m7_camera_i2c_boardinfo_vd6869_ar0260,
				ARRAY_SIZE(m7_camera_i2c_boardinfo_vd6869_ar0260));

			update_yushanII_flag(HTC_CAMERA_IMAGE_YUSHANII_BOARD);
		} else {

			
			msm_camera_sensor_ar0260_data.pdata = &m7_msm_camera_csi_device_data[1];
			

			i2c_register_board_info(APQ_8064_GSBI4_QUP_I2C_BUS_ID,
				m7_camera_i2c_boardinfo_imx135_ar0260,
				ARRAY_SIZE(m7_camera_i2c_boardinfo_imx135_ar0260));

			update_yushanII_flag(HTC_CAMERA_IMAGE_YUSHANII_BOARD);
		}
	} else {
		
		
		if (main_camera_id == CAMERA_SENSOR_ID_ST_4M) {
			i2c_register_board_info(APQ_8064_GSBI4_QUP_I2C_BUS_ID,
				m7_camera_i2c_boardinfo_vd6869_ov2722,
				ARRAY_SIZE(m7_camera_i2c_boardinfo_vd6869_ov2722));

			update_yushanII_flag(HTC_CAMERA_IMAGE_YUSHANII_BOARD);
		} else if(main_camera_id == CAMERA_SENSOR_ID_OV_4M){
			if(1){
				i2c_register_board_info(APQ_8064_GSBI4_QUP_I2C_BUS_ID,
					m7_camera_i2c_boardinfo_ov4688_0x20_ov2722,
					ARRAY_SIZE(m7_camera_i2c_boardinfo_ov4688_0x20_ov2722));
			}else{
				i2c_register_board_info(APQ_8064_GSBI4_QUP_I2C_BUS_ID,
					m7_camera_i2c_boardinfo_ov4688_0x6c_ov2722,
					ARRAY_SIZE(m7_camera_i2c_boardinfo_ov4688_0x6c_ov2722));
			}
			update_yushanII_flag(HTC_CAMERA_IMAGE_YUSHANII_BOARD);
		}
		else {

			
			msm_camera_sensor_ov2722_data.pdata = &m7_msm_camera_csi_device_data[1];
			

			i2c_register_board_info(APQ_8064_GSBI4_QUP_I2C_BUS_ID,
				m7_camera_i2c_boardinfo_imx135_ov2722,
				ARRAY_SIZE(m7_camera_i2c_boardinfo_imx135_ov2722));

			update_yushanII_flag(HTC_CAMERA_IMAGE_YUSHANII_BOARD);
		}
	}
#endif
}

#endif	



