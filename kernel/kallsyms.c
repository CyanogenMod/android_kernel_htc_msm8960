/*
 * kallsyms.c: in-kernel printing of symbolic oopses and stack traces.
 *
 * Rewritten and vastly simplified by Rusty Russell for in-kernel
 * module loader:
 *   Copyright 2002 Rusty Russell <rusty@rustcorp.com.au> IBM Corporation
 *
 * ChangeLog:
 *
 * (25/Aug/2004) Paulo Marques <pmarques@grupopie.com>
 *      Changed the compression method from stem compression to "table lookup"
 *      compression (see scripts/kallsyms.c for a more complete description)
 */
#include <linux/kallsyms.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/kdb.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>	
#include <linux/mm.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <mach/msm_iomap.h>

#include <asm/sections.h>

#ifdef CONFIG_KALLSYMS_ALL
#define all_var 1
#else
#define all_var 0
#endif

extern const unsigned long kallsyms_addresses[] __attribute__((weak));
extern const u8 kallsyms_names[] __attribute__((weak));

extern const unsigned long kallsyms_num_syms
__attribute__((weak, section(".rodata")));

extern const u8 kallsyms_token_table[] __attribute__((weak));
extern const u16 kallsyms_token_index[] __attribute__((weak));

extern const unsigned long kallsyms_markers[] __attribute__((weak));

static inline int is_kernel_inittext(unsigned long addr)
{
	if (addr >= (unsigned long)_sinittext
	    && addr <= (unsigned long)_einittext)
		return 1;
	return 0;
}

static inline int is_kernel_text(unsigned long addr)
{
	if ((addr >= (unsigned long)_stext && addr <= (unsigned long)_etext) ||
	    arch_is_kernel_text(addr))
		return 1;
	return in_gate_area_no_mm(addr);
}

static inline int is_kernel(unsigned long addr)
{
	if (addr >= (unsigned long)_stext && addr <= (unsigned long)_end)
		return 1;
	return in_gate_area_no_mm(addr);
}

static int is_ksym_addr(unsigned long addr)
{
	if (all_var)
		return is_kernel(addr);

	return is_kernel_text(addr) || is_kernel_inittext(addr);
}

static unsigned int kallsyms_expand_symbol(unsigned int off, char *result)
{
	int len, skipped_first = 0;
	const u8 *tptr, *data;

	
	data = &kallsyms_names[off];
	len = *data;
	data++;

	off += len + 1;

	while (len) {
		tptr = &kallsyms_token_table[kallsyms_token_index[*data]];
		data++;
		len--;

		while (*tptr) {
			if (skipped_first) {
				*result = *tptr;
				result++;
			} else
				skipped_first = 1;
			tptr++;
		}
	}

	*result = '\0';

	
	return off;
}

static char kallsyms_get_symbol_type(unsigned int off)
{
	return kallsyms_token_table[kallsyms_token_index[kallsyms_names[off + 1]]];
}


static unsigned int get_symbol_offset(unsigned long pos)
{
	const u8 *name;
	int i;

	name = &kallsyms_names[kallsyms_markers[pos >> 8]];

	for (i = 0; i < (pos & 0xFF); i++)
		name = name + (*name) + 1;

	return name - kallsyms_names;
}

unsigned long kallsyms_lookup_name(const char *name)
{
	char namebuf[KSYM_NAME_LEN];
	unsigned long i;
	unsigned int off;

	for (i = 0, off = 0; i < kallsyms_num_syms; i++) {
		off = kallsyms_expand_symbol(off, namebuf);

		if (strcmp(namebuf, name) == 0)
			return kallsyms_addresses[i];
	}
	return module_kallsyms_lookup_name(name);
}
EXPORT_SYMBOL_GPL(kallsyms_lookup_name);

