/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * HTC: zara machine driver which defines board-specific data
 * Copy from sound/soc/msm/msm8930.c
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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/mfd/pm8xxx/misc.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/q6asm.h>
#include <sound/jack.h>
#include <asm/mach-types.h>
#include <mach/socinfo.h>
#include <linux/module.h>
#include <mach/htc_acoustic_8960.h>
#include <mach/htc_audiogpio_8930.h>
#include <linux/tfa9887.h>
#include <linux/rt5501.h>
#include "board-zara.h"
#include "../../../../../sound/soc/msm/msm-pcm-routing.h"
#include "../../../../../sound/soc/msm/msm-compr-q6.h"
#include "../../../../../sound/soc/codecs/wcd9304.h"

#define MSM8930_SPK_ON 1
#define MSM8930_SPK_OFF 0

#define BTSCO_RATE_8KHZ 8000
#define BTSCO_RATE_16KHZ 16000

#define SITAR_EXT_CLK_RATE 12288000

#define SITAR_MBHC_DEF_BUTTONS 8
#define SITAR_MBHC_DEF_RLOADS 5
#define BOTTOM_SPK_AMP_POS	0x1
#define BOTTOM_SPK_AMP_NEG	0x2
#define TOP_SPK_AMP_POS		0x4
#define TOP_SPK_AMP_NEG		0x8
#define TOP_SPK_AMP		0x10
#define HS_AMP_POS		0x20
#define HS_AMP_NEG		0x40
#define RCV_AMP_POS		0x80
#define RCV_AMP_NEG		0x100

static int msm8930_spk_control;
static int msm8930_hac_control;
static int msm8930_ext_bottom_spk_pamp;
static int msm8930_ext_top_spk_pamp;
static int msm8930_hs_pamp;
static int msm8930_rcv_pamp;
extern struct htc_8930_gpio_pdata htc_audio_gpio;
static int msm8930_slim_0_rx_ch = 1;
static int msm8930_slim_0_tx_ch = 1;

static int msm8930_btsco_rate = BTSCO_RATE_8KHZ;
static int msm8930_btsco_ch = 1;
static int aux_pcm_open = 0;

static struct mutex aux_pcm_mutex;
static struct clk *codec_clk;
static int clk_users;

static struct snd_soc_jack hs_jack;
static struct snd_soc_jack button_jack;

static atomic_t q6_effect_mode = ATOMIC_INIT(-1);
extern unsigned int system_rev;
extern unsigned int engineerid;
extern unsigned skuid;

static int msm8930_enable_codec_ext_clk(
		struct snd_soc_codec *codec, int enable,
		bool dapm);

static inline int param_is_mask(int p)
{
	return ((p >= SNDRV_PCM_HW_PARAM_FIRST_MASK) &&
		(p <= SNDRV_PCM_HW_PARAM_LAST_MASK));
}

static inline struct snd_mask *param_to_mask(struct snd_pcm_hw_params *p, int n)
{
	return &(p->masks[n - SNDRV_PCM_HW_PARAM_FIRST_MASK]);
}

static void param_set_mask(struct snd_pcm_hw_params *p, int n, unsigned bit)
{
	if (bit >= SNDRV_MASK_MAX)
		return;
	if (param_is_mask(n)) {
		struct snd_mask *m = param_to_mask(p, n);
		m->bits[0] = 0;
		m->bits[1] = 0;
		m->bits[bit >> 5] |= (1 << (bit & 31));
	}
}

static int configure_gpios(struct request_gpio *gpio, unsigned int size)
{
	int	rtn;
	int	i;
	int	j;
	for (i = 0; i < size; i++) {
		if (gpio[i].gpio_no < 0) continue;
		rtn = gpio_request(gpio[i].gpio_no,
							gpio[i].gpio_name);
		pr_debug("%s: gpio = %d, gpio name = %s, rtn = %d\n",
					__func__,
					gpio[i].gpio_no,
					gpio[i].gpio_name,
					rtn);
		if (rtn) {
			pr_err("%s: Failed to request gpio %d\n",
					__func__,
					gpio[i].gpio_no);
			for (j = i; j >= 0; j--)
				gpio_free(gpio[j].gpio_no);
			goto err;
		}
	}
err:
	return rtn;
}

static int free_gpios(struct request_gpio *gpio, unsigned int size)
{
	int	i;
	for (i = 0; i < size; i++) {
		if (gpio[i].gpio_no < 0) continue;
		gpio_free(gpio[i].gpio_no);
	}
	return 0;
}

static struct clk *pri_i2s_osr_clk;
static struct clk *pri_i2s_bit_clk;
static atomic_t pri_i2s_rsc_ref;
static struct clk *mi2s_osr_clk;
static struct clk *mi2s_bit_clk;
static atomic_t mi2s_rsc_ref;

static int msm8930_mi2s_hw_params(struct snd_pcm_substream *substream,
			struct snd_pcm_hw_params *params)
{
	int rate = params_rate(params);
	int bit_clk_set = 0;
	int clock = 18432000;
	int bits = 24;

	bit_clk_set = clock/(rate * 2 * bits);
	clk_set_rate(mi2s_bit_clk, bit_clk_set);
	pr_info("%s: mi2s bit_clk_set = %d, clock = %d, bit width = %d", __func__, bit_clk_set, clock, bits);

	return 1;
}

static void msm8930_mi2s_shutdown(struct snd_pcm_substream *substream)
{
#ifdef CONFIG_AMP_TFA9887
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		pr_debug("spk amp off ++");
		htc_audio_gpio.amp_speaker(false);
		pr_debug("spk amp off --");
	}
#endif
	if (atomic_dec_return(&mi2s_rsc_ref) == 0) {
		pr_info("%s: free mi2s resources\n", __func__);
		if (mi2s_bit_clk) {
			clk_disable_unprepare(mi2s_bit_clk);
			clk_put(mi2s_bit_clk);
			mi2s_bit_clk = NULL;
		}

		if (mi2s_osr_clk) {
			clk_disable_unprepare(mi2s_osr_clk);
			clk_put(mi2s_osr_clk);
			mi2s_osr_clk = NULL;
		}

		free_gpios(htc_audio_gpio.mi2s_gpio, ARRAY_SIZE(htc_audio_gpio.mi2s_gpio));
	}
}

static int msm8930_mi2s_startup(struct snd_pcm_substream *substream)
{
	int ret = 0;
	int clock = 12288000;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;

	pr_debug("%s: dai name %s %p\n", __func__, cpu_dai->name, cpu_dai->dev);

	if (atomic_inc_return(&mi2s_rsc_ref) == 1) {
		pr_info("%s: acquire mi2s resources\n", __func__);
		configure_gpios(htc_audio_gpio.mi2s_gpio, ARRAY_SIZE(htc_audio_gpio.mi2s_gpio));
		mi2s_osr_clk = clk_get(cpu_dai->dev, "osr_clk");
		if (!IS_ERR(mi2s_osr_clk)) {

			clk_set_rate(mi2s_osr_clk, clock);
			clk_prepare_enable(mi2s_osr_clk);
		} else
			pr_err("Failed to get mi2s_osr_clk\n");
		mi2s_bit_clk = clk_get(cpu_dai->dev, "bit_clk");
		if (IS_ERR(mi2s_bit_clk)) {
			pr_err("Failed to get mi2s_bit_clk\n");
			clk_disable_unprepare(mi2s_osr_clk);
			clk_put(mi2s_osr_clk);
			return PTR_ERR(mi2s_bit_clk);
		}
		clk_set_rate(mi2s_bit_clk, 8);
		ret = clk_prepare_enable(mi2s_bit_clk);
		if (ret != 0) {
			pr_err("Unable to enable mi2s_rx_bit_clk\n");
			clk_put(mi2s_bit_clk);
			clk_disable_unprepare(mi2s_osr_clk);
			clk_put(mi2s_osr_clk);
			return ret;
		}
		ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_CBS_CFS);
		if (ret < 0)
			ret = snd_soc_dai_set_fmt(codec_dai,
						  SND_SOC_DAIFMT_CBS_CFS);
		if (ret < 0)
			pr_err("set format for codec dai failed\n");
	}
