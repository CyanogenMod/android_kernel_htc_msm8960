/* include/asm/mach-msm/htc_audiogpio_8930.h
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

struct request_gpio {
	int gpio_no;
	char *gpio_name;
};

struct htc_8930_gpio_pdata {
	void (*amp_speaker)(bool en);
	void (*amp_receiver)(bool en);
	void (*amp_headset)(bool en);
	void (*amp_hac)(bool en);
	struct request_gpio mi2s_gpio[4];
	struct request_gpio i2s_gpio[4];
	struct request_gpio aux_pcm_gpio[4];
};

