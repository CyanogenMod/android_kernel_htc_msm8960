#ifndef _LINUX_FCNTL_H
#define _LINUX_FCNTL_H

#include <asm/fcntl.h>

#define F_SETLEASE	(F_LINUX_SPECIFIC_BASE + 0)
#define F_GETLEASE	(F_LINUX_SPECIFIC_BASE + 1)

#define F_CANCELLK	(F_LINUX_SPECIFIC_BASE + 5)

#define F_DUPFD_CLOEXEC	(F_LINUX_SPECIFIC_BASE + 6)

#define F_NOTIFY	(F_LINUX_SPECIFIC_BASE+2)

#define F_SETPIPE_SZ	(F_LINUX_SPECIFIC_BASE + 7)
#define F_GETPIPE_SZ	(F_LINUX_SPECIFIC_BASE + 8)

#define DN_ACCESS	0x00000001	
#define DN_MODIFY	0x00000002	
#define DN_CREATE	0x00000004	
#define DN_DELETE	0x00000008	
#define DN_RENAME	0x00000010	
#define DN_ATTRIB	0x00000020	
#define DN_MULTISHOT	0x80000000	

#define AT_FDCWD		-100    
#define AT_SYMLINK_NOFOLLOW	0x100   
#define AT_REMOVEDIR		0x200   
#define AT_SYMLINK_FOLLOW	0x400   
#define AT_NO_AUTOMOUNT		0x800	
#define AT_EMPTY_PATH		0x1000	

#ifdef __KERNEL__

#ifndef force_o_largefile
#define force_o_largefile() (BITS_PER_LONG != 32)
#endif

#if BITS_PER_LONG == 32
#define IS_GETLK32(cmd)		((cmd) == F_GETLK)
#define IS_SETLK32(cmd)		((cmd) == F_SETLK)
#define IS_SETLKW32(cmd)	((cmd) == F_SETLKW)
#define IS_GETLK64(cmd)		((cmd) == F_GETLK64)
#define IS_SETLK64(cmd)		((cmd) == F_SETLK64)
#define IS_SETLKW64(cmd)	((cmd) == F_SETLKW64)
#else
#define IS_GETLK32(cmd)		(0)
#define IS_SETLK32(cmd)		(0)
#define IS_SETLKW32(cmd)	(0)
#define IS_GETLK64(cmd)		((cmd) == F_GETLK)
#define IS_SETLK64(cmd)		((cmd) == F_SETLK)
#define IS_SETLKW64(cmd)	((cmd) == F_SETLKW)
#endif 

#define IS_GETLK(cmd)	(IS_GETLK32(cmd)  || IS_GETLK64(cmd))
#define IS_SETLK(cmd)	(IS_SETLK32(cmd)  || IS_SETLK64(cmd))
#define IS_SETLKW(cmd)	(IS_SETLKW32(cmd) || IS_SETLKW64(cmd))

#endif 

#endif
