/* arch/arm/mach-msm/htc_battery_8960.c
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
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/wakelock.h>
#include <linux/gpio.h>
#include <mach/board.h>
#include <asm/mach-types.h>
#include <mach/board_htc.h>
#include <mach/htc_battery_core.h>
#include <mach/htc_battery_8960.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/reboot.h>
#include <linux/miscdevice.h>
#include <linux/pmic8058-xoadc.h>
#include <mach/mpp.h>
#include <linux/android_alarm.h>
#include <linux/suspend.h>
#include <linux/earlysuspend.h>

#include <mach/htc_gauge.h>
#include <mach/htc_charger.h>
#include <mach/htc_battery_cell.h>


/* disable charging reason */
#define HTC_BATT_CHG_DIS_BIT_EOC	(1)
#define HTC_BATT_CHG_DIS_BIT_ID		(1<<1)
#define HTC_BATT_CHG_DIS_BIT_TMP	(1<<2)
#define HTC_BATT_CHG_DIS_BIT_OVP	(1<<3)
#define HTC_BATT_CHG_DIS_BIT_TMR	(1<<4)
#define HTC_BATT_CHG_DIS_BIT_MFG	(1<<5)
#define HTC_BATT_CHG_DIS_BIT_USR_TMR	(1<<6)
static int chg_dis_reason;
static int chg_dis_active_mask = HTC_BATT_CHG_DIS_BIT_ID
								| HTC_BATT_CHG_DIS_BIT_MFG
								| HTC_BATT_CHG_DIS_BIT_TMP
								| HTC_BATT_CHG_DIS_BIT_USR_TMR;
static int chg_dis_control_mask = HTC_BATT_CHG_DIS_BIT_ID
								| HTC_BATT_CHG_DIS_BIT_MFG
								| HTC_BATT_CHG_DIS_BIT_USR_TMR;
/* disable pwrsrc reason */
#define HTC_BATT_PWRSRC_DIS_BIT_MFG		(1)
#define HTC_BATT_PWRSRC_DIS_BIT_API		(1<<1)
static int pwrsrc_dis_reason;

/* for sprint disable charging cmd */
static int chg_dis_user_timer;
/* for charger disable charging by temperature fault */
static int charger_dis_temp_fault;

/* for limited charge */
static int chg_limit_reason;
static int chg_limit_active_mask;

/* for suspend high frequency (5min) */
#define SUSPEND_HIGHFREQ_CHECK_BIT_TALK		(1)
#define SUSPEND_HIGHFREQ_CHECK_BIT_SEARCH	(1<<1)
static int suspend_highfreq_check_reason;

/* for batt_context_state */
#define CONTEXT_STATE_BIT_TALK			(1)
#define CONTEXT_STATE_BIT_SEARCH		(1<<1)
#define CONTEXT_STATE_BIT_NAVIGATION	(1<<2)
static int context_state;

#define BATT_SUSPEND_CHECK_TIME				(3600)
#define BATT_SUSPEND_HIGHFREQ_CHECK_TIME	(300)
#define BATT_TIMER_CHECK_TIME				(360)
#define BATT_TIMER_UPDATE_TIME				(60)

#ifdef CONFIG_ARCH_MSM8X60_LTE
/* MATT: fix me: check style warning */
/* extern int __ref cpu_down(unsigned int cpu); */
/* extern int board_mfg_mode(void); */
#endif

static void mbat_in_func(struct work_struct *work);
struct delayed_work mbat_in_struct;
//static DECLARE_DELAYED_WORK(mbat_in_struct, mbat_in_func);
static struct kset *htc_batt_kset;

#define BATT_REMOVED_SHUTDOWN_DELAY_MS (50)
static void shutdown_worker(struct work_struct *work);
struct delayed_work shutdown_work;
//static DECLARE_DELAYED_WORK(shutdown_work, shutdown_worker);

/* battery voltage alarm */
#define BATT_CRITICAL_LOW_VOLTAGE		(3000)
#define BATT_CRITICAL_ALARM_STEP		(200)
static int critical_shutdown = 0;
/*
 * critical_alarm_level = {0, 1, 2}, correspond
 * alarm voltage(level) = cirtical_alarm_voltage_mv
 *							+ (BATT_CRITICAL_ALARM_STEP * level)
 */
static int critical_alarm_level = 2;
static int critical_alarm_level_set = 2;
struct wake_lock voltage_alarm_wake_lock;

#ifdef CONFIG_HTC_BATT_ALARM
struct early_suspend early_suspend;
static int screen_state;

/* To disable holding wakelock when AC in for suspend test */
static int ac_suspend_flag;
#endif

/* static int prev_charging_src; */
static int latest_chg_src = CHARGER_BATTERY;

struct htc_battery_info {
	int device_id;

	/* lock to protect the battery info */
	struct mutex info_lock;

	spinlock_t batt_lock;
	int is_open;

	/* threshold voltage to drop battery level aggressively */
	int critical_low_voltage_mv;
	/* voltage alarm threshold voltage */
	int critical_alarm_voltage_mv;
	int overload_vol_thr_mv;
	int overload_curr_thr_ma;

	struct kobject batt_timer_kobj;
	struct kobject batt_cable_kobj;

	struct wake_lock vbus_wake_lock;
	char debug_log[DEBUG_LOG_LENGTH];

	struct battery_info_reply rep;
	struct mpp_config_data *mpp_config;
	struct battery_adc_reply adc_data;
	int adc_vref[ADC_REPLY_ARRAY_SIZE];

	int guage_driver;
	int charger;
	/* gauge/charger interface */
	struct htc_gauge *igauge;
	struct htc_charger *icharger;
	struct htc_battery_cell *bcell;
};
static struct htc_battery_info htc_batt_info;

struct htc_battery_timer {
	struct mutex schedule_lock;
	unsigned long batt_system_jiffies;
	unsigned long batt_suspend_ms;
	unsigned long total_time_ms;	/* since last do batt_work */
	unsigned int batt_alarm_status;
#ifdef CONFIG_HTC_BATT_ALARM
	unsigned int batt_critical_alarm_counter;
#endif
	unsigned int batt_alarm_enabled;
	unsigned int alarm_timer_flag;
	unsigned int time_out;
	struct work_struct batt_work;
	struct alarm batt_check_wakeup_alarm;
	struct timer_list batt_timer;
	struct workqueue_struct *batt_wq;
	struct wake_lock battery_lock;
};
static struct htc_battery_timer htc_batt_timer;

/* cable detect */
struct mutex cable_notifier_lock; /* synchronize notifier func call */
static void cable_status_notifier_func(int online);
static struct t_cable_status_notifier cable_status_notifier = {
	.name = "htc_battery_8960",
	.func = cable_status_notifier_func,
};

static int htc_battery_initial;
static int htc_full_level_flag;
static int htc_battery_set_charging(int ctl);

#ifdef CONFIG_HTC_BATT_ALARM
static int battery_vol_alarm_mode;
static struct battery_vol_alarm alarm_data;
struct mutex batt_set_alarm_lock;
#endif

/* For touch panel, touch panel may loss wireless charger notification when system boot up */
int htc_is_wireless_charger(void)
{
	if (htc_battery_initial)
		return (htc_batt_info.rep.charging_source == CHARGER_WIRELESS) ? 1 : 0;
	else
		return -1;
}

/* matt add */
int htc_batt_schedule_batt_info_update(void)
{
	/*  MATT: use spin_lock? */
	wake_lock(&htc_batt_timer.battery_lock);
	queue_work(htc_batt_timer.batt_wq, &htc_batt_timer.batt_work);
	return 0;
}

/* generic voltage alarm handle */
static void batt_lower_voltage_alarm_handler(int status)
{
	wake_lock(&voltage_alarm_wake_lock);
	if (status) {
		htc_batt_info.igauge->enable_lower_voltage_alarm(0);
		BATT_LOG("voltage_alarm level=%d (%d mV) triggered.",
			critical_alarm_level,
			htc_batt_info.critical_alarm_voltage_mv
				+ BATT_CRITICAL_ALARM_STEP * critical_alarm_level);
		if (critical_alarm_level == 0)
			critical_shutdown = 1;
		critical_alarm_level--;
		htc_batt_schedule_batt_info_update();
	} else {
		pr_info("[BATT] voltage_alarm level=%d (%d mV) raised back.\n",
			critical_alarm_level,
			htc_batt_info.critical_alarm_voltage_mv
				+ BATT_CRITICAL_ALARM_STEP * critical_alarm_level);
	}
	wake_unlock(&voltage_alarm_wake_lock);
}

