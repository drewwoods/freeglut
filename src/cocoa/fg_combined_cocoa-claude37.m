
// From src/cocoa/fg_init_cocoa.m
// #define DEBUG

#ifdef DEBUG
#define DGB( ... ) printf( __VA_ARGS__ )
#endif

// #define FREEGLUT_BUILDING_LIB

///////////////////////////////////////////
// INTERNAL
///////////////////////////////////////////

#ifndef FREEGLUT_INTERNAL_COCOA_H
#define FREEGLUT_INTERNAL_COCOA_H

#include <unistd.h>

#define TODO_IMPL fgWarning( "%s not implemented yet in Cocoa", __func__ )

/* Menu font and color definitions */
#define FREEGLUT_MENU_FONT GLUT_BITMAP_HELVETICA_18

#define FREEGLUT_MENU_PEN_FORE_COLORS  { 0.0f, 0.0f, 0.0f, 1.0f }
#define FREEGLUT_MENU_PEN_BACK_COLORS  { 0.70f, 0.70f, 0.70f, 1.0f }
#define FREEGLUT_MENU_PEN_HFORE_COLORS { 0.0f, 0.0f, 0.0f, 1.0f }
#define FREEGLUT_MENU_PEN_HBACK_COLORS { 1.0f, 1.0f, 1.0f, 1.0f }

/* Platform-specific display structure */
struct CocoaPlatformDisplay {
    uint32_t CocoaDisplay; /* The display ID */
};

/* Platform-specific window context */
struct CocoaPlatformContext {
    void *CocoaContext; /* OpenGL context */ // NSOpenGLContext*
    void *PixelFormat; /* Pixel format */    // NSOpenGLPixelFormat*
};

/* Platform window state info */
struct CocoaWindowState {
    int OldWidth;  /* Window width before resize */
    int OldHeight; /* Window height before resize */
};

#define _JS_MAX_AXES 16
struct CocoaPlatformJoystick {};

/*
 * Make "freeglut" window handle and context types so that we don't need so
 * much conditionally-compiled code later in the library.
 */
typedef void                        *SFG_WindowHandleType;   // NSWindow*
typedef void                        *SFG_WindowContextType;  // NSOpenGLContext*
typedef void                        *SFG_WindowColormapType; // CGColorSpaceRef
typedef struct CocoaWindowState      SFG_PlatformWindowState;
typedef struct CocoaPlatformDisplay  SFG_PlatformDisplay;
typedef struct CocoaPlatformContext  SFG_PlatformContext;
typedef struct CocoaPlatformJoystick SFG_PlatformJoystick;

#endif /* FREEGLUT_INTERNAL_COCOA_H */

///////////////////////////////////////////
// INCLUDES
///////////////////////////////////////////

#include <GL/freeglut.h>
#include "../fg_internal.h"
#include "../fg_init.h" // only in init_cocoa


///////////////////////////////////////////
// CMAP
///////////////////////////////////////////

void fgPlatformSetColor( int idx, float r, float g, float b )
{
    TODO_IMPL;
}

float fgPlatformGetColor( int idx, int comp )
{
    TODO_IMPL;

    return 0.0f;
}

void fgPlatformCopyColormap( int win )
{
    TODO_IMPL;
}

///////////////////////////////////////////
// CURSOR
///////////////////////////////////////////

void fgPlatformSetCursor( SFG_Window *window, int cursorID )
{
    TODO_IMPL;
}

void fgPlatformWarpPointer( int x, int y )
{
    TODO_IMPL;
}

void fghPlatformGetCursorPos( const SFG_Window *window, GLboolean client, SFG_XYUse *mouse_pos )
{
    TODO_IMPL;
}

///////////////////////////////////////////
// DISPLAY
///////////////////////////////////////////

void fgPlatformGlutSwapBuffers( SFG_PlatformDisplay *pDisplayPtr, SFG_Window *CurrentWindow )
{
    TODO_IMPL;
}

///////////////////////////////////////////
// EXT
///////////////////////////////////////////

GLUTproc fgPlatformGetGLUTProcAddress( const char* procName )
{
    TODO_IMPL;

    return NULL;
}

SFG_Proc fgPlatformGetProcAddress( const char* procName )
{
    TODO_IMPL;

    return NULL;
}

///////////////////////////////////////////
// GAMEMODE
///////////////////////////////////////////

void fgPlatformRememberState( void )
{
    TODO_IMPL;
}

void fgPlatformRestoreState( void )
{
    TODO_IMPL;
}

GLboolean fgPlatformChangeDisplayMode( GLboolean haveToTest )
{
    TODO_IMPL;
    return GL_FALSE;
}

void fgPlatformEnterGameMode( void )
{
    TODO_IMPL;
}

void fgPlatformLeaveGameMode( void )
{
    TODO_IMPL;
}

/*
 * Private function to get the virtual maximum screen extent
 */
GLvoid fgPlatformGetGameModeVMaxExtent( SFG_Window *window, int *x, int *y )
{
    TODO_IMPL;
}

///////////////////////////////////////////
// GLUT Definitions
///////////////////////////////////////////

