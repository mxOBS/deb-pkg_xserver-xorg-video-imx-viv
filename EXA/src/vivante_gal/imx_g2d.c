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


#include "vivante_common.h"
#include "vivante_gal.h"
#include "vivante_priv.h"

static gctBOOL SetupDriver(OUT Viv2DDriverPtr * driver) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    Viv2DDriverPtr pDrvHandle = gcvNULL;
    gctPOINTER mHandle = gcvNULL;


    gctUINT32 smask = 0x0;

    /*Allocating the driver*/
    gcmASSERT(*driver == gcvNULL);
    status = gcoOS_Allocate(gcvNULL, sizeof (Viv2DDriver), &mHandle);
    if (status < 0) {
        TRACE_ERROR("Unable to allocate driver, status = %d\n", status);
        TRACE_EXIT(gcvFALSE);
    }
    pDrvHandle = (Viv2DDriverPtr) mHandle;
    /*TODO*/
    pDrvHandle->mG2DBaseAddr=0;

    status = g2d_open(&(pDrvHandle->mG2DHandle));
    if (status < 0) {
        TRACE_ERROR("g2d_open failed, status = %d\n", status);
        goto FREESOURCE;
    }

    *driver = pDrvHandle;
    TRACE_EXIT(gcvTRUE);

FREESOURCE:
    if(mHandle)
        gcoOS_Free(gcvNULL, mHandle);

    TRACE_EXIT(gcvFALSE);
}

/**
 *
 * @param driver - driver object to be destroyed
 * @return  - status of the destriuction
 */
static gctBOOL DestroyDriver(IN Viv2DDriverPtr driver) {
    gceSTATUS status = gcvSTATUS_OK;
    gcmASSERT(driver != gcvNULL);
    TRACE_ENTER();

    if(driver->mG2DHandle) {
        status = g2d_close(driver->mG2DHandle);
        if (status < 0) {
            TRACE_ERROR("g2d_close failed, status = %d\n", status);
            TRACE_EXIT(gcvFALSE);
        }
    }
    status = gcoOS_Free(gcvNULL, driver);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Unable to free driver structure, status = %d\n", status);
        TRACE_EXIT(gcvFALSE);
    }
    driver = gcvNULL;
    TRACE_EXIT(gcvTRUE);
}



/************************************************************************
 * GPU RELATED (START)
 ************************************************************************/

Bool SetupG2D(GALINFOPTR galInfo) {
    TRACE_ENTER();
    static gctBOOL inited = gcvFALSE;
    gctBOOL ret = gcvFALSE;
    gctPOINTER mHandle = gcvNULL;
    VIVGPUPtr gpuctx = NULL;
    gceSTATUS status = gcvSTATUS_OK;
    if (inited) {
        TRACE_EXIT(TRUE);
    }
    if (galInfo->mGpu != NULL) {
        TRACE_ERROR("UNDEFINED GPU CTX\n");
        TRACE_EXIT(FALSE);
    }
    status = gcoOS_Allocate(gcvNULL, sizeof (VIVGPU), &mHandle);
    if (status < 0) {
        TRACE_ERROR("Unable to allocate driver, status = %d\n", status);
        TRACE_EXIT(FALSE);
    }
    gpuctx = (VIVGPUPtr) (mHandle);
    ret = SetupDriver(&gpuctx->mDriver);
    if (ret != gcvTRUE) {
        gcoOS_Free(gcvNULL, mHandle);
        TRACE_ERROR("GPU DRIVER  FAILED\n");
        TRACE_EXIT(FALSE);
    }

    inited = gcvTRUE;
    galInfo->mPreferredAllocator = IMXG2D;
    galInfo->mGpu = gpuctx;
    TRACE_EXIT(TRUE);
}

char *MapG2DPixmap(Viv2DPixmapPtr pdst ){

    GenericSurfacePtr surf = (GenericSurfacePtr) pdst->mVidMemInfo;

    return (surf ? surf->mVideoNode.mLogicalAddr:NULL);
}

Bool G2DGPUFlushGraphicsPipe(GALINFOPTR galInfo) {
    TRACE_ENTER();

    TRACE_EXIT(TRUE);
}

