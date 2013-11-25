/* Key garbage collector
 *
 * Copyright (C) 2009-2011 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/security.h>
#include <keys/keyring-type.h>
#include "internal.h"

unsigned key_gc_delay = 5 * 60;

static void key_garbage_collector(struct work_struct *work);
DECLARE_WORK(key_gc_work, key_garbage_collector);

static void key_gc_timer_func(unsigned long);
static DEFINE_TIMER(key_gc_timer, key_gc_timer_func, 0, 0);

static time_t key_gc_next_run = LONG_MAX;
static struct key_type *key_gc_dead_keytype;

static unsigned long key_gc_flags;
#define KEY_GC_KEY_EXPIRED	0	
#define KEY_GC_REAP_KEYTYPE	1	
#define KEY_GC_REAPING_KEYTYPE	2	


struct key_type key_type_dead = {
	.name = "dead",
};

void key_schedule_gc(time_t gc_at)
{
	unsigned long expires;
	time_t now = current_kernel_time().tv_sec;

	kenter("%ld", gc_at - now);

	if (gc_at <= now || test_bit(KEY_GC_REAP_KEYTYPE, &key_gc_flags)) {
		kdebug("IMMEDIATE");
		queue_work(system_nrt_wq, &key_gc_work);
	} else if (gc_at < key_gc_next_run) {
		kdebug("DEFERRED");
		key_gc_next_run = gc_at;
		expires = jiffies + (gc_at - now) * HZ;
		mod_timer(&key_gc_timer, expires);
	}
}

static void key_gc_timer_func(unsigned long data)
{
	kenter("");
	key_gc_next_run = LONG_MAX;
	set_bit(KEY_GC_KEY_EXPIRED, &key_gc_flags);
	queue_work(system_nrt_wq, &key_gc_work);
}

static int key_gc_wait_bit(void *flags)
{
	schedule();
	return 0;
}

void key_gc_keytype(struct key_type *ktype)
{
	kenter("%s", ktype->name);

	key_gc_dead_keytype = ktype;
	set_bit(KEY_GC_REAPING_KEYTYPE, &key_gc_flags);
	smp_mb();
	set_bit(KEY_GC_REAP_KEYTYPE, &key_gc_flags);

	kdebug("schedule");
	queue_work(system_nrt_wq, &key_gc_work);

	kdebug("sleep");
	wait_on_bit(&key_gc_flags, KEY_GC_REAPING_KEYTYPE, key_gc_wait_bit,
		    TASK_UNINTERRUPTIBLE);

	key_gc_dead_keytype = NULL;
	kleave("");
}

static void key_gc_keyring(struct key *keyring, time_t limit)
{
	struct keyring_list *klist;
	struct key *key;
	int loop;

	kenter("%x", key_serial(keyring));

	if (test_bit(KEY_FLAG_REVOKED, &keyring->flags))
		goto dont_gc;

	
	rcu_read_lock();
	klist = rcu_dereference(keyring->payload.subscriptions);
	if (!klist)
		goto unlock_dont_gc;

	loop = klist->nkeys;
	smp_rmb();
	for (loop--; loop >= 0; loop--) {
		key = klist->keys[loop];
		if (test_bit(KEY_FLAG_DEAD, &key->flags) ||
		    (key->expiry > 0 && key->expiry <= limit))
			goto do_gc;
	}

unlock_dont_gc:
	rcu_read_unlock();
dont_gc:
	kleave(" [no gc]");
	return;

do_gc:
	rcu_read_unlock();

	keyring_gc(keyring, limit);
	kleave(" [gc]");
}

static noinline void key_gc_unused_key(struct key *key)
{
	key_check(key);

	security_key_free(key);

	
	if (test_bit(KEY_FLAG_IN_QUOTA, &key->flags)) {
		spin_lock(&key->user->lock);
		key->user->qnkeys--;
		key->user->qnbytes -= key->quotalen;
		spin_unlock(&key->user->lock);
	}

	atomic_dec(&key->user->nkeys);
	if (test_bit(KEY_FLAG_INSTANTIATED, &key->flags))
		atomic_dec(&key->user->nikeys);

	key_user_put(key->user);

	
	if (key->type->destroy)
		key->type->destroy(key);

	kfree(key->description);

#ifdef KEY_DEBUGGING
	key->magic = KEY_DEBUG_MAGIC_X;
#endif
	kmem_cache_free(key_jar, key);
}

static void key_garbage_collector(struct work_struct *work)
{
	static u8 gc_state;		
#define KEY_GC_REAP_AGAIN	0x01	
#define KEY_GC_REAPING_LINKS	0x02	
#define KEY_GC_SET_TIMER	0x04	
#define KEY_GC_REAPING_DEAD_1	0x10	
#define KEY_GC_REAPING_DEAD_2	0x20	
#define KEY_GC_REAPING_DEAD_3	0x40	
#define KEY_GC_FOUND_DEAD_KEY	0x80	

	struct rb_node *cursor;
	struct key *key;
	time_t new_timer, limit;

	kenter("[%lx,%x]", key_gc_flags, gc_state);

	limit = current_kernel_time().tv_sec;
	if (limit > key_gc_delay)
		limit -= key_gc_delay;
	else
		limit = key_gc_delay;

	
	gc_state &= KEY_GC_REAPING_DEAD_1 | KEY_GC_REAPING_DEAD_2;
	gc_state <<= 1;
	if (test_and_clear_bit(KEY_GC_KEY_EXPIRED, &key_gc_flags))
		gc_state |= KEY_GC_REAPING_LINKS | KEY_GC_SET_TIMER;

	if (test_and_clear_bit(KEY_GC_REAP_KEYTYPE, &key_gc_flags))
		gc_state |= KEY_GC_REAPING_DEAD_1;
	kdebug("new pass %x", gc_state);

	new_timer = LONG_MAX;

	spin_lock(&key_serial_lock);
	cursor = rb_first(&key_serial_tree);

continue_scanning:
	while (cursor) {
		key = rb_entry(cursor, struct key, serial_node);
		cursor = rb_next(cursor);

		if (atomic_read(&key->usage) == 0)
			goto found_unreferenced_key;

		if (unlikely(gc_state & KEY_GC_REAPING_DEAD_1)) {
			if (key->type == key_gc_dead_keytype) {
				gc_state |= KEY_GC_FOUND_DEAD_KEY;
				set_bit(KEY_FLAG_DEAD, &key->flags);
				key->perm = 0;
				goto skip_dead_key;
			}
		}

		if (gc_state & KEY_GC_SET_TIMER) {
			if (key->expiry > limit && key->expiry < new_timer) {
				kdebug("will expire %x in %ld",
				       key_serial(key), key->expiry - limit);
				new_timer = key->expiry;
			}
		}

		if (unlikely(gc_state & KEY_GC_REAPING_DEAD_2))
			if (key->type == key_gc_dead_keytype)
				gc_state |= KEY_GC_FOUND_DEAD_KEY;

		if ((gc_state & KEY_GC_REAPING_LINKS) ||
		    unlikely(gc_state & KEY_GC_REAPING_DEAD_2)) {
			if (key->type == &key_type_keyring)
				goto found_keyring;
		}

		if (unlikely(gc_state & KEY_GC_REAPING_DEAD_3))
			if (key->type == key_gc_dead_keytype)
				goto destroy_dead_key;

	skip_dead_key:
		if (spin_is_contended(&key_serial_lock) || need_resched())
			goto contended;
	}

contended:
	spin_unlock(&key_serial_lock);

maybe_resched:
	if (cursor) {
		cond_resched();
		spin_lock(&key_serial_lock);
		goto continue_scanning;
	}

	kdebug("pass complete");

	if (gc_state & KEY_GC_SET_TIMER && new_timer != (time_t)LONG_MAX) {
		new_timer += key_gc_delay;
		key_schedule_gc(new_timer);
	}

	if (unlikely(gc_state & KEY_GC_REAPING_DEAD_2)) {
		kdebug("dead sync");
		synchronize_rcu();
	}

	if (unlikely(gc_state & (KEY_GC_REAPING_DEAD_1 |
				 KEY_GC_REAPING_DEAD_2))) {
		if (!(gc_state & KEY_GC_FOUND_DEAD_KEY)) {
			kdebug("dead short");
			gc_state &= ~(KEY_GC_REAPING_DEAD_1 | KEY_GC_REAPING_DEAD_2);
			gc_state |= KEY_GC_REAPING_DEAD_3;
		} else {
			gc_state |= KEY_GC_REAP_AGAIN;
		}
	}

	if (unlikely(gc_state & KEY_GC_REAPING_DEAD_3)) {
		kdebug("dead wake");
		smp_mb();
		clear_bit(KEY_GC_REAPING_KEYTYPE, &key_gc_flags);
		wake_up_bit(&key_gc_flags, KEY_GC_REAPING_KEYTYPE);
	}

	if (gc_state & KEY_GC_REAP_AGAIN)
		queue_work(system_nrt_wq, &key_gc_work);
	kleave(" [end %x]", gc_state);
	return;

found_unreferenced_key:
	kdebug("unrefd key %d", key->serial);
	rb_erase(&key->serial_node, &key_serial_tree);
	spin_unlock(&key_serial_lock);

	key_gc_unused_key(key);
	gc_state |= KEY_GC_REAP_AGAIN;
	goto maybe_resched;

found_keyring:
	spin_unlock(&key_serial_lock);
	kdebug("scan keyring %d", key->serial);
	key_gc_keyring(key, limit);
	goto maybe_resched;

destroy_dead_key:
	spin_unlock(&key_serial_lock);
	kdebug("destroy key %d", key->serial);
	down_write(&key->sem);
	key->type = &key_type_dead;
	if (key_gc_dead_keytype->destroy)
		key_gc_dead_keytype->destroy(key);
	memset(&key->payload, KEY_DESTROY, sizeof(key->payload));
	up_write(&key->sem);
	goto maybe_resched;
}
