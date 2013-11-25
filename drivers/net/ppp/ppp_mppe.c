/*
 * ppp_mppe.c - interface MPPE to the PPP code.
 * This version is for use with Linux kernel 2.6.14+
 *
 * By Frank Cusack <fcusack@fcusack.com>.
 * Copyright (c) 2002,2003,2004 Google, Inc.
 * All rights reserved.
 *
 * License:
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, provided that the above copyright
 * notice appears in all copies.  This software is provided without any
 * warranty, express or implied.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this product
 * may be distributed under the terms of the GNU General Public License (GPL),
 * in which case the provisions of the GPL apply INSTEAD OF those given above.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * Changelog:
 *      08/12/05 - Matt Domsch <Matt_Domsch@dell.com>
 *                 Only need extra skb padding on transmit, not receive.
 *      06/18/04 - Matt Domsch <Matt_Domsch@dell.com>, Oleg Makarenko <mole@quadra.ru>
 *                 Use Linux kernel 2.6 arc4 and sha1 routines rather than
 *                 providing our own.
 *      2/15/04 - TS: added #include <version.h> and testing for Kernel
 *                    version before using
 *                    MOD_DEC_USAGE_COUNT/MOD_INC_USAGE_COUNT which are
 *                    deprecated in 2.6
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/crypto.h>
#include <linux/mm.h>
#include <linux/ppp_defs.h>
#include <linux/ppp-comp.h>
#include <linux/scatterlist.h>
#include <asm/unaligned.h>

#include "ppp_mppe.h"

MODULE_AUTHOR("Frank Cusack <fcusack@fcusack.com>");
MODULE_DESCRIPTION("Point-to-Point Protocol Microsoft Point-to-Point Encryption support");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_ALIAS("ppp-compress-" __stringify(CI_MPPE));
MODULE_VERSION("1.0.2");

static unsigned int
setup_sg(struct scatterlist *sg, const void *address, unsigned int length)
{
	sg_set_buf(sg, address, length);
	return length;
}

#define SHA1_PAD_SIZE 40


struct sha_pad {
	unsigned char sha_pad1[SHA1_PAD_SIZE];
	unsigned char sha_pad2[SHA1_PAD_SIZE];
};
static struct sha_pad *sha_pad;

static inline void sha_pad_init(struct sha_pad *shapad)
{
	memset(shapad->sha_pad1, 0x00, sizeof(shapad->sha_pad1));
	memset(shapad->sha_pad2, 0xF2, sizeof(shapad->sha_pad2));
}

struct ppp_mppe_state {
	struct crypto_blkcipher *arc4;
	struct crypto_hash *sha1;
	unsigned char *sha1_digest;
	unsigned char master_key[MPPE_MAX_KEY_LEN];
	unsigned char session_key[MPPE_MAX_KEY_LEN];
	unsigned keylen;	
	
	
	
	unsigned char bits;	
	unsigned ccount;	
	unsigned stateful;	
	int discard;		
	int sanity_errors;	
	int unit;
	int debug;
	struct compstat stats;
};

#define MPPE_BIT_A	0x80	
#define MPPE_BIT_B	0x40	
#define MPPE_BIT_C	0x20	
#define MPPE_BIT_D	0x10	

#define MPPE_BIT_FLUSHED	MPPE_BIT_A
#define MPPE_BIT_ENCRYPTED	MPPE_BIT_D

#define MPPE_BITS(p) ((p)[4] & 0xf0)
#define MPPE_CCOUNT(p) ((((p)[4] & 0x0f) << 8) + (p)[5])
#define MPPE_CCOUNT_SPACE 0x1000	

#define MPPE_OVHD	2	
#define SANITY_MAX	1600	

static void get_new_key_from_sha(struct ppp_mppe_state * state)
{
	struct hash_desc desc;
	struct scatterlist sg[4];
	unsigned int nbytes;

	sg_init_table(sg, 4);

	nbytes = setup_sg(&sg[0], state->master_key, state->keylen);
	nbytes += setup_sg(&sg[1], sha_pad->sha_pad1,
			   sizeof(sha_pad->sha_pad1));
	nbytes += setup_sg(&sg[2], state->session_key, state->keylen);
	nbytes += setup_sg(&sg[3], sha_pad->sha_pad2,
			   sizeof(sha_pad->sha_pad2));

	desc.tfm = state->sha1;
	desc.flags = 0;

	crypto_hash_digest(&desc, sg, nbytes, state->sha1_digest);
}

/*
 * Perform the MPPE rekey algorithm, from RFC 3078, sec. 7.3.
 * Well, not what's written there, but rather what they meant.
 */
