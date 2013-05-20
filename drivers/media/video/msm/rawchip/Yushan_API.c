
#include "Yushan_API.h"
#include "Yushan_Platform_Specific.h"




bool_t	gPllLocked;


bool_t	Yushan_Init_LDO(bool_t	bUseExternalLDO)
{
	bool_t		fStatus = SUCCESS;
	uint8_t		bSpiReadData = 0, bLDOTrimValue = 0, bSpiData = 0;
	uint16_t	uwCount = 500;	

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	
	SPI_Read(YUSHAN_IOR_NVM_STATUS, 1, (uint8_t *)(&bSpiReadData));
	while (bSpiReadData != 1) {
		
		SPI_Read(YUSHAN_IOR_NVM_STATUS, 1, (uint8_t *)(&bSpiReadData));
		
		uwCount--;
		if (!uwCount)
			break;	
	}

	
	if (bSpiReadData == 1) {
		
		SPI_Read(YUSHAN_IOR_NVM_DATA_WORD_3, 1, (uint8_t *)(&bLDOTrimValue));
		if (bLDOTrimValue>>3 == 1)				
			bLDOTrimValue &= 0xF7;
		else {
			
			bLDOTrimValue = 0;
			SPI_Read(YUSHAN_IOR_NVM_DATA_WORD_2 + 3, 1, (uint8_t *)(&bLDOTrimValue));
			if (bLDOTrimValue>>3 == 1)			
				bLDOTrimValue &= 0xF7;
			else							
				bLDOTrimValue = 0 ;			
		}

		
		SPI_Write(YUSHAN_PRIVATE_TEST_LDO_NVM_CTRL, 1, (uint8_t *)(&bLDOTrimValue));

		
		bSpiReadData = 0;
		SPI_Read(YUSHAN_PRIVATE_TEST_LDO_CTRL,  1, (uint8_t *)(&bSpiReadData));
		bSpiData = bSpiReadData;

		DEBUGLOG("[CAM] %s: Yushan bUseExternalLDO = %d\n", __func__, bUseExternalLDO);
		if(!bUseExternalLDO) {
			
			bSpiData = (bSpiData&0xF7);			
			SPI_Write(YUSHAN_PRIVATE_TEST_LDO_CTRL,  1, (uint8_t *)(&bSpiData));
			bSpiData = (bSpiData&0xFE);			
			SPI_Write(YUSHAN_PRIVATE_TEST_LDO_CTRL,  1, (uint8_t *)(&bSpiData));
			bSpiData = (bSpiData&0xFD);			
			SPI_Write(YUSHAN_PRIVATE_TEST_LDO_CTRL,  1, (uint8_t *)(&bSpiData));
		} else {
			
			bSpiData |= 0x04;
			SPI_Write(YUSHAN_PRIVATE_TEST_LDO_CTRL,  1, (uint8_t *)(&bSpiData));
		}

		
		DEBUGLOG("[CAM] %s: Waiting for EVENT_LDO_STABLE Interrupt\n", __func__);
		fStatus &= Yushan_WaitForInterruptEvent(EVENT_LDO_STABLE, TIME_100MS);

		if (!fStatus) {
			ERRORLOG("[CAM] %s:LDO Interrupt Failed\n", __func__);
			VERBOSELOG("[CAM] %s: End with Failure\n", __func__);
			return FAILURE;
		} else {
			DEBUGLOG("[CAM] %s:LDO Interrupt received\n", __func__);
		}

	} else {
		ERRORLOG("[CAM] %s:NVM is not Ready for Read operation\n", __func__);
		VERBOSELOG("[CAM] %s: End\n", __func__);
		return FAILURE;
	}

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return SUCCESS;

}



bool_t Yushan_Init_Clocks(Yushan_Init_Struct_t *sInitStruct, Yushan_SystemStatus_t *sSystemStatus, uint32_t	*udwIntrMask)
{

	uint32_t		fpTargetPll_Clk, fpSys_Clk, fpPixel_Clk, fpDxo_Clk, fpIdp_Clk, fpDxoClockLow;		
	uint32_t		fpSysClk_Div, fpIdpClk_div, fpDxoClk_Div, fpTempVar;				

	uint32_t		udwSpiData = 0;
	uint8_t			bRx_PixelWidth, bTx_PixelWidth;
	int8_t			bCount;
	uint8_t			fStatus = SUCCESS, fDXOStatus = DXO_NO_ERR;
	uint8_t			bSpiData;
	uint32_t 		udwFullLine, udwFullFrame, udwFrameBlank;

	
	

	
	
	uint32_t		fpDividers[] = {0x10000, 0x18000, 0x20000, 0x28000, 0x30000, 0x38000, 0x40000, 0x48000, 0x50000, 0x58000, 0x60000,	\
										0x68000, 0x70000, 0x78000, 0x80000, 0xA0000};

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	
	DEBUGLOG("[CAM] %s:Switching to Host clock while evaluating different clock dividers\n", __func__);
	bSpiData = 0x00;
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+1, 1, (uint8_t *)(&bSpiData)); 
	bSpiData = 0x02;
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+1, 1, (uint8_t *)(&bSpiData)); 
	bSpiData = 0x19;
	fStatus &= SPI_Write(YUSHAN_PLL_CTRL_MAIN, 1, (uint8_t *)(&bSpiData));	
	
	bRx_PixelWidth = bTx_PixelWidth = (sInitStruct->uwPixelFormat) & (0x000F);

	
	fpTargetPll_Clk = Yushan_ConvertTo16p16FP((uint16_t)sInitStruct->uwBitRate);
	
	fpTargetPll_Clk = Yushan_Compute_Pll_Divs(sInitStruct->fpExternalClock, fpTargetPll_Clk);
	if (!fpTargetPll_Clk) {
		ERRORLOG("[CAM] %s:Wrong PLL Clk calculated. Returning", __func__);
		VERBOSELOG("[CAM] %s: End\n", __func__);
		return FAILURE;
	}

	
	
	
	
	
	
	
	
	
	fpIdpClk_div = (Yushan_ConvertTo16p16FP(bTx_PixelWidth)/sInitStruct->bNumberOfLanes);

	

	
	
	
	fpPixel_Clk = ( (Yushan_ConvertTo16p16FP(sInitStruct->uwBitRate)*sInitStruct->bNumberOfLanes)/bRx_PixelWidth);
	fpIdp_Clk = Yushan_ConvertTo16p16FP((uint16_t)(fpTargetPll_Clk/fpIdpClk_div));

	
	udwFullLine  = sInitStruct->uwActivePixels + sInitStruct->uwLineBlankStill;
	udwFullFrame = sInitStruct->uwLines + sInitStruct->uwFrameBlank;

	fpTempVar = Yushan_ConvertTo16p16FP((uint16_t)((sInitStruct->fpSpiClock*30)/0x80000));
	fpDxoClockLow = (fpTempVar >= fpIdp_Clk)?fpTempVar:fpIdp_Clk;

	if (fpDxoClockLow > DXO_CLK_LIMIT) {
		ERRORLOG("[CAM] %s:Lower limit of DxO clock is greater than higher limit, so EXITING the test\n", __func__);
		sSystemStatus->bDxoConstraints = DXO_LOLIMIT_EXCEED_HILIMIT;
		VERBOSELOG("[CAM] %s: End\n", __func__);
		return FAILURE;
	}

	
	bCount = ((bTx_PixelWidth == 8) ? 14 : 15);

	sSystemStatus->udwDxoConstraintsMinValue = 0;
	
	while (bCount >= 0) {
		fpDxoClk_Div = fpDividers[bCount];
		fpDxo_Clk = Yushan_ConvertTo16p16FP((uint16_t)(fpTargetPll_Clk / fpDxoClk_Div));

		if ((fpDxo_Clk >= fpDxoClockLow) && (fpDxo_Clk <= DXO_CLK_LIMIT)) {
			
			fDXOStatus = ((fStatus = Yushan_CheckDxoConstraints(sInitStruct->uwLineBlankStill, DXO_MIN_LINEBLANKING, fpDxo_Clk, fpPixel_Clk, 1, &sSystemStatus->udwDxoConstraintsMinValue))==0)? DXO_LINEBLANK_ERR : DXO_NO_ERR;
			if(fDXOStatus!=DXO_NO_ERR) { bCount--; continue;}
			fDXOStatus = ((fStatus = Yushan_CheckDxoConstraints(udwFullLine, DXO_MIN_LINELENGTH, fpDxo_Clk, fpPixel_Clk, 1, &sSystemStatus->udwDxoConstraintsMinValue))==0)? DXO_FULLLINE_ERR: DXO_NO_ERR ;
			if(fDXOStatus!=DXO_NO_ERR) { bCount--; continue;}
			
			
			fDXOStatus = ((fStatus = Yushan_CheckDxoConstraints(sInitStruct->uwLines, ((DXO_ACTIVE_FRAMELENGTH/udwFullLine)+1), fpDxo_Clk, fpPixel_Clk, 1, &sSystemStatus->udwDxoConstraintsMinValue))==0)? DXO_ACTIVE_FRAMELENGTH_ERR: DXO_NO_ERR;
			if(fDXOStatus!=DXO_NO_ERR) { bCount--; continue;}
			fDXOStatus = ((fStatus = Yushan_CheckDxoConstraints(udwFullFrame, ((DXO_FULL_FRAMELENGTH/udwFullLine)+1), fpDxo_Clk, fpPixel_Clk, 1, &sSystemStatus->udwDxoConstraintsMinValue))==0)? DXO_FULLFRAMELENGTH_ERR: DXO_NO_ERR;
			if(fDXOStatus!=DXO_NO_ERR) { bCount--; continue;}

			udwFrameBlank = ((8*fpDxo_Clk)/fpPixel_Clk);						
			if (sInitStruct->bDxoSettingCmdPerFrame == 1)
				udwFrameBlank += (135000/udwFullLine)+1;   
			if (sInitStruct->bDxoSettingCmdPerFrame > 1)
				udwFrameBlank += (220000/udwFullLine)+1;   

			fDXOStatus = ((fStatus = Yushan_CheckDxoConstraints(sInitStruct->uwFrameBlank, udwFrameBlank, fpDxo_Clk, fpPixel_Clk, 1, &sSystemStatus->udwDxoConstraintsMinValue))==0)? DXO_FRAMEBLANKING_ERR: DXO_NO_ERR;
			if(fDXOStatus!=DXO_NO_ERR) { bCount--; continue;}

			
			break;

        	}																															
		
		bCount--;
	}

	
	sSystemStatus->bDxoConstraints = fDXOStatus;

	
	if (bCount < 0) {
		ERRORLOG("[CAM] %s:DXO Dividers exhausted and returning FAILURE\n", __func__);
		VERBOSELOG("[CAM] %s: End\n", __func__);
		return FAILURE;
	}

	for (bCount = 0; bCount < 10; bCount++) {
		fpSysClk_Div = fpDividers[bCount];
		fpSys_Clk = Yushan_ConvertTo16p16FP((uint16_t)(fpTargetPll_Clk / fpSysClk_Div));
		if (fpSys_Clk <= SYS_CLK_LIMIT)
			break; 
	}


	fStatus = SUCCESS;

	
	bSpiData = 0x7F;
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL, 1, (uint8_t *)(&bSpiData)); 

	
	
	bSpiData = 0x00;
	fStatus &= SPI_Write(YUSHAN_RESET_CTRL, 1, (uint8_t *)(&bSpiData));
	bSpiData = 0x04;
	fStatus &= SPI_Write(YUSHAN_RESET_CTRL + 1, 1, (uint8_t *)(&bSpiData));
	bSpiData = 0x00;
	fStatus &= SPI_Write(YUSHAN_RESET_CTRL + 2, 1, (uint8_t *)(&bSpiData));
	bSpiData = 0x37;
	fStatus &= SPI_Write(YUSHAN_RESET_CTRL, 1, (uint8_t *)(&bSpiData));
	bSpiData = 0x0f;
	fStatus &= SPI_Write(YUSHAN_RESET_CTRL + 1, 1, (uint8_t *)(&bSpiData));
	bSpiData = 0x07;
	fStatus &= SPI_Write(YUSHAN_RESET_CTRL + 2, 1, (uint8_t *)(&bSpiData));
	bSpiData = 1;
	fStatus &= SPI_Write(YUSHAN_RESET_CTRL + 3, 1, (uint8_t *)(&bSpiData));
	bSpiData = 0;
	fStatus &= SPI_Write(YUSHAN_RESET_CTRL + 3, 1, (uint8_t *)(&bSpiData));

	
	Yushan_Intr_Enable((uint8_t *)udwIntrMask);

	
	fStatus &= Yushan_Init_LDO(sInitStruct->bUseExternalLDO);
	if (!fStatus) {
		ERRORLOG("[CAM] %s:LDO setup FAILED\n", __func__);
		VERBOSELOG("[CAM] %s: End\n", __func__);
		return FAILURE;
	} else {
		DEBUGLOG("[CAM] %s:LDO setup done.\n", __func__);
	}

	
	
	bSpiData = (uint8_t)(fpDxoClk_Div>>16);
	bSpiData = bSpiData << 1;
	if ((fpDxoClk_Div&0x8000)==0x8000)				
		bSpiData |= 0x01;
	fStatus &= SPI_Write(YUSHAN_CLK_DIV_FACTOR,   1, (uint8_t *)(&bSpiData));
	bSpiData = (uint8_t)(fpIdpClk_div>>16);
	bSpiData = bSpiData << 1;
	if ((fpIdpClk_div&0x8000)==0x8000)				
		bSpiData |= 0x01;
	fStatus &= SPI_Write(YUSHAN_CLK_DIV_FACTOR+1, 1, (uint8_t *) (&bSpiData));
	bSpiData = (uint8_t)(fpSysClk_Div>>16);
	bSpiData = bSpiData << 1;
	if ((fpSysClk_Div&0x8000)==0x8000)				
		bSpiData |= 0x01;
	fStatus &= SPI_Write(YUSHAN_CLK_DIV_FACTOR + 2, 1, (uint8_t *) (&bSpiData));

	
	bSpiData = 0x18;
	fStatus &= SPI_Write(YUSHAN_PLL_CTRL_MAIN, 1, (uint8_t *)(&bSpiData));	

	if (!fStatus) {
		ERRORLOG("[CAM] %s: End with Failure at enabling PLLs\n", __func__);
		return FAILURE;
	}

	DEBUGLOG("[CAM] %s:Waiting for YUSHAN_PLL_CTRL_MAIN interrupt Starts here\n", __func__);
	fStatus &= Yushan_WaitForInterruptEvent(EVENT_PLL_STABLE, TIME_100MS);

	DEBUGLOG("[CAM] %s:YUSHAN_PLL_CTRL_MAIN interrupt received\n", __func__);

	if (!fStatus) {
		ERRORLOG("[CAM] %s: Failure at YUSHAN_PLL_CTRL_MAIN interrupt non-received Exiting...\n", __func__);
		return FAILURE;
	}

	
	
	udwSpiData = 0;
	SPI_Read(YUSHAN_CLK_CTRL,   4, (uint8_t*)(&udwSpiData));

	
	bSpiData = 0x00;
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL, 1, (uint8_t*)(&bSpiData));
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+1, 1, (uint8_t*)(&bSpiData));
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+2, 1, (uint8_t*)(&bSpiData));		
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+3, 1, (uint8_t*)(&bSpiData));		


	
	bSpiData = 0x01; 
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL + 1, 1, (uint8_t *)(&bSpiData));

	
	udwSpiData &= 0xFDFF; 
	udwSpiData |= 0x0100; 
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL, 4, (uint8_t *)(&udwSpiData));

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;


}



