/*
 * TwinHan AzureWave AD-TU700(704J) remote controller keytable
 *
 * Copyright (C) 2010 Antti Palosaari <crope@iki.fi>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <media/rc-map.h>
#include <linux/module.h>

static struct rc_map_table azurewave_ad_tu700[] = {
	{ 0x0000, KEY_TAB },             
	{ 0x0001, KEY_2 },
	{ 0x0002, KEY_CHANNELDOWN },
	{ 0x0003, KEY_1 },
	{ 0x0004, KEY_MENU },            
	{ 0x0005, KEY_CHANNELUP },
	{ 0x0006, KEY_3 },
	{ 0x0007, KEY_SLEEP },           
	{ 0x0008, KEY_VIDEO },           
	{ 0x0009, KEY_4 },
	{ 0x000a, KEY_VOLUMEDOWN },
	{ 0x000c, KEY_CANCEL },          
	{ 0x000d, KEY_7 },
	{ 0x000e, KEY_AGAIN },           
	{ 0x000f, KEY_TEXT },            
	{ 0x0010, KEY_MUTE },
	{ 0x0011, KEY_RECORD },
	{ 0x0012, KEY_FASTFORWARD },     
	{ 0x0013, KEY_BACK },            
	{ 0x0014, KEY_PLAY },
	{ 0x0015, KEY_0 },
	{ 0x0016, KEY_POWER2 },          
	{ 0x0017, KEY_FAVORITES },       
	{ 0x0018, KEY_RED },
	{ 0x0019, KEY_8 },
	{ 0x001a, KEY_STOP },
	{ 0x001b, KEY_9 },
	{ 0x001c, KEY_EPG },             
	{ 0x001d, KEY_5 },
	{ 0x001e, KEY_VOLUMEUP },
	{ 0x001f, KEY_6 },
	{ 0x0040, KEY_REWIND },          
	{ 0x0041, KEY_PREVIOUS },        
	{ 0x0042, KEY_NEXT },            
	{ 0x0043, KEY_SUBTITLE },        
	{ 0x0045, KEY_KPPLUS },          
	{ 0x0046, KEY_KPMINUS },         
	{ 0x0047, KEY_NEW },             
	{ 0x0048, KEY_INFO },            
	{ 0x0049, KEY_MODE },            
	{ 0x004a, KEY_CLEAR },           
	{ 0x004b, KEY_UP },              
	{ 0x004c, KEY_PAUSE },
	{ 0x004d, KEY_ZOOM },            
	{ 0x004e, KEY_LEFT },            
	{ 0x004f, KEY_OK },              
	{ 0x0050, KEY_LANGUAGE },        
	{ 0x0051, KEY_DOWN },            
	{ 0x0052, KEY_RIGHT },           
	{ 0x0053, KEY_GREEN },
	{ 0x0054, KEY_CAMERA },          
	{ 0x005e, KEY_YELLOW },
	{ 0x005f, KEY_BLUE },
};

static struct rc_map_list azurewave_ad_tu700_map = {
	.map = {
		.scan    = azurewave_ad_tu700,
		.size    = ARRAY_SIZE(azurewave_ad_tu700),
		.rc_type = RC_TYPE_NEC,
		.name    = RC_MAP_AZUREWAVE_AD_TU700,
	}
};

static int __init init_rc_map_azurewave_ad_tu700(void)
{
	return rc_map_register(&azurewave_ad_tu700_map);
}

static void __exit exit_rc_map_azurewave_ad_tu700(void)
{
	rc_map_unregister(&azurewave_ad_tu700_map);
}

module_init(init_rc_map_azurewave_ad_tu700)
module_exit(exit_rc_map_azurewave_ad_tu700)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Antti Palosaari <crope@iki.fi>");
