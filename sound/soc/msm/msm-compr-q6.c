/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
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


#include <linux/init.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/control.h>
#include <sound/q6asm.h>
#include <sound/q6adm.h>
#include <asm/dma.h>
#include <linux/dma-mapping.h>
#include <linux/android_pmem.h>
#include <sound/timer.h>

#define Q6_EFFECT_DEBUG 0

#include "msm-compr-q6.h"
#include "msm-pcm-routing.h"
#include <linux/wakelock.h>

#undef pr_info
#undef pr_err
#define pr_info(fmt, ...) pr_aud_info(fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...) pr_aud_err(fmt, ##__VA_ARGS__)

#define COMPRE_CAPTURE_NUM_PERIODS	16
#define COMPRE_CAPTURE_HEADER_SIZE	(sizeof(struct snd_compr_audio_info))
#define COMPRE_CAPTURE_MAX_FRAME_SIZE	(6144)
#define COMPRE_CAPTURE_PERIOD_SIZE	(COMPRE_CAPTURE_MAX_FRAME_SIZE + \
					 COMPRE_CAPTURE_HEADER_SIZE)
#define COMPRE_OUTPUT_METADATA_SIZE	(sizeof(struct output_meta_data_st))

struct wake_lock compr_lpa_wakelock;
struct wake_lock compr_lpa_q6_cb_wakelock;

struct snd_msm {
	struct msm_audio *prtd;
	unsigned volume;
};
static struct snd_msm compressed_audio = {NULL, 0} ;
static struct snd_msm compressed2_audio = {NULL, 0} ;

static struct audio_locks the_locks;

