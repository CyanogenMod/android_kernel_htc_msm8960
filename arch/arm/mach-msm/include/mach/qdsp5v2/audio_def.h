/* Copyright (c) 2009,2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _MACH_QDSP5_V2_AUDIO_DEF_H
#define _MACH_QDSP5_V2_AUDIO_DEF_H

#define SNDDEV_CAP_RX 0x1 
#define SNDDEV_CAP_TX 0x2 
#define SNDDEV_CAP_VOICE 0x4 
#define SNDDEV_CAP_PLAYBACK 0x8 
#define SNDDEV_CAP_FM 0x10 
#define SNDDEV_CAP_TTY 0x20 
#define SNDDEV_CAP_ANC 0x40 
#define SNDDEV_CAP_LB 0x80 
#define VOC_NB_INDEX	0
#define VOC_WB_INDEX	1
#define VOC_RX_VOL_ARRAY_NUM	2

#define SNDDEV_DEV_VOL_DIGITAL  0x1  
#define SNDDEV_DEV_VOL_ANALOG   0x2  

#define SIDE_TONE_MASK	0x01

#endif 
