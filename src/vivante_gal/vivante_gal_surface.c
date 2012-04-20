/****************************************************************************
*
*    Copyright (C) 2005 - 2012 by Vivante Corp.
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


#include "vivante_gal.h"
#include "vivante_priv.h"

#define WIDTH_ALIGNMENT  8
#define HEIGHT_ALIGNMENT 1
#define BITSTOBYTES(x) (((x)+7)/8)

/**
 *
 * @param Hal - Hardware abstraction layer object
 * @param Size - Size of the surface in bits
 * @param Pool - To allocate from which pool
 * @param Node - returned allocated video node
 * @return  - result of the process
 */
static gceSTATUS AllocVideoNode(
        IN gcoHAL Hal,
        IN OUT gctUINT_PTR Size,
        IN OUT gcePOOL *Pool,
        OUT gcuVIDMEM_NODE_PTR *Node) {
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmASSERT(Pool != gcvNULL);
    gcmASSERT(Size != gcvNULL);
    gcmASSERT(Node != gcvNULL);

    iface.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
    iface.u.AllocateLinearVideoMemory.bytes = *Size;
    iface.u.AllocateLinearVideoMemory.alignment = 64;
    iface.u.AllocateLinearVideoMemory.pool = *Pool;
    iface.u.AllocateLinearVideoMemory.type = gcvSURF_BITMAP;

    /* Call kernel API. */
    gcmONERROR(gcoHAL_Call(Hal, &iface));

    /* Get allocated node in video memory. */
    *Node = iface.u.AllocateLinearVideoMemory.node;
    *Pool = iface.u.AllocateLinearVideoMemory.pool;
    *Size = iface.u.AllocateLinearVideoMemory.bytes;

OnError:

    return status;
}

/**
 *
 * @param Hal - Hardware abstraction layer object
 * @param Node - video node
 * @return result of the process
 */
static gceSTATUS FreeVideoNode(
        IN gcoHAL Hal,
        IN gcuVIDMEM_NODE_PTR Node) {
    gcsHAL_INTERFACE iface;

    gcmASSERT(Node != gcvNULL);

    iface.command = gcvHAL_FREE_VIDEO_MEMORY;
    iface.u.FreeVideoMemory.node = Node;

    /* Call kernel API. */
    return gcoHAL_ScheduleEvent(Hal, &iface);
}

/**
 *  Use for  getting the physical and logical address
 * @param Hal Hardware abstraction layer object
 * @param Node video node
 * @param Address physical address
 * @param Memory logical address
 * @return result of the process
 */
static gceSTATUS LockVideoNode(
        IN gcoHAL Hal,
        IN gcuVIDMEM_NODE_PTR Node,
        OUT gctUINT32 *Address,
        OUT gctPOINTER *Memory) {
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmASSERT(Address != gcvNULL);
    gcmASSERT(Memory != gcvNULL);
    gcmASSERT(Node != gcvNULL);

    iface.command = gcvHAL_LOCK_VIDEO_MEMORY;
    iface.u.LockVideoMemory.node = Node;
    iface.u.LockVideoMemory.cacheable = gcvFALSE;
    /* Call kernel API. */
    gcmONERROR(gcoHAL_Call(Hal, &iface));

    /* Get allocated node in video memory. */
    *Address = iface.u.LockVideoMemory.address;
    *Memory = iface.u.LockVideoMemory.memory;

OnError:

    return status;
}

/**
 *
 * @param Hal
 * @param Node
 * @return
 */
static gceSTATUS UnlockVideoNode(
        IN gcoHAL Hal,
        IN gcuVIDMEM_NODE_PTR Node) {
    gcsHAL_INTERFACE iface;

    gcmASSERT(Node != gcvNULL);

    iface.command = gcvHAL_UNLOCK_VIDEO_MEMORY;
    iface.u.UnlockVideoMemory.node = Node;
    iface.u.UnlockVideoMemory.type = gcvSURF_BITMAP;
    iface.u.UnlockVideoMemory.asynchroneous = gcvTRUE;
    /* Call kernel API. */
    return gcoHAL_ScheduleEvent(Hal, &iface);

}

