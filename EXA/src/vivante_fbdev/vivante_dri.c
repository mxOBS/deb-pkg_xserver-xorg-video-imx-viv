/****************************************************************************
*
*    Copyright 2012 - 2018 Vivante Corporation, Santa Clara, California.
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


#ifndef DISABLE_VIVANTE_DRI

#include "vivante_common.h"
#include "vivante.h"
#include "vivante_dri.h"

#define VIV_PAGE_SHIFT      (12)
#define VIV_PAGE_SIZE       (1UL << VIV_PAGE_SHIFT)
#define VIV_PAGE_MASK       (~(VIV_PAGE_SIZE - 1))
#define VIV_PAGE_ALIGN(val) (((val) + (VIV_PAGE_SIZE -1)) & (VIV_PAGE_MASK))

static char VivKernelDriverName[] = "vivante";
static char VivClientDriverName[] = "vivante";

static Bool
VivCreateContext(ScreenPtr pScreen, VisualPtr visual,
        drm_context_t hwContext, void *pVisualConfigPriv,
        DRIContextType contextStore) {
    return TRUE;
}

static void
VivDestroyContext(ScreenPtr pScreen, drm_context_t hwContext,
        DRIContextType contextStore) {
}

Bool
VivDRIFinishScreenInit(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = GET_PSCR(pScreen);
    VivPtr pViv = GET_VIV_PTR(pScrn);
    DRIInfoPtr pDRIInfo = (DRIInfoPtr) pViv->pDRIInfo;

    pDRIInfo->driverSwapMethod = DRI_HIDE_X_CONTEXT;

    DRIFinishScreenInit(pScreen);

    return TRUE;
}

static void
VivDRISwapContext(ScreenPtr pScreen, DRISyncType syncType,
        DRIContextType oldContextType, void *oldContext,
        DRIContextType newContextType, void *newContext) {
    return;
}

static void
VivDRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 index) {
    return;
}

static void
VivDRIMoveBuffers(WindowPtr pParent, DDXPointRec ptOldOrg,
        RegionPtr prgnSrc, CARD32 index) {
    return;
}

Bool VivDRIScreenInit(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = GET_PSCR(pScreen);
    DRIInfoPtr pDRIInfo;
    int fd = -1;
    VivPtr pViv = GET_VIV_PTR(pScrn);

    /* Check that DRI, and DRM modules have been loaded by testing
     * for canonical symbols in each module. */
    if (!xf86LoaderCheckSymbol("DRIScreenInit")) return FALSE;
    if (!xf86LoaderCheckSymbol("DRIQueryVersion")) {
        xf86DrvMsg(pScreen->myNum, X_ERROR,
                "[dri] VivDRIScreenInit failed (libdri.a too old)\n");
        return FALSE;
    }

    /* Check the DRI version */
    {
        int major, minor, patch;
        DRIQueryVersion(&major, &minor, &patch);
        if (major < 4 || minor < 0) {
            xf86DrvMsg(pScreen->myNum, X_ERROR,
                    "[dri] VivDRIScreenInit failed because of a version mismatch.\n"
                    "[dri] libDRI version is %d.%d.%d but version 4.0.x is needed.\n"
                    "[dri] Disabling DRI.\n",
                    major, minor, patch);
            return FALSE;
        }
    }


    pDRIInfo = DRICreateInfoRec();

    if (!pDRIInfo) return FALSE;

    pViv->pDRIInfo = pDRIInfo;
    pDRIInfo->drmDriverName=VivKernelDriverName;
    pDRIInfo->clientDriverName=VivClientDriverName;
    pDRIInfo->busIdString =(char *)malloc(sizeof(char) * 128);
    memset(pDRIInfo->busIdString,0, sizeof(char) * 128);
    /* use = to copy string and it seems good, but when you free it, it will report invalid pointer, use strcpy instead */
    //pDRIInfo->busIdString="platform:Vivante GCCore";
    strcpy(pDRIInfo->busIdString,"platform:Vivante GCCore");

    /* Try to open it by platform:Vivante GCCore, if it fails, open with platform:Vivante GCCore:00 */
    fd = drmOpen(NULL,pDRIInfo->busIdString);
    if ( fd < 0)
    {
        strcpy(pDRIInfo->busIdString,"platform:Vivante GCCore:00");
        fd = drmOpen(NULL,pDRIInfo->busIdString);
        if (fd < 0)
        {
            xf86DrvMsg(pScreen->myNum, X_ERROR,"[dri] VivDRIScreenInit failed because Drm can't be opened.\n");
            return FALSE;
        }
        drmClose(fd);
    } else {
        /* fd > 0 means passing verification of the name, close fd*/
        drmClose(fd);
    }

    pDRIInfo->ddxDriverMajorVersion = VIV_DRI_VERSION_MAJOR;
    pDRIInfo->ddxDriverMinorVersion = VIV_DRI_VERSION_MINOR;
    pDRIInfo->ddxDriverPatchVersion = 0;
    pDRIInfo->frameBufferPhysicalAddress =(pointer)(pScrn->memPhysBase + pViv->mFB.mFBOffset);
    pDRIInfo->frameBufferSize = pScrn->videoRam;

    pDRIInfo->frameBufferStride = (pScrn->displayWidth *
            pScrn->bitsPerPixel / 8);

    pDRIInfo->maxDrawableTableEntry = VIV_MAX_DRAWABLES;

    pDRIInfo->SAREASize = SAREA_MAX;

    pDRIInfo->devPrivate = NULL;
    pDRIInfo->devPrivateSize = 0;
    pDRIInfo->contextSize = 1024;

    pDRIInfo->CreateContext = VivCreateContext;
    pDRIInfo->DestroyContext = VivDestroyContext;
    pDRIInfo->SwapContext = VivDRISwapContext;
    pDRIInfo->InitBuffers = VivDRIInitBuffers;
    pDRIInfo->MoveBuffers = VivDRIMoveBuffers;
    pDRIInfo->bufferRequests = DRI_ALL_WINDOWS;

    /*
     ** drmAddMap required the base and size of target buffer to be page aligned.
     ** While frameBufferSize sometime doesn't, so we force it aligned here, and
     ** restore it back after DRIScreenInit
     */
    pDRIInfo->frameBufferSize = VIV_PAGE_ALIGN(pDRIInfo->frameBufferSize);
    if (!DRIScreenInit(pScreen, pDRIInfo, &pViv->drmSubFD)) {
        xf86DrvMsg(pScreen->myNum, X_ERROR,
                "[dri] DRIScreenInit failed.  Disabling DRI.\n");
        DRIDestroyInfoRec(pViv->pDRIInfo);
        pViv->pDRIInfo = NULL;
        return FALSE;
    }
    pDRIInfo->frameBufferSize = pScrn->videoRam;

    /* Check the Vivante DRM version */
    {
        drmVersionPtr version = drmGetVersion(pViv->drmSubFD);
        if (version) {
            if (version->version_major != 1 ||
                    version->version_minor < 0) {
                /* incompatible drm version */
                xf86DrvMsg(pScreen->myNum, X_ERROR,
                        "[dri] VIVDRIScreenInit failed because of a version mismatch.\n"
                        "[dri] vivante.o kernel module version is %d.%d.%d but version 1.0.x is needed.\n"
                        "[dri] Disabling the DRI.\n",
                        version->version_major,
                        version->version_minor,
                        version->version_patchlevel);
                VivDRICloseScreen(pScreen);
                drmFreeVersion(version);
                return FALSE;
            }
        }
        drmFreeVersion(version);
    }

    return TRUE;
}

void VivDRICloseScreen(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = GET_PSCR(pScreen);
    VivPtr pViv = GET_VIV_PTR(pScrn);

    if (pViv->pDRIInfo) {
        DRICloseScreen(pScreen);
        DRIDestroyInfoRec(pViv->pDRIInfo);
        pViv->pDRIInfo = 0;
    }
}

void VivUpdateDriScreen(ScrnInfoPtr pScrn) {
    VivPtr pViv = GET_VIV_PTR(pScrn);
    DRIInfoPtr pDRIInfo = (DRIInfoPtr) pViv->pDRIInfo;
    if(pDRIInfo == NULL)
        return;

    pDRIInfo->frameBufferSize = pScrn->videoRam; // FIXME!
    pDRIInfo->frameBufferStride = (pScrn->displayWidth *
            pScrn->bitsPerPixel / 8);
}

#endif
