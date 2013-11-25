/*
 *  Copyright (c) 2003 ARM Limited
 *  All Rights Reserved
 *  Copyright (c) 2010, 2012 Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/linkage.h>
#include <linux/init.h>
#include <mach/msm_iomap.h>
/*
PHY define in msm_iomap-8960.h, VIRT define in msm_iomap.h
The counters to check kernel exit for both cpu's
kernel foot print for cpu0  		: phy 0x8F1F1000 : virt 0xFB600000
kernel foot print for cpu1  		: phy 0x8F1F1004 : virt 0xFB600004
kernel foot print for cpu2 		: phy 0x8F1F1008 : virt 0xFB600008
kernel foot print for cpu3 		: phy 0x8F1F100C : virt 0xFB60000C
kernel exit counter from cpu0		: phy 0x8F1F1010 : virt 0xFB600010
kernel exit counter from cpu1		: phy 0x8F1F1014 : virt 0xFB600014
kernel exit counter from cpu2		: phy 0x8F1F1018 : virt 0xFB600018
kernel exit counter from cpu3		: phy 0x8F1F101C : virt 0xFB60001C
msm_pm_boot_entry       		: phy 0x8F1F1020 : virt 0xFB600020
msm_pm_boot_vector      		: phy 0x8F1F1024 : virt 0xFB600024
reset vector for cpu0(init)		: phy 0x8F1F1028 : virt 0xFB600028
reset vector for cpu1(init)     	: phy 0x8F1F102C : virt 0xFB60002C
reset vector for cpu2(init)		: phy 0x8F1F1030 : virt 0xFB600030
reset vector for cpu3(init)	        : phy 0x8F1F1034 : virt 0xFB600034
cpu0 reset vector address		: phy 0x8F1F1038 : virt 0xFB600038
cpu1 reset vector address		: phy 0x8F1F103C : virt 0xFB60003C
cpu2 reset vector address       	: phy 0x8F1F1040 : virt 0xFB600040
cpu3 reset vector address	        : phy 0x8F1F1044 : virt 0xFB600044
cpu0 reset vector address value 	: phy 0x8F1F1048 : virt 0xFB600048
cpu1 reset vector address value		: phy 0x8F1F104C : virt 0xFB60004C
cpu2 reset vector address value	   	: phy 0x8F1F1050 : virt 0xFB600050
cpu3 reset vector address value	        : phy 0x8F1F1054 : virt 0xFB600054
cpu0 frequency          		: phy 0x8F1F1058 : virt 0xFB600058
cpu1 frequency          		: phy 0x8F1F105C : virt 0xFB60005C
cpu2 frequency                          : phy 0x8F1F1060 : virt 0xFB600060
cpu3 frequency                          : phy 0x8F1F1064 : virt 0xFB600064
L2 frequency   		           	: phy 0x8F1F1068 : virt 0xFB600068
acpuclk_set_rate footprint cpu0         : phy 0x8F1F106C : virt 0xFB60006C
acpuclk_set_rate footprint cpu1         : phy 0x8F1F1070 : virt 0xFB600070
acpuclk_set_rate footprint cpu2         : phy 0x8F1F1074 : virt 0xFB600074
acpuclk_set_rate footprint cpu3         : phy 0x8F1F1078 : virt 0xFB600078
io footprint cpu 0			: phy 0x8F1F107C-0x8F1F1084 : virt 0xFB60007C-0xFB600084
io footprint cpu 1			: phy 0x8F1F1088-0x8F1F1090 : virt 0xFB600088-0xFB600090
io footprint cpu 2			: phy 0x8F1F1094-0x8F1F109C : virt 0xFB600094-0xFB60009C
io footprint cpu 3			: phy 0x8F1F10A0-0x8F1F10A8 : virt 0xFB6000A0-0xFB6000A8
*/

#define CPU0_EXIT_COUNT_PHYS	(MSM_KERNEL_FOOTPRINT_PHYS + 0x10)
#define CPU_FOOT_PRINT_MAGIC			0xACBDFE00
#define CPU_FOOT_PRINT_BASE_CPU0_PHY	(MSM_KERNEL_FOOTPRINT_PHYS)

__CPUINIT

/*
 * MSM specific entry point for secondary CPUs.  This provides
 * a "holding pen" into which all secondary cores are held until we're
 * ready for them to initialise.
 *
 * This is executing in physical space with cache's off.
 */
ENTRY(msm_secondary_startup)
	mrc	p15, 0, r0, c0, c0, 5 	@ MPIDR
	and	r0, r0, #15		@ What CPU am I

	ldr	r1, =CPU0_EXIT_COUNT_PHYS
	ldr     r2, [r1, r0, LSL #2]
	add 	r2, r2, #1
	str	r2, [r1, r0, LSL #2]
	dsb

	ldr	r1, =CPU_FOOT_PRINT_BASE_CPU0_PHY
	mov	r2, #5
	strb	r2, [r1, r0, LSL #2] /* write footprint 5 */
	dsb

	adr	r4, 1f			@ address of
	ldmia	r4, {r5, r6}		@ load curr addr and pen_rel addr
	sub	r4, r4, r5		@ determine virtual/phys offsets
	add	r6, r6, r4		@ apply
pen:
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
ENDPROC(msm_secondary_startup)

1:	.long	.
	.long	pen_release
