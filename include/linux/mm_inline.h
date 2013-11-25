#ifndef LINUX_MM_INLINE_H
#define LINUX_MM_INLINE_H

#include <linux/huge_mm.h>

static inline int page_is_file_cache(struct page *page)
{
	return !PageSwapBacked(page);
}

static inline void
add_page_to_lru_list(struct zone *zone, struct page *page, enum lru_list lru)
{
	struct lruvec *lruvec;

	lruvec = mem_cgroup_lru_add_list(zone, page, lru);
	list_add(&page->lru, &lruvec->lists[lru]);
	__mod_zone_page_state(zone, NR_LRU_BASE + lru, hpage_nr_pages(page));
}

static inline void
del_page_from_lru_list(struct zone *zone, struct page *page, enum lru_list lru)
{
	mem_cgroup_lru_del_list(page, lru);
	list_del(&page->lru);
	__mod_zone_page_state(zone, NR_LRU_BASE + lru, -hpage_nr_pages(page));
}

static inline enum lru_list page_lru_base_type(struct page *page)
{
	if (page_is_file_cache(page))
		return LRU_INACTIVE_FILE;
	return LRU_INACTIVE_ANON;
}

static inline enum lru_list page_off_lru(struct page *page)
{
	enum lru_list lru;

	if (PageUnevictable(page)) {
		__ClearPageUnevictable(page);
		lru = LRU_UNEVICTABLE;
	} else {
		lru = page_lru_base_type(page);
		if (PageActive(page)) {
			__ClearPageActive(page);
			lru += LRU_ACTIVE;
		}
	}
	return lru;
}

static inline enum lru_list page_lru(struct page *page)
{
	enum lru_list lru;

	if (PageUnevictable(page))
		lru = LRU_UNEVICTABLE;
	else {
		lru = page_lru_base_type(page);
		if (PageActive(page))
			lru += LRU_ACTIVE;
	}
	return lru;
}

#endif
