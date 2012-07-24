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
	VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);
	GenericSurfacePtr surf = (GenericSurfacePtr) (pBlt->mSrcSurfInfo.mPriv->mVidMemInfo);
	VivBoxPtr srcbox = &galInfo->mBlitInfo.mSrcBox;
	VivBoxPtr dstbox = &galInfo->mBlitInfo.mDstBox;
	VivBoxPtr osrcbox = &galInfo->mBlitInfo.mOSrcBox;
	VivBoxPtr odstbox = &galInfo->mBlitInfo.mODstBox;
	Bool nstflag = TRUE;

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
		0xCC,
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
	VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);
	GenericSurfacePtr surf = (GenericSurfacePtr) (pBlt->mSrcSurfInfo.mPriv->mVidMemInfo);

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

typedef struct  _trans_node {
	PictTransform *porg;
	PictTransform idst;
	gceSURF_ROTATION rt;
	Bool stretchflag;
}TRANS_NODE;
static TRANS_NODE _v_dst = {0};

#define INSTALL_MATRIX(ptransform) do { pixman_transform_init_identity(&_v_dst.idst); \
										pixman_transform_invert(&_v_dst.idst,(struct pixman_transform *)ptransform); \
										_v_dst.porg = ptransform; \
									} while ( 0)

/**
 * Check if the current transform is supported
 * Only support 90,180,270 rotation
 * @param ptransform
  * @param stretchflag
 * return TRUE if supported , otherwise FALSE
 */
Bool VIVTransformSupported(PictTransform *ptransform,Bool *stretchflag)
{
	struct pixman_vector   srcp1;
	struct pixman_vector   srcp2;

	srcp2.vector[0] = 512;
	srcp2.vector[1] = 0;
	srcp2.vector[2] = 0;

	srcp1.vector[0] = 0;
	srcp1.vector[1] = 0;
	srcp1.vector[2] = 0;

	INSTALL_MATRIX(ptransform);

	pixman_transform_point(&_v_dst.idst,&srcp1);
	pixman_transform_point(&_v_dst.idst,&srcp2);

	srcp2.vector[0] = srcp2.vector[0] - srcp1.vector[0];
	srcp2.vector[1] = srcp2.vector[1] - srcp1.vector[1];
	srcp2.vector[2] = srcp2.vector[2] - srcp1.vector[2];

	if ( srcp2.vector[0] !=0 && srcp2.vector[1] !=0 )
		return FALSE;

	_v_dst.stretchflag = FALSE;
	if ( srcp2.vector[0] != 512 && srcp2.vector[0] != 0 )
	{
		*stretchflag = TRUE;
		_v_dst.stretchflag = TRUE;
		return TRUE;
	}

	if ( srcp2.vector[1] != 512 && srcp2.vector[1] != 0 )
	{
		*stretchflag = TRUE;
		_v_dst.stretchflag = TRUE;
		return TRUE;
	}

}

gceSURF_ROTATION VIVGetRotation(PictTransform *ptransform)
{
	gceSURF_ROTATION rt=gcvSURF_0_DEGREE;

	struct pixman_vector   srcp1;
	struct pixman_vector   srcp2;

	srcp2.vector[0] = 512;
	srcp2.vector[1] = 0;
	srcp2.vector[2] = 0;

	srcp1.vector[0] = 0;
	srcp1.vector[1] = 0;
	srcp1.vector[2] = 0;

	if (_v_dst.porg != ptransform) {
		INSTALL_MATRIX(ptransform);
	}

	pixman_transform_point(&_v_dst.idst,&srcp1);
	pixman_transform_point(&_v_dst.idst,&srcp2);

	srcp2.vector[0] = srcp2.vector[0] - srcp1.vector[0];
	srcp2.vector[1] = srcp2.vector[1] - srcp1.vector[1];
	srcp2.vector[2] = srcp2.vector[2] - srcp1.vector[2];

	if ( srcp2.vector[0] > 0 && srcp2.vector[1] == 0 )
		rt = gcvSURF_0_DEGREE;

	if ( srcp2.vector[1] <0 && srcp2.vector[0] == 0 )
		rt = gcvSURF_90_DEGREE;

	if ( srcp2.vector[0] < 0 && srcp2.vector[1] == 0 )
		rt = gcvSURF_180_DEGREE;

	if ( srcp2.vector[1] >0 && srcp2.vector[0] == 0 )
		rt = gcvSURF_270_DEGREE;

	_v_dst.rt = rt;

	return rt;

}

