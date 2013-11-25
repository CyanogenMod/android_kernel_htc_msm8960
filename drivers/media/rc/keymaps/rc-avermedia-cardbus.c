/* avermedia-cardbus.h - Keytable for avermedia_cardbus Remote Controller
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


static struct rc_map_table avermedia_cardbus[] = {
	{ 0x00, KEY_POWER },
	{ 0x01, KEY_TUNER },		
	{ 0x03, KEY_TEXT },		
	{ 0x04, KEY_EPG },
	{ 0x05, KEY_1 },
	{ 0x06, KEY_2 },
	{ 0x07, KEY_3 },
	{ 0x08, KEY_AUDIO },
	{ 0x09, KEY_4 },
	{ 0x0a, KEY_5 },
	{ 0x0b, KEY_6 },
	{ 0x0c, KEY_ZOOM },		
	{ 0x0d, KEY_7 },
	{ 0x0e, KEY_8 },
	{ 0x0f, KEY_9 },
	{ 0x10, KEY_PAGEUP },		
	{ 0x11, KEY_0 },
	{ 0x12, KEY_INFO },
	{ 0x13, KEY_AGAIN },		
	{ 0x14, KEY_MUTE },
	{ 0x15, KEY_EDIT },		
	{ 0x17, KEY_SAVE },		
	{ 0x18, KEY_PLAYPAUSE },
	{ 0x19, KEY_RECORD },
	{ 0x1a, KEY_PLAY },
	{ 0x1b, KEY_STOP },
	{ 0x1c, KEY_FASTFORWARD },
	{ 0x1d, KEY_REWIND },
	{ 0x1e, KEY_VOLUMEDOWN },
	{ 0x1f, KEY_VOLUMEUP },
	{ 0x22, KEY_SLEEP },		
	{ 0x23, KEY_ZOOM },		
	{ 0x26, KEY_SCREEN },		
	{ 0x27, KEY_ANGLE },		
	{ 0x28, KEY_SELECT },		
	{ 0x29, KEY_BLUE },		
	{ 0x2a, KEY_BACKSPACE },	
	{ 0x2b, KEY_VIDEO },		
	{ 0x2c, KEY_DOWN },
	{ 0x2e, KEY_DOT },
	{ 0x2f, KEY_TV },		
	{ 0x32, KEY_LEFT },
	{ 0x33, KEY_CLEAR },		
	{ 0x35, KEY_RED },		
	{ 0x36, KEY_UP },
	{ 0x37, KEY_HOME },		
	{ 0x39, KEY_GREEN },		
	{ 0x3d, KEY_YELLOW },		
	{ 0x3e, KEY_OK },		
	{ 0x3f, KEY_RIGHT },
	{ 0x40, KEY_NEXT },		
	{ 0x41, KEY_PREVIOUS },		
	{ 0x42, KEY_CHANNELDOWN },	
	{ 0x43, KEY_CHANNELUP },	
};

static struct rc_map_list avermedia_cardbus_map = {
	.map = {
		.scan    = avermedia_cardbus,
		.size    = ARRAY_SIZE(avermedia_cardbus),
		.rc_type = RC_TYPE_UNKNOWN,	
		.name    = RC_MAP_AVERMEDIA_CARDBUS,
	}
};

static int __init init_rc_map_avermedia_cardbus(void)
{
	return rc_map_register(&avermedia_cardbus_map);
}

static void __exit exit_rc_map_avermedia_cardbus(void)
{
	rc_map_unregister(&avermedia_cardbus_map);
}

module_init(init_rc_map_avermedia_cardbus)
module_exit(exit_rc_map_avermedia_cardbus)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mauro Carvalho Chehab <mchehab@redhat.com>");
