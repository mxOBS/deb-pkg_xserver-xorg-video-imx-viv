#ifndef IMX_G2D_H
#define IMX_G2D_H

#ifdef __cplusplus
extern "C" {
#endif

    gctBOOL SetupG2D(GALINFOPTR galInfo);

    /************************************************************************
     * G2D Surface Operations (START)
     ************************************************************************/
    Bool G2DCreateSurface(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr toBeUpdatedpPix, int fd);
    Bool G2DReUseSurface(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr toBeUpdatedpPix);
    Bool G2DCleanSurfaceBySW(GALINFOPTR galInfo, PixmapPtr pPixmap, Viv2DPixmapPtr pPix);
    Bool G2DDestroySurface(GALINFOPTR galInfo, Viv2DPixmapPtr ppriv);
	Bool G2DWrapSurface(PixmapPtr pPixmap, void * logical, unsigned int physical, Viv2DPixmapPtr pPix, int bytes);

    /*Mapping Functions*/
    void * G2DMapSurface(Viv2DPixmapPtr priv);
    void G2DUnMapSurface(Viv2DPixmapPtr priv);
    /************************************************************************
     * G2D Surface Operations (END)
     ************************************************************************/

    Bool G2DGPUFlushGraphicsPipe(GALINFOPTR galInfo);
    Bool G2DCacheOperation(GALINFOPTR galInfo, Viv2DPixmapPtr ppix, VIVFLUSHTYPE flush_type);

    char *MapG2DPixmap(Viv2DPixmapPtr pdst);
    unsigned int G2DGetStride(Viv2DPixmapPtr pixmap);

#ifdef __cplusplus
}
#endif

#endif  /* IMX_G2D_H */