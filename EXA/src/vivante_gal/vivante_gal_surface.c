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
#include "vivante_gal.h"
#include "vivante_priv.h"

#ifdef ENABLE_VIVANTE_DRI3
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <xf86drm.h>
#endif

extern Bool vivEnableCacheMemory;

/**
 *
 * @param Hal - Hardware abstraction layer object
 * @param Size - Size of the surface in bits
 * @param Pool - To allocate from which pool
 * @param Node - returned allocated video node
 * @return  - result of the process
 */
gceSTATUS AllocVideoNode(
    IN gcoHAL Hal,
    IN gctBOOL cacheable,
    IN gceSURF_TYPE surftype,
    IN OUT GenericSurfacePtr Surf
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmASSERT(Surf != gcvNULL);

#ifdef ENABLE_VIVANTE_DRI3
    {
        uint32_t node, tsNode;
        uint64_t bytes, pool;

        if (drm_vivante_bo_query(Surf->bo, DRM_VIV_GEM_PARAM_SIZE, &bytes))
        {
            gcmONERROR(gcvSTATUS_GENERIC_IO);
        }

        if (drm_vivante_bo_query(Surf->bo, DRM_VIV_GEM_PARAM_POOL, &pool))
        {
            gcmONERROR(gcvSTATUS_GENERIC_IO);
        }

        if (drm_vivante_bo_ref_node(Surf->bo, &node, &tsNode))
        {
            gcmONERROR(gcvSTATUS_GENERIC_IO);
        }

        Surf->mVideoNode.mNode  = (gctUINT64)node;
        Surf->mVideoNode.mPool  = (gcePOOL)pool;
        Surf->mVideoNode.mBytes = (gctUINT)bytes;
    }

#else
    {
        gcsHAL_INTERFACE iface;

        iface.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
        iface.u.AllocateLinearVideoMemory.bytes = Surf->mVideoNode.mBytes;
        iface.u.AllocateLinearVideoMemory.alignment = 64;
        iface.u.AllocateLinearVideoMemory.pool = Surf->mVideoNode.mPool;
        iface.u.AllocateLinearVideoMemory.type = surftype;
        iface.u.AllocateLinearVideoMemory.flag = cacheable ? gcvALLOC_FLAG_CACHEABLE : gcvALLOC_FLAG_NONE;

        /* Call kernel API. */
        gcmONERROR(gcoHAL_Call(Hal, &iface));

        /* Get allocated node in video memory. */
        Surf->mVideoNode.mNode  = iface.u.AllocateLinearVideoMemory.node;
        Surf->mVideoNode.mPool  = iface.u.AllocateLinearVideoMemory.pool;
        Surf->mVideoNode.mBytes = iface.u.AllocateLinearVideoMemory.bytes;
    }
#endif

OnError:
    return status;
}

/**
 *
 * @param Hal - Hardware abstraction layer object
 * @param Node - video node
 * @return result of the process
 */
gceSTATUS FreeVideoNode(
    IN gcoHAL Hal,
    IN gctUINT32 Node
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmASSERT(Node != gcvNULL);

    iface.command = gcvHAL_RELEASE_VIDEO_MEMORY;
    iface.u.ReleaseVideoMemory.node = Node;

    /* Call kernel API. */
    status = gcoHAL_Call(Hal, &iface);

    /* When unlock the video memory node, set event to the kernel,
    ** That is why Commit is needed here to make it work.
    */
    gcoHAL_Commit(gcvNULL, gcvFALSE);

    return status;
}



/**
 *  Use for  getting the physical and logical address
 * @param Hal Hardware abstraction layer object
 * @param Node video node
 * @param Address physical address
 * @param Memory logical address
 * @return result of the process
 */
gceSTATUS LockVideoNode(
    IN gcoHAL Hal,
    IN gctUINT32 Node,
    IN gctBOOL cacheable,
    OUT gctUINT32 *Address,
    OUT gctPOINTER *Memory
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmASSERT(Address != gcvNULL);
    gcmASSERT(Memory != gcvNULL);
    gcmASSERT(Node != gcvNULL);

    iface.engine = gcvENGINE_RENDER;
    iface.command = gcvHAL_LOCK_VIDEO_MEMORY;
    iface.u.LockVideoMemory.node = Node;
    iface.u.LockVideoMemory.cacheable = cacheable;
    /* Call kernel API. */
    gcmONERROR(gcoHAL_Call(Hal, &iface));

    /* Get allocated node in video memory. */
    *Address = iface.u.LockVideoMemory.address;
    *Memory  = gcmUINT64_TO_PTR(iface.u.LockVideoMemory.memory);

OnError:
    return status;
}

/**
 *
 * @param Hal
 * @param Node
 * @return
 */
gceSTATUS UnlockVideoNode(
    IN gcoHAL Hal,
    IN gctUINT32 Node,
    IN gceSURF_TYPE surftype
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmASSERT(Node != gcvNULL);

    iface.engine = gcvENGINE_RENDER;
    iface.ignoreTLS = gcvFALSE;
    iface.command = gcvHAL_UNLOCK_VIDEO_MEMORY;
    iface.u.UnlockVideoMemory.node = Node;
    iface.u.UnlockVideoMemory.type = surftype;
    iface.u.UnlockVideoMemory.asynchroneous = gcvTRUE;

    /* Call the kernel. */
    gcmONERROR(gcoHAL_Call(Hal, &iface));

    /* Do we need to schedule an event for the unlock? */
    if (iface.u.UnlockVideoMemory.asynchroneous)
    {
        iface.u.UnlockVideoMemory.asynchroneous = gcvFALSE;
        gcmONERROR(gcoHAL_ScheduleEvent(Hal, &iface));
    }

OnError:
    /* Call kernel API. */
    return status;
}

