/*
 * Copyright (c) 1999-2000 Pawel W. Olszta. All Rights Reserved.
 * Written by Pawel W. Olszta, <olszta@sourceforge.net>
 * Creation date: Thu Dec 2 1999
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
/* Various initialization functions */

#define FREEGLUT_BUILDING_LIB
#include <GL/freeglut.h>
#include "fg_internal.h"

/*
 * A structure pointed by fgDisplay holds all information
 * regarding the display, screen, root window etc.
 */
SFG_Display fgDisplay;

/*
 * The settings for the current freeglut session
 */
SFG_State fgState = { { -1, -1, GL_FALSE },  /* Position */
                      { 300, 300, GL_TRUE }, /* Size */
                      GLUT_RGBA | GLUT_SINGLE | GLUT_DEPTH,  /* DisplayMode */
                      GL_FALSE,              /* Initialised */
                      GLUT_TRY_DIRECT_CONTEXT,  /* DirectContext */
                      GL_FALSE,              /* ForceIconic */
                      GL_FALSE,              /* UseCurrentContext */
                      GL_FALSE,              /* GLDebugSwitch */
                      GL_FALSE,              /* XSyncSwitch */
                      GLUT_KEY_REPEAT_ON,    /* KeyRepeat */
                      INVALID_MODIFIERS,     /* Modifiers */
                      0,                     /* FPSInterval */
                      0,                     /* SwapCount */
                      0,                     /* SwapTime */
                      0,                     /* Time */
                      { NULL, NULL },         /* Timers */
                      { NULL, NULL },         /* FreeTimers */
                      NULL,                   /* IdleCallback */
                      NULL,                   /* IdleCallbackData */
                      0,                      /* ActiveMenus */
                      NULL,                   /* MenuStateCallback */
                      NULL,                   /* MenuStatusCallback */
                      NULL,                   /* MenuStatusCallbackData */
                      FREEGLUT_MENU_FONT,
                      { -1, -1, GL_TRUE },    /* GameModeSize */
                      -1,                     /* GameModeDepth */
                      -1,                     /* GameModeRefresh */
                      GLUT_ACTION_EXIT,       /* ActionOnWindowClose */
                      GLUT_EXEC_STATE_INIT,   /* ExecState */
                      NULL,                   /* ProgramName */
                      GL_FALSE,               /* JoysticksInitialised */
                      0,                      /* NumActiveJoysticks */
                      GL_FALSE,               /* InputDevsInitialised */
                      0,                      /* MouseWheelTicks */
                      1,                      /* AuxiliaryBufferNumber */
                      4,                      /* SampleNumber */
                      GL_FALSE,               /* SkipStaleMotion */
                      GL_FALSE,               /* StrokeFontDrawJoinDots */
                      GL_FALSE,               /* AllowNegativeWindowPosition */
                      1,                      /* OpenGL context MajorVersion */
                      0,                      /* OpenGL context MinorVersion */
                      0,                      /* OpenGL ContextFlags */
                      0,                      /* OpenGL ContextProfile */
                      0,                      /* HasOpenGL20 */
                      -1,                     /* HasSwapCtlTear */
                      NULL,                   /* ErrorFunc */
                      NULL,                   /* ErrorFuncData */
                      NULL,                   /* WarningFunc */
                      NULL,                   /* WarningFuncData */
                      { {FG_NONE,0}, {FG_NONE,0}, {FG_NONE,0}, {FG_NONE,0}, /* DisplayStrCriteria: alpha, red, green, blue */
                        {FG_NONE,0}, {FG_NONE,0}, {FG_NONE,0}, {FG_NONE,0}, /* depth, stencil, accumRed, accumGreen */
                        {FG_NONE,0}, {FG_NONE,0}, {FG_NONE,0}, {FG_NONE,0}, /* accumBlue, accumAlpha, samples, auxBuffers */
                        {FG_NONE,0}, 0, GL_FALSE }                           /* buffer, num, haveDisplayString */
};


extern void fgPlatformInitialize( const char* displayName );
extern void fgPlatformDeinitialiseInputDevices ( void );
extern void fgPlatformCloseDisplay ( void );
extern void fgPlatformDestroyContext ( SFG_PlatformDisplay pDisplay, SFG_WindowContextType MContext );

