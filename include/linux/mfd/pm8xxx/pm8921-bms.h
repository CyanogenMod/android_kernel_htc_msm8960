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

#ifndef __PM8XXX_BMS_H
#define __PM8XXX_BMS_H

#include <linux/errno.h>

#define PM8921_BMS_DEV_NAME	"pm8921-bms"

#define FCC_CC_COLS		5
#define FCC_TEMP_COLS		8

#define PC_CC_ROWS             31
#define PC_CC_COLS             13

#define PC_TEMP_ROWS		31
#define PC_TEMP_COLS		8

#define MAX_SINGLE_LUT_COLS	20

#define OCV_UPDATE_STOP_BIT_CABLE_IN			(1)
#define OCV_UPDATE_STOP_BIT_BATT_LEVEL			(1<<1)
#define OCV_UPDATE_STOP_BIT_ATTR_FILE			(1<<2)
#define OCV_UPDATE_STOP_BIT_BOOT_UP			(1<<3)

struct single_row_lut {
	int x[MAX_SINGLE_LUT_COLS];
	int y[MAX_SINGLE_LUT_COLS];
	int cols;
};

struct sf_lut {
	int rows;
	int cols;
	int row_entries[PC_CC_COLS];
	int percent[PC_CC_ROWS];
	int sf[PC_CC_ROWS][PC_CC_COLS];
};

struct pc_temp_ocv_lut {
	int rows;
	int cols;
	int temp[PC_TEMP_COLS];
	int percent[PC_TEMP_ROWS];
	int ocv[PC_TEMP_ROWS][PC_TEMP_COLS];
};

struct pm8921_bms_battery_data {
	unsigned int		fcc;
	struct single_row_lut	*fcc_temp_lut;
	struct single_row_lut	*fcc_sf_lut;
	struct pc_temp_ocv_lut	*pc_temp_ocv_lut;
	struct sf_lut		*pc_sf_lut;
	struct sf_lut		*rbatt_sf_lut;
	struct sf_lut		*rbatt_est_ocv_lut;
	int			default_rbatt_mohm;
	int			delta_rbatt_mohm;
	int			level_ocv_update_stop_begin; 
	int			level_ocv_update_stop_end; 
};

struct pm8921_bms_pj_data {
	struct single_row_lut	*pj_vth_discharge_lut;
	struct single_row_lut	*pj_dvi_discharge_lut;
	struct single_row_lut	*pj_vth_charge_lut;
	struct single_row_lut	*pj_dvi_charge_lut;
	struct single_row_lut	*pj_temp_lut;
};

struct pm8xxx_bms_core_data {
	unsigned int	batt_temp_channel;
	unsigned int	vbat_channel;
	unsigned int	ref625mv_channel;
	unsigned int	ref1p25v_channel;
	unsigned int	batt_id_channel;
};

enum battery_type {
	BATT_UNKNOWN = 0,
	BATT_PALLADIUM,
	BATT_DESAY,
};

struct pm8921_bms_platform_data {
	struct pm8xxx_bms_core_data	bms_cdata;
	enum battery_type		battery_type;
	unsigned int			r_sense;
	unsigned int			i_test;
	unsigned int			v_failure;
	unsigned int			max_voltage_uv;
	unsigned int			rconn_mohm;
	int				store_batt_data_soc_thre;
	int				enable_fcc_learning;
	unsigned int			criteria_sw_est_ocv; 
	unsigned int			rconn_mohm_sw_est_ocv;
	void (*get_power_jacket_status) (int *full, int *status, int *exist);
};

extern int batt_stored_magic_num;
extern int batt_stored_soc;
extern int batt_stored_ocv_uv;
extern int batt_stored_cc_uv;
extern unsigned long batt_stored_time_ms;

#if defined(CONFIG_PM8921_BMS) || defined(CONFIG_PM8921_BMS_MODULE)
extern struct pm8921_bms_battery_data  palladium_1500_data;
extern struct pm8921_bms_battery_data  desay_5200_data;
int pm8921_bms_get_vsense_avg(int *result);

int pm8921_bms_get_battery_current(int *result);

int pm8921_bms_get_percent_charge(void);

int pm8921_calculate_pj_level(int Vjk, int is_charging, int batt_temp);

int pm8921_bms_get_fcc(void);

int pm8921_bms_charging_began(void);
void pm8921_bms_charging_end(int is_battery_full);

void pm8921_bms_calibrate_hkadc(void);
int pm8921_bms_stop_ocv_updates(void);
int pm8921_bms_start_ocv_updates(void);
int pm8921_bms_get_simultaneous_battery_voltage_and_current(int *ibat_ua,
								int *vbat_uv);
int pm8921_bms_get_rbatt(void);
void pm8921_bms_invalidate_shutdown_soc(void);
int pm8921_bms_dump_all(void);
#ifdef CONFIG_HTC_BATT_8960
int pm8921_bms_get_batt_current(int *result);

int pm8921_store_hw_reset_reason(int is_hw_reset);
int pm8921_bms_get_batt_soc(int *result);
int pm8921_bms_get_batt_cc(int *result);
int pm8921_bms_store_battery_data_emmc(void);
int pm8921_bms_store_battery_ui_soc(int soc_ui);
int pm8921_bms_get_battery_ui_soc(void);
int pm8921_bms_get_attr_text(char *buf, int size);
#endif 
#else
static inline int pm8921_bms_get_vsense_avg(int *result)
{
	return -ENXIO;
}
static inline int pm8921_bms_get_battery_current(int *result)
{
	return -ENXIO;
}
static inline int pm8921_bms_get_percent_charge(void)
{
	return -ENXIO;
}
static inline int pm8921_calculate_pj_level(int Vjk, int is_charging, int batt_temp)
{
	return -ENXIO;
}
static inline int pm8921_bms_get_fcc(void)
{
	return -ENXIO;
}
static inline void pm8921_bms_charging_began(void)
{
}
static inline void pm8921_bms_charging_end(int is_battery_full)
{
}
static inline void pm8921_bms_calibrate_hkadc(void)
{
}
static inline void pm8921_bms_stop_ocv_updates(void)
{
}
static inline void pm8921_bms_start_ocv_updates(void)
{
}
static inline int pm8921_bms_get_simultaneous_battery_voltage_and_current(
						int *ibat_ua, int *vbat_uv)
{
	return -ENXIO;
}
static inline int pm8921_bms_get_rbatt(void)
{
	return -EINVAL;
}
static inline void pm8921_bms_invalidate_shutdown_soc(void)
{
}
static inline int pm8921_bms_dump_all(void)
{
	return -ENXIO;
}
#ifdef CONFIG_HTC_BATT_8960
static inline int pm8921_bms_get_batt_current(int *result)
{
	return -ENXIO;
}

static inline int pm8921_store_hw_reset_reason(int is_hw_reset)
{
	return -ENXIO;
}

static inline int pm8921_bms_get_batt_soc(int *result)
{
	return -ENXIO;
}
static inline int pm8921_bms_get_batt_cc(int *result)
{
	return -ENXIO;
}
static inline int pm8921_bms_store_battery_data_emmc(void)
{
	return -ENXIO;
}
static inline int pm8921_bms_store_battery_ui_soc(int soc_ui)
{
	return -ENXIO;
}
static inline int pm8921_bms_get_battery_ui_soc(void)
{
	return -ENXIO;
}
static inline int pm8921_bms_get_attr_text(char *buf, int size)
{
	return 0;
}
#endif 
#endif

#endif
