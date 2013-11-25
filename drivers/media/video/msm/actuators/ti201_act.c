/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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

#include "msm_actuator.h"
#include "msm_camera_i2c.h"
#include <mach/gpio.h>

#define	TI201_TOTAL_STEPS_NEAR_TO_FAR			30
#define	TI201_TOTAL_STEPS_NEAR_TO_FAR_RAWCHIP_AF			256 

#define REG_VCM_NEW_CODE			0x30F2
#define REG_VCM_I2C_ADDR			0x1C
#define REG_VCM_CODE_MSB			0x03
#define REG_VCM_CODE_LSB			0x04
#define REG_VCM_MODE			0x06
#define REG_VCM_FREQ			0x07
#define REG_VCM_RING_CTRL			0x400

int ti201_sharp_kernel_step_table[TI201_TOTAL_STEPS_NEAR_TO_FAR+1]
	= {304, 309, 314, 319, 324, 329, 333, 339, 344, 350,
	355, 361, 366, 372, 378, 385, 391, 399, 406, 416,
	425, 436, 446, 485, 469, 482, 494, 508, 522, 537, 552};

int ti201_liteon_kernel_step_table[TI201_TOTAL_STEPS_NEAR_TO_FAR+1]
	= {225, 229, 233, 237, 241, 245, 249, 254, 258, 263,
	267, 272, 276, 281, 286, 291, 296, 303, 309, 317,
	324, 333, 341, 351, 360, 371, 381, 393, 404, 417, 429};

#define DIV_CEIL(x, y) (x/y + (x%y) ? 1 : 0)
#if 0
#undef LINFO
#define LINFO pr_info
#endif
DEFINE_MUTEX(ti201_act_mutex);
static struct msm_actuator_ctrl_t ti201_act_t;

static struct region_params_t g_regions[] = {
	
	{
		.step_bound = {TI201_TOTAL_STEPS_NEAR_TO_FAR, 0},
		.code_per_step = 2,
	},
};

static uint16_t g_scenario[] = {
	
	TI201_TOTAL_STEPS_NEAR_TO_FAR,
};

static struct damping_params_t g_damping[] = {
	
	
	{
		.damping_step = 2,
		.damping_delay = 0,
	},
};

static struct damping_t g_damping_params[] = {
	
	
	{
		.ringing_params = g_damping,
	},
};

static struct msm_actuator_info *ti201_msm_actuator_info;

static int32_t ti201_poweron_af(void)
{
	int32_t rc = 0;
	pr_info("%s enable AF actuator, gpio = %d\n", __func__,
			ti201_msm_actuator_info->vcm_pwd);
	mdelay(1);
	rc = gpio_request(ti201_msm_actuator_info->vcm_pwd, "ti201");
	if (!rc)
		gpio_direction_output(ti201_msm_actuator_info->vcm_pwd, 1);
	else
		pr_err("%s: AF PowerON gpio_request failed %d\n", __func__, rc);
	gpio_free(ti201_msm_actuator_info->vcm_pwd);
	mdelay(1);
	return rc;
}

static void ti201_poweroff_af(void)
{
	int32_t rc = 0;

	pr_info("%s disable AF actuator, gpio = %d\n", __func__,
			ti201_msm_actuator_info->vcm_pwd);

	msleep(1);
	rc = gpio_request(ti201_msm_actuator_info->vcm_pwd, "ti201");
	if (!rc)
		gpio_direction_output(ti201_msm_actuator_info->vcm_pwd, 0);
	else
		pr_err("%s: AF PowerOFF gpio_request failed %d\n", __func__, rc);
	gpio_free(ti201_msm_actuator_info->vcm_pwd);
	msleep(1);
}

int32_t ti201_msm_actuator_init_table(
	struct msm_actuator_ctrl_t *a_ctrl)
{
	int32_t rc = 0;
	int *ref_table = NULL;

	LINFO("%s called\n", __func__);

	if (a_ctrl->func_tbl.actuator_set_params)
		a_ctrl->func_tbl.actuator_set_params(a_ctrl);
#if 0
	if (ti201_act_t.step_position_table) {
		LINFO("%s table inited\n", __func__);
		return rc;
	}
#endif

