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


#include "vivante_priv.h"
#include "vivante_common.h"
#include "vivante_gal.h"
#ifdef HAVE_G2D
#include "g2dExt.h"
#endif

gctBOOL CHIP_SUPPORTA8 = gcvFALSE;
gctBOOL EXA_G2D = gcvFALSE;
/**
 *
 * @param driver - Driver object to be returned
 * @return the status of the initilization
 */
static gctBOOL SetupDriver
(
        OUT Viv2DDriverPtr * driver,
        IN EXAHWTYPE exaHwType
        ) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    Viv2DDriverPtr pDrvHandle = gcvNULL;
    gctPOINTER mHandle = gcvNULL;

#define VDRIVER_MASK 0x1
#define VOS_MASK 0x2
#define VHAL_MASK 0x4
#define VINTERNAL_MASK 0x8
#define VEXTERNAL_MASK 0x10
#define VCONTIGUOUS_MASK 0x20

    gctUINT32 smask = 0x0;

    /*Allocating the driver*/
    gcmASSERT(*driver == gcvNULL);
    status = gcoOS_Allocate(gcvNULL, sizeof (Viv2DDriver), &mHandle);
    if (status < 0) {
        TRACE_ERROR("Unable to allocate driver, status = %d\n", status);
        TRACE_EXIT(gcvFALSE);
    }
    pDrvHandle = (Viv2DDriverPtr) mHandle;
    smask |= VDRIVER_MASK;

    status = gcoOS_Construct(gcvNULL, &(pDrvHandle->mOs));
    if (status < 0) {
        TRACE_ERROR("Unable to construct OS object, status = %d\n", status);
        goto FREESOURCE;
    }
    smask |= VOS_MASK;

    status = gcoHAL_Construct(gcvNULL, pDrvHandle->mOs, &(pDrvHandle->mHal));
    if (status < 0) {
        TRACE_ERROR("Unable to construct HAL object, status = %d\n", status);
        goto FREESOURCE;
    }

    smask |= VHAL_MASK;

#ifdef HAVE_G2D
    if(exaHwType == IMXG2D)
    {
        status = gcoHAL_SetHardwareType(pDrvHandle->mHal, gcvHARDWARE_3D);
        if (status < 0) {
            TRACE_ERROR("Unable to SetHardwareType, status = %d\n", status);
            goto FREESOURCE;
        }
        status = gcoOS_GetBaseAddress(gcvNULL, &pDrvHandle->mG2DBaseAddr);
        if (status < 0) {
            TRACE_ERROR("Unable to GetBaseAddress, status = %d\n", status);
            goto FREESOURCE;
        }
    }
    else
#endif
    {
        /*If Seperated*/
        pDrvHandle->mIsSeperated = gcoHAL_QuerySeparated2D(pDrvHandle->mHal) == gcvSTATUS_TRUE;

        if (pDrvHandle->mIsSeperated) {
            status = gcoHAL_SetHardwareType(pDrvHandle->mHal, gcvHARDWARE_2D);
            if (status < 0) {
                TRACE_ERROR("Unable to gcoHAL_SetHardwareType, status = %d\n", status);
                goto FREESOURCE;
            }
        }

        if (!gcoHAL_IsFeatureAvailable(pDrvHandle->mHal, gcvFEATURE_PIPE_2D)) {
            TRACE_ERROR("2D PIPE IS NOT AVAIBLE");
            goto FREESOURCE;
        }
    }

    /* Query the amount of video memory. */
    status = gcoHAL_QueryVideoMemory
            (pDrvHandle->mHal,
            &pDrvHandle->g_InternalPhysName,
            &pDrvHandle->g_InternalSize,
            &pDrvHandle->g_ExternalPhysName,
            &pDrvHandle->g_ExternalSize,
            &pDrvHandle->g_ContiguousPhysName,
            &pDrvHandle->g_ContiguousSize
            );


    if (status < 0) {
        TRACE_ERROR("gcoHAL_QueryVideoMemory failed, status = %d\n", status);
        goto FREESOURCE;
    }
    /* Map the local internal memory. */
    if (pDrvHandle->g_InternalSize > 0) {
        status = gcoHAL_MapMemory(
                pDrvHandle->mHal,
                pDrvHandle->g_InternalPhysName,
                pDrvHandle->g_InternalSize,
                &pDrvHandle->g_Internal
                );
        if (status < 0) {
            TRACE_ERROR("gcoHAL_MapMemory failed, status = %d\n", status);
            goto FREESOURCE;
        }
    }

    smask |= VINTERNAL_MASK;

    /* Map the local external memory. */
    if (pDrvHandle->g_ExternalSize > 0) {
        status = gcoHAL_MapMemory(
                pDrvHandle->mHal,
                pDrvHandle->g_ExternalPhysName,
                pDrvHandle->g_ExternalSize,
                &pDrvHandle->g_External
                );
        if (status < 0) {
            TRACE_ERROR("gcoHAL_MapMemory failed, status = %d\n", status);
            goto FREESOURCE;
        }
    }

    smask |= VEXTERNAL_MASK;

    /* Map the contiguous memory. */
    if (pDrvHandle->g_ContiguousSize > 0) {
        status = gcoHAL_MapMemory
                (pDrvHandle->mHal,
                pDrvHandle->g_ContiguousPhysName,
                pDrvHandle->g_ContiguousSize,
                &pDrvHandle->g_Contiguous
                );

        TRACE_INFO("Physcal : %x LOGICAL ADDR = %p  SIZE = 0x%lx\n", pDrvHandle->g_ContiguousPhysName, pDrvHandle->g_Contiguous, pDrvHandle->g_ContiguousSize);
        if (status < 0) {
            TRACE_ERROR("gcoHAL_MapMemory failed, status = %d\n", status);
            goto FREESOURCE;
        }
    }

    smask |= VCONTIGUOUS_MASK;


