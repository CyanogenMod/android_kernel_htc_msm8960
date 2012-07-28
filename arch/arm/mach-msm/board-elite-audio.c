/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * HTC: elite machine driver which defines board-specific data
 * Copy from sound/soc/msm/msm8960.c
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
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/regulator/gpio-regulator.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/soc-dsp.h>
#include <sound/pcm.h>
#include <sound/jack.h>
#include <sound/q6asm.h>
#include <mach/htc_acoustic_8960.h>
#include "../../../sound/soc/codecs/wcd9310.h"
#include "../sound/soc/msm/msm-pcm-routing.h"
#include "board-elite.h"

static atomic_t q6_effect_mode = ATOMIC_INIT(-1);

#include <mach/cable_detect.h>
#include <mach/board.h>

/* codec registers defined in linux/mfd/wcd9310/registers.h */
#define TABLA_A_CHIP_VERSION			(0x08)

#define ELITE_AUD_STEREO_REC	(PM8921_GPIO_PM_TO_SYS(3))
/* Naming of external AMP of hac, speaker is TOP_SPK_PAMP_GPIO,
 * BOTTOM_SPK_PAMP_GPIO, respectively */
#define TOP_SPK_PAMP_GPIO	(PM8921_GPIO_PM_TO_SYS(19))
#define BOTTOM_SPK_PAMP_GPIO	(PM8921_GPIO_PM_TO_SYS(18))
/* These GPIOs control Cradle outout */
#define DOCK_SPK_PAMP_GPIO	(PM8921_GPIO_PM_TO_SYS(1))
#define USB_ID_ADC_GPIO		(PM8921_GPIO_PM_TO_SYS(4))
#define MSM8960_SPK_ON 1
#define MSM8960_SPK_OFF 0

#define msm8960_SLIM_0_RX_MAX_CHANNELS		2
#define msm8960_SLIM_0_TX_MAX_CHANNELS		4

#define BTSCO_RATE_8KHZ 8000
#define BTSCO_RATE_16KHZ 16000

#define BOTTOM_SPK_AMP_POS	0x1
#define BOTTOM_SPK_AMP_NEG	0x2
#define TOP_SPK_AMP_POS		0x4
#define TOP_SPK_AMP_NEG		0x8
#define DOCK_SPK_AMP_POS	0x10
#define DOCK_SPK_AMP_NEG	0x20

#define GPIO_AUX_PCM_DOUT 63
#define GPIO_AUX_PCM_DIN 64
#define GPIO_AUX_PCM_SYNC 65
#define GPIO_AUX_PCM_CLK 66

#define TABLA_EXT_CLK_RATE 12288000

static int msm8960_spk_control;
static int msm8960_ext_bottom_spk_pamp;
static int msm8960_ext_top_spk_pamp;
static int msm8960_ext_dock_spk_pamp;
static int msm8960_slim_0_rx_ch = 1;
static int msm8960_slim_0_tx_ch = 1;

static int msm8960_btsco_rate = BTSCO_RATE_8KHZ;
static int msm8960_btsco_ch = 1;

static int elite_stereo_control;

static struct clk *codec_clk;
static int clk_users;

static struct snd_soc_jack hs_jack;
static struct snd_soc_jack button_jack;

#ifdef CONFIG_AUDIO_USAGE_FOR_POWER_CONSUMPTION
static ssize_t speaker_stat_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t headset_stat_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

static struct kobj_attribute speaker_attribute =
        __ATTR(audio_speaker_t, 0444, speaker_stat_show, NULL);

static struct kobj_attribute headset_attribute =
        __ATTR(audio_headset_t, 0444, headset_stat_show, NULL);

static struct attribute *attrs[] = {
        &speaker_attribute.attr,
        &headset_attribute.attr,
        NULL,   // need to NULL terminate the list of attributes
};

static struct kobject *audio_stat_kobj = NULL;

struct timespec g_spk_start_time;
struct timespec g_spk_end_time;
unsigned long g_spk_total_time = 0;
int g_spk_flag = 0;

struct timespec g_hs_start_time;
struct timespec g_hs_end_time;
unsigned long g_hs_total_time = 0;
int g_hs_flag = 0;
#endif

extern void release_audio_dock_lock(void);

static void msm8960_ext_spk_power_amp_off(u32);
static void audio_dock_notifier_func(enum usb_connect_type online)
{
	if (cable_get_accessory_type() != DOCK_STATE_AUDIO_DOCK) {
		pr_aud_info("accessory is not AUDIO_DOCK\n");
		return;
	}

	switch(online) {
	case CONNECT_TYPE_NONE:
		pr_aud_info("%s, VBUS is removed\n", __func__);
		/* disable Audio Dock GPIO */
		msm8960_ext_spk_power_amp_off(DOCK_SPK_AMP_POS|DOCK_SPK_AMP_NEG);
		/* release GPIO USB_ID_ADC and cable driver */
		release_audio_dock_lock();
		break;
	default:
		break;
	}

	return;
}

static struct mutex audio_notifier_lock;
static struct t_cable_status_notifier audio_dock_notifier =
{
	.name = "elite_audio_8960",
	.func = audio_dock_notifier_func,
};

