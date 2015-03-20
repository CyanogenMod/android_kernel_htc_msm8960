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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/slimbus/slimbus.h>
#include <linux/input.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/jack.h>
#include <asm/mach-types.h>
#include <mach/socinfo.h>
#include "msm-pcm-routing.h"
#include "../codecs/wcd9310.h"

#include <mach/htc_acoustic_8960.h>
#include <linux/tfa9887.h>
#include <linux/rt5501.h>
#include <sound/q6asm.h>
#include "board-t6.h"

/* 8064 machine driver */
#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)  (pm_gpio - 1 + PM8921_GPIO_BASE)

#define MSM8064_SPK_ON 1
#define MSM8064_SPK_OFF 0

#define MSM_SLIM_0_RX_MAX_CHANNELS		2
#define MSM_SLIM_0_TX_MAX_CHANNELS		4

#define SAMPLE_RATE_8KHZ 8000
#define SAMPLE_RATE_16KHZ 16000
#define SAMPLE_RATE_48KHZ 48000

#define BOTTOM_SPK_AMP_POS	0x1
#define BOTTOM_SPK_AMP_NEG	0x2
#define TOP_SPK_AMP_POS		0x4
#define TOP_SPK_AMP_NEG		0x8
#define TOP_SPK_AMP		0x10
#define HS_AMP_POS		0x20
#define HS_AMP_NEG		0x40
#define RCV_AMP_POS		0x80
#define RCV_AMP_NEG		0x100

#define GPIO_MI2S_RX_WS     27
#define GPIO_MI2S_RX_SCLK   28
#define GPIO_MI2S_RX_DOUT3  29
#define GPIO_MI2S_RX_DOUT0  32

#define TABLA_EXT_CLK_RATE 12288000

#define TABLA_MBHC_DEF_BUTTONS 8
#define TABLA_MBHC_DEF_RLOADS 5

#define HAC_PAMP_GPIO        6
#define RCV_PAMP_PMGPIO     33
#define RCV_SPK_SEL_PMGPIO  24
#define AUD_HP_EN_GPIO      PM8921_GPIO_PM_TO_SYS(37)

/* Shared channel numbers for Slimbus ports that connect APQ to MDM. */
enum {
	SLIM_1_RX_1 = 145, /* BT-SCO and USB TX */
	SLIM_1_TX_1 = 146, /* BT-SCO and USB RX */
	SLIM_1_TX_2 = 147, /* USB RX */
	SLIM_3_RX_1 = 151, /* External echo-cancellation ref */
	SLIM_3_RX_2 = 152, /* External echo-cancellation ref */
	SLIM_3_TX_1 = 153, /* HDMI RX */
	SLIM_3_TX_2 = 154, /* HDMI RX */
	SLIM_4_TX_1 = 148, /* In-call recording RX */
	SLIM_4_TX_2 = 149, /* In-call recording RX */
	SLIM_4_RX_1 = 150, /* In-call music delivery TX */
};

enum {
	INCALL_REC_MONO,
	INCALL_REC_STEREO,
};

static int msm_spk_control;
static int msm_hs_pamp;
static int msm_rcv_pamp;
static int msm_slim_0_rx_ch = 1;
static int msm_slim_0_tx_ch = 1;
static int msm_slim_3_rx_ch = 1;

static int msm_slim_1_rate = SAMPLE_RATE_8KHZ;
static int msm_btsco_ch = 1;
static int msm_slim_1_tx_ch = 1;

static int msm_hdmi_rx_ch = 2;
static int hdmi_rate_variable;
static int rec_mode = INCALL_REC_MONO;

static struct clk *mi2s_rx_osr_clk;
static struct clk *mi2s_rx_bit_clk;
static struct clk *pri_i2s_rx_osr_clk;
static struct clk *pri_i2s_rx_bit_clk;
static struct clk *codec_clk;
static int clk_users;

static struct snd_soc_jack hs_jack;
static struct snd_soc_jack button_jack;
static atomic_t auxpcm_rsc_ref;
static atomic_t q6_effect_mode = ATOMIC_INIT(-1);

static int apq8064_hs_detect_use_gpio = -1;
module_param(apq8064_hs_detect_use_gpio, int, 0444);
MODULE_PARM_DESC(apq8064_hs_detect_use_gpio, "Use GPIO for headset detection");

static bool apq8064_hs_detect_extn_cable;
module_param(apq8064_hs_detect_extn_cable, bool, 0444);
MODULE_PARM_DESC(apq8064_hs_detect_extn_cable, "Enable extension cable feature");

static bool apq8064_hs_detect_use_firmware;
module_param(apq8064_hs_detect_use_firmware, bool, 0444);
MODULE_PARM_DESC(apq8064_hs_detect_use_firmware, "Use firmware for headset "
		 "detection");

static int msm_enable_codec_ext_clk(struct snd_soc_codec *codec, int enable,
				    bool dapm);

static struct tabla_mbhc_config mbhc_cfg = {
	.headset_jack = &hs_jack,
	.button_jack = &button_jack,
	.read_fw_bin = false,
	.calibration = NULL,
	.micbias = TABLA_MICBIAS2,
	.mclk_cb_fn = msm_enable_codec_ext_clk,
	.mclk_rate = TABLA_EXT_CLK_RATE,
	.gpio = 0,
	.gpio_irq = 0,
	.gpio_level_insert = 1,
	.detect_extn_cable = false,
};

