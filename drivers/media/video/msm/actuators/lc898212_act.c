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

#define	LC898212_TOTAL_STEPS_NEAR_TO_FAR			  30 
#define LC898212_TOTAL_STEPS_NEAR_TO_FAR_RAWCHIP_AF                        256 

#define REG_VCM_I2C_ADDR			0xe4
#define REG_VCM_CODE_MSB			0x04
#define REG_VCM_CODE_LSB			0x05

#define DEFAULT_BIAS 0x40
#define DEFAULT_OFFSET 0x80
#define DEFAULT_INFINITY 0x7000
#define DEFAULT_MACRO -0x7000


DEFINE_MUTEX(lc898212_act_mutex);
static struct msm_actuator_ctrl_t lc898212_act_t;

static struct region_params_t g_regions[] = {
	
	{
		.step_bound = {LC898212_TOTAL_STEPS_NEAR_TO_FAR, 0},
		.code_per_step = 2,
	},
};

static uint16_t g_scenario[] = {
	
	LC898212_TOTAL_STEPS_NEAR_TO_FAR,
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

static struct msm_actuator_info *lc898212_msm_actuator_info;

static int32_t lc898212_poweron_af(void)
{
	int32_t rc = 0;
	pr_info("%s enable AF actuator, gpio = %d\n", __func__,
			lc898212_msm_actuator_info->vcm_pwd);
	mdelay(1);
	rc = gpio_request(lc898212_msm_actuator_info->vcm_pwd, "lc898212");
	if (!rc)
		gpio_direction_output(lc898212_msm_actuator_info->vcm_pwd, 1);
	else
		pr_err("%s: AF PowerON gpio_request failed %d\n", __func__, rc);
	gpio_free(lc898212_msm_actuator_info->vcm_pwd);
	mdelay(1);
	return rc;
}

static void lc898212_poweroff_af(void)
{
	int32_t rc = 0;

	pr_info("%s disable AF actuator, gpio = %d\n", __func__,
			lc898212_msm_actuator_info->vcm_pwd);

	msleep(1);
	rc = gpio_request(lc898212_msm_actuator_info->vcm_pwd, "lc898212");
	if (!rc)
		gpio_direction_output(lc898212_msm_actuator_info->vcm_pwd, 0);
	else
		pr_err("%s: AF PowerOFF gpio_request failed %d\n", __func__, rc);
	gpio_free(lc898212_msm_actuator_info->vcm_pwd);
	msleep(1);
}
#define Q_NUMBER (0x100)
#define	TO_Q(v)	((v)*Q_NUMBER)
#define BACK_Q(v) ((v)/Q_NUMBER)

static const int32_t dof[]={

    TO_Q(100.00/100.0),
    TO_Q(98.04/100.0),
    TO_Q(96.08/100.0),
    TO_Q(94.12/100.0),
    TO_Q(92.16/100.0),
    TO_Q(90.19/100.0),
    TO_Q(88.23/100.0),
    TO_Q(86.03/100.0),
    TO_Q(83.82/100.0),
    TO_Q(81.62/100.0),
    TO_Q(79.41/100.0),
    TO_Q(77.20/100.0),
    TO_Q(75.00/100.0),
    TO_Q(72.55/100.0),
    TO_Q(70.10/100.0),
    TO_Q(67.65/100.0),
    TO_Q(65.20/100.0),
    TO_Q(62.01/100.0),
    TO_Q(58.82/100.0),
    TO_Q(55.15/100.0),
    TO_Q(51.47/100.0),
    TO_Q(47.30/100.0),
    TO_Q(43.14/100.0),
    TO_Q(38.48/100.0),
    TO_Q(33.82/100.0),
    TO_Q(28.67/100.0),
    TO_Q(23.53/100.0),
    TO_Q(17.88/100.0),
    TO_Q(12.23/100.0),
    TO_Q(6.11/100.0),
    TO_Q(0.00/100.0),
};
int32_t lc898212_msm_actuator_init_table(
	struct msm_actuator_ctrl_t *a_ctrl)
{
	int32_t rc = 0;

	LINFO("%s called\n", __func__);

	if (a_ctrl->func_tbl.actuator_set_params)
		a_ctrl->func_tbl.actuator_set_params(a_ctrl);

	a_ctrl->set_info.total_steps = LC898212_TOTAL_STEPS_NEAR_TO_FAR;

	
	if (a_ctrl->step_position_table != NULL) {
		kfree(a_ctrl->step_position_table);
		a_ctrl->step_position_table = NULL;
	}
	a_ctrl->step_position_table =
		kmalloc(sizeof(uint16_t) * (a_ctrl->set_info.total_steps + 1),
			GFP_KERNEL);
    
	if (a_ctrl->step_position_table) {
		uint16_t i = 0;
		int16_t infinity = DEFAULT_INFINITY;
		int16_t macro = DEFAULT_MACRO;
		int32_t full_range = 0;

		if (a_ctrl->af_OTP_info.VCM_OTP_Read) {
			infinity = a_ctrl->af_OTP_info.VCM_Infinity;
			macro = a_ctrl->af_OTP_info.VCM_Macro;
		}

		full_range = (int32_t)infinity + (int32_t)-macro;
		pr_info("%s infinity=%x macro=%x full_range=%x\n",__func__, infinity, macro, full_range);

		a_ctrl->step_position_table[0] = a_ctrl->initial_code = infinity;
		pr_info("%s initial_code=%x infinity=%x final=%x\n", __func__, a_ctrl->initial_code, infinity, a_ctrl->step_position_table[0]);

		for (i = 1; i <= a_ctrl->set_info.total_steps; i++) {
			a_ctrl->step_position_table[i] = BACK_Q(full_range*dof[i]) + macro;
			pr_info("%s af table[%d]=%x (%d)\n",__func__, i, a_ctrl->step_position_table[i], (int16_t)a_ctrl->step_position_table[i]);
		}
		
		a_ctrl->curr_step_pos = 0;
		a_ctrl->curr_region_index = 0;
	} else {
		pr_err("%s table init failed\n", __func__);
		rc = -EFAULT;
	}

	return rc;
}

int32_t lc898212_msm_actuator_move_focus(
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
		a_ctrl->step_position_table[dest_step_pos], &dir);
	if (rc < 0) {
		pr_err("%s focus move failed\n", __func__);
		return rc;
	} else {
		a_ctrl->curr_step_pos = dest_step_pos;
		LINFO("%s current step: %d\n", __func__, a_ctrl->curr_step_pos);
	}

	return rc;
}

