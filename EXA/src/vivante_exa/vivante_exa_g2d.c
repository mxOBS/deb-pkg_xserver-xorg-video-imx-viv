/****************************************************************************
*
*    Copyright 2012 - 2018 Vivante Corporation, Santa Clara, California.
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


/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "vivante_exa.h"
#include "vivante.h"
#include "vivante_priv.h"

#ifdef HAVE_G2D

#define MAXSIZE_FORSWTOSCREEN  160000
#define MAX_COMPOSITE_SUB_SIZE IMX_EXA_MIN_PIXEL_AREA_COMPOSITE

static const G2dBlendOp
g2d_blendingOps[] = {
    {
        PictOpClear, 0, 0, G2D_ZERO, G2D_ZERO
    },
    {
        PictOpSrc, 0, 0, G2D_ONE, G2D_ZERO
    },
    {
        PictOpDst, 0, 0, G2D_ZERO, G2D_ONE
    },
    {
        PictOpOver, 1, 0, G2D_ONE, G2D_ONE_MINUS_SRC_ALPHA
    },
    {
        PictOpOverReverse, 0, 1, G2D_ONE_MINUS_DST_ALPHA, G2D_ONE
    },
    {
        PictOpIn, 0, 1, G2D_DST_ALPHA, G2D_ZERO
    },
    {
        PictOpInReverse, 1, 0, G2D_ZERO, G2D_SRC_ALPHA
    },
    {
        PictOpOut, 0, 1, G2D_ONE_MINUS_DST_ALPHA, G2D_ZERO
    },
    {
        PictOpOutReverse, 1, 0, G2D_ZERO, G2D_ONE_MINUS_SRC_ALPHA
    },
    {
        PictOpAtop, 1, 1, G2D_DST_ALPHA, G2D_ONE_MINUS_SRC_ALPHA
    },
    {
        PictOpAtopReverse, 1, 1, G2D_ONE_MINUS_DST_ALPHA, G2D_SRC_ALPHA
    },
    {
        PictOpXor, 1, 1, G2D_ONE_MINUS_DST_ALPHA, G2D_ONE_MINUS_SRC_ALPHA
    },
    {
        PictOpAdd, 0, 0, G2D_ONE, G2D_ONE
    }
};

static const G2dPictFormat
g2d_pict_formats[] = {
    {
        PICT_a8r8g8b8, 32, G2D_BGRA8888, 8
    },
    {
        PICT_x8r8g8b8, 32, G2D_BGRX8888, 0
    },
    {
        PICT_a8b8g8r8, 32, G2D_RGBA8888, 8
    },
    {
        PICT_x8b8g8r8, 32, G2D_RGBX8888, 0
    },
    {
        PICT_b8g8r8a8, 32, G2D_ARGB8888, 8
    },
    {
        PICT_b8g8r8x8, 32, G2D_XRGB8888, 0
    },
    {
        PICT_r5g6b5, 16, G2D_RGB565, 0
    }
    /*END*/
};

static PixmapPtr
G2dGetDrawablePixmap(DrawablePtr pDrawable) {
    /* Make sure there is a drawable. */
    if (NULL == pDrawable) {
        return NULL;
    }

    /* Check for a backing pixmap. */
    if (DRAWABLE_WINDOW == pDrawable->type) {

        WindowPtr pWindow = (WindowPtr) pDrawable;
        return pDrawable->pScreen->GetWindowPixmap(pWindow);
    }

    /* Otherwise, it's a regular pixmap. */
    return (PixmapPtr) pDrawable;
}