static uint32_t audio_mi2s_active_table[] = {
	GPIO_CFG(GPIO_MI2S_RX_WS, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
	GPIO_CFG(GPIO_MI2S_RX_SCLK, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
	GPIO_CFG(GPIO_MI2S_RX_DOUT3, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
	GPIO_CFG(GPIO_MI2S_RX_DOUT0, 1, GPIO_CFG_INPUT,  GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t audio_mi2s_sleep_table[] = {
	GPIO_CFG(GPIO_MI2S_RX_WS, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA),
	GPIO_CFG(GPIO_MI2S_RX_SCLK, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA),
	GPIO_CFG(GPIO_MI2S_RX_DOUT3, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_6MA),
	GPIO_CFG(GPIO_MI2S_RX_DOUT0, 1, GPIO_CFG_INPUT,  GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
};

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

static struct mutex cdc_mclk_mutex;

static void msm_enable_ext_spk_amp_gpio(u32 spk_amp_gpio)
{
	int ret = 0;

	static uint32_t rcv_amp_gpio_table[] = {
		GPIO_CFG(RCV_PAMP_PMGPIO, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	};

	if (spk_amp_gpio == RCV_PAMP_PMGPIO) {

		ret = gpio_request(RCV_PAMP_PMGPIO, "AUDIO_RCV_AMP");
		if (ret) {
			pr_err("%s: Error requesting AUDIO RCV AMP GPIO %u\n",
				__func__, RCV_PAMP_PMGPIO);
			return;
		}
		ret = gpio_request(PM8921_GPIO_PM_TO_SYS(RCV_SPK_SEL_PMGPIO), "RCV_SPK_SEL");
		if (ret) {
			pr_err("%s: Error requesting RCV SPK SEL PMGPIO %u\n",
				__func__, PM8921_GPIO_PM_TO_SYS(RCV_SPK_SEL_PMGPIO));
			gpio_free(RCV_PAMP_PMGPIO);
			return;
		}
		ret = gpio_tlmm_config(rcv_amp_gpio_table[0], GPIO_CFG_DISABLE);
		if (ret) {
			pr_err("%s: Failed to configure Rcv Ampl"
					" gpio %u\n", __func__, RCV_PAMP_PMGPIO);
		} else {
			pr_debug("%s: enable Rcv amp gpio\n", __func__);
			gpio_direction_output(PM8921_GPIO_PM_TO_SYS(RCV_PAMP_PMGPIO), 1);
			gpio_direction_output(PM8921_GPIO_PM_TO_SYS(RCV_SPK_SEL_PMGPIO), 1);
		}

	}
}

static void msm_ext_spk_power_amp_on(u32 spk)
{
	if (spk & (RCV_AMP_POS | RCV_AMP_NEG)) {

		if ((msm_rcv_pamp & RCV_AMP_POS) &&
			(msm_rcv_pamp & RCV_AMP_NEG)) {

			pr_debug("%s() RCV Ampl already"
				"turned on. spk = 0x%08x\n", __func__, spk);
			return;
		}

		msm_rcv_pamp |= spk;

		if ((msm_rcv_pamp & RCV_AMP_POS) &&
			(msm_rcv_pamp & RCV_AMP_NEG)) {

			msm_enable_ext_spk_amp_gpio(RCV_PAMP_PMGPIO);
			pr_debug("%s: sleeping 4 ms after turning on RCV Ampl\n", __func__);
			usleep_range(4000, 4000);
		}

	} else if (spk & (HS_AMP_POS | HS_AMP_NEG)) {

		if ((msm_hs_pamp & HS_AMP_POS) &&
			(msm_hs_pamp & HS_AMP_NEG)) {
			pr_debug("%s() HS Ampl already"
				"turned on. spk = 0x%08x\n", __func__, spk);
			return;
		}

		msm_hs_pamp |= spk;

		if ((msm_hs_pamp & HS_AMP_POS) &&
			(msm_hs_pamp & HS_AMP_NEG)) {
#ifdef CONFIG_AMP_RT5501
			if (query_rt5501())
				set_rt5501_amp(1);
#endif
			pr_debug("%s: sleeping 4 ms after turning on HS Ampl\n", __func__);
			usleep_range(4000, 4000);
		}
	}
}

static void msm_ext_spk_power_amp_off(u32 spk)
{
	if (spk & (RCV_AMP_POS | RCV_AMP_NEG)) {

		if (!msm_rcv_pamp)
			return;

		gpio_direction_output(PM8921_GPIO_PM_TO_SYS(RCV_PAMP_PMGPIO), 0);
		gpio_free(RCV_PAMP_PMGPIO);
		gpio_direction_output(PM8921_GPIO_PM_TO_SYS(RCV_SPK_SEL_PMGPIO), 0);
		gpio_free(PM8921_GPIO_PM_TO_SYS(RCV_SPK_SEL_PMGPIO));
		msm_rcv_pamp = 0;

		pr_debug("%s: sleeping 4 ms after turning off external Bottom"
			" Speaker Ampl\n", __func__);

		usleep_range(4000, 4000);

	} else if (spk & (HS_AMP_POS | HS_AMP_NEG)) {

		if (!msm_hs_pamp)
			return;

#ifdef CONFIG_AMP_RT5501
		if (query_rt5501())
			set_rt5501_amp(0);
#endif
		msm_hs_pamp = 0;

		pr_debug("%s: sleeping 4 ms after turning off external Bottom"
			" Speaker Ampl\n", __func__);

		usleep_range(4000, 4000);

	}
}

static void msm_ext_control(struct snd_soc_codec *codec)
{
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	pr_debug("%s: msm_spk_control = %d", __func__, msm_spk_control);
	if (msm_spk_control == MSM8064_SPK_ON) {
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

static int msm_get_spk(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm_spk_control = %d", __func__, msm_spk_control);
	ucontrol->value.integer.value[0] = msm_spk_control;
	return 0;
}
static int msm_set_spk(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	pr_debug("%s()\n", __func__);
	if (msm_spk_control == ucontrol->value.integer.value[0])
		return 0;

	msm_spk_control = ucontrol->value.integer.value[0];
	msm_ext_control(codec);
	return 1;
}
static int msm_spkramp_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *k, int event)
{
	pr_debug("%s() %x\n", __func__, SND_SOC_DAPM_EVENT_ON(event));

	if (SND_SOC_DAPM_EVENT_ON(event)) {
		if (!strncmp(w->name, "Ext Spk Bottom Pos", 18))
			msm_ext_spk_power_amp_on(BOTTOM_SPK_AMP_POS);
		else if (!strncmp(w->name, "Ext Spk Bottom Neg", 18))
			msm_ext_spk_power_amp_on(BOTTOM_SPK_AMP_NEG);
		else if (!strncmp(w->name, "Ext Spk Top Pos", 15))
			msm_ext_spk_power_amp_on(TOP_SPK_AMP_POS);
		else if  (!strncmp(w->name, "Ext Spk Top Neg", 15))
			msm_ext_spk_power_amp_on(TOP_SPK_AMP_NEG);
		else if  (!strncmp(w->name, "Ext Spk Top", 12))
			msm_ext_spk_power_amp_on(TOP_SPK_AMP);
		else if (!strncmp(w->name, "Ext Hs Pos", 10))
			msm_ext_spk_power_amp_on(HS_AMP_POS);
		else if  (!strncmp(w->name, "Ext Hs Neg", 10))
			msm_ext_spk_power_amp_on(HS_AMP_NEG);
		else if (!strncmp(w->name, "Ext Rcv Pos", 11))
			msm_ext_spk_power_amp_on(RCV_AMP_POS);
		else if  (!strncmp(w->name, "Ext Rcv Neg", 11))
			msm_ext_spk_power_amp_on(RCV_AMP_NEG);
		else {
			pr_err("%s() Invalid Speaker Widget = %s\n",
					__func__, w->name);
			return -EINVAL;
		}

	} else {
		if (!strncmp(w->name, "Ext Spk Bottom Pos", 18))
			msm_ext_spk_power_amp_off(BOTTOM_SPK_AMP_POS);
		else if (!strncmp(w->name, "Ext Spk Bottom Neg", 18))
			msm_ext_spk_power_amp_off(BOTTOM_SPK_AMP_NEG);
		else if (!strncmp(w->name, "Ext Spk Top Pos", 15))
			msm_ext_spk_power_amp_off(TOP_SPK_AMP_POS);
		else if  (!strncmp(w->name, "Ext Spk Top Neg", 15))
			msm_ext_spk_power_amp_off(TOP_SPK_AMP_NEG);
		else if  (!strncmp(w->name, "Ext Spk Top", 12))
			msm_ext_spk_power_amp_off(TOP_SPK_AMP);
		else if (!strncmp(w->name, "Ext Hs Pos", 10))
			msm_ext_spk_power_amp_off(HS_AMP_POS);
		else if  (!strncmp(w->name, "Ext Hs Neg", 10))
			msm_ext_spk_power_amp_off(HS_AMP_NEG);
		else if (!strncmp(w->name, "Ext Rcv Pos", 11))
			msm_ext_spk_power_amp_off(RCV_AMP_POS);
		else if  (!strncmp(w->name, "Ext Rcv Neg", 11))
			msm_ext_spk_power_amp_off(RCV_AMP_NEG);
		else {
			pr_err("%s() Invalid Speaker Widget = %s\n",
					__func__, w->name);
			return -EINVAL;
		}
	}
	return 0;
}

static int msm_enable_codec_ext_clk(struct snd_soc_codec *codec, int enable,
				    bool dapm)
{
	int r = 0;
	pr_debug("%s: enable = %d\n", __func__, enable);

	mutex_lock(&cdc_mclk_mutex);
	if (enable) {
		clk_users++;
		pr_debug("%s: clk_users = %d\n", __func__, clk_users);
		if (clk_users == 1) {
			if (codec_clk) {
				clk_set_rate(codec_clk, TABLA_EXT_CLK_RATE);
				clk_prepare_enable(codec_clk);
				tabla_mclk_enable(codec, 1, dapm);
			} else {
				pr_err("%s: Error setting Tabla MCLK\n",
				       __func__);
				clk_users--;
				r = -EINVAL;
			}
		}
	} else {
		if (clk_users > 0) {
			clk_users--;
			pr_debug("%s: clk_users = %d\n", __func__, clk_users);
			if (clk_users == 0) {
				pr_debug("%s: disabling MCLK. clk_users = %d\n",
					 __func__, clk_users);
				tabla_mclk_enable(codec, 0, dapm);
				clk_disable_unprepare(codec_clk);
			}
		} else {
			pr_err("%s: Error releasing Tabla MCLK\n", __func__);
			r = -EINVAL;
		}
	}
	mutex_unlock(&cdc_mclk_mutex);
	return r;
}

static int msm_mclk_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	pr_debug("%s: event = %d\n", __func__, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:

		clk_users++;
		pr_debug("%s: clk_users = %d\n", __func__, clk_users);

		if (clk_users != 1)
			return 0;

		if (codec_clk) {
			clk_set_rate(codec_clk, 12288000);
			clk_prepare_enable(codec_clk);
			tabla_mclk_enable(w->codec, 1, true);

		} else {
			pr_err("%s: Error setting Tabla MCLK\n", __func__);
			clk_users--;
			return -EINVAL;
		}
		break;
	case SND_SOC_DAPM_POST_PMD:

		pr_debug("%s: clk_users = %d\n", __func__, clk_users);

		if (clk_users == 0)
			return 0;

		clk_users--;

		if (!clk_users) {
			pr_debug("%s: disabling MCLK. clk_users = %d\n",
					__func__, clk_users);

			tabla_mclk_enable(w->codec, 0, true);
			clk_disable_unprepare(codec_clk);
		}
		break;
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

static const struct snd_soc_dapm_widget apq8064_dapm_widgets[] = {

	SND_SOC_DAPM_SUPPLY("MCLK",  SND_SOC_NOPM, 0, 0,
	msm_mclk_event, SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_SPK("Ext Spk Bottom Pos", msm_spkramp_event),
	SND_SOC_DAPM_SPK("Ext Spk Bottom Neg", msm_spkramp_event),

	SND_SOC_DAPM_SPK("Ext Spk Top Pos", msm_spkramp_event),
	SND_SOC_DAPM_SPK("Ext Spk Top Neg", msm_spkramp_event),
	SND_SOC_DAPM_SPK("Ext Spk Top", msm_spkramp_event),

	SND_SOC_DAPM_SPK("Ext Hs Pos", msm_spkramp_event),
	SND_SOC_DAPM_SPK("Ext Hs Neg", msm_spkramp_event),

	SND_SOC_DAPM_SPK("Ext Rcv Pos", msm_spkramp_event),
	SND_SOC_DAPM_SPK("Ext Rcv Neg", msm_spkramp_event),

	/************ Analog MICs ************/
	/**
	 * Analog mic7 (Front Top) on Liquid.
	 * Used as Handset mic on CDP.
	 */
	SND_SOC_DAPM_MIC("Analog mic7", NULL),

	SND_SOC_DAPM_MIC("Headset Mic", NULL),
	SND_SOC_DAPM_MIC("ANCRight Headset Mic", NULL),
	SND_SOC_DAPM_MIC("ANCLeft Headset Mic", NULL),

	/*********** Digital Mics ***************/
	SND_SOC_DAPM_MIC("Digital Mic1", NULL),
	SND_SOC_DAPM_MIC("Digital Mic2", NULL),
	SND_SOC_DAPM_MIC("Digital Mic3", NULL),
	SND_SOC_DAPM_MIC("Digital Mic4", NULL),
	SND_SOC_DAPM_MIC("Digital Mic5", NULL),
	SND_SOC_DAPM_MIC("Digital Mic6", NULL),

	/*********** T6 Amp Controls ***************/
	SND_SOC_DAPM_MIXER("Lineout Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("SPK AMP EN", SND_SOC_NOPM, 0, 0, &spkamp_switch_controls, 1),
	SND_SOC_DAPM_MIXER("DOCK AMP EN", SND_SOC_NOPM, 0, 0, &extspk_switch_controls, 1),
	SND_SOC_DAPM_MIXER("HS AMP EN", SND_SOC_NOPM, 0, 0, &hsamp_switch_controls, 1),
	SND_SOC_DAPM_MIXER("RCV AMP EN", SND_SOC_NOPM, 0, 0, &rcvamp_switch_controls, 1),
};

static const struct snd_soc_dapm_route apq8064_common_audio_map[] = {

	{"RX_BIAS", NULL, "MCLK"},
	{"LDO_H", NULL, "MCLK"},

	{"HEADPHONE", NULL, "LDO_H"},

	/* Speaker path */
	{"Ext Spk Bottom Pos", NULL, "LINEOUT1"},
	{"Ext Spk Bottom Neg", NULL, "LINEOUT3"},

	{"Ext Spk Top Pos", NULL, "LINEOUT2"},
	{"Ext Spk Top Neg", NULL, "LINEOUT4"},
	{"Ext Spk Top", NULL, "LINEOUT5"},

	{"Ext Hs Pos", NULL, "HS AMP EN"},
	{"Ext Hs Neg", NULL, "HS AMP EN"},
	{"HS AMP EN", "Switch", "Lineout Mixer"},

	{"Ext Rcv Pos", NULL, "RCV AMP EN"},
	{"Ext Rcv Neg", NULL, "RCV AMP EN"},
	{"RCV AMP EN", "Switch", "Lineout Mixer"},

	{"Lineout Mixer", NULL, "LINEOUT3"},
	{"Lineout Mixer", NULL, "LINEOUT1"},

	/************   Analog MIC Paths  ************/

	/* T6 Handset Mic */
	{"AMIC1", NULL, "MIC BIAS1 External"},
	{"MIC BIAS1 External", NULL, "Analog mic7"},

	/* Headset Mic */
	{"AMIC2", NULL, "MIC BIAS2 External"},
	{"MIC BIAS2 External", NULL, "Headset Mic"},

	/* Headset ANC microphones */
	{"AMIC3", NULL, "MIC BIAS3 External"},
	{"MIC BIAS3 External", NULL, "ANCRight Headset Mic"},

	{"AMIC4", NULL, "MIC BIAS1 Internal2"},
	{"MIC BIAS1 Internal2", NULL, "ANCLeft Headset Mic"},
};

static const struct snd_soc_dapm_route apq8064_mtp_audio_map[] = {

	/************   Digital MIC Paths  ************/

	/*
	 * Digital Mic1 (Front bottom Left) on MTP.
	 * Conncted to DMIC1 Input on Tabla codec.
	 */
	{"DMIC1", NULL, "MIC BIAS1 External"},
	{"MIC BIAS1 External", NULL, "Digital Mic1"},

	/**
	 * Digital Mic2 (Front bottom right) on MTP.
	 * Conncted to DMIC2 Input on Tabla codec.
	 */
	{"DMIC2", NULL, "MIC BIAS1 External"},
	{"MIC BIAS1 External", NULL, "Digital Mic2"},

	/**
	 * Digital Mic3 (Back bottom) on MTP.
	 * Conncted to DMIC3 Input on Tabla codec.
	 */
	{"DMIC3", NULL, "MIC BIAS3 External"},
	{"MIC BIAS3 External", NULL, "Digital Mic3"},

	/**
	 * Digital Mic4 (Back top) on MTP.
	 * Conncted to DMIC4 Input on Tabla codec.
	 */
	{"DMIC4", NULL, "MIC BIAS3 External"},
	{"MIC BIAS3 External", NULL, "Digital Mic4"},

	/**
	 * Digital Mic5 (Top front Mic) on MTP.
	 * Conncted to DMIC6 Input on Tabla codec.
	 */
	{"DMIC6", NULL, "MIC BIAS4 External"},
	{"MIC BIAS4 External", NULL, "Digital Mic5"},

};

static const struct snd_soc_dapm_route apq8064_liquid_cdp_audio_map[] = {

	/************   Analog MIC Paths  ************/
	/**
	 * Analog mic7 (Front Top Mic) on Liquid.
	 * Used as Handset mic on CDP.
	 * Not there on MTP.
	 */
	{"AMIC1", NULL, "MIC BIAS1 External"},
	{"MIC BIAS1 External", NULL, "Analog mic7"},


	/************   Digital MIC Paths  ************/
	/**
	 * The digital Mic routes are setup considering
	 * Liquid as default device.
	 */

	/**
	 * Digital Mic1 (Front bottom left corner) on Liquid.
	 * Digital Mic2 (Front bottom right) on MTP.
	 * Digital Mic GM1 on CDP mainboard.
	 * Conncted to DMIC2 Input on Tabla codec.
	 */
	{"DMIC2", NULL, "MIC BIAS1 External"},
	{"MIC BIAS1 External", NULL, "Digital Mic1"},

	/**
	 * Digital Mic2 (Front left side) on Liquid.
	 * Digital Mic GM2 on CDP mainboard.
	 * Not there on MTP.
	 * Conncted to DMIC3 Input on Tabla codec.
	 */
	{"DMIC3", NULL, "MIC BIAS3 External"},
	{"MIC BIAS3 External", NULL, "Digital Mic2"},

	/**
	 * Digital Mic3. Front bottom left of middle on Liquid.
	 * Digital Mic5 (Top front Mic) on MTP.
	 * Digital Mic GM5 on CDP mainboard.
	 * Conncted to DMIC6 Input on Tabla codec.
	 */
	{"DMIC6", NULL, "MIC BIAS4 External"},
	{"MIC BIAS4 External", NULL, "Digital Mic3"},

	/**
	 * Digital Mic4. Back bottom on Liquid.
	 * Digital Mic GM3 on CDP mainboard.
	 * Top Front Mic on MTP.
	 * Conncted to DMIC5 Input on Tabla codec.
	 */
	{"DMIC5", NULL, "MIC BIAS4 External"},
	{"MIC BIAS4 External", NULL, "Digital Mic4"},

	/**
	 * Digital Mic5. Front bottom right of middle on Liquid.
	 * Digital Mic GM6 on CDP mainboard.
	 * Not there on MTP.
	 * Conncted to DMIC4 Input on Tabla codec.
	 */
	{"DMIC4", NULL, "MIC BIAS3 External"},
	{"MIC BIAS3 External", NULL, "Digital Mic5"},

	/* Digital Mic6 (Front bottom right corner) on Liquid.
	 * Digital Mic1 (Front bottom Left) on MTP.
	 * Digital Mic GM4 on CDP.
	 * Conncted to DMIC1 Input on Tabla codec.
	 */
	{"DMIC1", NULL, "MIC BIAS1 External"},
	{"MIC BIAS1 External", NULL, "Digital Mic6"},
};

static const char *spk_function[] = {"Off", "On"};
static const char *slim0_rx_ch_text[] = {"One", "Two"};
static const char *slim0_tx_ch_text[] = {"One", "Two", "Three", "Four"};
static const char *hdmi_rx_ch_text[] = {"Two", "Three", "Four", "Five",
	"Six", "Seven", "Eight"};
static const char * const hdmi_rate[] = {"Default", "Variable"};

static const struct soc_enum msm_enum[] = {
	SOC_ENUM_SINGLE_EXT(2, spk_function),
	SOC_ENUM_SINGLE_EXT(2, slim0_rx_ch_text),
	SOC_ENUM_SINGLE_EXT(4, slim0_tx_ch_text),
	SOC_ENUM_SINGLE_EXT(7, hdmi_rx_ch_text),
	SOC_ENUM_SINGLE_EXT(2, hdmi_rate),
};

static const char * const slim1_rate_text[] = {"8000", "16000", "48000"};
static const struct soc_enum msm_slim_1_rate_enum[] = {
	SOC_ENUM_SINGLE_EXT(3, slim1_rate_text),
};
static const char * const slim1_tx_ch_text[] = {"One", "Two"};
static const struct soc_enum msm_slim_1_tx_ch_enum[] = {
	SOC_ENUM_SINGLE_EXT(2, slim1_tx_ch_text),
};
static int msm_slim_0_rx_ch_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm_slim_0_rx_ch  = %d\n", __func__,
			msm_slim_0_rx_ch);
	ucontrol->value.integer.value[0] = msm_slim_0_rx_ch - 1;
	return 0;
}

static int msm_slim_0_rx_ch_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	msm_slim_0_rx_ch = ucontrol->value.integer.value[0] + 1;

	pr_debug("%s: msm_slim_0_rx_ch = %d\n", __func__,
		 msm_slim_0_rx_ch);
	return 1;
}

static int msm_slim_0_tx_ch_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm_slim_0_tx_ch  = %d\n", __func__,
			msm_slim_0_tx_ch);
	ucontrol->value.integer.value[0] = msm_slim_0_tx_ch - 1;
	return 0;
}

static int msm_slim_0_tx_ch_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	msm_slim_0_tx_ch = ucontrol->value.integer.value[0] + 1;

	pr_debug("%s: msm_slim_0_tx_ch = %d\n", __func__,
			msm_slim_0_tx_ch);
	return 1;
}

static int msm_slim_1_tx_ch_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm_slim_1_tx_ch  = %d\n", __func__,
		 msm_slim_1_tx_ch);

	ucontrol->value.integer.value[0] = msm_slim_1_tx_ch - 1;
	return 0;
}

static int msm_slim_1_tx_ch_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	msm_slim_1_tx_ch = ucontrol->value.integer.value[0] + 1;

	pr_debug("%s: msm_slim_1_tx_ch = %d\n", __func__,
		 msm_slim_1_tx_ch);

	return 1;
}

static int msm_slim_3_rx_ch_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm_slim_3_rx_ch  = %d\n", __func__,
			msm_slim_3_rx_ch);
	ucontrol->value.integer.value[0] = msm_slim_3_rx_ch - 1;
	return 0;
}

