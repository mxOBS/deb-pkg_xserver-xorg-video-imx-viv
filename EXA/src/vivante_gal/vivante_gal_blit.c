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


#include "vivante_common.h"
#include "vivante_gal.h"
#include "vivante_priv.h"
#include "vivante.h"
extern int IsSourceAlphaRequired(int op);
extern int IsDestAlphaRequired(int op);
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
        PICT_a8, 8, gcvSURF_A8, 8
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
    gctUINT32 mask;
    mask = galInfo->mBlitInfo.mPlaneMask;

    status = gco2D_LoadSolidBrush
            (
            gpuctx->mDriver->m2DEngine,
            (gceSURF_FORMAT) galInfo->mBlitInfo.mDstSurfInfo.mFormat.mVivFmt,
            (gctBOOL) galInfo->mBlitInfo.mColorConvert,
            galInfo->mBlitInfo.mColorARGB32 & mask,
            (gctUINT64)(0xFFFFFFFFFFFFFFFF)
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
    gceSURF_BLEND_FACTOR_MODE srcfactor;
    gceSURF_BLEND_FACTOR_MODE dstfactor;
    VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);

    srcfactor = (gceSURF_BLEND_FACTOR_MODE)galInfo->mBlitInfo.mBlendOp.mSrcBlendingFactor;
    dstfactor = (gceSURF_BLEND_FACTOR_MODE)galInfo->mBlitInfo.mBlendOp.mDstBlendingFactor;

     if (!pBlt->mSrcSurfInfo.alpha)
            dstfactor = gcvSURF_BLEND_ONE;

     if (!pBlt->mDstSurfInfo.alpha)
            srcfactor = gcvSURF_BLEND_ONE;


     status = gco2D_EnableAlphaBlendAdvanced
     (
     gpuctx->mDriver->m2DEngine,
     gcvSURF_PIXEL_ALPHA_STRAIGHT, gcvSURF_PIXEL_ALPHA_STRAIGHT,
     gcvSURF_GLOBAL_ALPHA_OFF, gcvSURF_GLOBAL_ALPHA_OFF,
     srcfactor,
     dstfactor
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
    VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);
    VivBoxPtr srcbox = &galInfo->mBlitInfo.mSrcBox;
    VivBoxPtr dstbox = &galInfo->mBlitInfo.mDstBox;
    VivBoxPtr osrcbox = &galInfo->mBlitInfo.mOSrcBox;
    VivBoxPtr odstbox = &galInfo->mBlitInfo.mODstBox;

    gcsRECT mSrcClip = {srcbox->x1, srcbox->y1, srcbox->x2, srcbox->y2};
    gcsRECT mDstClip = {dstbox->x1, dstbox->y1, dstbox->x2, dstbox->y2};

    gcsRECT mOSrcClip = {osrcbox->x1, osrcbox->y1, osrcbox->x2, osrcbox->y2};
    gcsRECT mODstClip = {odstbox->x1, odstbox->y1, odstbox->x2, odstbox->y2};

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

    /* No stretching */
    if ( pBlt->mIsNotStretched )
    {
        status = gco2D_BatchBlit(
        gpuctx->mDriver->m2DEngine,
        1,
        &mSrcClip,
        &mDstClip,
        0xCC,
        0xAA,
        galInfo->mBlitInfo.mDstSurfInfo.mFormat.mVivFmt
        );
    }
    else {

        status = gco2D_SetStretchRectFactors(gpuctx->mDriver->m2DEngine,&mOSrcClip,&mODstClip);

        status = gco2D_SetSource(gpuctx->mDriver->m2DEngine, &mSrcClip);

        if (status != gcvSTATUS_OK) {
            TRACE_ERROR("gco2D_SetSource failed ");
            TRACE_EXIT(FALSE);
        }

        status = gco2D_StretchBlit(gpuctx->mDriver->m2DEngine, 1, &mDstClip, 0xCC, 0xCC, galInfo->mBlitInfo.mDstSurfInfo.mFormat.mVivFmt);

        if (status != gcvSTATUS_OK) {
            TRACE_ERROR("gco2D_StretchBlit failed ");
            TRACE_EXIT(FALSE);
        }
    }

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
    gcsRECT dstRect;

    /*setting the source surface*/
    dstRect.left = galInfo->mBlitInfo.mDstBox.x1;
    dstRect.top = galInfo->mBlitInfo.mDstBox.y1;
    dstRect.right = galInfo->mBlitInfo.mDstBox.x2;
    dstRect.bottom = galInfo->mBlitInfo.mDstBox.y2;

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
    status = gco2D_SetClipping(gpuctx->mDriver->m2DEngine, &dstRect);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("gco2D_SetClipping failed\n");
        TRACE_EXIT(FALSE);
    }

    status = gco2D_SetStretchRectFactors(gpuctx->mDriver->m2DEngine,&srcRect,&dstRect);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("gco2D_SetStretchRectFactors failed\n");
        TRACE_EXIT(FALSE);
    }
    /*Enabling the alpha blending*/
    if (!EnableAlphaBlending(galInfo)) {
        TRACE_ERROR("Alpha Blending Factor\n");
        TRACE_EXIT(FALSE);
    }
    status = gco2D_StretchBlit(gpuctx->mDriver->m2DEngine, 1, &dstRect, 0xCC, 0xCC, galInfo->mBlitInfo.mDstSurfInfo.mFormat.mVivFmt);
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
 * Check if the current transform is supported
 * Only support 90,180,270 rotation
 * @param ptransform
  * @param stretchflag
 * return TRUE if supported , otherwise FALSE
 */
Bool VIVTransformSupported(PictTransform *ptransform,Bool *stretchflag)
{
    *stretchflag = FALSE;
    return FALSE;
}

gceSURF_ROTATION VIVGetRotation(PictTransform *ptransform)
{
    return gcvSURF_0_DEGREE;
}

void VIVGetSourceWH(PictTransform *ptransform, gctUINT32 deswidth, gctUINT32 desheight, gctUINT32 *srcwidth, gctUINT32 *srcheight )
{
    *srcwidth = deswidth;
    *srcheight = desheight;
    return ;
}

