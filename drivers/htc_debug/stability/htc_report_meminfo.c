#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/seq_file.h>
#include <linux/swap.h>
#include <linux/fs.h>
#include <linux/msm_kgsl.h>
#include <htc_debug/stability/htc_report_meminfo.h>

static atomic_long_t meminfo_stat[NR_MEMINFO_STAT_ITEMS] = {ATOMIC_LONG_INIT(0)};

const char * const meminfo_stat_text[] = {
	[NR_KMALLOC_PAGES] = "Kmalloc:",
	[NR_DMA_PAGES] = "DMA ALLOC:"
};

static atomic_long_t cached_mapped_stat = ATOMIC_LONG_INIT(0);

static atomic_long_t kgsl_mapped_stat = ATOMIC_LONG_INIT(0);

extern unsigned long ftrace_total_pages(void);

void kmalloc_count(struct page *page, int to_alloc)
{
	if (unlikely(!page))
		return;

	if (to_alloc) {
		int order = compound_order(page);

		SetPageKmalloc(page);
		add_meminfo_total_pages(NR_KMALLOC_PAGES, 1 << order);
	} else if (PageKmalloc(page)) {
		int order = compound_order(page);

		ClearPageKmalloc(page);
		sub_meminfo_total_pages(NR_KMALLOC_PAGES, 1 << order);
	}
}

struct super_block;
extern int sb_is_blkdev_sb(struct super_block *sb);

static inline int page_is_cached(struct page *page)
{
	return	page_mapping(page) && !PageSwapCache(page) &&
		page->mapping->host && page->mapping->host->i_sb &&
		!sb_is_blkdev_sb(page->mapping->host->i_sb);
}

void mapped_count(struct page *page, int to_map)
{
	if (unlikely(!page))
		return;

	if (PageKgsl(page)) {
		if (to_map)
			atomic_long_inc(&kgsl_mapped_stat);
		else
			atomic_long_dec(&kgsl_mapped_stat);
	} else if (page_is_cached(page)) {
		if (to_map)
			atomic_long_inc(&cached_mapped_stat);
		else
			atomic_long_dec(&cached_mapped_stat);
	}
}

unsigned long meminfo_total_pages(enum meminfo_stat_item item)
{
	long total = atomic_long_read(&meminfo_stat[item]);

	if (total < 0)
		total = 0;
	return (unsigned long) total;
}

void inc_meminfo_total_pages(enum meminfo_stat_item item)
{
	atomic_long_inc(&meminfo_stat[item]);
}

void dec_meminfo_total_pages(enum meminfo_stat_item item)
{
	atomic_long_dec(&meminfo_stat[item]);
}

void add_meminfo_total_pages(enum meminfo_stat_item item, int delta)
{
	atomic_long_add(delta, &meminfo_stat[item]);
}

void sub_meminfo_total_pages(enum meminfo_stat_item item, int delta)
{
	atomic_long_sub(delta, &meminfo_stat[item]);
}

void report_meminfo_item(struct seq_file *m, enum meminfo_stat_item item)
{
	unsigned long total = meminfo_total_pages(item);

#define K(x) ((x) << (PAGE_SHIFT - 10))
	seq_printf(m, "%-16s%8lu kB\n", meminfo_stat_text[item], K(total));
#undef K
}

static void report_unmapped(struct seq_file *m, struct sysinfo *sysinfo)
{
	long cached, cached_unmapped, kgsl_unmapped;
	unsigned long kgsl_alloc = kgsl_get_alloc_size(1);

	cached = global_page_state(NR_FILE_PAGES) -
		total_swapcache_pages - sysinfo->bufferram;

	if (cached < 0)
		cached = 0;

	cached_unmapped = cached - atomic_long_read(&cached_mapped_stat);

	if (cached_unmapped < 0)
		cached_unmapped = 0;

	kgsl_unmapped = (kgsl_alloc >> PAGE_SHIFT) - atomic_long_read(&kgsl_mapped_stat);

	if (kgsl_unmapped < 0)
		kgsl_unmapped = 0;

#define K(x) ((x) << (PAGE_SHIFT - 10))
	seq_printf(m, "%-16s%8lu kB\n", "CachedUnmapped: ", K(cached_unmapped));
	seq_printf(m, "%-16s%8lu kB\n", "KgslUnmapped: ", K(kgsl_unmapped));
#undef K
}

void report_meminfo(struct seq_file *m, struct sysinfo *sysinfo)
{
	int i;

	for (i = 0; i < NR_MEMINFO_STAT_ITEMS; i++)
		report_meminfo_item(m, i);

	report_unmapped(m, sysinfo);

#define K(x) ((x) << (PAGE_SHIFT - 10))
	seq_printf(m, "%-16s%8lu kB\n", "Ftrace: ", K(ftrace_total_pages()));
#undef K
}
