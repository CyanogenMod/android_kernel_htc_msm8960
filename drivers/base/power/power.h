#include <linux/pm_qos.h>

#ifdef CONFIG_PM_RUNTIME

extern void pm_runtime_init(struct device *dev);
extern void pm_runtime_remove(struct device *dev);

#else 

static inline void pm_runtime_init(struct device *dev) {}
static inline void pm_runtime_remove(struct device *dev) {}

#endif 

#ifdef CONFIG_PM_SLEEP

extern int pm_async_enabled;

extern struct list_head dpm_list;	

static inline struct device *to_device(struct list_head *entry)
{
	return container_of(entry, struct device, power.entry);
}

extern void device_pm_init(struct device *dev);
extern void device_pm_add(struct device *);
extern void device_pm_remove(struct device *);
extern void device_pm_move_before(struct device *, struct device *);
extern void device_pm_move_after(struct device *, struct device *);
extern void device_pm_move_last(struct device *);

#else 

static inline void device_pm_init(struct device *dev)
{
	spin_lock_init(&dev->power.lock);
	dev->power.power_state = PMSG_INVALID;
	pm_runtime_init(dev);
}

static inline void device_pm_add(struct device *dev)
{
	dev_pm_qos_constraints_init(dev);
}

static inline void device_pm_remove(struct device *dev)
{
	dev_pm_qos_constraints_destroy(dev);
	pm_runtime_remove(dev);
}

static inline void device_pm_move_before(struct device *deva,
					 struct device *devb) {}
static inline void device_pm_move_after(struct device *deva,
					struct device *devb) {}
static inline void device_pm_move_last(struct device *dev) {}

#endif 

#ifdef CONFIG_PM


extern int dpm_sysfs_add(struct device *dev);
extern void dpm_sysfs_remove(struct device *dev);
extern void rpm_sysfs_remove(struct device *dev);
extern int wakeup_sysfs_add(struct device *dev);
extern void wakeup_sysfs_remove(struct device *dev);
extern int pm_qos_sysfs_add(struct device *dev);
extern void pm_qos_sysfs_remove(struct device *dev);

#else 

static inline int dpm_sysfs_add(struct device *dev) { return 0; }
static inline void dpm_sysfs_remove(struct device *dev) {}
static inline void rpm_sysfs_remove(struct device *dev) {}
static inline int wakeup_sysfs_add(struct device *dev) { return 0; }
static inline void wakeup_sysfs_remove(struct device *dev) {}
static inline int pm_qos_sysfs_add(struct device *dev) { return 0; }
static inline void pm_qos_sysfs_remove(struct device *dev) {}

#endif
