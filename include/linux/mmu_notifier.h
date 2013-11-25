#ifndef _LINUX_MMU_NOTIFIER_H
#define _LINUX_MMU_NOTIFIER_H

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/mm_types.h>

struct mmu_notifier;
struct mmu_notifier_ops;

#ifdef CONFIG_MMU_NOTIFIER

struct mmu_notifier_mm {
	
	struct hlist_head list;
	
	spinlock_t lock;
};

struct mmu_notifier_ops {
	void (*release)(struct mmu_notifier *mn,
			struct mm_struct *mm);

	int (*clear_flush_young)(struct mmu_notifier *mn,
				 struct mm_struct *mm,
				 unsigned long address);

	int (*test_young)(struct mmu_notifier *mn,
			  struct mm_struct *mm,
			  unsigned long address);

	void (*change_pte)(struct mmu_notifier *mn,
			   struct mm_struct *mm,
			   unsigned long address,
			   pte_t pte);

	void (*invalidate_page)(struct mmu_notifier *mn,
				struct mm_struct *mm,
				unsigned long address);

	void (*invalidate_range_start)(struct mmu_notifier *mn,
				       struct mm_struct *mm,
				       unsigned long start, unsigned long end);
	void (*invalidate_range_end)(struct mmu_notifier *mn,
				     struct mm_struct *mm,
				     unsigned long start, unsigned long end);
};

struct mmu_notifier {
	struct hlist_node hlist;
	const struct mmu_notifier_ops *ops;
};

static inline int mm_has_notifiers(struct mm_struct *mm)
{
	return unlikely(mm->mmu_notifier_mm);
}

extern int mmu_notifier_register(struct mmu_notifier *mn,
				 struct mm_struct *mm);
extern int __mmu_notifier_register(struct mmu_notifier *mn,
				   struct mm_struct *mm);
extern void mmu_notifier_unregister(struct mmu_notifier *mn,
				    struct mm_struct *mm);
extern void __mmu_notifier_mm_destroy(struct mm_struct *mm);
extern void __mmu_notifier_release(struct mm_struct *mm);
extern int __mmu_notifier_clear_flush_young(struct mm_struct *mm,
					  unsigned long address);
extern int __mmu_notifier_test_young(struct mm_struct *mm,
				     unsigned long address);
extern void __mmu_notifier_change_pte(struct mm_struct *mm,
				      unsigned long address, pte_t pte);
extern void __mmu_notifier_invalidate_page(struct mm_struct *mm,
					  unsigned long address);
extern void __mmu_notifier_invalidate_range_start(struct mm_struct *mm,
				  unsigned long start, unsigned long end);
extern void __mmu_notifier_invalidate_range_end(struct mm_struct *mm,
				  unsigned long start, unsigned long end);

static inline void mmu_notifier_release(struct mm_struct *mm)
{
	if (mm_has_notifiers(mm))
		__mmu_notifier_release(mm);
}

static inline int mmu_notifier_clear_flush_young(struct mm_struct *mm,
					  unsigned long address)
{
	if (mm_has_notifiers(mm))
		return __mmu_notifier_clear_flush_young(mm, address);
	return 0;
}

static inline int mmu_notifier_test_young(struct mm_struct *mm,
					  unsigned long address)
{
	if (mm_has_notifiers(mm))
		return __mmu_notifier_test_young(mm, address);
	return 0;
}

static inline void mmu_notifier_change_pte(struct mm_struct *mm,
					   unsigned long address, pte_t pte)
{
	if (mm_has_notifiers(mm))
		__mmu_notifier_change_pte(mm, address, pte);
}

static inline void mmu_notifier_invalidate_page(struct mm_struct *mm,
					  unsigned long address)
{
	if (mm_has_notifiers(mm))
		__mmu_notifier_invalidate_page(mm, address);
}

static inline void mmu_notifier_invalidate_range_start(struct mm_struct *mm,
				  unsigned long start, unsigned long end)
{
	if (mm_has_notifiers(mm))
		__mmu_notifier_invalidate_range_start(mm, start, end);
}

static inline void mmu_notifier_invalidate_range_end(struct mm_struct *mm,
				  unsigned long start, unsigned long end)
{
	if (mm_has_notifiers(mm))
		__mmu_notifier_invalidate_range_end(mm, start, end);
}

static inline void mmu_notifier_mm_init(struct mm_struct *mm)
{
	mm->mmu_notifier_mm = NULL;
}

