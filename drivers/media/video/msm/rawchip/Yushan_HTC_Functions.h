/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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

#ifndef _YUSHAN_HTC_FUNCTIONS_H
#define _YUSHAN_HTC_FUNCTIONS_H

#include "Yushan_API.h"
#include "Yushan_Platform_Specific.h"

enum yushan_orientation_type {
	YUSHAN_ORIENTATION_NONE,
	YUSHAN_ORIENTATION_MIRROR,
	YUSHAN_ORIENTATION_FLIP,
	YUSHAN_ORIENTATION_MIRROR_FLIP,
};

struct yushan_reg_conf {
	uint16_t addr;
	uint8_t  data;
};

struct yushan_reg_t {
	uint16_t pdpcode_first_addr;
	uint8_t *pdpcode;
	uint16_t pdpcode_size;

	uint16_t pdpclib_first_addr;
	uint8_t *pdpclib;
	uint16_t pdpclib_size;

	uint16_t pdpBootAddr;
	uint16_t pdpStartAddr;

	uint16_t dppcode_first_addr;
	uint8_t *dppcode;
	uint16_t dppcode_size;

	uint16_t dppclib_first_addr;
	uint8_t *dppclib;
	uint16_t dppclib_size;

	uint16_t dppBootAddr;
	uint16_t dppStartAddr;

	uint16_t dopcode_first_addr;
	uint8_t *dopcode;
	uint16_t dopcode_size;

	uint16_t dopclib_first_addr;
	uint8_t *dopclib;
	uint16_t dopclib_size;

	uint16_t dopBootAddr;
	uint16_t dopStartAddr;
};

extern struct yushan_reg_t yushan_regs;


struct yushan_reg_u_code_t {
	uint16_t pdpcode_first_addr;
	uint8_t *pdpcode;
	uint16_t pdpcode_size;

	uint16_t pdpBootAddr;
	uint16_t pdpStartAddr;

	uint16_t dppcode_first_addr;
	uint8_t *dppcode;
	uint16_t dppcode_size;

	uint16_t dppBootAddr;
	uint16_t dppStartAddr;

	uint16_t dopcode_first_addr;
	uint8_t *dopcode;
	uint16_t dopcode_size;

	uint16_t dopBootAddr;
	uint16_t dopStartAddr;
};

extern struct yushan_reg_u_code_t yushan_u_code_r2;
extern struct yushan_reg_u_code_t yushan_u_code_r3;



struct yushan_reg_clib_t {
	uint16_t pdpclib_first_addr;
	uint8_t *pdpclib;
	uint16_t pdpclib_size;

	uint16_t dppclib_first_addr;
	uint8_t *dppclib;
	uint16_t dppclib_size;

	uint16_t dopclib_first_addr;
	uint8_t *dopclib;
	uint16_t dopclib_size;
};

extern struct yushan_reg_clib_t yushan_regs_clib_s5k3h2yx;
extern struct yushan_reg_clib_t yushan_regs_clib_imx175;
extern struct yushan_reg_clib_t yushan_regs_clib_ov8838;
extern struct yushan_reg_clib_t yushan_regs_clib_ar0260;
extern struct yushan_reg_clib_t yushan_regs_clib_ov2722;


struct rawchip_sensor_init_data {
	const char *sensor_name;
	uint8_t spi_clk;
	uint8_t ext_clk;
	uint8_t lane_cnt;
	uint8_t orientation;
	uint8_t use_ext_1v2;
	uint16_t bitrate;
	uint16_t width;
	uint16_t height;
	uint16_t blk_pixels;
	uint16_t blk_lines;
	uint16_t x_addr_start;
	uint16_t y_addr_start;
	uint16_t x_addr_end;
	uint16_t y_addr_end;
	uint16_t x_even_inc;
	uint16_t x_odd_inc;
	uint16_t y_even_inc;
	uint16_t y_odd_inc;
	uint8_t binning_rawchip;
	uint8_t use_rawchip;
};

typedef enum {
  RAWCHIP_NEWFRAME_ACK_NOCHANGE,
  RAWCHIP_NEWFRAME_ACK_ENABLE,
  RAWCHIP_NEWFRAME_ACK_DISABLE,
} rawchip_newframe_ack_enable_t;

typedef struct {
  uint16_t gain;
  uint16_t dig_gain;
  uint16_t exp;
} rawchip_aec_params_t;

typedef struct {
  uint8_t rg_ratio; 
  uint8_t bg_ratio; 
} rawchip_awb_params_t;

typedef struct {
  int update;
  rawchip_aec_params_t aec_params;
  rawchip_awb_params_t awb_params;
} rawchip_update_aec_awb_params_t;

typedef struct {
  uint8_t active_number;
  Yushan_AF_ROI_t sYushanAfRoi[5];
} rawchip_af_params_t;

typedef struct {
  int update;
  rawchip_af_params_t af_params;
} rawchip_update_af_params_t;

typedef struct {
  uint8_t value;
}Yushan_DXO_DOP_afStrategy_t;

typedef struct
{
	Yushan_AF_Stats_t udwAfStats[5];
	uint16_t  frameIdx;
} rawchip_af_stats;

void YushanPrintDxODOPAfStrategy(void);
void YushanPrintFrameNumber(void);
void YushanPrintVisibleLineSizeAndRoi(void);
void YushanPrintImageInformation(void);

void Reset_Yushan(void);
void ASIC_Test(void);


int Yushan_sensor_open_init(struct rawchip_sensor_init_data data);
void Yushan_dump_register(void);
void Yushan_dump_all_register(void);
void Yushan_dump_Dxo(void);

int Yushan_ContextUpdate_Wrapper(Yushan_New_Context_Config_t	sYushanNewContextConfig, Yushan_ImageChar_t	sImageNewChar_context);
int Yushan_Get_Version(rawchip_dxo_version* dxo_version);
int Yushan_Set_AF_Strategy(uint8_t afStrategy);
bool_t Yushan_Dxo_Dop_Af_Run(Yushan_AF_ROI_t	*sYushanAfRoi, uint32_t *pAfStatsGreen, uint8_t	bRoiActiveNumber);
int Yushan_get_AFSU(rawchip_af_stats* af_stats);

int Yushan_Update_AEC_AWB_Params(rawchip_update_aec_awb_params_t *update_aec_awb_params);
int Yushan_Update_AF_Params(rawchip_update_af_params_t *update_af_params);
int Yushan_Update_3A_Params(rawchip_newframe_ack_enable_t enable_newframe_ack);

#endif
