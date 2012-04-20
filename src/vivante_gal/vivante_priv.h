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
 * File:   vivante_priv.h
 * Author: vivante
 *
 * Created on February 23, 2012, 12:33 PM
 */

#ifndef VIVANTE_PRIV_H
#define	VIVANTE_PRIV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAL/gc_hal.h"
#include "HAL/gc_hal_raster.h"
#include "HAL/gc_hal_base.h"

    /************************************************************************
     * PIXMAP_HANDLING_STUFF(START)
     ************************************************************************/
    typedef struct {
        gcuVIDMEM_NODE_PTR mNode;
        gcePOOL mPool;
        gctUINT mSizeInBytes;
        gctUINT32 mPhysicalAddr;
        gctPOINTER mLogicalAddr;
    } VideoNode, *VideoNodePtr;

    typedef struct {
        gctBOOL mIsWrapped;
        gceSURF_ROTATION mRotation;
        gceTILING mTiling;
        gctUINT32 mAlignedWidth;
        gctUINT32 mAlignedHeight;
        gctPOINTER mLogicalAddr;
        gctUINT32 mStride;
        VideoNode mVideoNode;
    } GenericSurface, *GenericSurfacePtr;

    /************************************************************************
     * PIXMAP_HANDLING_STUFF (END)
     ************************************************************************/

    /**************************************************************************
     * DRIVER & DEVICE  Structs (START)
     *************************************************************************/
    typedef struct _viv2DDriver {
        /*Base Objects*/
        gcoOS mOs;
        gcoHAL mHal;
        gco2D m2DEngine;
        gcoBRUSH mBrush;

        /*video memory mapping*/
        gctPHYS_ADDR g_InternalPhysical, g_ExternalPhysical, g_ContiguousPhysical;
        gctSIZE_T g_InternalSize, g_ExternalSize, g_ContiguousSize;
        gctPOINTER g_Internal, g_External, g_Contiguous;

        /* HW specific features. */
        gctBOOL mIsSeperated;
        gctBOOL mIsPe20Supported;
        gctBOOL mIsMultiSrcBltSupported;
        gctBOOL mIsMultiSrcBltExSupported;
        gctUINT mMaxSourceForMultiSrcOpt;
    } Viv2DDriver, *Viv2DDriverPtr;

    typedef struct _viv2DDevice {
        gceCHIPMODEL mChipModel; /*chip model */
        unsigned int mChipRevision; /* chip revision */
        unsigned int mChipFeatures; /* chip features */
        unsigned int mChipMinorFeatures; /* chip minor features */
    } Viv2DDevice, *Viv2DDevicePtr;

    typedef struct _vivanteGpu {
        Viv2DDriverPtr mDriver;
        Viv2DDevicePtr mDevice;
    } VIVGPU, *VIVGPUPtr;

    /**************************************************************************
     * DRIVER & DEVICE  Structs (END)
     *************************************************************************/

#ifdef __cplusplus
}
#endif

#endif	/* VIVANTE_PRIV_H */

