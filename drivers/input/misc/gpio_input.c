/* drivers/input/misc/gpio_input.c
 *
 * Copyright (C) 2007 Google, Inc.
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

#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/wakelock.h>

#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_3K
static uint8_t power_key_state;
static spinlock_t power_key_state_lock;

#define PWRKEY_PRESS_DUE 1*HZ
#include <linux/module.h>
static void init_power_key_api(void)
{
	spin_lock_init(&power_key_state_lock);
	power_key_state = 0;
}

static void setPowerKeyState(uint8_t flag)
{
	spin_lock(&power_key_state_lock);
	power_key_state = flag;
	spin_unlock(&power_key_state_lock);
}

uint8_t getPowerKeyState(void)
{
	uint8_t value;

	spin_lock(&power_key_state_lock);
	value = power_key_state;
	spin_unlock(&power_key_state_lock);

	return value;
}
EXPORT_SYMBOL(getPowerKeyState);

static void power_key_state_disable_work_func(struct work_struct *dummy)
{
	setPowerKeyState(0);
	printk(KERN_INFO "[KEY][PWR][STATE]power key pressed outdated\n");
}
static DECLARE_DELAYED_WORK(power_key_state_disable_work, power_key_state_disable_work_func);

static void handle_power_key_state(unsigned int code, int value)
{
	int ret = 0;
	if (code == KEY_POWER && value == 1) {
		printk(KERN_INFO "[PWR][STATE]try to schedule power key pressed due\n");
		ret = schedule_delayed_work(&power_key_state_disable_work, PWRKEY_PRESS_DUE);
		if (!ret) {
			printk(KERN_INFO "[PWR][STATE]Schedule power key pressed due failed, seems already have one, try to cancel...\n");
			ret = cancel_delayed_work(&power_key_state_disable_work);
			if (!ret) {
				setPowerKeyState(1);
				if (schedule_delayed_work(&power_key_state_disable_work, PWRKEY_PRESS_DUE)) {
					printk(KERN_INFO "[PWR][STATE]Re-schedule power key pressed due SCCUESS.\n");
					printk(KERN_INFO "[PWR][STATE] start count for power key pressed due\n");
					setPowerKeyState(1);
				} else
					printk(KERN_INFO "[PWR][STATE]Re-schedule power key pressed due FAILED, reason unknown, give up.\n");
			} else {
				printk(KERN_INFO "[PWR][STATE]Cancel scheduled power key due success, now re-schedule.\n");
				if (schedule_delayed_work(&power_key_state_disable_work, PWRKEY_PRESS_DUE)) {
					printk(KERN_INFO "[PWR][STATE]Re-schedule power key pressed due SCCUESS.\n");
					printk(KERN_INFO "[PWR][STATE] start count for power key pressed due\n");
					setPowerKeyState(1);
				} else
					printk(KERN_INFO "[PWR][STATE]Re-schedule power key pressed due FAILED, reason unknown, give up.\n");
			}
		} else {
			printk(KERN_INFO "[PWR][STATE] start count for power key pressed due\n");
			setPowerKeyState(1);
		}
	}
}
#endif /* CONFIG_TOUCHSCREEN_SYNAPTICS_3K */

#ifdef CONFIG_HTC_WAKE_ON_VOL
static DEFINE_MUTEX(wakeup_mutex);
static unsigned char wakeup_bitmask;
static unsigned char set_wakeup;
static unsigned int vol_up_irq;
static unsigned int vol_down_irq;
static ssize_t vol_wakeup_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	unsigned char bitmask = 0;
	bitmask = simple_strtoull(buf, NULL, 10);
	mutex_lock(&wakeup_mutex);
	if (bitmask) {
		if (bitmask == 127)
			wakeup_bitmask &= bitmask;
		else if (bitmask > 128)
			wakeup_bitmask &= bitmask;
		else
			wakeup_bitmask |= bitmask;
	}

	if (wakeup_bitmask && (!set_wakeup)) {
		enable_irq_wake(vol_up_irq);
		enable_irq_wake(vol_down_irq);
		set_wakeup = 1;
		printk(KERN_INFO "%s:change to wake up function(%d, %d)\n", __func__, vol_up_irq, vol_down_irq);
	} else if ((!wakeup_bitmask) && set_wakeup){
		disable_irq_wake(vol_up_irq);
		disable_irq_wake(vol_down_irq);
		set_wakeup = 0;
		printk(KERN_INFO "%s:change to non-wake up function(%d, %d)\n", __func__, vol_up_irq, vol_down_irq);
	}
	mutex_unlock(&wakeup_mutex);
	return count;
}
static ssize_t vol_wakeup_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%x\n", wakeup_bitmask);
}

