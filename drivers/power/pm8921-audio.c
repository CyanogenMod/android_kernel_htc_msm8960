/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/mfd/pm8xxx/pm8921-charger.h>
#include <linux/mfd/pm8xxx/pm8921-bms.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <linux/mfd/pm8xxx/ccadc.h>
#include <linux/mfd/pm8xxx/core.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/bitops.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/gpio.h>

#include <mach/board_htc.h>

#include <mach/htc_gauge.h>
#include <mach/htc_charger.h>
#include <mach/htc_battery_cell.h>

#undef pr_info
#undef pr_err
#define pr_info(fmt, ...) pr_aud_info(fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...) pr_aud_err(fmt, ##__VA_ARGS__)

#define PM8921_AUDIO_DEV_NAME "pm8921-audio"

#define S4_CTRL_ADDR 0x011
#define S4_MODE_MASK 0x30
#define S4_AUTO      0x30
#define S4_PFM       0x20
#define S4_PWM       0x10

struct pm8921_audio_chip {
	struct device			*dev;
};

static struct pm8921_audio_chip *the_chip;

static int pm_chg_masked_write(struct pm8921_audio_chip *chip, u16 addr,
							u8 mask, u8 val)
{
	int rc;
	u8 reg;

	rc = pm8xxx_readb(chip->dev->parent, addr, &reg);
	if (rc) {
		pr_err("pm8xxx_readb failed: addr=0x%03X, rc=%d\n", addr, rc);
		return rc;
	}
	reg &= ~mask;
	reg |= val & mask;
	rc = pm8xxx_writeb(chip->dev->parent, addr, reg);
	if (rc) {
		pr_err("pm8xxx_writeb failed: addr=0x%03X, rc=%d\n", addr, rc);
		return rc;
	}
	return 0;
}


int pm8921_aud_set_s4_auto(void)
{
    int ret = 0;
    if(!the_chip)
        return -1;

    pr_info("%s: set s4 to audo mode ++\n",__func__);
    ret = pm_chg_masked_write(the_chip, S4_CTRL_ADDR, S4_MODE_MASK, S4_AUTO);
    pr_info("%s: set s4 to audo mode --\n",__func__);

    return ret;
}

int pm8921_aud_set_s4_pfm(void)
{
    int ret = 0;
    if(!the_chip)
        return -1;

    pr_info("%s: set s4 to pfm mode ++\n",__func__);
    ret = pm_chg_masked_write(the_chip, S4_CTRL_ADDR, S4_MODE_MASK, S4_PFM);
    pr_info("%s: set s4 to pfm mode --\n",__func__);

    return ret;
}


int pm8921_aud_set_s4_pwm(void)
{
    int ret = 0;
    if(!the_chip)
        return -1;

    pr_info("%s: set s4 to pwm mode ++\n",__func__);
    ret = pm_chg_masked_write(the_chip, S4_CTRL_ADDR, S4_MODE_MASK, S4_PWM);
    pr_info("%s: set s4 to pwm mode --\n",__func__);

    return ret;
}

static int __devinit pm8921_audio_probe(struct platform_device *pdev)
{
	struct pm8921_audio_chip *chip;
        pr_info("%s\n",__func__);
	chip = kzalloc(sizeof(struct pm8921_audio_chip),
					GFP_KERNEL);
	if (!chip) {
		pr_err("Cannot allocate pm_audio_chip\n");
		return -ENOMEM;
	}

	chip->dev = &pdev->dev;
        platform_set_drvdata(pdev, chip);
        the_chip = chip;

	return 0;

}

static int __devexit pm8921_audio_remove(struct platform_device *pdev)
{
	struct pm8921_audio_chip *chip = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	the_chip = NULL;
	kfree(chip);
	return 0;
}

static struct platform_driver pm8921_audio_driver = {
	.probe	= pm8921_audio_probe,
	.remove	= __devexit_p(pm8921_audio_remove),
	.driver	= {
		.name	= PM8921_AUDIO_DEV_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init pm8921_audio_init(void)
{

	return platform_driver_register(&pm8921_audio_driver);
}

static void __exit pm8921_audio_exit(void)
{
	platform_driver_unregister(&pm8921_audio_driver);
}

late_initcall(pm8921_audio_init);
module_exit(pm8921_audio_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("PMIC8921 audio driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:" PM8921_AUDIO_DEV_NAME);
