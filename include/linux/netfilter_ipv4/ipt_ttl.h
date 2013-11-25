
#ifndef _IPT_TTL_H
#define _IPT_TTL_H

#include <linux/types.h>

enum {
	IPT_TTL_EQ = 0,		
	IPT_TTL_NE,		
	IPT_TTL_LT,		
	IPT_TTL_GT,		
};


struct ipt_ttl_info {
	__u8	mode;
	__u8	ttl;
};


#endif