static Bool GetDefaultG2dFormat(int bpp, G2dPictFmtPtr g2dPictFmt) {

G2dPictFormat r5g5b5 = {
    PICT_r5g6b5, 16, G2D_RGB565, 0
};

G2dPictFormat a8r8g8b8 = {
    PICT_a8r8g8b8, 32, G2D_BGRA8888, 8
};
    switch (bpp) {
        case 16:
            *g2dPictFmt = r5g5b5;
            break;
        case 32:
            *g2dPictFmt = a8r8g8b8;
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

static Bool GetG2dBlendingFactors(int op, G2dBlendOpPtr g2dBlendOp)
{
    int i = 0;
    Bool isFound = FALSE;

    for (i = 0; i < ARRAY_SIZE(g2d_blendingOps) && !isFound; i++) {
        if (g2d_blendingOps[i].mOp == op) {

            *g2dBlendOp = g2d_blendingOps[i];
            isFound = TRUE;
        }
    }
    return isFound;
}

static Bool GetG2dPictureFormat(int exa_fmt, G2dPictFmtPtr g2dPictFmt) {
    int i= 0;
    Bool isFound = FALSE;
    int size = ARRAY_SIZE(g2d_pict_formats);

    for (i = 0; i < size && !isFound; i++) {
        if (exa_fmt == g2d_pict_formats[i].mExaFmt) {
            *g2dPictFmt = (g2d_pict_formats[i]);
            isFound = TRUE;
        }
    }
    return isFound;
}

static Bool G2dTransformSupported(PictTransform *ptransform, enum g2d_rotation *rot)
{
    Bool isSupported = FALSE;

    *rot = G2D_ROTATION_0;
    if (NULL==ptransform)
    {
        isSupported = TRUE;
        return isSupported;
    }

    //rotate 0
    if ((ptransform->matrix[0][0]==pixman_fixed_1)
        &&(ptransform->matrix[0][1]==0)
        &&(ptransform->matrix[1][0]==0)
        &&(ptransform->matrix[1][1]==pixman_fixed_1))
    {
        *rot = G2D_ROTATION_0;
        isSupported = TRUE;
    }

    //rotate 90 clockwise
    if ((ptransform->matrix[0][0]==0)
        &&(ptransform->matrix[0][1]==pixman_fixed_1)
        &&(ptransform->matrix[1][0]==-pixman_fixed_1)
        &&(ptransform->matrix[1][1]==0))
    {
        *rot = G2D_ROTATION_270;
        isSupported = TRUE;
    }

    //rotate 270 clockwise
    if ((ptransform->matrix[0][0]==0)
        &&(ptransform->matrix[0][1]==-pixman_fixed_1)
        &&(ptransform->matrix[1][0]==pixman_fixed_1)
        &&(ptransform->matrix[1][1]==0))
    {
        *rot = G2D_ROTATION_90;
        isSupported = TRUE;
    }

    //rotate 180
    if ((ptransform->matrix[0][0]==-pixman_fixed_1)
        &&(ptransform->matrix[0][1]==0)
        &&(ptransform->matrix[1][0]==0)
        &&(ptransform->matrix[1][1]==-pixman_fixed_1))
    {
        *rot = G2D_ROTATION_180;
        isSupported = TRUE;
    }

    //reflect X
    if ((ptransform->matrix[0][0]==-pixman_fixed_1)
        &&(ptransform->matrix[0][1]==0)
        &&(ptransform->matrix[1][0]==0)
        &&(ptransform->matrix[1][1]==pixman_fixed_1))
    {
        *rot = G2D_FLIP_H;
        isSupported = TRUE;
    }

    //reflect Y
    if ((ptransform->matrix[0][0]==pixman_fixed_1)
        &&(ptransform->matrix[0][1]==0)
        &&(ptransform->matrix[1][0]==0)
        &&(ptransform->matrix[1][1]==-pixman_fixed_1))
    {
        *rot = G2D_FLIP_V;
        isSupported = TRUE;
    }

    return isSupported;
}


static void printG2dSurfaceInfo(struct g2d_surfaceEx* g2dSurface, const char* msg)
{
    xf86DrvMsg(0, X_ERROR,
        "%s physicAddr = %x left = %d right = %d top=%d bottom=%d stride= %d tiling = %d, format=%d, width=%d, height=%d \n",
        msg,
        g2dSurface->base.planes[0],
        g2dSurface->base.left,
        g2dSurface->base.right,
        g2dSurface->base.top,
        g2dSurface->base.bottom,
        g2dSurface->base.stride,
        g2dSurface->tiling,
        g2dSurface->base.format,
        g2dSurface->base.width,
        g2dSurface->base.height);
}

static void g2d_blitSurface(void *handle, struct g2d_surfaceEx * srcG2dSurface,
                        struct g2d_surfaceEx *dstG2dSurface, const char* msg)
{
    if(g2d_blitEx(handle, srcG2dSurface, dstG2dSurface))
    {
        xf86DrvMsg(0, X_ERROR, "From API %s\n", msg);
        printG2dSurfaceInfo(srcG2dSurface, "ERR SRC:");
        printG2dSurfaceInfo(dstG2dSurface, "ERR DST:");
    }
}

static void CalG2dSurfParam(struct g2d_surface *pg2d_surf,G2DBLITINFOPTR pBlt,
                        int left, int top,int right, int bottom,enum g2d_rotation rot)
{
    Viv2DPixmapPtr pVivPixSrc = pBlt->mSrcSurfInfo.mPriv;
    pg2d_surf->stride = ((GenericSurfacePtr )(pVivPixSrc->mVidMemInfo))->mAlignedWidth;
    pg2d_surf->width = ((GenericSurfacePtr )(pVivPixSrc->mVidMemInfo))->mAlignedWidth;
    pg2d_surf->height = ((GenericSurfacePtr )(pVivPixSrc->mVidMemInfo))->mAlignedHeight;
    pg2d_surf->rot = rot;
    switch(rot)
    {
        case G2D_ROTATION_0:
        default:
            pg2d_surf->left = left;
            pg2d_surf->top = top;
            pg2d_surf->right = right;
            pg2d_surf->bottom = bottom;
            break;
        case G2D_ROTATION_90:
            pg2d_surf->left = pg2d_surf->width - bottom;
            pg2d_surf->top = left;
            pg2d_surf->right = pg2d_surf->width - top;
            pg2d_surf->bottom = right;
            break;
        case G2D_ROTATION_270:
            pg2d_surf->left = top;
            pg2d_surf->top = pg2d_surf->height - right;
            pg2d_surf->right = bottom;
            pg2d_surf->bottom = pg2d_surf->height - left;
            break;
        case G2D_ROTATION_180:
            pg2d_surf->left = pg2d_surf->width - right;
            pg2d_surf->top = pg2d_surf->height - bottom;
            pg2d_surf->right = pg2d_surf->width - left;
            pg2d_surf->bottom = pg2d_surf->height - top;
            break;
       case G2D_FLIP_H:
            pg2d_surf->left = pg2d_surf->width - right;
            pg2d_surf->top = top;
            pg2d_surf->right = pg2d_surf->width - left;
            pg2d_surf->bottom = bottom;
            break;
       case G2D_FLIP_V:
            pg2d_surf->left = left;
            pg2d_surf->top = pg2d_surf->height - bottom;
            pg2d_surf->right = right;
            pg2d_surf->bottom = pg2d_surf->height - top;
            break;

    }
}

static void G2dSWComposite(PixmapPtr pxDst, int srcX, int srcY, int maskX, int maskY,
    int dstX, int dstY, int width, int height) {
    pixman_image_t *srcimage;
    pixman_image_t *dstimage;
    Viv2DPixmapPtr pVivPixSrc;
    Viv2DPixmapPtr pVivPixDst;
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pxDst);
    G2DBLITINFOPTR pBlt = &pViv->mGrCtx.mG2dBlitInfo;
    GenericSurfacePtr srcsurf = (GenericSurfacePtr) (pBlt->mSrcSurfInfo.mPriv->mVidMemInfo);
    GenericSurfacePtr dstsurf = (GenericSurfacePtr) (pBlt->mDstSurfInfo.mPriv->mVidMemInfo);

    pVivPixSrc = (Viv2DPixmapPtr)pBlt->mSrcSurfInfo.mPriv;
    pVivPixDst = (Viv2DPixmapPtr)pBlt->mDstSurfInfo.mPriv;

    if ( srcsurf->mData == NULL ) {
        srcimage = pixman_image_create_bits((pixman_format_code_t) pBlt->mSrcSurfInfo.mFormat.mExaFmt,
        pBlt->mSrcSurfInfo.mWidth,
        pBlt->mSrcSurfInfo.mHeight, (uint32_t *) srcsurf->mLogicalAddr,
        srcsurf->mStride);
        srcsurf->mData = (gctPOINTER)srcimage;
        //if ( pBlt->mSrcSurfInfo.mFormat.mBpp != 32 )
        //pixman_image_set_accessors(srcimage, real_reader, real_writer);
    } else {
        srcimage = (pixman_image_t *)srcsurf->mData;
    }

    if ( pBlt->mTransform )
        pixman_image_set_transform( srcimage, pBlt->mTransform);

    if ( pBlt->mSrcSurfInfo.repeat )
        pixman_image_set_repeat( srcimage, pBlt->mSrcSurfInfo.repeatType);

    if ( dstsurf->mData == NULL ) {
        dstimage = pixman_image_create_bits((pixman_format_code_t) pBlt->mDstSurfInfo.mFormat.mExaFmt,
        pBlt->mDstSurfInfo.mWidth,
        pBlt->mDstSurfInfo.mHeight, (uint32_t *) dstsurf->mLogicalAddr,
        dstsurf->mStride);
        dstsurf->mData = (gctPOINTER)dstimage;
        //if ( pBlt->mDstSurfInfo.mFormat.mBpp != 32 )
        //pixman_image_set_accessors(dstimage, real_reader, real_writer);
    } else {
        dstimage = (pixman_image_t *)dstsurf->mData;
    }

    pixman_image_composite(pBlt->mBlendOp.mOp, srcimage, NULL, dstimage, srcX, srcY, 0, 0, dstX, dstY, width, height);
    pVivPixSrc->mCpuBusy = TRUE;
    pVivPixDst->mCpuBusy = TRUE;
}
/**
 * PrepareSolid() sets up the driver for doing a solid fill.
 * @param pPixmap Destination pixmap
 * @param alu raster operation
 * @param planemask write mask for the fill
 * @param fg "foreground" color for the fill
 *
 * This call should set up the driver for doing a series of solid fills
 * through the Solid() call.  The alu raster op is one of the GX*
 * graphics functions listed in X.h, and typically maps to a similar
 * single-byte "ROP" setting in all hardware.  The planemask controls
 * which bits of the destination should be affected, and will only represent
 * the bits up to the depth of pPixmap.  The fg is the pixel value of the
 * foreground color referred to in ROP descriptions.
 *
 * Note that many drivers will need to store some of the data in the driver
 * private record, for sending to the hardware with each drawing command.
 *
 * The PrepareSolid() call is required of all drivers, but it may fail for any
 * reason.  Failure results in a fallback to software rendering.
 */
Bool
G2dPrepareSolid(PixmapPtr pPixmap, int alu, Pixel planemask, Pixel fg) {
    Viv2DPixmapPtr pVivPixDst = exaGetPixmapDriverPrivate(pPixmap);
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pPixmap);
    G2DBLITINFOPTR pBlt = &pViv->mGrCtx.mG2dBlitInfo;

    EXA_FAIL_IF(planemask != FB_ALLONES);
    EXA_FAIL_IF(alu != GXcopy);
    EXA_FAIL_IF(!EXA_PM_IS_SOLID(&pPixmap->drawable, planemask));
    SURF_SIZE_FOR_SW(pPixmap->drawable.width, pPixmap->drawable.height);

    if(!GetDefaultG2dFormat(pPixmap->drawable.bitsPerPixel, &pBlt->mDstSurfInfo.mFormat))
    {
        TRACE_EXA("%s fail dst bpp:%d",__FUNCTION__,pPixmap->drawable.bitsPerPixel);
        return FALSE;
    }

    /*Populating the information*/
    pBlt->mDstSurfInfo.mHeight = pPixmap->drawable.height;
    pBlt->mDstSurfInfo.mWidth = pPixmap->drawable.width;
    pBlt->mDstSurfInfo.mStride = pPixmap->devKind;
    pBlt->mDstSurfInfo.mPriv = pVivPixDst;
    pBlt->mColorARGB32 = fg;
    pBlt->mColorConvert = FALSE;
    pBlt->mPlaneMask = planemask;

    return TRUE;
}