static void mppe_rekey(struct ppp_mppe_state * state, int initial_key)
{
	struct scatterlist sg_in[1], sg_out[1];
	struct blkcipher_desc desc = { .tfm = state->arc4 };

	get_new_key_from_sha(state);
	if (!initial_key) {
		crypto_blkcipher_setkey(state->arc4, state->sha1_digest,
					state->keylen);
		sg_init_table(sg_in, 1);
		sg_init_table(sg_out, 1);
		setup_sg(sg_in, state->sha1_digest, state->keylen);
		setup_sg(sg_out, state->session_key, state->keylen);
		if (crypto_blkcipher_encrypt(&desc, sg_out, sg_in,
					     state->keylen) != 0) {
    		    printk(KERN_WARNING "mppe_rekey: cipher_encrypt failed\n");
		}
	} else {
		memcpy(state->session_key, state->sha1_digest, state->keylen);
	}
	if (state->keylen == 8) {
		
		state->session_key[0] = 0xd1;
		state->session_key[1] = 0x26;
		state->session_key[2] = 0x9e;
	}
	crypto_blkcipher_setkey(state->arc4, state->session_key, state->keylen);
}

static void *mppe_alloc(unsigned char *options, int optlen)
{
	struct ppp_mppe_state *state;
	unsigned int digestsize;

	if (optlen != CILEN_MPPE + sizeof(state->master_key) ||
	    options[0] != CI_MPPE || options[1] != CILEN_MPPE)
		goto out;

	state = kzalloc(sizeof(*state), GFP_KERNEL);
	if (state == NULL)
		goto out;


	state->arc4 = crypto_alloc_blkcipher("ecb(arc4)", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(state->arc4)) {
		state->arc4 = NULL;
		goto out_free;
	}

	state->sha1 = crypto_alloc_hash("sha1", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(state->sha1)) {
		state->sha1 = NULL;
		goto out_free;
	}

	digestsize = crypto_hash_digestsize(state->sha1);
	if (digestsize < MPPE_MAX_KEY_LEN)
		goto out_free;

	state->sha1_digest = kmalloc(digestsize, GFP_KERNEL);
	if (!state->sha1_digest)
		goto out_free;

	
	memcpy(state->master_key, &options[CILEN_MPPE],
	       sizeof(state->master_key));
	memcpy(state->session_key, state->master_key,
	       sizeof(state->master_key));


	return (void *)state;

	out_free:
	    if (state->sha1_digest)
		kfree(state->sha1_digest);
	    if (state->sha1)
		crypto_free_hash(state->sha1);
	    if (state->arc4)
		crypto_free_blkcipher(state->arc4);
	    kfree(state);
	out:
	return NULL;
}

static void mppe_free(void *arg)
{
	struct ppp_mppe_state *state = (struct ppp_mppe_state *) arg;
	if (state) {
	    if (state->sha1_digest)
		kfree(state->sha1_digest);
	    if (state->sha1)
		crypto_free_hash(state->sha1);
	    if (state->arc4)
		crypto_free_blkcipher(state->arc4);
	    kfree(state);
	}
}

