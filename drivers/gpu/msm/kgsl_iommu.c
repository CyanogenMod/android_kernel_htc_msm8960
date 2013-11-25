/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
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
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/genalloc.h>
#include <linux/slab.h>
#include <linux/iommu.h>
#include <linux/msm_kgsl.h>
#include <mach/socinfo.h>
#include <mach/msm_iomap.h>
#include <mach/board.h>
#include <stddef.h>

#include "kgsl.h"
#include "kgsl_device.h"
#include "kgsl_mmu.h"
#include "kgsl_sharedmem.h"
#include "kgsl_iommu.h"
#include "adreno_pm4types.h"
#include "adreno.h"
#include "kgsl_trace.h"
#include "z180.h"
#include "kgsl_cffdump.h"


static struct kgsl_iommu_register_list kgsl_iommuv1_reg[KGSL_IOMMU_REG_MAX] = {
	{ 0, 0, 0 },				
	{ 0x10, 0x0003FFFF, 14 },		
	{ 0x14, 0x0003FFFF, 14 },		
	{ 0x20, 0, 0 },				
	{ 0x800, 0, 0 },			
	{ 0x820, 0, 0 },			
	{ 0x03C, 0, 0 },			
	{ 0x818, 0, 0 },			
	{ 0x2C, 0, 0 },                         
	{ 0x2C, 0, 0 },                         
};

static struct kgsl_iommu_register_list kgsl_iommuv2_reg[KGSL_IOMMU_REG_MAX] = {
	{ 0, 0, 0 },				
	{ 0x20, 0x00FFFFFF, 14 },		
	{ 0x28, 0x00FFFFFF, 14 },		
	{ 0x58, 0, 0 },				
	{ 0x618, 0, 0 },			
	{ 0x008, 0, 0 },			
	{ 0, 0, 0 },				
	{ 0, 0, 0 },				
	{ 0x68, 0, 0 },				
	{ 0x6C, 0, 0 }				
};

struct remote_iommu_petersons_spinlock kgsl_iommu_sync_lock_vars;


static struct page *kgsl_guard_page;

static int get_iommu_unit(struct device *dev, struct kgsl_mmu **mmu_out,
			struct kgsl_iommu_unit **iommu_unit_out)
{
	int i, j, k;

	for (i = 0; i < KGSL_DEVICE_MAX; i++) {
		struct kgsl_mmu *mmu;
		struct kgsl_iommu *iommu;

		if (kgsl_driver.devp[i] == NULL)
			continue;

		mmu = kgsl_get_mmu(kgsl_driver.devp[i]);
		if (mmu == NULL || mmu->priv == NULL)
			continue;

		iommu = mmu->priv;

		for (j = 0; j < iommu->unit_count; j++) {
			struct kgsl_iommu_unit *iommu_unit =
				&iommu->iommu_units[j];
			for (k = 0; k < iommu_unit->dev_count; k++) {
				if (iommu_unit->dev[k].dev == dev) {
					*mmu_out = mmu;
					*iommu_unit_out = iommu_unit;
					return 0;
				}
			}
		}
	}

	return -EINVAL;
}

static struct kgsl_iommu_device *get_iommu_device(struct kgsl_iommu_unit *unit,
		struct device *dev)
{
	int k;

	for (k = 0; unit && k < unit->dev_count; k++) {
		if (unit->dev[k].dev == dev)
			return &(unit->dev[k]);
	}

	return NULL;
}



struct _mem_entry {
	unsigned int gpuaddr;
	unsigned int size;
	unsigned int flags;
	unsigned int priv;
	pid_t pid;
};


static void _prev_entry(struct kgsl_process_private *priv,
	unsigned int faultaddr, struct _mem_entry *ret)
{
	struct rb_node *node;
	struct kgsl_mem_entry *entry;

	for (node = rb_first(&priv->mem_rb); node; ) {
		entry = rb_entry(node, struct kgsl_mem_entry, node);

		if (entry->memdesc.gpuaddr > faultaddr)
			break;


		if (entry->memdesc.gpuaddr > ret->gpuaddr) {
			ret->gpuaddr = entry->memdesc.gpuaddr;
			ret->size = entry->memdesc.size;
			ret->flags = entry->memdesc.flags;
			ret->priv = entry->memdesc.priv;
			ret->pid = priv->pid;
		}

		node = rb_next(&entry->node);
	}
}


static void _next_entry(struct kgsl_process_private *priv,
	unsigned int faultaddr, struct _mem_entry *ret)
{
	struct rb_node *node;
	struct kgsl_mem_entry *entry;

	for (node = rb_last(&priv->mem_rb); node; ) {
		entry = rb_entry(node, struct kgsl_mem_entry, node);

		if (entry->memdesc.gpuaddr < faultaddr)
			break;


		if (entry->memdesc.gpuaddr < ret->gpuaddr) {
			ret->gpuaddr = entry->memdesc.gpuaddr;
			ret->size = entry->memdesc.size;
			ret->flags = entry->memdesc.flags;
			ret->priv = entry->memdesc.priv;
			ret->pid = priv->pid;
		}

		node = rb_prev(&entry->node);
	}
}

static void _find_mem_entries(struct kgsl_mmu *mmu, unsigned int faultaddr,
	unsigned int ptbase, struct _mem_entry *preventry,
	struct _mem_entry *nextentry)
{
	struct kgsl_process_private *private;
	int id = kgsl_mmu_get_ptname_from_ptbase(mmu, ptbase);

	memset(preventry, 0, sizeof(*preventry));
	memset(nextentry, 0, sizeof(*nextentry));

	
	nextentry->gpuaddr = 0xFFFFFFFF;

	mutex_lock(&kgsl_driver.process_mutex);

	list_for_each_entry(private, &kgsl_driver.process_list, list) {

		if (private->pagetable->name != id)
			continue;

		spin_lock(&private->mem_lock);
		_prev_entry(private, faultaddr, preventry);
		_next_entry(private, faultaddr, nextentry);
		spin_unlock(&private->mem_lock);
	}

	mutex_unlock(&kgsl_driver.process_mutex);
}

static void _print_entry(struct kgsl_device *device, struct _mem_entry *entry)
{
	char name[32];
	memset(name, 0, sizeof(name));

	kgsl_get_memory_usage(name, sizeof(name) - 1, entry->flags);

	KGSL_LOG_DUMP(device,
		"[%8.8X - %8.8X] %s (pid = %d) (%s)\n",
		entry->gpuaddr,
		entry->gpuaddr + entry->size,
		entry->priv & KGSL_MEMDESC_GUARD_PAGE ? "(+guard)" : "",
		entry->pid, name);
}

static void _check_if_freed(struct kgsl_iommu_device *iommu_dev,
	unsigned long addr, unsigned int pid)
{
	void *base = kgsl_driver.memfree_hist.base_hist_rb;
	struct kgsl_memfree_hist_elem *wptr;
	struct kgsl_memfree_hist_elem *p;

	mutex_lock(&kgsl_driver.memfree_hist_mutex);
	wptr = kgsl_driver.memfree_hist.wptr;
	p = wptr;
	for (;;) {
		if (p->size && p->pid == pid)
			if (addr >= p->gpuaddr &&
				addr < (p->gpuaddr + p->size)) {

				KGSL_LOG_DUMP(iommu_dev->kgsldev,
					"---- premature free ----\n");
				KGSL_LOG_DUMP(iommu_dev->kgsldev,
					"[%8.8X-%8.8X] was already freed by pid %d\n",
					p->gpuaddr,
					p->gpuaddr + p->size,
					p->pid);
			}
		p++;
		if ((void *)p >= base + kgsl_driver.memfree_hist.size)
			p = (struct kgsl_memfree_hist_elem *) base;

		if (p == kgsl_driver.memfree_hist.wptr)
			break;
	}
	mutex_unlock(&kgsl_driver.memfree_hist_mutex);
}

