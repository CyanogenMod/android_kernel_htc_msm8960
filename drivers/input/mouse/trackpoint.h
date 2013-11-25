/*
 * IBM TrackPoint PS/2 mouse driver
 *
 * Stephen Evanchik <evanchsa@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#ifndef _TRACKPOINT_H
#define _TRACKPOINT_H


#define TP_COMMAND		0xE2	

#define TP_READ_ID		0xE1	
#define TP_MAGIC_IDENT		0x01	
					


#define TP_RECALIB		0x51	
#define TP_POWER_DOWN		0x44	
#define TP_EXT_DEV		0x21	
#define TP_EXT_BTN		0x4B	
#define TP_POR			0x7F	
#define TP_POR_RESULTS		0x25	
#define TP_DISABLE_EXT		0x40	
#define TP_ENABLE_EXT		0x41	

#define TP_SET_SOFT_TRANS	0x4E	
#define TP_CANCEL_SOFT_TRANS	0xB9	
#define TP_SET_HARD_TRANS	0x45	


#define TP_WRITE_MEM		0x81
#define TP_READ_MEM		0x80	

#define TP_SENS			0x4A	
#define TP_MB			0x4C	
#define TP_INERTIA		0x4D	
#define TP_SPEED		0x60	
#define TP_REACH		0x57	
#define TP_DRAGHYS		0x58	
					
					

#define TP_MINDRAG		0x59	
					

#define TP_THRESH		0x5C	
#define TP_UP_THRESH		0x5A	
#define TP_Z_TIME		0x5E	
#define TP_JENKS_CURV		0x5D	

#define TP_TOGGLE		0x47	

#define TP_TOGGLE_MB		0x23	
#define TP_MASK_MB			0x01
#define TP_TOGGLE_EXT_DEV	0x23	
#define TP_MASK_EXT_DEV			0x02
#define TP_TOGGLE_DRIFT		0x23	
#define TP_MASK_DRIFT			0x80
#define TP_TOGGLE_BURST		0x28	
#define TP_MASK_BURST			0x80
#define TP_TOGGLE_PTSON		0x2C	
#define TP_MASK_PTSON			0x01
#define TP_TOGGLE_HARD_TRANS	0x2C	
#define TP_MASK_HARD_TRANS		0x80
#define TP_TOGGLE_TWOHAND	0x2D	
#define TP_MASK_TWOHAND			0x01
#define TP_TOGGLE_STICKY_TWO	0x2D	
#define TP_MASK_STICKY_TWO		0x04
#define TP_TOGGLE_SKIPBACK	0x2D	
#define TP_MASK_SKIPBACK		0x08
#define TP_TOGGLE_SOURCE_TAG	0x20	
#define TP_MASK_SOURCE_TAG		0x80
#define TP_TOGGLE_EXT_TAG	0x22	
#define TP_MASK_EXT_TAG			0x04


#define TP_POR_SUCCESS		0x3B

#define TP_DEF_SENS		0x80
#define TP_DEF_INERTIA		0x06
#define TP_DEF_SPEED		0x61
#define TP_DEF_REACH		0x0A

#define TP_DEF_DRAGHYS		0xFF
#define TP_DEF_MINDRAG		0x14

#define TP_DEF_THRESH		0x08
#define TP_DEF_UP_THRESH	0xFF
#define TP_DEF_Z_TIME		0x26
#define TP_DEF_JENKS_CURV	0x87

#define TP_DEF_MB		0x00
#define TP_DEF_PTSON		0x00
#define TP_DEF_SKIPBACK		0x00
#define TP_DEF_EXT_DEV		0x00	

#define MAKE_PS2_CMD(params, results, cmd) ((params<<12) | (results<<8) | (cmd))

struct trackpoint_data
{
	unsigned char sensitivity, speed, inertia, reach;
	unsigned char draghys, mindrag;
	unsigned char thresh, upthresh;
	unsigned char ztime, jenks;

	unsigned char press_to_select;
	unsigned char skipback;

	unsigned char ext_dev;
};

#ifdef CONFIG_MOUSE_PS2_TRACKPOINT
int trackpoint_detect(struct psmouse *psmouse, bool set_properties);
#else
inline int trackpoint_detect(struct psmouse *psmouse, bool set_properties)
{
	return -ENOSYS;
}
#endif 

#endif 