static void _rectIntersect(gcsRECT_PTR pdst, gcsRECT_PTR psrc1, gcsRECT_PTR psrc2)
{
    if ((psrc1->right < psrc2->left)
    || (psrc1->left > psrc2->right)
    || (psrc1->top > psrc2->bottom)
    || (psrc1->bottom < psrc2->top) )
    {
        pdst->left = 0;
        pdst->top = 0;
        pdst->right = 0;
        pdst->bottom = 0;
        return;
    }

    pdst->left = V_MAX(psrc1->left, psrc2->left);

    pdst->right = V_MIN(psrc1->right, psrc2->right);

    pdst->top = V_MAX(psrc1->top, psrc2->top);

    pdst->bottom = V_MIN(psrc1->bottom, psrc2->bottom);

}

static void _rectAdjustByWH(gcsRECT_PTR pdst,gcsRECT_PTR psrc, gctUINT32 width, gctUINT32 height)
{

    pdst->left = psrc->left % width;
    if ( (psrc->right % width) == 0 )
        pdst->right = width;
    else
        pdst->right = psrc->right % width;

    pdst->top = psrc->top % height;

    if ( ( psrc->bottom % height ) == 0 )
        pdst->bottom = height;
    else
        pdst->bottom = psrc->bottom % height;

}

static void _srcRect(VIV2DBLITINFOPTR pBlt, gcsRECT_PTR psrcrect)
{
        gctINT32 tempINT;
        if ( pBlt->mSrcBox.x1 < 0)
        {
            tempINT = 0 - pBlt->mSrcBox.x1;
            tempINT = gcmALIGN(tempINT, pBlt->mSrcSurfInfo.mWidth);
            //psrcrect->left = pBlt->mSrcBox.x1 + pBlt->mSrcSurfInfo.mWidth*tempINT - 1;

            psrcrect->left = pBlt->mSrcBox.x1 + tempINT - 1;
        }
        else
        psrcrect->left = pBlt->mSrcBox.x1;

        psrcrect->left %= pBlt->mSrcSurfInfo.mWidth;
        if ( pBlt->mSrcBox.y1 < 0 )
        {
            tempINT = 0 - pBlt->mSrcBox.y1;
            tempINT = gcmALIGN(tempINT, pBlt->mSrcSurfInfo.mHeight);
        //  psrcrect->top = pBlt->mSrcBox.y1 + pBlt->mSrcSurfInfo.mHeight*tempINT - 1;

            psrcrect->top = pBlt->mSrcBox.y1 + tempINT - 1;
        }
        else
        psrcrect->top = pBlt->mSrcBox.y1;

        psrcrect->top %= pBlt->mSrcSurfInfo.mHeight;
        if ( ( psrcrect->left + pBlt->mSrcBox.width ) > pBlt->mSrcTempSurfInfo.mWidth )
            psrcrect->right = pBlt->mSrcTempSurfInfo.mWidth;
        else
            psrcrect->right = psrcrect->left + pBlt->mSrcBox.width;

        if ( ( psrcrect->top + pBlt->mSrcBox.height ) > pBlt->mSrcTempSurfInfo.mHeight )
            psrcrect->bottom = pBlt->mSrcTempSurfInfo.mHeight;
        else
            psrcrect->bottom = psrcrect->top + pBlt->mSrcBox.height;
}

#if defined(ENABLE_MASK_OP)
#define TRACK_MASK_OP 0
static void _rectPrint(VIV2DBLITINFOPTR pBlt, char *msg, gctUINT32 xNum, gctUINT32 yNum, gcsRECT_PTR    pSrcClip, gcsRECT_PTR   pDstClip)
{
#if TRACK_MASK_OP
    static int index = 0;

    GenericSurfacePtr genericSurf;
    int i, j;
     xf86DrvMsg(0,X_INFO,"Track Num %d\n",index++);

    xf86DrvMsg(0,X_INFO,"---------rectInfo---------\n");
    xf86DrvMsg(0,X_INFO,"Src:x1=%d,y1=%d,x2=%d,y2=%d\n",pBlt->mSrcBox.x1,pBlt->mSrcBox.y1,pBlt->mSrcBox.x2,pBlt->mSrcBox.y2);
    xf86DrvMsg(0,X_INFO,"Msk:x1=%d,y1=%d,x2=%d,y2=%d\n",pBlt->mMskBox.x1,pBlt->mMskBox.y1,pBlt->mMskBox.x2,pBlt->mMskBox.y2);
    xf86DrvMsg(0,X_INFO,"Dst:x1=%d,y1=%d,x2=%d,y2=%d\n",pBlt->mDstBox.x1,pBlt->mDstBox.y1,pBlt->mDstBox.x2,pBlt->mDstBox.y2);
    if (pBlt->mSrcTempSurfInfo.mPriv)
    {
        genericSurf = (GenericSurfacePtr)pBlt->mSrcTempSurfInfo.mPriv->mVidMemInfo;
        xf86DrvMsg(0,X_INFO,"SrcSurf:mWidth=%d,mHeight=%d\n",pBlt->mSrcSurfInfo.mWidth,pBlt->mSrcSurfInfo.mHeight);
        xf86DrvMsg(0,X_INFO,"TempSrcSurf:mWidth=%d,mHeight=%d,AlignedWidth=%d,AlignedHeight=%d\n",pBlt->mSrcTempSurfInfo.mWidth,pBlt->mSrcTempSurfInfo.mHeight,genericSurf->mAlignedWidth,genericSurf->mAlignedHeight);
    }

        xf86DrvMsg(0,X_INFO,"MskSurf:mWidth=%d,mHeight=%d\n",pBlt->mMskSurfInfo.mWidth,pBlt->mMskSurfInfo.mHeight);
    if (pBlt->mMskTempSurfInfo.mPriv)
    {
        genericSurf = (GenericSurfacePtr)pBlt->mMskTempSurfInfo.mPriv->mVidMemInfo;
        xf86DrvMsg(0,X_INFO,"MskSurf:mWidth=%d,mHeight=%d\n",pBlt->mMskSurfInfo.mWidth,pBlt->mMskSurfInfo.mHeight);
        xf86DrvMsg(0,X_INFO,"TempMskSurf:mWidth=%d,mHeight=%d,AlignedWidth=%d,AlignedHeight=%d\n",pBlt->mMskTempSurfInfo.mWidth,pBlt->mMskTempSurfInfo.mHeight,genericSurf->mAlignedWidth,genericSurf->mAlignedHeight);

    }

    if (msg) {
        xf86DrvMsg(0,X_INFO,"rectInfo:cliprectMsg=%s,xNum=%d,yNum=%d\n",msg,xNum,yNum);
        for (j = 0; j < yNum; j ++)
        {
            for(i = 0; i < xNum; i++)
            {
                xf86DrvMsg(0,X_INFO,"SrcClip:x1=%d,y1=%d,x2=%d,y2=%d\n",pSrcClip[i].left,pSrcClip[i].top,pSrcClip[i].right,pSrcClip[i].bottom);
                xf86DrvMsg(0,X_INFO,"DstClip:x1=%d,y1=%d,x2=%d,y2=%d\n",pDstClip[i].left,pDstClip[i].top,pDstClip[i].right,pDstClip[i].bottom);
            }
            pSrcClip += xNum;
            pDstClip += xNum;
        }
    }
#endif
}