static DEVICE_ATTR(vol_wakeup, 0664, vol_wakeup_show, vol_wakeup_store);
#endif /* CONFIG_HTC_WAKE_ON_VOL */

enum {
	DEBOUNCE_UNSTABLE     = BIT(0),	/* Got irq, while debouncing */
	DEBOUNCE_PRESSED      = BIT(1),
	DEBOUNCE_NOTPRESSED   = BIT(2),
	DEBOUNCE_WAIT_IRQ     = BIT(3),	/* Stable irq state */
	DEBOUNCE_POLL         = BIT(4),	/* Stable polling state */

	DEBOUNCE_UNKNOWN =
		DEBOUNCE_PRESSED | DEBOUNCE_NOTPRESSED,
};

struct gpio_key_state {
	struct gpio_input_state *ds;
	uint8_t debounce;
};

struct gpio_input_state {
	struct gpio_event_input_devs *input_devs;
	const struct gpio_event_input_info *info;
	struct hrtimer timer;
	int use_irq;
	int debounce_count;
	spinlock_t irq_lock;
	struct wake_lock wake_lock;
	struct gpio_key_state key_state[0];
};

static enum hrtimer_restart gpio_event_input_timer_func(struct hrtimer *timer)
{
	int i;
	int pressed;
	struct gpio_input_state *ds =
		container_of(timer, struct gpio_input_state, timer);
	unsigned gpio_flags = ds->info->flags;
	unsigned npolarity;
	int nkeys = ds->info->keymap_size;
	const struct gpio_event_direct_entry *key_entry;
	struct gpio_key_state *key_state;
	unsigned long irqflags;
	uint8_t debounce;
	bool sync_needed;

#if 0
	key_entry = kp->keys_info->keymap;
	key_state = kp->key_state;
	for (i = 0; i < nkeys; i++, key_entry++, key_state++)
		pr_info("gpio_read_detect_status %d %d\n", key_entry->gpio,
			gpio_read_detect_status(key_entry->gpio));
#endif
	key_entry = ds->info->keymap;
	key_state = ds->key_state;
	sync_needed = false;
	spin_lock_irqsave(&ds->irq_lock, irqflags);
	for (i = 0; i < nkeys; i++, key_entry++, key_state++) {
		debounce = key_state->debounce;
		if (debounce & DEBOUNCE_WAIT_IRQ)
			continue;
		if (key_state->debounce & DEBOUNCE_UNSTABLE) {
			debounce = key_state->debounce = DEBOUNCE_UNKNOWN;
			enable_irq(gpio_to_irq(key_entry->gpio));
			if (gpio_flags & GPIOEDF_PRINT_KEY_UNSTABLE)
				pr_info("gpio_keys_scan_keys: key %x-%x, %d "
					"(%d) continue debounce\n",
					ds->info->type, key_entry->code,
					i, key_entry->gpio);
		}
		npolarity = !(gpio_flags & GPIOEDF_ACTIVE_HIGH);
		pressed = gpio_get_value(key_entry->gpio) ^ npolarity;
		if (debounce & DEBOUNCE_POLL) {
			if (pressed == !(debounce & DEBOUNCE_PRESSED)) {
				ds->debounce_count++;
				key_state->debounce = DEBOUNCE_UNKNOWN;
				if (gpio_flags & GPIOEDF_PRINT_KEY_DEBOUNCE)
					pr_info("gpio_keys_scan_keys: key %x-"
						"%x, %d (%d) start debounce\n",
						ds->info->type, key_entry->code,
						i, key_entry->gpio);
			}
			continue;
		}
		if (pressed && (debounce & DEBOUNCE_NOTPRESSED)) {
			if (gpio_flags & GPIOEDF_PRINT_KEY_DEBOUNCE)
				pr_info("gpio_keys_scan_keys: key %x-%x, %d "
					"(%d) debounce pressed 1\n",
					ds->info->type, key_entry->code,
					i, key_entry->gpio);
			key_state->debounce = DEBOUNCE_PRESSED;
			continue;
		}
		if (!pressed && (debounce & DEBOUNCE_PRESSED)) {
			if (gpio_flags & GPIOEDF_PRINT_KEY_DEBOUNCE)
				pr_info("gpio_keys_scan_keys: key %x-%x, %d "
					"(%d) debounce pressed 0\n",
					ds->info->type, key_entry->code,
					i, key_entry->gpio);
			key_state->debounce = DEBOUNCE_NOTPRESSED;
			continue;
		}
		/* key is stable */
		ds->debounce_count--;
		if (ds->use_irq)
			key_state->debounce |= DEBOUNCE_WAIT_IRQ;
		else
			key_state->debounce |= DEBOUNCE_POLL;
		if (gpio_flags & GPIOEDF_PRINT_KEYS)
			pr_info("gpio_keys_scan_keys: key %x-%x, %d (%d) "
				"changed to %d\n", ds->info->type,
				key_entry->code, i, key_entry->gpio, pressed);
#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_3K
		handle_power_key_state(key_entry->code, pressed);
#endif
		input_event(ds->input_devs->dev[key_entry->dev], ds->info->type,
			    key_entry->code, pressed);
		sync_needed = true;
	}
	if (sync_needed) {
		for (i = 0; i < ds->input_devs->count; i++)
			input_sync(ds->input_devs->dev[i]);
	}

#if 0
	key_entry = kp->keys_info->keymap;
	key_state = kp->key_state;
	for (i = 0; i < nkeys; i++, key_entry++, key_state++) {
		pr_info("gpio_read_detect_status %d %d\n", key_entry->gpio,
			gpio_read_detect_status(key_entry->gpio));
	}
#endif

