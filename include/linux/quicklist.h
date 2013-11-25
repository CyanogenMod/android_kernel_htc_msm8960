#ifndef LINUX_QUICKLIST_H
#define LINUX_QUICKLIST_H
#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/percpu.h>

#ifdef CONFIG_QUICKLIST

struct quicklist {
	void *page;
	int nr_pages;
};

DECLARE_PER_CPU(struct quicklist, quicklist)[CONFIG_NR_QUICK];

static inline void *quicklist_alloc(int nr, gfp_t flags, void (*ctor)(void *))
{
	struct quicklist *q;
	void **p = NULL;

	q =&get_cpu_var(quicklist)[nr];
	p = q->page;
	if (likely(p)) {
		q->page = p[0];
		p[0] = NULL;
		q->nr_pages--;
	}
	put_cpu_var(quicklist);
	if (likely(p))
		return p;

	p = (void *)__get_free_page(flags | __GFP_ZERO);
	if (ctor && p)
		ctor(p);
	return p;
}

static inline void __quicklist_free(int nr, void (*dtor)(void *), void *p,
	struct page *page)
{
	struct quicklist *q;

	q = &get_cpu_var(quicklist)[nr];
	*(void **)p = q->page;
	q->page = p;
	q->nr_pages++;
	put_cpu_var(quicklist);
}

static inline void quicklist_free(int nr, void (*dtor)(void *), void *pp)
{
	__quicklist_free(nr, dtor, pp, virt_to_page(pp));
}

static inline void quicklist_free_page(int nr, void (*dtor)(void *),
							struct page *page)
{
	__quicklist_free(nr, dtor, page_address(page), page);
}

void quicklist_trim(int nr, void (*dtor)(void *),
	unsigned long min_pages, unsigned long max_free);

unsigned long quicklist_total_size(void);

#else

static inline unsigned long quicklist_total_size(void)
{
	return 0;
}

#endif

#endif 

