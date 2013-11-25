/*
 * Copyright (C) 2010 InvenSense Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _LIBS_CUTILS_MPL_LOG_H
#define _LIBS_CUTILS_MPL_LOG_H

#include <stdarg.h>

#ifdef ANDROID
#include <utils/Log.h>		
#endif

#ifdef __KERNEL__
#include <linux/kernel.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


#ifndef MPL_LOG_NDEBUG
#ifdef NDEBUG
#define MPL_LOG_NDEBUG 1
#else
#define MPL_LOG_NDEBUG 0
#endif
#endif

#ifdef __KERNEL__
#define MPL_LOG_UNKNOWN MPL_LOG_VERBOSE
#define MPL_LOG_DEFAULT KERN_DEFAULT
#define MPL_LOG_VERBOSE KERN_CONT
#define MPL_LOG_DEBUG   KERN_NOTICE
#define MPL_LOG_INFO    KERN_INFO
#define MPL_LOG_WARN    KERN_WARNING
#define MPL_LOG_ERROR   KERN_ERR
#define MPL_LOG_SILENT  MPL_LOG_VERBOSE

#else
#define MPL_LOG_UNKNOWN (0)
#define MPL_LOG_DEFAULT (1)
#define MPL_LOG_VERBOSE (2)
#define MPL_LOG_DEBUG (3)
#define MPL_LOG_INFO (4)
#define MPL_LOG_WARN (5)
#define MPL_LOG_ERROR (6)
#define MPL_LOG_SILENT (8)
#endif


#ifndef MPL_LOG_TAG
#ifdef __KERNEL__
#define MPL_LOG_TAG
#else
#define MPL_LOG_TAG NULL
#endif
#endif


#ifndef MPL_LOGV
#if MPL_LOG_NDEBUG
#define MPL_LOGV(...) ((void)0)
#else
#define MPL_LOGV(...) ((void)MPL_LOG(LOG_VERBOSE, MPL_LOG_TAG, __VA_ARGS__))
#endif
#endif

#ifndef CONDITION
#define CONDITION(cond)     ((cond) != 0)
#endif

#ifndef MPL_LOGV_IF
#if MPL_LOG_NDEBUG
#define MPL_LOGV_IF(cond, ...)   ((void)0)
#else
#define MPL_LOGV_IF(cond, ...) \
	((CONDITION(cond))						\
		? ((void)MPL_LOG(LOG_VERBOSE, MPL_LOG_TAG, __VA_ARGS__)) \
		: (void)0)
#endif
#endif

#ifndef MPL_LOGD
#define MPL_LOGD(...) ((void)MPL_LOG(LOG_DEBUG, MPL_LOG_TAG, __VA_ARGS__))
#endif

#ifndef MPL_LOGD_IF
#define MPL_LOGD_IF(cond, ...) \
	((CONDITION(cond))					       \
		? ((void)MPL_LOG(LOG_DEBUG, MPL_LOG_TAG, __VA_ARGS__)) \
		: (void)0)
#endif

#ifndef MPL_LOGI
#define MPL_LOGI(...) ((void)MPL_LOG(LOG_INFO, MPL_LOG_TAG, __VA_ARGS__))
#endif

#ifndef MPL_LOGI_IF
#define MPL_LOGI_IF(cond, ...) \
	((CONDITION(cond))                                              \
		? ((void)MPL_LOG(LOG_INFO, MPL_LOG_TAG, __VA_ARGS__))   \
		: (void)0)
#endif

#ifndef MPL_LOGW
#define MPL_LOGW(...) ((void)MPL_LOG(LOG_WARN, MPL_LOG_TAG, __VA_ARGS__))
#endif

#ifndef MPL_LOGW_IF
#define MPL_LOGW_IF(cond, ...) \
	((CONDITION(cond))					       \
		? ((void)MPL_LOG(LOG_WARN, MPL_LOG_TAG, __VA_ARGS__))  \
		: (void)0)
#endif

#ifndef MPL_LOGE
#define MPL_LOGE(...) ((void)MPL_LOG(LOG_ERROR, MPL_LOG_TAG, __VA_ARGS__))
#endif

#ifndef MPL_LOGE_IF
#define MPL_LOGE_IF(cond, ...) \
	((CONDITION(cond))					       \
		? ((void)MPL_LOG(LOG_ERROR, MPL_LOG_TAG, __VA_ARGS__)) \
		: (void)0)
#endif


#define MPL_LOG_ALWAYS_FATAL_IF(cond, ...) \
	((CONDITION(cond))					   \
		? ((void)android_printAssert(#cond, MPL_LOG_TAG, __VA_ARGS__)) \
		: (void)0)

#define MPL_LOG_ALWAYS_FATAL(...) \
	(((void)android_printAssert(NULL, MPL_LOG_TAG, __VA_ARGS__)))

#if MPL_LOG_NDEBUG

#define MPL_LOG_FATAL_IF(cond, ...) ((void)0)
#define MPL_LOG_FATAL(...)          ((void)0)

#else

#define MPL_LOG_FATAL_IF(cond, ...) MPL_LOG_ALWAYS_FATAL_IF(cond, __VA_ARGS__)
#define MPL_LOG_FATAL(...)          MPL_LOG_ALWAYS_FATAL(__VA_ARGS__)

#endif

#define MPL_LOG_ASSERT(cond, ...) MPL_LOG_FATAL_IF(!(cond), __VA_ARGS__)


#ifndef MPL_LOG
#define MPL_LOG(priority, tag, ...) \
    MPL_LOG_PRI(priority, tag, __VA_ARGS__)
#endif

#ifndef MPL_LOG_PRI
#ifdef ANDROID
#define MPL_LOG_PRI(priority, tag, ...) \
	LOG(priority, tag, __VA_ARGS__)
#elif defined __KERNEL__
#define MPL_LOG_PRI(priority, tag, ...) \
	printk(MPL_##priority tag __VA_ARGS__)
#else
#define MPL_LOG_PRI(priority, tag, ...) \
	_MLPrintLog(MPL_##priority, tag, __VA_ARGS__)
#endif
#endif

#ifndef MPL_LOG_PRI_VA
#ifdef ANDROID
#define MPL_LOG_PRI_VA(priority, tag, fmt, args) \
    android_vprintLog(priority, NULL, tag, fmt, args)
#elif defined __KERNEL__
#define MPL_LOG_PRI_VA(priority, tag, fmt, args) \
    vprintk(MPL_##priority tag fmt, args)
#else
#define MPL_LOG_PRI_VA(priority, tag, fmt, args) \
    _MLPrintVaLog(priority, NULL, tag, fmt, args)
#endif
#endif



#ifndef ANDROID
	int _MLPrintLog(int priority, const char *tag, const char *fmt,
			...);
	int _MLPrintVaLog(int priority, const char *tag, const char *fmt,
			  va_list args);
	int _MLWriteLog(const char *buf, int buflen);
#endif

#ifdef __cplusplus
}
#endif
#endif				