	if (ds->debounce_count)
		hrtimer_start(timer, ds->info->debounce_time, HRTIMER_MODE_REL);
	else if (!ds->use_irq)
		hrtimer_start(timer, ds->info->poll_time, HRTIMER_MODE_REL);
	else
		wake_unlock(&ds->wake_lock);

	spin_unlock_irqrestore(&ds->irq_lock, irqflags);

	return HRTIMER_NORESTART;
}

static irqreturn_t gpio_event_input_irq_handler(int irq, void *dev_id)
{
	struct gpio_key_state *ks = dev_id;
	struct gpio_input_state *ds = ks->ds;
	int keymap_index = ks - ds->key_state;
	const struct gpio_event_direct_entry *key_entry;
	unsigned long irqflags;
	int pressed;

	if (!ds->use_irq)
		return IRQ_HANDLED;

	key_entry = &ds->info->keymap[keymap_index];

	if (ds->info->debounce_time.tv64) {
		spin_lock_irqsave(&ds->irq_lock, irqflags);
		if (ks->debounce & DEBOUNCE_WAIT_IRQ) {
			ks->debounce = DEBOUNCE_UNKNOWN;
			if (ds->debounce_count++ == 0) {
				wake_lock(&ds->wake_lock);
				hrtimer_start(
					&ds->timer, ds->info->debounce_time,
					HRTIMER_MODE_REL);
			}
			if (ds->info->flags & GPIOEDF_PRINT_KEY_DEBOUNCE)
				pr_info("gpio_event_input_irq_handler: "
					"key %x-%x, %d (%d) start debounce\n",
					ds->info->type, key_entry->code,
					keymap_index, key_entry->gpio);
		} else {
			disable_irq_nosync(irq);
			ks->debounce = DEBOUNCE_UNSTABLE;
		}
		spin_unlock_irqrestore(&ds->irq_lock, irqflags);
	} else {
		pressed = gpio_get_value(key_entry->gpio) ^
			!(ds->info->flags & GPIOEDF_ACTIVE_HIGH);
		if (ds->info->flags & GPIOEDF_PRINT_KEYS)
			pr_info("gpio_event_input_irq_handler: key %x-%x, %d "
				"(%d) changed to %d\n",
				ds->info->type, key_entry->code, keymap_index,
				key_entry->gpio, pressed);
		input_event(ds->input_devs->dev[key_entry->dev], ds->info->type,
			    key_entry->code, pressed);
		input_sync(ds->input_devs->dev[key_entry->dev]);
	}
	return IRQ_HANDLED;
}