/*
 * The following are font symbols that fg_font.c expects to be present.  I'm guess this
 * allows different platforms to provide different fonts.  Using the included font symbols
 * glutStrokeRoman, glutBitmap9By15, etc. will use the default fonts.
 */

extern SFG_Font       fgFontFixed8x13;
extern SFG_Font       fgFontFixed9x15;
extern SFG_Font       fgFontHelvetica10;
extern SFG_Font       fgFontHelvetica12;
extern SFG_Font       fgFontHelvetica18;
extern SFG_Font       fgFontTimesRoman10;
extern SFG_Font       fgFontTimesRoman24;
extern SFG_StrokeFont fgStrokeRoman;
extern SFG_StrokeFont fgStrokeMonoRoman;

void *glutStrokeRoman     = &fgFontFixed8x13;
void *glutStrokeMonoRoman = &glutStrokeMonoRoman;

void *glutBitmap9By15        = &glutBitmap9By15;
void *glutBitmap8By13        = &glutBitmap8By13;
void *glutBitmapTimesRoman10 = &glutBitmapTimesRoman10;
void *glutBitmapTimesRoman24 = &glutBitmapTimesRoman24;
void *glutBitmapHelvetica10  = &glutBitmapHelvetica10;
void *glutBitmapHelvetica12  = &glutBitmapHelvetica12;
void *glutBitmapHelvetica18  = &glutBitmapHelvetica18;

/*
 * Other functions that fg_input.c expects to be present.  Not sure why they are not prefixed with
 * fgPlatform.
 */

typedef struct _serialport SERIALPORT;
SERIALPORT                *fg_serial_open( const char *device )
{
    return NULL;
}
void fg_serial_close( SERIALPORT *port )
{
}
int fg_serial_getchar( SERIALPORT *port )
{
    return EOF;
}
int fg_serial_putchar( SERIALPORT *port, unsigned char ch )
{
    return 0;
}
void fg_serial_flush( SERIALPORT *port )
{
}

///////////////////////////////////////////
// INIT
///////////////////////////////////////////

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#import <CoreVideo/CVDisplayLink.h>
#include <sys/time.h>

/*
 * Initialize the Cocoa platform specific stuff.
 */
void fgPlatformInitialize( const char *displayName )
{
    // Cocoa initialization
    // Don't need to call NSApplicationLoad() if using glutInit() properly

    // Get Main Screen information
    NSScreen *mainScreen = [NSScreen mainScreen];
    if ( !mainScreen ) {
        fgError( "Failed to get main screen" );
    }

    // Get screen dimensions
    NSRect screenFrame = [mainScreen frame];

    // Initialize the fgDisplay structure
    fgDisplay.pDisplay.CocoaDisplay = kCGDirectMainDisplay; // Use main display

    // Store screen width and height
    fgDisplay.ScreenWidth  = (int)screenFrame.size.width;
    fgDisplay.ScreenHeight = (int)screenFrame.size.height;

    // Get screen physical size (mm)
    // Note: macOS doesn't provide exact measurements in mm, so this is estimated
    CGSize physicalSize      = CGDisplayScreenSize( kCGDirectMainDisplay );
    fgDisplay.ScreenWidthMM  = (int)physicalSize.width;
    fgDisplay.ScreenHeightMM = (int)physicalSize.height;

    // Set the initial time
    fgState.Time = fgSystemTime( );

    // Mark as initialized
    fgState.Initialised = GL_TRUE;

    // Register cleanup at exit
    atexit( fgDeinitialize );

    // InputDevice uses GlutTimerFunc(), so fgState.Initialised must be TRUE
    fgInitialiseInputDevices( );
}

/*
 * Deinitialize all the input devices
 */
void fgPlatformDeinitialiseInputDevices( void )
{
    // Close all input devices
    fghCloseInputDevices( );

    // Mark joysticks and input devices as not initialized
    fgState.JoysticksInitialised = GL_FALSE;
    fgState.InputDevsInitialised = GL_FALSE;
}

/*
 * Clean up the display connection
 */
void fgPlatformCloseDisplay( void )
{
    // Nothing specific to clean up for Cocoa
    // In Cocoa we don't have a display connection like X11
}

/*
 * Destroy the OpenGL context
 */
void fgPlatformDestroyContext( SFG_PlatformDisplay pDisplay, SFG_WindowContextType MContext )
{
    NSOpenGLContext *context = (NSOpenGLContext *)MContext;

    // Check if the context is valid
    if ( context ) {
        // Properly release the NSOpenGLContext
        [context clearDrawable];
        [context release];
    }
}

/*
 * Helper function to set up CVDisplayLink for a window
 * This would be called during window creation
 */
static CVReturn displayLinkCallback( CVDisplayLinkRef displayLink,
    const CVTimeStamp                                *now,
    const CVTimeStamp                                *outputTime,
    CVOptionFlags                                     flagsIn,
    CVOptionFlags                                    *flagsOut,
    void                                             *userData )
{
    // Cast user data to window pointer
    SFG_Window *window = (SFG_Window *)userData;

    // Schedule display function to run on main thread
    // We use a perform selector here because OpenGL calls must be on the main thread
    [NSApp performSelectorOnMainThread:@selector( displayFreeGLUTWindow: )
                            withObject:[NSValue valueWithPointer:window]
                         waitUntilDone:NO];

    return kCVReturnSuccess;
}