#define AL_WIDTH IMX_EXA_NONCACHESURF_WIDTH
#define AL_HEIGHT IMX_EXA_NONCACHESURF_HEIGHT
#define AL_SWIDTH 500
#define AL_SHEIGHT 500
typedef struct _gsurfpool {
struct _gsurfpool *pnext;
struct _gsurfpool *prev;
GenericSurfacePtr surface;
} GSURFPOOL,*PGSURFPOOL;

typedef struct _gpoolhead {
gctUINT num;
PGSURFPOOL pfirst;
PGSURFPOOL plast;
}GPOOLHEAD;

static GPOOLHEAD __gsmallpoolhead = {0, NULL, NULL};
static GPOOLHEAD __gmidpoolhead = {0, NULL, NULL};
static GPOOLHEAD __gbigpoolhead = {0, NULL, NULL};

static GPOOLHEAD *__gpoolhead = &__gsmallpoolhead;

//#define TEST_POOL 1

#define MAX_BNODE 6
#define MAX_MNODE 6
#define MAX_SNODE 6


static gctUINT MAX_NODE = MAX_SNODE;

#define SETPOOL(alwidth, alheight, bytesPerPixel) \
    do \
    { \
        if ((alwidth * alheight * bytesPerPixel) >= (AL_WIDTH * AL_HEIGHT *bytesPerPixel)) \
        { \
            __gpoolhead = &__gbigpoolhead; \
            MAX_NODE = MAX_BNODE; \
            break; \
        } \
        if ((alwidth * alheight * bytesPerPixel) <= (AL_SWIDTH * AL_SHEIGHT * bytesPerPixel)) \
        { \
            __gpoolhead = &__gsmallpoolhead; \
            MAX_NODE = MAX_SNODE; \
            break; \
        } \
        __gpoolhead = &__gmidpoolhead; \
        MAX_NODE = MAX_MNODE; \
    } while(0)



/* When surface will be destroyed, call this to add it into pool */
/* Return non-null, which means the user has to destroy the ret surf */
/* Return null, which means the user has not to do the next destroy procedure */
static GenericSurfacePtr AddGSurfIntoPool(GenericSurfacePtr psurface)
{
#ifdef ENABLE_VIVANTE_DRI3
    return psurface;
#else
    PGSURFPOOL poolnode = NULL;
    PGSURFPOOL pnextnode = NULL;
    GenericSurfacePtr pretsurf = NULL;

    if ( psurface == NULL) return NULL;


    SETPOOL((psurface->mAlignedWidth), (psurface->mAlignedHeight), (psurface->mBytesPerPixel));

    gcmASSERT(__gpoolhead->num <= MAX_NODE);
    gcmASSERT( 3 <= MAX_NODE);

    if ( __gpoolhead->num == MAX_NODE )
    {
        if ( __gpoolhead->plast->surface->mVideoNode.mBytes >= psurface->mVideoNode.mBytes )
            return psurface;

        poolnode = __gpoolhead->plast;

        pretsurf = poolnode->surface;

        __gpoolhead->plast->prev->pnext = NULL;
        __gpoolhead->plast = __gpoolhead->plast->prev;
        __gpoolhead->num--;

        //free(poolnode);
    }

    if ( poolnode == NULL )
        poolnode = (PGSURFPOOL)malloc(sizeof(GSURFPOOL));

    poolnode->surface = psurface;
    poolnode->pnext = NULL;
    poolnode->prev= NULL;

    if ( __gpoolhead->pfirst == NULL) {

        poolnode->prev = poolnode;
        __gpoolhead->pfirst = poolnode;
        __gpoolhead->plast = poolnode;
        __gpoolhead->num = 1;
        return NULL;
    }

    /* if pfirst is not NULL, plast must be non-null */
    pnextnode = __gpoolhead->pfirst;

    while ( pnextnode )
    {
        if ( pnextnode->surface->mVideoNode.mBytes > psurface->mVideoNode.mBytes )
        {
            pnextnode = pnextnode->pnext;
            continue;
        }
        break;
    }

    if ( pnextnode == NULL )
    {
        poolnode->prev= __gpoolhead->plast;
        __gpoolhead->plast->pnext = poolnode;
        __gpoolhead->plast =poolnode;
    } else {
        if ( pnextnode == __gpoolhead->pfirst )
        {
            poolnode->pnext = pnextnode;
            pnextnode->prev = poolnode;
            __gpoolhead->pfirst = poolnode;
            poolnode->prev = __gpoolhead->pfirst;
        } else {
            poolnode->pnext = pnextnode;
            poolnode->prev = pnextnode->prev;
            pnextnode->prev->pnext = poolnode;
            pnextnode->prev = poolnode;
        }
    }

    __gpoolhead->num++;


#ifdef TEST_POOL
    if ( !TestSurfPool() )
        fprintf(stderr,"Surf Pool Gets ERR when adding node \n");
#endif

    return pretsurf;
#endif
}

