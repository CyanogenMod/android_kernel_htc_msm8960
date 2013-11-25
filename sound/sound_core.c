
#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/kdev_t.h>
#include <linux/major.h>
#include <sound/core.h>

#ifdef CONFIG_SOUND_OSS_CORE
static int __init init_oss_soundcore(void);
static void cleanup_oss_soundcore(void);
#else
static inline int init_oss_soundcore(void)	{ return 0; }
static inline void cleanup_oss_soundcore(void)	{ }
#endif

struct class *sound_class;
EXPORT_SYMBOL(sound_class);

MODULE_DESCRIPTION("Core sound module");
MODULE_AUTHOR("Alan Cox");
MODULE_LICENSE("GPL");

static char *sound_devnode(struct device *dev, umode_t *mode)
{
	if (MAJOR(dev->devt) == SOUND_MAJOR)
		return NULL;
	return kasprintf(GFP_KERNEL, "snd/%s", dev_name(dev));
}

static int __init init_soundcore(void)
{
	int rc;

	rc = init_oss_soundcore();
	if (rc)
		return rc;

	sound_class = class_create(THIS_MODULE, "sound");
	if (IS_ERR(sound_class)) {
		cleanup_oss_soundcore();
		return PTR_ERR(sound_class);
	}

	sound_class->devnode = sound_devnode;

	return 0;
}

static void __exit cleanup_soundcore(void)
{
	cleanup_oss_soundcore();
	class_destroy(sound_class);
}

subsys_initcall(init_soundcore);
module_exit(cleanup_soundcore);


