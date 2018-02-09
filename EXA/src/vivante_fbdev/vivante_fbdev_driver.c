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


#include "vivante_common.h"
#include "vivante.h"
#include "vivante_exa.h"
#ifndef DISABLE_VIVANTE_DRI
#include "vivante_dri.h"
#endif

#ifdef USE_PROBE_VIV_FBDEV_DRIVER
#include "../vivante_extension/vivante_ext.h"
#endif

#include <errno.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <xorg/shadow.h>

/* For ADD_FSL_XRANDR, only for freescale imx chips */
/* To enable ADD_FSL_XRANDR, you should get FSL imx display module first and then you can enable it */
#ifdef ADD_FSL_XRANDR
#include "imx_display.h"
#endif

#if XORG_VERSION_CURRENT > XORG_VERSION_NUMERIC(1,12,0,0,0)
#else
#include "mibstore.h"
#endif

#ifdef XSERVER_LIBPCIACCESS
#include <pciaccess.h>
#endif
/************************************************************************
 * MACROS FOR VERSIONING & INFORMATION (START)
 ************************************************************************/
#define VIVANTE_VERSION           1000
#define VIVANTE_VERSION_MAJOR      1
#define VIVANTE_VERSION_MINOR      0
#define VIVANTE_VERSION_PATCHLEVEL 0
#define VIVANTE_VERSION_STRING  "1.0"

#define VIVANTE_NAME              "VIVANTE"
#define VIVANTE_DRIVER_NAME       "vivante"

#ifdef XSERVER_LIBPCIACCESS
static const struct pci_id_match viv_device_match[] = {
    {
    PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY,
    0x00030000, 0x00ffffff, 0
    },

    { 0, 0, 0 },
};
#endif


Bool vivEnableCacheMemory = TRUE;
#ifdef ADD_FSL_XRANDR
static Bool vivEnableXrandr = FALSE;
static Bool gEnableFbSyncExt = TRUE;
#endif

#ifdef DEBUG
unsigned int  vivEnableDump = VIV_NOMSG;
#endif
/************************************************************************
 * MACROS FOR VERSIONING & INFORMATION (END)
 ************************************************************************/

/************************************************************************
 * Core Functions To FrameBuffer Driver
 * 1) AvailableOptions
 * 2) Identify
 * 3) Probe
 * 4) PreInit
 * 5) ScreenInit
 * 6) CloseScreen
 * 7) DriverFunction(Opt)
 ************************************************************************/

/************************************************************************
 *  Const/Dest Functions (START)
 ************************************************************************/
static Bool
VivGetRec(ScrnInfoPtr pScrn) {
    if (pScrn->driverPrivate != NULL) {
        TRACE_EXIT(TRUE);
    }
    pScrn->driverPrivate = malloc(sizeof (VivRec));
    memset((char *)pScrn->driverPrivate, 0, sizeof(VivRec));
#ifdef ADD_FSL_XRANDR
    VivPtr vPtr = GET_VIV_PTR(pScrn);

    vPtr->fbAlignOffset = ADDRESS_ALIGNMENT;
    vPtr->fbAlignWidth  = WIDTH_ALIGNMENT;
    vPtr->fbAlignHeight = HEIGHT_ALIGNMENT;

    imxInitSyncFlagsStorage(pScrn);
#endif
    TRACE_EXIT(TRUE);
}

static void
VivFreeRec(ScrnInfoPtr pScrn) {
    if (pScrn->driverPrivate == NULL) {
        TRACE_EXIT();
    }

#ifdef ADD_FSL_XRANDR
    VivPtr vPtr = GET_VIV_PTR(pScrn);
    if(vPtr->lastVideoMode) {
        xf86DeleteMode(&vPtr->lastVideoMode, vPtr->lastVideoMode);
    }
    imxFreeSyncFlagsStorage(pScrn);
#endif
    free(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
    TRACE_EXIT();
}

#ifdef ADD_FSL_XRANDR
static void
VivFreeScreen(FREE_SCREEN_ARGS_DECL)
{
#ifndef XF86_SCRN_INTERFACE
    ScrnInfoPtr pScrn = xf86Screens[arg];
#else
    ScrnInfoPtr pScrn = arg;
#endif

    VivFreeRec(pScrn);
}
#endif

/************************************************************************
 * R Const/Dest Functions (END)
 ************************************************************************/
static const OptionInfoRec *VivAvailableOptions(int chipid, int busid);
static void VivIdentify(int flags);
static Bool VivProbe(DriverPtr drv, int flags);
#ifdef XSERVER_LIBPCIACCESS
static Bool VivPciProbe(DriverPtr drv, int entity_num, struct pci_device *dev, intptr_t match_data);
#endif
static Bool VivPreInit(ScrnInfoPtr pScrn, int flags);

static Bool VivScreenInit(SCREEN_INIT_ARGS_DECL);
static Bool VivCloseScreen(CLOSE_SCREEN_ARGS_DECL);


static Bool VivDriverFunc(ScrnInfoPtr pScrn, xorgDriverFuncOp op,
        pointer ptr);

/************************************************************************
 * END OF  Core Functions To FrameBuffer Driver
 ************************************************************************/
static void VivPointerMoved(SCRN_ARG_TYPE arg, int x, int y);
static Bool VivDGAInit(ScrnInfoPtr pScrn, ScreenPtr pScreen);
static Bool VivCreateScreenResources(ScreenPtr pScreen);
enum { VIV_ROTATE_NONE=0, VIV_ROTATE_CW=270, VIV_ROTATE_UD=180, VIV_ROTATE_CCW=90 };
static Bool VivShadowInit(ScreenPtr pScreen);

#ifdef ADD_FSL_XRANDR
static Bool SaveBuildInModeSyncFlags(ScrnInfoPtr pScrn);
static Bool RestoreSyncFlags(ScrnInfoPtr pScrn);
#endif
/************************************************************************
 * SUPPORTED CHIPSETS (START)
 ************************************************************************/

typedef enum _chipSetID {
    GC500_ID = 0x33,
    GC2100_ID,
    GCCORE_ID
} CHIPSETID;

/*CHIP NAMES*/
#define GCCORE_STR "VivanteGCCORE"
#define GC500_STR  "VivanteGC500"
#define GC2100_STR "VivanteGC2100"

/************************************************************************
 * SUPPORTED CHIPSETS (END)
 ************************************************************************/
/*
 * This is intentionally screen-independent.  It indicates the binding
 * choice made in the first PreInit.
 */
static int pix24bpp = 0;


/************************************************************************
 * X Window System Registration (START)
 ************************************************************************/

#ifdef USE_PROBE_VIV_FBDEV_DRIVER
_X_EXPORT DriverRec VIV = {
    VIVANTE_VERSION,
    VIVANTE_DRIVER_NAME,
    VivIdentify,
    VivProbe,
    VivAvailableOptions,
    NULL,
    0,
    VivDriverFunc,
#ifdef XSERVER_LIBPCIACCESS
    viv_device_match,
    VivPciProbe
#endif
};

/* Supported "chipsets" */
static SymTabRec VivChipsets[] = {
    {GC500_ID, GC500_STR},
    {GC2100_ID, GC2100_STR},
    {GCCORE_ID, GCCORE_STR},
    {-1, NULL}
};
#endif


static const OptionInfoRec VivOptions[] = {
    { OPTION_SHADOW_FB, "ShadowFB", OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_ROTATE, "Rotate", OPTV_STRING, {0}, FALSE },
#ifdef DEBUG
    { OPTION_DUMP, "VDump", OPTV_STRING, {0}, FALSE },
#endif
#ifdef ADD_FSL_XRANDR
    { OPTION_XRANDR, "FSLXRandr", OPTV_BOOLEAN, {0}, FALSE },
#endif
#ifdef HAVE_G2D
    { OPTION_ACCELHW, "AccelHw", OPTV_STRING, {0}, FALSE },
#endif
    { OPTION_VIV, "vivante", OPTV_STRING, {0}, FALSE},
    { OPTION_NOACCEL, "NoAccel", OPTV_BOOLEAN, {0}, FALSE},
    { OPTION_ACCELMETHOD, "AccelMethod", OPTV_STRING, {0}, FALSE},
    { OPTION_VIVCACHEMEM, "VivCacheMem", OPTV_BOOLEAN, {0}, FALSE},
    { -1, NULL, OPTV_NONE, {0}, FALSE}
};


#ifdef USE_PROBE_VIV_FBDEV_DRIVER

/* -------------------------------------------------------------------- */

#ifdef XFree86LOADER

MODULESETUPPROTO(VivSetup);

static XF86ModuleVersionInfo VivVersRec = {
    VIVANTE_DRIVER_NAME,
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    VIVANTE_VERSION_MAJOR,
    VIVANTE_VERSION_MINOR,
    VIVANTE_VERSION_PATCHLEVEL,
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    NULL,
    {0, 0, 0, 0}
};


static Bool noVIVExtension;

static ExtensionModule VIVExt =
{
    VIVExtensionInit,
    VIVEXTNAME,
    &noVIVExtension
};

_X_EXPORT XF86ModuleData vivanteModuleData = {&VivVersRec, VivSetup, NULL};

pointer
VivSetup(pointer module, pointer opts, int *errmaj, int *errmin) {
    TRACE_ENTER();
    pointer ret;
    static Bool setupDone = FALSE;

    if (!setupDone) {
        setupDone = TRUE;
        xf86AddDriver(&VIV, module, HaveDriverFuncs);
        ret = (pointer) 1;

#if XORG_VERSION_CURRENT < XORG_VERSION_NUMERIC(1,15,0,0,0)
        LoadExtension(&VIVExt, FALSE);
#else
        LoadExtensionList(&VIVExt, 1, FALSE);
#endif

    } else {
        if (errmaj) *errmaj = LDR_ONCEONLY;
        ret = (pointer) 0;

    }
    TRACE_EXIT(ret);
}
#endif /* XFree86LOADER */
#endif

/************************************************************************
 * X Window System Registration (START)
 ************************************************************************/
/***********************************************************************
 * Shadow stuff
 ***********************************************************************/

static Bool
VivShadowInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    VivPtr fPtr = GET_VIV_PTR(pScrn);

    if (!shadowSetup(pScreen)) {
    return FALSE;
    }

    fPtr->CreateScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = VivCreateScreenResources;

    return TRUE;
}