/* TODO: synchornize this function */
int htc_gauge_event_notify(enum htc_gauge_event event)
{
	pr_info("[BATT] gauge event=%d\n", event);
	switch (event) {
	case HTC_GAUGE_EVENT_READY:
		if (!htc_batt_info.igauge) {
			pr_err("[BATT]err: htc_gauge is not hooked.\n");
			break;
		}
		mutex_lock(&htc_batt_info.info_lock);
		htc_batt_info.igauge->ready = 1;
/* FIXME */
#if 0
		if (htc_batt_info.icharger && htc_batt_info.icharger->ready)
			htc_batt_info.rep.batt_state = 1;
		if (htc_batt_info.rep.batt_state) {
			cable_detect_register_notifier(&cable_status_notifier);
			htc_batt_schedule_batt_info_update();
		}
#endif
		/* register batt alarm */
		if (htc_batt_info.igauge && htc_batt_info.critical_alarm_voltage_mv) {
			if (htc_batt_info.igauge->register_lower_voltage_alarm_notifier)
				htc_batt_info.igauge->register_lower_voltage_alarm_notifier(
									batt_lower_voltage_alarm_handler);
			if (htc_batt_info.igauge->set_lower_voltage_alarm_threshold)
				htc_batt_info.igauge->set_lower_voltage_alarm_threshold(
					htc_batt_info.critical_alarm_voltage_mv
					+ (BATT_CRITICAL_ALARM_STEP * critical_alarm_level));
			if (htc_batt_info.igauge->enable_lower_voltage_alarm)
				htc_batt_info.igauge->enable_lower_voltage_alarm(1);
		}

		mutex_unlock(&htc_batt_info.info_lock);
		break;
	case HTC_GAUGE_EVENT_EOC:
	case HTC_GAUGE_EVENT_TEMP_ZONE_CHANGE:
		htc_batt_schedule_batt_info_update();
		break;
	case HTC_GAUGE_EVENT_BATT_REMOVED:
		if (!(get_kernel_flag() & KERNEL_FLAG_TEST_PWR_SUPPLY)) {
			schedule_delayed_work(&shutdown_work,
					msecs_to_jiffies(BATT_REMOVED_SHUTDOWN_DELAY_MS));
		}
		break;
	default:
		pr_info("[BATT] unsupported gauge event(%d)\n", event);
		break;
	}
	return 0;
}

/* TODO: synchornize this function and gauge_event_notify:
 * define call condition: in interrupt context or not. or separate it:
 * idea1: move cable event to cable_status_handler directly.
 * idea2: move those need mutex protect code to work queue worker.
 * idea3: queuing all event handler here. or queuing all caller function
 *        to prevent it calls from interrupt handler. */
int htc_charger_event_notify(enum htc_charger_event event)
{
	/* MATT TODO: check we need to queue a work here */
	pr_info("[BATT] charger event=%d\n", event);
	switch (event) {
	case HTC_CHARGER_EVENT_VBUS_IN:
		/* latest_chg_src = CHARGER_USB; */
		/* htc_batt_info.rep.charging_source = CHARGER_USB; */
		break;
	case HTC_CHARGER_EVENT_VBUS_OUT:
	case HTC_CHARGER_EVENT_SRC_NONE: /* synchorized call from cable notifier */
		latest_chg_src = CHARGER_BATTERY;
		/* prev_charging_src = htc_batt_info.rep.charging_source;
		htc_batt_info.rep.charging_source = CHARGER_BATTERY; */
		htc_batt_schedule_batt_info_update();
		break;
	case HTC_CHARGER_EVENT_SRC_USB: /* synchorized call from cable notifier */
		latest_chg_src = CHARGER_USB;
		/* prev_charging_src = htc_batt_info.rep.charging_source;
		htc_batt_info.rep.charging_source = CHARGER_USB; */
		htc_batt_schedule_batt_info_update();
		break;
	case HTC_CHARGER_EVENT_SRC_AC: /* synchforized call form cable notifier */
		latest_chg_src = CHARGER_AC;
		/* prev_charging_src = htc_batt_info.rep.charging_source;
		htc_batt_info.rep.charging_source = CHARGER_AC; */
		htc_batt_schedule_batt_info_update();
		break;
	case HTC_CHARGER_EVENT_SRC_WIRELESS: /* synchorized call from cable notifier */
		latest_chg_src = CHARGER_WIRELESS;
		/* prev_charging_src = htc_batt_info.rep.charging_source;
		htc_batt_info.rep.charging_source = CHARGER_USB; */
		htc_batt_schedule_batt_info_update();
		break;
	case HTC_CHARGER_EVENT_OVP:
	case HTC_CHARGER_EVENT_OVP_RESOLVE:
		htc_batt_schedule_batt_info_update();
		break;
	case HTC_CHARGER_EVENT_READY:
		if (!htc_batt_info.icharger) {
			pr_err("[BATT]err: htc_charger is not hooked.\n");
				/* MATT TODO check if we allow batt_state set to 1 here */
			break;
		}
		mutex_lock(&htc_batt_info.info_lock);
		htc_batt_info.icharger->ready = 1;
/* FIXME */
#if 0
		if (htc_batt_info.igauge && htc_batt_info.igauge->ready)
			htc_batt_info.rep.batt_state = 1;
		if (htc_batt_info.rep.batt_state) {
			cable_detect_register_notifier(&cable_status_notifier);
			htc_batt_schedule_batt_info_update();
		}
#endif
		mutex_unlock(&htc_batt_info.info_lock);
		break;
	default:
		pr_info("[BATT] unsupported charger event(%d)\n", event);
		break;
	}
	return 0;
}


/* MATT porting */
#if 0
#ifdef CONFIG_HTC_BATT_ALARM
static int batt_set_voltage_alarm(unsigned long lower_threshold,
			unsigned long upper_threshold)
#else
static int batt_alarm_config(unsigned long lower_threshold,
			unsigned long upper_threshold)
#endif
{
	int rc = 0;

	BATT_LOG("%s(lw = %lu, up = %lu)", __func__,
		lower_threshold, upper_threshold);
	rc = pm8058_batt_alarm_state_set(0, 0);
	if (rc) {
		BATT_ERR("state_set disabled failed, rc=%d", rc);
		goto done;
	}

	rc = pm8058_batt_alarm_threshold_set(lower_threshold, upper_threshold);
	if (rc) {
		BATT_ERR("threshold_set failed, rc=%d!", rc);
		goto done;
	}

#ifdef CONFIG_HTC_BATT_ALARM
	rc = pm8058_batt_alarm_state_set(1, 0);
	if (rc) {
		BATT_ERR("state_set enabled failed, rc=%d", rc);
		goto done;
	}
#endif

done:
	return rc;
}
#ifdef CONFIG_HTC_BATT_ALARM
static int batt_clear_voltage_alarm(void)
{
	int rc = pm8058_batt_alarm_state_set(0, 0);
	BATT_LOG("disable voltage alarm");
	if (rc)
		BATT_ERR("state_set disabled failed, rc=%d", rc);
	return rc;
}

static int batt_set_voltage_alarm_mode(int mode)
{
	int rc = 0;


	BATT_LOG("%s , mode:%d\n", __func__, mode);


	mutex_lock(&batt_set_alarm_lock);
	/*if (battery_vol_alarm_mode != BATT_ALARM_DISABLE_MODE &&
		mode != BATT_ALARM_DISABLE_MODE)
		BATT_ERR("%s:Warning:set mode to %d from non-disable mode(%d)",
			__func__, mode, battery_vol_alarm_mode); */
	switch (mode) {
	case BATT_ALARM_DISABLE_MODE:
		rc = batt_clear_voltage_alarm();
		break;
	case BATT_ALARM_CRITICAL_MODE:
		rc = batt_set_voltage_alarm(BATT_CRITICAL_LOW_VOLTAGE,
			alarm_data.upper_threshold);
		break;
	default:
	case BATT_ALARM_NORMAL_MODE:
		rc = batt_set_voltage_alarm(alarm_data.lower_threshold,
			alarm_data.upper_threshold);
		break;
	}
	if (!rc)
		battery_vol_alarm_mode = mode;
	else {
		battery_vol_alarm_mode = BATT_ALARM_DISABLE_MODE;
		batt_clear_voltage_alarm();
	}
	mutex_unlock(&batt_set_alarm_lock);
	return rc;
}
#endif

static int battery_alarm_notifier_func(struct notifier_block *nfb,
					unsigned long value, void *data);
static struct notifier_block battery_alarm_notifier = {
	.notifier_call = battery_alarm_notifier_func,
};

static int battery_alarm_notifier_func(struct notifier_block *nfb,
					unsigned long status, void *data)
{

#ifdef CONFIG_HTC_BATT_ALARM
	BATT_LOG("%s \n", __func__);

	if (battery_vol_alarm_mode == BATT_ALARM_CRITICAL_MODE) {
		BATT_LOG("%s(): CRITICAL_MODE counter = %d", __func__,
			htc_batt_timer.batt_critical_alarm_counter + 1);
		if (++htc_batt_timer.batt_critical_alarm_counter >= 3) {
			BATT_LOG("%s: 3V voltage alarm is triggered.", __func__);
			htc_batt_info.rep.level = 1;
			/* htc_battery_core_update(BATTERY_SUPPLY); */
			htc_battery_core_update_changed();
		}
		batt_set_voltage_alarm_mode(BATT_ALARM_CRITICAL_MODE);
	} else if (battery_vol_alarm_mode == BATT_ALARM_NORMAL_MODE) {
		htc_batt_timer.batt_alarm_status++;
		BATT_LOG("%s: NORMAL_MODE batt alarm status = %u", __func__,
			htc_batt_timer.batt_alarm_status);
	} else { /* BATT_ALARM_DISABLE_MODE */
		BATT_ERR("%s:Warning: batt alarm triggerred in disable mode ", __func__);
	}
#else
	htc_batt_timer.batt_alarm_status++;
	BATT_LOG("%s: batt alarm status %u", __func__, htc_batt_timer.batt_alarm_status);
#endif
	return 0;
}
#endif