uint32_t Yushan_ConvertTo16p16FP(uint16_t uwNonFPValue)
{

	return (((uint32_t)(uwNonFPValue)) << 16);

}




bool_t	Yushan_Init_Dxo(Yushan_Init_Dxo_Struct_t *sDxoStruct, bool_t fBypassDxoUpload)
{

	uint8_t bSpiData;
	uint32_t	udwHWnMicroCodeID = 0; 
	uint32_t udwDxoBaseAddress = 0, udwSpiBaseIndex = 0;
	bool_t	fStatus = 1;
	

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	

	udwSpiBaseIndex = 0x08000;
	fStatus=SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

	udwDxoBaseAddress=DXO_DOP_BASE_ADDR;

	if (!fBypassDxoUpload) {
		DEBUGLOG("[CAM] load DxO DOP firmware\n");
		fStatus &= SPI_Write_4thByte((uint16_t)(sDxoStruct->uwBaseAddrDopMicroCode[0]+udwDxoBaseAddress), sDxoStruct->uwDxoDopRamImageSize[0], sDxoStruct->pDxoDopRamImage[0]);
		fStatus &= SPI_Write_4thByte((uint16_t)(sDxoStruct->uwBaseAddrDopMicroCode[1]+udwDxoBaseAddress), sDxoStruct->uwDxoDopRamImageSize[1], sDxoStruct->pDxoDopRamImage[1]);
	}
	
	
	fStatus &= SPI_Write((uint16_t)(sDxoStruct->uwDxoDopBootAddr + udwDxoBaseAddress), 2,  (uint8_t*)(&sDxoStruct->uwDxoDopStartAddr));
	
	bSpiData = DXO_BOOT_CMD;
	fStatus &= SPI_Write((uint16_t)(DxODOP_boot + udwDxoBaseAddress), 1,  (uint8_t*)(&bSpiData));

	
	udwSpiBaseIndex = 0x10000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

	udwDxoBaseAddress=(0x8000 + DXO_DPP_BASE_ADDR) - udwSpiBaseIndex; 
	
	if (!fBypassDxoUpload) {
		DEBUGLOG("[CAM] load DxO DPP firmware\n");
		fStatus &= SPI_Write_4thByte((uint16_t)(sDxoStruct->uwBaseAddrDppMicroCode[0]+ udwDxoBaseAddress), sDxoStruct->uwDxoDppRamImageSize[0], sDxoStruct->pDxoDppRamImage[0]);
		
		udwSpiBaseIndex = DXO_DPP_BASE_ADDR + sDxoStruct->uwBaseAddrDppMicroCode[1]; 
		SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));
		udwDxoBaseAddress = ((DXO_DPP_BASE_ADDR + sDxoStruct->uwBaseAddrDppMicroCode[1]) - udwSpiBaseIndex) + 0x8000; 

		fStatus &= SPI_Write_4thByte((uint16_t)(udwDxoBaseAddress), sDxoStruct->uwDxoDppRamImageSize[1], sDxoStruct->pDxoDppRamImage[1]);
	}
	
	udwSpiBaseIndex = 0x18000;
	udwDxoBaseAddress = 0x8000;  
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

	
	
	fStatus &= SPI_Write((uint16_t)((DXO_DPP_BASE_ADDR + sDxoStruct->uwDxoDppBootAddr) - udwSpiBaseIndex + udwDxoBaseAddress), 2,  (uint8_t*)(&sDxoStruct->uwDxoDppStartAddr));
	
	bSpiData = DXO_BOOT_CMD;
	fStatus &= SPI_Write(((uint16_t)(DXO_DPP_BASE_ADDR + DxODPP_boot) - udwSpiBaseIndex + udwDxoBaseAddress), 1,  (uint8_t*)(&bSpiData));

	
	udwSpiBaseIndex = 0x8000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

	
	
	if (!fBypassDxoUpload) {
		DEBUGLOG("[CAM] load DxO PDP firmware\n");
		fStatus &= SPI_Write_4thByte((uint16_t)(sDxoStruct->uwBaseAddrPdpMicroCode[0]+ DXO_PDP_BASE_ADDR), sDxoStruct->uwDxoPdpRamImageSize[0], sDxoStruct->pDxoPdpRamImage[0]);
		fStatus &= SPI_Write_4thByte((uint16_t)(sDxoStruct->uwBaseAddrPdpMicroCode[1]+ DXO_PDP_BASE_ADDR), sDxoStruct->uwDxoPdpRamImageSize[1], sDxoStruct->pDxoPdpRamImage[1]);
  	}
	
	
	fStatus &= SPI_Write((uint16_t)(sDxoStruct->uwDxoPdpBootAddr + DXO_PDP_BASE_ADDR), 2,  (uint8_t*)(&sDxoStruct->uwDxoPdpStartAddr));

	
	bSpiData = DXO_BOOT_CMD;
	fStatus &= SPI_Write((uint16_t)(DxOPDP_boot + DXO_PDP_BASE_ADDR), 1,  (uint8_t*)(&bSpiData));

	DEBUGLOG("[CAM] %s:Waiting for EVENT_DOP7_BOOT interrupt Starts here\n", __func__);
	
	fStatus &= Yushan_WaitForInterruptEvent2 (EVENT_DOP7_BOOT, TIME_100MS);
	if (!fStatus) {
		ERRORLOG("[CAM] %s: EVENT_DOP7_BOOT not received. Exiting ...\n", __func__);
		return fStatus;
	}
	DEBUGLOG("[CAM] %s:DOP7 IP Booted\n", __func__);

	

		
	SPI_Read((DxODOP_ucode_id_7_0 + DXO_DOP_BASE_ADDR) , 4, (uint8_t *)(&udwHWnMicroCodeID));
#if 0
	SPI_Read((DxODOP_calib_id_0_7_0 + DXO_DOP_BASE_ADDR), 4, (uint8_t *)(&udwCalibrationDataID));
	SPI_Read((DxODOP_error_code_7_0 + DXO_DOP_BASE_ADDR), 1, (uint8_t *)(&udwErrorCode));
	DEBUGLOG("DXO DOP udwHWnMicroCodeID:%x;udwCalibrationDataID:%x;udwErrorCode:%x\n",
		udwHWnMicroCodeID, udwCalibrationDataID, udwErrorCode);
#endif

	udwHWnMicroCodeID &= 0xFFFFFF00;	
	if ((udwHWnMicroCodeID != DXO_DOP_HW_MICROCODE_ID)) {
		ERRORLOG("[CAM] %s: Error with DOP microcode check. Exiting ...\n", __func__);
		return FAILURE;
	}

	DEBUGLOG("[CAM] %s:Waiting for EVENT_DPP_BOOT interrupt Starts here\n", __func__);
	
	fStatus &= Yushan_WaitForInterruptEvent (EVENT_DPP_BOOT, TIME_100MS);
	if (!fStatus) {
		ERRORLOG("[CAM] %s: EVENT_DPP_BOOT not received. Exiting ...\n", __func__);
		return fStatus;
	}
	DEBUGLOG("[CAM] %s:DPP IP Booted\n", __func__);

	
	udwSpiBaseIndex = 0x010000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

	SPI_Read(DxODPP_ucode_id_7_0+ DXO_DPP_BASE_ADDR-0x8000, 4, (uint8_t *)(&udwHWnMicroCodeID));
#if 0
	SPI_Read(DxODPP_calib_id_0_7_0+DXO_DPP_BASE_ADDR-0x8000, 4, (uint8_t *)(&udwCalibrationDataID));
	SPI_Read(DxOPDP_error_code_7_0+DXO_DPP_BASE_ADDR-0x8000, 1, (uint8_t *)(&udwErrorCode));
	DEBUGLOG("DXO DPP udwHWnMicroCodeID:%x;udwCalibrationDataID:%x;udwErrorCode:%x\n",
		udwHWnMicroCodeID, udwCalibrationDataID, udwErrorCode);
#endif

	udwHWnMicroCodeID &= 0xFFFFFF00;	
	if (udwHWnMicroCodeID != DXO_DPP_HW_MICROCODE_ID) {
		ERRORLOG("[CAM] %s: Error with DPP microcode check. Exiting ...\n", __func__);
		return FAILURE;
	}
	
	udwSpiBaseIndex = 0x08000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

	DEBUGLOG("[CAM] %s:Waiting for EVENT_PDP_BOOT interrupt Starts here\n", __func__);
	
	fStatus &= Yushan_WaitForInterruptEvent (EVENT_PDP_BOOT, TIME_100MS);

	if (!fStatus) {
		ERRORLOG("[CAM] %s: EVENT_PDP_BOOT not received. Exiting ...\n", __func__);
		return fStatus;
	}
	DEBUGLOG("[CAM] %s:PDP IP Booted\n", __func__);

	
	SPI_Read((DxOPDP_ucode_id_7_0 + DXO_PDP_BASE_ADDR) , 4, (uint8_t *)(&udwHWnMicroCodeID));
#if 0
	SPI_Read((DxOPDP_calib_id_0_7_0 + DXO_PDP_BASE_ADDR), 4, (uint8_t *)(&udwCalibrationDataID));
	SPI_Read((DxOPDP_error_code_7_0 + DXO_PDP_BASE_ADDR), 1, (uint8_t *)(&udwErrorCode));
	DEBUGLOG("DXO PDP udwHWnMicroCodeID:%x;udwCalibrationDataID:%x;udwErrorCode:%x\n",
		udwHWnMicroCodeID, udwCalibrationDataID, udwErrorCode);
#endif

	udwHWnMicroCodeID &= 0xFFFFFF00; 
	if (udwHWnMicroCodeID != DXO_PDP_HW_MICROCODE_ID) {
		ERRORLOG("[CAM] %s: Error with PDP microcode check. Exiting ...\n", __func__);
		return FAILURE;
	}

	VERBOSELOG("[CAM] %s: End with Success\n", __func__);
	return SUCCESS;
}