static int kgsl_iommu_fault_handler(struct iommu_domain *domain,
	struct device *dev, unsigned long addr, int flags)
{
	int ret = 0;
	struct kgsl_mmu *mmu;
	struct kgsl_iommu *iommu;
	struct kgsl_iommu_unit *iommu_unit;
	struct kgsl_iommu_device *iommu_dev;
	unsigned int ptbase, fsr;
	struct kgsl_device *device;
	struct adreno_device *adreno_dev;
	unsigned int no_page_fault_log = 0;
	unsigned int curr_context_id = 0;
	unsigned int curr_global_ts = 0;
	static struct kgsl_context *context;
	unsigned int pid;
	unsigned int fsynr0, fsynr1;
	int write;
	struct _mem_entry prev, next;

	ret = get_iommu_unit(dev, &mmu, &iommu_unit);
	if (ret)
		goto done;
	iommu_dev = get_iommu_device(iommu_unit, dev);
	if (!iommu_dev) {
		KGSL_CORE_ERR("Invalid IOMMU device %p\n", dev);
		ret = -ENOSYS;
		goto done;
	}
	iommu = mmu->priv;
	device = mmu->device;
	adreno_dev = ADRENO_DEVICE(device);

	ptbase = KGSL_IOMMU_GET_CTX_REG(iommu, iommu_unit,
					iommu_dev->ctx_id, TTBR0);

	fsr = KGSL_IOMMU_GET_CTX_REG(iommu, iommu_unit,
		iommu_dev->ctx_id, FSR);
	fsynr0 = KGSL_IOMMU_GET_CTX_REG(iommu, iommu_unit,
		iommu_dev->ctx_id, FSYNR0);
	fsynr1 = KGSL_IOMMU_GET_CTX_REG(iommu, iommu_unit,
		iommu_dev->ctx_id, FSYNR1);

	if (!msm_soc_version_supports_iommu_v1())
		write = ((fsynr1 & (KGSL_IOMMU_FSYNR1_AWRITE_MASK <<
			KGSL_IOMMU_FSYNR1_AWRITE_SHIFT)) ? 1 : 0);
	else
		write = ((fsynr0 & (KGSL_IOMMU_V1_FSYNR0_WNR_MASK <<
			KGSL_IOMMU_V1_FSYNR0_WNR_SHIFT)) ? 1 : 0);

	if (adreno_dev->ft_pf_policy & KGSL_FT_PAGEFAULT_LOG_ONE_PER_PAGE)
		no_page_fault_log = kgsl_mmu_log_fault_addr(mmu, ptbase, addr);

	pid = kgsl_mmu_get_ptname_from_ptbase(mmu, ptbase);
	if (!no_page_fault_log) {
		KGSL_MEM_CRIT(iommu_dev->kgsldev,
			"GPU PAGE FAULT: addr = %lX pid = %d\n", addr, pid);
		KGSL_MEM_CRIT(iommu_dev->kgsldev,
		"context = %d FSR = %X FSYNR0 = %X FSYNR1 = %X(%s fault)\n",
			iommu_dev->ctx_id, fsr, fsynr0, fsynr1,
			write ? "write" : "read");
	}

	_check_if_freed(iommu_dev, addr, pid);

	KGSL_LOG_DUMP(iommu_dev->kgsldev, "---- nearby memory ----\n");

	_find_mem_entries(mmu, addr, ptbase, &prev, &next);

	if (prev.gpuaddr)
		_print_entry(iommu_dev->kgsldev, &prev);
	else
		KGSL_LOG_DUMP(iommu_dev->kgsldev, "*EMPTY*\n");

	KGSL_LOG_DUMP(iommu_dev->kgsldev, " <- fault @ %8.8lX\n", addr);

	if (next.gpuaddr != 0xFFFFFFFF)
		_print_entry(iommu_dev->kgsldev, &next);
	else
		KGSL_LOG_DUMP(iommu_dev->kgsldev, "*EMPTY*\n");

	mmu->fault = 1;
	iommu_dev->fault = 1;

	kgsl_sharedmem_readl(&device->memstore, &curr_context_id,
		KGSL_MEMSTORE_OFFSET(KGSL_MEMSTORE_GLOBAL, current_context));
	context = kgsl_context_get(device, curr_context_id);

	if (context != NULL && (context->devctxt != NULL)) {
		struct adreno_context *drawctxt = context->devctxt;

		ret = kgsl_sharedmem_readl(&device->memstore, &curr_global_ts,
				KGSL_MEMSTORE_OFFSET(KGSL_MEMSTORE_GLOBAL, eoptimestamp));

		if (ret < 0) {
			KGSL_CORE_ERR("Invalid curr_global_ts = %d\n", curr_global_ts);
			goto done;
		}


		if (drawctxt != NULL) {
			drawctxt->pagefault = 1;
			drawctxt->pagefault_ts = curr_global_ts;
		}

		kgsl_context_put(context);
	}


	trace_kgsl_mmu_pagefault(iommu_dev->kgsldev, addr,
			kgsl_mmu_get_ptname_from_ptbase(mmu, ptbase),
				write ? "write" : "read");

	if (adreno_dev->ft_pf_policy & KGSL_FT_PAGEFAULT_GPUHALT_ENABLE)
		ret = -EBUSY;
done:
	return ret;
}

static void kgsl_iommu_disable_clk(struct kgsl_mmu *mmu)
{
	struct kgsl_iommu *iommu = mmu->priv;
	struct msm_iommu_drvdata *iommu_drvdata;
	int i, j;

	for (i = 0; i < iommu->unit_count; i++) {
		struct kgsl_iommu_unit *iommu_unit = &iommu->iommu_units[i];
		for (j = 0; j < iommu_unit->dev_count; j++) {
			if (!iommu_unit->dev[j].clk_enabled)
				continue;
			iommu_drvdata = dev_get_drvdata(
					iommu_unit->dev[j].dev->parent);
			if (iommu_drvdata->clk)
				clk_disable_unprepare(iommu_drvdata->clk);
			clk_disable_unprepare(iommu_drvdata->pclk);
			iommu_unit->dev[j].clk_enabled = false;
		}
	}
}

static void kgsl_iommu_clk_disable_event(struct kgsl_device *device, void *data,
					unsigned int id, unsigned int ts,
					u32 type)
{
	struct kgsl_mmu *mmu = data;
	struct kgsl_iommu *iommu = mmu->priv;

	if (!iommu->clk_event_queued) {
		if (0 > timestamp_cmp(ts, iommu->iommu_last_cmd_ts))
			KGSL_DRV_ERR(device,
			"IOMMU disable clock event being cancelled, "
			"iommu_last_cmd_ts: %x, retired ts: %x\n",
			iommu->iommu_last_cmd_ts, ts);
		return;
	}

	if (0 <= timestamp_cmp(ts, iommu->iommu_last_cmd_ts)) {
		kgsl_iommu_disable_clk(mmu);
		iommu->clk_event_queued = false;
	} else {
		if (kgsl_add_event(device, id, iommu->iommu_last_cmd_ts,
			kgsl_iommu_clk_disable_event, mmu, mmu)) {
				KGSL_DRV_ERR(device,
				"Failed to add IOMMU disable clk event\n");
				iommu->clk_event_queued = false;
		}
	}
}

