/*
 * Copyright (C) 2007 HTC Incorporated
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __HTC_BATTERY_CELL_H__
#define __HTC_BATTERY_CELL_H__

enum htc_battery_cell_type {
	HTC_BATTERY_CELL_TYPE_NORMAL,
	HTC_BATTERY_CELL_TYPE_HV,
};

#define HTC_BATTERY_CELL_ID_UNKNOWN	(255)
#define HTC_BATTERY_CELL_ID_DEVELOP	(254)

struct htc_battery_cell {
	const char *model_name;
	const int	capacity;
	const int	id;
	const int	id_raw_min;
	const int	id_raw_max;
	const int	type;	
	const int	voltage_max;
	const int	voltage_min;
	void *chg_param;	
	void *gauge_param;	
};


int htc_battery_cell_init(struct htc_battery_cell *ary, int ary_aize);

struct htc_battery_cell *htc_battery_cell_find(int id_raw);

int htc_battery_cell_find_and_set_id_auto(int id_raw);

struct htc_battery_cell *htc_battery_cell_get_cur_cell(void);

struct htc_battery_cell *htc_battery_cell_set_cur_cell(
				struct htc_battery_cell *cell);

int htc_battery_cell_set_cur_cell_by_id(int id);

void *htc_battery_cell_get_cur_cell_charger_cdata(void);

void *htc_battery_cell_get_cur_cell_gauge_cdata(void);

int htc_battery_cell_hv_authenticated(void);

#endif
