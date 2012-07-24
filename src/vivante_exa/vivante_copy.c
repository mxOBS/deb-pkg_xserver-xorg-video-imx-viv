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
 * PrepareCopy() sets up the driver for doing a copy within video
 * memory.
 *
 * @param pSrcPixmap source pixmap
 * @param pDstPixmap destination pixmap
 * @param dx X copy direction
 * @param dy Y copy direction
 * @param alu raster operation
 * @param planemask write mask for the fill
 *
 * This call should set up the driver for doing a series of copies from the
 * the pSrcPixmap to the pDstPixmap.  The dx flag will be positive if the
 * hardware should do the copy from the left to the right, and dy will be
 * positive if the copy should be done from the top to the bottom.  This
 * is to deal with self-overlapping copies when pSrcPixmap == pDstPixmap.
 * If your hardware can only support blits that are (left to right, top to
 * bottom) or (right to left, bottom to top), then you should set
 * #EXA_TWO_BITBLT_DIRECTIONS, and EXA will break down Copy operations to
 * ones that meet those requirements.  The alu raster op is one of the GX*
 * graphics functions listed in X.h, and typically maps to a similar
 * single-byte "ROP" setting in all hardware.  The planemask controls which
 * bits of the destination should be affected, and will only represent the
 * bits up to the depth of pPixmap.
 *
 * Note that many drivers will need to store some of the data in the driver
 * private record, for sending to the hardware with each drawing command.
 *
 * The PrepareCopy() call is required of all drivers, but it may fail for any
 * reason.  Failure results in a fallback to software rendering.
 */

Bool
VivPrepareCopy(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap,
	int xdir, int ydir, int alu, Pixel planemask) {
	TRACE_ENTER();
	Viv2DPixmapPtr psrc = exaGetPixmapDriverPrivate(pSrcPixmap);
	Viv2DPixmapPtr pdst = exaGetPixmapDriverPrivate(pDstPixmap);
	VivPtr pViv = VIVPTR_FROM_PIXMAP(pDstPixmap);
	int fgop = 0xCC;
	int bgop = 0xCC;

#if VIV_EXA_COPY_SIZE_CHECK_ENABLE
	if (pSrcPixmap->drawable.width * pSrcPixmap->drawable.height <= IMX_EXA_MIN_PIXEL_AREA_COPY
	|| pDstPixmap->drawable.width * pDstPixmap->drawable.height <= IMX_EXA_MIN_PIXEL_AREA_COPY) {
		TRACE_EXIT(FALSE);
	}
#endif

	if (!CheckBltvalidity(pDstPixmap, alu, planemask)) {
		TRACE_EXIT(FALSE);
	}

	if (!GetDefaultFormat(pSrcPixmap->drawable.bitsPerPixel, &(pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mFormat))) {
		TRACE_EXIT(FALSE);
	}

	if (!GetDefaultFormat(pDstPixmap->drawable.bitsPerPixel, &(pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mFormat))) {
		TRACE_EXIT(FALSE);
	}

	ConvertXAluToOPS(pDstPixmap, alu, planemask, &fgop,&bgop);

	pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mHeight = pDstPixmap->drawable.height;
	pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mWidth = pDstPixmap->drawable.width;
	pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride = pDstPixmap->devKind;
	pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mPriv = pdst;

	pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mHeight = pSrcPixmap->drawable.height;
	pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mWidth = pSrcPixmap->drawable.width;
	pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mStride = pSrcPixmap->devKind;
	pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mPriv = psrc;


	pViv->mGrCtx.mBlitInfo.mBgRop = fgop;
	pViv->mGrCtx.mBlitInfo.mFgRop = bgop;

	if ( alu == GXcopy )
		pViv->mGrCtx.mBlitInfo.mOperationCode = VIVSIMCOPY;
	else
		pViv->mGrCtx.mBlitInfo.mOperationCode = VIVCOPY;

	if (0) {
		if (!SetDestinationSurface(&pViv->mGrCtx)) {
			TRACE_EXIT(FALSE);
		}
		if (!SetSourceSurface(&pViv->mGrCtx)) {
			TRACE_EXIT(FALSE);
		}
		if (!SetClipping(&pViv->mGrCtx)) {
			TRACE_EXIT(FALSE);
		}
	}

	TRACE_EXIT(TRUE);
}

