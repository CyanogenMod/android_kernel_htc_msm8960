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
#include "rumbas_ois.h"
#include <mach/gpio.h>
#include <linux/firmware.h>
#include "msm_sensor.h"


#define TRUE  1
#define FALSE 0

#define	RUMBAS_TOTAL_STEPS_NEAR_TO_FAR			100
#define	RUMBAS_TOTAL_STEPS_NEAR_TO_FAR_RAWCHIP_AF			256

#define REG_VCM_NEW_CODE			0x30F2
#define REG_VCM_I2C_ADDR			0x32
#define REG_VCM_CODE_MSB			0x04
#define REG_VCM_CODE_LSB			0x05
#define REG_VCM_THRES_MSB			0x06
#define REG_VCM_THRES_LSB			0x07
#define REG_VCM_RING_CTRL			0x400

#define DIV_CEIL(x, y) (x/y + (x%y) ? 1 : 0)

#define VCM_STEP_ONE_TO_ONE_MAPPING	1
#define VCM_CALIBRATED	1
#define GOLDEN_FAR_END			50
#define GOLDEN_NEAR_END		250

#define kernel_step_table_size	31

static af_value_t g_af_value;
static int copy_otp_once = FALSE;

#if 0
int kernel_step_table[kernel_step_table_size]
	= {0, 10, 21, 32, 43, 53, 64, 67, 71, 74,
	78, 81, 85, 89, 93, 107, 121, 135, 150, 152,
	155, 158, 162, 165, 168, 173, 178, 181, 185};

int kernel_step_table[kernel_step_table_size]
	= {0, 10, 21, 32, 43, 53, 64, 67, 71, 74,
	78, 81, 85, 88, 92, 106, 121, 135, 149, 163,
	178, 192, 206, 220, 234, 248, 263, 277, 291};
#else
int kernel_step_table[kernel_step_table_size]
	= {17, 25, 34, 42, 51, 64, 78, 91, 105, 118,
	 132, 145, 159, 176, 194, 211, 229, 246, 264, 281,
	 299, 319, 340, 360, 381, 401, 422, 442, 463, 483, 504};
#endif


DEFINE_MUTEX(rumbas_act_mutex);
static struct msm_actuator_ctrl_t rumbas_act_t;

void rumbas_move_lens_position_by_stroke(int16_t stroke);

static struct region_params_t g_regions[] = {
	
	{
		.step_bound = {RUMBAS_TOTAL_STEPS_NEAR_TO_FAR, 0},
		.code_per_step = 2,
	},
};

static uint16_t g_scenario[] = {
	
	RUMBAS_TOTAL_STEPS_NEAR_TO_FAR,
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

static struct msm_actuator_info *rumbas_msm_actuator_info;

void rumbas_set_internal_clk(void);
void rumbas_disable_OIS(void);
void rumbas_move_lens_position(int16_t next_lens_position);
int32_t rumbas_poweron_af(void);
void rumbas_poweroff_af(void);
int get_fixed_lens_position(void);

void rumbas_do_cam_vcm_on_cb(void)
{
	int rc = 0;
	int fixed_lens_position;
	pr_info("[CAM_VCM_CB]  %s called\n", __func__);

	rumbas_msm_actuator_info->vcm_wa_vreg_on();
	mdelay(5);

	
	rc = rumbas_poweron_af();
	if (rc < 0) {
		pr_err("%s rumbas power on failed\n", __func__);
		rumbas_msm_actuator_info->vcm_wa_vreg_off();
		return;
	}
	mdelay(50);

	rumbas_set_internal_clk();
	rumbas_disable_OIS();
	fixed_lens_position = get_fixed_lens_position();

#if 1
#if 1
	pr_info("[CAM_VCM_CB]  cam_vcm_on_cb()	 call rumbas_move_lens_position(%d)\n", 100);
	rumbas_move_lens_position(100);
	mdelay(1);
	pr_info("[CAM_VCM_CB]  cam_vcm_on_cb()	 call rumbas_move_lens_position(%d)\n", 200);
	rumbas_move_lens_position(200);
	mdelay(1);
#else
	pr_info("[CAM_VCM_CB]  cam_vcm_on_cb()	 call rumbas_move_lens_position(%d)\n", fixed_lens_position);
	rumbas_move_lens_position(fixed_lens_position);
	mdelay(1);
#endif
	pr_info("[CAM_VCM_CB]  cam_vcm_on_cb()	 call rumbas_move_lens_position()	DONE\n");
#endif

}
void rumbas_do_cam_vcm_off_cb(void)
{
#if 1
	pr_info("[CAM_VCM_CB]  cam_vcm_on_cb()	 call rumbas_move_lens_position(%d)\n", 50);
	rumbas_move_lens_position(50);
	mdelay(1);
	pr_info("[CAM_VCM_CB]  cam_vcm_on_cb()	 call rumbas_move_lens_position(%d)\n", 20);
	rumbas_move_lens_position(20);
	mdelay(40);
	pr_info("[CAM_VCM_CB]  cam_vcm_on_cb()	 call rumbas_move_lens_position(%d)\n", 10);
	rumbas_move_lens_position(10);
	mdelay(10);
	pr_info("[CAM_VCM_CB]  cam_vcm_on_cb()	 call rumbas_move_lens_position(%d)\n", 5);
	rumbas_move_lens_position(5);
	mdelay(1);

	pr_info("[CAM_VCM_CB]  do_cam_vcm_off_work()   call rumbas_move_lens_position(0)\n");
	rumbas_move_lens_position(0);
	mdelay(10);
	pr_info("[CAM_VCM_CB]  do_cam_vcm_off_work()   call rumbas_move_lens_position()   DONE\n");

	
	rumbas_move_lens_position_by_stroke(45);
	mdelay(20);
	rumbas_move_lens_position_by_stroke(40);
	mdelay(20);
	rumbas_move_lens_position_by_stroke(30);
	mdelay(20);
	rumbas_move_lens_position_by_stroke(5);
	mdelay(10);
#endif
	rumbas_poweroff_af();

	rumbas_msm_actuator_info->vcm_wa_vreg_off();
}

int32_t load_cmd_prevalue(int cmd_id, uint8_t *byte_data)
{
	int32_t rc = 0;
	uint8_t byte_read[9] = {0,0,0,0,0,0,0,0,0};

	memset(byte_read, 0, sizeof(byte_read));
	rc = msm_camera_i2c_read_seq_rumbas(&(rumbas_act_t.i2c_client), (cmd_id|0x80), &byte_read[0], 9);
	if (rc < 0) {
		pr_err("%s 0x1F reading error\n", __func__);
		return rc;
	}

	*(byte_data) = byte_read[1];
	*(byte_data+1) = byte_read[2];
	*(byte_data+2) = byte_read[3];
	*(byte_data+3) = byte_read[4];
	*(byte_data+4) = byte_read[5];
	*(byte_data+5) = byte_read[6];
	*(byte_data+6) = byte_read[7];
	*(byte_data+7) = byte_read[8];

	return rc;
}

static uint8_t rumbas_select_ois_map_table(table_type op)
{
	int32_t rc = 0;
	const struct firmware *fw = NULL;
	unsigned char *ois_data;
	switch(op) {
		case USER_TABLE:
			ois_off_tbl = 1;
			tbl_threshold_ptr = &tbl_threshold[0];
			mapping_tbl_ptr = mapping_tbl;
			break;
		case MFG_TABLE:
			ois_off_tbl = 0;
			tbl_threshold_ptr = &mfg_tbl_threshold[0];
			mapping_tbl_ptr = mfg_mapping_tbl;
			break;
		default:
			tbl_threshold_ptr = &tbl_threshold[0];
			mapping_tbl_ptr = mapping_tbl;
			break;
	}
	rc = request_firmware(&fw, "ois_param", &(rumbas_act_t.i2c_client.client->dev));
	if (rc!=0) {
		pr_info("[OIS] request_firmware: Debug file doesn't exist, use default table\n");
	} else {
		ois_data = (unsigned char *)fw->data;
		sscanf(ois_data, "Line:%d %d Gyro:%d %d %d BotRow:%d %d %d %d %d %d MedRow:%d %d %d %d %d %d TopRow:%d %d %d %d %d %d",tbl_threshold_ptr,tbl_threshold_ptr+1,tbl_threshold_ptr+2,tbl_threshold_ptr+3,tbl_threshold_ptr+4,&mapping_tbl_ptr[0][0],&mapping_tbl_ptr[0][1],&mapping_tbl_ptr[1][0],&mapping_tbl_ptr[1][1],&mapping_tbl_ptr[2][0],&mapping_tbl_ptr[2][1],&mapping_tbl_ptr[3][0],&mapping_tbl_ptr[3][1],&mapping_tbl_ptr[4][0],&mapping_tbl_ptr[4][1],&mapping_tbl_ptr[5][0],&mapping_tbl_ptr[5][1],&mapping_tbl_ptr[6][0],&mapping_tbl_ptr[6][1],&mapping_tbl_ptr[7][0],&mapping_tbl_ptr[7][1],&mapping_tbl_ptr[8][0],&mapping_tbl_ptr[8][1]);
		release_firmware(fw);
	}
	if((tbl_threshold_ptr != NULL) && (mapping_tbl_ptr != NULL)) {
		memcpy(rumbas_act_t.get_ois_tbl.tbl_thre,tbl_threshold_ptr,sizeof(rumbas_act_t.get_ois_tbl.tbl_thre));
		memcpy(rumbas_act_t.get_ois_tbl.tbl_info,mapping_tbl_ptr,sizeof(rumbas_act_t.get_ois_tbl.tbl_info));
	}
	return rc;

}

#if 0
static uint8_t rumbas_level_angle_map_table(void *level_data, void *angle_data)
{
	int32_t rc = 0;
	uint8_t ois_enable = 1;
	uint32_t line = 0;
	uint32_t gyro_energy_x = 0, gyro_energy_y = 0, sqrt_gyro = 0;
	uint8_t byte_read[9] = {0,0,0,0,0,0,0,0,0};
	uint8_t row_flag = 1, col_flag = 0;

	if(!tbl_threshold_ptr || !mapping_tbl_ptr)
		return 0;
	
	line = ois_line;
	if(line > *tbl_threshold_ptr)
		row_flag = 0;
	else if(line< *(tbl_threshold_ptr+1))
		row_flag = 2;

	
	rc = msm_camera_i2c_read_seq_rumbas(&(rumbas_act_t.i2c_client), (0x1E|0x80), &byte_read[0], 9);
	if( rc < 0 )
	{
		pr_err("%s i2c read failed(%d)\n", __func__, rc);
	} else {
		gyro_energy_x = byte_read[1] << 24 | byte_read[2] << 16 | byte_read[3] << 8 | byte_read[4];
		gyro_energy_y = byte_read[5] << 24 | byte_read[6] << 16 | byte_read[7] << 8 | byte_read[8];
	}

	sqrt_gyro = (int_sqrt((gyro_energy_x >> 7) * (gyro_energy_x >> 7) + (gyro_energy_y >> 7) * (gyro_energy_y >> 7))) << 7;
	if((sqrt_gyro >= *(tbl_threshold_ptr+2) && sqrt_gyro < *(tbl_threshold_ptr+3)))
		col_flag = 1;
	else if((sqrt_gyro >= *(tbl_threshold_ptr+3) && sqrt_gyro <= *(tbl_threshold_ptr+4)))
		col_flag = 2;

	if(ois_off_tbl && sqrt_gyro > *(tbl_threshold_ptr+4))
		ois_enable  = 0;

	*((uint8_t *)level_data) = mapping_tbl_ptr[row_flag + 3*col_flag][0];
	*((uint16_t *)angle_data) = mapping_tbl_ptr[row_flag + 3*col_flag][1];
	rumbas_act_t.get_ois_info.gyro_info = sqrt_gyro;
	rumbas_act_t.get_ois_info.ois_index = ois_enable * (row_flag + 3*col_flag + 1);
	return ois_enable;
}
#endif

static int32_t actuator_ois_setting_i2c_write(void)
{
	int32_t rc = 0;
	uint8_t byte_data[8];

	
			load_cmd_prevalue(0x1F, &byte_data[0]);
			byte_data[0] = 0x10; 
			rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x1F, &byte_data[0], 8);
			if (rc < 0) {
			pr_err("%s set 0x1F failed\n", __func__);
			}

	
			memset(byte_data, 0, sizeof(byte_data));
 			byte_data[0] = 0x00;
			rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x30, &byte_data[0], 8);
			if (rc < 0) {
				pr_err("%s set 0x30 failed\n", __func__);
			}

	
			memset(byte_data, 0, sizeof(byte_data));
			load_cmd_prevalue(0x1C, &byte_data[0]);
			((uint16_t *)byte_data)[0] = ENDIAN(3200);
			((uint16_t *)byte_data)[1] = ENDIAN(128);
			((uint16_t *)byte_data)[2] = ENDIAN(128);
			rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x1C, &byte_data[0], 8);
			if (rc < 0) {
				pr_err("[OIS] %s setting i2c write failed (%d)\n", __func__, rc);
			}
	return rc;
}


