/*******************************************************************************
################################################################################
#                             (C) STMicroelectronics 2012
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License version 2 and only version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
#------------------------------------------------------------------------------
#                             Imaging Division
################################################################################
********************************************************************************/



#include "ilp0100_ST_core.h"
#include "rawchip_spi.h"
#include <linux/delay.h>
#include "ilp0100_customer_iq_settings.h"

bool_t		PllLocked		=	0;
uint32_t 	SpiWin0BaseAddr	=	0x00000000;


ilp0100_error Ilp0100_init(const Ilp0100_structInit Init, Ilp0100_structInitFirmware InitFirmware, const Ilp0100_structSensorParams SensorParams)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;
	uint32_t IntrClrMask=0xFFFFFFFF;

	ILP0100_LOG_FUNCTION_START((void*)&Init, (void*)&InitFirmware, (void*)&SensorParams);

	if(InitFirmware.pIlp0100Firmware!=NULL) 
	{
		Ret= Ilp0100_core_initClocks(Init,SensorParams); 	
		if (Ret!=ILP0100_ERROR_NONE)
			{
			ILP0100_ERROR_LOG("Ilp0100_core_initClocks failed");
			ILP0100_LOG_FUNCTION_END(Ret);
			return Ret; 
			}

		Ret= Ilp0100_core_uploadFirmware(InitFirmware); 	
		if (Ret!=ILP0100_ERROR_NONE)
			{
			ILP0100_ERROR_LOG("Ilp0100_core_uploadFirmware failed");
			ILP0100_LOG_FUNCTION_END(Ret);
			return Ret; 
			}

		Ret= Ilp0100_core_bootXp70();		
		if (Ret!=ILP0100_ERROR_NONE)
			{
			ILP0100_ERROR_LOG("Ilp0100_core_bootXp70 failed");
			ILP0100_LOG_FUNCTION_END(Ret);
			return Ret; 
			}
	}

	
	if(Init.UsedSensorInterface==SENSOR_0)
	{
		Ilp0100_core_disableIlp0100SensorClock(SENSOR_1);
    Ilp0100_core_enableIlp0100SensorClock(SENSOR_0);
	}
	if(Init.UsedSensorInterface==SENSOR_1)
	{
		Ilp0100_core_disableIlp0100SensorClock(SENSOR_0);
    Ilp0100_core_enableIlp0100SensorClock(SENSOR_1);
	}


	
	Ilp0100_core_saveSpiWindow1Addr(&Addr); 

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_e_ImageFormat_Output, 1, (uint8_t*)&Init.uwPixelFormat);
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_e_ImageFormat_Input, 1, (uint8_t*)&Init.uwPixelFormat);


	
	if(Init.UsedSensorInterface==SENSOR_0||Init.UsedSensorInterface==SENSOR_1)
	{
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_SensorSelected, 1, (uint8_t*)&Init.UsedSensorInterface);
	}
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_u8_CSIRxDataLaneCount, 1, (uint8_t*)&Init.NumberOfLanes);
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_u32_SensorDataRate_bps_Byte_0, 4, (uint8_t*)&Init.BitRate);
	
	
	
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_u32_MaxOutputDataRate_bps_Byte_0, 4, (uint8_t*)&Init.BitRate);
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_u32_ExternalClockFrequency_hz_Byte_0, 4, (uint8_t*)&Init.ExternalClock);
	
	
	
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+CSI2Tx_Control_u8_NumberofLanes, 1, (uint8_t*)&Init.NumberOfLanes);

	
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Rubik_Input_u16_VSensorSize_Byte_0, 2, (uint8_t*)&SensorParams.FullActiveLines);
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Rubik_Input_u16_HSensorSize_Byte_0, 2, (uint8_t*)&SensorParams.FullActivePixels);
	
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_e_PixelOrder, 1, (uint8_t*)&SensorParams.PixelOrder);
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_u8_InputNbStatusLines, 1, (uint8_t*)&SensorParams.StatusNbLines);

	
	Ret= Ilp0100_core_interruptDisable(IntrClrMask, INTR_PIN_0);
	if (Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_ERROR_LOG("Ilp0100_core_interruptDisable INTR_PIN_0 failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	Ret= Ilp0100_core_interruptDisable(IntrClrMask, INTR_PIN_1);
	if (Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_ERROR_LOG("Ilp0100_core_interruptDisable INTR_PIN_1 failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	
	Ret= Ilp0100_core_interruptEnable(Init.IntrEnablePin1, INTR_PIN_0);
	if (Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_ERROR_LOG("Ilp0100_core_interruptEnable INTR_PIN_0 failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	
	Ret= Ilp0100_core_interruptEnable(Init.IntrEnablePin2, INTR_PIN_1);
	if (Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_ERROR_LOG("Ilp0100_core_interruptEnable INTR_PIN_1 failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_defineMode(const Ilp0100_structFrameFormat FrameFormat)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&FrameFormat);
	Ret=Ilp0100_core_defineMode(FrameFormat);
	if(Ret!= ILP0100_ERROR_NONE)
	{
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_stop()
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START();

	Ret=Ilp0100_core_stop();
	if(Ret!= ILP0100_ERROR_NONE)
	{
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}
	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_configDefcorShortOrNormal(const Ilp0100_structDefcorConfig ShortOrNormalDefcorConfig)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&ShortOrNormalDefcorConfig);

	Ret=Ilp0100_core_configDefcor(ShortOrNormalDefcorConfig, NORMAL_OR_HDR_SHORT);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_configDefcor failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_configDefcorLong(const Ilp0100_structDefcorConfig LongDefcorConfig)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&LongDefcorConfig);

	Ret=Ilp0100_core_configDefcor(LongDefcorConfig, HDR_LONG);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_configDefcor failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_updateDefcorShortOrNormal(const Ilp0100_structDefcorParams ShortOrNormalDefcorParams)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&ShortOrNormalDefcorParams);

	Ret=Ilp0100_core_updateDefcor(ShortOrNormalDefcorParams, NORMAL_OR_HDR_SHORT);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_updateDefcor failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_updateDefcorLong(const Ilp0100_structDefcorParams LongDefcorParams)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&LongDefcorParams);

	Ret=Ilp0100_core_updateDefcor(LongDefcorParams, HDR_LONG);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_updateDefcor failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}


ilp0100_error Ilp0100_configChannelOffsetShortOrNormal(const Ilp0100_structChannelOffsetConfig ShortOrNormalChannelOffsetConfig)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&ShortOrNormalChannelOffsetConfig);

	Ret=Ilp0100_core_configChannelOffset(ShortOrNormalChannelOffsetConfig, NORMAL_OR_HDR_SHORT);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_configChannelOffset failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}
ilp0100_error Ilp0100_configChannelOffsetLong(const Ilp0100_structChannelOffsetConfig LongChannelOffsetConfig)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&LongChannelOffsetConfig);

	Ret=Ilp0100_core_configChannelOffset(LongChannelOffsetConfig, HDR_LONG);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_configChannelOffset failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}
ilp0100_error Ilp0100_updateChannelOffsetShortOrNormal(const Ilp0100_structChannelOffsetParams ShortOrNormalChannelOffsetParams)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&ShortOrNormalChannelOffsetParams);

	Ret=Ilp0100_core_updateChannelOffset(ShortOrNormalChannelOffsetParams, NORMAL_OR_HDR_SHORT);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_updateChannelOffset failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}
ilp0100_error Ilp0100_updateChannelOffsetLong(const Ilp0100_structChannelOffsetParams LongChannelOffsetParams)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&LongChannelOffsetParams);

	Ret=Ilp0100_core_updateChannelOffset(LongChannelOffsetParams, HDR_LONG);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_updateChannelOffset failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_configGlaceShortOrNormal(const Ilp0100_structGlaceConfig ShortOrNormalGlaceConfig)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&ShortOrNormalGlaceConfig);

	Ret=Ilp0100_core_configGlace(ShortOrNormalGlaceConfig, NORMAL_OR_HDR_SHORT);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_configGlace failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_configGlaceLong(const Ilp0100_structGlaceConfig LongGlaceConfig)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&LongGlaceConfig);

	Ret=Ilp0100_core_configGlace(LongGlaceConfig, HDR_LONG);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_configGlace failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_configHistShortOrNormal(const Ilp0100_structHistConfig ShortOrNormalHistConfig)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&ShortOrNormalHistConfig);

	Ret=Ilp0100_core_configHist(ShortOrNormalHistConfig, NORMAL_OR_HDR_SHORT);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_configHist failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_configHistLong(const Ilp0100_structHistConfig LongHistConfig)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&LongHistConfig);

	Ret=Ilp0100_core_configHist(LongHistConfig, HDR_LONG);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_configHist failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_configHdrMerge(const Ilp0100_structHdrMergeConfig HdrMergeConfig)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&HdrMergeConfig);

	Ret=Ilp0100_core_configHdrMerge(HdrMergeConfig);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_configHdrMerge failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_updateHdrMerge(const Ilp0100_structHdrMergeParams HdrMergeParams)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&HdrMergeParams);

	Ret=Ilp0100_core_updateHdrMerge(HdrMergeParams);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_updateHdrMerge failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}


