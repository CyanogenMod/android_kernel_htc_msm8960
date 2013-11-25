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



#ifndef _SPS_MAP_H_
#define _SPS_MAP_H_

#include <linux/types.h>	

struct sps_map_end_point {
	u32 periph_class;	
	u32 periph_phy_addr;	
	u32 pipe_index;		
	u32 event_thresh;	
};

struct sps_map {
	
	struct sps_map_end_point src;

	
	struct sps_map_end_point dest;

	
	u32 config;	 
	u32 desc_base;	 
	u32 desc_size;	 
	u32 data_base;	 
	u32 data_size;	 

};

#endif 
