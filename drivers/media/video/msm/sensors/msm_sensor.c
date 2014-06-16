/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "msm_sensor.h"
#include "msm.h"
#include "msm_ispif.h"
#include "msm_camera_i2c_mux.h"
#include <linux/init.h>
#include <linux/bootmem.h>

#ifdef CONFIG_RAWCHIP
#include "rawchip/rawchip.h"
#endif

#ifdef CONFIG_RAWCHIPII
#include "yushanII.h"
#include "ilp0100_ST_api.h"
#include "ilp0100_customer_sensor_config.h"
#endif

uint32_t ois_line;

extern char *saved_command_line;	

static int oem_sensor_init(void *arg)
{
	struct msm_sensor_ctrl_t *s_ctrl = (struct msm_sensor_ctrl_t *) arg;
	int res;

#ifdef CONFIG_RAWCHIP
	struct rawchip_sensor_data rawchip_data;
	struct timespec ts_start, ts_end;
#endif

	
	pr_info("%s: E", __func__);
	

	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);

	s_ctrl->curr_csi_params = NULL;
	msm_sensor_enable_debugfs(s_ctrl);

	res =0;
	if (s_ctrl->curr_csi_params != s_ctrl->csi_params[res]) {
		s_ctrl->curr_csi_params = s_ctrl->csi_params[res];
		s_ctrl->curr_csi_params->csid_params.lane_assign =
			s_ctrl->sensordata->sensor_platform_info->
			csi_lane_params->csi_lane_assign;
		s_ctrl->curr_csi_params->csiphy_params.lane_mask =
			s_ctrl->sensordata->sensor_platform_info->
			csi_lane_params->csi_lane_mask;
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_CSID_CFG,
			&s_ctrl->curr_csi_params->csid_params);
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_CID_CHANGE, &s_ctrl->intf);
		mb();
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_CSIPHY_CFG,
			&s_ctrl->curr_csi_params->csiphy_params);
		mb();
	}
	msleep(30);

	msm_sensor_write_init_settings(s_ctrl);
	

#ifdef CONFIG_RAWCHIP
	if (s_ctrl->sensordata->use_rawchip) {
		rawchip_data.sensor_name = s_ctrl->sensordata->sensor_name;
		if (s_ctrl->curr_csi_params) {
			rawchip_data.datatype = s_ctrl->curr_csi_params->csid_params.lut_params.vc_cfg->dt;
			rawchip_data.lane_cnt = s_ctrl->curr_csi_params->csid_params.lane_cnt;
		}
		else {
		pr_info("%s: s_ctrl->curr_csi_params is null\n", __func__);
		}
		rawchip_data.pixel_clk = s_ctrl->msm_sensor_reg->output_settings[res].op_pixel_clk;
		rawchip_data.mirror_flip = s_ctrl->mirror_flip;

		rawchip_data.width = s_ctrl->msm_sensor_reg->output_settings[res].x_output;
		rawchip_data.height = s_ctrl->msm_sensor_reg->output_settings[res].y_output;
		rawchip_data.line_length_pclk = s_ctrl->msm_sensor_reg->output_settings[res].line_length_pclk;
		rawchip_data.frame_length_lines = s_ctrl->msm_sensor_reg->output_settings[res].frame_length_lines;
		rawchip_data.x_addr_start = s_ctrl->msm_sensor_reg->output_settings[res].x_addr_start;
		rawchip_data.y_addr_start = s_ctrl->msm_sensor_reg->output_settings[res].y_addr_start;
		rawchip_data.x_addr_end = s_ctrl->msm_sensor_reg->output_settings[res].x_addr_end;
		rawchip_data.y_addr_end = s_ctrl->msm_sensor_reg->output_settings[res].y_addr_end;
		rawchip_data.x_even_inc = s_ctrl->msm_sensor_reg->output_settings[res].x_even_inc;
		rawchip_data.x_odd_inc = s_ctrl->msm_sensor_reg->output_settings[res].x_odd_inc;
		rawchip_data.y_even_inc = s_ctrl->msm_sensor_reg->output_settings[res].y_even_inc;
		rawchip_data.y_odd_inc = s_ctrl->msm_sensor_reg->output_settings[res].y_odd_inc;
		rawchip_data.binning_rawchip = s_ctrl->msm_sensor_reg->output_settings[res].binning_rawchip;

		rawchip_data.fullsize_width = s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].x_output;
		rawchip_data.fullsize_height = s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].y_output;
		rawchip_data.fullsize_line_length_pclk =
			s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].line_length_pclk;
		rawchip_data.fullsize_frame_length_lines =
			s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].frame_length_lines;
		rawchip_data.use_rawchip = s_ctrl->sensordata->use_rawchip;

		ktime_get_ts(&ts_start);
		rawchip_set_size(rawchip_data);
		ktime_get_ts(&ts_end);

		res = (ts_end.tv_sec-ts_start.tv_sec)*1000+(ts_end.tv_nsec-ts_start.tv_nsec)/1000000;
		pr_info("%s: rawchip_set_size:%ld ms\n", __func__, (long)res);

	}
#endif

	pr_info("%s: X", __func__);
	mutex_unlock(s_ctrl->sensor_first_mutex);

	return 0;
}

int32_t msm_sensor_adjust_frame_lines(struct msm_sensor_ctrl_t *s_ctrl,
	uint16_t res)
{
	uint16_t cur_line = 0;
	uint16_t exp_fl_lines = 0;
	CDBG("%s: called\n", __func__);

	if (s_ctrl->sensor_exp_gain_info) {
		if (s_ctrl->prev_gain && s_ctrl->prev_line &&
			s_ctrl->func_tbl->sensor_write_exp_gain)
			s_ctrl->func_tbl->sensor_write_exp_gain(
				s_ctrl,
				s_ctrl->prev_gain,
				s_ctrl->prev_line);
	
		msm_camera_i2c_read(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
			&cur_line,
			MSM_CAMERA_I2C_WORD_DATA);
		exp_fl_lines = cur_line +
			s_ctrl->sensor_exp_gain_info->vert_offset;
		if (exp_fl_lines > s_ctrl->msm_sensor_reg->
			output_settings[res].frame_length_lines)
			msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
				s_ctrl->sensor_output_reg_addr->
				frame_length_lines,
				exp_fl_lines,
				MSM_CAMERA_I2C_WORD_DATA);
		CDBG("%s cur_fl_lines %d, exp_fl_lines %d\n", __func__,
			s_ctrl->msm_sensor_reg->
			output_settings[res].frame_length_lines,
			exp_fl_lines);
	}
	return 0;
}
 

int32_t msm_sensor_write_init_settings(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc;
	CDBG("%s: called\n", __func__);

	if ((s_ctrl->sensordata->htc_image == HTC_CAMERA_IMAGE_YUSHANII_BOARD) && (s_ctrl->msm_sensor_reg->init_settings_yushanii))
	{
		rc = msm_sensor_write_all_conf_array(
			s_ctrl->sensor_i2c_client,
			s_ctrl->msm_sensor_reg->init_settings_yushanii,
			s_ctrl->msm_sensor_reg->init_size_yushanii);
	} else {
		rc = msm_sensor_write_all_conf_array(
			s_ctrl->sensor_i2c_client,
			s_ctrl->msm_sensor_reg->init_settings,
			s_ctrl->msm_sensor_reg->init_size);
	}

    return rc;
}

int32_t msm_sensor_write_res_settings(struct msm_sensor_ctrl_t *s_ctrl,
	uint16_t res)
{
	int32_t rc;
	CDBG("%s: called\n", __func__);

	rc = msm_sensor_write_conf_array(
		s_ctrl->sensor_i2c_client,
		s_ctrl->msm_sensor_reg->mode_settings, res);
	if (rc < 0)
	{
		pr_info("%s: failed to msm_sensor_write_conf_array return < 0\n", __func__);
		return rc;
	}

	rc = msm_sensor_write_output_settings(s_ctrl, res);
	if (rc < 0)
	{
		pr_info("%s: failed to msm_sensor_write_output_settings return < 0\n", __func__);
		return rc;
	}

	if (s_ctrl->func_tbl->sensor_write_output_settings_specific) {
		rc = s_ctrl->func_tbl->sensor_write_output_settings_specific(s_ctrl, res);
		if (rc < 0)
		{
			pr_info("%s: failed to sensor_write_output_settings_specific return < 0\n", __func__);
			return rc;
		}
	}

	if (s_ctrl->func_tbl->sensor_adjust_frame_lines)
		rc = s_ctrl->func_tbl->sensor_adjust_frame_lines(s_ctrl, res);

	if (s_ctrl->func_tbl->sensor_yushanII_set_default_ae) {
		s_ctrl->func_tbl->sensor_yushanII_set_default_ae(s_ctrl, res);
	} else {
		if (s_ctrl->prev_dig_gain > 0 && s_ctrl->prev_line > 0){
			if (s_ctrl->func_tbl->
				sensor_write_exp_gain_ex != NULL){
			    s_ctrl->func_tbl->
			        sensor_write_exp_gain_ex(
			        s_ctrl,
			        SENSOR_PREVIEW_MODE,
			        s_ctrl->prev_gain,
			        s_ctrl->prev_dig_gain,
			        s_ctrl->prev_line);
			}
		}
	}

	return rc;
}

int32_t msm_sensor_write_output_settings(struct msm_sensor_ctrl_t *s_ctrl,
	uint16_t res)
{
	int32_t rc = -EFAULT;
	struct msm_camera_i2c_reg_conf dim_settings[] = {
		{s_ctrl->sensor_output_reg_addr->x_output,
			s_ctrl->msm_sensor_reg->
			output_settings[res].x_output},
		{s_ctrl->sensor_output_reg_addr->y_output,
			s_ctrl->msm_sensor_reg->
			output_settings[res].y_output},
		{s_ctrl->sensor_output_reg_addr->line_length_pclk,
			s_ctrl->msm_sensor_reg->
			output_settings[res].line_length_pclk},
		{s_ctrl->sensor_output_reg_addr->frame_length_lines,
			s_ctrl->msm_sensor_reg->
			output_settings[res].frame_length_lines},
	};
	CDBG("%s: called\n", __func__);

	rc = msm_camera_i2c_write_tbl(s_ctrl->sensor_i2c_client, dim_settings,
		ARRAY_SIZE(dim_settings), MSM_CAMERA_I2C_WORD_DATA);
	return rc;
}

void msm_sensor_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	CDBG("%s: called\n", __func__);

	if ((s_ctrl->sensordata->htc_image == HTC_CAMERA_IMAGE_YUSHANII_BOARD) && (s_ctrl->msm_sensor_reg->start_stream_conf_yushanii))
	{
		msm_camera_i2c_write_tbl(
		s_ctrl->sensor_i2c_client,
		s_ctrl->msm_sensor_reg->start_stream_conf_yushanii,
		s_ctrl->msm_sensor_reg->start_stream_conf_size_yushanii,
		s_ctrl->msm_sensor_reg->default_data_type);
	} else {
		msm_camera_i2c_write_tbl(
		s_ctrl->sensor_i2c_client,
		s_ctrl->msm_sensor_reg->start_stream_conf,
		s_ctrl->msm_sensor_reg->start_stream_conf_size,
		s_ctrl->msm_sensor_reg->default_data_type);
	}
}