ilp0100_error Ilp0100_configCls(const Ilp0100_structClsConfig ClsConfig)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&ClsConfig);

	Ret=Ilp0100_core_configCls(ClsConfig);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_configCls failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_updateCls(const Ilp0100_structClsParams ClsParams)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&ClsParams);

	Ret=Ilp0100_core_updateCls(ClsParams);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_updateCls failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_configToneMapping(const Ilp0100_structToneMappingConfig ToneMappingConfig)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&ToneMappingConfig);

	Ret=Ilp0100_core_configToneMapping(ToneMappingConfig);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_configToneMapping failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_updateToneMapping(const Ilp0100_structToneMappingParams ToneMappingParams)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&ToneMappingParams);

	Ret=Ilp0100_core_updateToneMapping(ToneMappingParams);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_updateToneMapping failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_updateSensorParamsShortOrNormal(const Ilp0100_structFrameParams ShortOrNormalFrameParams)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&ShortOrNormalFrameParams);

	Ret=Ilp0100_core_updateSensorParams(ShortOrNormalFrameParams, NORMAL_OR_HDR_SHORT); 
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_updateSensorParams failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_updateSensorParamsLong(const Ilp0100_structFrameParams LongFrameParams)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&LongFrameParams);

	Ret=Ilp0100_core_updateSensorParams(LongFrameParams, HDR_LONG); 
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_updateSensorParams failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_readBackGlaceStatisticsShortOrNormal(Ilp0100_structGlaceStatsData *pShortOrNormalGlaceStatsData)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)pShortOrNormalGlaceStatsData);

	Ret=Ilp0100_core_readBackGlaceStatistics(pShortOrNormalGlaceStatsData, NORMAL_OR_HDR_SHORT); 
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_readBackGlaceStatistics failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_readBackGlaceStatisticsLong(Ilp0100_structGlaceStatsData *pLongGlaceStatsData)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)pLongGlaceStatsData);

	Ret=Ilp0100_core_readBackGlaceStatistics(pLongGlaceStatsData, HDR_LONG); 
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_readBackGlaceStatistics failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_readBackHistStatisticsShortOrNormal(Ilp0100_structHistStatsData *pShortOrNormalHistStatsData)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)pShortOrNormalHistStatsData);

	Ret=Ilp0100_core_readBackHistStatistics(pShortOrNormalHistStatsData, NORMAL_OR_HDR_SHORT); 
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_readBackHistStatistics failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_readBackHistStatisticsLong(Ilp0100_structHistStatsData *pLongHistStatsData)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)pLongHistStatsData);

	Ret=Ilp0100_core_readBackHistStatistics(pLongHistStatsData, HDR_LONG); 
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_readBackHistStatistics failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_setVirtualChannelShortOrNormal(uint8_t VirtualChannel)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&VirtualChannel);

	Ret=Ilp0100_core_setVirtualChannel(VirtualChannel, NORMAL_OR_HDR_SHORT); 
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_setVirtualChannel failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_setVirtualChannelLong(uint8_t VirtualChannel)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&VirtualChannel);

	Ret=Ilp0100_core_setVirtualChannel(VirtualChannel, HDR_LONG); 
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_setVirtualChannel failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_setHDRFactor(uint8_t HDRFactor)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&HDRFactor);

	Ret=Ilp0100_core_setHDRFactor(HDRFactor);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("ilp0100_core_setHDRFactor failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_reset()
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START(NULL);
	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_onTheFlyReset()
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START(NULL);
	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}



ilp0100_error Ilp0100_interruptEnable(uint32_t InterruptSetMask, bool_t Pin)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&InterruptSetMask, (void*)&Pin);

	Ret=Ilp0100_core_interruptEnable(InterruptSetMask, Pin);
	if(Ret!= ILP0100_ERROR_NONE)
	{
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_interruptDisable(uint32_t InterruptClrMask, bool_t Pin)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&InterruptClrMask, (void*)&Pin);

	Ret=Ilp0100_core_interruptDisable(InterruptClrMask, Pin);
	if(Ret!= ILP0100_ERROR_NONE)
	{
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_interruptReadStatus(uint32_t* pInterruptId, uint8_t Pin)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)pInterruptId, (void*)&Pin);

	Ret=Ilp0100_core_interruptReadStatus(pInterruptId, Pin);
	if(Ret!= ILP0100_ERROR_NONE)
	{
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_interruptClearStatus(uint32_t InterruptId, uint8_t Pin)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&InterruptId, (void*)&Pin);

	Ret=Ilp0100_core_interruptClearStatus(InterruptId, Pin);
	if(Ret!= ILP0100_ERROR_NONE)
	{
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_getApiVersionNumber(uint8_t* pMajorNumber, uint8_t* pMinorNumber)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)pMajorNumber, (void*)pMinorNumber);


	Ret=Ilp0100_core_getApiVersionNumber(pMajorNumber, pMinorNumber);
	if (Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_ERROR_LOG("Ilp0100_core_getApiVersionNumber failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_getFirmwareVersionumber(uint8_t* pMajorNumber, uint8_t* pMinorNumber)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)pMajorNumber, (void*)pMinorNumber);

	Ret=Ilp0100_core_getFirmwareVersionumber(pMajorNumber, pMinorNumber);
	if (Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}


ilp0100_error Ilp0100_startTestMode(uint16_t IpsBypass, uint16_t TestMode)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&IpsBypass, (void*)&TestMode);

	Ret=Ilp0100_core_startTestMode(IpsBypass, TestMode);
	if (Ret!=ILP0100_ERROR_NONE)
		{
		ILP0100_ERROR_LOG("Ilp0100_core_startTestMode failed");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
		}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_readRegister(uint32_t RegisterName, uint8_t *pData)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t Addr;
	uint32_t Offset;
	uint16_t SpiWindow0MaxAddr=0x4000;			
	uint32_t HwRegistersMinAddr=0x800000;		
	uint32_t HwRegistersMaxAddr=0x808000;		

	ILP0100_LOG_FUNCTION_START((void*)&RegisterName, (void*)pData);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	
	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_0, 4, (uint8_t*)&SpiWin0BaseAddr);


	if(RegisterName<SpiWindow0MaxAddr)
	{
		Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+RegisterName, 4, (uint8_t*)pData);
	}
	else if((RegisterName>=HwRegistersMinAddr)&&(RegisterName<HwRegistersMaxAddr))
	{
		Ret|= Ilp0100_core_readRegister((uint16_t)RegisterName, 4, (uint8_t*)pData);
	}
	else
	{
		Offset=RegisterName&0xFFFFFFFC; 
		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&Offset);
		Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_1+(RegisterName&0x3), 4, (uint8_t*)pData);
	}

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_writeRegister(uint32_t RegisterName, uint8_t *pData)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t Addr;
	uint32_t Offset;
	uint16_t SpiWindow0MaxAddr=0x4000; 		
	uint32_t HwRegistersMinAddr=0x800000;	
	uint32_t HwRegistersMaxAddr=0x808000;	

	ILP0100_LOG_FUNCTION_START((void*)&RegisterName, (void*)pData);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	
	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_0, 4, (uint8_t*)&SpiWin0BaseAddr);

	if(RegisterName<SpiWindow0MaxAddr)
	{
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+RegisterName, 4, (uint8_t*)pData);
	}
	else if((RegisterName>=HwRegistersMinAddr)&&(RegisterName<HwRegistersMaxAddr))
	{
		Ret|= Ilp0100_core_writeRegister((uint16_t)RegisterName, 4, (uint8_t*)pData);
	}
	else
	{
		Offset=RegisterName&0xFFFFFFFC; 
		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&Offset);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_1+(RegisterName&0x3), 4, (uint8_t*)pData);
	}

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
	return Ret;

}

ilp0100_error Ilp0100_enableIlp0100SensorClock(bool_t SensorInterface)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&SensorInterface);

	Ret=Ilp0100_core_enableIlp0100SensorClock(SensorInterface);
	if (Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_disableIlp0100SensorClock(bool_t SensorInterface)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&SensorInterface);

	Ret=Ilp0100_core_disableIlp0100SensorClock(SensorInterface);
	if (Ret!=ILP0100_ERROR_NONE)
	{
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}