static int msm_slim_3_rx_ch_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	msm_slim_3_rx_ch = ucontrol->value.integer.value[0] + 1;

	pr_debug("%s: msm_slim_3_rx_ch = %d\n", __func__,
			msm_slim_3_rx_ch);
	return 1;
}

static int msm_slim_1_rate_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm_slim_1_rate  = %d", __func__,
		 msm_slim_1_rate);

	ucontrol->value.integer.value[0] = msm_slim_1_rate;
	return 0;
}

static int msm_slim_1_rate_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	switch (ucontrol->value.integer.value[0]) {
	case 8000:
		msm_slim_1_rate = SAMPLE_RATE_8KHZ;
		break;
	case 16000:
		msm_slim_1_rate = SAMPLE_RATE_16KHZ;
		break;
	case 48000:
		msm_slim_1_rate = SAMPLE_RATE_48KHZ;
		break;
	default:
		msm_slim_1_rate = SAMPLE_RATE_8KHZ;
		break;
	}
	pr_debug("%s: msm_slim_1_rate = %d\n", __func__,
		 msm_slim_1_rate);
	return 0;
}

static int msm_incall_rec_mode_get(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = rec_mode;
	return 0;
}

static int msm_incall_rec_mode_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{

	rec_mode = ucontrol->value.integer.value[0];
	pr_debug("%s: rec_mode:%d\n", __func__, rec_mode);

	return 0;
}

static int msm_hdmi_rx_ch_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	pr_debug("%s: msm_hdmi_rx_ch  = %d\n", __func__,
			msm_hdmi_rx_ch);
	ucontrol->value.integer.value[0] = msm_hdmi_rx_ch - 2;
	return 0;
}

static int msm_hdmi_rx_ch_put(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	msm_hdmi_rx_ch = ucontrol->value.integer.value[0] + 2;

	pr_debug("%s: msm_hdmi_rx_ch = %d\n", __func__,
		msm_hdmi_rx_ch);
	return 1;
}

static int msm_hdmi_rate_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	hdmi_rate_variable = ucontrol->value.integer.value[0];
	pr_debug("%s: hdmi_rate_variable = %d\n", __func__, hdmi_rate_variable);
	return 0;
}

static int msm_hdmi_rate_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = hdmi_rate_variable;
	return 0;
}

