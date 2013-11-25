#ifndef _ASMARM_SEMBUF_H
#define _ASMARM_SEMBUF_H


struct semid64_ds {
	struct ipc64_perm sem_perm;		
	__kernel_time_t	sem_otime;		
	unsigned long	__unused1;
	__kernel_time_t	sem_ctime;		
	unsigned long	__unused2;
	unsigned long	sem_nsems;		
	unsigned long	__unused3;
	unsigned long	__unused4;
};

#endif 
