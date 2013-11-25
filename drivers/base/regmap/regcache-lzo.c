/*
 * Register cache access API - LZO caching support
 *
 * Copyright 2011 Wolfson Microelectronics plc
 *
 * Author: Dimitris Papastamos <dp@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/device.h>
#include <linux/lzo.h>

#include "internal.h"

static int regcache_lzo_exit(struct regmap *map);

struct regcache_lzo_ctx {
	void *wmem;
	void *dst;
	const void *src;
	size_t src_len;
	size_t dst_len;
	size_t decompressed_size;
	unsigned long *sync_bmp;
	int sync_bmp_nbits;
};

#define LZO_BLOCK_NUM 8
static int regcache_lzo_block_count(struct regmap *map)
{
	return LZO_BLOCK_NUM;
}

static int regcache_lzo_prepare(struct regcache_lzo_ctx *lzo_ctx)
{
	lzo_ctx->wmem = kmalloc(LZO1X_MEM_COMPRESS, GFP_KERNEL);
	if (!lzo_ctx->wmem)
		return -ENOMEM;
	return 0;
}

static int regcache_lzo_compress(struct regcache_lzo_ctx *lzo_ctx)
{
	size_t compress_size;
	int ret;

	ret = lzo1x_1_compress(lzo_ctx->src, lzo_ctx->src_len,
			       lzo_ctx->dst, &compress_size, lzo_ctx->wmem);
	if (ret != LZO_E_OK || compress_size > lzo_ctx->dst_len)
		return -EINVAL;
	lzo_ctx->dst_len = compress_size;
	return 0;
}

static int regcache_lzo_decompress(struct regcache_lzo_ctx *lzo_ctx)
{
	size_t dst_len;
	int ret;

	dst_len = lzo_ctx->dst_len;
	ret = lzo1x_decompress_safe(lzo_ctx->src, lzo_ctx->src_len,
				    lzo_ctx->dst, &dst_len);
	if (ret != LZO_E_OK || dst_len != lzo_ctx->dst_len)
		return -EINVAL;
	return 0;
}

static int regcache_lzo_compress_cache_block(struct regmap *map,
		struct regcache_lzo_ctx *lzo_ctx)
{
	int ret;

	lzo_ctx->dst_len = lzo1x_worst_compress(PAGE_SIZE);
	lzo_ctx->dst = kmalloc(lzo_ctx->dst_len, GFP_KERNEL);
	if (!lzo_ctx->dst) {
		lzo_ctx->dst_len = 0;
		return -ENOMEM;
	}

	ret = regcache_lzo_compress(lzo_ctx);
	if (ret < 0)
		return ret;
	return 0;
}

static int regcache_lzo_decompress_cache_block(struct regmap *map,
		struct regcache_lzo_ctx *lzo_ctx)
{
	int ret;

	lzo_ctx->dst_len = lzo_ctx->decompressed_size;
	lzo_ctx->dst = kmalloc(lzo_ctx->dst_len, GFP_KERNEL);
	if (!lzo_ctx->dst) {
		lzo_ctx->dst_len = 0;
		return -ENOMEM;
	}

	ret = regcache_lzo_decompress(lzo_ctx);
	if (ret < 0)
		return ret;
	return 0;
}

static inline int regcache_lzo_get_blkindex(struct regmap *map,
					    unsigned int reg)
{
	return (reg * map->cache_word_size) /
		DIV_ROUND_UP(map->cache_size_raw,
			     regcache_lzo_block_count(map));
}

static inline int regcache_lzo_get_blkpos(struct regmap *map,
					  unsigned int reg)
{
	return reg % (DIV_ROUND_UP(map->cache_size_raw,
				   regcache_lzo_block_count(map)) /
		      map->cache_word_size);
}

static inline int regcache_lzo_get_blksize(struct regmap *map)
{
	return DIV_ROUND_UP(map->cache_size_raw,
			    regcache_lzo_block_count(map));
}

static int regcache_lzo_init(struct regmap *map)
{
	struct regcache_lzo_ctx **lzo_blocks;
	size_t bmp_size;
	int ret, i, blksize, blkcount;
	const char *p, *end;
	unsigned long *sync_bmp;

	ret = 0;

	blkcount = regcache_lzo_block_count(map);
	map->cache = kzalloc(blkcount * sizeof *lzo_blocks,
			     GFP_KERNEL);
	if (!map->cache)
		return -ENOMEM;
	lzo_blocks = map->cache;

	bmp_size = map->num_reg_defaults_raw;
	sync_bmp = kmalloc(BITS_TO_LONGS(bmp_size) * sizeof(long),
			   GFP_KERNEL);
	if (!sync_bmp) {
		ret = -ENOMEM;
		goto err;
	}
	bitmap_zero(sync_bmp, bmp_size);

	
	for (i = 0; i < blkcount; i++) {
		lzo_blocks[i] = kzalloc(sizeof **lzo_blocks,
					GFP_KERNEL);
		if (!lzo_blocks[i]) {
			kfree(sync_bmp);
			ret = -ENOMEM;
			goto err;
		}
		lzo_blocks[i]->sync_bmp = sync_bmp;
		lzo_blocks[i]->sync_bmp_nbits = bmp_size;
		
		ret = regcache_lzo_prepare(lzo_blocks[i]);
		if (ret < 0)
			goto err;
	}

	blksize = regcache_lzo_get_blksize(map);
	p = map->reg_defaults_raw;
	end = map->reg_defaults_raw + map->cache_size_raw;
	
	for (i = 0; i < blkcount; i++, p += blksize) {
		lzo_blocks[i]->src = p;
		if (p + blksize > end)
			lzo_blocks[i]->src_len = end - p;
		else
			lzo_blocks[i]->src_len = blksize;
		ret = regcache_lzo_compress_cache_block(map,
						       lzo_blocks[i]);
		if (ret < 0)
			goto err;
		lzo_blocks[i]->decompressed_size =
			lzo_blocks[i]->src_len;
	}

	return 0;
err:
	regcache_lzo_exit(map);
	return ret;
}

static int regcache_lzo_exit(struct regmap *map)
{
	struct regcache_lzo_ctx **lzo_blocks;
	int i, blkcount;

	lzo_blocks = map->cache;
	if (!lzo_blocks)
		return 0;

	blkcount = regcache_lzo_block_count(map);
	if (lzo_blocks[0])
		kfree(lzo_blocks[0]->sync_bmp);
	for (i = 0; i < blkcount; i++) {
		if (lzo_blocks[i]) {
			kfree(lzo_blocks[i]->wmem);
			kfree(lzo_blocks[i]->dst);
		}
		
		kfree(lzo_blocks[i]);
	}
	kfree(lzo_blocks);
	map->cache = NULL;
	return 0;
}

static int regcache_lzo_read(struct regmap *map,
			     unsigned int reg, unsigned int *value)
{
	struct regcache_lzo_ctx *lzo_block, **lzo_blocks;
	int ret, blkindex, blkpos;
	size_t blksize, tmp_dst_len;
	void *tmp_dst;

	
	blkindex = regcache_lzo_get_blkindex(map, reg);
	
	blkpos = regcache_lzo_get_blkpos(map, reg);
	
	blksize = regcache_lzo_get_blksize(map);
	lzo_blocks = map->cache;
	lzo_block = lzo_blocks[blkindex];

	
	tmp_dst = lzo_block->dst;
	tmp_dst_len = lzo_block->dst_len;

	
	lzo_block->src = lzo_block->dst;
	lzo_block->src_len = lzo_block->dst_len;

	
	ret = regcache_lzo_decompress_cache_block(map, lzo_block);
	if (ret >= 0)
		
		*value = regcache_get_val(lzo_block->dst, blkpos,
					  map->cache_word_size);

	kfree(lzo_block->dst);
	
	lzo_block->dst = tmp_dst;
	lzo_block->dst_len = tmp_dst_len;

	return ret;
}

static int regcache_lzo_write(struct regmap *map,
			      unsigned int reg, unsigned int value)
{
	struct regcache_lzo_ctx *lzo_block, **lzo_blocks;
	int ret, blkindex, blkpos;
	size_t blksize, tmp_dst_len;
	void *tmp_dst;

	
	blkindex = regcache_lzo_get_blkindex(map, reg);
	
	blkpos = regcache_lzo_get_blkpos(map, reg);
	
	blksize = regcache_lzo_get_blksize(map);
	lzo_blocks = map->cache;
	lzo_block = lzo_blocks[blkindex];

	
	tmp_dst = lzo_block->dst;
	tmp_dst_len = lzo_block->dst_len;

	
	lzo_block->src = lzo_block->dst;
	lzo_block->src_len = lzo_block->dst_len;

	
	ret = regcache_lzo_decompress_cache_block(map, lzo_block);
	if (ret < 0) {
		kfree(lzo_block->dst);
		goto out;
	}

	
	if (regcache_set_val(lzo_block->dst, blkpos, value,
			     map->cache_word_size)) {
		kfree(lzo_block->dst);
		goto out;
	}

	
	lzo_block->src = lzo_block->dst;
	lzo_block->src_len = lzo_block->dst_len;

	
	ret = regcache_lzo_compress_cache_block(map, lzo_block);
	if (ret < 0) {
		kfree(lzo_block->dst);
		kfree(lzo_block->src);
		goto out;
	}

	
	set_bit(reg, lzo_block->sync_bmp);
	kfree(tmp_dst);
	kfree(lzo_block->src);
	return 0;
out:
	lzo_block->dst = tmp_dst;
	lzo_block->dst_len = tmp_dst_len;
	return ret;
}

static int regcache_lzo_sync(struct regmap *map, unsigned int min,
			     unsigned int max)
{
	struct regcache_lzo_ctx **lzo_blocks;
	unsigned int val;
	int i;
	int ret;

	lzo_blocks = map->cache;
	i = min;
	for_each_set_bit_from(i, lzo_blocks[0]->sync_bmp,
			      lzo_blocks[0]->sync_bmp_nbits) {
		if (i > max)
			continue;

		ret = regcache_read(map, i, &val);
		if (ret)
			return ret;

		
		ret = regcache_lookup_reg(map, i);
		if (ret > 0 && val == map->reg_defaults[ret].def)
			continue;

		map->cache_bypass = 1;
		ret = _regmap_write(map, i, val);
		map->cache_bypass = 0;
		if (ret)
			return ret;
		dev_dbg(map->dev, "Synced register %#x, value %#x\n",
			i, val);
	}

	return 0;
}

struct regcache_ops regcache_lzo_ops = {
	.type = REGCACHE_COMPRESSED,
	.name = "lzo",
	.init = regcache_lzo_init,
	.exit = regcache_lzo_exit,
	.read = regcache_lzo_read,
	.write = regcache_lzo_write,
	.sync = regcache_lzo_sync
};
