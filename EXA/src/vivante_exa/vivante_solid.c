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


#include "vivante_exa.h"
#include "vivante.h"


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
VivPrepareSolid(PixmapPtr pPixmap, int alu, Pixel planemask, Pixel fg) {
    TRACE_ENTER();
    Viv2DPixmapPtr pdst = exaGetPixmapDriverPrivate(pPixmap);
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pPixmap);
    int fgop = 0xF0;
    int bgop = 0xF0;

    SURF_SIZE_FOR_SW(pPixmap->drawable.width, pPixmap->drawable.height);

    if (!CheckFILLValidity(pPixmap, alu, planemask)) {
        TRACE_EXIT(FALSE);
    }
    if (!GetDefaultFormat(pPixmap->drawable.bitsPerPixel, &(pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mFormat))) {
        TRACE_EXIT(FALSE);
    }

    /*Populating the information*/
    pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mHeight = pPixmap->drawable.height;
    pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mWidth = pPixmap->drawable.width;
    pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride = pPixmap->devKind;
    pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mPriv = pdst;
    pViv->mGrCtx.mBlitInfo.mFgRop = fgop;
    pViv->mGrCtx.mBlitInfo.mBgRop = bgop;
    pViv->mGrCtx.mBlitInfo.mColorARGB32 = fg;
    pViv->mGrCtx.mBlitInfo.mColorConvert = FALSE;
    pViv->mGrCtx.mBlitInfo.mPlaneMask = planemask;
    pViv->mGrCtx.mBlitInfo.mOperationCode = VIVSOLID;

    TRACE_EXIT(TRUE);
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
VivSolid(PixmapPtr pPixmap, int x1, int y1, int x2, int y2) {
    TRACE_ENTER();
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pPixmap);
    Viv2DPixmapPtr pdst = exaGetPixmapDriverPrivate(pPixmap);
    static int _last_hw_solid = 0;

    VIV2DBLITINFOPTR pBlt = &pViv->mGrCtx.mBlitInfo;
    /*Setting up the rectangle*/
    pViv->mGrCtx.mBlitInfo.mDstBox.x1 = x1;
    pViv->mGrCtx.mBlitInfo.mDstBox.y1 = y1;
    pViv->mGrCtx.mBlitInfo.mDstBox.x2 = x2;
    pViv->mGrCtx.mBlitInfo.mDstBox.y2 = y2;

    pBlt->mSwsolid = FALSE;
    /* when surface > IMX_EXA_NONCACHESURF_SIZE but actual solid size < IMX_EXA_NONCACHESURF_SIZE, go sw path */
    if ( (  x2 - x1 ) * ( y2 - y1 ) < IMX_EXA_NONCACHESURF_SIZE )
    {

        if (_last_hw_solid >0)
            VIV2DGPUBlitComplete(&pViv->mGrCtx, TRUE);
        _last_hw_solid = 0;

        pdst->mCpuBusy = TRUE;

        /* mStride should be 4 aligned cause width is 8 aligned,Stride%4 !=0 shouldn't happen */
        gcmASSERT((pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride%4)==0);

        pixman_fill((uint32_t *) MapViv2DPixmap(pdst), pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride/4, pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mFormat.mBpp, x1, y1 , x2-x1, y2-y1, pViv->mGrCtx.mBlitInfo.mColorARGB32);

        pBlt->mSwsolid = TRUE;
        TRACE_EXIT();
    }

    if (pdst->mCpuBusy) {
        VIV2DCacheOperation(&pViv->mGrCtx, pdst,FLUSH);
        pdst->mCpuBusy = FALSE;
    }

    if (!SetDestinationSurface(&pViv->mGrCtx)) {
            TRACE_ERROR("Solid Blit Failed\n");
    }

    if (!SetClipping(&pViv->mGrCtx)) {
            TRACE_ERROR("Solid Blit Failed\n");
    }

    if (!SetSolidBrush(&pViv->mGrCtx)) {
            TRACE_ERROR("Solid Blit Failed\n");
    }

    if (!DoSolidBlit(&pViv->mGrCtx)) {
        TRACE_ERROR("Solid Blit Failed\n");
    }
    _last_hw_solid = 1;
    TRACE_EXIT();
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
VivDoneSolid(PixmapPtr pPixmap) {
    TRACE_ENTER();

    VivPtr pViv = VIVPTR_FROM_PIXMAP(pPixmap);
    VIV2DBLITINFOPTR pBlt = &pViv->mGrCtx.mBlitInfo;

    if ( pBlt && pBlt->mSwsolid )
        TRACE_EXIT();
    pBlt->hwMask |= 0x1;
    VIV2DGPUFlushGraphicsPipe(&pViv->mGrCtx);
    VIV2DGPUBlitComplete(&pViv->mGrCtx, FALSE);

    TRACE_EXIT();
}
