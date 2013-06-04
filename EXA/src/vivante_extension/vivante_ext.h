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


#ifndef _VIV_HELP
#define _VIV_HELP

#define X_XF86VIVHelpQueryVersion			0
#define X_XF86VIVHelpPIXMAPPHYSADDR		1
#define X_XF86VIVHelpDRAWABLEFLUSH		2
#define X_XF86VIVHelpDRAWABLEINFO			3


#define XF86VIVHelpNumberEvents		0

#define XF86VIVHelpClientNotLocal		0
#define XF86VIVHelpOperationNotSupported	1
#define XF86VIVHelpNumberErrors		(XF86VIVHelpOperationNotSupported + 1)



#define XF86VIVHELPNAME "vivext"

/*
#define XORG_VERSION_CURRENT 10.4
*/

#define XF86VIVHelp_MAJOR_VERSION	1
#define XF86VIVHelp_MINOR_VERSION	0
#define XF86VIVHelp_PATCH_VERSION	0

typedef struct _XF86VIVHelpQueryVersion {
	CARD8	reqType;
	CARD8	vivHelpReqType;
	CARD16	length B16;
} xXF86VIVHelpQueryVersionReq;
#define sz_xXF86VIVHelpQueryVersionReq	4


typedef struct {
	BYTE	type;/* X_Reply */
	BYTE	pad1;
	CARD16	sequenceNumber B16;
	CARD32	length B32;
	CARD16	majorVersion B16;		/* major version of vivHelp protocol */
	CARD16	minorVersion B16;		/* minor version of vivHelp protocol */
	CARD32	patchVersion B32;		/* patch version of vivHelp protocol */
	CARD32	pad3 B32;
	CARD32	pad4 B32;
	CARD32	pad5 B32;
	CARD32	pad6 B32;
} xXF86VIVHelpQueryVersionReply;
#define sz_xXF86VIVHelpQueryVersionReply	32


typedef struct _XF86VIVHelpDRAWABLEFLUSH {
	CARD8	reqType;		/* always vivHelpReqCode */
	CARD8	vivHelpReqType;		/* always X_vivHelpDRAWABLEFLUSH */
	CARD16	length B16;
	CARD32	screen B32;
	CARD32	drawable B32;
} xXF86VIVHelpDRAWABLEFLUSHReq;
#define sz_xXF86VIVHelpDRAWABLEFLUSHReq	12



typedef struct _XF86VIVHelpDRAWABLEINFO {
	CARD8	reqType;
	CARD8	vivHelpReqType;
	CARD16	length B16;
	CARD32	screen B32;
	CARD32	drawable B32;
} xXF86VIVHelpDRAWABLEINFOReq;
#define sz_xXF86VIVHelpDRAWABLEINFOReq	12

typedef struct {
	BYTE	type;			/* X_Reply */
	BYTE	pad1;
	CARD16	sequenceNumber B16;
	CARD32	length B32;
	INT16	drawableX B16;
	INT16	drawableY B16;
	INT16	drawableWidth B16;
	INT16	drawableHeight B16;
	CARD32	numClipRects B32;
	INT16       relX B16;
	INT16       relY B16;
	CARD32      alignedWidth B32;
	CARD32      alignedHeight B32;
	CARD32      stride B32;
	CARD32      backNode B32;
	CARD32      phyAddress B32;
} xXF86VIVHelpDRAWABLEINFOReply;

#define sz_xXF86VIVHelpDRAWABLEINFOReply	44


/************************************************************************/

typedef struct {
	CARD8	reqType;	/* always XTestReqCode */
	CARD8	xtReqType;	/* always X_IMX_EXT_GetPixmapPhysAddr */
	CARD16	length B16;
	Pixmap	pixmap B32;
} xXF86VIVHelpPIXMAPPHYSADDRReq;
#define sz_xXF86VIVHelpPIXMAPPHYSADDRReq 8

typedef enum
{
	VIV_PixmapUndefined,	/* pixmap is not defined */
	VIV_PixmapFramebuffer,	/* pixmap is in framebuffer */
	VIV_PixmapOther		/* pixmap is not in framebuffer */
} XF86VIVHelp_PixmapState;

typedef struct {
	CARD8	type;			/* must be X_Reply */
	CARD8	pixmapState;		/* has value of IMX_EXT_PixmapState */
	CARD16	sequenceNumber B16;	/* of last request received by server */
	CARD32	length B32;		/* 4 byte quantities beyond size of GenericReply */
	CARD32	pixmapPhysAddr B32;	/* pixmap phys addr; otherwise NULL */
	CARD32	pixmapStride B32;	/* bytes between lines in pixmap */
	CARD32	pad0 B32;		/* bytes 17-20 */
	CARD32	pad1 B32;		/* bytes 21-24 */
	CARD32	pad2 B32;		/* bytes 25-28 */
	CARD32	pad3 B32;		/* bytes 29-32 */
} xXF86VIVHelpPIXMAPPHYSADDRReply;
#define	sz_xXF86VIVHelpPIXMAPPHYSADDRReply 32


void XFree86VIVHELPExtensionInit(void);


#endif


