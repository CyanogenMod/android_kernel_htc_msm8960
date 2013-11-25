/*
 * Copyright (c) 2011 Synaptics Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/syscalls.h>
#include <linux/i2c.h>
#include <linux/rmi.h>

#define CHAR_DEVICE_NAME "rmi"

#define REG_ADDR_LIMIT 0xFFFF

static int rmi_char_dev_major_num;



static loff_t rmi_char_dev_llseek(struct file *filp, loff_t off, int whence)
{
	loff_t newpos;
	struct rmi_char_dev *my_char_dev = filp->private_data;

	if (IS_ERR(my_char_dev)) {
		pr_err("%s: pointer of char device is invalid", __func__);
		return -EBADF;
	}

	mutex_lock(&(my_char_dev->mutex_file_op));

	switch (whence) {
	case SEEK_SET:
		newpos = off;
		break;

	case SEEK_CUR:
		newpos = filp->f_pos + off;
		break;

	case SEEK_END:
		newpos = REG_ADDR_LIMIT + off;
		break;

	default:		
		newpos = -EINVAL;
		goto clean_up;
	}

	if (newpos < 0 || newpos > REG_ADDR_LIMIT) {
		dev_err(my_char_dev->phys->dev, "newpos 0x%04x is invalid.\n",
			(unsigned int)newpos);
		newpos = -EINVAL;
		goto clean_up;
	}

	filp->f_pos = newpos;

clean_up:
	mutex_unlock(&(my_char_dev->mutex_file_op));
	return newpos;
}

static ssize_t rmi_char_dev_read(struct file *filp, char __user *buf,
		size_t count, loff_t *f_pos)
{
	struct rmi_char_dev *my_char_dev = filp->private_data;
	ssize_t ret_value  = 0;
	unsigned char tmpbuf[count+1];
	

	
	if (count > (REG_ADDR_LIMIT - *f_pos))
		count = REG_ADDR_LIMIT - *f_pos;

	if (count == 0)
		return 0;

	if (IS_ERR(my_char_dev)) {
		pr_err("%s: pointer of char device is invalid", __func__);
		ret_value = -EBADF;
		return ret_value;
	}

	mutex_lock(&(my_char_dev->mutex_file_op));

	

	
	ret_value = i2c_rmi_read(*f_pos, tmpbuf, count);


	if (ret_value < 0)
		goto clean_up;
	else
		*f_pos += ret_value;

	if (copy_to_user(buf, tmpbuf, count))
		ret_value = -EFAULT;

clean_up:

	mutex_unlock(&(my_char_dev->mutex_file_op));

	return ret_value;
}

/*
 * rmi_char_dev_write: - use to write data into RMI stream
 *
 * @filep : file structure for write
 * @buf: user-level buffer pointer contains data to be written
 * @count: number of byte be be written
 * @f_pos: offset (starting register address)
 *
 * @return number of bytes written from user buffer (buf) if succeeds
 *         negative number if error occurs.
 */
static ssize_t rmi_char_dev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	struct rmi_char_dev *my_char_dev = filp->private_data;
	ssize_t ret_value  = 0;
	unsigned char tmpbuf[count+1];
	

	
	if (count > (REG_ADDR_LIMIT - *f_pos))
		count = REG_ADDR_LIMIT - *f_pos;

	if (count == 0)
		return 0;

	if (IS_ERR(my_char_dev)) {
		pr_err("%s: pointer of char device is invalid", __func__);
		ret_value = -EBADF;
		return ret_value;
	}

	if (copy_from_user(tmpbuf, buf, count)) {
		ret_value = -EFAULT;
		return ret_value;
	}

	mutex_lock(&(my_char_dev->mutex_file_op));

	

	
	ret_value = i2c_rmi_write(*f_pos, tmpbuf, count);

	if (ret_value >= 0)
		*f_pos += count;

	mutex_unlock(&(my_char_dev->mutex_file_op));

	return ret_value;
}

static int rmi_char_dev_open(struct inode *inp, struct file *filp)
{
	
	struct rmi_char_dev *my_dev = container_of(inp->i_cdev,
			struct rmi_char_dev, main_dev);
	
	int ret_value = 0;

	filp->private_data = my_dev;

	
		

	mutex_lock(&(my_dev->mutex_file_op));
	if (my_dev->ref_count < 1)
		my_dev->ref_count++;
	else
		ret_value = -EACCES;

	mutex_unlock(&(my_dev->mutex_file_op));

	return ret_value;
}