/**
 * Solid() performs a solid fill set up in the last PrepareSolid() call.
 *
 * @param pPixmap destination pixmap
 * @param x1 left coordinate
 * @param y1 top coordinate
 * @param x2 right coordinate
 * @param y2 bottom coordinate
 *
 * Performs the fill set up by the last PrepareSolid() call, covering the
 * area from (x1,y1) to (x2,y2) in pPixmap.  Note that the coordinates are
 * in the coordinate space of the destination pixmap, so the driver will
 * need to set up the hardware's offset and pitch for the destination
 * coordinates according to the pixmap's offset and pitch within
 * framebuffer.  This likely means using exaGetPixmapOffset() and
 * exaGetPixmapPitch().
 *
 * This call is required if PrepareSolid() ever succeeds.
 */

void
G2dSolid(PixmapPtr pPixmap, int x1, int y1, int x2, int y2) {
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pPixmap);
    VIVGPUPtr pGpuCtx = (VIVGPUPtr)(pViv->mGrCtx.mGpu);
    G2DBLITINFOPTR pBlt = &pViv->mGrCtx.mG2dBlitInfo;
    static int _last_hw_solid = 0;

    Viv2DPixmapPtr pVivPixDst = NULL;

    GenericSurfacePtr pDstSurf = NULL;

    pVivPixDst = pBlt->mDstSurfInfo.mPriv;
    pDstSurf = (GenericSurfacePtr)pVivPixDst->mVidMemInfo;

    TRACE_EXA("%s (%d,%d,%d,%d) w:%d h:%d format:%d clrcolor:%x planemask:%x hwMask:%x",
                __FUNCTION__,x1,y1,x2,y2,x2-x1,y2-y1,
                pBlt->mDstSurfInfo.mFormat.mG2dFmt,
                pBlt->mColorARGB32,pBlt->mPlaneMask,pBlt->hwMask);
    TRACE_EXA("%s dst VivPixmap:%p bpp:%d cpubusy:%d mHWPath:%d w:%d h:%d",
            __FUNCTION__,pVivPixDst,pBlt->mDstSurfInfo.mFormat.mBpp,
             pVivPixDst->mCpuBusy,pVivPixDst->mHWPath,
             pDstSurf->mAlignedWidth,pDstSurf->mAlignedHeight);

    pBlt->mDstG2dSurf.base.planes[0] = pDstSurf->mVideoNode.mPhysicalAddr + pGpuCtx->mDriver->mG2DBaseAddr;
    pBlt->mDstG2dSurf.base.left = x1;
    pBlt->mDstG2dSurf.base.top = y1;
    pBlt->mDstG2dSurf.base.right = x2;
    pBlt->mDstG2dSurf.base.bottom = y2;
    pBlt->mDstG2dSurf.base.stride = pDstSurf->mAlignedWidth;
    pBlt->mDstG2dSurf.base.width = pDstSurf->mAlignedWidth;
    pBlt->mDstG2dSurf.base.height = pDstSurf->mAlignedHeight;
    pBlt->mDstG2dSurf.base.format = pBlt->mDstSurfInfo.mFormat.mG2dFmt;
    pBlt->mDstG2dSurf.base.rot = G2D_ROTATION_0;
    pBlt->mDstG2dSurf.tiling = G2D_LINEAR;

    pBlt->mSwsolid = FALSE;

    /* when surface > IMX_EXA_NONCACHESURF_SIZE but actual solid size < IMX_EXA_NONCACHESURF_SIZE, go sw path */
    if (((  x2 - x1 ) * ( y2 - y1 ) < IMX_EXA_NONCACHESURF_SIZE ) &&
        0 == (pViv->mGrCtx.mG2dBlitInfo.mDstSurfInfo.mStride%4))
    {

        if (_last_hw_solid >0)
        {
            g2d_finish(pGpuCtx->mDriver->mG2DHandle);
        }
        _last_hw_solid = 0;

        pVivPixDst->mCpuBusy = TRUE;

        /* mStride should be 4 aligned cause width is 8 aligned,Stride%4 !=0 shouldn't happen */

        pixman_fill((uint32_t *) MapViv2DPixmap(pVivPixDst),
                    pViv->mGrCtx.mG2dBlitInfo.mDstSurfInfo.mStride/4,
                    pViv->mGrCtx.mG2dBlitInfo.mDstSurfInfo.mFormat.mBpp,
                    x1, y1 , x2-x1, y2-y1, pViv->mGrCtx.mG2dBlitInfo.mColorARGB32);
        pBlt->mSwsolid = TRUE;
        return;
    }

    if(pVivPixDst->mCpuBusy)
    {
        VIV2DCacheOperation(&pViv->mGrCtx, pVivPixDst, FLUSH);
        pVivPixDst->mCpuBusy=FALSE;
    }

    if(pPixmap->drawable.bitsPerPixel == 16)
    {
        pBlt->mDstG2dSurf.base.clrcolor = (((pBlt->mColorARGB32 >> 8 & 0xF8 ) | (pBlt->mColorARGB32 >> 11 & 0x7)) |
                                          ((pBlt->mColorARGB32 >> 3 & 0xFC ) | (pBlt->mColorARGB32 >> 5 & 0x3))<<8 |
                                          ((pBlt->mColorARGB32 << 3 & 0xF8 ) | (pBlt->mColorARGB32 & 0x7))<<16 |
                                          0xff000000) & pBlt->mPlaneMask;
    }
    else
    {
        pBlt->mDstG2dSurf.base.clrcolor = pBlt->mColorARGB32 & pBlt->mPlaneMask;
    }

    g2d_clear(pGpuCtx->mDriver->mG2DHandle, &(pBlt->mDstG2dSurf.base));
    _last_hw_solid = 1;
    return;
}