#ifdef CONFIG_AMP_TFA9887
	pr_debug("spk amp on ++");
	htc_audio_gpio.amp_speaker(true);
	pr_debug("spk amp on --");
#endif
	return ret;
}

static struct snd_soc_ops msm8930_mi2s_be_ops = {
	.startup = msm8930_mi2s_startup,
	.shutdown = msm8930_mi2s_shutdown,
	.hw_params = msm8930_mi2s_hw_params,
};

static int msm8930_i2s_hw_params(struct snd_pcm_substream *substream,
			struct snd_pcm_hw_params *params)
{
	int rate = params_rate(params);
	int bit_clk_set = 0;

	bit_clk_set = 12288000/(rate * 2 * 16);
	pr_info("%s, bit clock is %d\n", __func__, bit_clk_set);
	clk_set_rate(pri_i2s_bit_clk, bit_clk_set);

	return 1;
}

static void msm8930_i2s_shutdown(struct snd_pcm_substream *substream)
{
	pr_debug("%s(): substream = %s\n", __func__, substream->name);
	if (atomic_dec_return(&pri_i2s_rsc_ref) == 0) {
		if (pri_i2s_bit_clk) {
			clk_disable_unprepare(pri_i2s_bit_clk);
			clk_put(pri_i2s_bit_clk);
			pri_i2s_bit_clk = NULL;
		}
		if (pri_i2s_osr_clk) {
			clk_disable_unprepare(pri_i2s_osr_clk);
			clk_put(pri_i2s_osr_clk);
			pri_i2s_osr_clk = NULL;
		}
		free_gpios(htc_audio_gpio.i2s_gpio, ARRAY_SIZE(htc_audio_gpio.i2s_gpio));
	}
}

static int msm8930_i2s_startup(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;

	pr_debug("%s(): substream = %s\n", __func__, substream->name);
	if (atomic_inc_return(&pri_i2s_rsc_ref) == 1) {
		configure_gpios(htc_audio_gpio.i2s_gpio, ARRAY_SIZE(htc_audio_gpio.i2s_gpio));

		pri_i2s_osr_clk = clk_get(cpu_dai->dev, "osr_clk");
		if (IS_ERR(pri_i2s_osr_clk)) {
			pr_err("Failed to get pri_i2s_osr_clk\n");
			return PTR_ERR(pri_i2s_osr_clk);
		}
		clk_set_rate(pri_i2s_osr_clk, 12288000);
		clk_prepare_enable(pri_i2s_osr_clk);

		pri_i2s_bit_clk = clk_get(cpu_dai->dev, "bit_clk");
		if (IS_ERR(pri_i2s_bit_clk)) {
			pr_err("Failed to get pri_i2s_bit_clk\n");
			clk_disable_unprepare(pri_i2s_osr_clk);
			clk_put(pri_i2s_osr_clk);
			return PTR_ERR(pri_i2s_bit_clk);
		}
		clk_set_rate(pri_i2s_bit_clk, 8);
		ret = clk_prepare_enable(pri_i2s_bit_clk);
		if (ret != 0) {
			pr_err("Unable to enable pri_i2s_bit_clk\n");
			clk_put(pri_i2s_bit_clk);
			clk_disable_unprepare(pri_i2s_osr_clk);
			clk_put(pri_i2s_osr_clk);
			return ret;
		}

		ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_CBS_CFS);
		if (ret < 0) {
			pr_info("%s: set format for cpu dai failed\n", __func__);
			ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_CBS_CFS);
		}
		if (ret < 0)
			pr_err("set format for codec dai failed\n");
	}
	return ret;
}

static struct snd_soc_ops msm8930_i2s_be_ops = {
	.startup = msm8930_i2s_startup,
	.shutdown = msm8930_i2s_shutdown,
	.hw_params = msm8930_i2s_hw_params,
};

static void msm8930_ext_spk_power_amp_on(u32 spk)
{
	if (spk & (RCV_AMP_POS | RCV_AMP_NEG)) {
		if ((msm8930_rcv_pamp & RCV_AMP_POS) &&
			(msm8930_rcv_pamp & RCV_AMP_NEG)) {
			pr_debug("%s() HS Ampl already "
				"turned on. spk = 0x%08x\n", __func__, spk);
			return;
		}

		msm8930_rcv_pamp |= spk;
		if ((msm8930_rcv_pamp & RCV_AMP_POS) &&
			(msm8930_rcv_pamp & RCV_AMP_NEG)) {
			pr_debug("rcv amp on++");
			htc_audio_gpio.amp_receiver(true);
			pr_debug("rcv amp on--");

			pr_debug("%s: slepping 4 ms after turning on external "
				" Bottom Speaker Ampl\n", __func__);
			usleep_range(4000, 4000);
		}
	} else if (spk & (HS_AMP_POS | HS_AMP_NEG)) {
		if ((msm8930_hs_pamp & HS_AMP_POS) &&
			(msm8930_hs_pamp & HS_AMP_NEG)) {
			pr_debug("%s() HS Ampl already "
				"turned on. spk = 0x%08x\n", __func__, spk);
			return;
		}

		msm8930_hs_pamp |= spk;
		pr_debug("hs amp on++");
		htc_audio_gpio.amp_headset(true);
		pr_debug("hs amp on--");
		pr_debug("%s: slepping 4 ms after turning on external "
			" Bottom Speaker Ampl\n", __func__);
		usleep_range(4000, 4000);
	} else if (spk & (BOTTOM_SPK_AMP_POS | BOTTOM_SPK_AMP_NEG)) {
		if ((msm8930_ext_bottom_spk_pamp & BOTTOM_SPK_AMP_POS) &&
			(msm8930_ext_bottom_spk_pamp & BOTTOM_SPK_AMP_NEG)) {
			pr_debug("%s() External Bottom Speaker Ampl already "
				"turned on. spk = 0x%08x\n", __func__, spk);
			return;
		}

		msm8930_ext_bottom_spk_pamp |= spk;
#ifndef CONFIG_AMP_TFA9887
		pr_info("spk amp on ++");
		htc_audio_gpio.amp_speaker(true);
		pr_info("spk amp on --");
#endif
		if ((msm8930_ext_bottom_spk_pamp & BOTTOM_SPK_AMP_POS) &&
			(msm8930_ext_bottom_spk_pamp & BOTTOM_SPK_AMP_NEG)) {
			pr_debug("%s: slepping 4 ms after turning on external "
				" Bottom Speaker Ampl\n", __func__);
			usleep_range(4000, 4000);
		}
	} else if (spk & (TOP_SPK_AMP_POS | TOP_SPK_AMP_NEG | TOP_SPK_AMP)) {
		pr_debug("%s():top_spk_amp_state = 0x%x spk_event = 0x%x\n",
			__func__, msm8930_ext_top_spk_pamp, spk);
		if (((msm8930_ext_top_spk_pamp & TOP_SPK_AMP_POS) &&
			(msm8930_ext_top_spk_pamp & TOP_SPK_AMP_NEG)) ||
			(msm8930_ext_top_spk_pamp & TOP_SPK_AMP)) {
			pr_debug("%s() External Top Speaker Ampl already"
				"turned on. spk = 0x%08x\n", __func__, spk);
			return;
		}

		msm8930_ext_top_spk_pamp |= spk;
		if (((msm8930_ext_top_spk_pamp & TOP_SPK_AMP_POS) &&
			(msm8930_ext_top_spk_pamp & TOP_SPK_AMP_NEG)) ||
				(msm8930_ext_top_spk_pamp & TOP_SPK_AMP)) {
				pr_debug("%s: sleeping 4 ms after turning on "
				" external Top Speaker Ampl\n", __func__);
			usleep_range(4000, 4000);
		}
	} else {
		pr_err("%s: ERROR : Invalid External Speaker Ampl. spk = 0x%08x\n",
			__func__, spk);
		return;
	}
}

