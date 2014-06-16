#ifndef _HTC_DEBUG_STABILITY_REPORT_MEMINFO
#define _HTC_DEBUG_STABILITY_REPORT_MEMINFO

struct page;
struct seq_file;
struct sysinfo;

enum meminfo_stat_item {
	NR_KMALLOC_PAGES,
	NR_DMA_PAGES,
	NR_MEMINFO_STAT_ITEMS};

#ifdef CONFIG_HTC_DEBUG_REPORT_MEMINFO

void kmalloc_count(struct page *page, int to_alloc);

void mapped_count(struct page *page, int to_map);

unsigned long meminfo_total_pages(enum meminfo_stat_item item);

void inc_meminfo_total_pages(enum meminfo_stat_item item);

void dec_meminfo_total_pages(enum meminfo_stat_item item);

void add_meminfo_total_pages(enum meminfo_stat_item item, int delta);

void sub_meminfo_total_pages(enum meminfo_stat_item item, int delta);

void report_meminfo_item(struct seq_file *m, enum meminfo_stat_item item);

void report_meminfo(struct seq_file *m, struct sysinfo *sysinfo);

#else 

static inline void kmalloc_count(struct page *page, int to_alloc)
{
}

static inline void mapped_count(struct page *page, int to_map)
{
}

static inline unsigned long meminfo_total_pages(enum meminfo_stat_item item)
{
	return 0UL;
}

static inline void inc_meminfo_total_pages(enum meminfo_stat_item item)
{
}

static inline void dec_meminfo_total_pages(enum meminfo_stat_item item)
{
}

static inline void add_meminfo_total_pages(enum meminfo_stat_item item, int delta)
{
}

static inline void sub_meminfo_total_pages(enum meminfo_stat_item item, int delta)
{
}

static inline void report_meminfo_item(struct seq_file *m, enum meminfo_stat_item item)
{
}

static inline void report_meminfo(struct seq_file *m, struct sysinfo *sysinfo)
{
}

#endif 

#endif 
