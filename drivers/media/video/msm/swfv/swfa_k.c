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
 *
 */

#include "swfa_k.h"
#include <linux/vmalloc.h>
#include "../yushanII/file_operation.h"
#include <linux/fs.h>
#include <linux/slab.h>

#define SW_BUF_SIZE 	32*1024
#define ROI_BUF_SIZE 	128*1024

static int m_nCurROIWidth = 0, m_nCurROIHeight = 0;
static uint8_t *Y1 = 0;
static int nNewROIX = 0;
static int nNewROIY = 0;
static int nNewROIW = 0;
static int nNewROIH = 0;

int swfa_FeatureAnalysis(uint8_t* pCurBuffer, int nImgWidth, int nImgHeight, int nROIX, int nROIY, int nROIW, int nROIH, int bRefineROI)
{
	int status = 1;

#if 0
	uint8_t *path= "/data/test.yuv";
	struct file *pFile;
	static int i=0;

	if (i==0){
		pFile = file_open(path, O_CREAT|O_RDWR, 666);
		if(pFile == NULL){
			pr_err("open file %s fail",path);
			return 0;
		}
		file_write(pFile, 0, (void*)pCurBuffer, nImgWidth*nImgHeight*3/2);
		file_close(pFile);
		i++;
	}
#endif
	if(!pCurBuffer || (nROIW<16) || (nROIH<16)){
		return 0;
	}

	if((nROIX + nROIW > nImgWidth) || (nROIY + nROIH > nImgHeight)){
		return 0;
	}

	
	if(bRefineROI==0){
		nNewROIX = nROIX;
		nNewROIY = nROIY;
		nNewROIW = nROIW;
		nNewROIH = nROIH;
	}
	else{
#if 0
		nNewROIX = nROIX + nROIW*15/100;
		nNewROIW = nNewROIX + nROIW*7/10 < nImgWidth ? nROIW*7/10 : nImgWidth-1-nNewROIX;
#else
		nNewROIX = nROIX;
		nNewROIW = nROIW;
#endif
		nNewROIY = nROIY;
		nNewROIH = nNewROIY + nROIH*SWFA_ROIRATIO_HEIGHT < nImgHeight ? nROIH*SWFA_ROIRATIO_HEIGHT : nImgHeight-1-nNewROIY;

	}

	nNewROIW-=(nNewROIW%4);
	nNewROIH-=(nNewROIH%4);

	if( (nNewROIW*nNewROIH) > ROI_BUF_SIZE){
		return 0;
	}

	status = swfa_Transform( pCurBuffer, nNewROIX, nNewROIY, nNewROIW, nNewROIH, nImgWidth);
	return status;
}

int swfa_Transform(uint8_t* pCurBuffer, int nNewROIX, int nNewROIY, int nImgWidth, int nImgHeight, int nWholeImgWidth)
{

	register unsigned int x=0, y=0;
	int nHalfWLWidth=0;
	int nHalfWLHeight=0;
	uint8_t *pY1 = Y1;
	uint8_t *pBuf = pCurBuffer;
	
	

	if(!Y1){
		pr_err("[swfa] %s: memory allocate error\n", __func__);
		return 0;
	}

	nHalfWLWidth = nImgWidth/2;
	nHalfWLHeight = nImgHeight/2;

	for(y=0; y<nImgHeight; y++)
	{
		pBuf = pCurBuffer + ((y+nNewROIY)*nWholeImgWidth+nNewROIX);
		pY1 = Y1 + y*nImgWidth;
		for(x=0; x<nHalfWLWidth/2; x++)
		{
			*pY1 = ((*pBuf) + (*(pBuf+1)))>>1;
			*((pY1++)+nHalfWLWidth/2) = abs((*pBuf)-(*(pBuf+1)));
			pBuf+=4;
		}
	}
	return 1;
}

int swfa_Transform2(uint8_t *swfaBuffer, int *newROIW, int *newROIH)
{
	register unsigned int x=0, y=0;
	int nHalfWLWidth=0;
	int nHalfWLHeight=0;
	int nTmp1;
	unsigned int nTmp;
	uint8_t *pY1 = Y1;
	uint8_t *pTarget =0;

	*newROIW = nNewROIW;
	*newROIH = nNewROIH;

	if(!Y1){
		pr_err("[swfa] %s: memory allocate error\n", __func__);
		return 0;
	}

	nHalfWLWidth = nNewROIW/2;
	nHalfWLHeight = nNewROIH/2;

	nTmp = nHalfWLHeight/2 * nNewROIW;
	nTmp1 = nHalfWLHeight/2 * nNewROIW/2;
	pTarget = swfaBuffer;

	for(y=0; y<nHalfWLHeight/2; y++)
	{
		pY1 = Y1 + 4*y*nNewROIW;
		for(x=0; x<nNewROIW/2; x++)
		{
			*pTarget=((*pY1) + (*(pY1+nNewROIW)))>>1;
			*((pTarget++)+nTmp1) = abs((*pY1)-(*(pY1+nNewROIW)));
			++pY1;
		}
	}
	return 1;
}

void swfa_DeleteBuf(int nBufWidth, int nBufHeight)
{

	if(nBufWidth<=0 || nBufHeight<=0)
		return;

	if(Y1!=0){
		kfree(Y1);
	}
}

static int __init swfa_init(void)
{
	
	Y1=0;
	m_nCurROIWidth = -1;
	m_nCurROIHeight = -1;
	nNewROIX = 0;
	nNewROIY = 0;
	nNewROIW = 0;
	nNewROIH = 0;
	Y1 = kcalloc(ROI_BUF_SIZE, sizeof(*Y1), GFP_ATOMIC);
	if(!Y1){
		return 0;
	}
	pr_info("[swfa] %s: SW-MM-A-2013-0314-1600-v0001\n", __func__);
	return 1;
}

static void __exit swfa_exit(void)
{
	if(Y1!=0){
		kfree(Y1);
	}
}

MODULE_DESCRIPTION("swfa");
MODULE_VERSION("swfa 1.0");


module_init(swfa_init);
module_exit(swfa_exit);
