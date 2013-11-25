#include <linux/mount.h>
#include "../../../fs/mount.h"
#ifdef CONFIG_DIRTY_SYSTEM_DETECTOR
extern int mark_system_dirty(const char *file_name);
extern int is_system_dirty(void);
#else
static inline int mark_system_dirty(const char *file_name) {return 0;};
static inline int is_system_dirty(void) {return 0;};
#endif