#define ILLEGAL_CMD_INPUT_VALUE 10000

static int16_t g_fixed_rx_angle = ILLEGAL_CMD_INPUT_VALUE;
static int16_t g_fixed_ry_angle = ILLEGAL_CMD_INPUT_VALUE;
static int16_t g_OIS_level = ILLEGAL_CMD_INPUT_VALUE;
static uint16_t g_AutoOnOffThresholds_parm1 = ILLEGAL_CMD_INPUT_VALUE;
static uint16_t g_AutoOnOffThresholds_parm2 = ILLEGAL_CMD_INPUT_VALUE;
static int16_t g_LFCE = ILLEGAL_CMD_INPUT_VALUE;
static int16_t g_fast_reset_mode= ILLEGAL_CMD_INPUT_VALUE;
static uint16_t g_reset_speed = ILLEGAL_CMD_INPUT_VALUE;
static uint16_t g_calibration_coefficient = ILLEGAL_CMD_INPUT_VALUE;

int get_fixed_lens_position(void);
int16_t get_fixed_rx_angle(void);
int16_t get_fixed_ry_angle(void);
int16_t get_OIS_level(void);
uint16_t get_AutoOnOffThresholds_parm1(void);
uint16_t get_AutoOnOffThresholds_parm2(void);
int16_t get_LFCE(void);
int16_t get_fast_reset_mode(void);
uint16_t get_reset_speed(void);
uint16_t get_calibration_coefficient(void);


static int32_t check_if_enable_OIS_MFG_debug(void)
{
#ifdef FORCE_DISABLE_OIS_DEBUG
	return 0;
#endif
#ifdef FORCE_ENABLE_OIS_DEBUG
	return 1;
#endif

	if(board_mfg_mode())
		return 1;
	else
		return 0;
}

static int32_t process_OIS_MFG_debug(void)
{
	int32_t rc = 0;
	uint8_t byte_data[8] = {0,0,0,0,0,0,0,0};
	int16_t *x_angle_ptr;
	int16_t *y_angle_ptr;
	uint8_t x_angle[2];
	uint8_t y_angle[2];

#ifndef LESS_LOG_FOR_OIS_DEBUG
	int startup_mode;
	
	uint8_t auto_OIS_mode;
	uint8_t fast_reset_mode;
	
	uint8_t panning_mode;
#endif
	
	int16_t *x_gyro_raw_ptr;
	int16_t *y_gyro_raw_ptr;
	int16_t *temperature_ptr;
	uint8_t x_gyro_raw[2];
	uint8_t y_gyro_raw[2];
	uint8_t temperature[2];
	int32_t temperature_degree;
	
	int32_t *x_gyro_corrected_ptr;
	int32_t *y_gyro_corrected_ptr;
	uint8_t x_gyro_corrected[4];
	uint8_t y_gyro_corrected[4];
	
	int16_t *x_gyro_biases_ptr;
	int16_t *y_gyro_biases_ptr;
	uint8_t x_gyro_biases[2];
	uint8_t y_gyro_biases[2];
	
	uint32_t *x_cumulative_speed_ptr;
	uint32_t *y_cumulative_speed_ptr;
	uint8_t x_cumulative_speed[4];
	uint8_t y_cumulative_speed[4];
	
	uint32_t *x_cumulative_acceleration_ptr;
	uint32_t *y_cumulative_acceleration_ptr;
	uint8_t x_cumulative_acceleration[4];
	uint8_t y_cumulative_acceleration[4];

	
	
	if(get_calibration_coefficient() < ILLEGAL_CMD_INPUT_VALUE)
	{
		load_cmd_prevalue(0x1F, &byte_data[0]);
		pr_info("[RUMBA_S]  write 0x1F ,  calibration_coefficient = %d\n", get_calibration_coefficient());
		byte_data[2] = get_calibration_coefficient();
		rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x1F, &byte_data[0], 8);
		if (rc < 0) {
			pr_err("%s set calibration_coefficient failed\n", __func__);
		}
	}

	
	if(get_reset_speed() < ILLEGAL_CMD_INPUT_VALUE)
	{
		load_cmd_prevalue(0x1C, &byte_data[0]);
		pr_info("[RUMBA_S]  write 0x1C ,  reset_speed = %d\n", get_reset_speed());
		((uint16_t *)byte_data)[0] = ENDIAN(get_reset_speed());
		rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x1C, &byte_data[0], 8);
		if (rc < 0) {
			pr_err("%s set reset_speed failed\n", __func__);
		}
	}

	
	if(get_fast_reset_mode() < ILLEGAL_CMD_INPUT_VALUE)
	{
		load_cmd_prevalue(0x10, &byte_data[0]);
		pr_info("[RUMBA_S]  write 0x10 ,  fast_reset_mode = %d\n", get_fast_reset_mode());
		byte_data[6] = (byte_data[6] & 0xFD) |(get_fast_reset_mode() << 1);
		rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x10, &byte_data[0], 8);
		if (rc < 0) {
			pr_err("%s set fast_reset_mode failed\n", __func__);
		}
	}

	
	if(get_LFCE() < ILLEGAL_CMD_INPUT_VALUE)
	{
		load_cmd_prevalue(0x10, &byte_data[0]);
		pr_info("[RUMBA_S]  write 0x10 ,  LFCE = %d\n", get_LFCE());
		byte_data[4] = (byte_data[4] & 0xF7) |(get_LFCE() << 3);
		rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x10, &byte_data[0], 8);
		if (rc < 0) {
			pr_err("%s set LFCE failed\n", __func__);
		}
	}

	
	if(get_OIS_level() < ILLEGAL_CMD_INPUT_VALUE)
	{
		load_cmd_prevalue(0x10, &byte_data[0]);
		pr_info("[RUMBA_S]  write 0x10 ,  OIS_level = %d\n", get_OIS_level());
		byte_data[4] = (byte_data[4] & 0xF8) |get_OIS_level();
		rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x10, &byte_data[0], 8);
		if (rc < 0) {
			pr_err("%s set ois mode failed\n", __func__);
		}
	}

	
	if ( (get_AutoOnOffThresholds_parm1() < ILLEGAL_CMD_INPUT_VALUE) && (get_AutoOnOffThresholds_parm2() < ILLEGAL_CMD_INPUT_VALUE) )
	{
		load_cmd_prevalue(0x1B, &byte_data[0]);
		pr_info("[RUMBA_S]  write 0x1B ,  AutoOnOffThresholds_parm1 = %d\n", get_AutoOnOffThresholds_parm1());
		pr_info("[RUMBA_S]  write 0x1B ,  AutoOnOffThresholds_parm2 = %d\n", get_AutoOnOffThresholds_parm2());
		((uint16_t *)byte_data)[0] = ENDIAN(get_AutoOnOffThresholds_parm1());
		((uint16_t *)byte_data)[1] = ENDIAN(get_AutoOnOffThresholds_parm2());
		rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x1B, &byte_data[0], 8);
		if (rc < 0) {
			pr_err("%s set ois mode failed\n", __func__);
		}
	}

	
	if( (get_fixed_rx_angle() < ILLEGAL_CMD_INPUT_VALUE) && (get_fixed_ry_angle() < ILLEGAL_CMD_INPUT_VALUE) )
	{
		load_cmd_prevalue(0x12, &byte_data[0]);

		x_angle[0] = byte_data[2];
		x_angle[1] = byte_data[1];
		y_angle[0] = byte_data[4];
		y_angle[1] = byte_data[3];
		x_angle_ptr = (int16_t *)(&x_angle[0]);
		y_angle_ptr = (int16_t *)(&y_angle[0]);
		pr_info("[RUMBA_S]  OLD Compensation Angles   x_angle=%d , y_angle=%d\n",*x_angle_ptr, *y_angle_ptr);
		*x_angle_ptr = get_fixed_rx_angle();
		*y_angle_ptr = get_fixed_ry_angle();
		pr_info("[RUMBA_S]  NEW Compensation Angles   x_angle=%d , y_angle=%d\n",*x_angle_ptr, *y_angle_ptr);
		byte_data[2] = x_angle[0];
		byte_data[1] = x_angle[1];
		byte_data[4] = y_angle[0];
		byte_data[3] = y_angle[1];
		pr_info("[RUMBA_S]  0x12 set [Angle]\n");
		rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x12, &byte_data[0], 8);
		if (rc < 0) {
			pr_err("[OIS] %s MAX compensation angle i2c write failed (%d)\n", __func__, rc);
		}
	}


	
	
	load_cmd_prevalue(0x12, &byte_data[0]);
	x_angle[0] = byte_data[1];
	x_angle[1] = byte_data[0];
	y_angle[0] = byte_data[3];
	y_angle[1] = byte_data[2];
	x_angle_ptr = (int16_t *)(&x_angle[0]);
	y_angle_ptr = (int16_t *)(&y_angle[0]);
	pr_info("[RUMBA_S]  Compensation Angles   x_angle=%05d , y_angle=%05d\n",*x_angle_ptr, *y_angle_ptr);


