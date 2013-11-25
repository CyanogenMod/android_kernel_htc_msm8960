#ifndef INFTREES_H
#define INFTREES_H

/* inftrees.h -- header to use inftrees.c
 * Copyright (C) 1995-2005 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */


typedef struct {
    unsigned char op;           
    unsigned char bits;         
    unsigned short val;         
} code;


#define ENOUGH 2048
#define MAXD 592

typedef enum {
    CODES,
    LENS,
    DISTS
} codetype;

extern int zlib_inflate_table (codetype type, unsigned short *lens,
                             unsigned codes, code **table,
                             unsigned *bits, unsigned short *work);
#endif