int lc898212_actuator_af_power_down(void *params)
{
	int rc = 0;
	LINFO("%s called\n", __func__);

	rc = (int) msm_actuator_af_power_down(&lc898212_act_t);
	lc898212_poweroff_af();
	return rc;
}
static int32_t lc898212_wrapper_i2c_write(struct msm_actuator_ctrl_t *a_ctrl,
	int16_t next_lens_position, void *params)
{
    int32_t rc = 0;
    int dir = *(int*)params;

    rc = msm_camera_i2c_write(&a_ctrl->i2c_client,
            0xa1,
            next_lens_position,
            MSM_CAMERA_I2C_WORD_DATA);
    if (rc < 0) {
        pr_err("%s 0xa1 i2c write failed (%d)\n", __func__, rc);
        return rc;
    }

    rc = msm_camera_i2c_write(&a_ctrl->i2c_client,
         0x16,
         dir ? 0x180 : 0xfe80,
         MSM_CAMERA_I2C_WORD_DATA);
    if (rc < 0) {
        pr_err("%s 0x16 i2c write failed (%d)\n", __func__, rc);
        return rc;
    }

    rc = msm_camera_i2c_write(&a_ctrl->i2c_client,
         0x8a,
         5,
         MSM_CAMERA_I2C_BYTE_DATA);
    if (rc < 0) {
         pr_err("%s 0x8a i2c write failed (%d)\n", __func__, rc);
         return rc;
    }

	return rc;
}
int32_t lc898212_act_write_focus(
	struct msm_actuator_ctrl_t *a_ctrl,
	uint16_t curr_lens_pos,
	struct damping_params_t *damping_params,
	int8_t sign_direction,
	int16_t code_boundary)
{
	int32_t rc = 0;
	

	LINFO("%s called, curr lens pos = %d, code_boundary = %d\n",
		  __func__,
		  curr_lens_pos,
		  code_boundary);
#if 0
	if (sign_direction == 1)
		dac_value = (code_boundary - curr_lens_pos);
	else
		dac_value = (curr_lens_pos - code_boundary);

	LINFO("%s dac_value = %d\n",
		  __func__,
		  dac_value);

	rc = a_ctrl->func_tbl.actuator_i2c_write(a_ctrl, dac_value, NULL);
#endif
	return rc;
}


static struct msm_camera_i2c_reg_conf lc898212_settings_1[] = {
{0x80, 0x34},
{0x81, 0x20},
{0x84, 0xE0},
{0x87, 0x05},
{0xA4, 0x24},
{0x8B, 0x80},
{0x3A, 0x00},
{0x3B, 0x00},
{0x04, 0x00},
{0x05, 0x00},
{0x02, 0x00},
{0x03, 0x00},
{0x18, 0x00},
{0x19, 0x00},
{0x28, 0x80},
{0x29, 0x40},
{0x83, 0x2C},
{0x84, 0xE3},
{0x97, 0x00},
{0x98, 0x42},
{0x99, 0x00},
{0x9A, 0x00},
};

static struct msm_camera_i2c_reg_conf lc898212_settings_2_0x11[] = {
{0x88, 0x70},
{0x92, 0x00},
{0xA0, 0x01},
{0x7A, 0x68},
{0x7B, 0x00},
{0x7E, 0x78},
{0x7F, 0x00},
{0x7C, 0x03},
{0x7D, 0x00},
{0x93, 0x00},
{0x86, 0x60},

{0x40, 0x80},
{0x41, 0x10},
{0x42, 0x71},
{0x43, 0x50},
{0x44, 0x8F},
{0x45, 0x90},
{0x46, 0x61},
{0x47, 0xB0},
{0x48, 0x65},
{0x49, 0xB0},
{0x76, 0x0C},
{0x77, 0x50},
{0x4A, 0x28},
{0x4B, 0x70},
{0x50, 0x04},
{0x51, 0xF0},
{0x52, 0x76},
{0x53, 0x10},
{0x54, 0x16},
{0x55, 0xC0},
{0x56, 0x00},
{0x57, 0x00},
{0x58, 0x7F},
{0x59, 0xF0},
{0x4C, 0x40},
{0x4D, 0x30},
{0x78, 0x40},
{0x79, 0x00},
{0x4E, 0x7F},
{0x4F, 0xF0},
{0x6E, 0x00},
{0x6F, 0x00},
{0x72, 0x18},
{0x73, 0xE0},
{0x74, 0x4E},
{0x75, 0x30},
{0x30, 0x00},
{0x31, 0x00},
{0x5A, 0x06},
{0x5B, 0x80},
{0x5C, 0x72},
{0x5D, 0xF0},
{0x5E, 0x7F},
{0x5F, 0x70},
{0x60, 0x7E},
{0x61, 0xD0},
{0x62, 0x7F},
{0x63, 0xF0},
{0x64, 0x00},
{0x65, 0x00},
{0x66, 0x00},
{0x67, 0x00},
{0x68, 0x51},
{0x69, 0x30},
{0x6A, 0x72},
{0x6B, 0xF0},
{0x70, 0x00},
{0x71, 0x00},
{0x6C, 0x80},
{0x6D, 0x10},

{0x76, 0x0c},
{0x77, 0x50},
{0x78, 0x40},
{0x79, 0x00},
{0x30, 0x00},
{0x31, 0x00},
};