	if (ti201_msm_actuator_info->use_rawchip_af && a_ctrl->af_algo == AF_ALGO_RAWCHIP)
		a_ctrl->set_info.total_steps = TI201_TOTAL_STEPS_NEAR_TO_FAR_RAWCHIP_AF;
	else {
		a_ctrl->set_info.total_steps = TI201_TOTAL_STEPS_NEAR_TO_FAR;
		ref_table = (a_ctrl->af_OTP_info.VCM_Vendor == 0x1) ? ti201_sharp_kernel_step_table : ti201_liteon_kernel_step_table;
		if (a_ctrl->af_OTP_info.VCM_OTP_Read)
			a_ctrl->initial_code = a_ctrl->af_OTP_info.VCM_Infinity;
		else
			a_ctrl->initial_code = ref_table[0];
	}

	
	if (a_ctrl->step_position_table != NULL) {
		kfree(a_ctrl->step_position_table);
		a_ctrl->step_position_table = NULL;
	}
	a_ctrl->step_position_table =
		kmalloc(sizeof(uint16_t) * (a_ctrl->set_info.total_steps + 1),
			GFP_KERNEL);

	if (a_ctrl->step_position_table != NULL) {
		uint16_t i = 0;
		uint16_t ti201_nl_region_boundary1 = 2;
		uint16_t ti201_nl_region_code_per_step1 = 32;
		uint16_t ti201_l_region_code_per_step = 16;
		uint16_t ti201_max_value = 1023;

		a_ctrl->step_position_table[0] = a_ctrl->initial_code;

		for (i = 1; i <= a_ctrl->set_info.total_steps; i++) {
			if (ti201_msm_actuator_info->use_rawchip_af && a_ctrl->af_algo == AF_ALGO_RAWCHIP) 
				a_ctrl->step_position_table[i] =
					a_ctrl->step_position_table[i-1] + 4;
			else
			{
				if (ref_table != NULL) {
					if (a_ctrl->af_OTP_info.VCM_OTP_Read)
						a_ctrl->step_position_table[i] = a_ctrl->af_OTP_info.VCM_Infinity +
							(ref_table[i] - ref_table[0]) * (a_ctrl->af_OTP_info.VCM_Macro - a_ctrl->af_OTP_info.VCM_Infinity) /
							(ref_table[a_ctrl->set_info.total_steps] - ref_table[0]);
					else
						a_ctrl->step_position_table[i] = ref_table[i];
				} else {
					if (i <= ti201_nl_region_boundary1) {
						a_ctrl->step_position_table[i] =
							a_ctrl->step_position_table[i-1]
							+ ti201_nl_region_code_per_step1;
					} else {
						a_ctrl->step_position_table[i] =
							a_ctrl->step_position_table[i-1]
							+ ti201_l_region_code_per_step;
					}
				}

				if (a_ctrl->step_position_table[i] > ti201_max_value)
					a_ctrl->step_position_table[i] = ti201_max_value;
			}
		}
		a_ctrl->curr_step_pos = 0;
		a_ctrl->curr_region_index = 0;
	} else {
		pr_err("%s table init failed\n", __func__);
		rc = -EFAULT;
	}

	return rc;
}

int32_t ti201_msm_actuator_move_focus(
	struct msm_actuator_ctrl_t *a_ctrl,
	int dir,
	int32_t num_steps)
{
	int32_t rc = 0;
	int8_t sign_dir = 0;
	int16_t dest_step_pos = 0;

	LINFO("%s called, dir %d, num_steps %d\n",
		__func__,
		dir,
		num_steps);

	
	if (dir == MOVE_NEAR)
		sign_dir = 1;
	else if (dir == MOVE_FAR)
		sign_dir = -1;
	else {
		pr_err("Illegal focus direction\n");
		rc = -EINVAL;
		return rc;
	}

	
	dest_step_pos = a_ctrl->curr_step_pos +
		(sign_dir * num_steps);

	if (dest_step_pos < 0)
		dest_step_pos = 0;
	else if (dest_step_pos > a_ctrl->set_info.total_steps)
		dest_step_pos = a_ctrl->set_info.total_steps;

	if (dest_step_pos == a_ctrl->curr_step_pos)
		return rc;

	rc = a_ctrl->func_tbl.actuator_i2c_write(a_ctrl,
		a_ctrl->step_position_table[dest_step_pos], NULL);
	if (rc < 0) {
		pr_err("%s focus move failed\n", __func__);
		return rc;
	} else {
		a_ctrl->curr_step_pos = dest_step_pos;
		LINFO("%s current step: %d\n", __func__, a_ctrl->curr_step_pos);
	}

	return rc;
}

