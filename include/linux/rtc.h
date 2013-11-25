/*
 * Generic RTC interface.
 * This version contains the part of the user interface to the Real Time Clock
 * service. It is used with both the legacy mc146818 and also  EFI
 * Struct rtc_time and first 12 ioctl by Paul Gortmaker, 1996 - separated out
 * from <linux/mc146818rtc.h> to this file for 2.4 kernels.
 *
 * Copyright (C) 1999 Hewlett-Packard Co.
 * Copyright (C) 1999 Stephane Eranian <eranian@hpl.hp.com>
 */
#ifndef _LINUX_RTC_H_
#define _LINUX_RTC_H_


struct rtc_time {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

struct rtc_wkalrm {
	unsigned char enabled;	
	unsigned char pending;  
	struct rtc_time time;	
};


struct rtc_pll_info {
	int pll_ctrl;       
	int pll_value;      
	int pll_max;        
	int pll_min;        
	int pll_posmult;    
	int pll_negmult;    
	long pll_clock;     
};


#define RTC_AIE_ON	_IO('p', 0x01)	
#define RTC_AIE_OFF	_IO('p', 0x02)	
#define RTC_UIE_ON	_IO('p', 0x03)	
#define RTC_UIE_OFF	_IO('p', 0x04)	
#define RTC_PIE_ON	_IO('p', 0x05)	
#define RTC_PIE_OFF	_IO('p', 0x06)	
#define RTC_WIE_ON	_IO('p', 0x0f)  
#define RTC_WIE_OFF	_IO('p', 0x10)  

#define RTC_ALM_SET	_IOW('p', 0x07, struct rtc_time) 
#define RTC_ALM_READ	_IOR('p', 0x08, struct rtc_time) 
#define RTC_RD_TIME	_IOR('p', 0x09, struct rtc_time) 
#define RTC_SET_TIME	_IOW('p', 0x0a, struct rtc_time) 
#define RTC_IRQP_READ	_IOR('p', 0x0b, unsigned long)	 
#define RTC_IRQP_SET	_IOW('p', 0x0c, unsigned long)	 
#define RTC_EPOCH_READ	_IOR('p', 0x0d, unsigned long)	 
#define RTC_EPOCH_SET	_IOW('p', 0x0e, unsigned long)	 

#define RTC_WKALM_SET	_IOW('p', 0x0f, struct rtc_wkalrm)
#define RTC_WKALM_RD	_IOR('p', 0x10, struct rtc_wkalrm)

#define RTC_PLL_GET	_IOR('p', 0x11, struct rtc_pll_info)  
#define RTC_PLL_SET	_IOW('p', 0x12, struct rtc_pll_info)  

#define RTC_IRQF 0x80	
#define RTC_PF 0x40	
#define RTC_AF 0x20	
#define RTC_UF 0x10	


#define RTC_MAX_FREQ	8192

#ifdef __KERNEL__

#include <linux/types.h>
#include <linux/interrupt.h>

extern int rtc_month_days(unsigned int month, unsigned int year);
extern int rtc_year_days(unsigned int day, unsigned int month, unsigned int year);
extern int rtc_valid_tm(struct rtc_time *tm);
extern int rtc_tm_to_time(struct rtc_time *tm, unsigned long *time);
extern void rtc_time_to_tm(unsigned long time, struct rtc_time *tm);
ktime_t rtc_tm_to_ktime(struct rtc_time tm);
struct rtc_time rtc_ktime_to_tm(ktime_t kt);


#include <linux/device.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/timerqueue.h>
#include <linux/workqueue.h>

extern struct class *rtc_class;

struct rtc_class_ops {
	int (*open)(struct device *);
	void (*release)(struct device *);
	int (*ioctl)(struct device *, unsigned int, unsigned long);
	int (*read_time)(struct device *, struct rtc_time *);
	int (*set_time)(struct device *, struct rtc_time *);
	int (*read_alarm)(struct device *, struct rtc_wkalrm *);
	int (*set_alarm)(struct device *, struct rtc_wkalrm *);
	int (*proc)(struct device *, struct seq_file *);
	int (*set_mmss)(struct device *, unsigned long secs);
	int (*read_callback)(struct device *, int data);
	int (*alarm_irq_enable)(struct device *, unsigned int enabled);
};

#define RTC_DEVICE_NAME_SIZE 20
typedef struct rtc_task {
	void (*func)(void *private_data);
	void *private_data;
} rtc_task_t;


struct rtc_timer {
	struct rtc_task	task;
	struct timerqueue_node node;
	ktime_t period;
	int enabled;
};


#define RTC_DEV_BUSY 0

struct rtc_device
{
	struct device dev;
	struct module *owner;