ilp0100_error Ilp0100_core_initClocks(const Ilp0100_structInit Init, const Ilp0100_structSensorParams SensorParams)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	
	uint8_t PllDiv, PllLoop, PllOdf, SpiData, Div;
	uint16_t i; 

	ILP0100_LOG_FUNCTION_START((void*)&Init, (void*)&SensorParams);

	
	
	PllDiv=1;
	PllLoop=25;
	
	PllOdf=4;

	
	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_PLL_CTRL_MAIN+1, 1, (uint8_t*)&PllDiv);
	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_PLL_LOOP_OUT_DF+1, 1, (uint8_t*)&PllLoop);
	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_PLL_LOOP_OUT_DF, 1, (uint8_t*)&PllOdf);

	
	ILP0100_DEBUG_LOG("Enable Pll");
    SpiData=0x18;
    Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_PLL_CTRL_MAIN, 1, &SpiData);

	
	
	for(i=0;i<1000;i++) 
	{
	
	SpiData=0x00;
	Ret|= Ilp0100_core_readRegister((uint16_t)ILP0100_PLL_CTRL_MAIN+2, 1, &SpiData);
	
	if((SpiData & 0x01) == 0x01)
	{
		ILP0100_DEBUG_LOG("Pll is locked");
		PllLocked=1;
		break;
	}
	}

	if(i==1000)
	{
	
	ILP0100_ERROR_LOG("PLL Lock Failed");
	Ret=ILP0100_ERROR;
	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
	}

	

	

	
	Div=1;
	Div<<=1; 
	
	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_CLK_DIV_FACTOR+1, 1, (uint8_t*)&Div);


	
	ILP0100_DEBUG_LOG("Set PLL_lock in ILP0100_CLK_CTRL");
	SpiData=0x00;
	Ret|= Ilp0100_core_readRegister((uint16_t)ILP0100_CLK_CTRL+1, 1, &SpiData);


	SpiData |= 0x01; 
	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_CLK_CTRL+1, 1, &SpiData);


	
	ILP0100_DEBUG_LOG("Bypass PLL");
	SpiData &= 0xFD;
	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_CLK_CTRL+1, 1, &SpiData);


	
	ILP0100_DEBUG_LOG("Pll Clk generates Clk sys");
	SpiData=0x00;
	Ret|= Ilp0100_core_readRegister((uint16_t)ILP0100_CLK_CTRL+2, 1, &SpiData);

	SpiData |= 0x01;
	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_CLK_CTRL+2, 1, &SpiData);

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_core_uploadFirmware( Ilp0100_structInitFirmware InitFirmware)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	file_hdr_t FileHdr[3];
	uint32_t SpiWindowSize= 16*1024;
	uint32_t SpiWin1BaseAddr;
	uint16_t RemainingData;
	uint32_t Addr;
	uint8_t MagicId1[]="STCO", MagicId2[]="STDA"; 
	uint8_t Data;
	bool_t Part2PartCalibdataPresent;


	ILP0100_LOG_FUNCTION_START((void*)&InitFirmware);

	
	FileHdr[0].magic_id= *(uint32_t*)InitFirmware.pIlp0100Firmware;
	FileHdr[0].load_add= *(uint32_t*)(InitFirmware.pIlp0100Firmware+4);
	FileHdr[0].code_size= *(uint32_t*)(InitFirmware.pIlp0100Firmware+8);
	FileHdr[0].crc= *(uint16_t*)(InitFirmware.pIlp0100Firmware+12);
	FileHdr[0].file_offset= *(uint16_t*)(InitFirmware.pIlp0100Firmware+14);

	
	FileHdr[1].magic_id= *(uint32_t*)InitFirmware.pIlp0100SensorGenericCalibData;
	FileHdr[1].load_add= *(uint32_t*)(InitFirmware.pIlp0100SensorGenericCalibData+4);
	FileHdr[1].code_size= *(uint32_t*)(InitFirmware.pIlp0100SensorGenericCalibData+8);
	FileHdr[1].crc= *(uint16_t*)(InitFirmware.pIlp0100SensorGenericCalibData+12);
	FileHdr[1].file_offset= *(uint16_t*)(InitFirmware.pIlp0100SensorGenericCalibData+14);

	
	
	if(InitFirmware.Ilp0100SensorRawPart2PartCalibDataSize!=0) {
		Part2PartCalibdataPresent=TRUE;
		FileHdr[2].magic_id= 0;
		FileHdr[2].load_add= 0x7C00;
		FileHdr[2].code_size= InitFirmware.Ilp0100SensorRawPart2PartCalibDataSize- sizeof(FileHdr[2].crc);
		FileHdr[2].crc= *(uint16_t*)InitFirmware.pIlp0100SensorRawPart2PartCalibData;
		FileHdr[2].file_offset= 0x0002;
	} else {
		Part2PartCalibdataPresent=FALSE;
		FileHdr[2].magic_id= 0;
		FileHdr[2].load_add= 0;
		FileHdr[2].code_size= 0;
		FileHdr[2].crc= 0;
		FileHdr[2].file_offset= 0;
	}


	

	if(!((FileHdr[0].magic_id==*(uint32_t*)MagicId1)||(FileHdr[0].magic_id==*(uint32_t*)MagicId2)))
	{
		Ret=ILP0100_ERROR;
		ILP0100_ERROR_LOG("Firmware Code Binary Magic Id is wrong");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;

	}

	
	if(!((FileHdr[1].magic_id==*(uint32_t*)MagicId1)||(FileHdr[1].magic_id==*(uint32_t*)MagicId2)))
	{
		Ret=ILP0100_ERROR;
		ILP0100_ERROR_LOG("Firmware Data Binary Magic Id is wrong");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	
	if(InitFirmware.Ilp0100FirmwareSize!=(FileHdr[0].code_size+ sizeof(FileHdr[0])))
	{
		Ret=ILP0100_ERROR;
		ILP0100_ERROR_LOG("Firmware Code Binary Size is not as expected");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	
	if(InitFirmware.Ilp0100SensorGenericCalibDataSize!=(FileHdr[1].code_size+ sizeof(FileHdr[1])))
	{
		Ret=ILP0100_ERROR;
		ILP0100_ERROR_LOG("Firmware Data Binary Size is not as expected");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	
	InitFirmware.pIlp0100Firmware += FileHdr[0].file_offset;

	

	
	Ret=Ilp0100_core_checkCrc(InitFirmware.pIlp0100Firmware, FileHdr[0].code_size, FileHdr[0].crc);
	if (Ret!= ILP0100_ERROR_NONE)
	{
		ILP0100_ERROR_LOG("\n Error in CRC calculation of firmware data");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}


	
	InitFirmware.pIlp0100SensorGenericCalibData += FileHdr[1].file_offset;

	
	Ret=Ilp0100_core_checkCrc(InitFirmware.pIlp0100SensorGenericCalibData, FileHdr[1].code_size, FileHdr[1].crc);
	if (Ret!= ILP0100_ERROR_NONE)
	{
		ILP0100_ERROR_LOG("\n Error in CRC calculation of Calib Data");
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}

	
	InitFirmware.pIlp0100SensorRawPart2PartCalibData += FileHdr[2].file_offset;

	
	if(Part2PartCalibdataPresent==TRUE)
	{
		Ret=Ilp0100_core_checkCrc(InitFirmware.pIlp0100SensorRawPart2PartCalibData, FileHdr[2].code_size, FileHdr[2].crc);
		if (Ret!= ILP0100_ERROR_NONE)
		{
			ILP0100_ERROR_LOG("\n Error in CRC calculation of Part to Part Calib Data");
			ILP0100_LOG_FUNCTION_END(Ret);
			return Ret;
		}
	}

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);
	
	
	ILP0100_DEBUG_LOG("Download Firmware code Start");

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&FileHdr[0].load_add);

	
	if (FileHdr[0].code_size <= SpiWindowSize)
	{
		ILP0100_DEBUG_LOG("Write bytes %d",FileHdr[0].code_size);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_1, FileHdr[0].code_size, (uint8_t*)InitFirmware.pIlp0100Firmware);
	}
	else
	{	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_1, SpiWindowSize, (uint8_t*)InitFirmware.pIlp0100Firmware);
		SpiWin1BaseAddr=FileHdr[0].load_add+0x4000; 
		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);  
		RemainingData= FileHdr[0].code_size-SpiWindowSize;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_1, RemainingData, (uint8_t*)(InitFirmware.pIlp0100Firmware+0x4000));
	}

	if(Ret == ILP0100_ERROR)
	{
		ILP0100_ERROR_LOG("\n Error in firmware download writes");
	}
	ILP0100_DEBUG_LOG("Download Firmware code Done");

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	
	ILP0100_DEBUG_LOG("Download Calib Data Start");

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_0, 4, (uint8_t*)&FileHdr[1].load_add);

	
	if (FileHdr[1].code_size <= SpiWindowSize)
	{
		ILP0100_DEBUG_LOG("Write bytes %d",FileHdr[1].code_size);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0, FileHdr[1].code_size, (uint8_t*)InitFirmware.pIlp0100SensorGenericCalibData);
	}
	else
	{	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0, SpiWindowSize, (uint8_t*)InitFirmware.pIlp0100SensorGenericCalibData);
		SpiWin0BaseAddr=FileHdr[1].load_add+0x4000; 
		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_0, 4, (uint8_t*)&SpiWin0BaseAddr);  
		RemainingData= FileHdr[1].code_size-SpiWindowSize;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0, RemainingData, (uint8_t*)(InitFirmware.pIlp0100SensorGenericCalibData+0x4000));
	}

	if(Ret == ILP0100_ERROR)
	{
		ILP0100_ERROR_LOG("\n Error in Calib Data download writes");
	}
	ILP0100_DEBUG_LOG("Download Calib Data Done");

	
	if(Part2PartCalibdataPresent==TRUE)
	{
		ILP0100_DEBUG_LOG("Download Part to Part Calib Data Start");

		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_0, 4, (uint8_t*)&FileHdr[2].load_add);

		
		if (FileHdr[2].code_size <= SpiWindowSize)
		{
			ILP0100_DEBUG_LOG("Write bytes %d",FileHdr[2].code_size);
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0, FileHdr[2].code_size, (uint8_t*)InitFirmware.pIlp0100SensorRawPart2PartCalibData);
		}
		else
		{	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0, SpiWindowSize, (uint8_t*)InitFirmware.pIlp0100SensorRawPart2PartCalibData);
			SpiWin0BaseAddr=FileHdr[2].load_add+0x4000; 
			Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_0, 4, (uint8_t*)&SpiWin0BaseAddr);  
			RemainingData= FileHdr[2].code_size-SpiWindowSize;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0, RemainingData, (uint8_t*)(InitFirmware.pIlp0100SensorRawPart2PartCalibData+0x4000));
		}

		if(Ret == ILP0100_ERROR)
		{
			ILP0100_ERROR_LOG("\n Error in Part to Part Calib Data download writes");
		}
		ILP0100_DEBUG_LOG("Download Part to Part Calib Data Done");
	}


	
	SpiWin0BaseAddr=0x00000000;
	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_0, 4, (uint8_t*)&SpiWin0BaseAddr);

	SpiWin1BaseAddr=0x00004000;

		
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	
	Data=RubikTop_BufferState_e_DOWNLOADED;
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+RubikTop_Control_e_BufferState, 1, (uint8_t*)&Data);

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_core_checkCrc(uint8_t* pData, uint32_t SizeInBytes, uint16_t CrcExpected)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint16_t Crc=0xFFFF;		
	uint16_t Poly=0x8408;		
	uint16_t Data;
	uint32_t i;

	ILP0100_LOG_FUNCTION_START((void*)pData, (void*)&SizeInBytes, (void*)&CrcExpected);

	
	if (SizeInBytes==0){
		ILP0100_LOG_FUNCTION_END(Ret);
	{
		Ret= ILP0100_ERROR;
		ILP0100_LOG_FUNCTION_END(Ret);
		return Ret;
	}
	}
	do{
		for (i=0,Data=(unsigned int)0xff & *pData++; i<8; i++, Data>>=1)
		{
			if((Crc&0x0001) ^ (Data &0x0001))
			Crc = (Crc >>1) ^ Poly;
			else
				Crc >>= 1;
		}
	} while (--SizeInBytes);

	Crc=~Crc;
	Data= Crc;
	Crc=(Crc << 8) | (Data >>8 & 0xFF);

	if (Crc!=CrcExpected)		
	{
		ILP0100_ERROR_LOG("\n Crc mistached. Calculated Crc is %x", Crc);
		ILP0100_LOG_FUNCTION_END(Ret);
		return ILP0100_ERROR;
	}
	else
	{
		ILP0100_DEBUG_LOG("\n Crc match successful.");	
	}

	ILP0100_LOG_FUNCTION_END(Ret);

	return Ret;

}



ilp0100_error Ilp0100_core_bootXp70()
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint8_t SpiData;


	ILP0100_LOG_FUNCTION_START(NULL);

	SpiData=0x01;
	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_PERIPH_MCU_CTRL, 1, (uint8_t*)&SpiData);


	ILP0100_LOG_FUNCTION_END(Ret);

	return Ret;

}

