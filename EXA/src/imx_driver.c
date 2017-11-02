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
#include "vivante.h"

#include <errno.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <xorg/shadow.h>

/************************************************************************
 * MACROS FOR VERSIONING & INFORMATION (START)
 ************************************************************************/
#define IMX_VERSION           1000
#define IMX_VERSION_MAJOR      1
#define IMX_VERSION_MINOR      0
#define IMX_VERSION_PATCHLEVEL 0
#define IMX_VERSION_STRING  "1.0"

#define IMX_NAME              "VIVANTE"
#define IMX_DRIVER_NAME       "vivante"

/*Can be moved to separate header*/
extern Bool vivante_fbdev_viv_probe(DriverPtr drv, int flags);
extern Bool imx_kms_probe(DriverPtr drv, int flags);


/************************************************************************
 * MACROS FOR VERSIONING & INFORMATION (END)
 ************************************************************************/

/************************************************************************
 * R Const/Dest Functions (END)
 ************************************************************************/
static const OptionInfoRec *imx_availableOptions(int chipid, int busid);
static void imx_identify(int flags);
static Bool imx_probe(DriverPtr drv, int flags);

static Bool imx_driver_func(ScrnInfoPtr pScrn, xorgDriverFuncOp op,
        pointer ptr);

/************************************************************************
 * X Window System Registration (START)
 ************************************************************************/
_X_EXPORT DriverRec IMX = {
    IMX_VERSION,
    IMX_DRIVER_NAME,
    imx_identify,
    imx_probe,
    imx_availableOptions,
    NULL,
    0,
    imx_driver_func,

};

/* Supported "chipsets" */
static SymTabRec imxChipsets[] = {
    {0, "i.MX6 with Vivante GCCORE"},
    {0, "i.MX7ULP with Vivante GCCORE"},
    {0, "i.MX8 with IMX G2D"},
    {-1, NULL}
};


static const OptionInfoRec imxOptions[] = {
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

MODULESETUPPROTO(imx_setup);

static XF86ModuleVersionInfo imxVersRec = {
    IMX_DRIVER_NAME,
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    IMX_VERSION_MAJOR,
    IMX_VERSION_MINOR,
    IMX_VERSION_PATCHLEVEL,
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    NULL,
    {0, 0, 0, 0}
};

_X_EXPORT XF86ModuleData vivanteModuleData = {&imxVersRec, imx_setup, NULL};

pointer
imx_setup(pointer module, pointer opts, int *errmaj, int *errmin) {
    TRACE_ENTER();
    pointer ret;
    static Bool setupDone = FALSE;

    if (!setupDone) {

        setupDone = TRUE;
        xf86AddDriver(&IMX, module, HaveDriverFuncs);
        ret = (pointer) 1;

    } else {
        if (errmaj) *errmaj = LDR_ONCEONLY;
        ret = (pointer) 0;

    }
    TRACE_EXIT(ret);
}
#endif /* XFree86LOADER */


/************************************************************************
 * START OF THE IMPLEMENTATION FOR CORE FUNCTIONS
 ************************************************************************/

static const OptionInfoRec *
imx_availableOptions(int chipid, int busid) {
    /*Chip id may also be used for special cases*/
    TRACE_ENTER();
    TRACE_EXIT(imxOptions);
}

static void
imx_identify(int flags) {
    TRACE_ENTER();
    xf86PrintChipsets(IMX_NAME, "Driver for i.MX Chipsets\n", imxChipsets);
    TRACE_EXIT();
}

static Bool
imx_probe(DriverPtr drv, int flags) {
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
    numDevSections = xf86MatchDevice(IMX_DRIVER_NAME, &devSections);
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
        foundScreen = vivante_fbdev_viv_probe(drv, flags);
    }
    else if(isKMS){
        foundScreen = imx_kms_probe(drv, flags);
    }
    free(devSections);
    TRACE_EXIT(foundScreen);
}

static Bool imx_driver_func(ScrnInfoPtr pScrn, xorgDriverFuncOp op,
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
