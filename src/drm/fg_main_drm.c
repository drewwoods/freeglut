/*
 * fg_main_drm.c
 *
 * DRM/KMS/GBM-specific main loop functions
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* For input devices - integrate with libevdev or libinput */
#include <linux/input.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

/* Input device handling */
#define MAX_INPUT_DEVICES 16
static struct {
    int  fd;
    char name[256];
    int  is_keyboard;
    int  is_mouse;
} input_devices[MAX_INPUT_DEVICES];
static int num_input_devices = 0;

/* Initialize input devices - scan for keyboard and mouse devices */
static void fgDrmInitInputDevices( void )
{
    DIR           *dir;
    struct dirent *entry;
    char           path[256];

    /* Open the /dev/input directory */
    dir = opendir( "/dev/input" );
    if ( !dir ) {
        fgWarning( "Could not open /dev/input: %s", strerror( errno ) );
        return;
    }

    /* Scan for event devices */
    while ( ( entry = readdir( dir ) ) != NULL && num_input_devices < MAX_INPUT_DEVICES ) {
        if ( strncmp( entry->d_name, "event", 5 ) == 0 ) {
            int fd;
            snprintf( path, sizeof( path ), "/dev/input/%s", entry->d_name );

            fd = open( path, O_RDONLY | O_NONBLOCK );
            if ( fd == -1 ) {
                fgWarning( "Could not open input device %s: %s", path, strerror( errno ) );
                continue;
            }

            /* Get device name and capabilities */
            char          name[256] = "Unknown";
            unsigned long evbit[NBITS( EV_MAX )];
            unsigned long keybit[NBITS( KEY_MAX )];
            unsigned long relbit[NBITS( REL_MAX )];

            ioctl( fd, EVIOCGNAME( sizeof( name ) - 1 ), name );
            ioctl( fd, EVIOCGBIT( 0, sizeof( evbit ) ), evbit );

            /* Check if it's a keyboard or mouse */
            int is_keyboard = 0;
            int is_mouse    = 0;

            if ( ISSET( evbit, EV_KEY ) ) {
                ioctl( fd, EVIOCGBIT( EV_KEY, sizeof( keybit ) ), keybit );

                /* Check for keyboard keys */
                is_keyboard = ISSET( keybit, KEY_Q ) && ISSET( keybit, KEY_SPACE );

                /* Check for mouse buttons */
                is_mouse = ISSET( keybit, BTN_LEFT );

                /* Also check for relative events (mouse movement) */
                if ( ISSET( evbit, EV_REL ) ) {
                    ioctl( fd, EVIOCGBIT( EV_REL, sizeof( relbit ) ), relbit );
                    is_mouse |= ISSET( relbit, REL_X ) && ISSET( relbit, REL_Y );
                }
            }

            /* If it's a keyboard or mouse, add it to our list */
            if ( is_keyboard || is_mouse ) {
                input_devices[num_input_devices].fd = fd;
                strncpy( input_devices[num_input_devices].name, name, 255 );
                input_devices[num_input_devices].name[255]   = '\0';
                input_devices[num_input_devices].is_keyboard = is_keyboard;
                input_devices[num_input_devices].is_mouse    = is_mouse;

                fgWarning( "Found input device: %s (keyboard: %d, mouse: %d)", name, is_keyboard, is_mouse );

                num_input_devices++;
            }
            else {
                close( fd );
            }
        }
    }

    closedir( dir );

    fgWarning( "Initialized %d input devices", num_input_devices );
}

/* Close input devices */
static void fgDrmCloseInputDevices( void )
{
    int i;
    for ( i = 0; i < num_input_devices; i++ ) {
        if ( input_devices[i].fd >= 0 ) {
            close( input_devices[i].fd );
            input_devices[i].fd = -1;
        }
    }
    num_input_devices = 0;
}