/**
 * DoneSolid() finishes a set of solid fills.
 *
 * @param pPixmap destination pixmap.
 *
 * The DoneSolid() call is called at the end of a series of consecutive
 * Solid() calls following a successful PrepareSolid().  This allows drivers
 * to finish up emitting drawing commands that were buffered, or clean up
 * state from PrepareSolid().
 *
 * This call is required if PrepareSolid() ever succeeds.
 */
void
G2dDoneSolid(PixmapPtr pPixmap) {
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pPixmap);
    VIVGPUPtr pGpuCtx = (VIVGPUPtr)(pViv->mGrCtx.mGpu);
    G2DBLITINFOPTR pBlt = &pViv->mGrCtx.mG2dBlitInfo;

    TRACE_EXA("%s:%d",__FUNCTION__,__LINE__);
    if ( pBlt && pBlt->mSwsolid )
        return;
    g2d_finish(pGpuCtx->mDriver->mG2DHandle);
    return;
}

/**
 * PrepareCopy() sets up the driver for doing a copy within video
 * memory.
 *
 * @param pSrcPixmap source pixmap
 * @param pDstPixmap destination pixmap
 * @param dx X copy direction
 * @param dy Y copy direction
 * @param alu raster operation
 * @param planemask write mask for the fill
 *
 * This call should set up the driver for doing a series of copies from the
 * the pSrcPixmap to the pDstPixmap.  The dx flag will be positive if the
 * hardware should do the copy from the left to the right, and dy will be
 * positive if the copy should be done from the top to the bottom.  This
 * is to deal with self-overlapping copies when pSrcPixmap == pDstPixmap.
 * If your hardware can only support blits that are (left to right, top to
 * bottom) or (right to left, bottom to top), then you should set
 * #EXA_TWO_BITBLT_DIRECTIONS, and EXA will break down Copy operations to
 * ones that meet those requirements.  The alu raster op is one of the GX*
 * graphics functions listed in X.h, and typically maps to a similar
 * single-byte "ROP" setting in all hardware.  The planemask controls which
 * bits of the destination should be affected, and will only represent the
 * bits up to the depth of pPixmap.
 *
 * Note that many drivers will need to store some of the data in the driver
 * private record, for sending to the hardware with each drawing command.
 *
 * The PrepareCopy() call is required of all drivers, but it may fail for any
 * reason.  Failure results in a fallback to software rendering.
 */
Bool
G2dPrepareCopy(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap,
    int xdir, int ydir, int alu, Pixel planemask) {
    Viv2DPixmapPtr pVivPixSrc = exaGetPixmapDriverPrivate(pSrcPixmap);
    Viv2DPixmapPtr pVivPixDst = exaGetPixmapDriverPrivate(pDstPixmap);
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pDstPixmap);
    G2DBLITINFOPTR pBlt = &pViv->mGrCtx.mG2dBlitInfo;

    EXA_FAIL_IF(planemask != FB_ALLONES);
    EXA_FAIL_IF(alu != GXcopy);

    if ( pVivPixSrc->mHWPath == FALSE && pVivPixDst->mHWPath == FALSE )
    {
        SURF_SIZE_FOR_SW(pSrcPixmap->drawable.width, pSrcPixmap->drawable.height);
        SURF_SIZE_FOR_SW(pDstPixmap->drawable.width, pDstPixmap->drawable.height);
    }

    if(!GetDefaultG2dFormat(pSrcPixmap->drawable.bitsPerPixel, &pBlt->mSrcSurfInfo.mFormat))
    {
        TRACE_EXA("%s fail src bpp:%d",__FUNCTION__,pSrcPixmap->drawable.bitsPerPixel);
        return FALSE;
    }

    if(!GetDefaultG2dFormat(pDstPixmap->drawable.bitsPerPixel, &pBlt->mDstSurfInfo.mFormat))
    {
        TRACE_EXA("%s fail dst bpp:%d",__FUNCTION__,pDstPixmap->drawable.bitsPerPixel);
        return FALSE;
    }

    pBlt->mSrcSurfInfo.mHeight = pSrcPixmap->drawable.height;
    pBlt->mSrcSurfInfo.mWidth = pSrcPixmap->drawable.width;
    pBlt->mSrcSurfInfo.mStride = pSrcPixmap->devKind;
    pBlt->mSrcSurfInfo.mPriv = pVivPixSrc;

    pBlt->mDstSurfInfo.mHeight = pDstPixmap->drawable.height;
    pBlt->mDstSurfInfo.mWidth = pDstPixmap->drawable.width;
    pBlt->mDstSurfInfo.mStride = pDstPixmap->devKind;
    pBlt->mDstSurfInfo.mPriv = pVivPixDst;

    return TRUE;
}