static void
kgsl_iommu_disable_clk_on_ts(struct kgsl_mmu *mmu, unsigned int ts,
				bool ts_valid)
{
	struct kgsl_iommu *iommu = mmu->priv;

	if (iommu->clk_event_queued) {
		if (ts_valid && (0 <
			timestamp_cmp(ts, iommu->iommu_last_cmd_ts)))
			iommu->iommu_last_cmd_ts = ts;
	} else {
		if (ts_valid) {
			iommu->iommu_last_cmd_ts = ts;
			iommu->clk_event_queued = true;
			if (kgsl_add_event(mmu->device, KGSL_MEMSTORE_GLOBAL,
				ts, kgsl_iommu_clk_disable_event, mmu, mmu)) {
				KGSL_DRV_ERR(mmu->device,
				"Failed to add IOMMU disable clk event\n");
				iommu->clk_event_queued = false;
			}
		} else {
			kgsl_iommu_disable_clk(mmu);
		}
	}
}

static int kgsl_iommu_enable_clk(struct kgsl_mmu *mmu,
				int ctx_id)
{
	int ret = 0;
	int i, j;
	struct kgsl_iommu *iommu = mmu->priv;
	struct msm_iommu_drvdata *iommu_drvdata;

	for (i = 0; i < iommu->unit_count; i++) {
		struct kgsl_iommu_unit *iommu_unit = &iommu->iommu_units[i];
		for (j = 0; j < iommu_unit->dev_count; j++) {
			if (iommu_unit->dev[j].clk_enabled ||
				ctx_id != iommu_unit->dev[j].ctx_id)
				continue;
			iommu_drvdata =
			dev_get_drvdata(iommu_unit->dev[j].dev->parent);
			ret = clk_prepare_enable(iommu_drvdata->pclk);
			if (ret)
				goto done;
			if (iommu_drvdata->clk) {
				ret = clk_prepare_enable(iommu_drvdata->clk);
				if (ret) {
					clk_disable_unprepare(
						iommu_drvdata->pclk);
					goto done;
				}
			}
#if 0
			if (iommu_drvdata->aclk) {
				ret = clk_prepare_enable(iommu_drvdata->aclk);
				if (ret) {
					if (iommu_drvdata->clk)
						clk_disable_unprepare(
							iommu_drvdata->clk);
					clk_disable_unprepare(
							iommu_drvdata->pclk);
					goto done;
				}
			}
#endif
			iommu_unit->dev[j].clk_enabled = true;
		}
	}
done:
	if (ret)
		kgsl_iommu_disable_clk(mmu);
	return ret;
}

static int kgsl_iommu_pt_equal(struct kgsl_mmu *mmu,
				struct kgsl_pagetable *pt,
				unsigned int pt_base)
{
	struct kgsl_iommu *iommu = mmu->priv;
	struct kgsl_iommu_pt *iommu_pt = pt ? pt->priv : NULL;
	unsigned int domain_ptbase = iommu_pt ?
				iommu_get_pt_base_addr(iommu_pt->domain) : 0;
	
	domain_ptbase &=
		(iommu->iommu_reg_list[KGSL_IOMMU_CTX_TTBR0].reg_mask <<
		iommu->iommu_reg_list[KGSL_IOMMU_CTX_TTBR0].reg_shift);

	pt_base &=
		(iommu->iommu_reg_list[KGSL_IOMMU_CTX_TTBR0].reg_mask <<
		iommu->iommu_reg_list[KGSL_IOMMU_CTX_TTBR0].reg_shift);

	return domain_ptbase && pt_base &&
		(domain_ptbase == pt_base);
}

static void kgsl_iommu_destroy_pagetable(void *mmu_specific_pt)
{
	struct kgsl_iommu_pt *iommu_pt = mmu_specific_pt;
	if (iommu_pt->domain)
		iommu_domain_free(iommu_pt->domain);
	kfree(iommu_pt);
}

void *kgsl_iommu_create_pagetable(void)
{
	struct kgsl_iommu_pt *iommu_pt;

	iommu_pt = kzalloc(sizeof(struct kgsl_iommu_pt), GFP_KERNEL);
	if (!iommu_pt) {
		KGSL_CORE_ERR("kzalloc(%d) failed\n",
				sizeof(struct kgsl_iommu_pt));
		return NULL;
	}
	
	if (msm_soc_version_supports_iommu_v1())
		iommu_pt->domain = iommu_domain_alloc(&platform_bus_type,
					MSM_IOMMU_DOMAIN_PT_CACHEABLE);
	else
		iommu_pt->domain = iommu_domain_alloc(&platform_bus_type,
					0);
	if (!iommu_pt->domain) {
		KGSL_CORE_ERR("Failed to create iommu domain\n");
		kfree(iommu_pt);
		return NULL;
	} else {
		iommu_set_fault_handler(iommu_pt->domain,
			kgsl_iommu_fault_handler);
	}

	return iommu_pt;
}

static void kgsl_detach_pagetable_iommu_domain(struct kgsl_mmu *mmu)
{
	struct kgsl_iommu_pt *iommu_pt;
	struct kgsl_iommu *iommu = mmu->priv;
	int i, j;

	for (i = 0; i < iommu->unit_count; i++) {
		struct kgsl_iommu_unit *iommu_unit = &iommu->iommu_units[i];
		iommu_pt = mmu->defaultpagetable->priv;
		for (j = 0; j < iommu_unit->dev_count; j++) {
			if (mmu->priv_bank_table &&
				(KGSL_IOMMU_CONTEXT_PRIV == j))
				iommu_pt = mmu->priv_bank_table->priv;
			if (iommu_unit->dev[j].attached) {
				iommu_detach_device(iommu_pt->domain,
						iommu_unit->dev[j].dev);
				iommu_unit->dev[j].attached = false;
				KGSL_MEM_INFO(mmu->device, "iommu %p detached "
					"from user dev of MMU: %p\n",
					iommu_pt->domain, mmu);
			}
		}
	}
}

static int kgsl_attach_pagetable_iommu_domain(struct kgsl_mmu *mmu)
{
	struct kgsl_iommu_pt *iommu_pt;
	struct kgsl_iommu *iommu = mmu->priv;
	int i, j, ret = 0;

	for (i = 0; i < iommu->unit_count; i++) {
		struct kgsl_iommu_unit *iommu_unit = &iommu->iommu_units[i];
		iommu_pt = mmu->defaultpagetable->priv;
		for (j = 0; j < iommu_unit->dev_count; j++) {
			if (mmu->priv_bank_table &&
				(KGSL_IOMMU_CONTEXT_PRIV == j))
				iommu_pt = mmu->priv_bank_table->priv;
			if (!iommu_unit->dev[j].attached) {
				ret = iommu_attach_device(iommu_pt->domain,
							iommu_unit->dev[j].dev);
				if (ret) {
					KGSL_MEM_ERR(mmu->device,
						"Failed to attach device, err %d\n",
						ret);
					goto done;
				}
				iommu_unit->dev[j].attached = true;
				KGSL_MEM_INFO(mmu->device,
				"iommu pt %p attached to dev %p, ctx_id %d\n",
				iommu_pt->domain, iommu_unit->dev[j].dev,
				iommu_unit->dev[j].ctx_id);
			}
		}
	}
done:
	return ret;
}

