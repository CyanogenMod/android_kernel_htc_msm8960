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
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/bug.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <asm/hardware/gic.h>
#include <mach/msm_iomap.h>
#include <mach/rpm.h>
#include <mach/board_htc.h>


struct msm_rpm_request {
	struct msm_rpm_iv_pair *req;
	int count;
	uint32_t *ctx_mask_ack;
	uint32_t *sel_masks_ack;
	struct completion *done;
};

struct msm_rpm_notif_config {
	struct msm_rpm_iv_pair iv[SEL_MASK_SIZE * 2];
};

#define configured_iv(notif_cfg) ((notif_cfg)->iv)
#define registered_iv(notif_cfg) ((notif_cfg)->iv + msm_rpm_sel_mask_size)

static uint32_t msm_rpm_sel_mask_size;
static struct msm_rpm_platform_data msm_rpm_data;

static stats_blob *msm_rpm_stat_data;

static DEFINE_MUTEX(msm_rpm_mutex);
static DEFINE_SPINLOCK(msm_rpm_lock);
static DEFINE_SPINLOCK(msm_rpm_irq_lock);

static struct msm_rpm_request *msm_rpm_request;
static struct msm_rpm_request msm_rpm_request_irq_mode;
static struct msm_rpm_request msm_rpm_request_poll_mode;

static LIST_HEAD(msm_rpm_notifications);
static struct msm_rpm_notif_config msm_rpm_notif_cfgs[MSM_RPM_CTX_SET_COUNT];
static bool msm_rpm_init_notif_done;

static inline unsigned int target_enum(unsigned int id)
{
	BUG_ON(id >= MSM_RPM_ID_LAST);
	return msm_rpm_data.target_id[id].id;
}

static inline unsigned int target_status(unsigned int id)
{
	BUG_ON(id >= MSM_RPM_STATUS_ID_LAST);
	return msm_rpm_data.target_status[id];
}

static inline unsigned int target_ctrl(unsigned int id)
{
	BUG_ON(id >= MSM_RPM_CTRL_LAST);
	return msm_rpm_data.target_ctrl_id[id];
}

static inline uint32_t msm_rpm_read(unsigned int page, unsigned int reg)
{
	return __raw_readl(msm_rpm_data.reg_base_addrs[page] + reg * 4);
}

static inline void msm_rpm_write(
	unsigned int page, unsigned int reg, uint32_t value)
{
	__raw_writel(value,
		msm_rpm_data.reg_base_addrs[page] + reg * 4);
}

static inline void msm_rpm_read_contiguous(
	unsigned int page, unsigned int reg, uint32_t *values, int count)
{
	int i;

	for (i = 0; i < count; i++)
		values[i] = msm_rpm_read(page, reg + i);
}

static inline void msm_rpm_write_contiguous(
	unsigned int page, unsigned int reg, uint32_t *values, int count)
{
	int i;

	for (i = 0; i < count; i++)
		msm_rpm_write(page, reg + i, values[i]);
}

static inline void msm_rpm_write_contiguous_zeros(
	unsigned int page, unsigned int reg, int count)
{
	int i;

	for (i = 0; i < count; i++)
		msm_rpm_write(page, reg + i, 0);
}

static inline uint32_t msm_rpm_map_id_to_sel(uint32_t id)
{
	return (id >= MSM_RPM_ID_LAST) ? msm_rpm_data.sel_last + 1 :
		msm_rpm_data.target_id[id].sel;
}

static int msm_rpm_fill_sel_masks(
	uint32_t *sel_masks, struct msm_rpm_iv_pair *req, int count)
{
	uint32_t sel;
	int i;

	for (i = 0; i < count; i++) {
		sel = msm_rpm_map_id_to_sel(req[i].id);

		if (sel > msm_rpm_data.sel_last) {
			pr_err("%s(): RPM ID %d not defined for target\n",
					__func__, req[i].id);
			return -EINVAL;
		}

		sel_masks[msm_rpm_get_sel_mask_reg(sel)] |=
			msm_rpm_get_sel_mask(sel);
	}

	return 0;
}

static inline void msm_rpm_send_req_interrupt(void)
{
	__raw_writel(msm_rpm_data.ipc_rpm_val,
			msm_rpm_data.ipc_rpm_reg);
}

