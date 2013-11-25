
#include <linux/kernel.h>
#include <linux/kmemcheck.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <net/inet_hashtables.h>
#include <net/inet_timewait_sock.h>
#include <net/ip.h>


int inet_twsk_unhash(struct inet_timewait_sock *tw)
{
	if (hlist_nulls_unhashed(&tw->tw_node))
		return 0;

	hlist_nulls_del_rcu(&tw->tw_node);
	sk_nulls_node_init(&tw->tw_node);
	return 1;
}

int inet_twsk_bind_unhash(struct inet_timewait_sock *tw,
			  struct inet_hashinfo *hashinfo)
{
	struct inet_bind_bucket *tb = tw->tw_tb;

	if (!tb)
		return 0;

	__hlist_del(&tw->tw_bind_node);
	tw->tw_tb = NULL;
	inet_bind_bucket_destroy(hashinfo->bind_bucket_cachep, tb);
	return 1;
}

static void __inet_twsk_kill(struct inet_timewait_sock *tw,
			     struct inet_hashinfo *hashinfo)
{
	struct inet_bind_hashbucket *bhead;
	int refcnt;
	
	spinlock_t *lock = inet_ehash_lockp(hashinfo, tw->tw_hash);

	spin_lock(lock);
	refcnt = inet_twsk_unhash(tw);
	spin_unlock(lock);

	
	bhead = &hashinfo->bhash[inet_bhashfn(twsk_net(tw), tw->tw_num,
			hashinfo->bhash_size)];

	spin_lock(&bhead->lock);
	refcnt += inet_twsk_bind_unhash(tw, hashinfo);
	spin_unlock(&bhead->lock);

#ifdef SOCK_REFCNT_DEBUG
	if (atomic_read(&tw->tw_refcnt) != 1) {
		printk(KERN_DEBUG "%s timewait_sock %p refcnt=%d\n",
		       tw->tw_prot->name, tw, atomic_read(&tw->tw_refcnt));
	}
#endif
	while (refcnt) {
		inet_twsk_put(tw);
		refcnt--;
	}
}

static noinline void inet_twsk_free(struct inet_timewait_sock *tw)
{
	struct module *owner = tw->tw_prot->owner;
	twsk_destructor((struct sock *)tw);
#ifdef SOCK_REFCNT_DEBUG
	pr_debug("%s timewait_sock %p released\n", tw->tw_prot->name, tw);
#endif
	release_net(twsk_net(tw));
	kmem_cache_free(tw->tw_prot->twsk_prot->twsk_slab, tw);
	module_put(owner);
}

void inet_twsk_put(struct inet_timewait_sock *tw)
{
	if (atomic_dec_and_test(&tw->tw_refcnt))
		inet_twsk_free(tw);
}
EXPORT_SYMBOL_GPL(inet_twsk_put);

void __inet_twsk_hashdance(struct inet_timewait_sock *tw, struct sock *sk,
			   struct inet_hashinfo *hashinfo)
{
	const struct inet_sock *inet = inet_sk(sk);
	const struct inet_connection_sock *icsk = inet_csk(sk);
	struct inet_ehash_bucket *ehead = inet_ehash_bucket(hashinfo, sk->sk_hash);
	spinlock_t *lock = inet_ehash_lockp(hashinfo, sk->sk_hash);
	struct inet_bind_hashbucket *bhead;
	bhead = &hashinfo->bhash[inet_bhashfn(twsk_net(tw), inet->inet_num,
			hashinfo->bhash_size)];
	spin_lock(&bhead->lock);
	tw->tw_tb = icsk->icsk_bind_hash;
	WARN_ON(!icsk->icsk_bind_hash);
	inet_twsk_add_bind_node(tw, &tw->tw_tb->owners);
	spin_unlock(&bhead->lock);

	spin_lock(lock);

	inet_twsk_add_node_rcu(tw, &ehead->twchain);

	
	if (__sk_nulls_del_node_init_rcu(sk))
		sock_prot_inuse_add(sock_net(sk), sk->sk_prot, -1);

	atomic_add(1 + 1 + 1, &tw->tw_refcnt);

	spin_unlock(lock);
}
EXPORT_SYMBOL_GPL(__inet_twsk_hashdance);

struct inet_timewait_sock *inet_twsk_alloc(const struct sock *sk, const int state)
{
	struct inet_timewait_sock *tw =
		kmem_cache_alloc(sk->sk_prot_creator->twsk_prot->twsk_slab,
				 GFP_ATOMIC);
	if (tw != NULL) {
		const struct inet_sock *inet = inet_sk(sk);

		kmemcheck_annotate_bitfield(tw, flags);

		
		tw->tw_daddr	    = inet->inet_daddr;
		tw->tw_rcv_saddr    = inet->inet_rcv_saddr;
		tw->tw_bound_dev_if = sk->sk_bound_dev_if;
		tw->tw_tos	    = inet->tos;
		tw->tw_num	    = inet->inet_num;
		tw->tw_state	    = TCP_TIME_WAIT;
		tw->tw_substate	    = state;
		tw->tw_sport	    = inet->inet_sport;
		tw->tw_dport	    = inet->inet_dport;
		tw->tw_family	    = sk->sk_family;
		tw->tw_reuse	    = sk->sk_reuse;
		tw->tw_hash	    = sk->sk_hash;
		tw->tw_ipv6only	    = 0;
		tw->tw_transparent  = inet->transparent;
		tw->tw_prot	    = sk->sk_prot_creator;
		twsk_net_set(tw, hold_net(sock_net(sk)));
		atomic_set(&tw->tw_refcnt, 0);
		inet_twsk_dead_node_init(tw);
		__module_get(tw->tw_prot->owner);
	}