/* Grab a surf from the pool, if return null, You have to allocate the new surface */
/* Otherwise you get surface from the pool, you have not to allocate the surface */
static GenericSurfacePtr GrabSurfFromPool(gctUINT alignedwidth, gctUINT alignedheight, gctUINT bytesPerPixel)
{
#ifdef ENABLE_VIVANTE_DRI3
    return NULL;
#else
    PGSURFPOOL pnextnode = NULL;
    GenericSurfacePtr pret = NULL;
    gctUINT size = 0;


    SETPOOL(alignedwidth, alignedheight, bytesPerPixel);

    if ( __gpoolhead->pfirst == NULL )
    {
        gcmASSERT( __gpoolhead->num == 0 );
        return NULL;
    }

    size = alignedwidth * alignedheight * bytesPerPixel;

    pnextnode = __gpoolhead->pfirst;
    while ( pnextnode )
    {
        if ( pnextnode->surface->mVideoNode.mBytes >= size )
        {
            if ( pnextnode->pnext )
                pnextnode->pnext->prev = pnextnode->prev;

            pnextnode->prev->pnext = pnextnode->pnext;

            if ( pnextnode == __gpoolhead->pfirst)
            {
                __gpoolhead->pfirst = pnextnode->pnext;
                if ( pnextnode->pnext)
                pnextnode->pnext->prev = __gpoolhead->pfirst;
            }

            if (pnextnode == __gpoolhead->plast ) {
                if ( __gpoolhead->pfirst == NULL)
                    __gpoolhead->plast = NULL;
                else
                    __gpoolhead->plast = pnextnode->prev;
            }

            pret = pnextnode->surface;

            __gpoolhead->num--;

            free(pnextnode);
            break;
        }

        pnextnode = pnextnode->pnext;

    }

#ifdef TEST_POOL
    if ( !TestSurfPool() )
        fprintf(stderr,"Surf Pool Gets ERR when grabbing node \n");
#endif

    return pret;
#endif
}

/************************************************************************
 * PIXMAP RELATED (START)
************************************************************************/

static void FreeGenericGPUSurface(VIVGPUPtr gpuctx, GenericSurfacePtr surf)
{
#ifdef ENABLE_VIVANTE_DRI3
    gcmASSERT(surf);

    if (surf->fd >= 0)
    {
        close(surf->fd);
        surf->fd = -1;
    }

    if (surf->bo)
    {
        drm_vivante_bo_destroy(surf->bo);
        surf->bo = gcvNULL;
    }
#endif
}

static gctBOOL FreeGPUSurface(VIVGPUPtr gpuctx, Viv2DPixmapPtr ppriv) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    GenericSurfacePtr surf = gcvNULL;
    gceSURF_TYPE surftype;

    surf = (GenericSurfacePtr) (ppriv->mVidMemInfo);
    if (surf->mIsWrapped) {
        goto delete_wrapper;
    }
    TRACE_INFO("DESTROYED SURFACE ADDRESS = %p - %p\n", surf, ppriv->mVidMemInfo);

    surf = AddGSurfIntoPool(surf);

    if ( surf ==NULL )
    {
        ppriv->mVidMemInfo = NULL;
        TRACE_EXIT(gcvTRUE);
    }

    if ( surf->mData )
        pixman_image_unref( (pixman_image_t *)surf->mData );

    surf->mData = gcvNULL;

#if ALL_NONCACHE_BIGSURFACE
    if ( surf->mAlignedWidth >= IMX_EXA_NONCACHESURF_WIDTH && surf->mAlignedHeight >= IMX_EXA_NONCACHESURF_HEIGHT )
    {
        surftype = gcvSURF_BITMAP;
        cacheable = FALSE;
    } else {
#endif
        if (vivEnableCacheMemory) {
            surftype = gcvSURF_BITMAP;
            surf->mVideoNode.mPool = gcvPOOL_CONTIGUOUS;
        } else {
            surftype = gcvSURF_BITMAP;
        }

#if ALL_NONCACHE_BIGSURFACE
    }
#endif

    if (surf->mVideoNode.mNode != 0) {
        if (surf->mVideoNode.mLogicalAddr != gcvNULL) {
            status = UnlockVideoNode(gpuctx->mDriver->mHal, surf->mVideoNode.mNode, surftype);
            if (status != gcvSTATUS_OK) {
                TRACE_ERROR("Unable to UnLock video node\n");
                TRACE_EXIT(gcvFALSE);
            }
        }
        status = FreeVideoNode(gpuctx->mDriver->mHal, surf->mVideoNode.mNode);
        if (status != gcvSTATUS_OK) {
            TRACE_ERROR("Unable to Free video node\n");
            TRACE_EXIT(gcvFALSE);
        }

        FreeGenericGPUSurface(gpuctx, surf);
    }

delete_wrapper:
    status = gcoOS_Free(gcvNULL, surf);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Unable to Free surface\n");
        TRACE_EXIT(gcvFALSE);
    }
    ppriv->mVidMemInfo = NULL;

    TRACE_EXIT(gcvTRUE);
}

