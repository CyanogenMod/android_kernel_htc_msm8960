/* inffast.c -- fast decoding
 * Copyright (C) 1995-2004 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include <linux/zutil.h>
#include "inftrees.h"
#include "inflate.h"
#include "inffast.h"

#ifndef ASMINF

union uu {
	unsigned short us;
	unsigned char b[2];
};

static inline unsigned short
get_unaligned16(const unsigned short *p)
{
	union uu  mm;
	unsigned char *b = (unsigned char *)p;

	mm.b[0] = b[0];
	mm.b[1] = b[1];
	return mm.us;
}

#ifdef POSTINC
#  define OFF 0
#  define PUP(a) *(a)++
#  define UP_UNALIGNED(a) get_unaligned16((a)++)
#else
#  define OFF 1
#  define PUP(a) *++(a)
#  define UP_UNALIGNED(a) get_unaligned16(++(a))
#endif

void inflate_fast(z_streamp strm, unsigned start)
{
    struct inflate_state *state;
    const unsigned char *in;    
    const unsigned char *last;  
    unsigned char *out;         
    unsigned char *beg;         
    unsigned char *end;         
#ifdef INFLATE_STRICT
    unsigned dmax;              
#endif
    unsigned wsize;             
    unsigned whave;             
    unsigned write;             
    unsigned char *window;      
    unsigned long hold;         
    unsigned bits;              
    code const *lcode;          
    code const *dcode;          
    unsigned lmask;             
    unsigned dmask;             
    code this;                  
    unsigned op;                
                                
    unsigned len;               
    unsigned dist;              
    unsigned char *from;        

    
    state = (struct inflate_state *)strm->state;
    in = strm->next_in - OFF;
    last = in + (strm->avail_in - 5);
    out = strm->next_out - OFF;
    beg = out - (start - strm->avail_out);
    end = out + (strm->avail_out - 257);
#ifdef INFLATE_STRICT
    dmax = state->dmax;
#endif
    wsize = state->wsize;
    whave = state->whave;
    write = state->write;
    window = state->window;
    hold = state->hold;
    bits = state->bits;
    lcode = state->lencode;
    dcode = state->distcode;
    lmask = (1U << state->lenbits) - 1;
    dmask = (1U << state->distbits) - 1;

    do {
        if (bits < 15) {
            hold += (unsigned long)(PUP(in)) << bits;
            bits += 8;
            hold += (unsigned long)(PUP(in)) << bits;
            bits += 8;
        }
        this = lcode[hold & lmask];
      dolen:
        op = (unsigned)(this.bits);
        hold >>= op;
        bits -= op;
        op = (unsigned)(this.op);
        if (op == 0) {                          
            PUP(out) = (unsigned char)(this.val);
        }
        else if (op & 16) {                     
            len = (unsigned)(this.val);
            op &= 15;                           
            if (op) {
                if (bits < op) {
                    hold += (unsigned long)(PUP(in)) << bits;
                    bits += 8;
                }
                len += (unsigned)hold & ((1U << op) - 1);
                hold >>= op;
                bits -= op;
            }
            if (bits < 15) {
                hold += (unsigned long)(PUP(in)) << bits;
                bits += 8;
                hold += (unsigned long)(PUP(in)) << bits;
                bits += 8;
            }
            this = dcode[hold & dmask];
          dodist:
            op = (unsigned)(this.bits);
            hold >>= op;
            bits -= op;
            op = (unsigned)(this.op);
            if (op & 16) {                      
                dist = (unsigned)(this.val);
                op &= 15;                       
                if (bits < op) {
                    hold += (unsigned long)(PUP(in)) << bits;
                    bits += 8;
                    if (bits < op) {
                        hold += (unsigned long)(PUP(in)) << bits;
                        bits += 8;
                    }
                }
                dist += (unsigned)hold & ((1U << op) - 1);
#ifdef INFLATE_STRICT
                if (dist > dmax) {
                    strm->msg = (char *)"invalid distance too far back";
                    state->mode = BAD;
                    break;
                }
#endif
                hold >>= op;
                bits -= op;
                op = (unsigned)(out - beg);     
                if (dist > op) {                
                    op = dist - op;             
                    if (op > whave) {
                        strm->msg = (char *)"invalid distance too far back";
                        state->mode = BAD;
                        break;
                    }
                    from = window - OFF;
                    if (write == 0) {           
                        from += wsize - op;
                        if (op < len) {         
                            len -= op;
                            do {
                                PUP(out) = PUP(from);
                            } while (--op);
                            from = out - dist;  
                        }
                    }
                    else if (write < op) {      
                        from += wsize + write - op;
                        op -= write;
                        if (op < len) {         
                            len -= op;
                            do {
                                PUP(out) = PUP(from);
                            } while (--op);
                            from = window - OFF;
                            if (write < len) {  
                                op = write;
                                len -= op;
                                do {
                                    PUP(out) = PUP(from);
                                } while (--op);
                                from = out - dist;      
                            }
                        }
                    }
                    else {                      
                        from += write - op;
                        if (op < len) {         
                            len -= op;
                            do {
                                PUP(out) = PUP(from);
                            } while (--op);
                            from = out - dist;  
                        }
                    }
                    while (len > 2) {
                        PUP(out) = PUP(from);
                        PUP(out) = PUP(from);
                        PUP(out) = PUP(from);
                        len -= 3;
                    }
                    if (len) {
                        PUP(out) = PUP(from);
                        if (len > 1)
                            PUP(out) = PUP(from);
                    }
                }
                else {
		    unsigned short *sout;
		    unsigned long loops;

                    from = out - dist;          
		    
		    
		    if (!((long)(out - 1 + OFF) & 1)) {
			PUP(out) = PUP(from);
			len--;
		    }
		    sout = (unsigned short *)(out - OFF);
		    if (dist > 2) {
			unsigned short *sfrom;

			sfrom = (unsigned short *)(from - OFF);
			loops = len >> 1;
			do
#ifdef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
			    PUP(sout) = PUP(sfrom);
#else
			    PUP(sout) = UP_UNALIGNED(sfrom);
#endif
			while (--loops);
			out = (unsigned char *)sout + OFF;
			from = (unsigned char *)sfrom + OFF;
		    } else { 
			unsigned short pat16;

			pat16 = *(sout-1+OFF);
			if (dist == 1) {
				union uu mm;
				
				mm.us = pat16;
				mm.b[0] = mm.b[1];
				pat16 = mm.us;
			}
			loops = len >> 1;
			do
			    PUP(sout) = pat16;
			while (--loops);
			out = (unsigned char *)sout + OFF;
		    }
		    if (len & 1)
			PUP(out) = PUP(from);
                }
            }
            else if ((op & 64) == 0) {          
                this = dcode[this.val + (hold & ((1U << op) - 1))];
                goto dodist;
            }
            else {
                strm->msg = (char *)"invalid distance code";
                state->mode = BAD;
                break;
            }
        }
        else if ((op & 64) == 0) {              
            this = lcode[this.val + (hold & ((1U << op) - 1))];
            goto dolen;
        }
        else if (op & 32) {                     
            state->mode = TYPE;
            break;
        }
        else {
            strm->msg = (char *)"invalid literal/length code";
            state->mode = BAD;
            break;
        }
    } while (in < last && out < end);

    
    len = bits >> 3;
    in -= len;
    bits -= len << 3;
    hold &= (1U << bits) - 1;

    
    strm->next_in = in + OFF;
    strm->next_out = out + OFF;
    strm->avail_in = (unsigned)(in < last ? 5 + (last - in) : 5 - (in - last));
    strm->avail_out = (unsigned)(out < end ?
                                 257 + (end - out) : 257 - (out - end));
    state->hold = hold;
    state->bits = bits;
    return;
}


#endif 