static struct msm_camera_i2c_reg_conf lc898212_settings_2_0x12[] = {
{0x88, 0x70},
{0x92, 0x00},
{0xA0, 0x01},
{0x7A, 0x68},
{0x7B, 0x00},
{0x7E, 0x78},
{0x7F, 0x00},
{0x7C, 0x03},
{0x7D, 0x00},
{0x93, 0x00},
{0x86, 0x60},

{0x40, 0x80},
{0x41, 0x10},
{0x42, 0x71},
{0x43, 0x50},
{0x44, 0x8F},
{0x45, 0x90},
{0x46, 0x61},
{0x47, 0xB0},
{0x48, 0x72},
{0x49, 0x10},
{0x76, 0x0C},
{0x77, 0x50},
{0x4A, 0x28},
{0x4B, 0x70},
{0x50, 0x04},
{0x51, 0xF0},
{0x52, 0x76},
{0x53, 0x10},
{0x54, 0x16},
{0x55, 0xC0},
{0x56, 0x00},
{0x57, 0x00},
{0x58, 0x7F},
{0x59, 0xF0},
{0x4C, 0x40},
{0x4D, 0x30},
{0x78, 0x60},
{0x79, 0x00},
{0x4E, 0x7F},
{0x4F, 0xF0},
{0x6E, 0x00},
{0x6F, 0x00},
{0x72, 0x18},
{0x73, 0xE0},
{0x74, 0x4E},
{0x75, 0x30},
{0x30, 0x00},
{0x31, 0x00},
{0x5A, 0x06},
{0x5B, 0x80},
{0x5C, 0x72},
{0x5D, 0xF0},
{0x5E, 0x7F},
{0x5F, 0x70},
{0x60, 0x7E},
{0x61, 0xD0},
{0x62, 0x7F},
{0x63, 0xF0},
{0x64, 0x00},
{0x65, 0x00},
{0x66, 0x00},
{0x67, 0x00},
{0x68, 0x51},
{0x69, 0x30},
{0x6A, 0x72},
{0x6B, 0xF0},
{0x70, 0x00},
{0x71, 0x00},
{0x6C, 0x80},
{0x6D, 0x10},

{0x76, 0x0c},
{0x77, 0x50},
{0x78, 0x40},
{0x79, 0x00},
{0x30, 0x00},
{0x31, 0x00},
};

static struct msm_camera_i2c_reg_conf lc898212_settings_2_0x13[] = {
{0x88, 0x70},
{0x92, 0x00},
{0xA0, 0x01},
{0x7A, 0x68},
{0x7B, 0x00},
{0x7E, 0x78},
{0x7F, 0x00},
{0x7C, 0x03},
{0x7D, 0x00},
{0x93, 0x00},
{0x86, 0x60},

{0x40, 0x80},
{0x41, 0x10},
{0x42, 0x74},
{0x43, 0x10},
{0x44, 0x8c},
{0x45, 0xd0},
{0x46, 0x67},
{0x47, 0x30},
{0x48, 0x47},
{0x49, 0xf0},
{0x76, 0x10},
{0x77, 0x50},
{0x4A, 0x40},
{0x4B, 0x30},
{0x50, 0x04},
{0x51, 0xF0},
{0x52, 0x76},
{0x53, 0x10},
{0x54, 0x28},
{0x55, 0x70},
{0x56, 0x00},
{0x57, 0x00},
{0x58, 0x7F},
{0x59, 0xF0},
{0x4C, 0x40},
{0x4D, 0x30},
{0x78, 0x40},
{0x79, 0x00},
{0x4E, 0x7F},
{0x4F, 0xF0},
{0x6E, 0x00},
{0x6F, 0x00},
{0x72, 0x15},
{0x73, 0x70},
{0x74, 0x55},
{0x75, 0x30},
{0x30, 0x00},
{0x31, 0x00},
{0x5A, 0x06},
{0x5B, 0x80},
{0x5C, 0x72},
{0x5D, 0xF0},
{0x5E, 0x7F},
{0x5F, 0x70},
{0x60, 0x7E},
{0x61, 0xD0},
{0x62, 0x7F},
{0x63, 0xF0},
{0x64, 0x00},
{0x65, 0x00},
{0x66, 0x00},
{0x67, 0x00},
{0x68, 0x51},
{0x69, 0x30},
{0x6A, 0x72},
{0x6B, 0xF0},
{0x70, 0x00},
{0x71, 0x00},
{0x6C, 0x80},
{0x6D, 0x10},

{0x76, 0x0c},
{0x77, 0x50},
{0x78, 0x40},
{0x79, 0x00},
{0x30, 0x00},
{0x31, 0x00},
};

