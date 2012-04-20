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
#include "vivante.h"

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
