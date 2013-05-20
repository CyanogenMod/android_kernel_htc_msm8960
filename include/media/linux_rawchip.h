/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * ALTERNATIVELY, this product may be distributed under the terms of
 * the GNU General Public License, version 2, in which case the provisions
 * of the GPL version 2 are required INSTEAD OF the BSD license.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ALL OF
 * WHICH ARE HEREBY DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF NOT ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef __LINUX_RAWCHIP_H
#define __LINUX_RAWCHIP_H

#include <linux/types.h>
#include <linux/ioctl.h>

#define RAWCHIP_IOCTL_MAGIC 'g'

#define RAWCHIP_IOCTL_GET_INT \
	_IOR(RAWCHIP_IOCTL_MAGIC, 1, struct rawchip_stats_event_ctrl *)

#define RAWCHIP_IOCTL_GET_AF_STATUS \
	_IOR(RAWCHIP_IOCTL_MAGIC, 2, struct rawchip_stats_event_ctrl *)

#define RAWCHIP_IOCTL_UPDATE_AEC_AWB \
	_IOW(RAWCHIP_IOCTL_MAGIC, 3, struct rawchip_stats_event_ctrl *)

#define RAWCHIP_IOCTL_UPDATE_AF \
	_IOW(RAWCHIP_IOCTL_MAGIC, 4, struct rawchip_stats_event_ctrl *)

#define RAWCHIP_IOCTL_UPDATE_3A \
	_IOW(RAWCHIP_IOCTL_MAGIC, 5, struct rawchip_stats_event_ctrl *)

#define RAWCHIP_IOCTL_SET_DXOPRC_AF_STRATEGY \
	_IOW(RAWCHIP_IOCTL_MAGIC, 6, struct rawchip_stats_event_ctrl *)

#define RAWCHIP_IOCTL_GET_DXOPRC_VER\
	_IOW(RAWCHIP_IOCTL_MAGIC, 7, struct rawchip_stats_event_ctrl *)
	
#define RAWCHIP_IOCTL_GET_DXOPRC_FRAMESETTING\
	_IOW(RAWCHIP_IOCTL_MAGIC, 8, struct rawchip_stats_event_ctrl *)
	
typedef struct
{	
	uint8_t 	orientation;
	uint16_t	xStart;
	uint16_t	yStart;
	uint16_t	xEnd;
	uint16_t	yEnd;
	uint16_t	xEvenInc;  
	uint16_t	xOddInc;
	uint16_t	yEvenInc;  
	uint16_t	yOddInc;
	uint8_t 	binning;	
} rawchip_dxo_frameSetting ;

typedef struct {
	uint16_t 	udwDOPUcodeId;
	uint16_t 	udwDOPHwId;
	uint32_t 	udwDOPCalibId;

	uint16_t 	udwDPPUcodeId;
	uint16_t 	udwDPPHwId;
	uint32_t 	udwDPPCalibId;
	
	uint16_t 	udwPDPUcodeId;
	uint16_t 	udwPDPHwId;
	uint32_t 	udwPDPCalibId;
} rawchip_dxo_version;

struct rawchip_stats_event_ctrl {
	uint32_t type;
	uint32_t timeout_ms;
	uint32_t length;
	void *data;
};

#endif 

