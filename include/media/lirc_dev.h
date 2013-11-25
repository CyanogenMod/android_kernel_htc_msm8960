/*
 * LIRC base driver
 *
 * by Artur Lipowski <alipowski@interia.pl>
 *        This code is licensed under GNU GPL
 *
 */

#ifndef _LINUX_LIRC_DEV_H
#define _LINUX_LIRC_DEV_H

#define MAX_IRCTL_DEVICES 8
#define BUFLEN            16

#define mod(n, div) ((n) % (div))

#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/poll.h>
#include <linux/kfifo.h>
#include <media/lirc.h>

struct lirc_buffer {
	wait_queue_head_t wait_poll;
	spinlock_t fifo_lock;
	unsigned int chunk_size;
	unsigned int size; 
	struct kfifo fifo;
	u8 fifo_initialized;
};

static inline void lirc_buffer_clear(struct lirc_buffer *buf)
{
	unsigned long flags;

	if (buf->fifo_initialized) {
		spin_lock_irqsave(&buf->fifo_lock, flags);
		kfifo_reset(&buf->fifo);
		spin_unlock_irqrestore(&buf->fifo_lock, flags);
	} else
		WARN(1, "calling %s on an uninitialized lirc_buffer\n",
		     __func__);
}

static inline int lirc_buffer_init(struct lirc_buffer *buf,
				    unsigned int chunk_size,
				    unsigned int size)
{
	int ret;

	init_waitqueue_head(&buf->wait_poll);
	spin_lock_init(&buf->fifo_lock);
	buf->chunk_size = chunk_size;
	buf->size = size;
	ret = kfifo_alloc(&buf->fifo, size * chunk_size, GFP_KERNEL);
	if (ret == 0)
		buf->fifo_initialized = 1;

	return ret;
}

static inline void lirc_buffer_free(struct lirc_buffer *buf)
{
	if (buf->fifo_initialized) {
		kfifo_free(&buf->fifo);
		buf->fifo_initialized = 0;
	} else
		WARN(1, "calling %s on an uninitialized lirc_buffer\n",
		     __func__);
}

static inline int lirc_buffer_len(struct lirc_buffer *buf)
{
	int len;
	unsigned long flags;

	spin_lock_irqsave(&buf->fifo_lock, flags);
	len = kfifo_len(&buf->fifo);
	spin_unlock_irqrestore(&buf->fifo_lock, flags);

	return len;
}

static inline int lirc_buffer_full(struct lirc_buffer *buf)
{
	return lirc_buffer_len(buf) == buf->size * buf->chunk_size;
}

static inline int lirc_buffer_empty(struct lirc_buffer *buf)
{
	return !lirc_buffer_len(buf);
}

static inline int lirc_buffer_available(struct lirc_buffer *buf)
{
	return buf->size - (lirc_buffer_len(buf) / buf->chunk_size);
}

static inline unsigned int lirc_buffer_read(struct lirc_buffer *buf,
					    unsigned char *dest)
{
	unsigned int ret = 0;

	if (lirc_buffer_len(buf) >= buf->chunk_size)
		ret = kfifo_out_locked(&buf->fifo, dest, buf->chunk_size,
				       &buf->fifo_lock);
	return ret;

}

static inline unsigned int lirc_buffer_write(struct lirc_buffer *buf,
					     unsigned char *orig)
{
	unsigned int ret;

	ret = kfifo_in_locked(&buf->fifo, orig, buf->chunk_size,
			      &buf->fifo_lock);

	return ret;
}

struct lirc_driver {
	char name[40];
	int minor;
	__u32 code_length;
	unsigned int buffer_size; 
	int sample_rate;
	__u32 features;

	unsigned int chunk_size;

	void *data;
	int min_timeout;
	int max_timeout;
	int (*add_to_buf) (void *data, struct lirc_buffer *buf);
	struct lirc_buffer *rbuf;
	int (*set_use_inc) (void *data);
	void (*set_use_dec) (void *data);
	const struct file_operations *fops;
	struct device *dev;
	struct module *owner;
};



extern int lirc_register_driver(struct lirc_driver *d);

extern int lirc_unregister_driver(int minor);

void *lirc_get_pdata(struct file *file);

int lirc_dev_fop_open(struct inode *inode, struct file *file);
int lirc_dev_fop_close(struct inode *inode, struct file *file);
unsigned int lirc_dev_fop_poll(struct file *file, poll_table *wait);
long lirc_dev_fop_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
ssize_t lirc_dev_fop_read(struct file *file, char __user *buffer, size_t length,
			  loff_t *ppos);
ssize_t lirc_dev_fop_write(struct file *file, const char __user *buffer,
			   size_t length, loff_t *ppos);

#endif