void fghParseCommandLineArguments ( int* pargc, char** argv, char **pDisplayName, char **pGeometry )
{
#ifndef _WIN32_WCE
    int i, j, argc = *pargc;

    {
        /* check if GLUT_FPS env var is set */
        const char *fps = getenv( "GLUT_FPS" );

        if( fps )
        {
            int interval;
            sscanf( fps, "%d", &interval );

            if( interval <= 0 )
                fgState.FPSInterval = 5000;  /* 5000 millisecond default */
            else
                fgState.FPSInterval = interval;
        }
    }

    *pDisplayName = getenv( "DISPLAY" );

    for( i = 1; i < argc; i++ )
    {
        if( strcmp( argv[ i ], "-display" ) == 0 )
        {
            if( ++i >= argc )
                fgError( "-display parameter must be followed by display name" );

            *pDisplayName = argv[ i ];

            argv[ i - 1 ] = NULL;
            argv[ i     ] = NULL;
            ( *pargc ) -= 2;
        }
        else if( strcmp( argv[ i ], "-geometry" ) == 0 )
        {
            if( ++i >= argc )
                fgError( "-geometry parameter must be followed by window "
                         "geometry settings" );

            *pGeometry = argv[ i ];

            argv[ i - 1 ] = NULL;
            argv[ i     ] = NULL;
            ( *pargc ) -= 2;
        }
        else if( strcmp( argv[ i ], "-direct" ) == 0)
        {
            if( fgState.DirectContext == GLUT_FORCE_INDIRECT_CONTEXT )
                fgError( "parameters ambiguity, -direct and -indirect "
                    "cannot be both specified" );

            fgState.DirectContext = GLUT_FORCE_DIRECT_CONTEXT;
            argv[ i ] = NULL;
            ( *pargc )--;
        }
        else if( strcmp( argv[ i ], "-indirect" ) == 0 )
        {
            if( fgState.DirectContext == GLUT_FORCE_DIRECT_CONTEXT )
                fgError( "parameters ambiguity, -direct and -indirect "
                    "cannot be both specified" );

            fgState.DirectContext = GLUT_FORCE_INDIRECT_CONTEXT;
            argv[ i ] = NULL;
            (*pargc)--;
        }
        else if( strcmp( argv[ i ], "-iconic" ) == 0 )
        {
            fgState.ForceIconic = GL_TRUE;
            argv[ i ] = NULL;
            ( *pargc )--;
        }
        else if( strcmp( argv[ i ], "-gldebug" ) == 0 )
        {
            fgState.GLDebugSwitch = GL_TRUE;
            argv[ i ] = NULL;
            ( *pargc )--;
        }
        else if( strcmp( argv[ i ], "-sync" ) == 0 )
        {
            fgState.XSyncSwitch = GL_TRUE;
            argv[ i ] = NULL;
            ( *pargc )--;
        }
    }

    /* Compact {argv}. */
    for( i = j = 1; i < *pargc; i++, j++ )
    {
        /* Guaranteed to end because there are "*pargc" arguments left */
        while ( argv[ j ] == NULL )
            j++;
        if ( i != j )
            argv[ i ] = argv[ j ];
    }

#endif /* _WIN32_WCE */

}


void fghCloseInputDevices ( void )
{
    if ( fgState.JoysticksInitialised )
        fgJoystickClose( );

    if ( fgState.InputDevsInitialised )
        fgInputDeviceClose( );
}


