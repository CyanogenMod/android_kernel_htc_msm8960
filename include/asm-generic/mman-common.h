#ifndef __ASM_GENERIC_MMAN_COMMON_H
#define __ASM_GENERIC_MMAN_COMMON_H


#define PROT_READ	0x1		
#define PROT_WRITE	0x2		/* page can be written */
#define PROT_EXEC	0x4		
#define PROT_SEM	0x8		
#define PROT_NONE	0x0		
#define PROT_GROWSDOWN	0x01000000	
#define PROT_GROWSUP	0x02000000	

#define MAP_SHARED	0x01		
#define MAP_PRIVATE	0x02		
#define MAP_TYPE	0x0f		
#define MAP_FIXED	0x10		
#define MAP_ANONYMOUS	0x20		
#ifdef CONFIG_MMAP_ALLOW_UNINITIALIZED
# define MAP_UNINITIALIZED 0x4000000	
#else
# define MAP_UNINITIALIZED 0x0		
#endif

#define MS_ASYNC	1		
#define MS_INVALIDATE	2		
#define MS_SYNC		4		

#define MADV_NORMAL	0		
#define MADV_RANDOM	1		
#define MADV_SEQUENTIAL	2		
#define MADV_WILLNEED	3		
#define MADV_DONTNEED	4		

#define MADV_REMOVE	9		
#define MADV_DONTFORK	10		
#define MADV_DOFORK	11		
#define MADV_HWPOISON	100		
#define MADV_SOFT_OFFLINE 101		

#define MADV_MERGEABLE   12		
#define MADV_UNMERGEABLE 13		

#define MADV_HUGEPAGE	14		
#define MADV_NOHUGEPAGE	15		

#define MADV_DONTDUMP   16		
#define MADV_DODUMP	17		

#define MAP_FILE	0

#endif 
