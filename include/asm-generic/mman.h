#ifndef __ASM_GENERIC_MMAN_H
#define __ASM_GENERIC_MMAN_H

#include <asm-generic/mman-common.h>

#define MAP_GROWSDOWN	0x0100		
#define MAP_DENYWRITE	0x0800		
#define MAP_EXECUTABLE	0x1000		
#define MAP_LOCKED	0x2000		
#define MAP_NORESERVE	0x4000		
#define MAP_POPULATE	0x8000		
#define MAP_NONBLOCK	0x10000		
#define MAP_STACK	0x20000		
#define MAP_HUGETLB	0x40000		

#define MCL_CURRENT	1		
#define MCL_FUTURE	2		

#endif 
