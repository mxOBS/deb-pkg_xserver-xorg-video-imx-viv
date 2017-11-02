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
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2011 Dave Airlie
 * Copyright 2017 NXP
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * Original Author: Alan Hourihane <alanh@tungstengraphics.com>
 * Rewrite: Dave Airlie <airlied@redhat.com>
 *
 */

#include "vivante_common.h"
#include "vivante.h"
#include "vivante_exa_g2d.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xorg-server.h"
#include <unistd.h>
#include <fcntl.h>
#include "compat-api.h"

#include "xf86.h"
#include "xf86_OSproc.h"
#include "compiler.h"
#include "xf86Pci.h"
#include "mipointer.h"
#include "micmap.h"
#include <X11/extensions/randr.h>
#include "fb.h"
#include "edid.h"
#include "xf86i2c.h"
#include "xf86Crtc.h"
#include "miscstruct.h"
#include "dixstruct.h"

#include "xorg/shadow.h"
#include "shadow.h"
#include "xf86xv.h"


#ifdef XSERVER_PLATFORM_BUS
#include "xf86platformBus.h"
#endif
#ifdef XSERVER_LIBPCIACCESS
#include <pciaccess.h>
#endif

#include "imx_drm_driver.h"

#include <errno.h>
#include <drm.h>
#include <xf86drm.h>
#include <damage.h>
#include "drmmode_display.h"

#define DRV_ERROR(msg)    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, msg);

#define VIVANTE_DRIVER_NAME       "vivante"

extern  Bool VivPrepareCopyFail(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap,int xdir, int ydir, int alu, Pixel planemask);
extern  Bool VivPrepareSolidFail(PixmapPtr pPixmap, int alu, Pixel planemask, Pixel fg);
extern Bool VivCheckCompositeFail(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture, PicturePtr pDstPicture);
extern Bool VivPrepareCompositeFail(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture, PicturePtr pDstPicture,PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst);

static void AdjustFrame(ADJUST_FRAME_ARGS_DECL);
static Bool CloseScreen(CLOSE_SCREEN_ARGS_DECL);
static Bool EnterVT(VT_FUNC_ARGS_DECL);
static void Identify(int flags);
static const OptionInfoRec *AvailableOptions(int chipid, int busid);
static ModeStatus ValidMode(SCRN_ARG_TYPE arg, DisplayModePtr mode, Bool verbose,
    int flags);
static void FreeScreen(FREE_SCREEN_ARGS_DECL);
static void LeaveVT(VT_FUNC_ARGS_DECL);
static Bool SwitchMode(SWITCH_MODE_ARGS_DECL);
static Bool ScreenInit(SCREEN_INIT_ARGS_DECL);
static Bool PreInit(ScrnInfoPtr pScrn, int flags);

static Bool Probe(DriverPtr drv, int flags);
static Bool ms_pci_probe(DriverPtr driver,
    int entity_num, struct pci_device *device,
    intptr_t match_data);
static Bool ms_driver_func(ScrnInfoPtr scrn, xorgDriverFuncOp op,
    void *data);

#ifdef XSERVER_LIBPCIACCESS
static const struct pci_id_match viv_device_match[] = {
    {
    PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY,
    0x00030000, 0x00ffffff, 0
    },

    { 0, 0, 0 },
};
#endif

#ifdef XSERVER_PLATFORM_BUS
static Bool ms_platform_probe(DriverPtr driver,
    int entity_num, int flags, struct xf86_platform_device *device,
    intptr_t match_data);
#endif

static const OptionInfoRec Options[] = {
    {OPTION_SW_CURSOR, "SWcursor", OPTV_BOOLEAN, {0}, FALSE},
    {OPTION_DEVICE_PATH, "kmsdev", OPTV_STRING, {0}, FALSE },
    {OPTION_SHADOW_FB, "ShadowFB", OPTV_BOOLEAN, {0}, FALSE },
    {-1, NULL, OPTV_NONE, {0}, FALSE}
};

int modesettingEntityIndex = -1;


extern Bool VIV2DGPUCtxInit(GALINFOPTR galInfo);

static Bool InitExaLayer(ScreenPtr pScreen) {
    ExaDriverPtr pExa;
    ScrnInfoPtr pScrn = GET_PSCR(pScreen);
    VivPtr pViv = GET_VIV_PTR(pScrn);

    TRACE_ENTER();

    xf86DrvMsg(pScreen->myNum, X_INFO, "test Initializing EXA\n");
    /*Initing EXA*/
    pExa = exaDriverAlloc();
    if (!pExa) {
        TRACE_ERROR("Unable to allocate exa driver");
        pViv->mFakeExa.mNoAccelFlag = TRUE;
        TRACE_EXIT(FALSE);
    }
    pViv->mFakeExa.mExaDriver = pExa;
    /*Exa version*/
    pExa->exa_major = EXA_VERSION_MAJOR;
    pExa->exa_minor = EXA_VERSION_MINOR;

    /* 15 bit coordinates */
    pExa->maxX = VIV_MAX_WIDTH;
    pExa->maxY = VIV_MAX_HEIGHT;


    if (!VIV2DGPUCtxInit(&pViv->mGrCtx)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "internal error: GPU Ctx Init Failed\n");
        TRACE_EXIT(FALSE);
    }

    /*flags*/
    pExa->flags = EXA_HANDLES_PIXMAPS | EXA_SUPPORTS_PREPARE_AUX | EXA_OFFSCREEN_PIXMAPS;
    /*Subject to change*/
    pExa->pixmapOffsetAlign = 8;
    /*This is for sure*/
    pExa->pixmapPitchAlign = PIXMAP_PITCH_ALIGN;
#ifdef HAVE_G2D

    if(pViv->mGrCtx.mExaHwType == IMXG2D)
    {
        pExa->WaitMarker = G2dEXASync;

        pExa->PrepareSolid = pViv->mFakeExa.mNoAccelFlag?
                             VivPrepareSolidFail:G2dPrepareSolid;
        pExa->Solid = G2dSolid;
        pExa->DoneSolid = G2dDoneSolid;

        pExa->PrepareCopy = pViv->mFakeExa.mNoAccelFlag?
                            VivPrepareCopyFail:G2dPrepareCopy;
        pExa->Copy = G2dCopy;
        pExa->DoneCopy = G2dDoneCopy;

        pExa->UploadToScreen = pViv->mFakeExa.mNoAccelFlag?
                               NULL:G2dUploadToScreen;

        pExa->CheckComposite = pViv->mFakeExa.mNoAccelFlag?
                               VivCheckCompositeFail:G2dCheckComposite;
        pExa->PrepareComposite = pViv->mFakeExa.mNoAccelFlag?
                                 VivPrepareCompositeFail:G2dPrepareComposite;
        pExa->Composite = G2dComposite;
        pExa->DoneComposite = G2dDoneComposite;

        pExa->CreatePixmap = G2dVivCreatePixmap;
        pExa->DestroyPixmap = G2dVivDestroyPixmap;
        pExa->ModifyPixmapHeader = G2dVivModifyPixmapHeader;
        pExa->PixmapIsOffscreen = G2dVivPixmapIsOffscreen;
        pExa->PrepareAccess = G2dVivPrepareAccess;
        pExa->FinishAccess = G2dVivFinishAccess;
    }
    else
