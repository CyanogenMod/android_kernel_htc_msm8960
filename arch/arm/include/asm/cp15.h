#ifndef __ASM_ARM_CP15_H
#define __ASM_ARM_CP15_H

#include <asm/barrier.h>

#define CR_M	(1 << 0)	
#define CR_A	(1 << 1)	
#define CR_C	(1 << 2)	
#define CR_W	(1 << 3)	
#define CR_P	(1 << 4)	
#define CR_D	(1 << 5)	
#define CR_L	(1 << 6)	
#define CR_B	(1 << 7)	
#define CR_S	(1 << 8)	
#define CR_R	(1 << 9)	
#define CR_F	(1 << 10)	
#define CR_Z	(1 << 11)	
#define CR_I	(1 << 12)	
#define CR_V	(1 << 13)	
#define CR_RR	(1 << 14)	
#define CR_L4	(1 << 15)	
#define CR_DT	(1 << 16)
#define CR_IT	(1 << 18)
#define CR_ST	(1 << 19)
#define CR_FI	(1 << 21)	
#define CR_U	(1 << 22)	
#define CR_XP	(1 << 23)	
#define CR_VE	(1 << 24)	
#define CR_EE	(1 << 25)	
#define CR_TRE	(1 << 28)	
#define CR_AFE	(1 << 29)	
#define CR_TE	(1 << 30)	

#ifndef __ASSEMBLY__

#if __LINUX_ARM_ARCH__ >= 4
#define vectors_high()	(cr_alignment & CR_V)
#else
#define vectors_high()	(0)
#endif

extern unsigned long cr_no_alignment;	
extern unsigned long cr_alignment;	

static inline unsigned int get_cr(void)
{
	unsigned int val;
	asm("mrc p15, 0, %0, c1, c0, 0	@ get CR" : "=r" (val) : : "cc");
	return val;
}

static inline void set_cr(unsigned int val)
{
	asm volatile("mcr p15, 0, %0, c1, c0, 0	@ set CR"
	  : : "r" (val) : "cc");
	isb();
}

#ifndef CONFIG_SMP
extern void adjust_cr(unsigned long mask, unsigned long set);
#endif

#define CPACC_FULL(n)		(3 << (n * 2))
#define CPACC_SVC(n)		(1 << (n * 2))
#define CPACC_DISABLE(n)	(0 << (n * 2))

static inline unsigned int get_copro_access(void)
{
	unsigned int val;
	asm("mrc p15, 0, %0, c1, c0, 2 @ get copro access"
	  : "=r" (val) : : "cc");
	return val;
}

static inline void set_copro_access(unsigned int val)
{
	asm volatile("mcr p15, 0, %0, c1, c0, 2 @ set copro access"
	  : : "r" (val) : "cc");
	isb();
}

#endif

#endif