static gctBOOL
VIV2DGPUSurfaceAlloc(
    VIVGPUPtr gpuctx,
    gctUINT alignedWidth,
    gctUINT alignedHeight,
    gctUINT bytesPerPixel,
    GenericSurfacePtr * surface
    )
{
    TRACE_ENTER();

    gceSTATUS status = gcvSTATUS_OK;
    GenericSurfacePtr surf = gcvNULL;
    gctBOOL allocated = gcvFALSE;
    gctBOOL ret = gcvTRUE;

    surf = GrabSurfFromPool(alignedWidth, alignedHeight, bytesPerPixel);

    if (surf == NULL)
    {
        gctBOOL cacheable = gcvFALSE;
        gceSURF_TYPE surftype = gcvSURF_TYPE_UNKNOWN;

        gcmONERROR(gcoOS_Allocate(gcvNULL, sizeof(GenericSurface), (gctPOINTER*)&surf));
        allocated = gcvTRUE;

        memset(surf, 0, sizeof(GenericSurface));
        surf->mVideoNode.mBytes = alignedWidth * bytesPerPixel * alignedHeight;
        surf->mVideoNode.mPool = gcvPOOL_DEFAULT;

#if ALL_NONCACHE_BIGSURFACE
        if (alignedWidth >= IMX_EXA_NONCACHESURF_WIDTH && alignedHeight >= IMX_EXA_NONCACHESURF_HEIGHT)
        {
            surftype = gcvSURF_BITMAP;
            cacheable = gcvFALSE;
        }
        else
#endif
        {
            if (vivEnableCacheMemory)
            {
                surftype = gcvSURF_BITMAP;
                cacheable = gcvTRUE;
                surf->mVideoNode.mPool = gcvPOOL_CONTIGUOUS;
            }
            else
            {
                surftype = gcvSURF_BITMAP;
                cacheable = gcvFALSE;
            }
        }

#ifdef ENABLE_VIVANTE_DRI3
        cacheable = gcvFALSE;

        surf->fd = -1;
        if (drm_vivante_bo_create(gpuctx->mDriver->drm, 0, surf->mVideoNode.mBytes, &surf->bo))
        {
            TRACE_ERROR("Failed to create drm create drm_vivante_bo\n");
            gcmONERROR(gcvSTATUS_GENERIC_IO);
        }

        if (drm_vivante_bo_export_to_fd(surf->bo, &surf->fd))
        {
            TRACE_ERROR("Failed to export drm_vivante_bo to fd\n");
            gcmONERROR(gcvSTATUS_GENERIC_IO);
        }
#endif

        gcmONERROR(AllocVideoNode(gpuctx->mDriver->mHal, cacheable, surftype, surf));
        gcmONERROR(LockVideoNode(gpuctx->mDriver->mHal, (gctUINT32)surf->mVideoNode.mNode, cacheable,
                                 &surf->mVideoNode.mPhysicalAddr, &surf->mVideoNode.mLogicalAddr));

        TRACE_INFO("VIDEO NODE CREATED =>  LOGICAL = %p  PHYSICAL = 0x%x  SIZE = 0x%x\n",
                   surf->mVideoNode.mLogicalAddr, surf->mVideoNode.mPhysicalAddr, surf->mVideoNode.mBytes);
    }

    surf->mTiling = gcvLINEAR;
    surf->mAlignedWidth = alignedWidth;
    surf->mAlignedHeight = alignedHeight;
    surf->mBytesPerPixel = bytesPerPixel;
    surf->mStride = alignedWidth * bytesPerPixel;
    surf->mRotation = gcvSURF_0_DEGREE;
    surf->mLogicalAddr = surf->mVideoNode.mLogicalAddr;
    surf->mIsWrapped = gcvFALSE;
    surf->mData = gcvNULL;
    *surface = surf;

OnError:
    if (gcmIS_ERROR(status) && allocated)
    {
        gcmASSERT(surf);

        if (surf->mVideoNode.mNode)
        {
            gcmVERIFY_OK(FreeVideoNode(gpuctx->mDriver->mHal, surf->mVideoNode.mNode));
        }

        FreeGenericGPUSurface(gpuctx, surf);
        gcmOS_SAFE_FREE(gcvNULL, surf);

        ret = gcvFALSE;
    }

    TRACE_EXIT(ret);
}

