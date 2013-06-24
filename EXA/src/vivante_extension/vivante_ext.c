/****************************************************************************
*
*    Copyright (C) 2005 - 2013 by Vivante Corp.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the license, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****************************************************************************/


/*
 * Authors:
 * vivantecorp.com
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xorg-server.h"

#include <errno.h>
#include <string.h>
#include "xf86.h"
#include <X11/X.h>
#include <X11/Xproto.h>
#include "scrnintstr.h"
#include "windowstr.h"
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "extinit.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "swaprep.h"
#include "drm.h"
#include "xf86Module.h"
#include "globals.h"
#include "pixmapstr.h"
/*
#include "xf86Extensions.h"
*/
#include "vivante_ext.h"
#include "vivante_exa.h"
#include "vivante.h"
#include "vivante_priv.h"


static unsigned char VIVHelpReqCode = 0;
static int VIVHelpErrorBase;

static int ProcXF86DRAWABLEFLUSH(register ClientPtr client)
{
	DrawablePtr 	pDrawable;
	WindowPtr	pWin;
	ScreenPtr 	pScreen;
	PixmapPtr      pWinPixmap;
	Viv2DPixmapPtr ppriv = NULL;
	int rc;
	GenericSurfacePtr surf;

	REQUEST(xXF86VIVHelpDRAWABLEFLUSHReq);
	REQUEST_SIZE_MATCH(xXF86VIVHelpDRAWABLEFLUSHReq);
	if (stuff->screen >= screenInfo.numScreens) {
		client->errorValue = stuff->screen;
		return BadValue;
	}

	rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
			DixReadAccess);

	if (rc != Success)
		return rc;

	if ( pDrawable->type == DRAWABLE_WINDOW)
	{
		pWin = (WindowPtr)pDrawable;

		pScreen = screenInfo.screens[stuff->screen];

		pWinPixmap = pScreen->GetWindowPixmap(pWin);

		ppriv = (Viv2DPixmapPtr)exaGetPixmapDriverPrivate(pWinPixmap);

		if (ppriv) {
			surf = (GenericSurfacePtr)ppriv->mVidMemInfo;
			gcoOS_CacheFlush(gcvNULL, surf->mVideoNode.mNode, surf->mVideoNode.mLogicalAddr, surf->mStride * surf->mAlignedHeight);
		}
	}

	if (pDrawable->type == DRAWABLE_PIXMAP)
	{

		ppriv = (Viv2DPixmapPtr)exaGetPixmapDriverPrivate(pDrawable);
		if ( ppriv )
		{
			surf = (GenericSurfacePtr)ppriv->mVidMemInfo;
			gcoOS_CacheFlush(gcvNULL, surf->mVideoNode.mNode, surf->mVideoNode.mLogicalAddr, surf->mStride * surf->mAlignedHeight);
		}
	}

	return  Success;
}

