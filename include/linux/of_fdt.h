/*
 * Definitions for working with the Flattened Device Tree data format
 *
 * Copyright 2009 Benjamin Herrenschmidt, IBM Corp
 * benh@kernel.crashing.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef _LINUX_OF_FDT_H
#define _LINUX_OF_FDT_H

#include <linux/types.h>
#include <linux/init.h>

#define OF_DT_HEADER		0xd00dfeed	
#define OF_DT_BEGIN_NODE	0x1		
#define OF_DT_END_NODE		0x2		
#define OF_DT_PROP		0x3		
#define OF_DT_NOP		0x4		
#define OF_DT_END		0x9

#define OF_DT_VERSION		0x10

#ifndef __ASSEMBLY__
struct boot_param_header {
	__be32	magic;			
	__be32	totalsize;		
	__be32	off_dt_struct;		
	__be32	off_dt_strings;		
	__be32	off_mem_rsvmap;		
	__be32	version;		
	__be32	last_comp_version;	
	
	__be32	boot_cpuid_phys;	
	
	__be32	dt_strings_size;	
	
	__be32	dt_struct_size;		
};

#if defined(CONFIG_OF_FLATTREE)

struct device_node;

extern char *of_fdt_get_string(struct boot_param_header *blob, u32 offset);
extern void *of_fdt_get_property(struct boot_param_header *blob,
				 unsigned long node,
				 const char *name,
				 unsigned long *size);
extern int of_fdt_is_compatible(struct boot_param_header *blob,
				unsigned long node,
				const char *compat);
extern int of_fdt_match(struct boot_param_header *blob, unsigned long node,
			const char *const *compat);
extern void of_fdt_unflatten_tree(unsigned long *blob,
			       struct device_node **mynodes);

extern int __initdata dt_root_addr_cells;
extern int __initdata dt_root_size_cells;
extern struct boot_param_header *initial_boot_params;

extern char *find_flat_dt_string(u32 offset);
extern int of_scan_flat_dt(int (*it)(unsigned long node, const char *uname,
				     int depth, void *data),
			   void *data);
extern void *of_get_flat_dt_prop(unsigned long node, const char *name,
				 unsigned long *size);
extern int of_flat_dt_is_compatible(unsigned long node, const char *name);
extern int of_flat_dt_match(unsigned long node, const char *const *matches);
extern unsigned long of_get_flat_dt_root(void);

extern int early_init_dt_scan_chosen(unsigned long node, const char *uname,
				     int depth, void *data);
extern void early_init_dt_check_for_initrd(unsigned long node);
extern int early_init_dt_scan_memory(unsigned long node, const char *uname,
				     int depth, void *data);
extern void early_init_dt_add_memory_arch(u64 base, u64 size);
extern void * early_init_dt_alloc_memory_arch(u64 size, u64 align);
extern u64 dt_mem_next_cell(int s, __be32 **cellp);

#ifdef CONFIG_BLK_DEV_INITRD
extern void early_init_dt_setup_initrd_arch(unsigned long start,
					    unsigned long end);
#endif

extern int early_init_dt_scan_root(unsigned long node, const char *uname,
				   int depth, void *data);

extern void unflatten_device_tree(void);
extern void early_init_devtree(void *);
#else 
static inline void unflatten_device_tree(void) {}
#endif 

#endif 
#endif 
