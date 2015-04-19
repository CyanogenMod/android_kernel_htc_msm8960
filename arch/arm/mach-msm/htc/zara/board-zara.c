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

#include <asm/hardware/gic.h>
#include <asm/mach/arch.h>

#include <linux/bma250.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/msm_ssbi.h>
#include <linux/regulator/msm-gpio-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <linux/regulator/consumer.h>
#include <linux/slimbus/slimbus.h>
#include <linux/bootmem.h>
#include <linux/cm3629.h>
#include <linux/dma-contiguous.h>
#include <linux/dma-mapping.h>
#include <linux/fmem.h>
#include <linux/gpio.h>
#include <linux/htc_flashlight.h>
#include <linux/i2c.h>
#include <linux/i2c/isl9519.h>
#include <linux/leds.h>
#include <linux/leds-pm8038.h>
#include <linux/gpio_keys.h>
#include <linux/memory.h>
#include <linux/memblock.h>
#include <linux/mfd/wcd9xxx/pdata.h>
#include <linux/msm_ion.h>
#include <linux/msm_thermal.h>
#include <linux/msm_tsens.h>
#include <linux/platform_data/qcom_crypto_device.h>
#include <linux/rt5501.h>
#include <linux/spi/spi.h>
#include <linux/synaptics_i2c_rmi.h>
#include <linux/slimbus/slimbus.h>
#include <linux/mfd/wcd9xxx/core.h>
#include <asm/mach-types.h>
#include <asm/setup.h>
#include <asm/mach/mmc.h>
#include <mach/board.h>
#include <linux/tfa9887.h>
#include <linux/usb/android.h>
#include <linux/usb/msm_hsusb.h>
#include <mach/usbdiag.h>

#include <mach/board_htc.h>
#include <mach/cable_detect.h>
#include <mach/dma.h>
#include <mach/htc_headset_gpio.h>
#include <mach/htc_headset_mgr.h>
#include <mach/htc_headset_one_wire.h>
#include <mach/htc_headset_pmic.h>
#include <mach/iommu_domains.h>
#include <mach/mpm.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_memtypes.h>
#include <mach/gpiomux.h>
#include <mach/rpm.h>
#include <mach/msm_iomap.h>
#include <mach/msm_serial_hs.h>
#include <mach/msm_spi.h>
#include <mach/msm_xo.h>
#include <mach/restart.h>
#include <mach/socinfo.h>

#include <linux/ion.h>
#include <mach/ion.h>
#include <mach/mdm2.h>
#include <mach/kgsl.h>
#include "timer.h"
#include "devices.h"
#include "devices-msm8x60.h"
#include "spm.h"
#include "pm.h"
#include <mach/cpuidle.h>
#include "rpm_resources.h"
#include "clock.h"
#include "smd_private.h"
#include "pm-boot.h"
#include "msm_watchdog.h"
#include "board-8930.h"
#include "acpuclock-krait.h"
#ifdef CONFIG_SENSORS_NFC_PN544
#include <linux/pn544.h>
#endif
#include "board-zara.h"
#include <linux/proc_fs.h>
#include <mach/ncp6924.h>



#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif
#ifdef CONFIG_BT
#include <mach/htc_bdaddress.h>
#endif
#ifdef CONFIG_HTC_BATT_8960
#include <mach/htc_battery_8960.h>
#include <mach/htc_battery_cell.h>
#include <linux/mfd/pm8xxx/pm8921-charger.h>
#endif
#ifdef CONFIG_MSM_CACHE_DUMP
#include <mach/msm_cache_dump.h>
#endif
#ifdef CONFIG_MSM_RTB
#include <mach/msm_rtb.h>
#endif


static struct platform_device msm_fm_platform_init = {
	.name = "iris_fm",
	.id   = -1,
};

#define MSM_PMEM_ADSP_SIZE         0x7800000
#define MSM_PMEM_AUDIO_SIZE        0x4CF000
#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
#define MSM_PMEM_SIZE 0x4000000 /* 64 Mbytes */
#else
#define MSM_PMEM_SIZE 0x2800000 /* 40 Mbytes */
#endif

#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
#define HOLE_SIZE	0x20000
#define MSM_CONTIG_MEM_SIZE	0x65000
#ifdef CONFIG_MSM_IOMMU
#define MSM_ION_MM_SIZE            0x4800000 /* Need to be multiple of 64K */
#define MSM_ION_SF_SIZE            0x0
#define MSM_ION_QSECOM_SIZE	0x780000 /* (7.5MB) */
#define MSM_ION_HEAP_NUM	7
#else
#define MSM_ION_SF_SIZE		MSM_PMEM_SIZE
#define MSM_ION_MM_SIZE		MSM_PMEM_ADSP_SIZE
#define MSM_ION_QSECOM_SIZE	0x600000 /* (6MB) */
#define MSM_ION_HEAP_NUM	8
#endif
#define MSM_ION_MM_FW_SIZE	(0x200000 - HOLE_SIZE) /* 2MB - 128Kb */
#define MSM_ION_MFC_SIZE	SZ_8K
#define MSM_ION_AUDIO_SIZE	MSM_PMEM_AUDIO_SIZE

#define MSM8930_FIXED_AREA_START (0xa0000000 - (MSM_ION_MM_FW_SIZE + \
								HOLE_SIZE))