static int msm_rpm_process_ack_interrupt(void)
{
	uint32_t ctx_mask_ack;
	uint32_t sel_masks_ack[SEL_MASK_SIZE] = {0};

	ctx_mask_ack = msm_rpm_read(MSM_RPM_PAGE_CTRL,
			target_ctrl(MSM_RPM_CTRL_ACK_CTX_0));
	msm_rpm_read_contiguous(MSM_RPM_PAGE_CTRL,
		target_ctrl(MSM_RPM_CTRL_ACK_SEL_0),
		sel_masks_ack, msm_rpm_sel_mask_size);

	if (ctx_mask_ack & msm_rpm_get_ctx_mask(MSM_RPM_CTX_NOTIFICATION)) {
		struct msm_rpm_notification *n;
		int i;

		list_for_each_entry(n, &msm_rpm_notifications, list)
			for (i = 0; i < msm_rpm_sel_mask_size; i++)
				if (sel_masks_ack[i] & n->sel_masks[i]) {
					up(&n->sem);
					break;
				}

		msm_rpm_write_contiguous_zeros(MSM_RPM_PAGE_CTRL,
			target_ctrl(MSM_RPM_CTRL_ACK_SEL_0),
			msm_rpm_sel_mask_size);
		msm_rpm_write(MSM_RPM_PAGE_CTRL,
			target_ctrl(MSM_RPM_CTRL_ACK_CTX_0), 0);
		
		mb();

		return 1;
	}

	if (msm_rpm_request) {
		int i;

		*(msm_rpm_request->ctx_mask_ack) = ctx_mask_ack;
		memcpy(msm_rpm_request->sel_masks_ack, sel_masks_ack,
			sizeof(sel_masks_ack));

		for (i = 0; i < msm_rpm_request->count; i++)
			msm_rpm_request->req[i].value =
				msm_rpm_read(MSM_RPM_PAGE_ACK,
				target_enum(msm_rpm_request->req[i].id));

		msm_rpm_write_contiguous_zeros(MSM_RPM_PAGE_CTRL,
			target_ctrl(MSM_RPM_CTRL_ACK_SEL_0),
			msm_rpm_sel_mask_size);
		msm_rpm_write(MSM_RPM_PAGE_CTRL,
			target_ctrl(MSM_RPM_CTRL_ACK_CTX_0), 0);
		
		mb();

		if (msm_rpm_request->done)
			complete_all(msm_rpm_request->done);

		msm_rpm_request = NULL;
		return 0;
	}

	return 2;
}

static void msm_rpm_err_fatal(void)
{
	
	__raw_writel(0x1, msm_rpm_data.ipc_rpm_reg);
	panic("RPM error fataled");
}

static irqreturn_t msm_rpm_err_interrupt(int irq, void *dev_id)
{
	msm_rpm_err_fatal();
	return IRQ_HANDLED;
}

static irqreturn_t msm_rpm_ack_interrupt(int irq, void *dev_id)
{
	unsigned long flags;
	int rc;

	if (dev_id != &msm_rpm_ack_interrupt)
		return IRQ_NONE;

	spin_lock_irqsave(&msm_rpm_irq_lock, flags);
	rc = msm_rpm_process_ack_interrupt();
	spin_unlock_irqrestore(&msm_rpm_irq_lock, flags);

	return IRQ_HANDLED;
}

static void msm_rpm_busy_wait_for_request_completion(
	bool allow_async_completion)
{
	int rc;

	do {
		while (!gic_is_irq_pending(msm_rpm_data.irq_ack) &&
				msm_rpm_request) {
			if (allow_async_completion)
				spin_unlock(&msm_rpm_irq_lock);
			if (gic_is_irq_pending(msm_rpm_data.irq_err))
				msm_rpm_err_fatal();
			gic_clear_irq_pending(msm_rpm_data.irq_err);
			udelay(1);
			if (allow_async_completion)
				spin_lock(&msm_rpm_irq_lock);
		}

		if (!msm_rpm_request)
			break;

		rc = msm_rpm_process_ack_interrupt();
		gic_clear_irq_pending(msm_rpm_data.irq_ack);
	} while (rc);
}

