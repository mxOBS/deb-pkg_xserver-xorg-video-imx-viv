/****************************************************************************
*
*    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/


/*
 * File:   vivante_debug.h
 * Author: vivante
 *
 * Created on December 24, 2011, 12:50 PM
 */

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
//#define VIVFBDEV_DEBUG
#ifdef VIVFBDEV_DEBUG
#define DEBUGP(x, args ...) fprintf(stderr, "[%s(), %s:%u]\n\n" \
x, __FILE__, __FUNCTION__ ,__LINE__, ## args)
#else
#define DEBUGP(x, args ...)
#endif

#ifdef VIVFBDEV_DEBUG
#define TRACE_ENTER() \
    do {  PrintEnter(); DEBUGP("ENTERED FUNCTION : %s\n", __FUNCTION__); } while (0)
#define TRACE_EXIT(val) \
    do { PrintExit(); DEBUGP("EXITED FUNCTION : %s\n", __FUNCTION__); return val;  } while (0)
#define TRACE_INFO(x, args ...) \
    do { fprintf(stderr, "[INFO : %s(), %s:%u]\n\n" x, __FILE__, __FUNCTION__ ,__LINE__, ## args); } while (0)
#define TRACE_ERROR(x, args ...) \
    do {  fprintf(stderr, "[ERROR : %s(), %s:%u]\n\n" x, __FILE__, __FUNCTION__ ,__LINE__, ## args); } while (0)
#else
#define TRACE_ENTER() \
    do {  ; } while (0)
#define TRACE_EXIT(val) \
    do { return val;  } while (0)
#define TRACE_INFO(x, args ...) \
    do { ;} while (0)
#define TRACE_ERROR(x, args ...) \
    do {  ;} while (0)
#endif



    /*******************************************************************************
     *
     * DEBUG Macros (END)
     *
     ******************************************************************************/

    void PrintEnter();
    void PrintExit();
    void PrintString(const char* str);

#ifdef __cplusplus
}
#endif

#endif	/* VIVANTE_DEBUG_H */

