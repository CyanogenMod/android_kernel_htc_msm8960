/* asus-pc39.h - Keytable for asus_pc39 Remote Controller
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


static struct rc_map_table asus_pc39[] = {
	
	{ 0x082a, KEY_0 },
	{ 0x0816, KEY_1 },
	{ 0x0812, KEY_2 },
	{ 0x0814, KEY_3 },
	{ 0x0836, KEY_4 },
	{ 0x0832, KEY_5 },
	{ 0x0834, KEY_6 },
	{ 0x080e, KEY_7 },
	{ 0x080a, KEY_8 },
	{ 0x080c, KEY_9 },

	{ 0x0801, KEY_RADIO },		
	{ 0x083c, KEY_MENU },		
	{ 0x0815, KEY_VOLUMEUP },
	{ 0x0826, KEY_VOLUMEDOWN },
	{ 0x0808, KEY_UP },
	{ 0x0804, KEY_DOWN },
	{ 0x0818, KEY_LEFT },
	{ 0x0810, KEY_RIGHT },
	{ 0x081a, KEY_VIDEO },		
	{ 0x0806, KEY_AUDIO },		

	{ 0x081e, KEY_TV },		
	{ 0x0822, KEY_EXIT },		
	{ 0x0835, KEY_CHANNELUP },	
	{ 0x0824, KEY_CHANNELDOWN },	
	{ 0x0825, KEY_ENTER },		

	{ 0x0839, KEY_PAUSE },		
	{ 0x0821, KEY_PREVIOUS },		
	{ 0x0819, KEY_NEXT },		
	{ 0x0831, KEY_REWIND },		
	{ 0x0805, KEY_FASTFORWARD },	
	{ 0x0809, KEY_STOP },
	{ 0x0811, KEY_RECORD },		
	{ 0x0829, KEY_POWER },		

	{ 0x082e, KEY_ZOOM },		
	{ 0x082c, KEY_MACRO },		
	{ 0x081c, KEY_HOME },		
	{ 0x083a, KEY_PVR },		
	{ 0x0802, KEY_MUTE },		
	{ 0x083e, KEY_DVD },		
};

static struct rc_map_list asus_pc39_map = {
	.map = {
		.scan    = asus_pc39,
		.size    = ARRAY_SIZE(asus_pc39),
		.rc_type = RC_TYPE_RC5,
		.name    = RC_MAP_ASUS_PC39,
	}
};

static int __init init_rc_map_asus_pc39(void)
{
	return rc_map_register(&asus_pc39_map);
}

static void __exit exit_rc_map_asus_pc39(void)
{
	rc_map_unregister(&asus_pc39_map);
}

module_init(init_rc_map_asus_pc39)
module_exit(exit_rc_map_asus_pc39)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mauro Carvalho Chehab <mchehab@redhat.com>");
