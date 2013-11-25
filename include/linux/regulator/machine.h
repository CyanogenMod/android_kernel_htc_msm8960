/*
 * machine.h -- SoC Regulator support, machine/board driver API.
 *
 * Copyright (C) 2007, 2008 Wolfson Microelectronics PLC.
 *
 * Author: Liam Girdwood <lrg@slimlogic.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Regulator Machine/Board Interface.
 */

#ifndef __LINUX_REGULATOR_MACHINE_H_
#define __LINUX_REGULATOR_MACHINE_H_

#include <linux/regulator/consumer.h>
#include <linux/suspend.h>

struct regulator;


#define REGULATOR_CHANGE_VOLTAGE	0x1
#define REGULATOR_CHANGE_CURRENT	0x2
#define REGULATOR_CHANGE_MODE		0x4
#define REGULATOR_CHANGE_STATUS		0x8
#define REGULATOR_CHANGE_DRMS		0x10

struct regulator_state {
	int uV;	
	unsigned int mode; 
	int enabled; 
	int disabled; 
};

struct regulation_constraints {

	const char *name;

	
	int min_uV;
	int max_uV;

	int uV_offset;

	
	int min_uA;
	int max_uA;

	
	unsigned int valid_modes_mask;

	
	unsigned int valid_ops_mask;

	
	int input_uV;

	
	struct regulator_state state_disk;
	struct regulator_state state_mem;
	struct regulator_state state_standby;
	suspend_state_t initial_state; 

	
	unsigned int initial_mode;

	
	unsigned always_on:1;	
	unsigned boot_on:1;	
	unsigned apply_uV:1;	
};

struct regulator_consumer_supply {
	const char *dev_name;   
	const char *supply;	
};

#define REGULATOR_SUPPLY(_name, _dev_name)			\
{								\
	.supply		= _name,				\
	.dev_name	= _dev_name,				\
}

struct regulator_init_data {
	const char *supply_regulator;        

	struct regulation_constraints constraints;

	int num_consumer_supplies;
	struct regulator_consumer_supply *consumer_supplies;

	
	int (*regulator_init)(void *driver_data);
	void *driver_data;	
};

int regulator_suspend_prepare(suspend_state_t state);
int regulator_suspend_finish(void);

#ifdef CONFIG_REGULATOR
void regulator_has_full_constraints(void);
void regulator_use_dummy_regulator(void);
void regulator_suppress_info_printing(void);
#else
static inline void regulator_has_full_constraints(void)
{
}

static inline void regulator_use_dummy_regulator(void)
{
}

static inline void regulator_suppress_info_printing(void)
{
}
#endif

#endif