static gctINT32 _xLEN(gctUINT32 i, gctUINT32 num, gctUINT32 width, gcsRECT_PTR ref)
{

    if ( num == 1)
        return ref->right - ref->left;

    if ( i == 0 )
        return width - ref->left % width;

    if ( i == (num - 1) )
    {
        if ( ( ref->right % width ) == 0 )
        return width;

        return ref ->right % width;
    }

    return width;
}

static gctINT32 _yLEN(gctUINT32 j, gctUINT32 num, gctUINT32 height, gcsRECT_PTR ref)
{

    if ( num == 1)
        return ref->bottom- ref->top;

    if ( j == 0 )
        return height - ref->top % height;

    if ( j == (num - 1) )
    {
        if ( ( ref->bottom% height ) == 0 )
        return height;

        return ref ->bottom% height;
    }

    return height;
}

static void _mskRect(VIV2DBLITINFOPTR pBlt, gcsRECT_PTR pmskrect) {
        gctINT32 tempINT;

        if ( pBlt->mMskBox.x1 < 0)
        {
            tempINT = 0 - pBlt->mMskBox.x1;
            tempINT = gcmALIGN(tempINT, pBlt->mMskSurfInfo.mWidth);
            //pmskrect->left = pBlt->mMskBox.x1 + pBlt->mMskSurfInfo.mWidth * tempINT - 1;

            pmskrect->left = pBlt->mMskBox.x1 + tempINT - 1;
        }
        else
        pmskrect->left = pBlt->mMskBox.x1;
        pmskrect->left %= pBlt->mMskSurfInfo.mWidth;
        if ( pBlt->mMskBox.y1 < 0 )
        {
            tempINT = 0 - pBlt->mMskBox.y1;
            tempINT = gcmALIGN(tempINT, pBlt->mMskSurfInfo.mHeight);
            //pmskrect->top = pBlt->mMskBox.y1 + pBlt->mMskSurfInfo.mHeight* tempINT - 1;

            pmskrect->top = pBlt->mMskBox.y1 + tempINT - 1;
        }
        else
        pmskrect->top = pBlt->mMskBox.y1;
        pmskrect->top %= pBlt->mMskSurfInfo.mHeight;
        if ( ( pmskrect->left + pBlt->mMskBox.width ) > pBlt->mMskTempSurfInfo.mWidth )
            pmskrect->right = pBlt->mMskTempSurfInfo.mWidth;
        else
            pmskrect->right = pmskrect->left + pBlt->mMskBox.width;
        if ( ( pmskrect->top + pBlt->mMskBox.height ) > pBlt->mMskTempSurfInfo.mHeight )
            pmskrect->bottom = pBlt->mMskTempSurfInfo.mHeight;
        else
            pmskrect->bottom = pmskrect->top + pBlt->mMskBox.height;

}
static void _vivFormatPrint(unsigned int format, char *msg)
{
    switch (format)
    {
    case gcvSURF_A8R8G8B8:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_A8R8G8B8\n",msg);
        break;
    case gcvSURF_X8R8G8B8:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_X8R8G8B8\n",msg);
        break;
    case gcvSURF_A8B8G8R8:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_A8B8G8R8\n",msg);
        break;
    case gcvSURF_X8B8G8R8:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_X8B8G8R8\n",msg);
        break;
    case gcvSURF_B8G8R8A8:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_B8G8R8A8\n",msg);
        break;
    case gcvSURF_B8G8R8X8:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_B8G8R8X8\n",msg);
        break;
    case gcvSURF_R8G8B8:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_R8G8B8\n",msg);
        break;
    case gcvSURF_B8G8R8:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_B8G8R8\n",msg);
        break;
    case gcvSURF_R5G6B5:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_R5G6B5\n",msg);
        break;
    case gcvSURF_A1R5G5B5:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_A1R5G5B5\n",msg);
        break;
    case gcvSURF_X1R5G5B5:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_X1R5G5B5\n",msg);
        break;
    case gcvSURF_A1B5G5R5:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_A1B5G5R5\n",msg);
        break;
    case gcvSURF_X1B5G5R5:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_X1B5G5R5\n",msg);
        break;
    case gcvSURF_A4R4G4B4:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_A4R4G4B4\n",msg);
        break;
    case gcvSURF_X4R4G4B4:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_X4R4G4B4\n",msg);
        break;
    case gcvSURF_A4B4G4R4:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_A4B4G4R4\n",msg);
        break;
    case gcvSURF_X4B4G4R4:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_X4B4G4R4\n",msg);
        break;
    case gcvSURF_A8:
        xf86DrvMsg(0,X_INFO,"%s=gcvSURF_A8\n");
        break;
    default : ;
    }

}

