/*
 *  linux/include/asm/setup.h
 *
 *  Copyright (C) 1997-1999 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Structure passed to kernel to tell it about the
 *  hardware it's running on.  See Documentation/arm/Setup
 *  for more info.
 */
#ifndef __ASMARM_SETUP_H
#define __ASMARM_SETUP_H

#include <linux/types.h>

#define COMMAND_LINE_SIZE 1024

extern unsigned int system_rev;

#define ATAG_NONE	0x00000000

struct tag_header {
	__u32 size;
	__u32 tag;
};

#define ATAG_CORE	0x54410001

struct tag_core {
	__u32 flags;		
	__u32 pagesize;
	__u32 rootdev;
};

#define ATAG_MEM	0x54410002

struct tag_mem32 {
	__u32	size;
	__u32	start;	
};

#define ATAG_VIDEOTEXT	0x54410003

struct tag_videotext {
	__u8		x;
	__u8		y;
	__u16		video_page;
	__u8		video_mode;
	__u8		video_cols;
	__u16		video_ega_bx;
	__u8		video_lines;
	__u8		video_isvga;
	__u16		video_points;
};

#define ATAG_RAMDISK	0x54410004

struct tag_ramdisk {
	__u32 flags;	
	__u32 size;	
	__u32 start;	
};

#define ATAG_INITRD	0x54410005

#define ATAG_INITRD2	0x54420005

struct tag_initrd {
	__u32 start;	
	__u32 size;	
};

#define ATAG_SERIAL	0x54410006

struct tag_serialnr {
	__u32 low;
	__u32 high;
};

#define ATAG_REVISION	0x54410007

struct tag_revision {
	__u32 rev;
	__u32 rev2;
};

#define ATAG_VIDEOLFB	0x54410008

struct tag_videolfb {
	__u16		lfb_width;
	__u16		lfb_height;
	__u16		lfb_depth;
	__u16		lfb_linelength;
	__u32		lfb_base;
	__u32		lfb_size;
	__u8		red_size;
	__u8		red_pos;
	__u8		green_size;
	__u8		green_pos;
	__u8		blue_size;
	__u8		blue_pos;
	__u8		rsvd_size;
	__u8		rsvd_pos;
};

#define ATAG_CMDLINE	0x54410009

struct tag_cmdline {
	char	cmdline[1];	
};

#define ATAG_ACORN	0x41000101

struct tag_acorn {
	__u32 memc_control_reg;
	__u32 vram_pages;
	__u8 sounddefault;
	__u8 adfsdrives;
};

#define ATAG_MEMCLK	0x41000402

struct tag_memclk {
	__u32 fmemclk;
};

#define ATAG_ALS	0x5441001b

struct tag_als_kadc {
	__u32 kadc;
};

#define ATAG_BLDR_LOG	0x54410024

struct tag_bldr_log {
	__u32 addr;
	__u32 size;
};

#define ATAG_LAST_BLDR_LOG	0x54410025

struct tag_last_bldr_log {
	__u32 addr;
	__u32 size;
};

struct tag {
	struct tag_header hdr;
	union {
		struct tag_core		core;
		struct tag_mem32	mem;
		struct tag_videotext	videotext;
		struct tag_ramdisk	ramdisk;
		struct tag_initrd	initrd;
		struct tag_serialnr	serialnr;
		struct tag_revision	revision;
		struct tag_videolfb	videolfb;
		struct tag_cmdline	cmdline;
		struct tag_als_kadc als_kadc;
		struct tag_acorn	acorn;

		struct tag_memclk	memclk;
		struct tag_bldr_log	bldr_log;
		struct tag_last_bldr_log last_bldr_log;
	} u;
};

struct tagtable {
	__u32 tag;
	int (*parse)(const struct tag *);
};

#define tag_member_present(tag,member)				\
	((unsigned long)(&((struct tag *)0L)->member + 1)	\
		<= (tag)->hdr.size * 4)

#define tag_next(t)	((struct tag *)((__u32 *)(t) + (t)->hdr.size))
#define tag_size(type)	((sizeof(struct tag_header) + sizeof(struct type)) >> 2)

#define for_each_tag(t,base)		\
	for (t = base; t->hdr.size; t = tag_next(t))

#ifdef __KERNEL__

#define __tag __used __attribute__((__section__(".taglist.init")))
#define __tagtable(tag, fn) \
static const struct tagtable __tagtable_##fn __tag = { tag, fn }

#define NR_BANKS	CONFIG_ARM_NR_BANKS

struct membank {
	phys_addr_t start;
	unsigned long size;
	unsigned int highmem;
};

struct meminfo {
	int nr_banks;
	struct membank bank[NR_BANKS];
};

extern struct meminfo meminfo;

#define for_each_bank(iter,mi)				\
	for (iter = 0; iter < (mi)->nr_banks; iter++)

#define bank_pfn_start(bank)	__phys_to_pfn((bank)->start)
#define bank_pfn_end(bank)	(__phys_to_pfn((bank)->start) + \
						__phys_to_pfn((bank)->size))
#define bank_pfn_size(bank)	((bank)->size >> PAGE_SHIFT)
#define bank_phys_start(bank)	(bank)->start
#define bank_phys_end(bank)	((bank)->start + (bank)->size)
#define bank_phys_size(bank)	(bank)->size

extern int arm_add_memory(phys_addr_t start, unsigned long size);
extern void early_print(const char *str, ...);
extern void dump_machine_table(void);

struct early_params {
	const char *arg;
	void (*fn)(char **p);
};

#define __early_param(name,fn)					\
static struct early_params __early_##fn __used			\
__attribute__((__section__(".early_param.init"))) = { name, fn }

#endif  

#endif
