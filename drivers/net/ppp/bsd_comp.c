/*
 * Update: The Berkeley copyright was changed, and the change
 * is retroactive to all "true" BSD software (ie everything
 * from UCB as opposed to other peoples code that just carried
 * the same license). The new copyright doesn't clash with the
 * GPL, so the module-only restriction has been removed..
 */

/* Because this code is derived from the 4.3BSD compress source:
 *
 * Copyright (c) 1985, 1986 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * James A. Woods, derived from original work by Spencer Thomas
 * and Joseph Orost.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/string.h>

#include <linux/ppp_defs.h>

#undef   PACKETPTR
#define  PACKETPTR 1
#include <linux/ppp-comp.h>
#undef   PACKETPTR

#include <asm/byteorder.h>



#define BSD_VERSION(x)	((x) >> 5)
#define BSD_NBITS(x)	((x) & 0x1F)

#define BSD_CURRENT_VERSION	1


struct bsd_dict {
    union {				
	unsigned long	fcode;
	struct {
#if defined(__LITTLE_ENDIAN)		
	    unsigned short	prefix;	
	    unsigned char	suffix; 
	    unsigned char	pad;
#elif defined(__BIG_ENDIAN)		
	    unsigned char	pad;
	    unsigned char	suffix; 
	    unsigned short	prefix; 
#else
#error Endianness not defined...
#endif
	} hs;
    } f;
    unsigned short codem1;		
    unsigned short cptr;		
};

struct bsd_db {
    int	    totlen;			
    unsigned int   hsize;		
    unsigned char  hshift;		
    unsigned char  n_bits;		
    unsigned char  maxbits;		
    unsigned char  debug;		
    unsigned char  unit;		
    unsigned short seqno;		
    unsigned int   mru;			
    unsigned int   maxmaxcode;		
    unsigned int   max_ent;		
    unsigned int   in_count;		
    unsigned int   bytes_out;		
    unsigned int   ratio;		
    unsigned int   checkpoint;		
    unsigned int   clear_count;		
    unsigned int   incomp_count;	
    unsigned int   incomp_bytes;	
    unsigned int   uncomp_count;	
    unsigned int   uncomp_bytes;	
    unsigned int   comp_count;		
    unsigned int   comp_bytes;		
    unsigned short  *lens;		
    struct bsd_dict *dict;		
};

#define BSD_OVHD	2		
#define MIN_BSD_BITS	9
#define BSD_INIT_BITS	MIN_BSD_BITS
#define MAX_BSD_BITS	15

static void	bsd_free (void *state);
static void	*bsd_alloc(unsigned char *options, int opt_len, int decomp);
static void	*bsd_comp_alloc (unsigned char *options, int opt_len);
static void	*bsd_decomp_alloc (unsigned char *options, int opt_len);

static int	bsd_init        (void *db, unsigned char *options,
			         int opt_len, int unit, int debug, int decomp);
static int	bsd_comp_init   (void *state, unsigned char *options,
			         int opt_len, int unit, int opthdr, int debug);
static int	bsd_decomp_init (void *state, unsigned char *options,
				 int opt_len, int unit, int opthdr, int mru,
				 int debug);

static void	bsd_reset (void *state);
static void	bsd_comp_stats (void *state, struct compstat *stats);

static int	bsd_compress (void *state, unsigned char *rptr,
			      unsigned char *obuf, int isize, int osize);
static void	bsd_incomp (void *state, unsigned char *ibuf, int icnt);

static int	bsd_decompress (void *state, unsigned char *ibuf, int isize,
				unsigned char *obuf, int osize);

extern int  ppp_register_compressor   (struct compressor *cp);
extern void ppp_unregister_compressor (struct compressor *cp);

#define CLEAR	256			
#define FIRST	257			
#define LAST	255

#define MAXCODE(b)	((1 << (b)) - 1)
#define BADCODEM1	MAXCODE(MAX_BSD_BITS)

#define BSD_HASH(prefix,suffix,hshift) ((((unsigned long)(suffix))<<(hshift)) \
					 ^ (unsigned long)(prefix))
#define BSD_KEY(prefix,suffix)		((((unsigned long)(suffix)) << 16) \
					 + (unsigned long)(prefix))

#define CHECK_GAP	10000		

#define RATIO_SCALE_LOG	8
#define RATIO_SCALE	(1<<RATIO_SCALE_LOG)
#define RATIO_MAX	(0x7fffffff>>RATIO_SCALE_LOG)


static void
bsd_clear(struct bsd_db *db)
{
    db->clear_count++;
    db->max_ent      = FIRST-1;
    db->n_bits       = BSD_INIT_BITS;
    db->bytes_out    = 0;
    db->in_count     = 0;
    db->ratio	     = 0;
    db->checkpoint   = CHECK_GAP;
}


static int bsd_check (struct bsd_db *db)	
  {
    unsigned int new_ratio;

    if (db->in_count >= db->checkpoint)
      {
	
	if (db->in_count >= RATIO_MAX || db->bytes_out >= RATIO_MAX)
	  {
	    db->in_count  -= (db->in_count  >> 2);
	    db->bytes_out -= (db->bytes_out >> 2);
	  }

	db->checkpoint = db->in_count + CHECK_GAP;

	if (db->max_ent >= db->maxmaxcode)
	  {

	    new_ratio = db->in_count << RATIO_SCALE_LOG;
	    if (db->bytes_out != 0)
	      {
		new_ratio /= db->bytes_out;
	      }

	    if (new_ratio < db->ratio || new_ratio < 1 * RATIO_SCALE)
	      {
		bsd_clear (db);
		return 1;
	      }
	    db->ratio = new_ratio;
	  }
      }
    return 0;
  }


static void bsd_comp_stats (void *state, struct compstat *stats)
  {
    struct bsd_db *db = (struct bsd_db *) state;

    stats->unc_bytes    = db->uncomp_bytes;
    stats->unc_packets  = db->uncomp_count;
    stats->comp_bytes   = db->comp_bytes;
    stats->comp_packets = db->comp_count;
    stats->inc_bytes    = db->incomp_bytes;
    stats->inc_packets  = db->incomp_count;
    stats->in_count     = db->in_count;
    stats->bytes_out    = db->bytes_out;
  }


static void bsd_reset (void *state)
  {
    struct bsd_db *db = (struct bsd_db *) state;

    bsd_clear(db);

    db->seqno       = 0;
    db->clear_count = 0;
  }


static void bsd_free (void *state)
{
	struct bsd_db *db = state;

	if (!db)
		return;

	vfree(db->dict);
	db->dict = NULL;
	vfree(db->lens);
	db->lens = NULL;
	kfree(db);
}


static void *bsd_alloc (unsigned char *options, int opt_len, int decomp)
  {
    int bits;
    unsigned int hsize, hshift, maxmaxcode;
    struct bsd_db *db;

    if (opt_len != 3 || options[0] != CI_BSD_COMPRESS || options[1] != 3
	|| BSD_VERSION(options[2]) != BSD_CURRENT_VERSION)
      {
	return NULL;
      }

    bits = BSD_NBITS(options[2]);

    switch (bits)
      {
    case 9:			
    case 10:			
    case 11:			
    case 12:			
	hsize = 5003;
	hshift = 4;
	break;
    case 13:			
	hsize = 9001;
	hshift = 5;
	break;
    case 14:			
	hsize = 18013;
	hshift = 6;
	break;
    case 15:			
	hsize = 35023;
	hshift = 7;
	break;
    case 16:			
		
		
	
    default:
	return NULL;
      }
    maxmaxcode = MAXCODE(bits);
    db         = kzalloc(sizeof (struct bsd_db),
					    GFP_KERNEL);
    if (!db)
      {
	return NULL;
      }

    db->dict = vmalloc(hsize * sizeof(struct bsd_dict));
    if (!db->dict)
      {
	bsd_free (db);
	return NULL;
      }

    if (!decomp)
      {
	db->lens = NULL;
      }
    else
      {
        db->lens = vmalloc((maxmaxcode + 1) * sizeof(db->lens[0]));
	if (!db->lens)
	  {
	    bsd_free (db);
	    return NULL;
	  }
      }
    db->totlen     = sizeof (struct bsd_db)   +
      		    (sizeof (struct bsd_dict) * hsize);

    db->hsize      = hsize;
    db->hshift     = hshift;
    db->maxmaxcode = maxmaxcode;
    db->maxbits    = bits;

    return (void *) db;
  }

static void *bsd_comp_alloc (unsigned char *options, int opt_len)
  {
    return bsd_alloc (options, opt_len, 0);
  }

static void *bsd_decomp_alloc (unsigned char *options, int opt_len)
  {
    return bsd_alloc (options, opt_len, 1);
  }


static int bsd_init (void *state, unsigned char *options,
		     int opt_len, int unit, int debug, int decomp)
  {
    struct bsd_db *db = state;
    int indx;

    if ((opt_len != 3) || (options[0] != CI_BSD_COMPRESS) || (options[1] != 3)
	|| (BSD_VERSION(options[2]) != BSD_CURRENT_VERSION)
	|| (BSD_NBITS(options[2]) != db->maxbits)
	|| (decomp && db->lens == NULL))
      {
	return 0;
      }

    if (decomp)
      {
	indx = LAST;
	do
	  {
	    db->lens[indx] = 1;
	  }
	while (indx-- > 0);
      }

    indx = db->hsize;
    while (indx-- != 0)
      {
	db->dict[indx].codem1 = BADCODEM1;
	db->dict[indx].cptr   = 0;
      }

    db->unit = unit;
    db->mru  = 0;
#ifndef DEBUG
    if (debug)
#endif
      db->debug = 1;

    bsd_reset(db);

    return 1;
  }

static int bsd_comp_init (void *state, unsigned char *options,
			  int opt_len, int unit, int opthdr, int debug)
  {
    return bsd_init (state, options, opt_len, unit, debug, 0);
  }

static int bsd_decomp_init (void *state, unsigned char *options,
			    int opt_len, int unit, int opthdr, int mru,
			    int debug)
  {
    return bsd_init (state, options, opt_len, unit, debug, 1);
  }


#define dict_ptrx(p,idx) &(p->dict[idx])
#define lens_ptrx(p,idx) &(p->lens[idx])

#ifdef DEBUG
static unsigned short *lens_ptr(struct bsd_db *db, int idx)
  {
    if ((unsigned int) idx > (unsigned int) db->maxmaxcode)
      {
	printk ("<9>ppp: lens_ptr(%d) > max\n", idx);
	idx = 0;
      }
    return lens_ptrx (db, idx);
  }

static struct bsd_dict *dict_ptr(struct bsd_db *db, int idx)
  {
    if ((unsigned int) idx >= (unsigned int) db->hsize)
      {
	printk ("<9>ppp: dict_ptr(%d) > max\n", idx);
	idx = 0;
      }
    return dict_ptrx (db, idx);
  }

#else
#define lens_ptr(db,idx) lens_ptrx(db,idx)
#define dict_ptr(db,idx) dict_ptrx(db,idx)
#endif


static int bsd_compress (void *state, unsigned char *rptr, unsigned char *obuf,
			 int isize, int osize)
  {
    struct bsd_db *db;
    int hshift;
    unsigned int max_ent;
    unsigned int n_bits;
    unsigned int bitno;
    unsigned long accm;
    int ent;
    unsigned long fcode;
    struct bsd_dict *dictp;
    unsigned char c;
    int hval;
    int disp;
    int ilen;
    int mxcode;
    unsigned char *wptr;
    int olen;

#define PUTBYTE(v)			\
  {					\
    ++olen;				\
    if (wptr)				\
      {					\
	*wptr++ = (unsigned char) (v);	\
	if (olen >= osize)		\
	  {				\
	    wptr = NULL;		\
	  }				\
      }					\
  }

#define OUTPUT(ent)			\
  {					\
    bitno -= n_bits;			\
    accm |= ((ent) << bitno);		\
    do					\
      {					\
	PUTBYTE(accm >> 24);		\
	accm <<= 8;			\
	bitno += 8;			\
      }					\
    while (bitno <= 24);		\
  }


    ent = PPP_PROTOCOL(rptr);
    if (ent < 0x21 || ent > 0xf9)
      {
	return 0;
      }

    db      = (struct bsd_db *) state;
    hshift  = db->hshift;
    max_ent = db->max_ent;
    n_bits  = db->n_bits;
    bitno   = 32;
    accm    = 0;
    mxcode  = MAXCODE (n_bits);

    
    wptr  = obuf;
    olen  = PPP_HDRLEN + BSD_OVHD;

    if (osize > isize)
      {
	osize = isize;
      }

    
    if (wptr)
      {
	*wptr++ = PPP_ADDRESS(rptr);
	*wptr++ = PPP_CONTROL(rptr);
	*wptr++ = 0;
	*wptr++ = PPP_COMP;
	*wptr++ = db->seqno >> 8;
	*wptr++ = db->seqno;
      }

    
    rptr  += PPP_HDRLEN;
    isize -= PPP_HDRLEN;
    ilen   = ++isize;	

    while (--ilen > 0)
      {
	c     = *rptr++;
	fcode = BSD_KEY  (ent, c);
	hval  = BSD_HASH (ent, c, hshift);
	dictp = dict_ptr (db, hval);

	
	if (dictp->codem1 >= max_ent)
	  {
	    goto nomatch;
	  }

	if (dictp->f.fcode == fcode)
	  {
	    ent = dictp->codem1 + 1;
	    continue;	
	  }

	
	disp = (hval == 0) ? 1 : hval;

	do
	  {
	    hval += disp;
	    if (hval >= db->hsize)
	      {
		hval -= db->hsize;
	      }
	    dictp = dict_ptr (db, hval);
	    if (dictp->codem1 >= max_ent)
	      {
		goto nomatch;
	      }
	  }
	while (dictp->f.fcode != fcode);

	ent = dictp->codem1 + 1;	
	continue;

nomatch:
	OUTPUT(ent);		

	
	if (max_ent < db->maxmaxcode)
	  {
	    struct bsd_dict *dictp2;
	    struct bsd_dict *dictp3;
	    int    indx;

	    
	    if (max_ent >= mxcode)
	      {
		db->n_bits = ++n_bits;
		mxcode     = MAXCODE (n_bits);
	      }


	    dictp2 = dict_ptr (db, max_ent + 1);
	    indx   = dictp2->cptr;
	    dictp3 = dict_ptr (db, indx);

	    if (dictp3->codem1 == max_ent)
	      {
		dictp3->codem1 = BADCODEM1;
	      }

	    dictp2->cptr   = hval;
	    dictp->codem1  = max_ent;
	    dictp->f.fcode = fcode;
	    db->max_ent    = ++max_ent;

	    if (db->lens)
	      {
		unsigned short *len1 = lens_ptr (db, max_ent);
		unsigned short *len2 = lens_ptr (db, ent);
		*len1 = *len2 + 1;
	      }
	  }
	ent = c;
      }

    OUTPUT(ent);		

    db->bytes_out    += olen - PPP_HDRLEN - BSD_OVHD;
    db->uncomp_bytes += isize;
    db->in_count     += isize;
    ++db->uncomp_count;
    ++db->seqno;

    if (bitno < 32)
      {
	++db->bytes_out; 
      }


    if (bsd_check(db))
      {
	OUTPUT (CLEAR);
      }


    if (bitno != 32)
      {
	PUTBYTE((accm | (0xff << (bitno-8))) >> 24);
      }


    if (max_ent >= mxcode && max_ent < db->maxmaxcode)
      {
	db->n_bits++;
      }

    
    if (wptr == NULL)
      {
	++db->incomp_count;
	db->incomp_bytes += isize;
	olen              = 0;
      }
    else 
      {
	++db->comp_count;
	db->comp_bytes += olen;
      }

    
    return olen;
#undef OUTPUT
#undef PUTBYTE
  }


static void bsd_incomp (void *state, unsigned char *ibuf, int icnt)
  {
    (void) bsd_compress (state, ibuf, (char *) 0, icnt, 0);
  }


static int bsd_decompress (void *state, unsigned char *ibuf, int isize,
			   unsigned char *obuf, int osize)
  {
    struct bsd_db *db;
    unsigned int max_ent;
    unsigned long accm;
    unsigned int bitno;		
    unsigned int n_bits;
    unsigned int tgtbitno;	
    struct bsd_dict *dictp;
    int explen;
    int seq;
    unsigned int incode;
    unsigned int oldcode;
    unsigned int finchar;
    unsigned char *p;
    unsigned char *wptr;
    int adrs;
    int ctrl;
    int ilen;
    int codelen;
    int extra;

    db       = (struct bsd_db *) state;
    max_ent  = db->max_ent;
    accm     = 0;
    bitno    = 32;		
    n_bits   = db->n_bits;
    tgtbitno = 32 - n_bits;	


    adrs  = PPP_ADDRESS (ibuf);
    ctrl  = PPP_CONTROL (ibuf);

    seq   = (ibuf[4] << 8) + ibuf[5];

    ibuf += (PPP_HDRLEN + 2);
    ilen  = isize - (PPP_HDRLEN + 2);


    if (seq != db->seqno)
      {
	if (db->debug)
	  {
	    printk("bsd_decomp%d: bad sequence # %d, expected %d\n",
		   db->unit, seq, db->seqno - 1);
	  }
	return DECOMP_ERROR;
      }

    ++db->seqno;
    db->bytes_out += ilen;


    wptr    = obuf;
    *wptr++ = adrs;
    *wptr++ = ctrl;
    *wptr++ = 0;

    oldcode = CLEAR;
    explen  = 3;


    for (;;)
      {
	if (ilen-- <= 0)
	  {
	    db->in_count += (explen - 3); 
	    break;
	  }


	bitno -= 8;
	accm  |= *ibuf++ << bitno;
	if (tgtbitno < bitno)
	  {
	    continue;
	  }

	incode = accm >> tgtbitno;
	accm <<= n_bits;
	bitno += n_bits;


	if (incode == CLEAR)
	  {
	    if (ilen > 0)
	      {
		if (db->debug)
		  {
		    printk("bsd_decomp%d: bad CLEAR\n", db->unit);
		  }
		return DECOMP_FATALERROR;	
	      }

	    bsd_clear(db);
	    break;
	  }

	if ((incode > max_ent + 2) || (incode > db->maxmaxcode)
	    || (incode > max_ent && oldcode == CLEAR))
	  {
	    if (db->debug)
	      {
		printk("bsd_decomp%d: bad code 0x%x oldcode=0x%x ",
		       db->unit, incode, oldcode);
		printk("max_ent=0x%x explen=%d seqno=%d\n",
		       max_ent, explen, db->seqno);
	      }
	    return DECOMP_FATALERROR;	
	  }

	
	if (incode > max_ent)
	  {
	    finchar = oldcode;
	    extra   = 1;
	  }
	else
	  {
	    finchar = incode;
	    extra   = 0;
	  }

	codelen = *(lens_ptr (db, finchar));
	explen += codelen + extra;
	if (explen > osize)
	  {
	    if (db->debug)
	      {
		printk("bsd_decomp%d: ran out of mru\n", db->unit);
#ifdef DEBUG
		printk("  len=%d, finchar=0x%x, codelen=%d, explen=%d\n",
		       ilen, finchar, codelen, explen);
#endif
	      }
	    return DECOMP_FATALERROR;
	  }


	wptr += codelen;
	p     = wptr;
	while (finchar > LAST)
	  {
	    struct bsd_dict *dictp2 = dict_ptr (db, finchar);

	    dictp = dict_ptr (db, dictp2->cptr);
#ifdef DEBUG
	    if (--codelen <= 0 || dictp->codem1 != finchar-1)
	      {
		if (codelen <= 0)
		  {
		    printk("bsd_decomp%d: fell off end of chain ", db->unit);
		    printk("0x%x at 0x%x by 0x%x, max_ent=0x%x\n",
			   incode, finchar, dictp2->cptr, max_ent);
		  }
		else
		  {
		    if (dictp->codem1 != finchar-1)
		      {
			printk("bsd_decomp%d: bad code chain 0x%x "
			       "finchar=0x%x ",
			       db->unit, incode, finchar);

			printk("oldcode=0x%x cptr=0x%x codem1=0x%x\n",
			       oldcode, dictp2->cptr, dictp->codem1);
		      }
		  }
		return DECOMP_FATALERROR;
	      }
#endif
	    *--p    = dictp->f.hs.suffix;
	    finchar = dictp->f.hs.prefix;
	  }
	*--p = finchar;

#ifdef DEBUG
	if (--codelen != 0)
	  {
	    printk("bsd_decomp%d: short by %d after code 0x%x, max_ent=0x%x\n",
		   db->unit, codelen, incode, max_ent);
	  }
#endif

	if (extra)		
	  {
	    *wptr++ = finchar;
	  }


	if (oldcode != CLEAR && max_ent < db->maxmaxcode)
	  {
	    struct bsd_dict *dictp2, *dictp3;
	    unsigned short  *lens1,  *lens2;
	    unsigned long fcode;
	    int hval, disp, indx;

	    fcode = BSD_KEY(oldcode,finchar);
	    hval  = BSD_HASH(oldcode,finchar,db->hshift);
	    dictp = dict_ptr (db, hval);

	    
	    if (dictp->codem1 < max_ent)
	      {
		disp = (hval == 0) ? 1 : hval;
		do
		  {
		    hval += disp;
		    if (hval >= db->hsize)
		      {
			hval -= db->hsize;
		      }
		    dictp = dict_ptr (db, hval);
		  }
		while (dictp->codem1 < max_ent);
	      }


	    dictp2 = dict_ptr (db, max_ent + 1);
	    indx   = dictp2->cptr;
	    dictp3 = dict_ptr (db, indx);

	    if (dictp3->codem1 == max_ent)
	      {
		dictp3->codem1 = BADCODEM1;
	      }

	    dictp2->cptr   = hval;
	    dictp->codem1  = max_ent;
	    dictp->f.fcode = fcode;
	    db->max_ent    = ++max_ent;

	    
	    lens1  = lens_ptr (db, max_ent);
	    lens2  = lens_ptr (db, oldcode);
	    *lens1 = *lens2 + 1;

	    
	    if (max_ent >= MAXCODE(n_bits) && max_ent < db->maxmaxcode)
	      {
		db->n_bits = ++n_bits;
		tgtbitno   = 32-n_bits;
	      }
	  }
	oldcode = incode;
      }

    ++db->comp_count;
    ++db->uncomp_count;
    db->comp_bytes   += isize - BSD_OVHD - PPP_HDRLEN;
    db->uncomp_bytes += explen;

    if (bsd_check(db))
      {
	if (db->debug)
	  {
	    printk("bsd_decomp%d: peer should have cleared dictionary on %d\n",
		   db->unit, db->seqno - 1);
	  }
      }
    return explen;
  }


static struct compressor ppp_bsd_compress = {
	.compress_proto =	CI_BSD_COMPRESS,
	.comp_alloc =		bsd_comp_alloc,
	.comp_free =		bsd_free,
	.comp_init =		bsd_comp_init,
	.comp_reset =		bsd_reset,
	.compress =		bsd_compress,
	.comp_stat =		bsd_comp_stats,
	.decomp_alloc =		bsd_decomp_alloc,
	.decomp_free =		bsd_free,
	.decomp_init =		bsd_decomp_init,
	.decomp_reset =		bsd_reset,
	.decompress =		bsd_decompress,
	.incomp =		bsd_incomp,
	.decomp_stat =		bsd_comp_stats,
	.owner =		THIS_MODULE
};


static int __init bsdcomp_init(void)
{
	int answer = ppp_register_compressor(&ppp_bsd_compress);
	if (answer == 0)
		printk(KERN_INFO "PPP BSD Compression module registered\n");
	return answer;
}

static void __exit bsdcomp_cleanup(void)
{
	ppp_unregister_compressor(&ppp_bsd_compress);
}

module_init(bsdcomp_init);
module_exit(bsdcomp_cleanup);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_ALIAS("ppp-compress-" __stringify(CI_BSD_COMPRESS));
