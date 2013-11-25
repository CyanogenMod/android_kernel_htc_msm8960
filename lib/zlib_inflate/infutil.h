/* infutil.h -- types and macros common to blocks and codes
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */


#ifndef _INFUTIL_H
#define _INFUTIL_H

#include <linux/zlib.h>


struct inflate_workspace {
	struct inflate_state inflate_state;
	unsigned char working_window[1 << MAX_WBITS];
};

#define WS(z) ((struct inflate_workspace *)(z->workspace))

#endif
