#ifndef _LINUX_PTRACE_H
#define _LINUX_PTRACE_H


#define PTRACE_TRACEME		   0
#define PTRACE_PEEKTEXT		   1
#define PTRACE_PEEKDATA		   2
#define PTRACE_PEEKUSR		   3
#define PTRACE_POKETEXT		   4
#define PTRACE_POKEDATA		   5
#define PTRACE_POKEUSR		   6
#define PTRACE_CONT		   7
#define PTRACE_KILL		   8
#define PTRACE_SINGLESTEP	   9

#define PTRACE_ATTACH		  16
#define PTRACE_DETACH		  17

#define PTRACE_SYSCALL		  24

#define PTRACE_SETOPTIONS	0x4200
#define PTRACE_GETEVENTMSG	0x4201
#define PTRACE_GETSIGINFO	0x4202
#define PTRACE_SETSIGINFO	0x4203

/*
 * Generic ptrace interface that exports the architecture specific regsets
 * using the corresponding NT_* types (which are also used in the core dump).
 * Please note that the NT_PRSTATUS note type in a core dump contains a full
 * 'struct elf_prstatus'. But the user_regset for NT_PRSTATUS contains just the
 * elf_gregset_t that is the pr_reg field of 'struct elf_prstatus'. For all the
 * other user_regset flavors, the user_regset layout and the ELF core dump note
 * payload are exactly the same layout.
 *
 * This interface usage is as follows:
 *	struct iovec iov = { buf, len};
 *
 *	ret = ptrace(PTRACE_GETREGSET/PTRACE_SETREGSET, pid, NT_XXX_TYPE, &iov);
 *
 * On the successful completion, iov.len will be updated by the kernel,
 * specifying how much the kernel has written/read to/from the user's iov.buf.
 */
#define PTRACE_GETREGSET	0x4204
#define PTRACE_SETREGSET	0x4205

#define PTRACE_SEIZE		0x4206
#define PTRACE_INTERRUPT	0x4207
#define PTRACE_LISTEN		0x4208

#define PTRACE_EVENT_FORK	1
#define PTRACE_EVENT_VFORK	2
#define PTRACE_EVENT_CLONE	3
#define PTRACE_EVENT_EXEC	4
#define PTRACE_EVENT_VFORK_DONE	5
#define PTRACE_EVENT_EXIT	6
#define PTRACE_EVENT_STOP	128

#define PTRACE_O_TRACESYSGOOD	1
#define PTRACE_O_TRACEFORK	(1 << PTRACE_EVENT_FORK)
#define PTRACE_O_TRACEVFORK	(1 << PTRACE_EVENT_VFORK)
#define PTRACE_O_TRACECLONE	(1 << PTRACE_EVENT_CLONE)
#define PTRACE_O_TRACEEXEC	(1 << PTRACE_EVENT_EXEC)
#define PTRACE_O_TRACEVFORKDONE	(1 << PTRACE_EVENT_VFORK_DONE)
#define PTRACE_O_TRACEEXIT	(1 << PTRACE_EVENT_EXIT)

#define PTRACE_O_MASK		0x0000007f

#include <asm/ptrace.h>

#ifdef __KERNEL__

#define PT_SEIZED	0x00010000	
#define PT_PTRACED	0x00000001
#define PT_DTRACE	0x00000002	
#define PT_PTRACE_CAP	0x00000004	

#define PT_OPT_FLAG_SHIFT	3
#define PT_EVENT_FLAG(event)	(1 << (PT_OPT_FLAG_SHIFT + (event)))
#define PT_TRACESYSGOOD		PT_EVENT_FLAG(0)
#define PT_TRACE_FORK		PT_EVENT_FLAG(PTRACE_EVENT_FORK)
#define PT_TRACE_VFORK		PT_EVENT_FLAG(PTRACE_EVENT_VFORK)
#define PT_TRACE_CLONE		PT_EVENT_FLAG(PTRACE_EVENT_CLONE)
#define PT_TRACE_EXEC		PT_EVENT_FLAG(PTRACE_EVENT_EXEC)
#define PT_TRACE_VFORK_DONE	PT_EVENT_FLAG(PTRACE_EVENT_VFORK_DONE)
#define PT_TRACE_EXIT		PT_EVENT_FLAG(PTRACE_EVENT_EXIT)

#define PT_SINGLESTEP_BIT	31
#define PT_SINGLESTEP		(1<<PT_SINGLESTEP_BIT)
#define PT_BLOCKSTEP_BIT	30
#define PT_BLOCKSTEP		(1<<PT_BLOCKSTEP_BIT)

#include <linux/compiler.h>		
#include <linux/sched.h>		
#include <linux/err.h>			
#include <linux/bug.h>			


extern long arch_ptrace(struct task_struct *child, long request,
			unsigned long addr, unsigned long data);
extern int ptrace_readdata(struct task_struct *tsk, unsigned long src, char __user *dst, int len);
extern int ptrace_writedata(struct task_struct *tsk, char __user *src, unsigned long dst, int len);
extern void ptrace_disable(struct task_struct *);
extern int ptrace_check_attach(struct task_struct *task, bool ignore_state);
extern int ptrace_request(struct task_struct *child, long request,
			  unsigned long addr, unsigned long data);
extern void ptrace_notify(int exit_code);
extern void __ptrace_link(struct task_struct *child,
			  struct task_struct *new_parent);