#ifndef LESS_LOG_FOR_OIS_DEBUG
	
	load_cmd_prevalue(0x1B, &byte_data[0]);
	auto_OIS_mode = (byte_data[4] & 0x04) >> 2;
	if (auto_OIS_mode == 1)
		pr_info("[RUMBA_S]  OIS is Auto Off\n");
	else
		pr_info("[RUMBA_S]  OIS is Auto On\n");

	fast_reset_mode = (byte_data[4] & 0x02) >> 1;
	pr_info("[RUMBA_S]  [0x1B] Read fast reset mode status :  fast_reset_mode=0x%02x\n", fast_reset_mode);


	load_cmd_prevalue(0x10, &byte_data[0]);
	startup_mode = (byte_data[7] & 0x01);
	pr_info("[RUMBA_S]  [0x10] Read startup mode :  startup_mode=0x%02x\n", startup_mode);


	
	load_cmd_prevalue(0x1F, &byte_data[0]);
	panning_mode = (byte_data[1] & 0x03);
	if (panning_mode == 0) {
		if (auto_OIS_mode == 1)
			pr_info("[RUMBA_S]  NOT in any panning mode  : X Still\n");
		else
			pr_info("[RUMBA_S]  NOT in any panning mode  : X Off\n");
	}
	else if (panning_mode == 1)
		pr_info("[RUMBA_S]  Rx : in the Real panning mode : X Real\n");
	else if (panning_mode == 2)
		pr_info("[RUMBA_S]  Rx : in the Fake panning mode : X Fake\n");

	panning_mode = (byte_data[1] & 0x0C) >> 2;
	if (panning_mode == 0) {
		if (auto_OIS_mode == 1)
			pr_info("[RUMBA_S]  NOT in any panning mode  :  Y Still\n");
		else
			pr_info("[RUMBA_S]  NOT in any panning mode  :  Y Off\n");
	}
	else if (panning_mode == 1)
		pr_info("[RUMBA_S]  Ry : in the Real panning mode : Y Real\n");
	else if (panning_mode == 2)
		pr_info("[RUMBA_S]  Ry : in the Fake panning mode : Y Fake\n");
#endif


	
	load_cmd_prevalue(0x16, &byte_data[0]);
	x_gyro_raw[0] = byte_data[1];
	x_gyro_raw[1] = byte_data[0];
	y_gyro_raw[0] = byte_data[3];
	y_gyro_raw[1] = byte_data[2];
	temperature[0] = byte_data[7];
	temperature[1] = byte_data[6];
	x_gyro_raw_ptr = (int16_t *)(&x_gyro_raw[0]);
	y_gyro_raw_ptr = (int16_t *)(&y_gyro_raw[0]);
	temperature_ptr = (int16_t *)(&temperature[0]);
	temperature_degree= *temperature_ptr;
	temperature_degree = 21000 + ((*temperature_ptr) * 3);
	pr_info("[RUMBA_S]  x_gyro_raw =       %08d , y_gyro_raw =       %08d , temperature= %d.%03d (%d)\n",
		*x_gyro_raw_ptr, *y_gyro_raw_ptr, (temperature_degree/1000), (temperature_degree%1000) , *temperature_ptr);


	
	load_cmd_prevalue(0x34, &byte_data[0]);
	x_gyro_corrected[0] = byte_data[3];
	x_gyro_corrected[1] = byte_data[2];
	x_gyro_corrected[2] = byte_data[1];
	x_gyro_corrected[3] = byte_data[0];
	y_gyro_corrected[0] = byte_data[7];
	y_gyro_corrected[1] = byte_data[6];
	y_gyro_corrected[2] = byte_data[5];
	y_gyro_corrected[3] = byte_data[4];
	x_gyro_corrected_ptr = (int32_t *)(&x_gyro_corrected[0]);
	y_gyro_corrected_ptr = (int32_t *)(&y_gyro_corrected[0]);
	pr_info("[RUMBA_S]  x_gyro_corrected = %08d , y_gyro_corrected = %08d\n", *x_gyro_corrected_ptr, *y_gyro_corrected_ptr);


	
	load_cmd_prevalue(0x35, &byte_data[0]);
	x_gyro_biases[0] = byte_data[1];
	x_gyro_biases[1] = byte_data[0];
	y_gyro_biases[0] = byte_data[3];
	y_gyro_biases[1] = byte_data[2];
	x_gyro_biases_ptr = (int16_t *)(&x_gyro_biases[0]);
	y_gyro_biases_ptr = (int16_t *)(&y_gyro_biases[0]);
	pr_info("[RUMBA_S]  x_gyro_biases =    %08d , y_gyro_biases =    %08d\n",*x_gyro_biases_ptr, *y_gyro_biases_ptr);


	
	load_cmd_prevalue(0x1D, &byte_data[0]);
	x_cumulative_speed[0] = byte_data[3];
	x_cumulative_speed[1] = byte_data[2];
	x_cumulative_speed[2] = byte_data[1];
	x_cumulative_speed[3] = byte_data[0];
	y_cumulative_speed[0] = byte_data[7];
	y_cumulative_speed[1] = byte_data[6];
	y_cumulative_speed[2] = byte_data[5];
	y_cumulative_speed[3] = byte_data[4];
	x_cumulative_speed_ptr = (int32_t *)(&x_cumulative_speed[0]);
	y_cumulative_speed_ptr = (int32_t *)(&y_cumulative_speed[0]);
	pr_info("[RUMBA_S]  x_cumulative_speed = %08d , y_cumulative_speed = %08d\n",
		*x_cumulative_speed_ptr / 128, *y_cumulative_speed_ptr / 128);


	
	load_cmd_prevalue(0x1E, &byte_data[0]);
	x_cumulative_acceleration[0] = byte_data[3];
	x_cumulative_acceleration[1] = byte_data[2];
	x_cumulative_acceleration[2] = byte_data[1];
	x_cumulative_acceleration[3] = byte_data[0];
	y_cumulative_acceleration[0] = byte_data[7];
	y_cumulative_acceleration[1] = byte_data[6];
	y_cumulative_acceleration[2] = byte_data[5];
	y_cumulative_acceleration[3] = byte_data[4];
	x_cumulative_acceleration_ptr = (int32_t *)(&x_cumulative_acceleration[0]);
	y_cumulative_acceleration_ptr = (int32_t *)(&y_cumulative_acceleration[0]);
	pr_info("[RUMBA_S]  x_cumulative_acceleration = %08d , y_cumulative_acceleration= %08d\n",
		*x_cumulative_acceleration_ptr / 128, *y_cumulative_acceleration_ptr/128);

	return rc;
}


