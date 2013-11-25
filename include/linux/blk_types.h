#ifndef __LINUX_BLK_TYPES_H
#define __LINUX_BLK_TYPES_H

#ifdef CONFIG_BLOCK

#include <linux/types.h>

struct bio_set;
struct bio;
struct bio_integrity_payload;
struct page;
struct block_device;
typedef void (bio_end_io_t) (struct bio *, int);
typedef void (bio_destructor_t) (struct bio *);

struct bio_vec {
	struct page	*bv_page;
	unsigned int	bv_len;
	unsigned int	bv_offset;
};

struct bio {
	sector_t		bi_sector;	
	struct bio		*bi_next;	
	struct block_device	*bi_bdev;
	unsigned long		bi_flags;	
	unsigned long		bi_rw;		

	unsigned short		bi_vcnt;	
	unsigned short		bi_idx;		

	unsigned int		bi_phys_segments;

	unsigned int		bi_size;	

	unsigned int		bi_seg_front_size;
	unsigned int		bi_seg_back_size;

	unsigned int		bi_max_vecs;	

	atomic_t		bi_cnt;		

	struct bio_vec		*bi_io_vec;	

	bio_end_io_t		*bi_end_io;

	void			*bi_private;
#if defined(CONFIG_BLK_DEV_INTEGRITY)
	struct bio_integrity_payload *bi_integrity;  
#endif

	bio_destructor_t	*bi_destructor;	

	struct bio_vec		bi_inline_vecs[0];
};

#define BIO_UPTODATE	0	
#define BIO_RW_BLOCK	1	
#define BIO_EOF		2	
#define BIO_SEG_VALID	3	
#define BIO_CLONED	4	
#define BIO_BOUNCED	5	
#define BIO_USER_MAPPED 6	
#define BIO_EOPNOTSUPP	7	
#define BIO_NULL_MAPPED 8	
#define BIO_FS_INTEGRITY 9	
#define BIO_QUIET	10	
#define BIO_MAPPED_INTEGRITY 11
#define bio_flagged(bio, flag)	((bio)->bi_flags & (1 << (flag)))

#define BIO_POOL_BITS		(4)
#define BIO_POOL_NONE		((1UL << BIO_POOL_BITS) - 1)
#define BIO_POOL_OFFSET		(BITS_PER_LONG - BIO_POOL_BITS)
#define BIO_POOL_MASK		(1UL << BIO_POOL_OFFSET)
#define BIO_POOL_IDX(bio)	((bio)->bi_flags >> BIO_POOL_OFFSET)

#endif 

enum rq_flag_bits {
	
	__REQ_WRITE,		
	__REQ_FAILFAST_DEV,	
	__REQ_FAILFAST_TRANSPORT, 
	__REQ_FAILFAST_DRIVER,	

	__REQ_SYNC,		
	__REQ_META,		
	__REQ_PRIO,		
	__REQ_DISCARD,		
	__REQ_SECURE,		

	__REQ_NOIDLE,		
	__REQ_FUA,		
	__REQ_FLUSH,		

	
	__REQ_RAHEAD,		
	__REQ_THROTTLED,	

	
	__REQ_SORTED,		
	__REQ_SOFTBARRIER,	
	__REQ_NOMERGE,		
	__REQ_STARTED,		
	__REQ_DONTPREP,		
	__REQ_QUEUED,		
	__REQ_ELVPRIV,		
	__REQ_FAILED,		
	__REQ_QUIET,		
	__REQ_PREEMPT,		
	__REQ_ALLOCED,		
	__REQ_COPY_USER,	
	__REQ_FLUSH_SEQ,	
	__REQ_IO_STAT,		
	__REQ_MIXED_MERGE,	
	__REQ_SANITIZE,		
	__REQ_NR_BITS,		
};

#define REQ_WRITE		(1 << __REQ_WRITE)
#define REQ_FAILFAST_DEV	(1 << __REQ_FAILFAST_DEV)
#define REQ_FAILFAST_TRANSPORT	(1 << __REQ_FAILFAST_TRANSPORT)
#define REQ_FAILFAST_DRIVER	(1 << __REQ_FAILFAST_DRIVER)
#define REQ_SYNC		(1 << __REQ_SYNC)
#define REQ_META		(1 << __REQ_META)
#define REQ_PRIO		(1 << __REQ_PRIO)
#define REQ_DISCARD		(1 << __REQ_DISCARD)
#define REQ_SANITIZE		(1 << __REQ_SANITIZE)
#define REQ_NOIDLE		(1 << __REQ_NOIDLE)

#define REQ_FAILFAST_MASK \
	(REQ_FAILFAST_DEV | REQ_FAILFAST_TRANSPORT | REQ_FAILFAST_DRIVER)
#define REQ_COMMON_MASK \
	(REQ_WRITE | REQ_FAILFAST_MASK | REQ_SYNC | REQ_META | REQ_PRIO | \
	 REQ_DISCARD | REQ_NOIDLE | REQ_FLUSH | REQ_FUA | REQ_SECURE | \
	 REQ_SANITIZE)
#define REQ_CLONE_MASK		REQ_COMMON_MASK

#define REQ_RAHEAD		(1 << __REQ_RAHEAD)
#define REQ_THROTTLED		(1 << __REQ_THROTTLED)

#define REQ_SORTED		(1 << __REQ_SORTED)
#define REQ_SOFTBARRIER		(1 << __REQ_SOFTBARRIER)
#define REQ_FUA			(1 << __REQ_FUA)
#define REQ_NOMERGE		(1 << __REQ_NOMERGE)
#define REQ_STARTED		(1 << __REQ_STARTED)
#define REQ_DONTPREP		(1 << __REQ_DONTPREP)
#define REQ_QUEUED		(1 << __REQ_QUEUED)
#define REQ_ELVPRIV		(1 << __REQ_ELVPRIV)
#define REQ_FAILED		(1 << __REQ_FAILED)
#define REQ_QUIET		(1 << __REQ_QUIET)
#define REQ_PREEMPT		(1 << __REQ_PREEMPT)
#define REQ_ALLOCED		(1 << __REQ_ALLOCED)
#define REQ_COPY_USER		(1 << __REQ_COPY_USER)
#define REQ_FLUSH		(1 << __REQ_FLUSH)
#define REQ_FLUSH_SEQ		(1 << __REQ_FLUSH_SEQ)
#define REQ_IO_STAT		(1 << __REQ_IO_STAT)
#define REQ_MIXED_MERGE		(1 << __REQ_MIXED_MERGE)
#define REQ_SECURE		(1 << __REQ_SECURE)

#endif 
