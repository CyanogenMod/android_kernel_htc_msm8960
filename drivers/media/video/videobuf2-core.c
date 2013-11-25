/*
 * videobuf2-core.c - V4L2 driver helper framework
 *
 * Copyright (C) 2010 Samsung Electronics
 *
 * Author: Pawel Osciak <pawel@osciak.com>
 *	   Marek Szyprowski <m.szyprowski@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/sched.h>

#include <media/videobuf2-core.h>

static int debug;
module_param(debug, int, 0644);

#define dprintk(level, fmt, arg...)					\
	do {								\
		if (debug >= level)					\
			printk(KERN_DEBUG "vb2: " fmt, ## arg);		\
	} while (0)

#define call_memop(q, plane, op, args...)				\
	(((q)->mem_ops->op) ?						\
		((q)->mem_ops->op(args)) : 0)

#define call_qop(q, op, args...)					\
	(((q)->ops->op) ? ((q)->ops->op(args)) : 0)

#define V4L2_BUFFER_STATE_FLAGS	(V4L2_BUF_FLAG_MAPPED | V4L2_BUF_FLAG_QUEUED | \
				 V4L2_BUF_FLAG_DONE | V4L2_BUF_FLAG_ERROR)

static int __vb2_buf_mem_alloc(struct vb2_buffer *vb,
				unsigned long *plane_sizes)
{
	struct vb2_queue *q = vb->vb2_queue;
	void *mem_priv;
	int plane;

	
	for (plane = 0; plane < vb->num_planes; ++plane) {
		mem_priv = call_memop(q, plane, alloc, q->alloc_ctx[plane],
					plane_sizes[plane]);
		if (IS_ERR_OR_NULL(mem_priv))
			goto free;

		
		vb->planes[plane].mem_priv = mem_priv;
		vb->v4l2_planes[plane].length = plane_sizes[plane];
	}

	return 0;
free:
	
	for (; plane > 0; --plane)
		call_memop(q, plane, put, vb->planes[plane - 1].mem_priv);

	return -ENOMEM;
}

static void __vb2_buf_mem_free(struct vb2_buffer *vb)
{
	struct vb2_queue *q = vb->vb2_queue;
	unsigned int plane;

	for (plane = 0; plane < vb->num_planes; ++plane) {
		call_memop(q, plane, put, vb->planes[plane].mem_priv);
		vb->planes[plane].mem_priv = NULL;
		dprintk(3, "Freed plane %d of buffer %d\n",
				plane, vb->v4l2_buf.index);
	}
}

static void __vb2_buf_userptr_put(struct vb2_buffer *vb)
{
	struct vb2_queue *q = vb->vb2_queue;
	unsigned int plane;

	for (plane = 0; plane < vb->num_planes; ++plane) {
		void *mem_priv = vb->planes[plane].mem_priv;

		if (mem_priv) {
			call_memop(q, plane, put_userptr, mem_priv);
			vb->planes[plane].mem_priv = NULL;
		}
	}
}

static void __setup_offsets(struct vb2_queue *q)
{
	unsigned int buffer, plane;
	struct vb2_buffer *vb;
	unsigned long off = 0;

	for (buffer = 0; buffer < q->num_buffers; ++buffer) {
		vb = q->bufs[buffer];
		if (!vb)
			continue;

		for (plane = 0; plane < vb->num_planes; ++plane) {
			vb->v4l2_planes[plane].m.mem_offset = off;

			dprintk(3, "Buffer %d, plane %d offset 0x%08lx\n",
					buffer, plane, off);

			off += vb->v4l2_planes[plane].length;
			off = PAGE_ALIGN(off);
		}
	}
}

static int __vb2_queue_alloc(struct vb2_queue *q, enum v4l2_memory memory,
			     unsigned int num_buffers, unsigned int num_planes,
			     unsigned long plane_sizes[])
{
	unsigned int buffer;
	struct vb2_buffer *vb;
	int ret;

	for (buffer = 0; buffer < num_buffers; ++buffer) {
		
		vb = kzalloc(q->buf_struct_size, GFP_KERNEL);
		if (!vb) {
			dprintk(1, "Memory alloc for buffer struct failed\n");
			break;
		}

		
		if (V4L2_TYPE_IS_MULTIPLANAR(q->type))
			vb->v4l2_buf.length = num_planes;

		vb->state = VB2_BUF_STATE_DEQUEUED;
		vb->vb2_queue = q;
		vb->num_planes = num_planes;
		vb->v4l2_buf.index = buffer;
		vb->v4l2_buf.type = q->type;
		vb->v4l2_buf.memory = memory;

		
		if (memory == V4L2_MEMORY_MMAP) {
			ret = __vb2_buf_mem_alloc(vb, plane_sizes);
			if (ret) {
				dprintk(1, "Failed allocating memory for "
						"buffer %d\n", buffer);
				kfree(vb);
				break;
			}
			ret = call_qop(q, buf_init, vb);
			if (ret) {
				dprintk(1, "Buffer %d %p initialization"
					" failed\n", buffer, vb);
				__vb2_buf_mem_free(vb);
				kfree(vb);
				break;
			}
		}

		q->bufs[buffer] = vb;
	}

	q->num_buffers = buffer;

	__setup_offsets(q);

	dprintk(1, "Allocated %d buffers, %d plane(s) each\n",
			q->num_buffers, num_planes);

	return buffer;
}

static void __vb2_free_mem(struct vb2_queue *q)
{
	unsigned int buffer;
	struct vb2_buffer *vb;

	for (buffer = 0; buffer < q->num_buffers; ++buffer) {
		vb = q->bufs[buffer];
		if (!vb)
			continue;

		
		if (q->memory == V4L2_MEMORY_MMAP)
			__vb2_buf_mem_free(vb);
		else
			__vb2_buf_userptr_put(vb);
	}
}

static void __vb2_queue_free(struct vb2_queue *q)
{
	unsigned int buffer;

	
	if (q->ops->buf_cleanup) {
		for (buffer = 0; buffer < q->num_buffers; ++buffer) {
			if (NULL == q->bufs[buffer])
				continue;
			q->ops->buf_cleanup(q->bufs[buffer]);
		}
	}

	
	__vb2_free_mem(q);

	
	for (buffer = 0; buffer < q->num_buffers; ++buffer) {
		kfree(q->bufs[buffer]);
		q->bufs[buffer] = NULL;
	}

	q->num_buffers = 0;
	q->memory = 0;
}

static int __verify_planes_array(struct vb2_buffer *vb, struct v4l2_buffer *b)
{
	
	if (NULL == b->m.planes) {
		dprintk(1, "Multi-planar buffer passed but "
			   "planes array not provided\n");
		return -EINVAL;
	}

	if (b->length < vb->num_planes || b->length > VIDEO_MAX_PLANES) {
		dprintk(1, "Incorrect planes array length, "
			   "expected %d, got %d\n", vb->num_planes, b->length);
		return -EINVAL;
	}

	return 0;
}

static int __fill_v4l2_buffer(struct vb2_buffer *vb, struct v4l2_buffer *b)
{
	struct vb2_queue *q = vb->vb2_queue;
	int ret = 0;

	
	memcpy(b, &vb->v4l2_buf, offsetof(struct v4l2_buffer, m));
	b->input = vb->v4l2_buf.input;
	b->reserved = vb->v4l2_buf.reserved;

	if (V4L2_TYPE_IS_MULTIPLANAR(q->type)) {
		ret = __verify_planes_array(vb, b);
		if (ret)
			return ret;

		memcpy(b->m.planes, vb->v4l2_planes,
			b->length * sizeof(struct v4l2_plane));
	} else {
		b->length = vb->v4l2_planes[0].length;
		b->bytesused = vb->v4l2_planes[0].bytesused;
		if (q->memory == V4L2_MEMORY_MMAP)
			b->m.offset = vb->v4l2_planes[0].m.mem_offset;
		else if (q->memory == V4L2_MEMORY_USERPTR)
			b->m.userptr = vb->v4l2_planes[0].m.userptr;
	}

	b->flags &= ~V4L2_BUFFER_STATE_FLAGS;

	switch (vb->state) {
	case VB2_BUF_STATE_QUEUED:
	case VB2_BUF_STATE_ACTIVE:
		b->flags |= V4L2_BUF_FLAG_QUEUED;
		break;
	case VB2_BUF_STATE_ERROR:
		b->flags |= V4L2_BUF_FLAG_ERROR;
		
	case VB2_BUF_STATE_DONE:
		b->flags |= V4L2_BUF_FLAG_DONE;
		break;
	case VB2_BUF_STATE_DEQUEUED:
		
		break;
	}

	if (vb->num_planes_mapped == vb->num_planes)
		b->flags |= V4L2_BUF_FLAG_MAPPED;

	return ret;
}

int vb2_querybuf(struct vb2_queue *q, struct v4l2_buffer *b)
{
	struct vb2_buffer *vb;

	if (b->type != q->type) {
		dprintk(1, "querybuf: wrong buffer type\n");
		return -EINVAL;
	}

	if (b->index >= q->num_buffers) {
		dprintk(1, "querybuf: buffer index out of range\n");
		return -EINVAL;
	}
	vb = q->bufs[b->index];

	return __fill_v4l2_buffer(vb, b);
}
EXPORT_SYMBOL(vb2_querybuf);

static int __verify_userptr_ops(struct vb2_queue *q)
{
	if (!(q->io_modes & VB2_USERPTR) || !q->mem_ops->get_userptr ||
	    !q->mem_ops->put_userptr)
		return -EINVAL;

	return 0;
}

static int __verify_mmap_ops(struct vb2_queue *q)
{
	if (!(q->io_modes & VB2_MMAP) || !q->mem_ops->alloc ||
	    !q->mem_ops->put || !q->mem_ops->mmap)
		return -EINVAL;

	return 0;
}

static bool __buffers_in_use(struct vb2_queue *q)
{
	unsigned int buffer, plane;
	struct vb2_buffer *vb;

	for (buffer = 0; buffer < q->num_buffers; ++buffer) {
		vb = q->bufs[buffer];
		for (plane = 0; plane < vb->num_planes; ++plane) {
			if (call_memop(q, plane, num_users,
					vb->planes[plane].mem_priv) > 1)
				return true;
		}
	}

	return false;
}

int vb2_reqbufs(struct vb2_queue *q, struct v4l2_requestbuffers *req)
{
	unsigned int num_buffers, num_planes;
	unsigned long plane_sizes[VIDEO_MAX_PLANES];
	int ret = 0;
	
	num_buffers = 0;
	num_planes = 0;

	if (q->fileio) {
		dprintk(1, "reqbufs: file io in progress\n");
		return -EBUSY;
	}

	if (req->memory != V4L2_MEMORY_MMAP
			&& req->memory != V4L2_MEMORY_USERPTR) {
		dprintk(1, "reqbufs: unsupported memory type\n");
		return -EINVAL;
	}

	if (req->type != q->type) {
		dprintk(1, "reqbufs: requested type is incorrect\n");
		return -EINVAL;
	}

	if (q->streaming) {
		dprintk(1, "reqbufs: streaming active\n");
		return -EBUSY;
	}

	if (req->memory == V4L2_MEMORY_MMAP && __verify_mmap_ops(q)) {
		dprintk(1, "reqbufs: MMAP for current setup unsupported\n");
		return -EINVAL;
	}

	if (req->memory == V4L2_MEMORY_USERPTR && __verify_userptr_ops(q)) {
		dprintk(1, "reqbufs: USERPTR for current setup unsupported\n");
		return -EINVAL;
	}

	if (q->memory == req->memory && req->count == q->num_buffers)
		return 0;

	if (req->count == 0 || q->num_buffers != 0 || q->memory != req->memory) {
		if (q->memory == V4L2_MEMORY_MMAP && __buffers_in_use(q)) {
			dprintk(1, "reqbufs: memory in use, cannot free\n");
			return -EBUSY;
		}

		__vb2_queue_free(q);

		if (req->count == 0)
			return 0;
	}

	num_buffers = min_t(unsigned int, req->count, VIDEO_MAX_FRAME);
	memset(plane_sizes, 0, sizeof(plane_sizes));
	memset(q->alloc_ctx, 0, sizeof(q->alloc_ctx));
	q->memory = req->memory;

	ret = call_qop(q, queue_setup, q, NULL, &num_buffers,
				&num_planes, q->plane_sizes, q->alloc_ctx);

	if (ret)
		return ret;

	
	ret = __vb2_queue_alloc(q, req->memory, num_buffers, num_planes,
				plane_sizes);
	if (ret == 0) {
		dprintk(1, "Memory allocation failed\n");
		return -ENOMEM;
	}

	if (ret < num_buffers) {
		unsigned int orig_num_buffers;

		orig_num_buffers = num_buffers = ret;
		ret = call_qop(q, queue_setup, q, NULL, &num_buffers,
			       &num_planes, q->plane_sizes, q->alloc_ctx);
		if (ret)
			goto free_mem;

		if (orig_num_buffers < num_buffers) {
			ret = -ENOMEM;
			goto free_mem;
		}

		ret = num_buffers;
	}

	req->count = ret;

	return 0;

free_mem:
	__vb2_queue_free(q);
	return ret;
}
EXPORT_SYMBOL_GPL(vb2_reqbufs);

void *vb2_plane_vaddr(struct vb2_buffer *vb, unsigned int plane_no)
{
	struct vb2_queue *q = vb->vb2_queue;

	if (plane_no > vb->num_planes)
		return NULL;

	return call_memop(q, plane_no, vaddr, vb->planes[plane_no].mem_priv);

}
EXPORT_SYMBOL_GPL(vb2_plane_vaddr);

void *vb2_plane_cookie(struct vb2_buffer *vb, unsigned int plane_no)
{
	struct vb2_queue *q = vb->vb2_queue;

	if (plane_no > vb->num_planes)
		return NULL;

	return call_memop(q, plane_no, cookie, vb->planes[plane_no].mem_priv);
}
EXPORT_SYMBOL_GPL(vb2_plane_cookie);

void vb2_buffer_done(struct vb2_buffer *vb, enum vb2_buffer_state state)
{
	struct vb2_queue *q = vb->vb2_queue;
	unsigned long flags;

	if (vb->state != VB2_BUF_STATE_ACTIVE)
		return;

	if (state != VB2_BUF_STATE_DONE && state != VB2_BUF_STATE_ERROR)
		return;

	dprintk(4, "Done processing on buffer %d, state: %d\n",
			vb->v4l2_buf.index, vb->state);

	
	spin_lock_irqsave(&q->done_lock, flags);
	vb->state = state;
	list_add_tail(&vb->done_entry, &q->done_list);
	atomic_dec(&q->queued_count);
	spin_unlock_irqrestore(&q->done_lock, flags);

	
	wake_up(&q->done_wq);
}
EXPORT_SYMBOL_GPL(vb2_buffer_done);

static int __fill_vb2_buffer(struct vb2_buffer *vb, struct v4l2_buffer *b,
				struct v4l2_plane *v4l2_planes)
{
	unsigned int plane;
	int ret;

	if (V4L2_TYPE_IS_MULTIPLANAR(b->type)) {
		ret = __verify_planes_array(vb, b);
		if (ret)
			return ret;

		
		if (V4L2_TYPE_IS_OUTPUT(b->type)) {
			for (plane = 0; plane < vb->num_planes; ++plane) {
				v4l2_planes[plane].bytesused =
					b->m.planes[plane].bytesused;
				v4l2_planes[plane].data_offset =
					b->m.planes[plane].data_offset;
			}
		}

		if (b->memory == V4L2_MEMORY_USERPTR) {
			for (plane = 0; plane < vb->num_planes; ++plane) {
				v4l2_planes[plane].m.userptr =
					b->m.planes[plane].m.userptr;
				v4l2_planes[plane].length =
					b->m.planes[plane].length;
			}
		}
	} else {
		if (V4L2_TYPE_IS_OUTPUT(b->type))
			v4l2_planes[0].bytesused = b->bytesused;

		if (b->memory == V4L2_MEMORY_USERPTR) {
			v4l2_planes[0].m.userptr = b->m.userptr;
			v4l2_planes[0].length = b->length;
		}
	}

	vb->v4l2_buf.field = b->field;
	vb->v4l2_buf.timestamp = b->timestamp;
	vb->v4l2_buf.input = b->input;
	vb->v4l2_buf.flags = b->flags & ~V4L2_BUFFER_STATE_FLAGS;

	return 0;
}

static int __qbuf_userptr(struct vb2_buffer *vb, struct v4l2_buffer *b)
{
	struct v4l2_plane planes[VIDEO_MAX_PLANES];
	struct vb2_queue *q = vb->vb2_queue;
	void *mem_priv;
	unsigned int plane;
	int ret;
	int write = !V4L2_TYPE_IS_OUTPUT(q->type);

	
	ret = __fill_vb2_buffer(vb, b, planes);
	if (ret)
		return ret;

	for (plane = 0; plane < vb->num_planes; ++plane) {
		
		if (vb->v4l2_planes[plane].m.userptr == planes[plane].m.userptr
		    && vb->v4l2_planes[plane].length == planes[plane].length)
			continue;

		dprintk(3, "qbuf: userspace address for plane %d changed, "
				"reacquiring memory\n", plane);

		
		if (vb->planes[plane].mem_priv)
			call_memop(q, plane, put_userptr,
					vb->planes[plane].mem_priv);

		vb->planes[plane].mem_priv = NULL;

		
		if (q->mem_ops->get_userptr) {
			mem_priv = q->mem_ops->get_userptr(q->alloc_ctx[plane],
							planes[plane].m.userptr,
							planes[plane].length,
							write);
			if (IS_ERR(mem_priv)) {
				dprintk(1, "qbuf: failed acquiring userspace "
						"memory for plane %d\n", plane);
				ret = PTR_ERR(mem_priv);
				goto err;
			}
			vb->planes[plane].mem_priv = mem_priv;
		}
	}

	ret = call_qop(q, buf_init, vb);
	if (ret) {
		dprintk(1, "qbuf: buffer initialization failed\n");
		goto err;
	}

	for (plane = 0; plane < vb->num_planes; ++plane)
		vb->v4l2_planes[plane] = planes[plane];

	return 0;
err:
	
	for (plane = 0; plane < vb->num_planes; ++plane) {
		if (vb->planes[plane].mem_priv)
			call_memop(q, plane, put_userptr,
				   vb->planes[plane].mem_priv);
		vb->planes[plane].mem_priv = NULL;
		vb->v4l2_planes[plane].m.userptr = 0;
		vb->v4l2_planes[plane].length = 0;
	}

	return ret;
}

static int __qbuf_mmap(struct vb2_buffer *vb, struct v4l2_buffer *b)
{
	return __fill_vb2_buffer(vb, b, vb->v4l2_planes);
}

static void __enqueue_in_driver(struct vb2_buffer *vb)
{
	struct vb2_queue *q = vb->vb2_queue;

	vb->state = VB2_BUF_STATE_ACTIVE;
	atomic_inc(&q->queued_count);
	q->ops->buf_queue(vb);
}

int vb2_qbuf(struct vb2_queue *q, struct v4l2_buffer *b)
{
	struct vb2_buffer *vb;
	int ret = 0;

	if (q->fileio) {
		dprintk(1, "qbuf: file io in progress\n");
		return -EBUSY;
	}

	if (b->type != q->type) {
		dprintk(1, "qbuf: invalid buffer type\n");
		return -EINVAL;
	}

	if (b->index >= q->num_buffers) {
		dprintk(1, "qbuf: buffer index out of range\n");
		return -EINVAL;
	}

	vb = q->bufs[b->index];
	if (NULL == vb) {
		
		dprintk(1, "qbuf: buffer is NULL\n");
		return -EINVAL;
	}

	if (b->memory != q->memory) {
		dprintk(1, "qbuf: invalid memory type\n");
		return -EINVAL;
	}

	if (vb->state != VB2_BUF_STATE_DEQUEUED) {
		dprintk(1, "qbuf: buffer already in use\n");
		return -EINVAL;
	}

	if (q->memory == V4L2_MEMORY_MMAP)
		ret = __qbuf_mmap(vb, b);
	else if (q->memory == V4L2_MEMORY_USERPTR)
		ret = __qbuf_userptr(vb, b);
	else {
		WARN(1, "Invalid queue type\n");
		return -EINVAL;
	}

	if (ret)
		return ret;

	ret = call_qop(q, buf_prepare, vb);
	if (ret) {
		dprintk(1, "qbuf: buffer preparation failed\n");
		return ret;
	}

	list_add_tail(&vb->queued_entry, &q->queued_list);
	vb->state = VB2_BUF_STATE_QUEUED;

	if (q->streaming)
		__enqueue_in_driver(vb);

	dprintk(1, "qbuf of buffer %d succeeded\n", vb->v4l2_buf.index);
	return 0;
}
EXPORT_SYMBOL_GPL(vb2_qbuf);

static int __vb2_wait_for_done_vb(struct vb2_queue *q, int nonblocking)
{

	for (;;) {
		int ret;

		if (!q->streaming) {
			dprintk(1, "Streaming off, will not wait for buffers\n");
			return -EINVAL;
		}

		if (!list_empty(&q->done_list)) {
			break;
		}

		if (nonblocking) {
			dprintk(1, "Nonblocking and no buffers to dequeue, "
								"will not wait\n");
			return -EAGAIN;
		}

		call_qop(q, wait_prepare, q);

		dprintk(3, "Will sleep waiting for buffers\n");
		ret = wait_event_interruptible(q->done_wq,
				!list_empty(&q->done_list) || !q->streaming);

		call_qop(q, wait_finish, q);
		if (ret)
			return ret;
	}
	return 0;
}

static int __vb2_get_done_vb(struct vb2_queue *q, struct vb2_buffer **vb,
				int nonblocking)
{
	unsigned long flags;
	int ret;

	ret = __vb2_wait_for_done_vb(q, nonblocking);
	if (ret)
		return ret;

	spin_lock_irqsave(&q->done_lock, flags);
	*vb = list_first_entry(&q->done_list, struct vb2_buffer, done_entry);
	list_del(&(*vb)->done_entry);
	spin_unlock_irqrestore(&q->done_lock, flags);

	return 0;
}

int vb2_wait_for_all_buffers(struct vb2_queue *q)
{
	if (!q->streaming) {
		dprintk(1, "Streaming off, will not wait for buffers\n");
		return -EINVAL;
	}
	wait_event(q->done_wq, !atomic_read(&q->queued_count));
	return 0;
}
EXPORT_SYMBOL_GPL(vb2_wait_for_all_buffers);

int vb2_dqbuf(struct vb2_queue *q, struct v4l2_buffer *b, bool nonblocking)
{
	struct vb2_buffer *vb = NULL;
	int ret;

	if (q->fileio) {
		dprintk(1, "dqbuf: file io in progress\n");
		return -EBUSY;
	}

	if (b->type != q->type) {
		dprintk(1, "dqbuf: invalid buffer type\n");
		return -EINVAL;
	}

	ret = __vb2_get_done_vb(q, &vb, nonblocking);
	if (ret < 0) {
		dprintk(1, "dqbuf: error getting next done buffer\n");
		return ret;
	}

	ret = call_qop(q, buf_finish, vb);
	if (ret) {
		dprintk(1, "dqbuf: buffer finish failed\n");
		return ret;
	}

	switch (vb->state) {
	case VB2_BUF_STATE_DONE:
		dprintk(3, "dqbuf: Returning done buffer\n");
		break;
	case VB2_BUF_STATE_ERROR:
		dprintk(3, "dqbuf: Returning done buffer with errors\n");
		break;
	default:
		dprintk(1, "dqbuf: Invalid buffer state\n");
		return -EINVAL;
	}

	
	__fill_v4l2_buffer(vb, b);
	
	list_del(&vb->queued_entry);

	dprintk(1, "dqbuf of buffer %d, with state %d\n",
			vb->v4l2_buf.index, vb->state);

	vb->state = VB2_BUF_STATE_DEQUEUED;
	return 0;
}
EXPORT_SYMBOL_GPL(vb2_dqbuf);

static void __vb2_queue_cancel(struct vb2_queue *q)
{
	unsigned int i;

	if (q->streaming)
		call_qop(q, stop_streaming, q);
	q->streaming = 0;

	INIT_LIST_HEAD(&q->queued_list);
	INIT_LIST_HEAD(&q->done_list);
	atomic_set(&q->queued_count, 0);
	wake_up_all(&q->done_wq);

	for (i = 0; i < q->num_buffers; ++i)
		q->bufs[i]->state = VB2_BUF_STATE_DEQUEUED;
}

int vb2_streamon(struct vb2_queue *q, enum v4l2_buf_type type)
{
	struct vb2_buffer *vb;
	int ret;

	if (q->fileio) {
		dprintk(1, "streamon: file io in progress\n");
		return -EBUSY;
	}

	if (type != q->type) {
		dprintk(1, "streamon: invalid stream type\n");
		return -EINVAL;
	}

	if (q->streaming) {
		dprintk(1, "streamon: already streaming\n");
		return -EBUSY;
	}

	list_for_each_entry(vb, &q->queued_list, queued_entry)
		__enqueue_in_driver(vb);

	ret = call_qop(q, start_streaming, q, atomic_read(&q->queued_count));
	if (ret) {
		dprintk(1, "streamon: driver refused to start streaming\n");
		__vb2_queue_cancel(q);
		return ret;
	}

	q->streaming = 1;

	dprintk(3, "Streamon successful\n");
	return 0;
}
EXPORT_SYMBOL_GPL(vb2_streamon);


int vb2_streamoff(struct vb2_queue *q, enum v4l2_buf_type type)
{
	if (q->fileio) {
		dprintk(1, "streamoff: file io in progress\n");
		return -EBUSY;
	}

	if (type != q->type) {
		dprintk(1, "streamoff: invalid stream type\n");
		return -EINVAL;
	}

	if (!q->streaming) {
		dprintk(1, "streamoff: not streaming\n");
		return -EINVAL;
	}

	__vb2_queue_cancel(q);

	dprintk(3, "Streamoff successful\n");
	return 0;
}
EXPORT_SYMBOL_GPL(vb2_streamoff);

static int __find_plane_by_offset(struct vb2_queue *q, unsigned long off,
			unsigned int *_buffer, unsigned int *_plane)
{
	struct vb2_buffer *vb;
	unsigned int buffer, plane;

	for (buffer = 0; buffer < q->num_buffers; ++buffer) {
		vb = q->bufs[buffer];

		for (plane = 0; plane < vb->num_planes; ++plane) {
			if (vb->v4l2_planes[plane].m.mem_offset == off) {
				*_buffer = buffer;
				*_plane = plane;
				return 0;
			}
		}
	}

	return -EINVAL;
}

int vb2_mmap(struct vb2_queue *q, struct vm_area_struct *vma)
{
	unsigned long off = vma->vm_pgoff << PAGE_SHIFT;
	struct vb2_plane *vb_plane;
	struct vb2_buffer *vb;
	unsigned int buffer, plane;
	int ret;

	if (q->memory != V4L2_MEMORY_MMAP) {
		dprintk(1, "Queue is not currently set up for mmap\n");
		return -EINVAL;
	}

	if (!(vma->vm_flags & VM_SHARED)) {
		dprintk(1, "Invalid vma flags, VM_SHARED needed\n");
		return -EINVAL;
	}
	if (V4L2_TYPE_IS_OUTPUT(q->type)) {
		if (!(vma->vm_flags & VM_WRITE)) {
			dprintk(1, "Invalid vma flags, VM_WRITE needed\n");
			return -EINVAL;
		}
	} else {
		if (!(vma->vm_flags & VM_READ)) {
			dprintk(1, "Invalid vma flags, VM_READ needed\n");
			return -EINVAL;
		}
	}

	ret = __find_plane_by_offset(q, off, &buffer, &plane);
	if (ret)
		return ret;

	vb = q->bufs[buffer];
	vb_plane = &vb->planes[plane];

	ret = q->mem_ops->mmap(vb_plane->mem_priv, vma);
	if (ret)
		return ret;

	vb_plane->mapped = 1;
	vb->num_planes_mapped++;

	dprintk(3, "Buffer %d, plane %d successfully mapped\n", buffer, plane);
	return 0;
}
EXPORT_SYMBOL_GPL(vb2_mmap);

static int __vb2_init_fileio(struct vb2_queue *q, int read);
static int __vb2_cleanup_fileio(struct vb2_queue *q);

unsigned int vb2_poll(struct vb2_queue *q, struct file *file, poll_table *wait)
{
	unsigned long flags;
	unsigned int ret;
	struct vb2_buffer *vb = NULL;

	if (q->num_buffers == 0 && q->fileio == NULL) {
		if (!V4L2_TYPE_IS_OUTPUT(q->type) && (q->io_modes & VB2_READ)) {
			ret = __vb2_init_fileio(q, 1);
			if (ret)
				return POLLERR;
		}
		if (V4L2_TYPE_IS_OUTPUT(q->type) && (q->io_modes & VB2_WRITE)) {
			ret = __vb2_init_fileio(q, 0);
			if (ret)
				return POLLERR;
			return POLLOUT | POLLWRNORM;
		}
	}

	if (list_empty(&q->queued_list))
		return POLLERR;

	poll_wait(file, &q->done_wq, wait);

	spin_lock_irqsave(&q->done_lock, flags);
	if (!list_empty(&q->done_list))
		vb = list_first_entry(&q->done_list, struct vb2_buffer,
					done_entry);
	spin_unlock_irqrestore(&q->done_lock, flags);

	if (vb && (vb->state == VB2_BUF_STATE_DONE
			|| vb->state == VB2_BUF_STATE_ERROR)) {
		return (V4L2_TYPE_IS_OUTPUT(q->type)) ? POLLOUT | POLLWRNORM :
			POLLIN | POLLRDNORM;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(vb2_poll);

int vb2_queue_init(struct vb2_queue *q)
{
	BUG_ON(!q);
	BUG_ON(!q->ops);
	BUG_ON(!q->mem_ops);
	BUG_ON(!q->type);
	BUG_ON(!q->io_modes);

	BUG_ON(!q->ops->queue_setup);
	BUG_ON(!q->ops->buf_queue);

	INIT_LIST_HEAD(&q->queued_list);
	INIT_LIST_HEAD(&q->done_list);
	spin_lock_init(&q->done_lock);
	init_waitqueue_head(&q->done_wq);

	if (q->buf_struct_size == 0)
		q->buf_struct_size = sizeof(struct vb2_buffer);

	return 0;
}
EXPORT_SYMBOL_GPL(vb2_queue_init);

void vb2_queue_release(struct vb2_queue *q)
{
	__vb2_cleanup_fileio(q);
	__vb2_queue_cancel(q);
	__vb2_queue_free(q);
}
EXPORT_SYMBOL_GPL(vb2_queue_release);

struct vb2_fileio_buf {
	void *vaddr;
	unsigned int size;
	unsigned int pos;
	unsigned int queued:1;
};

struct vb2_fileio_data {
	struct v4l2_requestbuffers req;
	struct v4l2_buffer b;
	struct vb2_fileio_buf bufs[VIDEO_MAX_FRAME];
	unsigned int index;
	unsigned int q_count;
	unsigned int dq_count;
	unsigned int flags;
};

static int __vb2_init_fileio(struct vb2_queue *q, int read)
{
	struct vb2_fileio_data *fileio;
	int i, ret;
	unsigned int count = 0;

	if ((read && !(q->io_modes & VB2_READ)) ||
	   (!read && !(q->io_modes & VB2_WRITE)))
		BUG();

	if (!q->mem_ops->vaddr)
		return -EBUSY;

	if (q->streaming || q->num_buffers > 0)
		return -EBUSY;

	count = 1;

	dprintk(3, "setting up file io: mode %s, count %d, flags %08x\n",
		(read) ? "read" : "write", count, q->io_flags);

	fileio = kzalloc(sizeof(struct vb2_fileio_data), GFP_KERNEL);
	if (fileio == NULL)
		return -ENOMEM;

	fileio->flags = q->io_flags;

	fileio->req.count = count;
	fileio->req.memory = V4L2_MEMORY_MMAP;
	fileio->req.type = q->type;
	ret = vb2_reqbufs(q, &fileio->req);
	if (ret)
		goto err_kfree;

	if (q->bufs[0]->num_planes != 1) {
		fileio->req.count = 0;
		ret = -EBUSY;
		goto err_reqbufs;
	}

	for (i = 0; i < q->num_buffers; i++) {
		fileio->bufs[i].vaddr = vb2_plane_vaddr(q->bufs[i], 0);
		if (fileio->bufs[i].vaddr == NULL)
			goto err_reqbufs;
		fileio->bufs[i].size = vb2_plane_size(q->bufs[i], 0);
	}

	if (read) {
		for (i = 0; i < q->num_buffers; i++) {
			struct v4l2_buffer *b = &fileio->b;
			memset(b, 0, sizeof(*b));
			b->type = q->type;
			b->memory = q->memory;
			b->index = i;
			ret = vb2_qbuf(q, b);
			if (ret)
				goto err_reqbufs;
			fileio->bufs[i].queued = 1;
		}

		ret = vb2_streamon(q, q->type);
		if (ret)
			goto err_reqbufs;
	}

	q->fileio = fileio;

	return ret;

err_reqbufs:
	vb2_reqbufs(q, &fileio->req);

err_kfree:
	kfree(fileio);
	return ret;
}

static int __vb2_cleanup_fileio(struct vb2_queue *q)
{
	struct vb2_fileio_data *fileio = q->fileio;

	if (fileio) {
		q->fileio = NULL;

		vb2_streamoff(q, q->type);
		fileio->req.count = 0;
		vb2_reqbufs(q, &fileio->req);
		kfree(fileio);
		dprintk(3, "file io emulator closed\n");
	}
	return 0;
}

static size_t __vb2_perform_fileio(struct vb2_queue *q, char __user *data, size_t count,
		loff_t *ppos, int nonblock, int read)
{
	struct vb2_fileio_data *fileio;
	struct vb2_fileio_buf *buf;
	int ret, index;

	dprintk(3, "file io: mode %s, offset %ld, count %zd, %sblocking\n",
		read ? "read" : "write", (long)*ppos, count,
		nonblock ? "non" : "");

	if (!data)
		return -EINVAL;

	if (!q->fileio) {
		ret = __vb2_init_fileio(q, read);
		dprintk(3, "file io: vb2_init_fileio result: %d\n", ret);
		if (ret)
			return ret;
	}
	fileio = q->fileio;

	q->fileio = NULL;

	index = fileio->index;
	buf = &fileio->bufs[index];

	if (buf->queued) {
		struct vb2_buffer *vb;

		memset(&fileio->b, 0, sizeof(fileio->b));
		fileio->b.type = q->type;
		fileio->b.memory = q->memory;
		fileio->b.index = index;
		ret = vb2_dqbuf(q, &fileio->b, nonblock);
		dprintk(5, "file io: vb2_dqbuf result: %d\n", ret);
		if (ret)
			goto end;
		fileio->dq_count += 1;

		vb = q->bufs[index];
		buf->size = vb2_get_plane_payload(vb, 0);
		buf->queued = 0;
	}

	if (buf->pos + count > buf->size) {
		count = buf->size - buf->pos;
		dprintk(5, "reducing read count: %zd\n", count);
	}

	dprintk(3, "file io: copying %zd bytes - buffer %d, offset %u\n",
		count, index, buf->pos);
	if (read)
		ret = copy_to_user(data, buf->vaddr + buf->pos, count);
	else
		ret = copy_from_user(buf->vaddr + buf->pos, data, count);
	if (ret) {
		dprintk(3, "file io: error copying data\n");
		ret = -EFAULT;
		goto end;
	}

	buf->pos += count;
	*ppos += count;

	if (buf->pos == buf->size ||
	   (!read && (fileio->flags & VB2_FILEIO_WRITE_IMMEDIATELY))) {
		if (read && (fileio->flags & VB2_FILEIO_READ_ONCE) &&
		    fileio->dq_count == 1) {
			dprintk(3, "file io: read limit reached\n");
			q->fileio = fileio;
			return __vb2_cleanup_fileio(q);
		}

		memset(&fileio->b, 0, sizeof(fileio->b));
		fileio->b.type = q->type;
		fileio->b.memory = q->memory;
		fileio->b.index = index;
		fileio->b.bytesused = buf->pos;
		ret = vb2_qbuf(q, &fileio->b);
		dprintk(5, "file io: vb2_dbuf result: %d\n", ret);
		if (ret)
			goto end;

		buf->pos = 0;
		buf->queued = 1;
		buf->size = q->bufs[0]->v4l2_planes[0].length;
		fileio->q_count += 1;

		fileio->index = (index + 1) % q->num_buffers;

		if (!read && !q->streaming) {
			ret = vb2_streamon(q, q->type);
			if (ret)
				goto end;
		}
	}

	if (ret == 0)
		ret = count;
end:
	q->fileio = fileio;
	return ret;
}

size_t vb2_read(struct vb2_queue *q, char __user *data, size_t count,
		loff_t *ppos, int nonblocking)
{
	return __vb2_perform_fileio(q, data, count, ppos, nonblocking, 1);
}
EXPORT_SYMBOL_GPL(vb2_read);

size_t vb2_write(struct vb2_queue *q, char __user *data, size_t count,
		loff_t *ppos, int nonblocking)
{
	return __vb2_perform_fileio(q, data, count, ppos, nonblocking, 0);
}
EXPORT_SYMBOL_GPL(vb2_write);

MODULE_DESCRIPTION("Driver helper framework for Video for Linux 2");
MODULE_AUTHOR("Pawel Osciak <pawel@osciak.com>, Marek Szyprowski");
MODULE_LICENSE("GPL");
