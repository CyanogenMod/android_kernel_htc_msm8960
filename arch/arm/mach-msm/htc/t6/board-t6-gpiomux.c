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
 *
 */

#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <asm/mach-types.h>
#include <asm/mach/mmc.h>
#include <mach/msm_bus_board.h>
#include <mach/board.h>
#include <linux/gpio.h>
#include <mach/gpiomux.h>
#include <mach/socinfo.h>
#include "devices.h"

#include "board-t6.h"

static struct gpiomux_setting mi2s_rx_sclk = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mi2s_rx_ws = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mi2s_rx_dout0 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mi2s_rx_dout3 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm8960_mi2s_rx_configs[] __initdata = {
	{
		.gpio = 27,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mi2s_rx_ws,
		},
	},
	{
		.gpio = 28,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mi2s_rx_sclk,
		},
	},
	{
		.gpio = 29,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mi2s_rx_dout3,
		},
	},
	{
		.gpio = 32,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mi2s_rx_dout0,
		},
	},
};

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)

static struct gpiomux_setting gpio_spi_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_spi_cs_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_UP,
};

struct msm_gpiomux_config t6_ethernet_configs[] = {
};
#endif

static struct gpiomux_setting gpio_i2c_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting gpio_i2c_config_sus = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting gpio_gsbi7_data_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_gsbi7_clk_config = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting cdc_mclk = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting wdc_intr = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting wcnss_5wire_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting wcnss_5wire_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting slimbus = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

#ifdef CONFIG_SERIAL_CIR
static struct gpiomux_setting gsbi3_tx_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_4MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting gsbi3_rx_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_4MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting gsbi3_tx_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_4MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi3_rx_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_4MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif

static struct gpiomux_setting gsbi7_func1_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting gsbi7_func2_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi3_suspended_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi3_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi4_suspended_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting gsbi4_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

#ifdef CONFIG_USB_EHCI_MSM_HSIC
static struct gpiomux_setting hsic_act_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hsic_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting hsic_wakeup_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting hsic_wakeup_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config t6_hsic_configs[] = {
	{
		.gpio = 88,
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
	{
		.gpio = 89,
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
	{
		.gpio = 47,
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_wakeup_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_wakeup_sus_cfg,
		},
	},
};
#endif

static struct gpiomux_setting mdp_vsync_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdp_vsync_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct msm_gpiomux_config mdp_vsync_configs[] __initdata = {
	{
		.gpio = MSM_LCD_TE,
		.settings = {
			[GPIOMUX_ACTIVE] = &mdp_vsync_active_cfg,
			[GPIOMUX_SUSPENDED] = &mdp_vsync_suspend_cfg,
		},
	}
};

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static struct gpiomux_setting mhl_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mhl_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config mhl_configs[] __initdata = {
	{
		.gpio = MSM_MHL_INT,
		.settings = {
			[GPIOMUX_ACTIVE] = &mhl_active_cfg,
			[GPIOMUX_SUSPENDED] = &mhl_suspend_cfg,
		},
	},
};

static struct gpiomux_setting hdmi_suspend_pu_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting hdmi_suspend_np_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hdmi_active_1_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting hdmi_active_2_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config hdmi_configs[] __initdata = {
	{
		.gpio = MSM_HDMI_DDC_CLK,
		.settings = {
			[GPIOMUX_ACTIVE] = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_np_cfg,
		},
	},
	{
		.gpio = MSM_HDMI_DDC_DATA,
		.settings = {
			[GPIOMUX_ACTIVE] = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_np_cfg,
		},
	},
	{
		.gpio = MSM_HDMI_HPLG_DET,
		.settings = {
			[GPIOMUX_ACTIVE] = &hdmi_active_2_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_pu_cfg,
		},
	},
};
#endif

static struct msm_gpiomux_config t6_gsbi_configs[] __initdata = {
	{
		.gpio = 25,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_sus,
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
		},
	},
	{
		.gpio = 24,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config_sus,
			[GPIOMUX_ACTIVE] = &gpio_i2c_config,
		},
	},

#ifdef CONFIG_SERIAL_CIR
	{
		.gpio = 6,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi3_tx_suspend_cfg,
			[GPIOMUX_ACTIVE] = &gsbi3_tx_active_cfg,
		},
	},
	{
		.gpio = 7,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi3_rx_suspend_cfg,
			[GPIOMUX_ACTIVE] = &gsbi3_rx_active_cfg,
		},
	},