static struct msm_camera_i2c_reg_conf lc898212_settings_3[] = {
{0x3A, 0x00},
{0x3B, 0x00}, 
{0x04, 0x00},
{0x05, 0x00}, 
{0x02, 0x00},
{0x03, 0x00}, 
{0x85, 0xC0}, 
};
static struct msm_camera_i2c_reg_conf lc898212_settings_4[] = {
{0x5A, 0x08},
{0x5B, 0x00},
{0x83, 0xac},
{0xA0, 0x01},
};

#include "lc898212_act.h"


stAdjPar	StAdjPar ;				

void RamReadA (struct msm_camera_i2c_client* i2c_client, uint8_t addr, uint16_t* data)
{
    msm_camera_i2c_read(i2c_client, addr, data, MSM_CAMERA_I2C_WORD_DATA);
}

void RamWriteA (struct msm_camera_i2c_client* i2c_client, uint8_t addr, uint16_t data)
{
    msm_camera_i2c_write(i2c_client, addr, data, MSM_CAMERA_I2C_WORD_DATA);
}

void RegReadA (struct msm_camera_i2c_client* i2c_client, uint8_t addr, uint8_t* data)
{
    uint16_t d;
    msm_camera_i2c_read(i2c_client, addr, &d, MSM_CAMERA_I2C_BYTE_DATA);
    *data = d;
}

void RegWriteA(struct msm_camera_i2c_client* i2c_client, uint8_t addr, uint8_t data)
{
    msm_camera_i2c_write(i2c_client, addr, data, MSM_CAMERA_I2C_BYTE_DATA);
}

void WitTim(int ms)
{
    usleep(ms*1000);
}


unsigned long TnePtpAf ( unsigned char	UcDirSel , unsigned char UcBfrAft ,struct msm_actuator_ctrl_t* a_ctrl)
{
	UnDwdVal			StTneVal;
	unsigned short		UsHigSmp[3], UsLowSmp[3];

	if(!UcDirSel){
		RamWriteA(&a_ctrl->i2c_client, PIDZO_211H, 0x8001 );
	}else{
		RamWriteA(&a_ctrl->i2c_client, PIDZO_211H, 0x7fff );
	}

	WitTim( TNE ) ;
	RamReadA(&a_ctrl->i2c_client,  ADOFFSET_211H ,	&UsHigSmp[0] ) ;
	WitTim(5);
	RamReadA(&a_ctrl->i2c_client,  ADOFFSET_211H ,	&UsHigSmp[1] ) ;
	WitTim(5);
	RamReadA(&a_ctrl->i2c_client,  ADOFFSET_211H ,	&UsHigSmp[2] ) ;
	WitTim(5);

	if( (signed short)UsHigSmp[0] < (signed short)0x8040){
		UsHigSmp[0] = 0x8040;
	}
	if( (signed short)UsHigSmp[1] < (signed short)0x8040){
		UsHigSmp[1] = 0x8040;
	}
	if( (signed short)UsHigSmp[2] < (signed short)0x8040){
		UsHigSmp[2] = 0x8040;
	}
	if(!UcDirSel){
		RamWriteA(&a_ctrl->i2c_client,  PIDZO_211H, 0x7fff );
	}else{
		RamWriteA(&a_ctrl->i2c_client,  PIDZO_211H, 0x8001 );
	}

	WitTim( TNE ) ;
	RamReadA(&a_ctrl->i2c_client,  ADOFFSET_211H ,	&UsLowSmp[0] ) ;
	WitTim(5);
	RamReadA(&a_ctrl->i2c_client,  ADOFFSET_211H ,	&UsLowSmp[1] ) ;
	WitTim(5);
	RamReadA(&a_ctrl->i2c_client,  ADOFFSET_211H ,	&UsLowSmp[2] ) ;
	WitTim(5);

	if( (signed short)UsLowSmp[0] < (signed short)0x8040){
		UsLowSmp[0] = 0x8040;
	}
	if( (signed short)UsLowSmp[1] < (signed short)0x8040){
		UsLowSmp[1] = 0x8040;
	}
	if( (signed short)UsLowSmp[2] < (signed short)0x8040){
		UsLowSmp[2] = 0x8040;
	}
	StTneVal.StDwdVal.UsHigVal	=((unsigned long)UsHigSmp[0]+(unsigned long)UsHigSmp[1]+(unsigned long)UsHigSmp[2])/3;
	StTneVal.StDwdVal.UsLowVal	=((unsigned long)UsLowSmp[0]+(unsigned long)UsLowSmp[1]+(unsigned long)UsLowSmp[2])/3;
	if( UcBfrAft == 0 ) {
			StAdjPar.StHalAdj.UsHlbCen	= ( ( signed short )StTneVal.StDwdVal.UsHigVal + ( signed short )StTneVal.StDwdVal.UsLowVal ) / 2 ;
			StAdjPar.StHalAdj.UsHlbMax	= StTneVal.StDwdVal.UsHigVal ;
			StAdjPar.StHalAdj.UsHlbMin	= StTneVal.StDwdVal.UsLowVal ;
	} else {
			StAdjPar.StHalAdj.UsHlaCen	= ( ( signed short )StTneVal.StDwdVal.UsHigVal + ( signed short )StTneVal.StDwdVal.UsLowVal ) / 2 ;
			StAdjPar.StHalAdj.UsHlaMax	= StTneVal.StDwdVal.UsHigVal ;
			StAdjPar.StHalAdj.UsHlaMin	= StTneVal.StDwdVal.UsLowVal ;
	}

	if( (signed short)(StTneVal.StDwdVal.UsHigVal) <= 0 ){
		StTneVal.StDwdVal.UsHigVal = (unsigned short)0x7fff - (signed short)StTneVal.StDwdVal.UsHigVal;
	}else{
		StTneVal.StDwdVal.UsHigVal	= (unsigned short)0x7fff - StTneVal.StDwdVal.UsHigVal;
	}

	if( (signed short)(StTneVal.StDwdVal.UsLowVal) >= 0 ){
		StTneVal.StDwdVal.UsLowVal = (unsigned short)0x7fff + (signed short)StTneVal.StDwdVal.UsLowVal;
	}else{
		StTneVal.StDwdVal.UsLowVal	= StTneVal.StDwdVal.UsLowVal - (unsigned short)0x7fff;
	}

	
	return( StTneVal.UlDwdVal ) ;
}