extern Bool vivEnableCacheMemory;
Bool G2DCacheOperation(GALINFOPTR galInfo, Viv2DPixmapPtr ppix, VIVFLUSHTYPE flush_type) {
    int status = 0;
    GenericSurfacePtr surf = (GenericSurfacePtr) (ppix->mVidMemInfo);
    VIVGPUPtr gpuctx = (VIVGPUPtr) (galInfo->mGpu);

#if defined(__mips__) || defined(mips)
    TRACE_EXIT(TRUE);
#endif

    if ( surf == NULL )
        TRACE_EXIT(TRUE);

    if ( surf->mIsWrapped ) {
        TRACE_EXIT(TRUE);
    }

    if (surf->g2dbuf == NULL) {
        TRACE_EXIT(TRUE);
    }

    if (vivEnableCacheMemory == FALSE){
        TRACE_EXIT(TRUE);
    }

    if(galInfo->mPreferredAllocator == VIVGAL2D) {
        return VIV2DCacheOperation(galInfo, ppix, flush_type);
    }

    TRACE_INFO("FLUSH INFO => LOGICAL = %d PHYSICAL = %d STRIDE = %d  ALIGNED HEIGHT = %d\n", surf->mVideoNode.mLogicalAddr, surf->mVideoNode.mPhysicalAddr, surf->mStride, surf->mAlignedHeight);

    switch (flush_type) {
        case INVALIDATE:
            status = g2d_cache_op(surf->g2dbuf, G2D_CACHE_INVALIDATE);
            if (status < 0) {
                TRACE_ERROR("Cache Invalidation Failed\n");
                TRACE_EXIT(FALSE);
            }
            break;
        case FLUSH:
            status = g2d_cache_op(surf->g2dbuf, G2D_CACHE_FLUSH);
            if (status < 0) {
                TRACE_ERROR("Cache Invalidation Failed\n");
                TRACE_EXIT(FALSE);
            }
            break;
        case CLEAN:
           status = g2d_cache_op(surf->g2dbuf, G2D_CACHE_CLEAN);
            if (status < 0) {
                TRACE_ERROR("Cache Invalidation Failed\n");
                TRACE_EXIT(FALSE);
            }
            break;
        default:
            TRACE_ERROR("UNIDENTIFIED Cache Operation\n");
            TRACE_EXIT(FALSE);
            break;
    }
    TRACE_EXIT(TRUE);
}

Bool G2DReUseSurface(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr toBeUpdatedpPix)
{
    GenericSurfacePtr surf = gcvNULL;
    gctUINT alignedWidth, alignedHeight;
    gctUINT bytesPerPixel;
    alignedWidth = gcmALIGN(pPixmap->drawable.width, WIDTH_ALIGNMENT);
    alignedHeight = gcmALIGN(pPixmap->drawable.height, HEIGHT_ALIGNMENT);
    bytesPerPixel = BITSTOBYTES(pPixmap->drawable.bitsPerPixel);

    /* The same as CreatSurface */
    if (bytesPerPixel < 2) {
        bytesPerPixel = 2;
    }

    surf = (GenericSurfacePtr)toBeUpdatedpPix->mVidMemInfo;

    if ( surf && surf->mVideoNode.mSizeInBytes >= (alignedWidth * alignedHeight * bytesPerPixel))
    {
        surf->mTiling = gcvLINEAR;
        surf->mAlignedWidth = alignedWidth;
        surf->mAlignedHeight = alignedHeight;
        surf->mStride = alignedWidth * bytesPerPixel;
        surf->mRotation = gcvSURF_0_DEGREE;
        surf->mLogicalAddr = surf->mVideoNode.mLogicalAddr;
        surf->mIsWrapped = gcvFALSE;

        if ( surf->mData )
            pixman_image_unref( (pixman_image_t *)surf->mData );

        surf->mData = gcvNULL;
        TRACE_EXIT(TRUE);
    }
    TRACE_EXIT(FALSE);
}

Bool G2DWrapSurface(PixmapPtr pPixmap, void * logical, unsigned int physical, Viv2DPixmapPtr pPix, int bytes) {
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


    bytesPerPixel = BITSTOBYTES(pPixmap->drawable.bitsPerPixel);
    alignedWidth = gcmALIGN(pPixmap->devKind/bytesPerPixel, WIDTH_ALIGNMENT);
    alignedHeight = gcmALIGN(pPixmap->drawable.height, HEIGHT_ALIGNMENT);


    surf->mVideoNode.mSizeInBytes = bytes;
    surf->mVideoNode.mPool = gcvPOOL_USER;

    surf->mVideoNode.mPhysicalAddr = physical;
    surf->mVideoNode.mLogicalAddr = (gctPOINTER) logical;

    surf->mBytesPerPixel = bytesPerPixel;
    surf->mTiling = gcvLINEAR;
    surf->mAlignedWidth = alignedWidth;
    surf->mAlignedHeight = alignedHeight;
    surf->mStride = pPixmap->devKind;;
    surf->mRotation = gcvSURF_0_DEGREE;
    surf->mLogicalAddr = surf->mVideoNode.mLogicalAddr;
    surf->mIsWrapped = gcvTRUE;

    pPix->mVidMemInfo = surf;
    TRACE_EXIT(TRUE);
}