#define MAX_FIXED_AREA_SIZE	0x10000000
#define MSM8930_FW_START	MSM8930_FIXED_AREA_START
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
static int __init pmem_size_setup(char *p)
{
	pmem_size = memparse(p, NULL);
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

static struct platform_device msm8930_android_pmem_device = {
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
static struct platform_device msm8930_android_pmem_adsp_device = {
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

static struct platform_device msm8930_android_pmem_audio_device = {
	.name = "android_pmem",
	.id = 4,
	.dev = { .platform_data = &android_pmem_audio_pdata },
};
#endif /* CONFIG_MSM_MULTIMEDIA_USE_ION */
#endif /* CONFIG_ANDROID_PMEM */

static struct fmem_platform_data msm8930_fmem_pdata = {
};

#define DSP_RAM_BASE_8960 0x8da00000
#define DSP_RAM_SIZE_8960 0x1800000
static int dspcrashd_pdata_8960 = 0xDEADDEAD;

static struct resource resources_dspcrashd_8960[] = {
	{
		.name   = "msm_dspcrashd",
		.start  = DSP_RAM_BASE_8960,
		.end    = DSP_RAM_BASE_8960 + DSP_RAM_SIZE_8960,
		.flags  = IORESOURCE_DMA,
	},
};

static struct platform_device msm_device_dspcrashd_8960 = {
	.name           = "msm_dspcrashd",
	.num_resources  = ARRAY_SIZE(resources_dspcrashd_8960),
	.resource       = resources_dspcrashd_8960,
	.dev = { .platform_data = &dspcrashd_pdata_8960 },
};

static struct memtype_reserve msm8930_reserve_table[] __initdata = {
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
	msm8930_reserve_table[MEMTYPE_EBI1].size += msm8930_rtb_pdata.size;
	pr_info("mem_map: rtb reserved with size 0x%x in pool\n",
			msm8930_rtb_pdata.size);
#endif
}

static void __init size_pmem_devices(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	android_pmem_adsp_pdata.size = pmem_adsp_size;
	android_pmem_pdata.size = pmem_size;
	android_pmem_audio_pdata.size = MSM_PMEM_AUDIO_SIZE;
#endif /*CONFIG_MSM_MULTIMEDIA_USE_ION*/
#endif /*CONFIG_ANDROID_PMEM*/
}

#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static void __init reserve_memory_for(struct android_pmem_platform_data *p)
{
	msm8930_reserve_table[p->memory_type].size += p->size;
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
#endif /*CONFIG_MSM_MULTIMEDIA_USE_ION*/
	msm8930_reserve_table[MEMTYPE_EBI1].size += msm_contig_mem_size;
	pr_info("mem_map: contig_mem reserved with size 0x%x in pool\n",
			msm_contig_mem_size);
#endif /*CONFIG_ANDROID_PMEM*/
}

static int msm8930_paddr_to_memtype(unsigned int paddr)
{
	return MEMTYPE_EBI1;
}

#define FMEM_ENABLED 0
#ifdef CONFIG_ION_MSM
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct ion_cp_heap_pdata cp_mm_zara_ion_pdata = {
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

static struct ion_cp_heap_pdata cp_mfc_zara_ion_pdata = {
	.permission_type = IPT_TYPE_MFC_SHAREDMEM,
	.align = PAGE_SIZE,
	.reusable = 0,
	.mem_is_fmem = FMEM_ENABLED,
	.fixed_position = FIXED_HIGH,
};

static struct ion_co_heap_pdata co_zara_ion_pdata = {
	.adjacent_mem_id = INVALID_HEAP_ID,
	.align = PAGE_SIZE,
	.mem_is_fmem = 0,
};

static struct ion_co_heap_pdata fw_co_zara_ion_pdata = {
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
static struct ion_platform_heap msm8930_heaps[] = {
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
			.extra_data = (void *) &cp_mm_zara_ion_pdata,
			.priv	= &ion_mm_heap_device.dev
		},
		{
			.id	= ION_MM_FIRMWARE_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_MM_FIRMWARE_HEAP_NAME,
			.size	= MSM_ION_MM_FW_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &fw_co_zara_ion_pdata,
		},
		{
			.id	= ION_CP_MFC_HEAP_ID,
			.type	= ION_HEAP_TYPE_CP,
			.name	= ION_MFC_HEAP_NAME,
			.size	= MSM_ION_MFC_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &cp_mfc_zara_ion_pdata,
		},
#ifndef CONFIG_MSM_IOMMU
		{
			.id	= ION_SF_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_SF_HEAP_NAME,
			.size	= MSM_ION_SF_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_zara_ion_pdata,
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
			.extra_data = (void *) &co_zara_ion_pdata,
		},
		{
			.id	= ION_AUDIO_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_AUDIO_HEAP_NAME,
			.size	= MSM_ION_AUDIO_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_zara_ion_pdata,
		},
#ifdef CONFIG_CMA
		{
			.id	= ION_ADSP_HEAP_ID,
			.type	= ION_HEAP_TYPE_DMA,
			.name	= ION_ADSP_HEAP_NAME,
			.size	= MSM_ION_ADSP_SIZE,
			.memory-type = ION_EBI_TYPE,
			.extra_data = (void *) &co_zara_ion_pdata,
			.priv	= &ion_adsp_heap_device.dev,
		},
#endif
#endif
};

static struct ion_platform_data msm8930_ion_pdata = {
	.nr = MSM_ION_HEAP_NUM,
	.heaps = msm8930_heaps,
};

static struct platform_device msm8930_ion_dev = {
	.name = "ion-msm",
	.id = 1,
	.dev = { .platform_data = &msm8930_ion_pdata },
};
#endif

static struct platform_device msm8930_fmem_device = {
	.name = "fmem",
	.id = 1,
	.dev = { .platform_data = &msm8930_fmem_pdata },
};

static void __init reserve_mem_for_ion(enum ion_memory_types mem_type,
				      unsigned long size)
{
	msm8930_reserve_table[mem_type].size += size;
}

static void __init msm8930_reserve_fixed_area(unsigned long fixed_area_size)
{
#if defined(CONFIG_ION_MSM) && defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	int ret;

	if (fixed_area_size > MAX_FIXED_AREA_SIZE)
		panic("fixed area size is larger than %dM\n",
			MAX_FIXED_AREA_SIZE >> 20);

	reserve_info->fixed_area_size = fixed_area_size;
	reserve_info->fixed_area_start = MSM8930_FW_START;

	ret = memblock_remove(reserve_info->fixed_area_start,
		reserve_info->fixed_area_size);
	pr_info("mem_map: fixed_area reserved at 0x%lx with size 0x%lx\n",
			reserve_info->fixed_area_start,
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

	for (i = 0; i < msm8930_ion_pdata.nr; ++i) {
		struct ion_platform_heap *heap =
						&(msm8930_ion_pdata.heaps[i]);

		int use_cma = 0;

		if (heap->extra_data) {
			int fixed_position = NOT_FIXED;

			switch ((int) heap->type) {
			case ION_HEAP_TYPE_CP:
				if (((struct ion_cp_heap_pdata *)
					heap->extra_data)->is_cma) {
					heap->size = ALIGN(heap->size,
						cma_alignment);
						use_cma = 1;
				}
				fixed_position = ((struct ion_cp_heap_pdata *)
					heap->extra_data)->fixed_position;
				break;
			case ION_HEAP_TYPE_DMA:
				use_cma = 1;
				/* Purposely fall through here */
			case ION_HEAP_TYPE_CARVEOUT:
				fixed_position = ((struct ion_co_heap_pdata *)
					heap->extra_data)->fixed_position;
				break;
			default:
				break;
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

	fixed_low_start = MSM8930_FIXED_AREA_START;
	if (low_use_cma) {
		BUG_ON(!IS_ALIGNED(fixed_low_size + HOLE_SIZE, cma_alignment));
		BUG_ON(!IS_ALIGNED(fixed_low_start, cma_alignment));
	} else {
		BUG_ON(!IS_ALIGNED(fixed_low_size + HOLE_SIZE, SECTION_SIZE));
		ret = memblock_remove(fixed_low_start,
				      fixed_low_size + HOLE_SIZE);
		pr_info("mem_map: fixed_low_area reserved at 0x%lx with size \
				0x%x\n", fixed_low_start,
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
		pr_info("mem_map: fixed_middle_area reserved at 0x%lx with \
				size 0x%x\n", fixed_middle_start,
				fixed_middle_size);
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
		pr_info("mem_map: fixed_high_area reserved at 0x%lx with size \
				0x%x\n", fixed_high_start,
				fixed_high_size);
		BUG_ON(ret);
	}

	for (i = 0; i < msm8930_ion_pdata.nr; ++i) {
		struct ion_platform_heap *heap = &(msm8930_ion_pdata.heaps[i]);

		if (heap->extra_data) {
			int fixed_position = NOT_FIXED;
			struct ion_cp_heap_pdata *pdata = NULL;

			switch ((int) heap->type) {
			case ION_HEAP_TYPE_CP:
				pdata =
				(struct ion_cp_heap_pdata *)heap->extra_data;
				fixed_position = pdata->fixed_position;
				break;
			case ION_HEAP_TYPE_DMA:
			case ION_HEAP_TYPE_CARVEOUT:
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
				if (middle_use_cma)
					dma_declare_contiguous(
						heap->priv,
						heap->size,
						fixed_middle_start,
						0xa0000000);
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
	zara_mdp_writeback(msm8930_reserve_table);
}

#ifdef CONFIG_MSM_CACHE_DUMP
static void __init reserve_cache_dump_memory(void)
{
	unsigned int total;

	total = msm8930_cache_dump_pdata.l1_size +
		msm8930_cache_dump_pdata.l2_size;
	msm8930_reserve_table[MEMTYPE_EBI1].size += total;
	pr_info("mem_map: cache_dump reserved with size 0x%x in pool\n",
			total);
}
#else
static void __init reserve_cache_dump_memory(void) { }
#endif

static void __init msm8930_calculate_reserve_sizes(void)
{
	size_pmem_devices();
	reserve_pmem_memory();
	reserve_ion_memory();
	reserve_mdp_memory();
	reserve_rtb_memory();
	reserve_cache_dump_memory();
}

static struct reserve_info msm8930_reserve_info __initdata = {
	.memtype_reserve_table = msm8930_reserve_table,
	.calculate_reserve_sizes = msm8930_calculate_reserve_sizes,
	.reserve_fixed_area = msm8930_reserve_fixed_area,
	.paddr_to_memtype = msm8930_paddr_to_memtype,
};

static void __init zara_early_memory(void)
{
	reserve_info = &msm8930_reserve_info;
}

static void __init zara_reserve(void)
{
	msm_reserve();
}

static void __init msm8930_allocate_memory_regions(void)
{
	zara_allocate_fb_region();
}

#ifdef CONFIG_HTC_BATT_8960
static int critical_alarm_voltage_mv[] = {3000, 3200, 3400};

static struct htc_battery_platform_data htc_battery_pdev_data = {
	.guage_driver = 0,
	.chg_limit_active_mask = HTC_BATT_CHG_LIMIT_BIT_TALK |
	HTC_BATT_CHG_LIMIT_BIT_NAVI,
	.critical_low_voltage_mv = 3100,
	.critical_alarm_vol_ptr = critical_alarm_voltage_mv,
	.critical_alarm_vol_cols = sizeof(critical_alarm_voltage_mv) / sizeof(int),
	.overload_vol_thr_mv = 4000,
	.overload_curr_thr_ma = 0,
	.smooth_chg_full_delay_min = 1,
	.shutdown_voltage_critiria_setting = 3400,
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

static struct pm8921_charger_batt_param chg_batt_params[] = {
	[0] = {
		.max_voltage = 4200,
		.cool_bat_voltage = 4200,
		.warm_bat_voltage = 4000,
	},
	[1] = {
		.max_voltage = 4340,
		.cool_bat_voltage = 4340,
		.warm_bat_voltage = 4000,
	},
	[2] = {
		.max_voltage = 4300,
		.cool_bat_voltage = 4300,
		.warm_bat_voltage = 4000,
	},
	[3] = {
		.max_voltage = 4350,
		.cool_bat_voltage = 4350,
		.warm_bat_voltage = 4000,
	},
};

static struct single_row_lut fcc_temp_id_1 = {
	.x      = {-20,-10, 0, 10, 20, 30, 40},
	.y      = {1820, 2000, 2010, 2025, 2025, 2035, 2035},
	.cols   = 7
};

static struct single_row_lut fcc_sf_id_1 = {
	.x	= {0},
	.y	= {100},
	.cols	= 1
};

static struct sf_lut pc_sf_id_1 = {
	.rows	= 1,
	.cols		= 1,
	.row_entries = {0},
	.percent	= {100},
	.sf 	= {
				{100}
	}
};

static struct sf_lut rbatt_est_ocv_id_1 = {
	.rows 		= 1,
	.cols		= 2,
	.row_entries	= {20, 40},
	.percent	= {100},
	.sf		= {
				{140, 100}
	},
};

static struct pc_temp_ocv_lut  pc_temp_ocv_id_1 = {
	.rows       = 29,
	.cols       = 7,
	.temp       = {-20,-10, 0, 10, 20, 30, 40},
	.percent    = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
	.ocv        = {
					{4326,4322,4318,4316,4312,4312,4310},
					{4265,4265,4265,4265,4265,4265,4265},
					{4204,4227,4227,4227,4227,4225,4225},
					{4148,4175,4175,4175,4175,4173,4173},
					{4104,4126,4126,4126,4126,4124,4124},
					{4041,4079,4079,4079,4079,4077,4077},
					{3986,4028,4035,4035,4035,4032,4032},
					{3947,3974,3992,3992,3992,3992,3992},
					{3909,3928,3948,3948,3948,3948,3948},
					{3876,3888,3892,3892,3892,3892,3892},
					{3850,3857,3863,3863,3863,3863,3863},
					{3831,3833,3839,3839,3839,3839,3839},
					{3817,3818,3818,3818,3818,3818,3818},
					{3800,3799,3799,3799,3799,3799,3799},
					{3783,3781,3781,3781,3781,3781,3781},
					{3763,3766,3766,3759,3750,3750,3750},
					{3744,3733,3723,3715,3706,3706,3706},
					{3720,3709,3701,3696,3692,3692,3692},
					{3676,3686,3678,3667,3650,3650,3650},
					{3661,3666,3652,3640,3621,3621,3621},
					{3643,3642,3622,3609,3589,3589,3589},
					{3622,3613,3584,3570,3548,3548,3548},
					{3600,3584,3546,3531,3507,3507,3507},
					{3568,3540,3489,3473,3446,3446,3446},
					{3555,3515,3458,3435,3412,3412,3412},
					{3541,3489,3427,3398,3378,3378,3378},
					{3523,3454,3383,3345,3329,3329,3329},
					{3494,3398,3315,3262,3254,3254,3254},
					{3437,3286,3178,3102,3102,3102,3102}
	}
};

static struct sf_lut rbatt_sf_id_1 = {
	.rows        = 19,
	.cols        = 7,

	.row_entries = {-20,-10, 0, 10, 20, 30, 40},
	.percent     = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10},
	.sf          = {
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82},
					{240,166,156,138,82,82,82}
		}
};

static struct pm8921_bms_battery_data  bms_battery_data_id_1 = {
	.fcc		= 2100,
	.fcc_temp_lut		= &fcc_temp_id_1,
	.fcc_sf_lut			= &fcc_sf_id_1,
	.pc_temp_ocv_lut		= &pc_temp_ocv_id_1,
	.pc_sf_lut		= &pc_sf_id_1,
	.rbatt_sf_lut		= &rbatt_sf_id_1,
	.rbatt_est_ocv_lut		= &rbatt_est_ocv_id_1,
	.default_rbatt_mohm		=250,
	.delta_rbatt_mohm		= 0,
};

static struct single_row_lut fcc_temp_id_2 = {
	.x      = {-20,-10, 0, 10, 20, 30, 40},
	.y      = {1980, 2000, 2020, 2030, 2060, 2060, 2060},
	.cols   = 7
};

static struct single_row_lut fcc_sf_id_2 = {
	.x	= {0},
	.y	= {100},
	.cols	= 1
};

static struct sf_lut pc_sf_id_2 = {
	.rows	= 1,
	.cols		= 1,

	.row_entries	= {0},
	.percent	= {100},
	.sf		= {
			{100}
	}
};

static struct sf_lut rbatt_est_ocv_id_2 = {
	.rows		= 1,
	.cols		= 2,
	.row_entries	= {20, 40},
	.percent	= {100},
	.sf		= {
				{140, 100}
	},
};

static struct pc_temp_ocv_lut  pc_temp_ocv_id_2 = {
	.rows       = 29,
	.cols       = 7,
	.temp       = {-20,-10, 0, 10, 20, 30, 40},
	.percent    = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
	.ocv		= {
					{4316,4316,4316,4316,4316,4316,4316},
					{4246,4246,4246,4246,4246,4246,4246},
					{4182,4182,4182,4182,4182,4182,4182},
					{4124,4124,4124,4124,4124,4124,4124},
					{4076,4076,4076,4076,4076,4076,4076},
					{4033,4033,4033,4033,4033,4033,4033},
					{3990,3990,3990,3990,3990,3990,3990},
					{3951,3951,3951,3951,3951,3951,3951},
					{3905,3905,3905,3905,3905,3905,3905},
					{3853,3853,3853,3853,3853,3853,3853},
					{3827,3827,3827,3827,3827,3827,3827},
					{3807,3807,3807,3807,3807,3807,3807},
					{3790,3790,3790,3790,3790,3790,3790},
					{3778,3778,3778,3778,3778,3778,3778},
					{3767,3767,3767,3767,3767,3767,3767},
					{3751,3751,3751,3751,3751,3751,3751},
					{3732,3732,3732,3732,3732,3732,3732},
					{3691,3691,3691,3691,3691,3691,3691},
					{3661,3661,3661,3661,3661,3661,3661},
					{3652,3652,3652,3652,3652,3652,3652},
					{3644,3644,3644,3644,3644,3644,3644},
					{3635,3635,3635,3635,3635,3635,3635},
					{3627,3627,3627,3627,3627,3627,3627},
					{3618,3618,3618,3618,3618,3618,3618},
					{3569,3569,3569,3569,3569,3569,3569},
					{3495,3495,3495,3495,3495,3495,3495},
					{3470,3470,3470,3470,3470,3470,3470},
					{3408,3408,3408,3408,3408,3408,3408},
					{3000,3000,3000,3000,3000,3000,3000}
	}
};

static struct sf_lut rbatt_sf_id_2 = {
	.rows           = 19,
	.cols           = 7,

	.row_entries    = {-20,-10, 0, 10, 20, 30, 40},
	.percent        = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15, 10},
	.sf 	= {
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,},
								{215,158,151,138,130,113,99,}
		}
};

static struct pm8921_bms_battery_data  bms_battery_data_id_2 = {
	.fcc		= 2100,
	.fcc_temp_lut		= &fcc_temp_id_2,
	.fcc_sf_lut		= &fcc_sf_id_2,
	.pc_temp_ocv_lut	= &pc_temp_ocv_id_2,
	.pc_sf_lut		= &pc_sf_id_2,
	.rbatt_sf_lut		= &rbatt_sf_id_2,
	.rbatt_est_ocv_lut	= &rbatt_est_ocv_id_2,
	.default_rbatt_mohm		=250,
	.delta_rbatt_mohm		= 0,
};

static struct htc_battery_cell htc_battery_cells[] = {
	[0] = {
		.model_name = "ICP286083L1",
		.capacity = 2100,
		.id = 1,
		.id_raw_min = 50,
		.id_raw_max = 204,
		.type = HTC_BATTERY_CELL_TYPE_HV,
		.voltage_max = 4340,
		.voltage_min = 3200,
		.chg_param = &chg_batt_params[1],
		.gauge_param = &bms_battery_data_id_1,
	},
	[1] = {
		.model_name = "BL80100",
		.capacity = 2100,
		.id = 2,
		.id_raw_min = 204,
		.id_raw_max = 407,
		.type = HTC_BATTERY_CELL_TYPE_HV,
		.voltage_max = 4340,
		.voltage_min = 3200,
		.chg_param = &chg_batt_params[1],
		.gauge_param = &bms_battery_data_id_2,
	},
	[2] = {
		.model_name = "UNKNOWN",
		.capacity = 2100,
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

#ifdef CONFIG_REGULATOR_NCP6924
#define NCP6924_I2C_SLAVE_ADDR (0x20 >> 1)
static void ncp6924_voltage_init(void)
{
}

struct ncp6924_platform_data ncp6924_data = {
	.voltage_init	  = ncp6924_voltage_init,
	.en_gpio        = PM8038_GPIO_PM_TO_SYS(6),
	.en_init_val	  = 0x0,
	.dc1_init_val	  = 0x30,
	.dc2_init_val	  = 0x60,
	.ldo1_init_val  = 0x24,
	.ldo2_init_val  = 0x24,
	.ldo3_init_val  = 0x10,
	.ldo4_init_val  = 0x10,
};

static struct i2c_board_info ncp6924_info[] = {
	{
		I2C_BOARD_INFO( NCP6924_I2C_NAME, NCP6924_I2C_SLAVE_ADDR),
		.platform_data = &ncp6924_data,
	},
};
#endif

#define TFA9887_I2C_SLAVE_ADDR  (0x68 >> 1)
#define TFA9887L_I2C_SLAVE_ADDR (0x6A >> 1)
#define RT5501_I2C_SLAVE_ADDR	(0xF0 >> 1)

#ifdef CONFIG_AMP_RT5501
struct rt5501_platform_data rt5501_data = {
	.gpio_rt5501_spk_en = MSM_AUD_HP_EN,
};

static struct i2c_board_info msm_i2c_gsbi2_rt5501_info[] = {
	{
		I2C_BOARD_INFO( RT5501_I2C_NAME, RT5501_I2C_SLAVE_ADDR),
		.platform_data = &rt5501_data,
	},
};
#endif

#ifdef CONFIG_AMP_TFA9887
static struct i2c_board_info msm_i2c_gsbi2_tfa9887_info[] = {
	{
		I2C_BOARD_INFO(TFA9887_I2C_NAME, TFA9887_I2C_SLAVE_ADDR)
	},
#ifdef CONFIG_AMP_TFA9887L
	{
		I2C_BOARD_INFO(TFA9887L_I2C_NAME, TFA9887L_I2C_SLAVE_ADDR)
	},
#endif
};
#endif

#ifdef CONFIG_WCD9304_CODEC

#define SITAR_INTERRUPT_BASE (NR_MSM_IRQS + NR_GPIO_IRQS + NR_PM8921_IRQS)

/* Micbias setting is based on 8660 CDP/MTP/FLUID requirement
 * 4 micbiases are used to power various analog and digital
 * microphones operating at 1800 mV. Technically, all micbiases
 * can source from single cfilter since all microphones operate
 * at the same voltage level. The arrangement below is to make
 * sure all cfilters are exercised. LDO_H regulator ouput level
 * does not need to be as high as 2.85V. It is choosen for
 * microphone sensitivity purpose.
 */
static struct wcd9xxx_pdata sitar_platform_data = {
		.slimbus_slave_device = {
		.name = "sitar-slave",
		.e_addr = {0, 0, 0x00, 0, 0x17, 2},
	},
	.amic_settings = {
		.legacy_mode = 0x7F,
		.use_pdata = 0x7F,
	},
	.irq = MSM_GPIO_TO_INT(MSM_AUD_WCD_INTR_OUT),
	.irq_base = SITAR_INTERRUPT_BASE,
	.num_irqs = NR_WCD9XXX_IRQS,
	.reset_gpio = MSM_AUD_WCD_RESET_N,
	.micbias = {
		.ldoh_v = SITAR_LDOH_2P85_V,
		.cfilt1_mv = 1800,
		.cfilt2_mv = 1800,
		.bias1_cfilt_sel = SITAR_CFILT1_SEL,
		.bias2_cfilt_sel = SITAR_CFILT2_SEL,
		.bias1_cap_mode = MICBIAS_NO_EXT_BYP_CAP,
		.bias2_cap_mode = MICBIAS_NO_EXT_BYP_CAP,
	},
	.regulator = {
	{
		.name = "CDC_VDD_CP",
		.min_uV = 2200000,
		.max_uV = 2200000,
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
		.min_uV = 1200000,
		.max_uV = 1250000,
		.optimum_uA = WCD9XXX_VDDD_CDC_D_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_A_1P2V",
		.min_uV = 1200000,
		.max_uV = 1250000,
		.optimum_uA = WCD9XXX_VDDD_CDC_A_CUR_MAX,
	},
	},
};

static struct slim_device msm_slim_sitar = {
	.name = "sitar-slim",
	.e_addr = {0, 1, 0x00, 0, 0x17, 2},
	.dev = {
	.platform_data = &sitar_platform_data,
	},
};

static struct wcd9xxx_pdata sitar1p1_platform_data = {
		.slimbus_slave_device = {
		.name = "sitar-slave",
		.e_addr = {0, 0, 0x70, 0, 0x17, 2},
	},
	.irq = MSM_GPIO_TO_INT(62),
	.irq_base = SITAR_INTERRUPT_BASE,
	.num_irqs = NR_WCD9XXX_IRQS,
	.reset_gpio = 42,
	.micbias = {
		.ldoh_v = SITAR_LDOH_2P85_V,
		.cfilt1_mv = 1800,
		.cfilt2_mv = 1800,
		.bias1_cfilt_sel = SITAR_CFILT1_SEL,
		.bias2_cfilt_sel = SITAR_CFILT2_SEL,
		.bias1_cap_mode = MICBIAS_NO_EXT_BYP_CAP,
		.bias2_cap_mode = MICBIAS_NO_EXT_BYP_CAP,
	},
	.regulator = {
	{
		.name = "CDC_VDD_CP",
		.min_uV = 2200000,
		.max_uV = 2200000,
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
		.min_uV = 1200000,
		.max_uV = 1250000,
		.optimum_uA = WCD9XXX_VDDD_CDC_D_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_A_1P2V",
		.min_uV = 1200000,
		.max_uV = 1250000,
		.optimum_uA = WCD9XXX_VDDD_CDC_A_CUR_MAX,
	},
	},
};

static struct slim_device msm_slim_sitar1p1 = {
	.name = "sitar1p1-slim",
	.e_addr = {0, 1, 0x70, 0, 0x17, 2},
	.dev = {
	.platform_data = &sitar1p1_platform_data,
	},
};
#endif

static struct slim_boardinfo msm_slim_devices[] = {
#ifdef CONFIG_WCD9304_CODEC
	{
		.bus_num = 1,
		.slim_slave = &msm_slim_sitar,
	},
	{
		.bus_num = 1,
		.slim_slave = &msm_slim_sitar1p1,
	},
#endif
};

#ifdef CONFIG_QSEECOM
/* qseecom bus scaling */
static struct msm_bus_vectors qseecom_clks_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ib = 0,
		.ab = 0,
	},
	{
		.src = MSM_BUS_MASTER_SPDM,
		.dst = MSM_BUS_SLAVE_SPDM,
		.ib = 0,
		.ab = 0,
	},
};

static struct msm_bus_vectors qseecom_enable_dfab_vectors[] = {
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ib = (492 * 8) * 1000000UL,
		.ab = (492 * 8) *  100000UL,
	},
	{
		.src = MSM_BUS_MASTER_SPDM,
		.dst = MSM_BUS_SLAVE_SPDM,
		.ib = 0,
		.ab = 0,
	},
};

static struct msm_bus_vectors qseecom_enable_sfpb_vectors[] = {
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ib = 0,
		.ab = 0,
	},
	{
		.src = MSM_BUS_MASTER_SPDM,
		.dst = MSM_BUS_SLAVE_SPDM,
		.ib = (64 * 8) * 1000000UL,
		.ab = (64 * 8) *  100000UL,
	},
};

static struct msm_bus_paths qseecom_hw_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(qseecom_clks_init_vectors),
		qseecom_clks_init_vectors,
	},
	{
		ARRAY_SIZE(qseecom_enable_dfab_vectors),
		qseecom_enable_dfab_vectors,
	},
	{
		ARRAY_SIZE(qseecom_enable_sfpb_vectors),
		qseecom_enable_sfpb_vectors,
	},
};

static struct msm_bus_scale_pdata qseecom_bus_pdata = {
	qseecom_hw_bus_scale_usecases,
	ARRAY_SIZE(qseecom_hw_bus_scale_usecases),
	.name = "qsee",
};

static struct platform_device qseecom_device = {
	.name		= "qseecom",
	.id		= 0,
	.dev		= {
		.platform_data = &qseecom_bus_pdata,
	},
};
#endif

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

static int64_t k2_zara_get_usbid_adc(void)
{
	struct pm8xxx_adc_chan_result result;
	int err = 0, adc =0;

	err = pm8xxx_adc_mpp_config_read(PM8XXX_AMUX_MPP_3, ADC_MPP_1_AMUX6, &result);
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

static uint32_t usb_ID_PIN_input_table[] = {
	GPIO_CFG(MSM_USB_ID1, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t usb_ID_PIN_ouput_table[] = {
	GPIO_CFG(MSM_USB_ID1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static void config_k2_zara_usb_id_gpios(bool output)
{
	if (output) {
		gpio_tlmm_config(usb_ID_PIN_ouput_table[0], GPIO_CFG_ENABLE);
		gpio_set_value(MSM_USB_ID1, 1);
		pr_info("[CABLE] %s: %d output high\n",  __func__, MSM_USB_ID1);
	} else {
		gpio_tlmm_config(usb_ID_PIN_input_table[0], GPIO_CFG_ENABLE);
		pr_info("[CABLE] %s: %d input none pull\n",  __func__, MSM_USB_ID1);
	}
}

static struct cable_detect_platform_data cable_detect_pdata = {
	.detect_type		= CABLE_TYPE_PMIC_ADC,
	.usb_id_pin_gpio	= MSM_USB_ID1,
	.get_adc_cb		= k2_zara_get_usbid_adc,
	.config_usb_id_gpios	= config_k2_zara_usb_id_gpios,
};

static struct platform_device cable_detect_device = {
	.name   = "cable_detect",
	.id     = -1,
	.dev    = {
		.platform_data = &cable_detect_pdata,
	},
};

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

static struct msm_ce_hw_support qcedev_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
	.sha_hmac = QCE_SHA_HMAC_SUPPORT,
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

#ifdef CONFIG_MSM_MPM
static uint16_t msm_mpm_irqs_m2a[MSM_MPM_NR_MPM_IRQS] __initdata = {
	[1] = MSM_GPIO_TO_INT(46),
	[2] = MSM_GPIO_TO_INT(150),
	[4] = MSM_GPIO_TO_INT(103),
	[5] = MSM_GPIO_TO_INT(104),
	[6] = MSM_GPIO_TO_INT(105),
	[7] = MSM_GPIO_TO_INT(106),
	[9] = MSM_GPIO_TO_INT(7),
	[10] = MSM_GPIO_TO_INT(11),
	[11] = MSM_GPIO_TO_INT(15),
	[12] = MSM_GPIO_TO_INT(19),
	[13] = MSM_GPIO_TO_INT(23),
	[14] = MSM_GPIO_TO_INT(27),
	[15] = MSM_GPIO_TO_INT(31),
	[16] = MSM_GPIO_TO_INT(35),
	[19] = MSM_GPIO_TO_INT(90),
	[20] = MSM_GPIO_TO_INT(92),
	[23] = MSM_GPIO_TO_INT(85),
	[24] = MSM_GPIO_TO_INT(83),
	[25] = USB1_HS_IRQ,
	[26] = MSM_GPIO_TO_INT(6),
	[27] = HDMI_IRQ,
	[29] = MSM_GPIO_TO_INT(10),
	[30] = MSM_GPIO_TO_INT(102),
	[31] = MSM_GPIO_TO_INT(81),
	[32] = MSM_GPIO_TO_INT(78),
	[33] = MSM_GPIO_TO_INT(94),
	[34] = MSM_GPIO_TO_INT(72),
	[35] = MSM_GPIO_TO_INT(39),
	[36] = MSM_GPIO_TO_INT(43),
	[37] = MSM_GPIO_TO_INT(61),
	[38] = MSM_GPIO_TO_INT(50),
	[39] = MSM_GPIO_TO_INT(42),
	[41] = MSM_GPIO_TO_INT(62),
	[42] = MSM_GPIO_TO_INT(8),
	[43] = MSM_GPIO_TO_INT(33),
	[44] = MSM_GPIO_TO_INT(70),
	[45] = MSM_GPIO_TO_INT(69),
	[46] = MSM_GPIO_TO_INT(67),
	[47] = MSM_GPIO_TO_INT(65),
	[48] = MSM_GPIO_TO_INT(55),
	[49] = MSM_GPIO_TO_INT(74),
	[50] = MSM_GPIO_TO_INT(98),
	[51] = MSM_GPIO_TO_INT(49),
	[52] = MSM_GPIO_TO_INT(40),
	[53] = MSM_GPIO_TO_INT(37),
	[54] = MSM_GPIO_TO_INT(24),
	[55] = MSM_GPIO_TO_INT(14),
};

static uint16_t msm_mpm_bypassed_apps_irqs[] __initdata = {
	TLMM_MSM_SUMMARY_IRQ,
	RPM_APCC_CPU0_GP_HIGH_IRQ,
	RPM_APCC_CPU0_GP_MEDIUM_IRQ,
	RPM_APCC_CPU0_GP_LOW_IRQ,
	RPM_APCC_CPU0_WAKE_UP_IRQ,
	RPM_APCC_CPU1_GP_HIGH_IRQ,
	RPM_APCC_CPU1_GP_MEDIUM_IRQ,
	RPM_APCC_CPU1_GP_LOW_IRQ,
	RPM_APCC_CPU1_WAKE_UP_IRQ,
	MSS_TO_APPS_IRQ_0,
	MSS_TO_APPS_IRQ_1,
	MSS_TO_APPS_IRQ_2,
	MSS_TO_APPS_IRQ_3,
	MSS_TO_APPS_IRQ_4,
	MSS_TO_APPS_IRQ_5,
	MSS_TO_APPS_IRQ_6,
	MSS_TO_APPS_IRQ_7,
	MSS_TO_APPS_IRQ_8,
	MSS_TO_APPS_IRQ_9,
	LPASS_SCSS_GP_LOW_IRQ,
	LPASS_SCSS_GP_MEDIUM_IRQ,
	LPASS_SCSS_GP_HIGH_IRQ,
	SPS_MTI_30,
	SPS_MTI_31,
	RIVA_APSS_SPARE_IRQ,
	RIVA_APPS_WLAN_SMSM_IRQ,
	RIVA_APPS_WLAN_RX_DATA_AVAIL_IRQ,
	RIVA_APPS_WLAN_DATA_XFER_DONE_IRQ,
};

static struct msm_mpm_device_data msm8930_mpm_dev_data __initdata = {
	.irqs_m2a = msm_mpm_irqs_m2a,
	.irqs_m2a_size = ARRAY_SIZE(msm_mpm_irqs_m2a),
	.bypassed_apps_irqs = msm_mpm_bypassed_apps_irqs,
	.bypassed_apps_irqs_size = ARRAY_SIZE(msm_mpm_bypassed_apps_irqs),
	.mpm_request_reg_base = MSM_RPM_BASE + 0x9d8,
	.mpm_status_reg_base = MSM_RPM_BASE + 0xdf8,
	.mpm_apps_ipc_reg = MSM_APCS_GCC_BASE + 0x008,
	.mpm_apps_ipc_val =  BIT(1),
	.mpm_ipc_irq = RPM_APCC_CPU0_GP_MEDIUM_IRQ,
};
#endif

#define MSM_SHARED_RAM_PHYS 0x80000000

static void __init zara_map_io(void)
{
	msm_shared_ram_phys = MSM_SHARED_RAM_PHYS;
	msm_map_msm8930_io();

	if (socinfo_init() < 0)
		pr_err("socinfo_init() failed!\n");
}

static void __init zara_init_irq(void)
{
	struct msm_mpm_device_data *data = NULL;
#ifdef CONFIG_MSM_MPM
	data = &msm8930_mpm_dev_data;
#endif

	msm_mpm_irq_extn_init(data);
	gic_init(0, GIC_PPI_START, MSM_QGIC_DIST_BASE,
						(void *)MSM_QGIC_CPU_BASE);
}

static DEFINE_MUTEX(sensor_lock);
static struct regulator *pl_reg_l9;
static int capella_pl_sensor_lpm_power(uint8_t enable)
{
	int rc = 0;

	mutex_lock(&sensor_lock);

	if (pl_reg_l9 == NULL) {
		pl_reg_l9 = regulator_get(NULL, "8038_l9");
		if (IS_ERR(pl_reg_l9)) {
			pr_err("[PS][cm3629] %s: Unable to get '8038_l9' \n", __func__);
			mutex_unlock(&sensor_lock);
			pr_info("[PS][cm3629] %s: sensor_lock unlock 1\n", __func__);
			return -ENODEV;
		}
	}
	if (enable == 1) {
		rc = regulator_set_optimum_mode(pl_reg_l9, 100);
		if (rc < 0)
			pr_err("[PS][cm3629] %s: enter lmp,set_optimum_mode l6 failed, rc=%d\n", __func__, rc);
		else
			pr_info("[PS][cm3629] %s: enter lmp,OK\n", __func__);
	} else {
		rc = regulator_set_optimum_mode(pl_reg_l9, 100000);
		if (rc < 0)
			pr_err("[PS][cm3629] %s: leave lmp,set_optimum_mode l6 failed, rc=%d\n", __func__, rc);
		else
			pr_info("[PS][cm3629] %s: leave lmp,OK\n", __func__);
		msleep(10);
	}
	mutex_unlock(&sensor_lock);
	pr_info("[PS][cm3629] %s: sensor_lock unlock 2\n", __func__);
	return rc;
}

static int cm3629_power(int ls_or_ps, uint8_t enable)
{
	int rc = 0;

	mutex_lock(&sensor_lock);

	if (pl_reg_l9 == NULL) {
		pl_reg_l9 = regulator_get(NULL, "8038_l9");
		if (IS_ERR(pl_reg_l9)) {
			pr_err("[PS][cm3629] %s: Unable to get '8038_l9' \n", __func__);
			mutex_unlock(&sensor_lock);
			pr_info("[PS][cm3629] %s: sensor_lock unlock 3\n", __func__);
			return -ENODEV;
		}
	}
	if (enable == 1) {
		rc = regulator_set_voltage(pl_reg_l9, 2850000, 2850000);
		if (rc)
			pr_err("[PS][cm3629] %s: unable to regulator_set_voltage, rc:%d\n", __func__, rc);

		rc = regulator_enable(pl_reg_l9);
		if (rc)
			pr_err("[PS][cm3629]'%s' regulator enable L9 failed, rc=%d\n", __func__,rc);
		else
			pr_info("[PS][cm3629]'%s' L9 power on\n", __func__);
	}
	mutex_unlock(&sensor_lock);
	pr_info("[PS][cm3629] %s: sensor_lock unlock 4\n", __func__);
	return rc;
}

static uint8_t cm3629_mapping_table[] = {
	0x0, 0x3, 0x6, 0x9, 0xC,
	0xF, 0x12, 0x15, 0x18, 0x1B,
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

static struct cm3629_platform_data cm36282_pdata = {
	.model = CAPELLA_CM36282,
	.ps_select = CM3629_PS1_ONLY,
	.intr = MSM_PROXIMITY_INTz,
	.levels = { 0x19, 0x35, 0x50, 0x1E5, 0x461, 0x1451, 0x20B5, 0x556B, 0x622C, 0xFFFF},
	.golden_adc = 0x1010,
	.power = cm3629_power,
	.lpm_power = capella_pl_sensor_lpm_power,
	.cm3629_slave_address = 0xC0>>1,
	.ps1_thd_set = 0x14,
	.ps1_thh_diff = 1,
	.ps_th_add = 7,
	.ps1_thd_no_cal = 0xF1,
	.ps1_thd_with_cal = 0x14,
	.ps_calibration_rule = 1,
	.ps_conf1_val = CM3629_PS_DR_1_40 | CM3629_PS_IT_1_6T |
			CM3629_PS1_PERS_2,
	.ps_conf2_val = CM3629_PS_ITB_1 | CM3629_PS_ITR_1 |
			CM3629_PS2_INT_DIS | CM3629_PS1_INT_DIS,
	.ps_conf3_val = CM3629_PS2_PROL_32,
	.dark_level = 3,
	.dynamical_threshold = 1,
	.mapping_table = cm3629_mapping_table,
	.mapping_size = ARRAY_SIZE(cm3629_mapping_table),
};

static struct i2c_board_info i2c_CM36282_devices[] = {
	{
		I2C_BOARD_INFO(CM3629_I2C_NAME, 0xC0 >> 1),
		.platform_data = &cm36282_pdata,
		.irq =  MSM_GPIO_TO_INT(MSM_PROXIMITY_INTz),
	},
};

static void __init zara_init_buses(void)
{
#ifdef CONFIG_MSM_BUS_SCALING
	msm_bus_rpm_set_mt_mask();
	msm_bus_8930_apps_fabric_pdata.rpm_enabled = 1;
	msm_bus_8930_sys_fabric_pdata.rpm_enabled = 1;
	msm_bus_8930_mm_fabric_pdata.rpm_enabled = 1;
	msm_bus_8930_apps_fabric.dev.platform_data =
		&msm_bus_8930_apps_fabric_pdata;
	msm_bus_8930_sys_fabric.dev.platform_data =
		&msm_bus_8930_sys_fabric_pdata;
	msm_bus_8930_mm_fabric.dev.platform_data =
		&msm_bus_8930_mm_fabric_pdata;
	msm_bus_8930_sys_fpb.dev.platform_data = &msm_bus_8930_sys_fpb_pdata;
	msm_bus_8930_cpss_fpb.dev.platform_data = &msm_bus_8930_cpss_fpb_pdata;
#endif
}

static struct msm_spi_platform_data msm8930_qup_spi_gsbi10_pdata = {
	.max_clock_speed = 27000000,
};

static int msm_hsusb_vbus_power(bool on)
{
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (on) {
		rc = gpio_request(MSM_BOOST_5V_EN, "USB_BOOST_5V");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_BOOST_5V", MSM_BOOST_5V_EN, rc);
			return rc;
		}
		gpio_set_value(MSM_BOOST_5V_EN, 1);
		pr_info("%s(on): success\n", __func__);
	} else {
		gpio_set_value(MSM_BOOST_5V_EN, 0);
		gpio_free(MSM_BOOST_5V_EN);
		pr_info("%s(off): success\n", __func__);
	}

	prev_on = on;
	return 0;
}

#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors usb_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors usb_max_vectors[] = {
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 60000000,
		.ib = 960000000,
	},
};

static struct msm_bus_paths usb_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(usb_init_vectors),
		usb_init_vectors,
	},
	{
		ARRAY_SIZE(usb_max_vectors),
		usb_max_vectors,
	},
};

static struct msm_bus_scale_pdata usb_bus_scale_pdata = {
	usb_bus_scale_usecases,
	ARRAY_SIZE(usb_bus_scale_usecases),
	.name = "usb",
};
#endif

static int zara_phy_init_seq[] = {
	0x3C, 0x81,
	0x24, 0x82,
	-1
};

static int zara_phy_init_seq_no_uart[] = {
	0x3b, 0x81,
	0x24, 0x82,
	-1
};

static struct msm_otg_platform_data msm_otg_pdata = {
	.phy_init_seq	= zara_phy_init_seq,
	.mode			= USB_OTG,
	.otg_control		= OTG_PMIC_CONTROL,
	.phy_type		= SNPS_28NM_INTEGRATED_PHY,
	.vbus_power		= msm_hsusb_vbus_power,
	.power_budget		= 750,
#ifdef CONFIG_MSM_BUS_SCALING
	.bus_scale_table	= &usb_bus_scale_pdata,
#endif
};

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

	dload->magic_struct.pid = PID_MAGIC_ID;
	dload->pid = pid;


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
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};

static void zara_add_usb_devices(void)
{
	printk(KERN_INFO "%s rev: %d\n", __func__, system_rev);
	if ((system_rev >= PVT_VERSION) && (system_rev < EVM_VERSION))
		msm_otg_pdata.phy_init_seq = zara_phy_init_seq_no_uart;

	printk(KERN_INFO "%s: OTG_PMIC_CONTROL in rev: %d\n",
			__func__, system_rev);
}

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
		.reg_init_values[MSM_SPM_REG_SAW2_CFG] = 0x1F,
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
		.reg_init_values[MSM_SPM_REG_SAW2_CFG] = 0x1F,
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

static void config_flashlight_gpios(void)
{
	static uint32_t flashlight_gpio_table[] = {
		GPIO_CFG(MSM_DRIVER_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG(MSM_TORCH_FLASHz, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	};

	gpio_tlmm_config(flashlight_gpio_table[0], GPIO_CFG_ENABLE);
	gpio_tlmm_config(flashlight_gpio_table[1], GPIO_CFG_ENABLE);
}

static struct TPS61310_flashlight_platform_data flashlight_data = {
	.gpio_init = config_flashlight_gpios,
	.tps61310_strb0 = MSM_DRIVER_EN,
	.tps61310_strb1 = MSM_TORCH_FLASHz,
	.flash_duration_ms = 600,
	.led_count = 1,
	.disable_tx_mask = 1,
};

static struct i2c_board_info i2c_tps61310_flashlight[] = {
	{
		I2C_BOARD_INFO("TPS61310_FLASHLIGHT", 0x66 >> 1),
		.platform_data = &flashlight_data,
	},
};

#ifdef CONFIG_SENSORS_NFC_PN544
static void nfc_gpio_init(void)
{
	static uint32_t nfc_gpio_table[] = {
	GPIO_CFG(MSM_NFC_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	};
	printk(KERN_INFO"[NFC] %s, config NFC_IRQ pin\n",__func__);
	gpio_tlmm_config(nfc_gpio_table[0], GPIO_CFG_ENABLE);
	return;
}

static struct pn544_i2c_platform_data nfc_platform_data = {
	.irq_gpio = MSM_NFC_IRQ,
	.ven_gpio = MSM_NFC_VEN,
	.firm_gpio = MSM_NFC_DL_MODE,
	.ven_isinvert = 1,
	.gpio_init = nfc_gpio_init,
};

static struct i2c_board_info pn544_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO(PN544_I2C_NAME, 0x50 >> 1),
		.platform_data = &nfc_platform_data,
		.irq = MSM_GPIO_TO_INT(MSM_NFC_IRQ),
	},
};

#endif

static ssize_t syn_vkeys_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 200,
	__stringify(EV_KEY) ":" __stringify(KEY_BACK) ":100:1010:90:90"
	":" __stringify(EV_KEY) ":" __stringify(KEY_HOME) ":460:1010:90:90"
	"\n");
}

static struct kobj_attribute syn_vkeys_attr = {
	.attr = {
		.name = "virtualkeys.synaptics-rmi-touchscreen",
		.mode = S_IRUGO,
	},
	.show = &syn_vkeys_show,
};

static struct attribute *syn_properties_attrs[] = {
	&syn_vkeys_attr.attr,
	NULL
};

static struct attribute_group syn_properties_attr_group = {
	.attrs = syn_properties_attrs,
};

#define ZARA_GPIO_CAM_I2C_DAT 	MSM_CAM_I2C_SDA
#define ZARA_GPIO_CAM_I2C_CLK 	MSM_CAM_I2C_SCL

static uint32_t gsbi3_gpio_table[] = {
	GPIO_CFG(MSM_TP_I2C_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(MSM_TP_I2C_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static int synaptic_rmi_tp_power(int on)
{
	static struct regulator *reg_lvs2;
	static uint32_t tp_power[] = {
		GPIO_CFG(MSM_V_TP_LED_3V3_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, 0),
		GPIO_CFG(MSM_TP_ATTz, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, 0),
	};

	pr_info("[TP] %s\n", __func__);

	reg_lvs2 = regulator_get(NULL, "8038_lvs2");
	if (IS_ERR(reg_lvs2)) {
		pr_err("[TP] Not get 8038_lvs2, ret = %ld\n", PTR_ERR(reg_lvs2));
		return -ENODEV;
	}

	if (regulator_enable(reg_lvs2)) {
		pr_err("[TP] unable to enable lvs2\n");
		return -1;
	}
	msleep(1000);

	gpio_tlmm_config(gsbi3_gpio_table[0], GPIO_CFG_ENABLE);
	gpio_tlmm_config(gsbi3_gpio_table[1], GPIO_CFG_ENABLE);

	msleep(500);


	gpio_set_value(MSM_TP_RSTz, 0);
	msleep(50);
	gpio_set_value(MSM_TP_RSTz, 1);

	gpio_tlmm_config(tp_power[1], GPIO_CFG_ENABLE);
	return 0;
}

static DEFINE_MUTEX(tp_lock);
static struct regulator *tp_reg_l17;
static int synaptics_power_LPM(int on)
{
	int rc = 0;
	if (system_rev < PVT_VERSION) {
		mutex_lock(&tp_lock);
		pr_info("[TP] %s: enter:%d\n", __func__, on);

		if (tp_reg_l17 == NULL) {
			tp_reg_l17 = regulator_get(NULL, "8038_l17");
			if (IS_ERR(tp_reg_l17)) {
				pr_err("[TP] %s: Unable to get '8038_l17' \n", __func__);
				mutex_unlock(&tp_lock);
				return -ENODEV;
			}
		}
		if (on == 1) {
			rc = regulator_set_optimum_mode(tp_reg_l17, 100);
			if (rc < 0)
				pr_err("[TP] %s: enter LPM,set_optimum_mode l17 failed, rc=%d\n", __func__, rc);

			rc = regulator_enable(tp_reg_l17);
			if (rc) {
				pr_err("'%s' regulator enable failed rc=%d\n",
						"tp_reg_l17", rc);
				mutex_unlock(&tp_lock);
				return rc;
			}
			pr_info("[TP] %s: enter LPM mode\n", __func__);
		} else {
			rc = regulator_set_optimum_mode(tp_reg_l17, 100000);
			if (rc < 0)
				pr_err("[TP] %s: leave LPM,set_optimum_mode l17 failed, rc=%d\n", __func__, rc);

			rc = regulator_enable(tp_reg_l17);
			if (rc) {
				pr_err("'%s' regulator enable failed, rc=%d\n",
						"tp_reg_l17", rc);
				mutex_unlock(&tp_lock);
				return rc;
			}
			pr_info("[TP] %s: leave LPM mode\n", __func__);
			usleep(10);
		}
		mutex_unlock(&tp_lock);
	} else
		pr_info("[TP] pvt device, not control L17");

	return rc;
}

static struct synaptics_virtual_key zara_vk_data[] = {
	{
		.index = 1,
		.keycode = KEY_BACK,
		.x_range_min = 80,
		.x_range_max = 130,
		.y_range_min = 1460,
		.y_range_max = 1660,
	},
	{
		.index = 2,
		.keycode = KEY_HOME,
		.x_range_min = 560,
		.x_range_max = 860,
		.y_range_min = 1460,
		.y_range_max = 1660,
	},
	{
		.index = 0,
	},
};

static struct synaptics_i2c_rmi_platform_data syn_ts_3k_data[] = {
	{
		.version = 0x3332,
		.packrat_number = 1514368,
		.abs_x_min = 0,
		.abs_x_max = 810,
		.abs_y_min = 0,
		.abs_y_max = 1440,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		.gpio_reset = MSM_TP_RSTz,
		.i2c_err_handler_en = 1,
		.tw_pin_mask = 0x0080,
		.sensor_id = 0x0080 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.default_config = 1,
		.virtual_key = zara_vk_data,
		.lpm_power = synaptics_power_LPM,
		.config = {
			0x33, 0x32, 0x00, 0x08, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x50,
			0xA5, 0x18, 0x24, 0x03, 0x01, 0x3C, 0x21, 0x01,
			0x21, 0x01, 0xD7, 0x5D, 0xC1, 0x5A, 0x7C, 0xB3,
			0xB4, 0xB4, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xAD, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x11, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF4, 0xC8,
			0xDC, 0x42, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x1E, 0x05, 0x00, 0x02, 0xA4,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x12, 0x3E, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x64, 0x40, 0x60, 0x60,
			0x60, 0x48, 0x48, 0x40, 0x40, 0x40, 0x2A, 0x2A,
			0x29, 0x27, 0x26, 0x25, 0x24, 0x23, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xE8,
			0xFD, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x00, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x5E, 0x61,
			0x63, 0x66, 0x69, 0x6C, 0x6F, 0x39, 0x00, 0xC8,
			0x00, 0x04, 0x28, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x07, 0x0A, 0x0D, 0x10, 0x11, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x04, 0x03, 0x00, 0x09, 0x11, 0x0F,
			0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x6A, 0x6A, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x1A, 0x12, 0x1B, 0x13, 0x19, 0x16,
			0x18, 0x15, 0x17, 0x11, 0xFF, 0xFF, 0xFF, 0x0C,
			0x01, 0x10, 0x13, 0x11, 0x0D, 0x0A, 0x08, 0x03,
			0x02, 0x06, 0x07, 0x0E, 0x0B, 0x12, 0x05, 0x04,
			0x09, 0x0F, 0xFF, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x19, 0x03,
			0x02, 0x36, 0x3C, 0x01, 0x84, 0x10, 0x14, 0x0A,
			0x82, 0x14, 0x64, 0x03, 0x06, 0x01, 0x18,
		},
	},
	{
		.version = 0x3332,
		.packrat_number = 1514368,
		.abs_x_min = 0,
		.abs_x_max = 810,
		.abs_y_min = 0,
		.abs_y_max = 1440,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		.gpio_reset = MSM_TP_RSTz,
		.i2c_err_handler_en = 1,
		.tw_pin_mask = 0x0080,
		.sensor_id = 0x0000 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.default_config = 1,
		.virtual_key = zara_vk_data,
		.lpm_power = synaptics_power_LPM,
		.config = {
			0x33, 0x32, 0x00, 0x08, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x50,
			0xA5, 0x18, 0x24, 0x03, 0x01, 0x3C, 0x21, 0x01,
			0x21, 0x01, 0xD7, 0x5D, 0xC1, 0x5A, 0x7C, 0xB3,
			0xB4, 0xB4, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xAD, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x11, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF4, 0xC8,
			0xDC, 0x42, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x1E, 0x05, 0x00, 0x02, 0xA4,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x12, 0x3E, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x64, 0x40, 0x60, 0x60,
			0x60, 0x48, 0x48, 0x40, 0x40, 0x40, 0x2A, 0x2A,
			0x29, 0x27, 0x26, 0x25, 0x24, 0x23, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xE8,
			0xFD, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x00, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x5E, 0x61,
			0x63, 0x66, 0x69, 0x6C, 0x6F, 0x39, 0x00, 0xC8,
			0x00, 0x04, 0x28, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x07, 0x0A, 0x0D, 0x10, 0x11, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x04, 0x03, 0x00, 0x09, 0x11, 0x0F,
			0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x6A, 0x6A, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x1A, 0x12, 0x1B, 0x13, 0x19, 0x16,
			0x18, 0x15, 0x17, 0x11, 0xFF, 0xFF, 0xFF, 0x0C,
			0x01, 0x10, 0x13, 0x11, 0x0D, 0x0A, 0x08, 0x03,
			0x02, 0x06, 0x07, 0x0E, 0x0B, 0x12, 0x05, 0x04,
			0x09, 0x0F, 0xFF, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x19, 0x03,
			0x02, 0x36, 0x3C, 0x01, 0x84, 0x10, 0x14, 0x0A,
			0x82, 0x14, 0x64, 0x03, 0x06, 0x01, 0x18,
		},
	},
	{
		.version = 0x3332,
		.packrat_number = 1508458,
		.abs_x_min = 0,
		.abs_x_max = 810,
		.abs_y_min = 0,
		.abs_y_max = 1440,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		.gpio_reset = MSM_TP_RSTz,
		.i2c_err_handler_en = 1,
		.tw_pin_mask = 0x0080,
		.sensor_id = 0x0080 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.default_config = 1,
		.virtual_key = zara_vk_data,
		.lpm_power = synaptics_power_LPM,
		.config = {
			0x33, 0x32, 0x00, 0x04, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x50,
			0xA5, 0x18, 0x24, 0x03, 0x01, 0x3C, 0x21, 0x01,
			0x21, 0x01, 0xD7, 0x5D, 0xC1, 0x5A, 0x7C, 0xB3,
			0xB4, 0xB4, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xAD, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x11, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF4, 0xC8,
			0xDC, 0x42, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x1E, 0x05, 0x00, 0x02, 0xA4,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x12, 0x3E, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x64, 0x40, 0x60, 0x60,
			0x60, 0x48, 0x48, 0x40, 0x40, 0x40, 0x2A, 0x2A,
			0x29, 0x27, 0x26, 0x25, 0x24, 0x23, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xE8,
			0xFD, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x00, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x5E, 0x61,
			0x63, 0x66, 0x69, 0x6C, 0x6F, 0x39, 0x00, 0x8C,
			0x00, 0x04, 0x28, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x07, 0x0A, 0x0D, 0x10, 0x11, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x03, 0x00, 0x09, 0x11, 0x0F,
			0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x6A, 0x6A, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x1A, 0x12, 0x1B, 0x13, 0x19, 0x16,
			0x18, 0x15, 0x17, 0x11, 0xFF, 0xFF, 0xFF, 0x0C,
			0x01, 0x10, 0x13, 0x11, 0x0D, 0x0A, 0x08, 0x03,
			0x02, 0x06, 0x07, 0x0E, 0x0B, 0x12, 0x05, 0x04,
			0x09, 0x0F, 0xFF, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x19, 0x03,
			0x02, 0x36, 0x3C, 0x01, 0x84, 0x10, 0x14, 0x0A,
			0x82, 0x14, 0x3C, 0x03, 0x07,
		},
	},
	{
		.version = 0x3332,
		.packrat_number = 1508458,
		.abs_x_min = 0,
		.abs_x_max = 810,
		.abs_y_min = 0,
		.abs_y_max = 1440,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		.gpio_reset = MSM_TP_RSTz,
		.i2c_err_handler_en = 1,
		.tw_pin_mask = 0x0080,
		.sensor_id = 0x0000 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.default_config = 1,
		.virtual_key = zara_vk_data,
		.lpm_power = synaptics_power_LPM,
		.config = {
			0x33, 0x32, 0x00, 0x04, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x50,
			0xA5, 0x18, 0x24, 0x03, 0x01, 0x3C, 0x21, 0x01,
			0x21, 0x01, 0xD7, 0x5D, 0xC1, 0x5A, 0x7C, 0xB3,
			0xB4, 0xB4, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xAD, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x11, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF4, 0xC8,
			0xDC, 0x42, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x1E, 0x05, 0x00, 0x02, 0xA4,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x12, 0x3E, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x64, 0x40, 0x60, 0x60,
			0x60, 0x48, 0x48, 0x40, 0x40, 0x40, 0x2A, 0x2A,
			0x29, 0x27, 0x26, 0x25, 0x24, 0x23, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xE8,
			0xFD, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x00, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x5E, 0x61,
			0x63, 0x66, 0x69, 0x6C, 0x6F, 0x39, 0x00, 0x8C,
			0x00, 0x04, 0x28, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x07, 0x0A, 0x0D, 0x10, 0x11, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x03, 0x00, 0x09, 0x11, 0x0F,
			0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x6A, 0x6A, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x1A, 0x12, 0x1B, 0x13, 0x19, 0x16,
			0x18, 0x15, 0x17, 0x11, 0xFF, 0xFF, 0xFF, 0x0C,
			0x01, 0x10, 0x13, 0x11, 0x0D, 0x0A, 0x08, 0x03,
			0x02, 0x06, 0x07, 0x0E, 0x0B, 0x12, 0x05, 0x04,
			0x09, 0x0F, 0xFF, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x19, 0x03,
			0x02, 0x36, 0x3C, 0x01, 0x84, 0x10, 0x14, 0x0A,
			0x82, 0x14, 0x3C, 0x03, 0x07,
		},
	},
	{
		.version = 0x3332,
		.packrat_number = 1498736,
		.abs_x_min = 0,
		.abs_x_max = 810,
		.abs_y_min = 0,
		.abs_y_max = 1440,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		.gpio_reset = MSM_TP_RSTz,
		.i2c_err_handler_en = 1,
		.tw_pin_mask = 0x0080,
		.sensor_id = 0x0080 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.default_config = 1,
		.virtual_key = zara_vk_data,
		.lpm_power = synaptics_power_LPM,
		.config = {
			0x33, 0x32, 0x00, 0x03, 0x60, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x50,
			0xA5, 0x18, 0x24, 0x03, 0x01, 0x3C, 0x21, 0x01,
			0x21, 0x01, 0xD7, 0x5D, 0xC1, 0x5A, 0x7C, 0xB3,
			0xB4, 0xB4, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x11, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF4, 0xC8,
			0xDC, 0x42, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x1E, 0x05, 0x00, 0x02, 0xA4,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x12, 0x3E, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x10, 0x0A, 0x60, 0x60,
			0x60, 0x48, 0x48, 0x40, 0x40, 0x40, 0x2A, 0x2A,
			0x29, 0x27, 0x26, 0x25, 0x24, 0x23, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xE8,
			0xFD, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x00, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x5E, 0x61,
			0x63, 0x66, 0x69, 0x6C, 0x6F, 0x39, 0x00, 0x8C,
			0x00, 0x04, 0x04, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x07, 0x0A, 0x0D, 0x10, 0x11, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x03, 0x00, 0x09, 0x11, 0x0F,
			0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x6A, 0x6A, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x1A, 0x12, 0x1B, 0x13, 0x19, 0x16,
			0x18, 0x15, 0x17, 0x11, 0xFF, 0xFF, 0xFF, 0x0C,
			0x01, 0x10, 0x13, 0x11, 0x0D, 0x0A, 0x08, 0x03,
			0x02, 0x06, 0x07, 0x0E, 0x0B, 0x12, 0x05, 0x04,
			0x09, 0x0F, 0xFF, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x19, 0x01,
			0x02, 0x36, 0x44, 0x08, 0x84, 0x10, 0x14, 0x0A,
			0x82, 0x14, 0x3C, 0x03, 0x07,
		},
	},
	{
		.version = 0x3332,
		.packrat_number = 1498736,
		.abs_x_min = 0,
		.abs_x_max = 810,
		.abs_y_min = 0,
		.abs_y_max = 1440,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		.gpio_reset = MSM_TP_RSTz,
		.i2c_err_handler_en = 1,
		.tw_pin_mask = 0x0080,
		.sensor_id = 0x0000 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.default_config = 1,
		.virtual_key = zara_vk_data,
		.lpm_power = synaptics_power_LPM,
		.config = {
			0x33, 0x32, 0x00, 0x03, 0x60, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x50,
			0xA5, 0x18, 0x24, 0x03, 0x01, 0x3C, 0x21, 0x01,
			0x21, 0x01, 0xD7, 0x5D, 0xC1, 0x5A, 0x7C, 0xB3,
			0xB4, 0xB4, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x11, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF4, 0xC8,
			0xDC, 0x42, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x1E, 0x05, 0x00, 0x02, 0xA4,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x12, 0x3E, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x10, 0x0A, 0x60, 0x60,
			0x60, 0x48, 0x48, 0x40, 0x40, 0x40, 0x2A, 0x2A,
			0x29, 0x27, 0x26, 0x25, 0x24, 0x23, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xE8,
			0xFD, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x00, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x5E, 0x61,
			0x63, 0x66, 0x69, 0x6C, 0x6F, 0x39, 0x00, 0x8C,
			0x00, 0x04, 0x04, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x07, 0x0A, 0x0D, 0x10, 0x11, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x03, 0x00, 0x09, 0x11, 0x0F,
			0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x6A, 0x6A, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x1A, 0x12, 0x1B, 0x13, 0x19, 0x16,
			0x18, 0x15, 0x17, 0x11, 0xFF, 0xFF, 0xFF, 0x0C,
			0x01, 0x10, 0x13, 0x11, 0x0D, 0x0A, 0x08, 0x03,
			0x02, 0x06, 0x07, 0x0E, 0x0B, 0x12, 0x05, 0x04,
			0x09, 0x0F, 0xFF, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x19, 0x01,
			0x02, 0x36, 0x44, 0x08, 0x84, 0x10, 0x14, 0x0A,
			0x82, 0x14, 0x3C, 0x03, 0x07,
		},
	},
	{
		.version = 0x3332,
		.packrat_number = 1473052,
		.abs_x_min = 0,
		.abs_x_max = 810,
		.abs_y_min = 0,
		.abs_y_max = 1440,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		.gpio_reset = MSM_TP_RSTz,
		.i2c_err_handler_en = 1,
		.tw_pin_mask = 0x0080,
		.sensor_id = 0x0080 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.default_config = 1,
		.virtual_key = zara_vk_data,
		.lpm_power = synaptics_power_LPM,
		.config = {
			0x33, 0x32, 0x00, 0x02, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x50,
			0xA5, 0x18, 0x24, 0x03, 0x01, 0x3C, 0x21, 0x01,
			0x21, 0x01, 0xD7, 0x5D, 0xC1, 0x5A, 0x7C, 0xB3,
			0xB4, 0xB4, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x11, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF4, 0xC8,
			0xDC, 0x42, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x1E, 0x05, 0x00, 0x02, 0xA4,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x13, 0x3E, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x10, 0x0A, 0x60, 0x60,
			0x60, 0x48, 0x48, 0x40, 0x40, 0x40, 0x2A, 0x2A,
			0x29, 0x27, 0x26, 0x25, 0x24, 0x23, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xE8,
			0xFD, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x00, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x5E, 0x61,
			0x63, 0x66, 0x69, 0x6C, 0x6F, 0x39, 0x00, 0x8C,
			0x00, 0x10, 0x28, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x07, 0x0A, 0x0D, 0x10, 0x11, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x03, 0x00, 0x09, 0x11, 0x0F,
			0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x51, 0x51, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x1A, 0x12, 0x1B, 0x13, 0x19, 0x16,
			0x18, 0x15, 0x17, 0x11, 0xFF, 0xFF, 0xFF, 0x0C,
			0x01, 0x10, 0x13, 0x11, 0x0D, 0x0A, 0x08, 0x03,
			0x02, 0x06, 0x07, 0x0E, 0x0B, 0x12, 0x05, 0x04,
			0x09, 0x0F, 0xFF, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x19, 0x01,
			0x02, 0x36, 0x44, 0x08, 0x84, 0x10, 0x14, 0x0A,
			0x78, 0x14, 0x34, 0x03, 0x08,
		},
	},
	{
		.version = 0x3332,
		.packrat_number = 1473052,
		.abs_x_min = 0,
		.abs_x_max = 810,
		.abs_y_min = 0,
		.abs_y_max = 1440,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		.gpio_reset = MSM_TP_RSTz,
		.i2c_err_handler_en = 1,
		.tw_pin_mask = 0x0080,
		.sensor_id = 0x0000 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.default_config = 1,
		.virtual_key = zara_vk_data,
		.lpm_power = synaptics_power_LPM,
		.config = {
			0x33, 0x32, 0x00, 0x02, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x09, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x50,
			0xA5, 0x18, 0x24, 0x03, 0x01, 0x3C, 0x21, 0x01,
			0x21, 0x01, 0xD7, 0x5D, 0xC1, 0x5A, 0x7C, 0xB3,
			0xB4, 0xB4, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x00,
			0x09, 0x04, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x11, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x78, 0x07, 0xF4, 0xC8,
			0xDC, 0x42, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x1E, 0x05, 0x00, 0x02, 0xA4,
			0x01, 0x80, 0x03, 0x0E, 0x1F, 0x13, 0x3E, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x10, 0x0A, 0x60, 0x60,
			0x60, 0x48, 0x48, 0x40, 0x40, 0x40, 0x2A, 0x2A,
			0x29, 0x27, 0x26, 0x25, 0x24, 0x23, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xE8,
			0xFD, 0x00, 0x64, 0x00, 0xC8, 0x00, 0x80, 0x0A,
			0x00, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x20, 0x10, 0x5E, 0x61,
			0x63, 0x66, 0x69, 0x6C, 0x6F, 0x39, 0x00, 0x8C,
			0x00, 0x10, 0x28, 0x00, 0x00, 0x00, 0x02, 0x04,
			0x07, 0x0A, 0x0D, 0x10, 0x11, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x03, 0x00, 0x09, 0x11, 0x0F,
			0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x51, 0x51, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x1A, 0x12, 0x1B, 0x13, 0x19, 0x16,
			0x18, 0x15, 0x17, 0x11, 0xFF, 0xFF, 0xFF, 0x0C,
			0x01, 0x10, 0x13, 0x11, 0x0D, 0x0A, 0x08, 0x03,
			0x02, 0x06, 0x07, 0x0E, 0x0B, 0x12, 0x05, 0x04,
			0x09, 0x0F, 0xFF, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x19, 0x01,
			0x02, 0x36, 0x44, 0x08, 0x84, 0x10, 0x14, 0x0A,
			0x78, 0x14, 0x34, 0x03, 0x08,
		},
	},
	{
		.version = 0x3332,
		.packrat_number = 1293984,
		.abs_x_min = 0,
		.abs_x_max = 810,
		.abs_y_min = 0,
		.abs_y_max = 1440,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		.gpio_reset = MSM_TP_RSTz,
		.i2c_err_handler_en = 1,
		.tw_pin_mask = 0x0080,
		.sensor_id = 0x0080 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.default_config = 1,
		.virtual_key = zara_vk_data,
		.lpm_power = synaptics_power_LPM,
		.config = {
			0x33, 0x32, 0xEE, 0x00, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x08, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x28,
			0xF5, 0x28, 0x1E, 0x05, 0x01, 0x3C, 0x30, 0x00,
			0x30, 0x00, 0xD7, 0x5D, 0xC1, 0x5A, 0x22, 0xD4,
			0x80, 0x8C, 0x01, 0x70, 0x00, 0x00, 0x00, 0x00,
			0x0A, 0x04, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x11, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x64, 0x07, 0x66, 0x64,
			0xC0, 0x43, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x1E, 0x05, 0x00, 0x02, 0x9B,
			0x01, 0x80, 0x02, 0x0E, 0x1F, 0x13, 0x35, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x10, 0x28, 0x68, 0x60,
			0x60, 0x60, 0x48, 0x40, 0x40, 0x40, 0x2E, 0x2D,
			0x2B, 0x29, 0x27, 0x26, 0x24, 0x22, 0x00, 0x00,
			0x00, 0x00, 0x03, 0x06, 0x0A, 0x0E, 0x00, 0x88,
			0x13, 0x00, 0x64, 0x00, 0xC8, 0x00, 0xCD, 0x14,
			0xC0, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x10, 0x10, 0x55, 0x5A,
			0x5E, 0x62, 0x66, 0x6B, 0x38, 0x3B, 0x00, 0x78,
			0x00, 0x10, 0x28, 0x18, 0x00, 0x00, 0x04, 0x08,
			0x0C, 0x0D, 0x0F, 0x10, 0x12, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x03, 0x00, 0x09, 0x11, 0x0F,
			0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB9, 0xB9, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x1A, 0x12, 0x1B, 0x13, 0x19, 0x16,
			0x18, 0x15, 0x17, 0x11, 0xFF, 0xFF, 0xFF, 0x0C,
			0x01, 0x10, 0x13, 0x11, 0x0D, 0x0A, 0x08, 0x03,
			0x02, 0x06, 0x07, 0x0E, 0x0B, 0x12, 0x05, 0x04,
			0x09, 0x0F, 0xFF, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x0F, 0x01,
			0x4F, 0x53,
		},
	},
	{
		.version = 0x3332,
		.packrat_number = 1293984,
		.abs_x_min = 0,
		.abs_x_max = 810,
		.abs_y_min = 0,
		.abs_y_max = 1440,
		.display_width = 540,
		.display_height = 960,
		.gpio_irq = MSM_TP_ATTz,
		.gpio_reset = MSM_TP_RSTz,
		.i2c_err_handler_en = 1,
		.tw_pin_mask = 0x0080,
		.sensor_id = 0x0000 | SENSOR_ID_CHECKING_EN,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.psensor_detection = 1,
		.default_config = 1,
		.virtual_key = zara_vk_data,
		.lpm_power = synaptics_power_LPM,
		.config = {
			0x33, 0x32, 0xEE, 0x00, 0x00, 0x7F, 0x03, 0x1E,
			0x05, 0x08, 0x00, 0x01, 0x01, 0x00, 0x10, 0x2A,
			0x03, 0xA0, 0x05, 0x02, 0x14, 0x1E, 0x05, 0x28,
			0xF5, 0x28, 0x1E, 0x05, 0x01, 0x3C, 0x30, 0x00,
			0x30, 0x00, 0xD7, 0x5D, 0xC1, 0x5A, 0x22, 0xD4,
			0x80, 0x8C, 0x01, 0x70, 0x00, 0x00, 0x00, 0x00,
			0x0A, 0x04, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x19, 0x01, 0x00, 0x0A, 0x0A, 0x11, 0x0A,
			0x00, 0x14, 0x0A, 0x40, 0x64, 0x07, 0x66, 0x64,
			0xC0, 0x43, 0x2A, 0x05, 0x00, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x3C, 0x32, 0x00, 0x00, 0x00,
			0x4C, 0x6C, 0x74, 0x1E, 0x05, 0x00, 0x02, 0x9B,
			0x01, 0x80, 0x02, 0x0E, 0x1F, 0x13, 0x35, 0x00,
			0x13, 0x04, 0x1B, 0x00, 0x10, 0x28, 0x68, 0x60,
			0x60, 0x60, 0x48, 0x40, 0x40, 0x40, 0x2E, 0x2D,
			0x2B, 0x29, 0x27, 0x26, 0x24, 0x22, 0x00, 0x00,
			0x00, 0x00, 0x03, 0x06, 0x0A, 0x0E, 0x00, 0x88,
			0x13, 0x00, 0x64, 0x00, 0xC8, 0x00, 0xCD, 0x14,
			0xC0, 0xB8, 0x0B, 0x00, 0xC0, 0x80, 0x02, 0x02,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x20, 0x20,
			0x20, 0x20, 0x20, 0x20, 0x10, 0x10, 0x55, 0x5A,
			0x5E, 0x62, 0x66, 0x6B, 0x38, 0x3B, 0x00, 0x78,
			0x00, 0x10, 0x28, 0x18, 0x00, 0x00, 0x04, 0x08,
			0x0C, 0x0D, 0x0F, 0x10, 0x12, 0x00, 0x31, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x03, 0x00, 0x09, 0x11, 0x0F,
			0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xB9, 0xB9, 0x51,
			0x51, 0x51, 0x51, 0x51, 0x51, 0xCD, 0x0D, 0x04,
			0x01, 0x14, 0x1A, 0x12, 0x1B, 0x13, 0x19, 0x16,
			0x18, 0x15, 0x17, 0x11, 0xFF, 0xFF, 0xFF, 0x0C,
			0x01, 0x10, 0x13, 0x11, 0x0D, 0x0A, 0x08, 0x03,
			0x02, 0x06, 0x07, 0x0E, 0x0B, 0x12, 0x05, 0x04,
			0x09, 0x0F, 0xFF, 0x00, 0x10, 0x00, 0x10, 0x00,
			0x10, 0x00, 0x10, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
			0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x0F, 0x01,
			0x4F, 0x53,
		},
	},
	{
		.version = 0x3330,
		.packrat_number = 7788,
		.abs_x_min = 0,
		.abs_x_max = 1100,
		.abs_y_min = 0,
		.abs_y_max = 1770,
		.display_width = 544,
		.display_height = 960,
		.flags = SYNAPTICS_FLIP_X,
		.gpio_irq = MSM_TP_ATTz,
		.gpio_reset = MSM_TP_RSTz,
		.i2c_err_handler_en = 1,
		.large_obj_check = 1,
		.tw_pin_mask = 0x0080,
		.report_type = SYN_AND_REPORT_TYPE_B,
		.segmentation_bef_unlock = 0x50,
		.power = synaptic_rmi_tp_power,
	},
};

static struct i2c_board_info msm_i2c_syn_gsbi3_info[] __initdata = {
	{
		I2C_BOARD_INFO(SYNAPTICS_3200_NAME, 0x40 >> 1),
		.platform_data = &syn_ts_3k_data,
		.irq = MSM_GPIO_TO_INT(MSM_TP_ATTz)
	},
};

static void syn_init_vkeys_zara(void)
{
	int rc = 0;
	static struct kobject *syn_properties_kobj;

	syn_properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (syn_properties_kobj)
		rc = sysfs_create_group(syn_properties_kobj, &syn_properties_attr_group);
	if (!syn_properties_kobj || rc)
		pr_err("%s: failed to create board_properties\n", __func__);
	else {
		for (rc = 0; rc < ARRAY_SIZE(syn_ts_3k_data); rc++) {
			syn_ts_3k_data[rc].vk_obj = syn_properties_kobj;
			syn_ts_3k_data[rc].vk2Use = &syn_vkeys_attr;
		}
	}

	return;
}

static struct htc_headset_gpio_platform_data htc_headset_gpio_data = {
	.hpin_gpio		= MSM_EARPHONE_DETz,
	.key_enable_gpio	= 0,
	.mic_select_gpio	= 0,
};

static struct platform_device htc_headset_gpio = {
	.name	= "HTC_HEADSET_GPIO",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_gpio_data,
	},
};

static struct htc_headset_pmic_platform_data htc_headset_pmic_data = {
	.driver_flag		= DRIVER_HS_PMIC_ADC,
	.hpin_gpio		= 0,
	.hpin_irq		= 0,
	.key_gpio		= MSM_MDM_RX,
	.key_irq		= 0,
	.key_enable_gpio	= 0,
	.adc_mic		= 0,
	.adc_remote		= {0, 56, 57, 146, 147, 339},
	.adc_mpp		= PM8XXX_AMUX_MPP_4,
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

static uint32_t headset_onewire_gpio[] = {
	GPIO_CFG(MSM_MDM_RX, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(MSM_MDM_TX, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(MSM_MDM_RX, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(MSM_MDM_TX, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static void uart_tx_gpo(int mode)
{
	switch (mode) {
		case 0:
			gpio_tlmm_config(headset_onewire_gpio[1], GPIO_CFG_ENABLE);
			gpio_set_value_cansleep(MSM_MDM_TX, 0);
			break;
		case 1:
			gpio_tlmm_config(headset_onewire_gpio[1], GPIO_CFG_ENABLE);
			gpio_set_value_cansleep(MSM_MDM_TX, 1);
			break;
		case 2:
			gpio_tlmm_config(headset_onewire_gpio[3], GPIO_CFG_ENABLE);
			break;
	}
}

static void uart_lv_shift_en(int enable)
{
	gpio_set_value_cansleep(MSM_LEVEL_SHIFTER_EN, enable);
}

static struct htc_headset_1wire_platform_data htc_headset_1wire_data = {
	.tx_level_shift_en	= MSM_LEVEL_SHIFTER_EN,
	.uart_sw		= 0,
	.one_wire_remote	={0x7E, 0x7F, 0x7D, 0x7F, 0x7B, 0x7F},
	.remote_press		= 0,
	.onewire_tty_dev	= "/dev/ttyHSL1",
};

static struct platform_device htc_headset_one_wire = {
	.name	= "HTC_HEADSET_1WIRE",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_1wire_data,
	},
};

static struct platform_device *headset_devices[] = {
	&htc_headset_gpio,
	&htc_headset_pmic,
	&htc_headset_one_wire,
};

static struct headset_adc_config htc_headset_mgr_config[] = {
	{
		.type = HEADSET_MIC,
		.adc_max = 1557,
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
		.type = HEADSET_MIC,
		.adc_max = 565,
		.adc_min = 255,
	},
	{
		.type = HEADSET_NO_MIC,
		.adc_max = 254,
		.adc_min = 0,
	},
};

static uint32_t headset_cpu_gpio[] = {
	GPIO_CFG(MSM_V_AUD_HSMIC_2V85_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(MSM_LEVEL_SHIFTER_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(MSM_AUD_REMO_PRESz, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static void headset_init(void)
{
	int i, ret;
	for (i = 0; i < ARRAY_SIZE(headset_cpu_gpio); i++) {
		gpio_tlmm_config(headset_cpu_gpio[i], GPIO_CFG_ENABLE);
	}
	ret = gpio_request(MSM_V_AUD_HSMIC_2V85_EN, "headset_mic_bias");
	if (ret) {
		pr_err("[HS_BOARD]gpio_request for %d gpio failed rc(%d)\n", MSM_V_AUD_HSMIC_2V85_EN, ret);
		return;
	}

	ret = gpio_direction_output(MSM_V_AUD_HSMIC_2V85_EN, 0);
	if (ret) {
		pr_err("[HS_BOARD]gpio_direction_output failed for %d gpio rc(%d)\n",MSM_V_AUD_HSMIC_2V85_EN, ret);
		return;
	}
	gpio_free(MSM_V_AUD_HSMIC_2V85_EN);

	ret = gpio_request(MSM_LEVEL_SHIFTER_EN, "1wire_level_en");
	if (ret) {
		pr_err("[HS_BOARD]gpio_request for %d gpio failed rc(%d)\n", MSM_LEVEL_SHIFTER_EN, ret);
		return;
	}

	ret = gpio_direction_output(MSM_LEVEL_SHIFTER_EN, 1);
	if (ret) {
		pr_err("[HS_BOARD]gpio_direction_output failed for %d gpio rc(%d)\n", MSM_LEVEL_SHIFTER_EN, ret);
		return;
	}
	gpio_free(MSM_LEVEL_SHIFTER_EN);
}

static void headset_power(int enable)
{
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(headset_cpu_gpio); i++) {
		gpio_tlmm_config(headset_cpu_gpio[i], GPIO_CFG_ENABLE);
	}

	pr_info("[HS_BOARD]%s mic bias\n", enable ? "Enable" : "Disable");
	ret = gpio_request(MSM_V_AUD_HSMIC_2V85_EN, "headset_mic_bias");
	if (ret) {
		pr_err("[HS_BOARD]gpio_request for %d gpio failed rc(%d)\n", MSM_V_AUD_HSMIC_2V85_EN, ret);
		return;
	}

	ret = gpio_direction_output(MSM_V_AUD_HSMIC_2V85_EN, enable);
	if (ret) {
		pr_err("[HS_BOARD]gpio_direction_output failed for %d gpio rc(%d)\n",MSM_V_AUD_HSMIC_2V85_EN, ret);
		return;
	}
	gpio_free(MSM_V_AUD_HSMIC_2V85_EN);

	ret = gpio_request(MSM_LEVEL_SHIFTER_EN, "1wire_level_en");
	if (ret) {
		pr_err("[HS_BOARD]gpio_request for %d gpio failed rc(%d)\n", MSM_LEVEL_SHIFTER_EN, ret);
		return;
	}

	ret = gpio_direction_output(MSM_LEVEL_SHIFTER_EN, 1);
	if (ret) {
		pr_err("[HS_BOARD]gpio_direction_output failed for %d gpio rc(%d)\n", MSM_LEVEL_SHIFTER_EN, ret);
		return;
	}
	gpio_free(MSM_LEVEL_SHIFTER_EN);
}

static struct htc_headset_mgr_platform_data htc_headset_mgr_data = {
	.driver_flag		= DRIVER_HS_MGR_FLOAT_DET,
	.headset_devices_num	= ARRAY_SIZE(headset_devices),
	.headset_devices	= headset_devices,
	.headset_config_num	= ARRAY_SIZE(htc_headset_mgr_config),
	.headset_config		= htc_headset_mgr_config,
	.headset_init		= headset_init,
	.headset_power		= headset_power,
	.uart_tx_gpo		= uart_tx_gpo,
	.uart_lv_shift_en	= uart_lv_shift_en,
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
	pr_info("[HS_BOARD] (%s) hw_rev %d\n", __func__, system_rev);
	platform_device_register(&htc_headset_mgr);
}

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi2_pdata = {
	.clk_freq = 384000,
	.src_clk_rate = 24000000,
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi4_pdata = {
	.clk_freq = 384000,
	.src_clk_rate = 24000000,
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi3_pdata = {
	.clk_freq = 384000,
	.src_clk_rate = 24000000,
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi9_pdata = {
	.clk_freq = 384000,
	.src_clk_rate = 24000000,
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi12_pdata = {
	.clk_freq = 384000,
	.src_clk_rate = 24000000,
};

#if defined(CONFIG_MSM_CAMERA) && defined(CONFIG_RAWCHIP)
static struct spi_board_info rawchip_spi_board_info[] __initdata = {
	{
		.modalias               = "spi_rawchip",
		.max_speed_hz           = 27000000,
		.bus_num                = 1,
		.chip_select            = 0,
		.mode                   = SPI_MODE_0,
	},
};
#endif
static struct platform_device msm_device_saw_core0 = {
	.name	= "saw-regulator",
	.id	= 0,
	.dev	= {
		.platform_data = &msm8930_saw_regulator_core0_pdata,
	},
};

static struct platform_device msm_device_saw_core1 = {
	.name	= "saw-regulator",
	.id	= 1,
	.dev	= {
		.platform_data = &msm8930_saw_regulator_core1_pdata,
	},
};

static struct tsens_platform_data msm_tsens_pdata  = {
	.tsens_factor		= 1000,
	.hw_type		= APQ_8064,
	.tsens_num_sensor	= 10,
	.slope = {1132, 1135, 1137, 1135, 1157,
			1142, 1124, 1153, 1175, 1166},
};

static struct platform_device msm_tsens_device = {
	.name   = "tsens8960-tm",
	.id = -1,
};

static struct msm_thermal_data msm_thermal_pdata = {
	.sensor_id = 9,
	.poll_ms = 1000,
	.limit_temp_degC = 60,
	.temp_hysteresis_degC = 10,
};

static struct platform_device msm8930_device_rpm_regulator __devinitdata = {
	.name	= "rpm-regulator",
	.id	= -1,
	.dev	= {
		.platform_data = &msm8930_rpm_regulator_pdata,
	},
};

static struct msm_serial_hs_platform_data msm_uart_dm6_pdata = {
	.inject_rx_on_wakeup = 0,
};

static struct platform_device zara_rfkill = {
	.name = "zara_rfkill",
	.id = -1,
};

static struct platform_device *common_devices[] __initdata = {
	&msm8960_device_dmov,
	&msm8930_device_qup_spi_gsbi10,
	&msm8960_device_qup_i2c_gsbi3,
	&msm8960_device_qup_i2c_gsbi4,
	&msm8960_device_qup_i2c_gsbi2,
	&msm8960_device_qup_i2c_gsbi9,
	&msm8960_device_qup_i2c_gsbi12,
#ifdef CONFIG_BT
	&msm_device_uart_dm6,
	&zara_rfkill,
#endif
	&msm_device_smd,
	&msm8960_device_uart_gsbi8,
	&msm8960_device_ssbi_pmic,
	&msm8960_device_otg,
	&msm8960_device_gadget_peripheral,
	&msm_device_hsusb_host,
	&android_usb_device,
	&msm_8960_q6_lpass,
	&msm_8960_q6_mss_fw,
	&msm_8960_q6_mss_sw,
	&msm_pil_tzapps,
	&msm_pil_vidc,
	&msm_slim_ctrl,
#if defined(CONFIG_QSEECOM)
	&qseecom_device,
#endif

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)
	&qcrypto_device,
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)
	&qcedev_device,
#endif
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	&msm8930_android_pmem_device,
	&msm8930_android_pmem_adsp_device,
	&msm8930_android_pmem_audio_device,
#endif /*CONFIG_MSM_MULTIMEDIA_USE_ION*/
#endif /*CONFIG_ANDROID_PMEM*/
#ifdef CONFIG_ION_MSM
	&msm8930_ion_dev,
#endif
	&msm8960_device_watchdog,
	&msm_device_saw_core0,
	&msm_device_saw_core1,
	&msm8930_fmem_device,
	&msm_device_bam_dmux,
	&msm_fm_platform_init,
#ifdef CONFIG_HW_RANDOM_MSM
	&msm_device_rng,
#endif
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
	&msm_voice,
	&msm_voip,
	&msm_lpa_pcm,
	&msm_lowlatency_pcm,
	&msm_pcm_hostless,
	&msm_cpudai_afe_01_rx,
	&msm_cpudai_afe_01_tx,
	&msm_cpudai_afe_02_rx,
	&msm_cpudai_afe_02_tx,
	&msm_pcm_afe,
	&apq_cpudai_pri_i2s_rx,
	&apq_cpudai_pri_i2s_tx,
	&msm_compr_dsp,
	&msm_multi_ch_pcm,
	&msm_cpudai_incall_music_rx,
	&msm_cpudai_incall_record_rx,
	&msm_cpudai_incall_record_tx,
	&msm8930_rpm_device,
	&msm8930_rpm_log_device,
	&msm8930_rpm_rbcpr_device,
	&msm8930_rpm_stat_device,
	&msm8930_rpm_master_stat_device,
	&msm_device_tz_log,
	&msm_device_dspcrashd_8960,
	&msm8930_rtb_device,
	&msm_bus_8930_apps_fabric,
	&msm_bus_8930_sys_fabric,
	&msm_bus_8930_mm_fabric,
	&msm_bus_8930_sys_fpb,
	&msm_bus_8930_cpss_fpb,
	&msm8960_device_cache_erp,
#ifdef CONFIG_MSM_GEMINI
	&msm8960_gemini_device,
#endif
	&msm8930_iommu_domain_device,
	&msm_tsens_device,
#ifdef CONFIG_HTC_BATT_8960
	&htc_battery_pdev,
#endif
#if defined(CONFIG_MSM_CAMERA) && defined(CONFIG_RAWCHIP)
	&zara_msm_rawchip_device,
#endif
};

static struct platform_device *cdp_devices[] __initdata = {
	&msm8960_device_uart_gsbi3,
	&msm_cpudai_mi2s,
	&msm_device_sps,
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
        &msm8930_cpu_slp_status,
};

static uint32_t msm_uart_gpio[] = {
	GPIO_CFG(MSM_UART_TX, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(MSM_UART_RX, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
};

static void msm_uart_gsbi_gpio_init(void)
{
	gpio_tlmm_config(msm_uart_gpio[0], GPIO_CFG_ENABLE);
	gpio_tlmm_config(msm_uart_gpio[1], GPIO_CFG_ENABLE);
}

static void __init zara_i2c_init(void)
{
	msm8960_device_qup_i2c_gsbi2.dev.platform_data =
					&msm8960_i2c_qup_gsbi2_pdata;

	msm8960_device_qup_i2c_gsbi4.dev.platform_data =
					&msm8960_i2c_qup_gsbi4_pdata;

	msm8960_device_qup_i2c_gsbi3.dev.platform_data =
					&msm8960_i2c_qup_gsbi3_pdata;

	msm8960_device_qup_i2c_gsbi9.dev.platform_data =
					&msm8960_i2c_qup_gsbi9_pdata;

	msm8960_device_qup_i2c_gsbi12.dev.platform_data =
					&msm8960_i2c_qup_gsbi12_pdata;
}

static struct msm_rpmrs_level msm_rpmrs_levels[] __initdata = {
	{
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		1, 784, 180000, 100,
	},
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
		[MSM_RPMRS_VDD_DIG_RET_LOW]	= 0,
		[MSM_RPMRS_VDD_DIG_RET_HIGH]	= 0,
		[MSM_RPMRS_VDD_DIG_ACTIVE]	= 1,
		[MSM_RPMRS_VDD_DIG_MAX]		= 3,
	},
	.vdd_mask = 0x7FFFFF,
	.rpmrs_target_id = {
		[MSM_RPMRS_ID_PXO_CLK]		= MSM_RPM_ID_PXO_CLK,
		[MSM_RPMRS_ID_L2_CACHE_CTL]	= MSM_RPM_ID_LAST,
		[MSM_RPMRS_ID_VDD_DIG_0]	= MSM_RPM_ID_VOLTAGE_CORNER,
		[MSM_RPMRS_ID_VDD_DIG_1]	= MSM_RPM_ID_LAST,
		[MSM_RPMRS_ID_VDD_MEM_0]	= MSM_RPM_ID_PM8038_L24_0,
		[MSM_RPMRS_ID_VDD_MEM_1]	= MSM_RPM_ID_PM8038_L24_1,
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
#define I2C_LIQUID (1 << 5)

struct i2c_registry {
	u8                     machs;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

#ifdef CONFIG_ISL9519_CHARGER
static struct isl_platform_data isl_data __initdata = {
	.valid_n_gpio		= 0,
	.chg_detection_config	= NULL,
	.max_system_voltage	= 4200,
	.min_system_voltage	= 3200,
	.chgcurrent		= 1000,
	.term_current		= 400,
	.input_current		= 2048,
};

static struct i2c_board_info isl_charger_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("isl9519q", 0x9),
		.irq		= 0,
		.platform_data	= &isl_data,
	},
};
#endif

 #define ZARA_LAYOUTS		{\
		{ { 0,  1, 0}, { 1,  0,  0}, {0, 0, -1} }, \
		{ { 0, -1, 0}, {-1,  0,  0}, {0, 0,  1} }, \
		{ { 1,  0, 0}, { 0, -1,  0}, {0, 0, -1} }, \
		{ {-1,  0, 0}, { 0,  0, -1}, {0, 1,  0} }  \
		}


static struct regulator *g_sensor_reg_l9;

static int g_sensor_power_LPM(int on)
{
	int rc = 0;

	mutex_lock(&sensor_lock);

	printk(KERN_DEBUG "[GSNR][BMA250_WO_COMP] %s, on = %d\n", __func__, on);

	if (g_sensor_reg_l9 == NULL) {
		g_sensor_reg_l9 = regulator_get(NULL, "8038_l9_g_sensor");
		if (IS_ERR(g_sensor_reg_l9)) {
			pr_err("[GSNR][BMA250_WO_COMP] %s: Unable to get"
			" '8038_l9_g_sensor' \n", __func__);
			mutex_unlock(&sensor_lock);
			pr_info("[GSNR][BMA250_WO_COMP] %s: sensor_lock unlock 1\n", __func__);
			return -ENODEV;
		}
	}

	if (on) {
		rc = regulator_set_optimum_mode(g_sensor_reg_l9, 100);
		if (rc < 0) {
			pr_err("[GSNR][BMA250_WO_COMP] set_optimum_mode L9 to LPM"
				" failed, rc = %d\n", rc);
			mutex_unlock(&sensor_lock);
			pr_info("[GSNR][BMA250_WO_COMP] %s: sensor_lock unlock 2\n", __func__);
			return -EINVAL;
		}
		printk(KERN_DEBUG "[GSNR][BMA250_WO_COMP] %s, Set to Low Power"
			" Mode\n", __func__);
	} else {
		rc = regulator_set_optimum_mode(g_sensor_reg_l9, 100000);
		if (rc < 0) {
			pr_err("[GSNR][BMA250_WO_COMP] set_optimum_mode L9 to"
				" Normal mode failed, rc = %d\n", rc);
			mutex_unlock(&sensor_lock);
			pr_info("[GSNR][BMA250_WO_COMP] %s: sensor_lock unlock 3\n", __func__);
			return -EINVAL;
		}
		printk(KERN_DEBUG "[GSNR][BMA250_WO_COMP] %s, Set to Normal Mode\n",
			__func__);
	}
	mutex_unlock(&sensor_lock);
	pr_info("[GSNR][BMA250_WO_COMP] %s: sensor_lock unlock 4\n", __func__);
	return 0;
}

static struct bma250_platform_data gsensor_bma250_platform_data = {
	.intr = MSM_GSENSOR_INT,
	.chip_layout = 1,
	.layouts = ZARA_LAYOUTS,
	.power_LPM = g_sensor_power_LPM,
};

static struct i2c_board_info msm_i2c_gsbi12_info[] = {
	{
		I2C_BOARD_INFO(BMA250_I2C_NAME_REMOVE_ECOMPASS, 0x30 >> 1),
		.platform_data = &gsensor_bma250_platform_data,
		.irq = MSM_GPIO_TO_INT(MSM_GSENSOR_INT),
	},
};

static struct i2c_registry msm8960_i2c_devices[] __initdata = {
#ifdef CONFIG_REGULATOR_NCP6924
	{
		I2C_SURF | I2C_FFA,
		MSM_8930_GSBI12_QUP_I2C_BUS_ID,
		ncp6924_info,
		ARRAY_SIZE(ncp6924_info),
	},
#endif
#ifdef CONFIG_ISL9519_CHARGER
	{
		I2C_LIQUID,
		MSM_8930_GSBI10_QUP_I2C_BUS_ID,
		isl_charger_i2c_info,
		ARRAY_SIZE(isl_charger_i2c_info),
	},
#endif
#ifdef CONFIG_SENSORS_NFC_PN544
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_8930_GSBI9_QUP_I2C_BUS_ID,
		pn544_i2c_boardinfo,
		ARRAY_SIZE(pn544_i2c_boardinfo),
	},
#endif
	{
		I2C_SURF | I2C_FFA | I2C_FLUID,
		MSM_8930_GSBI3_QUP_I2C_BUS_ID,
		msm_i2c_syn_gsbi3_info,
		ARRAY_SIZE(msm_i2c_syn_gsbi3_info),
	},
#ifdef CONFIG_FLASHLIGHT_TPS61310
	{
		I2C_SURF | I2C_FFA,
		MSM_8930_GSBI12_QUP_I2C_BUS_ID,
		i2c_tps61310_flashlight,
		ARRAY_SIZE(i2c_tps61310_flashlight),
	},
#endif
	{
		I2C_SURF | I2C_FFA,
		MSM_8930_GSBI12_QUP_I2C_BUS_ID,
		msm_i2c_gsbi12_info,
		ARRAY_SIZE(msm_i2c_gsbi12_info),
	},
	{
		I2C_SURF | I2C_FFA,
		MSM_8930_GSBI12_QUP_I2C_BUS_ID,
		i2c_CM36282_devices,
		ARRAY_SIZE(i2c_CM36282_devices),
	},
#ifdef CONFIG_AMP_RT5501
	{
		I2C_SURF | I2C_FFA,
		MSM_8930_GSBI2_QUP_I2C_BUS_ID,
		msm_i2c_gsbi2_rt5501_info,
		ARRAY_SIZE(msm_i2c_gsbi2_rt5501_info),
	},
#endif
#ifdef CONFIG_AMP_TFA9887
	{
		I2C_SURF | I2C_FFA,
		MSM_8930_GSBI2_QUP_I2C_BUS_ID,
		msm_i2c_gsbi2_tfa9887_info,
		ARRAY_SIZE(msm_i2c_gsbi2_tfa9887_info),
	},
#endif
};
#endif

static void __init register_i2c_devices(void)
{
#ifdef CONFIG_I2C
	u8 mach_mask = 0;
	int i;
#ifdef CONFIG_MSM_CAMERA
	struct i2c_registry msm8930_camera_i2c_devices;

	msm8930_camera_i2c_devices.machs = I2C_SURF | I2C_FFA | I2C_FLUID | I2C_LIQUID | I2C_RUMI;
	msm8930_camera_i2c_devices.bus = MSM_8930_GSBI4_QUP_I2C_BUS_ID;
	if (system_rev ==0) {
	msm8930_camera_i2c_devices.info = zara_camera_board_info_XA.board_info;
	msm8930_camera_i2c_devices.len = zara_camera_board_info_XA.num_i2c_board_info;
	} else {
		msm8930_camera_i2c_devices.info = zara_camera_board_info_XB.board_info;
		msm8930_camera_i2c_devices.len = zara_camera_board_info_XB.num_i2c_board_info;
	}
#endif

	mach_mask = I2C_SURF;

	for (i = 0; i < ARRAY_SIZE(msm8960_i2c_devices); ++i) {
		if (msm8960_i2c_devices[i].machs & mach_mask)
			i2c_register_board_info(msm8960_i2c_devices[i].bus,
						msm8960_i2c_devices[i].info,
						msm8960_i2c_devices[i].len);
	}
#ifdef CONFIG_MSM_CAMERA
	if (msm8930_camera_i2c_devices.machs & mach_mask)
		i2c_register_board_info(msm8930_camera_i2c_devices.bus,
			msm8930_camera_i2c_devices.info,
			msm8930_camera_i2c_devices.len);
#endif
#endif
}

static void __init zara_init(void)
{
	if (meminfo_init(SYS_MEMORY, SZ_256M) < 0)
		pr_err("meminfo_init() failed!\n");

	htc_add_ramconsole_devices();

	platform_device_register(&msm_gpio_device);
	msm_tsens_early_init(&msm_tsens_pdata);
	msm_thermal_init(&msm_thermal_pdata);
	BUG_ON(msm_rpm_init(&msm8930_rpm_data));
	BUG_ON(msm_rpmrs_levels_init(&msm_rpmrs_data));

	regulator_suppress_info_printing();
	if (msm_xo_init())
		pr_err("Failed to initialize XO votes\n");

	platform_device_register(&msm8930_device_rpm_regulator);

	msm_clock_init(&msm8930_clock_init_data);
	msm8960_device_otg.dev.platform_data = &msm_otg_pdata;
	android_usb_pdata.swfi_latency =
			msm_rpmrs_levels[0].latency_us;
	msm8930_init_gpiomux();
	msm8930_device_qup_spi_gsbi10.dev.platform_data =
				&msm8930_qup_spi_gsbi10_pdata;
#ifdef CONFIG_RAWCHIP
	spi_register_board_info(rawchip_spi_board_info, ARRAY_SIZE(rawchip_spi_board_info));
#endif
	zara_init_pmic();
	zara_i2c_init();
	msm8930_init_gpu();
	msm_spm_init(msm_spm_data, ARRAY_SIZE(msm_spm_data));
	msm_spm_l2_init(msm_spm_l2_data);
	zara_init_buses();

	platform_device_register(&cable_detect_device);

#ifdef CONFIG_HTC_BATT_8960
	htc_battery_cell_init(htc_battery_cells, ARRAY_SIZE(htc_battery_cells));
#endif

#ifdef CONFIG_BT
	bt_export_bd_address();
	msm_uart_dm6_pdata.wakeup_irq = gpio_to_irq(MSM_BT_HOST_WAKE);
#ifdef CONFIG_SERIAL_MSM_HS_BRCM
	msm_device_uart_dm6.name = "msm_serial_hs_brcm";
#else
	msm_device_uart_dm6.name = "msm_serial_hs";
#endif
	msm_device_uart_dm6.dev.platform_data = &msm_uart_dm6_pdata;
#endif

	platform_add_devices(msm8930_footswitch, msm8930_num_footswitch);

	if(cpu_is_msm8930aa())
		platform_device_register(&msm8930aa_device_acpuclk);
	else
		platform_device_register(&msm8930_device_acpuclk);

	platform_add_devices(common_devices, ARRAY_SIZE(common_devices));

	msm_uart_gsbi_gpio_init();
	msm8930_add_vidc_device();
	msm8930_pm8038_gpio_mpp_init();
	platform_add_devices(cdp_devices, ARRAY_SIZE(cdp_devices));
	zara_init_cam();
	msm8930_init_mmc();
	zara_init_mmc();
	register_i2c_devices();
	zara_init_fb();
	syn_init_vkeys_zara();
	zara_wifi_init();
	slim_register_board_info(msm_slim_devices,
		ARRAY_SIZE(msm_slim_devices));
	BUG_ON(msm_pm_boot_init(&msm_pm_boot_pdata));

	zara_init_keypad();
	headset_device_register();

	if (!(board_mfg_mode() == 6 || board_mfg_mode() == 7))
		zara_add_usb_devices();
}

#define SMLOG_MB_SIZE		8 * 1024 *1024
#define PHY_BASE_ADDR1		CONFIG_PHYS_OFFSET
#define SIZE_ADDR1		(136 * 1024 * 1024)
#define SIZE_ADDR1_SMLOG	(SIZE_ADDR1 - SMLOG_MB_SIZE)

#define PHY_BASE_ADDR2		0x90000000
#define SIZE_ADDR2		(768 * 1024 * 1024)

static void __init zara_fixup(struct tag *tags,
				 char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 2;
	mi->bank[0].start = PHY_BASE_ADDR1;
	if(parse_tag_smlog(tags))
		mi->bank[0].size = SIZE_ADDR1_SMLOG;
	else
		mi->bank[0].size = SIZE_ADDR1;
	mi->bank[1].start = PHY_BASE_ADDR2;
	mi->bank[1].size = SIZE_ADDR2;
}

MACHINE_START(ZARA, "zara")
	.fixup = zara_fixup,
	.map_io = zara_map_io,
	.reserve = zara_reserve,
	.init_irq = zara_init_irq,
	.handle_irq = gic_handle_irq,
	.timer = &msm_timer,
	.init_machine = zara_init,
	.init_early = msm8930_allocate_memory_regions,
	.init_very_early = zara_early_memory,
	.restart = msm_restart,
MACHINE_END