#endif

	{
		.gpio = 8,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi3_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi3_active_cfg,
		},
	},
	{
		.gpio = 9,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi3_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi3_active_cfg,
		},
	},

	{
		.gpio = 12,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi4_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi4_active_cfg,
		},
	},
	{
		.gpio = 13,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi4_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi4_active_cfg,
		},
	},

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio = 52,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio = 53,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio = 54,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
#endif
	{
		.gpio = 53,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_cs_config,
		},
	},
	{
		.gpio = 82,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi7_func2_cfg,
		},
	},
	{
		.gpio = 83,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi7_func1_cfg,
		},
	},

	{
		.gpio = 85,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_gsbi7_clk_config,
			[GPIOMUX_ACTIVE] = &gpio_gsbi7_clk_config,
		},
	},
	{
		.gpio = 84,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_gsbi7_data_config,
			[GPIOMUX_ACTIVE] = &gpio_gsbi7_data_config,
		},
	},
};

static struct msm_gpiomux_config t6_slimbus_config[] __initdata = {
	{
		.gpio = 40,
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
	{
		.gpio = 41,
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
};

static struct msm_gpiomux_config t6_audio_codec_configs[] __initdata = {
	{
		.gpio = 39,
		.settings = {
			[GPIOMUX_SUSPENDED] = &cdc_mclk,
		},
	},
	{
		.gpio = 42,
		.settings = {
			[GPIOMUX_SUSPENDED] = &wdc_intr,
		},
	},
};

static struct gpiomux_setting gpio_rotate_key_act_config = {
	.pull = GPIOMUX_PULL_UP,
	.drv = GPIOMUX_DRV_8MA,
	.func = GPIOMUX_FUNC_GPIO,
};

static struct gpiomux_setting gpio_rotate_key_sus_config = {
	.pull = GPIOMUX_PULL_NONE,
	.drv = GPIOMUX_DRV_2MA,
	.func = GPIOMUX_FUNC_GPIO,
};

struct msm_gpiomux_config t6_rotate_key_config[] = {
	{
		.gpio = 46,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_rotate_key_sus_config,
			[GPIOMUX_ACTIVE] = &gpio_rotate_key_act_config,
		}
	},
};

static struct msm_gpiomux_config wcnss_5wire_interface[] = {
	{
		.gpio = 64,
		.settings = {
			[GPIOMUX_ACTIVE] = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 65,
		.settings = {
			[GPIOMUX_ACTIVE] = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},

	{
		.gpio = 66,
		.settings = {
			[GPIOMUX_ACTIVE] = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},

	{
		.gpio = 67,
		.settings = {
			[GPIOMUX_ACTIVE] = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 68,
		.settings = {
			[GPIOMUX_ACTIVE] = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
};

static struct gpiomux_setting gsbi1_uartdm_active = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi1_uartdm_suspended = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config apq8064_uartdm_gsbi1_configs[] __initdata = {
	{
		.gpio = 18,
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi1_uartdm_active,
			[GPIOMUX_SUSPENDED] = &gsbi1_uartdm_suspended,
		},
	},
	{
		.gpio = 19,
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi1_uartdm_active,
			[GPIOMUX_SUSPENDED] = &gsbi1_uartdm_suspended,
		},
	},
	{
		.gpio = 20,
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi1_uartdm_active,
			[GPIOMUX_SUSPENDED] = &gsbi1_uartdm_suspended,
		},
	},
	{
		.gpio = 21,
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi1_uartdm_active,
			[GPIOMUX_SUSPENDED] = &gsbi1_uartdm_suspended,
		},
	},
};

void __init t6_init_gpiomux(void)
{
	int rc;

	rc = msm_gpiomux_init(NR_GPIO_IRQS);
	if (rc) {
		pr_err(KERN_ERR "msm_gpiomux_init failed %d\n", rc);
		return;
	}
#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
	msm_gpiomux_install(t6_ethernet_configs,
			    ARRAY_SIZE(t6_ethernet_configs));
#endif

	msm_gpiomux_install(wcnss_5wire_interface,
			    ARRAY_SIZE(wcnss_5wire_interface));

	msm_gpiomux_install(t6_gsbi_configs, ARRAY_SIZE(t6_gsbi_configs));

	msm_gpiomux_install(t6_slimbus_config, ARRAY_SIZE(t6_slimbus_config));
	msm_gpiomux_install(t6_audio_codec_configs,
			    ARRAY_SIZE(t6_audio_codec_configs));

	msm_gpiomux_install(msm8960_mi2s_rx_configs,
			    ARRAY_SIZE(msm8960_mi2s_rx_configs));
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
	msm_gpiomux_install(hdmi_configs, ARRAY_SIZE(hdmi_configs));
	msm_gpiomux_install(mhl_configs, ARRAY_SIZE(mhl_configs));
#endif
	msm_gpiomux_install(mdp_vsync_configs, ARRAY_SIZE(mdp_vsync_configs));

#ifdef CONFIG_USB_EHCI_MSM_HSIC
	msm_gpiomux_install(t6_hsic_configs, ARRAY_SIZE(t6_hsic_configs));
#endif

	msm_gpiomux_install(apq8064_uartdm_gsbi1_configs,
			    ARRAY_SIZE(apq8064_uartdm_gsbi1_configs));
}
