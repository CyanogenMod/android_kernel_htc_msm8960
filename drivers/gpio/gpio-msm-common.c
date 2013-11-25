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
 *
 */
#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/syscore_ops.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/err.h>

#include <asm/mach/irq.h>

#include <mach/msm_iomap.h>
#include <mach/gpiomux.h>
#include <mach/mpm.h>
#include "gpio-msm-common.h"

#ifdef CONFIG_GPIO_MSM_V2
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include "../../arch/arm/mach-msm/pm.h"
#endif 

#if defined(CONFIG_ARCH_MSM8960) || defined(CONFIG_ARCH_APQ8064)
#define GPIO_PM_USR_INTz       (104)
#endif

#ifdef CONFIG_GPIO_MSM_V3
enum msm_tlmm_register {
	SDC4_HDRV_PULL_CTL = 0x0, 
	SDC3_HDRV_PULL_CTL = 0x0, 
	SDC2_HDRV_PULL_CTL = 0x2048,
	SDC1_HDRV_PULL_CTL = 0x2044,
};
#else
enum msm_tlmm_register {
	SDC4_HDRV_PULL_CTL = 0x20a0,
	SDC3_HDRV_PULL_CTL = 0x20a4,
	SDC2_HDRV_PULL_CTL = 0x0, 
	SDC1_HDRV_PULL_CTL = 0x20a0,
};
#endif

struct tlmm_field_cfg {
	enum msm_tlmm_register reg;
	u8                     off;
};

static const struct tlmm_field_cfg tlmm_hdrv_cfgs[] = {
	{SDC4_HDRV_PULL_CTL, 6}, 
	{SDC4_HDRV_PULL_CTL, 3}, 
	{SDC4_HDRV_PULL_CTL, 0}, 
	{SDC3_HDRV_PULL_CTL, 6}, 
	{SDC3_HDRV_PULL_CTL, 3}, 
	{SDC3_HDRV_PULL_CTL, 0}, 
	{SDC2_HDRV_PULL_CTL, 6}, 
	{SDC2_HDRV_PULL_CTL, 3}, 
	{SDC2_HDRV_PULL_CTL, 0}, 
	{SDC1_HDRV_PULL_CTL, 6}, 
	{SDC1_HDRV_PULL_CTL, 3}, 
	{SDC1_HDRV_PULL_CTL, 0}, 
};

static const struct tlmm_field_cfg tlmm_pull_cfgs[] = {
	{SDC4_HDRV_PULL_CTL, 14}, 
	{SDC4_HDRV_PULL_CTL, 11}, 
	{SDC4_HDRV_PULL_CTL, 9},  
	{SDC3_HDRV_PULL_CTL, 14}, 
	{SDC3_HDRV_PULL_CTL, 11}, 
	{SDC3_HDRV_PULL_CTL, 9},  
	{SDC2_HDRV_PULL_CTL, 14}, 
	{SDC2_HDRV_PULL_CTL, 11}, 
	{SDC2_HDRV_PULL_CTL, 9},  
	{SDC1_HDRV_PULL_CTL, 13}, 
	{SDC1_HDRV_PULL_CTL, 11}, 
	{SDC1_HDRV_PULL_CTL, 9},  
};

struct irq_chip msm_gpio_irq_extn = {
	.irq_eoi	= NULL,
	.irq_mask	= NULL,
	.irq_unmask	= NULL,
	.irq_retrigger	= NULL,
	.irq_set_type	= NULL,
	.irq_set_wake	= NULL,
	.irq_disable	= NULL,
};

struct msm_gpio_dev {
	struct gpio_chip gpio_chip;
	DECLARE_BITMAP(enabled_irqs, NR_MSM_GPIOS);
	DECLARE_BITMAP(wake_irqs, NR_MSM_GPIOS);
	DECLARE_BITMAP(dual_edge_irqs, NR_MSM_GPIOS);
	struct irq_domain *domain;
};

static DEFINE_SPINLOCK(tlmm_lock);

static inline struct msm_gpio_dev *to_msm_gpio_dev(struct gpio_chip *chip)
{
	return container_of(chip, struct msm_gpio_dev, gpio_chip);
}

static int msm_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	int rc;
	rc = __msm_gpio_get_inout(offset);
	mb();
	return rc;
}