static struct snd_pcm_hardware msm_compr_hardware_capture = {
	.info =		 (SNDRV_PCM_INFO_MMAP |
				SNDRV_PCM_INFO_BLOCK_TRANSFER |
				SNDRV_PCM_INFO_MMAP_VALID |
				SNDRV_PCM_INFO_INTERLEAVED |
				SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats =	      SNDRV_PCM_FMTBIT_S16_LE,
	.rates =		SNDRV_PCM_RATE_8000_48000,
	.rate_min =	     8000,
	.rate_max =	     48000,
	.channels_min =	 1,
	.channels_max =	 8,
	.buffer_bytes_max =
		COMPRE_CAPTURE_PERIOD_SIZE * COMPRE_CAPTURE_NUM_PERIODS ,
	.period_bytes_min =	COMPRE_CAPTURE_PERIOD_SIZE,
	.period_bytes_max = COMPRE_CAPTURE_PERIOD_SIZE,
	.periods_min =	  COMPRE_CAPTURE_NUM_PERIODS,
	.periods_max =	  COMPRE_CAPTURE_NUM_PERIODS,
	.fifo_size =	    0,
};

static struct snd_pcm_hardware msm_compr_hardware_playback = {
	.info =		 (SNDRV_PCM_INFO_MMAP |
				SNDRV_PCM_INFO_BLOCK_TRANSFER |
				SNDRV_PCM_INFO_MMAP_VALID |
				SNDRV_PCM_INFO_INTERLEAVED |
				SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats =	      SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
	.rates =		SNDRV_PCM_RATE_8000_48000 | SNDRV_PCM_RATE_KNOT,
	.rate_min =	     8000,
	.rate_max =	     48000,
	.channels_min =	 1,
	.channels_max =	 2,
	.buffer_bytes_max =     2 * 1024 * 1024,
	.period_bytes_min =	8 * 1024,
	.period_bytes_max =     1024 * 1024,
	.periods_min =	  2,
	.periods_max =	  256,
	.fifo_size =	    0,
};

static unsigned int supported_sample_rates[] = {
	8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000
};

static struct snd_pcm_hw_constraint_list constraints_sample_rates = {
	.count = ARRAY_SIZE(supported_sample_rates),
	.list = supported_sample_rates,
	.mask = 0,
};

static struct msm_compr_q6_ops default_cops;
static struct msm_compr_q6_ops *cops = &default_cops;

void htc_register_compr_q6_ops(struct msm_compr_q6_ops *ops)
{
	cops = ops;
}

static void compr_event_handler(uint32_t opcode,
		uint32_t token, uint32_t *payload, void *priv)
{
	struct compr_audio *compr = priv;
	struct msm_audio *prtd = &compr->prtd;
	struct snd_pcm_substream *substream = prtd->substream;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct audio_aio_write_param param;
	struct audio_aio_read_param read_param;
	struct audio_buffer *buf = NULL;
	struct output_meta_data_st output_meta_data;
	uint32_t *ptrmem = (uint32_t *)payload;
	int i = 0;
	int time_stamp_flag = 0;
	int buffer_length = 0;
	wake_lock_timeout(&compr_lpa_q6_cb_wakelock, 1.5 * HZ);

	
	switch (opcode) {
	case ASM_DATA_EVENT_WRITE_DONE: {
		uint32_t *ptrmem = (uint32_t *)&param;
		pr_debug("[%p] ASM_DATA_EVENT_WRITE_DONE\n", prtd);
		pr_debug("[%p] Buffer Consumed = 0x%08x\n", prtd, *ptrmem);
		prtd->pcm_irq_pos += prtd->pcm_count;
		if (atomic_read(&prtd->start))
			snd_pcm_period_elapsed(substream);
		else
			if (substream->timer_running)
				snd_timer_interrupt(substream->timer, 1);
		atomic_inc(&prtd->out_count);
		wake_up(&the_locks.write_wait);
		if (!atomic_read(&prtd->start)) {
			atomic_set(&prtd->pending_buffer, 1);
			break;
		} else
			atomic_set(&prtd->pending_buffer, 0);
		if (runtime->status->hw_ptr >= runtime->control->appl_ptr)
			break;
		buf = prtd->audio_client->port[IN].buf;
		pr_debug("[%p] %s:writing %d bytes of buffer[%d] to dsp 2\n",
				prtd, __func__, prtd->pcm_count, prtd->out_head);
		pr_debug("[%p] %s:writing buffer[%d] from 0x%08x\n",
				prtd, __func__, prtd->out_head,
				((unsigned int)buf[0].phys
				+ (prtd->out_head * prtd->pcm_count)));

		if (runtime->tstamp_mode == SNDRV_PCM_TSTAMP_ENABLE)
			time_stamp_flag = SET_TIMESTAMP;
		else
			time_stamp_flag = NO_TIMESTAMP;
		memcpy(&output_meta_data, (char *)(buf->data +
			prtd->out_head * prtd->pcm_count),
			COMPRE_OUTPUT_METADATA_SIZE);

		buffer_length = output_meta_data.frame_size;
		pr_debug("[%p] meta_data_length: %d, frame_length: %d\n",
			 prtd,
			 output_meta_data.meta_data_length,
			 output_meta_data.frame_size);
		pr_debug("[%p] timestamp_msw: %d, timestamp_lsw: %d\n",
			 prtd,
			 output_meta_data.timestamp_msw,
			 output_meta_data.timestamp_lsw);
		if (buffer_length == 0) {
			pr_debug("[%p] Recieved a zero length buffer-break out", prtd);
			break;
		}
		param.paddr = (unsigned long)buf[0].phys
				+ (prtd->out_head * prtd->pcm_count)
				+ output_meta_data.meta_data_length;
		param.len = buffer_length;
		param.msw_ts = output_meta_data.timestamp_msw;
		param.lsw_ts = output_meta_data.timestamp_lsw;
		param.flags = time_stamp_flag;
		param.uid =  (unsigned long)buf[0].phys
				+ (prtd->out_head * prtd->pcm_count
				+ output_meta_data.meta_data_length);
		for (i = 0; i < sizeof(struct audio_aio_write_param)/4;
					i++, ++ptrmem)
			pr_debug("[%p] cmd[%d]=0x%08x\n", prtd, i, *ptrmem);
		if (q6asm_async_write(prtd->audio_client,
					&param) < 0)
			pr_err("[%p] %s:q6asm_async_write failed\n",
				prtd, __func__);
		else
			prtd->out_head =
				(prtd->out_head + 1) & (runtime->periods - 1);
		break;
	}
	case ASM_DATA_CMDRSP_EOS:
		pr_debug("[%p] ASM_DATA_CMDRSP_EOS\n", prtd);
		if (atomic_read(&prtd->eos)) {
			pr_debug("[%p] ASM_DATA_CMDRSP_EOS wake up\n", prtd);
			prtd->cmd_ack = 1;
			wake_up(&the_locks.eos_wait);
			atomic_set(&prtd->eos, 0);
		}
		atomic_set(&prtd->pending_buffer, 1);
		break;
	case ASM_DATA_EVENT_READ_DONE: {
		pr_debug("[%p] ASM_DATA_EVENT_READ_DONE\n", prtd);
		pr_debug("[%p] buf = %p, data = 0x%X, *data = %p,\n"
			 "prtd->pcm_irq_pos = %d\n",
			    prtd,
				prtd->audio_client->port[OUT].buf,
			 *(uint32_t *)prtd->audio_client->port[OUT].buf->data,
				prtd->audio_client->port[OUT].buf->data,
				prtd->pcm_irq_pos);

		memcpy(prtd->audio_client->port[OUT].buf->data +
			   prtd->pcm_irq_pos, (ptrmem + 2),
			   COMPRE_CAPTURE_HEADER_SIZE);
		pr_debug("[%p] buf = %p, updated data = 0x%X, *data = %p\n",
				prtd,
				prtd->audio_client->port[OUT].buf,
			*(uint32_t *)(prtd->audio_client->port[OUT].buf->data +
				prtd->pcm_irq_pos),
				prtd->audio_client->port[OUT].buf->data);
		if (!atomic_read(&prtd->start))
			break;
		pr_debug("[%p] frame size=%d, buffer = 0x%X\n", prtd, ptrmem[2],
				ptrmem[1]);
		if (ptrmem[2] > COMPRE_CAPTURE_MAX_FRAME_SIZE) {
			pr_err("[%p] Frame length exceeded the max length", prtd);
			break;
		}
		buf = prtd->audio_client->port[OUT].buf;
		pr_debug("[%p] pcm_irq_pos=%d, buf[0].phys = 0x%X\n",
				prtd, prtd->pcm_irq_pos, (uint32_t)buf[0].phys);
		read_param.len = prtd->pcm_count - COMPRE_CAPTURE_HEADER_SIZE ;
		read_param.paddr = (unsigned long)(buf[0].phys) +
			prtd->pcm_irq_pos + COMPRE_CAPTURE_HEADER_SIZE;
		prtd->pcm_irq_pos += prtd->pcm_count;

		if (atomic_read(&prtd->start))
			snd_pcm_period_elapsed(substream);

		q6asm_async_read(prtd->audio_client, &read_param);
		break;
	}
	case APR_BASIC_RSP_RESULT: {
		switch (payload[0]) {
		case ASM_SESSION_CMD_RUN: {
			if (substream->stream
				!= SNDRV_PCM_STREAM_PLAYBACK) {
				atomic_set(&prtd->start, 1);
				break;
			}
			if (!atomic_read(&prtd->pending_buffer))
				break;
			pr_debug("[%p] %s:writing %d bytes"
				" of buffer[%d] to dsp\n",
				prtd, __func__, prtd->pcm_count, prtd->out_head);
			buf = prtd->audio_client->port[IN].buf;
			pr_debug("[%p] %s:writing buffer[%d] from 0x%08x\n",
				prtd, __func__, prtd->out_head,
				((unsigned int)buf[0].phys
				+ (prtd->out_head * prtd->pcm_count)));
			if (runtime->tstamp_mode == SNDRV_PCM_TSTAMP_ENABLE)
				time_stamp_flag = SET_TIMESTAMP;
			else
				time_stamp_flag = NO_TIMESTAMP;
			memcpy(&output_meta_data, (char *)(buf->data +
				prtd->out_head * prtd->pcm_count),
				COMPRE_OUTPUT_METADATA_SIZE);
			buffer_length = output_meta_data.frame_size;
			pr_debug("[%p] meta_data_length: %d, frame_length: %d\n",
				 prtd,
				 output_meta_data.meta_data_length,
				 output_meta_data.frame_size);
			pr_debug("[%p] timestamp_msw: %d, timestamp_lsw: %d\n",
				 prtd,
				 output_meta_data.timestamp_msw,
				 output_meta_data.timestamp_lsw);
			param.paddr = (unsigned long)buf[prtd->out_head].phys
					+ output_meta_data.meta_data_length;
			param.len = buffer_length;
			param.msw_ts = output_meta_data.timestamp_msw;
			param.lsw_ts = output_meta_data.timestamp_lsw;
			param.flags = time_stamp_flag;
			param.uid =  (unsigned long)buf[prtd->out_head].phys
					+ output_meta_data.meta_data_length;
			if (q6asm_async_write(prtd->audio_client,
						&param) < 0)
				pr_err("[%p] %s:q6asm_async_write failed\n",
				    prtd,
					__func__);
			else
				prtd->out_head =
					(prtd->out_head + 1)
					& (runtime->periods - 1);
			atomic_set(&prtd->pending_buffer, 0);
		}
			break;
		case ASM_STREAM_CMD_FLUSH:
			pr_debug("[%p] ASM_STREAM_CMD_FLUSH\n", prtd);
			prtd->cmd_ack = 1;
			wake_up(&the_locks.flush_wait);
			break;
		default:
			break;
		}
		break;
	}
	default:
		
		break;
	}
}

static int msm_compr_playback_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct compr_audio *compr = runtime->private_data;
	struct msm_audio *prtd = &compr->prtd;
	struct asm_aac_cfg aac_cfg;
	struct asm_wma_cfg wma_cfg;
	struct asm_wmapro_cfg wma_pro_cfg;
	int ret;

