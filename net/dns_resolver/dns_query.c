/* Upcall routine, designed to work as a key type and working through
 * /sbin/request-key to contact userspace when handling DNS queries.
 *
 * See Documentation/networking/dns_resolver.txt
 *
 *   Copyright (c) 2007 Igor Mammedov
 *   Author(s): Igor Mammedov (niallain@gmail.com)
 *              Steve French (sfrench@us.ibm.com)
 *              Wang Lei (wang840925@gmail.com)
 *		David Howells (dhowells@redhat.com)
 *
 *   The upcall wrapper used to make an arbitrary DNS query.
 *
 *   This function requires the appropriate userspace tool dns.upcall to be
 *   installed and something like the following lines should be added to the
 *   /etc/request-key.conf file:
 *
 *	create dns_resolver * * /sbin/dns.upcall %k
 *
 *   For example to use this module to query AFSDB RR:
 *
 *	create dns_resolver afsdb:* * /sbin/dns.afsdb %k
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published
 *   by the Free Software Foundation; either version 2.1 of the License, or
 *   (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/dns_resolver.h>
#include <linux/err.h>
#include <keys/dns_resolver-type.h>
#include <keys/user-type.h>

#include "internal.h"

int dns_query(const char *type, const char *name, size_t namelen,
	      const char *options, char **_result, time_t *_expiry)
{
	struct key *rkey;
	struct user_key_payload *upayload;
	const struct cred *saved_cred;
	size_t typelen, desclen;
	char *desc, *cp;
	int ret, len;

	kenter("%s,%*.*s,%zu,%s",
	       type, (int)namelen, (int)namelen, name, namelen, options);

	if (!name || namelen == 0 || !_result)
		return -EINVAL;

	
	typelen = 0;
	desclen = 0;
	if (type) {
		typelen = strlen(type);
		if (typelen < 1)
			return -EINVAL;
		desclen += typelen + 1;
	}

	if (!namelen)
		namelen = strlen(name);
	if (namelen < 3)
		return -EINVAL;
	desclen += namelen + 1;

	desc = kmalloc(desclen, GFP_KERNEL);
	if (!desc)
		return -ENOMEM;

	cp = desc;
	if (type) {
		memcpy(cp, type, typelen);
		cp += typelen;
		*cp++ = ':';
	}
	memcpy(cp, name, namelen);
	cp += namelen;
	*cp = '\0';

	if (!options)
		options = "";
	kdebug("call request_key(,%s,%s)", desc, options);

	saved_cred = override_creds(dns_resolver_cache);
	rkey = request_key(&key_type_dns_resolver, desc, options);
	revert_creds(saved_cred);
	kfree(desc);
	if (IS_ERR(rkey)) {
		ret = PTR_ERR(rkey);
		goto out;
	}

	down_read(&rkey->sem);
	rkey->perm |= KEY_USR_VIEW;

	ret = key_validate(rkey);
	if (ret < 0)
		goto put;

	
	ret = rkey->type_data.x[0];
	if (ret)
		goto put;

	upayload = rcu_dereference_protected(rkey->payload.data,
					     lockdep_is_held(&rkey->sem));
	len = upayload->datalen;

	ret = -ENOMEM;
	*_result = kmalloc(len + 1, GFP_KERNEL);
	if (!*_result)
		goto put;

	memcpy(*_result, upayload->data, len + 1);
	if (_expiry)
		*_expiry = rkey->expiry;

	ret = len;
put:
	up_read(&rkey->sem);
	key_put(rkey);
out:
	kleave(" = %d", ret);
	return ret;
}
EXPORT_SYMBOL(dns_query);