static int msm_rpm_set_exclusive(int ctx,
	uint32_t *sel_masks, struct msm_rpm_iv_pair *req, int count)
{
	DECLARE_COMPLETION_ONSTACK(ack);
	unsigned long flags;
	uint32_t ctx_mask = msm_rpm_get_ctx_mask(ctx);
	uint32_t ctx_mask_ack = 0;
	uint32_t sel_masks_ack[SEL_MASK_SIZE];
	int i;

	msm_rpm_request_irq_mode.req = req;
	msm_rpm_request_irq_mode.count = count;
	msm_rpm_request_irq_mode.ctx_mask_ack = &ctx_mask_ack;
	msm_rpm_request_irq_mode.sel_masks_ack = sel_masks_ack;
	msm_rpm_request_irq_mode.done = &ack;

	spin_lock_irqsave(&msm_rpm_lock, flags);
	spin_lock(&msm_rpm_irq_lock);

	BUG_ON(msm_rpm_request);
	msm_rpm_request = &msm_rpm_request_irq_mode;

	for (i = 0; i < count; i++) {
		BUG_ON(target_enum(req[i].id) >= MSM_RPM_ID_LAST);
		msm_rpm_write(MSM_RPM_PAGE_REQ,
				target_enum(req[i].id), req[i].value);
	}

	msm_rpm_write_contiguous(MSM_RPM_PAGE_CTRL,
		target_ctrl(MSM_RPM_CTRL_REQ_SEL_0),
		sel_masks, msm_rpm_sel_mask_size);
	msm_rpm_write(MSM_RPM_PAGE_CTRL,
		target_ctrl(MSM_RPM_CTRL_REQ_CTX_0), ctx_mask);

	/* Ensure RPM data is written before sending the interrupt */
	mb();
	msm_rpm_send_req_interrupt();

	spin_unlock(&msm_rpm_irq_lock);
	spin_unlock_irqrestore(&msm_rpm_lock, flags);

	wait_for_completion(&ack);

	BUG_ON((ctx_mask_ack & ~(msm_rpm_get_ctx_mask(MSM_RPM_CTX_REJECTED)))
		!= ctx_mask);
	BUG_ON(memcmp(sel_masks, sel_masks_ack, sizeof(sel_masks_ack)));

	if (ctx_mask_ack & msm_rpm_get_ctx_mask(MSM_RPM_CTX_REJECTED)) {
		pr_warn("[K] %s: following request is rejected by rpm\n", __func__);
		for (i = 0; i < count; i++)
				pr_warn("[K] %s: id: %d, value: %d\n", __func__, req[i].id, req[i].value);
		return -ENOSPC;
	} else {
		return 0;
	}
}

static int msm_rpm_set_exclusive_noirq(int ctx,
	uint32_t *sel_masks, struct msm_rpm_iv_pair *req, int count)
{
	unsigned int irq = msm_rpm_data.irq_ack;
	unsigned long flags;
	uint32_t ctx_mask = msm_rpm_get_ctx_mask(ctx);
	uint32_t ctx_mask_ack = 0;
	uint32_t sel_masks_ack[SEL_MASK_SIZE];
	struct irq_chip *irq_chip, *err_chip;
	int i;

	msm_rpm_request_poll_mode.req = req;
	msm_rpm_request_poll_mode.count = count;
	msm_rpm_request_poll_mode.ctx_mask_ack = &ctx_mask_ack;
	msm_rpm_request_poll_mode.sel_masks_ack = sel_masks_ack;
	msm_rpm_request_poll_mode.done = NULL;

	spin_lock_irqsave(&msm_rpm_irq_lock, flags);
	irq_chip = irq_get_chip(irq);
	if (!irq_chip) {
		spin_unlock_irqrestore(&msm_rpm_irq_lock, flags);
		return -ENOSPC;
	}
	irq_chip->irq_mask(irq_get_irq_data(irq));
	err_chip = irq_get_chip(msm_rpm_data.irq_err);
	if (!err_chip) {
		irq_chip->irq_unmask(irq_get_irq_data(irq));
		spin_unlock_irqrestore(&msm_rpm_irq_lock, flags);
		return -ENOSPC;
	}
	err_chip->irq_mask(irq_get_irq_data(msm_rpm_data.irq_err));

	if (msm_rpm_request) {
		msm_rpm_busy_wait_for_request_completion(true);
		BUG_ON(msm_rpm_request);
	}

	msm_rpm_request = &msm_rpm_request_poll_mode;

	for (i = 0; i < count; i++) {
		BUG_ON(target_enum(req[i].id) >= MSM_RPM_ID_LAST);
		msm_rpm_write(MSM_RPM_PAGE_REQ,
				target_enum(req[i].id), req[i].value);
	}

	msm_rpm_write_contiguous(MSM_RPM_PAGE_CTRL,
		target_ctrl(MSM_RPM_CTRL_REQ_SEL_0),
		sel_masks, msm_rpm_sel_mask_size);
	msm_rpm_write(MSM_RPM_PAGE_CTRL,
		target_ctrl(MSM_RPM_CTRL_REQ_CTX_0), ctx_mask);

	/* Ensure RPM data is written before sending the interrupt */
	mb();
	msm_rpm_send_req_interrupt();

	msm_rpm_busy_wait_for_request_completion(false);
	BUG_ON(msm_rpm_request);

	err_chip->irq_unmask(irq_get_irq_data(msm_rpm_data.irq_err));
	irq_chip->irq_unmask(irq_get_irq_data(irq));
	spin_unlock_irqrestore(&msm_rpm_irq_lock, flags);

	BUG_ON((ctx_mask_ack & ~(msm_rpm_get_ctx_mask(MSM_RPM_CTX_REJECTED)))
		!= ctx_mask);
	BUG_ON(memcmp(sel_masks, sel_masks_ack, sizeof(sel_masks_ack)));

	if (ctx_mask_ack & msm_rpm_get_ctx_mask(MSM_RPM_CTX_REJECTED)) {
		pr_warn("[K] %s: following request is rejected by rpm\n", __func__);
		for (i = 0; i < count; i++)
				pr_warn("[K] %s: id: %d, value: %d\n", __func__, req[i].id, req[i].value);
		return -ENOSPC;
	} else {
		return 0;
	}
}

