/*******************************************************************************
################################################################################
#                             C STMicroelectronics
#    Reproduction and Communication of this document is strictly prohibited
#      unless specifically authorized in writing by STMicroelectronics.
#------------------------------------------------------------------------------
#                             Imaging Division
################################################################################
File Name:	Yushan_API.c
Author:		Rajdeep Patel
Description: Contains API functions to be used by host to Initialize Yushan clocks,
			 regsiters and DXO parameters. Also BOOT the DXO IPs.
********************************************************************************/
#include "yushan_registermap.h"
#include "DxODOP_regMap.h"
#include "DxODPP_regMap.h"
#include "DxOPDP_regMap.h"
#include "Yushan_API.h"
#include "Yushan_Platform_Specific.h"




/************************************************************************
						External Symbols
************************************************************************/
bool_t	gPllLocked;



/*******************************************************************
API Function:	Yushan_Init_LDO. Performs the LDO initialization
Input:			void
Return:			SUCCESS(1),FALIURE(0)
********************************************************************/
bool_t	Yushan_Init_LDO(bool_t	bUseExternalLDO)
{

	bool_t		fStatus = SUCCESS;
	uint8_t		bSpiReadData = 0, bLDOTrimValue = 0, bSpiData = 0;
	uint16_t	uwCount = 500;	/* Some high number */

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	/* NVM Ready check */
	SPI_Read(YUSHAN_IOR_NVM_STATUS, 1, (uint8_t *)(&bSpiReadData));
	while (bSpiReadData != 1) {
		/* NVM is not ready to be read. Check again the READY_STATUS */
		SPI_Read(YUSHAN_IOR_NVM_STATUS, 1, (uint8_t *)(&bSpiReadData));
		/* Decrement the counter. */
		uwCount--;
		if (!uwCount)
			break;	/* Maximum wait ends here */
	}

	/* Enter, if NVM is ready for read */
	if (bSpiReadData == 1) {
		/* Now read the LDO trim IDEAL value */
		SPI_Read(YUSHAN_IOR_NVM_DATA_WORD_3, 1, (uint8_t *)(&bLDOTrimValue));
		if (bLDOTrimValue>>3 == 1)				/* Check for Validity of IDEAL Trim value */
			bLDOTrimValue &= 0xF7;
		else {
			/* No Valid Ideal trim value, read the Default value */
			bLDOTrimValue = 0;
			SPI_Read(YUSHAN_IOR_NVM_DATA_WORD_2 + 3, 1, (uint8_t *)(&bLDOTrimValue));
			if (bLDOTrimValue>>3 == 1)			/* Check for Validity of Default Trim value */
				bLDOTrimValue &= 0xF7;
			else							/* No Ideal, nor valid default */
				bLDOTrimValue = 0 ;			/* DEFAULT_LDO_TRIM_VALUE; */
		}

		/* Write the LDO_Trim value to the LDO */
		SPI_Write(YUSHAN_PRIVATE_TEST_LDO_NVM_CTRL, 1, (uint8_t *)(&bLDOTrimValue));

		/* Mode transition of LDO: Sequence has to be followed as it is done below: */
		bSpiReadData = 0;
		SPI_Read(YUSHAN_PRIVATE_TEST_LDO_CTRL,  1, (uint8_t *)(&bSpiReadData));
		bSpiData = bSpiReadData;

		DEBUGLOG("[CAM] %s: Yushan bUseExternalLDO = %d\n", __func__, bUseExternalLDO);
		if(!bUseExternalLDO) {
			/* For running LDO in internal mode */
			bSpiData = (bSpiData&0xF7);			/* tst_BGPDLV to 0 */
			SPI_Write(YUSHAN_PRIVATE_TEST_LDO_CTRL,  1, (uint8_t *)(&bSpiData));
			bSpiData = (bSpiData&0xFE);			/* tst_DLVDPDLV to 0 */
			SPI_Write(YUSHAN_PRIVATE_TEST_LDO_CTRL,  1, (uint8_t *)(&bSpiData));
			bSpiData = (bSpiData&0xFD);			/* tst_REGPDLV to 0 */
			SPI_Write(YUSHAN_PRIVATE_TEST_LDO_CTRL,  1, (uint8_t *)(&bSpiData));
		} else {
			/* For running LDO in external/test mode */
			bSpiData |= 0x04;
			SPI_Write(YUSHAN_PRIVATE_TEST_LDO_CTRL,  1, (uint8_t *)(&bSpiData));
		}

		/* Wait for LDO_STABLE interrupt */
		DEBUGLOG("[CAM] %s: Waiting for EVENT_LDO_STABLE Interrupt\n", __func__);
		fStatus &= Yushan_WaitForInterruptEvent(EVENT_LDO_STABLE, TIME_20MS);

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




/*******************************************************************
API Function:	Init_Clk_Yushan, perform the clock initialization of
				Yushan and starts the Pll.
	Input:			InitStruct
Return:			SUCCESS(1),FALIURE(0)
********************************************************************/
bool_t Yushan_Init_Clocks(Yushan_Init_Struct_t *sInitStruct, Yushan_SystemStatus_t *sSystemStatus, uint32_t	*udwIntrMask)
{

	uint32_t		fpTargetPll_Clk, fpSys_Clk, fpPixel_Clk, fpDxo_Clk, fpIdp_Clk, fpDxoClockLow;		// 16.16 Fixed Point notation
	uint32_t		fpSysClk_Div, fpIdpClk_div, fpDxoClk_Div, fpTempVar;				// 16.16 Fixed Point notation

	uint32_t		udwSpiData = 0;
	uint8_t			bRx_PixelWidth, bTx_PixelWidth;
	int8_t			bCount;
	uint8_t			fStatus = SUCCESS, fDXOStatus = DXO_NO_ERR;
	uint8_t			bSpiData;
	uint32_t 		udwFullLine, udwFullFrame, udwFrameBlank;

	/* IntrBitMask: Total 12 Bytes (83 Intr). Currently DXO intrs and Pll Lock intr has been enabled */
	//uint32_t	udwIntrMask[] = {0x00130C30, 0x00000000, 0x00018000};	// DXO_DOP_NEW_FR_PR enabled

	/* Set of Dividers */
	/* float_t		fpDividers[] = {1, 1.5, 2, 2.5, 3, 3.5, 4, 5, 8, 10}; */
	uint32_t		fpDividers[] = {0x10000, 0x18000, 0x20000, 0x28000, 0x30000, 0x38000, 0x40000, 0x48000, 0x50000, 0x58000, 0x60000,	\
										0x68000, 0x70000, 0x78000, 0x80000, 0xA0000};

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	/* Switch to Host Clock */
	DEBUGLOG("[CAM] %s:Switching to Host clock while evaluating different clock dividers\n", __func__);
	bSpiData = 0x00;
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+1, 1, (uint8_t *)(&bSpiData)); /* Tx, Rx, Dxo, Idp clks enabled. */
	bSpiData = 0x02;
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+1, 1, (uint8_t *)(&bSpiData)); /* Tx, Rx, Dxo, Idp clks enabled. */
	bSpiData = 0x19;
	fStatus &= SPI_Write(YUSHAN_PLL_CTRL_MAIN, 1, (uint8_t *)(&bSpiData));	/* Pll in power down state keeping earlier config intact */
	/* Rx and Tx Pixel width */
	bRx_PixelWidth = bTx_PixelWidth = (sInitStruct->uwPixelFormat) & (0x000F);

	/* fpTargetPll_Clk is same as BitRate per lane. */
	fpTargetPll_Clk = Yushan_ConvertTo16p16FP((uint16_t)sInitStruct->uwBitRate);
	/* Compute Pll components */
	fpTargetPll_Clk = Yushan_Compute_Pll_Divs(sInitStruct->fpExternalClock, fpTargetPll_Clk);
	if (!fpTargetPll_Clk) {
		ERRORLOG("[CAM] %s:Wrong PLL Clk calculated. Returning", __func__);
		VERBOSELOG("[CAM] %s: End\n", __func__);
		return FAILURE;
	}

	/* IDP_CLK: */
	/* fpidp_clk = (sInitStruct->uwBitRate * bNumberOfLanes)/bTx_PixelWidth; */
	/* or fpidp_clk = fpTargetPll_Clk/fpIdpClk_div; */
	
	/* fpIdpClk_div = (bTx_PixelWidth/sInitStruct->bNumberOfLanes); */
	/* fpIdpClk_div = Yushan_ConvertTo16p16FP((uint16_t)(bTx_PixelWidth/sInitStruct->bNumberOfLanes)); */
	/* Above the IdpClk_div will be (2*0x10000) in case PixWidth is 10 and NoLanes is 4, since (PixWidth/NoLanes)=2, */
	/* not 2.5. Multiplying PixWidth by 0x10000 makes the result in fixed point without loosing on precesion post */
	/* decimal point. */
	fpIdpClk_div = (Yushan_ConvertTo16p16FP(bTx_PixelWidth)/sInitStruct->bNumberOfLanes);

	/******** DXO_Clk: Compute the lowest DxO clock matching constraints *********/

	/* Below may results in loss of precesion as division takes place before changing to fixed point. */
	/* fpPixel_Clk = Yushan_ConvertTo16p16FP((uint16_t)((sInitStruct->uwBitRate * sInitStruct->bNumberOfLanes)/bRx_PixelWidth)); */
	/* Result in fixed point notation and not loosen on precesion post decimal point. // Chk multplication not exceeding 32 bit max value. */
	fpPixel_Clk = ( (Yushan_ConvertTo16p16FP(sInitStruct->uwBitRate)*sInitStruct->bNumberOfLanes)/bRx_PixelWidth);
	fpIdp_Clk = Yushan_ConvertTo16p16FP((uint16_t)(fpTargetPll_Clk/fpIdpClk_div));

	/* FullLine and FullFrame */
	udwFullLine  = sInitStruct->uwActivePixels + sInitStruct->uwLineBlankStill;
	udwFullFrame = sInitStruct->uwLines + sInitStruct->uwFrameBlank;

	/* Compute the lower limit of DxO clock:
	*  (SpiClock/8)*30: There can be times, when SPIClk<8 (division may yield 0. So instead of
	*  using float value) first multiply it by 30 and then divide the result by 8. Fine as here
	*  SpiClk is in fixed point notation. */
	fpTempVar = Yushan_ConvertTo16p16FP((uint16_t)((sInitStruct->fpSpiClock*30)/0x80000));
	fpDxoClockLow = (fpTempVar >= fpIdp_Clk)?fpTempVar:fpIdp_Clk;

	if (fpDxoClockLow > DXO_CLK_LIMIT) {
		ERRORLOG("[CAM] %s:Lower limit of DxO clock is greater than higher limit, so EXITING the test\n", __func__);
		sSystemStatus->bDxoConstraints = DXO_LOLIMIT_EXCEED_HILIMIT;
		VERBOSELOG("[CAM] %s: End\n", __func__);
		return FAILURE;
	}

	/* Max index must be the index corresponding to PixelWidth. */
	bCount = ((bTx_PixelWidth == 8) ? 14 : 15);

	sSystemStatus->udwDxoConstraintsMinValue = 0;
	/* pick the highest divider for which all the Dxo constraints satisfies. */
	while (bCount >= 0) {
		fpDxoClk_Div = fpDividers[bCount];
		fpDxo_Clk = Yushan_ConvertTo16p16FP((uint16_t)(fpTargetPll_Clk / fpDxoClk_Div));

		if ((fpDxo_Clk >= fpDxoClockLow) && (fpDxo_Clk <= DXO_CLK_LIMIT)) {
			/* Checking DXO constraints with LineBlanking, FullLine, ActiveFrameLength, FullFrame and FrameBalnking */
			fDXOStatus = ((fStatus = Yushan_CheckDxoConstraints(sInitStruct->uwLineBlankStill, DXO_MIN_LINEBLANKING, fpDxo_Clk, fpPixel_Clk, 1, &sSystemStatus->udwDxoConstraintsMinValue))==0)? DXO_LINEBLANK_ERR : DXO_NO_ERR;
			if(fDXOStatus!=DXO_NO_ERR) { bCount--; continue;}
			fDXOStatus = ((fStatus = Yushan_CheckDxoConstraints(udwFullLine, DXO_MIN_LINELENGTH, fpDxo_Clk, fpPixel_Clk, 1, &sSystemStatus->udwDxoConstraintsMinValue))==0)? DXO_FULLLINE_ERR: DXO_NO_ERR ;
			if(fDXOStatus!=DXO_NO_ERR) { bCount--; continue;}
			/* From next three checks, udwFullLine avoided to make computation easier. */
			/* Checking constraints in terms of LineCount not in terms of Clks. */
			fDXOStatus = ((fStatus = Yushan_CheckDxoConstraints(sInitStruct->uwLines, ((DXO_ACTIVE_FRAMELENGTH/udwFullLine)+1), fpDxo_Clk, fpPixel_Clk, 1, &sSystemStatus->udwDxoConstraintsMinValue))==0)? DXO_ACTIVE_FRAMELENGTH_ERR: DXO_NO_ERR;
			if(fDXOStatus!=DXO_NO_ERR) { bCount--; continue;}
			fDXOStatus = ((fStatus = Yushan_CheckDxoConstraints(udwFullFrame, ((DXO_FULL_FRAMELENGTH/udwFullLine)+1), fpDxo_Clk, fpPixel_Clk, 1, &sSystemStatus->udwDxoConstraintsMinValue))==0)? DXO_FULLFRAMELENGTH_ERR: DXO_NO_ERR;
			if(fDXOStatus!=DXO_NO_ERR) { bCount--; continue;}

			/* udwFrameBlank = ((8 * udwFullLine) * (fpDxo_Clk/fpPixel_Clk));
			*  The below sequence is imp, as in absence of float, DxoClk/PixClk may yield 0, instead of some meaningful values.
			*  so multiply DxoClk by 8 first and then divide it by PixClk. Avoiding FullLine to make checks easier. */
			udwFrameBlank = ((8*fpDxo_Clk)/fpPixel_Clk);						
			if (sInitStruct->bDxoSettingCmdPerFrame == 1)
				udwFrameBlank += (135000/udwFullLine)+1;   /*(35000 + 100000);		For 1 SettingsPerFrame. */
			if (sInitStruct->bDxoSettingCmdPerFrame > 1)
				udwFrameBlank += (220000/udwFullLine)+1;   /*(35000 + 185000);		For more than 1 SettingsPerFrame.*/

			fDXOStatus = ((fStatus = Yushan_CheckDxoConstraints(sInitStruct->uwFrameBlank, udwFrameBlank, fpDxo_Clk, fpPixel_Clk, 1, &sSystemStatus->udwDxoConstraintsMinValue))==0)? DXO_FRAMEBLANKING_ERR: DXO_NO_ERR;
			if(fDXOStatus!=DXO_NO_ERR) { bCount--; continue;}

			/* For the current divider, no error occured, so loop can be break here. */
			break;

        	}																															
		
		/* This is required when Dxo_Clk does not lie between low and high limit.
		*  and also Dxo constraints have not yet checked. */
		bCount--;
	}

	/* Report DXO Status. */
	sSystemStatus->bDxoConstraints = fDXOStatus;

	/* If any of the DXO_CLK_DIV does not satisfy constraints: */
	if (bCount < 0) {
		ERRORLOG("[CAM] %s:DXO Dividers exhausted and returning FAILURE\n", __func__);
		VERBOSELOG("[CAM] %s: End\n", __func__);
		return FAILURE;
	}

	/* Compute the highest system clock. Finding the lowest divider for which
	*  system clock is just below the system clk limit. */
	for (bCount = 0; bCount < 10; bCount++) {
		fpSysClk_Div = fpDividers[bCount];
		fpSys_Clk = Yushan_ConvertTo16p16FP((uint16_t)(fpTargetPll_Clk / fpSysClk_Div));
		if (fpSys_Clk <= SYS_CLK_LIMIT)
			break; /* Got the desired SysClk_Div. */
	}


	fStatus = SUCCESS;

	/* Enabling Clks */
	bSpiData = 0x7F;
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL, 1, (uint8_t *)(&bSpiData)); /* Tx, Rx, Dxo, Idp clks enabled. */

	/* DXO out of Reset */
	/* Togling the resets */
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

	/* Enable/Disable Interrupts after resetting the IPs */
	Yushan_Intr_Enable((uint8_t *)udwIntrMask);

	/* LDO Initialization */
	fStatus &= Yushan_Init_LDO(sInitStruct->bUseExternalLDO);
	if (!fStatus) {
		ERRORLOG("[CAM] %s:LDO setup FAILED\n", __func__);
		VERBOSELOG("[CAM] %s: End\n", __func__);
		return FAILURE;
	} else {
		DEBUGLOG("[CAM] %s:LDO setup done.\n", __func__);
	}

	/* Writing DXO, SYS and IDP Dividers to their respective regsiters */
	/* Converting fixed point to non fixed point */
	bSpiData = (uint8_t)(fpDxoClk_Div>>16);
	bSpiData = bSpiData << 1;
	if ((fpDxoClk_Div&0x8000)==0x8000)				/* Decimal point (0x8000fp = 0.5floating) */
		bSpiData |= 0x01;
	fStatus &= SPI_Write(YUSHAN_CLK_DIV_FACTOR,   1, (uint8_t *)(&bSpiData));
	bSpiData = (uint8_t)(fpIdpClk_div>>16);
	bSpiData = bSpiData << 1;
	if ((fpIdpClk_div&0x8000)==0x8000)				/* Decimal point (0x8000fp = 0.5floating) */
		bSpiData |= 0x01;
	fStatus &= SPI_Write(YUSHAN_CLK_DIV_FACTOR+1, 1, (uint8_t *) (&bSpiData));
	bSpiData = (uint8_t)(fpSysClk_Div>>16);
	bSpiData = bSpiData << 1;
	if ((fpSysClk_Div&0x8000)==0x8000)				/* Decimal point (0x8000fp = 0.5floating) */
		bSpiData |= 0x01;
	fStatus &= SPI_Write(YUSHAN_CLK_DIV_FACTOR + 2, 1, (uint8_t *) (&bSpiData));

	/* Enable Pll */
	bSpiData = 0x18;
	fStatus &= SPI_Write(YUSHAN_PLL_CTRL_MAIN, 1, (uint8_t *)(&bSpiData));	/* Pll in functional state keeping earlier config intact */

	if (!fStatus) {
		ERRORLOG("[CAM] %s: End with Failure at enabling PLLs\n", __func__);
		return FAILURE;
	}

	DEBUGLOG("[CAM] %s:Waiting for YUSHAN_PLL_CTRL_MAIN interrupt Starts here\n", __func__);
	/* After programming all the clocks and enabling the PLL and its parameters,
	*  wait till Pll gets lock/stable. */
	fStatus &= Yushan_WaitForInterruptEvent(EVENT_PLL_STABLE, TIME_20MS);

	DEBUGLOG("[CAM] %s:YUSHAN_PLL_CTRL_MAIN interrupt received\n", __func__);

	if (!fStatus) {
		ERRORLOG("[CAM] %s: Failure at YUSHAN_PLL_CTRL_MAIN interrupt non-received Exiting...\n", __func__);
		return FAILURE;
	}

	/* For glitch free switching, first disable the clocks */
	/* Saving the context/data of the Clk_Ctrl */
	udwSpiData = 0;
	SPI_Read(YUSHAN_CLK_CTRL,   4, (uint8_t*)(&udwSpiData));

	/* Disable the clocks */
	bSpiData = 0x00;
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL, 1, (uint8_t*)(&bSpiData));
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+1, 1, (uint8_t*)(&bSpiData));
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+2, 1, (uint8_t*)(&bSpiData));		
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+3, 1, (uint8_t*)(&bSpiData));		


	/* Switch Sys clk to Pll clk from Host clk */
	bSpiData = 0x01; /* Clk_Sys_Sel set to 1 */
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL + 1, 1, (uint8_t *)(&bSpiData));

	/* Enable the clocks */
	udwSpiData &= 0xFDFF; /* Keeping Bypass Pll Off (0) */
	udwSpiData |= 0x0100; /* Keeping Sys_Sel On (1) */
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL, 4, (uint8_t *)(&udwSpiData));

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;


}



