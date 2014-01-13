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

#ifndef ILP0100_ST_CORE_H_
#define ILP0100_ST_CORE_H_

#include "ilp0100_ST_api.h"
#include "ilp0100_ST_definitions.h"
#include "ilp0100_ST_firmware_elements.h"

#define 	SPI_BASE_ADD_0		0x8000
#define 	SPI_BASE_ADD_1 		0xC000


typedef struct {
                uint32_t magic_id ; 
                uint32_t load_add ;
                uint32_t code_size ;
                uint16_t crc ;
                uint16_t file_offset ;
                uint8_t  major_version;
                uint8_t  minor_version;
                uint16_t pad ;

} file_hdr_t;
ilp0100_error Ilp0100_core_initClocks(const Ilp0100_structInit Init, const Ilp0100_structSensorParams SensorParams);

ilp0100_error Ilp0100_core_uploadFirmware(const Ilp0100_structInitFirmware InitFirmware);

ilp0100_error Ilp0100_core_checkCrc(uint8_t* pData, uint32_t SizeInBytes, uint16_t CrcExpected);

ilp0100_error Ilp0100_core_bootXp70(void);

ilp0100_error Ilp0100_core_defineMode(const Ilp0100_structFrameFormat FrameFormat);

ilp0100_error Ilp0100_core_stop(void);

ilp0100_error Ilp0100_core_configDefcor(const Ilp0100_structDefcorConfig DefcorConfig, uint8_t HDRMode);

ilp0100_error Ilp0100_core_updateDefcor(const Ilp0100_structDefcorParams DefcorParams, uint8_t HDRMode);

ilp0100_error Ilp0100_core_configChannelOffset(const Ilp0100_structChannelOffsetConfig ChannelOffsetConfig, uint8_t HDRMode);

ilp0100_error Ilp0100_core_updateChannelOffset(const Ilp0100_structChannelOffsetParams ChannelOffsetParams, uint8_t HDRMode);

ilp0100_error Ilp0100_core_configGlace(const Ilp0100_structGlaceConfig GlaceConfig, uint8_t HDRMode);

ilp0100_error Ilp0100_core_configHist(const Ilp0100_structHistConfig HistConfig, uint8_t HDRMode);

ilp0100_error Ilp0100_core_configHdrMerge(const Ilp0100_structHdrMergeConfig HdrMergeConfig);

ilp0100_error Ilp0100_core_updateHdrMerge(const Ilp0100_structHdrMergeParams HdrMergeParams);

ilp0100_error Ilp0100_core_configCls(const Ilp0100_structClsConfig ClsConfig);

ilp0100_error Ilp0100_core_updateCls(const Ilp0100_structClsParams ClsParams);

ilp0100_error Ilp0100_core_configToneMapping(const Ilp0100_structToneMappingConfig ToneMappingConfig);

ilp0100_error Ilp0100_core_updateToneMapping(const Ilp0100_structToneMappingParams ToneMappingParams);

ilp0100_error Ilp0100_core_updateSensorParams(const Ilp0100_structFrameParams FrameParams, uint8_t HDRMode);

ilp0100_error Ilp0100_core_startTestMode(uint16_t IpsBypass, uint16_t TestMode);

ilp0100_error Ilp0100_core_readBackGlaceStatistics(Ilp0100_structGlaceStatsData *pGlaceStatsData, uint8_t HDRMode);

ilp0100_error Ilp0100_core_readBackHistStatistics(Ilp0100_structHistStatsData *pHistStatsData, uint8_t HDRMode);

ilp0100_error Ilp0100_core_setVirtualChannel(uint8_t VirtualChannel, uint8_t HDRMode);

ilp0100_error Ilp0100_core_setHDRFactor(uint8_t HDRFactor);

ilp0100_error Ilp0100_core_reset(void);

ilp0100_error Ilp0100_core_onTheFlyReset(void);

ilp0100_error Ilp0100_core_interruptEnable(uint32_t InterruptSetMask, bool_t Pin);

ilp0100_error Ilp0100_core_interruptDisable(uint32_t InterruptClrMask, bool_t Pin);

ilp0100_error Ilp0100_core_interruptReadStatus(uint32_t* pInterruptId, uint8_t Pin);

ilp0100_error Ilp0100_core_interruptClearStatus(uint32_t InterruptId, uint8_t Pin);

ilp0100_error Ilp0100_core_getApiVersionNumber(uint8_t* pMajorNumber, uint8_t* pMinorNumber);

ilp0100_error Ilp0100_core_getFirmwareVersionumber(uint8_t* pMajorNumber, uint8_t* pMinorNumber);

ilp0100_error Ilp0100_core_readRegister(uint16_t RegisterName, uint16_t Count, uint8_t *pData);

ilp0100_error Ilp0100_core_writeRegister(uint16_t RegisterName, uint16_t Count, uint8_t *pData);

ilp0100_error Ilp0100_core_enableIlp0100SensorClock(bool_t SensorInterface);

ilp0100_error Ilp0100_core_disableIlp0100SensorClock(bool_t SensorInterface);

/*!
 * \fn 			ilp0100_error Ilp0100_core_saveSpiWindow1Addr(uint32_t *pAddr)
 * \brief		Core function to save Spi Window 1 Address
 * \ingroup		Core_Functions
 * \param[out] 	pAddr	: Present SPI window Address retrieved and written on pData.
 * \retval 		ILP0100_ERROR_NONE : Success
 * \retval 		"Other Error Code" : Failure
 */
ilp0100_error Ilp0100_core_saveSpiWindow1Addr(uint32_t *pAddr);

ilp0100_error Ilp0100_core_restoreSpiWindow1Addr(uint32_t Addr);

ilp0100_error Ilp0100_core_strCmpr(const char* Str1, const char* Str2, uint8_t *pEqual);

#endif 
