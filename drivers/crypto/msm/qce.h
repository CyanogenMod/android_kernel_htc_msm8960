/* Qualcomm Crypto Engine driver API
 *
 * Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#ifndef __CRYPTO_MSM_QCE_H
#define __CRYPTO_MSM_QCE_H

#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/crypto.h>

#include <crypto/algapi.h>
#include <crypto/aes.h>
#include <crypto/des.h>
#include <crypto/sha.h>
#include <crypto/aead.h>
#include <crypto/authenc.h>
#include <crypto/scatterwalk.h>

#define SHA256_DIGESTSIZE		32
#define SHA1_DIGESTSIZE			20

#define HMAC_KEY_SIZE			(SHA1_DIGESTSIZE)    
#define SHA_HMAC_KEY_SIZE		64
#define DES_KEY_SIZE			8
#define TRIPLE_DES_KEY_SIZE		24
#define AES128_KEY_SIZE			16
#define AES192_KEY_SIZE			24
#define AES256_KEY_SIZE			32
#define MAX_CIPHER_KEY_SIZE		AES256_KEY_SIZE

#define AES_IV_LENGTH			16
#define DES_IV_LENGTH                   8
#define MAX_IV_LENGTH			AES_IV_LENGTH

#define QCE_MAX_OPER_DATA		0xFF00

#define MAX_NONCE  16

typedef void (*qce_comp_func_ptr_t)(void *areq,
		unsigned char *icv, unsigned char *iv, int ret);

enum qce_cipher_alg_enum {
	CIPHER_ALG_DES = 0,
	CIPHER_ALG_3DES = 1,
	CIPHER_ALG_AES = 2,
	CIPHER_ALG_LAST
};

enum qce_hash_alg_enum {
	QCE_HASH_SHA1   = 0,
	QCE_HASH_SHA256 = 1,
	QCE_HASH_SHA1_HMAC   = 2,
	QCE_HASH_SHA256_HMAC = 3,
	QCE_HASH_AES_CMAC = 4,
	QCE_HASH_LAST
};

enum qce_cipher_dir_enum {
	QCE_ENCRYPT = 0,
	QCE_DECRYPT = 1,
	QCE_CIPHER_DIR_LAST
};

enum qce_cipher_mode_enum {
	QCE_MODE_CBC = 0,
	QCE_MODE_ECB = 1,
	QCE_MODE_CTR = 2,
	QCE_MODE_XTS = 3,
	QCE_MODE_CCM = 4,
	QCE_CIPHER_MODE_LAST
};

enum qce_req_op_enum {
	QCE_REQ_ABLK_CIPHER = 0,
	QCE_REQ_ABLK_CIPHER_NO_KEY = 1,
	QCE_REQ_AEAD = 2,
	QCE_REQ_LAST
};

struct ce_hw_support {
	bool sha1_hmac_20; 
	bool sha1_hmac; 
	bool sha256_hmac; 
	bool sha_hmac; 
	bool cmac;
	bool aes_key_192;
	bool aes_xts;
	bool aes_ccm;
	bool ota;
};

struct qce_sha_req {
	qce_comp_func_ptr_t qce_cb;	
	enum qce_hash_alg_enum alg;	
	unsigned char *digest;		
	struct scatterlist *src;	
	uint32_t  auth_data[4];		
	unsigned char *authkey;		
	unsigned int  authklen;		
	bool first_blk;			
	bool last_blk;			
	unsigned int size;		
	void *areq;
};

struct qce_req {
	enum qce_req_op_enum op;	
	qce_comp_func_ptr_t qce_cb;	
	void *areq;
	enum qce_cipher_alg_enum   alg;	
	enum qce_cipher_dir_enum dir;	
	enum qce_cipher_mode_enum mode;	
	unsigned char *authkey;		
	unsigned int authklen;		
	unsigned int authsize;		
	unsigned char  nonce[MAX_NONCE];
	unsigned char *assoc;		
	unsigned int assoclen;		
	struct scatterlist *asg;	
	unsigned char *enckey;		
	unsigned int encklen;		
	unsigned char *iv;		
	unsigned int ivsize;		
	unsigned int cryptlen;		
	unsigned int use_pmem;		
	struct qcedev_pmem_info *pmem;	
};

void *qce_open(struct platform_device *pdev, int *rc);
int qce_close(void *handle);
int qce_aead_req(void *handle, struct qce_req *req);
int qce_ablk_cipher_req(void *handle, struct qce_req *req);
int qce_hw_support(void *handle, struct ce_hw_support *support);
int qce_process_sha_req(void *handle, struct qce_sha_req *s_req);

#endif 