/*****************************************************************
API Internal Function:	Convert to 16 point 16 fixed point notation
Input:					Unsigned 16 bit integer.
******************************************************************/
uint32_t Yushan_ConvertTo16p16FP(uint16_t uwNonFPValue)
{

	return (((uint32_t)(uwNonFPValue)) << 16);

}




/************************************************************
API Function: Upload DXO micro code and perform the boot operation.
Input:		  DXO_InitStruct
*************************************************************/
bool_t	Yushan_Init_Dxo(Yushan_Init_Dxo_Struct_t *sDxoStruct, bool_t fBypassDxoUpload)
{

	uint8_t bSpiData;
	uint32_t	udwHWnMicroCodeID = 0; /*, udwCalibrationDataID = 0, udwErrorCode = 0;*/
	uint32_t udwDxoBaseAddress = 0, udwSpiBaseIndex = 0;
	bool_t	fStatus = 1;
	/* uint8_t readback=0; */

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	/******************************* DXO DOP RAM ***************************************************/

	udwSpiBaseIndex = 0x08000;
	fStatus=SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

	udwDxoBaseAddress=DXO_DOP_BASE_ADDR;

	if (!fBypassDxoUpload) {
		DEBUGLOG("[CAM] load DxO DOP firmware\n");
		fStatus &= SPI_Write_4thByte((uint16_t)(sDxoStruct->uwBaseAddrDopMicroCode[0]+udwDxoBaseAddress), sDxoStruct->uwDxoDopRamImageSize[0], sDxoStruct->pDxoDopRamImage[0]);
		fStatus &= SPI_Write_4thByte((uint16_t)(sDxoStruct->uwBaseAddrDopMicroCode[1]+udwDxoBaseAddress), sDxoStruct->uwDxoDopRamImageSize[1], sDxoStruct->pDxoDopRamImage[1]);
	}
	/* Boot Sequence */
	/* As per the scenerio files, first program the start address of the code */
	fStatus &= SPI_Write((uint16_t)(sDxoStruct->uwDxoDopBootAddr + udwDxoBaseAddress), 2,  (uint8_t*)(&sDxoStruct->uwDxoDopStartAddr));
	/* Give Boot Command and Wait for the EOB interrupt. */
	bSpiData = DXO_BOOT_CMD;
	fStatus &= SPI_Write((uint16_t)(DxODOP_boot + udwDxoBaseAddress), 1,  (uint8_t*)(&bSpiData));

	/******************************* DXO DPP RAM *****************************************************/
	udwSpiBaseIndex = 0x10000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

	/* NewAddr = (ActualAddress - udwSpiBaseIndex) + 0x8000:  0x8000 is the SPIWindow_1 base address.
	*  ActualAddress = DXO_XXX_BASE_ADDR + Offset;
	*  when NewAddr, is in window (0x8000-> 0xFFFF), then SPI slave will check the SPI base address
	*  to know the actual physical address to access. */
	udwDxoBaseAddress=(0x8000 + DXO_DPP_BASE_ADDR) - udwSpiBaseIndex; /* OR ## DXO_DPP_BASE_ADDR-0x8000; */
	/* Loading Microcode, setting address for boot and Boot command. Then Verifying the DPP IP */
	if (!fBypassDxoUpload) {
		DEBUGLOG("[CAM] load DxO DPP firmware\n");
		fStatus &= SPI_Write_4thByte((uint16_t)(sDxoStruct->uwBaseAddrDppMicroCode[0]+ udwDxoBaseAddress), sDxoStruct->uwDxoDppRamImageSize[0], sDxoStruct->pDxoDppRamImage[0]);
		/* Setting base address to the DPP calibration data base address. */
		udwSpiBaseIndex = DXO_DPP_BASE_ADDR + sDxoStruct->uwBaseAddrDppMicroCode[1]; /* 0x018000; */
		SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));
		udwDxoBaseAddress = ((DXO_DPP_BASE_ADDR + sDxoStruct->uwBaseAddrDppMicroCode[1]) - udwSpiBaseIndex) + 0x8000; /*(0x8000 + DXO_DPP_BASE_ADDR) - udwSpiBaseIndex;*/

		fStatus &= SPI_Write_4thByte((uint16_t)(udwDxoBaseAddress), sDxoStruct->uwDxoDppRamImageSize[1], sDxoStruct->pDxoDppRamImage[1]);
	}
	/* Since " SET_ADDR_DPP_BOOT + DXO_DPP_BASE_ADDR " exceed 0x018000, changing the access base address */
	udwSpiBaseIndex = 0x18000;
	udwDxoBaseAddress = 0x8000; /*(0x8000 + DXO_DPP_BASE_ADDR) - udwSpiBaseIndex;*/ /* OR DXO_DPP_BASE_ADDR-0x10000; */
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

	/* Boot Sequence */
	/* As per the scenerio files, first program the start address of the code */
	fStatus &= SPI_Write((uint16_t)((DXO_DPP_BASE_ADDR + sDxoStruct->uwDxoDppBootAddr) - udwSpiBaseIndex + udwDxoBaseAddress), 2,  (uint8_t*)(&sDxoStruct->uwDxoDppStartAddr));
	/* Give Boot Command and Wait for the EOB interrupt. */
	bSpiData = DXO_BOOT_CMD;
	fStatus &= SPI_Write(((uint16_t)(DXO_DPP_BASE_ADDR + DxODPP_boot) - udwSpiBaseIndex + udwDxoBaseAddress), 1,  (uint8_t*)(&bSpiData));

	/* Restoring the SpiBaseAddress to 0x08000. */
	udwSpiBaseIndex = 0x8000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

	/*********************************************** DXO PDP RAM ************************************/
	/* Loading Microcode, setting address for boot and Boot command. Then Verifying the PDP IP */
	if (!fBypassDxoUpload) {
		DEBUGLOG("[CAM] load DxO PDP firmware\n");
		fStatus &= SPI_Write_4thByte((uint16_t)(sDxoStruct->uwBaseAddrPdpMicroCode[0]+ DXO_PDP_BASE_ADDR), sDxoStruct->uwDxoPdpRamImageSize[0], sDxoStruct->pDxoPdpRamImage[0]);
		fStatus &= SPI_Write_4thByte((uint16_t)(sDxoStruct->uwBaseAddrPdpMicroCode[1]+ DXO_PDP_BASE_ADDR), sDxoStruct->uwDxoPdpRamImageSize[1], sDxoStruct->pDxoPdpRamImage[1]);
  	}
	/* Boot Sequence */
	/* As per the scenerio files, first program the start address of the code */
	fStatus &= SPI_Write((uint16_t)(sDxoStruct->uwDxoPdpBootAddr + DXO_PDP_BASE_ADDR), 2,  (uint8_t*)(&sDxoStruct->uwDxoPdpStartAddr));

	/* Give Boot Command and Wait for the EOB interrupt. */
	bSpiData = DXO_BOOT_CMD;
	fStatus &= SPI_Write((uint16_t)(DxOPDP_boot + DXO_PDP_BASE_ADDR), 1,  (uint8_t*)(&bSpiData));

	DEBUGLOG("[CAM] %s:Waiting for EVENT_DOP7_BOOT interrupt Starts here\n", __func__);
	/* Wait for the End of Boot interrupts. This indicates the completion of Boot command */
	fStatus &= Yushan_WaitForInterruptEvent2 (EVENT_DOP7_BOOT, TIME_10MS);
	if (!fStatus) {
		ERRORLOG("[CAM] %s: EVENT_DOP7_BOOT not received. Exiting ...\n", __func__);
		return fStatus;
	}
	DEBUGLOG("[CAM] %s:DOP7 IP Booted\n", __func__);

	/* Verifying DXO_XXX HW, Micro code and Calibration data IDs. */

	/* Verifying DOP Ids */	
	SPI_Read((DxODOP_ucode_id_7_0 + DXO_DOP_BASE_ADDR) , 4, (uint8_t *)(&udwHWnMicroCodeID));
#if 0
	SPI_Read((DxODOP_calib_id_0_7_0 + DXO_DOP_BASE_ADDR), 4, (uint8_t *)(&udwCalibrationDataID));
	SPI_Read((DxODOP_error_code_7_0 + DXO_DOP_BASE_ADDR), 1, (uint8_t *)(&udwErrorCode));
	DEBUGLOG("DXO DOP udwHWnMicroCodeID:%x;udwCalibrationDataID:%x;udwErrorCode:%x\n",
		udwHWnMicroCodeID, udwCalibrationDataID, udwErrorCode);
#endif

	udwHWnMicroCodeID &= 0xFFFFFF00;	// Not checking the ucode minor version
	if (udwHWnMicroCodeID != DXO_DOP_HW_MICROCODE_ID) {
		ERRORLOG("[CAM] %s: Error with DOP microcode check. Exiting ...\n", __func__);
		return FAILURE;
	}

	DEBUGLOG("[CAM] %s:Waiting for EVENT_DPP_BOOT interrupt Starts here\n", __func__);
	/* Wait for the End of Boot interrupts. This indicates the completion of Boot command */
	fStatus &= Yushan_WaitForInterruptEvent (EVENT_DPP_BOOT, TIME_10MS);
	if (!fStatus) {
		ERRORLOG("[CAM] %s: EVENT_DPP_BOOT not received. Exiting ...\n", __func__);
		return fStatus;
	}
	DEBUGLOG("[CAM] %s:DPP IP Booted\n", __func__);

	/* Verifying DPP Ids */
	udwSpiBaseIndex = 0x010000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

	SPI_Read(DxODPP_ucode_id_7_0+ DXO_DPP_BASE_ADDR-0x8000, 4, (uint8_t *)(&udwHWnMicroCodeID));
#if 0
	SPI_Read(DxODPP_calib_id_0_7_0+DXO_DPP_BASE_ADDR-0x8000, 4, (uint8_t *)(&udwCalibrationDataID));
	SPI_Read(DxOPDP_error_code_7_0+DXO_DPP_BASE_ADDR-0x8000, 1, (uint8_t *)(&udwErrorCode));
	DEBUGLOG("DXO DPP udwHWnMicroCodeID:%x;udwCalibrationDataID:%x;udwErrorCode:%x\n",
		udwHWnMicroCodeID, udwCalibrationDataID, udwErrorCode);