static void
VivPointerMoved(SCRN_ARG_TYPE arg, int x, int y)
{
    SCRN_INFO_PTR(arg);
    VivPtr fPtr = GET_VIV_PTR(pScrn);
    int newX, newY;

    switch (fPtr->rotate)
    {
    case VIV_ROTATE_CW:
    /* 90 degrees CW rotation. */
    newX = pScrn->pScreen->height - y - 1;
    newY = x;
    break;

    case VIV_ROTATE_CCW:
    /* 90 degrees CCW rotation. */
    newX = y;
    newY = pScrn->pScreen->width - x - 1;
    break;

    case VIV_ROTATE_UD:
    /* 180 degrees UD rotation. */
    newX = pScrn->pScreen->width - x - 1;
    newY = pScrn->pScreen->height - y - 1;
    break;

    default:
    /* No rotation. */
    newX = x;
    newY = y;
    break;
    }

    /* Pass adjusted pointer coordinates to wrapped PointerMoved function. */
    (*fPtr->PointerMoved)(arg, newX, newY);
}


/***********************************************************************
 * DGA stuff
 ***********************************************************************/
static Bool VivDGAOpenFramebuffer(ScrnInfoPtr pScrn, char **DeviceName,
                   unsigned char **ApertureBase,
                   int *ApertureSize, int *ApertureOffset,
                   int *flags);
static Bool VivDGASetMode(ScrnInfoPtr pScrn, DGAModePtr pDGAMode);
static void VivDGASetViewport(ScrnInfoPtr pScrn, int x, int y, int flags);

static Bool
VivDGAOpenFramebuffer(ScrnInfoPtr pScrn, char **DeviceName,
               unsigned char **ApertureBase, int *ApertureSize,
               int *ApertureOffset, int *flags)
{
    *DeviceName = NULL;     /* No special device */
    *ApertureBase = (unsigned char *)(pScrn->memPhysBase);
    *ApertureSize = pScrn->videoRam;
    *ApertureOffset = pScrn->fbOffset;
    *flags = 0;

    return TRUE;
}

static Bool
VivDGASetMode(ScrnInfoPtr pScrn, DGAModePtr pDGAMode)
{
    DisplayModePtr pMode;
    int frameX0, frameY0;

    if (pDGAMode) {
    pMode = pDGAMode->mode;
    frameX0 = frameY0 = 0;
    }
    else {
    if (!(pMode = pScrn->currentMode))
        return TRUE;

    frameX0 = pScrn->frameX0;
    frameY0 = pScrn->frameY0;
    }

    if (!(*pScrn->SwitchMode)(SWITCH_MODE_ARGS(pScrn, pMode)))
    return FALSE;
    (*pScrn->AdjustFrame)(ADJUST_FRAME_ARGS(pScrn, frameX0, frameY0));

    return TRUE;
}

static void
VivDGASetViewport(ScrnInfoPtr pScrn, int x, int y, int flags)
{
    (*pScrn->AdjustFrame)(ADJUST_FRAME_ARGS(pScrn, x, y));
}

static int
VivDGAGetViewport(ScrnInfoPtr pScrn)
{
    return (0);
}

static DGAFunctionRec VivDGAFunctions =
{
    VivDGAOpenFramebuffer,
    NULL,       /* CloseFramebuffer */
    VivDGASetMode,
    VivDGASetViewport,
    VivDGAGetViewport,
    NULL,       /* Sync */
    NULL,       /* FillRect */
    NULL,       /* BlitRect */
    NULL,       /* BlitTransRect */
};

