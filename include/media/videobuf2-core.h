/*
 * videobuf2-core.h - V4L2 driver helper framework
 *
 * Copyright (C) 2010 Samsung Electronics
 *
 * Author: Pawel Osciak <pawel@osciak.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */
#ifndef _MEDIA_VIDEOBUF2_CORE_H
#define _MEDIA_VIDEOBUF2_CORE_H

#include <linux/mm_types.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/videodev2.h>

struct vb2_alloc_ctx;
struct vb2_fileio_data;

struct vb2_mem_ops {
	void		*(*alloc)(void *alloc_ctx, unsigned long size);
	void		(*put)(void *buf_priv);

	void		*(*get_userptr)(void *alloc_ctx, unsigned long vaddr,
					unsigned long size, int write);
	void		(*put_userptr)(void *buf_priv);

	void		*(*vaddr)(void *buf_priv);
	void		*(*cookie)(void *buf_priv);

	unsigned int	(*num_users)(void *buf_priv);

	int		(*mmap)(void *buf_priv, struct vm_area_struct *vma);
};

struct vb2_plane {
	void			*mem_priv;
	int			mapped:1;
};

enum vb2_io_modes {
	VB2_MMAP	= (1 << 0),
	VB2_USERPTR	= (1 << 1),
	VB2_READ	= (1 << 2),
	VB2_WRITE	= (1 << 3),
};

enum vb2_fileio_flags {
	VB2_FILEIO_READ_ONCE		= (1 << 0),
	VB2_FILEIO_WRITE_IMMEDIATELY	= (1 << 1),
};

enum vb2_buffer_state {
	VB2_BUF_STATE_DEQUEUED,
	VB2_BUF_STATE_QUEUED,
	VB2_BUF_STATE_ACTIVE,
	VB2_BUF_STATE_DONE,
	VB2_BUF_STATE_ERROR,
};

struct vb2_queue;

struct vb2_buffer {
	struct v4l2_buffer	v4l2_buf;
	struct v4l2_plane	v4l2_planes[VIDEO_MAX_PLANES];

	struct vb2_queue	*vb2_queue;

	unsigned int		num_planes;

	enum vb2_buffer_state	state;

	struct list_head	queued_entry;
	struct list_head	done_entry;

	struct vb2_plane	planes[VIDEO_MAX_PLANES];
	unsigned int		num_planes_mapped;
};

struct vb2_ops {
	int (*queue_setup)(struct vb2_queue *q, unsigned int *num_buffers,
			   unsigned int *num_planes, unsigned long sizes[],
			   void *alloc_ctxs[]);

	void (*wait_prepare)(struct vb2_queue *q);
	void (*wait_finish)(struct vb2_queue *q);

	int (*buf_init)(struct vb2_buffer *vb);
	int (*buf_prepare)(struct vb2_buffer *vb);
	int (*buf_finish)(struct vb2_buffer *vb);
	void (*buf_cleanup)(struct vb2_buffer *vb);

	int (*start_streaming)(struct vb2_queue *q);
	int (*stop_streaming)(struct vb2_queue *q);

	void (*buf_queue)(struct vb2_buffer *vb);
};

struct vb2_queue {
	enum v4l2_buf_type		type;
	unsigned int			io_modes;
	unsigned int			io_flags;

	const struct vb2_ops		*ops;
	const struct vb2_mem_ops	*mem_ops;
	void				*drv_priv;
	unsigned int			buf_struct_size;

	enum v4l2_memory		memory;
	struct vb2_buffer		*bufs[VIDEO_MAX_FRAME];
	unsigned int			num_buffers;

	struct list_head		queued_list;

	atomic_t			queued_count;
	struct list_head		done_list;
	spinlock_t			done_lock;
	wait_queue_head_t		done_wq;

	void				*alloc_ctx[VIDEO_MAX_PLANES];

	unsigned int			streaming:1;

	struct vb2_fileio_data		*fileio;
};

void *vb2_plane_vaddr(struct vb2_buffer *vb, unsigned int plane_no);
void *vb2_plane_cookie(struct vb2_buffer *vb, unsigned int plane_no);

void vb2_buffer_done(struct vb2_buffer *vb, enum vb2_buffer_state state);
int vb2_wait_for_all_buffers(struct vb2_queue *q);

int vb2_querybuf(struct vb2_queue *q, struct v4l2_buffer *b);
int vb2_reqbufs(struct vb2_queue *q, struct v4l2_requestbuffers *req);

int vb2_queue_init(struct vb2_queue *q);

void vb2_queue_release(struct vb2_queue *q);

int vb2_qbuf(struct vb2_queue *q, struct v4l2_buffer *b);
int vb2_dqbuf(struct vb2_queue *q, struct v4l2_buffer *b, bool nonblocking);

int vb2_streamon(struct vb2_queue *q, enum v4l2_buf_type type);
int vb2_streamoff(struct vb2_queue *q, enum v4l2_buf_type type);

int vb2_mmap(struct vb2_queue *q, struct vm_area_struct *vma);
unsigned int vb2_poll(struct vb2_queue *q, struct file *file, poll_table *wait);
size_t vb2_read(struct vb2_queue *q, char __user *data, size_t count,
		loff_t *ppos, int nonblock);
size_t vb2_write(struct vb2_queue *q, char __user *data, size_t count,
		loff_t *ppos, int nonblock);

static inline bool vb2_is_streaming(struct vb2_queue *q)
{
	return q->streaming;
}

static inline bool vb2_is_busy(struct vb2_queue *q)
{
	return (q->num_buffers > 0);
}

static inline void *vb2_get_drv_priv(struct vb2_queue *q)
{
	return q->drv_priv;
}

static inline void vb2_set_plane_payload(struct vb2_buffer *vb,
				 unsigned int plane_no, unsigned long size)
{
	if (plane_no < vb->num_planes)
		vb->v4l2_planes[plane_no].bytesused = size;
}

static inline unsigned long vb2_get_plane_payload(struct vb2_buffer *vb,
				 unsigned int plane_no)
{
	if (plane_no < vb->num_planes)
		return vb->v4l2_planes[plane_no].bytesused;
	return 0;
}

static inline unsigned long
vb2_plane_size(struct vb2_buffer *vb, unsigned int plane_no)
{
	if (plane_no < vb->num_planes)
		return vb->v4l2_planes[plane_no].length;
	return 0;
}

#endif 
