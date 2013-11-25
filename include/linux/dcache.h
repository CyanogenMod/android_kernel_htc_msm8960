#ifndef __LINUX_DCACHE_H
#define __LINUX_DCACHE_H

#include <linux/atomic.h>
#include <linux/list.h>
#include <linux/rculist.h>
#include <linux/rculist_bl.h>
#include <linux/spinlock.h>
#include <linux/seqlock.h>
#include <linux/cache.h>
#include <linux/rcupdate.h>

struct nameidata;
struct path;
struct vfsmount;

/*
 * linux/include/linux/dcache.h
 *
 * Dirent cache data structures
 *
 * (C) Copyright 1997 Thomas Schoebel-Theuer,
 * with heavy changes by Linus Torvalds
 */

#define IS_ROOT(x) ((x) == (x)->d_parent)

struct qstr {
	unsigned int hash;
	unsigned int len;
	const unsigned char *name;
};

struct dentry_stat_t {
	int nr_dentry;
	int nr_unused;
	int age_limit;          
	int want_pages;         
	int dummy[2];
};
extern struct dentry_stat_t dentry_stat;

#define init_name_hash()		0

static inline unsigned long
partial_name_hash(unsigned long c, unsigned long prevhash)
{
	return (prevhash + (c << 4) + (c >> 4)) * 11;
}

static inline unsigned long end_name_hash(unsigned long hash)
{
	return (unsigned int) hash;
}

extern unsigned int full_name_hash(const unsigned char *, unsigned int);

#ifdef CONFIG_64BIT
# define DNAME_INLINE_LEN 32 
#else
# ifdef CONFIG_SMP
#  define DNAME_INLINE_LEN 36 
# else
#  define DNAME_INLINE_LEN 40 
# endif
#endif

struct dentry {
	
	unsigned int d_flags;		
	seqcount_t d_seq;		
	struct hlist_bl_node d_hash;	
	struct dentry *d_parent;	
	struct qstr d_name;
	struct inode *d_inode;		
	unsigned char d_iname[DNAME_INLINE_LEN];	

	
	unsigned int d_count;		
	spinlock_t d_lock;		
	const struct dentry_operations *d_op;
	struct super_block *d_sb;	
	unsigned long d_time;		
	void *d_fsdata;			

	struct list_head d_lru;		
	union {
		struct list_head d_child;	
	 	struct rcu_head d_rcu;
	} d_u;
	struct list_head d_subdirs;	
	struct list_head d_alias;	
};

enum dentry_d_lock_class
{
	DENTRY_D_LOCK_NORMAL, 
	DENTRY_D_LOCK_NESTED
};

struct dentry_operations {
	int (*d_revalidate)(struct dentry *, struct nameidata *);
	int (*d_hash)(const struct dentry *, const struct inode *,
			struct qstr *);
	int (*d_compare)(const struct dentry *, const struct inode *,
			const struct dentry *, const struct inode *,
			unsigned int, const char *, const struct qstr *);
	int (*d_delete)(const struct dentry *);
	void (*d_release)(struct dentry *);
	void (*d_prune)(struct dentry *);
	void (*d_iput)(struct dentry *, struct inode *);
	char *(*d_dname)(struct dentry *, char *, int);
	struct vfsmount *(*d_automount)(struct path *);
	int (*d_manage)(struct dentry *, bool);
} ____cacheline_aligned;


#define DCACHE_OP_HASH		0x0001
#define DCACHE_OP_COMPARE	0x0002
#define DCACHE_OP_REVALIDATE	0x0004
#define DCACHE_OP_DELETE	0x0008
#define DCACHE_OP_PRUNE         0x0010

#define	DCACHE_DISCONNECTED	0x0020

#define DCACHE_REFERENCED	0x0040  
#define DCACHE_RCUACCESS	0x0080	

#define DCACHE_CANT_MOUNT	0x0100
#define DCACHE_GENOCIDE		0x0200
#define DCACHE_SHRINK_LIST	0x0400

#define DCACHE_NFSFS_RENAMED	0x1000
#define DCACHE_COOKIE		0x2000	
#define DCACHE_FSNOTIFY_PARENT_WATCHED 0x4000
     

#define DCACHE_MOUNTED		0x10000	
#define DCACHE_NEED_AUTOMOUNT	0x20000	
#define DCACHE_MANAGE_TRANSIT	0x40000	
#define DCACHE_NEED_LOOKUP	0x80000 
#define DCACHE_MANAGED_DENTRY \
	(DCACHE_MOUNTED|DCACHE_NEED_AUTOMOUNT|DCACHE_MANAGE_TRANSIT)

extern seqlock_t rename_lock;

static inline int dname_external(struct dentry *dentry)
{
	return dentry->d_name.name != dentry->d_iname;
}

