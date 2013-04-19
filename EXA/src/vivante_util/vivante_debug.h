/****************************************************************************
*
*    Copyright (C) 2005 - 2013 by Vivante Corp.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the license, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****************************************************************************/


#ifndef VIVANTE_DEBUG_H
#define VIVANTE_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>

//#define ENABLE_LOG
#if defined(ENABLE_LOG)
void OpenLog();
void CloseLog();
void LogText(const char *fmt, ...);

#define LOG_START OpenLog
#define LOG_END   CloseLog
#define LOG       LogText
#define LOGD LOG
#define LOGW LOG
#define LOGE LOG
#define TRACE_INFO  LOGD
#define TRACE_ERROR LOGE
#else
#define LOG_START(...)
#define LOG_END(...)
#define LOG(...)
#define LOGD(...)
#define LOGW(...)
#define LOGE(...)
#define TRACE_INFO(...)
#define TRACE_ERROR(...)
#endif
#define DEBUGP(x, args ...)

#define TRACE_ENTER()
#define TRACE_EXIT(val) \
    do { return val;  } while (0)

#define FSLASSERT(x) do { if(!(x)) {LOG("Assertion failed @%s:%d\n", __FILE__, __LINE__);} } while(0);

#ifdef __cplusplus
}
#endif

#endif	/* VIVANTE_DEBUG_H */