static void msm8960_enable_ext_spk_amp_gpio(u32 spk_amp_gpio)
{
	int ret = 0;

	struct pm_gpio param = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 1,
		.pull      = PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
	};

	if (spk_amp_gpio == BOTTOM_SPK_PAMP_GPIO) {

		ret = gpio_request(BOTTOM_SPK_PAMP_GPIO, "BOTTOM_SPK_AMP");
		if (ret) {
			pr_aud_err("%s: Error requesting BOTTOM SPK AMP GPIO %u\n",
				__func__, BOTTOM_SPK_PAMP_GPIO);
			return;
		}
		ret = pm8xxx_gpio_config(BOTTOM_SPK_PAMP_GPIO, &param);
		if (ret)
			pr_aud_err("%s: Failed to configure Bottom Spk Ampl"
				" gpio %u\n", __func__, BOTTOM_SPK_PAMP_GPIO);
		else {
			pr_debug("%s: enable spkr amp gpio\n", __func__);
			gpio_direction_output(BOTTOM_SPK_PAMP_GPIO, 1);
		}

	} else if (spk_amp_gpio == TOP_SPK_PAMP_GPIO) {

		ret = gpio_request(TOP_SPK_PAMP_GPIO, "TOP_SPK_AMP");
		if (ret) {
			pr_aud_err("%s: Error requesting GPIO %d\n", __func__,
				TOP_SPK_PAMP_GPIO);
			return;
		}
		ret = pm8xxx_gpio_config(TOP_SPK_PAMP_GPIO, &param);
		if (ret)
			pr_aud_err("%s: Failed to configure Top Spk Ampl"
				" gpio %u\n", __func__, TOP_SPK_PAMP_GPIO);
		else {
			pr_debug("%s: enable hac amp gpio\n", __func__);
			gpio_direction_output(TOP_SPK_PAMP_GPIO, 1);
		}

	} else if (spk_amp_gpio == DOCK_SPK_PAMP_GPIO) {

		ret = gpio_request(DOCK_SPK_PAMP_GPIO, "DOCK_SPK_AMP");
		if (ret) {
			pr_aud_err("%s: Error requesting GPIO %d\n", __func__,
				DOCK_SPK_PAMP_GPIO);
			return;
		}
		ret = pm8xxx_gpio_config(DOCK_SPK_PAMP_GPIO, &param);
		if (ret)
			pr_aud_err("%s: Failed to configure Dock Spk Ampl"
				" gpio %u\n", __func__, DOCK_SPK_PAMP_GPIO);
		else {
			pr_debug("%s: enable dock amp gpio\n", __func__);
			gpio_direction_output(DOCK_SPK_PAMP_GPIO, 1);
		}

		ret = gpio_request(USB_ID_ADC_GPIO, "USB_ID_ADC");
		if (ret) {
			pr_aud_err("%s: Error requesting USB_ID_ADC PMIC GPIO %u\n",
				__func__, USB_ID_ADC_GPIO);
			return;
		}
		ret = pm8xxx_gpio_config(USB_ID_ADC_GPIO, &param);
		if (ret)
			pr_aud_err("%s: Failed to configure USB_ID_ADC PMIC"
				" gpio %u\n", __func__, USB_ID_ADC_GPIO);

	} else {
		pr_aud_err("%s: ERROR : Invalid External Speaker Ampl GPIO."
			" gpio = %u\n", __func__, spk_amp_gpio);
		return;
	}
}

static void msm8960_ext_spk_power_amp_on(u32 spk)
{
	#ifdef CONFIG_AUDIO_USAGE_FOR_POWER_CONSUMPTION
	g_spk_flag = 1;
	g_spk_start_time = current_kernel_time();
	#endif

	if (spk & (BOTTOM_SPK_AMP_POS | BOTTOM_SPK_AMP_NEG)) {

		if ((msm8960_ext_bottom_spk_pamp & BOTTOM_SPK_AMP_POS) &&
			(msm8960_ext_bottom_spk_pamp & BOTTOM_SPK_AMP_NEG)) {

			pr_debug("%s() External Bottom Speaker Ampl already "
				"turned on. spk = 0x%08x\n", __func__, spk);
			return;
		}

		msm8960_ext_bottom_spk_pamp |= spk;

		if ((msm8960_ext_bottom_spk_pamp & BOTTOM_SPK_AMP_POS) &&
			(msm8960_ext_bottom_spk_pamp & BOTTOM_SPK_AMP_NEG)) {

			msm8960_enable_ext_spk_amp_gpio(BOTTOM_SPK_PAMP_GPIO);
			pr_debug("%s: slepping 4 ms after turning on external "
				" Speaker Ampl\n", __func__);
			usleep_range(4000, 4000);
		}

	} else if (spk & (TOP_SPK_AMP_POS | TOP_SPK_AMP_NEG)) {

		if ((msm8960_ext_top_spk_pamp & TOP_SPK_AMP_POS) &&
			(msm8960_ext_top_spk_pamp & TOP_SPK_AMP_NEG)) {

			pr_debug("%s() External Top Speaker Ampl already"
				"turned on. spk = 0x%08x\n", __func__, spk);
			return;
		}

		msm8960_ext_top_spk_pamp |= spk;

		if ((msm8960_ext_top_spk_pamp & TOP_SPK_AMP_POS) &&
			(msm8960_ext_top_spk_pamp & TOP_SPK_AMP_NEG)) {

			msm8960_enable_ext_spk_amp_gpio(TOP_SPK_PAMP_GPIO);
			pr_debug("%s: sleeping 4 ms after turning on "
				" external HAC Ampl\n", __func__);
			usleep_range(4000, 4000);
		}

	} else if (spk & (DOCK_SPK_AMP_POS | DOCK_SPK_AMP_NEG)) {

		mutex_lock(&audio_notifier_lock);

		if ((msm8960_ext_dock_spk_pamp & DOCK_SPK_AMP_POS) &&
			(msm8960_ext_dock_spk_pamp & DOCK_SPK_AMP_NEG)) {

			pr_debug("%s() External Dock Speaker Ampl already"
				"turned on. spk = 0x%08x\n", __func__, spk);
			return;
		}

		msm8960_ext_dock_spk_pamp |= spk;

		if ((msm8960_ext_dock_spk_pamp & DOCK_SPK_AMP_POS) &&
			(msm8960_ext_dock_spk_pamp & DOCK_SPK_AMP_NEG)) {

			msm8960_enable_ext_spk_amp_gpio(DOCK_SPK_PAMP_GPIO);

			pr_debug("%s: sleeping 4 ms after turning on "
				" external DOCK Ampl\n", __func__);
			usleep_range(4000, 4000);
		}
		mutex_unlock(&audio_notifier_lock);

	} else  {

		pr_aud_err("%s: ERROR : Invalid External Speaker Ampl. spk = 0x%08x\n",
			__func__, spk);
		return;
	}
}

static void msm8960_ext_spk_power_amp_off(u32 spk)
{
	struct pm_gpio param = {
		.direction      = PM_GPIO_DIR_IN,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.pull      = PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
	};

	#ifdef CONFIG_AUDIO_USAGE_FOR_POWER_CONSUMPTION
	g_spk_flag = 0;
	g_spk_end_time = current_kernel_time();

	g_spk_total_time += (g_spk_end_time.tv_sec - g_spk_start_time.tv_sec);
	#endif

	if (spk & (BOTTOM_SPK_AMP_POS | BOTTOM_SPK_AMP_NEG)) {

		if (!msm8960_ext_bottom_spk_pamp)
			return;

		gpio_direction_output(BOTTOM_SPK_PAMP_GPIO, 0);
		gpio_free(BOTTOM_SPK_PAMP_GPIO);
		msm8960_ext_bottom_spk_pamp = 0;

		pr_debug("%s: sleeping 4 ms after turning off external"
			" Speaker Ampl\n", __func__);

		usleep_range(4000, 4000);

	} else if (spk & (TOP_SPK_AMP_POS | TOP_SPK_AMP_NEG)) {

		if (!msm8960_ext_top_spk_pamp)
			return;

		gpio_direction_output(TOP_SPK_PAMP_GPIO, 0);
		gpio_free(TOP_SPK_PAMP_GPIO);
		msm8960_ext_top_spk_pamp = 0;

		pr_debug("%s: sleeping 4 ms after turning off external"
			" HAC Ampl\n", __func__);

		usleep_range(4000, 4000);

	} else if (spk & (DOCK_SPK_AMP_POS | DOCK_SPK_AMP_NEG)) {

		mutex_lock(&audio_notifier_lock);

		if (!msm8960_ext_dock_spk_pamp) {
			mutex_unlock(&audio_notifier_lock);
			return;
		}

		gpio_direction_input(DOCK_SPK_PAMP_GPIO);
		gpio_free(DOCK_SPK_PAMP_GPIO);

		gpio_direction_input(USB_ID_ADC_GPIO);
		if (pm8xxx_gpio_config(USB_ID_ADC_GPIO, &param))
			pr_aud_err("%s: Failed to configure USB_ID_ADC PMIC"
				" gpio %u\n", __func__, USB_ID_ADC_GPIO);
		gpio_free(USB_ID_ADC_GPIO);
		msm8960_ext_dock_spk_pamp = 0;

		mutex_unlock(&audio_notifier_lock);

		pr_debug("%s: sleeping 4 ms after turning off external"
			" DOCK Ampl\n", __func__);

		usleep_range(4000, 4000);

	} else  {

		pr_aud_err("%s: ERROR : Invalid Ext Spk Ampl. spk = 0x%08x\n",
			__func__, spk);
		return;
	}
}