/* MATT porting */
#if 0
static void update_wake_lock(int status)
{
#ifdef CONFIG_HTC_BATT_ALARM
	/* hold an wakelock when charger connected until disconnected
		except for AC under test mode(ac_suspend_flag=1). */
	if (status != CHARGER_BATTERY && !ac_suspend_flag)
		wake_lock(&htc_batt_info.vbus_wake_lock);
	else if (status == CHARGER_USB && ac_suspend_flag)
		/* For suspend test, not hold wake lock when AC in */
		wake_lock(&htc_batt_info.vbus_wake_lock);
	else
		/* give userspace some time to see the uevent and update
		   LED state or whatnot...*/
		   wake_lock_timeout(&htc_batt_info.vbus_wake_lock, HZ * 5);
#else
	if (status == CHARGER_USB)
		wake_lock(&htc_batt_info.vbus_wake_lock);
	else
		/* give userspace some time to see the uevent and update
		   LED state or whatnot...*/
		wake_lock_timeout(&htc_batt_info.vbus_wake_lock, HZ * 5);
#endif
}
#endif

static void cable_status_notifier_func(enum usb_connect_type online)
{
	static int first_update = 1;
	mutex_lock(&cable_notifier_lock);
	/* MATT: */
	htc_batt_info.rep.batt_state = 1;

	BATT_LOG("%s(%u)", __func__, online);
	//if (online == htc_batt_info.rep.charging_source && !first_update) {
	if (online == latest_chg_src && !first_update) {
		BATT_LOG("%s: charger type (%u) same return.",
			__func__, online);
		mutex_unlock(&cable_notifier_lock);
		return;
	}
	first_update = 0;

	switch (online) {
	case CONNECT_TYPE_USB:
		BATT_LOG("USB charger");
		htc_charger_event_notify(HTC_CHARGER_EVENT_SRC_USB);
		/* radio_set_cable_status(CHARGER_USB); */
		break;
	case CONNECT_TYPE_AC:
		BATT_LOG("5V AC charger");
		htc_charger_event_notify(HTC_CHARGER_EVENT_SRC_AC);
		/* radio_set_cable_status(CHARGER_AC); */
		break;
	case CONNECT_TYPE_WIRELESS:
		BATT_LOG("wireless charger (not supported)");
		htc_charger_event_notify(HTC_CHARGER_EVENT_SRC_WIRELESS);
		/* radio_set_cable_status(CHARGER_WIRELESS); */
		break;
	case CONNECT_TYPE_UNKNOWN:
		BATT_ERR("unknown cable");
		htc_charger_event_notify(HTC_CHARGER_EVENT_SRC_USB);
		break;
	case CONNECT_TYPE_INTERNAL:
		BATT_LOG("delivers power to VBUS from battery (not supported)");
		/* htc_battery_set_charging(POWER_SUPPLY_ENABLE_INTERNAL);
		mutex_unlock(&htc_batt_info.info_lock);
		return; */
		break;
	case CONNECT_TYPE_NONE:
		BATT_LOG("No cable exists");
		htc_charger_event_notify(HTC_CHARGER_EVENT_SRC_NONE);
		/* radio_set_cable_status(CHARGER_BATTERY); */
		break;
	default:
		BATT_LOG("unsupported connect_type=%d", online);
		htc_charger_event_notify(HTC_CHARGER_EVENT_SRC_NONE);
		/* radio_set_cable_status(CHARGER_BATTERY); */
		break;
	}
#if 0 /* MATT check porting */
	htc_batt_timer.alarm_timer_flag =
			(unsigned int)htc_batt_info.rep.charging_source;

	update_wake_lock(htc_batt_info.rep.charging_source);
#endif
	mutex_unlock(&cable_notifier_lock);
}

static int htc_battery_set_charging(int ctl)
{
	int rc = 0;
/* MATT porting
	if (htc_batt_info.charger == SWITCH_CHARGER_TPS65200)
		rc = tps_set_charger_ctrl(ctl);
*/
	return rc;
}

static void __context_event_handler(enum batt_context_event event)
{
	pr_info("[BATT] handle context event(%d)\n", event);

	switch (event) {
	case EVENT_TALK_START:
		if (chg_limit_active_mask & HTC_BATT_CHG_LIMIT_BIT_TALK) {
			chg_limit_reason |= HTC_BATT_CHG_LIMIT_BIT_TALK;
			if (htc_batt_info.icharger &&
					htc_batt_info.icharger->set_limit_charge_enable)
				htc_batt_info.icharger->set_limit_charge_enable(true);
		}
		suspend_highfreq_check_reason |= SUSPEND_HIGHFREQ_CHECK_BIT_TALK;
		break;
	case EVENT_TALK_STOP:
		if (chg_limit_active_mask & HTC_BATT_CHG_LIMIT_BIT_TALK) {
			chg_limit_reason &= ~HTC_BATT_CHG_LIMIT_BIT_TALK;
			if (!chg_limit_reason &&
					htc_batt_info.icharger &&
					htc_batt_info.icharger->set_limit_charge_enable)
				htc_batt_info.icharger->set_limit_charge_enable(false);
		}
		suspend_highfreq_check_reason &= ~SUSPEND_HIGHFREQ_CHECK_BIT_TALK;
		break;
	case EVENT_NAVIGATION_START:
		if (chg_limit_active_mask & HTC_BATT_CHG_LIMIT_BIT_NAVI) {
			chg_limit_reason |= HTC_BATT_CHG_LIMIT_BIT_NAVI;
			if (htc_batt_info.icharger &&
					htc_batt_info.icharger->set_limit_charge_enable)
				htc_batt_info.icharger->set_limit_charge_enable(true);
		}
		break;
	case EVENT_NAVIGATION_STOP:
		if (chg_limit_active_mask & HTC_BATT_CHG_LIMIT_BIT_NAVI) {
			chg_limit_reason &= ~HTC_BATT_CHG_LIMIT_BIT_NAVI;
			if (!chg_limit_reason &&
					htc_batt_info.icharger &&
					htc_batt_info.icharger->set_limit_charge_enable)
				htc_batt_info.icharger->set_limit_charge_enable(false);
		}
		break;
	case EVENT_NETWORK_SEARCH_START:
		suspend_highfreq_check_reason |= SUSPEND_HIGHFREQ_CHECK_BIT_SEARCH;
		break;
	case EVENT_NETWORK_SEARCH_STOP:
		suspend_highfreq_check_reason &= ~SUSPEND_HIGHFREQ_CHECK_BIT_SEARCH;
		break;
	default:
		pr_warn("unsupported context event (%d)\n", event);
		return;
	}

	htc_batt_schedule_batt_info_update();
}

struct mutex context_event_handler_lock; /* synchroniz context_event_handler */
static int htc_batt_context_event_handler(enum batt_context_event event)
{
	int prev_context_state;

	mutex_lock(&context_event_handler_lock);
	prev_context_state = context_state;
	/* STEP.1: check if state not changed then return */
	switch (event) {
	case EVENT_TALK_START:
		if (context_state & CONTEXT_STATE_BIT_TALK)
			goto exit;
		context_state |= CONTEXT_STATE_BIT_TALK;
		break;
	case EVENT_TALK_STOP:
		if (!(context_state & CONTEXT_STATE_BIT_TALK))
			goto exit;
		context_state &= ~CONTEXT_STATE_BIT_TALK;
		break;
	case EVENT_NETWORK_SEARCH_START:
		if (context_state & CONTEXT_STATE_BIT_SEARCH)
			goto exit;
		context_state |= CONTEXT_STATE_BIT_SEARCH;
		break;
	case EVENT_NETWORK_SEARCH_STOP:
		if (!(context_state & CONTEXT_STATE_BIT_SEARCH))
			goto exit;
		context_state &= ~CONTEXT_STATE_BIT_SEARCH;
		break;
	case EVENT_NAVIGATION_START:
		if (context_state & CONTEXT_STATE_BIT_NAVIGATION)
			goto exit;
		context_state |= CONTEXT_STATE_BIT_NAVIGATION;
		break;
	case EVENT_NAVIGATION_STOP:
		if (!(context_state & CONTEXT_STATE_BIT_NAVIGATION))
			goto exit;
		context_state &= ~CONTEXT_STATE_BIT_NAVIGATION;
		break;
	default:
		pr_warn("unsupported context event (%d)\n", event);
		goto exit;
	}
	BATT_LOG("context_state: 0x%x -> 0x%x", prev_context_state, context_state);

	/* STEP.2: handle incoming event */
	__context_event_handler(event);

exit:
	mutex_unlock(&context_event_handler_lock);
	return 0;
}

static int htc_batt_charger_control(enum charger_control_flag control)
{
	int ret = 0;

	BATT_LOG("%s: user switch charger to mode: %u", __func__, control);
	if (control == STOP_CHARGER)
		chg_dis_user_timer = 1;
	else if (control == ENABLE_CHARGER)
		chg_dis_user_timer = 0;
	else if (control == DISABLE_PWRSRC)
			pwrsrc_dis_reason |= HTC_BATT_PWRSRC_DIS_BIT_API;
	else if (control == ENABLE_PWRSRC)
			pwrsrc_dis_reason &= ~HTC_BATT_PWRSRC_DIS_BIT_API;
	else if (control == DISABLE_LIMIT_CHARGER) {
		/* FIXME: remove this control handle */
		htc_batt_context_event_handler(EVENT_TALK_STOP);
		return 0;
	} else if (control == ENABLE_LIMIT_CHARGER) {
		/* FIXME: remove this control handle */
		htc_batt_context_event_handler(EVENT_TALK_START);
		return 0;
	} else {
		BATT_LOG("%s: unsupported charger_contorl(%d)", __func__, control);
		return -1;
	}
	htc_batt_schedule_batt_info_update();
	return ret;
}

