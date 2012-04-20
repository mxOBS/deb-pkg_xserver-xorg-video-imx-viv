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
 * File:   vivante_common.h
 * Author: vivante
 *
 * Created on February 18, 2012, 7:15 PM
 */

#ifndef VIVANTE_COMMON_H
#define	VIVANTE_COMMON_H

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
#include "mibstore.h"
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
#ifdef __cplusplus
}
#endif

#endif	/* VIVANTE_COMMON_H */

