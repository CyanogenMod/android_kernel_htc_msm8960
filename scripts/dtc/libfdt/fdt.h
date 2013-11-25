#ifndef _FDT_H
#define _FDT_H

#ifndef __ASSEMBLY__

struct fdt_header {
	uint32_t magic;			 
	uint32_t totalsize;		 
	uint32_t off_dt_struct;		 
	uint32_t off_dt_strings;	 
	uint32_t off_mem_rsvmap;	 
	uint32_t version;		 
	uint32_t last_comp_version;	 

	
	uint32_t boot_cpuid_phys;	 
	
	uint32_t size_dt_strings;	 

	
	uint32_t size_dt_struct;	 
};

struct fdt_reserve_entry {
	uint64_t address;
	uint64_t size;
};

struct fdt_node_header {
	uint32_t tag;
	char name[0];
};

struct fdt_property {
	uint32_t tag;
	uint32_t len;
	uint32_t nameoff;
	char data[0];
};

#endif 

#define FDT_MAGIC	0xd00dfeed	
#define FDT_TAGSIZE	sizeof(uint32_t)

#define FDT_BEGIN_NODE	0x1		
#define FDT_END_NODE	0x2		
#define FDT_PROP	0x3		
#define FDT_NOP		0x4		
#define FDT_END		0x9

#define FDT_V1_SIZE	(7*sizeof(uint32_t))
#define FDT_V2_SIZE	(FDT_V1_SIZE + sizeof(uint32_t))
#define FDT_V3_SIZE	(FDT_V2_SIZE + sizeof(uint32_t))
#define FDT_V16_SIZE	FDT_V3_SIZE
#define FDT_V17_SIZE	(FDT_V16_SIZE + sizeof(uint32_t))

#endif 
