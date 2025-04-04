/*
 * fg_structure_drm.c
 *
 * DRM/KMS/GBM-specific structure management functions
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
#include <string.h>

/*
 * Create a menu for the current window
 */
void fgPlatformCreateMenu( SFG_Menu *menu )
{
    /* Nothing special for DRM/KMS case */
}

/*
 * Destroy the menu associated with the current window
 */
void fgPlatformDestroyMenu( SFG_Menu *menu )
{
    /* Nothing special for DRM/KMS case */
}

/*
 * Set the window's position on the screen
 */
void fgPlatformPositionWindow( SFG_Window *window, int x, int y )
{
    /* In DRM/KMS we're fullscreen, so this is a no-op */
    /* But we do need to track the position internally for GLUT API */
    window->State.Xpos = x;
    window->State.Ypos = y;
}

/*
 * Resize the window on the screen
 */
void fgPlatformReshapeWindow( SFG_Window *window, int width, int height )
{
    /* Recreate the window at the new size */
    fgDrmReshapeWindow( window, width, height );

    /* Store the new size */
    window->State.Width  = width;
    window->State.Height = height;

    /* Call the reshape callback if it exists */
    if ( FETCH_WCB( *window, Reshape ) )
        INVOKE_WCB( *window, Reshape, ( width, height ) );
}

/*
 * Show/hide the current window
 */
void fgPlatformVisibilityChange( SFG_Window *window, GLboolean visible )
{
    /* Update window visibility state */
    window->State.Visible = visible;

    /* In DRM/KMS we're always visible when running, but we can handle the callbacks */
    if ( visible ) {
        INVOKE_WCB( *window, WindowStatus, ( GLUT_FULLY_RETAINED ) );
    }
    else {
        INVOKE_WCB( *window, WindowStatus, ( GLUT_FULLY_COVERED ) );
    }
}

/*
 * Enter/leave game mode
 */
GLboolean fgPlatformGameModeGet( SFG_GameModeWindow *game_mode, SFG_GameModeSettings *settings )
{
    /* Game mode is essentially the same as normal operation in DRM/KMS - we're always fullscreen */
    /* Just provide the current screen dimensions */
    settings->width        = fgDisplay.ScreenWidth;
    settings->height       = fgDisplay.ScreenHeight;
    settings->depth        = 24; /* Standard depth */
    settings->refresh_rate = 60; /* Default refresh rate - could query actual rate from DRM mode */

    return GL_TRUE;
}

/*
 * Change the current window's cursor
 */
void fgPlatformSetCursor( SFG_Window *window, int cursorID )
{
    /* Handle cursor changes for DRM/KMS - often requires kernel interface */
    /* For now, we'll just invoke the function that will be a stub */
    fgDrmSetCursor( window, cursorID );
}

/*
 * Establish the window z-order
 */
void fgPlatformPushWindow( SFG_Window *window )
{
    /* No z-order in DRM/KMS - always on top */
}

void fgPlatformPopWindow( SFG_Window *window )
{
    /* No z-order in DRM/KMS - always on top */
}

/*
 * Make the current window visible
 */
void fgPlatformShowWindow( SFG_Window *window )
{
    /* Window is always visible in DRM/KMS mode, just update state */
    window->State.Visible = GL_TRUE;
}

/*
 * Hide the current window
 */
void fgPlatformHideWindow( SFG_Window *window )
{
    /* Can't actually hide window in DRM/KMS mode but update state */
    window->State.Visible = GL_FALSE;
    fgDrmHideWindow( window );
}

/*
 * Iconify the current window
 */
void fgPlatformIconifyWindow( SFG_Window *window )
{
    /* Not applicable in DRM/KMS mode */
    window->State.Visible = GL_FALSE;
}

/*
 * Set current window's title
 */
void fgPlatformGlutSetWindowTitle( SFG_Window *window, const char *title )
{
    /* No window title visible in DRM/KMS mode */
}

void fgPlatformGlutSetIconTitle( SFG_Window *window, const char *title )
{
    /* No icon title in DRM/KMS mode */
}

/*
 * Initialize the window structure
 */
void fgPlatformCreateWindow( SFG_Window *window )
{
    /* If given position is negative, calculate from screen size */
    if ( window->State.Width <= 0 )
        window->State.Width = fgDisplay.ScreenWidth;
    if ( window->State.Height <= 0 )
        window->State.Height = fgDisplay.ScreenHeight;

    /* Create the DRM window */
    if ( !fgDrmCreateWindow( window ) ) {
        fgError( "Failed to create DRM/KMS window" );
        return;
    }

    /* Window is now visible */
    window->State.Visible = GL_TRUE;

    /* Call the reshape callback if it exists */
    if ( FETCH_WCB( *window, Reshape ) )
        INVOKE_WCB( *window, Reshape, ( window->State.Width, window->State.Height ) );
}

/*
 * Close a window, destroy the structure
 */
void fgPlatformCloseWindow( SFG_Window *window )
{
    /* Destroy the DRM window */
    fgDrmDestroyWindow( window );
}

/*
 * Destroy a menu and all its entries
 */
void fgPlatformDestroyWindowAndContext( SFG_Window *window )
{
    /* In DRM/KMS we only have the window, and we've already closed it */
    /* So nothing more to do here */
}

/*
 * Request a new top-level window
 */
void fgPlatformOpenWindow( SFG_Window *window,
    const char                        *title,
    GLboolean                          positionUse,
    int                                x,
    int                                y,
    GLboolean                          sizeUse,
    int                                w,
    int                                h,
    GLboolean                          gameMode,
    GLboolean                          isSubWindow )
{
    /* In DRM/KMS, there are no real "windows" in the traditional sense */
    /* We'll ignore the position arguments as we're always fullscreen */

    /* Set the window's width/height based on arguments or screen size */
    if ( sizeUse ) {
        window->State.Width  = w;
        window->State.Height = h;
    }
    else {
        window->State.Width  = fgDisplay.ScreenWidth;
        window->State.Height = fgDisplay.ScreenHeight;
    }

    /* Store the requested position, even though it doesn't apply in DRM mode */
    if ( positionUse ) {
        window->State.Xpos = x;
        window->State.Ypos = y;
    }
    else {
        window->State.Xpos = 0;
        window->State.Ypos = 0;
    }

    /* Create the actual window */
    fgPlatformCreateWindow( window );
}