static gctBOOL
VIV2DGPUSurfaceAllocWithFd(
    VIVGPUPtr gpuctx,
    gctUINT alignedWidth,
    gctUINT alignedHeight,
    gctUINT bytesPerPixel,
    GenericSurfacePtr * surface,
    int fd
    )
{
    TRACE_ENTER();

    gceSTATUS status = gcvSTATUS_OK;
    GenericSurfacePtr surf = gcvNULL;
    gctBOOL allocated = gcvFALSE;
    gctBOOL ret = gcvTRUE;

    surf = GrabSurfFromPool(alignedWidth, alignedHeight, bytesPerPixel);

    if (surf == NULL)
    {
        gctBOOL cacheable = gcvFALSE;
        gceSURF_TYPE surftype = gcvSURF_TYPE_UNKNOWN;

        gcmONERROR(gcoOS_Allocate(gcvNULL, sizeof(GenericSurface), (gctPOINTER*)&surf));
        allocated = gcvTRUE;

        memset(surf, 0, sizeof(GenericSurface));
        surf->mVideoNode.mBytes = alignedWidth * bytesPerPixel * alignedHeight;
        surf->mVideoNode.mPool = gcvPOOL_DEFAULT;

#if ALL_NONCACHE_BIGSURFACE
        if (alignedWidth >= IMX_EXA_NONCACHESURF_WIDTH && alignedHeight >= IMX_EXA_NONCACHESURF_HEIGHT)
        {
            surftype = gcvSURF_BITMAP;
            cacheable = FALSE;
        } else
#endif
        {
            if (vivEnableCacheMemory)
            {
                surftype = gcvSURF_BITMAP;
                cacheable = TRUE;
                surf->mVideoNode.mPool = gcvPOOL_CONTIGUOUS;
            }
            else
            {
                surftype = gcvSURF_BITMAP;
                cacheable = FALSE;
            }
        }

#ifdef ENABLE_VIVANTE_DRI3
        cacheable = gcvFALSE;

        surf->fd = -1;
        if (drm_vivante_bo_import_from_fd(gpuctx->mDriver->drm, fd, &surf->bo))
        {
            TRACE_ERROR("Failed to create drm create drm_vivante_bo\n");
            gcmONERROR(gcvSTATUS_GENERIC_IO);
        }
#endif

        gcmONERROR(AllocVideoNode(gpuctx->mDriver->mHal, cacheable, surftype, surf));
        gcmONERROR(LockVideoNode(gpuctx->mDriver->mHal, (gctUINT32)surf->mVideoNode.mNode, cacheable,
                                 &surf->mVideoNode.mPhysicalAddr, &surf->mVideoNode.mLogicalAddr));

        TRACE_INFO("VIDEO NODE CREATED =>  LOGICAL = %p  PHYSICAL = 0x%x  SIZE = 0x%x\n",
                   surf->mVideoNode.mLogicalAddr, surf->mVideoNode.mPhysicalAddr, surf->mVideoNode.mBytes);
    }

    surf->mTiling = gcvLINEAR;
    surf->mAlignedWidth = alignedWidth;
    surf->mAlignedHeight = alignedHeight;
    surf->mBytesPerPixel = bytesPerPixel;
    surf->mStride = alignedWidth * bytesPerPixel;
    surf->mRotation = gcvSURF_0_DEGREE;
    surf->mLogicalAddr = surf->mVideoNode.mLogicalAddr;
    surf->mIsWrapped = gcvFALSE;
    surf->mData = gcvNULL;
    *surface = surf;

OnError:
    if (gcmIS_ERROR(status) && allocated)
    {
        gcmASSERT(surf);

        if (surf->mVideoNode.mNode)
        {
            gcmVERIFY_OK(FreeVideoNode(gpuctx->mDriver->mHal, surf->mVideoNode.mNode));
        }

        FreeGenericGPUSurface(gpuctx, surf);
        gcmOS_SAFE_FREE(gcvNULL, surf);

        ret = gcvFALSE;
    }

    TRACE_EXIT(ret);
}

Bool ReUseSurface(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr toBeUpdatedpPix)
{
#ifdef ENABLE_VIVANTE_DRI3
    TRACE_EXIT(FALSE);
#else
    GenericSurfacePtr surf = gcvNULL;
    gctUINT alignedWidth, alignedHeight;
    gctUINT bytesPerPixel;
    alignedWidth = gcmALIGN(pPixmap->drawable.width, WIDTH_ALIGNMENT);
    alignedHeight = gcmALIGN(pPixmap->drawable.height, HEIGHT_ALIGNMENT);
    bytesPerPixel = BITSTOBYTES(pPixmap->drawable.bitsPerPixel);

    /* The same as CreatSurface */
    if (bytesPerPixel < 2) {
        bytesPerPixel = 2;
    }

    surf = (GenericSurfacePtr)toBeUpdatedpPix->mVidMemInfo;
    if ( surf && surf->mVideoNode.mBytes >= (alignedWidth * alignedHeight * bytesPerPixel))
    {
        surf->mTiling = gcvLINEAR;
        surf->mAlignedWidth = alignedWidth;
        surf->mAlignedHeight = alignedHeight;
        surf->mStride = alignedWidth * bytesPerPixel;
        surf->mRotation = gcvSURF_0_DEGREE;
        surf->mLogicalAddr = surf->mVideoNode.mLogicalAddr;
        surf->mIsWrapped = gcvFALSE;

        if ( surf->mData )
            pixman_image_unref( (pixman_image_t *)surf->mData );

        surf->mData = gcvNULL;
        TRACE_EXIT(TRUE);
    }

    TRACE_EXIT(FALSE);
#endif
}

/*Creating and Destroying Functions*/
Bool CreateSurface(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr pPix) {
    GenericSurfacePtr surf = gcvNULL;
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    gctUINT alignedWidth, alignedHeight;
    gctUINT bytesPerPixel;
    alignedWidth = gcmALIGN(pPixmap->drawable.width, WIDTH_ALIGNMENT);
    alignedHeight = gcmALIGN(pPixmap->drawable.height, HEIGHT_ALIGNMENT);
    bytesPerPixel = BITSTOBYTES(pPixmap->drawable.bitsPerPixel);

    /*QUICK FIX*/
    if (bytesPerPixel < 2) {
        bytesPerPixel = 2;
    }

    if (!VIV2DGPUSurfaceAlloc(gpuctx, alignedWidth, alignedHeight, bytesPerPixel, &surf)) {
        TRACE_ERROR("Surface Creation Error\n");
        TRACE_EXIT(FALSE);
    }

    pPix->mVidMemInfo = surf;
    TRACE_EXIT(TRUE);
}

