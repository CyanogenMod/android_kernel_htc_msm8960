#ifndef _LINUX_SEM_H
#define _LINUX_SEM_H

#include <linux/ipc.h>

#define SEM_UNDO        0x1000  

#define GETPID  11       
#define GETVAL  12       
#define GETALL  13       
#define GETNCNT 14       
#define GETZCNT 15       
#define SETVAL  16       
#define SETALL  17       

#define SEM_STAT 18
#define SEM_INFO 19

struct semid_ds {
	struct ipc_perm	sem_perm;		
	__kernel_time_t	sem_otime;		
	__kernel_time_t	sem_ctime;		
	struct sem	*sem_base;		
	struct sem_queue *sem_pending;		
	struct sem_queue **sem_pending_last;	
	struct sem_undo	*undo;			
	unsigned short	sem_nsems;		
};

#include <asm/sembuf.h>

struct sembuf {
	unsigned short  sem_num;	
	short		sem_op;		
	short		sem_flg;	
};

union semun {
	int val;			
	struct semid_ds __user *buf;	
	unsigned short __user *array;	
	struct seminfo __user *__buf;	
	void __user *__pad;
};

struct  seminfo {
	int semmap;
	int semmni;
	int semmns;
	int semmnu;
	int semmsl;
	int semopm;
	int semume;
	int semusz;
	int semvmx;
	int semaem;
};

#define SEMMNI  128             
#define SEMMSL  250             
#define SEMMNS  (SEMMNI*SEMMSL) 
#define SEMOPM  32	        
#define SEMVMX  32767           
#define SEMAEM  SEMVMX          

#define SEMUME  SEMOPM          
#define SEMMNU  SEMMNS          
#define SEMMAP  SEMMNS          
#define SEMUSZ  20		

#ifdef __KERNEL__
#include <linux/atomic.h>
#include <linux/rcupdate.h>
#include <linux/cache.h>

struct task_struct;

struct sem_array {
	struct kern_ipc_perm	____cacheline_aligned_in_smp
				sem_perm;	
	time_t			sem_otime;	
	time_t			sem_ctime;	
	struct sem		*sem_base;	
	struct list_head	sem_pending;	
	struct list_head	list_id;	
	int			sem_nsems;	
	int			complex_count;	
};

#ifdef CONFIG_SYSVIPC

struct sysv_sem {
	struct sem_undo_list *undo_list;
};

extern int copy_semundo(unsigned long clone_flags, struct task_struct *tsk);
extern void exit_sem(struct task_struct *tsk);

#else

struct sysv_sem {
	
};

static inline int copy_semundo(unsigned long clone_flags, struct task_struct *tsk)
{
	return 0;
}

static inline void exit_sem(struct task_struct *tsk)
{
	return;
}
#endif

#endif 

#endif 