void msm_sensor_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	CDBG("%s: called\n", __func__);

	if ((s_ctrl->sensordata->htc_image == HTC_CAMERA_IMAGE_YUSHANII_BOARD) && (s_ctrl->msm_sensor_reg->stop_stream_conf_yushanii))
	{
		msm_camera_i2c_write_tbl(
		s_ctrl->sensor_i2c_client,
		s_ctrl->msm_sensor_reg->stop_stream_conf_yushanii,
		s_ctrl->msm_sensor_reg->stop_stream_conf_size_yushanii,
		s_ctrl->msm_sensor_reg->default_data_type);
	} else {
		msm_camera_i2c_write_tbl(
		s_ctrl->sensor_i2c_client,
		s_ctrl->msm_sensor_reg->stop_stream_conf,
		s_ctrl->msm_sensor_reg->stop_stream_conf_size,
		s_ctrl->msm_sensor_reg->default_data_type);
	}
}

void msm_sensor_group_hold_on(struct msm_sensor_ctrl_t *s_ctrl)
{
	CDBG("%s: called\n", __func__);

	msm_camera_i2c_write_tbl(
		s_ctrl->sensor_i2c_client,
		s_ctrl->msm_sensor_reg->group_hold_on_conf,
		s_ctrl->msm_sensor_reg->group_hold_on_conf_size,
		s_ctrl->msm_sensor_reg->default_data_type);
}

void msm_sensor_group_hold_off(struct msm_sensor_ctrl_t *s_ctrl)
{
	CDBG("%s: called\n", __func__);

	msm_camera_i2c_write_tbl(
		s_ctrl->sensor_i2c_client,
		s_ctrl->msm_sensor_reg->group_hold_off_conf,
		s_ctrl->msm_sensor_reg->group_hold_off_conf_size,
		s_ctrl->msm_sensor_reg->default_data_type);
}

void msm_sensor_group_hold_on_hdr(struct msm_sensor_ctrl_t *s_ctrl)
{
	CDBG("%s: called\n", __func__);

	msm_camera_i2c_write_tbl(
		s_ctrl->sensor_i2c_client,
		s_ctrl->msm_sensor_reg->group_hold_on_conf_hdr,
		s_ctrl->msm_sensor_reg->group_hold_on_conf_size_hdr,
		s_ctrl->msm_sensor_reg->default_data_type);
}

void msm_sensor_group_hold_off_hdr(struct msm_sensor_ctrl_t *s_ctrl)
{
	CDBG("%s: called\n", __func__);

	msm_camera_i2c_write_tbl(
		s_ctrl->sensor_i2c_client,
		s_ctrl->msm_sensor_reg->group_hold_off_conf_hdr,
		s_ctrl->msm_sensor_reg->group_hold_off_conf_size_hdr,
		s_ctrl->msm_sensor_reg->default_data_type);
}

int32_t msm_sensor_set_fps(struct msm_sensor_ctrl_t *s_ctrl,
						struct fps_cfg *fps)
{
	s_ctrl->fps_divider = fps->fps_div;

	return 0;
}

int32_t msm_sensor_write_exp_gain1(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	uint32_t fl_lines;
	uint8_t offset;
	CDBG("%s: called\n", __func__);

	fl_lines = s_ctrl->curr_frame_length_lines;
	fl_lines = (fl_lines * s_ctrl->fps_divider) / Q10;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line > (fl_lines - offset))
		fl_lines = line + offset;

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines, fl_lines,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, line,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain,
		MSM_CAMERA_I2C_WORD_DATA);
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}

int32_t msm_sensor_write_exp_gain2(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	uint32_t fl_lines, ll_pclk, ll_ratio;
	uint8_t offset;
	CDBG("%s: called\n", __func__);

	fl_lines = s_ctrl->curr_frame_length_lines * s_ctrl->fps_divider / Q10;
	ll_pclk = s_ctrl->curr_line_length_pclk;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line > (fl_lines - offset)) {
		ll_ratio = (line * Q10) / (fl_lines - offset);
		ll_pclk = ll_pclk * ll_ratio / Q10;
		line = fl_lines - offset;
	}

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->line_length_pclk, ll_pclk,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, line,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain,
		MSM_CAMERA_I2C_WORD_DATA);
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}

int32_t msm_sensor_setting3(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;
	static int csi_config;
	CDBG("%s: called\n", __func__);

	if (update_type == MSM_SENSOR_REG_INIT) {
		CDBG("Register INIT\n");
		s_ctrl->curr_csi_params = NULL;
		csi_config = 0;
		msm_camera_i2c_write(
			s_ctrl->sensor_i2c_client,
			0x0e, 0x08,
			MSM_CAMERA_I2C_BYTE_DATA);
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		CDBG("PERIODIC : %d\n", res);
		if (res == 0)
			return 0;
		if (!csi_config) {
			msm_sensor_write_conf_array(
				s_ctrl->sensor_i2c_client,
				s_ctrl->msm_sensor_reg->mode_settings, res);
			msleep(30);
			s_ctrl->curr_csic_params = s_ctrl->csic_params[res];
			CDBG("CSI config in progress\n");
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIC_CFG,
				s_ctrl->curr_csic_params);
			CDBG("CSI config is done\n");
			mb();
			msleep(30);
			msm_camera_i2c_write(
					s_ctrl->sensor_i2c_client,
					0x0e, 0x00,
					MSM_CAMERA_I2C_BYTE_DATA);
			csi_config = 1;
		}
		msleep(50);
	}
	return rc;
}

int32_t msm_sensor_write_exp_gain1_ex(struct msm_sensor_ctrl_t *s_ctrl,
		int mode, uint16_t gain, uint16_t dig_gain, uint32_t line) 
{
	uint32_t fl_lines;
	uint8_t offset;

	uint32_t fps_divider = Q10;
	CDBG("%s: called\n", __func__);
    ois_line = line;
	if (mode == SENSOR_PREVIEW_MODE)
		fps_divider = s_ctrl->fps_divider;

	if(line > s_ctrl->sensor_exp_gain_info->sensor_max_linecount)
		line = s_ctrl->sensor_exp_gain_info->sensor_max_linecount;

	fl_lines = s_ctrl->curr_frame_length_lines;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line * Q10 > (fl_lines - offset) * fps_divider)
		fl_lines = line + offset;
	else
		fl_lines = (fl_lines * fps_divider) / Q10;

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines, fl_lines,
		MSM_CAMERA_I2C_WORD_DATA);

	if (s_ctrl->func_tbl->sensor_ov2722_write_exp_line) {
		s_ctrl->func_tbl->sensor_ov2722_write_exp_line (s_ctrl,line);
	}
	else {
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, line,
			MSM_CAMERA_I2C_WORD_DATA);
	}
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain,
		MSM_CAMERA_I2C_WORD_DATA);
	
	if (s_ctrl->func_tbl->sensor_set_dig_gain)
		s_ctrl->func_tbl->sensor_set_dig_gain(s_ctrl, dig_gain);
	
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}

int32_t msm_sensor_write_exp_gain2_ex(struct msm_sensor_ctrl_t *s_ctrl,
		int mode, uint16_t gain, uint32_t line) 
{
	uint32_t fl_lines, ll_pclk, ll_ratio;
	uint8_t offset;
	uint32_t fps_divider = Q10;
	CDBG("%s: called\n", __func__);

	if (mode == SENSOR_PREVIEW_MODE)
		fps_divider = s_ctrl->fps_divider;
	fl_lines = s_ctrl->curr_frame_length_lines;
	ll_pclk = s_ctrl->curr_line_length_pclk;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line * Q10 > (fl_lines - offset) * fps_divider) {
		ll_ratio = (line * Q10) / (fl_lines - offset);
		ll_pclk = ll_pclk * ll_ratio / Q10;
		line = fl_lines - offset;
	} else {
		ll_pclk = ll_pclk * fps_divider / Q10;
		line = line / fps_divider;
	}

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->line_length_pclk, ll_pclk,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, line,
		MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain,
		MSM_CAMERA_I2C_WORD_DATA);
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}

int32_t msm_sensor_write_exp_gain_ov(struct msm_sensor_ctrl_t *s_ctrl,
		int mode, uint16_t gain, uint16_t dig_gain, uint32_t line)
{
	uint32_t fl_lines;
	uint8_t offset;
	uint32_t aec_msb_24; 
	uint16_t aec_msb;
	uint16_t aec_lsb;
	uint32_t phy_line_2 = 0;

	uint32_t fps_divider = Q10;
	CDBG("%s: called\n", __func__);

	if (mode == SENSOR_PREVIEW_MODE)
		fps_divider = s_ctrl->fps_divider;

	if(line > s_ctrl->sensor_exp_gain_info->sensor_max_linecount)
		line = s_ctrl->sensor_exp_gain_info->sensor_max_linecount;

	fl_lines = s_ctrl->curr_frame_length_lines;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line * Q10 > (fl_lines - offset) * fps_divider)
		fl_lines = line + offset;
	else
		fl_lines = (fl_lines * fps_divider) / Q10;

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines, fl_lines,
		MSM_CAMERA_I2C_WORD_DATA);

	phy_line_2 = line << 4;
	aec_msb_24 = (uint32_t)(phy_line_2 & 0xFF0000) >> 16;
	aec_msb = (uint16_t)(phy_line_2 & 0xFF00) >> 8;
	aec_lsb = (uint16_t)(phy_line_2 & 0x00FF);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, (uint8_t)aec_msb_24, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, s_ctrl->sensor_exp_gain_info->coarse_int_time_addr+1, (uint8_t)aec_msb, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, s_ctrl->sensor_exp_gain_info->coarse_int_time_addr+2, (uint8_t)aec_lsb, MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, gain,
		MSM_CAMERA_I2C_BYTE_DATA);
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}

int32_t msm_sensor_setting1(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;
	static int csi_config;

#ifdef CONFIG_RAWCHIP
	struct rawchip_sensor_data rawchip_data;
	struct timespec ts_start, ts_end;
#endif
	CDBG("%s: called\n", __func__);

	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);

	if (update_type == MSM_SENSOR_REG_INIT) {
		CDBG("Register INIT\n");
		s_ctrl->curr_csi_params = NULL;
		msm_sensor_enable_debugfs(s_ctrl);
		msm_sensor_write_init_settings(s_ctrl);
		csi_config = 0;
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		CDBG("PERIODIC : %d\n", res);
		msm_sensor_write_conf_array(
			s_ctrl->sensor_i2c_client,
			s_ctrl->msm_sensor_reg->mode_settings, res);
		msleep(30);
		if (!csi_config) {
			s_ctrl->curr_csic_params = s_ctrl->csic_params[res];
			CDBG("CSI config in progress\n");
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIC_CFG,
				s_ctrl->curr_csic_params);
			CDBG("CSI config is done\n");
			mb();
			msleep(20);
			csi_config = 1;
		}
