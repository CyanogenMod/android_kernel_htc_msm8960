/*
 *  compress_offload.h - compress offload header definations
 *
 *  Copyright (C) 2011 Intel Corporation
 *  Authors:	Vinod Koul <vinod.koul@linux.intel.com>
 *		Pierre-Louis Bossart <pierre-louis.bossart@linux.intel.com>
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 */
#ifndef __COMPRESS_OFFLOAD_H
#define __COMPRESS_OFFLOAD_H

#include <linux/types.h>
#include <sound/asound.h>
#include "compress_params.h"


#define SNDRV_COMPRESS_VERSION SNDRV_PROTOCOL_VERSION(0, 1, 0)
struct snd_compressed_buffer {
	__u32 fragment_size;
	__u32 fragments;
};

struct snd_compr_params {
	struct snd_compressed_buffer buffer;
	struct snd_codec codec;
	__u8 no_wake_mode;
};

struct snd_compr_tstamp {
	__u32 byte_offset;
	__u32 copied_total;
	snd_pcm_uframes_t pcm_frames;
	snd_pcm_uframes_t pcm_io_frames;
	__u32 sampling_rate;
	uint64_t timestamp;
};

struct snd_compr_avail {
	__u64 avail;
	struct snd_compr_tstamp tstamp;
};

enum snd_compr_direction {
	SND_COMPRESS_PLAYBACK = 0,
	SND_COMPRESS_CAPTURE
};

struct snd_compr_caps {
	__u32 num_codecs;
	__u32 direction;
	__u32 min_fragment_size;
	__u32 max_fragment_size;
	__u32 min_fragments;
	__u32 max_fragments;
	__u32 codecs[MAX_NUM_CODECS];
	__u32 reserved[11];
};

struct snd_compr_codec_caps {
	__u32 codec;
	__u32 num_descriptors;
	struct snd_codec_desc descriptor[MAX_NUM_CODEC_DESCRIPTORS];
};

struct snd_compr_audio_info {
	uint32_t frame_size;
	uint32_t reserved[15];
};

#define SNDRV_COMPRESS_IOCTL_VERSION	_IOR('C', 0x00, int)
#define SNDRV_COMPRESS_GET_CAPS		_IOWR('C', 0x10, struct snd_compr_caps)
#define SNDRV_COMPRESS_GET_CODEC_CAPS	_IOWR('C', 0x11,\
						struct snd_compr_codec_caps)
#define SNDRV_COMPRESS_SET_PARAMS	_IOW('C', 0x12, struct snd_compr_params)
#define SNDRV_COMPRESS_GET_PARAMS	_IOR('C', 0x13, struct snd_codec)
#define SNDRV_COMPRESS_TSTAMP		_IOR('C', 0x20, struct snd_compr_tstamp)
#define SNDRV_COMPRESS_AVAIL		_IOR('C', 0x21, struct snd_compr_avail)
#define SNDRV_COMPRESS_PAUSE		_IO('C', 0x30)
#define SNDRV_COMPRESS_RESUME		_IO('C', 0x31)
#define SNDRV_COMPRESS_START		_IO('C', 0x32)
#define SNDRV_COMPRESS_STOP		_IO('C', 0x33)
#define SNDRV_COMPRESS_DRAIN		_IO('C', 0x34)
#define SND_COMPR_TRIGGER_DRAIN 7 
#endif