static void
VivDGAAddModes(ScrnInfoPtr pScrn)
{
    VivPtr fPtr = GET_VIV_PTR(pScrn);
    DisplayModePtr pMode = pScrn->modes;
    DGAModePtr pDGAMode;

    do {
    pDGAMode = realloc(fPtr->pDGAMode,
                   (fPtr->nDGAMode + 1) * sizeof(DGAModeRec));
    if (!pDGAMode)
        break;

    fPtr->pDGAMode = pDGAMode;
    pDGAMode += fPtr->nDGAMode;
    (void)memset(pDGAMode, 0, sizeof(DGAModeRec));

    ++fPtr->nDGAMode;
    pDGAMode->mode = pMode;
    pDGAMode->flags = DGA_CONCURRENT_ACCESS | DGA_PIXMAP_AVAILABLE;
    pDGAMode->byteOrder = pScrn->imageByteOrder;
    pDGAMode->depth = pScrn->depth;
    pDGAMode->bitsPerPixel = pScrn->bitsPerPixel;
    pDGAMode->red_mask = pScrn->mask.red;
    pDGAMode->green_mask = pScrn->mask.green;
    pDGAMode->blue_mask = pScrn->mask.blue;
    pDGAMode->visualClass = pScrn->bitsPerPixel > 8 ?
        TrueColor : PseudoColor;
    pDGAMode->xViewportStep = 1;
    pDGAMode->yViewportStep = 1;
    pDGAMode->viewportWidth = pMode->HDisplay;
    pDGAMode->viewportHeight = pMode->VDisplay;

    if (fPtr->lineLength)
      pDGAMode->bytesPerScanline = fPtr->lineLength;
    else
      pDGAMode->bytesPerScanline = fPtr->lineLength = fbdevHWGetLineLength(pScrn);

    pDGAMode->imageWidth = pMode->HDisplay;
    pDGAMode->imageHeight =  pMode->VDisplay;
    pDGAMode->pixmapWidth = pDGAMode->imageWidth;
    pDGAMode->pixmapHeight = pDGAMode->imageHeight;
    pDGAMode->maxViewportX = pScrn->virtualX -
                    pDGAMode->viewportWidth;
    pDGAMode->maxViewportY = pScrn->virtualY -
                    pDGAMode->viewportHeight;

    pDGAMode->address = fPtr->mFB.mFBStart;

    pMode = pMode->next;
    } while (pMode != pScrn->modes);
}

static Bool
VivDGAInit(ScrnInfoPtr pScrn, ScreenPtr pScreen)
{
#ifdef XFreeXDGA
    VivPtr fPtr = GET_VIV_PTR(pScrn);

    if (pScrn->depth < 8)
    return FALSE;

    if (!fPtr->nDGAMode)
    VivDGAAddModes(pScrn);

    return (DGAInit(pScreen, &VivDGAFunctions,
        fPtr->pDGAMode, fPtr->nDGAMode));
#else
    return TRUE;
#endif
}


#ifdef XSERVER_LIBPCIACCESS
static Bool VivPciProbe(DriverPtr drv, int entity_num,
              struct pci_device *dev, intptr_t match_data)
{
    ScrnInfoPtr pScrn = NULL;

    if (!xf86LoadDrvSubModule(drv, "fbdevhw"))
    return FALSE;

    pScrn = xf86ConfigPciEntity(NULL, 0, entity_num, NULL, NULL,
                NULL, NULL, NULL, NULL);
    if (pScrn) {
    const char *device;
    GDevPtr devSection = xf86GetDevFromEntity(pScrn->entityList[0],
                          pScrn->entityInstanceList[0]);

    device = xf86FindOptionValue(devSection->options, "vivante");
    if (fbdevHWProbe(NULL, (char *)device, NULL)) {
        pScrn->driverVersion = VIVANTE_VERSION;
        pScrn->driverName    = VIVANTE_DRIVER_NAME;
        pScrn->name          = VIVANTE_NAME;
        pScrn->Probe         = VivProbe;
        pScrn->PreInit       = VivPreInit;
        pScrn->ScreenInit    = VivScreenInit;
        pScrn->SwitchMode    = fbdevHWSwitchModeWeak();
        pScrn->AdjustFrame   = fbdevHWAdjustFrameWeak();
        pScrn->EnterVT       = fbdevHWEnterVTWeak();
        pScrn->LeaveVT       = fbdevHWLeaveVTWeak();
        pScrn->ValidMode     = fbdevHWValidModeWeak();

        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
               "claimed PCI slot %d@%d:%d:%d\n",
               dev->bus, dev->domain, dev->dev, dev->func);
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
               "using %s\n", device ? device : "default device");
    }
    else {
        pScrn = NULL;
    }
    }

    return (pScrn != NULL);
}
#endif
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

    /*Memory Manager*/
    pExa->memoryBase = pViv->mFB.mFBStart; /*logical*/
    pExa->memorySize = pScrn->videoRam;
    pExa->offScreenBase = pScrn->virtualY * pScrn->displayWidth * (pScrn->bitsPerPixel >> 3);

    if (!VIV2DGPUCtxInit(&pViv->mGrCtx)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "internal error: GPU Ctx Init Failed\n");
        TRACE_EXIT(FALSE);
    }

    if (!VIV2DGPUUserMemMap((char*) pExa->memoryBase, pScrn->memPhysBase, pExa->memorySize, &pViv->mFB.mMappingInfo, (unsigned int *)&pViv->mFB.memGpuBase)) {
        TRACE_ERROR("ERROR ON MAPPING FB\n");
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
        pExa->WaitMarker = VivEXASync;

        pExa->PrepareSolid = pViv->mFakeExa.mNoAccelFlag?
                             VivPrepareSolidFail:VivPrepareSolid;
        pExa->Solid = VivSolid;
        pExa->DoneSolid = VivDoneSolid;

        pExa->PrepareCopy = pViv->mFakeExa.mNoAccelFlag?
                            VivPrepareCopyFail:VivPrepareCopy;
        pExa->Copy = VivCopy;
        pExa->DoneCopy = VivDoneCopy;

        pExa->UploadToScreen = VivUploadToScreen;


        pExa->CheckComposite = pViv->mFakeExa.mNoAccelFlag?
                               VivCheckCompositeFail:VivCheckComposite;
        pExa->PrepareComposite = pViv->mFakeExa.mNoAccelFlag?
                                 VivPrepareCompositeFail:VivPrepareComposite;
        pExa->Composite = VivComposite;
        pExa->DoneComposite = VivDoneComposite;

        pExa->CreatePixmap = VivCreatePixmap;
        pExa->DestroyPixmap = VivDestroyPixmap;
        pExa->ModifyPixmapHeader = VivModifyPixmapHeader;
        pExa->PixmapIsOffscreen = VivPixmapIsOffscreen;
        pExa->PrepareAccess = VivPrepareAccess;
        pExa->FinishAccess = VivFinishAccess;
    }

    if (!exaDriverInit(pScreen, pExa)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "exaDriverinit failed.\n");
        TRACE_EXIT(FALSE);
    }

    pViv->mFakeExa.mIsInited = TRUE;
    TRACE_EXIT(TRUE);
}