static void msm_gpio_set(struct gpio_chip *chip, unsigned offset, int val)
{
	__msm_gpio_set_inout(offset, val);
	mb();
}

static int msm_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&tlmm_lock, irq_flags);
	__msm_gpio_set_config_direction(offset, 1, 0);
	mb();
	spin_unlock_irqrestore(&tlmm_lock, irq_flags);
	return 0;
}

static int msm_gpio_direction_output(struct gpio_chip *chip,
				unsigned offset,
				int val)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&tlmm_lock, irq_flags);
	__msm_gpio_set_config_direction(offset, 0, val);
	mb();
	spin_unlock_irqrestore(&tlmm_lock, irq_flags);
	return 0;
}

#ifdef CONFIG_OF
static int msm_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	struct msm_gpio_dev *g_dev = to_msm_gpio_dev(chip);
	struct irq_domain *domain = g_dev->domain;
	return irq_linear_revmap(domain, offset);
}

static inline int msm_irq_to_gpio(struct gpio_chip *chip, unsigned irq)
{
	struct irq_data *irq_data = irq_get_irq_data(irq);
	return irq_data->hwirq;
}
#else
static int msm_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	return MSM_GPIO_TO_INT(offset - chip->base);
}

static inline int msm_irq_to_gpio(struct gpio_chip *chip, unsigned irq)
{
	return irq - MSM_GPIO_TO_INT(chip->base);
}
#endif

static int msm_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	return msm_gpiomux_get(chip->base + offset);
}

static void msm_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	msm_gpiomux_put(chip->base + offset);
}

static struct msm_gpio_dev msm_gpio = {
	.gpio_chip = {
		.label		  = "msmgpio",
		.base             = 0,
		.ngpio            = NR_MSM_GPIOS,
		.direction_input  = msm_gpio_direction_input,
		.direction_output = msm_gpio_direction_output,
		.get              = msm_gpio_get,
		.set              = msm_gpio_set,
		.to_irq           = msm_gpio_to_irq,
		.request          = msm_gpio_request,
		.free             = msm_gpio_free,
	},
};

static void switch_mpm_config(struct irq_data *d, unsigned val)
{
	
	if (!msm_gpio_irq_extn.irq_set_type)
		return;

	if (val)
		msm_gpio_irq_extn.irq_set_type(d, IRQF_TRIGGER_FALLING);
	else
		msm_gpio_irq_extn.irq_set_type(d, IRQF_TRIGGER_RISING);
}

static void msm_gpio_update_dual_edge_pos(struct irq_data *d, unsigned gpio)
{
	int loop_limit = 100;
	unsigned val, val2, intstat;

	do {
		val = __msm_gpio_get_inout(gpio);
		__msm_gpio_set_polarity(gpio, val);
		val2 = __msm_gpio_get_inout(gpio);
		intstat = __msm_gpio_get_intr_status(gpio);
		if (intstat || val == val2) {
			switch_mpm_config(d, val);
			return;
		}
	} while (loop_limit-- > 0);
	pr_err("%s: dual-edge irq failed to stabilize, %#08x != %#08x\n",
	       __func__, val, val2);
}

static void msm_gpio_irq_ack(struct irq_data *d)
{
	int gpio = msm_irq_to_gpio(&msm_gpio.gpio_chip, d->irq);

	__msm_gpio_set_intr_status(gpio);
	if (test_bit(gpio, msm_gpio.dual_edge_irqs))
		msm_gpio_update_dual_edge_pos(d, gpio);
	mb();
}

static void msm_gpio_irq_mask(struct irq_data *d)
{
	int gpio = msm_irq_to_gpio(&msm_gpio.gpio_chip, d->irq);
	unsigned long irq_flags;

	spin_lock_irqsave(&tlmm_lock, irq_flags);
	__msm_gpio_set_intr_cfg_enable(gpio, 0);
	__clear_bit(gpio, msm_gpio.enabled_irqs);
	mb();
	spin_unlock_irqrestore(&tlmm_lock, irq_flags);

	if (msm_gpio_irq_extn.irq_mask)
		msm_gpio_irq_extn.irq_mask(d);

}

