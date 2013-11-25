/* drivers/input/keyreset.c
 *
 * Copyright (C) 2008 Google, Inc.
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

#include <linux/input.h>
#include <linux/keyreset.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <mach/board.h>
#include <mach/msm_watchdog.h>
#include <mach/board_htc.h>
#include <mach/msm_smsm.h>

#define KEYRESET_DELAY 3*HZ

struct keyreset_state {
	struct input_handler input_handler;
	unsigned long keybit[BITS_TO_LONGS(KEY_CNT)];
	unsigned long upbit[BITS_TO_LONGS(KEY_CNT)];
	unsigned long key[BITS_TO_LONGS(KEY_CNT)];
	spinlock_t lock;
	int key_down_target;
	int key_down;
	int key_up;
	int restart_disabled;
	int (*reset_fn)(void);
};

static int restart_requested;
static unsigned long restart_timeout;

static void deferred_restart(struct work_struct *dummy)
{
	unsigned long kernel_flag = get_kernel_flag();

	if ((kernel_flag & KERNEL_FLAG_ENABLE_SSR_MODEM) && (kernel_flag & KERNEL_FLAG_ENABLE_SSR_WCNSS)) {
		pr_info("keyreset::%s combination key can't support modem & wcnss restart at the same time because shared SMSM_RESET !!!\n", __func__);
	} else if (kernel_flag & KERNEL_FLAG_ENABLE_SSR_MODEM) {
		pr_info("keyreset::%s to trigger modem restart\n", __func__);
		smsm_change_state_ssr(SMSM_APPS_STATE, SMSM_APPS_SEND_MODEM_FATAL, 0, KERNEL_FLAG_ENABLE_SSR_MODEM);
		smsm_change_state_ssr(SMSM_APPS_STATE, 0, SMSM_APPS_SEND_MODEM_FATAL, KERNEL_FLAG_ENABLE_SSR_MODEM);
	} else if (kernel_flag & KERNEL_FLAG_ENABLE_SSR_WCNSS) {
		pr_info("keyreset::%s to trigger wcnss restart\n", __func__);
		smsm_change_state_ssr(SMSM_APPS_STATE, 0, SMSM_RESET, KERNEL_FLAG_ENABLE_SSR_WCNSS);
	} else {
		pr_info("keyreset::%s in\n", __func__);
		restart_requested = 2;
		sys_sync();
		restart_requested = 3;
		kernel_restart(NULL);
	}
}
static DECLARE_DELAYED_WORK(restart_work, deferred_restart);

static void keyreset_event_ssr(struct input_handle *handle, unsigned int type,
			   unsigned int code, int value)
{
	unsigned long flags;
	struct keyreset_state *state = handle->private;

	if (type != EV_KEY)
		return;

	if (code >= KEY_MAX)
		return;

	if (!test_bit(code, state->keybit))
		return;

	spin_lock_irqsave(&state->lock, flags);
	if (!test_bit(code, state->key) == !value)
		goto done;
	__change_bit(code, state->key);
	if (test_bit(code, state->upbit)) {
		if (value) {
			state->restart_disabled = 1;
			state->key_up++;
		} else
			state->key_up--;
	} else {
		if (value)
			state->key_down++;
		else
			state->key_down--;
	}
	if (state->key_down == 0 && state->key_up == 0)
		state->restart_disabled = 0;

	pr_info("reset key changed %d %d new state %d-%d-%d\n", code, value,
		 state->key_down, state->key_up, state->restart_disabled);

	if (value && !state->restart_disabled &&
	    state->key_down == state->key_down_target) {

#if 0 
		if (state->reset_fn) {
			restart_requested = state->reset_fn();
		} else {
#endif
			pr_info("keyboard reset\n");
			schedule_delayed_work(&restart_work, KEYRESET_DELAY);
			restart_requested = 1;
	} else if (restart_requested == 1) {
		if (cancel_delayed_work(&restart_work)) {
			pr_info("%s: cancel restart work\n", __func__);
			restart_requested = 0;
		} else
			pr_info("%s: cancel failed\n", __func__);
	}
done:
	spin_unlock_irqrestore(&state->lock, flags);
}
static void keyreset_event(struct input_handle *handle, unsigned int type,
			   unsigned int code, int value)
{
	unsigned long flags;
	struct keyreset_state *state = handle->private;

	if (type != EV_KEY)
		return;

	if (code >= KEY_MAX)
		return;

	if (!test_bit(code, state->keybit))
		return;

	spin_lock_irqsave(&state->lock, flags);
	if (!test_bit(code, state->key) == !value)
		goto done;
	__change_bit(code, state->key);
	if (test_bit(code, state->upbit)) {
		if (value) {
			state->restart_disabled = 1;
			state->key_up++;
		} else
			state->key_up--;
	} else {
		if (value)
			state->key_down++;
		else
			state->key_down--;
	}
	if (state->key_down == 0 && state->key_up == 0)
		state->restart_disabled = 0;

	pr_debug("reset key changed %d %d new state %d-%d-%d\n", code, value,
		 state->key_down, state->key_up, state->restart_disabled);

	if (value && !state->restart_disabled &&
	    state->key_down == state->key_down_target) {
		state->restart_disabled = 1;
		if (restart_requested) {
			msm_watchdog_suspend(NULL);
			
			printk(KERN_INFO "\n### Show Blocked State ###\n");
			show_state_filter(TASK_UNINTERRUPTIBLE);
			msm_watchdog_resume(NULL);
			if (time_after(jiffies, restart_timeout))
				panic("keyboard reset failed, %d", restart_requested);
			return;
		}
		if (state->reset_fn) {
			restart_requested = state->reset_fn();
		} else {
			pr_info("keyboard reset\n");
			schedule_delayed_work(&restart_work, KEYRESET_DELAY);
			restart_requested = 1;
			restart_timeout = jiffies + 20 * HZ;
			msm_watchdog_suspend(NULL);
			
			printk(KERN_INFO "\n### Show Blocked State ###\n");
			show_state_filter(TASK_UNINTERRUPTIBLE);
			msm_watchdog_resume(NULL);
		}
	} else if (restart_requested == 1) {
		if (cancel_delayed_work(&restart_work)) {
			pr_info("%s: cancel restart work\n", __func__);
			restart_requested = 0;
		} else
			pr_info("%s: cancel failed\n", __func__);
	}
done:
	spin_unlock_irqrestore(&state->lock, flags);
}

static int keyreset_connect(struct input_handler *handler,
					  struct input_dev *dev,
					  const struct input_device_id *id)
{
	int i;
	int ret;
	struct input_handle *handle;
	struct keyreset_state *state =
		container_of(handler, struct keyreset_state, input_handler);

	for (i = 0; i < KEY_MAX; i++) {
		if (test_bit(i, state->keybit) && test_bit(i, dev->keybit))
			break;
	}
	if (i == KEY_MAX)
		return -ENODEV;

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "keyreset";
	handle->private = state;

	ret = input_register_handle(handle);
	if (ret)
		goto err_input_register_handle;

	ret = input_open_device(handle);
	if (ret)
		goto err_input_open_device;

	pr_info("using input dev %s for key reset\n", dev->name);

	return 0;

err_input_open_device:
	input_unregister_handle(handle);
err_input_register_handle:
	kfree(handle);
	return ret;
}

static void keyreset_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id keyreset_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_KEY) },
	},
	{ },
};
MODULE_DEVICE_TABLE(input, keyreset_ids);

static int keyreset_probe(struct platform_device *pdev)
{
	int ret;
	int key, *keyp;
	struct keyreset_state *state;
	struct keyreset_platform_data *pdata = pdev->dev.platform_data;

	if (!board_build_flag()) {
		printk(KERN_INFO "[KEY] Ship code, disable key reset.\n");
		return 0;
	}

	if (!pdata)
		return -EINVAL;

	state = kzalloc(sizeof(*state), GFP_KERNEL);
	if (!state)
		return -ENOMEM;

	spin_lock_init(&state->lock);
	if (get_kernel_flag() & (KERNEL_FLAG_ENABLE_SSR_MODEM | KERNEL_FLAG_ENABLE_SSR_WCNSS)) {
		pdata->keys_down[0] = KEY_VOLUMEDOWN;
		pdata->keys_down[1] = KEY_VOLUMEUP;
		pdata->keys_down[2] = 0;
	}
	keyp = pdata->keys_down;
	while ((key = *keyp++)) {
		if (key >= KEY_MAX)
			continue;
		state->key_down_target++;
		__set_bit(key, state->keybit);
	}
	if (pdata->keys_up) {
		keyp = pdata->keys_up;
		while ((key = *keyp++)) {
			if (key >= KEY_MAX)
				continue;
			__set_bit(key, state->keybit);
			__set_bit(key, state->upbit);
		}
	}

	if (pdata->reset_fn)
		state->reset_fn = pdata->reset_fn;

	if (get_kernel_flag() & (KERNEL_FLAG_ENABLE_SSR_MODEM | KERNEL_FLAG_ENABLE_SSR_WCNSS))
		state->input_handler.event = keyreset_event_ssr;
	else
		state->input_handler.event = keyreset_event;

	state->input_handler.connect = keyreset_connect;
	state->input_handler.disconnect = keyreset_disconnect;
	state->input_handler.name = KEYRESET_NAME;
	state->input_handler.id_table = keyreset_ids;
	ret = input_register_handler(&state->input_handler);
	if (ret) {
		kfree(state);
		return ret;
	}
	platform_set_drvdata(pdev, state);
	return 0;
}

int keyreset_remove(struct platform_device *pdev)
{
	struct keyreset_state *state = platform_get_drvdata(pdev);
	if (board_build_flag()) {
		input_unregister_handler(&state->input_handler);
		kfree(state);
	}
	return 0;
}


struct platform_driver keyreset_driver = {
	.driver.name = KEYRESET_NAME,
	.probe = keyreset_probe,
	.remove = keyreset_remove,
};

static int __init keyreset_init(void)
{
	return platform_driver_register(&keyreset_driver);
}

static void __exit keyreset_exit(void)
{
	return platform_driver_unregister(&keyreset_driver);
}

module_init(keyreset_init);
module_exit(keyreset_exit);
