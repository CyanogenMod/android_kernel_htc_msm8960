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
 *
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <mach/socinfo.h>
#include <mach/scm.h>

#include "kgsl.h"
#include "kgsl_pwrscale.h"
#include "kgsl_device.h"

#define TZ_GOVERNOR_PERFORMANCE 0
#define TZ_GOVERNOR_ONDEMAND    1

struct tz_priv {
	int governor;
	unsigned int no_switch_cnt;
	unsigned int skip_cnt;
};
spinlock_t tz_lock;

#define SWITCH_OFF		200
#define SWITCH_OFF_RESET_TH	40
#define SKIP_COUNTER		500
#define TZ_RESET_ID		0x3
#define TZ_UPDATE_ID		0x4

#define PARAM_INDEX_WRITE_DOWNTHRESHOLD 100
#define PARAM_INDEX_WRITE_UPTHRESHOLD 101
#define PARAM_INDEX_WRITE_MINGAPCOUNT 102
#define PARAM_INDEX_WRITE_NUMGAPS 103
#define PARAM_INDEX_WRITE_INITIDLEVECTOR 104


#define PARAM_INDEX_READ_DOWNTHRESHOLD 200
#define PARAM_INDEX_READ_UPTHRESHOLD 201
#define PARAM_INDEX_READ_MINGAPCOUNT 202
#define PARAM_INDEX_READ_NUMGAPS 203
#define PARAM_INDEX_READ_INITIDLEVECTOR 204


#ifdef CONFIG_MSM_SCM
/* Trap into the TrustZone, and call funcs there. */
static int __secure_tz_entry(u32 cmd, u32 val, u32 id)
{
	int ret;
	spin_lock(&tz_lock);
	__iowmb();
	ret = scm_call_atomic2(SCM_SVC_IO, cmd, val, id);
	spin_unlock(&tz_lock);
	return ret;
}
#else
static int __secure_tz_entry(u32 cmd, u32 val, u32 id)
{
	return 0;
}
#endif /* CONFIG_MSM_SCM */

static ssize_t tz_governor_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	struct tz_priv *priv = pwrscale->priv;
	int ret;

	if (priv->governor == TZ_GOVERNOR_ONDEMAND)
		ret = snprintf(buf, 10, "ondemand\n");
	else
		ret = snprintf(buf, 13, "performance\n");

	return ret;
}

static ssize_t tz_governor_store(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				 const char *buf, size_t count)
{
	char str[20];
	struct tz_priv *priv = pwrscale->priv;
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	int ret;

	ret = sscanf(buf, "%20s", str);
	if (ret != 1)
		return -EINVAL;

	mutex_lock(&device->mutex);

	if (!strncmp(str, "ondemand", 8))
		priv->governor = TZ_GOVERNOR_ONDEMAND;
	else if (!strncmp(str, "performance", 11))
		priv->governor = TZ_GOVERNOR_PERFORMANCE;

	if (priv->governor == TZ_GOVERNOR_PERFORMANCE)
		kgsl_pwrctrl_pwrlevel_change(device, pwr->thermal_pwrlevel);

	mutex_unlock(&device->mutex);
	return count;
}

static ssize_t dcvs_downthreshold_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	int val, ret;
	val = __secure_tz_entry(TZ_UPDATE_ID, 0, PARAM_INDEX_READ_DOWNTHRESHOLD);

	ret = sprintf(buf, "%d\n", val);

	return ret;
}

static ssize_t dcvs_downthreshold_store(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				 const char *buf, size_t count)
{
	int val, ret;

	ret = sscanf(buf, "%d", &val);

	if (ret != 1)
		return -EINVAL;

	__secure_tz_entry(TZ_UPDATE_ID, val, PARAM_INDEX_WRITE_DOWNTHRESHOLD);

	return count;
}

static ssize_t dcvs_upthreshold_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	int val, ret;
	val = __secure_tz_entry(TZ_UPDATE_ID, 0, PARAM_INDEX_READ_UPTHRESHOLD);

	ret = sprintf(buf, "%d\n", val);

	return ret;
}

static ssize_t dcvs_upthreshold_store(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				 const char *buf, size_t count)
{
	int val, ret;

	ret = sscanf(buf, "%d", &val);

	if (ret != 1)
		return -EINVAL;

	__secure_tz_entry(TZ_UPDATE_ID, val, PARAM_INDEX_WRITE_UPTHRESHOLD);

	return count;
}