static Bool DestroyExaLayer(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = GET_PSCR(pScreen);
    VivPtr pViv = GET_VIV_PTR(pScrn);
    TRACE_ENTER();
    xf86DrvMsg(pScreen->myNum, X_INFO, "Shutdown EXA\n");

    ExaDriverPtr pExa = pViv->mFakeExa.mExaDriver;
    if (!VIV2DGPUUserMemUnMap((char*) pExa->memoryBase, pExa->memorySize, pViv->mFB.mMappingInfo, pViv->mFB.memGpuBase)) {
        TRACE_ERROR("Unmapping User memory Failed\n");
    }

    exaDriverFini(pScreen);

    if (!VIV2DGPUCtxDeInit(&pViv->mGrCtx)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "internal error: GPU Ctx DeInit Failed\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}



/************************************************************************
 * START OF THE IMPLEMENTATION FOR CORE FUNCTIONS
 ************************************************************************/

#ifdef USE_PROBE_VIV_FBDEV_DRIVER
static const OptionInfoRec *
VivAvailableOptions(int chipid, int busid) {
    /*Chip id may also be used for special cases*/
    TRACE_ENTER();
    TRACE_EXIT(VivOptions);
}

static void
VivIdentify(int flags) {
    TRACE_ENTER();
    xf86PrintChipsets(VIVANTE_NAME, "fb driver for vivante", VivChipsets);
    TRACE_EXIT();
}
#endif

static Bool
VivProbe(DriverPtr drv, int flags) {
    int i;
    ScrnInfoPtr pScrn;
    GDevPtr *devSections;
    int numDevSections;
    char *dev;
#ifdef DEBUG
    char *pdump = NULL;
#endif
    Bool foundScreen = FALSE;
#ifdef ADD_FSL_XRANDR
    OptionInfoPtr pOptions = NULL;
#endif
#ifndef XSERVER_LIBPCIACCESS
        int bus,device,func;
#endif

    TRACE_ENTER();

    /* For now, just bail out for PROBE_DETECT. */
    if (flags & PROBE_DETECT) {
        /*Look into PROBE_DETECT*/
        TRACE_EXIT(FALSE);
    }


    /*
     * int xf86MatchDevice(char * drivername, GDevPtr ** sectlist)
     * with its driver name. The function allocates an array of GDevPtr and
     * returns this via sectlist and returns the number of elements in
     * this list as return value. 0 means none found, -1 means fatal error.
     *
     * It can figure out which of the Device sections to use for which card
     * (using things like the Card statement, etc). For single headed servers
     * there will of course be just one such Device section.
     */
    numDevSections = xf86MatchDevice(VIVANTE_DRIVER_NAME, &devSections);
    if (numDevSections <= 0) {
        TRACE_ERROR("No matching device\n");
        TRACE_EXIT(FALSE);
    }

    if (!xf86LoadDrvSubModule(drv, "fbdevhw")) {
        TRACE_ERROR("Unable to load fbdev module\n");
        TRACE_EXIT(FALSE);
    }

    for (i = 0; i < numDevSections; i++) {
        Bool isIsa = FALSE;
        Bool isPci = FALSE;
        dev = (char *)xf86FindOptionValue(devSections[i]->options, "vivante");
        if (devSections[i]->busID) {
#ifndef XSERVER_LIBPCIACCESS
            if (xf86ParsePciBusString(devSections[i]->busID,&bus,&device, &func)) {
                if (!xf86CheckPciSlot(bus,device,func))
                    continue;
                    isPci = TRUE;
            }
#endif
#ifdef HAVE_ISA
            if ( (isPci == FALSE) && xf86ParseIsaBusString(devSections[i]->busID))
                isIsa = TRUE;
#endif
        }

        if (fbdevHWProbe(NULL,(char *)dev,NULL)) {
            pScrn = NULL;
            if (isPci) {
#ifndef XSERVER_LIBPCIACCESS
                /* XXX what about when there's no busID set? */
                int entity;
                entity = xf86ClaimPciSlot(bus,device,func,drv, 0, devSections[i], TRUE);
                pScrn = (ScrnInfoPtr)xf86ConfigPciEntity(pScrn,0,entity, NULL, RES_SHARED_VGA, NULL, NULL, NULL, NULL);
                /* xf86DrvMsg() can't be called without setting these */
                pScrn->driverName    = VIVANTE_DRIVER_NAME;
                pScrn->name          = VIVANTE_NAME;
                xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "claimed PCI slot %d:%d:%d\n",bus,device,func);
#endif
            } else if (isIsa) {
#ifdef HAVE_ISA
                int entity;
                entity = xf86ClaimIsaSlot(drv, 0, devSections[i], TRUE);
                pScrn = xf86ConfigIsaEntity( pScrn, 0, entity, NULL, RES_SHARED_VGA, NULL, NULL, NULL, NULL);
#endif
            } else {
                int entity;
                entity = xf86ClaimFbSlot(drv, 0, devSections[i], TRUE);
                pScrn = xf86ConfigFbEntity(pScrn, 0, entity, NULL, NULL, NULL, NULL);
            }

            if (pScrn) {

                foundScreen = TRUE;
                pScrn->driverVersion = VIVANTE_VERSION;
                pScrn->driverName = VIVANTE_DRIVER_NAME;
                pScrn->name = VIVANTE_NAME;
                pScrn->Probe = VivProbe;
                pScrn->PreInit = VivPreInit;
                pScrn->ScreenInit = VivScreenInit;

#ifdef DEBUG
                pdump = (char *)xf86FindOptionValue(devSections[i]->options, "VDump");
                if ( pdump != NULL )
                {
                    if ( strcmp(pdump, "Xorg") == 0 )
                        vivEnableDump = VIV_XORG;
                    if ( strcmp(pdump, "Term") == 0 )
                        vivEnableDump = VIV_TERM;
                }
#endif

#ifdef ADD_FSL_XRANDR
                pOptions = (OptionInfoPtr) malloc( sizeof(VivOptions) );
                memcpy(pOptions, VivOptions, sizeof (VivOptions));
                xf86ProcessOptions(-1, devSections[i]->options, pOptions);
                if ( xf86ReturnOptValBool(pOptions, OPTION_XRANDR, TRUE)
                && !xf86ReturnOptValBool(pOptions, OPTION_SHADOW_FB, FALSE) )
                {
                    pScrn->FreeScreen    = VivFreeScreen;
                    pScrn->SwitchMode = imxDisplaySwitchMode;
                    pScrn->AdjustFrame = imxDisplayAdjustFrame;
                    pScrn->EnterVT = imxDisplayEnterVT;
                    pScrn->LeaveVT = imxDisplayLeaveVT;
                    pScrn->ValidMode = imxDisplayValidMode;
                    pScrn->PMEvent = imxPMEvent;
                    vivEnableXrandr = TRUE;
                }
                 else
#endif
                {
                    pScrn->SwitchMode = fbdevHWSwitchModeWeak();
                    pScrn->AdjustFrame = fbdevHWAdjustFrameWeak();
                    pScrn->EnterVT = fbdevHWEnterVTWeak();
                    pScrn->LeaveVT = fbdevHWLeaveVTWeak();
                    pScrn->ValidMode = fbdevHWValidModeWeak();
                }
#ifdef ADD_FSL_XRANDR
                if ( pOptions )
                    free(pOptions);
#endif
                xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "using %s\n", dev ? dev : "default device");
            }
        }
    }
    free(devSections);
    TRACE_EXIT(foundScreen);
}

