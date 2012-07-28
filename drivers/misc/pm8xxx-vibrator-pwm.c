/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mfd/pm8xxx/core.h>
#include <linux/mfd/pm8xxx/pm8xxx-vibrator-pwm.h>
#include <linux/pwm.h>
#include "../staging/android/timed_output.h"
#include <linux/gpio.h>

#define VIB_PWM_DBG(fmt, ...) \
		({ if (0) printk(KERN_DEBUG "[VIB_PWM]" fmt, ##__VA_ARGS__); })
#define VIB_PWM_INFO(fmt, ...) \
		printk(KERN_INFO "[VIB_PWM]" fmt, ##__VA_ARGS__)
#define VIB_PWM_ERR(fmt, ...) \
		printk(KERN_ERR "[VIB_PWM][ERR]" fmt, ##__VA_ARGS__)

struct pm8xxx_vib_pwm {
	struct hrtimer vib_timer;
	struct timed_output_dev timed_dev;
	struct pwm_device 		*pwm_vib;
	spinlock_t lock;
	struct work_struct work;
	struct device *dev;
	const struct pm8xxx_vibrator_pwm_platform_data *pdata;
	int state;
	int vdd_gpio;
	int ena_gpio;
};
static int duty_us, period_us;
static int switch_state = 1;
static int pm8xxx_vib_set_on(struct pm8xxx_vib_pwm *vib)
{
	int rc = 0;

	VIB_PWM_INFO("%s \n",__func__);


		if (switch_state == 0){
			VIB_PWM_INFO("%s vibrator is disable by switch\n",__func__);
			return 0;
		}
		rc = pwm_config(vib->pwm_vib, duty_us, period_us);
		if (rc < 0) {
			VIB_PWM_ERR("%s pwm config fail",__func__);
			return -EINVAL;
		}
		rc = pwm_enable(vib->pwm_vib);
		if (rc < 0) {
			VIB_PWM_ERR("%s pwm enable fail",__func__);
			return -EINVAL;
		}
		rc = gpio_direction_output(vib->ena_gpio, ENABLE_AMP);
		if (rc < 0) {
			VIB_PWM_ERR("%s enable amp fail",__func__);
			return -EINVAL;
		}
	return rc;
}

static int pm8xxx_vib_set_off(struct pm8xxx_vib_pwm *vib)
{
	int rc = 0;
	VIB_PWM_INFO("%s \n",__func__);

		rc = pwm_config(vib->pwm_vib, period_us/2, period_us);
		if (rc < 0) {
			VIB_PWM_ERR("%s pwm config fail",__func__);
			return -EINVAL;
		}
		rc = pwm_enable(vib->pwm_vib);
		if (rc < 0) {
			VIB_PWM_ERR("%s pwm enable fail",__func__);
			return -EINVAL;
		}
		rc = gpio_direction_output(vib->ena_gpio, DISABLE_AMP);
		if (rc < 0) {
			VIB_PWM_ERR("%s disable amp fail",__func__);
			return -EINVAL;
		}
		pwm_disable(vib->pwm_vib);
	return rc;
}

static void pm8xxx_vib_enable(struct timed_output_dev *dev, int value)
{
	struct pm8xxx_vib_pwm *vib = container_of(dev, struct pm8xxx_vib_pwm,
					 timed_dev);
	VIB_PWM_INFO("%s vibrate period: %d msec\n",__func__,value);

retry:

	if (hrtimer_try_to_cancel(&vib->vib_timer) < 0) {
		cpu_relax();
		goto retry;
	}

	if (value == 0)
		pm8xxx_vib_set_off(vib);
	else {
		value = (value > vib->pdata->max_timeout_ms ?
				 vib->pdata->max_timeout_ms : value);
                pm8xxx_vib_set_on(vib);
		hrtimer_start(&vib->vib_timer,
			      ktime_set(value / 1000, (value % 1000) * 1000000),
			      HRTIMER_MODE_REL);
	}
}

static void pm8xxx_vib_update(struct work_struct *work)
{
	struct pm8xxx_vib_pwm *vib = container_of(work, struct pm8xxx_vib_pwm,
					 work);
	pm8xxx_vib_set_off(vib);
}

static int pm8xxx_vib_get_time(struct timed_output_dev *dev)
{
	struct pm8xxx_vib_pwm *vib = container_of(dev, struct pm8xxx_vib_pwm,
							 timed_dev);
	if (hrtimer_active(&vib->vib_timer)) {
		ktime_t r = hrtimer_get_remaining(&vib->vib_timer);
		return (int)ktime_to_us(r);
	} else
		return 0;
}

static enum hrtimer_restart pm8xxx_vib_timer_func(struct hrtimer *timer)
{
	int rc;
	struct pm8xxx_vib_pwm *vib = container_of(timer, struct pm8xxx_vib_pwm,
							 vib_timer);
	VIB_PWM_INFO("%s \n",__func__);

	rc = gpio_direction_output(vib->ena_gpio, DISABLE_AMP);
	if (rc < 0) {
		VIB_PWM_ERR("%s disable amp fail",__func__);
	}
	schedule_work(&vib->work);

	return HRTIMER_NORESTART;
}

static ssize_t duty_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf, "duty_us:%d , duty_period:%d\n", duty_us,period_us);
}

static ssize_t duty_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	int duty, duty_period;


	duty = -1;
	duty_period = -1;
	sscanf(buf, "%d %d", &duty, &duty_period);
	VIB_PWM_INFO("%s dutys input: duty:%d, duty_period:%d\n",__func__,duty,duty_period);
	if (duty > duty_period || duty < 0){
		VIB_PWM_INFO("%s dutys input fail, duty must between 0 and duty_period\n",__func__);
		return -EINVAL;
	}
	if (duty_period > 62 || duty_period < 50){
		VIB_PWM_INFO("%s dutys input fail, duty_period must between 50 to 62\n",__func__);
		return -EINVAL;
	}
	duty_us = duty;
	period_us = duty_period;
	return size;
}

static DEVICE_ATTR(dutys, S_IRUGO | S_IWUSR, duty_show, duty_store);

static ssize_t switch_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf, "switch status:%d \n", switch_state);
}