static int32_t actuator_mappingTbl_i2c_write(int startup_mode, struct sensor_actuator_info_t * sensor_actuator_info)
{
	int32_t rc = 0;
	uint8_t byte_data[8];
	static int ois_nonmap_setting = 1;
#if 0
	uint16_t max_com_angel = DEFAULT_MAX_ANGLE_COM;
	uint16_t ois_level = DEFAULT_OIS_LEVEL;
	uint8_t ois_enable = 0;
	static uint16_t pre_max_angel = 0;
	static uint16_t pre_ois_level = 0;
#endif
	camera_video_mode_type cur_cam_mode;
	uint32_t cur_line_cnt = 0;
	uint32_t cur_exp_time = 0;
	int32_t cur_zoom_level = 0;
	uint16_t cur_cmp_angle = 500;
	uint16_t cur_ois_level = 5;
	uint16_t cur_threshold_1 = 0;
	uint16_t cur_threshold_2 = 0;
	uint16_t ois_off = 0;
	uint8_t AutoOISLevel = 0;
	uint32_t level_threshold_2 = 0;
	uint32_t level_threshold_3 = 0;
	uint32_t level_threshold_4 = 0;
	uint32_t level_threshold_5 = 0;
	uint32_t level_threshold_6 = 7000;   
	uint32_t level_threshold_7 = 1000000;
	uint32_t level_threshold_8 = 1000000;

	if (startup_mode == 1) {
		
		load_cmd_prevalue(0x10, &byte_data[0]);

		byte_data[4] = (byte_data[4] & 0xF8) | 0x05;
		pr_info("[RUMBA_S]  set OIS level : byte_data[4]=0x%x\n", byte_data[4]);

		byte_data[7] = byte_data[7] & 0xFE;
		pr_info("[RUMBA_S]  set Startup Mode : byte_data[7]=0x%x\n", byte_data[7]);

		rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x10, &byte_data[0], 8);
		if (rc < 0) {
			pr_err("[OIS] %s set Startup Mode, i2c write failed (%d)\n", __func__, rc);
		}
		

		if(!board_mfg_mode()) {
			
			load_cmd_prevalue(0x1A, &byte_data[0]);
			((uint16_t *)byte_data)[0] = ENDIAN(500);
			pr_info("[RUMBA_S]  set MAX compensation angle : 500\n");
			rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x1A, &byte_data[0], 8);
			if (rc < 0) {
				pr_err("[RUMBA_S] %s MAX compensation angle i2c write failed (%d)\n", __func__, rc);
			}
		}
	}


	if (check_if_enable_OIS_MFG_debug())
		process_OIS_MFG_debug();
	else
		pr_info("[RUMBA_S]  Force disable OIS debug log\n");


	if(ois_nonmap_setting) {
		actuator_ois_setting_i2c_write();
		ois_nonmap_setting = 0;
	}

	cur_cam_mode = sensor_actuator_info->cam_mode;
	cur_line_cnt = sensor_actuator_info->cur_line_cnt;
	cur_exp_time = sensor_actuator_info->cur_exp_time;
	cur_zoom_level = sensor_actuator_info->zoom_level;

	if (sensor_actuator_info->fast_reset_mode) {
		pr_info("[RUMBA_S] fast_reset_mode, ois off\n");
		ois_off = 1;

	} else if (cur_cam_mode == CAM_MODE_CAMERA_PREVIEW) {
		AutoOISLevel = 0;
		cur_cmp_angle = 600;
		if (cur_exp_time >= (1000/24)) {
			cur_ois_level = 6;
			cur_threshold_1 = 0;
			cur_threshold_2 = 0;
			cur_cmp_angle = 600;
		} else if (cur_exp_time >= (1000/48)) {
			cur_ois_level = 5;
			cur_threshold_1 = 5;
			cur_threshold_2 = 4;
			cur_cmp_angle = 400;
		} else if (cur_exp_time >= (1000/83)) {
			cur_ois_level = 4;
			cur_threshold_1 = 5;
			cur_threshold_2 = 4;
			cur_cmp_angle = 400;
		} else {
			ois_off = 1;
		}

		if (cur_zoom_level >= 45) {

			if (cur_exp_time >= (1000/24))
				cur_ois_level = 6;
			else
				cur_ois_level = 5;
			cur_cmp_angle = 1000;
			cur_threshold_1 = 0;
			cur_threshold_2 = 0;
			ois_off = 0;

		} else if (cur_zoom_level >= 30) {

			if (cur_exp_time >= (1000/24))
				cur_ois_level = 6;
			else
				cur_ois_level = 5;
			cur_cmp_angle = 750;
			cur_threshold_1 = 0;
			cur_threshold_2 = 0;
			ois_off = 0;

		} else if (cur_zoom_level >= 15) {

			if (cur_exp_time >= (1000/24))
				cur_ois_level = 6;
			else
				cur_ois_level = 5;
			cur_cmp_angle = 600;
			cur_threshold_1 = 0;
			cur_threshold_2 = 0;
			ois_off = 0;
		}

	} else if (cur_cam_mode == CAM_MODE_VIDEO_RECORDING) {
		AutoOISLevel = 1;
		if (cur_exp_time >= (1000/24)) {
			cur_ois_level= 6;
			level_threshold_6 = 0;
			level_threshold_7 = 7000;
		} else if (cur_exp_time >= (1000/48)) {
			cur_ois_level= 6;
		} else if (cur_exp_time >= (1000/83)) {
			cur_ois_level= 5;
		} else {
			cur_ois_level= 5;
		}

		cur_cmp_angle = 500;
		cur_threshold_1 = 0;
		cur_threshold_2 = 0;

		if (cur_zoom_level >= 45) {
			cur_ois_level = 6;
			cur_cmp_angle = 1000;
		} else if (cur_zoom_level >= 30) {
			cur_ois_level = 6;
		}

		if (board_mfg_mode()) {
			pr_info("[RUMBA_S] mfg mode\n");
			cur_ois_level= 5;
			cur_cmp_angle = 1000;
		}
	} else {
		cur_ois_level= 4;
		cur_cmp_angle = 1000;
		cur_threshold_1 = 0;
		cur_threshold_2 = 0;
	}

	pr_info("[RUMBA_S] ois_off=%d, cur_cam_mode=%d,  cur_line_cnt=%d, cur_exp_time=%d, cur_ois_level=%d, cur_cmp_angle=%d, TH1_=%d, TH_2=%d, cur_zoom_level=%d, AutoOISLevel=%d\n",
		ois_off,  cur_cam_mode, cur_line_cnt, cur_exp_time, cur_ois_level,
		cur_cmp_angle, cur_threshold_1, cur_threshold_2, cur_zoom_level, AutoOISLevel);

	if (ois_off) {
		
		load_cmd_prevalue(0x10, &byte_data[0]);
		byte_data[0] = 0x02; 
		byte_data[6] = (byte_data[6] & 0xFD) |(0x01 << 1); 
		rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x10, &byte_data[0], 8);
		if (rc < 0) {
			pr_err("[RUMBA_S] %s  OIS mode and level  i2c write failed (%d)\n", __func__, rc);
		}
	} else {
		
		load_cmd_prevalue(0x10, &byte_data[0]);
		byte_data[0] = 0x02; 
		byte_data[4] = (byte_data[4] & 0xF8) | cur_ois_level;
		byte_data[6] = (byte_data[6] & 0xFD);
		rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x10, &byte_data[0], 8);
		if (rc < 0) {
			pr_err("[RUMBA_S] %s  OIS mode and level  i2c write failed (%d)\n", __func__, rc);
		}

		
		load_cmd_prevalue(0x1A, &byte_data[0]);
		((uint16_t *)byte_data)[0] = ENDIAN(cur_cmp_angle);
		rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x1A, &byte_data[0], 8);
		if (rc < 0) {
			pr_err("[RUMBA_S] %s  MAX compensation angle  i2c write failed (%d)\n", __func__, rc);
		}

		
		load_cmd_prevalue(0x1B, &byte_data[0]);
		((uint16_t *)byte_data)[0] = ENDIAN(cur_threshold_1);
		((uint16_t *)byte_data)[1] = ENDIAN(cur_threshold_2);
		rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x1B, &byte_data[0], 8);
		if (rc < 0) {
			pr_err("[RUMBA_S] %s  OIS Auto On/Off Threshold  i2c write failed (%d)\n", __func__, rc);
		}

		load_cmd_prevalue(0x30, &byte_data[0]);
		byte_data[0] = (byte_data[0]& 0xfe) | AutoOISLevel;
		if (AutoOISLevel){
			((uint32_t *)byte_data)[1] = ENDIAN32(level_threshold_2);
			rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x30, &byte_data[0], 8);
			if (rc < 0) {
				pr_err("[RUMBA_S] %s Auto OIS Level Threshold2  i2c write failed (%d)\n", __func__, rc);
				return rc;
			}
			load_cmd_prevalue(0x31, &byte_data[0]);
			((uint32_t *)byte_data)[0] = ENDIAN32(level_threshold_3);
			((uint32_t *)byte_data)[1] = ENDIAN32(level_threshold_4);
			rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x31, &byte_data[0], 8);
			if (rc < 0) {
				pr_err("[RUMBA_S] %s Auto OIS Level Threshold34  i2c write failed (%d)\n", __func__, rc);
				return rc;
			}
			load_cmd_prevalue(0x32, &byte_data[0]);
			((uint32_t *)byte_data)[0] = ENDIAN32(level_threshold_5);
			((uint32_t *)byte_data)[1] = ENDIAN32(level_threshold_6);
			rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x32, &byte_data[0], 8);
			if (rc < 0) {
				pr_err("[RUMBA_S] %s Auto OIS Level Threshold56  i2c write failed (%d)\n", __func__, rc);
				return rc;
			}
			load_cmd_prevalue(0x33, &byte_data[0]);
			((uint32_t *)byte_data)[0] = ENDIAN32(level_threshold_7);
			((uint32_t *)byte_data)[1] = ENDIAN32(level_threshold_8);
			rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x33, &byte_data[0], 8);
			if (rc < 0) {
				pr_err("[RUMBA_S] %s Auto OIS Level Threshold78  i2c write failed (%d)\n", __func__, rc);
				return rc;
			}
			AutoOISLevel = 0;
		}else{
			rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x30, &byte_data[0], 8);
			if (rc < 0) {
				pr_err("[RUMBA_S] %s Auto OIS Level Setting i2c write failed (%d)\n", __func__, rc);
				return rc;
			}
		}
	}


#if 0
	ois_enable = rumbas_level_angle_map_table(&ois_level, &max_com_angel);

	if(pre_max_angel == max_com_angel && pre_ois_level == ois_level )
		return rc;
	pre_max_angel = max_com_angel;
	pre_ois_level = ois_level;

	load_cmd_prevalue(0x1B, &byte_data[0]);
	
	if(!ois_enable)
		((uint16_t *)byte_data)[0] = 0xffff;
	rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x1B, &byte_data[0], 8);
	if (rc < 0) {
		pr_err("%s set OIS Auto On/Off Thresholds failed\n", __func__);
	}

	if(!ois_enable)
		return rc;

	
	memset(byte_data, 0, sizeof(byte_data));
	((uint16_t *)byte_data)[0] = ENDIAN(max_com_angel);
	rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x1A, &byte_data[0], 8);
	if (rc < 0) {
		pr_err("[OIS] %s MAX compensation angle i2c write failed (%d)\n", __func__, rc);
	}

	
	memset(byte_data, 0, sizeof(byte_data));
	load_cmd_prevalue(0x10, &byte_data[0]);
	byte_data[0] = 0x02; 
	ois_level_value = byte_data[4] = ois_level+8; 
	pan_mode_enable = byte_data[5] = 0x01; 
	tri_moode_enable = byte_data[6] = 0x00; 
	rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x10, &byte_data[0], 8);
	if (rc < 0) {
		pr_err("%s set ois mode failed\n", __func__);
	}
#endif

	return rc;
}


int32_t rumbas_poweron_af(void)
{
	int32_t rc = 0;

	
	if (rumbas_msm_actuator_info == NULL)
		return -1;

	pr_info("%s enable AF actuator, gpio = %d\n", __func__,
			rumbas_msm_actuator_info->vcm_pwd);
	mdelay(1);
	rc = gpio_request(rumbas_msm_actuator_info->vcm_pwd, "rumbas");
	if (!rc)
		gpio_direction_output(rumbas_msm_actuator_info->vcm_pwd, 1);
	else
		pr_err("%s: AF PowerON gpio_request failed %d\n", __func__, rc);
	gpio_free(rumbas_msm_actuator_info->vcm_pwd);
	mdelay(1);
	return rc;
}

void rumbas_poweroff_af(void)
{
	int32_t rc = 0;

	
	if (rumbas_msm_actuator_info == NULL)
		return;

	pr_info("%s disable AF actuator, gpio = %d\n", __func__,
			rumbas_msm_actuator_info->vcm_pwd);

	mdelay(1);
	rc = gpio_request(rumbas_msm_actuator_info->vcm_pwd, "rumbas");
	if (!rc)
		gpio_direction_output(rumbas_msm_actuator_info->vcm_pwd, 0);
	else
		pr_err("%s: AF PowerOFF gpio_request failed %d\n", __func__, rc);
	gpio_free(rumbas_msm_actuator_info->vcm_pwd);
	mdelay(1);
}

