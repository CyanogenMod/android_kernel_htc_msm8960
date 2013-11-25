/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SWFA_K_H
#define SWFA_K_H

#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>

#define SWFA_AUTHOR "SW-MM-A"
#define SWFA_TIME "2013-0314-1600"
#define SWFA_VERSION "v0001"
#define SWFA_ROIRATIO_HEIGHT 2

int swfa_FeatureAnalysis(uint8_t* pCurBuffer, int nImgWidth, int nImgHeight, int nROIX, int nROIY, int nROIW, int nROIH, int bRefineROI);
int swfa_Transform(uint8_t* pCurBuffer, int nNewROIX, int nNewROIY, int nWidht, int nHeight , int nWholeImgWidth);

int swfa_Transform2(uint8_t *swfaBuffer, int *newROIW, int *newROIH);
#endif