static void msm8930_ext_spk_power_amp_off(u32 spk)
{
	if (spk & (RCV_AMP_POS | RCV_AMP_NEG)) {
		if (!msm8930_rcv_pamp)
			return;

		pr_debug("rcv amp off ++");
		htc_audio_gpio.amp_receiver(false);
		pr_debug("rcv amp off --");

		msm8930_rcv_pamp = 0;

		pr_debug("%s: sleeping 4 ms after turning off external Bottom"
			" Speaker Ampl\n", __func__);
		usleep_range(4000, 4000);
	} else if (spk & (HS_AMP_POS | HS_AMP_NEG)) {
		if (!msm8930_hs_pamp)
			return;

		pr_debug("hs amp off++");
		htc_audio_gpio.amp_headset(false);
		pr_debug("hs amp off--");

		msm8930_hs_pamp = 0;

		pr_debug("%s: sleeping 4 ms after turning off external Bottom"
			" Speaker Ampl\n", __func__);
		usleep_range(4000, 4000);
	} else if (spk & (BOTTOM_SPK_AMP_POS | BOTTOM_SPK_AMP_NEG)) {
		if (!msm8930_ext_bottom_spk_pamp)
			return;

#ifndef CONFIG_AMP_TFA9887
		pr_debug("spk amp off ++");
		htc_audio_gpio.amp_speaker(false);
		pr_debug("spk amp off --");
#endif

		msm8930_ext_bottom_spk_pamp = 0;

		pr_debug("%s: sleeping 4 ms after turning off external Bottom"
			" Speaker Ampl\n", __func__);
		usleep_range(4000, 4000);
	} else if (spk & (TOP_SPK_AMP_POS | TOP_SPK_AMP_NEG | TOP_SPK_AMP)) {
		pr_debug("%s: top_spk_amp_state = 0x%x spk_event = 0x%x\n",
				__func__, msm8930_ext_top_spk_pamp, spk);

		if (!msm8930_ext_top_spk_pamp)
			return;

		if ((spk & TOP_SPK_AMP_POS) || (spk & TOP_SPK_AMP_NEG)) {
			msm8930_ext_top_spk_pamp &= (~(TOP_SPK_AMP_POS |
							TOP_SPK_AMP_NEG));
		} else if (spk & TOP_SPK_AMP) {
			msm8930_ext_top_spk_pamp &=  ~TOP_SPK_AMP;
		}

		if (msm8930_ext_top_spk_pamp)
			return;

		msm8930_ext_top_spk_pamp = 0;

		pr_debug("%s: sleeping 4 ms after ext Top Spek Ampl is off\n",
				__func__);
		usleep_range(4000, 4000);
	} else  {
		pr_err("%s: ERROR : Invalid Ext Spk Ampl. spk = 0x%08x\n",
			__func__, spk);
		return;
	}
}

static void msm8930_ext_control(struct snd_soc_codec *codec)
{
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	pr_debug("%s: msm8930_spk_control = %d", __func__, msm8930_spk_control);
	if (msm8930_spk_control == MSM8930_SPK_ON) {
		snd_soc_dapm_enable_pin(dapm, "Ext Spk Bottom Pos");
		snd_soc_dapm_enable_pin(dapm, "Ext Spk Bottom Neg");
		snd_soc_dapm_enable_pin(dapm, "Ext Spk Top Pos");
		snd_soc_dapm_enable_pin(dapm, "Ext Spk Top Neg");
		snd_soc_dapm_enable_pin(dapm, "Ext Hs Pos");
		snd_soc_dapm_enable_pin(dapm, "Ext Hs Neg");
		snd_soc_dapm_enable_pin(dapm, "Ext Rcv Pos");
		snd_soc_dapm_enable_pin(dapm, "Ext Rcv Neg");
	} else {
		snd_soc_dapm_disable_pin(dapm, "Ext Spk Bottom Pos");
		snd_soc_dapm_disable_pin(dapm, "Ext Spk Bottom Neg");
		snd_soc_dapm_disable_pin(dapm, "Ext Spk Top Pos");
		snd_soc_dapm_disable_pin(dapm, "Ext Spk Top Neg");
		snd_soc_dapm_disable_pin(dapm, "Ext Hs Pos");
		snd_soc_dapm_disable_pin(dapm, "Ext Hs Neg");
		snd_soc_dapm_disable_pin(dapm, "Ext Rcv Pos");
		snd_soc_dapm_disable_pin(dapm, "Ext Rcv Neg");
	}

	snd_soc_dapm_sync(dapm);
}

static int msm8930_get_spk(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm8930_spk_control = %d", __func__, msm8930_spk_control);
	ucontrol->value.integer.value[0] = msm8930_spk_control;
	return 0;
}

static int msm8930_set_spk(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	pr_debug("%s()\n", __func__);
	if (msm8930_spk_control == ucontrol->value.integer.value[0])
		return 0;

	msm8930_spk_control = ucontrol->value.integer.value[0];
	msm8930_ext_control(codec);
	return 1;
}

static int msm8930_spkramp_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *k, int event)
{
	pr_info("%s() wname %s %x\n", __func__, w->name, SND_SOC_DAPM_EVENT_ON(event));
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		if (!strncmp(w->name, "Ext Spk Bottom Pos", 18))
			msm8930_ext_spk_power_amp_on(BOTTOM_SPK_AMP_POS);
		else if (!strncmp(w->name, "Ext Spk Bottom Neg", 18))
			msm8930_ext_spk_power_amp_on(BOTTOM_SPK_AMP_NEG);
		else if (!strncmp(w->name, "Ext Spk Top Pos", 15))
			msm8930_ext_spk_power_amp_on(TOP_SPK_AMP_POS);
		else if  (!strncmp(w->name, "Ext Spk Top Neg", 15))
			msm8930_ext_spk_power_amp_on(TOP_SPK_AMP_NEG);
		else if (!strncmp(w->name, "Ext Hs Pos", 10))
			msm8930_ext_spk_power_amp_on(HS_AMP_POS);
		else if  (!strncmp(w->name, "Ext Hs Neg", 10))
			msm8930_ext_spk_power_amp_on(HS_AMP_NEG);
		else if (!strncmp(w->name, "Ext Rcv Pos", 11))
			msm8930_ext_spk_power_amp_on(RCV_AMP_POS);
		else if  (!strncmp(w->name, "Ext Rcv Neg", 11))
			msm8930_ext_spk_power_amp_on(RCV_AMP_NEG);
		else if  (!strncmp(w->name, "Ext Spk Top", 12))
			msm8930_ext_spk_power_amp_on(TOP_SPK_AMP);
		else {
			pr_err("%s() Invalid Amplifier Widget = %s\n",
					__func__, w->name);
			return -EINVAL;
		}
	} else {
		if (!strncmp(w->name, "Ext Spk Bottom Pos", 18))
			msm8930_ext_spk_power_amp_off(BOTTOM_SPK_AMP_POS);
		else if (!strncmp(w->name, "Ext Spk Bottom Neg", 18))
			msm8930_ext_spk_power_amp_off(BOTTOM_SPK_AMP_NEG);
		else if (!strncmp(w->name, "Ext Spk Top Pos", 15))
			msm8930_ext_spk_power_amp_off(TOP_SPK_AMP_POS);
		else if  (!strncmp(w->name, "Ext Spk Top Neg", 15))
			msm8930_ext_spk_power_amp_off(TOP_SPK_AMP_NEG);
		else if (!strncmp(w->name, "Ext Hs Pos", 10))
			msm8930_ext_spk_power_amp_off(HS_AMP_POS);
		else if  (!strncmp(w->name, "Ext Hs Neg", 10))
			msm8930_ext_spk_power_amp_off(HS_AMP_NEG);
		else if (!strncmp(w->name, "Ext Rcv Pos", 11))
			msm8930_ext_spk_power_amp_off(RCV_AMP_POS);
		else if  (!strncmp(w->name, "Ext Rcv Neg", 11))
			msm8930_ext_spk_power_amp_off(RCV_AMP_NEG);
		else if  (!strncmp(w->name, "Ext Spk Top", 12))
			msm8930_ext_spk_power_amp_off(TOP_SPK_AMP);
		else {
			pr_err("%s() Invalid Amplifier Widget = %s\n",
					__func__, w->name);
			return -EINVAL;
		}
	}
	return 0;
}