static int _get_iommu_ctxs(struct kgsl_mmu *mmu,
	struct kgsl_device_iommu_data *data, unsigned int unit_id)
{
	struct kgsl_iommu *iommu = mmu->priv;
	struct kgsl_iommu_unit *iommu_unit = &iommu->iommu_units[unit_id];
	int i;

	if (data->iommu_ctx_count > KGSL_IOMMU_MAX_DEVS_PER_UNIT) {
		KGSL_CORE_ERR("Too many iommu devices defined for an "
				"IOMMU unit\n");
		return -EINVAL;
	}

	for (i = 0; i < data->iommu_ctx_count; i++) {
		if (!data->iommu_ctxs[i].iommu_ctx_name)
			continue;

		iommu_unit->dev[iommu_unit->dev_count].dev =
			msm_iommu_get_ctx(data->iommu_ctxs[i].iommu_ctx_name);
		if (iommu_unit->dev[iommu_unit->dev_count].dev == NULL) {
			KGSL_CORE_ERR("Failed to get iommu dev handle for "
			"device %s\n", data->iommu_ctxs[i].iommu_ctx_name);
			return -EINVAL;
		}
		if (KGSL_IOMMU_CONTEXT_USER != data->iommu_ctxs[i].ctx_id &&
			KGSL_IOMMU_CONTEXT_PRIV != data->iommu_ctxs[i].ctx_id) {
			KGSL_CORE_ERR("Invalid context ID defined: %d\n",
					data->iommu_ctxs[i].ctx_id);
			return -EINVAL;
		}
		iommu_unit->dev[iommu_unit->dev_count].ctx_id =
						data->iommu_ctxs[i].ctx_id;
		iommu_unit->dev[iommu_unit->dev_count].kgsldev = mmu->device;

		KGSL_DRV_INFO(mmu->device,
				"Obtained dev handle %p for iommu context %s\n",
				iommu_unit->dev[iommu_unit->dev_count].dev,
				data->iommu_ctxs[i].iommu_ctx_name);

		iommu_unit->dev_count++;
	}

	return 0;
}
static int kgsl_iommu_start_sync_lock(struct kgsl_mmu *mmu)
{
	struct kgsl_iommu *iommu = mmu->priv;
	uint32_t lock_gpu_addr = 0;

	if (KGSL_DEVICE_3D0 != mmu->device->id ||
		!msm_soc_version_supports_iommu_v1() ||
		!kgsl_mmu_is_perprocess() ||
		iommu->sync_lock_vars)
		return 0;

	if (!(mmu->flags & KGSL_MMU_FLAGS_IOMMU_SYNC)) {
		KGSL_DRV_ERR(mmu->device,
		"The GPU microcode does not support IOMMUv1 sync opcodes\n");
		return -ENXIO;
	}
	
	lock_gpu_addr = (iommu->sync_lock_desc.gpuaddr +
			iommu->sync_lock_offset);

	kgsl_iommu_sync_lock_vars.flag[PROC_APPS] = (lock_gpu_addr +
		(offsetof(struct remote_iommu_petersons_spinlock,
			flag[PROC_APPS])));
	kgsl_iommu_sync_lock_vars.flag[PROC_GPU] = (lock_gpu_addr +
		(offsetof(struct remote_iommu_petersons_spinlock,
			flag[PROC_GPU])));
	kgsl_iommu_sync_lock_vars.turn = (lock_gpu_addr +
		(offsetof(struct remote_iommu_petersons_spinlock, turn)));

	iommu->sync_lock_vars = &kgsl_iommu_sync_lock_vars;

	return 0;
}

static int kgsl_iommu_init_sync_lock(struct kgsl_mmu *mmu)
{
	struct kgsl_iommu *iommu = mmu->priv;
	int status = 0;
	uint32_t lock_phy_addr = 0;
	uint32_t page_offset = 0;

	if (KGSL_DEVICE_3D0 != mmu->device->id ||
		!msm_soc_version_supports_iommu_v1() ||
		!kgsl_mmu_is_perprocess())
		return status;

	if (KGSL_DEVICE_2D0 == mmu->device->id ||
		KGSL_DEVICE_2D1 == mmu->device->id) {
		return status;
	}

	
	if (iommu->sync_lock_initialized)
		return status;

	
	lock_phy_addr = (msm_iommu_lock_initialize()
			- MSM_SHARED_RAM_BASE + msm_shared_ram_phys);

	if (!lock_phy_addr) {
		KGSL_DRV_ERR(mmu->device,
				"GPU CPU sync lock is not supported by kernel\n");
		return -ENXIO;
	}

	
	page_offset = (lock_phy_addr & (PAGE_SIZE - 1));
	lock_phy_addr = (lock_phy_addr & ~(PAGE_SIZE - 1));
	iommu->sync_lock_desc.physaddr = (unsigned int)lock_phy_addr;
	iommu->sync_lock_offset = page_offset;

	iommu->sync_lock_desc.size =
				PAGE_ALIGN(sizeof(kgsl_iommu_sync_lock_vars));
	status =  memdesc_sg_phys(&iommu->sync_lock_desc,
				 iommu->sync_lock_desc.physaddr,
				 iommu->sync_lock_desc.size);

	if (status)
		return status;


	
	iommu->sync_lock_initialized = 1;

	return status;
}

inline unsigned int kgsl_iommu_sync_lock(struct kgsl_mmu *mmu,
						unsigned int *cmds)
{
	struct kgsl_device *device = mmu->device;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct kgsl_iommu *iommu = mmu->device->mmu.priv;
	struct remote_iommu_petersons_spinlock *lock_vars =
					iommu->sync_lock_vars;
	unsigned int *start = cmds;

	if (!iommu->sync_lock_initialized)
		return 0;

	*cmds++ = cp_type3_packet(CP_MEM_WRITE, 2);
	*cmds++ = lock_vars->flag[PROC_GPU];
	*cmds++ = 1;

	cmds += adreno_add_idle_cmds(adreno_dev, cmds);

	*cmds++ = cp_type3_packet(CP_WAIT_REG_MEM, 5);
	
	*cmds++ = 0x13;
	*cmds++ = lock_vars->flag[PROC_GPU];
	*cmds++ = 0x1;
	*cmds++ = 0x1;
	*cmds++ = 0x1;

	*cmds++ = cp_type3_packet(CP_MEM_WRITE, 2);
	*cmds++ = lock_vars->turn;
	*cmds++ = 0;

	cmds += adreno_add_idle_cmds(adreno_dev, cmds);

	*cmds++ = cp_type3_packet(CP_WAIT_REG_MEM, 5);
	
	*cmds++ = 0x13;
	*cmds++ = lock_vars->flag[PROC_GPU];
	*cmds++ = 0x1;
	*cmds++ = 0x1;
	*cmds++ = 0x1;

	*cmds++ = cp_type3_packet(CP_TEST_TWO_MEMS, 3);
	*cmds++ = lock_vars->flag[PROC_APPS];
	*cmds++ = lock_vars->turn;
	*cmds++ = 0;

	cmds += adreno_add_idle_cmds(adreno_dev, cmds);

	return cmds - start;
}