static int gpio_event_input_request_irqs(struct gpio_input_state *ds)
{
	int i;
	int err;
	unsigned int irq;
	unsigned long req_flags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;

	for (i = 0; i < ds->info->keymap_size; i++) {
		err = irq = gpio_to_irq(ds->info->keymap[i].gpio);
		if (err < 0)
			goto err_gpio_get_irq_num_failed;
		err = request_irq(irq, gpio_event_input_irq_handler,
				  req_flags, "gpio_keys", &ds->key_state[i]);
		if (err) {
			pr_err("gpio_event_input_request_irqs: request_irq "
				"failed for input %d, irq %d\n",
				ds->info->keymap[i].gpio, irq);
			goto err_request_irq_failed;
		}
#ifdef CONFIG_HTC_WAKE_ON_VOL
		if (ds->info->keymap[i].code == KEY_VOLUMEUP ||
			ds->info->keymap[i].code == KEY_VOLUMEDOWN) {
			pr_info("keycode = %d, gpio = %d, irq = %d\n",
				ds->info->keymap[i].code,
				ds->info->keymap[i].gpio, irq);
			if (ds->info->keymap[i].code == KEY_VOLUMEUP)
				vol_up_irq = irq;
			else
				vol_down_irq = irq;
		} else {
			enable_irq_wake(irq);
		}
#endif
		if (ds->info->info.no_suspend) {
			err = enable_irq_wake(irq);
			if (err) {
				pr_err("gpio_event_input_request_irqs: "
					"enable_irq_wake failed for input %d, "
					"irq %d\n",
					ds->info->keymap[i].gpio, irq);
				goto err_enable_irq_wake_failed;
			}
		}
	}
#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_3K
	init_power_key_api();
#endif
	return 0;

	for (i = ds->info->keymap_size - 1; i >= 0; i--) {
		irq = gpio_to_irq(ds->info->keymap[i].gpio);
		if (ds->info->info.no_suspend)
			disable_irq_wake(irq);
err_enable_irq_wake_failed:
		free_irq(irq, &ds->key_state[i]);
err_request_irq_failed:
err_gpio_get_irq_num_failed:
		;
	}
	return err;
}

int gpio_event_input_func(struct gpio_event_input_devs *input_devs,
			struct gpio_event_info *info, void **data, int func)
{
	int ret;
	int i;
	unsigned long irqflags;
	struct gpio_event_input_info *di;
	struct gpio_input_state *ds = *data;
#ifdef CONFIG_HTC_WAKE_ON_VOL
	struct kobject *keyboard_kobj;
#endif

	di = container_of(info, struct gpio_event_input_info, info);

	if (func == GPIO_EVENT_FUNC_SUSPEND) {
		if (ds->use_irq)
			for (i = 0; i < di->keymap_size; i++)
				disable_irq(gpio_to_irq(di->keymap[i].gpio));
		hrtimer_cancel(&ds->timer);
		return 0;
	}
	if (func == GPIO_EVENT_FUNC_RESUME) {
		spin_lock_irqsave(&ds->irq_lock, irqflags);
		if (ds->use_irq)
			for (i = 0; i < di->keymap_size; i++)
				enable_irq(gpio_to_irq(di->keymap[i].gpio));
		hrtimer_start(&ds->timer, ktime_set(0, 0), HRTIMER_MODE_REL);
		spin_unlock_irqrestore(&ds->irq_lock, irqflags);
		return 0;
	}