int ti201_actuator_af_power_down(void *params)
{
	int rc = 0;
	LINFO("%s called\n", __func__);

#if defined(CONFIG_ACT_OIS_BINDER)
	if (ti201_msm_actuator_info->oisbinder_power_down)
		ti201_msm_actuator_info->oisbinder_power_down();
#endif

	rc = (int) msm_actuator_af_power_down(&ti201_act_t);
	ti201_poweroff_af();
	return rc;
}

static int32_t ti201_wrapper_i2c_write(struct msm_actuator_ctrl_t *a_ctrl,
	int16_t next_lens_position, void *params)
{
	int32_t rc = 0;

	rc = msm_camera_i2c_write(&a_ctrl->i2c_client,
		REG_VCM_CODE_MSB,
		((next_lens_position & 0x0300) >> 8),	
		MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s VCM_CODE_MSB i2c write failed (%d)\n", __func__, rc);
		return rc;
	}

	rc = msm_camera_i2c_write(&a_ctrl->i2c_client,
		REG_VCM_CODE_LSB,
		(next_lens_position & 0x00FF),	
		MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s VCM_CODE_LSB i2c write failed (%d)\n", __func__, rc);
		return rc;
	}

	return rc;
}

int32_t ti201_act_write_focus(
	struct msm_actuator_ctrl_t *a_ctrl,
	uint16_t curr_lens_pos,
	struct damping_params_t *damping_params,
	int8_t sign_direction,
	int16_t code_boundary)
{
	int32_t rc = 0;
	uint16_t dac_value = 0;

	LINFO("%s called, curr lens pos = %d, code_boundary = %d\n",
		  __func__,
		  curr_lens_pos,
		  code_boundary);

	if (sign_direction == 1)
		dac_value = (code_boundary - curr_lens_pos);
	else
		dac_value = (curr_lens_pos - code_boundary);

	LINFO("%s dac_value = %d\n",
		  __func__,
		  dac_value);

	rc = a_ctrl->func_tbl.actuator_i2c_write(a_ctrl, dac_value, NULL);

	return rc;
}

static int32_t ti201_act_init_focus(struct msm_actuator_ctrl_t *a_ctrl)
{
	int32_t rc = 0;

	rc = a_ctrl->func_tbl.actuator_i2c_write(a_ctrl, a_ctrl->initial_code,
		NULL);
	if (rc < 0)
		pr_err("%s i2c write failed\n", __func__);
	else
		a_ctrl->curr_step_pos = 0;

	return rc;
}

int32_t  ti201_act_set_af_value(struct msm_actuator_ctrl_t *a_ctrl, af_value_t af_value)
{
	int32_t rc = 0;
	uint8_t OTP_data[8] = {0,0,0,0,0,0,0,0};
	int16_t VCM_Infinity = 0;
	int32_t otp_deviation = 0;

	if (ti201_msm_actuator_info->use_rawchip_af && a_ctrl->af_algo == AF_ALGO_RAWCHIP)
		return rc;

	OTP_data[0] = af_value.VCM_START_MSB;
	OTP_data[1] = af_value.VCM_START_LSB;
	OTP_data[2] = af_value.AF_INF_MSB;
	OTP_data[3] = af_value.AF_INF_LSB;
	OTP_data[4] = af_value.AF_MACRO_MSB;
	OTP_data[5] = af_value.AF_MACRO_LSB;
	

	if (OTP_data[2] || OTP_data[3] || OTP_data[4] || OTP_data[5]) {
		a_ctrl->af_OTP_info.VCM_OTP_Read = true;
		a_ctrl->af_OTP_info.VCM_Vendor = af_value.VCM_VENDOR;
		otp_deviation = (a_ctrl->af_OTP_info.VCM_Vendor == 0x1) ? 160 : 60;
		a_ctrl->af_OTP_info.VCM_Start = 0;
		VCM_Infinity = (int16_t)(OTP_data[2]<<8 | OTP_data[3]) - otp_deviation;
		if (VCM_Infinity < 0){
			a_ctrl->af_OTP_info.VCM_Infinity = 0;
		}else{
			a_ctrl->af_OTP_info.VCM_Infinity = VCM_Infinity;
		}
		a_ctrl->af_OTP_info.VCM_Macro = (OTP_data[4]<<8 | OTP_data[5]);
	}
	pr_info("OTP_data[2] %d OTP_data[3] %d OTP_data[4] %d OTP_data[5] %d\n",
		OTP_data[2], OTP_data[3], OTP_data[4], OTP_data[5]);
	pr_info("VCM_Start = %d\n", a_ctrl->af_OTP_info.VCM_Start);
	pr_info("VCM_Infinity = %d\n", a_ctrl->af_OTP_info.VCM_Infinity);
	pr_info("VCM_Macro = %d\n", a_ctrl->af_OTP_info.VCM_Macro);
	pr_info("VCM Module vendor =  = %d\n", a_ctrl->af_OTP_info.VCM_Vendor);
	return rc;
}