bool_t Yushan_Init(Yushan_Init_Struct_t *sInitStruct)
{

	uint32_t spi_base_address, udwSpiData;
	uint16_t uwHsizeStill = 0;
	uint16_t uwHsizeVf = 0;
	uint16_t uwLecciStill;
	uint16_t uwLecciVf;
	uint8_t bCount, bStillIndex = 0xf, bVfIndex = 0xf;
	uint8_t bSofEofLength = 32;
	uint8_t bDxoClkDiv , bPixClkDiv;
	uint8_t		bDXODecimalFactor = 0, bIdpDecimalFactor = 0; 
	uint8_t		bUsedDataType = 0;

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	
	spi_base_address = 0x8000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (unsigned char *)&spi_base_address);

	udwSpiData = 0x1;
	SPI_Write(YUSHAN_IOR_NVM_CTRL, 1, (unsigned char *)&udwSpiData); 

	
	udwSpiData = 4000/sInitStruct->uwBitRate;
	SPI_Write(YUSHAN_MIPI_TX_UIX4, 1, (unsigned char *)&udwSpiData);
	SPI_Write(YUSHAN_MIPI_RX_UIX4, 1, (unsigned char *)&udwSpiData);
	udwSpiData = 1;
	SPI_Write(YUSHAN_MIPI_RX_COMP, 1, (unsigned char*)&udwSpiData);

	if (sInitStruct->bNumberOfLanes == 1)
		udwSpiData = 0x11; 
	else if (sInitStruct->bNumberOfLanes == 2)
		udwSpiData = 0x31; 
	else if (sInitStruct->bNumberOfLanes == 4)
		udwSpiData = 0xf1; 

	
	SPI_Write(YUSHAN_MIPI_TX_ENABLE,1,(unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_MIPI_RX_ENABLE,1,(unsigned char*)&udwSpiData); 
	
	
	udwSpiData=4000/sInitStruct->uwBitRate;
	SPI_Write(YUSHAN_MIPI_TX_UIX4,1,(unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_MIPI_RX_UIX4,1,(unsigned char*)&udwSpiData); 

	
	SPI_Read(YUSHAN_MIPI_RX_ENABLE,1,(unsigned char*)&udwSpiData); 
	udwSpiData |= 0x02; 
	SPI_Write(YUSHAN_MIPI_TX_ENABLE,1,(unsigned char*)&udwSpiData);

	

	udwSpiData=sInitStruct->bNumberOfLanes;
	SPI_Write(YUSHAN_CSI2_RX_NB_DATA_LANES,1,(unsigned char*)&udwSpiData); 

	udwSpiData=sInitStruct->uwPixelFormat & 0x0f;
	SPI_Write(YUSHAN_CSI2_RX_IMG_UNPACKING_FORMAT,1,(unsigned char*)&udwSpiData); 

	udwSpiData = 4; 
	SPI_Write(YUSHAN_CSI2_RX_BYTE2PIXEL_READ_TH,1,(unsigned char*)&udwSpiData); 

	

	udwSpiData=sInitStruct->uwPixelFormat & 0x0f;
	SPI_Write(YUSHAN_SMIA_FM_PIX_WIDTH,1,(unsigned char*)&udwSpiData); 

	udwSpiData=0;
	SPI_Write(YUSHAN_SMIA_FM_GROUPED_PARAMETER_HOLD,1,(unsigned char*)&udwSpiData); 
	udwSpiData=0x19;
	SPI_Write(YUSHAN_SMIA_FM_CTRL,1,(unsigned char*)&udwSpiData); 
	
	udwSpiData = 0;
	SPI_Write(YUSHAN_SMIA_FM_EOF_INT_EN, 1, (unsigned char*)&udwSpiData);	
	
	

	
	udwSpiData=((((sInitStruct->uwPixelFormat & 0x0f) * bSofEofLength )/32 )<<8)|0x01;
	SPI_Write(YUSHAN_CSI2_TX_WRAPPER_THRESH,2,(unsigned char*)&udwSpiData); 

	

	udwSpiData=1;
	SPI_Write(YUSHAN_EOF_RESIZE_PRE_DXO_AUTOMATIC_CONTROL,1,(unsigned char*)&udwSpiData); 
	SPI_Write(YUSHAN_EOF_RESIZE_PRE_DXO_ENABLE,1,(unsigned char*)&udwSpiData); 

	
	udwSpiData=0; 
	SPI_Write(YUSHAN_EOF_RESIZE_POST_DXO_AUTOMATIC_CONTROL,1,(unsigned char*)&udwSpiData); 
	udwSpiData= bSofEofLength;
	SPI_Write(YUSHAN_EOF_RESIZE_POST_DXO_H_SIZE,1,(unsigned char*)&udwSpiData); 
	udwSpiData=1;
	SPI_Write(YUSHAN_EOF_RESIZE_POST_DXO_ENABLE,1,(unsigned char*)&udwSpiData); 

	SPI_Read(YUSHAN_CLK_DIV_FACTOR,1,(unsigned char*)&bDxoClkDiv); 
	SPI_Read(YUSHAN_CLK_DIV_FACTOR+1,1,(unsigned char*)&bPixClkDiv); 
	
	

	

	for (bCount = 0 ; bCount < 14 ; bCount++) {

		if (bCount < sInitStruct->bValidWCEntries) {
		udwSpiData=sInitStruct->sFrameFormat[bCount].uwWordcount | 
			( sInitStruct->sFrameFormat[bCount].bDatatype << 16 )|
			( sInitStruct->sFrameFormat[bCount].bActiveDatatype << 24 );

		SPI_Write((uint16_t)(YUSHAN_IDP_GEN_WC_DI_0+4*bCount),4,(unsigned char*)&udwSpiData);

		udwSpiData=sInitStruct->sFrameFormat[bCount].uwWordcount ;
		SPI_Write((uint16_t)(YUSHAN_CSI2_TX_PACKET_SIZE_0+0xc*bCount),4,(unsigned char*)&udwSpiData);

		udwSpiData=sInitStruct->sFrameFormat[bCount].bDatatype ;
		SPI_Write((uint16_t)(YUSHAN_CSI2_TX_DI_INDEX_CTRL_0+0xc*bCount),1,(unsigned char*)&udwSpiData);

		if (sInitStruct->sFrameFormat[bCount].bSelectStillVfMode == YUSHAN_FRAME_FORMAT_VF_MODE) {
			bUsedDataType = sInitStruct->sFrameFormat[bCount].bDatatype;
			uwHsizeVf=(sInitStruct->sFrameFormat[bCount].uwWordcount * 8 )/ (sInitStruct->uwPixelFormat & 0x0f);
			bVfIndex=bCount;
		}

		if (sInitStruct->sFrameFormat[bCount].bSelectStillVfMode == YUSHAN_FRAME_FORMAT_STILL_MODE) {
			bUsedDataType = sInitStruct->sFrameFormat[bCount].bDatatype;
			uwHsizeStill=(sInitStruct->sFrameFormat[bCount].uwWordcount * 8 )/ (sInitStruct->uwPixelFormat & 0x0f);
			bStillIndex=bCount;
		}
		} else {
			udwSpiData = (0)|(bUsedDataType << 16)|(1<<24);
			SPI_Write((uint16_t)(YUSHAN_IDP_GEN_WC_DI_0+4*bCount),4,(unsigned char*)&udwSpiData);
		}
	}

	udwSpiData=bVfIndex | ( bStillIndex << 4 ) | (bSofEofLength << 8)|0x3ff0000; 
	SPI_Write(YUSHAN_IDP_GEN_CONTROL,4,(unsigned char*)&udwSpiData);
	udwSpiData=0xc810;
	SPI_Write(YUSHAN_IDP_GEN_ERROR_LINES_EOF_GAP,2,(unsigned char*)&udwSpiData);

	  


	udwSpiData=sInitStruct->bNumberOfLanes-1;
	SPI_Write(YUSHAN_CSI2_TX_NUMBER_OF_LANES,1,(unsigned char*)&udwSpiData); 

	udwSpiData=1;
	SPI_Write(YUSHAN_CSI2_TX_PACKET_CONTROL,1,(unsigned char*)&udwSpiData); 
	udwSpiData=1;
	SPI_Write(YUSHAN_CSI2_TX_ENABLE,1,(unsigned char*)&udwSpiData); 

	
	
	udwSpiData=0x1;
	SPI_Write(YUSHAN_DTFILTER_BYPASS_MATCH0,1,(unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_DTFILTER_BYPASS_MATCH1,1,(unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_DTFILTER_BYPASS_MATCH2,1,(unsigned char*)&udwSpiData);
	udwSpiData=0x3;
	SPI_Write(YUSHAN_DTFILTER_BYPASS_MATCH3,1,(unsigned char*)&udwSpiData);
	udwSpiData=0x01;
	SPI_Write(YUSHAN_DTFILTER_BYPASS_ENABLE,1,(unsigned char*)&udwSpiData);

	

	udwSpiData=0xd;
	SPI_Write(YUSHAN_DTFILTER_DXO_MATCH0,1,(unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_DTFILTER_DXO_MATCH2,1,(unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_DTFILTER_DXO_MATCH3,1,(unsigned char*)&udwSpiData);
	udwSpiData=0x02;
	SPI_Write(YUSHAN_DTFILTER_DXO_MATCH1,1,(unsigned char*)&udwSpiData);
	udwSpiData=0x01;
	SPI_Write(YUSHAN_DTFILTER_DXO_ENABLE,1,(unsigned char*)&udwSpiData);


	

	
	udwSpiData=(uwHsizeStill&0xff) | ((uwHsizeVf&0xff) << 8 );
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT,2,(unsigned char*)&udwSpiData);
	
	udwSpiData=DXO_PDP_BASE_ADDR+DxOPDP_visible_line_size_7_0;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=((uwHsizeStill>>8)&0xff) | (((uwHsizeVf>>8)&0xff) << 8 );
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+8,2,(unsigned char*)&udwSpiData);
	
	udwSpiData=DXO_PDP_BASE_ADDR + DxOPDP_visible_line_size_15_8;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+8+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=0x0101;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+16,2,(unsigned char*)&udwSpiData);
	
	udwSpiData=DXO_PDP_BASE_ADDR + DxOPDP_newFrameCmd;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+16+2,3,(unsigned char*)&udwSpiData);

	
	udwSpiData=(uwHsizeStill&0xff) | ((uwHsizeVf&0xff) << 8 );
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+24,2,(unsigned char*)&udwSpiData);
	
	udwSpiData=DXO_DPP_BASE_ADDR + DxODPP_visible_line_size_7_0;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+24+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=((uwHsizeStill>>8)&0xff) | (((uwHsizeVf>>8)&0xff) << 8 );
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+32,2,(unsigned char*)&udwSpiData);
	
	udwSpiData=DXO_DPP_BASE_ADDR + DxODPP_visible_line_size_15_8;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+32+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=0x0101;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+40,2,(unsigned char*)&udwSpiData);
	
	udwSpiData=DXO_DPP_BASE_ADDR + DxODPP_newFrameCmd;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+40+2,3,(unsigned char*)&udwSpiData);

	
	udwSpiData=(uwHsizeStill&0xff) | ((uwHsizeVf&0xff) << 8);
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+48,2,(unsigned char*)&udwSpiData);
	
	udwSpiData=DXO_DOP_BASE_ADDR + DxODOP_visible_line_size_7_0;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+48+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=((uwHsizeStill>>8)&0xff) | (((uwHsizeVf>>8)&0xff) << 8);
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+56,2,(unsigned char*)&udwSpiData);
	
	udwSpiData=DXO_DOP_BASE_ADDR + DxODOP_visible_line_size_15_8;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+56+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=0x0101;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+64,2,(unsigned char*)&udwSpiData);
	
	udwSpiData=DXO_DOP_BASE_ADDR + DxODOP_newFrameCmd;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+64+2,3,(unsigned char*)&udwSpiData);

	
	udwSpiData=(uwHsizeStill&0xff) | ((uwHsizeVf&0xff) << 8);
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+72,2,(unsigned char*)&udwSpiData);
	udwSpiData=YUSHAN_LBE_PRE_DXO_H_SIZE;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+72+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=((uwHsizeStill>>8)&0xff) | (((uwHsizeVf>>8)&0xff) << 8);
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+80,2,(unsigned char*)&udwSpiData);
	udwSpiData=YUSHAN_LBE_PRE_DXO_H_SIZE+1;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+80+2,3,(unsigned char*)&udwSpiData);

	
	udwSpiData=(uwHsizeStill&0xff) | ((uwHsizeVf&0xff) << 8);
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+88,2,(unsigned char*)&udwSpiData);
	udwSpiData=YUSHAN_LBE_POST_DXO_H_SIZE;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+88+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=((uwHsizeStill>>8)&0xff) | (((uwHsizeVf>>8)&0xff) << 8);
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+96,2,(unsigned char*)&udwSpiData);
	udwSpiData=YUSHAN_LBE_POST_DXO_H_SIZE+1;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+96+2,3,(unsigned char*)&udwSpiData);

	
	udwSpiData=(uwHsizeStill&0xff) | ((uwHsizeVf&0xff) << 8);
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+104,2,(unsigned char*)&udwSpiData);
	udwSpiData=YUSHAN_LECCI_LINE_SIZE;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+104+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=((uwHsizeStill>>8)&0xff) | (((uwHsizeVf>>8)&0xff) << 8);
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+112,2,(unsigned char*)&udwSpiData);
	udwSpiData=YUSHAN_LECCI_LINE_SIZE+1;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+112+2,3,(unsigned char*)&udwSpiData);

	
	
	if (bDxoClkDiv != 0 && bPixClkDiv != 0) {
		uwLecciVf    = sInitStruct->uwLineBlankVf*bPixClkDiv/ bDxoClkDiv;
		uwLecciStill = sInitStruct->uwLineBlankStill*bPixClkDiv/ bDxoClkDiv;
	} else {
		uwLecciVf    = 300;
		uwLecciStill = 300;
	}

	
	udwSpiData=(uwLecciStill&0xff) | ((uwLecciVf&0xff) << 8 );
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+120,2,(unsigned char*)&udwSpiData); 
	udwSpiData=YUSHAN_LECCI_MIN_INTERLINE;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+120+2,3,(unsigned char*)&udwSpiData); 
	udwSpiData=((uwLecciStill>>8)&0xff) | (((uwLecciVf>>8)&0xff) << 8 );
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+128,2,(unsigned char*)&udwSpiData); 
	udwSpiData=YUSHAN_LECCI_MIN_INTERLINE+1;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+128+2,3,(unsigned char*)&udwSpiData);

 
	udwSpiData=17; 
	SPI_Write(YUSHAN_T1_DMA_REG_REFILL_ELT_NB,1,(unsigned char*)&udwSpiData); 
	udwSpiData=1;
	SPI_Write(YUSHAN_T1_DMA_REG_ENABLE,1,(unsigned char*)&udwSpiData); 

	
	

	udwSpiData=4;  
	SPI_Write(YUSHAN_LBE_PRE_DXO_READ_START,1,(unsigned char*)&udwSpiData); 
	udwSpiData=1;  
	SPI_Write(YUSHAN_LBE_PRE_DXO_ENABLE,1,(unsigned char*)&udwSpiData); 

	
	
	udwSpiData = 0x10;  
	SPI_Write(YUSHAN_LBE_POST_DXO_READ_START,1,(unsigned char*)&udwSpiData);
	udwSpiData=1;  
	SPI_Write(YUSHAN_LBE_POST_DXO_ENABLE,1,(unsigned char*)&udwSpiData);

	
	
	
	bDXODecimalFactor = bIdpDecimalFactor = 0;

	if(((bDxoClkDiv&0x01) == 1))		
		bDXODecimalFactor = 1;		
	if(((bPixClkDiv&0x01) == 1))		
		bIdpDecimalFactor = 1;

	bDxoClkDiv = bDxoClkDiv>>1;		
	bPixClkDiv = bPixClkDiv>>1;		

	if(bDxoClkDiv==bPixClkDiv)
		udwSpiData = (((2*bDxoClkDiv + bDXODecimalFactor)<<8) | 0x1);
	else
		udwSpiData = (((2*bDxoClkDiv + bDXODecimalFactor)<<8)|((bPixClkDiv*2 + bIdpDecimalFactor)-1));


	SPI_Write(YUSHAN_LECCI_OUT_BURST_CTRL,2,(unsigned char*)&udwSpiData);
	udwSpiData=0x01;
	SPI_Write(YUSHAN_LECCI_ENABLE,1,(unsigned char*)&udwSpiData);

	DEBUGLOG("[CAM] Yushan_Init return success\n");
	VERBOSELOG("[CAM] %s: End\n", __func__);
	return SUCCESS;

}


bool_t Yushan_Update_ImageChar(Yushan_ImageChar_t *sImageChar)
{
	uint8_t  *pData;
	bool_t fStatus;
	uint32_t udwSpiData;
	uint8_t  bData[20];

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	
	pData = (uint8_t *)bData;

	bData[0]=sImageChar->bImageOrientation;
	bData[1]=sImageChar->uwXAddrStart & 0xff;
	bData[2]=sImageChar->uwXAddrStart >> 8;
	bData[3]=sImageChar->uwYAddrStart & 0xff;
	bData[4]=sImageChar->uwYAddrStart >> 8;
	bData[5]=sImageChar->uwXAddrEnd & 0xff;
	bData[6]=sImageChar->uwXAddrEnd >> 8;
	bData[7]=sImageChar->uwYAddrEnd & 0xff;
	bData[8]=sImageChar->uwYAddrEnd >> 8;
	bData[9]=sImageChar->uwXEvenInc & 0xff;
	bData[10]=sImageChar->uwXEvenInc >> 8;
	bData[11]=sImageChar->uwXOddInc & 0xff;
	bData[12]=sImageChar->uwXOddInc >> 8;
	bData[13]=sImageChar->uwYEvenInc & 0xff;
	bData[14]=sImageChar->uwYEvenInc >> 8;
	bData[15]=sImageChar->uwYOddInc & 0xff;
	bData[16]=sImageChar->uwYOddInc >> 8;
	bData[17]=sImageChar->bBinning ;

	
	fStatus = SPI_Write(DXO_PDP_BASE_ADDR+DxOPDP_image_orientation_7_0, 18, pData);
	
	fStatus = SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_image_orientation_7_0, 18, pData);
	udwSpiData = 0x10000 ;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);
	fStatus = SPI_Write(DXO_DPP_BASE_ADDR+DxODPP_image_orientation_7_0-0x8000, 18, pData);
	udwSpiData = 0x8000 ;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;

}