int kallsyms_on_each_symbol(int (*fn)(void *, const char *, struct module *,
				      unsigned long),
			    void *data)
{
	char namebuf[KSYM_NAME_LEN];
	unsigned long i;
	unsigned int off;
	int ret;

	for (i = 0, off = 0; i < kallsyms_num_syms; i++) {
		off = kallsyms_expand_symbol(off, namebuf);
		ret = fn(data, namebuf, NULL, kallsyms_addresses[i]);
		if (ret != 0)
			return ret;
	}
	return module_kallsyms_on_each_symbol(fn, data);
}
EXPORT_SYMBOL_GPL(kallsyms_on_each_symbol);

static unsigned long get_symbol_pos(unsigned long addr,
				    unsigned long *symbolsize,
				    unsigned long *offset)
{
	unsigned long symbol_start = 0, symbol_end = 0;
	unsigned long i, low, high, mid;

	
	BUG_ON(!kallsyms_addresses);

	
	low = 0;
	high = kallsyms_num_syms;

	while (high - low > 1) {
		mid = low + (high - low) / 2;
		if (kallsyms_addresses[mid] <= addr)
			low = mid;
		else
			high = mid;
	}

	while (low && kallsyms_addresses[low-1] == kallsyms_addresses[low])
		--low;

	symbol_start = kallsyms_addresses[low];

	
	for (i = low + 1; i < kallsyms_num_syms; i++) {
		if (kallsyms_addresses[i] > symbol_start) {
			symbol_end = kallsyms_addresses[i];
			break;
		}
	}

	
	if (!symbol_end) {
		if (is_kernel_inittext(addr))
			symbol_end = (unsigned long)_einittext;
		else if (all_var)
			symbol_end = (unsigned long)_end;
		else
			symbol_end = (unsigned long)_etext;
	}

	if (symbolsize)
		*symbolsize = symbol_end - symbol_start;
	if (offset)
		*offset = addr - symbol_start;

	return low;
}

int kallsyms_lookup_size_offset(unsigned long addr, unsigned long *symbolsize,
				unsigned long *offset)
{
	char namebuf[KSYM_NAME_LEN];
	if (is_ksym_addr(addr))
		return !!get_symbol_pos(addr, symbolsize, offset);

	return !!module_address_lookup(addr, symbolsize, offset, NULL, namebuf);
}

const char *kallsyms_lookup(unsigned long addr,
			    unsigned long *symbolsize,
			    unsigned long *offset,
			    char **modname, char *namebuf)
{
	namebuf[KSYM_NAME_LEN - 1] = 0;
	namebuf[0] = 0;

	if (is_ksym_addr(addr)) {
		unsigned long pos;

		pos = get_symbol_pos(addr, symbolsize, offset);
		
		kallsyms_expand_symbol(get_symbol_offset(pos), namebuf);
		if (modname)
			*modname = NULL;
		return namebuf;
	}

	
	return module_address_lookup(addr, symbolsize, offset, modname,
				     namebuf);
}

int lookup_symbol_name(unsigned long addr, char *symname)
{
	symname[0] = '\0';
	symname[KSYM_NAME_LEN - 1] = '\0';

	if (is_ksym_addr(addr)) {
		unsigned long pos;

		pos = get_symbol_pos(addr, NULL, NULL);
		
		kallsyms_expand_symbol(get_symbol_offset(pos), symname);
		return 0;
	}
	
	return lookup_module_symbol_name(addr, symname);
}

int lookup_symbol_attrs(unsigned long addr, unsigned long *size,
			unsigned long *offset, char *modname, char *name)
{
	name[0] = '\0';
	name[KSYM_NAME_LEN - 1] = '\0';

	if (is_ksym_addr(addr)) {
		unsigned long pos;

		pos = get_symbol_pos(addr, size, offset);
		
		kallsyms_expand_symbol(get_symbol_offset(pos), name);
		modname[0] = '\0';
		return 0;
	}
	
	return lookup_module_symbol_attrs(addr, size, offset, modname, name);
}