/**
 * Copy() performs a copy set up in the last PrepareCopy call.
 *
 * @param pDstPixmap destination pixmap
 * @param srcX source X coordinate
 * @param srcY source Y coordinate
 * @param dstX destination X coordinate
 * @param dstY destination Y coordinate
 * @param width width of the rectangle to be copied
 * @param height height of the rectangle to be copied.
 *
 * Performs the copy set up by the last PrepareCopy() call, copying the
 * rectangle from (srcX, srcY) to (srcX + width, srcY + width) in the source
 * pixmap to the same-sized rectangle at (dstX, dstY) in the destination
 * pixmap.  Those rectangles may overlap in memory, if
 * pSrcPixmap == pDstPixmap.  Note that this call does not receive the
 * pSrcPixmap as an argument -- if it's needed in this function, it should
 * be stored in the driver private during PrepareCopy().  As with Solid(),
 * the coordinates are in the coordinate space of each pixmap, so the driver
 * will need to set up source and destination pitches and offsets from those
 * pixmaps, probably using exaGetPixmapOffset() and exaGetPixmapPitch().
 *
 * This call is required if PrepareCopy ever succeeds.
 *
**/
void
G2dCopy(PixmapPtr pDstPixmap, int srcX, int srcY,
    int dstX, int dstY, int width, int height) {
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pDstPixmap);
    VIVGPUPtr pGpuCtx = (VIVGPUPtr)(pViv->mGrCtx.mGpu);
    G2DBLITINFOPTR pBlt = &pViv->mGrCtx.mG2dBlitInfo;
    static int  _support_pixman_blit = 1;
    static int  _last_hw_cpy = 0;

    Viv2DPixmapPtr pVivPixSrc = NULL;
    Viv2DPixmapPtr pVivPixDst = NULL;
    GenericSurfacePtr pSrcSurf = NULL;
    GenericSurfacePtr pDstSurf = NULL;

    pVivPixSrc = pBlt->mSrcSurfInfo.mPriv;
    pVivPixDst = pBlt->mDstSurfInfo.mPriv;
    pSrcSurf = (GenericSurfacePtr)pVivPixSrc->mVidMemInfo;
    pDstSurf = (GenericSurfacePtr)pVivPixDst->mVidMemInfo;

    TRACE_EXA("%s src x:%d y:%d dst x:%d y:%d w:%d h:%d hwMask:%x",
                __FUNCTION__,srcX,srcY,dstX,dstY,width,height,pBlt->hwMask);

    TRACE_EXA("%s src VivPixmap:%p bpp:%d cpubusy:%d mHWPath:%d w:%d h:%d dst VivPixmap:%p bpp:%d cpubusy:%d mHWPath:%d w:%d h%d",
                __FUNCTION__,pVivPixSrc,pBlt->mSrcSurfInfo.mFormat.mBpp,
                pVivPixSrc->mCpuBusy,pVivPixSrc->mHWPath,
                pSrcSurf->mAlignedWidth,pSrcSurf->mAlignedHeight,
                pVivPixDst,pBlt->mDstSurfInfo.mFormat.mBpp,
                pVivPixDst->mCpuBusy,pVivPixDst->mHWPath,
                pDstSurf->mAlignedWidth,pDstSurf->mAlignedHeight);


    pBlt->mSrcG2dSurf.base.planes[0] = pSrcSurf->mVideoNode.mPhysicalAddr + pGpuCtx->mDriver->mG2DBaseAddr;
    pBlt->mSrcG2dSurf.base.left = srcX;
    pBlt->mSrcG2dSurf.base.top = srcY;
    pBlt->mSrcG2dSurf.base.right = srcX + width;
    pBlt->mSrcG2dSurf.base.bottom = srcY + height;
    pBlt->mSrcG2dSurf.base.stride = pSrcSurf->mAlignedWidth;
    pBlt->mSrcG2dSurf.base.width = pSrcSurf->mAlignedWidth;
    pBlt->mSrcG2dSurf.base.height = pSrcSurf->mAlignedHeight;
    pBlt->mSrcG2dSurf.base.format = pBlt->mSrcSurfInfo.mFormat.mG2dFmt;
    pBlt->mSrcG2dSurf.base.rot = G2D_ROTATION_0;
    pBlt->mSrcG2dSurf.tiling = G2D_LINEAR;

    pBlt->mDstG2dSurf.base.planes[0] = pDstSurf->mVideoNode.mPhysicalAddr + pGpuCtx->mDriver->mG2DBaseAddr;
    pBlt->mDstG2dSurf.base.left = dstX;
    pBlt->mDstG2dSurf.base.top = dstY;
    pBlt->mDstG2dSurf.base.right = dstX + width;
    pBlt->mDstG2dSurf.base.bottom = dstY + height;
    pBlt->mDstG2dSurf.base.stride = pDstSurf->mAlignedWidth;
    pBlt->mDstG2dSurf.base.width = pDstSurf->mAlignedWidth;
    pBlt->mDstG2dSurf.base.height = pDstSurf->mAlignedHeight;
    pBlt->mDstG2dSurf.base.format = pBlt->mDstSurfInfo.mFormat.mG2dFmt;
    pBlt->mDstG2dSurf.base.rot = G2D_ROTATION_0;
    pBlt->mDstG2dSurf.tiling = G2D_LINEAR;

    pBlt->mSwcpy = FALSE;

    if ( pVivPixSrc->mHWPath == FALSE &&
         pVivPixDst->mHWPath == FALSE &&
         (0 == (pViv->mGrCtx.mG2dBlitInfo.mDstSurfInfo.mStride%4)) &&
         (0 == (pViv->mGrCtx.mG2dBlitInfo.mSrcSurfInfo.mStride%4)))
    {
        /* when surface > IMX_EXA_NONCACHESURF_SIZE but actual copy size < IMX_EXA_NONCACHESURF_SIZE, go sw path */
        if ( ( width * height ) < IMX_EXA_NONCACHESURF_SIZE
            && _support_pixman_blit )
        {
            if ( MapViv2DPixmap(pVivPixSrc) != MapViv2DPixmap(pVivPixDst) )
            {
                pVivPixDst->mCpuBusy = TRUE;
                pVivPixSrc->mCpuBusy = TRUE;

                if (_last_hw_cpy > 0)
                {
                    g2d_finish(pGpuCtx->mDriver->mG2DHandle);
                }
                _last_hw_cpy = 0;

                if ( pixman_blt((uint32_t *) MapViv2DPixmap(pVivPixSrc),
                    (uint32_t *) MapViv2DPixmap(pVivPixDst),
                    pViv->mGrCtx.mG2dBlitInfo.mSrcSurfInfo.mStride/4,
                    pViv->mGrCtx.mG2dBlitInfo.mDstSurfInfo.mStride/4,
                    pViv->mGrCtx.mG2dBlitInfo.mSrcSurfInfo.mFormat.mBpp,
                    pViv->mGrCtx.mG2dBlitInfo.mDstSurfInfo.mFormat.mBpp,
                    srcX,
                    srcY,
                    dstX,
                    dstY,
                    width,
                    height) )
                {
                    pBlt->mSwcpy = TRUE;
                    return;
                } else {
                    _support_pixman_blit = 0;
                }
            }
        }
    }

    pVivPixSrc->mHWPath = FALSE;
    pVivPixDst->mHWPath = FALSE;
    if(pVivPixSrc->mCpuBusy)
    {
        VIV2DCacheOperation(&pViv->mGrCtx, pVivPixSrc, FLUSH);
        pVivPixSrc->mCpuBusy=FALSE;
    }

    if(pVivPixDst->mCpuBusy)
    {
        VIV2DCacheOperation(&pViv->mGrCtx,pVivPixDst, FLUSH);
        pVivPixDst->mCpuBusy=FALSE;
    }

    g2d_blitSurface(pGpuCtx->mDriver->mG2DHandle,&pBlt->mSrcG2dSurf,&pBlt->mDstG2dSurf, "G2dCopy");
    _last_hw_cpy = 1;
    return;
}

/**
 * DoneCopy() finishes a set of copies.
 *
 * @param pPixmap destination pixmap.
 *
 * The DoneCopy() call is called at the end of a series of consecutive
 * Copy() calls following a successful PrepareCopy().  This allows drivers
 * to finish up emitting drawing commands that were buffered, or clean up
 * state from PrepareCopy().
 *
 * This call is required if PrepareCopy() ever succeeds.
 */
void
G2dDoneCopy(PixmapPtr pDstPixmap) {
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pDstPixmap);
    VIVGPUPtr pGpuCtx = (VIVGPUPtr)(pViv->mGrCtx.mGpu);
    G2DBLITINFOPTR pBlt = &pViv->mGrCtx.mG2dBlitInfo;

    TRACE_EXA("%s:%d",__FUNCTION__,__LINE__);
    if ( pBlt && pBlt->mSwcpy )
        return;
    g2d_finish(pGpuCtx->mDriver->mG2DHandle);
    return;
}