/************************************************************************
 * PIXMAP RELATED (START)
 ************************************************************************/
static gctBOOL FreeGPUSurface(VIVGPUPtr gpuctx, Viv2DPixmapPtr ppriv) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    GenericSurfacePtr surf = gcvNULL;
    surf = (GenericSurfacePtr) (ppriv->mVidMemInfo);
    if (surf->mIsWrapped) {
        goto delete_wrapper;
    }
    TRACE_INFO("DESTROYED SURFACE ADDRESS = %x - %x\n", surf, ppriv->mVidMemInfo);
    if (surf->mVideoNode.mNode != gcvNULL) {
        if (surf->mVideoNode.mLogicalAddr != gcvNULL) {
            status = UnlockVideoNode(gpuctx->mDriver->mHal, surf->mVideoNode.mNode);
            if (status != gcvSTATUS_OK) {
                TRACE_ERROR("Unable to UnLock video node\n");
                TRACE_EXIT(gcvFALSE);
            }
        }
        status = FreeVideoNode(gpuctx->mDriver->mHal, surf->mVideoNode.mNode);
        if (status != gcvSTATUS_OK) {
            TRACE_ERROR("Unable to Free video node\n");
            TRACE_EXIT(gcvFALSE);
        }
delete_wrapper:
        status = gcoOS_Free(gcvNULL, surf);
        if (status != gcvSTATUS_OK) {
            TRACE_ERROR("Unable to Free surface\n");
            TRACE_EXIT(gcvFALSE);
        }
        ppriv->mVidMemInfo = NULL;
    }
    TRACE_EXIT(gcvTRUE);
}

static gctBOOL VIV2DGPUSurfaceAlloc(VIVGPUPtr gpuctx, gctUINT alignedWidth, gctUINT alignedHeight,
        gctUINT bytesPerPixel, GenericSurfacePtr * surface) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    GenericSurfacePtr surf = gcvNULL;
    gctPOINTER mHandle = gcvNULL;
    status = gcoOS_Allocate(gcvNULL, sizeof (GenericSurface), &mHandle);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Unable to allocate generic surface\n");
        TRACE_EXIT(FALSE);
    }
    memset(mHandle, 0, sizeof (GenericSurface));
    surf = (GenericSurfacePtr) mHandle;

    surf->mVideoNode.mSizeInBytes = alignedWidth * bytesPerPixel * alignedHeight;
    surf->mVideoNode.mPool = gcvPOOL_DEFAULT;

    status = AllocVideoNode(gpuctx->mDriver->mHal, &surf->mVideoNode.mSizeInBytes, &surf->mVideoNode.mPool, &surf->mVideoNode.mNode);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Unable to allocate video node\n");
        TRACE_EXIT(FALSE);
    }

    status = LockVideoNode(gpuctx->mDriver->mHal, surf->mVideoNode.mNode, &surf->mVideoNode.mPhysicalAddr, &surf->mVideoNode.mLogicalAddr);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Unable to Lock video node\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_INFO("VIDEO NODE CREATED =>  LOGICAL = %d  PHYSICAL = %d  SIZE = %d\n", surf->mVideoNode.mLogicalAddr, surf->mVideoNode.mPhysicalAddr, surf->mVideoNode.mSizeInBytes);

    surf->mTiling = gcvLINEAR;
    surf->mAlignedWidth = alignedWidth;
    surf->mAlignedHeight = alignedHeight;
    surf->mStride = alignedWidth * bytesPerPixel;
    surf->mRotation = gcvSURF_0_DEGREE;
    surf->mLogicalAddr = surf->mVideoNode.mLogicalAddr;
    surf->mIsWrapped = gcvFALSE;
    *surface = surf;
    TRACE_EXIT(TRUE);
}

