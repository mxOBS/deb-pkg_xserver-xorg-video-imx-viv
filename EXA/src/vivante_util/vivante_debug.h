/****************************************************************************
*
*    Copyright 2012 - 2017 Vivante Corporation, Santa Clara, California.
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

    /*******************************************************************************
     *
     * DEBUG Macros (START)
     *
     ******************************************************************************/
#ifdef DEBUG
enum { VIV_NOMSG=0,VIV_XORG=1, VIV_TERM=2};
extern unsigned int  vivEnableDump;
#define DEBUGP(x, args ...) do { \
                        if (vivEnableDump == VIV_TERM) \
                        { \
                            fprintf(stderr, x, ## args); \
                        } else { \
                            if ( vivEnableDump == VIV_XORG ) \
                                xf86DrvMsg(0, X_INFO, x, ## args); \
                        } }while(0)

#else
#define DEBUGP(x, args ...)
#endif

#ifdef DEBUG
#define TRACE_ENTER() \
    do { DEBUGP("ENTERED FUNCTION : %s,Line %d\n", __FUNCTION__,__LINE__);} while (0)
#define TRACE_EXIT(val) \
    do { DEBUGP("EXITED FUNCTION : %s,Line %d\n", __FUNCTION__,__LINE__); return val;  } while (0)
#define TRACE_INFO(x, args ...) \
    do { DEBUGP("[INFO : %s(), %s:%u]\n"x, __FILE__, __FUNCTION__ ,__LINE__, ## args); } while (0)
#define TRACE_ERROR(x, args ...) \
    do {  DEBUGP("[ERROR : %s(), %s:%u]\n"x, __FILE__, __FUNCTION__ ,__LINE__, ## args); } while (0)
#else
#define TRACE_ENTER()
#define TRACE_EXIT(val) \
    do { return val;  } while (0)
#define TRACE_INFO(x, args ...)
#define TRACE_ERROR(x, args ...)
#endif


#ifdef __cplusplus
}
#endif

#endif  /* VIVANTE_DEBUG_H */

