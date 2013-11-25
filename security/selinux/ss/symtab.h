#ifndef _SS_SYMTAB_H_
#define _SS_SYMTAB_H_

#include "hashtab.h"

struct symtab {
	struct hashtab *table;	
	u32 nprim;		
};

int symtab_init(struct symtab *s, unsigned int size);

#endif	


