/*
 *  linux/arch/arm/vfp/vfpmodule.c
 *
 *  Copyright (C) 2004 ARM Limited.
 *  Written by Deep Blue Solutions Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/types.h>
#include <linux/cpu.h>
#include <linux/cpu_pm.h>
#include <linux/hardirq.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/user.h>
#include <linux/proc_fs.h>

#include <asm/cp15.h>
#include <asm/cputype.h>
#include <asm/system_info.h>
#include <asm/thread_notify.h>
#include <asm/vfp.h>

#include "vfpinstr.h"
#include "vfp.h"

void vfp_testing_entry(void);
void vfp_support_entry(void);
void vfp_null_entry(void);

void (*vfp_vector)(void) = vfp_null_entry;

unsigned int VFP_arch;

union vfp_state *vfp_current_hw_state[NR_CPUS];

static bool vfp_state_in_hw(unsigned int cpu, struct thread_info *thread)
{
#ifdef CONFIG_SMP
	if (thread->vfpstate.hard.cpu != cpu)
		return false;
#endif
	return vfp_current_hw_state[cpu] == &thread->vfpstate;
}

static void vfp_force_reload(unsigned int cpu, struct thread_info *thread)
{
	if (vfp_state_in_hw(cpu, thread)) {
		fmxr(FPEXC, fmrx(FPEXC) & ~FPEXC_EN);
		vfp_current_hw_state[cpu] = NULL;
	}
#ifdef CONFIG_SMP
	thread->vfpstate.hard.cpu = NR_CPUS;
	vfp_current_hw_state[cpu] = NULL;
#endif
}

static atomic64_t vfp_bounce_count = ATOMIC64_INIT(0);

static void vfp_thread_flush(struct thread_info *thread)
{
	union vfp_state *vfp = &thread->vfpstate;
	unsigned int cpu;

	cpu = get_cpu();
	if (vfp_current_hw_state[cpu] == vfp)
		vfp_current_hw_state[cpu] = NULL;
	fmxr(FPEXC, fmrx(FPEXC) & ~FPEXC_EN);
	put_cpu();

	memset(vfp, 0, sizeof(union vfp_state));

	vfp->hard.fpexc = FPEXC_EN;
	vfp->hard.fpscr = FPSCR_ROUND_NEAREST;
#ifdef CONFIG_SMP
	vfp->hard.cpu = NR_CPUS;
#endif
}

static void vfp_thread_exit(struct thread_info *thread)
{
	
	union vfp_state *vfp = &thread->vfpstate;
	unsigned int cpu = get_cpu();

	if (vfp_current_hw_state[cpu] == vfp)
		vfp_current_hw_state[cpu] = NULL;
	put_cpu();
}

static void vfp_thread_copy(struct thread_info *thread)
{
	struct thread_info *parent = current_thread_info();

	vfp_sync_hwstate(parent);
	thread->vfpstate = parent->vfpstate;
#ifdef CONFIG_SMP
	thread->vfpstate.hard.cpu = NR_CPUS;
#endif
}

static int vfp_notifier(struct notifier_block *self, unsigned long cmd, void *v)
{
	struct thread_info *thread = v;
	u32 fpexc;
#ifdef CONFIG_SMP
	unsigned int cpu;
#endif

	switch (cmd) {
	case THREAD_NOTIFY_SWITCH:
		fpexc = fmrx(FPEXC);

#ifdef CONFIG_SMP
		cpu = thread->cpu;

		if ((fpexc & FPEXC_EN) && vfp_current_hw_state[cpu])
			vfp_save_state(vfp_current_hw_state[cpu], fpexc);
#endif

		fmxr(FPEXC, fpexc & ~FPEXC_EN);
		break;

	case THREAD_NOTIFY_FLUSH:
		vfp_thread_flush(thread);
		break;

	case THREAD_NOTIFY_EXIT:
		vfp_thread_exit(thread);
		break;

	case THREAD_NOTIFY_COPY:
		vfp_thread_copy(thread);
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block vfp_notifier_block = {
	.notifier_call	= vfp_notifier,
};

static void vfp_raise_sigfpe(unsigned int sicode, struct pt_regs *regs)
{
	siginfo_t info;

	memset(&info, 0, sizeof(info));

	info.si_signo = SIGFPE;
	info.si_code = sicode;
	info.si_addr = (void __user *)(instruction_pointer(regs) - 4);

	current->thread.error_code = 0;
	current->thread.trap_no = 6;

	send_sig_info(SIGFPE, &info, current);
}

static void vfp_panic(char *reason, u32 inst)
{
	int i;

	printk(KERN_ERR "VFP: Error: %s\n", reason);
	printk(KERN_ERR "VFP: EXC 0x%08x SCR 0x%08x INST 0x%08x\n",
		fmrx(FPEXC), fmrx(FPSCR), inst);
	for (i = 0; i < 32; i += 2)
		printk(KERN_ERR "VFP: s%2u: 0x%08x s%2u: 0x%08x\n",
		       i, vfp_get_float(i), i+1, vfp_get_float(i+1));
}

static void vfp_raise_exceptions(u32 exceptions, u32 inst, u32 fpscr, struct pt_regs *regs)
{
	int si_code = 0;

	pr_debug("VFP: raising exceptions %08x\n", exceptions);

	if (exceptions == VFP_EXCEPTION_ERROR) {
		vfp_panic("unhandled bounce", inst);
		vfp_raise_sigfpe(0, regs);
		return;
	}

	if (exceptions & (FPSCR_N|FPSCR_Z|FPSCR_C|FPSCR_V))
		fpscr &= ~(FPSCR_N|FPSCR_Z|FPSCR_C|FPSCR_V);

	fpscr |= exceptions;

	fmxr(FPSCR, fpscr);

#define RAISE(stat,en,sig)				\
	if (exceptions & stat && fpscr & en)		\
		si_code = sig;

	RAISE(FPSCR_DZC, FPSCR_DZE, FPE_FLTDIV);
	RAISE(FPSCR_IXC, FPSCR_IXE, FPE_FLTRES);
	RAISE(FPSCR_UFC, FPSCR_UFE, FPE_FLTUND);
	RAISE(FPSCR_OFC, FPSCR_OFE, FPE_FLTOVF);
	RAISE(FPSCR_IOC, FPSCR_IOE, FPE_FLTINV);

	if (si_code)
		vfp_raise_sigfpe(si_code, regs);
}

static u32 vfp_emulate_instruction(u32 inst, u32 fpscr, struct pt_regs *regs)
{
	u32 exceptions = VFP_EXCEPTION_ERROR;

	pr_debug("VFP: emulate: INST=0x%08x SCR=0x%08x\n", inst, fpscr);

	if (INST_CPRTDO(inst)) {
		if (!INST_CPRT(inst)) {
			if (vfp_single(inst)) {
				exceptions = vfp_single_cpdo(inst, fpscr);
			} else {
				exceptions = vfp_double_cpdo(inst, fpscr);
			}
		} else {
		}
	} else {
	}
	return exceptions & ~VFP_NAN_FLAG;
}

void VFP_bounce(u32 trigger, u32 fpexc, struct pt_regs *regs)
{
	u32 fpscr, orig_fpscr, fpsid, exceptions;

	pr_debug("VFP: bounce: trigger %08x fpexc %08x\n", trigger, fpexc);
	atomic64_inc(&vfp_bounce_count);

	fmxr(FPEXC, fpexc & ~(FPEXC_EX|FPEXC_DEX|FPEXC_FP2V|FPEXC_VV|FPEXC_TRAP_MASK));

	fpsid = fmrx(FPSID);
	orig_fpscr = fpscr = fmrx(FPSCR);

	if ((fpsid & FPSID_ARCH_MASK) == (1 << FPSID_ARCH_BIT)
	    && (fpscr & FPSCR_IXE)) {
		goto emulate;
	}

	if (fpexc & FPEXC_EX) {
#ifndef CONFIG_CPU_FEROCEON
		trigger = fmrx(FPINST);
		regs->ARM_pc -= 4;
#endif
	} else if (!(fpexc & FPEXC_DEX)) {
		 vfp_raise_exceptions(VFP_EXCEPTION_ERROR, trigger, fpscr, regs);
		goto exit;
	}

	if (fpexc & (FPEXC_EX | FPEXC_VV)) {
		u32 len;

		len = fpexc + (1 << FPEXC_LENGTH_BIT);

		fpscr &= ~FPSCR_LENGTH_MASK;
		fpscr |= (len & FPEXC_LENGTH_MASK) << (FPSCR_LENGTH_BIT - FPEXC_LENGTH_BIT);
	}

	exceptions = vfp_emulate_instruction(trigger, fpscr, regs);
	if (exceptions)
		vfp_raise_exceptions(exceptions, trigger, orig_fpscr, regs);

	if (fpexc ^ (FPEXC_EX | FPEXC_FP2V))
		goto exit;

	barrier();
	trigger = fmrx(FPINST2);

 emulate:
	exceptions = vfp_emulate_instruction(trigger, orig_fpscr, regs);
	if (exceptions)
		vfp_raise_exceptions(exceptions, trigger, orig_fpscr, regs);
 exit:
	preempt_enable();
}

static void vfp_enable(void *unused)
{
	u32 access;

	BUG_ON(preemptible());
	access = get_copro_access();

	set_copro_access(access | CPACC_FULL(10) | CPACC_FULL(11));
}

#ifdef CONFIG_CPU_PM
int vfp_pm_suspend(void)
{
	struct thread_info *ti = current_thread_info();
	u32 fpexc = fmrx(FPEXC);

	
	if (fpexc & FPEXC_EN) {
		printk(KERN_DEBUG "%s: saving vfp state\n", __func__);
		vfp_save_state(&ti->vfpstate, fpexc);

		
		fmxr(FPEXC, fmrx(FPEXC) & ~FPEXC_EN);
	} else if (vfp_current_hw_state[ti->cpu]) {
#ifndef CONFIG_SMP
		fmxr(FPEXC, fpexc | FPEXC_EN);
		vfp_save_state(vfp_current_hw_state[ti->cpu], fpexc);
		fmxr(FPEXC, fpexc);
#endif
	}

	
	vfp_current_hw_state[ti->cpu] = NULL;

	return 0;
}

void vfp_pm_resume(void)
{
	
	vfp_enable(NULL);

	
	fmxr(FPEXC, fmrx(FPEXC) & ~FPEXC_EN);
}

static int vfp_cpu_pm_notifier(struct notifier_block *self, unsigned long cmd,
	void *v)
{
	switch (cmd) {
	case CPU_PM_ENTER:
		vfp_pm_suspend();
		break;
	case CPU_PM_ENTER_FAILED:
	case CPU_PM_EXIT:
		vfp_pm_resume();
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block vfp_cpu_pm_notifier_block = {
	.notifier_call = vfp_cpu_pm_notifier,
};

static void vfp_pm_init(void)
{
	cpu_pm_register_notifier(&vfp_cpu_pm_notifier_block);
}

#else
static inline void vfp_pm_init(void) { }
#endif 

void vfp_sync_hwstate(struct thread_info *thread)
{
	unsigned int cpu = get_cpu();

	if (vfp_state_in_hw(cpu, thread)) {
		u32 fpexc = fmrx(FPEXC);

		fmxr(FPEXC, fpexc | FPEXC_EN);
		vfp_save_state(&thread->vfpstate, fpexc | FPEXC_EN);
		fmxr(FPEXC, fpexc);
	}

	put_cpu();
}

void vfp_flush_hwstate(struct thread_info *thread)
{
	unsigned int cpu = get_cpu();

	vfp_force_reload(cpu, thread);

	put_cpu();
}

int vfp_preserve_user_clear_hwstate(struct user_vfp __user *ufp,
				    struct user_vfp_exc __user *ufp_exc)
{
	struct thread_info *thread = current_thread_info();
	struct vfp_hard_struct *hwstate = &thread->vfpstate.hard;
	int err = 0;

	
	vfp_sync_hwstate(thread);

	err |= __copy_to_user(&ufp->fpregs, &hwstate->fpregs,
			      sizeof(hwstate->fpregs));
	__put_user_error(hwstate->fpscr, &ufp->fpscr, err);

	__put_user_error(hwstate->fpexc, &ufp_exc->fpexc, err);
	__put_user_error(hwstate->fpinst, &ufp_exc->fpinst, err);
	__put_user_error(hwstate->fpinst2, &ufp_exc->fpinst2, err);

	if (err)
		return -EFAULT;

	
	vfp_flush_hwstate(thread);

	hwstate->fpscr &= ~(FPSCR_LENGTH_MASK | FPSCR_STRIDE_MASK);
	return 0;
}

int vfp_restore_user_hwstate(struct user_vfp __user *ufp,
			     struct user_vfp_exc __user *ufp_exc)
{
	struct thread_info *thread = current_thread_info();
	struct vfp_hard_struct *hwstate = &thread->vfpstate.hard;
	unsigned long fpexc;
	int err = 0;

	
	vfp_flush_hwstate(thread);

	err |= __copy_from_user(&hwstate->fpregs, &ufp->fpregs,
				sizeof(hwstate->fpregs));
	__get_user_error(hwstate->fpscr, &ufp->fpscr, err);

	__get_user_error(fpexc, &ufp_exc->fpexc, err);

	
	fpexc |= FPEXC_EN;

	
	fpexc &= ~(FPEXC_EX | FPEXC_FP2V);
	hwstate->fpexc = fpexc;

	__get_user_error(hwstate->fpinst, &ufp_exc->fpinst, err);
	__get_user_error(hwstate->fpinst2, &ufp_exc->fpinst2, err);

	return err ? -EFAULT : 0;
}

static int vfp_hotplug(struct notifier_block *b, unsigned long action,
	void *hcpu)
{
	if (action == CPU_DYING || action == CPU_DYING_FROZEN) {
		vfp_force_reload((long)hcpu, current_thread_info());
	} else if (action == CPU_STARTING || action == CPU_STARTING_FROZEN)
		vfp_enable(NULL);
	return NOTIFY_OK;
}

#ifdef CONFIG_PROC_FS
static int proc_read_status(char *page, char **start, off_t off, int count,
			    int *eof, void *data)
{
	char *p = page;
	int len;

	p += snprintf(p, PAGE_SIZE, "%llu\n", atomic64_read(&vfp_bounce_count));

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}
#endif

static int __init vfp_init(void)
{
	unsigned int vfpsid;
	unsigned int cpu_arch = cpu_architecture();
#ifdef CONFIG_PROC_FS
	static struct proc_dir_entry *procfs_entry;
#endif
	if (cpu_arch >= CPU_ARCH_ARMv6)
		on_each_cpu(vfp_enable, NULL, 1);

	vfp_vector = vfp_testing_entry;
	barrier();
	vfpsid = fmrx(FPSID);
	barrier();
	vfp_vector = vfp_null_entry;

	printk(KERN_INFO "VFP support v0.3: ");
	if (VFP_arch)
		printk("not present\n");
	else if (vfpsid & FPSID_NODOUBLE) {
		printk("no double precision support\n");
	} else {
		hotcpu_notifier(vfp_hotplug, 0);

		VFP_arch = (vfpsid & FPSID_ARCH_MASK) >> FPSID_ARCH_BIT;  
		printk("implementor %02x architecture %d part %02x variant %x rev %x\n",
			(vfpsid & FPSID_IMPLEMENTER_MASK) >> FPSID_IMPLEMENTER_BIT,
			(vfpsid & FPSID_ARCH_MASK) >> FPSID_ARCH_BIT,
			(vfpsid & FPSID_PART_MASK) >> FPSID_PART_BIT,
			(vfpsid & FPSID_VARIANT_MASK) >> FPSID_VARIANT_BIT,
			(vfpsid & FPSID_REV_MASK) >> FPSID_REV_BIT);

		vfp_vector = vfp_support_entry;

		thread_register_notifier(&vfp_notifier_block);
		vfp_pm_init();

		elf_hwcap |= HWCAP_VFP;
#ifdef CONFIG_VFPv3
		if (VFP_arch >= 2) {
			elf_hwcap |= HWCAP_VFPv3;

			if (((fmrx(MVFR0) & MVFR0_A_SIMD_MASK)) == 1)
				elf_hwcap |= HWCAP_VFPv3D16;
		}
#endif
		if ((read_cpuid_id() & 0x000f0000) == 0x000f0000) {
#ifdef CONFIG_NEON
			if ((fmrx(MVFR1) & 0x000fff00) == 0x00011100)
				elf_hwcap |= HWCAP_NEON;
#endif
			if ((fmrx(MVFR1) & 0xf0000000) == 0x10000000 ||
			    (read_cpuid_id() & 0xff00fc00) == 0x51000400)
				elf_hwcap |= HWCAP_VFPv4;
		}
	}

#ifdef CONFIG_PROC_FS
	procfs_entry = create_proc_entry("cpu/vfp_bounce", S_IRUGO, NULL);

	if (procfs_entry)
		procfs_entry->read_proc = proc_read_status;
	else
		pr_err("Failed to create procfs node for VFP bounce reporting\n");
#endif

	return 0;
}

late_initcall(vfp_init);
