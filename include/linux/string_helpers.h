#ifndef _LINUX_STRING_HELPERS_H_
#define _LINUX_STRING_HELPERS_H_

#include <linux/types.h>

enum string_size_units {
	STRING_UNITS_10,	
	STRING_UNITS_2,		
};

int string_get_size(u64 size, enum string_size_units units,
		    char *buf, int len);

#endif
