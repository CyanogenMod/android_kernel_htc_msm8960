/* Basic authentication token and access key management
 *
 * Copyright (C) 2004-2008 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/poison.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/security.h>
#include <linux/workqueue.h>
#include <linux/random.h>
#include <linux/err.h>
#include <linux/user_namespace.h>
#include "internal.h"

struct kmem_cache *key_jar;
struct rb_root		key_serial_tree; 
DEFINE_SPINLOCK(key_serial_lock);

struct rb_root	key_user_tree; 
DEFINE_SPINLOCK(key_user_lock);

unsigned int key_quota_root_maxkeys = 200;	
unsigned int key_quota_root_maxbytes = 20000;	
unsigned int key_quota_maxkeys = 200;		
unsigned int key_quota_maxbytes = 20000;	

static LIST_HEAD(key_types_list);
static DECLARE_RWSEM(key_types_sem);

DEFINE_MUTEX(key_construction_mutex);

#ifdef KEY_DEBUGGING
void __key_check(const struct key *key)
{
	printk("__key_check: key %p {%08x} should be {%08x}\n",
	       key, key->magic, KEY_DEBUG_MAGIC);
	BUG();
}
#endif

struct key_user *key_user_lookup(uid_t uid, struct user_namespace *user_ns)
{
	struct key_user *candidate = NULL, *user;
	struct rb_node *parent = NULL;
	struct rb_node **p;

try_again:
	p = &key_user_tree.rb_node;
	spin_lock(&key_user_lock);

	
	while (*p) {
		parent = *p;
		user = rb_entry(parent, struct key_user, node);

		if (uid < user->uid)
			p = &(*p)->rb_left;
		else if (uid > user->uid)
			p = &(*p)->rb_right;
		else if (user_ns < user->user_ns)
			p = &(*p)->rb_left;
		else if (user_ns > user->user_ns)
			p = &(*p)->rb_right;
		else
			goto found;
	}

	
	if (!candidate) {
		spin_unlock(&key_user_lock);

		user = NULL;
		candidate = kmalloc(sizeof(struct key_user), GFP_KERNEL);
		if (unlikely(!candidate))
			goto out;

		goto try_again;
	}

	atomic_set(&candidate->usage, 1);
	atomic_set(&candidate->nkeys, 0);
	atomic_set(&candidate->nikeys, 0);
	candidate->uid = uid;
	candidate->user_ns = get_user_ns(user_ns);
	candidate->qnkeys = 0;
	candidate->qnbytes = 0;
	spin_lock_init(&candidate->lock);
	mutex_init(&candidate->cons_lock);

	rb_link_node(&candidate->node, parent, p);
	rb_insert_color(&candidate->node, &key_user_tree);
	spin_unlock(&key_user_lock);
	user = candidate;
	goto out;

	
found:
	atomic_inc(&user->usage);
	spin_unlock(&key_user_lock);
	kfree(candidate);
out:
	return user;
}

void key_user_put(struct key_user *user)
{
	if (atomic_dec_and_lock(&user->usage, &key_user_lock)) {
		rb_erase(&user->node, &key_user_tree);
		spin_unlock(&key_user_lock);
		put_user_ns(user->user_ns);

		kfree(user);
	}
}

static inline void key_alloc_serial(struct key *key)
{
	struct rb_node *parent, **p;
	struct key *xkey;

	do {
		get_random_bytes(&key->serial, sizeof(key->serial));

		key->serial >>= 1; 
	} while (key->serial < 3);

	spin_lock(&key_serial_lock);

attempt_insertion:
	parent = NULL;
	p = &key_serial_tree.rb_node;

	while (*p) {
		parent = *p;
		xkey = rb_entry(parent, struct key, serial_node);

		if (key->serial < xkey->serial)
			p = &(*p)->rb_left;
		else if (key->serial > xkey->serial)
			p = &(*p)->rb_right;
		else
			goto serial_exists;
	}

	
	rb_link_node(&key->serial_node, parent, p);
	rb_insert_color(&key->serial_node, &key_serial_tree);

	spin_unlock(&key_serial_lock);
	return;

serial_exists:
	for (;;) {
		key->serial++;
		if (key->serial < 3) {
			key->serial = 3;
			goto attempt_insertion;
		}

		parent = rb_next(parent);
		if (!parent)
			goto attempt_insertion;

		xkey = rb_entry(parent, struct key, serial_node);
		if (key->serial < xkey->serial)
			goto attempt_insertion;
	}
}

struct key *key_alloc(struct key_type *type, const char *desc,
		      uid_t uid, gid_t gid, const struct cred *cred,
		      key_perm_t perm, unsigned long flags)
{
	struct key_user *user = NULL;
	struct key *key;
	size_t desclen, quotalen;
	int ret;

	key = ERR_PTR(-EINVAL);
	if (!desc || !*desc)
		goto error;

	if (type->vet_description) {
		ret = type->vet_description(desc);
		if (ret < 0) {
			key = ERR_PTR(ret);
			goto error;
		}
	}

	desclen = strlen(desc) + 1;
	quotalen = desclen + type->def_datalen;

	
	user = key_user_lookup(uid, cred->user->user_ns);
	if (!user)
		goto no_memory_1;

	if (!(flags & KEY_ALLOC_NOT_IN_QUOTA)) {
		unsigned maxkeys = (uid == 0) ?
			key_quota_root_maxkeys : key_quota_maxkeys;
		unsigned maxbytes = (uid == 0) ?
			key_quota_root_maxbytes : key_quota_maxbytes;

		spin_lock(&user->lock);
		if (!(flags & KEY_ALLOC_QUOTA_OVERRUN)) {
			if (user->qnkeys + 1 >= maxkeys ||
			    user->qnbytes + quotalen >= maxbytes ||
			    user->qnbytes + quotalen < user->qnbytes)
				goto no_quota;
		}

		user->qnkeys++;
		user->qnbytes += quotalen;
		spin_unlock(&user->lock);
	}

	
	key = kmem_cache_alloc(key_jar, GFP_KERNEL);
	if (!key)
		goto no_memory_2;

	if (desc) {
		key->description = kmemdup(desc, desclen, GFP_KERNEL);
		if (!key->description)
			goto no_memory_3;
	}

	atomic_set(&key->usage, 1);
	init_rwsem(&key->sem);
	lockdep_set_class(&key->sem, &type->lock_class);
	key->type = type;
	key->user = user;
	key->quotalen = quotalen;
	key->datalen = type->def_datalen;
	key->uid = uid;
	key->gid = gid;
	key->perm = perm;
	key->flags = 0;
	key->expiry = 0;
	key->payload.data = NULL;
	key->security = NULL;

	if (!(flags & KEY_ALLOC_NOT_IN_QUOTA))
		key->flags |= 1 << KEY_FLAG_IN_QUOTA;

	memset(&key->type_data, 0, sizeof(key->type_data));

#ifdef KEY_DEBUGGING
	key->magic = KEY_DEBUG_MAGIC;
#endif

	
	ret = security_key_alloc(key, cred, flags);
	if (ret < 0)
		goto security_error;

	
	atomic_inc(&user->nkeys);
	key_alloc_serial(key);

error:
	return key;

security_error:
	kfree(key->description);
	kmem_cache_free(key_jar, key);
	if (!(flags & KEY_ALLOC_NOT_IN_QUOTA)) {
		spin_lock(&user->lock);
		user->qnkeys--;
		user->qnbytes -= quotalen;
		spin_unlock(&user->lock);
	}
	key_user_put(user);
	key = ERR_PTR(ret);
	goto error;

no_memory_3:
	kmem_cache_free(key_jar, key);
no_memory_2:
	if (!(flags & KEY_ALLOC_NOT_IN_QUOTA)) {
		spin_lock(&user->lock);
		user->qnkeys--;
		user->qnbytes -= quotalen;
		spin_unlock(&user->lock);
	}
	key_user_put(user);
no_memory_1:
	key = ERR_PTR(-ENOMEM);
	goto error;

no_quota:
	spin_unlock(&user->lock);
	key_user_put(user);
	key = ERR_PTR(-EDQUOT);
	goto error;
}
EXPORT_SYMBOL(key_alloc);

int key_payload_reserve(struct key *key, size_t datalen)
{
	int delta = (int)datalen - key->datalen;
	int ret = 0;

	key_check(key);

	
	if (delta != 0 && test_bit(KEY_FLAG_IN_QUOTA, &key->flags)) {
		unsigned maxbytes = (key->user->uid == 0) ?
			key_quota_root_maxbytes : key_quota_maxbytes;

		spin_lock(&key->user->lock);

		if (delta > 0 &&
		    (key->user->qnbytes + delta >= maxbytes ||
		     key->user->qnbytes + delta < key->user->qnbytes)) {
			ret = -EDQUOT;
		}
		else {
			key->user->qnbytes += delta;
			key->quotalen += delta;
		}
		spin_unlock(&key->user->lock);
	}

	
	if (ret == 0)
		key->datalen = datalen;

	return ret;
}
EXPORT_SYMBOL(key_payload_reserve);

static int __key_instantiate_and_link(struct key *key,
				      const void *data,
				      size_t datalen,
				      struct key *keyring,
				      struct key *authkey,
				      unsigned long *_prealloc)
{
	int ret, awaken;

	key_check(key);
	key_check(keyring);

	awaken = 0;
	ret = -EBUSY;

	mutex_lock(&key_construction_mutex);

	
	if (!test_bit(KEY_FLAG_INSTANTIATED, &key->flags)) {
		
		ret = key->type->instantiate(key, data, datalen);

		if (ret == 0) {
			
			atomic_inc(&key->user->nikeys);
			set_bit(KEY_FLAG_INSTANTIATED, &key->flags);

			if (test_and_clear_bit(KEY_FLAG_USER_CONSTRUCT, &key->flags))
				awaken = 1;

			
			if (keyring)
				__key_link(keyring, key, _prealloc);

			
			if (authkey)
				key_revoke(authkey);
		}
	}

	mutex_unlock(&key_construction_mutex);

	
	if (awaken)
		wake_up_bit(&key->flags, KEY_FLAG_USER_CONSTRUCT);

	return ret;
}

int key_instantiate_and_link(struct key *key,
			     const void *data,
			     size_t datalen,
			     struct key *keyring,
			     struct key *authkey)
{
	unsigned long prealloc;
	int ret;

	if (keyring) {
		ret = __key_link_begin(keyring, key->type, key->description,
				       &prealloc);
		if (ret < 0)
			return ret;
	}

	ret = __key_instantiate_and_link(key, data, datalen, keyring, authkey,
					 &prealloc);

	if (keyring)
		__key_link_end(keyring, key->type, prealloc);

	return ret;
}

EXPORT_SYMBOL(key_instantiate_and_link);

int key_reject_and_link(struct key *key,
			unsigned timeout,
			unsigned error,
			struct key *keyring,
			struct key *authkey)
{
	unsigned long prealloc;
	struct timespec now;
	int ret, awaken, link_ret = 0;

	key_check(key);
	key_check(keyring);

	awaken = 0;
	ret = -EBUSY;

	if (keyring)
		link_ret = __key_link_begin(keyring, key->type,
					    key->description, &prealloc);

	mutex_lock(&key_construction_mutex);

	
	if (!test_bit(KEY_FLAG_INSTANTIATED, &key->flags)) {
		
		atomic_inc(&key->user->nikeys);
		set_bit(KEY_FLAG_NEGATIVE, &key->flags);
		set_bit(KEY_FLAG_INSTANTIATED, &key->flags);
		key->type_data.reject_error = -error;
		now = current_kernel_time();
		key->expiry = now.tv_sec + timeout;
		key_schedule_gc(key->expiry + key_gc_delay);

		if (test_and_clear_bit(KEY_FLAG_USER_CONSTRUCT, &key->flags))
			awaken = 1;

		ret = 0;

		
		if (keyring && link_ret == 0)
			__key_link(keyring, key, &prealloc);

		
		if (authkey)
			key_revoke(authkey);
	}

	mutex_unlock(&key_construction_mutex);

	if (keyring)
		__key_link_end(keyring, key->type, prealloc);

	
	if (awaken)
		wake_up_bit(&key->flags, KEY_FLAG_USER_CONSTRUCT);

	return ret == 0 ? link_ret : ret;
}
EXPORT_SYMBOL(key_reject_and_link);

void key_put(struct key *key)
{
	if (key) {
		key_check(key);

		if (atomic_dec_and_test(&key->usage))
			queue_work(system_nrt_wq, &key_gc_work);
	}
}
EXPORT_SYMBOL(key_put);

struct key *key_lookup(key_serial_t id)
{
	struct rb_node *n;
	struct key *key;

	spin_lock(&key_serial_lock);

	
	n = key_serial_tree.rb_node;
	while (n) {
		key = rb_entry(n, struct key, serial_node);

		if (id < key->serial)
			n = n->rb_left;
		else if (id > key->serial)
			n = n->rb_right;
		else
			goto found;
	}

not_found:
	key = ERR_PTR(-ENOKEY);
	goto error;

found:
	
	if (atomic_read(&key->usage) == 0)
		goto not_found;

	atomic_inc(&key->usage);

error:
	spin_unlock(&key_serial_lock);
	return key;
}

struct key_type *key_type_lookup(const char *type)
{
	struct key_type *ktype;

	down_read(&key_types_sem);

	list_for_each_entry(ktype, &key_types_list, link) {
		if (strcmp(ktype->name, type) == 0)
			goto found_kernel_type;
	}

	up_read(&key_types_sem);
	ktype = ERR_PTR(-ENOKEY);

found_kernel_type:
	return ktype;
}

void key_set_timeout(struct key *key, unsigned timeout)
{
	struct timespec now;
	time_t expiry = 0;

	
	down_write(&key->sem);

	if (timeout > 0) {
		now = current_kernel_time();
		expiry = now.tv_sec + timeout;
	}

	key->expiry = expiry;
	key_schedule_gc(key->expiry + key_gc_delay);

	up_write(&key->sem);
}
EXPORT_SYMBOL_GPL(key_set_timeout);

void key_type_put(struct key_type *ktype)
{
	up_read(&key_types_sem);
}

static inline key_ref_t __key_update(key_ref_t key_ref,
				     const void *payload, size_t plen)
{
	struct key *key = key_ref_to_ptr(key_ref);
	int ret;

	
	ret = key_permission(key_ref, KEY_WRITE);
	if (ret < 0)
		goto error;

	ret = -EEXIST;
	if (!key->type->update)
		goto error;

	down_write(&key->sem);

	ret = key->type->update(key, payload, plen);
	if (ret == 0)
		
		clear_bit(KEY_FLAG_NEGATIVE, &key->flags);

	up_write(&key->sem);

	if (ret < 0)
		goto error;
out:
	return key_ref;

error:
	key_put(key);
	key_ref = ERR_PTR(ret);
	goto out;
}

key_ref_t key_create_or_update(key_ref_t keyring_ref,
			       const char *type,
			       const char *description,
			       const void *payload,
			       size_t plen,
			       key_perm_t perm,
			       unsigned long flags)
{
	unsigned long prealloc;
	const struct cred *cred = current_cred();
	struct key_type *ktype;
	struct key *keyring, *key = NULL;
	key_ref_t key_ref;
	int ret;

	ktype = key_type_lookup(type);
	if (IS_ERR(ktype)) {
		key_ref = ERR_PTR(-ENODEV);
		goto error;
	}

	key_ref = ERR_PTR(-EINVAL);
	if (!ktype->match || !ktype->instantiate)
		goto error_2;

	keyring = key_ref_to_ptr(keyring_ref);

	key_check(keyring);

	key_ref = ERR_PTR(-ENOTDIR);
	if (keyring->type != &key_type_keyring)
		goto error_2;

	ret = __key_link_begin(keyring, ktype, description, &prealloc);
	if (ret < 0)
		goto error_2;

	ret = key_permission(keyring_ref, KEY_WRITE);
	if (ret < 0) {
		key_ref = ERR_PTR(ret);
		goto error_3;
	}

	if (ktype->update) {
		key_ref = __keyring_search_one(keyring_ref, ktype, description,
					       0);
		if (!IS_ERR(key_ref))
			goto found_matching_key;
	}

	
	if (perm == KEY_PERM_UNDEF) {
		perm = KEY_POS_VIEW | KEY_POS_SEARCH | KEY_POS_LINK | KEY_POS_SETATTR;
		perm |= KEY_USR_VIEW | KEY_USR_SEARCH | KEY_USR_LINK | KEY_USR_SETATTR;

		if (ktype->read)
			perm |= KEY_POS_READ | KEY_USR_READ;

		if (ktype == &key_type_keyring || ktype->update)
			perm |= KEY_USR_WRITE;
	}

	
	key = key_alloc(ktype, description, cred->fsuid, cred->fsgid, cred,
			perm, flags);
	if (IS_ERR(key)) {
		key_ref = ERR_CAST(key);
		goto error_3;
	}

	
	ret = __key_instantiate_and_link(key, payload, plen, keyring, NULL,
					 &prealloc);
	if (ret < 0) {
		key_put(key);
		key_ref = ERR_PTR(ret);
		goto error_3;
	}

	key_ref = make_key_ref(key, is_key_possessed(keyring_ref));

 error_3:
	__key_link_end(keyring, ktype, prealloc);
 error_2:
	key_type_put(ktype);
 error:
	return key_ref;

 found_matching_key:
	__key_link_end(keyring, ktype, prealloc);
	key_type_put(ktype);

	key_ref = __key_update(key_ref, payload, plen);
	goto error;
}
EXPORT_SYMBOL(key_create_or_update);

int key_update(key_ref_t key_ref, const void *payload, size_t plen)
{
	struct key *key = key_ref_to_ptr(key_ref);
	int ret;

	key_check(key);

	
	ret = key_permission(key_ref, KEY_WRITE);
	if (ret < 0)
		goto error;

	
	ret = -EOPNOTSUPP;
	if (key->type->update) {
		down_write(&key->sem);

		ret = key->type->update(key, payload, plen);
		if (ret == 0)
			
			clear_bit(KEY_FLAG_NEGATIVE, &key->flags);

		up_write(&key->sem);
	}

 error:
	return ret;
}
EXPORT_SYMBOL(key_update);

void key_revoke(struct key *key)
{
	struct timespec now;
	time_t time;

	key_check(key);

	down_write_nested(&key->sem, 1);
	if (!test_and_set_bit(KEY_FLAG_REVOKED, &key->flags) &&
	    key->type->revoke)
		key->type->revoke(key);

	
	now = current_kernel_time();
	time = now.tv_sec;
	if (key->revoked_at == 0 || key->revoked_at > time) {
		key->revoked_at = time;
		key_schedule_gc(key->revoked_at + key_gc_delay);
	}

	up_write(&key->sem);
}
EXPORT_SYMBOL(key_revoke);

int register_key_type(struct key_type *ktype)
{
	struct key_type *p;
	int ret;

	memset(&ktype->lock_class, 0, sizeof(ktype->lock_class));

	ret = -EEXIST;
	down_write(&key_types_sem);

	
	list_for_each_entry(p, &key_types_list, link) {
		if (strcmp(p->name, ktype->name) == 0)
			goto out;
	}

	
	list_add(&ktype->link, &key_types_list);
	ret = 0;

out:
	up_write(&key_types_sem);
	return ret;
}
EXPORT_SYMBOL(register_key_type);

void unregister_key_type(struct key_type *ktype)
{
	down_write(&key_types_sem);
	list_del_init(&ktype->link);
	downgrade_write(&key_types_sem);
	key_gc_keytype(ktype);
	up_read(&key_types_sem);
}
EXPORT_SYMBOL(unregister_key_type);

void __init key_init(void)
{
	
	key_jar = kmem_cache_create("key_jar", sizeof(struct key),
			0, SLAB_HWCACHE_ALIGN|SLAB_PANIC, NULL);

	
	list_add_tail(&key_type_keyring.link, &key_types_list);
	list_add_tail(&key_type_dead.link, &key_types_list);
	list_add_tail(&key_type_user.link, &key_types_list);
	list_add_tail(&key_type_logon.link, &key_types_list);

	
	rb_link_node(&root_key_user.node,
		     NULL,
		     &key_user_tree.rb_node);

	rb_insert_color(&root_key_user.node,
			&key_user_tree);
}