static int msm_rpm_set_common(
	int ctx, struct msm_rpm_iv_pair *req, int count, bool noirq)
{
	uint32_t sel_masks[SEL_MASK_SIZE] = {};
	int rc;

	if (ctx >= MSM_RPM_CTX_SET_COUNT) {
		rc = -EINVAL;
		goto set_common_exit;
	}

	rc = msm_rpm_fill_sel_masks(sel_masks, req, count);
	if (rc)
		goto set_common_exit;

	if (noirq) {
		unsigned long flags;

		spin_lock_irqsave(&msm_rpm_lock, flags);
		rc = msm_rpm_set_exclusive_noirq(ctx, sel_masks, req, count);
		spin_unlock_irqrestore(&msm_rpm_lock, flags);
	} else {
		mutex_lock(&msm_rpm_mutex);
		rc = msm_rpm_set_exclusive(ctx, sel_masks, req, count);
		mutex_unlock(&msm_rpm_mutex);
	}

set_common_exit:
	return rc;
}

static int msm_rpm_clear_common(
	int ctx, struct msm_rpm_iv_pair *req, int count, bool noirq)
{
	uint32_t sel_masks[SEL_MASK_SIZE] = {};
	struct msm_rpm_iv_pair r[SEL_MASK_SIZE];
	int rc;
	int i;

	if (ctx >= MSM_RPM_CTX_SET_COUNT) {
		rc = -EINVAL;
		goto clear_common_exit;
	}

	rc = msm_rpm_fill_sel_masks(sel_masks, req, count);
	if (rc)
		goto clear_common_exit;

	for (i = 0; i < ARRAY_SIZE(r); i++) {
		r[i].id = MSM_RPM_ID_INVALIDATE_0 + i;
		r[i].value = sel_masks[i];
	}

	memset(sel_masks, 0, sizeof(sel_masks));
	sel_masks[msm_rpm_get_sel_mask_reg(msm_rpm_data.sel_invalidate)] |=
		msm_rpm_get_sel_mask(msm_rpm_data.sel_invalidate);

	if (noirq) {
		unsigned long flags;

		spin_lock_irqsave(&msm_rpm_lock, flags);
		rc = msm_rpm_set_exclusive_noirq(ctx, sel_masks, r,
			ARRAY_SIZE(r));
		spin_unlock_irqrestore(&msm_rpm_lock, flags);
		BUG_ON(rc);
	} else {
		mutex_lock(&msm_rpm_mutex);
		rc = msm_rpm_set_exclusive(ctx, sel_masks, r, ARRAY_SIZE(r));
		mutex_unlock(&msm_rpm_mutex);
		BUG_ON(rc);
	}

clear_common_exit:
	return rc;
}

static void msm_rpm_update_notification(uint32_t ctx,
	struct msm_rpm_notif_config *curr_cfg,
	struct msm_rpm_notif_config *new_cfg)
{
	unsigned int sel_notif = msm_rpm_data.sel_notification;

	if (memcmp(curr_cfg, new_cfg, sizeof(*new_cfg))) {
		uint32_t sel_masks[SEL_MASK_SIZE] = {};
		int rc;

		sel_masks[msm_rpm_get_sel_mask_reg(sel_notif)]
			|= msm_rpm_get_sel_mask(sel_notif);

		rc = msm_rpm_set_exclusive(ctx,
			sel_masks, new_cfg->iv, ARRAY_SIZE(new_cfg->iv));
		BUG_ON(rc);

		memcpy(curr_cfg, new_cfg, sizeof(*new_cfg));
	}
}

static void msm_rpm_initialize_notification(void)
{
	struct msm_rpm_notif_config cfg;
	unsigned int ctx;
	int i;

	for (ctx = MSM_RPM_CTX_SET_0; ctx <= MSM_RPM_CTX_SET_SLEEP; ctx++) {
		cfg = msm_rpm_notif_cfgs[ctx];

		for (i = 0; i < msm_rpm_sel_mask_size; i++) {
			configured_iv(&cfg)[i].id =
				MSM_RPM_ID_NOTIFICATION_CONFIGURED_0 + i;
			configured_iv(&cfg)[i].value = ~0UL;

			registered_iv(&cfg)[i].id =
				MSM_RPM_ID_NOTIFICATION_REGISTERED_0 + i;
			registered_iv(&cfg)[i].value = 0;
		}

		msm_rpm_update_notification(ctx,
			&msm_rpm_notif_cfgs[ctx], &cfg);
	}
}


void msm_rpm_print_sleep_tick(void)
{
	uint32_t *mpm_sleep_tick = (void *) (MSM_RPM_MPM_BASE + 0x24);
	pr_info("MPM_SLEEP_TICK: %llums\n", ((uint64_t)(*mpm_sleep_tick) * 1000) >> 15);
}
EXPORT_SYMBOL(msm_rpm_print_sleep_tick);

