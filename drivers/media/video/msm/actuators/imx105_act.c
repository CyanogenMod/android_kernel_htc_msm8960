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

#ifdef USE_RAWCHIP_AF
#define	IMX105_TOTAL_STEPS_NEAR_TO_FAR			128
#else
#define	IMX105_TOTAL_STEPS_NEAR_TO_FAR			58
#endif


#define REG_VCM_I2C_ADDR			0x34
#define REG_VCM_CODE_MSB			0x3403
#define REG_VCM_CODE_LSB			0x3402



#define DIV_CEIL(x, y) (x/y + (x%y) ? 1 : 0)

int32_t msm_camera_i2c_write_lens_position(int16_t lens_position);

DEFINE_MUTEX(imx105_act_mutex);
static struct msm_actuator_ctrl_t imx105_act_t;

static struct region_params_t g_regions[] = {
	/* step_bound[0] - macro side boundary
	 * step_bound[1] - infinity side boundary
	 */
	/* Region 1 */
	{
		.step_bound = {IMX105_TOTAL_STEPS_NEAR_TO_FAR, 0},
		.code_per_step = 2,
	},
};

static uint16_t g_scenario[] = {
	/* MOVE_NEAR and MOVE_FAR dir*/
	IMX105_TOTAL_STEPS_NEAR_TO_FAR,
};

static struct damping_params_t g_damping[] = {
	/* MOVE_NEAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 2,
		.damping_delay = 0,
	},
};

static struct damping_t g_damping_params[] = {
	/* MOVE_NEAR and MOVE_FAR dir */
	/* Region 1 */
	{
		.ringing_params = g_damping,
	},
};

static struct msm_actuator_info *imx105_msm_actuator_info;

static int32_t imx105_poweron_af(void)
{
	int32_t rc = 0;
	pr_info("%s enable AF actuator, gpio = %d : E\n", __func__,
			imx105_msm_actuator_info->vcm_pwd);
	mdelay(1);
	rc = gpio_request(imx105_msm_actuator_info->vcm_pwd, "imx105");
	if (!rc)
		gpio_direction_output(imx105_msm_actuator_info->vcm_pwd, 1);
	else
		pr_err("%s: AF PowerON gpio_request failed %d\n", __func__, rc);
	gpio_free(imx105_msm_actuator_info->vcm_pwd);
	mdelay(1);

	pr_info("%s enable AF actuator, gpio = %d : X\n", __func__,
			imx105_msm_actuator_info->vcm_pwd);	
	return rc;
}

static void imx105_poweroff_af(void)
{
	int32_t rc = 0;

	pr_info("%s disable AF actuator, gpio = %d\n", __func__,
			imx105_msm_actuator_info->vcm_pwd);

	msleep(1);
	rc = gpio_request(imx105_msm_actuator_info->vcm_pwd, "imx105");
	if (!rc)
		gpio_direction_output(imx105_msm_actuator_info->vcm_pwd, 0);
	else
		pr_err("%s: AF PowerOFF gpio_request failed %d\n", __func__, rc);
	gpio_free(imx105_msm_actuator_info->vcm_pwd);
	msleep(1);
}

int32_t imx105_msm_actuator_init_table(
		struct msm_actuator_ctrl_t *a_ctrl)
{
	int32_t rc = 0;

	pr_info("%s called\n", __func__);

	if (a_ctrl->func_tbl.actuator_set_params)
		a_ctrl->func_tbl.actuator_set_params(a_ctrl);

	if (imx105_act_t.step_position_table) {
		pr_info("%s table inited\n", __func__);
		return rc;
	}

	/* Fill step position table */
	if (a_ctrl->step_position_table != NULL) {
		kfree(a_ctrl->step_position_table);
		a_ctrl->step_position_table = NULL;
	}
	a_ctrl->step_position_table =
		kmalloc(sizeof(uint16_t) * (a_ctrl->set_info.total_steps + 1),
				GFP_KERNEL);