#endif
    {
    }
    if (!exaDriverInit(pScreen, pExa)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "exaDriverinit failed.\n");
        TRACE_EXIT(FALSE);
    }
    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "***********************1 InitExaLayer end\n");
    pViv->mFakeExa.mIsInited = TRUE;
    TRACE_EXIT(TRUE);
}

extern Bool VIV2DGPUCtxDeInit(GALINFOPTR galInfo);
static Bool DestroyExaLayer(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = GET_PSCR(pScreen);
    VivPtr pViv = GET_VIV_PTR(pScrn);
    TRACE_ENTER();
    xf86DrvMsg(pScreen->myNum, X_INFO, "InitExaLayer end\n");

    exaDriverFini(pScreen);

    if (!VIV2DGPUCtxDeInit(&pViv->mGrCtx)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "internal error: GPU Ctx DeInit Failed\n");
        TRACE_EXIT(FALSE);
    }

    TRACE_EXIT(TRUE);
}

int ms_entity_index = -1;

static void
ms_setup_entity(ScrnInfoPtr scrn, int entity_num)
{
    DevUnion *pPriv;

    xf86SetEntitySharable(entity_num);

    if (ms_entity_index == -1)
        ms_entity_index = xf86AllocateEntityPrivateIndex();

    pPriv = xf86GetEntityPrivate(entity_num,
                                 ms_entity_index);

    xf86SetEntityInstanceForScreen(scrn, entity_num, xf86GetNumEntityInstances(entity_num) - 1);

    if (!pPriv->ptr)
        pPriv->ptr = xnfcalloc(sizeof(modesettingEntRec), 1);
}
modesettingEntPtr ms_ent_priv(ScrnInfoPtr scrn)
{
    DevUnion     *pPriv;
    VivPtr fPtr = GET_VIV_PTR(scrn);
    pPriv = xf86GetEntityPrivate(fPtr->pEnt->index,
                                 ms_entity_index);
    return pPriv->ptr;
}

static int open_hw(const char *dev)
{
    int fd;
    if (dev)
        fd = open(dev, O_RDWR, 0);
    else {
        dev = getenv("KMSDEVICE");
        if ((NULL == dev) || ((fd = open(dev, O_RDWR, 0)) == -1)) {
            dev = "/dev/dri/card0";
            fd = open(dev, O_RDWR, 0);
        }
    }
    if (fd == -1)
        xf86DrvMsg(-1, X_ERROR, "open %s: %s\n", dev, strerror(errno));

    return fd;
}

static int
check_outputs(int fd, int *count)
{
    drmModeResPtr res = drmModeGetResources(fd);
    int ret;

    if (!res)
        return FALSE;

    if (count)
        *count = res->count_connectors;

    ret = res->count_connectors > 0;
#if defined(DRM_CAP_PRIME) && defined(GLAMOR_HAS_GBM_LINEAR)
    if (ret == FALSE) {
        uint64_t value = 0;
        if (drmGetCap(fd, DRM_CAP_PRIME, &value) == 0 &&
                (value & DRM_PRIME_CAP_EXPORT))
            ret = TRUE;
    }
#endif

    drmModeFreeResources(res);
    return ret;
}

static Bool probe_hw(const char *dev, struct xf86_platform_device *platform_dev)
{
    int fd;

#if XF86_PDEV_SERVER_FD
    if (platform_dev && (platform_dev->flags & XF86_PDEV_SERVER_FD)) {
        fd = xf86_platform_device_odev_attributes(platform_dev)->fd;
        if (fd == -1)
            return FALSE;
        return check_outputs(fd, NULL);
    }
#endif

    fd = open_hw(dev);
    if (fd != -1) {
        int ret = check_outputs(fd, NULL);
        close(fd);
        return ret;
    }
    return FALSE;
}

static char *
ms_DRICreatePCIBusID(const struct pci_device *dev)
{
    char *busID;

    if (asprintf(&busID, "pci:%04x:%02x:%02x.%d", dev->domain, dev->bus, dev->dev, dev->func) == -1)
        return NULL;

    return busID;
}


static Bool probe_hw_pci(const char *dev, struct pci_device *pdev)
{
    int ret = FALSE, fd = open_hw(dev);
    char *id, *devid;
    drmSetVersion sv;

    if (fd == -1)
        return FALSE;

    sv.drm_di_major = 1;
    sv.drm_di_minor = 4;
    sv.drm_dd_major = -1;
    sv.drm_dd_minor = -1;
    if (drmSetInterfaceVersion(fd, &sv)) {
        close(fd);
        return FALSE;
    }


    id = drmGetBusid(fd);
    devid = ms_DRICreatePCIBusID(pdev);

    if (id && devid && !strcmp(id, devid))
        ret = check_outputs(fd, NULL);

    close(fd);
    free(id);
    free(devid);
    return ret;
}
static Bool
ms_driver_func(ScrnInfoPtr scrn, xorgDriverFuncOp op, void *data)
{
    xorgHWFlags *flag;

    switch (op) {
    case GET_REQUIRED_HW_INTERFACES:
        flag = (CARD32 *)data;
        (*flag) = 0;
        return TRUE;
#if XORG_VERSION_CURRENT >= XORG_VERSION_NUMERIC(1,15,99,902,0)
    case SUPPORTS_SERVER_FDS:
        return TRUE;
#endif
    default:
        return FALSE;
    }
}