int32_t rumbas_msm_actuator_init_table(
	struct msm_actuator_ctrl_t *a_ctrl)
{
	int32_t rc = 0;

	LINFO("%s called\n", __func__);

	if (a_ctrl->func_tbl.actuator_set_params)
		a_ctrl->func_tbl.actuator_set_params(a_ctrl);

#if 0
	if (rumbas_act_t.step_position_table) {
		LINFO("%s table inited\n", __func__);
		return rc;
	}
#endif

	if (rumbas_msm_actuator_info->use_rawchip_af && a_ctrl->af_algo == AF_ALGO_RAWCHIP)
		a_ctrl->set_info.total_steps = RUMBAS_TOTAL_STEPS_NEAR_TO_FAR_RAWCHIP_AF;
	else
		a_ctrl->set_info.total_steps = RUMBAS_TOTAL_STEPS_NEAR_TO_FAR;

	
	a_ctrl->ois_mfgtest_in_progress = a_ctrl->set_info.ois_mfgtest_in_progress_reload;
	if (a_ctrl->ois_mfgtest_in_progress) pr_info("[MFGOIS] %s: mfg ois testing...", __FUNCTION__);
	

	
	if (a_ctrl->step_position_table != NULL) {
		kfree(a_ctrl->step_position_table);
		a_ctrl->step_position_table = NULL;
	}
	a_ctrl->step_position_table =
		kmalloc(sizeof(uint16_t) * (a_ctrl->set_info.total_steps + 1),
			GFP_KERNEL);

	if (a_ctrl->step_position_table != NULL) {
		uint16_t i = 0;
		uint16_t rumbas_nl_region_boundary1 = 2;
		uint16_t rumbas_nl_region_code_per_step1 = 32;
		uint16_t rumbas_l_region_code_per_step = 16;
		uint16_t rumbas_max_value = 1023;

		a_ctrl->step_position_table[0] = a_ctrl->initial_code;
		for (i = 1; i <= a_ctrl->set_info.total_steps; i++) {
			if (rumbas_msm_actuator_info->use_rawchip_af && a_ctrl->af_algo == AF_ALGO_RAWCHIP) 
				a_ctrl->step_position_table[i] =
					a_ctrl->step_position_table[i-1] + 4;
			else
			{
			if (i <= rumbas_nl_region_boundary1) {
				a_ctrl->step_position_table[i] =
					a_ctrl->step_position_table[i-1]
					+ rumbas_nl_region_code_per_step1;
			} else {
				a_ctrl->step_position_table[i] =
					a_ctrl->step_position_table[i-1]
					+ rumbas_l_region_code_per_step;
			}

			if (a_ctrl->step_position_table[i] > rumbas_max_value)
				a_ctrl->step_position_table[i] = rumbas_max_value;
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

#if VCM_CALIBRATED
int32_t VCM_Transform(int near_end, int far_end, int position)
{
	return (position - GOLDEN_FAR_END) * (near_end - far_end) / (GOLDEN_NEAR_END - GOLDEN_FAR_END) + far_end;
}
#endif

int32_t rumbas_msm_actuator_move_focus(
	struct msm_actuator_ctrl_t *a_ctrl,
	int dir,
	int32_t num_steps)
{
	int32_t rc = 0;
	int8_t sign_dir = 0;
	int16_t dest_step_pos = 0;
	#if VCM_STEP_ONE_TO_ONE_MAPPING
	int16_t dest_step_pos_VCM = 0;    
	#endif

	LINFO("%s called, dir %d, num_steps %d\n",
		__func__,
		dir,
		num_steps);

	
	if (a_ctrl->set_info.total_steps == 0)
		a_ctrl->set_info.total_steps = RUMBAS_TOTAL_STEPS_NEAR_TO_FAR;

	
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

#if VCM_STEP_ONE_TO_ONE_MAPPING   
	if (!a_ctrl->ois_mfgtest_in_progress) {
		if (dest_step_pos == a_ctrl->curr_step_pos)
			return rc;
		
		if (dest_step_pos < 0)
			dest_step_pos = 0;
		else if (dest_step_pos >= kernel_step_table_size)
			dest_step_pos = (kernel_step_table_size - 1);

		dest_step_pos_VCM = kernel_step_table[dest_step_pos];
		if (dest_step_pos_VCM < 0)
			dest_step_pos_VCM = 0;
		else if (dest_step_pos_VCM > 1023)
			dest_step_pos_VCM = 1023;

		rc = a_ctrl->func_tbl.actuator_i2c_write(a_ctrl, dest_step_pos_VCM, NULL);
	} else {
		if (dest_step_pos < 0)
			dest_step_pos = 0;
		else if (dest_step_pos > a_ctrl->set_info.total_steps)
			dest_step_pos = a_ctrl->set_info.total_steps;

		if (dest_step_pos == a_ctrl->curr_step_pos)
			return rc;

		rc = a_ctrl->func_tbl.actuator_i2c_write(a_ctrl,
			a_ctrl->step_position_table[dest_step_pos], NULL);
	}

#else
	if (dest_step_pos < 0)
		dest_step_pos = 0;
	else if (dest_step_pos > a_ctrl->set_info.total_steps)
		dest_step_pos = a_ctrl->set_info.total_steps;

	if (dest_step_pos == a_ctrl->curr_step_pos)
		return rc;

	rc = a_ctrl->func_tbl.actuator_i2c_write(a_ctrl,
		a_ctrl->step_position_table[dest_step_pos], NULL);
#endif

	if (rc < 0) {
		pr_err("%s focus move failed\n", __func__);
		return rc;
	} else {
		a_ctrl->curr_step_pos = dest_step_pos;
		LINFO("%s current step: %d\n", __func__, a_ctrl->curr_step_pos);
	}

	return rc;
}

int rumbas_actuator_af_power_down(void *params)
{
	int rc = 0;
	LINFO("%s called\n", __func__);

	rc = (int) msm_actuator_af_power_down(&rumbas_act_t);

	
	rumbas_move_lens_position_by_stroke(45);
	mdelay(15);
	rumbas_move_lens_position_by_stroke(40);
	mdelay(15);
	rumbas_move_lens_position_by_stroke(30);
	mdelay(15);
	rumbas_move_lens_position_by_stroke(5);
	mdelay(5);

	rumbas_poweroff_af();
	return rc;
}

static int32_t rumbas_wrapper_i2c_write(struct msm_actuator_ctrl_t *a_ctrl,
	int16_t next_lens_position, void *params)
{
	int32_t rc = 0;
	uint8_t byte_data[8] = {0,0,0,0,0,0,0,0};
	int32_t positon;

#if 0
	
	positon = 100 + (next_lens_position * 300 /1024);
#else
	
	
	
	#if VCM_CALIBRATED
	int32_t	sharp_position_at_Golden_sample = 0;

	if (a_ctrl->af_OTP_info.VCM_OTP_Read && !a_ctrl->ois_mfgtest_in_progress)
	{
		sharp_position_at_Golden_sample = 50 + (next_lens_position * (450-50) /1024);
		positon = VCM_Transform(a_ctrl->af_OTP_info.VCM_Macro, a_ctrl->af_OTP_info.VCM_Infinity, sharp_position_at_Golden_sample);
		
	}
	else
		positon = 50 + (next_lens_position * (450-50) /1024);
		
		
	#else
	positon = 50 + (next_lens_position * (450-50) /1024);
	#endif
#endif


	if (check_if_enable_OIS_MFG_debug())
	{
		if (get_fixed_lens_position() < ILLEGAL_CMD_INPUT_VALUE)
			positon = get_fixed_lens_position();
		pr_info("[RUMBA_S]  VCM lens position = %d\n", positon);
	}

	if(a_ctrl->enable_focus_step_log)
		pr_info("%s next_lens_position: %d, positon: %d, VCM_Macro: %d, VCM_Infinity: %d\n", __func__,
			next_lens_position, positon, a_ctrl->af_OTP_info.VCM_Macro, a_ctrl->af_OTP_info.VCM_Infinity);

	memset(byte_data, 0, sizeof(byte_data));
	byte_data[0] = (positon & 0xFF00) >> 8;
	byte_data[1] = positon & 0x00FF;
	rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client),
		0x15, &byte_data[0], 8);
	if (rc < 0) {
		pr_err("%s VCM position  i2c write failed (%d)\n", __func__, rc);
		return rc;
	}

	return rc;
}

int32_t rumbas_act_write_focus(
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

static int32_t rumbas_act_init_focus(struct msm_actuator_ctrl_t *a_ctrl)
{
	int32_t rc = 0, rc_rumbas = 0;
	uint8_t byte_read[9];
	uint8_t byte_data[8];

	rc = a_ctrl->func_tbl.actuator_i2c_write(a_ctrl, a_ctrl->initial_code,
		NULL);
	if (rc < 0)
		pr_err("%s i2c write failed\n", __func__);
	else
		a_ctrl->curr_step_pos = 0;

	a_ctrl->ois_mfgtest_in_progress = 0; 

	if(rumbas_act_t.actuator_ext_ctrl.is_ois_supported) {
		memset(byte_read, 0, sizeof(byte_read));
		rc_rumbas = msm_camera_i2c_read_seq_rumbas(&(rumbas_act_t.i2c_client), (0x11|0x80), &byte_read[0], 9);
		if (rc_rumbas < 0)
			pr_err("%s read fw version error\n", __func__);
		else {
			pr_info("[RUMBA_S] firmware version : 0x%02x%02x%02x%02x\n", byte_read[1], byte_read[2], byte_read[3], byte_read[4]);
			pr_info("[RUMBA_S] auto tuner version : 0x%02x%02x , VCM grade : 0x%02x\n", byte_read[5], byte_read[6], byte_read[7]);
			if(byte_read[2] >= OIS_READY_VERSION)
				rumbas_act_t.ois_ready_version = 1;
		}
	}

	
	if (rumbas_act_t.ois_ready_version) {
		if (a_ctrl->af_OTP_info.act_id == 0x11 || a_ctrl->af_OTP_info.act_id == 0x31) { 
			memset(byte_data, 0, sizeof(byte_data));
			load_cmd_prevalue(0x39, &byte_data[0]);
			((uint16_t *)byte_data)[0] = ENDIAN(19);
			((uint16_t *)byte_data)[1] = ENDIAN(150);
			((uint16_t *)byte_data)[2] = ENDIAN(100);
			rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x39, &byte_data[0], 8);
			if (rc < 0) {
				pr_err("[OIS] %s 0x39 cmd   i2c write failed (%d)\n", __func__, rc);
			}
		}

		memset(byte_data, 0, sizeof(byte_data));
		load_cmd_prevalue(0x27, &byte_data[0]);
		if (a_ctrl->af_OTP_info.act_id == 0x11 || a_ctrl->af_OTP_info.act_id == 0x31) 
			((int16_t *)byte_data)[0] = ENDIAN(8);
		else 
			((int16_t *)byte_data)[0] = ENDIAN(-10);
		rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client),
			0x27, &byte_data[0], 8);
		if (rc < 0) {
			pr_err("%s 0x27 cmd  i2c write failed (%d)\n", __func__, rc);
		}

		if(!board_mfg_mode()) {
			load_cmd_prevalue(0x10, &byte_data[0]);
			byte_data[6] = (byte_data[6] & 0xFD) |(0x01 << 1); 
			rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x10, &byte_data[0], 8);
			if (rc < 0) {
				pr_err("[RUMBA_S] %s  set fast reset mode , i2c write failed (%d)\n", __func__, rc);
			}
		}
	}
	

	return rc;
}