int msm_rpm_local_request_is_outstanding(void)
{
	unsigned long flags;
	int outstanding = 0;

	if (!spin_trylock_irqsave(&msm_rpm_lock, flags))
		goto local_request_is_outstanding_exit;

	if (!spin_trylock(&msm_rpm_irq_lock))
		goto local_request_is_outstanding_unlock;

	outstanding = (msm_rpm_request != NULL);
	spin_unlock(&msm_rpm_irq_lock);

local_request_is_outstanding_unlock:
	spin_unlock_irqrestore(&msm_rpm_lock, flags);

local_request_is_outstanding_exit:
	return outstanding;
}

int msm_rpm_get_status(struct msm_rpm_iv_pair *status, int count)
{
	uint32_t seq_begin;
	uint32_t seq_end;
	int rc;
	int i;

	seq_begin = msm_rpm_read(MSM_RPM_PAGE_STATUS,
				target_status(MSM_RPM_STATUS_ID_SEQUENCE));

	for (i = 0; i < count; i++) {
		int target_status_id;

		if (status[i].id >= MSM_RPM_STATUS_ID_LAST) {
			pr_err("%s(): Status ID beyond limits\n", __func__);
			rc = -EINVAL;
			goto get_status_exit;
		}

		target_status_id = target_status(status[i].id);
		if (target_status_id >= MSM_RPM_STATUS_ID_LAST) {
			pr_err("%s(): Status id %d not defined for target\n",
					__func__,
					target_status_id);
			rc = -EINVAL;
			goto get_status_exit;
		}

		status[i].value = msm_rpm_read(MSM_RPM_PAGE_STATUS,
				target_status_id);
	}

	seq_end = msm_rpm_read(MSM_RPM_PAGE_STATUS,
				target_status(MSM_RPM_STATUS_ID_SEQUENCE));

	rc = (seq_begin != seq_end || (seq_begin & 0x01)) ? -EBUSY : 0;

get_status_exit:
	return rc;
}
EXPORT_SYMBOL(msm_rpm_get_status);

int msm_rpm_set(int ctx, struct msm_rpm_iv_pair *req, int count)
{
	return msm_rpm_set_common(ctx, req, count, false);
}
EXPORT_SYMBOL(msm_rpm_set);

int msm_rpm_set_noirq(int ctx, struct msm_rpm_iv_pair *req, int count)
{
	WARN(!irqs_disabled(), "msm_rpm_set_noirq can only be called "
		"safely when local irqs are disabled.  Consider using "
		"msm_rpm_set or msm_rpm_set_nosleep instead.");
	return msm_rpm_set_common(ctx, req, count, true);
}
EXPORT_SYMBOL(msm_rpm_set_noirq);

int msm_rpm_clear(int ctx, struct msm_rpm_iv_pair *req, int count)
{
	return msm_rpm_clear_common(ctx, req, count, false);
}
EXPORT_SYMBOL(msm_rpm_clear);

int msm_rpm_clear_noirq(int ctx, struct msm_rpm_iv_pair *req, int count)
{
	WARN(!irqs_disabled(), "msm_rpm_clear_noirq can only be called "
		"safely when local irqs are disabled.  Consider using "
		"msm_rpm_clear or msm_rpm_clear_nosleep instead.");
	return msm_rpm_clear_common(ctx, req, count, true);
}
EXPORT_SYMBOL(msm_rpm_clear_noirq);

int msm_rpm_register_notification(struct msm_rpm_notification *n,
	struct msm_rpm_iv_pair *req, int count)
{
	unsigned long flags;
	unsigned int ctx;
	struct msm_rpm_notif_config cfg;
	int rc;
	int i;

	INIT_LIST_HEAD(&n->list);
	rc = msm_rpm_fill_sel_masks(n->sel_masks, req, count);
	if (rc)
		goto register_notification_exit;

	mutex_lock(&msm_rpm_mutex);

	if (!msm_rpm_init_notif_done) {
		msm_rpm_initialize_notification();
		msm_rpm_init_notif_done = true;
	}

	spin_lock_irqsave(&msm_rpm_irq_lock, flags);
	list_add(&n->list, &msm_rpm_notifications);
	spin_unlock_irqrestore(&msm_rpm_irq_lock, flags);

	ctx = MSM_RPM_CTX_SET_0;
	cfg = msm_rpm_notif_cfgs[ctx];

	for (i = 0; i < msm_rpm_sel_mask_size; i++)
		registered_iv(&cfg)[i].value |= n->sel_masks[i];

	msm_rpm_update_notification(ctx, &msm_rpm_notif_cfgs[ctx], &cfg);
	mutex_unlock(&msm_rpm_mutex);

register_notification_exit:
	return rc;
}
EXPORT_SYMBOL(msm_rpm_register_notification);

