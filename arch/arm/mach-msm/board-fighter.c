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
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>
#include <linux/i2c/isl9519.h>
#include <linux/gpio.h>
#include <linux/usb/android_composite.h>
#include <linux/msm_ssbi.h>
#include <linux/pn544.h>
#include <linux/regulator/gpio-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/slimbus/slimbus.h>
#include <linux/bootmem.h>
#include <linux/msm_kgsl.h>
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif
#include <linux/cyttsp.h>
#include <linux/dma-mapping.h>
#include <linux/platform_data/qcom_crypto_device.h>
#include <linux/platform_data/qcom_wcnss_device.h>
#include <linux/leds.h>
#include <linux/leds-pm8xxx.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/synaptics_i2c_rmi.h>
#include <linux/msm_tsens.h>
#include <linux/ks8851.h>
#include <linux/proc_fs.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>
#include <asm/hardware/gic.h>
#include <asm/mach/mmc.h>
#include <linux/cm3629.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/msm_spi.h>
#ifdef CONFIG_USB_MSM_OTG_72K
#include <mach/msm_hsusb.h>
#else
#include <linux/usb/msm_hsusb.h>
#endif
#include <mach/htc_usb.h>
#include <mach/usbdiag.h>
#include <mach/socinfo.h>
#include <mach/rpm.h>
#include <mach/gpio.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_memtypes.h>
#include <mach/dma.h>
#include <mach/msm_dsps.h>
#include <mach/msm_xo.h>
#include <mach/restart.h>
#include <mach/htc_headset_mgr.h>
#include <mach/htc_headset_pmic.h>
#include <mach/cable_detect.h>

#include <linux/akm8975.h>
#include <linux/bma250.h>
#ifdef CONFIG_WCD9310_CODEC
#include <linux/slimbus/slimbus.h>
#include <linux/mfd/wcd9310/core.h>
#include <linux/mfd/wcd9310/pdata.h>
#endif

#ifdef CONFIG_PERFLOCK
#include <mach/perflock.h>
#endif

#ifdef CONFIG_BT
#include <mach/htc_bdaddress.h>
#endif

#include <linux/ion.h>
#include <mach/ion.h>

#include <mach/msm_rtb.h>
#include <mach/msm_cache_dump.h>
#include <mach/scm.h>
#include <linux/fmem.h>

#include "timer.h"
#include "devices.h"
#include "devices-msm8x60.h"
#include "spm.h"
#include "board-fighter.h"
#include <mach/pm.h>
#include <mach/cpuidle.h>
#include "rpm_resources.h"
#include "mpm.h"
#include "acpuclock.h"
#include "rpm_log.h"
#include "smd_private.h"
#include "pm-boot.h"

#include <linux/atmel_qt602240.h>
#include <mach/tpa2051d3.h>

#include <linux/htc_flashlight.h>
#include <mach/board_htc.h>
#include <mach/htc_util.h>
#ifdef CONFIG_HTC_BATT_8960
#include "mach/htc_battery_8960.h"
#include "mach/htc_battery_cell.h"
#include "linux/mfd/pm8xxx/pm8921-charger.h"
#endif


static uint32_t msm_rpm_get_swfi_latency(void);
static unsigned int engineerid;

#ifdef CONFIG_FLASHLIGHT_TPS61310
#ifdef CONFIG_MSM_CAMERA_FLASH
static int flashlight_control(int mode)
{
pr_info("%s, linear led, mode=%d", __func__, mode);
	return tps61310_flashlight_control(mode);
}
#endif

static void config_flashlight_gpios(void)
{
	static uint32_t flashlight_gpio_table[] = {
		GPIO_CFG(32, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG(33, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	};

	gpio_tlmm_config(flashlight_gpio_table[0], GPIO_CFG_ENABLE);
	gpio_tlmm_config(flashlight_gpio_table[1], GPIO_CFG_ENABLE);
}

static struct TPS61310_flashlight_platform_data fighter_flashlight_data = {
	.gpio_init = config_flashlight_gpios,
	.tps61310_strb0 = 33,
	.tps61310_strb1 = 32,
	.led_count = 1,
	.flash_duration_ms = 600,
};

static struct i2c_board_info i2c_tps61310_flashlight[] = {
	{
		I2C_BOARD_INFO("TPS61310_FLASHLIGHT", 0x66 >> 1),
		.platform_data = &fighter_flashlight_data,
	},
};
#endif

static struct platform_device msm_fm_platform_init = {
	.name = "iris_fm",
	.id   = -1,
};

struct pm8xxx_gpio_init {
	unsigned			gpio;
	struct pm_gpio			config;
};

struct pm8xxx_mpp_init {
	unsigned			mpp;
	struct pm8xxx_mpp_config_data	config;
};

#define PM8XXX_GPIO_INIT(_gpio, _dir, _buf, _val, _pull, _vin, _out_strength, \
			_func, _inv, _disable) \
{ \
	.gpio	= PM8921_GPIO_PM_TO_SYS(_gpio), \
	.config	= { \
		.direction	= _dir, \
		.output_buffer	= _buf, \
		.output_value	= _val, \
		.pull		= _pull, \
		.vin_sel	= _vin, \
		.out_strength	= _out_strength, \
		.function	= _func, \
		.inv_int_pol	= _inv, \
		.disable_pin	= _disable, \
	} \
}

#define PM8XXX_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8921_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

#define PM8XXX_GPIO_DISABLE(_gpio) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, 0, 0, 0, PM_GPIO_VIN_S4, \
			 0, 0, 0, 1)

#define PM8XXX_GPIO_OUTPUT(_gpio, _val) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_HIGH, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_INPUT(_gpio, _pull) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, PM_GPIO_OUT_BUF_CMOS, 0, \
			_pull, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_NO, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_OUTPUT_FUNC(_gpio, _val, _func) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_HIGH, \
			_func, 0, 0)

#define PM8XXX_GPIO_OUTPUT_VIN_BB_FUNC(_gpio, _val, _func) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_BB, \
			PM_GPIO_STRENGTH_HIGH, \
			_func, 0, 0)

/* Initial PM8921 GPIO configurations */
static struct pm8xxx_gpio_init pm8921_gpios[] __initdata = {
	PM8XXX_GPIO_OUTPUT_VIN_BB_FUNC(FIGHTER_GREEN_LED, 1, PM_GPIO_FUNC_NORMAL),
	PM8XXX_GPIO_OUTPUT_VIN_BB_FUNC(FIGHTER_AMBER_LED, 1, PM_GPIO_FUNC_NORMAL),
	PM8XXX_GPIO_INIT(FIGHTER_EARPHONE_DETz, PM_GPIO_DIR_IN,
			 PM_GPIO_OUT_BUF_CMOS, 0, PM_GPIO_PULL_UP_1P5,
			 PM_GPIO_VIN_S4, PM_GPIO_STRENGTH_LOW,
			 PM_GPIO_FUNC_NORMAL, 0, 0),
};

/* Initial PM8921 MPP configurations */
static struct pm8xxx_mpp_init pm8921_mpps[] __initdata = {
	/* External 5V regulator enable; shared by HDMI and USB_OTG switches. */
	PM8XXX_MPP_INIT(PM8XXX_AMUX_MPP_8, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH8,
								DOUT_CTRL_LOW),
};