static inline void mmu_notifier_mm_destroy(struct mm_struct *mm)
{
	if (mm_has_notifiers(mm))
		__mmu_notifier_mm_destroy(mm);
}

#define ptep_clear_flush_notify(__vma, __address, __ptep)		\
({									\
	pte_t __pte;							\
	struct vm_area_struct *___vma = __vma;				\
	unsigned long ___address = __address;				\
	__pte = ptep_clear_flush(___vma, ___address, __ptep);		\
	mmu_notifier_invalidate_page(___vma->vm_mm, ___address);	\
	__pte;								\
})

#define pmdp_clear_flush_notify(__vma, __address, __pmdp)		\
({									\
	pmd_t __pmd;							\
	struct vm_area_struct *___vma = __vma;				\
	unsigned long ___address = __address;				\
	VM_BUG_ON(__address & ~HPAGE_PMD_MASK);				\
	mmu_notifier_invalidate_range_start(___vma->vm_mm, ___address,	\
					    (__address)+HPAGE_PMD_SIZE);\
	__pmd = pmdp_clear_flush(___vma, ___address, __pmdp);		\
	mmu_notifier_invalidate_range_end(___vma->vm_mm, ___address,	\
					  (__address)+HPAGE_PMD_SIZE);	\
	__pmd;								\
})

#define pmdp_splitting_flush_notify(__vma, __address, __pmdp)		\
({									\
	struct vm_area_struct *___vma = __vma;				\
	unsigned long ___address = __address;				\
	VM_BUG_ON(__address & ~HPAGE_PMD_MASK);				\
	mmu_notifier_invalidate_range_start(___vma->vm_mm, ___address,	\
					    (__address)+HPAGE_PMD_SIZE);\
	pmdp_splitting_flush(___vma, ___address, __pmdp);		\
	mmu_notifier_invalidate_range_end(___vma->vm_mm, ___address,	\
					  (__address)+HPAGE_PMD_SIZE);	\
})

#define ptep_clear_flush_young_notify(__vma, __address, __ptep)		\
({									\
	int __young;							\
	struct vm_area_struct *___vma = __vma;				\
	unsigned long ___address = __address;				\
	__young = ptep_clear_flush_young(___vma, ___address, __ptep);	\
	__young |= mmu_notifier_clear_flush_young(___vma->vm_mm,	\
						  ___address);		\
	__young;							\
})

#define pmdp_clear_flush_young_notify(__vma, __address, __pmdp)		\
({									\
	int __young;							\
	struct vm_area_struct *___vma = __vma;				\
	unsigned long ___address = __address;				\
	__young = pmdp_clear_flush_young(___vma, ___address, __pmdp);	\
	__young |= mmu_notifier_clear_flush_young(___vma->vm_mm,	\
						  ___address);		\
	__young;							\
})

#define set_pte_at_notify(__mm, __address, __ptep, __pte)		\
({									\
	struct mm_struct *___mm = __mm;					\
	unsigned long ___address = __address;				\
	pte_t ___pte = __pte;						\
									\
	set_pte_at(___mm, ___address, __ptep, ___pte);			\
	mmu_notifier_change_pte(___mm, ___address, ___pte);		\
})

#else 

static inline void mmu_notifier_release(struct mm_struct *mm)
{
}

static inline int mmu_notifier_clear_flush_young(struct mm_struct *mm,
					  unsigned long address)
{
	return 0;
}

static inline int mmu_notifier_test_young(struct mm_struct *mm,
					  unsigned long address)
{
	return 0;
}

static inline void mmu_notifier_change_pte(struct mm_struct *mm,
					   unsigned long address, pte_t pte)
{
}

static inline void mmu_notifier_invalidate_page(struct mm_struct *mm,
					  unsigned long address)
{
}

static inline void mmu_notifier_invalidate_range_start(struct mm_struct *mm,
				  unsigned long start, unsigned long end)
{
}

static inline void mmu_notifier_invalidate_range_end(struct mm_struct *mm,
				  unsigned long start, unsigned long end)
{
}

static inline void mmu_notifier_mm_init(struct mm_struct *mm)
{
}

static inline void mmu_notifier_mm_destroy(struct mm_struct *mm)
{
}

#define ptep_clear_flush_young_notify ptep_clear_flush_young
#define pmdp_clear_flush_young_notify pmdp_clear_flush_young
#define ptep_clear_flush_notify ptep_clear_flush
#define pmdp_clear_flush_notify pmdp_clear_flush
#define pmdp_splitting_flush_notify pmdp_splitting_flush
#define set_pte_at_notify set_pte_at

#endif 

#endif 