	pr_debug("[%p] compressed stream prepare\n", prtd);
	prtd->pcm_size = snd_pcm_lib_buffer_bytes(substream);
	prtd->pcm_count = snd_pcm_lib_period_bytes(substream);
	prtd->pcm_irq_pos = 0;
	
	prtd->samp_rate = runtime->rate;
	prtd->channel_mode = runtime->channels;
	prtd->out_head = 0;
	atomic_set(&prtd->out_count, runtime->periods);

	if (prtd->enabled)
		return 0;

	switch (compr->info.codec_param.codec.id) {
	case SND_AUDIOCODEC_MP3:
		pr_debug("[%p] %s: SND_AUDIOCODEC_MP3\n", prtd, __func__);
		ret = q6asm_media_format_block(prtd->audio_client,
				compr->codec);
		if (ret < 0)
			pr_info("[%p] %s: CMD Format block failed\n", prtd, __func__);
		break;
	case SND_AUDIOCODEC_PCM:
		pr_debug("[%p] %s: SND_AUDIOCODEC_PCM, sampleRate: %d, channel: %d\n",
				prtd, __func__, prtd->samp_rate, prtd->channel_mode);
		ret = q6asm_media_format_block_pcm(prtd->audio_client, prtd->samp_rate,
				prtd->channel_mode);
		if (ret < 0)
			pr_info("[%p] %s: CMD Format block failed\n", prtd, __func__);
		break;

	case SND_AUDIOCODEC_AAC:
		pr_debug("[%p] %s: SND_AUDIOCODEC_AAC\n", prtd, __func__);
		memset(&aac_cfg, 0x0, sizeof(struct asm_aac_cfg));
		aac_cfg.aot = AAC_ENC_MODE_EAAC_P;
		aac_cfg.format = 0x03;
		aac_cfg.ch_cfg = runtime->channels;
		aac_cfg.sample_rate = runtime->rate;
		ret = q6asm_media_format_block_aac(prtd->audio_client,
					&aac_cfg);
		if (ret < 0)
			pr_err("[%p] %s: CMD Format block failed\n", prtd, __func__);
		break;
	case SND_AUDIOCODEC_AC3_PASS_THROUGH:
		pr_debug("[%p] compressd playback, no need to send"
			" the decoder params\n", prtd);
		break;
	case SND_AUDIOCODEC_DTS_PASS_THROUGH:
		pr_debug("[%p] compressd DTS playback,dont send the decoder params\n", prtd);
		break;
	case SND_AUDIOCODEC_WMA:
		pr_debug("[%p] SND_AUDIOCODEC_WMA\n", prtd);
		memset(&wma_cfg, 0x0, sizeof(struct asm_wma_cfg));
		wma_cfg.format_tag = compr->info.codec_param.codec.format;
		wma_cfg.ch_cfg = runtime->channels;
		wma_cfg.sample_rate = compr->info.codec_param.codec.sample_rate;
		wma_cfg.avg_bytes_per_sec =
			compr->info.codec_param.codec.bit_rate/8;
		wma_cfg.block_align = compr->info.codec_param.codec.align;
		wma_cfg.valid_bits_per_sample =
		compr->info.codec_param.codec.options.wma.bits_per_sample;
		wma_cfg.ch_mask =
			compr->info.codec_param.codec.options.wma.channelmask;
		wma_cfg.encode_opt =
			compr->info.codec_param.codec.options.wma.encodeopt;
		ret = q6asm_media_format_block_wma(prtd->audio_client,
					&wma_cfg);
		if (ret < 0)
			pr_err("[%p] %s: CMD Format block failed\n", prtd, __func__);
		break;
	case SND_AUDIOCODEC_WMA_PRO:
		pr_debug("[%p] SND_AUDIOCODEC_WMA_PRO\n", prtd);
		memset(&wma_pro_cfg, 0x0, sizeof(struct asm_wmapro_cfg));
		wma_pro_cfg.format_tag = compr->info.codec_param.codec.format;
		wma_pro_cfg.ch_cfg = compr->info.codec_param.codec.ch_in;
		wma_pro_cfg.sample_rate =
			compr->info.codec_param.codec.sample_rate;
		wma_pro_cfg.avg_bytes_per_sec =
			compr->info.codec_param.codec.bit_rate/8;
		wma_pro_cfg.block_align = compr->info.codec_param.codec.align;
		wma_pro_cfg.valid_bits_per_sample =
			compr->info.codec_param.codec\
				.options.wma.bits_per_sample;
		wma_pro_cfg.ch_mask =
			compr->info.codec_param.codec.options.wma.channelmask;
		wma_pro_cfg.encode_opt =
			compr->info.codec_param.codec.options.wma.encodeopt;
		wma_pro_cfg.adv_encode_opt =
			compr->info.codec_param.codec.options.wma.encodeopt1;
		wma_pro_cfg.adv_encode_opt2 =
			compr->info.codec_param.codec.options.wma.encodeopt2;
		ret = q6asm_media_format_block_wmapro(prtd->audio_client,
				&wma_pro_cfg);
		if (ret < 0)
			pr_err("[%p] %s: CMD Format block failed\n", prtd, __func__);
		break;
	case SND_AUDIOCODEC_DTS:
	case SND_AUDIOCODEC_DTS_LBR:
		pr_debug("[%p] SND_AUDIOCODEC_DTS\n", prtd);
		ret = q6asm_media_format_block(prtd->audio_client,
				compr->codec);
		if (ret < 0) {
			pr_err("[%p] %s: CMD Format block failed\n", prtd, __func__);
			return ret;
		}
		break;
	default:
		return -EINVAL;
	}

