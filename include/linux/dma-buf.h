/*
 * Header file for dma buffer sharing framework.
 *
 * Copyright(C) 2011 Linaro Limited. All rights reserved.
 * Author: Sumit Semwal <sumit.semwal@ti.com>
 *
 * Many thanks to linaro-mm-sig list, and specially
 * Arnd Bergmann <arnd@arndb.de>, Rob Clark <rob@ti.com> and
 * Daniel Vetter <daniel@ffwll.ch> for their support in creation and
 * refining of this idea.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __DMA_BUF_H__
#define __DMA_BUF_H__

#include <linux/file.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/list.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>

struct device;
struct dma_buf;
struct dma_buf_attachment;

struct dma_buf_ops {
	int (*attach)(struct dma_buf *, struct device *,
			struct dma_buf_attachment *);

	void (*detach)(struct dma_buf *, struct dma_buf_attachment *);

	struct sg_table * (*map_dma_buf)(struct dma_buf_attachment *,
						enum dma_data_direction);
	void (*unmap_dma_buf)(struct dma_buf_attachment *,
						struct sg_table *,
						enum dma_data_direction);

	
	void (*release)(struct dma_buf *);

	int (*begin_cpu_access)(struct dma_buf *, size_t, size_t,
				enum dma_data_direction);
	void (*end_cpu_access)(struct dma_buf *, size_t, size_t,
			       enum dma_data_direction);
	void *(*kmap_atomic)(struct dma_buf *, unsigned long);
	void (*kunmap_atomic)(struct dma_buf *, unsigned long, void *);
	void *(*kmap)(struct dma_buf *, unsigned long);
	void (*kunmap)(struct dma_buf *, unsigned long, void *);

	int (*mmap)(struct dma_buf *, struct vm_area_struct *vma);
};

struct dma_buf {
	size_t size;
	struct file *file;
	struct list_head attachments;
	const struct dma_buf_ops *ops;
	
	struct mutex lock;
	void *priv;
};

struct dma_buf_attachment {
	struct dma_buf *dmabuf;
	struct device *dev;
	struct list_head node;
	void *priv;
};

static inline void get_dma_buf(struct dma_buf *dmabuf)
{
	get_file(dmabuf->file);
}

#ifdef CONFIG_DMA_SHARED_BUFFER
struct dma_buf_attachment *dma_buf_attach(struct dma_buf *dmabuf,
							struct device *dev);
void dma_buf_detach(struct dma_buf *dmabuf,
				struct dma_buf_attachment *dmabuf_attach);
struct dma_buf *dma_buf_export(void *priv, const struct dma_buf_ops *ops,
			       size_t size, int flags);
int dma_buf_fd(struct dma_buf *dmabuf, int flags);
struct dma_buf *dma_buf_get(int fd);
void dma_buf_put(struct dma_buf *dmabuf);

struct sg_table *dma_buf_map_attachment(struct dma_buf_attachment *,
					enum dma_data_direction);
void dma_buf_unmap_attachment(struct dma_buf_attachment *, struct sg_table *,
				enum dma_data_direction);
int dma_buf_begin_cpu_access(struct dma_buf *dma_buf, size_t start, size_t len,
			     enum dma_data_direction dir);
void dma_buf_end_cpu_access(struct dma_buf *dma_buf, size_t start, size_t len,
			    enum dma_data_direction dir);
void *dma_buf_kmap_atomic(struct dma_buf *, unsigned long);
void dma_buf_kunmap_atomic(struct dma_buf *, unsigned long, void *);
void *dma_buf_kmap(struct dma_buf *, unsigned long);
void dma_buf_kunmap(struct dma_buf *, unsigned long, void *);

int dma_buf_mmap(struct dma_buf *, struct vm_area_struct *,
		 unsigned long);
#else

static inline struct dma_buf_attachment *dma_buf_attach(struct dma_buf *dmabuf,
							struct device *dev)
{
	return ERR_PTR(-ENODEV);
}

static inline void dma_buf_detach(struct dma_buf *dmabuf,
				  struct dma_buf_attachment *dmabuf_attach)
{
	return;
}

static inline struct dma_buf *dma_buf_export(void *priv,
					     const struct dma_buf_ops *ops,
					     size_t size, int flags)
{
	return ERR_PTR(-ENODEV);
}

static inline int dma_buf_fd(struct dma_buf *dmabuf, int flags)
{
	return -ENODEV;
}

static inline struct dma_buf *dma_buf_get(int fd)
{
	return ERR_PTR(-ENODEV);
}

static inline void dma_buf_put(struct dma_buf *dmabuf)
{
	return;
}

static inline struct sg_table *dma_buf_map_attachment(
	struct dma_buf_attachment *attach, enum dma_data_direction write)
{
	return ERR_PTR(-ENODEV);
}

static inline void dma_buf_unmap_attachment(struct dma_buf_attachment *attach,
			struct sg_table *sg, enum dma_data_direction dir)
{
	return;
}

static inline int dma_buf_begin_cpu_access(struct dma_buf *dmabuf,
					   size_t start, size_t len,
					   enum dma_data_direction dir)
{
	return -ENODEV;
}

static inline void dma_buf_end_cpu_access(struct dma_buf *dmabuf,
					  size_t start, size_t len,
					  enum dma_data_direction dir)
{
}

static inline void *dma_buf_kmap_atomic(struct dma_buf *dmabuf,
					unsigned long pnum)
{
	return NULL;
}

static inline void dma_buf_kunmap_atomic(struct dma_buf *dmabuf,
					 unsigned long pnum, void *vaddr)
{
}

static inline void *dma_buf_kmap(struct dma_buf *dmabuf, unsigned long pnum)
{
	return NULL;
}

static inline void dma_buf_kunmap(struct dma_buf *dmabuf,
				  unsigned long pnum, void *vaddr)
{
}

static inline int dma_buf_mmap(struct dma_buf *dmabuf,
			       struct vm_area_struct *vma,
			       unsigned long pgoff)
{
	return -ENODEV;
}
#endif 

#endif 