static Bool
VivPreInit(ScrnInfoPtr pScrn, int flags) {
    VivPtr fPtr;
    int default_depth, fbbpp;
    const char *s;
    char *dev_node;

    TRACE_ENTER();

    if (flags & PROBE_DETECT) {
        TRACE_EXIT(FALSE);
    }

    /* Check the number of entities, and fail if it isn't one. */
    if (pScrn->numEntities != 1) {
        TRACE_ERROR("Not Just One Entry");
        TRACE_EXIT(FALSE);
    }

    /*Setting the monitor*/
    pScrn->monitor = pScrn->confScreen->monitor;
    /*Allocating the rectangle structure*/
    VivGetRec(pScrn);
    /*Getting a pointer to Rectangle Structure*/
    fPtr = GET_VIV_PTR(pScrn);

    /*Fetching the entity*/
    fPtr->pEnt = xf86GetEntityInfo(pScrn->entityList[0]);

#ifndef XSERVER_LIBPCIACCESS
    pScrn->racMemFlags = RAC_FB | RAC_COLORMAP | RAC_CURSOR | RAC_VIEWPORT;
    /* XXX Is this right?  Can probably remove RAC_FB */
    pScrn->racIoFlags = RAC_FB | RAC_COLORMAP | RAC_CURSOR | RAC_VIEWPORT;

    if (fPtr->pEnt->location.type == BUS_PCI &&
       xf86RegisterResources(fPtr->pEnt->index,NULL,ResExclusive)) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
            "xf86RegisterResources() found resource conflicts\n");
            return FALSE;
    }
#endif

 /*Getting the device path*/
    dev_node = (char *)xf86FindOptionValue(fPtr->pEnt->device->options, "fbdev");
    if (!dev_node) {
        dev_node = "/dev/fb2";
    }

    /* open device */
    if (!fbdevHWInit(pScrn, NULL, dev_node)) {
        TRACE_ERROR("CAN'T OPEN THE FRAMEBUFFER DEVICE");
        TRACE_EXIT(FALSE);
    }

#ifdef ADD_FSL_XRANDR
    strcpy(fPtr->fbDeviceName, dev_node + 5); // skip past "/dev/"
    /* get device preferred video mode */
    imxGetDevicePreferredMode(pScrn);

    /* save sync value */
    if(!SaveBuildInModeSyncFlags(pScrn))
        return FALSE;
#endif

    /*Get the default depth*/
    default_depth = fbdevHWGetDepth(pScrn, &fbbpp);
    if(fbbpp == 24)
            fbbpp = 32;
    /*Setting the depth Info*/
    if (!xf86SetDepthBpp(pScrn, default_depth, default_depth, fbbpp,
            Support24bppFb | Support32bppFb)) {
        TRACE_ERROR("DEPTH IS NOT SUPPORTED");
        TRACE_EXIT(FALSE);
    }

    /*Printing as info*/
    xf86PrintDepthBpp(pScrn);

    /* Get the depth24 pixmap format */
    if (pScrn->depth == 24 && pix24bpp == 0)
        pix24bpp = xf86GetBppFromDepth(pScrn, 24);

    /* color weight */
    if (pScrn->depth > 8) {
        rgb zeros = {0, 0, 0};
        if (!xf86SetWeight(pScrn, zeros, zeros)) {
            TRACE_ERROR("Color weight ");
            TRACE_EXIT(FALSE);
        }
    }

    /* visual init */
    if (!xf86SetDefaultVisual(pScrn, -1)) {
        TRACE_ERROR("Visual Init Problem");
        TRACE_EXIT(FALSE);
    }

    /* We don't currently support DirectColor at > 8bpp */
    if (pScrn->depth > 8 && pScrn->defaultVisual != TrueColor) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "requested default visual"
                " (%s) is not supported at depth %d\n",
                xf86GetVisualName(pScrn->defaultVisual), pScrn->depth);
        TRACE_EXIT(FALSE);
    }

    {
        Gamma zeros = {0.0, 0.0, 0.0};

        if (!xf86SetGamma(pScrn, zeros)) {
            TRACE_ERROR("Unable to Set Gamma\n");
            TRACE_EXIT(FALSE);
        }
    }

    pScrn->progClock = TRUE; /*clock is programmable*/
    if (pScrn->depth == 8) {
        pScrn->rgbBits = 8; /* 8 bits in r/g/b */
    }
    pScrn->chipset = "vivante";

    /* handle options */
    xf86CollectOptions(pScrn, NULL);
    if (!(fPtr->mSupportedOptions = malloc(sizeof (VivOptions)))) {
        TRACE_ERROR("Unable to allocate the supported Options\n");
        TRACE_EXIT(FALSE);
    }

    memcpy(fPtr->mSupportedOptions, VivOptions, sizeof (VivOptions));
    xf86ProcessOptions(pScrn->scrnIndex, fPtr->pEnt->device->options, fPtr->mSupportedOptions);

    vivEnableCacheMemory = xf86ReturnOptValBool(fPtr->mSupportedOptions, OPTION_VIVCACHEMEM, TRUE);

    fPtr->mFakeExa.mNoAccelFlag = xf86ReturnOptValBool(fPtr->mSupportedOptions, OPTION_NOACCEL, FALSE);

    /* dont use shadow framebuffer by default */
    fPtr->shadowFB = xf86ReturnOptValBool(fPtr->mSupportedOptions, OPTION_SHADOW_FB, FALSE);

    /* rotation */
    fPtr->rotate = VIV_ROTATE_NONE;
    if ((s = xf86GetOptValString(fPtr->mSupportedOptions, OPTION_ROTATE)))
    {
        if(!xf86NameCmp(s, "CW"))
        {
            fPtr->shadowFB = TRUE;
            fPtr->rotate = VIV_ROTATE_CW;
            xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
            "rotating screen clockwise\n");
        }
        else if(!xf86NameCmp(s, "CCW"))
        {
            fPtr->shadowFB = TRUE;
            fPtr->rotate = VIV_ROTATE_CCW;
            xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
            "rotating screen counter-clockwise\n");
        }
        else if(!xf86NameCmp(s, "UD"))
        {
            fPtr->shadowFB = TRUE;
            fPtr->rotate = VIV_ROTATE_UD;
            xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
            "rotating screen upside-down\n");
        }
        else
        {
            xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
            "\"%s\" is not a valid value for Option \"Rotate\"\n", s);
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
            "valid options are \"CW\", \"CCW\" and \"UD\"\n");
        }
    }

    if(fPtr->shadowFB) {
        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Shadow buffer enabled, 2D GPU acceleration disabled.\n");
        fPtr->mFakeExa.mUseExaFlag = FALSE;
        fPtr->mFakeExa.mNoAccelFlag = TRUE;
        vivEnableCacheMemory = FALSE;
    }

    if (fPtr->mFakeExa.mNoAccelFlag) {
        /*use null exa driver*/
        fPtr->mFakeExa.mUseExaFlag = TRUE;
        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    } else {
        char *s = (char *)xf86GetOptValString(fPtr->mSupportedOptions, OPTION_ACCELMETHOD);
        fPtr->mFakeExa.mNoAccelFlag = FALSE;
        fPtr->mFakeExa.mUseExaFlag = TRUE;
        if (s && xf86NameCmp(s, "EXA")) {
            fPtr->mFakeExa.mUseExaFlag = FALSE;
            fPtr->mFakeExa.mNoAccelFlag = TRUE;
        }
    }

    if ((s = xf86GetOptValString(fPtr->mSupportedOptions, OPTION_ACCELHW)))
    {
        if (!xf86NameCmp(s, "gal2d")) {
            fPtr->mGrCtx.mExaHwType = VIVGAL2D;
#ifdef HAVE_G2D
        }else if (!xf86NameCmp(s, "g2d")){
            fPtr->mGrCtx.mExaHwType = IMXG2D;
#endif
        }else{
            fPtr->mGrCtx.mExaHwType = VIVGAL2D;
        }
    } else {
#ifdef HAVE_G2D
        fPtr->mGrCtx.mExaHwType = IMXG2D;
#else
        fPtr->mGrCtx.mExaHwType = VIVGAL2D;
#endif
    }
    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,"mExaHwType:%d\n",fPtr->mGrCtx.mExaHwType);
    /* select video modes */

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "checking modes against framebuffer device...\n");
    fbdevHWSetVideoModes(pScrn);

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "checking modes against monitor...\n");
    {
        DisplayModePtr mode, first = mode = pScrn->modes;

        if (mode != NULL) do {
                mode->status = xf86CheckModeForMonitor(mode, pScrn->monitor);
                mode = mode->next;
            } while (mode != NULL && mode != first);

        xf86PruneDriverModes(pScrn);
    }

    if (NULL == pScrn->modes) {
#ifdef ADD_FSL_XRANDR
                if ( !vivEnableXrandr )
                {
                    fbdevHWUseBuildinMode(pScrn);
                }
#else
                    fbdevHWUseBuildinMode(pScrn);
#endif
    }
    /*setting current mode*/
    pScrn->currentMode = pScrn->modes;

    xf86PrintModes(pScrn);

    /* Set display resolution */
    xf86SetDpi(pScrn, 0, 0);

    if (xf86LoadSubModule(pScrn, "fb") == NULL) {
        VivFreeRec(pScrn);
        TRACE_ERROR("Unable to load fb submodule\n");
        TRACE_EXIT(FALSE);
    }

    /* Load shadow if needed */
    if (fPtr->shadowFB) {
        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "using shadow framebuffer\n");
        if (!xf86LoadSubModule(pScrn, "shadow")) {
            VivFreeRec(pScrn);
            return FALSE;
        }
    }

    /* Load EXA acceleration if needed */
    if (fPtr->mFakeExa.mUseExaFlag) {
        if (!xf86LoadSubModule(pScrn, "exa")) {
            VivFreeRec(pScrn);
            xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Error on loading exa submodule\n");
            TRACE_EXIT(FALSE);
        }
    }

