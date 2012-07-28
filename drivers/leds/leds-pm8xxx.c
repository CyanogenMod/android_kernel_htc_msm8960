
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

#define pr_fmt(fmt)	"%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/pwm.h>

#include <linux/mfd/pm8xxx/core.h>
#include <linux/leds-pm8xxx.h>

#define SSBI_REG_ADDR_DRV_KEYPAD	0x48
#define PM8XXX_DRV_KEYPAD_BL_MASK	0xf0
#define PM8XXX_DRV_KEYPAD_BL_SHIFT	0x04

#define SSBI_REG_ADDR_FLASH_DRV0        0x49
#define PM8XXX_DRV_FLASH_MASK           0xf0
#define PM8XXX_DRV_FLASH_SHIFT          0x04

#define SSBI_REG_ADDR_FLASH_DRV1        0xFB

#define SSBI_REG_ADDR_LED_CTRL_BASE	0x131
#define SSBI_REG_ADDR_LED_CTRL(n)	(SSBI_REG_ADDR_LED_CTRL_BASE + (n))
#define PM8XXX_DRV_LED_CTRL_MASK	0xf8
#define PM8XXX_DRV_LED_CTRL_SHIFT	0x03

#define MAX_FLASH_LED_CURRENT		300
#define MAX_LC_LED_CURRENT		40
#define MAX_KP_BL_LED_CURRENT		300

#define PM8XXX_ID_LED_CURRENT_FACTOR	2  /* Iout = x * 2mA */
#define PM8XXX_ID_FLASH_CURRENT_FACTOR	20 /* Iout = x * 20mA */

#define PM8XXX_FLASH_MODE_DBUS1		1
#define PM8XXX_FLASH_MODE_DBUS2		2
#define PM8XXX_FLASH_MODE_PWM		3

#define PM8XXX_LED_OFFSET(id) (PM8XXX_ID_LED_0 - (id))

#define MAX_LC_LED_BRIGHTNESS		20
#define MAX_FLASH_BRIGHTNESS		15
#define MAX_KB_LED_BRIGHTNESS		15

#define PM8XXX_LPG_CTL_REGS		7