void VIVGetSourceWH(PictTransform *ptransform, gctUINT32 deswidth, gctUINT32 desheight, gctUINT32 *srcwidth, gctUINT32 *srcheight )
{
	gceSURF_ROTATION rt=gcvSURF_0_DEGREE;
	Bool stretchflag;
	struct pixman_vector   srcp1x;
	struct pixman_vector   srcp2x;


	srcp1x.vector[0] = 0;
	srcp1x.vector[1] = 0;
	srcp1x.vector[2] = 0;

	srcp2x.vector[0] = deswidth;
	srcp2x.vector[1] = desheight;
	srcp2x.vector[2] = 0;


	if (_v_dst.porg != ptransform) {
		INSTALL_MATRIX(ptransform);
		VIVTransformSupported(ptransform,&stretchflag);
		VIVGetRotation(ptransform);
	}

	pixman_transform_point(&_v_dst.idst,&srcp1x);
	pixman_transform_point(&_v_dst.idst,&srcp2x);

	srcp2x.vector[0] = srcp2x.vector[0] - srcp1x.vector[0];
	srcp2x.vector[1] = srcp2x.vector[1] - srcp1x.vector[1];
	srcp2x.vector[2] = srcp2x.vector[2] - srcp1x.vector[2];

	switch ( _v_dst.rt ) {
		case gcvSURF_0_DEGREE:
		case gcvSURF_180_DEGREE:
			*srcwidth = abs(srcp2x.vector[0]);
			*srcheight = abs(srcp2x.vector[1]);
			break;
		case gcvSURF_90_DEGREE:
		case gcvSURF_270_DEGREE:
			*srcwidth = abs(srcp2x.vector[1]);
			*srcheight = abs(srcp2x.vector[0]);
			break;
	}

	return ;
}


static gctUINT8 VBITMASK[9]={0xFF,0x1,0x3,0x7,0xF,0x1F,0x3F,0x7F,0xFF};

#define VIVMONOWIDTH(width) ( ( ( width ) + 31 ) / 32 )

#define VIVMONOSIZE(width,height) ( ( VIVMONOWIDTH(width) ) * sizeof(gctUINT32) * (height) )

/* pdate should be type of (char *) */
#define VFASTSET1BIT(pdata,i,value) ( ( *( (pdata) + (i) ) ) |= (value) )
#define VFASTSET0BIT(pdata,i,value) ( ( *( (pdata) + (i) ) ) &= ( ~(value) ) )

#define XPOSINBITS(i,bitlen) ((i)*(bitlen)/8)
#define XOFFINBITS(i,bitlen) ((i)*(bitlen)%8)

#define VGETBITVALUE(pdata,x,off,bitlen) ( ( (*((pdata)+(x))) >> (off) & VBITMASK[(bitlen)] ) )

static void VIVInitIndexTable(gctUINT8 *pindex, gctUINT32 indexwidth)
{
	gctUINT32 i = 0;

	for ( i = 0; i < indexwidth ; i ++ )
		pindex[i] = 0x1 << ( (i) % 8 );
}