int msm_rpm_unregister_notification(struct msm_rpm_notification *n)
{
	unsigned long flags;
	unsigned int ctx;
	struct msm_rpm_notif_config cfg;
	int rc = 0;
	int i;

	mutex_lock(&msm_rpm_mutex);
	ctx = MSM_RPM_CTX_SET_0;
	cfg = msm_rpm_notif_cfgs[ctx];

	for (i = 0; i < msm_rpm_sel_mask_size; i++)
		registered_iv(&cfg)[i].value = 0;

	spin_lock_irqsave(&msm_rpm_irq_lock, flags);
	list_del(&n->list);
	list_for_each_entry(n, &msm_rpm_notifications, list)
		for (i = 0; i < msm_rpm_sel_mask_size; i++)
			registered_iv(&cfg)[i].value |= n->sel_masks[i];
	spin_unlock_irqrestore(&msm_rpm_irq_lock, flags);

	msm_rpm_update_notification(ctx, &msm_rpm_notif_cfgs[ctx], &cfg);
	mutex_unlock(&msm_rpm_mutex);

	return rc;
}
EXPORT_SYMBOL(msm_rpm_unregister_notification);

static uint32_t fw_major, fw_minor, fw_build;

static ssize_t driver_version_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u.%u.%u\n",
		msm_rpm_data.ver[0], msm_rpm_data.ver[1], msm_rpm_data.ver[2]);
}

static ssize_t fw_version_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u.%u.%u\n",
			fw_major, fw_minor, fw_build);
}

static struct kobj_attribute driver_version_attr = __ATTR_RO(driver_version);
static struct kobj_attribute fw_version_attr = __ATTR_RO(fw_version);

static struct attribute *driver_attributes[] = {
	&driver_version_attr.attr,
	&fw_version_attr.attr,
	NULL
};

static struct attribute_group driver_attr_group = {
	.attrs = driver_attributes,
};

static int __devinit msm_rpm_probe(struct platform_device *pdev)
{
	return sysfs_create_group(&pdev->dev.kobj, &driver_attr_group);
}

static int __devexit msm_rpm_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &driver_attr_group);
	return 0;
}

static struct platform_driver msm_rpm_platform_driver = {
	.probe = msm_rpm_probe,
	.remove = __devexit_p(msm_rpm_remove),
	.driver = {
		.name = "msm_rpm",
		.owner = THIS_MODULE,
	},
};

static void __init msm_rpm_populate_map(struct msm_rpm_platform_data *data)
{
	int i, j;
	struct msm_rpm_map_data *src = NULL;
	struct msm_rpm_map_data *dst = NULL;

	for (i = 0; i < MSM_RPM_ID_LAST;) {
		src = &data->target_id[i];
		dst = &msm_rpm_data.target_id[i];

		dst->id = MSM_RPM_ID_LAST;
		dst->sel = msm_rpm_data.sel_last + 1;

		for (j = 0; j < src->count; j++) {
			dst = &msm_rpm_data.target_id[i + j];
			dst->id = src->id + j;
			dst->sel = src->sel;
		}

		i += (src->count) ? src->count : 1;
	}

	for (i = 0; i < MSM_RPM_STATUS_ID_LAST; i++) {
		if (data->target_status[i] & MSM_RPM_STATUS_ID_VALID)
			msm_rpm_data.target_status[i] &=
				~MSM_RPM_STATUS_ID_VALID;
		else
			msm_rpm_data.target_status[i] = MSM_RPM_STATUS_ID_LAST;
	}
}

static irqreturn_t msm_pm_rpm_wakeup_interrupt(int irq, void *dev_id)
{
	if (dev_id != &msm_pm_rpm_wakeup_interrupt)
		return IRQ_NONE;

	return IRQ_HANDLED;
}