#ifdef CONFIG_SOUND_OSS_CORE
/*
 *	OSS sound core handling. Breaks out sound functions to submodules
 *	
 *	Author:		Alan Cox <alan@lxorguk.ukuu.org.uk>
 *
 *	Fixes:
 *
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *                         --------------------
 * 
 *	Top level handler for the sound subsystem. Various devices can
 *	plug into this. The fact they don't all go via OSS doesn't mean 
 *	they don't have to implement the OSS API. There is a lot of logic
 *	to keeping much of the OSS weight out of the code in a compatibility
 *	module, but it's up to the driver to rember to load it...
 *
 *	The code provides a set of functions for registration of devices
 *	by type. This is done rather than providing a single call so that
 *	we can hide any future changes in the internals (eg when we go to
 *	32bit dev_t) from the modules and their interface.
 *
 *	Secondly we need to allocate the dsp, dsp16 and audio devices as
 *	one. Thus we misuse the chains a bit to simplify this.
 *
 *	Thirdly to make it more fun and for 2.3.x and above we do all
 *	of this using fine grained locking.
 *
 *	FIXME: we have to resolve modules and fine grained load/unload
 *	locking at some point in 2.3.x.
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sound.h>
#include <linux/kmod.h>

#define SOUND_STEP 16

struct sound_unit
{
	int unit_minor;
	const struct file_operations *unit_fops;
	struct sound_unit *next;
	char name[32];
};

#ifdef CONFIG_SOUND_MSNDCLAS
extern int msnd_classic_init(void);
#endif
#ifdef CONFIG_SOUND_MSNDPIN
extern int msnd_pinnacle_init(void);
#endif

#ifdef CONFIG_SOUND_OSS_CORE_PRECLAIM
static int preclaim_oss = 1;
#else
static int preclaim_oss = 0;
#endif

module_param(preclaim_oss, int, 0444);

static int soundcore_open(struct inode *, struct file *);

static const struct file_operations soundcore_fops =
{
	
	.owner	= THIS_MODULE,
	.open	= soundcore_open,
	.llseek = noop_llseek,
};


static int __sound_insert_unit(struct sound_unit * s, struct sound_unit **list, const struct file_operations *fops, int index, int low, int top)
{
	int n=low;

	if (index < 0) {	

		while (*list && (*list)->unit_minor<n)
			list=&((*list)->next);

		while(n<top)
		{
			
			if(*list==NULL || (*list)->unit_minor>n)
				break;
			list=&((*list)->next);
			n+=SOUND_STEP;
		}

		if(n>=top)
			return -ENOENT;
	} else {
		n = low+(index*16);
		while (*list) {
			if ((*list)->unit_minor==n)
				return -EBUSY;
			if ((*list)->unit_minor>n)
				break;
			list=&((*list)->next);
		}
	}	
		
	 
	s->unit_minor=n;
	s->unit_fops=fops;
	
	 
	s->next=*list;
	*list=s;
	
	
	return n;
}

 
static struct sound_unit *__sound_remove_unit(struct sound_unit **list, int unit)
{
	while(*list)
	{
		struct sound_unit *p=*list;
		if(p->unit_minor==unit)
		{
			*list=p->next;
			return p;
		}
		list=&(p->next);
	}
	printk(KERN_ERR "Sound device %d went missing!\n", unit);
	return NULL;
}


static DEFINE_SPINLOCK(sound_loader_lock);


static int sound_insert_unit(struct sound_unit **list, const struct file_operations *fops, int index, int low, int top, const char *name, umode_t mode, struct device *dev)
{
	struct sound_unit *s = kmalloc(sizeof(*s), GFP_KERNEL);
	int r;

	if (!s)
		return -ENOMEM;

	spin_lock(&sound_loader_lock);
retry:
	r = __sound_insert_unit(s, list, fops, index, low, top);
	spin_unlock(&sound_loader_lock);
	
	if (r < 0)
		goto fail;
	else if (r < SOUND_STEP)
		sprintf(s->name, "sound/%s", name);
	else
		sprintf(s->name, "sound/%s%d", name, r / SOUND_STEP);

	if (!preclaim_oss) {
		r = __register_chrdev(SOUND_MAJOR, s->unit_minor, 1, s->name,
				      &soundcore_fops);
		if (r < 0) {
			spin_lock(&sound_loader_lock);
			__sound_remove_unit(list, s->unit_minor);
			if (index < 0) {
				low = s->unit_minor + SOUND_STEP;
				goto retry;
			}
			spin_unlock(&sound_loader_lock);
			return -EBUSY;
		}
	}

	device_create(sound_class, dev, MKDEV(SOUND_MAJOR, s->unit_minor),
		      NULL, s->name+6);
	return s->unit_minor;

fail:
	kfree(s);
	return r;
}

 	
static void sound_remove_unit(struct sound_unit **list, int unit)
{
	struct sound_unit *p;

	spin_lock(&sound_loader_lock);
	p = __sound_remove_unit(list, unit);
	spin_unlock(&sound_loader_lock);
	if (p) {
		if (!preclaim_oss)
			__unregister_chrdev(SOUND_MAJOR, p->unit_minor, 1,
					    p->name);
		device_destroy(sound_class, MKDEV(SOUND_MAJOR, p->unit_minor));
		kfree(p);
	}
}


static struct sound_unit *chains[SOUND_STEP];

 
int register_sound_special_device(const struct file_operations *fops, int unit,
				  struct device *dev)
{
	const int chain = unit % SOUND_STEP;
	int max_unit = 128 + chain;
	const char *name;
	char _name[16];

	switch (chain) {
	    case 0:
		name = "mixer";
		break;
	    case 1:
		name = "sequencer";
		if (unit >= SOUND_STEP)
			goto __unknown;
		max_unit = unit + 1;
		break;
	    case 2:
		name = "midi";
		break;
	    case 3:
		name = "dsp";
		break;
	    case 4:
		name = "audio";
		break;
	    case 5:
		name = "dspW";
		break;
	    case 8:
		name = "sequencer2";
		if (unit >= SOUND_STEP)
			goto __unknown;
		max_unit = unit + 1;
		break;
	    case 9:
		name = "dmmidi";
		break;
	    case 10:
		name = "dmfm";
		break;
	    case 12:
		name = "adsp";
		break;
	    case 13:
		name = "amidi";
		break;
	    case 14:
		name = "admmidi";
		break;
	    default:
	    	{
		    __unknown:
			sprintf(_name, "unknown%d", chain);
		    	if (unit >= SOUND_STEP)
		    		strcat(_name, "-");
		    	name = _name;
		}
		break;
	}
	return sound_insert_unit(&chains[chain], fops, -1, unit, max_unit,
				 name, S_IRUSR | S_IWUSR, dev);
}
 
EXPORT_SYMBOL(register_sound_special_device);

int register_sound_special(const struct file_operations *fops, int unit)
{
	return register_sound_special_device(fops, unit, NULL);
}

EXPORT_SYMBOL(register_sound_special);


int register_sound_mixer(const struct file_operations *fops, int dev)
{
	return sound_insert_unit(&chains[0], fops, dev, 0, 128,
				 "mixer", S_IRUSR | S_IWUSR, NULL);
}

EXPORT_SYMBOL(register_sound_mixer);


int register_sound_midi(const struct file_operations *fops, int dev)
{
	return sound_insert_unit(&chains[2], fops, dev, 2, 130,
				 "midi", S_IRUSR | S_IWUSR, NULL);
}

EXPORT_SYMBOL(register_sound_midi);

 

int register_sound_dsp(const struct file_operations *fops, int dev)
{
	return sound_insert_unit(&chains[3], fops, dev, 3, 131,
				 "dsp", S_IWUSR | S_IRUSR, NULL);
}

EXPORT_SYMBOL(register_sound_dsp);



void unregister_sound_special(int unit)
{
	sound_remove_unit(&chains[unit % SOUND_STEP], unit);
}
 
EXPORT_SYMBOL(unregister_sound_special);


void unregister_sound_mixer(int unit)
{
	sound_remove_unit(&chains[0], unit);
}

EXPORT_SYMBOL(unregister_sound_mixer);


void unregister_sound_midi(int unit)
{
	sound_remove_unit(&chains[2], unit);
}

EXPORT_SYMBOL(unregister_sound_midi);


void unregister_sound_dsp(int unit)
{
	sound_remove_unit(&chains[3], unit);
}


EXPORT_SYMBOL(unregister_sound_dsp);

static struct sound_unit *__look_for_unit(int chain, int unit)
{
	struct sound_unit *s;
	
	s=chains[chain];
	while(s && s->unit_minor <= unit)
	{
		if(s->unit_minor==unit)
			return s;
		s=s->next;
	}
	return NULL;
}

static int soundcore_open(struct inode *inode, struct file *file)
{
	int chain;
	int unit = iminor(inode);
	struct sound_unit *s;
	const struct file_operations *new_fops = NULL;

	chain=unit&0x0F;
	if(chain==4 || chain==5)	
	{
		unit&=0xF0;
		unit|=3;
		chain=3;
	}
	
	spin_lock(&sound_loader_lock);
	s = __look_for_unit(chain, unit);
	if (s)
		new_fops = fops_get(s->unit_fops);
	if (preclaim_oss && !new_fops) {
		spin_unlock(&sound_loader_lock);

		request_module("sound-slot-%i", unit>>4);
		request_module("sound-service-%i-%i", unit>>4, chain);

		if (request_module("char-major-%d-%d", SOUND_MAJOR, unit) > 0)
			request_module("char-major-%d", SOUND_MAJOR);

		spin_lock(&sound_loader_lock);
		s = __look_for_unit(chain, unit);
		if (s)
			new_fops = fops_get(s->unit_fops);
	}
	if (new_fops) {
		int err = 0;
		const struct file_operations *old_fops = file->f_op;
		file->f_op = new_fops;
		spin_unlock(&sound_loader_lock);

		if (file->f_op->open)
			err = file->f_op->open(inode,file);

		if (err) {
			fops_put(file->f_op);
			file->f_op = fops_get(old_fops);
		}

		fops_put(old_fops);
		return err;
	}
	spin_unlock(&sound_loader_lock);
	return -ENODEV;
}

MODULE_ALIAS_CHARDEV_MAJOR(SOUND_MAJOR);

static void cleanup_oss_soundcore(void)
{
	unregister_chrdev(SOUND_MAJOR, "sound");
}

static int __init init_oss_soundcore(void)
{
	if (preclaim_oss &&
	    register_chrdev(SOUND_MAJOR, "sound", &soundcore_fops) == -1) {
		printk(KERN_ERR "soundcore: sound device already in use.\n");
		return -EBUSY;
	}

	return 0;
}

#endif 
