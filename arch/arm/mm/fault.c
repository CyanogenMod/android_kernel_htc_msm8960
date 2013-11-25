/*
 *  linux/arch/arm/mm/fault.c
 *
 *  Copyright (C) 1995  Linus Torvalds
 *  Modifications for ARM processor (c) 1995-2004 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/mm.h>
#include <linux/hardirq.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/uaccess.h>
#include <linux/page-flags.h>
#include <linux/sched.h>
#include <linux/highmem.h>
#include <linux/perf_event.h>

#include <asm/exception.h>
#include <asm/pgtable.h>
#include <asm/system_misc.h>
#include <asm/system_info.h>
#include <asm/tlbflush.h>
#include <asm/cputype.h>
#if defined(CONFIG_ARCH_MSM_SCORPION) && !defined(CONFIG_MSM_SMP)
#include <asm/io.h>
#include <mach/msm_iomap.h>
#endif
#include <mach/msm_rtb.h>

#ifdef CONFIG_EMULATE_DOMAIN_MANAGER_V7
#include <asm/domain.h>
#endif 

#include "fault.h"
static inline unsigned int read_DFSR(void)
{
	unsigned int dfsr;
	asm volatile ("mrc p15, 0, %0, c5, c0, 0" : "=r" (dfsr));
	return dfsr;
}

static inline unsigned int read_TTBCR(void)
{
	unsigned int ttbcr;
	asm volatile ("mrc p15, 0, %0, c2, c0, 2" : "=r" (ttbcr));
	return ttbcr;
}

static inline unsigned int read_TTBR0(void)
{
	unsigned int ttbr0;
	asm volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r" (ttbr0));
	return ttbr0;
}

static inline unsigned int read_TTBR1(void)
{
	unsigned int ttbr1;
	asm volatile ("mrc p15, 0, %0, c2, c0, 1" : "=r" (ttbr1));
	return ttbr1;
}

static inline unsigned int read_MAIR0(void)
{
	unsigned int mair0;
	asm volatile ("mrc p15, 0, %0, c10, c2, 0" : "=r" (mair0));
	return mair0;
}

static inline unsigned int read_MAIR1(void)
{
	unsigned int mair1;
	asm volatile ("mrc p15, 0, %0, c10, c2, 1" : "=r" (mair1));
	return mair1;
}

static inline unsigned int read_SCTLR(void)
{
	unsigned int sctlr;
	asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (sctlr));
	return sctlr;
}

#ifdef CONFIG_MMU

#ifdef CONFIG_KPROBES
static inline int notify_page_fault(struct pt_regs *regs, unsigned int fsr)
{
	int ret = 0;

	if (!user_mode(regs)) {
		
		preempt_disable();
		if (kprobe_running() && kprobe_fault_handler(regs, fsr))
			ret = 1;
		preempt_enable();
	}

	return ret;
}
#else
static inline int notify_page_fault(struct pt_regs *regs, unsigned int fsr)
{
	return 0;
}
#endif

void show_pte(struct mm_struct *mm, unsigned long addr)
{
	pgd_t *pgd;

	if (!mm)
		mm = &init_mm;

	printk(KERN_ALERT "pgd = %p\n", mm->pgd);
	pgd = pgd_offset(mm, addr);
	printk(KERN_ALERT "[%08lx] *pgd=%08llx",
			addr, (long long)pgd_val(*pgd));

	do {
		pud_t *pud;
		pmd_t *pmd;
		pte_t *pte;

		if (pgd_none(*pgd))
			break;

		if (pgd_bad(*pgd)) {
			printk("(bad)");
			break;
		}

		pud = pud_offset(pgd, addr);
		if (PTRS_PER_PUD != 1)
			printk(", *pud=%08llx", (long long)pud_val(*pud));

		if (pud_none(*pud))
			break;

		if (pud_bad(*pud)) {
			printk("(bad)");
			break;
		}

		pmd = pmd_offset(pud, addr);
		if (PTRS_PER_PMD != 1)
			printk(", *pmd=%08llx", (long long)pmd_val(*pmd));

		if (pmd_none(*pmd))
			break;

		if (pmd_bad(*pmd)) {
			printk("(bad)");
			break;
		}

		
		if (PageHighMem(pfn_to_page(pmd_val(*pmd) >> PAGE_SHIFT)))
			break;

		pte = pte_offset_map(pmd, addr);
		printk(", *pte=%08llx", (long long)pte_val(*pte));
#ifndef CONFIG_ARM_LPAE
		printk(", *ppte=%08llx",
		       (long long)pte_val(pte[PTE_HWTABLE_PTRS]));
#endif
		pte_unmap(pte);
	} while(0);

	printk("\n");
	printk("DFSR=%08x, TTBCR=%08x, TTBR0=%08x, TTBR1=%08x\n", read_DFSR(), read_TTBCR(), read_TTBR0(), read_TTBR1());
	printk("MAIR0=%08x, MAIR1=%08x, SCTLR=%08x\n", read_MAIR0(), read_MAIR1(), read_SCTLR());
}
#else					
void show_pte(struct mm_struct *mm, unsigned long addr)
{ }
#endif					

static void
__do_kernel_fault(struct mm_struct *mm, unsigned long addr, unsigned int fsr,
		  struct pt_regs *regs)
{
	static int enable_logk_die = 1;

	if (enable_logk_die) {
		enable_logk_die = 0;
		uncached_logk(LOGK_DIE, (void *)regs->ARM_pc);
		uncached_logk(LOGK_DIE, (void *)regs->ARM_lr);
		uncached_logk(LOGK_DIE, (void *)addr);
	}

	if (fixup_exception(regs))
		return;

	
	msm_rtb_disable();

	bust_spinlocks(1);
	printk(KERN_ALERT
		"Unable to handle kernel %s at virtual address %08lx\n",
		(addr < PAGE_SIZE) ? "NULL pointer dereference" :
		"paging request", addr);

	show_pte(mm, addr);
	die("Oops", regs, fsr);
	bust_spinlocks(0);
	do_exit(SIGKILL);
}

static void
__do_user_fault(struct task_struct *tsk, unsigned long addr,
		unsigned int fsr, unsigned int sig, int code,
		struct pt_regs *regs)
{
	struct siginfo si;

#ifdef CONFIG_DEBUG_USER
	if (((user_debug & UDBG_SEGV) && (sig == SIGSEGV)) ||
	    ((user_debug & UDBG_BUS)  && (sig == SIGBUS))) {
		printk(KERN_DEBUG "%s: unhandled page fault (%d) at 0x%08lx, code 0x%03x\n",
		       tsk->comm, sig, addr, fsr);
		show_pte(tsk->mm, addr);
		show_regs(regs);
	}
#endif

	tsk->thread.address = addr;
	tsk->thread.error_code = fsr;
	tsk->thread.trap_no = 14;
	si.si_signo = sig;
	si.si_errno = 0;
	si.si_code = code;
	si.si_addr = (void __user *)addr;
	force_sig_info(sig, &si, tsk);
}

void do_bad_area(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
	struct task_struct *tsk = current;
	struct mm_struct *mm = tsk->active_mm;

	if (user_mode(regs))
		__do_user_fault(tsk, addr, fsr, SIGSEGV, SEGV_MAPERR, regs);
	else
		__do_kernel_fault(mm, addr, fsr, regs);
}

#ifdef CONFIG_MMU
#define VM_FAULT_BADMAP		0x010000
#define VM_FAULT_BADACCESS	0x020000

static inline bool access_error(unsigned int fsr, struct vm_area_struct *vma)
{
	unsigned int mask = VM_READ | VM_WRITE | VM_EXEC;

	if (fsr & FSR_WRITE)
		mask = VM_WRITE;
	if (fsr & FSR_LNX_PF)
		mask = VM_EXEC;

	return vma->vm_flags & mask ? false : true;
}

static int __kprobes
__do_page_fault(struct mm_struct *mm, unsigned long addr, unsigned int fsr,
		unsigned int flags, struct task_struct *tsk)
{
	struct vm_area_struct *vma;
	int fault;

	vma = find_vma(mm, addr);
	fault = VM_FAULT_BADMAP;
	if (unlikely(!vma))
		goto out;
	if (unlikely(vma->vm_start > addr))
		goto check_stack;

good_area:
	if (access_error(fsr, vma)) {
		fault = VM_FAULT_BADACCESS;
		goto out;
	}

	return handle_mm_fault(mm, vma, addr & PAGE_MASK, flags);

check_stack:
	
	if (vma->vm_flags & VM_GROWSDOWN &&
	    addr >= FIRST_USER_ADDRESS && !expand_stack(vma, addr))
		goto good_area;
out:
	return fault;
}

static int __kprobes
do_page_fault(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
	struct task_struct *tsk;
	struct mm_struct *mm;
	int fault, sig, code;
	int write = fsr & FSR_WRITE;
	unsigned int flags = FAULT_FLAG_ALLOW_RETRY | FAULT_FLAG_KILLABLE |
				(write ? FAULT_FLAG_WRITE : 0);

	if (notify_page_fault(regs, fsr))
		return 0;

	tsk = current;
	mm  = tsk->mm;

	
	if (interrupts_enabled(regs))
		local_irq_enable();

	if (in_atomic() || !mm)
		goto no_context;

	if (!down_read_trylock(&mm->mmap_sem)) {
		if (!user_mode(regs) && !search_exception_tables(regs->ARM_pc))
			goto no_context;
retry:
		down_read(&mm->mmap_sem);
	} else {
		might_sleep();
#ifdef CONFIG_DEBUG_VM
		if (!user_mode(regs) &&
		    !search_exception_tables(regs->ARM_pc))
			goto no_context;
#endif
	}

	fault = __do_page_fault(mm, addr, fsr, flags, tsk);

	if ((fault & VM_FAULT_RETRY) && fatal_signal_pending(current))
		return 0;


	perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS, 1, regs, addr);
	if (!(fault & VM_FAULT_ERROR) && flags & FAULT_FLAG_ALLOW_RETRY) {
		if (fault & VM_FAULT_MAJOR) {
			tsk->maj_flt++;
			perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS_MAJ, 1,
					regs, addr);
		} else {
			tsk->min_flt++;
			perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS_MIN, 1,
					regs, addr);
		}
		if (fault & VM_FAULT_RETRY) {
			flags &= ~FAULT_FLAG_ALLOW_RETRY;
			goto retry;
		}
	}

	up_read(&mm->mmap_sem);

	if (likely(!(fault & (VM_FAULT_ERROR | VM_FAULT_BADMAP | VM_FAULT_BADACCESS))))
		return 0;

	if (fault & VM_FAULT_OOM) {
		pagefault_out_of_memory();
		return 0;
	}

	if (!user_mode(regs))
		goto no_context;

	if (fault & VM_FAULT_SIGBUS) {
		sig = SIGBUS;
		code = BUS_ADRERR;
	} else {
		sig = SIGSEGV;
		code = fault == VM_FAULT_BADACCESS ?
			SEGV_ACCERR : SEGV_MAPERR;
	}

	__do_user_fault(tsk, addr, fsr, sig, code, regs);
	return 0;

no_context:
	__do_kernel_fault(mm, addr, fsr, regs);
	return 0;
}
#else					
static int
do_page_fault(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
	return 0;
}
#endif					

#ifdef CONFIG_MMU
static int __kprobes
do_translation_fault(unsigned long addr, unsigned int fsr,
		     struct pt_regs *regs)
{
	unsigned int index;
	pgd_t *pgd, *pgd_k;
	pud_t *pud, *pud_k;
	pmd_t *pmd, *pmd_k;

	if (addr < TASK_SIZE)
		return do_page_fault(addr, fsr, regs);

	if (user_mode(regs))
		goto bad_area;

	index = pgd_index(addr);

	pgd = cpu_get_pgd() + index;
	pgd_k = init_mm.pgd + index;

	if (pgd_none(*pgd_k))
		goto bad_area;
	if (!pgd_present(*pgd))
		set_pgd(pgd, *pgd_k);

	pud = pud_offset(pgd, addr);
	pud_k = pud_offset(pgd_k, addr);

	if (pud_none(*pud_k))
		goto bad_area;
	if (!pud_present(*pud))
		set_pud(pud, *pud_k);

	pmd = pmd_offset(pud, addr);
	pmd_k = pmd_offset(pud_k, addr);

#ifdef CONFIG_ARM_LPAE
	index = 0;
#else
	index = (addr >> SECTION_SHIFT) & 1;
#endif
	if (pmd_none(pmd_k[index]))
		goto bad_area;

	copy_pmd(pmd, pmd_k);
	return 0;

bad_area:
	do_bad_area(addr, fsr, regs);
	return 0;
}
#else					
static int
do_translation_fault(unsigned long addr, unsigned int fsr,
		     struct pt_regs *regs)
{
	return 0;
}
#endif					

static int
do_sect_fault(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
	do_bad_area(addr, fsr, regs);
	return 0;
}

static int
do_bad(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
	return 1;
}

#if defined(CONFIG_ARCH_MSM_SCORPION) && !defined(CONFIG_MSM_SMP)
#define __str(x) #x
#define MRC(x, v1, v2, v4, v5, v6) do {					\
	unsigned int __##x;						\
	asm("mrc " __str(v1) ", " __str(v2) ", %0, " __str(v4) ", "	\
		__str(v5) ", " __str(v6) "\n" \
		: "=r" (__##x));					\
	pr_info("%s: %s = 0x%.8x\n", __func__, #x, __##x);		\
} while(0)

#define MSM_TCSR_SPARE2 (MSM_TCSR_BASE + 0x60)

#endif

int
do_imprecise_ext(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
#if defined(CONFIG_ARCH_MSM_SCORPION) && !defined(CONFIG_MSM_SMP)
	MRC(ADFSR,    p15, 0,  c5, c1, 0);
	MRC(DFSR,     p15, 0,  c5, c0, 0);
	MRC(ACTLR,    p15, 0,  c1, c0, 1);
	MRC(EFSR,     p15, 7, c15, c0, 1);
	MRC(L2SR,     p15, 3, c15, c1, 0);
	MRC(L2CR0,    p15, 3, c15, c0, 1);
	MRC(L2CPUESR, p15, 3, c15, c1, 1);
	MRC(L2CPUCR,  p15, 3, c15, c0, 2);
	MRC(SPESR,    p15, 1,  c9, c7, 0);
	MRC(SPCR,     p15, 0,  c9, c7, 0);
	MRC(DMACHSR,  p15, 1, c11, c0, 0);
	MRC(DMACHESR, p15, 1, c11, c0, 1);
	MRC(DMACHCR,  p15, 0, c11, c0, 2);

	
	asm volatile ("mcr p15, 7, %0, c15, c0, 1\n\t"
		      "mcr p15, 0, %0, c5, c1, 0"
		      : : "r" (0));
#endif
#if defined(CONFIG_ARCH_MSM_SCORPION) && !defined(CONFIG_MSM_SMP)
	pr_info("%s: TCSR_SPARE2 = 0x%.8x\n", __func__, readl(MSM_TCSR_SPARE2));
#endif
	return 1;
}

struct fsr_info {
	int	(*fn)(unsigned long addr, unsigned int fsr, struct pt_regs *regs);
	int	sig;
	int	code;
	const char *name;
};

#ifdef CONFIG_ARM_LPAE
#include "fsr-3level.c"
#else
#include "fsr-2level.c"
#endif

void __init
hook_fault_code(int nr, int (*fn)(unsigned long, unsigned int, struct pt_regs *),
		int sig, int code, const char *name)
{
	if (nr < 0 || nr >= ARRAY_SIZE(fsr_info))
		BUG();

	fsr_info[nr].fn   = fn;
	fsr_info[nr].sig  = sig;
	fsr_info[nr].code = code;
	fsr_info[nr].name = name;
}

#ifdef CONFIG_MSM_KRAIT_TBB_ABORT_HANDLER
static int krait_tbb_fixup(unsigned int fsr, struct pt_regs *regs)
{
	int base_cond, cond = 0;
	unsigned int p1, cpsr_z, cpsr_c, cpsr_n, cpsr_v;

	if ((read_cpuid_id() & 0xFFFFFFFC) != 0x510F04D0)
		return 0;

	if (!thumb_mode(regs))
		return 0;

	
	if ((regs->ARM_cpsr & PSR_IT_MASK) == 0)
		return 0;

	cpsr_n = (regs->ARM_cpsr & PSR_N_BIT) ? 1 : 0;
	cpsr_z = (regs->ARM_cpsr & PSR_Z_BIT) ? 1 : 0;
	cpsr_c = (regs->ARM_cpsr & PSR_C_BIT) ? 1 : 0;
	cpsr_v = (regs->ARM_cpsr & PSR_V_BIT) ? 1 : 0;

	p1 = (regs->ARM_cpsr & BIT(12)) ? 1 : 0;

	base_cond = (regs->ARM_cpsr >> 13) & 0x07;

	switch (base_cond) {
	case 0x0:	
		cond = cpsr_z;
		break;

	case 0x1:	
		cond = cpsr_c;
		break;

	case 0x2:	
		cond = cpsr_n;
		break;

	case 0x3:	
		cond = cpsr_v;
		break;

	case 0x4:	
		cond = (cpsr_c == 1) && (cpsr_z == 0);
		break;

	case 0x5:	
		cond = (cpsr_n == cpsr_v);
		break;

	case 0x6:	
		cond = (cpsr_z == 0) && (cpsr_n == cpsr_v);
		break;

	case 0x7:	
		cond = 1;
		break;
	};

	if (cond == p1) {
		pr_debug("Conditional abort fixup, PC=%08x, base=%d, cond=%d\n",
			 (unsigned int) regs->ARM_pc, base_cond, cond);
		regs->ARM_pc += 2;
		return 1;
	}
	return 0;
}
#endif

asmlinkage void __exception
do_DataAbort(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
	const struct fsr_info *inf = fsr_info + fsr_fs(fsr);
	struct siginfo info;

#ifdef CONFIG_EMULATE_DOMAIN_MANAGER_V7
	if (emulate_domain_manager_data_abort(fsr, addr))
		return;
#endif

#ifdef CONFIG_MSM_KRAIT_TBB_ABORT_HANDLER
	if (krait_tbb_fixup(fsr, regs))
		return;
#endif

	if (!inf->fn(addr, fsr & ~FSR_LNX_PF, regs))
		return;

	printk(KERN_ALERT "Unhandled fault: %s (0x%03x) at 0x%08lx\n",
		inf->name, fsr, addr);

	info.si_signo = inf->sig;
	info.si_errno = 0;
	info.si_code  = inf->code;
	info.si_addr  = (void __user *)addr;
	arm_notify_die("", regs, &info, fsr, 0);
}

void __init
hook_ifault_code(int nr, int (*fn)(unsigned long, unsigned int, struct pt_regs *),
		 int sig, int code, const char *name)
{
	if (nr < 0 || nr >= ARRAY_SIZE(ifsr_info))
		BUG();

	ifsr_info[nr].fn   = fn;
	ifsr_info[nr].sig  = sig;
	ifsr_info[nr].code = code;
	ifsr_info[nr].name = name;
}

asmlinkage void __exception
do_PrefetchAbort(unsigned long addr, unsigned int ifsr, struct pt_regs *regs)
{
	const struct fsr_info *inf = ifsr_info + fsr_fs(ifsr);
	struct siginfo info;

#ifdef CONFIG_EMULATE_DOMAIN_MANAGER_V7
	if (emulate_domain_manager_prefetch_abort(ifsr, addr))
		return;
#endif

	if (!inf->fn(addr, ifsr | FSR_LNX_PF, regs))
		return;

	printk(KERN_ALERT "Unhandled prefetch abort: %s (0x%03x) at 0x%08lx\n",
		inf->name, ifsr, addr);

	info.si_signo = inf->sig;
	info.si_errno = 0;
	info.si_code  = inf->code;
	info.si_addr  = (void __user *)addr;
	arm_notify_die("", regs, &info, ifsr, 0);
}

#ifndef CONFIG_ARM_LPAE
static int __init exceptions_init(void)
{
	if (cpu_architecture() >= CPU_ARCH_ARMv6) {
		hook_fault_code(4, do_translation_fault, SIGSEGV, SEGV_MAPERR,
				"I-cache maintenance fault");
	}

	if (cpu_architecture() >= CPU_ARCH_ARMv7) {
		hook_fault_code(3, do_bad, SIGSEGV, SEGV_MAPERR,
				"section access flag fault");
		hook_fault_code(6, do_bad, SIGSEGV, SEGV_MAPERR,
				"section access flag fault");
	}

	return 0;
}

arch_initcall(exceptions_init);
#endif
