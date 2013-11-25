/*
    V4L2 device support header.

    Copyright (C) 2008  Hans Verkuil <hverkuil@xs4all.nl>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _V4L2_DEVICE_H
#define _V4L2_DEVICE_H

#include <media/media-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-dev.h>


#define V4L2_DEVICE_NAME_SIZE (20 + 16)

struct v4l2_ctrl_handler;

struct v4l2_device {
	struct device *dev;
#if defined(CONFIG_MEDIA_CONTROLLER)
	struct media_device *mdev;
#endif
	
	struct list_head subdevs;
	spinlock_t lock;
	
	char name[V4L2_DEVICE_NAME_SIZE];
	
	void (*notify)(struct v4l2_subdev *sd,
			unsigned int notification, void *arg);
	
	struct v4l2_ctrl_handler *ctrl_handler;
	
	struct v4l2_prio_state prio;
	
	struct mutex ioctl_lock;
	
	struct kref ref;
	
	void (*release)(struct v4l2_device *v4l2_dev);
};

static inline void v4l2_device_get(struct v4l2_device *v4l2_dev)
{
	kref_get(&v4l2_dev->ref);
}

int v4l2_device_put(struct v4l2_device *v4l2_dev);

int __must_check v4l2_device_register(struct device *dev, struct v4l2_device *v4l2_dev);

int v4l2_device_set_name(struct v4l2_device *v4l2_dev, const char *basename,
						atomic_t *instance);

void v4l2_device_disconnect(struct v4l2_device *v4l2_dev);

void v4l2_device_unregister(struct v4l2_device *v4l2_dev);

int __must_check v4l2_device_register_subdev(struct v4l2_device *v4l2_dev,
						struct v4l2_subdev *sd);
void v4l2_device_unregister_subdev(struct v4l2_subdev *sd);

int __must_check
v4l2_device_register_subdev_nodes(struct v4l2_device *v4l2_dev);

#define v4l2_device_for_each_subdev(sd, v4l2_dev)			\
	list_for_each_entry(sd, &(v4l2_dev)->subdevs, list)

#define __v4l2_device_call_subdevs_p(v4l2_dev, sd, cond, o, f, args...)	\
	do { 								\
		list_for_each_entry((sd), &(v4l2_dev)->subdevs, list)	\
			if ((cond) && (sd)->ops->o && (sd)->ops->o->f)	\
				(sd)->ops->o->f((sd) , ##args);		\
	} while (0)

#define __v4l2_device_call_subdevs(v4l2_dev, cond, o, f, args...)	\
	do {								\
		struct v4l2_subdev *__sd;				\
									\
		__v4l2_device_call_subdevs_p(v4l2_dev, __sd, cond, o,	\
						f , ##args);		\
	} while (0)

#define __v4l2_device_call_subdevs_until_err_p(v4l2_dev, sd, cond, o, f, args...) \
({ 									\
	long __err = 0;							\
									\
	list_for_each_entry((sd), &(v4l2_dev)->subdevs, list) {		\
		if ((cond) && (sd)->ops->o && (sd)->ops->o->f)		\
			__err = (sd)->ops->o->f((sd) , ##args);		\
		if (__err && __err != -ENOIOCTLCMD)			\
			break; 						\
	} 								\
	(__err == -ENOIOCTLCMD) ? 0 : __err;				\
})

#define __v4l2_device_call_subdevs_until_err(v4l2_dev, cond, o, f, args...) \
({									\
	struct v4l2_subdev *__sd;					\
	__v4l2_device_call_subdevs_until_err_p(v4l2_dev, __sd, cond, o,	\
						f , ##args);		\
})

#define v4l2_device_call_all(v4l2_dev, grpid, o, f, args...)		\
	do {								\
		struct v4l2_subdev *__sd;				\
									\
		__v4l2_device_call_subdevs_p(v4l2_dev, __sd,		\
			!(grpid) || __sd->grp_id == (grpid), o, f ,	\
			##args);					\
	} while (0)

#define v4l2_device_call_until_err(v4l2_dev, grpid, o, f, args...) 	\
({									\
	struct v4l2_subdev *__sd;					\
	__v4l2_device_call_subdevs_until_err_p(v4l2_dev, __sd,		\
			!(grpid) || __sd->grp_id == (grpid), o, f ,	\
			##args);					\
})

#endif
