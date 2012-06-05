/****************************************************************************
*
*    Copyright (C) 2005 - 2012 by Vivante Corp.
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


#include "vivante_exa.h"
#include "vivante.h"
#include "../vivante_gal/vivante_priv.h"

/**
 * PrepareSolid() sets up the driver for doing a solid fill.
 * @param pPixmap Destination pixmap
 * @param alu raster operation
 * @param planemask write mask for the fill
 * @param fg "foreground" color for the fill
 *
 * This call should set up the driver for doing a series of solid fills
 * through the Solid() call.  The alu raster op is one of the GX*
 * graphics functions listed in X.h, and typically maps to a similar
 * single-byte "ROP" setting in all hardware.  The planemask controls
 * which bits of the destination should be affected, and will only represent
 * the bits up to the depth of pPixmap.  The fg is the pixel value of the
 * foreground color referred to in ROP descriptions.
 *
 * Note that many drivers will need to store some of the data in the driver
 * private record, for sending to the hardware with each drawing command.
 *
 * The PrepareSolid() call is required of all drivers, but it may fail for any
 * reason.  Failure results in a fallback to software rendering.
 */
Bool
VivPrepareSolid(PixmapPtr pPixmap, int alu, Pixel planemask, Pixel fg) {
	TRACE_ENTER();
	Viv2DPixmapPtr pdst = exaGetPixmapDriverPrivate(pPixmap);
	VivPtr pViv = VIVPTR_FROM_PIXMAP(pPixmap);

#if VIV_EXA_SOLID_SIZE_CHECK_ENABLE
	if (pPixmap->drawable.width * pPixmap->drawable.height <= IMX_EXA_MIN_PIXEL_AREA_SOLID) {
		TRACE_EXIT(FALSE);
	}
#endif

	if (!CheckBltvalidity(pPixmap, alu, planemask)) {
		TRACE_EXIT(FALSE);
	}
	if (!GetDefaultFormat(pPixmap->drawable.bitsPerPixel, &(pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mFormat))) {
		TRACE_EXIT(FALSE);
	}
	/*Populating the information*/
	pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mHeight = pPixmap->drawable.height;
	pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mWidth = pPixmap->drawable.width;
	pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride = pPixmap->devKind;
	pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mPriv = pdst;
	pViv->mGrCtx.mBlitInfo.mFgRop = 0xF0;
	pViv->mGrCtx.mBlitInfo.mBgRop = 0xF0;
	pViv->mGrCtx.mBlitInfo.mColorARGB32 = fg;
	pViv->mGrCtx.mBlitInfo.mColorConvert = FALSE;
	pViv->mGrCtx.mBlitInfo.mPlaneMask = planemask;
	pViv->mGrCtx.mBlitInfo.mOperationCode = VIVSOLID;

	if (0) {
		if (!SetDestinationSurface(&pViv->mGrCtx)) {
			TRACE_EXIT(FALSE);
		}

		if (!SetClipping(&pViv->mGrCtx)) {
			TRACE_EXIT(FALSE);
		}

		if (!SetSolidBrush(&pViv->mGrCtx)) {
			TRACE_EXIT(FALSE);
		}
	}

    TRACE_EXIT(TRUE);
}