static void htc_batt_set_full_level(int percent)
{
	if (percent < 0)
		htc_batt_info.rep.full_level = 0;
	else if (100 < percent)
		htc_batt_info.rep.full_level = 100;
	else
		htc_batt_info.rep.full_level = percent;

	BATT_LOG(" set full_level constraint as %d.", percent);

	return;
}

static ssize_t htc_battery_show_batt_attr(struct device_attribute *attr,
					char *buf)
{
	int len = 0;

	/* collect htc_battery vars */
	len += scnprintf(buf + len, PAGE_SIZE - len,
			"charging_source: %d;\n"
			"charging_enabled: %d;\n"
			"overload: %d;\n"
			"Percentage(%%): %d;\n",
			htc_batt_info.rep.charging_source,
			htc_batt_info.rep.charging_enabled,
			htc_batt_info.rep.overload,
			htc_batt_info.rep.level
			);

	/* collect gague vars */
	if (htc_batt_info.igauge) {
#if 0
		if (htc_batt_info.igauge->name)
			len += scnprintf(buf + len, PAGE_SIZE - len,
				"gauge: %s;\n", htc_batt_info.igauge->name);
#endif
		if (htc_batt_info.igauge->get_attr_text)
			len += htc_batt_info.igauge->get_attr_text(buf + len,
						PAGE_SIZE - len);
	}

	/* collect charger vars */
	if (htc_batt_info.icharger) {
#if 0
		if (htc_batt_info.icharger->name)
			len += scnprintf(buf + len, PAGE_SIZE - len,
				"charger: %s;\n", htc_batt_info.icharger->name);
#endif
		if (htc_batt_info.icharger->get_attr_text)
			len += htc_batt_info.icharger->get_attr_text(buf + len,
						PAGE_SIZE - len);
	}
	return len;
}

static ssize_t htc_battery_show_cc_attr(struct device_attribute *attr,
					char *buf)
{
	int len = 0, cc_uah = 0;

	/* collect gague vars */
	if (htc_batt_info.igauge) {
		if (htc_batt_info.igauge->get_battery_cc) {
			htc_batt_info.igauge->get_battery_cc(&cc_uah);
			len += scnprintf(buf + len, PAGE_SIZE - len,
				"cc:%d\n", cc_uah);
		}
	}

	return len;
}

static int htc_batt_open(struct inode *inode, struct file *filp)
{
	int ret = 0;

	BATT_LOG("%s: open misc device driver.", __func__);
	spin_lock(&htc_batt_info.batt_lock);

	if (!htc_batt_info.is_open)
		htc_batt_info.is_open = 1;
	else
		ret = -EBUSY;

	spin_unlock(&htc_batt_info.batt_lock);

#ifdef CONFIG_ARCH_MSM8X60_LTE
	/*Always disable cpu1 for off-mode charging.
	 * For 8x60_LTE projects only */
	if (board_mfg_mode() == 5)
		cpu_down(1);

#endif

	return ret;
}

static int htc_batt_release(struct inode *inode, struct file *filp)
{
	BATT_LOG("%s: release misc device driver.", __func__);
	spin_lock(&htc_batt_info.batt_lock);
	htc_batt_info.is_open = 0;
	spin_unlock(&htc_batt_info.batt_lock);

	return 0;
}

static int htc_batt_get_battery_info(struct battery_info_reply *htc_batt_update)
{
	htc_batt_update->batt_vol = htc_batt_info.rep.batt_vol;
	htc_batt_update->batt_id = htc_batt_info.rep.batt_id;
	htc_batt_update->batt_temp = htc_batt_info.rep.batt_temp;
/* MATT porting */
	htc_batt_update->batt_current = htc_batt_info.rep.batt_current;
#if 0
	/* report the net current injection into battery no
	 * matter charging is enable or not (may negative) */
	htc_batt_update->batt_current = htc_batt_info.rep.batt_current -
			htc_batt_info.rep.batt_discharg_current;
	htc_batt_update->batt_discharg_current =
				htc_batt_info.rep.batt_discharg_current;
#endif
	htc_batt_update->level = htc_batt_info.rep.level;
	htc_batt_update->charging_source =
				htc_batt_info.rep.charging_source;
	htc_batt_update->charging_enabled =
				htc_batt_info.rep.charging_enabled;
	htc_batt_update->full_bat = htc_batt_info.rep.full_bat;
	htc_batt_update->full_level = htc_batt_info.rep.full_level;
	htc_batt_update->over_vchg = htc_batt_info.rep.over_vchg;
	htc_batt_update->temp_fault = htc_batt_info.rep.temp_fault;
	htc_batt_update->batt_state = htc_batt_info.rep.batt_state;
	htc_batt_update->overload = htc_batt_info.rep.overload;
	return 0;
}

static void batt_set_check_timer(u32 seconds)
{
	pr_debug("[BATT] %s(%u sec)\n", __func__, seconds);
	mod_timer(&htc_batt_timer.batt_timer,
			jiffies + msecs_to_jiffies(seconds * 1000));
}


u32 htc_batt_getmidvalue(int32_t *value)
{
	int i, j, n, len;
	len = ADC_REPLY_ARRAY_SIZE;
	for (i = 0; i < len - 1; i++) {
		for (j = i + 1; j < len; j++) {
			if (value[i] > value[j]) {
				n = value[i];
				value[i] = value[j];
				value[j] = n;
			}
		}
	}

	return value[len / 2];
}

/* MATT porting */
#if 0
static int32_t htc_batt_get_battery_adc(void)
{
	int ret = 0;
	u32 vref = 0;
	u32 battid_adc = 0;
	struct battery_adc_reply adc;

	/* Read battery voltage adc data. */
	ret = pm8058_htc_config_mpp_and_adc_read(
			adc.adc_voltage,
			ADC_REPLY_ARRAY_SIZE,
			CHANNEL_ADC_BATT_AMON,
			htc_batt_info.mpp_config->vol[XOADC_MPP],
			htc_batt_info.mpp_config->vol[PM_MPP_AIN_AMUX]);
	if (ret)
		goto get_adc_failed;
	/* Read battery current adc data. */
	ret = pm8058_htc_config_mpp_and_adc_read(
			adc.adc_current,
			ADC_REPLY_ARRAY_SIZE,
			CHANNEL_ADC_BATT_AMON,
			htc_batt_info.mpp_config->curr[XOADC_MPP],
			htc_batt_info.mpp_config->curr[PM_MPP_AIN_AMUX]);
	if (ret)
		goto get_adc_failed;
	/* Read battery temperature adc data. */
	ret = pm8058_htc_config_mpp_and_adc_read(
			adc.adc_temperature,
			ADC_REPLY_ARRAY_SIZE,
			CHANNEL_ADC_BATT_AMON,
			htc_batt_info.mpp_config->temp[XOADC_MPP],
			htc_batt_info.mpp_config->temp[PM_MPP_AIN_AMUX]);
	if (ret)
		goto get_adc_failed;
	/* Read battery id adc data. */
	ret = pm8058_htc_config_mpp_and_adc_read(
			adc.adc_battid,
			ADC_REPLY_ARRAY_SIZE,
			CHANNEL_ADC_BATT_AMON,
			htc_batt_info.mpp_config->battid[XOADC_MPP],
			htc_batt_info.mpp_config->battid[PM_MPP_AIN_AMUX]);

	vref = htc_batt_getmidvalue(adc.adc_voltage);
	battid_adc = htc_batt_getmidvalue(adc.adc_battid);

	BATT_LOG("%s , vref:%d, battid_adc:%d, battid:%d\n", __func__,  vref, battid_adc, battid_adc * 1000 / vref);

	if (ret)
		goto get_adc_failed;

	memcpy(&htc_batt_info.adc_data, &adc,
		sizeof(struct battery_adc_reply));

get_adc_failed:
	return ret;
}
#endif

static void batt_regular_timer_handler(unsigned long data)
{
	pr_debug("[BATT] %s()\n", __func__);
	htc_batt_schedule_batt_info_update();
	/*
	wake_lock(&htc_batt_timer.battery_lock);
	queue_work(htc_batt_timer.batt_wq, &htc_batt_timer.batt_work);
	*/
}

static void batt_check_alarm_handler(struct alarm *alarm)
{
	BATT_LOG("alarm handler, but do nothing.");
	return;
}

static int bounding_fullly_charged_level(int upperbd, int current_level)
{
	static int pingpong = 1;
	int lowerbd;
	int b_is_charge_off_by_bounding = 0;

	lowerbd = upperbd - 5; /* 5% range */

	if (lowerbd < 0)
		lowerbd = 0;

	if (pingpong == 1 && upperbd <= current_level) {
		pr_info("MFG: lowerbd=%d, upperbd=%d, current=%d,"
				" pingpong:1->0 turn off\n", lowerbd, upperbd, current_level);
		b_is_charge_off_by_bounding = 1;
		pingpong = 0;
	} else if (pingpong == 0 && lowerbd < current_level) {
		pr_info("MFG: lowerbd=%d, upperbd=%d, current=%d,"
				" toward 0, turn off\n", lowerbd, upperbd, current_level);
		b_is_charge_off_by_bounding = 1;
	} else if (pingpong == 0 && current_level <= lowerbd) {
		pr_info("MFG: lowerbd=%d, upperbd=%d, current=%d,"
				" pingpong:0->1 turn on\n", lowerbd, upperbd, current_level);
		pingpong = 1;
	} else {
		pr_info("MFG: lowerbd=%d, upperbd=%d, current=%d,"
				" toward %d, turn on\n", lowerbd, upperbd, current_level, pingpong);
	}
	return b_is_charge_off_by_bounding;
}

