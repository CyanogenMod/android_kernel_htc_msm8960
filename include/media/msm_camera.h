/* Copyright (c) 2009-2012, Code Aurora Forum. All rights reserved.
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
#ifndef __LINUX_MSM_CAMERA_H
#define __LINUX_MSM_CAMERA_H

#ifdef MSM_CAMERA_BIONIC
#include <sys/types.h>
#endif
#include <linux/types.h>
#include <linux/ioctl.h>
#ifdef __KERNEL__
#include <linux/cdev.h>
#endif
#ifdef MSM_CAMERA_GCC
#include <time.h>
#else
#include <linux/time.h>
#endif

#include <linux/ion.h>

#define BIT(nr)   (1UL << (nr))

#define MSM_CAM_IOCTL_MAGIC 'm'

#define MSM_CAM_IOCTL_GET_SENSOR_INFO \
	_IOR(MSM_CAM_IOCTL_MAGIC, 1, struct msm_camsensor_info *)

#define MSM_CAM_IOCTL_REGISTER_PMEM \
	_IOW(MSM_CAM_IOCTL_MAGIC, 2, struct msm_pmem_info *)

#define MSM_CAM_IOCTL_UNREGISTER_PMEM \
	_IOW(MSM_CAM_IOCTL_MAGIC, 3, unsigned)

#define MSM_CAM_IOCTL_CTRL_COMMAND \
	_IOW(MSM_CAM_IOCTL_MAGIC, 4, struct msm_ctrl_cmd *)

#define MSM_CAM_IOCTL_CONFIG_VFE  \
	_IOW(MSM_CAM_IOCTL_MAGIC, 5, struct msm_camera_vfe_cfg_cmd *)

#define MSM_CAM_IOCTL_GET_STATS \
	_IOR(MSM_CAM_IOCTL_MAGIC, 6, struct msm_camera_stats_event_ctrl *)

#define MSM_CAM_IOCTL_GETFRAME \
	_IOR(MSM_CAM_IOCTL_MAGIC, 7, struct msm_camera_get_frame *)

#define MSM_CAM_IOCTL_ENABLE_VFE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 8, struct camera_enable_cmd *)

#define MSM_CAM_IOCTL_CTRL_CMD_DONE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 9, struct camera_cmd *)

#define MSM_CAM_IOCTL_CONFIG_CMD \
	_IOW(MSM_CAM_IOCTL_MAGIC, 10, struct camera_cmd *)

#define MSM_CAM_IOCTL_DISABLE_VFE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 11, struct camera_enable_cmd *)

#define MSM_CAM_IOCTL_PAD_REG_RESET2 \
	_IOW(MSM_CAM_IOCTL_MAGIC, 12, struct camera_enable_cmd *)

#define MSM_CAM_IOCTL_VFE_APPS_RESET \
	_IOW(MSM_CAM_IOCTL_MAGIC, 13, struct camera_enable_cmd *)

#define MSM_CAM_IOCTL_RELEASE_FRAME_BUFFER \
	_IOW(MSM_CAM_IOCTL_MAGIC, 14, struct camera_enable_cmd *)

#define MSM_CAM_IOCTL_RELEASE_STATS_BUFFER \
	_IOW(MSM_CAM_IOCTL_MAGIC, 15, struct msm_stats_buf *)

#define MSM_CAM_IOCTL_AXI_CONFIG \
	_IOW(MSM_CAM_IOCTL_MAGIC, 16, struct msm_camera_vfe_cfg_cmd *)

#define MSM_CAM_IOCTL_GET_PICTURE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 17, struct msm_frame *)

#define MSM_CAM_IOCTL_SET_CROP \
	_IOW(MSM_CAM_IOCTL_MAGIC, 18, struct crop_info *)

#define MSM_CAM_IOCTL_PICT_PP \
	_IOW(MSM_CAM_IOCTL_MAGIC, 19, uint8_t *)

#define MSM_CAM_IOCTL_PICT_PP_DONE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 20, struct msm_snapshot_pp_status *)

#define MSM_CAM_IOCTL_SENSOR_IO_CFG \
	_IOW(MSM_CAM_IOCTL_MAGIC, 21, struct sensor_cfg_data *)

#define MSM_CAM_IOCTL_FLASH_LED_CFG \
	_IOW(MSM_CAM_IOCTL_MAGIC, 22, unsigned *)

#define MSM_CAM_IOCTL_UNBLOCK_POLL_FRAME \
	_IO(MSM_CAM_IOCTL_MAGIC, 23)

#define MSM_CAM_IOCTL_CTRL_COMMAND_2 \
	_IOW(MSM_CAM_IOCTL_MAGIC, 24, struct msm_ctrl_cmd *)

#define MSM_CAM_IOCTL_AF_CTRL \
	_IOR(MSM_CAM_IOCTL_MAGIC, 25, struct msm_ctrl_cmt_t *)

#define MSM_CAM_IOCTL_AF_CTRL_DONE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 26, struct msm_ctrl_cmt_t *)

#define MSM_CAM_IOCTL_CONFIG_VPE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 27, struct msm_camera_vpe_cfg_cmd *)

#define MSM_CAM_IOCTL_AXI_VPE_CONFIG \
	_IOW(MSM_CAM_IOCTL_MAGIC, 28, struct msm_camera_vpe_cfg_cmd *)

#define MSM_CAM_IOCTL_STROBE_FLASH_CFG \
	_IOW(MSM_CAM_IOCTL_MAGIC, 29, uint32_t *)

#define MSM_CAM_IOCTL_STROBE_FLASH_CHARGE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 30, uint32_t *)

#define MSM_CAM_IOCTL_STROBE_FLASH_RELEASE \
	_IO(MSM_CAM_IOCTL_MAGIC, 31)

#define MSM_CAM_IOCTL_FLASH_CTRL \
	_IOW(MSM_CAM_IOCTL_MAGIC, 32, struct flash_ctrl_data *)

#define MSM_CAM_IOCTL_ERROR_CONFIG \
	_IOW(MSM_CAM_IOCTL_MAGIC, 33, uint32_t *)

#define MSM_CAM_IOCTL_ABORT_CAPTURE \
	_IO(MSM_CAM_IOCTL_MAGIC, 34)

#define MSM_CAM_IOCTL_SET_FD_ROI \
	_IOW(MSM_CAM_IOCTL_MAGIC, 35, struct fd_roi_info *)

#define MSM_CAM_IOCTL_GET_CAMERA_INFO \
	_IOR(MSM_CAM_IOCTL_MAGIC, 36, struct msm_camera_info *)

#define MSM_CAM_IOCTL_UNBLOCK_POLL_PIC_FRAME \
	_IO(MSM_CAM_IOCTL_MAGIC, 37)

#define MSM_CAM_IOCTL_RELEASE_PIC_BUFFER \
	_IOW(MSM_CAM_IOCTL_MAGIC, 38, struct camera_enable_cmd *)

#define MSM_CAM_IOCTL_PUT_ST_FRAME \
	_IOW(MSM_CAM_IOCTL_MAGIC, 39, struct msm_camera_st_frame *)

#define MSM_CAM_IOCTL_V4L2_EVT_NOTIFY \
	_IOR(MSM_CAM_IOCTL_MAGIC, 40, struct v4l2_event *)

#define MSM_CAM_IOCTL_SET_MEM_MAP_INFO \
	_IOR(MSM_CAM_IOCTL_MAGIC, 41, struct msm_mem_map_info *)

#define MSM_CAM_IOCTL_ACTUATOR_IO_CFG \
	_IOW(MSM_CAM_IOCTL_MAGIC, 42, struct msm_actuator_cfg_data *)

#define MSM_CAM_IOCTL_MCTL_POST_PROC \
	_IOW(MSM_CAM_IOCTL_MAGIC, 43, struct msm_mctl_post_proc_cmd *)

#define MSM_CAM_IOCTL_RESERVE_FREE_FRAME \
	_IOW(MSM_CAM_IOCTL_MAGIC, 44, struct msm_cam_evt_divert_frame *)

#define MSM_CAM_IOCTL_RELEASE_FREE_FRAME \
	_IOR(MSM_CAM_IOCTL_MAGIC, 45, struct msm_cam_evt_divert_frame *)

#define MSM_CAM_IOCTL_PICT_PP_DIVERT_DONE \
	_IOR(MSM_CAM_IOCTL_MAGIC, 46, struct msm_pp_frame *)

#define MSM_CAM_IOCTL_SENSOR_V4l2_S_CTRL \
	_IOR(MSM_CAM_IOCTL_MAGIC, 47, struct v4l2_control)

#define MSM_CAM_IOCTL_SENSOR_V4l2_QUERY_CTRL \
	_IOR(MSM_CAM_IOCTL_MAGIC, 48, struct v4l2_queryctrl)

#define MSM_CAM_IOCTL_GET_KERNEL_SYSTEM_TIME \
	_IOW(MSM_CAM_IOCTL_MAGIC, 49, struct timeval *)

#define MSM_CAM_IOCTL_SET_VFE_OUTPUT_TYPE \
	_IOW(MSM_CAM_IOCTL_MAGIC, 50, uint32_t *)

#define MSM_CAM_IOCTL_MCTL_DIVERT_DONE \
	_IOR(MSM_CAM_IOCTL_MAGIC, 51, struct msm_cam_evt_divert_frame *)

#define MSM_CAM_IOCTL_GET_ACTUATOR_INFO \
	_IOW(MSM_CAM_IOCTL_MAGIC, 52, struct msm_actuator_cfg_data *)

#define VIDIOC_MSM_AXI_RDI_COUNT_UPDATE \
	_IOWR('V', 53, struct rdi_count_msg)

#define MSM_CAM_IOCTL_SENSOR_INTERFACE_CFG \
	_IOW(MSM_CAM_IOCTL_MAGIC, 54, struct sensor_cfg_data *)

#define QCT_IOCTL_MAX 55 

#define MSM_CAM_IOCTL_ENABLE_DROP_FRAME \
	_IOW(MSM_CAM_IOCTL_MAGIC, QCT_IOCTL_MAX+1, int *)

#define MSM_CAM_IOCTL_SET_DROP_FRAME_NUM \
	_IOW(MSM_CAM_IOCTL_MAGIC, QCT_IOCTL_MAX+2, int *)

#define MSM_CAM_IOCTL_RETURN_FREE_FRAME \
	_IOR(MSM_CAM_IOCTL_MAGIC, QCT_IOCTL_MAX+3, struct msm_cam_evt_divert_frame *)

#define MSM_CAM_IOCTL_SET_PERF_LOCK \
	_IOW(MSM_CAM_IOCTL_MAGIC, QCT_IOCTL_MAX+4, int *)

struct msm_mctl_pp_cmd {
	int32_t  id;
	uint16_t length;
	void     *value;
};

struct msm_mctl_post_proc_cmd {
	int32_t type;
	struct msm_mctl_pp_cmd cmd;
};

#define MSM_CAMERA_LED_OFF  0
#define MSM_CAMERA_LED_LOW  1
#define MSM_CAMERA_LED_HIGH 2
#define MSM_CAMERA_LED_INIT 3
#define MSM_CAMERA_LED_RELEASE 4
#define MSM_CAMERA_LED_VIDEO 30

#define MSM_CAMERA_STROBE_FLASH_NONE 0
#define MSM_CAMERA_STROBE_FLASH_XENON 1

#define MSM_MAX_CAMERA_SENSORS  5
#define MAX_SENSOR_NAME 32
#define MAX_CAM_NAME_SIZE 32
#define MAX_ACT_MOD_NAME_SIZE 32
#define MAX_ACT_NAME_SIZE 32
#define NUM_ACTUATOR_DIR 2
#define MAX_ACTUATOR_SCENARIO 8
#define MAX_ACTUATOR_REGION 5
#define MAX_ACTUATOR_INIT_SET 12
#define MAX_ACTUATOR_TYPE_SIZE 32
#define MAX_ACTUATOR_REG_TBL_SIZE 8

#define MSM_MAX_CAMERA_CONFIGS 2

#define PP_SNAP  0x01
#define PP_RAW_SNAP ((0x01)<<1)
#define PP_PREV  ((0x01)<<2)
#define PP_THUMB ((0x01)<<3)
#define PP_MASK		(PP_SNAP|PP_RAW_SNAP|PP_PREV|PP_THUMB)

#define MSM_CAM_CTRL_CMD_DONE  0
#define MSM_CAM_SENSOR_VFE_CMD 1

#define MAX_PLANES 8


struct msm_ctrl_cmd {
	uint16_t type;
	uint16_t length;
	void *value;
	uint16_t status;
	uint32_t timeout_ms;
	int resp_fd; 
	int vnode_id;  
	int queue_idx;
	uint32_t evt_id;
	uint32_t stream_type; 
	int config_ident; 
};

struct msm_cam_evt_msg {
	unsigned short type;	
	unsigned short msg_id;
	unsigned int len;	
	uint32_t frame_id;
	void *data;
	struct timespec timestamp;
};

struct msm_pp_frame_sp {
	
	unsigned long  phy_addr;
	uint32_t       y_off;
	uint32_t       cbcr_off;
	
	uint32_t       length;
	int32_t        fd;
	uint32_t       addr_offset;
	
	unsigned long  vaddr;
};

struct msm_pp_frame_mp {
	
	unsigned long  phy_addr;
	
	uint32_t       data_offset;
	
	uint32_t       length;
	int32_t        fd;
	uint32_t       addr_offset;
	
	unsigned long  vaddr;
};

struct msm_pp_frame {
	uint32_t       handle; 
	uint32_t       frame_id;
	unsigned short buf_idx;
	int            path;
	unsigned short image_type;
	unsigned short num_planes; 
	struct timeval timestamp;
	union {
		struct msm_pp_frame_sp sp;
		struct msm_pp_frame_mp mp[MAX_PLANES];
	};
	int node_type;
};

struct msm_cam_evt_divert_frame {
	unsigned short image_mode;
	unsigned short op_mode;
	unsigned short inst_idx;
	unsigned short node_idx;
	struct msm_pp_frame frame;
	int            do_pp;
};

struct msm_mctl_pp_cmd_ack_event {
	uint32_t cmd;        
	int      status;     
	uint32_t cookie;     
};

struct msm_mctl_pp_event_info {
	int32_t  event;
	union {
		struct msm_mctl_pp_cmd_ack_event ack;
	};
};

struct msm_isp_event_ctrl {
	unsigned short resptype;
	union {
		struct msm_cam_evt_msg isp_msg;
		struct msm_ctrl_cmd ctrl;
		struct msm_cam_evt_divert_frame div_frame;
		struct msm_mctl_pp_event_info pp_event_info;
	} isp_data;
};

#define MSM_CAM_RESP_CTRL              0
#define MSM_CAM_RESP_STAT_EVT_MSG      1
#define MSM_CAM_RESP_STEREO_OP_1       2
#define MSM_CAM_RESP_STEREO_OP_2       3
#define MSM_CAM_RESP_V4L2              4
#define MSM_CAM_RESP_DIV_FRAME_EVT_MSG 5
#define MSM_CAM_RESP_DONE_EVENT        6
#define MSM_CAM_RESP_MCTL_PP_EVENT     7
#define MSM_CAM_RESP_MAX               8

#define MSM_CAM_APP_NOTIFY_EVENT  0
#define MSM_CAM_APP_NOTIFY_ERROR_EVENT  1


struct msm_stats_event_ctrl {
	int resptype;
	int timeout_ms;
	struct msm_ctrl_cmd ctrl_cmd;
	
	struct msm_cam_evt_msg stats_event;
};

struct msm_camera_cfg_cmd {
	uint16_t cfg_type;

	
	uint16_t cmd_type;
	uint16_t queue;
	uint16_t length;
	void *value;
};

#define CMD_GENERAL			0
#define CMD_AXI_CFG_OUT1		1
#define CMD_AXI_CFG_SNAP_O1_AND_O2	2
#define CMD_AXI_CFG_OUT2		3
#define CMD_PICT_T_AXI_CFG		4
#define CMD_PICT_M_AXI_CFG		5
#define CMD_RAW_PICT_AXI_CFG		6

#define CMD_FRAME_BUF_RELEASE		7
#define CMD_PREV_BUF_CFG		8
#define CMD_SNAP_BUF_RELEASE		9
#define CMD_SNAP_BUF_CFG		10
#define CMD_STATS_DISABLE		11
#define CMD_STATS_AEC_AWB_ENABLE	12
#define CMD_STATS_AF_ENABLE		13
#define CMD_STATS_AEC_ENABLE		14
#define CMD_STATS_AWB_ENABLE		15
#define CMD_STATS_ENABLE  		16

#define CMD_STATS_AXI_CFG		17
#define CMD_STATS_AEC_AXI_CFG		18
#define CMD_STATS_AF_AXI_CFG 		19
#define CMD_STATS_AWB_AXI_CFG		20
#define CMD_STATS_RS_AXI_CFG		21
#define CMD_STATS_CS_AXI_CFG		22
#define CMD_STATS_IHIST_AXI_CFG		23
#define CMD_STATS_SKIN_AXI_CFG		24

#define CMD_STATS_BUF_RELEASE		25
#define CMD_STATS_AEC_BUF_RELEASE	26
#define CMD_STATS_AF_BUF_RELEASE	27
#define CMD_STATS_AWB_BUF_RELEASE	28
#define CMD_STATS_RS_BUF_RELEASE	29
#define CMD_STATS_CS_BUF_RELEASE	30
#define CMD_STATS_IHIST_BUF_RELEASE	31
#define CMD_STATS_SKIN_BUF_RELEASE	32

#define UPDATE_STATS_INVALID		33
#define CMD_AXI_CFG_SNAP_GEMINI		34
#define CMD_AXI_CFG_SNAP		35
#define CMD_AXI_CFG_PREVIEW		36
#define CMD_AXI_CFG_VIDEO		37

#define CMD_STATS_IHIST_ENABLE 38
#define CMD_STATS_RS_ENABLE 39
#define CMD_STATS_CS_ENABLE 40
#define CMD_VPE 41
#define CMD_AXI_CFG_VPE 42
#define CMD_AXI_CFG_ZSL 43
#define CMD_AXI_CFG_SNAP_VPE 44
#define CMD_AXI_CFG_SNAP_THUMB_VPE 45

#define CMD_CONFIG_PING_ADDR 46
#define CMD_CONFIG_PONG_ADDR 47
#define CMD_CONFIG_FREE_BUF_ADDR 48
#define CMD_AXI_CFG_ZSL_ALL_CHNLS 49
#define CMD_AXI_CFG_VIDEO_ALL_CHNLS 50
#define CMD_VFE_BUFFER_RELEASE 51
#define CMD_VFE_PROCESS_IRQ 52

#define CMD_AXI_CFG_PRIM		BIT(8)
#define CMD_AXI_CFG_PRIM_ALL_CHNLS	BIT(9)
#define CMD_AXI_CFG_SEC			BIT(10)
#define CMD_AXI_CFG_SEC_ALL_CHNLS	BIT(11)
#define CMD_AXI_CFG_TERT1		BIT(12)

#define CMD_STATS_BG_ENABLE 53
#define CMD_STATS_BF_ENABLE 54
#define CMD_STATS_BHIST_ENABLE 55
#define CMD_STATS_BG_BUF_RELEASE 56
#define CMD_STATS_BF_BUF_RELEASE 57
#define CMD_STATS_BHIST_BUF_RELEASE 58

#define CMD_AXI_START  0xE1
#define CMD_AXI_STOP   0xE2

struct msm_vfe_cfg_cmd {
	int cmd_type;
	uint16_t length;
	void *value;
};

struct msm_vpe_cfg_cmd {
	int cmd_type;
	uint16_t length;
	void *value;
};

#define MAX_CAMERA_ENABLE_NAME_LEN 32
struct camera_enable_cmd {
	char name[MAX_CAMERA_ENABLE_NAME_LEN];
};

#define MSM_PMEM_OUTPUT1		0
#define MSM_PMEM_OUTPUT2		1
#define MSM_PMEM_OUTPUT1_OUTPUT2	2
#define MSM_PMEM_THUMBNAIL		3
#define MSM_PMEM_MAINIMG		4
#define MSM_PMEM_RAW_MAINIMG		5
#define MSM_PMEM_AEC_AWB		6
#define MSM_PMEM_AF			7
#define MSM_PMEM_AEC			8
#define MSM_PMEM_AWB			9
#define MSM_PMEM_RS			10
#define MSM_PMEM_CS			11
#define MSM_PMEM_IHIST			12
#define MSM_PMEM_SKIN			13
#define MSM_PMEM_VIDEO			14
#define MSM_PMEM_PREVIEW		15
#define MSM_PMEM_VIDEO_VPE		16
#define MSM_PMEM_C2D			17
#define MSM_PMEM_MAINIMG_VPE    18
#define MSM_PMEM_THUMBNAIL_VPE  19
#define MSM_PMEM_BAYER_GRID		20
#define MSM_PMEM_BAYER_FOCUS	21
#define MSM_PMEM_BAYER_HIST		22
#define MSM_PMEM_MAX            23

#define STAT_AEAW			0
#define STAT_AEC			1
#define STAT_AF				2
#define STAT_AWB			3
#define STAT_RS				4
#define STAT_CS				5
#define STAT_IHIST			6
#define STAT_SKIN			7
#define STAT_BG				8
#define STAT_BF				9
#define STAT_BHIST			10
#define STAT_MAX			11

#define FRAME_PREVIEW_OUTPUT1		0
#define FRAME_PREVIEW_OUTPUT2		1
#define FRAME_SNAPSHOT			2
#define FRAME_THUMBNAIL			3
#define FRAME_RAW_SNAPSHOT		4
#define FRAME_MAX			5

struct msm_pmem_info {
	int type;
	int fd;
	void *vaddr;
	uint32_t offset;
	uint32_t len;
	uint32_t y_off;
	uint32_t cbcr_off;
	uint32_t planar0_off;
	uint32_t planar1_off;
	uint32_t planar2_off;
	uint8_t active;
};

struct outputCfg {
	uint32_t height;
	uint32_t width;

	uint32_t window_height_firstline;
	uint32_t window_height_lastline;
};

#define VIDEO_NODE 0
#define MCTL_NODE 1

#define OUTPUT_1	0
#define OUTPUT_2	1
#define OUTPUT_1_AND_2            2   
#define OUTPUT_1_AND_3            3   
#define CAMIF_TO_AXI_VIA_OUTPUT_2 4
#define OUTPUT_1_AND_CAMIF_TO_AXI_VIA_OUTPUT_2 5
#define OUTPUT_2_AND_CAMIF_TO_AXI_VIA_OUTPUT_1 6
#define OUTPUT_1_2_AND_3 7
#define OUTPUT_ALL_CHNLS 8
#define OUTPUT_VIDEO_ALL_CHNLS 9
#define OUTPUT_ZSL_ALL_CHNLS 10
#define LAST_AXI_OUTPUT_MODE_ENUM = OUTPUT_ZSL_ALL_CHNLS

#define OUTPUT_PRIM		BIT(8)
#define OUTPUT_PRIM_ALL_CHNLS	BIT(9)
#define OUTPUT_SEC		BIT(10)
#define OUTPUT_SEC_ALL_CHNLS	BIT(11)
#define OUTPUT_TERT1		BIT(12)


#define MSM_FRAME_PREV_1	0
#define MSM_FRAME_PREV_2	1
#define MSM_FRAME_ENC		2

#define OUTPUT_TYPE_P    BIT(0)
#define OUTPUT_TYPE_T    BIT(1)
#define OUTPUT_TYPE_S    BIT(2)
#define OUTPUT_TYPE_V    BIT(3)
#define OUTPUT_TYPE_L    BIT(4)
#define OUTPUT_TYPE_ST_L BIT(5)
#define OUTPUT_TYPE_ST_R BIT(6)
#define OUTPUT_TYPE_ST_D BIT(7)
#define OUTPUT_TYPE_R    BIT(8)
#define OUTPUT_TYPE_R1   BIT(9)

struct fd_roi_info {
	void *info;
	int info_len;
};

struct msm_mem_map_info {
	uint32_t cookie;
	uint32_t length;
	uint32_t mem_type;
};

#define MSM_MEM_MMAP		0
#define MSM_MEM_USERPTR		1
#define MSM_PLANE_MAX		8
#define MSM_PLANE_Y			0
#define MSM_PLANE_UV		1

struct msm_frame {
	struct timespec ts;
	int path;
	int type;
	unsigned long buffer;
	uint32_t phy_offset;
	uint32_t y_off;
	uint32_t cbcr_off;
	uint32_t planar0_off;
	uint32_t planar1_off;
	uint32_t planar2_off;
	int fd;

	void *cropinfo;
	int croplen;
	uint32_t error_code;
	struct fd_roi_info roi_info;
	uint32_t frame_id;
	int stcam_quality_ind;
	uint32_t stcam_conv_value;

	struct ion_allocation_data ion_alloc;
	struct ion_fd_data fd_data;
	int ion_dev_fd;
};

enum msm_st_frame_packing {
	SIDE_BY_SIDE_HALF,
	SIDE_BY_SIDE_FULL,
	TOP_DOWN_HALF,
	TOP_DOWN_FULL,
};

struct msm_st_crop {
	uint32_t in_w;
	uint32_t in_h;
	uint32_t out_w;
	uint32_t out_h;
};

struct msm_st_half {
	uint32_t buf_p0_off;
	uint32_t buf_p1_off;
	uint32_t buf_p0_stride;
	uint32_t buf_p1_stride;
	uint32_t pix_x_off;
	uint32_t pix_y_off;
	struct msm_st_crop stCropInfo;
};

struct msm_st_frame {
	struct msm_frame buf_info;
	int type;
	enum msm_st_frame_packing packing;
	struct msm_st_half L;
	struct msm_st_half R;
	int frame_id;
};

#define MSM_CAMERA_ERR_MASK (0xFFFFFFFF & 1)

struct stats_buff {
	unsigned long buff;
	int fd;
};

struct stats_htc_af_input {
	int preview_width;
	int preview_height;
	int roi_x;
	int roi_y;
	int roi_width;
	int roi_height;
	uint8_t af_use_sw_sharpness;
};

struct stats_htc_af_output {
	uint32_t hw_frame_id;
	uint32_t sw_frame_id;
	uint32_t actuator_frame_id;
	uint32_t sw_sharpness_value;
};

struct stats_htc_af {
	struct stats_htc_af_input af_input;
	struct stats_htc_af_output af_output;
};

struct msm_stats_buf {
	uint8_t awb_ymin;
	struct stats_buff aec;
	struct stats_buff awb;
	struct stats_buff af;
	struct stats_buff ihist;
	struct stats_buff rs;
	struct stats_buff cs;
	struct stats_buff skin;
	int type;
	uint32_t status_bits;
	unsigned long buffer;
	int fd;
	int length;
	struct ion_handle *handle;
	uint32_t frame_id;
	struct stats_htc_af htc_af_info;
};
#define MSM_V4L2_EXT_CAPTURE_MODE_DEFAULT 0
#define MSM_V4L2_EXT_CAPTURE_MODE_PREVIEW \
	(MSM_V4L2_EXT_CAPTURE_MODE_DEFAULT+1)
#define MSM_V4L2_EXT_CAPTURE_MODE_VIDEO \
	(MSM_V4L2_EXT_CAPTURE_MODE_DEFAULT+2)
#define MSM_V4L2_EXT_CAPTURE_MODE_MAIN (MSM_V4L2_EXT_CAPTURE_MODE_DEFAULT+3)
#define MSM_V4L2_EXT_CAPTURE_MODE_THUMBNAIL \
	(MSM_V4L2_EXT_CAPTURE_MODE_DEFAULT+4)
#define MSM_V4L2_EXT_CAPTURE_MODE_RAW \
	(MSM_V4L2_EXT_CAPTURE_MODE_DEFAULT+5)
#define MSM_V4L2_EXT_CAPTURE_MODE_RDI \
	(MSM_V4L2_EXT_CAPTURE_MODE_DEFAULT+6)
#define MSM_V4L2_EXT_CAPTURE_MODE_RDI1 \
	(MSM_V4L2_EXT_CAPTURE_MODE_DEFAULT+7)
#define MSM_V4L2_EXT_CAPTURE_MODE_MAX (MSM_V4L2_EXT_CAPTURE_MODE_DEFAULT+8)


#define MSM_V4L2_PID_MOTION_ISO              V4L2_CID_PRIVATE_BASE
#define MSM_V4L2_PID_EFFECT                 (V4L2_CID_PRIVATE_BASE+1)
#define MSM_V4L2_PID_HJR                    (V4L2_CID_PRIVATE_BASE+2)
#define MSM_V4L2_PID_LED_MODE               (V4L2_CID_PRIVATE_BASE+3)
#define MSM_V4L2_PID_PREP_SNAPSHOT          (V4L2_CID_PRIVATE_BASE+4)
#define MSM_V4L2_PID_EXP_METERING           (V4L2_CID_PRIVATE_BASE+5)
#define MSM_V4L2_PID_ISO                    (V4L2_CID_PRIVATE_BASE+6)
#define MSM_V4L2_PID_CAM_MODE               (V4L2_CID_PRIVATE_BASE+7)
#define MSM_V4L2_PID_LUMA_ADAPTATION	    (V4L2_CID_PRIVATE_BASE+8)
#define MSM_V4L2_PID_BEST_SHOT              (V4L2_CID_PRIVATE_BASE+9)
#define MSM_V4L2_PID_FOCUS_MODE	            (V4L2_CID_PRIVATE_BASE+10)
#define MSM_V4L2_PID_BL_DETECTION           (V4L2_CID_PRIVATE_BASE+11)
#define MSM_V4L2_PID_SNOW_DETECTION         (V4L2_CID_PRIVATE_BASE+12)
#define MSM_V4L2_PID_CTRL_CMD               (V4L2_CID_PRIVATE_BASE+13)
#define MSM_V4L2_PID_EVT_SUB_INFO           (V4L2_CID_PRIVATE_BASE+14)
#define MSM_V4L2_PID_STROBE_FLASH           (V4L2_CID_PRIVATE_BASE+15)
#define MSM_V4L2_PID_MMAP_ENTRY             (V4L2_CID_PRIVATE_BASE+16)
#define MSM_V4L2_PID_MMAP_INST              (V4L2_CID_PRIVATE_BASE+17)
#define MSM_V4L2_PID_PP_PLANE_INFO          (V4L2_CID_PRIVATE_BASE+18)
#define MSM_V4L2_PID_MAX                    MSM_V4L2_PID_PP_PLANE_INFO

#define MSM_V4L2_CAM_OP_DEFAULT         0
#define MSM_V4L2_CAM_OP_PREVIEW         (MSM_V4L2_CAM_OP_DEFAULT+1)
#define MSM_V4L2_CAM_OP_VIDEO           (MSM_V4L2_CAM_OP_DEFAULT+2)
#define MSM_V4L2_CAM_OP_CAPTURE         (MSM_V4L2_CAM_OP_DEFAULT+3)
#define MSM_V4L2_CAM_OP_ZSL             (MSM_V4L2_CAM_OP_DEFAULT+4)
#define MSM_V4L2_CAM_OP_RAW             (MSM_V4L2_CAM_OP_DEFAULT+5)
#define MSM_V4L2_CAM_OP_JPEG_CAPTURE    (MSM_V4L2_CAM_OP_DEFAULT+6)


#define MSM_V4L2_VID_CAP_TYPE	0
#define MSM_V4L2_STREAM_ON		1
#define MSM_V4L2_STREAM_OFF		2
#define MSM_V4L2_SNAPSHOT		3
#define MSM_V4L2_QUERY_CTRL		4
#define MSM_V4L2_GET_CTRL		5
#define MSM_V4L2_SET_CTRL		6
#define MSM_V4L2_QUERY			7
#define MSM_V4L2_GET_CROP		8
#define MSM_V4L2_SET_CROP		9
#define MSM_V4L2_OPEN			10
#define MSM_V4L2_CLOSE			11
#define MSM_V4L2_SET_CTRL_CMD	12
#define MSM_V4L2_EVT_SUB_MASK	13
#define MSM_V4L2_MAX			14
#define V4L2_CAMERA_EXIT		43

struct crop_info {
	void *info;
	int len;
};

struct msm_postproc {
	int ftnum;
	struct msm_frame fthumnail;
	int fmnum;
	struct msm_frame fmain;
};

struct msm_snapshot_pp_status {
	void *status;
};

#define CFG_SET_MODE			0
#define CFG_SET_EFFECT			1
#define CFG_START			2
#define CFG_PWR_UP			3
#define CFG_PWR_DOWN			4
#define CFG_WRITE_EXPOSURE_GAIN		5
#define CFG_SET_DEFAULT_FOCUS		6
#define CFG_MOVE_FOCUS			7
#define CFG_REGISTER_TO_REAL_GAIN	8
#define CFG_REAL_TO_REGISTER_GAIN	9
#define CFG_SET_FPS			10
#define CFG_SET_PICT_FPS		11
#define CFG_SET_BRIGHTNESS		12
#define CFG_SET_CONTRAST		13
#define CFG_SET_ZOOM			14
#define CFG_SET_EXPOSURE_MODE		15
#define CFG_SET_WB			16
#define CFG_SET_ANTIBANDING		17
#define CFG_SET_EXP_GAIN		18
#define CFG_SET_PICT_EXP_GAIN		19
#define CFG_SET_LENS_SHADING		20
#define CFG_GET_PICT_FPS		21
#define CFG_GET_PREV_L_PF		22
#define CFG_GET_PREV_P_PL		23
#define CFG_GET_PICT_L_PF		24
#define CFG_GET_PICT_P_PL		25
#define CFG_GET_AF_MAX_STEPS		26
#define CFG_GET_PICT_MAX_EXP_LC		27
#define CFG_SEND_WB_INFO    28
#define CFG_SENSOR_INIT    29
#define CFG_GET_3D_CALI_DATA 30
#define CFG_GET_CALIB_DATA		31
#define CFG_GET_OUTPUT_INFO		32
#define CFG_GET_EEPROM_DATA		33
#define CFG_SET_ACTUATOR_INFO		34
#define CFG_GET_ACTUATOR_INFO           35
#define CFG_SET_SATURATION            36
#define CFG_SET_SHARPNESS             37
#define CFG_SET_TOUCHAEC              38
#define CFG_SET_AUTO_FOCUS            39
#define CFG_SET_AUTOFLASH             40
#define CFG_SET_EXPOSURE_COMPENSATION 41
#define CFG_SET_ISO                   42
#if 1 
#define CFG_SET_OV_LSC_RAW_CAPTURE 43
#define CFG_SET_COORDINATE		44
#define CFG_RUN_AUTO_FOCUS		45
#define CFG_CANCEL_AUTO_FOCUS		46
#define CFG_GET_EXP_FOR_LED		47
#define CFG_UPDATE_AEC_FOR_LED		48
#define CFG_SET_FRONT_CAMERA_MODE	49
#define CFG_SET_QCT_LSC_RAW_CAPTURE 50
#define CFG_SET_QTR_SIZE_MODE		51
#define CFG_GET_AF_STATE		52
#define CFG_SET_DMODE			53
#define CFG_SET_CALIBRATION	54
#define CFG_SET_AF_MODE		55
#define CFG_GET_SP3D_L_FRAME	56
#define CFG_GET_SP3D_R_FRAME	57
#define CFG_SET_FLASHLIGHT		58
#define CFG_SET_FLASHLIGHT_EXP_DIV 59
#define CFG_GET_ISO             60
#define CFG_GET_EXP_GAIN	61
#define CFG_SET_FRAMERATE 	62
#endif 
#define CFG_GET_ACTUATOR_CURR_STEP_POS 63
#define CFG_GET_VCM_OPTIMIZED_POSITIONS 64 
#define CFG_SET_ACTUATOR_AF_ALGO		65 
#define CFG_SET_OIS_MODE                         66 
#define CFG_SET_HDR_EXP_GAIN           67 
#define CFG_UPDATE_OIS_TBL           68 
#define CFG_GET_OIS_DEBUG_INFO			69
#define CFG_GET_OIS_DEBUG_TBL			70
#define CFG_SET_ACTUATOR_AF_VALUE		71
#define CFG_SET_HDR_OUTDOOR_FLAG		72 
#define CFG_SET_OIS_CALIBRATION		73 
#define CFG_SET_VCM_CALIBRATION 74 
#define CFG_SET_ISP_INTERFACE    75
#define CFG_SET_STOP_STREAMING   76
#define CFG_SET_BLACK_LEVEL_CALIBRATION_ONGOING 77
#define CFG_SET_BLACK_LEVEL_CALIBRATION_DONE 78
#define CFG_SET_CHANNEL_OFFSET 79
#define CFG_SET_START_STREAMING   80
#define CFG_SET_AEC_WEIGHTING  81
#define CFG_SET_AE 82 
#define CFG_GET_HDR_EXP_GAIN 83 
#define CFG_MAX			        84

#define CFG_I2C_IOCTL_R_OTP 70

#define MOVE_NEAR	0
#define MOVE_FAR	1

#define SENSOR_PREVIEW_MODE		0 
#define SENSOR_SNAPSHOT_MODE		1 
#define SENSOR_RAW_SNAPSHOT_MODE	2 
#define SENSOR_HFR_60FPS_MODE 3
#define SENSOR_HFR_90FPS_MODE 4
#define SENSOR_HFR_120FPS_MODE 5
#define SENSOR_PREVIEW_MODE_WIDE 6

#define SENSOR_QTR_SIZE			0
#define SENSOR_FULL_SIZE		1
#define SENSOR_QVGA_SIZE		2
#define SENSOR_INVALID_SIZE		3

#define CAMERA_EFFECT_OFF		0
#define CAMERA_EFFECT_MONO		1
#define CAMERA_EFFECT_NEGATIVE		2
#define CAMERA_EFFECT_SOLARIZE		3
#define CAMERA_EFFECT_SEPIA		4
#define CAMERA_EFFECT_POSTERIZE		5
#define CAMERA_EFFECT_WHITEBOARD	6
#define CAMERA_EFFECT_BLACKBOARD	7
#define CAMERA_EFFECT_AQUA		8
#define CAMERA_EFFECT_EMBOSS		9
#define CAMERA_EFFECT_SKETCH		10
#define CAMERA_EFFECT_NEON		11
#define CAMERA_EFFECT_MAX		12

#define CAMERA_EFFECT_BW		10
#define CAMERA_EFFECT_BLUISH	12
#define CAMERA_EFFECT_REDDISH	13
#define CAMERA_EFFECT_GREENISH	14

#define CAMERA_ANTIBANDING_OFF		0
#define CAMERA_ANTIBANDING_50HZ		2
#define CAMERA_ANTIBANDING_60HZ		1
#define CAMERA_ANTIBANDING_AUTO		3

#define CAMERA_CONTRAST_LV0			0
#define CAMERA_CONTRAST_LV1			1
#define CAMERA_CONTRAST_LV2			2
#define CAMERA_CONTRAST_LV3			3
#define CAMERA_CONTRAST_LV4			4
#define CAMERA_CONTRAST_LV5			5
#define CAMERA_CONTRAST_LV6			6
#define CAMERA_CONTRAST_LV7			7
#define CAMERA_CONTRAST_LV8			8
#define CAMERA_CONTRAST_LV9			9

#define CAMERA_BRIGHTNESS_LV0			0
#define CAMERA_BRIGHTNESS_LV1			1
#define CAMERA_BRIGHTNESS_LV2			2
#define CAMERA_BRIGHTNESS_LV3			3
#define CAMERA_BRIGHTNESS_LV4			4
#define CAMERA_BRIGHTNESS_LV5			5
#define CAMERA_BRIGHTNESS_LV6			6
#define CAMERA_BRIGHTNESS_LV7			7
#define CAMERA_BRIGHTNESS_LV8			8


#define CAMERA_SATURATION_LV0			0
#define CAMERA_SATURATION_LV1			1
#define CAMERA_SATURATION_LV2			2
#define CAMERA_SATURATION_LV3			3
#define CAMERA_SATURATION_LV4			4
#define CAMERA_SATURATION_LV5			5
#define CAMERA_SATURATION_LV6			6
#define CAMERA_SATURATION_LV7			7
#define CAMERA_SATURATION_LV8			8

#define CAMERA_SHARPNESS_LV0		0
#define CAMERA_SHARPNESS_LV1		3
#define CAMERA_SHARPNESS_LV2		6
#define CAMERA_SHARPNESS_LV3		9
#define CAMERA_SHARPNESS_LV4		12
#define CAMERA_SHARPNESS_LV5		15
#define CAMERA_SHARPNESS_LV6		18
#define CAMERA_SHARPNESS_LV7		21
#define CAMERA_SHARPNESS_LV8		24
#define CAMERA_SHARPNESS_LV9		27
#define CAMERA_SHARPNESS_LV10		30

#define CAMERA_SETAE_AVERAGE		0
#define CAMERA_SETAE_CENWEIGHT	1

#define  CAMERA_WB_AUTO               1 
#define  CAMERA_WB_CUSTOM             2
#define  CAMERA_WB_INCANDESCENT       3
#define  CAMERA_WB_FLUORESCENT        4
#define  CAMERA_WB_DAYLIGHT           5
#define  CAMERA_WB_CLOUDY_DAYLIGHT    6
#define  CAMERA_WB_TWILIGHT           7
#define  CAMERA_WB_SHADE              8

#define CAMERA_EXPOSURE_COMPENSATION_LV0			12
#define CAMERA_EXPOSURE_COMPENSATION_LV1			6
#define CAMERA_EXPOSURE_COMPENSATION_LV2			0
#define CAMERA_EXPOSURE_COMPENSATION_LV3			-6
#define CAMERA_EXPOSURE_COMPENSATION_LV4			-12

enum msm_v4l2_saturation_level {
	MSM_V4L2_SATURATION_L0,
	MSM_V4L2_SATURATION_L1,
	MSM_V4L2_SATURATION_L2,
	MSM_V4L2_SATURATION_L3,
	MSM_V4L2_SATURATION_L4,
	MSM_V4L2_SATURATION_L5,
	MSM_V4L2_SATURATION_L6,
	MSM_V4L2_SATURATION_L7,
	MSM_V4L2_SATURATION_L8,
	MSM_V4L2_SATURATION_L9,
	MSM_V4L2_SATURATION_L10,
};
enum msm_v4l2_contrast_level {
	MSM_V4L2_CONTRAST_L0,
	MSM_V4L2_CONTRAST_L1,
	MSM_V4L2_CONTRAST_L2,
	MSM_V4L2_CONTRAST_L3,
	MSM_V4L2_CONTRAST_L4,
	MSM_V4L2_CONTRAST_L5,
	MSM_V4L2_CONTRAST_L6,
	MSM_V4L2_CONTRAST_L7,
	MSM_V4L2_CONTRAST_L8,
	MSM_V4L2_CONTRAST_L9,
	MSM_V4L2_CONTRAST_L10,
};

enum msm_v4l2_exposure_level {
	MSM_V4L2_EXPOSURE_N2,
	MSM_V4L2_EXPOSURE_N1,
	MSM_V4L2_EXPOSURE_D,
	MSM_V4L2_EXPOSURE_P1,
	MSM_V4L2_EXPOSURE_P2,
};

enum msm_v4l2_sharpness_level {
	MSM_V4L2_SHARPNESS_L0,
	MSM_V4L2_SHARPNESS_L1,
	MSM_V4L2_SHARPNESS_L2,
	MSM_V4L2_SHARPNESS_L3,
	MSM_V4L2_SHARPNESS_L4,
	MSM_V4L2_SHARPNESS_L5,
	MSM_V4L2_SHARPNESS_L6,
};

enum msm_v4l2_expo_metering_mode {
	MSM_V4L2_EXP_FRAME_AVERAGE,
	MSM_V4L2_EXP_CENTER_WEIGHTED,
	MSM_V4L2_EXP_SPOT_METERING,
};

enum msm_v4l2_iso_mode {
	MSM_V4L2_ISO_AUTO = 0,
	MSM_V4L2_ISO_DEBLUR,
	MSM_V4L2_ISO_100,
	MSM_V4L2_ISO_200,
	MSM_V4L2_ISO_400,
	MSM_V4L2_ISO_800,
	MSM_V4L2_ISO_1600,
};

enum msm_v4l2_wb_mode {
	MSM_V4L2_WB_OFF,
	MSM_V4L2_WB_AUTO ,
	MSM_V4L2_WB_CUSTOM,
	MSM_V4L2_WB_INCANDESCENT,
	MSM_V4L2_WB_FLUORESCENT,
	MSM_V4L2_WB_DAYLIGHT,
	MSM_V4L2_WB_CLOUDY_DAYLIGHT,
};

enum msm_v4l2_special_effect {
	MSM_V4L2_EFFECT_OFF,
	MSM_V4L2_EFFECT_MONO,
	MSM_V4L2_EFFECT_NEGATIVE,
	MSM_V4L2_EFFECT_SOLARIZE,
	MSM_V4L2_EFFECT_SEPIA,
	MSM_V4L2_EFFECT_POSTERAIZE,
	MSM_V4L2_EFFECT_WHITEBOARD,
	MSM_V4L2_EFFECT_BLACKBOARD,
	MSM_V4L2_EFFECT_AQUA,
	MSM_V4L2_EFFECT_EMBOSS,
	MSM_V4L2_EFFECT_SKETCH,
	MSM_V4L2_EFFECT_NEON,
	MSM_V4L2_EFFECT_MAX,
};

enum msm_v4l2_power_line_frequency {
	MSM_V4L2_POWER_LINE_OFF,
	MSM_V4L2_POWER_LINE_60HZ,
	MSM_V4L2_POWER_LINE_50HZ,
	MSM_V4L2_POWER_LINE_AUTO,
};

#define CAMERA_ISO_TYPE_AUTO           0
#define CAMEAR_ISO_TYPE_HJR            1
#define CAMEAR_ISO_TYPE_100            2
#define CAMERA_ISO_TYPE_200            3
#define CAMERA_ISO_TYPE_400            4
#define CAMEAR_ISO_TYPE_800            5
#define CAMERA_ISO_TYPE_1600           6

struct sensor_pict_fps {
	uint16_t prevfps;
	uint16_t pictfps;
};

struct exp_gain_cfg {
	uint16_t gain;
	uint32_t line;
	uint32_t long_line;
	uint32_t short_line;
	uint16_t long_dig_gain;
	uint16_t short_dig_gain;
	uint8_t is_outdoor;
	uint16_t dig_gain; 
};

struct focus_cfg {
	int32_t steps;
	int dir;
};

struct fps_cfg {
	uint16_t f_mult;
	uint16_t fps_div;
	uint32_t pict_fps_div;
};
struct wb_info_cfg {
	uint16_t red_gain;
	uint16_t green_gain;
	uint16_t blue_gain;
};
struct sensor_3d_exp_cfg {
	uint16_t gain;
	uint32_t line;
	uint16_t r_gain;
	uint16_t b_gain;
	uint16_t gr_gain;
	uint16_t gb_gain;
	uint16_t gain_adjust;
};
struct sensor_3d_cali_data_t{
	unsigned char left_p_matrix[3][4][8];
	unsigned char right_p_matrix[3][4][8];
	unsigned char square_len[8];
	unsigned char focal_len[8];
	unsigned char pixel_pitch[8];
	uint16_t left_r;
	uint16_t left_b;
	uint16_t left_gb;
	uint16_t left_af_far;
	uint16_t left_af_mid;
	uint16_t left_af_short;
	uint16_t left_af_5um;
	uint16_t left_af_50up;
	uint16_t left_af_50down;
	uint16_t right_r;
	uint16_t right_b;
	uint16_t right_gb;
	uint16_t right_af_far;
	uint16_t right_af_mid;
	uint16_t right_af_short;
	uint16_t right_af_5um;
	uint16_t right_af_50up;
	uint16_t right_af_50down;
};
struct sensor_init_cfg {
	uint8_t prev_res;
	uint8_t pict_res;
};

struct sensor_calib_data {
	
	uint16_t r_over_g;
	uint16_t b_over_g;
	uint16_t gr_over_gb;

	
	uint16_t macro_2_inf;
	uint16_t inf_2_macro;
	uint16_t stroke_amt;
	uint16_t af_pos_1m;
	uint16_t af_pos_inf;
};

enum msm_sensor_resolution_t {
	MSM_SENSOR_RES_FULL,
	MSM_SENSOR_RES_QTR,
	MSM_SENSOR_RES_VIDEO,
	MSM_SENSOR_RES_VIDEO_HFR,
	MSM_SENSOR_RES_16_9,
	MSM_SENSOR_RES_4_3,
	MSM_SENSOR_RES_VIDEO_HFR_5_3,
	MSM_SENSOR_RES_5_3,
	MSM_SENSOR_RES_ZOE,
	MSM_SENSOR_RES_VIDEO_60FPS,
	MSM_SENSOR_RES_2,
	MSM_SENSOR_RES_3,
	MSM_SENSOR_RES_4,
	MSM_SENSOR_RES_5,
	MSM_SENSOR_RES_6,
	MSM_SENSOR_RES_7,
	MSM_SENSOR_INVALID_RES,
};

struct msm_sensor_output_info_t {
	uint16_t x_output;
	uint16_t y_output;
	uint16_t line_length_pclk;
	uint16_t frame_length_lines;
	uint32_t vt_pixel_clk;
	uint32_t op_pixel_clk;
	uint16_t binning_factor;
	
	uint16_t x_addr_start;
	uint16_t y_addr_start;
	uint16_t x_addr_end;
	uint16_t y_addr_end;
	uint16_t x_even_inc;
	uint16_t x_odd_inc;
	uint16_t y_even_inc;
	uint16_t y_odd_inc;
	uint8_t binning_rawchip;
	uint8_t is_hdr;
	
	uint8_t yushan_status_line_enable;
	uint8_t yushan_status_line; 
	uint8_t yushan_sensor_status_line; 
};

struct sensor_output_info_t {
	struct msm_sensor_output_info_t *output_info;
	uint16_t num_info;
 
	uint16_t vert_offset;
	uint16_t min_vert;
	int mirror_flip;
	uint32_t sensor_max_linecount; 
};

struct sensor_eeprom_data_t {
	void *eeprom_data;
	uint16_t index;
};

struct mirror_flip {
	int32_t x_mirror;
	int32_t y_flip;
};

struct cord {
	uint32_t x;
	uint32_t y;
};

#if 1 
enum antibanding_mode{
	CAMERA_ANTI_BANDING_50HZ,
	CAMERA_ANTI_BANDING_60HZ,
	CAMERA_ANTI_BANDING_AUTO,
};

enum brightness_t{
	CAMERA_BRIGHTNESS_N3,
	CAMERA_BRIGHTNESS_N2,
	CAMERA_BRIGHTNESS_N1,
	CAMERA_BRIGHTNESS_D,
	CAMERA_BRIGHTNESS_P1,
	CAMERA_BRIGHTNESS_P2,
	CAMERA_BRIGHTNESS_P3,
	CAMERA_BRIGHTNESS_P4,
	CAMERA_BRIGHTNESS_N4,
};

enum frontcam_t{
	CAMERA_MIRROR,
	CAMERA_REVERSE,
	CAMERA_PORTRAIT_REVERSE, 
};

enum wb_mode{
	CAMERA_AWB_AUTO,
	CAMERA_AWB_CLOUDY,
	CAMERA_AWB_INDOOR_HOME,
	CAMERA_AWB_INDOOR_OFFICE,
	CAMERA_AWB_SUNNY,
};

enum iso_mode{
  CAMERA_ISO_MODE_AUTO = 0,
  CAMERA_ISO_MODE_DEBLUR,
  CAMERA_ISO_MODE_100,
  CAMERA_ISO_MODE_200,
  CAMERA_ISO_MODE_400,
  CAMERA_ISO_MODE_800,
  CAMERA_ISO_MODE_1250,
  CAMERA_ISO_MODE_1600,
  CAMERA_ISO_MODE_MAX
};

enum sharpness_mode{
	CAMERA_SHARPNESS_X0,
	CAMERA_SHARPNESS_X1,
	CAMERA_SHARPNESS_X2,
	CAMERA_SHARPNESS_X3,
	CAMERA_SHARPNESS_X4,
	CAMERA_SHARPNESS_X5,
	CAMERA_SHARPNESS_X6,
};

enum saturation_mode{
	CAMERA_SATURATION_X0,
	CAMERA_SATURATION_X05,
	CAMERA_SATURATION_X1,
	CAMERA_SATURATION_X15,
	CAMERA_SATURATION_X2,
};

enum contrast_mode{
	CAMERA_CONTRAST_N2,
	CAMERA_CONTRAST_N1,
	CAMERA_CONTRAST_D,
	CAMERA_CONTRAST_P1,
	CAMERA_CONTRAST_P2,
};

enum qtr_size_mode{
	NORMAL_QTR_SIZE_MODE,
	LARGER_QTR_SIZE_MODE,
};

enum sensor_af_mode{
	SENSOR_AF_MODE_AUTO,
	SENSOR_AF_MODE_NORMAL,
	SENSOR_AF_MODE_MACRO,
};
#endif 

struct fuse_id{
	uint32_t fuse_id_word1;
	uint32_t fuse_id_word2;
	uint32_t fuse_id_word3;
	uint32_t fuse_id_word4;
};

typedef struct{
    uint16_t min;
    uint16_t med;
    uint16_t max;
}vcm_pos;

#define PIX_0 (0x01 << 0)
#define RDI_0 (0x01 << 1)
#define PIX_1 (0x01 << 2)
#define RDI_1 (0x01 << 3)
#define RDI_2 (0x01 << 4)

enum msm_ispif_intftype {
	PIX0,
	RDI0,
	PIX1,
	RDI1,
	RDI2,
	INTF_MAX,
};

typedef struct{
	uint8_t VCM_START_MSB;
	uint8_t VCM_START_LSB;
	uint8_t AF_INF_MSB;
	uint8_t AF_INF_LSB;
	uint8_t AF_MACRO_MSB;
	uint8_t AF_MACRO_LSB;
	
	uint8_t VCM_BIAS;
	uint8_t VCM_OFFSET;
	uint8_t VCM_BOTTOM_MECH_MSB;
	uint8_t VCM_BOTTOM_MECH_LSB;
	uint8_t VCM_TOP_MECH_MSB;
	uint8_t VCM_TOP_MECH_LSB;
	uint8_t VCM_VENDOR_ID_VERSION;
	
	uint8_t VCM_VENDOR;
	uint8_t ACT_ID;
}af_value_t;

struct sensor_cfg_data {
	int cfgtype;
	int mode;
	int rs;
	uint8_t max_steps;
	int8_t sensor_ver;
	af_value_t af_value;

	union {
		int8_t ae_active;
		int8_t effect;
		uint8_t lens_shading;
		uint16_t prevl_pf;
		uint16_t prevp_pl;
		uint16_t pictl_pf;
		uint16_t pictp_pl;
		uint32_t pict_max_exp_lc;
		uint16_t p_fps;
		uint8_t iso_type;
		struct sensor_init_cfg init_info;
		struct sensor_pict_fps gfps;
		struct exp_gain_cfg exp_gain;
		struct focus_cfg focus;
		struct fps_cfg fps;
		struct wb_info_cfg wb_info;
		struct sensor_3d_exp_cfg sensor_3d_exp;
		struct sensor_calib_data calib_info;
		struct sensor_output_info_t output_info;
		struct sensor_eeprom_data_t eeprom_data;
		
		uint16_t antibanding;
		uint8_t contrast;
		uint8_t saturation;
		uint8_t sharpness;
		int8_t brightness;
		int ae_mode;
		uint8_t wb_val;
		int8_t exp_compensation;
		struct cord aec_cord;
		int is_autoflash;
		struct mirror_flip mirror_flip;

		
		
		struct fuse_id fuse;
		
		vcm_pos calib_vcm_pos; 
#if 1 
		enum antibanding_mode antibanding_value;
		enum brightness_t brightness_value;
		enum frontcam_t frontcam_value;
		enum wb_mode wb_value;
		enum iso_mode iso_value;
		enum sharpness_mode sharpness_value;
		enum saturation_mode saturation_value;
		enum contrast_mode contrast_value;
		enum qtr_size_mode qtr_size_mode_value;
		enum sensor_af_mode af_mode_value;
#endif 
		enum msm_ispif_intftype intf; 
		uint16_t aec_weighting[25];
	} cfg;
};

typedef enum {
  AF_ALGO_QCT,
  AF_ALGO_RAWCHIP,
  AF_ALGO_RAWCHIP_WITHOUT_HW,
} af_algo_t;

typedef enum {
  VFE_CAMERA_MODE_DEFAULT,
  VFE_CAMERA_MODE_ZOE,
  VFE_CAMERA_MODE_ZSL,
  VFE_CAMERA_MODE_VIDEO,
  VFE_CAMERA_MODE_VIDEO_60FPS,
  VFE_CAMERA_MODE_DUALCAM, 
  VFE_CAMERA_MODE_MAX
} vfe_camera_mode_type;

typedef enum {
  CAM_MODE_CAMERA_PREVIEW,
  CAM_MODE_VIDEO_RECORDING,
} camera_video_mode_type;

struct sensor_actuator_info_t {
  int16_t startup_mode;
  camera_video_mode_type cam_mode;
  uint32_t cur_line_cnt;
  uint32_t cur_exp_time;
  int32_t zoom_level;
  int16_t fast_reset_mode;
};

struct damping_params_t {
	uint32_t damping_step;
	uint32_t damping_delay;
	uint32_t hw_params;
};

enum actuator_type {
	ACTUATOR_VCM,
	ACTUATOR_PIEZO,
};

enum msm_actuator_data_type {
	MSM_ACTUATOR_BYTE_DATA = 1,
	MSM_ACTUATOR_WORD_DATA,
};

enum msm_actuator_addr_type {
	MSM_ACTUATOR_BYTE_ADDR = 1,
	MSM_ACTUATOR_WORD_ADDR,
};

enum msm_actuator_write_type {
	MSM_ACTUATOR_WRITE_HW_DAMP,
	MSM_ACTUATOR_WRITE_DAC,
};

struct msm_actuator_reg_params_t {
	enum msm_actuator_write_type reg_write_type;
	uint32_t hw_mask;
	uint16_t reg_addr;
	uint16_t hw_shift;
	uint16_t data_shift;
};

struct reg_settings_t {
	uint16_t reg_addr;
	uint16_t reg_data;
};

struct region_params_t {
	uint16_t step_bound[2];
	uint16_t code_per_step;
};

struct msm_actuator_move_params_t {
	int8_t dir;
	int8_t sign_dir;
	int16_t dest_step_pos;
	int32_t num_steps;
	struct damping_params_t *ringing_params;
};

struct msm_actuator_tuning_params_t {
	int16_t initial_code;
	uint16_t pwd_step;
	uint16_t region_size;
	uint32_t total_steps;
	struct region_params_t *region_params;
};

struct msm_actuator_params_t {
	enum actuator_type act_type;
	uint8_t reg_tbl_size;
	uint16_t data_size;
	uint16_t init_setting_size;
	uint32_t i2c_addr;
	enum msm_actuator_addr_type i2c_addr_type;
	enum msm_actuator_data_type i2c_data_type;
	struct msm_actuator_reg_params_t *reg_tbl_params;
	struct reg_settings_t *init_settings;
};

struct msm_actuator_set_info_t {
	uint32_t total_steps; 
	uint16_t gross_steps; 
	uint16_t fine_steps; 
	uint16_t ois_mfgtest_in_progress_reload; 
	struct msm_actuator_params_t actuator_params;
	struct msm_actuator_tuning_params_t af_tuning_params;
};

struct msm_actuator_get_info_t {
	uint32_t focal_length_num;
	uint32_t focal_length_den;
	uint32_t f_number_num;
	uint32_t f_number_den;
	uint32_t f_pix_num;
	uint32_t f_pix_den;
	uint32_t total_f_dist_num;
	uint32_t total_f_dist_den;
	uint32_t hor_view_angle_num;
	uint32_t hor_view_angle_den;
	uint32_t ver_view_angle_num;
	uint32_t ver_view_angle_den;
};

struct msm_actuator_get_ois_info_t {
	uint32_t gyro_info;
	uint8_t ois_index;
};

struct msm_actuator_get_ois_tbl_t {
	uint32_t tbl_thre[5];
	uint32_t tbl_info[9][2];
};


enum ois_cal_mode_type_t {
	OIS_CAL_MODE_READ_FIRMWARE,
	OIS_CAL_MODE_COLLECT_DATA,
	OIS_CAL_MODE_WRITE_FIRMWARE,
};

struct msm_actuator_get_ois_cal_info_t {
	
	int16_t x_offset;
	int16_t y_offset;
	int16_t temperature;
	int8_t x_slope;
	int8_t y_slope;

	
	enum ois_cal_mode_type_t ois_cal_mode;
	int16_t cal_collect_interval;
	int16_t lens_position;
	int8_t write_flash_status;
	int8_t otp_check_pass;
	int8_t cal_method;
	int8_t cal_current_point;
	int8_t cal_max_point;
	int8_t bypass_ois_cal;
};

struct msm_actuator_get_vcm_cal_info_t {
    uint8_t offset;
    uint8_t bias;
    uint16_t hall_max;
    uint16_t hall_min;
    uint8_t rc;
};

struct msm_flash_ois_cal_data_t {
	
	int16_t x_offset_sharp;
	int16_t y_offset_sharp;
	int16_t temperature_sharp;
	int8_t x_slope_sharp;
	int8_t y_slope_sharp;

	
	int16_t x_offset_htc;
	int16_t y_offset_htc;
	int16_t temperature_htc;
	int8_t x_slope_htc;
	int8_t y_slope_htc;

	
	int8_t write_sharp_data;
	int8_t write_htc_data;
};

struct msm_actuator_af_OTP_info_t {
	uint8_t VCM_OTP_Read;
	uint16_t VCM_Start;
	uint16_t VCM_Infinity;
	uint16_t VCM_Macro;
	
	uint8_t VCM_Bias;
	uint8_t VCM_Offset;
	uint16_t VCM_Bottom_Mech;
	uint16_t VCM_Top_Mech;
	uint8_t VCM_Vendor_Id_Version;
	
	uint8_t VCM_Vendor;
	uint8_t act_id;
};

enum af_camera_name {
	ACTUATOR_MAIN_CAM_0,
	ACTUATOR_MAIN_CAM_1,
	ACTUATOR_MAIN_CAM_2,
	ACTUATOR_MAIN_CAM_3,
	ACTUATOR_MAIN_CAM_4,
	ACTUATOR_MAIN_CAM_5,
	ACTUATOR_WEB_CAM_0,
	ACTUATOR_WEB_CAM_1,
	ACTUATOR_WEB_CAM_2,
};

struct msm_actuator_cfg_data {
	int cfgtype;
	uint8_t is_af_supported;
	uint8_t is_ois_supported;
    uint8_t is_cal_supported; 
	int8_t enable_focus_step_log;
	uint8_t small_step_damping;
	uint8_t medium_step_damping;
	uint8_t big_step_damping;
	uint8_t is_af_infinity_supported;
	union {
		struct msm_actuator_move_params_t move;
		struct msm_actuator_set_info_t set_info;
		struct msm_actuator_get_info_t get_info;
		enum af_camera_name cam_name;
		int16_t curr_step_pos; 
		af_algo_t af_algo; 
		int16_t ois_mode; 
		struct msm_actuator_get_ois_info_t get_ois_info;
		struct msm_actuator_get_ois_tbl_t get_ois_tbl;
		af_value_t af_value;
		struct msm_actuator_get_ois_cal_info_t get_osi_cal_info; 
		struct sensor_actuator_info_t sensor_actuator_info; 
		struct msm_actuator_get_vcm_cal_info_t get_vcm_cal_info; 
	} cfg;
};

struct sensor_large_data {
	int cfgtype;
	union {
		struct sensor_3d_cali_data_t sensor_3d_cali_data;
	} data;
};

enum sensor_type_t {
	BAYER,
	YUV,
	JPEG_SOC,
};

enum flash_type {
	LED_FLASH,
	STROBE_FLASH,
};

enum strobe_flash_ctrl_type {
	STROBE_FLASH_CTRL_INIT,
	STROBE_FLASH_CTRL_CHARGE,
	STROBE_FLASH_CTRL_RELEASE
};

struct strobe_flash_ctrl_data {
	enum strobe_flash_ctrl_type type;
	int charge_en;
};

struct msm_camera_info {
	int num_cameras;
	uint8_t has_3d_support[MSM_MAX_CAMERA_SENSORS];
	uint8_t is_internal_cam[MSM_MAX_CAMERA_SENSORS];
	uint32_t s_mount_angle[MSM_MAX_CAMERA_SENSORS];
	const char *video_dev_name[MSM_MAX_CAMERA_SENSORS];
	enum sensor_type_t sensor_type[MSM_MAX_CAMERA_SENSORS];
};

struct msm_cam_config_dev_info {
	int num_config_nodes;
	const char *config_dev_name[MSM_MAX_CAMERA_CONFIGS];
	int config_dev_id[MSM_MAX_CAMERA_CONFIGS];
};

struct msm_mctl_node_info {
	int num_mctl_nodes;
	const char *mctl_node_name[MSM_MAX_CAMERA_SENSORS];
};

struct flash_ctrl_data {
	int flashtype;
	union {
		int led_state;
		struct strobe_flash_ctrl_data strobe_ctrl;
	} ctrl_data;
};

enum htc_camera_image_type {
	HTC_CAMERA_IMAGE_NONE,
	HTC_CAMERA_IMAGE_YUSHANII,
	HTC_CAMERA_IMAGE_MAX,
};

#define GET_NAME			0
#define GET_PREVIEW_LINE_PER_FRAME	1
#define GET_PREVIEW_PIXELS_PER_LINE	2
#define GET_SNAPSHOT_LINE_PER_FRAME	3
#define GET_SNAPSHOT_PIXELS_PER_LINE	4
#define GET_SNAPSHOT_FPS		5
#define GET_SNAPSHOT_MAX_EP_LINE_CNT	6

struct msm_camsensor_info {
	char name[MAX_SENSOR_NAME];
	uint8_t flash_enabled;
	uint8_t strobe_flash_enabled;
	uint8_t actuator_enabled;
	int8_t total_steps;
	uint8_t support_3d;
	enum flash_type flashtype;
	enum sensor_type_t sensor_type;
	uint32_t pxlcode; 
	uint32_t camera_type; 
	int mount_angle;
	uint32_t max_width;
	uint32_t max_height;
	enum htc_camera_image_type htc_image;	
	uint8_t hdr_mode;	
	uint8_t use_rawchip; 
	uint8_t video_hdr_capability;
	int dual_camera; 
};

#define V4L2_SINGLE_PLANE	0
#define V4L2_MULTI_PLANE_Y	0
#define V4L2_MULTI_PLANE_CBCR	1
#define V4L2_MULTI_PLANE_CB	1
#define V4L2_MULTI_PLANE_CR	2

struct plane_data {
	int plane_id;
	uint32_t offset;
	unsigned long size;
};

struct img_plane_info {
	uint32_t width;
	uint32_t height;
	uint32_t pixelformat;
	uint8_t buffer_type; 
	uint8_t output_port;
	uint32_t ext_mode;
	uint8_t num_planes;
	struct plane_data plane[MAX_PLANES];
	uint32_t sp_y_offset;
	uint8_t vpe_can_use;
};

#define QCAMERA_NAME "qcamera"
#define QCAMERA_DEVICE_GROUP_ID 1
#define QCAMERA_VNODE_GROUP_ID 2
#define MSM_CAM_V4L2_IOCTL_GET_CAMERA_INFO \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 1, struct msm_camera_v4l2_ioctl_t *)

#define MSM_CAM_V4L2_IOCTL_GET_CONFIG_INFO \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 2, struct msm_camera_v4l2_ioctl_t *)

#define MSM_CAM_V4L2_IOCTL_GET_MCTL_INFO \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 3, struct msm_camera_v4l2_ioctl_t *)

#define MSM_CAM_V4L2_IOCTL_CTRL_CMD_DONE \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 4, struct msm_camera_v4l2_ioctl_t *)

#define MSM_CAM_V4L2_IOCTL_GET_EVENT_PAYLOAD \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 5, struct msm_camera_v4l2_ioctl_t *)

#define MSM_CAM_IOCTL_SEND_EVENT \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 6, struct v4l2_event)

struct msm_camera_v4l2_ioctl_t {
	void __user *ioctl_ptr;
};

struct msm_ver_num_info {
        uint32_t main;
        uint32_t minor;
        uint32_t rev;
};

#endif 
