#ifndef _LINUX_MODULELOADER_H
#define _LINUX_MODULELOADER_H

#include <linux/module.h>
#include <linux/elf.h>


int module_frob_arch_sections(Elf_Ehdr *hdr,
			      Elf_Shdr *sechdrs,
			      char *secstrings,
			      struct module *mod);

unsigned int arch_mod_section_prepend(struct module *mod, unsigned int section);

void *module_alloc(unsigned long size);

void module_free(struct module *mod, void *module_region);

int apply_relocate(Elf_Shdr *sechdrs,
		   const char *strtab,
		   unsigned int symindex,
		   unsigned int relsec,
		   struct module *mod);

int apply_relocate_add(Elf_Shdr *sechdrs,
		       const char *strtab,
		       unsigned int symindex,
		       unsigned int relsec,
		       struct module *mod);

int module_finalize(const Elf_Ehdr *hdr,
		    const Elf_Shdr *sechdrs,
		    struct module *mod);

void module_arch_cleanup(struct module *mod);

#endif