static void msm_gpio_irq_unmask(struct irq_data *d)
{
	int gpio = msm_irq_to_gpio(&msm_gpio.gpio_chip, d->irq);
	unsigned long irq_flags;

	spin_lock_irqsave(&tlmm_lock, irq_flags);
	__set_bit(gpio, msm_gpio.enabled_irqs);
	if (!__msm_gpio_get_intr_cfg_enable(gpio)) {
		__msm_gpio_set_intr_status(gpio);
		__msm_gpio_set_intr_cfg_enable(gpio, 1);
		mb();
	}
	spin_unlock_irqrestore(&tlmm_lock, irq_flags);

	if (msm_gpio_irq_extn.irq_mask)
		msm_gpio_irq_extn.irq_unmask(d);
}

static void msm_gpio_irq_disable(struct irq_data *d)
{
	if (msm_gpio_irq_extn.irq_disable)
		msm_gpio_irq_extn.irq_disable(d);
}

static int msm_gpio_irq_set_type(struct irq_data *d, unsigned int flow_type)
{
	int gpio = msm_irq_to_gpio(&msm_gpio.gpio_chip, d->irq);
	unsigned long irq_flags;

	spin_lock_irqsave(&tlmm_lock, irq_flags);

	if (flow_type & IRQ_TYPE_EDGE_BOTH) {
		__irq_set_handler_locked(d->irq, handle_edge_irq);
		if ((flow_type & IRQ_TYPE_EDGE_BOTH) == IRQ_TYPE_EDGE_BOTH)
			__set_bit(gpio, msm_gpio.dual_edge_irqs);
		else
			__clear_bit(gpio, msm_gpio.dual_edge_irqs);
	} else {
		__irq_set_handler_locked(d->irq, handle_level_irq);
		__clear_bit(gpio, msm_gpio.dual_edge_irqs);
	}

	__msm_gpio_set_intr_cfg_type(gpio, flow_type);

	if ((flow_type & IRQ_TYPE_EDGE_BOTH) == IRQ_TYPE_EDGE_BOTH)
		msm_gpio_update_dual_edge_pos(d, gpio);

	mb();
	spin_unlock_irqrestore(&tlmm_lock, irq_flags);

	if (msm_gpio_irq_extn.irq_set_type)
		msm_gpio_irq_extn.irq_set_type(d, flow_type);

	return 0;
}

static irqreturn_t msm_summary_irq_handler(int irq, void *data)
{
	unsigned long i;
	struct irq_desc *desc = irq_to_desc(irq);
	struct irq_chip *chip = irq_desc_get_chip(desc);

	chained_irq_enter(chip, desc);

	for (i = find_first_bit(msm_gpio.enabled_irqs, NR_MSM_GPIOS);
	     i < NR_MSM_GPIOS;
	     i = find_next_bit(msm_gpio.enabled_irqs, NR_MSM_GPIOS, i + 1)) {
		if (__msm_gpio_get_intr_status(i))
			generic_handle_irq(msm_gpio_to_irq(&msm_gpio.gpio_chip,
							   i));
	}

	chained_irq_exit(chip, desc);
	return IRQ_HANDLED;
}

static int msm_gpio_irq_set_wake(struct irq_data *d, unsigned int on)
{
	int gpio = msm_irq_to_gpio(&msm_gpio.gpio_chip, d->irq);

	if (on) {
		if (bitmap_empty(msm_gpio.wake_irqs, NR_MSM_GPIOS))
			irq_set_irq_wake(TLMM_MSM_SUMMARY_IRQ, 1);
		set_bit(gpio, msm_gpio.wake_irqs);
	} else {
		clear_bit(gpio, msm_gpio.wake_irqs);
		if (bitmap_empty(msm_gpio.wake_irqs, NR_MSM_GPIOS))
			irq_set_irq_wake(TLMM_MSM_SUMMARY_IRQ, 0);
	}

	if (msm_gpio_irq_extn.irq_set_wake)
		msm_gpio_irq_extn.irq_set_wake(d, on);

	return 0;
}

static struct irq_chip msm_gpio_irq_chip = {
	.name		= "msmgpio",
	.irq_mask	= msm_gpio_irq_mask,
	.irq_unmask	= msm_gpio_irq_unmask,
	.irq_ack	= msm_gpio_irq_ack,
	.irq_set_type	= msm_gpio_irq_set_type,
	.irq_set_wake	= msm_gpio_irq_set_wake,
	.irq_disable	= msm_gpio_irq_disable,
};

