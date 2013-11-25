/* Key permission checking
 *
 * Copyright (C) 2005 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/security.h>
#include "internal.h"

int key_task_permission(const key_ref_t key_ref, const struct cred *cred,
			key_perm_t perm)
{
	struct key *key;
	key_perm_t kperm;
	int ret;

	key = key_ref_to_ptr(key_ref);

	if (key->user->user_ns != cred->user->user_ns)
		goto use_other_perms;

	
	if (key->uid == cred->fsuid) {
		kperm = key->perm >> 16;
		goto use_these_perms;
	}

	if (key->gid != -1 && key->perm & KEY_GRP_ALL) {
		if (key->gid == cred->fsgid) {
			kperm = key->perm >> 8;
			goto use_these_perms;
		}

		ret = groups_search(cred->group_info, key->gid);
		if (ret) {
			kperm = key->perm >> 8;
			goto use_these_perms;
		}
	}

use_other_perms:

	
	kperm = key->perm;

use_these_perms:

	if (is_key_possessed(key_ref))
		kperm |= key->perm >> 24;

	kperm = kperm & perm & KEY_ALL;

	if (kperm != perm)
		return -EACCES;

	
	return security_key_permission(key_ref, cred, perm);
}
EXPORT_SYMBOL(key_task_permission);

int key_validate(struct key *key)
{
	struct timespec now;
	int ret = 0;

	if (key) {
		
		ret = -EKEYREVOKED;
		if (test_bit(KEY_FLAG_REVOKED, &key->flags) ||
		    test_bit(KEY_FLAG_DEAD, &key->flags))
			goto error;

		
		ret = 0;
		if (key->expiry) {
			now = current_kernel_time();
			if (now.tv_sec >= key->expiry)
				ret = -EKEYEXPIRED;
		}
	}

error:
	return ret;
}
EXPORT_SYMBOL(key_validate);