static int __sprint_symbol(char *buffer, unsigned long address,
			   int symbol_offset, int add_offset)
{
	char *modname;
	const char *name;
	unsigned long offset, size;
	int len;

	address += symbol_offset;
	name = kallsyms_lookup(address, &size, &offset, &modname, buffer);
	if (!name)
		return sprintf(buffer, "0x%lx", address);

	if (name != buffer)
		strcpy(buffer, name);
	len = strlen(buffer);
	offset -= symbol_offset;

	if (add_offset)
		len += sprintf(buffer + len, "+%#lx/%#lx", offset, size);

	if (modname)
		len += sprintf(buffer + len, " [%s]", modname);

	return len;
}

int sprint_symbol(char *buffer, unsigned long address)
{
	return __sprint_symbol(buffer, address, 0, 1);
}
EXPORT_SYMBOL_GPL(sprint_symbol);

int sprint_symbol_no_offset(char *buffer, unsigned long address)
{
	return __sprint_symbol(buffer, address, 0, 0);
}
EXPORT_SYMBOL_GPL(sprint_symbol_no_offset);

int sprint_backtrace(char *buffer, unsigned long address)
{
	return __sprint_symbol(buffer, address, -1, 1);
}

void __print_symbol(const char *fmt, unsigned long address)
{
	char buffer[KSYM_SYMBOL_LEN];

	sprint_symbol(buffer, address);

	printk(fmt, buffer);
}
EXPORT_SYMBOL(__print_symbol);

struct kallsym_iter {
	loff_t pos;
	unsigned long value;
	unsigned int nameoff; 
	char type;
	char name[KSYM_NAME_LEN];
	char module_name[MODULE_NAME_LEN];
	int exported;
};

static int get_ksymbol_mod(struct kallsym_iter *iter)
{
	if (module_get_kallsym(iter->pos - kallsyms_num_syms, &iter->value,
				&iter->type, iter->name, iter->module_name,
				&iter->exported) < 0)
		return 0;
	return 1;
}

static unsigned long get_ksymbol_core(struct kallsym_iter *iter)
{
	unsigned off = iter->nameoff;

	iter->module_name[0] = '\0';
	iter->value = kallsyms_addresses[iter->pos];

	iter->type = kallsyms_get_symbol_type(off);

	off = kallsyms_expand_symbol(off, iter->name);

	return off - iter->nameoff;
}

static void reset_iter(struct kallsym_iter *iter, loff_t new_pos)
{
	iter->name[0] = '\0';
	iter->nameoff = get_symbol_offset(new_pos);
	iter->pos = new_pos;
}

static int update_iter(struct kallsym_iter *iter, loff_t pos)
{
	
	if (pos >= kallsyms_num_syms) {
		iter->pos = pos;
		return get_ksymbol_mod(iter);
	}

	
	if (pos != iter->pos)
		reset_iter(iter, pos);

	iter->nameoff += get_ksymbol_core(iter);
	iter->pos++;

	return 1;
}

static void *s_next(struct seq_file *m, void *p, loff_t *pos)
{
	(*pos)++;

	if (!update_iter(m->private, *pos))
		return NULL;
	return p;
}

static void *s_start(struct seq_file *m, loff_t *pos)
{
	if (!update_iter(m->private, *pos))
		return NULL;
	return m->private;
}

static void s_stop(struct seq_file *m, void *p)
{
}

static int s_show(struct seq_file *m, void *p)
{
	struct kallsym_iter *iter = m->private;

	
	if (!iter->name[0])
		return 0;

	if (iter->module_name[0]) {
		char type;

		type = iter->exported ? toupper(iter->type) :
					tolower(iter->type);
		seq_printf(m, "%pK %c %s\t[%s]\n", (void *)iter->value,
			   type, iter->name, iter->module_name);
	} else
		seq_printf(m, "%pK %c %s\n", (void *)iter->value,
			   iter->type, iter->name);
	return 0;
}

static const struct seq_operations kallsyms_op = {
	.start = s_start,
	.next = s_next,
	.stop = s_stop,
	.show = s_show
};

