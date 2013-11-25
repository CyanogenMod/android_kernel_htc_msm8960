/*
 * include/media/v4l2-int-device.h
 *
 * V4L2 internal ioctl interface.
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Contact: Sakari Ailus <sakari.ailus@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef V4L2_INT_DEVICE_H
#define V4L2_INT_DEVICE_H

#include <linux/module.h>
#include <media/v4l2-common.h>

#define V4L2NAMESIZE 32


enum v4l2_int_type {
	v4l2_int_type_master = 1,
	v4l2_int_type_slave
};

struct v4l2_int_device;

struct v4l2_int_master {
	int (*attach)(struct v4l2_int_device *slave);
	void (*detach)(struct v4l2_int_device *slave);
};

typedef int (v4l2_int_ioctl_func)(struct v4l2_int_device *);
typedef int (v4l2_int_ioctl_func_0)(struct v4l2_int_device *);
typedef int (v4l2_int_ioctl_func_1)(struct v4l2_int_device *, void *);

struct v4l2_int_ioctl_desc {
	int num;
	v4l2_int_ioctl_func *func;
};

struct v4l2_int_slave {
	
	struct v4l2_int_device *master;

	char attach_to[V4L2NAMESIZE];

	int num_ioctls;
	struct v4l2_int_ioctl_desc *ioctls;
};

struct v4l2_int_device {
	
	struct list_head head;

	struct module *module;

	char name[V4L2NAMESIZE];

	enum v4l2_int_type type;
	union {
		struct v4l2_int_master *master;
		struct v4l2_int_slave *slave;
	} u;

	void *priv;
};

void v4l2_int_device_try_attach_all(void);

int v4l2_int_device_register(struct v4l2_int_device *d);
void v4l2_int_device_unregister(struct v4l2_int_device *d);

int v4l2_int_ioctl_0(struct v4l2_int_device *d, int cmd);
int v4l2_int_ioctl_1(struct v4l2_int_device *d, int cmd, void *arg);


enum v4l2_power {
	V4L2_POWER_OFF = 0,
	V4L2_POWER_ON,
	V4L2_POWER_STANDBY,
};

enum v4l2_if_type {
	V4L2_IF_TYPE_BT656,
};

enum v4l2_if_type_bt656_mode {
	V4L2_IF_TYPE_BT656_MODE_NOBT_8BIT,
	V4L2_IF_TYPE_BT656_MODE_NOBT_10BIT,
	V4L2_IF_TYPE_BT656_MODE_NOBT_12BIT,
	V4L2_IF_TYPE_BT656_MODE_BT_8BIT,
	V4L2_IF_TYPE_BT656_MODE_BT_10BIT,
};

struct v4l2_if_type_bt656 {
	unsigned frame_start_on_rising_vs:1;
	
	unsigned bt_sync_correct:1;
	
	unsigned swap:1;
	
	unsigned latch_clk_inv:1;
	
	unsigned nobt_hs_inv:1;
	
	unsigned nobt_vs_inv:1;
	enum v4l2_if_type_bt656_mode mode;
	
	u32 clock_min;
	
	u32 clock_max;
	u32 clock_curr;
};

struct v4l2_ifparm {
	enum v4l2_if_type if_type;
	union {
		struct v4l2_if_type_bt656 bt656;
	} u;
};

enum v4l2_int_ioctl_num {
	vidioc_int_enum_fmt_cap_num = 1,
	vidioc_int_g_fmt_cap_num,
	vidioc_int_s_fmt_cap_num,
	vidioc_int_try_fmt_cap_num,
	vidioc_int_queryctrl_num,
	vidioc_int_g_ctrl_num,
	vidioc_int_s_ctrl_num,
	vidioc_int_cropcap_num,
	vidioc_int_g_crop_num,
	vidioc_int_s_crop_num,
	vidioc_int_g_parm_num,
	vidioc_int_s_parm_num,
	vidioc_int_querystd_num,
	vidioc_int_s_std_num,
	vidioc_int_s_video_routing_num,

	
	vidioc_int_dev_init_num = 1000,
	
	vidioc_int_dev_exit_num,
	
	vidioc_int_s_power_num,
	vidioc_int_g_priv_num,
	
	vidioc_int_g_ifparm_num,
	
	vidioc_int_g_needs_reset_num,
	vidioc_int_enum_framesizes_num,
	vidioc_int_enum_frameintervals_num,

	
	vidioc_int_reset_num,
	
	vidioc_int_init_num,
	
	vidioc_int_g_chip_ident_num,

	vidioc_int_priv_start_num = 2000,
};


#define V4L2_INT_WRAPPER_0(name)					\
	static inline int vidioc_int_##name(struct v4l2_int_device *d)	\
	{								\
		return v4l2_int_ioctl_0(d, vidioc_int_##name##_num);	\
	}								\
									\
	static inline struct v4l2_int_ioctl_desc			\
	vidioc_int_##name##_cb(int (*func)				\
			       (struct v4l2_int_device *))		\
	{								\
		struct v4l2_int_ioctl_desc desc;			\
									\
		desc.num = vidioc_int_##name##_num;			\
		desc.func = (v4l2_int_ioctl_func *)func;		\
									\
		return desc;						\
	}

#define V4L2_INT_WRAPPER_1(name, arg_type, asterisk)			\
	static inline int vidioc_int_##name(struct v4l2_int_device *d,	\
					    arg_type asterisk arg)	\
	{								\
		return v4l2_int_ioctl_1(d, vidioc_int_##name##_num,	\
					(void *)(unsigned long)arg);	\
	}								\
									\
	static inline struct v4l2_int_ioctl_desc			\
	vidioc_int_##name##_cb(int (*func)				\
			       (struct v4l2_int_device *,		\
				arg_type asterisk))			\
	{								\
		struct v4l2_int_ioctl_desc desc;			\
									\
		desc.num = vidioc_int_##name##_num;			\
		desc.func = (v4l2_int_ioctl_func *)func;		\
									\
		return desc;						\
	}

V4L2_INT_WRAPPER_1(enum_fmt_cap, struct v4l2_fmtdesc, *);
V4L2_INT_WRAPPER_1(g_fmt_cap, struct v4l2_format, *);
V4L2_INT_WRAPPER_1(s_fmt_cap, struct v4l2_format, *);
V4L2_INT_WRAPPER_1(try_fmt_cap, struct v4l2_format, *);
V4L2_INT_WRAPPER_1(queryctrl, struct v4l2_queryctrl, *);
V4L2_INT_WRAPPER_1(g_ctrl, struct v4l2_control, *);
V4L2_INT_WRAPPER_1(s_ctrl, struct v4l2_control, *);
V4L2_INT_WRAPPER_1(cropcap, struct v4l2_cropcap, *);
V4L2_INT_WRAPPER_1(g_crop, struct v4l2_crop, *);
V4L2_INT_WRAPPER_1(s_crop, struct v4l2_crop, *);
V4L2_INT_WRAPPER_1(g_parm, struct v4l2_streamparm, *);
V4L2_INT_WRAPPER_1(s_parm, struct v4l2_streamparm, *);
V4L2_INT_WRAPPER_1(querystd, v4l2_std_id, *);
V4L2_INT_WRAPPER_1(s_std, v4l2_std_id, *);
V4L2_INT_WRAPPER_1(s_video_routing, struct v4l2_routing, *);

V4L2_INT_WRAPPER_0(dev_init);
V4L2_INT_WRAPPER_0(dev_exit);
V4L2_INT_WRAPPER_1(s_power, enum v4l2_power, );
V4L2_INT_WRAPPER_1(g_priv, void, *);
V4L2_INT_WRAPPER_1(g_ifparm, struct v4l2_ifparm, *);
V4L2_INT_WRAPPER_1(g_needs_reset, void, *);
V4L2_INT_WRAPPER_1(enum_framesizes, struct v4l2_frmsizeenum, *);
V4L2_INT_WRAPPER_1(enum_frameintervals, struct v4l2_frmivalenum, *);

V4L2_INT_WRAPPER_0(reset);
V4L2_INT_WRAPPER_0(init);
V4L2_INT_WRAPPER_1(g_chip_ident, int, *);

#endif