int32_t rumbas_act_set_ois_mode(struct msm_actuator_ctrl_t *a_ctrl, int ois_mode)
{
	int32_t rc = 0;
	uint8_t byte_data[8];
	uint8_t byte_data_2[8];

	pr_info("%s called , ois_mode:%d\n", __func__, ois_mode);
	load_cmd_prevalue(0x10, &byte_data[0]);
	if (ois_mode == 0) {
		byte_data[0] = 0x01; 
	} else {
		byte_data[0] = 0x02; 

#if 0
		byte_data[4] = ois_level_value;
		byte_data[5] = pan_mode_enable;
		byte_data[6] = tri_moode_enable; 
#endif

		if(ois_mode == 1) {
			rumbas_select_ois_map_table(MFG_TABLE);
			
			load_cmd_prevalue(0x1B, &byte_data_2[0]);
			((uint16_t *)byte_data_2)[0] = ENDIAN(0);
			((uint16_t *)byte_data_2)[1] = ENDIAN(0);
			rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x1B, &byte_data_2[0], 8);
			if (rc < 0) {
				pr_err("[RUMBA_S] %s  OIS Auto On/Off Threshold  i2c write failed (%d)\n", __func__, rc);
			}
		} else
			rumbas_select_ois_map_table(USER_TABLE);
	}
	rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client),
		0x10, &byte_data[0], 8);
	if (rc < 0) {
		pr_err("%s set ois mode failed\n", __func__);
		return rc;
	}

	return rc;
}

int32_t rumbas_act_update_ois_tbl(struct msm_actuator_ctrl_t *a_ctrl, struct sensor_actuator_info_t * sensor_actuator_info)
{
	int32_t rc = 0;
	pr_info("[RUMBA_S]  %s  startup_mode=%d\n", __func__, sensor_actuator_info->startup_mode);

	if(rumbas_act_t.ois_ready_version) {
		rc = actuator_mappingTbl_i2c_write(sensor_actuator_info->startup_mode, sensor_actuator_info);
	}
	return rc;
}

int32_t rumbas_act_set_af_value(struct msm_actuator_ctrl_t *a_ctrl, af_value_t af_value)
{
	int32_t rc = 0;
	uint8_t OTP_data[8] = {0,0,0,0,0,0,0,0};
	int16_t VCM_Infinity = 0;
	int32_t otp_deviation = 85;

	if (copy_otp_once == FALSE) {
		memcpy(&g_af_value, &af_value, sizeof(g_af_value));
		copy_otp_once = TRUE;
	}

	OTP_data[0] = af_value.VCM_START_MSB;
	OTP_data[1] = af_value.VCM_START_LSB;
	OTP_data[2] = af_value.AF_INF_MSB;
	OTP_data[3] = af_value.AF_INF_LSB;
	OTP_data[4] = af_value.AF_MACRO_MSB;
	OTP_data[5] = af_value.AF_MACRO_LSB;
	

	
	if(rumbas_msm_actuator_info->otp_diviation)
		otp_deviation = rumbas_msm_actuator_info->otp_diviation;

	if (OTP_data[2] || OTP_data[3] || OTP_data[4] || OTP_data[5]) {
		a_ctrl->af_OTP_info.VCM_OTP_Read = true;
		a_ctrl->af_OTP_info.VCM_Start = 0;
		VCM_Infinity = (int16_t)(OTP_data[2]<<8 | OTP_data[3]) - otp_deviation;
		if (VCM_Infinity < 50){
			a_ctrl->af_OTP_info.VCM_Infinity = 50;
		}else{
			a_ctrl->af_OTP_info.VCM_Infinity = VCM_Infinity;
		}
		a_ctrl->af_OTP_info.VCM_Macro = (OTP_data[4]<<8 | OTP_data[5]);
	}
	a_ctrl->af_OTP_info.act_id = af_value.ACT_ID;
	pr_info("OTP_data[2] %d OTP_data[3] %d OTP_data[4] %d OTP_data[5] %d\n",
		OTP_data[2], OTP_data[3], OTP_data[4], OTP_data[5]);
	pr_info("VCM_Start = %d\n", a_ctrl->af_OTP_info.VCM_Start);
	pr_info("VCM_Infinity = %d\n", a_ctrl->af_OTP_info.VCM_Infinity);
	pr_info("VCM_Macro = %d\n", a_ctrl->af_OTP_info.VCM_Macro);
	pr_info("ACT_ID = 0x%x\n", a_ctrl->af_OTP_info.act_id);
	return rc;
}



void rumbas_move_lens_position(int16_t next_lens_position);
static struct msm_actuator_get_ois_cal_info_t cal_info;

int32_t  rumbas_act_read_cal_data(void)
{
	int32_t rc = 0;
	uint8_t byte_data[8];

	int16_t *x_offset_ptr;
	int16_t *y_offset_ptr;
	int16_t *temperature_ptr;
	uint8_t x_offset[2];
	uint8_t y_offset[2];
	uint8_t temperature[2];
	int32_t temperature_degree;
	int8_t x_slope;
	int8_t y_slope;

       
       load_cmd_prevalue(0x19, &byte_data[0]);
	x_offset[0] = byte_data[1];
	x_offset[1] = byte_data[0];
	y_offset[0] = byte_data[3];
	y_offset[1] = byte_data[2];
	x_slope = (int8_t)byte_data[4];
	y_slope = (int8_t)byte_data[5];
	temperature[0] = byte_data[7];
	temperature[1] = byte_data[6];
	x_offset_ptr = (int16_t *)(&x_offset[0]);
	y_offset_ptr = (int16_t *)(&y_offset[0]);
	temperature_ptr = (int16_t *)(&temperature[0]);

	cal_info.x_offset = *x_offset_ptr;
	cal_info.y_offset = *y_offset_ptr;
	cal_info.x_slope = x_slope;
	cal_info.y_slope = y_slope;
	cal_info.temperature = *temperature_ptr;

	temperature_degree= *temperature_ptr;
	temperature_degree = 21000 + ((*temperature_ptr) * 3);
	pr_info("[RUMBA_S]  [0x19] Gyro Offset Calibration: X-Offset %08d, Y-Offset %08d, X-Slope %08d, Y-Slope %08d,  temperature= %d.%03d (%d)\n",
		       *x_offset_ptr, *y_offset_ptr, x_slope, y_slope, (temperature_degree/1000), (temperature_degree%1000) , *temperature_ptr);

	return rc;
}


int32_t  rumbas_act_trigger_offline_calibration(int start)
{
	int32_t rc = 0;
	uint8_t byte_data[8];

	memset(byte_data, 0, sizeof(byte_data));
	if (start)
		byte_data[0] = 0x01;
	rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x29, &byte_data[0], 8);
	if (rc < 0) {
		pr_err("%s trigger gyro offline calibration failed\n", __func__);
	}

	return rc;
}

int32_t rumbas_act_write_ois_calibration_method1(struct msm_actuator_get_ois_cal_info_t * cal_info_user)
{
	int32_t rc = 0;
	uint8_t byte_data[8];

	pr_info("[RUMBA_S]  %s\n", __func__);

	
	memset(byte_data, 0, sizeof(byte_data));
	byte_data[2] = 0xFF;
	byte_data[3] = 0xFF;
	byte_data[4] = 0xFF;
	byte_data[5] = 0xFF;
	rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x3A, &byte_data[0], 8);
	if (rc < 0) {
		pr_err("%s clear the 10-points calibration data failed\n", __func__);
		return rc;
	}

	
	((int16_t *)byte_data)[0] = ENDIAN(cal_info_user->x_offset);
	((int16_t *)byte_data)[1] = ENDIAN(cal_info_user->y_offset);
	byte_data[4] = cal_info_user->x_slope;
	byte_data[5] = cal_info_user->y_slope;
	((int16_t *)byte_data)[3] = ENDIAN(cal_info_user->temperature);

	rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x19, &byte_data[0], 8);
	if (rc < 0) {
		pr_err("%s write the new calibration data to firmware failed\n", __func__);
		return rc;
	}

	pr_info("[RUMBA_S]  Witre data to flash, new x_offset=%08d\n", cal_info_user->x_offset);
	pr_info("[RUMBA_S]  Witre data to flash, new y_offset=%08d\n", cal_info_user->y_offset);
	pr_info("[RUMBA_S]  Witre data to flash, new x_slope=%08d\n", cal_info_user->x_slope);
	pr_info("[RUMBA_S]  Witre data to flash, new y_slope=%08d\n", cal_info_user->y_slope);
	pr_info("[RUMBA_S]  Witre data to flash, new temperature=%08d\n", cal_info_user->temperature);

	return rc;
}

int32_t rumbas_act_write_ois_calibration_method2(struct msm_actuator_get_ois_cal_info_t * cal_info_user)
{
	int32_t rc = 0;
	uint8_t byte_data[8];

	pr_info("[RUMBA_S]  %s\n", __func__);

	
	memset(byte_data, 0, sizeof(byte_data));
	byte_data[0] = 0; 
	byte_data[1] = cal_info_user->cal_current_point - 1;
	((int16_t *)byte_data)[1] = ENDIAN(cal_info_user->x_offset);
	((int16_t *)byte_data)[2] = ENDIAN(cal_info_user->temperature);
	rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x3A, &byte_data[0], 8);
	if (rc < 0) {
		pr_err("%s write the new calibration data to firmware failed\n", __func__);
		return rc;
	}

	memset(byte_data, 0, sizeof(byte_data));
	byte_data[0] = 1; 
	byte_data[1] = cal_info_user->cal_current_point - 1;
	((int16_t *)byte_data)[1] = ENDIAN(cal_info_user->y_offset);
	((int16_t *)byte_data)[2] = ENDIAN(cal_info_user->temperature);
	rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x3A, &byte_data[0], 8);
	if (rc < 0) {
		pr_err("%s write the new calibration data to firmware failed\n", __func__);
		return rc;
	}

	pr_info("[RUMBA_S]  Witre data to flash, point number=%d\n", cal_info_user->cal_current_point);
	pr_info("[RUMBA_S]  Witre data to flash, new x_offset=%08d\n", cal_info_user->x_offset);
	pr_info("[RUMBA_S]  Witre data to flash, new y_offset=%08d\n", cal_info_user->y_offset);
	pr_info("[RUMBA_S]  Witre data to flash, new temperature=%08d\n", cal_info_user->temperature);

	return rc;
}

