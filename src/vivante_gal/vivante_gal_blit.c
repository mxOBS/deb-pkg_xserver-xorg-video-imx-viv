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


#include "vivante_common.h"
#include "vivante_gal.h"
#include "vivante_priv.h"
#include "vivante.h"
/*
 * Blending operation factors.
 */
static const VivBlendOp
blendingOps[] = {
    {
        PictOpClear, gcvSURF_BLEND_ZERO, gcvSURF_BLEND_ZERO
    },
    {
        PictOpSrc, gcvSURF_BLEND_ONE, gcvSURF_BLEND_ZERO
    },
    {
        PictOpDst, gcvSURF_BLEND_ZERO, gcvSURF_BLEND_ONE
    },
    {
        PictOpOver, gcvSURF_BLEND_ONE, gcvSURF_BLEND_INVERSED
    },
    {
        PictOpOverReverse, gcvSURF_BLEND_INVERSED, gcvSURF_BLEND_ONE
    },
    {
        PictOpIn, gcvSURF_BLEND_STRAIGHT, gcvSURF_BLEND_ZERO
    },
    {
        PictOpInReverse, gcvSURF_BLEND_ZERO, gcvSURF_BLEND_STRAIGHT
    },
    {
        PictOpOut, gcvSURF_BLEND_INVERSED, gcvSURF_BLEND_ZERO
    },
    {
        PictOpOutReverse, gcvSURF_BLEND_ZERO, gcvSURF_BLEND_INVERSED
    },
    {
        PictOpAtop, gcvSURF_BLEND_STRAIGHT, gcvSURF_BLEND_INVERSED
    },
    {
        PictOpAtopReverse, gcvSURF_BLEND_INVERSED, gcvSURF_BLEND_STRAIGHT
    },
    {
        PictOpXor, gcvSURF_BLEND_INVERSED, gcvSURF_BLEND_INVERSED
    },
    {
        PictOpAdd, gcvSURF_BLEND_ONE, gcvSURF_BLEND_ONE
    },
    {
        PictOpSaturate, gcvSURF_BLEND_SRC_ALPHA_SATURATED, gcvSURF_BLEND_ONE
    }
};
/**
 * Picture Formats and their counter parts
 */
static const VivPictFormat
vivpict_formats[] = {
    {
        PICT_a8r8g8b8, 32, gcvSURF_A8R8G8B8, 8
    },
    {
        PICT_x8r8g8b8, 32, gcvSURF_X8R8G8B8, 0
    },
    {
        PICT_a8b8g8r8, 32, gcvSURF_A8B8G8R8, 8
    },
    {
        PICT_x8b8g8r8, 32, gcvSURF_X8B8G8R8, 0
    },
    {
        PICT_b8g8r8a8, 32, gcvSURF_B8G8R8A8, 8
    },
    {
        PICT_b8g8r8x8, 32, gcvSURF_B8G8R8X8, 0
    },
    {
        PICT_r8g8b8, 24, gcvSURF_R8G8B8, 0
    },
    {
        PICT_b8g8r8, 24, gcvSURF_B8G8R8, 0
    },
    {
        PICT_r5g6b5, 16, gcvSURF_R5G6B5, 0
    },
    {
        PICT_a1r5g5b5, 16, gcvSURF_A1R5G5B5, 1
    },
    {
        PICT_x1r5g5b5, 16, gcvSURF_X1R5G5B5, 0
    },
    {
        PICT_a1b5g5r5, 16, gcvSURF_A1B5G5R5, 1
    },
    {
        PICT_x1b5g5r5, 16, gcvSURF_X1B5G5R5, 0
    },
    {
        PICT_a4r4g4b4, 16, gcvSURF_A4R4G4B4, 4
    },
    {
        PICT_x4r4g4b4, 16, gcvSURF_X4R4G4B4, 0
    },
    {
        PICT_a4b4g4r4, 16, gcvSURF_A4B4G4R4, 4
    },
    {
        PICT_x4b4g4r4, 16, gcvSURF_X4B4G4R4, 0
    },
    {
        NO_PICT_FORMAT, 0, gcvSURF_UNKNOWN, 0
    } /*END*/
};

/**
 * Sets the solid brush
 * @param galInfo
 * @return
 */