static gctUINT32 *VIVCreateGalMonoData(VIV2DBLITINFOPTR pBlt, VivBoxPtr opbox, gctUINT32 *alignedwidth, gctUINT32 *alignedheight)
{

	static gctUINT32 *pmonodata = NULL;
	static gctUINT32 monosize = 0;
	static gctUINT8 *pindex=NULL;
	static gctUINT32 indexwidth=1024;
	gctUINT32 temp = 0;
	char *psrc = NULL;
	char *pdes = NULL;
	gctUINT32 srcstride = 0;
	gctUINT32 RGBAMASK = 0xFF;
	gctUINT32 srcpixel = 0;
	char *psrcpixel = (char *)&srcpixel;
	gctUINT32 i , j , k;
	gctUINT32 deswidth;
	gctUINT32 subdeswidth;
	gctUINT32 monox;
	gctUINT32 bitssize;
	gctUINT32 sx, sy, sx1, sx2,sxoff,sxoff2;
	gctUINT8 bitvalue;
	char 	*pcmonodata = NULL;

	GenericSurfacePtr msksurf = (GenericSurfacePtr) (pBlt->mMskSurfInfo.mPriv->mVidMemInfo);

	subdeswidth = VIVMONOWIDTH(opbox->width);
	deswidth = VIVMONOWIDTH(pBlt->mMskSurfInfo.mWidth);

	sx = pBlt->mMskBox.x1;
	sy = pBlt->mMskBox.y1;

	if (1) {
		/* Fast to check if createing monodata is a must */
		if ( sx ==0 && sy == 0 && ( ( (msksurf->mStride*8) % 32 ) == 0 ) ) {
			opbox->width = subdeswidth<<5;
			*alignedwidth = (msksurf->mStride*8);
			*alignedheight = pBlt->mMskSurfInfo.mHeight;
			return ( gctUINT32 *)msksurf->mLogicalAddr;
		}

	}

	if ( pindex == NULL || indexwidth < pBlt->mMskSurfInfo.mWidth )
	{
		if ( pindex )
			free(pindex);

		if ( indexwidth < pBlt->mMskSurfInfo.mWidth )
			indexwidth = pBlt->mMskSurfInfo.mWidth;

		pindex = (gctUINT8 *)malloc(sizeof(gctUINT8) * indexwidth);
		VIVInitIndexTable(pindex,indexwidth);

	}

	if ( ( temp = VIVMONOSIZE(pBlt->mMskSurfInfo.mWidth, pBlt->mMskSurfInfo.mHeight) ) > monosize )
	{
		monosize = temp;
		if ( pmonodata )
			free( pmonodata );
		pmonodata = (gctUINT32 *)malloc( monosize );
	}

	psrc = (char *)msksurf->mLogicalAddr;
	srcstride = msksurf->mStride;

	/* According to picformat, set RGBAMASK */
	switch(pBlt->mMskSurfInfo.mFormat.mExaFmt)
	{
		default :
			;
	}

	//memset(pmonodata,0,monosize);


	pcmonodata = (char *)pmonodata;
	switch( pBlt->mMskSurfInfo.mFormat.mBpp)
	{
		case 1:
			monox = deswidth*4;
			psrc += (sy*srcstride);
			pdes = (char *)pcmonodata;
			sx1 = XPOSINBITS((sx),1);
			temp = sx1;
			sxoff = XOFFINBITS((sx),1);
			sx2 = XPOSINBITS((opbox->width-1+sx),1);
			sxoff2 = XOFFINBITS((opbox->width-1+sx),1);

			psrc += sx1;
			for ( j = 0; j < opbox->height; j++) {
				sx1 = temp;
				if (sxoff == 0) {
					i = 0;
					if (sx1 < sx2 ) {
						while ( sx1 < sx2 ) {
							pdes[i] = psrc[i];
							i++;
							sx1++;
						}
					}
					pdes[i] = psrc[i] & VBITMASK[sxoff2+1];
				} else {
					i = 0;
					if ( sx1 < sx2 ){
						pdes[0] = psrc[0]>>sxoff;
						sx1++;
						i = 1;
						while ( sx1 <= sx2 ) {
							pdes[i-1] = pdes[i-1] | ( psrc[i] << (8-sxoff) );
							pdes[i] = psrc[i]>>(sxoff);
							i++;
							sx1++;
						}
						/* recalculate the last one */
						pdes[i-2] &= VBITMASK[8-sxoff-1];
						pdes[i-2] = pdes[i-2] | ( ( psrc[i-1] & VBITMASK[sxoff2+1] ) << (8-sxoff) );
						pdes[i-1] = ( psrc[i-1] & VBITMASK[sxoff2+1] ) >>(sxoff);

					} else {
						pdes[i] = psrc[i]>>sxoff;
						pdes[i] = pdes[i] & VBITMASK[sxoff2+1-sxoff];
					}
				}
				psrc += srcstride;
				pdes += monox;
			}
			break;
		case 4:
			psrc += (sy*srcstride);
			monox = deswidth*4;
			for ( j = 0; j < opbox->height; j++) {
				for ( i = 0; i < opbox->width; i++) {
					sx1 = XPOSINBITS((i+sx),4);
					sxoff = XOFFINBITS((i+sx),4);
					bitvalue = VGETBITVALUE(psrc,sx1,sxoff,4);
					if ( bitvalue )
							VFASTSET1BIT(pcmonodata,i>>3,pindex[i]);
						else
							VFASTSET0BIT(pcmonodata,i>>3,pindex[i]);
				}
				psrc += srcstride;
				pcmonodata +=monox;
			}
			break;
			break;
		case 8:
			psrc += (sy*srcstride);
			monox = deswidth*4;
			for ( j = 0; j < opbox->height; j ++ ) {
				for ( i = 0; i <= opbox->width ; i++) {
					if ( psrc[i+sx] )
						VFASTSET1BIT(pcmonodata,i>>3,pindex[i]);
					else
						VFASTSET0BIT(pcmonodata,i>>3,pindex[i]);
				}
				psrc += srcstride;
				pcmonodata +=monox;
			}
			break;
		case 16:
			psrc += (sy*srcstride);
			monox = deswidth*4;
			for ( j = 0; j < opbox->height; j ++ ) {
				for ( i = 0; i < opbox->width ; i ++) {
					k = 2*(i+sx);
					psrcpixel[0] = psrc[k];
					psrcpixel[1] = psrc[k+1];
					if ( srcpixel & RGBAMASK )
						VFASTSET1BIT(pcmonodata,i>>3,pindex[i]);
					else
						VFASTSET0BIT(pcmonodata,i>>3,pindex[i]);
					srcpixel = 0;
				}
				psrc += srcstride;
				pcmonodata +=monox;
			}
			break;
		case 24:
			psrc += (sy*srcstride);
			monox = deswidth*4;
			for ( j = 0; j < opbox->height; j ++ ) {
				for ( i = 0; i < opbox->width ; i++) {
					k = 3*(i+sx);
					psrcpixel[0] = psrc[k];
					psrcpixel[1] = psrc[k+1];
					psrcpixel[2] = psrc[k+2];
					if ( srcpixel & RGBAMASK )
						VFASTSET1BIT(pcmonodata,i>>3,pindex[i]);
					else
						VFASTSET0BIT(pcmonodata,i>>3,pindex[i]);
					srcpixel = 0;
				}
				psrc += srcstride;
				pcmonodata +=monox;
			}
			break;
		case 32:
			psrc += (sy*srcstride);
			monox = deswidth*4;
			for ( j = 0; j < opbox->height ; j ++ ) {
				for ( i = 0; i < opbox->width; i++ ) {
					k = 4*(i+sx);
					psrcpixel = &psrc[k];
					if ( *((gctUINT32 *)psrcpixel) & RGBAMASK )
						VFASTSET1BIT(pcmonodata,i>>3,pindex[i]);
					else
						VFASTSET0BIT(pcmonodata,i>>3,pindex[i]);
					//srcpixel = 0;
				}
				psrc += srcstride;
				pcmonodata +=monox;
			}
			break;
		default :
			;
	}

	opbox->width = subdeswidth<<5;
	*alignedwidth = deswidth<<5;
	*alignedheight = pBlt->mMskSurfInfo.mHeight;

	return pmonodata;

}