	prtd->enabled = 1;
	prtd->cmd_ack = 0;

	return 0;
}

static int msm_compr_capture_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct compr_audio *compr = runtime->private_data;
	struct msm_audio *prtd = &compr->prtd;
	struct audio_buffer *buf = prtd->audio_client->port[OUT].buf;
	struct audio_aio_read_param read_param;
	int ret = 0;
	int i;
	prtd->pcm_size = snd_pcm_lib_buffer_bytes(substream);
	prtd->pcm_count = snd_pcm_lib_period_bytes(substream);
	prtd->pcm_irq_pos = 0;

	
	prtd->samp_rate = runtime->rate;
	prtd->channel_mode = runtime->channels;

	if (prtd->enabled)
		return ret;
	read_param.len = prtd->pcm_count - COMPRE_CAPTURE_HEADER_SIZE;
	pr_debug("[%p] %s: Samp_rate = %d, Channel = %d, pcm_size = %d,\n"
			 "pcm_count = %d, periods = %d\n",
			 prtd,
			 __func__, prtd->samp_rate, prtd->channel_mode,
			 prtd->pcm_size, prtd->pcm_count, runtime->periods);

	for (i = 0; i < runtime->periods; i++) {
		read_param.uid = i;
		read_param.paddr = ((unsigned long)(buf[i].phys) +
					COMPRE_CAPTURE_HEADER_SIZE);
		q6asm_async_read(prtd->audio_client, &read_param);
	}
	prtd->periods = runtime->periods;

	prtd->enabled = 1;

	return ret;
}

static int msm_compr_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *soc_prtd = substream->private_data;
	struct compr_audio *compr = runtime->private_data;
	struct msm_audio *prtd = &compr->prtd;

	pr_debug("[%p][AUD] %s, cmd %d, 5 sec wake lock\n", prtd,__func__, cmd);
	wake_lock_timeout(&compr_lpa_wakelock, 5 * HZ);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		prtd->pcm_irq_pos = 0;
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (compr->info.codec_param.codec.id ==
				SND_AUDIOCODEC_AC3_PASS_THROUGH ||
					compr->info.codec_param.codec.id ==
					SND_AUDIOCODEC_DTS_PASS_THROUGH) {
				msm_pcm_routing_reg_psthr_stream(
					soc_prtd->dai_link->be_id,
					prtd->session_id, substream->stream,
					1);
			}
		} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			msm_pcm_routing_reg_psthr_stream(
				soc_prtd->dai_link->be_id,
				prtd->session_id, substream->stream, 1);
		}
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		pr_debug("[%p] %s: Trigger start/resume\n", prtd, __func__);
		q6asm_run_nowait(prtd->audio_client, 0, 0, 0);
		atomic_set(&prtd->start, 1);
		
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		pr_debug("[%p] SNDRV_PCM_TRIGGER_STOP\n", prtd);
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			if (compr->info.codec_param.codec.id ==
					SND_AUDIOCODEC_AC3_PASS_THROUGH) {
				msm_pcm_routing_reg_psthr_stream(
					soc_prtd->dai_link->be_id,
					prtd->session_id, substream->stream,
					0);
			}
		} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			msm_pcm_routing_reg_psthr_stream(
				soc_prtd->dai_link->be_id,
				prtd->session_id, substream->stream,
				0);
		}
		atomic_set(&prtd->start, 0);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		pr_debug("[%p] SNDRV_PCM_TRIGGER_PAUSE\n", prtd);
		q6asm_cmd_nowait(prtd->audio_client, CMD_PAUSE);
		atomic_set(&prtd->start, 0);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static void populate_codec_list(struct compr_audio *compr,
		struct snd_pcm_runtime *runtime)
{
	pr_debug("%s\n", __func__);
	
	compr->info.compr_cap.num_codecs = 11;
	compr->info.compr_cap.min_fragment_size = runtime->hw.period_bytes_min;
	compr->info.compr_cap.max_fragment_size = runtime->hw.period_bytes_max;
	compr->info.compr_cap.min_fragments = runtime->hw.periods_min;
	compr->info.compr_cap.max_fragments = runtime->hw.periods_max;
	compr->info.compr_cap.codecs[0] = SND_AUDIOCODEC_MP3;
	compr->info.compr_cap.codecs[1] = SND_AUDIOCODEC_AAC;
	compr->info.compr_cap.codecs[2] = SND_AUDIOCODEC_AC3_PASS_THROUGH;
	compr->info.compr_cap.codecs[3] = SND_AUDIOCODEC_WMA;
	compr->info.compr_cap.codecs[4] = SND_AUDIOCODEC_WMA_PRO;
	compr->info.compr_cap.codecs[5] = SND_AUDIOCODEC_DTS;
	compr->info.compr_cap.codecs[6] = SND_AUDIOCODEC_DTS_LBR;
	compr->info.compr_cap.codecs[7] = SND_AUDIOCODEC_DTS_PASS_THROUGH;
	compr->info.compr_cap.codecs[8] = SND_AUDIOCODEC_AMRWB;
	compr->info.compr_cap.codecs[9] = SND_AUDIOCODEC_AMRWBPLUS;
	compr->info.compr_cap.codecs[10] = SND_AUDIOCODEC_PCM;
	
}

