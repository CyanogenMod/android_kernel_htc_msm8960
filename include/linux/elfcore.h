#ifndef _LINUX_ELFCORE_H
#define _LINUX_ELFCORE_H

#include <linux/types.h>
#include <linux/signal.h>
#include <linux/time.h>
#ifdef __KERNEL__
#include <linux/user.h>
#include <linux/bug.h>
#endif
#include <linux/ptrace.h>
#include <linux/elf.h>
#include <linux/fs.h>

struct elf_siginfo
{
	int	si_signo;			
	int	si_code;			
	int	si_errno;			
};

#ifdef __KERNEL__
#include <asm/elf.h>
#endif

#ifndef __KERNEL__
typedef elf_greg_t greg_t;
typedef elf_gregset_t gregset_t;
typedef elf_fpregset_t fpregset_t;
typedef elf_fpxregset_t fpxregset_t;
#define NGREG ELF_NGREG
#endif

struct elf_prstatus
{
#if 0
	long	pr_flags;	
	short	pr_why;		
	short	pr_what;	
#endif
	struct elf_siginfo pr_info;	
	short	pr_cursig;		
	unsigned long pr_sigpend;	
	unsigned long pr_sighold;	
#if 0
	struct sigaltstack pr_altstack;	
	struct sigaction pr_action;	
#endif
	pid_t	pr_pid;
	pid_t	pr_ppid;
	pid_t	pr_pgrp;
	pid_t	pr_sid;
	struct timeval pr_utime;	
	struct timeval pr_stime;	
	struct timeval pr_cutime;	
	struct timeval pr_cstime;	
#if 0
	long	pr_instr;		
#endif
	elf_gregset_t pr_reg;	
#ifdef CONFIG_BINFMT_ELF_FDPIC
	unsigned long pr_exec_fdpic_loadmap;
	unsigned long pr_interp_fdpic_loadmap;
#endif
	int pr_fpvalid;		
};

#define ELF_PRARGSZ	(80)	

struct elf_prpsinfo
{
	char	pr_state;	
	char	pr_sname;	
	char	pr_zomb;	
	char	pr_nice;	
	unsigned long pr_flag;	
	__kernel_uid_t	pr_uid;
	__kernel_gid_t	pr_gid;
	pid_t	pr_pid, pr_ppid, pr_pgrp, pr_sid;
	
	char	pr_fname[16];	
	char	pr_psargs[ELF_PRARGSZ];	
};

#ifndef __KERNEL__
typedef struct elf_prstatus prstatus_t;
typedef struct elf_prpsinfo prpsinfo_t;
#define PRARGSZ ELF_PRARGSZ 
#endif

#ifdef __KERNEL__
static inline void elf_core_copy_regs(elf_gregset_t *elfregs, struct pt_regs *regs)
{
#ifdef ELF_CORE_COPY_REGS
	ELF_CORE_COPY_REGS((*elfregs), regs)
#else
	BUG_ON(sizeof(*elfregs) != sizeof(*regs));
	*(struct pt_regs *)elfregs = *regs;
#endif
}

static inline void elf_core_copy_kernel_regs(elf_gregset_t *elfregs, struct pt_regs *regs)
{
#ifdef ELF_CORE_COPY_KERNEL_REGS
	ELF_CORE_COPY_KERNEL_REGS((*elfregs), regs);
#else
	elf_core_copy_regs(elfregs, regs);
#endif
}

static inline int elf_core_copy_task_regs(struct task_struct *t, elf_gregset_t* elfregs)
{
#if defined (ELF_CORE_COPY_TASK_REGS)
	return ELF_CORE_COPY_TASK_REGS(t, elfregs);
#elif defined (task_pt_regs)
	elf_core_copy_regs(elfregs, task_pt_regs(t));
#endif
	return 0;
}

extern int dump_fpu (struct pt_regs *, elf_fpregset_t *);

static inline int elf_core_copy_task_fpregs(struct task_struct *t, struct pt_regs *regs, elf_fpregset_t *fpu)
{
#ifdef ELF_CORE_COPY_FPREGS
	return ELF_CORE_COPY_FPREGS(t, fpu);
#else
	return dump_fpu(regs, fpu);
#endif
}

#ifdef ELF_CORE_COPY_XFPREGS
static inline int elf_core_copy_task_xfpregs(struct task_struct *t, elf_fpxregset_t *xfpu)
{
	return ELF_CORE_COPY_XFPREGS(t, xfpu);
}
#endif

extern Elf_Half elf_core_extra_phdrs(void);
extern int
elf_core_write_extra_phdrs(struct file *file, loff_t offset, size_t *size,
			   unsigned long limit);
extern int
elf_core_write_extra_data(struct file *file, size_t *size, unsigned long limit);
extern size_t elf_core_extra_data_size(void);

#endif 

#endif 