	if (func == GPIO_EVENT_FUNC_INIT) {
		if (ktime_to_ns(di->poll_time) <= 0)
			di->poll_time = ktime_set(0, 20 * NSEC_PER_MSEC);

		*data = ds = kzalloc(sizeof(*ds) + sizeof(ds->key_state[0]) *
					di->keymap_size, GFP_KERNEL);
		if (ds == NULL) {
			ret = -ENOMEM;
			pr_err("gpio_event_input_func: "
				"Failed to allocate private data\n");
			goto err_ds_alloc_failed;
		}
		ds->debounce_count = di->keymap_size;
		ds->input_devs = input_devs;
		ds->info = di;
		wake_lock_init(&ds->wake_lock, WAKE_LOCK_SUSPEND, "gpio_input");
		spin_lock_init(&ds->irq_lock);

		for (i = 0; i < di->keymap_size; i++) {
			int dev = di->keymap[i].dev;
			if (dev >= input_devs->count) {
				pr_err("gpio_event_input_func: bad device "
					"index %d >= %d for key code %d\n",
					dev, input_devs->count,
					di->keymap[i].code);
				ret = -EINVAL;
				goto err_bad_keymap;
			}
			input_set_capability(input_devs->dev[dev], di->type,
					     di->keymap[i].code);
			ds->key_state[i].ds = ds;
			ds->key_state[i].debounce = DEBOUNCE_UNKNOWN;
		}

		for (i = 0; i < di->keymap_size; i++) {
			ret = gpio_request(di->keymap[i].gpio, "gpio_kp_in");
			if (ret) {
				pr_err("gpio_event_input_func: gpio_request "
					"failed for %d\n", di->keymap[i].gpio);
				goto err_gpio_request_failed;
			}
			ret = gpio_direction_input(di->keymap[i].gpio);
			if (ret) {
				pr_err("gpio_event_input_func: "
					"gpio_direction_input failed for %d\n",
					di->keymap[i].gpio);
				goto err_gpio_configure_failed;
			}
		}

		ret = gpio_event_input_request_irqs(ds);

#ifdef CONFIG_HTC_WAKE_ON_VOL
		keyboard_kobj = kobject_create_and_add("keyboard", NULL);
		if (keyboard_kobj == NULL) {
			printk(KERN_ERR "KEY_ERR: %s: subsystem_register failed\n", __func__);
			ret = -ENOMEM;
			return ret;
		}
		if (sysfs_create_file(keyboard_kobj, &dev_attr_vol_wakeup.attr))
			printk(KERN_ERR "KEY_ERR: %s: sysfs_create_file "
					"return %d\n", __func__, ret);
		wakeup_bitmask = 0;
		set_wakeup = 0;
#endif

		spin_lock_irqsave(&ds->irq_lock, irqflags);
		ds->use_irq = ret == 0;

		pr_info("GPIO Input Driver: Start gpio inputs for %s%s in %s "
			"mode\n", input_devs->dev[0]->name,
			(input_devs->count > 1) ? "..." : "",
			ret == 0 ? "interrupt" : "polling");

		hrtimer_init(&ds->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ds->timer.function = gpio_event_input_timer_func;
		hrtimer_start(&ds->timer, ktime_set(0, 0), HRTIMER_MODE_REL);
		spin_unlock_irqrestore(&ds->irq_lock, irqflags);
		return 0;
	}

	ret = 0;
	spin_lock_irqsave(&ds->irq_lock, irqflags);
	hrtimer_cancel(&ds->timer);
	if (ds->use_irq) {
		for (i = di->keymap_size - 1; i >= 0; i--) {
			int irq = gpio_to_irq(di->keymap[i].gpio);
			if (ds->info->info.no_suspend)
				disable_irq_wake(irq);
			free_irq(irq, &ds->key_state[i]);
		}
	}
	spin_unlock_irqrestore(&ds->irq_lock, irqflags);

	for (i = di->keymap_size - 1; i >= 0; i--) {
err_gpio_configure_failed:
		gpio_free(di->keymap[i].gpio);
err_gpio_request_failed:
		;
	}
err_bad_keymap:
	wake_lock_destroy(&ds->wake_lock);
	kfree(ds);
err_ds_alloc_failed:
	return ret;
}