bool_t Yushan_Update_SensorParameters(Yushan_GainsExpTime_t *sGainsExpInfo)
{
	uint8_t *pData;
	bool_t fStatus;
	uint32_t udwSpiData;
	uint8_t bData[20];

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	pData = (uint8_t *)bData;

	bData[0]=sGainsExpInfo->uwAnalogGainCodeGR & 0xff;
	bData[1]=sGainsExpInfo->uwAnalogGainCodeGR >> 8;
	bData[2]=sGainsExpInfo->uwAnalogGainCodeR & 0xff;
	bData[3]=sGainsExpInfo->uwAnalogGainCodeR >> 8;
	bData[4]=sGainsExpInfo->uwAnalogGainCodeB & 0xff;
	bData[5]=sGainsExpInfo->uwAnalogGainCodeB >> 8;
	bData[6]=sGainsExpInfo->uwPreDigGainGR & 0xff;
	bData[7]=sGainsExpInfo->uwPreDigGainGR >> 8;
	bData[8]=sGainsExpInfo->uwPreDigGainR & 0xff;
	bData[9]=sGainsExpInfo->uwPreDigGainR >> 8;
	bData[10]=sGainsExpInfo->uwPreDigGainB & 0xff;
	bData[11]=sGainsExpInfo->uwPreDigGainB >> 8;
	bData[12]=sGainsExpInfo->uwExposureTime & 0xff;
	bData[13]=sGainsExpInfo->uwExposureTime >> 8;
	
	
	fStatus = SPI_Write(DXO_PDP_BASE_ADDR+DxOPDP_analogue_gain_code_greenr_7_0, 14, pData);
	fStatus = SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_analogue_gain_code_greenr_7_0, 12, pData);
	pData = (uint8_t *)&(sGainsExpInfo->bRedGreenRatio);
	fStatus = SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_red_green_ratio_7_0, 2, pData);
	udwSpiData = 0x10000 ;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);
	pData = (uint8_t *)(sGainsExpInfo);
	fStatus = SPI_Write(DXO_DPP_BASE_ADDR+DxODPP_analogue_gain_code_greenr_7_0-0x8000, 2, pData);
	pData = (uint8_t *)&(sGainsExpInfo->uwPreDigGainGR);
	fStatus = SPI_Write(DXO_DPP_BASE_ADDR+DxODPP_pre_digital_gain_greenr_7_0-0x8000, 2, pData);
	pData = (uint8_t *)&(sGainsExpInfo->uwExposureTime);
	fStatus = SPI_Write(DXO_DPP_BASE_ADDR+DxODPP_exposure_time_7_0-0x8000, 2, pData);
	udwSpiData = 0x8000 ;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;

}



bool_t Yushan_Update_DxoPdp_TuningParameters(Yushan_DXO_PDP_Tuning_t *sDxoPdpTuning)
{
	uint8_t  *pData,fStatus;
	uint8_t bData[3];

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	pData = (uint8_t *)bData;
	bData[0]=sDxoPdpTuning->bDeadPixelCorrectionLowGain;
	bData[1]=sDxoPdpTuning->bDeadPixelCorrectionMedGain;
	bData[2]=sDxoPdpTuning->bDeadPixelCorrectionHiGain;

	
	
	fStatus = SPI_Write(DXO_PDP_BASE_ADDR+DxOPDP_dead_pixels_correction_lowGain_7_0, 3, pData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;

}





bool_t Yushan_Update_DxoDpp_TuningParameters(Yushan_DXO_DPP_Tuning_t *sDxoDppTuning)
{

	uint8_t  *pData;
	bool_t	fStatus;
	uint32_t udwSpiData;
	uint8_t bData[10];
	VERBOSELOG("[CAM] %s: Start\n", __func__);

	pData = (uint8_t *)bData;

	bData[0]=sDxoDppTuning->bTemporalSmoothing;
	bData[1]=sDxoDppTuning->uwFlashPreflashRating & 0xff ;
	bData[2]=sDxoDppTuning->uwFlashPreflashRating >> 8;
	bData[3]=sDxoDppTuning->bFocalInfo;

	udwSpiData = 0x10000 ;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);
	fStatus = SPI_Write(DXO_DPP_BASE_ADDR+DxODPP_temporal_smoothing_7_0-0x8000, 4,  pData);

	
  	udwSpiData = 0x8000 ;
  	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;

}


bool_t Yushan_Update_DxoDop_TuningParameters(Yushan_DXO_DOP_Tuning_t *sDxoDopTuning)
{
	uint8_t *pData;
	bool_t fStatus;
	uint8_t bData[10];

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	pData = (uint8_t *) bData;

	bData[0]=sDxoDopTuning->bEstimationMode ;
	
	fStatus = SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_estimation_mode_7_0, 1, pData);

	bData[0]=sDxoDopTuning->bSharpness;
	bData[1]=sDxoDopTuning->bDenoisingLowGain;
	bData[2]=sDxoDopTuning->bDenoisingMedGain;
	bData[3]=sDxoDopTuning->bDenoisingHiGain;
	bData[4]=sDxoDopTuning->bNoiseVsDetailsLowGain ;
	bData[5]=sDxoDopTuning->bNoiseVsDetailsMedGain ;
	bData[6]=sDxoDopTuning->bNoiseVsDetailsHiGain ;
	bData[7]=sDxoDopTuning->bTemporalSmoothing ;

	fStatus &= SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_sharpness_7_0, 8, pData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;

}




bool_t Yushan_Update_Commit(uint8_t  bPdpMode, uint8_t  bDppMode, uint8_t  bDopMode)
{
	uint8_t bData, *pData;
	bool_t fStatus = SUCCESS;
	uint32_t udwSpiData ;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	bData = 1;
	pData = &bData;

	
	SPI_Write(DXO_PDP_BASE_ADDR+DxOPDP_mode_7_0, 1, &bPdpMode);
	
	SPI_Write(DXO_PDP_BASE_ADDR+DxOPDP_execCmd, 1, pData);

	
	SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_mode_7_0, 1, &bDopMode);
	
	SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_execCmd, 1, pData);

	
	udwSpiData = 0x10000 ;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);
	
	SPI_Write(DXO_DPP_BASE_ADDR+DxODPP_mode_7_0-0x8000, 1, &bDppMode);
	udwSpiData = 0x18000 ;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);
	
	SPI_Write(DXO_DPP_BASE_ADDR+DxODPP_execCmd-0x10000, 1, pData);


	
	udwSpiData = 0x8000 ;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);

#if 0
	DEBUGLOG("[CAM] %s:Waiting for EOF_EXECCMD interrupt Starts here\n", __func__);
	
	
	fStatus = Yushan_WaitForInterruptEvent(EVENT_PDP_EOF_EXECCMD, TIME_100MS);
	if (!fStatus) {
		ERRORLOG("[CAM] %s:Failed in EVENT_PDP_EOF_EXECCMD interrupt\n", __func__);
		return FAILURE;
	}
	DEBUGLOG("[CAM] %s:DXO PDP commited now\n", __func__);

	fStatus = Yushan_WaitForInterruptEvent2(EVENT_DOP7_EOF_EXECCMD, TIME_100MS);
	if (!fStatus) {
		ERRORLOG("[CAM] %s:Failed in EVENT_DOP7_EOF_EXECCMD interrupt\n", __func__);
		return FAILURE;
	}
	DEBUGLOG("[CAM] %s:DXO DOP7 commited now\n", __func__);

	fStatus = Yushan_WaitForInterruptEvent(EVENT_DPP_EOF_EXECCMD, TIME_100MS);
	if (!fStatus) {
		ERRORLOG("[CAM] %s:Failed in EVENT_DPP_EOF_EXECCMD interrupt\n", __func__);
		return FAILURE;
	}
#endif

	DEBUGLOG("[CAM] %s:DXO IPs commited now\n", __func__);
	VERBOSELOG("[CAM] %s: End\n", __func__);
	
	return fStatus;

}



bool_t Yushan_CheckDxoConstraints(uint32_t udwParameters, uint32_t udwMinLimit, uint32_t fpDxo_Clk, uint32_t fpPixel_Clk, uint16_t uwFullLine, uint32_t * pMinValue)
{
	

	
	uint16_t	uwLocalDxoClk, uwLocalPixelClk;
	uint32_t	udwTemp1, udwTemp2;

	
	uwLocalDxoClk = fpDxo_Clk>>16;
	uwLocalPixelClk = fpPixel_Clk>>16;

	udwTemp1 = (udwParameters*0xFFFF)/uwLocalPixelClk;
	udwTemp2 = (udwMinLimit*0xFFFF)/uwLocalDxoClk;

	
	if (udwTemp1 < udwTemp2) {
		
		
		udwTemp1 = ((udwTemp2*uwLocalPixelClk)/0xFFFF); 
		*pMinValue = (udwTemp1+1);

		return FAILURE;
	}

	
	return SUCCESS;

}



uint32_t Yushan_Compute_Pll_Divs(uint32_t fpExternal_Clk, uint32_t fpTarget_PllClk)
{
	
	
	
	uint32_t		fpIDiv, fpClkPll; 
	uint32_t		fpBelow[4], fpAbove[4], *pDivPll;	
	uint32_t		fpFreqVco, fpIdFreq;
	uint8_t			bODiv, bLoop, bIDivCount = 0;
	uint8_t			bSpiData;
	bool_t			fStatus = SUCCESS;

	
	fpBelow[PLL_CLK_INDEX]=0;
	fpAbove[PLL_CLK_INDEX]=0x7FFFFFFF; 

	for (bIDivCount = 1; bIDivCount < 8; bIDivCount++) {
		
		fpIDiv = Yushan_ConvertTo16p16FP((uint16_t)bIDivCount);
		
		fpIdFreq = fpExternal_Clk/bIDivCount;
		
		if ((fpIdFreq >= 0x60000) && (fpIdFreq <= 0x320000)) {
			for (bLoop = 10; bLoop < 168; bLoop++) {
				
				fpFreqVco = fpIdFreq * 2 * bLoop;
				
				if ((fpFreqVco >= 0x3E80000) && (fpFreqVco <= 0x7D00000)) {
					for (bODiv = 1; bODiv < 64; bODiv++) {
						fpClkPll = (fpFreqVco / (2 * bODiv));
						if ((fpClkPll <= fpTarget_PllClk) && (fpClkPll >= fpBelow[PLL_CLK_INDEX]))
							fpBelow[PLL_CLK_INDEX] = fpClkPll, fpBelow[IDIV_INDEX] = fpIDiv, fpBelow[LOOP_INDEX] = Yushan_ConvertTo16p16FP((uint16_t)bLoop), fpBelow[ODIV_INDEX] = Yushan_ConvertTo16p16FP((uint16_t)bODiv);
						if ((fpClkPll >= fpTarget_PllClk) && (fpClkPll <= fpAbove[PLL_CLK_INDEX]))
							fpAbove[PLL_CLK_INDEX] = fpClkPll, fpAbove[IDIV_INDEX] = fpIDiv, fpAbove[LOOP_INDEX] = Yushan_ConvertTo16p16FP((uint16_t)bLoop), fpAbove[ODIV_INDEX] = Yushan_ConvertTo16p16FP((uint16_t)bODiv);
					}
				}
			}               
		}
	}

	
#if 0
	if ((fpTarget_PllClk - fpBelow[PLL_CLK_INDEX]) < (fpAbove[PLL_CLK_INDEX] - fpTarget_PllClk))
		pDivPll = &fpBelow[PLL_CLK_INDEX];
	else
		pDivPll = &fpAbove[PLL_CLK_INDEX];
#else
	
	
	if ((fpBelow[PLL_CLK_INDEX] <= fpTarget_PllClk)&&(fpAbove[PLL_CLK_INDEX]>fpTarget_PllClk))
		pDivPll = &fpBelow[PLL_CLK_INDEX];
	else if (fpAbove[PLL_CLK_INDEX] == fpTarget_PllClk)
		pDivPll = &fpAbove[PLL_CLK_INDEX];
	else {
		
		ERRORLOG("[CAM] %s: Error, Above Conditions not satisfied ...\n", __func__);
		return FAILURE;
	}
#endif

	
	
	bSpiData = (uint8_t)((*(pDivPll+1))>>16);
	fStatus	&= SPI_Write(YUSHAN_PLL_CTRL_MAIN+1, 1, (uint8_t *)  (&bSpiData));
	bSpiData = (uint8_t)((*(pDivPll+2))>>16);
	fStatus	&= SPI_Write(YUSHAN_PLL_LOOP_OUT_DF, 1, (uint8_t *) (&bSpiData));
	bSpiData = (uint8_t)((*(pDivPll+3))>>16);
	fStatus	&= SPI_Write(YUSHAN_PLL_LOOP_OUT_DF+1, 1, (uint8_t *) (&bSpiData));

	if (!fStatus) 
		return FAILURE; 
	else	
		return *pDivPll;
}






bool_t Yushan_Get_Version_Information(Yushan_Version_Info_t * sYushanVersionInfo) 
{
#if 1 
	uint32_t udwSpiBaseIndex;
	VERBOSELOG("[CAM] %s: Start\n", __func__);

	SPI_Read((DxODOP_ucode_id_7_0 + DXO_DOP_BASE_ADDR) , 4, (uint8_t *)(&sYushanVersionInfo->udwDopVersion));
	SPI_Read((DxODOP_calib_id_0_7_0+ DXO_DOP_BASE_ADDR), 4, (uint8_t *)(&sYushanVersionInfo->udwDopCalibrationVersion));	

	udwSpiBaseIndex = 0x010000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));
	
	SPI_Read(DxODPP_ucode_id_7_0+ DXO_DPP_BASE_ADDR-0x8000, 4, (uint8_t *)(&sYushanVersionInfo->udwDppVersion));
	SPI_Read(DxODPP_calib_id_0_7_0+ DXO_DPP_BASE_ADDR-0x8000, 4, (uint8_t *)(&sYushanVersionInfo->udwDppCalibrationVersion));

	udwSpiBaseIndex = 0x08000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

	SPI_Read((DxOPDP_ucode_id_7_0 + DXO_PDP_BASE_ADDR) , 4, (uint8_t *)(&sYushanVersionInfo->udwPdpVersion));
	SPI_Read((DxOPDP_calib_id_0_7_0+ DXO_PDP_BASE_ADDR), 4, (uint8_t *)(&sYushanVersionInfo->udwPdpCalibrationVersion));

#endif
	sYushanVersionInfo->bApiMajorVersion=API_MAJOR_VERSION;
	sYushanVersionInfo->bApiMinorVersion=API_MINOR_VERSION;

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return SUCCESS;
}





void	Yushan_AssignInterruptGroupsToPad1(uint16_t	uwAssignITRGrpToPad1)
{
	VERBOSELOG("[CAM] %s: Start\n", __func__);
	VERBOSELOG("[CAM] %s: uwAssignITRGrpToPad1:0x%x\n", __func__, uwAssignITRGrpToPad1);
	SPI_Write(YUSHAN_IOR_NVM_SEND_ITR_PAD1, 2, (uint8_t	*)&uwAssignITRGrpToPad1);
	VERBOSELOG("[CAM] %s: End\n", __func__);
}