static int msm8930_get_hac(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_info("%s: msm8930_hac_control = %d", __func__, msm8930_hac_control);
	ucontrol->value.integer.value[0] = msm8930_hac_control;
	return 0;
}

static int msm8930_set_hac(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	if (msm8930_hac_control == ucontrol->value.integer.value[0])
		return 0;

	msm8930_hac_control = ucontrol->value.integer.value[0];
	pr_info("%s()  %d\n", __func__, msm8930_hac_control);
	return 1;
}

static int msm8930_hacramp_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *k, int event)
{
	pr_info("%s: HAC amp %s\n", __func__, SND_SOC_DAPM_EVENT_ON(event) ? "on" : "off");
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		htc_audio_gpio.amp_hac(true);
	} else {
		htc_audio_gpio.amp_hac(false);
	}
	return 0;
}

static int msm8930_enable_codec_ext_clk(
		struct snd_soc_codec *codec, int enable,
		bool dapm)
{
	pr_debug("%s: enable = %d\n", __func__, enable);
	if (enable) {
		clk_users++;
		pr_debug("%s: clk_users = %d\n", __func__, clk_users);
		if (clk_users != 1)
			return 0;

		if (codec_clk) {
			clk_set_rate(codec_clk, SITAR_EXT_CLK_RATE);
			clk_prepare_enable(codec_clk);
			sitar_mclk_enable(codec, 1, dapm);
		} else {
			pr_err("%s: Error setting Sitar MCLK\n", __func__);
			clk_users--;
			return -EINVAL;
		}
	} else {
		pr_debug("%s: clk_users = %d\n", __func__, clk_users);
		if (clk_users == 0)
			return 0;
		clk_users--;
		if (!clk_users) {
			pr_debug("%s: disabling MCLK. clk_users = %d\n",
					 __func__, clk_users);
			sitar_mclk_enable(codec, 0, dapm);
			clk_disable_unprepare(codec_clk);
		}
	}
	return 0;
}

static int msm8930_mclk_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	pr_debug("%s: event = %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		return msm8930_enable_codec_ext_clk(w->codec, 1, true);
	case SND_SOC_DAPM_POST_PMD:
		return msm8930_enable_codec_ext_clk(w->codec, 0, true);
	}
	return 0;
}

static const struct snd_kcontrol_new extspk_switch_controls =
	SOC_DAPM_SINGLE("Switch", 0, 0, 1, 0);
static const struct snd_kcontrol_new earamp_switch_controls =
	SOC_DAPM_SINGLE("Switch", 0, 0, 1, 0);
static const struct snd_kcontrol_new spkamp_switch_controls =
	SOC_DAPM_SINGLE("Switch", 0, 0, 1, 0);
static const struct snd_kcontrol_new hsamp_switch_controls =
	SOC_DAPM_SINGLE("Switch", 0, 0, 1, 0);
static const struct snd_kcontrol_new rcvamp_switch_controls =
	SOC_DAPM_SINGLE("Switch", 0, 0, 1, 0);

static const struct snd_soc_dapm_widget msm8930_dapm_widgets[] = {

	SND_SOC_DAPM_SUPPLY("MCLK",  SND_SOC_NOPM, 0, 0,
	msm8930_mclk_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_SPK("Ext Spk Bottom Pos", msm8930_spkramp_event),
	SND_SOC_DAPM_SPK("Ext Spk Bottom Neg", msm8930_spkramp_event),

	SND_SOC_DAPM_SPK("Ext Spk Top Pos", msm8930_spkramp_event),
	SND_SOC_DAPM_SPK("Ext Spk Top Neg", msm8930_spkramp_event),
	SND_SOC_DAPM_SPK("Ext Spk Top", msm8930_spkramp_event),

	SND_SOC_DAPM_SPK("Ext Hs Pos", msm8930_spkramp_event),
	SND_SOC_DAPM_SPK("Ext Hs Neg", msm8930_spkramp_event),

	SND_SOC_DAPM_SPK("Ext Rcv Pos", msm8930_spkramp_event),
	SND_SOC_DAPM_SPK("Ext Rcv Neg", msm8930_spkramp_event),

	SND_SOC_DAPM_SPK("Ext Hac Pos", msm8930_hacramp_event),
	SND_SOC_DAPM_SPK("Ext Hac Neg", NULL),

	SND_SOC_DAPM_MIC("Headset Mic", NULL),
	SND_SOC_DAPM_MIC("Digital Mic1", NULL),
	SND_SOC_DAPM_MIC("ANCRight Headset Mic", NULL),
	SND_SOC_DAPM_MIC("ANCLeft Headset Mic", NULL),

	SND_SOC_DAPM_MIC("Digital Mic1", NULL),
	SND_SOC_DAPM_MIC("Digital Mic2", NULL),
	SND_SOC_DAPM_MIC("Digital Mic3", NULL),
	SND_SOC_DAPM_MIC("Digital Mic4", NULL),

	SND_SOC_DAPM_MIXER("Lineout Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("SPK AMP EN", SND_SOC_NOPM, 0, 0, &spkamp_switch_controls, 1),
	SND_SOC_DAPM_MIXER("HAC AMP EN", SND_SOC_NOPM, 0, 0, &earamp_switch_controls, 1),
	SND_SOC_DAPM_MIXER("DOCK AMP EN", SND_SOC_NOPM, 0, 0, &extspk_switch_controls, 1),
	SND_SOC_DAPM_MIXER("HS AMP EN", SND_SOC_NOPM, 0, 0, &hsamp_switch_controls, 1),
	SND_SOC_DAPM_MIXER("RCV AMP EN", SND_SOC_NOPM, 0, 0, &rcvamp_switch_controls, 1),
};

static const struct snd_soc_dapm_route common_audio_map[] = {

	{"RX_BIAS", NULL, "MCLK"},
	{"LDO_H", NULL, "MCLK"},

	{"MIC BIAS1 Internal1", NULL, "MCLK"},
	{"MIC BIAS2 Internal1", NULL, "MCLK"},


	{"Ext Spk Bottom Pos", NULL, "SPK AMP EN"},
	{"Ext Spk Bottom Neg", NULL, "SPK AMP EN"},
	{"SPK AMP EN", "Switch", "Lineout Mixer"},


	{"Ext Hac Pos", NULL, "HAC AMP EN"},
	{"Ext Hac Neg", NULL, "HAC AMP EN"},
	{"HAC AMP EN", "Switch", "Lineout Mixer"},


	{"Ext Hs Pos", NULL, "HS AMP EN"},
	{"Ext Hs Neg", NULL, "HS AMP EN"},
	{"HS AMP EN", "Switch", "Lineout Mixer"},


	{"Ext Rcv Pos", NULL, "RCV AMP EN"},
	{"Ext Rcv Neg", NULL, "RCV AMP EN"},
	{"RCV AMP EN", "Switch", "Lineout Mixer"},

	{"Lineout Mixer", NULL, "LINEOUT2"},
	{"Lineout Mixer", NULL, "LINEOUT1"},


	{"AMIC2", NULL, "MIC BIAS2 External"},
	{"MIC BIAS2 External", NULL, "Headset Mic"},


	{"AMIC1", NULL, "MIC BIAS1 External"},
	{"MIC BIAS1 External", NULL, "ANCLeft Headset Mic"},

	{"AMIC3", NULL, "MIC BIAS2 External"},
	{"MIC BIAS2 External", NULL, "ANCRight Headset Mic"},

	{"HEADPHONE", NULL, "LDO_H"},


	{"DMIC1", NULL, "MIC BIAS1 External"},
	{"MIC BIAS1 External", NULL, "Digital Mic1"},

	{"DMIC2", NULL, "MIC BIAS1 External"},
	{"MIC BIAS1 External", NULL, "Digital Mic2"},
	{"DMIC3", NULL, "MIC BIAS2 External"},
	{"MIC BIAS2 External", NULL, "Digital Mic3"},

	{"DMIC4", NULL, "MIC BIAS2 External"},
	{"MIC BIAS2 External", NULL, "Digital Mic4"},


};

static const char *spk_function[] = {"Off", "On"};
static const char *slim0_rx_ch_text[] = {"One", "Two"};
static const char *slim0_tx_ch_text[] = {"One", "Two", "Three", "Four"};

static const struct soc_enum msm8930_enum[] = {
	SOC_ENUM_SINGLE_EXT(2, spk_function),
	SOC_ENUM_SINGLE_EXT(2, slim0_rx_ch_text),
	SOC_ENUM_SINGLE_EXT(4, slim0_tx_ch_text),
};

static const char *btsco_rate_text[] = {"8000", "16000"};
static const struct soc_enum msm8930_btsco_enum[] = {
		SOC_ENUM_SINGLE_EXT(2, btsco_rate_text),
};

static int msm8930_slim_0_rx_ch_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm8930_slim_0_rx_ch  = %d\n", __func__,
		msm8930_slim_0_rx_ch);
	ucontrol->value.integer.value[0] = msm8930_slim_0_rx_ch - 1;
	return 0;
}