#ifdef CONFIG_RAWCHIP
			if (s_ctrl->sensordata->use_rawchip) {
				rawchip_data.sensor_name = s_ctrl->sensordata->sensor_name;
				rawchip_data.datatype = s_ctrl->curr_csic_params->data_format;
					CDBG("datatype = %d\n", rawchip_data.datatype);
				rawchip_data.lane_cnt = s_ctrl->curr_csic_params->lane_cnt;
					CDBG("lane_cnt = %d\n", rawchip_data.lane_cnt);
				rawchip_data.pixel_clk = s_ctrl->msm_sensor_reg->output_settings[res].op_pixel_clk;
					CDBG("pixel_clk = %d\n", rawchip_data.pixel_clk);
				rawchip_data.mirror_flip = s_ctrl->mirror_flip;
					CDBG("mirror_flip = %d\n", rawchip_data.mirror_flip);

				rawchip_data.width = s_ctrl->msm_sensor_reg->output_settings[res].x_output;
					CDBG("width = %d\n", rawchip_data.width);
				rawchip_data.height = s_ctrl->msm_sensor_reg->output_settings[res].y_output;
					CDBG("height = %d\n", rawchip_data.height);
				rawchip_data.line_length_pclk = s_ctrl->msm_sensor_reg->output_settings[res].line_length_pclk;
					CDBG("line_length_pclk = %d\n", rawchip_data.line_length_pclk);
				rawchip_data.frame_length_lines = s_ctrl->msm_sensor_reg->output_settings[res].frame_length_lines;
					CDBG("frame_length_lines = %d\n", rawchip_data.frame_length_lines);
				rawchip_data.x_addr_start = s_ctrl->msm_sensor_reg->output_settings[res].x_addr_start;
					CDBG("x_addr_start = %d\n", rawchip_data.x_addr_start);
				rawchip_data.y_addr_start = s_ctrl->msm_sensor_reg->output_settings[res].y_addr_start;
					CDBG("y_addr_start = %d\n", rawchip_data.y_addr_start);
				rawchip_data.x_addr_end = s_ctrl->msm_sensor_reg->output_settings[res].x_addr_end;
					CDBG("x_addr_end = %d\n", rawchip_data.x_addr_end);
				rawchip_data.y_addr_end = s_ctrl->msm_sensor_reg->output_settings[res].y_addr_end;
					CDBG("y_addr_end = %d\n", rawchip_data.y_addr_end);
				rawchip_data.x_even_inc = s_ctrl->msm_sensor_reg->output_settings[res].x_even_inc;
					CDBG("x_even_inc = %d\n", rawchip_data.x_even_inc);
				rawchip_data.x_odd_inc = s_ctrl->msm_sensor_reg->output_settings[res].x_odd_inc;
					CDBG("x_odd_inc = %d\n", rawchip_data.x_odd_inc);
				rawchip_data.y_even_inc = s_ctrl->msm_sensor_reg->output_settings[res].y_even_inc;
					CDBG("y_even_inc = %d\n", rawchip_data.y_even_inc);
				rawchip_data.y_odd_inc = s_ctrl->msm_sensor_reg->output_settings[res].y_odd_inc;
					CDBG("y_odd_inc = %d\n", rawchip_data.y_odd_inc);
				rawchip_data.binning_rawchip = s_ctrl->msm_sensor_reg->output_settings[res].binning_rawchip;
					CDBG("binning_rawchip = %d\n", rawchip_data.binning_rawchip);

				rawchip_data.fullsize_width = s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].x_output;
					CDBG("fullsize_width = %d\n", rawchip_data.fullsize_width);
				rawchip_data.fullsize_height = s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].y_output;
					CDBG("fullsize_height = %d\n", rawchip_data.fullsize_height);
				rawchip_data.fullsize_line_length_pclk =
					s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].line_length_pclk;
					CDBG("fullsize_line_length_pclk = %d\n", rawchip_data.fullsize_line_length_pclk);
				rawchip_data.fullsize_frame_length_lines =
					s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].frame_length_lines;
					CDBG("rawchip_data.fullsize_frame_length_lines = %d\n", rawchip_data.fullsize_frame_length_lines);
				rawchip_data.use_rawchip = s_ctrl->sensordata->use_rawchip;

				ktime_get_ts(&ts_start);
				rawchip_set_size(rawchip_data);
				ktime_get_ts(&ts_end);
				pr_info("%s: %ld ms\n", __func__,
					(ts_end.tv_sec-ts_start.tv_sec)*1000+(ts_end.tv_nsec-ts_start.tv_nsec)/1000000);
			}
#endif
		s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
	}
	return rc;
}

int32_t msm_sensor_setting_parallel(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;

#ifdef CONFIG_RAWCHIP
	struct rawchip_sensor_data rawchip_data;
	struct timespec ts_start, ts_end;
#endif

	pr_info("%s: update_type=%d, res=%d\n", __func__, update_type, res);

	if (update_type == MSM_SENSOR_REG_INIT) {
		if (s_ctrl->first_init)   {
			pr_info("%s: MSM_SENSOR_REG_INIT already inited\n", __func__);
			return rc;
		}
		mutex_lock(s_ctrl->sensor_first_mutex);  

#ifdef CONFIG_RAWCHIPII
		if (s_ctrl->sensordata->htc_image == HTC_CAMERA_IMAGE_YUSHANII_BOARD) {
			YushanII_reload_firmware();
		}
#endif

		s_ctrl->tsk_sensor_init = kthread_create(oem_sensor_init, s_ctrl, "oem_sensor_init");
		if (IS_ERR(s_ctrl->tsk_sensor_init)) {
			pr_err("%s: kthread_create failed", __func__);
			rc = PTR_ERR(s_ctrl->tsk_sensor_init);
			s_ctrl->tsk_sensor_init = NULL;
			mutex_unlock(s_ctrl->sensor_first_mutex);  
		} else
			wake_up_process(s_ctrl->tsk_sensor_init);

		s_ctrl->first_init = 1;
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		

		
		mutex_lock(s_ctrl->sensor_first_mutex);

#ifdef CONFIG_RAWCHIPII
		if (s_ctrl->sensordata->htc_image == HTC_CAMERA_IMAGE_YUSHANII_BOARD) {
			if(YushanII_Get_reloadInfo() == 0){
				pr_info("stop YushanII first");
				Ilp0100_stop();
			}
		}
#endif
		switch (s_ctrl->intf) {
		case RDI0:
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				RDI_0, ISPIF_OFF_IMMEDIATELY));
			break;
		default:
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				PIX_0, ISPIF_OFF_IMMEDIATELY));
			break;
		}
		s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);

		if(!s_ctrl->first_init)
			mdelay(50);
		s_ctrl->first_init = 0;

		pr_info("%s: update_type=MSM_SENSOR_UPDATE_PERIODIC, res=%d\n", __func__, res);  
		

		msm_sensor_write_res_settings(s_ctrl, res);
		if (s_ctrl->curr_csi_params != s_ctrl->csi_params[res]) {
			s_ctrl->curr_csi_params = s_ctrl->csi_params[res];
			s_ctrl->curr_csi_params->csid_params.lane_assign =
				s_ctrl->sensordata->sensor_platform_info->
				csi_lane_params->csi_lane_assign;
			s_ctrl->curr_csi_params->csiphy_params.lane_mask =
				s_ctrl->sensordata->sensor_platform_info->
				csi_lane_params->csi_lane_mask;
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSID_CFG,
				&s_ctrl->curr_csi_params->csid_params);
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
						NOTIFY_CID_CHANGE, &s_ctrl->intf);
			mb();
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIPHY_CFG,
				&s_ctrl->curr_csi_params->csiphy_params);
			mb();
			msleep(20);
		}

#ifdef CONFIG_RAWCHIP
		if (s_ctrl->sensordata->use_rawchip) {
			rawchip_data.sensor_name = s_ctrl->sensordata->sensor_name;
			rawchip_data.datatype = s_ctrl->curr_csi_params->csid_params.lut_params.vc_cfg->dt;
			rawchip_data.lane_cnt = s_ctrl->curr_csi_params->csid_params.lane_cnt;
			rawchip_data.pixel_clk = s_ctrl->msm_sensor_reg->output_settings[res].op_pixel_clk;
			rawchip_data.mirror_flip = s_ctrl->mirror_flip;

			rawchip_data.width = s_ctrl->msm_sensor_reg->output_settings[res].x_output;
			rawchip_data.height = s_ctrl->msm_sensor_reg->output_settings[res].y_output;
			rawchip_data.line_length_pclk = s_ctrl->msm_sensor_reg->output_settings[res].line_length_pclk;
			rawchip_data.frame_length_lines = s_ctrl->msm_sensor_reg->output_settings[res].frame_length_lines;
			rawchip_data.x_addr_start = s_ctrl->msm_sensor_reg->output_settings[res].x_addr_start;
			rawchip_data.y_addr_start = s_ctrl->msm_sensor_reg->output_settings[res].y_addr_start;
			rawchip_data.x_addr_end = s_ctrl->msm_sensor_reg->output_settings[res].x_addr_end;
			rawchip_data.y_addr_end = s_ctrl->msm_sensor_reg->output_settings[res].y_addr_end;
			rawchip_data.x_even_inc = s_ctrl->msm_sensor_reg->output_settings[res].x_even_inc;
			rawchip_data.x_odd_inc = s_ctrl->msm_sensor_reg->output_settings[res].x_odd_inc;
			rawchip_data.y_even_inc = s_ctrl->msm_sensor_reg->output_settings[res].y_even_inc;
			rawchip_data.y_odd_inc = s_ctrl->msm_sensor_reg->output_settings[res].y_odd_inc;
			rawchip_data.binning_rawchip = s_ctrl->msm_sensor_reg->output_settings[res].binning_rawchip;

			rawchip_data.fullsize_width = s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].x_output;
			rawchip_data.fullsize_height = s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].y_output;
			rawchip_data.fullsize_line_length_pclk =
				s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].line_length_pclk;
			rawchip_data.fullsize_frame_length_lines =
				s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].frame_length_lines;
			rawchip_data.use_rawchip = s_ctrl->sensordata->use_rawchip;

			ktime_get_ts(&ts_start);
			rawchip_set_size(rawchip_data);
			ktime_get_ts(&ts_end);
			pr_info("%s: %ld ms\n", __func__,
				(ts_end.tv_sec-ts_start.tv_sec)*1000+(ts_end.tv_nsec-ts_start.tv_nsec)/1000000);
		}
#endif

#ifdef CONFIG_RAWCHIPII
			if (s_ctrl->sensordata->htc_image == HTC_CAMERA_IMAGE_YUSHANII_BOARD) {
				if (s_ctrl->msm_sensor_reg->output_settings_yushanii)	 
					s_ctrl->msm_sensor_reg->output_settings = s_ctrl->msm_sensor_reg->output_settings_yushanii;
				YushanII_Init(s_ctrl,res);
			}
#endif

		switch (s_ctrl->intf) {
		case RDI0:
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				RDI_0, ISPIF_ON_FRAME_BOUNDARY));
			break;
		default:
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_PCLK_CHANGE, &s_ctrl->msm_sensor_reg->
				output_settings[res].op_pixel_clk);

			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				PIX_0, ISPIF_ON_FRAME_BOUNDARY));
			break;
		}
		s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
		msleep(30);
		mutex_unlock(s_ctrl->sensor_first_mutex);  
	}
	return rc;
}

int32_t msm_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;

#ifdef CONFIG_RAWCHIP
	struct rawchip_sensor_data rawchip_data;
	struct timespec ts_start, ts_end;