Bool SetSolidBrush(GALINFOPTR galInfo) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    status = gco2D_LoadSolidBrush
            (
            gpuctx->mDriver->m2DEngine,
            (gceSURF_FORMAT) galInfo->mBlitInfo.mDstSurfInfo.mFormat.mVivFmt,
            (gctBOOL) galInfo->mBlitInfo.mColorConvert,
            galInfo->mBlitInfo.mColorARGB32,
            galInfo->mBlitInfo.mPlaneMask
            );
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("gco2D_LoadSolidBrush failed\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

/**
 * Sets the destination surface
 * @param galInfo
 * @return
 */
Bool SetDestinationSurface(GALINFOPTR galInfo) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    VIVGPUPtr gpuctx = (VIVGPUPtr) (galInfo->mGpu);
    VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);
    GenericSurfacePtr surf = (GenericSurfacePtr) (pBlt->mDstSurfInfo.mPriv->mVidMemInfo);

    status = gco2D_SetGenericTarget
            (
            gpuctx->mDriver->m2DEngine,
            &surf->mVideoNode.mPhysicalAddr,
            1,
            &surf->mStride,
            1,
            surf->mTiling,
            pBlt->mDstSurfInfo.mFormat.mVivFmt,
            surf->mRotation,
            surf->mAlignedWidth,
            surf->mAlignedHeight
            );

    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("gco2D_SetGenericTarget failed\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

/**
 * Sets the source surface
 * @param galInfo
 * @return
 */
Bool SetSourceSurface(GALINFOPTR galInfo) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    VIVGPUPtr gpuctx = (VIVGPUPtr) (galInfo->mGpu);
    VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);
    GenericSurfacePtr surf = (GenericSurfacePtr) (pBlt->mSrcSurfInfo.mPriv->mVidMemInfo);

    status = gco2D_SetGenericSource
            (
            gpuctx->mDriver->m2DEngine,
            &surf->mVideoNode.mPhysicalAddr,
            1,
            &surf->mStride,
            1,
            surf->mTiling,
            pBlt->mSrcSurfInfo.mFormat.mVivFmt,
            surf->mRotation,
            surf->mAlignedWidth,
            surf->mAlignedHeight
            );

    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("gco2D_SetGenericSource failed\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

/**
 * Sets clipping on the destination surface
 * @param galInfo
 * @return
 */
Bool SetClipping(GALINFOPTR galInfo) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    VIVGPUPtr gpuctx = (VIVGPUPtr) (galInfo->mGpu);
    gcsRECT dstRect = {0, 0, galInfo->mBlitInfo.mDstSurfInfo.mWidth, galInfo->mBlitInfo.mDstSurfInfo.mHeight};
    status = gco2D_SetClipping(gpuctx-> mDriver->m2DEngine, &dstRect);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("gco2D_SetClipping failed\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

/**
 * Clears a surface
 * @param pBltInfo
 * @param gpuctx
 * @return
 */
static Bool ClearSurface(VIV2DBLITINFOPTR pBltInfo, VIVGPUPtr gpuctx) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    gcsRECT dstRect = {pBltInfo->mDstBox.x1, pBltInfo->mDstBox.y1, pBltInfo->mDstBox.x2, pBltInfo->mDstBox.y2};
    status = gco2D_Clear(gpuctx->mDriver->m2DEngine, 1, &dstRect, pBltInfo->mColorARGB32, pBltInfo->mFgRop, pBltInfo->mBgRop, pBltInfo->mDstSurfInfo.mFormat.mVivFmt);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("gco2D_Clear failed\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

/**
 * Copy blitting
 * @param galInfo
 * @return
 */
Bool DoCopyBlit(GALINFOPTR galInfo) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    VIV2DBLITINFOPTR pBltInfo = &galInfo->mBlitInfo;
    gcsRECT srcRect = {pBltInfo->mSrcBox.x1, pBltInfo->mSrcBox.y1, pBltInfo->mSrcBox.x2, pBltInfo->mSrcBox.y2};
    gcsRECT dstRect = {pBltInfo->mDstBox.x1, pBltInfo->mDstBox.y1, pBltInfo->mDstBox.x2, pBltInfo->mDstBox.y2};

    status = gco2D_BatchBlit(
            gpuctx->mDriver->m2DEngine,
            1,
            &srcRect,
            &dstRect,
            pBltInfo->mFgRop,
            pBltInfo->mBgRop,
            pBltInfo->mDstSurfInfo.mFormat.mVivFmt
            );

    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Copy Blit Failed");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

/**
 * Solid filling
 * @param galInfo
 * @return
 */
Bool DoSolidBlit(GALINFOPTR galInfo) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    VIV2DBLITINFOPTR pBltInfo = &galInfo->mBlitInfo;
    gcsRECT dstRect = {pBltInfo->mDstBox.x1, pBltInfo->mDstBox.y1, pBltInfo->mDstBox.x2, pBltInfo->mDstBox.y2};
    status = gco2D_Blit(gpuctx->mDriver->m2DEngine, 1, &dstRect, pBltInfo->mFgRop, pBltInfo->mBgRop, pBltInfo->mDstSurfInfo.mFormat.mVivFmt);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Blit failed\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

/**
 * Blitting using mapped system memory
 * @param mmInfo
 * @param galInfo
 * @return
 */
Bool CopyBlitFromHost(MemMapInfoPtr mmInfo, GALINFOPTR galInfo) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    VIVGPUPtr gpuctx = (VIVGPUPtr) (galInfo->mGpu);
    VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);
    if (!SetDestinationSurface(galInfo)) {
        TRACE_EXIT(FALSE);
    }
    if (!SetClipping(galInfo)) {
        TRACE_EXIT(FALSE);
    }

    status = gco2D_SetGenericSource
            (
            gpuctx->mDriver->m2DEngine,
            &mmInfo->physical,
            1,
            &pBlt->mSrcSurfInfo.mStride,
            1,
            gcvLINEAR,
            pBlt->mSrcSurfInfo.mFormat.mVivFmt,
            gcvSURF_0_DEGREE,
            pBlt->mSrcSurfInfo.mWidth,
            pBlt->mSrcSurfInfo.mHeight
            );
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("gco2D_SetGenericSource failed - %d\n", status);
        TRACE_EXIT(FALSE);
    }

    if (!DoCopyBlit(galInfo)) {
        TRACE_ERROR("ERROR ON COPY BLIT\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

/**
 * Enables alpha blending
 * @param galInfo
 * @return
 */
Bool EnableAlphaBlending(GALINFOPTR galInfo) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;

    status = gco2D_EnableAlphaBlendAdvanced
            (
            gpuctx->mDriver->m2DEngine,
            gcvSURF_PIXEL_ALPHA_STRAIGHT, gcvSURF_PIXEL_ALPHA_STRAIGHT,
            gcvSURF_GLOBAL_ALPHA_OFF, gcvSURF_GLOBAL_ALPHA_OFF,
            (gceSURF_BLEND_FACTOR_MODE) galInfo->mBlitInfo.mBlendOp.mSrcBlendingFactor, (gceSURF_BLEND_FACTOR_MODE) galInfo->mBlitInfo.mBlendOp.mDstBlendingFactor
            );

    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("gco2D_EnableAlphaBlend\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

Bool DisableAlphaBlending(GALINFOPTR galInfo) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    status = gco2D_DisableAlphaBlend(gpuctx->mDriver->m2DEngine);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Alpha Blend Disabling failed\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

/**
 *
 * @param galInfo
 * @param opbox
 * @return
 */
static Bool composite_one_pass(GALINFOPTR galInfo, VivBoxPtr opbox) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    VivBoxPtr srcbox = &galInfo->mBlitInfo.mSrcBox;
    VivBoxPtr dstbox = &galInfo->mBlitInfo.mDstBox;
    gcsRECT mSrcClip = {srcbox->x1, srcbox->y1, srcbox->x1 + opbox->width, srcbox->y1 + opbox->height};
    gcsRECT mDstClip = {dstbox->x1, dstbox->y1, dstbox->x1 + opbox->width, dstbox->y1 + opbox->height};
    /*setting the source surface*/
    if (!SetSourceSurface(galInfo)) {
        TRACE_ERROR("ERROR SETTING SOURCE SURFACE\n");
        TRACE_EXIT(FALSE);
    }
    /*Setting the dest surface*/
    if (!SetDestinationSurface(galInfo)) {
        TRACE_ERROR("ERROR SETTING DST SURFACE\n");
        TRACE_EXIT(FALSE);
    }
    /*setting the clipping for dest*/
    if (!SetClipping(galInfo)) {
        TRACE_ERROR("ERROR SETTING DST CLIPPING\n");
        TRACE_EXIT(FALSE);
    }
    /*Enabling the alpha blending*/
    if (!EnableAlphaBlending(galInfo)) {
        TRACE_ERROR("Alpha Blending Factor\n");
        TRACE_EXIT(FALSE);
    }
    status = gco2D_BatchBlit(
            gpuctx->mDriver->m2DEngine,
            1,
            &mSrcClip,
            &mDstClip,
            0xCC,
            0xCC,
            galInfo->mBlitInfo.mDstSurfInfo.mFormat.mVivFmt
            );

    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Copy Blit Failed");
        TRACE_EXIT(FALSE);
    }

    if (!DisableAlphaBlending(galInfo)) {
        TRACE_ERROR("Disabling Alpha Blend Failed\n");
        TRACE_EXIT(FALSE);
    }

    TRACE_EXIT(TRUE);
}

/**
 *
 * @param galInfo
 * @param opbox
 * @return
 */
static Bool composite_stretch_blit_onebyonepixel(GALINFOPTR galInfo, VivBoxPtr opbox) {
    gceSTATUS status = gcvSTATUS_OK;
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    gcsRECT srcRect = {0, 0, 1, 1};
    gcsRECT dstRect = {galInfo->mBlitInfo.mDstBox.x1, galInfo->mBlitInfo.mDstBox.y1, galInfo->mBlitInfo.mDstBox.x1 + opbox->width, galInfo->mBlitInfo.mDstBox.y1 + opbox->height};
    /*setting the source surface*/
    if (!SetSourceSurface(galInfo)) {
        TRACE_ERROR("ERROR SETTING SOURCE SURFACE\n");
        TRACE_EXIT(FALSE);
    }
    /*setting the required area*/
    status = gco2D_SetSource(gpuctx->mDriver->m2DEngine, &srcRect);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("gco2D_SetSource failed\n");
        TRACE_EXIT(FALSE);
    }
    /*Setting the dest surface*/
    if (!SetDestinationSurface(galInfo)) {
        TRACE_ERROR("ERROR SETTING DST SURFACE\n");
        TRACE_EXIT(FALSE);
    }
    /*setting the clipping to the dst window for preventing the spill out*/
    status = gco2D_SetClipping(gpuctx-> mDriver->m2DEngine, &dstRect);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("gco2D_SetClipping failed\n");
        TRACE_EXIT(FALSE);
    }
    /*Enabling the alpha blending*/
    if (!EnableAlphaBlending(galInfo)) {
        TRACE_ERROR("Alpha Blending Factor\n");
        TRACE_EXIT(FALSE);
    }
    status = gco2D_StretchBlit(gpuctx-> mDriver->m2DEngine, 1, &dstRect, 0xCC, 0xCC, galInfo->mBlitInfo.mDstSurfInfo.mFormat.mVivFmt);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Copy Blit Failed");
        TRACE_EXIT(FALSE);
    }
    if (!DisableAlphaBlending(galInfo)) {
        TRACE_ERROR("Disabling Alpha Blend Failed\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

/**
 *
 * @param galInfo
 * @param opbox
 * @return
 */
static Bool composite_one_pass_special(GALINFOPTR galInfo, VivBoxPtr opbox) {
    gceSTATUS status = gcvSTATUS_OK;
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    Bool isFinished = FALSE;
    VivBoxPtr srcbox = &galInfo->mBlitInfo.mSrcBox;
    VivBoxPtr dstbox = &galInfo->mBlitInfo.mDstBox;
    int mSrcWidth = galInfo->mBlitInfo.mSrcSurfInfo.mWidth;
    int mSrcHeight = galInfo->mBlitInfo.mSrcSurfInfo.mHeight;
    int width, height, stx, sty;
    gcsRECT mSrcClip;
    gcsRECT mDstClip;

    stx = dstbox->x1;
    sty = dstbox->y1;

    /* Make sure srcX and srcY are in source region */
    srcbox->x1 = ((srcbox->x1 % (int) mSrcWidth) + (int) mSrcWidth)
            % (int) mSrcWidth;
    srcbox->y1 = ((srcbox->y1 % (int) mSrcHeight) + (int) mSrcHeight)
            % (int) mSrcHeight;

    width = mSrcWidth - srcbox->x1;
    height = mSrcHeight - srcbox->y1;



    if (opbox->width < width) {
        width = opbox->width;
    }

    if (opbox->height < height) {
        height = opbox->height;
    }

    mSrcClip.left = srcbox->x1;
    mSrcClip.top = srcbox->y1;
    mSrcClip.right = srcbox->x1 + width;
    mSrcClip.bottom = srcbox->y1 + height;

    /*setting the source surface*/
    if (!SetSourceSurface(galInfo)) {
        TRACE_ERROR("ERROR SETTING SOURCE SURFACE\n");
        TRACE_EXIT(FALSE);
    }
    /*Setting the dest surface*/
    if (!SetDestinationSurface(galInfo)) {
        TRACE_ERROR("ERROR SETTING DST SURFACE\n");
        TRACE_EXIT(FALSE);
    }
    /*setting the clipping for dest*/
    if (!SetClipping(galInfo)) {
        TRACE_ERROR("ERROR SETTING DST CLIPPING\n");
        TRACE_EXIT(FALSE);
    }


    while (!isFinished) {
        /**/
        mDstClip.left = stx;
        mDstClip.top = sty;
        mDstClip.right = stx + width;
        mDstClip.bottom = sty + height;
        /*Enabling the alpha blending*/
        if (!EnableAlphaBlending(galInfo)) {
            TRACE_ERROR("Alpha Blending Factor\n");
            TRACE_EXIT(FALSE);
        }
        status = gco2D_BatchBlit(
                gpuctx->mDriver->m2DEngine,
                1,
                &mSrcClip,
                &mDstClip,
                0xCC,
                0xCC,
                galInfo->mBlitInfo.mDstSurfInfo.mFormat.mVivFmt
                );

        if (status != gcvSTATUS_OK) {
            TRACE_ERROR("Copy Blit Failed");
            TRACE_EXIT(FALSE);
        }
        if (!DisableAlphaBlending(galInfo)) {
            TRACE_ERROR("Disabling Alpha Blend Failed\n");
            TRACE_EXIT(FALSE);
        }

        stx += width;
        if (stx >= dstbox->x1 + opbox->width) {
            stx = dstbox->x1;
            sty += height;
            if (sty >= dstbox->y1 + opbox->height) {
                isFinished = TRUE;
            }
        }

        if (stx == dstbox->x1) {
            mSrcClip.left = srcbox->x1;
            mSrcClip.top = 0;
            width = ((dstbox->x1 + opbox->width) - stx) > (mSrcWidth - srcbox->x1)
                    ? (mSrcWidth - srcbox->x1) : ((dstbox->x1 + opbox->width) - stx);
            height = ((dstbox->y1 + opbox->height) - sty) > mSrcHeight
                    ? mSrcHeight : ((dstbox->y1 + opbox->height) - sty);
            mSrcClip.right = mSrcClip.left + width;
            mSrcClip.bottom = mSrcClip.top + height;
        } else if (sty == opbox->y1) {
            mSrcClip.left = 0;
            mSrcClip.top = srcbox->y1;
            width = ((dstbox->x1 + opbox->width) - stx) > mSrcWidth
                    ? mSrcWidth : ((dstbox->x1 + opbox->width) - stx);
            height = ((dstbox->y1 + opbox->height) - sty) > (mSrcHeight -
                    srcbox->y1) ? (mSrcHeight - srcbox->y1) : ((dstbox->y1 + opbox->height) - sty);
            mSrcClip.right = mSrcClip.left + width;
            mSrcClip.bottom = mSrcClip.top + height;
        } else {
            mSrcClip.left = 0;
            mSrcClip.top = 0;
            width = ((dstbox->x1 + opbox->width) - stx) > mSrcWidth
                    ? mSrcWidth : ((dstbox->x1 + opbox->width) - stx);
            height = ((dstbox->y1 + opbox->height) - sty) > mSrcHeight
                    ? mSrcHeight : ((dstbox->y1 + opbox->height) - sty);
            mSrcClip.right = mSrcClip.left + width;
            mSrcClip.bottom = mSrcClip.top + height;
        }
    }


    TRACE_EXIT(TRUE);
}

/**
 * Checks for the operation box is out of bounds
 * @param opbox
 * @param dstbox
 * @return
 */
static Bool isOutOfBounds(VivBoxPtr opbox, VivBoxPtr dstbox) {
    TRACE_ENTER();
    opbox->x1 += opbox->width;
    if (opbox->x1 >= dstbox->x1 + dstbox->width) {
        opbox->x1 = dstbox->x1;
        opbox->y1 += opbox->height;
        if (opbox->y1 >= dstbox->y1 + dstbox->height) {
            TRACE_EXIT(TRUE);
        }
    }
    TRACE_EXIT(FALSE);
}

static Bool BlendMaskedConstPatternRect(GALINFOPTR galInfo, VivBoxPtr opbox) {
    TRACE_ENTER();
    TRACE_EXIT(TRUE);
}

static Bool BlendMaskedArbitraryPatternRect(GALINFOPTR galInfo, VivBoxPtr opbox) {
    TRACE_ENTER();
    TRACE_EXIT(TRUE);
}

static Bool BlendMaskedSimpleRect(GALINFOPTR galInfo, VivBoxPtr opbox) {
    TRACE_ENTER();
    gcsRECT streamRect;
    gcsPOINT streamSize;
    int srcSize = 0;
    int i;
    gceSTATUS status = gcvSTATUS_OK;

#define TEST 1
#if TEST
    gctUINT8_PTR monodata = gcvNULL;
#else
    gctUINT32_PTR monodata = gcvNULL;
#endif
    gceSURF_MONOPACK srcType = gcvSURF_UNPACKED;
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);

    GenericSurfacePtr dstsurf = (GenericSurfacePtr) (pBlt->mDstSurfInfo.mPriv->mVidMemInfo);
    GenericSurfacePtr srcsurf = (GenericSurfacePtr) (pBlt->mSrcSurfInfo.mPriv->mVidMemInfo);
    GenericSurfacePtr msksurf = (GenericSurfacePtr) (pBlt->mMskSurfInfo.mPriv->mVidMemInfo);

#if TEST
    gcsRECT mSrcRect = {pBlt->mSrcBox.x1, pBlt->mSrcBox.y1, pBlt->mSrcBox.x1 + opbox->width, pBlt->mSrcBox.y1 + opbox->height};
    gcsRECT mDstRect = {pBlt->mDstBox.x1, pBlt->mDstBox.y1, pBlt->mDstBox.x1 + opbox->width, pBlt->mDstBox.y1 + opbox->height};
#else
    gcsRECT mSrcRect = {0, 0, 0, 0};
    gcsRECT mDstRect = {0, 0, 320, 200};
#endif
    status = gco2D_SetMaskedSource(gpuctx->mDriver->m2DEngine,
            srcsurf->mVideoNode.mPhysicalAddr,
            srcsurf->mStride,
            pBlt->mSrcSurfInfo.mFormat.mVivFmt,
            gcvFALSE,
            srcType);

    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Error on setting the masked source\n");
        TRACE_EXIT(FALSE);
    }
    status = gco2D_SetSource(gpuctx->mDriver->m2DEngine, &mSrcRect);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Error on setting the rect for source\n");
        TRACE_EXIT(FALSE);
    }

    /*setting the destination*/
    status = gco2D_SetTarget(gpuctx->mDriver->m2DEngine, dstsurf->mVideoNode.mPhysicalAddr, dstsurf->mStride, gcvFALSE, dstsurf->mAlignedWidth);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Set Target Failed\n");
        TRACE_EXIT(FALSE);
    }
    /*setting the clipping for dest*/
    status = gco2D_SetClipping(gpuctx-> mDriver->m2DEngine, &mDstRect);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("gco2D_SetClipping failed\n");
        TRACE_EXIT(FALSE);
    }
    /*Enabling the alpha blending*/
    if (!EnableAlphaBlending(galInfo)) {
        TRACE_ERROR("Alpha Blending Factor\n");
        TRACE_EXIT(FALSE);
    }