ilp0100_error Ilp0100_core_defineMode(const Ilp0100_structFrameFormat FrameFormat)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint8_t ModeStatus, ModeControl, ModeChange, SystemError, TestPatternInputInterface, UseInputStatusLines, Flag;
	uint32_t SpiData;
	uint8_t Delay, PixelOrder, SmiaRXPixelOrder;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;
	uint16_t ExpTime;
	uint16_t OvrData;
	uint32_t InterruptId;
	uint8_t InputNbStatusLines;
	uint8_t ILPSOFLines,LinesCnvtToSOF;

	ILP0100_LOG_FUNCTION_START((void*)&FrameFormat);
	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	
	TestPatternInputInterface=HIF_InputImageSource_e_SENSOR;
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_e_InputImageSource,1 ,(uint8_t*)&TestPatternInputInterface);


	
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_u16_CustomImageSizeX_Byte_0, 2, (uint8_t*)&FrameFormat.ActiveLineLengthPixels);
	
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_u16_CustomImageSizeY_Byte_0, 2, (uint8_t*)&FrameFormat.ActiveFrameLengthLines);
	
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_FrameDimension_u16_FrameLength_Byte_0, 2, (uint8_t*)&FrameFormat.FrameLengthLines);
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_FrameDimension_u16_LineLength_Byte_0, 2, (uint8_t*)&FrameFormat.LineLengthPixels);

	Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_u8_InputNbStatusLines, 1, (uint8_t*)&InputNbStatusLines);


	
	
	if(InputNbStatusLines)
	{
		UseInputStatusLines = InputInterface_StatusLine_e_ENABLED_BUT_OVERRIDEN;
		ILPSOFLines = InputNbStatusLines;
		LinesCnvtToSOF = 0;
	}
	else
	{
		ILPSOFLines = 2;
		if(FrameFormat.StatusNbLines!=0)
		{
			ILPSOFLines=FrameFormat.StatusNbLines;
		}
		LinesCnvtToSOF = ILPSOFLines;
		UseInputStatusLines = InputInterface_StatusLine_e_DISABLED;
	}
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_StatusLine_0_e_StatusLine_Control, 1, (uint8_t*)&UseInputStatusLines);
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_StatusLine_1_e_StatusLine_Control, 1, (uint8_t*)&UseInputStatusLines);

	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_u16_StatusLineLengthPixels_Byte_0, 2, (uint8_t*)&FrameFormat.StatusLineLengthPixels);

	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_u8_NoStatusLinesOutputted, 1, (uint8_t*)&FrameFormat.StatusNbLines);
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_e_Flag_StatusLinesOutputted, 1, (uint8_t*)&FrameFormat.StatusLinesOutputted);
	
	



	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_u16_MinInterframeAtOutput_Byte_0, 2, (uint8_t*)&FrameFormat.MinInterframe);
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_e_Flag_AutoFrameParamUpdate, 1, (uint8_t*)&FrameFormat.AutomaticFrameParamsUpdate);
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_e_Orientation_FlipXY, 1, (uint8_t*)&FrameFormat.ImageOrientation);

	
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_Flag_HDRMode, 1, (uint8_t*)&FrameFormat.HDRMode);

	SpiData=CRMTop_Flag_e_TRUE;
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+CRMTop_Control_e_Flag_AutoPixClockMode, 1, (uint8_t*)&SpiData);

	
	Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_e_PixelOrder, 1, (uint8_t*)&PixelOrder);
	
	SmiaRXPixelOrder = PixelOrder ^ FrameFormat.ImageOrientation;
	
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_FrameDimension_1_e_PixelOrder, 1, (uint8_t*)&SmiaRXPixelOrder);

	if(FrameFormat.HDRMode)
	{
		
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_FrameDimension_0_e_PixelOrder, 1, (uint8_t*)&SmiaRXPixelOrder);

		ExpTime=4;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_Exposure_0_u16_CurrentExposureTime_Byte_0, 2, (uint8_t*)&ExpTime);
		ExpTime=1;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_Exposure_1_u16_CurrentExposureTime_Byte_0, 2, (uint8_t*)&ExpTime);

		OvrData=0x12;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+SMIARx_Control_0_u8_FFModelSubtypeOverride, 1, (uint8_t*)&OvrData);
		OvrData=0xa000 |FrameFormat.ActiveLineLengthPixels;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+SMIARx_Control_0_u16_FFormatDescriptorOverride0_Byte_0, 2, (uint8_t*)&OvrData);
		OvrData=0x2000 |ILPSOFLines;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+SMIARx_Control_0_u16_FFormatDescriptorOverride01_Byte_0, 2, (uint8_t*)&OvrData);
		OvrData=0xa000 |(FrameFormat.ActiveFrameLengthLines-LinesCnvtToSOF);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+SMIARx_Control_0_u16_FFormatDescriptorOverride02_Byte_0, 2, (uint8_t*)&OvrData);
        Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_e_ImageFormat_Output, 1, (uint8_t*)&FrameFormat.uwOutputPixelFormat);
	}
	else
	{
		Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_e_ImageFormat_Input, 1, (uint8_t*)&SpiData);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_e_ImageFormat_Output, 1, (uint8_t*)&SpiData);
	}

	OvrData=0x12;
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+SMIARx_Control_1_u8_FFModelSubtypeOverride, 1, (uint8_t*)&OvrData);
	OvrData=0xa000 |FrameFormat.ActiveLineLengthPixels;
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+SMIARx_Control_1_u16_FFormatDescriptorOverride0_Byte_0, 2, (uint8_t*)&OvrData);
	OvrData=0x2000 |ILPSOFLines;
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+SMIARx_Control_1_u16_FFormatDescriptorOverride01_Byte_0, 2, (uint8_t*)&OvrData);
	OvrData=0xa000 |(FrameFormat.ActiveFrameLengthLines-LinesCnvtToSOF);
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+SMIARx_Control_1_u16_FFormatDescriptorOverride02_Byte_0, 2, (uint8_t*)&OvrData);
	SpiData = FrameFormat.ActiveFrameLengthLines-LinesCnvtToSOF;
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_u16_CustomImageSizeY_Byte_0, 2, (uint8_t*)&SpiData);

	
	
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_FrameDimension_u32_HPreScaleFactor_Byte_0, 4, (uint8_t*)&FrameFormat.HScaling);
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_FrameDimension_u32_VPreScaleFactor_Byte_0, 4, (uint8_t*)&FrameFormat.VScaling);

	
	Flag=HIF_Flag_e_FALSE;
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_Flag_Binning, 1, (uint8_t*)&Flag);
	

	
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_FrameDimension_u16_HCropStart_Byte_0, 2, (uint8_t*)&FrameFormat.Hoffset);
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_FrameDimension_u16_VCropStart_Byte_0, 2, (uint8_t*)&FrameFormat.Voffset);


	
	Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_ModeStatus, 1, (uint8_t*)&ModeStatus);

	if(ModeStatus!=HIF_ModeStatus_e_STREAM)
	{
		
		while(ModeStatus!=HIF_ModeStatus_e_IDLE )
		{
			ILP0100_DEBUG_LOG("Checking Firmware to be in IDLE Mode");
			Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_ModeStatus, 1, (uint8_t*)&ModeStatus);
		}
		
		ModeControl= HIF_ModeControl_e_STREAM;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_ModeControl, 1, (uint8_t*)&ModeControl);

		
		for(Delay=0; Delay<200; Delay++);

		
		do
		{
			ILP0100_DEBUG_LOG("\n Waiting for Firmware Streaming");
			Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_ModeStatus, 1, (uint8_t*)&ModeStatus);
			if(ModeStatus==HIF_ModeStatus_e_ERROR)
			{
			

			Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_SystemError, 1, (uint8_t*)&SystemError);
			ILP0100_ERROR_LOG("ILP0100 Streaming System error =%x occured",SystemError);
			ILP0100_LOG_FUNCTION_END(Ret);
			return ILP0100_ERROR;
			}
		}
		while(ModeStatus!=HIF_ModeStatus_e_STREAM);
		ILP0100_DEBUG_LOG("Firmware Streaming OK");
	}
	else
	{
		
		ModeChange= HIF_ModeChange_e_REQUEST;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_ModeChange, 1, (uint8_t*)&ModeChange);

		
		while(1)
		{
			Ret= Ilp0100_core_readRegister((uint16_t)ILP0100_ITM_ISP2HOST_0_STATUS,4,(uint8_t*)&InterruptId);
			InterruptId&=INTR_MODE_CHANGE_REQUEST;
			if(InterruptId==INTR_MODE_CHANGE_REQUEST)
			{
				
				Ret=Ilp0100_core_interruptClearStatus(InterruptId, INTR_PIN_0);
				break;
			}
		}
		ILP0100_DEBUG_LOG("Firmware Mode Change OK");
	}

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_core_stop()
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	HIF_ModeControl_te ModeControl=HIF_ModeControl_e_IDLE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;
	uint8_t ModeStatus, SystemError;
	uint32_t Data;

	ILP0100_LOG_FUNCTION_START();

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_ModeControl, 1, (uint8_t*)&ModeControl);

	do
	{
		ILP0100_DEBUG_LOG("\n Waiting for Firmware to be in Idle State");
		Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_ModeStatus, 1, (uint8_t*)&ModeStatus);
		if(ModeStatus==HIF_ModeStatus_e_ERROR)
		{
		

		Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_SystemError, 1, (uint8_t*)&SystemError);
		ILP0100_ERROR_LOG("ILP0100 Streaming System error =%x occured",SystemError);
		ILP0100_LOG_FUNCTION_END(Ret);
		return ILP0100_ERROR;
		}
	}
	while(ModeStatus!=HIF_ModeStatus_e_IDLE);
	
	ILP0100_DEBUG_LOG("Firmware is in Idle State");
	
	Ret= Ilp0100_core_readRegister((uint16_t)ILP0100_ITM_ISP2HOST_0_STATUS_BCLR, 4, (uint8_t*)&Data);
	Data|=0x30300;
	Ret= Ilp0100_core_writeRegister((uint16_t)ILP0100_ITM_ISP2HOST_0_STATUS_BCLR, 4, (uint8_t*)&Data);

	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_core_configDefcor(const Ilp0100_structDefcorConfig DefcorConfig, uint8_t HDRMode)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;
	uint8_t Flag;

	ILP0100_LOG_FUNCTION_START((void*)&DefcorConfig, (void*)&HDRMode);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);


	if(HDRMode)
	{
		if(DefcorConfig.Mode==OFF)
		{
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_DusterEnable, 1, (uint8_t*)&Flag);
		}
		else if (DefcorConfig.Mode==SINGLET_ONLY)
		{
			Flag=Duster_Flag_e_True;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_DusterEnable, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_BypassDefcor, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_True;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_BypassRC, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_RC_DoubletsOnly, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_BypassCC, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_RC_UseSimplified, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_True;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_BypassGaussian, 1, (uint8_t*)&Flag);
			Flag=Duster_ScytheMode_e_BypassScythe;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_ScytheMode, 1, (uint8_t*)&Flag);


		}
		else if (DefcorConfig.Mode==COUPLET_ONLY)
		{
			Flag=Duster_Flag_e_True;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_DusterEnable, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_BypassDefcor, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_BypassRC, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_True;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_RC_DoubletsOnly, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_BypassCC, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_RC_UseSimplified, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_True;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_BypassGaussian, 1, (uint8_t*)&Flag);
			Flag=Duster_ScytheMode_e_BypassScythe;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_ScytheMode, 1, (uint8_t*)&Flag);
		}
		else
		{
			Flag=Duster_Flag_e_True;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_DusterEnable, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_BypassDefcor, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_BypassRC, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_RC_DoubletsOnly, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_BypassCC, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_RC_UseSimplified, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_True;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_BypassGaussian, 1, (uint8_t*)&Flag);
			Flag=Duster_ScytheMode_e_BypassScythe;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_ScytheMode, 1, (uint8_t*)&Flag);
		}
	}
	else
	{
		if(DefcorConfig.Mode==OFF)
		{
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_DusterEnable, 1, (uint8_t*)&Flag);
		}
		else if (DefcorConfig.Mode==SINGLET_ONLY)
		{
			Flag=Duster_Flag_e_True;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_DusterEnable, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_BypassDefcor, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_True;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_BypassRC, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_RC_DoubletsOnly, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_BypassCC, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_RC_UseSimplified, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_True;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_BypassGaussian, 1, (uint8_t*)&Flag);
			Flag=Duster_ScytheMode_e_BypassScythe;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_ScytheMode, 1, (uint8_t*)&Flag);


		}
		else if (DefcorConfig.Mode==COUPLET_ONLY)
		{
			Flag=Duster_Flag_e_True;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_DusterEnable, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_BypassDefcor, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_BypassRC, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_True;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_RC_DoubletsOnly, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_BypassCC, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_RC_UseSimplified, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_True;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_BypassGaussian, 1, (uint8_t*)&Flag);
			Flag=Duster_ScytheMode_e_BypassScythe;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_ScytheMode, 1, (uint8_t*)&Flag);
		}
		else
		{
			Flag=Duster_Flag_e_True;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_DusterEnable, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_BypassDefcor, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_BypassRC, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_RC_DoubletsOnly, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_BypassCC, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_False;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_RC_UseSimplified, 1, (uint8_t*)&Flag);
			Flag=Duster_Flag_e_True;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_BypassGaussian, 1, (uint8_t*)&Flag);
			Flag=Duster_ScytheMode_e_BypassScythe;
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_ScytheMode, 1, (uint8_t*)&Flag);
		}
	}

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	ILP0100_LOG_FUNCTION_END(Ret);

	return Ret;
}