/*Creating and Destroying Functions*/
Bool CreateSurfaceWithFd(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr pPix, int fd) {
    GenericSurfacePtr surf = gcvNULL;
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    gctUINT alignedWidth, alignedHeight;
    gctUINT bytesPerPixel;
    alignedWidth = gcmALIGN(pPixmap->drawable.width, WIDTH_ALIGNMENT);
    alignedHeight = gcmALIGN(pPixmap->drawable.height, HEIGHT_ALIGNMENT);
    bytesPerPixel = BITSTOBYTES(pPixmap->drawable.bitsPerPixel);

    /*QUICK FIX*/
    if (bytesPerPixel < 2) {
        bytesPerPixel = 2;
    }

    if (!VIV2DGPUSurfaceAllocWithFd(gpuctx, alignedWidth, alignedHeight, bytesPerPixel, &surf, fd)) {
        TRACE_ERROR("Surface Creation Error\n");
        TRACE_EXIT(FALSE);
    }

    pPix->mVidMemInfo = surf;
    TRACE_EXIT(TRUE);
}

Bool CleanSurfaceBySW(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr pPix)
{
    GenericSurfacePtr surf = NULL;
    VIVGPUPtr gpuctx = (VIVGPUPtr) (galInfo->mGpu);
    gceSTATUS status = gcvSTATUS_OK;
    VivPictFormat mFormat;
    gcsRECT dstRect = {0, 0, pPixmap->drawable.width, pPixmap->drawable.height};

    if ( pPix == NULL )
        TRACE_EXIT(FALSE);
    surf = (GenericSurfacePtr)pPix->mVidMemInfo;

#ifdef HAVE_G2D

/* Make compiler happer */

    mFormat = mFormat;
    dstRect = dstRect;

    pPix->mCpuBusy = TRUE;
    memset((char *)surf->mVideoNode.mLogicalAddr,0,surf->mVideoNode.mBytes);

#else
    pPix->mCpuBusy = FALSE;

    if (!GetDefaultFormat(pPixmap->drawable.bitsPerPixel, &mFormat)) {
        TRACE_EXIT(FALSE);
    }

    status = gco2D_SetGenericTarget
            (
            gpuctx->mDriver->m2DEngine,
            &surf->mVideoNode.mPhysicalAddr,
            1,
            &surf->mStride,
            1,
            surf->mTiling,
            mFormat.mVivFmt,
            surf->mRotation,
            surf->mAlignedWidth,
            surf->mAlignedHeight
            );
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("In CleanSurfaceBySW gco2D_SetGenericTarget failed\n");
        TRACE_EXIT(FALSE);
    }

    status = gco2D_SetClipping(gpuctx->mDriver->m2DEngine, &dstRect);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("In CleanSurfaceBySW gco2D_SetClipping failed\n");
        TRACE_EXIT(FALSE);
    }

    status = gco2D_LoadSolidBrush
            (
            gpuctx->mDriver->m2DEngine,
            (gceSURF_FORMAT) mFormat.mVivFmt,
            gcvFALSE,
            0,
            (gctUINT64)(0xFFFFFFFFFFFFFFFF)
            );

    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("In CleanSurfaceBySW gco2D_LoadSolidBrush failed\n");
        TRACE_EXIT(FALSE);
    }
    status = gco2D_Clear(gpuctx->mDriver->m2DEngine, 1, &dstRect, 0, 0xCC, 0xCC, mFormat.mVivFmt);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("In CleanSurfaceBySW gco2D_Clear failed\n");
        TRACE_EXIT(FALSE);
    }

    VIV2DGPUBlitComplete(galInfo, TRUE);

#endif

    TRACE_EXIT(TRUE);

}

Bool WrapSurface(PixmapPtr pPixmap, void * logical, unsigned int physical, Viv2DPixmapPtr pPix, int bytes) {
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT alignedWidth, alignedHeight;
    gctUINT bytesPerPixel;
    GenericSurfacePtr surf = gcvNULL;
    gctPOINTER mHandle = gcvNULL;
    status = gcoOS_Allocate(gcvNULL, sizeof (GenericSurface), &mHandle);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Unable to allocate generic surface\n");
        TRACE_EXIT(FALSE);
    }
    memset(mHandle, 0, sizeof(GenericSurface));
    surf = (GenericSurfacePtr)mHandle;

    bytesPerPixel = BITSTOBYTES(pPixmap->drawable.bitsPerPixel);
    alignedWidth = gcmALIGN(pPixmap->devKind/bytesPerPixel, WIDTH_ALIGNMENT);
    alignedHeight = gcmALIGN(pPixmap->drawable.height, HEIGHT_ALIGNMENT);

    surf->mVideoNode.mBytes = bytes;
    surf->mVideoNode.mPool = gcvPOOL_USER;

    surf->mVideoNode.mPhysicalAddr = physical;
    surf->mVideoNode.mLogicalAddr = (gctPOINTER) logical;

    surf->mBytesPerPixel = bytesPerPixel;
    surf->mTiling = gcvLINEAR;
    surf->mAlignedWidth = alignedWidth;
    surf->mAlignedHeight = alignedHeight;
    surf->mStride = pPixmap->devKind;
    surf->mRotation = gcvSURF_0_DEGREE;
    surf->mLogicalAddr = surf->mVideoNode.mLogicalAddr;
    surf->mIsWrapped = gcvTRUE;

