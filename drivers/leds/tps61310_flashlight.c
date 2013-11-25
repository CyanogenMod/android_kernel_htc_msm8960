/* drivers/i2c/chips/tps61310_flashlight.c
 *
 * Copyright (C) 2008-2009 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <mach/msm_iomap.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/leds.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/htc_flashlight.h>
#include <linux/module.h>

#define FLT_DBG_LOG(fmt, ...) \
		printk(KERN_DEBUG "[FLT]TPS " fmt, ##__VA_ARGS__)
#define FLT_INFO_LOG(fmt, ...) \
		printk(KERN_INFO "[FLT]TPS " fmt, ##__VA_ARGS__)
#define FLT_ERR_LOG(fmt, ...) \
		printk(KERN_ERR "[FLT][ERR]TPS " fmt, ##__VA_ARGS__)

#define TPS61310_RETRY_COUNT 10

struct tps61310_data {
	struct led_classdev 		fl_lcdev;
	struct early_suspend		fl_early_suspend;
	enum flashlight_mode_flags 	mode_status;
	uint32_t			flash_sw_timeout;
	struct mutex 			tps61310_data_mutex;
	uint32_t			strb0;
	uint32_t			strb1;
	uint32_t			reset;
	uint8_t 			led_count;
	uint8_t 			mode_pin_suspend_state_low;
	uint8_t				enable_FLT_1500mA;
	uint8_t				disable_tx_mask;
	uint32_t			power_save;
	uint32_t			power_save_2;
};

static struct i2c_client *this_client;
static struct tps61310_data *this_tps61310;
struct delayed_work tps61310_delayed_work;
static struct workqueue_struct *tps61310_work_queue;
static struct mutex tps61310_mutex;

static int switch_state = 1;
static int retry = 0;
static int reg_init_fail = 0;

static int tps61310_i2c_command(uint8_t, uint8_t);

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
	FLT_INFO_LOG("%s: %d\n",__func__,switch_status);
	switch_state = switch_status;
	return size;
}

static DEVICE_ATTR(function_switch, S_IRUGO | S_IWUSR, switch_show, switch_store);

static int TPS61310_I2C_TxData(char *txData, int length)
{
	uint8_t loop_i;
	struct i2c_msg msg[] = {
		{
		 .addr = this_client->addr,
		 .flags = 0,
		 .len = length,
		 .buf = txData,
		 },
	};

	for (loop_i = 0; loop_i < TPS61310_RETRY_COUNT; loop_i++) {
		if (i2c_transfer(this_client->adapter, msg, 1) > 0)
			break;

		mdelay(10);
	}

	if (loop_i >= TPS61310_RETRY_COUNT) {
		FLT_ERR_LOG("%s retry over %d\n", __func__,
							TPS61310_RETRY_COUNT);
		return -EIO;
	}

	return 0;
}

static int tps61310_i2c_command(uint8_t address, uint8_t data)
{
	uint8_t buffer[2];
	int ret;
	int err = 0;

	buffer[0] = address;
	buffer[1] = data;
	ret = TPS61310_I2C_TxData(buffer, 2);
	if (ret < 0) {
		FLT_ERR_LOG("%s error\n", __func__);
		if (this_tps61310->reset) {
			FLT_INFO_LOG("reset register");
			gpio_set_value_cansleep(this_tps61310->reset, 0);
			mdelay(10);
			gpio_set_value_cansleep(this_tps61310->reset, 1);
			if (address!=0x07 && address!=0x04) {
				if (this_tps61310->enable_FLT_1500mA) {
					err |= tps61310_i2c_command(0x07, 0x46);
					err |= tps61310_i2c_command(0x04, 0x10);
				} else {
					
					err |= tps61310_i2c_command(0x07, 0xF6);
				}
				if (err)
					reg_init_fail++;
			} else {
				reg_init_fail++;
			}
		}
		return ret;
	}
	return 0;
}

static int flashlight_turn_off(void)
{
	int status;
	FLT_INFO_LOG("%s\n", __func__);
	gpio_set_value_cansleep(this_tps61310->strb0, 0);
	gpio_set_value_cansleep(this_tps61310->strb1, 1);
	tps61310_i2c_command(0x02, 0x08);
	tps61310_i2c_command(0x01, 0x00);
	FLT_INFO_LOG("%s %d\n", __func__,this_tps61310->mode_status);
	if (this_tps61310->power_save) {
		status = this_tps61310->mode_status;
		if (status == 2 || (status >= 10 && status <=16)) {
			FLT_INFO_LOG("Disable power saving\n");
			gpio_set_value_cansleep(this_tps61310->power_save, 0);
		} else if (status == FL_MODE_PRE_FLASH) {
			FLT_INFO_LOG("Enable power saving\n");
			gpio_set_value_cansleep(this_tps61310->power_save, 1);
		}
	}
	if (this_tps61310->power_save_2) {
		status = this_tps61310->mode_status;
		if (status == 2 || (status >= 10 && status <=16)) {
			FLT_INFO_LOG("Disable power saving\n");
			gpio_set_value_cansleep(this_tps61310->power_save_2, 0);
		} else if (status == FL_MODE_PRE_FLASH) {
			FLT_INFO_LOG("Enable power saving\n");
			gpio_set_value_cansleep(this_tps61310->power_save_2, 1);
		}
	}
	this_tps61310->mode_status = FL_MODE_OFF;
	return 0;
}

void retry_flashlight_control(int err, int mode)
{
	if (err && !retry) {
		FLT_INFO_LOG("%s error once\n", __func__);
		retry++;
		mutex_unlock(&tps61310_mutex);
		tps61310_flashlight_control(mode);
		mutex_lock(&tps61310_mutex);
	} else if(err) {
		FLT_INFO_LOG("%s error twice\n", __func__);
		retry = 0;
	}
}

int tps61310_flashlight_control(int mode)
{
	int ret = 0;
	int err = 0;

	mutex_lock(&tps61310_mutex);
	if (this_tps61310->reset && reg_init_fail) {
		reg_init_fail = 0;
		if (this_tps61310->enable_FLT_1500mA) {
			err |= tps61310_i2c_command(0x07, 0x46);
			err |= tps61310_i2c_command(0x04, 0x10);
		} else {
			
			err |= tps61310_i2c_command(0x07, 0xF6);
		}
	}
	if (err) {
		FLT_ERR_LOG("%s error init register\n", __func__);
		reg_init_fail = 0;
		mutex_unlock(&tps61310_mutex);
		return -err;
	}
	if (this_tps61310->led_count == 1) {
		if (this_tps61310->enable_FLT_1500mA) {
#if defined CONFIG_FLASHLIGHT_1500mA
			switch (mode) {
			case FL_MODE_OFF:
				flashlight_turn_off();
			break;
			case FL_MODE_FLASH:
				FLT_INFO_LOG("flash 1.5A\n");
				tps61310_i2c_command(0x05, 0x6F);
				tps61310_i2c_command(0x00, 0x00);
				tps61310_i2c_command(0x01, 0x9E);
				tps61310_i2c_command(0x02, 0x8F);
				gpio_set_value_cansleep(this_tps61310->strb1, 0);
				gpio_set_value_cansleep(this_tps61310->strb0, 1);
				queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
						   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
			break;
			case FL_MODE_FLASH_LEVEL1:
					tps61310_i2c_command(0x05, 0x6A);
					tps61310_i2c_command(0x00, 0x00);
					tps61310_i2c_command(0x01, 0x86);
					gpio_set_value_cansleep(this_tps61310->strb1, 0);
					gpio_set_value_cansleep(this_tps61310->strb0, 1);
					queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
							   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
			break;
			case FL_MODE_FLASH_LEVEL2:
					tps61310_i2c_command(0x05, 0x6A);
					tps61310_i2c_command(0x00, 0x00);
					tps61310_i2c_command(0x01, 0x88);
					gpio_set_value_cansleep(this_tps61310->strb1, 0);
					gpio_set_value_cansleep(this_tps61310->strb0, 1);
					queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
							   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
			break;
			case FL_MODE_FLASH_LEVEL3:
					tps61310_i2c_command(0x05, 0x6A);
					tps61310_i2c_command(0x00, 0x00);
					tps61310_i2c_command(0x01, 0x8C);
					gpio_set_value_cansleep(this_tps61310->strb1, 0);
					gpio_set_value_cansleep(this_tps61310->strb0, 1);
					queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
							   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
			break;
			case FL_MODE_FLASH_LEVEL4:
					tps61310_i2c_command(0x05, 0x6A);
					tps61310_i2c_command(0x00, 0x00);
					tps61310_i2c_command(0x01, 0x90);
					gpio_set_value_cansleep(this_tps61310->strb1, 0);
					gpio_set_value_cansleep(this_tps61310->strb0, 1);
					queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
							   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
			break;
			case FL_MODE_FLASH_LEVEL5:
					tps61310_i2c_command(0x05, 0x6A);
					tps61310_i2c_command(0x00, 0x00);
					tps61310_i2c_command(0x01, 0x94);
					gpio_set_value_cansleep(this_tps61310->strb1, 0);
					gpio_set_value_cansleep(this_tps61310->strb0, 1);
					queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
							   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
			break;
			case FL_MODE_FLASH_LEVEL6:
					tps61310_i2c_command(0x05, 0x6A);
					tps61310_i2c_command(0x00, 0x00);
					tps61310_i2c_command(0x01, 0x98);
					gpio_set_value_cansleep(this_tps61310->strb1, 0);
					gpio_set_value_cansleep(this_tps61310->strb0, 1);
					queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
							   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
			break;
			case FL_MODE_FLASH_LEVEL7:
					tps61310_i2c_command(0x05, 0x6A);
					tps61310_i2c_command(0x00, 0x00);
					tps61310_i2c_command(0x01, 0x9C);
					gpio_set_value_cansleep(this_tps61310->strb1, 0);
					gpio_set_value_cansleep(this_tps61310->strb0, 1);
					queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
							   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
			break;
			case FL_MODE_PRE_FLASH:
				tps61310_i2c_command(0x05, 0x6A);
				tps61310_i2c_command(0x00, 0x04);
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				tps61310_i2c_command(0x01, 0x40);
			break;
			case FL_MODE_VIDEO_TORCH:
				tps61310_i2c_command(0x05, 0x6A);
				tps61310_i2c_command(0x00, 0x04);
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				tps61310_i2c_command(0x01, 0x40);
			break;
			case FL_MODE_VIDEO_TORCH_1:
				tps61310_i2c_command(0x05, 0x6A);
				tps61310_i2c_command(0x00, 0x01);
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				tps61310_i2c_command(0x01, 0x40);
			break;
			case FL_MODE_VIDEO_TORCH_2:
				tps61310_i2c_command(0x05, 0x6A);
				tps61310_i2c_command(0x00, 0x02);
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				tps61310_i2c_command(0x01, 0x40);
			break;
			case FL_MODE_VIDEO_TORCH_3:
				tps61310_i2c_command(0x05, 0x6A);
				tps61310_i2c_command(0x00, 0x03);
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				tps61310_i2c_command(0x01, 0x40);
			break;
			case FL_MODE_VIDEO_TORCH_4:
				tps61310_i2c_command(0x05, 0x6A);
				tps61310_i2c_command(0x00, 0x04);
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				tps61310_i2c_command(0x01, 0x40);
			break;
			case FL_MODE_TORCH:
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				err |= tps61310_i2c_command(0x05, 0x6A);
				err |= tps61310_i2c_command(0x00, 0x05);
				err |= tps61310_i2c_command(0x01, 0x40);
				if (this_tps61310->reset)
					retry_flashlight_control(err, mode);
			break;
			case FL_MODE_TORCH_LEVEL_1:
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				err |= tps61310_i2c_command(0x05, 0x6A);
				err |= tps61310_i2c_command(0x00, 0x01);
				err |= tps61310_i2c_command(0x01, 0x40);
				if (this_tps61310->reset)
					retry_flashlight_control(err, mode);
			break;
			case FL_MODE_TORCH_LEVEL_2:
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				err |= tps61310_i2c_command(0x05, 0x6A);
				err |= tps61310_i2c_command(0x00, 0x03);
				err |= tps61310_i2c_command(0x01, 0x40);
				if (this_tps61310->reset)
					retry_flashlight_control(err, mode);
			break;
			default:
				FLT_ERR_LOG("%s: unknown flash_light flags: %d\n",
									__func__, mode);
				ret = -EINVAL;
			break;
			}
#endif
		} else {
#if !defined CONFIG_FLASHLIGHT_1500mA
			switch (mode) {
			case FL_MODE_OFF:
				flashlight_turn_off();
			break;
			case FL_MODE_FLASH:
				tps61310_i2c_command(0x05, 0x6A);
				tps61310_i2c_command(0x00, 0x00);
				tps61310_i2c_command(0x01, 0x9E);
				gpio_set_value_cansleep(this_tps61310->strb1, 0);
				gpio_set_value_cansleep(this_tps61310->strb0, 1);
				queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
						   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
			break;
			case FL_MODE_FLASH_LEVEL1:
					tps61310_i2c_command(0x05, 0x6A);
					tps61310_i2c_command(0x00, 0x00);
					tps61310_i2c_command(0x01, 0x86);
					gpio_set_value_cansleep(this_tps61310->strb1, 0);
					gpio_set_value_cansleep(this_tps61310->strb0, 1);
					queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
							   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
			break;
			case FL_MODE_FLASH_LEVEL2:
					tps61310_i2c_command(0x05, 0x6A);
					tps61310_i2c_command(0x00, 0x00);
					tps61310_i2c_command(0x01, 0x88);
					gpio_set_value_cansleep(this_tps61310->strb1, 0);
					gpio_set_value_cansleep(this_tps61310->strb0, 1);
					queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
							   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
			break;
			case FL_MODE_FLASH_LEVEL3:
					tps61310_i2c_command(0x05, 0x6A);
					tps61310_i2c_command(0x00, 0x00);
					tps61310_i2c_command(0x01, 0x8C);
					gpio_set_value_cansleep(this_tps61310->strb1, 0);
					gpio_set_value_cansleep(this_tps61310->strb0, 1);
					queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
							   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
			break;
			case FL_MODE_FLASH_LEVEL4:
					tps61310_i2c_command(0x05, 0x6A);
					tps61310_i2c_command(0x00, 0x00);
					tps61310_i2c_command(0x01, 0x90);
					gpio_set_value_cansleep(this_tps61310->strb1, 0);
					gpio_set_value_cansleep(this_tps61310->strb0, 1);
					queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
							   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
			break;
			case FL_MODE_FLASH_LEVEL5:
					tps61310_i2c_command(0x05, 0x6A);
					tps61310_i2c_command(0x00, 0x00);
					tps61310_i2c_command(0x01, 0x94);
					gpio_set_value_cansleep(this_tps61310->strb1, 0);
					gpio_set_value_cansleep(this_tps61310->strb0, 1);
					queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
							   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
			break;
			case FL_MODE_FLASH_LEVEL6:
					tps61310_i2c_command(0x05, 0x6A);
					tps61310_i2c_command(0x00, 0x00);
					tps61310_i2c_command(0x01, 0x98);
					gpio_set_value_cansleep(this_tps61310->strb1, 0);
					gpio_set_value_cansleep(this_tps61310->strb0, 1);
					queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
							   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
			break;
			case FL_MODE_FLASH_LEVEL7:
					tps61310_i2c_command(0x05, 0x6A);
					tps61310_i2c_command(0x00, 0x00);
					tps61310_i2c_command(0x01, 0x9C);
					gpio_set_value_cansleep(this_tps61310->strb1, 0);
					gpio_set_value_cansleep(this_tps61310->strb0, 1);
					queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
							   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
			break;
			case FL_MODE_PRE_FLASH:
				tps61310_i2c_command(0x05, 0x6A);
				tps61310_i2c_command(0x00, 0x04);
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				tps61310_i2c_command(0x01, 0x40);
			break;
			case FL_MODE_VIDEO_TORCH:
				tps61310_i2c_command(0x05, 0x6A);
				tps61310_i2c_command(0x00, 0x04);
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				tps61310_i2c_command(0x01, 0x40);
			break;
			case FL_MODE_VIDEO_TORCH_1:
				tps61310_i2c_command(0x05, 0x6A);
				tps61310_i2c_command(0x00, 0x01);
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				tps61310_i2c_command(0x01, 0x40);
			break;
			case FL_MODE_VIDEO_TORCH_2:
				tps61310_i2c_command(0x05, 0x6A);
				tps61310_i2c_command(0x00, 0x02);
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				tps61310_i2c_command(0x01, 0x40);
			break;
			case FL_MODE_VIDEO_TORCH_3:
				tps61310_i2c_command(0x05, 0x6A);
				tps61310_i2c_command(0x00, 0x03);
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				tps61310_i2c_command(0x01, 0x40);
			break;
			case FL_MODE_VIDEO_TORCH_4:
				tps61310_i2c_command(0x05, 0x6A);
				tps61310_i2c_command(0x00, 0x04);
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				tps61310_i2c_command(0x01, 0x40);
			break;
			case FL_MODE_TORCH:
				tps61310_i2c_command(0x05, 0x6A);
				tps61310_i2c_command(0x00, 0x05);
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				tps61310_i2c_command(0x01, 0x40);
			break;
			case FL_MODE_TORCH_LEVEL_1:
				tps61310_i2c_command(0x05, 0x6A);
				tps61310_i2c_command(0x00, 0x01);
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				tps61310_i2c_command(0x01, 0x40);
			break;
			case FL_MODE_TORCH_LEVEL_2:
				tps61310_i2c_command(0x05, 0x6A);
				tps61310_i2c_command(0x00, 0x03);
				gpio_set_value_cansleep(this_tps61310->strb0, 0);
				gpio_set_value_cansleep(this_tps61310->strb1, 1);
				tps61310_i2c_command(0x01, 0x40);
			break;
			default:
				FLT_ERR_LOG("%s: unknown flash_light flags: %d\n",
									__func__, mode);
				ret = -EINVAL;
			break;
			}
#endif
		}
	} else if (this_tps61310->led_count == 2) {
#if defined CONFIG_TWO_FLASHLIGHT
	switch (mode) {
	case FL_MODE_OFF:
		flashlight_turn_off();
	break;
	case FL_MODE_FLASH:
		tps61310_i2c_command(0x05, 0x6B);
		tps61310_i2c_command(0x00, 0x00);
		tps61310_i2c_command(0x02, 0x90);
		tps61310_i2c_command(0x01, 0x90);
		gpio_set_value_cansleep(this_tps61310->strb1, 0);
		gpio_set_value_cansleep(this_tps61310->strb0, 1);
		queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
				   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
	break;
	case FL_MODE_FLASH_LEVEL1:
			tps61310_i2c_command(0x05, 0x6B);
			tps61310_i2c_command(0x00, 0x00);
			tps61310_i2c_command(0x02, 0x83);
			tps61310_i2c_command(0x01, 0x83);
			gpio_set_value_cansleep(this_tps61310->strb1, 0);
			gpio_set_value_cansleep(this_tps61310->strb0, 1);
			queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
					   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
	break;
	case FL_MODE_FLASH_LEVEL2:
			tps61310_i2c_command(0x05, 0x6B);
			tps61310_i2c_command(0x00, 0x00);
			tps61310_i2c_command(0x02, 0x84);
			tps61310_i2c_command(0x01, 0x84);
			gpio_set_value_cansleep(this_tps61310->strb1, 0);
			gpio_set_value_cansleep(this_tps61310->strb0, 1);
			queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
					   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
	break;
	case FL_MODE_FLASH_LEVEL3:
			tps61310_i2c_command(0x05, 0x6B);
			tps61310_i2c_command(0x00, 0x00);
			tps61310_i2c_command(0x02, 0x86);
			tps61310_i2c_command(0x01, 0x86);
			gpio_set_value_cansleep(this_tps61310->strb1, 0);
			gpio_set_value_cansleep(this_tps61310->strb0, 1);
			queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
					   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
	break;
	case FL_MODE_FLASH_LEVEL4:
			tps61310_i2c_command(0x05, 0x6B);
			tps61310_i2c_command(0x00, 0x00);
			tps61310_i2c_command(0x02, 0x88);
			tps61310_i2c_command(0x01, 0x88);
			gpio_set_value_cansleep(this_tps61310->strb1, 0);
			gpio_set_value_cansleep(this_tps61310->strb0, 1);
			queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
					   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
	break;
	case FL_MODE_FLASH_LEVEL5:
			tps61310_i2c_command(0x05, 0x6B);
			tps61310_i2c_command(0x00, 0x00);
			tps61310_i2c_command(0x02, 0x8A);
			tps61310_i2c_command(0x01, 0x8A);
			gpio_set_value_cansleep(this_tps61310->strb1, 0);
			gpio_set_value_cansleep(this_tps61310->strb0, 1);
			queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
					   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
	break;
	case FL_MODE_FLASH_LEVEL6:
			tps61310_i2c_command(0x05, 0x6B);
			tps61310_i2c_command(0x00, 0x00);
			tps61310_i2c_command(0x02, 0x8C);
			tps61310_i2c_command(0x01, 0x8C);
			gpio_set_value_cansleep(this_tps61310->strb1, 0);
			gpio_set_value_cansleep(this_tps61310->strb0, 1);
			queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
					   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
	break;
	case FL_MODE_FLASH_LEVEL7:
			tps61310_i2c_command(0x05, 0x6B);
			tps61310_i2c_command(0x00, 0x00);
			tps61310_i2c_command(0x02, 0x8E);
			tps61310_i2c_command(0x01, 0x8E);
			gpio_set_value_cansleep(this_tps61310->strb1, 0);
			gpio_set_value_cansleep(this_tps61310->strb0, 1);
			queue_delayed_work(tps61310_work_queue, &tps61310_delayed_work,
					   msecs_to_jiffies(this_tps61310->flash_sw_timeout));
	break;
	case FL_MODE_PRE_FLASH:
		tps61310_i2c_command(0x05, 0x6B);
		tps61310_i2c_command(0x00, 0x12);
		gpio_set_value_cansleep(this_tps61310->strb0, 0);
		gpio_set_value_cansleep(this_tps61310->strb1, 1);
		tps61310_i2c_command(0x01, 0x40);
	break;
	case FL_MODE_VIDEO_TORCH:
		tps61310_i2c_command(0x05, 0x6B);
		tps61310_i2c_command(0x00, 0x12);
		gpio_set_value_cansleep(this_tps61310->strb0, 0);
		gpio_set_value_cansleep(this_tps61310->strb1, 1);
		tps61310_i2c_command(0x01, 0x40);
	break;
	case FL_MODE_TORCH:
		tps61310_i2c_command(0x05, 0x6B);
		tps61310_i2c_command(0x00, 0x1B);
		gpio_set_value_cansleep(this_tps61310->strb0, 0);
		gpio_set_value_cansleep(this_tps61310->strb1, 1);
		tps61310_i2c_command(0x01, 0x40);
	break;
	case FL_MODE_TORCH_LEVEL_1:
		tps61310_i2c_command(0x05, 0x6B);
		tps61310_i2c_command(0x00, 0x09);
		gpio_set_value_cansleep(this_tps61310->strb0, 0);
		gpio_set_value_cansleep(this_tps61310->strb1, 1);
		tps61310_i2c_command(0x01, 0x40);
	break;
	case FL_MODE_TORCH_LEVEL_2:
		tps61310_i2c_command(0x05, 0x6B);
		tps61310_i2c_command(0x00, 0x12);
		gpio_set_value_cansleep(this_tps61310->strb0, 0);
		gpio_set_value_cansleep(this_tps61310->strb1, 1);
		tps61310_i2c_command(0x01, 0x40);
	break;
	case FL_MODE_TORCH_LED_A:
		tps61310_i2c_command(0x05, 0x69);
		tps61310_i2c_command(0x00, 0x09);
		gpio_set_value_cansleep(this_tps61310->strb0, 0);
		gpio_set_value_cansleep(this_tps61310->strb1, 1);
		tps61310_i2c_command(0x01, 0x40);
	break;
	case FL_MODE_TORCH_LED_B:
		tps61310_i2c_command(0x05, 0x6A);
		tps61310_i2c_command(0x00, 0x09);
		gpio_set_value_cansleep(this_tps61310->strb0, 0);
		gpio_set_value_cansleep(this_tps61310->strb1, 1);
		tps61310_i2c_command(0x01, 0x40);
	break;

	default:
		FLT_ERR_LOG("%s: unknown flash_light flags: %d\n",
							__func__, mode);
		ret = -EINVAL;
	break;
	}
#endif
		}

	FLT_INFO_LOG("%s: mode: %d\n", __func__, mode);
	this_tps61310->mode_status = mode;
	mutex_unlock(&tps61310_mutex);

	return ret;
}

static void fl_lcdev_brightness_set(struct led_classdev *led_cdev,
						enum led_brightness brightness)
{
	enum flashlight_mode_flags mode;
	int ret = -1;


	if (brightness > 0 && brightness <= LED_HALF) {
		if (brightness == (LED_HALF - 2))
			mode = FL_MODE_TORCH_LEVEL_1;
		else if (brightness == (LED_HALF - 1))
			mode = FL_MODE_TORCH_LEVEL_2;
		else if (brightness == 1 && this_tps61310->led_count ==2)
			mode = FL_MODE_TORCH_LED_A;
		else if (brightness == 2 && this_tps61310->led_count ==2)
			mode = FL_MODE_TORCH_LED_B;
		else
			mode = FL_MODE_TORCH;
	} else if (brightness > LED_HALF && brightness <= LED_FULL) {
		if (brightness == (LED_HALF + 1))
			mode = FL_MODE_PRE_FLASH; 
		else if (brightness == (LED_HALF + 3))
			mode = FL_MODE_FLASH_LEVEL1; 
		else if (brightness == (LED_HALF + 4))
			mode = FL_MODE_FLASH_LEVEL2; 
		else if (brightness == (LED_HALF + 5))
			mode = FL_MODE_FLASH_LEVEL3; 
		else if (brightness == (LED_HALF + 6))
			mode = FL_MODE_FLASH_LEVEL4; 
		else if (brightness == (LED_HALF + 7))
			mode = FL_MODE_FLASH_LEVEL5; 
		else if (brightness == (LED_HALF + 8))
			mode = FL_MODE_FLASH_LEVEL6; 
		else if (brightness == (LED_HALF + 9))
			mode = FL_MODE_FLASH_LEVEL7; 
		else
			mode = FL_MODE_FLASH; 
	} else
		
		mode = FL_MODE_OFF;

	if ((mode != FL_MODE_OFF) && switch_state == 0){
		FLT_INFO_LOG("%s flashlight is disabled by switch, mode = %d\n",__func__, mode);
		return;
	}

	retry = 0;
	ret = tps61310_flashlight_control(mode);
	if (ret) {
		FLT_ERR_LOG("%s: control failure rc:%d\n", __func__, ret);
		return;
	}
}

static void flashlight_early_suspend(struct early_suspend *handler)
{
	FLT_INFO_LOG("%s\n", __func__);
	if (this_tps61310 != NULL && this_tps61310->mode_status)
		flashlight_turn_off();
	if (this_tps61310->power_save)
		gpio_set_value_cansleep(this_tps61310->power_save, 0);
	if (this_tps61310->power_save_2)
		gpio_set_value_cansleep(this_tps61310->power_save_2, 0);
}

static void flashlight_late_resume(struct early_suspend *handler)
{
}

static void flashlight_turn_off_work(struct work_struct *work)
{
	FLT_INFO_LOG("%s\n", __func__);
	flashlight_turn_off();
}

static int tps61310_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct tps61310_data *tps61310;
	struct TPS61310_flashlight_platform_data *pdata;
	int err = 0;

	FLT_INFO_LOG("%s +\n", __func__);
	pdata = client->dev.platform_data;
	if (!pdata) {
		FLT_ERR_LOG("%s: Assign platform_data error!!\n", __func__);
		return -EINVAL;
	}

	if (pdata->gpio_init)
		pdata->gpio_init();

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto check_functionality_failed;
	}

	tps61310 = kzalloc(sizeof(struct tps61310_data), GFP_KERNEL);
	if (!tps61310) {
		FLT_ERR_LOG("%s: kzalloc fail !!!\n", __func__);
		return -ENOMEM;
	}

	i2c_set_clientdata(client, tps61310);
	this_client = client;

	INIT_DELAYED_WORK(&tps61310_delayed_work, flashlight_turn_off_work);
	tps61310_work_queue = create_singlethread_workqueue("tps61310_wq");
	if (!tps61310_work_queue)
		goto err_create_tps61310_work_queue;

	
	tps61310->fl_lcdev.name           = FLASHLIGHT_NAME;
	tps61310->fl_lcdev.brightness_set = fl_lcdev_brightness_set;
	tps61310->strb0                   = pdata->tps61310_strb0;
	tps61310->strb1                   = pdata->tps61310_strb1;
	tps61310->reset                   = pdata->tps61310_reset;
	tps61310->flash_sw_timeout	  = pdata->flash_duration_ms;
	tps61310->led_count = (pdata->led_count) ? pdata->led_count : 1;
	tps61310->mode_pin_suspend_state_low = pdata->mode_pin_suspend_state_low;
	tps61310->enable_FLT_1500mA = pdata->enable_FLT_1500mA;
	tps61310->disable_tx_mask     = pdata->disable_tx_mask;
	tps61310->power_save              = pdata->power_save;
	tps61310->power_save_2              = pdata->power_save_2;

	if (tps61310->flash_sw_timeout <= 0)
		tps61310->flash_sw_timeout = 600;

	mutex_init(&tps61310_mutex);
	err = led_classdev_register(&client->dev, &tps61310->fl_lcdev);
	if (err < 0) {
		FLT_ERR_LOG("%s: failed on led_classdev_register\n", __func__);
		goto platform_data_null;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	tps61310->fl_early_suspend.suspend = flashlight_early_suspend;
	tps61310->fl_early_suspend.resume  = flashlight_late_resume;
	register_early_suspend(&tps61310->fl_early_suspend);
#endif
	this_tps61310 = tps61310;

	err = device_create_file(tps61310->fl_lcdev.dev, &dev_attr_function_switch);
	if (err < 0) {
		FLT_ERR_LOG("%s, create function_switch sysfs fail\n", __func__);
	}
	
	tps61310_i2c_command(0x01, 0x00);

	if (this_tps61310->enable_FLT_1500mA) {
		FLT_INFO_LOG("Flashlight with 1.5A\n");
		tps61310_i2c_command(0x07, 0x46);
		tps61310_i2c_command(0x04, 0x10);
	} else {
		
		tps61310_i2c_command(0x07, 0xF6);
	}
	
	if (this_tps61310->disable_tx_mask)
		tps61310_i2c_command(0x03, 0xC0);
	if (this_tps61310->reset)
		FLT_INFO_LOG("%s reset pin exist\n", __func__);
	else
		FLT_INFO_LOG("%s no reset pin\n", __func__);
	if (this_tps61310->power_save) {
		FLT_INFO_LOG("%s power save pin exist\n", __func__);
		gpio_set_value_cansleep(this_tps61310->power_save, 0);
	}
	else
		FLT_INFO_LOG("%s no power save pin\n", __func__);
	if (this_tps61310->power_save_2) {
		FLT_INFO_LOG("%s power save pin_2 exist\n", __func__);
		gpio_set_value_cansleep(this_tps61310->power_save_2, 0);
	}
	else
		FLT_INFO_LOG("%s no power save pin_2\n", __func__);
	FLT_INFO_LOG("%s -\n", __func__);
	return 0;


platform_data_null:
	destroy_workqueue(tps61310_work_queue);
	mutex_destroy(&tps61310_mutex);
err_create_tps61310_work_queue:
	kfree(tps61310);
check_functionality_failed:
	return err;
}

static int tps61310_remove(struct i2c_client *client)
{
	struct tps61310_data *tps61310 = i2c_get_clientdata(client);

	led_classdev_unregister(&tps61310->fl_lcdev);
	destroy_workqueue(tps61310_work_queue);
	mutex_destroy(&tps61310_mutex);
	unregister_early_suspend(&tps61310->fl_early_suspend);
	kfree(tps61310);

	FLT_INFO_LOG("%s:\n", __func__);
	return 0;
}

static const struct i2c_device_id tps61310_id[] = {
	{ "TPS61310_FLASHLIGHT", 0 },
	{ }
};
static int tps61310_resume(struct i2c_client *client)
{

		FLT_INFO_LOG("%s:\n", __func__);
		if (this_tps61310->mode_pin_suspend_state_low)
			gpio_set_value_cansleep(this_tps61310->strb1, 1);
		return 0;
}
static int tps61310_suspend(struct i2c_client *client, pm_message_t state)
{

		FLT_INFO_LOG("%s:\n", __func__);
		if (this_tps61310->mode_pin_suspend_state_low)
			gpio_set_value_cansleep(this_tps61310->strb1, 0);

		return 0;
}

static struct i2c_driver tps61310_driver = {
	.probe		= tps61310_probe,
	.remove		= tps61310_remove,
	.suspend	= tps61310_suspend,
	.resume		= tps61310_resume,
	.id_table	= tps61310_id,
	.driver		= {
		.name = "TPS61310_FLASHLIGHT",
	},
};

static int __init tps61310_init(void)
{
	FLT_INFO_LOG("tps61310 Led Flash driver: init\n");
	return i2c_add_driver(&tps61310_driver);
}

static void __exit tps61310_exit(void)
{
	i2c_del_driver(&tps61310_driver);
}

module_init(tps61310_init);
module_exit(tps61310_exit);

MODULE_DESCRIPTION("TPS61310 Led Flash driver");
MODULE_LICENSE("GPL");