static int kallsyms_open(struct inode *inode, struct file *file)
{
	struct kallsym_iter *iter;
	int ret;

	iter = kmalloc(sizeof(*iter), GFP_KERNEL);
	if (!iter)
		return -ENOMEM;
	reset_iter(iter, 0);

	ret = seq_open(file, &kallsyms_op);
	if (ret == 0)
		((struct seq_file *)file->private_data)->private = iter;
	else
		kfree(iter);
	return ret;
}

#ifdef	CONFIG_KGDB_KDB
const char *kdb_walk_kallsyms(loff_t *pos)
{
	static struct kallsym_iter kdb_walk_kallsyms_iter;
	if (*pos == 0) {
		memset(&kdb_walk_kallsyms_iter, 0,
		       sizeof(kdb_walk_kallsyms_iter));
		reset_iter(&kdb_walk_kallsyms_iter, 0);
	}
	while (1) {
		if (!update_iter(&kdb_walk_kallsyms_iter, *pos))
			return NULL;
		++*pos;
		
		if (kdb_walk_kallsyms_iter.name[0])
			return kdb_walk_kallsyms_iter.name;
	}
}
#endif	

static const struct file_operations kallsyms_operations = {
	.open = kallsyms_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release_private,
};

#define KALLSYMS_ADDRESSES_ADDR		(MSM_KALLSYMS_SAVE_BASE + 0x0)
#define KALLSYMS_NAMES_ADDR			(MSM_KALLSYMS_SAVE_BASE + 0x4)
#define KALLSYMS_NUM_SYMS_ADDR		(MSM_KALLSYMS_SAVE_BASE + 0x8)
#define KALLSYMS_TOKEN_TABLE_ADDR	(MSM_KALLSYMS_SAVE_BASE + 0xC)
#define KALLSYMS_TOKEN_INDEX_ADDR	(MSM_KALLSYMS_SAVE_BASE + 0x10)
#define KALLSYMS_MARKERS_ADDR			(MSM_KALLSYMS_SAVE_BASE + 0x14)
#define _STEXT_ADDR						(MSM_KALLSYMS_SAVE_BASE + 0x18)
#define _SINITTEXT_ADDR				(MSM_KALLSYMS_SAVE_BASE + 0x1C)
#define _EINITTEXT_ADDR				(MSM_KALLSYMS_SAVE_BASE + 0x20)
#define _END_ADDR						(MSM_KALLSYMS_SAVE_BASE + 0x24)
#define KALLSYMS_MAGIC_ADDR			(MSM_KALLSYMS_SAVE_BASE + 0x28)
#define KALLSYMS_MAGIC					0xA0B1C2D3

static void save_kallsyms_addresses(void)
{
	*(unsigned *)KALLSYMS_ADDRESSES_ADDR = (unsigned)kallsyms_addresses;
	*(unsigned *)KALLSYMS_NAMES_ADDR = (unsigned)kallsyms_names;
	*(unsigned *)KALLSYMS_NUM_SYMS_ADDR = (unsigned)kallsyms_num_syms;
	*(unsigned *)KALLSYMS_TOKEN_TABLE_ADDR = (unsigned)kallsyms_token_table;
	*(unsigned *)KALLSYMS_TOKEN_INDEX_ADDR = (unsigned)kallsyms_token_index;
	*(unsigned *)KALLSYMS_MARKERS_ADDR = (unsigned)kallsyms_markers;
	*(unsigned *)_STEXT_ADDR = (unsigned)_stext;
	*(unsigned *)_SINITTEXT_ADDR = (unsigned)_sinittext;
	*(unsigned *)_EINITTEXT_ADDR = (unsigned)_einittext;
	*(unsigned *)_END_ADDR = (unsigned)_end;
	*(unsigned *)KALLSYMS_MAGIC_ADDR = (unsigned)KALLSYMS_MAGIC;
}

static int __init kallsyms_init(void)
{
	proc_create("kallsyms", 0444, NULL, &kallsyms_operations);
	save_kallsyms_addresses();
	return 0;
}
device_initcall(kallsyms_init);
