/* tuner-xc2028_types
 *
 * This file includes internal tipes to be used inside tuner-xc2028.
 * Shouldn't be included outside tuner-xc2028
 *
 * Copyright (c) 2007-2008 Mauro Carvalho Chehab (mchehab@infradead.org)
 * This code is placed under the terms of the GNU General Public License v2
 */


#define BASE		(1<<0)
#define BASE_TYPES	(BASE|F8MHZ|MTS|FM|INPUT1|INPUT2|INIT1)

#define F8MHZ		(1<<1)

#define MTS		(1<<2)

#define D2620		(1<<3)
#define D2633		(1<<4)

#define DTV6           (1 << 5)
#define QAM            (1 << 6)
#define DTV7		(1<<7)
#define DTV78		(1<<8)
#define DTV8		(1<<9)

#define DTV_TYPES	(D2620|D2633|DTV6|QAM|DTV7|DTV78|DTV8|ATSC)

#define	FM		(1<<10)

#define STD_SPECIFIC_TYPES (MTS|FM|LCD|NOGD)

#define INPUT1		(1<<11)


#define LCD		(1<<12)

#define NOGD		(1<<13)

#define INIT1		(1<<14)

#define MONO           (1 << 15)
#define ATSC           (1 << 16)
#define IF             (1 << 17)
#define LG60           (1 << 18)
#define ATI638         (1 << 19)
#define OREN538        (1 << 20)
#define OREN36         (1 << 21)
#define TOYOTA388      (1 << 22)
#define TOYOTA794      (1 << 23)
#define DIBCOM52       (1 << 24)
#define ZARLINK456     (1 << 25)
#define CHINA          (1 << 26)
#define F6MHZ          (1 << 27)
#define INPUT2         (1 << 28)
#define SCODE          (1 << 29)

#define HAS_IF         (1 << 30)

#define SCODE_TYPES (SCODE | MTS)



#define V4L2_STD_SECAM_K3	(0x04000000)


#define V4L2_STD_A2_A		(1LL<<32)
#define V4L2_STD_A2_B		(1LL<<33)
#define V4L2_STD_NICAM_A	(1LL<<34)
#define V4L2_STD_NICAM_B	(1LL<<35)
#define V4L2_STD_AM		(1LL<<36)
#define V4L2_STD_BTSC		(1LL<<37)
#define V4L2_STD_EIAJ		(1LL<<38)

#define V4L2_STD_A2		(V4L2_STD_A2_A    | V4L2_STD_A2_B)
#define V4L2_STD_NICAM		(V4L2_STD_NICAM_A | V4L2_STD_NICAM_B)


#define V4L2_STD_AUDIO		(V4L2_STD_A2    | \
				 V4L2_STD_NICAM | \
				 V4L2_STD_AM    | \
				 V4L2_STD_BTSC  | \
				 V4L2_STD_EIAJ)


#define V4L2_STD_PAL_BG_A2_A	(V4L2_STD_PAL_BG | V4L2_STD_A2_A)
#define V4L2_STD_PAL_BG_A2_B	(V4L2_STD_PAL_BG | V4L2_STD_A2_B)
#define V4L2_STD_PAL_BG_NICAM_A	(V4L2_STD_PAL_BG | V4L2_STD_NICAM_A)
#define V4L2_STD_PAL_BG_NICAM_B	(V4L2_STD_PAL_BG | V4L2_STD_NICAM_B)
#define V4L2_STD_PAL_DK_A2	(V4L2_STD_PAL_DK | V4L2_STD_A2)
#define V4L2_STD_PAL_DK_NICAM	(V4L2_STD_PAL_DK | V4L2_STD_NICAM)
#define V4L2_STD_SECAM_L_NICAM	(V4L2_STD_SECAM_L | V4L2_STD_NICAM)
#define V4L2_STD_SECAM_L_AM	(V4L2_STD_SECAM_L | V4L2_STD_AM)
