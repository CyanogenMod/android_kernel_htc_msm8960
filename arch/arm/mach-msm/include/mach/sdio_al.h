/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
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


#ifndef __SDIO_AL__
#define __SDIO_AL__

#include <linux/mmc/card.h>

struct sdio_channel; 


#define SDIO_EVENT_DATA_READ_AVAIL      0x01
#define SDIO_EVENT_DATA_WRITE_AVAIL     0x02

#ifdef CONFIG_MSM_SDIO_AL

struct sdio_al_platform_data {
	int (*config_mdm2ap_status)(int);
	int (*get_mdm2ap_status)(void);
	int allow_sdioc_version_major_2;
	int peer_sdioc_version_minor;
	int peer_sdioc_version_major;
	int peer_sdioc_boot_version_minor;
	int peer_sdioc_boot_version_major;
};

int sdio_open(const char *name, struct sdio_channel **ch, void *priv,
	     void (*notify)(void *priv, unsigned channel_event));


int sdio_close(struct sdio_channel *ch);

int sdio_read(struct sdio_channel *ch, void *data, int len);

int sdio_write(struct sdio_channel *ch, const void *data, int len);

int sdio_write_avail(struct sdio_channel *ch);

int sdio_read_avail(struct sdio_channel *ch);

#else

static int __maybe_unused sdio_open(const char *name, struct sdio_channel **ch,
		void *priv, void (*notify)(void *priv, unsigned channel_event))
{
	return -ENODEV;
}

static int __maybe_unused sdio_close(struct sdio_channel *ch)
{
	return -ENODEV;
}

static int __maybe_unused sdio_read(struct sdio_channel *ch, void *data,
						int len)
{
	return -ENODEV;
}

static int __maybe_unused sdio_write(struct sdio_channel *ch, const void *data,
						int len)
{
	return -ENODEV;
}

static int __maybe_unused sdio_write_avail(struct sdio_channel *ch)
{
	return -ENODEV;
}

static int __maybe_unused sdio_read_avail(struct sdio_channel *ch)
{
	return -ENODEV;
}
#endif

#endif 