#ifdef HAVE_G2D
    if(exaHwType == IMXG2D)
    {
        status = g2d_open(&(pDrvHandle->mG2DHandle));
        if (status < 0) {
            TRACE_ERROR("g2d_open failed, status = %d\n", status);
            goto FREESOURCE;
        }
    }
    else
#endif
    {
        /* Determine whether PE 2.0 is present. */
        pDrvHandle->mIsPe20Supported = gcoHAL_IsFeatureAvailable(pDrvHandle ->mHal,
                gcvFEATURE_2DPE20)
                == gcvSTATUS_TRUE;


        /*Multi source options*/
        pDrvHandle->mIsMultiSrcBltSupported = gcoHAL_IsFeatureAvailable(pDrvHandle->mHal, gcvFEATURE_2D_MULTI_SOURCE_BLT) == gcvSTATUS_TRUE;
        pDrvHandle->mIsMultiSrcBltExSupported = gcoHAL_IsFeatureAvailable(pDrvHandle->mHal, gcvFEATURE_2D_MULTI_SOURCE_BLT_EX) == gcvSTATUS_TRUE;
        pDrvHandle->mMaxSourceForMultiSrcOpt = pDrvHandle->mIsMultiSrcBltExSupported ? 8 : (pDrvHandle->mIsMultiSrcBltSupported ? 4 : 1);
        CHIP_SUPPORTA8 = gcoHAL_IsFeatureAvailable(pDrvHandle->mHal, gcvFEATURE_2D_A8_TARGET) == gcvSTATUS_TRUE;

        /*Getting the 2d engine*/
        status = gcoHAL_Get2DEngine(pDrvHandle->mHal, &(pDrvHandle->m2DEngine));

        if (status < 0) {
            TRACE_ERROR("Unable to construct 2DEngine object, status = %d\n", status);
            goto FREESOURCE;
        }
    }
#ifdef HAVE_G2D
    if(exaHwType == IMXG2D)
    {
        EXA_G2D = gcvTRUE;
    }
#endif
    *driver = pDrvHandle;
    TRACE_EXIT(gcvTRUE);

FREESOURCE:
    if (smask & VINTERNAL_MASK)
        gcoHAL_UnmapMemory(pDrvHandle->mHal,
                pDrvHandle->g_InternalPhysName,
                pDrvHandle->g_InternalSize,
                pDrvHandle->g_Internal
                );

    if (smask & VEXTERNAL_MASK)
        gcoHAL_UnmapMemory(pDrvHandle->mHal,
                pDrvHandle->g_ExternalPhysName,
                pDrvHandle->g_ExternalSize,
                pDrvHandle->g_External
                );

    if (smask & VCONTIGUOUS_MASK)
        gcoHAL_UnmapMemory(pDrvHandle->mHal,
                pDrvHandle->g_ContiguousPhysName,
                pDrvHandle->g_ContiguousSize,
                pDrvHandle->g_Contiguous
                );

    if (smask & VHAL_MASK)
        gcoHAL_Destroy(pDrvHandle->mHal);

    if (smask & VOS_MASK)
        gcoOS_Destroy(pDrvHandle->mOs);

    if (smask & VDRIVER_MASK)
        gcoOS_Free(gcvNULL, mHandle);

    TRACE_EXIT(gcvFALSE);
}