static int
mppe_init(void *arg, unsigned char *options, int optlen, int unit, int debug,
	  const char *debugstr)
{
	struct ppp_mppe_state *state = (struct ppp_mppe_state *) arg;
	unsigned char mppe_opts;

	if (optlen != CILEN_MPPE ||
	    options[0] != CI_MPPE || options[1] != CILEN_MPPE)
		return 0;

	MPPE_CI_TO_OPTS(&options[2], mppe_opts);
	if (mppe_opts & MPPE_OPT_128)
		state->keylen = 16;
	else if (mppe_opts & MPPE_OPT_40)
		state->keylen = 8;
	else {
		printk(KERN_WARNING "%s[%d]: unknown key length\n", debugstr,
		       unit);
		return 0;
	}
	if (mppe_opts & MPPE_OPT_STATEFUL)
		state->stateful = 1;

	
	mppe_rekey(state, 1);

	if (debug) {
		int i;
		char mkey[sizeof(state->master_key) * 2 + 1];
		char skey[sizeof(state->session_key) * 2 + 1];

		printk(KERN_DEBUG "%s[%d]: initialized with %d-bit %s mode\n",
		       debugstr, unit, (state->keylen == 16) ? 128 : 40,
		       (state->stateful) ? "stateful" : "stateless");

		for (i = 0; i < sizeof(state->master_key); i++)
			sprintf(mkey + i * 2, "%02x", state->master_key[i]);
		for (i = 0; i < sizeof(state->session_key); i++)
			sprintf(skey + i * 2, "%02x", state->session_key[i]);
		printk(KERN_DEBUG
		       "%s[%d]: keys: master: %s initial session: %s\n",
		       debugstr, unit, mkey, skey);
	}

	state->ccount = MPPE_CCOUNT_SPACE - 1;

	state->bits = MPPE_BIT_ENCRYPTED;

	state->unit = unit;
	state->debug = debug;

	return 1;
}

static int
mppe_comp_init(void *arg, unsigned char *options, int optlen, int unit,
	       int hdrlen, int debug)
{
	
	return mppe_init(arg, options, optlen, unit, debug, "mppe_comp_init");
}

static void mppe_comp_reset(void *arg)
{
	struct ppp_mppe_state *state = (struct ppp_mppe_state *) arg;

	state->bits |= MPPE_BIT_FLUSHED;
}

static int
mppe_compress(void *arg, unsigned char *ibuf, unsigned char *obuf,
	      int isize, int osize)
{
	struct ppp_mppe_state *state = (struct ppp_mppe_state *) arg;
	struct blkcipher_desc desc = { .tfm = state->arc4 };
	int proto;
	struct scatterlist sg_in[1], sg_out[1];

	proto = PPP_PROTOCOL(ibuf);
	if (proto < 0x0021 || proto > 0x00fa)
		return 0;

	
	if (osize < isize + MPPE_OVHD + 2) {
		
		printk(KERN_DEBUG "mppe_compress[%d]: osize too small! "
		       "(have: %d need: %d)\n", state->unit,
		       osize, osize + MPPE_OVHD + 2);
		return -1;
	}

	osize = isize + MPPE_OVHD + 2;

	obuf[0] = PPP_ADDRESS(ibuf);
	obuf[1] = PPP_CONTROL(ibuf);
	put_unaligned_be16(PPP_COMP, obuf + 2);
	obuf += PPP_HDRLEN;

	state->ccount = (state->ccount + 1) % MPPE_CCOUNT_SPACE;
	if (state->debug >= 7)
		printk(KERN_DEBUG "mppe_compress[%d]: ccount %d\n", state->unit,
		       state->ccount);
	put_unaligned_be16(state->ccount, obuf);

	if (!state->stateful ||	
	    ((state->ccount & 0xff) == 0xff) ||	
	    (state->bits & MPPE_BIT_FLUSHED)) {	
		
		if (state->debug && state->stateful)
			printk(KERN_DEBUG "mppe_compress[%d]: rekeying\n",
			       state->unit);
		mppe_rekey(state, 0);
		state->bits |= MPPE_BIT_FLUSHED;
	}
	obuf[0] |= state->bits;
	state->bits &= ~MPPE_BIT_FLUSHED;	

	obuf += MPPE_OVHD;
	ibuf += 2;		
	isize -= 2;

	
	sg_init_table(sg_in, 1);
	sg_init_table(sg_out, 1);
	setup_sg(sg_in, ibuf, isize);
	setup_sg(sg_out, obuf, osize);
	if (crypto_blkcipher_encrypt(&desc, sg_out, sg_in, isize) != 0) {
		printk(KERN_DEBUG "crypto_cypher_encrypt failed\n");
		return -1;
	}

	state->stats.unc_bytes += isize;
	state->stats.unc_packets++;
	state->stats.comp_bytes += osize;
	state->stats.comp_packets++;

	return osize;
}

