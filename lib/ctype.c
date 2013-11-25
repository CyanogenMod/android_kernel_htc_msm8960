/*
 *  linux/lib/ctype.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linux/ctype.h>
#include <linux/compiler.h>
#include <linux/export.h>

const unsigned char _ctype[] = {
_C,_C,_C,_C,_C,_C,_C,_C,				
_C,_C|_S,_C|_S,_C|_S,_C|_S,_C|_S,_C,_C,			
_C,_C,_C,_C,_C,_C,_C,_C,				
_C,_C,_C,_C,_C,_C,_C,_C,				
_S|_SP,_P,_P,_P,_P,_P,_P,_P,				
_P,_P,_P,_P,_P,_P,_P,_P,				
_D,_D,_D,_D,_D,_D,_D,_D,				
_D,_D,_P,_P,_P,_P,_P,_P,				
_P,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U,		
_U,_U,_U,_U,_U,_U,_U,_U,				
_U,_U,_U,_U,_U,_U,_U,_U,				
_U,_U,_U,_P,_P,_P,_P,_P,				
_P,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L,		
_L,_L,_L,_L,_L,_L,_L,_L,				
_L,_L,_L,_L,_L,_L,_L,_L,				
_L,_L,_L,_P,_P,_P,_P,_C,				
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,			
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,			
_S|_SP,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,	
_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,	
_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,	
_U,_U,_U,_U,_U,_U,_U,_P,_U,_U,_U,_U,_U,_U,_U,_L,	
_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,	
_L,_L,_L,_L,_L,_L,_L,_P,_L,_L,_L,_L,_L,_L,_L,_L};	

EXPORT_SYMBOL(_ctype);