/* Process input events */
static void fgDrmProcessInputEvents( void )
{
    struct input_event ev;
    int                i, rd;
    static int         mouse_x = 0, mouse_y = 0;

    /* For each input device */
    for ( i = 0; i < num_input_devices; i++ ) {
        /* Read all pending events from this device */
        while ( ( rd = read( input_devices[i].fd, &ev, sizeof( ev ) ) ) > 0 ) {
            if ( rd < (int)sizeof( ev ) ) {
                break;
            }

            /* Process the event based on its type */
            if ( ev.type == EV_KEY ) {
                int button = 0;

                /* Keyboard events */
                if ( input_devices[i].is_keyboard ) {
                    int fgKey = -1;

                    /* Map Linux input key to GLUT key */
                    /* This is a simplified mapping */
                    switch ( ev.code ) {
                    case KEY_ESCAPE:
                        fgKey = GLUT_KEY_ESC;
                        break;
                    case KEY_F1:
                        fgKey = GLUT_KEY_F1;
                        break;
                    case KEY_F2:
                        fgKey = GLUT_KEY_F2;
                        break;
                    case KEY_F3:
                        fgKey = GLUT_KEY_F3;
                        break;
                    case KEY_F4:
                        fgKey = GLUT_KEY_F4;
                        break;
                    case KEY_F5:
                        fgKey = GLUT_KEY_F5;
                        break;
                    case KEY_F6:
                        fgKey = GLUT_KEY_F6;
                        break;
                    case KEY_F7:
                        fgKey = GLUT_KEY_F7;
                        break;
                    case KEY_F8:
                        fgKey = GLUT_KEY_F8;
                        break;
                    case KEY_F9:
                        fgKey = GLUT_KEY_F9;
                        break;
                    case KEY_F10:
                        fgKey = GLUT_KEY_F10;
                        break;
                    case KEY_F11:
                        fgKey = GLUT_KEY_F11;
                        break;
                    case KEY_F12:
                        fgKey = GLUT_KEY_F12;
                        break;
                    case KEY_LEFT:
                        fgKey = GLUT_KEY_LEFT;
                        break;
                    case KEY_UP:
                        fgKey = GLUT_KEY_UP;
                        break;
                    case KEY_RIGHT:
                        fgKey = GLUT_KEY_RIGHT;
                        break;
                    case KEY_DOWN:
                        fgKey = GLUT_KEY_DOWN;
                        break;
                    case KEY_PAGEUP:
                        fgKey = GLUT_KEY_PAGE_UP;
                        break;
                    case KEY_PAGEDOWN:
                        fgKey = GLUT_KEY_PAGE_DOWN;
                        break;
                    case KEY_HOME:
                        fgKey = GLUT_KEY_HOME;
                        break;
                    case KEY_END:
                        fgKey = GLUT_KEY_END;
                        break;
                    case KEY_INSERT:
                        fgKey = GLUT_KEY_INSERT;
                        break;
                    }

                    /* Handle special keys */
                    if ( fgKey >= 0 ) {
                        if ( ev.value == 1 || ev.value == 2 ) { /* Key press or repeat */
                            INVOKE_WCB( *fgStructure.CurrentWindow, Special, ( fgKey, mouse_x, mouse_y ) );
                            fgState.Modifiers |= GLUT_ACTIVE_SPECIAL;
                        }
                        else { /* Key release */
                            INVOKE_WCB( *fgStructure.CurrentWindow, SpecialUp, ( fgKey, mouse_x, mouse_y ) );
                            fgState.Modifiers &= ~GLUT_ACTIVE_SPECIAL;
                        }
                    }
                    else {
                        /* ASCII keys */
                        unsigned char ascii = 0;
                        switch ( ev.code ) {
                        case KEY_SPACE:
                            ascii = ' ';
                            break;
                        case KEY_APOSTROPHE:
                            ascii = '\'';
                            break;
                        case KEY_COMMA:
                            ascii = ',';
                            break;
                        case KEY_MINUS:
                            ascii = '-';
                            break;
                        case KEY_DOT:
                            ascii = '.';
                            break;
                        case KEY_SLASH:
                            ascii = '/';
                            break;
                        case KEY_0:
                            ascii = '0';
                            break;
                        case KEY_1:
                            ascii = '1';
                            break;
                        case KEY_2:
                            ascii = '2';
                            break;
                        case KEY_3:
                            ascii = '3';
                            break;
                        case KEY_4:
                            ascii = '4';
                            break;
                        case KEY_5:
                            ascii = '5';
                            break;
                        case KEY_6:
                            ascii = '6';
                            break;
                        case KEY_7:
                            ascii = '7';
                            break;
                        case KEY_8:
                            ascii = '8';
                            break;
                        case KEY_9:
                            ascii = '9';
                            break;
                        case KEY_SEMICOLON:
                            ascii = ';';
                            break;
                        case KEY_EQUAL:
                            ascii = '=';
                            break;
                        case KEY_A:
                            ascii = 'a';
                            break;
                        case KEY_B:
                            ascii = 'b';
                            break;
                        case KEY_C:
                            ascii = 'c';
                            break;
                        case KEY_D:
                            ascii = 'd';
                            break;
                        case KEY_E:
                            ascii = 'e';
                            break;
                        case KEY_F:
                            ascii = 'f';
                            break;
                        case KEY_G:
                            ascii = 'g';
                            break;
                        case KEY_H:
                            ascii = 'h';
                            break;
                        case KEY_I:
                            ascii = 'i';
                            break;
                        case KEY_J:
                            ascii = 'j';
                            break;
                        case KEY_K:
                            ascii = 'k';
                            break;
                        case KEY_L:
                            ascii = 'l';
                            break;
                        case KEY_M:
                            ascii = 'm';
                            break;
                        case KEY_N:
                            ascii = 'n';
                            break;
                        case KEY_O:
                            ascii = 'o';
                            break;
                        case KEY_P:
                            ascii = 'p';
                            break;
                        case KEY_Q:
                            ascii = 'q';
                            break;
                        case KEY_R:
                            ascii = 'r';
                            break;
                        case KEY_S:
                            ascii = 's';
                            break;
                        case KEY_T:
                            ascii = 't';
                            break;
                        case KEY_U:
                            ascii = 'u';
                            break;
                        case KEY_V:
                            ascii = 'v';
                            break;
                        case KEY_W:
                            ascii = 'w';
                            break;
                        case KEY_X:
                            ascii = 'x';
                            break;
                        case KEY_Y:
                            ascii = 'y';
                            break;
                        case KEY_Z:
                            ascii = 'z';
                            break;
                        case KEY_LEFTBRACE:
                            ascii = '[';
                            break;
                        case KEY_BACKSLASH:
                            ascii = '\\';
                            break;
                        case KEY_RIGHTBRACE:
                            ascii = ']';
                            break;
                        case KEY_GRAVE:
                            ascii = '`';
                            break;
                        case KEY_BACKSPACE:
                            ascii = 8;
                            break;
                        case KEY_TAB:
                            ascii = 9;
                            break;
                        case KEY_ENTER:
                            ascii = 13;
                            break;
                        case KEY_ESC:
                            ascii = 27;
                            break;
                        case KEY_DELETE:
                            ascii = 127;
                            break;
                        }

                        if ( ascii ) {
                            if ( ev.value == 1 || ev.value == 2 ) { /* Key press or repeat */
                                INVOKE_WCB( *fgStructure.CurrentWindow, Keyboard, ( ascii, mouse_x, mouse_y ) );
                            }
                            else { /* Key release */
                                INVOKE_WCB( *fgStructure.CurrentWindow, KeyboardUp, ( ascii, mouse_x, mouse_y ) );
                            }
                        }
                    }
                }

                /* Mouse button events */
                if ( input_devices[i].is_mouse ) {
                    int button = -1;

                    switch ( ev.code ) {
                    case BTN_LEFT:
                        button = GLUT_LEFT_BUTTON;
                        break;
                    case BTN_MIDDLE:
                        button = GLUT_MIDDLE_BUTTON;
                        break;
                    case BTN_RIGHT:
                        button = GLUT_RIGHT_BUTTON;
                        break;
                    }

                    if ( button >= 0 ) {
                        if ( ev.value == 1 ) { /* Button press */
                            INVOKE_WCB( *fgStructure.CurrentWindow, Mouse, ( button, GLUT_DOWN, mouse_x, mouse_y ) );
                        }
                        else if ( ev.value == 0 ) { /* Button release */
                            INVOKE_WCB( *fgStructure.CurrentWindow, Mouse, ( button, GLUT_UP, mouse_x, mouse_y ) );
                        }
                    }
                }
            }
            else if ( ev.type == EV_REL ) {
                /* Mouse movement */
                if ( input_devices[i].is_mouse ) {
                    if ( ev.code == REL_X ) {
                        mouse_x += ev.value;
                        /* Keep within bounds */
                        if ( mouse_x < 0 )
                            mouse_x = 0;
                        if ( mouse_x >= fgStructure.CurrentWindow->State.Width )
                            mouse_x = fgStructure.CurrentWindow->State.Width - 1;
                    }
                    else if ( ev.code == REL_Y ) {
                        mouse_y += ev.value;
                        /* Keep within bounds */
                        if ( mouse_y < 0 )
                            mouse_y = 0;
                        if ( mouse_y >= fgStructure.CurrentWindow->State.Height )
                            mouse_y = fgStructure.CurrentWindow->State.Height - 1;
                    }
                    else if ( ev.code == REL_WHEEL ) {
                        /* Mouse wheel */
                        int button = ( ev.value > 0 ) ? 3 : 4; /* 3=wheel up, 4=wheel down */

                        INVOKE_WCB( *fgStructure.CurrentWindow, Mouse, ( button, GLUT_DOWN, mouse_x, mouse_y ) );
                        INVOKE_WCB( *fgStructure.CurrentWindow, Mouse, ( button, GLUT_UP, mouse_x, mouse_y ) );
                    }

                    /* If mouse position changed, generate motion callback */
                    if ( ev.code == REL_X || ev.code == REL_Y ) {
                        INVOKE_WCB( *fgStructure.CurrentWindow, Motion, ( mouse_x, mouse_y ) );
                    }
                }
            }
        }

        /* Handle read errors */
        if ( rd < 0 && errno != EAGAIN && errno != EWOULDBLOCK ) {
            fgWarning( "Error reading from input device %d: %s", i, strerror( errno ) );
            close( input_devices[i].fd );
            input_devices[i].fd = -1;
        }
    }
}

