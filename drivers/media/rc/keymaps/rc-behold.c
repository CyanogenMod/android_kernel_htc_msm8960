/* behold.h - Keytable for behold Remote Controller
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


static struct rc_map_table behold[] = {

	{ 0x6b861c, KEY_TUNER },	
	{ 0x6b8612, KEY_POWER },

	{ 0x6b8601, KEY_1 },
	{ 0x6b8602, KEY_2 },
	{ 0x6b8603, KEY_3 },
	{ 0x6b8604, KEY_4 },
	{ 0x6b8605, KEY_5 },
	{ 0x6b8606, KEY_6 },
	{ 0x6b8607, KEY_7 },
	{ 0x6b8608, KEY_8 },
	{ 0x6b8609, KEY_9 },

	{ 0x6b860a, KEY_AGAIN },
	{ 0x6b8600, KEY_0 },
	{ 0x6b8617, KEY_MODE },

	{ 0x6b8614, KEY_SCREEN },
	{ 0x6b8610, KEY_ZOOM },

	{ 0x6b860b, KEY_CHANNELUP },
	{ 0x6b8618, KEY_VOLUMEDOWN },
	{ 0x6b8616, KEY_OK },		
	{ 0x6b860c, KEY_VOLUMEUP },
	{ 0x6b8615, KEY_CHANNELDOWN },

	{ 0x6b8611, KEY_MUTE },
	{ 0x6b860d, KEY_INFO },

	{ 0x6b860f, KEY_RECORD },
	{ 0x6b861b, KEY_PLAYPAUSE },
	{ 0x6b861a, KEY_STOP },
	{ 0x6b860e, KEY_TEXT },
	{ 0x6b861f, KEY_RED },	
	{ 0x6b861e, KEY_VIDEO },

	{ 0x6b861d, KEY_SLEEP },
	{ 0x6b8613, KEY_GREEN },
	{ 0x6b8619, KEY_BLUE },	

	{ 0x6b8658, KEY_SLOW },
	{ 0x6b865c, KEY_CAMERA },

};

static struct rc_map_list behold_map = {
	.map = {
		.scan    = behold,
		.size    = ARRAY_SIZE(behold),
		.rc_type = RC_TYPE_NEC,
		.name    = RC_MAP_BEHOLD,
	}
};

static int __init init_rc_map_behold(void)
{
	return rc_map_register(&behold_map);
}

static void __exit exit_rc_map_behold(void)
{
	rc_map_unregister(&behold_map);
}

module_init(init_rc_map_behold)
module_exit(exit_rc_map_behold)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mauro Carvalho Chehab <mchehab@redhat.com>");
