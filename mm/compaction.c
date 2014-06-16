/*
 * linux/mm/compaction.c
 *
 * Memory compaction for the reduction of external fragmentation. Note that
 * this heavily depends upon page migration to do all the real heavy
 * lifting
 *
 * Copyright IBM Corp. 2007-2010 Mel Gorman <mel@csn.ul.ie>
 */
#include <linux/swap.h>
#include <linux/migrate.h>
#include <linux/compaction.h>
#include <linux/mm_inline.h>
#include <linux/backing-dev.h>
#include <linux/sysctl.h>
#include <linux/sysfs.h>
#include "internal.h"

#if defined CONFIG_COMPACTION || defined CONFIG_CMA

#define CREATE_TRACE_POINTS
#include <trace/events/compaction.h>

static unsigned long release_freepages(struct list_head *freelist)
{
	struct page *page, *next;
	unsigned long count = 0;

	list_for_each_entry_safe(page, next, freelist, lru) {
		list_del(&page->lru);
		__free_page(page);
		count++;
	}

	return count;
}

static void map_pages(struct list_head *list)
{
	struct page *page;

	list_for_each_entry(page, list, lru) {
		arch_alloc_page(page, 0);
		kernel_map_pages(page, 1, 1);
	}
}

static inline bool migrate_async_suitable(int migratetype)
{
	return is_migrate_cma(migratetype) || migratetype == MIGRATE_MOVABLE;
}

static unsigned long isolate_freepages_block(unsigned long blockpfn,
				unsigned long end_pfn,
				struct list_head *freelist,
				bool strict)
{
	int nr_scanned = 0, total_isolated = 0;
	struct page *cursor;

	cursor = pfn_to_page(blockpfn);

	
	for (; blockpfn < end_pfn; blockpfn++, cursor++) {
		int isolated, i;
		struct page *page = cursor;

		if (!pfn_valid_within(blockpfn)) {
			if (strict)
				return 0;
			continue;
		}
		nr_scanned++;

		if (!PageBuddy(page)) {
			if (strict)
				return 0;
			continue;
		}

		
		isolated = split_free_page(page);
		if (!isolated && strict)
			return 0;
		total_isolated += isolated;
		for (i = 0; i < isolated; i++) {
			list_add(&page->lru, freelist);
			page++;
		}

		
		if (isolated) {
			blockpfn += isolated - 1;
			cursor += isolated - 1;
		}
	}

	trace_mm_compaction_isolate_freepages(nr_scanned, total_isolated);
	return total_isolated;
}

unsigned long
isolate_freepages_range(unsigned long start_pfn, unsigned long end_pfn)
{
	unsigned long isolated, pfn, block_end_pfn, flags;
	struct zone *zone = NULL;
	LIST_HEAD(freelist);

	if (pfn_valid(start_pfn))
		zone = page_zone(pfn_to_page(start_pfn));

	for (pfn = start_pfn; pfn < end_pfn; pfn += isolated) {
		if (!pfn_valid(pfn) || zone != page_zone(pfn_to_page(pfn)))
			break;

		block_end_pfn = ALIGN(pfn + 1, pageblock_nr_pages);
		block_end_pfn = min(block_end_pfn, end_pfn);

		spin_lock_irqsave(&zone->lock, flags);
		isolated = isolate_freepages_block(pfn, block_end_pfn,
						   &freelist, true);
		spin_unlock_irqrestore(&zone->lock, flags);

		if (!isolated)
			break;

	}

	
	map_pages(&freelist);

	if (pfn < end_pfn) {
		
		release_freepages(&freelist);
		return 0;
	}

	
	return pfn;
}

static void acct_isolated(struct zone *zone, struct compact_control *cc)
{
	struct page *page;
	unsigned int count[2] = { 0, };

	list_for_each_entry(page, &cc->migratepages, lru)
		count[!!page_is_file_cache(page)]++;

	__mod_zone_page_state(zone, NR_ISOLATED_ANON, count[0]);
	__mod_zone_page_state(zone, NR_ISOLATED_FILE, count[1]);
}

