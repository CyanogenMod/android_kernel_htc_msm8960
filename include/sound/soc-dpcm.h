/*
 * linux/sound/soc-dpcm.h -- ALSA SoC Dynamic PCM Support
 *
 * Author:		Liam Girdwood <lrg@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_SND_SOC_DPCM_H
#define __LINUX_SND_SOC_DPCM_H

#include <sound/pcm.h>

#define SND_SOC_DPCM_UPDATE_NO	0
#define SND_SOC_DPCM_UPDATE_BE	1
#define SND_SOC_DPCM_UPDATE_FE	2

enum snd_soc_dpcm_link_state {
	SND_SOC_DPCM_LINK_STATE_NEW	= 0,	
	SND_SOC_DPCM_LINK_STATE_FREE,			
};

struct snd_soc_dpcm_params {
	
	struct snd_soc_pcm_runtime *be;
	struct snd_soc_pcm_runtime *fe;

	
	enum snd_soc_dpcm_link_state state;

	struct list_head list_be;
	struct list_head list_fe;

	
	struct snd_pcm_hw_params hw_params;

#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_state;
#endif
};


static inline int snd_soc_dpcm_fe_can_update(struct snd_soc_pcm_runtime *fe,
		int stream)
{
	return (fe->dpcm[stream].runtime_update == SND_SOC_DPCM_UPDATE_FE);
}

static inline int snd_soc_dpcm_be_can_update(struct snd_soc_pcm_runtime *fe,
		struct snd_soc_pcm_runtime *be, int stream)
{
	if ((fe->dpcm[stream].runtime_update == SND_SOC_DPCM_UPDATE_FE) ||
	    ((fe->dpcm[stream].runtime_update == SND_SOC_DPCM_UPDATE_BE) &&
		  be->dpcm[stream].runtime_update))
		return 1;
	else
		return 0;
}

static inline int
	snd_soc_dpcm_platform_trigger(struct snd_pcm_substream *substream,
	int cmd, struct snd_soc_platform *platform)
{
	if (platform->driver->ops->trigger)
		return platform->driver->ops->trigger(substream, cmd);
	return 0;
}

int snd_soc_dpcm_can_be_free_stop(struct snd_soc_pcm_runtime *fe,
		struct snd_soc_pcm_runtime *be, int stream);

static inline struct snd_pcm_substream *
	snd_soc_dpcm_get_substream(struct snd_soc_pcm_runtime *be, int stream)
{
	return be->pcm->streams[stream].substream;
}

static inline enum snd_soc_dpcm_state
	snd_soc_dpcm_be_get_state(struct snd_soc_pcm_runtime *be, int stream)
{
	return be->dpcm[stream].state;
}

static inline void snd_soc_dpcm_be_set_state(struct snd_soc_pcm_runtime *be,
		int stream, enum snd_soc_dpcm_state state)
{
	be->dpcm[stream].state = state;
}
#endif