#if XSERVER_LIBPCIACCESS
static Bool
ms_pci_probe(DriverPtr driver,
    int entity_num, struct pci_device *dev, intptr_t match_data)
{
    ScrnInfoPtr scrn = NULL;

    scrn = xf86ConfigPciEntity(scrn, 0, entity_num, NULL,
        NULL, NULL, NULL, NULL, NULL);
    if (scrn) {
        const char *devpath;
        GDevPtr devSection = xf86GetDevFromEntity(scrn->entityList[0],
            scrn->entityInstanceList[0]);

        devpath = xf86FindOptionValue(devSection->options, "kmsdev");
        if (probe_hw_pci(devpath, dev)) {
            scrn->driverVersion = 1;
            scrn->driverName = VIVANTE_DRIVER_NAME;
            scrn->name = VIVANTE_DRIVER_NAME;
            scrn->Probe = NULL;
            scrn->PreInit = PreInit;
            scrn->ScreenInit = ScreenInit;
            scrn->SwitchMode = SwitchMode;
            scrn->AdjustFrame = AdjustFrame;
            scrn->EnterVT = EnterVT;
            scrn->LeaveVT = LeaveVT;
            scrn->FreeScreen = FreeScreen;
            scrn->ValidMode = ValidMode;

            xf86DrvMsg(scrn->scrnIndex, X_CONFIG,
                "claimed PCI slot %d@%d:%d:%d\n",
                dev->bus, dev->domain, dev->dev, dev->func);
            xf86DrvMsg(scrn->scrnIndex, X_INFO,
                "using %s\n", devpath ? devpath : "default device");

            ms_setup_entity(scrn, entity_num);
        }
        else
            scrn = NULL;
    }
    return scrn != NULL;
}
#endif

#ifdef XSERVER_PLATFORM_BUS
static Bool
ms_platform_probe(DriverPtr driver,
    int entity_num, int flags, struct xf86_platform_device *dev, intptr_t match_data)
{
    ScrnInfoPtr scrn = NULL;
    const char *path = xf86_platform_device_odev_attributes(dev)->path;
    int scr_flags = 0;

    if (flags & PLATFORM_PROBE_GPU_SCREEN)
        scr_flags = XF86_ALLOCATE_GPU_SCREEN;

    if (probe_hw(path, dev)) {
        scrn = xf86AllocateScreen(driver, scr_flags);
        xf86AddEntityToScreen(scrn, entity_num);
        scrn->driverName = VIVANTE_DRIVER_NAME;
        scrn->name = VIVANTE_DRIVER_NAME;
        scrn->PreInit = PreInit;
        scrn->ScreenInit = ScreenInit;
        scrn->SwitchMode = SwitchMode;
        scrn->AdjustFrame = AdjustFrame;
        scrn->EnterVT = EnterVT;
        scrn->LeaveVT = LeaveVT;
        scrn->FreeScreen = FreeScreen;
        scrn->ValidMode = ValidMode;
        xf86DrvMsg(scrn->scrnIndex, X_INFO,
            "using drv %s\n", path ? path : "default device");
            ms_setup_entity(scrn, entity_num);
    }

    return scrn != NULL;
}
#endif

static Bool
Probe(DriverPtr drv, int flags)
{
    int i, numDevSections;
    GDevPtr *devSections;
    Bool foundScreen = FALSE;
    const char *dev;
    ScrnInfoPtr scrn = NULL;

    /* For now, just bail out for PROBE_DETECT. */
    if (flags & PROBE_DETECT)
        return FALSE;

    /*
     * Find the config file Device sections that match this
     * driver, and return if there are none.
     */
    if ((numDevSections = xf86MatchDevice(VIVANTE_DRIVER_NAME, &devSections)) <= 0) {
        xf86DrvMsg(0, X_INFO,
            "match devide fails\n");
        return FALSE;
    }

    for (i = 0; i < numDevSections; i++) {
        int entity_num;
        dev = xf86FindOptionValue(devSections[i]->options, "kmsdev");
        if (probe_hw(dev, NULL)) {
            entity_num = xf86ClaimFbSlot(drv, 0, devSections[i], TRUE);
            scrn = xf86ConfigFbEntity(scrn, 0, entity_num,
                NULL, NULL, NULL, NULL);
        }

        if (scrn) {

            foundScreen = TRUE;
            scrn->driverVersion = 1;
            scrn->driverName = VIVANTE_DRIVER_NAME;
            scrn->name = VIVANTE_DRIVER_NAME;
            scrn->Probe = Probe;
            scrn->PreInit = PreInit;
            scrn->ScreenInit = ScreenInit;
            scrn->SwitchMode = SwitchMode;
            scrn->AdjustFrame = AdjustFrame;
            scrn->EnterVT = EnterVT;
            scrn->LeaveVT = LeaveVT;
            scrn->FreeScreen = FreeScreen;
            scrn->ValidMode = ValidMode;
            ms_setup_entity(scrn, entity_num);
            xf86DrvMsg(scrn->scrnIndex, X_INFO,
                "using %s\n", dev ? dev : "default device");
        }
    }

    free(devSections);

    return foundScreen;
}

static Bool
GetRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate)
        return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(VivRec), 1);

    return TRUE;
}

static int dispatch_dirty_region(ScrnInfoPtr scrn,
    PixmapPtr pixmap,
    DamagePtr damage,
    int fb_id)
{
    VivPtr fPtr = GET_VIV_PTR(scrn);
    RegionPtr dirty = DamageRegion(damage);
    unsigned num_cliprects = REGION_NUM_RECTS(dirty);
    int ret = 0;

    if (num_cliprects) {
        drmModeClip *clip = xallocarray(num_cliprects, sizeof(drmModeClip));
        BoxPtr rect = REGION_RECTS(dirty);
        int i, ret;

        if (!clip)
            return -ENOMEM;

        /* XXX no need for copy? */
        for (i = 0; i < num_cliprects; i++, rect++) {
            clip[i].x1 = rect->x1;
            clip[i].y1 = rect->y1;
            clip[i].x2 = rect->x2;
            clip[i].y2 = rect->y2;
        }

        /* TODO query connector property to see if this is needed */
        ret = drmModeDirtyFB(fPtr->fd, fb_id, clip, num_cliprects);

        /* if we're swamping it with work, try one at a time */
        if (ret == -EINVAL) {
            for (i = 0; i < num_cliprects; i++) {
                if ((ret = drmModeDirtyFB(fPtr->fd, fb_id, &clip[i], 1)) < 0)
                    break;
            }
        }
        free(clip);
        DamageEmpty(damage);
        if (ret) {
            if (ret == -EINVAL)
                return ret;
        }
    }
    return ret;
}

