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

#include "../../../../drivers/video/msm/msm_fb.h"
#include "../../../../drivers/video/msm/mipi_dsi.h"
#include "../../../../drivers/video/msm/mdp.h"
#include "../../../../drivers/video/msm/mdp4.h"
#include "../../../../drivers/video/msm/msm_fb_panel.h"
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <mach/panel_id.h>
#include <mach/msm_memtypes.h>
#include <linux/bootmem.h>
#include <video/msm_hdmi_modes.h>
#include "../devices.h"
#include "../board-fighter.h"
#if defined (CONFIG_FB_MSM_MDP_ABL)
#include <linux/fb.h>
#endif

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#define MSM_FB_PRIM_BUF_SIZE (960 * 544 * 4 * 3) /* 4 bpp x 3 pages */
#else
#define MSM_FB_PRIM_BUF_SIZE (960 * 544 * 4 * 2) /* 4 bpp x 2 pages */
#endif


#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
#define MSM_FB_EXT_BUF_SIZE (1920 * 1088 * 2 * 1) /* 2 bpp x 1 page */
#elif defined(CONFIG_FB_MSM_TVOUT)
#define MSM_FB_EXT_BUF_SIZE (720 * 576 * 2 * 2) /* 2 bpp x 2 pages */
#else
#define MSM_FB_EXT_BUF_SIZE 0
#endif

/* Note: must be multiple of 4096 */
#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE + MSM_FB_EXT_BUF_SIZE, 4096)