/**
 * CheckComposite() checks to see if a composite operation could be
 * accelerated.
 *
 * @param op Render operation
 * @param pSrcPicture source Picture
 * @param pMaskPicture mask picture
 * @param pDstPicture destination Picture
 *
 * The CheckComposite() call checks if the driver could handle acceleration
 * of op with the given source, mask, and destination pictures.  This allows
 * drivers to check source and destination formats, supported operations,
 * transformations, and component alpha state, and send operations it can't
 * support to software rendering early on.  This avoids costly pixmap
 * migration to the wrong places when the driver can't accelerate
 * operations.  Note that because migration hasn't happened, the driver
 * can't know during CheckComposite() what the offsets and pitches of the
 * pixmaps are going to be.
 *
 * See PrepareComposite() for more details on likely issues that drivers
 * will have in accelerating Composite operations.
 *
 * The CheckComposite() call is recommended if PrepareComposite() is
 * implemented, but is not required.
 */
Bool
G2dCheckComposite(int op, PicturePtr pSrc, PicturePtr pMsk, PicturePtr pDst) {
    VivPtr pViv = NULL;
    G2DBLITINFOPTR pBlt = NULL;
    PixmapPtr pPixSrc = G2dGetDrawablePixmap(pSrc->pDrawable);
    PixmapPtr pPixDst = G2dGetDrawablePixmap(pDst->pDrawable);

    EXA_FAIL_IF(!pPixSrc);
    EXA_FAIL_IF(!pPixDst);

    pViv = VIVPTR_FROM_PIXMAP(pPixDst);
    pBlt = &pViv->mGrCtx.mG2dBlitInfo;

    EXA_FAIL_IF((pPixDst->drawable.width * pPixDst->drawable.height) < IMX_EXA_MIN_PIXEL_AREA_COMPOSITE );

    /*No Gradient*/
    EXA_FAIL_IF(pSrc->pSourcePict);
    /*Mask Related Checks*/
    EXA_FAIL_IF(pMsk);
    /*Not supported op*/
    EXA_FAIL_IF(!GetG2dBlendingFactors(op, &pBlt->mBlendOp));

    /*Format Checks*/
    if (!GetG2dPictureFormat(pSrc->format, &pBlt->mSrcSurfInfo.mFormat)) {
        TRACE_EXA("%s Src Format:%d is not supported",__FUNCTION__,pSrc->format);
        return FALSE;
    }

    if (!GetG2dPictureFormat(pDst->format, &pBlt->mDstSurfInfo.mFormat)) {
        TRACE_EXA("%s Dest Format:%d is not supported",__FUNCTION__,pDst->format);
        return FALSE;
    }
    /*Have some ideas about this but now let it be*/
    EXA_FAIL_IF(!G2dTransformSupported(pSrc->transform ,&pBlt->mRotation ));

    return TRUE;
}

/**
 * PrepareComposite() sets up the driver for doing a Composite operation
 * described in the Render extension protocol spec.
 *
 * @param op Render operation
 * @param pSrcPicture source Picture
 * @param pMaskPicture mask picture
 * @param pDstPicture destination Picture
 * @param pSrc source pixmap
 * @param pMask mask pixmap
 * @param pDst destination pixmap
 *
 * This call should set up the driver for doing a series of Composite
 * operations, as described in the Render protocol spec, with the given
 * pSrcPicture, pMaskPicture, and pDstPicture.  The pSrc, pMask, and
 * pDst are the pixmaps containing the pixel data, and should be used for
 * setting the offset and pitch used for the coordinate spaces for each of
 * the Pictures.
 *
 * Notes on interpreting Picture structures:
 * - The Picture structures will always have a valid pDrawable.
 * - The Picture structures will never have alphaMap set.
 * - The mask Picture (and therefore pMask) may be NULL, in which case the
 *   operation is simply src OP dst instead of src IN mask OP dst, and
 *   mask coordinates should be ignored.
 * - pMarkPicture may have componentAlpha set, which greatly changes
 *   the behavior of the Composite operation.  componentAlpha has no effect
 *   when set on pSrcPicture or pDstPicture.
 * - The source and mask Pictures may have a transformation set
 *   (Picture->transform != NULL), which means that the source coordinates
 *   should be transformed by that transformation, resulting in scaling,
 *   rotation, etc.  The PictureTransformPoint() call can transform
 *   coordinates for you.  Transforms have no effect on Pictures when used
 *   as a destination.
 * - The source and mask pictures may have a filter set.  PictFilterNearest
 *   and PictFilterBilinear are defined in the Render protocol, but others
 *   may be encountered, and must be handled correctly (usually by
 *   PrepareComposite failing, and falling back to software).  Filters have
 *   no effect on Pictures when used as a destination.
 * - The source and mask Pictures may have repeating set, which must be
 *   respected.  Many chipsets will be unable to support repeating on
 *   pixmaps that have a width or height that is not a power of two.
 *
 * If your hardware can't support source pictures (textures) with
 * non-power-of-two pitches, you should set #EXA_OFFSCREEN_ALIGN_POT.
 *
 * Note that many drivers will need to store some of the data in the driver
 * private record, for sending to the hardware with each drawing command.
 *
 * The PrepareComposite() call is not required.  However, it is highly
 * recommended for performance of antialiased font rendering and performance
 * of cairo applications.  Failure results in a fallback to software
 * rendering.
 */
Bool
G2dPrepareComposite(int op, PicturePtr pSrc, PicturePtr pMsk,
    PicturePtr pDst, PixmapPtr pxSrc, PixmapPtr pxMsk, PixmapPtr pxDst) {
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pxDst);
    G2DBLITINFOPTR pBlt = &pViv->mGrCtx.mG2dBlitInfo;
    /*Private Pixmaps*/
    Viv2DPixmapPtr pVivPixSrc = exaGetPixmapDriverPrivate(pxSrc);
    Viv2DPixmapPtr pVivPixDst = exaGetPixmapDriverPrivate(pxDst);

    TRACE_EXA("%s:%d op:%d",__FUNCTION__,__LINE__,op);
    /*Not supported op*/
    EXA_FAIL_IF(!GetG2dBlendingFactors(op, &pBlt->mBlendOp));

    /*Format Checks*/
    if (!GetG2dPictureFormat(pSrc->format, &pBlt->mSrcSurfInfo.mFormat)) {
        TRACE_EXA("%s Src Format:%d is not supported",__FUNCTION__,pSrc->format);
        return FALSE;
    }

    if (!GetG2dPictureFormat(pDst->format, &pBlt->mDstSurfInfo.mFormat)) {
        TRACE_EXA("%s Dest Format:%d is not supported",__FUNCTION__,pDst->format);
        return FALSE;
    }

    /*Have some ideas about this but now let it be*/
    EXA_FAIL_IF(!G2dTransformSupported(pSrc->transform ,&pBlt->mRotation ));

    /*START-Populating the info for src and dst*/
    pBlt->mSrcSurfInfo.alpha = PICT_FORMAT_A(pSrc->format);
    pBlt->mSrcSurfInfo.mWidth = pxSrc->drawable.width;
    pBlt->mSrcSurfInfo.mHeight = pxSrc->drawable.height;
    pBlt->mSrcSurfInfo.mStride = pxSrc->devKind;
    pBlt->mSrcSurfInfo.mPriv = pVivPixSrc;

    pBlt->mDstSurfInfo.alpha = PICT_FORMAT_A(pDst->format);
    pBlt->mDstSurfInfo.mWidth = pxDst->drawable.width;
    pBlt->mDstSurfInfo.mHeight = pxDst->drawable.height;
    pBlt->mDstSurfInfo.mStride = pxDst->devKind;
    pBlt->mDstSurfInfo.mPriv = pVivPixDst;

    return TRUE;
}
/**
 * Composite() performs a Composite operation set up in the last
 * PrepareComposite() call.
 *
 * @param pDstPixmap destination pixmap
 * @param srcX source X coordinate
 * @param srcY source Y coordinate
 * @param maskX source X coordinate
 * @param maskY source Y coordinate
 * @param dstX destination X coordinate
 * @param dstY destination Y coordinate
 * @param width destination rectangle width
 * @param height destination rectangle height
 *
 * Performs the Composite operation set up by the last PrepareComposite()
 * call, to the rectangle from (dstX, dstY) to (dstX + width, dstY + height)
 * in the destination Pixmap.  Note that if a transformation was set on
 * the source or mask Pictures, the source rectangles may not be the same
 * size as the destination rectangles and filtering.  Getting the coordinate
 * transformation right at the subpixel level can be tricky, and rendercheck
 * can test this for you.
 *
 * This call is required if PrepareComposite() ever succeeds.
 */

