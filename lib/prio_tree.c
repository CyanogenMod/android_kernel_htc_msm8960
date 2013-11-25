/*
 * lib/prio_tree.c - priority search tree
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

#include <linux/init.h>
#include <linux/mm.h>
#include <linux/prio_tree.h>



#define RADIX_INDEX(vma)  ((vma)->vm_pgoff)
#define VMA_SIZE(vma)	  (((vma)->vm_end - (vma)->vm_start) >> PAGE_SHIFT)
#define HEAP_INDEX(vma)	  ((vma)->vm_pgoff + (VMA_SIZE(vma) - 1))


static void get_index(const struct prio_tree_root *root,
    const struct prio_tree_node *node,
    unsigned long *radix, unsigned long *heap)
{
	if (root->raw) {
		struct vm_area_struct *vma = prio_tree_entry(
		    node, struct vm_area_struct, shared.prio_tree_node);

		*radix = RADIX_INDEX(vma);
		*heap = HEAP_INDEX(vma);
	}
	else {
		*radix = node->start;
		*heap = node->last;
	}
}

static unsigned long index_bits_to_maxindex[BITS_PER_LONG];

void __init prio_tree_init(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(index_bits_to_maxindex) - 1; i++)
		index_bits_to_maxindex[i] = (1UL << (i + 1)) - 1;
	index_bits_to_maxindex[ARRAY_SIZE(index_bits_to_maxindex) - 1] = ~0UL;
}

static inline unsigned long prio_tree_maxindex(unsigned int bits)
{
	return index_bits_to_maxindex[bits - 1];
}

static void prio_set_parent(struct prio_tree_node *parent,
			    struct prio_tree_node *child, bool left)
{
	if (left)
		parent->left = child;
	else
		parent->right = child;

	child->parent = parent;
}

static struct prio_tree_node *prio_tree_expand(struct prio_tree_root *root,
		struct prio_tree_node *node, unsigned long max_heap_index)
{
	struct prio_tree_node *prev;

	if (max_heap_index > prio_tree_maxindex(root->index_bits))
		root->index_bits++;

	prev = node;
	INIT_PRIO_TREE_NODE(node);

	while (max_heap_index > prio_tree_maxindex(root->index_bits)) {
		struct prio_tree_node *tmp = root->prio_tree_node;

		root->index_bits++;

		if (prio_tree_empty(root))
			continue;

		prio_tree_remove(root, root->prio_tree_node);
		INIT_PRIO_TREE_NODE(tmp);

		prio_set_parent(prev, tmp, true);
		prev = tmp;
	}

	if (!prio_tree_empty(root))
		prio_set_parent(prev, root->prio_tree_node, true);

	root->prio_tree_node = node;
	return node;
}

struct prio_tree_node *prio_tree_replace(struct prio_tree_root *root,
		struct prio_tree_node *old, struct prio_tree_node *node)
{
	INIT_PRIO_TREE_NODE(node);

	if (prio_tree_root(old)) {
		BUG_ON(root->prio_tree_node != old);
		root->prio_tree_node = node;
	} else
		prio_set_parent(old->parent, node, old->parent->left == old);

	if (!prio_tree_left_empty(old))
		prio_set_parent(node, old->left, true);

	if (!prio_tree_right_empty(old))
		prio_set_parent(node, old->right, false);

	return old;
}

struct prio_tree_node *prio_tree_insert(struct prio_tree_root *root,
		struct prio_tree_node *node)
{
	struct prio_tree_node *cur, *res = node;
	unsigned long radix_index, heap_index;
	unsigned long r_index, h_index, index, mask;
	int size_flag = 0;

	get_index(root, node, &radix_index, &heap_index);

	if (prio_tree_empty(root) ||
			heap_index > prio_tree_maxindex(root->index_bits))
		return prio_tree_expand(root, node, heap_index);

	cur = root->prio_tree_node;
	mask = 1UL << (root->index_bits - 1);

	while (mask) {
		get_index(root, cur, &r_index, &h_index);

		if (r_index == radix_index && h_index == heap_index)
			return cur;

                if (h_index < heap_index ||
		    (h_index == heap_index && r_index > radix_index)) {
			struct prio_tree_node *tmp = node;
			node = prio_tree_replace(root, cur, node);
			cur = tmp;
			
			index = r_index;
			r_index = radix_index;
			radix_index = index;
			index = h_index;
			h_index = heap_index;
			heap_index = index;
		}

		if (size_flag)
			index = heap_index - radix_index;
		else
			index = radix_index;

		if (index & mask) {
			if (prio_tree_right_empty(cur)) {
				INIT_PRIO_TREE_NODE(node);
				prio_set_parent(cur, node, false);
				return res;
			} else
				cur = cur->right;
		} else {
			if (prio_tree_left_empty(cur)) {
				INIT_PRIO_TREE_NODE(node);
				prio_set_parent(cur, node, true);
				return res;
			} else
				cur = cur->left;
		}

		mask >>= 1;

		if (!mask) {
			mask = 1UL << (BITS_PER_LONG - 1);
			size_flag = 1;
		}
	}
	
	BUG();
	return NULL;
}

void prio_tree_remove(struct prio_tree_root *root, struct prio_tree_node *node)
{
	struct prio_tree_node *cur;
	unsigned long r_index, h_index_right, h_index_left;

	cur = node;

	while (!prio_tree_left_empty(cur) || !prio_tree_right_empty(cur)) {
		if (!prio_tree_left_empty(cur))
			get_index(root, cur->left, &r_index, &h_index_left);
		else {
			cur = cur->right;
			continue;
		}

		if (!prio_tree_right_empty(cur))
			get_index(root, cur->right, &r_index, &h_index_right);
		else {
			cur = cur->left;
			continue;
		}

		
		if (h_index_left >= h_index_right)
			cur = cur->left;
		else
			cur = cur->right;
	}

	if (prio_tree_root(cur)) {
		BUG_ON(root->prio_tree_node != cur);
		__INIT_PRIO_TREE_ROOT(root, root->raw);
		return;
	}

	if (cur->parent->right == cur)
		cur->parent->right = cur->parent;
	else
		cur->parent->left = cur->parent;

	while (cur != node)
		cur = prio_tree_replace(root, cur->parent, cur);
}

static void iter_walk_down(struct prio_tree_iter *iter)
{
	iter->mask >>= 1;
	if (iter->mask) {
		if (iter->size_level)
			iter->size_level++;
		return;
	}

	if (iter->size_level) {
		BUG_ON(!prio_tree_left_empty(iter->cur));
		BUG_ON(!prio_tree_right_empty(iter->cur));
		iter->size_level++;
		iter->mask = ULONG_MAX;
	} else {
		iter->size_level = 1;
		iter->mask = 1UL << (BITS_PER_LONG - 1);
	}
}

static void iter_walk_up(struct prio_tree_iter *iter)
{
	if (iter->mask == ULONG_MAX)
		iter->mask = 1UL;
	else if (iter->size_level == 1)
		iter->mask = 1UL;
	else
		iter->mask <<= 1;
	if (iter->size_level)
		iter->size_level--;
	if (!iter->size_level && (iter->value & iter->mask))
		iter->value ^= iter->mask;
}


static struct prio_tree_node *prio_tree_left(struct prio_tree_iter *iter,
		unsigned long *r_index, unsigned long *h_index)
{
	if (prio_tree_left_empty(iter->cur))
		return NULL;

	get_index(iter->root, iter->cur->left, r_index, h_index);

	if (iter->r_index <= *h_index) {
		iter->cur = iter->cur->left;
		iter_walk_down(iter);
		return iter->cur;
	}

	return NULL;
}

static struct prio_tree_node *prio_tree_right(struct prio_tree_iter *iter,
		unsigned long *r_index, unsigned long *h_index)
{
	unsigned long value;

	if (prio_tree_right_empty(iter->cur))
		return NULL;

	if (iter->size_level)
		value = iter->value;
	else
		value = iter->value | iter->mask;

	if (iter->h_index < value)
		return NULL;

	get_index(iter->root, iter->cur->right, r_index, h_index);

	if (iter->r_index <= *h_index) {
		iter->cur = iter->cur->right;
		iter_walk_down(iter);
		return iter->cur;
	}

	return NULL;
}

static struct prio_tree_node *prio_tree_parent(struct prio_tree_iter *iter)
{
	iter->cur = iter->cur->parent;
	iter_walk_up(iter);
	return iter->cur;
}

static inline int overlap(struct prio_tree_iter *iter,
		unsigned long r_index, unsigned long h_index)
{
	return iter->h_index >= r_index && iter->r_index <= h_index;
}

static struct prio_tree_node *prio_tree_first(struct prio_tree_iter *iter)
{
	struct prio_tree_root *root;
	unsigned long r_index, h_index;

	INIT_PRIO_TREE_ITER(iter);

	root = iter->root;
	if (prio_tree_empty(root))
		return NULL;

	get_index(root, root->prio_tree_node, &r_index, &h_index);

	if (iter->r_index > h_index)
		return NULL;

	iter->mask = 1UL << (root->index_bits - 1);
	iter->cur = root->prio_tree_node;

	while (1) {
		if (overlap(iter, r_index, h_index))
			return iter->cur;

		if (prio_tree_left(iter, &r_index, &h_index))
			continue;

		if (prio_tree_right(iter, &r_index, &h_index))
			continue;

		break;
	}
	return NULL;
}

struct prio_tree_node *prio_tree_next(struct prio_tree_iter *iter)
{
	unsigned long r_index, h_index;

	if (iter->cur == NULL)
		return prio_tree_first(iter);

repeat:
	while (prio_tree_left(iter, &r_index, &h_index))
		if (overlap(iter, r_index, h_index))
			return iter->cur;

	while (!prio_tree_right(iter, &r_index, &h_index)) {
	    	while (!prio_tree_root(iter->cur) &&
				iter->cur->parent->right == iter->cur)
			prio_tree_parent(iter);

		if (prio_tree_root(iter->cur))
			return NULL;

		prio_tree_parent(iter);
	}

	if (overlap(iter, r_index, h_index))
		return iter->cur;

	goto repeat;
}