/**
 *
 * @param driver - driver object to be destroyed
 * @return  - status of the destriuction
 */
static gctBOOL DestroyDriver
(
        IN Viv2DDriverPtr driver
        ) {
    gceSTATUS status = gcvSTATUS_OK;
    gcmASSERT(driver != gcvNULL);
    TRACE_ENTER();
    /*Committing what is left*/
    gcoHAL_Commit(driver->mHal, gcvTRUE);

#ifdef HAVE_G2D
    if(driver->mG2DHandle) {
        status = g2d_close(driver->mG2DHandle);
        if (status < 0) {
            TRACE_ERROR("g2d_close failed, status = %d\n", status);
            TRACE_EXIT(gcvFALSE);
        }
    }
#endif
    /*Unmapping the memory*/
    if (driver->g_Internal != gcvNULL) {
        /* Unmap the local internal memory. */
        status = gcoHAL_UnmapMemory(driver->mHal,
                driver->g_InternalPhysName,
                driver->g_InternalSize,
                driver->g_Internal
                );
        if (status < 0) {
            TRACE_ERROR("gcoHAL_UnMapMemory failed, status = %d\n", status);
            TRACE_EXIT(gcvFALSE);
        }
    }

    if (driver->g_External != gcvNULL) {
        /* Unmap the local external memory. */
        status = gcoHAL_UnmapMemory(driver->mHal,
                driver->g_ExternalPhysName,
                driver->g_ExternalSize,
                driver->g_External
                );
        if (status < 0) {
            TRACE_ERROR("gcoHAL_UnMapMemory failed, status = %d\n", status);
            TRACE_EXIT(gcvFALSE);
        }
    }
    if (driver->g_Contiguous != gcvNULL) {
        /* Unmap the contiguous memory. */
        status = gcoHAL_UnmapMemory(driver->mHal,
                driver->g_ContiguousPhysName,
                driver->g_ContiguousSize,
                driver->g_Contiguous
                );

        if (status < 0) {
            TRACE_ERROR("gcoHAL_UnMapMemory failed, status = %d\n", status);
            TRACE_EXIT(gcvFALSE);
        }
    }


    /* Shutdown */
    if (driver->mHal != gcvNULL) {
        status = gcoHAL_Destroy(driver->mHal);
        if (status != gcvSTATUS_OK) {
            TRACE_ERROR("Unable to destroy HAL object, status = %d\n", status);
            TRACE_EXIT(gcvFALSE);
        }
        driver->mHal = gcvNULL;
    }

    /*Os Destroy*/
    if (driver->mOs != gcvNULL) {
        status = gcoOS_Destroy(driver->mOs);
        if (status != gcvSTATUS_OK) {
            TRACE_ERROR("Unable to destroy Os object, status = %d\n", status);
            TRACE_EXIT(gcvFALSE);
        }
        driver->mOs = gcvNULL;
    }
    status = gcoOS_Free(gcvNULL, driver);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Unable to free driver structure, status = %d\n", status);
        TRACE_EXIT(gcvFALSE);
    }
    driver = gcvNULL;
    TRACE_EXIT(gcvTRUE);
}

/**
 *
 * @param device - to be created
 * @param driver - driver to use the device
 * @return - status of creation
 */
static gctBOOL SetupDevice
(
        OUT Viv2DDevicePtr * device,
        IN Viv2DDriverPtr driver
        ) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    Viv2DDevicePtr pDeviceHandle;
    gctPOINTER mHandle = gcvNULL;
    /*assertions*/
    gcmASSERT(driver != gcvNULL);
    gcmASSERT(*device == gcvNULL);
    /*Allocation*/
    status = gcoOS_Allocate(gcvNULL, sizeof (Viv2DDevice), &mHandle);
    if (status < 0) {
        TRACE_ERROR("Unable to allocate driver, status = %d\n", status);
        TRACE_EXIT(gcvFALSE);
    }
    pDeviceHandle = (Viv2DDevicePtr) mHandle;
    /*Query*/
    status = gcoHAL_QueryChipIdentity(driver->mHal,
            &pDeviceHandle->mChipModel,
            &pDeviceHandle->mChipRevision,
            gcvNULL,
            gcvNULL);

    if (status != gcvSTATUS_OK) {
        gcoOS_Free(gcvNULL, mHandle);
        TRACE_ERROR("Unable to query chip Info, status = %d\n", status);
        TRACE_EXIT(gcvFALSE);
    }
    *device = pDeviceHandle;
    TRACE_EXIT(gcvTRUE);
}

