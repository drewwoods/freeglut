/*
 * fg_init_osmesa.c
 *
 * Initialization methods for the OSMesa (off-screen) backend.
 *
 * Copyright (c) 2026 freeglut contributors. All Rights Reserved.
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

/*
 * OSMesa renders into a client-supplied memory buffer with no window system,
 * so there is no display server to open. We set up a nominal virtual screen
 * size (for glutGet(GLUT_SCREEN_*)) and the monotonic time base.
 */
void fgPlatformInitialize( const char *displayName )
{
    fgDisplay.ScreenWidth    = 1024;
    fgDisplay.ScreenHeight   = 768;
    fgDisplay.ScreenWidthMM  = 0;
    fgDisplay.ScreenHeightMM = 0;

    fgState.Time = fgSystemTime();
    fgState.Initialised = GL_TRUE;

    atexit( fgDeinitialize );
}

void fgPlatformCloseDisplay( void )
{
    /* No display server / window system to tear down. */
}

void fgPlatformDestroyContext( SFG_PlatformDisplay pDisplay,
                               SFG_WindowContextType MContext )
{
    if( MContext )
        OSMesaDestroyContext( MContext );
}

void fgPlatformDeinitialiseInputDevices( void )
{
    fgState.JoysticksInitialised = GL_FALSE;
    fgState.InputDevsInitialised = GL_FALSE;
}