static ssize_t dcvs_down_count_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	int val, ret;
	val = __secure_tz_entry(TZ_UPDATE_ID, 0, PARAM_INDEX_READ_MINGAPCOUNT);

	ret = sprintf(buf, "%d\n", val);

	return ret;
}

static ssize_t dcvs_down_count_store(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				 const char *buf, size_t count)
{
	int val, ret;

	ret = sscanf(buf, "%d", &val);

	if (ret != 1)
		return -EINVAL;

	__secure_tz_entry(TZ_UPDATE_ID, val, PARAM_INDEX_WRITE_MINGAPCOUNT);

	return count;
}

static ssize_t dcvs_numgaps_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	int val, ret;
	val = __secure_tz_entry(TZ_UPDATE_ID, 0, PARAM_INDEX_READ_NUMGAPS);

	ret = sprintf(buf, "%d\n", val);

	return ret;
}

static ssize_t dcvs_numgaps_store(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				 const char *buf, size_t count)
{
	int val, ret;

	ret = sscanf(buf, "%d", &val);

	if (ret != 1)
		return -EINVAL;

	__secure_tz_entry(TZ_UPDATE_ID, val, PARAM_INDEX_WRITE_NUMGAPS);

	return count;
}

static ssize_t dcvs_init_idle_vector_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	int val, ret;
	val = __secure_tz_entry(TZ_UPDATE_ID, 0, PARAM_INDEX_READ_INITIDLEVECTOR);

	ret = sprintf(buf, "%d\n", val);

	return ret;
}

static ssize_t dcvs_init_idle_vector_store(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				 const char *buf, size_t count)
{
	int val, ret;

	ret = sscanf(buf, "%d", &val);

	if (ret != 1)
		return -EINVAL;

	__secure_tz_entry(TZ_UPDATE_ID, val, PARAM_INDEX_WRITE_INITIDLEVECTOR);

	return count;
}

//0: DCVS by busy percentage
//1: DCVS by busy time (Qualcomm original)
static int dcvs_algorithm = 0;

#define FRAME_INTERVAL 16667
#define DCVS_UPTHRESHOLD_PERCENT 70
#define DCVS_DOWNTHRESHOLD_PERCENT 50

#define DCVS_UPTHRESHOLD_COUNT 2
#define DCVS_DOWNTHRESHOLD_COUNT 5

static int dcvs_upthreshold_percent = DCVS_UPTHRESHOLD_PERCENT;
static int dcvs_downthreshold_percent = DCVS_DOWNTHRESHOLD_PERCENT;
static int dcvs_upthreshold_count = DCVS_UPTHRESHOLD_COUNT;
static int dcvs_downthreshold_count = DCVS_DOWNTHRESHOLD_COUNT;

static ssize_t dcvs_algorithm_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	int ret;

	ret = sprintf(buf, "%d\n", dcvs_algorithm);

	return ret;
}

static ssize_t dcvs_algorithm_store(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				const char *buf, size_t count)
{
	int val, ret;

	ret = sscanf(buf, "%d", &val);

	if (ret != 1)
		return -EINVAL;

	dcvs_algorithm = val;

	return count;
}

static ssize_t dcvs_upthreshold_percent_show(struct kgsl_device *device,
					struct kgsl_pwrscale *pwrscale,
					char *buf)
{
	int ret;

	ret = sprintf(buf, "%d\n", dcvs_upthreshold_percent);

	return ret;
}

static ssize_t dcvs_upthreshold_percent_store(struct kgsl_device *device,
					struct kgsl_pwrscale *pwrscale,
					const char *buf, size_t count)
{
	int val, ret;

	ret = sscanf(buf, "%d", &val);

	if (ret != 1)
		return -EINVAL;

	dcvs_upthreshold_percent = val;

	return count;
}

static ssize_t dcvs_downthreshold_percent_show(struct kgsl_device *device,
					struct kgsl_pwrscale *pwrscale,
					char *buf)
{
	int ret;

	ret = sprintf(buf, "%d\n", dcvs_downthreshold_percent);

	return ret;
}

