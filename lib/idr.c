/*
 * 2002-10-18  written by Jim Houston jim.houston@ccur.com
 *	Copyright (C) 2002 by Concurrent Computer Corporation
 *	Distributed under the GNU GPL license version 2.
 *
 * Modified by George Anzinger to reuse immediately and to use
 * find bit instructions.  Also removed _irq on spinlocks.
 *
 * Modified by Nadia Derbey to make it RCU safe.
 *
 * Small id to pointer translation service.
 *
 * It uses a radix tree like structure as a sparse array indexed
 * by the id to obtain the pointer.  The bitmap makes allocating
 * a new id quick.
 *
 * You call it to allocate an id (an int) an associate with that id a
 * pointer or what ever, we treat it as a (void *).  You can pass this
 * id to a user for him to pass back at a later time.  You then pass
 * that id to this code and it returns your pointer.

 * You can release ids at any time. When all ids are released, most of
 * the memory is returned (we keep IDR_FREE_MAX) in a local pool so we
 * don't need to go to the memory "store" during an id allocate, just
 * so you don't need to be too concerned about locking and conflicts
 * with the slab allocator.
 */

#ifndef TEST                        
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/export.h>
#endif
#include <linux/err.h>
#include <linux/string.h>
#include <linux/idr.h>
#include <linux/spinlock.h>

static struct kmem_cache *idr_layer_cache;
static DEFINE_SPINLOCK(simple_ida_lock);

static struct idr_layer *get_from_free_list(struct idr *idp)
{
	struct idr_layer *p;
	unsigned long flags;

	spin_lock_irqsave(&idp->lock, flags);
	if ((p = idp->id_free)) {
		idp->id_free = p->ary[0];
		idp->id_free_cnt--;
		p->ary[0] = NULL;
	}
	spin_unlock_irqrestore(&idp->lock, flags);
	return(p);
}

static void idr_layer_rcu_free(struct rcu_head *head)
{
	struct idr_layer *layer;

	layer = container_of(head, struct idr_layer, rcu_head);
	kmem_cache_free(idr_layer_cache, layer);
}

static inline void free_layer(struct idr_layer *p)
{
	call_rcu(&p->rcu_head, idr_layer_rcu_free);
}

static void __move_to_free_list(struct idr *idp, struct idr_layer *p)
{
	p->ary[0] = idp->id_free;
	idp->id_free = p;
	idp->id_free_cnt++;
}

static void move_to_free_list(struct idr *idp, struct idr_layer *p)
{
	unsigned long flags;

	spin_lock_irqsave(&idp->lock, flags);
	__move_to_free_list(idp, p);
	spin_unlock_irqrestore(&idp->lock, flags);
}

static void idr_mark_full(struct idr_layer **pa, int id)
{
	struct idr_layer *p = pa[0];
	int l = 0;

	__set_bit(id & IDR_MASK, &p->bitmap);
	while (p->bitmap == IDR_FULL) {
		if (!(p = pa[++l]))
			break;
		id = id >> IDR_BITS;
		__set_bit((id & IDR_MASK), &p->bitmap);
	}
}

int idr_pre_get(struct idr *idp, gfp_t gfp_mask)
{
	while (idp->id_free_cnt < IDR_FREE_MAX) {
		struct idr_layer *new;
		new = kmem_cache_zalloc(idr_layer_cache, gfp_mask);
		if (new == NULL)
			return (0);
		move_to_free_list(idp, new);
	}
	return 1;
}
EXPORT_SYMBOL(idr_pre_get);

