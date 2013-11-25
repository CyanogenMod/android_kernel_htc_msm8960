#ifndef _ASM_GENERIC_RESOURCE_H
#define _ASM_GENERIC_RESOURCE_H


#define RLIMIT_CPU		0	
#define RLIMIT_FSIZE		1	
#define RLIMIT_DATA		2	
#define RLIMIT_STACK		3	
#define RLIMIT_CORE		4	

#ifndef RLIMIT_RSS
# define RLIMIT_RSS		5	
#endif

#ifndef RLIMIT_NPROC
# define RLIMIT_NPROC		6	
#endif

#ifndef RLIMIT_NOFILE
# define RLIMIT_NOFILE		7	
#endif

#ifndef RLIMIT_MEMLOCK
# define RLIMIT_MEMLOCK		8	
#endif

#ifndef RLIMIT_AS
# define RLIMIT_AS		9	
#endif

#define RLIMIT_LOCKS		10	
#define RLIMIT_SIGPENDING	11	
#define RLIMIT_MSGQUEUE		12	
#define RLIMIT_NICE		13	
#define RLIMIT_RTPRIO		14	
#define RLIMIT_RTTIME		15	
#define RLIM_NLIMITS		16

#ifndef RLIM_INFINITY
# define RLIM_INFINITY		(~0UL)
#endif

#ifndef _STK_LIM_MAX
# define _STK_LIM_MAX		RLIM_INFINITY
#endif

#ifdef __KERNEL__

#define INIT_RLIMITS							\
{									\
	[RLIMIT_CPU]		= {  RLIM_INFINITY,  RLIM_INFINITY },	\
	[RLIMIT_FSIZE]		= {  RLIM_INFINITY,  RLIM_INFINITY },	\
	[RLIMIT_DATA]		= {  RLIM_INFINITY,  RLIM_INFINITY },	\
	[RLIMIT_STACK]		= {       _STK_LIM,   _STK_LIM_MAX },	\
	[RLIMIT_CORE]		= {              0,  RLIM_INFINITY },	\
	[RLIMIT_RSS]		= {  RLIM_INFINITY,  RLIM_INFINITY },	\
	[RLIMIT_NPROC]		= {              0,              0 },	\
	[RLIMIT_NOFILE]		= {   INR_OPEN_CUR,   INR_OPEN_MAX },	\
	[RLIMIT_MEMLOCK]	= {    MLOCK_LIMIT,    MLOCK_LIMIT },	\
	[RLIMIT_AS]		= {  RLIM_INFINITY,  RLIM_INFINITY },	\
	[RLIMIT_LOCKS]		= {  RLIM_INFINITY,  RLIM_INFINITY },	\
	[RLIMIT_SIGPENDING]	= { 		0,	       0 },	\
	[RLIMIT_MSGQUEUE]	= {   MQ_BYTES_MAX,   MQ_BYTES_MAX },	\
	[RLIMIT_NICE]		= { 0, 0 },				\
	[RLIMIT_RTPRIO]		= { 0, 0 },				\
	[RLIMIT_RTTIME]		= {  RLIM_INFINITY,  RLIM_INFINITY },	\
}

#endif	

#endif