int __init msm_rpm_init(struct msm_rpm_platform_data *data)
{
	int rc;

	memcpy(&msm_rpm_data, data, sizeof(struct msm_rpm_platform_data));
	msm_rpm_stat_data = (stats_blob *)msm_rpm_data.reg_base_addrs[MSM_RPM_PAGE_STAT];
	msm_rpm_sel_mask_size = msm_rpm_data.sel_last / 32 + 1;
	BUG_ON(SEL_MASK_SIZE < msm_rpm_sel_mask_size);

#ifndef CONFIG_ARCH_MSM8X60
	if ((get_radio_flag() & KERNEL_FLAG_APPSBARK) && msm_rpm_stat_data)
		msm_rpm_stat_data->rpm_debug_mode |= RPM_DEBUG_RAM_DUMP;

	if ((get_kernel_flag() & KERNEL_FLAG_PM_MONITOR) && msm_rpm_stat_data)
		msm_rpm_stat_data->rpm_debug_mode |= RPM_DEBUG_POWER_MEASUREMENT;

	
	if ((get_kernel_flag() & KERNEL_FLAG_RPM_DISABLE_WATCHDOG) && msm_rpm_stat_data)
		msm_rpm_stat_data->rpm_debug_mode |= RPM_DEBUG_DISABLE_WATCHDOG;
#endif
	fw_major = msm_rpm_read(MSM_RPM_PAGE_STATUS,
				target_status(MSM_RPM_STATUS_ID_VERSION_MAJOR));
	fw_minor = msm_rpm_read(MSM_RPM_PAGE_STATUS,
				target_status(MSM_RPM_STATUS_ID_VERSION_MINOR));
	fw_build = msm_rpm_read(MSM_RPM_PAGE_STATUS,
				target_status(MSM_RPM_STATUS_ID_VERSION_BUILD));
	pr_info("%s: RPM firmware %u.%u.%u\n", __func__,
			fw_major, fw_minor, fw_build);

	if (fw_major != msm_rpm_data.ver[0]) {
		pr_err("%s: RPM version %u.%u.%u incompatible with "
				"this driver version %u.%u.%u\n", __func__,
				fw_major, fw_minor, fw_build,
				msm_rpm_data.ver[0],
				msm_rpm_data.ver[1],
				msm_rpm_data.ver[2]);
		return -EFAULT;
	}

	msm_rpm_write(MSM_RPM_PAGE_CTRL,
		target_ctrl(MSM_RPM_CTRL_VERSION_MAJOR), msm_rpm_data.ver[0]);
	msm_rpm_write(MSM_RPM_PAGE_CTRL,
		target_ctrl(MSM_RPM_CTRL_VERSION_MINOR), msm_rpm_data.ver[1]);
	msm_rpm_write(MSM_RPM_PAGE_CTRL,
		target_ctrl(MSM_RPM_CTRL_VERSION_BUILD), msm_rpm_data.ver[2]);

	rc = request_irq(data->irq_ack, msm_rpm_ack_interrupt,
			IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND,
			"rpm_drv", msm_rpm_ack_interrupt);
	if (rc) {
		pr_err("%s: failed to request irq %d: %d\n",
			__func__, data->irq_ack, rc);
		return rc;
	}

	rc = irq_set_irq_wake(data->irq_ack, 1);
	if (rc) {
		pr_err("%s: failed to set wakeup irq %u: %d\n",
			__func__, data->irq_ack, rc);
		return rc;
	}

	rc = request_irq(data->irq_err, msm_rpm_err_interrupt,
			IRQF_TRIGGER_RISING, "rpm_err", NULL);
	if (rc) {
		pr_err("%s: failed to request error interrupt: %d\n",
			__func__, rc);
		return rc;
	}

	rc = request_irq(data->irq_wakeup,
			msm_pm_rpm_wakeup_interrupt, IRQF_TRIGGER_RISING,
			"pm_drv", msm_pm_rpm_wakeup_interrupt);
	if (rc) {
		pr_err("%s: failed to request irq %u: %d\n",
			__func__, data->irq_wakeup, rc);
		return rc;
	}

	rc = irq_set_irq_wake(data->irq_wakeup, 1);
	if (rc) {
		pr_err("%s: failed to set wakeup irq %u: %d\n",
			__func__, data->irq_wakeup, rc);
		return rc;
	}

	msm_rpm_populate_map(data);
	msm_rpm_print_sleep_tick();

	return platform_driver_register(&msm_rpm_platform_driver);
}

#ifdef CONFIG_ARCH_MSM8X60
void __init msm_rpm_lpm_init(uint32_t *lpm_setting, uint32_t num)
{
	uint32_t i = 0;
	for (i = 0; i < num; i++)
		msm_rpm_write(MSM_RPM_PAGE_STAT, RPM_LPM_PM8058 + i, lpm_setting[i]);
}
#endif

void msm_rpm_dump_stat(void)
{
	int i = 0;

	if (msm_rpm_stat_data) {
		pr_info("%s: %u, %llums, %u, %llums\n", __func__,
			msm_rpm_stat_data->stats[RPM_STAT_XO_SHUTDOWN_COUNT].value,
			((uint64_t)msm_rpm_stat_data->stats[RPM_STAT_XO_SHUTDOWN_TIME].value * 1000) >> 15,
			msm_rpm_stat_data->stats[RPM_STAT_VDD_MIN_COUNT].value,
			((uint64_t)msm_rpm_stat_data->stats[RPM_STAT_VDD_MIN_TIME].value * 1000) >> 15);
		for (i = 0; i < RPM_MASTER_COUNT; i++) {
#ifdef CONFIG_ARCH_MSM8X60
			pr_info("sleep_info_m.%d - %llums, %llums, %d %d %d %d\n", i, ((uint64_t)msm_rpm_stat_data->wake_info[i].timestamp * 1000) >> 15,
			   ((uint64_t)msm_rpm_stat_data->sleep_info[i].timestamp * 1000) >> 15, msm_rpm_stat_data->sleep_info[i].cxo,
			   msm_rpm_stat_data->sleep_info[i].pxo, msm_rpm_stat_data->sleep_info[i].vdd_mem,
			   msm_rpm_stat_data->sleep_info[i].vdd_dig);
#else
			pr_info("sleep_info_m.%d - %u (%d), %llums, %d %d %d %d\n", i, msm_rpm_stat_data->sleep_info[i].count,
				(msm_rpm_stat_data->sleep_info[i].stats[0] & 0x1), ((uint64_t)msm_rpm_stat_data->sleep_info[i].total_duration * 1000) >> 15,
				((msm_rpm_stat_data->sleep_info[i].stats[0] & 0x2) >> 1), ((msm_rpm_stat_data->sleep_info[i].stats[0] & 0x4) >>2),
				((msm_rpm_stat_data->sleep_info[i].stats[0] & 0xfffffff8) >> 3), msm_rpm_stat_data->sleep_info[i].stats[1]);
#endif
		}
	}
}

