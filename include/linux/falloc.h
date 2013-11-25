#ifndef _FALLOC_H_
#define _FALLOC_H_

#define FALLOC_FL_KEEP_SIZE	0x01 
#define FALLOC_FL_PUNCH_HOLE	0x02 

#ifdef __KERNEL__

struct space_resv {
	__s16		l_type;
	__s16		l_whence;
	__s64		l_start;
	__s64		l_len;		
	__s32		l_sysid;
	__u32		l_pid;
	__s32		l_pad[4];	
};

#define FS_IOC_RESVSP		_IOW('X', 40, struct space_resv)
#define FS_IOC_RESVSP64		_IOW('X', 42, struct space_resv)

#endif 

#endif 
