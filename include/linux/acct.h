/*
 *  BSD Process Accounting for Linux - Definitions
 *
 *  Author: Marco van Wieringen (mvw@planets.elm.net)
 *
 *  This header file contains the definitions needed to implement
 *  BSD-style process accounting. The kernel accounting code and all
 *  user-level programs that try to do something useful with the
 *  process accounting log must include this file.
 *
 *  Copyright (C) 1995 - 1997 Marco van Wieringen - ELM Consultancy B.V.
 *
 */

#ifndef _LINUX_ACCT_H
#define _LINUX_ACCT_H

#include <linux/types.h>

#include <asm/param.h>
#include <asm/byteorder.h>


typedef __u16	comp_t;
typedef __u32	comp2_t;

/*
 *   accounting file record
 *
 *   This structure contains all of the information written out to the
 *   process accounting file whenever a process exits.
 */

#define ACCT_COMM	16

struct acct
{
	char		ac_flag;		
	char		ac_version;		
	
	__u16		ac_uid16;		
	__u16		ac_gid16;		
	__u16		ac_tty;			
	__u32		ac_btime;		
	comp_t		ac_utime;		
	comp_t		ac_stime;		
	comp_t		ac_etime;		
	comp_t		ac_mem;			
	comp_t		ac_io;			
	comp_t		ac_rw;			/* Blocks Read or Written */
	comp_t		ac_minflt;		
	comp_t		ac_majflt;		
	comp_t		ac_swaps;		
#if !defined(CONFIG_M68K) || !defined(__KERNEL__)
	__u16		ac_ahz;			
#endif
	__u32		ac_exitcode;		
	char		ac_comm[ACCT_COMM + 1];	
	__u8		ac_etime_hi;		
	__u16		ac_etime_lo;		
	__u32		ac_uid;			
	__u32		ac_gid;			
};

struct acct_v3
{
	char		ac_flag;		
	char		ac_version;		
	__u16		ac_tty;			
	__u32		ac_exitcode;		
	__u32		ac_uid;			
	__u32		ac_gid;			
	__u32		ac_pid;			
	__u32		ac_ppid;		
	__u32		ac_btime;		
#ifdef __KERNEL__
	__u32		ac_etime;		
#else
	float		ac_etime;		
#endif
	comp_t		ac_utime;		
	comp_t		ac_stime;		
	comp_t		ac_mem;			
	comp_t		ac_io;			
	comp_t		ac_rw;			/* Blocks Read or Written */
	comp_t		ac_minflt;		
	comp_t		ac_majflt;		
	comp_t		ac_swaps;		
	char		ac_comm[ACCT_COMM];	
};

				
#define AFORK		0x01	
#define ASU		0x02	
#define ACOMPAT		0x04	
#define ACORE		0x08	
#define AXSIG		0x10	

#ifdef __BIG_ENDIAN
#define ACCT_BYTEORDER	0x80	
#else
#define ACCT_BYTEORDER	0x00	
#endif

#ifdef __KERNEL__


#ifdef CONFIG_BSD_PROCESS_ACCT
struct vfsmount;
struct super_block;
struct pacct_struct;
struct pid_namespace;
extern int acct_parm[]; 
extern void acct_auto_close_mnt(struct vfsmount *m);
extern void acct_auto_close(struct super_block *sb);
extern void acct_collect(long exitcode, int group_dead);
extern void acct_process(void);
extern void acct_exit_ns(struct pid_namespace *);
#else
#define acct_auto_close_mnt(x)	do { } while (0)
#define acct_auto_close(x)	do { } while (0)
#define acct_collect(x,y)	do { } while (0)
#define acct_process()		do { } while (0)
#define acct_exit_ns(ns)	do { } while (0)
#endif


#undef ACCT_VERSION
#undef AHZ

#ifdef CONFIG_BSD_PROCESS_ACCT_V3
#define ACCT_VERSION	3
#define AHZ		100
typedef struct acct_v3 acct_t;
#else
#ifdef CONFIG_M68K
#define ACCT_VERSION	1
#else
#define ACCT_VERSION	2
#endif
#define AHZ		(USER_HZ)
typedef struct acct acct_t;
#endif

#else
#define ACCT_VERSION	2
#define AHZ		(HZ)
#endif	

#ifdef __KERNEL__
#include <linux/jiffies.h>

static inline u32 jiffies_to_AHZ(unsigned long x)
{
#if (TICK_NSEC % (NSEC_PER_SEC / AHZ)) == 0
# if HZ < AHZ
	return x * (AHZ / HZ);
# else
	return x / (HZ / AHZ);
# endif
#else
        u64 tmp = (u64)x * TICK_NSEC;
        do_div(tmp, (NSEC_PER_SEC / AHZ));
        return (long)tmp;
#endif
}

static inline u64 nsec_to_AHZ(u64 x)
{
#if (NSEC_PER_SEC % AHZ) == 0
	do_div(x, (NSEC_PER_SEC / AHZ));
#elif (AHZ % 512) == 0
	x *= AHZ/512;
	do_div(x, (NSEC_PER_SEC / 512));
#else
	x *= 9;
	do_div(x, (unsigned long)((9ull * NSEC_PER_SEC + (AHZ/2))
	                          / AHZ));
#endif
	return x;
}

#endif  

#endif	
