/**
 * ldm - Part of the Linux-NTFS project.
 *
 * Copyright (C) 2001,2002 Richard Russon <ldm@flatcap.org>
 * Copyright (c) 2001-2007 Anton Altaparmakov
 * Copyright (C) 2001,2002 Jakob Kemi <jakob.kemi@telia.com>
 *
 * Documentation is available at http://www.linux-ntfs.org/doku.php?id=downloads 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the Linux-NTFS source
 * in the file COPYING); if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _FS_PT_LDM_H_
#define _FS_PT_LDM_H_

#include <linux/types.h>
#include <linux/list.h>
#include <linux/genhd.h>
#include <linux/fs.h>
#include <asm/unaligned.h>
#include <asm/byteorder.h>

struct parsed_partitions;

#define MAGIC_VMDB	0x564D4442		
#define MAGIC_VBLK	0x56424C4B		
#define MAGIC_PRIVHEAD	0x5052495648454144ULL	
#define MAGIC_TOCBLOCK	0x544F43424C4F434BULL	

#define VBLK_VOL5		0x51		
#define VBLK_CMP3		0x32		
#define VBLK_PRT3		0x33		
#define VBLK_DSK3		0x34		
#define VBLK_DSK4		0x44		
#define VBLK_DGR3		0x35		
#define VBLK_DGR4		0x45		

#define	VBLK_FLAG_COMP_STRIPE	0x10
#define	VBLK_FLAG_PART_INDEX	0x08
#define	VBLK_FLAG_DGR3_IDS	0x08
#define	VBLK_FLAG_DGR4_IDS	0x08
#define	VBLK_FLAG_VOLU_ID1	0x08
#define	VBLK_FLAG_VOLU_ID2	0x20
#define	VBLK_FLAG_VOLU_SIZE	0x80
#define	VBLK_FLAG_VOLU_DRIVE	0x02

#define VBLK_SIZE_HEAD		16
#define VBLK_SIZE_CMP3		22		
#define VBLK_SIZE_DGR3		12
#define VBLK_SIZE_DGR4		44
#define VBLK_SIZE_DSK3		12
#define VBLK_SIZE_DSK4		45
#define VBLK_SIZE_PRT3		28
#define VBLK_SIZE_VOL5		58

#define COMP_STRIPE		0x01		
#define COMP_BASIC		0x02		
#define COMP_RAID		0x03		

#define LDM_DB_SIZE		2048		

#define OFF_PRIV1		6		

#define OFF_PRIV2		1856		
#define OFF_PRIV3		2047

#define OFF_TOCB1		1		
#define OFF_TOCB2		2
#define OFF_TOCB3		2045
#define OFF_TOCB4		2046

#define OFF_VMDB		17		

#define LDM_PARTITION		0x42		

#define TOC_BITMAP1		"config"	
#define TOC_BITMAP2		"log"		

#define SYS_IND(p)		(get_unaligned(&(p)->sys_ind))

struct frag {				
	struct list_head list;
	u32		group;
	u8		num;		
	u8		rec;		
	u8		map;		
	u8		data[0];
};


#define GUID_SIZE		16

struct privhead {			
	u16	ver_major;
	u16	ver_minor;
	u64	logical_disk_start;
	u64	logical_disk_size;
	u64	config_start;
	u64	config_size;
	u8	disk_id[GUID_SIZE];
};

struct tocblock {			
	u8	bitmap1_name[16];
	u64	bitmap1_start;
	u64	bitmap1_size;
	u8	bitmap2_name[16];
	u64	bitmap2_start;
	u64	bitmap2_size;
};

struct vmdb {				
	u16	ver_major;
	u16	ver_minor;
	u32	vblk_size;
	u32	vblk_offset;
	u32	last_vblk_seq;
};

struct vblk_comp {			
	u8	state[16];
	u64	parent_id;
	u8	type;
	u8	children;
	u16	chunksize;
};

struct vblk_dgrp {			
	u8	disk_id[64];
};

struct vblk_disk {			
	u8	disk_id[GUID_SIZE];
	u8	alt_name[128];
};

struct vblk_part {			
	u64	start;
	u64	size;			
	u64	volume_offset;
	u64	parent_id;
	u64	disk_id;
	u8	partnum;
};

struct vblk_volu {			
	u8	volume_type[16];
	u8	volume_state[16];
	u8	guid[16];
	u8	drive_hint[4];
	u64	size;
	u8	partition_type;
};

struct vblk_head {			
	u32 group;
	u16 rec;
	u16 nrec;
};

struct vblk {				
	u8	name[64];
	u64	obj_id;
	u32	sequence;
	u8	flags;
	u8	type;
	union {
		struct vblk_comp comp;
		struct vblk_dgrp dgrp;
		struct vblk_disk disk;
		struct vblk_part part;
		struct vblk_volu volu;
	} vblk;
	struct list_head list;
};

struct ldmdb {				
	struct privhead ph;
	struct tocblock toc;
	struct vmdb     vm;
	struct list_head v_dgrp;
	struct list_head v_disk;
	struct list_head v_volu;
	struct list_head v_comp;
	struct list_head v_part;
};

int ldm_partition(struct parsed_partitions *state);

#endif 