#endif

	pr_info("%s: update_type=%d, res=%d\n", __func__, update_type, res);

	switch (s_ctrl->intf) {
	case RDI0:
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
			RDI_0, ISPIF_OFF_IMMEDIATELY));
		break;
	default:
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
			PIX_0, ISPIF_OFF_IMMEDIATELY));
		break;
	}

	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);

	msleep(30);
	if (update_type == MSM_SENSOR_REG_INIT) {
		s_ctrl->curr_csi_params = NULL;
		msm_sensor_enable_debugfs(s_ctrl);
		msm_sensor_write_init_settings(s_ctrl);
		s_ctrl->first_init = 1;
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		
		if(!s_ctrl->first_init)
			mdelay(50);
		s_ctrl->first_init = 0;

		msm_sensor_write_res_settings(s_ctrl, res);
		if (s_ctrl->curr_csi_params != s_ctrl->csi_params[res]) {
			s_ctrl->curr_csi_params = s_ctrl->csi_params[res];
			s_ctrl->curr_csi_params->csid_params.lane_assign =
				s_ctrl->sensordata->sensor_platform_info->
				csi_lane_params->csi_lane_assign;
			s_ctrl->curr_csi_params->csiphy_params.lane_mask =
				s_ctrl->sensordata->sensor_platform_info->
				csi_lane_params->csi_lane_mask;
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSID_CFG,
				&s_ctrl->curr_csi_params->csid_params);
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
						NOTIFY_CID_CHANGE, &s_ctrl->intf);
			mb();
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIPHY_CFG,
				&s_ctrl->curr_csi_params->csiphy_params);
			mb();
			msleep(20);
		}
#ifdef CONFIG_RAWCHIP
			if (s_ctrl->sensordata->use_rawchip) {
				rawchip_data.sensor_name = s_ctrl->sensordata->sensor_name;
				rawchip_data.datatype = s_ctrl->curr_csi_params->csid_params.lut_params.vc_cfg->dt;
				rawchip_data.lane_cnt = s_ctrl->curr_csi_params->csid_params.lane_cnt;
				rawchip_data.pixel_clk = s_ctrl->msm_sensor_reg->output_settings[res].op_pixel_clk;
				rawchip_data.mirror_flip = s_ctrl->mirror_flip;

				rawchip_data.width = s_ctrl->msm_sensor_reg->output_settings[res].x_output;
				rawchip_data.height = s_ctrl->msm_sensor_reg->output_settings[res].y_output;
				rawchip_data.line_length_pclk = s_ctrl->msm_sensor_reg->output_settings[res].line_length_pclk;
				rawchip_data.frame_length_lines = s_ctrl->msm_sensor_reg->output_settings[res].frame_length_lines;
				rawchip_data.x_addr_start = s_ctrl->msm_sensor_reg->output_settings[res].x_addr_start;
				rawchip_data.y_addr_start = s_ctrl->msm_sensor_reg->output_settings[res].y_addr_start;
				rawchip_data.x_addr_end = s_ctrl->msm_sensor_reg->output_settings[res].x_addr_end;
				rawchip_data.y_addr_end = s_ctrl->msm_sensor_reg->output_settings[res].y_addr_end;
				rawchip_data.x_even_inc = s_ctrl->msm_sensor_reg->output_settings[res].x_even_inc;
				rawchip_data.x_odd_inc = s_ctrl->msm_sensor_reg->output_settings[res].x_odd_inc;
				rawchip_data.y_even_inc = s_ctrl->msm_sensor_reg->output_settings[res].y_even_inc;
				rawchip_data.y_odd_inc = s_ctrl->msm_sensor_reg->output_settings[res].y_odd_inc;
				rawchip_data.binning_rawchip = s_ctrl->msm_sensor_reg->output_settings[res].binning_rawchip;

				rawchip_data.fullsize_width = s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].x_output;
				rawchip_data.fullsize_height = s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].y_output;
				rawchip_data.fullsize_line_length_pclk =
					s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].line_length_pclk;
				rawchip_data.fullsize_frame_length_lines =
					s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].frame_length_lines;
				rawchip_data.use_rawchip = s_ctrl->sensordata->use_rawchip;

				ktime_get_ts(&ts_start);
				rawchip_set_size(rawchip_data);
				ktime_get_ts(&ts_end);
				pr_info("%s: %ld ms\n", __func__,
					(ts_end.tv_sec-ts_start.tv_sec)*1000+(ts_end.tv_nsec-ts_start.tv_nsec)/1000000);
			}
#endif


		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_PCLK_CHANGE, &s_ctrl->msm_sensor_reg->
			output_settings[res].op_pixel_clk);
		switch (s_ctrl->intf) {
		case RDI0:
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				RDI_0, ISPIF_ON_FRAME_BOUNDARY));
			break;
		default:
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				PIX_0, ISPIF_ON_FRAME_BOUNDARY));
			break;
		}
		s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
		msleep(30);
	}
	return rc;
}

int32_t msm_sensor_setting_ov(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;

#ifdef CONFIG_RAWCHIP
	struct rawchip_sensor_data rawchip_data;
	struct timespec ts_start, ts_end;
#endif

	pr_info("%s: update_type=%d, res=%d\n", __func__, update_type, res);

	switch (s_ctrl->intf) {
	case RDI0:
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
			RDI_0, ISPIF_OFF_IMMEDIATELY));
		break;
	default:
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
			PIX_0, ISPIF_OFF_IMMEDIATELY));
		break;
	}

	msleep(30);
	if (update_type == MSM_SENSOR_REG_INIT) {
		s_ctrl->curr_csi_params = NULL;
		msm_sensor_enable_debugfs(s_ctrl);
		if (s_ctrl->sensor_id_info->sensor_id == 0x4581) {
			s_ctrl->curr_csi_params = s_ctrl->csi_params[res];
			s_ctrl->curr_csi_params->csid_params.lane_assign =
				s_ctrl->sensordata->sensor_platform_info->
				csi_lane_params->csi_lane_assign;
			s_ctrl->curr_csi_params->csiphy_params.lane_mask =
				s_ctrl->sensordata->sensor_platform_info->
				csi_lane_params->csi_lane_mask;
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSID_CFG,
				&s_ctrl->curr_csi_params->csid_params);
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
						NOTIFY_CID_CHANGE, &s_ctrl->intf);
			mb();
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIPHY_CFG,
				&s_ctrl->curr_csi_params->csiphy_params);
			mb();
			msleep(20);
		}
		msm_sensor_write_init_settings(s_ctrl);
		s_ctrl->first_init = 1;
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		
		if(!s_ctrl->first_init)
			mdelay(50);
		s_ctrl->first_init = 0;


		if (s_ctrl->curr_csi_params != s_ctrl->csi_params[res]) {
			s_ctrl->curr_csi_params = s_ctrl->csi_params[res];
			s_ctrl->curr_csi_params->csid_params.lane_assign =
				s_ctrl->sensordata->sensor_platform_info->
				csi_lane_params->csi_lane_assign;
			s_ctrl->curr_csi_params->csiphy_params.lane_mask =
				s_ctrl->sensordata->sensor_platform_info->
				csi_lane_params->csi_lane_mask;
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSID_CFG,
				&s_ctrl->curr_csi_params->csid_params);
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
						NOTIFY_CID_CHANGE, &s_ctrl->intf);
			mb();
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIPHY_CFG,
				&s_ctrl->curr_csi_params->csiphy_params);
			mb();
			msleep(20);
		}

#ifdef CONFIG_RAWCHIP
			if (s_ctrl->sensordata->use_rawchip) {
				rawchip_data.sensor_name = s_ctrl->sensordata->sensor_name;
				rawchip_data.datatype = s_ctrl->curr_csi_params->csid_params.lut_params.vc_cfg->dt;
				rawchip_data.lane_cnt = s_ctrl->curr_csi_params->csid_params.lane_cnt;
				rawchip_data.pixel_clk = s_ctrl->msm_sensor_reg->output_settings[res].op_pixel_clk;
				rawchip_data.mirror_flip = s_ctrl->mirror_flip;

				rawchip_data.width = s_ctrl->msm_sensor_reg->output_settings[res].x_output;
				rawchip_data.height = s_ctrl->msm_sensor_reg->output_settings[res].y_output;
				rawchip_data.line_length_pclk = s_ctrl->msm_sensor_reg->output_settings[res].line_length_pclk;
				rawchip_data.frame_length_lines = s_ctrl->msm_sensor_reg->output_settings[res].frame_length_lines;
				rawchip_data.x_addr_start = s_ctrl->msm_sensor_reg->output_settings[res].x_addr_start;
				rawchip_data.y_addr_start = s_ctrl->msm_sensor_reg->output_settings[res].y_addr_start;
				rawchip_data.x_addr_end = s_ctrl->msm_sensor_reg->output_settings[res].x_addr_end;
				rawchip_data.y_addr_end = s_ctrl->msm_sensor_reg->output_settings[res].y_addr_end;
				rawchip_data.x_even_inc = s_ctrl->msm_sensor_reg->output_settings[res].x_even_inc;
				rawchip_data.x_odd_inc = s_ctrl->msm_sensor_reg->output_settings[res].x_odd_inc;
				rawchip_data.y_even_inc = s_ctrl->msm_sensor_reg->output_settings[res].y_even_inc;
				rawchip_data.y_odd_inc = s_ctrl->msm_sensor_reg->output_settings[res].y_odd_inc;
				rawchip_data.binning_rawchip = s_ctrl->msm_sensor_reg->output_settings[res].binning_rawchip;

				rawchip_data.fullsize_width = s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].x_output;
				rawchip_data.fullsize_height = s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].y_output;
				rawchip_data.fullsize_line_length_pclk =
					s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].line_length_pclk;
				rawchip_data.fullsize_frame_length_lines =
					s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].frame_length_lines;
				rawchip_data.use_rawchip = s_ctrl->sensordata->use_rawchip;

				ktime_get_ts(&ts_start);
				rawchip_set_size(rawchip_data);
				ktime_get_ts(&ts_end);
				pr_info("%s: %ld ms\n", __func__,
					(ts_end.tv_sec-ts_start.tv_sec)*1000+(ts_end.tv_nsec-ts_start.tv_nsec)/1000000);
			}
#endif

		msm_sensor_write_res_settings(s_ctrl, res);

		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_PCLK_CHANGE, &s_ctrl->msm_sensor_reg->
			output_settings[res].op_pixel_clk);
		switch (s_ctrl->intf) {
		case RDI0:
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				RDI_0, ISPIF_ON_FRAME_BOUNDARY));
			break;
		default:
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				PIX_0, ISPIF_ON_FRAME_BOUNDARY));
			break;
		}
		s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
		msleep(30);
	}
	return rc;
}


static int oem_sensor_init_ov(void *arg)
{
	struct msm_sensor_ctrl_t *s_ctrl = (struct msm_sensor_ctrl_t *) arg;
	int res;
	int rc=0;

#ifdef CONFIG_RAWCHIP
	struct rawchip_sensor_data rawchip_data;
	struct timespec ts_start, ts_end;
#endif

	pr_info("%s: E", __func__);

	switch (s_ctrl->intf) {
	case RDI0:
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
			RDI_0, ISPIF_OFF_IMMEDIATELY));
		break;
	default:
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
			PIX_0, ISPIF_OFF_IMMEDIATELY));
		break;
	}
	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);

	s_ctrl->curr_csi_params = NULL;
	msm_sensor_enable_debugfs(s_ctrl);
	msm_sensor_write_init_settings(s_ctrl);
	res =0;

	if (s_ctrl->curr_csi_params != s_ctrl->csi_params[res]) {
		s_ctrl->curr_csi_params = s_ctrl->csi_params[res];
		s_ctrl->curr_csi_params->csid_params.lane_assign =
			s_ctrl->sensordata->sensor_platform_info->
			csi_lane_params->csi_lane_assign;
		s_ctrl->curr_csi_params->csiphy_params.lane_mask =
			s_ctrl->sensordata->sensor_platform_info->
			csi_lane_params->csi_lane_mask;
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_CSID_CFG,
			&s_ctrl->curr_csi_params->csid_params);
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
					NOTIFY_CID_CHANGE, &s_ctrl->intf);
		mb();
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_CSIPHY_CFG,
			&s_ctrl->curr_csi_params->csiphy_params);
		mb();
		msleep(20);
	}