static bool too_many_isolated(struct zone *zone)
{
	unsigned long active, inactive, isolated;

	inactive = zone_page_state(zone, NR_INACTIVE_FILE) +
					zone_page_state(zone, NR_INACTIVE_ANON);
	active = zone_page_state(zone, NR_ACTIVE_FILE) +
					zone_page_state(zone, NR_ACTIVE_ANON);
	isolated = zone_page_state(zone, NR_ISOLATED_FILE) +
					zone_page_state(zone, NR_ISOLATED_ANON);

	return isolated > (inactive + active) / 2;
}

unsigned long
isolate_migratepages_range(struct zone *zone, struct compact_control *cc,
			   unsigned long low_pfn, unsigned long end_pfn)
{
	unsigned long last_pageblock_nr = 0, pageblock_nr;
	unsigned long nr_scanned = 0, nr_isolated = 0;
	struct list_head *migratelist = &cc->migratepages;
	isolate_mode_t mode = ISOLATE_ACTIVE|ISOLATE_INACTIVE;

	while (unlikely(too_many_isolated(zone))) {
		
		if (!cc->sync)
			return 0;

		congestion_wait(BLK_RW_ASYNC, HZ/10);

		if (fatal_signal_pending(current))
			return 0;
	}

	
	cond_resched();
	spin_lock_irq(&zone->lru_lock);
	for (; low_pfn < end_pfn; low_pfn++) {
		struct page *page;
		bool locked = true;

		
		if (!((low_pfn+1) % SWAP_CLUSTER_MAX)) {
			spin_unlock_irq(&zone->lru_lock);
			locked = false;
		}
		if (need_resched() || spin_is_contended(&zone->lru_lock)) {
			if (locked)
				spin_unlock_irq(&zone->lru_lock);
			cond_resched();
			spin_lock_irq(&zone->lru_lock);
			if (fatal_signal_pending(current))
				break;
		} else if (!locked)
			spin_lock_irq(&zone->lru_lock);

		if ((low_pfn & (MAX_ORDER_NR_PAGES - 1)) == 0) {
			if (!pfn_valid(low_pfn)) {
				low_pfn += MAX_ORDER_NR_PAGES - 1;
				continue;
			}
		}

		if (!pfn_valid_within(low_pfn))
			continue;
		nr_scanned++;

		page = pfn_to_page(low_pfn);
		if (page_zone(page) != zone)
			continue;

		
		if (PageBuddy(page))
			continue;

		pageblock_nr = low_pfn >> pageblock_order;
		if (!cc->sync && last_pageblock_nr != pageblock_nr &&
		    !migrate_async_suitable(get_pageblock_migratetype(page))) {
			low_pfn += pageblock_nr_pages;
			low_pfn = ALIGN(low_pfn, pageblock_nr_pages) - 1;
			last_pageblock_nr = pageblock_nr;
			continue;
		}

		if (!PageLRU(page))
			continue;

		if (PageTransHuge(page)) {
			low_pfn += (1 << compound_order(page)) - 1;
			continue;
		}

		if (!cc->sync)
			mode |= ISOLATE_ASYNC_MIGRATE;

		
		if (__isolate_lru_page(page, mode, 0) != 0)
			continue;

		VM_BUG_ON(PageTransCompound(page));

		
		del_page_from_lru_list(zone, page, page_lru(page));
		list_add(&page->lru, migratelist);
		cc->nr_migratepages++;
		nr_isolated++;

		
		if (cc->nr_migratepages == COMPACT_CLUSTER_MAX) {
			++low_pfn;
			break;
		}
	}

	acct_isolated(zone, cc);

	spin_unlock_irq(&zone->lru_lock);

	trace_mm_compaction_isolate_migratepages(nr_scanned, nr_isolated);

	return low_pfn;
}

#endif 
#ifdef CONFIG_COMPACTION

