/* powercolor-real-angel.h - Keytable for powercolor_real_angel Remote Controller
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


static struct rc_map_table powercolor_real_angel[] = {
	{ 0x38, KEY_SWITCHVIDEOMODE },	
	{ 0x0c, KEY_MEDIA },		
	{ 0x00, KEY_0 },
	{ 0x01, KEY_1 },
	{ 0x02, KEY_2 },
	{ 0x03, KEY_3 },
	{ 0x04, KEY_4 },
	{ 0x05, KEY_5 },
	{ 0x06, KEY_6 },
	{ 0x07, KEY_7 },
	{ 0x08, KEY_8 },
	{ 0x09, KEY_9 },
	{ 0x0a, KEY_DIGITS },		
	{ 0x29, KEY_PREVIOUS },		
	{ 0x12, KEY_BRIGHTNESSUP },
	{ 0x13, KEY_BRIGHTNESSDOWN },
	{ 0x2b, KEY_MODE },		
	{ 0x2c, KEY_TEXT },		
	{ 0x20, KEY_CHANNELUP },	
	{ 0x21, KEY_CHANNELDOWN },	
	{ 0x10, KEY_VOLUMEUP },		
	{ 0x11, KEY_VOLUMEDOWN },	
	{ 0x0d, KEY_MUTE },
	{ 0x1f, KEY_RECORD },
	{ 0x17, KEY_PLAY },
	{ 0x16, KEY_PAUSE },
	{ 0x0b, KEY_STOP },
	{ 0x27, KEY_FASTFORWARD },
	{ 0x26, KEY_REWIND },
	{ 0x1e, KEY_SEARCH },		
	{ 0x0e, KEY_CAMERA },		
	{ 0x2d, KEY_SETUP },
	{ 0x0f, KEY_SCREEN },		
	{ 0x14, KEY_RADIO },		
	{ 0x25, KEY_POWER },		
};

static struct rc_map_list powercolor_real_angel_map = {
	.map = {
		.scan    = powercolor_real_angel,
		.size    = ARRAY_SIZE(powercolor_real_angel),
		.rc_type = RC_TYPE_UNKNOWN,	
		.name    = RC_MAP_POWERCOLOR_REAL_ANGEL,
	}
};

static int __init init_rc_map_powercolor_real_angel(void)
{
	return rc_map_register(&powercolor_real_angel_map);
}

static void __exit exit_rc_map_powercolor_real_angel(void)
{
	rc_map_unregister(&powercolor_real_angel_map);
}

module_init(init_rc_map_powercolor_real_angel)
module_exit(exit_rc_map_powercolor_real_angel)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mauro Carvalho Chehab <mchehab@redhat.com>");