static int msm8930_slim_0_rx_ch_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	msm8930_slim_0_rx_ch = ucontrol->value.integer.value[0] + 1;

	pr_debug("%s: msm8930_slim_0_rx_ch = %d\n", __func__,
		msm8930_slim_0_rx_ch);
	return 1;
}

static int msm8930_slim_0_tx_ch_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm8930_slim_0_tx_ch  = %d\n", __func__,
		 msm8930_slim_0_tx_ch);
	ucontrol->value.integer.value[0] = msm8930_slim_0_tx_ch - 1;
	return 0;
}

static int msm8930_slim_0_tx_ch_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	msm8930_slim_0_tx_ch = ucontrol->value.integer.value[0] + 1;

	pr_debug("%s: msm8930_slim_0_tx_ch = %d\n", __func__,
		 msm8930_slim_0_tx_ch);
	return 1;
}

static int msm8930_btsco_rate_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm8930_btsco_rate  = %d", __func__, msm8930_btsco_rate);
	ucontrol->value.integer.value[0] = msm8930_btsco_rate;
	return 0;
}

static int msm8930_btsco_rate_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	switch (ucontrol->value.integer.value[0]) {
	case 0:
		msm8930_btsco_rate = BTSCO_RATE_8KHZ;
		break;
	case 1:
		msm8930_btsco_rate = BTSCO_RATE_16KHZ;
		break;
	default:
		msm8930_btsco_rate = BTSCO_RATE_8KHZ;
		break;
	}
	pr_debug("%s: msm8930_btsco_rate = %d\n", __func__, msm8930_btsco_rate);
	return 0;
}

static const struct snd_kcontrol_new sitar_msm8930_controls[] = {
	SOC_ENUM_EXT("Speaker Function", msm8930_enum[0], msm8930_get_spk,
		msm8930_set_spk),
	SOC_ENUM_EXT("SLIM_0_RX Channels", msm8930_enum[1],
		msm8930_slim_0_rx_ch_get, msm8930_slim_0_rx_ch_put),
	SOC_ENUM_EXT("SLIM_0_TX Channels", msm8930_enum[2],
		msm8930_slim_0_tx_ch_get, msm8930_slim_0_tx_ch_put),
	SOC_ENUM_EXT("HAC AMP EN", msm8930_enum[0], msm8930_get_hac,
		msm8930_set_hac),
};

static const struct snd_kcontrol_new int_btsco_rate_mixer_controls[] = {
	SOC_ENUM_EXT("Internal BTSCO SampleRate", msm8930_btsco_enum[0],
		msm8930_btsco_rate_get, msm8930_btsco_rate_put),
};

static int msm8930_btsco_init(struct snd_soc_pcm_runtime *rtd)
{
	int err = 0;
	struct snd_soc_platform *platform = rtd->platform;
	pr_info("%s\n", __func__);

	err = snd_soc_add_platform_controls(platform,
			int_btsco_rate_mixer_controls,
		ARRAY_SIZE(int_btsco_rate_mixer_controls));
	if (err < 0)
		return err;
	return 0;
}

static int msm8930_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;
	unsigned int rx_ch[SLIM_MAX_RX_PORTS], tx_ch[SLIM_MAX_TX_PORTS];
	unsigned int rx_ch_cnt = 0, tx_ch_cnt = 0;

	pr_debug("%s: ch=%d\n", __func__,
					msm8930_slim_0_rx_ch);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ret = snd_soc_dai_get_channel_map(codec_dai,
				&tx_ch_cnt, tx_ch, &rx_ch_cnt , rx_ch);
		if (ret < 0) {
			pr_err("%s: failed to get codec chan map\n", __func__);
			goto end;
		}
		ret = snd_soc_dai_set_channel_map(cpu_dai, 0, 0,
				msm8930_slim_0_rx_ch, rx_ch);
		if (ret < 0) {
			pr_err("%s: failed to set cpu chan map\n", __func__);
			goto end;
		}
		ret = snd_soc_dai_set_channel_map(codec_dai, 0, 0,
				msm8930_slim_0_rx_ch, rx_ch);
		if (ret < 0) {
			pr_err("%s: failed to set codec channel map\n",
				__func__);
			goto end;
		}
	} else {
		ret = snd_soc_dai_get_channel_map(codec_dai,
				&tx_ch_cnt, tx_ch, &rx_ch_cnt , rx_ch);
		if (ret < 0) {
			pr_err("%s: failed to get codec chan map\n", __func__);
			goto end;
		}
		ret = snd_soc_dai_set_channel_map(cpu_dai,
				msm8930_slim_0_tx_ch, tx_ch, 0 , 0);
		if (ret < 0) {
			pr_err("%s: failed to set cpu chan map\n", __func__);
			goto end;
		}
		ret = snd_soc_dai_set_channel_map(codec_dai,
				msm8930_slim_0_tx_ch, tx_ch, 0, 0);
		if (ret < 0) {
			pr_err("%s: failed to set codec channel map\n",
				__func__);
			goto end;
		}
	}
