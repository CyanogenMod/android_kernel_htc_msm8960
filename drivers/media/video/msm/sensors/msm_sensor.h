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

#ifndef MSM_SENSOR_H
#define MSM_SENSOR_H

#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <mach/camera.h>
#include <mach/gpio.h>
#include <media/msm_camera.h>
#include <media/v4l2-subdev.h>
#include "msm_camera_i2c.h"
#include "msm_camera_eeprom.h"
#include "../yushanII/ilp0100_ST_definitions.h"
#include <media/linux_yushanii.h>

#define Q8  0x00000100
#define Q10 0x00000400

#define MSM_SENSOR_MCLK_8HZ 8000000
#define MSM_SENSOR_MCLK_16HZ 16000000
#define MSM_SENSOR_MCLK_24HZ 24000000

extern uint32_t ois_line;

typedef enum {
	OTP_VCM_CALIBRATION = 0,
	OTP_NUM_TYPES
} OTP_DATA_TYPE;
typedef enum {
	OTP_READ_DATA_SUCCESS = 0,
	OTP_DATA_NOT_READY_ERROR,
	OTP_READ_DATA_TYPE_ERROR,
	OTP_READ_DATA_STATUS_NUM
} OTP_DATA_READ_STATUS;
int set_VCM_OTP(int type, uint8_t* data, int size);
int get_VCM_OTP(int type, uint8_t* data, int size);

enum msm_sensor_reg_update {
	
	MSM_SENSOR_REG_INIT,
	
	MSM_SENSOR_UPDATE_PERIODIC,
	
	MSM_SENSOR_UPDATE_ALL,
	
	MSM_SENSOR_UPDATE_INVALID
};

enum msm_sensor_cam_mode_t {
	MSM_SENSOR_MODE_2D_RIGHT,
	MSM_SENSOR_MODE_2D_LEFT,
	MSM_SENSOR_MODE_3D,
	MSM_SENSOR_MODE_INVALID
};

struct msm_sensor_output_reg_addr_t {
	uint16_t x_output;
	uint16_t y_output;
	uint16_t line_length_pclk;
	uint16_t frame_length_lines;
};

struct msm_sensor_id_info_t {
	uint16_t sensor_id_reg_addr;
	uint16_t sensor_id;
};

struct msm_sensor_exp_gain_info_t {
	uint16_t coarse_int_time_addr;
	uint16_t global_gain_addr;
	uint16_t vert_offset;
	uint16_t min_vert; 
	uint32_t sensor_max_linecount; 
};

struct msm_sensor_reg_t {
	enum msm_camera_i2c_data_type default_data_type;
	struct msm_camera_i2c_reg_conf *start_stream_conf;
	uint8_t start_stream_conf_size;
	struct msm_camera_i2c_reg_conf *stop_stream_conf;
	uint8_t stop_stream_conf_size;

	struct msm_camera_i2c_reg_conf *start_stream_conf_yushanii;
	uint8_t start_stream_conf_size_yushanii;
	struct msm_camera_i2c_reg_conf *stop_stream_conf_yushanii;
	uint8_t stop_stream_conf_size_yushanii;

	struct msm_camera_i2c_reg_conf *group_hold_on_conf;
	uint8_t group_hold_on_conf_size;
	struct msm_camera_i2c_reg_conf *group_hold_off_conf;
	uint8_t group_hold_off_conf_size;
	struct msm_camera_i2c_conf_array *init_settings;
	uint8_t init_size;

	struct msm_camera_i2c_conf_array *init_settings_yushanii;
	uint8_t init_size_yushanii;

	struct msm_camera_i2c_conf_array *init_settings_2;
	uint8_t init_size_2;

	struct msm_camera_i2c_conf_array *mode_settings;
	struct msm_camera_i2c_conf_array *no_effect_settings;
	struct msm_sensor_output_info_t *output_settings;
	struct msm_sensor_output_info_t *output_settings_yushanii;
	uint8_t num_conf;
};

