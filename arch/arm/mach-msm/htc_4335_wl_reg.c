/* arch/arm/mach-msm/htc_BCM4335_wl_reg.c
 *
 * Copyright (C) 2012 HTC, Inc.
 * Author: assd bt <assd_bt@htc.com>
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


#include <linux/gpio.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <mach/htc_4335_wl_reg.h>

#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)

static int htc_BCM4335_wl_reg_pin = -2;
static int htc_BCM4335_wl_reg_state_wifi;
static int htc_BCM4335_wl_reg_state_bt;
static DEFINE_MUTEX(htc_w_b_mutex);



int set_BCM4335_wl_reg_onoff(int on)
{
	if (on) {
		printk(KERN_DEBUG "EN BCM4335 WL REG\n");
		gpio_set_value(PM8921_GPIO_PM_TO_SYS(htc_BCM4335_wl_reg_pin), 1);
	} else {
		printk(KERN_DEBUG "DIS BCM4335 WL REG\n");
		gpio_set_value(PM8921_GPIO_PM_TO_SYS(htc_BCM4335_wl_reg_pin), 0);
	}
	return 0;
}

int htc_BCM4335_wl_reg_ctl(int on, int id)
{
	int err = 0;

	printk(KERN_DEBUG "%s ON=%d, ID=%d\n", __func__, on, id);

	if (htc_BCM4335_wl_reg_pin < 0) {
		printk(KERN_DEBUG "== ERR WL REG PIN=%d ==\n",
			htc_BCM4335_wl_reg_pin);
		return htc_BCM4335_wl_reg_pin;
	}

	mutex_lock(&htc_w_b_mutex);
	if (on) {
		if ((BCM4335_WL_REG_OFF == htc_BCM4335_wl_reg_state_wifi)
			&& (BCM4335_WL_REG_OFF == htc_BCM4335_wl_reg_state_bt)) {

			err = set_BCM4335_wl_reg_onoff(BCM4335_WL_REG_ON);

			if (err) {
				mutex_unlock(&htc_w_b_mutex);
				return err;
			}
		} else if ((id == ID_WIFI) && (	BCM4335_WL_REG_ON == htc_BCM4335_wl_reg_state_bt)) {
			printk(KERN_ERR "[WLAN] Try to pull WL_REG off and on to reset Wi-Fi chip\n");
			err = set_BCM4335_wl_reg_onoff(BCM4335_WL_REG_OFF);
			if (err) {
				printk(KERN_ERR "[WLAN] Failed to pull WL_REG off\n");
				mutex_unlock(&htc_w_b_mutex);
				return err;
			}
			mdelay(1);
			err = set_BCM4335_wl_reg_onoff(BCM4335_WL_REG_ON);
			if (err) {
				printk(KERN_ERR "[WLAN] Failed to pull WL_REG on\n");
				mutex_unlock(&htc_w_b_mutex);
				return err;
			}
		}

		if (id == ID_BT)
			htc_BCM4335_wl_reg_state_bt = BCM4335_WL_REG_ON;
		else
			htc_BCM4335_wl_reg_state_wifi = BCM4335_WL_REG_ON;
	} else {
		if (((id == ID_BT) && (BCM4335_WL_REG_OFF == htc_BCM4335_wl_reg_state_wifi))
			|| ((id == ID_WIFI)
			&& (BCM4335_WL_REG_OFF == htc_BCM4335_wl_reg_state_bt))) {

			err = set_BCM4335_wl_reg_onoff(BCM4335_WL_REG_OFF);

			if (err) {
				mutex_unlock(&htc_w_b_mutex);
				return err;
			}
		} else {
			printk(KERN_DEBUG "KEEP BCM4335 WL REG ALIVE\n");
		}

		if (id)
			htc_BCM4335_wl_reg_state_bt = BCM4335_WL_REG_OFF;
		else
			htc_BCM4335_wl_reg_state_wifi = BCM4335_WL_REG_OFF;
	}
	mutex_unlock(&htc_w_b_mutex);

	printk(KERN_DEBUG "%s ON=%d, ID=%d DONE\n", __func__, on, id);

	return 0;
}

int htc_BCM4335_wl_reg_init(int BCM4335_wl_reg_pin)
{
	htc_BCM4335_wl_reg_pin = BCM4335_wl_reg_pin;

	htc_BCM4335_wl_reg_state_wifi = BCM4335_WL_REG_OFF;
	htc_BCM4335_wl_reg_state_bt = BCM4335_WL_REG_OFF;

	printk(KERN_DEBUG "%s, pin=%d\n", __func__, htc_BCM4335_wl_reg_pin);

	return true;
}