#endif

	udwHWnMicroCodeID &= 0xFFFFFF00;	/* Not checking the ucode minor version */
	if (udwHWnMicroCodeID != DXO_DPP_HW_MICROCODE_ID) {
		ERRORLOG("[CAM] %s: Error with DPP microcode check. Exiting ...\n", __func__);
		return FAILURE;
	}
	/* Restoring the SpiBaseAddress to 0x08000. */
	udwSpiBaseIndex = 0x08000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (uint8_t *)(&udwSpiBaseIndex));

	DEBUGLOG("[CAM] %s:Waiting for EVENT_PDP_BOOT interrupt Starts here\n", __func__);
	/* Wait for the End of Boot interrupts. This indicates the completion of Boot command */
	fStatus &= Yushan_WaitForInterruptEvent (EVENT_PDP_BOOT, TIME_10MS);

	if (!fStatus) {
		ERRORLOG("[CAM] %s: EVENT_PDP_BOOT not received. Exiting ...\n", __func__);
		return fStatus;
	}
	DEBUGLOG("[CAM] %s:PDP IP Booted\n", __func__);

	/* Verifying PDP Ids */
	SPI_Read((DxOPDP_ucode_id_7_0 + DXO_PDP_BASE_ADDR) , 4, (uint8_t *)(&udwHWnMicroCodeID));
#if 0
	SPI_Read((DxOPDP_calib_id_0_7_0 + DXO_PDP_BASE_ADDR), 4, (uint8_t *)(&udwCalibrationDataID));
	SPI_Read((DxOPDP_error_code_7_0 + DXO_PDP_BASE_ADDR), 1, (uint8_t *)(&udwErrorCode));
	DEBUGLOG("DXO PDP udwHWnMicroCodeID:%x;udwCalibrationDataID:%x;udwErrorCode:%x\n",
		udwHWnMicroCodeID, udwCalibrationDataID, udwErrorCode);
#endif

	udwHWnMicroCodeID &= 0xFFFFFF00; /* Not checking the ucode minor version */
	if (udwHWnMicroCodeID != DXO_PDP_HW_MICROCODE_ID) {
		ERRORLOG("[CAM] %s: Error with PDP microcode check. Exiting ...\n", __func__);
		return FAILURE;
	}

	VERBOSELOG("[CAM] %s: End with Success\n", __func__);
	return SUCCESS;
}


/************************************************************************************
API Function:	Init Yushan, perform the basic initialization
				of Yushan.
Input:			InitStruct
Return:
*************************************************************************************/
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
	uint8_t		bDXODecimalFactor = 0, bIdpDecimalFactor = 0; /* Yushan API 10.0*/
	uint8_t		bUsedDataType = 0;

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	/* Writing the base address  in order to access full 64KB */
	spi_base_address = 0x8000;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS, 4, (unsigned char *)&spi_base_address);

	udwSpiData = 0x1;
	SPI_Write(YUSHAN_IOR_NVM_CTRL, 1, (unsigned char *)&udwSpiData); /* NVM Control */

	/* MIPI  Initialization */
	udwSpiData = 4000/sInitStruct->uwBitRate;
	SPI_Write(YUSHAN_MIPI_TX_UIX4, 1, (unsigned char *)&udwSpiData);
	SPI_Write(YUSHAN_MIPI_RX_UIX4, 1, (unsigned char *)&udwSpiData);
	udwSpiData = 1;
	SPI_Write(YUSHAN_MIPI_RX_COMP, 1, (unsigned char*)&udwSpiData);

	if (sInitStruct->bNumberOfLanes == 1)
		udwSpiData = 0x11; /* Enabling Clock Lane and Data Lane1 */
	else if (sInitStruct->bNumberOfLanes == 2)
		udwSpiData = 0x31; /* Enabling Clock Lane and Data Lane1 */
	else if (sInitStruct->bNumberOfLanes == 4)
		udwSpiData = 0xf1; /* Enabling Clock Lane and Data Lane1 */

	/* Exempting tx_request_hs_cl in MIPI_TX_EN */
	SPI_Write(YUSHAN_MIPI_TX_ENABLE,1,(unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_MIPI_RX_ENABLE,1,(unsigned char*)&udwSpiData); 
	/* Adding delay (ideal: 3Microsec) between TX_Enable and setting of tx_request_hs_cl. */
	/* Re writing MIPI UIX4 */
	udwSpiData=4000/sInitStruct->uwBitRate;
	SPI_Write(YUSHAN_MIPI_TX_UIX4,1,(unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_MIPI_RX_UIX4,1,(unsigned char*)&udwSpiData); 

	/* Reading from RX_ENABLE */
	SPI_Read(YUSHAN_MIPI_RX_ENABLE,1,(unsigned char*)&udwSpiData); 
	udwSpiData |= 0x02; /* Enable Clk HS */
	SPI_Write(YUSHAN_MIPI_TX_ENABLE,1,(unsigned char*)&udwSpiData);

	/* CSI2Rx Initialization */

	udwSpiData=sInitStruct->bNumberOfLanes;
	SPI_Write(YUSHAN_CSI2_RX_NB_DATA_LANES,1,(unsigned char*)&udwSpiData); 

	udwSpiData=sInitStruct->uwPixelFormat & 0x0f;
	SPI_Write(YUSHAN_CSI2_RX_IMG_UNPACKING_FORMAT,1,(unsigned char*)&udwSpiData); 

	udwSpiData = 4; /* 32; - tmp workaround for s5k3h2yx video size */
	SPI_Write(YUSHAN_CSI2_RX_BYTE2PIXEL_READ_TH,1,(unsigned char*)&udwSpiData); 

	/* SMIA Formatter Initialization */

	udwSpiData=sInitStruct->uwPixelFormat & 0x0f;
	SPI_Write(YUSHAN_SMIA_FM_PIX_WIDTH,1,(unsigned char*)&udwSpiData); 

	udwSpiData=0;
	SPI_Write(YUSHAN_SMIA_FM_GROUPED_PARAMETER_HOLD,1,(unsigned char*)&udwSpiData); 
	udwSpiData=0x19;
	SPI_Write(YUSHAN_SMIA_FM_CTRL,1,(unsigned char*)&udwSpiData); 
	/* Disabling the SMIA_FM_EOF_INT_EN */
	udwSpiData = 0;
	SPI_Write(YUSHAN_SMIA_FM_EOF_INT_EN, 1, (unsigned char*)&udwSpiData);	
	/* udwSpiData=0x3; */
	/* SPI_Write(YUSHAN_SMIA_FM_EOF_INT_CTRL,1,(unsigned char*)&udwSpiData); */

	/* CSITX Wrapper  */
	udwSpiData=((((sInitStruct->uwPixelFormat & 0x0f) * bSofEofLength )/32 )<<8)|0x01;
	SPI_Write(YUSHAN_CSI2_TX_WRAPPER_THRESH,2,(unsigned char*)&udwSpiData); 

	/* EOF Resize PreDxo */

	udwSpiData=1;
	SPI_Write(YUSHAN_EOF_RESIZE_PRE_DXO_AUTOMATIC_CONTROL,1,(unsigned char*)&udwSpiData); 
	SPI_Write(YUSHAN_EOF_RESIZE_PRE_DXO_ENABLE,1,(unsigned char*)&udwSpiData); 

	/* EOF Resize PostDxo */
	udwSpiData=0; /* Manual Mode */
	SPI_Write(YUSHAN_EOF_RESIZE_POST_DXO_AUTOMATIC_CONTROL,1,(unsigned char*)&udwSpiData); 
	udwSpiData= bSofEofLength;
	SPI_Write(YUSHAN_EOF_RESIZE_POST_DXO_H_SIZE,1,(unsigned char*)&udwSpiData); 
	udwSpiData=1;
	SPI_Write(YUSHAN_EOF_RESIZE_POST_DXO_ENABLE,1,(unsigned char*)&udwSpiData); 

	SPI_Read(YUSHAN_CLK_DIV_FACTOR,1,(unsigned char*)&bDxoClkDiv); 
	SPI_Read(YUSHAN_CLK_DIV_FACTOR+1,1,(unsigned char*)&bPixClkDiv); 
	/* uwHsizeStill= (sImageChar->uwXAddrEnd - sImageChar->uwXAddrStart+1)/sImageChar->uwXOddInc; */
	/* uwHsizeVf= (sImageChar->uwXAddrEnd - sImageChar->uwXAddrStart+1)/sImageChar->uwXOddInc; */

	/* Idp Gen */

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
			udwSpiData= (0)  | ( bUsedDataType << 16) | ((1)<<24);
			SPI_Write((uint16_t)(YUSHAN_IDP_GEN_WC_DI_0+4*bCount),4,(unsigned char*)&udwSpiData);
		}
	}

	udwSpiData=bVfIndex | ( bStillIndex << 4 ) | (bSofEofLength << 8)|0x3ff0000; /********* Check ****************/
	SPI_Write(YUSHAN_IDP_GEN_CONTROL,4,(unsigned char*)&udwSpiData);
	udwSpiData=0xc810;
	SPI_Write(YUSHAN_IDP_GEN_ERROR_LINES_EOF_GAP,2,(unsigned char*)&udwSpiData);

	/* CSI2TX */  

	udwSpiData=sInitStruct->bNumberOfLanes-1;
	SPI_Write(YUSHAN_CSI2_TX_NUMBER_OF_LANES,1,(unsigned char*)&udwSpiData); 

	udwSpiData=1;
	SPI_Write(YUSHAN_CSI2_TX_PACKET_CONTROL,1,(unsigned char*)&udwSpiData); 
	udwSpiData=1;
	SPI_Write(YUSHAN_CSI2_TX_ENABLE,1,(unsigned char*)&udwSpiData); 

	/* DT Filter */
	/***** DT Filter 0 ******/
	udwSpiData=0x1;
	SPI_Write(YUSHAN_DTFILTER_BYPASS_MATCH0,1,(unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_DTFILTER_BYPASS_MATCH1,1,(unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_DTFILTER_BYPASS_MATCH2,1,(unsigned char*)&udwSpiData);
	udwSpiData=0x3;
	SPI_Write(YUSHAN_DTFILTER_BYPASS_MATCH3,1,(unsigned char*)&udwSpiData);
	udwSpiData=0x01;
	SPI_Write(YUSHAN_DTFILTER_BYPASS_ENABLE,1,(unsigned char*)&udwSpiData);

	/***** DT Filter 1 ******/
	udwSpiData=0xd;
	SPI_Write(YUSHAN_DTFILTER_DXO_MATCH0,1,(unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_DTFILTER_DXO_MATCH2,1,(unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_DTFILTER_DXO_MATCH3,1,(unsigned char*)&udwSpiData);
	udwSpiData=0x02;
	SPI_Write(YUSHAN_DTFILTER_DXO_MATCH1,1,(unsigned char*)&udwSpiData);
	udwSpiData=0x01;
	SPI_Write(YUSHAN_DTFILTER_DXO_ENABLE,1,(unsigned char*)&udwSpiData);

	/* T1 DMA programming */

	/* For PDP */
	udwSpiData=(uwHsizeStill&0xff) | ((uwHsizeVf&0xff) << 8 );
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT,2,(unsigned char*)&udwSpiData);
	/* udwSpiData=0x6205; */
	udwSpiData=DXO_PDP_BASE_ADDR+DxOPDP_visible_line_size_7_0;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=((uwHsizeStill>>8)&0xff) | (((uwHsizeVf>>8)&0xff) << 8 );
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+8,2,(unsigned char*)&udwSpiData);
	/* udwSpiData=0x6206; */
	udwSpiData=DXO_PDP_BASE_ADDR + DxOPDP_visible_line_size_15_8;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+8+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=0x0101;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+16,2,(unsigned char*)&udwSpiData);
	/* udwSpiData=0x7a0c; */
	udwSpiData=DXO_PDP_BASE_ADDR + DxOPDP_newFrameCmd;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+16+2,3,(unsigned char*)&udwSpiData);

	/* For DPP */
	udwSpiData=(uwHsizeStill&0xff) | ((uwHsizeVf&0xff) << 8 );
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+24,2,(unsigned char*)&udwSpiData);
	/* udwSpiData=0x10205; */
	udwSpiData=DXO_DPP_BASE_ADDR + DxODPP_visible_line_size_7_0;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+24+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=((uwHsizeStill>>8)&0xff) | (((uwHsizeVf>>8)&0xff) << 8 );
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+32,2,(unsigned char*)&udwSpiData);
	/* udwSpiData=0x10206; */
	udwSpiData=DXO_DPP_BASE_ADDR + DxODPP_visible_line_size_15_8;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+32+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=0x0101;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+40,2,(unsigned char*)&udwSpiData);
	/* udwSpiData=0x1d00c; */
	udwSpiData=DXO_DPP_BASE_ADDR + DxODPP_newFrameCmd;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+40+2,3,(unsigned char*)&udwSpiData);

	/* For DOP */
	udwSpiData=(uwHsizeStill&0xff) | ((uwHsizeVf&0xff) << 8);
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+48,2,(unsigned char*)&udwSpiData);
	/* udwSpiData=0x8205; */
	udwSpiData=DXO_DOP_BASE_ADDR + DxODOP_visible_line_size_7_0;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+48+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=((uwHsizeStill>>8)&0xff) | (((uwHsizeVf>>8)&0xff) << 8);
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+56,2,(unsigned char*)&udwSpiData);
	/* udwSpiData=0x8206; */
	udwSpiData=DXO_DOP_BASE_ADDR + DxODOP_visible_line_size_15_8;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+56+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=0x0101;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+64,2,(unsigned char*)&udwSpiData);
	/* udwSpiData=0xe80c; */
	udwSpiData=DXO_DOP_BASE_ADDR + DxODOP_newFrameCmd;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+64+2,3,(unsigned char*)&udwSpiData);


	/* For LBE PreDxo */
	udwSpiData=(uwHsizeStill&0xff) | ((uwHsizeVf&0xff) << 8);
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+72,2,(unsigned char*)&udwSpiData);
	udwSpiData=YUSHAN_LBE_PRE_DXO_H_SIZE;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+72+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=((uwHsizeStill>>8)&0xff) | (((uwHsizeVf>>8)&0xff) << 8);
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+80,2,(unsigned char*)&udwSpiData);
	udwSpiData=YUSHAN_LBE_PRE_DXO_H_SIZE+1;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+80+2,3,(unsigned char*)&udwSpiData);

	/* For LBE PostDxo */
	udwSpiData=(uwHsizeStill&0xff) | ((uwHsizeVf&0xff) << 8);
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+88,2,(unsigned char*)&udwSpiData);
	udwSpiData=YUSHAN_LBE_POST_DXO_H_SIZE;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+88+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=((uwHsizeStill>>8)&0xff) | (((uwHsizeVf>>8)&0xff) << 8);
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+96,2,(unsigned char*)&udwSpiData);
	udwSpiData=YUSHAN_LBE_POST_DXO_H_SIZE+1;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+96+2,3,(unsigned char*)&udwSpiData);

	/* For Lecci */
	udwSpiData=(uwHsizeStill&0xff) | ((uwHsizeVf&0xff) << 8);
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+104,2,(unsigned char*)&udwSpiData);
	udwSpiData=YUSHAN_LECCI_LINE_SIZE;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+104+2,3,(unsigned char*)&udwSpiData);
	udwSpiData=((uwHsizeStill>>8)&0xff) | (((uwHsizeVf>>8)&0xff) << 8);
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+112,2,(unsigned char*)&udwSpiData);
	udwSpiData=YUSHAN_LECCI_LINE_SIZE+1;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+112+2,3,(unsigned char*)&udwSpiData);

	/***** Lecci prog ******/
	/* Min_interline = Freq_DXO/Freq_Pix*interline. */

	if (bDxoClkDiv != 0 && bPixClkDiv != 0) {
		uwLecciVf    = sInitStruct->uwLineBlankVf*bPixClkDiv/ bDxoClkDiv;
		uwLecciStill = sInitStruct->uwLineBlankStill*bPixClkDiv/ bDxoClkDiv;
	} else {
		uwLecciVf    = 300;
		uwLecciStill = 300;
	}

	/* For Lecci */
	udwSpiData=(uwLecciStill&0xff) | ((uwLecciVf&0xff) << 8 );
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+120,2,(unsigned char*)&udwSpiData); 
	udwSpiData=YUSHAN_LECCI_MIN_INTERLINE;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+120+2,3,(unsigned char*)&udwSpiData); 
	udwSpiData=((uwLecciStill>>8)&0xff) | (((uwLecciVf>>8)&0xff) << 8 );
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+128,2,(unsigned char*)&udwSpiData); 
	udwSpiData=YUSHAN_LECCI_MIN_INTERLINE+1;
	SPI_Write(YUSHAN_T1_DMA_MEM_LOWER_ELT+128+2,3,(unsigned char*)&udwSpiData);

 
	udwSpiData=17; /* 9 enteried in T1 DMA memory */
	SPI_Write(YUSHAN_T1_DMA_REG_REFILL_ELT_NB,1,(unsigned char*)&udwSpiData); 
	udwSpiData=1;
	SPI_Write(YUSHAN_T1_DMA_REG_ENABLE,1,(unsigned char*)&udwSpiData); 

	/* LBE PreDxo */
	/*************************CHECK*********************/

	udwSpiData=4;  /*****************Check*****************/
	SPI_Write(YUSHAN_LBE_PRE_DXO_READ_START,1,(unsigned char*)&udwSpiData); 
	udwSpiData=1;  /*****************Check*****************/
	SPI_Write(YUSHAN_LBE_PRE_DXO_ENABLE,1,(unsigned char*)&udwSpiData); 

	/* LBE Post Dxo */
	/*************************CHECK*********************/
	udwSpiData=80;  /*****************Check*****************/
	SPI_Write(YUSHAN_LBE_POST_DXO_READ_START,1,(unsigned char*)&udwSpiData);
	udwSpiData=1;  /*****************Check*****************/
	SPI_Write(YUSHAN_LBE_POST_DXO_ENABLE,1,(unsigned char*)&udwSpiData);

	/***** Lecci prog ******/
	/* Ref_target = (bDxoClkDiv*2) */
	/* Window_size = if(dxoclk_div!=pixclk_div) (PixClkDiv*2 - 1) else 1. */
	bDXODecimalFactor = bIdpDecimalFactor = 0;

	if(((bDxoClkDiv&0x01) == 1))		/* Checking for Decimal point */
		bDXODecimalFactor = 1;		/* It is (2*0.5). In case of WindowSize/Ref_Traget.. (2*ClkDiv) */
	if(((bPixClkDiv&0x01) == 1))		/* Checking for Decimal point */
		bIdpDecimalFactor = 1;

	bDxoClkDiv = bDxoClkDiv>>1;		/* Integral part of divider */
	bPixClkDiv = bPixClkDiv>>1;		/* Integral part of divider */

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