bool_t Yushan_Intr_Enable(uint8_t *pIntrMask)
{
	
	
	uint8_t		bTotalEventInASet, *pSpiData, *pCompSpiData;
	uint8_t		bIntrCount = 1, bIntr32Count = 1, bIntrCountInSet = 0, bIntrSetID = 0;
	uint16_t	uwIntrSetOffset;
	uint32_t	udwLocalIntrMask, udwInterruptSetting, *pLocalIntrMask, udwSpiData = 0, udwCompSpiData = 0;
	uint32_t	udwLocalCompSPIMask = 0;
	uint16_t	ByteCount = 0;

	
	uint8_t		bFirstIndexForSet[] = EVENT_FIRST_INDEXFORSET;

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	
	pLocalIntrMask = (uint32_t *)(pIntrMask);
	udwLocalIntrMask	= *pLocalIntrMask;

	pSpiData = (uint8_t *)(&udwSpiData);
	pCompSpiData = (uint8_t *)(&udwCompSpiData);

	
	bTotalEventInASet = (bFirstIndexForSet[bIntrSetID+1] - bFirstIndexForSet[bIntrSetID]);

	
	while (bIntrCount <= (TOTAL_INTERRUPT_COUNT+1)) {
		if (bIntrCountInSet == bTotalEventInASet) {
			
			uwIntrSetOffset = (bIntrSetID*INTERRUPT_SET_SIZE);

			
			if (bTotalEventInASet == 32)
				udwLocalCompSPIMask = 0x00000000;
			else
				udwLocalCompSPIMask = (uint32_t)(0xFFFFFFFF<<bTotalEventInASet);
			udwCompSpiData = (uint32_t)(~(udwSpiData|udwLocalCompSPIMask));

			
			
			if (gPllLocked) {
				
				ByteCount = (uint16_t)((bTotalEventInASet+7)>>3);
				
				if (udwSpiData != 0)
					
					SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_ENABLE), ByteCount, pSpiData);
				
				if (udwCompSpiData != 0)
					
					SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_DISABLE), ByteCount, pCompSpiData);
			} else {
				
				if (udwSpiData != 0) {
					
					SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_ENABLE), 1, pSpiData);
				SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_ENABLE+1), 1, pSpiData+1);
				SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_ENABLE+2), 1, pSpiData+2);
				SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_ENABLE+3), 1, pSpiData+3);
			}
			
			
			
				if (udwCompSpiData != 0) {
					
					SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_DISABLE), 1, pCompSpiData);
					SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_DISABLE+1), 1, pCompSpiData+1);
					SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_DISABLE+2), 1, pCompSpiData+2);
					SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_DISABLE+3), 1, pCompSpiData+3);
				}
			}

			
			bIntrCountInSet = 0;
			udwSpiData = 0;

			
			bIntr32Count--;
			bIntrCount--;
			
			bIntrSetID++;

			
				bTotalEventInASet = (bFirstIndexForSet[bIntrSetID+1] - bFirstIndexForSet[bIntrSetID]);
		} else {
			
			udwInterruptSetting = ((udwLocalIntrMask>>(bIntr32Count-1))&0x01);
			
			udwSpiData |= (udwInterruptSetting<<(bIntrCountInSet));

			
			bIntrCountInSet++;
		}

		
		if (!(bIntr32Count % 32)) {
			
			bIntr32Count = 1;
			
			pLocalIntrMask++;
			udwLocalIntrMask	= *pLocalIntrMask;
		} else
			bIntr32Count++;

		
		bIntrCount++;
	}

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return SUCCESS;
}




void Yushan_Intr_Status_Read(uint8_t *bListOfInterrupts,	bool_t	fSelect_Intr_Pad)
{
	uint16_t	uwSpiData = 0, 	uwListOfITRGrpToPad1 = 0, RaisedGrpForPAD = 0;
	uint8_t		bIntrSetID = 0;
	bool_t		fStatus = SUCCESS;
	uint8_t		bSpiData = 0;

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	VERBOSELOG("[CAM] %s: Check for interrupts on Pad%i\n", __func__, fSelect_Intr_Pad);

	
	memset((void *)bListOfInterrupts, 0, 96/8);

	
	if (gPllLocked) {
		SPI_Read(YUSHAN_IOR_NVM_SEND_ITR_PAD1, 2, (uint8_t	*)&uwListOfITRGrpToPad1);
	} else {
		fStatus &= SPI_Read(YUSHAN_IOR_NVM_SEND_ITR_PAD1, 1, (uint8_t *)(&bSpiData));
		uwListOfITRGrpToPad1 = (uint16_t) bSpiData;
		fStatus &= SPI_Read(YUSHAN_IOR_NVM_SEND_ITR_PAD1+1, 1, (uint8_t *)(&bSpiData));
		uwListOfITRGrpToPad1 |= (((uint16_t) bSpiData)<<8);
	}


	if(fSelect_Intr_Pad==INTERRUPT_PAD_0)
		uwListOfITRGrpToPad1	=	~uwListOfITRGrpToPad1;

	if (gPllLocked) {
		fStatus &= SPI_Read(YUSHAN_IOR_NVM_INTR_STATUS, 2, (uint8_t *)(&uwSpiData));
	} else {
		fStatus &= SPI_Read(YUSHAN_IOR_NVM_INTR_STATUS, 1, (uint8_t *)(&bSpiData));
		uwSpiData = (uint16_t) bSpiData;
		fStatus &= SPI_Read(YUSHAN_IOR_NVM_INTR_STATUS+1, 1, (uint8_t *)(&bSpiData));
		uwSpiData |= ( ((uint16_t) bSpiData)<<8 );
	}

	RaisedGrpForPAD = uwSpiData & uwListOfITRGrpToPad1;
	while (bIntrSetID < TOTAL_INTERRUPT_SETS) {
		if ((RaisedGrpForPAD>>bIntrSetID)&0x01) {
			
			Yushan_Read_IntrEvent(bIntrSetID, (uint32_t *)bListOfInterrupts);
		}
		
		bIntrSetID++;
	}

	VERBOSELOG("[CAM] %s: Read interrupts status:0x%08x, 0x%08x, 0x%08x\n", __func__, *((uint32_t *)bListOfInterrupts), *(((uint32_t *)bListOfInterrupts)+1), *(((uint32_t *)bListOfInterrupts)+2));
	VERBOSELOG("[CAM] %s: End\n", __func__);
}





void Yushan_Read_IntrEvent(uint8_t bIntrSetID, uint32_t *udwListOfInterrupts)
{

	uint8_t		bInterruptID, bTotalEventInASet, bIntrCountInSet = 0, bTempIntrStatus = 0;
	uint8_t		bFirstIndexForSet[] = EVENT_FIRST_INDEXFORSET;
	uint16_t	uwIntrSetOffset;
	uint32_t	udwIntrStatus = 0, udwIntrEnableStatus = 0, udwCombinedStatus = 0;
	uint16_t	ByteCount = 0;

	bTotalEventInASet = (bFirstIndexForSet[bIntrSetID + 1] - bFirstIndexForSet[bIntrSetID]);

	
	uwIntrSetOffset = (bIntrSetID * INTERRUPT_SET_SIZE);


	if (gPllLocked) {	
		
		ByteCount = (uint16_t)((bTotalEventInASet+7)>>3);
		
		SPI_Read((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset), ByteCount, (uint8_t *)(&udwIntrStatus));
		
		SPI_Read((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_ENABLE_STATUS), ByteCount, (uint8_t *)(&udwIntrEnableStatus));
	} else {
		SPI_Read((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset),   1, (uint8_t *)(&bTempIntrStatus));
		udwIntrStatus = (uint32_t)(bTempIntrStatus);
		SPI_Read((uint16_t)((YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset)+1), 1, (uint8_t *)(&bTempIntrStatus));
		udwIntrStatus |= ((uint32_t)(bTempIntrStatus))<<8;
		SPI_Read((uint16_t)((YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset)+2), 1, (uint8_t *)(&bTempIntrStatus));
		udwIntrStatus |= ((uint32_t)(bTempIntrStatus))<<16;
		SPI_Read((uint16_t)((YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset)+3), 1, (uint8_t *)(&bTempIntrStatus));
		udwIntrStatus |= ((uint32_t)(bTempIntrStatus))<<24;

		
		SPI_Read((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_ENABLE_STATUS),    1, (uint8_t *)(&bTempIntrStatus));
		udwIntrEnableStatus = (uint32_t)(bTempIntrStatus);
		SPI_Read((uint16_t)((YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_ENABLE_STATUS) +1), 1, (uint8_t *)(&bTempIntrStatus));
		udwIntrEnableStatus |= ((uint32_t)(bTempIntrStatus))<<8;
		SPI_Read((uint16_t)((YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_ENABLE_STATUS) +2), 1, (uint8_t *)(&bTempIntrStatus));
		udwIntrEnableStatus |= ((uint32_t)(bTempIntrStatus))<<16;
		SPI_Read((uint16_t)((YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_ENABLE_STATUS) +3), 1, (uint8_t *)(&bTempIntrStatus));
		udwIntrEnableStatus |= ((uint32_t)(bTempIntrStatus))<<24;
	}

	DEBUGLOG("[CAM] udwIntrStatus:  0x%x, udwIntrEnableStatus: 0x%x\n", udwIntrStatus, udwIntrEnableStatus);
	udwCombinedStatus = (udwIntrStatus & udwIntrEnableStatus);
	
	while (bIntrCountInSet < bTotalEventInASet) {
		if ((udwCombinedStatus>>bIntrCountInSet) & 0x01) {
			bInterruptID = (bFirstIndexForSet[bIntrSetID] + bIntrCountInSet);

			Yushan_AddnRemoveIDInList(bInterruptID, udwListOfInterrupts, ADD_INTR_TO_LIST);

		}
		bIntrCountInSet++;
	}
}



bool_t Yushan_Intr_Status_Clear(uint8_t *bListOfInterrupts)
{
	uint8_t		bTotalEventInASet; 
	uint8_t		bIntrCount = 1, bIntr32Count = 1, bIntrCountInSet = 0, bIntrSetID = 0;
	uint16_t	uwIntrSetOffset;
	uint32_t	udwLocalIntrMask, udwInterruptSetting, *pLocalIntrMask, udwSpiData = 0;
	uint16_t	ByteCount = 0;

	
	uint8_t		bFirstIndexForSet[] = EVENT_FIRST_INDEXFORSET;

	VERBOSELOG("[CAM] %s: Start\n", __func__);


	pLocalIntrMask = (uint32_t *) bListOfInterrupts; 
	udwLocalIntrMask	= *pLocalIntrMask;

	
	bTotalEventInASet = (bFirstIndexForSet[bIntrSetID+1] - bFirstIndexForSet[bIntrSetID]);

	
	while (bIntrCount <= (TOTAL_INTERRUPT_COUNT + 1)) {
		if (bIntrCountInSet == bTotalEventInASet) {
			
			if (udwSpiData != 0) {
				
				ByteCount = (uint16_t)((bTotalEventInASet+7)>>3);
				
				uwIntrSetOffset = (bIntrSetID * INTERRUPT_SET_SIZE);
				
				
				
				
				
				SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_STATUS_CLEAR), ByteCount, (uint8_t *)(&udwSpiData));
			}

			
			bIntrCountInSet = 0;
			udwSpiData = 0;

			
			bIntr32Count--;
			bIntrCount--;
			
			bIntrSetID++;

			
			bTotalEventInASet = (bFirstIndexForSet[bIntrSetID + 1] - bFirstIndexForSet[bIntrSetID]);
		} else {
			
			udwInterruptSetting = ((udwLocalIntrMask>>(bIntr32Count-1))&0x01);
			
			udwSpiData |= (udwInterruptSetting<<(bIntrCountInSet));
			
			bIntrCountInSet++;
		}

		
		if (!(bIntr32Count % 32)) {
			
			bIntr32Count = 1;

			
			pLocalIntrMask++;
			udwLocalIntrMask	= *pLocalIntrMask;
		} else
			bIntr32Count++;


		
		bIntrCount++;

	}

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return SUCCESS;
}



bool_t	Yushan_Check_Pad_For_IntrID(uint8_t	bInterruptId)
{

	uint8_t		bFirstIndexForSet[] = EVENT_FIRST_INDEXFORSET;
	uint8_t		bIntrSetID = 0;
	uint16_t	uwIntrSetsDivertedToPad1 = 0;

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	
	SPI_Read(YUSHAN_IOR_NVM_SEND_ITR_PAD1 , 2, (uint8_t	*)&uwIntrSetsDivertedToPad1);

	
	while (bIntrSetID < TOTAL_INTERRUPT_SETS) {
		if ((bInterruptId >= bFirstIndexForSet[bIntrSetID]) && (bInterruptId < bFirstIndexForSet[bIntrSetID+1])) {
			if ((uwIntrSetsDivertedToPad1>>bIntrSetID)&0x01) {
				VERBOSELOG("[CAM] %s: End\n", __func__);
				return INTERRUPT_PAD_1;
			} else {
				VERBOSELOG("[CAM] %s: End\n", __func__);
				return INTERRUPT_PAD_0;
			}
		} else
			bIntrSetID++;
	}

	
	VERBOSELOG("[CAM] %s: End\n", __func__);
	return INTERRUPT_PAD_0;

}


bool_t Yushan_CheckForInterruptIDInList(uint8_t bInterruptID, uint32_t *udwProtoInterruptList)
{

	bool_t		fStatus = 0;
	uint8_t		bIntrDWordInList, bIndexInCurrentDWord;
	uint32_t	*pListOfInterrupt;


	
	pListOfInterrupt =  udwProtoInterruptList;


	if ((1<=bInterruptID)&&(bInterruptID<=32))
		bIntrDWordInList = 0;
	else if ((33<=bInterruptID)&&(bInterruptID<=64))
		bIntrDWordInList = 1;
	else
		bIntrDWordInList = 2;

	
	pListOfInterrupt = pListOfInterrupt + bIntrDWordInList;

	bIndexInCurrentDWord = (bInterruptID - (bIntrDWordInList*32)) - 1;


	
	fStatus = (bool_t)(((*pListOfInterrupt)>>bIndexInCurrentDWord) & 0x00000001);

	return fStatus;

}