#ifdef CONFIG_RAWCHIP
		if (s_ctrl->sensordata->use_rawchip) {
			rawchip_data.sensor_name = s_ctrl->sensordata->sensor_name;
			if (s_ctrl->curr_csi_params) {
				rawchip_data.datatype = s_ctrl->curr_csi_params->csid_params.lut_params.vc_cfg->dt;
				rawchip_data.lane_cnt = s_ctrl->curr_csi_params->csid_params.lane_cnt;
			}
			else {
			pr_info("%s: s_ctrl->curr_csi_params is null\n", __func__);
			}
			rawchip_data.pixel_clk = s_ctrl->msm_sensor_reg->output_settings[res].op_pixel_clk;
			rawchip_data.mirror_flip = s_ctrl->mirror_flip;

			rawchip_data.width = s_ctrl->msm_sensor_reg->output_settings[res].x_output;
			rawchip_data.height = s_ctrl->msm_sensor_reg->output_settings[res].y_output;
			rawchip_data.line_length_pclk = s_ctrl->msm_sensor_reg->output_settings[res].line_length_pclk;
			rawchip_data.frame_length_lines = s_ctrl->msm_sensor_reg->output_settings[res].frame_length_lines;
			rawchip_data.x_addr_start = s_ctrl->msm_sensor_reg->output_settings[res].x_addr_start;
			rawchip_data.y_addr_start = s_ctrl->msm_sensor_reg->output_settings[res].y_addr_start;
			rawchip_data.x_addr_end = s_ctrl->msm_sensor_reg->output_settings[res].x_addr_end;
			rawchip_data.y_addr_end = s_ctrl->msm_sensor_reg->output_settings[res].y_addr_end;
			rawchip_data.x_even_inc = s_ctrl->msm_sensor_reg->output_settings[res].x_even_inc;
			rawchip_data.x_odd_inc = s_ctrl->msm_sensor_reg->output_settings[res].x_odd_inc;
			rawchip_data.y_even_inc = s_ctrl->msm_sensor_reg->output_settings[res].y_even_inc;
			rawchip_data.y_odd_inc = s_ctrl->msm_sensor_reg->output_settings[res].y_odd_inc;
			rawchip_data.binning_rawchip = s_ctrl->msm_sensor_reg->output_settings[res].binning_rawchip;

			rawchip_data.fullsize_width = s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].x_output;
			rawchip_data.fullsize_height = s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].y_output;
			rawchip_data.fullsize_line_length_pclk =
				s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].line_length_pclk;
			rawchip_data.fullsize_frame_length_lines =
				s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].frame_length_lines;
			rawchip_data.use_rawchip = s_ctrl->sensordata->use_rawchip;

			ktime_get_ts(&ts_start);
			rawchip_set_size(rawchip_data);
			ktime_get_ts(&ts_end);
			pr_info("%s: %ld ms\n", __func__,
				(ts_end.tv_sec-ts_start.tv_sec)*1000+(ts_end.tv_nsec-ts_start.tv_nsec)/1000000);
		}
#endif

	pr_info("%s: X", __func__);
	mutex_unlock(s_ctrl->sensor_first_mutex);  

	return rc;
}


int32_t msm_sensor_setting_parallel_ov(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;

#ifdef CONFIG_RAWCHIP
	struct rawchip_sensor_data rawchip_data;
	struct timespec ts_start, ts_end;
#endif

	pr_info("%s: update_type=%d, res=%d\n", __func__, update_type, res);

	if (update_type == MSM_SENSOR_REG_INIT) {
		if (s_ctrl->first_init)   {
			pr_info("%s: MSM_SENSOR_REG_INIT already inited\n", __func__);
			return rc;
		}
		mutex_lock(s_ctrl->sensor_first_mutex);  

#ifdef CONFIG_RAWCHIPII
		if (s_ctrl->sensordata->htc_image == HTC_CAMERA_IMAGE_YUSHANII_BOARD) {
			YushanII_reload_firmware();
		}
#endif

		s_ctrl->tsk_sensor_init = kthread_create(oem_sensor_init_ov, s_ctrl, "oem_sensor_init_ov");
		if (IS_ERR(s_ctrl->tsk_sensor_init)) {
			pr_err("%s: kthread_create failed", __func__);
			rc = PTR_ERR(s_ctrl->tsk_sensor_init);
			s_ctrl->tsk_sensor_init = NULL;
			mutex_unlock(s_ctrl->sensor_first_mutex);  
		} else
			wake_up_process(s_ctrl->tsk_sensor_init);
			 

		s_ctrl->first_init = 1;
		
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
	
		mutex_lock(s_ctrl->sensor_first_mutex);

		switch (s_ctrl->intf) {
		case RDI0:
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				RDI_0, ISPIF_OFF_IMMEDIATELY));
			break;
		default:
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				PIX_0, ISPIF_OFF_IMMEDIATELY));
			break;
		}
		msleep(30);
		s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);

		
		if(!s_ctrl->first_init)
			mdelay(50);
		s_ctrl->first_init = 0;

		pr_info("%s: update_type=MSM_SENSOR_UPDATE_PERIODIC, res=%d\n", __func__, res);  
		

		msm_sensor_write_res_settings(s_ctrl, res);
		if (s_ctrl->curr_csi_params != s_ctrl->csi_params[res]) {
			s_ctrl->curr_csi_params = s_ctrl->csi_params[res];
			s_ctrl->curr_csi_params->csid_params.lane_assign =
				s_ctrl->sensordata->sensor_platform_info->
				csi_lane_params->csi_lane_assign;
			s_ctrl->curr_csi_params->csiphy_params.lane_mask =
				s_ctrl->sensordata->sensor_platform_info->
				csi_lane_params->csi_lane_mask;
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSID_CFG,
				&s_ctrl->curr_csi_params->csid_params);
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
						NOTIFY_CID_CHANGE, &s_ctrl->intf);
			mb();
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIPHY_CFG,
				&s_ctrl->curr_csi_params->csiphy_params);
			mb();
			msleep(20);
		}

#ifdef CONFIG_RAWCHIP
		if (s_ctrl->sensordata->use_rawchip) {

			pr_info("%s: use_rawchip\n", __func__);

			rawchip_data.sensor_name = s_ctrl->sensordata->sensor_name;
			rawchip_data.datatype = s_ctrl->curr_csi_params->csid_params.lut_params.vc_cfg->dt;
			rawchip_data.lane_cnt = s_ctrl->curr_csi_params->csid_params.lane_cnt;
			rawchip_data.pixel_clk = s_ctrl->msm_sensor_reg->output_settings[res].op_pixel_clk;
			rawchip_data.mirror_flip = s_ctrl->mirror_flip;

			rawchip_data.width = s_ctrl->msm_sensor_reg->output_settings[res].x_output;
			rawchip_data.height = s_ctrl->msm_sensor_reg->output_settings[res].y_output;
			rawchip_data.line_length_pclk = s_ctrl->msm_sensor_reg->output_settings[res].line_length_pclk;
			rawchip_data.frame_length_lines = s_ctrl->msm_sensor_reg->output_settings[res].frame_length_lines;
			rawchip_data.x_addr_start = s_ctrl->msm_sensor_reg->output_settings[res].x_addr_start;
			rawchip_data.y_addr_start = s_ctrl->msm_sensor_reg->output_settings[res].y_addr_start;
			rawchip_data.x_addr_end = s_ctrl->msm_sensor_reg->output_settings[res].x_addr_end;
			rawchip_data.y_addr_end = s_ctrl->msm_sensor_reg->output_settings[res].y_addr_end;
			rawchip_data.x_even_inc = s_ctrl->msm_sensor_reg->output_settings[res].x_even_inc;
			rawchip_data.x_odd_inc = s_ctrl->msm_sensor_reg->output_settings[res].x_odd_inc;
			rawchip_data.y_even_inc = s_ctrl->msm_sensor_reg->output_settings[res].y_even_inc;
			rawchip_data.y_odd_inc = s_ctrl->msm_sensor_reg->output_settings[res].y_odd_inc;
			rawchip_data.binning_rawchip = s_ctrl->msm_sensor_reg->output_settings[res].binning_rawchip;

			rawchip_data.fullsize_width = s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].x_output;
			rawchip_data.fullsize_height = s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].y_output;
			rawchip_data.fullsize_line_length_pclk =
				s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].line_length_pclk;
			rawchip_data.fullsize_frame_length_lines =
				s_ctrl->msm_sensor_reg->output_settings[MSM_SENSOR_RES_FULL].frame_length_lines;
			rawchip_data.use_rawchip = s_ctrl->sensordata->use_rawchip;

			ktime_get_ts(&ts_start);
			rawchip_set_size(rawchip_data);
			ktime_get_ts(&ts_end);
			pr_info("%s: %ld ms\n", __func__,
				(ts_end.tv_sec-ts_start.tv_sec)*1000+(ts_end.tv_nsec-ts_start.tv_nsec)/1000000);
		}
#endif

#ifdef CONFIG_RAWCHIPII
		if (s_ctrl->sensordata->htc_image == HTC_CAMERA_IMAGE_YUSHANII_BOARD) {
			if (s_ctrl->msm_sensor_reg->output_settings_yushanii)
				s_ctrl->msm_sensor_reg->output_settings = s_ctrl->msm_sensor_reg->output_settings_yushanii;
			YushanII_Init(s_ctrl,res);
		}
#endif

		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_PCLK_CHANGE, &s_ctrl->msm_sensor_reg->
			output_settings[res].op_pixel_clk);
		switch (s_ctrl->intf) {
		case RDI0:
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				RDI_0, ISPIF_ON_FRAME_BOUNDARY));
			break;
		default:
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
				PIX_0, ISPIF_ON_FRAME_BOUNDARY));
			break;
		}
		s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
		msleep(30);
		
		mutex_unlock(s_ctrl->sensor_first_mutex);
	}
	return rc;
}

int32_t msm_sensor_set_sensor_mode(struct msm_sensor_ctrl_t *s_ctrl,
	int mode, int res)
{
	int32_t rc = 0;
	pr_info("%s, res=%d, mode=%d\n", __func__, res, mode);

	if (s_ctrl->curr_res != res) {
		s_ctrl->curr_frame_length_lines =
			s_ctrl->msm_sensor_reg->
			output_settings[res].frame_length_lines;

		s_ctrl->curr_line_length_pclk =
			s_ctrl->msm_sensor_reg->
			output_settings[res].line_length_pclk;

		if (s_ctrl->func_tbl->sensor_setting
			(s_ctrl, MSM_SENSOR_UPDATE_PERIODIC, res) < 0)
			return rc;
		s_ctrl->curr_res = res;
	}

	return rc;
}