static int sub_alloc(struct idr *idp, int *starting_id, struct idr_layer **pa)
{
	int n, m, sh;
	struct idr_layer *p, *new;
	int l, id, oid;
	unsigned long bm;

	id = *starting_id;
 restart:
	p = idp->top;
	l = idp->layers;
	pa[l--] = NULL;
	while (1) {
		n = (id >> (IDR_BITS*l)) & IDR_MASK;
		bm = ~p->bitmap;
		m = find_next_bit(&bm, IDR_SIZE, n);
		if (m == IDR_SIZE) {
			
			l++;
			oid = id;
			id = (id | ((1 << (IDR_BITS * l)) - 1)) + 1;

			
			if (id >= 1 << (idp->layers * IDR_BITS)) {
				*starting_id = id;
				return IDR_NEED_TO_GROW;
			}
			p = pa[l];
			BUG_ON(!p);

			sh = IDR_BITS * (l + 1);
			if (oid >> sh == id >> sh)
				continue;
			else
				goto restart;
		}
		if (m != n) {
			sh = IDR_BITS*l;
			id = ((id >> sh) ^ n ^ m) << sh;
		}
		if ((id >= MAX_ID_BIT) || (id < 0))
			return IDR_NOMORE_SPACE;
		if (l == 0)
			break;
		if (!p->ary[m]) {
			new = get_from_free_list(idp);
			if (!new)
				return -1;
			new->layer = l-1;
			rcu_assign_pointer(p->ary[m], new);
			p->count++;
		}
		pa[l--] = p;
		p = p->ary[m];
	}

	pa[l] = p;
	return id;
}

static int idr_get_empty_slot(struct idr *idp, int starting_id,
			      struct idr_layer **pa)
{
	struct idr_layer *p, *new;
	int layers, v, id;
	unsigned long flags;

	id = starting_id;
build_up:
	p = idp->top;
	layers = idp->layers;
	if (unlikely(!p)) {
		if (!(p = get_from_free_list(idp)))
			return -1;
		p->layer = 0;
		layers = 1;
	}
	while ((layers < (MAX_LEVEL - 1)) && (id >= (1 << (layers*IDR_BITS)))) {
		layers++;
		if (!p->count) {
			p->layer++;
			continue;
		}
		if (!(new = get_from_free_list(idp))) {
			spin_lock_irqsave(&idp->lock, flags);
			for (new = p; p && p != idp->top; new = p) {
				p = p->ary[0];
				new->ary[0] = NULL;
				new->bitmap = new->count = 0;
				__move_to_free_list(idp, new);
			}
			spin_unlock_irqrestore(&idp->lock, flags);
			return -1;
		}
		new->ary[0] = p;
		new->count = 1;
		new->layer = layers-1;
		if (p->bitmap == IDR_FULL)
			__set_bit(0, &new->bitmap);
		p = new;
	}
	rcu_assign_pointer(idp->top, p);
	idp->layers = layers;
	v = sub_alloc(idp, &id, pa);
	if (v == IDR_NEED_TO_GROW)
		goto build_up;
	return(v);
}

static int idr_get_new_above_int(struct idr *idp, void *ptr, int starting_id)
{
	struct idr_layer *pa[MAX_LEVEL];
	int id;

	id = idr_get_empty_slot(idp, starting_id, pa);
	if (id >= 0) {
		rcu_assign_pointer(pa[0]->ary[id & IDR_MASK],
				(struct idr_layer *)ptr);
		pa[0]->count++;
		idr_mark_full(pa, id);
	}

	return id;
}

int idr_get_new_above(struct idr *idp, void *ptr, int starting_id, int *id)
{
	int rv;

	rv = idr_get_new_above_int(idp, ptr, starting_id);
	if (rv < 0)
		return _idr_rc_to_errno(rv);
	*id = rv;
	return 0;
}
EXPORT_SYMBOL(idr_get_new_above);

int idr_get_new(struct idr *idp, void *ptr, int *id)
{
	int rv;

	rv = idr_get_new_above_int(idp, ptr, 0);
	if (rv < 0)
		return _idr_rc_to_errno(rv);
	*id = rv;
	return 0;
}
EXPORT_SYMBOL(idr_get_new);

static void idr_remove_warning(int id)
{
	printk(KERN_WARNING
		"idr_remove called for id=%d which is not allocated.\n", id);
	dump_stack();
}

static void sub_remove(struct idr *idp, int shift, int id)
{
	struct idr_layer *p = idp->top;
	struct idr_layer **pa[MAX_LEVEL];
	struct idr_layer ***paa = &pa[0];
	struct idr_layer *to_free;
	int n;

	*paa = NULL;
	*++paa = &idp->top;

	while ((shift > 0) && p) {
		n = (id >> shift) & IDR_MASK;
		__clear_bit(n, &p->bitmap);
		*++paa = &p->ary[n];
		p = p->ary[n];
		shift -= IDR_BITS;
	}
	n = id & IDR_MASK;
	if (likely(p != NULL && test_bit(n, &p->bitmap))){
		__clear_bit(n, &p->bitmap);
		rcu_assign_pointer(p->ary[n], NULL);
		to_free = NULL;
		while(*paa && ! --((**paa)->count)){
			if (to_free)
				free_layer(to_free);
			to_free = **paa;
			**paa-- = NULL;
		}
		if (!*paa)
			idp->layers = 0;
		if (to_free)
			free_layer(to_free);
	} else
		idr_remove_warning(id);
}