static inline int is_bounding_fully_charged_level(void)
{
	if (0 < htc_batt_info.rep.full_level &&
			htc_batt_info.rep.full_level < 100)
		return bounding_fullly_charged_level(
				htc_batt_info.rep.full_level, htc_batt_info.rep.level);
	return 0;
}

static void batt_update_info_from_charger(void)
{
	if (!htc_batt_info.icharger) {
		BATT_LOG("warn: charger interface is not hooked.");
		return;
	}

	if (htc_batt_info.icharger->is_batt_temp_fault_disable_chg)
		htc_batt_info.icharger->is_batt_temp_fault_disable_chg(
				&charger_dis_temp_fault);
}

static void batt_update_info_from_gauge(void)
{
	if (!htc_batt_info.igauge) {
		BATT_LOG("warn: gauge interface is not hooked.");
		return;
	}

	/* STEP 1: read basic battery info */
	/* get voltage */
	if (htc_batt_info.igauge->get_battery_voltage)
		htc_batt_info.igauge->get_battery_voltage(
				&htc_batt_info.rep.batt_vol);
	/* get current */
	if (htc_batt_info.igauge->get_battery_current)
		htc_batt_info.igauge->get_battery_current(
				&htc_batt_info.rep.batt_current);
	/* get temperature */
	if (htc_batt_info.igauge->get_battery_temperature)
		htc_batt_info.igauge->get_battery_temperature(
				&htc_batt_info.rep.batt_temp);
	/* get temperature fault */
	if (htc_batt_info.igauge->is_battery_temp_fault)
		htc_batt_info.igauge->is_battery_temp_fault(
				&htc_batt_info.rep.temp_fault);
	/* get batt_id */
	if (htc_batt_info.igauge->get_battery_id)
		htc_batt_info.igauge->get_battery_id(
				&htc_batt_info.rep.batt_id);

	/* battery id check and cell assginment
	htc_batt_info.bcell = htc_battery_cell_find(batt_id_raw);
	htc_battery_cell_set_cur_cell(htc_batt_info.bcell);
	htc_batt_info.rep.batt_id = htc_batt_info.bcell->id;
	*/
	/* MATT: review the definition */
	if (htc_battery_cell_get_cur_cell())
		htc_batt_info.rep.full_bat = htc_battery_cell_get_cur_cell()->capacity;

	htc_batt_info.igauge->get_battery_soc(
		&htc_batt_info.rep.level);
	/* get charger ovp state */
	if (htc_batt_info.icharger->is_ovp)
		htc_batt_info.icharger->is_ovp(&htc_batt_info.rep.over_vchg);
}

inline static int is_voltage_critical_low(int voltage_mv)
{
	return (voltage_mv < htc_batt_info.critical_low_voltage_mv) ? 1 : 0;
}

static void batt_check_overload(void)
{
	static unsigned int overload_count;

	pr_debug("[BATT] Chk overload by CS=%d V=%d I=%d count=%d overload=%d\n",
			htc_batt_info.rep.charging_source, htc_batt_info.rep.batt_vol,
			htc_batt_info.rep.batt_current, overload_count, htc_batt_info.rep.overload);
	if ((htc_batt_info.rep.charging_source > 0) &&
			(htc_batt_info.rep.batt_vol < htc_batt_info.overload_vol_thr_mv) &&
			((htc_batt_info.rep.batt_current / 1000) >
				htc_batt_info.overload_curr_thr_ma)) {
		if (overload_count++ < 3) {
			htc_batt_info.rep.overload = 0;
		} else
			htc_batt_info.rep.overload = 1;
	} else { /* Cable is removed */
			overload_count = 0;
			htc_batt_info.rep.overload = 0;
	}
}

#define ONE_PERCENT_LIMIT_PERIOD_MS		(1000 * (60 + 10))
#define FIVE_PERCENT_LIMIT_PERIOD_MS	(1000 * (300 + 10))
static void batt_level_adjust(unsigned long time_since_last_update_ms)
{
	static int first = 1;
	static int critical_low_enter = 0;
	int prev_level, drop_level;
	int is_full = 0;
	const struct battery_info_reply *prev_batt_info_rep =
						htc_battery_core_get_batt_info_rep();

	/* FIXME
	if (prev_batt_info_rep->batt_state) */
	if (!first)
		prev_level = prev_batt_info_rep->level;
	else
		prev_level = htc_batt_info.rep.level;
	drop_level = prev_level - htc_batt_info.rep.level;

	if (!prev_batt_info_rep->charging_enabled) {
		/* battery discharging - level smoothen algorithm:
		 * IF VBATT < CRITICAL_LOW_VOLTAGE THEN
		 *		drop level by 6%
		 * ELSE
		 * - level cannot drop over 2% in 1 minute (here use 60 + 10 sec).
		 * - level cannot drop over 5% in 5 minute (here use 300 + 10 sec).
		 * - level cannot increase while discharging.
		 */
		if (is_voltage_critical_low(htc_batt_info.rep.batt_vol)) {
			critical_low_enter = 1;
			/* batt voltage is under critical low condition */
			pr_info("[BATT] battery level force decreses 6%% from %d%%"
					" (soc=%d)on critical low (%d mV)\n", prev_level,
						htc_batt_info.rep.level,
						htc_batt_info.critical_low_voltage_mv);
			htc_batt_info.rep.level =
					(prev_level - 6 > 0) ? (prev_level - 6) :	0;
		} else {
			if (time_since_last_update_ms <= ONE_PERCENT_LIMIT_PERIOD_MS) {
				if (2 < drop_level) {
					pr_info("[BATT] soc drop = %d%% > 2%% in %lu ms.\n",
								drop_level,time_since_last_update_ms);
					htc_batt_info.rep.level = prev_level - 2;
				} else if (drop_level < 0) {
					/* soc increased in discharging state:
					 * do not allow level increase. */
					if (critical_low_enter) {
						pr_warn("[BATT] level increase because of"
								" exit critical_low!\n");
					}
					htc_batt_info.rep.level = prev_level;
				} else {
					/* soc drop <= 2 or no increase. */
				}
			} else if ((chg_limit_reason & HTC_BATT_CHG_LIMIT_BIT_TALK) &&
				(time_since_last_update_ms <= FIVE_PERCENT_LIMIT_PERIOD_MS)) {
				if (5 < drop_level) {
					pr_info("[BATT] soc drop = %d%% > 5%% in %lu ms.\n",
								drop_level,time_since_last_update_ms);
					htc_batt_info.rep.level = prev_level - 5;
				} else if (drop_level < 0) {
					/* soc increased in discharging state:
					 * do not allow level increase. */
					if (critical_low_enter) {
						pr_warn("[BATT] level increase because of"
								" exit critical_low!\n");
					}
					htc_batt_info.rep.level = prev_level;
				} else {
					/* soc drop <= 5 or no increase. */
				}
			} else {
				if (3 < drop_level) {
					pr_info("[BATT] soc drop = %d%% > 3%% in %lu ms.\n",
								drop_level,time_since_last_update_ms);
					htc_batt_info.rep.level = prev_level - 3;
				} else if (drop_level < 0) {
					/* soc increased in discharging state:
					 * do not allow level increase. */
					if (critical_low_enter) {
						pr_warn("[BATT] level increase because of"
								" exit critical_low!\n");
					}
					htc_batt_info.rep.level = prev_level;
				} else {
					/* soc drop <= 3 or no increase. */
				}
			}

			if (critical_low_enter) {
				critical_low_enter = 0;
				pr_warn("[BATT] exit critical_low without charge!\n");
			}
		}
	} else {
		/* battery charging - level smoothen algorithm:
		 * IF batt is not full THEN
		 *		- restrict level less then 100
		 * ELSE
		 *		- set level = 100
		 */
		if (htc_batt_info.igauge->is_battery_full) {
			htc_batt_info.igauge->is_battery_full(&is_full);
			if (!is_full) {
				if (99 < htc_batt_info.rep.level)
					htc_batt_info.rep.level = 99; /* restrict at 99% */
			} else
				htc_batt_info.rep.level = 100; /* update to 100% */
		}
		critical_low_enter = 0;
	}
	first = 0;
}

