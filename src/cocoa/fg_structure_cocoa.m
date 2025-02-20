/*
 * fg_structure_cocoa.m
 *
 * The freeglut library private include file.
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

void fgPlatformCreateWindow(SFG_Window* window)
{
    /* Initialize window context and platform-specific data */
    window->Window.Handle                = 0;    /* Will be set when actual window is created */
    window->Window.Context               = NULL; /* OpenGL context */
    window->Window.pContext.CocoaContext = NULL; /* Platform-specific context */
    window->Window.pContext.PixelFormat  = NULL; /* Will be set during OpenGL setup */

    /* Initialize colormap */
    window->Window.cmap      = NULL;
    window->Window.cmap_size = 0;

    /* Initialize window state */
    window->State.pWState.OldWidth  = -1;
    window->State.pWState.OldHeight = -1;

    if (window->IsMenu) {
        window->State.pWState.OldWidth  = 100;
        window->State.pWState.OldHeight = 100;
    }
}