/**
 * Solid() performs a solid fill set up in the last PrepareSolid() call.
 *
 * @param pPixmap destination pixmap
 * @param x1 left coordinate
 * @param y1 top coordinate
 * @param x2 right coordinate
 * @param y2 bottom coordinate
 *
 * Performs the fill set up by the last PrepareSolid() call, covering the
 * area from (x1,y1) to (x2,y2) in pPixmap.  Note that the coordinates are
 * in the coordinate space of the destination pixmap, so the driver will
 * need to set up the hardware's offset and pitch for the destination
 * coordinates according to the pixmap's offset and pitch within
 * framebuffer.  This likely means using exaGetPixmapOffset() and
 * exaGetPixmapPitch().
 *
 * This call is required if PrepareSolid() ever succeeds.
 */
 static void VivSWSolid(VivPtr pViv,int bytesperpixel){
	int					x1,x2,y1,y2,x,y,k,w,w1;
	char					*lgdstaddr=NULL;
	char					*pcolor,*pcolor1;
	int					color32;
	Pixel					color= pViv->mGrCtx.mBlitInfo.mColorARGB32;
	VIV2DBLITINFOPTR	pBlt = &(pViv->mGrCtx.mBlitInfo);
	GenericSurfacePtr		dstsurf = (GenericSurfacePtr) (pBlt->mDstSurfInfo.mPriv->mVidMemInfo);

	lgdstaddr=(char *)dstsurf->mLogicalAddr;
	pcolor1=(char *)&color;
	pcolor=pcolor1;
	x1=pViv->mGrCtx.mBlitInfo.mDstBox.x1;
	y1=pViv->mGrCtx.mBlitInfo.mDstBox.y1;
	x2=pViv->mGrCtx.mBlitInfo.mDstBox.x2;
	y2=pViv->mGrCtx.mBlitInfo.mDstBox.y2;

	if (x1<0) x1=0;
	if (y1<0) y1=0;
	if (x1>pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mWidth) 	x1=pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mWidth;
	if (y1>pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mHeight) y1=pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mHeight;
	if (x2<0) x2=0;
	if (y2<0) y2=0;
	if (x2>pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mWidth) 	x2=pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mWidth;
	if (y2>pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mHeight) 	y2=pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mHeight;

	lgdstaddr+=(y1*pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride+x1*bytesperpixel);
	w=(x2-x1)*bytesperpixel;
	switch (bytesperpixel) {
	case 1:
		for(y=y1;y<y2;y++){
			for(x=0;x<w;){
					lgdstaddr[x]=pcolor[0];
			}
			lgdstaddr+=pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride;

		}
		break;
	case 2:
		if (w>4) {
			pcolor=(char *)(&color32);
			pcolor[0]=pcolor1[0];
			pcolor[1]=pcolor1[1];
			pcolor[2]=pcolor1[0];
			pcolor[3]=pcolor1[1];
			w1=w/4;
			w1*=4;
			w%=4;
			for(y=y1;y<y2;y++){
				for(x=0;x<w1;){
					/*
					lgdstaddr[x]=pcolor[0];
					x+=1;
					lgdstaddr[x]=pcolor[1];
					x+=1;
					*/
					*((int *)(&(lgdstaddr[x])))=color32;
					x+=4;
				}
				if (w==2){
					lgdstaddr[x]=pcolor1[0];
					lgdstaddr[x+1]=pcolor[1];
				}
				lgdstaddr+=pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride;

			}
		} else {

			for(y=y1;y<y2;y++){
				for(x=0;x<w;){
					lgdstaddr[x]=pcolor[0];
					x+=1;
					lgdstaddr[x]=pcolor[1];
					x+=1;
				}

				lgdstaddr+=pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride;

			}

		}
		break;
	case 3:
		for(y=y1;y<y2;y++){
			for(x=0;x<w;){
					lgdstaddr[x]=pcolor[0];
					x+=1;
					lgdstaddr[x]=pcolor[1];
					x+=1;
					lgdstaddr[x]=pcolor[2];
					x+=1;
			}
			lgdstaddr+=pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride;

		}
		break;
	case 4:
		for(y=y1;y<y2;y++){
			for(x=0;x<w;){
					/*
					lgdstaddr[x]=pcolor[0];
					x+=1;
					lgdstaddr[x]=pcolor[1];
					x+=1;
					lgdstaddr[x]=pcolor[2];
					x+=1;
					lgdstaddr[x]=pcolor[3];
					x+=1;
					*/

					*((int *)(&(lgdstaddr[x])))=*((int *)pcolor);
					x+=4;

			}

			lgdstaddr+=pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride;

		}
		break;
	default: return ;
	}
 }

void
VivSolid(PixmapPtr pPixmap, int x1, int y1, int x2, int y2) {
	TRACE_ENTER();
	VivPtr pViv = VIVPTR_FROM_PIXMAP(pPixmap);
	/*Setting up the rectangle*/
	pViv->mGrCtx.mBlitInfo.mDstBox.x1 = x1;
	pViv->mGrCtx.mBlitInfo.mDstBox.y1 = y1;
	pViv->mGrCtx.mBlitInfo.mDstBox.x2 = x2;
	pViv->mGrCtx.mBlitInfo.mDstBox.y2 = y2;
	pViv->mGrCtx.mBlitInfo.swsolid=FALSE;

	if (1) {
		if ( ((y2-y1)*(x2-x1))<IMX_EXA_MIN_PIXEL_AREA_SOLID ) {
			pViv->mGrCtx.mBlitInfo.swsolid=TRUE;
			VivSWSolid(pViv,pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mFormat.mBpp/8);
			TRACE_EXIT();
		}
	}

	if (!SetDestinationSurface(&pViv->mGrCtx)) {
			TRACE_ERROR("Solid Blit Failed\n");
	}

	if (!SetClipping(&pViv->mGrCtx)) {
			TRACE_ERROR("Solid Blit Failed\n");
	}

	if (!SetSolidBrush(&pViv->mGrCtx)) {
			TRACE_ERROR("Solid Blit Failed\n");
	}

	if (!DoSolidBlit(&pViv->mGrCtx)) {
		TRACE_ERROR("Solid Blit Failed\n");
	}
	TRACE_EXIT();
}

/**
 * DoneSolid() finishes a set of solid fills.
 *
 * @param pPixmap destination pixmap.
 *
 * The DoneSolid() call is called at the end of a series of consecutive
 * Solid() calls following a successful PrepareSolid().  This allows drivers
 * to finish up emitting drawing commands that were buffered, or clean up
 * state from PrepareSolid().
 *
 * This call is required if PrepareSolid() ever succeeds.
 */
void
VivDoneSolid(PixmapPtr pPixmap) {
	TRACE_ENTER();
	VivPtr pViv = VIVPTR_FROM_PIXMAP(pPixmap);
	if (pViv && pViv->mGrCtx.mBlitInfo.swsolid){
		pViv->mGrCtx.mBlitInfo.swsolid=FALSE;
		TRACE_EXIT();
	}

	VIV2DGPUFlushGraphicsPipe(&pViv->mGrCtx);
#if VIV_EXA_FLUSH_2D_CMD_ENABLE
	VIV2DGPUBlitComplete(&pViv->mGrCtx, TRUE);
#endif
	TRACE_EXIT();
}