static Bool
VIVHelpDrawableInfo(ScreenPtr pScreen,
	DrawablePtr pDrawable,
	int *X,
	int *Y,
	int *W,
	int *H,
	int *numClipRects,
	drm_clip_rect_t ** pClipRects,
	int *relX,
	int *relY,
	unsigned int *backNode,
	unsigned int *phyAddress,
	unsigned int *alignedWidth,
	unsigned int *alignedHeight,
	unsigned int *stride)
{

	WindowPtr		pWin;
	PixmapPtr      pWinPixmap;
	Viv2DPixmapPtr ppriv;
	GenericSurfacePtr surf = NULL;
	int			i;
	unsigned int * pPointer;

	if (pDrawable->type == DRAWABLE_WINDOW) {

		pWin = (WindowPtr)pDrawable;
		*X = (int)(pWin->drawable.x);
		*Y = (int)(pWin->drawable.y);
		*W = (int)(pWin->drawable.width);
		*H = (int)(pWin->drawable.height);
		*numClipRects = RegionNumRects(&pWin->clipList);
		*pClipRects = (drm_clip_rect_t *)RegionRects(&pWin->clipList);

		pWinPixmap = pScreen->GetWindowPixmap(pWin);
		*alignedWidth = gcmALIGN(pWinPixmap->drawable.width, WIDTH_ALIGNMENT);
		*alignedHeight = gcmALIGN(pWinPixmap->drawable.height, HEIGHT_ALIGNMENT);

		#ifdef COMPOSITE
		*relX = *X - pWinPixmap->screen_x;
		*relY = *Y - pWinPixmap->screen_y;
		#else
		*relX = pWin->origin.x;
		*relY = pWin->origin.y;
		#endif

		ppriv = (Viv2DPixmapPtr)exaGetPixmapDriverPrivate(pWinPixmap);
		*backNode = 0;
		*phyAddress = 0;
		*stride = 0;
		if (ppriv) {
			surf = (GenericSurfacePtr) (ppriv->mVidMemInfo);
			if (surf) {
				*backNode = (unsigned int)surf->mVideoNode.mNode;
				*phyAddress = (unsigned int)surf->mVideoNode.mPhysicalAddr;
				*stride = surf->mStride;
			}
		}

	} else {

		*relX = 0;
		*relY = 0;
		/* pixmap (or for GLX 1.3, a PBuffer) */
		pWinPixmap = (PixmapPtr)pDrawable;
		if (pDrawable->type == DRAWABLE_PIXMAP) {
			Viv2DPixmapPtr ppriv = (Viv2DPixmapPtr)exaGetPixmapDriverPrivate(pDrawable);
			GenericSurfacePtr surf = (GenericSurfacePtr) (ppriv->mVidMemInfo);
			*alignedWidth = gcmALIGN(pWinPixmap->drawable.width, WIDTH_ALIGNMENT);
			*alignedHeight = gcmALIGN(pWinPixmap->drawable.height, HEIGHT_ALIGNMENT);
			if (surf) {
				*backNode = (unsigned int)surf->mVideoNode.mNode;
				*phyAddress = (unsigned int)surf->mVideoNode.mPhysicalAddr;
				*stride = surf->mStride;
			} else {
				*backNode = 0;
				*phyAddress = 0;
				*stride = 0;
			}
			*numClipRects = 0;
			*pClipRects = 0;
			return TRUE;
		} else {
			*alignedWidth = 0;
			*alignedHeight = 0;
			*backNode = 0;
			*phyAddress = 0;
			*stride = 0;
			return FALSE;
		}

	}

	return TRUE;


}

