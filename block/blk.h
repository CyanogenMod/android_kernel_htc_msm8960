#ifndef BLK_INTERNAL_H
#define BLK_INTERNAL_H

#include <linux/idr.h>

#define BLK_BATCH_TIME	(HZ/50UL)

#define BLK_BATCH_REQ	32

extern struct kmem_cache *blk_requestq_cachep;
extern struct kobj_type blk_queue_ktype;
extern struct ida blk_queue_ida;

static inline void __blk_get_queue(struct request_queue *q)
{
	kobject_get(&q->kobj);
}

void init_request_from_bio(struct request *req, struct bio *bio);
void blk_rq_bio_prep(struct request_queue *q, struct request *rq,
			struct bio *bio);
int blk_rq_append_bio(struct request_queue *q, struct request *rq,
		      struct bio *bio);
void blk_drain_queue(struct request_queue *q, bool drain_all);
void blk_dequeue_request(struct request *rq);
void __blk_queue_free_tags(struct request_queue *q);
bool __blk_end_bidi_request(struct request *rq, int error,
			    unsigned int nr_bytes, unsigned int bidi_bytes);

void blk_rq_timed_out_timer(unsigned long data);
void blk_delete_timer(struct request *);
void blk_add_timer(struct request *);
void __generic_unplug_device(struct request_queue *);

enum rq_atomic_flags {
	REQ_ATOM_COMPLETE = 0,
	REQ_ATOM_URGENT = 1,
};

static inline int blk_mark_rq_complete(struct request *rq)
{
	return test_and_set_bit(REQ_ATOM_COMPLETE, &rq->atomic_flags);
}

static inline void blk_clear_rq_complete(struct request *rq)
{
	clear_bit(REQ_ATOM_COMPLETE, &rq->atomic_flags);
}

static inline int blk_mark_rq_urgent(struct request *rq)
{
	return test_and_set_bit(REQ_ATOM_URGENT, &rq->atomic_flags);
}

static inline void blk_clear_rq_urgent(struct request *rq)
{
	clear_bit(REQ_ATOM_URGENT, &rq->atomic_flags);
}

#define ELV_ON_HASH(rq)		(!hlist_unhashed(&(rq)->hash))

void blk_insert_flush(struct request *rq);
void blk_abort_flushes(struct request_queue *q);

static inline struct request *__elv_next_request(struct request_queue *q)
{
	struct request *rq;

	while (1) {
		if (!list_empty(&q->queue_head)) {
			rq = list_entry_rq(q->queue_head.next);
			return rq;
		}

		if (q->flush_pending_idx != q->flush_running_idx &&
				!queue_flush_queueable(q)) {
			q->flush_queue_delayed = 1;
			return NULL;
		}
		if (unlikely(blk_queue_dead(q)) ||
		    !q->elevator->type->ops.elevator_dispatch_fn(q, 0))
			return NULL;
	}
}

static inline void elv_activate_rq(struct request_queue *q, struct request *rq)
{
	struct elevator_queue *e = q->elevator;

	if (e->type->ops.elevator_activate_req_fn)
		e->type->ops.elevator_activate_req_fn(q, rq);
}

static inline void elv_deactivate_rq(struct request_queue *q, struct request *rq)
{
	struct elevator_queue *e = q->elevator;

	if (e->type->ops.elevator_deactivate_req_fn)
		e->type->ops.elevator_deactivate_req_fn(q, rq);
}

#ifdef CONFIG_FAIL_IO_TIMEOUT
int blk_should_fake_timeout(struct request_queue *);
ssize_t part_timeout_show(struct device *, struct device_attribute *, char *);
ssize_t part_timeout_store(struct device *, struct device_attribute *,
				const char *, size_t);
#else
static inline int blk_should_fake_timeout(struct request_queue *q)
{
	return 0;
}
#endif

int ll_back_merge_fn(struct request_queue *q, struct request *req,
		     struct bio *bio);
int ll_front_merge_fn(struct request_queue *q, struct request *req, 
		      struct bio *bio);
int attempt_back_merge(struct request_queue *q, struct request *rq);
int attempt_front_merge(struct request_queue *q, struct request *rq);
int blk_attempt_req_merge(struct request_queue *q, struct request *rq,
				struct request *next);
void blk_recalc_rq_segments(struct request *rq);
void blk_rq_set_mixed_merge(struct request *rq);
bool blk_rq_merge_ok(struct request *rq, struct bio *bio);
int blk_try_merge(struct request *rq, struct bio *bio);

void blk_queue_congestion_threshold(struct request_queue *q);

int blk_dev_init(void);

void elv_quiesce_start(struct request_queue *q);
void elv_quiesce_end(struct request_queue *q);


static inline int queue_congestion_on_threshold(struct request_queue *q)
{
	return q->nr_congestion_on;
}

static inline int queue_congestion_off_threshold(struct request_queue *q)
{
	return q->nr_congestion_off;
}

static inline int blk_do_io_stat(struct request *rq)
{
	return rq->rq_disk &&
	       (rq->cmd_flags & REQ_IO_STAT) &&
	       (rq->cmd_type == REQ_TYPE_FS ||
	        (rq->cmd_flags & REQ_DISCARD));
}

void get_io_context(struct io_context *ioc);
struct io_cq *ioc_lookup_icq(struct io_context *ioc, struct request_queue *q);
struct io_cq *ioc_create_icq(struct request_queue *q, gfp_t gfp_mask);
void ioc_clear_queue(struct request_queue *q);

void create_io_context_slowpath(struct task_struct *task, gfp_t gfp_mask,
				int node);

static inline struct io_context *create_io_context(struct task_struct *task,
						   gfp_t gfp_mask, int node)
{
	WARN_ON_ONCE(irqs_disabled());
	if (unlikely(!task->io_context))
		create_io_context_slowpath(task, gfp_mask, node);
	return task->io_context;
}

#ifdef CONFIG_BLK_DEV_THROTTLING
extern bool blk_throtl_bio(struct request_queue *q, struct bio *bio);
extern void blk_throtl_drain(struct request_queue *q);
extern int blk_throtl_init(struct request_queue *q);
extern void blk_throtl_exit(struct request_queue *q);
extern void blk_throtl_release(struct request_queue *q);
#else 
static inline bool blk_throtl_bio(struct request_queue *q, struct bio *bio)
{
	return false;
}
static inline void blk_throtl_drain(struct request_queue *q) { }
static inline int blk_throtl_init(struct request_queue *q) { return 0; }
static inline void blk_throtl_exit(struct request_queue *q) { }
static inline void blk_throtl_release(struct request_queue *q) { }
#endif 

#endif 