static void batt_worker(struct work_struct *work)
{
	static int first = 1;
	static int prev_pwrsrc_enabled = 1;
	static int prev_charging_enabled = 0;
	int charging_enabled = prev_charging_enabled;
	int pwrsrc_enabled = prev_pwrsrc_enabled;
	int prev_chg_src;
	unsigned long time_since_last_update_ms;
	unsigned long cur_jiffies;

	/* STEP 1: print out and reset total_time since last update */
	cur_jiffies = jiffies;
	time_since_last_update_ms = htc_batt_timer.total_time_ms +
		((cur_jiffies - htc_batt_timer.batt_system_jiffies) * MSEC_PER_SEC / HZ);
	BATT_LOG("%s: total_time since last batt update = %lu ms.",
				__func__, time_since_last_update_ms);
	htc_batt_timer.total_time_ms = 0; /* reset total time */
	htc_batt_timer.batt_system_jiffies = cur_jiffies;

	/* STEP 2: setup next batt uptate timer (can put in the last step)*/
	/* MATT move del timer here. TODO: move into set_check timer */
	del_timer_sync(&htc_batt_timer.batt_timer);
	batt_set_check_timer(htc_batt_timer.time_out);


	htc_batt_timer.batt_alarm_status = 0;
#ifdef CONFIG_HTC_BATT_ALARM
	htc_batt_timer.batt_critical_alarm_counter = 0;
#endif

	/* STEP 3: update charging_source */
	prev_chg_src = htc_batt_info.rep.charging_source;
	htc_batt_info.rep.charging_source = latest_chg_src;

	/* STEP 4: fresh battery information from gauge/charger */
	batt_update_info_from_gauge();
	batt_update_info_from_charger();

	/* STEP: battery level smoothen adjustment */
	batt_level_adjust(time_since_last_update_ms);

	/* STEP: force level=0 to trigger userspace shutdown */
	if (critical_shutdown) {
		BATT_LOG("critical shutdown (set level=0 to force shutdown)");
		htc_batt_info.rep.level = 0;
	}

	/* STEP: Check if overloading is happeneed during charging */
	batt_check_overload();

	pr_debug("[BATT] context_state=0x%x, suspend_highfreq_check_reason=0x%x\n",
			context_state, suspend_highfreq_check_reason);
	/* STEP 5: set the charger contorl depends on current status
	   batt id, batt temp, batt eoc, full_level
	   if charging source exist, determine charging_enable */
	if (htc_batt_info.rep.charging_source > 0) {
		/*  STEP 4.1. check and update chg_dis_reason */
		if (htc_batt_info.rep.batt_id == HTC_BATTERY_CELL_ID_UNKNOWN)
			chg_dis_reason |= HTC_BATT_CHG_DIS_BIT_ID; /* for disable charger */
		else
			chg_dis_reason &= ~HTC_BATT_CHG_DIS_BIT_ID;

		if (charger_dis_temp_fault)
			chg_dis_reason |= HTC_BATT_CHG_DIS_BIT_TMP;
		else
			chg_dis_reason &= ~HTC_BATT_CHG_DIS_BIT_TMP;

		if (chg_dis_user_timer)
			chg_dis_reason |= HTC_BATT_CHG_DIS_BIT_USR_TMR;
		else
			chg_dis_reason &= ~HTC_BATT_CHG_DIS_BIT_USR_TMR;

		if (is_bounding_fully_charged_level()) {
			chg_dis_reason |= HTC_BATT_CHG_DIS_BIT_MFG;
			pwrsrc_dis_reason |= HTC_BATT_PWRSRC_DIS_BIT_MFG;
		} else {
			chg_dis_reason &= ~HTC_BATT_CHG_DIS_BIT_MFG;
			pwrsrc_dis_reason &= ~HTC_BATT_PWRSRC_DIS_BIT_MFG;
		}

		if (htc_batt_info.rep.over_vchg)
			chg_dis_reason |= HTC_BATT_CHG_DIS_BIT_OVP;
		else
			chg_dis_reason &= ~HTC_BATT_CHG_DIS_BIT_OVP;

		/* STEP 5.2.1 determin pwrsrc_eanbled for charger control */
		if (pwrsrc_dis_reason)
			pwrsrc_enabled = 0;
		else
			pwrsrc_enabled = 1;

		/* STEP 5.2.2 determin charging_eanbled for charger control */
		if (chg_dis_reason & chg_dis_control_mask)
			charging_enabled = HTC_PWR_SOURCE_TYPE_BATT;
		else
			charging_enabled = htc_batt_info.rep.charging_source;

		/* STEP 5.2.3 determin charging_eanbled for userspace */
		if (chg_dis_reason & chg_dis_active_mask)
			htc_batt_info.rep.charging_enabled = HTC_PWR_SOURCE_TYPE_BATT;
		else
			htc_batt_info.rep.charging_enabled =
										htc_batt_info.rep.charging_source;

		/* STEP 5.3. control charger if state changed */
		pr_info("[BATT] prev_chg_src=%d, prev_chg_en=%d,"
				" chg_dis_reason/control/active=0x%x/0x%x/0x%x,"
				" chg_limit_reason=0x%x,"
				" pwrsrc_dis_reason=0x%x, prev_pwrsrc_enabled=%d,"
				" context_state=0x%x\n",
					prev_chg_src, prev_charging_enabled,
					chg_dis_reason,
					chg_dis_reason & chg_dis_control_mask,
					chg_dis_reason & chg_dis_active_mask,
					chg_limit_reason,
					pwrsrc_dis_reason, prev_pwrsrc_enabled,
					context_state);
		if (charging_enabled != prev_charging_enabled ||
				prev_chg_src != htc_batt_info.rep.charging_source ||
				first ||
				pwrsrc_enabled != prev_pwrsrc_enabled) {
			/* re-config charger when state changes */
			if (prev_chg_src != htc_batt_info.rep.charging_source ||
					first) {
				BATT_LOG("set_pwrsrc_and_charger_enable(%d, %d, %d)",
							htc_batt_info.rep.charging_source,
							charging_enabled,
							pwrsrc_enabled);
				if (htc_batt_info.icharger &&
						htc_batt_info.icharger->set_pwrsrc_and_charger_enable)
					htc_batt_info.icharger->set_pwrsrc_and_charger_enable(
								htc_batt_info.rep.charging_source,
								charging_enabled,
								pwrsrc_enabled);
			} else {
				if (pwrsrc_enabled != prev_pwrsrc_enabled) {
					BATT_LOG("set_pwrsrc_enable(%d)", pwrsrc_enabled);
					if (htc_batt_info.icharger &&
						htc_batt_info.icharger->set_pwrsrc_enable)
						htc_batt_info.icharger->set_pwrsrc_enable(
													pwrsrc_enabled);
				}
				if (charging_enabled != prev_charging_enabled) {
					BATT_LOG("set_charger_enable(%d)", charging_enabled);
					if (htc_batt_info.icharger &&
						htc_batt_info.icharger->set_charger_enable)
						htc_batt_info.icharger->set_charger_enable(
													charging_enabled);
				}
			}
		}
	} else {
		/* TODO: check if we need to enable batfet while unplugged */
		if (prev_chg_src != htc_batt_info.rep.charging_source) {
			chg_dis_reason = 0; /* reset on charger out */
			charging_enabled = 1; /* enable batfet */
			pwrsrc_enabled = 1; /* enable power source */
			BATT_LOG("set_pwrsrc_and_charger_enable"
							"(HTC_PWR_SOURCE_TYPE_BATT, 1, 1)");
			if (htc_batt_info.icharger &&
						htc_batt_info.icharger->set_pwrsrc_and_charger_enable)
				htc_batt_info.icharger->set_pwrsrc_and_charger_enable(
								HTC_PWR_SOURCE_TYPE_BATT,
								charging_enabled, pwrsrc_enabled);
			/* update to userspace charging_enabled state */
			htc_batt_info.rep.charging_enabled =
								htc_batt_info.rep.charging_source;
		}
	}

	/* MATT:TODO STEP: get back charger status */
	if (htc_batt_info.icharger) {
		/*
		 pm8921 FSM is not changed immediately
		htc_batt_info.icharger->get_charging_enabled(
				&htc_batt_info.rep.charging_enabled);
		*/
		htc_batt_info.icharger->dump_all();
	}

	/* use notify to set batt_state=1
	htc_batt_info.rep.batt_state = 1; */

	/* STEP: update change to userspace via batt_core */
	htc_battery_core_update_changed();

	/* STEP: check and set voltage alarm */
	if (0 <= critical_alarm_level &&
					critical_alarm_level < critical_alarm_level_set) {
		critical_alarm_level_set = critical_alarm_level;
		pr_info("[BATT] set voltage alarm level=%d\n", critical_alarm_level);
		htc_batt_info.igauge->set_lower_voltage_alarm_threshold(
					htc_batt_info.critical_alarm_voltage_mv
					+ (BATT_CRITICAL_ALARM_STEP * critical_alarm_level));
		htc_batt_info.igauge->enable_lower_voltage_alarm(1);
	}

	first = 0;
	prev_charging_enabled = charging_enabled;
	prev_pwrsrc_enabled = pwrsrc_enabled;

	wake_unlock(&htc_batt_timer.battery_lock);
	pr_info("[BATT] %s: done\n", __func__);
	return;
}