#if defined(CONFIG_ACT_OIS_BINDER)
int32_t ti201_act_set_ois_mode(struct msm_actuator_ctrl_t *a_ctrl, int ois_mode)
{
	int32_t rc = 0;

	pr_info("[OIS]  %s  ois_mode:%d\n", __func__, ois_mode);
	if (ti201_msm_actuator_info->oisbinder_act_set_ois_mode)
		rc = ti201_msm_actuator_info->oisbinder_act_set_ois_mode(ois_mode);
	return rc;
}
int32_t ti201_act_update_ois_tbl(struct msm_actuator_ctrl_t *a_ctrl, struct sensor_actuator_info_t * sensor_actuator_info)
{
	int32_t rc = 0;

	pr_info("[OIS]  %s  startup_mode=%d\n", __func__, sensor_actuator_info->startup_mode);
	if (ti201_msm_actuator_info->oisbinder_mappingTbl_i2c_write)
		rc = ti201_msm_actuator_info->oisbinder_mappingTbl_i2c_write(sensor_actuator_info->startup_mode, sensor_actuator_info);
	return rc;
}
#endif

static const struct i2c_device_id ti201_act_i2c_id[] = {
	{"ti201_act", (kernel_ulong_t)&ti201_act_t},
	{ }
};

static int ti201_act_config(
	void __user *argp)
{
	LINFO("%s called\n", __func__);
	return (int) msm_actuator_config(&ti201_act_t,
		ti201_msm_actuator_info, argp); 
}

static int ti201_i2c_add_driver_table(
	void)
{
	int32_t rc = 0;

	pr_info("%s called\n", __func__);

	rc = ti201_poweron_af();
	if (rc < 0) {
		pr_err("%s power on failed\n", __func__);
		return (int) rc;
	}

	rc = msm_camera_i2c_write(&ti201_act_t.i2c_client,
		0x02,
		0x02,
		MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s 0x02 ring enable register i2c write failed (%d)\n", __func__, rc);
		return rc;
	}
	
	
	

	
	
	
	rc = msm_camera_i2c_write(&ti201_act_t.i2c_client,
		REG_VCM_MODE,
		0x03,
		MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s REG_VCM_MODE i2c write failed (%d)\n", __func__, rc);
		return rc;
	}

	
	
	
	
	rc = msm_camera_i2c_write(&ti201_act_t.i2c_client,
		REG_VCM_FREQ,
		0x61,
		MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s VCM_CODE_LSB i2c write failed (%d)\n", __func__, rc);
		return rc;
	}

	ti201_act_t.step_position_table = NULL;
	rc = ti201_act_t.func_tbl.actuator_init_table(&ti201_act_t);
	if (rc < 0) {
		pr_err("%s init table failed\n", __func__);
		return (int) rc;
	}

	rc = msm_camera_i2c_write(&(ti201_act_t.i2c_client),
		0x0001, 0x01,
		MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s i2c write failed\n", __func__);
		return (int) rc;
	}

#if defined(CONFIG_ACT_OIS_BINDER)
	if (ti201_msm_actuator_info->oisbinder_open_init)
		ti201_msm_actuator_info->oisbinder_open_init();
#endif

	return (int) rc;
}