static void _formatPrint(VIV2DBLITINFOPTR pBlt)
{

    xf86DrvMsg(0,X_INFO,"BlendMasked op=%d\n",pBlt->mBlendOp.mOp);

    _vivFormatPrint(pBlt->mSrcSurfInfo.mFormat.mVivFmt,"SrcFomat");

    _vivFormatPrint(pBlt->mMskSurfInfo.mFormat.mVivFmt,"MskFomat");

    _vivFormatPrint(pBlt->mDstSurfInfo.mFormat.mVivFmt,"DstFomat");

}


static Bool BlendMaskedArbitraryPatternRect(GALINFOPTR galInfo, VivBoxPtr opbox) {
    TRACE_ENTER();
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);
    GenericSurfacePtr dstsurf;
    GenericSurfacePtr srcsurf;
    GenericSurfacePtr msksurf;

    gcsRECT_PTR pSrcClip = NULL;
    gcsRECT_PTR pDstClip = NULL;
    gcsRECT_PTR pOSrcClip = NULL;
    gcsRECT_PTR pODstClip = NULL;
    gcsRECT_PTR pXTempClip = NULL;
    gcsRECT_PTR pYTempClip = NULL;
    gctUINT32 tempXNum = 0;
    gctUINT32 tempYNum = 0;


    gceSTATUS status = gcvSTATUS_OK;

    gcsRECT mskrect;
    gcsRECT srcrect;
    gcsRECT dstrect;

    gctUINT32 minW = 0;
    gctUINT32 minH = 0;

    gctUINT32 i = 0;
    gctUINT32 j = 0;
    gctINT32 slen = 0;
    gctINT32 leftclips = 0;
    gctINT32 curNum = 0;

    srcsurf = (GenericSurfacePtr)pBlt->mSrcTempSurfInfo.mPriv->mVidMemInfo;
    msksurf = (GenericSurfacePtr)pBlt->mMskSurfInfo.mPriv->mVidMemInfo;

#if TRACK_MASK_OP
    _formatPrint(pBlt);
