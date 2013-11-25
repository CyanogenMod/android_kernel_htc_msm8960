/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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
#ifndef __ARCH_ARM_MACH_MSM_GPIO_COMMON_H
#define __ARCH_ARM_MACH_MSM_GPIO_COMMON_H

extern int msm_show_resume_irq_mask;

unsigned __msm_gpio_get_inout(unsigned gpio);
void __msm_gpio_set_inout(unsigned gpio, unsigned val);
void __msm_gpio_set_config_direction(unsigned gpio, int input, int val);
void __msm_gpio_set_polarity(unsigned gpio, unsigned val);
unsigned __msm_gpio_get_intr_status(unsigned gpio);
void __msm_gpio_set_intr_status(unsigned gpio);
unsigned __msm_gpio_get_intr_config(unsigned gpio);
unsigned __msm_gpio_get_intr_cfg_enable(unsigned gpio);
void __msm_gpio_set_intr_cfg_enable(unsigned gpio, unsigned val);
void __msm_gpio_set_intr_cfg_type(unsigned gpio, unsigned type);
void __gpio_tlmm_config(unsigned config);
void __msm_gpio_install_direct_irq(unsigned gpio, unsigned irq,
					unsigned int input_polarity);

#ifdef CONFIG_GPIO_MSM_V2
int _gpio_debug_direction_set(void *data, u64 val);
int _gpio_debug_direction_get(void *data, u64 *val);
int _gpio_debug_level_set(void *data, u64 val);
int _gpio_debug_level_get(void *data, u64 *val);
int _gpio_debug_drv_set(void *data, u64 val);
int _gpio_debug_drv_get(void *data, u64 *val);
int _gpio_debug_func_sel_set(void *data, u64 val);
int _gpio_debug_func_sel_get(void *data, u64 *val);
int _gpio_debug_pull_set(void *data, u64 val);
int _gpio_debug_pull_get(void *data, u64 *val);
int _gpio_debug_int_enable_get(void *data, u64 *val);
int _gpio_debug_int_owner_set(void *data, u64 val);
int _gpio_debug_int_owner_get(void *data, u64 *val);
int _gpio_debug_int_type_get(void *data, u64 *val);
int msm_dump_gpios(struct seq_file *m, int curr_len, char *gpio_buffer);
#else
int msm_dump_gpios(struct seq_file *m, int curr_len, char *gpio_buffer) {}
#endif 

#endif