static int msm_compr_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	char * str_name;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct compr_audio *compr;
	struct msm_audio *prtd;
	int ret = 0;

	pr_debug("%s\n", __func__);
	str_name = (char*)rtd->dai_link->stream_name;
	if (str_name != NULL)
		pr_info("%s, dai_link stream name = %s\n", __func__, str_name);
	compr = kzalloc(sizeof(struct compr_audio), GFP_KERNEL);
	if (compr == NULL) {
		pr_err("Failed to allocate memory for msm_audio\n");
		return -ENOMEM;
	}
	prtd = &compr->prtd;
	prtd->substream = substream;
	prtd->audio_client = q6asm_audio_client_alloc(
				(app_cb)compr_event_handler, compr);
	if (!prtd->audio_client) {
		pr_info("%s: Could not allocate memory\n", __func__);
		kfree(prtd);
		return -ENOMEM;
	}
	prtd->audio_client->perf_mode = false;
	pr_info("[%p] %s: session ID %d\n", prtd, __func__, prtd->audio_client->session);

	prtd->session_id = prtd->audio_client->session;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		runtime->hw = msm_compr_hardware_playback;
		prtd->cmd_ack = 1;
	} else {
		runtime->hw = msm_compr_hardware_capture;
	}


	ret = snd_pcm_hw_constraint_list(runtime, 0,
			SNDRV_PCM_HW_PARAM_RATE,
			&constraints_sample_rates);
	if (ret < 0)
		pr_info("[%p] snd_pcm_hw_constraint_list failed\n", prtd);
	
	ret = snd_pcm_hw_constraint_integer(runtime,
			    SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		pr_info("[%p] snd_pcm_hw_constraint_integer failed\n", prtd);

	prtd->dsp_cnt = 0;
	atomic_set(&prtd->pending_buffer, 1);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		compr->codec = FORMAT_MP3;
	populate_codec_list(compr, runtime);
	runtime->private_data = compr;
	atomic_set(&prtd->eos, 0);
	if (str_name != NULL && !strncmp(str_name,"COMPR2", 6)) {
		compressed2_audio.prtd =  &compr->prtd;
	} else {
		compressed_audio.prtd =  &compr->prtd;
	}

	return 0;
}

int compressed_set_volume(unsigned volume)
{
	int rc = 0;
	if (compressed_audio.prtd && compressed_audio.prtd->audio_client) {
		rc = q6asm_set_volume(compressed_audio.prtd->audio_client,
								 volume);
		if (rc < 0) {
			pr_err("[%p] %s: Send Volume command failed"
					" rc=%d\n", compressed_audio.prtd, __func__, rc);
		}else{
			pr_debug("[%p]compr vol %d\n",
					compressed_audio.prtd, volume);
		}
	}
	compressed_audio.volume = volume;
	return rc;
}
int compressed2_set_volume(unsigned volume)
{
	int rc = 0;
	if (compressed2_audio.prtd && compressed2_audio.prtd->audio_client) {
		rc = q6asm_set_volume(compressed2_audio.prtd->audio_client,
								 volume);
		if (rc < 0) {
			pr_err("[%p] %s: compr2: Send Volume command failed"
					" rc=%d\n", compressed2_audio.prtd, __func__, rc);
		}else{
			pr_debug("[%p]compr2 vol %d\n",
				compressed2_audio.prtd, volume);

		}
	}
	compressed2_audio.volume = volume;
	return rc;
}

static int msm_compr_playback_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	char * str_name;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *soc_prtd = substream->private_data;
	struct compr_audio *compr = runtime->private_data;
	struct msm_audio *prtd = &compr->prtd;
	int dir = 0;

	pr_debug("[%p] %s\n", prtd, __func__);

	dir = IN;
	atomic_set(&prtd->pending_buffer, 0);
	prtd->pcm_irq_pos = 0;
	q6asm_cmd(prtd->audio_client, CMD_CLOSE);

	str_name = (char*)rtd->dai_link->stream_name;
	if (str_name != NULL && !strncmp(str_name,"COMPR2", 6))
	    compressed2_audio.prtd = NULL;
	else
	    compressed_audio.prtd = NULL;
	q6asm_audio_client_buf_free_contiguous(dir,
				prtd->audio_client);
	if (!(compr->info.codec_param.codec.id ==
			SND_AUDIOCODEC_AC3_PASS_THROUGH))
		msm_pcm_routing_dereg_phy_stream(
			soc_prtd->dai_link->be_id,
			SNDRV_PCM_STREAM_PLAYBACK);
	q6asm_audio_client_free(prtd->audio_client);
	kfree(prtd);
	return 0;
}

static int msm_compr_capture_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *soc_prtd = substream->private_data;
	struct compr_audio *compr = runtime->private_data;
	struct msm_audio *prtd = &compr->prtd;
	int dir = OUT;

	pr_debug("[%p] %s\n", prtd, __func__);
	atomic_set(&prtd->pending_buffer, 0);
	q6asm_cmd(prtd->audio_client, CMD_CLOSE);
	q6asm_audio_client_buf_free_contiguous(dir,
				prtd->audio_client);
	msm_pcm_routing_dereg_phy_stream(soc_prtd->dai_link->be_id,
				SNDRV_PCM_STREAM_CAPTURE);
	q6asm_audio_client_free(prtd->audio_client);
	kfree(prtd);

	return 0;
}

static int msm_compr_close(struct snd_pcm_substream *substream)
{
	int ret = 0;
	
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct compr_audio *compr = runtime->private_data;
	struct msm_audio *prtd = &compr->prtd;
	if(prtd != NULL)
		pr_debug("[%p] %s +++\n", prtd, __func__);
	else
		pr_debug("%s +++\n", __func__);
	

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		ret = msm_compr_playback_close(substream);
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		ret = msm_compr_capture_close(substream);

	pr_debug("%s ---\n", __func__);
	return ret;
}
static int msm_compr_prepare(struct snd_pcm_substream *substream)
{
	int ret = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		ret = msm_compr_playback_prepare(substream);
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		ret = msm_compr_capture_prepare(substream);
	return ret;
}

static snd_pcm_uframes_t msm_compr_pointer(struct snd_pcm_substream *substream)
{

	struct snd_pcm_runtime *runtime = substream->runtime;
	struct compr_audio *compr = runtime->private_data;
	struct msm_audio *prtd = &compr->prtd;

	if (prtd->pcm_irq_pos >= prtd->pcm_size)
		prtd->pcm_irq_pos = 0;

	pr_debug("[%p] %s: pcm_irq_pos = %d, pcm_size = %d, sample_bits = %d,\n"
			 "frame_bits = %d\n", prtd, __func__, prtd->pcm_irq_pos,
			 prtd->pcm_size, runtime->sample_bits,
			 runtime->frame_bits);
	return bytes_to_frames(runtime, (prtd->pcm_irq_pos));
}