inline unsigned int kgsl_iommu_sync_unlock(struct kgsl_mmu *mmu,
					unsigned int *cmds)
{
	struct kgsl_device *device = mmu->device;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct kgsl_iommu *iommu = mmu->device->mmu.priv;
	struct remote_iommu_petersons_spinlock *lock_vars =
						iommu->sync_lock_vars;
	unsigned int *start = cmds;

	if (!iommu->sync_lock_initialized)
		return 0;

	*cmds++ = cp_type3_packet(CP_MEM_WRITE, 2);
	*cmds++ = lock_vars->flag[PROC_GPU];
	*cmds++ = 0;

	*cmds++ = cp_type3_packet(CP_WAIT_REG_MEM, 5);
	
	*cmds++ = 0x13;
	*cmds++ = lock_vars->flag[PROC_GPU];
	*cmds++ = 0x0;
	*cmds++ = 0x1;
	*cmds++ = 0x1;

	cmds += adreno_add_idle_cmds(adreno_dev, cmds);

	return cmds - start;
}

static int kgsl_get_iommu_ctxt(struct kgsl_mmu *mmu)
{
	struct platform_device *pdev =
		container_of(mmu->device->parentdev, struct platform_device,
				dev);
	struct kgsl_device_platform_data *pdata_dev = pdev->dev.platform_data;
	struct kgsl_iommu *iommu = mmu->device->mmu.priv;
	int i, ret = 0;

	
	if (KGSL_IOMMU_MAX_UNITS < pdata_dev->iommu_count) {
		KGSL_CORE_ERR("Too many IOMMU units defined\n");
		ret = -EINVAL;
		goto  done;
	}

	for (i = 0; i < pdata_dev->iommu_count; i++) {
		ret = _get_iommu_ctxs(mmu, &pdata_dev->iommu_data[i], i);
		if (ret)
			break;
	}
	iommu->unit_count = pdata_dev->iommu_count;
done:
	return ret;
}

static int kgsl_set_register_map(struct kgsl_mmu *mmu)
{
	struct platform_device *pdev =
		container_of(mmu->device->parentdev, struct platform_device,
				dev);
	struct kgsl_device_platform_data *pdata_dev = pdev->dev.platform_data;
	struct kgsl_iommu *iommu = mmu->device->mmu.priv;
	struct kgsl_iommu_unit *iommu_unit;
	int i = 0, ret = 0;

	for (; i < pdata_dev->iommu_count; i++) {
		struct kgsl_device_iommu_data data = pdata_dev->iommu_data[i];
		iommu_unit = &iommu->iommu_units[i];
		
		if (!data.physstart || !data.physend) {
			KGSL_CORE_ERR("The register range for IOMMU unit not"
					" specified\n");
			ret = -EINVAL;
			goto err;
		}
		iommu_unit->reg_map.hostptr = ioremap(data.physstart,
					data.physend - data.physstart + 1);
		if (!iommu_unit->reg_map.hostptr) {
			KGSL_CORE_ERR("Failed to map SMMU register address "
				"space from %x to %x\n", data.physstart,
				data.physend - data.physstart + 1);
			ret = -ENOMEM;
			i--;
			goto err;
		}
		iommu_unit->reg_map.size = data.physend - data.physstart + 1;
		iommu_unit->reg_map.physaddr = data.physstart;
		ret = memdesc_sg_phys(&iommu_unit->reg_map, data.physstart,
				iommu_unit->reg_map.size);
		if (ret)
			goto err;
	}
	iommu->unit_count = pdata_dev->iommu_count;
	return ret;
err:
	
	for (; i >= 0; i--) {
		iommu_unit = &iommu->iommu_units[i];
		iounmap(iommu_unit->reg_map.hostptr);
		iommu_unit->reg_map.size = 0;
		iommu_unit->reg_map.physaddr = 0;
	}
	return ret;
}

static unsigned int kgsl_iommu_get_pt_base_addr(struct kgsl_mmu *mmu,
						struct kgsl_pagetable *pt)
{
	struct kgsl_iommu *iommu = mmu->priv;
	struct kgsl_iommu_pt *iommu_pt = pt->priv;
	return iommu_get_pt_base_addr(iommu_pt->domain) &
			(iommu->iommu_reg_list[KGSL_IOMMU_CTX_TTBR0].reg_mask <<
			iommu->iommu_reg_list[KGSL_IOMMU_CTX_TTBR0].reg_shift);
}

static int kgsl_iommu_get_pt_lsb(struct kgsl_mmu *mmu,
				unsigned int unit_id,
				enum kgsl_iommu_context_id ctx_id)
{
	struct kgsl_iommu *iommu = mmu->priv;
	int i, j;
	for (i = 0; i < iommu->unit_count; i++) {
		struct kgsl_iommu_unit *iommu_unit = &iommu->iommu_units[i];
		for (j = 0; j < iommu_unit->dev_count; j++)
			if (unit_id == i &&
				ctx_id == iommu_unit->dev[j].ctx_id)
				return iommu_unit->dev[j].pt_lsb;
	}
	return 0;
}

static void kgsl_iommu_setstate(struct kgsl_mmu *mmu,
				struct kgsl_pagetable *pagetable,
				unsigned int context_id)
{
	if (mmu->flags & KGSL_FLAGS_STARTED) {
		if (mmu->hwpagetable != pagetable) {
			unsigned int flags = 0;
			mmu->hwpagetable = pagetable;
			flags |= kgsl_mmu_pt_get_flags(mmu->hwpagetable,
							mmu->device->id) |
							KGSL_MMUFLAGS_TLBFLUSH;
			kgsl_setstate(mmu, context_id,
				KGSL_MMUFLAGS_PTUPDATE | flags);
		}
	}
}

static int kgsl_iommu_setup_regs(struct kgsl_mmu *mmu,
				    struct kgsl_pagetable *pt)
{
	int status;
	int i = 0;
	struct kgsl_iommu *iommu = mmu->priv;

	if (!msm_soc_version_supports_iommu_v1())
		return 0;

	for (i = 0; i < iommu->unit_count; i++) {
		status = kgsl_mmu_map_global(pt,
				&(iommu->iommu_units[i].reg_map));
		if (status)
			goto err;
	}

	
	if (iommu->sync_lock_initialized) {
		status = kgsl_mmu_map_global(pt,
				&iommu->sync_lock_desc);
		if (status)
			goto err;
	}

	return 0;
err:
	for (i--; i >= 0; i--)
		kgsl_mmu_unmap(pt,
				&(iommu->iommu_units[i].reg_map));

	return status;
}

static void kgsl_iommu_cleanup_regs(struct kgsl_mmu *mmu,
					struct kgsl_pagetable *pt)
{
	struct kgsl_iommu *iommu = mmu->priv;
	int i;
	for (i = 0; i < iommu->unit_count; i++)
		kgsl_mmu_unmap(pt, &(iommu->iommu_units[i].reg_map));

	if (iommu->sync_lock_desc.gpuaddr)
		kgsl_mmu_unmap(pt, &iommu->sync_lock_desc);
}


