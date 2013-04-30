/* arch/arm/mach-msm/htc_battery_cell.c
 *
 * Copyright (C) 2011 HTC Corporation.
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <mach/htc_battery_cell.h>

static struct htc_battery_cell *cells;	
static int cell_num;					
static struct htc_battery_cell *cur_cell; 

static unsigned int hv_authenticated;

static struct htc_battery_cell default_cell = {
	.model_name = "DEVELOPMENT",
	.capacity = 1234,
	.id = HTC_BATTERY_CELL_ID_DEVELOP,
	.id_raw_min = INT_MIN,
	.id_raw_max = INT_MAX,
	.type = HTC_BATTERY_CELL_TYPE_NORMAL,
	.voltage_max = 4200,
	.voltage_min = 3200,
	.chg_param = NULL,
	.gauge_param = NULL,
};

int htc_battery_cell_init(struct htc_battery_cell *ary, int ary_size)
{
	if (ary == NULL || ary_size == 0) {
		pr_warn("[BATT] %s: default cell initiated\n", __func__);
		cells = &default_cell;
		cell_num = 1;
		return 1;
	}

	cells = ary;
	cell_num = ary_size;
	pr_info("[BATT] %s: %d cells initiated.\n", __func__, cell_num);
	return 0;
}

inline struct htc_battery_cell *htc_battery_cell_get_cur_cell(void)
{
	return cur_cell;
}

inline struct htc_battery_cell *htc_battery_cell_set_cur_cell(
			struct htc_battery_cell *cell)
{
	cur_cell = cell;
	return cur_cell;
}

inline int htc_battery_cell_set_cur_cell_by_id(int id)
{
	int i = 0;
	struct htc_battery_cell *pcell = NULL;

	pr_debug("%s: find id=%d\n", __func__, id);
	for (i = 0; i < cell_num; i++) {
		pcell = &cells[i];
		pr_debug("    cell[%d].id=%d\n", i, pcell->id);
		if (pcell->id == id) {
			pr_info("%s: set_cur_cell(id=%d)\n", __func__, id);
			cur_cell = pcell;
			return 0;
		}
	}
	pr_err("[BATT] %s: cell id=%d cannot be found\n", __func__, id);
	return 1;
}

inline void *htc_battery_cell_get_cur_cell_charger_cdata(void)
{
	if (cur_cell)
		return cur_cell->chg_param;
	return NULL;
}

inline void *htc_battery_cell_get_cur_cell_gauge_cdata(void)
{
	if (cur_cell)
		return cur_cell->gauge_param;
	return NULL;
}

inline struct htc_battery_cell *htc_battery_cell_find(int id_raw)
{
	int i = 0;
	struct htc_battery_cell *pcell = NULL;

	pr_debug("%s: find id_raw=%d\n", __func__, id_raw);
	for (i = 0; i < cell_num; i++) {
		pcell = &cells[i];
		pr_debug("    i=%d, (min,max)=(%d,%d)\n",
				i, pcell->id_raw_min, pcell->id_raw_max);
		if ((pcell->id_raw_min <= id_raw)
				&& (id_raw <= pcell->id_raw_max))
			return pcell;
	}
	pr_err("[BATT] %s: cell id can not be identified (id_raw=%d)\n",
			__func__, id_raw);
	
	return pcell;
}

#define HTC_BATTERY_CELL_CHECK_UNKNOWN_COUNT	(2)
inline int htc_battery_cell_find_and_set_id_auto(int id_raw)
{
	static int unknown_count = 0;
	struct htc_battery_cell *pcell = NULL;

	pcell = htc_battery_cell_find(id_raw);
	if (!pcell) {
		pr_err("[BATT] cell pointer is NULL so unknown ID is return.\n");
		return HTC_BATTERY_CELL_ID_UNKNOWN;
	}
	
	if (cur_cell == pcell)
		return pcell->id;
	
	if (cur_cell) {
		if (pcell->id == HTC_BATTERY_CELL_ID_UNKNOWN) {
			unknown_count++;
			if (unknown_count < HTC_BATTERY_CELL_CHECK_UNKNOWN_COUNT)
				return cur_cell->id; 
		} else
			unknown_count = 0;
	} else {
		
		pr_warn("[BATT]warn: cur_cell is initiated by %s", __func__);
		cur_cell = pcell;
		return pcell->id;
	}
	pr_debug("[BATT]dbg: battery cell id %d -> %d\n",
			cur_cell->id, pcell->id);
	cur_cell = pcell;
	return pcell->id;
}

inline int htc_battery_cell_hv_authenticated(void)
{
	return hv_authenticated;
}

static int __init check_dq_setup(char *str)
{
	if (!strcmp(str, "PASS")) {
		hv_authenticated = 1;
		pr_info("[BATT] HV authentication passed.\n");
	} else {
		hv_authenticated = 0;
		pr_info("[BATT] HV authentication failed.\n");
	}
	return 0; 
}
__setup("androidboot.dq=", check_dq_setup);


