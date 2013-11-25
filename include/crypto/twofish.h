#ifndef _CRYPTO_TWOFISH_H
#define _CRYPTO_TWOFISH_H

#include <linux/types.h>

#define TF_MIN_KEY_SIZE 16
#define TF_MAX_KEY_SIZE 32
#define TF_BLOCK_SIZE 16

struct crypto_tfm;

struct twofish_ctx {
	u32 s[4][256], w[8], k[32];
};

int __twofish_setkey(struct twofish_ctx *ctx, const u8 *key,
		     unsigned int key_len, u32 *flags);
int twofish_setkey(struct crypto_tfm *tfm, const u8 *key, unsigned int key_len);

#endif