static void msm8960_ext_control(struct snd_soc_codec *codec)
{
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	pr_debug("%s: msm8960_spk_control = %d", __func__, msm8960_spk_control);
	if (msm8960_spk_control == MSM8960_SPK_ON) {
		snd_soc_dapm_enable_pin(dapm, "Ext Spk Bottom Pos");
		snd_soc_dapm_enable_pin(dapm, "Ext Spk Bottom Neg");
		snd_soc_dapm_enable_pin(dapm, "Ext Spk Top Pos");
		snd_soc_dapm_enable_pin(dapm, "Ext Spk Top Neg");
		snd_soc_dapm_enable_pin(dapm, "Dock Spk Pos");
		snd_soc_dapm_enable_pin(dapm, "Dock Spk Neg");

	} else {
		snd_soc_dapm_disable_pin(dapm, "Ext Spk Bottom Pos");
		snd_soc_dapm_disable_pin(dapm, "Ext Spk Bottom Neg");
		snd_soc_dapm_disable_pin(dapm, "Ext Spk Top Pos");
		snd_soc_dapm_disable_pin(dapm, "Ext Spk Top Neg");
		snd_soc_dapm_disable_pin(dapm, "Dock Spk Pos");
		snd_soc_dapm_disable_pin(dapm, "Dock Spk Neg");
	}

	snd_soc_dapm_sync(dapm);
}

static int msm8960_get_spk(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm8960_spk_control = %d", __func__, msm8960_spk_control);
	ucontrol->value.integer.value[0] = msm8960_spk_control;
	return 0;
}
static int msm8960_set_spk(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	pr_aud_info("%s()\n", __func__);
	if (msm8960_spk_control == ucontrol->value.integer.value[0])
		return 0;

	msm8960_spk_control = ucontrol->value.integer.value[0];
	msm8960_ext_control(codec);
	return 1;
}


static int msm8960_spkramp_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *k, int event)
{
	pr_aud_info("%s() name = %s, %x\n", __func__, w->name, SND_SOC_DAPM_EVENT_ON(event));

	if (SND_SOC_DAPM_EVENT_ON(event)) {
		if (!strncmp(w->name, "Ext Spk Bottom Pos", 18))
			msm8960_ext_spk_power_amp_on(BOTTOM_SPK_AMP_POS);
		else if (!strncmp(w->name, "Ext Spk Bottom Neg", 18))
			msm8960_ext_spk_power_amp_on(BOTTOM_SPK_AMP_NEG);
		else if (!strncmp(w->name, "Ext Spk Top Pos", 15))
			msm8960_ext_spk_power_amp_on(TOP_SPK_AMP_POS);
		else if  (!strncmp(w->name, "Ext Spk Top Neg", 15))
			msm8960_ext_spk_power_amp_on(TOP_SPK_AMP_NEG);
		else if (!strncmp(w->name, "Dock Spk Pos", 12))
			msm8960_ext_spk_power_amp_on(DOCK_SPK_AMP_POS);
		else if  (!strncmp(w->name, "Dock Spk Neg", 12))
			msm8960_ext_spk_power_amp_on(DOCK_SPK_AMP_NEG);
		else {
			pr_aud_err("%s() Invalid Speaker Widget = %s\n",
					__func__, w->name);
			return -EINVAL;
		}

	} else {
		if (!strncmp(w->name, "Ext Spk Bottom Pos", 18))
			msm8960_ext_spk_power_amp_off(BOTTOM_SPK_AMP_POS);
		else if (!strncmp(w->name, "Ext Spk Bottom Neg", 18))
			msm8960_ext_spk_power_amp_off(BOTTOM_SPK_AMP_NEG);
		else if (!strncmp(w->name, "Ext Spk Top Pos", 15))
			msm8960_ext_spk_power_amp_off(TOP_SPK_AMP_POS);
		else if  (!strncmp(w->name, "Ext Spk Top Neg", 15))
			msm8960_ext_spk_power_amp_off(TOP_SPK_AMP_NEG);
		else if (!strncmp(w->name, "Dock Spk Pos", 12))
			msm8960_ext_spk_power_amp_off(DOCK_SPK_AMP_POS);
		else if  (!strncmp(w->name, "Dock Spk Neg", 12))
			msm8960_ext_spk_power_amp_off(DOCK_SPK_AMP_NEG);
		else {
			pr_aud_err("%s() Invalid Speaker Widget = %s\n",
					__func__, w->name);
			return -EINVAL;
		}
	}
	return 0;
}

int msm8960_enable_codec_ext_clk(
		struct snd_soc_codec *codec, int enable)
{
	pr_debug("%s: enable = %d\n", __func__, enable);
	if (enable) {
		clk_users++;
		pr_debug("%s: clk_users = %d\n", __func__, clk_users);
		if (clk_users != 1)
			return 0;

		codec_clk = clk_get(NULL, "i2s_spkr_osr_clk");
		if (codec_clk) {
			clk_set_rate(codec_clk, TABLA_EXT_CLK_RATE);
			clk_enable(codec_clk);
			tabla_mclk_enable(codec, 1);
		} else {
			pr_aud_err("%s: Error setting Tabla MCLK\n", __func__);
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
			clk_disable(codec_clk);
			clk_put(codec_clk);
			tabla_mclk_enable(codec, 0);
		}
	}
	return 0;
}

