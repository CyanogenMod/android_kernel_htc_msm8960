/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#ifndef __mrcmcr__h_
#define __mrcmcr__h_


#ifdef __ASSEMBLY__


#define MRC(reg, processor, op1, crn, crm, op2) \
(mrc      processor , op1 , reg,  crn , crm , op2)

#define MCR(reg, processor, op1, crn, crm, op2) \
(mcr      processor , op1 , reg,  crn , crm , op2)

#else

#define MRC(reg, processor, op1, crn, crm, op2) \
__asm__ __volatile__ ( \
"   mrc   "   #processor "," #op1 ", %0,"  #crn "," #crm "," #op2 "\n" \
: "=r" (reg))

#define MCR(reg, processor, op1, crn, crm, op2) \
__asm__ __volatile__ ( \
"   mcr   "   #processor "," #op1 ", %0,"  #crn "," #crm "," #op2 "\n" \
: : "r" (reg))
#endif


#define MRC15(reg, op1, crn, crm, op2) MRC(reg, p15, op1, crn, crm, op2)
#define MCR15(reg, op1, crn, crm, op2) MCR(reg, p15, op1, crn, crm, op2)

#endif