/*************************************************************
API Functtion: Update DXO image characteristics parameters.
Same for all the the three IPs
Input: ImageStruct
**************************************************************/
bool_t Yushan_Update_ImageChar(Yushan_ImageChar_t *sImageChar)
{
	uint8_t  *pData;
	bool_t fStatus;
	uint32_t udwSpiData;
	uint8_t  bData[20];

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	/* Assumption that the sturcture is packed and there is no padding. */
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

	/* Programming to the DXO IPs */
	fStatus = SPI_Write(DXO_PDP_BASE_ADDR+DxOPDP_image_orientation_7_0, 18, pData);
	bData[17]=0;
	fStatus = SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_image_orientation_7_0, 18, pData);
	udwSpiData = 0x10000 ;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);
	fStatus = SPI_Write(DXO_DPP_BASE_ADDR+DxODPP_image_orientation_7_0-0x8000, 18, pData);
	udwSpiData = 0x8000 ;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;

}



/*************************************************************
API Function: Update Sensor paramters. Same for all the the
three IPs.
Input: GainExpInfo
**************************************************************/
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
	
	/* Programming to the DXO IPs */
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



/***************************************************************************
API Function: Update Pdp Tuning paramter for Dead pixel correction.
***************************************************************************/
bool_t Yushan_Update_DxoPdp_TuningParameters(Yushan_DXO_PDP_Tuning_t *sDxoPdpTuning)
{
	uint8_t  *pData,fStatus;
	uint8_t bData[3];

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	pData = (uint8_t *)bData;
	bData[0]=sDxoPdpTuning->bDeadPixelCorrectionLowGain;
	bData[1]=sDxoPdpTuning->bDeadPixelCorrectionMedGain;
	bData[2]=sDxoPdpTuning->bDeadPixelCorrectionHiGain;

	/* Only three bytes need to be written. Fourth byte write will overwrite the */
	/* DxOPDP_frame_number_7_0 register */
	fStatus = SPI_Write(DXO_PDP_BASE_ADDR+DxOPDP_dead_pixels_correction_lowGain_7_0, 3, pData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;

}





/***************************************************************************
API Function: Update Dpp Tuning parameters
***************************************************************************/
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

	/* Restoring the SpiBaseAddress to 0x08000. */
  	udwSpiData = 0x8000 ;
  	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;

}


/***************************************************************************
API Function: Update Dop Tuning parameters
Input: DOP Tuning parameters
***************************************************************************/
bool_t Yushan_Update_DxoDop_TuningParameters(Yushan_DXO_DOP_Tuning_t *sDxoDopTuning)
{
	uint8_t *pData;
	bool_t fStatus;
	uint8_t bData[10];

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	pData = (uint8_t *) bData;

	bData[0]=sDxoDopTuning->bEstimationMode ;
	/* First five consecutive registers written. */
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




/***************************************************************************
API Function: Update_Commit. This function will apply the changes programmed
in the DXO parameters. This function will be called at the start of the frame
so that the changes will be well reflected in the coming frame.
***************************************************************************/
bool_t Yushan_Update_Commit(uint8_t  bPdpMode, uint8_t  bDppMode, uint8_t  bDopMode)
{
	uint8_t bData, *pData;
	bool_t fStatus = SUCCESS;
	uint32_t udwSpiData ;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	bData = 1;
	pData = &bData;

	/* pr_info("[CAM] %s:%d, %d, %d\n",__func__, bPdpMode, bDppMode, bDopMode); */

	/* Setting DXO mode */
	SPI_Write(DXO_PDP_BASE_ADDR+DxOPDP_mode_7_0, 1, &bPdpMode);
	/* Giving executing command, so that the programmed mode will be executed. */
	SPI_Write(DXO_PDP_BASE_ADDR+DxOPDP_execCmd, 1, pData);

	// Setting DXO mode */
	SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_mode_7_0, 1, &bDopMode);
	/* Giving executing command, so that the programmed mode will be executed. */
	SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_execCmd, 1, pData);

	/*  Setting SPI Window */
	udwSpiData = 0x10000 ;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);
	/* Setting DXO mode */
	SPI_Write(DXO_DPP_BASE_ADDR+DxODPP_mode_7_0-0x8000, 1, &bDppMode);
	udwSpiData = 0x18000 ;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);
	/* Giving executing command, so that the programmed mode will be executed. */
	SPI_Write(DXO_DPP_BASE_ADDR+DxODPP_execCmd-0x10000, 1, pData);


	/* Restoring the SpiBaseAddress to 0x08000. */
	udwSpiData = 0x8000 ;
	SPI_Write(YUSHAN_HOST_IF_SPI_BASE_ADDRESS,4,(uint8_t *)&udwSpiData);

#if 0
	DEBUGLOG("[CAM] %s:Waiting for EOF_EXECCMD interrupt Starts here\n", __func__);
	/* Wait for the EndOfExec interrupts. This indicates the completion of the */
	/* execution command */
	fStatus = Yushan_WaitForInterruptEvent(EVENT_PDP_EOF_EXECCMD, TIME_10MS);
	if (!fStatus) {
		ERRORLOG("[CAM] %s:Failed in EVENT_PDP_EOF_EXECCMD interrupt\n", __func__);
		return FAILURE;
	}
	DEBUGLOG("[CAM] %s:DXO PDP commited now\n", __func__);

	fStatus = Yushan_WaitForInterruptEvent2(EVENT_DOP7_EOF_EXECCMD, TIME_10MS);
	if (!fStatus) {
		ERRORLOG("[CAM] %s:Failed in EVENT_DOP7_EOF_EXECCMD interrupt\n", __func__);
		return FAILURE;
	}
	DEBUGLOG("[CAM] %s:DXO DOP7 commited now\n", __func__);

	fStatus = Yushan_WaitForInterruptEvent(EVENT_DPP_EOF_EXECCMD, TIME_10MS);
	if (!fStatus) {
		ERRORLOG("[CAM] %s:Failed in EVENT_DPP_EOF_EXECCMD interrupt\n", __func__);
		return FAILURE;
	}
#endif

	DEBUGLOG("[CAM] %s:DXO IPs commited now\n", __func__);
	VERBOSELOG("[CAM] %s: End\n", __func__);
	/* Return Success */
	return fStatus;

}



/******************************************************************
API Internal Function:	DXO Constraints check part of Clk_Init
Input:					DXO inputs, Clks and Min limits
Return:					SUCCESS OR FAILURE, if DXO constraints
						met or violated, respectively.
*******************************************************************/
bool_t Yushan_CheckDxoConstraints(uint32_t udwParameters, uint32_t udwMinLimit, uint32_t fpDxo_Clk, uint32_t fpPixel_Clk, uint16_t uwFullLine, uint32_t * pMinValue)
{
	/* fpDxo_Clk and fpPixel_Clk are in fixed point notations. */

	/* uint32_t	udwLocalParameters, udwLocalMinLimit, udwLocalFullLine; */
	uint16_t	uwLocalDxoClk, uwLocalPixelClk;
	uint32_t	udwTemp1, udwTemp2;

	/* Converting the params back to non fixed point format */
	uwLocalDxoClk = fpDxo_Clk>>16;
	uwLocalPixelClk = fpPixel_Clk>>16;

	/* In the absence of foating point arithmatic. Multiply numerator by some big value, lets 0xFFFF:
	*  so that divisions (below in comment) do not yield 0 for both the divisions.
	*  (parameter/pixelclk) < (MinLimit/DxoClk): */
	udwTemp1 = (udwParameters*0xFFFF)/uwLocalPixelClk;
	udwTemp2 = (udwMinLimit*0xFFFF)/uwLocalDxoClk;

	/* if((parameter/pixelclk) < (MinLimit/DxoClk)) is same as [(((parameter*0xFFFF)/pixelclk) < ((MinLimit*0xFFFF)/DxoClk))] */
	if (udwTemp1 < udwTemp2) {
		/* Dxo Constraints not met:
		*  return ceil(((MinLimit/DxoClk)*PixelClk)/FullLine); */
		
		/* Dividing again by 0xFFFF to get the actual value. */
		udwTemp1 = ((udwTemp2*uwLocalPixelClk)/0xFFFF); /* /uwFullLine); FullLine is 1 here. */
		*pMinValue = (udwTemp1+1);

		return FAILURE;
	}

	/* No error or exception */
	return SUCCESS;

}



/*******************************************************************************
API Internal Function:	Computing Pll dividers and multipliers	
Input:					Ext_Clk (from Host), and Pll clk desired.
Constraints:			6<=Ext_Clk<=350 and 7.93<=pll_clk<=1000
Return:					Pll CLk calculated or FAILURE	
********************************************************************************/
uint32_t Yushan_Compute_Pll_Divs(uint32_t fpExternal_Clk, uint32_t fpTarget_PllClk)
{
	/* fploop: Pll multiplier. */
	/* fpidiv: Input div factor. */
	/* bODiv: Output div factor. */
	uint32_t		fpIDiv, fpClkPll; /* Fixed point */
	uint32_t		fpBelow[4], fpAbove[4], *pDivPll; /* Fixed point notation */

	uint16_t		uwFreqVco, uwIdFreq;
	uint8_t			bODiv, bLoop, bIDivCount = 0;
	uint8_t			bSpiData;
	bool_t			fStatus = SUCCESS;


	/* Arrays for saving Pll paramters above or below desired Pll_Clk. */
	fpBelow[PLL_CLK_INDEX]=0;
	fpAbove[PLL_CLK_INDEX]=0x7FFFFFFF; /*0x1000000;*/

	/* below = {"clock": 0} */
	/* above = {"clock": 1000000} */
	for (bIDivCount = 1; bIDivCount < 8; bIDivCount++) {
		/*bIDivCount = 1;*/
		/* Corresponding IDiv in Fixed point format */
		fpIDiv = Yushan_ConvertTo16p16FP((uint16_t)bIDivCount);
		/* fpidfreq: Input freq to Pll post input divider.
		*  As division yields to be non Fixed point notation. So no need to convert the result in
		*  fixed point format for the sake of computation complexity. */
		uwIdFreq = fpExternal_Clk/fpIDiv;
		if ((uwIdFreq >= 6) && (uwIdFreq <= 50)) {
			for (bLoop = 10; bLoop < 168; bLoop++) {
				/* VCO_Freq at the output of PLL. */
				/* FreqVCO = IDFreq * 2 * Loop */
				uwFreqVco = uwIdFreq * 2 * bLoop;/* Not in Fixed point for the sake of computation complexity */
				if ((uwFreqVco >= 1000) && (uwFreqVco <= 2000)) {
					for (bODiv = 1; bODiv < 64; bODiv++) {/* Not in fixed point for the sake of computation complexity */

						/* fpClkPll = fpFreqVco / (2 * fpODiv); */
						fpClkPll = Yushan_ConvertTo16p16FP((uint16_t)(uwFreqVco / (2 * bODiv))); /* Can be replaced by */
						/* fpClkPll = ((uwFreqVco*0x10000)/(2 * bODiv)); */

						if ((fpClkPll <= fpTarget_PllClk) && (fpClkPll >= fpBelow[PLL_CLK_INDEX])) {
							fpBelow[PLL_CLK_INDEX] = fpClkPll, fpBelow[IDIV_INDEX] = fpIDiv, fpBelow[LOOP_INDEX] = Yushan_ConvertTo16p16FP((uint16_t)bLoop), fpBelow[ODIV_INDEX] = Yushan_ConvertTo16p16FP((uint16_t)bODiv);
						}
						if ((fpClkPll >= fpTarget_PllClk) && (fpClkPll <= fpAbove[PLL_CLK_INDEX])) {
							fpAbove[PLL_CLK_INDEX] = fpClkPll, fpAbove[IDIV_INDEX] = fpIDiv, fpAbove[LOOP_INDEX] = Yushan_ConvertTo16p16FP((uint16_t)bLoop), fpAbove[ODIV_INDEX] = Yushan_ConvertTo16p16FP((uint16_t)bODiv);
						}
					}
				}
			}               
		}
	}


	/* Commented for Workaround for HTC issue.*/
#if 0
	if ((fpTarget_PllClk - fpBelow[PLL_CLK_INDEX]) < (fpAbove[PLL_CLK_INDEX] - fpTarget_PllClk))
		pDivPll = &fpBelow[PLL_CLK_INDEX];
	else
		pDivPll = &fpAbove[PLL_CLK_INDEX];
#else
	/* Replacing the above condition. This will work well with the lower value of the Threshold */
	/* Either below or equal to desired data rate is feasible. Higher than desired rate is not acceptable. */
	if ((fpBelow[PLL_CLK_INDEX] <= fpTarget_PllClk)&&(fpAbove[PLL_CLK_INDEX]>fpTarget_PllClk))
		pDivPll = &fpBelow[PLL_CLK_INDEX];
	else if (fpAbove[PLL_CLK_INDEX] == fpTarget_PllClk)
		pDivPll = &fpAbove[PLL_CLK_INDEX];
	else {
		/* Unlikey condition. Added for debug only */
		ERRORLOG("[CAM] %s: Error, Above Conditions not satisfied ...\n", __func__);
		return FAILURE;
	}
#endif

	/* program pll dividers to H/W. */
	/* Shifting towards right will yeilds non fixed point values */
	bSpiData = (uint8_t)((*(pDivPll+1))>>16);
	fStatus	&= SPI_Write(YUSHAN_PLL_CTRL_MAIN+1, 1, (uint8_t *)  (&bSpiData));
	bSpiData = (uint8_t)((*(pDivPll+2))>>16);
	fStatus	&= SPI_Write(YUSHAN_PLL_LOOP_OUT_DF, 1, (uint8_t *) (&bSpiData));
	bSpiData = (uint8_t)((*(pDivPll+3))>>16);
	fStatus	&= SPI_Write(YUSHAN_PLL_LOOP_OUT_DF+1, 1, (uint8_t *) (&bSpiData));

	if (!fStatus) 
		return FAILURE; /* If any writes fails, then return FAILURE. */
	else	/* returning pll_clk */
		return *pDivPll;


}