static int
ProcXF86VIVHelpDRAWABLEINFO(register ClientPtr client)
{
	WindowPtr		pWin;
	PixmapPtr		pWinPixmap;
	xXF86VIVHelpDRAWABLEINFOReply rep = {
		.type = X_Reply,
		.sequenceNumber = client->sequence,
		.length = 0
	};
	DrawablePtr pDrawable;
	int X, Y, W, H;
	drm_clip_rect_t *pClipRects, *pClippedRects;
	int relX, relY, rc;


	REQUEST(xXF86VIVHelpDRAWABLEINFOReq);
	REQUEST_SIZE_MATCH(xXF86VIVHelpDRAWABLEINFOReq);


	if (stuff->screen >= screenInfo.numScreens) {
		client->errorValue = stuff->screen;
		return BadValue;
	}

	rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
		DixReadAccess);
	if (rc != Success)
	return rc;

	if (!VIVHelpDrawableInfo(screenInfo.screens[stuff->screen],
		pDrawable,
		(int *) &X,
		(int *) &Y,
		(int *) &W,
		(int *) &H,
		(int *) &rep.numClipRects,
		&pClipRects,
		&relX,
		&relY,
		(unsigned int *)&rep.backNode,
		(unsigned int *)&rep.phyAddress,
		(unsigned int *)&rep.alignedWidth,
		(unsigned int *)&rep.alignedHeight,
		(unsigned int *)&rep.stride)) {
		return BadValue;
	}

	ScreenPtr pScreen = screenInfo.screens[stuff->screen];
	if (pDrawable->type == DRAWABLE_WINDOW) {
		pWin = (WindowPtr)pDrawable;
		pWinPixmap = pScreen->GetWindowPixmap(pWin);
	}

	if (pDrawable->type == DRAWABLE_PIXMAP) {
		pWinPixmap = (PixmapPtr)pDrawable;
	}

	rep.drawableX = X;
	rep.drawableY = Y;
	rep.drawableWidth = W;
	rep.drawableHeight = H;
	rep.length = (SIZEOF(xXF86VIVHelpDRAWABLEINFOReply) - SIZEOF(xGenericReply));

	rep.relX = relX;
	rep.relY = relY;

	pClippedRects = pClipRects;

	if (rep.numClipRects) {
		/* Clip cliprects to screen dimensions (redirected windows) */
		pClippedRects = malloc(rep.numClipRects * sizeof(drm_clip_rect_t));

		if (pClippedRects) {

			int i, j;

			for (i = 0, j = 0; i < rep.numClipRects; i++) {

				#ifdef COMPOSITE
				pClippedRects[j].x1 = pClipRects[i].x1 - pWinPixmap->screen_x;
				pClippedRects[j].y1 = pClipRects[i].y1 - pWinPixmap->screen_y;
				pClippedRects[j].x2 = pClipRects[i].x2 - pWinPixmap->screen_x;
				pClippedRects[j].y2 = pClipRects[i].y2 - pWinPixmap->screen_y;
				#else
				pClippedRects[j].x1 = pClipRects[i].x1;
				pClippedRects[j].y1 = pClipRects[i].y1;
				pClippedRects[j].x2 = pClipRects[i].x2;
				pClippedRects[j].y2 = pClipRects[i].y2;
				#endif

				if (pClippedRects[j].x1 < pClippedRects[j].x2 &&
				pClippedRects[j].y1 < pClippedRects[j].y2) {
					j++;
				}
			}

			rep.numClipRects = j;
		} else {
			rep.numClipRects = 0;
		}

		rep.length += sizeof(drm_clip_rect_t) * rep.numClipRects;
	}

	rep.length = bytes_to_int32(rep.length);

	WriteToClient(client, sizeof(xXF86VIVHelpDRAWABLEINFOReply), &rep);

	if (rep.numClipRects) {
	WriteToClient(client,
	sizeof(drm_clip_rect_t) * rep.numClipRects,
	pClippedRects);
	free(pClippedRects);
	}

	return Success;

}

static int
ProcXF86VIVHelpPIXMAPPHYSADDR(register ClientPtr client)
{

	int n;

	REQUEST(xXF86VIVHelpPIXMAPPHYSADDRReq);
	REQUEST_SIZE_MATCH(xXF86VIVHelpPIXMAPPHYSADDRReq);

	/* Initialize reply */
	xXF86VIVHelpPIXMAPPHYSADDRReply rep;
	rep.type = X_Reply;
	rep.sequenceNumber = client->sequence;
	rep.length = 0;
	rep.pixmapState = VIV_PixmapUndefined;
	rep.pixmapPhysAddr = (CARD32)NULL;
	rep.pixmapStride = 0;

	/* Find the pixmap */
	PixmapPtr pPixmap;
	int rc = dixLookupResourceByType((pointer*)&pPixmap, stuff->pixmap, RT_PIXMAP, client,
					DixGetAttrAccess);
	if (Success == rc)
	{

		Viv2DPixmapPtr ppriv = (Viv2DPixmapPtr)exaGetPixmapDriverPrivate(pPixmap);
		GenericSurfacePtr surf = (GenericSurfacePtr) (ppriv->mVidMemInfo);
		if (surf) {

			rep.pixmapState = VIV_PixmapFramebuffer;
			rep.pixmapPhysAddr = (CARD32) surf->mVideoNode.mPhysicalAddr;
			rep.pixmapStride = surf->mStride;
		} else {
			rep.pixmapState = VIV_PixmapOther;
		}
	}

	/* Check if any reply values need byte swapping */
	if (client->swapped)
	{
#if defined(SWAP_SINGLE_PARAMETER)
		swaps(&rep.sequenceNumber);
		swapl(&rep.length);
		swapl(&rep.pixmapPhysAddr);
		swapl(&rep.pixmapStride);
#else
		swaps(&rep.sequenceNumber, n);
		swapl(&rep.length, n);
		swapl(&rep.pixmapPhysAddr, n);
		swapl(&rep.pixmapStride, n);
#endif
	}

	/* Reply to client */
	WriteToClient(client, sizeof(rep), (char*)&rep);
	return client->noClientException;

}


