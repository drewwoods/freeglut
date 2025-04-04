#ifndef __FG_INTERNAL_DRM_H__
#define __FG_INTERNAL_DRM_H__

/*
 * freeglut_internal_drm.h
 *
 * The DRM/KMS/GBM-specific Windows and OpenGL-related stuff.
 *
 * Copyright (c) 2024 FreeGLUT contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PAWEL W. OLSZTA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <EGL/egl.h>

/* DRM/GBM/EGL-specific structures */
typedef struct fg_drm_plane {
    drmModePlane            *plane;
    drmModeObjectProperties *props;
    drmModePropertyRes     **props_info;
} FGDRMPlane;

typedef struct fg_drm_crtc {
    drmModeCrtc             *crtc;
    drmModeObjectProperties *props;
    drmModePropertyRes     **props_info;
} FGDRMCrtc;

typedef struct fg_drm_connector {
    drmModeConnector        *connector;
    drmModeObjectProperties *props;
    drmModePropertyRes     **props_info;
} FGDRMConnector;

typedef struct fg_drm_fb {
    struct gbm_bo *bo;
    uint32_t       fb_id;
} FGDRMFb;

typedef struct fg_drm_device {
    int                fd;  /* DRM device file descriptor */
    struct gbm_device *gbm; /* GBM device */

    /* CRTC and connector info */
    FGDRMPlane     *plane;
    FGDRMCrtc      *crtc;
    FGDRMConnector *connector;
    int             crtc_index;
    int             kms_in_fence_fd;
    int             kms_out_fence_fd;

    drmModeModeInfo *mode;
    uint32_t         crtc_id;
    uint32_t         connector_id;

    /* EGL Display */
    EGLDisplay egl_display;
    EGLConfig  egl_config;

    /* Current context window */
    SFG_Window *current_window;
} FGDRMDevice;

typedef struct fg_drm_window {
    struct gbm_surface *gbm_surface;
    EGLSurface          egl_surface;
    FGDRMFb            *fb;
    struct gbm_bo      *current_bo;
    uint32_t            current_fb_id;
} FGDRMWindow;

/* DRM State Management */
extern FGDRMDevice fgDrmDevice;

/* Function prototypes */
extern int      fgDrmInit( void );
extern void     fgDrmClose( void );
extern FGDRMFb *fgDrmFbGetFromBo( struct gbm_bo *bo );
extern int      fgDrmCreateWindow( SFG_Window *window );
extern void     fgDrmDestroyWindow( SFG_Window *window );
extern void     fgDrmDisplayWindow( SFG_Window *window );
extern void     fgDrmMainLoop( void );
extern void     fgDrmSetCursor( SFG_Window *window, int cursorID );

#endif /* __FG_INTERNAL_DRM_H__ */
