/*
 * fg_display_osmesa.c
 *
 * Buffer-swap and GL-extension query for the OSMesa backend.
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

void fgPlatformGlutSwapBuffers( SFG_PlatformDisplay *pDisplayPtr,
                                SFG_Window *CurrentWindow )
{
    /* Single-buffered: there is nothing to swap. glFinish() so that a
     * subsequent OSMesaGetColorBuffer()/glReadPixels() sees a complete frame. */
    glFinish();
}

void fgPlatformInitSwapCtl( void )
{
}

void fgPlatformSwapInterval( int n )
{
}

int fgPlatformExtSupported( const char *ext )
{
    /* This is the window-system extension hook (the analogue of querying GLX or
     * WGL extensions); OSMesa has no window system. GL extensions proper are
     * queried by the generic glutExtensionSupported() before it falls back here,
     * so there is nothing for this backend to report. */
    return 0;
}
