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
#include <linux/gpio.h>
#include <linux/usb/msm_hsusb.h>
#include <linux/usb/android.h>
#include <linux/msm_ssbi.h>
#include <linux/pn544.h>
#include <linux/regulator/msm-gpio-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/slimbus/slimbus.h>
#include <sound/tpa2051d3.h>
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif
#include <linux/atmel_qt602240.h>
#include <linux/synaptics_i2c_rmi.h>
#include <linux/cyttsp-qc.h>
#include <linux/dma-contiguous.h>
#include <linux/dma-mapping.h>
#include <linux/platform_data/qcom_crypto_device.h>
#include <linux/platform_data/qcom_wcnss_device.h>
#include <linux/leds.h>
#include <linux/leds-pm8xxx-htc.h>
#include <linux/msm_tsens.h>
#include <linux/proc_fs.h>
#include <linux/cm3629.h>
#include <linux/memblock.h>
#include <linux/msm_thermal.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>
#include <asm/hardware/gic.h>
#include <asm/mach/mmc.h>

#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/msm_spi.h>
#ifdef CONFIG_USB_MSM_OTG_72K
#include <mach/msm_hsusb.h>
#else
#include <linux/usb/msm_hsusb.h>
#endif
#include <mach/htc_restart_handler.h>
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
#include <mach/panel_id.h>
#ifdef CONFIG_WCD9310_CODEC
#include <linux/slimbus/slimbus.h>
#include <linux/mfd/wcd9xxx/core.h>
#include <linux/mfd/wcd9xxx/pdata.h>
#endif
#include <linux/msm_ion.h>
#include <mach/ion.h>

#include <mach/msm_rtb.h>
#include <mach/msm_cache_dump.h>
#include <mach/scm.h>
#include <mach/iommu_domains.h>

#include <mach/kgsl.h>
#include <linux/fmem.h>

#include <linux/akm8975.h>
#include <linux/bma250.h>
#include <linux/ewtzmu2.h>
#ifdef CONFIG_BT
#include <mach/htc_bdaddress.h>
#endif
#include <mach/htc_headset_mgr.h>
#include <mach/htc_headset_pmic.h>
#include <mach/cable_detect.h>

#include "timer.h"
#include "devices.h"
#include "devices-msm8x60.h"
#include "spm.h"
#include "board-fighter.h"
#include "pm.h"
#include <mach/cpuidle.h>
#include "rpm_resources.h"
#include <mach/mpm.h>
#include "acpuclock.h"
#include "rpm_log.h"
#include "smd_private.h"
#include "pm-boot.h"

#ifdef CONFIG_FB_MSM_HDMI_MHL
#include <mach/mhl.h>
#endif

#ifdef CONFIG_MSM_CAMERA_FLASH
#include <linux/htc_flashlight.h>
#endif
#include <mach/board_htc.h>
#ifdef CONFIG_HTC_BATT_8960
#include "mach/htc_battery_8960.h"
#include "mach/htc_battery_cell.h"
#include "linux/mfd/pm8xxx/pm8921-charger-htc.h"
#endif

extern unsigned int engineerid;

#define HW_VER_ID_VIRT		(MSM_TLMM_BASE + 0x00002054)

unsigned skuid;
#ifdef CONFIG_MSM_CAMERA_FLASH
#ifdef CONFIG_FLASHLIGHT_TPS61310
static void config_flashlight_gpios(void)
{
	static uint32_t flashlight_gpio_table[] = {
		GPIO_CFG(FIGHTER_GPIO_TORCH_FLASHz, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG(FIGHTER_GPIO_DRIVER_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	};

	gpio_tlmm_config(flashlight_gpio_table[0], GPIO_CFG_ENABLE);
	gpio_tlmm_config(flashlight_gpio_table[1], GPIO_CFG_ENABLE);
}

static struct TPS61310_flashlight_platform_data fighter_flashlight_data = {
	.gpio_init = config_flashlight_gpios,
	.tps61310_strb0 = FIGHTER_GPIO_DRIVER_EN,
	.tps61310_strb1 = FIGHTER_GPIO_TORCH_FLASHz,
	.flash_duration_ms = 600,
	.mode_pin_suspend_state_low = 1,
};

static struct i2c_board_info i2c_tps61310_flashlight[] = {
	{
		I2C_BOARD_INFO("TPS61310_FLASHLIGHT", 0x66 >> 1),
		.platform_data = &fighter_flashlight_data,
	},
};
#endif
#endif

static struct platform_device msm_fm_platform_init = {
	.name = "iris_fm",
	.id   = -1,
};

#if defined(CONFIG_GPIO_SX150X) || defined(CONFIG_GPIO_SX150X_MODULE)
enum {
	GPIO_EXPANDER_IRQ_BASE = (PM8921_IRQ_BASE + PM8921_NR_IRQS),
	GPIO_EXPANDER_GPIO_BASE = (PM8921_MPP_BASE + PM8921_NR_MPPS),
	/* CAM Expander */
	GPIO_CAM_EXPANDER_BASE = GPIO_EXPANDER_GPIO_BASE,
	GPIO_CAM_GP_STROBE_READY = GPIO_CAM_EXPANDER_BASE,
	GPIO_CAM_GP_AFBUSY,
	GPIO_CAM_GP_STROBE_CE,
	GPIO_CAM_GP_CAM1MP_XCLR,
	GPIO_CAM_GP_CAMIF_RESET_N,
	GPIO_CAM_GP_XMT_FLASH_INT,
	GPIO_CAM_GP_LED_EN1,
	GPIO_CAM_GP_LED_EN2,

};
#endif

#define MSM_PMEM_ADSP_SIZE         0x6D00000
#define MSM_PMEM_AUDIO_SIZE        0x4CF000
#define MSM_PMEM_SIZE 0x2800000 /* 40 Mbytes */
#define MSM_LIQUID_PMEM_SIZE 0x4000000 /* 64 Mbytes */
#define MSM_HDMI_PRIM_PMEM_SIZE 0x4000000 /* 64 Mbytes */

#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
#define HOLE_SIZE	0x20000
#define MSM_CONTIG_MEM_SIZE  0x65000
#ifdef CONFIG_MSM_IOMMU
#define MSM_ION_MM_SIZE            0x3800000 /* Need to be multiple of 64K */
#define MSM_ION_SF_SIZE            0x0
#define MSM_ION_QSECOM_SIZE        0x780000 /* (7.5MB) */
#define MSM_ION_HEAP_NUM	7
#else
#define MSM_ION_MM_SIZE            MSM_PMEM_ADSP_SIZE
#define MSM_ION_SF_SIZE            MSM_PMEM_SIZE
#define MSM_ION_QSECOM_SIZE        0x600000 /* (6MB) */
#define MSM_ION_HEAP_NUM	8
#endif
#define MSM_ION_MM_FW_SIZE	(0x200000 - HOLE_SIZE) /* 128kb */
#define MSM_ION_MFC_SIZE	SZ_8K
#define MSM_ION_AUDIO_SIZE	MSM_PMEM_AUDIO_SIZE

#define MSM_LIQUID_ION_MM_SIZE (MSM_ION_MM_SIZE + 0x600000)
#define MSM_LIQUID_ION_SF_SIZE MSM_LIQUID_PMEM_SIZE
#define MSM_HDMI_PRIM_ION_SF_SIZE MSM_HDMI_PRIM_PMEM_SIZE

#define MSM_MM_FW_SIZE		(0x200000 - HOLE_SIZE) /* 2mb -128kb*/
#define MSM8960_FIXED_AREA_START (0xa0000000 - (MSM_ION_MM_FW_SIZE + \
							HOLE_SIZE))
#define MAX_FIXED_AREA_SIZE	0x10000000
#define MSM8960_FW_START	MSM8960_FIXED_AREA_START
#define MSM_ION_ADSP_SIZE	SZ_8M
#else
#define MSM_CONTIG_MEM_SIZE  0x110C000
#define MSM_ION_HEAP_NUM	1
#endif

static unsigned msm_contig_mem_size = MSM_CONTIG_MEM_SIZE;
#ifdef CONFIG_KERNEL_MSM_CONTIG_MEM_REGION
static int __init msm_contig_mem_size_setup(char *p)
{
	msm_contig_mem_size = memparse(p, NULL);
	return 0;
}
early_param("msm_contig_mem_size", msm_contig_mem_size_setup);
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
#endif /*CONFIG_MSM_MULTIMEDIA_USE_ION*/
#endif /*CONFIG_ANDROID_PMEM*/

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

static void __init reserve_rtb_memory(void)
{
#if defined(CONFIG_MSM_RTB)
	msm8960_reserve_table[MEMTYPE_EBI1].size += msm8960_rtb_pdata.size;
	pr_info("mem_map: rtb reserved with size 0x%x in pool\n",
			msm8960_rtb_pdata.size);
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
	android_pmem_audio_pdata.size = MSM_PMEM_AUDIO_SIZE;
#endif /*CONFIG_MSM_MULTIMEDIA_USE_ION*/
#endif /*CONFIG_ANDROID_PMEM*/
}

#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static void __init reserve_memory_for(struct android_pmem_platform_data *p)
{
	msm8960_reserve_table[p->memory_type].size += p->size;
}
#endif /*CONFIG_MSM_MULTIMEDIA_USE_ION*/
#endif /*CONFIG_ANDROID_PMEM*/


static void __init reserve_pmem_memory(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	reserve_memory_for(&android_pmem_adsp_pdata);
	reserve_memory_for(&android_pmem_pdata);
	reserve_memory_for(&android_pmem_audio_pdata);
#endif
	msm8960_reserve_table[MEMTYPE_EBI1].size += msm_contig_mem_size;
#endif
}

static int msm8960_paddr_to_memtype(unsigned int paddr)
{
	return MEMTYPE_EBI1;
}

#define FMEM_ENABLED 0

#ifdef CONFIG_ION_MSM
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct ion_cp_heap_pdata cp_mm_ion_pdata = {
	.permission_type = IPT_TYPE_MM_CARVEOUT,
	.align = PAGE_SIZE,
	.reusable = FMEM_ENABLED,
	.mem_is_fmem = FMEM_ENABLED,
	.fixed_position = FIXED_MIDDLE,
	.iommu_map_all = 1,
	.iommu_2x_map_domain = VIDEO_DOMAIN,
#ifdef CONFIG_CMA
	.is_cma = 1,
#endif
};