void Yushan_AddnRemoveIDInList(uint8_t bInterruptID, uint32_t *udwListOfInterrupts, bool_t fAddORClear)
{

	uint8_t		bIntrDWordInList, bIndexInCurrentDWord;
	uint32_t	*pListOfInterrupt, udwTempIntrList, udwMask = 0x00000001;

	
	pListOfInterrupt = udwListOfInterrupts;

	if ((1<=bInterruptID)&&(bInterruptID<=32))
		bIntrDWordInList = 0;
	else if ((33<=bInterruptID)&&(bInterruptID<=64))
		bIntrDWordInList = 1;
	else
		bIntrDWordInList = 2;

	
	pListOfInterrupt = pListOfInterrupt + bIntrDWordInList;

	
	bIndexInCurrentDWord = (bInterruptID - (bIntrDWordInList*32)) - 1;

	
	

	
	if (fAddORClear == ADD_INTR_TO_LIST) {
		
		udwTempIntrList = ( ((*pListOfInterrupt)>>(bIndexInCurrentDWord)) | udwMask ); 
		*pListOfInterrupt |= (udwTempIntrList << (bIndexInCurrentDWord));
	} else if (fAddORClear == DEL_INTR_FROM_LIST) {
		
		udwTempIntrList = ( ((*pListOfInterrupt)>>(bIndexInCurrentDWord)) & udwMask ); 
		*pListOfInterrupt &= ~(udwTempIntrList << (bIndexInCurrentDWord));
	}


}








void	Yushan_DCPX_CPX_Enable(void)
{

	uint8_t	bSpiData = 0;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	
	bSpiData = 0x01;
	SPI_Write(YUSHAN_SMIA_DCPX_ENABLE, 1, &bSpiData); 
	bSpiData = 0x08;
	SPI_Write(YUSHAN_SMIA_DCPX_MODE_REQ, 1, &bSpiData); 
	bSpiData = 0x0A;
	SPI_Write(YUSHAN_SMIA_DCPX_MODE_REQ+1, 1, &bSpiData); 

	
	bSpiData = 0x01;
	SPI_Write(YUSHAN_SMIA_CPX_CTRL_REQ, 1, &bSpiData); 
	
	SPI_Write(YUSHAN_SMIA_CPX_MODE_REQ, 1, &bSpiData); 

	VERBOSELOG("[CAM] %s: End\n", __func__);
}


uint8_t Yushan_GetCurrentStreamingMode(void)
{
	uint8_t bSpiData,currentEvent;
	uint8_t CurrentStreamingMode;

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	
	SPI_Read(YUSHAN_T1_DMA_REG_STATUS, 1, &bSpiData);

	currentEvent = (bSpiData&0x70)>>4;
	switch (currentEvent) {
	case 1:
		CurrentStreamingMode = YUSHAN_FRAME_FORMAT_STILL_MODE;
		break;
	case 2:
		CurrentStreamingMode = YUSHAN_FRAME_FORMAT_VF_MODE;
		break;
	default:
		
		CurrentStreamingMode = 255;
	}

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return CurrentStreamingMode;
}


bool_t	Yushan_Context_Config_Update(Yushan_New_Context_Config_t	*sYushanNewContextConfig)
{

	
	uint8_t		bVfStillIndex, bVFIndex, bStillIndex, bVFMask=0;
	
	uint8_t		bDataType=0, bCurrentDataType=0, bActiveDatatype=1, bRawFormat=0, bWCAlreadyPresent = 0;
	
	uint8_t		bPixClkDiv=0, bDxoClkDiv=0, bCount=0;
	uint16_t	uwNewHSize=0, uwLecci=0;
	uint32_t	udwSpiData = 0;
	uint16_t	wordCount = 0;
	uint8_t		bCurrentStreamingMode;
	
	VERBOSELOG("[CAM] %s: Start\n", __func__);

	
	bCurrentStreamingMode = Yushan_GetCurrentStreamingMode();
	if (bCurrentStreamingMode == YUSHAN_FRAME_FORMAT_STILL_MODE) {
		
		sYushanNewContextConfig->bSelectStillVfMode = YUSHAN_FRAME_FORMAT_VF_MODE;
	} else {
		if (bCurrentStreamingMode == YUSHAN_FRAME_FORMAT_VF_MODE) {
		
		sYushanNewContextConfig->bSelectStillVfMode = YUSHAN_FRAME_FORMAT_STILL_MODE;
		}
	}

	
	
	
	

	if ((sYushanNewContextConfig->uwPixelFormat&0x0F)==0x0A){
		bRawFormat = RAW10;
		bDataType  = 0x2b;
	} else if((sYushanNewContextConfig->uwPixelFormat&0x0F)==0x08) {
		bRawFormat= RAW8; 
		if(((sYushanNewContextConfig->uwPixelFormat>>8)&0x0F)==0x08)
			bDataType = 0x2a;
		else 
			bDataType = 0x30; 
	}

	
	SPI_Read(YUSHAN_IDP_GEN_CONTROL, 1, (uint8_t *)(&bVfStillIndex));
	bVFIndex = bVfStillIndex&0x0F;
	bStillIndex = (bVfStillIndex&0xF0)>>4;

	
	uwNewHSize = sYushanNewContextConfig->uwActivePixels;

	for (bCount = 0; bCount < 13; bCount++) {
		SPI_Read((uint16_t)(YUSHAN_IDP_GEN_WC_DI_0+4*bCount),4,(unsigned char*)&udwSpiData);
		wordCount = udwSpiData & 0x0000ffff; 
		if (wordCount==0) 
			break;

		bCurrentDataType = (udwSpiData>>16);							
		udwSpiData = ((udwSpiData&0xFFFF)*8)/bRawFormat;				
		
		
		if ((uwNewHSize == udwSpiData) && (bCurrentDataType == bDataType)) {
			bWCAlreadyPresent = 1;
			if((bCount==bVFIndex)||(bCount==bStillIndex)) {
				VERBOSELOG("[CAM] %s: End\n", __func__);
				return SUCCESS;			
			} else
				break;					
		}
	}

	
	
	SPI_Read(YUSHAN_IDP_GEN_CONTROL, 4, (uint8_t *)(&udwSpiData));
	if (sYushanNewContextConfig->bSelectStillVfMode == YUSHAN_FRAME_FORMAT_VF_MODE) {
		udwSpiData &= 0xFFFFFFF0;
		udwSpiData |= bCount;
		
		bVFMask = 1;
	} else if (sYushanNewContextConfig->bSelectStillVfMode == YUSHAN_FRAME_FORMAT_STILL_MODE) {
		udwSpiData &= 0xFFFFFF0F;
		udwSpiData |= bCount << 4;
		
		bVFMask = 0;
	}

	
	SPI_Write((uint16_t)(YUSHAN_IDP_GEN_CONTROL), 4, (uint8_t *)(&udwSpiData));

	
	if (bWCAlreadyPresent && (bCount != 13)) {
		
		SPI_Write((uint16_t)((YUSHAN_IDP_GEN_WC_DI_0+(4*bCount))+3),	1,	(unsigned char*)&bActiveDatatype);
	} else if (bCount != 13) {
		
		
		udwSpiData=(sYushanNewContextConfig->uwActivePixels*bRawFormat)/8;
		SPI_Write((uint16_t)(YUSHAN_CSI2_TX_PACKET_SIZE_0+0xc*bCount),	4,	(unsigned char*)&udwSpiData);

		SPI_Write((uint16_t)(YUSHAN_CSI2_TX_DI_INDEX_CTRL_0+0xc*bCount),	1,	(unsigned char*)&bDataType);

		udwSpiData = udwSpiData | (bDataType<<16) | (bActiveDatatype<<24);
		SPI_Write((uint16_t)(YUSHAN_IDP_GEN_WC_DI_0+4*bCount),	4,	(unsigned char*)&udwSpiData);
	} else {
		
		ERRORLOG("[CAM] %s: Error, No entry vacant. Exiting ...\n", __func__);
		return FAILURE;
	}
	
	
	
	udwSpiData = (uwNewHSize & 0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT + bVFMask),1,(unsigned char*)&udwSpiData);
	udwSpiData = ((uwNewHSize>>8)&0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+8+ bVFMask),1,(unsigned char*)&udwSpiData);

	
	udwSpiData=(uwNewHSize & 0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+24+ bVFMask),1,(unsigned char*)&udwSpiData);
	udwSpiData=((uwNewHSize>>8)&0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+32+ bVFMask),1,(unsigned char*)&udwSpiData);

	
	udwSpiData=(uwNewHSize & 0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+48+ bVFMask),1,(unsigned char*)&udwSpiData);
	udwSpiData=((uwNewHSize>>8)&0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+56+ bVFMask),1,(unsigned char*)&udwSpiData);

	
	udwSpiData=(uwNewHSize & 0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+72+ bVFMask),1,(unsigned char*)&udwSpiData);
	udwSpiData=((uwNewHSize>>8)&0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+80+ bVFMask),1,(unsigned char*)&udwSpiData);

	
	udwSpiData=(uwNewHSize & 0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+88+ bVFMask),1,(unsigned char*)&udwSpiData);
	udwSpiData=((uwNewHSize>>8)&0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+96+ bVFMask),1,(unsigned char*)&udwSpiData);

	
	udwSpiData=(uwNewHSize & 0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+104+ bVFMask),1,(unsigned char*)&udwSpiData);
	udwSpiData=((uwNewHSize>>8)&0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+112+ bVFMask),1,(unsigned char*)&udwSpiData);

	
	
	SPI_Read(YUSHAN_CLK_DIV_FACTOR,1,(unsigned char*)&bDxoClkDiv); 
	SPI_Read(YUSHAN_CLK_DIV_FACTOR+1,1,(unsigned char*)&bPixClkDiv); 

	if ((bDxoClkDiv !=0) && (bPixClkDiv !=0))
		uwLecci = (sYushanNewContextConfig->uwLineBlank*bPixClkDiv)/ bDxoClkDiv;
	else
		uwLecci = 300;

	
	udwSpiData=(uwLecci&0xff);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+120+bVFMask),1,(unsigned char*)&udwSpiData);

	udwSpiData=((uwLecci>>8)&0xff);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+128+bVFMask),1,(unsigned char*)&udwSpiData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return SUCCESS;


}





bool_t Yushan_Update_DxoDop_Af_Strategy(uint8_t  bAfStrategy)
{
	bool_t	fStatus = SUCCESS;
	VERBOSELOG("[CAM] %s: Start\n", __func__);
	
#if DxODOP_dfltVal_ucode_id_15_8 == 2
	fStatus = SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_af_strategy_7_0, 1, &bAfStrategy);
#else
	VERBOSELOG("[CAM] %s: DOP used not compatiblewith this function\n", __func__);
#endif
	VERBOSELOG("[CAM] %s: End with Status:%i\n", __func__, fStatus);
	return fStatus;
}



bool_t Yushan_AF_ROI_Update(Yushan_AF_ROI_t  *sYushanAfRoi, uint8_t bNumOfActiveRoi) 
{

	uint8_t		bSpiData[4];
	uint8_t		bStatus = SUCCESS, bCount=0;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	if (!bNumOfActiveRoi) { 
		DEBUGLOG("[CAM] %s:No ROI Activated so exiting with SUCCESS\n", __func__);
		return SUCCESS;
	} else {
		
		bStatus &= SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_ROI_active_number_7_0, 1, (uint8_t*)(&bNumOfActiveRoi));
	}

	

	
	while (bCount < bNumOfActiveRoi) {
		bSpiData[0] = (sYushanAfRoi->bXStart);
		bSpiData[1] = (sYushanAfRoi->bYStart);
		bSpiData[2] = (sYushanAfRoi->bXEnd);
		bSpiData[3] = (sYushanAfRoi->bYEnd);

		
		
		bStatus &= SPI_Write((uint16_t)(DXO_DOP_BASE_ADDR+DxODOP_ROI_0_x_start_7_0 + bCount*4), 4, (uint8_t*)(&bSpiData[0]));
		
		bCount++;

		
		sYushanAfRoi++;
	}
	VERBOSELOG("[CAM] %s: End\n", __func__);
	return bStatus;
}




bool_t Yushan_Enter_Standby_Mode(void)
{
	uint8_t		bSpiData;
	uint32_t	udwSpiData;
	bool_t		fStatus = SUCCESS;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	
	bSpiData = 0;
	fStatus &= SPI_Write(YUSHAN_MIPI_RX_ENABLE, 1, (uint8_t*)(&bSpiData));
	fStatus &= SPI_Write(YUSHAN_MIPI_TX_ENABLE, 1, (uint8_t*)(&bSpiData));


	
	
	
	
	SPI_Read(YUSHAN_CLK_CTRL,   1, (uint8_t*)(&bSpiData));		
	SPI_Read(YUSHAN_CLK_CTRL+2, 2, (uint8_t*)(&udwSpiData));	
	udwSpiData = (bSpiData | (udwSpiData<<16));

	
	bSpiData = 0x00;
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL, 1, (uint8_t*)(&bSpiData));
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+2, 1, (uint8_t*)(&bSpiData)); 
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+3, 1, (uint8_t*)(&bSpiData)); 

	
	SPI_Read(YUSHAN_CLK_CTRL+1, 1, (uint8_t*)(&bSpiData));
	bSpiData &= 0xFE;	
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+1, 1, (uint8_t*)(&bSpiData));

	
	
	bSpiData |= ((bSpiData>>1)|0x01)<<1;						
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+1, 1, (uint8_t*)(&bSpiData));

	
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL, 1, (uint8_t*)(&udwSpiData));		
	udwSpiData = (udwSpiData >> 16);										
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+2, 1, (uint8_t*)(&udwSpiData));	
	udwSpiData = (udwSpiData >> 8);
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+3, 1, (uint8_t*)(&udwSpiData));	
	 
	
	SPI_Read(YUSHAN_PLL_CTRL_MAIN, 1, (uint8_t*)(&bSpiData));
	bSpiData |= 0x01;			
	fStatus &= SPI_Write(YUSHAN_PLL_CTRL_MAIN, 1, (uint8_t*)(&bSpiData));

	

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;


}




