/*
 *  linux/drivers/mmc/core/sdio_io.c
 *
 *  Copyright 2007-2008 Pierre Ossman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#include <linux/export.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>

#include "sdio_ops.h"

void sdio_claim_host(struct sdio_func *func)
{
	BUG_ON(!func);
	BUG_ON(!func->card);

	mmc_claim_host(func->card->host);
}
EXPORT_SYMBOL_GPL(sdio_claim_host);

void sdio_release_host(struct sdio_func *func)
{
	BUG_ON(!func);
	BUG_ON(!func->card);

	mmc_release_host(func->card->host);
}
EXPORT_SYMBOL_GPL(sdio_release_host);

int sdio_enable_func(struct sdio_func *func)
{
	int ret;
	unsigned char reg;
	unsigned long timeout;

	BUG_ON(!func);
	BUG_ON(!func->card);

	pr_debug("SDIO: Enabling device %s...\n", sdio_func_id(func));

	ret = mmc_io_rw_direct(func->card, 0, 0, SDIO_CCCR_IOEx, 0, &reg);
	if (ret)
		goto err;

	reg |= 1 << func->num;

	ret = mmc_io_rw_direct(func->card, 1, 0, SDIO_CCCR_IOEx, reg, NULL);
	if (ret)
		goto err;

	timeout = jiffies + msecs_to_jiffies(func->enable_timeout);

	while (1) {
		ret = mmc_io_rw_direct(func->card, 0, 0, SDIO_CCCR_IORx, 0, &reg);
		if (ret)
			goto err;
		if (reg & (1 << func->num))
			break;
		ret = -ETIME;
		if (time_after(jiffies, timeout))
			goto err;
	}

	pr_debug("SDIO: Enabled device %s\n", sdio_func_id(func));

	return 0;

err:
	pr_debug("SDIO: Failed to enable device %s\n", sdio_func_id(func));
	return ret;
}
EXPORT_SYMBOL_GPL(sdio_enable_func);

int sdio_disable_func(struct sdio_func *func)
{
	int ret;
	unsigned char reg;

	BUG_ON(!func);
	BUG_ON(!func->card);

	pr_debug("SDIO: Disabling device %s...\n", sdio_func_id(func));

	ret = mmc_io_rw_direct(func->card, 0, 0, SDIO_CCCR_IOEx, 0, &reg);
	if (ret)
		goto err;

	reg &= ~(1 << func->num);

	ret = mmc_io_rw_direct(func->card, 1, 0, SDIO_CCCR_IOEx, reg, NULL);
	if (ret)
		goto err;

	pr_debug("SDIO: Disabled device %s\n", sdio_func_id(func));

	return 0;

err:
	pr_debug("SDIO: Failed to disable device %s\n", sdio_func_id(func));
	return -EIO;
}
EXPORT_SYMBOL_GPL(sdio_disable_func);

int sdio_set_block_size(struct sdio_func *func, unsigned blksz)
{
	int ret;

	if (blksz > func->card->host->max_blk_size)
		return -EINVAL;

	if (blksz == 0) {
		blksz = min(func->max_blksize, func->card->host->max_blk_size);
		blksz = min(blksz, 512u);
	}

	ret = mmc_io_rw_direct(func->card, 1, 0,
		SDIO_FBR_BASE(func->num) + SDIO_FBR_BLKSIZE,
		blksz & 0xff, NULL);
	if (ret)
		return ret;
	ret = mmc_io_rw_direct(func->card, 1, 0,
		SDIO_FBR_BASE(func->num) + SDIO_FBR_BLKSIZE + 1,
		(blksz >> 8) & 0xff, NULL);
	if (ret)
		return ret;
	func->cur_blksize = blksz;
	return 0;
}
EXPORT_SYMBOL_GPL(sdio_set_block_size);

static inline unsigned int sdio_max_byte_size(struct sdio_func *func)
{
	unsigned mval =	min(func->card->host->max_seg_size,
			    func->card->host->max_blk_size);

	if (mmc_blksz_for_byte_mode(func->card))
		mval = min(mval, func->cur_blksize);
	else
		mval = min(mval, func->max_blksize);

	if (mmc_card_broken_byte_mode_512(func->card))
		return min(mval, 511u);

	return min(mval, 512u); 
}

unsigned int sdio_align_size(struct sdio_func *func, unsigned int sz)
{
	unsigned int orig_sz;
	unsigned int blk_sz, byte_sz;
	unsigned chunk_sz;

	orig_sz = sz;

	sz = mmc_align_data_size(func->card, sz);

	if (sz <= sdio_max_byte_size(func))
		return sz;

	if (func->card->cccr.multi_block) {
		if ((sz % func->cur_blksize) == 0)
			return sz;

		blk_sz = ((sz + func->cur_blksize - 1) /
			func->cur_blksize) * func->cur_blksize;
		blk_sz = mmc_align_data_size(func->card, blk_sz);

		if ((blk_sz % func->cur_blksize) == 0)
			return blk_sz;

		byte_sz = mmc_align_data_size(func->card,
				sz % func->cur_blksize);
		if (byte_sz <= sdio_max_byte_size(func)) {
			blk_sz = sz / func->cur_blksize;
			return blk_sz * func->cur_blksize + byte_sz;
		}
	} else {
		chunk_sz = mmc_align_data_size(func->card,
				sdio_max_byte_size(func));
		if (chunk_sz == sdio_max_byte_size(func)) {
			byte_sz = orig_sz % chunk_sz;
			if (byte_sz) {
				byte_sz = mmc_align_data_size(func->card,
						byte_sz);
			}

			return (orig_sz / chunk_sz) * chunk_sz + byte_sz;
		}
	}

	return orig_sz;
}
EXPORT_SYMBOL_GPL(sdio_align_size);

static int sdio_io_rw_ext_helper(struct sdio_func *func, int write,
	unsigned addr, int incr_addr, u8 *buf, unsigned size)
{
	unsigned remainder = size;
	unsigned max_blocks;
	int ret;

	
	if (func->card->cccr.multi_block && (size > sdio_max_byte_size(func))) {
		max_blocks = min(func->card->host->max_blk_count,
			func->card->host->max_seg_size / func->cur_blksize);
		max_blocks = min(max_blocks, 511u);

		while (remainder >= func->cur_blksize) {
			unsigned blocks;

			blocks = remainder / func->cur_blksize;
			if (blocks > max_blocks)
				blocks = max_blocks;
			size = blocks * func->cur_blksize;

			ret = mmc_io_rw_extended(func->card, write,
				func->num, addr, incr_addr, buf,
				blocks, func->cur_blksize);
			if (ret)
				return ret;

			remainder -= size;
			buf += size;
			if (incr_addr)
				addr += size;
		}
	}

	
	while (remainder > 0) {
		size = min(remainder, sdio_max_byte_size(func));

		
		ret = mmc_io_rw_extended(func->card, write, func->num, addr,
			 incr_addr, buf, 0, size);
		if (ret)
			return ret;

		remainder -= size;
		buf += size;
		if (incr_addr)
			addr += size;
	}
	return 0;
}

u8 sdio_readb(struct sdio_func *func, unsigned int addr, int *err_ret)
{
	int ret;
	u8 val;

	BUG_ON(!func);

	if (err_ret)
		*err_ret = 0;

	ret = mmc_io_rw_direct(func->card, 0, func->num, addr, 0, &val);
	if (ret) {
		if (err_ret)
			*err_ret = ret;
		return 0xFF;
	}

	return val;
}
EXPORT_SYMBOL_GPL(sdio_readb);

unsigned char sdio_readb_ext(struct sdio_func *func, unsigned int addr,
	int *err_ret, unsigned in)
{
	int ret;
	unsigned char val;

	BUG_ON(!func);

	if (err_ret)
		*err_ret = 0;

	ret = mmc_io_rw_direct(func->card, 0, func->num, addr, (u8)in, &val);
	if (ret) {
		if (err_ret)
			*err_ret = ret;
		return 0xFF;
	}

	return val;
}
EXPORT_SYMBOL_GPL(sdio_readb_ext);

void sdio_writeb(struct sdio_func *func, u8 b, unsigned int addr, int *err_ret)
{
	int ret;

	BUG_ON(!func);

	ret = mmc_io_rw_direct(func->card, 1, func->num, addr, b, NULL);
	if (err_ret)
		*err_ret = ret;
}
EXPORT_SYMBOL_GPL(sdio_writeb);

/**
 *	sdio_writeb_readb - write and read a byte from SDIO function
 *	@func: SDIO function to access
 *	@write_byte: byte to write
 *	@addr: address to write to
 *	@err_ret: optional status value from transfer
 *
 *	Performs a RAW (Read after Write) operation as defined by SDIO spec -
 *	single byte is written to address space of a given SDIO function and
 *	response is read back from the same address, both using single request.
 *	If there is a problem with the operation, 0xff is returned and
 *	@err_ret will contain the error code.
 */
