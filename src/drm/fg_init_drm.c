/*
 * fg_init_drm.c
 *
 * DRM/KMS/GBM-specific initialization functions
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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* The DRM device singleton */
FGDRMDevice fgDrmDevice;

#define MAX_DRM_DEVICES 64

/*
 * Find a suitable DRM device
 */
static int fgFindDrmDevice( drmModeRes **resources )
{
    drmDevicePtr devices[MAX_DRM_DEVICES] = { NULL };
    int          num_devices, fd = -1;

    num_devices = drmGetDevices2( 0, devices, MAX_DRM_DEVICES );
    if ( num_devices < 0 ) {
        fgWarning( "drmGetDevices2 failed: %s", strerror( -num_devices ) );
        return -1;
    }

    for ( int i = 0; i < num_devices; i++ ) {
        drmDevicePtr device = devices[i];
        int          ret;

        if ( !( device->available_nodes & ( 1 << DRM_NODE_PRIMARY ) ) )
            continue;
        /* Check if it's a KMS-capable primary device */
        fd = open( device->nodes[DRM_NODE_PRIMARY], O_RDWR );
        if ( fd < 0 )
            continue;
        *resources = drmModeGetResources( fd );
        if ( *resources )
            break;
        close( fd );
        fd = -1;
    }
    drmFreeDevices( devices, num_devices );

    if ( fd < 0 )
        fgWarning( "No DRM/KMS device found" );
    return fd;
}

/*
 * Find a connected DRM connector
 */
static drmModeConnector *fgFindDrmConnector( int fd, drmModeRes *resources, int connector_id )
{
    drmModeConnector *connector = NULL;
    int               i;

    if ( connector_id >= 0 ) {
        if ( connector_id >= resources->count_connectors )
            return NULL;

        connector = drmModeGetConnector( fd, resources->connectors[connector_id] );
        if ( connector && connector->connection == DRM_MODE_CONNECTED )
            return connector;

        drmModeFreeConnector( connector );
        return NULL;
    }

    for ( i = 0; i < resources->count_connectors; i++ ) {
        connector = drmModeGetConnector( fd, resources->connectors[i] );
        if ( connector && connector->connection == DRM_MODE_CONNECTED ) {
            /* Found a connected connector */
            return connector;
        }
        drmModeFreeConnector( connector );
        connector = NULL;
    }

    return connector;
}

/*
 * Find CRTC for a connector
 */
static int32_t fgFindCrtcForEncoder( const drmModeRes *resources, const drmModeEncoder *encoder )
{
    int i;

    for ( i = 0; i < resources->count_crtcs; i++ ) {
        const uint32_t crtc_mask = 1 << i;
        const uint32_t crtc_id   = resources->crtcs[i];
        if ( encoder->possible_crtcs & crtc_mask ) {
            return crtc_id;
        }
    }

    /* No match found */
    return -1;
}

static int32_t fgFindCrtcForConnector(
    const FGDRMDevice *drm, const drmModeRes *resources, const drmModeConnector *connector )
{
    int i;

    for ( i = 0; i < connector->count_encoders; i++ ) {
        const uint32_t  encoder_id = connector->encoders[i];
        drmModeEncoder *encoder    = drmModeGetEncoder( drm->fd, encoder_id );

        if ( encoder ) {
            const int32_t crtc_id = fgFindCrtcForEncoder( resources, encoder );

            drmModeFreeEncoder( encoder );
            if ( crtc_id != 0 ) {
                return crtc_id;
            }
        }
    }

    /* No match found */
    return -1;
}

/*
 * Initialize the DRM device
 */
