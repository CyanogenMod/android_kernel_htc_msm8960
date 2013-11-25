#ifndef _LINUX_ERRNO_H
#define _LINUX_ERRNO_H

#include <asm/errno.h>

#ifdef __KERNEL__

#define ERESTARTSYS	512
#define ERESTARTNOINTR	513
#define ERESTARTNOHAND	514	
#define ENOIOCTLCMD	515	
#define ERESTART_RESTARTBLOCK 516 
#define EPROBE_DEFER	517	

#define EBADHANDLE	521	
#define ENOTSYNC	522	
#define EBADCOOKIE	523	
#define ENOTSUPP	524	
#define ETOOSMALL	525	
#define ESERVERFAULT	526	
#define EBADTYPE	527	
#define EJUKEBOX	528	
#define EIOCBQUEUED	529	
#define EIOCBRETRY	530	

#endif

#endif