#ifdef ADD_FSL_XRANDR
    /* init imx display engine */
    if ( vivEnableXrandr )
        imxDisplayPreInit(pScrn);
#endif


    pScrn->videoRam = fbdevHWGetVidmem(pScrn);

    /* make sure display width is correctly aligned */
    pScrn->displayWidth = gcmALIGN(pScrn->virtualX, fPtr->fbAlignWidth);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "VivPreInit: adjust display width %d\n",pScrn->displayWidth);
    TRACE_EXIT(TRUE);
}

static Bool
VivCreateScreenResources(ScreenPtr pScreen) {
    ScrnInfoPtr pScrn = GET_PSCR(pScreen);
    VivPtr fPtr = GET_VIV_PTR(pScrn);
    Bool ret;

    TRACE_ENTER();

    /*Setting up*/
    pScreen->CreateScreenResources = fPtr->CreateScreenResources;
    ret = pScreen->CreateScreenResources(pScreen);
    pScreen->CreateScreenResources = VivCreateScreenResources;

    if (!ret) {
        TRACE_ERROR("Unable to create screen resources\n");
        TRACE_EXIT(FALSE);
    }

    TRACE_EXIT(TRUE);
}
#ifdef ENABLE_VIVANTE_DRI3
extern Bool vivanteDRI3ScreenInit(ScreenPtr pScreen);
#endif
static Bool
VivScreenInit(SCREEN_INIT_ARGS_DECL)
{
    ScrnInfoPtr pScrn = GET_PSCR(pScreen);
    VivPtr fPtr = GET_VIV_PTR(pScrn);
    VisualPtr visual;
    int init_picture = 0;
    int ret, flags;
    int type;

    TRACE_ENTER();

    DEBUGP("\tbitsPerPixel=%d, depth=%d, defaultVisual=%s\n"
            "\tmask: %lu,%lu,%lu, offset: %lu,%lu,%lu\n",
            pScrn->bitsPerPixel,
            pScrn->depth,
            xf86GetVisualName(pScrn->defaultVisual),
            pScrn->mask.red, pScrn->mask.green, pScrn->mask.blue,
            pScrn->offset.red, pScrn->offset.green, pScrn->offset.blue);

    /*Init the hardware in current mode*/
    if (!fbdevHWModeInit(pScrn, pScrn->currentMode)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "mode initialization failed\n");
    }

    /*Mapping the Video memory*/
    if (NULL == (fPtr->mFB.mFBMemory = fbdevHWMapVidmem(pScrn))) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "mapping of video memory"
                " failed\n");
        TRACE_EXIT(FALSE);
    }
    /*Getting the  linear offset*/
    fPtr->mFB.mFBOffset = fbdevHWLinearOffset(pScrn);

    /*Setting the physcal addr*/
    fPtr->mFB.memPhysBase = pScrn->memPhysBase;

    /*Logical start address*/
    fPtr->mFB.mFBStart = fPtr->mFB.mFBMemory + fPtr->mFB.mFBOffset;

    /*Save Configuration*/
    fbdevHWSave(pScrn);
