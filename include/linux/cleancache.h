#ifndef _LINUX_CLEANCACHE_H
#define _LINUX_CLEANCACHE_H

#include <linux/fs.h>
#include <linux/exportfs.h>
#include <linux/mm.h>

#define CLEANCACHE_KEY_MAX 6

struct cleancache_filekey {
	union {
		ino_t ino;
		__u32 fh[CLEANCACHE_KEY_MAX];
		u32 key[CLEANCACHE_KEY_MAX];
	} u;
};

struct cleancache_ops {
	int (*init_fs)(size_t);
	int (*init_shared_fs)(char *uuid, size_t);
	int (*get_page)(int, struct cleancache_filekey,
			pgoff_t, struct page *);
	void (*put_page)(int, struct cleancache_filekey,
			pgoff_t, struct page *);
	void (*invalidate_page)(int, struct cleancache_filekey, pgoff_t);
	void (*invalidate_inode)(int, struct cleancache_filekey);
	void (*invalidate_fs)(int);
};

extern struct cleancache_ops
	cleancache_register_ops(struct cleancache_ops *ops);
extern void __cleancache_init_fs(struct super_block *);
extern void __cleancache_init_shared_fs(char *, struct super_block *);
extern int  __cleancache_get_page(struct page *);
extern void __cleancache_put_page(struct page *);
extern void __cleancache_invalidate_page(struct address_space *, struct page *);
extern void __cleancache_invalidate_inode(struct address_space *);
extern void __cleancache_invalidate_fs(struct super_block *);
extern int cleancache_enabled;

#ifdef CONFIG_CLEANCACHE
static inline bool cleancache_fs_enabled(struct page *page)
{
	return page->mapping->host->i_sb->cleancache_poolid >= 0;
}
static inline bool cleancache_fs_enabled_mapping(struct address_space *mapping)
{
	return mapping->host->i_sb->cleancache_poolid >= 0;
}
#else
#define cleancache_enabled (0)
#define cleancache_fs_enabled(_page) (0)
#define cleancache_fs_enabled_mapping(_page) (0)
#endif


static inline void cleancache_init_fs(struct super_block *sb)
{
	if (cleancache_enabled)
		__cleancache_init_fs(sb);
}

static inline void cleancache_init_shared_fs(char *uuid, struct super_block *sb)
{
	if (cleancache_enabled)
		__cleancache_init_shared_fs(uuid, sb);
}

static inline int cleancache_get_page(struct page *page)
{
	int ret = -1;

	if (cleancache_enabled && cleancache_fs_enabled(page))
		ret = __cleancache_get_page(page);
	return ret;
}

static inline void cleancache_put_page(struct page *page)
{
	if (cleancache_enabled && cleancache_fs_enabled(page))
		__cleancache_put_page(page);
}

static inline void cleancache_invalidate_page(struct address_space *mapping,
					struct page *page)
{
	
	if (cleancache_enabled && cleancache_fs_enabled_mapping(mapping))
		__cleancache_invalidate_page(mapping, page);
}

static inline void cleancache_invalidate_inode(struct address_space *mapping)
{
	if (cleancache_enabled && cleancache_fs_enabled_mapping(mapping))
		__cleancache_invalidate_inode(mapping);
}

static inline void cleancache_invalidate_fs(struct super_block *sb)
{
	if (cleancache_enabled)
		__cleancache_invalidate_fs(sb);
}

#endif 