static struct i2c_driver ti201_act_i2c_driver = {
	.id_table = ti201_act_i2c_id,
	.probe  = msm_actuator_i2c_probe,
	.remove = __exit_p(ti201_act_i2c_remove),
	.driver = {
		.name = "ti201_act",
	},
};

static int __init ti201_i2c_add_driver(
	void)
{
	pr_info("%s called\n", __func__);
	return i2c_add_driver(ti201_act_t.i2c_driver);
}

static struct v4l2_subdev_core_ops ti201_act_subdev_core_ops;

static struct v4l2_subdev_ops ti201_act_subdev_ops = {
	.core = &ti201_act_subdev_core_ops,
};

static int32_t ti201_act_create_subdevice(
	void *board_info,
	void *sdev)
{
	int rc = 0;

	LINFO("%s called\n", __func__);

	ti201_msm_actuator_info = (struct msm_actuator_info *)board_info;

	rc = (int) msm_actuator_create_subdevice(&ti201_act_t,
		ti201_msm_actuator_info->board_info,
		(struct v4l2_subdev *)sdev);

#if defined(CONFIG_ACT_OIS_BINDER)
	if (ti201_msm_actuator_info->oisbinder_i2c_add_driver)
		ti201_msm_actuator_info->oisbinder_i2c_add_driver(&(ti201_act_t.i2c_client));
#endif

	return rc;
}

static struct msm_actuator_ctrl_t ti201_act_t = {
	.i2c_driver = &ti201_act_i2c_driver,
	.i2c_addr = REG_VCM_I2C_ADDR,
	.act_v4l2_subdev_ops = &ti201_act_subdev_ops,
	.actuator_ext_ctrl = {
		.a_init_table = ti201_i2c_add_driver_table,
		.a_power_down = ti201_actuator_af_power_down,
		.a_create_subdevice = ti201_act_create_subdevice,
		.a_config = ti201_act_config,
#if defined(CONFIG_ACT_OIS_BINDER)
		.is_ois_supported = 1,
#endif
	},

	.i2c_client = {
		.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	},

	.set_info = {
		.total_steps = TI201_TOTAL_STEPS_NEAR_TO_FAR_RAWCHIP_AF, 
		.gross_steps = 3,	
		.fine_steps = 1,	
	},

	.curr_step_pos = 0,
	.curr_region_index = 0,
	.initial_code = 0,	
	.actuator_mutex = &ti201_act_mutex,
	.af_algo = AF_ALGO_RAWCHIP, 

	.func_tbl = {
		.actuator_init_table = ti201_msm_actuator_init_table,
		.actuator_move_focus = ti201_msm_actuator_move_focus,
		.actuator_write_focus = ti201_act_write_focus,
		.actuator_set_default_focus = msm_actuator_set_default_focus,
		.actuator_init_focus = ti201_act_init_focus,
		.actuator_i2c_write = ti201_wrapper_i2c_write,
		.actuator_set_af_value = ti201_act_set_af_value,
#if defined(CONFIG_ACT_OIS_BINDER)
		.actuator_set_ois_mode = ti201_act_set_ois_mode,
		.actuator_update_ois_tbl = ti201_act_update_ois_tbl,
#endif
	},

	.get_info = {	
		.focal_length_num = 46,
		.focal_length_den = 10,
		.f_number_num = 265,
		.f_number_den = 100,
		.f_pix_num = 14,
		.f_pix_den = 10,
		.total_f_dist_num = 197681,
		.total_f_dist_den = 1000,
	},

	
	.ringing_scenario[MOVE_NEAR] = g_scenario,
	.scenario_size[MOVE_NEAR] = ARRAY_SIZE(g_scenario),
	.ringing_scenario[MOVE_FAR] = g_scenario,
	.scenario_size[MOVE_FAR] = ARRAY_SIZE(g_scenario),

	
	.region_params = g_regions,
	.region_size = ARRAY_SIZE(g_regions),

	
	.damping[MOVE_NEAR] = g_damping_params,
	.damping[MOVE_FAR] = g_damping_params,
};

subsys_initcall(ti201_i2c_add_driver);
MODULE_DESCRIPTION("TI201 actuator");
MODULE_LICENSE("GPL v2");
