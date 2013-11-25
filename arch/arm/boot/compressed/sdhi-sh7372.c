/*
 * SuperH Mobile SDHI
 *
 * Copyright (C) 2010 Magnus Damm
 * Copyright (C) 2010 Kuninori Morimoto
 * Copyright (C) 2010 Simon Horman
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Parts inspired by u-boot
 */

#include <linux/io.h>
#include <mach/mmc.h>
#include <linux/mmc/boot.h>
#include <linux/mmc/tmio.h>

#include "sdhi-shmobile.h"

#define PORT179CR       0xe60520b3
#define PORT180CR       0xe60520b4
#define PORT181CR       0xe60520b5
#define PORT182CR       0xe60520b6
#define PORT183CR       0xe60520b7
#define PORT184CR       0xe60520b8

#define SMSTPCR3        0xe615013c

#define CR_INPUT_ENABLE 0x10
#define CR_FUNCTION1    0x01

#define SDHI1_BASE	(void __iomem *)0xe6860000
#define SDHI_BASE	SDHI1_BASE

asmlinkage void mmc_loader(unsigned short *buf, unsigned long len)
{
	int high_capacity;

	mmc_init_progress();

	mmc_update_progress(MMC_PROGRESS_ENTER);
        
        
        __raw_writeb(CR_FUNCTION1, PORT184CR);
        
        __raw_writeb(CR_INPUT_ENABLE|CR_FUNCTION1, PORT179CR);
        
        __raw_writeb(CR_FUNCTION1, PORT183CR);
        
        __raw_writeb(CR_FUNCTION1, PORT182CR);
        
        __raw_writeb(CR_FUNCTION1, PORT181CR);
        
        __raw_writeb(CR_FUNCTION1, PORT180CR);

        
        __raw_writel(__raw_readl(SMSTPCR3) & ~(1 << 13), SMSTPCR3);

	
	mmc_update_progress(MMC_PROGRESS_INIT);
	high_capacity = sdhi_boot_init(SDHI_BASE);
	if (high_capacity < 0)
		goto err;

	mmc_update_progress(MMC_PROGRESS_LOAD);
	
	if (sdhi_boot_do_read(SDHI_BASE, high_capacity,
			      0, 
			      (len + TMIO_BBS - 1) / TMIO_BBS, buf))
		goto err;

        
        __raw_writel(__raw_readl(SMSTPCR3) | (1 << 13), SMSTPCR3);

	mmc_update_progress(MMC_PROGRESS_DONE);

	return;
err:
	for(;;);
}
