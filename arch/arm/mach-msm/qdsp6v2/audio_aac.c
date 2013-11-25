/* aac audio output device
 *
 * Copyright (C) 2008 Google, Inc.
 * Copyright (C) 2008 HTC Corporation
 * Copyright (c) 2010-2012, The Linux Foundation. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/msm_audio_aac.h>
#include "audio_utils_aio.h"

#undef pr_info
#undef pr_err
#define pr_info(fmt, ...) pr_aud_info(fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...) pr_aud_err(fmt, ##__VA_ARGS__)

#define AUDIO_AAC_DUAL_MONO_INVALID -1
#define PCM_BUFSZ_MIN_AAC	((8*1024) + sizeof(struct dec_meta_out))
#define Q6_EFFECT_DEBUG 0

#ifdef CONFIG_DEBUG_FS
static const struct file_operations audio_aac_debug_fops = {
	.read = audio_aio_debug_read,
	.open = audio_aio_debug_open,
};
#endif

static long audio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct q6audio_aio *audio = file->private_data;
	int rc = 0;
	switch (cmd) {
	case AUDIO_START: {
		struct asm_aac_cfg aac_cfg;
		struct msm_audio_aac_config *aac_config;
		uint32_t sbr_ps = 0x00;
		pr_debug("%s: AUDIO_START session_id[%d]\n", __func__,
							audio->ac->session);
		if (audio->feedback == NON_TUNNEL_MODE) {
			
			rc = q6asm_enc_cfg_blk_pcm(audio->ac, 0, 0);
			if (rc < 0) {
				pr_err("pcm output block config failed\n");
				break;
			}
		}
		
		rc = q6asm_enable_sbrps(audio->ac, sbr_ps);
		if (rc < 0)
			pr_err("sbr-ps enable failed\n");
		aac_config = (struct msm_audio_aac_config *)audio->codec_cfg;
		if (aac_config->sbr_ps_on_flag)
			aac_cfg.aot = AAC_ENC_MODE_EAAC_P;
		else if (aac_config->sbr_on_flag)
			aac_cfg.aot = AAC_ENC_MODE_AAC_P;
		else
			aac_cfg.aot = AAC_ENC_MODE_AAC_LC;

		switch (aac_config->format) {
		case AUDIO_AAC_FORMAT_ADTS:
			aac_cfg.format = 0x00;
			break;
		case AUDIO_AAC_FORMAT_LOAS:
			aac_cfg.format = 0x01;
			break;
		case AUDIO_AAC_FORMAT_ADIF:
			aac_cfg.format = 0x02;
			break;
		default:
		case AUDIO_AAC_FORMAT_RAW:
			aac_cfg.format = 0x03;
		}
		aac_cfg.ep_config = aac_config->ep_config;
		aac_cfg.section_data_resilience =
			aac_config->aac_section_data_resilience_flag;
		aac_cfg.scalefactor_data_resilience =
			aac_config->aac_scalefactor_data_resilience_flag;
		aac_cfg.spectral_data_resilience =
			aac_config->aac_spectral_data_resilience_flag;
		aac_cfg.ch_cfg = audio->pcm_cfg.channel_count;
		if (audio->feedback == TUNNEL_MODE) {
			aac_cfg.sample_rate = aac_config->sample_rate;
			aac_cfg.ch_cfg = aac_config->channel_configuration;
		} else {
			aac_cfg.sample_rate =  audio->pcm_cfg.sample_rate;
			aac_cfg.ch_cfg = audio->pcm_cfg.channel_count;
		}

		pr_debug("%s:format=%x aot=%d  ch=%d sr=%d\n",
			__func__, aac_cfg.format,
			aac_cfg.aot, aac_cfg.ch_cfg,
			aac_cfg.sample_rate);

		
		rc = q6asm_media_format_block_aac(audio->ac, &aac_cfg);
		if (rc < 0) {
			pr_err("cmd media format block failed\n");
			break;
		}
		rc = audio_aio_enable(audio);
		audio->eos_rsp = 0;
		audio->eos_flag = 0;
		if (!rc) {
			audio->enabled = 1;
		} else {
			audio->enabled = 0;
			pr_err("Audio Start procedure failed rc=%d\n", rc);
			break;
		}
		pr_info("%s: AUDIO_START sessionid[%d]enable[%d]\n", __func__,
						audio->ac->session,
						audio->enabled);
		if (audio->stopped == 1)
			audio->stopped = 0;
		break;
	}
	case AUDIO_GET_AAC_CONFIG: {
		if (copy_to_user((void *)arg, audio->codec_cfg,
			sizeof(struct msm_audio_aac_config))) {
			rc = -EFAULT;
			break;
		}
		break;
	}
	case AUDIO_SET_AAC_CONFIG: {
		struct msm_audio_aac_config *aac_config;
		pr_debug("%s: AUDIO_SET_AAC_CONFIG\n", __func__);
		if (copy_from_user(audio->codec_cfg, (void *)arg,
			sizeof(struct msm_audio_aac_config))) {
			rc = -EFAULT;
			break;
		} else {
			uint16_t sce_left = 1, sce_right = 2;
			aac_config = audio->codec_cfg;
			if ((aac_config->dual_mono_mode <
				AUDIO_AAC_DUAL_MONO_PL_PR) ||
				(aac_config->dual_mono_mode >
				AUDIO_AAC_DUAL_MONO_PL_SR)) {
				pr_err("%s:AUDIO_SET_AAC_CONFIG: Invalid"
					"dual_mono mode =%d\n", __func__,
					aac_config->dual_mono_mode);
			} else {
				pr_debug("%s: AUDIO_SET_AAC_CONFIG: modify"
					 "dual_mono mode =%d\n", __func__,
					 aac_config->dual_mono_mode);
				switch (aac_config->dual_mono_mode) {
				case AUDIO_AAC_DUAL_MONO_PL_PR:
					sce_left = 1;
					sce_right = 1;
					break;
				case AUDIO_AAC_DUAL_MONO_SL_SR:
					sce_left = 2;
					sce_right = 2;
					break;
				case AUDIO_AAC_DUAL_MONO_SL_PR:
					sce_left = 2;
					sce_right = 1;
					break;
				case AUDIO_AAC_DUAL_MONO_PL_SR:
				default:
					sce_left = 1;
					sce_right = 2;
					break;
				}
				rc = q6asm_cfg_dual_mono_aac(audio->ac,
							sce_left, sce_right);
				if (rc < 0)
					pr_err("%s: asm cmd dualmono failed"
						" rc=%d\n", __func__, rc);
			}
		}
		break;
	}
	case AUDIO_SET_Q6_EFFECT: {
		struct param {
			uint32_t effect_type; 
			uint32_t module_id;
			uint32_t param_id;
			uint32_t payload_size;
		} q6_param;
		void *payload;

		pr_aud_info("AUDIO_SET_Q6_EFFECT, session %d ++++\n", audio->ac->session);
		if (copy_from_user(&q6_param, (void *) arg,
					sizeof(q6_param))) {
			pr_aud_err("%s: copy param from user failed\n",
				__func__);
			rc = -EFAULT;
			break;
		}

		if (q6_param.payload_size <= 0 ||
		    (q6_param.effect_type != 0 &&
		     q6_param.effect_type != 1)) {
			pr_aud_err("%s: unsupported param: %d, 0x%x, 0x%x, %d\n",
				__func__, q6_param.effect_type,
				q6_param.module_id, q6_param.param_id,
				q6_param.payload_size);
			rc = -EINVAL;
			break;
		}

		payload = kzalloc(q6_param.payload_size, GFP_KERNEL);
		if (!payload) {
			pr_aud_err("%s: failed to allocate memory\n",
				__func__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(payload, (void *) (arg + sizeof(q6_param)),
			q6_param.payload_size)) {
			pr_aud_err("%s: copy payload from user failed\n",
				__func__);
			kfree(payload);
			rc = -EFAULT;
			break;;
		}

		if (q6_param.effect_type == 0) { 
			rc = q6asm_enable_effect(audio->ac,
						q6_param.module_id,
						q6_param.param_id,
						q6_param.payload_size,
						payload);
			pr_aud_info("q6asm_enable_effect, return %d (session %d)\n", rc, audio->ac->session);
		}
#if Q6_EFFECT_DEBUG
		{
			int *ptr;
			int i;
			ptr = (int *)payload;
			for (i = 0; i < (q6_param.payload_size / 4); i++)
				pr_aud_info("0x%08x", *(ptr + i));
		}
#endif
		kfree(payload);
		pr_aud_info("AUDIO_SET_Q6_EFFECT, session %d ---\n", audio->ac->session);
		break;
	}

	default:
		
		rc = audio->codec_ioctl(file, cmd, arg);
		if (rc)
			pr_err("%s[%p]:Failed in utils_ioctl: %d\n",
				__func__, audio, rc);
	}
	return rc;
}

static int audio_open(struct inode *inode, struct file *file)
{
	struct q6audio_aio *audio = NULL;
	int rc = 0;
	struct msm_audio_aac_config *aac_config = NULL;
	struct asm_softpause_params softpause = {
		.enable = SOFT_PAUSE_ENABLE,
		.period = SOFT_PAUSE_PERIOD * 3,
		.step = SOFT_PAUSE_STEP,
		.rampingcurve = SOFT_PAUSE_CURVE_LINEAR,
	};
	struct asm_softvolume_params softvol = {
		.period = SOFT_VOLUME_PERIOD,
		.step = SOFT_VOLUME_STEP,
		.rampingcurve = SOFT_VOLUME_CURVE_LINEAR,
	};
#ifdef CONFIG_DEBUG_FS
	
	char name[sizeof "msm_aac_" + 5];
#endif
	audio = kzalloc(sizeof(struct q6audio_aio), GFP_KERNEL);

	if (audio == NULL) {
		pr_err("Could not allocate memory for aac decode driver\n");
		return -ENOMEM;
	}
	audio->codec_cfg = kzalloc(sizeof(struct msm_audio_aac_config),
					GFP_KERNEL);
	if (audio->codec_cfg == NULL) {
		pr_err("%s:Could not allocate memory for aac"
			"config\n", __func__);
		kfree(audio);
		return -ENOMEM;
	}
	aac_config = audio->codec_cfg;

	audio->pcm_cfg.buffer_size = PCM_BUFSZ_MIN_AAC;
	aac_config->dual_mono_mode = AUDIO_AAC_DUAL_MONO_INVALID;

	audio->ac = q6asm_audio_client_alloc((app_cb) q6_audio_cb,
					     (void *)audio);

	if (!audio->ac) {
		pr_err("Could not allocate memory for audio client\n");
		kfree(audio->codec_cfg);
		kfree(audio);
		return -ENOMEM;
	}

	
	if ((file->f_mode & FMODE_WRITE) && (file->f_mode & FMODE_READ)) {
		rc = q6asm_open_read_write(audio->ac, FORMAT_LINEAR_PCM,
					   FORMAT_MPEG4_AAC);
		if (rc < 0) {
			pr_err("NT mode Open failed rc=%d\n", rc);
			rc = -ENODEV;
			goto fail;
		}
		audio->feedback = NON_TUNNEL_MODE;
		audio->buf_cfg.meta_info_enable = 0x01;
	} else if ((file->f_mode & FMODE_WRITE) &&
			!(file->f_mode & FMODE_READ)) {
		rc = q6asm_open_write(audio->ac, FORMAT_MPEG4_AAC);
		if (rc < 0) {
			pr_err("T mode Open failed rc=%d\n", rc);
			rc = -ENODEV;
			goto fail;
		}
		audio->feedback = TUNNEL_MODE;
		audio->buf_cfg.meta_info_enable = 0x00;
	} else {
		pr_err("Not supported mode\n");
		rc = -EACCES;
		goto fail;
	}
	rc = audio_aio_open(audio, file);

#ifdef CONFIG_DEBUG_FS
	snprintf(name, sizeof name, "msm_aac_%04x", audio->ac->session);
	audio->dentry = debugfs_create_file(name, S_IFREG | S_IRUGO,
					    NULL, (void *)audio,
					    &audio_aac_debug_fops);

	if (IS_ERR(audio->dentry))
		pr_debug("debugfs_create_file failed\n");
#endif

	if (softpause.rampingcurve == SOFT_PAUSE_CURVE_LINEAR)
		softpause.step = SOFT_PAUSE_STEP_LINEAR;
	if (softvol.rampingcurve == SOFT_VOLUME_CURVE_LINEAR)
		softvol.step = SOFT_VOLUME_STEP_LINEAR;
	rc = q6asm_set_volume(audio->ac, 0);
	if (rc < 0)
		pr_err("%s: Send Volume command failed rc=%d\n",
			__func__, rc);
	rc = q6asm_set_softpause(audio->ac, &softpause);
	if (rc < 0)
		pr_err("%s: Send SoftPause Param failed rc=%d\n",
			__func__, rc);
	rc = q6asm_set_softvolume(audio->ac, &softvol);
	if (rc < 0)
		pr_err("%s: Send SoftVolume Param failed rc=%d\n",
			__func__, rc);
	
	rc = q6asm_set_mute(audio->ac, 0);
	if (rc < 0)
		pr_err("%s: Send mute command failed rc=%d\n",
			__func__, rc);

	pr_info("%s:aacdec success mode[%d]session[%d]\n", __func__,
						audio->feedback,
						audio->ac->session);
	return rc;
fail:
	q6asm_audio_client_free(audio->ac);
	kfree(audio->codec_cfg);
	kfree(audio);
	return rc;
}

static const struct file_operations audio_aac_fops = {
	.owner = THIS_MODULE,
	.open = audio_open,
	.release = audio_aio_release,
	.unlocked_ioctl = audio_ioctl,
	.fsync = audio_aio_fsync,
};

struct miscdevice audio_aac_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "msm_aac",
	.fops = &audio_aac_fops,
};

static int __init audio_aac_init(void)
{
	return misc_register(&audio_aac_misc);
}

device_initcall(audio_aac_init);
