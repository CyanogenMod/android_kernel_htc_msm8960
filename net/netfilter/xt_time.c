/*
 *	xt_time
 *	Copyright © CC Computer Consultants GmbH, 2007
 *
 *	based on ipt_time by Fabrice MARIE <fabrice@netfilter.org>
 *	This is a module which is used for time matching
 *	It is using some modified code from dietlibc (localtime() function)
 *	that you can find at http://www.fefe.de/dietlibc/
 *	This file is distributed under the terms of the GNU General Public
 *	License (GPL). Copies of the GPL can be obtained from gnu.org/gpl.
 */
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_time.h>

struct xtm {
	u_int8_t month;    
	u_int8_t monthday; 
	u_int8_t weekday;  
	u_int8_t hour;     
	u_int8_t minute;   
	u_int8_t second;   
	unsigned int dse;
};

extern struct timezone sys_tz; 

static const u_int16_t days_since_year[] = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334,
};

static const u_int16_t days_since_leapyear[] = {
	0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335,
};

enum {
	DSE_FIRST = 2039,
};
static const u_int16_t days_since_epoch[] = {
	
	25202, 24837, 24472, 24106, 23741, 23376, 23011, 22645, 22280, 21915,
	
	21550, 21184, 20819, 20454, 20089, 19723, 19358, 18993, 18628, 18262,
	
	17897, 17532, 17167, 16801, 16436, 16071, 15706, 15340, 14975, 14610,
	
	14245, 13879, 13514, 13149, 12784, 12418, 12053, 11688, 11323, 10957,
	
	10592, 10227, 9862, 9496, 9131, 8766, 8401, 8035, 7670, 7305,
	
	6940, 6574, 6209, 5844, 5479, 5113, 4748, 4383, 4018, 3652,
	
	3287, 2922, 2557, 2191, 1826, 1461, 1096, 730, 365, 0,
};

static inline bool is_leap(unsigned int y)
{
	return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
}

static inline unsigned int localtime_1(struct xtm *r, time_t time)
{
	unsigned int v, w;

	
	v         = time % 86400;
	r->second = v % 60;
	w         = v / 60;
	r->minute = w % 60;
	r->hour   = w / 60;
	return v;
}

static inline void localtime_2(struct xtm *r, time_t time)
{
	r->dse = time / 86400;

	r->weekday = (4 + r->dse - 1) % 7 + 1;
}

static void localtime_3(struct xtm *r, time_t time)
{
	unsigned int year, i, w = r->dse;

	for (i = 0, year = DSE_FIRST; days_since_epoch[i] > w;
	    ++i, --year)
		;

	w -= days_since_epoch[i];

	if (is_leap(year)) {
		
		for (i = ARRAY_SIZE(days_since_leapyear) - 1;
		    i > 0 && days_since_leapyear[i] > w; --i)
			;
		r->monthday = w - days_since_leapyear[i] + 1;
	} else {
		for (i = ARRAY_SIZE(days_since_year) - 1;
		    i > 0 && days_since_year[i] > w; --i)
			;
		r->monthday = w - days_since_year[i] + 1;
	}

	r->month    = i + 1;
}

static bool
time_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_time_info *info = par->matchinfo;
	unsigned int packet_time;
	struct xtm current_time;
	s64 stamp;

	if (skb->tstamp.tv64 == 0)
		__net_timestamp((struct sk_buff *)skb);

	stamp = ktime_to_ns(skb->tstamp);
	stamp = div_s64(stamp, NSEC_PER_SEC);

	if (info->flags & XT_TIME_LOCAL_TZ)
		
		stamp -= 60 * sys_tz.tz_minuteswest;


	if (stamp < info->date_start || stamp > info->date_stop)
		return false;

	packet_time = localtime_1(&current_time, stamp);

	if (info->daytime_start < info->daytime_stop) {
		if (packet_time < info->daytime_start ||
		    packet_time > info->daytime_stop)
			return false;
	} else {
		if (packet_time < info->daytime_start &&
		    packet_time > info->daytime_stop)
			return false;
	}

	localtime_2(&current_time, stamp);

	if (!(info->weekdays_match & (1 << current_time.weekday)))
		return false;

	
	if (info->monthdays_match != XT_TIME_ALL_MONTHDAYS) {
		localtime_3(&current_time, stamp);
		if (!(info->monthdays_match & (1 << current_time.monthday)))
			return false;
	}

	return true;
}

static int time_mt_check(const struct xt_mtchk_param *par)
{
	const struct xt_time_info *info = par->matchinfo;

	if (info->daytime_start > XT_TIME_MAX_DAYTIME ||
	    info->daytime_stop > XT_TIME_MAX_DAYTIME) {
		pr_info("invalid argument - start or "
			"stop time greater than 23:59:59\n");
		return -EDOM;
	}

	return 0;
}

static struct xt_match xt_time_mt_reg __read_mostly = {
	.name       = "time",
	.family     = NFPROTO_UNSPEC,
	.match      = time_mt,
	.checkentry = time_mt_check,
	.matchsize  = sizeof(struct xt_time_info),
	.me         = THIS_MODULE,
};

static int __init time_mt_init(void)
{
	int minutes = sys_tz.tz_minuteswest;

	if (minutes < 0) 
		printk(KERN_INFO KBUILD_MODNAME
		       ": kernel timezone is +%02d%02d\n",
		       -minutes / 60, -minutes % 60);
	else 
		printk(KERN_INFO KBUILD_MODNAME
		       ": kernel timezone is -%02d%02d\n",
		       minutes / 60, minutes % 60);

	return xt_register_match(&xt_time_mt_reg);
}

static void __exit time_mt_exit(void)
{
	xt_unregister_match(&xt_time_mt_reg);
}

module_init(time_mt_init);
module_exit(time_mt_exit);
MODULE_AUTHOR("Jan Engelhardt <jengelh@medozas.de>");
MODULE_DESCRIPTION("Xtables: time-based matching");
MODULE_LICENSE("GPL");
MODULE_ALIAS("ipt_time");
MODULE_ALIAS("ip6t_time");