#ifdef ADD_FSL_XRANDR
    /* record last video mode for later hdmi hot plugout/in */
    if(fPtr->lastVideoMode) {
        xf86DeleteMode(&fPtr->lastVideoMode, fPtr->lastVideoMode);
    }
    /*pScrn->currentMode != NULL */
    fPtr->lastVideoMode = xf86DuplicateMode(pScrn->currentMode);

    /* init imx display engine */
    if ( vivEnableXrandr )
    {
        imxSetShadowBuffer(pScreen);
    }
#endif


    fbdevHWSaveScreen(pScreen, SCREEN_SAVER_ON);
    fbdevHWAdjustFrame(FBDEVHWADJUSTFRAME_ARGS(0, 0));

    /* mi layer */
    miClearVisualTypes();
    if (pScrn->bitsPerPixel > 8) {
        if (!miSetVisualTypes(pScrn->depth, TrueColorMask, pScrn->rgbBits, TrueColor)) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "visual type setup failed"
                    " for %d bits per pixel [1]\n",
                    pScrn->bitsPerPixel);
            TRACE_EXIT(FALSE);
        }
    } else {
        if (!miSetVisualTypes(pScrn->depth,
                miGetDefaultVisualMask(pScrn->depth),
                pScrn->rgbBits, pScrn->defaultVisual)) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "visual type setup failed"
                    " for %d bits per pixel [2]\n",
                    pScrn->bitsPerPixel);
            TRACE_EXIT(FALSE);
        }
    }
    if (!miSetPixmapDepths()) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "pixmap depth setup failed\n");
        return FALSE;
    }

    /*Pitch*/
    pScrn->displayWidth = fbdevHWGetLineLength(pScrn) /
            (pScrn->bitsPerPixel / 8);
    if (pScrn->displayWidth != pScrn->virtualX) {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                "Pitch updated to %d after ModeInit\n",
                pScrn->displayWidth);
    }

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
            "FB Start = %p  FB Base = %p  FB Offset = %p\n",
            gcmINT2PTR(fPtr->mFB.mFBStart), gcmINT2PTR(fPtr->mFB.mFBMemory), gcmINT2PTR(fPtr->mFB.mFBOffset));

    if(fPtr->rotate == VIV_ROTATE_CW || fPtr->rotate == VIV_ROTATE_CCW)
    {
            int tmp = pScrn->virtualX;
            pScrn->virtualX = pScrn->displayWidth = pScrn->virtualY;
            pScrn->virtualY = tmp;
    } else if (!fPtr->shadowFB) {
            /* FIXME: this doesn't work for all cases, e.g. when each scanline has a padding which is independent from the depth (controlfb) */
            pScrn->displayWidth = fbdevHWGetLineLength(pScrn) /(pScrn->bitsPerPixel / 8);
            if (pScrn->displayWidth != pScrn->virtualX) {
            xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Pitch updated to %d after ModeInit\n", pScrn->displayWidth);
            }
    }

    if(fPtr->rotate && !fPtr->PointerMoved) {
            fPtr->PointerMoved = pScrn->PointerMoved;
            pScrn->PointerMoved = VivPointerMoved;
    }

    if (fPtr->shadowFB) {
            fPtr->shadow = calloc(1, pScrn->virtualX * pScrn->virtualY * pScrn->bitsPerPixel);

            if (!fPtr->shadow) {
                xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to allocate shadow framebuffer\n");
                return FALSE;
            }
    }

    switch ((type = fbdevHWGetType(pScrn)))
    {
        case FBDEVHW_PACKED_PIXELS:
            switch (pScrn->bitsPerPixel) {
                case 8:
                case 16:
                case 24:
                case 32:
                    ret = fbScreenInit(pScreen, fPtr->shadowFB ? fPtr->shadow
                    : fPtr->mFB.mFBStart, pScrn->virtualX,
                    pScrn->virtualY, pScrn->xDpi,
                    pScrn->yDpi, pScrn->displayWidth,
                    pScrn->bitsPerPixel);
                    init_picture = 1;
                    break;
                default:
                    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "internal error: invalid number of bits per"
                    " pixel (%d) encountered in"
                    " VivScreenInit()\n", pScrn->bitsPerPixel);
                    ret = FALSE;
                    break;
            }
            break;
        case FBDEVHW_INTERLEAVED_PLANES:
            /* This should never happen, we should check for this much much earlier ... */
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "internal error: interleaved planes are not yet "
                    "supported by the fbdev driver\n");
            ret = FALSE;
            break;
        case FBDEVHW_TEXT:
            /* This should never happen, we should check for this much much earlier ... */
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "internal error: text mode is not supported by the "
                    "fbdev driver\n");
            ret = FALSE;
            break;
        case FBDEVHW_VGA_PLANES:
            /* Not supported yet */
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "internal error: EGA/VGA Planes are not yet "
                    "supported by the fbdev driver\n");
            ret = FALSE;
            break;
        default:
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "internal error: unrecognised hardware type (%d) "
                    "encountered in FBDevScreenInit()\n", type);
            ret = FALSE;
            break;
    }

    if (!ret)
        return FALSE;

    /* Fixup RGB ordering */
    if (pScrn->bitsPerPixel > 8) {
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

    /* must be after RGB ordering fixed */
    if (init_picture && !fbPictureInit(pScreen, NULL, 0)) {
        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
                "Render extension initialisation failed\n");
    }

    if (fPtr->shadowFB && !VivShadowInit(pScreen)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "shadow framebuffer initialization failed\n");
        return FALSE;
    }

    if (!fPtr->rotate)
        VivDGAInit(pScrn, pScreen);
    else {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "display rotated; disabling DGA\n");
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "using driver rotation; disabling XRandR\n");
        xf86DisableRandR();
        if (pScrn->bitsPerPixel == 24)
        xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "rotation might be broken at 24 bits per pixel\n");
    }

    fPtr->mFakeExa.mIsInited = FALSE;
    if (fPtr->mFakeExa.mUseExaFlag) {
        TRACE_INFO("Loading EXA");
        if (!InitExaLayer(pScreen)) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "internal error: initExaLayer failed "
                    "in VivScreenInit()\n");
        }
    }

    xf86SetBlackWhitePixels(pScreen);
#if XORG_VERSION_CURRENT > XORG_VERSION_NUMERIC(1,12,0,0,0)
#else
    miInitializeBackingStore(pScreen);