/**
 * only for 1x1 source for filling destination surface with src repeat mode
 * @param galInfo
 * @param opbox
 * @return TRUE if successful otherwise FALSE
 */
static Bool BlendWithMonoBlit(GALINFOPTR galInfo, VivBoxPtr opbox)
{
	gceSURF_MONOPACK srcType = gcvSURF_UNPACKED;
	gceSURF_MONOPACK desType = gcvSURF_UNPACKED;
	gctUINT32 bgColor = 0, fgColor=0;
	gcsRECT dstRect = {0, 0, 0, 0};
	gcsRECT srcRect = {0, 0, 0, 0};
	gctUINT32 i = 0;
	char		*pdata = NULL;
	char		*dpdata = NULL;
	gcsRECT streamRect;
	gcsPOINT streamSize;
	gceSTATUS status;
	Bool 	ret = FALSE;
	gctUINT32	*pmonodata = NULL;
	gctUINT32	mwidth;
	gctUINT32	mheight;

	VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
	VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);

	GenericSurfacePtr dstsurf = (GenericSurfacePtr) (pBlt->mDstSurfInfo.mPriv->mVidMemInfo);
	GenericSurfacePtr srcsurf = (GenericSurfacePtr) (pBlt->mSrcSurfInfo.mPriv->mVidMemInfo);
	GenericSurfacePtr msksurf = (GenericSurfacePtr) (pBlt->mMskSurfInfo.mPriv->mVidMemInfo);

	/*Enabling the alpha blending*/
	if (!EnableAlphaBlending(galInfo)) {
		TRACE_ERROR("Alpha Blending Factor\n");
		TRACE_EXIT(FALSE);
	}

	srcType = gcvSURF_UNPACKED;


	desType = gcvSURF_UNPACKED;

	pdata = (char *)srcsurf->mLogicalAddr;
	dpdata = (char *)&fgColor;
	dpdata[0] = pdata[0];
	for( i=0; i<pBlt->mSrcSurfInfo.mFormat.mBpp/8; i++)
		dpdata[i] = pdata[i];
	bgColor = fgColor;

	status = gco2D_SetMonochromeSource(gpuctx->mDriver->m2DEngine,
									pBlt->mColorConvert,
									0,
									srcType,
									gcvFALSE,
									gcvSURF_SOURCE_MASK,
									fgColor,
									bgColor);
	if (status != gcvSTATUS_OK)
	{
		goto ERROR;
	}

	gcmVERIFY_OK(gco2D_SetSource(gpuctx->mDriver->m2DEngine, &srcRect));
	pBlt->mRotation=gcvSURF_90_DEGREE;
	gcmVERIFY_OK(gco2D_SetTarget(gpuctx->mDriver->m2DEngine,dstsurf->mVideoNode.mPhysicalAddr,dstsurf->mStride, pBlt->mRotation, dstsurf->mAlignedWidth));

	dstRect.left = 0;
	dstRect.top = 0;

	switch ( pBlt->mRotation) {
		case gcvSURF_0_DEGREE:
		case gcvSURF_180_DEGREE:
			dstRect.right = pBlt->mDstBox.x1+opbox->width;
			dstRect.bottom = pBlt->mDstBox.y1+opbox->height;
			break;
		case gcvSURF_90_DEGREE:
		case gcvSURF_270_DEGREE:
			dstRect.right = pBlt->mDstBox.x1+opbox->height;
			dstRect.bottom = pBlt->mDstBox.y1+opbox->width;
			break;
		default :
			break;
	}

	status = gco2D_SetClipping(gpuctx->mDriver->m2DEngine, &dstRect);
	if (status != gcvSTATUS_OK) {
		TRACE_ERROR("gco2D_SetClipping failed\n");
		TRACE_EXIT(FALSE);
	}

	pmonodata = VIVCreateGalMonoData(pBlt,opbox,&mwidth,&mheight);

	dstRect.left = pBlt->mDstBox.x1;
	dstRect.top = pBlt->mDstBox.y1;

	/* opbox perhaps changes */
	switch ( pBlt->mRotation) {
		case gcvSURF_0_DEGREE:
		case gcvSURF_180_DEGREE:
			dstRect.right = dstRect.left+ opbox->width;
			dstRect.bottom = dstRect.top+ opbox->height;
			break;
		case gcvSURF_90_DEGREE:
		case gcvSURF_270_DEGREE:
			dstRect.right = dstRect.left+ opbox->height;
			dstRect.bottom = dstRect.top+ opbox->width;
			break;
		default :
			break;
	}

	streamRect.left = 0;
	streamRect.top = 0;



	streamRect.right = streamRect.left+ opbox->width;
	streamRect.bottom = streamRect.top+ opbox->height;

	streamSize.x = mwidth;
	streamSize.y = mheight;

	gcmVERIFY_OK(gco2D_MonoBlit(gpuctx->mDriver->m2DEngine, pmonodata, &streamSize, &streamRect, srcType, desType,
		&dstRect, 0xCC, 0xAA, pBlt->mDstSurfInfo.mFormat.mVivFmt));

	ret=TRUE;