static void dispatch_dirty(ScreenPtr pScreen)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);
    VivPtr fPtr = GET_VIV_PTR(scrn);

    PixmapPtr pixmap = pScreen->GetScreenPixmap(pScreen);
    int fb_id = fPtr->drmmode.fb_id;
    int ret;

    ret = dispatch_dirty_region(scrn, pixmap, fPtr->damage, fb_id);
    if (ret == -EINVAL || ret == -ENOSYS) {
        fPtr->dirty_enabled = FALSE;
        DamageUnregister(fPtr->damage);
        DamageDestroy(fPtr->damage);
        fPtr->damage = NULL;
        xf86DrvMsg(scrn->scrnIndex, X_INFO, "Disabling kernel dirty updates, not required.\n");
        return;
    }
}

#ifdef MODESETTING_OUTPUT_SLAVE_SUPPORT
static void dispatch_dirty_crtc(ScrnInfoPtr scrn, xf86CrtcPtr crtc)
{
    VivPtr fPtr = GET_VIV_PTR(scrn);

    PixmapPtr pixmap = crtc->randr_crtc->scanout_pixmap;
    msPixmapPrivPtr ppriv = msGetPixmapPriv(&fPtr->drmmode, pixmap);
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    DamagePtr damage = drmmode_crtc->slave_damage;
    int fb_id = ppriv->fb_id;
    int ret;

    ret = dispatch_dirty_region(scrn, pixmap, damage, fb_id);
    if (ret) {

    }
}

static void dispatch_slave_dirty(ScreenPtr pScreen)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    int c;

    for (c = 0; c < xf86_config->num_crtc; c++) {
        xf86CrtcPtr crtc = xf86_config->crtc[c];

        if (!crtc->randr_crtc)
            continue;
        if (!crtc->randr_crtc->scanout_pixmap)
            continue;

        dispatch_dirty_crtc(scrn, crtc);
    }
}
#endif

static void msBlockHandler(BLOCKHANDLER_ARGS_DECL)
{
    SCREEN_PTR(arg);

    VivPtr fPtr = GET_VIV_PTR(xf86ScreenToScrn(pScreen));

    pScreen->BlockHandler = fPtr->BlockHandler;
    pScreen->BlockHandler(BLOCKHANDLER_ARGS);
    pScreen->BlockHandler = msBlockHandler;
#ifdef MODESETTING_OUTPUT_SLAVE_SUPPORT
    if (pScreen->isGPU)
        dispatch_slave_dirty(pScreen);
    else
#endif
        if (fPtr->dirty_enabled)
            dispatch_dirty(pScreen);
}

static void
FreeRec(ScrnInfoPtr pScrn)
{
    VivPtr fPtr;

    if (!pScrn)
        return;

    fPtr = GET_VIV_PTR(pScrn);
    if (!fPtr)
        return;
    pScrn->driverPrivate = NULL;

    if (fPtr->fd > 0) {
        int ret;

        if (fPtr->pEnt->location.type == BUS_PCI)
            ret = drmClose(fPtr->fd);
        else
#ifdef XF86_PDEV_SERVER_FD
            if (!(fPtr->pEnt->location.type == BUS_PLATFORM &&
                (fPtr->pEnt->location.id.plat->flags & XF86_PDEV_SERVER_FD)))
#endif
                ret = close(fPtr->fd);
        (void)ret;
    }
    free(fPtr->Options);
    free(fPtr);

}

#ifndef DRM_CAP_CURSOR_WIDTH
#define DRM_CAP_CURSOR_WIDTH 0x8
#endif

#ifndef DRM_CAP_CURSOR_HEIGHT
#define DRM_CAP_CURSOR_HEIGHT 0x9
#endif

static Bool
ms_get_drm_master_fd(ScrnInfoPtr pScrn)
{
    EntityInfoPtr pEnt;
    VivPtr fPtr;
    modesettingEntPtr ms_ent;

    fPtr = GET_VIV_PTR(pScrn);
    ms_ent = ms_ent_priv(pScrn);

    pEnt = fPtr->pEnt;

    if (ms_ent->fd) {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                   " reusing fd for second head\n");
        fPtr->fd = ms_ent->fd;
        ms_ent->fd_ref++;
        return TRUE;
    }

#ifdef XSERVER_PLATFORM_BUS
    if (pEnt->location.type == BUS_PLATFORM) {
#ifdef XF86_PDEV_SERVER_FD
        if (pEnt->location.id.plat->flags & XF86_PDEV_SERVER_FD)
            fPtr->fd =
                xf86_platform_device_odev_attributes(pEnt->location.id.plat)->
                fd;
        else
#endif
        {
            char *path =
                xf86_platform_device_odev_attributes(pEnt->location.id.plat)->
                path;
            fPtr->fd = open_hw(path);
        }
    }
    else
#endif
#ifdef XSERVER_LIBPCIACCESS
    if (pEnt->location.type == BUS_PCI) {
        char *BusID = NULL;
        struct pci_device *PciInfo;

        PciInfo = xf86GetPciInfoForEntity(fPtr->pEnt->index);
        if (PciInfo) {
            BusID = XNFalloc(64);
            sprintf(BusID, "PCI:%d:%d:%d",
                    ((PciInfo->domain << 8) | PciInfo->bus),
                    PciInfo->dev, PciInfo->func);
        }
        fPtr->fd = drmOpen(NULL, BusID);
        free(BusID);
    }
    else
#endif
    {
        const char *devicename;
        devicename = xf86FindOptionValue(fPtr->pEnt->device->options, "kmsdev");
        fPtr->fd = open_hw(devicename);
    }
    if (fPtr->fd < 0)
        return FALSE;

    ms_ent->fd = fPtr->fd;
    ms_ent->fd_ref = 1;
    return TRUE;
}