/* Perform the freeglut deinitialization...  */
void fgDeinitialize( void )
{
    SFG_Timer *timer;

    if( !fgState.Initialised )
    {
        return;
    }

    /* If we're in game mode, we want to leave game mode */
    if( fgStructure.GameModeWindow ) {
        glutLeaveGameMode();
    }

    /* If there was a menu created, destroy the rendering context */
    if( fgStructure.MenuContext )
    {
        fgPlatformDestroyContext (fgDisplay.pDisplay, fgStructure.MenuContext->MContext );
        free( fgStructure.MenuContext );
        fgStructure.MenuContext = NULL;
    }

    fgDestroyStructure( );

    while( ( timer = fgState.Timers.First) )
    {
        fgListRemove( &fgState.Timers, &timer->Node );
        free( timer );
    }

    while( ( timer = fgState.FreeTimers.First) )
    {
        fgListRemove( &fgState.FreeTimers, &timer->Node );
        free( timer );
    }

    fgPlatformDeinitialiseInputDevices ();

    fgState.MouseWheelTicks = 0;

    fgState.MajorVersion = 1;
    fgState.MinorVersion = 0;
    fgState.ContextFlags = 0;
    fgState.ContextProfile = 0;

    fgState.Initialised = GL_FALSE;

    fgState.Position.X = -1;
    fgState.Position.Y = -1;
    fgState.Position.Use = GL_FALSE;

    fgState.Size.X = 300;
    fgState.Size.Y = 300;
    fgState.Size.Use = GL_TRUE;

    fgState.DisplayMode = GLUT_RGBA | GLUT_SINGLE | GLUT_DEPTH;

    fgState.DirectContext  = GLUT_TRY_DIRECT_CONTEXT;
    fgState.ForceIconic         = GL_FALSE;
    fgState.UseCurrentContext   = GL_FALSE;
    fgState.GLDebugSwitch       = GL_FALSE;
    fgState.XSyncSwitch         = GL_FALSE;
    fgState.ActionOnWindowClose = GLUT_ACTION_EXIT;
    fgState.ExecState           = GLUT_EXEC_STATE_INIT;

    fgState.KeyRepeat       = GLUT_KEY_REPEAT_ON;
    fgState.Modifiers       = INVALID_MODIFIERS;

    fgState.GameModeSize.X  = -1;
    fgState.GameModeSize.Y  = -1;
    fgState.GameModeDepth   = -1;
    fgState.GameModeRefresh = -1;

    fgListInit( &fgState.Timers );
    fgListInit( &fgState.FreeTimers );

    fgState.IdleCallback           = ( FGCBIdleUC )NULL;
    fgState.IdleCallbackData       = NULL;
    fgState.MenuStateCallback      = ( FGCBMenuState )NULL;
    fgState.MenuStatusCallback     = ( FGCBMenuStatusUC )NULL;
    fgState.MenuStatusCallbackData = NULL;

    fgState.SwapCount   = 0;
    fgState.SwapTime    = 0;
    fgState.FPSInterval = 0;

    if( fgState.ProgramName )
    {
        free( fgState.ProgramName );
        fgState.ProgramName = NULL;
    }

    fgPlatformCloseDisplay ();

    fgState.Initialised = GL_FALSE;
}

/* Comparison to distinguish invalid criteria during parsing */
#define FG_INVALID ((FGCriterionComparison)(-1))

/* Parse a display string token and return a Criterion struct.
 * Based on the original GLUT implementation:
 * https://github.com/markkilgard/glut/blob/master/lib/glut/glut_dstr.c
 * Implements all valid comparators as described in glutInitDisplayString man page:
 *   = (equal), != (not equal)
 *   < (less than), > (greater than)
 *   <= (less than or equal), >= (greater than or equal)
 *   ~ (greater than or equal but preferring less)
 */
static FGCriterion parseCriteria(const char *word)
{
    FGCriterion c = { FG_NONE, 0 };
    const char *cstr, *vstr = NULL;
    char *endptr;

    cstr = strpbrk( word, "=><!~" );

    if ( cstr ) {
        switch ( cstr[0] ) {
        case '=':
            c.comparison = FG_EQ;
            vstr         = &cstr[1];
            break;
        case '~':
            c.comparison = FG_MIN;
            vstr         = &cstr[1];
            break;
        case '>':
            if ( cstr[1] == '=' ) {
                c.comparison = FG_GTE;
                vstr         = &cstr[2];
            }
            else {
                c.comparison = FG_GT;
                vstr         = &cstr[1];
            }
            break;
        case '<':
            if ( cstr[1] == '=' ) {
                c.comparison = FG_LTE;
                vstr         = &cstr[2];
            }
            else {
                c.comparison = FG_LT;
                vstr         = &cstr[1];
            }
            break;
        case '!':
            if ( cstr[1] == '=' ) {
                c.comparison = FG_NEQ;
                vstr         = &cstr[2];
            }
            else {
                c.comparison = FG_INVALID;
                return c;
            }
            break;
        default:
            c.comparison = FG_INVALID;
            return c;
        }

        if ( vstr && *vstr ) {
            c.value = (int)strtol( vstr, &endptr, 0 );
            if ( endptr == vstr || *endptr != '\0' ) {
                /* Not a valid number or has trailing garbage */
                c.comparison = FG_INVALID;
                c.value = 0;
            }
        } else {
            /* Comparator present but no value */
            c.comparison = FG_INVALID;
        }
    }

    return c;
}

