/****************************************************************************
*
*    Copyright 2012 - 2018 Vivante Corporation, Santa Clara, California.
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


#ifndef _VIV_EXT
#define _VIV_EXT

#define X_VIVEXTQueryVersion            0
#define X_VIVEXTPixmapPhysaddr          1
#define X_VIVEXTDrawableFlush           2
#define X_VIVEXTDrawableInfo            3
#define X_VIVEXTFULLScreenInfo          4


#define VIVEXTNumberEvents      0

#define VIVEXTClientNotLocal        0
#define VIVEXTOperationNotSupported 1
#define VIVEXTNumberErrors      (VIVEXTOperationNotSupported + 1)



#define VIVEXTNAME "vivext"

/*
#define XORG_VERSION_CURRENT 10.4
*/

#define VIVEXT_MAJOR_VERSION    1
#define VIVEXT_MINOR_VERSION    0
#define VIVEXT_PATCH_VERSION    0

typedef struct _VIVEXTQueryVersion {
    CARD8   reqType;
    CARD8   vivEXTReqType;
    CARD16  length B16;
} xVIVEXTQueryVersionReq;
#define sz_xVIVEXTQueryVersionReq   4


typedef struct {
    BYTE    type;/* X_Reply */
    BYTE    pad1;
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    CARD16  majorVersion B16;       /* major version of vivEXT protocol */
    CARD16  minorVersion B16;       /* minor version of vivEXT protocol */
    CARD32  patchVersion B32;       /* patch version of vivEXT protocol */
    CARD32  pad3 B32;
    CARD32  pad4 B32;
    CARD32  pad5 B32;
    CARD32  pad6 B32;
} xVIVEXTQueryVersionReply;
#define sz_xVIVEXTQueryVersionReply 32


typedef struct _VIVEXTDrawableFlush {
    CARD8   reqType;        /* always vivEXTReqCode */
    CARD8   vivEXTReqType;      /* always X_vivEXTDrawableFlush */
    CARD16  length B16;
    CARD32  screen B32;
    CARD32  drawable B32;
} xVIVEXTDrawableFlushReq;
#define sz_xVIVEXTDrawableFlushReq  12



typedef struct _VIVEXTDrawableInfo {
    CARD8   reqType;
    CARD8   vivEXTReqType;
    CARD16  length B16;
    CARD32  screen B32;
    CARD32  drawable B32;
} xVIVEXTDrawableInfoReq;
#define sz_xVIVEXTDrawableInfoReq   12

typedef struct {
    BYTE    type;           /* X_Reply */
    BYTE    pad1;
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    INT16   drawableX B16;
    INT16   drawableY B16;
    INT16   drawableWidth B16;
    INT16   drawableHeight B16;
    CARD32  numClipRects B32;
    INT16       relX B16;
    INT16       relY B16;
    CARD32      alignedWidth B32;
    CARD32      alignedHeight B32;
    CARD32      stride B32;
    CARD32      nodeName B32;
    CARD32      phyAddress B32;
} xVIVEXTDrawableInfoReply;

#define sz_xVIVEXTDrawableInfoReply 44


typedef struct _VIVEXTFULLScreenInfo {
    CARD8   reqType;
    CARD8   vivEXTReqType;
    CARD16  length B16;
    CARD32  screen B32;
    CARD32  drawable B32;
} xVIVEXTFULLScreenInfoReq;
#define sz_xVIVEXTFULLScreenInfoReq 12

typedef struct {
    BYTE    type;           /* X_Reply */
    BYTE    pad1;
    CARD16  sequenceNumber B16;
    CARD32  length B32;
    CARD32  fullscreenCovered B32;  /* if fullscreen is covered by windows, set to 1 otherwise 0 */
    CARD32  pad3 B32;
    CARD32  pad4 B32;
    CARD32  pad5 B32;
    CARD32  pad6 B32;
    CARD32  pad7 B32;       /* bytes 29-32 */
} xVIVEXTFULLScreenInfoReply;
#define sz_xVIVEXTFULLScreenInfoReply 32


/************************************************************************/

typedef struct {
    CARD8   reqType;    /* always XTestReqCode */
    CARD8   xtReqType;  /* always X_IMX_EXT_GetPixmapPhysaddr */
    CARD16  length B16;
    Pixmap  pixmap B32;
} xVIVEXTPixmapPhysaddrReq;
#define sz_xVIVEXTPixmapPhysaddrReq 8

typedef enum
{
    VIV_PixmapUndefined,    /* pixmap is not defined */
    VIV_PixmapFramebuffer,  /* pixmap is in framebuffer */
    VIV_PixmapOther     /* pixmap is not in framebuffer */
} VIVEXT_PixmapState;

typedef struct {
    CARD8   type;           /* must be X_Reply */
    CARD8   pixmapState;        /* has value of IMX_EXT_PixmapState */
    CARD16  sequenceNumber B16; /* of last request received by server */
    CARD32  length B32;     /* 4 byte quantities beyond size of GenericReply */
    CARD32  PixmapPhysaddr B32; /* pixmap phys addr; otherwise NULL */
    CARD32  pixmapStride B32;   /* bytes between lines in pixmap */
    CARD32  pad0 B32;       /* bytes 17-20 */
    CARD32  pad1 B32;       /* bytes 21-24 */
    CARD32  pad2 B32;       /* bytes 25-28 */
    CARD32  pad3 B32;       /* bytes 29-32 */
} xVIVEXTPixmapPhysaddrReply;
#define sz_xVIVEXTPixmapPhysaddrReply 32


void VIVExtensionInit(void);


#endif
