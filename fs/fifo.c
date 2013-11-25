/*
 *  linux/fs/fifo.c
 *
 *  written by Paul H. Hargrove
 *
 *  Fixes:
 *	10-06-1999, AV: fixed OOM handling in fifo_open(), moved
 *			initialization there, switched to external
 *			allocation of pipe_inode_info.
 */

#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/pipe_fs_i.h>

static int wait_for_partner(struct inode* inode, unsigned int *cnt)
{
	int cur = *cnt;	

	while (cur == *cnt) {
		pipe_wait(inode->i_pipe);
		if (signal_pending(current))
			break;
	}
	return cur == *cnt ? -ERESTARTSYS : 0;
}

static void wake_up_partner(struct inode* inode)
{
	wake_up_interruptible(&inode->i_pipe->wait);
}

static int fifo_open(struct inode *inode, struct file *filp)
{
	struct pipe_inode_info *pipe;
	int ret;

	mutex_lock(&inode->i_mutex);
	pipe = inode->i_pipe;
	if (!pipe) {
		ret = -ENOMEM;
		pipe = alloc_pipe_info(inode);
		if (!pipe)
			goto err_nocleanup;
		inode->i_pipe = pipe;
	}
	filp->f_version = 0;

	
	filp->f_mode &= (FMODE_READ | FMODE_WRITE);

	switch (filp->f_mode) {
	case FMODE_READ:
		filp->f_op = &read_pipefifo_fops;
		pipe->r_counter++;
		if (pipe->readers++ == 0)
			wake_up_partner(inode);

		if (!pipe->writers) {
			if ((filp->f_flags & O_NONBLOCK)) {
				filp->f_version = pipe->w_counter;
			} else {
				if (wait_for_partner(inode, &pipe->w_counter))
					goto err_rd;
			}
		}
		break;
	
	case FMODE_WRITE:
		ret = -ENXIO;
		if ((filp->f_flags & O_NONBLOCK) && !pipe->readers)
			goto err;

		filp->f_op = &write_pipefifo_fops;
		pipe->w_counter++;
		if (!pipe->writers++)
			wake_up_partner(inode);

		if (!pipe->readers) {
			if (wait_for_partner(inode, &pipe->r_counter))
				goto err_wr;
		}
		break;
	
	case FMODE_READ | FMODE_WRITE:
		filp->f_op = &rdwr_pipefifo_fops;

		pipe->readers++;
		pipe->writers++;
		pipe->r_counter++;
		pipe->w_counter++;
		if (pipe->readers == 1 || pipe->writers == 1)
			wake_up_partner(inode);
		break;

	default:
		ret = -EINVAL;
		goto err;
	}

	
	mutex_unlock(&inode->i_mutex);
	return 0;

err_rd:
	if (!--pipe->readers)
		wake_up_interruptible(&pipe->wait);
	ret = -ERESTARTSYS;
	goto err;

err_wr:
	if (!--pipe->writers)
		wake_up_interruptible(&pipe->wait);
	ret = -ERESTARTSYS;
	goto err;

err:
	if (!pipe->readers && !pipe->writers)
		free_pipe_info(inode);

err_nocleanup:
	mutex_unlock(&inode->i_mutex);
	return ret;
}

const struct file_operations def_fifo_fops = {
	.open		= fifo_open,	
	.llseek		= noop_llseek,
};
