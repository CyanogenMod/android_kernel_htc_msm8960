/*
 $License:
    Copyright (C) 2010 InvenSense Corporation, All Rights Reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
  $
 */


#ifndef MLTYPES_H
#define MLTYPES_H

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include "stdint_invensense.h"
#endif
#include "log.h"


typedef unsigned char tMLError;

#if defined(LINUX) || defined(__KERNEL__)
typedef unsigned int HANDLE;
#endif

#ifdef __KERNEL__
typedef HANDLE FILE;
#endif

#ifndef __cplusplus
#ifndef __KERNEL__
typedef int_fast8_t bool;
#endif
#endif


#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef DIM
#define DIM(array) (sizeof(array)/sizeof((array)[0]))
#endif

#define ERROR_NAME(x)   (#x)
#define ERROR_CHECK(x)                                                  \
	{								\
		if (ML_SUCCESS != x) {					\
			MPL_LOGE("[mpu_err]%s|%s|%d returning %d\n",	\
				__FILE__, __func__, __LINE__, x);	\
			return x;					\
		}							\
	}

#define ERROR_CHECK_FIRST(first, x)                                     \
	{ if (ML_SUCCESS == first) first = x; }

#define ML_SUCCESS                       (0)
#define ML_ERROR                         (1)

#define ML_ERROR_INVALID_PARAMETER       (2)
#define ML_ERROR_FEATURE_NOT_ENABLED     (3)
#define ML_ERROR_FEATURE_NOT_IMPLEMENTED (4)
#define ML_ERROR_DMP_NOT_STARTED         (6)
#define ML_ERROR_DMP_STARTED             (7)
#define ML_ERROR_NOT_OPENED              (8)
#define ML_ERROR_OPENED                  (9)
#define ML_ERROR_INVALID_MODULE         (10)
#define ML_ERROR_MEMORY_EXAUSTED        (11)
#define ML_ERROR_DIVIDE_BY_ZERO         (12)
#define ML_ERROR_ASSERTION_FAILURE      (13)
#define ML_ERROR_FILE_OPEN              (14)
#define ML_ERROR_FILE_READ              (15)
#define ML_ERROR_FILE_WRITE             (16)
#define ML_ERROR_INVALID_CONFIGURATION  (17)

#define ML_ERROR_SERIAL_CLOSED          (20)
#define ML_ERROR_SERIAL_OPEN_ERROR      (21)
#define ML_ERROR_SERIAL_READ            (22)
#define ML_ERROR_SERIAL_WRITE           (23)
#define ML_ERROR_SERIAL_DEVICE_NOT_RECOGNIZED  (24)

#define ML_ERROR_SM_TRANSITION          (25)
#define ML_ERROR_SM_IMPROPER_STATE      (26)

#define ML_ERROR_FIFO_OVERFLOW          (30)
#define ML_ERROR_FIFO_FOOTER            (31)
#define ML_ERROR_FIFO_READ_COUNT        (32)
#define ML_ERROR_FIFO_READ_DATA         (33)

#define ML_ERROR_MEMORY_SET             (40)

#define ML_ERROR_LOG_MEMORY_ERROR       (50)
#define ML_ERROR_LOG_OUTPUT_ERROR       (51)

#define ML_ERROR_OS_BAD_PTR             (60)
#define ML_ERROR_OS_BAD_HANDLE          (61)
#define ML_ERROR_OS_CREATE_FAILED       (62)
#define ML_ERROR_OS_LOCK_FAILED         (63)

#define ML_ERROR_COMPASS_DATA_OVERFLOW  (70)
#define ML_ERROR_COMPASS_DATA_UNDERFLOW (71)
#define ML_ERROR_COMPASS_DATA_NOT_READY (72)
#define ML_ERROR_COMPASS_DATA_ERROR     (73)

#define ML_ERROR_CALIBRATION_LOAD       (75)
#define ML_ERROR_CALIBRATION_STORE      (76)
#define ML_ERROR_CALIBRATION_LEN        (77)
#define ML_ERROR_CALIBRATION_CHECKSUM   (78)

#define ML_ERROR_ACCEL_DATA_OVERFLOW    (79)
#define ML_ERROR_ACCEL_DATA_UNDERFLOW   (80)
#define ML_ERROR_ACCEL_DATA_NOT_READY   (81)
#define ML_ERROR_ACCEL_DATA_ERROR       (82)



#endif				