static int msm8960_mclk_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	pr_debug("%s: event = %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		return msm8960_enable_codec_ext_clk(w->codec, 1);
	case SND_SOC_DAPM_POST_PMD:
		return msm8960_enable_codec_ext_clk(w->codec, 0);
	}
	return 0;
}

enum {
	RX_SWITCH_INDEX = 0,
	TX_SWITCH_INDEX,
	SWITCH_MAX,
};

static const struct snd_kcontrol_new extspk_switch_controls =
	SOC_DAPM_SINGLE("Switch", RX_SWITCH_INDEX, 0, 1, 0);

static const struct snd_kcontrol_new earamp_switch_controls =
	SOC_DAPM_SINGLE("Switch", RX_SWITCH_INDEX, 0, 1, 0);

static const struct snd_kcontrol_new spkamp_switch_controls =
	SOC_DAPM_SINGLE("Switch", RX_SWITCH_INDEX, 0, 1, 0);

static const struct snd_kcontrol_new micbias3_switch_controls =
	SOC_DAPM_SINGLE("Switch", TX_SWITCH_INDEX, 0, 1, 0);

static const struct snd_soc_dapm_widget elite_dapm_widgets[] = {
	SND_SOC_DAPM_MIXER("Lineout Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("SPK AMP EN", SND_SOC_NOPM, 0, 0, &spkamp_switch_controls, 1),
	SND_SOC_DAPM_MIXER("HAC AMP EN", SND_SOC_NOPM, 0, 0, &earamp_switch_controls, 1),
	SND_SOC_DAPM_MIXER("DOCK AMP EN", SND_SOC_NOPM, 0, 0, &extspk_switch_controls, 1),
	SND_SOC_DAPM_MIXER("DUAL MICBIAS", SND_SOC_NOPM, 0, 0, &micbias3_switch_controls, 1),
};

static const struct snd_soc_dapm_widget msm8960_dapm_widgets[] = {
	SND_SOC_DAPM_SUPPLY("MCLK",  SND_SOC_NOPM, 0, 0,
	msm8960_mclk_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_SPK("Ext Spk Bottom Pos", msm8960_spkramp_event),
	SND_SOC_DAPM_SPK("Ext Spk Bottom Neg", msm8960_spkramp_event),

	SND_SOC_DAPM_SPK("Ext Spk Top Pos", msm8960_spkramp_event),
	SND_SOC_DAPM_SPK("Ext Spk Top Neg", msm8960_spkramp_event),

	SND_SOC_DAPM_SPK("Dock Spk Pos", msm8960_spkramp_event),
	SND_SOC_DAPM_SPK("Dock Spk Neg", msm8960_spkramp_event),

	SND_SOC_DAPM_MIC("Handset Mic", NULL),
	SND_SOC_DAPM_MIC("Headset Mic", NULL),
	SND_SOC_DAPM_MIC("Back Mic", NULL),
	SND_SOC_DAPM_MIC("Digital Mic1", NULL),
	SND_SOC_DAPM_MIC("ANCRight Headset Mic", NULL),
	SND_SOC_DAPM_MIC("ANCLeft Headset Mic", NULL),

	SND_SOC_DAPM_MIC("Digital Mic1", NULL),
	SND_SOC_DAPM_MIC("Digital Mic2", NULL),
	SND_SOC_DAPM_MIC("Digital Mic3", NULL),
	SND_SOC_DAPM_MIC("Digital Mic4", NULL),
	SND_SOC_DAPM_MIC("Digital Mic5", NULL),
	SND_SOC_DAPM_MIC("Digital Mic6", NULL),

};

static const struct snd_soc_dapm_route tabla_1_x_audio_map[] = {

	{"Lineout Mixer", NULL, "LINEOUT2"},
	{"Lineout Mixer", NULL, "LINEOUT1"},
};

static const struct snd_soc_dapm_route tabla_2_x_audio_map[] = {

	{"Lineout Mixer", NULL, "LINEOUT3"},
	{"Lineout Mixer", NULL, "LINEOUT1"},
};

static const struct snd_soc_dapm_route common_audio_map[] = {

	{"RX_BIAS", NULL, "MCLK"},
	{"LDO_H", NULL, "MCLK"},

	/* Speaker path */
	{"Ext Spk Bottom Pos", NULL, "SPK AMP EN"},
	{"Ext Spk Bottom Neg", NULL, "SPK AMP EN"},
	{"SPK AMP EN", "Switch", "Lineout Mixer"},

	/* HAC path */
	{"Ext Spk Top Pos", NULL, "HAC AMP EN"},
	{"Ext Spk Top Neg", NULL, "HAC AMP EN"},
	{"HAC AMP EN", "Switch", "Lineout Mixer"},

	/* Cradle Dock path */
	{"Dock Spk Pos", NULL, "DOCK AMP EN"},
	{"Dock Spk Neg", NULL, "DOCK AMP EN"},
	{"DOCK AMP EN", "Switch", "Lineout Mixer"},

	/* Microphone path */
	{"AMIC1", NULL, "DUAL MICBIAS"},
	{"DUAL MICBIAS", NULL, "MIC BIAS1 External"},
	{"MIC BIAS1 External", NULL, "Handset Mic"},

	{"DUAL MICBIAS", "Switch", "MIC BIAS3 External"},

	{"AMIC2", NULL, "MIC BIAS2 External"},
	{"MIC BIAS2 External", NULL, "Headset Mic"},

	{"AMIC3", NULL, "MIC BIAS3 External"},
	{"MIC BIAS3 External", NULL, "Back Mic"},

	{"HEADPHONE", NULL, "LDO_H"},

	/**
	 * The digital Mic routes are setup considering
	 * fluid as default device.
	 */

	/**
	 * Digital Mic1. Front Bottom left Digital Mic on Fluid and MTP.
	 * Digital Mic GM5 on CDP mainboard.
	 * Conncted to DMIC2 Input on Tabla codec.
	 */
	{"DMIC2", NULL, "MIC BIAS1 External"},
	{"MIC BIAS1 External", NULL, "Digital Mic1"},

	/**
	 * Digital Mic2. Front Bottom right Digital Mic on Fluid and MTP.
	 * Digital Mic GM6 on CDP mainboard.
	 * Conncted to DMIC1 Input on Tabla codec.
	 */
	{"DMIC1", NULL, "MIC BIAS1 External"},
	{"MIC BIAS1 External", NULL, "Digital Mic2"},

	/**
	 * Digital Mic3. Back Bottom Digital Mic on Fluid.
	 * Digital Mic GM1 on CDP mainboard.
	 * Conncted to DMIC4 Input on Tabla codec.
	 */
	{"DMIC4", NULL, "MIC BIAS3 External"},
	{"MIC BIAS3 External", NULL, "Digital Mic3"},

	/**
	 * Digital Mic4. Back top Digital Mic on Fluid.
	 * Digital Mic GM2 on CDP mainboard.
	 * Conncted to DMIC3 Input on Tabla codec.
	 */
	{"DMIC3", NULL, "MIC BIAS3 External"},
	{"MIC BIAS3 External", NULL, "Digital Mic4"},

	/**
	 * Digital Mic5. Front top Digital Mic on Fluid.
	 * Digital Mic GM3 on CDP mainboard.
	 * Conncted to DMIC5 Input on Tabla codec.
	 */
	{"DMIC5", NULL, "MIC BIAS4 External"},
	{"MIC BIAS4 External", NULL, "Digital Mic5"},

};

static const char *spk_function[] = {"Off", "On"};
static const char *slim0_rx_ch_text[] = {"One", "Two"};
static const char *slim0_tx_ch_text[] = {"One", "Two", "Three", "Four"};

static const struct soc_enum msm8960_enum[] = {
	SOC_ENUM_SINGLE_EXT(2, spk_function),
	SOC_ENUM_SINGLE_EXT(2, slim0_rx_ch_text),
	SOC_ENUM_SINGLE_EXT(4, slim0_tx_ch_text),
};

static const char *stereo_mic_voice[] = {"Off", "On"};
static const struct soc_enum elite_msm8960_enum[] = {
	SOC_ENUM_SINGLE_EXT(2, stereo_mic_voice),
};

static const char *btsco_rate_text[] = {"8000", "16000"};
static const struct soc_enum msm8960_btsco_enum[] = {
		SOC_ENUM_SINGLE_EXT(2, btsco_rate_text),
};

static int msm8960_slim_0_rx_ch_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm8960_slim_0_rx_ch  = %d\n", __func__,
			msm8960_slim_0_rx_ch);
	ucontrol->value.integer.value[0] = msm8960_slim_0_rx_ch - 1;
	return 0;
}