void
G2dComposite(PixmapPtr pxDst, int srcX, int srcY, int maskX, int maskY,
    int dstX, int dstY, int width, int height) {
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pxDst);
    VIVGPUPtr pGpuCtx = (VIVGPUPtr)(pViv->mGrCtx.mGpu);
    G2DBLITINFOPTR pBlt = &pViv->mGrCtx.mG2dBlitInfo;
    static int  _last_hw_composite = 0;

    Viv2DPixmapPtr pVivPixSrc = NULL;
    Viv2DPixmapPtr pVivPixDst = NULL;
    GenericSurfacePtr pSrcSurf = NULL;
    GenericSurfacePtr pDstSurf = NULL;

    pVivPixSrc = pBlt->mSrcSurfInfo.mPriv;
    pVivPixDst = pBlt->mDstSurfInfo.mPriv;
    pSrcSurf = (GenericSurfacePtr)pVivPixSrc->mVidMemInfo;
    pDstSurf = (GenericSurfacePtr)pVivPixDst->mVidMemInfo;

    CalG2dSurfParam(&(pBlt->mSrcG2dSurf.base),pBlt,srcX,srcY,srcX+width,srcY+height,pBlt->mRotation);
    pBlt->mSrcG2dSurf.base.planes[0] = pSrcSurf->mVideoNode.mPhysicalAddr + pGpuCtx->mDriver->mG2DBaseAddr;
    pBlt->mSrcG2dSurf.base.format = pBlt->mSrcSurfInfo.mFormat.mG2dFmt;
    pBlt->mSrcG2dSurf.base.blendfunc = pBlt->mBlendOp.mSrcBlendingFactor;
    pBlt->mSrcG2dSurf.tiling = G2D_LINEAR;

    if ((!pBlt->mDstSurfInfo.alpha)&&(pBlt->mBlendOp.dest_alpha))
    {
        if (pBlt->mBlendOp.mSrcBlendingFactor == G2D_DST_ALPHA)
            pBlt->mSrcG2dSurf.base.blendfunc = G2D_ONE;
        else if (pBlt->mBlendOp.mSrcBlendingFactor == G2D_ONE_MINUS_DST_ALPHA)
            pBlt->mSrcG2dSurf.base.blendfunc = G2D_ZERO;
    }

    TRACE_EXA("%s:%d src region(%d,%d,%d,%d) w:%d h:%d format:%d blend:%d",
        __FUNCTION__,__LINE__,
        pBlt->mSrcG2dSurf.base.left, pBlt->mSrcG2dSurf.base.top,
        pBlt->mSrcG2dSurf.base.right,pBlt->mSrcG2dSurf.base.bottom,
        pBlt->mSrcG2dSurf.base.width,pBlt->mSrcG2dSurf.base.height,
        pBlt->mSrcG2dSurf.base.format,pBlt->mSrcG2dSurf.base.blendfunc);

    pBlt->mDstG2dSurf.base.planes[0] = pDstSurf->mVideoNode.mPhysicalAddr + pGpuCtx->mDriver->mG2DBaseAddr;
    pBlt->mDstG2dSurf.base.left = dstX;
    pBlt->mDstG2dSurf.base.top = dstY;
    pBlt->mDstG2dSurf.base.right = dstX + width;
    pBlt->mDstG2dSurf.base.bottom = dstY + height;
    pBlt->mDstG2dSurf.base.stride = pDstSurf->mAlignedWidth;
    pBlt->mDstG2dSurf.base.width = pDstSurf->mAlignedWidth;
    pBlt->mDstG2dSurf.base.height = pDstSurf->mAlignedHeight;
    pBlt->mDstG2dSurf.base.format = pBlt->mDstSurfInfo.mFormat.mG2dFmt;
    pBlt->mDstG2dSurf.base.rot = G2D_ROTATION_0;
    pBlt->mDstG2dSurf.base.blendfunc = pBlt->mBlendOp.mDstBlendingFactor;
    pBlt->mDstG2dSurf.tiling = G2D_LINEAR;

    TRACE_EXA("%s:%d dst region(%d,%d,%d,%d) w:%d h:%d format:%d blend:%d",
        __FUNCTION__,__LINE__,
        pBlt->mDstG2dSurf.base.left, pBlt->mDstG2dSurf.base.top,
        pBlt->mDstG2dSurf.base.right,pBlt->mDstG2dSurf.base.bottom,
        pBlt->mDstG2dSurf.base.width,pBlt->mDstG2dSurf.base.height,
        pBlt->mDstG2dSurf.base.format,pBlt->mDstG2dSurf.base.blendfunc);

    pBlt->mSwcmp = FALSE;
    if ( ( width * height ) < MAX_COMPOSITE_SUB_SIZE &&
         (IMX_EXA_NONCACHESURF_SIZE > MAX_COMPOSITE_SUB_SIZE))
    {

        if (_last_hw_composite > 0)
            g2d_finish(pGpuCtx->mDriver->mG2DHandle);
        _last_hw_composite = 0;

        pBlt->mSwcmp = TRUE;
        G2dSWComposite(pxDst, srcX, srcY, maskX, maskY, dstX, dstY, width, height);
        return ;
    }

    if(pVivPixSrc->mCpuBusy)
    {
        VIV2DCacheOperation(&pViv->mGrCtx, pVivPixSrc, FLUSH);
        pVivPixSrc->mCpuBusy=FALSE;
    }

    if(pVivPixDst->mCpuBusy)
    {
        VIV2DCacheOperation(&pViv->mGrCtx, pVivPixDst, FLUSH);
        pVivPixSrc->mCpuBusy=FALSE;
    }
    g2d_enable(pGpuCtx->mDriver->mG2DHandle,G2D_BLEND);
    g2d_blitSurface(pGpuCtx->mDriver->mG2DHandle,&pBlt->mSrcG2dSurf,&pBlt->mDstG2dSurf, "G2dComposite");
    g2d_disable(pGpuCtx->mDriver->mG2DHandle,G2D_BLEND);
    _last_hw_composite = 1;
    return;
}

