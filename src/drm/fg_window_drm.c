/*
 * fg_window_drm.c
 *
 * DRM/KMS/GBM-specific window handling functions
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

#include <GL/freeglut.h>
#include "fg_internal.h"
#include "fg_internal_drm.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <EGL/egl.h>

/*
 * Create a window using DRM/KMS/GBM
 */
int fgDrmCreateWindow( SFG_Window *window )
{
    FGDRMWindow *drm_window;
    EGLContext   context;

    /* Allocate DRM window data structure */
    drm_window = calloc( 1, sizeof( FGDRMWindow ) );
    if ( !drm_window ) {
        fgError( "Failed to allocate DRM window data" );
        return 0;
    }

    /* Create GBM surface */
    drm_window->gbm_surface = gbm_surface_create( fgDrmDevice.gbm,
        window->State.Width,
        window->State.Height,
        GBM_FORMAT_XRGB8888,
        GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING );

    if ( !drm_window->gbm_surface ) {
        fgError( "Failed to create GBM surface" );
        free( drm_window );
        return 0;
    }

    /* Create EGL surface */
    drm_window->egl_surface = eglCreateWindowSurface(
        fgDrmDevice.egl_display, fgDrmDevice.egl_config, (EGLNativeWindowType)drm_window->gbm_surface, NULL );

    if ( drm_window->egl_surface == EGL_NO_SURFACE ) {
        fgError( "Failed to create EGL surface" );
        gbm_surface_destroy( drm_window->gbm_surface );
        free( drm_window );
        return 0;
    }

    /* Create EGL context */
    EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION,
        2, /* Request OpenGL ES 2.0 or OpenGL 2.0+ */
        EGL_NONE };

    context = eglCreateContext( fgDrmDevice.egl_display, fgDrmDevice.egl_config, EGL_NO_CONTEXT, context_attribs );

    if ( context == EGL_NO_CONTEXT ) {
        fgError( "Failed to create EGL context" );
        eglDestroySurface( fgDrmDevice.egl_display, drm_window->egl_surface );
        gbm_surface_destroy( drm_window->gbm_surface );
        free( drm_window );
        return 0;
    }

    /* Make context current */
    if ( !eglMakeCurrent( fgDrmDevice.egl_display, drm_window->egl_surface, drm_window->egl_surface, context ) ) {
        fgError( "Failed to make EGL context current" );
        eglDestroyContext( fgDrmDevice.egl_display, context );
        eglDestroySurface( fgDrmDevice.egl_display, drm_window->egl_surface );
        gbm_surface_destroy( drm_window->gbm_surface );
        free( drm_window );
        return 0;
    }

    /* Associate the DRM window with the freeglut window structure */
    window->Window.pContext.eglContext = context;
    window->Window.pContext.eglSurface = drm_window->egl_surface;
    window->Window.Handle              = (SFG_WindowHandleType)drm_window;

    /* Store the current window in the DRM device */
    fgDrmDevice.current_window = window;

    /* Set this window to be the current OpenGL context */
    fgStructure.CurrentWindow = window;

    /* Setup for page flipping */
    drm_window->current_bo    = NULL;
    drm_window->current_fb_id = 0;

    return 1;
}

/*
 * Destroy a DRM/KMS window
 */
void fgDrmDestroyWindow( SFG_Window *window )
{
    FGDRMWindow *drm_window = (FGDRMWindow *)window->Window.Handle;

    if ( drm_window ) {
        /* Destroy EGL context */
        EGLContext context = window->Window.pContext.eglContext;
        if ( context != EGL_NO_CONTEXT ) {
            eglDestroyContext( fgDrmDevice.egl_display, context );
            window->Window.pContext.eglContext = EGL_NO_CONTEXT;
        }

        /* Destroy EGL surface */
        if ( drm_window->egl_surface != EGL_NO_SURFACE ) {
            eglDestroySurface( fgDrmDevice.egl_display, drm_window->egl_surface );
        }

        /* Destroy GBM surface */
        if ( drm_window->gbm_surface ) {
            gbm_surface_destroy( drm_window->gbm_surface );
        }

        /* Free DRM window struct */
        free( drm_window );
        window->Window.Handle = NULL;
    }
}