	if (a_ctrl->step_position_table != NULL) {
		uint16_t i = 0;
		uint16_t imx105_nl_region_boundary1 = 3;
		uint16_t imx105_nl_region_code_per_step1 = 48;
		uint16_t imx105_nl_region_boundary2 = 5;
		uint16_t imx105_nl_region_code_per_step2 = 12;
		uint16_t imx105_l_region_code_per_step = 16;
		uint16_t imx105_max_value = 1023;

		a_ctrl->step_position_table[0] = a_ctrl->initial_code;
		for (i = 1; i <= a_ctrl->set_info.total_steps; i++){
#ifdef USE_RAWCHIP_AF
			if (imx105_msm_actuator_info->use_rawchip_af)
				a_ctrl->step_position_table[i] =
					a_ctrl->step_position_table[i-1] + 4;
			else
#endif
			{
				if (i <= imx105_nl_region_boundary1) {
					a_ctrl->step_position_table[i] =
						a_ctrl->step_position_table[i-1]
						+ imx105_nl_region_code_per_step1;
				} else if (i <= imx105_nl_region_boundary2) {
					a_ctrl->step_position_table[i] =
						a_ctrl->step_position_table[i-1]
						+ imx105_nl_region_code_per_step2;
				} else {
					a_ctrl->step_position_table[i] =
						a_ctrl->step_position_table[i-1]
						+ imx105_l_region_code_per_step;
				}

				if (a_ctrl->step_position_table[i] > imx105_max_value)
					a_ctrl->step_position_table[i] = imx105_max_value;

				/*pr_info("%s step_position_table[%d] = %d\n", __func__,i,a_ctrl->step_position_table[i]);*/
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

int32_t imx105_msm_actuator_move_focus(
		struct msm_actuator_ctrl_t *a_ctrl,
		int dir,
		int32_t num_steps)
{
	int32_t rc = 0;
	int8_t sign_dir = 0;
	int16_t dest_step_pos = 0;

	pr_info("%s called, dir %d, num_steps %d\n",
			__func__,
			dir,
			num_steps);

	/* Determine sign direction */
	if (dir == MOVE_NEAR){
		sign_dir = 1;
		pr_info("%s MOVE_NEAR\n",
				__func__);
	}
	else if (dir == MOVE_FAR){
		sign_dir = -1;
		pr_info("%s MOVE_FAR\n",
				__func__);
	}
	else {
		pr_err("Illegal focus direction\n");
		rc = -EINVAL;
		return rc;
	}

	/* Determine destination step position */
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
		pr_info("%s current step: %d\n", __func__, a_ctrl->curr_step_pos);
	}

	return rc;
}

int imx105_actuator_af_power_down(void *params)
{
	int rc = 0;
	pr_info("%s called\n", __func__);

	rc = (int) msm_actuator_af_power_down(&imx105_act_t);
	imx105_poweroff_af();
	return rc;
}

static int32_t imx105_wrapper_i2c_write(struct msm_actuator_ctrl_t *a_ctrl, int16_t next_lens_position, void *params)
{
	int32_t rc = 0;


	pr_info("%s next_lens_position : %d\n", __func__, next_lens_position);

	rc = msm_camera_i2c_write_lens_position(next_lens_position);

	return rc;
}

int32_t imx105_act_write_focus(
		struct msm_actuator_ctrl_t *a_ctrl,
		uint16_t curr_lens_pos,
		struct damping_params_t *damping_params,
		int8_t sign_direction,
		int16_t code_boundary)
{
	int32_t rc = 0;
	uint16_t dac_value = 0;
	/*
	   pr_info("%s called, curr lens pos = %d, code_boundary = %d\n",
	   __func__,
	   curr_lens_pos,
	   code_boundary);
	   */
	if (sign_direction == 1)
		dac_value = (code_boundary - curr_lens_pos);
	else
		dac_value = (curr_lens_pos - code_boundary);
	/*
	   pr_info("%s dac_value = %d\n",
	   __func__,
	   dac_value);
	   */
	rc = a_ctrl->func_tbl.actuator_i2c_write(a_ctrl, dac_value, NULL);

	return rc;
}

static int32_t imx105_act_init_focus(struct msm_actuator_ctrl_t *a_ctrl)
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

static const struct i2c_device_id imx105_act_i2c_id[] = {
	{"imx105_act", (kernel_ulong_t)&imx105_act_t},
	{ }
};

static int imx105_act_config(
		void __user *argp)
{
	pr_info("%s called\n", __func__);
	return (int) msm_actuator_config(&imx105_act_t,
			imx105_msm_actuator_info, argp); /* HTC Angie 20111212 - Rawchip */
}

static int imx105_i2c_add_driver_table(
		void)
{
	int32_t rc = 0;

	pr_info("%s called\n", __func__);

	rc = imx105_poweron_af();
	if (rc < 0) {
		pr_err("%s power on failed\n", __func__);
		return (int) rc;
	}
	imx105_act_t.step_position_table = NULL;
	rc = imx105_act_t.func_tbl.actuator_init_table(&imx105_act_t);
	if (rc < 0) {
		pr_err("%s init table failed\n", __func__);
		return (int) rc;
	}

	return (int) rc;
}

static struct i2c_driver imx105_act_i2c_driver = {
	.id_table = imx105_act_i2c_id,
	.probe  = msm_actuator_i2c_probe,
	.remove = __exit_p(imx105_act_i2c_remove),
	.driver = {
		.name = "imx105_act",
	},
};

static int __init imx105_i2c_add_driver(void)
{
	pr_info("%s called\n", __func__);
	return i2c_add_driver(imx105_act_t.i2c_driver);
}

static struct v4l2_subdev_core_ops imx105_act_subdev_core_ops;

static struct v4l2_subdev_ops imx105_act_subdev_ops = {
	.core = &imx105_act_subdev_core_ops,
};

static int32_t imx105_act_create_subdevice(
		void *board_info,
		void *sdev)
{
	pr_info("%s called\n", __func__);

	imx105_msm_actuator_info = (struct msm_actuator_info *)board_info;

	return (int) msm_actuator_create_subdevice(&imx105_act_t,
			imx105_msm_actuator_info->board_info,
			(struct v4l2_subdev *)sdev);
}

static struct msm_actuator_ctrl_t imx105_act_t = {
	.i2c_driver = &imx105_act_i2c_driver,
	.i2c_addr = REG_VCM_I2C_ADDR,
	.act_v4l2_subdev_ops = &imx105_act_subdev_ops,
	.actuator_ext_ctrl = {
		.a_init_table = imx105_i2c_add_driver_table,
		.a_power_down = imx105_actuator_af_power_down,
		.a_create_subdevice = imx105_act_create_subdevice,
		.a_config = imx105_act_config,
	},

	.i2c_client = {
		.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	},

	.set_info = {
		.total_steps = IMX105_TOTAL_STEPS_NEAR_TO_FAR,		
		.gross_steps = 3,	/*[TBD]*/
		.fine_steps = 1,	/*[TBD]*/

	},

	.curr_step_pos = 0,
	.curr_region_index = 0,
	.initial_code = 0,	/*[TBD]*/
	.actuator_mutex = &imx105_act_mutex,

	.func_tbl = {
		.actuator_init_table = imx105_msm_actuator_init_table,
		.actuator_move_focus = imx105_msm_actuator_move_focus,
		.actuator_write_focus = imx105_act_write_focus,
		.actuator_set_default_focus = msm_actuator_set_default_focus,
		.actuator_init_focus = imx105_act_init_focus,
		.actuator_i2c_write = imx105_wrapper_i2c_write,
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

	/* Initialize scenario */
	.ringing_scenario[MOVE_NEAR] = g_scenario,
	.scenario_size[MOVE_NEAR] = ARRAY_SIZE(g_scenario),
	.ringing_scenario[MOVE_FAR] = g_scenario,
	.scenario_size[MOVE_FAR] = ARRAY_SIZE(g_scenario),

	/* Initialize region params */
	.region_params = g_regions,
	.region_size = ARRAY_SIZE(g_regions),

	/* Initialize damping params */
	.damping[MOVE_NEAR] = g_damping_params,
	.damping[MOVE_FAR] = g_damping_params,
};
subsys_initcall(imx105_i2c_add_driver);

MODULE_DESCRIPTION("imx105 actuator");
MODULE_LICENSE("GPL v2");
