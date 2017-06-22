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

#ifdef HAVE_DRI3

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* xorg includes */
#include "xorg-server.h"

#include "xf86.h"
#include "dri3.h"
#include "compat-api.h"
#include "misyncshm.h"
#include <xf86drm.h>

#include "vivante_common.h"
#include "vivante.h"
#include "vivante_priv.h"
#include "imx_g2d_dri3.h"

#include "g2d.h"

static int imx_g2d_open_node(ScreenPtr pScreen, int *out)
{
    ScrnInfoPtr pScrn = GET_PSCR(pScreen);
    VivPtr fPtr = GET_VIV_PTR(pScrn);
    drm_magic_t magic;
    int fd;

    fd = open(fPtr->device_name, O_RDWR | O_CLOEXEC);
    if (fd < 0)
            return BadAlloc;

    if (drmGetMagic(fd, &magic) < 0) {
        if (errno == EACCES) {
            *out = fd;
            return Success;
        } else {
            close(fd);
            return BadMatch;
        }
    }

    if (drmAuthMagic(fPtr->fd, magic) < 0) {
            close(fd);
            return BadMatch;
    }

    *out = fd;
    return Success;
}

static int imx_g2d_dri3_open(ScreenPtr pScreen, RRProviderPtr provider, int *out)
{
	return imx_g2d_open_node(pScreen, out);
}

static PixmapPtr imx_g2d_dri3_pixmap_from_fd(ScreenPtr pScreen, int fd,
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

    pVivPix->pixmapfd = fd;

    if (!pScreen->ModifyPixmapHeader(pixmap, width, height, 0, bpp, stride,
                NULL)) {
        goto free_pixmap;
    }
    return pixmap;
free_pixmap:
	fbDestroyPixmap(pixmap);
	return NULL;
}

static int imx_g2d_dri3_fd_from_pixmap(ScreenPtr pScreen, PixmapPtr pPixmap,
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

	fd = g2d_buf_export_fd(surf->g2dbuf);
    if( fd < 0) {
        return BadMatch;
    }
	*stride = pPixmap->devKind;
	*size = surf->g2dbuf->buf_size;
	return fd;
}

static dri3_screen_info_rec imx_g2d_dri3_info = {
	.version = 0,
	.open = imx_g2d_dri3_open,
	.pixmap_from_fd = imx_g2d_dri3_pixmap_from_fd,
	.fd_from_pixmap = imx_g2d_dri3_fd_from_pixmap,
};

Bool imxG2dDRI3ScreenInit(ScreenPtr pScreen){
    ScrnInfoPtr pScrn = GET_PSCR(pScreen);
    VivPtr fPtr = GET_VIV_PTR(pScrn);
    fPtr->device_name = drmGetDeviceNameFromFd(fPtr->fd);

    if (!miSyncShmScreenInit(pScreen))
                return FALSE;

	return dri3_screen_init(pScreen, &imx_g2d_dri3_info);
}

#endif