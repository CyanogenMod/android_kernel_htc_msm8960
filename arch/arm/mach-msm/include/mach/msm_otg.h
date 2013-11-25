/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
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

#ifndef __ARCH_ARM_MACH_MSM_OTG_H
#define __ARCH_ARM_MACH_MSM_OTG_H

#include <linux/workqueue.h>
#include <linux/wakelock.h>


struct msm_otg_transceiver {
	struct device		*dev;
	struct clk		*clk;
	struct clk		*pclk;
	int			in_lpm;
	struct msm_otg_ops	*dcd_ops;
	struct msm_otg_ops	*hcd_ops;
	int			irq;
	int			flags;
	int			state;
	int			active;
	void __iomem		*regs;		
	struct work_struct	work;
	spinlock_t		lock;
	struct wake_lock	wlock;

	
	int	(*set_host)(struct msm_otg_transceiver *otg,
				struct msm_otg_ops *hcd_ops);

	
	int	(*set_peripheral)(struct msm_otg_transceiver *otg,
				struct msm_otg_ops *dcd_ops);
	int	(*set_suspend)(struct msm_otg_transceiver *otg,
				int suspend);

};

struct msm_otg_ops {
	void		(*request)(void *, int);
	void		*handle;
};

#ifdef CONFIG_USB_MSM_OTG

extern struct msm_otg_transceiver *msm_otg_get_transceiver(void);
extern void msm_otg_put_transceiver(struct msm_otg_transceiver *xceiv);

#else

static inline struct msm_otg_transceiver *msm_otg_get_transceiver(void)
{
	return NULL;
}

static inline void msm_otg_put_transceiver(struct msm_otg_transceiver *xceiv)
{
}

#endif 

#endif