void idr_remove(struct idr *idp, int id)
{
	struct idr_layer *p;
	struct idr_layer *to_free;

	
	id &= MAX_ID_MASK;

	sub_remove(idp, (idp->layers - 1) * IDR_BITS, id);
	if (idp->top && idp->top->count == 1 && (idp->layers > 1) &&
	    idp->top->ary[0]) {
		to_free = idp->top;
		p = idp->top->ary[0];
		rcu_assign_pointer(idp->top, p);
		--idp->layers;
		to_free->bitmap = to_free->count = 0;
		free_layer(to_free);
	}
	while (idp->id_free_cnt >= IDR_FREE_MAX) {
		p = get_from_free_list(idp);
		kmem_cache_free(idr_layer_cache, p);
	}
	return;
}
EXPORT_SYMBOL(idr_remove);

void idr_remove_all(struct idr *idp)
{
	int n, id, max;
	int bt_mask;
	struct idr_layer *p;
	struct idr_layer *pa[MAX_LEVEL];
	struct idr_layer **paa = &pa[0];

	n = idp->layers * IDR_BITS;
	p = idp->top;
	rcu_assign_pointer(idp->top, NULL);
	max = 1 << n;

	id = 0;
	while (id < max) {
		while (n > IDR_BITS && p) {
			n -= IDR_BITS;
			*paa++ = p;
			p = p->ary[(id >> n) & IDR_MASK];
		}

		bt_mask = id;
		id += 1 << n;
		
		while (n < fls(id ^ bt_mask)) {
			if (p)
				free_layer(p);
			n += IDR_BITS;
			p = *--paa;
		}
	}
	idp->layers = 0;
}
EXPORT_SYMBOL(idr_remove_all);

void idr_destroy(struct idr *idp)
{
	while (idp->id_free_cnt) {
		struct idr_layer *p = get_from_free_list(idp);
		kmem_cache_free(idr_layer_cache, p);
	}
}
EXPORT_SYMBOL(idr_destroy);

void *idr_find(struct idr *idp, int id)
{
	int n;
	struct idr_layer *p;

	p = rcu_dereference_raw(idp->top);
	if (!p)
		return NULL;
	n = (p->layer+1) * IDR_BITS;

	
	id &= MAX_ID_MASK;

	if (id >= (1 << n))
		return NULL;
	BUG_ON(n == 0);

	while (n > 0 && p) {
		n -= IDR_BITS;
		BUG_ON(n != p->layer*IDR_BITS);
		p = rcu_dereference_raw(p->ary[(id >> n) & IDR_MASK]);
	}
	return((void *)p);
}
EXPORT_SYMBOL(idr_find);

int idr_for_each(struct idr *idp,
		 int (*fn)(int id, void *p, void *data), void *data)
{
	int n, id, max, error = 0;
	struct idr_layer *p;
	struct idr_layer *pa[MAX_LEVEL];
	struct idr_layer **paa = &pa[0];

	n = idp->layers * IDR_BITS;
	p = rcu_dereference_raw(idp->top);
	max = 1 << n;

	id = 0;
	while (id < max) {
		while (n > 0 && p) {
			n -= IDR_BITS;
			*paa++ = p;
			p = rcu_dereference_raw(p->ary[(id >> n) & IDR_MASK]);
		}

		if (p) {
			error = fn(id, (void *)p, data);
			if (error)
				break;
		}

		id += 1 << n;
		while (n < fls(id)) {
			n += IDR_BITS;
			p = *--paa;
		}
	}

	return error;
}
EXPORT_SYMBOL(idr_for_each);