/*Creating and Destroying Functions*/
Bool CreateSurface(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr pPix) {
    GenericSurfacePtr surf = gcvNULL;
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    gctUINT alignedWidth, alignedHeight;
    gctUINT bytesPerPixel;
    alignedWidth = gcmALIGN(pPixmap->drawable.width, WIDTH_ALIGNMENT);
    alignedHeight = gcmALIGN(pPixmap->drawable.height, HEIGHT_ALIGNMENT);
    bytesPerPixel = BITSTOBYTES(pPixmap->drawable.bitsPerPixel);

    /*QUICK FIX*/
    if (bytesPerPixel < 2) {
        bytesPerPixel = 2;
    }

    if (!VIV2DGPUSurfaceAlloc(gpuctx, alignedWidth, alignedHeight, bytesPerPixel, &surf)) {
        TRACE_ERROR("Surface Creation Error\n");
        TRACE_EXIT(FALSE);
    }
    pPix->mVidMemInfo = surf;
    TRACE_EXIT(TRUE);
}

Bool WrapSurface(PixmapPtr pPixmap, void * logical, unsigned int physical, Viv2DPixmapPtr pPix) {
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT alignedWidth, alignedHeight;
    gctUINT bytesPerPixel;
    GenericSurfacePtr surf = gcvNULL;
    gctPOINTER mHandle = gcvNULL;
    status = gcoOS_Allocate(gcvNULL, sizeof (GenericSurface), &mHandle);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Unable to allocate generic surface\n");
        TRACE_EXIT(FALSE);
    }
    memset(mHandle, 0, sizeof (GenericSurface));
    surf = (GenericSurfacePtr) mHandle;

    alignedWidth = gcmALIGN(pPixmap->drawable.width, WIDTH_ALIGNMENT);
    alignedHeight = gcmALIGN(pPixmap->drawable.height, HEIGHT_ALIGNMENT);
    bytesPerPixel = BITSTOBYTES(pPixmap->drawable.bitsPerPixel);

    surf->mVideoNode.mSizeInBytes = alignedWidth * bytesPerPixel * alignedHeight;
    surf->mVideoNode.mPool = gcvPOOL_USER;

    surf->mVideoNode.mPhysicalAddr = physical;
    surf->mVideoNode.mLogicalAddr = (gctPOINTER) logical;

    surf->mTiling = gcvLINEAR;
    surf->mAlignedWidth = alignedWidth;
    surf->mAlignedHeight = alignedHeight;
    surf->mStride = alignedWidth * bytesPerPixel;
    surf->mRotation = gcvSURF_0_DEGREE;
    surf->mLogicalAddr = surf->mVideoNode.mLogicalAddr;
    surf->mIsWrapped = gcvTRUE;

    pPix->mVidMemInfo = surf;
    TRACE_EXIT(TRUE);
}

Bool DestroySurface(GALINFOPTR galInfo, Viv2DPixmapPtr ppix) {
    TRACE_ENTER();
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    if (ppix->mVidMemInfo == NULL) {
        TRACE_INFO("NOT GPU GENERATED SURFACE\n");
        TRACE_EXIT(TRUE);
    }
    if (!FreeGPUSurface(gpuctx, ppix)) {
        TRACE_ERROR("Unable to free gpu surface\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

/*Mapping Functions*/
void * MapSurface(Viv2DPixmapPtr priv) {
    TRACE_ENTER();
    void * returnaddr = NULL;
    GenericSurfacePtr surf;
    surf = (GenericSurfacePtr) priv->mVidMemInfo;
    returnaddr = surf->mLogicalAddr;
    TRACE_EXIT(returnaddr);
}

void UnMapSurface(Viv2DPixmapPtr priv) {
    TRACE_ENTER();
    TRACE_EXIT();
}

unsigned int GetStride(Viv2DPixmapPtr pixmap) {
    TRACE_ENTER();
    GenericSurfacePtr surf = (GenericSurfacePtr) pixmap->mVidMemInfo;
    TRACE_EXIT(surf->mStride);
}

/************************************************************************
 * PIXMAP RELATED (END)
 ************************************************************************/
