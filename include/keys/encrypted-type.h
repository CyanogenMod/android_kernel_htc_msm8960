/*
 * Copyright (C) 2010 IBM Corporation
 * Copyright (C) 2010 Politecnico di Torino, Italy
 *                    TORSEC group -- http://security.polito.it
 *
 * Authors:
 * Mimi Zohar <zohar@us.ibm.com>
 * Roberto Sassu <roberto.sassu@polito.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 */

#ifndef _KEYS_ENCRYPTED_TYPE_H
#define _KEYS_ENCRYPTED_TYPE_H

#include <linux/key.h>
#include <linux/rcupdate.h>

struct encrypted_key_payload {
	struct rcu_head rcu;
	char *format;		
	char *master_desc;	
	char *datalen;		
	u8 *iv;			
	u8 *encrypted_data;	
	unsigned short datablob_len;	
	unsigned short decrypted_datalen;	
	unsigned short payload_datalen;		
	unsigned short encrypted_key_format;	
	u8 *decrypted_data;	
	u8 payload_data[0];	
};

extern struct key_type key_type_encrypted;

#endif 