static struct lock_class_key msm_gpio_lock_class;

static int __devinit msm_gpio_probe(void)
{
	int ret;
#ifndef CONFIG_OF
	int irq, i;
#endif

	spin_lock_init(&tlmm_lock);
	bitmap_zero(msm_gpio.enabled_irqs, NR_MSM_GPIOS);
	bitmap_zero(msm_gpio.wake_irqs, NR_MSM_GPIOS);
	bitmap_zero(msm_gpio.dual_edge_irqs, NR_MSM_GPIOS);
	ret = gpiochip_add(&msm_gpio.gpio_chip);
	if (ret < 0)
		return ret;

#ifndef CONFIG_OF
	for (i = 0; i < msm_gpio.gpio_chip.ngpio; ++i) {
		irq = msm_gpio_to_irq(&msm_gpio.gpio_chip, i);
		irq_set_lockdep_class(irq, &msm_gpio_lock_class);
		irq_set_chip_and_handler(irq, &msm_gpio_irq_chip,
					 handle_level_irq);
		set_irq_flags(irq, IRQF_VALID);
	}
#endif
	ret = request_irq(TLMM_MSM_SUMMARY_IRQ, msm_summary_irq_handler,
			IRQF_TRIGGER_HIGH, "msmgpio", NULL);
	if (ret) {
		pr_err("Request_irq failed for TLMM_MSM_SUMMARY_IRQ - %d\n",
				ret);
		return ret;
	}
	return 0;
}

static int __devexit msm_gpio_remove(void)
{
	int ret = gpiochip_remove(&msm_gpio.gpio_chip);

	if (ret < 0)
		return ret;

	irq_set_handler(TLMM_MSM_SUMMARY_IRQ, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int msm_gpio_suspend(void)
{
	unsigned long irq_flags;
	unsigned long i;

	spin_lock_irqsave(&tlmm_lock, irq_flags);
	for_each_set_bit(i, msm_gpio.enabled_irqs, NR_MSM_GPIOS)
		__msm_gpio_set_intr_cfg_enable(i, 0);

	for_each_set_bit(i, msm_gpio.wake_irqs, NR_MSM_GPIOS)
		__msm_gpio_set_intr_cfg_enable(i, 1);
	mb();
	spin_unlock_irqrestore(&tlmm_lock, irq_flags);
	return 0;
}

void msm_gpio_show_resume_irq(void)
{
	unsigned long irq_flags;
	int i, irq, intstat;

	if (!msm_show_resume_irq_mask)
		return;

	spin_lock_irqsave(&tlmm_lock, irq_flags);
	for_each_set_bit(i, msm_gpio.wake_irqs, NR_MSM_GPIOS) {
		intstat = __msm_gpio_get_intr_status(i);
		if (intstat) {
			irq = msm_gpio_to_irq(&msm_gpio.gpio_chip, i);
			pr_warning("%s: %d triggered\n", __func__, irq);
#if defined(CONFIG_ARCH_MSM8960) || defined(CONFIG_ARCH_APQ8064)
			if(irq != GPIO_PM_USR_INTz + NR_MSM_IRQS) {
#endif
				pr_warning("[K][WAKEUP] Resume caused by msmgpio-%d\n", irq - NR_MSM_IRQS);
#if defined(CONFIG_ARCH_MSM8960) || defined(CONFIG_ARCH_APQ8064)
			}
#endif
		}
	}
	spin_unlock_irqrestore(&tlmm_lock, irq_flags);
}

static void msm_gpio_resume(void)
{
	unsigned long irq_flags;
	unsigned long i;

	msm_gpio_show_resume_irq();

	spin_lock_irqsave(&tlmm_lock, irq_flags);
	for_each_set_bit(i, msm_gpio.wake_irqs, NR_MSM_GPIOS)
		__msm_gpio_set_intr_cfg_enable(i, 0);

	for_each_set_bit(i, msm_gpio.enabled_irqs, NR_MSM_GPIOS)
		__msm_gpio_set_intr_cfg_enable(i, 1);
	mb();
	spin_unlock_irqrestore(&tlmm_lock, irq_flags);
}
#else
#define msm_gpio_suspend NULL
#define msm_gpio_resume NULL
#endif

static struct syscore_ops msm_gpio_syscore_ops = {
	.suspend = msm_gpio_suspend,
	.resume = msm_gpio_resume,
};

#ifdef CONFIG_GPIO_MSM_V2
#define GPIO_FUNC_SEL_BIT 2
#define GPIO_DRV_BIT 6

#if defined(CONFIG_DEBUG_FS)

static int gpio_debug_direction_set(void *data, u64 val)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&tlmm_lock, irq_flags);
	_gpio_debug_direction_set(data, val);
	spin_unlock_irqrestore(&tlmm_lock, irq_flags);
	return 0;
}

