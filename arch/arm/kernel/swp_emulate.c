/*
 *  linux/arch/arm/kernel/swp_emulate.c
 *
 *  Copyright (C) 2009 ARM Limited
 *  __user_* functions adapted from include/asm/uaccess.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Implements emulation of the SWP/SWPB instructions using load-exclusive and
 *  store-exclusive for processors that have them disabled (or future ones that
 *  might not implement them).
 *
 *  Syntax of SWP{B} instruction: SWP{B}<c> <Rt>, <Rt2>, [<Rn>]
 *  Where: Rt  = destination
 *	   Rt2 = source
 *	   Rn  = address
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/perf_event.h>

#include <asm/opcodes.h>
#include <asm/traps.h>
#include <asm/uaccess.h>

#define __user_swpX_asm(data, addr, res, temp, B)		\
	__asm__ __volatile__(					\
	"	mov		%2, %1\n"			\
	"0:	ldrex"B"	%1, [%3]\n"			\
	"1:	strex"B"	%0, %2, [%3]\n"			\
	"	cmp		%0, #0\n"			\
	"	movne		%0, %4\n"			\
	"2:\n"							\
	"	.section	 .fixup,\"ax\"\n"		\
	"	.align		2\n"				\
	"3:	mov		%0, %5\n"			\
	"	b		2b\n"				\
	"	.previous\n"					\
	"	.section	 __ex_table,\"a\"\n"		\
	"	.align		3\n"				\
	"	.long		0b, 3b\n"			\
	"	.long		1b, 3b\n"			\
	"	.previous"					\
	: "=&r" (res), "+r" (data), "=&r" (temp)		\
	: "r" (addr), "i" (-EAGAIN), "i" (-EFAULT)		\
	: "cc", "memory")

#define __user_swp_asm(data, addr, res, temp) \
	__user_swpX_asm(data, addr, res, temp, "")
#define __user_swpb_asm(data, addr, res, temp) \
	__user_swpX_asm(data, addr, res, temp, "b")

#define EXTRACT_REG_NUM(instruction, offset) \
	(((instruction) & (0xf << (offset))) >> (offset))
#define RN_OFFSET  16
#define RT_OFFSET  12
#define RT2_OFFSET  0
#define TYPE_SWPB (1 << 22)

static unsigned long swpcounter;
static unsigned long swpbcounter;
static unsigned long abtcounter;
static pid_t         previous_pid;

#ifdef CONFIG_PROC_FS
static int proc_read_status(char *page, char **start, off_t off, int count,
			    int *eof, void *data)
{
	char *p = page;
	int len;

	p += sprintf(p, "Emulated SWP:\t\t%lu\n", swpcounter);
	p += sprintf(p, "Emulated SWPB:\t\t%lu\n", swpbcounter);
	p += sprintf(p, "Aborted SWP{B}:\t\t%lu\n", abtcounter);
	if (previous_pid != 0)
		p += sprintf(p, "Last process:\t\t%d\n", previous_pid);

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}
#endif

static void set_segfault(struct pt_regs *regs, unsigned long addr)
{
	siginfo_t info;

	if (find_vma(current->mm, addr) == NULL)
		info.si_code = SEGV_MAPERR;
	else
		info.si_code = SEGV_ACCERR;

	info.si_signo = SIGSEGV;
	info.si_errno = 0;
	info.si_addr  = (void *) instruction_pointer(regs);

	pr_debug("SWP{B} emulation: access caused memory abort!\n");
	arm_notify_die("Illegal memory access", regs, &info, 0, 0);

	abtcounter++;
}

static int emulate_swpX(unsigned int address, unsigned int *data,
			unsigned int type)
{
	unsigned int res = 0;

	if ((type != TYPE_SWPB) && (address & 0x3)) {
		
		pr_debug("SWP instruction on unaligned pointer!\n");
		return -EFAULT;
	}

	while (1) {
		unsigned long temp;

		smp_mb();

		if (type == TYPE_SWPB)
			__user_swpb_asm(*data, address, res, temp);
		else
			__user_swp_asm(*data, address, res, temp);

		if (likely(res != -EAGAIN) || signal_pending(current))
			break;

		cond_resched();
	}

	if (res == 0) {
		smp_mb();

		if (type == TYPE_SWPB)
			swpbcounter++;
		else
			swpcounter++;
	}

	return res;
}

static int check_condition(struct pt_regs *regs, unsigned int insn)
{
	unsigned int base_cond, neg, cond = 0;
	unsigned int cpsr_z, cpsr_c, cpsr_n, cpsr_v;

	cpsr_n = (regs->ARM_cpsr & PSR_N_BIT) ? 1 : 0;
	cpsr_z = (regs->ARM_cpsr & PSR_Z_BIT) ? 1 : 0;
	cpsr_c = (regs->ARM_cpsr & PSR_C_BIT) ? 1 : 0;
	cpsr_v = (regs->ARM_cpsr & PSR_V_BIT) ? 1 : 0;

	
	base_cond = insn >> 29;
	neg = insn & BIT(28) ? 1 : 0;

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

	return cond && !neg;
}

static int swp_handler(struct pt_regs *regs, unsigned int instr)
{
	unsigned int address, destreg, data, type;
	unsigned int res = 0;

	perf_sw_event(PERF_COUNT_SW_EMULATION_FAULTS, 1, regs, regs->ARM_pc);

	res = arm_check_condition(instr, regs->ARM_cpsr);
	switch (res) {
	case ARM_OPCODE_CONDTEST_PASS:
		break;
	case ARM_OPCODE_CONDTEST_FAIL:
		
		regs->ARM_pc += 4;
		return 0;
	case ARM_OPCODE_CONDTEST_UNCOND:
		
		return -EFAULT;
	default:
		return -EINVAL;
	}

	if (current->pid != previous_pid) {
		pr_debug("\"%s\" (%ld) uses deprecated SWP{B} instruction\n",
			 current->comm, (unsigned long)current->pid);
		previous_pid = current->pid;
	}

	
	if (!check_condition(regs, instr)) {
		regs->ARM_pc += 4;
		return 0;
	}

	address = regs->uregs[EXTRACT_REG_NUM(instr, RN_OFFSET)];
	data	= regs->uregs[EXTRACT_REG_NUM(instr, RT2_OFFSET)];
	destreg = EXTRACT_REG_NUM(instr, RT_OFFSET);

	type = instr & TYPE_SWPB;

	pr_debug("addr in r%d->0x%08x, dest is r%d, source in r%d->0x%08x)\n",
		 EXTRACT_REG_NUM(instr, RN_OFFSET), address,
		 destreg, EXTRACT_REG_NUM(instr, RT2_OFFSET), data);

	
	if (!access_ok(VERIFY_WRITE, (address & ~3), 4)) {
		pr_debug("SWP{B} emulation: access to %p not allowed!\n",
			 (void *)address);
		res = -EFAULT;
	} else {
		res = emulate_swpX(address, &data, type);
	}

	if (res == 0) {
		regs->ARM_pc += 4;
		regs->uregs[destreg] = data;
	} else if (res == -EFAULT) {
		set_segfault(regs, address);
	}

	return 0;
}

static struct undef_hook swp_hook = {
	.instr_mask = 0x0fb00ff0,
	.instr_val  = 0x01000090,
	.cpsr_mask  = MODE_MASK | PSR_T_BIT | PSR_J_BIT,
	.cpsr_val   = USR_MODE,
	.fn	    = swp_handler
};

static int __init swp_emulation_init(void)
{
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *res;

	res = create_proc_entry("cpu/swp_emulation", S_IRUGO, NULL);

	if (!res)
		return -ENOMEM;

	res->read_proc = proc_read_status;
#endif 

	printk(KERN_NOTICE "Registering SWP/SWPB emulation handler\n");
	register_undef_hook(&swp_hook);

	return 0;
}

late_initcall(swp_emulation_init);
