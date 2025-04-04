/*
 * fg_state_drm.c
 *
 * DRM/KMS/GBM-specific state query functions
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
#include <EGL/egl.h>

/*
 * Get the display size of the screen
 */
int fgPlatformGlutGet( GLenum eWhat )
{
    /* The DRM/KMS case is fairly simple since we operate on a single screen */
    switch ( eWhat ) {
    case GLUT_SCREEN_WIDTH:
        return fgDisplay.ScreenWidth;

    case GLUT_SCREEN_HEIGHT:
        return fgDisplay.ScreenHeight;

    case GLUT_SCREEN_WIDTH_MM:
        return fgDisplay.ScreenWidthMM;

    case GLUT_SCREEN_HEIGHT_MM:
        return fgDisplay.ScreenHeightMM;

    case GLUT_INIT_DISPLAY_MODE:
        return fgState.DisplayMode;

    case GLUT_INIT_WINDOW_X:
        return fgState.Position.X;

    case GLUT_INIT_WINDOW_Y:
        return fgState.Position.Y;

    case GLUT_INIT_WINDOW_WIDTH:
        return fgState.Size.X;

    case GLUT_INIT_WINDOW_HEIGHT:
        return fgState.Size.Y;

    /* The window's current position and size */
    case GLUT_WINDOW_X:
        if ( fgStructure.CurrentWindow )
            return fgStructure.CurrentWindow->State.Xpos;
        return 0;

    case GLUT_WINDOW_Y:
        if ( fgStructure.CurrentWindow )
            return fgStructure.CurrentWindow->State.Ypos;
        return 0;

    case GLUT_WINDOW_WIDTH:
        if ( fgStructure.CurrentWindow )
            return fgStructure.CurrentWindow->State.Width;
        return 0;

    case GLUT_WINDOW_HEIGHT:
        if ( fgStructure.CurrentWindow )
            return fgStructure.CurrentWindow->State.Height;
        return 0;

    /* Since we're fullscreen in DRM/KMS, the window size is always the full screen size */
    case GLUT_WINDOW_BUFFER_SIZE:
        return 32; /* Most DRM/KMS systems use 32-bit color */

    case GLUT_WINDOW_STENCIL_SIZE:
        return 8; /* Standard stencil depth */

    case GLUT_WINDOW_RED_SIZE:
    case GLUT_WINDOW_GREEN_SIZE:
    case GLUT_WINDOW_BLUE_SIZE:
        return 8; /* Standard 8 bits per component */

    case GLUT_WINDOW_ALPHA_SIZE:
        return 8; /* Standard alpha channel depth */

    case GLUT_WINDOW_DEPTH_SIZE:
        return 24; /* Standard depth buffer size */

    case GLUT_WINDOW_DOUBLEBUFFER:
        return 1; /* Always double-buffered */

    case GLUT_WINDOW_RGBA:
        return 1; /* Always RGBA mode */

    case GLUT_WINDOW_COLORMAP_SIZE:
        return 0; /* No colormap */

    case GLUT_ELAPSED_TIME:
        return (int)fgElapsedTime( );

    default:
        return -1;
    }
}

/*
 * Returns the GLUT modifier flags for the current window
 */
int fgPlatformGlutGetModifiers( void )
{
    /* Return the current modifiers state */
    return fgState.Modifiers;
}

/*
 * Return the state of the GLUT API
 */
int fgPlatformGlutDeviceGet( GLenum eWhat )
{
    switch ( eWhat ) {
    case GLUT_HAS_KEYBOARD:
        return 1; /* Always assume there is a keyboard */

    case GLUT_HAS_MOUSE:
        return 1; /* Always assume there is a mouse */

    case GLUT_NUM_MOUSE_BUTTONS:
        return 3; /* Assume standard 3-button mouse */

    default:
        return -1;
    }
}

/*
 * Returns the freeglut extension support state
 */
int fgPlatformGlutExtensionSupported( const char *extension )
{
    const char *extensions;
    const char *start;
    const char *where, *terminator;

    /* Check if EGL supports the extension */
    if ( fgDrmDevice.egl_display ) {
        extensions = eglQueryString( fgDrmDevice.egl_display, EGL_EXTENSIONS );
        if ( extensions ) {
            /* Extension names must not have spaces. */
            where = strstr( extensions, extension );
            if ( where ) {
                terminator = where + strlen( extension );
                if ( where == extensions || *( where - 1 ) == ' ' )
                    if ( *terminator == ' ' || *terminator == '\0' )
                        return 1;
            }
        }
    }

    /* Check if OpenGL supports the extension */
    if ( fgState.MajorVersion < 3 ) {
        /* Old style extension query */
        extensions = (const char *)glGetString( GL_EXTENSIONS );
        if ( extensions ) {
            /* Extension names must not have spaces. */
            where = strstr( extensions, extension );
            if ( where ) {
                terminator = where + strlen( extension );
                if ( where == extensions || *( where - 1 ) == ' ' )
                    if ( *terminator == ' ' || *terminator == '\0' )
                        return 1;
            }
        }
    }
    else {
        /* New style extension query for OpenGL 3.0+ */
        GLint numExtensions;
        GLint i;

        glGetIntegerv( GL_NUM_EXTENSIONS, &numExtensions );

        for ( i = 0; i < numExtensions; i++ ) {
            const char *name = (const char *)glGetStringi( GL_EXTENSIONS, i );
            if ( strcmp( name, extension ) == 0 )
                return 1;
        }
    }

    return 0;
}
