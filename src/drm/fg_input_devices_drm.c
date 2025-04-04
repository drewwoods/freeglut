/*
 * fg_input_devices_drm.c
 *
 * DRM/KMS/GBM-specific input device functions
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

/* -- JOYSTICK FUNCTIONS --------------------------------------------------- */
/* Most of this code can be shared with the X11 implementation */

/*
 * The static joystick structure pointer
 */
static fgJoystick *fgJoystick = NULL;

/*
 * Read the joystick state
 */
void fgPlatformJoystickRawRead( SFG_Joystick *joy, int *buttons, float *axes )
{
    /* Currently just a basic implementation - can be enhanced later */
    /* This uses the Linux joystick interface similar to the X11 implementation */
    if ( joy->pJoystick.js_fd < 0 ) {
        return;
    }

    /* Get the buffer state using the standard Linux joystick API */
    struct js_event js;
    while ( read( joy->pJoystick.js_fd, &js, sizeof( struct js_event ) ) > 0 ) {
        switch ( js.type & ~JS_EVENT_INIT ) {
        case JS_EVENT_BUTTON:
            if ( js.number < joy->num_buttons ) {
                buttons[js.number] = js.value;
            }
            break;
        case JS_EVENT_AXIS:
            if ( js.number < joy->num_axes ) {
                axes[js.number] = (float)js.value / 32767.0f;
            }
            break;
        }
    }
}

/*
 * Open the joystick device
 */
void fgPlatformJoystickOpen( SFG_Joystick *joy )
{
    /* Open the device using the standard Linux joystick API */
    char deviceName[256];
    int  i;

    /* Default for axes min and max is -/+ 32767 */
    for ( i = 0; i < joy->num_axes; i++ ) {
        joy->min[i]       = -32767;
        joy->max[i]       = 32767;
        joy->dead_band[i] = 0;
        joy->saturate[i]  = 1;
        joy->center[i]    = 0;
    }

    /* No buttons starting out */
    joy->num_buttons = 0;

    /* Try to open the joystick device */
    snprintf( deviceName, sizeof( deviceName ), "/dev/input/js%d", joy->id );
    joy->pJoystick.js_fd = open( deviceName, O_RDONLY | O_NONBLOCK );
    if ( joy->pJoystick.js_fd < 0 ) {
        fgWarning( "fgJoystickOpen: failed to open %s: %s", deviceName, strerror( errno ) );
        return;
    }

    /* Get joystick information */
    /* Axis and button counts */
    char buf[80];
    ioctl( joy->pJoystick.js_fd, JSIOCGAXES, &joy->num_axes );
    ioctl( joy->pJoystick.js_fd, JSIOCGBUTTONS, &joy->num_buttons );
    ioctl( joy->pJoystick.js_fd, JSIOCGNAME( sizeof( buf ) ), buf );
    if ( joy->num_buttons > _JS_MAX_BUTTONS )
        joy->num_buttons = _JS_MAX_BUTTONS;
    if ( joy->num_axes > _JS_MAX_AXES )
        joy->num_axes = _JS_MAX_AXES;

    /* Set the name */
    joy->name = strdup( buf );

    /* Empty the input events in the queue */
    struct js_event js;
    while ( read( joy->pJoystick.js_fd, &js, sizeof( struct js_event ) ) > 0 )
        ;
}

/*
 * Close the joystick device
 */
void fgPlatformJoystickClose( SFG_Joystick *joy )
{
    if ( joy->pJoystick.js_fd >= 0 ) {
        close( joy->pJoystick.js_fd );
        joy->pJoystick.js_fd = -1;
    }
    if ( joy->name ) {
        free( joy->name );
        joy->name = NULL;
    }
}

/*
 * Get joystick device status
 */
fgJoystick *fgPlatformJoystickInit( int ident )
{
    if ( fgJoystick == NULL ) {
        /* Allocate memory for the joystick structure */
        fgJoystick = (fgJoystick *)calloc( sizeof( fgJoystick ), 1 );
    }

    /* Initialize the joystick structure */
    fgJoystick->id              = ident;
    fgJoystick->error           = GL_FALSE;
    fgJoystick->num_axes        = 0;
    fgJoystick->num_buttons     = 0;
    fgJoystick->name            = NULL;
    fgJoystick->pJoystick.js_fd = -1;

    /* Open the joystick device */
    fgPlatformJoystickOpen( fgJoystick );

    /* Return the joystick structure pointer */
    return fgJoystick;
}

/* -- SPACEBALL/SPACEMOUSE FUNCTIONS --------------------------------------- */
/* Currently not implemented (placeholder stubs) */

void fgPlatformInitializeSpaceball( void )
{
    /* Not implemented yet */
}

void fgPlatformSpaceballClose( void )
{
    /* Not implemented yet */
}

int fgPlatformHasSpaceball( void )
{
    /* Not implemented yet */
    return 0;
}

int fgPlatformSpaceballNumButtons( void )
{
    /* Not implemented yet */
    return 0;
}

void fgPlatformSpaceballSetWindow( SFG_Window *window )
{
    /* Not implemented yet */
}

int fgPlatformPeekSpaceball( void )
{
    /* Not implemented yet */
    return 0;
}

void fgPlatformProcessSpaceballEvent( void )
{
    /* Not implemented yet */
}
