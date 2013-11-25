/*
 * mmap based event notifications for SELinux
 *
 * Author: KaiGai Kohei <kaigai@ak.jp.nec.com>
 *
 * Copyright (C) 2010 NEC corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * as published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include "avc.h"
#include "services.h"

static struct page *selinux_status_page;
static DEFINE_MUTEX(selinux_status_lock);

struct page *selinux_kernel_status_page(void)
{
	struct selinux_kernel_status   *status;
	struct page		       *result = NULL;

	mutex_lock(&selinux_status_lock);
	if (!selinux_status_page) {
		selinux_status_page = alloc_page(GFP_KERNEL|__GFP_ZERO);

		if (selinux_status_page) {
			status = page_address(selinux_status_page);

			status->version = SELINUX_KERNEL_STATUS_VERSION;
			status->sequence = 0;
			status->enforcing = selinux_enforcing;
			status->policyload = 0;
			status->deny_unknown = !security_get_allow_unknown();
		}
	}
	result = selinux_status_page;
	mutex_unlock(&selinux_status_lock);

	return result;
}

void selinux_status_update_setenforce(int enforcing)
{
	struct selinux_kernel_status   *status;

	mutex_lock(&selinux_status_lock);
	if (selinux_status_page) {
		status = page_address(selinux_status_page);

		status->sequence++;
		smp_wmb();

		status->enforcing = enforcing;

		smp_wmb();
		status->sequence++;
	}
	mutex_unlock(&selinux_status_lock);
}

void selinux_status_update_policyload(int seqno)
{
	struct selinux_kernel_status   *status;

	mutex_lock(&selinux_status_lock);
	if (selinux_status_page) {
		status = page_address(selinux_status_page);

		status->sequence++;
		smp_wmb();

		status->policyload = seqno;
		status->deny_unknown = !security_get_allow_unknown();

		smp_wmb();
		status->sequence++;
	}
	mutex_unlock(&selinux_status_lock);
}