	int id;
	char name[RTC_DEVICE_NAME_SIZE];

	const struct rtc_class_ops *ops;
	struct mutex ops_lock;

	struct cdev char_dev;
	unsigned long flags;

	unsigned long irq_data;
	spinlock_t irq_lock;
	wait_queue_head_t irq_queue;
	struct fasync_struct *async_queue;

	struct rtc_task *irq_task;
	spinlock_t irq_task_lock;
	int irq_freq;
	int max_user_freq;

	struct timerqueue_head timerqueue;
	struct rtc_timer aie_timer;
	struct rtc_timer uie_rtctimer;
	struct hrtimer pie_timer; 
	int pie_enabled;
	struct work_struct irqwork;
	
	int uie_unsupported;

#ifdef CONFIG_RTC_INTF_DEV_UIE_EMUL
	struct work_struct uie_task;
	struct timer_list uie_timer;
	
	unsigned int oldsecs;
	unsigned int uie_irq_active:1;
	unsigned int stop_uie_polling:1;
	unsigned int uie_task_active:1;
	unsigned int uie_timer_active:1;
#endif
};
#define to_rtc_device(d) container_of(d, struct rtc_device, dev)

extern struct rtc_device *rtc_device_register(const char *name,
					struct device *dev,
					const struct rtc_class_ops *ops,
					struct module *owner);
extern void rtc_device_unregister(struct rtc_device *rtc);

extern int rtc_read_time(struct rtc_device *rtc, struct rtc_time *tm);
extern int rtc_set_time(struct rtc_device *rtc, struct rtc_time *tm);
extern int rtc_set_mmss(struct rtc_device *rtc, unsigned long secs);
int __rtc_read_alarm(struct rtc_device *rtc, struct rtc_wkalrm *alarm);
extern int rtc_read_alarm(struct rtc_device *rtc,
			struct rtc_wkalrm *alrm);
extern int rtc_set_alarm(struct rtc_device *rtc,
				struct rtc_wkalrm *alrm);
extern int rtc_initialize_alarm(struct rtc_device *rtc,
				struct rtc_wkalrm *alrm);
extern void rtc_update_irq(struct rtc_device *rtc,
			unsigned long num, unsigned long events);

extern struct rtc_device *rtc_class_open(char *name);
extern void rtc_class_close(struct rtc_device *rtc);

extern int rtc_irq_register(struct rtc_device *rtc,
				struct rtc_task *task);
extern void rtc_irq_unregister(struct rtc_device *rtc,
				struct rtc_task *task);
extern int rtc_irq_set_state(struct rtc_device *rtc,
				struct rtc_task *task, int enabled);
extern int rtc_irq_set_freq(struct rtc_device *rtc,
				struct rtc_task *task, int freq);
extern int rtc_update_irq_enable(struct rtc_device *rtc, unsigned int enabled);
extern int rtc_alarm_irq_enable(struct rtc_device *rtc, unsigned int enabled);
extern int rtc_dev_update_irq_enable_emul(struct rtc_device *rtc,
						unsigned int enabled);

void rtc_handle_legacy_irq(struct rtc_device *rtc, int num, int mode);
void rtc_aie_update_irq(void *private);
void rtc_uie_update_irq(void *private);
enum hrtimer_restart rtc_pie_update_irq(struct hrtimer *timer);

int rtc_register(rtc_task_t *task);
int rtc_unregister(rtc_task_t *task);
int rtc_control(rtc_task_t *t, unsigned int cmd, unsigned long arg);

void rtc_timer_init(struct rtc_timer *timer, void (*f)(void* p), void* data);
int rtc_timer_start(struct rtc_device *rtc, struct rtc_timer* timer,
			ktime_t expires, ktime_t period);
int rtc_timer_cancel(struct rtc_device *rtc, struct rtc_timer* timer);
void rtc_timer_do_work(struct work_struct *work);

static inline bool is_leap_year(unsigned int year)
{
	return (!(year % 4) && (year % 100)) || !(year % 400);
}

#ifdef CONFIG_RTC_HCTOSYS
extern int rtc_hctosys_ret;
#else
#define rtc_hctosys_ret -ENODEV
#endif

#endif 

#endif 
