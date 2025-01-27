/*
 * fg_window_cocoa.m
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

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#define FREEGLUT_BUILDING_LIB
#include <GL/freeglut.h>
#include <unistd.h> /* usleep, gethostname, getpid */
#include "../fg_internal.h"

/*
 * Opens a window. Requires a SFG_Window object created and attached
 * to the freeglut structure. OpenGL context is created here.
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
    @autoreleasepool {
        NSRect     contentRect = NSMakeRect( x, y, w, h );
        NSUInteger styleMask   = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;

        if ( !window->IsMenu ) {
            styleMask |= NSWindowStyleMaskResizable;
        }

        /* Create the window */
        NSWindow *nswin = [[NSWindow alloc] initWithContentRect:contentRect
                                                      styleMask:styleMask
                                                        backing:NSBackingStoreBuffered
                                                          defer:NO];

        /* Configure window properties */
        [nswin setTitle:[NSString stringWithUTF8String:title]];
        [nswin setAcceptsMouseMovedEvents:YES];
        [nswin setRestorable:NO];

        /* Create OpenGL pixel format */
        NSOpenGLPixelFormatAttribute attributes[] = { NSOpenGLPFADoubleBuffer,
            NSOpenGLPFAColorSize,
            24,
            NSOpenGLPFAAlphaSize,
            8,
            NSOpenGLPFADepthSize,
            24,
            NSOpenGLPFAStencilSize,
            8,
            NSOpenGLPFAAccelerated,
            0 };

        NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
        if ( !pixelFormat ) {
            fgError( "Failed to create OpenGL pixel format" );
        }

        /* Create OpenGL context */
        NSOpenGLContext *context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
        if ( !context ) {
            fgError( "Failed to create OpenGL context" );
        }

        /* Create OpenGL view */
        NSRect        viewRect = NSMakeRect( 0, 0, w, h );
        NSOpenGLView *glView   = [[NSOpenGLView alloc] initWithFrame:viewRect pixelFormat:pixelFormat];
        [glView setOpenGLContext:context];
        [glView setWantsBestResolutionOpenGLSurface:YES];

        /* Set up window */
        [nswin setContentView:glView];
        [nswin makeFirstResponder:glView];
        [glView release];
        [pixelFormat release];

        /* Store window and context in GLUT window structure */
        window->Window.Handle  = (void *)nswin;
        window->Window.Context = (void *)context;

        /* Show window */
        [nswin makeKeyAndOrderFront:nil];
    }
}

/*
 * Request a window resize
 */
void fgPlatformReshapeWindow( SFG_Window *window, int width, int height )
{
    TODO_IMPL;
}

/*
 * Closes a window, destroying the frame and OpenGL context
 */
void fgPlatformCloseWindow( SFG_Window *window )
{
    TODO_IMPL;
}

/*
 * This function makes the specified window visible
 */
void fgPlatformShowWindow( SFG_Window *window )
{
    TODO_IMPL;
}

/*
 * This function hides the specified window
 */
void fgPlatformHideWindow( SFG_Window *window )
{
    TODO_IMPL;
}

/*
 * Iconify the specified window (top-level windows only)
 */
void fgPlatformIconifyWindow( SFG_Window *window )
{
    TODO_IMPL;
}

/*
 * Set the current window's title
 */
void fgPlatformGlutSetWindowTitle( const char *str )
{
    TODO_IMPL;
}

/*
 * Set the current window's iconified title
 */
void fgPlatformGlutSetIconTitle( const char *str )
{
    TODO_IMPL;
}

/*
 * Change the specified window's position
 */
void fgPlatformPositionWindow( SFG_Window *window, int x, int y )
{
    TODO_IMPL;
}

/*
 * Lowers the specified window (by Z order change)
 */
void fgPlatformPushWindow( SFG_Window *window )
{
    TODO_IMPL;
}

/*
 * Raises the specified window (by Z order change)
 */
void fgPlatformPopWindow( SFG_Window *window )
{
    TODO_IMPL;
}

/*
 * Toggle the window's full screen state.
 */
void fgPlatformFullScreenToggle( SFG_Window *win )
{
    TODO_IMPL;
}

void fgPlatformSetWindow( SFG_Window *window )
{
    TODO_IMPL;
}