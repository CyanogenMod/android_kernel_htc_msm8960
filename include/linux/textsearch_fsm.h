#ifndef __LINUX_TEXTSEARCH_FSM_H
#define __LINUX_TEXTSEARCH_FSM_H

#include <linux/types.h>

enum {
	TS_FSM_SPECIFIC,	
	TS_FSM_WILDCARD,	
	TS_FSM_DIGIT,		
	TS_FSM_XDIGIT,		
	TS_FSM_PRINT,		
	TS_FSM_ALPHA,		
	TS_FSM_ALNUM,		
	TS_FSM_ASCII,		
	TS_FSM_CNTRL,		
	TS_FSM_GRAPH,		
	TS_FSM_LOWER,		
	TS_FSM_UPPER,		
	TS_FSM_PUNCT,		
	TS_FSM_SPACE,		
	__TS_FSM_TYPE_MAX,
};
#define TS_FSM_TYPE_MAX (__TS_FSM_TYPE_MAX - 1)

enum {
	TS_FSM_SINGLE,		
	TS_FSM_PERHAPS,		
	TS_FSM_ANY,		
	TS_FSM_MULTI,		
	TS_FSM_HEAD_IGNORE,	
	__TS_FSM_RECUR_MAX,
};
#define TS_FSM_RECUR_MAX (__TS_FSM_RECUR_MAX - 1)

struct ts_fsm_token
{
	__u16		type;
	__u8		recur;
	__u8		value;
};

#endif