#if defined(NEED_XPARSEGEOMETRY_IMPL) || defined(TARGET_HOST_MS_WINDOWS)
#   include "util/xparsegeometry_repl.h"
#endif

/*
 * Perform initialization. This usually happens on the program startup
 * and restarting after glutMainLoop termination...
 */
void FGAPIENTRY glutInit( int* pargc, char** argv )
{
    char* displayName = NULL;
    char* geometry = NULL;
    if( fgState.Initialised )
        fgError( "illegal glutInit() reinitialization attempt" );

    if (pargc && *pargc && argv && *argv && **argv)
    {
        fgState.ProgramName = strdup (*argv);

        if( !fgState.ProgramName )
            fgError ("Could not allocate space for the program's name.");
    }

    fgCreateStructure( );

    fghParseCommandLineArguments ( pargc, argv, &displayName, &geometry );

    /*
     * Have the display created now. If there wasn't a "-display"
     * in the program arguments, we will use the DISPLAY environment
     * variable for opening the X display (see code above):
     */
    fgPlatformInitialize( displayName );

    /*
     * Geometry parsing deferred until here because we may need the screen
     * size.
     */

    if ( geometry )
    {
        unsigned int parsedWidth, parsedHeight;
        int mask = XParseGeometry( geometry,
                                   &fgState.Position.X, &fgState.Position.Y,
                                   &parsedWidth, &parsedHeight );
        /* TODO: Check for overflow? */
        fgState.Size.X = parsedWidth;
        fgState.Size.Y = parsedHeight;

        if( (mask & (WidthValue|HeightValue)) == (WidthValue|HeightValue) )
            fgState.Size.Use = GL_TRUE;

        if( ( mask & XNegative ) && !fgState.AllowNegativeWindowPosition )
            fgState.Position.X += fgDisplay.ScreenWidth - fgState.Size.X;

        if( ( mask & YNegative ) && !fgState.AllowNegativeWindowPosition )
            fgState.Position.Y += fgDisplay.ScreenHeight - fgState.Size.Y;

        if( (mask & (XValue|YValue)) == (XValue|YValue) )
            fgState.Position.Use = GL_TRUE;
    }
}

/* Undoes all the "glutInit" stuff */
void FGAPIENTRY glutExit ( void )
{
  fgDeinitialize ();
}

/* Sets the default initial window position for new windows */
void FGAPIENTRY glutInitWindowPosition( int x, int y )
{
    fgState.Position.X = x;
    fgState.Position.Y = y;

    if( ( ( x >= 0 ) && ( y >= 0 ) ) || fgState.AllowNegativeWindowPosition )
        fgState.Position.Use = GL_TRUE;
    else
        fgState.Position.Use = GL_FALSE;
}

/* Sets the default initial window size for new windows */
void FGAPIENTRY glutInitWindowSize( int width, int height )
{
    fgState.Size.X = width;
    fgState.Size.Y = height;

    if( ( width > 0 ) && ( height > 0 ) )
        fgState.Size.Use = GL_TRUE;
    else
        fgState.Size.Use = GL_FALSE;
}

/* Sets the default display mode for all new windows */
void FGAPIENTRY glutInitDisplayMode( unsigned int displayMode )
{
    /* We will make use of this value when creating a new OpenGL context... */
    fgState.DisplayMode = displayMode;
}


/* -- INIT DISPLAY STRING PARSING ------------------------------------------ */

static char* Tokens[] =
{
    "alpha", "acca", "acc", "blue", "buffer", "conformant", "depth", "double",
    "green", "index", "num", "red", "rgba", "rgb", "luminance", "stencil",
    "single", "stereo", "samples", "slow", "win32pdf", "win32pfd", "xvisual",
    "xstaticgray", "xgrayscale", "xstaticcolor", "xpseudocolor",
    "xtruecolor", "xdirectcolor",
    "xstaticgrey", "xgreyscale", "xstaticcolour", "xpseudocolour",
    "xtruecolour", "xdirectcolour", "borderless", "aux", "auxbufs"
};
#define NUM_TOKENS             (sizeof(Tokens) / sizeof(*Tokens))

