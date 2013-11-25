/*
 *   Additional mixer mapping
 *
 *   Copyright (c) 2002 by Takashi Iwai <tiwai@suse.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

struct usbmix_dB_map {
	u32 min;
	u32 max;
};

struct usbmix_name_map {
	int id;
	const char *name;
	int control;
	struct usbmix_dB_map *dB;
};

struct usbmix_selector_map {
	int id;
	int count;
	const char **names;
};

struct usbmix_ctl_map {
	u32 id;
	const struct usbmix_name_map *map;
	const struct usbmix_selector_map *selector_map;
	int ignore_ctl_error;
};



static struct usbmix_name_map extigy_map[] = {
	
	{ 2, "PCM Playback" }, 
	
	
	{ 5, NULL }, 
	{ 6, "Digital In" }, 
	
	{ 8, "Line Playback" }, 
	
	{ 10, "Mic Playback" }, 
	{ 11, "Capture Source" }, 
	{ 12, "Capture" }, 
	
	
	
	
	{ 17, NULL, 1 }, 
	{ 17, "Channel Routing", 2 },	
	{ 18, "Tone Control - Bass", UAC_FU_BASS }, 
	{ 18, "Tone Control - Treble", UAC_FU_TREBLE }, 
	{ 18, "Master Playback" }, 
	
	
	{ 21, NULL }, 
	{ 22, "Digital Out Playback" }, 
	{ 23, "Digital Out1 Playback" },   
	
	{ 25, "IEC958 Optical Playback" }, 
	{ 26, "IEC958 Optical Playback" }, 
	{ 27, NULL }, 
	
	{ 29, NULL }, 
	{ 0 } 
};

static struct usbmix_dB_map mp3plus_dB_1 = {-4781, 0};	
static struct usbmix_dB_map mp3plus_dB_2 = {-1781, 618}; 

static struct usbmix_name_map mp3plus_map[] = {
	
	
	
	
	
	
	
	{ 8, "Capture Source" }, 
		
	{ 9, "Master Playback" }, 
	 
	{ 10,  NULL, 2, .dB = &mp3plus_dB_2 },
		
	{ 10, "Mic Boost", 7 }, 
	{ 11, "Line Capture", .dB = &mp3plus_dB_2 },
		
	{ 12, "Digital In Playback" }, 
	{ 13,  .dB = &mp3plus_dB_1 },
		
	{ 14, "Line Playback", .dB = &mp3plus_dB_1 }, 
	
	{ 0 } 
};

static struct usbmix_name_map audigy2nx_map[] = {
	
	
	{ 6, "Digital In Playback" }, 
	
	{ 8, "Line Playback" }, 
	{ 11, "What-U-Hear Capture" }, 
	{ 12, "Line Capture" }, 
	{ 13, "Digital In Capture" }, 
	{ 14, "Capture Source" }, 
	
	
	{ 17, NULL }, 
	{ 18, "Master Playback" }, 
	
	
	{ 21, NULL }, 
	{ 22, "Digital Out Playback" }, 
	{ 23, NULL }, 
	
	{ 27, NULL }, 
	{ 28, "Speaker Playback" }, 
	{ 29, "Digital Out Source" }, 
	{ 30, "Headphone Playback" }, 
	{ 31, "Headphone Source" }, 
	{ 0 } 
};

static struct usbmix_selector_map audigy2nx_selectors[] = {
	{
		.id = 14, 
		.count = 3,
		.names = (const char*[]) {"Line", "Digital In", "What-U-Hear"}
	},
	{
		.id = 29, 
		.count = 3,
		.names = (const char*[]) {"Front", "PCM", "Digital In"}
	},
	{
		.id = 31, 
		.count = 2,
		.names = (const char*[]) {"Front", "Side"}
	},
	{ 0 } 
};

static struct usbmix_name_map live24ext_map[] = {
	
	{ 5, "Mic Capture" }, 
	{ 0 } 
};

static struct usbmix_name_map linex_map[] = {
	
	 
	{ 3, "Master" }, 
	{ 0 } 
};

static struct usbmix_name_map maya44_map[] = {
	
	{ 2, "Line Playback" }, 
	
	{ 4, "Line Playback" }, 
	
	
	{ 7, "Master Playback" }, 
	
	
	{ 10, "Line Capture" }, 
	
	
	{ }
};


static struct usbmix_name_map justlink_map[] = {
	
	
	{ 3, NULL}, 
	
	
	
	{ 7, "Master Playback" }, 
	{ 8, NULL }, 
	{ 9, NULL }, 
	
	
	{ 0xc, NULL }, 
	{ 0 } 
};

static struct usbmix_name_map aureon_51_2_map[] = {
	
	
	
	
	
	
	
	{ 8, "Capture Source" }, 
	{ 9, "Master Playback" }, 
	{ 10, "Mic Capture" }, 
	{ 11, "Line Capture" }, 
	{ 12, "IEC958 In Capture" }, 
	{ 13, "Mic Playback" }, 
	{ 14, "Line Playback" }, 
	
	{} 
};

static struct usbmix_name_map scratch_live_map[] = {
	
	
	
	{ 4, "Line 1 In" }, 
	
	
	
	
	{ 9, "Line 2 In" }, 
	
	
	
	{ 0 } 
};

static struct usbmix_name_map hercules_usb51_map[] = {
	{ 8, "Capture Source" },	
	{ 9, "Master Playback" },	
	{ 10, "Mic Boost", 7 },		
	{ 11, "Line Capture" },		
	{ 13, "Mic Bypass Playback" },	
	{ 14, "Line Bypass Playback" },	
	{ 0 }				
};


static struct usbmix_ctl_map usbmix_ctl_maps[] = {
	{
		.id = USB_ID(0x041e, 0x3000),
		.map = extigy_map,
		.ignore_ctl_error = 1,
	},
	{
		.id = USB_ID(0x041e, 0x3010),
		.map = mp3plus_map,
	},
	{
		.id = USB_ID(0x041e, 0x3020),
		.map = audigy2nx_map,
		.selector_map = audigy2nx_selectors,
	},
 	{
		.id = USB_ID(0x041e, 0x3040),
		.map = live24ext_map,
	},
	{
		.id = USB_ID(0x041e, 0x3048),
		.map = audigy2nx_map,
		.selector_map = audigy2nx_selectors,
	},
	{
		
		.id = USB_ID(0x06f8, 0xb000),
		.ignore_ctl_error = 1,
	},
	{
		
		.id = USB_ID(0x06f8, 0xd002),
		.ignore_ctl_error = 1,
	},
	{
		.id = USB_ID(0x06f8, 0xc000),
		.map = hercules_usb51_map,
	},
	{
		.id = USB_ID(0x08bb, 0x2702),
		.map = linex_map,
		.ignore_ctl_error = 1,
	},
	{
		.id = USB_ID(0x0a92, 0x0091),
		.map = maya44_map,
	},
	{
		.id = USB_ID(0x0c45, 0x1158),
		.map = justlink_map,
	},
	{
		.id = USB_ID(0x0ccd, 0x0028),
		.map = aureon_51_2_map,
	},
	{
		.id = USB_ID(0x13e5, 0x0001),
		.map = scratch_live_map,
		.ignore_ctl_error = 1,
	},
	{ 0 } 
};