static int msm8960_slim_0_rx_ch_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	msm8960_slim_0_rx_ch = ucontrol->value.integer.value[0] + 1;

	pr_debug("%s: msm8960_slim_0_rx_ch = %d\n", __func__,
			msm8960_slim_0_rx_ch);
	return 1;
}

static int msm8960_slim_0_tx_ch_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm8960_slim_0_tx_ch  = %d\n", __func__,
			msm8960_slim_0_tx_ch);
	ucontrol->value.integer.value[0] = msm8960_slim_0_tx_ch - 1;
	return 0;
}

static int msm8960_slim_0_tx_ch_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	msm8960_slim_0_tx_ch = ucontrol->value.integer.value[0] + 1;

	pr_debug("%s: msm8960_slim_0_tx_ch = %d\n", __func__,
			msm8960_slim_0_tx_ch);
	return 1;
}

static int elite_stereo_voice_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	pr_aud_info("%s: elite_stereo_control = %d\n", __func__,
			elite_stereo_control);
	ucontrol->value.integer.value[0] = elite_stereo_control;
	return 0;
}

static int elite_stereo_voice_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	struct pm_gpio param = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 1,
		.pull      = PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_L17,
		.out_strength   = PM_GPIO_STRENGTH_MED,
		.function       = PM_GPIO_FUNC_NORMAL,
	};

	if (elite_stereo_control == ucontrol->value.integer.value[0])
		return 0;

	elite_stereo_control = ucontrol->value.integer.value[0];

	pr_aud_info("%s: elite_stereo_control = %d\n", __func__,
			elite_stereo_control);

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		/* turn off selected pin of stereo mic to bypass A1028 */
		gpio_direction_output(ELITE_AUD_STEREO_REC, 1);
		gpio_free(ELITE_AUD_STEREO_REC);

		break;
	case 1:
		/* turn on selection of stereo mic to enable A1028 */
		ret = gpio_request(ELITE_AUD_STEREO_REC, "A1028_SWITCH");
		if (ret) {
			pr_aud_err("%s: Failed to request gpio %d\n", __func__,
					ELITE_AUD_STEREO_REC);
			return ret;
		}

		ret = pm8xxx_gpio_config(ELITE_AUD_STEREO_REC, &param);

		if (ret)
			pr_aud_err("%s: Failed to configure gpio %d\n", __func__,
					ELITE_AUD_STEREO_REC);
		else
			gpio_direction_output(ELITE_AUD_STEREO_REC, 0);
		break;
	}
	return ret;
}

static int msm8960_btsco_rate_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm8960_btsco_rate  = %d", __func__,
					msm8960_btsco_rate);
	ucontrol->value.integer.value[0] = msm8960_btsco_rate;
	return 0;
}

static int msm8960_btsco_rate_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	switch (ucontrol->value.integer.value[0]) {
	case 0:
		msm8960_btsco_rate = BTSCO_RATE_8KHZ;
		break;
	case 1:
		msm8960_btsco_rate = BTSCO_RATE_16KHZ;
		break;
	default:
		msm8960_btsco_rate = BTSCO_RATE_8KHZ;
		break;
	}
	pr_debug("%s: msm8960_btsco_rate = %d\n", __func__,
					msm8960_btsco_rate);
	return 0;
}

static const struct snd_kcontrol_new tabla_msm8960_controls[] = {
	SOC_ENUM_EXT("Speaker Function", msm8960_enum[0], msm8960_get_spk,
		msm8960_set_spk),
	SOC_ENUM_EXT("SLIM_0_RX Channels", msm8960_enum[1],
		msm8960_slim_0_rx_ch_get, msm8960_slim_0_rx_ch_put),
	SOC_ENUM_EXT("SLIM_0_TX Channels", msm8960_enum[2],
		msm8960_slim_0_tx_ch_get, msm8960_slim_0_tx_ch_put),
};

static const struct snd_kcontrol_new elite_msm8960_controls[] = {
	SOC_ENUM_EXT("Stereo Selection", elite_msm8960_enum[0], elite_stereo_voice_get,
		elite_stereo_voice_put),
};


static const struct snd_kcontrol_new int_btsco_rate_mixer_controls[] = {
	SOC_ENUM_EXT("Internal BTSCO SampleRate", msm8960_btsco_enum[0],
		msm8960_btsco_rate_get, msm8960_btsco_rate_put),
};

static int msm8960_btsco_init(struct snd_soc_pcm_runtime *rtd)
{
	int err = 0;
	struct snd_soc_platform *platform = rtd->platform;

	err = snd_soc_add_platform_controls(platform,
			int_btsco_rate_mixer_controls,
		ARRAY_SIZE(int_btsco_rate_mixer_controls));
	if (err < 0)
		return err;
	return 0;
}

