#ifndef _LINUX_BLOCKGROUP_LOCK_H
#define _LINUX_BLOCKGROUP_LOCK_H

#include <linux/spinlock.h>
#include <linux/cache.h>

#ifdef CONFIG_SMP


#if NR_CPUS >= 32
#define NR_BG_LOCKS	128
#elif NR_CPUS >= 16
#define NR_BG_LOCKS	64
#elif NR_CPUS >= 8
#define NR_BG_LOCKS	32
#elif NR_CPUS >= 4
#define NR_BG_LOCKS	16
#elif NR_CPUS >= 2
#define NR_BG_LOCKS	8
#else
#define NR_BG_LOCKS	4
#endif

#else	
#define NR_BG_LOCKS	1
#endif	

struct bgl_lock {
	spinlock_t lock;
} ____cacheline_aligned_in_smp;

struct blockgroup_lock {
	struct bgl_lock locks[NR_BG_LOCKS];
};

static inline void bgl_lock_init(struct blockgroup_lock *bgl)
{
	int i;

	for (i = 0; i < NR_BG_LOCKS; i++)
		spin_lock_init(&bgl->locks[i].lock);
}

static inline spinlock_t *
bgl_lock_ptr(struct blockgroup_lock *bgl, unsigned int block_group)
{
	return &bgl->locks[(block_group) & (NR_BG_LOCKS-1)].lock;
}

#endif
