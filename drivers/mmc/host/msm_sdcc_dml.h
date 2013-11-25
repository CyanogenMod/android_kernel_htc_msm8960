/*
 *  linux/drivers/mmc/host/msm_sdcc_dml.h - Qualcomm SDCC DML driver
 *					    header file
 *
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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

#ifndef _MSM_SDCC_DML_H
#define _MSM_SDCC_DML_H

#include <linux/types.h>
#include <linux/mmc/host.h>

#include "msm_sdcc.h"

#ifdef CONFIG_MMC_MSM_SPS_SUPPORT
int msmsdcc_dml_init(struct msmsdcc_host *host);

void msmsdcc_dml_start_xfer(struct msmsdcc_host *host, struct mmc_data *data);

bool msmsdcc_is_dml_busy(struct msmsdcc_host *host);

void msmsdcc_dml_reset(struct msmsdcc_host *host);

void msmsdcc_dml_exit(struct msmsdcc_host *host);
#else
static inline int msmsdcc_dml_init(struct msmsdcc_host *host) { return 0; }
static inline int msmsdcc_dml_start_xfer(struct msmsdcc_host *host,
				struct mmc_data *data) { return 0; }
static inline bool msmsdcc_is_dml_busy(
				struct msmsdcc_host *host) { return 0; }
static inline void msmsdcc_dml_reset(struct msmsdcc_host *host) { }
static inline void msmsdcc_dml_exit(struct msmsdcc_host *host) { }
#endif 

#endif 
