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


#ifndef VIVANTE_EXA_NULL_H
#define VIVANTE_EXA_NULL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vivante_common.h"

    /************************************************************************
     * NULL EXA (START)
     ************************************************************************/
    Bool
    VivPrepareCopyFail(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap,
            int xdir, int ydir, int alu, Pixel planemask);

    Bool
    VivPrepareSolidFail(PixmapPtr pPixmap, int alu, Pixel planemask, Pixel fg);

    Bool
    VivCheckCompositeFail(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture, PicturePtr pDstPicture);

    Bool
    VivPrepareCompositeFail(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture, PicturePtr pDstPicture,
            PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst);
    /************************************************************************
     * NULL EXA (FINISH)
     ************************************************************************/
    

#ifdef __cplusplus
}
#endif

#endif  /* VIVANTE_EXA_NULL_H */

