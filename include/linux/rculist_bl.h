#ifndef _LINUX_RCULIST_BL_H
#define _LINUX_RCULIST_BL_H

#include <linux/list_bl.h>
#include <linux/rcupdate.h>

static inline void hlist_bl_set_first_rcu(struct hlist_bl_head *h,
					struct hlist_bl_node *n)
{
	LIST_BL_BUG_ON((unsigned long)n & LIST_BL_LOCKMASK);
	LIST_BL_BUG_ON(((unsigned long)h->first & LIST_BL_LOCKMASK) !=
							LIST_BL_LOCKMASK);
	rcu_assign_pointer(h->first,
		(struct hlist_bl_node *)((unsigned long)n | LIST_BL_LOCKMASK));
}

static inline struct hlist_bl_node *hlist_bl_first_rcu(struct hlist_bl_head *h)
{
	return (struct hlist_bl_node *)
		((unsigned long)rcu_dereference(h->first) & ~LIST_BL_LOCKMASK);
}

static inline void hlist_bl_del_init_rcu(struct hlist_bl_node *n)
{
	if (!hlist_bl_unhashed(n)) {
		__hlist_bl_del(n);
		n->pprev = NULL;
	}
}

static inline void hlist_bl_del_rcu(struct hlist_bl_node *n)
{
	__hlist_bl_del(n);
	n->pprev = LIST_POISON2;
}

static inline void hlist_bl_add_head_rcu(struct hlist_bl_node *n,
					struct hlist_bl_head *h)
{
	struct hlist_bl_node *first;

	
	first = hlist_bl_first(h);

	n->next = first;
	if (first)
		first->pprev = &n->next;
	n->pprev = &h->first;

	
	hlist_bl_set_first_rcu(h, n);
}
#define hlist_bl_for_each_entry_rcu(tpos, pos, head, member)		\
	for (pos = hlist_bl_first_rcu(head);				\
		pos &&							\
		({ tpos = hlist_bl_entry(pos, typeof(*tpos), member); 1; }); \
		pos = rcu_dereference_raw(pos->next))

#endif
