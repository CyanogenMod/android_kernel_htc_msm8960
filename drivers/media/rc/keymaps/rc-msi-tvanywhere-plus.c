/* msi-tvanywhere-plus.h - Keytable for msi_tvanywhere_plus Remote Controller
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


static struct rc_map_table msi_tvanywhere_plus[] = {


	{ 0x01, KEY_1 },		
	{ 0x0b, KEY_2 },		
	{ 0x1b, KEY_3 },		
	{ 0x05, KEY_4 },		
	{ 0x09, KEY_5 },		
	{ 0x15, KEY_6 },		
	{ 0x06, KEY_7 },		
	{ 0x0a, KEY_8 },		
	{ 0x12, KEY_9 },		
	{ 0x02, KEY_0 },		
	{ 0x10, KEY_KPPLUS },		
	{ 0x13, KEY_AGAIN },		

	{ 0x1e, KEY_POWER },		
	{ 0x07, KEY_VIDEO },		
	{ 0x1c, KEY_SEARCH },		
	{ 0x18, KEY_MUTE },		

	{ 0x03, KEY_RADIO },		
	{ 0x3f, KEY_RIGHT },		
	{ 0x37, KEY_LEFT },		
	{ 0x2c, KEY_UP },		
	{ 0x24, KEY_DOWN },		

	{ 0x00, KEY_RECORD },		
	{ 0x08, KEY_STOP },		
	{ 0x11, KEY_PLAY },		

	{ 0x0f, KEY_CLOSE },		
	{ 0x19, KEY_ZOOM },		
	{ 0x1a, KEY_CAMERA },		
	{ 0x0d, KEY_LANGUAGE },		

	{ 0x14, KEY_VOLUMEDOWN },	
	{ 0x16, KEY_VOLUMEUP },		
	{ 0x17, KEY_CHANNELDOWN },	
	{ 0x1f, KEY_CHANNELUP },	

	{ 0x04, KEY_REWIND },		
	{ 0x0e, KEY_MENU },		
	{ 0x0c, KEY_FASTFORWARD },	
	{ 0x1d, KEY_RESTART },		
};

static struct rc_map_list msi_tvanywhere_plus_map = {
	.map = {
		.scan    = msi_tvanywhere_plus,
		.size    = ARRAY_SIZE(msi_tvanywhere_plus),
		.rc_type = RC_TYPE_UNKNOWN,	
		.name    = RC_MAP_MSI_TVANYWHERE_PLUS,
	}
};

static int __init init_rc_map_msi_tvanywhere_plus(void)
{
	return rc_map_register(&msi_tvanywhere_plus_map);
}

static void __exit exit_rc_map_msi_tvanywhere_plus(void)
{
	rc_map_unregister(&msi_tvanywhere_plus_map);
}

module_init(init_rc_map_msi_tvanywhere_plus)
module_exit(exit_rc_map_msi_tvanywhere_plus)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mauro Carvalho Chehab <mchehab@redhat.com>");