/* The main loop - process input and render the scene */
void fgDrmMainLoop( void )
{
    SFG_Window *window = fgStructure.CurrentWindow;

    /* Initialize input devices if not done yet */
    if ( num_input_devices == 0 ) {
        fgDrmInitInputDevices( );
    }

    /* If no window is visible, this is a good moment for some housekeeping */
    if ( !fgStructure.windows.first ) {
        fgDrmCloseInputDevices( );
        fgDrmClose( );
        exit( 0 );
    }

    /* Check if a window is available for rendering */
    while ( window ) {
        if ( window->State.Visible ) {
            break;
        }
        window = window->next;
    }

    /* Set up for the display callback */
    if ( window ) {
        fgStructure.CurrentWindow = window;

        /* Set up the window for rendering */
        if ( !fgSetWindow( window ) ) {
            fgWarning( "Failed to make window current" );
            return;
        }

        /* Make sure a display callback is defined */
        if ( FETCH_WCB( *window, Display ) ) {
            /* Process input events */
            fgDrmProcessInputEvents( );

            /* Handle idle, timer, and menu state */
            fgState.ExecIdle = GL_TRUE;
            fgState.ExecMenu = GL_FALSE;

            /* Invoke idle callback if registered */
            if ( fgState.IdleCallback ) {
                fgState.IdleCallback( );
            }

            /* Invoke timer callbacks that are due */
            fgListAppend( &fgState.Timers.Free, &fgState.Timers.Active );
            while ( fgState.Timers.Active.first ) {
                SFG_Timer *timer = fgState.Timers.Active.first;
                fgListRemove( &fgState.Timers.Active, &timer->node );

                if ( timer->callback ) {
                    timer->callback( timer->id );
                }

                /* If it's not a one-time callback, reinstall it */
                if ( timer->repeatTime > 0 ) {
                    timer->triggerTime = fgElapsedTime( ) + timer->repeatTime;
                    fgListAppend( &fgState.Timers.Active, &timer->node );
                }
                else {
                    fgListAppend( &fgState.Timers.Free, &timer->node );
                }
            }

            /* Invoke display callback */
            INVOKE_WCB( *window, Display, ( ) );
            fgDrmDisplayWindow( window );

            /* Swap buffers implicitly at the end of display function */
        }
    }

    /* Sleep to avoid consuming 100% CPU */
    /* TODO: Use select with timeout to sleep more efficiently */
    usleep( 10000 ); /* 10ms sleep */
}