int32_t msm_sensor_mode_init(struct msm_sensor_ctrl_t *s_ctrl,
			int mode, struct sensor_init_cfg *init_info)
{
	int32_t rc = 0;
	s_ctrl->fps_divider = Q10;
	s_ctrl->cam_mode = MSM_SENSOR_MODE_INVALID;
	pr_info("%s called\n", __func__);

	if (mode != s_ctrl->cam_mode) {
		s_ctrl->curr_res = MSM_SENSOR_INVALID_RES;
		s_ctrl->cam_mode = mode;

		s_ctrl->prev_gain = 0;
		s_ctrl->prev_dig_gain = 0;
		s_ctrl->prev_line = 0;

		rc = s_ctrl->func_tbl->sensor_setting(s_ctrl,
			MSM_SENSOR_REG_INIT, 0);
	}
	return rc;
}

int32_t msm_sensor_get_output_info(struct msm_sensor_ctrl_t *s_ctrl,
		struct sensor_output_info_t *sensor_output_info)
{
	int rc = 0;
	int i=0;
	CDBG("%s: called\n", __func__);

	sensor_output_info->num_info = s_ctrl->msm_sensor_reg->num_conf;
	sensor_output_info->vert_offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	sensor_output_info->min_vert = s_ctrl->sensor_exp_gain_info->min_vert;
	sensor_output_info->mirror_flip = s_ctrl->mirror_flip;


	
	if(s_ctrl->sensor_exp_gain_info->sensor_max_linecount == 0)
		s_ctrl->sensor_exp_gain_info->sensor_max_linecount = 0xFFFFFFFF;

	sensor_output_info->sensor_max_linecount = s_ctrl->sensor_exp_gain_info->sensor_max_linecount;
	
    for (i=0;i<s_ctrl->msm_sensor_reg->num_conf;++i) {
        if (s_ctrl->adjust_y_output_size)
            s_ctrl->msm_sensor_reg->output_settings[i].y_output -= 1;
        if (s_ctrl->adjust_frame_length_line)
            s_ctrl->msm_sensor_reg->output_settings[i].line_length_pclk *= 2;
    }
	
	
	 
	if ((s_ctrl->sensordata->htc_image == HTC_CAMERA_IMAGE_YUSHANII_BOARD) && (s_ctrl->msm_sensor_reg->output_settings_yushanii)) {
		if (copy_to_user((void *)sensor_output_info->output_info,
			s_ctrl->msm_sensor_reg->output_settings_yushanii,
			sizeof(struct msm_sensor_output_info_t) *
			s_ctrl->msm_sensor_reg->num_conf))
			rc = -EFAULT;
	} else {
		if (copy_to_user((void *)sensor_output_info->output_info,
			s_ctrl->msm_sensor_reg->output_settings,
			sizeof(struct msm_sensor_output_info_t) *
			s_ctrl->msm_sensor_reg->num_conf))
			rc = -EFAULT;
	}
	
    for (i=0;i<s_ctrl->msm_sensor_reg->num_conf;++i) {
        if (s_ctrl->adjust_y_output_size)
            s_ctrl->msm_sensor_reg->output_settings[i].y_output += 1;
        if (s_ctrl->adjust_frame_length_line)
            s_ctrl->msm_sensor_reg->output_settings[i].line_length_pclk /= 2;
    }
    
	
	return rc;
}

int32_t msm_sensor_release(struct msm_sensor_ctrl_t *s_ctrl)
{
	long fps = 0;
	uint32_t delay = 0;
	pr_info("%s: called\n", __func__);

	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
	if (s_ctrl->curr_res != MSM_SENSOR_INVALID_RES) {
		fps = s_ctrl->msm_sensor_reg->
			output_settings[s_ctrl->curr_res].vt_pixel_clk /
			s_ctrl->curr_frame_length_lines /
			s_ctrl->curr_line_length_pclk;
		delay = 1000 / fps;
		CDBG("%s fps = %ld, delay = %d\n", __func__, fps, delay);
		msleep(delay);
	}
	return 0;
}

int32_t msm_sensor_interface_config(struct msm_sensor_ctrl_t *s_ctrl, void __user *argp)
{
	struct sensor_cfg_data cdata;
	long   rc = 0;

	if (copy_from_user(&cdata,
		(void *)argp,
		sizeof(struct sensor_cfg_data)))
		return -EFAULT;
	mutex_lock(s_ctrl->msm_sensor_mutex);
	CDBG("%s: msm_ispif_config: cfgtype = %d\n", __func__, cdata.cfgtype);

		switch (cdata.cfgtype) {
		
		case CFG_SET_ISP_INTERFACE:
		{
			uint8_t cur_csid = 0;
			uint8_t new_csid = s_ctrl->sensordata->pdata->csid_core;
			msm_ispif_get_input_sel_cid(cdata.cfg.intf, &cur_csid);

			pr_info("%s: intftype %d cur_csid %d new_csid %d\n",__func__,
				cdata.cfg.intf, cur_csid, new_csid);

			if ((s_ctrl->curr_csi_params) && (s_ctrl->intf != cdata.cfg.intf || cur_csid != new_csid)) {
				s_ctrl->intf = cdata.cfg.intf;

				v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
					NOTIFY_CSID_CFG,
					&s_ctrl->curr_csi_params->csid_params);
				v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
					NOTIFY_CID_CHANGE, &s_ctrl->intf);
				mb();
				v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
					NOTIFY_CSIPHY_CFG,
					&s_ctrl->curr_csi_params->csiphy_params);
				mb();
				msleep(20);
			}
		}
		break;
		case CFG_SET_STOP_STREAMING:
			pr_info("%s: CFG_SET_STOP_STREAMING\n", __func__);

			switch (s_ctrl->intf) {
			case RDI0:
				v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
					NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
					RDI_0, ISPIF_OFF_IMMEDIATELY));
				break;
			default:
				v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
					NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
					PIX_0, ISPIF_OFF_IMMEDIATELY));
				break;
			}

			break;
		case CFG_SET_START_STREAMING:
			pr_info("%s: CFG_SET_START_STREAMING\n", __func__);

			switch (s_ctrl->intf) {
			case RDI0:
				v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
					NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
					RDI_0, ISPIF_ON_FRAME_BOUNDARY));
				break;
			default:
				v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
					NOTIFY_ISPIF_STREAM, (void *)ISPIF_STREAM(
					PIX_0, ISPIF_ON_FRAME_BOUNDARY));
				break;
			}

			break;
		
		default:
			rc = -EFAULT;
			break;
		}

	mutex_unlock(s_ctrl->msm_sensor_mutex);

	return rc;
}

long msm_sensor_subdev_ioctl(struct v4l2_subdev *sd,
			unsigned int cmd, void *arg)
{
	struct msm_sensor_ctrl_t *s_ctrl = get_sctrl(sd);
	void __user *argp = (void __user *)arg;
	CDBG("%s: cmd = %d\n", __func__, cmd);

	switch (cmd) {
	case VIDIOC_MSM_SENSOR_CFG:
		return s_ctrl->func_tbl->sensor_config(s_ctrl, argp);

	case VIDIOC_MSM_SENSOR_RELEASE:
		return msm_sensor_release(s_ctrl);

	case VIDIOC_MSM_SENSOR_INTERFACE_CFG:
		return msm_sensor_interface_config(s_ctrl, argp);

	default:
		return -ENOIOCTLCMD;
	}
}

int32_t msm_sensor_config(struct msm_sensor_ctrl_t *s_ctrl, void __user *argp)
{
	struct sensor_cfg_data cdata;
	long   rc = 0;

	if (copy_from_user(&cdata,
		(void *)argp,
		sizeof(struct sensor_cfg_data)))
		return -EFAULT;
	mutex_lock(s_ctrl->msm_sensor_mutex);
	CDBG("%s: msm_sensor_config: cfgtype = %d\n", __func__, cdata.cfgtype);

		switch (cdata.cfgtype) {
		case CFG_SET_FPS:
		case CFG_SET_PICT_FPS:
			if (s_ctrl->func_tbl->
			sensor_set_fps == NULL) {
				rc = -EFAULT;
				break;
			}
			rc = s_ctrl->func_tbl->
				sensor_set_fps(
				s_ctrl,
				&(cdata.cfg.fps));
			break;

		case CFG_SET_EXP_GAIN:
			if (s_ctrl->func_tbl->
			sensor_write_exp_gain_ex == NULL) {
				rc = -EFAULT;
				break;
			}
			rc =
				s_ctrl->func_tbl->
				sensor_write_exp_gain_ex(
					s_ctrl,
					cdata.mode, 
					cdata.cfg.exp_gain.gain,
					cdata.cfg.exp_gain.dig_gain, 
					cdata.cfg.exp_gain.line);
			s_ctrl->prev_gain = cdata.cfg.exp_gain.gain;
			s_ctrl->prev_line = cdata.cfg.exp_gain.line;
			s_ctrl->prev_dig_gain= cdata.cfg.exp_gain.dig_gain;
			break;

		case CFG_SET_HDR_EXP_GAIN:
			if (s_ctrl->func_tbl->
			sensor_write_hdr_exp_gain_ex == NULL) {
				rc = -EFAULT;
				break;
			}
			rc =
				s_ctrl->func_tbl->
				sensor_write_hdr_exp_gain_ex(
					s_ctrl,
					cdata.mode, 
					cdata.cfg.exp_gain.gain,
					cdata.cfg.exp_gain.long_dig_gain,
					cdata.cfg.exp_gain.short_dig_gain,
					cdata.cfg.exp_gain.long_line,
					cdata.cfg.exp_gain.short_line);
			break;
		case CFG_SET_HDR_OUTDOOR_FLAG:
			if (s_ctrl->func_tbl->
			sensor_write_hdr_outdoor_flag == NULL) {
				rc = -EFAULT;
				break;
			}
			rc =
				s_ctrl->func_tbl->
				sensor_write_hdr_outdoor_flag(
					s_ctrl,
					cdata.cfg.exp_gain.is_outdoor);
			break;

		case CFG_SET_PICT_EXP_GAIN:
			if (s_ctrl->func_tbl->
			sensor_write_snapshot_exp_gain_ex == NULL) {
				rc = -EFAULT;
				break;
			}
			rc =
				s_ctrl->func_tbl->
				sensor_write_snapshot_exp_gain_ex(
					s_ctrl,
					cdata.mode, 
					cdata.cfg.exp_gain.gain,
					cdata.cfg.exp_gain.dig_gain, 
					cdata.cfg.exp_gain.line);
			break;

		case CFG_SET_MODE:
			if (s_ctrl->func_tbl->
			sensor_set_sensor_mode == NULL) {
				rc = -EFAULT;
				break;
			}
			rc = s_ctrl->func_tbl->
				sensor_set_sensor_mode(
					s_ctrl,
					cdata.mode,
					cdata.rs);
			break;

		case CFG_SET_EFFECT:
			break;

		case CFG_SENSOR_INIT:
			if (s_ctrl->func_tbl->
			sensor_mode_init == NULL) {
				rc = -EFAULT;
				break;
			}
			rc = s_ctrl->func_tbl->
				sensor_mode_init(
				s_ctrl,
				cdata.mode,
				&(cdata.cfg.init_info));
			break;

		case CFG_GET_OUTPUT_INFO:
			if (s_ctrl->func_tbl->
			sensor_get_output_info == NULL) {
				rc = -EFAULT;
				break;
			}
			rc = s_ctrl->func_tbl->
				sensor_get_output_info(
				s_ctrl,
				&cdata.cfg.output_info);

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_GET_EEPROM_DATA:
			if (s_ctrl->sensor_eeprom_client == NULL ||
				s_ctrl->sensor_eeprom_client->
				func_tbl.eeprom_get_data == NULL) {
				rc = -EFAULT;
				break;
			}
			rc = s_ctrl->sensor_eeprom_client->
				func_tbl.eeprom_get_data(
				s_ctrl->sensor_eeprom_client,
				&cdata.cfg.eeprom_data);

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_eeprom_data_t)))
				rc = -EFAULT;
			break;
		
		case CFG_I2C_IOCTL_R_OTP:{
			pr_info("Line:%d CFG_I2C_IOCTL_R_OTP \n", __LINE__);
			if (s_ctrl->func_tbl->sensor_i2c_read_fuseid == NULL) {
				rc = -EFAULT;
				break;
			}
			rc = s_ctrl->func_tbl->sensor_i2c_read_fuseid(&cdata, s_ctrl);
			if (copy_to_user(argp, &cdata, sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		}
		break;
		
		case CFG_SET_BLACK_LEVEL_CALIBRATION_ONGOING:
			s_ctrl->is_black_level_calibration_ongoing = TRUE;
			break;
		case CFG_SET_BLACK_LEVEL_CALIBRATION_DONE:
			s_ctrl->is_black_level_calibration_ongoing = FALSE;
			break;
		default:
			rc = -EFAULT;
			break;
		}

	mutex_unlock(s_ctrl->msm_sensor_mutex);

	return rc;
}