struct v4l2_subdev_info {
	enum v4l2_mbus_pixelcode code;
	enum v4l2_colorspace colorspace;
	uint16_t fmt;
	uint16_t order;
};

struct msm_sensor_ctrl_t;

struct msm_sensor_v4l2_ctrl_info_t {
	uint32_t ctrl_id;
	int16_t min;
	int16_t max;
	int16_t step;
	struct msm_camera_i2c_enum_conf_array *enum_cfg_settings;
	int (*s_v4l2_ctrl) (struct msm_sensor_ctrl_t *,
		struct msm_sensor_v4l2_ctrl_info_t *, int);
};

struct msm_sensor_fn_t {
	void (*sensor_start_stream) (struct msm_sensor_ctrl_t *);
	void (*sensor_stop_stream) (struct msm_sensor_ctrl_t *);
	void (*sensor_group_hold_on) (struct msm_sensor_ctrl_t *);
	void (*sensor_group_hold_off) (struct msm_sensor_ctrl_t *);

	int32_t (*sensor_set_fps) (struct msm_sensor_ctrl_t *,
			struct fps_cfg *);
	int32_t (*sensor_write_exp_gain) (struct msm_sensor_ctrl_t *,
			uint16_t, uint32_t);
	int32_t (*sensor_write_snapshot_exp_gain) (struct msm_sensor_ctrl_t *,
			uint16_t, uint32_t);
	int32_t (*sensor_write_exp_gain_ex) (struct msm_sensor_ctrl_t *,
			int, uint16_t, uint16_t, uint32_t); 
	int32_t (*sensor_write_hdr_outdoor_flag) (struct msm_sensor_ctrl_t *, uint8_t); 
	int32_t (*sensor_write_hdr_exp_gain_ex) (struct msm_sensor_ctrl_t *,
			int, uint16_t, uint16_t, uint16_t, uint32_t, uint32_t); 
	int32_t (*sensor_write_snapshot_exp_gain_ex) (struct msm_sensor_ctrl_t *,
			int, uint16_t, uint16_t, uint32_t); 
	int32_t (*sensor_setting) (struct msm_sensor_ctrl_t *,
			int update_type, int rt);
	int32_t (*sensor_set_sensor_mode)
			(struct msm_sensor_ctrl_t *, int, int);
	int32_t (*sensor_mode_init) (struct msm_sensor_ctrl_t *,
		int, struct sensor_init_cfg *);
	int32_t (*sensor_get_output_info) (struct msm_sensor_ctrl_t *,
		struct sensor_output_info_t *);
	int (*sensor_config) (struct msm_sensor_ctrl_t *, void __user *);
	int (*sensor_power_down)
		(struct msm_sensor_ctrl_t *);
	int (*sensor_power_up) (struct msm_sensor_ctrl_t *);
	
	int (*sensor_i2c_read_fuseid)(struct sensor_cfg_data *cdata, struct msm_sensor_ctrl_t *s_ctrl);
	
       int (*sensor_i2c_read_vcm_driver_ic)(struct msm_sensor_ctrl_t *s_ctrl);
	int (*sensor_adjust_frame_lines)
		(struct msm_sensor_ctrl_t *s_ctrl, uint16_t res);

	int32_t (*sensor_set_dig_gain) (struct msm_sensor_ctrl_t *, uint16_t); 
	int32_t (*sensor_set_hdr_dig_gain) (struct msm_sensor_ctrl_t *, uint16_t, uint16_t); 
	
	void (*sensor_ov2722_write_exp_line) (struct msm_sensor_ctrl_t *, uint16_t); 

	int (*sensor_write_output_settings_specific)(struct msm_sensor_ctrl_t *s_ctrl, uint16_t res); 
	int (*sensor_i2c_read_otp)(struct sensor_cfg_data *cdata, struct msm_sensor_ctrl_t *s_ctrl);

