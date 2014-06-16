
#ifndef PAGE_FLAGS_H
#define PAGE_FLAGS_H

#include <linux/types.h>
#include <linux/bug.h>
#ifndef __GENERATING_BOUNDS_H
#include <linux/mm_types.h>
#include <generated/bounds.h>
#endif 


enum pageflags {
	PG_locked,		
	PG_error,
	PG_referenced,
	PG_uptodate,
	PG_dirty,
	PG_lru,
	PG_active,
	PG_slab,
	PG_owner_priv_1,	
	PG_arch_1,
	PG_reserved,
	PG_private,		
	PG_private_2,		
	PG_writeback,		
#ifdef CONFIG_PAGEFLAGS_EXTENDED
	PG_head,		
	PG_tail,		
#else
	PG_compound,		
#endif
	PG_swapcache,		
	PG_mappedtodisk,	
	PG_reclaim,		
	PG_swapbacked,		
	PG_unevictable,		
#ifdef CONFIG_MMU
	PG_mlocked,		
#endif
#ifdef CONFIG_ARCH_USES_PG_UNCACHED
	PG_uncached,		
#endif
#ifdef CONFIG_MEMORY_FAILURE
	PG_hwpoison,		
#endif
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
	PG_compound_lock,
#endif
#ifdef CONFIG_HTC_DEBUG_REPORT_MEMINFO
	PG_kmalloc,			
	PG_kgsl,
#endif
	__NR_PAGEFLAGS,

	
	PG_checked = PG_owner_priv_1,

	PG_fscache = PG_private_2,	

	
	PG_pinned = PG_owner_priv_1,
	PG_savepinned = PG_dirty,

	
	PG_slob_free = PG_private,
};

#ifndef __GENERATING_BOUNDS_H