void *idr_get_next(struct idr *idp, int *nextidp)
{
	struct idr_layer *p, *pa[MAX_LEVEL];
	struct idr_layer **paa = &pa[0];
	int id = *nextidp;
	int n, max;

	
	p = rcu_dereference_raw(idp->top);
	if (!p)
		return NULL;
	n = (p->layer + 1) * IDR_BITS;
	max = 1 << n;

	while (id < max) {
		while (n > 0 && p) {
			n -= IDR_BITS;
			*paa++ = p;
			p = rcu_dereference_raw(p->ary[(id >> n) & IDR_MASK]);
		}

		if (p) {
			*nextidp = id;
			return p;
		}

		id = round_up(id + 1, 1 << n);
		while (n < fls(id)) {
			n += IDR_BITS;
			p = *--paa;
		}
	}
	return NULL;
}
EXPORT_SYMBOL(idr_get_next);


void *idr_replace(struct idr *idp, void *ptr, int id)
{
	int n;
	struct idr_layer *p, *old_p;

	p = idp->top;
	if (!p)
		return ERR_PTR(-EINVAL);

	n = (p->layer+1) * IDR_BITS;

	id &= MAX_ID_MASK;

	if (id >= (1 << n))
		return ERR_PTR(-EINVAL);

	n -= IDR_BITS;
	while ((n > 0) && p) {
		p = p->ary[(id >> n) & IDR_MASK];
		n -= IDR_BITS;
	}

	n = id & IDR_MASK;
	if (unlikely(p == NULL || !test_bit(n, &p->bitmap)))
		return ERR_PTR(-ENOENT);

	old_p = p->ary[n];
	rcu_assign_pointer(p->ary[n], ptr);

	return old_p;
}
EXPORT_SYMBOL(idr_replace);

void __init idr_init_cache(void)
{
	idr_layer_cache = kmem_cache_create("idr_layer_cache",
				sizeof(struct idr_layer), 0, SLAB_PANIC, NULL);
}

void idr_init(struct idr *idp)
{
	memset(idp, 0, sizeof(struct idr));
	spin_lock_init(&idp->lock);
}
EXPORT_SYMBOL(idr_init);


/**
 * DOC: IDA description
 * IDA - IDR based ID allocator
 *
 * This is id allocator without id -> pointer translation.  Memory
 * usage is much lower than full blown idr because each id only
 * occupies a bit.  ida uses a custom leaf node which contains
 * IDA_BITMAP_BITS slots.
 *
 * 2007-04-25  written by Tejun Heo <htejun@gmail.com>
 */

static void free_bitmap(struct ida *ida, struct ida_bitmap *bitmap)
{
	unsigned long flags;

	if (!ida->free_bitmap) {
		spin_lock_irqsave(&ida->idr.lock, flags);
		if (!ida->free_bitmap) {
			ida->free_bitmap = bitmap;
			bitmap = NULL;
		}
		spin_unlock_irqrestore(&ida->idr.lock, flags);
	}

	kfree(bitmap);
}

int ida_pre_get(struct ida *ida, gfp_t gfp_mask)
{
	
	if (!idr_pre_get(&ida->idr, gfp_mask))
		return 0;

	
	if (!ida->free_bitmap) {
		struct ida_bitmap *bitmap;

		bitmap = kmalloc(sizeof(struct ida_bitmap), gfp_mask);
		if (!bitmap)
			return 0;

		free_bitmap(ida, bitmap);
	}

	return 1;
}
EXPORT_SYMBOL(ida_pre_get);