static struct ion_cp_heap_pdata cp_mfc_ion_pdata = {
	.permission_type = IPT_TYPE_MFC_SHAREDMEM,
	.align = PAGE_SIZE,
	.reusable = 0,
	.mem_is_fmem = FMEM_ENABLED,
	.fixed_position = FIXED_HIGH,
};

static struct ion_co_heap_pdata co_ion_pdata = {
	.adjacent_mem_id = INVALID_HEAP_ID,
	.align = PAGE_SIZE,
	.mem_is_fmem = 0,
};

static struct ion_co_heap_pdata fw_co_ion_pdata = {
	.adjacent_mem_id = ION_CP_MM_HEAP_ID,
	.align = SZ_128K,
	.mem_is_fmem = FMEM_ENABLED,
	.fixed_position = FIXED_LOW,
};
#endif

static u64 msm_dmamask = DMA_BIT_MASK(32);

static struct platform_device ion_mm_heap_device = {
	.name = "ion-mm-heap-device",
	.id = -1,
	.dev = {
		.dma_mask = &msm_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	}
};

#ifdef CONFIG_CMA
static struct platform_device ion_adsp_heap_device = {
	.name = "ion-adsp-heap-device",
	.id = -1,
	.dev = {
		.dma_mask = &msm_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	}
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
struct ion_platform_heap msm8960_heaps[] = {
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
			.priv	= &ion_mm_heap_device.dev,
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
		{
			.id	= ION_AUDIO_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_AUDIO_HEAP_NAME,
			.size	= MSM_ION_AUDIO_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_ion_pdata,
		},
#ifdef CONFIG_CMA
		{
			.id	= ION_ADSP_HEAP_ID,
			.type	= ION_HEAP_TYPE_DMA,
			.name	= ION_ADSP_HEAP_NAME,
			.size	= MSM_ION_ADSP_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_ion_pdata,
			.priv	= &ion_adsp_heap_device.dev,
		},
#endif
#endif
};

static struct ion_platform_data msm8960_ion_pdata = {
	.nr = MSM_ION_HEAP_NUM,
	.heaps = msm8960_heaps,
};

static struct platform_device ion_dev = {
	.name = "ion-msm",
	.id = 1,
	.dev = { .platform_data = &msm8960_ion_pdata },
};
#endif

struct platform_device fmem_device = {
	.name = "fmem",
	.id = 1,
	.dev = { .platform_data = &fmem_pdata },
};

static void __init reserve_mem_for_ion(enum ion_memory_types mem_type,
				      unsigned long size)
{
	msm8960_reserve_table[MEMTYPE_EBI1].size += size;
}

static void __init msm8960_reserve_fixed_area(unsigned long fixed_area_size)
{
#if defined(CONFIG_ION_MSM) && defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	int ret;

	if (fixed_area_size > MAX_FIXED_AREA_SIZE)
		panic("fixed area size is larger than %dM\n",
			MAX_FIXED_AREA_SIZE >> 20);

	reserve_info->fixed_area_size = fixed_area_size;
	reserve_info->fixed_area_start = MSM8960_FW_START;

	ret = memblock_remove(reserve_info->fixed_area_start,
		reserve_info->fixed_area_size);
	BUG_ON(ret);
#endif
}

/**
 * Reserve memory for ION and calculate amount of reusable memory for fmem.
 * We only reserve memory for heaps that are not reusable. However, we only
 * support one reusable heap at the moment so we ignore the reusable flag for
 * other than the first heap with reusable flag set. Also handle special case
 * for video heaps (MM,FW, and MFC). Video requires heaps MM and MFC to be
 * at a higher address than FW in addition to not more than 256MB away from the
 * base address of the firmware. This means that if MM is reusable the other
 * two heaps must be allocated in the same region as FW. This is handled by the
 * mem_is_fmem flag in the platform data. In addition the MM heap must be
 * adjacent to the FW heap for content protection purposes.
 */
static void __init reserve_ion_memory(void)
{
#if defined(CONFIG_ION_MSM) && defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	unsigned int i;
	int ret;
	unsigned int fixed_size = 0;
	unsigned int fixed_low_size, fixed_middle_size, fixed_high_size;
	unsigned long fixed_low_start, fixed_middle_start, fixed_high_start;
	unsigned long cma_alignment;
	unsigned int low_use_cma = 0;
	unsigned int middle_use_cma = 0;
	unsigned int high_use_cma = 0;

	fixed_low_size = 0;
	fixed_middle_size = 0;
	fixed_high_size = 0;

	cma_alignment = PAGE_SIZE << max(MAX_ORDER, pageblock_order);

	for (i = 0; i < msm8960_ion_pdata.nr; ++i) {
		struct ion_platform_heap *heap =
						&(msm8960_ion_pdata.heaps[i]);
		int align = SZ_4K;
		int iommu_map_all = 0;
		int adjacent_mem_id = INVALID_HEAP_ID;
		int use_cma = 0;

		if (heap->extra_data) {
			int fixed_position = NOT_FIXED;

			switch ((int) heap->type) {
			case ION_HEAP_TYPE_CP:
				fixed_position = ((struct ion_cp_heap_pdata *)
					heap->extra_data)->fixed_position;
				align = ((struct ion_cp_heap_pdata *)
						heap->extra_data)->align;
				iommu_map_all =
					((struct ion_cp_heap_pdata *)
					heap->extra_data)->iommu_map_all;
				if (((struct ion_cp_heap_pdata *)
					heap->extra_data)->is_cma) {
					heap->size = ALIGN(heap->size,
							cma_alignment);
					use_cma = 1;
				}
				break;
			case ION_HEAP_TYPE_DMA:
					use_cma = 1;
				/* Purposely fall through here */
			case ION_HEAP_TYPE_CARVEOUT:
				fixed_position = ((struct ion_co_heap_pdata *)
					heap->extra_data)->fixed_position;
				adjacent_mem_id = ((struct ion_co_heap_pdata *)
					heap->extra_data)->adjacent_mem_id;
				break;
			default:
				break;
			}

			if (iommu_map_all) {
				if (heap->size & (SZ_64K-1)) {
					heap->size = ALIGN(heap->size, SZ_64K);
					pr_info("Heap %s not aligned to 64K. Adjusting size to %x\n",
						heap->name, heap->size);
				}
			}

			if (fixed_position != NOT_FIXED)
				fixed_size += heap->size;
			else if (!use_cma)
				reserve_mem_for_ion(MEMTYPE_EBI1, heap->size);

			if (fixed_position == FIXED_LOW) {
				fixed_low_size += heap->size;
				low_use_cma = use_cma;
			} else if (fixed_position == FIXED_MIDDLE) {
				fixed_middle_size += heap->size;
				middle_use_cma = use_cma;
			} else if (fixed_position == FIXED_HIGH) {
				fixed_high_size += heap->size;
				high_use_cma = use_cma;
			} else if (use_cma) {
				/*
				 * Heaps that use CMA but are not part of the
				 * fixed set. Create wherever.
				 */
				dma_declare_contiguous(
					heap->priv,
					heap->size,
					0,
					0xb0000000);
			}
		}
	}

	if (!fixed_size)
		return;

	/*
	 * Given the setup for the fixed area, we can't round up all sizes.
	 * Some sizes must be set up exactly and aligned correctly. Incorrect
	 * alignments are considered a configuration issue
	 */

	fixed_low_start = MSM8960_FIXED_AREA_START;
	if (low_use_cma) {
		BUG_ON(!IS_ALIGNED(fixed_low_start, cma_alignment));
		BUG_ON(!IS_ALIGNED(fixed_low_size + HOLE_SIZE, cma_alignment));
	} else {
		BUG_ON(!IS_ALIGNED(fixed_low_size + HOLE_SIZE, SECTION_SIZE));
		ret = memblock_remove(fixed_low_start,
				      fixed_low_size + HOLE_SIZE);
		BUG_ON(ret);
	}

	fixed_middle_start = fixed_low_start + fixed_low_size + HOLE_SIZE;
	if (middle_use_cma) {
		BUG_ON(!IS_ALIGNED(fixed_middle_start, cma_alignment));
		BUG_ON(!IS_ALIGNED(fixed_middle_size, cma_alignment));
	} else {
		BUG_ON(!IS_ALIGNED(fixed_middle_size, SECTION_SIZE));
		ret = memblock_remove(fixed_middle_start, fixed_middle_size);
		BUG_ON(ret);
	}

	fixed_high_start = fixed_middle_start + fixed_middle_size;
	if (high_use_cma) {
		fixed_high_size = ALIGN(fixed_high_size, cma_alignment);
		BUG_ON(!IS_ALIGNED(fixed_high_start, cma_alignment));
	} else {
		/* This is the end of the fixed area so it's okay to round up */
		fixed_high_size = ALIGN(fixed_high_size, SECTION_SIZE);
		ret = memblock_remove(fixed_high_start, fixed_high_size);
		BUG_ON(ret);
	}



	for (i = 0; i < msm8960_ion_pdata.nr; ++i) {
		struct ion_platform_heap *heap = &(msm8960_ion_pdata.heaps[i]);

		if (heap->extra_data) {
			int fixed_position = NOT_FIXED;
			struct ion_cp_heap_pdata *pdata = NULL;

			switch ((int) heap->type) {
			case ION_HEAP_TYPE_CP:
				pdata =
				(struct ion_cp_heap_pdata *)heap->extra_data;
				fixed_position = pdata->fixed_position;
				break;
			case ION_HEAP_TYPE_CARVEOUT:
			case ION_HEAP_TYPE_DMA:
				fixed_position = ((struct ion_co_heap_pdata *)
					heap->extra_data)->fixed_position;
				break;
			default:
				break;
			}

			switch (fixed_position) {
			case FIXED_LOW:
				heap->base = fixed_low_start;
				break;
			case FIXED_MIDDLE:
				heap->base = fixed_middle_start;
				if (middle_use_cma) {
					ret = dma_declare_contiguous(
						&ion_mm_heap_device.dev,
						heap->size,
						fixed_middle_start,
						0xa0000000);
					WARN_ON(ret);
				}
				pdata->secure_base = fixed_middle_start
							- HOLE_SIZE;
				pdata->secure_size = HOLE_SIZE + heap->size;
				break;
			case FIXED_HIGH:
				heap->base = fixed_high_start;
				break;
			default:
				break;
			}
		}
	}
#endif
}

static void __init reserve_mdp_memory(void)
{
	msm8960_mdp_writeback(msm8960_reserve_table);
}

static void __init reserve_cache_dump_memory(void)
{
#ifdef CONFIG_MSM_CACHE_DUMP
	unsigned int total;

	total = msm8960_cache_dump_pdata.l1_size +
		msm8960_cache_dump_pdata.l2_size;
	msm8960_reserve_table[MEMTYPE_EBI1].size += total;
	pr_info("mem_map: cache_dump reserved with size 0x%x in pool\n",
			total);
#endif
}

static void __init msm8960_calculate_reserve_sizes(void)
{
	size_pmem_devices();
	reserve_pmem_memory();
	reserve_ion_memory();
	reserve_mdp_memory();
	reserve_rtb_memory();
	reserve_cache_dump_memory();
}

static struct reserve_info msm8960_reserve_info __initdata = {
	.memtype_reserve_table = msm8960_reserve_table,
	.calculate_reserve_sizes = msm8960_calculate_reserve_sizes,
	.reserve_fixed_area = msm8960_reserve_fixed_area,
	.paddr_to_memtype = msm8960_paddr_to_memtype,
};

static void __init fighter_early_memory(void)
{
	reserve_info = &msm8960_reserve_info;
}

static void __init fighter_reserve(void)
{
	msm_reserve();
}

static void __init msm8960_allocate_memory_regions(void)
{
	msm8960_allocate_fb_region();
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
static struct wcd9xxx_pdata tabla_platform_data = {
	.slimbus_slave_device = {
		.name = "tabla-slave",
		.e_addr = {0, 0, 0x10, 0, 0x17, 2},
	},
	.irq = MSM_GPIO_TO_INT(62),
	.irq_base = TABLA_INTERRUPT_BASE,
	.num_irqs = NR_WCD9XXX_IRQS,
	.reset_gpio = PM8921_GPIO_PM_TO_SYS(34),
	.micbias = {
		.ldoh_v = TABLA_LDOH_2P85_V,
		.cfilt1_mv = 1800,
		.cfilt2_mv = 2700,
		.cfilt3_mv = 1800,
		.bias1_cfilt_sel = TABLA_CFILT1_SEL,
		.bias2_cfilt_sel = TABLA_CFILT2_SEL,
		.bias3_cfilt_sel = TABLA_CFILT3_SEL,
		.bias4_cfilt_sel = TABLA_CFILT3_SEL,
	},
	.regulator = {
	{
		.name = "CDC_VDD_CP",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_CDC_VDDA_CP_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_RX",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_CDC_VDDA_RX_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_TX",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_CDC_VDDA_TX_CUR_MAX,
	},
	{
		.name = "VDDIO_CDC",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_VDDIO_CDC_CUR_MAX,
	},
	{
		.name = "VDDD_CDC_D",
		.min_uV = 1225000,
		.max_uV = 1250000,
		.optimum_uA = WCD9XXX_VDDD_CDC_D_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_A_1P2V",
		.min_uV = 1225000,
		.max_uV = 1250000,
		.optimum_uA = WCD9XXX_VDDD_CDC_A_CUR_MAX,
	},
	},
};

static struct slim_device msm_slim_tabla = {
	.name = "tabla-slim",
	.e_addr = {0, 1, 0x10, 0, 0x17, 2},
	.dev = {
		.platform_data = &tabla_platform_data,
	},
};

static struct wcd9xxx_pdata tabla20_platform_data = {
	.slimbus_slave_device = {
		.name = "tabla-slave",
		.e_addr = {0, 0, 0x60, 0, 0x17, 2},
	},
	.irq = MSM_GPIO_TO_INT(62),
	.irq_base = TABLA_INTERRUPT_BASE,
	.num_irqs = NR_WCD9XXX_IRQS,
	.reset_gpio = PM8921_GPIO_PM_TO_SYS(34),
	.amic_settings = {
		.legacy_mode = 0x7F,
		.use_pdata = 0x7F,
	},
	.micbias = {
		.ldoh_v = TABLA_LDOH_2P85_V,
		.cfilt1_mv = 1800,
		.cfilt2_mv = 2700,
		.cfilt3_mv = 1800,
		.bias1_cfilt_sel = TABLA_CFILT1_SEL,
		.bias2_cfilt_sel = TABLA_CFILT2_SEL,
		.bias3_cfilt_sel = TABLA_CFILT3_SEL,
		.bias4_cfilt_sel = TABLA_CFILT3_SEL,
	},
	.regulator = {
	{
		.name = "CDC_VDD_CP",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_CDC_VDDA_CP_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_RX",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_CDC_VDDA_RX_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_TX",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_CDC_VDDA_TX_CUR_MAX,
	},
	{
		.name = "VDDIO_CDC",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_VDDIO_CDC_CUR_MAX,
	},
	{
		.name = "VDDD_CDC_D",
		.min_uV = 1225000,
		.max_uV = 1250000,
		.optimum_uA = WCD9XXX_VDDD_CDC_D_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_A_1P2V",
		.min_uV = 1225000,
		.max_uV = 1250000,
		.optimum_uA = WCD9XXX_VDDD_CDC_A_CUR_MAX,
	},
	},
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

#ifdef CONFIG_HTC_BATT_8960
static int critical_alarm_voltage_mv[] = {3000, 3200, 3400};

static struct htc_battery_platform_data htc_battery_pdev_data = {
	.guage_driver = 0,
	.chg_limit_active_mask = HTC_BATT_CHG_LIMIT_BIT_TALK |
								HTC_BATT_CHG_LIMIT_BIT_NAVI,
#ifdef CONFIG_DUTY_CYCLE_LIMIT
	.chg_limit_timer_sub_mask = HTC_BATT_CHG_LIMIT_BIT_THRML,
#endif
	.critical_low_voltage_mv = 3100,
	.critical_alarm_vol_ptr = critical_alarm_voltage_mv,
	.critical_alarm_vol_cols = sizeof(critical_alarm_voltage_mv) / sizeof(int),
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
	.icharger.max_input_current = pm8921_set_hsml_target_ma,
	.icharger.is_safty_timer_timeout = pm8921_is_chg_safety_timer_timeout,
	.icharger.is_battery_full_eoc_stop = pm8921_is_batt_full_eoc_stop,
	/* gauge */
	.igauge.name = "pm8921",
	.igauge.get_battery_voltage = pm8921_get_batt_voltage,
	.igauge.get_battery_current = pm8921_bms_get_batt_current,
	.igauge.get_battery_temperature = pm8921_get_batt_temperature,
	.igauge.get_battery_id = pm8921_get_batt_id,
	.igauge.get_battery_soc = pm8921_bms_get_batt_soc,
	.igauge.get_battery_cc = pm8921_bms_get_batt_cc,
	.igauge.store_battery_data = pm8921_bms_store_battery_data_emmc,
	.igauge.store_battery_ui_soc = pm8921_bms_store_battery_ui_soc,
	.igauge.get_battery_ui_soc = pm8921_bms_get_battery_ui_soc,
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

struct pm8xxx_gpio_init {
	unsigned			gpio;
	struct pm_gpio			config;
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

static uint32_t headset_cpu_gpio[] = {
	GPIO_CFG(FIGHTER_GPIO_AUD_UART_TX, 2, GPIO_CFG_OUTPUT,
		 GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(FIGHTER_GPIO_AUD_UART_RX, 2, GPIO_CFG_INPUT,
		 GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(FIGHTER_GPIO_V_HSMIC_2v85_EN, 0, GPIO_CFG_OUTPUT,
		 GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

struct pm8xxx_gpio_init headset_pmic_gpio[] = {
	PM8XXX_GPIO_INIT(FIGHTER_PMGPIO_AUD_REMO_PRESz, PM_GPIO_DIR_IN,
			 PM_GPIO_OUT_BUF_CMOS, 0, PM_GPIO_PULL_NO,
			 PM_GPIO_VIN_L17, PM_GPIO_STRENGTH_LOW,
			 PM_GPIO_FUNC_PAIRED, 0, 0),
};

struct pm8xxx_gpio_init headset_pmic_gpio_rx_xc[] = {
	PM8XXX_GPIO_INIT(FIGHTER_PMGPIO_AUD_REMO_PRESz, PM_GPIO_DIR_IN,
			 PM_GPIO_OUT_BUF_CMOS, 0, PM_GPIO_PULL_NO,
			 PM_GPIO_VIN_L17, PM_GPIO_STRENGTH_LOW,
			 PM_GPIO_FUNC_PAIRED, 0, 0),
#ifdef CONFIG_HTC_HEADSET_ONE_WIRE
	PM8XXX_GPIO_INIT(FIGHTER_PMGPIO_AUD_1WIRE_DI, PM_GPIO_DIR_OUT,
			 PM_GPIO_OUT_BUF_CMOS, 0, PM_GPIO_PULL_NO,
			 PM_GPIO_VIN_S4, PM_GPIO_STRENGTH_LOW,
			 PM_GPIO_FUNC_PAIRED, 0, 0),
	PM8XXX_GPIO_INIT(FIGHTER_PMGPIO_AUD_1WIRE_O, PM_GPIO_DIR_OUT,
			 PM_GPIO_OUT_BUF_CMOS, 0, PM_GPIO_PULL_NO,
			 PM_GPIO_VIN_L17, PM_GPIO_STRENGTH_LOW,
			 PM_GPIO_FUNC_PAIRED, 0, 0),
	PM8XXX_GPIO_INIT(FIGHTER_PMGPIO_AUD_1WIRE_DO, PM_GPIO_DIR_IN,
			 PM_GPIO_OUT_BUF_CMOS, 0, PM_GPIO_PULL_DN,
			 PM_GPIO_VIN_S4, PM_GPIO_STRENGTH_LOW,
			 PM_GPIO_FUNC_PAIRED, 0, 0),
#endif
};

struct pm8xxx_gpio_init headset_pmic_gpio_rx_xd[] = {
	PM8XXX_GPIO_INIT(FIGHTER_PMGPIO_NC_40, PM_GPIO_DIR_OUT,
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
		for (i = 0; i < ARRAY_SIZE(headset_pmic_gpio); i++) {
			rc = pm8xxx_gpio_config(headset_pmic_gpio[i].gpio,
						&headset_pmic_gpio[i].config);
			if (rc)
				pr_info("[HS_BOARD] %s: Config ERROR: GPIO=%u, rc=%d\n",
					__func__, headset_pmic_gpio[i].gpio, rc);
		}
		return;
	}

	/* XC */
	for (i = 0; i < ARRAY_SIZE(headset_cpu_gpio); i++)
		gpio_tlmm_config(headset_cpu_gpio[i], GPIO_CFG_ENABLE);
	gpio_set_value(FIGHTER_GPIO_V_HSMIC_2v85_EN, 0);

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
		gpio_set_value(PM8921_GPIO_PM_TO_SYS(FIGHTER_PMGPIO_NC_40), 1);
	}
}

static void headset_power(int enable)
{
	if (system_rev < 2) /* XA, XB */
		return;

	pr_info("[HS_BOARD] (%s) Set MIC bias %d\n", __func__, enable);

	if (enable)
		gpio_set_value(FIGHTER_GPIO_V_HSMIC_2v85_EN, 1);
	else
		gpio_set_value(FIGHTER_GPIO_V_HSMIC_2v85_EN, 0);
}

#ifdef CONFIG_HTC_HEADSET_ONE_WIRE
static uint32_t headset_1wire_gpio[] = {
	GPIO_CFG(FIGHTER_GPIO_AUD_UART_RX, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(FIGHTER_GPIO_AUD_UART_TX, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(FIGHTER_GPIO_AUD_UART_RX, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(FIGHTER_GPIO_AUD_UART_TX, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static void uart_tx_gpo(int mode)
{
	switch (mode) {
		case 0:
			gpio_tlmm_config(headset_1wire_gpio[1], GPIO_CFG_ENABLE);
			gpio_set_value_cansleep(FIGHTER_GPIO_AUD_UART_TX, 0);
			break;
		case 1:
			gpio_tlmm_config(headset_1wire_gpio[1], GPIO_CFG_ENABLE);
			gpio_set_value_cansleep(FIGHTER_GPIO_AUD_UART_TX, 1);
			break;
		case 2:
			gpio_tlmm_config(headset_1wire_gpio[3], GPIO_CFG_ENABLE);
			break;
	}
}

static void uart_lv_shift_en(int enable)
{
	gpio_set_value_cansleep(PM8921_GPIO_PM_TO_SYS(FIGHTER_PMGPIO_NC_40), enable);
}
#endif

/* HTC_HEADSET_PMIC Driver */
static struct htc_headset_pmic_platform_data htc_headset_pmic_data = {
	.driver_flag		= DRIVER_HS_PMIC_ADC,
	.hpin_gpio		= PM8921_GPIO_PM_TO_SYS(
					FIGHTER_PMGPIO_EARPHONE_DETz),
	.hpin_irq		= 0,
	.key_gpio		= PM8921_GPIO_PM_TO_SYS(
					FIGHTER_PMGPIO_AUD_REMO_PRESz),
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

#ifdef CONFIG_HTC_HEADSET_ONE_WIRE
static struct htc_headset_1wire_platform_data htc_headset_1wire_data = {
	.tx_level_shift_en	= PM8921_GPIO_PM_TO_SYS(FIGHTER_PMGPIO_NC_40),
	.uart_sw		= 0,
	.one_wire_remote	={0x7E, 0x7F, 0x7D, 0x7F, 0x7B, 0x7F},
	.remote_press		= 0,
	.onewire_tty_dev	= "/dev/ttyHSL2",
};

static struct platform_device htc_headset_one_wire = {
	.name	= "HTC_HEADSET_1WIRE",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_1wire_data,
	},
};
#endif

/* HTC_HEADSET_MGR Driver */
static struct platform_device *headset_devices[] = {
	&htc_headset_pmic,
#ifdef CONFIG_HTC_HEADSET_ONE_WIRE
	&htc_headset_one_wire,
#endif
	/* Please put the headset detection driver on the last */
};

static struct headset_adc_config htc_headset_mgr_config[] = {
	{
		.type = HEADSET_MIC,
		.adc_max = 1530,
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
#ifdef CONFIG_HTC_HEADSET_ONE_WIRE
	.uart_tx_gpo		= uart_tx_gpo,
	.uart_lv_shift_en	= uart_lv_shift_en,
#endif
};

static struct platform_device htc_headset_mgr = {
	.name	= "HTC_HEADSET_MGR",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_mgr_data,
	},
};

static void headset_device_register(void)
{
	pr_info("[HS_BOARD] (%s) Headset device register (system_rev=%d)\n",
		__func__, system_rev);

	platform_device_register(&htc_headset_mgr);
}

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
		.gpio_irq = FIGHTER_GPIO_TP_ATTz,
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
		.gpio_irq = FIGHTER_GPIO_TP_ATTz,
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
		.gpio_irq = FIGHTER_GPIO_TP_ATTz,
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
		.gpio_irq = FIGHTER_GPIO_TP_ATTz,
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
		.version = 0x3332,
		.packrat_number = 1293984,
		.abs_x_min = 0,
		.abs_x_max = 1000,
		.abs_y_min = 0,
		.abs_y_max = 1760,
		.display_width = 540,
		.display_height = 960,
		.flags = SYNAPTICS_FLIP_Y,
		.gpio_irq = FIGHTER_GPIO_TP_ATTz,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.reduce_report_level = {65, 65, 50, 0, 0},
		.default_config = 2,
		.tw_pin_mask = 0x0080,
		.config = {0x33, 0x32, 0x00, 0x01, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0xE8,
			0x03, 0x6C, 0x07, 0x02, 0x14, 0x1E, 0x05, 0x50,
			0x24, 0x24, 0x7B, 0x02, 0x01, 0x3C, 0x17, 0x01,
			0x1B, 0x01, 0x66, 0x4E, 0x66, 0x46, 0xEF, 0xBA,
			0xCD, 0xB8, 0x00, 0xD0, 0x00, 0x00, 0x00, 0x00,
			0x0A, 0x04, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0B, 0x13, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x64, 0x07, 0x66, 0x64,
			0xC0, 0x43, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x1E, 0x05, 0x00, 0x02, 0x66,
			0x01, 0x53, 0x03, 0x0E, 0x1F, 0x00, 0x3B, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x10, 0x0A, 0x60, 0x60,
			0x68, 0x60, 0x68, 0x60, 0x68, 0x60, 0x2F, 0x2F,
			0x2E, 0x2D, 0x2B, 0x2A, 0x29, 0x28, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xD0,
			0x07, 0x00, 0x3C, 0x00, 0x2C, 0x01, 0xCD, 0x0A,
			0xC0, 0xD0, 0x07, 0x00, 0xC0, 0x19, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x5B, 0x5E,
			0x60, 0x62, 0x65, 0x68, 0x6B, 0x6E, 0x00, 0x78,
			0x00, 0x10, 0x28, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x06, 0x09, 0x0C, 0x0F, 0x10, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x51, 0x51, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
			0x19, 0x1A, 0x1B, 0x11, 0xFF, 0xFF, 0xFF, 0x03,
			0x05, 0x07, 0x13, 0x11, 0x0F, 0x0D, 0x0B, 0x0A,
			0x09, 0x08, 0x01, 0x02, 0x04, 0x06, 0x0C, 0x0E,
			0x10, 0x12, 0xFF, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x0F, 0x01,
			0x4F, 0x53},
	},
	{
		.version = 0x3330,
		.packrat_number = 1100754,
		.abs_x_min = 0,
		.abs_x_max = 1000,
		.abs_y_min = 0,
		.abs_y_max = 1760,
		.display_width = 540,
		.display_height = 960,
		.flags = SYNAPTICS_FLIP_Y,
		.gpio_irq = FIGHTER_GPIO_TP_ATTz,
		.gpio_reset = FIGHTER_GPIO_TP_RSTz,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.reduce_report_level = {65, 65, 50, 0, 0},
		.large_obj_check = 1,
		.default_config = 2,
		.customer_register = {0xF9, 0x32, 0x05, 0x64},
		.tw_pin_mask = 0x0080,
		.multitouch_calibration = 1,
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
		.gpio_irq = FIGHTER_GPIO_TP_ATTz,
		.gpio_reset = FIGHTER_GPIO_TP_RSTz,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.reduce_report_level = {65, 65, 120, 15, 15},
		.large_obj_check = 1,
		.default_config = 2,
		.tw_pin_mask = 0x0080,
		.multitouch_calibration = 1,
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
		.gpio_irq = FIGHTER_GPIO_TP_ATTz,
		.gpio_reset = FIGHTER_GPIO_TP_RSTz,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.reduce_report_level = {65, 65, 120, 15, 15},
		.large_obj_check = 1,
		.default_config = 2,
		.tw_pin_mask = 0x0080,
		.multitouch_calibration = 1,
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
		.gpio_irq = FIGHTER_GPIO_TP_ATTz,
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
		.gpio_irq = FIGHTER_GPIO_TP_ATTz,
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

static struct i2c_board_info msm_i2c_gsbi3_info[] = {
	{
		I2C_BOARD_INFO(ATMEL_MXT224E_NAME, 0x94 >> 1),
		.platform_data = &ts_atmel_data,
		.irq = MSM_GPIO_TO_INT(FIGHTER_GPIO_TP_ATTz)
	},
	{
		I2C_BOARD_INFO(SYNAPTICS_3200_NAME, 0x40 >> 1),
		.platform_data = &syn_ts_3k_data,
		.irq = MSM_GPIO_TO_INT(FIGHTER_GPIO_TP_ATTz)
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

static struct bma250_platform_data gsensor_bma250_platform_data = {
	.intr = FIGHTER_GPIO_GSENSOR_INT,
	.chip_layout = 1,
};

static struct akm8975_platform_data compass_platform_data = {
	.layouts = FIGHTER_LAYOUTS,
	.use_pana_gyro = 0,
};

static struct pn544_i2c_platform_data nfc_platform_data = {
	.irq_gpio = FIGHTER_GPIO_NFC_IRQ,
	.ven_gpio = FIGHTER_GPIO_NFC_VEN,
	.firm_gpio = FIGHTER_GPIO_NFC_DL_MODE,
	.ven_isinvert = 1,
};

static struct i2c_board_info pn544_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO(PN544_I2C_NAME, 0x50 >> 1),
		.platform_data = &nfc_platform_data,
		.irq = MSM_GPIO_TO_INT(FIGHTER_GPIO_NFC_IRQ),
	},
};

static struct i2c_board_info msm_i2c_gsbi12_info[] = {
	{
		I2C_BOARD_INFO(BMA250_I2C_NAME, 0x30 >> 1),
		.platform_data = &gsensor_bma250_platform_data,
		.irq = MSM_GPIO_TO_INT(FIGHTER_GPIO_GSENSOR_INT),
	},
	{
		I2C_BOARD_INFO(AKM8975_I2C_NAME, 0x1A >> 1),
		.platform_data = &compass_platform_data,
		.irq = MSM_GPIO_TO_INT(FIGHTER_GPIO_COMPASS_INT),
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
static uint8_t cm3629_mapping_table[] = {
	0x0,   0x3,  0x6,  0x9,  0xC,
	0xF,  0x12, 0x15, 0x18, 0x1B,
	0x1E, 0x21, 0x24, 0x27, 0x2A,
	0x2D, 0x30, 0x33, 0x36, 0x39,
	0x3C, 0x3F, 0x43, 0x47, 0x4B,
	0x4F, 0x53, 0x57, 0x5B, 0x5F,
	0x63, 0x67, 0x6B, 0x70, 0x75,
	0x7A, 0x7F, 0x84, 0x89, 0x8E,
	0x93, 0x98, 0x9D, 0xA2, 0xA8,
	0xAE, 0xB4, 0xBA, 0xC0, 0xC6,
	0xCC, 0xD3, 0xDA, 0xE1, 0xE8,
	0xEF, 0xF6, 0xFF
};

static struct cm3629_platform_data cm36282_XD_pdata = {
	.model = CAPELLA_CM36282,
	.ps_select = CM3629_PS1_ONLY,
	.intr = PM8921_GPIO_PM_TO_SYS(FIGHTER_PMGPIO_PROXIMITY_INTz),
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
	.ps_debounce = 1,
	.ps_delay_time = 200,
	.dynamical_threshold = 1,
	.mapping_table = cm3629_mapping_table,
	.mapping_size = ARRAY_SIZE(cm3629_mapping_table),
};

static struct i2c_board_info i2c_CM36282_XD_devices[] = {
	{
		I2C_BOARD_INFO(CM3629_I2C_NAME, 0xC0 >> 1),
		.platform_data = &cm36282_XD_pdata,
		.irq =  PM8921_GPIO_IRQ(PM8921_IRQ_BASE, FIGHTER_PMGPIO_PROXIMITY_INTz),
	},
};

static struct cm3629_platform_data cm36282_pdata = {
	.model = CAPELLA_CM36282,
	.ps_select = CM3629_PS1_ONLY,
	.intr = PM8921_GPIO_PM_TO_SYS(FIGHTER_PMGPIO_PROXIMITY_INTz),
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
	.ps_debounce = 1,
	.ps_delay_time = 200,
	.dynamical_threshold = 1,
	.mapping_table = cm3629_mapping_table,
	.mapping_size = ARRAY_SIZE(cm3629_mapping_table),
};

static struct i2c_board_info i2c_CM36282_devices[] = {
	{
		I2C_BOARD_INFO(CM3629_I2C_NAME, 0xC0 >> 1),
		.platform_data = &cm36282_pdata,
		.irq =  PM8921_GPIO_IRQ(PM8921_IRQ_BASE, FIGHTER_PMGPIO_PROXIMITY_INTz),
	},
};

static uint32_t usb_ID_PIN_input_table[] = {
	GPIO_CFG(FIGHTER_GPIO_USB_ID1, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t usb_ID_PIN_ouput_table[] = {
	GPIO_CFG(FIGHTER_GPIO_USB_ID1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

void config_fighter_usb_id_gpios(bool output)
{
	if (output) {
		gpio_tlmm_config(usb_ID_PIN_ouput_table[0], GPIO_CFG_ENABLE);
		gpio_set_value(FIGHTER_GPIO_USB_ID1, 1);
		pr_info("[CABLE] %s: %d output high\n",  __func__, FIGHTER_GPIO_USB_ID1);
	} else {
		gpio_tlmm_config(usb_ID_PIN_input_table[0], GPIO_CFG_ENABLE);
		pr_info("[CABLE] %s: %d input none pull\n",  __func__, FIGHTER_GPIO_USB_ID1);
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
	GPIO_CFG(FIGHTER_GPIO_USBz_AUDIO_SW, 0, GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(FIGHTER_GPIO_USBz_AUDIO_SW, 0, GPIO_CFG_INPUT,GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t audio_uart_switch_table[] = {
	GPIO_CFG(FIGHTER_GPIO_AUDIOz_UART_SW, 0, GPIO_CFG_OUTPUT,
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
			gpio_set_value(FIGHTER_GPIO_AUDIOz_UART_SW, 1);
		} else {
			gpio_set_value(FIGHTER_GPIO_AUDIOz_UART_SW, 1);
			gpio_set_value(FIGHTER_GPIO_USBz_AUDIO_SW, 0);
		}
		break;
	case PATH_USB_AUD:
		pr_info("[CABLE] %s: Set Audio path\n", __func__);

		if ((system_rev == 0x80 && engineerid == 1) || system_rev < 0x80) {
			gpio_set_value(FIGHTER_GPIO_AUDIOz_UART_SW, 0);
		} else {
			gpio_set_value(FIGHTER_GPIO_AUDIOz_UART_SW, 0);
			gpio_set_value(FIGHTER_GPIO_USBz_AUDIO_SW, 1);
		}
		break;
	}

#ifdef CONFIG_FB_MSM_HDMI_MHL
	sii9234_change_usb_owner((path == PATH_MHL) ? 1 : 0);
#endif
}

static struct cable_detect_platform_data cable_detect_pdata = {
	.detect_type		= CABLE_TYPE_PMIC_ADC,
	.usb_id_pin_gpio	= FIGHTER_GPIO_USB_ID1,
	.get_adc_cb		= fighter_get_usbid_adc,
	.config_usb_id_gpios	= config_fighter_usb_id_gpios,
	.usb_dpdn_switch	= fighter_usb_dpdn_switch,
};

static struct platform_device cable_detect_device = {
	.name   = "cable_detect",
	.id     = -1,
	.dev    = {
		.platform_data = &cable_detect_pdata,
	},
};

static void fighter_cable_detect_register(void)
{
	pr_info("%s\n", __func__);
	platform_device_register(&cable_detect_device);
}

void fighter_pm8xxx_adc_device_register(void)
{
	pr_info("%s: Register PM8921 ADC device\n", __func__);
	headset_device_register();
}

#define MSM_SHARED_RAM_PHYS 0x80000000

static void __init fighter_map_io(void)
{
	msm_shared_ram_phys = MSM_SHARED_RAM_PHYS;
	msm_map_msm8960_io();
	if (socinfo_init() < 0)
		pr_err("socinfo_init() failed!\n");
}

static void __init fighter_init_irq(void)
{
	struct msm_mpm_device_data *data = NULL;

#ifdef CONFIG_MSM_MPM
	data = &msm8960_mpm_dev_data;
#endif

	msm_mpm_irq_extn_init(data);
	gic_init(0, GIC_PPI_START, MSM_QGIC_DIST_BASE,
					(void *)MSM_QGIC_CPU_BASE);
}

static void __init msm8960_init_buses(void)
{
#ifdef CONFIG_MSM_BUS_SCALING
	msm_bus_8960_apps_fabric_pdata.rpm_enabled = 1;
	msm_bus_8960_sys_fabric_pdata.rpm_enabled = 1;
	msm_bus_8960_mm_fabric_pdata.rpm_enabled = 1;
	msm_bus_apps_fabric.dev.platform_data =
		&msm_bus_8960_apps_fabric_pdata;
	msm_bus_sys_fabric.dev.platform_data = &msm_bus_8960_sys_fabric_pdata;
	msm_bus_mm_fabric.dev.platform_data = &msm_bus_8960_mm_fabric_pdata;
	msm_bus_sys_fpb.dev.platform_data = &msm_bus_8960_sys_fpb_pdata;
	msm_bus_cpss_fpb.dev.platform_data = &msm_bus_8960_cpss_fpb_pdata;
	msm_bus_rpm_set_mt_mask();
#endif
}

static struct msm_spi_platform_data msm8960_qup_spi_gsbi10_pdata = {
	.max_clock_speed = 27000000,
};

#ifdef CONFIG_USB_MSM_OTG_72K
static struct msm_otg_platform_data msm_otg_pdata;
#else
#define USB_5V_EN		42
static int msm_hsusb_vbus_power(bool on)
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

	printk(KERN_ERR "%s: vbus_is_on=%d\n", __func__, on);
	if (vbus_is_on == on)
		return 0;

	printk(KERN_INFO "%s: %d\n", __func__, on);

	if (on) {
		mvs_otg_switch = regulator_get(&msm8960_device_otg.dev,
					       "vbus_otg");
		if (IS_ERR(mvs_otg_switch)) {
			pr_err("Unable to get mvs_otg_switch\n");
			return -1;
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
		return 0;
	}
disable_mvs_otg:
		regulator_disable(mvs_otg_switch);
free_usb_5v_en:
		gpio_free(PM8921_GPIO_PM_TO_SYS(USB_5V_EN));
put_mvs_otg:
		regulator_put(mvs_otg_switch);
		vbus_is_on = false;
		return -1;
}

static int phy_init_seq_v3[] = { 0x7f, 0x81, 0x3c, 0x82, -1};
static int phy_init_seq_v3_2_1[] = { 0x5f, 0x81, 0x3c, 0x82, -1};

static struct msm_otg_platform_data msm_otg_pdata = {
	.phy_init_seq		= phy_init_seq_v3,
	.mode			= USB_OTG,
	.otg_control		= OTG_PMIC_CONTROL,
	.phy_type		= SNPS_28NM_INTEGRATED_PHY,
#if 0
	.pmic_id_irq		= PM8921_USB_ID_IN_IRQ(PM8921_IRQ_BASE),
#endif
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

static struct android_usb_platform_data android_usb_pdata = {
	.update_pid_and_serial_num = usb_diag_update_pid_and_serial_num,
	.nluns = 2,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};

#define VERSION_ID (readl(HW_VER_ID_VIRT) & 0xf0000000) >> 28
#define HW_8960_V3_2_1   0x07
void fighter_add_usb_devices(void)
{
	if (VERSION_ID >= HW_8960_V3_2_1) {
		printk(KERN_INFO "%s rev: %d v3.2.1\n", __func__, system_rev);
		msm_otg_pdata.phy_init_seq = phy_init_seq_v3_2_1;
	} else {
		printk(KERN_INFO "%s rev: %d\n", __func__, system_rev);
		msm_otg_pdata.phy_init_seq = phy_init_seq_v3;
	}
	printk(KERN_INFO "%s: OTG_PMIC_CONTROL in rev: %d\n",
			__func__, system_rev);
}

static uint8_t spm_wfi_cmd_sequence[] __initdata = {
			0x03, 0x0f,
};

static uint8_t spm_retention_cmd_sequence[] __initdata = {
			0x00, 0x05, 0x03, 0x0D,
			0x0B, 0x00, 0x0f,
};

static uint8_t spm_retention_with_krait_v3_cmd_sequence[] __initdata = {
	0x42, 0x1B, 0x00,
	0x05, 0x03, 0x0D, 0x0B,
	0x00, 0x42, 0x1B,
	0x0f,
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

/* 8960AB has a different command to assert apc_pdn */
static uint8_t spm_power_collapse_without_rpm_krait_v3[] __initdata = {
	0x00, 0x24, 0x84, 0x10,
	0x09, 0x03, 0x01,
	0x10, 0x84, 0x30, 0x0C,
	0x24, 0x30, 0x0f,
};

static uint8_t spm_power_collapse_with_rpm_krait_v3[] __initdata = {
	0x00, 0x24, 0x84, 0x10,
	0x09, 0x07, 0x01, 0x0B,
	0x10, 0x84, 0x30, 0x0C,
	0x24, 0x30, 0x0f,
};

static struct msm_spm_seq_entry msm_spm_boot_cpu_seq_list[] __initdata = {
	[0] = {
		.mode = MSM_SPM_MODE_CLOCK_GATING,
		.notify_rpm = false,
		.cmd = spm_wfi_cmd_sequence,
	},

	[1] = {
		.mode = MSM_SPM_MODE_POWER_RETENTION,
		.notify_rpm = false,
		.cmd = spm_retention_cmd_sequence,
	},

	[2] = {
		.mode = MSM_SPM_MODE_POWER_COLLAPSE,
		.notify_rpm = false,
		.cmd = spm_power_collapse_without_rpm,
	},
	[3] = {
		.mode = MSM_SPM_MODE_POWER_COLLAPSE,
		.notify_rpm = true,
		.cmd = spm_power_collapse_with_rpm,
	},
};

static struct msm_spm_seq_entry msm_spm_nonboot_cpu_seq_list[] __initdata = {
	[0] = {
		.mode = MSM_SPM_MODE_CLOCK_GATING,
		.notify_rpm = false,
		.cmd = spm_wfi_cmd_sequence,
	},

	[1] = {
		.mode = MSM_SPM_MODE_POWER_RETENTION,
		.notify_rpm = false,
		.cmd = spm_retention_cmd_sequence,
	},

	[2] = {
		.mode = MSM_SPM_MODE_POWER_COLLAPSE,
		.notify_rpm = false,
		.cmd = spm_power_collapse_without_rpm,
	},

	[3] = {
		.mode = MSM_SPM_MODE_POWER_COLLAPSE,
		.notify_rpm = true,
		.cmd = spm_power_collapse_with_rpm,
	},
};

static struct msm_spm_platform_data msm_spm_data[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW0_BASE,
		.reg_init_values[MSM_SPM_REG_SAW2_CFG] = 0x1F,
#if defined(CONFIG_MSM_AVS_HW)
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_CTL] = 0x58589464,
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_HYSTERESIS] = 0x00020000,
#endif
		.reg_init_values[MSM_SPM_REG_SAW2_SPM_CTL] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DLY] = 0x03020004,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_0] = 0x0084009C,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_1] = 0x00A4001C,
		.vctl_timeout_us = 50,
		.num_modes = ARRAY_SIZE(msm_spm_boot_cpu_seq_list),
		.modes = msm_spm_boot_cpu_seq_list,
	},
	[1] = {
		.reg_base_addr = MSM_SAW1_BASE,
		.reg_init_values[MSM_SPM_REG_SAW2_CFG] = 0x1F,
#if defined(CONFIG_MSM_AVS_HW)
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_CTL] = 0x58589464,
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_HYSTERESIS] = 0x00020000,
#endif
		.reg_init_values[MSM_SPM_REG_SAW2_SPM_CTL] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DLY] = 0x03020004,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_0] = 0x0084009C,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_1] = 0x00A4001C,
		.vctl_timeout_us = 50,
		.num_modes = ARRAY_SIZE(msm_spm_nonboot_cpu_seq_list),
		.modes = msm_spm_nonboot_cpu_seq_list,
	},
};

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
		.reg_init_values[MSM_SPM_REG_SAW2_SPM_CTL] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DLY] = 0x02020204,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_0] = 0x00A000AE,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_1] = 0x00A00020,
		.modes = msm_spm_l2_seq_list,
		.num_modes = ARRAY_SIZE(msm_spm_l2_seq_list),
	},
};

#define CYTTSP_TS_GPIO_IRQ		11
#define CYTTSP_TS_SLEEP_GPIO		50
#define CYTTSP_TS_RESOUT_N_GPIO		52

/*virtual key support */
static ssize_t tma340_vkeys_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 200,
	__stringify(EV_KEY) ":" __stringify(KEY_BACK) ":73:1120:97:97"
	":" __stringify(EV_KEY) ":" __stringify(KEY_MENU) ":230:1120:97:97"
	":" __stringify(EV_KEY) ":" __stringify(KEY_HOME) ":389:1120:97:97"
	":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":544:1120:97:97"
	"\n");
}

static struct kobj_attribute tma340_vkeys_attr = {
	.attr = {
		.mode = S_IRUGO,
	},
	.show = &tma340_vkeys_show,
};

static struct attribute *tma340_properties_attrs[] = {
	&tma340_vkeys_attr.attr,
	NULL
};

static struct attribute_group tma340_properties_attr_group = {
	.attrs = tma340_properties_attrs,
};

static int cyttsp_platform_init(struct i2c_client *client)
{
	int rc = 0;
	static struct kobject *tma340_properties_kobj;

	tma340_vkeys_attr.attr.name = "virtualkeys.cyttsp-i2c";
	tma340_properties_kobj = kobject_create_and_add("board_properties",
								NULL);
	if (tma340_properties_kobj)
		rc = sysfs_create_group(tma340_properties_kobj,
					&tma340_properties_attr_group);
	if (!tma340_properties_kobj || rc)
		pr_err("%s: failed to create board_properties\n",
				__func__);

	return 0;
}

static struct cyttsp_regulator regulator_data[] = {
	{
		.name = "vdd",
		.min_uV = CY_TMA300_VTG_MIN_UV,
		.max_uV = CY_TMA300_VTG_MAX_UV,
		.hpm_load_uA = CY_TMA300_CURR_24HZ_UA,
		.lpm_load_uA = CY_TMA300_SLEEP_CURR_UA,
	},
	/* TODO: Remove after runtime PM is enabled in I2C driver */
	{
		.name = "vcc_i2c",
		.min_uV = CY_I2C_VTG_MIN_UV,
		.max_uV = CY_I2C_VTG_MAX_UV,
		.hpm_load_uA = CY_I2C_CURR_UA,
		.lpm_load_uA = CY_I2C_SLEEP_CURR_UA,
	},
};

static struct cyttsp_platform_data cyttsp_pdata = {
	.panel_maxx = 634,
	.panel_maxy = 1166,
	.disp_maxx = 616,
	.disp_maxy = 1023,
	.disp_minx = 0,
	.disp_miny = 16,
	.flags = 0x01,
	.gen = CY_GEN3,	/* or */
	.use_st = CY_USE_ST,
	.use_mt = CY_USE_MT,
	.use_hndshk = CY_SEND_HNDSHK,
	.use_trk_id = CY_USE_TRACKING_ID,
	.use_sleep = CY_USE_DEEP_SLEEP_SEL | CY_USE_LOW_POWER_SEL,
	.use_gestures = CY_USE_GESTURES,
	.fw_fname = "cyttsp_8960_cdp.hex",
	/* activate up to 4 groups
	 * and set active distance
	 */
	.gest_set = CY_GEST_GRP1 | CY_GEST_GRP2 |
				CY_GEST_GRP3 | CY_GEST_GRP4 |
				CY_ACT_DIST,
	/* change act_intrvl to customize the Active power state
	 * scanning/processing refresh interval for Operating mode
	 */
	.act_intrvl = CY_ACT_INTRVL_DFLT,
	/* change tch_tmout to customize the touch timeout for the
	 * Active power state for Operating mode
	 */
	.tch_tmout = CY_TCH_TMOUT_DFLT,
	/* change lp_intrvl to customize the Low Power power state
	 * scanning/processing refresh interval for Operating mode
	 */
	.lp_intrvl = CY_LP_INTRVL_DFLT,
	.sleep_gpio = CYTTSP_TS_SLEEP_GPIO,
	.resout_gpio = CYTTSP_TS_RESOUT_N_GPIO,
	.irq_gpio = CYTTSP_TS_GPIO_IRQ,
	.regulator_info = regulator_data,
	.num_regulators = ARRAY_SIZE(regulator_data),
	.init = cyttsp_platform_init,
	.correct_fw_ver = 9,
};

static struct i2c_board_info cyttsp_info[] __initdata = {
	{
		I2C_BOARD_INFO(CY_I2C_NAME, 0x24),
		.platform_data = &cyttsp_pdata,
#ifndef CY_USE_TIMER
		.irq = MSM_GPIO_TO_INT(CYTTSP_TS_GPIO_IRQ),
#endif /* CY_USE_TIMER */
	},
};

static uint32_t gsbi3_gpio_table[] = {
	GPIO_CFG(FIGHTER_GPIO_TP_I2C_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(FIGHTER_GPIO_TP_I2C_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi3_gpio_table_gpio[] = {
	GPIO_CFG(FIGHTER_GPIO_TP_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(FIGHTER_GPIO_TP_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

/* CAMERA setting */
static uint32_t gsbi4_gpio_table[] = {
	GPIO_CFG(FIGHTER_GPIO_CAM_I2C_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(FIGHTER_GPIO_CAM_I2C_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi4_gpio_table_gpio[] = {
	GPIO_CFG(FIGHTER_GPIO_CAM_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(FIGHTER_GPIO_CAM_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi8_gpio_table[] = {
	GPIO_CFG(FIGHTER_GPIO_AC_I2C_SDA, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(FIGHTER_GPIO_AC_I2C_SCL, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi8_gpio_table_gpio[] = {
	GPIO_CFG(FIGHTER_GPIO_AC_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(FIGHTER_GPIO_AC_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi12_gpio_table[] = {
	GPIO_CFG(FIGHTER_GPIO_SENSOR_I2C_SDA, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(FIGHTER_GPIO_SENSOR_I2C_SCL, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi12_gpio_table_gpio[] = {
	GPIO_CFG(FIGHTER_GPIO_SENSOR_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(FIGHTER_GPIO_SENSOR_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static void gsbi_qup_i2c_gpio_config(int adap_id, int config_type)
{
	printk(KERN_INFO "%s(): adap_id = %d, config_type = %d \n", __func__, adap_id, config_type);

	if ((adap_id == MSM_8960_GSBI3_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi3_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi3_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI3_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi3_gpio_table_gpio[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi3_gpio_table_gpio[1], GPIO_CFG_ENABLE);
	}

	/* CAMERA setting */
	if ((adap_id == MSM_8960_GSBI4_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi4_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi4_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI4_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi4_gpio_table_gpio[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi4_gpio_table_gpio[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI8_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi8_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi8_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI8_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi8_gpio_table_gpio[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi8_gpio_table_gpio[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI12_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi12_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi12_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI12_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi12_gpio_table_gpio[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi12_gpio_table_gpio[1], GPIO_CFG_ENABLE);
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

static struct platform_device msm_tsens_device = {
	.name   = "tsens8960-tm",
	.id = -1,
};

static struct msm_thermal_data msm_thermal_pdata = {
	.sensor_id = 0,
	.poll_ms = 1000,
	.limit_temp_degC = 60,
	.temp_hysteresis_degC = 10,
#if 0
	.limit_freq = 918000,
#endif
	.freq_step = 2,
};

#ifdef CONFIG_MSM_FAKE_BATTERY
static struct platform_device fish_battery_device = {
	.name = "fish_battery",
};
#endif

static struct platform_device scm_memchk_device = {
	.name		= "scm-memchk",
	.id		= -1,
};

static struct platform_device fighter_device_rpm_regulator __devinitdata = {
	.name	= "rpm-regulator",
	.id	= -1,
	.dev	= {
		.platform_data = &fighter_rpm_regulator_pdata,
	},
};

static struct platform_device *common_devices[] __initdata = {
	&msm8960_device_acpuclk,
	&msm8960_device_dmov,
	&msm_device_smd,
	&msm8960_device_uart_gsbi8,
	&msm_device_uart_dm6,
	&msm_device_saw_core0,
	&msm_device_saw_core1,
	&msm8960_device_ext_5v_vreg,
	&msm8960_device_qup_i2c_gsbi2,
	&msm8960_device_qup_i2c_gsbi3,
	&msm8960_device_qup_i2c_gsbi4,
	&msm8960_device_qup_i2c_gsbi8,
	&msm8960_device_qup_spi_gsbi10,
#ifndef CONFIG_MSM_DSPS
	&msm8960_device_qup_i2c_gsbi12,
#endif
	&msm8960_device_ssbi_pmic,
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
	&msm8960_cpu_slp_status,
	&msm_device_sps,
#ifdef CONFIG_MSM_FAKE_BATTERY
	&fish_battery_device,
#endif
	&fmem_device,
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
#endif
#endif
	&msm_device_vidc,
	&msm_device_bam_dmux,
	&msm_fm_platform_init,

#ifdef CONFIG_HW_RANDOM_MSM
	&msm_device_rng,
#endif
#ifdef CONFIG_ION_MSM
	&ion_dev,
#endif
#ifdef CONFIG_MSM_IOMMU
	&msm8960_iommu_domain_device,
#endif
	&msm8960_rpm_device,
	&msm8960_rpm_log_device,
	&msm8960_rpm_stat_device,
#ifdef CONFIG_MSM_QDSS
	&msm_etb_device,
	&msm_tpiu_device,
	&msm_funnel_device,
	&msm_etm_device,
#endif
	&msm8960_device_watchdog,
#ifdef CONFIG_MSM_RTB
	&msm8960_rtb_device,
#endif
#ifdef CONFIG_MSM_CACHE_ERP
	&msm8960_device_cache_erp,
#endif
#ifdef CONFIG_MSM_CACHE_DUMP
	&msm8960_cache_dump_device,
#endif
#ifdef CONFIG_HTC_BATT_8960
	&htc_battery_pdev,
#endif
	&msm_tsens_device,
};

static struct platform_device *fighter_devices[] __initdata = {
	&msm_8960_q6_lpass,
	&msm_8960_q6_mss_fw,
	&msm_8960_q6_mss_sw,
	&msm_8960_riva,
	&msm_pil_tzapps,
	&msm8960_device_otg,
	&msm8960_device_gadget_peripheral,
	&msm_device_hsusb_host,
	&android_usb_device,
	&msm_pcm,
	&msm_pcm_routing,
	&msm_multi_ch_pcm,
	&msm_cpudai0,
	&msm_cpudai1,
	&msm8960_cpudai_slimbus_2_tx,
	&msm8960_cpudai_slimbus_2_rx,
	&msm_cpudai_hdmi_rx,
	&msm_cpudai_bt_rx,
	&msm_cpudai_bt_tx,
	&msm_cpudai_fm_rx,
	&msm_cpudai_fm_tx,
	&msm_cpudai_auxpcm_rx,
	&msm_cpudai_auxpcm_tx,
	&msm_cpu_fe,
	&msm_stub_codec,
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
	&msm_compr_dsp,
	&msm_cpudai_incall_music_rx,
	&msm_cpudai_incall_record_rx,
	&msm_cpudai_incall_record_tx,
	&msm_pcm_hostless,
	&msm_lowlatency_pcm,
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
	struct kgsl_device_platform_data *kgsl_3d0_pdata =
		msm_kgsl_3d0.dev.platform_data;
	uint32_t soc_platform_version = socinfo_get_version();

	/* Fixup data that needs to change based on GPU ID */
	if (cpu_is_msm8960ab()) {
		kgsl_3d0_pdata->chipid = ADRENO_CHIPID(3, 2, 1, 0);
		/* 8960PRO nominal clock rate is 320Mhz */
		kgsl_3d0_pdata->pwrlevel[1].gpu_freq = 320000000;
	} else {
		kgsl_3d0_pdata->iommu_count = 1;
		if (SOCINFO_VERSION_MAJOR(soc_platform_version) == 1) {
			kgsl_3d0_pdata->pwrlevel[0].gpu_freq = 320000000;
			kgsl_3d0_pdata->pwrlevel[1].gpu_freq = 266667000;
		}
		if (SOCINFO_VERSION_MAJOR(soc_platform_version) >= 3) {
			/* 8960v3 GPU registers returns 5 for patch release
			 * but it should be 6, so dummy up the chipid here
			 * based the platform type
			 */
			kgsl_3d0_pdata->chipid = ADRENO_CHIPID(2, 2, 0, 6);
		}
	}

	/* Register the 3D core */
	platform_device_register(&msm_kgsl_3d0);

	/* Register the 2D cores if we are not 8960PRO */
	if (!cpu_is_msm8960ab()) {
		platform_device_register(&msm_kgsl_2d0);
		platform_device_register(&msm_kgsl_2d1);
	}
}

#ifdef CONFIG_HTC_BATT_8960
static struct pm8921_charger_batt_param chg_batt_params[] = {
	/* for normal type battery */
	[0] = {
		.max_voltage = 4200,
		.cool_bat_voltage = 4200,
		.warm_bat_voltage = 4000,
	},
	/* for HV type battery */
	[1] = {
		.max_voltage = 4340,
		.cool_bat_voltage = 4340,
		.warm_bat_voltage = 4000,
	},
	/* for another HV type battery */
	[2] = {
		.max_voltage = 4300,
		.cool_bat_voltage = 4300,
		.warm_bat_voltage = 4000,
	},
	/* for HV type battery for SMB charger IC */
	[3] = {
		.max_voltage = 4350,
		.cool_bat_voltage = 4350,
		.warm_bat_voltage = 4000,
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

static struct sf_lut pc_sf_id_1 = {
	.rows		= 10,
	.cols		= 5,
	/* row_entries are cycle */
	.row_entries	= {100, 200, 300, 400, 500},
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

static struct pc_temp_ocv_lut pc_temp_ocv_id_1 = {
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

struct pm8921_bms_battery_data bms_battery_data_id_1 = {
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

static struct sf_lut pc_sf_id_2 = {
	.rows		= 10,
	.cols		= 5,
	/* row_entries are cycle */
	.row_entries	= {100, 200, 300, 400, 500},
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

static struct pc_temp_ocv_lut pc_temp_ocv_id_2 = {
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

struct pm8921_bms_battery_data bms_battery_data_id_2 = {
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

static struct sf_lut pc_sf_id_3 = {
	.rows		= 10,
	.cols		= 5,
	/* row_entries are cycle */
	.row_entries	= {100, 200, 300, 400, 500},
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

static struct pc_temp_ocv_lut pc_temp_ocv_id_3 = {
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

struct pm8921_bms_battery_data bms_battery_data_id_3 = {
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

static struct msm_rpmrs_level msm_rpmrs_levels[] = {
	{
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		1, 784, 180000, 100,
	},

	{
		MSM_PM_SLEEP_MODE_RETENTION,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		415, 715, 340827, 475,
	},
#ifdef CONFIG_MSM_STANDALONE_POWER_COLLAPSE
	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		1300, 228, 1200000, 2000,
	},
#endif
	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, GDHS, MAX, ACTIVE),
		false,
		2000, 138, 1208400, 3200,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, HSFS_OPEN, ACTIVE, RET_HIGH),
		false,
		6000, 119, 1850300, 9000,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, GDHS, MAX, ACTIVE),
		false,
		9200, 68, 2839200, 16400,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, MAX, ACTIVE),
		false,
		10300, 63, 3128000, 18200,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, ACTIVE, RET_HIGH),
		false,
		18000, 10, 4602600, 27000,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, RET_HIGH, RET_LOW),
		false,
		20000, 2, 5752000, 32000,
	},
};

static struct msm_rpmrs_platform_data msm_rpmrs_data __initdata = {
	.levels = &msm_rpmrs_levels[0],
	.num_levels = ARRAY_SIZE(msm_rpmrs_levels),
	.vdd_mem_levels  = {
		[MSM_RPMRS_VDD_MEM_RET_LOW]	= 750000,
		[MSM_RPMRS_VDD_MEM_RET_HIGH]	= 750000,
		[MSM_RPMRS_VDD_MEM_ACTIVE]	= 1050000,
		[MSM_RPMRS_VDD_MEM_MAX]		= 1150000,
	},
	.vdd_dig_levels = {
		[MSM_RPMRS_VDD_DIG_RET_LOW]	= 500000,
		[MSM_RPMRS_VDD_DIG_RET_HIGH]	= 750000,
		[MSM_RPMRS_VDD_DIG_ACTIVE]	= 950000,
		[MSM_RPMRS_VDD_DIG_MAX]		= 1150000,
	},
	.vdd_mask = 0x7FFFFF,
	.rpmrs_target_id = {
		[MSM_RPMRS_ID_PXO_CLK]		= MSM_RPM_ID_PXO_CLK,
		[MSM_RPMRS_ID_L2_CACHE_CTL]	= MSM_RPM_ID_LAST,
		[MSM_RPMRS_ID_VDD_DIG_0]	= MSM_RPM_ID_PM8921_S3_0,
		[MSM_RPMRS_ID_VDD_DIG_1]	= MSM_RPM_ID_PM8921_S3_1,
		[MSM_RPMRS_ID_VDD_MEM_0]	= MSM_RPM_ID_PM8921_L24_0,
		[MSM_RPMRS_ID_VDD_MEM_1]	= MSM_RPM_ID_PM8921_L24_1,
		[MSM_RPMRS_ID_RPM_CTL]		= MSM_RPM_ID_RPM_CTL,
	},
};

static struct msm_pm_boot_platform_data msm_pm_boot_pdata __initdata = {
	.mode = MSM_PM_BOOT_CONFIG_TZ,
};

#ifdef CONFIG_I2C
#define I2C_SURF 1
#define I2C_FFA  (1 << 1)
#define I2C_RUMI (1 << 2)
#define I2C_SIM  (1 << 3)
#define I2C_FLUID (1 << 4)

struct i2c_registry {
	u8                     machs;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

#ifdef CONFIG_SND_SOC_TPA2051D3
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
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_8960_GSBI3_QUP_I2C_BUS_ID,
		cyttsp_info,
		ARRAY_SIZE(cyttsp_info),
	},
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
#ifdef CONFIG_SND_SOC_TPA2051D3
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

#ifdef CONFIG_MSM_CAMERA
	struct i2c_registry fighter_camera_i2c_devices = {
		I2C_SURF | I2C_FFA | I2C_FLUID | I2C_RUMI,
		MSM_8960_GSBI4_QUP_I2C_BUS_ID,
		fighter_camera_board_info.board_info,
		fighter_camera_board_info.num_i2c_board_info,
	};
#endif

	mach_mask = I2C_SURF;

	/* Run the array and install devices as appropriate */
	for (i = 0; i < ARRAY_SIZE(msm8960_i2c_devices); ++i) {
		if (msm8960_i2c_devices[i].machs & mach_mask) {
			i2c_register_board_info(msm8960_i2c_devices[i].bus,
						msm8960_i2c_devices[i].info,
						msm8960_i2c_devices[i].len);
		}
	}
#ifdef CONFIG_MSM_CAMERA
	if (fighter_camera_i2c_devices.machs & mach_mask)
		i2c_register_board_info(fighter_camera_i2c_devices.bus,
				fighter_camera_i2c_devices.info,
				fighter_camera_i2c_devices.len);
#endif
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
#endif /* CONFIG_I2C */
}

static void __init msm8960ab_update_krait_spm(void)
{
	int i;


	/* Update the SPM sequences for SPC and PC */
	for (i = 0; i < ARRAY_SIZE(msm_spm_data); i++) {
		int j;
		struct msm_spm_platform_data *pdata = &msm_spm_data[i];
		for (j = 0; j < pdata->num_modes; j++) {
			if (pdata->modes[j].cmd ==
					spm_power_collapse_without_rpm)
				pdata->modes[j].cmd =
				spm_power_collapse_without_rpm_krait_v3;
			else if (pdata->modes[j].cmd ==
					spm_power_collapse_with_rpm)
				pdata->modes[j].cmd =
				spm_power_collapse_with_rpm_krait_v3;
		}
	}
}

static void __init msm8960ab_update_retention_spm(void)
{
	int i;

	/* Update the SPM sequences for krait retention on all cores */
	for (i = 0; i < ARRAY_SIZE(msm_spm_data); i++) {
		int j;
		struct msm_spm_platform_data *pdata = &msm_spm_data[i];
		for (j = 0; j < pdata->num_modes; j++) {
			if (pdata->modes[j].cmd ==
					spm_retention_cmd_sequence)
				pdata->modes[j].cmd =
				spm_retention_with_krait_v3_cmd_sequence;
		}
	}
}


/*UART -> GSBI8*/
static uint32_t msm_uart_gpio[] = {
	GPIO_CFG(FIGHTER_GPIO_DEBUG_UART_TX, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(FIGHTER_GPIO_DEBUG_UART_RX, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
};
static void msm_uart_gsbi_gpio_init(void)
{
	gpio_tlmm_config(msm_uart_gpio[0], GPIO_CFG_ENABLE);
	gpio_tlmm_config(msm_uart_gpio[1], GPIO_CFG_ENABLE);
}

static uint32_t msm_region_gpio[] = {
	GPIO_CFG(FIGHTER_GPIO_REGION_ID, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, 0),
};
static void msm_region_id_gpio_init(void)
{
	gpio_tlmm_config(msm_region_gpio[0], GPIO_CFG_ENABLE);
}

static void __init fighter_init(void)
{
	u32 hw_ver_id = 0, rc = 0;
	struct kobject *properties_kobj;

	if (meminfo_init(SYS_MEMORY, SZ_256M) < 0)
		pr_err("meminfo_init() failed!\n");

	htc_add_ramconsole_devices();
	platform_device_register(&msm_gpio_device);

	msm_tsens_early_init(&msm_tsens_pdata);
	msm_thermal_init(&msm_thermal_pdata);
	BUG_ON(msm_rpm_init(&msm8960_rpm_data));
	BUG_ON(msm_rpmrs_levels_init(&msm_rpmrs_data));

	regulator_suppress_info_printing();
	if (msm_xo_init())
		pr_err("Failed to initialize XO votes\n");
	platform_device_register(&fighter_device_rpm_regulator);
	msm_clock_init(&msm8960_clock_init_data);
	msm8960_device_otg.dev.platform_data = &msm_otg_pdata;
	android_usb_pdata.swfi_latency =
		msm_rpmrs_levels[0].latency_us;
	fighter_gpiomux_init();
	msm8960_device_qup_spi_gsbi10.dev.platform_data =
				&msm8960_qup_spi_gsbi10_pdata;
	fighter_init_pmic();
	msm8960_i2c_init();
	msm8960_gfx_init();
	if (cpu_is_msm8960ab())
		msm8960ab_update_krait_spm();
	if (cpu_is_krait_v3()) {
		struct msm_pm_init_data_type *pdata =
			msm8960_pm_8x60.dev.platform_data;
		pdata->retention_calls_tz = false;
		msm8960ab_update_retention_spm();
	}
	platform_device_register(&msm8960_pm_8x60);

	msm_spm_init(msm_spm_data, ARRAY_SIZE(msm_spm_data));
	msm_spm_l2_init(msm_spm_l2_data);
	msm8960_init_buses();

	fighter_cable_detect_register();

#ifdef CONFIG_BT
	bt_export_bd_address();
#endif
#ifdef CONFIG_HTC_BATT_8960
	htc_battery_cell_init(htc_battery_cells, ARRAY_SIZE(htc_battery_cells));
#endif /* CONFIG_HTC_BATT_8960 */

	platform_add_devices(msm8960_footswitch, msm8960_num_footswitch);
	platform_device_register(&msm8960_device_ext_l2_vreg);

	platform_add_devices(common_devices, ARRAY_SIZE(common_devices));

	msm_uart_gsbi_gpio_init();
	fighter_pm8921_gpio_mpp_init();
	msm_region_id_gpio_init();
	platform_add_devices(fighter_devices, ARRAY_SIZE(fighter_devices));
	fighter_init_camera();
	fighter_init_mmc();
	register_i2c_devices();
	fighter_init_fb();
	slim_register_board_info(msm_slim_devices,
		ARRAY_SIZE(msm_slim_devices));
	BUG_ON(msm_pm_boot_init(&msm_pm_boot_pdata));

	/*usb driver won't be loaded in MFG 58 station and gift mode*/
	if (!(board_mfg_mode() == 6 || board_mfg_mode() == 7))
		fighter_add_usb_devices();

	properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (properties_kobj)
		rc = sysfs_create_group(properties_kobj,
			&properties_attr_group);

	fighter_init_keypad();
	hw_ver_id = readl(HW_VER_ID_VIRT);
	printk(KERN_INFO "hw_ver_id = %x\n", hw_ver_id);
}

#define PHY_BASE_ADDR1  0x80400000
#define SIZE_ADDR1      (132 * 1024 * 1024)

#define PHY_BASE_ADDR2  0x90000000
#define SIZE_ADDR2      (768 * 1024 * 1024)

static void __init fighter_fixup(struct tag *tags,
				 char **cmdline, struct meminfo *mi)
{
	engineerid = parse_tag_engineerid(tags);
	mi->nr_banks = 2;
	mi->bank[0].start = PHY_BASE_ADDR1;
	mi->bank[0].size = SIZE_ADDR1;
	mi->bank[1].start = PHY_BASE_ADDR2;
	mi->bank[1].size = SIZE_ADDR2;

	skuid = parse_tag_skuid((const struct tag *)tags);
	printk(KERN_INFO "Fighter_fixup:skuid=0x%x\n", skuid);
}

static int __init pm8921_late_init(void)
{
	return 0;
}

late_initcall(pm8921_late_init);

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
	.restart = msm_restart,
MACHINE_END
