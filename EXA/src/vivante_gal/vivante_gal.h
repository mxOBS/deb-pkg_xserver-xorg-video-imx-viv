/****************************************************************************
*
*    Copyright 2012 - 2015 Vivante Corporation, Santa Clara, California.
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


#ifndef VIVANTE_GAL_H
#define VIVANTE_GAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAL/gc_hal.h"

#include "HAL/gc_hal_raster.h"

#include "HAL/gc_hal_base.h"

    /*******************************************************************************
     *
     * Utility Macros (START)
     *
     ******************************************************************************/
#define IGNORE(a)  (a=a)
#define VIV_ALIGN( value, base ) (((value) + ((base) - 1)) & ~((base) - 1))
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof((a)) / (sizeof(*(a))))
#endif
#define NO_PICT_FORMAT -1
    /*******************************************************************************
     *
     * Utility Macros (END)
     *
     *******************************************************************************/

    /************************************************************************
     * STRUCTS & ENUMS(START)
     ************************************************************************/

    /*Memory Map Info*/
    typedef struct _mmInfo {
        unsigned int mSize;
        void * mUserAddr;
        unsigned int physical;
        gctUINT32 handle;
    } MemMapInfo, *MemMapInfoPtr;

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
        VIVSIMCOPY,
        VIVCOPY,
        VIVCOMPOSITE_MASKED_SRC_REPEAT_PIXEL_ONLY_PATTERN,
        VIVCOMPOSITE_MASKED_SRC_REPEAT_ARBITRARY_SIZE_PATTERN,
        VIVCOMPOSITE_MASKED_SIMPLE,
        VIVCOMPOSITE_SRC_REPEAT_PIXEL_ONLY_PATTERN,
        VIVCOMPOSITE_SRC_REPEAT_ARBITRARY_SIZE_PATTERN,
        VIVCOMPOSITE_SIMPLE
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
        int width;
        int height;
    } VivBox, *VivBoxPtr;

    /*Prv Pixmap Structure*/
    typedef struct _vivPixmapPriv Viv2DPixmap;
    typedef Viv2DPixmap * Viv2DPixmapPtr;

    struct _vivPixmapPriv {
        /*Video Memory*/
        void * mVidMemInfo;
        Bool mHWPath;
        Bool mCpuBusy;
        Bool mSwAnyWay;
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
        unsigned int repeat;
        unsigned int repeatType;
        unsigned int alpha;
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
        VivBox mMskBox;
        /*Foreground and Background ROP*/
        int mFgRop;
        int mBgRop;
        /*Blending opeartion*/
        VivBlendOp mBlendOp;
        /*Transformation for source*/
        PictTransformPtr mTransform;
        Pixel mColorARGB32; /*A8R8G8B8*/
        Bool mColorConvert;
        unsigned long mPlaneMask;
        /*Rotation for source*/
        gceSURF_ROTATION mRotation;
        Bool mSwcpy;
        Bool mSwsolid;
        Bool mSwcmp;
        Bool mIsNotStretched;
        /* record old srcBox and dstBox */
        VivBox mOSrcBox;
        VivBox mODstBox;
        /*Source*/
        VIV2DSURFINFO mSrcTempSurfInfo;
        /* for scale */
        VIV2DSURFINFO mSrcTempSurfInfo1;
        /* for rotate */
        VIV2DSURFINFO mSrcTempSurfInfo2;
        /*Mask*/
        VIV2DSURFINFO mMskTempSurfInfo;
        /* for scale */
        VIV2DSURFINFO mMskTempSurfInfo1;
        /* for rotate */
        VIV2DSURFINFO mMskTempSurfInfo2;
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


    /* Format convertor */
    Bool VIVTransformSupported(PictTransform *ptransform,Bool *stretchflag);
    gceSURF_ROTATION VIVGetRotation(PictTransform *ptransform);
    void VIVGetSourceWH(PictTransform *ptransform, gctUINT32 deswidth, gctUINT32 desheight, gctUINT32 *srcwidth, gctUINT32 *srcheight );
    /************************************************************************
     * STRUCTS & ENUMS (END)
     ************************************************************************/

    /************************************************************************
     * PIXMAP RELATED (START)
     ************************************************************************/
    /*Creating and Destroying Functions*/
    Bool CreateSurface(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr toBeUpdatedpPix);
    Bool CleanSurfaceBySW(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr pPix);
    Bool WrapSurface(PixmapPtr pPixmap, void * logical, unsigned int physical, Viv2DPixmapPtr toBeUpdatedpPix);
    Bool ReUseSurface(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr toBeUpdatedpPix);
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
    char *MapViv2DPixmap(Viv2DPixmapPtr pdst);
    void VSetSurfIndex(int n);
    Bool VGetSurfAddrBy16(GALINFOPTR galInfo, int maxsize, int *phyaddr, int *lgaddr, int *width, int *height, int *stride);
    Bool VGetSurfAddrBy32(GALINFOPTR galInfo, int maxsize, int *phyaddr, int *lgaddr, int *width, int *height, int *stride);
    void VDestroySurf();
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
    Bool VIV2DGPUUserMemMap(char* logical, unsigned int physical, unsigned int size, void ** mappingInfo, unsigned int * gpuAddress);
    Bool VIV2DGPUUserMemUnMap(char* logical, unsigned int size, void * mappingInfo, unsigned int gpuAddress);
    Bool MapUserMemToGPU(GALINFOPTR galInfo, MemMapInfoPtr mmInfo);
    void UnmapUserMem(GALINFOPTR galInfo, MemMapInfoPtr mmInfo);
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
    void VIVSWComposite(PixmapPtr pxDst, int srcX, int srcY, int maskX, int maskY, int dstX, int dstY, int width, int height);
    Bool DoCompositeBlit(GALINFOPTR galInfo, VivBoxPtr opbox);
    Bool DoSolidBlit(GALINFOPTR galInfo);
    Bool DoCopyBlit(GALINFOPTR galInfo);
    Bool CopyBlitFromHost(MemMapInfoPtr mmInfo, GALINFOPTR galInfo);
    /************************************************************************
     * 2D Operations (END)
     ************************************************************************/
#ifdef __cplusplus
}
#endif

#endif  /* VIVANTE_GAL_H */

