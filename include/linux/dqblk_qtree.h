
#ifndef _LINUX_DQBLK_QTREE_H
#define _LINUX_DQBLK_QTREE_H

#include <linux/types.h>

#define QTREE_INIT_ALLOC 4
#define QTREE_INIT_REWRITE 2
#define QTREE_DEL_ALLOC 0
#define QTREE_DEL_REWRITE 6

struct dquot;

struct qtree_fmt_operations {
	void (*mem2disk_dqblk)(void *disk, struct dquot *dquot);	
	void (*disk2mem_dqblk)(struct dquot *dquot, void *disk);	
	int (*is_id)(void *disk, struct dquot *dquot);	
};

struct qtree_mem_dqinfo {
	struct super_block *dqi_sb;	
	int dqi_type;			
	unsigned int dqi_blocks;	
	unsigned int dqi_free_blk;	
	unsigned int dqi_free_entry;	
	unsigned int dqi_blocksize_bits;	
	unsigned int dqi_entry_size;	
	unsigned int dqi_usable_bs;	
	unsigned int dqi_qtree_depth;	
	struct qtree_fmt_operations *dqi_ops;	
};

int qtree_write_dquot(struct qtree_mem_dqinfo *info, struct dquot *dquot);
int qtree_read_dquot(struct qtree_mem_dqinfo *info, struct dquot *dquot);
int qtree_delete_dquot(struct qtree_mem_dqinfo *info, struct dquot *dquot);
int qtree_release_dquot(struct qtree_mem_dqinfo *info, struct dquot *dquot);
int qtree_entry_unused(struct qtree_mem_dqinfo *info, char *disk);
static inline int qtree_depth(struct qtree_mem_dqinfo *info)
{
	unsigned int epb = info->dqi_usable_bs >> 2;
	unsigned long long entries = epb;
	int i;

	for (i = 1; entries < (1ULL << 32); i++)
		entries *= epb;
	return i;
}

#endif 