void FGAPIENTRY glutInitDisplayString( const char* displayMode )
{
    FGCriterion criteria;
    unsigned int glut_state_flag = 0;
    char *token;
    size_t len = strlen( displayMode );
    char *buffer = (char *)malloc( (len+1) * sizeof(char) );

    /* Initialize display string criteria with defaults */
    memset( &fgState.DisplayStrCriteria, 0, sizeof(FGDisplayStringCriteria) );
    fgState.DisplayStrCriteria.haveDisplayString = GL_TRUE;

    /* Unpack options from the display string delimited by blanks or tabs */
    memcpy( buffer, displayMode, len );
    buffer[len] = '\0';

    token = strtok( buffer, " \t" );

    while ( token )
    {
        int i;
        size_t cleanlength = strcspn( token, "=<>~!" );

        for ( i = 0; i < NUM_TOKENS; i++ )
        {
            if ( strncmp( token, Tokens[i], cleanlength ) == 0 ) break;
        }

        switch ( i )
        {
        case 0:  /* "alpha":  Alpha color buffer precision in bits. Default >=1 */
            glut_state_flag |= GLUT_ALPHA;
            criteria = parseCriteria( token );
            fgState.DisplayStrCriteria.alpha = (criteria.comparison != FG_INVALID) ? criteria : (FGCriterion){FG_GTE, 1};
            break;

        case 1:  /* "acca":  Red, green, blue, and alpha accumulation buffer precision. Default >=1 */
            glut_state_flag |= GLUT_ACCUM;
            criteria = parseCriteria( token );
            if (criteria.comparison == FG_INVALID) criteria = (FGCriterion){FG_GTE, 1};
            fgState.DisplayStrCriteria.accumRed = criteria;
            fgState.DisplayStrCriteria.accumGreen = criteria;
            fgState.DisplayStrCriteria.accumBlue = criteria;
            fgState.DisplayStrCriteria.accumAlpha = criteria;
            break;

        case 2:  /* "acc":  Red, green, blue accumulation buffer with zero alpha. Default >=1 for RGB, ~0 for alpha */
            glut_state_flag |= GLUT_ACCUM;
            criteria = parseCriteria( token );
            if (criteria.comparison == FG_INVALID) criteria = (FGCriterion){FG_GTE, 1};
            fgState.DisplayStrCriteria.accumRed = criteria;
            fgState.DisplayStrCriteria.accumGreen = criteria;
            fgState.DisplayStrCriteria.accumBlue = criteria;
            fgState.DisplayStrCriteria.accumAlpha = (FGCriterion){FG_MIN, 0};
            break;

        case 3:  /* "blue":  Blue color buffer precision in bits. Default >=1 */
            criteria = parseCriteria( token );
            fgState.DisplayStrCriteria.blue = (criteria.comparison != FG_INVALID) ? criteria : (FGCriterion){FG_GTE, 1};
            break;

        case 4:  /* "buffer":  Number of bits in the color index color buffer. Default >=1 */
            criteria = parseCriteria( token );
            fgState.DisplayStrCriteria.buffer = (criteria.comparison != FG_INVALID) ? criteria : (FGCriterion){FG_GTE, 1};
            break;

        case 5:  /* "conformant":  Boolean indicating conformance. Default =1 */
            /* Not currently stored in criteria */
            break;

        case 6:  /* "depth":  Depth buffer precision in bits. Default >=12 */
            glut_state_flag |= GLUT_DEPTH;
            criteria = parseCriteria( token );
            fgState.DisplayStrCriteria.depth = (criteria.comparison != FG_INVALID) ? criteria : (FGCriterion){FG_GTE, 12};
            break;

        case 7:  /* "double":  Boolean indicating double buffering. Default =1 */
            glut_state_flag |= GLUT_DOUBLE;
            break;

        case 8:  /* "green":  Green color buffer precision in bits. Default >=1 */
            criteria = parseCriteria( token );
            fgState.DisplayStrCriteria.green = (criteria.comparison != FG_INVALID) ? criteria : (FGCriterion){FG_GTE, 1};
            break;

        case 9:  /* "index":  Boolean if color model is color index. Default >=1 */
            glut_state_flag |= GLUT_INDEX;
            criteria = parseCriteria( token );
            fgState.DisplayStrCriteria.buffer = (criteria.comparison != FG_INVALID) ? criteria : (FGCriterion){FG_GTE, 1};
            break;

        case 10:  /* "num":  Nth frame buffer configuration matching description */
            criteria = parseCriteria( token );
            if (criteria.comparison == FG_EQ && criteria.value >= 0) {
                fgState.DisplayStrCriteria.num = criteria.value;
            }
            break;

        case 11:  /* "red":  Red color buffer precision in bits. Default >=1 */
            criteria = parseCriteria( token );
            fgState.DisplayStrCriteria.red = (criteria.comparison != FG_INVALID) ? criteria : (FGCriterion){FG_GTE, 1};
            break;

        case 12:  /* "rgba":  RGBA color buffer precision. Default >=1 for all channels */
            glut_state_flag |= GLUT_RGBA;
            criteria = parseCriteria( token );
            if (criteria.comparison == FG_INVALID) criteria = (FGCriterion){FG_GTE, 1};
            fgState.DisplayStrCriteria.red = criteria;
            fgState.DisplayStrCriteria.green = criteria;
            fgState.DisplayStrCriteria.blue = criteria;
            fgState.DisplayStrCriteria.alpha = criteria;
            break;

        case 13:  /* "rgb":  RGB color buffer with zero alpha. Default >=1 for RGB, ~0 for alpha */
            glut_state_flag |= GLUT_RGB;
            criteria = parseCriteria( token );
            if (criteria.comparison == FG_INVALID) criteria = (FGCriterion){FG_GTE, 1};
            fgState.DisplayStrCriteria.red = criteria;
            fgState.DisplayStrCriteria.green = criteria;
            fgState.DisplayStrCriteria.blue = criteria;
            fgState.DisplayStrCriteria.alpha = (FGCriterion){FG_MIN, 0};
            break;

        case 14:  /* "luminance":  Red channel for luminance, zero green/blue. Default >=1 for red, =0 for green/blue */
            glut_state_flag |= GLUT_LUMINANCE;
            criteria = parseCriteria( token );
            fgState.DisplayStrCriteria.red = (criteria.comparison != FG_INVALID) ? criteria : (FGCriterion){FG_GTE, 1};
            fgState.DisplayStrCriteria.green = (FGCriterion){FG_EQ, 0};
            fgState.DisplayStrCriteria.blue = (FGCriterion){FG_EQ, 0};
            break;

        case 15:  /* "stencil":  Stencil buffer bits. No default specified in man page */
            glut_state_flag |= GLUT_STENCIL;
            criteria = parseCriteria( token );
            if (criteria.comparison != FG_INVALID) {
                fgState.DisplayStrCriteria.stencil = criteria;
            }
            break;

        case 16:  /* "single":  Boolean single buffered. Double buffer =1 */
            glut_state_flag |= GLUT_SINGLE;
            break;

        case 17:  /* "stereo":  Boolean stereo. Default =1 */
            glut_state_flag |= GLUT_STEREO;
            break;

        case 18:  /* "samples":  Multisample count. Default <=4 */
            glut_state_flag |= GLUT_MULTISAMPLE;
            criteria = parseCriteria( token );
            if (criteria.comparison != FG_INVALID) {
                fgState.DisplayStrCriteria.samples = criteria;
                if (criteria.comparison == FG_EQ || criteria.comparison == FG_NONE) {
                    fgState.SampleNumber = (criteria.comparison == FG_NONE) ? 4 : criteria.value;
                }
            } else {
                fgState.DisplayStrCriteria.samples = (FGCriterion){FG_LTE, 4};
                fgState.SampleNumber = 4;
            }
            break;

        case 19:  /* "slow":  Boolean slow configuration. Default >=0 */
            /* Not currently stored in criteria */
            break;

        case 20:  /* "win32pdf": incorrect spelling */
        case 21:  /* "win32pfd":  Win32 Pixel Format Descriptor by number */
            /* Platform-specific, not stored in generic criteria */
            break;

        case 22:  /* "xvisual":  X visual ID by number */
            /* Platform-specific, not stored in generic criteria */
            break;

        case 23:  /* "xstaticgray": */
        case 29:  /* "xstaticgrey":  StaticGray X visual */
        case 24:  /* "xgrayscale": */
        case 30:  /* "xgreyscale":  GrayScale X visual */
        case 25:  /* "xstaticcolor": */
        case 31:  /* "xstaticcolour":  StaticColor X visual */
        case 26:  /* "xpseudocolor": */
        case 32:  /* "xpseudocolour":  PseudoColor X visual */
        case 27:  /* "xtruecolor": */
        case 33:  /* "xtruecolour":  TrueColor X visual */
        case 28:  /* "xdirectcolor": */
        case 34:  /* "xdirectcolour":  DirectColor X visual */
            /* Platform-specific X11 visuals, not stored in generic criteria */
            break;

        case 35:  /* "borderless":  Windows without borders */
            glut_state_flag |= GLUT_BORDERLESS;
            break;

        case 36:  /* "aux" */
        case 37:  /* "auxbufs":  Auxiliary buffers count. No default unless specified */
            glut_state_flag |= GLUT_AUX;
            criteria = parseCriteria( token );
            if (criteria.comparison != FG_INVALID) {
                fgState.DisplayStrCriteria.auxBuffers = criteria;
                if (criteria.comparison == FG_EQ || criteria.comparison == FG_NONE) {
                    fgState.AuxiliaryBufferNumber = (criteria.comparison == FG_NONE) ? 1 : criteria.value;
                }
            } else {
                fgState.DisplayStrCriteria.auxBuffers = (FGCriterion){FG_NONE, 1};
                fgState.AuxiliaryBufferNumber = 1;
            }
            break;

        case 38:  /* Unrecognized */
            fgWarning( "Display string token not recognized: %s", token );
            break;
        }

        token = strtok( NULL, " \t" );
    }

    free( buffer );

    /* Set the DisplayMode for compatibility */
    fgState.DisplayMode = glut_state_flag;
}

