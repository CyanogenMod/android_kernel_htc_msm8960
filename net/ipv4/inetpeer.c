/*
 *		INETPEER - A storage for permanent information about peers
 *
 *  This source is covered by the GNU GPL, the same as all kernel sources.
 *
 *  Authors:	Andrey V. Savochkin <saw@msu.ru>
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/random.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/net.h>
#include <linux/workqueue.h>
#include <net/ip.h>
#include <net/inetpeer.h>
#include <net/secure_seq.h>


static struct kmem_cache *peer_cachep __read_mostly;

static LIST_HEAD(gc_list);
static const int gc_delay = 60 * HZ;
static struct delayed_work gc_work;
static DEFINE_SPINLOCK(gc_lock);

#define node_height(x) x->avl_height

#define peer_avl_empty ((struct inet_peer *)&peer_fake_node)
#define peer_avl_empty_rcu ((struct inet_peer __rcu __force *)&peer_fake_node)
static const struct inet_peer peer_fake_node = {
	.avl_left	= peer_avl_empty_rcu,
	.avl_right	= peer_avl_empty_rcu,
	.avl_height	= 0
};

struct inet_peer_base {
	struct inet_peer __rcu *root;
	seqlock_t	lock;
	int		total;
};

static struct inet_peer_base v4_peers = {
	.root		= peer_avl_empty_rcu,
	.lock		= __SEQLOCK_UNLOCKED(v4_peers.lock),
	.total		= 0,
};

static struct inet_peer_base v6_peers = {
	.root		= peer_avl_empty_rcu,
	.lock		= __SEQLOCK_UNLOCKED(v6_peers.lock),
	.total		= 0,
};

#define PEER_MAXDEPTH 40 

int inet_peer_threshold __read_mostly = 65536 + 128;	
int inet_peer_minttl __read_mostly = 120 * HZ;	
int inet_peer_maxttl __read_mostly = 10 * 60 * HZ;	

static void inetpeer_gc_worker(struct work_struct *work)
{
	struct inet_peer *p, *n;
	LIST_HEAD(list);

	spin_lock_bh(&gc_lock);
	list_replace_init(&gc_list, &list);
	spin_unlock_bh(&gc_lock);

	if (list_empty(&list))
		return;

	list_for_each_entry_safe(p, n, &list, gc_list) {

		if(need_resched())
			cond_resched();

		if (p->avl_left != peer_avl_empty) {
			list_add_tail(&p->avl_left->gc_list, &list);
			p->avl_left = peer_avl_empty;
		}

		if (p->avl_right != peer_avl_empty) {
			list_add_tail(&p->avl_right->gc_list, &list);
			p->avl_right = peer_avl_empty;
		}

		n = list_entry(p->gc_list.next, struct inet_peer, gc_list);

		if (!atomic_read(&p->refcnt)) {
			list_del(&p->gc_list);
			kmem_cache_free(peer_cachep, p);
		}
	}

	if (list_empty(&list))
		return;

	spin_lock_bh(&gc_lock);
	list_splice(&list, &gc_list);
	spin_unlock_bh(&gc_lock);

	schedule_delayed_work(&gc_work, gc_delay);
}

void __init inet_initpeers(void)
{
	struct sysinfo si;

	
	si_meminfo(&si);
	if (si.totalram <= (32768*1024)/PAGE_SIZE)
		inet_peer_threshold >>= 1; 
	if (si.totalram <= (16384*1024)/PAGE_SIZE)
		inet_peer_threshold >>= 1; 
	if (si.totalram <= (8192*1024)/PAGE_SIZE)
		inet_peer_threshold >>= 2; 

	peer_cachep = kmem_cache_create("inet_peer_cache",
			sizeof(struct inet_peer),
			0, SLAB_HWCACHE_ALIGN | SLAB_PANIC,
			NULL);

	INIT_DELAYED_WORK_DEFERRABLE(&gc_work, inetpeer_gc_worker);
}

static int addr_compare(const struct inetpeer_addr *a,
			const struct inetpeer_addr *b)
{
	int i, n = (a->family == AF_INET ? 1 : 4);

	for (i = 0; i < n; i++) {
		if (a->addr.a6[i] == b->addr.a6[i])
			continue;
		if ((__force u32)a->addr.a6[i] < (__force u32)b->addr.a6[i])
			return -1;
		return 1;
	}

	return 0;
}

#define rcu_deref_locked(X, BASE)				\
	rcu_dereference_protected(X, lockdep_is_held(&(BASE)->lock.lock))

#define lookup(_daddr, _stack, _base)				\
({								\
	struct inet_peer *u;					\
	struct inet_peer __rcu **v;				\
								\
	stackptr = _stack;					\
	*stackptr++ = &_base->root;				\
	for (u = rcu_deref_locked(_base->root, _base);		\
	     u != peer_avl_empty; ) {				\
		int cmp = addr_compare(_daddr, &u->daddr);	\
		if (cmp == 0)					\
			break;					\
		if (cmp == -1)					\
			v = &u->avl_left;			\
		else						\
			v = &u->avl_right;			\
		*stackptr++ = v;				\
		u = rcu_deref_locked(*v, _base);		\
	}							\
	u;							\
})

static struct inet_peer *lookup_rcu(const struct inetpeer_addr *daddr,
				    struct inet_peer_base *base)
{
	struct inet_peer *u = rcu_dereference(base->root);
	int count = 0;

	while (u != peer_avl_empty) {
		int cmp = addr_compare(daddr, &u->daddr);
		if (cmp == 0) {
			if (!atomic_add_unless(&u->refcnt, 1, -1))
				u = NULL;
			return u;
		}
		if (cmp == -1)
			u = rcu_dereference(u->avl_left);
		else
			u = rcu_dereference(u->avl_right);
		if (unlikely(++count == PEER_MAXDEPTH))
			break;
	}
	return NULL;
}

#define lookup_rightempty(start, base)				\
({								\
	struct inet_peer *u;					\
	struct inet_peer __rcu **v;				\
	*stackptr++ = &start->avl_left;				\
	v = &start->avl_left;					\
	for (u = rcu_deref_locked(*v, base);			\
	     u->avl_right != peer_avl_empty_rcu; ) {		\
		v = &u->avl_right;				\
		*stackptr++ = v;				\
		u = rcu_deref_locked(*v, base);			\
	}							\
	u;							\
})

static void peer_avl_rebalance(struct inet_peer __rcu **stack[],
			       struct inet_peer __rcu ***stackend,
			       struct inet_peer_base *base)
{
	struct inet_peer __rcu **nodep;
	struct inet_peer *node, *l, *r;
	int lh, rh;

	while (stackend > stack) {
		nodep = *--stackend;
		node = rcu_deref_locked(*nodep, base);
		l = rcu_deref_locked(node->avl_left, base);
		r = rcu_deref_locked(node->avl_right, base);
		lh = node_height(l);
		rh = node_height(r);
		if (lh > rh + 1) { 
			struct inet_peer *ll, *lr, *lrl, *lrr;
			int lrh;
			ll = rcu_deref_locked(l->avl_left, base);
			lr = rcu_deref_locked(l->avl_right, base);
			lrh = node_height(lr);
			if (lrh <= node_height(ll)) {	
				RCU_INIT_POINTER(node->avl_left, lr);	
				RCU_INIT_POINTER(node->avl_right, r);	
				node->avl_height = lrh + 1; 
				RCU_INIT_POINTER(l->avl_left, ll);       
				RCU_INIT_POINTER(l->avl_right, node);	
				l->avl_height = node->avl_height + 1;
				RCU_INIT_POINTER(*nodep, l);
			} else { 
				lrl = rcu_deref_locked(lr->avl_left, base);
				lrr = rcu_deref_locked(lr->avl_right, base);
				RCU_INIT_POINTER(node->avl_left, lrr);	
				RCU_INIT_POINTER(node->avl_right, r);	
				node->avl_height = rh + 1; 
				RCU_INIT_POINTER(l->avl_left, ll);	
				RCU_INIT_POINTER(l->avl_right, lrl);	
				l->avl_height = rh + 1;	
				RCU_INIT_POINTER(lr->avl_left, l);	
				RCU_INIT_POINTER(lr->avl_right, node);	
				lr->avl_height = rh + 2;
				RCU_INIT_POINTER(*nodep, lr);
			}
		} else if (rh > lh + 1) { 
			struct inet_peer *rr, *rl, *rlr, *rll;
			int rlh;
			rr = rcu_deref_locked(r->avl_right, base);
			rl = rcu_deref_locked(r->avl_left, base);
			rlh = node_height(rl);
			if (rlh <= node_height(rr)) {	
				RCU_INIT_POINTER(node->avl_right, rl);	
				RCU_INIT_POINTER(node->avl_left, l);	
				node->avl_height = rlh + 1; 
				RCU_INIT_POINTER(r->avl_right, rr);	
				RCU_INIT_POINTER(r->avl_left, node);	
				r->avl_height = node->avl_height + 1;
				RCU_INIT_POINTER(*nodep, r);
			} else { 
				rlr = rcu_deref_locked(rl->avl_right, base);
				rll = rcu_deref_locked(rl->avl_left, base);
				RCU_INIT_POINTER(node->avl_right, rll);	
				RCU_INIT_POINTER(node->avl_left, l);	
				node->avl_height = lh + 1; 
				RCU_INIT_POINTER(r->avl_right, rr);	
				RCU_INIT_POINTER(r->avl_left, rlr);	
				r->avl_height = lh + 1;	
				RCU_INIT_POINTER(rl->avl_right, r);	
				RCU_INIT_POINTER(rl->avl_left, node);	
				rl->avl_height = lh + 2;
				RCU_INIT_POINTER(*nodep, rl);
			}
		} else {
			node->avl_height = (lh > rh ? lh : rh) + 1;
		}
	}
}

#define link_to_pool(n, base)					\
do {								\
	n->avl_height = 1;					\
	n->avl_left = peer_avl_empty_rcu;			\
	n->avl_right = peer_avl_empty_rcu;			\
				\
	rcu_assign_pointer(**--stackptr, n);			\
	peer_avl_rebalance(stack, stackptr, base);		\
} while (0)

static void inetpeer_free_rcu(struct rcu_head *head)
{
	kmem_cache_free(peer_cachep, container_of(head, struct inet_peer, rcu));
}

static void unlink_from_pool(struct inet_peer *p, struct inet_peer_base *base,
			     struct inet_peer __rcu **stack[PEER_MAXDEPTH])
{
	struct inet_peer __rcu ***stackptr, ***delp;

	if (lookup(&p->daddr, stack, base) != p)
		BUG();
	delp = stackptr - 1; 
	if (p->avl_left == peer_avl_empty_rcu) {
		*delp[0] = p->avl_right;
		--stackptr;
	} else {
		
		struct inet_peer *t;
		t = lookup_rightempty(p, base);
		BUG_ON(rcu_deref_locked(*stackptr[-1], base) != t);
		**--stackptr = t->avl_left;
		RCU_INIT_POINTER(*delp[0], t);
		t->avl_left = p->avl_left;
		t->avl_right = p->avl_right;
		t->avl_height = p->avl_height;
		BUG_ON(delp[1] != &p->avl_left);
		delp[1] = &t->avl_left; 
	}
	peer_avl_rebalance(stack, stackptr, base);
	base->total--;
	call_rcu(&p->rcu, inetpeer_free_rcu);
}

static struct inet_peer_base *family_to_base(int family)
{
	return family == AF_INET ? &v4_peers : &v6_peers;
}

static int inet_peer_gc(struct inet_peer_base *base,
			struct inet_peer __rcu **stack[PEER_MAXDEPTH],
			struct inet_peer __rcu ***stackptr)
{
	struct inet_peer *p, *gchead = NULL;
	__u32 delta, ttl;
	int cnt = 0;

	if (base->total >= inet_peer_threshold)
		ttl = 0; 
	else
		ttl = inet_peer_maxttl
				- (inet_peer_maxttl - inet_peer_minttl) / HZ *
					base->total / inet_peer_threshold * HZ;
	stackptr--; 
	while (stackptr > stack) {
		stackptr--;
		p = rcu_deref_locked(**stackptr, base);
		if (atomic_read(&p->refcnt) == 0) {
			smp_rmb();
			delta = (__u32)jiffies - p->dtime;
			if (delta >= ttl &&
			    atomic_cmpxchg(&p->refcnt, 0, -1) == 0) {
				p->gc_next = gchead;
				gchead = p;
			}
		}
	}
	while ((p = gchead) != NULL) {
		gchead = p->gc_next;
		cnt++;
		unlink_from_pool(p, base, stack);
	}
	return cnt;
}

struct inet_peer *inet_getpeer(const struct inetpeer_addr *daddr, int create)
{
	struct inet_peer __rcu **stack[PEER_MAXDEPTH], ***stackptr;
	struct inet_peer_base *base = family_to_base(daddr->family);
	struct inet_peer *p;
	unsigned int sequence;
	int invalidated, gccnt = 0;

	rcu_read_lock();
	sequence = read_seqbegin(&base->lock);
	p = lookup_rcu(daddr, base);
	invalidated = read_seqretry(&base->lock, sequence);
	rcu_read_unlock();

	if (p)
		return p;

	
	if (!create && !invalidated)
		return NULL;

	write_seqlock_bh(&base->lock);
relookup:
	p = lookup(daddr, stack, base);
	if (p != peer_avl_empty) {
		atomic_inc(&p->refcnt);
		write_sequnlock_bh(&base->lock);
		return p;
	}
	if (!gccnt) {
		gccnt = inet_peer_gc(base, stack, stackptr);
		if (gccnt && create)
			goto relookup;
	}
	p = create ? kmem_cache_alloc(peer_cachep, GFP_ATOMIC) : NULL;
	if (p) {
		p->daddr = *daddr;
		atomic_set(&p->refcnt, 1);
		atomic_set(&p->rid, 0);
		atomic_set(&p->ip_id_count,
				(daddr->family == AF_INET) ?
					secure_ip_id(daddr->addr.a4) :
					secure_ipv6_id(daddr->addr.a6));
		p->tcp_ts_stamp = 0;
		p->metrics[RTAX_LOCK-1] = INETPEER_METRICS_NEW;
		p->rate_tokens = 0;
		p->rate_last = 0;
		p->pmtu_expires = 0;
		p->pmtu_orig = 0;
		memset(&p->redirect_learned, 0, sizeof(p->redirect_learned));
		INIT_LIST_HEAD(&p->gc_list);

		
		link_to_pool(p, base);
		base->total++;
	}
	write_sequnlock_bh(&base->lock);

	return p;
}
EXPORT_SYMBOL_GPL(inet_getpeer);

void inet_putpeer(struct inet_peer *p)
{
	p->dtime = (__u32)jiffies;
	smp_mb__before_atomic_dec();
	atomic_dec(&p->refcnt);
}
EXPORT_SYMBOL_GPL(inet_putpeer);

#define XRLIM_BURST_FACTOR 6
bool inet_peer_xrlim_allow(struct inet_peer *peer, int timeout)
{
	unsigned long now, token;
	bool rc = false;

	if (!peer)
		return true;

	token = peer->rate_tokens;
	now = jiffies;
	token += now - peer->rate_last;
	peer->rate_last = now;
	if (token > XRLIM_BURST_FACTOR * timeout)
		token = XRLIM_BURST_FACTOR * timeout;
	if (token >= timeout) {
		token -= timeout;
		rc = true;
	}
	peer->rate_tokens = token;
	return rc;
}
EXPORT_SYMBOL(inet_peer_xrlim_allow);

static void inetpeer_inval_rcu(struct rcu_head *head)
{
	struct inet_peer *p = container_of(head, struct inet_peer, gc_rcu);

	spin_lock_bh(&gc_lock);
	list_add_tail(&p->gc_list, &gc_list);
	spin_unlock_bh(&gc_lock);

	schedule_delayed_work(&gc_work, gc_delay);
}

void inetpeer_invalidate_tree(int family)
{
	struct inet_peer *old, *new, *prev;
	struct inet_peer_base *base = family_to_base(family);

	write_seqlock_bh(&base->lock);

	old = base->root;
	if (old == peer_avl_empty_rcu)
		goto out;

	new = peer_avl_empty_rcu;

	prev = cmpxchg(&base->root, old, new);
	if (prev == old) {
		base->total = 0;
		call_rcu(&prev->gc_rcu, inetpeer_inval_rcu);
	}

out:
	write_sequnlock_bh(&base->lock);
}
EXPORT_SYMBOL(inetpeer_invalidate_tree);
