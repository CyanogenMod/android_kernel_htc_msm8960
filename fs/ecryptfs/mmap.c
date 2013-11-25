/**
 * eCryptfs: Linux filesystem encryption layer
 * This is where eCryptfs coordinates the symmetric encryption and
 * decryption of the file data as it passes between the lower
 * encrypted file and the upper decrypted file.
 *
 * Copyright (C) 1997-2003 Erez Zadok
 * Copyright (C) 2001-2003 Stony Brook University
 * Copyright (C) 2004-2007 International Business Machines Corp.
 *   Author(s): Michael A. Halcrow <mahalcro@us.ibm.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <linux/pagemap.h>
#include <linux/writeback.h>
#include <linux/page-flags.h>
#include <linux/mount.h>
#include <linux/file.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <asm/unaligned.h>
#include "ecryptfs_kernel.h"

struct page *ecryptfs_get_locked_page(struct inode *inode, loff_t index)
{
	struct page *page = read_mapping_page(inode->i_mapping, index, NULL);
	if (!IS_ERR(page))
		lock_page(page);
	return page;
}

static int ecryptfs_writepage(struct page *page, struct writeback_control *wbc)
{
	int rc;

	if (current->flags & PF_MEMALLOC) {
		redirty_page_for_writepage(wbc, page);
		rc = 0;
		goto out;
	}

	rc = ecryptfs_encrypt_page(page);
	if (rc) {
		ecryptfs_printk(KERN_WARNING, "Error encrypting "
				"page (upper index [0x%.16lx])\n", page->index);
		ClearPageUptodate(page);
		goto out;
	}
	SetPageUptodate(page);
out:
	unlock_page(page);
	return rc;
}

static void strip_xattr_flag(char *page_virt,
			     struct ecryptfs_crypt_stat *crypt_stat)
{
	if (crypt_stat->flags & ECRYPTFS_METADATA_IN_XATTR) {
		size_t written;

		crypt_stat->flags &= ~ECRYPTFS_METADATA_IN_XATTR;
		ecryptfs_write_crypt_stat_flags(page_virt, crypt_stat,
						&written);
		crypt_stat->flags |= ECRYPTFS_METADATA_IN_XATTR;
	}
}


static int
ecryptfs_copy_up_encrypted_with_header(struct page *page,
				       struct ecryptfs_crypt_stat *crypt_stat)
{
	loff_t extent_num_in_page = 0;
	loff_t num_extents_per_page = (PAGE_CACHE_SIZE
				       / crypt_stat->extent_size);
	int rc = 0;

	while (extent_num_in_page < num_extents_per_page) {
		loff_t view_extent_num = ((((loff_t)page->index)
					   * num_extents_per_page)
					  + extent_num_in_page);
		size_t num_header_extents_at_front =
			(crypt_stat->metadata_size / crypt_stat->extent_size);

		if (view_extent_num < num_header_extents_at_front) {
			
			char *page_virt;

			page_virt = kmap_atomic(page);
			memset(page_virt, 0, PAGE_CACHE_SIZE);
			
			if (view_extent_num == 0) {
				size_t written;

				rc = ecryptfs_read_xattr_region(
					page_virt, page->mapping->host);
				strip_xattr_flag(page_virt + 16, crypt_stat);
				ecryptfs_write_header_metadata(page_virt + 20,
							       crypt_stat,
							       &written);
			}
			kunmap_atomic(page_virt);
			flush_dcache_page(page);
			if (rc) {
				printk(KERN_ERR "%s: Error reading xattr "
				       "region; rc = [%d]\n", __func__, rc);
				goto out;
			}
		} else {
			
			loff_t lower_offset =
				((view_extent_num * crypt_stat->extent_size)
				 - crypt_stat->metadata_size);

			rc = ecryptfs_read_lower_page_segment(
				page, (lower_offset >> PAGE_CACHE_SHIFT),
				(lower_offset & ~PAGE_CACHE_MASK),
				crypt_stat->extent_size, page->mapping->host);
			if (rc) {
				printk(KERN_ERR "%s: Error attempting to read "
				       "extent at offset [%lld] in the lower "
				       "file; rc = [%d]\n", __func__,
				       lower_offset, rc);
				goto out;
			}
		}
		extent_num_in_page++;
	}
out:
	return rc;
}

static int ecryptfs_readpage(struct file *file, struct page *page)
{
	struct ecryptfs_crypt_stat *crypt_stat =
		&ecryptfs_inode_to_private(page->mapping->host)->crypt_stat;
	int rc = 0;

	if (!crypt_stat || !(crypt_stat->flags & ECRYPTFS_ENCRYPTED)) {
		rc = ecryptfs_read_lower_page_segment(page, page->index, 0,
						      PAGE_CACHE_SIZE,
						      page->mapping->host);
	} else if (crypt_stat->flags & ECRYPTFS_VIEW_AS_ENCRYPTED) {
		if (crypt_stat->flags & ECRYPTFS_METADATA_IN_XATTR) {
			rc = ecryptfs_copy_up_encrypted_with_header(page,
								    crypt_stat);
			if (rc) {
				printk(KERN_ERR "%s: Error attempting to copy "
				       "the encrypted content from the lower "
				       "file whilst inserting the metadata "
				       "from the xattr into the header; rc = "
				       "[%d]\n", __func__, rc);
				goto out;
			}

		} else {
			rc = ecryptfs_read_lower_page_segment(
				page, page->index, 0, PAGE_CACHE_SIZE,
				page->mapping->host);
			if (rc) {
				printk(KERN_ERR "Error reading page; rc = "
				       "[%d]\n", rc);
				goto out;
			}
		}
	} else {
		rc = ecryptfs_decrypt_page(page);
		if (rc) {
			ecryptfs_printk(KERN_ERR, "Error decrypting page; "
					"rc = [%d]\n", rc);
			goto out;
		}
	}
out:
	if (rc)
		ClearPageUptodate(page);
	else
		SetPageUptodate(page);
	ecryptfs_printk(KERN_DEBUG, "Unlocking page with index = [0x%.16lx]\n",
			page->index);
	unlock_page(page);
	return rc;
}

static int fill_zeros_to_end_of_page(struct page *page, unsigned int to)
{
	struct inode *inode = page->mapping->host;
	int end_byte_in_page;

	if ((i_size_read(inode) / PAGE_CACHE_SIZE) != page->index)
		goto out;
	end_byte_in_page = i_size_read(inode) % PAGE_CACHE_SIZE;
	if (to > end_byte_in_page)
		end_byte_in_page = to;
	zero_user_segment(page, end_byte_in_page, PAGE_CACHE_SIZE);
out:
	return 0;
}

static int ecryptfs_write_begin(struct file *file,
			struct address_space *mapping,
			loff_t pos, unsigned len, unsigned flags,
			struct page **pagep, void **fsdata)
{
	pgoff_t index = pos >> PAGE_CACHE_SHIFT;
	struct page *page;
	loff_t prev_page_end_size;
	int rc = 0;

	page = grab_cache_page_write_begin(mapping, index, flags);
	if (!page)
		return -ENOMEM;
	*pagep = page;

	prev_page_end_size = ((loff_t)index << PAGE_CACHE_SHIFT);
	if (!PageUptodate(page)) {
		struct ecryptfs_crypt_stat *crypt_stat =
			&ecryptfs_inode_to_private(mapping->host)->crypt_stat;

		if (!(crypt_stat->flags & ECRYPTFS_ENCRYPTED)) {
			rc = ecryptfs_read_lower_page_segment(
				page, index, 0, PAGE_CACHE_SIZE, mapping->host);
			if (rc) {
				printk(KERN_ERR "%s: Error attemping to read "
				       "lower page segment; rc = [%d]\n",
				       __func__, rc);
				ClearPageUptodate(page);
				goto out;
			} else
				SetPageUptodate(page);
		} else if (crypt_stat->flags & ECRYPTFS_VIEW_AS_ENCRYPTED) {
			if (crypt_stat->flags & ECRYPTFS_METADATA_IN_XATTR) {
				rc = ecryptfs_copy_up_encrypted_with_header(
					page, crypt_stat);
				if (rc) {
					printk(KERN_ERR "%s: Error attempting "
					       "to copy the encrypted content "
					       "from the lower file whilst "
					       "inserting the metadata from "
					       "the xattr into the header; rc "
					       "= [%d]\n", __func__, rc);
					ClearPageUptodate(page);
					goto out;
				}
				SetPageUptodate(page);
			} else {
				rc = ecryptfs_read_lower_page_segment(
					page, index, 0, PAGE_CACHE_SIZE,
					mapping->host);
				if (rc) {
					printk(KERN_ERR "%s: Error reading "
					       "page; rc = [%d]\n",
					       __func__, rc);
					ClearPageUptodate(page);
					goto out;
				}
				SetPageUptodate(page);
			}
		} else {
			if (prev_page_end_size
			    >= i_size_read(page->mapping->host)) {
				zero_user(page, 0, PAGE_CACHE_SIZE);
			} else {
				rc = ecryptfs_decrypt_page(page);
				if (rc) {
					printk(KERN_ERR "%s: Error decrypting "
					       "page at index [%ld]; "
					       "rc = [%d]\n",
					       __func__, page->index, rc);
					ClearPageUptodate(page);
					goto out;
				}
			}
			SetPageUptodate(page);
		}
	}
	if (index != 0) {
		if (prev_page_end_size > i_size_read(page->mapping->host)) {
			rc = ecryptfs_truncate(file->f_path.dentry,
					       prev_page_end_size);
			if (rc) {
				printk(KERN_ERR "%s: Error on attempt to "
				       "truncate to (higher) offset [%lld];"
				       " rc = [%d]\n", __func__,
				       prev_page_end_size, rc);
				goto out;
			}
		}
	}
	if ((i_size_read(mapping->host) == prev_page_end_size)
	    && (pos != 0))
		zero_user(page, 0, PAGE_CACHE_SIZE);
out:
	if (unlikely(rc)) {
		unlock_page(page);
		page_cache_release(page);
		*pagep = NULL;
	}
	return rc;
}

static int ecryptfs_write_inode_size_to_header(struct inode *ecryptfs_inode)
{
	char *file_size_virt;
	int rc;

	file_size_virt = kmalloc(sizeof(u64), GFP_KERNEL);
	if (!file_size_virt) {
		rc = -ENOMEM;
		goto out;
	}
	put_unaligned_be64(i_size_read(ecryptfs_inode), file_size_virt);
	rc = ecryptfs_write_lower(ecryptfs_inode, file_size_virt, 0,
				  sizeof(u64));
	kfree(file_size_virt);
	if (rc < 0)
		printk(KERN_ERR "%s: Error writing file size to header; "
		       "rc = [%d]\n", __func__, rc);
	else
		rc = 0;
out:
	return rc;
}

struct kmem_cache *ecryptfs_xattr_cache;

static int ecryptfs_write_inode_size_to_xattr(struct inode *ecryptfs_inode)
{
	ssize_t size;
	void *xattr_virt;
	struct dentry *lower_dentry =
		ecryptfs_inode_to_private(ecryptfs_inode)->lower_file->f_dentry;
	struct inode *lower_inode = lower_dentry->d_inode;
	int rc;

	if (!lower_inode->i_op->getxattr || !lower_inode->i_op->setxattr) {
		printk(KERN_WARNING
		       "No support for setting xattr in lower filesystem\n");
		rc = -ENOSYS;
		goto out;
	}
	xattr_virt = kmem_cache_alloc(ecryptfs_xattr_cache, GFP_KERNEL);
	if (!xattr_virt) {
		printk(KERN_ERR "Out of memory whilst attempting to write "
		       "inode size to xattr\n");
		rc = -ENOMEM;
		goto out;
	}
	mutex_lock(&lower_inode->i_mutex);
	size = lower_inode->i_op->getxattr(lower_dentry, ECRYPTFS_XATTR_NAME,
					   xattr_virt, PAGE_CACHE_SIZE);
	if (size < 0)
		size = 8;
	put_unaligned_be64(i_size_read(ecryptfs_inode), xattr_virt);
	rc = lower_inode->i_op->setxattr(lower_dentry, ECRYPTFS_XATTR_NAME,
					 xattr_virt, size, 0);
	mutex_unlock(&lower_inode->i_mutex);
	if (rc)
		printk(KERN_ERR "Error whilst attempting to write inode size "
		       "to lower file xattr; rc = [%d]\n", rc);
	kmem_cache_free(ecryptfs_xattr_cache, xattr_virt);
out:
	return rc;
}

int ecryptfs_write_inode_size_to_metadata(struct inode *ecryptfs_inode)
{
	struct ecryptfs_crypt_stat *crypt_stat;

	crypt_stat = &ecryptfs_inode_to_private(ecryptfs_inode)->crypt_stat;
	BUG_ON(!(crypt_stat->flags & ECRYPTFS_ENCRYPTED));
	if (crypt_stat->flags & ECRYPTFS_METADATA_IN_XATTR)
		return ecryptfs_write_inode_size_to_xattr(ecryptfs_inode);
	else
		return ecryptfs_write_inode_size_to_header(ecryptfs_inode);
}

static int ecryptfs_write_end(struct file *file,
			struct address_space *mapping,
			loff_t pos, unsigned len, unsigned copied,
			struct page *page, void *fsdata)
{
	pgoff_t index = pos >> PAGE_CACHE_SHIFT;
	unsigned from = pos & (PAGE_CACHE_SIZE - 1);
	unsigned to = from + copied;
	struct inode *ecryptfs_inode = mapping->host;
	struct ecryptfs_crypt_stat *crypt_stat =
		&ecryptfs_inode_to_private(ecryptfs_inode)->crypt_stat;
	int rc;
	int need_unlock_page = 1;

	ecryptfs_printk(KERN_DEBUG, "Calling fill_zeros_to_end_of_page"
			"(page w/ index = [0x%.16lx], to = [%d])\n", index, to);
	if (!(crypt_stat->flags & ECRYPTFS_ENCRYPTED)) {
		rc = ecryptfs_write_lower_page_segment(ecryptfs_inode, page, 0,
						       to);
		if (!rc) {
			rc = copied;
			fsstack_copy_inode_size(ecryptfs_inode,
				ecryptfs_inode_to_lower(ecryptfs_inode));
		}
		goto out;
	}
	
	rc = fill_zeros_to_end_of_page(page, to);
	if (rc) {
		ecryptfs_printk(KERN_WARNING, "Error attempting to fill "
			"zeros in page with index = [0x%.16lx]\n", index);
		goto out;
	}
	set_page_dirty(page);
	unlock_page(page);
	need_unlock_page = 0;
	if (pos + copied > i_size_read(ecryptfs_inode)) {
		i_size_write(ecryptfs_inode, pos + copied);
		ecryptfs_printk(KERN_DEBUG, "Expanded file size to "
			"[0x%.16llx]\n",
			(unsigned long long)i_size_read(ecryptfs_inode));
		balance_dirty_pages_ratelimited(mapping);
		rc = ecryptfs_write_inode_size_to_metadata(ecryptfs_inode);
		if (rc) {
			printk(KERN_ERR "Error writing inode size to metadata; "
			       "rc = [%d]\n", rc);
			goto out;
		}
	}
	rc = copied;
out:
	if (need_unlock_page)
		unlock_page(page);
	page_cache_release(page);
	return rc;
}

static sector_t ecryptfs_bmap(struct address_space *mapping, sector_t block)
{
	int rc = 0;
	struct inode *inode;
	struct inode *lower_inode;

	inode = (struct inode *)mapping->host;
	lower_inode = ecryptfs_inode_to_lower(inode);
	if (lower_inode->i_mapping->a_ops->bmap)
		rc = lower_inode->i_mapping->a_ops->bmap(lower_inode->i_mapping,
							 block);
	return rc;
}

const struct address_space_operations ecryptfs_aops = {
	.writepage = ecryptfs_writepage,
	.readpage = ecryptfs_readpage,
	.write_begin = ecryptfs_write_begin,
	.write_end = ecryptfs_write_end,
	.bmap = ecryptfs_bmap,
};