/****************************************************************************
API Function:	Read DXO IP versions
Input:			pointer to sYushanVersionInfo struct
Return:			Update sYushanVersionInfo
*****************************************************************************/
bool_t Yushan_Get_Version_Information(Yushan_Version_Info_t * sYushanVersionInfo) 
{
// default to use lib v1.1
#if 1 //#ifdef CONFIG_USEDXOAF
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




/*********************************************************************
				Interrupt Functions Starts Here
**********************************************************************/


/*******************************************************************
API Function:	Assign Interrupt Groups/Sets to Pad/Pin1
*******************************************************************/
void	Yushan_AssignInterruptGroupsToPad1(uint16_t	uwAssignITRGrpToPad1)
{
	VERBOSELOG("[CAM] %s: Start\n", __func__);
	VERBOSELOG("[CAM] %s: uwAssignITRGrpToPad1:0x%x\n", __func__, uwAssignITRGrpToPad1);
	SPI_Write(YUSHAN_IOR_NVM_SEND_ITR_PAD1, 2, (uint8_t	*)&uwAssignITRGrpToPad1);
	VERBOSELOG("[CAM] %s: End\n", __func__);
}



/********************************************************************
API Function:	Enable Interrupts or Disable Interrupts.
Input:			Pointer to 88bit Interrupt_BitMask, which carry 1 or
				0 for the interrupts to be enabled or disabled resp.
Return:			Success or Failure
*********************************************************************/
bool_t Yushan_Intr_Enable(uint8_t *pIntrMask)
{
	/* Two counters, bIntrCount to trace through Interrupt bits 1 through 83 and another counter */
	/* bIntr32Count to trace through 32 to 0 for checking interrupts in 32 bit double word. */
	uint8_t		bTotalEventInASet, *pSpiData;
	uint8_t		bIntrCount = 1, bIntr32Count = 1, bIntrCountInSet = 0, bIntrSetID = 1;
	uint16_t	uwIntrSetOffset;
	uint32_t	udwLocalIntrMask, udwInterruptSetting, *pLocalIntrMask, udwSpiData = 0;

	/* First element 0 does not represents any set. */
	uint8_t		bFirstIndexForSet[] = {0, 1, 5, 11, 17, 23, 27, 58, 62, 69, 77, 79, 82, 83};

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	/*  First four bytes of input IntrMask */
	pLocalIntrMask = (uint32_t *)(pIntrMask);
	udwLocalIntrMask	= *pLocalIntrMask;

	/* For the first set, TotalElement = (5-1); */
	bTotalEventInASet = (bFirstIndexForSet[2] - bFirstIndexForSet[1]);

	/* Trace through IntrMask on bit basis. Plus one to tackle writes of the last set. */
	while (bIntrCount <= (TOTAL_INTERRUPT_COUNT + 1)) {
		/* If counter/index reaches the number of interrupts in the set, then write the packet to the
		*  interrupt registers. The time when these two registers are equal, double word packet has been
		*  already created considering all the bits of that set and index has now been pointing to one
		*  location more than the number of interrupts/events. */
		if (bIntrCountInSet == bTotalEventInASet) {
			/* InterruptSet offset from the Interrupt base address. Minus one due to the fact that */
			/* InterSetID starts with 1 not 0. */
			uwIntrSetOffset = ((bIntrSetID - 1) * INTERRUPT_SET_SIZE);
			/* Write udwSpiData to the Interrupt Enable regsiter. Since clks are not dirven by Pll, no auto */
			/*  incremental writes. */
			pSpiData = (uint8_t *)(&udwSpiData);
			if (gPllLocked) {
				SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_ENABLE), 4, pSpiData);
			} else {
				SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_ENABLE), 1, pSpiData);
				SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_ENABLE+1), 1, pSpiData+1);
				SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_ENABLE+2), 1, pSpiData+2);
				SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_ENABLE+3), 1, pSpiData+3);
			}
			/* pr_info("Yushan_Intr_Enable: udwSpiData:  0x%x, uwIntrSetOffset: 0x%x\n", udwSpiData, uwIntrSetOffset); */
			/* Complement of udwSpiData. */
			udwSpiData = (uint32_t)(~udwSpiData);
			/* write complemented udwSpiData to the Interrupt Disable regsiters. */
			if (gPllLocked) {
				SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_DISABLE), 4, pSpiData);
			} else {
				SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_DISABLE), 1, pSpiData);
				SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_DISABLE+1), 1, pSpiData+1);
				SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_DISABLE+2), 1, pSpiData+2);
				SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_DISABLE+3), 1, pSpiData+3);
			}

			/* Reset the counter tracking the interrupts in the current set. Also reset the data. */
			bIntrCountInSet = 0;
			udwSpiData = 0;

			/* As Intr32Count and IntrCount has been incremented and not used while doing SPI writes. */
			bIntr32Count--;
			bIntrCount--;
			/* Increase the SetID too. */
			bIntrSetID++;

			/* Set the ElementInASet. */
			if (bIntrSetID < 13)
				bTotalEventInASet = (bFirstIndexForSet[bIntrSetID+1] - bFirstIndexForSet[bIntrSetID]);
			else if (bIntrSetID == 13)
				bTotalEventInASet = 2; /* Only two events in the last IntrSet */


		} else {
			/* udwInterruptSetting gives the per bit setting, 1 or 0. */
			udwInterruptSetting = ((udwLocalIntrMask>>(bIntr32Count-1))&0x01);
			/* Create a double word */
			udwSpiData |= (udwInterruptSetting<<(bIntrCountInSet));

			/* track to next interrupt bit. */
			bIntrCountInSet++;
		}

		/* If double word has been exhausted, look for the next four bytes. */
		if (!(bIntr32Count % 32)) {
			/* Reset the 32Count counter to 1, so as to shift udwLocalIntrMask by not more than 32 bits(max). */
			bIntr32Count = 1;

			/* After every four bytes, take next double word from the *pIntrMask. */
			pLocalIntrMask++;
			udwLocalIntrMask	= *pLocalIntrMask;
		} else
			bIntr32Count++;

		/* Increase the top counter. */
		bIntrCount++;

	}

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return SUCCESS;
}




/******************************************************************
API Internal Function:	Yushan_ReadIntr_Status
Input:					InterruptList
Return:					ID of the Interrupt asserted
*******************************************************************/
void Yushan_Intr_Status_Read(uint8_t *bListOfInterrupts,	bool_t	fSelect_Intr_Pad)
{
	uint16_t	uwSpiData = 0,	uwListOfITRGrpToPad1 = 0, RaisedGrpForPAD = 0;
	uint8_t		/* bIsIntrSetAsserted, */bIntrSetID = 0; /* bIntrEventID = FALSE_ALARM; */
	bool_t		fStatus = SUCCESS;
	uint8_t		bSpiData = 0;

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	VERBOSELOG("[CAM] %s: Check for interrupts on Pad%i\n", __func__, fSelect_Intr_Pad);

	/* Clearing the interrupt list */
	memset((void *)bListOfInterrupts, 0, 96/8);

	/* Read list of Intr groups diverted to Pad1 */
	if (gPllLocked) {
		SPI_Read(YUSHAN_IOR_NVM_SEND_ITR_PAD1, 2, (uint8_t	*)&uwListOfITRGrpToPad1);
	} else {
		fStatus &= SPI_Read(YUSHAN_IOR_NVM_SEND_ITR_PAD1, 1, (uint8_t *)(&bSpiData));
		uwListOfITRGrpToPad1 = (uint16_t) bSpiData;
		fStatus &= SPI_Read(YUSHAN_IOR_NVM_SEND_ITR_PAD1+1, 1, (uint8_t *)(&bSpiData));
		uwListOfITRGrpToPad1 |= (((uint16_t) bSpiData)<<8);
	}

	/* If the function called from ISR_Pad0: then invert the list uwListOfITRGrpToPad1
	 * It will give the list of intr groups linked to Pad0 */
	if(fSelect_Intr_Pad==INTERRUPT_PAD_0)
		uwListOfITRGrpToPad1	=	~uwListOfITRGrpToPad1;

	/* Read NVM_INTR_STATUS: The register masking all the system interrupts
	*  and reading the same will give the idea of the actual set where interrupt
	*  has been asserted. No auto increment reads in case Pll not locked. */
	if (gPllLocked) {
		fStatus &= SPI_Read(YUSHAN_IOR_NVM_INTR_STATUS, 2, (uint8_t *)(&uwSpiData));
		/* pr_info("[CAM] INT Status: 0x%x\n", uwSpiData); */
	} else {
		fStatus &= SPI_Read(YUSHAN_IOR_NVM_INTR_STATUS, 1, (uint8_t *)(&bSpiData));
		uwSpiData = (uint16_t) bSpiData;
		fStatus &= SPI_Read(YUSHAN_IOR_NVM_INTR_STATUS+1, 1, (uint8_t *)(&bSpiData));
		uwSpiData |= ( ((uint16_t) bSpiData)<<8 );
	}

	/*  Check now the sets where interrupt bits have been set or asserted.
	*  Note: More than one interrupt can be asserted at one point of time and may
	*  belong to different interrupt sets. */
	RaisedGrpForPAD = uwSpiData & uwListOfITRGrpToPad1;
	while (bIntrSetID < TOTAL_INTERRUPT_SETS) {
		if ((RaisedGrpForPAD>>bIntrSetID)&0x01) {
			/* Read Interrupt Event for the Current Pad interrupts only. */
			/* Checking all the events which have triggered the interrupts and add those
			*  interrupts in the ListOfInterrupt. */
			Yushan_Read_IntrEvent(bIntrSetID, (uint32_t *)bListOfInterrupts);
		}
		/* Move to next SetID */
		bIntrSetID++;

	}

	VERBOSELOG("[CAM] %s: Read interrupts status:0x%08x, 0x%08x, 0x%08x\n", __func__, *((uint32_t *)bListOfInterrupts), *(((uint32_t *)bListOfInterrupts)+1), *(((uint32_t *)bListOfInterrupts)+2));
	VERBOSELOG("[CAM] %s: End\n", __func__);
}





