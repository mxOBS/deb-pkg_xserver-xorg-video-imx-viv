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
 * File:   vivante_gal.h
 * Author: vivante
 *
 * Created on February 23, 2012, 11:58 AM
 */

#ifndef VIVANTE_GAL_H
#define	VIVANTE_GAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vivante_common.h"

    /*******************************************************************************
     *
     * Utility Macros (START)
     *
     ******************************************************************************/
#define IGNORE(a)  (a=a)
#define VIV_ALIGN( value, base ) (((value) + ((base) - 1)) & ~((base) - 1))
#define ARRAY_SIZE(a) (sizeof((a)) / (sizeof(*(a))))
#define NO_PICT_FORMAT -1
    /*******************************************************************************
     *
     * Utility Macros (END)
     *
     *******************************************************************************/

    /************************************************************************
     * STRUCTS & ENUMS(START)
     ************************************************************************/

    /*Cache Ops*/
    typedef enum _cacheOps {
        CLEAN,
        INVALIDATE,
        FLUSH,
        MEMORY_BARRIER
    } VIVFLUSHTYPE;

    /*Blit Code*/
    typedef enum _blitOpCode {
        VIVSOLID = 0,
        VIVCOPY,
        VIVCOMPOSITE
    } BlitCode;

    /*Format information*/
    typedef struct _vivPictFormats {
        int mExaFmt;
        int mBpp;
        unsigned int mVivFmt;
        int mAlphaBits;
    } VivPictFormat, *VivPictFmtPtr;

    /*Blending Operations*/
    typedef struct _vivBlendOps {
        int mOp;
        int mSrcBlendingFactor;
        int mDstBlendingFactor;
    } VivBlendOp, *VivBlendOpPtr;

    /*Rectangle*/
    typedef struct _vivBox {
        int x1;
        int y1;
        int x2;
        int y2;
    } VivBox, *VivBoxPtr;

    /*Prv Pixmap Structure*/
    typedef struct _vivPixmapPriv Viv2DPixmap;
    typedef Viv2DPixmap * Viv2DPixmapPtr;

    struct _vivPixmapPriv {
        /*Video Memory*/
        void * mVidMemInfo;
        /* Tracks pixmaps busy with GPU operation since last GPU sync. */
        Bool mGpuBusy;
        Viv2DPixmapPtr mNextGpuBusyPixmap;
        /*reference*/
        int mRef;
    };

    /*Surface Info*/
    typedef struct _vivSurfInfo {
        Viv2DPixmapPtr mPriv;
        VivPictFormat mFormat;
        unsigned int mWidth;
        unsigned int mHeight;
        unsigned int mStride;
    } VIV2DSURFINFO;

    /*Blit Info*/
    typedef struct _viv2DBlitInfo {
        /*Destination*/
        VIV2DSURFINFO mDstSurfInfo;
        /*Source*/
        VIV2DSURFINFO mSrcSurfInfo;
        /*Mask*/
        VIV2DSURFINFO mMskSurfInfo;
        /*BlitCode*/
        BlitCode mOperationCode;
        /*Operation Related*/
        VivBox mSrcBox;
        VivBox mDstBox;
        int mFgRop;
        int mBgRop;
        Pixel mColorARGB32; /*A8R8G8B8*/
        Bool mColorConvert;
        unsigned long mPlaneMask;
    } VIV2DBLITINFO, *VIV2DBLITINFOPTR;

    /*Gal Encapsulation*/
    typedef struct _GALINFO {
        /*Encapsulated blit info*/
        VIV2DBLITINFO mBlitInfo;
        /*Gpu busy pixmap linked list */
        Viv2DPixmapPtr mBusyPixmapLinkList;
        /*Gpu related*/
        void * mGpu;
    } GALINFO, *GALINFOPTR;

    /************************************************************************
     * STRUCTS & ENUMS (END)
     ************************************************************************/

    /************************************************************************
     * PIXMAP RELATED (START)
     ************************************************************************/
    /*Creating and Destroying Functions*/
    Bool CreateSurface(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr toBeUpdatedpPix);
    Bool WrapSurface(PixmapPtr pPixmap, void * logical, unsigned int physical, Viv2DPixmapPtr toBeUpdatedpPix);
    Bool DestroySurface(GALINFOPTR galInfo, Viv2DPixmapPtr ppriv);
    unsigned int GetStride(Viv2DPixmapPtr pixmap);
    /*Mapping Functions*/
    void * MapSurface(Viv2DPixmapPtr priv);
    void UnMapSurface(Viv2DPixmapPtr priv);
    /************************************************************************
     * PIXMAP RELATED (END)
     ************************************************************************/

    /************************************************************************
     * EXA RELATED UTILITY (START)
     ************************************************************************/
    Bool GetVivPictureFormat(int exa_fmt, VivPictFmtPtr viv);
    Bool GetDefaultFormat(int bpp, VivPictFmtPtr format);
    /************************************************************************
     *EXA RELATED UTILITY (END)
     ************************************************************************/

    /************************************************************************
     * GPU RELATED (START)
     ************************************************************************/
    Bool VIV2DGPUBlitComplete(GALINFOPTR galInfo, Bool wait);
    Bool VIV2DGPUFlushGraphicsPipe(GALINFOPTR galInfo);
    Bool VIV2DGPUCtxInit(GALINFOPTR galInfo);
    Bool VIV2DGPUCtxDeInit(GALINFOPTR galInfo);
    Bool VIV2DCacheOperation(GALINFOPTR galInfo, Viv2DPixmapPtr ppix, VIVFLUSHTYPE flush_type);
    /************************************************************************
     * GPU RELATED (END)
     ************************************************************************/

    /************************************************************************
     * 2D Operations (START)
     ************************************************************************/
    Bool SetSolidBrush(GALINFOPTR galInfo);
    Bool SetDestinationSurface(GALINFOPTR galInfo);
    Bool SetSourceSurface(GALINFOPTR galInfo);
    Bool SetClipping(GALINFOPTR galInfo);


    Bool DoSolidBlit(GALINFOPTR galInfo);
    Bool DoCopyBlit(GALINFOPTR galInfo);
    /************************************************************************
     * 2D Operations (END)
     ************************************************************************/


#ifdef __cplusplus
}
#endif

#endif	/* VIVANTE_GAL_H */