	void (*sensor_yushanii_status_line_modifier2)(uint8_t*);
	void (*sensor_yushanii_status_line_modifier)(uint16_t*);
	void (*sensor_yushanii_line_length_pclk_modifier)(uint16_t*);
	void (*sensor_yushanii_status_line_outputed_modifier)(unsigned char*);
    void (*sensor_yushanII_set_output_format)(struct msm_sensor_ctrl_t *sensor,int res, Ilp0100_structFrameFormat *output_format);
	void (*sensor_yushanII_set_parm)(struct msm_sensor_ctrl_t *sensor, int res,Ilp0100_structSensorParams *YushanII_sensor);
    void (*sensor_yushanII_set_IQ)(struct msm_sensor_ctrl_t *sensor,int*,int*,int*,struct yushanii_cls*);
};

struct msm_sensor_ctrl_t {
	struct  msm_camera_sensor_info *sensordata;
	struct i2c_client *msm_sensor_client;
	struct i2c_driver *sensor_i2c_driver;
	struct msm_camera_i2c_client *sensor_i2c_client;
	uint16_t sensor_i2c_addr;

	struct msm_camera_eeprom_client *sensor_eeprom_client;

	struct msm_sensor_output_reg_addr_t *sensor_output_reg_addr;
	struct msm_sensor_id_info_t *sensor_id_info;
	struct msm_sensor_exp_gain_info_t *sensor_exp_gain_info;
	struct msm_sensor_reg_t *msm_sensor_reg;
	struct msm_sensor_v4l2_ctrl_info_t *msm_sensor_v4l2_ctrl_info;
	uint16_t num_v4l2_ctrl;

	uint16_t curr_line_length_pclk;
	uint16_t curr_frame_length_lines;
	uint16_t prev_gain;
	uint16_t prev_line;
	uint16_t prev_dig_gain;

	uint32_t fps_divider;
	enum msm_sensor_resolution_t curr_res;
	enum msm_sensor_cam_mode_t cam_mode;

	struct mutex *msm_sensor_mutex;
	struct msm_camera_csi2_params *curr_csi_params;
	struct msm_camera_csi2_params **csi_params;
	struct msm_camera_csi_params **csic_params;
	struct msm_camera_csi_params *curr_csic_params;

	struct v4l2_subdev sensor_v4l2_subdev;
	struct v4l2_subdev_info *sensor_v4l2_subdev_info;
	uint8_t sensor_v4l2_subdev_info_size;
	struct v4l2_subdev_ops *sensor_v4l2_subdev_ops;
	struct msm_sensor_fn_t *func_tbl;
	struct regulator **reg_ptr;
	struct clk *cam_clk;
	long clk_rate;
	int mirror_flip;	
	struct mutex *sensor_first_mutex;  
	int hdr_mode;
	int yushanII_switch_virtual_channel;
	int adjust_y_output_size;
	int adjust_frame_length_line;
	uint8_t driver_ic;
};

void msm_sensor_start_stream(struct msm_sensor_ctrl_t *s_ctrl);
void msm_sensor_stop_stream(struct msm_sensor_ctrl_t *s_ctrl);
void msm_sensor_group_hold_on(struct msm_sensor_ctrl_t *s_ctrl);
void msm_sensor_group_hold_off(struct msm_sensor_ctrl_t *s_ctrl);

int32_t msm_sensor_set_fps(struct msm_sensor_ctrl_t *s_ctrl,
			struct fps_cfg   *fps);
int32_t msm_sensor_write_exp_gain1(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line);
int32_t msm_sensor_write_exp_gain2(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line);
int32_t msm_sensor_write_exp_gain1_ex(struct msm_sensor_ctrl_t *s_ctrl,
		int mode, uint16_t gain, uint16_t dig_gain, uint32_t line);
int32_t msm_sensor_write_exp_gain2_ex(struct msm_sensor_ctrl_t *s_ctrl,
		int mode, uint16_t gain, uint32_t line);