/******************************************************************
API Internal Function:	Function for reading exact interrupt event.
Input:					Asserted IntrSetID.
Return:					InterruptEventID as define in Yushan_API.h	
*******************************************************************/
void Yushan_Read_IntrEvent(uint8_t bIntrSetID, uint32_t *udwListOfInterrupts)
{

	uint8_t		bInterruptID, bTotalEventInASet, bIntrCountInSet = 0, bTempIntrStatus = 0;
	uint8_t		bFirstIndexForSet[] = {1, 5, 11, 17, 23, 27, 58, 62, 69, 77, 79, 82, 83};
	uint16_t	uwIntrSetOffset;
	uint32_t	udwIntrStatus, udwIntrEnableStatus, udwCombinedStatus;
	/* bool_t		fStatus = SUCCESS;, fIntrInListAlready = FAILURE; */
	
	if (bIntrSetID != 13)
		bTotalEventInASet = (bFirstIndexForSet[bIntrSetID+1] - bFirstIndexForSet[bIntrSetID]);
	else
		bTotalEventInASet = 2; /* Only two events in the last IntrSet */

	/* Move offset to points to the current set. */
	uwIntrSetOffset = (bIntrSetID * INTERRUPT_SET_SIZE);

	/* First read Intr_Status and Enable status of the asserted set.
	*  NOTE: NVM_INTR_STATUS register will set its bits only when events (1 or more), of a particular set, have been enabled 
	*  and interrupts to those events, of the same set have been asserted. So it is must for us to check both of the Interrupt 
	*  Enable Status and Interrupt Status regsiters. Auto Increment only after Pll locked. */

	if (gPllLocked) {	/* AutoIncrement if Pll locked */
		/* IntrStatus */
		SPI_Read((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset), 4, (uint8_t *)(&udwIntrStatus));
		/* IntrEnableStatus */
		SPI_Read((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_ENABLE_STATUS), 4, (uint8_t *)(&udwIntrEnableStatus));
	} else {
		SPI_Read((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset),   1, (uint8_t *)(&bTempIntrStatus));
		udwIntrStatus = (uint32_t)(bTempIntrStatus);
		SPI_Read((uint16_t)((YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset)+1), 1, (uint8_t *)(&bTempIntrStatus));
		udwIntrStatus |= ((uint32_t)(bTempIntrStatus))<<8;
		SPI_Read((uint16_t)((YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset)+2), 1, (uint8_t *)(&bTempIntrStatus));
		udwIntrStatus |= ((uint32_t)(bTempIntrStatus))<<16;
		SPI_Read((uint16_t)((YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset)+3), 1, (uint8_t *)(&bTempIntrStatus));
		udwIntrStatus |= ((uint32_t)(bTempIntrStatus))<<24;

		/* IntrEnableStatus */
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

	/* Scan in the set for the asserted event. */
	while (bIntrCountInSet < bTotalEventInASet) {
		/* Check if the Interrupt has been enabled and asserted. Then return the first InterruptEventID (being defined in
		*  Yushan_API.h) for which both the signals are high.
		*  Note: There can be more than one event asserted in a set, but return only the first one and clear its respective
		*  status before returning. */
		udwCombinedStatus = (udwIntrStatus & udwIntrEnableStatus);
		/* if ( ((udwIntrStatus>>bIntrCountInSet) & 0x01) && ((udwIntrEnableStatus>>bIntrCountInSet) & 0x01) ) */
		if ((udwCombinedStatus>>bIntrCountInSet) & 0x01) {
			/* Entered since both the IntrStatus and EnableStatus are true for the event, at bit, indexed at bIntrCountInSet.
			*  Add IntrID to the ListOfInterrupts. */
			bInterruptID = (bFirstIndexForSet[bIntrSetID] + bIntrCountInSet);


			Yushan_AddnRemoveIDInList(bInterruptID, udwListOfInterrupts, ADD_INTR_TO_LIST);


		}

		bIntrCountInSet++;
	}



}



/********************************************************************
API Function:	Clear Status of Interrupts asserted or occured
Input:			Pointer to 83bit udwListOfInterrupts, which carry 1 or
				0 for the interrupts to be cleared.
Return:			Success or Failure
*********************************************************************/
bool_t Yushan_Intr_Status_Clear(uint8_t *bListOfInterrupts)
{
	/* Two counters, bIntrCount to trace through Interrupt bits 1 through 83 and another counter
	*  bIntr32Count to trace through 32 to 0 for checking interrupts in 32 bit double word. */
	uint8_t		bTotalEventInASet; /*, *pSpiData; */
	uint8_t		bIntrCount = 1, bIntr32Count = 1, bIntrCountInSet = 0, bIntrSetID = 1;
	uint16_t	uwIntrSetOffset;
	uint32_t	udwLocalIntrMask, udwInterruptSetting, *pLocalIntrMask, udwSpiData = 0;
	/* uint8_t bSpiData; */

	/* First element 0 does not represents any set. */
	uint8_t		bFirstIndexForSet[] = {0, 1, 5, 11, 17, 23, 27, 58, 62, 69, 77, 79, 82, 83};

	VERBOSELOG("[CAM] %s: Start\n", __func__);


	pLocalIntrMask = (uint32_t *) bListOfInterrupts; 
	udwLocalIntrMask	= *pLocalIntrMask;

	/* For the first set, TotalElement = (5-1); */
	bTotalEventInASet = (bFirstIndexForSet[2] - bFirstIndexForSet[1]);

	/* Trace through IntrMask on bit basis. Plus one to tackle writes of the last set. */
	while (bIntrCount <= (TOTAL_INTERRUPT_COUNT + 1)) {

		/* If counter/index reaches the number of interrupts in the set, then write the packet to the
		*  interrupt registers. The time when these two registers are equal, double word packet has been
		*  already created considering all the bits of that set and index has now been pointing to one
		*  location more than the number of interrupts/events. */
		if (bIntrCountInSet == bTotalEventInASet) {
			/* InterruptSet offset from the Interrupt base address. Minus one due to the fact that
			*  InterSetID starts with 1 not 0. */
			uwIntrSetOffset = ((bIntrSetID-1)* INTERRUPT_SET_SIZE);
			/* Write udwSpiData to the Interrupt Enable regsiter. Since clks are not dirven by Pll, no auto 
			*  incremental writes.
			*  Auto increments writes possible after Pll locked */
			/* pr_info("[CAM] ClearINTStatus: 0x%x\n", udwSpiData); */
			/* udwSpiData = 0xFFFFFFFF; */
			SPI_Write((uint16_t)(YUSHAN_INTR_BASE_ADDR + uwIntrSetOffset + YUSHAN_OFFSET_INTR_STATUS_CLEAR), 4, (uint8_t *)(&udwSpiData));


			/* Reset the counter tracking the interrupts in the current set. Also reset the data. */
			bIntrCountInSet = 0;
			udwSpiData = 0;

			/* As Intr32Count and IntrCount has been incremented and not used while doing SPI writes. */
			bIntr32Count--;
			bIntrCount--;
			/* Increase the SetID too. */
			bIntrSetID++;

			/* Set the ElementInASet. */
			if (bIntrSetID < 13)
				bTotalEventInASet = (bFirstIndexForSet[bIntrSetID+1] - bFirstIndexForSet[bIntrSetID]);
			else if (bIntrSetID == 13)
				bTotalEventInASet = 2; /* Only two events in the last IntrSet */


		} else {
			/* udwInterruptSetting gives the per bit setting, 1 or 0. */
			udwInterruptSetting = ((udwLocalIntrMask>>(bIntr32Count-1))&0x01);
			/* Create a double word */
			udwSpiData |= (udwInterruptSetting<<(bIntrCountInSet));
			
			/* track to next interrupt bit. */
			bIntrCountInSet++;
		}

		/* If double word has been exhausted, look for the next four bytes. */
		if (!(bIntr32Count % 32)) {
			/* Reset the 32Count counter to 1, so as to shift udwLocalIntrMask by not more than 32 bits(max). */
			bIntr32Count = 1;

			/* After every 32bits, take next double word from the *pLocalIntrMask. */
			pLocalIntrMask++;
			udwLocalIntrMask	= *pLocalIntrMask;
		} else
			bIntr32Count++;


		/* Increase the top counter. */
		bIntrCount++;

	}

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return SUCCESS;
}



/*******************************************************************
API Internal Function:	Yushan_Check_Pad_For_IntrID:
This function check the PAD to which the current Interrupt belongs to.
*******************************************************************/
bool_t	Yushan_Check_Pad_For_IntrID(uint8_t	bInterruptId)
{

	uint8_t		bFirstIndexForSet[] = {1, 5, 11, 17, 23, 27, 58, 62, 69, 77, 79, 82, 83};
	uint8_t		bIntrSetID = 0;
	uint16_t	uwIntrSetsDivertedToPad1 = 0;

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	/* Read the list of the interrupt sets diverted to Pad1 */
	SPI_Read(YUSHAN_IOR_NVM_SEND_ITR_PAD1 , 2, (uint8_t	*)&uwIntrSetsDivertedToPad1);

	/* Trace through InterruptSets */
	while(bIntrSetID < TOTAL_INTERRUPT_SETS)
	{

		if( (bInterruptId>=bFirstIndexForSet[bIntrSetID])&&(bInterruptId<bFirstIndexForSet[bIntrSetID+1]) )
		{
			if((uwIntrSetsDivertedToPad1>>bIntrSetID)&0x01) {
				VERBOSELOG("[CAM] %s: End\n", __func__);
				return INTERRUPT_PAD_1;
			} else {
				VERBOSELOG("[CAM] %s: End\n", __func__);
				return INTERRUPT_PAD_0;
			}
		} else
			bIntrSetID++;

	}

	/* Just to remove warning */
	VERBOSELOG("[CAM] %s: End\n", __func__);
	return INTERRUPT_PAD_0;

}


/*******************************************************************
API Function:	Yushan_CheckForInterruptIDInList:
Check InterruptID in the Interrupt list.
*******************************************************************/
bool_t Yushan_CheckForInterruptIDInList(uint8_t bInterruptID, uint32_t *udwProtoInterruptList)
{

	bool_t		fStatus = 0;
	uint8_t		bIntrDWordInList, bIndexInCurrentDWord;
	uint32_t	*pListOfInterrupt;


	/* pListOfInterrupt =  &udwProtoInterruptList[0]; */
	pListOfInterrupt =  udwProtoInterruptList;


	if ((1<=bInterruptID)&&(bInterruptID<=32))
		bIntrDWordInList = 0;
	else if ((33<=bInterruptID)&&(bInterruptID<=64))
		bIntrDWordInList = 1;
	else
		bIntrDWordInList = 2;

	/* Shift pointer by (bIntrDWordInList*32) elements. */
	pListOfInterrupt = pListOfInterrupt + bIntrDWordInList;

	/* Carry those 32 bit, among which Interrupt in interest exists.
	*  Mnus one as the list is consistence with the bit mask, used, for enabling
	*  the interrupts */
	bIndexInCurrentDWord = (bInterruptID - (bIntrDWordInList*32)) - 1;


	/* Check whether the desired Interrupt has been aserted or not. */
	fStatus = (bool_t)(((*pListOfInterrupt)>>bIndexInCurrentDWord) & 0x00000001);

	return fStatus;

}




/**********************************************************************
Function:	To add or remove ( set or clear  bit corresponding
					to) interrupt mentioned in input.
Input:				Interrupt ID to be added or removed from the list
					and a Flag to decide to add or remove.
Return:
***********************************************************************/
void Yushan_AddnRemoveIDInList(uint8_t bInterruptID, uint32_t *udwListOfInterrupts, bool_t fAddORClear)
{

	uint8_t		bIntrDWordInList, bIndexInCurrentDWord;
	uint32_t	*pListOfInterrupt, udwTempIntrList, udwMask = 0x00000001;

	/* Point to 96bit (global) IntrList of which only first 83bits are valid for 83 interrupts. */
	pListOfInterrupt = udwListOfInterrupts;

	/* Scan the list for the received ID and add the same to the list, if requested.
	*  The list will be a 96 bit (3 element array of double word) of which
	*  83bits will be valid. Bit will carry 1, if corresponding interrupt has been aserted else will
	*  be zero. */
	if ((1<=bInterruptID)&&(bInterruptID<=32))
		bIntrDWordInList = 0;
	else if ((33<=bInterruptID)&&(bInterruptID<=64))
		bIntrDWordInList = 1;
	else
		bIntrDWordInList = 2;

	/* Shift pointer by (bIntrDWordInList*32) elements. */
	pListOfInterrupt = pListOfInterrupt + bIntrDWordInList;

	/* Minus one makes the list in line with the Bit mask required, input to Intr_Enable function */
	bIndexInCurrentDWord = (bInterruptID - (bIntrDWordInList*32)) - 1;

	/* Intermediate List */
	/* udwTempIntrList = ( ((*pListOfInterrupt)>>(bIndexInCurrentDWord)) | udwMask ); // udwMask = 0x00000001 */

	/* Modifying the list. */
	if (fAddORClear == ADD_INTR_TO_LIST) {
		/* For adding intr in list */
		udwTempIntrList = ( ((*pListOfInterrupt)>>(bIndexInCurrentDWord)) | udwMask ); /* udwMask = 0x00000001 */
		*pListOfInterrupt |= (udwTempIntrList << (bIndexInCurrentDWord));
	} else if (fAddORClear == DEL_INTR_FROM_LIST) {
		/* For Clearing intr from the list */
		udwTempIntrList = ( ((*pListOfInterrupt)>>(bIndexInCurrentDWord)) & udwMask ); /* udwMask = 0x00000001 */
		*pListOfInterrupt &= ~(udwTempIntrList << (bIndexInCurrentDWord));
	}


}


/*********************************************************************
				Interrupt Functions Ends Here
**********************************************************************/



/*********************************************************************
				Optional Functions Starts Here
**********************************************************************/



/*************************************************************
API Functtion: Update the CPX and DCPX IPs of Yushan in case of
RAW 10 to 8 pixel format
Input: void
**************************************************************/
void	Yushan_DCPX_CPX_Enable(void)
{

	uint8_t	bSpiData = 0;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	/* DCPX */
	bSpiData = 0x01;
	SPI_Write(YUSHAN_SMIA_DCPX_ENABLE, 1, &bSpiData); /* DCPX - DCPX_ENABLE */
	bSpiData = 0x08;
	SPI_Write(YUSHAN_SMIA_DCPX_MODE_REQ, 1, &bSpiData); /* DCPX - DCPX_MODE_REQ */
	bSpiData = 0x0A;
	SPI_Write(YUSHAN_SMIA_DCPX_MODE_REQ+1, 1, &bSpiData); /* DCPX - DCPX_MODE_REQ */

	/* CPX */
	bSpiData = 0x01;
	SPI_Write(YUSHAN_SMIA_CPX_CTRL_REQ, 1, &bSpiData); /* CPX - CPX_CTRL_REQ */
	/* bSpiData = 0x01 */
	SPI_Write(YUSHAN_SMIA_CPX_MODE_REQ, 1, &bSpiData); /* CPX - CPX_MODE_REQ */

	VERBOSELOG("[CAM] %s: End\n", __func__);
}


/*********************************************************************
API Function:	Read current streaming mode status
Input:

Return:			Current streaming mode (STILL or VF)
**********************************************************************/
uint8_t Yushan_GetCurrentStreamingMode(void)
{
	uint8_t bSpiData,currentEvent;
	uint8_t CurrentStreamingMode;

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	/* Read back value of T1_DMA_REG_STATUS register */
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
		/* Unknown streaming mode */
		CurrentStreamingMode = 255;
	}

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return CurrentStreamingMode;
}