bool_t Yushan_Exit_Standby_Mode(Yushan_Init_Struct_t * sInitStruct)
{
	uint8_t		bSpiData;
	uint32_t	udwSpiData;
	bool_t		fStatus = SUCCESS;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	

	
	SPI_Read(YUSHAN_PLL_CTRL_MAIN, 1, (uint8_t*)(&bSpiData));
	bSpiData &= 0xFE;	
	SPI_Write(YUSHAN_PLL_CTRL_MAIN, 1, (uint8_t*)(&bSpiData));

	fStatus &= Yushan_WaitForInterruptEvent(EVENT_PLL_STABLE, TIME_100MS);
	
	if (!fStatus) {
		ERRORLOG("[CAM] %s: Error: EVENT_PLL_STABLE not received. Exiting...\n", __func__);
		return fStatus;
	}

	
	
	SPI_Read(YUSHAN_CLK_CTRL,   1, (uint8_t*)(&bSpiData));		
	udwSpiData = (uint32_t)(bSpiData);
	SPI_Read(YUSHAN_CLK_CTRL+2, 1, (uint8_t*)(&bSpiData));		
	udwSpiData |= (((uint32_t)(bSpiData))<<8);
	SPI_Read(YUSHAN_CLK_CTRL+3, 1, (uint8_t*)(&bSpiData));		
	udwSpiData |= (((uint32_t)(bSpiData))<<16);


	
	bSpiData = 0x00;
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL, 1, (uint8_t*)(&bSpiData));
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+2, 1, (uint8_t*)(&bSpiData));		
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+3, 1, (uint8_t*)(&bSpiData));		

	
	SPI_Read(YUSHAN_CLK_CTRL+1, 1, (uint8_t*)(&bSpiData));
	bSpiData &= (bSpiData&0xFD);						
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+1, 1, (uint8_t*)(&bSpiData));

	
	
	bSpiData |= 0x01;											
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+1, 1, (uint8_t*)(&bSpiData));
	
	
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL, 1, (uint8_t*)(&udwSpiData));		
	udwSpiData = (udwSpiData >> 16);										
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+2, 2, (uint8_t*)(&udwSpiData));	


	
	if ( sInitStruct->bNumberOfLanes == 1)
		udwSpiData=0x11; 
	else
	if ( sInitStruct->bNumberOfLanes == 2)
		udwSpiData=0x31; 
	else
	if ( sInitStruct->bNumberOfLanes == 4)
		udwSpiData=0xf1; 

	fStatus &= SPI_Write(YUSHAN_MIPI_RX_ENABLE, 1,(uint8_t*)(&udwSpiData)); 
	udwSpiData |= 0x02; 
	fStatus &= SPI_Write(YUSHAN_MIPI_TX_ENABLE, 1,(uint8_t*)(&udwSpiData));

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;

}




