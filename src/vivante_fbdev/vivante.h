/****************************************************************************
*
*    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/


/*
 * File:   vivante.h
 * Author: vivante
 *
 * Created on November 9, 2011, 6:58 PM
 */

#ifndef VIVANTE_H
#define	VIVANTE_H

#ifdef __cplusplus
extern "C" {
#endif

    /*GAL*/
#include "vivante_gal.h"

    /********************************************************************************
     *  Rectangle Structs (START)
     ********************************************************************************/

    /* Supported options */
    typedef enum {
        OPTION_VIV,
        OPTION_NOACCEL,
        OPTION_ACCELMETHOD
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
        unsigned long memPhysBase;
        unsigned char* mFBStart; /*logical memory start address*/
        unsigned char* mFBMemory; /*memory  address*/
        int mFBOffset; /*framebuffer offset*/
    } FBINFO, *FBINFOPTR;

    typedef struct _vivRec {
        /*Graphics Context*/
        GALINFO mGrCtx;
        /*FBINFO*/
        FBINFO mFB;
        /*EXA STUFF*/
        VivFakeExa mFakeExa;
        /*Entity & Options*/
        EntityInfoPtr mEntity; /*Entity To Be Used with this driver*/
        OptionInfoPtr mSupportedOptions; /*Options to be parsed in xorg.conf*/
        /*Funct Pointers*/
        CloseScreenProcPtr CloseScreen; /*Screen Close Function*/
        CreateScreenResourcesProcPtr CreateScreenResources;

        /* DRI information */
        void * pDRIInfo;
        int drmSubFD;
    } VivRec, * VivPtr;

    /********************************************************************************
     *  Rectangle Structs (END)
     ********************************************************************************/
#define GET_VIV_PTR(p) ((VivPtr)((p)->driverPrivate))

#define VIVPTR_FROM_PIXMAP(x)		\
		GET_VIV_PTR(xf86Screens[(x)->drawable.pScreen->myNum])
#define VIVPTR_FROM_SCREEN(x)		\
		GET_VIV_PTR(xf86Screens[(x)->myNum])
#define VIVPTR_FROM_PICTURE(x)	\
		GET_VIV_PTR(xf86Screens[(x)->pDrawable->pScreen->myNum])

    /********************************************************************************
     *
     *  Macros For Access (END)
     *
     ********************************************************************************/

#ifdef __cplusplus
}
#endif

#endif	/* VIVANTE_H */