static Bool
PreInit(ScrnInfoPtr pScrn, int flags)
{
    VivPtr fPtr;
    rgb defaultWeight = { 0, 0, 0 };
    EntityInfoPtr pEnt;
    EntPtr msEnt = NULL;
    char *BusID = NULL;
    const char *devicename;
    Bool prefer_shadow = TRUE;
    uint64_t value = 0;
    int ret;
    int bppflags, connector_count;
    int defaultdepth, defaultbpp;

    if (pScrn->numEntities != 1)
        return FALSE;

    pEnt = xf86GetEntityInfo(pScrn->entityList[0]);

    if (flags & PROBE_DETECT) {
        return FALSE;
    }

    /* Allocate driverPrivate */
    if (!GetRec(pScrn))
        return FALSE;

    fPtr = GET_VIV_PTR(pScrn);
    fPtr->SaveGeneration = -1;
    fPtr->pEnt = pEnt;

    pScrn->displayWidth = 640;           /* default it */

    /* Allocate an entity private if necessary */
    if (xf86IsEntityShared(pScrn->entityList[0])) {
        msEnt = xf86GetEntityPrivate(pScrn->entityList[0],
            modesettingEntityIndex)->ptr;
        fPtr->entityPrivate = msEnt;
    }
    else
        fPtr->entityPrivate = NULL;

    if (xf86IsEntityShared(pScrn->entityList[0])) {
        if (xf86IsPrimInitDone(pScrn->entityList[0])) {
            /* do something */
        }
        else {
            xf86SetPrimInitDone(pScrn->entityList[0]);
        }
    }

    pScrn->monitor = pScrn->confScreen->monitor;
    pScrn->progClock = TRUE;
    pScrn->rgbBits = 8;

#ifdef HAVE_G2D
      fPtr->mGrCtx.mExaHwType = IMXG2D;
#endif
    if (!ms_get_drm_master_fd(pScrn))
        return FALSE;
    fPtr->drmmode.fd = fPtr->fd;
    if (!check_outputs(fPtr->fd, &connector_count))
        return FALSE;

    drmmode_get_default_bpp(pScrn, &fPtr->drmmode, &defaultdepth, &defaultbpp);
    if (defaultdepth == 24 && defaultbpp == 24) {
        fPtr->drmmode.force_24_32 = TRUE;
        fPtr->drmmode.kbpp = 24;
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                   "Using 24bpp hw front buffer with 32bpp shadow\n");
        defaultbpp = 32;
    } else {
        fPtr->drmmode.kbpp = defaultbpp;
    }
    bppflags = PreferConvert24to32 | SupportConvert24to32 | Support32bppFb;

    if (!xf86SetDepthBpp
        (pScrn, defaultdepth, defaultdepth, defaultbpp, bppflags))
        return FALSE;

    switch (pScrn->depth) {
    case 15:
    case 16:
    case 24:
        break;
    default:
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "Given depth (%d) is not supported by the driver\n",
                   pScrn->depth);
        return FALSE;
    }
    xf86PrintDepthBpp(pScrn);

    /* Process the options */
    xf86CollectOptions(pScrn, NULL);
    if (!(fPtr->drmmode.Options = malloc(sizeof(Options))))
        return FALSE;
    memcpy(fPtr->drmmode.Options, Options, sizeof(Options));
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, fPtr->drmmode.Options);

    if (!xf86SetWeight(pScrn, defaultWeight, defaultWeight))
        return FALSE;
    if (!xf86SetDefaultVisual(pScrn, -1))
        return FALSE;

    if (xf86ReturnOptValBool(fPtr->drmmode.Options, OPTION_SW_CURSOR, FALSE)) {
        fPtr->drmmode.sw_cursor = TRUE;
    }

    fPtr->cursor_width = 64;
    fPtr->cursor_height = 64;
    ret = drmGetCap(fPtr->fd, DRM_CAP_CURSOR_WIDTH, &value);
    if (!ret) {
        fPtr->cursor_width = value;
    }
    ret = drmGetCap(fPtr->fd, DRM_CAP_CURSOR_HEIGHT, &value);
    if (!ret) {
        fPtr->cursor_height = value;
    }

    fPtr->drmmode.pageflip =
        xf86ReturnOptValBool(fPtr->drmmode.Options, OPTION_PAGEFLIP, TRUE);

    pScrn->capabilities = 0;
#ifdef DRM_CAP_PRIME
    ret = drmGetCap(fPtr->fd, DRM_CAP_PRIME, &value);
    if (ret == 0) {
        if (connector_count && (value & DRM_PRIME_CAP_IMPORT)) {
            pScrn->capabilities |= RR_Capability_SinkOutput;
            if (fPtr->drmmode.glamor)
                pScrn->capabilities |= RR_Capability_SinkOffload;
        }
#ifdef GLAMOR_HAS_GBM_LINEAR
        if (value & DRM_PRIME_CAP_EXPORT && fPtr->drmmode.glamor)
            pScrn->capabilities |= RR_Capability_SourceOutput | RR_Capability_SourceOffload;
#endif
    }
#endif

    if (drmmode_pre_init(pScrn, &fPtr->drmmode, pScrn->bitsPerPixel / 8) == FALSE) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "KMS setup failed\n");
        goto fail;
    }

    /*
     * If the driver can do gamma correction, it should call xf86SetGamma() here.
     */
    {
        Gamma zeros = { 0.0, 0.0, 0.0 };

        if (!xf86SetGamma(pScrn, zeros)) {
            return FALSE;
        }
    }

    if (!(pScrn->is_gpu && connector_count == 0) && pScrn->modes == NULL) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No modes.\n");
        return FALSE;
    }

    pScrn->currentMode = pScrn->modes;

    /* Set display resolution */
    xf86SetDpi(pScrn, 0, 0);

    /* Load the required sub modules */
    if (!xf86LoadSubModule(pScrn, "fb")) {
        return FALSE;
    }

    if (fPtr->drmmode.shadow_enable) {
        if (!xf86LoadSubModule(pScrn, "shadow")) {
            return FALSE;
        }
    }

    return TRUE;
fail:
        return FALSE;
}
static void *
msShadowWindow(ScreenPtr screen, CARD32 row, CARD32 offset, int mode,
               CARD32 *size, void *closure)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(pScrn);
    int stride;

    stride = (pScrn->displayWidth * ms->drmmode.kbpp) / 8;
    *size = stride;

    return ((uint8_t *) ms->drmmode.front_bo.dumb->ptr + row * stride + offset);
}


/* somewhat arbitrary tile size, in pixels */
#define TILE 16

static int
msUpdateIntersect(modesettingPtr ms, shadowBufPtr pBuf, BoxPtr box,
                  xRectangle *prect)
{
    int i, dirty = 0, stride = pBuf->pPixmap->devKind, cpp = ms->drmmode.cpp;
    int width = (box->x2 - box->x1) * cpp;
    unsigned char *old, *new;

    old = ms->drmmode.shadow_fb2;
    old += (box->y1 * stride) + (box->x1 * cpp);
    new = ms->drmmode.shadow_fb;
    new += (box->y1 * stride) + (box->x1 * cpp);

    for (i = box->y2 - box->y1 - 1; i >= 0; i--) {
        unsigned char *o = old + i * stride,
                      *n = new + i * stride;
        if (memcmp(o, n, width) != 0) {
            dirty = 1;
            memcpy(o, n, width);
        }
    }

    if (dirty) {
        prect->x = box->x1;
        prect->y = box->y1;
        prect->width = box->x2 - box->x1;
        prect->height = box->y2 - box->y1;
    }

    return dirty;
}