static struct msm_cam_clk_info cam_clk_info[] = {
	{"cam_clk", MSM_SENSOR_MCLK_24HZ},
};

int32_t msm_sensor_enable_i2c_mux(struct msm_camera_i2c_conf *i2c_conf)
{
	struct v4l2_subdev *i2c_mux_sd =
		dev_get_drvdata(&i2c_conf->mux_dev->dev);
	CDBG("%s: called\n", __func__);

	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
		VIDIOC_MSM_I2C_MUX_INIT, NULL);
	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
		VIDIOC_MSM_I2C_MUX_CFG, (void *)&i2c_conf->i2c_mux_mode);
	return 0;
}

int32_t msm_sensor_disable_i2c_mux(struct msm_camera_i2c_conf *i2c_conf)
{
	struct v4l2_subdev *i2c_mux_sd =
		dev_get_drvdata(&i2c_conf->mux_dev->dev);
	CDBG("%s: called\n", __func__);

	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
				VIDIOC_MSM_I2C_MUX_RELEASE, NULL);
	return 0;
}

int32_t msm_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	CDBG("%s: called\n", __func__);

	s_ctrl->reg_ptr = kzalloc(sizeof(struct regulator *)
			* data->sensor_platform_info->num_vreg, GFP_KERNEL);
	if (!s_ctrl->reg_ptr) {
		pr_err("%s: failed to could not allocate mem for regulators\n", __func__);
		return -ENOMEM;
	}

	rc = msm_camera_request_gpio_table(data, 1);
	if (rc < 0) {
		pr_err("%s: request gpio failed\n", __func__);
		goto request_gpio_failed;
	}

	rc = msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		pr_err("%s: regulator on failed\n", __func__);
		goto config_vreg_failed;
	}

	rc = msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		pr_err("%s: enable regulator failed\n", __func__);
		goto enable_vreg_failed;
	}

	rc = msm_camera_config_gpio_table(data, 1);
	if (rc < 0) {
		pr_err("%s: config gpio failed\n", __func__);
		goto config_gpio_failed;
	}

	if (s_ctrl->clk_rate != 0)
		cam_clk_info->clk_rate = s_ctrl->clk_rate;

	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 1);
	if (rc < 0) {
		pr_err("%s: clk enable failed\n", __func__);
		goto enable_clk_failed;
	}

	usleep_range(1000, 2000);
	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(1);

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_enable_i2c_mux(data->sensor_platform_info->i2c_conf);

	return rc;
enable_clk_failed:
		msm_camera_config_gpio_table(data, 0);
config_gpio_failed:
	msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->reg_ptr, 0);

enable_vreg_failed:
	msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
config_vreg_failed:
	msm_camera_request_gpio_table(data, 0);
request_gpio_failed:
	kfree(s_ctrl->reg_ptr);
	return rc;
}

int32_t msm_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	CDBG("%s: called %d\n", __func__, __LINE__);

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_disable_i2c_mux(
			data->sensor_platform_info->i2c_conf);

	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(0);
	msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, &s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 0);
	msm_camera_config_gpio_table(data, 0);
	msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
	msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->reg_ptr, 0);
	msm_camera_request_gpio_table(data, 0);
	kfree(s_ctrl->reg_ptr);
	return 0;
}

int32_t msm_sensor_set_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	int32_t gpio = 0;
	struct msm_camera_sensor_info *data = NULL;
	pr_info("%s: called %d\n", __func__, __LINE__);

	if (s_ctrl && s_ctrl->sensordata)
		data = s_ctrl->sensordata;
	else {
		pr_err("%s: failed, s_ctrl sensordata is NULL\n", __func__);
		return (-1);
	}

	msm_camio_clk_rate_set(MSM_SENSOR_MCLK_24HZ);

    if (!data->sensor_platform_info->board_control_reset_pin) {
        if (data->sensor_platform_info->sensor_reset_enable)
            gpio = data->sensor_platform_info->sensor_reset;
        else
            gpio = data->sensor_platform_info->sensor_pwd;

        rc = gpio_request(gpio, "SENSOR_NAME");
        if (!rc) {
            CDBG("%s: reset sensor\n", __func__);
            gpio_direction_output(gpio, 0);
            usleep_range(1000, 2000);
            gpio_set_value_cansleep(gpio, 1);
            usleep_range(4000, 5000);
        } else {
            pr_err("%s: gpio request fail", __func__);
        }
    }

    return rc;
}

int32_t msm_sensor_set_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t gpio = 0;
	struct msm_camera_sensor_info *data = NULL;
	pr_info("%s: called %d\n", __func__, __LINE__);

	if (s_ctrl && s_ctrl->sensordata)
		data = s_ctrl->sensordata;
	else {
		pr_err("%s: failed to s_ctrl sensordata NULL\n", __func__);
		return (-1);
	}
    if (!data->sensor_platform_info->board_control_reset_pin) {
    	if (data->sensor_platform_info->sensor_reset_enable)
    		gpio = data->sensor_platform_info->sensor_reset;
    	else
    		gpio = data->sensor_platform_info->sensor_pwd;

    	gpio_set_value_cansleep(gpio, 0);
    	usleep_range(1000, 2000);
    	gpio_free(gpio);
	}
	return 0;
}

int32_t msm_sensor_match_id(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	uint16_t chipid = 0;
#if defined(CONFIG_MACH_MONARUDO) || defined(CONFIG_MACH_DELUXE_J) || defined(CONFIG_MACH_DELUXE_R) || defined(CONFIG_MACH_IMPRESSION_J)\
		|| defined(CONFIG_MACH_DELUXE_U) || defined(CONFIG_MACH_DELUXE_UL) || defined(CONFIG_MACH_DELUXE_UB1)
	int i=1;
#else
	int i=10;
#endif
	CDBG("%s: called %d\n", __func__, __LINE__);

	while(i--)
	{
		rc = msm_camera_i2c_read(
				s_ctrl->sensor_i2c_client,
				s_ctrl->sensor_id_info->sensor_id_reg_addr, &chipid,
				MSM_CAMERA_I2C_WORD_DATA);

		if (rc >= 0) {
			if (chipid != s_ctrl->sensor_id_info->sensor_id) {
				pr_info("%s sensor id: 0x%X?\n", __func__, chipid);
			} else
				break;
		}

		pr_info("%s: retry %d\n",  __func__, i);

	}
	if (rc < 0) {
		pr_err("%s: read id failed\n", __func__);
		return rc;
	}

	pr_info("%s: msm_sensor id: 0x%x,expect sensor_id=0x%x\n", __func__, chipid, s_ctrl->sensor_id_info->sensor_id);
#if 1	
		if (s_ctrl->sensor_id_info->sensor_id == 0x4581)
		{
			uint16_t chipid2 = 0;
			rc = msm_camera_i2c_read(
			s_ctrl->sensor_i2c_client,
			0x3000, &chipid2,
			MSM_CAMERA_I2C_WORD_DATA);
			if (rc < 0) {
				pr_err("%s: read id failed\n", __func__);
				return rc;
			}

			pr_info("%s: msm_sensor id 0x3000: 0x%x,expect sensor_id=0x%x\n", __func__, chipid2, s_ctrl->sensor_id_info->sensor_id);
		}
#endif
	if (chipid != s_ctrl->sensor_id_info->sensor_id) {
#if defined(CONFIG_MACH_MONARUDO) || defined(CONFIG_MACH_DELUXE_J) || defined(CONFIG_MACH_DELUXE_R) || defined(CONFIG_MACH_IMPRESSION_J)\
    || defined(CONFIG_MACH_DELUXE_U) || defined(CONFIG_MACH_DELUXE_UL)
		if (chipid == 0x174 && s_ctrl->sensor_id_info->sensor_id == 0x175)
		{
			
			pr_info("%s: WA for Liteon module written wrong sensor ID as IMX174\n", __func__);
			return rc;
		}
#endif
		pr_info("%s: failed to msm_sensor_match_id chip id doesnot match\n", __func__);
		return -ENODEV;
	}
	return rc;
}

struct msm_sensor_ctrl_t *get_sctrl(struct v4l2_subdev *sd)
{
	return container_of(sd, struct msm_sensor_ctrl_t, sensor_v4l2_subdev);
}