static const struct snd_kcontrol_new tabla_msm_controls[] = {
	SOC_ENUM_EXT("Speaker Function", msm_enum[0], msm_get_spk,
		msm_set_spk),
	SOC_ENUM_EXT("SLIM_0_RX Channels", msm_enum[1],
		msm_slim_0_rx_ch_get, msm_slim_0_rx_ch_put),
	SOC_ENUM_EXT("SLIM_0_TX Channels", msm_enum[2],
		msm_slim_0_tx_ch_get, msm_slim_0_tx_ch_put),
	SOC_ENUM_EXT("SLIM_1_TX Channels", msm_slim_1_tx_ch_enum[0],
		msm_slim_1_tx_ch_get, msm_slim_1_tx_ch_put),
	SOC_ENUM_EXT("SLIM_1 SampleRate", msm_slim_1_rate_enum[0],
		msm_slim_1_rate_get, msm_slim_1_rate_put),
	SOC_SINGLE_EXT("Incall Rec Mode", SND_SOC_NOPM, 0, 1, 0,
		msm_incall_rec_mode_get, msm_incall_rec_mode_put),
	SOC_ENUM_EXT("SLIM_3_RX Channels", msm_enum[1],
		msm_slim_3_rx_ch_get, msm_slim_3_rx_ch_put),
	SOC_ENUM_EXT("HDMI_RX Channels", msm_enum[3],
		msm_hdmi_rx_ch_get, msm_hdmi_rx_ch_put),
	SOC_ENUM_EXT("HDMI RX Rate", msm_enum[4],
		msm_hdmi_rate_get, msm_hdmi_rate_put),
};

static void *def_tabla_mbhc_cal(void)
{
	void *tabla_cal;
	struct tabla_mbhc_btn_detect_cfg *btn_cfg;
	u16 *btn_low, *btn_high;
	u8 *n_ready, *n_cic, *gain;

	tabla_cal = kzalloc(TABLA_MBHC_CAL_SIZE(TABLA_MBHC_DEF_BUTTONS,
						TABLA_MBHC_DEF_RLOADS),
			    GFP_KERNEL);
	if (!tabla_cal) {
		pr_err("%s: out of memory\n", __func__);
		return NULL;
	}

#define S(X, Y) ((TABLA_MBHC_CAL_GENERAL_PTR(tabla_cal)->X) = (Y))
	S(t_ldoh, 100);
	S(t_bg_fast_settle, 100);
	S(t_shutdown_plug_rem, 255);
	S(mbhc_nsa, 4);
	S(mbhc_navg, 4);
#undef S
#define S(X, Y) ((TABLA_MBHC_CAL_PLUG_DET_PTR(tabla_cal)->X) = (Y))
	S(mic_current, TABLA_PID_MIC_5_UA);
	S(hph_current, TABLA_PID_MIC_5_UA);
	S(t_mic_pid, 100);
	S(t_ins_complete, 250);
	S(t_ins_retry, 200);
#undef S
#define S(X, Y) ((TABLA_MBHC_CAL_PLUG_TYPE_PTR(tabla_cal)->X) = (Y))
	S(v_no_mic, 30);
	S(v_hs_max, 1550);
#undef S
#define S(X, Y) ((TABLA_MBHC_CAL_BTN_DET_PTR(tabla_cal)->X) = (Y))
	S(c[0], 62);
	S(c[1], 124);
	S(nc, 1);
	S(n_meas, 3);
	S(mbhc_nsc, 11);
	S(n_btn_meas, 1);
	S(n_btn_con, 2);
	S(num_btn, TABLA_MBHC_DEF_BUTTONS);
	S(v_btn_press_delta_sta, 100);
	S(v_btn_press_delta_cic, 50);
#undef S
	btn_cfg = TABLA_MBHC_CAL_BTN_DET_PTR(tabla_cal);
	btn_low = tabla_mbhc_cal_btn_det_mp(btn_cfg, TABLA_BTN_DET_V_BTN_LOW);
	btn_high = tabla_mbhc_cal_btn_det_mp(btn_cfg, TABLA_BTN_DET_V_BTN_HIGH);
	btn_low[0] = -50;
	btn_high[0] = 10;
	btn_low[1] = 11;
	btn_high[1] = 38;
	btn_low[2] = 39;
	btn_high[2] = 64;
	btn_low[3] = 65;
	btn_high[3] = 91;
	btn_low[4] = 92;
	btn_high[4] = 115;
	btn_low[5] = 116;
	btn_high[5] = 141;
	btn_low[6] = 142;
	btn_high[6] = 163;
	btn_low[7] = 164;
	btn_high[7] = 250;
	n_ready = tabla_mbhc_cal_btn_det_mp(btn_cfg, TABLA_BTN_DET_N_READY);
	n_ready[0] = 48;
	n_ready[1] = 38;
	n_cic = tabla_mbhc_cal_btn_det_mp(btn_cfg, TABLA_BTN_DET_N_CIC);
	n_cic[0] = 60;
	n_cic[1] = 47;
	gain = tabla_mbhc_cal_btn_det_mp(btn_cfg, TABLA_BTN_DET_GAIN);
	gain[0] = 11;
	gain[1] = 9;

	return tabla_cal;
}

static int msm_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;
	unsigned int rx_ch[SLIM_MAX_RX_PORTS], tx_ch[SLIM_MAX_TX_PORTS];
	unsigned int rx_ch_cnt = 0, tx_ch_cnt = 0;
	unsigned int num_tx_ch = 0;


	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {

		pr_debug("%s: rx_0_ch=%d\n", __func__, msm_slim_0_rx_ch);

		ret = snd_soc_dai_get_channel_map(codec_dai,
				&tx_ch_cnt, tx_ch, &rx_ch_cnt , rx_ch);
		if (ret < 0) {
			pr_err("%s: failed to get codec chan map\n", __func__);
			goto end;
		}

		ret = snd_soc_dai_set_channel_map(cpu_dai, 0, 0,
				msm_slim_0_rx_ch, rx_ch);
		if (ret < 0) {
			pr_err("%s: failed to set cpu chan map\n", __func__);
			goto end;
		}
		ret = snd_soc_dai_set_channel_map(codec_dai, 0, 0,
				msm_slim_0_rx_ch, rx_ch);
		if (ret < 0) {
			pr_err("%s: failed to set codec channel map\n",
								__func__);
			goto end;
		}
	} else {

		if (codec_dai->id  == 2)
			num_tx_ch =  msm_slim_0_tx_ch;
		else if (codec_dai->id == 5) {
			/* DAI 5 is used for external EC reference from codec.
			 * Since Rx is fed as reference for EC, the config of
			 * this DAI is based on that of the Rx path.
			 */
			num_tx_ch =  msm_slim_0_rx_ch;
		}

		pr_debug("%s: %s_tx_dai_id_%d_ch=%d\n", __func__,
			codec_dai->name, codec_dai->id, num_tx_ch);

		ret = snd_soc_dai_get_channel_map(codec_dai,
				&tx_ch_cnt, tx_ch, &rx_ch_cnt , rx_ch);
		if (ret < 0) {
			pr_err("%s: failed to get codec chan map\n", __func__);
			goto end;
		}

		ret = snd_soc_dai_set_channel_map(cpu_dai,
				num_tx_ch, tx_ch, 0 , 0);
		if (ret < 0) {
			pr_err("%s: failed to set cpu chan map\n", __func__);
			goto end;
		}
		ret = snd_soc_dai_set_channel_map(codec_dai,
				num_tx_ch, tx_ch, 0, 0);
		if (ret < 0) {
			pr_err("%s: failed to set codec channel map\n",
								__func__);
			goto end;
		}


	}
end:
	return ret;
}

static int msm_stubrx_init(struct snd_soc_pcm_runtime *rtd)
{
	rtd->pmdown_time = 0;

	return 0;
}

static int msm_slimbus_2_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;
	unsigned int rx_ch[SLIM_MAX_RX_PORTS], tx_ch[SLIM_MAX_TX_PORTS];
	unsigned int rx_ch_cnt = 0, tx_ch_cnt = 0;
	unsigned int num_tx_ch = 0;
	unsigned int num_rx_ch = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {

		num_rx_ch =  params_channels(params);

		pr_debug("%s: %s rx_dai_id = %d  num_ch = %d\n", __func__,
			codec_dai->name, codec_dai->id, num_rx_ch);

		ret = snd_soc_dai_get_channel_map(codec_dai,
				&tx_ch_cnt, tx_ch, &rx_ch_cnt , rx_ch);
		if (ret < 0) {
			pr_err("%s: failed to get codec chan map\n", __func__);
			goto end;
		}

		ret = snd_soc_dai_set_channel_map(cpu_dai, 0, 0,
				num_rx_ch, rx_ch);
		if (ret < 0) {
			pr_err("%s: failed to set cpu chan map\n", __func__);
			goto end;
		}
		ret = snd_soc_dai_set_channel_map(codec_dai, 0, 0,
				num_rx_ch, rx_ch);
		if (ret < 0) {
			pr_err("%s: failed to set codec channel map\n",
								__func__);
			goto end;
		}
	} else {

		num_tx_ch =  params_channels(params);

		pr_debug("%s: %s  tx_dai_id = %d  num_ch = %d\n", __func__,
			codec_dai->name, codec_dai->id, num_tx_ch);

		ret = snd_soc_dai_get_channel_map(codec_dai,
				&tx_ch_cnt, tx_ch, &rx_ch_cnt , rx_ch);
		if (ret < 0) {
			pr_err("%s: failed to get codec chan map\n", __func__);
			goto end;
		}

		ret = snd_soc_dai_set_channel_map(cpu_dai,
				num_tx_ch, tx_ch, 0 , 0);
		if (ret < 0) {
			pr_err("%s: failed to set cpu chan map\n", __func__);
			goto end;
		}
		ret = snd_soc_dai_set_channel_map(codec_dai,
				num_tx_ch, tx_ch, 0, 0);
		if (ret < 0) {
			pr_err("%s: failed to set codec channel map\n",
								__func__);
			goto end;
		}
	}
end:
	return ret;
}

static int msm_mi2s_hw_params(struct snd_pcm_substream *substream,
			struct snd_pcm_hw_params *params)
{
	int rate = params_rate(params);
	int bit_clk_set = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		bit_clk_set = 18432000/(rate * 2 * 24);
		clk_set_rate(mi2s_rx_bit_clk, bit_clk_set);
	}
	return 1;
}

static int msm_i2s_hw_params(struct snd_pcm_substream *substream,
			struct snd_pcm_hw_params *params)
{
	int rate = params_rate(params);
	int bit_clk_set = 0;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		bit_clk_set = 12288000/(rate * 2 * 16);
		pr_info("%s, bit clock is %d\n", __func__, bit_clk_set);
		clk_set_rate(pri_i2s_rx_bit_clk, bit_clk_set);
	}
	return 1;
}

