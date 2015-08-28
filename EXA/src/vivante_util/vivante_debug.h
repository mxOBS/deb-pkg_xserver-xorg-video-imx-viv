 /****************************************************************************
 *
 *    Copyright 2012 - 2015 Vivante Corporation, Santa Clara, California.
 *    All Rights Reserved.
 *
 *    Permission is hereby granted, free of charge, to any person obtaining
 *    a copy of this software and associated documentation files (the
 *    'Software'), to deal in the Software without restriction, including
 *    without limitation the rights to use, copy, modify, merge, publish,
 *    distribute, sub license, and/or sell copies of the Software, and to
 *    permit persons to whom the Software is furnished to do so, subject
 *    to the following conditions:
 *
 *    The above copyright notice and this permission notice (including the
 *    next paragraph) shall be included in all copies or substantial
 *    portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 *    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 *    IN NO EVENT SHALL VIVANTE AND/OR ITS SUPPLIERS BE LIABLE FOR ANY
 *    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/


#ifndef VIVANTE_DEBUG_H
#define VIVANTE_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>

//#define ENABLE_LOG
//#define DRAWING_STATISTICS

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
#define TRACE_INFO(...) //  LOGD
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

#include "HAL/gc_hal_version.h"

#define GPU_VERSION_GREATER_THAN(major, minor, patch, build) \
    (gcvVERSION_MAJOR > (major) || (gcvVERSION_MAJOR == (major) && \
    (gcvVERSION_MINOR > (minor) || (gcvVERSION_MINOR == (minor) && \
    (gcvVERSION_PATCH > (patch) || (gcvVERSION_PATCH == (patch) && \
    gcvVERSION_BUILD >= (build)))))))

#include "vivante_exa.h"
#include "vivante.h"

enum PixmapCachePolicy
{
    NONCACHEABLE,
    WRITETHROUGH,
    WRITEALLOC, // system default
    UNKNOWNCACHE
};

enum PixmapCachePolicy getPixmapCachePolicy();
int isGpuSyncMode();
void initPixmapQueue();
void freePixmapQueue();
void queuePixmapToGpu(Viv2DPixmapPtr vpixmap);
void preGpuDraw(VivPtr pViv, Viv2DPixmapPtr vpixmap, int bSrc);
void postGpuDraw(VivPtr pViv);
void preCpuDraw(VivPtr pViv, Viv2DPixmapPtr vivpixmap);
void postCpuDraw(VivPtr pViv, Viv2DPixmapPtr vivpixmap);

#if defined(DRAWING_STATISTICS)
void initDrawingStatistics();
void freeDrawingStatistics();
void startDrawingSolid(int width, int height);
void endDrawingSolid();
void startDrawingCopy(int width, int height, int rop);
void endDrawingCopy();
void startDrawingUpload(int width, int height);
void endDrawingUpload();
void startDrawingCompose(int width, int height);
void endDrawingCompose();
void startDrawingSW();
void endDrawingSW();
#else
#define initDrawingStatistics(...)
#define freeDrawingStatistics(...)
#define startDrawingSolid(...)
#define endDrawingSolid(...)
#define startDrawingCopy(...)
#define endDrawingCopy(...)
#define startDrawingUpload(...)
#define endDrawingUpload(...)
#define startDrawingCompose(...)
#define endDrawingCompose(...)
#define startDrawingSW(...)
#define endDrawingSW(...)
#endif

#ifdef __cplusplus
}
#endif

#endif    /* VIVANTE_DEBUG_H */