unsigned long	TneOffAf( UnDwdVal StTneVal, struct msm_actuator_ctrl_t* a_ctrl )
{
	signed long		SlSetOff ;
	unsigned short	UsSetOff ;

	RamReadA(&a_ctrl->i2c_client,  DAHLXO_211H , &UsSetOff ) ;															
	SlSetOff	= (signed long)(UsSetOff & 0xFF00) ;

	SlSetOff	+= ( (signed long)( StTneVal.StDwdVal.UsHigVal - StTneVal.StDwdVal.UsLowVal ) / ( 0xDE << 1 ) ) << 8 ;	
	if( SlSetOff > (signed long)0x0000FFFF ) {
		SlSetOff	= 0x0000FFFF ;
	} else if( SlSetOff < (signed long)0x00000000 ) {
		SlSetOff	= 0x00000000 ;
	}

	UsSetOff = (unsigned short)( ( SlSetOff & 0xFF00 ) | ( UsSetOff & 0x00FF ) );
	RamWriteA(&a_ctrl->i2c_client,  DAHLXO_211H , UsSetOff ) ;															

	StTneVal.UlDwdVal	= TnePtpAf( PTP_DIR , PTP_AFTER, a_ctrl ) ;

	return( StTneVal.UlDwdVal ) ;
}



unsigned long TneBiaAf( UnDwdVal StTneVal, struct msm_actuator_ctrl_t* a_ctrl )
{
	signed long		SlSetBia, UlSumAvr;
	unsigned short	UsSetBia ;

	UlSumAvr =	(unsigned long)( StTneVal.StDwdVal.UsHigVal + StTneVal.StDwdVal.UsLowVal ) / 2;

	RamReadA(&a_ctrl->i2c_client,  DAHLXO_211H ,	&UsSetBia );
	SlSetBia	= (signed long)UsSetBia & 0xFF;

	UsStpSiz	= UsStpSiz >> 1 ;																

	if ( UlSumAvr > BIAS_ADJ_BORDER ){
		if( ( SlSetBia + UsStpSiz ) < (signed short)0x0100 ){
			SlSetBia +=	UsStpSiz ;
		}
	} else {
		if( ( SlSetBia - UsStpSiz ) >= (signed short)0x0000 ){
			SlSetBia -=	UsStpSiz ;
		}
	}

	UsSetBia = (unsigned short)( ( SlSetBia & 0x00FF ) | ( UsSetBia & 0xFF00 ) ) ;
	RamWriteA(&a_ctrl->i2c_client,  DAHLXO_211H,	UsSetBia );

	StTneVal.UlDwdVal	= TnePtpAf( PTP_DIR, PTP_AFTER, a_ctrl);

	return( StTneVal.UlDwdVal );
}



unsigned char TneCenAf (struct msm_actuator_ctrl_t* a_ctrl)
{
	UnDwdVal		StTneVal;
	unsigned char 	UcTneRst, UcTofRst, UcTmeOut;
	unsigned short	UsOffDif;

	UsStpSiz	= 0x0100 ;
	UcTmeOut	= 1 ;
	UcTneRst	= FAILURE ;
	UcTofRst	= FAILURE ;

	StTneVal.UlDwdVal	= TnePtpAf( PTP_DIR, PTP_AFTER ,a_ctrl);
	StTneVal.UlDwdVal	= TnePtpAf( PTP_DIR, PTP_BEFORE,a_ctrl);

	while( UcTneRst && UcTmeOut )
	{
		if( UcTofRst == FAILURE ){
			StTneVal.UlDwdVal	= TneOffAf( StTneVal,a_ctrl ) ;
		}else{
			StTneVal.UlDwdVal	= TneBiaAf( StTneVal,a_ctrl ) ;
			UcTofRst	= FAILURE ;
		}

		UsOffDif	= abs( StTneVal.StDwdVal.UsHigVal - StTneVal.StDwdVal.UsLowVal ) / 2;					

		if( UsOffDif < MARGIN ){
			UcTofRst	= SUCCESS ;
		}else{
			UcTofRst	= FAILURE ;
		}

		if( ( StTneVal.StDwdVal.UsHigVal < HALL_MIN_GAP && StTneVal.StDwdVal.UsLowVal < HALL_MIN_GAP )		
		&&  ( StTneVal.StDwdVal.UsHigVal > HALL_MAX_GAP && StTneVal.StDwdVal.UsLowVal > HALL_MAX_GAP ) ){
			UcTneRst	= SUCCESS ;
			break ;
		}else if( UsStpSiz == 0 ){
			UcTneRst	= SUCCESS ;
			break ;
		}else{
			UcTneRst	= FAILURE ;
			UcTmeOut++ ;
		}

		if ( UcTmeOut == TIME_OUT ){
			UcTmeOut	= 0 ;
		}		 																							
	}

	if( UcTneRst == FAILURE ){
		UcTneRst	= EXE_HADJ ;
	}else{
		UcTneRst	= EXE_END ;
	}

	return( UcTneRst ) ;
}

