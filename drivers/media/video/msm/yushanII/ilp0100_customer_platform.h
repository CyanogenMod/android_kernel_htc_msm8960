/*
*******************************************************************************
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

#ifndef ILP0100_CUSTOMER_PLATFORM_H_
#define ILP0100_CUSTOMER_PLATFORM_H_


#include "ilp0100_ST_core.h"
#include "ilp0100_customer_platform_types.h"
#include "ilp0100_ST_definitions.h"
#include "ilp0100_ST_error_codes.h"
#include "ilp0100_ST_debugging.h"
#include <linux/module.h>
#include <linux/vmalloc.h>
#include "../sensors/msm_sensor.h"

#undef ILP0100_DEBUG_MSG
#define ILP0100_ERROR_MSG
#define ILP0100_DEBUG
#define KERNEL_DEBUG
#define KERNEL_ERROR

#ifdef LINUX_TEST
extern char gbuf[1000];
#endif


#ifdef ILP0100_DEBUG_MSG
#ifdef KERNEL_DEBUG
#define ILP0100_CUSTOMER_DEBUG_LOG(fmt,args...) pr_info(KERN_DEBUG "[CAM] File:%s(Line:%i)\n"fmt,__FILE__,__LINE__,##args);
#else
#ifndef LINUX_TEST
#define ILP0100_CUSTOMER_DEBUG_LOG(fmt,args...) printk("\nFile:%s(Line:%i)\n"fmt,__FILE__,__LINE__,##args);
#else
#define ILP0100_CUSTOMER_DEBUG_LOG(fmt,args...) { sprintf (gbuf,fmt,##args);myPrintMsg(gbuf); }
#endif
#endif
#else
#define ILP0100_CUSTOMER_DEBUG_LOG(...) ;
#endif

#ifdef ILP0100_ERROR_MSG
#ifdef KERNEL_ERROR
#define ILP0100_CUSTOMER_ERROR_LOG(fmt,args...) pr_err(KERN_ERR "[CAM] File:%s(Line:%i)\n"fmt,__FILE__,__LINE__,##args);
#else
#ifndef LINUX_TEST
#define ILP0100_CUSTOMER_ERROR_LOG(fmt,args...) printk("\nFile:%s(Line:%i)\n"fmt,__FILE__,__LINE__,##args);
#else
#define ILP0100_CUSTOMER_ERROR_LOG(fmt,args...) { sprintf (gbuf,fmt,##args);myPrintMsg(gbuf); }
#endif
#endif
#else
#define ILP0100_CUSTOMER_ERROR_LOG(...) ;
#endif

#ifdef ILP0100_DEBUG
#define ILP0100_BUFFERCREATE(SizeBytes)		kmalloc(SizeBytes, GFP_KERNEL)
#define ILP0100_BUFFERFREE(pBuffer) 		kfree(pBuffer)
#define ILP0100_SPRINTF(buf,fmt,args...)	sprintf(buf,fmt,##args)
#endif



#ifdef LINUX_TEST
extern int myPrintMsg(char *);
#endif

ilp0100_error Ilp0100_readFirmware(struct msm_sensor_ctrl_t *s_ctrl, Ilp0100_structInitFirmware *InitFirmwareData);


ilp0100_error Ilp0100_readFileInBuffer(struct msm_sensor_ctrl_t *s_ctrl, uint8_t *pFileName,uint8_t **pAdd, uint32_t* SizeOfBuffer);

ilp0100_error Ilp0100_interruptHandler(void);

ilp0100_error Ilp0100_interruptManager(uint32_t InterruptId, bool_t Pin);

ilp0100_error Ilp0100_GetTimeStamp(uint32_t *pTimeStamp);

void Ilp0100_release_firmware(void);

#ifdef ILP0100_DEBUG
ilp0100_error Ilp0100_DumpLogInFile(uint8_t *pFileName);
#endif
#endif 