static long htc_batt_ioctl(struct file *filp,
			unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	wake_lock(&htc_batt_timer.battery_lock);

	switch (cmd) {
	case HTC_BATT_IOCTL_READ_SOURCE: {
		if (copy_to_user((void __user *)arg,
			&htc_batt_info.rep.charging_source, sizeof(u32)))
			ret = -EFAULT;
		break;
	}
	case HTC_BATT_IOCTL_SET_BATT_ALARM: {
		u32 time_out = 0;
		if (copy_from_user(&time_out, (void *)arg, sizeof(u32))) {
			ret = -EFAULT;
			break;
		}

		htc_batt_timer.time_out = time_out;
		if (!htc_battery_initial) {
			htc_battery_initial = 1;
			batt_set_check_timer(htc_batt_timer.time_out);
		}
		break;
	}
	case HTC_BATT_IOCTL_GET_ADC_VREF: {
		if (copy_to_user((void __user *)arg, &htc_batt_info.adc_vref,
				sizeof(htc_batt_info.adc_vref))) {
			BATT_ERR("copy_to_user failed!");
			ret = -EFAULT;
		}
		break;
	}
	case HTC_BATT_IOCTL_GET_ADC_ALL: {
		if (copy_to_user((void __user *)arg, &htc_batt_info.adc_data,
					sizeof(struct battery_adc_reply))) {
			BATT_ERR("copy_to_user failed!");
			ret = -EFAULT;
		}
		break;
	}
	case HTC_BATT_IOCTL_CHARGER_CONTROL: {
		u32 charger_mode = 0;
		if (copy_from_user(&charger_mode, (void *)arg, sizeof(u32))) {
			ret = -EFAULT;
			break;
		}
		BATT_LOG("do charger control = %u", charger_mode);
		htc_battery_set_charging(charger_mode);
		break;
	}
	case HTC_BATT_IOCTL_UPDATE_BATT_INFO: {
		mutex_lock(&htc_batt_info.info_lock);
		if (copy_from_user(&htc_batt_info.rep, (void *)arg,
					sizeof(struct battery_info_reply))) {
			BATT_ERR("copy_from_user failed!");
			ret = -EFAULT;
			mutex_unlock(&htc_batt_info.info_lock);
			break;
		}
		mutex_unlock(&htc_batt_info.info_lock);

		BATT_LOG("ioctl: battery level update: %u",
			htc_batt_info.rep.level);

#ifdef CONFIG_HTC_BATT_ALARM
		/* Set a 3V voltage alarm when screen is on */
		if (screen_state == 1) {
			if (battery_vol_alarm_mode !=
				BATT_ALARM_CRITICAL_MODE)
				batt_set_voltage_alarm_mode(
					BATT_ALARM_CRITICAL_MODE);
		}
#endif
		htc_battery_core_update_changed();
		break;
	}
	case HTC_BATT_IOCTL_BATT_DEBUG_LOG:
		if (copy_from_user(htc_batt_info.debug_log, (void *)arg,
					DEBUG_LOG_LENGTH)) {
			BATT_ERR("copy debug log from user failed!");
			ret = -EFAULT;
		}
		break;
	case HTC_BATT_IOCTL_SET_VOLTAGE_ALARM: {
#ifdef CONFIG_HTC_BATT_ALARM
#else
		struct battery_vol_alarm alarm_data;

#endif
		if (copy_from_user(&alarm_data, (void *)arg,
					sizeof(struct battery_vol_alarm))) {
			BATT_ERR("user set batt alarm failed!");
			ret = -EFAULT;
			break;
		}

		htc_batt_timer.batt_alarm_status = 0;
		htc_batt_timer.batt_alarm_enabled = alarm_data.enable;

/* MATT porting
#ifdef CONFIG_HTC_BATT_ALARM
#else
		ret = batt_alarm_config(alarm_data.lower_threshold,
				alarm_data.upper_threshold);
		if (ret)
			BATT_ERR("batt alarm config failed!");
#endif
*/

		BATT_LOG("Set lower threshold: %d, upper threshold: %d, "
			"Enabled:%u.", alarm_data.lower_threshold,
			alarm_data.upper_threshold, alarm_data.enable);
		break;
	}
	case HTC_BATT_IOCTL_SET_ALARM_TIMER_FLAG: {
		/* alarm flag could be reset by cable. */
		unsigned int flag;
		if (copy_from_user(&flag, (void *)arg, sizeof(unsigned int))) {
			BATT_ERR("Set timer type into alarm failed!");
			ret = -EFAULT;
			break;
		}
		htc_batt_timer.alarm_timer_flag = flag;
		BATT_LOG("Set alarm timer flag:%u", flag);
		break;
	}
	default:
		BATT_ERR("%s: no matched ioctl cmd", __func__);
		break;
	}

	wake_unlock(&htc_batt_timer.battery_lock);

	return ret;
}

/* shutdown worker */
static void shutdown_worker(struct work_struct *work)
{
	BATT_LOG("shut down device due to battery removed");
	machine_power_off();
}

/*  MBAT_IN interrupt handler	*/
static void mbat_in_func(struct work_struct *work)
{
#if defined(CONFIG_MACH_RUBY) || defined(CONFIG_MACH_HOLIDAY) || defined(CONFIG_MACH_VIGOR)
	/* add sw debounce */
#define LTE_GPIO_MBAT_IN (61)
	if (gpio_get_value(LTE_GPIO_MBAT_IN) == 0) {
		pr_info("re-enable MBAT_IN irq!! due to false alarm\n");
		enable_irq(MSM_GPIO_TO_INT(LTE_GPIO_MBAT_IN));
		return;
	}
#endif

	BATT_LOG("shut down device due to MBAT_IN interrupt");
	htc_battery_set_charging(0);
	machine_power_off();

}
/* MATT porting */
#if 0
static irqreturn_t mbat_int_handler(int irq, void *data)
{
	struct htc_battery_platform_data *pdata = data;

	disable_irq_nosync(pdata->gpio_mbat_in);

	schedule_delayed_work(&mbat_in_struct, msecs_to_jiffies(50));

	return IRQ_HANDLED;
}
/*  MBAT_IN interrupt handler end   */
#endif

const struct file_operations htc_batt_fops = {
	.owner = THIS_MODULE,
	.open = htc_batt_open,
	.release = htc_batt_release,
	.unlocked_ioctl = htc_batt_ioctl,
};

static struct miscdevice htc_batt_device_node = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "htc_batt",
	.fops = &htc_batt_fops,
};

static void htc_batt_kobject_release(struct kobject *kobj)
{
	printk(KERN_ERR "htc_batt_kobject_release.\n");
	return;
}

static struct kobj_type htc_batt_ktype = {
	.release = htc_batt_kobject_release,
};

#ifdef CONFIG_HTC_BATT_ALARM
#ifdef CONFIG_HAS_EARLYSUSPEND
static void htc_battery_early_suspend(struct early_suspend *h)
{
	screen_state = 0;
	batt_set_voltage_alarm_mode(BATT_ALARM_DISABLE_MODE);
}

static void htc_battery_late_resume(struct early_suspend *h)
{
	screen_state = 1;
	batt_set_voltage_alarm_mode(BATT_ALARM_CRITICAL_MODE);
}
#endif
#endif

#define CHECH_TIME_TOLERANCE_MS	(1000)
static int htc_battery_prepare(struct device *dev)
{
	ktime_t interval;
	ktime_t slack = ktime_set(0, 0);
	ktime_t next_alarm;
	struct timespec xtime;
	unsigned long cur_jiffies;
	s64 next_alarm_sec = 0;
	int check_time = 0;

	xtime = CURRENT_TIME;
	cur_jiffies = jiffies;
	htc_batt_timer.total_time_ms += (cur_jiffies -
			htc_batt_timer.batt_system_jiffies) * MSEC_PER_SEC / HZ;
	htc_batt_timer.batt_system_jiffies = cur_jiffies;
	htc_batt_timer.batt_suspend_ms = xtime.tv_sec * MSEC_PER_SEC +
					xtime.tv_nsec / NSEC_PER_MSEC;

	if (suspend_highfreq_check_reason)
		check_time = BATT_SUSPEND_HIGHFREQ_CHECK_TIME;
	else
		check_time = BATT_SUSPEND_CHECK_TIME;

	interval = ktime_set(check_time - htc_batt_timer.total_time_ms / 1000, 0);
	next_alarm_sec = div_s64(interval.tv64, NSEC_PER_SEC);
	/* check if alarm is over time or in 1 second near future */
	if (next_alarm_sec <= 1) {
		BATT_LOG("%s: passing time:%lu ms, trigger batt_work immediately."
			"(suspend_highfreq_check_reason=0x%x)", __func__,
			htc_batt_timer.total_time_ms,
			suspend_highfreq_check_reason);
		htc_batt_schedule_batt_info_update();
		/* interval = ktime_set(check_time, 0);
		next_alarm = ktime_add(alarm_get_elapsed_realtime(), interval);
		alarm_start_range(&htc_batt_timer.batt_check_wakeup_alarm,
					next_alarm, ktime_add(next_alarm, slack));
		*/
		return -EBUSY;
	}

	BATT_LOG("%s: passing time:%lu ms, alarm will be triggered after %lld sec."
		"(suspend_highfreq_check_reason=0x%x)", __func__,
		htc_batt_timer.total_time_ms, next_alarm_sec,
		suspend_highfreq_check_reason);

	next_alarm = ktime_add(alarm_get_elapsed_realtime(), interval);
	alarm_start_range(&htc_batt_timer.batt_check_wakeup_alarm,
				next_alarm, ktime_add(next_alarm, slack));

	return 0;
}

static void htc_battery_complete(struct device *dev)
{
	unsigned long resume_ms;
	unsigned long sr_time_period_ms;
	unsigned long check_time;
	struct timespec xtime;

	xtime = CURRENT_TIME;
	htc_batt_timer.batt_system_jiffies = jiffies;
	resume_ms = xtime.tv_sec * MSEC_PER_SEC + xtime.tv_nsec / NSEC_PER_MSEC;
	sr_time_period_ms = resume_ms - htc_batt_timer.batt_suspend_ms;
	htc_batt_timer.total_time_ms += sr_time_period_ms;

	BATT_LOG("%s: sr_time_period=%lu ms; total passing time=%lu ms.",
			__func__, sr_time_period_ms, htc_batt_timer.total_time_ms);

	if (suspend_highfreq_check_reason)
		check_time = BATT_SUSPEND_HIGHFREQ_CHECK_TIME * MSEC_PER_SEC;
	else
		check_time = BATT_SUSPEND_CHECK_TIME * MSEC_PER_SEC;

	check_time -= CHECH_TIME_TOLERANCE_MS;

	/*
	 * When kernel resumes, battery driver should check total time to
	 * decide if do battery information update or just ignore.
	 */
	if (htc_batt_timer.total_time_ms >= check_time) {
		BATT_LOG("trigger batt_work while resume."
				"(suspend_highfreq_check_reason=0x%x)",
				suspend_highfreq_check_reason);
		htc_batt_schedule_batt_info_update();
	}

	return;
}

