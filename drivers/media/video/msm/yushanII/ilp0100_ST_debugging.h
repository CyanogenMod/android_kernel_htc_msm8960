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

#ifndef ILP0100_ST_DEBUGGING_H_
#define ILP0100_ST_DEBUGGING_H_


#include "ilp0100_ST_api.h"
#include "ilp0100_customer_platform.h"
#include "ilp0100_ST_core.h"

#define ILP0100_ERROR_LEVEL		0
#define ILP0100_DEBUG_LEVEL		1
#define ILP0100_VERBOSE_LEVEL	2

#define ILP0100_MAX_DEBUG_BUFFER_SIZE  4194304   
#define ILP0100_DEBUG_BUFFER_MARGIN  128



#ifdef ILP0100_DEBUG

extern uint8_t*	pIlp0100DebugBuffer;
extern uint32_t Ilp0100DebugLogSize;
extern bool_t	Ilp0100DebugStarted;

#define ILP0100_LOG_FUNCTION_START(...)	void* _pLogAllARgs_[]={ __VA_ARGS__ }; Ilp0100_loggingFunctionStart(__func__, _pLogAllARgs_);
#define ILP0100_LOG_FUNCTION_END(returned)	Ilp0100_loggingFunctionEnd(__func__, returned, _pLogAllARgs_)

#define ILP0100_LOG_ILP_ACCESS(RegisterName, ByteCount, pData, Returned)	Ilp0100_loggingFunctionIlpAccess(__func__, RegisterName, ByteCount, pData, Returned);

#define ILP0100_DEBUG_WRITE_IN_LOG(...) if((pIlp0100DebugBuffer!=0) && (Ilp0100DebugLogSize<(ILP0100_MAX_DEBUG_BUFFER_SIZE-ILP0100_DEBUG_BUFFER_MARGIN))){Ilp0100DebugLogSize=Ilp0100DebugLogSize+ILP0100_SPRINTF(pIlp0100DebugBuffer+Ilp0100DebugLogSize, ##__VA_ARGS__);}
#define ILP0100_INTERNAL_DEBUG_LOG(...) if(Ilp0100DebugStarted){Ilp0100_logDebugMessageStart(__func__);ILP0100_DEBUG_WRITE_IN_LOG(__VA_ARGS__);Ilp0100_logDebugMessageEnd();}
#define ILP0100_INTERNAL_ERROR_LOG(...) Ilp0100_logErrorMessageStart(__func__);ILP0100_DEBUG_WRITE_IN_LOG(__VA_ARGS__);Ilp0100_logErrorMessageEnd();

#define ILP0100_DEBUG_LOG(...)	ILP0100_INTERNAL_DEBUG_LOG(__VA_ARGS__);ILP0100_CUSTOMER_DEBUG_LOG(__VA_ARGS__)
#define ILP0100_ERROR_LOG(...)  ILP0100_INTERNAL_ERROR_LOG(__VA_ARGS__);ILP0100_CUSTOMER_ERROR_LOG(__VA_ARGS__)

#ifdef __cplusplus
extern "C"{
#endif   

ilp0100_error Ilp0100_loggingOpen(void);

ilp0100_error Ilp0100_loggingClose(void);

ilp0100_error Ilp0100_loggingStart(uint8_t DebugLevel);

ilp0100_error Ilp0100_loggingStop(void);

ilp0100_error Ilp0100_logDebugMessageStart(const char* pFunctionName);

ilp0100_error Ilp0100_logDebugMessageEnd(void);

ilp0100_error Ilp0100_logErrorMessageStart(const char* pFunctionName);

ilp0100_error Ilp0100_logErrorMessageEnd(void);

ilp0100_error Ilp0100_loggingGetSize(uint32_t* pLogSize);

ilp0100_error Ilp0100_loggingReadBack(uint8_t* pDebugLog, uint32_t* pLogSize);

ilp0100_error Ilp0100_loggingFunctionStart(const char* pFunctionName, void **pFuncArguments);

ilp0100_error Ilp0100_loggingFunctionEnd(const char* FunctionName, ilp0100_error ReturnedValue, void **pFuncArguments);

ilp0100_error Ilp0100_loggingFunctionIlpAccess(const char* pFunctionName, uint16_t RegisterName, uint16_t Count, uint8_t *pData, ilp0100_error ReturnedValue);

#ifdef __cplusplus
}
#endif   


#else

#define ILP0100_ERROR_LOG(...)  ILP0100_CUSTOMER_ERROR_LOG(__VA_ARGS__)
#define ILP0100_DEBUG_LOG(...)	ILP0100_CUSTOMER_DEBUG_LOG(__VA_ARGS__)

#define ILP0100_LOG_FUNCTION_START(...)	ILP0100_DEBUG_LOG("Entry %s", __func__)
#define ILP0100_LOG_FUNCTION_END(...)	ILP0100_DEBUG_LOG("Exit %s", __func__)

#define Ilp0100_loggingOpen()					ILP0100_ERROR_NONE
#define Ilp0100_loggingClose()					ILP0100_ERROR_NONE
#define Ilp0100_loggingStart(...)				ILP0100_ERROR_NONE
#define Ilp0100_loggingStop()					ILP0100_ERROR_NONE
#define Ilp0100_loggingGetSize(...) 			ILP0100_ERROR_NONE
#define Ilp0100_loggingReadBack(...)			ILP0100_ERROR_NONE
#define Ilp0100_loggingFunctionIlpAccess(...)	ILP0100_ERROR_NONE
#define ILP0100_LOG_ILP_ACCESS(...)				ILP0100_ERROR_NONE

#endif



#endif 
