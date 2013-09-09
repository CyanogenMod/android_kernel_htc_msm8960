/* Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
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
#include <linux/leds.h>
#include <linux/workqueue.h>
#include <linux/err.h>

#include <linux/mfd/pm8xxx/core.h>
#include <linux/mfd/pm8xxx/pwm.h>
#include <linux/leds-pm8038.h>
#include <linux/wakelock.h>
#include <linux/android_alarm.h>
#include <linux/module.h>
#include <mach/board.h>

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

#define SSBI_REG_ADDR_WLED_CTRL_BASE	0x25A
#define SSBI_REG_ADDR_WLED_CTRL(n)	(SSBI_REG_ADDR_WLED_CTRL_BASE + (n) - 1)

#define WLED_MOD_CTRL_REG		SSBI_REG_ADDR_WLED_CTRL(1)
#define WLED_MAX_CURR_CFG_REG(n)	SSBI_REG_ADDR_WLED_CTRL(n + 2)
#define WLED_BRIGHTNESS_CNTL_REG1(n)	SSBI_REG_ADDR_WLED_CTRL(n + 5)
#define WLED_BRIGHTNESS_CNTL_REG2(n)	SSBI_REG_ADDR_WLED_CTRL(n + 6)
#define WLED_SYNC_REG			SSBI_REG_ADDR_WLED_CTRL(11)
#define WLED_FSW_REG			SSBI_REG_ADDR_WLED_CTRL(12)
#define WLED_OVP_CFG_REG		SSBI_REG_ADDR_WLED_CTRL(13)
#define WLED_BOOST_CFG_REG		SSBI_REG_ADDR_WLED_CTRL(14)
#define WLED_RESERVED_REG		SSBI_REG_ADDR_WLED_CTRL(15)
#define WLED_HIGH_POLE_CAP_REG		SSBI_REG_ADDR_WLED_CTRL(16)

#define WLED_STRINGS			0x03
#define WLED_OVP_VAL_MASK		0x30
#define WLED_OVP_VAL_BIT_SHFT		0x04
#define WLED_BOOST_LIMIT_MASK		0xE0
#define WLED_BOOST_LIMIT_BIT_SHFT	0x05
#define WLED_BOOST_OFF			0x00
#define WLED_EN_MASK			0x01
#define WLED_CP_SELECT_MAX		0x03
#define WLED_CP_SELECT_MASK		0x03
#define WLED_DIG_MOD_GEN_MASK		0x70
#define WLED_CS_OUT_MASK		0x0E
#define WLED_DIG_MOD_GEN_SET		0x10	
#define WLED_CS_OUT_SET			0x02	
#define WLED_CTL_DLY_STEP		200
#define WLED_CTL_DLY_MAX		1400
#define WLED_CTL_DLY_MASK		0xE0
#define WLED_CTL_DLY_BIT_SHFT		0x05
#define WLED_MAX_CURR			25
#define WLED_MAX_CURR_MASK		0x1F
#define WLED_OP_FDBCK_MASK		0x1C
#define WLED_OP_FDBCK_BIT_SHFT		0x02
#define WLED_FSW_CTRL_SET		0x00	
#define WLED_MOD_CLK_CTRL_SET		0x06	
#define WLED_CNTRL_14_RESERVED_MASK	0x0F
#define WLED_CNTRL_14_RESERVED_SET	0x03
#define WLED_CNTRL_15_RESERVED_SET	0x3F	

#define WLED_MAX_LEVEL			255
#define WLED_8_BIT_MASK			0xFF
#define WLED_8_BIT_SHFT			0x08
#define WLED_MAX_DUTY_CYCLE		0xFFF

#define WLED_SYNC_VAL			0x07
#define WLED_SYNC_CABC_ENABLED		0x08
#define WLED_SYNC_RESET_VAL		0x00

#define SSBI_REG_ADDR_RGB_CNTL1		0x12D
#define SSBI_REG_ADDR_RGB_CNTL2		0x12E

#define PM8XXX_DRV_RGB_RED_LED		BIT(2)
#define PM8XXX_DRV_RGB_GREEN_LED	BIT(1)
#define PM8XXX_DRV_RGB_BLUE_LED		BIT(0)

#define MAX_FLASH_LED_CURRENT		300
#define MAX_LC_LED_CURRENT		40
#define MAX_KP_BL_LED_CURRENT		300

#define PM8XXX_ID_LED_CURRENT_FACTOR	2  
#define PM8XXX_ID_FLASH_CURRENT_FACTOR	20 

#define PM8XXX_FLASH_MODE_DBUS1		1
#define PM8XXX_FLASH_MODE_DBUS2		2
#define PM8XXX_FLASH_MODE_PWM		3

#define MAX_LC_LED_BRIGHTNESS		20
#define MAX_FLASH_BRIGHTNESS		15
#define MAX_KB_LED_BRIGHTNESS		15

#define PM8XXX_LED_OFFSET(id) ((id) - PM8XXX_ID_LED_0)

#define PM8XXX_LED_PWM_FLAGS	(PM_PWM_LUT_PAUSE_HI_EN | PM_PWM_LUT_RAMP_UP)


#define LED_MAP(_version, _kb, _led0, _led1, _led2, _flash_led0, _flash_led1, \
	_wled, _rgb_led_red, _rgb_led_green, _rgb_led_blue)\
	{\
		.version = _version,\
		.supported = _kb << PM8XXX_ID_LED_KB_LIGHT | \
			_led0 << PM8XXX_ID_LED_0 | _led1 << PM8XXX_ID_LED_1 | \
			_led2 << PM8XXX_ID_LED_2  | \
			_flash_led0 << PM8XXX_ID_FLASH_LED_0 | \
			_flash_led1 << PM8XXX_ID_FLASH_LED_1 | \
			_wled << PM8XXX_ID_WLED | \
			_rgb_led_red << PM8XXX_ID_RGB_LED_RED | \
			_rgb_led_green << PM8XXX_ID_RGB_LED_GREEN | \
			_rgb_led_blue << PM8XXX_ID_RGB_LED_BLUE, \
	}

#define LED_DBG(fmt, ...) \
			({ if (0) printk(KERN_DEBUG "[LED]" fmt, ##__VA_ARGS__); })
#define LED_INFO(fmt, ...) \
			printk(KERN_INFO "[LED]" fmt, ##__VA_ARGS__)
#define LED_ERR(fmt, ...) \
			printk(KERN_ERR "[LED][ERR]" fmt, ##__VA_ARGS__)

struct supported_leds {
	enum pm8xxx_version version;
	u32 supported;
};

static const struct supported_leds led_map[] = {
	LED_MAP(PM8XXX_VERSION_8058, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0),
	LED_MAP(PM8XXX_VERSION_8921, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0),
	LED_MAP(PM8XXX_VERSION_8018, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0),
	LED_MAP(PM8XXX_VERSION_8922, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1),
	LED_MAP(PM8XXX_VERSION_8038, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1),
};

struct pm8xxx_led_data {
	struct led_classdev		cdev;
	int						id;
	u8						reg;
	u8						wled_mod_ctrl_val;
	struct device			*dev;
	struct mutex			lock;
	struct pwm_device		*pwm_dev;
	int						pwm_channel;
	u32						pwm_period_us;
	struct pm8xxx_pwm_duty_cycles *pwm_duty_cycles;
	struct wled_config_data *wled_cfg;
	int						max_current;
	void 					(*gpio_status_switch)(bool);
	struct delayed_work		blink_delayed_work;
	struct delayed_work 	fade_delayed_work;
	int 					period_us;
	int 					duty_time_ms;
	struct alarm 			led_alarm;
	struct work_struct 		led_work;
	int						pwm_coefficient;
};

static struct workqueue_struct *g_led_work_queue;
struct wake_lock pmic_led_wake_lock;
static int virtual_key_led_state, current_blink;
static int flag_hold_virtual_key_led = 0;
static struct pm8xxx_led_data *for_key_led_data;
static int lut_coefficient = 100;
static int dutys_array[64];


#ifdef CONFIG_PM8038_LED_SYNC
static struct pm8xxx_led_data *led_data_button_backlight1, *led_data_button_backlight2;
#endif

static void wled_dump_regs(struct pm8xxx_led_data *led)
{
	int i;
	u8 val;

	for (i = 1; i < 17; i++) {
		pm8xxx_readb(led->dev->parent,
				SSBI_REG_ADDR_WLED_CTRL(i), &val);
		LED_INFO("WLED_CTRL_%d = 0x%02x\n", i, val);
	}
}

static void led_kp_set(struct pm8xxx_led_data *led, enum led_brightness value)
{
	int rc;
	u8 level;

	level = (value << PM8XXX_DRV_KEYPAD_BL_SHIFT) &
				 PM8XXX_DRV_KEYPAD_BL_MASK;

	led->reg &= ~PM8XXX_DRV_KEYPAD_BL_MASK;
	led->reg |= level;

	rc = pm8xxx_writeb(led->dev->parent, SSBI_REG_ADDR_DRV_KEYPAD,
								led->reg);
	if (rc < 0)
		LED_ERR("can't set keypad backlight level rc=%d\n", rc);
}

static void led_lc_set(struct pm8xxx_led_data *led, enum led_brightness value)
{
	int rc, offset;
	u8 level;

	level = (value << PM8XXX_DRV_LED_CTRL_SHIFT) &
				PM8XXX_DRV_LED_CTRL_MASK;

	offset = PM8XXX_LED_OFFSET(led->id);

	led->reg &= ~PM8XXX_DRV_LED_CTRL_MASK;
	led->reg |= level;

	rc = pm8xxx_writeb(led->dev->parent, SSBI_REG_ADDR_LED_CTRL(offset),
								led->reg);
	if (rc)
		LED_ERR("can't set (%d) led value rc=%d\n",	led->id, rc);
}

static void
led_flash_set(struct pm8xxx_led_data *led, enum led_brightness value)
{
	int rc;
	u8 level;
	u16 reg_addr;

	level = (value << PM8XXX_DRV_FLASH_SHIFT) &
				 PM8XXX_DRV_FLASH_MASK;

	led->reg &= ~PM8XXX_DRV_FLASH_MASK;
	led->reg |= level;

	if (led->id == PM8XXX_ID_FLASH_LED_0)
		reg_addr = SSBI_REG_ADDR_FLASH_DRV0;
	else
		reg_addr = SSBI_REG_ADDR_FLASH_DRV1;

	rc = pm8xxx_writeb(led->dev->parent, reg_addr, led->reg);
	if (rc < 0)
		LED_ERR("can't set flash led%d level rc=%d\n", led->id, rc);
}

static int
led_wled_set(struct pm8xxx_led_data *led, enum led_brightness value)
{
	int rc, duty;
	u8 val, i, num_wled_strings;

#ifdef CONFIG_BACKLIGHT_WLED_CABC
	return 0;
#endif

	if (value > WLED_MAX_LEVEL)
		value = WLED_MAX_LEVEL;

	if (value == 0) {
		rc = pm8xxx_writeb(led->dev->parent, WLED_MOD_CTRL_REG,
				WLED_BOOST_OFF);
		if (rc) {
			dev_err(led->dev->parent, "can't write wled ctrl config"
				" register rc=%d\n", rc);
			return rc;
		}
	} else {
		rc = pm8xxx_writeb(led->dev->parent, WLED_MOD_CTRL_REG,
				led->wled_mod_ctrl_val);
		if (rc) {
			dev_err(led->dev->parent, "can't write wled ctrl config"
				" register rc=%d\n", rc);
			return rc;
		}
	}

	duty = (WLED_MAX_DUTY_CYCLE * value) / WLED_MAX_LEVEL;

	num_wled_strings = led->wled_cfg->num_strings;

	
	for (i = 0; i < num_wled_strings; i++) {
		rc = pm8xxx_readb(led->dev->parent,
				WLED_BRIGHTNESS_CNTL_REG1(i), &val);
		if (rc) {
			LED_ERR("can't read wled brightnes ctrl"" register1 rc=%d\n", rc);
			return rc;
		}

		val = (val & ~WLED_MAX_CURR_MASK) | (duty >> WLED_8_BIT_SHFT);
		rc = pm8xxx_writeb(led->dev->parent,
				WLED_BRIGHTNESS_CNTL_REG1(i), val);
		if (rc) {
			LED_ERR("can't write wled brightness ctrl"" register1 rc=%d\n", rc);
			return rc;
		}

		val = duty & WLED_8_BIT_MASK;
		rc = pm8xxx_writeb(led->dev->parent,
				WLED_BRIGHTNESS_CNTL_REG2(i), val);
		if (rc) {
			LED_ERR("can't write wled brightness ctrl"" register2 rc=%d\n", rc);
			return rc;
		}
	}

	
	val = WLED_SYNC_VAL;
	rc = pm8xxx_writeb(led->dev->parent, WLED_SYNC_REG, val);
	if (rc) {
		LED_ERR("can't read wled sync register rc=%d\n", rc);
		return rc;
	}

	val = WLED_SYNC_RESET_VAL;
	rc = pm8xxx_writeb(led->dev->parent, WLED_SYNC_REG, val);
	if (rc) {
		LED_ERR("can't read wled sync register rc=%d\n", rc);
		return rc;
	}
	return 0;
}

static void
led_rgb_set(struct pm8xxx_led_data *led, enum led_brightness value)
{
	int rc;
	u8 val, mask;

	rc = pm8xxx_readb(led->dev->parent, SSBI_REG_ADDR_RGB_CNTL1, &val);
	if (rc) {
		LED_ERR("can't read rgb ctrl register rc=%d\n",	rc);
		return;
	}

	switch (led->id) {
	case PM8XXX_ID_RGB_LED_RED:
		mask = PM8XXX_DRV_RGB_RED_LED;
		break;
	case PM8XXX_ID_RGB_LED_GREEN:
		mask = PM8XXX_DRV_RGB_GREEN_LED;
		break;
	case PM8XXX_ID_RGB_LED_BLUE:
		mask = PM8XXX_DRV_RGB_BLUE_LED;
		break;
	default:
		return;
	}

	if (value)
		val |= mask;
	else
		val &= ~mask;

	rc = pm8xxx_writeb(led->dev->parent, SSBI_REG_ADDR_RGB_CNTL1, val);
	if (rc < 0)
		LED_ERR("can't set rgb led %d level rc=%d\n", led->id, rc);
}

static void __pm8xxx_led_work(struct pm8xxx_led_data *led,
					enum led_brightness level)
{
	

	mutex_lock(&led->lock);

	switch (led->id) {
	case PM8XXX_ID_LED_KB_LIGHT:
		led_kp_set(led, level);
		break;
	case PM8XXX_ID_LED_0:
	case PM8XXX_ID_LED_1:
	case PM8XXX_ID_LED_2:
		led_lc_set(led, level);
		break;
	case PM8XXX_ID_FLASH_LED_0:
	case PM8XXX_ID_FLASH_LED_1:
		led_flash_set(led, level);
		break;
	case PM8XXX_ID_WLED:
#if 0
		rc = led_wled_set(led, level);
		if (rc < 0)
			LED_ERR("wled brightness set failed %d\n", rc);
#endif
		break;
	case PM8XXX_ID_RGB_LED_RED:
	case PM8XXX_ID_RGB_LED_GREEN:
	case PM8XXX_ID_RGB_LED_BLUE:
		led_rgb_set(led, level);
		break;
	default:
		LED_ERR("unknown led id %d", led->id);
		break;
	}

	mutex_unlock(&led->lock);
}

static int first_wled_init = false;
static void pm8xxx_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	struct	pm8xxx_led_data *led;
	int rc;
	u8 val;
	led = container_of(led_cdev, struct pm8xxx_led_data, cdev);
	LED_INFO("%s, id:%d, brightness:%d\n", __func__, led->id, value);

	if (value < LED_OFF || value > led->cdev.max_brightness) {
		LED_ERR("Invalid brightness value exceeds");
		return;
	}
	if (led->id == PM8XXX_ID_WLED) {
#ifdef CONFIG_BACKLIGHT_WLED_CABC
		first_wled_init = true;
#endif
		if (!first_wled_init) {
			if (value == 0)
				return;
			else
				first_wled_init = true;
		}
		
		led_wled_set(led, value);

		
		rc = pm8xxx_readb(led->dev->parent, WLED_MOD_CTRL_REG, &val);
		if (rc) {
			LED_ERR("can't read wled module ctrl"" register rc=%d\n", rc);
			return;
		}

		if (value == 0)
			val = val & ~WLED_EN_MASK;
		else
			val |= WLED_EN_MASK;

		rc = pm8xxx_writeb(led->dev->parent, WLED_MOD_CTRL_REG, val);
		if (rc) {
			LED_ERR("can't write wled module ctrl"" register rc=%d\n", rc);
			return;
		}

		return;
	}
	cancel_delayed_work_sync(&led->fade_delayed_work);

	if (value){
		led_rgb_set(led, 1);
		if (led->pwm_duty_cycles == NULL) {
			pwm_disable(led->pwm_dev);
			rc = pwm_config(led->pwm_dev, 6400 * led->pwm_coefficient / 100, 6400);
			rc = pwm_enable(led->pwm_dev);
		}else{
			if (flag_hold_virtual_key_led == 1) {
				LED_INFO("%s, PWR key control \n", __func__);
				return;
			}
			virtual_key_led_state = value;
			rc = pm8xxx_pwm_lut_config(led->pwm_dev, led->pwm_period_us,
				dutys_array,
				led->pwm_duty_cycles->duty_ms,
				0, 8, 0, 0,
				(PM_PWM_LUT_PAUSE_HI_EN | PM_PWM_LUT_RAMP_UP));

#ifdef CONFIG_PM8038_LED_SYNC
				led_rgb_set(led_data_button_backlight1, 1);
				led_rgb_set(led_data_button_backlight2, 1);
				rc = pm8xxx_pwm_lut_config(led_data_button_backlight1->pwm_dev, led_data_button_backlight1->pwm_period_us,
					dutys_array,
					led_data_button_backlight1->pwm_duty_cycles->duty_ms,
					0, 8, 0, 0,
					(PM_PWM_LUT_PAUSE_HI_EN | PM_PWM_LUT_RAMP_UP));

				rc = pm8xxx_pwm_lut_config(led_data_button_backlight2->pwm_dev, led_data_button_backlight2->pwm_period_us,
					dutys_array,
					led_data_button_backlight2->pwm_duty_cycles->duty_ms,
					0, 8, 0, 0,
					(PM_PWM_LUT_PAUSE_HI_EN | PM_PWM_LUT_RAMP_UP));
				pm8xxx_pwm_lut_enable(led_data_button_backlight1->pwm_dev, 0);
				pm8xxx_pwm_lut_enable(led_data_button_backlight2->pwm_dev, 0);
				pm8xxx_pwm_lut_enable(led_data_button_backlight1->pwm_dev, 1);
				pm8xxx_pwm_lut_enable(led_data_button_backlight2->pwm_dev, 1);
#endif

			pm8xxx_pwm_lut_enable(led->pwm_dev, 0);
			pm8xxx_pwm_lut_enable(led->pwm_dev, 1);
		}
	}else{
		if (led->pwm_duty_cycles == NULL) {
			pwm_disable(led->pwm_dev);
			led_rgb_set(led, 0);
		}else{
			if (flag_hold_virtual_key_led == 1) {
				LED_INFO("%s, PWR key control \n", __func__);
				return;
			}
			virtual_key_led_state = value;
			wake_lock_timeout(&pmic_led_wake_lock, HZ*2);
			rc = pm8xxx_pwm_lut_config(led->pwm_dev, led->pwm_period_us,
					dutys_array + 8,
					led->pwm_duty_cycles->duty_ms,
					0, 8, 0, 0,
					(PM_PWM_LUT_PAUSE_HI_EN | PM_PWM_LUT_RAMP_UP));

#ifdef CONFIG_PM8038_LED_SYNC
			rc = pm8xxx_pwm_lut_config(led_data_button_backlight1->pwm_dev, led_data_button_backlight1->pwm_period_us,
					dutys_array + 8,
					led_data_button_backlight1->pwm_duty_cycles->duty_ms,
					0, 8, 0, 0,
					(PM_PWM_LUT_PAUSE_HI_EN | PM_PWM_LUT_RAMP_UP));
			rc = pm8xxx_pwm_lut_config(led_data_button_backlight2->pwm_dev, led_data_button_backlight2->pwm_period_us,
					dutys_array + 8,
					led_data_button_backlight2->pwm_duty_cycles->duty_ms,
					0, 8, 0, 0,
					(PM_PWM_LUT_PAUSE_HI_EN | PM_PWM_LUT_RAMP_UP));
			pm8xxx_pwm_lut_enable(led_data_button_backlight1->pwm_dev, 0);
			pm8xxx_pwm_lut_enable(led_data_button_backlight2->pwm_dev, 0);
			pm8xxx_pwm_lut_enable(led_data_button_backlight1->pwm_dev, 1);
			pm8xxx_pwm_lut_enable(led_data_button_backlight2->pwm_dev, 1);
#endif

			pm8xxx_pwm_lut_enable(led->pwm_dev, 0);
			pm8xxx_pwm_lut_enable(led->pwm_dev, 1);
			queue_delayed_work(g_led_work_queue,
						&led->fade_delayed_work,
						msecs_to_jiffies(led->pwm_duty_cycles->duty_ms * 8));

		}
	}
}

static int pm8xxx_set_led_mode_and_max_brightness(struct pm8xxx_led_data *led,
		enum pm8xxx_led_modes led_mode, int max_current)
{
	switch (led->id) {
	case PM8XXX_ID_LED_0:
	case PM8XXX_ID_LED_1:
	case PM8XXX_ID_LED_2:
		led->cdev.max_brightness = max_current /
						PM8XXX_ID_LED_CURRENT_FACTOR;
		if (led->cdev.max_brightness > MAX_LC_LED_BRIGHTNESS)
			led->cdev.max_brightness = MAX_LC_LED_BRIGHTNESS;
		led->reg = led_mode;
		break;
	case PM8XXX_ID_LED_KB_LIGHT:
	case PM8XXX_ID_FLASH_LED_0:
	case PM8XXX_ID_FLASH_LED_1:
		led->cdev.max_brightness = max_current /
						PM8XXX_ID_FLASH_CURRENT_FACTOR;
		if (led->cdev.max_brightness > MAX_FLASH_BRIGHTNESS)
			led->cdev.max_brightness = MAX_FLASH_BRIGHTNESS;

		switch (led_mode) {
		case PM8XXX_LED_MODE_PWM1:
		case PM8XXX_LED_MODE_PWM2:
		case PM8XXX_LED_MODE_PWM3:
			led->reg = PM8XXX_FLASH_MODE_PWM;
			break;
		case PM8XXX_LED_MODE_DTEST1:
			led->reg = PM8XXX_FLASH_MODE_DBUS1;
			break;
		case PM8XXX_LED_MODE_DTEST2:
			led->reg = PM8XXX_FLASH_MODE_DBUS2;
			break;
		default:
			led->reg = PM8XXX_LED_MODE_MANUAL;
			break;
		}
		break;
	case PM8XXX_ID_WLED:
		led->cdev.max_brightness = WLED_MAX_LEVEL;
		break;
	case PM8XXX_ID_RGB_LED_RED:
	case PM8XXX_ID_RGB_LED_GREEN:
	case PM8XXX_ID_RGB_LED_BLUE:
		led->cdev.max_brightness = LED_FULL;
		break;
	default:
		LED_ERR("LED Id is invalid");
		return -EINVAL;
	}

	return 0;
}

static enum led_brightness pm8xxx_led_get(struct led_classdev *led_cdev)
{
	struct pm8xxx_led_data *led;

	led = container_of(led_cdev, struct pm8xxx_led_data, cdev);

	return led->cdev.brightness;
}

static int __devinit init_wled(struct pm8xxx_led_data *led)
{
	int rc, i;
	u8 val, num_wled_strings;

	num_wled_strings = led->wled_cfg->num_strings;

	
	if (led->wled_cfg->ovp_val > WLED_OVP_37V) {
		LED_ERR("Invalid ovp value");
		return -EINVAL;
	}

	rc = pm8xxx_readb(led->dev->parent, WLED_OVP_CFG_REG, &val);
	if (rc) {
		LED_ERR("can't read wled ovp config"" register rc=%d\n", rc);
		return rc;
	}

	val = (val & ~WLED_OVP_VAL_MASK) |
		(led->wled_cfg->ovp_val << WLED_OVP_VAL_BIT_SHFT);

	rc = pm8xxx_writeb(led->dev->parent, WLED_OVP_CFG_REG, val);
	if (rc) {
		LED_ERR("can't write wled ovp config"" register rc=%d\n", rc);
		return rc;
	}

	
	if (led->wled_cfg->boost_curr_lim > WLED_CURR_LIMIT_1680mA) {
		LED_ERR("Invalid boost current limit");
		return -EINVAL;
	}

	rc = pm8xxx_readb(led->dev->parent, WLED_BOOST_CFG_REG, &val);
	if (rc) {
		LED_ERR("can't read wled boost config"" register rc=%d\n", rc);
		return rc;
	}

	val = (val & ~WLED_BOOST_LIMIT_MASK) |
		(led->wled_cfg->boost_curr_lim << WLED_BOOST_LIMIT_BIT_SHFT);

	val = (val & ~WLED_OP_FDBCK_MASK) |
		(led->wled_cfg->op_fdbck << WLED_OP_FDBCK_BIT_SHFT);

	val = (val & ~WLED_CNTRL_14_RESERVED_MASK) | WLED_CNTRL_14_RESERVED_SET;

	rc = pm8xxx_writeb(led->dev->parent, WLED_BOOST_CFG_REG, val);
	if (rc) {
		LED_ERR("can't write wled boost config"" register rc=%d\n", rc);
		return rc;
	}

	
	if (led->wled_cfg->cp_select > WLED_CP_SELECT_MAX) {
		LED_ERR("Invalid pole capacitance");
		return -EINVAL;
	}

	rc = pm8xxx_readb(led->dev->parent, WLED_HIGH_POLE_CAP_REG, &val);
	if (rc) {
		LED_ERR("can't read wled high pole"" capacitance register rc=%d\n", rc);
		return rc;
	}

	val = (val & ~WLED_CP_SELECT_MASK) | led->wled_cfg->cp_select;

	rc = pm8xxx_writeb(led->dev->parent, WLED_HIGH_POLE_CAP_REG, val);
	if (rc) {
		LED_ERR("can't write wled high pole"" capacitance register rc=%d\n", rc);
		return rc;
	}

	
	for (i = 0; i < num_wled_strings; i++) {
		rc = pm8xxx_readb(led->dev->parent,
				WLED_MAX_CURR_CFG_REG(i), &val);
		if (rc) {
			LED_ERR("can't read wled max current"" config register rc=%d\n", rc);
			return rc;
		}

		if ((led->wled_cfg->ctrl_delay_us % WLED_CTL_DLY_STEP) ||
			(led->wled_cfg->ctrl_delay_us > WLED_CTL_DLY_MAX)) {
			LED_ERR("Invalid control delay\n");
			return rc;
		}

		val = val / WLED_CTL_DLY_STEP;
		val = (val & ~WLED_CTL_DLY_MASK) |
			(led->wled_cfg->ctrl_delay_us << WLED_CTL_DLY_BIT_SHFT);

		if ((led->max_current > WLED_MAX_CURR)) {
			LED_ERR("Invalid max current\n");
			return -EINVAL;
		}

		val = (val & ~WLED_MAX_CURR_MASK) | led->max_current;

		rc = pm8xxx_writeb(led->dev->parent,
				WLED_MAX_CURR_CFG_REG(i), val);
		if (rc) {
			LED_ERR("can't write wled max current"" config register rc=%d\n", rc);
			return rc;
		}
	}
	
	val = WLED_FSW_CTRL_SET | WLED_MOD_CLK_CTRL_SET;
	rc = pm8xxx_writeb(led->dev->parent, WLED_FSW_REG, val);
	if (rc) {
		LED_ERR("can't write wled frequency control register rc=%d\n", rc);
		return rc;
	}

	val = WLED_CNTRL_15_RESERVED_SET;
	rc = pm8xxx_writeb(led->dev->parent, WLED_RESERVED_REG, val);
	if (rc) {
		LED_ERR("can't write wled reserved register rc=%d\n", rc);
		return rc;
	}

#ifdef CONFIG_BACKLIGHT_WLED_CABC
	
	val = 0x7F;
	rc = pm8xxx_writeb(led->dev->parent, WLED_BRIGHTNESS_CNTL_REG1(0), val);
	if (rc) {
		LED_ERR("can't write wled brightness ctrl"" register1 rc=%d\n", rc);
		return rc;
	}

	val = 0xFF;
	rc = pm8xxx_writeb(led->dev->parent, WLED_BRIGHTNESS_CNTL_REG2(0), val);
	if (rc) {
		LED_ERR("can't write wled brightness ctrl"" register2 rc=%d\n", rc);
		return rc;
	}

	val = WLED_SYNC_CABC_ENABLED;
	rc = pm8xxx_writeb(led->dev->parent, WLED_SYNC_REG, val);
	if (rc) {
		dev_err(led->dev->parent,
			"can't read wled sync register rc=%d\n", rc);
		return rc;
	}
#endif
	
	rc = pm8xxx_readb(led->dev->parent, WLED_MOD_CTRL_REG, &val);
	if (rc) {
		LED_ERR("can't read wled module ctrl"" register rc=%d\n", rc);
		return rc;
	}
	if (led->wled_cfg->dig_mod_gen_en)
		val = (val & ~WLED_DIG_MOD_GEN_MASK) | WLED_DIG_MOD_GEN_SET;

	if (led->wled_cfg->cs_out_en)
		val = (val & ~WLED_CS_OUT_MASK) | WLED_CS_OUT_SET;

	
	if (board_mfg_mode() == 4 || board_mfg_mode() == 5)
		val = val & ~WLED_EN_MASK;
	else
		val |= WLED_EN_MASK;

#if defined(CONFIG_MACH_DUMMY)
	
	val = val & ~WLED_EN_MASK;
#endif
	rc = pm8xxx_writeb(led->dev->parent, WLED_MOD_CTRL_REG, val);
	if (rc) {
		LED_ERR("can't write wled module ctrl"" register rc=%d\n", rc);
		return rc;
	}
	led->wled_mod_ctrl_val = val;

	
	wled_dump_regs(led);

	return 0;
}

static int __devinit get_init_value(struct pm8xxx_led_data *led, u8 *val)
{
	int rc, offset;
	u16 addr;

	switch (led->id) {
	case PM8XXX_ID_LED_KB_LIGHT:
		addr = SSBI_REG_ADDR_DRV_KEYPAD;
		break;
	case PM8XXX_ID_LED_0:
	case PM8XXX_ID_LED_1:
	case PM8XXX_ID_LED_2:
		offset = PM8XXX_LED_OFFSET(led->id);
		addr = SSBI_REG_ADDR_LED_CTRL(offset);
		break;
	case PM8XXX_ID_FLASH_LED_0:
		addr = SSBI_REG_ADDR_FLASH_DRV0;
		break;
	case PM8XXX_ID_FLASH_LED_1:
		addr = SSBI_REG_ADDR_FLASH_DRV1;
		break;
	case PM8XXX_ID_WLED:
		rc = init_wled(led);
		if (rc)
			LED_ERR("can't initialize wled rc=%d\n", rc);
		return rc;
	case PM8XXX_ID_RGB_LED_RED:
	case PM8XXX_ID_RGB_LED_GREEN:
	case PM8XXX_ID_RGB_LED_BLUE:
		return 0;
	default:
		LED_ERR("unknown led id %d", led->id);
		return -EINVAL;
	}

	rc = pm8xxx_readb(led->dev->parent, addr, val);
	if (rc)
		LED_ERR("can't get led(%d) level rc=%d\n", led->id, rc);

	return rc;
}

static int pm8xxx_led_pwm_configure(struct pm8xxx_led_data *led)
{
	int start_idx, idx_len;

	led->pwm_dev = pwm_request(led->pwm_channel,
					led->cdev.name);

	if (IS_ERR_OR_NULL(led->pwm_dev)) {
		LED_ERR("could not acquire PWM Channel %d, "
			"error %ld\n", led->pwm_channel,
			PTR_ERR(led->pwm_dev));
		led->pwm_dev = NULL;
		return -ENODEV;
	}

	if (led->pwm_duty_cycles != NULL) {
		start_idx = led->pwm_duty_cycles->start_idx;
		idx_len = led->pwm_duty_cycles->num_duty_pcts;
		if (idx_len >= PM_PWM_LUT_SIZE && start_idx) {
			LED_ERR("Wrong LUT size or index\n");
			return -EINVAL;
		}
		if ((start_idx + idx_len) > PM_PWM_LUT_SIZE) {
			LED_ERR("Exceed LUT limit\n");
			return -EINVAL;
		}
	}

	return 0;
}

static void led_fade_do_work(struct work_struct *work)
{
	struct pm8xxx_led_data *led;
	led = container_of(work, struct pm8xxx_led_data, fade_delayed_work.work);
	led_rgb_set(led, 0);

#ifdef CONFIG_PM8038_LED_SYNC
	led_rgb_set(led_data_button_backlight1, 0);
	led_rgb_set(led_data_button_backlight2, 0);
#endif

}
void pm8xxx_led_current_set_for_key(int brightness_key)
{
	LED_INFO("%s brightness_key: %d\n", __func__,brightness_key);

	if (brightness_key) {
		flag_hold_virtual_key_led = 1;
		led_rgb_set(for_key_led_data, 1);
		pwm_disable(for_key_led_data->pwm_dev);
		pwm_config(for_key_led_data->pwm_dev, 320000, 640000);
		pwm_enable(for_key_led_data->pwm_dev);

#ifdef CONFIG_PM8038_LED_SYNC
		led_rgb_set(led_data_button_backlight1, 1);
		pwm_disable(led_data_button_backlight1->pwm_dev);
		pwm_config(led_data_button_backlight1->pwm_dev, 320000, 640000);
		pwm_enable(led_data_button_backlight1->pwm_dev);
		led_rgb_set(led_data_button_backlight2, 1);
		pwm_disable(led_data_button_backlight2->pwm_dev);
		pwm_config(led_data_button_backlight2->pwm_dev, 320000, 640000);
		pwm_enable(led_data_button_backlight2->pwm_dev);
#endif

	} else {
		pwm_disable(for_key_led_data->pwm_dev);
		led_rgb_set(for_key_led_data, 0);

#ifdef CONFIG_PM8038_LED_SYNC
		pwm_disable(led_data_button_backlight1->pwm_dev);
		led_rgb_set(led_data_button_backlight1, 0);
		pwm_disable(led_data_button_backlight2->pwm_dev);
		led_rgb_set(led_data_button_backlight2, 0);
#endif

		if (virtual_key_led_state != 0){
			LED_INFO("virtual_key_led_state: %d\n", virtual_key_led_state);
			led_rgb_set(for_key_led_data, 1);
			pwm_disable(for_key_led_data->pwm_dev);
			pwm_config(for_key_led_data->pwm_dev, 64000, 64000);
			pwm_enable(for_key_led_data->pwm_dev);

#ifdef CONFIG_PM8038_LED_SYNC
			led_rgb_set(led_data_button_backlight1, 1);
			pwm_disable(led_data_button_backlight1->pwm_dev);
			pwm_config(led_data_button_backlight1->pwm_dev, 64000, 64000);
			pwm_enable(led_data_button_backlight1->pwm_dev);
			led_rgb_set(led_data_button_backlight2, 1);
			pwm_disable(led_data_button_backlight2->pwm_dev);
			pwm_config(led_data_button_backlight2->pwm_dev, 64000, 64000);
			pwm_enable(led_data_button_backlight2->pwm_dev);
#endif

		}
		flag_hold_virtual_key_led = 0;

	}
}



static void led_work_func(struct work_struct *work)
{
	struct pm8xxx_led_data *ldata;

	ldata = container_of(work, struct pm8xxx_led_data, led_work);
	pwm_disable(ldata->pwm_dev);
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
	pwm_disable(led->pwm_dev);
	pwm_config(led->pwm_dev, led->duty_time_ms * 1000, led->period_us);
	pwm_enable(led->pwm_dev);
}
static ssize_t pm8xxx_led_blink_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", current_blink);
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
	current_blink = val;

	led_cdev = (struct led_classdev *) dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct pm8xxx_led_data, cdev);

	LED_INFO("%s: id %d blink %d\n", __func__, ldata->id, val);

	switch (val) {
	case BLINK_STOP:
		if (ldata->gpio_status_switch != NULL)
			ldata->gpio_status_switch(0);
		pwm_disable(ldata->pwm_dev);
		break;
	case BLINK_UNCHANGE:
		pwm_disable(ldata->pwm_dev);
		if (led_cdev->brightness) {
			if (ldata->gpio_status_switch != NULL)
				ldata->gpio_status_switch(1);
			pwm_disable(ldata->pwm_dev);
			pwm_config(ldata->pwm_dev, 6400 * ldata->pwm_coefficient / 100, 6400);
			pwm_enable(ldata->pwm_dev);
		} else {
			pwm_disable(ldata->pwm_dev);
			if (ldata->gpio_status_switch != NULL)
				ldata->gpio_status_switch(0);
		}
		break;
	case BLINK_64MS_PER_2SEC:
		if (ldata->gpio_status_switch != NULL)
			ldata->gpio_status_switch(1);
		pwm_disable(ldata->pwm_dev);
		pwm_config(ldata->pwm_dev, 64000, 2000000);
		pwm_enable(ldata->pwm_dev);
		break;
	case BLINK_64MS_ON_310MS_PER_2SEC:
		cancel_delayed_work_sync(&ldata->blink_delayed_work);
		pwm_disable(ldata->pwm_dev);
		ldata->duty_time_ms = 64;
		ldata->period_us = 2000000;
		queue_delayed_work(g_led_work_queue, &ldata->blink_delayed_work,
				   msecs_to_jiffies(310));
		break;
	case BLINK_64MS_ON_2SEC_PER_2SEC:
		cancel_delayed_work_sync(&ldata->blink_delayed_work);
		pwm_disable(ldata->pwm_dev);
		ldata->duty_time_ms = 64;
		ldata->period_us = 2000000;
		queue_delayed_work(g_led_work_queue, &ldata->blink_delayed_work,
				   msecs_to_jiffies(1000));
		break;
	case BLINK_1SEC_PER_2SEC:
		pwm_disable(ldata->pwm_dev);
		pwm_config(ldata->pwm_dev, 1000000, 2000000);
		pwm_enable(ldata->pwm_dev);
		break;
	case key_blink_on:
		pm8xxx_led_current_set_for_key(1);
		break;
	case key_blink_off:
		pm8xxx_led_current_set_for_key(0);
		break;
	default:
		LED_ERR("%s: id %d did not support blink type %d\n", __func__, ldata->id, val);
		return -EINVAL;
	}

	return count;
}
static DEVICE_ATTR(blink, 0644, pm8xxx_led_blink_show, pm8xxx_led_blink_store);

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
static ssize_t pm8xxx_led_pwm_coefficient_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct led_classdev *led_cdev;
	struct pm8xxx_led_data *ldata;

	led_cdev = (struct led_classdev *) dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct pm8xxx_led_data, cdev);

	return sprintf(buf, "%d\n", ldata->pwm_coefficient);
}

static ssize_t pm8xxx_led_pwm_coefficient_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	int pwm_coefficient1 = 0;
	struct led_classdev *led_cdev;
	struct pm8xxx_led_data *ldata;

	sscanf(buf, "%d", &pwm_coefficient1);
	if ((pwm_coefficient1 < 0) || (pwm_coefficient1 > 100)) {
		LED_INFO("%s: pwm_coefficient = %d, out of range.\n",
			__func__, pwm_coefficient1);
		return -EINVAL;
	}

	led_cdev = (struct led_classdev *)dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct pm8xxx_led_data, cdev);

	LED_INFO("%s: pwm_coefficient %d\n", __func__, pwm_coefficient1);

	ldata->pwm_coefficient = pwm_coefficient1;

	return count;
}
static DEVICE_ATTR(pwm_coefficient, 0644, pm8xxx_led_pwm_coefficient_show, pm8xxx_led_pwm_coefficient_store);

static ssize_t pm8xxx_led_lut_coefficient_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct led_classdev *led_cdev;
	struct pm8xxx_led_data *ldata;

	led_cdev = (struct led_classdev *) dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct pm8xxx_led_data, cdev);

	return sprintf(buf, "%d\n", lut_coefficient);
}

static ssize_t pm8xxx_led_lut_coefficient_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	int lut_coefficient1 = 0;
	int i;
	struct led_classdev *led_cdev;
	struct pm8xxx_led_data *ldata;

	sscanf(buf, "%d", &lut_coefficient1);
	if (lut_coefficient1 < 0)
		return -EINVAL;

	led_cdev = (struct led_classdev *)dev_get_drvdata(dev);
	ldata = container_of(led_cdev, struct pm8xxx_led_data, cdev);

	LED_INFO("%s: lut_coefficient %d\n", __func__, lut_coefficient1);
	for (i = 0; i < 16; i++) {
		dutys_array[i] = ldata->pwm_duty_cycles->duty_pcts[i] * lut_coefficient1 / 100;
	}
	lut_coefficient = lut_coefficient1;
	return count;
}
static DEVICE_ATTR(lut_coefficient, 0644, pm8xxx_led_lut_coefficient_show, pm8xxx_led_lut_coefficient_store);

static void config_lut(struct pm8xxx_led_data *led_dat)
{
	int i;

	LED_INFO("%s: lut_coefficient %d\n", __func__, lut_coefficient);
	for (i = 0; i < 16; i++) {
		dutys_array[i] = led_dat->pwm_duty_cycles->duty_pcts[i] * lut_coefficient / 100;
		LED_INFO("%s: %d\n", __func__, dutys_array[i]);
	}
}

static int __devinit pm8xxx_led_probe(struct platform_device *pdev)
{
	const struct pm8xxx_led_platform_data *pdata = pdev->dev.platform_data;
	const struct led_platform_data *pcore_data;
	struct led_info *curr_led;
	struct pm8xxx_led_data *led, *led_dat;
	struct pm8xxx_led_config *led_cfg;
	enum pm8xxx_version version;
	bool found = false;
	int i, j, k;
	int rc = -ENOMEM;

	if (pdata == NULL) {
		LED_ERR("platform data not supplied\n");
		return -EINVAL;
	}

	pcore_data = pdata->led_core;

	if (pcore_data->num_leds != pdata->num_configs) {
		LED_ERR("#no. of led configs and #no. of led""entries are not equal\n");
		return -EINVAL;
	}

	led = kcalloc(pcore_data->num_leds, sizeof(*led), GFP_KERNEL);
	if (led == NULL) {
		LED_ERR("failed to alloc memory\n");
		return -ENOMEM;
	}
	wake_lock_init(&pmic_led_wake_lock, WAKE_LOCK_SUSPEND, "pmic_led");
	g_led_work_queue = create_workqueue("pm8xxx-led");
	if (g_led_work_queue == NULL) {
		LED_ERR("failed to create workqueue\n");
		goto err_create_work_queue;
	}

	for (i = 0; i < pcore_data->num_leds; i++) {
		curr_led	= &pcore_data->leds[i];
		led_dat		= &led[i];
		led_cfg		= &pdata->configs[i];

		led_dat->id     = led_cfg->id;
		led_dat->pwm_channel = led_cfg->pwm_channel;
		led_dat->pwm_period_us = led_cfg->pwm_period_us;
		led_dat->pwm_duty_cycles = led_cfg->pwm_duty_cycles;
		led_dat->wled_cfg = led_cfg->wled_cfg;
		led_dat->max_current = led_cfg->max_current;
		if( led_cfg->pwm_coefficient > 0 )
			led_dat->pwm_coefficient	= led_cfg->pwm_coefficient;
		else
			led_dat->pwm_coefficient	= 100;

		if (led_cfg->pwm_duty_cycles != NULL) {
			for (j = 0; j < 64; j++)
				dutys_array[j] = led_dat->pwm_duty_cycles->duty_pcts[j];
		}

		
		if (led_cfg->lut_coefficient > 0) {
			lut_coefficient	= led_cfg->lut_coefficient;
			config_lut(led_dat);
		}

		if (!((led_dat->id >= PM8XXX_ID_LED_KB_LIGHT) &&
				(led_dat->id < PM8XXX_ID_MAX))) {
			LED_ERR("invalid LED ID(%d) specified\n",
				led_dat->id);
			rc = -EINVAL;
			goto fail_id_check;

		}

		found = false;
		version = pm8xxx_get_version(pdev->dev.parent);
		for (j = 0; j < ARRAY_SIZE(led_map); j++) {
			if (version == led_map[j].version
			&& (led_map[j].supported & (1 << led_dat->id))) {
				found = true;
				break;
			}
		}

		if (!found) {
			LED_ERR("invalid LED ID(%d) specified\n",
				led_dat->id);
			rc = -EINVAL;
			goto fail_id_check;
		}

		led_dat->cdev.name		= curr_led->name;
		led_dat->cdev.default_trigger   = curr_led->default_trigger;
		led_dat->cdev.brightness_set    = pm8xxx_led_set;
		led_dat->cdev.brightness_get    = pm8xxx_led_get;
		led_dat->cdev.brightness	= LED_OFF;
		led_dat->cdev.flags		= curr_led->flags;
		led_dat->dev			= &pdev->dev;

		rc =  get_init_value(led_dat, &led_dat->reg);
		if (rc < 0)
			goto fail_id_check;

		rc = pm8xxx_set_led_mode_and_max_brightness(led_dat,
					led_cfg->mode, led_cfg->max_current);
		if (rc < 0)
			goto fail_id_check;

		mutex_init(&led_dat->lock);
		

		rc = led_classdev_register(&pdev->dev, &led_dat->cdev);
		if (rc) {
			LED_ERR("unable to register led %d,rc=%d\n", led_dat->id, rc);
			goto err_register_led_cdev;
		}


		
		if (led_cfg->default_state)
			led->cdev.brightness = led_dat->cdev.max_brightness;
		else
			led->cdev.brightness = LED_OFF;

		if (led_cfg->mode != PM8XXX_LED_MODE_MANUAL) {
			if (led_dat->id != PM8XXX_ID_RGB_LED_RED &&
				led_dat->id != PM8XXX_ID_RGB_LED_GREEN &&
				led_dat->id != PM8XXX_ID_RGB_LED_BLUE)
				__pm8xxx_led_work(led_dat,
					led_dat->cdev.max_brightness);

			if (led_dat->pwm_channel != -1) {
				led_dat->cdev.max_brightness = LED_FULL;
				rc = pm8xxx_led_pwm_configure(led_dat);
				if (rc) {
					LED_ERR("failed to ""configure LED, error: %d\n", rc);
					goto fail_id_check;
				}
			}
		} else {
			__pm8xxx_led_work(led_dat, led->cdev.brightness);
		}

		if (led_dat->pwm_duty_cycles != NULL) {
			INIT_DELAYED_WORK(&led[i].fade_delayed_work, led_fade_do_work);

			rc = device_create_file(led_dat->cdev.dev, &dev_attr_lut_coefficient);
			if (rc < 0) {
				LED_ERR("%s: Failed to create %d attr lut_coefficient\n", __func__, i);
				goto err_register_attr_lut_coefficient;
			}
		} else {
			INIT_DELAYED_WORK(&led[i].blink_delayed_work, led_blink_do_work);
		}
		if (!strcmp(led_dat->cdev.name, "amber") || !strcmp(led_dat->cdev.name, "green")) {
			rc = device_create_file(led_dat->cdev.dev, &dev_attr_pwm_coefficient);
			if (rc < 0) {
				LED_ERR("%s: Failed to create %d attr pwm_coefficient\n", __func__, i);
				goto err_register_attr_pwm_coefficient;
			}
		}
		rc = device_create_file(led_dat->cdev.dev, &dev_attr_blink);
		if (rc < 0) {
			LED_ERR("%s: Failed to create %d attr blink\n", __func__, i);
			goto err_register_attr_blink;
		}
		rc = device_create_file(led_dat->cdev.dev, &dev_attr_off_timer);
		if (rc < 0) {
			LED_ERR("%s: Failed to create %d attr off timer\n", __func__, i);
			goto err_register_attr_off_timer;
		}
		alarm_init(&led[i].led_alarm, ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP, led_alarm_handler);
		INIT_WORK(&led[i].led_work, led_work_func); 
		if (!strcmp(led_dat->cdev.name, "button-backlight")) {
			for_key_led_data = led_dat;
		}

#ifdef CONFIG_PM8038_LED_SYNC
		if (!strcmp(led_dat->cdev.name, "button-backlight1")) {
			led_data_button_backlight1 = led_dat;
		}
		if (!strcmp(led_dat->cdev.name, "button-backlight2")) {
			led_data_button_backlight2 = led_dat;
		}

#endif
	}
	platform_set_drvdata(pdev, led);

	return 0;
err_register_attr_off_timer:
	if (i > 0) {
		for (k = i - 1; k >= 0; k--) {
			device_remove_file(led[k].cdev.dev, &dev_attr_off_timer);

		}
	}
err_register_attr_blink:
	if (i > 0) {
		for (k = i - 1; k >= 0; k--) {
			device_remove_file(led[k].cdev.dev, &dev_attr_blink);
		}
	}
err_register_attr_pwm_coefficient:
	if (i > 0) {
		for (k = i - 1; k >= 0; k--) {
			device_remove_file(led[k].cdev.dev, &dev_attr_pwm_coefficient);
		}
	}
err_register_attr_lut_coefficient:
	if (i > 0) {
		for (k = i - 1; k >= 0; k--) {
			if (led_dat->pwm_duty_cycles != NULL)
				device_remove_file(led[k].cdev.dev, &dev_attr_lut_coefficient);
		}
	}

fail_id_check:
err_register_led_cdev:
	if (i > 0) {
		for (k = i - 1; k >= 0; k--) {
			mutex_destroy(&led[k].lock);
			led_classdev_unregister(&led[k].cdev);
			if (led[k].pwm_dev != NULL)
				pwm_free(led[k].pwm_dev);
		}
	}
	destroy_workqueue(g_led_work_queue);

err_create_work_queue:
	kfree(led);
	wake_lock_destroy(&pmic_led_wake_lock);

	return rc;
}

static int __devexit pm8xxx_led_remove(struct platform_device *pdev)
{
	int i;
	const struct led_platform_data *pdata =
				pdev->dev.platform_data;
	struct pm8xxx_led_data *led = platform_get_drvdata(pdev);

	for (i = 0; i < pdata->num_leds; i++) {
		
		mutex_destroy(&led[i].lock);
		led_classdev_unregister(&led[i].cdev);
		if (led[i].pwm_dev != NULL)
			pwm_free(led[i].pwm_dev);

		device_remove_file(led[i].cdev.dev, &dev_attr_blink);
		device_remove_file(led[i].cdev.dev, &dev_attr_off_timer);
	}
	wake_lock_destroy(&pmic_led_wake_lock);
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

MODULE_DESCRIPTION("PM8038 LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:pm8038-led");
