/*
 *  Copyright (c) 2003 ARM Limited
 *  All Rights Reserved
 *  Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/linkage.h>
#include <linux/init.h>

/*
PHY define in msm_iomap-8960.h, VIRT define in msm_iomap.h
The counters to check kernel exit for both cpu's
kernel foot print for cpu0  		: phy 0x889F1000 : virt 0xFE703000
kernel foot print for cpu1  		: phy 0x889F1004 : virt 0xFE703004
kernel exit counter from cpu0		: phy 0x889F1008 : virt 0xFE703008
kernel exit counter from cpu1		: phy 0x889F100C : virt 0xFE70300C
msm_pm_boot_entry			: phy 0x889F1010 : virt 0xFE703010
msm_pm_boot_vector			: phy 0x889F1014 : virt 0xFE703014
reset vector for cpu0(init)		: phy 0x889F1018 : virt 0xFE703018
reset vector for cpu1(init)		: phy 0x889F101C : virt 0xFE70301C
cpu0 reset vector address		: phy 0x889F1020 : virt 0xFE703020
cpu1 reset vector address		: phy 0x889F1024 : virt 0xFE703024
cpu0 reset vector address value		: phy 0x889F1028 : virt 0xFE703028
cpu1 reset vector address value		: phy 0x889F102C : virt 0xFE70302C
cpu0 frequency				: phy 0x889F1030 : virt 0xFE703030
cpu1 frequency				: phy 0x889F1034 : virt 0xFE703034
L2 frequency				: phy 0x889F1038 : virt 0xFE703038
acpuclk_set_rate footprint cpu0		: phy 0x889F103C : virt 0xFE70303C
acpuclk_set_rate footprint cpu1		: phy 0x889F1040 : virt 0xFE703040
*/

#define MSM_KERNEL_FOOTPRINT_PHYS	0x889F1000
#define MSM_KERNEL_FOOTPRINT_VIRT	0xFE703000

#define CPU1_EXIT_COUNT_PHYS	(MSM_KERNEL_FOOTPRINT_PHYS + 0xc)
#define CPU_FOOT_PRINT_MAGIC			0xACBDFE00
#define CPU_FOOT_PRINT_BASE_CPU1_PHY	(MSM_KERNEL_FOOTPRINT_PHYS + 0x4)

/*
 * MSM specific entry point for secondary CPUs.  This provides
 * a "holding pen" into which all secondary cores are held until we're
 * ready for them to initialise.
 *
 * This is executing in physical space with cache's off.
 */
ENTRY(msm_secondary_startup)
	ldr	r0, =CPU1_EXIT_COUNT_PHYS
	ldr     r4, [r0]
	add 	r4, r4, #1
	str	r4, [r0]
	dsb

	ldr	r0, =CPU_FOOT_PRINT_BASE_CPU1_PHY
	ldr	r4, =0xACBDFE04	/* make status number. */
	str	r4, [r0]
	dsb

	mrc	p15, 0, r0, c0, c0, 5 	@ MPIDR
	and	r0, r0, #15		@ What CPU am I
	adr	r4, 1f			@ address of
	ldmia	r4, {r5, r6}		@ load curr addr and pen_rel addr
	sub	r4, r4, r5		@ determine virtual/phys offsets
	add	r6, r6, r4		@ apply
pen:
	wfe
	dsb				@ ensure subsequent access is
					@ after event

	ldr	r7, [r6]		@ pen_rel has cpu to remove from reset
	cmp	r7, r0			@ are we lucky?
	bne	pen

	/*
	 * we've been released from the holding pen: secondary_stack
	 * should now contain the SVC stack for this core
	 */
	mvn	r7, #0			@ -1 to registers
	str r7,[r6]			@ back to the pen for ack
	b	secondary_startup

1:	.long	.
	.long	pen_release
