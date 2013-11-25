#ifndef __ASM_ARM_SWAB_H
#define __ASM_ARM_SWAB_H

#include <linux/compiler.h>
#include <linux/types.h>

#if !defined(__STRICT_ANSI__) || defined(__KERNEL__)
#  define __SWAB_64_THRU_32__
#endif

#if defined(__KERNEL__)
#if __LINUX_ARM_ARCH__ >= 6

static inline __attribute_const__ __u32 __arch_swahb32(__u32 x)
{
	__asm__ ("rev16 %0, %1" : "=r" (x) : "r" (x));
	return x;
}
#define __arch_swahb32 __arch_swahb32
#define __arch_swab16(x) ((__u16)__arch_swahb32(x))

static inline __attribute_const__ __u32 __arch_swab32(__u32 x)
{
	__asm__ ("rev %0, %1" : "=r" (x) : "r" (x));
	return x;
}
#define __arch_swab32 __arch_swab32

#endif
#endif

#if !defined(__KERNEL__) || __LINUX_ARM_ARCH__ < 6
static inline __attribute_const__ __u32 __arch_swab32(__u32 x)
{
	__u32 t;

#ifndef __thumb__
	if (!__builtin_constant_p(x)) {
		asm ("eor\t%0, %1, %1, ror #16" : "=r" (t) : "r" (x));
	} else
#endif
		t = x ^ ((x << 16) | (x >> 16)); 

	x = (x << 24) | (x >> 8);		
	t &= ~0x00FF0000;			
	x ^= (t >> 8);				

	return x;
}
#define __arch_swab32 __arch_swab32

#endif

#endif