#define LED_DBG(fmt, ...) \
		({ if (0) printk(KERN_DEBUG "[LED]" fmt, ##__VA_ARGS__); })
#define LED_INFO(fmt, ...) \
		printk(KERN_INFO "[LED]" fmt, ##__VA_ARGS__)
#define LED_ERR(fmt, ...) \
		printk(KERN_ERR "[LED][ERR]" fmt, ##__VA_ARGS__)

static struct workqueue_struct *g_led_work_queue;
static struct pm8xxx_led_data *pm8xxx_leds	, *for_key_led_data;
static int flag_hold_virtual_key = 0;
static int virtual_key_state;
u8 pm8xxxx_led_pwm_mode(int flag)
{
	u8 mode = 0;
	switch (flag) {
	case PM8XXX_ID_LED_0:
		mode = PM8XXX_LED_MODE_PWM3;
		break;
	case PM8XXX_ID_LED_1:
		mode = PM8XXX_LED_MODE_PWM2;
		break;
	case PM8XXX_ID_LED_2:
		mode = PM8XXX_LED_MODE_PWM1;
		break;
	}
	return mode;
}

static void led_fade_do_work(struct work_struct *work)
{
	struct pm8xxx_led_data *led;
	int rc, offset;
	u8 level;

	led = container_of(work, struct pm8xxx_led_data, fade_delayed_work.work);

	level = (0 << PM8XXX_DRV_LED_CTRL_SHIFT) & PM8XXX_DRV_LED_CTRL_MASK;
	offset = PM8XXX_LED_OFFSET(led->id);
	led->reg &= ~PM8XXX_DRV_LED_CTRL_MASK;
	led->reg |= level;
	rc = pm8xxx_writeb(led->dev->parent, SSBI_REG_ADDR_LED_CTRL(offset), led->reg);
	if (rc)
		LED_ERR("%s can't set (%d) led value rc=%d\n", __func__, led->id, rc);
}
void pm8xxx_led_current_set_for_key(int brightness_key)
{
	int rc, offset;
	static u8 level, register_key;

	LED_INFO("%s brightness_key: %d\n", __func__,brightness_key);

	if (brightness_key) {
		flag_hold_virtual_key = 1;
		level = (40 << PM8XXX_DRV_LED_CTRL_SHIFT) & PM8XXX_DRV_LED_CTRL_MASK;
		offset = PM8XXX_LED_OFFSET(PM8XXX_ID_LED_0);
		register_key	= pm8xxxx_led_pwm_mode(PM8XXX_ID_LED_0);
		register_key &= ~PM8XXX_DRV_LED_CTRL_MASK;
		register_key |= level;
		rc = pm8xxx_writeb(for_key_led_data->dev->parent, SSBI_REG_ADDR_LED_CTRL(offset), register_key);
		if (rc)
			LED_ERR("%s can't set (%d) led value rc=%d\n", __func__, PM8XXX_ID_LED_0, rc);
			pwm_config(for_key_led_data->pwm_led, 320000, 640000);
			pwm_enable(for_key_led_data->pwm_led);
	} else {
		pwm_disable(for_key_led_data->pwm_led);
		level = (0 << PM8XXX_DRV_LED_CTRL_SHIFT) & PM8XXX_DRV_LED_CTRL_MASK;
		offset = PM8XXX_LED_OFFSET(PM8XXX_ID_LED_0);
		register_key	= pm8xxxx_led_pwm_mode(PM8XXX_ID_LED_0);
		register_key &= ~PM8XXX_DRV_LED_CTRL_MASK;
		register_key |= level;
		rc = pm8xxx_writeb(for_key_led_data->dev->parent, SSBI_REG_ADDR_LED_CTRL(offset), register_key);
		if (rc)
			LED_ERR("%s can't set (%d) led value rc=%d\n", __func__, PM8XXX_ID_LED_0, rc);
		if (virtual_key_state != 0){
			level = 0;
			register_key = 0;
			level = (40 << PM8XXX_DRV_LED_CTRL_SHIFT) & PM8XXX_DRV_LED_CTRL_MASK;
			offset = PM8XXX_LED_OFFSET(PM8XXX_ID_LED_0);
			register_key    = pm8xxxx_led_pwm_mode(PM8XXX_ID_LED_0);
			register_key &= ~PM8XXX_DRV_LED_CTRL_MASK;
			register_key |= level;
			rc = pm8xxx_writeb(for_key_led_data->dev->parent, SSBI_REG_ADDR_LED_CTRL(offset), register_key);
			if (rc)
				LED_ERR("%s can't set (%d) led value rc=%d\n", __func__, PM8XXX_ID_LED_0, rc);
			pwm_config(for_key_led_data->pwm_led, 64000, 64000);
			pwm_enable(for_key_led_data->pwm_led);
		}
		flag_hold_virtual_key = 0;

	}
}
static void pm8xxx_led_current_set(struct led_classdev *led_cdev, enum led_brightness brightness)
{
	struct pm8xxx_led_data *led = container_of(led_cdev,  struct pm8xxx_led_data, cdev);
	int rc, offset;
	u8 level;

	int *pduties;

	LED_INFO("%s, bank:%d, brightness:%d\n", __func__, led->bank, brightness);
	cancel_delayed_work_sync(&led->fade_delayed_work);
	virtual_key_state = brightness;
	if (flag_hold_virtual_key == 1) {
		LED_INFO("%s, key control \n", __func__);
		return;
	}

	if(brightness) {
		level = (led->out_current << PM8XXX_DRV_LED_CTRL_SHIFT) & PM8XXX_DRV_LED_CTRL_MASK;

		offset = PM8XXX_LED_OFFSET(led->id);
		led->reg &= ~PM8XXX_DRV_LED_CTRL_MASK;
		led->reg |= level;
		rc = pm8xxx_writeb(led->dev->parent, SSBI_REG_ADDR_LED_CTRL(offset), led->reg);
		if (rc)
			LED_ERR("%s can't set (%d) led value rc=%d\n", __func__, led->id, rc);

		if (led->function_flags & LED_BRETH_FUNCTION) {
			pduties = led->duties;
			pm8xxx_pwm_lut_config(led->pwm_led,
						led->period_us,
						pduties,
						led->duty_time_ms,
						led->start_index,
						led->duites_size,
						0, 0,
						led->lut_flag);
			pm8xxx_pwm_lut_enable(led->pwm_led, 0);
			pm8xxx_pwm_lut_enable(led->pwm_led, 1);
		} else {
			pwm_config(led->pwm_led, 64000, 64000);
			pwm_enable(led->pwm_led);
		}
	} else {
		if (led->function_flags & LED_BRETH_FUNCTION) {
			pduties = led->duties + led->duites_size;
			pm8xxx_pwm_lut_config(led->pwm_led,
						led->period_us,
						pduties,
						led->duty_time_ms,
						led->start_index,
						led->duites_size,
						0, 0,
						led->lut_flag);
			pm8xxx_pwm_lut_enable(led->pwm_led, 0);
			pm8xxx_pwm_lut_enable(led->pwm_led, 1);
			queue_delayed_work(g_led_work_queue,
						&led->fade_delayed_work,
						msecs_to_jiffies(led->duty_time_ms*led->duites_size));
		} else {
			pwm_disable(led->pwm_led);
			level = (0 << PM8XXX_DRV_LED_CTRL_SHIFT) & PM8XXX_DRV_LED_CTRL_MASK;
			offset = PM8XXX_LED_OFFSET(led->id);
			led->reg &= ~PM8XXX_DRV_LED_CTRL_MASK;
			led->reg |= level;
			rc = pm8xxx_writeb(led->dev->parent, SSBI_REG_ADDR_LED_CTRL(offset), led->reg);
			if (rc)
				LED_ERR("%s can't set (%d) led value rc=%d\n", __func__, led->id, rc);
		}
	}
}

static void pm8xxx_led_gpio_set(struct led_classdev *led_cdev, enum led_brightness brightness)
{
	struct pm8xxx_led_data *led = container_of(led_cdev,  struct pm8xxx_led_data, cdev);
	LED_INFO("%s, bank:%d, brightness:%d\n", __func__, led->bank, brightness);
	if (led->gpio_status_switch != NULL)
		led->gpio_status_switch(0);
	pwm_disable(led->pwm_led);
	if (brightness) {
		if (led->gpio_status_switch != NULL)
			led->gpio_status_switch(1);
		pwm_config(led->pwm_led, 64000, 64000);
		pwm_enable(led->pwm_led);
	}
}

static void led_work_func(struct work_struct *work)
{
	struct pm8xxx_led_data *ldata;

	ldata = container_of(work, struct pm8xxx_led_data, led_work);
	pwm_disable(ldata->pwm_led);
	if (ldata->gpio_status_switch != NULL)
		ldata->gpio_status_switch(0);
}

static void led_alarm_handler(struct alarm *alarm)
{
	struct pm8xxx_led_data *ldata;

	ldata = container_of(alarm, struct pm8xxx_led_data, led_alarm);
	queue_work(g_led_work_queue, &ldata->led_work);
}

static void led_blink_do_work(struct work_struct *work)
{
	struct pm8xxx_led_data *led;

	led = container_of(work, struct pm8xxx_led_data, blink_delayed_work.work);

	if (led->gpio_status_switch != NULL)
		led->gpio_status_switch(1);
	pwm_config(led->pwm_led, led->duty_time_ms * 1000, led->period_us);
	pwm_enable(led->pwm_led);
}

static ssize_t pm8xxx_led_blink_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct led_classdev *led_cdev;
	struct pm8xxx_led_data *ldata;
	int val;

	val = -1;
	sscanf(buf, "%u", &val);
	if (val < 0 || val > 255)
		return -EINVAL;

	led_cdev = (struct led_classdev *) dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct pm8xxx_led_data, cdev);

	LED_INFO("%s: bank %d blink %d\n", __func__, ldata->bank, val);

	switch (val) {
	case BLINK_STOP:
		if (ldata->gpio_status_switch != NULL)
			ldata->gpio_status_switch(0);
		pwm_disable(ldata->pwm_led);
		break;
	case BLINK_UNCHANGE:
		pwm_disable(ldata->pwm_led);
		if (led_cdev->brightness) {
			if (ldata->gpio_status_switch != NULL)
				ldata->gpio_status_switch(1);
			pwm_config(ldata->pwm_led, 64000, 64000);
			pwm_enable(ldata->pwm_led);
		} else {
			pwm_disable(ldata->pwm_led);
			if (ldata->gpio_status_switch != NULL)
				ldata->gpio_status_switch(0);
		}
		break;
	case BLINK_64MS_PER_2SEC:
		if (ldata->gpio_status_switch != NULL)
			ldata->gpio_status_switch(1);
		pwm_disable(ldata->pwm_led);
		pwm_config(ldata->pwm_led, 64000, 2000000);
		pwm_enable(ldata->pwm_led);
		break;
	case BLINK_64MS_ON_310MS_PER_2SEC:
		cancel_delayed_work_sync(&ldata->blink_delayed_work);
		pwm_disable(ldata->pwm_led);
		ldata->duty_time_ms = 64;
		ldata->period_us = 2000000;
		queue_delayed_work(g_led_work_queue, &ldata->blink_delayed_work,
				   msecs_to_jiffies(310));
		break;
	case BLINK_64MS_ON_2SEC_PER_2SEC:
		cancel_delayed_work_sync(&ldata->blink_delayed_work);
		pwm_disable(ldata->pwm_led);
		ldata->duty_time_ms = 64;
		ldata->period_us = 2000000;
		queue_delayed_work(g_led_work_queue, &ldata->blink_delayed_work,
				   msecs_to_jiffies(1000));
		break;
	case BLINK_1SEC_PER_2SEC:
		pwm_disable(ldata->pwm_led);
		pwm_config(ldata->pwm_led, 1000000, 2000000);
		pwm_enable(ldata->pwm_led);
		break;
	default:
		LED_ERR("%s: bank %d did not support blink type %d\n", __func__, ldata->bank, val);
		return -EINVAL;
	}

	return count;
}
static DEVICE_ATTR(blink, 0644, NULL, pm8xxx_led_blink_store);

static ssize_t pm8xxx_led_off_timer_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct led_classdev *led_cdev;
	struct pm8xxx_led_data *ldata;
	int min, sec;
	uint16_t off_timer;
	ktime_t interval;
	ktime_t next_alarm;

	min = -1;
	sec = -1;
	sscanf(buf, "%d %d", &min, &sec);

	if (min < 0 || min > 255)
		return -EINVAL;
	if (sec < 0 || sec > 255)
		return -EINVAL;
	led_cdev = (struct led_classdev *) dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct pm8xxx_led_data, cdev);

	LED_INFO("Setting %s off_timer to %d min %d sec +\n", led_cdev->name, min, sec);
	off_timer = min * 60 + sec;

	alarm_cancel(&ldata->led_alarm);
	cancel_work_sync(&ldata->led_work);
	if (off_timer) {
		interval = ktime_set(off_timer, 0);
		next_alarm = ktime_add(alarm_get_elapsed_realtime(), interval);
		alarm_start_range(&ldata->led_alarm, next_alarm, next_alarm);
	}
	return count;
}
static DEVICE_ATTR(off_timer, 0644, NULL, pm8xxx_led_off_timer_store);

static ssize_t pm8xxx_led_currents_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct led_classdev *led_cdev;
	struct pm8xxx_led_data *ldata;

	led_cdev = (struct led_classdev *) dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct pm8xxx_led_data, cdev);

	return sprintf(buf, "%d\n", ldata->out_current);
}

