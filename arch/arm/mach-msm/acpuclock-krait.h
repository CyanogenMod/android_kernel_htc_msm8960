/*
 * Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#ifndef __ARCH_ARM_MACH_MSM_ACPUCLOCK_KRAIT_H
#define __ARCH_ARM_MACH_MSM_ACPUCLOCK_KRAIT_H

#define L2(x) (x)
#define BW_MBPS(_bw) \
	{ \
		.vectors = (struct msm_bus_vectors[]){ \
			{\
				.src = MSM_BUS_MASTER_AMPSS_M0, \
				.dst = MSM_BUS_SLAVE_EBI_CH0, \
				.ib = (_bw) * 1000000UL, \
			}, \
			{ \
				.src = MSM_BUS_MASTER_AMPSS_M1, \
				.dst = MSM_BUS_SLAVE_EBI_CH0, \
				.ib = (_bw) * 1000000UL, \
			}, \
		}, \
		.num_paths = 2, \
	}

enum src_id {
	PLL_0 = 0,
	HFPLL,
	PLL_8,
	NUM_SRC_ID
};

enum pvs {
	PVS_SLOW = 0,
	PVS_NOMINAL = 1,
	PVS_FAST = 3,
	PVS_FASTER = 4,
	NUM_PVS = 7
};

#define NUM_SPEED_BINS (16)

enum scalables {
	CPU0 = 0,
	CPU1,
	CPU2,
	CPU3,
	L2,
	MAX_SCALABLES
};


enum hfpll_vdd_levels {
	HFPLL_VDD_NONE,
	HFPLL_VDD_LOW,
	HFPLL_VDD_NOM,
	HFPLL_VDD_HIGH,
	NUM_HFPLL_VDD
};

enum vregs {
	VREG_CORE,
	VREG_MEM,
	VREG_DIG,
	VREG_HFPLL_A,
	VREG_HFPLL_B,
	NUM_VREG
};

struct vreg {
	const char *name;
	const int max_vdd;
	struct regulator *reg;
	struct rpm_regulator *rpm_reg;
	int cur_vdd;
	int cur_ua;
};

struct core_speed {
	unsigned long khz;
	int src;
	u32 pri_src_sel;
	u32 pll_l_val;
};

struct l2_level {
	const struct core_speed speed;
	const int vdd_dig;
	const int vdd_mem;
	const unsigned int bw_level;
};

struct acpu_level {
#ifdef CONFIG_ACPU_CUSTOM_FREQ_SUPPORT
	unsigned int use_for_scaling;
#else
	const int use_for_scaling;
#endif
	const struct core_speed speed;
	const unsigned int l2_level;
	int vdd_core;
	int ua_core;
	unsigned int avsdscr_setting;
};

struct hfpll_data {
	const u32 mode_offset;
	const u32 l_offset;
	const u32 m_offset;
	const u32 n_offset;
	const u32 config_offset;
	const u32 config_val;
	const bool has_droop_ctl;
	const u32 droop_offset;
	const u32 droop_val;
	u32 low_vdd_l_max;
	u32 nom_vdd_l_max;
	const u32 low_vco_l_max;
	const int vdd[NUM_HFPLL_VDD];
};

struct scalable {
	const phys_addr_t hfpll_phys_base;
	void __iomem *hfpll_base;
	const phys_addr_t aux_clk_sel_phys;
	const u32 aux_clk_sel;
	const u32 sec_clk_sel;
	const u32 l2cpmr_iaddr;
	const struct core_speed *cur_speed;
	unsigned int l2_vote;
	struct vreg vreg[NUM_VREG];
	bool initialized;
	bool avs_enabled;
};

struct pvs_table {
	struct acpu_level *table;
	size_t size;
	int boost_uv;
};

struct acpuclk_krait_params {
	struct scalable *scalable;
	size_t scalable_size;
	struct hfpll_data *hfpll_data;
	struct pvs_table (*pvs_tables)[NUM_PVS];
	struct l2_level *l2_freq_tbl;
	size_t l2_freq_tbl_size;
	phys_addr_t pte_efuse_phys;
	struct msm_bus_scale_pdata *bus_scale;
	unsigned long stby_khz;
};

struct drv_data {
	struct acpu_level *acpu_freq_tbl;
	const struct l2_level *l2_freq_tbl;
	struct scalable *scalable;
	struct hfpll_data *hfpll_data;
	u32 bus_perf_client;
	struct msm_bus_scale_pdata *bus_scale;
	int boost_uv;
	int speed_bin;
	int pvs_bin;
	struct device *dev;
};

struct acpuclk_platform_data {
	bool uses_pm8917;
};

extern int acpuclk_krait_init(struct device *dev,
			      const struct acpuclk_krait_params *params);

#ifdef CONFIG_DEBUG_FS
extern void __init acpuclk_krait_debug_init(struct drv_data *drv);
#else
static inline void acpuclk_krait_debug_init(void) { }
#endif

#endif
