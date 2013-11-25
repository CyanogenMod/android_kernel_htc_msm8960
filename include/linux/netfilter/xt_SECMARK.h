#ifndef _XT_SECMARK_H_target
#define _XT_SECMARK_H_target

#include <linux/types.h>

#define SECMARK_MODE_SEL	0x01		
#define SECMARK_SECCTX_MAX	256

struct xt_secmark_target_info {
	__u8 mode;
	__u32 secid;
	char secctx[SECMARK_SECCTX_MAX];
};

#endif 