int32_t rumbas_act_write_ois_calibration_dump_to_flash(struct msm_actuator_get_ois_cal_info_t * cal_info_user)
{
	int32_t rc = 0;
	uint8_t byte_data[8];
	uint8_t byte_read[9];
	int i;

	pr_info("[RUMBA_S]  %s\n", __func__);

	
	memset(byte_data, 0, sizeof(byte_data));
	byte_data[0] = 0x01;
	byte_data[1] = 0x43;
	byte_data[2] = 0x61;
	byte_data[3] = 0x6C;
	rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x3E, &byte_data[0], 8);
	if (rc < 0) {
		pr_err("%s trigger frmware to write the new calibration data to flash failed\n", __func__);
		return rc;
	}
	msleep(2000);

	for(i=0; i<5 ; i++) {
		memset(byte_read, 0, sizeof(byte_read));
		rc = msm_camera_i2c_read_seq_rumbas(&(rumbas_act_t.i2c_client), (0x3E|0x80), &byte_read[0], 9);
		if (rc < 0) {
			pr_err("%s 0x3E reading error\n", __func__);
			return rc;
		}

		if (byte_read[1] != 0xFF) {
			pr_info("[RUMBA_S]  write to flash done , status=0x%2x\n", byte_read[1]);
			cal_info_user->write_flash_status = 1;
			break;
		} else {
			msleep(1000);
			pr_info("[RUMBA_S]  write to flash busy , count =%d\n", i);
		}
	}

	if (i >=  5)
		pr_info("[RUMBA_S]  write to flash fail caused by firmware busy\n");

	return rc;
}

int32_t rumbas_act_set_ois_calibration(struct msm_actuator_ctrl_t *a_ctrl, struct msm_actuator_get_ois_cal_info_t * cal_info_user)
{
	int32_t rc = 0;
	int interval = 1000;
	int lens_position = 0;

	pr_info("[RUMBA_S]  %s\n", __func__);

	cal_info.ois_cal_mode = cal_info_user->ois_cal_mode;
	pr_info("[RUMBA_S]  cal_info_user->lens_position=%d\n", cal_info_user->lens_position);

	if (cal_info_user->ois_cal_mode == OIS_CAL_MODE_READ_FIRMWARE) {
		lens_position = cal_info_user->lens_position;
		pr_info("[RUMBA_S]  OIS calibration to fix the lens_position=%d\n", lens_position);
		rumbas_move_lens_position(lens_position);
		msleep(1000);

		
		pr_info("[RUMBA_S]  %s  OIS_CAL_MODE_READ_FIRMWARE\n", __func__);
		rumbas_act_read_cal_data();

		if( (g_af_value.AF_INF_MSB == 0) && (g_af_value.AF_INF_LSB == 0) &&
			(g_af_value.AF_MACRO_MSB == 0) && (g_af_value.AF_MACRO_LSB == 0) ) {
			pr_info("OTP data is error. It's not allowed to do any calibration\n");
			cal_info.otp_check_pass = FALSE;
		} else {
			cal_info.otp_check_pass = TRUE;
		}

		memcpy(cal_info_user, &cal_info, sizeof(struct msm_actuator_get_ois_cal_info_t));
	} else if (cal_info_user->ois_cal_mode == OIS_CAL_MODE_COLLECT_DATA) {
		
		pr_info("[RUMBA_S]  %s  OIS_CAL_MODE_COLLECT_DATA\n", __func__);
		interval = cal_info_user->cal_collect_interval;
		pr_info("[RUMBA_S]  %s  interval=%d\n", __func__, interval);
		rumbas_act_trigger_offline_calibration(1);
		mdelay(interval);
		rumbas_act_trigger_offline_calibration(0);

		rumbas_act_read_cal_data();

		memcpy(cal_info_user, &cal_info, sizeof(struct msm_actuator_get_ois_cal_info_t));
	} else if (cal_info_user->ois_cal_mode == OIS_CAL_MODE_WRITE_FIRMWARE) {
		
		pr_info("[RUMBA_S]  %s  OIS_CAL_MODE_WRITE_FIRMWARE\n", __func__);
		if (cal_info_user->cal_method == 0) {
			rumbas_act_write_ois_calibration_method1(cal_info_user);
			rumbas_act_write_ois_calibration_dump_to_flash(cal_info_user);
		} else {

			pr_info("[RUMBA_S]  cal_current_point=%d , cal_max_point=%d\n",
					cal_info_user->cal_current_point, cal_info_user->cal_max_point);

			if (cal_info_user->cal_max_point > 0 && cal_info_user->cal_current_point > 0 &&
				cal_info_user->cal_current_point <= cal_info_user->cal_max_point) {
				rumbas_act_write_ois_calibration_method2(cal_info_user);
				if (cal_info_user->cal_current_point == cal_info_user->cal_max_point)
					rumbas_act_write_ois_calibration_dump_to_flash(cal_info_user);
			}
		}
	}

	return rc;
}


static const struct i2c_device_id rumbas_act_i2c_id[] = {
	{"rumbas_act", (kernel_ulong_t)&rumbas_act_t},
	{ }
};

static int rumbas_act_config(
	void __user *argp)
{
	LINFO("%s called\n", __func__);
	return (int) msm_actuator_config(&rumbas_act_t,
		rumbas_msm_actuator_info, argp); 
}

static int rumbas_i2c_add_driver_table(
	void)
{
	int32_t rc = 0;

	LINFO("%s called\n", __func__);

	rc = rumbas_poweron_af();
	if (rc < 0) {
		pr_err("%s power on failed\n", __func__);
		return (int) rc;
	}

	rumbas_act_t.step_position_table = NULL;
	rc = rumbas_act_t.func_tbl.actuator_init_table(&rumbas_act_t);
	if (rc < 0) {
		pr_err("%s init table failed\n", __func__);
		return (int) rc;
	}
	if(rumbas_act_t.actuator_ext_ctrl.is_ois_supported)
		rumbas_select_ois_map_table(USER_TABLE);
	return (int) rc;
}

static struct i2c_driver rumbas_act_i2c_driver = {
	.id_table = rumbas_act_i2c_id,
	.probe  = msm_actuator_i2c_probe,
	.remove = __exit_p(rumbas_act_i2c_remove),
	.driver = {
		.name = "rumbas_act",
	},
};

static int rumbas_sysfs_init(void);

static int __init rumbas_i2c_add_driver(
	void)
{
	LINFO("%s called\n", __func__);

	rumbas_sysfs_init();

	return i2c_add_driver(rumbas_act_t.i2c_driver);
}

static struct v4l2_subdev_core_ops rumbas_act_subdev_core_ops;

static struct v4l2_subdev_ops rumbas_act_subdev_ops = {
	.core = &rumbas_act_subdev_core_ops,
};

static int32_t rumbas_act_create_subdevice(
	void *board_info,
	void *sdev)
{
	LINFO("%s called\n", __func__);

	rumbas_msm_actuator_info = (struct msm_actuator_info *)board_info;

	return (int) msm_actuator_create_subdevice(&rumbas_act_t,
		rumbas_msm_actuator_info->board_info,
		(struct v4l2_subdev *)sdev);
}

static struct msm_actuator_ctrl_t rumbas_act_t = {
	.i2c_driver = &rumbas_act_i2c_driver,
	.i2c_addr = REG_VCM_I2C_ADDR,
	.act_v4l2_subdev_ops = &rumbas_act_subdev_ops,
	.actuator_ext_ctrl = {
		.a_init_table = rumbas_i2c_add_driver_table,
		.a_power_down = rumbas_actuator_af_power_down,
		.a_create_subdevice = rumbas_act_create_subdevice,
		.a_config = rumbas_act_config,
		.is_ois_supported = 1,
		.small_step_damping = 47,
		.medium_step_damping = 75,
		.big_step_damping = 100,
		.is_af_infinity_supported = 0,
		
		.do_vcm_on_cb	= rumbas_do_cam_vcm_on_cb,
		.do_vcm_off_cb	= rumbas_do_cam_vcm_off_cb,
		.actuator_poweroff_af = rumbas_poweroff_af,
		
	},

	.i2c_client = {
		.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	},

	.set_info = {
		.total_steps = RUMBAS_TOTAL_STEPS_NEAR_TO_FAR_RAWCHIP_AF,
		.gross_steps = 3,	
		.fine_steps = 1,	
	},

	.curr_step_pos = 0,
	.curr_region_index = 0,
	.initial_code = 0,	
	.actuator_mutex = &rumbas_act_mutex,
	.af_algo = AF_ALGO_RAWCHIP, 
	.ois_ready_version = 0,

	.func_tbl = {
		.actuator_init_table = rumbas_msm_actuator_init_table,
		.actuator_move_focus = rumbas_msm_actuator_move_focus,
		.actuator_write_focus = rumbas_act_write_focus,
		.actuator_set_default_focus = msm_actuator_set_default_focus,
		.actuator_init_focus = rumbas_act_init_focus,
		.actuator_i2c_write = rumbas_wrapper_i2c_write,
		.actuator_set_ois_mode = rumbas_act_set_ois_mode,
		.actuator_update_ois_tbl = rumbas_act_update_ois_tbl,
		.actuator_set_af_value = rumbas_act_set_af_value,
		.actuator_set_ois_calibration = rumbas_act_set_ois_calibration,

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

void rumbas_set_internal_clk(void)
{
	int32_t rc = 0;
	uint8_t byte_data[8] = {0,0,0,0,0,0,0,0};

	
	if (rumbas_msm_actuator_info == NULL)
		return;

	memset(byte_data, 0, sizeof(byte_data));
	rc = msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client),
		0x25, &byte_data[0], 8);
	if (rc < 0) {
		pr_err("%s set internal clk  i2c write failed (%d)\n", __func__, rc);
	}
}

