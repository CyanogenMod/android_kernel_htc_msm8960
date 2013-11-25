/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
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


#include <linux/module.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/sysrq.h>
#include <linux/time.h>
#include "linux/proc_fs.h"
#include "linux/kernel_stat.h"
#include "asm/uaccess.h"
#include <linux/proc_fs.h>
#include "cp15_registers.h"
#include <asm/perftypes.h>
#include "perf.h"

VPVF pp_interrupt_out_ptr;
VPVF pp_interrupt_in_ptr;
VPULF pp_process_remove_ptr;
unsigned int pp_loaded;
EXPORT_SYMBOL(pp_loaded);
atomic_t pm_op_lock;
EXPORT_SYMBOL(pm_op_lock);

void perf_mon_interrupt_out(void)
{
  if (pp_loaded)
	(*pp_interrupt_out_ptr)();
}
EXPORT_SYMBOL(pp_interrupt_out_ptr);

void perf_mon_interrupt_in(void)
{
  if (pp_loaded)
	(*pp_interrupt_in_ptr)();
}
EXPORT_SYMBOL(pp_interrupt_in_ptr);

void per_process_remove(unsigned long pid)
{
  if (pp_loaded)
		(*pp_process_remove_ptr)(pid);
}
EXPORT_SYMBOL(pp_process_remove_ptr);
