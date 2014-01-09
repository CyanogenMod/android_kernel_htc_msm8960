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

#include <linux/platform_device.h>
#include <mach/htc_acoustic_8960.h>
#include <sound/pcm.h>
#include <sound/q6asm.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include "board-m7.h"
#include <mach/tpa6185.h>
#include <mach/rt5501.h>
#include "../sound/soc/msm/msm-pcm-routing.h"
#include "../sound/soc/msm/msm-compr-q6.h"
#define RCV_PAMP_GPIO 67

static atomic_t q6_effect_mode = ATOMIC_INIT(-1);
extern unsigned int system_rev;
extern unsigned int engineerid;
extern unsigned skuid;

static int m7_get_hw_component(void)
{
    int hw_com = 0;

    hw_com |= HTC_AUDIO_RT5501;
    return hw_com;
}

static int m7_enable_digital_mic(void)
{
	int ret;
	if((system_rev == XA)||(system_rev == XB)){
		ret = 0;
	}
	else if ((system_rev == XC)||(system_rev == XD)){
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
			((skuid & 0xFF) == 0x15)) {
			ret = 1;
		}
		else{
			ret = 0;
		}
	}
	else{
		if ((skuid & 0xFFF00) == 0x34C00) {
			ret = 1;
		}
		else if ((skuid & 0xFFF00) == 0x38900) {
			ret = 2;
		}
		else {
			ret = 3;
		}
	}
	printk(KERN_INFO "m7_enable_digital_mic:skuid=0x%x, system_rev=%x return %d\n", skuid, system_rev, ret);
	return ret;
}

void apq8064_set_q6_effect_mode(int mode)
{
	pr_info("%s: mode %d\n", __func__, mode);
	atomic_set(&q6_effect_mode, mode);
}

int apq8064_get_q6_effect_mode(void)
{
	int mode = atomic_read(&q6_effect_mode);
	pr_info("%s: mode %d\n", __func__, mode);
	return mode;
}

int apq8064_get_24b_audio(void)
{
	return 1;
}

static struct acoustic_ops acoustic = {
        .enable_digital_mic = m7_enable_digital_mic,
        .get_hw_component = m7_get_hw_component,
	.set_q6_effect = apq8064_set_q6_effect_mode
};

static struct q6asm_ops qops = {
	.get_q6_effect = apq8064_get_q6_effect_mode,
};

static struct msm_pcm_routing_ops rops = {
	.get_q6_effect = apq8064_get_q6_effect_mode,
};

static struct msm_compr_q6_ops cops = {
	.get_24b_audio = apq8064_get_24b_audio,
};


static int __init m7_audio_init(void)
{
        int ret = 0;

	
	gpio_request(RCV_PAMP_GPIO, "AUDIO_RCV_AMP");
	gpio_tlmm_config(GPIO_CFG(67, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_DISABLE);
	htc_register_q6asm_ops(&qops);
	htc_register_pcm_routing_ops(&rops);
	htc_register_compr_q6_ops(&cops);
	acoustic_register_ops(&acoustic);
	pr_info("%s", __func__);
	return ret;

}
late_initcall(m7_audio_init);

static void __exit m7_audio_exit(void)
{
	pr_info("%s", __func__);
}
module_exit(m7_audio_exit);

MODULE_DESCRIPTION("ALSA Platform Elite");
MODULE_LICENSE("GPL v2");