#endif

    status = gco2D_SetGenericSource
    (
        gpuctx->mDriver->m2DEngine,
        &msksurf->mVideoNode.mPhysicalAddr,
        1,
        &msksurf->mStride,
        1,
        msksurf->mTiling,
        pBlt->mMskSurfInfo.mFormat.mVivFmt,
        pBlt->mRotation,
        msksurf->mAlignedWidth,
        msksurf->mAlignedHeight
    );

    if ( status != gcvSTATUS_OK ) {
        TRACE_ERROR("ERROR SETTING SOURCE SURFACE\n");
        TRACE_EXIT(FALSE);
    }

    status = gco2D_SetGenericTarget
    (
        gpuctx->mDriver->m2DEngine,
        &srcsurf->mVideoNode.mPhysicalAddr,
        1,
        &srcsurf->mStride,
        1,
        srcsurf->mTiling,
        pBlt->mSrcTempSurfInfo.mFormat.mVivFmt,
        gcvSURF_0_DEGREE,
        srcsurf->mAlignedWidth,
        srcsurf->mAlignedHeight
    );

    dstrect.top = 0;
    dstrect.left = 0;

    if ( pBlt->mSrcTempSurfInfo.repeat )
    {
        _srcRect(pBlt, &srcrect);
        dstrect.right = pBlt->mSrcTempSurfInfo.mWidth;
        dstrect.bottom = pBlt->mSrcTempSurfInfo.mHeight;

    } else {

        srcrect.left = pBlt->mSrcBox.x1;
        srcrect.right = pBlt->mSrcBox.x2;
        srcrect.top = pBlt->mSrcBox.y1;
        srcrect.bottom = pBlt->mSrcBox.y2;

        if ( srcrect.left < 0 )
        srcrect.left = 0;

        if ( srcrect.top < 0 )
        srcrect.top = 0;

        if ( srcrect.right > pBlt->mSrcSurfInfo.mWidth )
            srcrect.right = pBlt->mSrcSurfInfo.mWidth;

        if ( srcrect.bottom > pBlt->mSrcSurfInfo.mHeight )
            srcrect.bottom = pBlt->mSrcSurfInfo.mHeight;

        dstrect.right = pBlt->mSrcSurfInfo.mWidth;
        dstrect.bottom = pBlt->mSrcSurfInfo.mHeight;
    }

    if ( pBlt->mMskTempSurfInfo.repeat )
    {
        _mskRect(pBlt, &mskrect);

    } else {

        mskrect.left = pBlt->mMskBox.x1;
        mskrect.right = pBlt->mMskBox.x2;
        mskrect.top = pBlt->mMskBox.y1;
        mskrect.bottom = pBlt->mMskBox.y2;

        if ( mskrect.left < 0 )
        mskrect.left = 0;

        if ( mskrect.top < 0 )
        mskrect.top = 0;

        if ( mskrect.right > pBlt->mMskSurfInfo.mWidth )
            mskrect.right = pBlt->mMskSurfInfo.mWidth;

        if ( mskrect.bottom > pBlt->mMskSurfInfo.mHeight )
            mskrect.bottom = pBlt->mMskSurfInfo.mHeight;


    }

    status = gco2D_SetClipping(gpuctx->mDriver->m2DEngine, &dstrect);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("gco2D_SetClipping failed\n");
        TRACE_EXIT(FALSE);
    }

    /* Cal Min Box */

    minW = V_MIN((mskrect.right - mskrect.left), (srcrect.right - srcrect.left));
    minH = V_MIN((mskrect.bottom - mskrect.top), (srcrect.bottom - srcrect.top));

    mskrect.right = mskrect.left + minW;
    srcrect.right = srcrect.left + minW;
    mskrect.bottom = mskrect.top + minH;
    srcrect.bottom = srcrect.top + minH;

    if (mskrect.right % pBlt->mMskSurfInfo.mWidth)
        tempXNum = ( mskrect.right ) / pBlt->mMskSurfInfo.mWidth - mskrect.left / pBlt->mMskSurfInfo.mWidth + 1;
    else
        tempXNum = ( mskrect.right ) / pBlt->mMskSurfInfo.mWidth - mskrect.left / pBlt->mMskSurfInfo.mWidth;

    if (mskrect.bottom % pBlt->mMskSurfInfo.mHeight)
        tempYNum = ( mskrect.bottom ) / pBlt->mMskSurfInfo.mHeight - mskrect.top / pBlt->mMskSurfInfo.mHeight + 1;
    else
        tempYNum = ( mskrect.bottom ) / pBlt->mMskSurfInfo.mHeight - mskrect.top / pBlt->mMskSurfInfo.mHeight;

    pSrcClip = (gcsRECT *)malloc(sizeof(gcsRECT) * tempXNum * tempYNum);
    pDstClip = (gcsRECT *)malloc(sizeof(gcsRECT) * tempXNum * tempYNum);

    pOSrcClip = pSrcClip;
    pODstClip = pDstClip;

    for ( j = 0 ; j < tempYNum ; j++)
    {
        if ( j == 0 ) {
        pOSrcClip[0].top = mskrect.top % pBlt->mMskSurfInfo.mHeight;
        pOSrcClip[0].bottom = pOSrcClip[0].top + _yLEN(0, tempYNum, pBlt->mMskSurfInfo.mHeight, &mskrect);
        } else {
        pXTempClip = pOSrcClip - tempXNum;
        pOSrcClip[0].top = pXTempClip[0].bottom % pBlt->mMskSurfInfo.mHeight;
        pOSrcClip[0].bottom = pOSrcClip[0].top+ _yLEN(j, tempYNum, pBlt->mMskSurfInfo.mHeight, &mskrect);
        }

        pOSrcClip[0].left = mskrect.left % pBlt->mMskSurfInfo.mWidth;
        pOSrcClip[0].right = pOSrcClip[0].left + _xLEN(0, tempXNum, pBlt->mMskSurfInfo.mWidth, &mskrect);

        pODstClip[0].left = srcrect.left;
        if ( j == 0) {
        pODstClip[0].top = srcrect.top;
        } else {
        pYTempClip = pODstClip - tempXNum;
        pODstClip[0].top = pYTempClip[0].bottom;
        }

        pODstClip[0].right = pODstClip[0].left + pOSrcClip[0].right - pOSrcClip[0].left;
        pODstClip[0].bottom = pODstClip[0].top + pOSrcClip[0].bottom - pOSrcClip[0].top;

        for(i = 1; i < tempXNum; i++)
        {
            pOSrcClip[i].left = pOSrcClip[i-1].right % pBlt->mMskSurfInfo.mWidth;
            pOSrcClip[i].right = pOSrcClip[i].left + _xLEN(i, tempXNum,pBlt->mMskSurfInfo.mWidth, &mskrect);
            pOSrcClip[i].top = pOSrcClip[0].top;
            pOSrcClip[i].bottom = pOSrcClip[0].bottom;

            pODstClip[i].left = pODstClip[i-1].right;
            pODstClip[i].right = pODstClip[i].left + pOSrcClip[i].right - pOSrcClip[i].left;

            pODstClip[i].top = pODstClip[0].top;
            pODstClip[i].bottom = pODstClip[0].bottom;
        }

        pOSrcClip += tempXNum;
        pODstClip += tempXNum;

    }

    _rectPrint(pBlt, "Msk to tempSrc",tempXNum,tempYNum,pSrcClip,pDstClip);
    status = gco2D_SetPorterDuffBlending(gpuctx->mDriver->m2DEngine, gcvPD_DST_IN);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("gco2D_SetPorterDuffBlending failed\n");
        TRACE_EXIT(FALSE);
    }

    if ( (tempYNum*tempXNum) >= (1920))
    {
            slen = (1920)/tempXNum;
            pOSrcClip = pSrcClip;
            pODstClip = pDstClip;
            if (slen >= 1)
            {
                leftclips = tempYNum * tempXNum;

                while(leftclips){

                if (leftclips > (slen*tempXNum))
                curNum = slen*tempXNum;
                else
                curNum = leftclips;
                status = gco2D_BatchBlit(
                gpuctx->mDriver->m2DEngine,
                curNum,
                pOSrcClip,
                pODstClip,
                0xCC,
                0xCC,
                galInfo->mBlitInfo.mSrcTempSurfInfo.mFormat.mVivFmt);
                pOSrcClip += curNum;
                pODstClip += curNum;
                leftclips -= curNum;

                }

            } else {
                fprintf(stderr,"Too many rects in one line \n");
            }

    } else {

        status = gco2D_BatchBlit(
            gpuctx->mDriver->m2DEngine,
            tempYNum * tempXNum,
            pSrcClip,
            pDstClip,
            0xCC,
            0xCC,
            galInfo->mBlitInfo.mSrcTempSurfInfo.mFormat.mVivFmt
        );
    }


    free(pSrcClip);
    free(pDstClip);

    /* Need commit(true) ? */
    /* ???????????????????? */

    status = gco2D_SetGenericSource
    (
        gpuctx->mDriver->m2DEngine,
        &srcsurf->mVideoNode.mPhysicalAddr,
        1,
        &srcsurf->mStride,
        1,
        srcsurf->mTiling,
        pBlt->mSrcSurfInfo.mFormat.mVivFmt,
        pBlt->mRotation,
        srcsurf->mAlignedWidth,
        srcsurf->mAlignedHeight
    );

    if ( status != gcvSTATUS_OK ) {
        TRACE_ERROR("ERROR SETTING SOURCE SURFACE\n");
        TRACE_EXIT(FALSE);
    }

    dstsurf = (GenericSurfacePtr)pBlt->mDstSurfInfo.mPriv->mVidMemInfo;

    status = gco2D_SetGenericTarget
    (
        gpuctx->mDriver->m2DEngine,
        &dstsurf->mVideoNode.mPhysicalAddr,
        1,
        &dstsurf->mStride,
        1,
        dstsurf->mTiling,
        pBlt->mDstSurfInfo.mFormat.mVivFmt,
        gcvSURF_0_DEGREE,
        dstsurf->mAlignedWidth,
        dstsurf->mAlignedHeight
    );

    dstrect.left = 0;
    dstrect.top = 0;
    dstrect.right = pBlt->mDstSurfInfo.mWidth;
    dstrect.bottom = pBlt->mDstSurfInfo.mHeight;

    status = gco2D_SetClipping(gpuctx->mDriver->m2DEngine, &dstrect);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("gco2D_SetClipping failed\n");
        TRACE_EXIT(FALSE);
    }

    dstrect.left = pBlt->mDstBox.x1;
    if ( dstrect.left < 0 )
        dstrect.left = 0;
    dstrect.top = pBlt->mDstBox.y1;
    if ( dstrect.top < 0 )
        dstrect.top = 0;

    dstrect.right = pBlt->mDstBox.x2;
    if ( dstrect.right > pBlt->mDstSurfInfo.mWidth)
        dstrect.right = pBlt->mDstSurfInfo.mWidth;

    dstrect.bottom = pBlt->mDstBox.y2;
    if ( dstrect.bottom > pBlt->mDstSurfInfo.mHeight )
        dstrect.bottom = pBlt->mDstSurfInfo.mHeight;

    minW = V_MIN((dstrect.right - dstrect.left), (srcrect.right - srcrect.left));
    minH = V_MIN((dstrect.bottom - dstrect.top), (srcrect.bottom - srcrect.top));



    srcrect.right = srcrect.left + minW;
    dstrect.right = dstrect.left + minW;
    srcrect.bottom = srcrect.top + minH;
    dstrect.bottom = dstrect.top + minH;

    if (!DisableAlphaBlending(galInfo)) {
        TRACE_ERROR("Error on disabling alpha\n");
        TRACE_EXIT(FALSE);
    }

    pBlt->mSrcSurfInfo.alpha = 1;

    if (!EnableAlphaBlending(galInfo)) {
            TRACE_ERROR("Alpha Blending Factor\n");
            TRACE_EXIT(FALSE);
    }

    status = gco2D_BatchBlit(
        gpuctx->mDriver->m2DEngine,
        1,
        &srcrect,
        &dstrect,
        0xCC,
        0xAA,
        pBlt->mDstSurfInfo.mFormat.mVivFmt
    );
    pBlt->mSrcSurfInfo.alpha = 0;
    if (!DisableAlphaBlending(galInfo)) {
        TRACE_ERROR("Error on disabling alpha\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}
#endif
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

static Bool BlendArbitraryPatternRect(GALINFOPTR galInfo, VivBoxPtr opbox) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);
    VivBoxPtr dstbox = &galInfo->mBlitInfo.mDstBox;

    gcsRECT_PTR pSrcClip = NULL;
    gcsRECT_PTR pDstClip = NULL;


    gcsRECT_PTR pOSrcClip = NULL;
    gcsRECT_PTR pODstClip = NULL;

    gctUINT32 xclipCount = 0;
    gctUINT32 yclipCount = 0;
    gcsRECT srcRect;
    gctUINT32 i = 0, j = 0;
    gctINT32 xoff = 0;
    gctINT32 yoff = 0;
    gctUINT32 wx = 0;
    gctUINT32 hy = 0;

    xclipCount = pBlt->mSrcTempSurfInfo.mWidth / pBlt->mSrcSurfInfo.mWidth;
    yclipCount = pBlt->mSrcTempSurfInfo.mHeight/ pBlt->mSrcSurfInfo.mHeight;

    pSrcClip = malloc(sizeof(gcsRECT) * xclipCount * yclipCount);
    pDstClip = malloc(sizeof(gcsRECT) * xclipCount * yclipCount);

    pOSrcClip = pSrcClip;
    pODstClip = pDstClip;

    _srcRect(pBlt, &srcRect);

    for(j = 0; j < yclipCount ; j++)
    {
        for(i = 0; i < xclipCount; i++)
        {
                pOSrcClip[i].left = 0;
                pOSrcClip[i].top = 0;
                pOSrcClip[i].right = pBlt->mSrcSurfInfo.mWidth;
                pOSrcClip[i].bottom = pBlt->mSrcSurfInfo.mHeight;

                pODstClip[i].left = i*pBlt->mSrcSurfInfo.mWidth;
                pODstClip[i].top = j*pBlt->mSrcSurfInfo.mHeight;
                pODstClip[i].right = (i + 1)*pBlt->mSrcSurfInfo.mWidth;
                pODstClip[i].bottom = (j+1)*pBlt->mSrcSurfInfo.mHeight;
                if ( j == 0 || j == (yclipCount -1)
                || i == 0 || i == (xclipCount -1) )
                {
                        _rectIntersect(&pODstClip[i], &pODstClip[i], &srcRect);
                        if ( j == 0 && i== 0 )
                        {
                            xoff = dstbox->x1 - pODstClip[0].left;
                            yoff = dstbox->y1 - pODstClip[0].top;
                        }
                        _rectAdjustByWH(&pOSrcClip[i], &pODstClip[i], pBlt->mSrcSurfInfo.mWidth, pBlt->mSrcSurfInfo.mHeight);
                }
                wx = pODstClip[i].right - pODstClip[i].left;
                hy = pODstClip[i].bottom - pODstClip[i].top;
                pODstClip[i].left += xoff;
                pODstClip[i].right = pODstClip[i].left + wx;
                pODstClip[i].top += yoff;
                pODstClip[i].bottom = pODstClip[i].top + hy;
        }
        pOSrcClip += xclipCount;
        pODstClip += xclipCount;
    }

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
    xclipCount*yclipCount,
    pSrcClip,
    pDstClip,
    0xCC,
    0xAA,
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


    free((void *)pSrcClip);
    free((void *)pDstClip);

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

    if (!composite_one_pass(galInfo, opbox)) {
        TRACE_ERROR("VIVCOMPOSITE_SIMPLE - NON MULTIPASS FAILED\n");
        TRACE_EXIT(FALSE);
    }

    TRACE_EXIT(TRUE);
}

