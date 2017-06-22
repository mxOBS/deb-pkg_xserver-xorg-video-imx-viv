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


#include "vivante_common.h"
#include "compat-api.h"
#include "vivante.h"

#include "vivante_ext.h"
#include <errno.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <xorg/shadow.h>

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

/*Can be moved to seprate header*/
#ifndef USE_VIV_FBDEV_DRIVER
Bool vivante_fbdev_VivProbe(DriverPtr drv, int flags);
#endif
Bool imx_kms_probe(DriverPtr drv, int flags);


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
 * R Const/Dest Functions (END)
 ************************************************************************/
static const OptionInfoRec *VivAvailableOptions(int chipid, int busid);
static void VivIdentify(int flags);
static Bool VivProbe(DriverPtr drv, int flags);
#ifdef XSERVER_LIBPCIACCESS
static Bool VivPciProbe(DriverPtr drv, int entity_num, struct pci_device *dev, intptr_t match_data);
#endif


static Bool VivDriverFunc(ScrnInfoPtr pScrn, xorgDriverFuncOp op,
        pointer ptr);

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
    VivPciProbe,
#endif

};

/* Supported "chipsets" */
static SymTabRec VivChipsets[] = {
    {GC500_ID, GC500_STR},
    {GC2100_ID, GC2100_STR},
    {GCCORE_ID, GCCORE_STR},
    {-1, NULL}
};


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
/*TODO*/
#if 0
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
#endif
    }
    else {
        pScrn = NULL;
    }
    }

    return (pScrn != NULL);
}
#endif

/************************************************************************
 * START OF THE IMPLEMENTATION FOR CORE FUNCTIONS
 ************************************************************************/

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

static Bool
VivProbe(DriverPtr drv, int flags) {
    int i;
    ScrnInfoPtr pScrn;
    GDevPtr *devSections;
    int numDevSections;
    Bool isFB = FALSE;
    Bool isKMS = FALSE;
    Bool foundScreen = FALSE;

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
    for (i = 0; i < numDevSections; i++) {
        const char *devpath;

        devpath = (char *)xf86FindOptionValue(devSections[i]->options, "kmsdev");
        if(devpath){
            isKMS = 1;
            break;
        }
        devpath = (char *)xf86FindOptionValue(devSections[i]->options, "fbdev");
        if(devpath){
            isFB = 1;
            break;
        }
    }
    if(isFB){
        foundScreen = vivante_fbdev_VivProbe(drv, flags);
    }
    else if(isKMS){
        foundScreen = imx_kms_probe(drv, flags);
    }
    free(devSections);
    TRACE_EXIT(foundScreen);
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
