#ifndef _LINUX_HIGHUID_H
#define _LINUX_HIGHUID_H

#include <linux/types.h>




extern int overflowuid;
extern int overflowgid;

extern void __bad_uid(void);
extern void __bad_gid(void);

#define DEFAULT_OVERFLOWUID	65534
#define DEFAULT_OVERFLOWGID	65534

#ifdef CONFIG_UID16

#define high2lowuid(uid) ((uid) & ~0xFFFF ? (old_uid_t)overflowuid : (old_uid_t)(uid))
#define high2lowgid(gid) ((gid) & ~0xFFFF ? (old_gid_t)overflowgid : (old_gid_t)(gid))
#define low2highuid(uid) ((uid) == (old_uid_t)-1 ? (uid_t)-1 : (uid_t)(uid))
#define low2highgid(gid) ((gid) == (old_gid_t)-1 ? (gid_t)-1 : (gid_t)(gid))

#define __convert_uid(size, uid) \
	(size >= sizeof(uid) ? (uid) : high2lowuid(uid))
#define __convert_gid(size, gid) \
	(size >= sizeof(gid) ? (gid) : high2lowgid(gid))
	

#else

#define __convert_uid(size, uid) (uid)
#define __convert_gid(size, gid) (gid)

#endif 

#define SET_UID(var, uid) do { (var) = __convert_uid(sizeof(var), (uid)); } while (0)
#define SET_GID(var, gid) do { (var) = __convert_gid(sizeof(var), (gid)); } while (0)


/*
 * This is the UID and GID that will get written to disk if a filesystem
 * only supports 16-bit UIDs and the kernel has a high UID/GID to write
 */
extern int fs_overflowuid;
extern int fs_overflowgid;

#define DEFAULT_FS_OVERFLOWUID	65534
#define DEFAULT_FS_OVERFLOWGID	65534

#define fs_high2lowuid(uid) ((uid) & ~0xFFFF ? (uid16_t)fs_overflowuid : (uid16_t)(uid))
#define fs_high2lowgid(gid) ((gid) & ~0xFFFF ? (gid16_t)fs_overflowgid : (gid16_t)(gid))

#define low_16_bits(x)	((x) & 0xFFFF)
#define high_16_bits(x)	(((x) & 0xFFFF0000) >> 16)

#endif 
