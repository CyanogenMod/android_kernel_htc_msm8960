/*****************************************************************************
 *                                                                           *
 * Copyright (c) David L. Mills 1993                                         *
 *                                                                           *
 * Permission to use, copy, modify, and distribute this software and its     *
 * documentation for any purpose and without fee is hereby granted, provided *
 * that the above copyright notice appears in all copies and that both the   *
 * copyright notice and this permission notice appear in supporting          *
 * documentation, and that the name University of Delaware not be used in    *
 * advertising or publicity pertaining to distribution of the software       *
 * without specific, written prior permission.  The University of Delaware   *
 * makes no representations about the suitability this software for any      *
 * purpose.  It is provided "as is" without express or implied warranty.     *
 *                                                                           *
 *****************************************************************************/

#ifndef _LINUX_TIMEX_H
#define _LINUX_TIMEX_H

#include <linux/time.h>

#define NTP_API		4	

struct timex {
	unsigned int modes;	
	long offset;		
	long freq;		
	long maxerror;		
	long esterror;		
	int status;		
	long constant;		
	long precision;		
	long tolerance;		
	struct timeval time;	
	long tick;		

	long ppsfreq;           
	long jitter;            
	int shift;              
	long stabil;            
	long jitcnt;            
	long calcnt;            
	long errcnt;            
	long stbcnt;            

	int tai;		

	int  :32; int  :32; int  :32; int  :32;
	int  :32; int  :32; int  :32; int  :32;
	int  :32; int  :32; int  :32;
};

#define ADJ_OFFSET		0x0001	
#define ADJ_FREQUENCY		0x0002	
#define ADJ_MAXERROR		0x0004	
#define ADJ_ESTERROR		0x0008	
#define ADJ_STATUS		0x0010	
#define ADJ_TIMECONST		0x0020	
#define ADJ_TAI			0x0080	
#define ADJ_SETOFFSET		0x0100  
#define ADJ_MICRO		0x1000	
#define ADJ_NANO		0x2000	
#define ADJ_TICK		0x4000	

#ifdef __KERNEL__
#define ADJ_ADJTIME		0x8000	
#define ADJ_OFFSET_SINGLESHOT	0x0001	
#define ADJ_OFFSET_READONLY	0x2000	
#else
#define ADJ_OFFSET_SINGLESHOT	0x8001	
#define ADJ_OFFSET_SS_READ	0xa001	
#endif

#define MOD_OFFSET	ADJ_OFFSET
#define MOD_FREQUENCY	ADJ_FREQUENCY
#define MOD_MAXERROR	ADJ_MAXERROR
#define MOD_ESTERROR	ADJ_ESTERROR
#define MOD_STATUS	ADJ_STATUS
#define MOD_TIMECONST	ADJ_TIMECONST
#define MOD_TAI	ADJ_TAI
#define MOD_MICRO	ADJ_MICRO
#define MOD_NANO	ADJ_NANO


#define STA_PLL		0x0001	
#define STA_PPSFREQ	0x0002	
#define STA_PPSTIME	0x0004	
#define STA_FLL		0x0008	

#define STA_INS		0x0010	
#define STA_DEL		0x0020	
#define STA_UNSYNC	0x0040	
#define STA_FREQHOLD	0x0080	

#define STA_PPSSIGNAL	0x0100	
#define STA_PPSJITTER	0x0200	
#define STA_PPSWANDER	0x0400	
#define STA_PPSERROR	0x0800	

#define STA_CLOCKERR	0x1000	
#define STA_NANO	0x2000	
#define STA_MODE	0x4000	
#define STA_CLK		0x8000	

#define STA_RONLY (STA_PPSSIGNAL | STA_PPSJITTER | STA_PPSWANDER | \
	STA_PPSERROR | STA_CLOCKERR | STA_NANO | STA_MODE | STA_CLK)

#define TIME_OK		0	
#define TIME_INS	1	
#define TIME_DEL	2	
#define TIME_OOP	3	
#define TIME_WAIT	4	
#define TIME_ERROR	5	
#define TIME_BAD	TIME_ERROR 

#ifdef __KERNEL__
#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/param.h>

#include <asm/timex.h>

#define SHIFT_PLL	2	
#define SHIFT_FLL	2	
#define MAXTC		10	

#define SHIFT_USEC 16		
#define PPM_SCALE ((s64)NSEC_PER_USEC << (NTP_SCALE_SHIFT - SHIFT_USEC))
#define PPM_SCALE_INV_SHIFT 19
#define PPM_SCALE_INV ((1LL << (PPM_SCALE_INV_SHIFT + NTP_SCALE_SHIFT)) / \
		       PPM_SCALE + 1)

#define MAXPHASE 500000000L	
#define MAXFREQ 500000		
#define MAXFREQ_SCALED ((s64)MAXFREQ << NTP_SCALE_SHIFT)
#define MINSEC 256		
#define MAXSEC 2048		
#define NTP_PHASE_LIMIT ((MAXPHASE / NSEC_PER_USEC) << 5) 

extern unsigned long tick_usec;		
extern unsigned long tick_nsec;		

extern void ntp_init(void);
extern void ntp_clear(void);

#define shift_right(x, s) ({	\
	__typeof__(x) __x = (x);	\
	__typeof__(s) __s = (s);	\
	__x < 0 ? -(-__x >> __s) : __x >> __s;	\
})

#define NTP_SCALE_SHIFT		32

#define NTP_INTERVAL_FREQ  (HZ)
#define NTP_INTERVAL_LENGTH (NSEC_PER_SEC/NTP_INTERVAL_FREQ)

extern u64 ntp_tick_length(void);

extern int second_overflow(unsigned long secs);
extern int do_adjtimex(struct timex *);
extern void hardpps(const struct timespec *, const struct timespec *);

int read_current_timer(unsigned long *timer_val);

#define PIT_TICK_RATE 1193182ul

#endif 

#endif 
