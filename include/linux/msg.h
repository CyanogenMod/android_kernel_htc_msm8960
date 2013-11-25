#ifndef _LINUX_MSG_H
#define _LINUX_MSG_H

#include <linux/ipc.h>

#define MSG_STAT 11
#define MSG_INFO 12

#define MSG_NOERROR     010000  
#define MSG_EXCEPT      020000  

struct msqid_ds {
	struct ipc_perm msg_perm;
	struct msg *msg_first;		
	struct msg *msg_last;		
	__kernel_time_t msg_stime;	
	__kernel_time_t msg_rtime;	
	__kernel_time_t msg_ctime;	
	unsigned long  msg_lcbytes;	
	unsigned long  msg_lqbytes;	
	unsigned short msg_cbytes;	
	unsigned short msg_qnum;	
	unsigned short msg_qbytes;	
	__kernel_ipc_pid_t msg_lspid;	
	__kernel_ipc_pid_t msg_lrpid;	
};

#include <asm/msgbuf.h>

struct msgbuf {
	long mtype;         
	char mtext[1];      
};

struct msginfo {
	int msgpool;
	int msgmap; 
	int msgmax; 
	int msgmnb; 
	int msgmni; 
	int msgssz; 
	int msgtql; 
	unsigned short  msgseg; 
};

#define MSG_MEM_SCALE 32

#define MSGMNI    16        
#define MSGMAX  8192      
#define MSGMNB 16384      

#define MSGPOOL (MSGMNI * MSGMNB / 1024) 
#define MSGTQL  MSGMNB            
#define MSGMAP  MSGMNB            
#define MSGSSZ  16                
#define __MSGSEG ((MSGPOOL * 1024) / MSGSSZ) 
#define MSGSEG (__MSGSEG <= 0xffff ? __MSGSEG : 0xffff)

#ifdef __KERNEL__
#include <linux/list.h>

struct msg_msg {
	struct list_head m_list; 
	long  m_type;          
	int m_ts;           
	struct msg_msgseg* next;
	void *security;
	
};

struct msg_queue {
	struct kern_ipc_perm q_perm;
	time_t q_stime;			
	time_t q_rtime;			
	time_t q_ctime;			
	unsigned long q_cbytes;		
	unsigned long q_qnum;		
	unsigned long q_qbytes;		
	pid_t q_lspid;			
	pid_t q_lrpid;			

	struct list_head q_messages;
	struct list_head q_receivers;
	struct list_head q_senders;
};

extern long do_msgsnd(int msqid, long mtype, void __user *mtext,
			size_t msgsz, int msgflg);
extern long do_msgrcv(int msqid, long *pmtype, void __user *mtext,
			size_t msgsz, long msgtyp, int msgflg);

#endif 

#endif 
