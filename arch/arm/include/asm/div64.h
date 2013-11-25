#ifndef __ASM_ARM_DIV64
#define __ASM_ARM_DIV64

#include <linux/types.h>
#include <asm/compiler.h>


#ifdef __ARMEB__
#define __xh "r0"
#define __xl "r1"
#else
#define __xl "r0"
#define __xh "r1"
#endif

#define __do_div_asm(n, base)					\
({								\
	register unsigned int __base      asm("r4") = base;	\
	register unsigned long long __n   asm("r0") = n;	\
	register unsigned long long __res asm("r2");		\
	register unsigned int __rem       asm(__xh);		\
	asm(	__asmeq("%0", __xh)				\
		__asmeq("%1", "r2")				\
		__asmeq("%2", "r0")				\
		__asmeq("%3", "r4")				\
		"bl	__do_div64"				\
		: "=r" (__rem), "=r" (__res)			\
		: "r" (__n), "r" (__base)			\
		: "ip", "lr", "cc");				\
	n = __res;						\
	__rem;							\
})

#if __GNUC__ < 4

#define do_div(n, base) __do_div_asm(n, base)

#elif __GNUC__ >= 4

#include <asm/bug.h>

#define do_div(n, base)							\
({									\
	unsigned int __r, __b = (base);					\
	if (!__builtin_constant_p(__b) || __b == 0 ||			\
	    (__LINUX_ARM_ARCH__ < 4 && (__b & (__b - 1)) != 0)) {	\
				\
		__r = __do_div_asm(n, __b);				\
	} else if ((__b & (__b - 1)) == 0) {				\
				\
				\
		__r = n;						\
		__r &= (__b - 1);					\
		n /= __b;						\
	} else {							\
			\
			\
			\
			\
			\
			\
		unsigned long long __res, __x, __t, __m, __n = n;	\
		unsigned int __c, __p, __z = 0;				\
			\
		__r = __n;						\
				\
		__p = 1 << __div64_fls(__b);				\
			\
		__m = (~0ULL / __b) * __p;				\
		__m += (((~0ULL % __b + 1) * __p) + __b - 1) / __b;	\
			\
		__x = ~0ULL / __b * __b - 1;				\
		__res = (__m & 0xffffffff) * (__x & 0xffffffff);	\
		__res >>= 32;						\
		__res += (__m & 0xffffffff) * (__x >> 32);		\
		__t = __res;						\
		__res += (__x & 0xffffffff) * (__m >> 32);		\
		__t = (__res < __t) ? (1ULL << 32) : 0;			\
		__res = (__res >> 32) + __t;				\
		__res += (__m >> 32) * (__x >> 32);			\
		__res /= __p;						\
				\
		if (~0ULL % (__b / (__b & -__b)) == 0) {		\
				\
			__n /= (__b & -__b);				\
			__m = ~0ULL / (__b / (__b & -__b));		\
			__p = 1;					\
			__c = 1;					\
		} else if (__res != __x / __b) {			\
				\
				\
				\
				\
				\
			__c = 1;					\
					\
			__m = (~0ULL / __b) * __p;			\
			__m += ((~0ULL % __b + 1) * __p) / __b;		\
		} else {						\
				\
				\
				\
			unsigned int __bits = -(__m & -__m);		\
			__bits |= __m >> 32;				\
			__bits = (~__bits) << 1;			\
				\
				\
				\
				\
				\
			if (!__bits) {					\
				__p /= (__m & -__m);			\
				__m /= (__m & -__m);			\
			} else {					\
				__p >>= __div64_fls(__bits);		\
				__m >>= __div64_fls(__bits);		\
			}						\
						\
			__c = 0;					\
		}							\
			\
			\
			\
			\
			\
			\
		if (!__c) {						\
			asm (	"umull	%Q0, %R0, %1, %Q2\n\t"		\
				"mov	%Q0, #0"			\
				: "=&r" (__res)				\
				: "r" (__m), "r" (__n)			\
				: "cc" );				\
		} else if (!(__m & ((1ULL << 63) | (1ULL << 31)))) {	\
			__res = __m;					\
			asm (	"umlal	%Q0, %R0, %Q1, %Q2\n\t"		\
				"mov	%Q0, #0"			\
				: "+&r" (__res)				\
				: "r" (__m), "r" (__n)			\
				: "cc" );				\
		} else {						\
			asm (	"umull	%Q0, %R0, %Q1, %Q2\n\t"		\
				"cmn	%Q0, %Q1\n\t"			\
				"adcs	%R0, %R0, %R1\n\t"		\
				"adc	%Q0, %3, #0"			\
				: "=&r" (__res)				\
				: "r" (__m), "r" (__n), "r" (__z)	\
				: "cc" );				\
		}							\
		if (!(__m & ((1ULL << 63) | (1ULL << 31)))) {		\
			asm (	"umlal	%R0, %Q0, %R1, %Q2\n\t"		\
				"umlal	%R0, %Q0, %Q1, %R2\n\t"		\
				"mov	%R0, #0\n\t"			\
				"umlal	%Q0, %R0, %R1, %R2"		\
				: "+&r" (__res)				\
				: "r" (__m), "r" (__n)			\
				: "cc" );				\
		} else {						\
			asm (	"umlal	%R0, %Q0, %R2, %Q3\n\t"		\
				"umlal	%R0, %1, %Q2, %R3\n\t"		\
				"mov	%R0, #0\n\t"			\
				"adds	%Q0, %1, %Q0\n\t"		\
				"adc	%R0, %R0, #0\n\t"		\
				"umlal	%Q0, %R0, %R2, %R3"		\
				: "+&r" (__res), "+&r" (__z)		\
				: "r" (__m), "r" (__n)			\
				: "cc" );				\
		}							\
		__res /= __p;						\
			\
			\
		{							\
			unsigned int __res0 = __res;			\
			unsigned int __b0 = __b;			\
			__r -= __res0 * __b0;				\
		}							\
			\
		n = __res;						\
	}								\
	__r;								\
})

#define __div64_fls(bits)						\
({									\
	unsigned int __left = (bits), __nr = 0;				\
	if (__left & 0xffff0000) __nr += 16, __left >>= 16;		\
	if (__left & 0x0000ff00) __nr +=  8, __left >>=  8;		\
	if (__left & 0x000000f0) __nr +=  4, __left >>=  4;		\
	if (__left & 0x0000000c) __nr +=  2, __left >>=  2;		\
	if (__left & 0x00000002) __nr +=  1;				\
	__nr;								\
})

#endif

#endif
