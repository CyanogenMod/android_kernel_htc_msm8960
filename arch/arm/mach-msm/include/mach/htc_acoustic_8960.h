/* include/asm/mach-msm/htc_acoustic_8960.h
 *
 * Copyright (C) 2012 HTC Corporation.
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
#ifndef _ARCH_ARM_MACH_MSM_HTC_ACOUSTIC_8960_H_
#define _ARCH_ARM_MACH_MSM_HTC_ACOUSTIC_8960_H_

#define HTC_AUDIO_TPA2051	0x01
#define HTC_AUDIO_TPA2026	0x02
#define HTC_AUDIO_TPA2028	0x04
#define HTC_AUDIO_A1028		0x08
#define HTC_AUDIO_TPA6185 (HTC_AUDIO_A1028 << 1)
#define HTC_AUDIO_RT5501 (HTC_AUDIO_TPA6185 << 1)
#define HTC_AUDIO_TFA9887 (HTC_AUDIO_RT5501 << 1)

struct acoustic_ops {
	void (*set_q6_effect)(int mode);
	int (*get_htc_revision)(void);
	int (*get_hw_component)(void);
	int (*enable_digital_mic)(void);
	int (*get_24b_audio)(void);
};

void acoustic_register_ops(struct acoustic_ops *ops);
struct acoustic_ops *acoustic_get_ops(void);
#endif