extern void d_instantiate(struct dentry *, struct inode *);
extern struct dentry * d_instantiate_unique(struct dentry *, struct inode *);
extern struct dentry * d_materialise_unique(struct dentry *, struct inode *);
extern void __d_drop(struct dentry *dentry);
extern void d_drop(struct dentry *dentry);
extern void d_delete(struct dentry *);
extern void d_set_d_op(struct dentry *dentry, const struct dentry_operations *op);

extern struct dentry * d_alloc(struct dentry *, const struct qstr *);
extern struct dentry * d_alloc_pseudo(struct super_block *, const struct qstr *);
extern struct dentry * d_splice_alias(struct inode *, struct dentry *);
extern struct dentry * d_add_ci(struct dentry *, struct inode *, struct qstr *);
extern struct dentry *d_find_any_alias(struct inode *inode);
extern struct dentry * d_obtain_alias(struct inode *);
extern void shrink_dcache_sb(struct super_block *);
extern void shrink_dcache_parent(struct dentry *);
extern void shrink_dcache_for_umount(struct super_block *);
extern int d_invalidate(struct dentry *);

extern struct dentry * d_make_root(struct inode *);

extern void d_genocide(struct dentry *);

extern struct dentry *d_find_alias(struct inode *);
extern void d_prune_aliases(struct inode *);

extern int have_submounts(struct dentry *);

extern void d_rehash(struct dentry *);

 
static inline void d_add(struct dentry *entry, struct inode *inode)
{
	d_instantiate(entry, inode);
	d_rehash(entry);
}

static inline struct dentry *d_add_unique(struct dentry *entry, struct inode *inode)
{
	struct dentry *res;

	res = d_instantiate_unique(entry, inode);
	d_rehash(res != NULL ? res : entry);
	return res;
}

extern void dentry_update_name_case(struct dentry *, struct qstr *);

extern void d_move(struct dentry *, struct dentry *);
extern struct dentry *d_ancestor(struct dentry *, struct dentry *);

extern struct dentry *d_lookup(struct dentry *, struct qstr *);
extern struct dentry *d_hash_and_lookup(struct dentry *, struct qstr *);
extern struct dentry *__d_lookup(struct dentry *, struct qstr *);
extern struct dentry *__d_lookup_rcu(const struct dentry *parent,
				const struct qstr *name,
				unsigned *seq, struct inode **inode);

static inline int __d_rcu_to_refcount(struct dentry *dentry, unsigned seq)
{
	int ret = 0;

	assert_spin_locked(&dentry->d_lock);
	if (!read_seqcount_retry(&dentry->d_seq, seq)) {
		ret = 1;
		dentry->d_count++;
	}

	return ret;
}

extern int d_validate(struct dentry *, struct dentry *);

extern char *dynamic_dname(struct dentry *, char *, int, const char *, ...);

extern char *__d_path(const struct path *, const struct path *, char *, int);
extern char *d_absolute_path(const struct path *, char *, int);
extern char *d_path(const struct path *, char *, int);
extern char *d_path_with_unreachable(const struct path *, char *, int);
extern char *dentry_path_raw(struct dentry *, char *, int);
extern char *dentry_path(struct dentry *, char *, int);


static inline struct dentry *dget_dlock(struct dentry *dentry)
{
	if (dentry)
		dentry->d_count++;
	return dentry;
}

static inline struct dentry *dget(struct dentry *dentry)
{
	if (dentry) {
		spin_lock(&dentry->d_lock);
		dget_dlock(dentry);
		spin_unlock(&dentry->d_lock);
	}
	return dentry;
}

extern struct dentry *dget_parent(struct dentry *dentry);

 
static inline int d_unhashed(struct dentry *dentry)
{
	return hlist_bl_unhashed(&dentry->d_hash);
}

static inline int d_unlinked(struct dentry *dentry)
{
	return d_unhashed(dentry) && !IS_ROOT(dentry);
}

static inline int cant_mount(struct dentry *dentry)
{
	return (dentry->d_flags & DCACHE_CANT_MOUNT);
}

static inline void dont_mount(struct dentry *dentry)
{
	spin_lock(&dentry->d_lock);
	dentry->d_flags |= DCACHE_CANT_MOUNT;
	spin_unlock(&dentry->d_lock);
}

extern void dput(struct dentry *);

static inline bool d_managed(struct dentry *dentry)
{
	return dentry->d_flags & DCACHE_MANAGED_DENTRY;
}

static inline bool d_mountpoint(struct dentry *dentry)
{
	return dentry->d_flags & DCACHE_MOUNTED;
}

static inline bool d_need_lookup(struct dentry *dentry)
{
	return dentry->d_flags & DCACHE_NEED_LOOKUP;
}

extern void d_clear_need_lookup(struct dentry *dentry);

extern int sysctl_vfs_cache_pressure;

#endif	