unsigned char	TneRunAf( struct msm_actuator_ctrl_t* a_ctrl )
{
	unsigned char	UcHalSts =0;
	unsigned char	UcTmpEnb=0;
	unsigned short	UsTmpHxb=0;

	RamReadA (&a_ctrl->i2c_client,  DAHLXO_211H ,	&UsTmpHxb ) ;
	StAdjPar.StHalAdj.UsHlbOff	= UsTmpHxb >> 8 ;
	StAdjPar.StHalAdj.UsHlbBia	= UsTmpHxb & 0x00FF ;
	RamReadA (&a_ctrl->i2c_client,  gain1_211H, &StAdjPar.StLopGan.UsLgbVal ) ;
	RamReadA (&a_ctrl->i2c_client,  OFFSET_211H, &StAdjPar.StHalAdj.UsAdbOff ) ;


	RegReadA (&a_ctrl->i2c_client,  0x87 ,	&UcTmpEnb );
	RegWriteA (&a_ctrl->i2c_client,  0x87 ,	0x00 );

	UcHalSts	= TneCenAf(a_ctrl) ;

	RegWriteA (&a_ctrl->i2c_client,  0x87 ,	UcTmpEnb );

	RamReadA (&a_ctrl->i2c_client,  DAHLXO_211H ,	&UsTmpHxb ) ;
	StAdjPar.StHalAdj.UsHlaOff	= UsTmpHxb >> 8 ;
	StAdjPar.StHalAdj.UsHlaBia	= UsTmpHxb & 0x00FF ;
	RamReadA (&a_ctrl->i2c_client,  gain1_211H, &StAdjPar.StLopGan.UsLgaVal ) ;
	RamReadA(&a_ctrl->i2c_client,  OFFSET_211H, &StAdjPar.StHalAdj.UsAdaOff ) ;

	return UcHalSts;
}


