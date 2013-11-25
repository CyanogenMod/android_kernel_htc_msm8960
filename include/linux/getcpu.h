#ifndef _LINUX_GETCPU_H
#define _LINUX_GETCPU_H 1

struct getcpu_cache {
	unsigned long blob[128 / sizeof(long)];
};

#endif
