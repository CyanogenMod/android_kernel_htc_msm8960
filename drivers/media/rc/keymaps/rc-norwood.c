/* norwood.h - Keytable for norwood Remote Controller
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


static struct rc_map_table norwood[] = {
	
	{ 0x20, KEY_0 },
	{ 0x21, KEY_1 },
	{ 0x22, KEY_2 },
	{ 0x23, KEY_3 },
	{ 0x24, KEY_4 },
	{ 0x25, KEY_5 },
	{ 0x26, KEY_6 },
	{ 0x27, KEY_7 },
	{ 0x28, KEY_8 },
	{ 0x29, KEY_9 },

	{ 0x78, KEY_VIDEO },		
	{ 0x2c, KEY_EXIT },		
	{ 0x2a, KEY_SELECT },		
	{ 0x69, KEY_AGAIN },		

	{ 0x32, KEY_BRIGHTNESSUP },	
	{ 0x33, KEY_BRIGHTNESSDOWN },	
	{ 0x6b, KEY_KPPLUS },		
	{ 0x6c, KEY_KPMINUS },		

	{ 0x2d, KEY_MUTE },		
	{ 0x30, KEY_VOLUMEUP },		
	{ 0x31, KEY_VOLUMEDOWN },	
	{ 0x60, KEY_CHANNELUP },	
	{ 0x61, KEY_CHANNELDOWN },	

	{ 0x3f, KEY_RECORD },		
	{ 0x37, KEY_PLAY },		
	{ 0x36, KEY_PAUSE },		
	{ 0x2b, KEY_STOP },		
	{ 0x67, KEY_FASTFORWARD },	
	{ 0x66, KEY_REWIND },		
	{ 0x3e, KEY_SEARCH },		
	{ 0x2e, KEY_CAMERA },		
	{ 0x6d, KEY_MENU },		
	{ 0x2f, KEY_ZOOM },		
	{ 0x34, KEY_RADIO },		
	{ 0x65, KEY_POWER },		
};

static struct rc_map_list norwood_map = {
	.map = {
		.scan    = norwood,
		.size    = ARRAY_SIZE(norwood),
		.rc_type = RC_TYPE_UNKNOWN,	
		.name    = RC_MAP_NORWOOD,
	}
};

static int __init init_rc_map_norwood(void)
{
	return rc_map_register(&norwood_map);
}

static void __exit exit_rc_map_norwood(void)
{
	rc_map_unregister(&norwood_map);
}

module_init(init_rc_map_norwood)
module_exit(exit_rc_map_norwood)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mauro Carvalho Chehab <mchehab@redhat.com>");