static int msm_compr_mmap(struct snd_pcm_substream *substream,
				struct vm_area_struct *vma)
{
	int result = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct compr_audio *compr = runtime->private_data;
	struct msm_audio *prtd = &compr->prtd;

	pr_debug("[%p] %s\n", prtd, __func__);
	prtd->mmap_flag = 1;
	if (runtime->dma_addr && runtime->dma_bytes) {
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		result = remap_pfn_range(vma, vma->vm_start,
				runtime->dma_addr >> PAGE_SHIFT,
				runtime->dma_bytes,
				vma->vm_page_prot);
	} else {
		pr_err("[%p] Physical address or size of buf is NULL", prtd);
		return -EINVAL;
	}
	return result;
}

static int msm_compr_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_pcm_runtime *soc_prtd = substream->private_data;
	struct compr_audio *compr = runtime->private_data;
	struct msm_audio *prtd = &compr->prtd;
	struct snd_dma_buffer *dma_buf = &substream->dma_buffer;
	struct audio_buffer *buf;
	int dir, ret;
	short bit_width = 16;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	char * str_name;
	struct asm_softpause_params softpause = {
		.enable = SOFT_PAUSE_ENABLE,
		.period = SOFT_PAUSE_PERIOD*3,
		.step = SOFT_PAUSE_STEP,
		.rampingcurve = SOFT_PAUSE_CURVE_LINEAR,
	};
	struct asm_softvolume_params softvol = {
#ifdef CONFIG_MSM8960_ONLY
		.period = 30,
#else
		.period = 50,
#endif
		.step = SOFT_VOLUME_STEP,
		.rampingcurve = SOFT_VOLUME_CURVE_LINEAR,
	};

	pr_debug("[%p] %s\n", prtd, __func__);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dir = IN;
	else
		dir = OUT;

	if (runtime->format == SNDRV_PCM_FORMAT_S24_LE)
		bit_width = 24;

	if (cops->get_24b_audio) {
		if (cops->get_24b_audio() == 1) {
			pr_info("%s: enable 24 bit Audio in POPP\n",
				    __func__);
			bit_width = 24;
		}
	}


	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (compr->info.codec_param.codec.id) {
		case SND_AUDIOCODEC_AC3_PASS_THROUGH:
		case SND_AUDIOCODEC_DTS_PASS_THROUGH:
			ret = q6asm_open_write_compressed(prtd->audio_client,
					compr->codec);

			if (ret < 0) {
				pr_err("[%p] %s: Session out open failed\n",
					prtd, __func__);
				return -ENOMEM;
			}
			break;
		default:
			ret = q6asm_open_write_v2(prtd->audio_client,
					compr->codec, bit_width);
			if (ret < 0) {
				pr_err("[%p] %s: Session out open failed\n",
					prtd, __func__);
				return -ENOMEM;
			}
			msm_pcm_routing_reg_phy_stream(
				soc_prtd->dai_link->be_id,
				prtd->audio_client->perf_mode,
				prtd->session_id,
				substream->stream);

			break;
		}
	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		ret = q6asm_open_read_compressed(prtd->audio_client,
					compr->codec);

		if (ret < 0) {
			pr_err("[%p] %s: compressed Session out open failed\n",
				prtd, __func__);
			return -ENOMEM;
		}
	}
	str_name = (char*)rtd->dai_link->stream_name;
	if (str_name != NULL)
		pr_info("%s, dai_link stream name = %s\n", __func__, str_name);

	if (str_name != NULL && !strncmp(str_name,"COMPR2", 6)) {
		if (compressed2_audio.prtd && compressed2_audio.prtd->audio_client) {
			pr_debug("[%p] %s compressed2 set ramping for %d ms\n", prtd, __func__,softvol.period);
			ret = compressed2_set_volume(compressed2_audio.volume);
			if (ret < 0)
				pr_err("[%p] %s : Set Volume2 failed : %d", prtd, __func__, ret);

			ret = q6asm_set_softpause(compressed2_audio.prtd->audio_client,
										&softpause);
			if (ret < 0)
				pr_err("[%p] %s: Send SoftPause2 Param failed ret=%d\n",
					prtd, __func__, ret);
			ret = q6asm_set_softvolume(compressed2_audio.prtd->audio_client,
										&softvol);
			if (ret < 0)
				pr_err("[%p] %s: Send SoftVolume2 Param failed ret=%d\n",
					prtd, __func__, ret);
		}
	} else {
		if (compressed_audio.prtd && compressed_audio.prtd->audio_client) {
			pr_debug("[%p] %s compressed set ramping for %d ms\n", prtd, __func__,softvol.period);
			ret = compressed_set_volume(compressed_audio.volume);
			if (ret < 0)
				pr_err("[%p] %s : Set Volume failed : %d", prtd,__func__, ret);

			ret = q6asm_set_softpause(compressed_audio.prtd->audio_client,
										&softpause);
			if (ret < 0)
				pr_err("[%p] %s: Send SoftPause Param failed ret=%d\n",
					prtd, __func__, ret);
			ret = q6asm_set_softvolume(compressed_audio.prtd->audio_client,
										&softvol);
			if (ret < 0)
				pr_err("[%p] %s: Send SoftVolume Param failed ret=%d\n",
					prtd, __func__, ret);
		}
	}

	ret = q6asm_set_io_mode(prtd->audio_client, ASYNC_IO_MODE);
	if (ret < 0) {
		pr_err("[%p] %s: Set IO mode failed\n", prtd, __func__);
		return -ENOMEM;
	}

	ret = q6asm_audio_client_buf_alloc_contiguous(dir,
			prtd->audio_client,
			runtime->hw.period_bytes_min,
			runtime->hw.periods_max);
	if (ret < 0) {
		pr_err("[%p] Audio Start: Buffer Allocation failed "
					"rc = %d\n", prtd, ret);
		return -ENOMEM;
	}
	buf = prtd->audio_client->port[dir].buf;

	dma_buf->dev.type = SNDRV_DMA_TYPE_DEV;
	dma_buf->dev.dev = substream->pcm->card->dev;
	dma_buf->private_data = NULL;
	dma_buf->area = buf[0].data;
	dma_buf->addr =  buf[0].phys;
	dma_buf->bytes = runtime->hw.buffer_bytes_max;

	pr_debug("[%p] %s: buf[%p]dma_buf->area[%p]dma_buf->addr[%p]\n"
		 "dma_buf->bytes[%d]\n", prtd, __func__,
		 (void *)buf, (void *)dma_buf->area,
		 (void *)dma_buf->addr, dma_buf->bytes);
	if (!dma_buf->area)
		return -ENOMEM;

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	return 0;
}

