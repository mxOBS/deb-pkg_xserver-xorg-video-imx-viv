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


#ifndef VIVANTE_EXA_H
#define VIVANTE_EXA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vivante_common.h"

#define IMX_EXA_MIN_AREA_CLEAN         40000
#define IMX_EXA_MIN_PIXEL_AREA_COMPOSITE    640

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
    Bool CheckCPYValidity(PixmapPtr pPixmap, int alu, Pixel planemask);
    Bool CheckFILLValidity(PixmapPtr pPixmap, int alu, Pixel planemask);
    void ConvertXAluToOPS(PixmapPtr pPixmap, int alu, Pixel planemask, int *fg, int *bg);
    PixmapPtr GetDrawablePixmap(DrawablePtr pDrawable);
    /************************************************************************
     * UTILITY FUNCTIONS (END)
     ************************************************************************/

#ifdef __cplusplus
}
#endif

#endif  /* VIVANTE_EXA_H */

