#ifndef __LINUX_PAGEISOLATION_H
#define __LINUX_PAGEISOLATION_H

extern int
start_isolate_page_range(unsigned long start_pfn, unsigned long end_pfn,
			 unsigned migratetype);

extern int
undo_isolate_page_range(unsigned long start_pfn, unsigned long end_pfn,
			unsigned migratetype);

int test_pages_isolated(unsigned long start_pfn, unsigned long end_pfn);

extern int set_migratetype_isolate(struct page *page);
extern void unset_migratetype_isolate(struct page *page, unsigned migratetype);


#endif
