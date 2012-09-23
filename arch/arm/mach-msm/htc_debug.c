#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#define PROCNAME "driver/hdf"
#define FLAG_LEN 64
static char htc_debug_flag[FLAG_LEN];

int htc_debug_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len=FLAG_LEN+3;
	char R_Buffer[FLAG_LEN+3];

	memset(R_Buffer,0,FLAG_LEN+3);
	//printk(KERN_INFO "%s called\n", __func__);

	//printk(KERN_INFO "Read[off: %d count:%d]\n",(int)off,count);

	if (off > 0) {
		len = 0;
	} else {
		memcpy(R_Buffer,"0x",2);
		memcpy(R_Buffer+2,htc_debug_flag,FLAG_LEN);
		//printk(KERN_INFO "Flag  : %s\n",htc_debug_flag);
		//printk(KERN_INFO "Return: %s\n",R_Buffer);

		memcpy(page,R_Buffer,FLAG_LEN+3);
		//printk(KERN_INFO "User Buffer: %s\n",page);

	}
	//printk(KERN_INFO "len: %d\n",len);
	return len;
}

int htc_debug_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char buf[FLAG_LEN+3];

	//printk(KERN_INFO "%s called (len:%d)\n", __func__, (int)count);

	if (count != sizeof(buf))
		return -EFAULT;

	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	memcpy(htc_debug_flag,buf+2,FLAG_LEN);
	//printk(KERN_INFO"Receive :%s\n",buf);
	//printk(KERN_INFO"Flag    :%s\n",htc_debug_flag);
	return count;
}

static int __init htc_debug_init(void)
{
	struct proc_dir_entry *entry;

	entry = create_proc_entry(PROCNAME, 0660, NULL);
	if (entry == NULL) {
		//printk(KERN_ERR "htc_debug: unable to create /proc entry\n");
		return -ENOMEM;
	}

	entry->read_proc = htc_debug_read;
	entry->write_proc = htc_debug_write;

	//printk(KERN_INFO "htc_debug driver loaded\n");

	return 0;
}

static void __exit htc_debug_exit(void)
{
	remove_proc_entry(PROCNAME, NULL);

	//printk(KERN_INFO "htc_debug driver unloaded\n");
}

module_init(htc_debug_init);
module_exit(htc_debug_exit);

MODULE_DESCRIPTION("HTC Secure Debug Log Driver");
MODULE_LICENSE("GPL");
MODULE_LICENSE("HTC SSD ANDROID");
