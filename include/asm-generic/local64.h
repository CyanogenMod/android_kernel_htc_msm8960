#ifndef _ASM_GENERIC_LOCAL64_H
#define _ASM_GENERIC_LOCAL64_H

#include <linux/percpu.h>
#include <asm/types.h>



#if BITS_PER_LONG == 64

#include <asm/local.h>

typedef struct {
	local_t a;
} local64_t;

#define LOCAL64_INIT(i)	{ LOCAL_INIT(i) }

#define local64_read(l)		local_read(&(l)->a)
#define local64_set(l,i)	local_set((&(l)->a),(i))
#define local64_inc(l)		local_inc(&(l)->a)
#define local64_dec(l)		local_dec(&(l)->a)
#define local64_add(i,l)	local_add((i),(&(l)->a))
#define local64_sub(i,l)	local_sub((i),(&(l)->a))

#define local64_sub_and_test(i, l) local_sub_and_test((i), (&(l)->a))
#define local64_dec_and_test(l) local_dec_and_test(&(l)->a)
#define local64_inc_and_test(l) local_inc_and_test(&(l)->a)
#define local64_add_negative(i, l) local_add_negative((i), (&(l)->a))
#define local64_add_return(i, l) local_add_return((i), (&(l)->a))
#define local64_sub_return(i, l) local_sub_return((i), (&(l)->a))
#define local64_inc_return(l)	local_inc_return(&(l)->a)

#define local64_cmpxchg(l, o, n) local_cmpxchg((&(l)->a), (o), (n))
#define local64_xchg(l, n)	local_xchg((&(l)->a), (n))
#define local64_add_unless(l, _a, u) local_add_unless((&(l)->a), (_a), (u))
#define local64_inc_not_zero(l)	local_inc_not_zero(&(l)->a)

#define __local64_inc(l)	local64_set((l), local64_read(l) + 1)
#define __local64_dec(l)	local64_set((l), local64_read(l) - 1)
#define __local64_add(i,l)	local64_set((l), local64_read(l) + (i))
#define __local64_sub(i,l)	local64_set((l), local64_read(l) - (i))

#else 

#include <linux/atomic.h>

typedef struct {
	atomic64_t a;
} local64_t;

#define LOCAL64_INIT(i)	{ ATOMIC_LONG_INIT(i) }

#define local64_read(l)		atomic64_read(&(l)->a)
#define local64_set(l,i)	atomic64_set((&(l)->a),(i))
#define local64_inc(l)		atomic64_inc(&(l)->a)
#define local64_dec(l)		atomic64_dec(&(l)->a)
#define local64_add(i,l)	atomic64_add((i),(&(l)->a))
#define local64_sub(i,l)	atomic64_sub((i),(&(l)->a))

#define local64_sub_and_test(i, l) atomic64_sub_and_test((i), (&(l)->a))
#define local64_dec_and_test(l) atomic64_dec_and_test(&(l)->a)
#define local64_inc_and_test(l) atomic64_inc_and_test(&(l)->a)
#define local64_add_negative(i, l) atomic64_add_negative((i), (&(l)->a))
#define local64_add_return(i, l) atomic64_add_return((i), (&(l)->a))
#define local64_sub_return(i, l) atomic64_sub_return((i), (&(l)->a))
#define local64_inc_return(l)	atomic64_inc_return(&(l)->a)

#define local64_cmpxchg(l, o, n) atomic64_cmpxchg((&(l)->a), (o), (n))
#define local64_xchg(l, n)	atomic64_xchg((&(l)->a), (n))
#define local64_add_unless(l, _a, u) atomic64_add_unless((&(l)->a), (_a), (u))
#define local64_inc_not_zero(l)	atomic64_inc_not_zero(&(l)->a)

#define __local64_inc(l)	local64_set((l), local64_read(l) + 1)
#define __local64_dec(l)	local64_set((l), local64_read(l) - 1)
#define __local64_add(i,l)	local64_set((l), local64_read(l) + (i))
#define __local64_sub(i,l)	local64_set((l), local64_read(l) - (i))

#endif 

#endif 