void VIVSWComposite(PixmapPtr pxDst, int srcX, int srcY, int maskX, int maskY,
    int dstX, int dstY, int width, int height) {
    pixman_image_t *srcimage;
    pixman_image_t *dstimage;
    Viv2DPixmapPtr psrc;
    Viv2DPixmapPtr pdst;
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pxDst);
    VIV2DBLITINFOPTR pBlt = &pViv->mGrCtx.mBlitInfo;
    GenericSurfacePtr srcsurf = (GenericSurfacePtr) (pBlt->mSrcSurfInfo.mPriv->mVidMemInfo);
    GenericSurfacePtr dstsurf = (GenericSurfacePtr) (pBlt->mDstSurfInfo.mPriv->mVidMemInfo);

    psrc = (Viv2DPixmapPtr)pBlt->mSrcSurfInfo.mPriv;
    pdst = (Viv2DPixmapPtr)pBlt->mDstSurfInfo.mPriv;

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
    psrc->mCpuBusy = TRUE;
    pdst->mCpuBusy = TRUE;
}
#if defined(ENABLE_MASK_OP)
static void SetTempSurfForRT(GALINFOPTR galInfo, VivBoxPtr opbox)
{
    GenericSurfacePtr srcsurf;
    gceSTATUS status = gcvSTATUS_OK;
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);


    Bool retvsurf;
    gctUINT32 physicaladdr;
    gctPOINTER linearaddr;
    gctUINT32 aligned_height;
    gctUINT32 aligned_width;
    gctUINT32 aligned_pitch;

    GenericSurfacePtr pinfo;
    gcsRECT_PTR pSrcClip = NULL;
    gcsRECT_PTR pDstClip = NULL;


    gcsRECT_PTR pOSrcClip = NULL;
    gcsRECT_PTR pODstClip = NULL;

    gctUINT32 xclipCount = 0;
    gctUINT32 yclipCount = 0;

    gcsRECT dstRect = {0, 0, 0, 0};
    gctUINT32 maxsize;
    gctUINT32 i = 0, j = 0;
    gctUINT32 mask = 0;
    gcsRECT srcRect;


    mask = (galInfo->mBlitInfo.mOperationCode == VIVCOMPOSITE_MASKED_SRC_REPEAT_PIXEL_ONLY_PATTERN) || (galInfo->mBlitInfo.mOperationCode == VIVCOMPOSITE_MASKED_SRC_REPEAT_ARBITRARY_SIZE_PATTERN) || (galInfo->mBlitInfo.mOperationCode == VIVCOMPOSITE_MASKED_SIMPLE);

    if ( pBlt->mSrcTempSurfInfo.repeat || mask)
    {
        srcsurf = (GenericSurfacePtr)(pBlt->mSrcSurfInfo.mPriv->mVidMemInfo);
        maxsize = V_MAX(pBlt->mSrcTempSurfInfo.mWidth, pBlt->mSrcTempSurfInfo.mHeight);

        pBlt->mSrcTempSurfInfo.mPriv = (Viv2DPixmapPtr)malloc(sizeof(Viv2DPixmap));
        pBlt->mSrcTempSurfInfo.mPriv->mVidMemInfo = (GenericSurfacePtr)malloc(sizeof(GenericSurface));
        pinfo = pBlt->mSrcTempSurfInfo.mPriv->mVidMemInfo;

        VSetSurfIndex(1);
        switch ( pBlt->mSrcTempSurfInfo.mFormat.mBpp )
        {
            case 8:
            case 16:
                retvsurf = VGetSurfAddrBy16(galInfo, maxsize, (int *) (&physicaladdr), (int *) (&linearaddr),(int *)&aligned_width, (int *)&aligned_height, (int *)&aligned_pitch);
                break;
            case 24:
            case 32:
                retvsurf = VGetSurfAddrBy32(galInfo, maxsize, (int *) (&physicaladdr), (int *) (&linearaddr), (int *)&aligned_width, (int *)&aligned_height, (int *)&aligned_pitch);
                break;
            default:
                return ;
        }

        if ( retvsurf == FALSE ) return ;

        status = gco2D_SetGenericSource
        (
        gpuctx->mDriver->m2DEngine,
        &srcsurf->mVideoNode.mPhysicalAddr,
        1,
        &srcsurf->mStride,
        1,
        srcsurf->mTiling,
        pBlt->mSrcSurfInfo.mFormat.mVivFmt,
        pBlt->mRotation,
        srcsurf->mAlignedWidth,
        srcsurf->mAlignedHeight
        );

        if ( status != gcvSTATUS_OK ) {
            TRACE_ERROR("ERROR SETTING SOURCE SURFACE\n");
            TRACE_EXIT();
        }


        status = gco2D_SetGenericTarget
        (
        gpuctx->mDriver->m2DEngine,
        &physicaladdr,
        1,
        &aligned_pitch,
        1,
        srcsurf->mTiling,
        pBlt->mSrcTempSurfInfo.mFormat.mVivFmt,
        gcvSURF_0_DEGREE,
        aligned_width,
        aligned_height
        );

        if ( status != gcvSTATUS_OK ) {
            TRACE_ERROR("ERROR SETTING SOURCE SURFACE\n");
            TRACE_EXIT();
        }

        dstRect.right = maxsize;
        dstRect.bottom = maxsize;

        status = gco2D_SetClipping(gpuctx->mDriver->m2DEngine, &dstRect);
        if (status != gcvSTATUS_OK) {
            TRACE_ERROR("gco2D_SetClipping failed\n");
            TRACE_EXIT();
        }

        if (galInfo->mBlitInfo.mOperationCode == VIVCOMPOSITE_MASKED_SRC_REPEAT_PIXEL_ONLY_PATTERN )
        {
            dstRect.top = 0;
            dstRect.left = 0;
            dstRect.bottom = pBlt->mSrcTempSurfInfo.mHeight;
            dstRect.right = pBlt->mSrcTempSurfInfo.mWidth;

            switch (pBlt->mSrcSurfInfo.mFormat.mBpp)
            {
                case 32:
                    status = gco2D_Clear(
                    gpuctx->mDriver->m2DEngine,
                    1,
                    &dstRect,
((pBlt->mSrcSurfInfo.mFormat.mAlphaBits) ? *((gctUINT32 *)(srcsurf->mLogicalAddr)) : (*((gctUINT32 *)(srcsurf->mLogicalAddr)) | 0xFF000000)),
                    0xCC,
                    0xFF,
                    pBlt->mSrcTempSurfInfo.mFormat.mVivFmt
                    );
                    break;
                case 24:
                    status = gco2D_Clear(
                    gpuctx->mDriver->m2DEngine,
                    1,
                    &dstRect,
                    (*((gctUINT32 *)(srcsurf->mLogicalAddr)) & 0x00FFFFFF) |0xFF000000,
                    0xCC,
                    0xAA,
                    pBlt->mSrcTempSurfInfo.mFormat.mVivFmt
                    );
                    break;
                    default:;
            }
        } else {
            xclipCount = pBlt->mSrcTempSurfInfo.mWidth / pBlt->mSrcSurfInfo.mWidth;
            yclipCount = pBlt->mSrcTempSurfInfo.mHeight/ pBlt->mSrcSurfInfo.mHeight;

            pSrcClip = malloc(sizeof(gcsRECT) * xclipCount * yclipCount);
            pDstClip = malloc(sizeof(gcsRECT) * xclipCount * yclipCount);

            pOSrcClip = pSrcClip;
            pODstClip = pDstClip;

            _srcRect(pBlt, &srcRect);

            for(j = 0; j < yclipCount ; j++)
            {
                for(i = 0; i < xclipCount; i++)
                {
                    pOSrcClip[i].left = 0;
                    pOSrcClip[i].top = 0;
                    pOSrcClip[i].right = pBlt->mSrcSurfInfo.mWidth;
                    pOSrcClip[i].bottom = pBlt->mSrcSurfInfo.mHeight;

                    pODstClip[i].left = i*pBlt->mSrcSurfInfo.mWidth;
                    pODstClip[i].top = j*pBlt->mSrcSurfInfo.mHeight;
                    pODstClip[i].right = (i + 1)*pBlt->mSrcSurfInfo.mWidth;
                    pODstClip[i].bottom = (j+1)*pBlt->mSrcSurfInfo.mHeight;
                    if ( j == 0 || j == (yclipCount -1)
                    || i == 0 || i == (xclipCount -1) )
                    {
                        _rectIntersect(&pODstClip[i], &pODstClip[i], &srcRect);
                        _rectAdjustByWH(&pOSrcClip[i], &pODstClip[i], pBlt->mSrcSurfInfo.mWidth, pBlt->mSrcSurfInfo.mHeight);
                    }
                }
                pOSrcClip += xclipCount;
                pODstClip += xclipCount;
            }

            _rectPrint(pBlt, "Src to TempSrc",xclipCount,yclipCount,pSrcClip,pDstClip);
            status = gco2D_BatchBlit(
            gpuctx->mDriver->m2DEngine,
            xclipCount*yclipCount,
            pSrcClip,
            pDstClip,
            0xCC,
            0xAA,
            pBlt->mSrcTempSurfInfo.mFormat.mVivFmt
            );

            free(pSrcClip);
            free(pDstClip);
        }

        pinfo->mAlignedWidth = aligned_width;
        pinfo->mAlignedHeight = aligned_height;
        pinfo->mLogicalAddr = (gctPOINTER)linearaddr;
        pinfo->mStride = aligned_pitch;
        pinfo->mVideoNode.mPhysicalAddr = physicaladdr;
        pinfo->mRotation = gcvSURF_0_DEGREE;
        pinfo->mTiling = srcsurf->mTiling;
    }

}