int32_t lc898212_act_do_cal(struct msm_actuator_ctrl_t* a_ctrl, struct msm_actuator_get_vcm_cal_info_t* vcm_cal_info)
{
    int32_t rc = TneRunAf (a_ctrl);

    if (rc == EXE_END) {

        vcm_cal_info->bias = StAdjPar.StHalAdj.UsHlaBia;
        vcm_cal_info->offset = StAdjPar.StHalAdj.UsHlaOff;
        vcm_cal_info->hall_max = StAdjPar.StHalAdj.UsHlaMax;
        vcm_cal_info->hall_min = StAdjPar.StHalAdj.UsHlaMin;
        vcm_cal_info->rc = rc;
	    pr_info("%s: bias=0x%x offset=0x%x hall max/min=0x%x/0x%x\n", __func__,
	        vcm_cal_info->bias,
	        vcm_cal_info->offset,
	        vcm_cal_info->hall_max,
	        vcm_cal_info->hall_min);
        return 0;
    }
    pr_err("%s: lc898212_act_do_cal fail (%d)\n", __func__, rc);
    return -EFAULT;
}
static int32_t lc898212_act_init_focus(struct msm_actuator_ctrl_t *a_ctrl)
{
	int32_t rc = 0;
	uint16_t data=0;
	uint16_t step=0;
	uint8_t bias = DEFAULT_BIAS;
	uint8_t offset = DEFAULT_OFFSET;
	uint16_t infinity = DEFAULT_INFINITY;

	if (a_ctrl->af_OTP_info.VCM_OTP_Read) {
		bias = a_ctrl->af_OTP_info.VCM_Bias;
		offset = a_ctrl->af_OTP_info.VCM_Offset;
		infinity = a_ctrl->af_OTP_info.VCM_Infinity;
	}

    rc = msm_camera_i2c_write_tbl (&a_ctrl->i2c_client,lc898212_settings_1, ARRAY_SIZE(lc898212_settings_1), MSM_CAMERA_I2C_BYTE_DATA);
    if (rc < 0) {
	    pr_err("%s: lc898212_settings_1 i2c write failed (%d)\n", __func__, rc);
	    return rc;
    }
    mdelay(1);

    switch (a_ctrl->af_OTP_info.VCM_Vendor_Id_Version) {
        case 0x11:
            rc = msm_camera_i2c_write_tbl (&a_ctrl->i2c_client,lc898212_settings_2_0x11, ARRAY_SIZE(lc898212_settings_2_0x11), MSM_CAMERA_I2C_BYTE_DATA);
            break;
        case 0x12:
            rc = msm_camera_i2c_write_tbl (&a_ctrl->i2c_client,lc898212_settings_2_0x12, ARRAY_SIZE(lc898212_settings_2_0x12), MSM_CAMERA_I2C_BYTE_DATA);
            break;
        case 0x13:
            rc = msm_camera_i2c_write_tbl (&a_ctrl->i2c_client,lc898212_settings_2_0x13, ARRAY_SIZE(lc898212_settings_2_0x13), MSM_CAMERA_I2C_BYTE_DATA);
            break;
        
        default: 
            rc = msm_camera_i2c_write_tbl (&a_ctrl->i2c_client,lc898212_settings_2_0x13, ARRAY_SIZE(lc898212_settings_2_0x13), MSM_CAMERA_I2C_BYTE_DATA);
            break;
        break;
    }
    if (rc < 0) {
	    pr_err("%s: lc898212_settings_2 i2c write failed (%d)\n", __func__, rc);
	    return rc;
    }

    rc = msm_camera_i2c_write(&a_ctrl->i2c_client, 0x28, offset, MSM_CAMERA_I2C_BYTE_DATA); 
	if (rc < 0) {
	    pr_err("%s 0x28 i2c write failed (%d)\n", __func__, rc);
	    return rc;
    }
    rc = msm_camera_i2c_write(&a_ctrl->i2c_client, 0x29, bias, MSM_CAMERA_I2C_BYTE_DATA); 
    if (rc < 0) {
        pr_err("%s 0x29 i2c write failed (%d)\n", __func__, rc);
        return rc;
    }

    rc = msm_camera_i2c_write_tbl (&a_ctrl->i2c_client,lc898212_settings_3, ARRAY_SIZE(lc898212_settings_3), MSM_CAMERA_I2C_BYTE_DATA);
    if (rc < 0) {
	    pr_err("%s: lc898212_settings_3 i2c write failed (%d)\n", __func__, rc);
	    return rc;
    }
    mdelay(1);

    
    rc = msm_camera_i2c_read (&a_ctrl->i2c_client, 0x3c, &data, MSM_CAMERA_I2C_WORD_DATA);
    if (rc < 0) {
        pr_err("%s: i2c read failed (%d)\n", __func__, rc);
        return rc;
    }
    
    rc = msm_camera_i2c_write(&a_ctrl->i2c_client, 0x4, data, MSM_CAMERA_I2C_WORD_DATA);
    if (rc < 0) {
        pr_err("%s: 0x87 i2c write failed (%d)\n", __func__, rc);
        return rc;
    }

	rc = msm_camera_i2c_write(&a_ctrl->i2c_client, 0x87, 0x85, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
	    pr_err("%s: 0x87 i2c write failed (%d)\n", __func__, rc);
	    return rc;
    }

    rc = msm_camera_i2c_write_tbl (&a_ctrl->i2c_client,lc898212_settings_4, ARRAY_SIZE(lc898212_settings_4), MSM_CAMERA_I2C_BYTE_DATA);
    if (rc < 0) {
        pr_err("%s: lc898212_settings_4 i2c write failed (%d)\n", __func__, rc);
        return rc;
    }

    
    rc = msm_camera_i2c_read (&a_ctrl->i2c_client, 0x3c, &data, MSM_CAMERA_I2C_WORD_DATA);
    if (rc < 0) {
        pr_err("%s: i2c read failed (%d)\n", __func__, rc);
        return rc;
    }

    rc = msm_camera_i2c_write (&a_ctrl->i2c_client, 0x18, data, MSM_CAMERA_I2C_WORD_DATA);
    if (rc < 0) {
        pr_err("%s: 0x18 i2c read failed (%d)\n", __func__, rc);
        return rc;
    }

    rc = msm_camera_i2c_write(&a_ctrl->i2c_client, 0xa1, infinity, MSM_CAMERA_I2C_WORD_DATA);
	if (rc < 0) {
	    pr_err("%s 0xa1 i2c write failed (%d)\n", __func__, rc);
	    return rc;
    }

    step = (signed short)infinity > (signed short)data ? 0xfe80 : 0x180;

    rc = msm_camera_i2c_write(&a_ctrl->i2c_client, 0x16, step, MSM_CAMERA_I2C_WORD_DATA);
    if (rc < 0) {
        pr_err("%s 0x16 i2c write failed (%d)\n", __func__, rc);
        return rc;
    }

    rc = msm_camera_i2c_write(&a_ctrl->i2c_client, 0x8a, 5, MSM_CAMERA_I2C_BYTE_DATA);
    if (rc < 0) {
        pr_err("%s 0x8a i2c write failed (%d)\n", __func__, rc);
        return rc;
    }

    return rc;

}