ERROR:

	if (!DisableAlphaBlending(galInfo)) {
		TRACE_ERROR("Disabling Alpha Blend Failed\n");
		ret=FALSE;
	}

	TRACE_EXIT(ret);

}

static Bool BlendSurfaceWithBrush(GALINFOPTR galInfo, VivBoxPtr opbox)
{



}

static Bool BlendMaskedConstPatternRect(GALINFOPTR galInfo, VivBoxPtr opbox) {

	TRACE_ENTER();

	VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
	VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);

	GenericSurfacePtr dstsurf = (GenericSurfacePtr) (pBlt->mDstSurfInfo.mPriv->mVidMemInfo);
	GenericSurfacePtr srcsurf = (GenericSurfacePtr) (pBlt->mSrcSurfInfo.mPriv->mVidMemInfo);
	GenericSurfacePtr msksurf = (GenericSurfacePtr) (pBlt->mMskSurfInfo.mPriv->mVidMemInfo);

	if ( ( pBlt->mSrcSurfInfo.mHeight * pBlt->mSrcSurfInfo.mWidth ) == 1 ){
			return BlendWithMonoBlit(galInfo,opbox);
	}

	/* Perhaps something else to do here */

	TRACE_EXIT(FALSE);
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
	gctUINT32	*pmonodata = NULL;
	gctUINT32	mwidth;
	gctUINT32	mheight;
	int i;
	gceSTATUS status = gcvSTATUS_OK;

	gctUINT8_PTR monodata = gcvNULL;

	gceSURF_MONOPACK srcType = gcvSURF_UNPACKED;
	VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
	VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);

	GenericSurfacePtr dstsurf = (GenericSurfacePtr) (pBlt->mDstSurfInfo.mPriv->mVidMemInfo);
	GenericSurfacePtr srcsurf = (GenericSurfacePtr) (pBlt->mSrcSurfInfo.mPriv->mVidMemInfo);
	GenericSurfacePtr msksurf = (GenericSurfacePtr) (pBlt->mMskSurfInfo.mPriv->mVidMemInfo);

	gcsRECT mSrcRect = {pBlt->mSrcBox.x1, pBlt->mSrcBox.y1, pBlt->mSrcBox.x1 + opbox->width, pBlt->mSrcBox.y1 + opbox->height};
	gcsRECT mDstRect = {pBlt->mDstBox.x1, pBlt->mDstBox.y1, pBlt->mDstBox.x1 + opbox->width, pBlt->mDstBox.y1 + opbox->height};

	mDstRect.left = pBlt->mDstBox.x1;
	mDstRect.top= pBlt->mDstBox.y1;

	switch ( pBlt->mRotation) {
		case gcvSURF_0_DEGREE:
		case gcvSURF_180_DEGREE:
			mDstRect.right = mDstRect.left+opbox->width;
			mDstRect.bottom = mDstRect.top+opbox->height;
			break;
		case gcvSURF_90_DEGREE:
		case gcvSURF_270_DEGREE:
			mDstRect.right = mDstRect.left+opbox->height;
			mDstRect.bottom = mDstRect.top+opbox->width;
			break;
		default :
			break;
	}

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
	status = gco2D_SetTarget(gpuctx->mDriver->m2DEngine, dstsurf->mVideoNode.mPhysicalAddr, dstsurf->mStride, pBlt->mRotation, dstsurf->mAlignedWidth);
	if (status != gcvSTATUS_OK) {
		TRACE_ERROR("Set Target Failed\n");
		TRACE_EXIT(FALSE);
	}
	/*setting the clipping for dest*/
	status = gco2D_SetClipping(gpuctx->mDriver->m2DEngine, &mDstRect);
	if (status != gcvSTATUS_OK) {
		TRACE_ERROR("gco2D_SetClipping failed\n");
		TRACE_EXIT(FALSE);
	}
	/*Enabling the alpha blending*/
	if (!EnableAlphaBlending(galInfo)) {
		TRACE_ERROR("Alpha Blending Factor\n");
		TRACE_EXIT(FALSE);
	}

	pmonodata = VIVCreateGalMonoData(pBlt,opbox,&mwidth,&mheight);

	switch ( pBlt->mRotation) {
		case gcvSURF_0_DEGREE:
		case gcvSURF_180_DEGREE:
			mDstRect.right = mDstRect.left+opbox->width;
			mDstRect.bottom = mDstRect.top+opbox->height;
			break;
		case gcvSURF_90_DEGREE:
		case gcvSURF_270_DEGREE:
			mDstRect.right = mDstRect.left+opbox->height;
			mDstRect.bottom = mDstRect.top+opbox->width;
			break;
		default :
			break;
	}

	/*rectangle*/
	streamRect.left = 0;
	streamRect.top = 0;
	streamRect.right = opbox->width;
	streamRect.bottom = opbox->height;

	/*size*/
	streamSize.x = mwidth;
	streamSize.y = mheight;

	status = gco2D_MonoBlit(gpuctx->mDriver->m2DEngine,pmonodata /*msksurf->mLogicalAddr*/,
		&streamSize, &streamRect, srcType, srcType, &mDstRect, 0xCC, 0xAA, pBlt->mDstSurfInfo.mFormat.mVivFmt);

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
static void
CalOverAndSrcBoxes(GALINFOPTR galInfo, gcsRECT *psrcrect, gcsRECT *pdstrect, int dstrectxnums, int dstrectynums)
{
	int i,j;
	int subwidth;
	int subheight;
	int x1;
	int y1;
	int x2;
	int y2;

	VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
	VivBoxPtr srcbox = &galInfo->mBlitInfo.mSrcBox;
	VivBoxPtr dstbox = &galInfo->mBlitInfo.mDstBox;
	VivBoxPtr osrcbox = &galInfo->mBlitInfo.mOSrcBox;
	VivBoxPtr odstbox = &galInfo->mBlitInfo.mODstBox;

	subwidth = srcbox->x2-srcbox->x1;
	subheight = srcbox->y2-srcbox->y1;

	x1 = srcbox->x1;
	x2 = srcbox->x2;

	y1 = srcbox->y1;
	y2 = srcbox->y2;

	for ( j = 0 ; j<dstrectynums; j++)
	{
		for ( i = 0 ; i<dstrectxnums; i++)
		{
				psrcrect[i].left = x1;
				psrcrect[i].top = y1;
				psrcrect[i].right = x2;
				psrcrect[i].bottom = y2;

				pdstrect[i].left = dstbox->x1 + subwidth*i;
				pdstrect[i].top = dstbox->y1 + subheight*j;
				pdstrect[i].right = pdstrect[i].left + subwidth;

				if ( j == (dstrectynums - 1 ) ) {
					pdstrect[i].bottom = V_MIN((pdstrect[i].top + subheight), dstbox->y2);
					psrcrect[i].bottom = psrcrect[i].top + pdstrect[i].bottom - pdstrect[i].top;
				} else {
					pdstrect[i].bottom = pdstrect[i].top + subheight;
				}

				if ( pdstrect[i].top == srcbox->y1 ) {
					x1 = 0;
					y1 = srcbox->y1;
					x2 = subwidth;
					y2 = subheight;
				} else {
					x1 = 0;
					y1 = 0;
					x2 = subwidth;
					y2 = subheight;
				}

		}

		pdstrect[i-1].right = V_MIN(pdstrect[i-1].right, dstbox->x2);
		psrcrect[i-1].right = psrcrect[i].left + pdstrect[i-1].right - pdstrect[i-1].left;

		pdstrect += dstrectxnums;
		psrcrect +=dstrectxnums;

		x1 = srcbox->x1;
		y1 = 0;
		x2 = subwidth;
		y2 = subheight;

	}

}