/*********************************************************************
API Function:	This function will update the context configuration.
Input:			Function will ask for inputs, i.e., New resolution, format
				and mode, i.e., preview or capture.

Return:			Success or Failure
**********************************************************************/
bool_t	Yushan_Context_Config_Update(Yushan_New_Context_Config_t	*sYushanNewContextConfig)
{

	/* bool_t		bStatus=SUCCESS; */
	uint8_t		bVfStillIndex, bVFIndex, bStillIndex, bVFMask=0;
	/* HTC_START (klockwork issue)*/
	uint8_t		bDataType=0, bCurrentDataType=0, bActiveDatatype=1, bRawFormat=0, bWCAlreadyPresent = 0;
	/* HTC_END */
	uint8_t		bPixClkDiv=0, bDxoClkDiv=0, bCount=0;
	uint16_t	uwNewHSize=0, uwLecci=0;
	uint32_t	udwSpiData = 0;
	uint16_t	wordCount = 0;
	
	VERBOSELOG("[CAM] %s: Start\n", __func__);


	/* RawFormat */
	/* bRawFormat = (sYushanNewContextConfig->uwPixelFormat&0x0F); */
	/* bDataType */
	/* bDataType = (bRawFormat==0x08)?0x2a:0x2b; */

	if ((sYushanNewContextConfig->uwPixelFormat&0x0F)==0x0A){
		bRawFormat = RAW10;
		bDataType  = 0x2b;
	} else if((sYushanNewContextConfig->uwPixelFormat&0x0F)==0x08) {
		bRawFormat= RAW8; 
		if(((sYushanNewContextConfig->uwPixelFormat>>8)&0x0F)==0x08)
			bDataType = 0x2a;
		else /* 10 to 8 case */
			bDataType = 0x30; /* bRawFormat at Yushan should be RAW8, not RAW10_8 */
	}

	/* Read the Index information of the VF or STILL resolution. */
	SPI_Read(YUSHAN_IDP_GEN_CONTROL, 1, (uint8_t *)(&bVfStillIndex));
	bVFIndex = bVfStillIndex&0x0F;
	bStillIndex = (bVfStillIndex&0xF0)>>4;

	/* Take new cofiguration */
	uwNewHSize = sYushanNewContextConfig->uwActivePixels;

	/* Find out if the required Wordcount is still present in IDP_Gen_WCIndex matrix. Also looking for its 
	*  corresponding mode. In case WC not present, add new WC after the last valid entry */
	for (bCount = 0; bCount < 13; bCount++) {
		SPI_Read((uint16_t)(YUSHAN_IDP_GEN_WC_DI_0+4*bCount),4,(unsigned char*)&udwSpiData);
		wordCount = udwSpiData & 0x0000ffff; /* Wordcount check */
		if (wordCount==0) /* Wordcount=0. Add new WC here */
			break;

		bCurrentDataType = (udwSpiData>>16);							/* Retaining Datatype */
		udwSpiData = ((udwSpiData&0xFFFF)*8)/bRawFormat;				/* Resolution/H_Size against WC */
		
		/* Check if the desired resolution is already present and with the same data type */
		if ((uwNewHSize == udwSpiData) && (bCurrentDataType == bDataType)) {
			bWCAlreadyPresent = 1;
			if((bCount==bVFIndex)||(bCount==bStillIndex)) {
				VERBOSELOG("[CAM] %s: End\n", __func__);
				return SUCCESS;			/* No need for reprogramming required */
			} else
				break;					/* Only T1_DMA programming required */
		}
	}

	
	/* Preparing Idp_Gen_Control packet */
	SPI_Read(YUSHAN_IDP_GEN_CONTROL, 4, (uint8_t *)(&udwSpiData));
	if (sYushanNewContextConfig->bSelectStillVfMode == YUSHAN_FRAME_FORMAT_VF_MODE) {
		udwSpiData &= 0xFFFFFFF0;
		udwSpiData |= bCount;
		/* For T1_DMA Programming */
		bVFMask = 1;
	} else if (sYushanNewContextConfig->bSelectStillVfMode == YUSHAN_FRAME_FORMAT_STILL_MODE) {
		udwSpiData &= 0xFFFFFF0F;
		udwSpiData |= bCount << 4;
		/* For T1_DMA Programming */
		bVFMask = 0;
	}

	/* Program the Idp_Gen_Ctrl */
	SPI_Write((uint16_t)(YUSHAN_IDP_GEN_CONTROL), 4, (uint8_t *)(&udwSpiData));

	/* Reprogram the Idp_Gen_Ctrl with new VF/STILL index. */
	if (bWCAlreadyPresent && (bCount != 13)) {
		/* WC already present with Normal mode programmed. */
		SPI_Write((uint16_t)((YUSHAN_IDP_GEN_WC_DI_0+(4*bCount))+3),	1,	(unsigned char*)&bActiveDatatype);
	} else if (bCount != 13) {
		/* New entry to be added at bCount */
		/* Wordcount */
		udwSpiData=(sYushanNewContextConfig->uwActivePixels*bRawFormat)/8;
		SPI_Write((uint16_t)(YUSHAN_CSI2_TX_PACKET_SIZE_0+0xc*bCount),	4,	(unsigned char*)&udwSpiData);

		SPI_Write((uint16_t)(YUSHAN_CSI2_TX_DI_INDEX_CTRL_0+0xc*bCount),	1,	(unsigned char*)&bDataType);

		udwSpiData = udwSpiData | (bDataType<<16) | (bActiveDatatype<<24);
		SPI_Write((uint16_t)(YUSHAN_IDP_GEN_WC_DI_0+4*bCount),	4,	(unsigned char*)&udwSpiData);
	} else {
		/* No entry vacant. Reconfiguration unsuccessful */
		ERRORLOG("[CAM] %s: Error, No entry vacant. Exiting ...\n", __func__);
		return FAILURE;
	}
	
	/* T1_DMA Programming. */
	/* For PDP */
	udwSpiData = (uwNewHSize & 0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT + bVFMask),1,(unsigned char*)&udwSpiData);
	udwSpiData = ((uwNewHSize>>8)&0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+8+ bVFMask),1,(unsigned char*)&udwSpiData);

	/* For DPP */
	udwSpiData=(uwNewHSize & 0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+24+ bVFMask),1,(unsigned char*)&udwSpiData);
	udwSpiData=((uwNewHSize>>8)&0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+32+ bVFMask),1,(unsigned char*)&udwSpiData);

	/* For DOP */
	udwSpiData=(uwNewHSize & 0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+48+ bVFMask),1,(unsigned char*)&udwSpiData);
	udwSpiData=((uwNewHSize>>8)&0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+56+ bVFMask),1,(unsigned char*)&udwSpiData);

	/* For LBE PreDxo */
	udwSpiData=(uwNewHSize & 0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+72+ bVFMask),1,(unsigned char*)&udwSpiData);
	udwSpiData=((uwNewHSize>>8)&0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+80+ bVFMask),1,(unsigned char*)&udwSpiData);

	/* For LBE PostDxo */
	udwSpiData=(uwNewHSize & 0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+88+ bVFMask),1,(unsigned char*)&udwSpiData);
	udwSpiData=((uwNewHSize>>8)&0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+96+ bVFMask),1,(unsigned char*)&udwSpiData);

	/* For Lecci */
	udwSpiData=(uwNewHSize & 0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+104+ bVFMask),1,(unsigned char*)&udwSpiData);
	udwSpiData=((uwNewHSize>>8)&0xFF);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+112+ bVFMask),1,(unsigned char*)&udwSpiData);

	/***** Lecci prog ******/
	/* Min_interline = (Freq_Pix*interline)/Freq_DXO;. */
	SPI_Read(YUSHAN_CLK_DIV_FACTOR,1,(unsigned char*)&bDxoClkDiv); 
	SPI_Read(YUSHAN_CLK_DIV_FACTOR+1,1,(unsigned char*)&bPixClkDiv); 

	if ((bDxoClkDiv !=0) && (bPixClkDiv !=0))
		uwLecci = (sYushanNewContextConfig->uwLineBlank*bPixClkDiv)/ bDxoClkDiv;
	else
		uwLecci = 300;

	/* For Lecci */
	udwSpiData=(uwLecci&0xff);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+120+bVFMask),1,(unsigned char*)&udwSpiData);

	udwSpiData=((uwLecci>>8)&0xff);
	SPI_Write((uint16_t)(YUSHAN_T1_DMA_MEM_LOWER_ELT+128+bVFMask),1,(unsigned char*)&udwSpiData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return SUCCESS;


}





/******************************************************************
API Function:	Set Af strategy
Input:			Byte type variable carrying AF strategy
Return:			Success or Failure
*******************************************************************/
bool_t Yushan_Update_DxoDop_Af_Strategy(uint8_t  bAfStrategy)
{
	bool_t	fStatus = SUCCESS;
	VERBOSELOG("[CAM] %s: Start\n", __func__);
	/* Temporary check until everyone use DOP7 version 2.x */
#if DxODOP_dfltVal_ucode_id_15_8 == 2
	fStatus = SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_af_strategy_7_0, 1, &bAfStrategy);
#else
	VERBOSELOG("[CAM] %s: DOP used not compatiblewith this function\n", __func__);
#endif
	VERBOSELOG("[CAM] %s: End with Status:%i\n", __func__, fStatus);
	return fStatus;
}



/*********************************************************************
API Function:	This function will update the DXO_DOP AF ROIs (Five ROIs).
Input:			Structure for ROI and NumOfActiveROI
Return:			Success or Failure
**********************************************************************/
bool_t Yushan_AF_ROI_Update(Yushan_AF_ROI_t  *sYushanAfRoi, uint8_t bNumOfActiveRoi) 
{

	uint8_t		bSpiData[4];
	uint8_t		bStatus = SUCCESS, bCount=0;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	if (!bNumOfActiveRoi) { /* NO ROI ACTIVATED */
		DEBUGLOG("[CAM] %s:No ROI Activated so exiting with SUCCESS\n", __func__);
		return SUCCESS;
	} else {
		/* Program active number of ROI */
		bStatus &= SPI_Write(DXO_DOP_BASE_ADDR+DxODOP_ROI_active_number_7_0, 1, (uint8_t*)(&bNumOfActiveRoi));
	}

	/* pr_info("[CAM] AF_ROI_Update:active number:%d, xstart:%d %d %d %d \n", bNumOfActiveRoi, sYushanAfRoi->bXStart, sYushanAfRoi->bYStart, sYushanAfRoi->bXEnd, sYushanAfRoi->bYEnd); */

	/* Program the active number of ROIs */
	while (bCount < bNumOfActiveRoi) {
		bSpiData[0] = (sYushanAfRoi->bXStart);
		bSpiData[1] = (sYushanAfRoi->bYStart);
		bSpiData[2] = (sYushanAfRoi->bXEnd);
		bSpiData[3] = (sYushanAfRoi->bYEnd);

		/* SPI transaction */
		/* bCount*8 will take care of the offset from the base adress. '8': Number of bytes in one ROI programming. */
		bStatus &= SPI_Write((uint16_t)(DXO_DOP_BASE_ADDR+DxODOP_ROI_0_x_start_7_0 + bCount*4), 4, (uint8_t*)(&bSpiData[0]));
		/* Count increment */
		bCount++;

		/* Points to the next ROI struct */
		sYushanAfRoi++;
	}
	VERBOSELOG("[CAM] %s: End\n", __func__);
	return bStatus;
}




/******************************************************************
API Function:	The function will put the Yushan in standby mode. It
				will stop the MIPI phy blocks, stop the PLL and LDO
				to reduce the power consumption.
Input:
Return:			Success or Failure
*******************************************************************/
bool_t Yushan_Enter_Standby_Mode(void)
{
	uint8_t		bSpiData;
	uint32_t	udwSpiData;
	bool_t		fStatus = SUCCESS;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	/* Disable the MIPI RX and TX. */
	bSpiData = 0;
	fStatus &= SPI_Write(YUSHAN_MIPI_RX_ENABLE, 1, (uint8_t*)(&bSpiData));
	fStatus &= SPI_Write(YUSHAN_MIPI_TX_ENABLE, 1, (uint8_t*)(&bSpiData));


	/* Switch the Clocks to Host clk and then disable the Pll. */
	
	/* For glitch free switching, first disable the clocks */
	/* Saving the context/data of the Clk_Ctrl */
	SPI_Read(YUSHAN_CLK_CTRL,   1, (uint8_t*)(&bSpiData));		/* Saving the First */
	SPI_Read(YUSHAN_CLK_CTRL+2, 2, (uint8_t*)(&udwSpiData));	/* Third and fourth byte. */
	udwSpiData = (bSpiData | (udwSpiData<<16));

	/* Disable the clocks */
	bSpiData = 0x00;
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL, 1, (uint8_t*)(&bSpiData));
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+2, 1, (uint8_t*)(&bSpiData)); 
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+3, 1, (uint8_t*)(&bSpiData)); 

	/* Switch Sys clk to Host clk from Pll clk */
	SPI_Read(YUSHAN_CLK_CTRL+1, 1, (uint8_t*)(&bSpiData));
	bSpiData &= 0xFE;	/* Clk_Sys_Sel set to 0 */
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+1, 1, (uint8_t*)(&bSpiData));

	/* Bypass Pll */
	// SPI_Read(YUSHAN_CLK_CTRL+1, 1, (uint8_t*)(&bSpiData));	/* No need of read, as the data written is saved in bSpiData */
	bSpiData |= ((bSpiData>>1)|0x01)<<1;						/* bypass Pll set to 1 */
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+1, 1, (uint8_t*)(&bSpiData));

	// Enable the clock */
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL, 1, (uint8_t*)(&udwSpiData));		/* First byte of the context */
	udwSpiData = (udwSpiData >> 16);										/* Third and fourth byte */
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+2, 1, (uint8_t*)(&udwSpiData));	
	udwSpiData = (udwSpiData >> 8);
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+3, 1, (uint8_t*)(&udwSpiData));	
	 
	/* Now disable the Pll clk */
	SPI_Read(YUSHAN_PLL_CTRL_MAIN, 1, (uint8_t*)(&bSpiData));
	bSpiData |= 0x01;			/* Power down  = 1 */
	fStatus &= SPI_Write(YUSHAN_PLL_CTRL_MAIN, 1, (uint8_t*)(&bSpiData));

	/* Disable the LDO */

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;


}




/*********************************************************************
API Function:	The function will bring the Yushan out of standby mode.
				It will enable the MIPI phy blocks,	PLL and LDO.
Input:			sInitStruct for NumberOfLanes information	
Return:			Success or Failure
**********************************************************************/
bool_t Yushan_Exit_Standby_Mode(Yushan_Init_Struct_t * sInitStruct)
{
	uint8_t		bSpiData;
	uint32_t	udwSpiData;
	bool_t		fStatus = SUCCESS;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	/* Switch the Clocks to Pll clk */

	/* Now Enable the Pll clk */
	SPI_Read(YUSHAN_PLL_CTRL_MAIN, 1, (uint8_t*)(&bSpiData));
	bSpiData &= 0xFE;	/* Power down  = 0 */
	SPI_Write(YUSHAN_PLL_CTRL_MAIN, 1, (uint8_t*)(&bSpiData));

	fStatus &= Yushan_WaitForInterruptEvent(EVENT_PLL_STABLE, TIME_20MS);
	
	if (!fStatus) {
		ERRORLOG("[CAM] %s: Error: EVENT_PLL_STABLE not received. Exiting...\n", __func__);
		return fStatus;
	}

	/* For glitch free switching, first disable the clocks */
	/* Saving the context/data of the Clk_Ctrl */
	SPI_Read(YUSHAN_CLK_CTRL,   1, (uint8_t*)(&bSpiData));		/* Saving the First */
	udwSpiData = (uint32_t)(bSpiData);
	SPI_Read(YUSHAN_CLK_CTRL+2, 1, (uint8_t*)(&bSpiData));		/* Third byte. */
	udwSpiData |= (((uint32_t)(bSpiData))<<8);
	SPI_Read(YUSHAN_CLK_CTRL+3, 1, (uint8_t*)(&bSpiData));		/* Fourth byte. */
	udwSpiData |= (((uint32_t)(bSpiData))<<16);


	/* Disable the clocks */
	bSpiData = 0x00;
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL, 1, (uint8_t*)(&bSpiData));
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+2, 1, (uint8_t*)(&bSpiData));		
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+3, 1, (uint8_t*)(&bSpiData));		

	/* No bypass Pll */
	SPI_Read(YUSHAN_CLK_CTRL+1, 1, (uint8_t*)(&bSpiData));
	bSpiData &= (bSpiData&0xFD);						/* bypass Pll set to 0 */
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+1, 1, (uint8_t*)(&bSpiData));

	/* Switch Sys clk to Pll clk from Host clk */
	/* SPI_Read(YUSHAN_CLK_CTRL+1, 1, (uint8_t*)(&bSpiData)); */
	bSpiData |= 0x01;											/* Clk_Sys_Sel set to 1 */
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+1, 1, (uint8_t*)(&bSpiData));
	
	/* Enable the clocks */
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL, 1, (uint8_t*)(&udwSpiData));		/* First byte of the context */
	udwSpiData = (udwSpiData >> 16);										/* Third and fourth byte */
	fStatus &= SPI_Write(YUSHAN_CLK_CTRL+2, 2, (uint8_t*)(&udwSpiData));	


	/* Enable the MIPI RX and TX. */
	if ( sInitStruct->bNumberOfLanes == 1)
		udwSpiData=0x11; /* Enabling Clock Lane and Data Lane1 */
	else
	if ( sInitStruct->bNumberOfLanes == 2)
		udwSpiData=0x31; /* Enabling Clock Lane and Data Lane2 */
	else
	if ( sInitStruct->bNumberOfLanes == 4)
		udwSpiData=0xf1; /* Enabling Clock Lane and Data Lane3 */

	fStatus &= SPI_Write(YUSHAN_MIPI_RX_ENABLE, 1,(uint8_t*)(&udwSpiData)); 
	udwSpiData |= 0x02; /* Enable Clk HS */
	fStatus &= SPI_Write(YUSHAN_MIPI_TX_ENABLE, 1,(uint8_t*)(&udwSpiData));

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;

}