int32_t lc898212_act_set_af_value(struct msm_actuator_ctrl_t *a_ctrl, af_value_t af_value)
{
	int16_t bottom_mech;
	int16_t top_mech;
	int16_t infinity;
	int16_t macro;

	if (af_value.VCM_BOTTOM_MECH_MSB == 0 && af_value.VCM_BOTTOM_MECH_LSB == 0 &&
		af_value.VCM_TOP_MECH_MSB == 0 && af_value.VCM_TOP_MECH_LSB == 0 &&
		af_value.VCM_VENDOR_ID_VERSION == 0) {
		pr_info("No value in OTP\n");
		return 0;
	}

	bottom_mech = (af_value.VCM_BOTTOM_MECH_MSB<<8) | af_value.VCM_BOTTOM_MECH_LSB;
	top_mech = (af_value.VCM_TOP_MECH_MSB<<8) | af_value.VCM_TOP_MECH_LSB;
	infinity = (af_value.AF_INF_MSB<<8) | af_value.AF_INF_LSB;
	macro = (af_value.AF_MACRO_MSB<<8) | af_value.AF_MACRO_LSB;
	if (bottom_mech < infinity || top_mech > macro) {
		pr_err("infinity/macro is out of mech bottom/top range (0x%x,0x%x,0x%x,0x%x)\n",
			infinity, macro, bottom_mech, top_mech);
		return 0;
	}

	a_ctrl->af_OTP_info.VCM_OTP_Read = true;
	a_ctrl->af_OTP_info.VCM_Infinity = infinity;
	a_ctrl->af_OTP_info.VCM_Macro = macro;
	a_ctrl->af_OTP_info.VCM_Bias = af_value.VCM_BIAS;
	a_ctrl->af_OTP_info.VCM_Offset = af_value.VCM_OFFSET;
	a_ctrl->af_OTP_info.VCM_Bottom_Mech = bottom_mech;
	a_ctrl->af_OTP_info.VCM_Top_Mech = top_mech;
	a_ctrl->af_OTP_info.VCM_Vendor_Id_Version = af_value.VCM_VENDOR_ID_VERSION;

	pr_info("%s: VCM_Infinity = 0x%x (%d)\n",       __func__, a_ctrl->af_OTP_info.VCM_Infinity,     (int16_t)a_ctrl->af_OTP_info.VCM_Infinity);
	pr_info("%s: VCM_Macro = 0x%x (%d)\n",          __func__, a_ctrl->af_OTP_info.VCM_Macro,        (int16_t)a_ctrl->af_OTP_info.VCM_Macro);
	pr_info("%s: VCM_Bias = 0x%x (%d)\n",           __func__, a_ctrl->af_OTP_info.VCM_Bias,         (int16_t)a_ctrl->af_OTP_info.VCM_Bias);
	pr_info("%s: VCM_Offset = 0x%x (%d)\n",         __func__, a_ctrl->af_OTP_info.VCM_Offset,       (int16_t)a_ctrl->af_OTP_info.VCM_Offset);
	pr_info("%s: VCM_Bottom_Mech = 0x%x (%d)\n",    __func__, a_ctrl->af_OTP_info.VCM_Bottom_Mech,  (int16_t)a_ctrl->af_OTP_info.VCM_Bottom_Mech);
	pr_info("%s: VCM_Top_Mech = 0x%x (%d)\n",       __func__, a_ctrl->af_OTP_info.VCM_Top_Mech,     (int16_t)a_ctrl->af_OTP_info.VCM_Top_Mech);
	pr_info("%s: VCM_Vendor_Id_Version = 0x%x (%d)\n", __func__, a_ctrl->af_OTP_info.VCM_Vendor_Id_Version, a_ctrl->af_OTP_info.VCM_Vendor_Id_Version);
	return 0;
}


static const struct i2c_device_id lc898212_act_i2c_id[] = {
	{"lc898212_act", (kernel_ulong_t)&lc898212_act_t},
	{ }
};

static int lc898212_act_config(
	void __user *argp)
{
	LINFO("%s called\n", __func__);
	return (int) msm_actuator_config(&lc898212_act_t,
		lc898212_msm_actuator_info, argp); 
}

static int lc898212_i2c_add_driver_table(
	void)
{
	int32_t rc = 0;

	LINFO("%s called\n", __func__);

	rc = lc898212_poweron_af();
	if (rc < 0) {
		pr_err("%s power on failed\n", __func__);
		return (int) rc;
	}
	return (int) rc;
}

static struct i2c_driver lc898212_act_i2c_driver = {
	.id_table = lc898212_act_i2c_id,
	.probe  = msm_actuator_i2c_probe,
	.remove = __exit_p(lc898212_act_i2c_remove),
	.driver = {
		.name = "lc898212_act",
	},
};

static int __init lc898212_i2c_add_driver(
	void)
{
	LINFO("%s called\n", __func__);
	return i2c_add_driver(lc898212_act_t.i2c_driver);
}

static struct v4l2_subdev_core_ops lc898212_act_subdev_core_ops;

static struct v4l2_subdev_ops lc898212_act_subdev_ops = {
	.core = &lc898212_act_subdev_core_ops,
};

static int32_t lc898212_act_create_subdevice(
	void *act_info,
	void *sdev)
{
	LINFO("%s called\n", __func__);

	lc898212_msm_actuator_info = (struct msm_actuator_info *)act_info;

	return (int) msm_actuator_create_subdevice(&lc898212_act_t,
		lc898212_msm_actuator_info->board_info,
		(struct v4l2_subdev *)sdev);
}

static struct msm_actuator_ctrl_t lc898212_act_t = {
	.i2c_driver = &lc898212_act_i2c_driver,
	.i2c_addr = REG_VCM_I2C_ADDR,
	.act_v4l2_subdev_ops = &lc898212_act_subdev_ops,
	.actuator_ext_ctrl = {
		.a_init_table = lc898212_i2c_add_driver_table,
		.a_power_down = lc898212_actuator_af_power_down,
		.a_create_subdevice = lc898212_act_create_subdevice,
		.a_config = lc898212_act_config,
		.is_cal_supported = 1, 
	},

	.i2c_client = {
		.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	},

	.set_info = {
		.total_steps = LC898212_TOTAL_STEPS_NEAR_TO_FAR, 
		.gross_steps = 3,	
		.fine_steps = 1,	
	},

	.curr_step_pos = 0,
	.curr_region_index = 0,
	.initial_code = 0,	
	.actuator_mutex = &lc898212_act_mutex,
	.af_algo = AF_ALGO_QCT,

	.func_tbl = {
		.actuator_init_table = lc898212_msm_actuator_init_table,
		.actuator_move_focus = lc898212_msm_actuator_move_focus,
		.actuator_write_focus = lc898212_act_write_focus,
		.actuator_set_default_focus = msm_actuator_set_default_focus,
		.actuator_init_focus = lc898212_act_init_focus,
		.actuator_i2c_write = lc898212_wrapper_i2c_write,
		.actuator_set_af_value = lc898212_act_set_af_value,
		.actuator_do_cal = lc898212_act_do_cal, 
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

subsys_initcall(lc898212_i2c_add_driver);
MODULE_DESCRIPTION("LC898212 actuator");
MODULE_LICENSE("GPL v2");