#endif
    xf86SetBackingStore(pScreen);

    pScrn->vtSema = TRUE;

    /* software cursor */
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    /* colormap */
    switch ((type = fbdevHWGetType(pScrn)))
    {
        /* XXX It would be simpler to use miCreateDefColormap() in all cases. */
        case FBDEVHW_PACKED_PIXELS:
            if (!miCreateDefColormap(pScreen)) {
                xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "internal error: miCreateDefColormap failed "
                "in FBDevScreenInit()\n");
                return FALSE;
            }
            break;
        case FBDEVHW_INTERLEAVED_PLANES:
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "internal error: interleaved planes are not yet "
                "supported by the fbdev driver\n");
            return FALSE;
        case FBDEVHW_TEXT:
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "internal error: text mode is not supported by "
                "the fbdev driver\n");
            return FALSE;
        case FBDEVHW_VGA_PLANES:
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "internal error: EGA/VGA planes are not yet "
                "supported by the fbdev driver\n");
            return FALSE;
        default:
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "internal error: unrecognised fbdev hardware type "
                "(%d) encountered in FBDevScreenInit()\n", type);
        return FALSE;
    }

    flags = CMAP_PALETTED_TRUECOLOR;
    if (!xf86HandleColormaps(pScreen, 256, 8, fbdevHWLoadPaletteWeak(),
            NULL, flags)) {
        TRACE_EXIT(FALSE);
    }

    xf86DPMSInit(pScreen, fbdevHWDPMSSetWeak(), 0);

    pScreen->SaveScreen = fbdevHWSaveScreenWeak();

    /* Wrap the current CloseScreen function */
    fPtr->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = VivCloseScreen;

    fPtr->CreateScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = VivCreateScreenResources;

    {
        XF86VideoAdaptorPtr *ptr;
        TRACE_INFO("Getting Adaptor");
        int n = xf86XVListGenericAdaptors(pScrn, &ptr);
        TRACE_INFO("Generic adaptor list = %d\n", n);
        if (n) {
            TRACE_INFO("BEFORE : pScreen= %p, ptr = %p n= %d\n", pScreen, ptr, n);
            xf86XVScreenInit(pScreen, ptr, n);
            TRACE_INFO("AFTER: pScreen= %p, ptr = %p n= %d\n", pScreen, ptr, n);
        }
    }


#ifdef ADD_FSL_XRANDR
    if ( vivEnableXrandr ) {
        if (!imxDisplayFinishScreenInit(pScrn->scrnIndex, pScreen)) {
            return FALSE;
        }
    }
#endif

#ifdef ENABLE_VIVANTE_DRI3
    if (!vivanteDRI3ScreenInit(pScreen))
    {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,"Fail to init DRI3\n");
    };
#else

#ifndef DISABLE_VIVANTE_DRI
    if (VivDRIScreenInit(pScreen)) {
        VivDRIFinishScreenInit(pScreen);
    }
#endif

#endif



    /* restore sync for FSL extension */
#ifdef ADD_FSL_XRANDR
    /* restore sync for FSL extension */
    if(!RestoreSyncFlags(pScrn))
        return FALSE;
#endif

    TRACE_EXIT(TRUE);
}

#ifdef ENABLE_VIVANTE_DRI3
extern void vivanteDRI3ScreenDeInit(ScreenPtr pScreen);
#endif

static Bool
VivCloseScreen(CLOSE_SCREEN_ARGS_DECL)
{
    ScrnInfoPtr pScrn = GET_PSCR(pScreen);
    VivPtr fPtr = GET_VIV_PTR(pScrn);
    Bool ret = FALSE;

    TRACE_ENTER();

#ifdef ENABLE_VIVANTE_DRI3
    vivanteDRI3ScreenDeInit(pScreen);
#else

#ifndef DISABLE_VIVANTE_DRI
    VivDRICloseScreen(pScreen);
#endif

#endif


    if (fPtr->mFakeExa.mUseExaFlag) {
        DEBUGP("UnLoading EXA");
        if (fPtr->mFakeExa.mIsInited && !DestroyExaLayer(pScreen)) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "internal error: DestroyExaLayer failed "
                    "in VivCloseScreen()\n");
        }
    }

    fbdevHWRestore(pScrn);
    fbdevHWUnmapVidmem(pScrn);
    if (fPtr->shadow) {
        shadowRemove(pScreen, pScreen->GetScreenPixmap(pScreen));
        free(fPtr->shadow);
        fPtr->shadow = NULL;
    }

    if (fPtr->pDGAMode) {
        free(fPtr->pDGAMode);
        fPtr->pDGAMode = NULL;
        fPtr->nDGAMode = 0;
    }

    pScrn->vtSema = FALSE;

    pScreen->CreateScreenResources = fPtr->CreateScreenResources;
    pScreen->CloseScreen = fPtr->CloseScreen;
    ret = (*pScreen->CloseScreen)(CLOSE_SCREEN_ARGS);

    TRACE_EXIT(ret);
}

static Bool VivDriverFunc(ScrnInfoPtr pScrn, xorgDriverFuncOp op,
        pointer ptr) {
    TRACE_ENTER();
    xorgHWFlags *flag;

    switch (op) {
        case GET_REQUIRED_HW_INTERFACES:
            flag = (CARD32*) ptr;
            (*flag) = 0;
            TRACE_EXIT(TRUE);
        default:
            TRACE_EXIT(FALSE);
    }
}

void
OnCrtcModeChanged(ScrnInfoPtr pScrn)
{
#ifndef DISABLE_VIVANTE_DRI
        VivUpdateDriScreen(pScrn);
#endif
}

#ifdef ADD_FSL_XRANDR
// call this function at startup
static Bool
SaveBuildInModeSyncFlags(ScrnInfoPtr pScrn)
{
    if(gEnableFbSyncExt) {
        int fdDev = fbdevHWGetFD(pScrn);
        struct fb_var_screeninfo fbVarScreenInfo;
        if (0 != ioctl(fdDev, FBIOGET_VSCREENINFO, &fbVarScreenInfo)) {

            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "unable to get VSCREENINFO %s\n",
                strerror(errno));
            return FALSE;
        }
        else {
            imxStoreSyncFlags(pScrn, "current", fbVarScreenInfo.sync);
        }
    }

    return TRUE;
}

static Bool
RestoreSyncFlags(ScrnInfoPtr pScrn)
{
    if(gEnableFbSyncExt) {
        char *modeName = "current";
        unsigned int fbSync = 0;
        if(pScrn->currentMode)
            modeName = (char*)pScrn->currentMode->name;

        if(!imxLoadSyncFlags(pScrn, modeName, &fbSync)) {
            xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
                "Failed to load FB_SYNC_ flags from storage for mode %s\n",
                modeName);
            return TRUE;
        }

        struct fb_var_screeninfo fbVarScreenInfo;
        int fdDev = fbdevHWGetFD(pScrn);
        if (-1 == fdDev) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "frame buffer device not available or initialized\n");
            return FALSE;
        }

        /* Query the FB variable screen info */
        if (0 != ioctl(fdDev, FBIOGET_VSCREENINFO, &fbVarScreenInfo)) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "unable to get FB VSCREENINFO for current mode: %s\n",
                strerror(errno));
            return FALSE;
        }

        fbVarScreenInfo.sync = fbSync;

        if (0 != ioctl(fdDev, FBIOPUT_VSCREENINFO, &fbVarScreenInfo)) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "unable to restore FB VSCREENINFO: %s\n",
                strerror(errno));
            return FALSE;
        }
    }

    return TRUE;
}
#endif

Bool vivante_fbdev_viv_probe(DriverPtr drv, int flags) {
    return VivProbe(drv, flags);
}