int32_t msm_sensor_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;
	pr_info("%s: sensor_i2c_probe called - name: %s\n", __func__, client->name);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("i2c_check_functionality failed\n");
		rc = -EFAULT;
		return rc;
	}

	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
	if (s_ctrl->sensor_i2c_client != NULL) {
		s_ctrl->sensor_i2c_client->client = client;
		if (s_ctrl->sensor_i2c_addr != 0)
			s_ctrl->sensor_i2c_client->client->addr =
				s_ctrl->sensor_i2c_addr;
	} else {
		pr_err("%s: failed to sensor_i2c_client is NULL\n", __func__);
		rc = -EFAULT;
		return rc;
	}

	s_ctrl->sensordata = client->dev.platform_data;
	if (s_ctrl->sensordata == NULL) {
		pr_err("%s: failed to sensor data is NULL\n", __func__);
		return -EFAULT;
	}

	msm_camio_probe_on_bootup(s_ctrl);	

	if (s_ctrl->sensordata->use_rawchip) {
#ifdef CONFIG_RAWCHIP
		rc = rawchip_probe_init();
		if (rc < 0) {
			msm_camio_probe_on_bootup(s_ctrl);	

			pr_err("%s: rawchip probe init failed\n", __func__);
			return rc;
		}
#endif
	}

	if (board_mfg_mode () || s_ctrl->sensordata->htc_image == HTC_CAMERA_IMAGE_YUSHANII_BOARD) {
#ifdef CONFIG_RAWCHIPII
		rc = YushanII_probe_init();
		if (rc < 0) {
			pr_err("%s: rawchip probe init failed\n", __func__);
			if(s_ctrl->sensordata->camera_yushanii_probed != NULL) {
				pr_info("%s: update htc_image to 0", __func__);
				s_ctrl->sensordata->camera_yushanii_probed(HTC_CAMERA_IMAGE_NONE_BOARD);
			}
			
		} else {
			pr_info("%s rawhchip probe init success\n", __func__);
			if(s_ctrl->sensordata->camera_yushanii_probed != NULL) {
				pr_info("%s: update htc_image to 1", __func__);
				s_ctrl->sensordata->camera_yushanii_probed(HTC_CAMERA_IMAGE_YUSHANII_BOARD);
			}
		}
#endif
	}

	if (s_ctrl->func_tbl && s_ctrl->func_tbl->sensor_power_up)
		rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);

	if (rc < 0)
		goto probe_fail;

	rc = msm_sensor_match_id(s_ctrl);
	if (rc < 0)
		goto probe_fail;

	if (s_ctrl->sensor_eeprom_client != NULL) {
		struct msm_camera_eeprom_client *eeprom_client =
			s_ctrl->sensor_eeprom_client;
		if (eeprom_client->func_tbl.eeprom_init != NULL &&
			eeprom_client->func_tbl.eeprom_release != NULL) {
			rc = eeprom_client->func_tbl.eeprom_init(
				eeprom_client,
				s_ctrl->sensor_i2c_client->client->adapter);
			if (rc < 0)
				goto probe_fail;

			rc = msm_camera_eeprom_read_tbl(eeprom_client,
			eeprom_client->read_tbl, eeprom_client->read_tbl_size);
			eeprom_client->func_tbl.eeprom_release(eeprom_client);
			if (rc < 0)
				goto probe_fail;
		}
	}

	snprintf(s_ctrl->sensor_v4l2_subdev.name,
		sizeof(s_ctrl->sensor_v4l2_subdev.name), "%s", id->name);
	v4l2_i2c_subdev_init(&s_ctrl->sensor_v4l2_subdev, client,
		s_ctrl->sensor_v4l2_subdev_ops);

    if (s_ctrl->func_tbl->sensor_i2c_read_vcm_driver_ic != NULL)
      s_ctrl->func_tbl->sensor_i2c_read_vcm_driver_ic(s_ctrl);

	msm_sensor_register(&s_ctrl->sensor_v4l2_subdev);
	goto power_down;
probe_fail:
	pr_info("%s_i2c_probe failed\n", client->name);
power_down:
	if (rc > 0)
		rc = 0;

	if (s_ctrl->func_tbl && s_ctrl->func_tbl->sensor_power_down)
		s_ctrl->func_tbl->sensor_power_down(s_ctrl);

	if (s_ctrl->sensordata->use_rawchip) {
#ifdef CONFIG_RAWCHIP
		rawchip_probe_deinit();
#endif
	}

	if (s_ctrl->sensordata->htc_image == HTC_CAMERA_IMAGE_YUSHANII_BOARD) {
#ifdef CONFIG_RAWCHIPII
		YushanII_probe_deinit();
#endif
	}

	msm_camio_probe_off_bootup(s_ctrl);	

	s_ctrl->intf = PIX0; 
	s_ctrl->tsk_sensor_init = NULL;
	s_ctrl->first_init = 0;

	return rc;
}

int32_t msm_sensor_power(struct v4l2_subdev *sd, int on)
{
	int rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl = get_sctrl(sd);
	CDBG("%s: called\n", __func__);

	mutex_lock(s_ctrl->msm_sensor_mutex);
	if (on)
		rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
	else
		rc = s_ctrl->func_tbl->sensor_power_down(s_ctrl);
	mutex_unlock(s_ctrl->msm_sensor_mutex);
	return rc;
}

int32_t msm_sensor_v4l2_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			   enum v4l2_mbus_pixelcode *code)
{
	struct msm_sensor_ctrl_t *s_ctrl = get_sctrl(sd);
	CDBG("%s: called\n", __func__);

	if ((unsigned int)index >= s_ctrl->sensor_v4l2_subdev_info_size)
		return -EINVAL;

	*code = s_ctrl->sensor_v4l2_subdev_info[index].code;
	return 0;
}

int32_t msm_sensor_v4l2_s_ctrl(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	int rc = -1, i = 0;
	struct msm_sensor_ctrl_t *s_ctrl = get_sctrl(sd);
	struct msm_sensor_v4l2_ctrl_info_t *v4l2_ctrl =
		s_ctrl->msm_sensor_v4l2_ctrl_info;
	CDBG("%s: ctrl->id=%d\n", __func__, ctrl->id);

	if (v4l2_ctrl == NULL)
	{
		pr_info("%s: failed to v4l2_ctrl == NULL\n", __func__);
		return rc;
	}

	for (i = 0; i < s_ctrl->num_v4l2_ctrl; i++) {
		if (v4l2_ctrl[i].ctrl_id == ctrl->id) {
			if (v4l2_ctrl[i].s_v4l2_ctrl != NULL) {
				CDBG("\n calling msm_sensor_s_ctrl_by_enum\n");
				rc = v4l2_ctrl[i].s_v4l2_ctrl(
					s_ctrl,
					&s_ctrl->msm_sensor_v4l2_ctrl_info[i],
					ctrl->value);
			}
			break;
		}
	}

	return rc;
}

int32_t msm_sensor_v4l2_query_ctrl(
	struct v4l2_subdev *sd, struct v4l2_queryctrl *qctrl)
{
	int rc = -1, i = 0;
	struct msm_sensor_ctrl_t *s_ctrl =
		(struct msm_sensor_ctrl_t *) sd->dev_priv;
	CDBG("%s id: %d\n", __func__, qctrl->id);

	if (s_ctrl->msm_sensor_v4l2_ctrl_info == NULL)
		return rc;

	for (i = 0; i < s_ctrl->num_v4l2_ctrl; i++) {
		if (s_ctrl->msm_sensor_v4l2_ctrl_info[i].ctrl_id == qctrl->id) {
			qctrl->minimum =
				s_ctrl->msm_sensor_v4l2_ctrl_info[i].min;
			qctrl->maximum =
				s_ctrl->msm_sensor_v4l2_ctrl_info[i].max;
			qctrl->flags = 1;
			rc = 0;
			break;
		}
	}

	return rc;
}

int msm_sensor_s_ctrl_by_enum(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	CDBG("%s enter\n", __func__);

	rc = msm_sensor_write_enum_conf_array(
		s_ctrl->sensor_i2c_client,
		ctrl_info->enum_cfg_settings, value);
	return rc;
}

static int msm_sensor_debugfs_stream_s(void *data, u64 val)
{
	struct msm_sensor_ctrl_t *s_ctrl = (struct msm_sensor_ctrl_t *) data;
	CDBG("%s called\n", __func__);

	if (val)
		s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
	else
		s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(sensor_debugfs_stream, NULL,
			msm_sensor_debugfs_stream_s, "%llu\n");

static int msm_sensor_debugfs_test_s(void *data, u64 val)
{
	CDBG("%s: val: %llu\n", __func__, val);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(sensor_debugfs_test, NULL,
			msm_sensor_debugfs_test_s, "%llu\n");

int msm_sensor_enable_debugfs(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct dentry *debugfs_base, *sensor_dir;
	CDBG("%s: called\n", __func__);

	debugfs_base = debugfs_create_dir("msm_sensor", NULL);
	if (!debugfs_base)
		return -ENOMEM;

	sensor_dir = debugfs_create_dir
		(s_ctrl->sensordata->sensor_name, debugfs_base);
	if (!sensor_dir)
		return -ENOMEM;

	if (!debugfs_create_file("stream", S_IRUGO | S_IWUSR, sensor_dir,
			(void *) s_ctrl, &sensor_debugfs_stream))
		return -ENOMEM;

	if (!debugfs_create_file("test", S_IRUGO | S_IWUSR, sensor_dir,
			(void *) s_ctrl, &sensor_debugfs_test))
		return -ENOMEM;

	return 0;
}

#include <linux/fs.h>
#include <linux/file.h>
#include <linux/vmalloc.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>

void msm_fclose(struct file* file) {
    filp_close(file, NULL);
}

int msm_fwrite(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size) {
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = vfs_write(file, data, size, &offset);

    set_fs(oldfs);
    return ret;
}

struct file* msm_fopen(const char* path, int flags, int rights) {
    struct file* filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);
    if(IS_ERR(filp)) {
        err = PTR_ERR(filp);
    pr_err("[CAM]File Open Error:%s",path);
        return NULL;
    }
    if(!filp->f_op){
    pr_err("[CAM]File Operation Method Error!!");
    return NULL;
    }

    return filp;
}

void msm_read_all_otp_data(struct msm_camera_i2c_client* client,short start_address, uint8_t* buffer, size_t count)
{
    int i=0, rc=0;
    uint16_t read_data=0;

    for (i=start_address; i<start_address+count; ++i) {
        rc = msm_camera_i2c_read (client, i, &read_data, MSM_CAMERA_I2C_BYTE_DATA);
        if (rc < 0){
          pr_err("%s: msm_camera_i2c_read 0x%x failed\n", __func__, i);
        }
        *buffer++= (uint8_t)read_data;
    }
}
void msm_dump_otp_to_file (const char* sensor_name, int valid_layer, short start_address, uint8_t* buffer, size_t count)  
{  
    uint8_t *path= "/data/otp.txt";  
    struct file* f = msm_fopen (path, O_CREAT|O_RDWR|O_TRUNC, 0666);  
    char buf[512];  
    int i=0;  
    int len=0,offset=0;  
    pr_info ("%s\n",__func__);  
  
    if (f) {  
        len = sprintf (buf,"%s\n",sensor_name);  
        msm_fwrite (f,offset,buf,len);  
        offset += len;  
        len = sprintf (buf,"valid layer=%d\n",valid_layer);  
        msm_fwrite (f,offset,buf,len);  
        offset += len;  

        for (i=start_address; i<start_address+count; ++i) {
            len = sprintf (buf,"0x%x 0x%x\n",i,*buffer++);  
            msm_fwrite (f,offset,buf,len);  
            offset += len;  
        }
        msm_fclose (f);  
    } else {  
        pr_err ("%s: fail to open file\n", __func__);  
    }  
}  

void msm_read_command_line(struct msm_sensor_ctrl_t *s_ctrl)
{
	char *p;
	size_t cmdline_len;
	char *smode;
	size_t sn_len = 0;

	pr_info("%s", __func__);

	cmdline_len = strlen(saved_command_line);
	p = saved_command_line;
	for (p = saved_command_line; p < saved_command_line + cmdline_len - strlen("androidboot.serialno="); p++) {
		if (!strncmp(p, "androidboot.mode=", strlen("androidboot.mode="))) {
			p += strlen("androidboot.mode=");
			while (*p != ' '  && *p != '\0') {
				sn_len++;
				p++;
			}
			p -= sn_len;

			smode = kmalloc(sn_len + 1, GFP_KERNEL);
			strncpy(smode, p, sn_len);

			if (!strncmp(p, "normal", strlen("normal"))) {
				s_ctrl->boot_mode_normal = true;
			}

			smode[sn_len] = '\0';
			pr_info("%s:smode=%s, boot_mode_normal=%d", __func__, smode, s_ctrl->boot_mode_normal);
		}
	}
}
