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
#include "vivante_gal.h"
#include "vivante.h"

void
VivEXASync(ScreenPtr pScreen, int marker) {
    TRACE_ENTER();
    VivPtr pViv = VIVPTR_FROM_SCREEN(pScreen);

    if (!VIV2DGPUBlitComplete(&pViv->mGrCtx, TRUE)) {
        TRACE_ERROR("ERROR with gpu sync\n");
    }

    TRACE_EXIT();
}

Bool CheckBltvalidity(PixmapPtr pPixmap, int alu, Pixel planemask) {
    TRACE_ENTER();
    if (alu != GXcopy) {
        TRACE_INFO("FALSE: (alu != GXcopy)\n");
        TRACE_EXIT(FALSE);
    }

    if (!EXA_PM_IS_SOLID(&pPixmap->drawable, planemask)) {
        TRACE_INFO("FALSE: (!EXA_PM_IS_SOLID(&pPixmap->drawable, planemask))\n");
        TRACE_EXIT(FALSE);
    }
    TRACE_EXIT(TRUE);
}