/**
 * Copy() performs a copy set up in the last PrepareCopy call.
 *
 * @param pDstPixmap destination pixmap
 * @param srcX source X coordinate
 * @param srcY source Y coordinate
 * @param dstX destination X coordinate
 * @param dstY destination Y coordinate
 * @param width width of the rectangle to be copied
 * @param height height of the rectangle to be copied.
 *
 * Performs the copy set up by the last PrepareCopy() call, copying the
 * rectangle from (srcX, srcY) to (srcX + width, srcY + width) in the source
 * pixmap to the same-sized rectangle at (dstX, dstY) in the destination
 * pixmap.  Those rectangles may overlap in memory, if
 * pSrcPixmap == pDstPixmap.  Note that this call does not receive the
 * pSrcPixmap as an argument -- if it's needed in this function, it should
 * be stored in the driver private during PrepareCopy().  As with Solid(),
 * the coordinates are in the coordinate space of each pixmap, so the driver
 * will need to set up source and destination pitches and offsets from those
 * pixmaps, probably using exaGetPixmapOffset() and exaGetPixmapPitch().
 *
 * This call is required if PrepareCopy ever succeeds.
 *
**/

#define MAX_SUB_COPY_SIZE    (700*1024)


 /* ported from pixman, we can perhaps use pixman function directly*/
 /* vectors are stored in 64-bit floating-point registers */
 typedef double __m64;
 #define force_inline __inline__ __attribute__ ((__always_inline__))

 /* Elemental unaligned loads */
 static force_inline __m64 ldq_u(__m64 *p)
{
	struct __una_u64 { __m64 x __attribute__((packed)); };
	const struct __una_u64 *ptr = (const struct __una_u64 *) p;
	return (__m64) ptr->x;
}

