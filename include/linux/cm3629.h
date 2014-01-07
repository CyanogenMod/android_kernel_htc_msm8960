/* include/linux/cm3629.h
 *
 * Copyright (C) 2010 HTC, Inc.
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

#ifndef __LINUX_CM3629_H
#define __LINUX_CM3629_H

#define CM3629_I2C_NAME "CM3629"


#define ALS_config_cmd		0x00
#define ALS_high_thd		0x01
#define ALS_low_thd		0x02

#define PS_config		0x03
#define PS_config_ms		0x04
#define PS_CANC			0x05
#define PS_1_thd		0x06
#define PS_2_thd		0x07
#define PS_data			0x08
#define ALS_data		0x09

#define WS_data			0x0A
#define INT_FLAG		0x0B
#define CH_ID			0x0C


#define ALS_CALIBRATED		0x6DA5
#define PS_CALIBRATED		0x5053

#define CM3629_ALS_IT_50ms 	(0 << 6)
#define CM3629_ALS_IT_100ms 	(1 << 6)
#define CM3629_ALS_IT_200ms 	(2 << 6)
#define CM3629_ALS_IT_400ms 	(3 << 6)

#define CM3629_ALS_IT_80ms 		(0 << 6)
#define CM3629_ALS_IT_160ms 	(1 << 6)
#define CM3629_ALS_IT_320ms 	(2 << 6)
#define CM3629_ALS_IT_640ms 	(3 << 6)


#define CM3629_ALS_AV_1		(0 << 4)
#define CM3629_ALS_AV_2		(1 << 4)
#define CM3629_ALS_AV_4		(2 << 4)
#define CM3629_ALS_AV_8		(3 << 4)
#define CM3629_ALS_PERS_1 	(0 << 2) 
#define CM3629_ALS_PERS_2 	(1 << 2)
#define CM3629_ALS_PERS_4 	(2 << 2)
#define CM3629_ALS_PERS_8 	(3 << 2)
#define CM3629_ALS_INT_EN	(1 << 1) 
#define CM3629_ALS_SD		(1 << 0) 

#define CM3629_PS_63_STEPS 	(0 << 4)
#define CM3629_PS_120_STEPS 	(1 << 4)
#define CM3629_PS_191_STEPS 	(2 << 4)
#define CM3629_PS_255_STEPS 	(3 << 4)



#define CM3629_PS_DR_1_40 	(0 << 6)
#define CM3629_PS_DR_1_80 	(1 << 6)
#define CM3629_PS_DR_1_160 	(2 << 6)
#define CM3629_PS_DR_1_320 	(3 << 6)

#define CM3629_PS_IT_1T 	(0 << 4)
#define CM3629_PS_IT_1_3T 	(1 << 4)
#define CM3629_PS_IT_1_6T 	(2 << 4)
#define CM3629_PS_IT_2T 	(3 << 4)

#define CM3629_PS1_PERS_1 	(0 << 2)
#define CM3629_PS1_PERS_2 	(1 << 2)
#define CM3629_PS1_PERS_3 	(2 << 2)
#define CM3629_PS1_PERS_4 	(3 << 2)

#define CM3629_PS2_SD		(1 << 1) 
#define CM3629_PS1_SD		(1 << 0) 

#define CM3629_PS_ITB_1_2 	(0 << 6)
#define CM3629_PS_ITB_1 	(1 << 6)
#define CM3629_PS_ITB_2 	(2 << 6)
#define CM3629_PS_ITB_4 	(3 << 6)

#define CM3629_PS_ITR_1 	(0 << 4)
#define CM3629_PS_ITR_1_2  	(1 << 4)
#define CM3629_PS_ITR_1_4 	(2 << 4)
#define CM3629_PS_ITR_1_8 	(3 << 4)

#define CM3629_PS2_INT_DIS 	(0 << 2)
#define CM3629_PS2_INT_CLS 	(1 << 2)
#define CM3629_PS2_INT_AWY 	(2 << 2)
#define CM3629_PS2_INT_BOTH	(3 << 2)

#define CM3629_PS1_INT_DIS 	(0 << 0)
#define CM3629_PS1_INT_CLS 	(1 << 0)
#define CM3629_PS1_INT_AWY 	(2 << 0)
#define CM3629_PS1_INT_BOTH	(3 << 0)


#define CM3629_PS2_PROL_4 	(0 << 6)
#define CM3629_PS2_PROL_8 	(1 << 6)
#define CM3629_PS2_PROL_16	(2 << 6)
#define CM3629_PS2_PROL_32 	(3 << 6)

#define CM3629_PS_INTT 		(1 << 5)
#define CM3629_PS_SMART_PRES 	(1 << 4)
#define CM3629_PS_PS_FOR 	(1 << 3)
#define CM3629_PS_PS_TRIG	(1 << 2)

#define CM3629_PS2_PERS_1 	(0 << 0)
#define CM3629_PS2_PERS_2 	(1 << 0)
#define CM3629_PS2_PERS_3 	(2 << 0)
#define CM3629_PS2_PERS_4 	(3 << 0)

#define CM3629_PS_MS 		(1 << 5)

#define CM3629_PS2_SPFLAG 	(1 << 7)
#define CM3629_PS1_SPFLAG 	(1 << 6)

#define CM3629_ALS_IF_L 	(1 << 5)
#define CM3629_ALS_IF_H 	(1 << 4)
#define CM3629_PS2_IF_CLOSE	(1 << 3)
#define CM3629_PS2_IF_AWAY	(1 << 2)
#define CM3629_PS1_IF_CLOSE	(1 << 1)
#define CM3629_PS1_IF_AWAY	(1 << 0)


extern unsigned int ps_kparam1;
extern unsigned int ps_kparam2;
extern unsigned int als_kadc;
extern unsigned int ws_kadc;
enum {
	CAPELLA_CM36282,
	CAPELLA_CM36292,
};

enum {
	CM3629_PS_DISABLE,
	CM3629_PS1_ONLY,
	CM3629_PS2_ONLY,
	CM3629_PS1_PS2_BOTH,
};
int get_lightsensoradc(void);
int get_lightsensorkadc(void);

struct cm3629_platform_data {
	int model;
	int intr;
	uint16_t levels[10];
	uint16_t correction[10];
	uint16_t golden_adc;
	int (*power)(int, uint8_t); 
	int (*lpm_power)(uint8_t); 
	uint16_t cm3629_slave_address;
	uint8_t ps_select;
	uint8_t ps1_thd_set;
	uint8_t ps1_thh_diff;
	uint8_t ps2_thd_set;
	uint8_t inte_cancel_set;
	
	uint8_t ps_conf2_val; 
	uint8_t *mapping_table;
	uint8_t mapping_size;
	uint8_t ps_base_index;
	uint8_t ps_calibration_rule;
	uint8_t ps_conf1_val;
	uint8_t ps_conf3_val;
	uint8_t dynamical_threshold;
	uint8_t ps1_thd_no_cal;
	uint8_t ps1_thd_with_cal;
	uint8_t ps2_thd_no_cal;
	uint8_t ps2_thd_with_cal;
	uint8_t ps_th_add;
	uint8_t ls_cmd;
	uint8_t ps1_adc_offset;
	uint8_t ps2_adc_offset;
	uint8_t ps_debounce;
	uint16_t ps_delay_time;
	unsigned int no_need_change_setting;
	uint8_t dark_level;
	uint16_t w_golden_adc;
};

#endif