/******************************************************************
API Function:	This function will swap the P & N pins at CSI
				Receiver for the lanes which are passed as TRUE.
Input:			Clk_Lane, DataLane1, 2, 3 and 4.
Return:			Success or Failure
*******************************************************************/
bool_t Yushan_Swap_Rx_Pins (bool_t fClkLane, bool_t fDataLane1, bool_t fDataLane2, bool_t fDataLane3, bool_t fDataLane4)
{
	bool_t		fStatus; /* = SUCCESS; */
	uint8_t		bSpiData = 0;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	bSpiData = (fClkLane | (fDataLane1<<4) | (fDataLane2<<5) | (fDataLane3<<6) | (fDataLane4<<7));

	DEBUGLOG("[CAM] %s:Spi data for Swapping Rx pins is %4.4x\n", __func__, bSpiData);

	fStatus = SPI_Write(YUSHAN_MIPI_RX_SWAP_PINS, 1, &bSpiData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;

}



/******************************************************************
API Function:	This function will invert the HS signals of the CSI 
				Receiver for the lanes which are passed	as TRUE. This
				is an optional function which might	be required for 
				specific platforms only
Input:			Clk_Lane, DataLane1, 2, 3 and 4.
Return:			Success or Failure
*******************************************************************/
bool_t Yushan_Invert_Rx_Pins (bool_t fClkLane, bool_t fDataLane1, bool_t fDataLane2, bool_t fDataLane3, bool_t fDataLane4)
{

	bool_t		fStatus; /* = SUCCESS; */
	uint8_t		bSpiData = 0;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	bSpiData = (fClkLane | (fDataLane1<<4) | (fDataLane2<<5) | (fDataLane3<<6) | (fDataLane4<<7));
	VERBOSELOG("[CAM] %s:Spi data for Inverting Rx pins is %4.4x\n", __func__, bSpiData);

	fStatus = SPI_Write(YUSHAN_MIPI_RX_INVERT_HS, 1, &bSpiData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
	return fStatus;
}




/******************************************************************
API Function:	This function will reset the modules as per the input.
Input:			bModuleMask, 32bits Bitmask of Modules which are required 
				to be reset
Return:			Success or Failure
*******************************************************************/
bool_t Yushan_Assert_Reset(uint32_t udwModuleMask, uint8_t bResetORDeReset)
{
	uint8_t bCurrentModuleToReset, bCurrentModule=0;
	uint8_t	bSpiData;
	bool_t	bSetORReset;

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	if (!(udwModuleMask & 0xFFFFFFFF)) {
		ERRORLOG("[CAM] %s:Mask not given for reset/dereset. Returning FAILURE\n", __func__);
		return FAILURE;	/*********chk what to return *********/
	}

	while (bCurrentModule < TOTAL_MODULES) {

		bSetORReset = ((udwModuleMask>>bCurrentModule) & 0x01);

		/* Check if the current module has to be reset or not. */
		if (!bSetORReset) {
			bCurrentModule++;
			pr_info("[CAM] Current Module is %d\n", bCurrentModule);
			continue;
		} else {
			bCurrentModuleToReset = bCurrentModule + 1;
			bCurrentModule++;
		}


		/* Reset or Dereset the IPs */
		if (bResetORDeReset)
			/* Reset Data */
			bSpiData = 0x11;
		else
			/* Dereset data */
			bSpiData = 0x01;

		/* Debug */
		pr_info("[CAM] Current module is %d and Module to reset/Dereset is %d\n", bCurrentModule, bCurrentModuleToReset);
		
		/* Selectively reset the IPs */
		switch (bCurrentModuleToReset) {
#if 0
			case DFT_RESET:
				break;
#endif
			case T1_DMA:
				SPI_Write(YUSHAN_T1_DMA_REG_ENABLE, 1, &bSpiData);
				break;
			case CSI2_RX:
				/* Reset or Dereset the CSI2_Rx: Byte, Pixel Clk */
				if(bResetORDeReset)
					/* Reset Data */
					bSpiData = 0x70;
				else
					/* Dereset data */
					bSpiData = 0x00;

				SPI_Write(YUSHAN_CSI2_RX_ENABLE, 1, &bSpiData);
				
				/* Reinstating the values */
				if(bResetORDeReset)
					/* Reset Data */
					bSpiData = 0x11;
				else
					/* Dereset data */
					bSpiData = 0x01;
				break;
			case IT_POINT:
				SPI_Write(YUSHAN_ITPOINT_ENABLE, 1, &bSpiData);
				break;
			case IDP_GEN:

				/* Read for Auto_Run bit. */
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
				/* Reset or Dereset the P2W_FIFO_WR */
				if(bResetORDeReset)
					/* Reset Data */
					bSpiData = 0x03;
				else
					/* Dereset data */
					bSpiData = 0x00;

				SPI_Write(YUSHAN_P2W_FIFO_WR_CTRL, 1, &bSpiData);
				
				/* Reinstating the values */
				if(bResetORDeReset)
					/* Reset Data */
					bSpiData = 0x11;
				else
					/* Dereset data */
					bSpiData = 0x01;

				break;
			case P2W_FIFO_RD:
				/* Reset or Dereset the P2W_FIFO_RD */
				if(bResetORDeReset)
					/* Reset Data */
					bSpiData = 0x03;
				else
					/* Dereset data */
					bSpiData = 0x00;

				SPI_Write(YUSHAN_P2W_FIFO_RD_CTRL, 1, &bSpiData);
				
				/* Reinstating the values */
				if(bResetORDeReset)
					/* Reset Data */
					bSpiData = 0x11;
				else
					/* Dereset data */
					bSpiData = 0x01;

				break;
			case CSI2_TX_WRAPPER:
				if(bResetORDeReset)
					/* Reset Data */
					bSpiData = 0x01;
				else
					/* Dereset data */
					bSpiData = 0x00;

				SPI_Write(YUSHAN_CSI2_TX_WRAPPER_CTRL, 1, &bSpiData);
				
				/* Reinstating the values */
				if(bResetORDeReset)
					/* Reset Data */
					bSpiData = 0x11;
				else
					/* Dereset data */
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




/*******************************************************************
API Function:	Yushan Pattern Gen Test
*******************************************************************/
bool_t	Yushan_PatternGenerator(Yushan_Init_Struct_t *sInitStruct, uint8_t	bPatternReq, bool_t	bDxoBypassForTestPattern) 
{

	uint8_t		bSpiData, bDxoClkDiv, bPixClkDiv;
	uint32_t	udwSpiData=0, uwHSize, uwVSize;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	uwHSize = sInitStruct->uwActivePixels;
	uwVSize = sInitStruct->uwLines;

	/* CSI2_TX */
	sInitStruct->sFrameFormat[1].bDatatype = 0x12;
	sInitStruct->sFrameFormat[1].uwWordcount=(uwHSize)*(sInitStruct->uwPixelFormat&0x0F)/8;

	udwSpiData=sInitStruct->sFrameFormat[1].uwWordcount;
	SPI_Write(YUSHAN_CSI2_TX_PACKET_SIZE_0+0xc*1,4,(unsigned char*)&udwSpiData);

	udwSpiData=sInitStruct->sFrameFormat[1].bDatatype ;
	SPI_Write(YUSHAN_CSI2_TX_DI_INDEX_CTRL_0+0xc*1,1,(unsigned char*)&udwSpiData); 

	/* LECCI Bypass */
	/* udwSpiData=0x01; */
	SPI_Write(YUSHAN_LECCI_BYPASS_CTRL, 1, (unsigned char*)&bDxoBypassForTestPattern); 

	SPI_Read(YUSHAN_CLK_DIV_FACTOR,1,(unsigned char*)&bDxoClkDiv);
	SPI_Read(YUSHAN_CLK_DIV_FACTOR+1,1,(unsigned char*)&bPixClkDiv);
	//Min_interline = Freq_DXO/Freq_Pix*interline.
	if ( bDxoClkDiv !=0 && bPixClkDiv !=0 )
	  udwSpiData    = sInitStruct->uwLineBlankStill*bPixClkDiv/ bDxoClkDiv;
	else
	  udwSpiData    = 300;
	SPI_Write(YUSHAN_LECCI_MIN_INTERLINE, 2, (unsigned char*)&udwSpiData);

	/* H Size */
	udwSpiData = uwHSize;
	SPI_Write(YUSHAN_LECCI_LINE_SIZE, 2, (unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_LBE_PRE_DXO_H_SIZE, 2, (unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_LBE_POST_DXO_H_SIZE, 2, (unsigned char*)&udwSpiData);

	/* Pattern Selection */
	SPI_Write(YUSHAN_PATTERN_GEN_PATTERN_TYPE_REQ, 1, &bPatternReq);
	/* Program resolution for color bars:	IDP_Gen */
	udwSpiData = (uwHSize|((uwHSize+sInitStruct->uwLineBlankStill)<<16));
	SPI_Write(YUSHAN_IDP_GEN_LINE_LENGTH, 4, (uint8_t *)(&udwSpiData));
	udwSpiData = (uwVSize|(0xFFF0<<16));
	SPI_Write(YUSHAN_IDP_GEN_FRAME_LENGTH, 4, (uint8_t *)(&udwSpiData));

	/* For Color Bar mode: Disable DCPX in case of RAW10_8 */
	if(sInitStruct->uwPixelFormat == 0x0a08) {
		/* getch(); */
		bSpiData = 0x00;
		SPI_Write(YUSHAN_SMIA_DCPX_ENABLE, 1, &bSpiData); /* DCPX - DCPX_DISABLE */
	}

	/* prog required tpat_data for SOLID pattern */

	/* Enable Pattern Gen */
	bSpiData = 1;
	SPI_Write(YUSHAN_PATTERN_GEN_ENABLE, 1, &bSpiData);

	// Auto Run mode of Idp_Gen
	bSpiData = 1;
	SPI_Write(YUSHAN_IDP_GEN_AUTO_RUN, 1, &bSpiData);
	VERBOSELOG("[CAM] %s: End\n", __func__);
	return SUCCESS;

}




/*******************************************************************
API Function:	Yushan_DXO_Lecci_Bypass
*******************************************************************/
void	Yushan_DXO_Lecci_Bypass(void)
{
	uint8_t	bSpiData;

	VERBOSELOG("[CAM] %s: Start\n", __func__);
	/* LECCI Bypass */
	bSpiData=0x01;
	SPI_Write(YUSHAN_LECCI_BYPASS_CTRL, 1, (unsigned char*)&bSpiData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
}




/*******************************************************************
API Function:	Yushan_DXO_DTFilter_Bypass
*******************************************************************/
void	Yushan_DXO_DTFilter_Bypass(void)
{
	uint32_t	udwSpiData=0;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

	/* DT Filter */
	/***** DT Filter 0 ******/
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

	/***** DT Filter 1 ******/

	udwSpiData=0x5;
	SPI_Write(YUSHAN_DTFILTER_DXO_MATCH0,1,(unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_DTFILTER_DXO_MATCH2,1,(unsigned char*)&udwSpiData);
	SPI_Write(YUSHAN_DTFILTER_DXO_MATCH3,1,(unsigned char*)&udwSpiData);
	/*  udwSpiData=0x02; */
	udwSpiData=0x05;
	SPI_Write(YUSHAN_DTFILTER_DXO_MATCH1,1,(unsigned char*)&udwSpiData);
	udwSpiData=0x01;
	SPI_Write(YUSHAN_DTFILTER_DXO_ENABLE,1,(unsigned char*)&udwSpiData);

	VERBOSELOG("[CAM] %s: End\n", __func__);
	/* return SUCCESS; */
}



/*******************************************************************************
						STATUS FUNCTIONS
*******************************************************************************/



/******************************************************************
API Function:	This function will read the AF statistics based on the
				active ROIs
Input:			Pointer to the frst 32bit element of the ROI[0].
Return:			Success or Failure
*******************************************************************/
bool_t Yushan_Read_AF_Statistics(Yushan_AF_Stats_t* sYushanAFStats, uint8_t	bNumOfActiveRoi, uint16_t *frameIdx)
{

	uint8_t		bStatus = SUCCESS, bCount = 0;
	uint32_t	udwSpiData[4];
	uint16_t	val;

	VERBOSELOG("[CAM] %s: Start\n", __func__);

#if 0 /* debug usage */
			YushanPrintFrameNumber();
			YushanPrintDxODOPAfStrategy();
			YushanPrintImageInformation();
			YushanPrintVisibleLineSizeAndRoi();
#endif

#if 0
	/* Read active number of ROI */
	bStatus &= SPI_Read(DXO_DOP_BASE_ADDR+DxODOP_ROI_active_number_7_0, 1, (uint8_t*)(&bNumOfActiveRoi));
	/* pr_err("[CAM]%s, bNumOfActiveRoi:%d\n", __func__, bNumOfActiveRoi); */
#endif

	if (!bNumOfActiveRoi) /* NO ROI ACTIVATED */
		return SUCCESS;
	if (frameIdx != NULL) {
		SPI_Read(DXO_DOP_BASE_ADDR+DxODOP_frame_number_7_0, 2, (uint8_t *)(&val));
		*frameIdx= val;
	}

	/* bNumOfActiveRoi = 4; */
	/* Program the active number of ROIs */
	while (bCount < bNumOfActiveRoi) {
		/* Reading the stats from the DXO registers. */
		bStatus &= SPI_Read((uint16_t)(DXO_DOP_BASE_ADDR + DxODOP_ROI_0_stats_G_7_0 + bCount*16), 16, (uint8_t *)(&udwSpiData[0]));

		/* Copying the stats to the AF Stats struct */
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

		/* Counter increment */
		bCount++;
	}

	VERBOSELOG("[CAM] %s: End With Status: %i\n", __func__, bStatus);
	return bStatus;

}



/******************************************************************
API Function:	This function will read current status and configuration
				for all Yushan Blocks and dump it in log
				DEBUG MODE at least must de enabled
Input:
Return:
*******************************************************************/
#define YUSHAN_REGISTER_CHECK(reg_addr) udwSpiData = 0; SPI_Read(reg_addr, 4, (uint8_t *)(&udwSpiData)); pr_info("[CAM] %s: %s: 0x%08x\n", __func__, #reg_addr, udwSpiData);
#define YUSHAN_DOP_REGISTER_CHECK(reg_addr) udwSpiData = 0; SPI_Read(reg_addr + DXO_DOP_BASE_ADDR, 1, (uint8_t *)(&udwSpiData)); pr_info("[CAM] %s: %s: 0x%02x\n", __func__, #reg_addr, udwSpiData);
#define YUSHAN_PDP_REGISTER_CHECK(reg_addr) udwSpiData = 0; SPI_Read(reg_addr + DXO_PDP_BASE_ADDR, 1, (uint8_t *)(&udwSpiData)); pr_info("[CAM] %s: %s: 0x%02x\n", __func__, #reg_addr, udwSpiData);
#define YUSHAN_DPP_REGISTER_CHECK(reg_addr) udwSpiData = 0; SPI_Read(reg_addr + DXO_DPP_BASE_ADDR-0x8000, 1, (uint8_t *)(&udwSpiData)); pr_info("[CAM] %s: %s: 0x%02x\n", __func__, #reg_addr, udwSpiData);
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

	VERBOSELOG("[CAM] %s: End\n", __func__);
}