/**
 *
 * @param device - to be destroyed
 * @return
 */
static gctBOOL DestroyDevice(Viv2DDevicePtr device) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    status = gcoOS_Free(gcvNULL, device);

    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Unable to free driver structure, status = %d\n", status);
        TRACE_EXIT(gcvFALSE);
    }
    device = gcvNULL;
    TRACE_EXIT(gcvTRUE);
}

/************************************************************************
 * GPU RELATED (START)
 ************************************************************************/

Bool VIV2DGPUCtxInit(GALINFOPTR galInfo) {
    TRACE_ENTER();
    static gctBOOL inited = gcvFALSE;
    gctBOOL ret = gcvFALSE;
    gctPOINTER mHandle = gcvNULL;
    VIVGPUPtr gpuctx = NULL;
    gceSTATUS status = gcvSTATUS_OK;
    if (inited) {
        TRACE_EXIT(TRUE);
    }
    if (galInfo->mGpu != NULL) {
        TRACE_ERROR("UNDEFINED GPU CTX\n");
        TRACE_EXIT(FALSE);
    }
    status = gcoOS_Allocate(gcvNULL, sizeof(VIVGPU), &mHandle);
    if (status < 0) {
        TRACE_ERROR("Unable to allocate driver, status = %d\n", status);
        TRACE_EXIT(FALSE);
    }
    memset(mHandle, 0, sizeof(VIVGPU));
    gpuctx = (VIVGPUPtr) (mHandle);
    ret = SetupDriver(&gpuctx->mDriver, galInfo->mExaHwType);
    if (ret != gcvTRUE) {
        gcoOS_Free(gcvNULL, mHandle);
        TRACE_ERROR("GPU DRIVER  FAILED\n");
        TRACE_EXIT(FALSE);
    }
    ret = SetupDevice(&(gpuctx->mDevice), gpuctx->mDriver);
    if (ret != gcvTRUE) {
        gcoOS_Free(gcvNULL, mHandle);
        TRACE_ERROR("GPU DEVICE INIT FAILED\n");
        TRACE_EXIT(FALSE);
    }
    inited = gcvTRUE;
    galInfo->mGpu = gpuctx;
    TRACE_EXIT(TRUE);
}

