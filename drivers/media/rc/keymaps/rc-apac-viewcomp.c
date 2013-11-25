/* apac-viewcomp.h - Keytable for apac_viewcomp Remote Controller
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


static struct rc_map_table apac_viewcomp[] = {

	{ 0x01, KEY_1 },
	{ 0x02, KEY_2 },
	{ 0x03, KEY_3 },
	{ 0x04, KEY_4 },
	{ 0x05, KEY_5 },
	{ 0x06, KEY_6 },
	{ 0x07, KEY_7 },
	{ 0x08, KEY_8 },
	{ 0x09, KEY_9 },
	{ 0x00, KEY_0 },
	{ 0x17, KEY_LAST },		
	{ 0x0a, KEY_LIST },		


	{ 0x1c, KEY_TUNER },		
	{ 0x15, KEY_SEARCH },		
	{ 0x12, KEY_POWER },		
	{ 0x1f, KEY_VOLUMEDOWN },	
	{ 0x1b, KEY_VOLUMEUP },		
	{ 0x1e, KEY_CHANNELDOWN },	
	{ 0x1a, KEY_CHANNELUP },	

	{ 0x11, KEY_VIDEO },		
	{ 0x0f, KEY_ZOOM },		
	{ 0x13, KEY_MUTE },		
	{ 0x10, KEY_TEXT },		

	{ 0x0d, KEY_STOP },		
	{ 0x0e, KEY_RECORD },		
	{ 0x1d, KEY_PLAYPAUSE },	
	{ 0x19, KEY_PLAY },		

	{ 0x16, KEY_GOTO },		
	{ 0x14, KEY_REFRESH },		
	{ 0x0c, KEY_KPPLUS },		
	{ 0x18, KEY_KPMINUS },		
};

static struct rc_map_list apac_viewcomp_map = {
	.map = {
		.scan    = apac_viewcomp,
		.size    = ARRAY_SIZE(apac_viewcomp),
		.rc_type = RC_TYPE_UNKNOWN,	
		.name    = RC_MAP_APAC_VIEWCOMP,
	}
};

static int __init init_rc_map_apac_viewcomp(void)
{
	return rc_map_register(&apac_viewcomp_map);
}

static void __exit exit_rc_map_apac_viewcomp(void)
{
	rc_map_unregister(&apac_viewcomp_map);
}

module_init(init_rc_map_apac_viewcomp)
module_exit(exit_rc_map_apac_viewcomp)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mauro Carvalho Chehab <mchehab@redhat.com>");