static void
msUpdatePacked(ScreenPtr pScreen, shadowBufPtr pBuf)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    Bool use_3224 = ms->drmmode.force_24_32 && pScrn->bitsPerPixel == 32;

    if (ms->drmmode.shadow_enable2 && ms->drmmode.shadow_fb2) do {
        RegionPtr damage = DamageRegion(pBuf->pDamage), tiles;
        BoxPtr extents = RegionExtents(damage);
        xRectangle *prect;
        int nrects;
        int i, j, tx1, tx2, ty1, ty2;

        tx1 = extents->x1 / TILE;
        tx2 = (extents->x2 + TILE - 1) / TILE;
        ty1 = extents->y1 / TILE;
        ty2 = (extents->y2 + TILE - 1) / TILE;

        nrects = (tx2 - tx1) * (ty2 - ty1);
        if (!(prect = calloc(nrects, sizeof(xRectangle))))
            break;

        nrects = 0;
        for (j = ty2 - 1; j >= ty1; j--) {
            for (i = tx2 - 1; i >= tx1; i--) {
                BoxRec box;

                box.x1 = max(i * TILE, extents->x1);
                box.y1 = max(j * TILE, extents->y1);
                box.x2 = min((i+1) * TILE, extents->x2);
                box.y2 = min((j+1) * TILE, extents->y2);

                if (RegionContainsRect(damage, &box) != rgnOUT) {
                    if (msUpdateIntersect(ms, pBuf, &box, prect + nrects)) {
                        nrects++;
                    }
                }
            }
        }

        tiles = RegionFromRects(nrects, prect, CT_NONE);
        RegionIntersect(damage, damage, tiles);
        RegionDestroy(tiles);
        free(prect);
    } while (0);

   shadowUpdatePacked(pScreen, pBuf);
}
#if XORG_VERSION_CURRENT >= XORG_VERSION_NUMERIC(1,19,0,0,0)
static Bool
msEnableSharedPixmapFlipping(RRCrtcPtr crtc, PixmapPtr front, PixmapPtr back)
{
    ScreenPtr screen = crtc->pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    EntityInfoPtr pEnt = ms->pEnt;
    xf86CrtcPtr xf86Crtc = crtc->devPrivate;

    if (!xf86Crtc)
        return FALSE;

    /* Not supported if we can't flip */
    if (!ms->drmmode.pageflip)
        return FALSE;

    /* Not currently supported with reverse PRIME */
    if (ms->drmmode.reverse_prime_offload_mode)
        return FALSE;

#ifdef XSERVER_PLATFORM_BUS
    if (pEnt->location.type == BUS_PLATFORM) {
        char *syspath =
            xf86_platform_device_odev_attributes(pEnt->location.id.plat)->
            syspath;

        /* Not supported for devices using USB transport due to misbehaved
         * vblank events */
        if (syspath && strstr(syspath, "usb"))
            return FALSE;
    }
#endif

    return drmmode_EnableSharedPixmapFlipping(xf86Crtc, &ms->drmmode,
                                              front, back);
}

static void
msDisableSharedPixmapFlipping(RRCrtcPtr crtc)
{
    ScreenPtr screen = crtc->pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    xf86CrtcPtr xf86Crtc = crtc->devPrivate;

    if (xf86Crtc)
        drmmode_DisableSharedPixmapFlipping(xf86Crtc, &ms->drmmode);
}

static Bool
msStartFlippingPixmapTracking(RRCrtcPtr crtc, PixmapPtr src,
                              PixmapPtr slave_dst1, PixmapPtr slave_dst2,
                              int x, int y, int dst_x, int dst_y,
                              Rotation rotation)
{
    ScreenPtr pScreen = src->drawable.pScreen;
    modesettingPtr ms = modesettingPTR(xf86ScreenToScrn(pScreen));

    msPixmapPrivPtr ppriv1 = msGetPixmapPriv(&ms->drmmode, slave_dst1),
                    ppriv2 = msGetPixmapPriv(&ms->drmmode, slave_dst2);

    if (!PixmapStartDirtyTracking(src, slave_dst1, x, y,
                                  dst_x, dst_y, rotation)) {
        return FALSE;
    }

    if (!PixmapStartDirtyTracking(src, slave_dst2, x, y,
                                  dst_x, dst_y, rotation)) {
        PixmapStopDirtyTracking(src, slave_dst1);
        return FALSE;
    }

    ppriv1->slave_src = src;
    ppriv2->slave_src = src;

    ppriv1->dirty = ms_dirty_get_ent(pScreen, slave_dst1);
    ppriv2->dirty = ms_dirty_get_ent(pScreen, slave_dst2);

    ppriv1->defer_dirty_update = TRUE;
    ppriv2->defer_dirty_update = TRUE;

    return TRUE;
}

static Bool
msPresentSharedPixmap(PixmapPtr slave_dst)
{
    ScreenPtr pScreen = slave_dst->drawable.pScreen;
    modesettingPtr ms = modesettingPTR(xf86ScreenToScrn(pScreen));

    msPixmapPrivPtr ppriv = msGetPixmapPriv(&ms->drmmode, slave_dst);

    RegionPtr region = DamageRegion(ppriv->dirty->damage);

    if (RegionNotEmpty(region)) {
        redisplay_dirty(ppriv->slave_src->drawable.pScreen, ppriv->dirty, NULL);
        DamageEmpty(ppriv->dirty->damage);

        return TRUE;
    }

    return FALSE;
}

static Bool
msStopFlippingPixmapTracking(PixmapPtr src,
                             PixmapPtr slave_dst1, PixmapPtr slave_dst2)
{
    ScreenPtr pScreen = src->drawable.pScreen;
    modesettingPtr ms = modesettingPTR(xf86ScreenToScrn(pScreen));

    msPixmapPrivPtr ppriv1 = msGetPixmapPriv(&ms->drmmode, slave_dst1),
                    ppriv2 = msGetPixmapPriv(&ms->drmmode, slave_dst2);

    Bool ret = TRUE;

    ret &= PixmapStopDirtyTracking(src, slave_dst1);
    ret &= PixmapStopDirtyTracking(src, slave_dst2);

    if (ret) {
        ppriv1->slave_src = NULL;
        ppriv2->slave_src = NULL;

        ppriv1->dirty = NULL;
        ppriv2->dirty = NULL;

        ppriv1->defer_dirty_update = FALSE;
        ppriv2->defer_dirty_update = FALSE;
    }

    return ret;
}
#endif