static int msm_slimbus_1_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;
	unsigned int rx_ch = SLIM_1_RX_1, tx_ch[2] = {SLIM_1_TX_1, SLIM_1_TX_2};

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		pr_debug("%s: APQ BT/USB TX -> SLIMBUS_1_RX -> MDM TX shared ch %d\n",
			__func__, rx_ch);

		ret = snd_soc_dai_set_channel_map(cpu_dai, 0, 0, 1, &rx_ch);
		if (ret < 0) {
			pr_err("%s: Erorr %d setting SLIM_1 RX channel map\n",
				__func__, ret);

			goto end;
		}
	} else {
		pr_debug("%s: MDM RX ->SLIMBUS_1_TX ->APQ BT/USB Rx shared ch %d %d\n",
			  __func__, tx_ch[0], tx_ch[1]);

		ret = snd_soc_dai_set_channel_map(cpu_dai, msm_slim_1_tx_ch,
						  tx_ch, 0, 0);
		if (ret < 0) {
			pr_err("%s: Erorr %d setting SLIM_1 TX channel map\n",
				__func__, ret);

			goto end;
		}
	}

end:
	return ret;
}

static int msm_slimbus_3_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;
	unsigned int rx_ch[2] = {SLIM_3_RX_1, SLIM_3_RX_2};
	unsigned int tx_ch[2] = {SLIM_3_TX_1, SLIM_3_TX_2};

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		pr_debug("%s: slim_3_rx_ch %d, sch %d %d\n",
			 __func__, msm_slim_3_rx_ch,
				 rx_ch[0], rx_ch[1]);

		ret = snd_soc_dai_set_channel_map(cpu_dai, 0, 0,
				msm_slim_3_rx_ch, rx_ch);
		if (ret < 0) {
			pr_err("%s: Erorr %d setting SLIM_3 RX channel map\n",
				__func__, ret);

			goto end;
		}
	} else {
		pr_debug("%s: MDM RX -> SLIMBUS_3_TX -> APQ HDMI ch: %d, %d\n",
			__func__, tx_ch[0], tx_ch[1]);

		ret = snd_soc_dai_set_channel_map(cpu_dai, 2, tx_ch, 0, 0);
		if (ret < 0) {
			pr_err("%s: Erorr %d setting SLIM_3 TX channel map\n",
				__func__, ret);

			goto end;
		}
	}

end:
	return ret;
}

static int msm_slimbus_4_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;
	unsigned int rx_ch = SLIM_4_RX_1, tx_ch[2];

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		pr_debug("%s: APQ Incall Playback SLIMBUS_4_RX -> MDM TX shared ch %d\n",
			__func__, rx_ch);

		ret = snd_soc_dai_set_channel_map(cpu_dai, 0, 0, 1, &rx_ch);
		if (ret < 0) {
			pr_err("%s: Erorr %d setting SLIM_4 RX channel map\n",
				__func__, ret);

		}
	} else {
		if (rec_mode == INCALL_REC_STEREO) {
			tx_ch[0] = SLIM_4_TX_1;
			tx_ch[1] = SLIM_4_TX_2;
			ret = snd_soc_dai_set_channel_map(cpu_dai, 2,
							tx_ch, 0, 0);
		} else {
			tx_ch[0] = SLIM_4_TX_1;
			ret = snd_soc_dai_set_channel_map(cpu_dai, 1,
							tx_ch, 0, 0);
		}
		pr_debug("%s: Incall Record shared tx_ch[0]:%d, tx_ch[1]:%d\n",
			__func__, tx_ch[0], tx_ch[1]);

		if (ret < 0) {
			pr_err("%s: Erorr %d setting SLIM_4 TX channel map\n",
				__func__, ret);

		}
	}

	return ret;
}

static int msm_audrx_init(struct snd_soc_pcm_runtime *rtd)
{
	int err;
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;

	pr_debug("%s(), dev_name%s\n", __func__, dev_name(cpu_dai->dev));

	/*if (machine_is_msm_liquid()) {
		top_spk_pamp_gpio = (PM8921_GPIO_PM_TO_SYS(19));
		bottom_spk_pamp_gpio = (PM8921_GPIO_PM_TO_SYS(18));
	}*/

	snd_soc_dapm_new_controls(dapm, apq8064_dapm_widgets,
				ARRAY_SIZE(apq8064_dapm_widgets));

	snd_soc_dapm_add_routes(dapm, apq8064_common_audio_map,
		ARRAY_SIZE(apq8064_common_audio_map));

	if (machine_is_apq8064_mtp() || machine_is_apq8064_mako()) {
		snd_soc_dapm_add_routes(dapm, apq8064_mtp_audio_map,
			ARRAY_SIZE(apq8064_mtp_audio_map));
	} else  {
		snd_soc_dapm_add_routes(dapm, apq8064_liquid_cdp_audio_map,
			ARRAY_SIZE(apq8064_liquid_cdp_audio_map));
	}

	snd_soc_dapm_enable_pin(dapm, "Ext Spk Bottom Pos");
	snd_soc_dapm_enable_pin(dapm, "Ext Spk Bottom Neg");
	snd_soc_dapm_enable_pin(dapm, "Ext Spk Top Pos");
	snd_soc_dapm_enable_pin(dapm, "Ext Spk Top Neg");
	snd_soc_dapm_enable_pin(dapm, "Ext Hs Pos");
	snd_soc_dapm_enable_pin(dapm, "Ext Hs Neg");
	snd_soc_dapm_enable_pin(dapm, "Ext Rcv Pos");
	snd_soc_dapm_enable_pin(dapm, "Ext Rcv Neg");

	snd_soc_dapm_sync(dapm);

	err = snd_soc_jack_new(codec, "Headset Jack",
			       (SND_JACK_HEADSET |  SND_JACK_LINEOUT |
				SND_JACK_OC_HPHL |  SND_JACK_OC_HPHR |
				SND_JACK_UNSUPPORTED),
			       &hs_jack);
	if (err) {
		pr_err("failed to create new jack\n");
		return err;
	}

	err = snd_soc_jack_new(codec, "Button Jack",
			       TABLA_JACK_BUTTON_MASK, &button_jack);
	if (err) {
		pr_err("failed to create new jack\n");
		return err;
	}

	codec_clk = clk_get(cpu_dai->dev, "osr_clk");

	return err;
}

static int msm_slim_0_rx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
	SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
			SNDRV_PCM_HW_PARAM_CHANNELS);

	pr_debug("%s()\n", __func__);
	rate->min = rate->max = 48000;
	channels->min = channels->max = msm_slim_0_rx_ch;
	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
		SNDRV_PCM_FORMAT_S24_LE);

	return 0;
}

static int msm_slim_0_stub_rx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
	SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
			SNDRV_PCM_HW_PARAM_CHANNELS);

	pr_debug("%s()\n", __func__);
	rate->min = rate->max = 48000;
	channels->min = channels->max = msm_slim_0_rx_ch;
	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
		SNDRV_PCM_FORMAT_S16_LE);

	return 0;
}

static int msm_slim_0_tx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
	SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
			SNDRV_PCM_HW_PARAM_CHANNELS);

	pr_debug("%s()\n", __func__);
	rate->min = rate->max = 48000;
	channels->min = channels->max = msm_slim_0_tx_ch;

	return 0;
}

static int msm_slim_3_rx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
	SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
			SNDRV_PCM_HW_PARAM_CHANNELS);

	pr_debug("%s()\n", __func__);
	rate->min = rate->max = 48000;
	channels->min = channels->max = msm_slim_3_rx_ch;
	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
		SNDRV_PCM_FORMAT_S16_LE);

	return 0;
}

static int msm_slim_3_tx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
			SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
			SNDRV_PCM_HW_PARAM_CHANNELS);

	pr_debug("%s()\n", __func__);
	rate->min = rate->max = 48000;
	channels->min = channels->max = 2;

	return 0;
}

static int msm_slim_4_tx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
			SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
			SNDRV_PCM_HW_PARAM_CHANNELS);

	pr_debug("%s()\n", __func__);

	rate->min = rate->max = 48000;
	if (rec_mode == INCALL_REC_STEREO)
		channels->min = channels->max = 2;
	else
		channels->min = channels->max = 1;

	pr_debug("%s channels->min %u channels->max %u ()\n", __func__,
			channels->min, channels->max);
	return 0;
}

static int msm_slim_4_rx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
			SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
			SNDRV_PCM_HW_PARAM_CHANNELS);

	rate->min = rate->max = 48000;
	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
		SNDRV_PCM_FORMAT_S16_LE);

	pr_debug("%s channels->min %u channels->max %u ()\n", __func__,
			channels->min, channels->max);
	return 0;
}


static int msm_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
	SNDRV_PCM_HW_PARAM_RATE);

	pr_debug("%s()\n", __func__);
	rate->min = rate->max = 48000;
	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
		SNDRV_PCM_FORMAT_S16_LE);

	return 0;
}

static int msm_hdmi_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
					struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	pr_debug("%s channels->min %u channels->max %u ()\n", __func__,
			channels->min, channels->max);

	if (!hdmi_rate_variable)
		rate->min = rate->max = 48000;
	channels->min = channels->max = msm_hdmi_rx_ch;
	if (channels->max < 2)
		channels->min = channels->max = 2;
	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
		SNDRV_PCM_FORMAT_S16_LE);

	return 0;
}

static int msm_btsco_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
					struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
						      SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	rate->min = rate->max = msm_slim_1_rate;
	channels->min = channels->max = msm_btsco_ch;

	return 0;
}
static int msm_slim_1_rx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
					    struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
						      SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	rate->min = rate->max = 48000;
	channels->min = channels->max = 1;

	return 0;
}

static int msm_slim_1_tx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
					    struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	rate->min = rate->max = 48000;
	channels->min = channels->max = 1;

	return 0;
}

static int msm_mi2s_rx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
					struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);

	pr_debug("%s()\n", __func__);
	rate->min = rate->max = 48000;
#ifdef CONFIG_AMP_TFA9887L
	channels->min = channels->max = 2;
#else
	channels->min = channels->max = 1;
#endif
	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
		SNDRV_PCM_FORMAT_S24_LE);

	return 0;
}

static int msm_mi2s_tx_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
	SNDRV_PCM_HW_PARAM_RATE);

	pr_debug("%s()\n", __func__);
	rate->min = rate->max = 48000;
	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
		SNDRV_PCM_FORMAT_S24_LE);

	return 0;
}

static int msm_auxpcm_be_params_fixup(struct snd_soc_pcm_runtime *rtd,
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
static int msm_proxy_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
			struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
	SNDRV_PCM_HW_PARAM_RATE);

	pr_debug("%s()\n", __func__);
	rate->min = rate->max = 48000;
	param_set_mask(params, SNDRV_PCM_HW_PARAM_FORMAT,
		SNDRV_PCM_FORMAT_S16_LE);

	return 0;
}