static bool suitable_migration_target(struct page *page)
{

	int migratetype = get_pageblock_migratetype(page);

	
	if (migratetype == MIGRATE_ISOLATE || migratetype == MIGRATE_RESERVE)
		return false;

	
	if (PageBuddy(page) && page_order(page) >= pageblock_order)
		return true;

	
	if (migrate_async_suitable(migratetype))
		return true;

	
	return false;
}

static void isolate_freepages(struct zone *zone,
				struct compact_control *cc)
{
	struct page *page;
	unsigned long high_pfn, low_pfn, pfn, zone_end_pfn, end_pfn;
	unsigned long flags;
	int nr_freepages = cc->nr_freepages;
	struct list_head *freelist = &cc->freepages;

	pfn = cc->free_pfn;
	low_pfn = cc->migrate_pfn + pageblock_nr_pages;

	high_pfn = min(low_pfn, pfn);

	zone_end_pfn = zone->zone_start_pfn + zone->spanned_pages;

	for (; pfn > low_pfn && cc->nr_migratepages > nr_freepages;
					pfn -= pageblock_nr_pages) {
		unsigned long isolated;

		if (!pfn_valid(pfn))
			continue;

		page = pfn_to_page(pfn);
		if (page_zone(page) != zone)
			continue;

		
		if (!suitable_migration_target(page))
			continue;

		isolated = 0;
		spin_lock_irqsave(&zone->lock, flags);
		if (suitable_migration_target(page)) {
			end_pfn = min(pfn + pageblock_nr_pages, zone_end_pfn);
			isolated = isolate_freepages_block(pfn, end_pfn,
							   freelist, false);
			nr_freepages += isolated;
		}
		spin_unlock_irqrestore(&zone->lock, flags);

		if (isolated)
			high_pfn = max(high_pfn, pfn);
	}

	
	map_pages(freelist);

	cc->free_pfn = high_pfn;
	cc->nr_freepages = nr_freepages;
}

static struct page *compaction_alloc(struct page *migratepage,
					unsigned long data,
					int **result)
{
	struct compact_control *cc = (struct compact_control *)data;
	struct page *freepage;

	
	if (list_empty(&cc->freepages)) {
		isolate_freepages(cc->zone, cc);

		if (list_empty(&cc->freepages))
			return NULL;
	}

	freepage = list_entry(cc->freepages.next, struct page, lru);
	list_del(&freepage->lru);
	cc->nr_freepages--;

	return freepage;
}

static void update_nr_listpages(struct compact_control *cc)
{
	int nr_migratepages = 0;
	int nr_freepages = 0;
	struct page *page;

	list_for_each_entry(page, &cc->migratepages, lru)
		nr_migratepages++;
	list_for_each_entry(page, &cc->freepages, lru)
		nr_freepages++;

	cc->nr_migratepages = nr_migratepages;
	cc->nr_freepages = nr_freepages;
}

typedef enum {
	ISOLATE_ABORT,		
	ISOLATE_NONE,		
	ISOLATE_SUCCESS,	
} isolate_migrate_t;

static isolate_migrate_t isolate_migratepages(struct zone *zone,
					struct compact_control *cc)
{
	unsigned long low_pfn, end_pfn;

	
	low_pfn = max(cc->migrate_pfn, zone->zone_start_pfn);

	
	end_pfn = ALIGN(low_pfn + pageblock_nr_pages, pageblock_nr_pages);

	
	if (end_pfn > cc->free_pfn || !pfn_valid(low_pfn)) {
		cc->migrate_pfn = end_pfn;
		return ISOLATE_NONE;
	}

	
	low_pfn = isolate_migratepages_range(zone, cc, low_pfn, end_pfn);
	if (!low_pfn)
		return ISOLATE_ABORT;

	cc->migrate_pfn = low_pfn;

	return ISOLATE_SUCCESS;
}

static int compact_finished(struct zone *zone,
			    struct compact_control *cc)
{
	unsigned int order;
	unsigned long watermark;

	if (fatal_signal_pending(current))
		return COMPACT_PARTIAL;

	
	if (cc->free_pfn <= cc->migrate_pfn)
		return COMPACT_COMPLETE;