Bool VIV2DGPUCtxDeInit(GALINFOPTR galInfo) {
    TRACE_ENTER();
    gctBOOL ret = gcvTRUE;
    VIVGPUPtr gpuctx = gcvNULL;
    if (galInfo->mGpu == NULL) {
        TRACE_ERROR("GPU CTX IS NULL\n");
        TRACE_EXIT(TRUE);
    }

    VDestroySurf();

    gpuctx = (VIVGPUPtr) (galInfo->mGpu);
    ret = DestroyDevice(gpuctx->mDevice);
    if (ret != gcvTRUE) {
        TRACE_ERROR("ERROR WHILE DESTROYING DEVICE \n");
        TRACE_EXIT(FALSE);
    }
    ret = DestroyDriver(gpuctx->mDriver);
    if (ret != gcvTRUE) {
        TRACE_ERROR("ERROR WHILE DESTROYING DRIVER\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

Bool VIV2DGPUBlitComplete(GALINFOPTR galInfo, Bool wait) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    VIVGPUPtr gpuctx = (galInfo->mGpu);
    gctBOOL stall = wait ? gcvTRUE : gcvFALSE;
    status = gcoHAL_Commit(gpuctx->mDriver->mHal, stall);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("HAL commit Failed\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

Bool VIV2DGPUFlushGraphicsPipe(GALINFOPTR galInfo) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    VIVGPUPtr gpuctx = (galInfo->mGpu);
    status = gco2D_Flush(gpuctx->mDriver->m2DEngine);
    if (status != gcvSTATUS_OK) {
        TRACE_ERROR("Flush Failed\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}

extern Bool vivEnableCacheMemory;
Bool VIV2DCacheOperation(GALINFOPTR galInfo, Viv2DPixmapPtr ppix, VIVFLUSHTYPE flush_type) {
    gceSTATUS status = gcvSTATUS_OK;
    GenericSurfacePtr surf = (GenericSurfacePtr) (ppix->mVidMemInfo);
    VIVGPUPtr gpuctx = (VIVGPUPtr) (galInfo->mGpu);

#if defined(__mips__) || defined(mips)
    TRACE_EXIT(TRUE);
#endif

    if ( surf == NULL )
        TRACE_EXIT(TRUE);

    if ( surf->mIsWrapped )
    {
        TRACE_EXIT(TRUE);
    }

    if ( vivEnableCacheMemory == FALSE )
    {
        TRACE_EXIT(TRUE);
    }

    TRACE_INFO("FLUSH INFO => LOGICAL = %p PHYSICAL = 0x%x STRIDE = 0x%x  ALIGNED HEIGHT = 0x%x\n", surf->mVideoNode.mLogicalAddr, surf->mVideoNode.mPhysicalAddr, surf->mStride, surf->mAlignedHeight);

    switch (flush_type) {
        case INVALIDATE:
            status = gcoOS_CacheInvalidate(gpuctx->mDriver->mOs, surf->mVideoNode.mNode, surf->mVideoNode.mLogicalAddr, surf->mStride * surf->mAlignedHeight);
            if (status != gcvSTATUS_OK) {
                TRACE_ERROR("Cache Invalidation Failed\n");
                TRACE_EXIT(FALSE);
            }
            break;
        case FLUSH:
            status = gcoOS_CacheFlush(gpuctx->mDriver->mOs, surf->mVideoNode.mNode, surf->mVideoNode.mLogicalAddr, surf->mStride * surf->mAlignedHeight);
            if (status != gcvSTATUS_OK) {
                TRACE_ERROR("Cache Invalidation Failed\n");
                TRACE_EXIT(FALSE);
            }
            break;
        case CLEAN:
            status = gcoOS_CacheClean(gpuctx->mDriver->mOs, surf->mVideoNode.mNode, surf->mVideoNode.mLogicalAddr, surf->mStride * surf->mAlignedHeight);
            if (status != gcvSTATUS_OK) {
                TRACE_ERROR("Cache Invalidation Failed\n");
                TRACE_EXIT(FALSE);
            }
            break;
        case MEMORY_BARRIER:
            status = gcoOS_MemoryBarrier(gpuctx->mDriver->mOs, surf->mVideoNode.mLogicalAddr);
            if (status != gcvSTATUS_OK) {
                TRACE_ERROR("Cache Invalidation Failed\n");
                TRACE_EXIT(FALSE);
            }
            break;
        default:
            TRACE_ERROR("UNIDENTIFIED Cache Operation\n");
            TRACE_EXIT(FALSE);
            break;
    }
    TRACE_EXIT(TRUE);
}

Bool VIV2DGPUUserMemMap(char* logical, unsigned int physical, unsigned int size, void ** mappingInfo, unsigned int * gpuAddress) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 handle = 0;

    gcsUSER_MEMORY_DESC desc = {
        .flag     = gcvALLOC_FLAG_USERMEMORY,
        .logical  = gcmPTR_TO_UINT64(logical),
        .physical = (gctUINT32)physical,
        .size     = (gctUINT32)size,
    };

    status = gcoHAL_WrapUserMemory(&desc, gcvVIDMEM_TYPE_BITMAP, &handle);

    if (status < 0) {
        TRACE_ERROR("Wrap Failed\n");
        *gpuAddress = 0;
        TRACE_EXIT(FALSE);
    }

    status = LockVideoNode(gcvNULL, handle, gcvFALSE, &physical, (gctPOINTER *)&logical);

    if (status < 0) {
        TRACE_ERROR("Lock Failed\n");
        gcoHAL_ReleaseVideoMemory(handle);
        *mappingInfo = gcvNULL;
        TRACE_EXIT(FALSE);
    }
    *mappingInfo = gcmINT2PTR(handle);
    *gpuAddress = physical;
    TRACE_EXIT(TRUE);
}

Bool VIV2DGPUUserMemUnMap(char* logical, unsigned int size, void * mappingInfo, unsigned int  gpuAddress) {
    TRACE_ENTER();
    gceSTATUS status = gcvSTATUS_OK;

    status = UnlockVideoNode(gcvNULL, gcmPTR2SIZE(mappingInfo), gcvSURF_BITMAP);
    if (status < 0) {
        TRACE_ERROR("Unlock Failed\n");
    }

    status = FreeVideoNode(gcvNULL, gcmPTR2SIZE(mappingInfo));
    if (status < 0) {
        TRACE_ERROR("Free Failed\n");
    }

    TRACE_EXIT(TRUE);
}

/************************************************************************
 * GPU RELATED (END)
 ************************************************************************/