static int gpio_debug_direction_get(void *data, u64 *val)
{
	return _gpio_debug_direction_get(data, val);
}

DEFINE_SIMPLE_ATTRIBUTE(gpio_direction_fops, gpio_debug_direction_get,
			gpio_debug_direction_set, "%llu\n");

static int gpio_debug_level_set(void *data, u64 val)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&tlmm_lock, irq_flags);
	_gpio_debug_level_set(data, val);
	spin_unlock_irqrestore(&tlmm_lock, irq_flags);
	return 0;
}

static int gpio_debug_level_get(void *data, u64 *val)
{
	return _gpio_debug_level_get(data, val);
}

DEFINE_SIMPLE_ATTRIBUTE(gpio_level_fops, gpio_debug_level_get,
			gpio_debug_level_set, "%llu\n");

static int gpio_debug_drv_set(void *data, u64 val)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&tlmm_lock, irq_flags);
	_gpio_debug_drv_set(data, val);
	spin_unlock_irqrestore(&tlmm_lock, irq_flags);
	return 0;
}

static int gpio_debug_drv_get(void *data, u64 *val)
{
	return _gpio_debug_drv_get(data, val);
}

DEFINE_SIMPLE_ATTRIBUTE(gpio_drv_fops, gpio_debug_drv_get,
			gpio_debug_drv_set, "%llu\n");

static int gpio_debug_func_sel_set(void *data, u64 val)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&tlmm_lock, irq_flags);
	_gpio_debug_func_sel_set(data, val);
	spin_unlock_irqrestore(&tlmm_lock, irq_flags);
	return 0;
}

static int gpio_debug_func_sel_get(void *data, u64 *val)
{
	return _gpio_debug_func_sel_get(data, val);
}

DEFINE_SIMPLE_ATTRIBUTE(gpio_func_sel_fops, gpio_debug_func_sel_get,
			gpio_debug_func_sel_set, "%llu\n");

static int gpio_debug_pull_set(void *data, u64 val)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&tlmm_lock, irq_flags);
	_gpio_debug_pull_set(data, val);
	spin_unlock_irqrestore(&tlmm_lock, irq_flags);
	return 0;
}

static int gpio_debug_pull_get(void *data, u64 *val)
{
	return _gpio_debug_pull_get(data, val);
}

DEFINE_SIMPLE_ATTRIBUTE(gpio_pull_fops, gpio_debug_pull_get,
			gpio_debug_pull_set, "%llu\n");

static int gpio_debug_int_enable_get(void *data, u64 *val)
{
	return _gpio_debug_int_enable_get(data, val);
}

DEFINE_SIMPLE_ATTRIBUTE(gpio_int_enable_fops, gpio_debug_int_enable_get,
			NULL, "%llu\n");

static int gpio_debug_int_owner_set(void *data, u64 val)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&tlmm_lock, irq_flags);
	_gpio_debug_int_owner_set(data, val);
	spin_unlock_irqrestore(&tlmm_lock, irq_flags);
	return 0;
}

static int gpio_debug_int_owner_get(void *data, u64 *val)
{
	return _gpio_debug_int_owner_get(data, val);
}

DEFINE_SIMPLE_ATTRIBUTE(gpio_int_owner_fops, gpio_debug_int_owner_get,
			gpio_debug_int_owner_set, "%llu\n");

static int gpio_debug_int_type_get(void *data, u64 *val)
{
	return _gpio_debug_int_type_get(data, val);
}

DEFINE_SIMPLE_ATTRIBUTE(gpio_int_type_fops, gpio_debug_int_type_get,
			NULL, "%llu\n");