static ssize_t dcvs_downthreshold_percent_store(struct kgsl_device *device,
					struct kgsl_pwrscale *pwrscale,
					const char *buf, size_t count)
{
	int val, ret;

	ret = sscanf(buf, "%d", &val);

	if (ret != 1)
		return -EINVAL;

	dcvs_downthreshold_percent = val;

	return count;
}

static ssize_t dcvs_upthreshold_count_show(struct kgsl_device *device,
					struct kgsl_pwrscale *pwrscale,
					char *buf)
{
	int ret;

	ret = sprintf(buf, "%d\n", dcvs_upthreshold_count);

	return ret;
}

static ssize_t dcvs_upthreshold_count_store(struct kgsl_device *device,
					struct kgsl_pwrscale *pwrscale,
					const char *buf, size_t count)
{
	int val, ret;

	ret = sscanf(buf, "%d", &val);

	if (ret != 1)
		return -EINVAL;

	dcvs_upthreshold_count = val;

	return count;
}

static ssize_t dcvs_downthreshold_count_show(struct kgsl_device *device,
					struct kgsl_pwrscale *pwrscale,
					char *buf)
{
	int ret;

	ret = sprintf(buf, "%d\n", dcvs_downthreshold_count);

	return ret;
}

static ssize_t dcvs_downthreshold_count_store(struct kgsl_device *device,
					struct kgsl_pwrscale *pwrscale,
					const char *buf, size_t count)
{
	int val, ret;

	ret = sscanf(buf, "%d", &val);

	if (ret != 1)
		return -EINVAL;

	dcvs_downthreshold_count = val;

	return count;
}


PWRSCALE_POLICY_ATTR(governor, 0644, tz_governor_show, tz_governor_store);
PWRSCALE_POLICY_ATTR(dcvs_downthreshold, 0644, dcvs_downthreshold_show, dcvs_downthreshold_store);
PWRSCALE_POLICY_ATTR(dcvs_upthreshold, 0644, dcvs_upthreshold_show, dcvs_upthreshold_store);
PWRSCALE_POLICY_ATTR(dcvs_down_count, 0644, dcvs_down_count_show, dcvs_down_count_store);
PWRSCALE_POLICY_ATTR(dcvs_numgaps, 0644, dcvs_numgaps_show, dcvs_numgaps_store);
PWRSCALE_POLICY_ATTR(dcvs_init_idle_vector, 0644, dcvs_init_idle_vector_show, dcvs_init_idle_vector_store);
PWRSCALE_POLICY_ATTR(dcvs_algorithm, 0644, dcvs_algorithm_show, dcvs_algorithm_store);
PWRSCALE_POLICY_ATTR(dcvs_upthreshold_percent, 0644, dcvs_upthreshold_percent_show, dcvs_upthreshold_percent_store);
PWRSCALE_POLICY_ATTR(dcvs_downthreshold_percent, 0644, dcvs_downthreshold_percent_show, dcvs_downthreshold_percent_store);
PWRSCALE_POLICY_ATTR(dcvs_upthreshold_count, 0644, dcvs_upthreshold_count_show, dcvs_upthreshold_count_store);
PWRSCALE_POLICY_ATTR(dcvs_downthreshold_count, 0644, dcvs_downthreshold_count_show, dcvs_downthreshold_count_store);


static struct attribute *tz_attrs[] = {
	&policy_attr_governor.attr,
	&policy_attr_dcvs_downthreshold.attr,
	&policy_attr_dcvs_upthreshold.attr,
	&policy_attr_dcvs_down_count.attr,
	&policy_attr_dcvs_numgaps.attr,
	&policy_attr_dcvs_init_idle_vector.attr,
	&policy_attr_dcvs_algorithm.attr,
	&policy_attr_dcvs_upthreshold_percent.attr,
	&policy_attr_dcvs_downthreshold_percent.attr,
	&policy_attr_dcvs_upthreshold_count.attr,
	&policy_attr_dcvs_downthreshold_count.attr,
	NULL
};

static struct attribute_group tz_attr_group = {
	.attrs = tz_attrs,
};

static int dcvs_total_time = 0;
static int dcvs_busy_time = 0;
static int dcvs_up_count = 0;
static int dcvs_down_count = 0;