end:
	return ret;
}

static int msm8930_audrx_init(struct snd_soc_pcm_runtime *rtd)
{
	int err;
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

	pr_info("%s(), dev_name%s\n", __func__, dev_name(cpu_dai->dev));

	rtd->pmdown_time = 0;

	snd_soc_dapm_new_controls(dapm, msm8930_dapm_widgets,
				ARRAY_SIZE(msm8930_dapm_widgets));

	snd_soc_dapm_add_routes(dapm, common_audio_map,
		ARRAY_SIZE(common_audio_map));

	snd_soc_dapm_enable_pin(dapm, "Ext Spk Bottom Pos");
	snd_soc_dapm_enable_pin(dapm, "Ext Spk Bottom Neg");
	snd_soc_dapm_enable_pin(dapm, "Ext Spk Top Pos");
	snd_soc_dapm_enable_pin(dapm, "Ext Spk Top Neg");
	snd_soc_dapm_enable_pin(dapm, "Ext Hs Pos");
	snd_soc_dapm_enable_pin(dapm, "Ext Hs Neg");
	snd_soc_dapm_enable_pin(dapm, "Ext Rcv Pos");
	snd_soc_dapm_enable_pin(dapm, "Ext Rcv Neg");
	snd_soc_dapm_enable_pin(dapm, "Ext Hac Pos");
	snd_soc_dapm_enable_pin(dapm, "Ext Hac Neg");

	snd_soc_dapm_sync(dapm);

	err = snd_soc_jack_new(codec, "Headset Jack",
		(SND_JACK_HEADSET | SND_JACK_OC_HPHL | SND_JACK_OC_HPHR),
		&hs_jack);
	if (err) {
		pr_err("failed to create new jack\n");
		return err;
	}

	err = snd_soc_jack_new(codec, "Button Jack",
				SITAR_JACK_BUTTON_MASK, &button_jack);
	if (err) {
		pr_err("failed to create new jack\n");
		return err;
	}
	codec_clk = clk_get(cpu_dai->dev, "osr_clk");

	return 0;
}

static int msm8930_slim_0_rx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	rate->min = rate->max = 48000;
	channels->min = channels->max = msm8930_slim_0_rx_ch;
	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
		SNDRV_PCM_FORMAT_S16_LE);

	return 0;
}

static int msm8930_slim_0_tx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	pr_debug("%s()\n", __func__);
	rate->min = rate->max = 48000;
	channels->min = channels->max = msm8930_slim_0_tx_ch;

	return 0;
}

static int msm8930_mi2s_rx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	rate->min = rate->max = 48000;
	channels->min = channels->max = 1;
#ifdef CONFIG_AMP_TFA9887L
	channels->min = channels->max = 2;
#endif
	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
		SNDRV_PCM_FORMAT_S16_LE);

	return 0;
}

static int msm8930_mi2s_tx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	int format = SNDRV_PCM_FORMAT_S16_LE;

	rate->min = rate->max = 48000;
	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
		format);
	channels->min = channels->max = 2;
	return 0;
}

static int msm8930_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	pr_debug("%s()\n", __func__);

	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
					SNDRV_PCM_FORMAT_S16_LE);

	rate->min = rate->max = 48000;

	return 0;
}

static int msm8930_hdmi_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
					struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
		SNDRV_PCM_FORMAT_S16_LE);

	rate->min = rate->max = 48000;
	channels->min = channels->max = 2;

	return 0;
}

static int msm8930_btsco_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
					struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	rate->min = rate->max = msm8930_btsco_rate;
	channels->min = channels->max = msm8930_btsco_ch;

	return 0;
}

static int msm8930_auxpcm_be_params_fixup(struct snd_soc_pcm_runtime *rtd,
				struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);


	rate->min = rate->max = 16000;
	channels->min = channels->max = 1;

	return 0;
}

static int msm8930_proxy_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	printk("%s()\n", __func__);

	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
		SNDRV_PCM_FORMAT_S16_LE);

	rate->min = rate->max = 48000;

	return 0;
}

static int msm8930_auxpcm_startup(struct snd_pcm_substream *substream)
{
	int ret = 0;

	pr_debug("%s(): substream = %s\n", __func__, substream->name);
	mutex_lock(&aux_pcm_mutex);
	aux_pcm_open++;
	if(aux_pcm_open > 1){
		mutex_unlock(&aux_pcm_mutex);
		return 0;
	}
	ret = configure_gpios(htc_audio_gpio.aux_pcm_gpio, ARRAY_SIZE(htc_audio_gpio.aux_pcm_gpio));
	if (ret < 0) {
		pr_err("%s: Aux PCM GPIO request failed\n", __func__);
		aux_pcm_open--;
		mutex_unlock(&aux_pcm_mutex);
		return -EINVAL;
	}
	mutex_unlock(&aux_pcm_mutex);
	return 0;
}

static void msm8930_auxpcm_shutdown(struct snd_pcm_substream *substream)
{
	pr_debug("%s(): substream = %s\n", __func__, substream->name);
	mutex_lock(&aux_pcm_mutex);
	aux_pcm_open--;
	if(aux_pcm_open < 1) {
		free_gpios(htc_audio_gpio.aux_pcm_gpio, ARRAY_SIZE(htc_audio_gpio.aux_pcm_gpio));
	}
	mutex_unlock(&aux_pcm_mutex);
}

static int msm8930_startup(struct snd_pcm_substream *substream)
{
	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
		 substream->name, substream->stream);
	return 0;
}

static void msm8930_shutdown(struct snd_pcm_substream *substream)
{
	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
		 substream->name, substream->stream);
}

static struct snd_soc_ops msm8930_be_ops = {
	.startup = msm8930_startup,
	.hw_params = msm8930_hw_params,
	.shutdown = msm8930_shutdown,
};

static struct snd_soc_ops msm8930_auxpcm_be_ops = {
	.startup = msm8930_auxpcm_startup,
	.shutdown = msm8930_auxpcm_shutdown,
};