static int msm8960_audrx_init(struct snd_soc_pcm_runtime *rtd)
{
	int err;
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	u8 tabla_version;

	pr_aud_info("%s()\n", __func__);

	rtd->pmdown_time = 0;
	err = snd_soc_add_controls(codec, tabla_msm8960_controls,
				ARRAY_SIZE(tabla_msm8960_controls));
	if (err < 0)
		return err;

	err = snd_soc_add_controls(codec, elite_msm8960_controls,
				ARRAY_SIZE(elite_msm8960_controls));
	if (err < 0) {
		pr_aud_err("%s() error in register asoc control for elite\n", __func__);
		return err;
	}

	snd_soc_dapm_new_controls(dapm, msm8960_dapm_widgets,
				ARRAY_SIZE(msm8960_dapm_widgets));

	snd_soc_dapm_new_controls(dapm, elite_dapm_widgets,
				ARRAY_SIZE(elite_dapm_widgets));

	snd_soc_dapm_add_routes(dapm, common_audio_map,
		ARRAY_SIZE(common_audio_map));

	/* determine HW connection based on codec revision */
	tabla_version = snd_soc_read(codec, TABLA_A_CHIP_VERSION);
	tabla_version &=  0x1F;
	pr_aud_info("%s : Tabla version %u\n", __func__, (u32)tabla_version);

	if ((tabla_version == TABLA_VERSION_1_0) ||
		(tabla_version == TABLA_VERSION_1_1)) {
		snd_soc_dapm_add_routes(dapm, tabla_1_x_audio_map,
			 ARRAY_SIZE(tabla_1_x_audio_map));
	} else {
		snd_soc_dapm_add_routes(dapm, tabla_2_x_audio_map,
			 ARRAY_SIZE(tabla_2_x_audio_map));
	}

	snd_soc_dapm_enable_pin(dapm, "Ext Spk Bottom Pos");
	snd_soc_dapm_enable_pin(dapm, "Ext Spk Bottom Neg");
	snd_soc_dapm_enable_pin(dapm, "Ext Spk Top Pos");
	snd_soc_dapm_enable_pin(dapm, "Ext Spk Top Neg");
	snd_soc_dapm_enable_pin(dapm, "Dock Spk Pos");
	snd_soc_dapm_enable_pin(dapm, "Dock Spk Neg");

	snd_soc_dapm_sync(dapm);

	err = snd_soc_jack_new(codec, "Headset Jack",
		(SND_JACK_HEADSET | SND_JACK_OC_HPHL | SND_JACK_OC_HPHR),
		&hs_jack);
	if (err) {
		pr_aud_err("failed to create new jack\n");
		return err;
	}

	err = snd_soc_jack_new(codec, "Button Jack",
				SND_JACK_BTN_0, &button_jack);
	if (err) {
		pr_aud_err("failed to create new jack\n");
		return err;
	}

	/* Do not pass MBHC calibration data to disable HS detection.
	 * Otherwise, sending project-based cal-data to enable
	 * MBHC mechanism that tabla provides */
	tabla_hs_detect(codec, &hs_jack, &button_jack, NULL,
			TABLA_MICBIAS2, msm8960_enable_codec_ext_clk, 0,
			TABLA_EXT_CLK_RATE);

	return 0;
}

static struct snd_soc_dsp_link lpa_fe_media = {
	.playback = true,
	.trigger = {
		SND_SOC_DSP_TRIGGER_POST,
		SND_SOC_DSP_TRIGGER_POST
	},
};

static struct snd_soc_dsp_link fe_media = {
	.playback = true,
	.capture = true,
	.trigger = {
		SND_SOC_DSP_TRIGGER_POST,
		SND_SOC_DSP_TRIGGER_POST
	},
};

static struct snd_soc_dsp_link slimbus0_hl_media = {
	.playback = true,
	.capture = true,
	.trigger = {
		SND_SOC_DSP_TRIGGER_POST,
		SND_SOC_DSP_TRIGGER_POST
	},
};

static struct snd_soc_dsp_link int_fm_hl_media = {
	.playback = true,
	.capture = true,
	.trigger = {
		SND_SOC_DSP_TRIGGER_POST,
		SND_SOC_DSP_TRIGGER_POST
	},
};

static int msm8960_slim_0_rx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
	SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
			SNDRV_PCM_HW_PARAM_CHANNELS);

	pr_debug("%s()\n", __func__);
	rate->min = rate->max = 48000;
	channels->min = channels->max = msm8960_slim_0_rx_ch;

	return 0;
}

static int msm8960_slim_0_tx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
	SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
			SNDRV_PCM_HW_PARAM_CHANNELS);

	pr_debug("%s()\n", __func__);
	rate->min = rate->max = 48000;
	channels->min = channels->max = msm8960_slim_0_tx_ch;

	return 0;
}

static int msm8960_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
	SNDRV_PCM_HW_PARAM_RATE);

	pr_debug("%s()\n", __func__);
	rate->min = rate->max = 48000;

	return 0;
}

static int msm8960_btsco_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
					struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	rate->min = rate->max = msm8960_btsco_rate;
	channels->min = channels->max = msm8960_btsco_ch;

	return 0;
}
static int msm8960_auxpcm_be_params_fixup(struct snd_soc_pcm_runtime *rtd,
					struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	/* PCM only supports mono output with 8khz sample rate */
	rate->min = rate->max = 8000;
	channels->min = channels->max = 1;

	return 0;
}
static int msm8960_aux_pcm_get_gpios(void)
{
	int ret = 0;

	pr_debug("%s\n", __func__);

	ret = gpio_request(GPIO_AUX_PCM_DOUT, "AUX PCM DOUT");
	if (ret < 0) {
		pr_aud_err("%s: Failed to request gpio(%d): AUX PCM DOUT",
				__func__, GPIO_AUX_PCM_DOUT);
		goto fail_dout;
	}

	ret = gpio_request(GPIO_AUX_PCM_DIN, "AUX PCM DIN");
	if (ret < 0) {
		pr_aud_err("%s: Failed to request gpio(%d): AUX PCM DIN",
				__func__, GPIO_AUX_PCM_DIN);
		goto fail_din;
	}

	ret = gpio_request(GPIO_AUX_PCM_SYNC, "AUX PCM SYNC");
	if (ret < 0) {
		pr_aud_err("%s: Failed to request gpio(%d): AUX PCM SYNC",
				__func__, GPIO_AUX_PCM_SYNC);
		goto fail_sync;
	}
	ret = gpio_request(GPIO_AUX_PCM_CLK, "AUX PCM CLK");
	if (ret < 0) {
		pr_aud_err("%s: Failed to request gpio(%d): AUX PCM CLK",
				__func__, GPIO_AUX_PCM_CLK);
		goto fail_clk;
	}

	return 0;

fail_clk:
	gpio_free(GPIO_AUX_PCM_SYNC);
fail_sync:
	gpio_free(GPIO_AUX_PCM_DIN);
fail_din:
	gpio_free(GPIO_AUX_PCM_DOUT);
fail_dout:

	return ret;
}