/*
 * Set up the display link for a window
 * This would be called during window creation
 */
void fgPlatformSetupDisplayLink( SFG_Window *window, CVDisplayLinkRef *displayLinkPtr )
{
    CVDisplayLinkRef displayLink;

    // Create a display link capable of being used with the current active display
    CVDisplayLinkCreateWithActiveCGDisplays( &displayLink );

    // Set the renderer output callback function
    CVDisplayLinkSetOutputCallback( displayLink, &displayLinkCallback, window );

    // Associate the display link with the OpenGL context
    NSOpenGLContext  *context    = (NSOpenGLContext *)window->Window.Context;
    CGLContextObj     cglContext = ( CGLContextObj )[context CGLContextObj];
    CGLPixelFormatObj cglPixelFormat =
        ( CGLPixelFormatObj )[[(NSOpenGLContext *)window->Window.Context pixelFormat] CGLPixelFormatObj];
    CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext( displayLink, cglContext, cglPixelFormat );

    // Store the display link
    *displayLinkPtr = displayLink;

    // Start the display link
    CVDisplayLinkStart( displayLink );
}

/*
 * Clean up display link resources
 * This would be called during window destruction
 */
void fgPlatformCleanupDisplayLink( CVDisplayLinkRef displayLink )
{
    if ( displayLink ) {
        CVDisplayLinkStop( displayLink );
        CVDisplayLinkRelease( displayLink );
    }
}

///////////////////////////////////////////
// INPUT DEVICES
///////////////////////////////////////////

/*
 * Try initializing the input device(s)
 */
void fgPlatformRegisterDialDevice( const char *dial_device )
{
    TODO_IMPL;
}

///////////////////////////////////////////
// JOYSTICK
///////////////////////////////////////////

void fgPlatformJoystickRawRead( SFG_Joystick *joy, int *buttons, float *axes )
{
    TODO_IMPL;
}

void fgPlatformJoystickOpen( SFG_Joystick *joy )
{
    TODO_IMPL;
}

void fgPlatformJoystickInit( SFG_Joystick *fgJoystick[], int ident )
{
    TODO_IMPL;
}

void fgPlatformJoystickClose( int ident )
{
    TODO_IMPL;
}

///////////////////////////////////////////
// MAIN
///////////////////////////////////////////


/*
 * System time in milliseconds, equivalent to X11 implementation
 */
fg_time_t fgPlatformSystemTime( void )
{
    struct timeval now;
    gettimeofday( &now, NULL );
    return now.tv_usec / 1000 + now.tv_sec * 1000;
}

/*
 * Does the magic required to relinquish the CPU until something interesting
 * happens.
 */

void fgPlatformSleepForEvents( fg_time_t msec )
{
    TODO_IMPL;
}

/*
 * Returns GLUT modifier mask for the state field of an X11 event.
 */
int fgPlatformGetModifiers( int state )
{
    TODO_IMPL;
    return 0;
}

void fgPlatformProcessSingleEvent( void )
{
    TODO_IMPL;
}

void fgPlatformMainLoopPreliminaryWork( void )
{
    TODO_IMPL;
}

/* deal with work list items */
void fgPlatformInitWork( SFG_Window *window )
{
    TODO_IMPL;
}

void fgPlatformPosResZordWork( SFG_Window *window, unsigned int workMask )
{
    TODO_IMPL;
}

void fgPlatformVisibilityWork( SFG_Window *window )
{
    TODO_IMPL;
}

///////////////////////////////////////////
// SPACEBALL
///////////////////////////////////////////

void fgPlatformInitializeSpaceball( void )
{
    TODO_IMPL;
}

void fgPlatformSpaceballClose( void )
{
    TODO_IMPL;
}

int fgPlatformHasSpaceball( void )
{
    TODO_IMPL;
    return 0;
}

int fgPlatformSpaceballNumButtons( void )
{
    TODO_IMPL;
    return 0;
}

void fgPlatformSpaceballSetWindow( SFG_Window *window )
{
    TODO_IMPL;
}

///////////////////////////////////////////
// STRUCTURE
///////////////////////////////////////////

void fgPlatformCreateWindow(SFG_Window* window)
{
    TODO_IMPL;
}

///////////////////////////////////////////
// STATE
///////////////////////////////////////////

int fgPlatformGlutDeviceGet( GLenum eWhat )
{
    TODO_IMPL;
    return 0;
}

int fgPlatformGlutGet( GLenum eWhat )
{
    TODO_IMPL;
    return 0;
}

int* fgPlatformGlutGetModeValues( GLenum eWhat, int* size )
{
    TODO_IMPL;
    return NULL;
}

///////////////////////////////////////////
// WINDOW
///////////////////////////////////////////

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
    TODO_IMPL;
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