static int msm_aux_pcm_get_gpios(void)
{
	pr_info("%s++\n ", __func__);
	pr_info("%s --\n", __func__);
	return 0;
}

static int configure_mi2s_rx_gpio(void)
{
	int ret;

	ret = gpio_request(GPIO_MI2S_RX_SCLK, "MI2S_RX_SCLK");
	if (ret) {
		pr_err("%s: Failed to request gpio %d\n",
				__func__, GPIO_MI2S_RX_SCLK);
		goto err;
	}
	ret = gpio_request(GPIO_MI2S_RX_WS, "MI2S_RX_WS");
	if (ret) {
		pr_err("%s: Failed to request gpio %d\n",
				__func__, GPIO_MI2S_RX_WS);
		goto err;
	}
	ret = gpio_request(GPIO_MI2S_RX_DOUT0, "MI2S_RX_DOUT0");
	if (ret) {
		pr_err("%s: Failed to request gpio %d\n",
				__func__, GPIO_MI2S_RX_DOUT0);
		goto err;
	}
	ret = gpio_request(GPIO_MI2S_RX_DOUT3, "MI2S_RX_DOUT3");
	if (ret) {
		pr_err("%s: Failed to request gpio %d\n",
				__func__, GPIO_MI2S_RX_DOUT3);
		goto err;
	}

	gpio_tlmm_config(audio_mi2s_active_table[0], GPIO_CFG_ENABLE);
	gpio_tlmm_config(audio_mi2s_active_table[1], GPIO_CFG_ENABLE);
	gpio_tlmm_config(audio_mi2s_active_table[2], GPIO_CFG_ENABLE);
	gpio_tlmm_config(audio_mi2s_active_table[3], GPIO_CFG_ENABLE);

err:
	return ret;
}

static int msm_aux_pcm_free_gpios(void)
{
	pr_info("%s ++\n", __func__);
	pr_info("%s --\n", __func__);
	return 0;
}

static int msm_mi2s_rx_free_gpios(void)
{
	int i;

	gpio_free(GPIO_MI2S_RX_DOUT0);
	gpio_free(GPIO_MI2S_RX_DOUT3);
	gpio_free(GPIO_MI2S_RX_SCLK);
	gpio_free(GPIO_MI2S_RX_WS);

	for (i = 0; i < ARRAY_SIZE(audio_mi2s_sleep_table); i++) {
		gpio_tlmm_config(audio_mi2s_sleep_table[i], GPIO_CFG_DISABLE);
	}

	return 0;
}

static int msm_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;

	pr_debug("%s(): dai_link_str_name = %s cpu_dai = %s codec_dai = %s\n",
		__func__, rtd->dai_link->stream_name,
		rtd->dai_link->cpu_dai_name,
		 rtd->dai_link->codec_dai_name);
	return 0;
}

static int msm_auxpcm_startup(struct snd_pcm_substream *substream)
{
	int ret = 0;

	pr_debug("%s(): substream = %s, auxpcm_rsc_ref counter = %d\n",
		__func__, substream->name, atomic_read(&auxpcm_rsc_ref));
	if (atomic_inc_return(&auxpcm_rsc_ref) == 1)
		ret = msm_aux_pcm_get_gpios();
	if (ret < 0) {
		pr_err("%s: Aux PCM GPIO request failed\n", __func__);
		return -EINVAL;
	}
	return 0;
}

static int msm_mi2s_startup(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		configure_mi2s_rx_gpio();

		mi2s_rx_osr_clk = clk_get(cpu_dai->dev, "osr_clk");
		if (mi2s_rx_osr_clk) {
			clk_set_rate(mi2s_rx_osr_clk, 18432000);
			clk_prepare_enable(mi2s_rx_osr_clk);
		}
		mi2s_rx_bit_clk = clk_get(cpu_dai->dev, "bit_clk");
		if (IS_ERR(mi2s_rx_bit_clk)) {
			pr_err("Failed to get mi2s_osr_clk\n");
			clk_disable_unprepare(mi2s_rx_osr_clk);
			clk_put(mi2s_rx_osr_clk);
			return PTR_ERR(mi2s_rx_bit_clk);
		}
		clk_set_rate(mi2s_rx_bit_clk, 8);
		ret = clk_prepare_enable(mi2s_rx_bit_clk);
		if (ret != 0) {
			pr_err("Unable to enable mi2s_rx_bit_clk\n");
			clk_put(mi2s_rx_bit_clk);
			clk_disable_unprepare(mi2s_rx_osr_clk);
			clk_put(mi2s_rx_osr_clk);
			return ret;
		}
		ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_CBS_CFS);
		if (ret < 0) {
			ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_CBS_CFS);
			if (ret < 0)
				pr_err("set format for codec dai failed\n");
		}
	}
#ifdef CONFIG_AMP_TFA9887
	set_tfa9887_spkamp(1, 0);
#endif
#ifdef CONFIG_AMP_TFA9887L
	set_tfa9887l_spkamp(1, 0);
#endif
	return ret;
}

static int msm_i2s_startup(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {

		pri_i2s_rx_osr_clk = clk_get(cpu_dai->dev, "osr_clk");
		if (IS_ERR(pri_i2s_rx_osr_clk)) {
			pr_err("Failed to get pri_i2s_rx_osr_clk\n");
			return PTR_ERR(pri_i2s_rx_osr_clk);
		}
		clk_set_rate(pri_i2s_rx_osr_clk, 12288000);
		clk_prepare_enable(pri_i2s_rx_osr_clk);
		pri_i2s_rx_bit_clk = clk_get(cpu_dai->dev, "bit_clk");
		if (IS_ERR(pri_i2s_rx_bit_clk)) {
			pr_err("Failed to get pri_i2s_rx_bit_clk\n");
			clk_disable_unprepare(pri_i2s_rx_osr_clk);
			clk_put(pri_i2s_rx_osr_clk);
			return PTR_ERR(pri_i2s_rx_bit_clk);
		}
		clk_set_rate(pri_i2s_rx_bit_clk, 8);
		ret = clk_prepare_enable(pri_i2s_rx_bit_clk);
		if (ret != 0) {
			pr_err("Unable to enable pri_i2s_rx_bit_clk\n");
			clk_put(pri_i2s_rx_bit_clk);
			clk_disable_unprepare(pri_i2s_rx_osr_clk);
			clk_put(pri_i2s_rx_osr_clk);
			return ret;
		}
		ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_CBS_CFS);
		if (ret < 0) {
			pr_info("%s: set format for cpu dai failed\n", __func__);
			ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_CBS_CFS);
			if (ret < 0)
				pr_err("set format for codec dai failed\n");
		}
	}
	return ret;
}

static int msm_slimbus_1_startup(struct snd_pcm_substream *substream)
{
	struct slim_controller *slim = slim_busnum_to_ctrl(1);

	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
		 substream->name, substream->stream);

	if (slim != NULL)
		pm_runtime_get_sync(slim->dev.parent);

	return 0;
}

static void msm_auxpcm_shutdown(struct snd_pcm_substream *substream)
{
	pr_debug("%s(): substream = %s, auxpcm_rsc_ref counter = %d\n",
		__func__, substream->name, atomic_read(&auxpcm_rsc_ref));
	if (atomic_dec_return(&auxpcm_rsc_ref) == 0)
		msm_aux_pcm_free_gpios();
}

static void msm_shutdown(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;

	pr_debug("%s(): dai_link_str_name = %s cpu_dai = %s codec_dai = %s\n",
		__func__, rtd->dai_link->stream_name,
		rtd->dai_link->cpu_dai_name, rtd->dai_link->codec_dai_name);
}

static void msm_mi2s_shutdown(struct snd_pcm_substream *substream)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
#ifdef CONFIG_AMP_TFA9887
		set_tfa9887_spkamp(0, 0);
#endif
#ifdef CONFIG_AMP_TFA9887L
		set_tfa9887l_spkamp(0, 0);
#endif
		if (mi2s_rx_bit_clk) {
			clk_disable_unprepare(mi2s_rx_bit_clk);
			clk_put(mi2s_rx_bit_clk);
			mi2s_rx_bit_clk = NULL;
		}
		if (mi2s_rx_osr_clk) {
			clk_disable_unprepare(mi2s_rx_osr_clk);
			clk_put(mi2s_rx_osr_clk);
			mi2s_rx_osr_clk = NULL;
		}
		msm_mi2s_rx_free_gpios();
	}
}

static void msm_i2s_shutdown(struct snd_pcm_substream *substream)
{
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (pri_i2s_rx_bit_clk) {
			clk_disable_unprepare(pri_i2s_rx_bit_clk);
			clk_put(pri_i2s_rx_bit_clk);
			pri_i2s_rx_bit_clk = NULL;
		}
		if (pri_i2s_rx_osr_clk) {
			clk_disable_unprepare(pri_i2s_rx_osr_clk);
			clk_put(pri_i2s_rx_osr_clk);
			pri_i2s_rx_osr_clk = NULL;
		}
	}
}

static void msm_slimbus_1_shutdown(struct snd_pcm_substream *substream)
{
	struct slim_controller *slim = slim_busnum_to_ctrl(1);

	pr_debug("%s(): substream = %s  stream = %d\n", __func__,
		 substream->name, substream->stream);

	if (slim != NULL) {
		pm_runtime_mark_last_busy(slim->dev.parent);
		pm_runtime_put(slim->dev.parent);
	}
}

static struct snd_soc_ops msm_be_ops = {
	.startup = msm_startup,
	.hw_params = msm_hw_params,
	.shutdown = msm_shutdown,
};

static struct snd_soc_ops msm_auxpcm_be_ops = {
	.startup = msm_auxpcm_startup,
	.shutdown = msm_auxpcm_shutdown,
};

static struct snd_soc_ops msm_mi2s_be_ops = {
	.startup = msm_mi2s_startup,
	.shutdown = msm_mi2s_shutdown,
	.hw_params = msm_mi2s_hw_params,
};

static struct snd_soc_ops msm_i2s_be_ops = {
	.startup = msm_i2s_startup,
	.shutdown = msm_i2s_shutdown,
	.hw_params = msm_i2s_hw_params,
};

static struct snd_soc_ops msm_slimbus_1_be_ops = {
	.startup = msm_slimbus_1_startup,
	.hw_params = msm_slimbus_1_hw_params,
	.shutdown = msm_slimbus_1_shutdown,
};