static struct snd_soc_dai_link msm8930_dai[] = {
	{
		.name = "MSM8930 Media1",
		.stream_name = "MultiMedia1",
		.cpu_dai_name	= "MultiMedia1",
		.platform_name  = "msm-pcm-dsp",
		.dynamic = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA1,
	},
	{
		.name = "MSM8930 Media2",
		.stream_name = "MultiMedia2",
		.cpu_dai_name	= "MultiMedia2",
		.platform_name  = "msm-pcm-dsp",
		.dynamic = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA2,
	},
	{
		.name = "Circuit-Switch Voice",
		.stream_name = "CS-Voice",
		.cpu_dai_name   = "CS-VOICE",
		.platform_name  = "msm-pcm-voice",
		.dynamic = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.be_id = MSM_FRONTEND_DAI_CS_VOICE,
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
	},
	{
		.name = "MSM VoIP",
		.stream_name = "VoIP",
		.cpu_dai_name	= "VoIP",
		.platform_name  = "msm-voip-dsp",
		.dynamic = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_VOIP,
	},
	{
		.name = "MSM8930 LPA",
		.stream_name = "LPA",
		.cpu_dai_name	= "MultiMedia3",
		.platform_name  = "msm-pcm-lpa",
		.dynamic = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA3,
	},
	{
		.name = "SLIMBUS_0 Hostless",
		.stream_name = "SLIMBUS_0 Hostless",
		.cpu_dai_name	= "SLIMBUS0_HOSTLESS",
		.platform_name  = "msm-pcm-hostless",
		.dynamic = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,

	},
	{
		.name = "MI2S_TX Hostless",
		.stream_name = "MI2S_TX Hostless",
		.cpu_dai_name	= "MI2S_TX_HOSTLESS",
		.platform_name  = "msm-pcm-hostless",
		.dynamic = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
		/* .be_id = do not care */
	},
	{
		.name = "MSM AFE-PCM RX",
		.stream_name = "AFE-PROXY RX",
		.cpu_dai_name = "msm-dai-q6.241",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.platform_name  = "msm-pcm-afe",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
	},
	{
		.name = "MSM AFE-PCM TX",
		.stream_name = "AFE-PROXY TX",
		.cpu_dai_name = "msm-dai-q6.240",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.platform_name  = "msm-pcm-afe",
		.ignore_suspend = 1,
	},
	{
		.name = "MSM8930 Compr",
		.stream_name = "COMPR",
		.cpu_dai_name	= "MultiMedia4",
		.platform_name  = "msm-compr-dsp",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA4,
	},
	{
		.name = "MSM8930 Compr2",
		.stream_name = "COMPR2",
		.cpu_dai_name	= "MultiMedia7",
		.platform_name	= "msm-compr-dsp",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA7,
	},
	{
		.name = "MSM8930 Compr3",
		.stream_name = "COMPR3",
		.cpu_dai_name	= "MultiMedia8",
		.platform_name	= "msm-compr-dsp",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA8,
	},
	{
		.name = "Compress Stub",
		.stream_name = "Compress Stub",
		.cpu_dai_name	= "MM_STUB",
		.platform_name  = "msm-pcm-hostless",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	{
		.name = "MSM8930 Media5",
		.stream_name = "MultiMedia5",
		.cpu_dai_name	= "MultiMedia5",
		.platform_name	= "msm-multi-ch-pcm-dsp",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			    SND_SOC_DPCM_TRIGGER_POST},
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA5
	},
	{
		.name = "MSM8930 Media6",
		.stream_name = "MultiMedia6",
		.cpu_dai_name	= "MultiMedia6",
		.platform_name	= "msm-multi-ch-pcm-dsp",
		.dynamic = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
				SND_SOC_DPCM_TRIGGER_POST},
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA6
	},
	{
		.name = "AUXPCM Hostless",
		.stream_name = "AUXPCM Hostless",
		.cpu_dai_name   = "AUXPCM_HOSTLESS",
		.platform_name  = "msm-pcm-hostless",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	{
		.name = "HDMI_RX_HOSTLESS",
		.stream_name = "HDMI_RX_HOSTLESS",
		.cpu_dai_name = "HDMI_HOSTLESS",
		.platform_name = "msm-pcm-hostless",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			    SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		/* this dainlink has playback support */
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	/* Backend DAI Links */
	{
		.name = LPASS_BE_SLIMBUS_0_RX,
		.stream_name = "Slimbus Playback",
		.cpu_dai_name = "msm-dai-q6.16384",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "sitar_codec",
		.codec_dai_name	= "sitar_rx1",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_SLIMBUS_0_RX,
		.init = &msm8930_audrx_init,
		.be_hw_params_fixup = msm8930_slim_0_rx_be_hw_params_fixup,
		.ops = &msm8930_be_ops,
		.ignore_pmdown_time = 1,
	},
	{
		.name = LPASS_BE_SLIMBUS_0_TX,
		.stream_name = "Slimbus Capture",
		.cpu_dai_name = "msm-dai-q6.16385",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "sitar_codec",
		.codec_dai_name	= "sitar_tx1",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_SLIMBUS_0_TX,
		.be_hw_params_fixup = msm8930_slim_0_tx_be_hw_params_fixup,
		.ops = &msm8930_be_ops,
	},
	{
		.name = LPASS_BE_INT_BT_SCO_RX,
		.stream_name = "Internal BT-SCO Playback",
		.cpu_dai_name = "msm-dai-q6.12288",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name	= "msm-stub-rx",
		.init = &msm8930_btsco_init,
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_INT_BT_SCO_RX,
		.be_hw_params_fixup = msm8930_btsco_be_hw_params_fixup,
		.ignore_pmdown_time = 1,
	},
	{
		.name = LPASS_BE_INT_BT_SCO_TX,
		.stream_name = "Internal BT-SCO Capture",
		.cpu_dai_name = "msm-dai-q6.12289",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name	= "msm-stub-tx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_INT_BT_SCO_TX,
		.be_hw_params_fixup = msm8930_btsco_be_hw_params_fixup,
	},
	{
		.name = LPASS_BE_INT_FM_RX,
		.stream_name = "Internal FM Playback",
		.cpu_dai_name = "msm-dai-q6.12292",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_INT_FM_RX,
		.be_hw_params_fixup = msm8930_be_hw_params_fixup,
		.ignore_pmdown_time = 1,
	},
	{
		.name = LPASS_BE_INT_FM_TX,
		.stream_name = "Internal FM Capture",
		.cpu_dai_name = "msm-dai-q6.12293",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_INT_FM_TX,
		.be_hw_params_fixup = msm8930_be_hw_params_fixup,
	},
	{
		.name = LPASS_BE_HDMI,
		.stream_name = "HDMI Playback",
		.cpu_dai_name = "msm-dai-q6-hdmi.8",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_HDMI_RX,
		.be_hw_params_fixup = msm8930_hdmi_be_hw_params_fixup,
		.ignore_pmdown_time = 1,
	},
	{
		.name = LPASS_BE_PRI_I2S_TX,
		.stream_name = "Primary I2S Capture",
		.cpu_dai_name = "msm-dai-q6.1",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_PRI_I2S_TX,
		.be_hw_params_fixup = msm8930_be_hw_params_fixup,
		.ops = &msm8930_i2s_be_ops,
	},
	{
		.name = LPASS_BE_MI2S_RX,
		.stream_name = "MI2S Playback",
		.cpu_dai_name = "msm-dai-q6-mi2s",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_MI2S_RX,
		.be_hw_params_fixup = msm8930_mi2s_rx_be_hw_params_fixup,
		.ops = &msm8930_mi2s_be_ops,
	},
	{
		.name = LPASS_BE_MI2S_TX,
		.stream_name = "MI2S Capture",
		.cpu_dai_name = "msm-dai-q6-mi2s",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.be_hw_params_fixup = msm8930_mi2s_tx_be_hw_params_fixup,
		.be_id = MSM_BACKEND_DAI_MI2S_TX,
		.ops = &msm8930_mi2s_be_ops,
	},
	{
		.name = LPASS_BE_PRI_I2S_RX,
		.stream_name = "Primary I2S Playback",
		.cpu_dai_name = "msm-dai-q6.1",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_PRI_I2S_RX,
		.be_hw_params_fixup = msm8930_be_hw_params_fixup,
		.ops = &msm8930_i2s_be_ops,
	},
	{
		.name = LPASS_BE_AFE_PCM_RX,
		.stream_name = "AFE Playback",
		.cpu_dai_name = "msm-dai-q6.224",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_AFE_PCM_RX,
		.be_hw_params_fixup = msm8930_proxy_be_hw_params_fixup,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
	},
	{
		.name = LPASS_BE_AFE_PCM_TX,
		.stream_name = "AFE Capture",
		.cpu_dai_name = "msm-dai-q6.225",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_AFE_PCM_TX,
		.be_hw_params_fixup = msm8930_proxy_be_hw_params_fixup,
	},
	{
		.name = LPASS_BE_AUXPCM_RX,
		.stream_name = "AUX PCM Playback",
		.cpu_dai_name = "msm-dai-q6.2",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_AUXPCM_RX,
		.be_hw_params_fixup = msm8930_auxpcm_be_params_fixup,
		.ops = &msm8930_auxpcm_be_ops,
	},
	{
		.name = LPASS_BE_AUXPCM_TX,
		.stream_name = "AUX PCM Capture",
		.cpu_dai_name = "msm-dai-q6.3",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_AUXPCM_TX,
		.be_hw_params_fixup = msm8930_auxpcm_be_params_fixup,
		.ops = &msm8930_auxpcm_be_ops,
	},
	{
		.name = LPASS_BE_VOICE_PLAYBACK_TX,
		.stream_name = "Voice Farend Playback",
		.cpu_dai_name = "msm-dai-q6.32773",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_VOICE_PLAYBACK_TX,
		.be_hw_params_fixup = msm8930_be_hw_params_fixup,
	},
	{
		.name = LPASS_BE_INCALL_RECORD_TX,
		.stream_name = "Voice Uplink Capture",
		.cpu_dai_name = "msm-dai-q6.32772",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_INCALL_RECORD_TX,
		.be_hw_params_fixup = msm8930_be_hw_params_fixup,
	},
	{
		.name = LPASS_BE_INCALL_RECORD_RX,
		.stream_name = "Voice Downlink Capture",
		.cpu_dai_name = "msm-dai-q6.32771",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_INCALL_RECORD_RX,
		.be_hw_params_fixup = msm8930_be_hw_params_fixup,
		.ignore_pmdown_time = 1,
	},
};

struct snd_soc_card snd_soc_card_msm8930 = {
	.name		= "msm8930-sitar-snd-card",
	.dai_link	= msm8930_dai,
	.num_links	= ARRAY_SIZE(msm8930_dai),
	.controls	= sitar_msm8930_controls,
	.num_controls	= ARRAY_SIZE(sitar_msm8930_controls),
};

static struct platform_device *msm8930_snd_device;

static void zara_amp_speaker(bool en)
{
#ifdef CONFIG_AMP_TFA9887
	set_tfa9887_spkamp(en, 0);
#ifdef CONFIG_AMP_TFA9887L
	set_tfa9887l_spkamp(en, 0);
#endif
#endif
}

static void zara_amp_receiver(bool en)
{
	gpio_direction_output(MSM_AUD_REC_EN, en);
	gpio_direction_output(MSM_AUD_RECEIVER_SEL, en);
}

static void zara_amp_headset(bool en)
{
#ifdef CONFIG_AMP_RT5501
	if(query_rt5501()) {
		set_rt5501_amp(en);
	}
#endif
}

static void zara_amp_hac(bool en)
{
}

struct htc_8930_gpio_pdata htc_audio_gpio = {
	.amp_speaker = zara_amp_speaker,
	.amp_receiver = zara_amp_receiver,
	.amp_headset = zara_amp_headset,
	.amp_hac = zara_amp_hac,
	.mi2s_gpio = {
	{
		.gpio_no = MSM_AUD_CPU_RX_I2S_WS,
		.gpio_name = "AUD_SPK_MI2S_WS",
	},
	{
		.gpio_no = MSM_AUD_CPU_RX_I2S_SCK,
		.gpio_name = "AUD_SPK_MI2S_BCLK",
	},
	{
		.gpio_no = MSM_AUD_TFA_DO_A,
		.gpio_name = "AUD_SPK_MI2S_DI",
	},
	{
		.gpio_no = MSM_AUD_CPU_RX_I2S_SD1,
		.gpio_name = "AUD_SPK_MI2S_DO",
	},
	},
	.i2s_gpio = {
	{
		.gpio_no = MSM_AUD_FM_I2S_BCLK,
		.gpio_name = "AUD_FM_I2S_CLK",
	},
	{
		.gpio_no = MSM_AUD_FM_I2S_SYNC,
		.gpio_name = "AUD_FM_I2S_WS",
	},
	{
		.gpio_no = MSM_AUD_FM_I2S_DIN,
		.gpio_name = "AUD_FM_I2S_DI",
	},
	{
		.gpio_no = -1,
		.gpio_name = "AUD_FM_I2S_DO",
	},
	},
	.aux_pcm_gpio = {
	{
		.gpio_no = MSM_AUD_BTPCM_DIN,
		.gpio_name = "AUD_AUX_PCM_DIN",
	},
	{
		.gpio_no = MSM_AUD_BTPCM_DOUT,
		.gpio_name = "AUD_AUX_PCM_DOUT",
	},
	{
		.gpio_no = MSM_AUD_BTPCM_SYNC,
		.gpio_name = "AUD_AUX_PCM_SYNC",
	},
	{
		.gpio_no = MSM_AUD_BTPCM_CLK,
		.gpio_name = "AUD_AUX_PCM_CLK",
	},
	},
};
EXPORT_SYMBOL_GPL(htc_audio_gpio);

static int zara_get_hw_revision(void)
{
	int audio_hw_rev;

	audio_hw_rev = 0;

	pr_info("%s: audio hw rev is %d\n", __func__, audio_hw_rev);
	return audio_hw_rev;
}

static int zara_get_hw_component(void)
{
	int hw_com = 0;

	hw_com |= HTC_AUDIO_TFA9887;

	if(query_rt5501())
		hw_com |= HTC_AUDIO_RT5501;

	pr_info("%s: audio hw component is %x\n", __func__, hw_com);
	return hw_com;
}

static int zara_enable_digital_mic(void)
{
	return 0;
}

static void zara_set_q6_effect_mode(int mode)
{
	pr_info("%s: mode %d\n", __func__, mode);
	atomic_set(&q6_effect_mode, mode);
}

int zara_get_24b_audio(void)
{
	return 1;
}

static struct acoustic_ops acoustic = {
	.set_q6_effect = zara_set_q6_effect_mode,
	.get_htc_revision = zara_get_hw_revision,
	.get_hw_component = zara_get_hw_component,
	.enable_digital_mic = zara_enable_digital_mic,
};

static int __init zara_audio_init(void)
{
	int ret = 0;
	pr_info("%s\n", __func__);

	if (!cpu_is_msm8930() && !cpu_is_msm8930aa()) {
		pr_err("%s: Not the right machine type\n", __func__);
		return -ENODEV ;
	}

	msm8930_snd_device = platform_device_alloc("soc-audio", 0);
	if (!msm8930_snd_device) {
		pr_err("Platform device allocation failed\n");
		return -ENOMEM;
	}

	platform_set_drvdata(msm8930_snd_device, &snd_soc_card_msm8930);
	ret = platform_device_add(msm8930_snd_device);
	if (ret) {
		platform_device_put(msm8930_snd_device);
		return ret;
	}

	atomic_set(&mi2s_rsc_ref, 0);
	atomic_set(&pri_i2s_rsc_ref, 0);
	mutex_init(&aux_pcm_mutex);

	gpio_request(MSM_AUD_REC_EN, "AUDIO_RCV_AMP");
	gpio_tlmm_config(GPIO_CFG(MSM_AUD_REC_EN, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(MSM_AUD_REC_EN, 0);

	gpio_request(MSM_AUD_RECEIVER_SEL, "AUD_RECEIVER_SEL");
	gpio_tlmm_config(GPIO_CFG(MSM_AUD_RECEIVER_SEL, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(MSM_AUD_RECEIVER_SEL, 0);

	acoustic_register_ops(&acoustic);
	pr_info("%s", __func__);
	return ret;
}
module_init(zara_audio_init);

static void __exit zara_audio_exit(void)
{
	if (!cpu_is_msm8930() && !cpu_is_msm8930aa()) {
		pr_err("%s: Not the right machine type\n", __func__);
		return ;
	}
	pr_info("%s", __func__);
	platform_device_unregister(msm8930_snd_device);
	mutex_destroy(&aux_pcm_mutex);
}
module_exit(zara_audio_exit);

MODULE_DESCRIPTION("ALSA Board zara");
MODULE_LICENSE("GPL v2");
