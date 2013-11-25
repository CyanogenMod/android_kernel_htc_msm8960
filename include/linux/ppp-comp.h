/*
 * ppp-comp.h - Definitions for doing PPP packet compression.
 *
 * Copyright 1994-1998 Paul Mackerras.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */
#ifndef _NET_PPP_COMP_H
#define _NET_PPP_COMP_H

struct module;


#ifndef DO_BSD_COMPRESS
#define DO_BSD_COMPRESS	1	
#endif
#ifndef DO_DEFLATE
#define DO_DEFLATE	1	
#endif
#define DO_PREDICTOR_1	0
#define DO_PREDICTOR_2	0


struct compressor {
	int	compress_proto;	

	
	void	*(*comp_alloc) (unsigned char *options, int opt_len);

	
	void	(*comp_free) (void *state);

	
	int	(*comp_init) (void *state, unsigned char *options,
			      int opt_len, int unit, int opthdr, int debug);

	
	void	(*comp_reset) (void *state);

	
	int     (*compress) (void *state, unsigned char *rptr,
			      unsigned char *obuf, int isize, int osize);

	
	void	(*comp_stat) (void *state, struct compstat *stats);

	
	void	*(*decomp_alloc) (unsigned char *options, int opt_len);

	
	void	(*decomp_free) (void *state);

	
	int	(*decomp_init) (void *state, unsigned char *options,
				int opt_len, int unit, int opthdr, int mru,
				int debug);

	
	void	(*decomp_reset) (void *state);

	
	int	(*decompress) (void *state, unsigned char *ibuf, int isize,
				unsigned char *obuf, int osize);

	
	void	(*incomp) (void *state, unsigned char *ibuf, int icnt);

	
	void	(*decomp_stat) (void *state, struct compstat *stats);

	
	struct module *owner;
	
	unsigned int comp_extra;
};


#define DECOMP_ERROR		-1	
#define DECOMP_FATALERROR	-2	


#define CCP_CONFREQ	1
#define CCP_CONFACK	2
#define CCP_TERMREQ	5
#define CCP_TERMACK	6
#define CCP_RESETREQ	14
#define CCP_RESETACK	15


#define CCP_MAX_OPTION_LENGTH	32


#define CCP_CODE(dp)		((dp)[0])
#define CCP_ID(dp)		((dp)[1])
#define CCP_LENGTH(dp)		(((dp)[2] << 8) + (dp)[3])
#define CCP_HDRLEN		4

#define CCP_OPT_CODE(dp)	((dp)[0])
#define CCP_OPT_LENGTH(dp)	((dp)[1])
#define CCP_OPT_MINLEN		2


#define CI_BSD_COMPRESS		21	
#define CILEN_BSD_COMPRESS	3	

#define BSD_NBITS(x)		((x) & 0x1F)	
#define BSD_VERSION(x)		((x) >> 5)	
#define BSD_CURRENT_VERSION	1		
#define BSD_MAKE_OPT(v, n)	(((v) << 5) | (n))

#define BSD_MIN_BITS		9	
#define BSD_MAX_BITS		15	


#define CI_DEFLATE		26	
#define CI_DEFLATE_DRAFT	24	
#define CILEN_DEFLATE		4	

#define DEFLATE_MIN_SIZE	9
#define DEFLATE_MAX_SIZE	15
#define DEFLATE_METHOD_VAL	8
#define DEFLATE_SIZE(x)		(((x) >> 4) + 8)
#define DEFLATE_METHOD(x)	((x) & 0x0F)
#define DEFLATE_MAKE_OPT(w)	((((w) - 8) << 4) + DEFLATE_METHOD_VAL)
#define DEFLATE_CHK_SEQUENCE	0


#define CI_MPPE                18      
#define CILEN_MPPE              6      


#define CI_PREDICTOR_1		1	
#define CILEN_PREDICTOR_1	2	
#define CI_PREDICTOR_2		2	
#define CILEN_PREDICTOR_2	2	

#ifdef __KERNEL__
extern int ppp_register_compressor(struct compressor *);
extern void ppp_unregister_compressor(struct compressor *);
#endif 

#endif 