	if (cc->order == -1)
		return COMPACT_CONTINUE;

	
	watermark = low_wmark_pages(zone);
	watermark += (1 << cc->order);

	if (!zone_watermark_ok(zone, cc->order, watermark, 0, 0))
		return COMPACT_CONTINUE;

	
	for (order = cc->order; order < MAX_ORDER; order++) {
		
		if (!list_empty(&zone->free_area[order].free_list[cc->migratetype]))
			return COMPACT_PARTIAL;

		
		if (order >= pageblock_order && zone->free_area[order].nr_free)
			return COMPACT_PARTIAL;
	}

	return COMPACT_CONTINUE;
}

unsigned long compaction_suitable(struct zone *zone, int order)
{
	int fragindex;
	unsigned long watermark;

	if (order == -1)
		return COMPACT_CONTINUE;

	watermark = low_wmark_pages(zone) + (2UL << order);
	if (!zone_watermark_ok(zone, 0, watermark, 0, 0))
		return COMPACT_SKIPPED;

	fragindex = fragmentation_index(zone, order);
	if (fragindex >= 0 && fragindex <= sysctl_extfrag_threshold)
		return COMPACT_SKIPPED;

	if (fragindex == -1000 && zone_watermark_ok(zone, order, watermark,
	    0, 0))
		return COMPACT_PARTIAL;

	return COMPACT_CONTINUE;
}

static int compact_zone(struct zone *zone, struct compact_control *cc)
{
	int ret;

	ret = compaction_suitable(zone, cc->order);
	switch (ret) {
	case COMPACT_PARTIAL:
	case COMPACT_SKIPPED:
		
		return ret;
	case COMPACT_CONTINUE:
		
		;
	}

	
	cc->migrate_pfn = zone->zone_start_pfn;
	cc->free_pfn = cc->migrate_pfn + zone->spanned_pages;
	cc->free_pfn &= ~(pageblock_nr_pages-1);

	migrate_prep_local();

	while ((ret = compact_finished(zone, cc)) == COMPACT_CONTINUE) {
		unsigned long nr_migrate, nr_remaining;
		int err;

		switch (isolate_migratepages(zone, cc)) {
		case ISOLATE_ABORT:
			ret = COMPACT_PARTIAL;
			goto out;
		case ISOLATE_NONE:
			continue;
		case ISOLATE_SUCCESS:
			;
		}

		nr_migrate = cc->nr_migratepages;
		err = migrate_pages(&cc->migratepages, compaction_alloc,
				(unsigned long)cc, false,
				cc->sync ? MIGRATE_SYNC_LIGHT : MIGRATE_ASYNC);
		update_nr_listpages(cc);
		nr_remaining = cc->nr_migratepages;

		count_vm_event(COMPACTBLOCKS);
		count_vm_events(COMPACTPAGES, nr_migrate - nr_remaining);
		if (nr_remaining)
			count_vm_events(COMPACTPAGEFAILED, nr_remaining);
		trace_mm_compaction_migratepages(nr_migrate - nr_remaining,
						nr_remaining);

		
		if (err) {
			putback_lru_pages(&cc->migratepages);
			cc->nr_migratepages = 0;
			if (err == -ENOMEM) {
				ret = COMPACT_PARTIAL;
				goto out;
			}
		}
	}

out:
	
	cc->nr_freepages -= release_freepages(&cc->freepages);
	VM_BUG_ON(cc->nr_freepages != 0);

	return ret;
}

static unsigned long compact_zone_order(struct zone *zone,
				 int order, gfp_t gfp_mask,
				 bool sync)
{
	struct compact_control cc = {
		.nr_freepages = 0,
		.nr_migratepages = 0,
		.order = order,
		.migratetype = allocflags_to_migratetype(gfp_mask),
		.zone = zone,
		.sync = sync,
	};
	INIT_LIST_HEAD(&cc.freepages);
	INIT_LIST_HEAD(&cc.migratepages);

	return compact_zone(zone, &cc);
}

int sysctl_extfrag_threshold = 500;

