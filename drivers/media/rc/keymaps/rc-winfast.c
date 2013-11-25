/* winfast.h - Keytable for winfast Remote Controller
 *
 * keymap imported from ir-keymaps.c
 *
 * Copyright (c) 2010 by Mauro Carvalho Chehab <mchehab@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <media/rc-map.h>
#include <linux/module.h>


static struct rc_map_table winfast[] = {
	
	{ 0x12, KEY_0 },
	{ 0x05, KEY_1 },
	{ 0x06, KEY_2 },
	{ 0x07, KEY_3 },
	{ 0x09, KEY_4 },
	{ 0x0a, KEY_5 },
	{ 0x0b, KEY_6 },
	{ 0x0d, KEY_7 },
	{ 0x0e, KEY_8 },
	{ 0x0f, KEY_9 },

	{ 0x00, KEY_POWER2 },
	{ 0x1b, KEY_AUDIO },		
	{ 0x02, KEY_TUNER },		
	{ 0x1e, KEY_VIDEO },		
	{ 0x16, KEY_INFO },		
	{ 0x04, KEY_RIGHT },
	{ 0x08, KEY_LEFT },
	{ 0x0c, KEY_UP },
	{ 0x10, KEY_DOWN },
	{ 0x03, KEY_ZOOM },		
	{ 0x1f, KEY_TEXT },		
	{ 0x20, KEY_SLEEP },
	{ 0x29, KEY_CLEAR },		
	{ 0x14, KEY_MUTE },
	{ 0x2b, KEY_RED },
	{ 0x2c, KEY_GREEN },
	{ 0x2d, KEY_YELLOW },
	{ 0x2e, KEY_BLUE },
	{ 0x18, KEY_KPPLUS },		
	{ 0x19, KEY_KPMINUS },		
	{ 0x2a, KEY_TV2 },		
	{ 0x21, KEY_DOT },
	{ 0x13, KEY_ENTER },
	{ 0x11, KEY_LAST },		
	{ 0x22, KEY_PREVIOUS },
	{ 0x23, KEY_PLAYPAUSE },
	{ 0x24, KEY_NEXT },
	{ 0x25, KEY_TIME },		
	{ 0x26, KEY_STOP },
	{ 0x27, KEY_RECORD },
	{ 0x28, KEY_CAMERA },		
	{ 0x2f, KEY_MENU },
	{ 0x30, KEY_CANCEL },
	{ 0x31, KEY_CHANNEL },		
	{ 0x32, KEY_SUBTITLE },
	{ 0x33, KEY_LANGUAGE },
	{ 0x34, KEY_REWIND },
	{ 0x35, KEY_FASTFORWARD },
	{ 0x36, KEY_TV },
	{ 0x37, KEY_RADIO },		
	{ 0x38, KEY_DVD },

	{ 0x1a, KEY_MODE},		
	{ 0x3e, KEY_VOLUMEUP },		
	{ 0x3a, KEY_VOLUMEDOWN },	
	{ 0x3b, KEY_CHANNELUP },	
	{ 0x3f, KEY_CHANNELDOWN }	
};

static struct rc_map_list winfast_map = {
	.map = {
		.scan    = winfast,
		.size    = ARRAY_SIZE(winfast),
		.rc_type = RC_TYPE_UNKNOWN,	
		.name    = RC_MAP_WINFAST,
	}
};

static int __init init_rc_map_winfast(void)
{
	return rc_map_register(&winfast_map);
}

static void __exit exit_rc_map_winfast(void)
{
	rc_map_unregister(&winfast_map);
}

module_init(init_rc_map_winfast)
module_exit(exit_rc_map_winfast)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mauro Carvalho Chehab <mchehab@redhat.com>");