static struct dev_pm_ops htc_battery_8960_pm_ops = {
	.prepare = htc_battery_prepare,
	.complete = htc_battery_complete,
};

static int htc_battery_probe(struct platform_device *pdev)
{
	int i, rc = 0;
	struct htc_battery_platform_data *pdata = pdev->dev.platform_data;
	struct htc_battery_core *htc_battery_core_ptr;
	pr_debug("[BATT] %s() in\n", __func__);

	htc_battery_core_ptr = kmalloc(sizeof(struct htc_battery_core),
					GFP_KERNEL);
	if (!htc_battery_core_ptr) {
		BATT_ERR("%s: kmalloc failed for htc_battery_core_ptr.",
				__func__);
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&mbat_in_struct, mbat_in_func);
	INIT_DELAYED_WORK(&shutdown_work, shutdown_worker);
/* MATT porting */
#if 0
	if (pdata->gpio_mbat_in_trigger_level == MBAT_IN_HIGH_TRIGGER)
		rc = request_irq(pdata->gpio_mbat_in,
				mbat_int_handler, IRQF_TRIGGER_HIGH,
				"mbat_in", pdata);
	else if (pdata->gpio_mbat_in_trigger_level == MBAT_IN_LOW_TRIGGER)
		rc = request_irq(pdata->gpio_mbat_in,
				mbat_int_handler, IRQF_TRIGGER_LOW,
				"mbat_in", pdata);
	if (rc)
		BATT_ERR("request mbat_in irq failed!");
	else
		set_irq_wake(pdata->gpio_mbat_in, 1);
#endif

	htc_battery_core_ptr->func_show_batt_attr = htc_battery_show_batt_attr;
	htc_battery_core_ptr->func_show_cc_attr = htc_battery_show_cc_attr;
	htc_battery_core_ptr->func_get_battery_info = htc_batt_get_battery_info;
	htc_battery_core_ptr->func_charger_control = htc_batt_charger_control;
	htc_battery_core_ptr->func_set_full_level = htc_batt_set_full_level;
	htc_battery_core_ptr->func_context_event_handler =
											htc_batt_context_event_handler;

	htc_battery_core_register(&pdev->dev, htc_battery_core_ptr);

	htc_batt_info.device_id = pdev->id;
/* MATT porting */
#if 0
	htc_batt_info.guage_driver = pdata->guage_driver;
	htc_batt_info.charger = pdata->charger;
#endif

	htc_batt_info.is_open = 0;

	for (i = 0; i < ADC_REPLY_ARRAY_SIZE; i++)
		htc_batt_info.adc_vref[i] = 66;

	/* MATT add */
	htc_batt_info.critical_low_voltage_mv = pdata->critical_low_voltage_mv;
	htc_batt_info.critical_alarm_voltage_mv = pdata->critical_alarm_voltage_mv;
	htc_batt_info.overload_vol_thr_mv = pdata->overload_vol_thr_mv;
	htc_batt_info.overload_curr_thr_ma = pdata->overload_curr_thr_ma;
	chg_limit_active_mask = pdata->chg_limit_active_mask;
	htc_batt_info.igauge = &pdata->igauge;
	htc_batt_info.icharger = &pdata->icharger;
/* MATT porting */
#if 0
	htc_batt_info.mpp_config = &pdata->mpp_data;
#endif

	INIT_WORK(&htc_batt_timer.batt_work, batt_worker);
	init_timer(&htc_batt_timer.batt_timer);
	htc_batt_timer.batt_timer.function = batt_regular_timer_handler;
	alarm_init(&htc_batt_timer.batt_check_wakeup_alarm,
			ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP,
			batt_check_alarm_handler);
	htc_batt_timer.batt_wq = create_singlethread_workqueue("batt_timer");

	rc = misc_register(&htc_batt_device_node);
	if (rc) {
		BATT_ERR("Unable to register misc device %d",
			MISC_DYNAMIC_MINOR);
		goto fail;
	}

	htc_batt_kset = kset_create_and_add("event_to_daemon", NULL,
			kobject_get(&htc_batt_device_node.this_device->kobj));
	if (!htc_batt_kset) {
		rc = -ENOMEM;
		goto fail;
	}

	htc_batt_info.batt_timer_kobj.kset = htc_batt_kset;
	rc = kobject_init_and_add(&htc_batt_info.batt_timer_kobj,
				&htc_batt_ktype, NULL, "htc_batt_timer");
	if (rc) {
		BATT_ERR("init kobject htc_batt_timer failed.");
		kobject_put(&htc_batt_info.batt_timer_kobj);
		goto fail;
	}

	htc_batt_info.batt_cable_kobj.kset = htc_batt_kset;
	rc = kobject_init_and_add(&htc_batt_info.batt_cable_kobj,
				&htc_batt_ktype, NULL, "htc_cable_detect");
	if (rc) {
		BATT_ERR("init kobject htc_cable_timer failed.");
		kobject_put(&htc_batt_info.batt_timer_kobj);
		goto fail;
	}

	/* FIXME: workaround 8960 cable detect status is not correct
	 * at register_notfier call */
	if (htc_batt_info.icharger &&
			htc_batt_info.icharger->charger_change_notifier_register)
		htc_batt_info.icharger->charger_change_notifier_register(
											&cable_status_notifier);

/* MATT porting
	if (pdata->charger == SWITCH_CHARGER_TPS65200)
		tps_register_notifier(&tps_int_notifier);
*/
/* MATT porting
	pm8058_batt_alarm_register_notifier(&battery_alarm_notifier);
*/
/* MATT porting
	rc = pm8058_htc_config_mpp_and_adc_read(
				htc_batt_info.adc_vref,
				ADC_REPLY_ARRAY_SIZE,
				CHANNEL_ADC_VCHG,
				0, 0);
	if (rc) {
		BATT_ERR("Get Vref ADC value failed!");
		goto fail;
	}

	BATT_LOG("vref ADC: %d %d %d %d %d", htc_batt_info.adc_vref[0],
					htc_batt_info.adc_vref[1],
					htc_batt_info.adc_vref[2],
					htc_batt_info.adc_vref[3],
					htc_batt_info.adc_vref[4]);

	rc = htc_batt_get_battery_adc();
	if (rc) {
		BATT_ERR("Get first battery ADC value failed!");
		goto fail;
	}
*/

#ifdef CONFIG_HTC_BATT_ALARM
#ifdef CONFIG_HAS_EARLYSUSPEND
	early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;
	early_suspend.suspend = htc_battery_early_suspend;
	early_suspend.resume = htc_battery_late_resume;
	register_early_suspend(&early_suspend);
#endif
#endif
/* MATT add + */
htc_batt_timer.time_out = BATT_TIMER_UPDATE_TIME;
batt_set_check_timer(htc_batt_timer.time_out);
/* MATT add - */
	BATT_LOG("htc_battery_probe(): finish");

fail:
	kfree(htc_battery_core_ptr);
	return rc;
}

static struct platform_driver htc_battery_driver = {
	.probe	= htc_battery_probe,
	.driver	= {
		.name	= "htc_battery",
		.owner	= THIS_MODULE,
		.pm = &htc_battery_8960_pm_ops,
	},
};

static int __init htc_battery_init(void)
{
	htc_battery_initial = 0;
	htc_full_level_flag = 0;
	spin_lock_init(&htc_batt_info.batt_lock);
	wake_lock_init(&htc_batt_info.vbus_wake_lock, WAKE_LOCK_SUSPEND,
			"vbus_present");
	wake_lock_init(&htc_batt_timer.battery_lock, WAKE_LOCK_SUSPEND,
			"htc_battery_8960");
	wake_lock_init(&voltage_alarm_wake_lock, WAKE_LOCK_SUSPEND,
			"htc_voltage_alarm");
	mutex_init(&htc_batt_info.info_lock);
	mutex_init(&htc_batt_timer.schedule_lock);
	mutex_init(&cable_notifier_lock);
	mutex_init(&context_event_handler_lock);
#ifdef CONFIG_HTC_BATT_ALARM
	mutex_init(&batt_set_alarm_lock);
#endif

	/* cable_detect_register_notifier(&cable_status_notifier); */

	platform_driver_register(&htc_battery_driver);

	/* init battery parameters. */
	htc_batt_info.rep.batt_vol = 3700;
	htc_batt_info.rep.batt_id = 1;
	htc_batt_info.rep.batt_temp = 250;
	htc_batt_info.rep.level = 33;
	htc_batt_info.rep.full_bat = 1579999;
	htc_batt_info.rep.full_level = 100;
	htc_batt_info.rep.batt_state = 0;
	htc_batt_info.rep.temp_fault = -1;
	htc_batt_info.rep.overload = 0;
	htc_batt_timer.total_time_ms = 0;
	htc_batt_timer.batt_system_jiffies = jiffies;
	htc_batt_timer.batt_alarm_status = 0;
	htc_batt_timer.alarm_timer_flag = 0;

#ifdef CONFIG_HTC_BATT_ALARM
	battery_vol_alarm_mode = BATT_ALARM_NORMAL_MODE;
	screen_state = 1;
	alarm_data.lower_threshold = 2800;
	alarm_data.upper_threshold = 4400;
#endif

	return 0;
}

module_init(htc_battery_init);
MODULE_DESCRIPTION("HTC Battery Driver");
MODULE_LICENSE("GPL");

