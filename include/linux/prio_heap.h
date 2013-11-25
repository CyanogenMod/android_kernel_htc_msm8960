#ifndef _LINUX_PRIO_HEAP_H
#define _LINUX_PRIO_HEAP_H


#include <linux/gfp.h>

struct ptr_heap {
	void **ptrs;
	int max;
	int size;
	int (*gt)(void *, void *);
};

extern int heap_init(struct ptr_heap *heap, size_t size, gfp_t gfp_mask,
		     int (*gt)(void *, void *));

void heap_free(struct ptr_heap *heap);

extern void *heap_insert(struct ptr_heap *heap, void *p);



#endif 