static struct snd_soc_ops msm_slimbus_3_be_ops = {
	.startup = msm_startup,
	.hw_params = msm_slimbus_3_hw_params,
	.shutdown = msm_shutdown,
};

static struct snd_soc_ops msm_slimbus_4_be_ops = {
	.startup = msm_startup,
	.hw_params = msm_slimbus_4_hw_params,
	.shutdown = msm_shutdown,
};
static struct snd_soc_ops msm_slimbus_2_be_ops = {
	.startup = msm_startup,
	.hw_params = msm_slimbus_2_hw_params,
	.shutdown = msm_shutdown,
};

/* Digital audio interface glue - connects codec <---> CPU */
static struct snd_soc_dai_link msm_dai[] = {
	/* FrontEnd DAI Links */
	{
		.name = "MSM8960 Media1",
		.stream_name = "MultiMedia1",
		.cpu_dai_name	= "MultiMedia1",
		.platform_name  = "msm-pcm-dsp",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA1
	},
	{
		.name = "MSM8960 Media2",
		.stream_name = "MultiMedia2",
		.cpu_dai_name	= "MultiMedia2",
		.platform_name  = "msm-multi-ch-pcm-dsp",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA2,
	},
	{
		.name = "Circuit-Switch Voice",
		.stream_name = "CS-Voice",
		.cpu_dai_name   = "CS-VOICE",
		.platform_name  = "msm-pcm-voice",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
		.be_id = MSM_FRONTEND_DAI_CS_VOICE,
	},
	{
		.name = "MSM VoIP",
		.stream_name = "VoIP",
		.cpu_dai_name	= "VoIP",
		.platform_name  = "msm-voip-dsp",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
		.be_id = MSM_FRONTEND_DAI_VOIP,
	},
	{
		.name = "MSM8960 LPA",
		.stream_name = "LPA",
		.cpu_dai_name	= "MultiMedia3",
		.platform_name  = "msm-pcm-lpa",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA3,
	},
	/* Hostless PMC purpose */
	{
		.name = "SLIMBUS_0 Hostless",
		.stream_name = "SLIMBUS_0 Hostless",
		.cpu_dai_name	= "SLIMBUS0_HOSTLESS",
		.platform_name  = "msm-pcm-hostless",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		/* .be_id = do not care */
	},
	{
		.name = "INT_FM Hostless",
		.stream_name = "INT_FM Hostless",
		.cpu_dai_name	= "INT_FM_HOSTLESS",
		.platform_name  = "msm-pcm-hostless",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
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
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
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
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA4,
	},
	{
		.name = "AUXPCM Hostless",
		.stream_name = "AUXPCM Hostless",
		.cpu_dai_name	= "AUXPCM_HOSTLESS",
		.platform_name  = "msm-pcm-hostless",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	/* HDMI Hostless */
	{
		.name = "HDMI_RX_HOSTLESS",
		.stream_name = "HDMI_RX_HOSTLESS",
		.cpu_dai_name = "HDMI_HOSTLESS",
		.platform_name = "msm-pcm-hostless",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
	},
	{
		.name = "Voice Stub",
		.stream_name = "Voice Stub",
		.cpu_dai_name = "VOICE_STUB",
		.platform_name = "msm-pcm-hostless",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	{
		.name = "VoLTE Stub",
		.stream_name = "VoLTE Stub",
		.cpu_dai_name   = "VOLTE_STUB",
		.platform_name  = "msm-pcm-hostless",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
			    SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
	},
	{
		.name = "MSM8960 LowLatency",
		.stream_name = "MultiMedia5",
		.cpu_dai_name   = "MultiMedia5",
		.platform_name  = "msm-lowlatency-pcm-dsp",
		.dynamic = 1,
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		.trigger = {SND_SOC_DPCM_TRIGGER_POST,
				SND_SOC_DPCM_TRIGGER_POST},
		.ignore_suspend = 1,
		/* this dainlink has playback support */
		.ignore_pmdown_time = 1,
		.be_id = MSM_FRONTEND_DAI_MULTIMEDIA5,
	},
	{
		.name = "Voice2 Stub",
		.stream_name = "Voice2 Stub",
		.cpu_dai_name = "VOICE2_STUB",
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
		.codec_name     = "tabla_codec",
		.codec_dai_name	= "tabla_rx1",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_SLIMBUS_0_RX,
		.init = &msm_audrx_init,
		.be_hw_params_fixup = msm_slim_0_rx_be_hw_params_fixup,
		.ops = &msm_be_ops,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
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
		.be_hw_params_fixup = msm_slim_0_tx_be_hw_params_fixup,
		.ops = &msm_be_ops,
	},
	/* Backend BT/FM DAI Links */
	{
		.name = LPASS_BE_INT_BT_SCO_RX,
		.stream_name = "Internal BT-SCO Playback",
		.cpu_dai_name = "msm-dai-q6.12288",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name	= "msm-stub-rx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_INT_BT_SCO_RX,
		.be_hw_params_fixup = msm_btsco_be_hw_params_fixup,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
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
		.be_hw_params_fixup = msm_btsco_be_hw_params_fixup,
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
		.be_hw_params_fixup = msm_slim_4_rx_be_hw_params_fixup,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
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
		.be_hw_params_fixup = msm_be_hw_params_fixup,
	},
	/* HDMI BACK END DAI Link */
	{
		.name = LPASS_BE_HDMI,
		.stream_name = "HDMI Playback",
		.cpu_dai_name = "msm-dai-q6-hdmi.8",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_HDMI_RX,
		.be_hw_params_fixup = msm_hdmi_be_hw_params_fixup,
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
		.be_hw_params_fixup = msm_be_hw_params_fixup,
		.ops = &msm_i2s_be_ops,
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
		.be_hw_params_fixup = msm_mi2s_rx_be_hw_params_fixup,
		.ops = &msm_mi2s_be_ops,
	},
	{
		.name = LPASS_BE_MI2S_TX,
		.stream_name = "MI2S Capture",
		.cpu_dai_name = "msm-dai-q6-mi2s",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.be_hw_params_fixup = msm_mi2s_tx_be_hw_params_fixup,
		.be_id = MSM_BACKEND_DAI_MI2S_TX,
		.ops = &msm_mi2s_be_ops,
	},
	/* Backend AFE DAI Links */
	{
		.name = LPASS_BE_AFE_PCM_RX,
		.stream_name = "AFE Playback",
		.cpu_dai_name = "msm-dai-q6.224",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_AFE_PCM_RX,
		.be_hw_params_fixup = msm_proxy_be_hw_params_fixup,
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
		.be_hw_params_fixup = msm_proxy_be_hw_params_fixup,
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
		.be_hw_params_fixup = msm_auxpcm_be_params_fixup,
		.ops = &msm_auxpcm_be_ops,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
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
		.be_hw_params_fixup = msm_auxpcm_be_params_fixup,
		.ops = &msm_auxpcm_be_ops,
	},
	{
		.name = LPASS_BE_STUB_RX,
		.stream_name = "Stub Playback",
		.cpu_dai_name = "msm-dai-stub",
		.platform_name = "msm-pcm-routing",
		.codec_name = "tabla_codec",
		.codec_dai_name = "tabla_rx2",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_EXTPROC_RX,
		.be_hw_params_fixup = msm_slim_0_stub_rx_be_hw_params_fixup,
		.init = &msm_stubrx_init,
		.ops = &msm_be_ops,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
	},
	{
		.name = LPASS_BE_STUB_TX,
		.stream_name = "Stub Capture",
		.cpu_dai_name = "msm-dai-stub",
		.platform_name = "msm-pcm-routing",
		.codec_name = "tabla_codec",
		.codec_dai_name = "tabla_tx1",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_EXTPROC_TX,
		.be_hw_params_fixup = msm_slim_0_tx_be_hw_params_fixup,
		.ops = &msm_be_ops,
	},
	{
		.name = LPASS_BE_SLIMBUS_1_RX,
		.stream_name = "Slimbus1 Playback",
		.cpu_dai_name = "msm-dai-q6.16386",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_SLIMBUS_1_RX,
		.be_hw_params_fixup = msm_slim_1_rx_be_hw_params_fixup,
		.ops = &msm_slimbus_1_be_ops,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */

	},
	{
		.name = LPASS_BE_SLIMBUS_1_TX,
		.stream_name = "Slimbus1 Capture",
		.cpu_dai_name = "msm-dai-q6.16387",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_SLIMBUS_1_TX,
		.be_hw_params_fixup =  msm_slim_1_tx_be_hw_params_fixup,
		.ops = &msm_slimbus_1_be_ops,
	},
	/* Ultrasound TX Back End DAI Link */
	{
		.name = "SLIMBUS_2 Hostless Capture",
		.stream_name = "SLIMBUS_2 Hostless Capture",
		.cpu_dai_name = "msm-dai-q6.16389",
		.platform_name = "msm-pcm-hostless",
		.codec_name = "tabla_codec",
		.codec_dai_name = "tabla_tx2",
		.ignore_suspend = 1,
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ops = &msm_slimbus_2_be_ops,
	},
	/* Ultrasound RX Back End DAI Link */
	{
		.name = "SLIMBUS_2 Hostless Playback",
		.stream_name = "SLIMBUS_2 Hostless Playback",
		.cpu_dai_name = "msm-dai-q6.16388",
		.platform_name = "msm-pcm-hostless",
		.codec_name = "tabla_codec",
		.codec_dai_name = "tabla_rx3",
		.ignore_suspend = 1,
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ops = &msm_slimbus_2_be_ops,
	},
	/* Incall Music Back End DAI Link */
	{
		.name = LPASS_BE_SLIMBUS_4_RX,
		.stream_name = "Slimbus4 Playback",
		.cpu_dai_name = "msm-dai-q6.16392",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_SLIMBUS_4_RX,
		.be_hw_params_fixup = msm_be_hw_params_fixup,
		.ops = &msm_slimbus_4_be_ops,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
	},
	/* Incall Record Back End DAI Link */
	{
		.name = LPASS_BE_SLIMBUS_4_TX,
		.stream_name = "Slimbus4 Capture",
		.cpu_dai_name = "msm-dai-q6.16393",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_SLIMBUS_4_TX,
		.be_hw_params_fixup = msm_slim_4_tx_be_hw_params_fixup,
		.ops = &msm_slimbus_4_be_ops,
	},
	{
		.name = LPASS_BE_STUB_1_TX,
		.stream_name = "Stub1 Capture",
		.cpu_dai_name = "msm-dai-stub",
		.platform_name = "msm-pcm-routing",
		.codec_name     = "tabla_codec",
		.codec_dai_name	= "tabla_tx3",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_EXTPROC_EC_TX,
		/* This BE is used for external EC reference from codec. Since
		 * Rx is fed as reference for EC, the config of this DAI is
		 * based on that of the Rx path.
		 */
		.be_hw_params_fixup = msm_slim_0_stub_rx_be_hw_params_fixup,
		.ops = &msm_be_ops,
	},
	{

		.name = LPASS_BE_SLIMBUS_3_RX,
		.stream_name = "Slimbus3 Playback",
		.cpu_dai_name = "msm-dai-q6.16390",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-rx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_SLIMBUS_3_RX,
		.be_hw_params_fixup = msm_slim_3_rx_be_hw_params_fixup,
		.ops = &msm_slimbus_3_be_ops,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
	},
	{
		.name = LPASS_BE_SLIMBUS_3_TX,
		.stream_name = "Slimbus3 Capture",
		.cpu_dai_name = "msm-dai-q6.16391",
		.platform_name = "msm-pcm-routing",
		.codec_name = "msm-stub-codec.1",
		.codec_dai_name = "msm-stub-tx",
		.no_pcm = 1,
		.be_id = MSM_BACKEND_DAI_SLIMBUS_3_TX,
		.be_hw_params_fixup = msm_slim_3_tx_be_hw_params_fixup,
		.ops = &msm_slimbus_3_be_ops,
	},
	{
		.name = "MI2S Hostless",
		.stream_name = "SLIMBUS_0 Hostless",
		.cpu_dai_name	= "SLIMBUS0_HOSTLESS",
		.platform_name  = "msm-pcm-hostless",
		.dynamic = 1,
		.trigger = {SND_SOC_DPCM_TRIGGER_POST, SND_SOC_DPCM_TRIGGER_POST},
		.no_host_mode = SND_SOC_DAI_LINK_NO_HOST,
		.ignore_suspend = 1,
		.ignore_pmdown_time = 1, /* this dainlink has playback support */
		.codec_dai_name = "snd-soc-dummy-dai",
		.codec_name = "snd-soc-dummy",
		/* .be_id = do not care */
	},
};

static struct snd_soc_card snd_soc_card_msm = {
	.name		= "apq8064-tabla-snd-card",
	.dai_link	= msm_dai,
	.num_links	= ARRAY_SIZE(msm_dai),
	.controls = tabla_msm_controls,
	.num_controls = ARRAY_SIZE(tabla_msm_controls),
};

static void mi2s_gpio_init(void)
{
	int i;
	static uint32_t audio_wdc_table[] = {
		GPIO_CFG(42, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	};

	for (i = 0; i < ARRAY_SIZE(audio_mi2s_active_table); i++) {
		gpio_tlmm_config(audio_mi2s_active_table[i], GPIO_CFG_ENABLE);
	}
	gpio_tlmm_config(audio_wdc_table[0], GPIO_CFG_ENABLE);
}

static void ext_hp_gpio_init(void)
{
	int ret;
	static struct pm_gpio param = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 1,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = PM_GPIO_VIN_S4,
		.out_strength   = PM_GPIO_STRENGTH_LOW,
		.function       = PM_GPIO_FUNC_NORMAL,
	};

	ret = gpio_request(AUD_HP_EN_GPIO, "hp_en");
	if (ret) {
		pr_err("%s: Failed to request gpio %d\n", __func__,
				AUD_HP_EN_GPIO);
		return;
	}
	ret = pm8xxx_gpio_config(AUD_HP_EN_GPIO, &param);
	if (ret)
		pr_err("%s: Failed to configure gpio %d\n", __func__,
				AUD_HP_EN_GPIO);
	else
		gpio_direction_output(AUD_HP_EN_GPIO, 0);
}

static int t6_get_hw_component(void)
{
	return HTC_AUDIO_RT5501;
}

extern unsigned int system_rev;
extern unsigned skuid;

static int t6_enable_digital_mic(void)
{
	int ret;

	if ((system_rev == XA) || (system_rev == XB)) {
		ret = 0;
	} else if ((system_rev == XC) || (system_rev == XD)) {
		if (((skuid & 0xFF) == 0x0B) ||
			((skuid & 0xFF) == 0x0D) ||
			((skuid & 0xFF) == 0x0C) ||
			((skuid & 0xFF) == 0x0E) ||
			((skuid & 0xFF) == 0x0F) ||
			((skuid & 0xFF) == 0x10) ||
			((skuid & 0xFF) == 0x11) ||
			((skuid & 0xFF) == 0x12) ||
			((skuid & 0xFF) == 0x13) ||
			((skuid & 0xFF) == 0x14) ||
			((skuid & 0xFF) == 0x15))
			ret = 1;
		else
			ret = 0;
	} else {
		if ((skuid & 0xFFF00) == 0x34C00)
			ret = 1;
		else if ((skuid & 0xFFF00) == 0x38900)
			ret = 2;
		else
			ret = 3;
	}
	pr_info("%s: skuid=0x%x, system_rev=%x return %d\n",
			__func__, skuid, system_rev, ret);
	return ret;
}

static void apq8064_set_q6_effect_mode(int mode)
{
	pr_info("%s: mode %d\n", __func__, mode);
	atomic_set(&q6_effect_mode, mode);
}

static struct acoustic_ops acoustic = {
	.enable_digital_mic = t6_enable_digital_mic,
	.get_hw_component = t6_get_hw_component,
	.set_q6_effect = apq8064_set_q6_effect_mode
};

static int t6_aud_speaker_vdd_enable(char *power_vreg_name, unsigned volt)
{
	struct regulator *aud_spk_amp_power;
	unsigned power_vreg_volt = volt;
	int ret = 0;

	pr_info("[AUD]%s, power_vreg_name=%s, volt=%u\n", __func__, power_vreg_name, volt);

	if (strlen(power_vreg_name) <= 0) {
		pr_err("[AUD]%s, vreg_name null is an invalid value\n", __func__);
		return -ENODEV;
	}

	if (power_vreg_volt < 0) {
		pr_err("[AUD]%s, power_vreg_volt is an invalid value\n", __func__);
		return -ENODEV;
	}

	aud_spk_amp_power = regulator_get(NULL, power_vreg_name);
	if (IS_ERR(aud_spk_amp_power)) {
		pr_err("[AUD]%s: Unable to get %s\n", __func__, power_vreg_name);
		return -ENODEV;
	}

	ret = regulator_enable(aud_spk_amp_power);
	if (ret < 0) {
		pr_err("[AUD]%s: Enable regulator %s failed\n", __func__, power_vreg_name);
		regulator_put(aud_spk_amp_power);
		aud_spk_amp_power = NULL;
		return -ENODEV;
	} else {
		pr_info("[AUD]%s: Enable regulator %s OK\n", __func__, power_vreg_name);
	}

	return 0;
}

static int t6_aud_vdd_enable(char *power_vreg_name, unsigned volt)
{
	struct regulator *aud_power;
	unsigned power_vreg_volt = volt;
	int ret = 0;

	pr_info("[AUD]%s, power_vreg_name=%s, volt=%u\n", __func__, power_vreg_name, volt);

	if (strlen(power_vreg_name) <= 0) {
		pr_err("[AUD]%s, vreg_name null is an invalid value\n", __func__);
		return -ENODEV;
	}

	if (power_vreg_volt < 0) {
		pr_err("[AUD]%s, power_vreg_volt is an invalid value\n", __func__);
		return -ENODEV;
	}

	aud_power = regulator_get(NULL, power_vreg_name);
	if (IS_ERR(aud_power)) {
		pr_err("[AUD]%s: Unable to get %s\n", __func__, power_vreg_name);
		return -ENODEV;
	}

	ret = regulator_set_voltage(aud_power, power_vreg_volt, power_vreg_volt);
	if (ret < 0) {
		pr_err("[AUD]%s: unable to set %s voltage to %d rc:%d\n",
				__func__, power_vreg_name, power_vreg_volt, ret);
		regulator_put(aud_power);
		aud_power = NULL;
		return -ENODEV;
	}

	ret = regulator_enable(aud_power);
	if (ret < 0) {
		pr_err("[AUD]%s: Enable regulator %s failed\n", __func__, power_vreg_name);
		regulator_put(aud_power);
		aud_power = NULL;
		return -ENODEV;
	} else {
		pr_info("[AUD]%s: Enable regulator %s OK\n", __func__, power_vreg_name);
	}

	return 0;
}

static void htc_audio_init(void)
{
	acoustic_register_ops(&acoustic);
	t6_aud_vdd_enable("aud_vdd_L9", 3000000);
	t6_aud_speaker_vdd_enable("tfa9887_vdd_LVS2", 1800000);
}

static struct platform_device *msm_snd_device;

static int __init msm_audio_init(void)
{
	int ret;
	u32	version = socinfo_get_platform_version();
	if (!soc_class_is_apq8064() ||
		(socinfo_get_id() == 130) ||
		(machine_is_apq8064_mtp() &&
		(SOCINFO_VERSION_MINOR(version) == 1))) {
		pr_info("%s: Not APQ8064 in SLIMBUS mode\n", __func__);
		return -ENODEV;
	}

	mbhc_cfg.calibration = def_tabla_mbhc_cal();
	if (!mbhc_cfg.calibration) {
		pr_err("Calibration data allocation failed\n");
		return -ENOMEM;
	}

	msm_snd_device = platform_device_alloc("soc-audio", 0);
	if (!msm_snd_device) {
		pr_err("Platform device allocation failed\n");
		kfree(mbhc_cfg.calibration);
		return -ENOMEM;
	}

	platform_set_drvdata(msm_snd_device, &snd_soc_card_msm);
	ret = platform_device_add(msm_snd_device);
	if (ret) {
		platform_device_put(msm_snd_device);
		kfree(mbhc_cfg.calibration);
		return ret;
	}

	htc_audio_init();
	ext_hp_gpio_init();
	mi2s_gpio_init();

	mutex_init(&cdc_mclk_mutex);
	atomic_set(&auxpcm_rsc_ref, 0);
	return ret;

}
module_init(msm_audio_init);

static void __exit msm_audio_exit(void)
{
	if (!soc_class_is_apq8064() || socinfo_get_id() == 130) {
		pr_err("%s: Not the right machine type\n", __func__);
		return ;
	}
	platform_device_unregister(msm_snd_device);
	if (mbhc_cfg.gpio)
		gpio_free(mbhc_cfg.gpio);
	kfree(mbhc_cfg.calibration);
	mutex_destroy(&cdc_mclk_mutex);
}
module_exit(msm_audio_exit);

MODULE_DESCRIPTION("ALSA SoC msm");
MODULE_LICENSE("GPL v2");