ilp0100_error Ilp0100_core_updateDefcor(const Ilp0100_structDefcorParams DefcorParams, uint8_t HDRMode)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;

	ILP0100_LOG_FUNCTION_START((void*)&DefcorParams, (void*)&HDRMode);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	if(HDRMode)
	{
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_u8_LocalSigmaTh_CC, 1, (uint8_t*)&DefcorParams.SingletThreshold);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_u8_NormalisedTh_RC, 1, (uint8_t*)&DefcorParams.CoupletThreshold);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_u8_WhiteDCStrength, 1, (uint8_t*)&DefcorParams.BlackStrength);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_u8_BlackDCStrength, 1, (uint8_t*)&DefcorParams.WhiteStrength);
	}
	else
	{
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_u8_LocalSigmaTh_CC, 1, (uint8_t*)&DefcorParams.SingletThreshold);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_u8_NormalisedTh_RC, 1, (uint8_t*)&DefcorParams.CoupletThreshold);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_u8_WhiteDCStrength, 1, (uint8_t*)&DefcorParams.BlackStrength);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_u8_BlackDCStrength, 1, (uint8_t*)&DefcorParams.WhiteStrength);

	}

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_core_configChannelOffset(const Ilp0100_structChannelOffsetConfig ChannelOffsetConfig, uint8_t HDRMode)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;

	ILP0100_LOG_FUNCTION_START((void*)&ChannelOffsetConfig, (void*)&HDRMode);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	if(HDRMode)
	{
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Pedestal_Control_0_e_Flag_Enable, 1, (uint8_t*)&ChannelOffsetConfig.Enable);
	}
	else
	{
			Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Pedestal_Control_1_e_Flag_Enable, 1, (uint8_t*)&ChannelOffsetConfig.Enable);

	}

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_core_updateChannelOffset(const Ilp0100_structChannelOffsetParams ChannelOffsetParams, uint8_t HDRMode)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;

	ILP0100_LOG_FUNCTION_START((void*)&ChannelOffsetParams, (void*)&HDRMode);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	if(HDRMode)
	{
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+OffsetCompensation_Control_0_u16_SensorDataPedestal_Byte_0, 2, (uint8_t*)&ChannelOffsetParams.SensorPedestalRed);
	    Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+OffsetCompensation_Control_1_u16_SensorDataPedestal_Byte_0, 2, (uint8_t*)&ChannelOffsetParams.SensorPedestalGreenRed);
	    Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+OffsetCompensation_Control_2_u16_SensorDataPedestal_Byte_0, 2, (uint8_t*)&ChannelOffsetParams.SensorPedestalGreenBlue);
	    Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+OffsetCompensation_Control_3_u16_SensorDataPedestal_Byte_0, 2, (uint8_t*)&ChannelOffsetParams.SensorPedestalBlue);

	}
	else
	{
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+OffsetCompensation_Control_4_u16_SensorDataPedestal_Byte_0, 2, (uint8_t*)&ChannelOffsetParams.SensorPedestalRed);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+OffsetCompensation_Control_5_u16_SensorDataPedestal_Byte_0, 2, (uint8_t*)&ChannelOffsetParams.SensorPedestalGreenRed);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+OffsetCompensation_Control_6_u16_SensorDataPedestal_Byte_0, 2, (uint8_t*)&ChannelOffsetParams.SensorPedestalGreenBlue);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+OffsetCompensation_Control_7_u16_SensorDataPedestal_Byte_0, 2, (uint8_t*)&ChannelOffsetParams.SensorPedestalBlue);

	}

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);


	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}


ilp0100_error Ilp0100_core_configGlace(const Ilp0100_structGlaceConfig GlaceConfig, uint8_t HDRMode)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;

	ILP0100_LOG_FUNCTION_START((void*)&GlaceConfig, (void*)&HDRMode);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	if(HDRMode)
	{
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_0_e_Flag_GlaceEnabled, 1, (uint8_t*)&GlaceConfig.Enable);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_0_u16_hOffset_Byte_0, 2, (uint8_t*)&GlaceConfig.RoiHStart);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_0_u16_vOffset_Byte_0, 2, (uint8_t*)&GlaceConfig.RoiVStart);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_0_u16_hBlockSize_Byte_0, 2, (uint8_t*)&GlaceConfig.RoiHBlockSize);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_0_u16_vBlockSize_Byte_0, 2, (uint8_t*)&GlaceConfig.RoiVBlockSize);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_0_u8_hGridSize, 1, (uint8_t*)&GlaceConfig.RoiHNumberOfBlocks);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_0_u8_vGridSize, 1, (uint8_t*)&GlaceConfig.RoiVNumberOfBlocks);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_0_u8_SatLevelRed, 1, (uint8_t*)&GlaceConfig.SaturationLevelRed);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_0_u8_SatLevelGreen, 1, (uint8_t*)&GlaceConfig.SaturationLevelGreen);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_0_u8_SatLevelBlue, 1, (uint8_t*)&GlaceConfig.SaturationLevelBlue);

	}
	else
	{
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_1_e_Flag_GlaceEnabled, 1, (uint8_t*)&GlaceConfig.Enable);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_1_u16_hOffset_Byte_0, 2, (uint8_t*)&GlaceConfig.RoiHStart);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_1_u16_vOffset_Byte_0, 2, (uint8_t*)&GlaceConfig.RoiVStart);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_1_u16_hBlockSize_Byte_0, 2, (uint8_t*)&GlaceConfig.RoiHBlockSize);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_1_u16_vBlockSize_Byte_0, 2, (uint8_t*)&GlaceConfig.RoiVBlockSize);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_1_u8_hGridSize, 1, (uint8_t*)&GlaceConfig.RoiHNumberOfBlocks);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_1_u8_vGridSize, 1, (uint8_t*)&GlaceConfig.RoiVNumberOfBlocks);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_1_u8_SatLevelRed, 1, (uint8_t*)&GlaceConfig.SaturationLevelRed);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_1_u8_SatLevelGreen, 1, (uint8_t*)&GlaceConfig.SaturationLevelGreen);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_1_u8_SatLevelBlue, 1, (uint8_t*)&GlaceConfig.SaturationLevelBlue);

	}

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);



	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}



ilp0100_error Ilp0100_core_configHist(const Ilp0100_structHistConfig HistConfig, uint8_t HDRMode)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;
	uint8_t Flag;

	ILP0100_LOG_FUNCTION_START((void*)&HistConfig, (void*)&HDRMode);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	if(HDRMode)
	{
		  if(HistConfig.Mode==HIST_OFF)
		  {
			  Flag= Histogram_Flag_e_FALSE;
			  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Control_0_e_Flag_Enabled, 1, (uint8_t*)&Flag);
		  }
		  else if (HistConfig.Mode==REDGREENBLUE)
		  {
			  Flag= Histogram_Flag_e_TRUE;
			  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Control_0_e_Flag_Enabled, 1, (uint8_t*)&Flag);

			  Flag=Bayer2Y_Flag_e_FALSE;
			  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Bayer2Y_Control_0_e_Flag_Bayer2YEnabled, 1, (uint8_t*)&Flag);

		  }
		  else
		  {
			  Flag= Histogram_Flag_e_TRUE;
			  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Control_0_e_Flag_Enabled, 1, (uint8_t*)&Flag);

			  Flag=Bayer2Y_Flag_e_TRUE;
			  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Bayer2Y_Control_0_e_Flag_Bayer2YEnabled, 1, (uint8_t*)&Flag);

		  }
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Control_0_u16_FOVOffsetX_Byte_0, 2, (uint8_t*)&HistConfig.RoiXOffset);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Control_0_u16_FOVOffsetY_Byte_0, 2, (uint8_t*)&HistConfig.RoiYOffset);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Control_0_u16_HistSizeX_Byte_0, 2, (uint8_t*)&HistConfig.RoiXSize);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Control_0_u16_HistSizeY_Byte_0, 2, (uint8_t*)&HistConfig.RoiYSize);


		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Bayer2Y_Status_0_u8_FPKgr_Byte_0, 1, (uint8_t*)&HistConfig.YConversionFactorGreenRed);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Bayer2Y_Status_0_u8_FPKr_Byte_0, 1, (uint8_t*)&HistConfig.YConversionFactorRed);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Bayer2Y_Status_0_u8_FPKb_Byte_0, 1, (uint8_t*)&HistConfig.YConversionFactorBlue);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Bayer2Y_Status_0_u8_FPKgb_Byte_0, 1, (uint8_t*)&HistConfig.YConversionFactorGreenBlue);

	}
	else
	{
		  if(HistConfig.Mode==HIST_OFF)
		  {
			  Flag= Histogram_Flag_e_FALSE;
			  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Control_1_e_Flag_Enabled, 1, (uint8_t*)&Flag);
		  }
		  else if (HistConfig.Mode==REDGREENBLUE)
		  {
			  Flag= Histogram_Flag_e_TRUE;
			  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Control_1_e_Flag_Enabled, 1, (uint8_t*)&Flag);

			  Flag=Bayer2Y_Flag_e_FALSE;
			  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Bayer2Y_Control_1_e_Flag_Bayer2YEnabled, 1, (uint8_t*)&Flag);
	      }
		  else
		  {
			  Flag= Histogram_Flag_e_TRUE;
			  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Control_1_e_Flag_Enabled, 1, (uint8_t*)&Flag);

			  Flag=Bayer2Y_Flag_e_TRUE;
			  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Bayer2Y_Control_1_e_Flag_Bayer2YEnabled, 1, (uint8_t*)&Flag);
		  }
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Control_1_u16_FOVOffsetX_Byte_0, 2, (uint8_t*)&HistConfig.RoiXOffset);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Control_1_u16_FOVOffsetY_Byte_0, 2, (uint8_t*)&HistConfig.RoiYOffset);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Control_1_u16_HistSizeX_Byte_0, 2, (uint8_t*)&HistConfig.RoiXSize);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Control_1_u16_HistSizeY_Byte_0, 2, (uint8_t*)&HistConfig.RoiYSize);

		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Bayer2Y_Status_1_u8_FPKgr_Byte_0, 1, (uint8_t*)&HistConfig.YConversionFactorGreenRed);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Bayer2Y_Status_1_u8_FPKr_Byte_0, 1, (uint8_t*)&HistConfig.YConversionFactorRed);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Bayer2Y_Status_1_u8_FPKb_Byte_0, 1, (uint8_t*)&HistConfig.YConversionFactorBlue);
		  Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Bayer2Y_Status_1_u8_FPKgb_Byte_0, 1, (uint8_t*)&HistConfig.YConversionFactorGreenBlue);
	}


	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);



	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}



