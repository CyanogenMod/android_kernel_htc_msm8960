/*
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/power_supply.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/wakelock.h>
#include <linux/gpio.h>
#include <linux/rtc.h>
#include <linux/workqueue.h>
#include <mach/htc_battery_core.h>
#include <linux/android_alarm.h>
#include <mach/board_htc.h>


static ssize_t htc_battery_show_property(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static ssize_t htc_battery_rt_attr_show(struct device *dev,
					struct device_attribute *attr,
					char *buf);

static int htc_power_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val);

static int htc_battery_get_property(struct power_supply *psy,
				    enum power_supply_property psp,
				    union power_supply_propval *val);

static ssize_t htc_battery_charger_ctrl_timer(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count);

#if 1
#define HTC_BATTERY_ATTR(_name)                                             \
{                                                                           \
	.attr = { .name = #_name, .mode = S_IRUGO},  \
	.show = htc_battery_show_property,                                  \
	.store = NULL,                                                      \
}
#else
#define HTC_BATTERY_ATTR(_name)                                             \
{                                                                           \
	.attr = { .name = #_name, .mode = S_IRUGO, .owner = THIS_MODULE },  \
	.show = htc_battery_show_property,                                  \
	.store = NULL,                                                      \
}
#endif

struct htc_battery_core_info {
	int present;
	int htc_charge_full;
	unsigned long update_time;
	struct mutex info_lock;
	struct battery_info_reply rep;
	struct htc_battery_core func;
};

static struct htc_battery_core_info battery_core_info;
static int battery_register = 1;
static int battery_over_loading;

static struct alarm batt_charger_ctrl_alarm;
static struct work_struct batt_charger_ctrl_work;
struct workqueue_struct *batt_charger_ctrl_wq;
static unsigned int charger_ctrl_stat;
static unsigned int phone_call_stat;

static int test_power_monitor;

static enum power_supply_property htc_battery_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_OVERLOAD,
};

static enum power_supply_property htc_power_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *supply_list[] = {
	"battery",
};

static struct power_supply htc_power_supplies[] = {
	{
		.name = "battery",
		.type = POWER_SUPPLY_TYPE_BATTERY,
		.properties = htc_battery_properties,
		.num_properties = ARRAY_SIZE(htc_battery_properties),
		.get_property = htc_battery_get_property,
	},
	{
		.name = "usb",
		.type = POWER_SUPPLY_TYPE_USB,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = htc_power_properties,
		.num_properties = ARRAY_SIZE(htc_power_properties),
		.get_property = htc_power_get_property,
	},
	{
		.name = "ac",
		.type = POWER_SUPPLY_TYPE_MAINS,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = htc_power_properties,
		.num_properties = ARRAY_SIZE(htc_power_properties),
		.get_property = htc_power_get_property,
	},
	{
		.name = "wireless",
		.type = POWER_SUPPLY_TYPE_WIRELESS,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = htc_power_properties,
		.num_properties = ARRAY_SIZE(htc_power_properties),
		.get_property = htc_power_get_property,
	},
};

static BLOCKING_NOTIFIER_HEAD(wireless_charger_notifier_list);
int register_notifier_wireless_charger(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&wireless_charger_notifier_list, nb);
}

int unregister_notifier_wireless_charger(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&wireless_charger_notifier_list, nb);
}

static int zcharge_enabled;
int htc_battery_get_zcharge_mode(void)
{
	return zcharge_enabled;
}
static int __init enable_zcharge_setup(char *str)
{
	int rc;
	unsigned long cal;

	rc = strict_strtoul(str, 10, &cal);

	if (rc)
		return rc;

	zcharge_enabled = cal;
	return 1;
}
__setup("enable_zcharge=", enable_zcharge_setup);

static int htc_battery_get_charging_status(void)
{
	enum charger_type_t charger;
	int ret;

	mutex_lock(&battery_core_info.info_lock);
	charger = battery_core_info.rep.charging_source;
	mutex_unlock(&battery_core_info.info_lock);

	if (battery_core_info.rep.batt_id == 255)
		charger = CHARGER_UNKNOWN;

	switch (charger) {
	case CHARGER_BATTERY:
		ret = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	case CHARGER_USB:
	case CHARGER_AC:
	case CHARGER_9V_AC:
	case CHARGER_WIRELESS:
	case CHARGER_MHL_AC:
	case CHARGER_DETECTING:
	case CHARGER_UNKNOWN_USB:
	case CHARGER_NOTIFY:
		if (battery_core_info.htc_charge_full)
			ret = POWER_SUPPLY_STATUS_FULL;
		else {
			if (battery_core_info.rep.charging_enabled != 0)
				ret = POWER_SUPPLY_STATUS_CHARGING;
			else
				ret = POWER_SUPPLY_STATUS_DISCHARGING;
		}
		break;
	default:
		ret = POWER_SUPPLY_STATUS_UNKNOWN;
	}

	return ret;
}

static ssize_t htc_battery_show_batt_attr(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return battery_core_info.func.func_show_batt_attr(attr, buf);
}

static ssize_t htc_battery_show_cc_attr(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return battery_core_info.func.func_show_cc_attr(attr, buf);
}

static ssize_t htc_battery_show_htc_extension_attr(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	if (battery_core_info.func.func_show_htc_extension_attr)
		return battery_core_info.func.func_show_htc_extension_attr(attr, buf);
	return 0;
}

static ssize_t htc_battery_set_delta(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long delta = 0;

	delta = simple_strtoul(buf, NULL, 10);

	if (delta > 100)
		return -EINVAL;

	return count;
}

static ssize_t htc_battery_debug_flag(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long debug_flag;
	debug_flag = simple_strtoul(buf, NULL, 10);

	if (debug_flag > 100 || debug_flag == 0)
		return -EINVAL;

	return 0;
}

static ssize_t htc_battery_set_full_level(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int rc = 0;
	unsigned long percent = 100;

	rc = strict_strtoul(buf, 10, &percent);
	if (rc)
		return rc;

	if (percent > 100 || percent == 0)
		return -EINVAL;

	if (!battery_core_info.func.func_set_full_level) {
		BATT_ERR("No set full level function!");
		return -ENOENT;
	}

	battery_core_info.func.func_set_full_level(percent);

	return count;
}

static ssize_t htc_battery_set_full_level_dis_batt_chg(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int rc = 0;
	unsigned long percent = 100;

	rc = strict_strtoul(buf, 10, &percent);
	if (rc)
		return rc;

	if (percent > 100 || percent == 0)
		return -EINVAL;

	if (!battery_core_info.func.func_set_full_level_dis_batt_chg) {
		BATT_ERR("No set full level (disable battery charging only) function!");
		return -ENOENT;
	}

	battery_core_info.func.func_set_full_level_dis_batt_chg(percent);

	return count;
}

int htc_battery_charger_disable()
{
	int rc = 0;

	if (!battery_core_info.func.func_charger_control) {
		BATT_ERR("No charger control function!");
		return -ENOENT;
	}
	rc = battery_core_info.func.func_charger_control(STOP_CHARGER);
	if (rc < 0)
		BATT_ERR("charger control failed!");

	return rc;
}


int htc_battery_set_max_input_current(int target_ma)
{
	int rc = 0;

	if (!battery_core_info.func.func_set_max_input_current) {
		BATT_ERR("No max input current function!");
		return -ENOENT;
	}
	rc = battery_core_info.func.func_set_max_input_current(target_ma);
	if (rc < 0)
		BATT_ERR("max input current control failed!");

	return rc;
}

int htc_battery_pwrsrc_disable()
{
	int rc = 0;

	if (!battery_core_info.func.func_charger_control) {
		BATT_ERR("No charger control function!");
		return -ENOENT;
	}
	rc = battery_core_info.func.func_charger_control(DISABLE_PWRSRC);
	if (rc < 0)
		BATT_ERR("charger control failed!");

	return rc;
}

static ssize_t htc_battery_charger_stat(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int i = 0;

	i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", charger_ctrl_stat);

	return i;
}

static ssize_t htc_battery_charger_switch(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long enable = 0;
	int rc = 0;

	rc = strict_strtoul(buf, 10, &enable);
	if (rc)
		return rc;

	BATT_LOG("Set charger_control:%lu", enable);
	if (enable >= END_CHARGER)
		return -EINVAL;

	if (!battery_core_info.func.func_charger_control) {
		BATT_ERR("No charger control function!");
		return -ENOENT;
	}

	rc = battery_core_info.func.func_charger_control(enable);
	if (rc < 0) {
		BATT_ERR("charger control failed!");
		return rc;
	}
	charger_ctrl_stat = enable;

	alarm_cancel(&batt_charger_ctrl_alarm);

	return count;
}

static ssize_t htc_battery_phone_call_stat(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	int i = 0;

	i += scnprintf(buf + i, PAGE_SIZE - i, "%u\n", phone_call_stat);

	return i;
}

static ssize_t htc_battery_set_phone_call(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long phone_call = 0;
	int rc = 0;

	rc = strict_strtoul(buf, 10, &phone_call);
	if (rc)
		return rc;

	BATT_LOG("set context phone_call=%lu", phone_call);

	if (!battery_core_info.func.func_context_event_handler) {
		BATT_ERR("No context_event_notify function!");
		return -ENOENT;
	}

	if (phone_call)
		battery_core_info.func.func_context_event_handler(EVENT_TALK_START);
	else
		battery_core_info.func.func_context_event_handler(EVENT_TALK_STOP);

	 phone_call_stat = phone_call;

	return count;
}
static ssize_t htc_battery_set_network_search(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long network_search = 0;
	int rc = 0;

	rc = strict_strtoul(buf, 10, &network_search);
	if (rc)
		return rc;

	BATT_LOG("Set context network_search=%lu", network_search);

	if (!battery_core_info.func.func_context_event_handler) {
		BATT_ERR("No context_event_notify function!");
		return -ENOENT;
	}

	if (network_search) {
		battery_core_info.func.func_context_event_handler(
									EVENT_NETWORK_SEARCH_START);
	} else {
		battery_core_info.func.func_context_event_handler(
									EVENT_NETWORK_SEARCH_STOP);
	}

	return count;
}
static ssize_t htc_battery_set_navigation(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long navigation = 0;
	int rc = 0;

	rc = strict_strtoul(buf, 10, &navigation);
	if (rc)
		return rc;

	BATT_LOG("Set context navigation=%lu", navigation);

	if (!battery_core_info.func.func_context_event_handler) {
		BATT_ERR("No context_event_notify function!");
		return -ENOENT;
	}

	if (navigation) {
		battery_core_info.func.func_context_event_handler(
									EVENT_NAVIGATION_START);
	} else {
		battery_core_info.func.func_context_event_handler(
									EVENT_NAVIGATION_STOP);
	}

	return count;
}
static ssize_t htc_battery_set_context_event(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned long event = 0;
	int rc = 0;

	rc = strict_strtoul(buf, 10, &event);
	if (rc)
		return rc;

	BATT_LOG("Set context event = %lu", event);

	if (!battery_core_info.func.func_context_event_handler) {
		BATT_ERR("No context_event_notify function!");
		return -ENOENT;
	}

	battery_core_info.func.func_context_event_handler(event);

	return count;
}

static ssize_t htc_battery_set_disable_limit_chg(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int rc = 0;
	unsigned long disable_limit_chg = 0;

	rc = strict_strtoul(buf, 10, &disable_limit_chg);
	if (rc)
		return rc;

	BATT_LOG("Set context disable_limit_chg = %lu", disable_limit_chg);

	if((disable_limit_chg != 0) && (disable_limit_chg != 1))
		return -EINVAL;

	if (!battery_core_info.func.func_context_event_handler) {
		BATT_ERR("No context_event_notify function!");
		return -ENOENT;
	}

	if (disable_limit_chg)
		battery_core_info.func.func_context_event_handler(EVENT_DAYDREAM_START);
	else
		battery_core_info.func.func_context_event_handler(EVENT_DAYDREAM_STOP);

	return count;
}

static ssize_t htc_battery_trigger_store_battery_data(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int rc = 0;
	unsigned long trigger_flag = 0;

	rc = strict_strtoul(buf, 10, &trigger_flag);
	if (rc)
		return rc;

	BATT_LOG("Set context trigger_flag = %lu", trigger_flag);

	if((trigger_flag != 0) && (trigger_flag != 1))
		return -EINVAL;

	if (!battery_core_info.func.func_trigger_store_battery_data) {
		BATT_ERR("No set trigger store battery data function!");
		return -ENOENT;
	}

	battery_core_info.func.func_trigger_store_battery_data(trigger_flag);

	return count;
}

static ssize_t htc_battery_qb_mode_shutdown_status(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int rc = 0;
	unsigned long trigger_flag = 0;

	rc = strict_strtoul(buf, 10, &trigger_flag);
	if (rc)
		return rc;

	BATT_LOG("Set context trigger_flag = %lu", trigger_flag);

	if((trigger_flag != 0) && (trigger_flag != 1))
		return -EINVAL;

	if (!battery_core_info.func.func_qb_mode_shutdown_status) {
		BATT_ERR("No set trigger qb mode shutdown status function!");
		return -ENOENT;
	}

	battery_core_info.func.func_qb_mode_shutdown_status(trigger_flag);

	return count;
}

static struct device_attribute htc_battery_attrs[] = {
	HTC_BATTERY_ATTR(batt_id),
	HTC_BATTERY_ATTR(batt_vol),
	HTC_BATTERY_ATTR(batt_temp),
	HTC_BATTERY_ATTR(batt_current),
	HTC_BATTERY_ATTR(charging_source),
	HTC_BATTERY_ATTR(charging_enabled),
	HTC_BATTERY_ATTR(full_bat),
	HTC_BATTERY_ATTR(over_vchg),
	HTC_BATTERY_ATTR(batt_state),
	HTC_BATTERY_ATTR(batt_overload),
	HTC_BATTERY_ATTR(pj_exist),
	HTC_BATTERY_ATTR(pj_status),
	HTC_BATTERY_ATTR(pj_level),
	HTC_BATTERY_ATTR(batt_cable_in),

	__ATTR(batt_attr_text, S_IRUGO, htc_battery_show_batt_attr, NULL),
	__ATTR(batt_power_meter, S_IRUGO, htc_battery_show_cc_attr, NULL),
	__ATTR(htc_extension, S_IRUGO, htc_battery_show_htc_extension_attr, NULL),
};

static struct device_attribute htc_set_delta_attrs[] = {
	__ATTR(delta, S_IWUSR | S_IWGRP, NULL, htc_battery_set_delta),
	__ATTR(full_level, S_IWUSR | S_IWGRP, NULL,
		htc_battery_set_full_level),
	__ATTR(full_level_dis_batt_chg, S_IWUSR | S_IWGRP, NULL,
		htc_battery_set_full_level_dis_batt_chg),
	__ATTR(batt_debug_flag, S_IWUSR | S_IWGRP, NULL,
		htc_battery_debug_flag),
	__ATTR(charger_control, S_IWUSR | S_IWGRP, htc_battery_charger_stat,
		htc_battery_charger_switch),
	__ATTR(charger_timer, S_IWUSR | S_IWGRP, NULL,
		htc_battery_charger_ctrl_timer),
	__ATTR(phone_call, S_IWUSR | S_IWGRP, htc_battery_phone_call_stat,
		htc_battery_set_phone_call),
	__ATTR(network_search, S_IWUSR | S_IWGRP, NULL,
		htc_battery_set_network_search),
	__ATTR(navigation, S_IWUSR | S_IWGRP, NULL,
		htc_battery_set_navigation),
	__ATTR(context_event, S_IWUSR | S_IWGRP, NULL,
		htc_battery_set_context_event),
	__ATTR(disable_limit_chg, S_IWUSR | S_IWGRP, NULL,
		htc_battery_set_disable_limit_chg),
	__ATTR(store_battery_data, S_IWUSR | S_IWGRP, NULL,
		htc_battery_trigger_store_battery_data),
	__ATTR(qb_mode_shutdown, S_IWUSR | S_IWGRP, NULL,
		htc_battery_qb_mode_shutdown_status),
};

static struct device_attribute htc_battery_rt_attrs[] = {
	__ATTR(batt_vol_now, S_IRUGO, htc_battery_rt_attr_show, NULL),
	__ATTR(batt_current_now, S_IRUGO, htc_battery_rt_attr_show, NULL),
	__ATTR(batt_temp_now, S_IRUGO, htc_battery_rt_attr_show, NULL),
	__ATTR(pj_exist_now, S_IRUGO, htc_battery_rt_attr_show, NULL),
	__ATTR(pj_vol_now, S_IRUGO, htc_battery_rt_attr_show, NULL),
	__ATTR(voltage_now, S_IRUGO, htc_battery_rt_attr_show, NULL),
};


static int htc_battery_create_attrs(struct device *dev)
{
	int i = 0, j = 0, k = 0, rc = 0;

	for (i = 0; i < ARRAY_SIZE(htc_battery_attrs); i++) {
		rc = device_create_file(dev, &htc_battery_attrs[i]);
		if (rc)
			goto htc_attrs_failed;
	}

	for (j = 0; j < ARRAY_SIZE(htc_set_delta_attrs); j++) {
		rc = device_create_file(dev, &htc_set_delta_attrs[j]);
		if (rc)
			goto htc_delta_attrs_failed;
	}

	for (k = 0; k < ARRAY_SIZE(htc_battery_rt_attrs); k++) {
		rc = device_create_file(dev, &htc_battery_rt_attrs[k]);
		if (rc)
			goto htc_rt_attrs_failed;
	}

	goto succeed;

htc_rt_attrs_failed:
	while (k--)
		device_remove_file(dev, &htc_battery_rt_attrs[k]);
htc_delta_attrs_failed:
	while (j--)
		device_remove_file(dev, &htc_set_delta_attrs[j]);
htc_attrs_failed:
	while (i--)
		device_remove_file(dev, &htc_battery_attrs[i]);
succeed:
	return rc;
}

static int htc_battery_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = htc_battery_get_charging_status();
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		if (battery_core_info.rep.temp_fault != -1) {
			if (battery_core_info.rep.temp_fault == 1)
				val->intval =  POWER_SUPPLY_HEALTH_OVERHEAT;
		}
		else if (battery_core_info.rep.batt_temp >= 480 ||
			battery_core_info.rep.batt_temp <= 0)
			val->intval =  POWER_SUPPLY_HEALTH_OVERHEAT;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = battery_core_info.present;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		mutex_lock(&battery_core_info.info_lock);
#ifdef CONFIG_HTC_BATT8x60
		if (battery_core_info.htc_charge_full == 0)
			val->intval = battery_core_info.rep.level;
		else
			val->intval = 100;
#else
		val->intval = battery_core_info.rep.level;
#endif
		mutex_unlock(&battery_core_info.info_lock);
		break;
	case POWER_SUPPLY_PROP_OVERLOAD:
		val->intval = battery_core_info.rep.overload;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int htc_power_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	enum charger_type_t charger;

	mutex_lock(&battery_core_info.info_lock);

	charger = battery_core_info.rep.charging_source;

#if 0
	if (battery_core_info.rep.batt_id == 255)
		charger = CHARGER_BATTERY;
#endif

	mutex_unlock(&battery_core_info.info_lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (psy->type == POWER_SUPPLY_TYPE_MAINS) {
			if (charger == CHARGER_AC ||
			    charger == CHARGER_9V_AC
			    ||  charger == CHARGER_MHL_AC)
				val->intval = 1;
			else
				val->intval = 0;
		} else if (psy->type == POWER_SUPPLY_TYPE_USB) {
			if (charger == CHARGER_USB ||
			    charger == CHARGER_UNKNOWN_USB ||
			    charger == CHARGER_DETECTING)
				val->intval = 1;
			else
				val->intval = 0;
		} else if (psy->type == POWER_SUPPLY_TYPE_WIRELESS)
			val->intval = (charger ==  CHARGER_WIRELESS ? 1 : 0);
		else
			val->intval = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static ssize_t htc_battery_show_property(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int i = 0;
	const ptrdiff_t off = attr - htc_battery_attrs;

	mutex_lock(&battery_core_info.info_lock);

	switch (off) {
	case BATT_ID:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery_core_info.rep.batt_id);
		break;
	case BATT_VOL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery_core_info.rep.batt_vol);
		break;
	case BATT_TEMP:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery_core_info.rep.batt_temp);
		break;
	case BATT_CURRENT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery_core_info.rep.batt_current);
		break;
	case CHARGING_SOURCE:
		if(battery_core_info.rep.charging_source == CHARGER_MHL_AC) {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", CHARGER_AC);
		}
		else {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery_core_info.rep.charging_source);
		}
		break;
	case CHARGING_ENABLED:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery_core_info.rep.charging_enabled);
		break;
	case FULL_BAT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery_core_info.rep.full_bat);
		break;
	case OVER_VCHG:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery_core_info.rep.over_vchg);
		break;
	case BATT_STATE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery_core_info.rep.batt_state);
		break;
	case OVERLOAD:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery_core_info.rep.overload);
		break;
	case PJ_EXIST:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery_core_info.rep.pj_src);
		break;
	case PJ_STATUS:
		if (battery_core_info.rep.pj_src) {
			
			if (battery_core_info.rep.pj_full)	{
				if ((battery_core_info.rep.pj_level - battery_core_info.rep.pj_level_pre) >= 19)
					BATT_LOG("level diff over 19, level:%d, pre_level:%d\n",
						battery_core_info.rep.pj_level, battery_core_info.rep.pj_level_pre);
				else
					i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", HTC_UI_PJ_FULL);
			} else { 
				if (battery_core_info.rep.pj_chg_status == 2 || battery_core_info.rep.charging_enabled)
					i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", HTC_UI_PJ_CHG);
				else
					i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", HTC_UI_PJ_NOT_CHG);
			}
		} else {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", HTC_UI_PJ_NOT_CHG);
		}
		break;
	case PJ_LEVEL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery_core_info.rep.pj_level);
		break;
	case BATT_CABLEIN:
		if(battery_core_info.rep.charging_source == CHARGER_BATTERY)
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
		else
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 1);
		break;
	default:
		i = -EINVAL;
	}
	mutex_unlock(&battery_core_info.info_lock);

	if (i < 0)
		BATT_ERR("%s: battery: attribute is not supported: %d",
			__func__, off);

	return i;
}

static ssize_t htc_battery_rt_attr_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int i = 0;
	int val = 0;
	int rc = 0;
	const ptrdiff_t attr_index = attr - htc_battery_rt_attrs;

	if (!battery_core_info.func.func_get_batt_rt_attr) {
		BATT_ERR("%s: func_get_batt_rt_attr does not exist", __func__);
		return -EINVAL;
	}

	rc = battery_core_info.func.func_get_batt_rt_attr(attr_index, &val);
	if (rc) {
		BATT_ERR("%s: get_batt_rt_attrs[%d] failed", __func__, attr_index);
		return -EINVAL;
	}

	i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", val);
	return i;
}

static ssize_t htc_battery_charger_ctrl_timer(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int rc;
	unsigned long time_out = 0;
	ktime_t interval;
	ktime_t next_alarm;

	rc = strict_strtoul(buf, 10, &time_out);
	if (rc)
		return rc;

	if (time_out > 65536)
		return -EINVAL;

	if (time_out > 0) {
		rc = battery_core_info.func.func_charger_control(STOP_CHARGER);
		if (rc < 0) {
			BATT_ERR("charger control failed!");
			return rc;
		}
		interval = ktime_set(time_out, 0);
		next_alarm = ktime_add(alarm_get_elapsed_realtime(), interval);
		alarm_start_range(&batt_charger_ctrl_alarm,
					next_alarm, next_alarm);
		charger_ctrl_stat = STOP_CHARGER;
	} else if (time_out == 0) {
		rc = battery_core_info.func.func_charger_control(
							ENABLE_CHARGER);
		if (rc < 0) {
			BATT_ERR("charger control failed!");
			return rc;
		}
		alarm_cancel(&batt_charger_ctrl_alarm);
		charger_ctrl_stat = ENABLE_CHARGER;
	}

	return count;
}

static void batt_charger_ctrl_func(struct work_struct *work)
{
	int rc;

	rc = battery_core_info.func.func_charger_control(ENABLE_CHARGER);
	if (rc) {
		BATT_ERR("charger control failed!");
		return;
	}

	charger_ctrl_stat = (unsigned int)ENABLE_CHARGER;
}

static void batt_charger_ctrl_alarm_handler(struct alarm *alarm)
{
	BATT_LOG("charger control alarm is timeout.");

	queue_work(batt_charger_ctrl_wq, &batt_charger_ctrl_work);
}

int htc_battery_core_update_changed(void)
{
	struct battery_info_reply new_batt_info_rep;
	int is_send_batt_uevent = 0;
	int is_send_usb_uevent = 0;
	int is_send_ac_uevent = 0;
	int is_send_wireless_charger_uevent = 0;
	static int batt_temp_over_68c_count = 0;

	if (battery_register) {
		BATT_ERR("No battery driver exists.");
		return -1;
	}

	mutex_lock(&battery_core_info.info_lock);
	memcpy(&new_batt_info_rep, &battery_core_info.rep, sizeof(struct battery_info_reply));
	mutex_unlock(&battery_core_info.info_lock);
	if (battery_core_info.func.func_get_battery_info) {
		battery_core_info.func.func_get_battery_info(&new_batt_info_rep);
	} else {
		BATT_ERR("no func_get_battery_info hooked.");
		return -EINVAL;
	}

	mutex_lock(&battery_core_info.info_lock);
	if (battery_core_info.rep.charging_source != new_batt_info_rep.charging_source) {
		if (CHARGER_BATTERY == battery_core_info.rep.charging_source ||
			CHARGER_BATTERY == new_batt_info_rep.charging_source)
			is_send_batt_uevent = 1;
		if (CHARGER_USB == battery_core_info.rep.charging_source ||
			CHARGER_USB == new_batt_info_rep.charging_source)
			is_send_usb_uevent = 1;
		if (CHARGER_DETECTING == battery_core_info.rep.charging_source ||
			CHARGER_DETECTING == new_batt_info_rep.charging_source)
			is_send_usb_uevent = 1;
		if (CHARGER_UNKNOWN_USB == battery_core_info.rep.charging_source ||
			CHARGER_UNKNOWN_USB == new_batt_info_rep.charging_source)
			is_send_usb_uevent = 1;
		if (CHARGER_AC == battery_core_info.rep.charging_source ||
			CHARGER_AC == new_batt_info_rep.charging_source)
			is_send_ac_uevent = 1;
		if (CHARGER_9V_AC == battery_core_info.rep.charging_source ||
			CHARGER_9V_AC == new_batt_info_rep.charging_source)
			is_send_ac_uevent = 1;
		if (CHARGER_MHL_AC == battery_core_info.rep.charging_source ||
			CHARGER_MHL_AC == new_batt_info_rep.charging_source)
			is_send_ac_uevent = 1;
		if (CHARGER_WIRELESS == battery_core_info.rep.charging_source ||
			CHARGER_WIRELESS == new_batt_info_rep.charging_source)
			is_send_wireless_charger_uevent = 1;

	}
	if ((!is_send_batt_uevent) &&
		((battery_core_info.rep.level != new_batt_info_rep.level) ||
		(battery_core_info.rep.batt_vol != new_batt_info_rep.batt_vol) ||
		(battery_core_info.rep.over_vchg != new_batt_info_rep.over_vchg) ||
		(battery_core_info.rep.batt_temp != new_batt_info_rep.batt_temp) ||
		(battery_core_info.rep.pj_full!= new_batt_info_rep.pj_full) ||
		(battery_core_info.rep.pj_src!= new_batt_info_rep.pj_src) ||
		(battery_core_info.rep.pj_chg_status!= new_batt_info_rep.pj_chg_status))) {
		is_send_batt_uevent = 1;
	}

	if ((battery_core_info.rep.charging_enabled != 0) &&
		(new_batt_info_rep.charging_enabled != 0)) {
		if (battery_core_info.rep.level > new_batt_info_rep.level)
			battery_over_loading++;
		else
			battery_over_loading = 0;
	}

	
	if (battery_core_info.func.func_notify_pnpmgr_charging_enabled) {
		if (battery_core_info.rep.charging_enabled !=
				new_batt_info_rep.charging_enabled)
			battery_core_info.func.func_notify_pnpmgr_charging_enabled(
										new_batt_info_rep.charging_enabled);
	}

	memcpy(&battery_core_info.rep, &new_batt_info_rep, sizeof(struct battery_info_reply));

	if (battery_core_info.rep.batt_temp > 680) {
		batt_temp_over_68c_count++;
		if (batt_temp_over_68c_count < 3) {
			pr_info("[BATT] batt_temp_over_68c_count=%d, (temp=%d)\n",
					batt_temp_over_68c_count, battery_core_info.rep.batt_temp);
			battery_core_info.rep.batt_temp = 680;
		}
	} else {
		
		batt_temp_over_68c_count = 0;
	}

	
	if (test_power_monitor) {
		BATT_LOG("test_power_monitor is set: overwrite fake batt info.");
		battery_core_info.rep.batt_id = 77;
		battery_core_info.rep.batt_temp = 330;
		battery_core_info.rep.level = 77;
		battery_core_info.rep.temp_fault = 0;
	}

	if (battery_core_info.rep.charging_source <= 0) {
		if (battery_core_info.rep.batt_id == 255) {
			pr_info("[BATT] Ignore invalid id when no charging_source");
			battery_core_info.rep.batt_id = 66;
		}
	}
#if 0
	battery_core_info.rep.batt_vol = new_batt_info_rep.batt_vol;
	battery_core_info.rep.batt_id = new_batt_info_rep.batt_id;
	battery_core_info.rep.batt_temp = new_batt_info_rep.batt_temp;
	battery_core_info.rep.batt_current = new_batt_info_rep.batt_current;
	battery_core_info.rep.batt_discharg_current = new_batt_info_rep.batt_discharg_current;
	battery_core_info.rep.level = new_batt_info_rep.level;
	battery_core_info.rep.charging_source = new_batt_info_rep.charging_source;
	battery_core_info.rep.charging_enabled = new_batt_info_rep.charging_enabled;
	battery_core_info.rep.full_bat = new_batt_info_rep.full_bat;
	battery_core_info.rep.over_vchg = new_batt_info_rep.over_vchg;
	battery_core_info.rep.temp_fault = new_batt_info_rep.temp_fault;
	battery_core_info.rep.batt_state = new_batt_info_rep.batt_state;
#endif

	if (battery_core_info.rep.charging_source == CHARGER_BATTERY)
		battery_core_info.htc_charge_full = 0;
	else {
#ifdef CONFIG_HTC_BATT8x60
		if (battery_core_info.htc_charge_full &&
				(battery_core_info.rep.full_level == 100))
			battery_core_info.htc_charge_full = 1;
#else
		if (battery_core_info.htc_charge_full &&
				(battery_core_info.rep.level == 100))
			battery_core_info.htc_charge_full = 1;
#endif
		else {
			if (battery_core_info.rep.level == 100)
				battery_core_info.htc_charge_full = 1;
			else
				battery_core_info.htc_charge_full = 0;
		}

		
		if (battery_over_loading >= 2) {
			battery_core_info.htc_charge_full = 0;
			battery_over_loading = 0;
		}
	}

	battery_core_info.update_time = jiffies;
	mutex_unlock(&battery_core_info.info_lock);

	BATT_LOG("ID=%d,level=%d,level_raw=%d,vol=%d,temp=%d,current=%d,"
		"chg_src=%d,chg_en=%d,pj_src=%d,pj_level=%d,full_bat=%d,"
		"over_vchg=%d,batt_state=%d,overload=%d,ui_chg_full=%d",
			battery_core_info.rep.batt_id,
			battery_core_info.rep.level,
			battery_core_info.rep.level_raw,
			battery_core_info.rep.batt_vol,
			battery_core_info.rep.batt_temp,
			battery_core_info.rep.batt_current,
			battery_core_info.rep.charging_source,
			battery_core_info.rep.charging_enabled,
			battery_core_info.rep.pj_src,
			battery_core_info.rep.pj_level,
			battery_core_info.rep.full_bat,
			battery_core_info.rep.over_vchg,
			battery_core_info.rep.batt_state,
			battery_core_info.rep.overload,
			battery_core_info.htc_charge_full);


	
	if (is_send_batt_uevent) {
		power_supply_changed(&htc_power_supplies[BATTERY_SUPPLY]);
		BATT_LOG("power_supply_changed: battery");
	}
	if (is_send_usb_uevent) {
		power_supply_changed(&htc_power_supplies[USB_SUPPLY]);
		BATT_LOG("power_supply_changed: usb");
	}
	if (is_send_ac_uevent) {
		power_supply_changed(&htc_power_supplies[AC_SUPPLY]);
		BATT_LOG("power_supply_changed: ac");
	}
	if (is_send_wireless_charger_uevent) {
		power_supply_changed(&htc_power_supplies[WIRELESS_SUPPLY]);
		BATT_LOG("power_supply_changed: wireless");
	}

	return 0;
}
EXPORT_SYMBOL_GPL(htc_battery_core_update_changed);

int htc_battery_core_register(struct device *dev,
				struct htc_battery_core *htc_battery)
{
	int i, rc = 0;

	if (!battery_register) {
		BATT_ERR("Only one battery driver could exist.");
		return -1;
	}
	battery_register = 0;

	test_power_monitor =
		(get_kernel_flag() & KERNEL_FLAG_TEST_PWR_SUPPLY) ? 1 : 0;

	mutex_init(&battery_core_info.info_lock);

	if (htc_battery->func_get_batt_rt_attr)
		battery_core_info.func.func_get_batt_rt_attr =
					htc_battery->func_get_batt_rt_attr;
	if (htc_battery->func_show_batt_attr)
		battery_core_info.func.func_show_batt_attr =
					htc_battery->func_show_batt_attr;
	if (htc_battery->func_show_cc_attr)
		battery_core_info.func.func_show_cc_attr =
					htc_battery->func_show_cc_attr;
	if (htc_battery->func_show_htc_extension_attr)
		battery_core_info.func.func_show_htc_extension_attr =
					htc_battery->func_show_htc_extension_attr;
	if (htc_battery->func_get_battery_info)
		battery_core_info.func.func_get_battery_info =
					htc_battery->func_get_battery_info;
	if (htc_battery->func_charger_control)
		battery_core_info.func.func_charger_control =
					htc_battery->func_charger_control;
	if (htc_battery->func_set_max_input_current)
		battery_core_info.func.func_set_max_input_current =
					htc_battery->func_set_max_input_current;
	if (htc_battery->func_context_event_handler)
		battery_core_info.func.func_context_event_handler =
					htc_battery->func_context_event_handler;
	if (htc_battery->func_set_full_level)
		battery_core_info.func.func_set_full_level =
					htc_battery->func_set_full_level;
	if (htc_battery->func_set_full_level_dis_batt_chg)
		battery_core_info.func.func_set_full_level_dis_batt_chg =
					htc_battery->func_set_full_level_dis_batt_chg;
	if (htc_battery->func_notify_pnpmgr_charging_enabled)
		battery_core_info.func.func_notify_pnpmgr_charging_enabled =
					htc_battery->func_notify_pnpmgr_charging_enabled;
	if (htc_battery->func_trigger_store_battery_data)
		battery_core_info.func.func_trigger_store_battery_data =
					htc_battery->func_trigger_store_battery_data;
	if (htc_battery->func_qb_mode_shutdown_status)
		battery_core_info.func.func_qb_mode_shutdown_status =
					htc_battery->func_qb_mode_shutdown_status;

	
	for (i = 0; i < ARRAY_SIZE(htc_power_supplies); i++) {
		rc = power_supply_register(dev, &htc_power_supplies[i]);
		if (rc)
			BATT_ERR("Failed to register power supply"
				" (%d)\n", rc);
	}

	
	htc_battery_create_attrs(htc_power_supplies[CHARGER_BATTERY].dev);

	
	charger_ctrl_stat = ENABLE_CHARGER;
	INIT_WORK(&batt_charger_ctrl_work, batt_charger_ctrl_func);
	alarm_init(&batt_charger_ctrl_alarm,
			ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP,
			batt_charger_ctrl_alarm_handler);
	batt_charger_ctrl_wq =
			create_singlethread_workqueue("charger_ctrl_timer");

	
	battery_core_info.update_time = jiffies;
	battery_core_info.present = 1;
	battery_core_info.htc_charge_full = 0;
	battery_core_info.rep.charging_source = CHARGER_BATTERY;
	battery_core_info.rep.batt_id = 1;
	battery_core_info.rep.batt_vol = 4000;
	battery_core_info.rep.batt_temp = 285;
	battery_core_info.rep.batt_current = 162;
	battery_core_info.rep.level = 66;
	battery_core_info.rep.level_raw = 0;
	battery_core_info.rep.full_bat = 1580000;
	battery_core_info.rep.full_level = 100;
	battery_core_info.rep.full_level_dis_batt_chg = 100;
	
	battery_core_info.rep.temp_fault = -1;
	
	battery_core_info.rep.batt_state = 0;
	battery_core_info.rep.overload = 0;

	battery_over_loading = 0;

	return 0;
}
EXPORT_SYMBOL_GPL(htc_battery_core_register);

const struct battery_info_reply* htc_battery_core_get_batt_info_rep(void)
{
	return &battery_core_info.rep;
}
EXPORT_SYMBOL_GPL(htc_battery_core_get_batt_info_rep);
