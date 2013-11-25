/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/fb.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/file.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/anon_inodes.h>
#include <linux/miscdevice.h>
#include <linux/genlock.h>


#define _UNLOCKED 0
#define _RDLOCK  GENLOCK_RDLOCK
#define _WRLOCK GENLOCK_WRLOCK

#define GENLOCK_LOG_ERR(fmt, args...) \
pr_err("genlock: %s: " fmt, __func__, ##args)

#define GENLOCK_MAGIC_OK  0xD2EAD10C
#define GENLOCK_MAGIC_BAD 0xD2EADBAD

struct genlock {
	unsigned int magic;       
	struct list_head active;  
	spinlock_t lock;          
	wait_queue_head_t queue;  
	struct file *file;        
	int state;                
	struct kref refcount;
};

struct genlock_handle {
	struct genlock *lock;     
	struct list_head entry;   
	struct file *file;        
	int active;		  
	struct genlock_info info;
};


static DEFINE_SPINLOCK(genlock_ref_lock);

static void genlock_destroy(struct kref *kref)
{
	struct genlock *lock = container_of(kref, struct genlock,
			refcount);


	if (lock->file)
		lock->file->private_data = NULL;
	lock->magic = GENLOCK_MAGIC_BAD;

	kfree(lock);
}


static int genlock_release(struct inode *inodep, struct file *file)
{
	struct genlock *lock = file->private_data;

	if (lock)
		lock->file = NULL;

	return 0;
}

static const struct file_operations genlock_fops = {
	.release = genlock_release,
};


struct genlock *genlock_create_lock(struct genlock_handle *handle)
{
	struct genlock *lock;

	if (IS_ERR_OR_NULL(handle)) {
		GENLOCK_LOG_ERR("Invalid handle\n");
		return ERR_PTR(-EINVAL);
	}

	if (handle->lock != NULL) {
		GENLOCK_LOG_ERR("Handle already has a lock attached\n");
		return ERR_PTR(-EINVAL);
	}

	lock = kzalloc(sizeof(*lock), GFP_KERNEL);
	if (lock == NULL) {
		GENLOCK_LOG_ERR("Unable to allocate memory for a lock\n");
		return ERR_PTR(-ENOMEM);
	}

	INIT_LIST_HEAD(&lock->active);
	init_waitqueue_head(&lock->queue);
	spin_lock_init(&lock->lock);

	lock->magic = GENLOCK_MAGIC_OK;
	lock->state = _UNLOCKED;


	lock->file = anon_inode_getfile("genlock", &genlock_fops,
		lock, O_RDWR);

	
	handle->lock = lock;
	kref_init(&lock->refcount);

	return lock;
}
EXPORT_SYMBOL(genlock_create_lock);


static int genlock_get_fd(struct genlock *lock)
{
	int ret;

	if (!lock->file) {
		GENLOCK_LOG_ERR("No file attached to the lock\n");
		return -EINVAL;
	}

	ret = get_unused_fd_flags(0);
	if (ret < 0)
		return ret;
	fd_install(ret, lock->file);
	return ret;
}


struct genlock *genlock_attach_lock(struct genlock_handle *handle, int fd)
{
	struct file *file;
	struct genlock *lock;

	if (IS_ERR_OR_NULL(handle)) {
		GENLOCK_LOG_ERR("Invalid handle\n");
		return ERR_PTR(-EINVAL);
	}

	if (handle->lock != NULL) {
		GENLOCK_LOG_ERR("Handle already has a lock attached\n");
		return ERR_PTR(-EINVAL);
	}

	file = fget(fd);
	if (file == NULL) {
		GENLOCK_LOG_ERR("Bad file descriptor\n");
		return ERR_PTR(-EBADF);
	}


	spin_lock(&genlock_ref_lock);
	lock = file->private_data;

	fput(file);

	if (lock == NULL) {
		GENLOCK_LOG_ERR("File descriptor is invalid\n");
		goto fail_invalid;
	}

	if (lock->magic != GENLOCK_MAGIC_OK) {
		GENLOCK_LOG_ERR("Magic is invalid - 0x%X\n", lock->magic);
		goto fail_invalid;
	}

	handle->lock = lock;
	kref_get(&lock->refcount);
	spin_unlock(&genlock_ref_lock);

	return lock;

fail_invalid:
	spin_unlock(&genlock_ref_lock);
	return ERR_PTR(-EINVAL);
}
EXPORT_SYMBOL(genlock_attach_lock);


static int handle_has_lock(struct genlock *lock, struct genlock_handle *handle)
{
	struct genlock_handle *h;

	list_for_each_entry(h, &lock->active, entry) {
		if (h == handle)
			return 1;
	}

	return 0;
}


static void _genlock_signal(struct genlock *lock)
{
	if (list_empty(&lock->active)) {
		
		lock->state = _UNLOCKED;
		
		wake_up(&lock->queue);
	}
}


static int _genlock_unlock(struct genlock *lock, struct genlock_handle *handle)
{
	int ret = -EINVAL;
	unsigned long irqflags;

	spin_lock_irqsave(&lock->lock, irqflags);

	if (lock->state == _UNLOCKED) {
		GENLOCK_LOG_ERR("Trying to unlock an unlocked handle\n");
		goto done;
	}

	
	if (!handle_has_lock(lock, handle)) {
		GENLOCK_LOG_ERR("handle does not have lock attached to it\n");
		goto done;
	}

	if (--handle->active == 0) {
		list_del(&handle->entry);
		_genlock_signal(lock);
	}

	ret = 0;

done:
	spin_unlock_irqrestore(&lock->lock, irqflags);
	return ret;
}


static int _genlock_lock(struct genlock *lock, struct genlock_handle *handle,
	int op, int flags, uint32_t timeout)
{
	unsigned long irqflags;
	int ret = 0;
	unsigned long ticks = msecs_to_jiffies(timeout);

	spin_lock_irqsave(&lock->lock, irqflags);


	if (in_interrupt() && !(flags & GENLOCK_NOBLOCK))
		BUG();

	

	if (lock->state == _UNLOCKED)
		goto dolock;

	if (handle_has_lock(lock, handle)) {


		if (lock->state == _RDLOCK && op == _RDLOCK) {
			handle->active++;
			goto done;
		}


		if (flags & GENLOCK_WRITE_TO_READ) {
			if (lock->state == _WRLOCK && op == _RDLOCK) {
				lock->state = _RDLOCK;
				wake_up(&lock->queue);
				goto done;
			} else {
				GENLOCK_LOG_ERR("Invalid state to convert"
					"write to read\n");
				ret = -EINVAL;
				goto done;
			}
		}
	} else {


		if (flags & GENLOCK_WRITE_TO_READ) {
			GENLOCK_LOG_ERR("Handle must have lock to convert"
				"write to read\n");
			ret = -EINVAL;
			goto done;
		}


		if (op == GENLOCK_RDLOCK && lock->state == _RDLOCK)
			goto dolock;
	}


	if (flags & GENLOCK_NOBLOCK || timeout == 0) {
		ret = -EAGAIN;
		goto done;
	}


	while ((lock->state == _RDLOCK && op == _WRLOCK) ||
			lock->state == _WRLOCK) {
		signed long elapsed;

		spin_unlock_irqrestore(&lock->lock, irqflags);

		elapsed = wait_event_interruptible_timeout(lock->queue,
			lock->state == _UNLOCKED ||
			(lock->state == _RDLOCK && op == _RDLOCK),
			ticks);

		spin_lock_irqsave(&lock->lock, irqflags);

		if (elapsed <= 0) {
			if(list_empty(&lock->active)) {
				pr_info("[genlock] lock failed, but list_empty\n");
			} else {
				struct genlock_handle *h;
				pr_info("[genlock] lock failed %d, the follows hold lock %d\n", op, lock->state);
				list_for_each_entry(h, &lock->active, entry) {
					printk("[genlock] handle %p pid %d\n", h, h->info.pid);
				}
			}
			ret = (elapsed < 0) ? elapsed : -ETIMEDOUT;
			goto done;
		}

		ticks = (unsigned long) elapsed;
	}

dolock:
	

	list_add_tail(&handle->entry, &lock->active);
	lock->state = op;
	handle->active++;

done:
	spin_unlock_irqrestore(&lock->lock, irqflags);
	return ret;

}


int genlock_lock(struct genlock_handle *handle, int op, int flags,
	uint32_t timeout)
{
	struct genlock *lock;
	unsigned long irqflags;

	int ret = 0;

	if (IS_ERR_OR_NULL(handle)) {
		GENLOCK_LOG_ERR("Invalid handle\n");
		return -EINVAL;
	}

	lock = handle->lock;

	if (lock == NULL) {
		GENLOCK_LOG_ERR("Handle does not have a lock attached\n");
		return -EINVAL;
	}

	switch (op) {
	case GENLOCK_UNLOCK:
		ret = _genlock_unlock(lock, handle);
		break;
	case GENLOCK_RDLOCK:
		spin_lock_irqsave(&lock->lock, irqflags);
		if (handle_has_lock(lock, handle)) {
			
			flags |= GENLOCK_WRITE_TO_READ;
		}
		spin_unlock_irqrestore(&lock->lock, irqflags);
		
	case GENLOCK_WRLOCK:
		ret = _genlock_lock(lock, handle, op, flags, timeout);
		break;
	default:
		GENLOCK_LOG_ERR("Invalid lock operation\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}
EXPORT_SYMBOL(genlock_lock);


int genlock_dreadlock(struct genlock_handle *handle, int op, int flags,
	uint32_t timeout)
{
	struct genlock *lock;

	int ret = 0;

	if (IS_ERR_OR_NULL(handle)) {
		GENLOCK_LOG_ERR("Invalid handle\n");
		return -EINVAL;
	}

	lock = handle->lock;

	if (lock == NULL) {
		GENLOCK_LOG_ERR("Handle does not have a lock attached\n");
		return -EINVAL;
	}

	switch (op) {
	case GENLOCK_UNLOCK:
		ret = _genlock_unlock(lock, handle);
		break;
	case GENLOCK_RDLOCK:
	case GENLOCK_WRLOCK:
		ret = _genlock_lock(lock, handle, op, flags, timeout);
		break;
	default:
		GENLOCK_LOG_ERR("Invalid lock operation\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}
EXPORT_SYMBOL(genlock_dreadlock);


int genlock_wait(struct genlock_handle *handle, uint32_t timeout)
{
	struct genlock *lock;
	unsigned long irqflags;
	int ret = 0;
	unsigned long ticks = msecs_to_jiffies(timeout);

	if (IS_ERR_OR_NULL(handle)) {
		GENLOCK_LOG_ERR("Invalid handle\n");
		return -EINVAL;
	}

	lock = handle->lock;

	if (lock == NULL) {
		GENLOCK_LOG_ERR("Handle does not have a lock attached\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&lock->lock, irqflags);


	if (timeout == 0) {
		ret = (lock->state == _UNLOCKED) ? 0 : -EAGAIN;
		goto done;
	}

	while (lock->state != _UNLOCKED) {
		signed long elapsed;

		spin_unlock_irqrestore(&lock->lock, irqflags);

		elapsed = wait_event_interruptible_timeout(lock->queue,
			lock->state == _UNLOCKED, ticks);

		spin_lock_irqsave(&lock->lock, irqflags);

		if (elapsed <= 0) {
			ret = (elapsed < 0) ? elapsed : -ETIMEDOUT;
			break;
		}

		ticks = (unsigned long) elapsed;
	}

done:
	spin_unlock_irqrestore(&lock->lock, irqflags);
	return ret;
}

static void genlock_release_lock(struct genlock_handle *handle)
{
	unsigned long flags;

	if (handle == NULL || handle->lock == NULL)
		return;

	spin_lock_irqsave(&handle->lock->lock, flags);

	

	if (handle_has_lock(handle->lock, handle)) {
		list_del(&handle->entry);
		_genlock_signal(handle->lock);
	}
	spin_unlock_irqrestore(&handle->lock->lock, flags);

	spin_lock(&genlock_ref_lock);
	kref_put(&handle->lock->refcount, genlock_destroy);
	spin_unlock(&genlock_ref_lock);
	handle->lock = NULL;
	handle->active = 0;
}


static int genlock_handle_release(struct inode *inodep, struct file *file)
{
	struct genlock_handle *handle = file->private_data;

	genlock_release_lock(handle);
	kfree(handle);

	return 0;
}

static const struct file_operations genlock_handle_fops = {
	.release = genlock_handle_release
};


static struct genlock_handle *_genlock_get_handle(void)
{
	struct genlock_handle *handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (handle == NULL) {
		GENLOCK_LOG_ERR("Unable to allocate memory for the handle\n");
		return ERR_PTR(-ENOMEM);
	}

	return handle;
}


struct genlock_handle *genlock_get_handle(void)
{
	struct genlock_handle *handle = _genlock_get_handle();
	if (IS_ERR(handle))
		return handle;

	handle->file = anon_inode_getfile("genlock-handle",
		&genlock_handle_fops, handle, O_RDWR);

	return handle;
}
EXPORT_SYMBOL(genlock_get_handle);


void genlock_put_handle(struct genlock_handle *handle)
{
	if (handle)
		fput(handle->file);
}
EXPORT_SYMBOL(genlock_put_handle);


struct genlock_handle *genlock_get_handle_fd(int fd)
{
	struct file *file = fget(fd);

	if (file == NULL)
		return ERR_PTR(-EINVAL);

	return file->private_data;
}
EXPORT_SYMBOL(genlock_get_handle_fd);

#ifdef CONFIG_GENLOCK_MISCDEVICE

static long genlock_dev_ioctl(struct file *filep, unsigned int cmd,
	unsigned long arg)
{
	struct genlock_lock param;
	struct genlock_handle *handle = filep->private_data;
	struct genlock *lock;
	int ret;

	if (IS_ERR_OR_NULL(handle))
		return -EINVAL;

	switch (cmd) {
	case GENLOCK_IOC_NEW: {
		lock = genlock_create_lock(handle);
		if (IS_ERR(lock))
			return PTR_ERR(lock);

		return 0;
	}
	case GENLOCK_IOC_EXPORT: {
		if (handle->lock == NULL) {
			GENLOCK_LOG_ERR("Handle does not have a lock"
					"attached\n");
			return -EINVAL;
		}

		ret = genlock_get_fd(handle->lock);
		if (ret < 0)
			return ret;

		param.fd = ret;

		if (copy_to_user((void __user *) arg, &param,
			sizeof(param)))
			return -EFAULT;

		return 0;
		}
	case GENLOCK_IOC_ATTACH: {
		if (copy_from_user(&param, (void __user *) arg,
			sizeof(param)))
			return -EFAULT;

		lock = genlock_attach_lock(handle, param.fd);
		if (IS_ERR(lock))
			return PTR_ERR(lock);

		return 0;
	}
	case GENLOCK_IOC_LOCK: {
		if (copy_from_user(&param, (void __user *) arg,
		sizeof(param)))
			return -EFAULT;

		return genlock_lock(handle, param.op, param.flags,
			param.timeout);
	}
	case GENLOCK_IOC_DREADLOCK: {
		if (copy_from_user(&param, (void __user *) arg,
		sizeof(param)))
			return -EFAULT;

		return genlock_dreadlock(handle, param.op, param.flags,
			param.timeout);
	}
	case GENLOCK_IOC_WAIT: {
		if (copy_from_user(&param, (void __user *) arg,
		sizeof(param)))
			return -EFAULT;

		return genlock_wait(handle, param.timeout);
	}
	case GENLOCK_IOC_RELEASE: {
		GENLOCK_LOG_ERR("Deprecated RELEASE ioctl called\n");
		return -EINVAL;
	}
	case GENLOCK_IOC_SETINFO: {
		if (copy_from_user(&(handle->info), (void __user *) arg, sizeof(handle->info))) {
			GENLOCK_LOG_ERR("invalid param size");
			return -EINVAL;
		}
		return 0;
	}
	default:
		GENLOCK_LOG_ERR("Invalid ioctl\n");
		return -EINVAL;
	}
}

static int genlock_dev_release(struct inode *inodep, struct file *file)
{
	struct genlock_handle *handle = file->private_data;

	if (handle->info.fd) {
		char task_name[FIELD_SIZEOF(struct task_struct, comm) + 1];
		get_task_comm(task_name, current);

		pr_warn("[GENLOCK] Unexpected! genlock fd was closed before detached. "
		    "owner={fd=%d, pid:%d, res=[0x%x, 0x%x]} current thread = %d (%s)\n",
			handle->info.fd, handle->info.pid,
			handle->info.rsvd[0], handle->info.rsvd[1], current->pid, task_name);
	}

	genlock_release_lock(handle);
	kfree(handle);

	return 0;
}

static int genlock_dev_open(struct inode *inodep, struct file *file)
{
	struct genlock_handle *handle = _genlock_get_handle();
	if (IS_ERR(handle))
		return PTR_ERR(handle);

	handle->file = file;
	file->private_data = handle;
	return 0;
}

static const struct file_operations genlock_dev_fops = {
	.open = genlock_dev_open,
	.release = genlock_dev_release,
	.unlocked_ioctl = genlock_dev_ioctl,
};

static struct miscdevice genlock_dev;

static int genlock_dev_init(void)
{
	genlock_dev.minor = MISC_DYNAMIC_MINOR;
	genlock_dev.name = "genlock";
	genlock_dev.fops = &genlock_dev_fops;
	genlock_dev.parent = NULL;

	return misc_register(&genlock_dev);
}

static void genlock_dev_close(void)
{
	misc_deregister(&genlock_dev);
}

module_init(genlock_dev_init);
module_exit(genlock_dev_close);

#endif
