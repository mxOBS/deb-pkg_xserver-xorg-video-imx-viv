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


#ifndef _VIVANTE_DRI_H_
#define _VIVANTE_DRI_H_ 1

/*
 * Datatypes
 */
typedef unsigned int    GLenum;
typedef unsigned char   GLboolean;
typedef unsigned int    GLbitfield;
typedef void        GLvoid;
typedef signed char GLbyte;     /* 1-byte signed */
typedef short       GLshort;    /* 2-byte signed */
typedef int     GLint;      /* 4-byte signed */
typedef unsigned char   GLubyte;    /* 1-byte unsigned */
typedef unsigned short  GLushort;   /* 2-byte unsigned */
typedef unsigned int    GLuint;     /* 4-byte unsigned */
typedef int     GLsizei;    /* 4-byte signed */
typedef float       GLfloat;    /* single precision float */
typedef float       GLclampf;   /* single precision float in [0,1] */
typedef double      GLdouble;   /* double precision float */
typedef double      GLclampd;   /* double precision float in [0,1] */
typedef int     Bool;

#define _XF86DRI_SERVER_
#include "drm.h"
#include "xf86drm.h"
#include "dri.h"
#include "sarea.h"

#define VIV_DRI_VERSION_MAJOR       4
#define VIV_DRI_VERSION_MINOR       1
#define VIV_MAX_DRAWABLES           256

typedef struct _ScreenConfigRec {
    int virtualX;
    int virtualY;
} screenConfig;

typedef struct _vvtDeviceInfoRec {
    int bufBpp;
    int zbufBpp;
    int backOffset;
    int depthOffset;
    int sareaPrivOffset;
    screenConfig ScrnConf;
} vvtDeviceInfo;

Bool VivDRIScreenInit(ScreenPtr pScreen);
void VivDRICloseScreen(ScreenPtr pScreen);
Bool VivDRIFinishScreenInit(ScreenPtr pScreen);
void VivUpdateDriScreen(ScrnInfoPtr pScrn);
#endif /* _VIVANTE_DRI_H_ */
