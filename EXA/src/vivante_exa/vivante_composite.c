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
#include "../vivante_gal/vivante_priv.h"
#include "vivante.h"

#define ITF(x)    IntToxFixed(x)
#define FTI(x)    xFixedToInt(x)
extern Bool GetBlendingFactors(int op, VivBlendOpPtr vivBlendOp);
static
Bool IsPixelOnly(DrawablePtr pDrawable) {
    if ((1 == pDrawable->width) && (1 == pDrawable->height)) {

        return TRUE;

    } else {

        return FALSE;
    }
}

int IsSourceAlphaRequired(int op) {
    return (((
            (1 << PictOpOver) |
            (1 << PictOpInReverse) |
            (1 << PictOpOutReverse) |
            (1 << PictOpAtop) |
            (1 << PictOpAtopReverse) |
            (1 << PictOpXor) |
            0) >> (op)) & 1);
}

int IsDestAlphaRequired(int op) {
    return (((
            (1 << PictOpOverReverse) |
            (1 << PictOpIn) |
            (1 << PictOpOut) |
            (1 << PictOpAtop) |
            (1 << PictOpAtopReverse) |
            (1 << PictOpXor) |
            0) >> (op)) & 1);
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
VivCheckComposite(int op, PicturePtr pSrc, PicturePtr pMsk, PicturePtr pDst) {
    TRACE_ENTER();
    VivPtr pViv;
    VIV2DBLITINFOPTR pBlt;
    PixmapPtr pxSrc = GetDrawablePixmap(pSrc->pDrawable);
    PixmapPtr pxDst = GetDrawablePixmap(pDst->pDrawable);


    Bool stretchflag = FALSE;

    pViv = VIVPTR_FROM_PIXMAP(pxDst);
    pBlt = &pViv->mGrCtx.mBlitInfo;

    /*Nothing*/
    if (pxDst == NULL) {
        TRACE_EXIT(FALSE);
    }

    if (pxSrc == NULL) {
        TRACE_EXIT(FALSE);
    }

    if ( (pxDst->drawable.width * pxDst->drawable.height) < IMX_EXA_MIN_PIXEL_AREA_COMPOSITE )
        TRACE_EXIT(FALSE);

    /*Not supported op*/
    if (!GetBlendingFactors(op, &pBlt->mBlendOp)) {
        TRACE_EXIT(FALSE);
    }

    /*Format Checks*/
    if (!GetVivPictureFormat(pDst->format, &pBlt->mDstSurfInfo.mFormat)) {
        TRACE_ERROR("Dest Format is not supported\n");
        TRACE_EXIT(FALSE);
    }

    if (!GetVivPictureFormat(pSrc->format, &pBlt->mSrcSurfInfo.mFormat)) {
        TRACE_ERROR("Src Format is not supported\n");
        TRACE_EXIT(FALSE);
    }

    if (pMsk)
    {
        if (!GetVivPictureFormat(pMsk->format, &pBlt->mMskSurfInfo.mFormat)) {
            TRACE_ERROR("Msk Format is not supported\n");
            TRACE_EXIT(FALSE);
        }

    }

    if ( pBlt->mDstSurfInfo.mFormat.mBpp < 8 || pBlt->mSrcSurfInfo.mFormat.mBpp < 8 )   {
        TRACE_ERROR("Src or Dst pixel is not wide enough\n");
        TRACE_EXIT(FALSE);
    }

    /*For forward compatibility*/
    if (op > PictOpSaturate) {
        TRACE_EXIT(FALSE);
    }

    /*No Gradient*/
    if (pSrc->pSourcePict) {
        TRACE_EXIT(FALSE);
    }

    if (pMsk && pMsk->transform) {
        TRACE_EXIT(FALSE);
    }

    if (pMsk && pMsk->componentAlpha) {
        TRACE_EXIT(FALSE);
    }

#if !defined(ENABLE_MASK_OP)
    if (pMsk && pMsk->repeat) {
        TRACE_EXIT(FALSE);
    }
#endif

    /*Have some ideas about this but now let it be*/
    if (pSrc->transform && VIVTransformSupported(pSrc->transform , &stretchflag )==FALSE ) {
        TRACE_EXIT(FALSE);
    }

    if ( pMsk && stretchflag ) {
        TRACE_EXIT(FALSE);
    }

    pBlt->mIsNotStretched = !stretchflag;

    /* The same as what prepare does */
    if ( (0 == PICT_FORMAT_A(pSrc->format) ) && IsSourceAlphaRequired(op)) {
        TRACE_EXIT(FALSE);
    }

    if ( (0 == PICT_FORMAT_A(pDst->format) ) && IsDestAlphaRequired(op)) {
        TRACE_EXIT(FALSE);
    }

    /*Mask Related Checks*/
    if ((NULL != pMsk)) {
    /* If mask is on, fallback to sw path currently */
#if !defined(ENABLE_MASK_OP)
        TRACE_EXIT(FALSE);
#endif
    } else {
        if (pSrc->repeat) {
            /* Source is 1x1 (constant) repeat pattern? */
            if ( IsPixelOnly(pSrc->pDrawable) ) {
              TRACE_EXIT(TRUE);
            } else {/* Source is arbitrary sized repeat pattern? */
                if ( pSrc->transform )
                    TRACE_EXIT(FALSE);
            }
        } else {/* Simple source blend */
            if (pBlt->mIsNotStretched)
            {
                SURF_SIZE_FOR_SW(pxSrc->drawable.width, pxSrc->drawable.height);
                SURF_SIZE_FOR_SW(pxDst->drawable.width, pxDst->drawable.height);
            }

        }

    }
    TRACE_EXIT(TRUE);
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
VivPrepareComposite(int op, PicturePtr pSrc, PicturePtr pMsk,
    PicturePtr pDst, PixmapPtr pxSrc, PixmapPtr pxMsk, PixmapPtr pxDst) {

    TRACE_ENTER();
    Bool ret = TRUE;
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pxDst);
    VIV2DBLITINFOPTR pBlt = &pViv->mGrCtx.mBlitInfo;
    /*Private Pixmaps*/
    Viv2DPixmapPtr psrc = exaGetPixmapDriverPrivate(pxSrc);
    Viv2DPixmapPtr pdst = exaGetPixmapDriverPrivate(pxDst);
    Viv2DPixmapPtr pmsk = NULL;

    /*Not supported op*/
    if (!GetBlendingFactors(op, &pBlt->mBlendOp)) {
        TRACE_EXIT(FALSE);
    }

    /*Format Checks*/
    if (!GetVivPictureFormat(pDst->format, &pBlt->mDstSurfInfo.mFormat)) {
        TRACE_ERROR("Dest Format is not supported\n");
        TRACE_EXIT(FALSE);
    }

    if (!GetVivPictureFormat(pSrc->format, &pBlt->mSrcSurfInfo.mFormat)) {
        TRACE_ERROR("Src Format is not supported\n");
        TRACE_EXIT(FALSE);
    }


    /* Checkcomposite should check if the transform is supported , otherwise it shouldn't be here */
    if (pSrc->transform )
        pBlt->mRotation = VIVGetRotation(pSrc->transform);
    else
        pBlt->mRotation = gcvSURF_0_DEGREE;

    /*START-Populating the info for src and dst*/
    pBlt->mDstSurfInfo.mHeight = pxDst->drawable.height;
    pBlt->mDstSurfInfo.mWidth = pxDst->drawable.width;
    pBlt->mDstSurfInfo.mStride = pxDst->devKind;
    pBlt->mDstSurfInfo.mPriv = pdst;

    pBlt->mSrcSurfInfo.mHeight = pxSrc->drawable.height;
    pBlt->mSrcSurfInfo.mWidth = pxSrc->drawable.width;
    pBlt->mSrcSurfInfo.mStride = pxSrc->devKind;
    pBlt->mSrcSurfInfo.mPriv = psrc;
    pBlt->mSrcSurfInfo.repeat = pSrc->repeat;
    pBlt->mSrcSurfInfo.repeatType = pSrc->repeatType;
    /*END-Populating the info for src and dst*/
    pBlt->mTransform = pSrc->transform;

    if (pSrc->format != pDst->format)
        pBlt->mColorConvert = TRUE;
    else
        pBlt->mColorConvert = FALSE;

    pBlt->mMskSurfInfo.mPriv = ( Viv2DPixmapPtr )NULL;

    pBlt->mSrcSurfInfo.alpha = PICT_FORMAT_A(pSrc->format);
    pBlt->mDstSurfInfo.alpha = PICT_FORMAT_A(pDst->format);
    /* Mask blend */
    if (NULL != pMsk) {
        /*Populating the mask info*/
        pmsk = exaGetPixmapDriverPrivate(pxMsk);
        pBlt->mMskSurfInfo.mHeight = pxMsk->drawable.height;
        pBlt->mMskSurfInfo.mWidth = pxMsk->drawable.width;
        pBlt->mMskSurfInfo.mStride = pxMsk->devKind;
        pBlt->mMskSurfInfo.mPriv = pmsk;
        pBlt->mMskSurfInfo.mFormat.mBpp = pxMsk->drawable.bitsPerPixel;
        pBlt->mMskSurfInfo.repeat = pMsk->repeat;
        pBlt->mMskSurfInfo.repeatType = pMsk->repeatType;
        /* Blend repeating source using a mask */
        if (pSrc->repeat) {
            /* Source is 1x1 (constant) repeat pattern?*/
            if ( IsPixelOnly(pSrc->pDrawable) ) {
                pBlt->mOperationCode = VIVCOMPOSITE_MASKED_SRC_REPEAT_PIXEL_ONLY_PATTERN;
#if !defined(ENABLE_MASK_OP)
                ret = FALSE;
#else
                ret = TRUE;
#endif
            } else {/* Source is arbitrary sized repeat pattern? */
                pBlt->mOperationCode = VIVCOMPOSITE_MASKED_SRC_REPEAT_ARBITRARY_SIZE_PATTERN;
#if !defined(ENABLE_MASK_OP)
                ret = FALSE;
#else
                ret = TRUE;
#endif
            }
        } else {/* Simple (source IN mask) blend */
        pBlt->mOperationCode = VIVCOMPOSITE_MASKED_SIMPLE;
#if !defined(ENABLE_MASK_OP)
        ret = FALSE;
#else
        ret = TRUE;
#endif
        }
    } else {/* Source only (no mask) blend */
        /* Blend repeating source  */
        if (pSrc->repeat) {
            /* Source is 1x1 (constant) repeat pattern? */
            if ( IsPixelOnly(pSrc->pDrawable) ) {
                pBlt->mOperationCode = VIVCOMPOSITE_SRC_REPEAT_PIXEL_ONLY_PATTERN;
                ret = TRUE;
            } else {/* Source is arbitrary sized repeat pattern? */
                pBlt->mOperationCode = VIVCOMPOSITE_SRC_REPEAT_ARBITRARY_SIZE_PATTERN;
                ret = TRUE;
                if ( pSrc->transform )
                    ret = FALSE;
            }
        } else {/* Simple source blend */
            pBlt->mOperationCode = VIVCOMPOSITE_SIMPLE;
            ret = TRUE;
        }
    }

    TRACE_EXIT(ret);
}

static int loBase(int n, unsigned int base)
{
    if ( n >= 0 )
        return base *( n / base);

    return base *( ( n-base + 1) /base );
}

static int hiBase(int n, unsigned int base)
{
    if ( n >= 0 )
        return base * ( (n +base -1) / base);

    return base *( n / base);
}

static void initTempSurf(VIV2DBLITINFOPTR pBlt)
{
    pBlt->mSrcTempSurfInfo.mPriv = NULL;
    pBlt->mMskTempSurfInfo.mPriv = NULL;
    pBlt->mMskTempSurfInfo.repeat = 0;
    pBlt->mSrcTempSurfInfo.repeat = 0;
}


static void CalTempSurfInfo(VIV2DBLITINFOPTR pBlt, int srcX, int srcY, int width, int height)
{
    pBlt->mSrcTempSurfInfo.repeat = pBlt->mSrcSurfInfo.repeat;
    if (pBlt->mSrcSurfInfo.repeat)
    {
        pBlt->mSrcTempSurfInfo.mPriv = NULL;
        pBlt->mSrcTempSurfInfo.mWidth = hiBase( srcX + width, pBlt->mSrcSurfInfo.mWidth) - loBase(srcX, pBlt->mSrcSurfInfo.mWidth);
        pBlt->mSrcTempSurfInfo.mHeight = hiBase( srcY + height, pBlt->mSrcSurfInfo.mHeight) - loBase(srcY, pBlt->mSrcSurfInfo.mHeight);
        pBlt->mSrcTempSurfInfo.mFormat = pBlt->mSrcSurfInfo.mFormat;
    } else {
        pBlt->mSrcTempSurfInfo.mPriv = NULL;
        pBlt->mSrcTempSurfInfo.mWidth = pBlt->mSrcSurfInfo.mWidth;
        pBlt->mSrcTempSurfInfo.mHeight = pBlt->mSrcSurfInfo.mHeight;
        pBlt->mSrcTempSurfInfo.mFormat = pBlt->mSrcSurfInfo.mFormat;
    }

}

static void CalTempMaskInfo(VIV2DBLITINFOPTR pBlt, int maskX, int maskY, int width, int height)
{
    pBlt->mMskTempSurfInfo.repeat = pBlt->mMskSurfInfo.repeat;

    if (pBlt->mMskSurfInfo.repeat)
    {
        pBlt->mMskTempSurfInfo.mPriv = NULL;
        pBlt->mMskTempSurfInfo.mWidth = hiBase( maskX + width, pBlt->mMskSurfInfo.mWidth) - loBase(maskX, pBlt->mMskSurfInfo.mWidth);
        pBlt->mMskTempSurfInfo.mHeight = hiBase( maskY + height, pBlt->mMskSurfInfo.mHeight) - loBase(maskY, pBlt->mMskSurfInfo.mHeight);
        pBlt->mMskTempSurfInfo.mFormat = pBlt->mMskSurfInfo.mFormat;
    } else {
        pBlt->mMskTempSurfInfo.mPriv = NULL;
        pBlt->mMskTempSurfInfo.mWidth = pBlt->mMskSurfInfo.mWidth;
        pBlt->mMskTempSurfInfo.mHeight = pBlt->mMskSurfInfo.mHeight;
        pBlt->mMskTempSurfInfo.mFormat = pBlt->mMskSurfInfo.mFormat;
    }
}

static void
CalOrgBoxInfoWithoutMask(VIV2DBLITINFOPTR pBlt, int srcX, int srcY, int maskX, int maskY,
    int dstX, int dstY, int width, int height, VivBox *opBox)
{
    gctUINT32 srcwidth,srcheight;
    int src_x_t, src_y_t;
    int opWidth = width;
    int opHeight = height;

    srcwidth = opWidth;
    srcheight = opHeight;

    if ( pBlt->mTransform )
        VIVGetSourceWH(pBlt->mTransform,opWidth,opHeight,&srcwidth,&srcheight);

    /*Populating the info*/
    opBox->x1 = srcX;
    opBox->y1 = srcY;
    opBox->width = srcwidth;
    opBox->height = srcheight;

    /*Src & Dst & Msk*/
    pBlt->mDstBox.x1 = dstX;
    pBlt->mDstBox.y1 = dstY;
    pBlt->mDstBox.x2 = dstX + width;
    pBlt->mDstBox.y2 = dstY + height;

    pBlt->mDstBox.width = width;
    pBlt->mDstBox.height = height;


    if ( pBlt->mRotation == gcvSURF_90_DEGREE )
    {
        src_x_t = -srcY + pixman_fixed_to_int ( ((struct pixman_transform *)pBlt->mTransform)->matrix[0][2] +  pixman_fixed_1 / 2 - pixman_fixed_e) - height;
        src_y_t = srcX + pixman_fixed_to_int ( ((struct pixman_transform *)pBlt->mTransform)->matrix[1][2] +  pixman_fixed_1 / 2 - pixman_fixed_e);
        srcX = src_x_t;
        srcY = src_y_t;
        /* Fix me for the next lines */
        pBlt->mSrcSurfInfo.alpha = 1;
        pBlt->mDstSurfInfo.alpha = 1;
    }


    if ( pBlt->mRotation == gcvSURF_270_DEGREE )
    {
        src_x_t = srcY + pixman_fixed_to_int (((struct pixman_transform *)pBlt->mTransform)->matrix[0][2] + pixman_fixed_1 / 2 - pixman_fixed_e);
        src_y_t = -srcX + pixman_fixed_to_int (((struct pixman_transform *)pBlt->mTransform)->matrix[1][2] + pixman_fixed_1 / 2 - pixman_fixed_e) - width;
        srcX = src_x_t;
        srcY = src_y_t;
        /* Fix me for the next lines */
        pBlt->mSrcSurfInfo.alpha = 1;
        pBlt->mDstSurfInfo.alpha = 1;
    }

    if ( pBlt->mRotation == gcvSURF_180_DEGREE )
    {
        src_x_t = pBlt->mSrcSurfInfo.mWidth - width - srcX;
        src_y_t = pBlt->mSrcSurfInfo.mHeight - height - srcY;
        srcX = src_x_t;
        srcY = src_y_t;
        /* Fix me for the next lines */
        pBlt->mSrcSurfInfo.alpha = 1;
        pBlt->mDstSurfInfo.alpha = 1;
    }

    if ( pBlt->mRotation == gcvSURF_FLIP_X )
    {
        src_x_t = pBlt->mSrcSurfInfo.mWidth - width - srcX;
        srcX = src_x_t;
        /* Fix me for the next lines */
        pBlt->mSrcSurfInfo.alpha = 1;
        pBlt->mDstSurfInfo.alpha = 1;
    }

    if ( pBlt->mRotation == gcvSURF_FLIP_Y )
    {
        src_y_t = pBlt->mSrcSurfInfo.mHeight - height - srcY;
        srcY = src_y_t;
        /* Fix me for the next lines */
        pBlt->mSrcSurfInfo.alpha = 1;
        pBlt->mDstSurfInfo.alpha = 1;
    }


    opBox->x1 = srcX;
    opBox->y1 = srcY;

    pBlt->mSrcBox.x1 = srcX;
    pBlt->mSrcBox.y1 = srcY;
    pBlt->mSrcBox.x2 = srcX+srcwidth;
    pBlt->mSrcBox.y2 = srcY+srcheight;

    pBlt->mSrcBox.width = srcwidth;
    pBlt->mSrcBox.height = srcheight;
    CalTempSurfInfo(pBlt, srcX, srcY, width, height);

}

#if defined(ENABLE_MASK_OP)
static void
CalOrgBoxInfoWithMask(VIV2DBLITINFOPTR pBlt, int srcX, int srcY, int maskX, int maskY,
    int dstX, int dstY, int width, int height, VivBox *opBox)
{
    /*Populating the info*/
    opBox->x1 = srcX;
    opBox->y1 = srcY;
    opBox->width = width;
    opBox->height = height;

    /*Src & Dst & Msk*/
    pBlt->mDstBox.x1 = dstX;
    pBlt->mDstBox.y1 = dstY;
    pBlt->mDstBox.x2 = dstX + width;
    pBlt->mDstBox.y2 = dstY + height;

    pBlt->mSrcBox.x1 = srcX;
    pBlt->mSrcBox.y1 = srcY;
    pBlt->mSrcBox.x2 = srcX+width;
    pBlt->mSrcBox.y2 = srcY+height;

    pBlt->mSrcBox.width = width;
    pBlt->mSrcBox.height = height;


    pBlt->mMskBox.x1 = maskX;
    pBlt->mMskBox.y1 = maskY;
    pBlt->mMskBox.x2 = maskX + width;
    pBlt->mMskBox.y2 = maskY + height;

    pBlt->mMskBox.width = width;
    pBlt->mMskBox.height = height;

    CalTempSurfInfo(pBlt, srcX, srcY, width, height);
    CalTempMaskInfo(pBlt, maskX, maskY, width, height);
}
#endif

static Bool
IsNotStretched(VIV2DBLITINFOPTR pBlt, VivBoxPtr srcbox, VivBoxPtr dstbox)
{
    return pBlt->mIsNotStretched;
}

static void
ReCalBoxByStretchInfo(VIV2DBLITINFOPTR pBlt, VivBox *opBox) {
    VivBoxPtr srcbox = &(pBlt->mSrcBox);
    VivBoxPtr dstbox = &(pBlt->mDstBox);
    VivBoxPtr osrcbox = &(pBlt->mOSrcBox);
    VivBoxPtr odstbox = &(pBlt->mODstBox);
    int minwid = 0;
    int minheight = 0;
    float  xfactors = 0.0;
    float  yfactors = 0.0;
    Bool nstflag = TRUE;


    memcpy((void *)&(pBlt->mOSrcBox),(void *)srcbox,sizeof(VivBox));
    memcpy((void *)&(pBlt->mODstBox),(void *)dstbox,sizeof(VivBox));

    nstflag = IsNotStretched(pBlt, srcbox, dstbox);

    pBlt->mIsNotStretched = nstflag;

    if ( pBlt->mOperationCode == VIVCOMPOSITE_SRC_REPEAT_PIXEL_ONLY_PATTERN )
    {
        dstbox->x2 = V_MIN(dstbox->x2, pBlt->mDstSurfInfo.mWidth);
        dstbox->y2 = V_MIN(dstbox->y2, pBlt->mDstSurfInfo.mHeight);
        TRACE_EXIT();
    }

    if ( pBlt->mOperationCode == VIVCOMPOSITE_SRC_REPEAT_ARBITRARY_SIZE_PATTERN )
    {
        gcmASSERT(dstbox->x1 <= pBlt->mDstSurfInfo.mWidth);
        gcmASSERT(dstbox->y1 <= pBlt->mDstSurfInfo.mHeight);
        gcmASSERT(dstbox->x2 <= pBlt->mDstSurfInfo.mWidth);
        gcmASSERT(dstbox->y2 <= pBlt->mDstSurfInfo.mHeight);
        /* To enable it, Currently disable it */
        /* When enabling it, how to consider to do this when stretched ??? */
        dstbox->x2 = V_MIN(dstbox->x2, pBlt->mDstSurfInfo.mWidth);
        dstbox->y2 = V_MIN(dstbox->y2, pBlt->mDstSurfInfo.mHeight);
        TRACE_EXIT();
    }

    if ( pBlt->mOperationCode != VIVCOMPOSITE_SIMPLE )
        TRACE_EXIT();

    /* No stretching */
    if ( nstflag )
    {
        switch ( pBlt->mRotation ) {
            case gcvSURF_0_DEGREE:
            case gcvSURF_180_DEGREE:
            case gcvSURF_FLIP_X:
            case gcvSURF_FLIP_Y:
                dstbox->x2 = V_MIN(odstbox->x2, pBlt->mDstSurfInfo.mWidth);
                minwid = dstbox->x2 - dstbox->x1;

                srcbox->x2 = V_MIN(osrcbox->x2, pBlt->mSrcSurfInfo.mWidth);
                minwid = V_MIN(srcbox->x2 - srcbox->x1, minwid);

                dstbox->y2 = V_MIN(odstbox->y2, pBlt->mDstSurfInfo.mHeight);
                minheight = dstbox->y2 - dstbox->y1;

                srcbox->y2 = V_MIN(osrcbox->y2, pBlt->mSrcSurfInfo.mHeight);
                minheight = V_MIN(srcbox->y2 - srcbox->y1, minheight);

                srcbox->x2 = srcbox->x1 + minwid;
                srcbox->y2 = srcbox->y1 + minheight;

                dstbox->x2 = dstbox->x1 + minwid;
                dstbox->y2 = dstbox->y1 + minheight;
                break;
            case gcvSURF_90_DEGREE:
            case gcvSURF_270_DEGREE:
                dstbox->x2 = V_MIN(odstbox->x2, pBlt->mDstSurfInfo.mWidth);
                minwid = dstbox->x2 - dstbox->x1;

                srcbox->y2 = V_MIN(osrcbox->y2, pBlt->mSrcSurfInfo.mHeight);
                minheight = srcbox->y2 - srcbox->y1;

                minheight = V_MIN(minwid, minheight);

                srcbox->y2 = srcbox->y1 + minheight;
                dstbox->x2 = dstbox->x1 + minheight;

                dstbox->y2 = V_MIN(odstbox->y2, pBlt->mDstSurfInfo.mHeight);
                minheight = dstbox->y2 - dstbox->y1;

                srcbox->x2 = V_MIN(osrcbox->x2, pBlt->mSrcSurfInfo.mWidth);
                minwid = srcbox->x2 - srcbox->x1;

                minwid = V_MIN(minheight, minwid);

                srcbox->x2 = srcbox->x1 + minwid;
                dstbox->y2 = dstbox->y1 + minwid;

                break;
            default :
                break;
        }
    } else {

        if ( osrcbox->x2 > pBlt->mSrcSurfInfo.mWidth )
            srcbox->x2 = pBlt->mSrcSurfInfo.mWidth;

        if ( osrcbox->y2 > pBlt->mSrcSurfInfo.mHeight )
            srcbox->y2 = pBlt->mSrcSurfInfo.mHeight;

        switch ( pBlt->mRotation ) {
            case gcvSURF_0_DEGREE:
            case gcvSURF_180_DEGREE:
                xfactors = ( (float)( odstbox->x2 - odstbox->x1 ) ) / ( (float)( osrcbox->x2 - osrcbox->x1 ) );
                dstbox->x2 = dstbox->x1 + (int) ( xfactors * ( (float)( srcbox->x2 - srcbox->x1 ) ) );
                yfactors = ( (float)( odstbox->y2 - odstbox->y1 ) ) / ( (float)( osrcbox->y2 - osrcbox->y1 ) );
                dstbox->y2 = dstbox->y1 + (int) ( yfactors * ( (float)( srcbox->y2 - srcbox->y1 ) ) );
                break;
            case gcvSURF_90_DEGREE:
            case gcvSURF_270_DEGREE:
                xfactors = ( (float)( odstbox->x2 - odstbox->x1 ) ) / ( (float)( osrcbox->y2 - osrcbox->y1 ) );
                dstbox->x2 = dstbox->x1 + (int) ( xfactors * ( (float)( srcbox->y2 - srcbox->y1 ) ) );
                yfactors = ( (float)( odstbox->y2 - odstbox->y1 ) ) / ( (float)( osrcbox->x2 - osrcbox->x1 ) );
                dstbox->y2 = dstbox->y1 + (int) ( yfactors * ( (float)( srcbox->x2 - srcbox->x1 ) ) );
                break;
            default :
                break;
        }


    }

}

#if defined(ENABLE_MASK_OP)
static void
ReCalBoxByStretchInfoWithMask(VIV2DBLITINFOPTR pBlt, VivBox *opBox) {
/* when mask on, to cal the src/dst box */
}
#endif
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
#define MAX_COMPOSITE_SUB_SIZE IMX_EXA_MIN_PIXEL_AREA_COMPOSITE
void
VivComposite(PixmapPtr pxDst, int srcX, int srcY, int maskX, int maskY,
    int dstX, int dstY, int width, int height) {

    TRACE_ENTER();
    static int  _last_hw_composite = 0;
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pxDst);
    VivBox opBox;
    VIV2DBLITINFOPTR pBlt = &pViv->mGrCtx.mBlitInfo;
    Viv2DPixmapPtr psrc;
    Viv2DPixmapPtr pdst;
#if defined(ENABLE_MASK_OP)
    Bool isMasked = pBlt->mOperationCode == VIVCOMPOSITE_MASKED_SRC_REPEAT_PIXEL_ONLY_PATTERN
    || pBlt->mOperationCode == VIVCOMPOSITE_MASKED_SRC_REPEAT_ARBITRARY_SIZE_PATTERN
    || pBlt->mOperationCode == VIVCOMPOSITE_MASKED_SIMPLE;
#endif
    pBlt->mSwcmp = FALSE;
    /* Currenlty if mask on, it will not fall into this path ,otherwise consider mask */
    /* IMX_EXA_NONCACHESURF_WIDTH * IMX_EXA_NONCACHESURF_HEIGHT is small, enable this */
    /* otherwise disable it, it is not meaningful when the size is big */
    if ( ( width * height ) < MAX_COMPOSITE_SUB_SIZE && (IMX_EXA_NONCACHESURF_SIZE > MAX_COMPOSITE_SUB_SIZE))
    {

        if (_last_hw_composite > 0)
            VIV2DGPUBlitComplete(&pViv->mGrCtx,TRUE);
            _last_hw_composite = 0;

        pBlt->mSwcmp = TRUE;
        VIVSWComposite(pxDst, srcX, srcY, maskX, maskY, dstX, dstY, width, height);
        return ;
    }


    psrc = (Viv2DPixmapPtr)pBlt->mSrcSurfInfo.mPriv;
    pdst = (Viv2DPixmapPtr)pBlt->mDstSurfInfo.mPriv;

    if ( psrc->mCpuBusy )
    {
        VIV2DCacheOperation(&pViv->mGrCtx,psrc, FLUSH);
        psrc->mCpuBusy = FALSE;
    }

#if ALL_NONCACHE_BIGSURFACE
    if ( pdst->mCpuBusy && SURF_SIZE_FOR_SW_COND(pxDst->drawable.width, pxDst->drawable.height))
    {
        VIV2DCacheOperation(&pViv->mGrCtx,pdst, FLUSH);
        pdst->mCpuBusy = FALSE;
    }
#else
    if ( pdst->mCpuBusy )
    {
        VIV2DCacheOperation(&pViv->mGrCtx,pdst, FLUSH);
        pdst->mCpuBusy = FALSE;
    }
#endif

    initTempSurf(pBlt);
#if defined(ENABLE_MASK_OP)
    if ( isMasked )
        CalOrgBoxInfoWithMask(pBlt, srcX, srcY, maskX, maskY, dstX, dstY, width, height, &opBox);
    else
#endif
        CalOrgBoxInfoWithoutMask(pBlt, srcX, srcY, maskX, maskY, dstX, dstY, width, height, &opBox);

    /*fitting the operation box */
    /* Currently when mask is on, it goes sw, it shouldn't go into this */
#if defined(ENABLE_MASK_OP)
    if (isMasked) {
        ReCalBoxByStretchInfoWithMask(pBlt, &opBox);
    } else
#endif
    {
        ReCalBoxByStretchInfo(pBlt, &opBox);
    }

    if (!DoCompositeBlit(&pViv->mGrCtx, &opBox)) {
        TRACE_ERROR("Copy Blit Failed\n");
    }

    _last_hw_composite = 1;

    TRACE_EXIT();
}

void
VivDoneComposite(PixmapPtr pDst) {
    TRACE_ENTER();
    VivPtr pViv = VIVPTR_FROM_PIXMAP(pDst);
    VIV2DBLITINFOPTR pBlt = &pViv->mGrCtx.mBlitInfo;

    if ( pBlt && pBlt->mSwcmp )
        TRACE_EXIT();
    pBlt->hwMask |= 0x1;
    VIV2DGPUFlushGraphicsPipe(&pViv->mGrCtx);
    VIV2DGPUBlitComplete(&pViv->mGrCtx, FALSE);
    TRACE_EXIT();
}