int fgDrmInit( void )
{
    drmModeRes       *resources;
    drmModeConnector *connector = NULL;
    drmModeEncoder   *encoder   = NULL;
    int               i, ret, area;

    /* Clear the DRM device structure */
    memset( &fgDrmDevice, 0, sizeof( fgDrmDevice ) );
    fgDrmDevice.fd = -1;

    /* Find and open a DRM device */
    fgDrmDevice.fd = fgFindDrmDevice( &resources );
    if ( fgDrmDevice.fd < 0 || !resources ) {
        fgWarning( "Failed to open DRM device" );
        return GLUT_FAILED_INITIALIZATION;
    }

    /* Find a connected connector */
    connector = fgFindDrmConnector( fgDrmDevice.fd, resources, -1 );
    if ( !connector ) {
        fgWarning( "No connected DRM connector found" );
        drmModeFreeResources( resources );
        close( fgDrmDevice.fd );
        fgDrmDevice.fd = -1;
        return GLUT_FAILED_INITIALIZATION;
    }

    /* Find preferred mode or the highest resolution mode */
    for ( i = 0, area = 0; i < connector->count_modes; i++ ) {
        drmModeModeInfo *current_mode = &connector->modes[i];

        if ( current_mode->type & DRM_MODE_TYPE_PREFERRED ) {
            fgDrmDevice.mode = malloc( sizeof( drmModeModeInfo ) );
            memcpy( fgDrmDevice.mode, current_mode, sizeof( drmModeModeInfo ) );
            break;
        }

        int current_area = current_mode->hdisplay * current_mode->vdisplay;
        if ( current_area > area ) {
            if ( fgDrmDevice.mode )
                free( fgDrmDevice.mode );
            fgDrmDevice.mode = malloc( sizeof( drmModeModeInfo ) );
            memcpy( fgDrmDevice.mode, current_mode, sizeof( drmModeModeInfo ) );
            area = current_area;
        }
    }

    if ( !fgDrmDevice.mode ) {
        fgWarning( "Could not find a suitable display mode" );
        drmModeFreeConnector( connector );
        drmModeFreeResources( resources );
        close( fgDrmDevice.fd );
        fgDrmDevice.fd = -1;
        return GLUT_FAILED_INITIALIZATION;
    }

    /* Find encoder */
    for ( i = 0; i < resources->count_encoders; i++ ) {
        encoder = drmModeGetEncoder( fgDrmDevice.fd, resources->encoders[i] );
        if ( encoder->encoder_id == connector->encoder_id )
            break;
        drmModeFreeEncoder( encoder );
        encoder = NULL;
    }

    if ( encoder ) {
        fgDrmDevice.crtc_id = encoder->crtc_id;
    }
    else {
        int32_t crtc_id = fgFindCrtcForConnector( &fgDrmDevice, resources, connector );
        if ( crtc_id == -1 ) {
            fgWarning( "No CRTC found for connector" );
            free( fgDrmDevice.mode );
            drmModeFreeConnector( connector );
            drmModeFreeResources( resources );
            close( fgDrmDevice.fd );
            fgDrmDevice.fd = -1;
            return GLUT_FAILED_INITIALIZATION;
        }

        fgDrmDevice.crtc_id = crtc_id;
    }

    /* Find CRTC index */
    for ( i = 0; i < resources->count_crtcs; i++ ) {
        if ( resources->crtcs[i] == fgDrmDevice.crtc_id ) {
            fgDrmDevice.crtc_index = i;
            break;
        }
    }

    /* Store connector ID */
    fgDrmDevice.connector_id = connector->connector_id;

    /* Clean up DRM resources */
    if ( encoder )
        drmModeFreeEncoder( encoder );
    drmModeFreeConnector( connector );
    drmModeFreeResources( resources );

    /* Initialize GBM */
    fgDrmDevice.gbm = gbm_create_device( fgDrmDevice.fd );
    if ( !fgDrmDevice.gbm ) {
        fgWarning( "Failed to create GBM device" );
        free( fgDrmDevice.mode );
        close( fgDrmDevice.fd );
        fgDrmDevice.fd = -1;
        return GLUT_FAILED_INITIALIZATION;
    }

    /* Initialize EGL */
    fgDrmDevice.egl_display = eglGetDisplay( (EGLNativeDisplayType)fgDrmDevice.gbm );
    if ( fgDrmDevice.egl_display == EGL_NO_DISPLAY ) {
        fgWarning( "Failed to get EGL display" );
        gbm_device_destroy( fgDrmDevice.gbm );
        free( fgDrmDevice.mode );
        close( fgDrmDevice.fd );
        fgDrmDevice.fd = -1;
        return GLUT_FAILED_INITIALIZATION;
    }

    if ( !eglInitialize( fgDrmDevice.egl_display, NULL, NULL ) ) {
        fgWarning( "Failed to initialize EGL" );
        gbm_device_destroy( fgDrmDevice.gbm );
        free( fgDrmDevice.mode );
        close( fgDrmDevice.fd );
        fgDrmDevice.fd = -1;
        return GLUT_FAILED_INITIALIZATION;
    }

    /* Choose an EGL configuration */
    static const EGLint config_attribs[] = { EGL_SURFACE_TYPE,
        EGL_WINDOW_BIT,
        EGL_RED_SIZE,
        8,
        EGL_GREEN_SIZE,
        8,
        EGL_BLUE_SIZE,
        8,
        EGL_ALPHA_SIZE,
        8,
        EGL_DEPTH_SIZE,
        24,
        EGL_STENCIL_SIZE,
        8,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_BIT,
        EGL_NONE };

    EGLint num_configs;
    if ( !eglChooseConfig( fgDrmDevice.egl_display, config_attribs, &fgDrmDevice.egl_config, 1, &num_configs ) ||
         num_configs != 1 ) {
        fgWarning( "Failed to choose EGL configuration" );
        eglTerminate( fgDrmDevice.egl_display );
        gbm_device_destroy( fgDrmDevice.gbm );
        free( fgDrmDevice.mode );
        close( fgDrmDevice.fd );
        fgDrmDevice.fd = -1;
        return GLUT_FAILED_INITIALIZATION;
    }

    /* Initialize EGL API */
    eglBindAPI( EGL_OPENGL_API );

    fgDisplay.pDisplay.drm = &fgDrmDevice;

    fgDisplay.ScreenWidth    = fgDrmDevice.mode->hdisplay;
    fgDisplay.ScreenHeight   = fgDrmDevice.mode->vdisplay;
    fgDisplay.ScreenWidthMM  = 0; /* TODO: get physical size from DRM */
    fgDisplay.ScreenHeightMM = 0;

    return GLUT_SUCCESS;
}

