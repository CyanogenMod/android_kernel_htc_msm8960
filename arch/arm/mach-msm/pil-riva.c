/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/elf.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>

#include <mach/msm_iomap.h>

#include "peripheral-loader.h"
#include "scm-pas.h"

#define RIVA_PMU_A2XB_CFG		0xB8
#define RIVA_PMU_A2XB_CFG_EN		BIT(0)

#define RIVA_PMU_CFG			0x28
#define RIVA_PMU_CFG_WARM_BOOT		BIT(0)
#define RIVA_PMU_CFG_IRIS_XO_MODE	0x6
#define RIVA_PMU_CFG_IRIS_XO_MODE_48	(3 << 1)

#define RIVA_PMU_OVRD_EN		0x2C
#define RIVA_PMU_OVRD_EN_CCPU_RESET	BIT(0)
#define RIVA_PMU_OVRD_EN_CCPU_CLK	BIT(1)

#define RIVA_PMU_OVRD_VAL		0x30
#define RIVA_PMU_OVRD_VAL_CCPU_RESET	BIT(0)
#define RIVA_PMU_OVRD_VAL_CCPU_CLK	BIT(1)

#define RIVA_PMU_CCPU_CTL		0x9C
#define RIVA_PMU_CCPU_CTL_HIGH_IVT	BIT(0)
#define RIVA_PMU_CCPU_CTL_REMAP_EN	BIT(2)

#define RIVA_PMU_CCPU_BOOT_REMAP_ADDR	0xA0

#define RIVA_PLL_MODE			(MSM_CLK_CTL_BASE + 0x31A0)
#define PLL_MODE_OUTCTRL		BIT(0)
#define PLL_MODE_BYPASSNL		BIT(1)
#define PLL_MODE_RESET_N		BIT(2)
#define PLL_MODE_REF_XO_SEL		0x30
#define PLL_MODE_REF_XO_SEL_CXO		(2 << 4)
#define PLL_MODE_REF_XO_SEL_RF		(3 << 4)
#define RIVA_PLL_L_VAL			(MSM_CLK_CTL_BASE + 0x31A4)
#define RIVA_PLL_M_VAL			(MSM_CLK_CTL_BASE + 0x31A8)
#define RIVA_PLL_N_VAL			(MSM_CLK_CTL_BASE + 0x31Ac)
#define RIVA_PLL_CONFIG			(MSM_CLK_CTL_BASE + 0x31B4)
#define RIVA_PLL_STATUS			(MSM_CLK_CTL_BASE + 0x31B8)
#define RIVA_RESET			    (MSM_CLK_CTL_BASE + 0x35E0)

#define RIVA_PMU_ROOT_CLK_SEL		0xC8
#define RIVA_PMU_ROOT_CLK_SEL_3		BIT(2)

#define RIVA_PMU_CLK_ROOT3			0x78
#define RIVA_PMU_CLK_ROOT3_ENA			BIT(0)
#define RIVA_PMU_CLK_ROOT3_SRC0_DIV		0x3C
#define RIVA_PMU_CLK_ROOT3_SRC0_DIV_2		(1 << 2)
#define RIVA_PMU_CLK_ROOT3_SRC0_SEL		0x1C0
#define RIVA_PMU_CLK_ROOT3_SRC0_SEL_RIVA	(1 << 6)
#define RIVA_PMU_CLK_ROOT3_SRC1_DIV		0x1E00
#define RIVA_PMU_CLK_ROOT3_SRC1_DIV_2		(1 << 9)
#define RIVA_PMU_CLK_ROOT3_SRC1_SEL		0xE000
#define RIVA_PMU_CLK_ROOT3_SRC1_SEL_RIVA	(1 << 13)

#define CLOCK_PLL_WARMUP_TIME_US         60
#define LPASS_CSR_BASE                   0x28000000
#define LCC_REG_BASE                     (LPASS_CSR_BASE      + 0x00000000)
#define HWIO_RIVA_XO_SRC_CLK_CTL_ADDR    (MSM_CLK_CTL_BASE + 0x3880)
#define HWIO_PLL_LOCK_DET_STATUS_ADDR    (MSM_CLK_CTL_BASE + 0x00003420)
#define HWIO_RIVA_PLL_MODE_ADDR          (MSM_CLK_CTL_BASE + 0x000031a0)
#define HWIO_LCC_PLL0_STATUS_ADDR        (LCC_REG_BASE      + 0x00000018)
#define HWIO_LCC_PLL0_STATUS_PLL_ACTIVE_FLAG_BMSK    0x10000
#define HWIO_PLL5_STATUS_ADDR            (MSM_CLK_CTL_BASE + 0x000030f8)
#define HWIO_PLL5_STATUS_PLL_ACTIVE_FLAG_BMSK    0x10000
#define HWIO_RIVA_PLL_STATUS_ADDR        (MSM_CLK_CTL_BASE + 0x000031b8)