static gctBOOL G2DGPUSurfaceAlloc(VIVGPUPtr gpuctx, gctUINT alignedWidth, gctUINT alignedHeight,
    gctUINT bytesPerPixel, GenericSurfacePtr * surface, int fd) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    GenericSurfacePtr surf = gcvNULL;
    gctPOINTER mHandle = gcvNULL;
    gceSURF_TYPE surftype;
    Bool cacheable;
    /*TODO*/
    /*surf = GrabSurfFromPool(alignedWidth, alignedHeight, bytesPerPixel);*/

    if ( surf == NULL )
    {
        status = gcoOS_Allocate(gcvNULL, sizeof(GenericSurface), &mHandle);
        if (status != gcvSTATUS_OK) {
            TRACE_ERROR("Unable to allocate generic surface\n");
            TRACE_EXIT(FALSE);
        }

        memset(mHandle, 0, sizeof (GenericSurface));
        surf = (GenericSurfacePtr) mHandle;

        surf->mVideoNode.mSizeInBytes = alignedWidth * bytesPerPixel * alignedHeight;
        surf->mVideoNode.mPool = gcvPOOL_DEFAULT;

        if(fd > 0 ) {
            surf->g2dbuf = g2d_buf_from_fd(fd);
        }
        else {
            surf->g2dbuf = g2d_alloc(surf->mVideoNode.mSizeInBytes, 0);
        }

        if (!surf->g2dbuf) {
            TRACE_ERROR("Unable to allocate generic surface\n");

            TRACE_EXIT(FALSE);
        }
        surf->mVideoNode.mLogicalAddr = surf->g2dbuf->buf_vaddr;
        surf->mVideoNode.mPhysicalAddr = surf->g2dbuf->buf_paddr;
        surf->mVideoNode.mSizeInBytes = surf->g2dbuf->buf_size;

        TRACE_INFO("VIDEO NODE CREATED =>  LOGICAL = %d  PHYSICAL = %d  SIZE = %d\n", surf->mVideoNode.mLogicalAddr, surf->mVideoNode.mPhysicalAddr, surf->mVideoNode.mSizeInBytes);
    }

    surf->mTiling = gcvLINEAR;
    surf->mAlignedWidth = alignedWidth;
    surf->mAlignedHeight = alignedHeight;
    surf->mBytesPerPixel = bytesPerPixel;
    surf->mStride = alignedWidth * bytesPerPixel;
    surf->mRotation = gcvSURF_0_DEGREE;
    surf->mLogicalAddr = surf->mVideoNode.mLogicalAddr;
    surf->mIsWrapped = gcvFALSE;
    surf->mData = gcvNULL;
    *surface = surf;

    TRACE_EXIT(TRUE);
}

/*Creating and Destroying Functions*/
Bool G2DCreateSurface(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr pPix, int fd) {
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

    if (!G2DGPUSurfaceAlloc(gpuctx, alignedWidth, alignedHeight, bytesPerPixel, &surf, fd)) {

        TRACE_ERROR("Surface Creation Error\n");
        TRACE_EXIT(FALSE);
    }

    pPix->mVidMemInfo = surf;
    TRACE_EXIT(TRUE);
}

Bool G2DCleanSurfaceBySW(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr pPix)
{
    GenericSurfacePtr surf = NULL;
    VIVGPUPtr gpuctx = (VIVGPUPtr) (galInfo->mGpu);
    gceSTATUS status = gcvSTATUS_OK;
    VivPictFormat mFormat;
    gcsRECT dstRect = {0, 0, pPixmap->drawable.width, pPixmap->drawable.height};

    if ( pPix == NULL )
        TRACE_EXIT(FALSE);
    surf = (GenericSurfacePtr)pPix->mVidMemInfo;

    mFormat = mFormat;
    dstRect = dstRect;

    pPix->mCpuBusy = TRUE;
    memset((char *)surf->mVideoNode.mLogicalAddr,0,surf->mVideoNode.mSizeInBytes);

    TRACE_EXIT(TRUE);

}

static gctBOOL FreeGPUSurface(VIVGPUPtr gpuctx, Viv2DPixmapPtr ppriv) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    GenericSurfacePtr surf = gcvNULL;
    gceSURF_TYPE surftype;
/*TBD*/
    surf = (GenericSurfacePtr) (ppriv->mVidMemInfo);
    if(surf->g2dbuf ) {
        g2d_free(surf->g2dbuf);
    }
    if (surf->mIsWrapped) {
        goto delete_wrapper;
    }
    TRACE_INFO("DESTROYED SURFACE ADDRESS = %x - %x\n", surf, ppriv->mVidMemInfo);

    if ( surf->mData )
        pixman_image_unref( (pixman_image_t *)surf->mData );

delete_wrapper:
    TRACE_EXIT(gcvTRUE);
}


Bool G2DDestroySurface(GALINFOPTR galInfo, Viv2DPixmapPtr ppix) {
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
void * G2DMapSurface(Viv2DPixmapPtr priv) {
    TRACE_ENTER();
    void * returnaddr = NULL;
    GenericSurfacePtr surf;
    surf = (GenericSurfacePtr) priv->mVidMemInfo;

    if ( surf == NULL ) {

        TRACE_EXIT(0);
    }
    returnaddr = surf->mLogicalAddr;
    TRACE_EXIT(returnaddr);
}

void G2DUnMapSurface(Viv2DPixmapPtr priv) {
    TRACE_ENTER();
    TRACE_EXIT();
}

unsigned int G2DGetStride(Viv2DPixmapPtr pixmap) {
    TRACE_ENTER();
    GenericSurfacePtr surf = (GenericSurfacePtr) pixmap->mVidMemInfo;
    TRACE_EXIT(surf->mStride);
}