	return tw;
}
EXPORT_SYMBOL_GPL(inet_twsk_alloc);

static int inet_twdr_do_twkill_work(struct inet_timewait_death_row *twdr,
				    const int slot)
{
	struct inet_timewait_sock *tw;
	struct hlist_node *node;
	unsigned int killed;
	int ret;

	killed = 0;
	ret = 0;
rescan:
	inet_twsk_for_each_inmate(tw, node, &twdr->cells[slot]) {
		__inet_twsk_del_dead_node(tw);
		spin_unlock(&twdr->death_lock);
		__inet_twsk_kill(tw, twdr->hashinfo);
#ifdef CONFIG_NET_NS
		NET_INC_STATS_BH(twsk_net(tw), LINUX_MIB_TIMEWAITED);
#endif
		inet_twsk_put(tw);
		killed++;
		spin_lock(&twdr->death_lock);
		if (killed > INET_TWDR_TWKILL_QUOTA) {
			ret = 1;
			break;
		}

		goto rescan;
	}

	twdr->tw_count -= killed;
#ifndef CONFIG_NET_NS
	NET_ADD_STATS_BH(&init_net, LINUX_MIB_TIMEWAITED, killed);
#endif
	return ret;
}

void inet_twdr_hangman(unsigned long data)
{
	struct inet_timewait_death_row *twdr;
	int unsigned need_timer;

	twdr = (struct inet_timewait_death_row *)data;
	spin_lock(&twdr->death_lock);

	if (twdr->tw_count == 0)
		goto out;

	need_timer = 0;
	if (inet_twdr_do_twkill_work(twdr, twdr->slot)) {
		twdr->thread_slots |= (1 << twdr->slot);
		schedule_work(&twdr->twkill_work);
		need_timer = 1;
	} else {
		
		if (twdr->tw_count)
			need_timer = 1;
		twdr->slot = ((twdr->slot + 1) & (INET_TWDR_TWKILL_SLOTS - 1));
	}
	if (need_timer)
		mod_timer(&twdr->tw_timer, jiffies + twdr->period);
out:
	spin_unlock(&twdr->death_lock);
}
EXPORT_SYMBOL_GPL(inet_twdr_hangman);

void inet_twdr_twkill_work(struct work_struct *work)
{
	struct inet_timewait_death_row *twdr =
		container_of(work, struct inet_timewait_death_row, twkill_work);
	int i;

	BUILD_BUG_ON((INET_TWDR_TWKILL_SLOTS - 1) >
			(sizeof(twdr->thread_slots) * 8));

	while (twdr->thread_slots) {
		spin_lock_bh(&twdr->death_lock);
		for (i = 0; i < INET_TWDR_TWKILL_SLOTS; i++) {
			if (!(twdr->thread_slots & (1 << i)))
				continue;

			while (inet_twdr_do_twkill_work(twdr, i) != 0) {
				if (need_resched()) {
					spin_unlock_bh(&twdr->death_lock);
					schedule();
					spin_lock_bh(&twdr->death_lock);
				}
			}

			twdr->thread_slots &= ~(1 << i);
		}
		spin_unlock_bh(&twdr->death_lock);
	}
}
EXPORT_SYMBOL_GPL(inet_twdr_twkill_work);


void inet_twsk_deschedule(struct inet_timewait_sock *tw,
			  struct inet_timewait_death_row *twdr)
{
	spin_lock(&twdr->death_lock);
	if (inet_twsk_del_dead_node(tw)) {
		inet_twsk_put(tw);
		if (--twdr->tw_count == 0)
			del_timer(&twdr->tw_timer);
	}
	spin_unlock(&twdr->death_lock);
	__inet_twsk_kill(tw, twdr->hashinfo);
}
EXPORT_SYMBOL(inet_twsk_deschedule);