#define __outpdw(port, val) (*((volatile u32 *) (port)) = ((u32) (val)))
#define out_dword(addr, val)        __outpdw(addr,val)
#define out_dword_masked_ns(io, mask, val, current_reg_content) \
  out_dword( io, ((current_reg_content & (u32)(~(mask))) | \
                 ((u32)((val) & (mask)))) )
#define __inpdw(port)       (*((volatile u32 *) (port)))
#define in_dword_masked(addr, mask) (__inpdw(addr) & (mask))
#define HWIO_RIVA_RESET_IN          \
        in_dword_masked(RIVA_RESET, 0xffffffff)
#define HWIO_RIVA_XO_SRC_CLK_CTL_IN          \
			in_dword_masked(HWIO_RIVA_XO_SRC_CLK_CTL_ADDR, 0xffffffff)
#define HWIO_RIVA_PLL_MODE_IN          \
        in_dword_masked(HWIO_RIVA_PLL_MODE_ADDR, 0xffffffff)
#define HWIO_RIVA_RESET_OUTM(m,v) \
			out_dword_masked_ns(RIVA_RESET,m,v,HWIO_RIVA_RESET_IN)
#define HWIO_RIVA_XO_SRC_CLK_CTL_OUTM(m,v) \
        out_dword_masked_ns(HWIO_RIVA_XO_SRC_CLK_CTL_ADDR,m,v,HWIO_RIVA_XO_SRC_CLK_CTL_IN)
#define HWIO_RIVA_PLL_MODE_OUTM(m,v) \
        out_dword_masked_ns(HWIO_RIVA_PLL_MODE_ADDR,m,v,HWIO_RIVA_PLL_MODE_IN)

#define APQ8064_HW_VER_2_0   0x2
#define HW_VER_ID_VIRT      (MSM_TLMM_BASE + 0x00002054)

#define TRUE 1
#define FALSE 0

struct riva_data {
	void __iomem *base;
	unsigned long start_addr;
	struct clk *xo;
	struct regulator *pll_supply;
	struct pil_device *pil;
};

enum
{
  CLOCK_SOURCE_XO,    
  CLOCK_SOURCE_WCNXO, 
  CLOCK_SOURCE_PLL4,  
  CLOCK_SOURCE_PLL5,  
  CLOCK_SOURCE_PLL13  
};

#ifdef CONFIG_QUALCOMM_WLAN_PXO
static bool Clock_WaitForPLLActive (u32 nPLL)
{
  u32 nLoops = 100;
  u32 nRegAddr, nRegMask;

  switch(nPLL)
  {
    case CLOCK_SOURCE_PLL4:
      nRegAddr = HWIO_LCC_PLL0_STATUS_ADDR;
      nRegMask = HWIO_LCC_PLL0_STATUS_PLL_ACTIVE_FLAG_BMSK;
      break;
    case CLOCK_SOURCE_PLL5:
      nRegAddr = (u32)HWIO_PLL5_STATUS_ADDR;
      nRegMask = HWIO_PLL5_STATUS_PLL_ACTIVE_FLAG_BMSK;
      break;
    case CLOCK_SOURCE_PLL13:
      nRegAddr = (u32)HWIO_RIVA_PLL_STATUS_ADDR;
      nRegMask = 0x1;
      break;
    default:
      return FALSE;
  }

  while(nLoops > 0)
  {
    if(__inpdw(nRegAddr) & nRegMask)
    {
      return TRUE;
    }

    udelay(1);
    nLoops--;
  }

  return FALSE;

} 
#endif

static bool cxo_is_needed(struct riva_data *drv)
{
	u32 reg = readl_relaxed(drv->base + RIVA_PMU_CFG);
	return (reg & RIVA_PMU_CFG_IRIS_XO_MODE)
		!= RIVA_PMU_CFG_IRIS_XO_MODE_48;
}

