/* Copyright (c) 2010-2012, The Linux Foundation. All rights reserved.
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

#include <linux/export.h>
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
	struct kgsl_power_stats bin;
};
spinlock_t tz_lock;

#define FLOOR			5000
#define CEILING			50000
#define SWITCH_OFF		200
#define SWITCH_OFF_RESET_TH	40
#define SKIP_COUNTER		500
#define TZ_RESET_ID		0x3
#define TZ_UPDATE_ID		0x4
#define TZ_CMD_ID              0x90

#define PARAM_INDEX_WRITE_DOWNTHRESHOLD 100
#define PARAM_INDEX_WRITE_UPTHRESHOLD 101
#define PARAM_INDEX_WRITE_MINGAPCOUNT 102
#define PARAM_INDEX_WRITE_NUMGAPS 103
#define PARAM_INDEX_WRITE_INITIDLEVECTOR 104
#define PARAM_INDEX_WRITE_DOWNTHRESHOLD_PERCENT 105
#define PARAM_INDEX_WRITE_UPTHRESHOLD_PERCENT 106
#define PARAM_INDEX_WRITE_DOWNTHRESHOLD_COUNT 107
#define PARAM_INDEX_WRITE_UPTHRESHOLD_COUNT 108
#define PARAM_INDEX_WRITE_ALGORITHM 109

#define PARAM_INDEX_READ_DOWNTHRESHOLD 200
#define PARAM_INDEX_READ_UPTHRESHOLD 201
#define PARAM_INDEX_READ_MINGAPCOUNT 202
#define PARAM_INDEX_READ_NUMGAPS 203
#define PARAM_INDEX_READ_INITIDLEVECTOR 204
#define PARAM_INDEX_READ_DOWNTHRESHOLD_PERCENT 205
#define PARAM_INDEX_READ_UPTHRESHOLD_PERCENT 206
#define PARAM_INDEX_READ_DOWNTHRESHOLD_COUNT 207
#define PARAM_INDEX_READ_UPTHRESHOLD_COUNT 208
#define PARAM_INDEX_READ_ALGORITHM 209

#ifdef CONFIG_MSM_SCM
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
#endif 

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
		kgsl_pwrctrl_pwrlevel_change(device, pwr->max_pwrlevel);

	mutex_unlock(&device->mutex);
	return count;
}

static ssize_t dcvs_downthreshold_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	int val, ret;

	val = __secure_tz_entry(TZ_CMD_ID, 0, PARAM_INDEX_READ_DOWNTHRESHOLD);
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

	__secure_tz_entry(TZ_CMD_ID, val, PARAM_INDEX_WRITE_DOWNTHRESHOLD);

	return count;
}

static ssize_t dcvs_upthreshold_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	int val, ret;

	val = __secure_tz_entry(TZ_CMD_ID, 0, PARAM_INDEX_READ_UPTHRESHOLD);
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

	__secure_tz_entry(TZ_CMD_ID, val, PARAM_INDEX_WRITE_UPTHRESHOLD);

	return count;
}

static ssize_t dcvs_down_count_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	int val, ret;

	val = __secure_tz_entry(TZ_CMD_ID, 0, PARAM_INDEX_READ_MINGAPCOUNT);
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

	__secure_tz_entry(TZ_CMD_ID, val, PARAM_INDEX_WRITE_MINGAPCOUNT);

	return count;
}

static ssize_t dcvs_numgaps_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	int val, ret;

	val = __secure_tz_entry(TZ_CMD_ID, 0, PARAM_INDEX_READ_NUMGAPS);
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

	__secure_tz_entry(TZ_CMD_ID, val, PARAM_INDEX_WRITE_NUMGAPS);

	return count;
}

static ssize_t dcvs_init_idle_vector_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	int val, ret;

	val = __secure_tz_entry(TZ_CMD_ID, 0, PARAM_INDEX_READ_INITIDLEVECTOR);
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

	__secure_tz_entry(TZ_CMD_ID, val, PARAM_INDEX_WRITE_INITIDLEVECTOR);

	return count;
}

static ssize_t dcvs_algorithm_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	int val, ret;
	val = __secure_tz_entry(TZ_CMD_ID, 0, PARAM_INDEX_READ_ALGORITHM);

	ret = sprintf(buf, "%d\n", val);

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

	__secure_tz_entry(TZ_CMD_ID, val, PARAM_INDEX_WRITE_ALGORITHM);

	return count;
}

static ssize_t dcvs_upthreshold_percent_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	int val, ret;

	val = __secure_tz_entry(TZ_CMD_ID, 0, PARAM_INDEX_READ_UPTHRESHOLD_PERCENT);
	ret = sprintf(buf, "%d\n", val);

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

	__secure_tz_entry(TZ_CMD_ID, val, PARAM_INDEX_WRITE_UPTHRESHOLD_PERCENT);

	return count;
}

static ssize_t dcvs_downthreshold_percent_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	int val, ret;

	val = __secure_tz_entry(TZ_CMD_ID, 0, PARAM_INDEX_READ_DOWNTHRESHOLD_PERCENT);
	ret = sprintf(buf, "%d\n", val);

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

	__secure_tz_entry(TZ_CMD_ID, val, PARAM_INDEX_WRITE_DOWNTHRESHOLD_PERCENT);

	return count;
}

static ssize_t dcvs_upthreshold_count_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	int val, ret;

	val = __secure_tz_entry(TZ_CMD_ID, 0, PARAM_INDEX_READ_UPTHRESHOLD_COUNT);
	ret = sprintf(buf, "%d\n", val);

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

	__secure_tz_entry(TZ_CMD_ID, val, PARAM_INDEX_WRITE_UPTHRESHOLD_COUNT);

	return count;
}

static ssize_t dcvs_downthreshold_count_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	int val, ret;

	val = __secure_tz_entry(TZ_CMD_ID, 0, PARAM_INDEX_READ_DOWNTHRESHOLD_COUNT);
	ret = sprintf(buf, "%d\n", val);

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

	__secure_tz_entry(TZ_CMD_ID, val, PARAM_INDEX_WRITE_DOWNTHRESHOLD_COUNT);

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

static void tz_wake(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	struct tz_priv *priv = pwrscale->priv;
	if (device->state != KGSL_STATE_NAP &&
		priv->governor == TZ_GOVERNOR_ONDEMAND)
		kgsl_pwrctrl_pwrlevel_change(device,
					device->pwrctrl.default_pwrlevel);
}

static void tz_idle(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	struct tz_priv *priv = pwrscale->priv;
	struct kgsl_power_stats stats;
	int val, idle, total_time;

	if (priv->governor == TZ_GOVERNOR_PERFORMANCE)
		return;

	device->ftbl->power_stats(device, &stats);
	priv->bin.total_time += stats.total_time;
	priv->bin.busy_time += stats.busy_time;
	if ((stats.total_time == 0) ||
		(priv->bin.total_time < FLOOR))
		return;

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

	if (priv->bin.busy_time > CEILING) {
		val = -1;
	} else {
		idle = priv->bin.total_time - priv->bin.busy_time;
		idle = (idle > 0) ? idle : 0;
		
		total_time = stats.total_time & 0x0FFFFFFF;
		total_time |= (pwr->active_pwrlevel) << 28;

		val = __secure_tz_entry(TZ_UPDATE_ID, idle, total_time);
	}
	priv->bin.total_time = 0;
	priv->bin.busy_time = 0;
	if (val)
		kgsl_pwrctrl_pwrlevel_change(device,
					     pwr->active_pwrlevel + val);
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
	priv->bin.total_time = 0;
	priv->bin.busy_time = 0;
}

#ifdef CONFIG_MSM_SCM
static int tz_init(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	struct tz_priv *priv;
	int ret;

	priv = pwrscale->priv = kzalloc(sizeof(struct tz_priv), GFP_KERNEL);
	if (pwrscale->priv == NULL)
		return -ENOMEM;

	priv->governor = TZ_GOVERNOR_ONDEMAND;
	spin_lock_init(&tz_lock);
	kgsl_pwrscale_policy_add_files(device, pwrscale, &tz_attr_group);

	ret = __secure_tz_entry(TZ_CMD_ID, 0, PARAM_INDEX_WRITE_ALGORITHM);

	if(ret == 1)
		pr_info("Using HTC GPU DCVS algorithm\n");
	else
		pr_info("Using QCT GPU DCVS algorithm\n");

	return 0;
}
#else
static int tz_init(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	return -EINVAL;
}
#endif 

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