/*
 * Clean up the DRM device
 */
void fgDrmClose( void )
{
    if ( fgDrmDevice.egl_display != EGL_NO_DISPLAY ) {
        eglTerminate( fgDrmDevice.egl_display );
    }

    if ( fgDrmDevice.gbm ) {
        gbm_device_destroy( fgDrmDevice.gbm );
    }

    if ( fgDrmDevice.mode ) {
        free( fgDrmDevice.mode );
    }

    if ( fgDrmDevice.fd >= 0 ) {
        close( fgDrmDevice.fd );
        fgDrmDevice.fd = -1;
    }
}

/*
 * Get a framebuffer associated with a GBM buffer object
 */
static void fgDrmFbDestroyCallback( struct gbm_bo *bo, void *data )
{
    FGDRMFb *fb = data;

    if ( fb->fb_id )
        drmModeRmFB( fgDrmDevice.fd, fb->fb_id );

    free( fb );
}

FGDRMFb *fgDrmFbGetFromBo( struct gbm_bo *bo )
{
    FGDRMFb *fb = gbm_bo_get_user_data( bo );
    uint32_t width, height, format, strides[4] = { 0 }, handles[4] = { 0 }, offsets[4] = { 0 }, flags = 0;
    int      ret;

    if ( fb )
        return fb;

    fb     = calloc( 1, sizeof( *fb ) );
    fb->bo = bo;

    width  = gbm_bo_get_width( bo );
    height = gbm_bo_get_height( bo );
    format = gbm_bo_get_format( bo );

    memcpy( handles, (uint32_t[4]){ gbm_bo_get_handle( bo ).u32, 0, 0, 0 }, 16 );
    memcpy( strides, (uint32_t[4]){ gbm_bo_get_stride( bo ), 0, 0, 0 }, 16 );
    memset( offsets, 0, 16 );
    ret = drmModeAddFB2( fgDrmDevice.fd, width, height, format, handles, strides, offsets, &fb->fb_id, 0 );

    if ( ret ) {
        fgWarning( "Failed to create framebuffer: %s", strerror( errno ) );
        free( fb );
        return NULL;
    }

    gbm_bo_set_user_data( bo, fb, fgDrmFbDestroyCallback );

    return fb;
}