unsigned long try_to_compact_pages(struct zonelist *zonelist,
			int order, gfp_t gfp_mask, nodemask_t *nodemask,
			bool sync)
{
	enum zone_type high_zoneidx = gfp_zone(gfp_mask);
	int may_enter_fs = gfp_mask & __GFP_FS;
	int may_perform_io = gfp_mask & __GFP_IO;
	struct zoneref *z;
	struct zone *zone;
	int rc = COMPACT_SKIPPED;
	int alloc_flags = 0;

	if (!order || !may_enter_fs || !may_perform_io)
		return rc;

	count_vm_event(COMPACTSTALL);

#ifdef CONFIG_CMA
	if (allocflags_to_migratetype(gfp_mask) == MIGRATE_MOVABLE)
		alloc_flags |= ALLOC_CMA;
#endif
	
	for_each_zone_zonelist_nodemask(zone, z, zonelist, high_zoneidx,
								nodemask) {
		int status;

		status = compact_zone_order(zone, order, gfp_mask, sync);
		rc = max(status, rc);

		
		if (zone_watermark_ok(zone, order, low_wmark_pages(zone), 0,
				      alloc_flags))
			break;
	}

	return rc;
}


static int __compact_pgdat(pg_data_t *pgdat, struct compact_control *cc)
{
	int zoneid;
	struct zone *zone;

	for (zoneid = 0; zoneid < MAX_NR_ZONES; zoneid++) {

		zone = &pgdat->node_zones[zoneid];
		if (!populated_zone(zone))
			continue;

		cc->nr_freepages = 0;
		cc->nr_migratepages = 0;
		cc->zone = zone;
		INIT_LIST_HEAD(&cc->freepages);
		INIT_LIST_HEAD(&cc->migratepages);

		if (cc->order == -1 || !compaction_deferred(zone, cc->order))
			compact_zone(zone, cc);

		if (cc->order > 0) {
			int ok = zone_watermark_ok(zone, cc->order,
						low_wmark_pages(zone), 0, 0);
			if (ok && cc->order > zone->compact_order_failed)
				zone->compact_order_failed = cc->order + 1;
			
			else if (!ok && cc->sync)
				defer_compaction(zone, cc->order);
		}

		VM_BUG_ON(!list_empty(&cc->freepages));
		VM_BUG_ON(!list_empty(&cc->migratepages));
	}

	return 0;
}

int compact_pgdat(pg_data_t *pgdat, int order)
{
	struct compact_control cc = {
		.order = order,
		.sync = false,
	};

	return __compact_pgdat(pgdat, &cc);
}

static int compact_node(int nid)
{
	struct compact_control cc = {
		.order = -1,
		.sync = true,
	};

	return __compact_pgdat(NODE_DATA(nid), &cc);
}

static int compact_nodes(void)
{
	int nid;

	
	lru_add_drain_all();

	for_each_online_node(nid)
		compact_node(nid);

	return COMPACT_COMPLETE;
}

/* The written value is actually unused, all memory is compacted */
int sysctl_compact_memory;

int sysctl_compaction_handler(struct ctl_table *table, int write,
			void __user *buffer, size_t *length, loff_t *ppos)
{
	if (write)
		return compact_nodes();

	return 0;
}

int sysctl_extfrag_handler(struct ctl_table *table, int write,
			void __user *buffer, size_t *length, loff_t *ppos)
{
	proc_dointvec_minmax(table, write, buffer, length, ppos);

	return 0;
}

#if defined(CONFIG_SYSFS) && defined(CONFIG_NUMA)
ssize_t sysfs_compact_node(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	int nid = dev->id;

	if (nid >= 0 && nid < nr_node_ids && node_online(nid)) {
		
		lru_add_drain_all();

		compact_node(nid);
	}

	return count;
}
static DEVICE_ATTR(compact, S_IWUSR, NULL, sysfs_compact_node);

int compaction_register_node(struct node *node)
{
	return device_create_file(&node->dev, &dev_attr_compact);
}

void compaction_unregister_node(struct node *node)
{
	return device_remove_file(&node->dev, &dev_attr_compact);
}
#endif 

#endif 