static ssize_t switch_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	int switch_status;
	switch_status = -1;
	sscanf(buf, "%d ",&switch_status);
	VIB_PWM_INFO("%s: %d\n",__func__,switch_status);
	switch_state = switch_status;
	return size;
}

static DEVICE_ATTR(function_switch, S_IRUGO | S_IWUSR, switch_show, switch_store);



#ifdef CONFIG_PM
static int pm8xxx_vib_suspend(struct device *dev)
{
	struct pm8xxx_vib_pwm *vib = dev_get_drvdata(dev);
	VIB_PWM_INFO("%s \n",__func__);
	hrtimer_cancel(&vib->vib_timer);
	cancel_work_sync(&vib->work);
	/* turn-off vibrator */
	pm8xxx_vib_set_off(vib);
	gpio_direction_output(vib->vdd_gpio, DISABLE_VDD);
	return 0;
}
static int pm8xxx_vib_resume(struct device *dev)
{
	struct pm8xxx_vib_pwm *vib = dev_get_drvdata(dev);
	VIB_PWM_INFO("%s \n",__func__);
	gpio_direction_output(vib->vdd_gpio, ENABLE_VDD);
	return 0;
}
static const struct dev_pm_ops pm8xxx_vib_pm_ops = {
	.suspend = pm8xxx_vib_suspend,
	.resume = pm8xxx_vib_resume,
};
#endif

static int __devinit pm8xxx_vib_probe(struct platform_device *pdev)

