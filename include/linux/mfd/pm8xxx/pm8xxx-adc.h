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

#ifndef __PM8XXX_ADC_H
#define __PM8XXX_ADC_H

#include <linux/kernel.h>
#include <linux/list.h>

enum pm8xxx_adc_channels {
	CHANNEL_VCOIN = 0,
	CHANNEL_VBAT,
	CHANNEL_DCIN,
	CHANNEL_ICHG,
	CHANNEL_VPH_PWR,
	CHANNEL_IBAT,
	CHANNEL_MPP_1,
	CHANNEL_MPP_2,
	CHANNEL_BATT_THERM,
	CHANNEL_BATT_ID_THERM = CHANNEL_BATT_THERM,
	CHANNEL_BATT_ID,
	CHANNEL_USBIN,
	CHANNEL_DIE_TEMP,
	CHANNEL_625MV,
	CHANNEL_125V,
	CHANNEL_CHG_TEMP,
	CHANNEL_MUXOFF,
	CHANNEL_NONE,
	ADC_MPP_1_ATEST_8 = 20,
	ADC_MPP_1_USB_SNS_DIV20,
	ADC_MPP_1_DCIN_SNS_DIV20,
	ADC_MPP_1_AMUX3,
	ADC_MPP_1_AMUX4,
	ADC_MPP_1_AMUX5,
	ADC_MPP_1_AMUX6,
	ADC_MPP_1_AMUX7,
	ADC_MPP_1_AMUX8,
	ADC_MPP_1_ATEST_1,
	ADC_MPP_1_ATEST_2,
	ADC_MPP_1_ATEST_3,
	ADC_MPP_1_ATEST_4,
	ADC_MPP_1_ATEST_5,
	ADC_MPP_1_ATEST_6,
	ADC_MPP_1_ATEST_7,
	ADC_MPP_2_ATEST_8 = 40,
	ADC_MPP_2_USB_SNS_DIV20,
	ADC_MPP_2_DCIN_SNS_DIV20,
	ADC_MPP_2_AMUX3,
	ADC_MPP_2_AMUX4,
	ADC_MPP_2_AMUX5,
	ADC_MPP_2_AMUX6,
	ADC_MPP_2_AMUX7,
	ADC_MPP_2_AMUX8,
	ADC_MPP_2_ATEST_1,
	ADC_MPP_2_ATEST_2,
	ADC_MPP_2_ATEST_3,
	ADC_MPP_2_ATEST_4,
	ADC_MPP_2_ATEST_5,
	ADC_MPP_2_ATEST_6,
	ADC_MPP_2_ATEST_7,
	ADC_CHANNEL_MAX_NUM,
};

#define PM8XXX_ADC_PMIC_0	0x0

#define PM8XXX_CHANNEL_ADC_625_UV	625000
#define PM8XXX_CHANNEL_MPP_SCALE1_IDX	20
#define PM8XXX_CHANNEL_MPP_SCALE3_IDX	40

#define PM8XXX_AMUX_MPP_1	0x1
#define PM8XXX_AMUX_MPP_2	0x2
#define PM8XXX_AMUX_MPP_3	0x3
#define PM8XXX_AMUX_MPP_4	0x4
#define PM8XXX_AMUX_MPP_5	0x5
#define PM8XXX_AMUX_MPP_6	0x6
#define PM8XXX_AMUX_MPP_7	0x7
#define PM8XXX_AMUX_MPP_8	0x8
#define PM8XXX_AMUX_MPP_9	0x9
#define PM8XXX_AMUX_MPP_10	0xA
#define PM8XXX_AMUX_MPP_11	0xB
#define PM8XXX_AMUX_MPP_12	0xC

#define PM8XXX_ADC_DEV_NAME	"pm8xxx-adc"

enum pm8xxx_adc_decimation_type {
	ADC_DECIMATION_TYPE1 = 0,
	ADC_DECIMATION_TYPE2,
	ADC_DECIMATION_TYPE3,
	ADC_DECIMATION_TYPE4,
	ADC_DECIMATION_NONE,
};

enum pm8xxx_adc_calib_type {
	ADC_CALIB_ABSOLUTE = 0,
	ADC_CALIB_RATIOMETRIC,
	ADC_CALIB_NONE,
};

enum pm8xxx_adc_channel_scaling_param {
	CHAN_PATH_SCALING1 = 0,
	CHAN_PATH_SCALING2,
	CHAN_PATH_SCALING3,
	CHAN_PATH_SCALING4,
	CHAN_PATH_SCALING_NONE,
};