static force_inline uint32_t ldl_u(const uint32_t *p)
{
	struct __una_u32 { uint32_t x __attribute__((packed)); };
	const struct __una_u32 *ptr = (const struct __una_u32 *) p;
	return ptr->x;
}

 static Bool
 Blt_64Bits(uint32_t *src_bits,
			uint32_t *dst_bits,
			int       src_stride,
			int       dst_stride,
			int       src_bpp,
			int       dst_bpp,
			int       src_x,
			int       src_y,
			int       dest_x,
			int       dest_y,
			int       width,
			int       height)
{

	uint8_t *   src_bytes;
	uint8_t *   dst_bytes;
	int byte_width;

	if (src_bpp != dst_bpp)
		return FALSE;

	if (src_bpp == 16)
	{
		src_stride = src_stride * (int) sizeof (uint32_t) / 2;
		dst_stride = dst_stride * (int) sizeof (uint32_t) / 2;
		src_bytes = (uint8_t *)(((uint16_t *)src_bits) + src_stride * (src_y) + (src_x));
		dst_bytes = (uint8_t *)(((uint16_t *)dst_bits) + dst_stride * (dest_y) + (dest_x));
		byte_width = 2 * width;
		src_stride *= 2;
		dst_stride *= 2;
	}
	else if (src_bpp == 32)
	{
		src_stride = src_stride * (int) sizeof (uint32_t) / 4;
		dst_stride = dst_stride * (int) sizeof (uint32_t) / 4;
		src_bytes = (uint8_t *)(((uint32_t *)src_bits) + src_stride * (src_y) + (src_x));
		dst_bytes = (uint8_t *)(((uint32_t *)dst_bits) + dst_stride * (dest_y) + (dest_x));
		byte_width = 4 * width;
		src_stride *= 4;
		dst_stride *= 4;
	}
	else
	{
		return FALSE;
	}

	while (height--)
	{
		int w;
		uint8_t *s = src_bytes;
		uint8_t *d = dst_bytes;
		src_bytes += src_stride;
		dst_bytes += dst_stride;
		w = byte_width;

		if (w >= 1 && ((unsigned long)d & 1))
		{
			*(uint8_t *)d = *(uint8_t *)s;
			w -= 1;
			s += 1;
			d += 1;
		}

		if (w >= 2 && ((unsigned long)d & 3))
		{
			*(uint16_t *)d = *(uint16_t *)s;
			w -= 2;
			s += 2;
			d += 2;
		}

		while (w >= 4 && ((unsigned long)d & 7))
		{
			*(uint32_t *)d = ldl_u ((uint32_t *)s);

			w -= 4;
			s += 4;
			d += 4;
		}

		while (w >= 64)
		{

			__m64 v0 = ldq_u ((__m64 *)(s + 0));
			__m64 v1 = ldq_u ((__m64 *)(s + 8));
			__m64 v2 = ldq_u ((__m64 *)(s + 16));
			__m64 v3 = ldq_u ((__m64 *)(s + 24));
			__m64 v4 = ldq_u ((__m64 *)(s + 32));
			__m64 v5 = ldq_u ((__m64 *)(s + 40));
			__m64 v6 = ldq_u ((__m64 *)(s + 48));
			__m64 v7 = ldq_u ((__m64 *)(s + 56));
			*(__m64 *)(d + 0)  = v0;
			*(__m64 *)(d + 8)  = v1;
			*(__m64 *)(d + 16) = v2;
			*(__m64 *)(d + 24) = v3;
			*(__m64 *)(d + 32) = v4;
			*(__m64 *)(d + 40) = v5;
			*(__m64 *)(d + 48) = v6;
			*(__m64 *)(d + 56) = v7;


			w -= 64;
			s += 64;
			d += 64;
		}
		while (w >= 4)
		{
			*(uint32_t *)d = ldl_u ((uint32_t *)s);

			w -= 4;
			s += 4;
			d += 4;
		}
		if (w >= 2)
		{
			*(uint16_t *)d = *(uint16_t *)s;
			w -= 2;
			s += 2;
			d += 2;
		}
	}

	return TRUE;
}
 static void VivSWCopy(VivPtr pViv) {
    VIV2DBLITINFOPTR pBlt = &(pViv->mGrCtx.mBlitInfo);
	char				*lgsrcaddr=NULL;
	char				*lgdstaddr=NULL;
	int				srcX,srcY,dstX,dstY;
	int				width;
	int				height;
	int				dirx=1;
	int				diry=1;
	int				i,j,k;
	int				temx;
	int				temy;
	int				*lgisrcaddr=NULL;
	int				*lgidstaddr=NULL;
	int				ioff;
	int				inum;

	int				srcstride;
	int				dststride;
	int				bytesperpixel;


	lgsrcaddr = MapViv2DPixmap(pBlt->mSrcSurfInfo.mPriv);
	lgdstaddr = MapViv2DPixmap(pBlt->mDstSurfInfo.mPriv);

	dstX = pViv->mGrCtx.mBlitInfo.mDstBox.x1;
	dstY = pViv->mGrCtx.mBlitInfo.mDstBox.y1;

	srcX = pViv->mGrCtx.mBlitInfo.mSrcBox.x1;
	srcY = pViv->mGrCtx.mBlitInfo.mSrcBox.y1;

	temx = V_MIN(pViv->mGrCtx.mBlitInfo.mDstBox.x2, pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mWidth);
	width = temx - pViv->mGrCtx.mBlitInfo.mDstBox.x1;
	temx = V_MIN(pViv->mGrCtx.mBlitInfo.mSrcBox.x2, pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mWidth);
	width = V_MIN(width,temx - pViv->mGrCtx.mBlitInfo.mSrcBox.x1);

	temy = V_MIN(pViv->mGrCtx.mBlitInfo.mDstBox.y2, pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mHeight);
	height = temy - pViv->mGrCtx.mBlitInfo.mDstBox.y1;
	temy = V_MIN(pViv->mGrCtx.mBlitInfo.mSrcBox.y2, pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mHeight);
	height = V_MIN(height,temy - pViv->mGrCtx.mBlitInfo.mSrcBox.y1);



	bytesperpixel = pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mFormat.mBpp / 8;

	if (srcY<dstY || ((srcY==dstY)&&(srcX<dstX)) )
	{
		srcX=pViv->mGrCtx.mBlitInfo.mSrcBox.x1+width-1;
		srcY=pViv->mGrCtx.mBlitInfo.mSrcBox.y1+height-1;
		dstX=pViv->mGrCtx.mBlitInfo.mDstBox.x1+width-1;
		dstY=pViv->mGrCtx.mBlitInfo.mDstBox.y1+height-1;
		dirx=-1;
		diry=-1;
	}

	lgsrcaddr+=(srcY*pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mStride+srcX*bytesperpixel);
	/* move to the last byte */
	if (dirx<0)
		lgsrcaddr+=(bytesperpixel-1);

	lgdstaddr+=(dstY*pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride+dstX*bytesperpixel);
	/* move to the last byte */
	if (diry<0)
		lgdstaddr+=(bytesperpixel-1);

	dststride = diry*pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mStride;
	srcstride = dirx*pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mStride;
	k = (width)*bytesperpixel;


	if (dirx > 0) {
		for(i=0;i<height;i++) {
			memcpy(lgdstaddr,lgsrcaddr,k);
			lgdstaddr+=dststride;
			lgsrcaddr+=srcstride;
		}
	} else {
		for(i=0;i<height;i++) {
			memcpy(lgdstaddr-k+1,lgsrcaddr-k+1,k);
			lgdstaddr+=dststride;
			lgsrcaddr+=srcstride;
		}
	}

  }