ilp0100_error Ilp0100_core_configHdrMerge(const Ilp0100_structHdrMergeConfig HdrMergeConfig)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;
	uint8_t Data;

	ILP0100_LOG_FUNCTION_START((void*)&HdrMergeConfig);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	if(HdrMergeConfig.Mode==ON)
	{
		Data=HIF_ImagePipe_e_HDR;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_ImagePipe_StreamOutput, 1, (uint8_t*)&Data);
	}
	else if(HdrMergeConfig.Mode==OUTPUT_LONG_ONLY)
	{
		Data=HIF_ImagePipe_e_LONG;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_ImagePipe_StreamOutput, 1, (uint8_t*)&Data);
	}
	else
	{
		Data=HIF_ImagePipe_e_SHORT;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_ImagePipe_StreamOutput, 1, (uint8_t*)&Data);
	}

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);


	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_core_updateHdrMerge(const Ilp0100_structHdrMergeParams HdrMergeParams)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;

	ILP0100_LOG_FUNCTION_START((void*)&HdrMergeParams);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HawkEyes_Control_e_MergeMode, 1, (uint8_t*)&HdrMergeParams.Method);
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HawkEyes_Control_e_Flag_ByPassMacroPixel, 1, (uint8_t*)&HdrMergeParams.ImageCodes);

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);


	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}


ilp0100_error Ilp0100_core_configCls(const Ilp0100_structClsConfig ClsConfig)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;

	ILP0100_LOG_FUNCTION_START((void*)&ClsConfig);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Rubik_Input_e_Flag_RubikEnable, 1, (uint8_t*)&ClsConfig.Enable);

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);


	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}


ilp0100_error Ilp0100_core_updateCls(const Ilp0100_structClsParams ClsParams)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;

	ILP0100_LOG_FUNCTION_START((void*)&ClsParams);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+RubikTop_Control_u8_CornerGain , 1, (uint8_t*)&ClsParams.BowlerCornerGain);
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+RubikTop_Control_u16_ColourTemp_Byte_0, 2, (uint8_t*)&ClsParams.ColorTempKelvin);

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);


	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_core_configToneMapping(const Ilp0100_structToneMappingConfig ToneMappingConfig)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;

	ILP0100_LOG_FUNCTION_START((void*)&ToneMappingConfig);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+ToneMap_Control_e_Flag_ToneMapEnabled, 1, (uint8_t*)&ToneMappingConfig.Enable);
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+ToneMap_Control_e_Flag_UserDefinedCurve, 1, (uint8_t*)&ToneMappingConfig.UserDefinedCurveEnable);


	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_core_updateToneMapping(const Ilp0100_structToneMappingParams ToneMappingParams)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;

	ILP0100_LOG_FUNCTION_START((void*)&ToneMappingParams);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);


	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+ToneMap_Control_u8_Strength, 1, (uint8_t*)&ToneMappingParams.Strength);
	
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histom_Control_u16_KneePoints_0_Byte_0, 512, (uint8_t*)&ToneMappingParams.UserDefinedCurve);

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);


	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_core_updateSensorParams(const Ilp0100_structFrameParams FrameParams, uint8_t HDRMode)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;

	ILP0100_LOG_FUNCTION_START((void*)&FrameParams, (void*)&HDRMode);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	if(HDRMode)
	{
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_Exposure_0_u16_CurrentExposureTime_Byte_0, 2, (uint8_t*)&FrameParams.ExposureTime);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_Exposure_0_u16_AnalogGainCodeGreen_Byte_0, 2, (uint8_t*)&FrameParams.AnalogGainCodeGreen);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_Exposure_0_u16_AnalogGainCodeRed_Byte_0, 2, (uint8_t*)&FrameParams.AnalogGainCodeRed);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_Exposure_0_u16_AnalogGainCodeBlue_Byte_0, 2, (uint8_t*)&FrameParams.AnalogGainCodeBlue);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_Exposure_0_u16_DigitalGainCodeGreen_Byte_0, 2, (uint8_t*)&FrameParams.DigitalGainCodeGreen);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_Exposure_0_u16_DigitalGainCodeRed_Byte_0, 2, (uint8_t*)&FrameParams.DigitalGainCodeRed);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_Exposure_0_u16_DigitalGainCodeBlue_Byte_0, 2, (uint8_t*)&FrameParams.DigitalGainCodeBlue);
	}
	else
	{
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_Exposure_1_u16_CurrentExposureTime_Byte_0, 2, (uint8_t*)&FrameParams.ExposureTime);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_Exposure_1_u16_AnalogGainCodeGreen_Byte_0, 2, (uint8_t*)&FrameParams.AnalogGainCodeGreen);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_Exposure_1_u16_AnalogGainCodeRed_Byte_0, 2, (uint8_t*)&FrameParams.AnalogGainCodeRed);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_Exposure_1_u16_AnalogGainCodeBlue_Byte_0, 2, (uint8_t*)&FrameParams.AnalogGainCodeBlue);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_Exposure_1_u16_DigitalGainCodeGreen_Byte_0, 2, (uint8_t*)&FrameParams.DigitalGainCodeGreen);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_Exposure_1_u16_DigitalGainCodeRed_Byte_0, 2, (uint8_t*)&FrameParams.DigitalGainCodeRed);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+InputInterface_Exposure_1_u16_DigitalGainCodeBlue_Byte_0, 2, (uint8_t*)&FrameParams.DigitalGainCodeBlue);
	}

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	ILP0100_LOG_FUNCTION_END(Ret);

	return Ret;

}

ilp0100_error Ilp0100_core_startTestMode(uint16_t IpsBypass, uint16_t TestMode)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint8_t SystemError,ModeStatus,ModeControl,Data,Flag, TestPatternInputInterface, HDRMode, SpiData;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;
	
	ILP0100_LOG_FUNCTION_START((void*)&IpsBypass, (void*)&TestMode);


	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	
	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	ModeControl=HIF_ModeControl_e_IDLE;
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_ModeControl, 1, (uint8_t*)&ModeControl);
	do
	{
		ILP0100_DEBUG_LOG("\n Waiting for Firmware to be in Idle State");
		Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_ModeStatus, 1, (uint8_t*)&ModeStatus);
		if(ModeStatus==HIF_ModeStatus_e_ERROR)
		{
		

		Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_SystemError, 1, (uint8_t*)&SystemError);
		ILP0100_ERROR_LOG("ILP0100 Streaming System error =%x occured",SystemError);
		ILP0100_LOG_FUNCTION_END(Ret);
		return ILP0100_ERROR;
		}
	}
	while(ModeStatus!=HIF_ModeStatus_e_IDLE);

	
	Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_Flag_HDRMode, 1, (uint8_t*)&HDRMode);
	if(HDRMode==STAGGERED)
	{
		SpiData=CRMTop_Flag_e_FALSE;
	}
	else
	{
		SpiData=CRMTop_Flag_e_TRUE;
	}
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+CRMTop_Control_e_Flag_AutoPixClockMode, 1, (uint8_t*)&SpiData);

	if(IpsBypass&BYPASS_CHANNEL_OFFSET)
	{
		Flag= Pedestal_Flag_e_FALSE;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Pedestal_Control_0_e_Flag_Enable,1 ,(uint8_t*)&Flag);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Pedestal_Control_1_e_Flag_Enable,1 ,(uint8_t*)&Flag);
	}
	if(IpsBypass&BYPASS_DEFCOR)
	{
		Flag= Duster_Flag_e_False;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_0_e_Flag_DusterEnable,1 ,(uint8_t*)&Flag);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Duster_Control_1_e_Flag_DusterEnable,1 ,(uint8_t*)&Flag);
	}
	if(IpsBypass&BYPASS_STATS)
	{
		Flag= Glace_Flag_e_FALSE;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_0_e_Flag_GlaceEnabled,1 ,(uint8_t*)&Flag);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Glace_Control_1_e_Flag_GlaceEnabled,1 ,(uint8_t*)&Flag);
		Flag= Histogram_Flag_e_FALSE;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Control_0_e_Flag_Enabled,1 ,(uint8_t*)&Flag);
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Control_1_e_Flag_Enabled,1 ,(uint8_t*)&Flag);
	}
	if(IpsBypass&BYPASS_HDR_MERGE_KEEP_LONG)
	{
		Flag=HIF_ImagePipe_e_LONG;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_ImagePipe_StreamOutput, 1, (uint8_t*)&Flag);
	}
	if(IpsBypass&BYPASS_HDR_MERGE_KEEP_SHORT)
	{
		Flag=HIF_ImagePipe_e_SHORT;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_ImagePipe_StreamOutput, 1, (uint8_t*)&Flag);
	}
	if(IpsBypass&BYPASS_TONE_MAPPING)
	{
		Flag= ToneMap_Flag_e_FALSE;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+ToneMap_Control_e_Flag_ToneMapEnabled,1 ,(uint8_t*)&Flag);
	}
	if(IpsBypass&BYPASS_LSC)
	{
		Flag= Rubik_Flag_e_FALSE;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+Rubik_Input_e_Flag_RubikEnable,1 ,(uint8_t*)&Flag);
	}
	if(IpsBypass&BYPASS_ISP)   
	{
		Flag= HIF_Flag_e_TRUE;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_Flag_ISPBypass,1 ,(uint8_t*)&Flag);
	}

	if(IpsBypass&FORCE_LSC_GRIDS0)
	{
		Data= RubikTop_Mode_e_MANUAL_GRID0;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+RubikTop_Control_e_Mode_GridSettings,1 ,(uint8_t*)&Data);
	}
	else if(IpsBypass&FORCE_LSC_GRIDS1)
	{
		Data= RubikTop_Mode_e_MANUAL_GRID1;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+RubikTop_Control_e_Mode_GridSettings,1 ,(uint8_t*)&Data);
	}
	else if(IpsBypass&FORCE_LSC_GRIDS2)
	{
		Data= RubikTop_Mode_e_MANUAL_GRID2;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+RubikTop_Control_e_Mode_GridSettings,1 ,(uint8_t*)&Data);
	}
	else if(IpsBypass&FORCE_LSC_GRIDS3)
	{
		Data= RubikTop_Mode_e_MANUAL_GRID3;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+RubikTop_Control_e_Mode_GridSettings,1 ,(uint8_t*)&Data);
	}
	else
	{
		
	}

	switch(TestMode)
	{
	case TEST_NO_TEST_MODE:
		TestPatternInputInterface=HIF_InputImageSource_e_SENSOR;
		break;
	case TEST_UNIFORM_GREY:
		TestPatternInputInterface=HIF_InputImageSource_e_RX_TP_DIAG_GREY;
		break;
	case TEST_COLORBAR:
		TestPatternInputInterface=HIF_InputImageSource_e_RX_TP_COLOUR_BAR;
		break;
	case TEST_PN29:
		TestPatternInputInterface=HIF_InputImageSource_e_RX_TP_PSEUDORANDOM;
		break;
	case TEST_SOLID:
		TestPatternInputInterface=HIF_InputImageSource_e_RX_TP_SOLID;
		break;
	case TEST_ADD_STATUS_LINES:
		
		break;
	default:
		break;

	}

	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_ImageCharacteristics_e_InputImageSource,1 ,(uint8_t*)&TestPatternInputInterface);


	
	ModeControl=HIF_ModeControl_e_STREAM;
	Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_ModeControl, 1, (uint8_t*)&ModeControl);
	do
	{
		ILP0100_DEBUG_LOG("\n Waiting for Firmware Streaming");
		Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_ModeStatus, 1, (uint8_t*)&ModeStatus);
		if(ModeStatus==HIF_ModeStatus_e_ERROR)
		{
		

		Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_SystemError, 1, (uint8_t*)&SystemError);
		ILP0100_ERROR_LOG("ILP0100 Streaming System error =%x occured",SystemError);
		ILP0100_LOG_FUNCTION_END(Ret);
		return ILP0100_ERROR;
		}
	}
	while(ModeStatus!=HIF_ModeStatus_e_STREAM);

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	ILP0100_LOG_FUNCTION_END(Ret);

	return Ret;

}