void msm_rpm_set_suspend_flag(bool app_from_suspend)
{
	if (msm_rpm_stat_data) {
#ifdef CONFIG_ARCH_MSM8X60
		msm_rpm_stat_data->app_from_suspend = (!!app_from_suspend);
#else
		if (app_from_suspend)
			msm_rpm_stat_data->rpm_debug_mode |= RPM_DEBUG_APP_FROM_SUSPEND;
		else
			msm_rpm_stat_data->rpm_debug_mode &= ~RPM_DEBUG_APP_FROM_SUSPEND;
#endif
	}
}

#ifdef CONFIG_APQ8064_ONLY 
uint32_t htc_dump_vdd_min_time(uint32_t suspend_start, uint32_t resume_start)
{
	pr_info("%s: ", __func__);
	if (!msm_rpm_stat_data)
		return 0;

	pr_info("core0 shutdown time=%d ticks, delta=%d ticks",
		msm_rpm_stat_data->core0_shutdown_time,
		msm_rpm_stat_data->core0_shutdown_time - suspend_start);
	pr_info("vdd min enter time=%d ticks, delta=%d ticks",
		msm_rpm_stat_data->vdd_min_enter_time,
		msm_rpm_stat_data->vdd_min_enter_time - msm_rpm_stat_data->core0_shutdown_time);
	pr_info("rpm sw-done enter time=%d ticks, delta=%d ticks",
		msm_rpm_stat_data->sw_done_enter_time,
		msm_rpm_stat_data->sw_done_enter_time - msm_rpm_stat_data->vdd_min_enter_time);
	pr_info("rpm sw-done exit time=%d ticks, delta=%d ticks",
		msm_rpm_stat_data->sw_done_exit_time,
		msm_rpm_stat_data->sw_done_exit_time - msm_rpm_stat_data->sw_done_enter_time);
	pr_info("vdd min exit time=%d ticks, delta=%d ticks",
		msm_rpm_stat_data->vdd_min_exit_time,
		msm_rpm_stat_data->vdd_min_exit_time - msm_rpm_stat_data->sw_done_exit_time);
	pr_info("core0 bringup time=%d ticks, delta=%d ticks",
		msm_rpm_stat_data->core0_bringup_time,
		msm_rpm_stat_data->core0_bringup_time - msm_rpm_stat_data->vdd_min_exit_time);

	return msm_rpm_stat_data->sw_done_exit_time;
}
#endif
int htc_get_xo_vdd_min_info(uint32_t* xo_count, uint64_t* xo_time, uint32_t* vddmin_count, uint64_t* vddmin_time )
{
	if(!msm_rpm_stat_data)
		return 0;
	*xo_count = msm_rpm_stat_data->stats[RPM_STAT_XO_SHUTDOWN_COUNT].value;
	*xo_time = ((uint64_t)msm_rpm_stat_data->stats[RPM_STAT_XO_SHUTDOWN_TIME].value * 1000) >> 15;
	*vddmin_count = msm_rpm_stat_data->stats[RPM_STAT_VDD_MIN_COUNT].value;
	*vddmin_time = ((uint64_t)msm_rpm_stat_data->stats[RPM_STAT_VDD_MIN_TIME].value * 1000) >> 15;
	return 1;
}

uint64_t msm_rpm_get_xo_time()
{
	if (msm_rpm_stat_data)
		return ((uint64_t)msm_rpm_stat_data->stats[RPM_STAT_XO_SHUTDOWN_TIME].value * 1000000000) >> 15;
	else
		return 0;
}

uint64_t msm_rpm_get_vdd_time()
{
	if (msm_rpm_stat_data)
		return ((uint64_t)msm_rpm_stat_data->stats[RPM_STAT_VDD_MIN_TIME].value * 1000000000) >> 15;
	else
		return 0;
}
