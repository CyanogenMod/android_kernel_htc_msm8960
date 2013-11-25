/* encore-enltv-fm53.h - Keytable for encore_enltv_fm53 Remote Controller
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


static struct rc_map_table encore_enltv_fm53[] = {
	{ 0x10, KEY_POWER2},
	{ 0x06, KEY_MUTE},

	{ 0x09, KEY_1},
	{ 0x1d, KEY_2},
	{ 0x1f, KEY_3},
	{ 0x19, KEY_4},
	{ 0x1b, KEY_5},
	{ 0x11, KEY_6},
	{ 0x17, KEY_7},
	{ 0x12, KEY_8},
	{ 0x16, KEY_9},
	{ 0x48, KEY_0},

	{ 0x04, KEY_LIST},		
	{ 0x40, KEY_LAST},		

	{ 0x02, KEY_MODE},		
	{ 0x05, KEY_CAMERA},		

	{ 0x4c, KEY_CHANNELUP},		
	{ 0x00, KEY_CHANNELDOWN},	
	{ 0x0d, KEY_VOLUMEUP},		
	{ 0x15, KEY_VOLUMEDOWN},	
	{ 0x49, KEY_ENTER},		

	{ 0x54, KEY_RECORD},
	{ 0x4d, KEY_PLAY},		

	{ 0x1e, KEY_MENU},		
	{ 0x0e, KEY_RIGHT},		
	{ 0x1a, KEY_LEFT},		

	{ 0x0a, KEY_CLEAR},		
	{ 0x0c, KEY_ZOOM},		
	{ 0x47, KEY_SLEEP},		
};

static struct rc_map_list encore_enltv_fm53_map = {
	.map = {
		.scan    = encore_enltv_fm53,
		.size    = ARRAY_SIZE(encore_enltv_fm53),
		.rc_type = RC_TYPE_UNKNOWN,	
		.name    = RC_MAP_ENCORE_ENLTV_FM53,
	}
};

static int __init init_rc_map_encore_enltv_fm53(void)
{
	return rc_map_register(&encore_enltv_fm53_map);
}

static void __exit exit_rc_map_encore_enltv_fm53(void)
{
	rc_map_unregister(&encore_enltv_fm53_map);
}

module_init(init_rc_map_encore_enltv_fm53)
module_exit(exit_rc_map_encore_enltv_fm53)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mauro Carvalho Chehab <mchehab@redhat.com>");