static int pil_riva_make_proxy_vote(struct pil_desc *pil)
{
	struct riva_data *drv = dev_get_drvdata(pil->dev);
	int ret;

	ret = regulator_enable(drv->pll_supply);
	if (ret) {
		dev_err(pil->dev, "failed to enable pll supply\n");
		goto err;
	}
	ret = clk_prepare_enable(drv->xo);
	if (ret) {
		dev_err(pil->dev, "failed to enable xo\n");
		goto err_clk;
	}
	return 0;
err_clk:
	regulator_disable(drv->pll_supply);
err:
	return ret;
}

static void pil_riva_remove_proxy_vote(struct pil_desc *pil)
{
	struct riva_data *drv = dev_get_drvdata(pil->dev);
	regulator_disable(drv->pll_supply);
	clk_disable_unprepare(drv->xo);
}

static int pil_riva_init_image(struct pil_desc *pil, const u8 *metadata,
		size_t size)
{
	const struct elf32_hdr *ehdr = (struct elf32_hdr *)metadata;
	struct riva_data *drv = dev_get_drvdata(pil->dev);
	drv->start_addr = ehdr->e_entry;
	return 0;
}

static int pil_riva_reset(struct pil_desc *pil)
{
	u32 reg, sel;
	struct riva_data *drv = dev_get_drvdata(pil->dev);
	void __iomem *base = drv->base;
	unsigned long start_addr = drv->start_addr;
	bool use_cxo = cxo_is_needed(drv);
#ifdef CONFIG_QUALCOMM_WLAN_PXO
	u32 nLoopCount = 25;
	u32 nRetryCount = 5;
	u32 hw_ver_id;
	hw_ver_id = (readl(HW_VER_ID_VIRT)& 0xf0000000) >> 28;

	printk("[WLAN][SSR] Get hw_ver_id = %#x\n", hw_ver_id);
#endif

	
	reg = readl_relaxed(base + RIVA_PMU_A2XB_CFG);
	reg |= RIVA_PMU_A2XB_CFG_EN;
	writel_relaxed(reg, base + RIVA_PMU_A2XB_CFG);

#ifdef CONFIG_QUALCOMM_WLAN_PXO
	while (nRetryCount > 0)
	{
#endif
	
	reg = readl_relaxed(RIVA_PLL_MODE);
	reg &= ~(PLL_MODE_BYPASSNL | PLL_MODE_OUTCTRL | PLL_MODE_RESET_N);
	writel_relaxed(reg, RIVA_PLL_MODE);

	if (use_cxo)
		writel_relaxed(0x40000C00 | 50, RIVA_PLL_L_VAL);
	else
		writel_relaxed(0x40000C00 | 40, RIVA_PLL_L_VAL);
	writel_relaxed(0, RIVA_PLL_M_VAL);
	writel_relaxed(1, RIVA_PLL_N_VAL);
	writel_relaxed(0x01495227, RIVA_PLL_CONFIG);

	reg = readl_relaxed(RIVA_PLL_MODE);
	reg &= ~(PLL_MODE_REF_XO_SEL);
	reg |= use_cxo ? PLL_MODE_REF_XO_SEL_CXO : PLL_MODE_REF_XO_SEL_RF;
	writel_relaxed(reg, RIVA_PLL_MODE);

	
	reg |= PLL_MODE_BYPASSNL;
	writel_relaxed(reg, RIVA_PLL_MODE);

	mb();
	usleep_range(10, 20);

	reg |= PLL_MODE_RESET_N;
	writel_relaxed(reg, RIVA_PLL_MODE);
	reg |= PLL_MODE_OUTCTRL;
	writel_relaxed(reg, RIVA_PLL_MODE);

	
	mb();
	usleep_range(50, 100);

	
#ifdef CONFIG_QUALCOMM_WLAN_PXO
	if (hw_ver_id >= APQ8064_HW_VER_2_0) {
		printk("[WLAN][SSR] Wait for PLL warm-up\n");
		
		while(nLoopCount > 0) 
		{
			udelay(CLOCK_PLL_WARMUP_TIME_US);
			if(in_dword_masked(HWIO_PLL_LOCK_DET_STATUS_ADDR, 0xffffffff) & (1 << 13))
			{break;}
			nLoopCount--;
		}

		if(nLoopCount == 0)
		{
			
			
			printk("[WLAN][SSR] PLL lock detection failed!, retry %d\n", (5-nRetryCount));
                        nRetryCount--;
			nLoopCount = 25;
			continue;
		}
		printk("[WLAN][SSR] Check PLL lock detection passed\n");

		HWIO_RIVA_PLL_MODE_OUTM(0x1, (u32)(1) << (0x0)); 

		if (Clock_WaitForPLLActive(CLOCK_SOURCE_PLL13) == FALSE)
		
		{
			nRetryCount--;
			nLoopCount = 25;
			continue;
		}
		printk("[WLAN][SSR] Wait for PLL Active ...OK!\n");
		break;
	}
#endif
	
#ifdef CONFIG_QUALCOMM_WLAN_PXO
	}
#endif

	
	sel = readl_relaxed(base + RIVA_PMU_ROOT_CLK_SEL);
	reg = readl_relaxed(base + RIVA_PMU_CLK_ROOT3);
	if (sel & RIVA_PMU_ROOT_CLK_SEL_3) {
		reg &= ~(RIVA_PMU_CLK_ROOT3_SRC0_SEL |
			 RIVA_PMU_CLK_ROOT3_SRC0_DIV);
		reg |= RIVA_PMU_CLK_ROOT3_SRC0_SEL_RIVA |
		       RIVA_PMU_CLK_ROOT3_SRC0_DIV_2;
	} else {
		reg &= ~(RIVA_PMU_CLK_ROOT3_SRC1_SEL |
			 RIVA_PMU_CLK_ROOT3_SRC1_DIV);
		reg |= RIVA_PMU_CLK_ROOT3_SRC1_SEL_RIVA |
		       RIVA_PMU_CLK_ROOT3_SRC1_DIV_2;
	}
	writel_relaxed(reg, base + RIVA_PMU_CLK_ROOT3);
	reg |= RIVA_PMU_CLK_ROOT3_ENA;
	writel_relaxed(reg, base + RIVA_PMU_CLK_ROOT3);
	reg = readl_relaxed(base + RIVA_PMU_ROOT_CLK_SEL);
	reg ^= RIVA_PMU_ROOT_CLK_SEL_3;
	writel_relaxed(reg, base + RIVA_PMU_ROOT_CLK_SEL);

	
	reg = readl_relaxed(base + RIVA_PMU_CCPU_CTL);
	reg |= RIVA_PMU_CCPU_CTL_HIGH_IVT | RIVA_PMU_CCPU_CTL_REMAP_EN;
	writel_relaxed(reg, base + RIVA_PMU_CCPU_CTL);

	
	writel_relaxed(start_addr >> 16, base + RIVA_PMU_CCPU_BOOT_REMAP_ADDR);

	
	reg = readl_relaxed(base + RIVA_PMU_CFG);
	reg &= ~(RIVA_PMU_CFG_WARM_BOOT);
	writel_relaxed(reg, base + RIVA_PMU_CFG);

	
	reg = readl_relaxed(base + RIVA_PMU_OVRD_VAL);
	reg |= RIVA_PMU_OVRD_VAL_CCPU_CLK;
	writel_relaxed(reg, base + RIVA_PMU_OVRD_VAL);

	
#ifdef CONFIG_QUALCOMM_WLAN_PXO
	if (hw_ver_id >= APQ8064_HW_VER_2_0) {
		printk("[WLAN][SSR] Use PXO for RIVA\n");
		HWIO_RIVA_RESET_OUTM((0x2),(u32)(1) << (0x1)); 
		HWIO_RIVA_XO_SRC_CLK_CTL_OUTM((0x00000004), (u32)(1) << (0x2)); 
		HWIO_RIVA_RESET_OUTM((0x2),(u32)(0) << (0x1)); 
	}
#endif
	

	
	reg |= RIVA_PMU_OVRD_VAL_CCPU_RESET;
	writel_relaxed(reg, base + RIVA_PMU_OVRD_VAL);

	return 0;
}

