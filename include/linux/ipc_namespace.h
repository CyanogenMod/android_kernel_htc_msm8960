#ifndef __IPC_NAMESPACE_H__
#define __IPC_NAMESPACE_H__

#include <linux/err.h>
#include <linux/idr.h>
#include <linux/rwsem.h>
#include <linux/notifier.h>
#include <linux/nsproxy.h>

#define IPCNS_MEMCHANGED   0x00000001   
#define IPCNS_CREATED  0x00000002   
#define IPCNS_REMOVED  0x00000003   

#define IPCNS_CALLBACK_PRI 0

struct user_namespace;

struct ipc_ids {
	int in_use;
	unsigned short seq;
	unsigned short seq_max;
	struct rw_semaphore rw_mutex;
	struct idr ipcs_idr;
};

struct ipc_namespace {
	atomic_t	count;
	struct ipc_ids	ids[3];

	int		sem_ctls[4];
	int		used_sems;

	int		msg_ctlmax;
	int		msg_ctlmnb;
	int		msg_ctlmni;
	atomic_t	msg_bytes;
	atomic_t	msg_hdrs;
	int		auto_msgmni;

	size_t		shm_ctlmax;
	size_t		shm_ctlall;
	int		shm_ctlmni;
	int		shm_tot;
	int		shm_rmid_forced;

	struct notifier_block ipcns_nb;

	
	struct vfsmount	*mq_mnt;

	
	unsigned int    mq_queues_count;

	
	unsigned int    mq_queues_max;   
	unsigned int    mq_msg_max;      
	unsigned int    mq_msgsize_max;  

	
	struct user_namespace *user_ns;
};

extern struct ipc_namespace init_ipc_ns;
extern atomic_t nr_ipc_ns;

extern spinlock_t mq_lock;

#ifdef CONFIG_SYSVIPC
extern int register_ipcns_notifier(struct ipc_namespace *);
extern int cond_register_ipcns_notifier(struct ipc_namespace *);
extern void unregister_ipcns_notifier(struct ipc_namespace *);
extern int ipcns_notify(unsigned long);
extern void shm_destroy_orphaned(struct ipc_namespace *ns);
#else 
static inline int register_ipcns_notifier(struct ipc_namespace *ns)
{ return 0; }
static inline int cond_register_ipcns_notifier(struct ipc_namespace *ns)
{ return 0; }
static inline void unregister_ipcns_notifier(struct ipc_namespace *ns) { }
static inline int ipcns_notify(unsigned long l) { return 0; }
static inline void shm_destroy_orphaned(struct ipc_namespace *ns) {}
#endif 

#ifdef CONFIG_POSIX_MQUEUE
extern int mq_init_ns(struct ipc_namespace *ns);
#define DFLT_QUEUESMAX 256     
#define DFLT_MSGMAX    10      
#define HARD_MSGMAX    (32768*sizeof(void *)/4)
#define DFLT_MSGSIZEMAX 8192   
#else
static inline int mq_init_ns(struct ipc_namespace *ns) { return 0; }
#endif

#if defined(CONFIG_IPC_NS)
extern struct ipc_namespace *copy_ipcs(unsigned long flags,
				       struct task_struct *tsk);
static inline struct ipc_namespace *get_ipc_ns(struct ipc_namespace *ns)
{
	if (ns)
		atomic_inc(&ns->count);
	return ns;
}

extern void put_ipc_ns(struct ipc_namespace *ns);
#else
static inline struct ipc_namespace *copy_ipcs(unsigned long flags,
					      struct task_struct *tsk)
{
	if (flags & CLONE_NEWIPC)
		return ERR_PTR(-EINVAL);

	return tsk->nsproxy->ipc_ns;
}

static inline struct ipc_namespace *get_ipc_ns(struct ipc_namespace *ns)
{
	return ns;
}

static inline void put_ipc_ns(struct ipc_namespace *ns)
{
}
#endif

#ifdef CONFIG_POSIX_MQUEUE_SYSCTL

struct ctl_table_header;
extern struct ctl_table_header *mq_register_sysctl_table(void);

#else 

static inline struct ctl_table_header *mq_register_sysctl_table(void)
{
	return NULL;
}

#endif 
#endif
