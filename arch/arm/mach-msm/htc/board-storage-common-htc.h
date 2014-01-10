/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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

#ifndef _BOARD_STORAGE_HTC_H
#define _BOARD_STORAGE_HTC_H

#include <asm/mach/mmc.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_bus.h>

#undef MSM_BUS_SPS_TO_DDR_VOTE_VECTOR
#define MSM_BUS_SPS_TO_DDR_VOTE_VECTOR(num, _ib) \
static struct msm_bus_vectors wifi_sps_to_ddr_perf_vectors_##num[] = { \
	{ \
		.src = MSM_BUS_MASTER_SPS, \
		.dst = MSM_BUS_SLAVE_EBI_CH0, \
		.ib = (_ib), \
		.ab = ((_ib) / 2), \
	} \
}

#undef MSM_BUS_SPS_TO_DDR_VOTE_VECTOR_USECASE
#define MSM_BUS_SPS_TO_DDR_VOTE_VECTOR_USECASE(num) \
	{ \
		ARRAY_SIZE(wifi_sps_to_ddr_perf_vectors_##num), \
		wifi_sps_to_ddr_perf_vectors_##num, \
	}

MSM_BUS_SPS_TO_DDR_VOTE_VECTOR(0, 0);
MSM_BUS_SPS_TO_DDR_VOTE_VECTOR(1, 13 * 1024 * 1024);
MSM_BUS_SPS_TO_DDR_VOTE_VECTOR(2, UINT_MAX);

static unsigned int wifi_sdcc_bw_vectors[] = {0, (13 * 1024 * 1024),
				UINT_MAX};

static struct msm_bus_paths wifi_sps_to_ddr_bus_scale_usecases[] = {
	MSM_BUS_SPS_TO_DDR_VOTE_VECTOR_USECASE(0),
	MSM_BUS_SPS_TO_DDR_VOTE_VECTOR_USECASE(1),
	MSM_BUS_SPS_TO_DDR_VOTE_VECTOR_USECASE(2),
	
	
	
	
};

static struct msm_bus_scale_pdata wifi_sps_to_ddr_bus_scale_data = {
	wifi_sps_to_ddr_bus_scale_usecases,
	ARRAY_SIZE(wifi_sps_to_ddr_bus_scale_usecases),
	.name = "msm_wifi_sdcc3",
};

static struct msm_mmc_bus_voting_data wifi_sps_to_ddr_bus_voting_data = {
	.use_cases = &wifi_sps_to_ddr_bus_scale_data,
	.bw_vecs = wifi_sdcc_bw_vectors,
	.bw_vecs_size = sizeof(wifi_sdcc_bw_vectors),
};

#endif 