static ssize_t pm8xxx_led_currents_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	int currents = 0;
	struct led_classdev *led_cdev;
	struct pm8xxx_led_data *ldata;

	sscanf(buf, "%d", &currents);
	if (currents < 0)
		return -EINVAL;

	led_cdev = (struct led_classdev *)dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct pm8xxx_led_data, cdev);

	LED_INFO("%s: bank %d currents %d\n", __func__, ldata->bank, currents);
	ldata->out_current = currents;

	ldata->cdev.brightness_set(led_cdev, 0);
	if (currents)
		ldata->cdev.brightness_set(led_cdev, 255);

	return count;
}
static DEVICE_ATTR(currents, 0644, pm8xxx_led_currents_show, pm8xxx_led_currents_store);

static int __devinit pm8xxx_led_probe(struct platform_device *pdev)
{
	const struct pm8xxx_led_platform_data *pdata = pdev->dev.platform_data;
	struct pm8xxx_led_configure *curr_led;
	struct pm8xxx_led_data *led, *led_dat;
	int i, ret = -ENOMEM;

	if (pdata == NULL) {
		LED_ERR("platform data not supplied\n");
		return -EINVAL;
	}

	/* Let the last member of the list be zero to
	 * mark the end of the list.
	 */
	led = kcalloc(pdata->num_leds + 1, sizeof(*led), GFP_KERNEL);
	if (led == NULL) {
		LED_ERR("failed to alloc memory\n");
		return -ENOMEM;
	}

	g_led_work_queue = create_workqueue("pm8xxx-led");
	if (g_led_work_queue == NULL) {
		LED_ERR("failed to create workqueue\n");
		goto err_create_work_queue;
	}

	for (i = 0; i < pdata->num_leds; i++) {
		curr_led			= &pdata->leds[i];
		led_dat				= &led[i];
		led_dat->cdev.name		= curr_led->name;
		led_dat->id     		= curr_led->flags;
		led_dat->bank			= curr_led->flags;
		led_dat->function_flags		= curr_led->function_flags;
		led_dat->start_index		= curr_led->start_index;
		led_dat->duty_time_ms		= curr_led->duty_time_ms;
		led_dat->period_us		= curr_led->period_us;
		led_dat->duites_size		= curr_led->duites_size;
		led_dat->lut_flag		= curr_led->lut_flag;
		led_dat->out_current		= curr_led->out_current;
		led_dat->duties			= &(curr_led->duties[0]);
		led_dat->pwm_led 		= pwm_request(led_dat->bank, led_dat->cdev.name);

		switch (led_dat->id) {
		case PM8XXX_ID_GPIO24:
		case PM8XXX_ID_GPIO25:
		case PM8XXX_ID_GPIO26:
			led_dat->cdev.brightness_set = pm8xxx_led_gpio_set;
			if (curr_led->gpio_status_switch != NULL)
				led_dat->gpio_status_switch = curr_led->gpio_status_switch;
			break;
		case PM8XXX_ID_LED_0:
		case PM8XXX_ID_LED_1:
		case PM8XXX_ID_LED_2:
			led_dat->cdev.brightness_set    = pm8xxx_led_current_set;
			if (led_dat->function_flags & LED_PWM_FUNCTION) {
				led_dat->reg		= pm8xxxx_led_pwm_mode(led_dat->id);
				INIT_DELAYED_WORK(&led[i].fade_delayed_work, led_fade_do_work);
			} else
				led_dat->reg		= PM8XXX_LED_MODE_MANUAL;
			break;
		case PM8XXX_ID_LED_KB_LIGHT:
			break;
		}
		led_dat->cdev.brightness	= LED_OFF;
		led_dat->dev			= &pdev->dev;

		ret = led_classdev_register(&pdev->dev, &led_dat->cdev);
		if (ret) {
			LED_ERR("unable to register led %d,ret=%d\n", led_dat->id, ret);
			goto err_register_led_cdev;
		}

		if (led_dat->id >= PM8XXX_ID_LED_2 && led_dat->id <= PM8XXX_ID_LED_0) {
			ret = device_create_file(led_dat->cdev.dev, &dev_attr_currents);
			if (ret < 0) {
				LED_ERR("%s: Failed to create %d attr currents\n", __func__, i);
				goto err_register_attr_currents;
			}
		}

		if (led_dat->function_flags & LED_BLINK_FUNCTION) {
			INIT_DELAYED_WORK(&led[i].blink_delayed_work, led_blink_do_work);
			ret = device_create_file(led_dat->cdev.dev, &dev_attr_blink);
			if (ret < 0) {
				LED_ERR("%s: Failed to create %d attr blink\n", __func__, i);
				goto err_register_attr_blink;
			}

			ret = device_create_file(led_dat->cdev.dev, &dev_attr_off_timer);
			if (ret < 0) {
				LED_ERR("%s: Failed to create %d attr off timer\n", __func__, i);
				goto err_register_attr_off_timer;
			}
			alarm_init(&led[i].led_alarm, ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP, led_alarm_handler);
			INIT_WORK(&led[i].led_work, led_work_func); /*Off blink after alarm*/
		}

		if(led_dat->id == PM8XXX_ID_LED_0)
		{
			for_key_led_data = led_dat;
		}
	}

	pm8xxx_leds = led;

	platform_set_drvdata(pdev, led);

	return 0;

err_register_attr_off_timer:
	if (i > 0) {
		for (i = i - 1; i >= 0; i--) {
			if (led[i].function_flags & LED_BLINK_FUNCTION)
				device_remove_file(led[i].cdev.dev, &dev_attr_off_timer);
		}
	}
	i = pdata->num_leds;
err_register_attr_blink:
	if (i > 0) {
		for (i = i - 1; i >= 0; i--) {
			if (led[i].function_flags & LED_BLINK_FUNCTION)
				device_remove_file(led[i].cdev.dev, &dev_attr_blink);
		}
	}
	i = pdata->num_leds;
err_register_attr_currents:
	if (i > 0) {
		for (i = i - 1; i >= 0; i--) {
			if (led[i].function_flags >= PM8XXX_ID_LED_2 && led[i].function_flags <= PM8XXX_ID_LED_0)
				device_remove_file(led[i].cdev.dev, &dev_attr_currents);
		}
	}
	i = pdata->num_leds;
err_register_led_cdev:
	if (i > 0) {
		for (i = i - 1; i >= 0; i--) {
			pwm_free(led[i].pwm_led);
			led_classdev_unregister(&led[i].cdev);
		}
	}
	destroy_workqueue(g_led_work_queue);
err_create_work_queue:
	kfree(led);
	return ret;
}