static int msm_compr_ioctl(struct snd_pcm_substream *substream,
		unsigned int cmd, void *arg)
{
	int rc = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct compr_audio *compr = runtime->private_data;
	struct msm_audio *prtd = &compr->prtd;
	uint64_t timestamp;
	uint64_t temp;
	
	uint32_t eos_flush_check;
	

	wake_lock_timeout(&compr_lpa_wakelock, 5 * HZ);
	switch (cmd) {
	case SNDRV_COMPRESS_TSTAMP: {
		struct snd_compr_tstamp tstamp;
		

		memset(&tstamp, 0x0, sizeof(struct snd_compr_tstamp));
		rc = q6asm_get_session_time(prtd->audio_client, &timestamp);
		if (rc < 0) {
			pr_err("[%p] %s: fail to get session tstamp\n", prtd, __func__);
			return rc;
		}
		temp = (timestamp * 2 * runtime->channels);
		temp = temp * (runtime->rate/1000);
		temp = div_u64(temp, 1000);
		tstamp.sampling_rate = runtime->rate;
		tstamp.timestamp = timestamp;
		if (copy_to_user((void *) arg, &tstamp,
			sizeof(struct snd_compr_tstamp)))
				return -EFAULT;
		return 0;
	}
	case SNDRV_COMPRESS_GET_CAPS:
		pr_debug("[%p] SNDRV_COMPRESS_GET_CAPS\n", prtd);
		if (copy_to_user((void *) arg, &compr->info.compr_cap,
			sizeof(struct snd_compr_caps))) {
			rc = -EFAULT;
			pr_err("[%p] %s: ERROR: copy to user\n", prtd, __func__);
			return rc;
		}
		return 0;
	case SNDRV_COMPRESS_SET_PARAMS:
		pr_debug("[%p] SNDRV_COMPRESS_SET_PARAMS: ", prtd);
		if (copy_from_user(&compr->info.codec_param, (void *) arg,
			sizeof(struct snd_compr_params))) {
			rc = -EFAULT;
			pr_err("[%p] %s: ERROR: copy from user\n", prtd, __func__);
			return rc;
		}
		switch (compr->info.codec_param.codec.id) {
		case SND_AUDIOCODEC_MP3:
			
			pr_debug("[%p] SND_AUDIOCODEC_MP3\n", prtd);
			compr->codec = FORMAT_MP3;
			break;
		case SND_AUDIOCODEC_PCM:
			
			pr_debug("[%p] SND_AUDIOCODEC_PCM\n", prtd);
			compr->codec = FORMAT_LINEAR_PCM;
			break;
		case SND_AUDIOCODEC_AAC:
			pr_debug("[%p] SND_AUDIOCODEC_AAC\n", prtd);
			compr->codec = FORMAT_MPEG4_AAC;
			break;
		case SND_AUDIOCODEC_AC3_PASS_THROUGH:
			pr_debug("[%p] SND_AUDIOCODEC_AC3_PASS_THROUGH\n", prtd);
			compr->codec = FORMAT_AC3;
			break;
		case SND_AUDIOCODEC_WMA:
			pr_debug("[%p] SND_AUDIOCODEC_WMA\n", prtd);
			compr->codec = FORMAT_WMA_V9;
			break;
		case SND_AUDIOCODEC_WMA_PRO:
			pr_debug("[%p] SND_AUDIOCODEC_WMA_PRO\n", prtd);
			compr->codec = FORMAT_WMA_V10PRO;
			break;
		case SND_AUDIOCODEC_DTS_PASS_THROUGH:
			pr_debug("[%p] SND_AUDIOCODEC_DTS_PASS_THROUGH\n", prtd);
			compr->codec = FORMAT_DTS;
			break;
		case SND_AUDIOCODEC_DTS:
			pr_debug("[%p] SND_AUDIOCODEC_DTS\n", prtd);
			compr->codec = FORMAT_DTS;
			break;
		case SND_AUDIOCODEC_DTS_LBR:
			pr_debug("[%p] SND_AUDIOCODEC_DTS\n", prtd);
			compr->codec = FORMAT_DTS_LBR;
			break;
		default:
			pr_debug("[%p] FORMAT_LINEAR_PCM\n", prtd);
			compr->codec = FORMAT_LINEAR_PCM;
			break;
		}
		return 0;
	case SNDRV_PCM_IOCTL1_RESET:
		pr_debug("[%p] %s: SNDRV_PCM_IOCTL1_RESET\n", prtd, __func__);
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK ||
				(substream->stream == SNDRV_PCM_STREAM_CAPTURE &&
				atomic_read(&prtd->start))) {
			
			eos_flush_check = 0;
			
			if (atomic_read(&prtd->eos)) {
				prtd->cmd_ack = 1;
				wake_up(&the_locks.eos_wait);
				atomic_set(&prtd->eos, 0);
				
				eos_flush_check = 1;
				
				pr_debug("[%p] %s: SNDRV_PCM_IOCTL1_RESET, set pending_buffer 1\n", prtd, __func__);
				atomic_set(&prtd->pending_buffer, 1);
			}

			
			if(eos_flush_check == 0){
			
				prtd->cmd_ack = 0;
				rc = q6asm_cmd(prtd->audio_client, CMD_FLUSH);
				if (rc < 0){
					pr_err("[%p] %s: flush cmd failed rc=%d\n", prtd, __func__, rc);
					return rc;
				}
				rc = wait_event_timeout(the_locks.flush_wait,
					prtd->cmd_ack, 5 * HZ);
				if (rc < 0)
					pr_err("[%p] Flush cmd timeout\n", prtd);
				atomic_set(&prtd->pending_buffer, 1);
			
			}
			
			prtd->pcm_irq_pos = 0;
		}
		break;
	case SNDRV_COMPRESS_DRAIN:
		pr_debug("[%p] %s: SNDRV_COMPRESS_DRAIN\n", prtd, __func__);
		atomic_set(&prtd->eos, 1);
		atomic_set(&prtd->pending_buffer, 0);
		prtd->cmd_ack = 0;
		q6asm_cmd_nowait(prtd->audio_client, CMD_EOS);

		
		rc = wait_event_interruptible(the_locks.eos_wait,
			prtd->cmd_ack);
		if (rc < 0)
			pr_err("[%p] SNDRV_COMPRESS_DRAIN EOS cmd timeout, rc = %d\n", prtd, rc);
		pr_debug("[%p] %s: SNDRV_COMPRESS_DRAIN	out of wait\n", prtd, __func__);
		return 0;
	case SNDRV_PCM_IOCTL1_ENABLE_EFFECT:
	{
		struct param {
			uint32_t effect_type; 
			uint32_t module_id;
			uint32_t param_id;
			uint32_t payload_size;
		} q6_param;
		void *payload;

		pr_info("[%p] %s: SNDRV_PCM_IOCTL1_ENABLE_EFFECT\n", prtd, __func__);
		if (copy_from_user(&q6_param, (void *) arg,
					sizeof(q6_param))) {
			pr_err("[%p] %s: copy param from user failed\n",
				prtd, __func__);
			return -EFAULT;
		}

		if (q6_param.payload_size <= 0 ||
		    (q6_param.effect_type != 0 &&
		     q6_param.effect_type != 1)) {
			pr_err("[%p] %s: unsupported param: %d, 0x%x, 0x%x, %d\n",
				prtd, __func__, q6_param.effect_type,
				q6_param.module_id, q6_param.param_id,
				q6_param.payload_size);
			return -EINVAL;
		}

		payload = kzalloc(q6_param.payload_size, GFP_KERNEL);
		if (!payload) {
			pr_err("[%p] %s: failed to allocate memory\n",
				prtd, __func__);
			return -ENOMEM;
		}
		if (copy_from_user(payload, (void *) (arg + sizeof(q6_param)),
			q6_param.payload_size)) {
			pr_err("[%p] %s: copy payload from user failed\n",
				prtd, __func__);
			kfree(payload);
			return -EFAULT;
		}

		if (q6_param.effect_type == 0) { 
			if (!prtd->audio_client) {
				pr_debug("%s: audio_client not found\n",
					__func__);
				kfree(payload);
				return -EACCES;
			}
			rc = q6asm_enable_effect(prtd->audio_client,
						q6_param.module_id,
						q6_param.param_id,
						q6_param.payload_size,
						payload);
			pr_info("[%p] %s: call q6asm_enable_effect, rc %d\n",
				prtd, __func__, rc);
		} else { 
			int port_id = msm_pcm_routing_get_port(substream);
			int index = afe_get_port_index(port_id);
			pr_info("[%p] %s: use copp topology, port id %d, index %d\n",
				prtd, __func__, port_id, index);
			if (port_id < 0) {
				pr_err("[%p] %s: invalid port_id %d\n",
					prtd, __func__, port_id);
			} else {
				rc = q6adm_enable_effect(index,
						     q6_param.module_id,
						     q6_param.param_id,
						     q6_param.payload_size,
						     payload);
				pr_info("[%p] %s: call q6adm_enable_effect, rc %d\n",
					prtd, __func__, rc);
			}
		}
#if Q6_EFFECT_DEBUG
		{
			int *ptr;
			int i;
			ptr = (int *)payload;
			for (i = 0; i < (q6_param.payload_size / 4); i++)
				pr_aud_info("[%p] 0x%08x", prtd, *(ptr + i));
		}
#endif
		kfree(payload);
		return rc;
	}

	default:
		break;
	}
	return snd_pcm_lib_ioctl(substream, cmd, arg);
}