void rumbas_disable_OIS(void)
{
	
	if (rumbas_msm_actuator_info == NULL)
		return;

	rumbas_act_set_ois_mode(&rumbas_act_t, 0);
}

void rumbas_move_lens_position(int16_t next_lens_position)
{
	
	if (rumbas_msm_actuator_info == NULL)
		return;

	rumbas_wrapper_i2c_write(&rumbas_act_t, next_lens_position, NULL);
}

void rumbas_move_lens_position_by_stroke(int16_t stroke)
{
	uint8_t byte_data[8];

	
	if (rumbas_msm_actuator_info == NULL)
		return;

	if (stroke < 0 || stroke > 50)
		return;

	memset(byte_data, 0, sizeof(byte_data));
	byte_data[0] = (stroke & 0xFF00) >> 8;
	byte_data[1] = stroke & 0x00FF;
	msm_camera_i2c_write_seq(&(rumbas_act_t.i2c_client), 0x15, &byte_data[0], 8);
}

#if 0
static int g_fixed_lens_position = 500;
#else
static int g_fixed_lens_position = ILLEGAL_CMD_INPUT_VALUE;
#endif

int get_fixed_lens_position(void)
{
	return g_fixed_lens_position;
}

static ssize_t fixed_lens_position_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;
	pr_info("%s called\n", __func__);

	sscanf(buf, "%d ",&input);
	pr_info("%s  input: %d\n",__func__,input);
	g_fixed_lens_position = input;
	return size;
}

static ssize_t fixed_lens_position_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	pr_info("%s called\n", __func__);

	sprintf(buf, "%d\n", g_fixed_lens_position);
	ret = strlen(buf) + 1;
	return ret;
}

static DEVICE_ATTR(fixed_lens_position, S_IRUGO | S_IWUSR, fixed_lens_position_show, fixed_lens_position_store);


int16_t get_fixed_rx_angle(void)
{
	return g_fixed_rx_angle;
}
static ssize_t fixed_rx_angle_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;
	pr_info("%s called\n", __func__);

	sscanf(buf, "%d ",&input);
	pr_info("%s  input: %d\n",__func__,input);
	g_fixed_rx_angle = input;
	return size;
}
static ssize_t fixed_rx_angle_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	pr_info("%s called\n", __func__);

	sprintf(buf, "%d\n", g_fixed_rx_angle);
	ret = strlen(buf) + 1;
	return ret;
}
static DEVICE_ATTR(fixed_rx_angle, S_IRUGO | S_IWUSR, fixed_rx_angle_show, fixed_rx_angle_store);


int16_t get_fixed_ry_angle(void)
{
	return g_fixed_ry_angle;
}
static ssize_t fixed_ry_angle_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;
	pr_info("%s called\n", __func__);

	sscanf(buf, "%d ",&input);
	pr_info("%s  input: %d\n",__func__,input);
	g_fixed_ry_angle = input;
	return size;
}
static ssize_t fixed_ry_angle_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	pr_info("%s called\n", __func__);

	sprintf(buf, "%d\n", g_fixed_ry_angle);
	ret = strlen(buf) + 1;
	return ret;
}
static DEVICE_ATTR(fixed_ry_angle, S_IRUGO | S_IWUSR, fixed_ry_angle_show, fixed_ry_angle_store);


int16_t get_OIS_level(void)
{
	return g_OIS_level;
}
static ssize_t OIS_level_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;
	pr_info("%s called\n", __func__);

	sscanf(buf, "%d ",&input);
	pr_info("%s  input: %d\n",__func__,input);
	g_OIS_level = input;
	return size;
}
static ssize_t OIS_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	pr_info("%s called\n", __func__);

	sprintf(buf, "%d\n", g_OIS_level);
	ret = strlen(buf) + 1;
	return ret;
}
static DEVICE_ATTR(OIS_level, S_IRUGO | S_IWUSR, OIS_level_show, OIS_level_store);


uint16_t get_AutoOnOffThresholds_parm1(void)
{
	return g_AutoOnOffThresholds_parm1;
}
static ssize_t AutoOnOffThresholds_parm1_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;
	pr_info("%s called\n", __func__);

	sscanf(buf, "%d ",&input);
	pr_info("%s  input: %d\n",__func__,input);
	g_AutoOnOffThresholds_parm1 = input;
	return size;
}
static ssize_t AutoOnOffThresholds_parm1_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	pr_info("%s called\n", __func__);

	sprintf(buf, "%d\n", g_AutoOnOffThresholds_parm1);
	ret = strlen(buf) + 1;
	return ret;
}
static DEVICE_ATTR(AutoOnOffThresholds_parm1, S_IRUGO | S_IWUSR, AutoOnOffThresholds_parm1_show, AutoOnOffThresholds_parm1_store);


uint16_t get_AutoOnOffThresholds_parm2(void)
{
	return g_AutoOnOffThresholds_parm2;
}
static ssize_t AutoOnOffThresholds_parm2_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;
	pr_info("%s called\n", __func__);

	sscanf(buf, "%d ",&input);
	pr_info("%s  input: %d\n",__func__,input);
	g_AutoOnOffThresholds_parm2 = input;
	return size;
}
static ssize_t AutoOnOffThresholds_parm2_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	pr_info("%s called\n", __func__);

	sprintf(buf, "%d\n", g_AutoOnOffThresholds_parm2);
	ret = strlen(buf) + 1;
	return ret;
}
static DEVICE_ATTR(AutoOnOffThresholds_parm2, S_IRUGO | S_IWUSR, AutoOnOffThresholds_parm2_show, AutoOnOffThresholds_parm2_store);


int16_t get_LFCE(void)
{
	return g_LFCE;
}
static ssize_t LFCE_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;
	pr_info("%s called\n", __func__);

	sscanf(buf, "%d ",&input);
	pr_info("%s  input: %d\n",__func__,input);
	g_LFCE = input;
	return size;
}
static ssize_t LFCE_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	pr_info("%s called\n", __func__);

	sprintf(buf, "%d\n", g_LFCE);
	ret = strlen(buf) + 1;
	return ret;
}
static DEVICE_ATTR(LFCE, S_IRUGO | S_IWUSR, LFCE_show, LFCE_store);


int16_t get_fast_reset_mode(void)
{
	return g_fast_reset_mode;
}
static ssize_t fast_reset_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;
	pr_info("%s called\n", __func__);

	sscanf(buf, "%d ",&input);
	pr_info("%s  input: %d\n",__func__,input);
	g_fast_reset_mode = input;
	return size;
}
static ssize_t fast_reset_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	pr_info("%s called\n", __func__);

	sprintf(buf, "%d\n", g_fast_reset_mode);
	ret = strlen(buf) + 1;
	return ret;
}
static DEVICE_ATTR(fast_reset_mode, S_IRUGO | S_IWUSR, fast_reset_mode_show, fast_reset_mode_store);


uint16_t get_reset_speed(void)
{
	return g_reset_speed;
}
static ssize_t reset_speed_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;
	pr_info("%s called\n", __func__);

	sscanf(buf, "%d ",&input);
	pr_info("%s  input: %d\n",__func__,input);
	g_reset_speed = input;
	return size;
}
static ssize_t reset_speed_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	pr_info("%s called\n", __func__);

	sprintf(buf, "%d\n", g_reset_speed);
	ret = strlen(buf) + 1;
	return ret;
}
static DEVICE_ATTR(reset_speed, S_IRUGO | S_IWUSR, reset_speed_show, reset_speed_store);


uint16_t get_calibration_coefficient(void)
{
	return g_calibration_coefficient;
}
static ssize_t calibration_coefficient_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;
	pr_info("%s called\n", __func__);

	sscanf(buf, "%d ",&input);
	pr_info("%s  input: %d\n",__func__,input);
	g_calibration_coefficient = input;
	return size;
}
static ssize_t calibration_coefficient_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	pr_info("%s called\n", __func__);

	sprintf(buf, "%d\n", g_calibration_coefficient);
	ret = strlen(buf) + 1;
	return ret;
}
static DEVICE_ATTR(calibration_coefficient, S_IRUGO | S_IWUSR, calibration_coefficient_show, calibration_coefficient_store);


static struct kobject *android_rumbas;

static int rumbas_sysfs_init(void)
{
	int ret ;
	pr_info("%s: rumbas:kobject creat and add\n", __func__);

	android_rumbas = kobject_create_and_add("android_cam_vcm", NULL);
	if (android_rumbas == NULL) {
		pr_info("rumbas_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("rumbas:sysfs_create_file\n");
	ret = sysfs_create_file(android_rumbas, &dev_attr_fixed_lens_position.attr);
	if (ret) {
		pr_info("rumbas_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_rumbas);
	}

	ret = sysfs_create_file(android_rumbas, &dev_attr_fixed_rx_angle.attr);
	if (ret) {
		pr_info("rumbas_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_rumbas);
	}

	ret = sysfs_create_file(android_rumbas, &dev_attr_fixed_ry_angle.attr);
	if (ret) {
		pr_info("rumbas_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_rumbas);
	}

	ret = sysfs_create_file(android_rumbas, &dev_attr_OIS_level.attr);
	if (ret) {
		pr_info("rumbas_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_rumbas);
	}

	ret = sysfs_create_file(android_rumbas, &dev_attr_AutoOnOffThresholds_parm1.attr);
	if (ret) {
		pr_info("rumbas_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_rumbas);
	}

	ret = sysfs_create_file(android_rumbas, &dev_attr_AutoOnOffThresholds_parm2.attr);
	if (ret) {
		pr_info("rumbas_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_rumbas);
	}

	ret = sysfs_create_file(android_rumbas, &dev_attr_LFCE.attr);
	if (ret) {
		pr_info("rumbas_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_rumbas);
	}

	ret = sysfs_create_file(android_rumbas, &dev_attr_fast_reset_mode.attr);
	if (ret) {
		pr_info("rumbas_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_rumbas);
	}

	ret = sysfs_create_file(android_rumbas, &dev_attr_reset_speed.attr);
	if (ret) {
		pr_info("rumbas_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_rumbas);
	}

	ret = sysfs_create_file(android_rumbas, &dev_attr_calibration_coefficient.attr);
	if (ret) {
		pr_info("rumbas_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_rumbas);
	}

	return 0 ;
}

subsys_initcall(rumbas_i2c_add_driver);
MODULE_DESCRIPTION("RUMBA-S actuator");
MODULE_LICENSE("GPL v2");