static int msm8960_aux_pcm_free_gpios(void)
{
	gpio_free(GPIO_AUX_PCM_DIN);
	gpio_free(GPIO_AUX_PCM_DOUT);
	gpio_free(GPIO_AUX_PCM_SYNC);
	gpio_free(GPIO_AUX_PCM_CLK);

	return 0;
}
static int msm8960_startup(struct snd_pcm_substream *substream)
{
	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
		 substream->name, substream->stream);
	return 0;
}

static int msm8960_auxpcm_startup(struct snd_pcm_substream *substream)
{
	int ret = 0;

	pr_debug("%s(): substream = %s\n", __func__, substream->name);
	ret = msm8960_aux_pcm_get_gpios();
	if (ret < 0) {
		pr_aud_err("%s: Aux PCM GPIO request failed\n", __func__);
		return -EINVAL;
	}
	return 0;
}

static void msm8960_auxpcm_shutdown(struct snd_pcm_substream *substream)
{

	pr_debug("%s(): substream = %s\n", __func__, substream->name);
	msm8960_aux_pcm_free_gpios();
}

static void msm8960_shutdown(struct snd_pcm_substream *substream)
{
	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
		 substream->name, substream->stream);
}

static struct snd_soc_ops msm8960_be_ops = {
	.startup = msm8960_startup,
	.shutdown = msm8960_shutdown,
};

static struct snd_soc_ops msm8960_auxpcm_be_ops = {
	.startup = msm8960_auxpcm_startup,
	.shutdown = msm8960_auxpcm_shutdown,
};

/* Digital audio interface glue - connects codec <---> CPU */
static struct snd_soc_dai_link msm8960_dai[] = {
	/* FrontEnd DAI Links */
	{
		.name = "MSM8960 Media1",
		.stream_name = "MultiMedia1",
		.cpu_dai_name	= "MultiMedia1",
		.platform_name  = "msm-pcm-dsp",
		.dynamic = 1,
		.dsp_link = &fe_media,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA1
	},
	{
		.name = "MSM8960 Media2",
		.stream_name = "MultiMedia2",
		.cpu_dai_name	= "MultiMedia2",
		.platform_name  = "msm-pcm-dsp",
		.dynamic = 1,
		.dsp_link = &fe_media,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA2,
	},
	{
		.name = "Circuit-Switch Voice",
		.stream_name = "CS-Voice",
		.cpu_dai_name   = "CS-VOICE",
		.platform_name  = "msm-pcm-voice",
		.dynamic = 1,
		.dsp_link = &fe_media,
		.be_id = MSM_FRONTEND_DAI_CS_VOICE,
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
	},
	{
		.name = "MSM VoIP",
		.stream_name = "VoIP",
		.cpu_dai_name	= "VoIP",
		.platform_name  = "msm-voip-dsp",
		.dynamic = 1,
		.dsp_link = &fe_media,
		.be_id = MSM_FRONTEND_DAI_VOIP,
	},
	{
		.name = "MSM8960 LPA",
		.stream_name = "LPA",
		.cpu_dai_name	= "MultiMedia3",
		.platform_name  = "msm-pcm-lpa",
		.dynamic = 1,
		.dsp_link = &lpa_fe_media,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA3,
		.ignore_suspend = 1,
	},
	/* Hostless PMC purpose */
	{
		.name = "SLIMBUS_0 Hostless",
		.stream_name = "SLIMBUS_0 Hostless",
		.cpu_dai_name	= "SLIMBUS0_HOSTLESS",
		.platform_name  = "msm-pcm-hostless",
		.dynamic = 1,
		.dsp_link = &slimbus0_hl_media,
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		/* .be_id = do not care */
	},
	{
		.name = "INT_FM Hostless",
		.stream_name = "INT_FM Hostless",
		.cpu_dai_name	= "INT_FM_HOSTLESS",
		.platform_name  = "msm-pcm-hostless",
		.dynamic = 1,
		.dsp_link = &int_fm_hl_media,
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
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
		.name = "MSM8960 Compr",
		.stream_name = "COMPR",
		.cpu_dai_name	= "MultiMedia4",
		.platform_name  = "msm-compr-dsp",
		.dynamic = 1,
		.dsp_link = &lpa_fe_media,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA4,
		.ignore_suspend = 1,
	},
	/* Backend DAI Links */
	{
		.name = LPASS_BE_SLIMBUS_0_RX,
		.stream_name = "Slimbus Playback",
		.cpu_dai_name = "msm-dai-q6.16384",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "tabla_codec",
		.codec_dai_name	= "tabla_rx1",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_SLIMBUS_0_RX,
		.init = &msm8960_audrx_init,
		.be_hw_params_fixup = msm8960_slim_0_rx_be_hw_params_fixup,
		.ops = &msm8960_be_ops,
	},
	{
		.name = LPASS_BE_SLIMBUS_0_TX,
		.stream_name = "Slimbus Capture",
		.cpu_dai_name = "msm-dai-q6.16385",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "tabla_codec",
		.codec_dai_name	= "tabla_tx1",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_SLIMBUS_0_TX,
		.be_hw_params_fixup = msm8960_slim_0_tx_be_hw_params_fixup,
		.ops = &msm8960_be_ops,
	},
	/* Backend BT/FM DAI Links */
	{
		.name = LPASS_BE_INT_BT_SCO_RX,
		.stream_name = "Internal BT-SCO Playback",
		.cpu_dai_name = "msm-dai-q6.12288",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name	= "msm-stub-rx",
		.init = &msm8960_btsco_init,
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_INT_BT_SCO_RX,
		.be_hw_params_fixup = msm8960_btsco_be_hw_params_fixup,
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
		.be_hw_params_fixup = msm8960_btsco_be_hw_params_fixup,
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
		.be_hw_params_fixup = msm8960_be_hw_params_fixup,
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
		.be_hw_params_fixup = msm8960_be_hw_params_fixup,
	},
	/* HDMI BACK END DAI Link */
	{
		.name = LPASS_BE_HDMI,
		.stream_name = "HDMI Playback",
		.cpu_dai_name = "msm-dai-q6.8",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.no_codec = 1,
		.be_id = MSM_BACKEND_DAI_HDMI_RX,
		.be_hw_params_fixup = msm8960_be_hw_params_fixup,
	},
	/* Backend AFE DAI Links */
	{
		.name = LPASS_BE_AFE_PCM_RX,
		.stream_name = "AFE Playback",
		.cpu_dai_name = "msm-dai-q6.224",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_codec = 1,
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_AFE_PCM_RX,
	},
	{
		.name = LPASS_BE_AFE_PCM_TX,
		.stream_name = "AFE Capture",
		.cpu_dai_name = "msm-dai-q6.225",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_codec = 1,
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_AFE_PCM_TX,
	},
	/* AUX PCM Backend DAI Links */
	{
		.name = LPASS_BE_AUXPCM_RX,
		.stream_name = "AUX PCM Playback",
		.cpu_dai_name = "msm-dai-q6.2",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_AUXPCM_RX,
		.be_hw_params_fixup = msm8960_auxpcm_be_params_fixup,
		.ops = &msm8960_auxpcm_be_ops,
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
		.be_hw_params_fixup = msm8960_auxpcm_be_params_fixup,
	},
	/* Incall Music BACK END DAI Link */
	{
		.name = LPASS_BE_VOICE_PLAYBACK_TX,
		.stream_name = "Voice Farend Playback",
		.cpu_dai_name = "msm-dai-q6.32773",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.no_codec = 1,
		.be_id = MSM_BACKEND_DAI_VOICE_PLAYBACK_TX,
		.be_hw_params_fixup = msm8960_be_hw_params_fixup,
	},
	/* Incall Record Uplink BACK END DAI Link */
	{
		.name = LPASS_BE_INCALL_RECORD_TX,
		.stream_name = "Voice Uplink Capture",
		.cpu_dai_name = "msm-dai-q6.32772",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.no_codec = 1,
		.be_id = MSM_BACKEND_DAI_INCALL_RECORD_TX,
		.be_hw_params_fixup = msm8960_be_hw_params_fixup,
	},
	/* Incall Record Downlink BACK END DAI Link */
	{
		.name = LPASS_BE_INCALL_RECORD_RX,
		.stream_name = "Voice Downlink Capture",
		.cpu_dai_name = "msm-dai-q6.32771",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.no_codec = 1,
		.be_id = MSM_BACKEND_DAI_INCALL_RECORD_RX,
		.be_hw_params_fixup = msm8960_be_hw_params_fixup,
	},
};

