#ifndef _LINUX_RMAP_H
#define _LINUX_RMAP_H

#include <linux/list.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/memcontrol.h>

struct anon_vma {
	struct anon_vma *root;	
	struct mutex mutex;	
	atomic_t refcount;

	/*
	 * NOTE: the LSB of the head.next is set by
	 * mm_take_all_locks() _after_ taking the above lock. So the
	 * head must only be read/written after taking the above lock
	 * to be sure to see a valid next pointer. The LSB bit itself
	 * is serialized by a system wide lock only visible to
	 * mm_take_all_locks() (mm_all_locks_mutex).
	 */
	struct list_head head;	
};

struct anon_vma_chain {
	struct vm_area_struct *vma;
	struct anon_vma *anon_vma;
	struct list_head same_vma;   
	struct list_head same_anon_vma;	
};

#ifdef CONFIG_MMU
static inline void get_anon_vma(struct anon_vma *anon_vma)
{
	atomic_inc(&anon_vma->refcount);
}

void __put_anon_vma(struct anon_vma *anon_vma);

static inline void put_anon_vma(struct anon_vma *anon_vma)
{
	if (atomic_dec_and_test(&anon_vma->refcount))
		__put_anon_vma(anon_vma);
}

static inline struct anon_vma *page_anon_vma(struct page *page)
{
	if (((unsigned long)page->mapping & PAGE_MAPPING_FLAGS) !=
					    PAGE_MAPPING_ANON)
		return NULL;
	return page_rmapping(page);
}

static inline void vma_lock_anon_vma(struct vm_area_struct *vma)
{
	struct anon_vma *anon_vma = vma->anon_vma;
	if (anon_vma)
		mutex_lock(&anon_vma->root->mutex);
}

static inline void vma_unlock_anon_vma(struct vm_area_struct *vma)
{
	struct anon_vma *anon_vma = vma->anon_vma;
	if (anon_vma)
		mutex_unlock(&anon_vma->root->mutex);
}

static inline void anon_vma_lock(struct anon_vma *anon_vma)
{
	mutex_lock(&anon_vma->root->mutex);
}

static inline void anon_vma_unlock(struct anon_vma *anon_vma)
{
	mutex_unlock(&anon_vma->root->mutex);
}

void anon_vma_init(void);	
int  anon_vma_prepare(struct vm_area_struct *);
void unlink_anon_vmas(struct vm_area_struct *);
int anon_vma_clone(struct vm_area_struct *, struct vm_area_struct *);
void anon_vma_moveto_tail(struct vm_area_struct *);
int anon_vma_fork(struct vm_area_struct *, struct vm_area_struct *);

static inline void anon_vma_merge(struct vm_area_struct *vma,
				  struct vm_area_struct *next)
{
	VM_BUG_ON(vma->anon_vma != next->anon_vma);
	unlink_anon_vmas(next);
}

struct anon_vma *page_get_anon_vma(struct page *page);

void page_move_anon_rmap(struct page *, struct vm_area_struct *, unsigned long);
void page_add_anon_rmap(struct page *, struct vm_area_struct *, unsigned long);
void do_page_add_anon_rmap(struct page *, struct vm_area_struct *,
			   unsigned long, int);
void page_add_new_anon_rmap(struct page *, struct vm_area_struct *, unsigned long);
void page_add_file_rmap(struct page *);
void page_remove_rmap(struct page *);

void hugepage_add_anon_rmap(struct page *, struct vm_area_struct *,
			    unsigned long);
void hugepage_add_new_anon_rmap(struct page *, struct vm_area_struct *,
				unsigned long);

static inline void page_dup_rmap(struct page *page)
{
	atomic_inc(&page->_mapcount);
}

int page_referenced(struct page *, int is_locked,
			struct mem_cgroup *memcg, unsigned long *vm_flags);
int page_referenced_one(struct page *, struct vm_area_struct *,
	unsigned long address, unsigned int *mapcount, unsigned long *vm_flags);

enum ttu_flags {
	TTU_UNMAP = 0,			
	TTU_MIGRATION = 1,		
	TTU_MUNLOCK = 2,		
	TTU_ACTION_MASK = 0xff,

	TTU_IGNORE_MLOCK = (1 << 8),	
	TTU_IGNORE_ACCESS = (1 << 9),	
	TTU_IGNORE_HWPOISON = (1 << 10),
};
#define TTU_ACTION(x) ((x) & TTU_ACTION_MASK)

bool is_vma_temporary_stack(struct vm_area_struct *vma);

int try_to_unmap(struct page *, enum ttu_flags flags);
int try_to_unmap_one(struct page *, struct vm_area_struct *,
			unsigned long address, enum ttu_flags flags);

pte_t *__page_check_address(struct page *, struct mm_struct *,
				unsigned long, spinlock_t **, int);

static inline pte_t *page_check_address(struct page *page, struct mm_struct *mm,
					unsigned long address,
					spinlock_t **ptlp, int sync)
{
	pte_t *ptep;

	__cond_lock(*ptlp, ptep = __page_check_address(page, mm, address,
						       ptlp, sync));
	return ptep;
}

unsigned long page_address_in_vma(struct page *, struct vm_area_struct *);

int page_mkclean(struct page *);

int try_to_munlock(struct page *);

struct anon_vma *page_lock_anon_vma(struct page *page);
void page_unlock_anon_vma(struct anon_vma *anon_vma);
int page_mapped_in_vma(struct page *page, struct vm_area_struct *vma);

int rmap_walk(struct page *page, int (*rmap_one)(struct page *,
		struct vm_area_struct *, unsigned long, void *), void *arg);

#else	

#define anon_vma_init()		do {} while (0)
#define anon_vma_prepare(vma)	(0)
#define anon_vma_link(vma)	do {} while (0)

static inline int page_referenced(struct page *page, int is_locked,
				  struct mem_cgroup *memcg,
				  unsigned long *vm_flags)
{
	*vm_flags = 0;
	return 0;
}

#define try_to_unmap(page, refs) SWAP_FAIL

static inline int page_mkclean(struct page *page)
{
	return 0;
}


#endif	

#define SWAP_SUCCESS	0
#define SWAP_AGAIN	1
#define SWAP_FAIL	2
#define SWAP_MLOCK	3

#endif	