static void ReleaseTempSurfForRT(GALINFOPTR galInfo, VivBoxPtr opbox)
{
    VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);

    if ( pBlt->mSrcTempSurfInfo.mPriv )
    {
        free(pBlt->mSrcTempSurfInfo.mPriv->mVidMemInfo);
        free(pBlt->mSrcTempSurfInfo.mPriv);
    }
}
#endif
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
    #if defined(ENABLE_MASK_OP)
        case VIVCOMPOSITE_MASKED_SRC_REPEAT_PIXEL_ONLY_PATTERN:
        case VIVCOMPOSITE_MASKED_SIMPLE:
        case VIVCOMPOSITE_MASKED_SRC_REPEAT_ARBITRARY_SIZE_PATTERN:
            /* if REPEAT MODE happens, set temp surf */
            SetTempSurfForRT(galInfo,opbox);

            ret = BlendMaskedArbitraryPatternRect(galInfo, opbox);

            ReleaseTempSurfForRT(galInfo,opbox);

            break;
    #endif
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
            xf86DrvMsg(0, X_INFO,"VivComposite can't handle mask and do nothing in %s\n",__FUNCTION__);
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

    /* Disable op= PIXMAN_OP_SRC, currently hw path can't defeat sw path */
    if ( op == PIXMAN_OP_SRC )
        TRACE_EXIT(FALSE);

    for (i = 0; i < ARRAY_SIZE(blendingOps) && !isFound; i++) {
        if (blendingOps[i].mOp == op) {

            *vivBlendOp = blendingOps[i];
            isFound = TRUE;
        }
    }
    TRACE_EXIT(isFound);
}
extern gctBOOL CHIP_SUPPORTA8;
/************************************************************************
 * EXA RELATED UTILITY (START)
 ************************************************************************/

Bool GetVivPictureFormat(int exa_fmt, VivPictFmtPtr viv) {
    TRACE_ENTER();
    int i;
    Bool isFound = FALSE;
    int size = ARRAY_SIZE(vivpict_formats);

    if ( PICT_a8 == exa_fmt && CHIP_SUPPORTA8 == gcvFALSE)
        goto ENDF;

    for (i = 0; i < size && !isFound; i++) {
        if (exa_fmt == vivpict_formats[i].mExaFmt) {
            *viv = (vivpict_formats[i]);
            isFound = TRUE;
        }
    }

ENDF:
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

    VivPictFormat a8 = {
        PICT_a8, 8, gcvSURF_A8, 8
    };
    VivPictFormat no_format = {
        NO_PICT_FORMAT, 0, gcvSURF_UNKNOWN, 0
    };

    DEBUGP("BPP = %d\n", bpp);
    switch (bpp) {
        case 8:
            *format = a8;
            break;
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