struct snd_soc_card snd_soc_card_msm8960 = {
	.name		= "msm8960-snd-card",
	.dai_link	= msm8960_dai,
	.num_links	= ARRAY_SIZE(msm8960_dai),
};

void elite_set_q6_effect_mode(int mode)
{
	pr_aud_info("%s: mode %d\n", __func__, mode);
	atomic_set(&q6_effect_mode, mode);
}

int elite_get_q6_effect_mode(void)
{
	int mode = atomic_read(&q6_effect_mode);
	pr_aud_info("%s: mode %d\n", __func__, mode);
	return mode;
}

int elite_get_component_info(void)
{
	int ret = 0;
	uint32_t audio_hw_comp = HTC_AUDIO_A1028;

	ret = gpio_request(ELITE_GPIO_NC_97, "audience_detect");
	if (ret) {
		pr_aud_err("%s: Error requesting Audience Detection GPIO %u\n",
			__func__, ELITE_GPIO_NC_97);
		return audio_hw_comp;
	}

	/*
	 * This GPIO is configured as Input/PU pin
	 * Value Low : support Audience
	 * Value High: Audience chip doesn't be mount
	 */
	if (gpio_get_value(ELITE_GPIO_NC_97))
		audio_hw_comp &= (~HTC_AUDIO_A1028);

	gpio_free(ELITE_GPIO_NC_97);
	return audio_hw_comp;
}

static struct acoustic_ops acoustic = {
	.set_q6_effect = elite_set_q6_effect_mode,
	.get_hw_component = elite_get_component_info,
};

static struct q6asm_ops qops = {
	.get_q6_effect = elite_get_q6_effect_mode,
};

static struct msm_pcm_routing_ops rops = {
	.get_q6_effect = elite_get_q6_effect_mode,
};

static struct platform_device *msm8960_snd_device;

#ifdef CONFIG_AUDIO_USAGE_FOR_POWER_CONSUMPTION
static ssize_t speaker_stat_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	if (g_spk_flag == 1) {
		g_spk_end_time = current_kernel_time();

		g_spk_total_time += (g_spk_end_time.tv_sec - g_spk_start_time.tv_sec);
		g_spk_start_time = current_kernel_time();
	}
	return sprintf(buf, "%lu\n", g_spk_total_time);
}
static ssize_t headset_stat_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	if (g_hs_flag == 1) {
		g_hs_end_time = current_kernel_time();

		g_hs_total_time += (g_hs_end_time.tv_sec - g_hs_start_time.tv_sec);
		g_hs_start_time = current_kernel_time();
	}
	return sprintf(buf, "%lu\n", g_hs_total_time);
}
#endif

static int __init elite_audio_init(void)
{
	int ret;
	#ifdef CONFIG_AUDIO_USAGE_FOR_POWER_CONSUMPTION
	int retval = 0;
	#endif
	pr_aud_info("%s", __func__);

	msm8960_snd_device = platform_device_alloc("soc-audio", 0);
	if (!msm8960_snd_device) {
		pr_aud_err("%s, Platform device allocation failed\n", __func__);
		return -ENOMEM;
	}

	platform_set_drvdata(msm8960_snd_device, &snd_soc_card_msm8960);
	ret = platform_device_add(msm8960_snd_device);
	if (ret) {
		pr_aud_err("%s, Platform device add failed\n", __func__);
		platform_device_put(msm8960_snd_device);
		return ret;
	}

	mutex_init(&audio_notifier_lock);
	pr_aud_info("%s: register cable detect func for dock", __func__);
	ret = cable_detect_register_notifier(&audio_dock_notifier);

	#ifdef CONFIG_AUDIO_USAGE_FOR_POWER_CONSUMPTION
	/* Create two audio status file nodes, audio_speaker_t and
	 * audio_headset_t under /sys/audio_stats/					*/
	if (audio_stat_kobj == NULL)
	{
		audio_stat_kobj = kobject_create_and_add("audio_stats", NULL);
		retval = sysfs_create_file(audio_stat_kobj, attrs[0]);
		if (!retval)
		{
			pr_err("Speaker file node creation failed under kobject\n");
		}
		retval = sysfs_create_file(audio_stat_kobj, attrs[1]);
		if (!retval)
		{
			pr_err("Headset file node creation failed under kobject\n");
		}
	}
	#endif

	htc_8960_register_q6asm_ops(&qops);
	htc_8960_register_pcm_routing_ops(&rops);
	acoustic_register_ops(&acoustic);
	return ret;

}
late_initcall(elite_audio_init);

static void __exit elite_audio_exit(void)
{
	pr_aud_info("%s", __func__);
	platform_device_unregister(msm8960_snd_device);
}
module_exit(elite_audio_exit);

MODULE_DESCRIPTION("ALSA Platform Elite");
MODULE_LICENSE("GPL v2");