ilp0100_error Ilp0100_core_readBackGlaceStatistics(Ilp0100_structGlaceStatsData *pGlaceStatsData, uint8_t HDRMode)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;

	uint8_t Loop, SizeGlaceZones=48;
	
	uint8_t ArrGlaceStats[384]; 
	uint32_t Addr;
	uint32_t GlaceStatsMemoryAddress;
	uint32_t StatsAdd;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint8_t StatsFlag;
	uint32_t GlaceStatsNbSaturatedPixels;
	ILP0100_LOG_FUNCTION_START((void*)pGlaceStatsData,(void*)&HDRMode);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	
	if(HDRMode)
	{
		StatsAdd=(uint32_t)ILP0100_LONG_EXP_GLACE_STATS_PAGE;
		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&StatsAdd);
	}
	else
	{
		StatsAdd=(uint32_t)ILP0100_SHORT_EXP_GLACE_STATS_PAGE;
		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&StatsAdd);
	}

	GlaceStatsMemoryAddress=SPI_BASE_ADD_1+0x004;


	
	
	Ret|= Ilp0100_core_readRegister((uint16_t)GlaceStatsMemoryAddress, SizeGlaceZones*0x08, (uint8_t*)ArrGlaceStats);
	for(Loop=0; Loop<SizeGlaceZones; Loop++)
	{
		pGlaceStatsData->GlaceStatsBlueMean[Loop]=ArrGlaceStats[0x08*Loop+0];
		pGlaceStatsData->GlaceStatsGreenMean[Loop]=ArrGlaceStats[0x08*Loop+1];
	 	pGlaceStatsData->GlaceStatsRedMean[Loop]=ArrGlaceStats[0x08*Loop+2];
		GlaceStatsNbSaturatedPixels=*(uint32_t*)(ArrGlaceStats+0x08*Loop+4);
		
		GlaceStatsNbSaturatedPixels=GlaceStatsNbSaturatedPixels>>1;
		pGlaceStatsData->GlaceStatsNbOfSaturatedPixels[Loop]= (uint16_t)GlaceStatsNbSaturatedPixels;
	}

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr); 
	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	if(HDRMode)
	{
		
		StatsFlag=HIF_StatsHostAck_e_DONE;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_StatsHostAck_LongGlace,1 ,(uint8_t*)&StatsFlag);
	}
	else
	{
		
		StatsFlag=HIF_StatsHostAck_e_DONE;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_StatsHostAck_ShortGlace,1 ,(uint8_t*)&StatsFlag);
	}

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);


	ILP0100_LOG_FUNCTION_END(Ret);

	return Ret;

}

ilp0100_error Ilp0100_core_readBackHistStatistics(Ilp0100_structHistStatsData *pHistStatsData, uint8_t HDRMode)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;

	uint8_t Loop, SizeHistBins=64;
	uint32_t ArrHistStats[64]; 
	uint16_t ArrHistBins[9]; 
	uint32_t Addr;
	uint32_t HistStatsMemoryAddressGreenBins;
	uint32_t HistStatsMemoryAddressRedBins;
	uint32_t HistStatsMemoryAddressBlueBins;
	uint32_t StatsAdd;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint8_t StatsFlag;
	ILP0100_LOG_FUNCTION_START((void*)pHistStatsData, (void*)&HDRMode);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	
	if(HDRMode)
	{
		StatsAdd=(uint32_t)ILP0100_LONG_EXP_HIST_STATS_PAGE;
		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&StatsAdd);
	}
	else
	{
		StatsAdd=(uint32_t)ILP0100_SHORT_EXP_HIST_STATS_PAGE;
		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&StatsAdd);
	}

	HistStatsMemoryAddressGreenBins=SPI_BASE_ADD_1+0x004;
	HistStatsMemoryAddressRedBins=SPI_BASE_ADD_1+0x104;
	HistStatsMemoryAddressBlueBins=SPI_BASE_ADD_1+0x204;

	

	Ret|= Ilp0100_core_readRegister((uint16_t)HistStatsMemoryAddressGreenBins, SizeHistBins*4, (uint8_t*)ArrHistStats);
	for (Loop=0; Loop<64 ; Loop++)
	{
		pHistStatsData->HistStatsGreenBin[Loop]=ArrHistStats[Loop] & 0x001FFFFF; 
	}

	Ret|= Ilp0100_core_readRegister((uint16_t)HistStatsMemoryAddressRedBins, SizeHistBins*4,(uint8_t*)ArrHistStats);
	for (Loop=0; Loop<64 ; Loop++)
	{
		pHistStatsData->HistStatsRedBin[Loop]=ArrHistStats[Loop] & 0x000FFFFF; 
	}

	Ret|= Ilp0100_core_readRegister((uint16_t)HistStatsMemoryAddressBlueBins, SizeHistBins*4,(uint8_t*)ArrHistStats);
	for (Loop=0; Loop<64 ; Loop++)
	{
		pHistStatsData->HistStatsBlueBin[Loop]=ArrHistStats[Loop] & 0x000FFFFF; 
	}

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr); 
	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);


	
	if (HDRMode)
	{
		Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Status_0_u16_DarkestBin_R_Byte_0, 18,(uint8_t*)ArrHistBins);
	}
	else
	{
		Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+Histogram_Status_1_u16_DarkestBin_R_Byte_0, 18,(uint8_t*)ArrHistBins);
	}
	
	pHistStatsData->HistStatsRedDarkestBin=ArrHistBins[0]& 0x003F; 
	pHistStatsData->HistStatsRedBrightestBin=ArrHistBins[1]& 0x003F;
	pHistStatsData->HistStatsRedHighestBin=ArrHistBins[2]& 0x003F;
	pHistStatsData->HistStatsGreenDarkestBin=ArrHistBins[3]& 0x003F;
	pHistStatsData->HistStatsGreenBrightestBin=ArrHistBins[4]& 0x003F;
	pHistStatsData->HistStatsGreenHighestBin=ArrHistBins[5]& 0x003F;
	pHistStatsData->HistStatsBlueDarkestBin=ArrHistBins[6]& 0x003F;
	pHistStatsData->HistStatsBlueBrightestBin=ArrHistBins[7]& 0x003F;
	pHistStatsData->HistStatsBlueHighestBin=ArrHistBins[8]& 0x003F;

	

	pHistStatsData->HistStatsRedDarkestCount=pHistStatsData->HistStatsRedBin[pHistStatsData->HistStatsRedDarkestBin];
	pHistStatsData->HistStatsRedBrightestCount=pHistStatsData->HistStatsRedBin[pHistStatsData->HistStatsRedBrightestBin];
	pHistStatsData->HistStatsRedHighestCount=pHistStatsData->HistStatsRedBin[pHistStatsData->HistStatsRedHighestBin];
	pHistStatsData->HistStatsGreenDarkestCount=pHistStatsData->HistStatsGreenBin[pHistStatsData->HistStatsGreenDarkestBin];
	pHistStatsData->HistStatsGreenBrightestCount=pHistStatsData->HistStatsGreenBin[pHistStatsData->HistStatsGreenBrightestBin];
	pHistStatsData->HistStatsGreenHighestCount=pHistStatsData->HistStatsGreenBin[pHistStatsData->HistStatsGreenHighestBin];
	pHistStatsData->HistStatsBlueDarkestCount=pHistStatsData->HistStatsBlueBin[pHistStatsData->HistStatsBlueDarkestBin];
	pHistStatsData->HistStatsBlueBrightestCount=pHistStatsData->HistStatsBlueBin[pHistStatsData->HistStatsBlueBrightestBin];
	pHistStatsData->HistStatsBlueHighestCount=pHistStatsData->HistStatsBlueBin[pHistStatsData->HistStatsBlueHighestBin];


	if(HDRMode)
	{
		
		StatsFlag=HIF_StatsHostAck_e_DONE;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_StatsHostAck_LongHist,1 ,(uint8_t*)&StatsFlag);
	}
	else
	{
		
		StatsFlag=HIF_StatsHostAck_e_DONE;
		Ret|= Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HIF_SystemandStatus_e_StatsHostAck_ShortHist,1 ,(uint8_t*)&StatsFlag);
	}

	
	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	ILP0100_LOG_FUNCTION_END(Ret);

	return Ret;

}