/* -- SETTING OPENGL 3.0 CONTEXT CREATION PARAMETERS ---------------------- */

void FGAPIENTRY glutInitContextVersion( int majorVersion, int minorVersion )
{
    /* We will make use of these value when creating a new OpenGL context... */
    fgState.MajorVersion = majorVersion;
    fgState.MinorVersion = minorVersion;
}


void FGAPIENTRY glutInitContextFlags( int flags )
{
    /* We will make use of this value when creating a new OpenGL context... */
    fgState.ContextFlags = flags;
}

void FGAPIENTRY glutInitContextProfile( int profile )
{
    /* We will make use of this value when creating a new OpenGL context... */
    fgState.ContextProfile = profile;
}

/* -------------- User Defined Error/Warning Handler Support -------------- */

/*
 * Sets the user error handler (note the use of va_list for the args to the fmt)
 */
void FGAPIENTRY glutInitErrorFuncUcall( FGErrorUC callback, FGCBUserData userData )
{
    /* This allows user programs to handle freeglut errors */
    fgState.ErrorFunc = callback;
    fgState.ErrorFuncData = userData;
}

static void fghInitErrorFuncCallback( const char *fmt, va_list ap, FGCBUserData userData )
{
    FGError* callback = (FGError*)&userData;
    (*callback)( fmt, ap );
}

