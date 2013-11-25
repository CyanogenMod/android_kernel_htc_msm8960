/*
 * mm/prio_tree.c - priority search tree for mapping->i_mmap
 *
 * Copyright (C) 2004, Rajesh Venkatasubramanian <vrajesh@umich.edu>
 *
 * This file is released under the GPL v2.
 *
 * Based on the radix priority search tree proposed by Edward M. McCreight
 * SIAM Journal of Computing, vol. 14, no.2, pages 257-276, May 1985
 *
 * 02Feb2004	Initial version
 */

#include <linux/mm.h>
#include <linux/prio_tree.h>
#include <linux/prefetch.h>



#define RADIX_INDEX(vma)  ((vma)->vm_pgoff)
#define VMA_SIZE(vma)	  (((vma)->vm_end - (vma)->vm_start) >> PAGE_SHIFT)
#define HEAP_INDEX(vma)   ((vma)->vm_pgoff + (VMA_SIZE(vma) - 1))


void vma_prio_tree_add(struct vm_area_struct *vma, struct vm_area_struct *old)
{
	
	BUG_ON(RADIX_INDEX(vma) != RADIX_INDEX(old));
	BUG_ON(HEAP_INDEX(vma) != HEAP_INDEX(old));

	vma->shared.vm_set.head = NULL;
	vma->shared.vm_set.parent = NULL;

	if (!old->shared.vm_set.parent)
		list_add(&vma->shared.vm_set.list,
				&old->shared.vm_set.list);
	else if (old->shared.vm_set.head)
		list_add_tail(&vma->shared.vm_set.list,
				&old->shared.vm_set.head->shared.vm_set.list);
	else {
		INIT_LIST_HEAD(&vma->shared.vm_set.list);
		vma->shared.vm_set.head = old;
		old->shared.vm_set.head = vma;
	}
}

void vma_prio_tree_insert(struct vm_area_struct *vma,
			  struct prio_tree_root *root)
{
	struct prio_tree_node *ptr;
	struct vm_area_struct *old;

	vma->shared.vm_set.head = NULL;

	ptr = raw_prio_tree_insert(root, &vma->shared.prio_tree_node);
	if (ptr != (struct prio_tree_node *) &vma->shared.prio_tree_node) {
		old = prio_tree_entry(ptr, struct vm_area_struct,
					shared.prio_tree_node);
		vma_prio_tree_add(vma, old);
	}
}

void vma_prio_tree_remove(struct vm_area_struct *vma,
			  struct prio_tree_root *root)
{
	struct vm_area_struct *node, *head, *new_head;

	if (!vma->shared.vm_set.head) {
		if (!vma->shared.vm_set.parent)
			list_del_init(&vma->shared.vm_set.list);
		else
			raw_prio_tree_remove(root, &vma->shared.prio_tree_node);
	} else {
		
		BUG_ON(vma->shared.vm_set.head->shared.vm_set.head != vma);
		if (vma->shared.vm_set.parent) {
			head = vma->shared.vm_set.head;
			if (!list_empty(&head->shared.vm_set.list)) {
				new_head = list_entry(
					head->shared.vm_set.list.next,
					struct vm_area_struct,
					shared.vm_set.list);
				list_del_init(&head->shared.vm_set.list);
			} else
				new_head = NULL;

			raw_prio_tree_replace(root, &vma->shared.prio_tree_node,
					&head->shared.prio_tree_node);
			head->shared.vm_set.head = new_head;
			if (new_head)
				new_head->shared.vm_set.head = head;

		} else {
			node = vma->shared.vm_set.head;
			if (!list_empty(&vma->shared.vm_set.list)) {
				new_head = list_entry(
					vma->shared.vm_set.list.next,
					struct vm_area_struct,
					shared.vm_set.list);
				list_del_init(&vma->shared.vm_set.list);
				node->shared.vm_set.head = new_head;
				new_head->shared.vm_set.head = node;
			} else
				node->shared.vm_set.head = NULL;
		}
	}
}

struct vm_area_struct *vma_prio_tree_next(struct vm_area_struct *vma,
					struct prio_tree_iter *iter)
{
	struct prio_tree_node *ptr;
	struct vm_area_struct *next;

	if (!vma) {
		ptr = prio_tree_next(iter);
		if (ptr) {
			next = prio_tree_entry(ptr, struct vm_area_struct,
						shared.prio_tree_node);
			prefetch(next->shared.vm_set.head);
			return next;
		} else
			return NULL;
	}

	if (vma->shared.vm_set.parent) {
		if (vma->shared.vm_set.head) {
			next = vma->shared.vm_set.head;
			prefetch(next->shared.vm_set.list.next);
			return next;
		}
	} else {
		next = list_entry(vma->shared.vm_set.list.next,
				struct vm_area_struct, shared.vm_set.list);
		if (!next->shared.vm_set.head) {
			prefetch(next->shared.vm_set.list.next);
			return next;
		}
	}

	ptr = prio_tree_next(iter);
	if (ptr) {
		next = prio_tree_entry(ptr, struct vm_area_struct,
					shared.prio_tree_node);
		prefetch(next->shared.vm_set.head);
		return next;
	} else
		return NULL;
}
