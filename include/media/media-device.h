/*
 * Media device
 *
 * Copyright (C) 2010 Nokia Corporation
 *
 * Contacts: Laurent Pinchart <laurent.pinchart@ideasonboard.com>
 *	     Sakari Ailus <sakari.ailus@iki.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _MEDIA_DEVICE_H
#define _MEDIA_DEVICE_H

#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>

#include <media/media-devnode.h>
#include <media/media-entity.h>

struct device;

struct media_device {
	
	struct device *dev;
	struct media_devnode devnode;

	char model[32];
	char serial[40];
	char bus_info[32];
	u32 hw_revision;
	u32 driver_version;

	u32 entity_id;
	struct list_head entities;

	
	spinlock_t lock;
	
	struct mutex graph_mutex;

	int (*link_notify)(struct media_pad *source,
			   struct media_pad *sink, u32 flags);
};

#define to_media_device(node) container_of(node, struct media_device, devnode)

int __must_check media_device_register(struct media_device *mdev);
void media_device_unregister(struct media_device *mdev);

int __must_check media_device_register_entity(struct media_device *mdev,
					      struct media_entity *entity);
void media_device_unregister_entity(struct media_entity *entity);

#define media_device_for_each_entity(entity, mdev)			\
	list_for_each_entry(entity, &(mdev)->entities, list)

#endif
