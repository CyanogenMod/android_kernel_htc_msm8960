#include <linux/list.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/bug.h>
#include <linux/sched.h>
#include <mach/restart.h>

extern const struct bug_entry __start___bug_table[], __stop___bug_table[];

static inline unsigned long bug_addr(const struct bug_entry *bug)
{
#ifndef CONFIG_GENERIC_BUG_RELATIVE_POINTERS
	return bug->bug_addr;
#else
	return (unsigned long)bug + bug->bug_addr_disp;
#endif
}

#ifdef CONFIG_MODULES
static LIST_HEAD(module_bug_list);

static const struct bug_entry *module_find_bug(unsigned long bugaddr)
{
	struct module *mod;

	list_for_each_entry(mod, &module_bug_list, bug_list) {
		const struct bug_entry *bug = mod->bug_table;
		unsigned i;

		for (i = 0; i < mod->num_bugs; ++i, ++bug)
			if (bugaddr == bug_addr(bug))
				return bug;
	}
	return NULL;
}

void module_bug_finalize(const Elf_Ehdr *hdr, const Elf_Shdr *sechdrs,
			 struct module *mod)
{
	char *secstrings;
	unsigned int i;

	mod->bug_table = NULL;
	mod->num_bugs = 0;

	
	secstrings = (char *)hdr + sechdrs[hdr->e_shstrndx].sh_offset;
	for (i = 1; i < hdr->e_shnum; i++) {
		if (strcmp(secstrings+sechdrs[i].sh_name, "__bug_table"))
			continue;
		mod->bug_table = (void *) sechdrs[i].sh_addr;
		mod->num_bugs = sechdrs[i].sh_size / sizeof(struct bug_entry);
		break;
	}

	list_add(&mod->bug_list, &module_bug_list);
}

void module_bug_cleanup(struct module *mod)
{
	list_del(&mod->bug_list);
}

#else

static inline const struct bug_entry *module_find_bug(unsigned long bugaddr)
{
	return NULL;
}
#endif

const struct bug_entry *find_bug(unsigned long bugaddr)
{
	const struct bug_entry *bug;

	for (bug = __start___bug_table; bug < __stop___bug_table; ++bug)
		if (bugaddr == bug_addr(bug))
			return bug;

	return module_find_bug(bugaddr);
}

extern char ramdump_buf[256];
extern int ramdump_source;
enum bug_trap_type report_bug(unsigned long bugaddr, struct pt_regs *regs)
{
	const struct bug_entry *bug;
	const char *file;
	unsigned line, warning;

	if (!is_valid_bugaddr(bugaddr))
		return BUG_TRAP_TYPE_NONE;

	bug = find_bug(bugaddr);

	file = NULL;
	line = 0;
	warning = 0;

	if (bug) {
#ifdef CONFIG_DEBUG_BUGVERBOSE
#ifndef CONFIG_GENERIC_BUG_RELATIVE_POINTERS
		file = bug->file;
#else
		file = (const char *)bug + bug->file_disp;
#endif
		line = bug->line;
#endif
		warning = (bug->flags & BUGFLAG_WARNING) != 0;
	}

	if (warning) {
		
		printk(KERN_WARNING "------------[ cut here ]------------\n");
		ramdump_source = RAMDUMP_BUG;

		if (file)
		{
			printk(KERN_WARNING "WARNING: at %s:%u\n",
			       file, line);
			sprintf(ramdump_buf, "kernel BUG at %s:%u!", file, line);
		}
		else
		{
			printk(KERN_WARNING "WARNING: at %p "
			       "[verbose debug info unavailable]\n",
			       (void *)bugaddr);
			sprintf(ramdump_buf, "Kernel BUG at %p [verbose debug info unavailable]", (void *)bugaddr);
		}

		print_modules();
		show_regs(regs);
		print_oops_end_marker();
		add_taint(BUG_GET_TAINT(bug));
		return BUG_TRAP_TYPE_WARN;
	}

	printk(KERN_DEFAULT "------------[ cut here ]------------\n");

	if (file)
		printk(KERN_CRIT "kernel BUG at %s:%u!\n",
		       file, line);
	else
		printk(KERN_CRIT "Kernel BUG at %p "
		       "[verbose debug info unavailable]\n",
		       (void *)bugaddr);

	return BUG_TRAP_TYPE_BUG;
}