static Bool
CreateScreenResources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    VivPtr fPtr = GET_VIV_PTR(pScrn);
    PixmapPtr rootPixmap;
    Bool ret;
    void *pixels = NULL;
    int err;
    pScreen->CreateScreenResources = fPtr->createScreenResources;
    ret = pScreen->CreateScreenResources(pScreen);
    pScreen->CreateScreenResources = CreateScreenResources;


    if (!drmmode_set_desired_modes(pScrn, &fPtr->drmmode, TRUE))
        return FALSE;

    drmmode_uevent_init(pScrn, &fPtr->drmmode);

    if (!fPtr->drmmode.sw_cursor)
        drmmode_map_cursor_bos(pScrn, &fPtr->drmmode);
    pixels = drmmode_map_front_bo(&fPtr->drmmode);
    if (!pixels)
        return FALSE;

    rootPixmap = pScreen->GetScreenPixmap(pScreen);

    if (fPtr->drmmode.shadow_enable)
        pixels = fPtr->drmmode.shadow_fb;

    if (!pScreen->ModifyPixmapHeader(rootPixmap, -1, -1, -1, -1, -1, pixels))
        FatalError("Couldn't adjust screen pixmap\n");

    if (fPtr->drmmode.shadow_enable) {
         if (!shadowAdd(pScreen, rootPixmap, msUpdatePacked, msShadowWindow,
                       0, 0))
            return FALSE;
    }

    err = drmModeDirtyFB(fPtr->fd, fPtr->drmmode.fb_id, NULL, 0);

    if (err != -EINVAL && err != -ENOSYS) {
        fPtr->damage = DamageCreate(NULL, NULL, DamageReportNone, TRUE,
            pScreen, rootPixmap);

        if (fPtr->damage) {
            DamageRegister(&rootPixmap->drawable, fPtr->damage);
            fPtr->dirty_enabled = TRUE;
            xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Damage tracking initialized\n");
        }
        else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "Failed to create screen damage record\n");
            return FALSE;
        }
    }
#if XORG_VERSION_CURRENT >= XORG_VERSION_NUMERIC(1,19,0,0,0)
    pScrPriv->rrEnableSharedPixmapFlipping = msEnableSharedPixmapFlipping;
    pScrPriv->rrDisableSharedPixmapFlipping = msDisableSharedPixmapFlipping;

    pScrPriv->rrStartFlippingPixmapTracking = msStartFlippingPixmapTracking;
#endif
    return ret;
}

static Bool
msShadowInit(ScreenPtr pScreen)
{
    if (!shadowSetup(pScreen)) {
        return FALSE;
    }
    return TRUE;
}

#ifdef MODESETTING_OUTPUT_SLAVE_SUPPORT
static Bool
msSetSharedPixmapBacking(PixmapPtr ppix, void *fd_handle)
{
    ScreenPtr screen = ppix->drawable.pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    VivPtr fPtr = GET_VIV_PTR(scrn);
    Bool ret;
    int size = ppix->devKind * ppix->drawable.height;
    int ihandle = (int)(long)fd_handle;

    ret = drmmode_SetSlaveBO(ppix, &fPtr->drmmode, ihandle, ppix->devKind, size);
    if (ret == FALSE)
        return ret;

    return TRUE;
}
#endif

static Bool
SetMaster(ScrnInfoPtr pScrn)
{
    VivPtr fPtr = GET_VIV_PTR(pScrn);
    int ret;

#ifdef XF86_PDEV_SERVER_FD
    if (fPtr->pEnt->location.type == BUS_PLATFORM &&
        (fPtr->pEnt->location.id.plat->flags & XF86_PDEV_SERVER_FD))
        return TRUE;
#endif

    ret = drmSetMaster(fPtr->fd);
    if (ret)
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "drmSetMaster failed: %s\n",
            strerror(errno));

    return ret == 0;
}
/* When the root window is created, initialize the screen contents from
 * console if -background none was specified on the command line
 */
static Bool
CreateWindow_oneshot(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    Bool ret;

    pScreen->CreateWindow = ms->CreateWindow;
    ret = pScreen->CreateWindow(pWin);

    if (ret)
        drmmode_copy_fb(pScrn, &ms->drmmode);
    return ret;
}

#ifdef ENABLE_VIVANTE_DRI3
extern Bool vivanteDRI3ScreenInit(ScreenPtr pScreen);
#endif

static Bool
ScreenInit(SCREEN_INIT_ARGS_DECL)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    VivPtr fPtr = GET_VIV_PTR(pScrn);
    VisualPtr visual;
        int ret, flags;
        int type;

    pScrn->pScreen = pScreen;

    if (!SetMaster(pScrn))
        return FALSE;

    /* HW dependent - FIXME */
    pScrn->displayWidth = pScrn->virtualX;
    if (!drmmode_create_initial_bos(pScrn, &fPtr->drmmode))
        return FALSE;

    if (fPtr->drmmode.shadow_enable) {
        fPtr->drmmode.shadow_fb = calloc(1, pScrn->displayWidth * pScrn->virtualY *
            ((pScrn->bitsPerPixel + 7) >> 3));
        if (!fPtr->drmmode.shadow_fb)
            fPtr->drmmode.shadow_enable = FALSE;
    }

    miClearVisualTypes();

    if (!miSetVisualTypes(pScrn->depth,
        miGetDefaultVisualMask(pScrn->depth),
        pScrn->rgbBits, pScrn->defaultVisual))
        return FALSE;

    if (!miSetPixmapDepths())
        return FALSE;

#ifdef MODESETTING_OUTPUT_SLAVE_SUPPORT
    if (!dixRegisterScreenSpecificPrivateKey(pScreen, &fPtr->drmmode.pixmapPrivateKeyRec,
        PRIVATE_PIXMAP, sizeof(msPixmapPrivRec))) {
        return FALSE;
    }
