/*
 * linux/include/linux/relay.h
 *
 * Copyright (C) 2002, 2003 - Tom Zanussi (zanussi@us.ibm.com), IBM Corp
 * Copyright (C) 1999, 2000, 2001, 2002 - Karim Yaghmour (karim@opersys.com)
 *
 * CONFIG_RELAY definitions and declarations
 */

#ifndef _LINUX_RELAY_H
#define _LINUX_RELAY_H

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/bug.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/kref.h>

#define FIX_SIZE(x) ((((x) - 1) & PAGE_MASK) + PAGE_SIZE)

#define RELAYFS_CHANNEL_VERSION		7

struct rchan_buf
{
	void *start;			
	void *data;			
	size_t offset;			
	size_t subbufs_produced;	
	size_t subbufs_consumed;	
	struct rchan *chan;		
	wait_queue_head_t read_wait;	
	struct timer_list timer; 	
	struct dentry *dentry;		
	struct kref kref;		
	struct page **page_array;	
	unsigned int page_count;	
	unsigned int finalized;		
	size_t *padding;		
	size_t prev_padding;		
	size_t bytes_consumed;		
	size_t early_bytes;		
	unsigned int cpu;		
} ____cacheline_aligned;

struct rchan
{
	u32 version;			
	size_t subbuf_size;		
	size_t n_subbufs;		
	size_t alloc_size;		
	struct rchan_callbacks *cb;	
	struct kref kref;		
	void *private_data;		
	size_t last_toobig;		
	struct rchan_buf *buf[NR_CPUS]; 
	int is_global;			
	struct list_head list;		
	struct dentry *parent;		
	int has_base_filename;		
	char base_filename[NAME_MAX];	
};

struct rchan_callbacks
{
	int (*subbuf_start) (struct rchan_buf *buf,
			     void *subbuf,
			     void *prev_subbuf,
			     size_t prev_padding);

        void (*buf_mapped)(struct rchan_buf *buf,
			   struct file *filp);

        void (*buf_unmapped)(struct rchan_buf *buf,
			     struct file *filp);
	struct dentry *(*create_buf_file)(const char *filename,
					  struct dentry *parent,
					  umode_t mode,
					  struct rchan_buf *buf,
					  int *is_global);

	int (*remove_buf_file)(struct dentry *dentry);
};


struct rchan *relay_open(const char *base_filename,
			 struct dentry *parent,
			 size_t subbuf_size,
			 size_t n_subbufs,
			 struct rchan_callbacks *cb,
			 void *private_data);
extern int relay_late_setup_files(struct rchan *chan,
				  const char *base_filename,
				  struct dentry *parent);
extern void relay_close(struct rchan *chan);
extern void relay_flush(struct rchan *chan);
extern void relay_subbufs_consumed(struct rchan *chan,
				   unsigned int cpu,
				   size_t consumed);
extern void relay_reset(struct rchan *chan);
extern int relay_buf_full(struct rchan_buf *buf);

extern size_t relay_switch_subbuf(struct rchan_buf *buf,
				  size_t length);

/**
 *	relay_write - write data into the channel
 *	@chan: relay channel
 *	@data: data to be written
 *	@length: number of bytes to write
 *
 *	Writes data into the current cpu's channel buffer.
 *
 *	Protects the buffer by disabling interrupts.  Use this
 *	if you might be logging from interrupt context.  Try
 *	__relay_write() if you know you	won't be logging from
 *	interrupt context.
 */
static inline void relay_write(struct rchan *chan,
			       const void *data,
			       size_t length)
{
	unsigned long flags;
	struct rchan_buf *buf;

	local_irq_save(flags);
	buf = chan->buf[smp_processor_id()];
	if (unlikely(buf->offset + length > chan->subbuf_size))
		length = relay_switch_subbuf(buf, length);
	memcpy(buf->data + buf->offset, data, length);
	buf->offset += length;
	local_irq_restore(flags);
}

/**
 *	__relay_write - write data into the channel
 *	@chan: relay channel
 *	@data: data to be written
 *	@length: number of bytes to write
 *
 *	Writes data into the current cpu's channel buffer.
 *
 *	Protects the buffer by disabling preemption.  Use
 *	relay_write() if you might be logging from interrupt
 *	context.
 */
static inline void __relay_write(struct rchan *chan,
				 const void *data,
				 size_t length)
{
	struct rchan_buf *buf;

	buf = chan->buf[get_cpu()];
	if (unlikely(buf->offset + length > buf->chan->subbuf_size))
		length = relay_switch_subbuf(buf, length);
	memcpy(buf->data + buf->offset, data, length);
	buf->offset += length;
	put_cpu();
}

static inline void *relay_reserve(struct rchan *chan, size_t length)
{
	void *reserved;
	struct rchan_buf *buf = chan->buf[smp_processor_id()];

	if (unlikely(buf->offset + length > buf->chan->subbuf_size)) {
		length = relay_switch_subbuf(buf, length);
		if (!length)
			return NULL;
	}
	reserved = buf->data + buf->offset;
	buf->offset += length;

	return reserved;
}

static inline void subbuf_start_reserve(struct rchan_buf *buf,
					size_t length)
{
	BUG_ON(length >= buf->chan->subbuf_size - 1);
	buf->offset = length;
}

extern const struct file_operations relay_file_operations;

#endif 

