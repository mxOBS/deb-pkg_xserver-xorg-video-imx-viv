#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "vivante_debug.h"

static const char *logFile = "/home/linaro/share/exa_log.txt";

static FILE *fpLog = NULL;

void OpenLog()
{
    if(fpLog == NULL)
        fpLog = fopen(logFile, "w");
}

void CloseLog()
{
    if(fpLog)
        fclose(fpLog);
    fpLog = NULL;
}

void LogString(const char *str)
{
    if(fpLog == NULL)
        return;

    if(str == NULL || str[0] == 0)
        return;

    if(strlen(str) > 1024*10)
        return;

    fwrite(str, 1, strlen(str), fpLog);

    fflush(fpLog);
}

static char tmp[1024*10];
void LogText(const char *fmt, ...)
{
    va_list args;

    if(fpLog == NULL)
        return;

    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end (args);

    LogString(tmp);
}

typedef struct tagPixmapQueue
{
    struct tagPixmapQueue *next;
    Viv2DPixmap * pixmap;
}PixmapQueue;

static PixmapQueue queue_of_pixmaps_held_by_gpu;

void initPixmapQueue()
{
    LOGD("---- initPixmapQueue\n");
    queue_of_pixmaps_held_by_gpu.next = NULL;
    queue_of_pixmaps_held_by_gpu.pixmap = NULL;
}

void freePixmapQueue()
{
    PixmapQueue *pnext = queue_of_pixmaps_held_by_gpu.next;
    while(pnext)
    {
        PixmapQueue *p = pnext->next;
        pnext->pixmap->mGpuBusy = FALSE;
        free(pnext);
        pnext = p;
    }

    queue_of_pixmaps_held_by_gpu.next = NULL;
}

void queuePixmapToGpu(Viv2DPixmapPtr vpixmap)
{
    PixmapQueue *p = NULL;

    if(vpixmap == NULL)
        return;

    p = (PixmapQueue *)malloc(sizeof(PixmapQueue));
    if(p != NULL)
    {
        // set flag
        vpixmap->mGpuBusy = TRUE;

        // store
        p->pixmap = vpixmap;

        // put in the front
        p->next   = queue_of_pixmaps_held_by_gpu.next;
        queue_of_pixmaps_held_by_gpu.next = p;
    }
}

enum PixmapCachePolicy
{
    NONCACHEABLE,
    WRITETHROUGH,
    WRITEALLOC // system default
};

static enum PixmapCachePolicy getPixmapCachePolicy()
{
    return WRITEALLOC;
}

int isGpuSyncMode()
{
    return 0;
}

void preGpuDraw(VivPtr pViv, Viv2DPixmapPtr vpixmap, int bSrc)
{
    enum PixmapCachePolicy ePolicy = getPixmapCachePolicy();

    if(vpixmap == NULL || !vpixmap->mCpuBusy)
        return;

    if(bSrc)
    {
        // this is a source pixmap
        if(ePolicy == WRITETHROUGH)
        {
            // no need to clean cache
        }
        else if(ePolicy == WRITEALLOC)
        {
    		VIV2DCacheOperation(&pViv->mGrCtx, vpixmap, FLUSH);
    		vpixmap->mCpuBusy = FALSE;
        }
        else if(ePolicy == NONCACHEABLE)
        {
	    	vpixmap->mCpuBusy = FALSE;
        }
    }
    else
    {
        // this is a dest pixmap
        if(ePolicy == WRITETHROUGH)
        {
    		VIV2DCacheOperation(&pViv->mGrCtx, vpixmap, INVALIDATE);
    		vpixmap->mCpuBusy = FALSE;
        }
        else if(ePolicy == WRITEALLOC)
        {
    		VIV2DCacheOperation(&pViv->mGrCtx, vpixmap, FLUSH);
    		vpixmap->mCpuBusy = FALSE;
        }
        else if(ePolicy == NONCACHEABLE)
        {
    		vpixmap->mCpuBusy = FALSE;
        }
    }
}

void postGpuDraw(VivPtr pViv)
{
	VIV2DGPUFlushGraphicsPipe(&pViv->mGrCtx); // need flush?

    if(isGpuSyncMode())
    {
        // wait until gpu done
    	VIV2DGPUBlitComplete(&pViv->mGrCtx, TRUE);
        freePixmapQueue();
    }
    else
    {
        // fire but not wait
    	VIV2DGPUBlitComplete(&pViv->mGrCtx, FALSE);
    }
}

void preCpuDraw(VivPtr pViv, Viv2DPixmapPtr vivpixmap)
{
    if(vivpixmap)
    {
        // wait until gpu done
        if(vivpixmap->mGpuBusy)
        {
            FSLASSERT(!isGpuSyncMode());

        	VIV2DGPUBlitComplete(&pViv->mGrCtx, TRUE);
            freePixmapQueue();
        }

        // set flag
        vivpixmap->mCpuBusy = TRUE;
    }
}

void postCpuDraw(VivPtr pViv, Viv2DPixmapPtr vivpixmap)
{
    // nothing to do
}