static int pil_riva_shutdown(struct pil_desc *pil)
{
	struct riva_data *drv = dev_get_drvdata(pil->dev);
	u32 reg;

	
	reg = readl_relaxed(drv->base + RIVA_PMU_OVRD_VAL);
	reg &= ~(RIVA_PMU_OVRD_VAL_CCPU_RESET | RIVA_PMU_OVRD_VAL_CCPU_CLK);
	writel_relaxed(reg, drv->base + RIVA_PMU_OVRD_VAL);
	reg = readl_relaxed(drv->base + RIVA_PMU_OVRD_EN);
	reg |= RIVA_PMU_OVRD_EN_CCPU_RESET | RIVA_PMU_OVRD_EN_CCPU_CLK;
	writel_relaxed(reg, drv->base + RIVA_PMU_OVRD_EN);
	mb();

	
	writel_relaxed(1, RIVA_RESET);
	mb();
	usleep_range(1000, 2000);

	
	writel_relaxed(0, RIVA_RESET);
	mb();

	return 0;
}

static struct pil_reset_ops pil_riva_ops = {
	.init_image = pil_riva_init_image,
	.auth_and_reset = pil_riva_reset,
	.shutdown = pil_riva_shutdown,
	.proxy_vote = pil_riva_make_proxy_vote,
	.proxy_unvote = pil_riva_remove_proxy_vote,
};

