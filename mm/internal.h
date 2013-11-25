/* internal.h: mm/ internal definitions
 *
 * Copyright (C) 2004 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#ifndef __MM_INTERNAL_H
#define __MM_INTERNAL_H

#include <linux/mm.h>

void free_pgtables(struct mmu_gather *tlb, struct vm_area_struct *start_vma,
		unsigned long floor, unsigned long ceiling);

static inline void set_page_count(struct page *page, int v)
{
	atomic_set(&page->_count, v);
}

static inline void set_page_refcounted(struct page *page)
{
	VM_BUG_ON(PageTail(page));
	VM_BUG_ON(atomic_read(&page->_count));
	set_page_count(page, 1);
}

static inline void __put_page(struct page *page)
{
	atomic_dec(&page->_count);
}

static inline void __get_page_tail_foll(struct page *page,
					bool get_page_head)
{
	VM_BUG_ON(atomic_read(&page->first_page->_count) <= 0);
	VM_BUG_ON(atomic_read(&page->_count) != 0);
	VM_BUG_ON(page_mapcount(page) < 0);
	if (get_page_head)
		atomic_inc(&page->first_page->_count);
	atomic_inc(&page->_mapcount);
}

static inline void get_page_foll(struct page *page)
{
	if (unlikely(PageTail(page)))
		__get_page_tail_foll(page, true);
	else {
		VM_BUG_ON(atomic_read(&page->_count) <= 0);
		atomic_inc(&page->_count);
	}
}

extern unsigned long highest_memmap_pfn;

extern int isolate_lru_page(struct page *page);
extern void putback_lru_page(struct page *page);

extern void __free_pages_bootmem(struct page *page, unsigned int order);
extern void prep_compound_page(struct page *page, unsigned long order);
#ifdef CONFIG_MEMORY_FAILURE
extern bool is_free_buddy_page(struct page *page);
#endif

#if defined CONFIG_COMPACTION || defined CONFIG_CMA

struct compact_control {
	struct list_head freepages;	
	struct list_head migratepages;	
	unsigned long nr_freepages;	
	unsigned long nr_migratepages;	
	unsigned long free_pfn;		
	unsigned long migrate_pfn;	
	bool sync;			

	int order;			
	int migratetype;		
	struct zone *zone;
};

unsigned long
isolate_freepages_range(unsigned long start_pfn, unsigned long end_pfn);
unsigned long
isolate_migratepages_range(struct zone *zone, struct compact_control *cc,
			   unsigned long low_pfn, unsigned long end_pfn);

#endif

static inline unsigned long page_order(struct page *page)
{
	
	return page_private(page);
}

void __vma_link_list(struct mm_struct *mm, struct vm_area_struct *vma,
		struct vm_area_struct *prev, struct rb_node *rb_parent);

#ifdef CONFIG_MMU
extern long mlock_vma_pages_range(struct vm_area_struct *vma,
			unsigned long start, unsigned long end);
extern void munlock_vma_pages_range(struct vm_area_struct *vma,
			unsigned long start, unsigned long end);
static inline void munlock_vma_pages_all(struct vm_area_struct *vma)
{
	munlock_vma_pages_range(vma, vma->vm_start, vma->vm_end);
}

static inline int is_mlocked_vma(struct vm_area_struct *vma, struct page *page)
{
	VM_BUG_ON(PageLRU(page));

	if (likely((vma->vm_flags & (VM_LOCKED | VM_SPECIAL)) != VM_LOCKED))
		return 0;

	if (!TestSetPageMlocked(page)) {
		inc_zone_page_state(page, NR_MLOCK);
		count_vm_event(UNEVICTABLE_PGMLOCKED);
	}
	return 1;
}

extern void mlock_vma_page(struct page *page);
extern void munlock_vma_page(struct page *page);

extern void __clear_page_mlock(struct page *page);
static inline void clear_page_mlock(struct page *page)
{
	if (unlikely(TestClearPageMlocked(page)))
		__clear_page_mlock(page);
}

static inline void mlock_migrate_page(struct page *newpage, struct page *page)
{
	if (TestClearPageMlocked(page)) {
		unsigned long flags;

		local_irq_save(flags);
		__dec_zone_page_state(page, NR_MLOCK);
		SetPageMlocked(newpage);
		__inc_zone_page_state(newpage, NR_MLOCK);
		local_irq_restore(flags);
	}
}

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
extern unsigned long vma_address(struct page *page,
				 struct vm_area_struct *vma);
#endif
#else 
static inline int is_mlocked_vma(struct vm_area_struct *v, struct page *p)
{
	return 0;
}
static inline void clear_page_mlock(struct page *page) { }
static inline void mlock_vma_page(struct page *page) { }
static inline void mlock_migrate_page(struct page *new, struct page *old) { }

#endif 

static inline struct page *mem_map_offset(struct page *base, int offset)
{
	if (unlikely(offset >= MAX_ORDER_NR_PAGES))
		return pfn_to_page(page_to_pfn(base) + offset);
	return base + offset;
}

static inline struct page *mem_map_next(struct page *iter,
						struct page *base, int offset)
{
	if (unlikely((offset & (MAX_ORDER_NR_PAGES - 1)) == 0)) {
		unsigned long pfn = page_to_pfn(base) + offset;
		if (!pfn_valid(pfn))
			return NULL;
		return pfn_to_page(pfn);
	}
	return iter + 1;
}

#ifdef CONFIG_SPARSEMEM
#define __paginginit __meminit
#else
#define __paginginit __init
#endif

enum mminit_level {
	MMINIT_WARNING,
	MMINIT_VERIFY,
	MMINIT_TRACE
};

#ifdef CONFIG_DEBUG_MEMORY_INIT

extern int mminit_loglevel;

#define mminit_dprintk(level, prefix, fmt, arg...) \
do { \
	if (level < mminit_loglevel) { \
		printk(level <= MMINIT_WARNING ? KERN_WARNING : KERN_DEBUG); \
		printk(KERN_CONT "mminit::" prefix " " fmt, ##arg); \
	} \
} while (0)

extern void mminit_verify_pageflags_layout(void);
extern void mminit_verify_page_links(struct page *page,
		enum zone_type zone, unsigned long nid, unsigned long pfn);
extern void mminit_verify_zonelist(void);

#else

static inline void mminit_dprintk(enum mminit_level level,
				const char *prefix, const char *fmt, ...)
{
}

static inline void mminit_verify_pageflags_layout(void)
{
}

static inline void mminit_verify_page_links(struct page *page,
		enum zone_type zone, unsigned long nid, unsigned long pfn)
{
}

static inline void mminit_verify_zonelist(void)
{
}
#endif 

#if defined(CONFIG_SPARSEMEM)
extern void mminit_validate_memmodel_limits(unsigned long *start_pfn,
				unsigned long *end_pfn);
#else
static inline void mminit_validate_memmodel_limits(unsigned long *start_pfn,
				unsigned long *end_pfn)
{
}
#endif 

#define ZONE_RECLAIM_NOSCAN	-2
#define ZONE_RECLAIM_FULL	-1
#define ZONE_RECLAIM_SOME	0
#define ZONE_RECLAIM_SUCCESS	1
#endif

extern int hwpoison_filter(struct page *p);

extern u32 hwpoison_filter_dev_major;
extern u32 hwpoison_filter_dev_minor;
extern u64 hwpoison_filter_flags_mask;
extern u64 hwpoison_filter_flags_value;
extern u64 hwpoison_filter_memcg;
extern u32 hwpoison_filter_enable;