#if TEST

    /*rectangle*/
    streamRect.left = pBlt->mMskBox.x1;
    streamRect.top = pBlt->mMskBox.y1;
    streamRect.right = pBlt->mMskBox.x1 + opbox->width;
    streamRect.bottom = pBlt->mMskBox.y1 + opbox->height;

    /*size*/
    streamSize.x = msksurf->mAlignedWidth;
    streamSize.y = msksurf->mAlignedHeight;
#else

    /*rectangle*/
    streamRect.left = 0;
    streamRect.top = 0;
    streamRect.right = 320;
    streamRect.bottom = 200;

    /*size*/
    streamSize.x = 320;
    streamSize.y = 200;
#endif





#if TEST


#else
    srcSize = 320 * 200 >> 5;
    monodata = (gctUINT32*) malloc(srcSize * sizeof (gctUINT32));
    for (i = 0; i < srcSize; i++) {
        *(monodata + i) = CONVERT_BYTE(i);
    }

    status = gco2D_MonoBlit(gpuctx->mDriver->m2DEngine, monodata,
            &streamSize, &streamRect, srcType, srcType, &mDstRect, 0xCC, 0x33, pBlt->mDstSurfInfo.mFormat.mVivFmt);
#endif

    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Error on mono blit\n");
        TRACE_EXIT(FALSE);
    }



    if (!DisableAlphaBlending(galInfo)) {
        TRACE_ERROR("Error on disabling alpha\n");
        TRACE_EXIT(FALSE);
    }



    TRACE_EXIT(TRUE);
}