static int pil_riva_init_image_trusted(struct pil_desc *pil,
		const u8 *metadata, size_t size)
{
	return pas_init_image(PAS_RIVA, metadata, size);
}

static int pil_riva_reset_trusted(struct pil_desc *pil)
{
	return pas_auth_and_reset(PAS_RIVA);
}

static int pil_riva_shutdown_trusted(struct pil_desc *pil)
{
	return pas_shutdown(PAS_RIVA);
}

static struct pil_reset_ops pil_riva_ops_trusted = {
	.init_image = pil_riva_init_image_trusted,
	.auth_and_reset = pil_riva_reset_trusted,
	.shutdown = pil_riva_shutdown_trusted,
	.proxy_vote = pil_riva_make_proxy_vote,
	.proxy_unvote = pil_riva_remove_proxy_vote,
};

static int __devinit pil_riva_probe(struct platform_device *pdev)
{
	struct riva_data *drv;
	struct resource *res;
	struct pil_desc *desc;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;

	drv = devm_kzalloc(&pdev->dev, sizeof(*drv), GFP_KERNEL);
	if (!drv)
		return -ENOMEM;
	platform_set_drvdata(pdev, drv);

	drv->base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!drv->base)
		return -ENOMEM;

	desc = devm_kzalloc(&pdev->dev, sizeof(*desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	drv->pll_supply = devm_regulator_get(&pdev->dev, "pll_vdd");
	if (IS_ERR(drv->pll_supply)) {
		dev_err(&pdev->dev, "failed to get pll supply\n");
		return PTR_ERR(drv->pll_supply);
	}
	if (regulator_count_voltages(drv->pll_supply) > 0) {
		ret = regulator_set_voltage(drv->pll_supply, 1800000, 1800000);
		if (ret) {
			dev_err(&pdev->dev,
				"failed to set pll supply voltage\n");
			return ret;
		}

		ret = regulator_set_optimum_mode(drv->pll_supply, 100000);
		if (ret < 0) {
			dev_err(&pdev->dev,
				"failed to set pll supply optimum mode\n");
			return ret;
		}
	}

	desc->name = "wcnss";
	desc->dev = &pdev->dev;
	desc->owner = THIS_MODULE;
	desc->proxy_timeout = 10000;

	if (pas_supported(PAS_RIVA) > 0) {
		desc->ops = &pil_riva_ops_trusted;
		dev_info(&pdev->dev, "using secure boot\n");
	} else {
		desc->ops = &pil_riva_ops;
		dev_info(&pdev->dev, "using non-secure boot\n");
	}

	drv->xo = devm_clk_get(&pdev->dev, "cxo");
	if (IS_ERR(drv->xo))
		return PTR_ERR(drv->xo);

	drv->pil = msm_pil_register(desc);
	if (IS_ERR(drv->pil))
		return PTR_ERR(drv->pil);
	return 0;
}

static int __devexit pil_riva_remove(struct platform_device *pdev)
{
	struct riva_data *drv = platform_get_drvdata(pdev);
	msm_pil_unregister(drv->pil);
	return 0;
}

static struct platform_driver pil_riva_driver = {
	.probe = pil_riva_probe,
	.remove = __devexit_p(pil_riva_remove),
	.driver = {
		.name = "pil_riva",
		.owner = THIS_MODULE,
	},
};

static int __init pil_riva_init(void)
{
	return platform_driver_register(&pil_riva_driver);
}
module_init(pil_riva_init);

static void __exit pil_riva_exit(void)
{
	platform_driver_unregister(&pil_riva_driver);
}
module_exit(pil_riva_exit);

MODULE_DESCRIPTION("Support for booting RIVA (WCNSS) processors");
MODULE_LICENSE("GPL v2");
