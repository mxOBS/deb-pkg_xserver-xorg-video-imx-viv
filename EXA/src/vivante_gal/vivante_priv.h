/****************************************************************************
*
*    Copyright 2012 - 2019 Vivante Corporation, Santa Clara, California.
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


#ifndef VIVANTE_PRIV_H
#define VIVANTE_PRIV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gc_hal.h"
#include "gc_hal_raster.h"
#include "gc_hal_base.h"

#ifdef ENABLE_VIVANTE_DRI3
#include "vivante_bo.h"
#endif

    /************************************************************************
     * PIXMAP_HANDLING_STUFF(START)
     ************************************************************************/
    typedef struct {
        gctUINT64 mNode;
        gcePOOL mPool;
        gctUINT mBytes;
        gctUINT32 mPhysicalAddr;
        gctPOINTER mLogicalAddr;
    } VideoNode, *VideoNodePtr;

    typedef struct {
        gctBOOL mIsWrapped;
        gceSURF_ROTATION mRotation;
        gceTILING mTiling;
        gctUINT32 mAlignedWidth;
        gctUINT32 mAlignedHeight;
        gctUINT32 mBytesPerPixel;
        gctPOINTER mLogicalAddr;
        gctUINT32 mStride;
        VideoNode mVideoNode;
        gctPOINTER mData;
#ifdef ENABLE_VIVANTE_DRI3
        struct drm_vivante_bo *bo;
        int fd;
#endif
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
#ifdef HAVE_G2D
        void* mG2DHandle;
        gctUINT32 mG2DBaseAddr;
#endif
        gcoBRUSH mBrush;
#ifdef ENABLE_VIVANTE_DRI3
        struct drm_vivante *drm;
#endif
        /*video memory mapping*/
        gctUINT32 g_InternalPhysName, g_ExternalPhysName, g_ContiguousPhysName;
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
    } Viv2DDevice, *Viv2DDevicePtr;

    typedef struct _vivanteGpu {
        Viv2DDriverPtr mDriver;
        Viv2DDevicePtr mDevice;
    } VIVGPU, *VIVGPUPtr;

gceSTATUS AllocVideoNode(
    IN gcoHAL Hal,
    IN gctBOOL cacheable,
    IN gceSURF_TYPE surftype,
    IN OUT GenericSurfacePtr Surf
    );

gceSTATUS FreeVideoNode(
        IN gcoHAL Hal,
        IN gctUINT32 Node);

gceSTATUS LockVideoNode(
        IN gcoHAL Hal,
        IN gctUINT32 Node,
        IN gctBOOL cacheable,
        OUT gctUINT32 *Address,
        OUT gctPOINTER *Memory);

gceSTATUS UnlockVideoNode(
    IN gcoHAL Hal,
    IN gctUINT32 Node,
    IN gceSURF_TYPE surftype);
    /**************************************************************************
     * DRIVER & DEVICE  Structs (END)
     *************************************************************************/

#ifdef __cplusplus
}
#endif

#endif  /* VIVANTE_PRIV_H */

