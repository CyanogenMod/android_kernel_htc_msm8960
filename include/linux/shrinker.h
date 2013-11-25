#ifndef _LINUX_SHRINKER_H
#define _LINUX_SHRINKER_H

struct shrink_control {
	gfp_t gfp_mask;

	
	unsigned long nr_to_scan;
};

struct shrinker {
	int (*shrink)(struct shrinker *, struct shrink_control *sc);
	int seeks;	
	long batch;	

	
	struct list_head list;
	atomic_long_t nr_in_batch; 
};
#define DEFAULT_SEEKS 2 
extern void register_shrinker(struct shrinker *);
extern void unregister_shrinker(struct shrinker *);
#endif