ilp0100_error Ilp0100_core_setVirtualChannel(uint8_t VirtualChannel, uint8_t HDRMode)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint8_t Data[4];
	uint8_t ShortOrNormalDataPath=0x00, LongDataPath=0x01;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;

	ILP0100_LOG_FUNCTION_START((void*)&VirtualChannel, (void*)&HDRMode);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	Ret|=Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+CSI2Rx_Control_e_VirtualChannelMap_Ch0, 4, (uint8_t*)&Data);

	if(VirtualChannel<4)
	{
		if(HDRMode)
		{
			Data[VirtualChannel]=LongDataPath;
		}
		else
		{
			Data[VirtualChannel]=ShortOrNormalDataPath;
		}
	}

	Ret|=Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+CSI2Rx_Control_e_VirtualChannelMap_Ch0, 4, (uint8_t*)&Data);

	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_core_setHDRFactor(uint8_t HDRFactor)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;

	ILP0100_LOG_FUNCTION_START((void*)&HDRFactor);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	Ret|=Ilp0100_core_writeRegister((uint16_t)SPI_BASE_ADD_0+HDRMerge_Control_u8_HDRFactor, 1, (uint8_t*)&HDRFactor);

	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

ilp0100_error Ilp0100_core_reset()
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START();

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_core_onTheFlyReset()
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START();

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}
ilp0100_error Ilp0100_core_interruptEnable(uint32_t InterruptSetMask, bool_t Pin)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&InterruptSetMask, (void*)&Pin);
	
	if(Pin==INTR_PIN_0)
	{
		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_ITM_ISP2HOST_0_EN_STATUS_BSET, 4, (uint8_t*)&InterruptSetMask);
	}
	else
	{
		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_ITM_ISP2HOST_1_EN_STATUS_BSET, 4, (uint8_t*)&InterruptSetMask);
	}


	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_core_interruptDisable(uint32_t InterruptClrMask, bool_t Pin)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&InterruptClrMask, (void*)&Pin);
	
	if(Pin==INTR_PIN_0)
	{
		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_ITM_ISP2HOST_0_EN_STATUS_BCLR, 4, (uint8_t*)&InterruptClrMask);
	}
	else
	{
		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_ITM_ISP2HOST_1_EN_STATUS_BCLR, 4, (uint8_t*)&InterruptClrMask);
	}


	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_core_interruptReadStatus(uint32_t* pInterruptId, uint8_t Pin)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t InterruptStatus, InterruptEnableStatus;
	
	

	ILP0100_LOG_FUNCTION_START((void*)pInterruptId, (void*)&Pin);

	if(Pin==0)
	{
		Ret|=Ilp0100_core_readRegister((uint16_t)ILP0100_ITM_ISP2HOST_0_STATUS,4,(uint8_t*)&InterruptStatus);  
		Ret|=Ilp0100_core_readRegister((uint16_t)ILP0100_ITM_ISP2HOST_0_EN_STATUS,4,(uint8_t*)&InterruptEnableStatus); 
	}
	else
	{
		Ret|=Ilp0100_core_readRegister((uint16_t)ILP0100_ITM_ISP2HOST_1_STATUS,4,(uint8_t*)&InterruptStatus);  
		Ret|=Ilp0100_core_readRegister((uint16_t)ILP0100_ITM_ISP2HOST_1_EN_STATUS,4,(uint8_t*)&InterruptEnableStatus); 
	}

	
	InterruptStatus&=InterruptEnableStatus;

	
	*pInterruptId=InterruptStatus;

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_core_interruptClearStatus(uint32_t InterruptId, uint8_t Pin)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t InterruptClrStatus;
	ILP0100_LOG_FUNCTION_START((void*)&InterruptId, (void*)&Pin);

	if(Pin==0)
	{
		Ret|=Ilp0100_core_readRegister((uint16_t)ILP0100_ITM_ISP2HOST_0_STATUS_BCLR,4,(uint8_t*)&InterruptClrStatus);  
		InterruptClrStatus|=InterruptId; 
		Ret|=Ilp0100_core_writeRegister((uint16_t)ILP0100_ITM_ISP2HOST_0_STATUS_BCLR,4,(uint8_t*)&InterruptClrStatus);  
	}
	else
	{
		Ret|=Ilp0100_core_readRegister((uint16_t)ILP0100_ITM_ISP2HOST_0_STATUS_BCLR,4,(uint8_t*)&InterruptClrStatus);  
		InterruptClrStatus|=InterruptId;  
		Ret|=Ilp0100_core_writeRegister((uint16_t)ILP0100_ITM_ISP2HOST_1_STATUS_BCLR,4,(uint8_t*)&InterruptClrStatus);  
	}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_core_getApiVersionNumber(uint8_t* pMajorNumber, uint8_t* pMinorNumber)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)pMajorNumber, (void*)pMinorNumber);
	*pMajorNumber= ILP0100_MAJOR_VERSION_NUMBER;
	*pMinorNumber= ILP0100_MINOR_VERSION_NUMBER;
	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_core_getFirmwareVersionumber(uint8_t* pMajorNumber, uint8_t* pMinorNumber)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint32_t SpiWin1BaseAddr=0x00004000;
	uint32_t Addr;

	ILP0100_LOG_FUNCTION_START((void*)pMajorNumber, (void*)pMinorNumber);

	
	Ilp0100_core_saveSpiWindow1Addr(&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&SpiWin1BaseAddr);

	

	Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+DeviceInfo_Status_u8_SystemFirmwareVersionMajor,1 ,(uint8_t*)pMajorNumber);
	Ret|= Ilp0100_core_readRegister((uint16_t)SPI_BASE_ADD_0+DeviceInfo_Status_u8_SystemFirmwareVersionMinor,1 ,(uint8_t*)pMinorNumber);

	Ilp0100_core_restoreSpiWindow1Addr(Addr);

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_core_readRegister(uint16_t RegisterName, uint16_t Count, uint8_t *pData)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	bool_t SpiStatus=1;
	uint16_t Loop;

	if(PllLocked) 
	{
		SpiStatus&= SPI_Read((uint16_t)RegisterName, Count, (uint8_t*)pData);
	}
	else
	{
		for(Loop=0; Loop<Count; Loop++)
		{
			SpiStatus&= SPI_Read((uint16_t)RegisterName+Loop, 1, (uint8_t*)pData+Loop);
		}
	}

	if(SpiStatus!=1)
	{
		Ret=ILP0100_ERROR;
		ILP0100_LOG_ILP_ACCESS(RegisterName, Count, pData, Ret);
		return Ret;
	}
	else
	{
		ILP0100_LOG_ILP_ACCESS(RegisterName, Count, pData, Ret);
		return Ret;
	}
}

ilp0100_error Ilp0100_core_writeRegister(uint16_t RegisterName, uint16_t Count, uint8_t *pData)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	bool_t SpiStatus=1;
	uint16_t Loop;

	if(PllLocked) 
	{
		SpiStatus&= SPI_Write((uint16_t)RegisterName, Count, (uint8_t*)pData);
	}
	else
	{
		for(Loop=0; Loop<Count; Loop++)
		{
			SpiStatus&= SPI_Write((uint16_t)RegisterName+Loop, 1, (uint8_t*)pData+Loop);

		}
	}

	if(SpiStatus!=1)
	{
		Ret=ILP0100_ERROR;
		ILP0100_LOG_ILP_ACCESS(RegisterName, Count, pData, Ret);
		return Ret;
	}
	else
	{
		ILP0100_LOG_ILP_ACCESS(RegisterName, Count, pData, Ret);
		return Ret;
	}
}

ilp0100_error Ilp0100_core_enableIlp0100SensorClock(bool_t SensorInterface)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;

	uint8_t SpiData;
	ILP0100_LOG_FUNCTION_START((void*)&SensorInterface);
	if(SensorInterface==SENSOR_0)
	{
		Ret |= Ilp0100_core_readRegister((uint16_t)ILP0100_CLK_CTRL+1, 1, &SpiData );
		SpiData|=0x04;
		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_CLK_CTRL+1, 1, &SpiData );
	}
	else
	{
		Ret|= Ilp0100_core_readRegister((uint16_t)ILP0100_CLK_CTRL+1, 1, &SpiData );
		SpiData|=0x08;
		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_CLK_CTRL+1, 1, &SpiData );
	}

	ILP0100_LOG_FUNCTION_END(Ret);

	return Ret;

}

ilp0100_error Ilp0100_core_disableIlp0100SensorClock(bool_t SensorInterface)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	uint8_t SpiData;
	ILP0100_LOG_FUNCTION_START((void*)&SensorInterface);
	if(SensorInterface==SENSOR_0)
	{
		Ret|= Ilp0100_core_readRegister((uint16_t)ILP0100_CLK_CTRL+1, 1, &SpiData );
		SpiData&=0xFB;
		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_CLK_CTRL+1, 1, &SpiData );
	}
	else
	{
		Ret|= Ilp0100_core_readRegister((uint16_t)ILP0100_CLK_CTRL+1, 1, &SpiData );
		SpiData&=0xF7;
		Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_CLK_CTRL+1, 1, &SpiData );
	}

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;

}

/*!
 * \fn 			ilp0100_error Ilp0100_core_saveSpiWindow1Addr(uint32_t *pAddr)
 * \brief		Core function to save Spi Window 1 Address
 * \ingroup		Core_Functions
 * \param[out] 	pAddr	: Present SPI window Address retrieved and written on pData.
 * \retval 		ILP0100_ERROR_NONE : Success
 * \retval 		"Other Error Code" : Failure
 */
ilp0100_error Ilp0100_core_saveSpiWindow1Addr(uint32_t *pAddr)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)pAddr);

	Ret|= Ilp0100_core_readRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)pAddr);

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_core_restoreSpiWindow1Addr(uint32_t Addr)
{
	ilp0100_error Ret=ILP0100_ERROR_NONE;
	ILP0100_LOG_FUNCTION_START((void*)&Addr);

	Ret|= Ilp0100_core_writeRegister((uint16_t)ILP0100_HOST_IF_SPI_BASE_ADDRESS_1, 4, (uint8_t*)&Addr);

	ILP0100_LOG_FUNCTION_END(Ret);
	return Ret;
}

ilp0100_error Ilp0100_core_strCmpr(const char* Str1, const char* Str2, uint8_t *pEqual)
{
	ilp0100_error Status = ILP0100_ERROR_NONE;
	uint16_t Str1Length = 0;
	uint16_t Str2Length = 0;
	uint16_t Index;

	*pEqual=0;
	Index=0;
	while(((uint8_t)(*(Str1+Index)))!=0)
	{
		Index=Index+1;
	}
	Str1Length=Index;

	Index=0;
	while(((uint8_t)(*(Str2+Index)))!=0)
	{
		Index=Index+1;
	}
	Str2Length=Index;

	if(Str1Length != Str2Length){
		*pEqual=0;
		return Status;
	}

	for(Index=0;Index<Str1Length;Index++){
		if(((uint8_t)(*(Str1+Index)))!=((uint8_t)(*(Str2+Index)))){
			*pEqual=0;
			return Status;
		}
	}

	*pEqual=1;
	return Status;
}

#ifdef __cplusplus
}
#endif   

