static void mppe_comp_stats(void *arg, struct compstat *stats)
{
	struct ppp_mppe_state *state = (struct ppp_mppe_state *) arg;

	*stats = state->stats;
}

static int
mppe_decomp_init(void *arg, unsigned char *options, int optlen, int unit,
		 int hdrlen, int mru, int debug)
{
	
	return mppe_init(arg, options, optlen, unit, debug, "mppe_decomp_init");
}

static void mppe_decomp_reset(void *arg)
{
	
	return;
}

static int
mppe_decompress(void *arg, unsigned char *ibuf, int isize, unsigned char *obuf,
		int osize)
{
	struct ppp_mppe_state *state = (struct ppp_mppe_state *) arg;
	struct blkcipher_desc desc = { .tfm = state->arc4 };
	unsigned ccount;
	int flushed = MPPE_BITS(ibuf) & MPPE_BIT_FLUSHED;
	int sanity = 0;
	struct scatterlist sg_in[1], sg_out[1];

	if (isize <= PPP_HDRLEN + MPPE_OVHD) {
		if (state->debug)
			printk(KERN_DEBUG
			       "mppe_decompress[%d]: short pkt (%d)\n",
			       state->unit, isize);
		return DECOMP_ERROR;
	}

	if (osize < isize - MPPE_OVHD - 1) {
		printk(KERN_DEBUG "mppe_decompress[%d]: osize too small! "
		       "(have: %d need: %d)\n", state->unit,
		       osize, isize - MPPE_OVHD - 1);
		return DECOMP_ERROR;
	}
	osize = isize - MPPE_OVHD - 2;	

	ccount = MPPE_CCOUNT(ibuf);
	if (state->debug >= 7)
		printk(KERN_DEBUG "mppe_decompress[%d]: ccount %d\n",
		       state->unit, ccount);

	
	if (!(MPPE_BITS(ibuf) & MPPE_BIT_ENCRYPTED)) {
		printk(KERN_DEBUG
		       "mppe_decompress[%d]: ENCRYPTED bit not set!\n",
		       state->unit);
		state->sanity_errors += 100;
		sanity = 1;
	}
	if (!state->stateful && !flushed) {
		printk(KERN_DEBUG "mppe_decompress[%d]: FLUSHED bit not set in "
		       "stateless mode!\n", state->unit);
		state->sanity_errors += 100;
		sanity = 1;
	}
	if (state->stateful && ((ccount & 0xff) == 0xff) && !flushed) {
		printk(KERN_DEBUG "mppe_decompress[%d]: FLUSHED bit not set on "
		       "flag packet!\n", state->unit);
		state->sanity_errors += 100;
		sanity = 1;
	}

	if (sanity) {
		if (state->sanity_errors < SANITY_MAX)
			return DECOMP_ERROR;
		else
			return DECOMP_FATALERROR;
	}


	if (!state->stateful) {
		
		while (state->ccount != ccount) {
			mppe_rekey(state, 0);
			state->ccount = (state->ccount + 1) % MPPE_CCOUNT_SPACE;
		}
	} else {
		
		if (!state->discard) {
			
			state->ccount = (state->ccount + 1) % MPPE_CCOUNT_SPACE;
			if (ccount != state->ccount) {
				state->discard = 1;
				return DECOMP_ERROR;
			}
		} else {
			
			if (!flushed) {
				
				return DECOMP_ERROR;
			} else {
				
				while ((ccount & ~0xff) !=
				       (state->ccount & ~0xff)) {
					mppe_rekey(state, 0);
					state->ccount =
					    (state->ccount +
					     256) % MPPE_CCOUNT_SPACE;
				}

				
				state->discard = 0;
				state->ccount = ccount;
			}
		}
		if (flushed)
			mppe_rekey(state, 0);
	}

	obuf[0] = PPP_ADDRESS(ibuf);	
	obuf[1] = PPP_CONTROL(ibuf);	
	obuf += 2;
	ibuf += PPP_HDRLEN + MPPE_OVHD;
	isize -= PPP_HDRLEN + MPPE_OVHD;	
	

	sg_init_table(sg_in, 1);
	sg_init_table(sg_out, 1);
	setup_sg(sg_in, ibuf, 1);
	setup_sg(sg_out, obuf, 1);
	if (crypto_blkcipher_decrypt(&desc, sg_out, sg_in, 1) != 0) {
		printk(KERN_DEBUG "crypto_cypher_decrypt failed\n");
		return DECOMP_ERROR;
	}

	if ((obuf[0] & 0x01) != 0) {
		obuf[1] = obuf[0];
		obuf[0] = 0;
		obuf++;
		osize++;
	}

	
	setup_sg(sg_in, ibuf + 1, isize - 1);
	setup_sg(sg_out, obuf + 1, osize - 1);
	if (crypto_blkcipher_decrypt(&desc, sg_out, sg_in, isize - 1)) {
		printk(KERN_DEBUG "crypto_cypher_decrypt failed\n");
		return DECOMP_ERROR;
	}

	state->stats.unc_bytes += osize;
	state->stats.unc_packets++;
	state->stats.comp_bytes += isize;
	state->stats.comp_packets++;

	
	state->sanity_errors >>= 1;

	return osize;
}

