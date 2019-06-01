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


#include "vivante_exa.h"
#include "vivante.h"

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
VivPrepareCopy(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap,
    int xdir, int ydir, int alu, Pixel planemask) {
    TRACE_ENTER();
    Viv2DPixmapPtr psrc = exaGetPixmapDriverPrivate(pSrcPixmap);
    Viv2DPixmapPtr pdst = exaGetPixmapDriverPrivate(pDstPixmap);
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pDstPixmap);
    int fgop = 0xCC;
    int bgop = 0xCC;

    if ( psrc->mHWPath == FALSE && pdst->mHWPath == FALSE )
    {
        SURF_SIZE_FOR_SW(pSrcPixmap->drawable.width, pSrcPixmap->drawable.height);
        SURF_SIZE_FOR_SW(pDstPixmap->drawable.width, pDstPixmap->drawable.height);
    }

    if (!CheckCPYValidity(pDstPixmap, alu, planemask)) {
        TRACE_EXIT(FALSE);
    }

    if (!GetDefaultFormat(pSrcPixmap->drawable.bitsPerPixel, &(pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mFormat))) {
        TRACE_EXIT(FALSE);
    }

    if (!GetDefaultFormat(pDstPixmap->drawable.bitsPerPixel, &(pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mFormat))) {
        TRACE_EXIT(FALSE);
    }

    ConvertXAluToOPS(pDstPixmap, alu, planemask, &fgop,&bgop);

    pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mHeight = pDstPixmap->drawable.height;
    pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mWidth = pDstPixmap->drawable.width;
    pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride = pDstPixmap->devKind;
    pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mPriv = pdst;

    pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mHeight = pSrcPixmap->drawable.height;
    pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mWidth = pSrcPixmap->drawable.width;
    pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mStride = pSrcPixmap->devKind;
    pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mPriv = psrc;


    pViv->mGrCtx.mBlitInfo.mBgRop = fgop;
    pViv->mGrCtx.mBlitInfo.mFgRop = bgop;

    if ( alu == GXcopy)
        pViv->mGrCtx.mBlitInfo.mOperationCode = VIVSIMCOPY;
    else
        pViv->mGrCtx.mBlitInfo.mOperationCode = VIVCOPY;

    TRACE_EXIT(TRUE);
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
VivCopy(PixmapPtr pDstPixmap, int srcX, int srcY,
    int dstX, int dstY, int width, int height) {
    TRACE_ENTER();
    static int  _support_pixman_blit = 1;
    static int  _last_hw_cpy = 0;
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pDstPixmap);

    VIV2DBLITINFOPTR pBlt = &pViv->mGrCtx.mBlitInfo;


    Viv2DPixmapPtr psrc = NULL;
    Viv2DPixmapPtr pdst = NULL;
    pBlt->mSwcpy = FALSE;

    pdst = pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mPriv;
    psrc = pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mPriv;
    /*Setting up the rectangle*/
    pViv->mGrCtx.mBlitInfo.mDstBox.x1 = dstX;
    pViv->mGrCtx.mBlitInfo.mDstBox.y1 = dstY;
    pViv->mGrCtx.mBlitInfo.mDstBox.x2 = dstX + width;
    pViv->mGrCtx.mBlitInfo.mDstBox.y2 = dstY + height;

    pViv->mGrCtx.mBlitInfo.mSrcBox.x1 = srcX;
    pViv->mGrCtx.mBlitInfo.mSrcBox.y1 = srcY;
    pViv->mGrCtx.mBlitInfo.mSrcBox.x2 = srcX + width;
    pViv->mGrCtx.mBlitInfo.mSrcBox.y2 = srcY + height;

    if ( psrc->mHWPath == FALSE && pdst->mHWPath == FALSE )
    {
        /* when surface > IMX_EXA_NONCACHESURF_SIZE but actual copy size < IMX_EXA_NONCACHESURF_SIZE, go sw path */
        if ( ( width * height ) < IMX_EXA_NONCACHESURF_SIZE
            && pViv->mGrCtx.mBlitInfo.mOperationCode == VIVSIMCOPY
            && _support_pixman_blit )
        {

            /* mStride should be 4 aligned cause width is 8 aligned,Stride%4 !=0 shouldn't happen */
            gcmASSERT((pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride%4)==0);
            gcmASSERT((pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mStride%4)==0);

            if ( MapViv2DPixmap(psrc) != MapViv2DPixmap(pdst) )
            {
                pdst->mCpuBusy = TRUE;
                psrc->mCpuBusy = TRUE;

                if (_last_hw_cpy > 0)
                    VIV2DGPUBlitComplete(&pViv->mGrCtx,TRUE);
                _last_hw_cpy = 0;

                if ( pixman_blt((uint32_t *) MapViv2DPixmap(psrc),
                    (uint32_t *) MapViv2DPixmap(pdst),
                    pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mStride/4,
                    pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride/4,
                    pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mFormat.mBpp,
                    pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mFormat.mBpp,
                    srcX,
                    srcY,
                    dstX,
                    dstY,
                    width,
                    height) )
                {
                    pBlt->mSwcpy = TRUE;
                    TRACE_EXIT();
                } else {
                    _support_pixman_blit = 0;
                }
            }
        }
    }

    psrc->mHWPath = FALSE;
    pdst->mHWPath = FALSE;

    if (psrc->mCpuBusy) {
        VIV2DCacheOperation(&pViv->mGrCtx, psrc, FLUSH);
        psrc->mCpuBusy = FALSE;
    }

    if (pdst->mCpuBusy) {
        VIV2DCacheOperation(&pViv->mGrCtx,pdst, FLUSH);
        pdst->mCpuBusy = FALSE;
    }

    if (!SetDestinationSurface(&pViv->mGrCtx)) {
        TRACE_ERROR("Copy Blit Failed\n");
    }

    if (!SetSourceSurface(&pViv->mGrCtx)) {
        TRACE_ERROR("Copy Blit Failed\n");
    }

    if (!SetClipping(&pViv->mGrCtx)) {
        TRACE_ERROR("Copy Blit Failed\n");
    }

    if (!DoCopyBlit(&pViv->mGrCtx)) {
        TRACE_ERROR("Copy Blit Failed\n");
    }
    _last_hw_cpy = 1;
    TRACE_EXIT();
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
VivDoneCopy(PixmapPtr pDstPixmap) {

    TRACE_ENTER();
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pDstPixmap);
    VIV2DBLITINFOPTR pBlt = &pViv->mGrCtx.mBlitInfo;

    if ( pBlt && pBlt->mSwcpy )
        TRACE_EXIT();
    pBlt->hwMask |= 0x1;
    VIV2DGPUFlushGraphicsPipe(&pViv->mGrCtx);
    VIV2DGPUBlitComplete(&pViv->mGrCtx,FALSE);

    TRACE_EXIT();
}
