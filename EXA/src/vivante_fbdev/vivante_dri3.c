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


/*
 * Copyright © 2013 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifdef ENABLE_VIVANTE_DRI3

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* xorg includes */
#include "xorg-server.h"

#include "xf86.h"
#include "dri3.h"
#include "misyncshm.h"
#include <xf86drm.h>

#include "vivante_common.h"
#include "vivante.h"
#include "vivante_priv.h"

static int vivante_open_node(ScreenPtr pScreen, int *out)
{
    ScrnInfoPtr pScrn = GET_PSCR(pScreen);
    drm_magic_t magic;
    int fd;

    fd = drmOpenWithType("vivante", NULL, DRM_NODE_RENDER);
    if (fd < 0)
            return BadAlloc;

    *out = fd;
    return Success;
}

static int vivante_dri3_open(ScreenPtr pScreen, RRProviderPtr provider, int *out)
{
    return vivante_open_node(pScreen, out);
}

static PixmapPtr vivante_dri3_pixmap_from_fd(ScreenPtr pScreen, int fd,
    CARD16 width, CARD16 height, CARD16 stride, CARD8 depth, CARD8 bpp)
{
    PixmapPtr pixmap;
    Viv2DPixmapPtr pVivPix;

    if (depth < 8)
        return NULL;

    switch (bpp) {
        case 8:
        case 16:
        case 32:
            break;
        default:
            return NULL;
    }

    pixmap = pScreen->CreatePixmap(pScreen, 0, 0, depth, 0);
    if (!pixmap)
        return NULL;

    pVivPix = exaGetPixmapDriverPrivate(pixmap);

    pVivPix->fdToPixmap= fd;

    if (!pScreen->ModifyPixmapHeader(pixmap, width, height, 0, bpp, stride,
                NULL)) {
        goto free_pixmap;
    }
    return pixmap;
free_pixmap:
    fbDestroyPixmap(pixmap);
    return NULL;
}

static int vivante_dri3_fd_from_pixmap(ScreenPtr pScreen, PixmapPtr pPixmap,
    CARD16 *stride, CARD32 *size)
{
    int fd;
    GenericSurfacePtr surf = NULL;
    ScrnInfoPtr pScrn = GET_PSCR(pScreen);
    Viv2DPixmapPtr pVivPix = exaGetPixmapDriverPrivate(pPixmap);

    if (!pVivPix || !pVivPix->mVidMemInfo ) {
        return BadMatch;
    }
    surf = (GenericSurfacePtr)pVivPix->mVidMemInfo;

    if (pPixmap->devKind > UINT16_MAX)
        return -1;

    fd = dup(surf->fd);
    if( fd < 0) {
        return BadMatch;
    }
    *stride = (CARD16)surf->mStride;
    *size = surf->mVideoNode.mBytes;
    return fd;
}

static dri3_screen_info_rec vivante_dri3_info = {
    .version = DRI3_SCREEN_INFO_VERSION,
    .open = vivante_dri3_open,
    .pixmap_from_fd = vivante_dri3_pixmap_from_fd,
    .fd_from_pixmap = vivante_dri3_fd_from_pixmap,
};

Bool vivanteDRI3ScreenInit(ScreenPtr pScreen){
    ScrnInfoPtr pScrn = GET_PSCR(pScreen);
    int fd;
    drmVersionPtr version;
    VivPtr fPtr = GET_VIV_PTR(pScrn);
    VIVGPUPtr gpuctx = (VIVGPUPtr)fPtr->mGrCtx.mGpu;

    fd = drmOpenWithType("vivante", NULL, DRM_NODE_RENDER);
    if (drm_vivante_create(fd, &gpuctx->mDriver->drm) != 0)
    {
        xf86DrvMsg(0, X_ERROR, "drm_vivante_create() failed\n");
    }

    version = drmGetVersion(fd);
    if (version) {
        xf86DrvMsg(0,X_INFO,"Version: %d.%d.%d\n", version->version_major, version->version_minor, version->version_patchlevel);
        xf86DrvMsg(0,X_INFO,"  Name: %s\n", version->name);
        xf86DrvMsg(0,X_INFO,"  Date: %s\n", version->date);
        xf86DrvMsg(0,X_INFO,"  Description: %s\n", version->desc);
        drmFreeVersion(version);
   }

    if (!miSyncShmScreenInit(pScreen))
                return FALSE;

    return dri3_screen_init(pScreen, &vivante_dri3_info);
}

void vivanteDRI3ScreenDeInit(ScreenPtr pScreen){
    ScrnInfoPtr pScrn = GET_PSCR(pScreen);
    VivPtr fPtr = GET_VIV_PTR(pScrn);
    VIVGPUPtr gpuctx = (VIVGPUPtr)fPtr->mGrCtx.mGpu;

    if (gpuctx->mDriver->drm)
        drm_vivante_close(gpuctx->mDriver->drm);

    gpuctx->mDriver->drm = (struct drm_vivante *)NULL;
}


#endif