void FGAPIENTRY glutInitErrorFunc( FGError callback )
{
    if (callback)
    {
        FGError* reference = &callback;
        glutInitErrorFuncUcall( fghInitErrorFuncCallback, *((FGCBUserData*)reference) );
    }
    else
    {
        glutInitErrorFuncUcall( NULL, NULL );
    }
}

/*
 * Sets the user warning handler (note the use of va_list for the args to the fmt)
 */
void FGAPIENTRY glutInitWarningFuncUcall( FGWarningUC callback, FGCBUserData userData )
{
    /* This allows user programs to handle freeglut warnings */
    fgState.WarningFunc = callback;
    fgState.WarningFuncData = userData;
}

static void fghInitWarningFuncCallback( const char *fmt, va_list ap, FGCBUserData userData )
{
    FGWarning* callback = (FGWarning*)&userData;
    (*callback)( fmt, ap );
}

void FGAPIENTRY glutInitWarningFunc( FGWarning callback )
{
    if (callback)
    {
        FGWarning* reference = &callback;
        glutInitWarningFuncUcall( fghInitWarningFuncCallback, *((FGCBUserData*)reference) );
    }
    else
    {
        glutInitWarningFuncUcall( NULL, NULL );
    }
}

/* -- DISPLAY STRING TESTING ------------------------------------------------ */