/*
 * Resize a DRM/KMS window
 */
void fgDrmReshapeWindow( SFG_Window *window, int width, int height )
{
    /* For now, we'll just recreate the window to handle resizing */
    fgDrmDestroyWindow( window );

    window->State.Width  = width;
    window->State.Height = height;

    fgDrmCreateWindow( window );
}

/*
 * Position a DRM/KMS window - in DRM we always use full screen so this is a no-op
 */
void fgDrmPositionWindow( SFG_Window *window, int x, int y )
{
    /* No-op for DRM/KMS - always full screen */
}

/*
 * Display a DRM/KMS window
 */
static void page_flip_handler( int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data )
{
    int *waiting_for_flip = data;
    *waiting_for_flip     = 0;
}

void fgDrmDisplayWindow( SFG_Window *window )
{
    FGDRMWindow    *drm_window = (FGDRMWindow *)window->Window.Handle;
    struct gbm_bo  *bo;
    FGDRMFb        *fb;
    int             waiting_for_flip = 1;
    fd_set          fds;
    drmEventContext ev = {
        .version           = DRM_EVENT_CONTEXT_VERSION,
        .page_flip_handler = page_flip_handler,
    };
    struct timeval timeout = {
        .tv_sec  = 3,
        .tv_usec = 0,
    };
    int ret;

    /* Swap the EGL buffers which renders to the GBM surface */
    eglSwapBuffers( fgDrmDevice.egl_display, drm_window->egl_surface );

    /* Get the new front buffer */
    bo = gbm_surface_lock_front_buffer( drm_window->gbm_surface );
    if ( !bo ) {
        fgWarning( "Failed to lock GBM surface front buffer" );
        return;
    }

    /* Get a framebuffer for this buffer object */
    fb = fgDrmFbGetFromBo( bo );
    if ( !fb ) {
        fgWarning( "Failed to get framebuffer from buffer object" );
        gbm_surface_release_buffer( drm_window->gbm_surface, bo );
        return;
    }

    /* First time: set the CRTC with the new buffer */
    if ( !drm_window->current_fb_id ) {
        ret = drmModeSetCrtc(
            fgDrmDevice.fd, fgDrmDevice.crtc_id, fb->fb_id, 0, 0, &fgDrmDevice.connector_id, 1, fgDrmDevice.mode );
        if ( ret ) {
            fgWarning( "Failed to set CRTC: %m" );
            gbm_surface_release_buffer( drm_window->gbm_surface, bo );
            return;
        }
    }

    /* Request a page flip to the new buffer */
    ret =
        drmModePageFlip( fgDrmDevice.fd, fgDrmDevice.crtc_id, fb->fb_id, DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip );
    if ( ret ) {
        fgWarning( "Failed to queue page flip: %m" );
        gbm_surface_release_buffer( drm_window->gbm_surface, bo );
        return;
    }

    /* Wait for the page flip to complete */
    while ( waiting_for_flip ) {
        FD_ZERO( &fds );
        FD_SET( fgDrmDevice.fd, &fds );
        ret = select( fgDrmDevice.fd + 1, &fds, NULL, NULL, &timeout );

        if ( ret <= 0 ) {
            fgWarning( "Select timeout or error waiting for page flip" );
            break;
        }

        drmHandleEvent( fgDrmDevice.fd, &ev );
    }

    /* Release the previous buffer */
    if ( drm_window->current_bo ) {
        gbm_surface_release_buffer( drm_window->gbm_surface, drm_window->current_bo );
    }

    /* Save the new buffer for next release */
    drm_window->current_bo    = bo;
    drm_window->current_fb_id = fb->fb_id;
}

void fgDrmHideWindow( SFG_Window *window )
{
    /* Not applicable for DRM/KMS - we're always full-screen */
}

void fgDrmSetCursor( SFG_Window *window, int cursorID )
{
    /* DRM/KMS cursor support needs kernel interface */
    /* For now, just do nothing */
}
