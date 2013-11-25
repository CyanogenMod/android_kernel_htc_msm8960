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

#ifndef ILP0100_ST_API_H_
#define ILP0100_ST_API_H_

#include "ilp0100_ST_definitions.h"
#include "ilp0100_ST_debugging.h"
#include "ilp0100_customer_platform.h"
#include "ilp0100_customer_sensor_config.h"
#include "ilp0100_customer_iq_settings.h"
#include "ilp0100_ST_error_codes.h"
#include "ilp0100_ST_register_map.h"

#ifdef __cplusplus
extern "C"{
#endif   


ilp0100_error Ilp0100_init(const Ilp0100_structInit Init, Ilp0100_structInitFirmware InitFirmware, const Ilp0100_structSensorParams SensorParams);

ilp0100_error Ilp0100_defineMode(const Ilp0100_structFrameFormat FrameFormat);

ilp0100_error Ilp0100_stop(void);

ilp0100_error Ilp0100_configDefcorShortOrNormal(const Ilp0100_structDefcorConfig ShortOrNormalDefcorConfig);

ilp0100_error Ilp0100_configDefcorLong(const Ilp0100_structDefcorConfig LongDefcorConfig);

ilp0100_error Ilp0100_updateDefcorShortOrNormal(const Ilp0100_structDefcorParams ShortOrNormalDefcorParams);

ilp0100_error Ilp0100_updateDefcorLong(const Ilp0100_structDefcorParams LongDefcorParams);

ilp0100_error Ilp0100_configChannelOffsetShortOrNormal(const Ilp0100_structChannelOffsetConfig ShortOrNormalChannelOffsetConfig);

ilp0100_error Ilp0100_configChannelOffsetLong(const Ilp0100_structChannelOffsetConfig LongChannelOffsetConfig);

ilp0100_error Ilp0100_updateChannelOffsetShortOrNormal(const Ilp0100_structChannelOffsetParams ShortOrNormalChannelOffsetParams);

ilp0100_error Ilp0100_updateChannelOffsetLong(const Ilp0100_structChannelOffsetParams LongChannelOffsetParams);
ilp0100_error Ilp0100_configGlaceShortOrNormal(const Ilp0100_structGlaceConfig ShortOrNormalGlaceConfig);

ilp0100_error Ilp0100_configGlaceLong(const Ilp0100_structGlaceConfig LongGlaceConfig);

ilp0100_error Ilp0100_configHistShortOrNormal(const Ilp0100_structHistConfig ShortOrNormalHistConfig);

ilp0100_error Ilp0100_configHistLong(const Ilp0100_structHistConfig LongHistConfig);

ilp0100_error Ilp0100_configHdrMerge(const Ilp0100_structHdrMergeConfig HdrMergeConfig);

ilp0100_error Ilp0100_updateHdrMerge(const Ilp0100_structHdrMergeParams HdrMergeParams);

ilp0100_error Ilp0100_configCls(const Ilp0100_structClsConfig ClsConfig);

ilp0100_error Ilp0100_updateCls(const Ilp0100_structClsParams ClsParams);

ilp0100_error Ilp0100_configToneMapping(const Ilp0100_structToneMappingConfig ToneMappingConfig);

ilp0100_error Ilp0100_updateToneMapping(const Ilp0100_structToneMappingParams ToneMappingParams);

ilp0100_error Ilp0100_updateSensorParamsShortOrNormal(const Ilp0100_structFrameParams ShortOrNormalFrameParams);

ilp0100_error Ilp0100_updateSensorParamsLong(const Ilp0100_structFrameParams LongFrameParams);

ilp0100_error Ilp0100_readBackGlaceStatisticsShortOrNormal(Ilp0100_structGlaceStatsData *pShortOrNormalGlaceStatsData);

ilp0100_error Ilp0100_readBackGlaceStatisticsLong(Ilp0100_structGlaceStatsData *pLongGlaceStatsData);

ilp0100_error Ilp0100_readBackHistStatisticsShortOrNormal(Ilp0100_structHistStatsData *pShortOrNormalHistStatsData);

ilp0100_error Ilp0100_readBackHistStatisticsLong(Ilp0100_structHistStatsData *pLongHistStatsData);

ilp0100_error Ilp0100_setVirtualChannelShortOrNormal(uint8_t VirtualChannel);

ilp0100_error Ilp0100_setVirtualChannelLong(uint8_t VirtualChannel);

ilp0100_error Ilp0100_setHDRFactor(uint8_t HDRFactor);

ilp0100_error Ilp0100_reset(void);

ilp0100_error Ilp0100_onTheFlyReset(void);


ilp0100_error Ilp0100_interruptEnable(uint32_t InterruptSetMask, bool_t Pin);

ilp0100_error Ilp0100_interruptDisable(uint32_t InterruptClrMask, bool_t Pin);

ilp0100_error Ilp0100_interruptReadStatus(uint32_t* pInterruptId, uint8_t Pin);

ilp0100_error Ilp0100_interruptClearStatus(uint32_t InterruptId, uint8_t Pin);


ilp0100_error Ilp0100_getApiVersionNumber(uint8_t* pMajorNumber, uint8_t* pMinorNumber);

ilp0100_error Ilp0100_getFirmwareVersionumber(uint8_t* pMajorNumber, uint8_t* pMinorNumber);

ilp0100_error Ilp0100_startTestMode(uint16_t IpsBypass, uint16_t TestMode);

ilp0100_error Ilp0100_readRegister(uint32_t RegisterName, uint8_t *pData);

ilp0100_error Ilp0100_writeRegister(uint32_t RegisterName, uint8_t *pData);

ilp0100_error Ilp0100_enableIlp0100SensorClock(bool_t SensorInterface);

ilp0100_error Ilp0100_disableIlp0100SensorClock(bool_t SensorInterface);

#ifdef __cplusplus
}
#endif   





#endif 