/**
 * 1x1 source blending
 * @param galInfo
 * @param opbox
 * @return
 */
static Bool BlendConstPatternRect(GALINFOPTR galInfo, VivBoxPtr opbox) {
    TRACE_ENTER();
    if (!composite_stretch_blit_onebyonepixel(galInfo, opbox)) {
        TRACE_ERROR("1x1 no mask composite failed\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

/**
 * Blends for an arbitrary size of rectangle using small rectangles
 *
 * @param galInfo
 * @param opbox
 * @return
 */
static Bool BlendArbitraryPatternRect(GALINFOPTR galInfo, VivBoxPtr opbox) {
    TRACE_ENTER();
    int op = galInfo->mBlitInfo.mBlendOp.mOp;
    Bool isFinished = FALSE;
    Bool isOverOrSrc = (op == PictOpOver || op == PictOpSrc);
    int mSrcWidth = galInfo->mBlitInfo.mSrcSurfInfo.mWidth;
    int mSrcHeight = galInfo->mBlitInfo.mSrcSurfInfo.mHeight;

    if (isOverOrSrc) {
        opbox->width = galInfo->mBlitInfo.mDstBox.width;
        opbox->height = galInfo->mBlitInfo.mDstBox.height;
        if (!composite_one_pass_special(galInfo, opbox)) {

            TRACE_ERROR("VIVCOMPOSITE_SIMPLE - one_pass_special FAILED\n");
            TRACE_EXIT(FALSE);
        }
    } else {
        if (mSrcWidth < opbox->width) {
            opbox->width = mSrcWidth;
        }
        if (mSrcHeight < opbox->height) {
            opbox->height = mSrcHeight;
        }
        while (!isFinished) {
            if (!composite_one_pass(galInfo, opbox)) {

                TRACE_ERROR("VIVCOMPOSITE_SIMPLE - NON MULTIPASS FAILED\n");
                TRACE_EXIT(FALSE);
            }
            isFinished = isOutOfBounds(opbox, &galInfo->mBlitInfo.mDstBox);
        }
    }


    TRACE_EXIT(TRUE);
}

/**
 * Alpha blends for a simple rectangle (SRC OP DST)
 * @param galInfo - GAL Layer
 * @param opbox - operation rect
 * @return  true if success
 */
static Bool BlendSimpleRect(GALINFOPTR galInfo, VivBoxPtr opbox) {
    TRACE_ENTER();
    int op = galInfo->mBlitInfo.mBlendOp.mOp;
    Bool isOverOrSrc = (op == PictOpOver || op == PictOpSrc);
    int mSrcWidth = galInfo->mBlitInfo.mSrcSurfInfo.mWidth;
    int mSrcHeight = galInfo->mBlitInfo.mSrcSurfInfo.mHeight;
    int srcX = galInfo->mBlitInfo.mSrcBox.x1;
    int srcY = galInfo->mBlitInfo.mSrcBox.y1;
    /*Fitting the box*/
    if (isOverOrSrc) {
        if (((srcX >= 0 && srcY >= mSrcHeight) ||
                (srcY >= 0 && srcX >= mSrcWidth))) {
            if (op == PictOpOver) {
                TRACE_EXIT(TRUE); /*Nothing to do*/
            } else {
                /*It is a Clear Op Equivalent*/
                GetBlendingFactors(PictOpClear, &galInfo->mBlitInfo.mBlendOp);
                opbox->width = galInfo->mBlitInfo.mDstBox.width;
                opbox->height = galInfo->mBlitInfo.mDstBox.height;
            }
        } else if (srcX >= 0 && srcY >= 0) {
            if (mSrcWidth - srcX < opbox->width) {
                opbox->width = mSrcWidth - srcX;
            }
            if (mSrcHeight - srcY < opbox->height) {
                opbox->height = mSrcHeight - srcY;
            }
        } else if (srcX < 0 || srcY < 0) {
            /*@TODO : have a look at this*/
            TRACE_ERROR("Doesn't make sense\n");
            TRACE_EXIT(FALSE);
        } else {
            goto other;
        }
    } else {
other:
        if (mSrcWidth < opbox->width) {
            opbox->width = mSrcWidth;
        }
        if (mSrcHeight < opbox->height) {
            opbox->height = mSrcHeight;
        }
    }
    /*End of fitting the box*/

    /*Alpha Blend*/
    if (!composite_one_pass(galInfo, opbox)) {

        TRACE_ERROR("VIVCOMPOSITE_SIMPLE - NON MULTIPASS FAILED\n");
        TRACE_EXIT(FALSE);
    }
    /*End of Alpha Blend*/


    TRACE_EXIT(TRUE);
}

/**
 *
 * @param galInfo - GAL layer
 * @param opbox - operation box
 * @return true if success
 */
Bool DoCompositeBlit(GALINFOPTR galInfo, VivBoxPtr opbox) {
    TRACE_ENTER();
    Bool ret = TRUE;
    /* Perform rectangle render based on setup in PrepareComposite */
    switch (galInfo->mBlitInfo.mOperationCode) {
        case VIVCOMPOSITE_MASKED_SRC_REPEAT_PIXEL_ONLY_PATTERN:
            ret = BlendMaskedConstPatternRect(galInfo, opbox);
            break;
        case VIVCOMPOSITE_MASKED_SRC_REPEAT_ARBITRARY_SIZE_PATTERN:
            ret = BlendMaskedArbitraryPatternRect(galInfo, opbox);
            break;
        case VIVCOMPOSITE_MASKED_SIMPLE:
            ret = BlendMaskedSimpleRect(galInfo, opbox);
            break;
        case VIVCOMPOSITE_SRC_REPEAT_PIXEL_ONLY_PATTERN:
            ret = BlendConstPatternRect(galInfo, opbox);
            break;
        case VIVCOMPOSITE_SRC_REPEAT_ARBITRARY_SIZE_PATTERN:
            ret = BlendArbitraryPatternRect(galInfo, opbox);
            break;
        case VIVCOMPOSITE_SIMPLE:
            ret = BlendSimpleRect(galInfo, opbox);
            break;
        default:
            ret = FALSE;

            break;
    }
    TRACE_EXIT(ret);
}

/************************************************************************
 * EXA RELATED UTILITY (START)
 ************************************************************************/
Bool GetBlendingFactors(int op, VivBlendOpPtr vivBlendOp) {
    TRACE_ENTER();
    int i;
    Bool isFound = FALSE;
    for (i = 0; i < ARRAY_SIZE(blendingOps) && !isFound; i++) {
        if (blendingOps[i].mOp == op) {

            *vivBlendOp = blendingOps[i];
            isFound = TRUE;
        }
    }
    TRACE_EXIT(isFound);
}

/************************************************************************
 * EXA RELATED UTILITY (START)
 ************************************************************************/

Bool GetVivPictureFormat(int exa_fmt, VivPictFmtPtr viv) {
    TRACE_ENTER();
    int i;
    Bool isFound = FALSE;
    int size = ARRAY_SIZE(vivpict_formats);

    for (i = 0; i < size && !isFound; i++) {
        if (exa_fmt == vivpict_formats[i].mExaFmt) {
            *viv = (vivpict_formats[i]);
            isFound = TRUE;
        }
    }
    /*May be somehow usable*/
    if (!isFound) {

        *viv = vivpict_formats[size - 1];
        viv->mExaFmt = exa_fmt;
    }
    TRACE_EXIT(isFound);
}

Bool GetDefaultFormat(int bpp, VivPictFmtPtr format) {
    TRACE_ENTER();
    VivPictFormat r5g5b5 = {
        PICT_r5g6b5, 16, gcvSURF_R5G6B5, 0
    };
    VivPictFormat r8g8b8 = {
        PICT_r8g8b8, 24, gcvSURF_R8G8B8, 0
    };

    VivPictFormat a8r8g8b8 = {
        PICT_a8r8g8b8, 32, gcvSURF_A8R8G8B8, 8
    };

    VivPictFormat no_format = {
        NO_PICT_FORMAT, 0, gcvSURF_UNKNOWN, 0
    };

    DEBUGP("BPP = %d\n", bpp);
    switch (bpp) {
        case 16:
            *format = r5g5b5;
            break;
        case 24:
            *format = r8g8b8;
            break;
        case 32:
            *format = a8r8g8b8;
            break;
        default:
            *format = no_format;
            TRACE_EXIT(FALSE);
            break;
    }
    TRACE_EXIT(TRUE);
}
/************************************************************************
 *EXA RELATED UTILITY (END)
 ************************************************************************/