#define TESTPAGEFLAG(uname, lname)					\
static inline int Page##uname(const struct page *page)			\
			{ return test_bit(PG_##lname, &page->flags); }

#define SETPAGEFLAG(uname, lname)					\
static inline void SetPage##uname(struct page *page)			\
			{ set_bit(PG_##lname, &page->flags); }

#define CLEARPAGEFLAG(uname, lname)					\
static inline void ClearPage##uname(struct page *page)			\
			{ clear_bit(PG_##lname, &page->flags); }

#define __SETPAGEFLAG(uname, lname)					\
static inline void __SetPage##uname(struct page *page)			\
			{ __set_bit(PG_##lname, &page->flags); }

#define __CLEARPAGEFLAG(uname, lname)					\
static inline void __ClearPage##uname(struct page *page)		\
			{ __clear_bit(PG_##lname, &page->flags); }

#define TESTSETFLAG(uname, lname)					\
static inline int TestSetPage##uname(struct page *page)			\
		{ return test_and_set_bit(PG_##lname, &page->flags); }

#define TESTCLEARFLAG(uname, lname)					\
static inline int TestClearPage##uname(struct page *page)		\
		{ return test_and_clear_bit(PG_##lname, &page->flags); }

#define __TESTCLEARFLAG(uname, lname)					\
static inline int __TestClearPage##uname(struct page *page)		\
		{ return __test_and_clear_bit(PG_##lname, &page->flags); }

#define PAGEFLAG(uname, lname) TESTPAGEFLAG(uname, lname)		\
	SETPAGEFLAG(uname, lname) CLEARPAGEFLAG(uname, lname)

#define __PAGEFLAG(uname, lname) TESTPAGEFLAG(uname, lname)		\
	__SETPAGEFLAG(uname, lname)  __CLEARPAGEFLAG(uname, lname)

#define PAGEFLAG_FALSE(uname) 						\
static inline int Page##uname(const struct page *page)			\
			{ return 0; }

#define TESTSCFLAG(uname, lname)					\
	TESTSETFLAG(uname, lname) TESTCLEARFLAG(uname, lname)

#define SETPAGEFLAG_NOOP(uname)						\
static inline void SetPage##uname(struct page *page) {  }

#define CLEARPAGEFLAG_NOOP(uname)					\
static inline void ClearPage##uname(struct page *page) {  }

#define __CLEARPAGEFLAG_NOOP(uname)					\
static inline void __ClearPage##uname(struct page *page) {  }

#define TESTCLEARFLAG_FALSE(uname)					\
static inline int TestClearPage##uname(struct page *page) { return 0; }

#define __TESTCLEARFLAG_FALSE(uname)					\
static inline int __TestClearPage##uname(struct page *page) { return 0; }

struct page;	

TESTPAGEFLAG(Locked, locked)
PAGEFLAG(Error, error) TESTCLEARFLAG(Error, error)
PAGEFLAG(Referenced, referenced) TESTCLEARFLAG(Referenced, referenced)
PAGEFLAG(Dirty, dirty) TESTSCFLAG(Dirty, dirty) __CLEARPAGEFLAG(Dirty, dirty)
PAGEFLAG(LRU, lru) __CLEARPAGEFLAG(LRU, lru)
PAGEFLAG(Active, active) __CLEARPAGEFLAG(Active, active)
	TESTCLEARFLAG(Active, active)
__PAGEFLAG(Slab, slab)
PAGEFLAG(Checked, checked)		
PAGEFLAG(Pinned, pinned) TESTSCFLAG(Pinned, pinned)	
PAGEFLAG(SavePinned, savepinned);			
PAGEFLAG(Reserved, reserved) __CLEARPAGEFLAG(Reserved, reserved)
PAGEFLAG(SwapBacked, swapbacked) __CLEARPAGEFLAG(SwapBacked, swapbacked)

__PAGEFLAG(SlobFree, slob_free)

PAGEFLAG(Private, private) __SETPAGEFLAG(Private, private)
	__CLEARPAGEFLAG(Private, private)
PAGEFLAG(Private2, private_2) TESTSCFLAG(Private2, private_2)
PAGEFLAG(OwnerPriv1, owner_priv_1) TESTCLEARFLAG(OwnerPriv1, owner_priv_1)

TESTPAGEFLAG(Writeback, writeback) TESTSCFLAG(Writeback, writeback)
PAGEFLAG(MappedToDisk, mappedtodisk)

PAGEFLAG(Reclaim, reclaim) TESTCLEARFLAG(Reclaim, reclaim)
PAGEFLAG(Readahead, reclaim)		

#ifdef CONFIG_HIGHMEM
#define PageHighMem(__p) is_highmem(page_zone(__p))
#else
PAGEFLAG_FALSE(HighMem)
#endif

#ifdef CONFIG_SWAP
PAGEFLAG(SwapCache, swapcache)
#else
PAGEFLAG_FALSE(SwapCache)
	SETPAGEFLAG_NOOP(SwapCache) CLEARPAGEFLAG_NOOP(SwapCache)
#endif

PAGEFLAG(Unevictable, unevictable) __CLEARPAGEFLAG(Unevictable, unevictable)
	TESTCLEARFLAG(Unevictable, unevictable)

#ifdef CONFIG_MMU
PAGEFLAG(Mlocked, mlocked) __CLEARPAGEFLAG(Mlocked, mlocked)
	TESTSCFLAG(Mlocked, mlocked) __TESTCLEARFLAG(Mlocked, mlocked)
#else
PAGEFLAG_FALSE(Mlocked) SETPAGEFLAG_NOOP(Mlocked)
	TESTCLEARFLAG_FALSE(Mlocked) __TESTCLEARFLAG_FALSE(Mlocked)
#endif

#ifdef CONFIG_ARCH_USES_PG_UNCACHED
PAGEFLAG(Uncached, uncached)
#else
PAGEFLAG_FALSE(Uncached)
#endif

#ifdef CONFIG_MEMORY_FAILURE
PAGEFLAG(HWPoison, hwpoison)
TESTSCFLAG(HWPoison, hwpoison)
#define __PG_HWPOISON (1UL << PG_hwpoison)
#else
PAGEFLAG_FALSE(HWPoison)
#define __PG_HWPOISON 0
#endif

#ifdef CONFIG_HTC_DEBUG_REPORT_MEMINFO
PAGEFLAG(Kmalloc, kmalloc)
PAGEFLAG(Kgsl, kgsl)
#else
PAGEFLAG_FALSE(Kgsl) SETPAGEFLAG_NOOP(Kgsl)
	CLEARPAGEFLAG_NOOP(Kgsl)
#endif

u64 stable_page_flags(struct page *page);

static inline int PageUptodate(struct page *page)
{
	int ret = test_bit(PG_uptodate, &(page)->flags);

	if (ret)
		smp_rmb();

	return ret;
}

static inline void __SetPageUptodate(struct page *page)
{
	smp_wmb();
	__set_bit(PG_uptodate, &(page)->flags);
}

static inline void SetPageUptodate(struct page *page)
{
#ifdef CONFIG_S390
	if (!test_and_set_bit(PG_uptodate, &page->flags))
		page_set_storage_key(page_to_phys(page), PAGE_DEFAULT_KEY, 0);
#else
	smp_wmb();
	set_bit(PG_uptodate, &(page)->flags);
#endif
}

CLEARPAGEFLAG(Uptodate, uptodate)

extern void cancel_dirty_page(struct page *page, unsigned int account_size);

int test_clear_page_writeback(struct page *page);
int test_set_page_writeback(struct page *page);

static inline void set_page_writeback(struct page *page)
{
	test_set_page_writeback(page);
}

#ifdef CONFIG_PAGEFLAGS_EXTENDED
__PAGEFLAG(Head, head) CLEARPAGEFLAG(Head, head)
__PAGEFLAG(Tail, tail)

static inline int PageCompound(struct page *page)
{
	return page->flags & ((1L << PG_head) | (1L << PG_tail));

}
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
static inline void ClearPageCompound(struct page *page)
{
	BUG_ON(!PageHead(page));
	ClearPageHead(page);
}
#endif
#else
TESTPAGEFLAG(Compound, compound)
__PAGEFLAG(Head, compound)

#define PG_head_tail_mask ((1L << PG_compound) | (1L << PG_reclaim))

static inline int PageTail(struct page *page)
{
	return ((page->flags & PG_head_tail_mask) == PG_head_tail_mask);
}

static inline void __SetPageTail(struct page *page)
{
	page->flags |= PG_head_tail_mask;
}

static inline void __ClearPageTail(struct page *page)
{
	page->flags &= ~PG_head_tail_mask;
}

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
static inline void ClearPageCompound(struct page *page)
{
	BUG_ON((page->flags & PG_head_tail_mask) != (1 << PG_compound));
	clear_bit(PG_compound, &page->flags);
}
#endif

#endif 

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
static inline int PageTransHuge(struct page *page)
{
	VM_BUG_ON(PageTail(page));
	return PageHead(page);
}

static inline int PageTransCompound(struct page *page)
{
	return PageCompound(page);
}

static inline int PageTransTail(struct page *page)
{
	return PageTail(page);
}

#else

static inline int PageTransHuge(struct page *page)
{
	return 0;
}

static inline int PageTransCompound(struct page *page)
{
	return 0;
}

static inline int PageTransTail(struct page *page)
{
	return 0;
}
#endif

#ifdef CONFIG_MMU
#define __PG_MLOCKED		(1 << PG_mlocked)
#else
#define __PG_MLOCKED		0
#endif

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
#define __PG_COMPOUND_LOCK		(1 << PG_compound_lock)
#else
#define __PG_COMPOUND_LOCK		0
#endif

#define PAGE_FLAGS_CHECK_AT_FREE \
	(1 << PG_lru	 | 1 << PG_locked    | \
	 1 << PG_private | 1 << PG_private_2 | \
	 1 << PG_writeback | 1 << PG_reserved | \
	 1 << PG_slab	 | 1 << PG_swapcache | 1 << PG_active | \
	 1 << PG_unevictable | __PG_MLOCKED | __PG_HWPOISON | \
	 __PG_COMPOUND_LOCK)

#define PAGE_FLAGS_CHECK_AT_PREP	((1 << NR_PAGEFLAGS) - 1)

#define PAGE_FLAGS_PRIVATE				\
	(1 << PG_private | 1 << PG_private_2)
static inline int page_has_private(struct page *page)
{
	return !!(page->flags & PAGE_FLAGS_PRIVATE);
}

#endif 

#endif	
