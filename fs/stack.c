#include <linux/export.h>
#include <linux/fs.h>
#include <linux/fs_stack.h>

void fsstack_copy_inode_size(struct inode *dst, struct inode *src)
{
	loff_t i_size;
	blkcnt_t i_blocks;

	i_size = i_size_read(src);

	if (sizeof(i_blocks) > sizeof(long))
		spin_lock(&src->i_lock);
	i_blocks = src->i_blocks;
	if (sizeof(i_blocks) > sizeof(long))
		spin_unlock(&src->i_lock);

	if (sizeof(i_size) > sizeof(long) || sizeof(i_blocks) > sizeof(long))
		spin_lock(&dst->i_lock);
	i_size_write(dst, i_size);
	dst->i_blocks = i_blocks;
	if (sizeof(i_size) > sizeof(long) || sizeof(i_blocks) > sizeof(long))
		spin_unlock(&dst->i_lock);
}
EXPORT_SYMBOL_GPL(fsstack_copy_inode_size);

void fsstack_copy_attr_all(struct inode *dest, const struct inode *src)
{
	dest->i_mode = src->i_mode;
	dest->i_uid = src->i_uid;
	dest->i_gid = src->i_gid;
	dest->i_rdev = src->i_rdev;
	dest->i_atime = src->i_atime;
	dest->i_mtime = src->i_mtime;
	dest->i_ctime = src->i_ctime;
	dest->i_blkbits = src->i_blkbits;
	dest->i_flags = src->i_flags;
	set_nlink(dest, src->i_nlink);
}
EXPORT_SYMBOL_GPL(fsstack_copy_attr_all);
