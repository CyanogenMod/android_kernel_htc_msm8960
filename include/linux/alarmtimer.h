#ifndef _LINUX_ALARMTIMER_H
#define _LINUX_ALARMTIMER_H

#include <linux/time.h>
#include <linux/hrtimer.h>
#include <linux/timerqueue.h>
#include <linux/rtc.h>

enum alarmtimer_type {
	ALARM_REALTIME,
	ALARM_BOOTTIME,

	ALARM_NUMTYPE,
};

enum alarmtimer_restart {
	ALARMTIMER_NORESTART,
	ALARMTIMER_RESTART,
};


#define ALARMTIMER_STATE_INACTIVE	0x00
#define ALARMTIMER_STATE_ENQUEUED	0x01
#define ALARMTIMER_STATE_CALLBACK	0x02

struct alarm {
	struct timerqueue_node	node;
	enum alarmtimer_restart	(*function)(struct alarm *, ktime_t now);
	enum alarmtimer_type	type;
	int			state;
	void			*data;
};

void alarm_init(struct alarm *alarm, enum alarmtimer_type type,
		enum alarmtimer_restart (*function)(struct alarm *, ktime_t));
void alarm_start(struct alarm *alarm, ktime_t start);
int alarm_try_to_cancel(struct alarm *alarm);
int alarm_cancel(struct alarm *alarm);

u64 alarm_forward(struct alarm *alarm, ktime_t now, ktime_t interval);

static inline int alarmtimer_active(const struct alarm *timer)
{
	return timer->state != ALARMTIMER_STATE_INACTIVE;
}

static inline int alarmtimer_is_queued(struct alarm *timer)
{
	return timer->state & ALARMTIMER_STATE_ENQUEUED;
}

static inline int alarmtimer_callback_running(struct alarm *timer)
{
	return timer->state & ALARMTIMER_STATE_CALLBACK;
}


struct rtc_device *alarmtimer_get_rtcdev(void);

#endif
