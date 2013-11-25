/*
 * DigitalNow TinyTwin remote controller keytable
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

static struct rc_map_table digitalnow_tinytwin[] = {
	{ 0x0000, KEY_MUTE },            
	{ 0x0001, KEY_VOLUMEUP },
	{ 0x0002, KEY_POWER2 },          
	{ 0x0003, KEY_2 },
	{ 0x0004, KEY_3 },
	{ 0x0005, KEY_4 },
	{ 0x0006, KEY_6 },
	{ 0x0007, KEY_7 },
	{ 0x0008, KEY_8 },
	{ 0x0009, KEY_NUMERIC_STAR },    
	{ 0x000a, KEY_0 },
	{ 0x000b, KEY_NUMERIC_POUND },   
	{ 0x000c, KEY_RIGHT },           
	{ 0x000d, KEY_HOMEPAGE },        
	{ 0x000e, KEY_RED },             
	{ 0x0010, KEY_POWER },           
	{ 0x0011, KEY_YELLOW },          
	{ 0x0012, KEY_DOWN },            
	{ 0x0013, KEY_GREEN },           
	{ 0x0014, KEY_CYCLEWINDOWS },    
	{ 0x0015, KEY_FAVORITES },       
	{ 0x0016, KEY_UP },              
	{ 0x0017, KEY_LEFT },            
	{ 0x0018, KEY_OK },              
	{ 0x0019, KEY_BLUE },            
	{ 0x001a, KEY_REWIND },          
	{ 0x001b, KEY_PLAY },            
	{ 0x001c, KEY_5 },
	{ 0x001d, KEY_9 },
	{ 0x001e, KEY_VOLUMEDOWN },
	{ 0x001f, KEY_1 },
	{ 0x0040, KEY_STOP },            
	{ 0x0042, KEY_PAUSE },           
	{ 0x0043, KEY_SCREEN },          
	{ 0x0044, KEY_FORWARD },         
	{ 0x0045, KEY_NEXT },            
	{ 0x0048, KEY_RECORD },          
	{ 0x0049, KEY_VIDEO },           
	{ 0x004a, KEY_EPG },             
	{ 0x004b, KEY_CHANNELUP },
	{ 0x004c, KEY_HELP },            
	{ 0x004d, KEY_RADIO },           
	{ 0x004f, KEY_CHANNELDOWN },
	{ 0x0050, KEY_DVD },             
	{ 0x0051, KEY_AUDIO },           
	{ 0x0052, KEY_TITLE },           
	{ 0x0053, KEY_NEW },             
	{ 0x0057, KEY_MENU },            
	{ 0x005a, KEY_PREVIOUS },        
};

static struct rc_map_list digitalnow_tinytwin_map = {
	.map = {
		.scan    = digitalnow_tinytwin,
		.size    = ARRAY_SIZE(digitalnow_tinytwin),
		.rc_type = RC_TYPE_NEC,
		.name    = RC_MAP_DIGITALNOW_TINYTWIN,
	}
};

static int __init init_rc_map_digitalnow_tinytwin(void)
{
	return rc_map_register(&digitalnow_tinytwin_map);
}

static void __exit exit_rc_map_digitalnow_tinytwin(void)
{
	rc_map_unregister(&digitalnow_tinytwin_map);
}

module_init(init_rc_map_digitalnow_tinytwin)
module_exit(exit_rc_map_digitalnow_tinytwin)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Antti Palosaari <crope@iki.fi>");
