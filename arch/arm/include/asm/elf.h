#ifndef __ASMARM_ELF_H
#define __ASMARM_ELF_H

#include <asm/hwcap.h>

#include <asm/ptrace.h>
#include <asm/user.h>

struct task_struct;

typedef unsigned long elf_greg_t;
typedef unsigned long elf_freg_t[3];

#define ELF_NGREG (sizeof (struct pt_regs) / sizeof(elf_greg_t))
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

typedef struct user_fp elf_fpregset_t;

#define EM_ARM	40

#define EF_ARM_EABI_MASK	0xff000000
#define EF_ARM_EABI_UNKNOWN	0x00000000
#define EF_ARM_EABI_VER1	0x01000000
#define EF_ARM_EABI_VER2	0x02000000
#define EF_ARM_EABI_VER3	0x03000000
#define EF_ARM_EABI_VER4	0x04000000
#define EF_ARM_EABI_VER5	0x05000000

#define EF_ARM_BE8		0x00800000	
#define EF_ARM_LE8		0x00400000	
#define EF_ARM_MAVERICK_FLOAT	0x00000800	
#define EF_ARM_VFP_FLOAT	0x00000400	
#define EF_ARM_SOFT_FLOAT	0x00000200	
#define EF_ARM_OLD_ABI		0x00000100	
#define EF_ARM_NEW_ABI		0x00000080	
#define EF_ARM_ALIGN8		0x00000040	
#define EF_ARM_PIC		0x00000020	
#define EF_ARM_MAPSYMSFIRST	0x00000010	
#define EF_ARM_APCS_FLOAT	0x00000010	
#define EF_ARM_DYNSYMSUSESEGIDX	0x00000008	
#define EF_ARM_APCS_26		0x00000008	
#define EF_ARM_SYMSARESORTED	0x00000004	
#define EF_ARM_INTERWORK	0x00000004	
#define EF_ARM_HASENTRY		0x00000002	
#define EF_ARM_RELEXEC		0x00000001	

#define R_ARM_NONE		0
#define R_ARM_PC24		1
#define R_ARM_ABS32		2
#define R_ARM_CALL		28
#define R_ARM_JUMP24		29
#define R_ARM_V4BX		40
#define R_ARM_PREL31		42
#define R_ARM_MOVW_ABS_NC	43
#define R_ARM_MOVT_ABS		44

#define R_ARM_THM_CALL		10
#define R_ARM_THM_JUMP24	30
#define R_ARM_THM_MOVW_ABS_NC	47
#define R_ARM_THM_MOVT_ABS	48

#define ELF_CLASS	ELFCLASS32
#ifdef __ARMEB__
#define ELF_DATA	ELFDATA2MSB
#else
#define ELF_DATA	ELFDATA2LSB
#endif
#define ELF_ARCH	EM_ARM

#define ELF_PLATFORM_SIZE 8
#define ELF_PLATFORM	(elf_platform)

extern char elf_platform[];

struct elf32_hdr;

extern int elf_check_arch(const struct elf32_hdr *);
#define elf_check_arch elf_check_arch

#define vmcore_elf64_check_arch(x) (0)

extern int arm_elf_read_implies_exec(const struct elf32_hdr *, int);
#define elf_read_implies_exec(ex,stk) arm_elf_read_implies_exec(&(ex), stk)

struct task_struct;
int dump_task_regs(struct task_struct *t, elf_gregset_t *elfregs);
#define ELF_CORE_COPY_TASK_REGS dump_task_regs

#define CORE_DUMP_USE_REGSET
#define ELF_EXEC_PAGESIZE	4096


#define ELF_ET_DYN_BASE	(2 * TASK_SIZE / 3)

#define ELF_PLAT_INIT(_r, load_addr)	(_r)->ARM_r0 = 0

extern void elf_set_personality(const struct elf32_hdr *);
#define SET_PERSONALITY(ex)	elf_set_personality(&(ex))

struct mm_struct;
extern unsigned long arch_randomize_brk(struct mm_struct *mm);
#define arch_randomize_brk arch_randomize_brk

#endif