void
VivCopy(PixmapPtr pDstPixmap, int srcX, int srcY,
	int dstX, int dstY, int width, int height) {
	TRACE_ENTER();
	VivPtr pViv = VIVPTR_FROM_PIXMAP(pDstPixmap);

	Viv2DPixmapPtr psrc = NULL;
	Viv2DPixmapPtr pdst = NULL;

	pdst = pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mPriv;
	psrc = pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mPriv;
	/*Setting up the rectangle*/
	pViv->mGrCtx.mBlitInfo.mDstBox.x1 = dstX;
	pViv->mGrCtx.mBlitInfo.mDstBox.y1 = dstY;
	pViv->mGrCtx.mBlitInfo.mDstBox.x2 = dstX + width;
	pViv->mGrCtx.mBlitInfo.mDstBox.y2 = dstY + height;

	pViv->mGrCtx.mBlitInfo.mSrcBox.x1 = srcX;
	pViv->mGrCtx.mBlitInfo.mSrcBox.y1 = srcY;
	pViv->mGrCtx.mBlitInfo.mSrcBox.x2 = srcX + width;
	pViv->mGrCtx.mBlitInfo.mSrcBox.y2 = srcY + height;
	pViv->mGrCtx.mBlitInfo.mSwcpy=FALSE;


	if ( 1 ) {
		if ( ( (width*height) < MAX_SUB_COPY_SIZE )
			&& pViv->mGrCtx.mBlitInfo.mOperationCode == VIVSIMCOPY
			&& ( pViv->mGrCtx.mBlitInfo.mSrcSurfInfo.mFormat.mBpp == pViv->mGrCtx.mBlitInfo.mDstSurfInfo.mFormat.mBpp ) ){
				pViv->mGrCtx.mBlitInfo.mSwcpy = TRUE;
				VivSWCopy(pViv);

				pdst->mCpuBusy = TRUE;
				psrc->mCpuBusy = TRUE;

				TRACE_EXIT();
		}
	}

	if (psrc->mCpuBusy) {
		VIV2DCacheOperation(&pViv->mGrCtx, psrc, FLUSH);
		psrc->mCpuBusy = FALSE;
	}

	if (pdst->mCpuBusy) {
		VIV2DCacheOperation(&pViv->mGrCtx,pdst, FLUSH);
		pdst->mCpuBusy = FALSE;
	}


	if (!SetDestinationSurface(&pViv->mGrCtx)) {
		TRACE_ERROR("Copy Blit Failed\n");
	}

	if (!SetSourceSurface(&pViv->mGrCtx)) {
		TRACE_ERROR("Copy Blit Failed\n");
	}

	if (!SetClipping(&pViv->mGrCtx)) {
		TRACE_ERROR("Copy Blit Failed\n");
	}

	if (!DoCopyBlit(&pViv->mGrCtx)) {
		TRACE_ERROR("Copy Blit Failed\n");
	}
	TRACE_EXIT();
}

/**
 * DoneCopy() finishes a set of copies.
 *
 * @param pPixmap destination pixmap.
 *
 * The DoneCopy() call is called at the end of a series of consecutive
 * Copy() calls following a successful PrepareCopy().  This allows drivers
 * to finish up emitting drawing commands that were buffered, or clean up
 * state from PrepareCopy().
 *
 * This call is required if PrepareCopy() ever succeeds.
 */
void
VivDoneCopy(PixmapPtr pDstPixmap) {

	TRACE_ENTER();
	VivPtr pViv = VIVPTR_FROM_PIXMAP(pDstPixmap);
	if (pViv && pViv->mGrCtx.mBlitInfo.mSwcpy) {
		pViv->mGrCtx.mBlitInfo.mSwcpy=FALSE;
		TRACE_EXIT();
	}

	VIV2DGPUFlushGraphicsPipe(&pViv->mGrCtx);
#if VIV_EXA_FLUSH_2D_CMD_ENABLE
	VIV2DGPUBlitComplete(&pViv->mGrCtx,TRUE);
#endif
	TRACE_EXIT();
}