#ifdef CONFIG_FB_MSM_OVERLAY0_WRITEBACK
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((960 * 544 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY0_WRITEBACK */

#ifdef CONFIG_FB_MSM_OVERLAY1_WRITEBACK
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE roundup((1920 * 1088 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY1_WRITEBACK */

static struct resource msm_fb_resources[] = {
	{
		.flags = IORESOURCE_DMA,
	}
};

static struct msm_fb_platform_data msm_fb_pdata;

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

static int isOrise(void)
{
  return (panel_type == PANEL_ID_FIGHTER_SONY_OTM || panel_type == PANEL_ID_FIGHTER_SONY_OTM_C1_1 || panel_type == PANEL_ID_FIGHTER_SONY_OTM_MP);
}

static void __init msm8960_set_display_params(char *prim_panel, char *ext_panel)
{
	if (strnlen(prim_panel, PANEL_NAME_MAX_LEN)) {
		strlcpy(msm_fb_pdata.prim_panel_name, prim_panel,
			PANEL_NAME_MAX_LEN);
		pr_debug("msm_fb_pdata.prim_panel_name %s\n",
			msm_fb_pdata.prim_panel_name);
	}
	if (strnlen(ext_panel, PANEL_NAME_MAX_LEN)) {
		strlcpy(msm_fb_pdata.ext_panel_name, ext_panel,
			PANEL_NAME_MAX_LEN);
		pr_debug("msm_fb_pdata.ext_panel_name %s\n",
			msm_fb_pdata.ext_panel_name);
	}
}

int fighter_panel_first_init = 1;
static bool dsi_power_on;

static int mipi_dsi_panel_power(int on)
{
	static struct regulator *v_lcm, *v_lcmio, *v_dsivdd;
	static bool bPanelPowerOn = false;
	int rc;

	char *lcm_str = "8921_l11";
	char *lcmio_str = "8921_lvs5";
	char *dsivdd_str = "8921_l2";
        
        printk(KERN_ERR  "[DISP] %s +++\n", __func__);
	/* To avoid system crash in shutdown for non-panel case */
	if (panel_type == PANEL_ID_NONE)
		return -ENODEV;

	printk(KERN_INFO "%s: state : %d\n", __func__, on);

	if (!dsi_power_on) {
		v_lcm = regulator_get(&msm_mipi_dsi1_device.dev,
				lcm_str);
		if (IS_ERR_OR_NULL(v_lcm)) {
			printk(KERN_ERR "could not get %s, rc = %ld\n",
				lcm_str, PTR_ERR(v_lcm));
			return -ENODEV;
		}

		v_lcmio = regulator_get(&msm_mipi_dsi1_device.dev,
				lcmio_str);
		if (IS_ERR_OR_NULL(v_lcmio)) {
			printk(KERN_ERR "could not get %s, rc = %ld\n",
				lcmio_str, PTR_ERR(v_lcmio));
			return -ENODEV;
		}


		v_dsivdd = regulator_get(&msm_mipi_dsi1_device.dev,
				dsivdd_str);
		if (IS_ERR_OR_NULL(v_dsivdd)) {
			printk(KERN_ERR "could not get %s, rc = %ld\n",
				dsivdd_str, PTR_ERR(v_dsivdd));
			return -ENODEV;
		}

		rc = regulator_set_voltage(v_lcm, 3000000, 3000000);
		if (rc) {
			printk(KERN_ERR "%s#%d: set_voltage %s failed, rc=%d\n", __func__, __LINE__, lcm_str, rc);
			return -EINVAL;
		}

		rc = regulator_set_voltage(v_dsivdd, 1200000, 1200000);
		if (rc) {
			printk(KERN_ERR "%s#%d: set_voltage %s failed, rc=%d\n", __func__, __LINE__, dsivdd_str, rc);
			return -EINVAL;
		}

		rc = gpio_request(FIGHTER_LCD_RSTz, "LCM_RST_N");
		if (rc) {
			printk(KERN_ERR "%s:LCM gpio %d request failed, rc=%d\n", __func__,  FIGHTER_LCD_RSTz, rc);
			return -EINVAL;
		}

		dsi_power_on = true;
	}

	if (on) {
		printk(KERN_INFO "%s: on\n", __func__);
		rc = regulator_set_optimum_mode(v_lcm, 100000);
		if (rc < 0) {
			printk(KERN_ERR "set_optimum_mode %s failed, rc=%d\n", lcm_str, rc);
			return -EINVAL;
		}

		rc = regulator_set_optimum_mode(v_dsivdd, 100000);
		if (rc < 0) {
			printk(KERN_ERR "set_optimum_mode %s failed, rc=%d\n", dsivdd_str, rc);
			return -EINVAL;
		}

                    rc = regulator_enable(v_dsivdd);
                    if (rc) {
                      printk(KERN_ERR "enable regulator %s failed, rc=%d\n", dsivdd_str, rc);
                      return -ENODEV;
                    }
                if (isOrise())
                  {
                    //hr_msleep(1);
                    rc = regulator_enable(v_lcmio);
                    if (rc) {
                      printk(KERN_ERR "enable regulator %s failed, rc=%d\n", lcmio_str, rc);
                      return -ENODEV;
                    }
                  }
                else
                  {
                    rc = regulator_enable(v_lcm);
                    if (rc) {
                      printk(KERN_ERR "enable regulator %s failed, rc=%d\n", lcm_str, rc);
                      return -ENODEV;
                    }
                  }

		if (!fighter_panel_first_init)
                  {
                    //                    hr_msleep(10);
                    gpio_set_value(FIGHTER_LCD_RSTz, 1);
                    //hr_msleep(1);
                    gpio_set_value(FIGHTER_LCD_RSTz, 0);
                    //hr_msleep(35);
                    gpio_set_value(FIGHTER_LCD_RSTz, 1);
		}
		//hr_msleep(60);

		bPanelPowerOn = true;
	} 
        else
          {
		printk(KERN_INFO "%s: off\n", __func__);
		if (!bPanelPowerOn) return 0;
		//hr_msleep(100);
		gpio_set_value(FIGHTER_LCD_RSTz, 0);
		//hr_msleep(10);

		if (regulator_disable(v_dsivdd)) {
			printk(KERN_ERR "%s: Unable to enable the regulator: %s\n", __func__, dsivdd_str);
			return -EINVAL;
		}

                if (isOrise())
                  {
                    //hr_msleep(5);
                    if (regulator_disable(v_lcmio)) {
                      printk(KERN_ERR "%s: Unable to enable the regulator: %s\n", __func__, lcmio_str);
                      return -EINVAL;
                    }
                  }
                else
                  {
                    if (regulator_disable(v_lcm)) {
                      printk(KERN_ERR "%s: Unable to enable the regulator: %s\n", __func__, lcm_str);
                      return -EINVAL;
                    }
                  }

		rc = regulator_set_optimum_mode(v_dsivdd, 100);
		if (rc < 0) {
			printk(KERN_ERR "%s: Unable to enable the regulator: %s\n", __func__, dsivdd_str);
			return -EINVAL;
		}

		bPanelPowerOn = false;
	}
        //        if (bPanelPowerOn)
        //          fighter_display_on(mfd);
	return 0;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio = FIGHTER_LCD_TE,
	.dsi_power_save = mipi_dsi_panel_power,
};

static struct platform_device mipi_dsi_fighter_panel_device = {
	.name = "mipi_fighter",
	.id = 0,
};

#ifdef CONFIG_MSM_BUS_SCALING

static struct msm_bus_vectors mdp_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors mdp_ui_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216000000 * 2,
		.ib = 270000000 * 2,
	},
};

static struct msm_bus_vectors mdp_vga_vectors[] = {
	/* VGA and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216000000 * 2,
		.ib = 270000000 * 2,
	},
};

static struct msm_bus_vectors mdp_720p_vectors[] = {
	/* 720p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 230400000 * 2,
		.ib = 288000000 * 2,
	},
};

static struct msm_bus_vectors mdp_1080p_vectors[] = {
	/* 1080p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 334080000 * 2,
		.ib = 417600000 * 2,
	},
};

static struct msm_bus_paths mdp_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(mdp_init_vectors),
		mdp_init_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_vga_vectors),
		mdp_vga_vectors,
	},
	{
		ARRAY_SIZE(mdp_720p_vectors),
		mdp_720p_vectors,
	},
	{
		ARRAY_SIZE(mdp_1080p_vectors),
		mdp_1080p_vectors,
	},
};

static struct msm_bus_scale_pdata mdp_bus_scale_pdata = {
	mdp_bus_scale_usecases,
	ARRAY_SIZE(mdp_bus_scale_usecases),
	.name = "mdp",
};

static struct msm_bus_vectors dtv_bus_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};
static struct msm_bus_vectors dtv_bus_def_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 566092800 * 2,
		.ib = 707616000 * 2,
	},
};
static struct msm_bus_paths dtv_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(dtv_bus_init_vectors),
		dtv_bus_init_vectors,
	},
	{
		ARRAY_SIZE(dtv_bus_def_vectors),
		dtv_bus_def_vectors,
	},
};
static struct msm_bus_scale_pdata dtv_bus_scale_pdata = {
	dtv_bus_scale_usecases,
	ARRAY_SIZE(dtv_bus_scale_usecases),
	.name = "dtv",
};

static struct lcdc_platform_data dtv_pdata = {
	.bus_scale_table = &dtv_bus_scale_pdata,
};
#endif

int mdp_core_clk_rate_table[] = {
	85330000,
	85330000,
	160000000,
	200000000,
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = FIGHTER_LCD_TE,
	//	.mdp_core_clk_rate = 85330000,
	//	.mdp_core_clk_table = mdp_core_clk_rate_table,
	//	.num_mdp_clk = ARRAY_SIZE(mdp_core_clk_rate_table),
#ifdef CONFIG_MSM_BUS_SCALING
	.mdp_bus_scale_table = &mdp_bus_scale_pdata,
#endif
	.mdp_rev = MDP_REV_42,
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	.mem_hid = BIT(ION_CP_MM_HEAP_ID),
#else
	.mem_hid = MEMTYPE_EBI1,
#endif
	.mdp_max_clk = 200000000,
};

void __init msm8960_allocate_fb_region(void)
{
	void *addr;
	unsigned long size;

	size = MSM_FB_SIZE;
	addr = alloc_bootmem_align(size, 0x1000);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
			size, addr, __pa(addr));

}

void __init msm8960_mdp_writeback(struct memtype_reserve* reserve_table)
{
	mdp_pdata.ov0_wb_size = MSM_FB_OVERLAY0_WRITEBACK_SIZE;
	mdp_pdata.ov1_wb_size = MSM_FB_OVERLAY1_WRITEBACK_SIZE;
#if defined(CONFIG_ANDROID_PMEM) && !defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	reserve_table[mdp_pdata.mem_hid].size +=
		mdp_pdata.ov0_wb_size;
	reserve_table[mdp_pdata.mem_hid].size +=
		mdp_pdata.ov1_wb_size;
#endif
}

static char wfd_check_mdp_iommu_split_domain(void)
{
	return mdp_pdata.mdp_iommu_split_domain;
}

#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
static struct msm_wfd_platform_data wfd_pdata = {
	.wfd_check_mdp_iommu_split = wfd_check_mdp_iommu_split_domain,
};

static struct platform_device wfd_panel_device = {
	.name = "wfd_panel",
	.id = 0,
	.dev.platform_data = NULL,
};

static struct platform_device wfd_device = {
	.name          = "msm_wfd",
	.id            = -1,
	.dev.platform_data = &wfd_pdata,
};
#endif

void __init fighter_init_fb(void)
{
	msm8960_set_display_params("mipi_fighter", "hdmi_msm");
	platform_device_register(&msm_fb_device);
#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
	platform_device_register(&wfd_panel_device);
	platform_device_register(&wfd_device);
#endif
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
	platform_device_register(&hdmi_msm_device);
#endif
	platform_device_register(&mipi_dsi_fighter_panel_device);
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
	msm_fb_register_device("dtv", &dtv_pdata);
}