#endif

    pScrn->memPhysBase = 0;
    pScrn->fbOffset = 0;

    if (!fbScreenInit(pScreen, NULL,
        pScrn->virtualX, pScrn->virtualY,
        pScrn->xDpi, pScrn->yDpi,
        pScrn->displayWidth, pScrn->bitsPerPixel))
        return FALSE;

    if (pScrn->bitsPerPixel > 8) {
        /* Fixup RGB ordering */
        visual = pScreen->visuals + pScreen->numVisuals;
        while (--visual >= pScreen->visuals) {
            if ((visual->class | DynamicClass) == DirectColor) {
                visual->offsetRed = pScrn->offset.red;
                visual->offsetGreen = pScrn->offset.green;
                visual->offsetBlue = pScrn->offset.blue;
                visual->redMask = pScrn->mask.red;
                visual->greenMask = pScrn->mask.green;
                visual->blueMask = pScrn->mask.blue;
            }
        }
    }
    fbPictureInit(pScreen, NULL, 0);

    if (fPtr->drmmode.shadow_enable && !msShadowInit(pScreen)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
            "shadow fb init failed\n");
        return FALSE;
    }

    fPtr->createScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = CreateScreenResources;

    if (!xf86LoadSubModule(pScrn, "exa")) {
            FreeRec(pScrn);
            xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Error on loading exa submodule\n");
            TRACE_EXIT(FALSE);
        }

    if (!InitExaLayer(pScreen)) {
                xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "internal error: initExaLayer failed "
                    "in ScreenInit()\n");
        }
#ifdef ENABLE_VIVANTE_DRI3
        if (!vivanteDRI3ScreenInit(pScreen)) {
            xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
                   "[drm] IMX G2D DRI3 initialization failed, running unaccelerated\n");
        }
#endif
    xf86SetBlackWhitePixels(pScreen);

    xf86SetBackingStore(pScreen);
    xf86SetSilkenMouse(pScreen);
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());


    /* Need to extend HWcursor support to handle mask interleave */
    if (!fPtr->drmmode.sw_cursor)
        xf86_cursors_init(pScreen, fPtr->cursor_width, fPtr->cursor_height,
            HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64 |
            HARDWARE_CURSOR_ARGB);

    /* Must force it before EnterVT, so we are in control of VT and
     * later memory should be bound when allocating, e.g rotate_mem */
    pScrn->vtSema = TRUE;

    pScreen->SaveScreen = xf86SaveScreen;
    fPtr->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = CloseScreen;

    fPtr->BlockHandler = pScreen->BlockHandler;
    pScreen->BlockHandler = msBlockHandler;

#if XORG_VERSION_CURRENT >= XORG_VERSION_NUMERIC(1,19,0,0,0)
    pScreen->SharePixmapBacking = msSharePixmapBacking;
    pScreen->SetSharedPixmapBacking = msSetSharedPixmapBacking;
    pScreen->StartPixmapTracking = PixmapStartDirtyTracking;
    pScreen->StopPixmapTracking = PixmapStopDirtyTracking;

    pScreen->SharedPixmapNotifyDamage = msSharedPixmapNotifyDamage;
    pScreen->RequestSharedPixmapNotifyDamage =
        msRequestSharedPixmapNotifyDamage;

    pScreen->PresentSharedPixmap = msPresentSharedPixmap;
    pScreen->StopFlippingPixmapTracking = msStopFlippingPixmapTracking;

#ifdef MODESETTING_OUTPUT_SLAVE_SUPPORT
    pScreen->SetSharedPixmapBacking = msSetSharedPixmapBacking;
#endif
#endif

    if (!xf86CrtcScreenInit(pScreen))
        return FALSE;

    if (!miCreateDefColormap(pScreen))
        return FALSE;

    xf86DPMSInit(pScreen, xf86DPMSSet, 0);
    if (serverGeneration == 1)
        xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

    return EnterVT(VT_FUNC_ARGS);
}

static void
AdjustFrame(ADJUST_FRAME_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    VivPtr fPtr = GET_VIV_PTR(pScrn);

    drmmode_adjust_frame(pScrn, &fPtr->drmmode, x, y);
}

static void
FreeScreen(FREE_SCREEN_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    FreeRec(pScrn);
}

static void
LeaveVT(VT_FUNC_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    VivPtr fPtr = GET_VIV_PTR(pScrn);
    xf86_hide_cursors(pScrn);

    pScrn->vtSema = FALSE;

#ifdef XF86_PDEV_SERVER_FD
    if (fPtr->pEnt->location.type == BUS_PLATFORM &&
        (fPtr->pEnt->location.id.plat->flags & XF86_PDEV_SERVER_FD))
        return;
#endif

    drmDropMaster(fPtr->fd);
}

/*
 * This gets called when gaining control of the VT, and from ScreenInit().
 */
static Bool
EnterVT(VT_FUNC_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    VivPtr fPtr = GET_VIV_PTR(pScrn);

    pScrn->vtSema = TRUE;

    SetMaster(pScrn);

    if (!drmmode_set_desired_modes(pScrn, &fPtr->drmmode, TRUE))
        return FALSE;

    return TRUE;
}

static Bool
SwitchMode(SWITCH_MODE_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);

    return xf86SetSingleMode(pScrn, mode, RR_Rotate_0);
}

#ifdef ENABLE_VIVANTE_DRI3
extern void vivanteDRI3ScreenDeInit(ScreenPtr pScreen);
#endif

static Bool
CloseScreen(CLOSE_SCREEN_ARGS_DECL)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    VivPtr fPtr = GET_VIV_PTR(pScrn);

    if (fPtr->damage) {
                DamageUnregister(fPtr->damage);
        DamageDestroy(fPtr->damage);
        fPtr->damage = NULL;
    }

    if (fPtr->drmmode.shadow_enable) {
        shadowRemove(pScreen, pScreen->GetScreenPixmap(pScreen));
        free(fPtr->drmmode.shadow_fb);
        fPtr->drmmode.shadow_fb = NULL;
    }
    drmmode_uevent_fini(pScrn, &fPtr->drmmode);

    drmmode_free_bos(pScrn, &fPtr->drmmode);

    if (pScrn->vtSema) {
        LeaveVT(VT_FUNC_ARGS);
    }

#ifdef ENABLE_VIVANTE_DRI3
    vivanteDRI3ScreenDeInit(pScreen);
#endif

    pScreen->CreateScreenResources = fPtr->createScreenResources;
    pScreen->BlockHandler = fPtr->BlockHandler;

    pScrn->vtSema = FALSE;
    pScreen->CloseScreen = fPtr->CloseScreen;
    return (*pScreen->CloseScreen) (CLOSE_SCREEN_ARGS);
}

static ModeStatus
ValidMode(SCRN_ARG_TYPE arg, DisplayModePtr mode, Bool verbose, int flags)
{
    return MODE_OK;
}

Bool imx_kms_probe(DriverPtr drv, int flags) {
    return Probe(drv, flags);
}
