#if !defined(__FSL_PIXMAP_EXT_H__)
#define __FSL_PIXMAP_EXT_H__

#if defined(__cpluspplus)
extern "C" {
#endif

#include <X11/Xlib.h>

void * FslLockPixmap(Display *dpy, Pixmap pixmap, int *stride);
void FslUnlockPixmap(Display *dpy, Pixmap pixmap);
void FslSyncPixmap(Display *dpy, Pixmap pixmap);

#if defined(__cpluspplus)
}
#endif

#endif /* __FSL_PIXMAP_EXT_H__ */