static void mppe_incomp(void *arg, unsigned char *ibuf, int icnt)
{
	struct ppp_mppe_state *state = (struct ppp_mppe_state *) arg;

	if (state->debug &&
	    (PPP_PROTOCOL(ibuf) >= 0x0021 && PPP_PROTOCOL(ibuf) <= 0x00fa))
		printk(KERN_DEBUG
		       "mppe_incomp[%d]: incompressible (unencrypted) data! "
		       "(proto %04x)\n", state->unit, PPP_PROTOCOL(ibuf));

	state->stats.inc_bytes += icnt;
	state->stats.inc_packets++;
	state->stats.unc_bytes += icnt;
	state->stats.unc_packets++;
}


static struct compressor ppp_mppe = {
	.compress_proto = CI_MPPE,
	.comp_alloc     = mppe_alloc,
	.comp_free      = mppe_free,
	.comp_init      = mppe_comp_init,
	.comp_reset     = mppe_comp_reset,
	.compress       = mppe_compress,
	.comp_stat      = mppe_comp_stats,
	.decomp_alloc   = mppe_alloc,
	.decomp_free    = mppe_free,
	.decomp_init    = mppe_decomp_init,
	.decomp_reset   = mppe_decomp_reset,
	.decompress     = mppe_decompress,
	.incomp         = mppe_incomp,
	.decomp_stat    = mppe_comp_stats,
	.owner          = THIS_MODULE,
	.comp_extra     = MPPE_PAD,
};


static int __init ppp_mppe_init(void)
{
	int answer;
	if (!(crypto_has_blkcipher("ecb(arc4)", 0, CRYPTO_ALG_ASYNC) &&
	      crypto_has_hash("sha1", 0, CRYPTO_ALG_ASYNC)))
		return -ENODEV;

	sha_pad = kmalloc(sizeof(struct sha_pad), GFP_KERNEL);
	if (!sha_pad)
		return -ENOMEM;
	sha_pad_init(sha_pad);

	answer = ppp_register_compressor(&ppp_mppe);

	if (answer == 0)
		printk(KERN_INFO "PPP MPPE Compression module registered\n");
	else
		kfree(sha_pad);

	return answer;
}

static void __exit ppp_mppe_cleanup(void)
{
	ppp_unregister_compressor(&ppp_mppe);
	kfree(sha_pad);
}

module_init(ppp_mppe_init);
module_exit(ppp_mppe_cleanup);
