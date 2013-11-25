/* arch/arm/mach-msm/io_footprint.h
 * Copyright (C) 2012 HTC Corporation.
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


#ifndef __IO_FOOTPRINT_H
#define __IO_FOOTPRINT_H

#if defined(CONFIG_IO_FOOTPRINT)
extern void io_footprint_start(void *data);
extern void io_footprint_end(void);
#else
static inline void io_footprint_start(void *data) { return; }
static inline void io_footprint_end(void) { return; }
#endif

#endif