static int rmi_char_dev_release(struct inode *inp, struct file *filp)
{
	struct rmi_char_dev *my_dev = container_of(inp->i_cdev,
			struct rmi_char_dev, main_dev);

	mutex_lock(&(my_dev->mutex_file_op));

	my_dev->ref_count--;
	if (my_dev->ref_count < 0)
		my_dev->ref_count = 0;

	mutex_unlock(&(my_dev->mutex_file_op));

	return 0;
}

static const struct file_operations rmi_char_dev_fops = {
	.owner =    THIS_MODULE,
	.llseek =   rmi_char_dev_llseek,
	.read =     rmi_char_dev_read,
	.write =    rmi_char_dev_write,
	.open =     rmi_char_dev_open,
	.release =  rmi_char_dev_release,
};

static void rmi_char_dev_clean_up(struct rmi_char_dev *char_dev,
			struct class *char_device_class)
{
	dev_t devno;

	
	if (char_dev) {
		devno = char_dev->main_dev.dev;

		cdev_del(&char_dev->main_dev);
		kfree(char_dev);

		if (char_device_class) {
			device_destroy(char_device_class, devno);
			class_unregister(char_device_class);
			class_destroy(char_device_class);
		}

		
		unregister_chrdev_region(devno, 1);
		pr_debug("%s: rmi_char_dev is removed\n", __func__);
	}
}

static char *rmi_char_devnode(struct device *dev, mode_t *mode)
{
	if (!mode)
		return NULL;
	
	
	*mode = (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);

	return kasprintf(GFP_KERNEL, "rmi/%s", dev_name(dev));
}

int rmi_char_dev_register(void)
{
	struct rmi_char_dev *char_dev;
	dev_t dev_no;
	int err;
	int result;
	struct device *device_ptr;
	struct class *rmi_char_device_class;

	if (rmi_char_dev_major_num) {
		dev_no = MKDEV(rmi_char_dev_major_num, 0);
		result = register_chrdev_region(dev_no, 1, CHAR_DEVICE_NAME);
	} else {
		result = alloc_chrdev_region(&dev_no, 0, 1, CHAR_DEVICE_NAME);
		
		rmi_char_dev_major_num = MAJOR(dev_no);
		printk(KERN_ERR "Major number of rmi_char_dev: %d\n",
				 rmi_char_dev_major_num);
	}
	if (result < 0)
		return result;

	char_dev = kzalloc(sizeof(struct rmi_char_dev), GFP_KERNEL);
	if (!char_dev) {
		printk(KERN_ERR "Failed to allocate rmi_char_dev.\n");
		
		__unregister_chrdev(rmi_char_dev_major_num, MINOR(dev_no), 1,
				CHAR_DEVICE_NAME);
		return -ENOMEM;
	}

	mutex_init(&char_dev->mutex_file_op);


	cdev_init(&char_dev->main_dev, &rmi_char_dev_fops);

	err = cdev_add(&char_dev->main_dev, dev_no, 1);
	if (err) {
		printk(KERN_ERR "Error %d adding rmi_char_dev.\n", err);
		
		
		return err;
	}

	
	rmi_char_device_class =
		class_create(THIS_MODULE, CHAR_DEVICE_NAME);

	if (IS_ERR(rmi_char_device_class)) {
		printk(KERN_INFO "Failed to create /dev/%s.\n",
			CHAR_DEVICE_NAME);
		rmi_char_dev_clean_up(char_dev,
				 rmi_char_device_class);
		return -ENODEV;
	}
	
	rmi_char_device_class->devnode = rmi_char_devnode;

	
	device_ptr = device_create(
			rmi_char_device_class,
			NULL, dev_no, NULL,
			CHAR_DEVICE_NAME"%d",
			MINOR(dev_no));

	if (IS_ERR(device_ptr)) {
		printk(KERN_ERR "Failed to create rmi device.\n");
		rmi_char_dev_clean_up(char_dev,
				 rmi_char_device_class);
		return -ENODEV;
	}

	return 0;
}
EXPORT_SYMBOL(rmi_char_dev_register);


void rmi_char_dev_unregister(struct rmi_phys_device *phys)
{
	
	if (phys)
		rmi_char_dev_clean_up(phys->char_dev,
				 phys->rmi_char_device_class);
}
EXPORT_SYMBOL(rmi_char_dev_unregister);

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("RMI4 Char Device");
MODULE_LICENSE("GPL");
