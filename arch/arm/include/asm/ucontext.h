#ifndef _ASMARM_UCONTEXT_H
#define _ASMARM_UCONTEXT_H

#include <asm/fpstate.h>


struct ucontext {
	unsigned long	  uc_flags;
	struct ucontext  *uc_link;
	stack_t		  uc_stack;
	struct sigcontext uc_mcontext;
	sigset_t	  uc_sigmask;
	
	int		  __unused[32 - (sizeof (sigset_t) / sizeof (int))];
 	unsigned long	  uc_regspace[128] __attribute__((__aligned__(8)));
};

#ifdef __KERNEL__


#ifdef CONFIG_CRUNCH
#define CRUNCH_MAGIC		0x5065cf03
#define CRUNCH_STORAGE_SIZE	(CRUNCH_SIZE + 8)

struct crunch_sigframe {
	unsigned long	magic;
	unsigned long	size;
	struct crunch_state	storage;
} __attribute__((__aligned__(8)));
#endif

#ifdef CONFIG_IWMMXT
#define IWMMXT_MAGIC		0x12ef842a
#define IWMMXT_STORAGE_SIZE	(IWMMXT_SIZE + 8)

struct iwmmxt_sigframe {
	unsigned long	magic;
	unsigned long	size;
	struct iwmmxt_struct storage;
} __attribute__((__aligned__(8)));
#endif 

#ifdef CONFIG_VFP
#define VFP_MAGIC		0x56465001

struct vfp_sigframe
{
	unsigned long		magic;
	unsigned long		size;
	struct user_vfp		ufp;
	struct user_vfp_exc	ufp_exc;
} __attribute__((__aligned__(8)));

#define VFP_STORAGE_SIZE	sizeof(struct vfp_sigframe)

#endif 

struct aux_sigframe {
#ifdef CONFIG_CRUNCH
	struct crunch_sigframe	crunch;
#endif
#ifdef CONFIG_IWMMXT
	struct iwmmxt_sigframe	iwmmxt;
#endif
#ifdef CONFIG_VFP
	struct vfp_sigframe	vfp;
#endif
	
	unsigned long		end_magic;
} __attribute__((__aligned__(8)));

#endif

#endif 
