#ifndef _LINUX_BLKPG_H
#define _LINUX_BLKPG_H

#include <linux/compiler.h>
#include <linux/ioctl.h>

#define BLKPG      _IO(0x12,105)

struct blkpg_ioctl_arg {
        int op;
        int flags;
        int datalen;
        void __user *data;
};

#define BLKPG_ADD_PARTITION	1
#define BLKPG_DEL_PARTITION	2

#define BLKPG_DEVNAMELTH	64
#define BLKPG_VOLNAMELTH	64

struct blkpg_partition {
	long long start;		
	long long length;		
	int pno;			
	char devname[BLKPG_DEVNAMELTH];	
	char volname[BLKPG_VOLNAMELTH];	
};

#endif 