void inet_twsk_schedule(struct inet_timewait_sock *tw,
		       struct inet_timewait_death_row *twdr,
		       const int timeo, const int timewait_len)
{
	struct hlist_head *list;
	int slot;

	slot = (timeo + (1 << INET_TWDR_RECYCLE_TICK) - 1) >> INET_TWDR_RECYCLE_TICK;

	spin_lock(&twdr->death_lock);

	
	if (inet_twsk_del_dead_node(tw))
		twdr->tw_count--;
	else
		atomic_inc(&tw->tw_refcnt);

	if (slot >= INET_TWDR_RECYCLE_SLOTS) {
		
		if (timeo >= timewait_len) {
			slot = INET_TWDR_TWKILL_SLOTS - 1;
		} else {
			slot = DIV_ROUND_UP(timeo, twdr->period);
			if (slot >= INET_TWDR_TWKILL_SLOTS)
				slot = INET_TWDR_TWKILL_SLOTS - 1;
		}
		tw->tw_ttd = jiffies + timeo;
		slot = (twdr->slot + slot) & (INET_TWDR_TWKILL_SLOTS - 1);
		list = &twdr->cells[slot];
	} else {
		tw->tw_ttd = jiffies + (slot << INET_TWDR_RECYCLE_TICK);

		if (twdr->twcal_hand < 0) {
			twdr->twcal_hand = 0;
			twdr->twcal_jiffie = jiffies;
			twdr->twcal_timer.expires = twdr->twcal_jiffie +
					      (slot << INET_TWDR_RECYCLE_TICK);
			add_timer(&twdr->twcal_timer);
		} else {
			if (time_after(twdr->twcal_timer.expires,
				       jiffies + (slot << INET_TWDR_RECYCLE_TICK)))
				mod_timer(&twdr->twcal_timer,
					  jiffies + (slot << INET_TWDR_RECYCLE_TICK));
			slot = (twdr->twcal_hand + slot) & (INET_TWDR_RECYCLE_SLOTS - 1);
		}
		list = &twdr->twcal_row[slot];
	}

	hlist_add_head(&tw->tw_death_node, list);

	if (twdr->tw_count++ == 0)
		mod_timer(&twdr->tw_timer, jiffies + twdr->period);
	spin_unlock(&twdr->death_lock);
}
EXPORT_SYMBOL_GPL(inet_twsk_schedule);

void inet_twdr_twcal_tick(unsigned long data)
{
	struct inet_timewait_death_row *twdr;
	int n, slot;
	unsigned long j;
	unsigned long now = jiffies;
	int killed = 0;
	int adv = 0;

	twdr = (struct inet_timewait_death_row *)data;

	spin_lock(&twdr->death_lock);
	if (twdr->twcal_hand < 0)
		goto out;

	slot = twdr->twcal_hand;
	j = twdr->twcal_jiffie;

	for (n = 0; n < INET_TWDR_RECYCLE_SLOTS; n++) {
		if (time_before_eq(j, now)) {
			struct hlist_node *node, *safe;
			struct inet_timewait_sock *tw;

			inet_twsk_for_each_inmate_safe(tw, node, safe,
						       &twdr->twcal_row[slot]) {
				__inet_twsk_del_dead_node(tw);
				__inet_twsk_kill(tw, twdr->hashinfo);
#ifdef CONFIG_NET_NS
				NET_INC_STATS_BH(twsk_net(tw), LINUX_MIB_TIMEWAITKILLED);
#endif
				inet_twsk_put(tw);
				killed++;
			}
		} else {
			if (!adv) {
				adv = 1;
				twdr->twcal_jiffie = j;
				twdr->twcal_hand = slot;
			}

			if (!hlist_empty(&twdr->twcal_row[slot])) {
				mod_timer(&twdr->twcal_timer, j);
				goto out;
			}
		}
		j += 1 << INET_TWDR_RECYCLE_TICK;
		slot = (slot + 1) & (INET_TWDR_RECYCLE_SLOTS - 1);
	}
	twdr->twcal_hand = -1;

out:
	if ((twdr->tw_count -= killed) == 0)
		del_timer(&twdr->tw_timer);
#ifndef CONFIG_NET_NS
	NET_ADD_STATS_BH(&init_net, LINUX_MIB_TIMEWAITKILLED, killed);
#endif
	spin_unlock(&twdr->death_lock);
}
EXPORT_SYMBOL_GPL(inet_twdr_twcal_tick);

void inet_twsk_purge(struct inet_hashinfo *hashinfo,
		     struct inet_timewait_death_row *twdr, int family)
{
	struct inet_timewait_sock *tw;
	struct sock *sk;
	struct hlist_nulls_node *node;
	unsigned int slot;

	for (slot = 0; slot <= hashinfo->ehash_mask; slot++) {
		struct inet_ehash_bucket *head = &hashinfo->ehash[slot];
restart_rcu:
		rcu_read_lock();
restart:
		sk_nulls_for_each_rcu(sk, node, &head->twchain) {
			tw = inet_twsk(sk);
			if ((tw->tw_family != family) ||
				atomic_read(&twsk_net(tw)->count))
				continue;

			if (unlikely(!atomic_inc_not_zero(&tw->tw_refcnt)))
				continue;

			if (unlikely((tw->tw_family != family) ||
				     atomic_read(&twsk_net(tw)->count))) {
				inet_twsk_put(tw);
				goto restart;
			}

			rcu_read_unlock();
			local_bh_disable();
			inet_twsk_deschedule(tw, twdr);
			local_bh_enable();
			inet_twsk_put(tw);
			goto restart_rcu;
		}
		if (get_nulls_value(node) != slot)
			goto restart;
		rcu_read_unlock();
	}
}
EXPORT_SYMBOL_GPL(inet_twsk_purge);