static int kgsl_iommu_init(struct kgsl_mmu *mmu)
{
	int status = 0;
	struct kgsl_iommu *iommu;

	iommu = kzalloc(sizeof(struct kgsl_iommu), GFP_KERNEL);
	if (!iommu) {
		KGSL_CORE_ERR("kzalloc(%d) failed\n",
				sizeof(struct kgsl_iommu));
		return -ENOMEM;
	}

	mmu->priv = iommu;
	status = kgsl_get_iommu_ctxt(mmu);
	if (status)
		goto done;
	status = kgsl_set_register_map(mmu);
	if (status)
		goto done;

	status = kgsl_iommu_init_sync_lock(mmu);
	if (status)
		goto done;

	iommu->iommu_reg_list = kgsl_iommuv1_reg;
	iommu->ctx_offset = KGSL_IOMMU_CTX_OFFSET_V1;

	if (msm_soc_version_supports_iommu_v1()) {
		iommu->iommu_reg_list = kgsl_iommuv1_reg;
		iommu->ctx_offset = KGSL_IOMMU_CTX_OFFSET_V1;
	} else {
		iommu->iommu_reg_list = kgsl_iommuv2_reg;
		iommu->ctx_offset = KGSL_IOMMU_CTX_OFFSET_V2;
	}

	kgsl_sharedmem_writel(&mmu->setstate_memory,
				KGSL_IOMMU_SETSTATE_NOP_OFFSET,
				cp_nop_packet(1));

	if (cpu_is_msm8960()) {
		iommu_ops.mmu_setup_pt = kgsl_iommu_setup_regs;
		iommu_ops.mmu_cleanup_pt = kgsl_iommu_cleanup_regs;
	}

	if (kgsl_guard_page == NULL) {
		kgsl_guard_page = alloc_page(GFP_KERNEL | __GFP_ZERO |
				__GFP_HIGHMEM);
		if (kgsl_guard_page == NULL) {
			status = -ENOMEM;
			goto done;
		}
	}

	dev_info(mmu->device->dev, "|%s| MMU type set for device is IOMMU\n",
			__func__);
done:
	if (status) {
		kfree(iommu);
		mmu->priv = NULL;
	}
	return status;
}

static int kgsl_iommu_setup_defaultpagetable(struct kgsl_mmu *mmu)
{
	int status = 0;

	if (!cpu_is_msm8960() && msm_soc_version_supports_iommu_v1()) {
		mmu->priv_bank_table =
			kgsl_mmu_getpagetable(KGSL_MMU_PRIV_BANK_TABLE_NAME);
		if (mmu->priv_bank_table == NULL) {
			status = -ENOMEM;
			goto err;
		}
		status = kgsl_iommu_setup_regs(mmu, mmu->priv_bank_table);
		if (status)
			goto err;
	}
	mmu->defaultpagetable = kgsl_mmu_getpagetable(KGSL_MMU_GLOBAL_PT);
	
	if (mmu->defaultpagetable == NULL) {
		status = -ENOMEM;
		goto err;
	}
	return status;
err:
	if (mmu->priv_bank_table) {
		kgsl_iommu_cleanup_regs(mmu, mmu->priv_bank_table);
		kgsl_mmu_putpagetable(mmu->priv_bank_table);
		mmu->priv_bank_table = NULL;
	}
	if (mmu->defaultpagetable) {
		kgsl_mmu_putpagetable(mmu->defaultpagetable);
		mmu->defaultpagetable = NULL;
	}
	return status;
}

static void kgsl_iommu_lock_rb_in_tlb(struct kgsl_mmu *mmu)
{
	struct kgsl_device *device = mmu->device;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct adreno_ringbuffer *rb;
	struct kgsl_iommu *iommu = mmu->priv;
	unsigned int num_tlb_entries;
	unsigned int tlblkcr = 0;
	unsigned int v2pxx = 0;
	unsigned int vaddr = 0;
	int i, j, k, l;

	if (!iommu->sync_lock_initialized)
		return;

	rb = &adreno_dev->ringbuffer;
	num_tlb_entries = rb->buffer_desc.size / PAGE_SIZE;

	for (i = 0; i < iommu->unit_count; i++) {
		struct kgsl_iommu_unit *iommu_unit = &iommu->iommu_units[i];
		for (j = 0; j < iommu_unit->dev_count; j++) {
			tlblkcr = 0;
			if (cpu_is_msm8960())
				tlblkcr |= ((num_tlb_entries &
					KGSL_IOMMU_TLBLKCR_FLOOR_MASK) <<
					KGSL_IOMMU_TLBLKCR_FLOOR_SHIFT);
			else
				tlblkcr |= (((num_tlb_entries *
					iommu_unit->dev_count) &
					KGSL_IOMMU_TLBLKCR_FLOOR_MASK) <<
					KGSL_IOMMU_TLBLKCR_FLOOR_SHIFT);
			
			tlblkcr	|= ((1 & KGSL_IOMMU_TLBLKCR_TLBIALLCFG_MASK)
				<< KGSL_IOMMU_TLBLKCR_TLBIALLCFG_SHIFT);
			tlblkcr	|= ((1 & KGSL_IOMMU_TLBLKCR_TLBIASIDCFG_MASK)
				<< KGSL_IOMMU_TLBLKCR_TLBIASIDCFG_SHIFT);
			tlblkcr	|= ((1 & KGSL_IOMMU_TLBLKCR_TLBIVAACFG_MASK)
				<< KGSL_IOMMU_TLBLKCR_TLBIVAACFG_SHIFT);
			
			tlblkcr |= ((1 & KGSL_IOMMU_TLBLKCR_LKE_MASK)
				<< KGSL_IOMMU_TLBLKCR_LKE_SHIFT);
			KGSL_IOMMU_SET_CTX_REG(iommu, iommu_unit,
					iommu_unit->dev[j].ctx_id,
					TLBLKCR, tlblkcr);
		}
		for (j = 0; j < iommu_unit->dev_count; j++) {
			
			if (cpu_is_msm8960() &&  KGSL_IOMMU_CONTEXT_PRIV == j)
				continue;
			
			vaddr = rb->buffer_desc.gpuaddr;
			for (k = 0; k < num_tlb_entries; k++) {
				v2pxx = 0;
				v2pxx |= (((k + j * num_tlb_entries) &
					KGSL_IOMMU_V2PXX_INDEX_MASK)
					<< KGSL_IOMMU_V2PXX_INDEX_SHIFT);
				v2pxx |= vaddr & (KGSL_IOMMU_V2PXX_VA_MASK <<
						KGSL_IOMMU_V2PXX_VA_SHIFT);

				KGSL_IOMMU_SET_CTX_REG(iommu, iommu_unit,
						iommu_unit->dev[j].ctx_id,
						V2PUR, v2pxx);
				mb();
				vaddr += PAGE_SIZE;
				for (l = 0; l < iommu_unit->dev_count; l++) {
					tlblkcr = KGSL_IOMMU_GET_CTX_REG(iommu,
						iommu_unit,
						iommu_unit->dev[l].ctx_id,
						TLBLKCR);
					mb();
					tlblkcr &=
					~(KGSL_IOMMU_TLBLKCR_VICTIM_MASK
					<< KGSL_IOMMU_TLBLKCR_VICTIM_SHIFT);
					tlblkcr |= (((k + 1 +
					(j * num_tlb_entries)) &
					KGSL_IOMMU_TLBLKCR_VICTIM_MASK) <<
					KGSL_IOMMU_TLBLKCR_VICTIM_SHIFT);
					KGSL_IOMMU_SET_CTX_REG(iommu,
						iommu_unit,
						iommu_unit->dev[l].ctx_id,
						TLBLKCR, tlblkcr);
				}
			}
		}
		for (j = 0; j < iommu_unit->dev_count; j++) {
			tlblkcr = KGSL_IOMMU_GET_CTX_REG(iommu, iommu_unit,
						iommu_unit->dev[j].ctx_id,
						TLBLKCR);
			mb();
			
			tlblkcr &= ~(KGSL_IOMMU_TLBLKCR_LKE_MASK
				<< KGSL_IOMMU_TLBLKCR_LKE_SHIFT);
			KGSL_IOMMU_SET_CTX_REG(iommu, iommu_unit,
				iommu_unit->dev[j].ctx_id, TLBLKCR, tlblkcr);
		}
	}
}