#ifdef FREEGLUT_TEST_DISPLAY_STRING

#include <stdio.h>

static const char* comparisonName(FGCriterionComparison comp)
{
    switch(comp) {
        case FG_NONE: return "NONE";
        case FG_EQ:   return "=";
        case FG_NEQ:  return "!=";
        case FG_LT:   return "<";
        case FG_GT:   return ">";
        case FG_LTE:  return "<=";
        case FG_GTE:  return ">=";
        case FG_MIN:  return "~";
        default:      return "INVALID";
    }
}

static void printCriterion(const char* name, FGCriterion crit)
{
    if (crit.comparison != FG_NONE) {
        printf("  %-12s: %s %d\n", name, comparisonName(crit.comparison), crit.value);
    }
}

static void dumpDisplayStringCriteria(const char* testName, const char* displayString)
{
    FGDisplayStringCriteria *c;

    printf("\n=== Test: %s ===\n", testName);
    printf("Input: \"%s\"\n", displayString);

    glutInitDisplayString(displayString);
    c = &fgState.DisplayStrCriteria;

    printf("Parsed criteria:\n");
    printCriterion("alpha", c->alpha);
    printCriterion("red", c->red);
    printCriterion("green", c->green);
    printCriterion("blue", c->blue);
    printCriterion("depth", c->depth);
    printCriterion("stencil", c->stencil);
    printCriterion("accumRed", c->accumRed);
    printCriterion("accumGreen", c->accumGreen);
    printCriterion("accumBlue", c->accumBlue);
    printCriterion("accumAlpha", c->accumAlpha);
    printCriterion("samples", c->samples);
    printCriterion("auxBuffers", c->auxBuffers);
    printCriterion("buffer", c->buffer);

    if (c->num != 0) {
        printf("  num         : %d\n", c->num);
    }

    printf("DisplayMode flags: 0x%08x\n", fgState.DisplayMode);
}

int main(int argc, char** argv)
{
    /* Initialize minimal state */
    fgState.Initialised = GL_TRUE;

    printf("Testing glutInitDisplayString comparator parsing\n");
    printf("=================================================\n");

    /* Test 1: Equal comparator */
    dumpDisplayStringCriteria("Equal comparator",
                              "depth=16 samples=4");

    /* Test 2: Greater-than-or-equal comparator */
    dumpDisplayStringCriteria("GTE comparator",
                              "red>=8 green>=8 blue>=8 depth>=24");

    /* Test 3: MIN (~) comparator */
    dumpDisplayStringCriteria("MIN comparator",
                              "stencil~8 auxbufs~2");

    /* Test 4: Less-than comparator */
    dumpDisplayStringCriteria("Less-than comparator",
                              "samples<8");

    /* Test 5: Greater-than comparator */
    dumpDisplayStringCriteria("Greater-than comparator",
                              "depth>16");

    /* Test 6: Less-than-or-equal comparator */
    dumpDisplayStringCriteria("LTE comparator",
                              "samples<=4");

    /* Test 7: Not-equal comparator */
    dumpDisplayStringCriteria("Not-equal comparator",
                              "depth!=16");

    /* Test 8: Complex example from man page */
    dumpDisplayStringCriteria("Man page example",
                              "stencil~2 rgb double depth>=16 samples");

    /* Test 9: Hex and octal values */
    dumpDisplayStringCriteria("Hex/octal values",
                              "depth=0x10 stencil=010");

    /* Test 10: Default values without comparators */
    dumpDisplayStringCriteria("Defaults",
                              "alpha red green blue depth");

    /* Test 11: rgb vs rgba */
    dumpDisplayStringCriteria("RGB (no alpha)",
                              "rgb");

    dumpDisplayStringCriteria("RGBA (with alpha)",
                              "rgba");

    /* Test 12: Accumulation buffers */
    dumpDisplayStringCriteria("Accumulation (acc)",
                              "acc");

    dumpDisplayStringCriteria("Accumulation (acca)",
                              "acca");

    printf("\n=== All tests complete ===\n");
    return 0;
}

#endif /* FREEGLUT_TEST_DISPLAY_STRING */