int ida_get_new_above(struct ida *ida, int starting_id, int *p_id)
{
	struct idr_layer *pa[MAX_LEVEL];
	struct ida_bitmap *bitmap;
	unsigned long flags;
	int idr_id = starting_id / IDA_BITMAP_BITS;
	int offset = starting_id % IDA_BITMAP_BITS;
	int t, id;

 restart:
	
	t = idr_get_empty_slot(&ida->idr, idr_id, pa);
	if (t < 0)
		return _idr_rc_to_errno(t);

	if (t * IDA_BITMAP_BITS >= MAX_ID_BIT)
		return -ENOSPC;

	if (t != idr_id)
		offset = 0;
	idr_id = t;

	
	bitmap = (void *)pa[0]->ary[idr_id & IDR_MASK];
	if (!bitmap) {
		spin_lock_irqsave(&ida->idr.lock, flags);
		bitmap = ida->free_bitmap;
		ida->free_bitmap = NULL;
		spin_unlock_irqrestore(&ida->idr.lock, flags);

		if (!bitmap)
			return -EAGAIN;

		memset(bitmap, 0, sizeof(struct ida_bitmap));
		rcu_assign_pointer(pa[0]->ary[idr_id & IDR_MASK],
				(void *)bitmap);
		pa[0]->count++;
	}

	
	t = find_next_zero_bit(bitmap->bitmap, IDA_BITMAP_BITS, offset);
	if (t == IDA_BITMAP_BITS) {
		
		idr_id++;
		offset = 0;
		goto restart;
	}

	id = idr_id * IDA_BITMAP_BITS + t;
	if (id >= MAX_ID_BIT)
		return -ENOSPC;

	__set_bit(t, bitmap->bitmap);
	if (++bitmap->nr_busy == IDA_BITMAP_BITS)
		idr_mark_full(pa, idr_id);

	*p_id = id;

	if (ida->idr.id_free_cnt || ida->free_bitmap) {
		struct idr_layer *p = get_from_free_list(&ida->idr);
		if (p)
			kmem_cache_free(idr_layer_cache, p);
	}

	return 0;
}
EXPORT_SYMBOL(ida_get_new_above);

int ida_get_new(struct ida *ida, int *p_id)
{
	return ida_get_new_above(ida, 0, p_id);
}
EXPORT_SYMBOL(ida_get_new);

void ida_remove(struct ida *ida, int id)
{
	struct idr_layer *p = ida->idr.top;
	int shift = (ida->idr.layers - 1) * IDR_BITS;
	int idr_id = id / IDA_BITMAP_BITS;
	int offset = id % IDA_BITMAP_BITS;
	int n;
	struct ida_bitmap *bitmap;

	
	while ((shift > 0) && p) {
		n = (idr_id >> shift) & IDR_MASK;
		__clear_bit(n, &p->bitmap);
		p = p->ary[n];
		shift -= IDR_BITS;
	}

	if (p == NULL)
		goto err;

	n = idr_id & IDR_MASK;
	__clear_bit(n, &p->bitmap);

	bitmap = (void *)p->ary[n];
	if (!test_bit(offset, bitmap->bitmap))
		goto err;

	
	__clear_bit(offset, bitmap->bitmap);
	if (--bitmap->nr_busy == 0) {
		__set_bit(n, &p->bitmap);	
		idr_remove(&ida->idr, idr_id);
		free_bitmap(ida, bitmap);
	}

	return;

 err:
	printk(KERN_WARNING
	       "ida_remove called for id=%d which is not allocated.\n", id);
}
EXPORT_SYMBOL(ida_remove);

void ida_destroy(struct ida *ida)
{
	idr_destroy(&ida->idr);
	kfree(ida->free_bitmap);
}
EXPORT_SYMBOL(ida_destroy);

int ida_simple_get(struct ida *ida, unsigned int start, unsigned int end,
		   gfp_t gfp_mask)
{
	int ret, id;
	unsigned int max;
	unsigned long flags;

	BUG_ON((int)start < 0);
	BUG_ON((int)end < 0);

	if (end == 0)
		max = 0x80000000;
	else {
		BUG_ON(end < start);
		max = end - 1;
	}

again:
	if (!ida_pre_get(ida, gfp_mask))
		return -ENOMEM;

	spin_lock_irqsave(&simple_ida_lock, flags);
	ret = ida_get_new_above(ida, start, &id);
	if (!ret) {
		if (id > max) {
			ida_remove(ida, id);
			ret = -ENOSPC;
		} else {
			ret = id;
		}
	}
	spin_unlock_irqrestore(&simple_ida_lock, flags);

	if (unlikely(ret == -EAGAIN))
		goto again;

	return ret;
}
EXPORT_SYMBOL(ida_simple_get);

void ida_simple_remove(struct ida *ida, unsigned int id)
{
	unsigned long flags;

	BUG_ON((int)id < 0);
	spin_lock_irqsave(&simple_ida_lock, flags);
	ida_remove(ida, id);
	spin_unlock_irqrestore(&simple_ida_lock, flags);
}
EXPORT_SYMBOL(ida_simple_remove);

void ida_init(struct ida *ida)
{
	memset(ida, 0, sizeof(struct ida));
	idr_init(&ida->idr);

}
EXPORT_SYMBOL(ida_init);
