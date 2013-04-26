/* drivers/input/touchscreen/cy8c_cs.c
 *
 * Copyright (C) 2011 HTC Corporation.
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

#include <linux/input/cy8c_cs.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>

#define CY8C_I2C_RETRY_TIMES 	(10)
#define CY8C_KEYLOCKTIME    	(1500)
#define CY8C_KEYLOCKRESET	(6)

struct cy8c_cs_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct workqueue_struct *cy8c_wq;
	struct work_struct work;
	struct early_suspend early_suspend;
	int use_irq;
	struct hrtimer timer;
	uint16_t version;
	struct infor id;
	uint16_t intr;
	uint8_t vk_id;
	uint8_t debug_level;
	int *keycode;
	int (*power)(int on);
	int (*reset)(void);
	int func_support;
	struct workqueue_struct *wq_raw;
	struct delayed_work work_raw;
};

static struct cy8c_cs_data *private_cs;

static irqreturn_t cy8c_cs_irq_handler(int, void *);
static int disable_key;
static int reset_cnt; 

extern int board_build_flag(void);
#ifdef CONFIG_HAS_EARLYSUSPEND
static void cy8c_cs_early_suspend(struct early_suspend *h);
static void cy8c_cs_late_resume(struct early_suspend *h);
#endif

int i2c_cy8c_read(struct i2c_client *client, uint8_t addr, uint8_t *data, uint8_t length)
{
	int retry;

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &addr,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		}
	};

	for (retry = 0; retry < CY8C_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(client->adapter, msg, 2) == 2)
			break;
		mdelay(10);
	}
	if (retry == CY8C_I2C_RETRY_TIMES) {
		printk(KERN_INFO "[cap]i2c_read_block retry over %d\n",
			CY8C_I2C_RETRY_TIMES);
		return -EIO;
	}
	return 0;

}

int i2c_cy8c_write(struct i2c_client *client, uint8_t addr, uint8_t *data, uint8_t length)
{
	int retry, loop_i;
	uint8_t buf[length + 1];

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length + 1,
			.buf = buf,
		}
	};

	buf[0] = addr;
	for (loop_i = 0; loop_i < length; loop_i++)
		buf[loop_i + 1] = data[loop_i];

	for (retry = 0; retry < CY8C_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;
		mdelay(10);
	}

	if (retry == CY8C_I2C_RETRY_TIMES) {
		printk(KERN_ERR "[cap]i2c_write_block retry over %d\n",
			CY8C_I2C_RETRY_TIMES);
		return -EIO;
	}
	return 0;

}

int i2c_cy8c_write_byte_data(struct i2c_client *client, uint8_t addr, uint8_t value)
{
	return i2c_cy8c_write(client, addr, &value, 1);
}

static ssize_t diff(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0, i;
	char data[8] = {0};
	struct cy8c_cs_data *cs;

	pr_info("[cap] %s", __func__);

	cs = private_cs;
	ret = i2c_cy8c_write_byte_data(cs->client, CS_SELECT, CS_CMD_BASELINE);
	if (ret < 0) {
		pr_err("[cap] i2c Write baseline Err\n");
		return ret;
	}
	msleep(100);
	ret = i2c_cy8c_read(cs->client, CS_BL_HB, data, ARRAY_SIZE(data));
	if (ret < 0) {
		pr_err("[cap] i2c Read baseline Err\n");
		return ret;
	}

	for (i = 0; i < 8 ; i += 2)
		ret += sprintf(buf+ret, "BTN(%d)=%d, ", (i/2),
			       (data[i] << 8 | data[i+1]));
	ret += sprintf(buf+ret, "\n");

	return ret;
}
static DEVICE_ATTR(diff, S_IRUGO, diff, NULL);

static ssize_t reset(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct cy8c_cs_data *cs;
	cs = private_cs;

	pr_info("[cap] reset\n");
	cs->reset();
	ret = sprintf(buf, "Reset chip");
	return ret;
}
static DEVICE_ATTR(reset, S_IRUGO, reset, NULL);

static ssize_t stop_report(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int err;
	unsigned long i = 0;
	err = strict_strtoul(buf, 10, &i);

	if (disable_key < 2 || disable_key >= 0) {
		disable_key = i;
		pr_info("[cap] KEY Report %s!!\n", disable_key ?
				"DISABLE" : "ENABLE");
	} else
		pr_info("[cap] Parameter Error\n");

	return count;
}

static ssize_t show_flag(struct device *dev, struct device_attribute *attr,
						 char *buf)
{
	return sprintf(buf, "[cap] disable_key = %d\n", disable_key);
}
static DEVICE_ATTR(diskey, (S_IWUSR|S_IRUGO), show_flag, stop_report);

static int cy8c_printcs_raw(struct cy8c_cs_data *cs, char *buf)
{
	int ret = 0, pos = 0, i, j, cmd[4] = {CS_CMD_BTN1, CS_CMD_BTN2, CS_CMD_BTN3, CS_CMD_BTN4};
	char data[6] = {0}, capstate[3][10] = {"BL", "Raw", "Dlt"};

	for (i = 0; i < cs->id.config; i++) {
		ret = i2c_cy8c_write_byte_data(cs->client, CS_SELECT, cmd[i]);
		if (ret < 0) {
			pr_err("[cap] i2c Write inform (%d_%#x) Err\n", i+1, cmd[i]);
			return ret;
		}
		msleep(50);
		ret = i2c_cy8c_read(cs->client, CS_BL_HB, data, ARRAY_SIZE(data));
		if (ret < 0) {
			pr_err("[cap] i2c Read inform (%d_%#x)) Err\n", i+1, cmd[i]);
			return ret;
		}
		pos += sprintf(buf+pos, "BTN(%d)", i);
		for (j = 0; j < 6 ; j += 2)
			pos += sprintf(buf+pos, "%s=%d, ", capstate[j/2],
				       (data[j] << 8 | data[j+1]));
		pos += sprintf(buf+pos, "\n");
		memset(data, 0, sizeof(ARRAY_SIZE(data)));
	}
	return pos;
}

static ssize_t inform(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0, pos = 0;
	char data[2] = {0};
	struct cy8c_cs_data *cs;

	cs = private_cs;

	if (cs->id.version < 0x10 && cs->id.version > 0x08)
		cs->id.version = cs->id.version << 4;

	if (cs->id.version >= 0x86) {
		memset(data, 0, sizeof(ARRAY_SIZE(data)));
		ret = i2c_cy8c_read(cs->client, CS_INT_STATUS, data, 2);
		if (ret < 0) {
			pr_err("[cap] i2c Read inform INT status Err\n");
			return ret;
		}
		pos += sprintf(buf+pos, "Btn code = %x, INT status= %x\n", data[0], data[1]);
	}
	pos += cy8c_printcs_raw(cs, buf+pos);

	return pos;
}
static DEVICE_ATTR(inform, S_IRUGO, inform, NULL);

static ssize_t cs_vendor_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char data[3] = {0};
	int ret = 0;
	struct cy8c_cs_data *cs;
	cs = private_cs;

	ret = i2c_cy8c_read(cs->client, CS_FW_VERSION, data, 2);
	if (ret < 0) {
		pr_err("[cap] i2c Read version Err\n");
		return ret;
	}
	if (cs->id.chipid == CS_CHIPID)
		sprintf(buf, "%s_V%x\n", CYPRESS_SS_NAME, data[0]);
	else
		sprintf(buf, "%s_V%x\n", CYPRESS_CS_NAME, data[0]);
	ret += strlen(buf)+1;

	return ret;
}
static DEVICE_ATTR(vendor, S_IRUGO, cs_vendor_show, NULL);

static ssize_t cy8c_cs_gpio_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct cy8c_cs_data *cs_data;
	struct cy8c_i2c_cs_platform_data *pdata;

	cs_data = private_cs;
	pdata = cs_data->client->dev.platform_data;

	ret = gpio_get_value(pdata->gpio_irq);
	printk(KERN_DEBUG "[cap] GPIO_CS_INT_N=%d\n", pdata->gpio_irq);
	sprintf(buf, "GPIO_CS_INT_N=%d\n", ret);
	ret = strlen(buf) + 1;
	return ret;
}
static DEVICE_ATTR(gpio, S_IRUGO, cy8c_cs_gpio_show, NULL);

static ssize_t cy8c_cs_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct cy8c_cs_data *cs_data;
	struct cy8c_i2c_cs_platform_data *pdata;

	cs_data = private_cs;
	pdata = cs_data->client->dev.platform_data;

	ret = gpio_get_value(pdata->gpio_irq);
	printk(KERN_DEBUG "GPIO_CS_INT_N=%d\n", pdata->gpio_irq);
	ret = strlen(buf) + 1;
	return ret;
}
static DEVICE_ATTR(read, S_IRUGO, cy8c_cs_read_show, NULL);

static ssize_t debug_level_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int i;
	struct cy8c_cs_data *cs_data;
	cs_data = private_cs;
	if (sscanf(buf, "%d", &i) == 1 && i < 2) {
		cs_data->debug_level = i;
		pr_info("[cap] debug_level = %d\b", cs_data->debug_level);
	} else
		pr_info("[cap] Parameter Error\n");
	return count;
}

static ssize_t debug_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct cy8c_cs_data *cs_data;
	cs_data = private_cs;
	return sprintf(buf, "[cap] debug_level = %d\n", cs_data->debug_level);
}
DEVICE_ATTR(debug_level, (S_IWUSR|S_IRUGO), debug_level_show, debug_level_set);

static struct kobject *android_touchkey_kobj;

static int cy8c_touchkey_sysfs_init(void)
{
	int ret;
	android_touchkey_kobj = kobject_create_and_add("android_key", NULL);
	if (android_touchkey_kobj == NULL) {
		printk(KERN_ERR "%s: subsystem_register failed\n", __func__);
		ret = -ENOMEM;
		return ret;
	}
	ret = sysfs_create_file(android_touchkey_kobj, &dev_attr_gpio.attr);
	if (ret) {
		printk(KERN_ERR "%s: sysfs_create_file gpio failed\n", __func__);
		return ret;
	}
	ret = sysfs_create_file(android_touchkey_kobj, &dev_attr_read.attr);
	if (ret) {
		printk(KERN_ERR "%s: sysfs_create_file read failed\n", __func__);
		return ret;
	}
	ret = sysfs_create_file(android_touchkey_kobj, &dev_attr_vendor.attr);
	if (ret) {
		printk(KERN_ERR "%s: sysfs_create_file vendor failed\n", __func__);
		return ret;
	}
	ret = sysfs_create_file(android_touchkey_kobj, &dev_attr_inform.attr);
	if (ret) {
		printk(KERN_ERR "%s: sysfs_create_file inform failed\n", __func__);
		return ret;
	}
	ret = sysfs_create_file(android_touchkey_kobj, &dev_attr_diff.attr);
	if (ret) {
		printk(KERN_ERR "%s: sysfs_create_file inform failed\n", __func__);
		return ret;
	}
	ret = sysfs_create_file(android_touchkey_kobj, &dev_attr_debug_level.attr);
	if (ret) {
		printk(KERN_ERR "%s: sysfs_create_file debug_level failed\n", __func__);
		return ret;
	}
	ret = sysfs_create_file(android_touchkey_kobj, &dev_attr_reset.attr);
	if (ret) {
		printk(KERN_ERR "%s: sysfs_create_file debug_level failed\n", __func__);
		return ret;
	}
	ret = sysfs_create_file(android_touchkey_kobj, &dev_attr_diskey.attr);
	if (ret) {
		printk(KERN_ERR "%s: sysfs_create_file debug_level failed\n", __func__);
		return ret;
	}
	return 0;
}

static void cy8c_touchkey_sysfs_deinit(void)
{
	sysfs_remove_file(android_touchkey_kobj, &dev_attr_gpio.attr);
	sysfs_remove_file(android_touchkey_kobj, &dev_attr_read.attr);
	sysfs_remove_file(android_touchkey_kobj, &dev_attr_vendor.attr);
	sysfs_remove_file(android_touchkey_kobj, &dev_attr_inform.attr);
	sysfs_remove_file(android_touchkey_kobj, &dev_attr_diff.attr);
	sysfs_remove_file(android_touchkey_kobj, &dev_attr_debug_level.attr);
	sysfs_remove_file(android_touchkey_kobj, &dev_attr_reset.attr);
	sysfs_remove_file(android_touchkey_kobj, &dev_attr_diskey.attr);
	kobject_del(android_touchkey_kobj);
}

static void cy8c_rawdata_print(struct work_struct *work)
{
	char buf[150] = {0};
	int pos = 0;
	struct cy8c_cs_data *cs = container_of(work, struct cy8c_cs_data,
					       work_raw.work);
	pos += cy8c_printcs_raw(cs, buf+pos);
	pos = strlen(buf)+1;
	pr_info("[cap]%s\n", buf);

	if (cs->vk_id) {
		reset_cnt++;
		if (reset_cnt % CY8C_KEYLOCKRESET == 0) {
			pr_info("[cap] keylock reset\n");
			cs->reset();
			reset_cnt = 0;
			cs->vk_id = 0;
		}
		queue_delayed_work(cs->wq_raw, &cs->work_raw,
				   msecs_to_jiffies(CY8C_KEYLOCKTIME-500));
	}
}

static int cy8c_init_sensor(struct cy8c_cs_data *cs, struct cy8c_i2c_cs_platform_data *pdata)
{
	uint8_t ver[2] = {0}, chip[2] = {0};
	int ret = 0;
	pr_info("[cap] %s\n", __func__);

	ret = i2c_cy8c_read(cs->client, CS_FW_CHIPID, chip, 1);
	if (ret < 0) {
		printk(KERN_ERR "[cap_err] Chip Read Err\n");
		goto err_fw_get_fail;
	}

	ret = i2c_cy8c_read(cs->client, CS_FW_VERSION, ver, 1);
	if (!ret)
		cs->id.version = ver[0];
	else {
		printk(KERN_ERR "[cap_err] Ver Read Err\n");
		goto err_fw_get_fail;
	}

	if (chip[0] == CS_CHIPID) {
		cs->id.chipid = chip[0];
		pr_info("[cap] CY8C_Smart_V%x\n", cs->id.version);
	} else
		pr_info("[cap] CY8C_Cap_V%x\n", cs->id.version);

	ver[0] = 0;
	ret = i2c_cy8c_read(cs->client, CS_FW_KEYCFG, ver, 1);
	if (ret < 0) {
		printk(KERN_ERR "[cap_err] Config Read Err\n");
		goto err_fw_get_fail;
	} else {
		if ((ver[0] != 0) && (ver[0] == CS_KEY_3 || ver[0] == CS_KEY_4))
			cs->id.config = ver[0];
		else
			cs->id.config = 0;
		pr_info("[cap] config = %d\n", cs->id.config);
	}
	return 0;

err_fw_get_fail:
	return ret;
}

static void report_key_func(struct cy8c_cs_data *cs, uint8_t vk)
{
	int ret = 0;
	if ((cs->debug_level & 0x01) || board_build_flag() > 0)
		pr_info("[cap] vk = %x\n", vk);

	if (vk) {
		switch (vk) {
		case 0x01:
			input_report_key(cs->input_dev, cs->keycode[0], 1);
			cs->vk_id = vk;
			break;
		case 0x02:
			input_report_key(cs->input_dev, cs->keycode[1], 1);
			cs->vk_id = vk;
			break;
		case 0x04:
			input_report_key(cs->input_dev, cs->keycode[2], 1);
			cs->vk_id = vk;
			break;
		case 0x08:
			input_report_key(cs->input_dev, cs->keycode[3], 1);
			cs->vk_id = vk;
			break;
		}
#if defined(CONFIG_TOUCH_KEY_FILTER)
		blocking_notifier_call_chain(&touchkey_notifier_list, 1, NULL);
#endif
	} else {
		switch (cs->vk_id) {
		case 0x01:
			input_report_key(cs->input_dev, cs->keycode[0], 0);
			break;
		case 0x02:
			input_report_key(cs->input_dev, cs->keycode[1], 0);
			break;
		case 0x04:
			input_report_key(cs->input_dev, cs->keycode[2], 0);
			break;
		case 0x08:
			input_report_key(cs->input_dev, cs->keycode[3], 0);
			break;
		}
		cs->vk_id = 0;
	}
	input_sync(cs->input_dev);

	if (cs->func_support & CS_FUNC_PRINTRAW) {
		if (cs->vk_id) {
			queue_delayed_work(cs->wq_raw, &cs->work_raw,
					   msecs_to_jiffies(CY8C_KEYLOCKTIME));
		} else {
			ret = cancel_delayed_work_sync(&cs->work_raw);
			if (!ret)
				cancel_delayed_work(&cs->work_raw);
		}
	}
}

static void cy8c_cs_work_func(struct work_struct *work)
{
	struct cy8c_cs_data *cs;
	uint8_t buf[3] = {0};
	static	uint8_t pre_buf[3] = {0};

	cs = container_of(work, struct cy8c_cs_data, work);

	if (i2c_cy8c_read(cs->client, CS_STATUS, buf, 2) < 0) {
		memset(buf, 0, sizeof(buf));
		memset(pre_buf, 0, sizeof(pre_buf));
		pr_err("[cap_err] %s i2c read fail", __func__);
		goto enableirq;
	}

	if (!disable_key)
		report_key_func(cs, buf[0]);

	memcpy(pre_buf, buf, 2);
enableirq:
	if (!cs->use_irq)
		hrtimer_start(&cs->timer, ktime_set(0, 20000000), HRTIMER_MODE_REL);
	else
		enable_irq(cs->client->irq);
}

#if 1
static enum hrtimer_restart cy8c_cs_timer_func(struct hrtimer *timer)
{
	struct cy8c_cs_data *cs;

	cs = container_of(timer, struct cy8c_cs_data, timer);
	queue_work(cs->cy8c_wq, &cs->work);
	return HRTIMER_NORESTART;
}
#endif
#if 1
static irqreturn_t cy8c_cs_irq_handler(int irq, void *dev_id)
{
	struct cy8c_cs_data *cs = dev_id;

	disable_irq_nosync(cs->client->irq);
	queue_work(cs->cy8c_wq, &cs->work);
	return IRQ_HANDLED;
}
#endif

static int cy8c_cs_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct cy8c_cs_data *cs;
	struct cy8c_i2c_cs_platform_data *pdata;
	int ret = 0;

	printk(KERN_DEBUG "[cap] %s: enter\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "[cap_err] need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	cs = kzalloc(sizeof(struct cy8c_cs_data), GFP_KERNEL);
	if (cs == NULL) {
		printk(KERN_ERR "[cap_err] allocate cy8c_cs_data failed\n");
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	cs->client = client;
	i2c_set_clientdata(client, cs);
	pdata = client->dev.platform_data;

	if (pdata) {
		pdata->reset();
		msleep(50);
		cs->intr = pdata->gpio_irq;
	}

	if (cy8c_init_sensor(cs, pdata) < 0) {
		pr_err("[cap_err] init failure, not probe up driver\n");
		goto err_init_sensor_failed;
	}
	if (pdata) {
		if (pdata->id.config != cs->id.config) {
			pr_info("[cap] pdata ++\n");
			pdata++;
		}
	}

	cs->cy8c_wq = create_singlethread_workqueue("cypress_touchkey");
	if (!cs->cy8c_wq) {
		printk(KERN_ERR "[cap_err] create_singlethread_workqueue cy8c_wq fail\n");
		goto err_create_wq_failed;
	}
	INIT_WORK(&cs->work, cy8c_cs_work_func);

	cs->input_dev = input_allocate_device();
	if (cs->input_dev == NULL) {
		ret = -ENOMEM;
		printk(KERN_ERR "[cap_err] Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	cs->input_dev->name       = "cy8c-touchkey";
	cs->input_dev->id.product = cs->id.chipid;
	cs->input_dev->id.version = cs->id.version;
	cs->func_support          = pdata->func_support;
	cs->keycode               = pdata->keycode;
	cs->reset                 = pdata->reset;

	set_bit(EV_SYN, cs->input_dev->evbit);
	set_bit(EV_KEY, cs->input_dev->evbit);

	set_bit(KEY_BACK, cs->input_dev->keybit);
	set_bit(KEY_HOME, cs->input_dev->keybit);
	set_bit(KEY_APP_SWITCH, cs->input_dev->keybit);
	set_bit(KEY_MENU, cs->input_dev->keybit);

	set_bit(KEY_SEARCH, cs->input_dev->keybit);
	set_bit(KEY_WEIBO, cs->input_dev->keybit);

	ret = input_register_device(cs->input_dev);
	if (ret) {
		printk(KERN_ERR "[cap_err] unable to register %s input device\n",
			cs->input_dev->name);

		goto err_input_register_device_failed;
	}

	private_cs = cs;

	if (cs->func_support & CS_FUNC_PRINTRAW) {
		pr_info("[cap]support_keylock(%x)\n", cs->func_support);
		cs->wq_raw = create_singlethread_workqueue("CY8C_print_rawdata");
		if (!cs->wq_raw) {
			pr_err("[cap]allocate cy8c_cs_print_rawdata failed\n");
			ret = -ENOMEM;
			goto err_input_register_device_failed;
		}
		INIT_DELAYED_WORK(&cs->work_raw, cy8c_rawdata_print);
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	cs->early_suspend.level   = EARLY_SUSPEND_LEVEL_STOP_DRAWING;
	cs->early_suspend.suspend = cy8c_cs_early_suspend;
	cs->early_suspend.resume  = cy8c_cs_late_resume;
	register_early_suspend(&cs->early_suspend);
#endif
	cy8c_touchkey_sysfs_init();

	cs->use_irq = 1;
	if (client->irq && cs->use_irq) {
		ret = request_irq(client->irq, cy8c_cs_irq_handler,
				  IRQF_TRIGGER_FALLING,
				  cs->id.chipid == CS_CHIPID ? CYPRESS_SS_NAME : CYPRESS_CS_NAME,
				  cs);
		if (ret < 0) {
			dev_err(&client->dev, "[cap_err]request_irq failed\n");
			printk(KERN_ERR "[cap_err] request_irq failed for gpio %d,"
			       " irq %d\n", cs->intr, client->irq);
		}
	}

	if (!cs->use_irq) {
		hrtimer_init(&cs->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		cs->timer.function = cy8c_cs_timer_func;
		hrtimer_start(&cs->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}

	return 0;

err_input_register_device_failed:
	input_free_device(cs->input_dev);

err_input_dev_alloc_failed:
	destroy_workqueue(cs->cy8c_wq);
err_init_sensor_failed:
err_create_wq_failed:
	kfree(cs);

err_alloc_data_failed:
err_check_functionality_failed:
	return ret;
}

static int cy8c_cs_remove(struct i2c_client *client)
{
	struct cy8c_cs_data *cs = i2c_get_clientdata(client);

	cy8c_touchkey_sysfs_deinit();

	unregister_early_suspend(&cs->early_suspend);
	free_irq(client->irq, cs);
	input_unregister_device(cs->input_dev);

	kfree(cs);

	return 0;
}

static int cy8c_cs_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	struct cy8c_cs_data *cs = i2c_get_clientdata(client);

	pr_info("[cap] %s\n", __func__);

	if (cs->func_support & CS_FUNC_PRINTRAW) {
		ret = cancel_delayed_work_sync(&cs->work_raw);
		if (!ret)
			cancel_delayed_work(&cs->work_raw);
	}
	if (client->irq && cs->use_irq) {
		disable_irq(client->irq);
		ret = cancel_work_sync(&cs->work);
		if (ret)
			enable_irq(client->irq);
	}
	i2c_cy8c_write_byte_data(client, CS_MODE, CS_CMD_DSLEEP);
	return 0;
}

static int cy8c_cs_resume(struct i2c_client *client)
{
	struct cy8c_cs_data *cs = i2c_get_clientdata(client);

	pr_info("[cap] %s\n", __func__);
	cs->reset();

	msleep(50);

	if (client->irq && cs->use_irq)
		enable_irq(client->irq);
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cy8c_cs_early_suspend(struct early_suspend *h)
{
	struct cy8c_cs_data *ts;
	ts = container_of(h, struct cy8c_cs_data, early_suspend);
	cy8c_cs_suspend(ts->client, PMSG_SUSPEND);
}

static void cy8c_cs_late_resume(struct early_suspend *h)
{
	struct cy8c_cs_data *ts;
	ts = container_of(h, struct cy8c_cs_data, early_suspend);
	cy8c_cs_resume(ts->client);
}
#endif

static const struct i2c_device_id cy8c_cs_id[] = {
	{ CYPRESS_CS_NAME, 0 },
};

static struct i2c_driver cy8c_cs_driver = {
	.probe		= cy8c_cs_probe,
	.remove		= cy8c_cs_remove,
	.id_table	= cy8c_cs_id,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= cy8c_cs_suspend,
	.resume		= cy8c_cs_resume,
#endif
	.driver		= {
		.name = CYPRESS_CS_NAME,
	},
};

static int __init cy8c_cs_init(void)
{
	printk(KERN_INFO "[cap] %s: enter\n", __func__);
	return i2c_add_driver(&cy8c_cs_driver);
}

static void __exit cy8c_cs_exit(void)
{
	i2c_del_driver(&cy8c_cs_driver);
}

module_init(cy8c_cs_init);
module_exit(cy8c_cs_exit);

MODULE_DESCRIPTION("cy8c_cs driver");
MODULE_LICENSE("GPL");