bool_t Yushan_Swap_Rx_Pins (bool_t fClkLane, bool_t fDataLane1, bool_t fDataLane2, bool_t fDataLane3, bool_t fDataLane4)
{
	bool_t		fStatus; 
	uint8_t		bSpiData = 0;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	bSpiData = (fClkLane | (fDataLane1<<4) | (fDataLane2<<5) | (fDataLane3<<6) | (fDataLane4<<7));

	DEBUGLOG("[CAM] %s:Spi data for Swapping Rx pins is %4.4x\n", __func__, bSpiData);

	fStatus = SPI_Write(YUSHAN_MIPI_RX_SWAP_PINS, 1, &bSpiData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;

}



bool_t Yushan_Invert_Rx_Pins (bool_t fClkLane, bool_t fDataLane1, bool_t fDataLane2, bool_t fDataLane3, bool_t fDataLane4)
{

	bool_t		fStatus; 
	uint8_t		bSpiData = 0;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	bSpiData = (fClkLane | (fDataLane1<<4) | (fDataLane2<<5) | (fDataLane3<<6) | (fDataLane4<<7));
	VERBOSELOG("[CAM] %s:Spi data for Inverting Rx pins is %4.4x\n", __func__, bSpiData);

	fStatus = SPI_Write(YUSHAN_MIPI_RX_INVERT_HS, 1, &bSpiData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;
}




bool_t Yushan_Assert_Reset(uint32_t udwModuleMask, uint8_t bResetORDeReset)
{
	uint8_t bCurrentModuleToReset, bCurrentModule=0;
	uint8_t	bSpiData;
	bool_t	bSetORReset;

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	if (!(udwModuleMask & 0xFFFFFFFF)) {
		ERRORLOG("[CAM] %s:Mask not given for reset/dereset. Returning FAILURE\n", __func__);
		return FAILURE;	
	}

	while (bCurrentModule < TOTAL_MODULES) {

		bSetORReset = ((udwModuleMask>>bCurrentModule) & 0x01);

		
		if (!bSetORReset) {
			bCurrentModule++;
			pr_info("[CAM] Current Module is %d\n", bCurrentModule);
			continue;
		} else {
			bCurrentModuleToReset = bCurrentModule + 1;
			bCurrentModule++;
		}


		
		if (bResetORDeReset)
			
			bSpiData = 0x11;
		else
			
			bSpiData = 0x01;

		
		pr_info("[CAM] Current module is %d and Module to reset/Dereset is %d\n", bCurrentModule, bCurrentModuleToReset);
		
		
		switch (bCurrentModuleToReset) {
#if 0
			case DFT_RESET:
				break;
#endif
			case T1_DMA:
				SPI_Write(YUSHAN_T1_DMA_REG_ENABLE, 1, &bSpiData);
				break;
			case CSI2_RX:
				
				if(bResetORDeReset)
					
					bSpiData = 0x70;
				else
					
					bSpiData = 0x00;

				SPI_Write(YUSHAN_CSI2_RX_ENABLE, 1, &bSpiData);
				
				
				if(bResetORDeReset)
					
					bSpiData = 0x11;
				else
					
					bSpiData = 0x01;
				break;
			case IT_POINT:
				SPI_Write(YUSHAN_ITPOINT_ENABLE, 1, &bSpiData);
				break;
			case IDP_GEN:

				
				SPI_Read(YUSHAN_IDP_GEN_AUTO_RUN, 1, &bSpiData);

				if(bResetORDeReset)
					bSpiData |= 0x10;
				else
					bSpiData &=0x01;
				SPI_Write(YUSHAN_IDP_GEN_AUTO_RUN, 1, &bSpiData);

				break;
			case MIPI_RX_DTCHK:
				SPI_Write(YUSHAN_MIPI_RX_DTCHK_ENABLE, 1, &bSpiData);
				break;
			case SMIA_PATTERN_GEN:
				SPI_Write(YUSHAN_PATTERN_GEN_ENABLE, 1, &bSpiData);
				break;
			case SMIA_DCPX:
				SPI_Write(YUSHAN_SMIA_DCPX_ENABLE, 1, &bSpiData);
				break;
			case P2W_FIFO_WR:
				
				if(bResetORDeReset)
					
					bSpiData = 0x03;
				else
					
					bSpiData = 0x00;

				SPI_Write(YUSHAN_P2W_FIFO_WR_CTRL, 1, &bSpiData);
				
				
				if(bResetORDeReset)
					
					bSpiData = 0x11;
				else
					
					bSpiData = 0x01;

				break;
			case P2W_FIFO_RD:
				
				if(bResetORDeReset)
					
					bSpiData = 0x03;
				else
					
					bSpiData = 0x00;

				SPI_Write(YUSHAN_P2W_FIFO_RD_CTRL, 1, &bSpiData);
				
				
				if(bResetORDeReset)
					
					bSpiData = 0x11;
				else
					
					bSpiData = 0x01;

				break;
			case CSI2_TX_WRAPPER:
				if(bResetORDeReset)
					
					bSpiData = 0x01;
				else
					
					bSpiData = 0x00;

				SPI_Write(YUSHAN_CSI2_TX_WRAPPER_CTRL, 1, &bSpiData);
				
				
				if(bResetORDeReset)
					
					bSpiData = 0x11;
				else
					
					bSpiData = 0x01;

				break;

			case CSI2_TX:
				SPI_Write(YUSHAN_CSI2_TX_ENABLE, 1, &bSpiData);
				break;
			case LINE_FILTER_BYPASS:
				SPI_Write(YUSHAN_LINE_FILTER_BYPASS_ENABLE, 1, &bSpiData);
				break;
			case DT_FILTER_BYPASS:
				SPI_Write(YUSHAN_DTFILTER_BYPASS_ENABLE, 1, &bSpiData);
				break;
			case LINE_FILTER_DXO:
				SPI_Write(YUSHAN_LINE_FILTER_DXO_ENABLE, 1, &bSpiData);
				break;
			case DT_FILTER_DXO:
				SPI_Write(YUSHAN_DTFILTER_DXO_ENABLE, 1, &bSpiData);
				break;
			case EOF_RESIZE_PRE_DXO:
				SPI_Write(YUSHAN_EOF_RESIZE_PRE_DXO_ENABLE, 1, &bSpiData);
				break;
			case LBE_PRE_DXO:
				SPI_Write(YUSHAN_LBE_PRE_DXO_ENABLE, 1, &bSpiData);
				break;
			case EOF_RESIZE_POST_DXO:
				SPI_Write(YUSHAN_EOF_RESIZE_POST_DXO_ENABLE, 1, &bSpiData);
				break;
			case LECCI_RESET:
				SPI_Write(YUSHAN_LECCI_ENABLE, 1,  &bSpiData);
				break;
			case LBE_POST_DXO:
				SPI_Write(YUSHAN_LBE_POST_DXO_ENABLE, 1,  &bSpiData);
				break;

		}



	}
	VERBOSELOG("[CAM] %s: End\n", __func__);
	return SUCCESS;

}




bool_t	Yushan_PatternGenerator(Yushan_Init_Struct_t *sInitStruct, uint8_t	bPatternReq, bool_t	bDxoBypassForTestPattern) 
{

	uint8_t		bSpiData, bDxoClkDiv, bPixClkDiv;
	uint32_t	udwSpiData=0, uwHSize, uwVSize;
	uint32_t	uwVBlk;
	uint16_t	uwMaxInterframeGap;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	uwHSize = sInitStruct->uwActivePixels;
	uwVSize = sInitStruct->uwLines;
	uwVBlk	= sInitStruct->uwFrameBlank;

	
	sInitStruct->sFrameFormat[1].bDatatype = 0x12;
	sInitStruct->sFrameFormat[1].uwWordcount=(uwHSize)*(sInitStruct->uwPixelFormat&0x0F)/8;

	udwSpiData=sInitStruct->sFrameFormat[1].uwWordcount;
	SPI_Write(YUSHAN_CSI2_TX_PACKET_SIZE_0+0xc*1,4,(unsigned char*)&udwSpiData);

	udwSpiData=sInitStruct->sFrameFormat[1].bDatatype ;
	SPI_Write(YUSHAN_CSI2_TX_DI_INDEX_CTRL_0+0xc*1,1,(unsigned char*)&udwSpiData); 

	
	
	SPI_Write(YUSHAN_LECCI_BYPASS_CTRL, 1, (unsigned char*)&bDxoBypassForTestPattern); 

	SPI_Read(YUSHAN_CLK_DIV_FACTOR,1,(unsigned char*)&bDxoClkDiv);
	SPI_Read(YUSHAN_CLK_DIV_FACTOR+1,1,(unsigned char*)&bPixClkDiv);
	
	if ( bDxoClkDiv !=0 && bPixClkDiv !=0 )
	  udwSpiData    = sInitStruct->uwLineBlankStill*bPixClkDiv/ bDxoClkDiv;
	else
	  udwSpiData    = 300;
	SPI_Write(YUSHAN_LECCI_MIN_INTERLINE, 2, (unsigned char*)&udwSpiData);

	
	udwSpiData = uwHSize;
	SPI_Write(YUSHAN_LECCI_LINE_SIZE, 2, (unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_LBE_PRE_DXO_H_SIZE, 2, (unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_LBE_POST_DXO_H_SIZE, 2, (unsigned char*)&udwSpiData);

	
	SPI_Write(YUSHAN_PATTERN_GEN_PATTERN_TYPE_REQ, 1, &bPatternReq);
	
	udwSpiData = (uwHSize|((uwHSize+sInitStruct->uwLineBlankStill)<<16));
	SPI_Write(YUSHAN_IDP_GEN_LINE_LENGTH, 4, (uint8_t *)(&udwSpiData));
	if (bDxoBypassForTestPattern) {
	udwSpiData = (uwVSize|(0xFFF0<<16));
		SPI_Write(YUSHAN_IDP_GEN_FRAME_LENGTH, 4, (uint8_t *)(&udwSpiData));
	} else {
		uwMaxInterframeGap = (0xFFF0/uwHSize)*uwHSize;
		uwVBlk = uwVBlk - (uwMaxInterframeGap/uwHSize);
		udwSpiData = ((uwVSize+uwVBlk)|(uwMaxInterframeGap<<16));
		SPI_Write(YUSHAN_IDP_GEN_FRAME_LENGTH, 4, (uint8_t *)(&udwSpiData));
		
		udwSpiData = 1;
		SPI_Write(YUSHAN_LINE_FILTER_BYPASS_ENABLE, 1, (unsigned char *)&udwSpiData);
		udwSpiData = 1;
		SPI_Write(YUSHAN_LINE_FILTER_BYPASS_LSTART_LEVEL, 2, (unsigned char *)&udwSpiData);
		udwSpiData = uwVBlk+1;
		SPI_Write(YUSHAN_LINE_FILTER_BYPASS_LSTOP_LEVEL, 2, (unsigned char *)&udwSpiData);
		
		udwSpiData = 0x1;
		SPI_Write(YUSHAN_DTFILTER_BYPASS_MATCH0, 1, (unsigned char *)&udwSpiData);
		udwSpiData = 0xd;
		SPI_Write(YUSHAN_DTFILTER_BYPASS_MATCH1, 1, (unsigned char *)&udwSpiData);
		SPI_Write(YUSHAN_DTFILTER_BYPASS_MATCH2, 1, (unsigned char *)&udwSpiData);
		udwSpiData = 0x03;
		SPI_Write(YUSHAN_DTFILTER_BYPASS_MATCH3, 1, (unsigned char *)&udwSpiData);
		udwSpiData = 0x01;
		SPI_Write(YUSHAN_DTFILTER_BYPASS_ENABLE, 1, (unsigned char *)&udwSpiData);
		
		udwSpiData = 1;
		SPI_Write(YUSHAN_LINE_FILTER_DXO_ENABLE, 1, (unsigned char *)&udwSpiData);
		udwSpiData = uwVBlk+1;
		SPI_Write(YUSHAN_LINE_FILTER_DXO_LSTART_LEVEL, 2, (unsigned char *)&udwSpiData);
		udwSpiData = uwVSize+uwVBlk+1;
		SPI_Write(YUSHAN_LINE_FILTER_DXO_LSTOP_LEVEL, 2, (unsigned char *)&udwSpiData);
	}

	
	if(sInitStruct->uwPixelFormat == 0x0a08) {
		
		bSpiData = 0x00;
		SPI_Write(YUSHAN_SMIA_DCPX_ENABLE, 1, &bSpiData); 
	}

	

	
	bSpiData = 1;
	SPI_Write(YUSHAN_PATTERN_GEN_ENABLE, 1, &bSpiData);

	
	bSpiData = 1;
	SPI_Write(YUSHAN_IDP_GEN_AUTO_RUN, 1, &bSpiData);
	VERBOSELOG("[CAM] %s: End\n", __func__);
	return SUCCESS;

}




void	Yushan_DXO_Lecci_Bypass(void)
{
	uint8_t	bSpiData;

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	
	bSpiData=0x01;
	SPI_Write(YUSHAN_LECCI_BYPASS_CTRL, 1, (unsigned char*)&bSpiData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
}




void	Yushan_DXO_DTFilter_Bypass(void)
{
	uint32_t	udwSpiData=0;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	
	
	udwSpiData=0x1;
	SPI_Write(YUSHAN_DTFILTER_BYPASS_MATCH0,1,(unsigned char*)&udwSpiData);
	udwSpiData=0xd;
	SPI_Write(YUSHAN_DTFILTER_BYPASS_MATCH1,1,(unsigned char*)&udwSpiData);
	udwSpiData=0x02;
	SPI_Write(YUSHAN_DTFILTER_BYPASS_MATCH2,1,(unsigned char*)&udwSpiData);
	udwSpiData=0x03;
	SPI_Write(YUSHAN_DTFILTER_BYPASS_MATCH3,1,(unsigned char*)&udwSpiData);
	udwSpiData=0x01;
	SPI_Write(YUSHAN_DTFILTER_BYPASS_ENABLE,1,(unsigned char*)&udwSpiData);

	

	udwSpiData=0x5;
	SPI_Write(YUSHAN_DTFILTER_DXO_MATCH0,1,(unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_DTFILTER_DXO_MATCH2,1,(unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_DTFILTER_DXO_MATCH3,1,(unsigned char*)&udwSpiData);
	
	udwSpiData=0x05;
	SPI_Write(YUSHAN_DTFILTER_DXO_MATCH1,1,(unsigned char*)&udwSpiData);
	udwSpiData=0x01;
	SPI_Write(YUSHAN_DTFILTER_DXO_ENABLE,1,(unsigned char*)&udwSpiData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
	
}






bool_t Yushan_Read_AF_Statistics(Yushan_AF_Stats_t* sYushanAFStats, uint8_t	bNumOfActiveRoi, uint16_t *frameIdx)
{

	uint8_t		bStatus = SUCCESS, bCount = 0;
	uint32_t	udwSpiData[4];
	uint16_t	val;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

#if 0 
			YushanPrintFrameNumber();
			YushanPrintDxODOPAfStrategy();
			YushanPrintImageInformation();
			YushanPrintVisibleLineSizeAndRoi();
#endif

#if 0
	
	bStatus &= SPI_Read(DXO_DOP_BASE_ADDR+DxODOP_ROI_active_number_7_0, 1, (uint8_t*)(&bNumOfActiveRoi));
	
#endif

	if (!bNumOfActiveRoi) 
		return SUCCESS;
	if (frameIdx != NULL) {
		SPI_Read(DXO_DOP_BASE_ADDR+DxODOP_frame_number_7_0, 2, (uint8_t *)(&val));
		*frameIdx= val;
	}

	
	
	while (bCount < bNumOfActiveRoi) {
		
		bStatus &= SPI_Read((uint16_t)(DXO_DOP_BASE_ADDR + DxODOP_ROI_0_stats_G_7_0 + bCount*16), 16, (uint8_t *)(&udwSpiData[0]));

		
		sYushanAFStats[bCount].udwAfStatsGreen = udwSpiData[0];
		sYushanAFStats[bCount].udwAfStatsRed = udwSpiData[1];
		sYushanAFStats[bCount].udwAfStatsBlue = udwSpiData[2];
		sYushanAFStats[bCount].udwAfStatsConfidence = udwSpiData[3];
		DEBUGLOG("[CAM]%s, G:%d, R:%d, B:%d, confidence:%d (%d) \n", __func__,
			sYushanAFStats[bCount].udwAfStatsGreen,
			sYushanAFStats[bCount].udwAfStatsRed,
			sYushanAFStats[bCount].udwAfStatsBlue,
			sYushanAFStats[bCount].udwAfStatsConfidence,
			*frameIdx);

		
		bCount++;
	}

	VERBOSELOG("[CAM] %s: End With Status: %i\n", __func__, bStatus);
	return bStatus;

}



#define YUSHAN_REGISTER_CHECK(reg_addr) udwSpiData = 0; SPI_Read(reg_addr, 4, (uint8_t *)(&udwSpiData)); pr_info("[CAM] %s: %s: 0x%08x\n", __func__, #reg_addr, udwSpiData);
#define YUSHAN_DOP_REGISTER_CHECK(reg_addr) udwSpiData = 0; SPI_Read(reg_addr + DXO_DOP_BASE_ADDR, 1, (uint8_t *)(&udwSpiData)); pr_info("[CAM] %s: %s: 0x%02x\n", __func__, #reg_addr, udwSpiData);
#define YUSHAN_PDP_REGISTER_CHECK(reg_addr) udwSpiData = 0; SPI_Read(reg_addr + DXO_PDP_BASE_ADDR, 1, (uint8_t *)(&udwSpiData)); pr_info("[CAM] %s: %s: 0x%02x\n", __func__, #reg_addr, udwSpiData);
#define YUSHAN_DPP_REGISTER_CHECK(reg_addr) udwSpiData = 0; SPI_Read(reg_addr + DXO_DPP_BASE_ADDR-0x8000, 1, (uint8_t *)(&udwSpiData)); pr_info("[CAM] %s: %s: 0x%02x\n", __func__, #reg_addr, udwSpiData);
#define YUSHAN_DPP_REGISTER_CHECK2(reg_addr) udwSpiData = 0; SPI_Read(reg_addr + DXO_DPP_BASE_ADDR-0x10000, 1, (uint8_t *)(&udwSpiData)); pr_info("[CAM] %s: %s: 0x%02x\n", __func__, #reg_addr, udwSpiData);
void Yushan_Status_Snapshot(void)
{
	uint32_t	udwSpiData;
	uint32_t	udwSpiBaseIndex;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	DEBUGLOG("[CAM] %s: **** CLK CONFIG  CHECK ****\n", __func__);
	YUSHAN_REGISTER_CHECK(YUSHAN_CLK_DIV_FACTOR);
	YUSHAN_REGISTER_CHECK(YUSHAN_CLK_DIV_FACTOR_2);
	YUSHAN_REGISTER_CHECK(YUSHAN_CLK_CTRL);
	YUSHAN_REGISTER_CHECK(YUSHAN_PLL_CTRL_MAIN);
	YUSHAN_REGISTER_CHECK(YUSHAN_PLL_LOOP_OUT_DF);

	DEBUGLOG("[CAM] %s: **** CSI2 RX INTERFACE  CHECK ****\n", __func__);
	YUSHAN_REGISTER_CHECK(YUSHAN_CSI2_RX_FRAME_NUMBER);
	YUSHAN_REGISTER_CHECK(YUSHAN_CSI2_RX_DATA_TYPE);
	YUSHAN_REGISTER_CHECK(YUSHAN_CSI2_RX_WORD_COUNT);
	YUSHAN_REGISTER_CHECK(YUSHAN_ITM_CSI2RX_STATUS);

	DEBUGLOG("[CAM] %s: **** PRE-DXO CHECK ****\n", __func__);
	YUSHAN_REGISTER_CHECK(YUSHAN_IDP_GEN_WC_DI_0);
	YUSHAN_REGISTER_CHECK(YUSHAN_IDP_GEN_WC_DI_1);
	YUSHAN_REGISTER_CHECK(YUSHAN_IDP_GEN_WC_DI_2);
	YUSHAN_REGISTER_CHECK(YUSHAN_IDP_GEN_WC_DI_3);
	YUSHAN_REGISTER_CHECK(YUSHAN_IDP_GEN_WC_DI_4);
	YUSHAN_REGISTER_CHECK(YUSHAN_IDP_GEN_WC_DI_5);
	YUSHAN_REGISTER_CHECK(YUSHAN_IDP_GEN_WC_DI_6);
	YUSHAN_REGISTER_CHECK(YUSHAN_IDP_GEN_WC_DI_7);
	YUSHAN_REGISTER_CHECK(YUSHAN_IDP_GEN_WC_DI_8);
	YUSHAN_REGISTER_CHECK(YUSHAN_IDP_GEN_WC_DI_9);
	YUSHAN_REGISTER_CHECK(YUSHAN_IDP_GEN_WC_DI_10);
	YUSHAN_REGISTER_CHECK(YUSHAN_IDP_GEN_WC_DI_11);
	YUSHAN_REGISTER_CHECK(YUSHAN_IDP_GEN_WC_DI_12);
	YUSHAN_REGISTER_CHECK(YUSHAN_IDP_GEN_WC_DI_13);
	YUSHAN_REGISTER_CHECK(YUSHAN_IDP_GEN_WC_DI_14);
	YUSHAN_REGISTER_CHECK(YUSHAN_IDP_GEN_CONTROL);
	YUSHAN_REGISTER_CHECK(YUSHAN_ITM_IDP_STATUS);
	YUSHAN_REGISTER_CHECK(YUSHAN_T1_DMA_REG_STATUS);

	DEBUGLOG("[CAM] %s: **** DXO PDP CHECK ****\n", __func__);
	YUSHAN_PDP_REGISTER_CHECK(DxOPDP_ucode_id_7_0);
	YUSHAN_PDP_REGISTER_CHECK(DxOPDP_ucode_id_15_8);
	YUSHAN_PDP_REGISTER_CHECK(DxOPDP_hw_id_7_0);
	YUSHAN_PDP_REGISTER_CHECK(DxOPDP_hw_id_15_8);
	YUSHAN_PDP_REGISTER_CHECK(DxOPDP_calib_id_0_7_0);
	YUSHAN_PDP_REGISTER_CHECK(DxOPDP_calib_id_1_7_0);
	YUSHAN_PDP_REGISTER_CHECK(DxOPDP_calib_id_2_7_0);
	YUSHAN_PDP_REGISTER_CHECK(DxOPDP_calib_id_3_7_0);
	YUSHAN_PDP_REGISTER_CHECK(DxOPDP_error_code_7_0);
	YUSHAN_PDP_REGISTER_CHECK(DxOPDP_frame_number_7_0);
	YUSHAN_PDP_REGISTER_CHECK(DxOPDP_frame_number_15_8);
	YUSHAN_PDP_REGISTER_CHECK(DxOPDP_visible_line_size_7_0);
	YUSHAN_PDP_REGISTER_CHECK(DxOPDP_visible_line_size_15_8);

	DEBUGLOG("[CAM] %s: **** DXO DPP CHECK ****\n", __func__);
	udwSpiBaseIndex = 0x010000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));
	YUSHAN_DPP_REGISTER_CHECK(DxODPP_ucode_id_7_0);
	YUSHAN_DPP_REGISTER_CHECK(DxODPP_ucode_id_15_8);
	YUSHAN_DPP_REGISTER_CHECK(DxODPP_hw_id_7_0);
	YUSHAN_DPP_REGISTER_CHECK(DxODPP_hw_id_15_8);
	YUSHAN_DPP_REGISTER_CHECK(DxODPP_calib_id_0_7_0);
	YUSHAN_DPP_REGISTER_CHECK(DxODPP_calib_id_1_7_0);
	YUSHAN_DPP_REGISTER_CHECK(DxODPP_calib_id_2_7_0);
	YUSHAN_DPP_REGISTER_CHECK(DxODPP_calib_id_3_7_0);
	YUSHAN_DPP_REGISTER_CHECK(DxODPP_error_code_7_0);
	YUSHAN_DPP_REGISTER_CHECK(DxODPP_frame_number_7_0);
	YUSHAN_DPP_REGISTER_CHECK(DxODPP_frame_number_15_8);
	YUSHAN_DPP_REGISTER_CHECK(DxODPP_visible_line_size_7_0);
	YUSHAN_DPP_REGISTER_CHECK(DxODPP_visible_line_size_15_8);
	udwSpiBaseIndex = 0x08000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

	DEBUGLOG("[CAM] %s: **** DXO DOP CHECK ****\n", __func__);
	YUSHAN_DOP_REGISTER_CHECK(DxODOP_ucode_id_7_0);
	YUSHAN_DOP_REGISTER_CHECK(DxODOP_ucode_id_15_8);
	YUSHAN_DOP_REGISTER_CHECK(DxODOP_hw_id_7_0);
	YUSHAN_DOP_REGISTER_CHECK(DxODOP_hw_id_15_8);
	YUSHAN_DOP_REGISTER_CHECK(DxODOP_calib_id_0_7_0);
	YUSHAN_DOP_REGISTER_CHECK(DxODOP_calib_id_1_7_0);
	YUSHAN_DOP_REGISTER_CHECK(DxODOP_calib_id_2_7_0);
	YUSHAN_DOP_REGISTER_CHECK(DxODOP_calib_id_3_7_0);
	YUSHAN_DOP_REGISTER_CHECK(DxODOP_error_code_7_0);
	YUSHAN_DOP_REGISTER_CHECK(DxODOP_frame_number_7_0);
	YUSHAN_DOP_REGISTER_CHECK(DxODOP_frame_number_15_8);
	YUSHAN_DOP_REGISTER_CHECK(DxODOP_visible_line_size_7_0);
	YUSHAN_DOP_REGISTER_CHECK(DxODOP_visible_line_size_15_8);
	YUSHAN_DOP_REGISTER_CHECK(DxODOP_binning_7_0);

	DEBUGLOG("[CAM] %s: **** POST-DXO CHECK ****\n", __func__);
	YUSHAN_REGISTER_CHECK(YUSHAN_P2W_FIFO_WR_STATUS);
	YUSHAN_REGISTER_CHECK(YUSHAN_P2W_FIFO_RD_STATUS);
	YUSHAN_REGISTER_CHECK(YUSHAN_ITM_P2W_UFLOW_STATUS);
	YUSHAN_REGISTER_CHECK(YUSHAN_ITM_LBE_POST_DXO_STATUS);

	DEBUGLOG("[CAM] %s: **** CSI2 RX INTERFACE  CHECK ****\n", __func__);
	YUSHAN_REGISTER_CHECK(YUSHAN_CSI2_TX_PACKET_SIZE_0);
	YUSHAN_REGISTER_CHECK(YUSHAN_CSI2_TX_DI_INDEX_CTRL_0);
	YUSHAN_REGISTER_CHECK(YUSHAN_CSI2_TX_LINE_NO_0);
	YUSHAN_REGISTER_CHECK(YUSHAN_CSI2_TX_PACKET_SIZE_1);
	YUSHAN_REGISTER_CHECK(YUSHAN_CSI2_TX_DI_INDEX_CTRL_1);
	YUSHAN_REGISTER_CHECK(YUSHAN_CSI2_TX_LINE_NO_1);
	YUSHAN_REGISTER_CHECK(YUSHAN_CSI2_TX_PACKET_SIZE_2);
	YUSHAN_REGISTER_CHECK(YUSHAN_CSI2_TX_DI_INDEX_CTRL_2);
	YUSHAN_REGISTER_CHECK(YUSHAN_CSI2_TX_LINE_NO_2);
	YUSHAN_REGISTER_CHECK(YUSHAN_CSI2_TX_FRAME_NO_0);
	YUSHAN_REGISTER_CHECK(YUSHAN_ITM_CSI2TX_STATUS);
	VERBOSELOG("[CAM] %s: End\n", __func__);
}

