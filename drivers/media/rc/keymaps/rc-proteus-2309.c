/* proteus-2309.h - Keytable for proteus_2309 Remote Controller
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


static struct rc_map_table proteus_2309[] = {
	
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

	{ 0x5c, KEY_POWER },		
	{ 0x20, KEY_ZOOM },		
	{ 0x0f, KEY_BACKSPACE },	
	{ 0x1b, KEY_ENTER },		
	{ 0x41, KEY_RECORD },		
	{ 0x43, KEY_STOP },		
	{ 0x16, KEY_S },
	{ 0x1a, KEY_POWER2 },		
	{ 0x2e, KEY_RED },
	{ 0x1f, KEY_CHANNELDOWN },	
	{ 0x1c, KEY_CHANNELUP },	
	{ 0x10, KEY_VOLUMEDOWN },	
	{ 0x1e, KEY_VOLUMEUP },		
	{ 0x14, KEY_F1 },
};

static struct rc_map_list proteus_2309_map = {
	.map = {
		.scan    = proteus_2309,
		.size    = ARRAY_SIZE(proteus_2309),
		.rc_type = RC_TYPE_UNKNOWN,	
		.name    = RC_MAP_PROTEUS_2309,
	}
};

static int __init init_rc_map_proteus_2309(void)
{
	return rc_map_register(&proteus_2309_map);
}

static void __exit exit_rc_map_proteus_2309(void)
{
	rc_map_unregister(&proteus_2309_map);
}

module_init(init_rc_map_proteus_2309)
module_exit(exit_rc_map_proteus_2309)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mauro Carvalho Chehab <mchehab@redhat.com>");