void
G2dDoneComposite(PixmapPtr pDst) {
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pDst);
    VIVGPUPtr pGpuCtx = (VIVGPUPtr)(pViv->mGrCtx.mGpu);
    G2DBLITINFOPTR pBlt = &pViv->mGrCtx.mG2dBlitInfo;

    TRACE_EXA("%s:%d",__FUNCTION__,__LINE__);
    if ( pBlt && pBlt->mSwcmp )
        return;
    g2d_finish(pGpuCtx->mDriver->mG2DHandle);
    return;
}

void
G2dEXASync(ScreenPtr pScreen, int marker) {
    return;
}

static Bool G2dDoneBySWCPY(PixmapPtr pPixmap, int x, int y, int w,
        int h, char *src, int src_pitch) {
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pPixmap);
    Viv2DPixmapPtr pVivPixDst = exaGetPixmapDriverPrivate(pPixmap);
    char * mDestAddr;
    int stride = 0, i = 0;
    int cpp = (pPixmap->drawable.bitsPerPixel + 7) / 8;

    EXA_FAIL_IF(!pViv);
    EXA_FAIL_IF(!pVivPixDst);

    stride = GetStride(pVivPixDst);
    mDestAddr = (char*) MapViv2DPixmap(pVivPixDst);

    EXA_FAIL_IF(!mDestAddr);

    mDestAddr += y*stride + x*cpp;

    for (i = 0; i < h; i++) {
        memcpy(mDestAddr, src, w * cpp);
        mDestAddr += stride;
        src += src_pitch;
    }

    pVivPixDst->mCpuBusy = TRUE;

    return TRUE;
}

static Bool G2dDoneByVSurf(PixmapPtr pDst, int x, int y, int w,
        int h, char *src, int src_pitch) {
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pDst);
    VIVGPUPtr pGpuCtx = (VIVGPUPtr)(pViv->mGrCtx.mGpu);
    Viv2DPixmapPtr pVivPixDst = exaGetPixmapDriverPrivate(pDst);
    G2DBLITINFOPTR pBlt = &(pViv->mGrCtx.mG2dBlitInfo);
    MemMapInfo mmap;
    struct g2d_surface g2d_surf_src;
    int aligned_pitch;
    int aligned_height;
    int aligned_width;
    char *aligned_start;
    int bytesperpixel;
    int maxsize;
    int height = h;
    BOOL retvsurf = TRUE;
    GenericSurfacePtr pDstSurf = NULL;

    pDstSurf = (GenericSurfacePtr)pVivPixDst->mVidMemInfo;

    if(!GetDefaultG2dFormat(pDst->drawable.bitsPerPixel, &pBlt->mDstSurfInfo.mFormat));
    {
        TRACE_EXA("%s fail dst bpp:%d",__FUNCTION__,pDst->drawable.bitsPerPixel);
        return FALSE;
    }

    maxsize = (w > h) ? w : h;
    VSetSurfIndex(1);
    switch (pDst->drawable.bitsPerPixel) {

        case 16:
            bytesperpixel = 2;
            retvsurf = VGetSurfAddrBy16(&pViv->mGrCtx, maxsize, (int *) (&mmap.physical), (int *) (&(mmap.mUserAddr)), &aligned_width, &aligned_height, &aligned_pitch);

            break;
        case 32:
            bytesperpixel = 4;
            retvsurf = VGetSurfAddrBy32(&pViv->mGrCtx, maxsize, (int *) (&mmap.physical), (int *) (&(mmap.mUserAddr)), &aligned_width, &aligned_height, &aligned_pitch);
            break;
        default:
            return FALSE;
    }

    EXA_FAIL_IF(retvsurf == FALSE);

    mmap.mSize = aligned_pitch*aligned_width;

    aligned_start = (char *) mmap.mUserAddr;

    while (height--) {
        memcpy(aligned_start, src, w * bytesperpixel);
        src += src_pitch;
        aligned_start += aligned_pitch;
    }

    pBlt->mSrcG2dSurf.base.planes[0] = mmap.physical + pGpuCtx->mDriver->mG2DBaseAddr;
    pBlt->mSrcG2dSurf.base.left = 0;
    pBlt->mSrcG2dSurf.base.top = 0;
    pBlt->mSrcG2dSurf.base.right = w;
    pBlt->mSrcG2dSurf.base.bottom = h;
    pBlt->mSrcG2dSurf.base.stride = aligned_width;
    pBlt->mSrcG2dSurf.base.width = aligned_width;
    pBlt->mSrcG2dSurf.base.height = aligned_height;
    pBlt->mSrcG2dSurf.base.rot = G2D_ROTATION_0;
    pBlt->mSrcG2dSurf.base.format = pBlt->mDstSurfInfo.mFormat.mG2dFmt;
    pBlt->mSrcG2dSurf.tiling = G2D_LINEAR;


    pBlt->mDstG2dSurf.base.planes[0] = pDstSurf->mVideoNode.mPhysicalAddr + pGpuCtx->mDriver->mG2DBaseAddr;
    pBlt->mDstG2dSurf.base.left = x;
    pBlt->mDstG2dSurf.base.top = y;
    pBlt->mDstG2dSurf.base.right = x + w;
    pBlt->mDstG2dSurf.base.bottom = y + h;
    pBlt->mDstG2dSurf.base.stride = pDstSurf->mAlignedWidth;
    pBlt->mDstG2dSurf.base.width = pDstSurf->mAlignedWidth;
    pBlt->mDstG2dSurf.base.height = pDstSurf->mAlignedHeight;
    pBlt->mDstG2dSurf.base.format = pBlt->mDstSurfInfo.mFormat.mG2dFmt;
    pBlt->mDstG2dSurf.base.rot = G2D_ROTATION_0;
    pBlt->mDstG2dSurf.tiling = G2D_LINEAR;

    pBlt->mDstSurfInfo.mHeight = pDst->drawable.height;
    pBlt->mDstSurfInfo.mWidth = pDst->drawable.width;

    if (pVivPixDst->mCpuBusy) {
       VIV2DCacheOperation(&pViv->mGrCtx,pVivPixDst,FLUSH);
       pVivPixDst->mCpuBusy = FALSE;
    }

    g2d_blitSurface(pGpuCtx->mDriver->mG2DHandle,&pBlt->mSrcG2dSurf,&pBlt->mDstG2dSurf, "G2dUploadToScreen");
    g2d_finish(pGpuCtx->mDriver->mG2DHandle);
    return TRUE;
}

Bool
G2dUploadToScreen(PixmapPtr pDst, int x, int y, int w,
    int h, char *src, int src_pitch) {
    TRACE_EXA("%s:%d w:%d h:%d",__FUNCTION__,__LINE__,w,h);
    if ( ( w*h ) < MAXSIZE_FORSWTOSCREEN )
        return G2dDoneBySWCPY(pDst, x, y, w, h, src, src_pitch);
    else
        return G2dDoneByVSurf(pDst, x, y, w, h, src, src_pitch);
}
#endif