u8 sdio_writeb_readb(struct sdio_func *func, u8 write_byte,
	unsigned int addr, int *err_ret)
{
	int ret;
	u8 val;

	ret = mmc_io_rw_direct(func->card, 1, func->num, addr,
			write_byte, &val);
	if (err_ret)
		*err_ret = ret;
	if (ret)
		val = 0xff;

	return val;
}
EXPORT_SYMBOL_GPL(sdio_writeb_readb);

int sdio_memcpy_fromio(struct sdio_func *func, void *dst,
	unsigned int addr, int count)
{
	return sdio_io_rw_ext_helper(func, 0, addr, 1, dst, count);
}
EXPORT_SYMBOL_GPL(sdio_memcpy_fromio);

int sdio_memcpy_toio(struct sdio_func *func, unsigned int addr,
	void *src, int count)
{
	return sdio_io_rw_ext_helper(func, 1, addr, 1, src, count);
}
EXPORT_SYMBOL_GPL(sdio_memcpy_toio);

int sdio_readsb(struct sdio_func *func, void *dst, unsigned int addr,
	int count)
{
	return sdio_io_rw_ext_helper(func, 0, addr, 0, dst, count);
}
EXPORT_SYMBOL_GPL(sdio_readsb);

int sdio_writesb(struct sdio_func *func, unsigned int addr, void *src,
	int count)
{
	return sdio_io_rw_ext_helper(func, 1, addr, 0, src, count);
}
EXPORT_SYMBOL_GPL(sdio_writesb);

