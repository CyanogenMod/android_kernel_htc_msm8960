/* avermedia-dvbt.h - Keytable for avermedia_dvbt Remote Controller
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


static struct rc_map_table avermedia_dvbt[] = {
	{ 0x28, KEY_0 },		
	{ 0x22, KEY_1 },		
	{ 0x12, KEY_2 },		
	{ 0x32, KEY_3 },		
	{ 0x24, KEY_4 },		
	{ 0x14, KEY_5 },		
	{ 0x34, KEY_6 },		
	{ 0x26, KEY_7 },		
	{ 0x16, KEY_8 },		
	{ 0x36, KEY_9 },		

	{ 0x20, KEY_VIDEO },		
	{ 0x10, KEY_TEXT },		
	{ 0x00, KEY_POWER },		
	{ 0x04, KEY_AUDIO },		
	{ 0x06, KEY_ZOOM },		
	{ 0x18, KEY_SWITCHVIDEOMODE },	
	{ 0x38, KEY_SEARCH },		
	{ 0x08, KEY_INFO },		
	{ 0x2a, KEY_REWIND },		
	{ 0x1a, KEY_FASTFORWARD },	
	{ 0x3a, KEY_RECORD },		
	{ 0x0a, KEY_MUTE },		
	{ 0x2c, KEY_RECORD },		
	{ 0x1c, KEY_PAUSE },		
	{ 0x3c, KEY_STOP },		
	{ 0x0c, KEY_PLAY },		
	{ 0x2e, KEY_RED },		
	{ 0x01, KEY_BLUE },		
	{ 0x0e, KEY_YELLOW },		
	{ 0x21, KEY_GREEN },		
	{ 0x11, KEY_CHANNELDOWN },	
	{ 0x31, KEY_CHANNELUP },	
	{ 0x1e, KEY_VOLUMEDOWN },	
	{ 0x3e, KEY_VOLUMEUP },		
};

static struct rc_map_list avermedia_dvbt_map = {
	.map = {
		.scan    = avermedia_dvbt,
		.size    = ARRAY_SIZE(avermedia_dvbt),
		.rc_type = RC_TYPE_UNKNOWN,	
		.name    = RC_MAP_AVERMEDIA_DVBT,
	}
};

static int __init init_rc_map_avermedia_dvbt(void)
{
	return rc_map_register(&avermedia_dvbt_map);
}

static void __exit exit_rc_map_avermedia_dvbt(void)
{
	rc_map_unregister(&avermedia_dvbt_map);
}

module_init(init_rc_map_avermedia_dvbt)
module_exit(exit_rc_map_avermedia_dvbt)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mauro Carvalho Chehab <mchehab@redhat.com>");
