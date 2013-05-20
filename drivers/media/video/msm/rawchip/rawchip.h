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
 */

#ifndef MSM_RAWCHIP_H
#define MSM_RAWCHIP_H

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/earlysuspend.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/cdev.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>

#include <mach/camera.h>


#include <mach/gpio.h>
#include <mach/vreg.h>
#include <asm/mach-types.h>
#include <mach/board.h>

#include <linux/kernel.h>
#include <linux/string.h>


#include <media/linux_rawchip.h>
#include "Yushan_API.h"
#include "Yushan_Platform_Specific.h"
#include "Yushan_HTC_Functions.h"
#include "../msm.h"

struct rawchip_ctrl {
	struct msm_camera_rawchip_info *pdata;
	struct cdev   cdev;

	struct mutex raw_ioctl_lock;
	int rawchip_init;
	atomic_t check_intr0;
	atomic_t check_intr1;

	int error_interrupt_times[TOTAL_INTERRUPT_COUNT];
	int total_error_interrupt_times;

};

struct rawchip_sensor_data {
	const char *sensor_name;
	uint8_t datatype;
	uint8_t lane_cnt;
	uint32_t pixel_clk;
	uint16_t width;
	uint16_t height;
	uint16_t line_length_pclk;
	uint16_t frame_length_lines;
	uint16_t fullsize_width;
	uint16_t fullsize_height;
	uint16_t fullsize_line_length_pclk;
	uint16_t fullsize_frame_length_lines;
	int mirror_flip;
	uint16_t x_addr_start;
	uint16_t y_addr_start;
	uint16_t x_addr_end;
	uint16_t y_addr_end;
	uint16_t x_even_inc;
	uint16_t x_odd_inc;
	uint16_t y_even_inc;
	uint16_t y_odd_inc;
	uint8_t binning_rawchip;
	uint8_t use_rawchip;
};

struct rawchip_id_info_t {
	uint16_t rawchip_id_reg_addr;
	uint32_t rawchip_id;
};

struct rawchip_info_t {
	struct rawchip_id_info_t *rawchip_id_info;
};

int rawchip_probe_init(void);
void rawchip_probe_deinit(void);
void rawchip_release(void);
int rawchip_open_init(void);
int rawchip_set_size(struct rawchip_sensor_data data);

static inline void rawchip_dump_register(void) { Yushan_dump_register(); }

#endif