u16 sdio_readw(struct sdio_func *func, unsigned int addr, int *err_ret)
{
	int ret;

	if (err_ret)
		*err_ret = 0;

	ret = sdio_memcpy_fromio(func, func->tmpbuf, addr, 2);
	if (ret) {
		if (err_ret)
			*err_ret = ret;
		return 0xFFFF;
	}

	return le16_to_cpup((__le16 *)func->tmpbuf);
}
EXPORT_SYMBOL_GPL(sdio_readw);

void sdio_writew(struct sdio_func *func, u16 b, unsigned int addr, int *err_ret)
{
	int ret;

	*(__le16 *)func->tmpbuf = cpu_to_le16(b);

	ret = sdio_memcpy_toio(func, addr, func->tmpbuf, 2);
	if (err_ret)
		*err_ret = ret;
}
EXPORT_SYMBOL_GPL(sdio_writew);

u32 sdio_readl(struct sdio_func *func, unsigned int addr, int *err_ret)
{
	int ret;

	if (err_ret)
		*err_ret = 0;

	ret = sdio_memcpy_fromio(func, func->tmpbuf, addr, 4);
	if (ret) {
		if (err_ret)
			*err_ret = ret;
		return 0xFFFFFFFF;
	}

	return le32_to_cpup((__le32 *)func->tmpbuf);
}
EXPORT_SYMBOL_GPL(sdio_readl);

void sdio_writel(struct sdio_func *func, u32 b, unsigned int addr, int *err_ret)
{
	int ret;

	*(__le32 *)func->tmpbuf = cpu_to_le32(b);

	ret = sdio_memcpy_toio(func, addr, func->tmpbuf, 4);
	if (err_ret)
		*err_ret = ret;
}
EXPORT_SYMBOL_GPL(sdio_writel);

unsigned char sdio_f0_readb(struct sdio_func *func, unsigned int addr,
	int *err_ret)
{
	int ret;
	unsigned char val;

	BUG_ON(!func);

	if (err_ret)
		*err_ret = 0;

	ret = mmc_io_rw_direct(func->card, 0, 0, addr, 0, &val);
	if (ret) {
		if (err_ret)
			*err_ret = ret;
		return 0xFF;
	}

	return val;
}
EXPORT_SYMBOL_GPL(sdio_f0_readb);

void sdio_f0_writeb(struct sdio_func *func, unsigned char b, unsigned int addr,
	int *err_ret)
{
	int ret;

	BUG_ON(!func);

	if ((addr < 0xF0 || addr > 0xFF) && (!mmc_card_lenient_fn0(func->card))) {
		if (err_ret)
			*err_ret = -EINVAL;
		return;
	}

	ret = mmc_io_rw_direct(func->card, 1, 0, addr, b, NULL);
	if (err_ret)
		*err_ret = ret;
}
EXPORT_SYMBOL_GPL(sdio_f0_writeb);

mmc_pm_flag_t sdio_get_host_pm_caps(struct sdio_func *func)
{
	BUG_ON(!func);
	BUG_ON(!func->card);

	return func->card->host->pm_caps;
}
EXPORT_SYMBOL_GPL(sdio_get_host_pm_caps);

int sdio_set_host_pm_flags(struct sdio_func *func, mmc_pm_flag_t flags)
{
	struct mmc_host *host;

	BUG_ON(!func);
	BUG_ON(!func->card);

	host = func->card->host;

	if (flags & ~host->pm_caps)
		return -EINVAL;

	
	host->pm_flags |= flags;
	return 0;
}
EXPORT_SYMBOL_GPL(sdio_set_host_pm_flags);