static int list_gpios_show(struct seq_file *m, void *unused)
{
	msm_dump_gpios(m, 0, NULL);
	pm8xxx_dump_gpios(m, 0, NULL);
	pm8xxx_dump_mpp(m, 0, NULL);
	return 0;
}

static int list_sleep_gpios_show(struct seq_file *m, void *unused)
{
	print_gpio_buffer(m);
	return 0;
}

static int list_gpios_open(struct inode *inode, struct file *file)
{
	return single_open(file, list_gpios_show, inode->i_private);
}

static int list_sleep_gpios_open(struct inode *inode, struct file *file)
{
	return single_open(file, list_sleep_gpios_show, inode->i_private);
}

static int list_sleep_gpios_release(struct inode *inode, struct file *file)
{
	free_gpio_buffer();
	return single_release(inode, file);
}

static const struct file_operations list_gpios_fops = {
	.open		= list_gpios_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static const struct file_operations list_sleep_gpios_fops = {
	.open		= list_sleep_gpios_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= list_sleep_gpios_release,
};

static struct dentry *debugfs_base;
#define DEBUG_MAX_FNAME    8

static int gpio_add_status(int id)
{
	unsigned int *index_p;
	struct dentry *gpio_dir;
	char name[DEBUG_MAX_FNAME];

	index_p = kzalloc(sizeof(*index_p), GFP_KERNEL);
	if (!index_p)
		return -ENOMEM;
	*index_p = id;
	snprintf(name, DEBUG_MAX_FNAME-1, "%d", *index_p);

	gpio_dir = debugfs_create_dir(name, debugfs_base);
	if (!gpio_dir)
		return -ENOMEM;

	if (!debugfs_create_file("direction", S_IRUGO | S_IWUSR, gpio_dir,
				index_p, &gpio_direction_fops))
		goto error;

	if (!debugfs_create_file("level", S_IRUGO | S_IWUSR, gpio_dir,
				index_p, &gpio_level_fops))
		goto error;

	if (!debugfs_create_file("drv_strength", S_IRUGO | S_IWUSR, gpio_dir,
				index_p, &gpio_drv_fops))
		goto error;

	if (!debugfs_create_file("func_sel", S_IRUGO | S_IWUSR, gpio_dir,
				index_p, &gpio_func_sel_fops))
		goto error;

	if (!debugfs_create_file("pull", S_IRUGO | S_IWUSR, gpio_dir,
				index_p, &gpio_pull_fops))
		goto error;

	if (!debugfs_create_file("int_enable", S_IRUGO, gpio_dir,
				index_p, &gpio_int_enable_fops))
		goto error;

	if (!debugfs_create_file("int_owner", S_IRUGO | S_IWUSR, gpio_dir,
				index_p, &gpio_int_owner_fops))
		goto error;

	if (!debugfs_create_file("int_type", S_IRUGO, gpio_dir,
				index_p, &gpio_int_type_fops))
		goto error;

	return 0;
error:
	debugfs_remove_recursive(gpio_dir);
	return -ENOMEM;
}

int __init gpio_status_debug_init(void)
{
	int i;
	int err = 0;

	debugfs_base = debugfs_create_dir("htc_gpio", NULL);
	if (!debugfs_base)
		return -ENOMEM;

	if (!debugfs_create_file("list_gpios", S_IRUGO, debugfs_base,
				&msm_gpio.gpio_chip, &list_gpios_fops))
		return -ENOMEM;

	if (!debugfs_create_file("list_sleep_gpios", S_IRUGO, debugfs_base,
				&msm_gpio.gpio_chip, &list_sleep_gpios_fops))
		return -ENOMEM;

	for (i = msm_gpio.gpio_chip.base; i < msm_gpio.gpio_chip.ngpio; i++)
		err = gpio_add_status(i);

	return err;
}
#endif 

int msm_dump_gpios(struct seq_file *m, int curr_len, char *gpio_buffer)
{
	unsigned int i, len;
	char list_gpio[100];
	char *title_msg = "------------ MSM GPIO -------------";
	u64 func_sel, dir, pull, drv, value, int_en, int_owner;

	if (m) {
		seq_printf(m, "%s\n", title_msg);
	} else {
		pr_info("[K] %s\n", title_msg);
		curr_len += sprintf(gpio_buffer + curr_len,
		"%s\n", title_msg);
	}

	for (i = msm_gpio.gpio_chip.base; i < msm_gpio.gpio_chip.ngpio; i++) {
		memset(list_gpio, 0 , sizeof(list_gpio));
		len = 0;

		len += sprintf(list_gpio + len, "GPIO[%3d]: ", i);

		_gpio_debug_func_sel_get((void *)&i, &func_sel);
		len += sprintf(list_gpio + len, "[FS]0x%x, ", (unsigned int)func_sel);

		_gpio_debug_direction_get((void *)&i, &dir);
		_gpio_debug_level_get((void *)&i, &value);
		if (dir)
				len += sprintf(list_gpio + len, "[DIR] IN, [VAL]%s, ", value ? "HIGH" : " LOW");
		else
				len += sprintf(list_gpio + len, "[DIR]OUT, [VAL]%s, ", value ? "HIGH" : " LOW");

		_gpio_debug_pull_get((void *)&i, &pull);
		switch (pull) {
		case 0x0:
			len += sprintf(list_gpio + len, "[PULL]NO, ");
			break;
		case 0x1:
			len += sprintf(list_gpio + len, "[PULL]PD, ");
			break;
		case 0x2:
			len += sprintf(list_gpio + len, "[PULL]KP, ");
			break;
		case 0x3:
			len += sprintf(list_gpio + len, "[PULL]PU, ");
			break;
		default:
			break;
		}

		_gpio_debug_drv_get((void *)&i, &drv);
		len += sprintf(list_gpio + len, "[DRV]%2dmA, ", 2*((int)drv+1));

		if (dir != 0) {
			_gpio_debug_int_enable_get((void *)&i, &int_en);
			len += sprintf(list_gpio + len, "[INT]%s, ", int_en ? "YES" : " NO");
			if (int_en) {
				_gpio_debug_int_owner_get((void *)&i, &int_owner);
				switch (int_owner) {
				case 0x0:
					len += sprintf(list_gpio + len, "MSS_PROC, ");
					break;
				case 0x1:
					len += sprintf(list_gpio + len, "SPS_PROC, ");
					break;
				case 0x2:
					len += sprintf(list_gpio + len, " LPA_DSP, ");
					break;
				case 0x3:
					len += sprintf(list_gpio + len, "RPM_PROC, ");
					break;
				case 0x4:
					len += sprintf(list_gpio + len, " SC_PROC, ");
					break;
				case 0x5:
					len += sprintf(list_gpio + len, "RESERVED, ");
					break;
				case 0x6:
					len += sprintf(list_gpio + len, "RESERVED, ");
					break;
				case 0x7:
					len += sprintf(list_gpio + len, "    NONE, ");
					break;
				default:
					break;
				}
			}
		}

		list_gpio[99] = '\0';
		if (m) {
			seq_printf(m, "%s\n", list_gpio);
		} else {
			pr_info("[K] %s\n", list_gpio);
			curr_len += sprintf(gpio_buffer +
			curr_len, "%s\n", list_gpio);
		}
	}

	return curr_len;
}
EXPORT_SYMBOL(msm_dump_gpios);
#endif 

static int __init msm_gpio_init(void)
{
	msm_gpio_probe();
	register_syscore_ops(&msm_gpio_syscore_ops);
	gpio_status_debug_init();
	return 0;
}

static void __exit msm_gpio_exit(void)
{
	unregister_syscore_ops(&msm_gpio_syscore_ops);
	msm_gpio_remove();
}

postcore_initcall(msm_gpio_init);
module_exit(msm_gpio_exit);

static void msm_tlmm_set_field(const struct tlmm_field_cfg *configs,
			       unsigned id, unsigned width, unsigned val)
{
	unsigned long irqflags;
	u32 mask = (1 << width) - 1;
	u32 __iomem *reg = MSM_TLMM_BASE + configs[id].reg;
	u32 reg_val;

	spin_lock_irqsave(&tlmm_lock, irqflags);
	reg_val = __raw_readl(reg);
	reg_val &= ~(mask << configs[id].off);
	reg_val |= (val & mask) << configs[id].off;
	__raw_writel(reg_val, reg);
	mb();
	spin_unlock_irqrestore(&tlmm_lock, irqflags);
}

void msm_tlmm_set_hdrive(enum msm_tlmm_hdrive_tgt tgt, int drv_str)
{
	msm_tlmm_set_field(tlmm_hdrv_cfgs, tgt, 3, drv_str);
}
EXPORT_SYMBOL(msm_tlmm_set_hdrive);

void msm_tlmm_set_pull(enum msm_tlmm_pull_tgt tgt, int pull)
{
	msm_tlmm_set_field(tlmm_pull_cfgs, tgt, 2, pull);
}
EXPORT_SYMBOL(msm_tlmm_set_pull);

static u32 msm_tlmm_get_field(const struct tlmm_field_cfg *configs,
			       unsigned id, unsigned width)
{
	unsigned long irqflags;
	u32 mask = (1 << width) - 1;
	u32 __iomem *reg = MSM_TLMM_BASE + configs[id].reg;
	u32 reg_val;

	spin_lock_irqsave(&tlmm_lock, irqflags);
	reg_val = __raw_readl(reg);
	reg_val = (reg_val >> configs[id].off) & mask;
	spin_unlock_irqrestore(&tlmm_lock, irqflags);

	return reg_val;
}

u32 msm_tlmm_get_hdrive(enum msm_tlmm_hdrive_tgt tgt)
{
	return msm_tlmm_get_field(tlmm_hdrv_cfgs, tgt, 3);
}
EXPORT_SYMBOL(msm_tlmm_get_hdrive);

u32 msm_tlmm_get_pull(enum msm_tlmm_pull_tgt tgt)
{
	return msm_tlmm_get_field(tlmm_pull_cfgs, tgt, 2);
}
EXPORT_SYMBOL(msm_tlmm_get_pull);

int gpio_tlmm_config(unsigned config, unsigned disable)
{
	unsigned gpio = GPIO_PIN(config);

	if (gpio > NR_MSM_GPIOS)
		return -EINVAL;

	__gpio_tlmm_config(config);
	mb();

	return 0;
}
EXPORT_SYMBOL(gpio_tlmm_config);

int msm_gpio_install_direct_irq(unsigned gpio, unsigned irq,
					unsigned int input_polarity)
{
	unsigned long irq_flags;

	if (gpio >= NR_MSM_GPIOS || irq >= NR_TLMM_MSM_DIR_CONN_IRQ)
		return -EINVAL;

	spin_lock_irqsave(&tlmm_lock, irq_flags);
	__msm_gpio_install_direct_irq(gpio, irq, input_polarity);
	mb();
	spin_unlock_irqrestore(&tlmm_lock, irq_flags);

	return 0;
}
EXPORT_SYMBOL(msm_gpio_install_direct_irq);

#ifdef CONFIG_OF
static int msm_gpio_irq_domain_xlate(struct irq_domain *d,
				     struct device_node *controller,
				     const u32 *intspec,
				     unsigned int intsize,
				     unsigned long *out_hwirq,
				     unsigned int *out_type)
{
	if (d->of_node != controller)
		return -EINVAL;
	if (intsize != 2)
		return -EINVAL;

	
	*out_hwirq = intspec[0];

	
	*out_type = intspec[1] & IRQ_TYPE_SENSE_MASK;
	return 0;
}

static int msm_gpio_irq_domain_map(struct irq_domain *d, unsigned int irq,
				   irq_hw_number_t hwirq)
{
	irq_set_lockdep_class(irq, &msm_gpio_lock_class);
	irq_set_chip_and_handler(irq, &msm_gpio_irq_chip,
			handle_level_irq);
	set_irq_flags(irq, IRQF_VALID);

	return 0;
}

static struct irq_domain_ops msm_gpio_irq_domain_ops = {
	.xlate = msm_gpio_irq_domain_xlate,
	.map = msm_gpio_irq_domain_map,
};

int __init msm_gpio_of_init(struct device_node *node,
			    struct device_node *parent)
{
	msm_gpio.domain = irq_domain_add_linear(node, NR_MSM_GPIOS,
			&msm_gpio_irq_domain_ops, &msm_gpio);
	if (!msm_gpio.domain) {
		WARN(1, "Cannot allocate irq_domain\n");
		return -ENOMEM;
	}

	return 0;
}
#endif

MODULE_AUTHOR("Gregory Bean <gbean@codeaurora.org>");
MODULE_DESCRIPTION("Driver for Qualcomm MSM TLMMv2 SoC GPIOs");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("sysdev:msmgpio");
