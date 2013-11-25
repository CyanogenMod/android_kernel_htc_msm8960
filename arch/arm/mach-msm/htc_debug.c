#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/sched.h>


#define PROCNAME "driver/hdf"
#define FLAG_LEN 64
static char htc_debug_flag[FLAG_LEN];

#define TAG "[SEC] "
#undef PDEBUG
#define PDEBUG(fmt, args...) printk(KERN_INFO TAG "[K] %s(%i, %s): " fmt "\n", \
		__func__, current->pid, current->comm, ## args)

#undef PERR
#define PERR(fmt, args...) printk(KERN_ERR TAG "[E] %s(%i, %s): " fmt "\n", \
		__func__, current->pid, current->comm, ## args)

#undef PINFO
#define PINFO(fmt, args...) printk(KERN_INFO TAG "[I] %s(%i, %s): " fmt "\n", \
		__func__, current->pid, current->comm, ## args)

int htc_debug_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len=FLAG_LEN+3;
	char R_Buffer[FLAG_LEN+3];


	memset(R_Buffer,0,FLAG_LEN+3);
	

	

	if (off > 0) {
		len = 0;
	} else {
		memcpy(R_Buffer,"0x",2);
		memcpy(R_Buffer+2,htc_debug_flag,FLAG_LEN);
		
		

		memcpy(page,R_Buffer,FLAG_LEN+3);
		

	}
	
	return len;
}

int htc_debug_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char buf[FLAG_LEN+3];


	

	if (count != sizeof(buf))
		return -EFAULT;

	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	memcpy(htc_debug_flag,buf+2,FLAG_LEN);
	
	
	return count;
}

static int __init htc_debug_init(void)
{
	struct proc_dir_entry *entry;

	entry = create_proc_entry(PROCNAME, 0660, NULL);
	if (entry == NULL) {
		PERR("unable to create /proc entry");
		return -ENOMEM;
	}

	entry->read_proc = htc_debug_read;
	entry->write_proc = htc_debug_write;

	

	return 0;
}

static void __exit htc_debug_exit(void)
{
	remove_proc_entry(PROCNAME, NULL);

	
}

module_init(htc_debug_init);
module_exit(htc_debug_exit);

MODULE_DESCRIPTION("HTC Secure Debug Log Driver");
MODULE_LICENSE("GPL");
MODULE_LICENSE("HTC SSD ANDROID");
