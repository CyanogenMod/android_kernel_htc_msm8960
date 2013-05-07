
#include <linux/types.h>


#define GET_RAMDUMP_LENGTH	0x01
#define JUMP_RAMUDMP_START	0x02


struct ramdump_region {
	unsigned long start;
	unsigned long size;
};

struct ramdump_platform_data {
	int count;
	struct ramdump_region region[];
};
