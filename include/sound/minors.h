#ifndef __SOUND_MINORS_H
#define __SOUND_MINORS_H

/*
 *  MINOR numbers
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#define SNDRV_OS_MINORS			256

#define SNDRV_MINOR_DEVICES		32
#define SNDRV_MINOR_CARD(minor)		((minor) >> 5)
#define SNDRV_MINOR_DEVICE(minor)	((minor) & 0x001f)
#define SNDRV_MINOR(card, dev)		(((card) << 5) | (dev))

#define SNDRV_MINOR_CONTROL		0	
#define SNDRV_MINOR_GLOBAL		1	
#define SNDRV_MINOR_SEQUENCER		1	
#define SNDRV_MINOR_TIMER		33	

#ifndef CONFIG_SND_DYNAMIC_MINORS
#define SNDRV_MINOR_COMPRESS		2	
#define SNDRV_MINOR_HWDEP		4	
#define SNDRV_MINOR_RAWMIDI		8	
#define SNDRV_MINOR_PCM_PLAYBACK	16	
#define SNDRV_MINOR_PCM_CAPTURE		24	

#define SNDRV_DEVICE_TYPE_CONTROL	SNDRV_MINOR_CONTROL
#define SNDRV_DEVICE_TYPE_HWDEP		SNDRV_MINOR_HWDEP
#define SNDRV_DEVICE_TYPE_RAWMIDI	SNDRV_MINOR_RAWMIDI
#define SNDRV_DEVICE_TYPE_PCM_PLAYBACK	SNDRV_MINOR_PCM_PLAYBACK
#define SNDRV_DEVICE_TYPE_PCM_CAPTURE	SNDRV_MINOR_PCM_CAPTURE
#define SNDRV_DEVICE_TYPE_SEQUENCER	SNDRV_MINOR_SEQUENCER
#define SNDRV_DEVICE_TYPE_TIMER		SNDRV_MINOR_TIMER
#define SNDRV_DEVICE_TYPE_COMPRESS	SNDRV_MINOR_COMPRESS

#else 

enum {
	SNDRV_DEVICE_TYPE_CONTROL,
	SNDRV_DEVICE_TYPE_SEQUENCER,
	SNDRV_DEVICE_TYPE_TIMER,
	SNDRV_DEVICE_TYPE_HWDEP,
	SNDRV_DEVICE_TYPE_RAWMIDI,
	SNDRV_DEVICE_TYPE_PCM_PLAYBACK,
	SNDRV_DEVICE_TYPE_PCM_CAPTURE,
	SNDRV_DEVICE_TYPE_COMPRESS,
};

#endif 

#define SNDRV_MINOR_HWDEPS		4
#define SNDRV_MINOR_RAWMIDIS		8
#define SNDRV_MINOR_PCMS		8


#ifdef CONFIG_SND_OSSEMUL

#define SNDRV_MINOR_OSS_DEVICES		16
#define SNDRV_MINOR_OSS_CARD(minor)	((minor) >> 4)
#define SNDRV_MINOR_OSS_DEVICE(minor)	((minor) & 0x000f)
#define SNDRV_MINOR_OSS(card, dev)	(((card) << 4) | (dev))

#define SNDRV_MINOR_OSS_MIXER		0	
#define SNDRV_MINOR_OSS_SEQUENCER	1	
#define	SNDRV_MINOR_OSS_MIDI		2	
#define SNDRV_MINOR_OSS_PCM		3	
#define SNDRV_MINOR_OSS_PCM_8		3	
#define SNDRV_MINOR_OSS_AUDIO		4	
#define SNDRV_MINOR_OSS_PCM_16		5	
#define SNDRV_MINOR_OSS_SNDSTAT		6	
#define SNDRV_MINOR_OSS_RESERVED7	7	
#define SNDRV_MINOR_OSS_MUSIC		8	
#define SNDRV_MINOR_OSS_DMMIDI		9	
#define SNDRV_MINOR_OSS_DMFM		10	
#define SNDRV_MINOR_OSS_MIXER1		11	
#define SNDRV_MINOR_OSS_PCM1		12	
#define SNDRV_MINOR_OSS_MIDI1		13	
#define SNDRV_MINOR_OSS_DMMIDI1		14	
#define SNDRV_MINOR_OSS_RESERVED15	15	

#define SNDRV_OSS_DEVICE_TYPE_MIXER	0
#define SNDRV_OSS_DEVICE_TYPE_SEQUENCER	1
#define SNDRV_OSS_DEVICE_TYPE_PCM	2
#define SNDRV_OSS_DEVICE_TYPE_MIDI	3
#define SNDRV_OSS_DEVICE_TYPE_DMFM	4
#define SNDRV_OSS_DEVICE_TYPE_SNDSTAT	5
#define SNDRV_OSS_DEVICE_TYPE_MUSIC	6

#define MODULE_ALIAS_SNDRV_MINOR(type) \
	MODULE_ALIAS("sound-service-?-" __stringify(type))

#endif

#endif 