static int kgsl_iommu_start(struct kgsl_mmu *mmu)
{
	int status;
	struct kgsl_iommu *iommu = mmu->priv;
	int i, j;

	if (mmu->flags & KGSL_FLAGS_STARTED)
		return 0;

	if (mmu->defaultpagetable == NULL) {
		status = kgsl_iommu_setup_defaultpagetable(mmu);
		if (status)
			return -ENOMEM;
	}

	status = kgsl_iommu_start_sync_lock(mmu);
	if (status)
		return status;

	if (cpu_is_msm8960() && KGSL_DEVICE_3D0 == mmu->device->id) {
		struct kgsl_mh *mh = &(mmu->device->mh);
		BUG_ON(iommu->iommu_units[0].reg_map.gpuaddr != 0 &&
			mh->mpu_base > iommu->iommu_units[0].reg_map.gpuaddr);
		kgsl_regwrite(mmu->device, MH_MMU_CONFIG, 0x00000001);

		kgsl_regwrite(mmu->device, MH_MMU_MPU_END,
			mh->mpu_base + mh->mpu_range);
	}

	mmu->hwpagetable = mmu->defaultpagetable;

	status = kgsl_attach_pagetable_iommu_domain(mmu);
	if (status) {
		mmu->hwpagetable = NULL;
		goto done;
	}
	status = kgsl_iommu_enable_clk(mmu, KGSL_IOMMU_CONTEXT_USER);
	if (status) {
		KGSL_CORE_ERR("clk enable failed\n");
		goto done;
	}
	status = kgsl_iommu_enable_clk(mmu, KGSL_IOMMU_CONTEXT_PRIV);
	if (status) {
		KGSL_CORE_ERR("clk enable failed\n");
		goto done;
	}
	msm_iommu_lock();
	for (i = 0; i < iommu->unit_count; i++) {
		struct kgsl_iommu_unit *iommu_unit = &iommu->iommu_units[i];
		for (j = 0; j < iommu_unit->dev_count; j++) {
			iommu_unit->dev[j].pt_lsb = KGSL_IOMMMU_PT_LSB(iommu,
						KGSL_IOMMU_GET_CTX_REG(iommu,
						iommu_unit,
						iommu_unit->dev[j].ctx_id,
						TTBR0));
		}
	}
	kgsl_iommu_lock_rb_in_tlb(mmu);
	msm_iommu_unlock();

	
	kgsl_cffdump_setmem(mmu->setstate_memory.gpuaddr +
				KGSL_IOMMU_SETSTATE_NOP_OFFSET,
				cp_nop_packet(1), sizeof(unsigned int));

	kgsl_iommu_disable_clk_on_ts(mmu, 0, false);
	mmu->flags |= KGSL_FLAGS_STARTED;

done:
	if (status) {
		kgsl_iommu_disable_clk_on_ts(mmu, 0, false);
		kgsl_detach_pagetable_iommu_domain(mmu);
	}
	return status;
}

static int
kgsl_iommu_unmap(void *mmu_specific_pt,
		struct kgsl_memdesc *memdesc,
		unsigned int *tlb_flags)
{
	int ret;
	unsigned int range = memdesc->size;
	struct kgsl_iommu_pt *iommu_pt = mmu_specific_pt;


	unsigned int gpuaddr = memdesc->gpuaddr &  KGSL_MMU_ALIGN_MASK;

	if (range == 0 || gpuaddr == 0)
		return 0;

	if (kgsl_memdesc_has_guard_page(memdesc))
		range += PAGE_SIZE;

	ret = iommu_unmap_range(iommu_pt->domain, gpuaddr, range);
	if (ret)
		KGSL_CORE_ERR("iommu_unmap_range(%p, %x, %d) failed "
			"with err: %d\n", iommu_pt->domain, gpuaddr,
			range, ret);

	if (!ret && kgsl_mmu_is_perprocess())
		*tlb_flags = UINT_MAX;
	return 0;
}

static int
kgsl_iommu_map(void *mmu_specific_pt,
			struct kgsl_memdesc *memdesc,
			unsigned int protflags,
			unsigned int *tlb_flags)
{
	int ret;
	unsigned int iommu_virt_addr;
	struct kgsl_iommu_pt *iommu_pt = mmu_specific_pt;
	int size = memdesc->size;

	BUG_ON(NULL == iommu_pt);

	iommu_virt_addr = memdesc->gpuaddr;

	ret = iommu_map_range(iommu_pt->domain, iommu_virt_addr, memdesc->sg,
				size, protflags);
	if (ret) {
		KGSL_CORE_ERR("iommu_map_range(%p, %x, %p, %d, %x) err: %d\n",
			iommu_pt->domain, iommu_virt_addr, memdesc->sg, size,
			protflags, ret);
		return ret;
	}
	if (kgsl_memdesc_has_guard_page(memdesc)) {
		ret = iommu_map(iommu_pt->domain, iommu_virt_addr + size,
				page_to_phys(kgsl_guard_page), PAGE_SIZE,
				protflags & ~IOMMU_WRITE);
		if (ret) {
			KGSL_CORE_ERR("iommu_map(%p, %x, %x, %x) err: %d\n",
				iommu_pt->domain, iommu_virt_addr + size,
				page_to_phys(kgsl_guard_page),
				protflags & ~IOMMU_WRITE,
				ret);
			
			iommu_unmap_range(iommu_pt->domain, iommu_virt_addr,
					  size);
		}
	}
	return ret;
}

static void kgsl_iommu_stop(struct kgsl_mmu *mmu)
{
	struct kgsl_iommu *iommu = mmu->priv;
	int i, j;
	if (mmu->flags & KGSL_FLAGS_STARTED) {
		
		kgsl_detach_pagetable_iommu_domain(mmu);
		mmu->hwpagetable = NULL;

		mmu->flags &= ~KGSL_FLAGS_STARTED;

		if (mmu->fault) {
			for (i = 0; i < iommu->unit_count; i++) {
				struct kgsl_iommu_unit *iommu_unit =
					&iommu->iommu_units[i];
				for (j = 0; j < iommu_unit->dev_count; j++) {
					if (iommu_unit->dev[j].fault) {
						kgsl_iommu_enable_clk(mmu, j);
						msm_iommu_lock();
						KGSL_IOMMU_SET_CTX_REG(iommu,
						iommu_unit,
						iommu_unit->dev[j].ctx_id,
						RESUME, 1);
						msm_iommu_unlock();
						iommu_unit->dev[j].fault = 0;
					}
				}
			}
			mmu->fault = 0;
		}
	}
	
	iommu->clk_event_queued = false;
	kgsl_cancel_events(mmu->device, mmu);
	kgsl_iommu_disable_clk(mmu);
}

