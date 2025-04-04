/*
 * fg_cursor_drm.c
 *
 * DRM/KMS/GBM-specific cursor rendering
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

/*
 * DRM/KMS cursor handling
 *
 * Setting hardware cursor requires using the DRM API,
 * which is a bit complex for a full implementation.
 *
 * For now, we provide a minimal implementation that
 * could be expanded in the future to support hardware
 * cursor rendering.
 */

void fgDrmSetCursor( SFG_Window *window, int cursorID )
{
    /* Basic cursor implementation - this could be expanded later */
    /* For a full implementation, you'd need to:
       1. Create a DRM buffer with the cursor image
       2. Use the drmModeSetCursor/drmModeSetCursor2 functions to display it
       3. Handle cursor positioning with drmModeMoveCursor
    */

    /* Just store the cursor ID for now */
    window->State.Cursor = cursorID;
}