extern void __ptrace_unlink(struct task_struct *child);
extern void exit_ptrace(struct task_struct *tracer);
#define PTRACE_MODE_READ	0x01
#define PTRACE_MODE_ATTACH	0x02
#define PTRACE_MODE_NOAUDIT	0x04
extern int __ptrace_may_access(struct task_struct *task, unsigned int mode);
extern bool ptrace_may_access(struct task_struct *task, unsigned int mode);

static inline int ptrace_reparented(struct task_struct *child)
{
	return !same_thread_group(child->real_parent, child->parent);
}

static inline void ptrace_unlink(struct task_struct *child)
{
	if (unlikely(child->ptrace))
		__ptrace_unlink(child);
}

int generic_ptrace_peekdata(struct task_struct *tsk, unsigned long addr,
			    unsigned long data);
int generic_ptrace_pokedata(struct task_struct *tsk, unsigned long addr,
			    unsigned long data);

static inline struct task_struct *ptrace_parent(struct task_struct *task)
{
	if (unlikely(task->ptrace))
		return rcu_dereference(task->parent);
	return NULL;
}

static inline bool ptrace_event_enabled(struct task_struct *task, int event)
{
	return task->ptrace & PT_EVENT_FLAG(event);
}

static inline void ptrace_event(int event, unsigned long message)
{
	if (unlikely(ptrace_event_enabled(current, event))) {
		current->ptrace_message = message;
		ptrace_notify((event << 8) | SIGTRAP);
	} else if (event == PTRACE_EVENT_EXEC) {
		
		if ((current->ptrace & (PT_PTRACED|PT_SEIZED)) == PT_PTRACED)
			send_sig(SIGTRAP, current, 0);
	}
}

static inline void ptrace_init_task(struct task_struct *child, bool ptrace)
{
	INIT_LIST_HEAD(&child->ptrace_entry);
	INIT_LIST_HEAD(&child->ptraced);
#ifdef CONFIG_HAVE_HW_BREAKPOINT
	atomic_set(&child->ptrace_bp_refcnt, 1);
#endif
	child->jobctl = 0;
	child->ptrace = 0;
	child->parent = child->real_parent;

	if (unlikely(ptrace) && current->ptrace) {
		child->ptrace = current->ptrace;
		__ptrace_link(child, current->parent);

		if (child->ptrace & PT_SEIZED)
			task_set_jobctl_pending(child, JOBCTL_TRAP_STOP);
		else
			sigaddset(&child->pending.signal, SIGSTOP);

		set_tsk_thread_flag(child, TIF_SIGPENDING);
	}
}

static inline void ptrace_release_task(struct task_struct *task)
{
	BUG_ON(!list_empty(&task->ptraced));
	ptrace_unlink(task);
	BUG_ON(!list_empty(&task->ptrace_entry));
}

#ifndef force_successful_syscall_return
#define force_successful_syscall_return() do { } while (0)
#endif

#ifndef is_syscall_success
#define is_syscall_success(regs) (!IS_ERR_VALUE((unsigned long)(regs_return_value(regs))))
#endif


#ifndef arch_has_single_step
#define arch_has_single_step()		(0)

static inline void user_enable_single_step(struct task_struct *task)
{
	BUG();			
}

static inline void user_disable_single_step(struct task_struct *task)
{
}
#else
extern void user_enable_single_step(struct task_struct *);
extern void user_disable_single_step(struct task_struct *);
#endif	

#ifndef arch_has_block_step
#define arch_has_block_step()		(0)

static inline void user_enable_block_step(struct task_struct *task)
{
	BUG();			
}
#else
extern void user_enable_block_step(struct task_struct *);
#endif	

#ifdef ARCH_HAS_USER_SINGLE_STEP_INFO
extern void user_single_step_siginfo(struct task_struct *tsk,
				struct pt_regs *regs, siginfo_t *info);
#else
static inline void user_single_step_siginfo(struct task_struct *tsk,
				struct pt_regs *regs, siginfo_t *info)
{
	memset(info, 0, sizeof(*info));
	info->si_signo = SIGTRAP;
}
#endif

#ifndef arch_ptrace_stop_needed
#define arch_ptrace_stop_needed(code, info)	(0)
#endif

#ifndef arch_ptrace_stop
/**
 * arch_ptrace_stop - Do machine-specific work before stopping for ptrace
 * @code:	current->exit_code value ptrace will stop with
 * @info:	siginfo_t pointer (or %NULL) for signal ptrace will stop with
 *
 * This is called with no locks held when arch_ptrace_stop_needed() has
 * just returned nonzero.  It is allowed to block, e.g. for user memory
 * access.  The arch can have machine-specific work to be done before
 * ptrace stops.  On ia64, register backing store gets written back to user
 * memory here.  Since this can be costly (requires dropping the siglock),
 * we only do it when the arch requires it for this particular stop, as
 * indicated by arch_ptrace_stop_needed().
 */
#define arch_ptrace_stop(code, info)		do { } while (0)
#endif

extern int task_current_syscall(struct task_struct *target, long *callno,
				unsigned long args[6], unsigned int maxargs,
				unsigned long *sp, unsigned long *pc);

#ifdef CONFIG_HAVE_HW_BREAKPOINT
extern int ptrace_get_breakpoints(struct task_struct *tsk);
extern void ptrace_put_breakpoints(struct task_struct *tsk);
#else
static inline void ptrace_put_breakpoints(struct task_struct *tsk) { }
#endif 

#endif 

#endif