static int __devexit pm8xxx_led_remove(struct platform_device *pdev)
{
	int i;
	const struct led_platform_data *pdata =
				pdev->dev.platform_data;
	struct pm8xxx_led_data *led = platform_get_drvdata(pdev);

	for (i = 0; i < pdata->num_leds; i++) {
		led_classdev_unregister(&led[i].cdev);
		if (led[i].function_flags >= PM8XXX_ID_LED_2 && led[i].function_flags <= PM8XXX_ID_LED_0)
			device_remove_file(led[i].cdev.dev, &dev_attr_currents);
		if (led[i].function_flags & LED_BLINK_FUNCTION)
			device_remove_file(led[i].cdev.dev, &dev_attr_blink);
		if (led[i].function_flags & LED_BLINK_FUNCTION)
				device_remove_file(led[i].cdev.dev, &dev_attr_off_timer);
	}

	destroy_workqueue(g_led_work_queue);
	kfree(led);

	return 0;
}

static struct platform_driver pm8xxx_led_driver = {
	.probe		= pm8xxx_led_probe,
	.remove		= __devexit_p(pm8xxx_led_remove),
	.driver		= {
		.name	= PM8XXX_LEDS_DEV_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init pm8xxx_led_init(void)
{
	return platform_driver_register(&pm8xxx_led_driver);
}
subsys_initcall(pm8xxx_led_init);

static void __exit pm8xxx_led_exit(void)
{
	platform_driver_unregister(&pm8xxx_led_driver);
}
module_exit(pm8xxx_led_exit);

MODULE_DESCRIPTION("PM8XXX LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:pm8xxx-led");
