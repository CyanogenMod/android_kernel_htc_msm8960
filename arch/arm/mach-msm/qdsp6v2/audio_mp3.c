/* mp3 audio output device
 *
 * Copyright (C) 2008 Google, Inc.
 * Copyright (C) 2008 HTC Corporation
 * Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#include "audio_utils_aio.h"

#undef pr_info
#undef pr_err
#define pr_info(fmt, ...) pr_aud_info(fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...) pr_aud_err(fmt, ##__VA_ARGS__)

#define Q6_EFFECT_DEBUG 0

#ifdef CONFIG_DEBUG_FS
static const struct file_operations audio_mp3_debug_fops = {
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
		pr_debug("%s[%p]: AUDIO_START session_id[%d]\n", __func__,
						audio, audio->ac->session);
		if (audio->feedback == NON_TUNNEL_MODE) {
			
			rc = q6asm_enc_cfg_blk_pcm(audio->ac,
					audio->pcm_cfg.sample_rate,
					audio->pcm_cfg.channel_count);
			if (rc < 0) {
				pr_err("pcm output block config failed\n");
				break;
			}
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
	}
	return rc;
}

static int audio_open(struct inode *inode, struct file *file)
{
	struct q6audio_aio *audio = NULL;
	int rc = 0;
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
	
	char name[sizeof "msm_mp3_" + 5];
#endif
	audio = kzalloc(sizeof(struct q6audio_aio), GFP_KERNEL);

	if (audio == NULL) {
		pr_err("Could not allocate memory for mp3 decode driver\n");
		return -ENOMEM;
	}

	audio->pcm_cfg.buffer_size = PCM_BUFSZ_MIN;

	audio->ac = q6asm_audio_client_alloc((app_cb) q6_audio_cb,
					     (void *)audio);

	if (!audio->ac) {
		pr_err("Could not allocate memory for audio client\n");
		kfree(audio);
		return -ENOMEM;
	}

	
	if ((file->f_mode & FMODE_WRITE) && (file->f_mode & FMODE_READ)) {
		rc = q6asm_open_read_write(audio->ac, FORMAT_LINEAR_PCM,
					   FORMAT_MP3);
		if (rc < 0) {
			pr_err("NT mode Open failed rc=%d\n", rc);
			rc = -ENODEV;
			goto fail;
		}
		audio->feedback = NON_TUNNEL_MODE;
		audio->buf_cfg.meta_info_enable = 0x01;
	} else if ((file->f_mode & FMODE_WRITE) &&
			!(file->f_mode & FMODE_READ)) {
		rc = q6asm_open_write(audio->ac, FORMAT_MP3);
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
	snprintf(name, sizeof name, "msm_mp3_%04x", audio->ac->session);
	audio->dentry = debugfs_create_file(name, S_IFREG | S_IRUGO,
					    NULL, (void *)audio,
					    &audio_mp3_debug_fops);

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

	pr_info("%s:mp3dec success mode[%d]session[%d]\n", __func__,
						audio->feedback,
						audio->ac->session);
	return rc;
fail:
	q6asm_audio_client_free(audio->ac);
	kfree(audio);
	return rc;
}

static const struct file_operations audio_mp3_fops = {
	.owner = THIS_MODULE,
	.open = audio_open,
	.release = audio_aio_release,
	.unlocked_ioctl = audio_ioctl,
	.fsync = audio_aio_fsync,
};

struct miscdevice audio_mp3_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "msm_mp3",
	.fops = &audio_mp3_fops,
};

static int __init audio_mp3_init(void)
{
	return misc_register(&audio_mp3_misc);
}

device_initcall(audio_mp3_init);
