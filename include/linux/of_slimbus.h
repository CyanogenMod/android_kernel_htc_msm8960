/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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

#include <linux/slimbus/slimbus.h>
#include <linux/of_irq.h>

#ifdef CONFIG_OF_SLIMBUS
extern int of_register_slim_devices(struct slim_controller *ctrl);
#else
static int of_register_slim_devices(struct slim_controller *ctrl)
{
	return 0;
}
#endif 