int dcvs_update(int total_time, int busy_time)
{
	int percent = busy_time * 100 / total_time;

	if (percent > dcvs_upthreshold_percent) {
		if (++dcvs_up_count >= dcvs_upthreshold_count) {
			dcvs_down_count = 0;
			return -1;
		}
	} else if (percent < dcvs_downthreshold_percent) {
		dcvs_up_count = 0;
		if (++dcvs_down_count >= dcvs_downthreshold_count) {
			return 1;
		}
	} else {
		dcvs_down_count = 0;
		dcvs_up_count = 0;
	}

	return 0;
}

static void tz_wake(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	struct tz_priv *priv = pwrscale->priv;
	if (device->state != KGSL_STATE_NAP &&
		priv->governor == TZ_GOVERNOR_ONDEMAND &&
		device->pwrctrl.restore_slumber == 0)
		kgsl_pwrctrl_pwrlevel_change(device,
					     device->pwrctrl.default_pwrlevel);
}

static void tz_idle(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	struct tz_priv *priv = pwrscale->priv;
	struct kgsl_power_stats stats;
	int val, idle;

	/* In "performance" mode the clock speed always stays
	   the same */

	if (priv->governor == TZ_GOVERNOR_PERFORMANCE)
		return;

	device->ftbl->power_stats(device, &stats);
	if (stats.total_time == 0)
		return;

	/* If the GPU has stayed in turbo mode for a while, *
	 * stop writing out values. */
	if (pwr->active_pwrlevel == 0) {
		if (priv->no_switch_cnt > SWITCH_OFF) {
			priv->skip_cnt++;
			if (priv->skip_cnt > SKIP_COUNTER) {
				priv->no_switch_cnt -= SWITCH_OFF_RESET_TH;
				priv->skip_cnt = 0;
			}
			return;
		}
		priv->no_switch_cnt++;
	} else {
		priv->no_switch_cnt = 0;
	}

	idle = stats.total_time - stats.busy_time;
	idle = (idle > 0) ? idle : 0;

	dcvs_total_time += stats.total_time;
	if (idle)
		dcvs_busy_time += stats.busy_time;
	else
		dcvs_busy_time += stats.total_time;

	if (dcvs_algorithm == 0) { //DCVS algorithm by percentage
		if (dcvs_total_time < FRAME_INTERVAL)
			return;

		val = dcvs_update(dcvs_total_time, dcvs_busy_time);
	} else { //Qualcomm DCVS algorithm
		val = __secure_tz_entry(TZ_UPDATE_ID, idle, device->id);
	}

	if (val)
		kgsl_pwrctrl_pwrlevel_change(device,
					     pwr->active_pwrlevel + val);

	dcvs_total_time = 0;
	dcvs_busy_time = 0;
}

static void tz_busy(struct kgsl_device *device,
	struct kgsl_pwrscale *pwrscale)
{
	device->on_time = ktime_to_us(ktime_get());
}

static void tz_sleep(struct kgsl_device *device,
	struct kgsl_pwrscale *pwrscale)
{
	struct tz_priv *priv = pwrscale->priv;

	__secure_tz_entry(TZ_RESET_ID, 0, device->id);
	priv->no_switch_cnt = 0;

	dcvs_total_time = 0;
	dcvs_busy_time = 0;
	dcvs_up_count = 0;
	dcvs_down_count = 0;
}

static int tz_init(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	struct tz_priv *priv;

	/* Trustzone is only valid for some SOCs */
	if (!(cpu_is_msm8x60() || cpu_is_msm8960()))
		return -EINVAL;

	priv = pwrscale->priv = kzalloc(sizeof(struct tz_priv), GFP_KERNEL);
	if (pwrscale->priv == NULL)
		return -ENOMEM;

	priv->governor = TZ_GOVERNOR_ONDEMAND;
	spin_lock_init(&tz_lock);
	kgsl_pwrscale_policy_add_files(device, pwrscale, &tz_attr_group);

	return 0;
}

static void tz_close(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	kgsl_pwrscale_policy_remove_files(device, pwrscale, &tz_attr_group);
	kfree(pwrscale->priv);
	pwrscale->priv = NULL;
}

struct kgsl_pwrscale_policy kgsl_pwrscale_policy_tz = {
	.name = "trustzone",
	.init = tz_init,
	.busy = tz_busy,
	.idle = tz_idle,
	.sleep = tz_sleep,
	.wake = tz_wake,
	.close = tz_close
};
EXPORT_SYMBOL(kgsl_pwrscale_policy_tz);
