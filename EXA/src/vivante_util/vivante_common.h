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


#ifndef VIVANTE_COMMON_H
#define VIVANTE_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif
    /*Normal Includes*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


    /* X Window */
#include "xorg-server.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86xv.h"
#include "xf86RandR12.h"
#include "xorg-server.h"


#include "mipointer.h"
#include "micmap.h"
#include "mipointrst.h"
#include "inputstr.h"
#include "colormapst.h"
#include "xf86cmap.h"
#include "shadow.h"
#include "dgaproc.h"

    /* for visuals */
#include "fb.h"

#if GET_ABI_MAJOR(ABI_VIDEODRV_VERSION) < 6
#include "xf86Resources.h"
#include "xf86RAC.h"
#endif

    /*FrameBuffer*/
#include "fbdevhw.h"
    /*For EXA*/
#include "exa.h"

    /*For Cursor*/
#include "xf86.h"
#include "xf86Cursor.h"
#include "xf86Crtc.h"
#include "cursorstr.h"

    /*Debug*/
#include "vivante_debug.h"

#ifndef XF86_HAS_SCRN_CONV
    #define xf86ScreenToScrn(s) xf86Screens[(s)->myNum]
    #define xf86ScrnToScreen(s) screenInfo.screens[(s)->scrnIndex]
#endif

#ifndef XF86_SCRN_INTERFACE
#define SCRN_ARG_TYPE int
#define SCRN_INFO_PTR(arg1) ScrnInfoPtr pScrn = xf86Screens[(arg1)]
#define SCREEN_INIT_ARGS_DECL int scrnIndex, ScreenPtr pScreen, int argc, char **argv
#define CLOSE_SCREEN_ARGS_DECL int scrnIndex, ScreenPtr pScreen
#define CLOSE_SCREEN_ARGS scrnIndex, pScreen
#define ADJUST_FRAME_ARGS_DECL int arg, int x, int y, int flags
#define FBDEVHWADJUSTFRAME_ARGS(x, y) scrnIndex, (x), (y), 0
#define SWITCH_MODE_ARGS_DECL int arg, DisplayModePtr mode, int flags
#define ADJUST_FRAME_ARGS(scrn, x, y) (scrn), (x), (y), 0
#define SWITCH_MODE_ARGS(arg, mode) (arg), (mode), 0
#define FREE_SCREEN_ARGS_DECL int arg, int flags
#define VT_FUNC_ARGS_DECL int arg, int flags
#define VALID_MODE_DECL int arg, DisplayModePtr mode, Bool verbose, int flags
#define PM_EVENT_DECL int arg, pmEvent event, Bool undo
#define VT_FUNC_ARGS(flags) pScrn->scrnIndex, (flags)
#else
#define SCRN_ARG_TYPE ScrnInfoPtr
#define SCRN_INFO_PTR(arg1) ScrnInfoPtr pScrn = (arg1)
#define SCREEN_INIT_ARGS_DECL ScreenPtr pScreen, int argc, char **argv
#define CLOSE_SCREEN_ARGS_DECL ScreenPtr pScreen
#define CLOSE_SCREEN_ARGS pScreen
#define ADJUST_FRAME_ARGS_DECL ScrnInfoPtr arg, int x, int y
#define FBDEVHWADJUSTFRAME_ARGS(x, y) pScrn, (x), (y)
#define SWITCH_MODE_ARGS_DECL ScrnInfoPtr arg, DisplayModePtr mode
#define ADJUST_FRAME_ARGS(scrn, x, y) (scrn), (x), (y)
#define SWITCH_MODE_ARGS(arg, mode) (arg), (mode)
#define FREE_SCREEN_ARGS_DECL ScrnInfoPtr arg
#define VT_FUNC_ARGS_DECL ScrnInfoPtr arg
#define VALID_MODE_DECL ScrnInfoPtr arg, DisplayModePtr mode, Bool verbose, int flags
#define PM_EVENT_DECL ScrnInfoPtr arg, pmEvent event, Bool undo
#define VT_FUNC_ARGS(flags) pScrn
#endif

#if XF86_HAS_SCRN_CONV
#define GET_PSCR(pScreen) (xf86ScreenToScrn(pScreen))
#define GET_SCRINDEX(pScrn) int scrnIndex; \
                            do{     scrnIndex = pScrn->scrnIndex; }while(0)


#else
#define GET_PSCR(pScreen) (xf86Screens[pScreen->myNum])
#define GET_SCRINDEX(pScrn) do { }while(0)
#endif

#define V_MIN(a,b) ((a)>(b)?(b):(a))
#define V_MAX(a,b) ((a)>(b)?(a):(b))

//#define ALL_NONCACHE_BIGSURFACE 1
#define ADDRESS_ALIGNMENT 64
#define WIDTH_ALIGNMENT 16
#define HEIGHT_ALIGNMENT 16
//#define WIDTH_ALIGNMENT 8
//#define HEIGHT_ALIGNMENT 1

#define BITSTOBYTES(x) (((x)+7)/8)

#if defined(EXA_SW_WIDTH) && defined(EXA_SW_HEIGHT)

#define IMX_EXA_NONCACHESURF_WIDTH EXA_SW_WIDTH
#define IMX_EXA_NONCACHESURF_HEIGHT EXA_SW_HEIGHT

#else

#ifdef SMALL_THRESHOLD
#define IMX_EXA_NONCACHESURF_WIDTH 300
#define IMX_EXA_NONCACHESURF_HEIGHT 300
#else
#define IMX_EXA_NONCACHESURF_WIDTH 1024
#define IMX_EXA_NONCACHESURF_HEIGHT 1024
#endif

#endif

#define IMX_EXA_NONCACHESURF_SIZE ( IMX_EXA_NONCACHESURF_WIDTH * IMX_EXA_NONCACHESURF_HEIGHT )

#define SURF_SIZE_FOR_SW(sw,sh) do {    \
                        if ( gcmALIGN(sw, WIDTH_ALIGNMENT) < IMX_EXA_NONCACHESURF_WIDTH \
                        || gcmALIGN(sh, HEIGHT_ALIGNMENT) < IMX_EXA_NONCACHESURF_HEIGHT)    \
                        TRACE_EXIT(FALSE);  \
                } while ( 0 )

#define SURF_SIZE_FOR_SW_COND(sw,sh) (  \
                        ( gcmALIGN(sw, WIDTH_ALIGNMENT) < IMX_EXA_NONCACHESURF_WIDTH    \
                        || gcmALIGN(sh, HEIGHT_ALIGNMENT) < IMX_EXA_NONCACHESURF_HEIGHT )   \
                        )



#ifdef ADD_FSL_XRANDR
#define  IMX_ALIGN(offset, align)  gcmALIGN_NP2(offset, align)
#endif

#ifdef __cplusplus
}
#endif

#endif  /* VIVANTE_COMMON_H */