enum pm8xxx_adc_amux_input_rsv {
	AMUX_RSV0 = 0,
	AMUX_RSV1,
	AMUX_RSV2,
	AMUX_RSV3,
	AMUX_RSV4,
	AMUX_RSV5,
	AMUX_NONE,
};

enum pm8xxx_adc_premux_mpp_scale_type {
	PREMUX_MPP_SCALE_0 = 0,
	PREMUX_MPP_SCALE_1,
	PREMUX_MPP_SCALE_1_DIV3,
	PREMUX_MPP_NONE,
};

enum pm8xxx_adc_scale_fn_type {
	ADC_SCALE_DEFAULT = 0,
	ADC_SCALE_BATT_THERM,
	ADC_SCALE_PA_THERM,
	ADC_SCALE_PMIC_THERM,
	ADC_SCALE_XOTHERM,
	ADC_SCALE_NONE,
};

struct pm8xxx_adc_linear_graph {
	int64_t dy;
	int64_t dx;
	int64_t adc_vref;
	int64_t adc_gnd;
};

struct pm8xxx_adc_map_pt {
	int32_t x;
	int32_t y;
};

struct pm8xxx_adc_map_table {
	const struct pm8xxx_adc_map_pt *table;
	int32_t size;
};

struct pm8xxx_adc_scaling_ratio {
	int32_t num;
	int32_t den;
};

struct pm8xxx_adc_properties {
	uint32_t	adc_vdd_reference;
	uint32_t	bitresolution;
	bool		bipolar;
};

struct pm8xxx_adc_chan_properties {
	uint32_t			offset_gain_numerator;
	uint32_t			offset_gain_denominator;
	struct pm8xxx_adc_linear_graph	adc_graph[2];
};

struct pm8xxx_adc_chan_result {
	uint32_t	chan;
	int32_t		adc_code;
	int64_t		measurement;
	int64_t		physical;
};

#if defined(CONFIG_SENSORS_PM8XXX_ADC)					\
			|| defined(CONFIG_SENSORS_PM8XXX_ADC_MODULE)
void pm8xxx_adc_set_adcmap_btm_table(
			struct pm8xxx_adc_map_table *adcmap_table);
int32_t pm8xxx_adc_scale_default(int32_t adc_code,
			const struct pm8xxx_adc_properties *adc_prop,
			const struct pm8xxx_adc_chan_properties *chan_prop,
			struct pm8xxx_adc_chan_result *chan_rslt);
int32_t pm8xxx_adc_tdkntcg_therm(int32_t adc_code,
			const struct pm8xxx_adc_properties *adc_prop,
			const struct pm8xxx_adc_chan_properties *chan_prop,
			struct pm8xxx_adc_chan_result *chan_rslt);
int32_t pm8xxx_adc_scale_batt_therm(int32_t adc_code,
			const struct pm8xxx_adc_properties *adc_prop,
			const struct pm8xxx_adc_chan_properties *chan_prop,
			struct pm8xxx_adc_chan_result *chan_rslt);
int32_t pm8xxx_adc_scale_pa_therm(int32_t adc_code,
			const struct pm8xxx_adc_properties *adc_prop,
			const struct pm8xxx_adc_chan_properties *chan_prop,
			struct pm8xxx_adc_chan_result *chan_rslt);
int32_t pm8xxx_adc_scale_pmic_therm(int32_t adc_code,
			const struct pm8xxx_adc_properties *adc_prop,
			const struct pm8xxx_adc_chan_properties *chan_prop,
			struct pm8xxx_adc_chan_result *chan_rslt);
int32_t pm8xxx_adc_scale_batt_id(int32_t adc_code,
			const struct pm8xxx_adc_properties *adc_prop,
			const struct pm8xxx_adc_chan_properties *chan_prop,
			struct pm8xxx_adc_chan_result *chan_rslt);