#ifdef ENABLE_VIVANTE_DRI3
    surf->bo = gcvNULL;
    surf->fd = -1;
#endif

    pPix->mVidMemInfo = surf;
    TRACE_EXIT(TRUE);
}

Bool DestroySurface(GALINFOPTR galInfo, Viv2DPixmapPtr ppix) {
    TRACE_ENTER();
    VIVGPUPtr gpuctx = (VIVGPUPtr) galInfo->mGpu;
    if (ppix->mVidMemInfo == NULL) {
        TRACE_INFO("NOT GPU GENERATED SURFACE\n");
        TRACE_EXIT(TRUE);
    }
    if (!FreeGPUSurface(gpuctx, ppix)) {
        TRACE_ERROR("Unable to free gpu surface\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

/*Mapping Functions*/
void * MapSurface(Viv2DPixmapPtr priv) {
    TRACE_ENTER();
    void * returnaddr = NULL;
    GenericSurfacePtr surf;
    surf = (GenericSurfacePtr) priv->mVidMemInfo;

    if ( surf == NULL )
    TRACE_EXIT(0);

    returnaddr = surf->mLogicalAddr;
    TRACE_EXIT(returnaddr);
}

void UnMapSurface(Viv2DPixmapPtr priv) {
    TRACE_ENTER();
    TRACE_EXIT();
}

char *MapViv2DPixmap(Viv2DPixmapPtr pdst ){

    GenericSurfacePtr surf = (GenericSurfacePtr) pdst->mVidMemInfo;

    return (surf ? surf->mVideoNode.mLogicalAddr:NULL);
}

unsigned int GetStride(Viv2DPixmapPtr pixmap) {
    TRACE_ENTER();
    GenericSurfacePtr surf = (GenericSurfacePtr) pixmap->mVidMemInfo;
    TRACE_EXIT(surf->mStride);
}

/************************************************************************
 * PIXMAP RELATED (END)
 ************************************************************************/

Bool MapUserMemToGPU(GALINFOPTR galInfo, MemMapInfoPtr mmInfo) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    gctPOINTER logical = (gctPOINTER) mmInfo->mUserAddr;
    gctSIZE_T size = (gctSIZE_T) (mmInfo->mSize);
    gctUINT32 physical = 0;
    gctUINT32 handle = 0;
    gcsUSER_MEMORY_DESC desc = {
        .flag     = gcvALLOC_FLAG_USERMEMORY,
        .logical  = gcmPTR_TO_UINT64(logical),
        .physical = gcvINVALID_ADDRESS,
        .size     = size,
    };

    status = gcoHAL_WrapUserMemory(&desc, &handle);

    if (status < 0) {
        TRACE_ERROR("Wrap Failed\n");
        mmInfo->physical = 0;
        TRACE_EXIT(FALSE);
    }

    status = LockVideoNode(gcvNULL, handle, gcvFALSE, &physical, &logical);

    if (status < 0) {
        TRACE_ERROR("Lock Failed\n");
        gcoHAL_ReleaseVideoMemory(handle);
        mmInfo->physical = 0;
        TRACE_EXIT(FALSE);
    }

    mmInfo->physical = physical;
    mmInfo->handle = handle;
    TRACE_EXIT(TRUE);
}

void UnmapUserMem(GALINFOPTR galInfo, MemMapInfoPtr mmInfo) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 handle = mmInfo->handle;

    status = UnlockVideoNode(gcvNULL, handle, gcvSURF_BITMAP);
    if (status < 0) {
        TRACE_ERROR("Unlock Failed\n");
    }

    status = FreeVideoNode(gcvNULL, handle);
    if (status < 0) {
        TRACE_ERROR("Free Failed\n");
    }

    mmInfo->physical = 0;
    TRACE_EXIT();
}

#define  MAX_WIDTH      1024
#define  MAX_HEIGHT     1024

typedef struct _IVSURF {
gcoSURF surf;
long        lineaddr;
}IVSURF,*PIVSURF;


static IVSURF _vsurf16_1 = {NULL,0};
static IVSURF _vsurf32_1 = {NULL,0};

static IVSURF _vsurf16_2 = {NULL,0};
static IVSURF _vsurf32_2 = {NULL,0};

static IVSURF _vsurf16_3 = {NULL,0};
static IVSURF _vsurf32_3 = {NULL,0};

static PIVSURF _vsurf16=&_vsurf16_1;
static PIVSURF _vsurf32=&_vsurf32_1;

static gctUINT32 _surfIndex = 0;

void VSetSurfIndex(int n)
{
    switch (n) {
        case 1:
            _vsurf16 = & _vsurf16_1;
            _vsurf32 = & _vsurf32_1;
            _surfIndex = 0;
            break;
        case 2:
            _vsurf16 = & _vsurf16_2;
            _vsurf32 = & _vsurf32_2;
            _surfIndex = 1;
            break;
        case 3:
            _vsurf16 = & _vsurf16_3;
            _vsurf32 = & _vsurf32_3;
            _surfIndex = 2;
            break;
        default:
            _vsurf16 = & _vsurf16_1;
            _vsurf32 = & _vsurf32_1;
            _surfIndex = 0;
            break;
    }

}

static Bool VDestroySurf16() {
    gceSTATUS status = gcvSTATUS_OK;

    if (_vsurf16->surf==NULL) TRACE_EXIT(TRUE);

    status=gcoSURF_Unlock(_vsurf16->surf, &(_vsurf16->lineaddr));

    if (status!=gcvSTATUS_OK)
        TRACE_EXIT(FALSE);

    status=gcoSURF_Destroy(_vsurf16->surf);

    _vsurf16->surf=NULL;

    TRACE_EXIT(TRUE);
}

static Bool VDestroySurf32() {

    gceSTATUS status = gcvSTATUS_OK;

    if (_vsurf32->surf==NULL) TRACE_EXIT(TRUE);

    status=gcoSURF_Unlock(_vsurf32->surf, &(_vsurf32->lineaddr));

    if (status!=gcvSTATUS_OK)
        TRACE_EXIT(FALSE);

    status=gcoSURF_Destroy(_vsurf32->surf);

    _vsurf32->surf=NULL;

    TRACE_EXIT(TRUE);

}

Bool  VGetSurfAddrBy16(GALINFOPTR galInfo,int maxsize,int *phyaddr,int *lgaddr,int *width,int *height,int *stride)
{
    static unsigned int gphyaddr[4];
    static unsigned int glgaddr[4];
    static unsigned int gwidth[4];
    static unsigned int gheight[4];
    static int gstride[4];
    static int lastmaxsize[4]={ 0 };

    gceSTATUS status = gcvSTATUS_OK;

    VIVGPUPtr gpuctx = (VIVGPUPtr) (galInfo->mGpu);

    if (maxsize < MAX_WIDTH)
        maxsize = MAX_WIDTH;

    if (_vsurf16->surf && (maxsize >lastmaxsize[_surfIndex])) {
        if (VDestroySurf16()!=TRUE)
            TRACE_EXIT(FALSE);
        lastmaxsize[_surfIndex]=maxsize;
    }

    if (_vsurf16->surf==NULL) {

        lastmaxsize[_surfIndex]=maxsize;
        status=gcoSURF_Construct(gpuctx->mDriver->mHal,maxsize,maxsize,1,gcvSURF_BITMAP,gcvSURF_R5G6B5,gcvPOOL_DEFAULT,&(_vsurf16->surf));

        if (status!=gcvSTATUS_OK)
            TRACE_EXIT(FALSE);

        status=gcoSURF_GetAlignedSize(_vsurf16->surf,&gwidth[_surfIndex],&gheight[_surfIndex],&gstride[_surfIndex]);

        if (status!=gcvSTATUS_OK)
            TRACE_EXIT(FALSE);

        status=gcoSURF_Lock(_vsurf16->surf,  &gphyaddr[_surfIndex], (void *)&glgaddr[_surfIndex]);

        _vsurf16->lineaddr=glgaddr[_surfIndex];

    }

    *phyaddr=gphyaddr[_surfIndex];
    *lgaddr=glgaddr[_surfIndex];
    *width=gwidth[_surfIndex];
    *height=gheight[_surfIndex];
    *stride=gstride[_surfIndex];

    TRACE_EXIT(TRUE);

}


Bool  VGetSurfAddrBy32(GALINFOPTR galInfo,int maxsize, int *phyaddr,int *lgaddr,int *width,int *height,int *stride)
{
    static unsigned int gphyaddr[4];
    static unsigned int glgaddr[4];
    static unsigned int gwidth[4];
    static unsigned int gheight[4];

    static int gstride[4];
    static int lastmaxsize[4]={0};
    gceSTATUS status = gcvSTATUS_OK;

    VIVGPUPtr gpuctx = (VIVGPUPtr) (galInfo->mGpu);

    if (maxsize <MAX_WIDTH)
        maxsize=MAX_WIDTH;

    if (_vsurf32->surf && (maxsize >lastmaxsize[_surfIndex])) {
        if (VDestroySurf32()!=TRUE)
            TRACE_EXIT(FALSE);
        lastmaxsize[_surfIndex]=maxsize;
    }


    if (_vsurf32->surf==NULL) {

    lastmaxsize[_surfIndex]=maxsize;
    status=gcoSURF_Construct(gpuctx->mDriver->mHal,maxsize,maxsize,1,gcvSURF_BITMAP,gcvSURF_A8R8G8B8,gcvPOOL_DEFAULT,&(_vsurf32->surf));

    if (status!=gcvSTATUS_OK)
        TRACE_EXIT(FALSE);

    status=gcoSURF_GetAlignedSize(_vsurf32->surf,&gwidth[_surfIndex],&gheight[_surfIndex],&gstride[_surfIndex]);

    if (status!=gcvSTATUS_OK)
        TRACE_EXIT(FALSE);

    status=gcoSURF_Lock(_vsurf32->surf,  &gphyaddr[_surfIndex], (void *)&glgaddr[_surfIndex]);

    _vsurf32->lineaddr=glgaddr[_surfIndex];

    }

    *phyaddr=gphyaddr[_surfIndex];
    *lgaddr=glgaddr[_surfIndex];
    *width=gwidth[_surfIndex];
    *height=gheight[_surfIndex];
    *stride=gstride[_surfIndex];

    TRACE_EXIT(TRUE);
}

void  VDestroySurf()
{
    VDestroySurf16();
    VDestroySurf32();
}