static int kgsl_iommu_close(struct kgsl_mmu *mmu)
{
	struct kgsl_iommu *iommu = mmu->priv;
	int i;

	if (mmu->priv_bank_table != NULL) {
		kgsl_iommu_cleanup_regs(mmu, mmu->priv_bank_table);
		kgsl_mmu_putpagetable(mmu->priv_bank_table);
	}

	if (mmu->defaultpagetable != NULL)
		kgsl_mmu_putpagetable(mmu->defaultpagetable);

	for (i = 0; i < iommu->unit_count; i++) {
		struct kgsl_memdesc *reg_map = &iommu->iommu_units[i].reg_map;

		if (reg_map->hostptr)
			iounmap(reg_map->hostptr);
		kgsl_sg_free(reg_map->sg, reg_map->sglen);
		reg_map->priv &= ~KGSL_MEMDESC_GLOBAL;
	}
	
	kgsl_sg_free(iommu->sync_lock_desc.sg, iommu->sync_lock_desc.sglen);
	memset(&iommu->sync_lock_desc, 0, sizeof(iommu->sync_lock_desc));
	iommu->sync_lock_vars = NULL;

	kfree(iommu);

	if (kgsl_guard_page != NULL) {
		__free_page(kgsl_guard_page);
		kgsl_guard_page = NULL;
	}

	return 0;
}

static unsigned int
kgsl_iommu_get_current_ptbase(struct kgsl_mmu *mmu)
{
	unsigned int pt_base;
	struct kgsl_iommu *iommu = mmu->priv;
	if (in_interrupt())
		return 0;
	
	kgsl_iommu_enable_clk(mmu, KGSL_IOMMU_CONTEXT_USER);
	pt_base = KGSL_IOMMU_GET_CTX_REG(iommu, (&iommu->iommu_units[0]),
					KGSL_IOMMU_CONTEXT_USER,
					TTBR0);
	kgsl_iommu_disable_clk_on_ts(mmu, 0, false);
	return pt_base &
		(iommu->iommu_reg_list[KGSL_IOMMU_CTX_TTBR0].reg_mask <<
		iommu->iommu_reg_list[KGSL_IOMMU_CTX_TTBR0].reg_shift);
}

static void kgsl_iommu_default_setstate(struct kgsl_mmu *mmu,
					uint32_t flags)
{
	struct kgsl_iommu *iommu = mmu->priv;
	int temp;
	int i;
	unsigned int pt_base = kgsl_iommu_get_pt_base_addr(mmu,
						mmu->hwpagetable);
	unsigned int pt_val;

	if (kgsl_iommu_enable_clk(mmu, KGSL_IOMMU_CONTEXT_USER)) {
		KGSL_DRV_ERR(mmu->device, "Failed to enable iommu clocks\n");
		return;
	}
	
	pt_base &= (iommu->iommu_reg_list[KGSL_IOMMU_CTX_TTBR0].reg_mask <<
			iommu->iommu_reg_list[KGSL_IOMMU_CTX_TTBR0].reg_shift);

	
	if (msm_soc_version_supports_iommu_v1())
		kgsl_idle(mmu->device);

	
	msm_iommu_lock();

	if (flags & KGSL_MMUFLAGS_PTUPDATE) {
		if (!msm_soc_version_supports_iommu_v1())
			kgsl_idle(mmu->device);
		for (i = 0; i < iommu->unit_count; i++) {
			pt_val = kgsl_iommu_get_pt_lsb(mmu, i,
						KGSL_IOMMU_CONTEXT_USER);
			pt_val += pt_base;

			KGSL_IOMMU_SET_CTX_REG(iommu, (&iommu->iommu_units[i]),
				KGSL_IOMMU_CONTEXT_USER, TTBR0, pt_val);

			mb();
			temp = KGSL_IOMMU_GET_CTX_REG(iommu,
				(&iommu->iommu_units[i]),
				KGSL_IOMMU_CONTEXT_USER, TTBR0);
		}
	}
	
	if (flags & KGSL_MMUFLAGS_TLBFLUSH) {
		for (i = 0; i < iommu->unit_count; i++) {
			KGSL_IOMMU_SET_CTX_REG(iommu, (&iommu->iommu_units[i]),
				KGSL_IOMMU_CONTEXT_USER, TLBIALL, 1);
			mb();
		}
	}

	
	msm_iommu_unlock();

	
	kgsl_iommu_disable_clk_on_ts(mmu, 0, false);
}

static unsigned int kgsl_iommu_get_reg_gpuaddr(struct kgsl_mmu *mmu,
					int iommu_unit, int ctx_id, int reg)
{
	struct kgsl_iommu *iommu = mmu->priv;

	if (KGSL_IOMMU_GLOBAL_BASE == reg)
		return iommu->iommu_units[iommu_unit].reg_map.gpuaddr;
	else
		return iommu->iommu_units[iommu_unit].reg_map.gpuaddr +
			iommu->iommu_reg_list[reg].reg_offset +
			(ctx_id << KGSL_IOMMU_CTX_SHIFT) + iommu->ctx_offset;
}

static int kgsl_iommu_get_num_iommu_units(struct kgsl_mmu *mmu)
{
	struct kgsl_iommu *iommu = mmu->priv;
	return iommu->unit_count;
}

struct kgsl_mmu_ops iommu_ops = {
	.mmu_init = kgsl_iommu_init,
	.mmu_close = kgsl_iommu_close,
	.mmu_start = kgsl_iommu_start,
	.mmu_stop = kgsl_iommu_stop,
	.mmu_setstate = kgsl_iommu_setstate,
	.mmu_device_setstate = kgsl_iommu_default_setstate,
	.mmu_pagefault = NULL,
	.mmu_get_current_ptbase = kgsl_iommu_get_current_ptbase,
	.mmu_enable_clk = kgsl_iommu_enable_clk,
	.mmu_disable_clk_on_ts = kgsl_iommu_disable_clk_on_ts,
	.mmu_get_pt_lsb = kgsl_iommu_get_pt_lsb,
	.mmu_get_reg_gpuaddr = kgsl_iommu_get_reg_gpuaddr,
	.mmu_get_num_iommu_units = kgsl_iommu_get_num_iommu_units,
	.mmu_pt_equal = kgsl_iommu_pt_equal,
	.mmu_get_pt_base_addr = kgsl_iommu_get_pt_base_addr,
	.mmu_sync_lock = kgsl_iommu_sync_lock,
	.mmu_sync_unlock = kgsl_iommu_sync_unlock,
	
	.mmu_setup_pt = NULL,
	.mmu_cleanup_pt = NULL,
};

struct kgsl_mmu_pt_ops iommu_pt_ops = {
	.mmu_map = kgsl_iommu_map,
	.mmu_unmap = kgsl_iommu_unmap,
	.mmu_create_pagetable = kgsl_iommu_create_pagetable,
	.mmu_destroy_pagetable = kgsl_iommu_destroy_pagetable,
};