#else
static inline void pm8xxx_adc_set_adcmap_btm_table(
			struct pm8xxx_adc_map_table *adcmap_table)
{ return; }
static inline int32_t pm8xxx_adc_scale_default(int32_t adc_code,
			const struct pm8xxx_adc_properties *adc_prop,
			const struct pm8xxx_adc_chan_properties *chan_prop,
			struct pm8xxx_adc_chan_result *chan_rslt)
{ return -ENXIO; }
static inline int32_t pm8xxx_adc_tdkntcg_therm(int32_t adc_code,
			const struct pm8xxx_adc_properties *adc_prop,
			const struct pm8xxx_adc_chan_properties *chan_prop,
			struct pm8xxx_adc_chan_result *chan_rslt)
{ return -ENXIO; }
static inline int32_t pm8xxx_adc_scale_batt_therm(int32_t adc_code,
			const struct pm8xxx_adc_properties *adc_prop,
			const struct pm8xxx_adc_chan_properties *chan_prop,
			struct pm8xxx_adc_chan_result *chan_rslt)
{ return -ENXIO; }
static inline int32_t pm8xxx_adc_scale_pa_therm(int32_t adc_code,
			const struct pm8xxx_adc_properties *adc_prop,
			const struct pm8xxx_adc_chan_properties *chan_prop,
			struct pm8xxx_adc_chan_result *chan_rslt)
{ return -ENXIO; }
static inline int32_t pm8xxx_adc_scale_pmic_therm(int32_t adc_code,
			const struct pm8xxx_adc_properties *adc_prop,
			const struct pm8xxx_adc_chan_properties *chan_prop,
			struct pm8xxx_adc_chan_result *chan_rslt)
{ return -ENXIO; }
static inline int32_t pm8xxx_adc_scale_batt_id(int32_t adc_code,
			const struct pm8xxx_adc_properties *adc_prop,
			const struct pm8xxx_adc_chan_properties *chan_prop,
			struct pm8xxx_adc_chan_result *chan_rslt)
{ return -ENXIO; }
#endif

struct pm8xxx_adc_scale_fn {
	int32_t (*chan) (int32_t,
		const struct pm8xxx_adc_properties *,
		const struct pm8xxx_adc_chan_properties *,
		struct pm8xxx_adc_chan_result *);
};

struct pm8xxx_adc_amux {
	char					*name;
	enum pm8xxx_adc_channels		channel_name;
	enum pm8xxx_adc_channel_scaling_param	chan_path_prescaling;
	enum pm8xxx_adc_amux_input_rsv		adc_rsv;
	enum pm8xxx_adc_decimation_type		adc_decimation;
	enum pm8xxx_adc_scale_fn_type		adc_scale_fn;
};

struct pm8xxx_adc_arb_btm_param {
	int32_t		low_thr_temp;
	int32_t		high_thr_temp;
	uint64_t	low_thr_voltage;
	uint64_t	high_thr_voltage;
	int32_t		interval;
	void		(*btm_warm_fn) (bool);
	void		(*btm_cool_fn) (bool);
};

int32_t pm8xxx_adc_batt_scaler(struct pm8xxx_adc_arb_btm_param *,
			const struct pm8xxx_adc_properties *adc_prop,
			const struct pm8xxx_adc_chan_properties *chan_prop);
struct pm8xxx_adc_platform_data {
	struct pm8xxx_adc_properties	*adc_prop;
	struct pm8xxx_adc_amux		*adc_channel;
	uint32_t			adc_num_board_channel;
	uint32_t			adc_mpp_base;
	struct pm8xxx_adc_map_table	*adc_map_btm_table;
	void				(*pm8xxx_adc_device_register)(void);
};

#if defined(CONFIG_SENSORS_PM8XXX_ADC)				\
			|| defined(CONFIG_SENSORS_PM8XXX_ADC_MODULE)
uint32_t pm8xxx_adc_read(enum pm8xxx_adc_channels channel,
				struct pm8xxx_adc_chan_result *result);
uint32_t pm8xxx_adc_mpp_config_read(uint32_t mpp_num,
				enum pm8xxx_adc_channels channel,
				struct pm8xxx_adc_chan_result *result);
uint32_t pm8xxx_adc_btm_start(void);

uint32_t pm8xxx_adc_btm_end(void);

uint32_t pm8xxx_adc_btm_configure(struct pm8xxx_adc_arb_btm_param *);

int pm8xxx_adc_btm_is_cool(void);

int pm8xxx_adc_btm_is_warm(void);
#else
static inline uint32_t pm8xxx_adc_read(uint32_t channel,
				struct pm8xxx_adc_chan_result *result)
{ return -ENXIO; }
static inline uint32_t pm8xxx_adc_mpp_config_read(uint32_t mpp_num,
				enum pm8xxx_adc_channels channel,
				struct pm8xxx_adc_chan_result *result)
{ return -ENXIO; }
static inline uint32_t pm8xxx_adc_btm_start(void)
{ return -ENXIO; }
static inline uint32_t pm8xxx_adc_btm_end(void)
{ return -ENXIO; }
static inline uint32_t pm8xxx_adc_btm_configure(
		struct pm8xxx_adc_arb_btm_param *param)
{ return -ENXIO; }
static inline int pm8xxx_adc_btm_is_cool(void)
{ return 0; }
static inline int pm8xxx_adc_btm_is_warm(void)
{ return 0; }
#endif

#endif 