static void
CalNormalBoxes(GALINFOPTR galInfo, gcsRECT *psrcrect, gcsRECT *pdstrect, int dstrectxnums, int dstrectynums)
{

	int i,j;
	int subwidth;
	int subheight;

	VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
	VivBoxPtr srcbox = &galInfo->mBlitInfo.mSrcBox;
	VivBoxPtr dstbox = &galInfo->mBlitInfo.mDstBox;

	subwidth = srcbox->x2-srcbox->x1;
	subheight = srcbox->y2-srcbox->y1;

	for ( j = 0 ; j<dstrectynums; j++)
	{
		for ( i = 0 ; i<dstrectxnums; i++)
		{
				psrcrect[i].left = srcbox->x1;
				psrcrect[i].top = srcbox->y1;
				psrcrect[i].right = srcbox->x2;
				psrcrect[i].bottom = srcbox->y2;

				pdstrect[i].left = dstbox->x1 + subwidth*i;
				pdstrect[i].top = dstbox->y1 + subheight*j;
				pdstrect[i].right = pdstrect[i].left + subwidth;

				if ( j == (dstrectynums - 1 ) ) {
					pdstrect[i].bottom = V_MIN((pdstrect[i].top + subheight), dstbox->y2);
					psrcrect[i].bottom = psrcrect[i].top + pdstrect[i].bottom - pdstrect[i].top;
				} else {
					pdstrect[i].bottom = pdstrect[i].top + subheight;
				}

		}

		pdstrect[i-1].right = V_MIN(pdstrect[i-1].right, dstbox->x2);
		psrcrect[i-1].right = psrcrect[i].left + pdstrect[i-1].right - pdstrect[i-1].left;

		pdstrect += dstrectxnums;
		psrcrect +=dstrectxnums;

	}

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

static Bool BlendArbitraryPatternRect(GALINFOPTR galInfo, VivBoxPtr opbox) {
	TRACE_ENTER();
	gceSTATUS status = gcvSTATUS_OK;
	VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
	VivBoxPtr srcbox = &galInfo->mBlitInfo.mSrcBox;
	VivBoxPtr dstbox = &galInfo->mBlitInfo.mDstBox;

	gcsRECT *pdstrect=NULL;
	gcsRECT *psrcrect=NULL;

	gcsRECT *podstrect=NULL;
	gcsRECT *posrcrect=NULL;

	int dstrectxnums;
	int dstrectynums;
	int i,j;
	int subwidth;
	int subheight;

	subwidth = srcbox->x2-srcbox->x1;
	subheight = srcbox->y2-srcbox->y1;

	dstrectxnums = ( dstbox->x2-dstbox->x1 + subwidth-1) / (subwidth);
	dstrectynums = ( dstbox->y2-dstbox->y1 + subheight-1) / (subheight);

	pdstrect = (gcsRECT *)malloc( sizeof( gcsRECT )*dstrectxnums*dstrectynums);
	psrcrect = (gcsRECT *)malloc( sizeof( gcsRECT )*dstrectxnums*dstrectynums);

	podstrect = pdstrect;
	posrcrect = psrcrect;

	if ( galInfo->mBlitInfo.mBlendOp.mOp == PictOpOver || galInfo->mBlitInfo.mBlendOp.mOp == PictOpSrc )
	{
		/* composite_one_pass_special will be replaced by CalOverAndSrcBoxes, especially when rotation is on */
		//CalOverAndSrcBoxes(galInfo,psrcrect,pdstrect,dstrectxnums,dstrectynums);
		composite_one_pass_special(galInfo,opbox);
		goto ENDFLAG;

	} else {
		CalNormalBoxes( galInfo, psrcrect, pdstrect, dstrectxnums, dstrectynums);
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
	dstrectxnums*dstrectynums,
	posrcrect,
	podstrect,
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

ENDFLAG:

	free((void *)posrcrect);
	free((void *)podstrect);

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

static uint32_t
real_reader (const void *src, int size)
{
	switch (size)
	{
		case 1:
			return *(uint8_t *)src;
		case 2:
			return *(uint16_t *)src;
		case 3:
			return (*(uint32_t *)src) & 0x00FFFFFF;
		case 4:
			return *(uint32_t *)src;
		default:
			assert (0);
		return 0; /* silence MSVC */
	}
}

static void
real_writer (void *src, uint32_t value, int size)
{
	switch (size)
	{
		case 1:
			*(uint8_t *)src = value;
			break;
		case 2:
			*(uint16_t *)src = value;
			break;
		case 3:
			(*(uint16_t *)src) = value & 0xFFFF;
			(*( ( (uint8_t *)src )+1) ) = value>>16;
			break;
		case 4:
			*(uint32_t *)src = value;
		break;
		default:
			assert (0);
			break;
	}
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
					srcsurf->mAlignedWidth,
					srcsurf->mAlignedHeight, (uint32_t *) srcsurf->mLogicalAddr,
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
					dstsurf->mAlignedWidth,
					dstsurf->mAlignedHeight, (uint32_t *) dstsurf->mLogicalAddr,
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

static void SetTempSurfForRT(GALINFOPTR galInfo, VivBoxPtr opbox, GenericSurfacePtr *pinfo)
{
	GenericSurfacePtr srcsurf;
	gceSTATUS status = gcvSTATUS_OK;
	VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
	VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);
	VivBoxPtr osrcbox = &galInfo->mBlitInfo.mOSrcBox;
	VivBoxPtr odstbox = &galInfo->mBlitInfo.mODstBox;
	VivBoxPtr srcbox = &galInfo->mBlitInfo.mOSrcBox;
	VivBoxPtr dstbox = &galInfo->mBlitInfo.mODstBox;
	Bool retvsurf;
	gctUINT32 physicaladdr;
	gctUINT32 linearaddr;
	gctUINT32 aligned_height;
	gctUINT32 aligned_width;
	gctUINT32 aligned_pitch;
	gctUINT32 tx;


	gcsRECT srcrectx = {0, 0, osrcbox->x2, osrcbox->y2};



	gcsRECT dstRect = {0, 0, 0, 0};

	gctUINT32 maxsize;

	if ( pBlt->mRotation == gcvSURF_0_DEGREE)
		return ;

	/* neccessary ?, fix me ?
	if ( osrcbox->x1 == 0
		&& osrcbox->y1 == 0
		&& odstbox->x1 == 0
		&& odstbox->y1 == 0)
		return ;
	*/

	srcsurf = (GenericSurfacePtr) (pBlt->mSrcSurfInfo.mPriv->mVidMemInfo);
	maxsize = V_MAX(srcsurf->mAlignedWidth, srcsurf->mAlignedHeight);

	*pinfo = (GenericSurfacePtr)malloc(sizeof(GenericSurface));
	memcpy((void *)(*pinfo), (void *)srcsurf, sizeof(GenericSurface));


	switch ( pBlt->mSrcSurfInfo.mFormat.mBpp )
	{

		case 16:
			retvsurf = VGetSurfAddrBy16(galInfo, maxsize, (int *) (&physicaladdr), (int *) (&linearaddr), &aligned_width, &aligned_height, &aligned_pitch);
		case 32:
			retvsurf = VGetSurfAddrBy32(galInfo, maxsize, (int *) (&physicaladdr), (int *) (&linearaddr), &aligned_width, &aligned_height, &aligned_pitch);
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
		pBlt->mSrcSurfInfo.mFormat.mVivFmt,
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

	/*setting the required area*/
	status = gco2D_SetSource(gpuctx->mDriver->m2DEngine, &srcrectx);
	if (status != gcvSTATUS_OK) {
		TRACE_ERROR("gco2D_SetSource failed\n");
		TRACE_EXIT();
	}

	switch ( pBlt->mRotation ) {
		case gcvSURF_0_DEGREE:
		case gcvSURF_180_DEGREE:
			dstRect.right = srcrectx.right;
			dstRect.bottom = srcrectx.bottom;
			break;
		case gcvSURF_90_DEGREE:
		case gcvSURF_270_DEGREE:
			dstRect.right = srcrectx.bottom;
			dstRect.bottom = srcrectx.right;
			break;
		default :
			;
	}

	status = gco2D_Blit(gpuctx->mDriver->m2DEngine, 1, &dstRect, pBlt->mFgRop, pBlt->mBgRop, pBlt->mDstSurfInfo.mFormat.mVivFmt);
	if (status != gcvSTATUS_OK) {
		TRACE_ERROR("Blit failed\n");
		TRACE_EXIT();
	}

	srcsurf->mAlignedWidth = aligned_width;
	srcsurf->mAlignedHeight = aligned_height;
	srcsurf->mLogicalAddr = (gctPOINTER)linearaddr;
	srcsurf->mStride = aligned_pitch;
	srcsurf->mVideoNode.mPhysicalAddr = physicaladdr;
	srcsurf->mRotation = gcvSURF_0_DEGREE;

	/* update source box */

	switch ( pBlt->mRotation ) {

		case gcvSURF_90_DEGREE:
			tx = srcbox->y1;
			srcbox->y1 = srcbox->x2;
			srcbox->x1 = tx;
			tx = srcbox->y2;
			srcbox->y2 = srcbox->x1;
			srcbox->x2 = tx;
			break;
		case gcvSURF_180_DEGREE:
			srcbox->x2 = srcbox->x2 - srcbox->x1;
			srcbox->y2 = srcbox->y2 - srcbox->y1;
			srcbox->x1 = 0;
			srcbox->y1 = 0;
			break;
		case gcvSURF_270_DEGREE:
			tx = srcbox->x2;
			srcbox->x2 = srcbox->y2 - srcbox->y1;
			srcbox->y2 = tx - srcbox->x1;
			srcbox->x1 = 0;
			srcbox->y1 = 0;
			break;
		default :
			;
	}

}

static void ReleaseTempSurfForRT(GALINFOPTR galInfo, VivBoxPtr opbox, GenericSurfacePtr *pinfo)
{
	GenericSurfacePtr srcsurf;
	VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
	VIV2DBLITINFOPTR pBlt = &(galInfo->mBlitInfo);

	if (pBlt->mRotation == gcvSURF_0_DEGREE)
		return ;

	if (*pinfo == NULL ) return ;

	srcsurf = (GenericSurfacePtr) (pBlt->mSrcSurfInfo.mPriv->mVidMemInfo);
	memcpy((void *)(srcsurf), (void *)(*pinfo), sizeof(GenericSurface));

	free(*pinfo);

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
	GenericSurfacePtr psurfinfo;

	/* if rotation happens, set temp surf , currently it is not debugged To fix it, when rotation is on*/
	SetTempSurfForRT(galInfo,opbox,&psurfinfo);

	/* Perform rectangle render based on setup in PrepareComposite */
	switch (galInfo->mBlitInfo.mOperationCode) {

	#if ENABLE_MASK_OP
		case VIVCOMPOSITE_MASKED_SRC_REPEAT_PIXEL_ONLY_PATTERN:
			ret = BlendMaskedConstPatternRect(galInfo, opbox);
			break;
		case VIVCOMPOSITE_MASKED_SRC_REPEAT_ARBITRARY_SIZE_PATTERN:
			ret = BlendMaskedArbitraryPatternRect(galInfo, opbox);
			break;
		case VIVCOMPOSITE_MASKED_SIMPLE:
			ret = BlendMaskedSimpleRect(galInfo, opbox);
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

	ReleaseTempSurfForRT(galInfo,opbox, &psurfinfo);
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
