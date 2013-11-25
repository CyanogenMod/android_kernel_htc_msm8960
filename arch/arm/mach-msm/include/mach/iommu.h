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
 */

#ifndef MSM_IOMMU_H
#define MSM_IOMMU_H

#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <mach/socinfo.h>

extern pgprot_t     pgprot_kernel;
extern struct platform_device *msm_iommu_root_dev;

#define MSM_IOMMU_DOMAIN_PT_CACHEABLE	0x1

#define MSM_IOMMU_CP_MASK		0x03

#define MAX_NUM_MIDS	32

struct msm_iommu_dev {
	const char *name;
	int ncb;
	int ttbr_split;
};

struct msm_iommu_ctx_dev {
	const char *name;
	int num;
	int mids[MAX_NUM_MIDS];
};


struct msm_iommu_drvdata {
	void __iomem *base;
	int ncb;
	int ttbr_split;
	struct clk *clk;
	struct clk *pclk;
	const char *name;
	struct regulator *gdsc;
};

struct msm_iommu_ctx_drvdata {
	int num;
	struct platform_device *pdev;
	struct list_head attached_elm;
	struct iommu_domain *attached_domain;
	const char *name;
};

irqreturn_t msm_iommu_fault_handler(int irq, void *dev_id);
irqreturn_t msm_iommu_fault_handler_v2(int irq, void *dev_id);

enum {
	PROC_APPS,
	PROC_GPU,
	PROC_MAX
};

struct remote_iommu_petersons_spinlock {
	uint32_t flag[PROC_MAX];
	uint32_t turn;
};

#ifdef CONFIG_MSM_IOMMU
void *msm_iommu_lock_initialize(void);
void msm_iommu_mutex_lock(void);
void msm_iommu_mutex_unlock(void);
#else
static inline void *msm_iommu_lock_initialize(void)
{
	return NULL;
}
static inline void msm_iommu_mutex_lock(void) { }
static inline void msm_iommu_mutex_unlock(void) { }
#endif

#ifdef CONFIG_MSM_IOMMU_GPU_SYNC
void msm_iommu_remote_p0_spin_lock(void);
void msm_iommu_remote_p0_spin_unlock(void);

#define msm_iommu_remote_lock_init() _msm_iommu_remote_spin_lock_init()
#define msm_iommu_remote_spin_lock() msm_iommu_remote_p0_spin_lock()
#define msm_iommu_remote_spin_unlock() msm_iommu_remote_p0_spin_unlock()
#else
#define msm_iommu_remote_lock_init()
#define msm_iommu_remote_spin_lock()
#define msm_iommu_remote_spin_unlock()
#endif

#define msm_iommu_lock() \
	do { \
		msm_iommu_mutex_lock(); \
		msm_iommu_remote_spin_lock(); \
	} while (0)

#define msm_iommu_unlock() \
	do { \
		msm_iommu_remote_spin_unlock(); \
		msm_iommu_mutex_unlock(); \
	} while (0)

#ifdef CONFIG_MSM_IOMMU
struct device *msm_iommu_get_ctx(const char *ctx_name);
#else
static inline struct device *msm_iommu_get_ctx(const char *ctx_name)
{
	return NULL;
}
#endif


static inline int msm_soc_version_supports_iommu_v1(void)
{
#ifdef CONFIG_OF
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "qcom,msm-smmu-v2");
	if (node) {
		of_node_put(node);
		return 0;
	}
#endif
	if (cpu_is_msm8960() &&
	    SOCINFO_VERSION_MAJOR(socinfo_get_version()) < 2)
		return 0;

	if (cpu_is_msm8x60() &&
	    (SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 2 ||
	    SOCINFO_VERSION_MINOR(socinfo_get_version()) < 1))	{
		return 0;
	}
	return 1;
}
#endif
