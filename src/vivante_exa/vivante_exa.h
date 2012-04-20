/****************************************************************************
*
*    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/


/*
 * File:   vivante_exa.h
 * Author: vivante
 *
 * Created on February 23, 2012, 11:10 AM
 */

#ifndef VIVANTE_EXA_H
#define	VIVANTE_EXA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vivante_common.h"

#define VIV_EXA_FLUSH_2D_CMD_ENABLE         0
#define VIV_EXA_SOLID_SIZE_CHECK_ENABLE     1
#define VIV_EXA_COPY_SIZE_CHECK_ENABLE      1

#define	IMX_EXA_MIN_PIXEL_AREA_SOLID        10000
#define	IMX_EXA_MIN_PIXEL_AREA_COPY         14400
#define	IMX_EXA_MIN_PIXEL_AREA_COMPOSITE    150

    /************************************************************************
     * EXA COPY  (START)
     ************************************************************************/
    Bool
    VivPrepareCopy(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap,
            int xdir, int ydir, int alu, Pixel planemask);

    void
    VivCopy(PixmapPtr pDstPixmap, int srcX, int srcY, int dstX, int dstY, int width, int height);

    void
    VivDoneCopy(PixmapPtr pDstPixmap);

    /************************************************************************
     * EXA COPY (FINISH)
     ************************************************************************/

    /************************************************************************
     * EXA SOLID  (START)
     ************************************************************************/
    Bool
    VivPrepareSolid(PixmapPtr pPixmap, int alu, Pixel planemask, Pixel fg);

    void
    VivSolid(PixmapPtr pPixmap, int x1, int y1, int x2, int y2);

    void
    VivDoneSolid(PixmapPtr pPixmap);

    /************************************************************************
     * EXA SOLID (FINISH)
     ************************************************************************/


    /************************************************************************
     * EXA COMPOSITE  (START)
     ************************************************************************/
    Bool
    VivCheckComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture, PicturePtr pDstPicture);
    Bool
    VivPrepareComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture, PicturePtr pDstPicture,
            PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst);
    void
    VivComposite(PixmapPtr pDst, int srcX, int srcY, int maskX, int maskY,
            int dstX, int dstY, int width, int height);
    void
    VivDoneComposite(PixmapPtr pDst);

    /************************************************************************
     * EXA COMPOSITE (FINISH)
     ************************************************************************/

    /************************************************************************
     * EXA PIXMAP  (START)
     ************************************************************************/
    Bool
    VivPrepareAccess(PixmapPtr pPix, int index);

    void
    VivFinishAccess(PixmapPtr pPix, int index);

    void *
    VivCreatePixmap(ScreenPtr pScreen, int size, int align);

    void
    VivDestroyPixmap(ScreenPtr pScreen, void *dPriv);

    Bool
    VivPixmapIsOffscreen(PixmapPtr pPixmap);

    Bool
    VivModifyPixmapHeader(PixmapPtr pPixmap, int width, int height,
            int depth, int bitsPerPixel, int devKind,
            pointer pPixData);

    /************************************************************************
     * EXA PIXMAP (FINISH)
     ************************************************************************/

    /************************************************************************
     * EXA OTHER FUNCTIONS  (START)
     ************************************************************************/
    void
    VivEXASync(ScreenPtr pScreen, int marker);

    Bool
    VivUploadToScreen(PixmapPtr pDst, int x, int y, int w, int h, char *src, int src_pitch);

    Bool
    VivDownloadFromScreen(PixmapPtr pSrc, int x, int y, int w, int h, char *dst, int dst_pitch);
    /************************************************************************
     * EXA OTHER FUNCTIONS (END)
     ************************************************************************/

    /************************************************************************
     * UTILITY FUNCTIONS  (START)
     ************************************************************************/
    Bool CheckBltvalidity(PixmapPtr pPixmap, int alu, Pixel planemask);
    /************************************************************************
     * UTILITY FUNCTIONS (END)
     ************************************************************************/

#ifdef __cplusplus
}
#endif

#endif	/* VIVANTE_EXA_H */