static int
ProcXF86VIVHelpDispatch(register ClientPtr client)
{
	REQUEST(xReq);

	switch (stuff->data) {
		case X_XF86VIVHelpDRAWABLEFLUSH:
			return ProcXF86DRAWABLEFLUSH(client);
		case X_XF86VIVHelpDRAWABLEINFO:
			return ProcXF86VIVHelpDRAWABLEINFO(client);
		case X_XF86VIVHelpPIXMAPPHYSADDR:
			return ProcXF86VIVHelpPIXMAPPHYSADDR(client);
		default:
			return BadRequest;
	}

}

static int
ProcXF86VIVHelpQueryVersion(
	register ClientPtr client
)
{
	xXF86VIVHelpQueryVersionReply rep;
	register int n;

	REQUEST_SIZE_MATCH(xXF86VIVHelpQueryVersionReq);
	rep.type = X_Reply;
	rep.length = 0;
	rep.sequenceNumber = client->sequence;
	rep.majorVersion = XF86VIVHelp_MAJOR_VERSION;
	rep.minorVersion = XF86VIVHelp_MINOR_VERSION;
	rep.patchVersion = XF86VIVHelp_PATCH_VERSION;

	if (client->swapped) {
#if defined(SWAP_SINGLE_PARAMETER)
		swaps(&rep.sequenceNumber);
		swapl(&rep.length);
		swaps(&rep.majorVersion);
		swaps(&rep.minorVersion);
		swapl(&rep.patchVersion);
#else
		swaps(&rep.sequenceNumber, n);
		swapl(&rep.length, n);
		swaps(&rep.majorVersion, n);
		swaps(&rep.minorVersion, n);
		swapl(&rep.patchVersion, n);
#endif
	}

	WriteToClient(client, sizeof(xXF86VIVHelpQueryVersionReply), (char *)&rep);

	return Success;
}

static int
SProcXF86VIVHelpQueryVersion(
	register ClientPtr	client
)
{
	register int n;
	REQUEST(xXF86VIVHelpQueryVersionReq);
#if defined(SWAP_SINGLE_PARAMETER)
	swaps(&stuff->length);
#else
	swaps(&stuff->length, n);
#endif
	return ProcXF86VIVHelpQueryVersion(client);
}

static int
SProcXF86VIVHelpDispatch (
	register ClientPtr	client
)
{
	REQUEST(xReq);
	/*
	* Only local clients are allowed vivhelp access, but remote clients still need
	* these requests to find out cleanly.
	*/
	switch (stuff->data)
	{
		case X_XF86VIVHelpQueryVersion:
			return SProcXF86VIVHelpQueryVersion(client);
		default:
			return VIVHelpErrorBase + XF86VIVHelpClientNotLocal;
	}
}

/*ARGSUSED*/
static void
XF86VIVHelpResetProc (
	ExtensionEntry* extEntry
)
{

}

void
XFree86VIVHELPExtensionInit(void)
{
	ExtensionEntry *extEntry;

	extEntry = AddExtension(XF86VIVHELPNAME,
				XF86VIVHelpNumberEvents,
				XF86VIVHelpNumberErrors,
				ProcXF86VIVHelpDispatch,
				SProcXF86VIVHelpDispatch,
				XF86VIVHelpResetProc, StandardMinorOpcode);

	VIVHelpReqCode = (unsigned char) extEntry->base;
	VIVHelpErrorBase = extEntry->errorBase;

}



