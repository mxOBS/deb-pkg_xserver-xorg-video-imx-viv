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


#ifndef VIVANTE_H
#define VIVANTE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef USE_PROBE_VIV_FBDEV_DRIVER
#include <xf86drm.h>
#include "xf86Crtc.h"
#include "drmmode_display.h"
#endif

    /*GAL*/
#include "../vivante_gal/vivante_gal.h"

#define VIV_MAX_WIDTH   ((1 <<15) -1)
#define VIV_MAX_HEIGHT ((1 <<15) -1)
#define PIXMAP_PITCH_ALIGN    64

    /********************************************************************************
     *  Rectangle Structs (START)
     ********************************************************************************/

    /* Supported options */
    typedef enum {
        OPTION_SHADOW_FB,
        OPTION_ROTATE,
        OPTION_DUMP,
        OPTION_XRANDR,
        OPTION_ACCELHW,
        OPTION_VIV,
        OPTION_NOACCEL,
        OPTION_ACCELMETHOD,
        OPTION_VIVCACHEMEM,
        OPTION_SW_CURSOR,
        OPTION_DEVICE_PATH,
        OPTION_PAGEFLIP,
        OPTION_ZAPHOD_HEADS,
        OPTION_DOUBLE_SHADOW,
    } VivOpts;

    typedef struct _vivFakeExa {
        ExaDriverPtr mExaDriver;
        /*Feature Switches  For Exa*/
        Bool mNoAccelFlag;
        Bool mUseExaFlag;
        Bool mIsInited;
        /*Fake EXA Operations*/
        int op;
        PicturePtr pSrcPicture, pMaskPicture, pDstPicture;
        PixmapPtr pDst, pSrc, pMask;
        GCPtr pGC;
        CARD32 copytmpval[2];
        CARD32 solidtmpval[3];
    } VivFakeExa, *VivFakeExaPtr;

    typedef struct _fbInfo {
        void * mMappingInfo;
        unsigned long memPhysBase;
        unsigned char* mFBStart; /*logical memory start address*/
        unsigned char* mFBMemory; /*memory  address*/
        int mFBOffset; /*framebuffer offset*/
        unsigned long memGpuBase;
    } FBINFO, *FBINFOPTR;
#ifndef USE_PROBE_VIV_FBDEV_DRIVER
    typedef struct
    {
        int lastInstance;
        int refCount;
        ScrnInfoPtr pScrn_1;
        ScrnInfoPtr pScrn_2;
    } EntRec, *EntPtr;
#endif
#ifdef ADD_FSL_XRANDR
    typedef struct _fbSyncFlags {
        char * modeName;
        unsigned int syncFlags;
    } FBSYNCFLAGS;
#define MAX_MODES_SUPPORTED 256
#endif
    typedef struct _vivRec {
        /*Graphics Context*/
        GALINFO mGrCtx;
        /*FBINFO*/
        FBINFO mFB;
        /*EXA STUFF*/
        VivFakeExa mFakeExa;
        /*Entity & Options*/
        EntityInfoPtr pEnt; /*Entity To Be Used with this driver*/
        OptionInfoPtr mSupportedOptions; /*Options to be parsed in xorg.conf*/
        /*Funct Pointers*/
        CloseScreenProcPtr CloseScreen; /*Screen Close Function*/
#ifndef USE_PROBE_VIV_FBDEV_DRIVER
        CreateWindowProcPtr CreateWindow;
#endif
        CreateScreenResourcesProcPtr CreateScreenResources;

        /* DRI information */
        void * pDRIInfo;
        int drmSubFD;

        /* ---- from fb ----*/
        int         lineLength;
        int         rotate;
        Bool        shadowFB;
        void       *shadow;
        void      (*PointerMoved)(SCRN_ARG_TYPE arg, int x, int y);

        /* DGA info */
        DGAModePtr  pDGAMode;
        int         nDGAMode;

        /* ---- imx display section ----*/

        char    fbId[80];
        char    fbDeviceName[32];

        /* size of FB memory mapped; includes offset for page align */
        int    fbMemorySize;

        /* virtual addr for start 2nd FB memory for XRandR rotation */
        unsigned char*    fbMemoryStart2;

        /* total bytes FB memory to reserve for screen(s) */
        int    fbMemoryScreenReserve;

        /* frame buffer alignment properties */
        int    fbAlignOffset;
        int    fbAlignWidth;
        int    fbAlignHeight;

        /* Driver phase/state information */
        Bool suspended;

        void* displayPrivate;

        /* sync value: support FSL extension */
#ifdef ADD_FSL_XRANDR
        unsigned char*    fbMemoryStart2_noxshift; /* A fix to fbMemoryStart2 to make sure xoffset==0 */
        char  bootupVideoMode[64];
        DisplayModePtr  lastVideoMode;
        FBSYNCFLAGS fbSync[MAX_MODES_SUPPORTED];
#endif

#ifndef USE_PROBE_VIV_FBDEV_DRIVER
        /*newly added may need cleanup after reuse*/


        int fd;
        char *device_name;

        EntPtr entityPrivate;

        int Chipset;

#if XSERVER_LIBPCIACCESS
        struct pci_device *PciInfo;
#else
        pciVideoPtr PciInfo;
        PCITAG PciTag;
#endif

        Bool noAccel;


        /* Broken-out options. */
        OptionInfoPtr Options;

        unsigned int SaveGeneration;

        CreateScreenResourcesProcPtr createScreenResources;
        ScreenBlockHandlerProcPtr BlockHandler;
        void *driver;

        drmmode_rec drmmode;

        DamagePtr damage;
        Bool dirty_enabled;

        uint32_t cursor_width, cursor_height;

        struct dumb_bo *bo;
#endif
    } VivRec, * VivPtr,modesettingRec, *modesettingPtr;

    /********************************************************************************
     *  Rectangle Structs (END)
     ********************************************************************************/
#define GET_VIV_PTR(p) ((VivPtr)((p)->driverPrivate))
#define modesettingPTR(p) ((VivPtr)((p)->driverPrivate))

#if XORG_VERSION_CURRENT > XORG_VERSION_NUMERIC(1,12,0,0,0)
#define VIVPTR_FROM_PIXMAP(x)       \
        GET_VIV_PTR( xf86ScreenToScrn( (x)->drawable.pScreen) )
#define VIVPTR_FROM_SCREEN(x)       \
        GET_VIV_PTR(xf86ScreenToScrn(x))
#define VIVPTR_FROM_PICTURE(x)  \
        GET_VIV_PTR( xf86ScreenToScrn( (x)->pDrawable->pScreen) )

#else
#define VIVPTR_FROM_PIXMAP(x)       \
        GET_VIV_PTR(xf86Screens[(x)->drawable.pScreen->myNum])
#define VIVPTR_FROM_SCREEN(x)       \
        GET_VIV_PTR(xf86Screens[(x)->myNum])
#define VIVPTR_FROM_PICTURE(x)  \
        GET_VIV_PTR(xf86Screens[(x)->pDrawable->pScreen->myNum])


#endif

    /********************************************************************************
     *
     *  Macros For Access (END)
     *
     ********************************************************************************/

#ifdef __cplusplus
}
#endif

#endif  /* VIVANTE_H */