{
	const struct pm8xxx_vibrator_pwm_platform_data *pdata =
						pdev->dev.platform_data;
	struct pm8xxx_vib_pwm *vib;
	int rc;
	VIB_PWM_INFO("%s+\n", __func__);

	if (!pdata)
		return -EINVAL;
	if (pdata->duty_us > pdata->PERIOD_US  ||
			 pdata->duty_us < 0)
		return -EINVAL;
	vib = kzalloc(sizeof(*vib), GFP_KERNEL);
	if (!vib)
		return -ENOMEM;
	vib->pdata	= pdata;
	vib->vdd_gpio	= pdata->vdd_gpio;
	vib->ena_gpio	= pdata->ena_gpio;
	vib->dev	= &pdev->dev;
	spin_lock_init(&vib->lock);
	INIT_WORK(&vib->work, pm8xxx_vib_update);
	hrtimer_init(&vib->vib_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vib->vib_timer.function = pm8xxx_vib_timer_func;
	vib->timed_dev.name = "vibrator";
	vib->timed_dev.get_time = pm8xxx_vib_get_time;
	vib->timed_dev.enable = pm8xxx_vib_enable;
	vib->pwm_vib = pwm_request(vib->pdata->bank, vib->timed_dev.name);
	if (vib->pwm_vib < 0){
		rc = -ENOMEM;
		VIB_PWM_ERR("%s, pwm_request fail\n", __func__);
		goto err_pwm_request;
	}

	rc = gpio_request(vib->ena_gpio, "TI_AMP_ena");
	if (rc) {
		rc = -ENOMEM;
		VIB_PWM_ERR("%s, gpio_request ena fail\n", __func__);
		goto err_ena_gpio_request;
	}
	rc= gpio_request(vib->vdd_gpio, "TI_AMP_vdd");
	if(rc) {
		rc = -ENOMEM;
		VIB_PWM_ERR("%s, gpio_request vdd fail\n", __func__);
		goto err_vdd_gpio_request;
	}
	rc = timed_output_dev_register(&vib->timed_dev);
	if (rc < 0)
		goto err_read_vib;
	rc = device_create_file(vib->timed_dev.dev, &dev_attr_dutys);
	if (rc < 0) {
		VIB_PWM_ERR("%s, create duty sysfs fail: dutys\n", __func__);
	}
	rc = device_create_file(vib->timed_dev.dev, &dev_attr_function_switch);
	if (rc < 0) {
		VIB_PWM_ERR("%s, create duty sysfs fail: function_switch\n", __func__);
	}

	platform_set_drvdata(pdev, vib);
	duty_us= vib->pdata->duty_us;
	period_us=vib->pdata->PERIOD_US;
	VIB_PWM_INFO("%s-\n", __func__);
	return 0;
err_vdd_gpio_request:
	gpio_free(vib->vdd_gpio);
err_ena_gpio_request:
	gpio_free(vib->ena_gpio);
err_pwm_request:
	pwm_free(vib->pwm_vib);
err_read_vib:
	kfree(vib);
	return rc;
}

static int __devexit pm8xxx_vib_remove(struct platform_device *pdev)
{
	struct pm8xxx_vib_pwm *vib = platform_get_drvdata(pdev);

	cancel_work_sync(&vib->work);
	hrtimer_cancel(&vib->vib_timer);
	timed_output_dev_unregister(&vib->timed_dev);
	platform_set_drvdata(pdev, NULL);
	kfree(vib);

	return 0;
}

static struct platform_driver pm8xxx_vib_driver = {
	.probe		= pm8xxx_vib_probe,
	.remove		= __devexit_p(pm8xxx_vib_remove),
	.driver		= {
		.name	= PM8XXX_VIBRATOR_PWM_DEV_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &pm8xxx_vib_pm_ops,
#endif
	},
};
static int __init pm8xxx_vib_init(void)
{
	return platform_driver_register(&pm8xxx_vib_driver);
}
module_init(pm8xxx_vib_init);

static void __exit pm8xxx_vib_exit(void)
{
	platform_driver_unregister(&pm8xxx_vib_driver);
}
module_exit(pm8xxx_vib_exit);

MODULE_ALIAS("platform:" PM8XXX_VIBRATOR_PWM_DEV_NAME);
MODULE_DESCRIPTION("pm8xxx vibrator pwm driver");
MODULE_LICENSE("GPL v2");

