#ifndef __FS_NOTIFY_FSNOTIFY_H_
#define __FS_NOTIFY_FSNOTIFY_H_

#include <linux/list.h>
#include <linux/fsnotify.h>
#include <linux/srcu.h>
#include <linux/types.h>

extern void fsnotify_flush_notify(struct fsnotify_group *group);

extern struct srcu_struct fsnotify_mark_srcu;

extern void fsnotify_set_inode_mark_mask_locked(struct fsnotify_mark *fsn_mark,
						__u32 mask);
extern int fsnotify_add_inode_mark(struct fsnotify_mark *mark,
				   struct fsnotify_group *group, struct inode *inode,
				   int allow_dups);
extern int fsnotify_add_vfsmount_mark(struct fsnotify_mark *mark,
				      struct fsnotify_group *group, struct vfsmount *mnt,
				      int allow_dups);

extern void fsnotify_final_destroy_group(struct fsnotify_group *group);

extern void fsnotify_destroy_vfsmount_mark(struct fsnotify_mark *mark);
extern void fsnotify_destroy_inode_mark(struct fsnotify_mark *mark);
extern void fsnotify_clear_marks_by_inode(struct inode *inode);
extern void fsnotify_clear_marks_by_mount(struct vfsmount *mnt);
extern void __fsnotify_update_child_dentry_flags(struct inode *inode);

extern struct fsnotify_event_holder *fsnotify_alloc_event_holder(void);
extern void fsnotify_destroy_event_holder(struct fsnotify_event_holder *holder);

#endif	
