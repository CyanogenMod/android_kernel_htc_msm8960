/* Definitions for key type implementations
 *
 * Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */

#ifndef _LINUX_KEY_TYPE_H
#define _LINUX_KEY_TYPE_H

#include <linux/key.h>

#ifdef CONFIG_KEYS

struct key_construction {
	struct key	*key;	
	struct key	*authkey;
};

typedef int (*request_key_actor_t)(struct key_construction *key,
				   const char *op, void *aux);

struct key_type {
	
	const char *name;

	size_t def_datalen;

	
	int (*vet_description)(const char *description);

	int (*instantiate)(struct key *key, const void *data, size_t datalen);

	int (*update)(struct key *key, const void *data, size_t datalen);

	
	int (*match)(const struct key *key, const void *desc);

	void (*revoke)(struct key *key);

	
	void (*destroy)(struct key *key);

	
	void (*describe)(const struct key *key, struct seq_file *p);

	long (*read)(const struct key *key, char __user *buffer, size_t buflen);

	request_key_actor_t request_key;

	
	struct list_head	link;		
	struct lock_class_key	lock_class;	
};

extern struct key_type key_type_keyring;

extern int register_key_type(struct key_type *ktype);
extern void unregister_key_type(struct key_type *ktype);

extern int key_payload_reserve(struct key *key, size_t datalen);
extern int key_instantiate_and_link(struct key *key,
				    const void *data,
				    size_t datalen,
				    struct key *keyring,
				    struct key *instkey);
extern int key_reject_and_link(struct key *key,
			       unsigned timeout,
			       unsigned error,
			       struct key *keyring,
			       struct key *instkey);
extern void complete_request_key(struct key_construction *cons, int error);

static inline int key_negate_and_link(struct key *key,
				      unsigned timeout,
				      struct key *keyring,
				      struct key *instkey)
{
	return key_reject_and_link(key, timeout, ENOKEY, keyring, instkey);
}

#endif 
#endif 