int32_t msm_sensor_write_exp_gain_ov (struct msm_sensor_ctrl_t *s_ctrl,
		int mode, uint16_t gain, uint16_t dig_gain, uint32_t line); 
int32_t msm_sensor_set_sensor_mode(struct msm_sensor_ctrl_t *s_ctrl,
	int mode, int res);
int32_t msm_sensor_mode_init(struct msm_sensor_ctrl_t *s_ctrl,
			int mode, struct sensor_init_cfg *init_info);
int32_t msm_sensor_get_output_info(struct msm_sensor_ctrl_t *,
		struct sensor_output_info_t *);
int32_t msm_sensor_config(struct msm_sensor_ctrl_t *s_ctrl,
			void __user *argp);

int32_t msm_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl);
int32_t msm_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl);
#if 1	
int32_t msm_sensor_set_power_up(struct msm_sensor_ctrl_t *s_ctrl);
int32_t msm_sensor_set_power_down(struct msm_sensor_ctrl_t *s_ctrl);

int32_t msm_sensor_match_id(struct msm_sensor_ctrl_t *s_ctrl);
int msm_sensor_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id);
int32_t msm_sensor_release(struct msm_sensor_ctrl_t *s_ctrl);
int32_t msm_sensor_open_init(struct msm_sensor_ctrl_t *s_ctrl,
				const struct msm_camera_sensor_info *data);
int msm_sensor_probe(struct msm_sensor_ctrl_t *s_ctrl,
		const struct msm_camera_sensor_info *info,
		struct msm_sensor_ctrl *s);

int msm_sensor_v4l2_probe(struct msm_sensor_ctrl_t *s_ctrl,
	const struct msm_camera_sensor_info *info,
	struct v4l2_subdev *sdev, struct msm_sensor_ctrl *s);

#endif	

int32_t msm_sensor_match_id(struct msm_sensor_ctrl_t *s_ctrl);
int msm_sensor_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id);
int32_t msm_sensor_power(struct v4l2_subdev *sd, int on);

int32_t msm_sensor_v4l2_s_ctrl(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl);

int32_t msm_sensor_v4l2_query_ctrl(
	struct v4l2_subdev *sd, struct v4l2_queryctrl *qctrl);

int msm_sensor_s_ctrl_by_index(struct msm_sensor_ctrl_t *s_ctrl,
	struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value);

int msm_sensor_s_ctrl_by_enum(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value);

int msm_sensor_v4l2_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			enum v4l2_mbus_pixelcode *code);

int msm_sensor_write_init_settings(struct msm_sensor_ctrl_t *s_ctrl);
int msm_sensor_write_res_settings
	(struct msm_sensor_ctrl_t *s_ctrl, uint16_t res);

int32_t msm_sensor_write_output_settings(struct msm_sensor_ctrl_t *s_ctrl,
	uint16_t res);

int32_t msm_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res);

int32_t msm_sensor_setting_parallel(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res);
int32_t msm_sensor_setting_parallel_ov(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res);

int32_t msm_sensor_setting_ov(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res);

int32_t msm_sensor_setting1(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res);

int msm_sensor_enable_debugfs(struct msm_sensor_ctrl_t *s_ctrl);

long msm_sensor_subdev_ioctl(struct v4l2_subdev *sd,
			unsigned int cmd, void *arg);

struct msm_sensor_ctrl_t *get_sctrl(struct v4l2_subdev *sd);

#if (defined CONFIG_WEBCAM_OV7692_QRD || defined CONFIG_OV5647)
	extern int lcd_camera_power_onoff(int on);
#endif
int32_t msm_sensor_adjust_frame_lines(struct msm_sensor_ctrl_t *s_ctrl,
	uint16_t res);


#define VIDIOC_MSM_SENSOR_CFG \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 10, void __user *)

#define VIDIOC_MSM_SENSOR_RELEASE \
	_IO('V', BASE_VIDIOC_PRIVATE + 11)

#endif