static void __init pm8921_gpio_mpp_init(void)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(pm8921_gpios); i++) {
		rc = pm8xxx_gpio_config(pm8921_gpios[i].gpio,
					&pm8921_gpios[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_gpio_config: rc=%d\n", __func__, rc);
			break;
		}
	}

	for (i = 0; i < ARRAY_SIZE(pm8921_mpps); i++) {
		rc = pm8xxx_mpp_config(pm8921_mpps[i].mpp,
					&pm8921_mpps[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_mpp_config: rc=%d\n", __func__, rc);
			break;
		}
	}
}

#ifdef CONFIG_I2C

#define MSM_8960_GSBI2_QUP_I2C_BUS_ID 2
#define MSM_8960_GSBI4_QUP_I2C_BUS_ID 4
#define MSM_8960_GSBI3_QUP_I2C_BUS_ID 3
#define MSM_8960_GSBI8_QUP_I2C_BUS_ID 8
#define MSM_8960_GSBI12_QUP_I2C_BUS_ID 12

#endif

#define MSM_PMEM_ADSP_SIZE         0x6D00000
#define MSM_PMEM_ADSP2_SIZE        0x700000
#define MSM_PMEM_AUDIO_SIZE        0x2B4000
#ifdef CONFIG_MSM_IOMMU
#define MSM_PMEM_SIZE 0x00000000 /* 0 Mbytes */
#else
#define MSM_PMEM_SIZE 0x4000000 /* 64 Mbytes */
#endif
#define MSM_LIQUID_PMEM_SIZE 0x4000000 /* 64 Mbytes */

#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
#define MSM_PMEM_KERNEL_EBI1_SIZE  0x280000
#define MSM_ION_SF_SIZE		MSM_PMEM_SIZE
#define MSM_ION_MM_FW_SIZE	0x200000 /* (2MB) */
#ifdef CONFIG_MSM_IOMMU
/* QHD PREVIEW x 9
 * QHD THUMBNAIL x 5
 * 8M 16:9 SNAPSHOT x 5
 * 8M 16:9 JPEG x 1
 * 3A, 4K x 15 + 8K x 3
 * ALIGH INTERGER + 1MB
 */
#define MSM_ION_MM_SIZE		0x3F00000
#else
#define MSM_ION_MM_SIZE		MSM_PMEM_ADSP_SIZE - MSM_PMEM_ADSP2_SIZE
#endif
#define MSM_ION_ROTATOR_SIZE	MSM_PMEM_ADSP2_SIZE
#define MSM_ION_QSECOM_SIZE	0x100000 /* (1MB) */
#define MSM_ION_MFC_SIZE	0x100000  //SZ_8K
#ifdef CONFIG_MSM_IOMMU
#define MSM_ION_HEAP_NUM	7
#else
#define MSM_ION_HEAP_NUM	8
#endif
#define MSM_LIQUID_ION_MM_SIZE (MSM_ION_MM_SIZE + 0x600000)
static unsigned int msm_ion_cp_mm_size = MSM_ION_MM_SIZE;
#else
#define MSM_PMEM_KERNEL_EBI1_SIZE  0x110C000
#define MSM_ION_HEAP_NUM	1
#endif

#ifdef CONFIG_KERNEL_PMEM_EBI_REGION
static unsigned pmem_kernel_ebi1_size = MSM_PMEM_KERNEL_EBI1_SIZE;
static int __init pmem_kernel_ebi1_size_setup(char *p)
{
	pmem_kernel_ebi1_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_kernel_ebi1_size", pmem_kernel_ebi1_size_setup);
#endif

#ifdef CONFIG_ANDROID_PMEM
static unsigned pmem_size = MSM_PMEM_SIZE;
static unsigned pmem_param_set = 0;
static int __init pmem_size_setup(char *p)
{
	pmem_size = memparse(p, NULL);
	pmem_param_set = 1;
	return 0;
}
early_param("pmem_size", pmem_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;

static int __init pmem_adsp_size_setup(char *p)
{
	pmem_adsp_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_adsp_size", pmem_adsp_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;

static int __init pmem_audio_size_setup(char *p)
{
	pmem_audio_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_audio_size", pmem_audio_size_setup);
#endif

#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = {.platform_data = &android_pmem_pdata},
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};
static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};
#endif

static struct android_pmem_platform_data android_pmem_audio_pdata = {
	.name = "pmem_audio",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_audio_device = {
	.name = "android_pmem",
	.id = 4,
	.dev = { .platform_data = &android_pmem_audio_pdata },
};
#endif

struct fmem_platform_data fmem_pdata = {
};

static struct memtype_reserve msm8960_reserve_table[] __initdata = {
	[MEMTYPE_SMI] = {
	},
	[MEMTYPE_EBI0] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
	[MEMTYPE_EBI1] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
};

#if defined(CONFIG_MSM_RTB)
static struct msm_rtb_platform_data msm_rtb_pdata = {
	.size = SZ_1K,
};

static int __init msm_rtb_set_buffer_size(char *p)
{
	int s;

	s = memparse(p, NULL);
	msm_rtb_pdata.size = ALIGN(s, SZ_4K);
	return 0;
}
early_param("msm_rtb_size", msm_rtb_set_buffer_size);


static struct platform_device msm_rtb_device = {
	.name           = "msm_rtb",
	.id             = -1,
	.dev            = {
		.platform_data = &msm_rtb_pdata,
	},
};
#endif

static void __init reserve_rtb_memory(void)
{
#if defined(CONFIG_MSM_RTB)
	msm8960_reserve_table[MEMTYPE_EBI1].size += msm_rtb_pdata.size;
#endif
}

static void __init size_pmem_devices(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	android_pmem_adsp_pdata.size = pmem_adsp_size;

	if (!pmem_param_set && machine_is_msm8960_liquid())
		pmem_size = MSM_LIQUID_PMEM_SIZE;
	android_pmem_pdata.size = pmem_size;
#endif
	android_pmem_audio_pdata.size = MSM_PMEM_AUDIO_SIZE;
#endif
}

static void __init reserve_memory_for(struct android_pmem_platform_data *p)
{
	msm8960_reserve_table[p->memory_type].size += p->size;
}

static void __init reserve_pmem_memory(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	reserve_memory_for(&android_pmem_adsp_pdata);
	reserve_memory_for(&android_pmem_pdata);
#endif
	reserve_memory_for(&android_pmem_audio_pdata);
	msm8960_reserve_table[MEMTYPE_EBI1].size += pmem_kernel_ebi1_size;
#endif
}

static void __init reserve_fmem_memory(void)
{
}

static int msm8960_paddr_to_memtype(unsigned int paddr)
{
	return MEMTYPE_EBI1;
}

#ifdef CONFIG_ION_MSM
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct ion_cp_heap_pdata cp_mm_ion_pdata = {
	.permission_type = IPT_TYPE_MM_CARVEOUT,
	.align = PAGE_SIZE,
};

static struct ion_cp_heap_pdata cp_mfc_ion_pdata = {
	.permission_type = IPT_TYPE_MFC_SHAREDMEM,
	.align = PAGE_SIZE,
};

static struct ion_co_heap_pdata co_ion_pdata = {
	.adjacent_mem_id = INVALID_HEAP_ID,
	.align = PAGE_SIZE,
};

static struct ion_co_heap_pdata fw_co_ion_pdata = {
	.adjacent_mem_id = ION_CP_MM_HEAP_ID,
	.align = SZ_128K,
};
#endif

/**
 * These heaps are listed in the order they will be allocated. Due to
 * video hardware restrictions and content protection the FW heap has to
 * be allocated adjacent (below) the MM heap and the MFC heap has to be
 * allocated after the MM heap to ensure MFC heap is not more than 256MB
 * away from the base address of the FW heap.
 * However, the order of FW heap and MM heap doesn't matter since these
 * two heaps are taken care of by separate code to ensure they are adjacent
 * to each other.
 * Don't swap the order unless you know what you are doing!
 */
static struct ion_platform_data ion_pdata = {
	.nr = MSM_ION_HEAP_NUM,
	.heaps = {
		{
			.id	= ION_SYSTEM_HEAP_ID,
			.type	= ION_HEAP_TYPE_SYSTEM,
			.name	= ION_VMALLOC_HEAP_NAME,
		},
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
		{
			.id	= ION_CP_MM_HEAP_ID,
			.type	= ION_HEAP_TYPE_CP,
			.name	= ION_MM_HEAP_NAME,
			.size	= MSM_ION_MM_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &cp_mm_ion_pdata,
		},
		{
			.id	= ION_CP_ROTATOR_HEAP_ID,
			.type	= ION_HEAP_TYPE_CP,
			.name	= ION_ROTATOR_HEAP_NAME,
			.size	= MSM_ION_ROTATOR_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &cp_mm_ion_pdata,
		},
		{
			.id	= ION_MM_FIRMWARE_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_MM_FIRMWARE_HEAP_NAME,
			.size	= MSM_ION_MM_FW_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &fw_co_ion_pdata,
		},
		{
			.id	= ION_CP_MFC_HEAP_ID,
			.type	= ION_HEAP_TYPE_CP,
			.name	= ION_MFC_HEAP_NAME,
			.size	= MSM_ION_MFC_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &cp_mfc_ion_pdata,
		},
#ifndef CONFIG_MSM_IOMMU
		{
			.id	= ION_SF_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_SF_HEAP_NAME,
			.size	= MSM_ION_SF_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_ion_pdata,
		},
#endif
		{
			.id	= ION_IOMMU_HEAP_ID,
			.type	= ION_HEAP_TYPE_IOMMU,
			.name	= ION_IOMMU_HEAP_NAME,
		},
		{
			.id	= ION_QSECOM_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_QSECOM_HEAP_NAME,
			.size	= MSM_ION_QSECOM_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_ion_pdata,
		},
#endif
	}
};

static struct platform_device ion_dev = {
	.name = "ion-msm",
	.id = 1,
	.dev = { .platform_data = &ion_pdata },
};
#endif

struct platform_device fmem_device = {
	.name = "fmem",
	.id = 1,
	.dev = { .platform_data = &fmem_pdata },
};

static void reserve_ion_memory(void)
{
#if defined(CONFIG_ION_MSM) && defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	unsigned int i;

	if (!pmem_param_set && machine_is_msm8960_liquid()) {
		msm_ion_cp_mm_size = MSM_LIQUID_ION_MM_SIZE;
		for (i = 0; i < ion_pdata.nr; i++) {
			if (ion_pdata.heaps[i].id == ION_CP_MM_HEAP_ID) {
				ion_pdata.heaps[i].size = msm_ion_cp_mm_size;
				pr_debug("msm_ion_cp_mm_size 0x%x\n",
					msm_ion_cp_mm_size);
				break;
			}
		}
	}
	msm8960_reserve_table[MEMTYPE_EBI1].size += msm_ion_cp_mm_size;
	msm8960_reserve_table[MEMTYPE_EBI1].size += MSM_ION_ROTATOR_SIZE;
	msm8960_reserve_table[MEMTYPE_EBI1].size += MSM_ION_MM_FW_SIZE;
	msm8960_reserve_table[MEMTYPE_EBI1].size += MSM_ION_SF_SIZE;
	msm8960_reserve_table[MEMTYPE_EBI1].size += MSM_ION_MFC_SIZE;
	msm8960_reserve_table[MEMTYPE_EBI1].size += MSM_ION_QSECOM_SIZE;
#endif
}

void __init msm8960_mdp_writeback(struct memtype_reserve* reserve_table);
static void __init reserve_mdp_memory(void)
{
	msm8960_mdp_writeback(msm8960_reserve_table);
}

#if defined(CONFIG_MSM_CACHE_DUMP)
static struct msm_cache_dump_platform_data msm_cache_dump_pdata = {
	.l2_size = L2_BUFFER_SIZE,
};

static struct platform_device msm_cache_dump_device = {
	.name		= "msm_cache_dump",
	.id		= -1,
	.dev		= {
		.platform_data = &msm_cache_dump_pdata,
	},
};

#endif

static void reserve_cache_dump_memory(void)
{
#ifdef CONFIG_MSM_CACHE_DUMP
	unsigned int spare;
	unsigned int l1_size;
	unsigned int total;
	int ret;

	ret = scm_call(L1C_SERVICE_ID, L1C_BUFFER_GET_SIZE_COMMAND_ID, &spare,
		sizeof(spare), &l1_size, sizeof(l1_size));

	if (ret)
		/* Fall back to something reasonable here */
		l1_size = L1_BUFFER_SIZE;

	total = l1_size + L2_BUFFER_SIZE;

	msm8960_reserve_table[MEMTYPE_EBI1].size += total;
	msm_cache_dump_pdata.l1_size = l1_size;
#endif
}

static void __init msm8960_calculate_reserve_sizes(void)
{
	size_pmem_devices();
	reserve_pmem_memory();
	reserve_ion_memory();
	reserve_fmem_memory();
	reserve_mdp_memory();
	reserve_rtb_memory();
	reserve_cache_dump_memory();
}

static struct reserve_info msm8960_reserve_info __initdata = {
	.memtype_reserve_table = msm8960_reserve_table,
	.calculate_reserve_sizes = msm8960_calculate_reserve_sizes,
	.paddr_to_memtype = msm8960_paddr_to_memtype,
};

#ifdef MEMORY_HOTPLUG
static int msm8960_memory_bank_size(void)
{
	return 1<<29;
}

static void __init locate_unstable_memory(void)
{
	struct membank *mb = &meminfo.bank[meminfo.nr_banks - 1];
	unsigned long bank_size;
	unsigned long low, high;

	bank_size = msm8960_memory_bank_size();
	low = meminfo.bank[0].start;
	high = mb->start + mb->size;

	/* Check if 32 bit overflow occured */
	if (high < mb->start)
		high = ~0UL;

	low &= ~(bank_size - 1);

	if (high - low <= bank_size)
		return;
	msm8960_reserve_info.low_unstable_address = low + bank_size;
	/* To avoid overflow of u32 compute max_unstable_size
	 * by first subtracting low from mb->start)
	 * */
	msm8960_reserve_info.max_unstable_size = (mb->start - low) +
						mb->size - bank_size;

	msm8960_reserve_info.bank_size = bank_size;
	pr_info("low unstable address %lx max size %lx bank size %lx\n",
		msm8960_reserve_info.low_unstable_address,
		msm8960_reserve_info.max_unstable_size,
		msm8960_reserve_info.bank_size);
}

static void __init place_movable_zone(void)
{
	movable_reserved_start = msm8960_reserve_info.low_unstable_address;
	movable_reserved_size = msm8960_reserve_info.max_unstable_size;
	pr_info("movable zone start %lx size %lx\n",
		movable_reserved_start, movable_reserved_size);
}
#endif

static void __init fighter_early_memory(void)
{
	reserve_info = &msm8960_reserve_info;
#ifdef MEMORY_HOTPLUG
	locate_unstable_memory();
	place_movable_zone();
#endif
}

static void __init fighter_reserve(void)
{
	msm_reserve();
	fmem_pdata.phys = reserve_memory_for_fmem(fmem_pdata.size);
}
static int msm8960_change_memory_power(u64 start, u64 size,
	int change_type)
{
	return soc_change_memory_power(start, size, change_type);
}

#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_2_PHASE
int set_two_phase_freq(int cpufreq);
#endif

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#define MSM_FB_PRIM_BUF_SIZE (960 * 544 * 4 * 3) /* 4 bpp x 3 pages */
#else
#define MSM_FB_PRIM_BUF_SIZE (960 * 544 * 4 * 2) /* 4 bpp x 2 pages */
#endif


#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
#define MSM_FB_EXT_BUF_SIZE	(1920 * 1088 * 2 * 1) /* 2 bpp x 1 page */
#elif defined(CONFIG_FB_MSM_TVOUT)
#define MSM_FB_EXT_BUF_SIZE (720 * 576 * 2 * 2) /* 2 bpp x 2 pages */
#else
#define MSM_FB_EXT_BUF_SIZE	0
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

#define MDP_VSYNC_GPIO 0

#define PANEL_NAME_MAX_LEN	30
#define MIPI_CMD_NOVATEK_QHD_PANEL_NAME	"mipi_cmd_novatek_qhd"
#define MIPI_VIDEO_NOVATEK_QHD_PANEL_NAME	"mipi_video_novatek_qhd"
#define MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME	"mipi_video_toshiba_wsvga"
#define MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME	"mipi_video_chimei_wxga"
#define MIPI_VIDEO_SIMULATOR_VGA_PANEL_NAME	"mipi_video_simulator_vga"
#define MIPI_CMD_RENESAS_FWVGA_PANEL_NAME	"mipi_cmd_renesas_fwvga"
#define HDMI_PANEL_NAME	"hdmi_msm"
#define TVOUT_PANEL_NAME	"tvout_msm"

static struct resource msm_fb_resources[] = {
	{
		.flags = IORESOURCE_DMA,
	}
};

static int msm_fb_detect_panel(const char *name)
{
	if (machine_is_msm8960_liquid()) {
		if (!strncmp(name, MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;
	} else {
		if (!strncmp(name, MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

#ifndef CONFIG_FB_MSM_MIPI_PANEL_DETECT
		if (!strncmp(name, MIPI_VIDEO_NOVATEK_QHD_PANEL_NAME,
				strnlen(MIPI_VIDEO_NOVATEK_QHD_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

		if (!strncmp(name, MIPI_CMD_NOVATEK_QHD_PANEL_NAME,
				strnlen(MIPI_CMD_NOVATEK_QHD_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

		if (!strncmp(name, MIPI_VIDEO_SIMULATOR_VGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_SIMULATOR_VGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

		if (!strncmp(name, MIPI_CMD_RENESAS_FWVGA_PANEL_NAME,
				strnlen(MIPI_CMD_RENESAS_FWVGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;
#endif
	}

	if (!strncmp(name, HDMI_PANEL_NAME,
			strnlen(HDMI_PANEL_NAME,
				PANEL_NAME_MAX_LEN)))
		return 0;

	if (!strncmp(name, TVOUT_PANEL_NAME,
			strnlen(TVOUT_PANEL_NAME,
				PANEL_NAME_MAX_LEN)))
		return 0;

	pr_warning("%s: not supported '%s'", __func__, name);
	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

static struct atmel_i2c_platform_data ts_atmel_data[] = {
	{
		.version = 0x0011,
		.build = 0xAA,
		.source = 1, /* YFO */
		.abs_x_min = 0,
		.abs_x_max = 1024,
		.abs_y_min = 0,
		.abs_y_max = 950,
		.abs_pressure_min = 0,
		.abs_pressure_max = 255,
		.abs_width_min = 0,
		.abs_width_max = 20,
		.gpio_irq = FIGHTER_TP_ATTz,
		.config_T6 = {0, 0, 0, 0, 0, 0},
		.config_T7 = {15, 8, 50},
		.config_T8 = {27, 0, 5, 20, 0, 0, 2, 30, 16, 192},
		.config_T9 = {139, 0, 0, 19, 11, 0, 16, 40, 3, 1, 0, 5, 2, 0, 4, 20, 40, 10, 0, 0, 0, 0, 5, 10, 18, 40, 146, 50, 147, 80, 18, 17, 58, 59, 0},
		.config_T15 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T18 = {0, 0},
		.config_T19 = {3, 0, 0, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T23 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T25 = {3, 0, 37, 103, 133, 87, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T40 = {0, 0, 0, 0, 0},
		.config_T42 = {0, 0, 40, 35, 128, 2, 0, 10},
		.config_T46 = {0, 3, 8, 8, 0, 0, 0, 0, 0},
		.config_T47 = {0, 20, 50, 5, 2, 50, 40, 0, 0, 63},
		.config_T48 = {15, 0, 68, 0, 0, 0, 0, 0, 0, 0, 0, 35, 0, 0, 0, 0, 0, 100, 4, 64, 10, 0, 20, 0, 0, 38, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T55 = {0, 0, 0, 0},
		.config_T58 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},
	{
		.version = 0x0011,
		.build = 0xAA,
		.source = 0, /* TPK */
		.abs_x_min = 0,
		.abs_x_max = 1024,
		.abs_y_min = 0,
		.abs_y_max = 950,
		.abs_pressure_min = 0,
		.abs_pressure_max = 255,
		.abs_width_min = 0,
		.abs_width_max = 20,
		.gpio_irq = FIGHTER_TP_ATTz,
		.config_T6 = {0, 0, 0, 0, 0, 0},
		.config_T7 = {15, 8, 50},
		.config_T8 = {20, 0, 5, 20, 0, 0, 2, 30, 16, 192},
		.config_T9 = {139, 0, 0, 19, 11, 0, 16, 40, 3, 1, 0, 5, 2, 0, 4, 20, 40, 10, 0, 0, 0, 0, 3, 6, 22, 38, 143, 55, 147, 82, 18, 17, 58, 59, 0},
		.config_T15 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T18 = {0, 0},
		.config_T19 = {3, 0, 0, 60, 0, 0},
		.config_T23 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T25 = {3, 0, 252, 98, 92, 83, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T40 = {0, 0, 0, 0, 0},
		.config_T42 = {0, 0, 40, 35, 128, 2, 0, 10},
		.config_T46 = {0, 3, 8, 8, 0, 0, 0, 0, 0},
		.config_T47 = {0, 20, 50, 5, 2, 50, 40, 0, 0, 63},
		.config_T48 = {15, 0, 68, 0, 0, 0, 0, 0, 0, 0, 0, 35, 0, 0, 0, 0, 0, 100, 4, 64, 10, 0, 20, 0, 0, 38, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T55 = {0, 0, 0, 0},
		.config_T58 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},
	{
		.version = 0x0010,
		.build = 0xAA,
		.source = 1, /* YFO */
		.abs_x_min = 0,
		.abs_x_max = 1024,
		.abs_y_min = 0,
		.abs_y_max = 950,
		.abs_pressure_min = 0,
		.abs_pressure_max = 255,
		.abs_width_min = 0,
		.abs_width_max = 20,
		.gpio_irq = FIGHTER_TP_ATTz,
		.config_T6 = {0, 0, 0, 0, 0, 0},
		.config_T7 = {15, 8, 50},
		.config_T8 = {20, 0, 5, 20, 0, 0, 2, 30, 16, 192},
		.config_T9 = {139, 0, 0, 19, 11, 0, 16, 40, 1, 1, 0, 5, 2, 0, 4, 20, 40, 10, 0, 0, 0, 0, 0, 0, 35, 35, 142, 60, 138, 75, 18, 17, 58, 59, 0},
		.config_T15 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T18 = {0, 0},
		.config_T19 = {3, 0, 0, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T23 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T25 = {3, 0, 224, 110, 64, 95, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T40 = {0, 0, 0, 0, 0},
		.config_T42 = {0, 0, 40, 35, 128, 2, 0, 10},
		.config_T46 = {0, 3, 4, 8, 0, 0, 0, 0, 0},
		.config_T47 = {0, 20, 50, 5, 2, 50, 40, 0, 0, 63},
		.config_T48 = {3, 64, 196, 0, 0, 0, 0, 0, 0, 0, 0, 25, 0, 0, 0, 0, 0, 100, 4, 64, 10, 0, 20, 0, 0, 38, 0, 25, 0, 0, 0, 0, 0, 0, 0, 35, 3, 5, 2, 1, 5, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 18, 17, 4},
	},
	{
		.version = 0x0010,
		.build = 0xAA,
		.source = 0, /* TPK */
		.abs_x_min = 0,
		.abs_x_max = 1024,
		.abs_y_min = 0,
		.abs_y_max = 950,
		.abs_pressure_min = 0,
		.abs_pressure_max = 255,
		.abs_width_min = 0,
		.abs_width_max = 20,
		.gpio_irq = FIGHTER_TP_ATTz,
		.config_T6 = {0, 0, 0, 0, 0, 0},
		.config_T7 = {15, 8, 50},
		.config_T8 = {20, 0, 5, 20, 0, 0, 2, 30, 16, 192},
		.config_T9 = {139, 0, 0, 19, 11, 0, 16, 40, 1, 1, 0, 5, 2, 0, 4, 20, 40, 10, 0, 0, 0, 0, 10, 10, 40, 40, 138, 50, 139, 60, 18, 17, 58, 59, 0},
		.config_T15 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T18 = {0, 0},
		.config_T19 = {3, 0, 0, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T23 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T25 = {3, 0, 224, 110, 64, 95, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T40 = {0, 0, 0, 0, 0},
		.config_T42 = {0, 0, 40, 35, 128, 2, 0, 10},
		.config_T46 = {0, 3, 4, 8, 0, 0, 0, 0, 0},
		.config_T47 = {0, 20, 50, 5, 2, 50, 40, 0, 0, 63},
		.config_T48 = {3, 64, 196, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 100, 4, 64, 10, 0, 20, 0, 0, 38, 0, 25, 0, 0, 0, 0, 0, 0, 0, 35, 3, 5, 2, 1, 5, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 18, 17, 4},
	},
};

static struct synaptics_i2c_rmi_platform_data syn_ts_3k_data[] = { /* Synatpics sensor */
	{
		.version = 0x3330,
		.packrat_number = 1100754,
		.abs_x_min = 0,
		.abs_x_max = 1000,
		.abs_y_min = 0,
		.abs_y_max = 1760,
		.flags = SYNAPTICS_FLIP_Y,
		.gpio_irq = FIGHTER_TP_ATTz,
		.gpio_reset = FIGHTER_TP_RSTz,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.reduce_report_level = {65, 65, 50, 0, 0},
		.large_obj_check = 1,
		.default_config = 2,
		.customer_register = {0xF9, 0x32, 0x05, 0x64},
		.tw_pin_mask = 0x0080,
		.config = {0x41, 0x30, 0x31, 0x40, 0x00, 0x3F, 0x03, 0x1E,
			0x05, 0xB1, 0x09, 0x0B, 0x01, 0x01, 0x00, 0x00,
			0xE8, 0x03, 0x75, 0x07, 0x02, 0x14, 0x1E, 0x05,
			0x28, 0xF4, 0x28, 0x1E, 0x05, 0x01, 0x3C, 0x17,
			0x01, 0x1B, 0x01, 0x66, 0x4E, 0x66, 0x46, 0x7F,
			0xBB, 0x30, 0xB7, 0x01, 0x40, 0x30, 0x30, 0x00,
			0x00, 0x0A, 0x04, 0xB2, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x19, 0x01, 0x00, 0x0A, 0x50, 0x32,
			0xE2, 0x02, 0x32, 0x0A, 0x0A, 0x96, 0x0B, 0x13,
			0x00, 0x02, 0x66, 0x01, 0x80, 0x02, 0x0E, 0x1F,
			0x15, 0x3B, 0x00, 0x19, 0x04, 0x1B, 0x00, 0x10,
			0xFF, 0x01, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
			0x18, 0x19, 0x1A, 0x1B, 0x11, 0xFF, 0xFF, 0xFF,
			0x03, 0x05, 0x07, 0x13, 0x11, 0x0F, 0x0D, 0x0B,
			0x0A, 0x09, 0x08, 0x01, 0x02, 0x04, 0x06, 0x0C,
			0x0E, 0x10, 0x12, 0xFF, 0xA0, 0xA0, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x3F, 0x3D, 0x3C, 0x3A,
			0x38, 0x37, 0x35, 0x34, 0x00, 0x03, 0x06, 0x09,
			0x0C, 0x0F, 0x12, 0x15, 0x00, 0xD0, 0x07, 0xFD,
			0x3C, 0x00, 0x64, 0x00, 0xCD, 0xC8, 0x80, 0xD0,
			0x07, 0x00, 0xC0, 0x80, 0x00, 0x10, 0x00, 0x10,
			0x00, 0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x61, 0x65,
			0x68, 0x6B, 0x6E, 0x71, 0x74, 0x77, 0x00, 0xC8,
			0x00, 0x10, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04},
	},
	{
		.version = 0x3330,
		.packrat_number = 1091743,
		.abs_x_min = 0,
		.abs_x_max = 1000,
		.abs_y_min = 0,
		.abs_y_max = 1760,
		.flags = SYNAPTICS_FLIP_Y,
		.gpio_irq = FIGHTER_TP_ATTz,
		.gpio_reset = FIGHTER_TP_RSTz,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.reduce_report_level = {65, 65, 120, 15, 15},
		.large_obj_check = 1,
		.default_config = 2,
		.tw_pin_mask = 0x0080,
		.config = {0x41, 0x30, 0x31, 0x39, 0x00, 0x3F, 0x03, 0x1E,
			0x05, 0xB1, 0x09, 0x0B, 0x01, 0x01, 0x00, 0x00,
			0xE8, 0x03, 0x75, 0x07, 0x02, 0x14, 0x1E, 0x05,
			0x28, 0xF4, 0x28, 0x1E, 0x05, 0x01, 0x3C, 0x17,
			0x01, 0x1B, 0x01, 0x66, 0x4E, 0x66, 0x46, 0x7F,
			0xBB, 0x30, 0xB7, 0x01, 0x40, 0x30, 0x30, 0x00,
			0x00, 0x0A, 0x04, 0xB2, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x19, 0x01, 0x00, 0x0A, 0x60, 0x32,
			0xE2, 0x02, 0x32, 0x0A, 0x0A, 0x96, 0x0B, 0x13,
			0x00, 0x02, 0x66, 0x01, 0x80, 0x02, 0x0E, 0x1F,
			0x15, 0x3B, 0x00, 0x19, 0x04, 0x1B, 0x00, 0x10,
			0xFF, 0x01, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
			0x18, 0x19, 0x1A, 0x1B, 0x11, 0xFF, 0xFF, 0xFF,
			0x03, 0x05, 0x07, 0x13, 0x11, 0x0F, 0x0D, 0x0B,
			0x0A, 0x09, 0x08, 0x01, 0x02, 0x04, 0x06, 0x0C,
			0x0E, 0x10, 0x12, 0xFF, 0xA0, 0xA0, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x3F, 0x3D, 0x3C, 0x3A,
			0x38, 0x37, 0x35, 0x34, 0x00, 0x03, 0x06, 0x09,
			0x0C, 0x0F, 0x12, 0x15, 0x00, 0xD0, 0x07, 0xFD,
			0x3C, 0x00, 0x64, 0x00, 0xCD, 0xC8, 0x80, 0xD0,
			0x07, 0x00, 0xC0, 0x80, 0x00, 0x10, 0x00, 0x10,
			0x00, 0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x61, 0x65,
			0x68, 0x6B, 0x6E, 0x71, 0x74, 0x77, 0x00, 0xC8,
			0x00, 0x10, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04},
	},
	{
		.version = 0x3330,
		.abs_x_min = 0,
		.abs_x_max = 1000,
		.abs_y_min = 0,
		.abs_y_max = 1760,
		.flags = SYNAPTICS_FLIP_Y,
		.gpio_irq = FIGHTER_TP_ATTz,
		.gpio_reset = FIGHTER_TP_RSTz,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.reduce_report_level = {65, 65, 120, 15, 15},
		.large_obj_check = 1,
		.default_config = 2,
		.tw_pin_mask = 0x0080,
		.config = {0x30, 0x30, 0x30, 0x33, 0x00, 0x3F, 0x03, 0x1E,
			0x05, 0xB1, 0x09, 0x0B, 0x01, 0x01, 0x00, 0x00,
			0xE8, 0x03, 0x75, 0x07, 0x02, 0x14, 0x1E, 0x05,
			0x2D, 0xD6, 0x09, 0x23, 0x02, 0x01, 0x3C, 0x29,
			0x00, 0x22, 0x00, 0x00, 0x48, 0x00, 0x40, 0x5D,
			0xBE, 0xC8, 0xB2, 0x00, 0x18, 0x20, 0x20, 0x00,
			0x00, 0x0A, 0x04, 0xB2, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x19, 0x01, 0x00, 0x0A, 0x30, 0x32,
			0xA3, 0x03, 0x32, 0x04, 0x04, 0x78, 0x0B, 0x13,
			0x00, 0x02, 0x47, 0x01, 0x80, 0x03, 0x0E, 0x1F,
			0x15, 0x2F, 0x00, 0x19, 0x04, 0x00, 0x00, 0x10,
			0xFF, 0x01, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
			0x18, 0x19, 0x1A, 0x1B, 0x11, 0xFF, 0xFF, 0xFF,
			0x03, 0x05, 0x07, 0x13, 0x11, 0x0F, 0x0D, 0x0B,
			0x0A, 0x09, 0x08, 0x01, 0x02, 0x04, 0x06, 0x0C,
			0x0E, 0x10, 0x12, 0xFF, 0xA0, 0xA0, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x3F, 0x3D, 0x3C, 0x3A,
			0x38, 0x37, 0x35, 0x34, 0x00, 0x03, 0x06, 0x09,
			0x0C, 0x0F, 0x12, 0x15, 0x00, 0xD0, 0x07, 0x01,
			0x3C, 0x00, 0x64, 0x00, 0xCD, 0xC8, 0x80, 0xD0,
			0x07, 0x00, 0xC0, 0x80, 0x00, 0x10, 0x00, 0x10,
			0x00, 0x18, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
			0x55, 0x58, 0x5A, 0x5C, 0x5E, 0x60, 0x62, 0x64,
			0x00, 0x4E, 0x00, 0x10, 0x0A, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D,
			0x04},
	},
	{
		.version = 0x3230,
		.abs_x_min = 0,
		.abs_x_max = 1000,
		.abs_y_min = 0,
		.abs_y_max = 1770,
		.flags = SYNAPTICS_FLIP_Y,
		.gpio_irq = FIGHTER_TP_ATTz,
		.default_config = 1,
		.config = {0x30, 0x30, 0x30, 0x34, 0x84, 0x0F, 0x03, 0x1E,
			0x06, 0x20, 0xB1, 0x01, 0x0B, 0x19, 0x19, 0x00,
			0x00, 0xE8, 0x03, 0x6C, 0x07, 0x1E, 0x05, 0x2D,
			0xBF, 0x10, 0xBE, 0x01, 0x01, 0x33, 0x00, 0x43,
			0xFF, 0x00, 0x48, 0x00, 0x48, 0xDD, 0xAF, 0xF7,
			0xB0, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x14,
			0x04, 0xAD, 0x00, 0x02, 0x6D, 0x01, 0x80, 0x03,
			0x0D, 0x1E, 0x00, 0x29, 0x00, 0x1C, 0x04, 0x1E,
			0x00, 0x10, 0x00, 0x01, 0x12, 0x13, 0x14, 0x15,
			0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x11, 0xFF,
			0xFF, 0xFF, 0x03, 0x05, 0x07, 0x13, 0x11, 0x0F,
			0x0D, 0x0B, 0x0A, 0x09, 0x08, 0x01, 0x02, 0x04,
			0x06, 0x0C, 0x0E, 0x10, 0x12, 0xFF, 0xA0, 0xA0,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x3F, 0x3D,
			0x3C, 0x3A, 0x38, 0x37, 0x35, 0x34, 0x00, 0x03,
			0x06, 0x09, 0x0C, 0x0F, 0x12, 0x15, 0x01, 0x40,
			0x14, 0x80, 0x40, 0x14, 0x64, 0x1A, 0xC0, 0x14,
			0xCC, 0x1A, 0x15, 0x00, 0xC0, 0x80, 0x00, 0x10,
			0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
			0x02, 0x02, 0x20, 0x30, 0x20, 0x20, 0x20, 0x20,
			0x20, 0x20, 0x51, 0x7E, 0x57, 0x5A, 0x5D, 0x60,
			0x63, 0x66, 0x33, 0x43, 0x00, 0x1E, 0x19, 0x05,
			0x00, 0xFF, 0x3D, 0x08},
	},
	{
		.version = 0x0000,
		.abs_x_min = 35,
		.abs_x_max = 965,
		.abs_y_min = 0,
		.abs_y_max = 1770,
		.sensitivity_adjust = 0,
		.finger_support = 10,
		.flags = SYNAPTICS_FLIP_Y,
		.gpio_irq = FIGHTER_TP_ATTz,
		.config = {0x30, 0x30, 0x30, 0x31, 0x84, 0x0F, 0x03, 0x1E,
			0x05, 0x01, 0x0B, 0x19, 0x19, 0x00, 0x00, 0xE8,
			0x03, 0x75, 0x07, 0x1E, 0x05, 0x28, 0xF5, 0x28,
			0x1E, 0x05, 0x01, 0x30, 0x00, 0x30, 0x00, 0x00,
			0x48, 0x00, 0x48, 0x0D, 0xD6, 0x56, 0xBE, 0x00,
			0x70, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x04, 0x00,
			0x02, 0xCD, 0x00, 0x80, 0x03, 0x0D, 0x1F, 0x00,
			0x21, 0x00, 0x15, 0x04, 0x1E, 0x00, 0x10, 0x02,
			0x01, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
			0x19, 0x1A, 0x1B, 0x11, 0xFF, 0xFF, 0xFF, 0x05,
			0x13, 0x0F, 0x0B, 0x09, 0x01, 0x04, 0x0C, 0x10,
			0x03, 0x07, 0x11, 0x0D, 0x0A, 0x08, 0x02, 0x06,
			0x0E, 0x12, 0xFF, 0xA0, 0xA0, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x3F, 0x3D, 0x3C, 0x3A, 0x38,
			0x37, 0x35, 0x34, 0x00, 0x03, 0x06, 0x09, 0x0C,
			0x0F, 0x12, 0x15, 0x00, 0x04, 0x08, 0x0C, 0x1E,
			0x14, 0x3C, 0x1E, 0x00, 0x9B, 0x7F, 0x46, 0x20,
			0x4E, 0x9B, 0x7F, 0x28, 0x80, 0xCC, 0xF4, 0x01,
			0x00, 0xC0, 0x80, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x30, 0x30, 0x00, 0x1E, 0x19,
			0x05, 0x00, 0x00, 0x3D, 0x08, 0x00, 0x00, 0x00,
			0xBC, 0x02, 0x80},
	},
};


struct i2c_board_info msm_i2c_gsbi3_info[] = {
	{
		I2C_BOARD_INFO(ATMEL_MXT224E_NAME, 0x94 >> 1),
		.platform_data = &ts_atmel_data,
		.irq = MSM_GPIO_TO_INT(FIGHTER_TP_ATTz)
	},
	{
		I2C_BOARD_INFO(SYNAPTICS_3200_NAME, 0x40 >> 1),
		.platform_data = &syn_ts_3k_data,
		.irq = MSM_GPIO_TO_INT(FIGHTER_TP_ATTz)
	},
};

static ssize_t virtual_atmel_keys_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
		__stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":62:1015:110:100"
			":" __stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":200:1015:106:100"
			":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":340:1015:120:100"
			":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":482:1015:110:100"
		"\n");
}


static ssize_t virtual_syn_keys_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
		__stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":80:1040:110:110"
			":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":270:1040:130:110"
			":" __stringify(EV_KEY) ":" __stringify(KEY_APP_SWITCH)   ":460:1040:110:110"
		"\n");
}

static struct kobj_attribute virtual_keys_atmel_attr = {
	.attr = {
		.name = "virtualkeys.atmel-touchscreen",
		.mode = S_IRUGO,
	},
	.show = &virtual_atmel_keys_show,
};

static struct kobj_attribute virtual_keys_synaptics_attr = {
	.attr = {
		.name = "virtualkeys.synaptics-rmi-touchscreen",
		.mode = S_IRUGO,
	},
	.show = &virtual_syn_keys_show,
};

static struct attribute *properties_attrs[] = {
	&virtual_keys_atmel_attr.attr,
	&virtual_keys_synaptics_attr.attr,
	NULL
};

static struct attribute_group properties_attr_group = {
	.attrs = properties_attrs,
};

#ifdef CONFIG_MSM_CAMERA_FLASH
static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
	.camera_flash = flashlight_control,
};
#endif

static uint16_t msm_cam_gpio_tbl[] = {
	FIGHTER_CAM_MCLK0, /*CAMIF_MCLK*/
	FIGHTER_CAM_MCLK1,
#if 0
	FIGHTER_CAM_I2C_SDA, /*CAMIF_I2C_DATA*/
	FIGHTER_CAM_I2C_SCL, /*CAMIF_I2C_CLK*/
#endif
};

static struct msm_camera_gpio_conf gpio_conf = {
	.cam_gpiomux_conf_tbl = NULL,
	.cam_gpiomux_conf_tbl_size = 0,
	.cam_gpio_tbl = msm_cam_gpio_tbl,
	.cam_gpio_tbl_size = ARRAY_SIZE(msm_cam_gpio_tbl),
};


static struct msm_bus_vectors cam_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_preview_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 96215040,
		.ib  = 378224640,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_video_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 342150912,
		.ib  = 1361968128,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 207747072,
		.ib  = 489756672,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 60318720,
		.ib  = 150796800,
	},
};

static struct msm_bus_vectors cam_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 147045888,
		.ib  = 588183552,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 263678976,
		.ib  = 659197440,
	},
};

static struct msm_bus_vectors cam_zsl_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 319044096,
		.ib  = 1271531520,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 239708160,
		.ib  = 599270400,
	},
};

static struct msm_bus_paths cam_bus_client_config[] = {
	{
		ARRAY_SIZE(cam_init_vectors),
		cam_init_vectors,
	},
	{
		ARRAY_SIZE(cam_preview_vectors),
		cam_preview_vectors,
	},
	{
		ARRAY_SIZE(cam_video_vectors),
		cam_video_vectors,
	},
	{
		ARRAY_SIZE(cam_snapshot_vectors),
		cam_snapshot_vectors,
	},
	{
		ARRAY_SIZE(cam_zsl_vectors),
		cam_zsl_vectors,
	},
};

static struct msm_bus_scale_pdata cam_bus_client_pdata = {
		cam_bus_client_config,
		ARRAY_SIZE(cam_bus_client_config),
		.name = "msm_camera",
};

static struct regulator *reg_8921_l2;
static struct regulator *reg_8921_l8;
static struct regulator *reg_8921_l9;
static struct regulator *reg_8921_l12;
static struct regulator *reg_8921_lvs6;

static int camera_sensor_power_enable(char *power, unsigned volt, struct regulator **sensor_power)
{
	int rc;

	if (power == NULL)
		return -ENODEV;

	*sensor_power = regulator_get(NULL, power);

	if (IS_ERR(*sensor_power)) {
		pr_err("[CAM] %s: Unable to get %s\n", __func__, power);
		return -ENODEV;
	}

	if (volt != 1800000) {
		rc = regulator_set_voltage(*sensor_power, volt, volt);
		if (rc < 0) {
			pr_err("[CAM] %s: unable to set %s voltage to %d rc:%d\n",
					__func__, power, volt, rc);
			regulator_put(*sensor_power);
			*sensor_power = NULL;
			return -ENODEV;
		}
	}

	rc = regulator_enable(*sensor_power);
	if (rc < 0) {
		pr_err("[CAM] %s: Enable regulator %s failed\n", __func__, power);
		regulator_put(*sensor_power);
		*sensor_power = NULL;
		return -ENODEV;
	}

	return rc;
}

static int camera_sensor_power_disable(struct regulator *sensor_power)
{

	int rc;
	if (sensor_power == NULL)
		return -ENODEV;

	if (IS_ERR(sensor_power)) {
		pr_err("[CAM] %s: Invalid requlator ptr\n", __func__);
		return -ENODEV;
	}

	rc = regulator_disable(sensor_power);
	if (rc < 0)
		pr_err("[CAM] %s: disable regulator failed\n", __func__);

	regulator_put(sensor_power);
	sensor_power = NULL;
	return rc;
}

static int fighter_csi_vreg_on(void)
{
	pr_info("%s\n", __func__);
	return camera_sensor_power_enable("8921_l2", 1200000, &reg_8921_l2);
}

static int fighter_csi_vreg_off(void)
{
	pr_info("%s\n", __func__);
	return camera_sensor_power_disable(reg_8921_l2);
}

struct msm_camera_device_platform_data msm_camera_csi_device_data[] = {
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 0,
		.camera_csi_on = fighter_csi_vreg_on,
		.camera_csi_off = fighter_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
	{
		.ioclk.mclk_clk_rate = 24000000,
		.ioclk.vfe_clk_rate  = 228570000,
		.csid_core = 1,
		.camera_csi_on = fighter_csi_vreg_on,
		.camera_csi_off = fighter_csi_vreg_off,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
};

#ifdef CONFIG_S5K3H2YX
static int fighter_s5k3h2yx_vreg_on(void)
{
	int rc;
	pr_info("[CAM] %s\n", __func__);

	/* VCM */
	rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}

	/* IO */
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8921_lvs6\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}

	/* analog */
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}

	udelay(50);

	/* digital */
	if (system_rev >= 2) {	/* fighter use LDO by GPIO95 on XC */
		rc = gpio_request(FIGHTER_CAM_EXT_LDO, "CAM_EXT_LDO");
		if (rc < 0)
			pr_err("[CAM] %s: gpio %d request failed, rc=%d\n", __func__,
				FIGHTER_CAM_EXT_LDO, rc);
		else {
			gpio_direction_output(FIGHTER_CAM_EXT_LDO, 1);
			gpio_free(FIGHTER_CAM_EXT_LDO);
		}
	}
	else
		rc = camera_sensor_power_enable("8921_l12", 1200000, &reg_8921_l12);
	if (rc < 0) {
		pr_err("[CAM] sensor_power_enable(\"8921_l12\", 1.5) FAILED %d\n", rc);
		goto enable_digital_fail;
	}

	return rc;

enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_lvs6);
enable_io_fail:
	camera_sensor_power_disable(reg_8921_l9);
enable_vcm_fail:
	return rc;
}

static int fighter_s5k3h2yx_vreg_off(void)
{
	int rc = 0;

	pr_info("[CAM] %s\n", __func__);

	/* VCM */
	rc = camera_sensor_power_disable(reg_8921_l9);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable(\"8921_l9\") FAILED %d\n", rc);

	/* digital */
	if (system_rev >= 2) {	/* fighter use LDO by GPIO95 on XC */
		rc = gpio_request(FIGHTER_CAM_EXT_LDO, "CAM_EXT_LDO");
		if (rc < 0)
			pr_err("[CAM] %s: gpio %d request failed, rc=%d\n", __func__,
				FIGHTER_CAM_EXT_LDO, rc);
		else {
			gpio_direction_output(FIGHTER_CAM_EXT_LDO, 0);
			gpio_free(FIGHTER_CAM_EXT_LDO);
		}
	}
	else
		rc = camera_sensor_power_disable(reg_8921_l12);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable(\"8921_l12\") FAILED %d\n", rc);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable(\"8921_l8\") FAILED %d\n", rc);

	udelay(50);

	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs6);
	if (rc < 0)
		pr_err("[CAM] sensor_power_disable(\"8921_lvs6\") FAILED %d\n", rc);

	return rc;
}

#ifdef CONFIG_S5K3H2YX_ACT
static struct i2c_board_info s5k3h2yx_actuator_i2c_info = {
	I2C_BOARD_INFO("s5k3h2yx_act", 0x11),
};

static struct msm_actuator_info s5k3h2yx_actuator_info = {
	.board_info     = &s5k3h2yx_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = FIGHTER_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif

static struct msm_camera_sensor_platform_info sensor_s5k3h2yx_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_MIRROR_FLIP,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= FIGHTER_CAM_PWDN,
	.vcm_pwd	= FIGHTER_CAM_VCM_PD,
	.vcm_enable	= 1,
};

/* Andrew_Cheng linear led 20111205 MB */
static struct camera_led_est msm_camera_sensor_s5k3h2yx_led_table[] = {
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL1,
		.current_ma = 150,
		.lumen_value = 150,
		.min_step = 50,
		.max_step = 70
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,//245,//240,   //mk0118
		.min_step = 44,
		.max_step = 52
	},
		{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL3,
		.current_ma = 300,
		.lumen_value = 300,
		.min_step = 29,
		.max_step = 34
	},
		{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL4,
		.current_ma = 400,
		.lumen_value = 400,
		.min_step = 27,
		.max_step = 28
	},
		{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL5,
		.current_ma = 500,
		.lumen_value = 500,
		.min_step = 25,
		.max_step = 26
	},
		{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL6,
		.current_ma = 600,
		.lumen_value = 600,
		.min_step = 23,
		.max_step = 24
	},
		{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL7,
		.current_ma = 700,
		.lumen_value = 700,
		.min_step = 21,
		.max_step = 22
	},
		{
		.enable = 1,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 750,//740,//725,   //mk0118
		.min_step = 0,
		.max_step = 43
	},

		{
		.enable = 2,
		.led_state = FL_MODE_FLASH_LEVEL2,
		.current_ma = 200,
		.lumen_value = 250,//245,
		.min_step = 0,
		.max_step = 270
	},
		{
		.enable = 0,
		.led_state = FL_MODE_OFF,
		.current_ma = 0,
		.lumen_value = 0,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 0,
		.led_state = FL_MODE_TORCH,
		.current_ma = 150,
		.lumen_value = 150,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 2,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 750,//740,//725,
		.min_step = 271,
		.max_step = 325
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH_LEVEL5,
		.current_ma = 500,
		.lumen_value = 500,
		.min_step = 25,
		.max_step = 26
	},
	{
		.enable = 3,
		.led_state = FL_MODE_FLASH,
		.current_ma = 750,
		.lumen_value = 750,//740,//725,
		.min_step = 271,
		.max_step = 325
	},
	{
		.enable = 0,
		.led_state = FL_MODE_TORCH_LEVEL_2,
		.current_ma = 200,
		.lumen_value = 75,
		.min_step = 0,
		.max_step = 40
	},};

static struct camera_led_info msm_camera_sensor_s5k3h2yx_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 800,
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_s5k3h2yx_led_table),
};

static struct camera_flash_info msm_camera_sensor_s5k3h2yx_flash_info = {
	.led_info = &msm_camera_sensor_s5k3h2yx_led_info,
	.led_est_table = msm_camera_sensor_s5k3h2yx_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_s5k3h2yx_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 15,
	.flash_info             = &msm_camera_sensor_s5k3h2yx_flash_info,
};
/* Andrew_Cheng linear led 20111205 ME */

static struct msm_camera_sensor_flash_data flash_s5k3h2yx = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_flash_src,
#endif
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3h2yx_data = {
	.sensor_name	= "s5k3h2yx",
	.camera_power_on = fighter_s5k3h2yx_vreg_on,
	.camera_power_off = fighter_s5k3h2yx_vreg_off,
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_s5k3h2yx,
	.sensor_platform_info = &sensor_s5k3h2yx_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
#ifdef CONFIG_S5K3H2YX_ACT
	.actuator_info = &s5k3h2yx_actuator_info,
#endif
	.use_rawchip = 0,
	.flash_cfg = &msm_camera_sensor_s5k3h2yx_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};

struct platform_device fighter_camera_sensor_s5k3h2yx = {
	.name	= "msm_camera_s5k3h2yx",
	.dev	= {
		.platform_data = &msm_camera_sensor_s5k3h2yx_data,
	},
};
#endif

//HTC start Tom Lin 2011/12/19
#ifdef CONFIG_IMX105
static int fighter_imx105_vreg_on(void)
{
	int rc;
	pr_info("%s\n", __func__);

	/* VCM */
	rc = camera_sensor_power_enable("8921_l9", 2800000, &reg_8921_l9);//based on 8921_l9 definition needs to be between 2.8 to 2.85v
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_l9\", 2.8V) FAILED %d\n", rc);
		goto enable_vcm_fail;
	}

       pr_info("imx105 VCM on\n");


	/* analog */
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);//based on 8921_l9 definition needs to be between 2.8 to 2.85v
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_l8\", 2.8V) FAILED %d\n", rc);
		goto enable_analog_fail;
	}	   

       pr_info("imx105 A on\n");

	udelay(50);

	/* digital */
	if (system_rev >= 2) {	/* fighter use LDO by GPIO95 on XC */
		pr_err("system_rev >=2:vreg on");
		rc = gpio_request(FIGHTER_CAM_EXT_LDO, "CAM_EXT_LDO");
		if (rc < 0)
			pr_err(" %s: gpio %d request failed, rc=%d\n", __func__,
				FIGHTER_CAM_EXT_LDO, rc);
		else {
			gpio_direction_output(FIGHTER_CAM_EXT_LDO, 1);
			gpio_free(FIGHTER_CAM_EXT_LDO);
		}
	}
	else{
		pr_err("system_rev <2:vreg on");
		rc = camera_sensor_power_enable("8921_l12", 1260000, &reg_8921_l12);/*Tom 20120224 increase to 1.26v for 876MHz*/
	}

	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_l12\", 1.2V) FAILED %d\n", rc);
		goto enable_digital_fail;
	}

      pr_info("imx105 D on\n");
	/* IO */
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
	if (rc < 0) {
		pr_err("sensor_power_enable(\"8921_lvs6\", 1.8V) FAILED %d\n", rc);
		goto enable_io_fail;
	}
	
        pr_info("imx105 IO on\n");

	return rc;

enable_digital_fail:
	camera_sensor_power_disable(reg_8921_l12);
enable_analog_fail:
	camera_sensor_power_disable(reg_8921_l8);
enable_io_fail:
	camera_sensor_power_disable(reg_8921_lvs6);
enable_vcm_fail:
	camera_sensor_power_disable(reg_8921_l9);
       return rc;
}

static int fighter_imx105_vreg_off(void)
{
	int rc = 0;

	pr_info("%s\n", __func__);

	/* IO */
	rc = camera_sensor_power_disable(reg_8921_lvs6);
        pr_info("imx105 IO off\n");
	if (rc < 0)
		pr_err("sensor_power_disable(\"8921_lvs6\") FAILED %d\n", rc);
	
	/* digital */
	if (system_rev >= 2) {	/* fighter use LDO by GPIO95 on XC */
		pr_err("system_rev >=2:vreg off");
		rc = gpio_request(FIGHTER_CAM_EXT_LDO, "CAM_EXT_LDO");
		if (rc < 0)
			pr_err("%s: gpio %d request failed, rc=%d\n", __func__,
				FIGHTER_CAM_EXT_LDO, rc);
		else {
			gpio_direction_output(FIGHTER_CAM_EXT_LDO, 0);
			gpio_free(FIGHTER_CAM_EXT_LDO);
		}
	}
	else{
              pr_err("system_rev <2:vreg off");
	      rc = camera_sensor_power_disable(reg_8921_l12);
             pr_info("imx105 D off\n");
	}
	if (rc < 0)
		pr_err("sensor_power_disable(\"8921_l12\") FAILED %d\n", rc);	

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
       pr_info("imx105 A off\n");
	if (rc < 0)
		pr_err("sensor_power_disable(\"8921_l8\") FAILED %d\n", rc);	

	udelay(50);

	/* VCM */
	rc = camera_sensor_power_disable(reg_8921_l9);
       pr_info("imx105 VCM off\n");
	if (rc < 0)
		pr_err("sensor_power_disable(\"8921_l9\") FAILED %d\n", rc);

	return rc;
}

#ifdef CONFIG_IMX105_ACT
static struct i2c_board_info imx105_actuator_i2c_info = {
	I2C_BOARD_INFO("imx105_act", 0x1B),
};

static struct msm_actuator_info imx105_actuator_info = {
	.board_info     = &imx105_actuator_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = FIGHTER_CAM_VCM_PD,
	.vcm_enable     = 1,
};
#endif

static struct msm_camera_sensor_platform_info sensor_imx105_board_info = {
	.mount_angle = 90,
	.mirror_flip = CAMERA_SENSOR_NONE,
	.sensor_reset_enable = 0,
	.sensor_reset	= 0,
	.sensor_pwd	= FIGHTER_CAM_PWDN,
	.vcm_pwd	= FIGHTER_CAM_VCM_PD,
	.vcm_enable	= 1,
};

/* Andrew_Cheng linear led 20111205 MB */
static struct camera_led_est msm_camera_sensor_imx105_led_table[] = {
	{
		.enable = 0,
		.led_state = FL_MODE_OFF,
		.current_ma = 0,
		.lumen_value = 0,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 0,
		.led_state = FL_MODE_TORCH,
		.current_ma = 150,
		.lumen_value = 150,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH,
		.current_ma = 800,
		.lumen_value = 800,
		.min_step = 33,
		.max_step = 42
	},
	{
		.enable = 0,
		.led_state = FL_MODE_PRE_FLASH,
		.current_ma = 100,
		.lumen_value = 100,
		.min_step = 0,
		.max_step = 0
	},
	{
		.enable = 0,
		.led_state = FL_MODE_FLASH,
		.current_ma = 800,
		.lumen_value = 800,
		.min_step = 40,
		.max_step = 52
	},
	{
		.enable = 0,
		.led_state = FL_MODE_TORCH_LEVEL_2,
		.current_ma = 200,
		.lumen_value = 264,
		.min_step = 0,
		.max_step = 39
	},};

static struct camera_led_info msm_camera_sensor_imx105_led_info = {
	.enable = 1,
	.low_limit_led_state = FL_MODE_TORCH,
	.max_led_current_ma = 800,
	.num_led_est_table = ARRAY_SIZE(msm_camera_sensor_imx105_led_table),
};

static struct camera_flash_info msm_camera_sensor_imx105_flash_info = {
	.led_info = &msm_camera_sensor_imx105_led_info,
	.led_est_table = msm_camera_sensor_imx105_led_table,
};

static struct camera_flash_cfg msm_camera_sensor_imx105_flash_cfg = {
	.low_temp_limit		= 5,
	.low_cap_limit		= 15,
	.flash_info             = &msm_camera_sensor_imx105_flash_info,
};
/* Andrew_Cheng linear led 20111205 ME */

static struct msm_camera_sensor_flash_data flash_imx105 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_flash_src,
#endif
};

static struct msm_camera_sensor_info msm_camera_sensor_imx105_data = {
	.sensor_name	= "imx105",
	.camera_power_on = fighter_imx105_vreg_on,
	.camera_power_off = fighter_imx105_vreg_off,
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx105,
	.sensor_platform_info = &sensor_imx105_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
#ifdef CONFIG_IMX105_ACT
	.actuator_info = &imx105_actuator_info,
#endif
	.use_rawchip = 0,
	.flash_cfg = &msm_camera_sensor_imx105_flash_cfg, /* Andrew_Cheng linear led 20111205 */
};

struct platform_device fighter_camera_sensor_imx105 = {
	.name	= "msm_camera_imx105",
	.dev	= {
		.platform_data = &msm_camera_sensor_imx105_data,
	},
};
#endif
//HTC end Tom Lin 2011/12/19

#ifdef CONFIG_MT9V113
static int fighter_mt9v113_vreg_on(void)
{
	int rc;

	pr_info("[CAM] %s\n", __func__);

	/* IO */
	rc = gpio_request(FIGHTER_V_CAM2_D1V8_EN, "CAM_D1V8_EN");
	pr_info("[CAM] fighter_mt9v113_vreg_on %d 1v8\n", FIGHTER_V_CAM2_D1V8_EN);
	if (rc) {
		pr_err("[CAM] %s:GPIO_CAM_D1V8_EN gpio %d request failed, rc=%d\n", __func__,  FIGHTER_V_CAM2_D1V8_EN, rc);
		goto init_fail;
	}
	gpio_direction_output(FIGHTER_V_CAM2_D1V8_EN, 1);
	gpio_free(FIGHTER_V_CAM2_D1V8_EN);

	udelay(50);

	/* Reset */
	rc = gpio_request(FIGHTER_CAM2_RSTz, "mt9v113");
	if (!rc) {
		gpio_direction_output(FIGHTER_CAM2_RSTz, 1);
		msleep(2);
	}	else
		pr_err("[CAM] %s:FIGHTER_CAM2_RSTz gpio %d request failed, rc=%d\n", __func__,  FIGHTER_CAM2_RSTz, rc);
	gpio_free(FIGHTER_CAM2_RSTz);

	udelay(50);

	/* analog */
	rc = camera_sensor_power_enable("8921_l8", 2800000, &reg_8921_l8);
	pr_info("[CAM] sensor_power_enable(\"8921_l8\", 2.8V) == %d\n", rc);

	if (rc < 0)
		goto init_fail;

	udelay(50);

	/* digital */
	rc = camera_sensor_power_enable("8921_lvs6", 1800000, &reg_8921_lvs6);
	pr_info("[CAM] sensor_power_enable(\"8921_lvs6\", 1.8V) == %d\n", rc);

	if (rc < 0)
		goto init_fail;

	udelay(50);

init_fail:
	return rc;
}

static int fighter_mt9v113_vreg_off(void)
{
	int rc;

	pr_info("[CAM] %s\n", __func__);

	/* digital */
	rc = camera_sensor_power_disable(reg_8921_lvs6);
	pr_info("[CAM] camera_sensor_power_disable(\"8921_lvs6\", 1.8V) == %d\n", rc);

	if (rc < 0)
		goto init_fail;

	udelay(50);

	/* analog */
	rc = camera_sensor_power_disable(reg_8921_l8);
	pr_info("[CAM] camera_sensor_power_disable(\"8921_l8\", 2.8V) == %d\n", rc);

	if (rc < 0)
		goto init_fail;

	udelay(50);

	/* IO */
	rc = gpio_request(FIGHTER_V_CAM2_D1V8_EN, "CAM_D1V8_EN");
	pr_info("[CAM] fighter_mt9v113_vreg_off %d 1v8\n", FIGHTER_V_CAM2_D1V8_EN);
	if (rc) {
		pr_err("[CAM] %s:GPIO_CAM_D1V8_EN gpio %d request failed, rc=%d\n", __func__,  FIGHTER_V_CAM2_D1V8_EN, rc);
		goto init_fail;
	}
	gpio_direction_output(FIGHTER_V_CAM2_D1V8_EN, 0);
	gpio_free(FIGHTER_V_CAM2_D1V8_EN);

	udelay(50);

	/* Reset */
	rc = gpio_request(FIGHTER_CAM2_RSTz, "mt9v113");
	if (!rc) {
		gpio_direction_output(FIGHTER_CAM2_RSTz, 0);
		msleep(2);
	} else
		pr_err("[CAM] %s:FIGHTER_CAM2_RSTz gpio %d request failed, rc=%d\n", __func__,	FIGHTER_CAM2_RSTz, rc);
	gpio_free(FIGHTER_CAM2_RSTz);

	udelay(50);

init_fail:
		return rc;
}

static struct msm_camera_sensor_platform_info sensor_mt9v113_board_info = {
	.mount_angle = 270,
	.mirror_flip = CAMERA_SENSOR_NONE,
};

static struct msm_camera_sensor_flash_data flash_mt9v113 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9v113_data = {
	.sensor_name	= "mt9v113",
	.sensor_reset	= FIGHTER_CAM2_RSTz,
	.sensor_pwd	= FIGHTER_CAM_PWDN,
	.vcm_pwd	= 0,
	.vcm_enable	= 1,
	.camera_power_on = fighter_mt9v113_vreg_on,
	.camera_power_off = fighter_mt9v113_vreg_off,
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_mt9v113,
	.sensor_platform_info = &sensor_mt9v113_board_info,
	.gpio_conf = &gpio_conf,
	.csi_if	= 1,
	.use_rawchip = 0,
};

struct platform_device fighter_camera_sensor_mt9v113 = {
	.name	= "msm_camera_mt9v113",
	.dev	= {
		.platform_data = &msm_camera_sensor_mt9v113_data,
	},
};
#endif


static void __init msm8960_init_cam(void)
{
	int i;
	struct platform_device *cam_dev[] = {
#ifdef CONFIG_S5K3H2YX
		&fighter_camera_sensor_s5k3h2yx,
#endif
//HTC start Tom Lin 2011/12/19
#ifdef CONFIG_IMX105
		&fighter_camera_sensor_imx105,
#endif
//HTC end Tom Lin 2011/12/19
#ifdef CONFIG_MT9V113
	&fighter_camera_sensor_mt9v113,
#endif
	};

	for (i = 0; i < ARRAY_SIZE(cam_dev); i++) {
		struct msm_camera_sensor_info *s_info;
		s_info = cam_dev[i]->dev.platform_data;
		msm_get_cam_resources(s_info);
		platform_device_register(cam_dev[i]);
	}
	platform_device_register(&msm8960_device_csiphy0);
	platform_device_register(&msm8960_device_csiphy1);
	platform_device_register(&msm8960_device_csid0);
	platform_device_register(&msm8960_device_csid1);
	platform_device_register(&msm8960_device_ispif);
	platform_device_register(&msm8960_device_vfe);
	platform_device_register(&msm8960_device_vpe);
}

#if 0
static bool dsi_power_on;

/**
 * LiQUID panel on/off
 *
 * @param on
 *
 * @return int
 */
static int mipi_dsi_liquid_panel_power(int on)
{
	static struct regulator *reg_l2, *reg_ext_3p3v;
	static int gpio21, gpio24, gpio43;
	int rc;

	pr_info("%s: on=%d\n", __func__, on);

	gpio21 = PM8921_GPIO_PM_TO_SYS(21); /* disp power enable_n */
	gpio43 = PM8921_GPIO_PM_TO_SYS(43); /* Displays Enable (rst_n)*/
	gpio24 = PM8921_GPIO_PM_TO_SYS(24); /* Backlight PWM */

	if (!dsi_power_on) {

		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdda");
		if (IS_ERR(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}

		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		reg_ext_3p3v = regulator_get(&msm_mipi_dsi1_device.dev,
			"vdd_lvds_3p3v");
		if (IS_ERR(reg_ext_3p3v)) {
			pr_err("could not get reg_ext_3p3v, rc = %ld\n",
			       PTR_ERR(reg_ext_3p3v));
		    return -ENODEV;
		}

		rc = gpio_request(gpio21, "disp_pwr_en_n");
		if (rc) {
			pr_err("request gpio 21 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = gpio_request(gpio43, "disp_rst_n");
		if (rc) {
			pr_err("request gpio 43 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = gpio_request(gpio24, "disp_backlight_pwm");
		if (rc) {
			pr_err("request gpio 24 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		dsi_power_on = true;
	}

	if (on) {
		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_enable(reg_ext_3p3v);
		if (rc) {
			pr_err("enable reg_ext_3p3v failed, rc=%d\n", rc);
			return -ENODEV;
		}

		/* set reset pin before power enable */
		gpio_set_value_cansleep(gpio43, 0); /* disp disable (resx=0) */

		gpio_set_value_cansleep(gpio21, 0); /* disp power enable_n */
		msleep(20);
		gpio_set_value_cansleep(gpio43, 1); /* disp enable */
		msleep(20);
		gpio_set_value_cansleep(gpio43, 0); /* disp enable */
		msleep(20);
		gpio_set_value_cansleep(gpio43, 1); /* disp enable */
		msleep(20);
	} else {
		gpio_set_value_cansleep(gpio43, 0);
		gpio_set_value_cansleep(gpio21, 1);

		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_ext_3p3v);
		if (rc) {
			pr_err("disable reg_ext_3p3v failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_l2, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
	}

	return 0;
}

static int mipi_dsi_cdp_panel_power(int on)
{
	static struct regulator *reg_l8, *reg_l23, *reg_l2;
	static int gpio43;
	int rc;

	pr_info("%s: state : %d\n", __func__, on);

	if (!dsi_power_on) {

		reg_l8 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdc");
		if (IS_ERR(reg_l8)) {
			pr_err("could not get 8921_l8, rc = %ld\n",
				PTR_ERR(reg_l8));
			return -ENODEV;
		}
		reg_l23 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vddio");
		if (IS_ERR(reg_l23)) {
			pr_err("could not get 8921_l23, rc = %ld\n",
				PTR_ERR(reg_l23));
			return -ENODEV;
		}
		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdda");
		if (IS_ERR(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_l8, 2800000, 3000000);
		if (rc) {
			pr_err("set_voltage l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_voltage(reg_l23, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		gpio43 = PM8921_GPIO_PM_TO_SYS(43);
		rc = gpio_request(gpio43, "disp_rst_n");
		if (rc) {
			pr_err("request gpio 43 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		dsi_power_on = true;
	}
	if (on) {
		rc = regulator_set_optimum_mode(reg_l8, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l23, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_l8);
		if (rc) {
			pr_err("enable l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_enable(reg_l23);
		if (rc) {
			pr_err("enable l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		gpio_set_value_cansleep(gpio43, 1);
	} else {
		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_l8);
		if (rc) {
			pr_err("disable reg_l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_l23);
		if (rc) {
			pr_err("disable reg_l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_l8, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l23, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l2, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		gpio_set_value_cansleep(gpio43, 0);
	}
	return 0;
}
#endif

#if 0
static int mipi_dsi_panel_power(int on)
{
	int ret;

	pr_info("%s: on=%d\n", __func__, on);

	if (machine_is_msm8960_liquid())
		ret = mipi_dsi_liquid_panel_power(on);
	else
		ret = mipi_dsi_cdp_panel_power(on);

	return ret;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio = MDP_VSYNC_GPIO,
	.dsi_power_save = mipi_dsi_panel_power,
};
#endif

#ifdef CONFIG_MSM_BUS_SCALING

static struct msm_bus_vectors rotator_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors rotator_ui_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = (1024 * 600 * 4 * 2 * 60),
		.ib  = (1024 * 600 * 4 * 2 * 60 * 1.5),
	},
};

static struct msm_bus_vectors rotator_vga_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = (640 * 480 * 2 * 2 * 30),
		.ib  = (640 * 480 * 2 * 2 * 30 * 1.5),
	},
};
static struct msm_bus_vectors rotator_720p_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = (1280 * 736 * 2 * 2 * 30),
		.ib  = (1280 * 736 * 2 * 2 * 30 * 1.5),
	},
};

static struct msm_bus_vectors rotator_1080p_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = (1920 * 1088 * 2 * 2 * 30),
		.ib  = (1920 * 1088 * 2 * 2 * 30 * 1.5),
	},
};

static struct msm_bus_paths rotator_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(rotator_init_vectors),
		rotator_init_vectors,
	},
	{
		ARRAY_SIZE(rotator_ui_vectors),
		rotator_ui_vectors,
	},
	{
		ARRAY_SIZE(rotator_vga_vectors),
		rotator_vga_vectors,
	},
	{
		ARRAY_SIZE(rotator_720p_vectors),
		rotator_720p_vectors,
	},
	{
		ARRAY_SIZE(rotator_1080p_vectors),
		rotator_1080p_vectors,
	},
};

struct msm_bus_scale_pdata rotator_bus_scale_pdata = {
	rotator_bus_scale_usecases,
	ARRAY_SIZE(rotator_bus_scale_usecases),
	.name = "rotator",
};

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

#endif

int mdp_core_clk_rate_table[] = {
	85330000,
	85330000,
	160000000,
	200000000,
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = MDP_VSYNC_GPIO,
	.mdp_core_clk_rate = 85330000,
	.mdp_core_clk_table = mdp_core_clk_rate_table,
	.num_mdp_clk = ARRAY_SIZE(mdp_core_clk_rate_table),
#ifdef CONFIG_MSM_BUS_SCALING
	.mdp_bus_scale_table = &mdp_bus_scale_pdata,
#endif
	.mdp_rev = MDP_REV_42,
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	.mem_hid = BIT(ION_CP_MM_HEAP_ID),
#else
	.mem_hid = MEMTYPE_EBI1,
#endif
};

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

#if 0
#define LPM_CHANNEL0 0
static int toshiba_gpio[] = {LPM_CHANNEL0};

static struct mipi_dsi_panel_platform_data toshiba_pdata = {
	.gpio = toshiba_gpio,
};

static struct platform_device mipi_dsi_toshiba_panel_device = {
	.name = "mipi_toshiba",
	.id = 0,
	.dev = {
		.platform_data = &toshiba_pdata,
	}
};

static int dsi2lvds_gpio[2] = {
	0,/* Backlight PWM-ID=0 for PMIC-GPIO#24 */
	0x1F08 /* DSI2LVDS Bridge GPIO Output, mask=0x1f, out=0x08 */
	};

static struct msm_panel_common_pdata mipi_dsi2lvds_pdata = {
	.gpio_num = dsi2lvds_gpio,
};

static struct mipi_dsi_phy_ctrl dsi_novatek_cmd_mode_phy_db = {

/* DSI_BIT_CLK at 500MHz, 2 lane, RGB888 */
	{0x0F, 0x0a, 0x04, 0x00, 0x20},	/* regulator */
	/* timing   */
	{0xab, 0x8a, 0x18, 0x00, 0x92, 0x97, 0x1b, 0x8c,
	0x0c, 0x03, 0x04, 0xa0},
	{0x5f, 0x00, 0x00, 0x10},	/* phy ctrl */
	{0xff, 0x00, 0x06, 0x00},	/* strength */
	/* pll control */
	{0x40, 0xf9, 0x30, 0xda, 0x00, 0x40, 0x03, 0x62,
	0x40, 0x07, 0x03,
	0x00, 0x1a, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01},
};

static struct mipi_dsi_panel_platform_data novatek_pdata = {
	.phy_ctrl_settings = &dsi_novatek_cmd_mode_phy_db,
};

static struct platform_device mipi_dsi_novatek_panel_device = {
	.name = "mipi_novatek",
	.id = 0,
	.dev = {
		.platform_data = &novatek_pdata,
	}
};

static struct platform_device mipi_dsi2lvds_bridge_device = {
	.name = "mipi_tc358764",
	.id = 0,
	.dev.platform_data = &mipi_dsi2lvds_pdata,
};
#endif

static uint32_t headset_gpio_xc[] = {
	GPIO_CFG(FIGHTER_HS_TX_CPU, 0, GPIO_CFG_OUTPUT,
		 GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(FIGHTER_HS_RX_CPU, 0, GPIO_CFG_INPUT,
		 GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(FIGHTER_HS_MIC_BIAS_EN, 0, GPIO_CFG_OUTPUT,
		 GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

struct pm8xxx_gpio_init headset_pmic_gpio[] = {
	PM8XXX_GPIO_INIT(FIGHTER_HS_RX_PMIC_REMO, PM_GPIO_DIR_IN,
			 PM_GPIO_OUT_BUF_CMOS, 0, PM_GPIO_PULL_NO,
			 PM_GPIO_VIN_L17, PM_GPIO_STRENGTH_LOW,
			 PM_GPIO_FUNC_PAIRED, 0, 0),
};

struct pm8xxx_gpio_init headset_pmic_gpio_rx_xc[] = {
	PM8XXX_GPIO_INIT(FIGHTER_HS_TX_PMIC_UART, PM_GPIO_DIR_IN,
			 PM_GPIO_OUT_BUF_CMOS, 0, PM_GPIO_PULL_DN,
			 PM_GPIO_VIN_S4, PM_GPIO_STRENGTH_LOW,
			 PM_GPIO_FUNC_NORMAL, 0, 0),
	PM8XXX_GPIO_INIT(FIGHTER_HS_TX_PMIC_REMO, PM_GPIO_DIR_IN,
			 PM_GPIO_OUT_BUF_CMOS, 0, PM_GPIO_PULL_NO,
			 PM_GPIO_VIN_L17, PM_GPIO_STRENGTH_LOW,
			 PM_GPIO_FUNC_NORMAL, 0, 0),
	PM8XXX_GPIO_INIT(FIGHTER_HS_RX_PMIC_REMO, PM_GPIO_DIR_IN,
			 PM_GPIO_OUT_BUF_CMOS, 0, PM_GPIO_PULL_NO,
			 PM_GPIO_VIN_L17, PM_GPIO_STRENGTH_LOW,
			 PM_GPIO_FUNC_NORMAL, 0, 0),
	PM8XXX_GPIO_INIT(FIGHTER_HS_RX_PMIC_UART, PM_GPIO_DIR_IN,
			 PM_GPIO_OUT_BUF_CMOS, 0, PM_GPIO_PULL_DN,
			 PM_GPIO_VIN_S4, PM_GPIO_STRENGTH_LOW,
			 PM_GPIO_FUNC_NORMAL, 0, 0),
};

struct pm8xxx_gpio_init headset_pmic_gpio_rx_xd[] = {
	PM8XXX_GPIO_INIT(FIGHTER_LS_EN, PM_GPIO_DIR_OUT,
			 PM_GPIO_OUT_BUF_CMOS, 0, PM_GPIO_PULL_NO,
			 PM_GPIO_VIN_S4, PM_GPIO_STRENGTH_HIGH,
			 PM_GPIO_FUNC_NORMAL, 0, 0),
};

static void headset_init(void)
{
	int i = 0;
	int rc = 0;

	pr_info("[HS_BOARD] (%s) Headset initiation\n", __func__);

	/* XA, XB */
	if (system_rev < 2) {
		rc = pm8xxx_gpio_config(headset_pmic_gpio[0].gpio,
					&headset_pmic_gpio[0].config);
		if (rc)
			pr_info("[HS_BOARD] %s: Config ERROR: GPIO=%u, rc=%d\n",
				__func__, headset_pmic_gpio[0].gpio, rc);
		return;
	}

	/* XC */
	gpio_tlmm_config(headset_gpio_xc[0], GPIO_CFG_ENABLE);
	gpio_tlmm_config(headset_gpio_xc[1], GPIO_CFG_ENABLE);
	gpio_tlmm_config(headset_gpio_xc[2], GPIO_CFG_ENABLE);
	gpio_set_value(FIGHTER_HS_TX_CPU, 0);
	gpio_set_value(FIGHTER_HS_MIC_BIAS_EN, 0);

	for (i = 0; i < ARRAY_SIZE(headset_pmic_gpio_rx_xc); i++) {
		rc = pm8xxx_gpio_config(headset_pmic_gpio_rx_xc[i].gpio,
					&headset_pmic_gpio_rx_xc[i].config);
		if (rc)
			pr_info("[HS_BOARD] %s: Config ERROR: GPIO=%u, rc=%d\n",
				__func__, headset_pmic_gpio_rx_xc[i].gpio, rc);
	}

	if (system_rev > 2) {
		for (i = 0; i < ARRAY_SIZE(headset_pmic_gpio_rx_xd); i++) {
			rc = pm8xxx_gpio_config(headset_pmic_gpio_rx_xd[i].gpio,
					&headset_pmic_gpio_rx_xd[i].config);
			if (rc)
				pr_info("[HS_BOARD] %s: ERR: GPIO=%u, rc=%d\n",
					__func__,
					headset_pmic_gpio_rx_xd[i].gpio, rc);
		}
		gpio_set_value(PM8921_GPIO_PM_TO_SYS(FIGHTER_LS_EN), 1);
	}
}

static void headset_power(int enable)
{
	if (system_rev < 2) /* XA, XB */
		return;

	pr_info("[HS_BOARD] (%s) Set MIC bias %d\n", __func__, enable);

	if (enable)
		gpio_set_value(FIGHTER_HS_MIC_BIAS_EN, 1);
	else
		gpio_set_value(FIGHTER_HS_MIC_BIAS_EN, 0);
}

/* HTC_HEADSET_PMIC Driver */
static struct htc_headset_pmic_platform_data htc_headset_pmic_data = {
	.driver_flag		= DRIVER_HS_PMIC_ADC,
	.hpin_gpio		= PM8921_GPIO_PM_TO_SYS(FIGHTER_EARPHONE_DETz),
	.hpin_irq		= 0,
	.key_gpio		= PM8921_GPIO_PM_TO_SYS(
					FIGHTER_HS_RX_PMIC_REMO),
	.key_irq		= 0,
	.key_enable_gpio	= 0,
	.adc_mic		= 0,
	.adc_remote		= {0, 57, 58, 147, 148, 339},
	.adc_mpp		= PM8XXX_AMUX_MPP_10,
	.adc_amux		= ADC_MPP_1_AMUX6,
	.hs_controller		= 0,
	.hs_switch		= 0,
};

static struct platform_device htc_headset_pmic = {
	.name	= "HTC_HEADSET_PMIC",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_pmic_data,
	},
};

/* HTC_HEADSET_MGR Driver */
static struct platform_device *headset_devices[] = {
	&htc_headset_pmic,
	/* Please put the headset detection driver on the last */
};

static struct headset_adc_config htc_headset_mgr_config[] = {
	{
		.type = HEADSET_MIC,
		.adc_max = 1560,
		.adc_min = 1223,
	},
	{
		.type = HEADSET_BEATS,
		.adc_max = 1222,
		.adc_min = 916,
	},
	{
		.type = HEADSET_BEATS_SOLO,
		.adc_max = 915,
		.adc_min = 566,
	},
	{
		.type = HEADSET_MIC, /* No Metrico device */
		.adc_max = 565,
		.adc_min = 255,
	},
	{
		.type = HEADSET_NO_MIC,
		.adc_max = 254,
		.adc_min = 0,
	},
};

static struct htc_headset_mgr_platform_data htc_headset_mgr_data = {
	.driver_flag		= DRIVER_HS_MGR_OLD_AJ,
	.headset_devices_num	= ARRAY_SIZE(headset_devices),
	.headset_devices	= headset_devices,
	.headset_config_num	= ARRAY_SIZE(htc_headset_mgr_config),
	.headset_config		= htc_headset_mgr_config,
	.headset_init		= headset_init,
	.headset_power		= headset_power,
};

static struct platform_device htc_headset_mgr = {
	.name	= "HTC_HEADSET_MGR",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_mgr_data,
	},
};

static uint32_t usb_ID_PIN_input_table[] = {
	GPIO_CFG(FIGHTER_USB_ID1, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t usb_ID_PIN_ouput_table[] = {
	GPIO_CFG(FIGHTER_USB_ID1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

void config_fighter_usb_id_gpios(bool output)
{
	if (output) {
		gpio_tlmm_config(usb_ID_PIN_ouput_table[0], GPIO_CFG_ENABLE);
		gpio_set_value(FIGHTER_USB_ID1, 1);
		pr_info("[CABLE] %s: %d output high\n",  __func__, FIGHTER_USB_ID1);
	} else {
		gpio_tlmm_config(usb_ID_PIN_input_table[0], GPIO_CFG_ENABLE);
		pr_info("[CABLE] %s: %d input none pull\n",  __func__, FIGHTER_USB_ID1);
	}
}

int64_t fighter_get_usbid_adc(void)
{
	struct pm8xxx_adc_chan_result result;
	int err = 0, adc = 0;

	err = pm8xxx_adc_mpp_config_read(PM8XXX_AMUX_MPP_7,
					ADC_MPP_1_AMUX6, &result);
	if (err) {
		pr_info("[CABLE] %s: get adc fail, err %d\n", __func__, err);
		return err;
	}
	pr_info("[CABLE] chan=%d, adc_code=%d, measurement=%lld, \
			physical=%lld\n", result.chan, result.adc_code,
			result.measurement, result.physical);
	adc = result.physical;
	return adc/1000;
}

static uint32_t usb_audio_switch_table[] = {
	GPIO_CFG(FIGHTER_USBz_AUDIO_SW, 0, GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(FIGHTER_USBz_AUDIO_SW, 0, GPIO_CFG_INPUT,GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t audio_uart_switch_table[] = {
	GPIO_CFG(FIGHTER_AUDIOz_UART_SW, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static void fighter_usb_dpdn_switch(int path)
{
	if ((system_rev == 0x80 && engineerid == 1) || system_rev < 0x80) {
		gpio_tlmm_config(audio_uart_switch_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(usb_audio_switch_table[1], GPIO_CFG_ENABLE);
	} else {
		gpio_tlmm_config(audio_uart_switch_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(usb_audio_switch_table[0], GPIO_CFG_ENABLE);
	}

	switch (path) {
	case PATH_USB:
		pr_info("[CABLE] %s: Set USB path\n", __func__);

		if ((system_rev == 0x80 && engineerid == 1) || system_rev < 0x80) {
			gpio_set_value(FIGHTER_AUDIOz_UART_SW, 1);
		} else {
			gpio_set_value(FIGHTER_AUDIOz_UART_SW, 1);
			gpio_set_value(FIGHTER_USBz_AUDIO_SW, 0);
		}
		break;
	case PATH_USB_AUD:
		pr_info("[CABLE] %s: Set Audio path\n", __func__);

		if ((system_rev == 0x80 && engineerid == 1) || system_rev < 0x80) {
			gpio_set_value(FIGHTER_AUDIOz_UART_SW, 0);
		} else {
			gpio_set_value(FIGHTER_AUDIOz_UART_SW, 0);
			gpio_set_value(FIGHTER_USBz_AUDIO_SW, 1);
		}
		break;
	}

#ifdef CONFIG_FB_MSM_HDMI_MHL
	sii9234_change_usb_owner((path == PATH_MHL) ? 1 : 0);
#endif
}


static struct cable_detect_platform_data cable_detect_pdata = {
	.detect_type		= CABLE_TYPE_PMIC_ADC,
	.usb_id_pin_gpio	= FIGHTER_USB_ID1,
	.get_adc_cb		= fighter_get_usbid_adc,
	.config_usb_id_gpios	= config_fighter_usb_id_gpios,
	.usb_dpdn_switch	= fighter_usb_dpdn_switch,
	/* wireless charger */
	.ad_en_active_state = 1,
	.ad_en_gpio = PM8921_GPIO_PM_TO_SYS(FIGHTER_AD_EN_MSM),
	.ad_en_irq = PM8921_GPIO_PM_TO_SYS(FIGHTER_AD_EN_MSM),
};

static struct platform_device cable_detect_device = {
	.name   = "cable_detect",
	.id     = -1,
	.dev    = {
		.platform_data = &cable_detect_pdata,
	},
};

static void headset_device_register(void)
{
	pr_info("[HS_BOARD] (%s) Headset device register\n", __func__);
	platform_device_register(&htc_headset_mgr);
	platform_device_register(&cable_detect_device);
}

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static struct resource hdmi_msm_resources[] = {
	{
		.name  = "hdmi_msm_qfprom_addr",
		.start = 0x00700000,
		.end   = 0x007060FF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_hdmi_addr",
		.start = 0x04A00000,
		.end   = 0x04A00FFF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_irq",
		.start = HDMI_IRQ,
		.end   = HDMI_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static int hdmi_enable_5v(int on);
static int hdmi_core_power(int on, int show);
static int hdmi_cec_power(int on);

static struct msm_hdmi_platform_data hdmi_msm_data = {
	.irq = HDMI_IRQ,
	.enable_5v = hdmi_enable_5v,
	.core_power = hdmi_core_power,
	.cec_power = hdmi_cec_power,
};

static struct platform_device hdmi_msm_device = {
	.name = "hdmi_msm",
	.id = 0,
	.num_resources = ARRAY_SIZE(hdmi_msm_resources),
	.resource = hdmi_msm_resources,
	.dev.platform_data = &hdmi_msm_data,
};
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */

#ifdef CONFIG_MSM_BUS_SCALING
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

static void __init msm_fb_add_devices(void)
{
/*
	struct platform_device *ptr = NULL;

	if (machine_is_msm8960_liquid())
		ptr = &mipi_dsi2lvds_bridge_device;
	else
		ptr = &mipi_dsi_toshiba_panel_device;
	platform_add_devices(&ptr, 1);
*/
	if (machine_is_msm8x60_rumi3()) {
		msm_fb_register_device("mdp", NULL);
	} else
		msm_fb_register_device("mdp", &mdp_pdata);

#ifdef CONFIG_MSM_BUS_SCALING
	msm_fb_register_device("dtv", &dtv_pdata);
#endif
}

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static int hdmi_enable_5v(int on)
{
	/* TBD: PM8921 regulator instead of 8901 */
	static struct regulator *reg_8921_hdmi_mvs;	/* HDMI_5V */
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (!reg_8921_hdmi_mvs)
		reg_8921_hdmi_mvs = regulator_get(&hdmi_msm_device.dev,
			"hdmi_mvs");

	if (on) {
		rc = regulator_enable(reg_8921_hdmi_mvs);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"8921_hdmi_mvs", rc);
			return rc;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(reg_8921_hdmi_mvs);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"8921_hdmi_mvs", rc);
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
}

static int hdmi_core_power(int on, int show)
{
	static struct regulator *reg_8921_l23, *reg_8921_s4;
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	/* TBD: PM8921 regulator instead of 8901 */
	if (!reg_8921_l23) {
		reg_8921_l23 = regulator_get(&hdmi_msm_device.dev, "hdmi_avdd");
		if (IS_ERR(reg_8921_l23)) {
			pr_err("could not get reg_8921_l23, rc = %ld\n",
				PTR_ERR(reg_8921_l23));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_8921_l23, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage failed for 8921_l23, rc=%d\n", rc);
			return -EINVAL;
		}
	}
	if (!reg_8921_s4) {
		reg_8921_s4 = regulator_get(&hdmi_msm_device.dev, "hdmi_vcc");
		if (IS_ERR(reg_8921_s4)) {
			pr_err("could not get reg_8921_s4, rc = %ld\n",
				PTR_ERR(reg_8921_s4));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_8921_s4, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage failed for 8921_s4, rc=%d\n", rc);
			return -EINVAL;
		}
	}

	if (on) {
		rc = regulator_set_optimum_mode(reg_8921_l23, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_8921_l23);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_avdd", rc);
			return rc;
		}
		rc = regulator_enable(reg_8921_s4);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_vcc", rc);
			return rc;
		}

		rc = gpio_request(100, "HDMI_DDC_CLK");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_CLK", 100, rc);
			goto error1;
		}
		rc = gpio_request(101, "HDMI_DDC_DATA");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_DATA", 101, rc);
			goto error2;
		}
		rc = gpio_request(102, "HDMI_HPD");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_HPD", 102, rc);
			goto error3;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		gpio_free(100);
		gpio_free(101);
		gpio_free(102);

		rc = regulator_disable(reg_8921_l23);
		if (rc) {
			pr_err("disable reg_8921_l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_8921_s4);
		if (rc) {
			pr_err("disable reg_8921_s4 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_8921_l23, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;

error3:
	gpio_free(101);
error2:
	gpio_free(100);
error1:
	regulator_disable(reg_8921_l23);
	regulator_disable(reg_8921_s4);
	return rc;
}

static int hdmi_cec_power(int on)
{
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (on) {
		rc = gpio_request(99, "HDMI_CEC_VAR");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_CEC_VAR", 99, rc);
			goto error;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		gpio_free(99);
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
error:
	return rc;
}
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */

static void __init msm8960_allocate_memory_regions(void)
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
#ifdef CONFIG_WCD9310_CODEC

#define TABLA_INTERRUPT_BASE (NR_MSM_IRQS + NR_GPIO_IRQS + NR_PM8921_IRQS)

/* Micbias setting is based on 8660 CDP/MTP/FLUID requirement
 * 4 micbiases are used to power various analog and digital
 * microphones operating at 1800 mV. Technically, all micbiases
 * can source from single cfilter since all microphones operate
 * at the same voltage level. The arrangement below is to make
 * sure all cfilters are exercised. LDO_H regulator ouput level
 * does not need to be as high as 2.85V. It is choosen for
 * microphone sensitivity purpose.
 */
static struct tabla_pdata tabla_platform_data = {
	.slimbus_slave_device = {
		.name = "tabla-slave",
		.e_addr = {0, 0, 0x10, 0, 0x17, 2},
	},
	.irq = MSM_GPIO_TO_INT(62),
	.irq_base = TABLA_INTERRUPT_BASE,
	.num_irqs = NR_TABLA_IRQS,
	.reset_gpio = PM8921_GPIO_PM_TO_SYS(34),
	.micbias = {
		.ldoh_v = TABLA_LDOH_2P85_V,
		.cfilt1_mv = 1800,
		.cfilt2_mv = 1800,
		.cfilt3_mv = 1800,
		.bias1_cfilt_sel = TABLA_CFILT1_SEL,
		.bias2_cfilt_sel = TABLA_CFILT2_SEL,
		.bias3_cfilt_sel = TABLA_CFILT3_SEL,
		.bias4_cfilt_sel = TABLA_CFILT3_SEL,
	}
};

static struct slim_device msm_slim_tabla = {
	.name = "tabla-slim",
	.e_addr = {0, 1, 0x10, 0, 0x17, 2},
	.dev = {
		.platform_data = &tabla_platform_data,
	},
};
static struct tabla_pdata tabla20_platform_data = {
	.slimbus_slave_device = {
		.name = "tabla-slave",
		.e_addr = {0, 0, 0x60, 0, 0x17, 2},
	},
	.irq = MSM_GPIO_TO_INT(62),
	.irq_base = TABLA_INTERRUPT_BASE,
	.num_irqs = NR_TABLA_IRQS,
	.reset_gpio = PM8921_GPIO_PM_TO_SYS(34),
	.amic_settings = {
		.legacy_mode = 0x7F,
		.use_pdata = 0x7F,
	},
	.micbias = {
		.ldoh_v = TABLA_LDOH_2P85_V,
		.cfilt1_mv = 1800,
		.cfilt2_mv = 1800,
		.cfilt3_mv = 1800,
		.bias1_cfilt_sel = TABLA_CFILT1_SEL,
		.bias2_cfilt_sel = TABLA_CFILT2_SEL,
		.bias3_cfilt_sel = TABLA_CFILT3_SEL,
		.bias4_cfilt_sel = TABLA_CFILT3_SEL,
	}
};

static struct slim_device msm_slim_tabla20 = {
	.name = "tabla2x-slim",
	.e_addr = {0, 1, 0x60, 0, 0x17, 2},
	.dev = {
		.platform_data = &tabla20_platform_data,
	},
};
#endif

static struct slim_boardinfo msm_slim_devices[] = {
#ifdef CONFIG_WCD9310_CODEC
	{
		.bus_num = 1,
		.slim_slave = &msm_slim_tabla,
	},
	{
		.bus_num = 1,
		.slim_slave = &msm_slim_tabla20,
	},
#endif
	/* add more slimbus slaves as needed */
};

#define MSM_WCNSS_PHYS	0x03000000
#define MSM_WCNSS_SIZE	0x280000

static struct resource resources_wcnss_wlan[] = {
	{
		.start	= RIVA_APPS_WLAN_RX_DATA_AVAIL_IRQ,
		.end	= RIVA_APPS_WLAN_RX_DATA_AVAIL_IRQ,
		.name	= "wcnss_wlanrx_irq",
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= RIVA_APPS_WLAN_DATA_XFER_DONE_IRQ,
		.end	= RIVA_APPS_WLAN_DATA_XFER_DONE_IRQ,
		.name	= "wcnss_wlantx_irq",
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= MSM_WCNSS_PHYS,
		.end	= MSM_WCNSS_PHYS + MSM_WCNSS_SIZE - 1,
		.name	= "wcnss_mmio",
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= 84,
		.end	= 88,
		.name	= "wcnss_gpios_5wire",
		.flags	= IORESOURCE_IO,
	},
};

static struct qcom_wcnss_opts qcom_wcnss_pdata = {
	.has_48mhz_xo	= 1,
};

static struct platform_device msm_device_wcnss_wlan = {
	.name		= "wcnss_wlan",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(resources_wcnss_wlan),
	.resource	= resources_wcnss_wlan,
	.dev		= {.platform_data = &qcom_wcnss_pdata},
};

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

#define QCE_SIZE		0x10000
#define QCE_0_BASE		0x18500000

#define QCE_HW_KEY_SUPPORT	0
#define QCE_SHA_HMAC_SUPPORT	1
#define QCE_SHARE_CE_RESOURCE	1
#define QCE_CE_SHARED		0

/* Begin Bus scaling definitions */
static struct msm_bus_vectors crypto_hw_init_vectors[] = {
        {
                .src = MSM_BUS_MASTER_ADM_PORT0,
                .dst = MSM_BUS_SLAVE_EBI_CH0,
                .ab = 0,
                .ib = 0,
        },
        {
                .src = MSM_BUS_MASTER_ADM_PORT1,
                .dst = MSM_BUS_SLAVE_GSBI1_UART,
                .ab = 0,
                .ib = 0,
        },
};

static struct msm_bus_vectors crypto_hw_active_vectors[] = {
        {
                .src = MSM_BUS_MASTER_ADM_PORT0,
                .dst = MSM_BUS_SLAVE_EBI_CH0,
                .ab = 70000000UL,
                .ib = 70000000UL,
        },
        {
                .src = MSM_BUS_MASTER_ADM_PORT1,
                .dst = MSM_BUS_SLAVE_GSBI1_UART,
                .ab = 2480000000UL,
                .ib = 2480000000UL,
        },
};

static struct msm_bus_paths crypto_hw_bus_scale_usecases[] = {
        {
                ARRAY_SIZE(crypto_hw_init_vectors),
                crypto_hw_init_vectors,
        },
        {
                ARRAY_SIZE(crypto_hw_active_vectors),
                crypto_hw_active_vectors,
        },
};

static struct msm_bus_scale_pdata crypto_hw_bus_scale_pdata = {
                crypto_hw_bus_scale_usecases,
                ARRAY_SIZE(crypto_hw_bus_scale_usecases),
                .name = "cryptohw",
};
/* End Bus Scaling Definitions*/

static struct resource qcrypto_resources[] = {
	[0] = {
		.start = QCE_0_BASE,
		.end = QCE_0_BASE + QCE_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "crypto_channels",
		.start = DMOV_CE_IN_CHAN,
		.end = DMOV_CE_OUT_CHAN,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.name = "crypto_crci_in",
		.start = DMOV_CE_IN_CRCI,
		.end = DMOV_CE_IN_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.name = "crypto_crci_out",
		.start = DMOV_CE_OUT_CRCI,
		.end = DMOV_CE_OUT_CRCI,
		.flags = IORESOURCE_DMA,
	},
};

static struct resource qcedev_resources[] = {
	[0] = {
		.start = QCE_0_BASE,
		.end = QCE_0_BASE + QCE_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "crypto_channels",
		.start = DMOV_CE_IN_CHAN,
		.end = DMOV_CE_OUT_CHAN,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.name = "crypto_crci_in",
		.start = DMOV_CE_IN_CRCI,
		.end = DMOV_CE_IN_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.name = "crypto_crci_out",
		.start = DMOV_CE_OUT_CRCI,
		.end = DMOV_CE_OUT_CRCI,
		.flags = IORESOURCE_DMA,
	},
};

#endif

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)

static struct msm_ce_hw_support qcrypto_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
	.sha_hmac = QCE_SHA_HMAC_SUPPORT,
	.bus_scale_table = &crypto_hw_bus_scale_pdata,
};

static struct platform_device qcrypto_device = {
	.name		= "qcrypto",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qcrypto_resources),
	.resource	= qcrypto_resources,
	.dev		= {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &qcrypto_ce_hw_suppport,
	},
};
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

static struct msm_ce_hw_support qcedev_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
	.sha_hmac = QCE_SHA_HMAC_SUPPORT,
	.bus_scale_table = &crypto_hw_bus_scale_pdata,
};

static struct platform_device qcedev_device = {
	.name		= "qce",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qcedev_resources),
	.resource	= qcedev_resources,
	.dev		= {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &qcedev_ce_hw_suppport,
	},
};
#endif


#define MSM_SHARED_RAM_PHYS 0x80000000

static void pm8xxx_adc_device_register(void)
{
	pr_info("%s: Register PM8XXX ADC device\n", __func__);
	headset_device_register();
}

static struct pm8xxx_adc_amux pm8xxx_adc_channels_data[] = {
	{"vcoin", CHANNEL_VCOIN, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"vbat", CHANNEL_VBAT, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"dcin", CHANNEL_DCIN, CHAN_PATH_SCALING4, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"ichg", CHANNEL_ICHG, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"vph_pwr", CHANNEL_VPH_PWR, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"ibat", CHANNEL_IBAT, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"batt_therm", CHANNEL_BATT_THERM, CHAN_PATH_SCALING1, AMUX_RSV2,
		ADC_DECIMATION_TYPE2, ADC_SCALE_BATT_THERM},
	{"batt_id", CHANNEL_BATT_ID, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"usbin", CHANNEL_USBIN, CHAN_PATH_SCALING3, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"pmic_therm", CHANNEL_DIE_TEMP, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PMIC_THERM},
	{"625mv", CHANNEL_625MV, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"125v", CHANNEL_125V, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"chg_temp", CHANNEL_CHG_TEMP, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"pa_therm1", ADC_MPP_1_AMUX8, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PA_THERM},
	{"xo_therm", CHANNEL_MUXOFF, CHAN_PATH_SCALING1, AMUX_RSV0,
		ADC_DECIMATION_TYPE2, ADC_SCALE_XOTHERM},
	{"pa_therm0", ADC_MPP_1_AMUX3, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PA_THERM},
	{"mpp_amux6", ADC_MPP_1_AMUX6, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
};

static struct pm8xxx_adc_properties pm8xxx_adc_data = {
	.adc_vdd_reference	= 1800, /* milli-voltage for this adc */
	.bitresolution		= 15,
	.bipolar                = 0,
};

static const struct pm8xxx_adc_map_pt fighter_adcmap_btm_table[] = {
	{-200,	1671},
	{-190,	1663},
	{-180,	1654},
	{-170,	1646},
	{-160,	1636},
	{-150,	1627},
	{-140,	1617},
	{-130,	1606},
	{-120,	1595},
	{-110,	1584},
	{-100,	1572},
	{-90,	1560},
	{-80,	1548},
	{-70,	1534},
	{-60,	1521},
	{-50,	1507},
	{-40,	1492},
	{-30,	1477},
	{-20,	1462},
	{-10,	1446},
	{-0,	1430},
	{10,	1413},
	{20,	1396},
	{30,	1379},
	{40,	1361},
	{50,	1343},
	{60,	1325},
	{70,	1306},
	{80,	1287},
	{90,	1267},
	{100,	1248},
	{110,	1228},
	{120,	1208},
	{130,	1188},
	{140,	1168},
	{150,	1147},
	{160,	1127},
	{170,	1106},
	{180,	1086},
	{190,	1065},
	{200,	1044},
	{210,	1024},
	{220,	1004},
	{230,	983},
	{240,	963},
	{250,	943},
	{260,	923},
	{270,	903},
	{280,	884},
	{290,	864},
	{300,	845},
	{310,	827},
	{320,	808},
	{330,	790},
	{340,	772},
	{350,	755},
	{360,	738},
	{370,	721},
	{380,	704},
	{390,	688},
	{400,	672},
	{410,	657},
	{420,	642},
	{430,	627},
	{440,	613},
	{450,	599},
	{460,	585},
	{470,	572},
	{480,	559},
	{490,	547},
	{500,	535},
	{510,	523},
	{520,	511},
	{530,	500},
	{540,	489},
	{550,	479},
	{560,	469},
	{570,	459},
	{580,	449},
	{590,	440},
	{600,	431},
	{610,	423},
	{620,	414},
	{630,	406},
	{640,	398},
	{650,	390},
	{660,	383},
	{670,	376},
	{680,	369},
	{690,	363},
	{700,	356},
	{710,	350},
	{720,	344},
	{730,	338},
	{740,	333},
	{750,	327},
	{760,	322},
	{770,	317},
	{780,	312},
	{790,	308}
};

static struct pm8xxx_adc_map_table pm8xxx_adcmap_btm_table = {
	.table = fighter_adcmap_btm_table,
	.size = ARRAY_SIZE(fighter_adcmap_btm_table),
};

static struct pm8xxx_adc_platform_data pm8xxx_adc_pdata = {
	.adc_channel			= pm8xxx_adc_channels_data,
	.adc_num_board_channel		= ARRAY_SIZE(pm8xxx_adc_channels_data),
	.adc_prop			= &pm8xxx_adc_data,
	.adc_mpp_base			= PM8921_MPP_PM_TO_SYS(1),
	.adc_map_btm_table	= &pm8xxx_adcmap_btm_table,
	.pm8xxx_adc_device_register	= pm8xxx_adc_device_register,
};

static void __init fighter_map_io(void)
{
	msm_shared_ram_phys = MSM_SHARED_RAM_PHYS;
	msm_map_msm8960_io();

	if (socinfo_init() < 0)
		pr_err("socinfo_init() failed!\n");
}

static void __init fighter_init_irq(void)
{
	msm_mpm_irq_extn_init();
	gic_init(0, GIC_PPI_START, MSM_QGIC_DIST_BASE,
						(void *)MSM_QGIC_CPU_BASE);

	/* Edge trigger PPIs except AVS_SVICINT and AVS_SVICINTSWDONE */
	writel_relaxed(0xFFFFD7FF, MSM_QGIC_DIST_BASE + GIC_DIST_CONFIG + 4);

	writel_relaxed(0x0000FFFF, MSM_QGIC_DIST_BASE + GIC_DIST_ENABLE_SET);
	mb();
}

/* MSM8960 has 5 SDCC controllers */
enum sdcc_controllers {
	SDCC1,
	SDCC2,
	SDCC3,
	SDCC4,
	SDCC5,
	MAX_SDCC_CONTROLLER
};

/* All SDCC controllers require VDD/VCC voltage */
static struct msm_mmc_reg_data mmc_vdd_reg_data[MAX_SDCC_CONTROLLER] = {
	/* SDCC1 : eMMC card connected */
	[SDCC1] = {
		.name = "sdc_vdd",
		.set_voltage_sup = 1,
		.high_vol_level = 2950000,
		.low_vol_level = 2950000,
		.always_on = 1,
		.lpm_sup = 1,
		.lpm_uA = 9000,
		.hpm_uA = 200000, /* 200mA */
	},
	/* SDCC3 : External card slot connected */
	[SDCC3] = {
		.name = "sdc_vdd",
		.set_voltage_sup = 1,
		.high_vol_level = 2850000,
		.low_vol_level = 2850000,
		.hpm_uA = 600000, /* 600mA */
	}
};

/* Only slots having eMMC card will require VCCQ voltage */
static struct msm_mmc_reg_data mmc_vccq_reg_data[1] = {
	/* SDCC1 : eMMC card connected */
	[SDCC1] = {
		.name = "sdc_vccq",
		.set_voltage_sup = 1,
		.always_on = 1,
		.high_vol_level = 1800000,
		.low_vol_level = 1800000,
		.hpm_uA = 200000, /* 200mA */
	}
};

/* All SDCC controllers may require voting for VDD PAD voltage */
static struct msm_mmc_reg_data mmc_vddp_reg_data[MAX_SDCC_CONTROLLER] = {
	/* SDCC3 : External card slot connected */
	[SDCC3] = {
		.name = "sdc_vddp",
		.set_voltage_sup = 1,
		.high_vol_level = 2850000,
		.low_vol_level = 1850000,
		.always_on = 1,
		.lpm_sup = 1,
		/* Max. Active current required is 16 mA */
		.hpm_uA = 16000,
		/*
		 * Sleep current required is ~300 uA. But min. vote can be
		 * in terms of mA (min. 1 mA). So let's vote for 2 mA
		 * during sleep.
		 */
		.lpm_uA = 2000,
	}
};

static struct msm_mmc_slot_reg_data mmc_slot_vreg_data[MAX_SDCC_CONTROLLER] = {
	/* SDCC1 : eMMC card connected */
	[SDCC1] = {
		.vdd_data = &mmc_vdd_reg_data[SDCC1],
		.vccq_data = &mmc_vccq_reg_data[SDCC1],
	},
	/* SDCC3 : External card slot connected */
	[SDCC3] = {
		.vdd_data = &mmc_vdd_reg_data[SDCC3],
		.vddp_data = &mmc_vddp_reg_data[SDCC3],
	}
};

/* SDC1 pad data */
static struct msm_mmc_pad_drv sdc1_pad_drv_on_cfg[] = {
	{TLMM_HDRV_SDC1_CLK, GPIO_CFG_10MA},
	{TLMM_HDRV_SDC1_CMD, GPIO_CFG_6MA},
	{TLMM_HDRV_SDC1_DATA, GPIO_CFG_8MA}
};

static struct msm_mmc_pad_drv sdc1_pad_drv_off_cfg[] = {
	{TLMM_HDRV_SDC1_CLK, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC1_CMD, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC1_DATA, GPIO_CFG_2MA}
};

static struct msm_mmc_pad_pull sdc1_pad_pull_on_cfg[] = {
	{TLMM_PULL_SDC1_CLK, GPIO_CFG_NO_PULL},
	{TLMM_PULL_SDC1_CMD, GPIO_CFG_PULL_UP},
	{TLMM_PULL_SDC1_DATA, GPIO_CFG_PULL_UP}
};

static struct msm_mmc_pad_pull sdc1_pad_pull_off_cfg[] = {
	{TLMM_PULL_SDC1_CLK, GPIO_CFG_NO_PULL},
	{TLMM_PULL_SDC1_CMD, GPIO_CFG_PULL_UP},
	{TLMM_PULL_SDC1_DATA, GPIO_CFG_PULL_UP}
};

/* SDC3 pad data */
static struct msm_mmc_pad_drv sdc3_pad_drv_on_cfg[] = {
	{TLMM_HDRV_SDC3_CLK, GPIO_CFG_12MA},
	{TLMM_HDRV_SDC3_CMD, GPIO_CFG_8MA},
	{TLMM_HDRV_SDC3_DATA, GPIO_CFG_10MA}
};

static struct msm_mmc_pad_drv sdc3_pad_drv_off_cfg[] = {
	{TLMM_HDRV_SDC3_CLK, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC3_CMD, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC3_DATA, GPIO_CFG_2MA}
};

static struct msm_mmc_pad_pull sdc3_pad_pull_on_cfg[] = {
	{TLMM_PULL_SDC3_CLK, GPIO_CFG_NO_PULL},
	{TLMM_PULL_SDC3_CMD, GPIO_CFG_PULL_UP},
	{TLMM_PULL_SDC3_DATA, GPIO_CFG_PULL_UP}
};

static struct msm_mmc_pad_pull sdc3_pad_pull_off_cfg[] = {
	{TLMM_PULL_SDC3_CLK, GPIO_CFG_NO_PULL},
	{TLMM_PULL_SDC3_CMD, GPIO_CFG_PULL_DOWN},
	{TLMM_PULL_SDC3_DATA, GPIO_CFG_PULL_DOWN}
};

struct msm_mmc_pad_pull_data mmc_pad_pull_data[MAX_SDCC_CONTROLLER] = {
	[SDCC1] = {
		.on = sdc1_pad_pull_on_cfg,
		.off = sdc1_pad_pull_off_cfg,
		.size = ARRAY_SIZE(sdc1_pad_pull_on_cfg)
	},
	[SDCC3] = {
		.on = sdc3_pad_pull_on_cfg,
		.off = sdc3_pad_pull_off_cfg,
		.size = ARRAY_SIZE(sdc3_pad_pull_on_cfg)
	},
};

struct msm_mmc_pad_drv_data mmc_pad_drv_data[MAX_SDCC_CONTROLLER] = {
	[SDCC1] = {
		.on = sdc1_pad_drv_on_cfg,
		.off = sdc1_pad_drv_off_cfg,
		.size = ARRAY_SIZE(sdc1_pad_drv_on_cfg)
	},
	[SDCC3] = {
		.on = sdc3_pad_drv_on_cfg,
		.off = sdc3_pad_drv_off_cfg,
		.size = ARRAY_SIZE(sdc3_pad_drv_on_cfg)
	},
};

struct msm_mmc_pad_data mmc_pad_data[MAX_SDCC_CONTROLLER] = {
	[SDCC1] = {
		.pull = &mmc_pad_pull_data[SDCC1],
		.drv = &mmc_pad_drv_data[SDCC1]
	},
	[SDCC3] = {
		.pull = &mmc_pad_pull_data[SDCC3],
		.drv = &mmc_pad_drv_data[SDCC3]
	},
};

struct msm_mmc_pin_data mmc_slot_pin_data[MAX_SDCC_CONTROLLER] = {
	[SDCC1] = {
		.pad_data = &mmc_pad_data[SDCC1],
	},
	[SDCC3] = {
		.pad_data = &mmc_pad_data[SDCC3],
	},
};

static unsigned int sdc1_sup_clk_rates[] = {
	400000, 24000000, 48000000, 96000000
};

static unsigned int sdc3_sup_clk_rates[] = {
	400000, 24000000, 48000000, 96000000, 192000000
};

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
static unsigned int fighter_sdc1_slot_type = MMC_TYPE_MMC;
static struct mmc_platform_data msm8960_sdc1_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29,
#ifdef CONFIG_MMC_MSM_SDC1_8_BIT_SUPPORT
	.mmc_bus_width  = MMC_CAP_8_BIT_DATA,
#else
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#endif
	.sup_clk_table	= sdc1_sup_clk_rates,
	.sup_clk_cnt	= ARRAY_SIZE(sdc1_sup_clk_rates),
	.slot_type		= &fighter_sdc1_slot_type,
	.pclk_src_dfab	= 1,
	.nonremovable	= 1,
	.vreg_data	= &mmc_slot_vreg_data[SDCC1],
	.pin_data	= &mmc_slot_pin_data[SDCC1],
	.uhs_caps	= (MMC_CAP_1_8V_DDR | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_DDR50)
};
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static unsigned int fighter_sdslot_type = MMC_TYPE_SD;
static struct mmc_platform_data msm8960_sdc3_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.sup_clk_table	= sdc3_sup_clk_rates,
	.sup_clk_cnt	= ARRAY_SIZE(sdc3_sup_clk_rates),
	.pclk_src_dfab	= 1,
#ifdef CONFIG_MMC_MSM_SDC3_WP_SUPPORT
	/*.wpswitch_gpio	= PM8921_GPIO_PM_TO_SYS(16),*/
#endif
	.vreg_data	= &mmc_slot_vreg_data[SDCC3],
	.pin_data	= &mmc_slot_pin_data[SDCC3],
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	.status_gpio	= PM8921_GPIO_PM_TO_SYS(23),
	.status_irq	= PM8921_GPIO_IRQ(PM8921_IRQ_BASE, 23),
	.irq_flags	= IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
#endif
	.slot_type = &fighter_sdslot_type,
#if 0
	.xpc_cap	= 1,
	.uhs_caps	= (MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
			MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_DDR50 |
			MMC_CAP_UHS_SDR104 | MMC_CAP_MAX_CURRENT_600),
#endif
};
#endif

static void __init msm8960_init_mmc(void)
{
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	msm8960_sdc1_data.swfi_latency = msm_rpm_get_swfi_latency();
	/* SDC1 : eMMC card connected */
	msm_add_sdcc(1, &msm8960_sdc1_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	msm8960_sdc3_data.swfi_latency = msm_rpm_get_swfi_latency();
	/* SDC3: External card slot */
	msm_add_sdcc(3, &msm8960_sdc3_data);
#endif
}

static void __init msm8960_init_buses(void)
{
#ifdef CONFIG_MSM_BUS_SCALING
	msm_bus_rpm_set_mt_mask();
	msm_bus_8960_apps_fabric_pdata.rpm_enabled = 1;
	msm_bus_8960_sys_fabric_pdata.rpm_enabled = 1;
	msm_bus_8960_mm_fabric_pdata.rpm_enabled = 1;
	msm_bus_apps_fabric.dev.platform_data =
		&msm_bus_8960_apps_fabric_pdata;
	msm_bus_sys_fabric.dev.platform_data = &msm_bus_8960_sys_fabric_pdata;
	msm_bus_mm_fabric.dev.platform_data = &msm_bus_8960_mm_fabric_pdata;
	msm_bus_sys_fpb.dev.platform_data = &msm_bus_8960_sys_fpb_pdata;
	msm_bus_cpss_fpb.dev.platform_data = &msm_bus_8960_cpss_fpb_pdata;
#endif
}


#ifdef CONFIG_USB_MSM_OTG_72K
static struct msm_otg_platform_data msm_otg_pdata;
#else
#define USB_5V_EN		42
static void msm_hsusb_vbus_power(bool on)
{
	int rc;
	static bool vbus_is_on;
	static struct regulator *mvs_otg_switch;
	struct pm_gpio param = {
		.direction	= PM_GPIO_DIR_OUT,
		.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
		.output_value	= 1,
		.pull		= PM_GPIO_PULL_NO,
		.vin_sel	= PM_GPIO_VIN_S4,
		.out_strength	= PM_GPIO_STRENGTH_MED,
		.function	= PM_GPIO_FUNC_NORMAL,
	};

	if (vbus_is_on == on)
		return;

	if (on) {
		mvs_otg_switch = regulator_get(&msm8960_device_otg.dev,
					       "vbus_otg");
		if (IS_ERR(mvs_otg_switch)) {
			pr_err("Unable to get mvs_otg_switch\n");
			return;
		}

		rc = gpio_request(PM8921_GPIO_PM_TO_SYS(USB_5V_EN),
						"usb_5v_en");
		if (rc < 0) {
			pr_err("failed to request usb_5v_en gpio\n");
			goto put_mvs_otg;
		}

		if (regulator_enable(mvs_otg_switch)) {
			pr_err("unable to enable mvs_otg_switch\n");
			goto free_usb_5v_en;
		}

		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(USB_5V_EN),
				&param);
		if (rc < 0) {
			pr_err("failed to configure usb_5v_en gpio\n");
			goto disable_mvs_otg;
		}
		vbus_is_on = true;
		return;
	}
disable_mvs_otg:
		regulator_disable(mvs_otg_switch);
free_usb_5v_en:
		gpio_free(PM8921_GPIO_PM_TO_SYS(USB_5V_EN));
put_mvs_otg:
		regulator_put(mvs_otg_switch);
		vbus_is_on = false;
}

static int fighter_phy_init_seq[] = { 0x7f, 0x81, -1 };

static struct msm_otg_platform_data msm_otg_pdata = {
	.phy_init_seq		= fighter_phy_init_seq,
	.mode			= USB_PERIPHERAL,
	.otg_control		= OTG_PMIC_CONTROL,
	.phy_type		= SNPS_28NM_INTEGRATED_PHY,
	.vbus_power		= msm_hsusb_vbus_power,
	.power_budget		= 750,
};
#endif

#define PID_MAGIC_ID		0x71432909
#define SERIAL_NUM_MAGIC_ID	0x61945374
#define SERIAL_NUMBER_LENGTH	127
#define DLOAD_USB_BASE_ADD	0x2A03F0C8

struct magic_num_struct {
	uint32_t pid;
	uint32_t serial_num;
};

struct dload_struct {
	uint32_t	reserved1;
	uint32_t	reserved2;
	uint32_t	reserved3;
	uint16_t	reserved4;
	uint16_t	pid;
	char		serial_number[SERIAL_NUMBER_LENGTH];
	uint16_t	reserved5;
	struct magic_num_struct magic_struct;
};

static int usb_diag_update_pid_and_serial_num(uint32_t pid, const char *snum)
{
	struct dload_struct __iomem *dload = 0;

	dload = ioremap(DLOAD_USB_BASE_ADD, sizeof(*dload));
	if (!dload) {
		pr_err("%s: cannot remap I/O memory region: %08x\n",
					__func__, DLOAD_USB_BASE_ADD);
		return -ENXIO;
	}

	pr_debug("%s: dload:%p pid:%x serial_num:%s\n",
				__func__, dload, pid, snum);
	/* update pid */
	dload->magic_struct.pid = PID_MAGIC_ID;
	dload->pid = pid;

	/* update serial number */
	dload->magic_struct.serial_num = 0;
	if (!snum) {
		memset(dload->serial_number, 0, SERIAL_NUMBER_LENGTH);
		goto out;
	}

	dload->magic_struct.serial_num = SERIAL_NUM_MAGIC_ID;
	strlcpy(dload->serial_number, snum, SERIAL_NUMBER_LENGTH);
out:
	iounmap(dload);
	return 0;
}

#ifdef CONFIG_USB_GADGET_VERIZON_PRODUCT_ID
static int fighter_usb_product_id_match_array[] = {
	0x0c93, 0x0e31, /* mtp */
	0x0ca8, 0x0e32, /* mtp + adb */
	0x0fda, 0x0e33, /* ums + adb + 9k rmnet */
	0x0fdb, 0x0e34, /* ums + 9k rmnet */
	0x0c07, 0x0e37, /* ums + adb + diag */
	0x0c08, 0x0e38, /* ums + diag */
	0x0ff8, 0x0e39, /* CDC-ECM */
	0x0ff9, 0x0e77, /* ums */
	0x0ffc, 0x0e7e, /* adb + rndis, tethering with USB debugging */
	-1,
};

static int fighter_usb_product_id_rndis[] = {
	0x0e35, 0x0e36,
};

static int fighter_usb_product_id_match(int product_id, int intrsharing)
{
	int *pid_array = fighter_usb_product_id_match_array;
	int *rndis_array = fighter_usb_product_id_rndis;

	if (!pid_array)
		return product_id;

	/* VZW pid re-match will not apply on MFG mode */
	if (board_mfg_mode() == 1)
		return product_id;

	while (pid_array[0] >= 0) {
		if (product_id == pid_array[0])
			return pid_array[1];
		pid_array += 2;
	}

	if (product_id == 0x0ffe) {
		/* rndis */
		if (intrsharing)
			return rndis_array[0];
		else
			return rndis_array[1];
	}

	return product_id;
}
#endif

#define SFAB_BW_MBPS(_bw) \
	{ \
		.vectors = (struct msm_bus_vectors[]){ \
			{\
				.src = MSM_BUS_MASTER_MSS, \
				.dst = MSM_BUS_SLAVE_EBI_CH0, \
				.ib = (_bw) * 1000000UL, \
				.ab = (_bw) *  100000UL, \
			}, \
		}, \
		.num_paths = 1, \
	}
#define CLKRGM_BSP_LPXO_FREQ_MHZ (27 * 8)
static struct msm_bus_paths usb_client_sfab_bw_level_tbl[] = {
	[0] =  SFAB_BW_MBPS(CLKRGM_BSP_LPXO_FREQ_MHZ), /* At least  27 MHz on bus. */
	[1] =  SFAB_BW_MBPS(528), /* At least  66.667 MHz on bus. */
	[2] =  SFAB_BW_MBPS(640), /* At least  80 MHz on bus. */
	[3] =  SFAB_BW_MBPS(800), /* At least  100 MHz on bus. */
	[4] =  SFAB_BW_MBPS(1064), /* At least 133 MHz on bus. */
	[5] =  SFAB_BW_MBPS(1312), /* At least 164 MHz on bus. */
};

static struct msm_bus_scale_pdata usb_client_sfab_bus_pdata = {
	.usecase = usb_client_sfab_bw_level_tbl,
	.num_usecases = ARRAY_SIZE(usb_client_sfab_bw_level_tbl),
	.active_only = 1,
	.name = "usb_client_systemclock",
};

static void usb_sfab_lock_control(int lock)
{
	static u32 sfab_bus_client;
	static int initial = 0;
	int ret;

	if (!initial) {
		sfab_bus_client = msm_bus_scale_register_client(&usb_client_sfab_bus_pdata);
		if (!sfab_bus_client) {
			pr_err("[USB:ERR] %s unable to register bus client\n", __func__);
			return;
		}
		pr_info("[USB] %s: register sfab usb client\n", __func__);
		initial = 1;
	}

	/* Update bandwidth if request has changed. This may sleep. */
	if (lock)
		ret = msm_bus_scale_client_update_request(sfab_bus_client, 4);
	else
		ret = msm_bus_scale_client_update_request(sfab_bus_client, 0);

	if (ret)
		pr_err("[USB:ERR] %s  bandwidth request failed (%d)\n", __func__, ret);
}

static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x0BB4,
	.product_id	= 0x0cda,
	.version	= 0x0100,
	.product_name		= "Android Phone",
	.manufacturer_name	= "HTC",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
	.update_pid_and_serial_num = usb_diag_update_pid_and_serial_num,
	.usb_id_pin_gpio = FIGHTER_USB_ID1,
	.usb_rmnet_interface = "smd,bam",
	.nluns = 2,
	.sfab_lock = usb_sfab_lock_control,
#ifdef CONFIG_USB_GADGET_VERIZON_PRODUCT_ID
	.match = fighter_usb_product_id_match,
#endif
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};

void fighter_add_usb_devices(void)
{
	printk(KERN_INFO "%s rev: %d\n", __func__, system_rev);
	android_usb_pdata.products[0].product_id =
			android_usb_pdata.product_id;


	/* diag bit set */
	if (get_radio_flag() & 0x20000) {
		android_usb_pdata.diag_init = 1;
	}

	/* add cdrom support in normal mode */
	if (board_mfg_mode() == 0) {
		android_usb_pdata.nluns = 3;
		android_usb_pdata.cdrom_lun = 0x4;
	}

	platform_device_register(&msm8960_device_gadget_peripheral);
	platform_device_register(&android_usb_device);
	printk(KERN_INFO "%s: OTG_PMIC_CONTROL in rev: %d\n",
			__func__, system_rev);
}

static int __init board_serialno_setup(char *serialno)
{
	android_usb_pdata.serial_number = serialno;
	return 1;
}
__setup("androidboot.serialno=", board_serialno_setup);

static uint8_t spm_wfi_cmd_sequence[] __initdata = {
			0x03, 0x0f,
};

static uint8_t spm_power_collapse_without_rpm[] __initdata = {
			0x00, 0x24, 0x54, 0x10,
			0x09, 0x03, 0x01,
			0x10, 0x54, 0x30, 0x0C,
			0x24, 0x30, 0x0f,
};

static uint8_t spm_power_collapse_with_rpm[] __initdata = {
			0x00, 0x24, 0x54, 0x10,
			0x09, 0x07, 0x01, 0x0B,
			0x10, 0x54, 0x30, 0x0C,
			0x24, 0x30, 0x0f,
};

static struct msm_spm_seq_entry msm_spm_seq_list[] __initdata = {
	[0] = {
		.mode = MSM_SPM_MODE_CLOCK_GATING,
		.notify_rpm = false,
		.cmd = spm_wfi_cmd_sequence,
	},
	[1] = {
		.mode = MSM_SPM_MODE_POWER_COLLAPSE,
		.notify_rpm = false,
		.cmd = spm_power_collapse_without_rpm,
	},
	[2] = {
		.mode = MSM_SPM_MODE_POWER_COLLAPSE,
		.notify_rpm = true,
		.cmd = spm_power_collapse_with_rpm,
	},
};

static struct msm_spm_platform_data msm_spm_data[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW0_BASE,
		.reg_init_values[MSM_SPM_REG_SAW2_SECURE] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_CFG] = 0x1F,
		.reg_init_values[MSM_SPM_REG_SAW2_VCTL] = 0xB0,
#if defined(CONFIG_MSM_AVS_HW)
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_CTL] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_HYSTERESIS] = 0x00,
#endif
		.reg_init_values[MSM_SPM_REG_SAW2_SPM_CTL] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DLY] = 0x02020204,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_0] = 0x0060009C,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_1] = 0x0000001C,
		.vctl_timeout_us = 50,
		.num_modes = ARRAY_SIZE(msm_spm_seq_list),
		.modes = msm_spm_seq_list,
	},
	[1] = {
		.reg_base_addr = MSM_SAW1_BASE,
		.reg_init_values[MSM_SPM_REG_SAW2_SECURE] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_CFG] = 0x1F,
		.reg_init_values[MSM_SPM_REG_SAW2_VCTL] = 0xB0,
#if defined(CONFIG_MSM_AVS_HW)
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_CTL] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_HYSTERESIS] = 0x00,
#endif
		.reg_init_values[MSM_SPM_REG_SAW2_SPM_CTL] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DLY] = 0x02020204,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_0] = 0x0060009C,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_1] = 0x0000001C,
		.vctl_timeout_us = 50,
		.num_modes = ARRAY_SIZE(msm_spm_seq_list),
		.modes = msm_spm_seq_list,
	},
};

#ifdef CONFIG_PERFLOCK
static unsigned fighter_perf_acpu_table[] = {
	810000000, /* LOWEST */
	918000000, /* LOW */
	1026000000, /* MEDIUM */
	1242000000,/* HIGH */
	1512000000, /* HIGHEST */
};

static struct perflock_platform_data fighter_perflock_data = {
	.perf_acpu_table = fighter_perf_acpu_table,
	.table_size = ARRAY_SIZE(fighter_perf_acpu_table),
};

static unsigned fighter_cpufreq_ceiling_acpu_table[] = {
	702000000,
	918000000,
	1026000000,
};

static struct perflock_platform_data fighter_cpufreq_ceiling_data = {
	.perf_acpu_table = fighter_cpufreq_ceiling_acpu_table,
	.table_size = ARRAY_SIZE(fighter_cpufreq_ceiling_acpu_table),
};
#endif

static uint8_t l2_spm_wfi_cmd_sequence[] __initdata = {
			0x00, 0x20, 0x03, 0x20,
			0x00, 0x0f,
};

static uint8_t l2_spm_gdhs_cmd_sequence[] __initdata = {
			0x00, 0x20, 0x34, 0x64,
			0x48, 0x07, 0x48, 0x20,
			0x50, 0x64, 0x04, 0x34,
			0x50, 0x0f,
};
static uint8_t l2_spm_power_off_cmd_sequence[] __initdata = {
			0x00, 0x10, 0x34, 0x64,
			0x48, 0x07, 0x48, 0x10,
			0x50, 0x64, 0x04, 0x34,
			0x50, 0x0F,
};

static struct msm_spm_seq_entry msm_spm_l2_seq_list[] __initdata = {
	[0] = {
		.mode = MSM_SPM_L2_MODE_RETENTION,
		.notify_rpm = false,
		.cmd = l2_spm_wfi_cmd_sequence,
	},
	[1] = {
		.mode = MSM_SPM_L2_MODE_GDHS,
		.notify_rpm = true,
		.cmd = l2_spm_gdhs_cmd_sequence,
	},
	[2] = {
		.mode = MSM_SPM_L2_MODE_POWER_COLLAPSE,
		.notify_rpm = true,
		.cmd = l2_spm_power_off_cmd_sequence,
	},
};


static struct msm_spm_platform_data msm_spm_l2_data[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW_L2_BASE,
		.reg_init_values[MSM_SPM_REG_SAW2_SECURE] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_SPM_CTL] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DLY] = 0x02020204,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_0] = 0x00A000AE,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_1] = 0x00A00020,
		.modes = msm_spm_l2_seq_list,
		.num_modes = ARRAY_SIZE(msm_spm_l2_seq_list),
	},
};

static uint32_t gsbi2_gpio_table[] = {
	GPIO_CFG(FIGHTER_NFC_I2C_SDA, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(FIGHTER_NFC_I2C_SCL, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi3_gpio_table[] = {
	GPIO_CFG(FIGHTER_TP_I2C_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(FIGHTER_TP_I2C_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

/* CAMERA setting */
static uint32_t gsbi4_gpio_table[] = {
	GPIO_CFG(FIGHTER_CAM_I2C_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(FIGHTER_CAM_I2C_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi8_gpio_table[] = {
	GPIO_CFG(FIGHTER_AC_I2C_SDA, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(FIGHTER_AC_I2C_SCL, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi12_gpio_table[] = {
	GPIO_CFG(FIGHTER_SENSOR_I2C_SDA, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(FIGHTER_SENSOR_I2C_SCL, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static void gsbi_qup_i2c_gpio_config(int adap_id, int config_type)
{
	printk(KERN_INFO "%s(): adap_id = %d, config_type = %d \n", __func__, adap_id, config_type);

	if ((adap_id == MSM_8960_GSBI2_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi2_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi2_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI2_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi2_gpio_table[0], GPIO_CFG_DISABLE);
		gpio_tlmm_config(gsbi2_gpio_table[1], GPIO_CFG_DISABLE);
	}

	if ((adap_id == MSM_8960_GSBI3_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi3_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi3_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI3_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi3_gpio_table[0], GPIO_CFG_DISABLE);
		gpio_tlmm_config(gsbi3_gpio_table[1], GPIO_CFG_DISABLE);
	}

	/* CAMERA setting */
	if ((adap_id == MSM_8960_GSBI4_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi4_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi4_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI4_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi4_gpio_table[0], GPIO_CFG_DISABLE);
		gpio_tlmm_config(gsbi4_gpio_table[1], GPIO_CFG_DISABLE);
	}

	if ((adap_id == MSM_8960_GSBI8_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi8_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi8_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI8_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi8_gpio_table[0], GPIO_CFG_DISABLE);
		gpio_tlmm_config(gsbi8_gpio_table[1], GPIO_CFG_DISABLE);
	}

	if ((adap_id == MSM_8960_GSBI12_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi12_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi12_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI12_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi12_gpio_table[0], GPIO_CFG_DISABLE);
		gpio_tlmm_config(gsbi12_gpio_table[1], GPIO_CFG_DISABLE);
	}
}

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi2_pdata = {
	.clk_freq = 100000,	/* use 100Mhz first, then 400Mhz */
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi4_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi3_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi8_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
	.share_uart_flag = 1,	/* check if QUP-I2C and Uart share the gisb */
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi12_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_rpm_platform_data msm_rpm_data = {
	.reg_base_addrs = {
		[MSM_RPM_PAGE_STATUS] = MSM_RPM_BASE,
		[MSM_RPM_PAGE_CTRL] = MSM_RPM_BASE + 0x400,
		[MSM_RPM_PAGE_REQ] = MSM_RPM_BASE + 0x600,
		[MSM_RPM_PAGE_ACK] = MSM_RPM_BASE + 0xa00,
		[MSM_RPM_PAGE_STAT] = MSM_RPM_BASE + 0x5204,
	},

	.irq_ack = RPM_APCC_CPU0_GP_HIGH_IRQ,
	.irq_err = RPM_APCC_CPU0_GP_LOW_IRQ,
	.irq_vmpm = RPM_APCC_CPU0_GP_MEDIUM_IRQ,
	.msm_apps_ipc_rpm_reg = MSM_APCS_GCC_BASE + 0x008,
	.msm_apps_ipc_rpm_val = 4,
};

static struct msm_pm_sleep_status_data msm_pm_slp_sts_data = {
	.base_addr = MSM_ACC0_BASE + 0x08,
	.cpu_offset = MSM_ACC1_BASE - MSM_ACC0_BASE,
	.mask = 1UL << 13,
};

static struct platform_device msm_device_saw_core0 = {
	.name          = "saw-regulator",
	.id            = 0,
	.dev	= {
		.platform_data = &msm_saw_regulator_pdata_s5,
	},
};

static struct platform_device msm_device_saw_core1 = {
	.name          = "saw-regulator",
	.id            = 1,
	.dev	= {
		.platform_data = &msm_saw_regulator_pdata_s6,
	},
};

static struct tsens_platform_data msm_tsens_pdata  = {
		.slope			= {910, 910, 910, 910, 910},
		.tsens_factor		= 1000,
		.hw_type		= MSM_8960,
		.tsens_num_sensor	= 5,
};

#ifdef CONFIG_MSM_FAKE_BATTERY
static struct platform_device fish_battery_device = {
	.name = "fish_battery",
};
#endif

static struct msm_rpm_log_platform_data msm_rpm_log_pdata = {
	.phys_addr_base = 0x0010C000,
	.reg_offsets = {
		[MSM_RPM_LOG_PAGE_INDICES] = 0x00000080,
		[MSM_RPM_LOG_PAGE_BUFFER]  = 0x000000A0,
	},
	.phys_size = SZ_8K,
	.log_len = 4096,		  /* log's buffer length in bytes */
	.log_len_mask = (4096 >> 2) - 1,  /* length mask in units of u32 */
};

static struct platform_device msm_rpm_log_device = {
	.name	= "msm_rpm_log",
	.id	= -1,
	.dev	= {
		.platform_data = &msm_rpm_log_pdata,
	},
};

#define SHARED_IMEM_TZ_BASE 0x2a03f720
static struct resource tzlog_resources[] = {
	{
		.start = SHARED_IMEM_TZ_BASE,
		.end = SHARED_IMEM_TZ_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device msm_device_tz_log = {
	.name		= "tz_log",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(tzlog_resources),
	.resource	= tzlog_resources,
};

static struct platform_device scm_memchk_device = {
	.name		= "scm-memchk",
	.id		= -1,
};

#define MSM_RAM_CONSOLE_BASE   MSM_HTC_RAM_CONSOLE_PHYS
#define MSM_RAM_CONSOLE_SIZE   MSM_HTC_RAM_CONSOLE_SIZE

static struct resource ram_console_resources[] = {
	{
		.start	= MSM_RAM_CONSOLE_BASE,
		.end	= MSM_RAM_CONSOLE_BASE + MSM_RAM_CONSOLE_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device ram_console_device = {
	.name		= "ram_console",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(ram_console_resources),
	.resource	= ram_console_resources,
};

#ifdef CONFIG_HTC_BATT_8960
static struct htc_battery_platform_data htc_battery_pdev_data = {
	/* param */
	.guage_driver = 0,
	.chg_limit_active_mask = HTC_BATT_CHG_LIMIT_BIT_TALK |
								HTC_BATT_CHG_LIMIT_BIT_NAVI,
	.critical_low_voltage_mv = 3100,
	.critical_alarm_voltage_mv = 3000,
	.overload_vol_thr_mv = 4000,
	.overload_curr_thr_ma = 0,
	/* charger */
	.icharger.name = "pm8921",
	.icharger.get_charging_source = pm8921_get_charging_source,
	.icharger.get_charging_enabled = pm8921_get_charging_enabled,
	.icharger.set_charger_enable = pm8921_charger_enable,
	.icharger.set_pwrsrc_enable = pm8921_pwrsrc_enable,
	.icharger.set_pwrsrc_and_charger_enable =
						pm8921_set_pwrsrc_and_charger_enable,
	.icharger.set_limit_charge_enable = pm8921_limit_charge_enable,
	.icharger.is_ovp = pm8921_is_charger_ovp,
	.icharger.is_batt_temp_fault_disable_chg =
						pm8921_is_batt_temp_fault_disable_chg,
	.icharger.charger_change_notifier_register =
						cable_detect_register_notifier,
	.icharger.dump_all = pm8921_dump_all,
	.icharger.get_attr_text = pm8921_charger_get_attr_text,
	/* gauge */
	.igauge.name = "pm8921",
	.igauge.get_battery_voltage = pm8921_get_batt_voltage,
	.igauge.get_battery_current = pm8921_bms_get_batt_current,
	.igauge.get_battery_temperature = pm8921_get_batt_temperature,
	.igauge.get_battery_id = pm8921_get_batt_id,
	.igauge.get_battery_soc = pm8921_bms_get_batt_soc,
	.igauge.get_battery_cc = pm8921_bms_get_batt_cc,
	.igauge.is_battery_temp_fault = pm8921_is_batt_temperature_fault,
	.igauge.is_battery_full = pm8921_is_batt_full,
	.igauge.get_attr_text = pm8921_gauge_get_attr_text,
	.igauge.register_lower_voltage_alarm_notifier =
						pm8xxx_batt_lower_alarm_register_notifier,
	.igauge.enable_lower_voltage_alarm = pm8xxx_batt_lower_alarm_enable,
	.igauge.set_lower_voltage_alarm_threshold =
						pm8xxx_batt_lower_alarm_threshold_set,
};

static struct platform_device htc_battery_pdev = {
	.name = "htc_battery",
	.id = -1,
	.dev    = {
	.platform_data = &htc_battery_pdev_data,
	},
};
#endif /* CONFIG_HTC_BATT_8960 */

static struct platform_device *common_devices[] __initdata = {
	&ram_console_device,
	&msm8960_device_dmov,
	&msm_device_smd,
	&msm8960_device_uart_gsbi8,
	&msm_device_uart_dm6,
	&msm_device_saw_core0,
	&msm_device_saw_core1,
	&msm8960_device_ext_5v_vreg,
	&msm8960_device_ssbi_pmic,
	&msm8960_device_qup_i2c_gsbi2,
	&msm8960_device_qup_i2c_gsbi3,
	&msm8960_device_qup_i2c_gsbi4,
	&msm8960_device_qup_i2c_gsbi8,
#ifndef CONFIG_MSM_DSPS
	&msm8960_device_qup_i2c_gsbi12,
#endif
	&msm_slim_ctrl,
	&msm_device_wcnss_wlan,
#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)
	&qcrypto_device,
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)
	&qcedev_device,
#endif
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
	&msm_device_sps,
#ifdef CONFIG_MSM_FAKE_BATTERY
	&fish_battery_device,
#endif
	&fmem_device,
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	&android_pmem_device,
	&android_pmem_adsp_device,
#endif
	&android_pmem_audio_device,
#endif
	&msm_fb_device,
	&msm_device_vidc,
	&msm_device_bam_dmux,
	&msm_fm_platform_init,

#ifdef CONFIG_HW_RANDOM_MSM
	&msm_device_rng,
#endif
	&msm_rpm_device,
#ifdef CONFIG_ION_MSM
	&ion_dev,
#endif
	&msm_rpm_log_device,
	&msm_rpm_stat_device,
#ifdef CONFIG_MSM_QDSS
	&msm_etb_device,
	&msm_tpiu_device,
	&msm_funnel_device,
	&msm_etm_device,
#endif
	&msm8960_device_watchdog,
#ifdef CONFIG_MSM_RTB
	&msm_rtb_device,
#endif
	&msm8960_device_cache_erp,
#ifdef CONFIG_MSM_CACHE_DUMP
	&msm_cache_dump_device,
#endif
#ifdef CONFIG_HTC_BATT_8960
	&htc_battery_pdev,
#endif
};

static struct platform_device *fighter_devices[] __initdata = {
	&msm_8960_q6_lpass,
	&msm_8960_q6_mss_fw,
	&msm_8960_q6_mss_sw,
	&msm_8960_riva,
	&msm_pil_tzapps,
	&msm8960_device_otg,
	&msm_device_hsusb_host,
	&msm_pcm,
	&msm_pcm_routing,
	&msm_cpudai0,
	&msm_cpudai1,
	&msm_cpudai_hdmi_rx,
	&msm_cpudai_bt_rx,
	&msm_cpudai_bt_tx,
	&msm_cpudai_fm_rx,
	&msm_cpudai_fm_tx,
	&msm_cpudai_auxpcm_rx,
	&msm_cpudai_auxpcm_tx,
	&msm_cpu_fe,
	&msm_stub_codec,
	&msm_kgsl_3d0,
#ifdef CONFIG_MSM_KGSL_2D
	&msm_kgsl_2d0,
	&msm_kgsl_2d1,
#endif
#ifdef CONFIG_MSM_GEMINI
	&msm8960_gemini_device,
#endif
	&msm_voice,
	&msm_voip,
	&msm_lpa_pcm,
	&msm_cpudai_afe_01_rx,
	&msm_cpudai_afe_01_tx,
	&msm_cpudai_afe_02_rx,
	&msm_cpudai_afe_02_tx,
	&msm_pcm_afe,
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
	&hdmi_msm_device,
#endif
	&msm_compr_dsp,
	&msm_cpudai_incall_music_rx,
	&msm_cpudai_incall_record_rx,
	&msm_cpudai_incall_record_tx,
	&msm_pcm_hostless,
	&msm_bus_apps_fabric,
	&msm_bus_sys_fabric,
	&msm_bus_mm_fabric,
	&msm_bus_sys_fpb,
	&msm_bus_cpss_fpb,
	&msm_device_tz_log,
	&scm_memchk_device,
};

static void __init msm8960_i2c_init(void)
{
	msm8960_device_qup_i2c_gsbi4.dev.platform_data =
					&msm8960_i2c_qup_gsbi4_pdata;

	msm8960_device_qup_i2c_gsbi2.dev.platform_data =
					&msm8960_i2c_qup_gsbi2_pdata;

	msm8960_device_qup_i2c_gsbi3.dev.platform_data =
					&msm8960_i2c_qup_gsbi3_pdata;

	msm8960_device_qup_i2c_gsbi8.dev.platform_data =
					&msm8960_i2c_qup_gsbi8_pdata;

	msm8960_device_qup_i2c_gsbi12.dev.platform_data =
					&msm8960_i2c_qup_gsbi12_pdata;
}

static void __init msm8960_gfx_init(void)
{
	uint32_t soc_platform_version = socinfo_get_version();
	if (SOCINFO_VERSION_MAJOR(soc_platform_version) == 1) {
		struct kgsl_device_platform_data *kgsl_3d0_pdata =
				msm_kgsl_3d0.dev.platform_data;
		kgsl_3d0_pdata->pwrlevel[0].gpu_freq = 320000000;
		kgsl_3d0_pdata->pwrlevel[1].gpu_freq = 266667000;
		kgsl_3d0_pdata->nap_allowed = false;
	}
}

#ifdef CONFIG_HTC_BATT_8960
static struct pm8921_charger_batt_param chg_batt_params[] = {
	/* for normal type battery */
	[0] = {
		.max_voltage = 4200,
	},
	/* for HV type battery */
	[1] = {
	.max_voltage = 4340,
	},
};

static struct single_row_lut fcc_temp_id_1 = {
	.x	= {-20, -10, 0, 5, 10, 20, 30, 40},
	.y	= {900, 1050, 1350, 1450, 1520, 1600, 1620, 1630},
	.cols	= 8,
};

static struct single_row_lut fcc_sf_id_1 = {
	.x	= {100, 200, 300, 400, 500},
	.y	= {100, 100, 100, 100, 100},
	.cols	= 5,
};

static struct pc_sf_lut pc_sf_id_1 = {
	.rows		= 10,
	.cols		= 5,
	.cycles		= {100, 200, 300, 400, 500},
	.percent	= {100, 90, 80, 70, 60, 50, 40, 30, 20, 10},
	.sf		= {
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100}
	},
};

static struct pc_temp_ocv_lut  pc_temp_ocv_id_1 = {
	.rows		= 29,
	.cols		= 8,
	.temp		= {-20, -10,  0, 5, 10, 20, 30, 40},
	.percent	= {100, 95, 90, 85, 80, 75, 70, 65, 60, 55,
				50, 45, 40, 35, 30, 25, 20, 15, 10, 9,
				8, 7, 6, 5, 4, 3, 2, 1, 0
	},
	.ocv		= {
			{4290, 4290, 4290, 4290, 4290, 4290, 4290, 4290},
			{4268, 4268, 4268, 4268, 4266, 4263, 4263, 4259},
			{4215, 4215, 4215, 4215, 4212, 4207, 4206, 4203},
			{4166, 4166, 4166, 4166, 4161, 4153, 4152, 4148},
			{4118, 4118, 4118, 4118, 4111, 4101, 4100, 4097},
			{4073, 4073, 4073, 4073, 4065, 4053, 4051, 4048},
			{4029, 4029, 4029, 4029, 4021, 4008, 4006, 4004},
			{3989, 3989, 3989, 3989, 3981, 3969, 3966, 3964},
			{3950, 3950, 3950, 3950, 3942, 3930, 3928, 3926},
			{3903, 3903, 3903, 3903, 3892, 3875, 3872, 3872},
			{3863, 3863, 3863, 3863, 3852, 3840, 3839, 3839},
			{3836, 3836, 3836, 3836, 3827, 3818, 3817, 3816},
			{3816, 3816, 3816, 3816, 3808, 3800, 3800, 3799},
			{3799, 3799, 3799, 3799, 3793, 3787, 3785, 3785},
			{3787, 3787, 3787, 3787, 3782, 3778, 3775, 3773},
			{3779, 3779, 3779, 3779, 3776, 3770, 3760, 3754},
			{3772, 3772, 3772, 3772, 3767, 3751, 3738, 3733},
			{3759, 3759, 3759, 3759, 3746, 3716, 3699, 3698},
			{3732, 3732, 3732, 3732, 3708, 3683, 3681, 3677},
			{3725, 3725, 3725, 3725, 3702, 3676, 3671, 3665},
			{3717, 3717, 3717, 3717, 3695, 3668, 3661, 3653},
			{3710, 3710, 3710, 3710, 3689, 3660, 3651, 3641},
			{3702, 3702, 3702, 3702, 3682, 3652, 3641, 3629},
			{3694, 3694, 3694, 3694, 3675, 3644, 3630, 3616},
			{3688, 3688, 3688, 3688, 3665, 3608, 3592, 3579},
			{3682, 3682, 3682, 3682, 3654, 3571, 3554, 3542},
			{3676, 3676, 3676, 3676, 3644, 3534, 3516, 3505},
			{3670, 3670, 3670, 3670, 3633, 3497, 3478, 3468},
			{3663, 3663, 3663, 3663, 3622, 3460, 3440, 3430}
	},
};

struct pm8921_bms_battery_data  bms_battery_data_id_1 = {
	.fcc			= 1700,
	.fcc_temp_lut		= &fcc_temp_id_1,
	.fcc_sf_lut		= &fcc_sf_id_1,
	.pc_temp_ocv_lut	= &pc_temp_ocv_id_1,
	.pc_sf_lut		= &pc_sf_id_1,
};

static struct single_row_lut fcc_temp_id_2 = {
	.x	= {-20, -10, 0, 5, 10, 20, 30, 40},
	.y	= {900, 1050, 1360, 1540, 1500, 1600, 1620, 1630},
	.cols	= 8,
};

static struct single_row_lut fcc_sf_id_2 = {
	.x	= {100, 200, 300, 400, 500},
	.y	= {100, 100, 100, 100, 100},
	.cols	= 5,
};

static struct pc_sf_lut pc_sf_id_2 = {
	.rows		= 10,
	.cols		= 5,
	.cycles		= {100, 200, 300, 400, 500},
	.percent	= {100, 90, 80, 70, 60, 50, 40, 30, 20, 10},
	.sf		= {
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100}
	},
};

static struct pc_temp_ocv_lut  pc_temp_ocv_id_2 = {
	.rows		= 29,
	.cols		= 8,
	.temp		= {-20, -10,  0, 5, 10, 20, 30, 40},
	.percent	= {100, 95, 90, 85, 80, 75, 70, 65, 60, 55,
				50, 45, 40, 35, 30, 25, 20, 15, 10, 9,
				8, 7, 6, 5, 4, 3, 2, 1, 0
	},
	.ocv		= {
			{4290, 4290, 4290, 4290, 4290, 4290, 4290, 4290},
			{4264, 4264, 4264, 4264, 4263, 4260, 4259, 4257},
			{4212, 4212, 4212, 4212, 4210, 4206, 4204, 4202},
			{4163, 4163, 4163, 4163, 4160, 4154, 4151, 4150},
			{4116, 4116, 4116, 4116, 4111, 4105, 4101, 4100},
			{4072, 4072, 4072, 4072, 4066, 4057, 4054, 4053},
			{4026, 4026, 4026, 4026, 4021, 4014, 4008, 4008},
			{3979, 3979, 3979, 3979, 3977, 3972, 3967, 3967},
			{3933, 3933, 3933, 3933, 3926, 3918, 3910, 3912},
			{3893, 3893, 3893, 3893, 3884, 3876, 3871, 3873},
			{3862, 3862, 3862, 3862, 3855, 3848, 3844, 3845},
			{3838, 3838, 3838, 3838, 3831, 3825, 3821, 3822},
			{3817, 3817, 3817, 3817, 3812, 3806, 3802, 3803},
			{3800, 3800, 3800, 3800, 3795, 3790, 3786, 3789},
			{3785, 3785, 3785, 3785, 3781, 3775, 3763, 3765},
			{3773, 3773, 3773, 3773, 3767, 3754, 3734, 3740},
			{3754, 3754, 3754, 3754, 3741, 3717, 3697, 3704},
			{3724, 3724, 3724, 3724, 3708, 3696, 3690, 3688},
			{3704, 3704, 3704, 3704, 3696, 3687, 3668, 3668},
			{3702, 3702, 3702, 3702, 3691, 3666, 3646, 3646},
			{3699, 3699, 3699, 3699, 3685, 3645, 3624, 3624},
			{3697, 3697, 3697, 3697, 3680, 3624, 3601, 3601},
			{3694, 3694, 3694, 3694, 3674, 3603, 3579, 3579},
			{3691, 3691, 3691, 3691, 3668, 3582, 3556, 3556},
			{3677, 3677, 3677, 3677, 3639, 3542, 3517, 3517},
			{3663, 3663, 3663, 3663, 3609, 3501, 3477, 3478},
			{3649, 3649, 3649, 3649, 3579, 3460, 3438, 3439},
			{3635, 3635, 3635, 3635, 3549, 3419, 3398, 3400},
			{3621, 3621, 3621, 3621, 3519, 3378, 3358, 3360}
	},
};

struct pm8921_bms_battery_data  bms_battery_data_id_2 = {
	.fcc			= 1700,
	.fcc_temp_lut		= &fcc_temp_id_2,
	.fcc_sf_lut		= &fcc_sf_id_2,
	.pc_temp_ocv_lut	= &pc_temp_ocv_id_2,
	.pc_sf_lut		= &pc_sf_id_2,
};

static struct single_row_lut fcc_temp_id_3 = {
	.x	= {-20, -10, 0, 5, 10, 20, 30, 40},
	.y	= {1450, 1750, 1890, 2000, 2080, 2150, 2150, 2150},
	.cols	= 8,
};

static struct single_row_lut fcc_sf_id_3 = {
	.x	= {100, 200, 300, 400, 500},
	.y	= {100, 97, 93, 92, 90},
	.cols	= 5,
};

static struct pc_sf_lut pc_sf_id_3 = {
	.rows		= 10,
	.cols		= 5,
	.cycles		= {100, 200, 300, 400, 500},
	.percent	= {100, 90, 80, 70, 60, 50, 40, 30, 20, 10},
	.sf		= {
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100}
	},
};

static struct pc_temp_ocv_lut  pc_temp_ocv_id_3 = {
	.rows		= 29,
	.cols		= 8,
	.temp		= {-20, -10, 0, 5, 10, 20, 30, 40},
	.percent	= {100, 95, 90, 85, 80, 75, 70, 65, 60, 55,
				50, 45, 40, 35, 30, 25, 20, 15, 10, 9,
				8, 7, 6, 5, 4, 3, 2, 1, 0
	},
	.ocv		= {
			{4150, 4150, 4150, 4150, 4150, 4150, 4150, 4150},
			{4144, 4144, 4144, 4144, 4142, 4138, 4137, 4135},
			{4103, 4103, 4103, 4103, 4100, 4094, 4093, 4090},
			{4073, 4073, 4073, 4073, 4068, 4059, 4055, 4051},
			{4023, 4023, 4023, 4023, 4015, 4007, 4007, 4007},
			{3983, 3983, 3983, 3983, 3979, 3975, 3975, 3975},
			{3959, 3959, 3959, 3959, 3956, 3949, 3946, 3944},
			{3935, 3935, 3935, 3935, 3931, 3921, 3918, 3916},
			{3905, 3905, 3905, 3905, 3900, 3890, 3887, 3888},
			{3868, 3868, 3868, 3868, 3860, 3844, 3841, 3841},
			{3840, 3840, 3840, 3840, 3832, 3821, 3820, 3819},
			{3820, 3820, 3820, 3820, 3814, 3806, 3804, 3803},
			{3806, 3806, 3806, 3806, 3801, 3793, 3791, 3790},
			{3795, 3795, 3795, 3795, 3790, 3783, 3781, 3779},
			{3786, 3786, 3786, 3786, 3782, 3777, 3773, 3770},
			{3780, 3780, 3780, 3780, 3777, 3771, 3763, 3756},
			{3774, 3774, 3774, 3774, 3770, 3756, 3743, 3737},
			{3765, 3765, 3765, 3765, 3756, 3724, 3709, 3706},
			{3745, 3745, 3745, 3745, 3726, 3694, 3687, 3681},
			{3739, 3739, 3739, 3739, 3721, 3693, 3686, 3680},
			{3733, 3733, 3733, 3733, 3716, 3692, 3684, 3679},
			{3727, 3727, 3727, 3727, 3710, 3690, 3682, 3677},
			{3721, 3721, 3721, 3721, 3705, 3689, 3680, 3676},
			{3715, 3715, 3715, 3715, 3699, 3687, 3678, 3674},
			{3712, 3712, 3712, 3712, 3698, 3654, 3634, 3624},
			{3709, 3709, 3709, 3709, 3696, 3621, 3590, 3574},
			{3706, 3706, 3706, 3706, 3694, 3588, 3546, 3524},
			{3703, 3703, 3703, 3703, 3692, 3555, 3502, 3474},
			{3699, 3699, 3699, 3699, 3690, 3521, 3457, 3423}
	},
};

struct pm8921_bms_battery_data  bms_battery_data_id_3 = {
	.fcc			= 2150,
	.fcc_temp_lut		= &fcc_temp_id_3,
	.fcc_sf_lut		= &fcc_sf_id_3,
	.pc_temp_ocv_lut	= &pc_temp_ocv_id_3,
	.pc_sf_lut		= &pc_sf_id_3,
};

static struct htc_battery_cell htc_battery_cells[] = {
	[0] = { /* WTE/Sanyo R=10k */
		.model_name = "BJ53100",
		.capacity = 1700,
		.id = 1,
		.id_raw_min = 70, /* unit:mV (10kohm) */
		.id_raw_max = 204,
		.type = HTC_BATTERY_CELL_TYPE_HV,
		.voltage_max = 4340,
		.voltage_min = 3200,
		.chg_param = &chg_batt_params[1],
		.gauge_param = &bms_battery_data_id_1,
	},
	[1] = { /* TWS/Maxell R=22k */
		.model_name = "BJ53100",
		.capacity = 1700,
		.id = 2,
		.id_raw_min = 205, /* unit:mV (22kohm) */
		.id_raw_max = 386,
		.type = HTC_BATTERY_CELL_TYPE_HV,
		.voltage_max = 4340,
		.voltage_min = 3200,
		.chg_param = &chg_batt_params[1],
		.gauge_param = &bms_battery_data_id_2,
	},
	[2] = { /* WTC/Maxell R=47k */
		.model_name = "BJ53200",
		.capacity = 2150,
		.id = 3,
		.id_raw_min = 387, /* unit:mV (47kohm) */
		.id_raw_max = 595,
		.type = HTC_BATTERY_CELL_TYPE_NORMAL,
		.voltage_max = 4200,
		.voltage_min = 3200,
		.chg_param = &chg_batt_params[0],
		.gauge_param = &bms_battery_data_id_3,
	},
	[3] = {
		.model_name = "UNKNOWN",
		.capacity = 1700,
		.id = 255,
		.id_raw_min = INT_MIN,
		.id_raw_max = INT_MAX,
		.type = HTC_BATTERY_CELL_TYPE_NORMAL,
		.voltage_max = 4200,
		.voltage_min = 3200,
		.chg_param = &chg_batt_params[0],
		.gauge_param = NULL,
	},
};
#endif /* CONFIG_HTC_BATT_8960 */

static struct pm8xxx_irq_platform_data pm8xxx_irq_pdata __devinitdata = {
	.irq_base		= PM8921_IRQ_BASE,
	.devirq			= MSM_GPIO_TO_INT(104),
	.irq_trigger_flag	= IRQF_TRIGGER_LOW,
};

static struct pm8xxx_gpio_platform_data pm8xxx_gpio_pdata __devinitdata = {
	.gpio_base	= PM8921_GPIO_PM_TO_SYS(1),
};

static struct pm8xxx_mpp_platform_data pm8xxx_mpp_pdata __devinitdata = {
	.mpp_base	= PM8921_MPP_PM_TO_SYS(1),
};

static struct pm8xxx_rtc_platform_data pm8xxx_rtc_pdata __devinitdata = {
	.rtc_write_enable       = true,
	.rtc_alarm_powerup	= false,
};

static struct pm8xxx_pwrkey_platform_data pm8xxx_pwrkey_pdata = {
	.pull_up		= 1,
	.kpd_trigger_delay_us	= 970,
	.wakeup			= 1,
};

static int pm8921_therm_mitigation[] = {
	1100,
	700,
	600,
	225,
};

static struct pm8921_charger_platform_data pm8921_chg_pdata __devinitdata = {
	.safety_time		= 510,
	.update_time		= 60000,
	.max_voltage		= 4200,
	.min_voltage		= 3200,
	.resume_voltage_delta	= 50,
	.term_current		= 50,
	.cool_temp		= 0,
	.warm_temp		= 48,
	.temp_check_period	= 1,
	.max_bat_chg_current	= 1025,
	.cool_bat_chg_current	= 1025,
	.warm_bat_chg_current	= 1025,
	.cool_bat_voltage	= 4200,
	.warm_bat_voltage	= 4000,
	.mbat_in_gpio		= FIGHTER_MBAT_IN,
	.thermal_mitigation	= pm8921_therm_mitigation,
	.thermal_levels		= ARRAY_SIZE(pm8921_therm_mitigation),
	.cold_thr = PM_SMBC_BATT_TEMP_COLD_THR__HIGH,
	.hot_thr = PM_SMBC_BATT_TEMP_HOT_THR__LOW,
};

static struct pm8xxx_misc_platform_data pm8xxx_misc_pdata = {
	.priority		= 0,
};

static struct pm8921_bms_platform_data pm8921_bms_pdata __devinitdata = {
	.r_sense		= 10,
	.i_test			= 0,
	.v_failure		= 3000,
	.calib_delay_ms		= 600000,
	.max_voltage_uv		= 4200 * 1000,
};

static int __init check_dq_setup(char *str)
{
	if (!strcmp(str, "PASS")) {
		pr_info("[BATT] overwrite HV battery config\n");
		pm8921_chg_pdata.max_voltage = 4340;
		pm8921_chg_pdata.cool_bat_voltage = 4340;
		pm8921_bms_pdata.max_voltage_uv = 4340 * 1000;
	} else {
		pr_info("[BATT] use default battery config\n");
		pm8921_chg_pdata.max_voltage = 4200;
		pm8921_chg_pdata.cool_bat_voltage = 4200;
		pm8921_bms_pdata.max_voltage_uv = 4200 * 1000;
	}
	return 1;
}
__setup("androidboot.dq=", check_dq_setup);

static struct pm8xxx_gpio_init green_gpios[] = {
	PM8XXX_GPIO_OUTPUT_VIN_BB_FUNC(FIGHTER_GREEN_LED, 1, PM_GPIO_FUNC_2),
	PM8XXX_GPIO_OUTPUT_VIN_BB_FUNC(FIGHTER_GREEN_LED, 1, PM_GPIO_FUNC_NORMAL),
};

static struct pm8xxx_gpio_init amber_gpios[] = {
	PM8XXX_GPIO_OUTPUT_VIN_BB_FUNC(FIGHTER_AMBER_LED, 1, PM_GPIO_FUNC_2),
	PM8XXX_GPIO_OUTPUT_VIN_BB_FUNC(FIGHTER_AMBER_LED, 1, PM_GPIO_FUNC_NORMAL),
};

static void green_gpio_config(bool enable)
{
	if (enable)
		pm8xxx_gpio_config(green_gpios[0].gpio, &green_gpios[0].config);
	else
		pm8xxx_gpio_config(green_gpios[1].gpio, &green_gpios[1].config);
}

static void amber_gpio_config(bool enable)
{
	if (enable)
		pm8xxx_gpio_config(amber_gpios[0].gpio, &amber_gpios[0].config);
	else
		pm8xxx_gpio_config(amber_gpios[1].gpio, &amber_gpios[1].config);
}

static struct pm8xxx_led_configure pm8921_led_info[] = {
	[0] = {
		.name		= "button-backlight",
		.flags		= PM8XXX_ID_LED_0,
		.function_flags = LED_PWM_FUNCTION | LED_BRETH_FUNCTION,
		.period_us 	= USEC_PER_SEC / 1000,
		.start_index 	= 0,
		.duites_size 	= 8,
		.duty_time_ms 	= 64,
		.lut_flag 	= PM_PWM_LUT_RAMP_UP | PM_PWM_LUT_PAUSE_HI_EN,
		.out_current    = 40,
		.duties		= {0, 15, 30, 45, 60, 75, 90, 100,
				100, 90, 75, 60, 45, 30, 15, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0},
	},
	[1] = {
		.name           = "green",
		.flags		= PM8XXX_ID_GPIO24,
		.function_flags = LED_PWM_FUNCTION | LED_BLINK_FUNCTION,
		.gpio_status_switch = green_gpio_config,
	},
	[2] = {
		.name           = "amber",
		.flags		= PM8XXX_ID_GPIO25,
		.function_flags = LED_PWM_FUNCTION | LED_BLINK_FUNCTION,
		.gpio_status_switch = amber_gpio_config,
	},
};

static struct pm8xxx_led_platform_data pm8xxx_leds_pdata = {
	.num_leds = ARRAY_SIZE(pm8921_led_info),
	.leds = pm8921_led_info,
};

static struct pm8xxx_vibrator_platform_data pm8xxx_vib_pdata = {
	.initial_vibrate_ms = 0,
	.max_timeout_ms = 15000,
	.level_mV = 3100,
};

static struct pm8xxx_ccadc_platform_data pm8xxx_ccadc_pdata = {
	.r_sense		= 10,
};

static struct pm8921_platform_data pm8921_platform_data __devinitdata = {
	.irq_pdata		= &pm8xxx_irq_pdata,
	.gpio_pdata		= &pm8xxx_gpio_pdata,
	.mpp_pdata		= &pm8xxx_mpp_pdata,
	.rtc_pdata              = &pm8xxx_rtc_pdata,
	.pwrkey_pdata		= &pm8xxx_pwrkey_pdata,
	.misc_pdata		= &pm8xxx_misc_pdata,
	.regulator_pdatas	= msm_pm8921_regulator_pdata,
	.charger_pdata		= &pm8921_chg_pdata,
	.bms_pdata		= &pm8921_bms_pdata,
	.adc_pdata		= &pm8xxx_adc_pdata,
	.leds_pdata		= &pm8xxx_leds_pdata,
	.ccadc_pdata		= &pm8xxx_ccadc_pdata,
	.vibrator_pdata		= &pm8xxx_vib_pdata,
};

static struct msm_ssbi_platform_data msm8960_ssbi_pm8921_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name			= "pm8921-core",
		.platform_data		= &pm8921_platform_data,
	},
};


static struct msm_cpuidle_state msm_cstates[] __initdata = {
	{0, 0, "C0", "WFI",
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT},

	{0, 1, "C1", "STANDALONE_POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE},

	{0, 2, "C2", "POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE},

	{1, 0, "C0", "WFI",
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT},

	{1, 1, "C1", "STANDALONE_POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE},
};

static struct msm_pm_platform_data msm_pm_data[MSM_PM_SLEEP_MODE_NR * 2] = {
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
	},
};

static struct msm_rpmrs_level msm_rpmrs_levels[] = {
	{
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		100, 8000, 100000, 1,
	},
#if 0
	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		2000, 6000, 60100000, 3000,
	},
#endif
	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, GDHS, MAX, ACTIVE),
		false,
		4600, 5000, 60350000, 3500,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, HSFS_OPEN, MAX, ACTIVE),
		false,
		6700, 4500, 65350000, 4800,
	},
	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, HSFS_OPEN, ACTIVE, RET_HIGH),
		false,
		7400, 3500, 66600000, 5150,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, GDHS, MAX, ACTIVE),
		false,
		12100, 2500, 67850000, 5500,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, MAX, ACTIVE),
		false,
		14200, 2000, 71850000, 6800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, ACTIVE, RET_HIGH),
		false,
		30100, 500, 75850000, 8800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, RET_HIGH, RET_LOW),
		false,
		30100, 0, 76350000, 9800,
	},
};

static struct msm_pm_boot_platform_data msm_pm_boot_pdata __initdata = {
	.mode = MSM_PM_BOOT_CONFIG_TZ,
};

static uint32_t msm_rpm_get_swfi_latency(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(msm_rpmrs_levels); i++) {
		if (msm_rpmrs_levels[i].sleep_mode ==
			MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)
			return msm_rpmrs_levels[i].latency_us;
	}

	return 0;
}

static struct bma250_platform_data gsensor_bma250_platform_data = {
	.intr = FIGHTER_GSENSOR_INT,
	.chip_layout = 1,
};

static struct akm8975_platform_data compass_platform_data = {
	.layouts = FIGHTER_LAYOUTS,
};

static struct pn544_i2c_platform_data nfc_platform_data = {
	.irq_gpio = FIGHTER_NFC_IRQ,
	.ven_gpio = FIGHTER_NFC_VEN,
	.firm_gpio = FIGHTER_NFC_DL_MODE,
	.ven_isinvert = 1,
};

static struct i2c_board_info pn544_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO(PN544_I2C_NAME, 0x50 >> 1),
		.platform_data = &nfc_platform_data,
		.irq = MSM_GPIO_TO_INT(FIGHTER_NFC_IRQ),
	},
};

static struct i2c_board_info msm_i2c_gsbi12_info[] = {
	{
		I2C_BOARD_INFO(BMA250_I2C_NAME, 0x30 >> 1),
		.platform_data = &gsensor_bma250_platform_data,
		.irq = MSM_GPIO_TO_INT(FIGHTER_GSENSOR_INT),
	},
	{
		I2C_BOARD_INFO(AKM8975_I2C_NAME, 0x1A >> 1),
		.platform_data = &compass_platform_data,
		.irq = MSM_GPIO_TO_INT(FIGHTER_COMPASS_INT),
	},
};
static DEFINE_MUTEX(capella_cm36282_lock);
static struct regulator *PL_sensor_pwr;
static int capella_pl_sensor_lpm_power(uint8_t enable)
{
	int ret = 0;
	int rc;

	mutex_lock(&capella_cm36282_lock);
	if (PL_sensor_pwr == NULL)
	{
		PL_sensor_pwr = regulator_get(NULL, "8921_l16");
	}
	if (IS_ERR(PL_sensor_pwr)) {
		pr_err("[PS][cm3629] %s: Unable to get '8921_l16' \n", __func__);
		mutex_unlock(&capella_cm36282_lock);
		return -ENODEV;
	}
	if (enable == 1) {
		rc = regulator_set_optimum_mode(PL_sensor_pwr, 100);
		if (rc < 0)
			pr_err("[PS][cm3629] %s: enter lmp,set_optimum_mode l16 failed, rc=%d\n", __func__, rc);
		else
			pr_info("[PS][cm3629] %s: enter lmp,OK\n", __func__);
	} else {
		rc = regulator_set_optimum_mode(PL_sensor_pwr, 100000);
		if (rc < 0)
			pr_err("[PS][cm3629] %s: leave lmp,set_optimum_mode l16 failed, rc=%d\n", __func__, rc);
		else
			pr_info("[PS][cm3629] %s: leave lmp,OK\n", __func__);
		msleep(10);
	}
	mutex_unlock(&capella_cm36282_lock);
	return ret;
}
static int capella_cm36282_power(int pwr_device, uint8_t enable)
{
	int ret = 0;
	int rc;

	mutex_lock(&capella_cm36282_lock);

	if (PL_sensor_pwr == NULL)
	{
		PL_sensor_pwr = regulator_get(NULL, "8921_l16");
	}
	if (IS_ERR(PL_sensor_pwr)) {
		pr_err("[PS][cm3629] %s: Unable to get '8921_l16' \n", __func__);
		mutex_unlock(&capella_cm36282_lock);
		return -ENODEV;
	}
	if (enable == 1) {
		rc = regulator_set_voltage(PL_sensor_pwr, 2850000, 2850000);
		if (rc)
			pr_err("[PS][cm3629] %s: unable to regulator_set_voltage, rc:%d\n", __func__, rc);

		rc = regulator_enable(PL_sensor_pwr);
		if (rc)
			pr_err("[PS][cm3629]'%s' regulator enable l16 failed, rc=%d\n", __func__,rc);
		else
			pr_info("[PS][cm3629]'%s' l16 power on\n", __func__);
	}
	mutex_unlock(&capella_cm36282_lock);
	return ret;
}
static struct cm3629_platform_data cm36282_XD_pdata = {
	.model = CAPELLA_CM36282,
	.ps_select = CM3629_PS1_ONLY,
	.intr = PM8921_GPIO_PM_TO_SYS(FIGHTER_PROXIMITY_INTz),
	.levels = { 9, 19, 29, 399, 1000, 2575, 4200, 4428, 4655, 65535},
	.golden_adc = 3295,
	.power = capella_cm36282_power,
	.lpm_power = capella_pl_sensor_lpm_power,
	.cm3629_slave_address = 0xC0>>1,
	.ps1_thd_set = 17,
	.ps1_thd_no_cal = 0xF1,
	.ps1_thd_with_cal = 17,
	.ps_calibration_rule = 1,
	.ps_conf1_val = CM3629_PS_DR_1_80 | CM3629_PS_IT_2T |
			CM3629_PS1_PERS_3,
	.ps_conf2_val = CM3629_PS_ITB_2 | CM3629_PS_ITR_1 |
			CM3629_PS2_INT_DIS | CM3629_PS1_INT_DIS,
	.ps_conf3_val = CM3629_PS2_PROL_32,
};

static struct i2c_board_info i2c_CM36282_XD_devices[] = {
	{
		I2C_BOARD_INFO(CM3629_I2C_NAME, 0xC0 >> 1),
		.platform_data = &cm36282_XD_pdata,
		.irq =  PM8921_GPIO_IRQ(PM8921_IRQ_BASE, FIGHTER_PROXIMITY_INTz),
	},
};
static struct cm3629_platform_data cm36282_pdata = {
	.model = CAPELLA_CM36282,
	.ps_select = CM3629_PS1_ONLY,
	.intr = PM8921_GPIO_PM_TO_SYS(FIGHTER_PROXIMITY_INTz),
	.levels = { 9, 19, 29, 399, 1000, 2575, 4200, 4428, 4655, 65535},
	.golden_adc = 3295,
	.power = capella_cm36282_power,
	.lpm_power = capella_pl_sensor_lpm_power,
	.cm3629_slave_address = 0xC0>>1,
	.ps1_thd_set = 17,
	.ps1_thd_no_cal = 0xF1,
	.ps1_thd_with_cal = 17,
	.ps_calibration_rule = 1,
	.ps_conf1_val = CM3629_PS_DR_1_80 | CM3629_PS_IT_1_6T |
			CM3629_PS1_PERS_3,
	.ps_conf2_val = CM3629_PS_ITB_1 | CM3629_PS_ITR_1 |
			CM3629_PS2_INT_DIS | CM3629_PS1_INT_DIS,
	.ps_conf3_val = CM3629_PS2_PROL_32,
};

static struct i2c_board_info i2c_CM36282_devices[] = {
	{
		I2C_BOARD_INFO(CM3629_I2C_NAME, 0xC0 >> 1),
		.platform_data = &cm36282_pdata,
		.irq =  PM8921_GPIO_IRQ(PM8921_IRQ_BASE, FIGHTER_PROXIMITY_INTz),
	},
};
#ifdef CONFIG_I2C
#define I2C_SURF 1
#define I2C_FFA  (1 << 1)
#define I2C_RUMI (1 << 2)
#define I2C_SIM  (1 << 3)
#define I2C_FLUID (1 << 4)
#define I2C_LIQUID (1 << 5)

struct i2c_registry {
	u8                     machs;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

#ifdef CONFIG_MSM_CAMERA
static struct i2c_board_info msm_camera_boardinfo[] __initdata = {
#ifdef CONFIG_S5K3H2YX
        {
        I2C_BOARD_INFO("s5k3h2yx", 0x20 >> 1),
        },
#endif
//HTC start Tom Lin 2011/12/19
#ifdef CONFIG_IMX105
	{
	I2C_BOARD_INFO("imx105", 0x1A), 
	},
#endif
//HTC end Tom Lin 2011/12/19
#ifdef CONFIG_MT9V113
        {
        I2C_BOARD_INFO("mt9v113", 0x3C),
        },
#endif
#ifdef CONFIG_S5K6A1GX
        {
        I2C_BOARD_INFO("s5k6a1gx", 0x6C >> 1),
        },
#endif
#ifdef CONFIG_IMX074
	{
	I2C_BOARD_INFO("imx074", 0x1A),
	},
#endif
#ifdef CONFIG_OV2720
	{
	I2C_BOARD_INFO("ov2720", 0x6C),
	},
#endif
#ifdef CONFIG_MSM_CAMERA_FLASH_SC628A
	{
	I2C_BOARD_INFO("sc628a", 0x6E),
	},
#endif
};
#endif

#ifdef CONFIG_AMP_TPA2051D3
static struct tpa2051d3_platform_data tpa2051d3_pdata = {
	.spkr_cmd = {0x00, 0x82, 0x27, 0x57, 0x13, 0x0D, 0x0D},
	.hsed_cmd = {0x00, 0x0C, 0x25, 0x57, 0x6D, 0x4D, 0x0D},
	.rece_cmd = {0x00, 0x82, 0x25, 0x57, 0x2D, 0x4D, 0x0D},
};

#define TPA2051D3_I2C_SLAVE_ADDR	(0xE0 >> 1)

static struct i2c_board_info msm_i2c_gsbi8_tpa2051d3_info[] = {
	{
		I2C_BOARD_INFO(TPA2051D3_I2C_NAME, TPA2051D3_I2C_SLAVE_ADDR),
		.platform_data = &tpa2051d3_pdata,
	},
};
#endif

static struct i2c_registry msm8960_i2c_devices[] __initdata = {
#ifdef CONFIG_MSM_CAMERA
	{
		I2C_SURF | I2C_FFA | I2C_FLUID | I2C_LIQUID | I2C_RUMI,
		MSM_8960_GSBI4_QUP_I2C_BUS_ID,
		msm_camera_boardinfo,
		ARRAY_SIZE(msm_camera_boardinfo),
	},
#endif
	{
		I2C_SURF | I2C_FFA,
		MSM_8960_GSBI12_QUP_I2C_BUS_ID,
		msm_i2c_gsbi12_info,
		ARRAY_SIZE(msm_i2c_gsbi12_info),
	},
#ifdef CONFIG_FLASHLIGHT_TPS61310
	{
		I2C_SURF | I2C_FFA,
		MSM_8960_GSBI12_QUP_I2C_BUS_ID,
		i2c_tps61310_flashlight,
		ARRAY_SIZE(i2c_tps61310_flashlight),
	},
#endif
	{
		I2C_SURF | I2C_FFA,
		MSM_8960_GSBI3_QUP_I2C_BUS_ID,
		msm_i2c_gsbi3_info,
		ARRAY_SIZE(msm_i2c_gsbi3_info),
	},
	{
		I2C_SURF | I2C_FFA,
		MSM_8960_GSBI2_QUP_I2C_BUS_ID,
		pn544_i2c_boardinfo,
		ARRAY_SIZE(pn544_i2c_boardinfo),
	},
#ifdef CONFIG_AMP_TPA2051D3
	{
		I2C_SURF | I2C_FFA,
		MSM_8960_GSBI8_QUP_I2C_BUS_ID,
		msm_i2c_gsbi8_tpa2051d3_info,
		ARRAY_SIZE(msm_i2c_gsbi8_tpa2051d3_info),
	},
#endif
};
#endif /* CONFIG_I2C */

static void __init register_i2c_devices(void)
{
#ifdef CONFIG_I2C
	u8 mach_mask = 0;
	int i;

	mach_mask = I2C_SURF;

	/* Run the array and install devices as appropriate */
	for (i = 0; i < ARRAY_SIZE(msm8960_i2c_devices); ++i) {
		if (msm8960_i2c_devices[i].machs & mach_mask) {
			i2c_register_board_info(msm8960_i2c_devices[i].bus,
						msm8960_i2c_devices[i].info,
						msm8960_i2c_devices[i].len);
		}
	}
	if (system_rev < 3) {
		i2c_register_board_info(MSM_8960_GSBI12_QUP_I2C_BUS_ID,
			i2c_CM36282_devices, ARRAY_SIZE(i2c_CM36282_devices));
		pr_info("%s: cm36282 PL-sensor for XA,XB,XC, system_rev %d ",
				 __func__, system_rev);
	} else {
		i2c_register_board_info(MSM_8960_GSBI12_QUP_I2C_BUS_ID,
			i2c_CM36282_XD_devices,	ARRAY_SIZE(i2c_CM36282_XD_devices));
		pr_info("%s: cm36282 PL-sensor for XD and newer HW version, system_rev %d ",
				__func__, system_rev);
	}
#endif
}

/*UART -> GSBI8*/
static uint32_t msm_uart_gpio[] = {
	GPIO_CFG(34, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(35, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
};

static void msm_uart_gsbi_gpio_init(void)
{
	gpio_tlmm_config(msm_uart_gpio[0], GPIO_CFG_ENABLE);
	gpio_tlmm_config(msm_uart_gpio[1], GPIO_CFG_ENABLE);
}

static void __init fighter_init(void)
{
	int rc = 0, i = 0;
	struct kobject *properties_kobj;

	if (meminfo_init(SYS_MEMORY, SZ_256M) < 0)
		pr_err("meminfo_init() failed!\n");

	msm_tsens_early_init(&msm_tsens_pdata);
	BUG_ON(msm_rpm_init(&msm_rpm_data));
	BUG_ON(msm_rpmrs_levels_init(msm_rpmrs_levels,
				ARRAY_SIZE(msm_rpmrs_levels)));
	msm_rpmrs_lpm_init(1, 1, 1, 1);

	pmic_reset_irq = PM8921_IRQ_BASE + PM8921_RESOUT_IRQ;
	regulator_suppress_info_printing();
	if (msm_xo_init())
		pr_err("Failed to initialize XO votes\n");
	platform_device_register(&msm8960_device_rpm_regulator);
	msm_clock_init(&msm8960_clock_init_data);

	/* added by htc for clock debugging */
	clk_ignor_list_add("msm_serial_hsl.0", "core_clk");

	msm_otg_pdata.swfi_latency = msm_rpmrs_levels[0].latency_us;
	msm8960_device_otg.dev.platform_data = &msm_otg_pdata;
	fighter_gpiomux_init();

	msm8960_device_ssbi_pmic.dev.platform_data =
				&msm8960_ssbi_pm8921_pdata;
	pm8921_platform_data.num_regulators = msm_pm8921_regulator_pdata_len;
	msm8960_i2c_init();
	msm8960_gfx_init();
	msm_spm_init(msm_spm_data, ARRAY_SIZE(msm_spm_data));
	msm_spm_l2_init(msm_spm_l2_data);
	msm8960_init_buses();
#ifdef CONFIG_BT
	bt_export_bd_address();
#endif
#ifdef CONFIG_HTC_BATT_8960
	htc_battery_cell_init(htc_battery_cells, ARRAY_SIZE(htc_battery_cells));
#endif /* CONFIG_HTC_BATT_8960 */
	platform_add_devices(msm_footswitch_devices,
		msm_num_footswitch_devices);
	platform_device_register(&msm8960_device_ext_l2_vreg);
	platform_add_devices(common_devices, ARRAY_SIZE(common_devices));
	msm_uart_gsbi_gpio_init();
	pm8921_gpio_mpp_init();
	platform_add_devices(fighter_devices, ARRAY_SIZE(fighter_devices));
	msm8960_init_cam();
	msm8960_init_mmc();
	acpuclk_init(&acpuclk_8960_soc_data);
	if (board_mfg_mode() == 1) {
		for (i = 0; i < ARRAY_SIZE(syn_ts_3k_data);  i++)
			syn_ts_3k_data[i].mfg_flag = 1;
	}
	register_i2c_devices();
	msm_fb_add_devices();
	slim_register_board_info(msm_slim_devices,
		ARRAY_SIZE(msm_slim_devices));
	msm_pm_set_platform_data(msm_pm_data, ARRAY_SIZE(msm_pm_data));
	msm_pm_set_rpm_wakeup_irq(RPM_APCC_CPU0_WAKE_UP_IRQ);
	msm_cpuidle_set_states(msm_cstates, ARRAY_SIZE(msm_cstates),
				msm_pm_data);
	change_memory_power = &msm8960_change_memory_power;
	create_proc_read_entry("emmc", 0, NULL, emmc_partition_read_proc, NULL);
	create_proc_read_entry("dying_processes", 0, NULL, dying_processors_read_proc, NULL);

#ifdef CONFIG_PERFLOCK
	perflock_init(&fighter_perflock_data);
	cpufreq_ceiling_init(&fighter_cpufreq_ceiling_data);
#endif

#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_2_PHASE
	if(!cpu_is_krait_v1())
		set_two_phase_freq(1134000);
#endif

	/*usb driver won't be loaded in MFG 58 station and gift mode*/
	if (!(board_mfg_mode() == 6 || board_mfg_mode() == 7))
		fighter_add_usb_devices();
	properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (properties_kobj)
		rc = sysfs_create_group(properties_kobj,
			&properties_attr_group);
	fighter_init_keypad();
	BUG_ON(msm_pm_boot_init(&msm_pm_boot_pdata));
        if (get_kernel_flag() & KERNEL_FLAG_PM_MONITOR) {
		htc_monitor_init();
		htc_pm_monitor_init();
	}
	msm_pm_init_sleep_status_data(&msm_pm_slp_sts_data);
	msm_pm_network_info_init(MSM_SHARED_RAM_BASE + 0x1F00C0);
}

#define PHY_BASE_ADDR1  0x80400000
#define SIZE_ADDR1      (132 * 1024 * 1024)

#define PHY_BASE_ADDR2  0x90000000
#define SIZE_ADDR2      (768 * 1024 * 1024)

static void __init fighter_fixup(struct machine_desc *desc, struct tag *tags,
				 char **cmdline, struct meminfo *mi)
{
	engineerid = parse_tag_engineerid(tags);
	mi->nr_banks = 2;
	mi->bank[0].start = PHY_BASE_ADDR1;
	mi->bank[0].size = SIZE_ADDR1;
	mi->bank[1].start = PHY_BASE_ADDR2;
	mi->bank[1].size = SIZE_ADDR2;
}


MACHINE_START(FIGHTER, "fighter")
	.fixup = fighter_fixup,
	.map_io = fighter_map_io,
	.reserve = fighter_reserve,
	.init_irq = fighter_init_irq,
	.handle_irq = gic_handle_irq,
	.timer = &msm_timer,
	.init_machine = fighter_init,
	.init_early = msm8960_allocate_memory_regions,
	.init_very_early = fighter_early_memory,
MACHINE_END