static struct snd_pcm_ops msm_compr_ops = {
	.open	   = msm_compr_open,
	.hw_params	= msm_compr_hw_params,
	.close	  = msm_compr_close,
	.ioctl	  = msm_compr_ioctl,
	.prepare	= msm_compr_prepare,
	.trigger	= msm_compr_trigger,
	.pointer	= msm_compr_pointer,
	.mmap		= msm_compr_mmap,
};

static int msm_asoc_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	int ret = 0;

	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);
	return ret;
}

static struct snd_soc_platform_driver msm_soc_platform = {
	.ops		= &msm_compr_ops,
	.pcm_new	= msm_asoc_pcm_new,
};

static __devinit int msm_compr_probe(struct platform_device *pdev)
{
	pr_info("%s: dev name %s\n", __func__, dev_name(&pdev->dev));
	return snd_soc_register_platform(&pdev->dev,
				   &msm_soc_platform);
}

static int msm_compr_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static struct platform_driver msm_compr_driver = {
	.driver = {
		.name = "msm-compr-dsp",
		.owner = THIS_MODULE,
	},
	.probe = msm_compr_probe,
	.remove = __devexit_p(msm_compr_remove),
};

static int __init msm_soc_platform_init(void)
{
	init_waitqueue_head(&the_locks.enable_wait);
	init_waitqueue_head(&the_locks.eos_wait);
	init_waitqueue_head(&the_locks.write_wait);
	init_waitqueue_head(&the_locks.read_wait);
	init_waitqueue_head(&the_locks.flush_wait);

	wake_lock_init(&compr_lpa_q6_cb_wakelock, WAKE_LOCK_SUSPEND,
				"compr_lpa_q6_cb");

	wake_lock_init(&compr_lpa_wakelock, WAKE_LOCK_SUSPEND,
				"compr_lpa");

	return platform_driver_register(&msm_compr_driver);
}
module_init(msm_soc_platform_init);

static void __exit msm_soc_platform_exit(void)
{
	platform_driver_unregister(&msm_compr_driver);
}
module_exit(msm_soc_platform_exit);

MODULE_DESCRIPTION("PCM module platform driver");
MODULE_LICENSE("GPL v2");
