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

#ifndef VIVANTE_EXA_G2D_H
#define VIVANTE_EXA_G2D_H

#ifdef __cplusplus
extern "C" {
#endif

#define ENABLE_EXA_TRACE                  0
#define ENABLE_EXA_TRACE_ERROR            0
#define ENABLE_SW_FALLBACK_REPORTS        0

#define TRACE_EXA_ERROR(fmt, ...) do {                              \
        if (ENABLE_EXA_TRACE)                                       \
            ErrorF("EXA: " fmt"\n", ##__VA_ARGS__);                 \
    } while (0)

#define TRACE_EXA(fmt, ...) do {                                    \
        if (ENABLE_EXA_TRACE)                                       \
            ErrorF("EXA: " fmt"\n", ##__VA_ARGS__);                 \
    } while (0)

#define EXA_FAIL_IF(cond) do {                                      \
        if (cond) {                                                 \
            if (ENABLE_SW_FALLBACK_REPORTS) {                       \
                ErrorF("%s:%d FALLBACK: " #cond"\n",__FUNCTION__,__LINE__);\
            }                                                       \
            return FALSE;                                           \
        }                                                           \
    } while (0)

    /************************************************************************
     * G2d EXA driver (START)
     ************************************************************************/
    Bool
    G2dPrepareCopy(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap,
            int xdir, int ydir, int alu, Pixel planemask);

    void
    G2dCopy(PixmapPtr pDstPixmap, int srcX, int srcY, int dstX, int dstY, int width, int height);

    void
    G2dDoneCopy(PixmapPtr pDstPixmap);

    Bool
    G2dPrepareSolid(PixmapPtr pPixmap, int alu, Pixel planemask, Pixel fg);

    void
    G2dSolid(PixmapPtr pPixmap, int x1, int y1, int x2, int y2);

    void
    G2dDoneSolid(PixmapPtr pPixmap);

    Bool
    G2dCheckComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture, PicturePtr pDstPicture);
    Bool
    G2dPrepareComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture, PicturePtr pDstPicture,
            PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst);
    void
    G2dComposite(PixmapPtr pDst, int srcX, int srcY, int maskX, int maskY,
            int dstX, int dstY, int width, int height);
    void
    G2dDoneComposite(PixmapPtr pDst);

    Bool
    G2dVivPrepareAccess(PixmapPtr pPix, int index);

    void
    G2dVivFinishAccess(PixmapPtr pPix, int index);

    void *
    G2dVivCreatePixmap(ScreenPtr pScreen, int size, int align);

    void
    G2dVivDestroyPixmap(ScreenPtr pScreen, void *dPriv);

    Bool
    G2dVivPixmapIsOffscreen(PixmapPtr pPixmap);

    Bool
    G2dVivModifyPixmapHeader(PixmapPtr pPixmap, int width, int height,
            int depth, int bitsPerPixel, int devKind,
            pointer pPixData);
    Bool
    G2dUploadToScreen(PixmapPtr pDst, int x, int y, int w, int h, char *src, int src_pitch);

    void
    G2dEXASync(ScreenPtr pScreen, int marker);
    /************************************************************************
     * G2d EXA driver(FINISH)
     ************************************************************************/
#ifdef __cplusplus
}
#endif

#endif  /* VIVANTE_EXA_G2D_H */
