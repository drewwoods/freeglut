
// From src/cocoa/fg_init_cocoa.m
// #define DEBUG

#if 1
#ifdef DEBUG
#define DGB( ... ) printf( __VA_ARGS__ )
#endif

// #define FREEGLUT_BUILDING_LIB

///////////////////////////////////////////
// INTERNAL
///////////////////////////////////////////

///////////////////////////////////////////
// INTERNAL
///////////////////////////////////////////

#ifndef FREEGLUT_INTERNAL_COCOA_H
#define FREEGLUT_INTERNAL_COCOA_H

#include <unistd.h>
#include <CoreVideo/CVDisplayLink.h>
#include <Cocoa/Cocoa.h>
#include <OpenGL/gl.h>

#define TODO_IMPL fgWarning( "%s not implemented yet in Cocoa", __func__ )

/* Menu font and color definitions */
#define FREEGLUT_MENU_FONT GLUT_BITMAP_HELVETICA_18

#define FREEGLUT_MENU_PEN_FORE_COLORS  { 0.0f, 0.0f, 0.0f, 1.0f }
#define FREEGLUT_MENU_PEN_BACK_COLORS  { 0.70f, 0.70f, 0.70f, 1.0f }
#define FREEGLUT_MENU_PEN_HFORE_COLORS { 0.0f, 0.0f, 0.0f, 1.0f }
#define FREEGLUT_MENU_PEN_HBACK_COLORS { 1.0f, 1.0f, 1.0f, 1.0f }

/* Platform-specific display structure */
struct CocoaPlatformDisplay {
    uint32_t  CocoaDisplay;       /* The display ID (CGDirectDisplayID) */
    NSScreen *MainScreen;         /* The main screen reference */
    CGFloat   BackingScaleFactor; /* For Retina display support */
};

/* Platform-specific window context */
struct CocoaPlatformContext {
    void            *CocoaContext;  /* OpenGL context (NSOpenGLContext*) */
    void            *PixelFormat;   /* Pixel format (NSOpenGLPixelFormat*) */
    void            *View;          /* The OpenGL view (NSOpenGLView*) */
    CVDisplayLinkRef DisplayLink;   /* Core Video display link for smooth animation */
    NSTimeInterval   LastFrameTime; /* Timestamp of the last frame for timing */
};

/* Platform window state info */
struct CocoaWindowState {
    int    OldWidth;     /* Window width before resize */
    int    OldHeight;    /* Window height before resize */
    BOOL   InFullScreen; /* Is the window in fullscreen mode? */
    NSRect WindowedRect; /* Window rect before going fullscreen */
    int    WindowLevel;  /* The window level/z-order */
    BOOL   CursorHidden; /* Is the cursor hidden? */
};

#define _JS_MAX_AXES 16
struct CocoaPlatformJoystick {
    // Joystick implementation details would go here
    // For now, it's empty as per original code
};

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
// PROTOTYPES
///////////////////////////////////////////

void fgPlatformPositionWindow( SFG_Window *window, int x, int y );
void fgPlatformReshapeWindow( SFG_Window *window, int width, int height );
void fgPlatformFullScreenToggle( SFG_Window *window );
void fgPlatformShowWindow( SFG_Window *window );
void fgPlatformSetWindow( SFG_Window *window );
void fgPlatformProcessSingleEvent( void );
void fgPlatformSleepForEvents( fg_time_t msec );

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

/*
 * Gets the cursor position
 */
void fghPlatformGetCursorPos( const SFG_Window *window, GLboolean client, SFG_XYUse *mouse_pos )
{
    if ( !window || !mouse_pos )
        return;

    NSWindow *nsWindow = (NSWindow *)window->Window.Handle;
    if ( !nsWindow )
        return;

    // Get the current mouse position in screen coordinates
    NSPoint screenPos = [NSEvent mouseLocation];

    // Convert to window coordinates
    NSPoint windowPos = [nsWindow convertPointFromScreen:screenPos];

    if ( client ) {
        // For client coordinates, convert to content view coordinates
        NSView *contentView = [nsWindow contentView];
        NSPoint viewPos     = [contentView convertPoint:windowPos fromView:nil];

        // Convert from Cocoa's bottom-left to GLUT's top-left origin
        mouse_pos->X = viewPos.x;
        mouse_pos->Y = [contentView bounds].size.height - viewPos.y;
    }
    else {
        // For window coordinates
        mouse_pos->X = windowPos.x;
        mouse_pos->Y = [nsWindow frame].size.height - windowPos.y;
    }

    mouse_pos->Use = GL_TRUE;
}

/*
 * Set the cursor for a window
 */
void fgPlatformSetCursor( SFG_Window *window, int cursorID )
{
    if ( !window )
        return;

    // Map GLUT cursor IDs to NSCursor types
    NSCursor *cursor = nil;

    switch ( cursorID ) {
    case GLUT_CURSOR_RIGHT_ARROW:
        cursor = [NSCursor arrowCursor];
        break;
    case GLUT_CURSOR_LEFT_ARROW:
        cursor = [NSCursor arrowCursor]; // No direct equivalent, use arrow
        break;
    case GLUT_CURSOR_INFO:
        cursor = [NSCursor pointingHandCursor];
        break;
    case GLUT_CURSOR_DESTROY:
        cursor = [NSCursor operationNotAllowedCursor];
        break;
    case GLUT_CURSOR_HELP:
        cursor = [NSCursor pointingHandCursor]; // No direct help cursor
        break;
    case GLUT_CURSOR_CYCLE:
        cursor = [NSCursor pointingHandCursor]; // No direct equivalent
        break;
    case GLUT_CURSOR_SPRAY:
        cursor = [NSCursor crosshairCursor]; // No direct equivalent
        break;
    case GLUT_CURSOR_WAIT:
        cursor = [NSCursor arrowCursor]; // No wait cursor in stock NSCursor
        break;
    case GLUT_CURSOR_TEXT:
        cursor = [NSCursor IBeamCursor];
        break;
    case GLUT_CURSOR_CROSSHAIR:
        cursor = [NSCursor crosshairCursor];
        break;
    case GLUT_CURSOR_UP_DOWN:
        cursor = [NSCursor resizeUpDownCursor];
        break;
    case GLUT_CURSOR_LEFT_RIGHT:
        cursor = [NSCursor resizeLeftRightCursor];
        break;
    case GLUT_CURSOR_TOP_SIDE:
        cursor = [NSCursor resizeUpCursor];
        break;
    case GLUT_CURSOR_BOTTOM_SIDE:
        cursor = [NSCursor resizeDownCursor];
        break;
    case GLUT_CURSOR_LEFT_SIDE:
        cursor = [NSCursor resizeLeftCursor];
        break;
    case GLUT_CURSOR_RIGHT_SIDE:
        cursor = [NSCursor resizeRightCursor];
        break;
    case GLUT_CURSOR_TOP_LEFT_CORNER:
        cursor = [NSCursor resizeUpLeftCursor];
        break;
    case GLUT_CURSOR_TOP_RIGHT_CORNER:
        cursor = [NSCursor resizeUpRightCursor];
        break;
    case GLUT_CURSOR_BOTTOM_RIGHT_CORNER:
        cursor = [NSCursor resizeDownRightCursor];
        break;
    case GLUT_CURSOR_BOTTOM_LEFT_CORNER:
        cursor = [NSCursor resizeDownLeftCursor];
        break;
    case GLUT_CURSOR_FULL_CROSSHAIR:
        cursor = [NSCursor crosshairCursor];
        break;
    case GLUT_CURSOR_NONE:
        // Hide the cursor
        [NSCursor hide];
        window->State.pWState.CursorHidden = YES;
        return;
    case GLUT_CURSOR_INHERIT:
        cursor = [NSCursor arrowCursor]; // Default cursor
        break;
    default:
        cursor = [NSCursor arrowCursor];
        break;
    }

    // If cursor was hidden, show it again
    if ( window->State.pWState.CursorHidden ) {
        [NSCursor unhide];
        window->State.pWState.CursorHidden = NO;
    }

    // Set the cursor
    if ( cursor ) {
        [cursor set];
    }

    window->State.Cursor = cursorID;
}

/*
 * Warp the pointer to a given position
 */
void fgPlatformWarpPointer( int x, int y )
{
    if ( !fgStructure.CurrentWindow )
        return;

    NSWindow *nsWindow = (NSWindow *)fgStructure.CurrentWindow->Window.Handle;
    if ( !nsWindow )
        return;

    // Convert from GLUT coordinates (top-left origin) to Cocoa (bottom-left)
    NSRect  frame     = [nsWindow frame];
    NSPoint windowPos = NSMakePoint( x, frame.size.height - y );

    // Convert to screen coordinates
    NSPoint screenPos = [nsWindow convertPointToScreen:windowPos];

    // Create a CGPoint for CGDisplayMoveCursorToPoint
    CGPoint point = CGPointMake( screenPos.x, screenPos.y );

    // Move the cursor
    CGDisplayMoveCursorToPoint( kCGDirectMainDisplay, point );

    // Update our internal mouse position
    fgStructure.CurrentWindow->State.MouseX = x;
    fgStructure.CurrentWindow->State.MouseY = y;
}

// NSApp category to handle display callbacks for CVDisplayLink
@interface NSApplication ( FreeGLUT )
- (void)displayFreeGLUTWindow:(NSValue *)windowPtr;
@end

@implementation NSApplication ( FreeGLUT )
- (void)displayFreeGLUTWindow:(NSValue *)windowPtr
{
    SFG_Window *window = (SFG_Window *)[windowPtr pointerValue];

    // Skip if window isn't valid anymore
    if ( !window || !window->Window.Context )
        return;

    // Make the context current
    fgSetWindow( window );

    // Call the display callback
    INVOKE_WCB( *window, Display, ( ) );

    // Swap buffers if it's double-buffered
    if ( window->Window.DoubleBuffered ) {
        NSOpenGLContext *context = (NSOpenGLContext *)window->Window.Context;
        [context flushBuffer];
    }
    else {
        glFlush( );
    }
}
@end

///////////////////////////////////////////
// DISPLAY
///////////////////////////////////////////

/*
 * Swap buffers for the current window
 */
void fgPlatformGlutSwapBuffers( SFG_PlatformDisplay *pDisplayPtr, SFG_Window *CurrentWindow )
{
    if ( !CurrentWindow )
        return;

    // Make sure the context is current
    NSOpenGLContext *context = (NSOpenGLContext *)CurrentWindow->Window.Context;
    if ( context ) {
        [context flushBuffer];
    }
}

///////////////////////////////////////////
// EXT
///////////////////////////////////////////

#include <dlfcn.h>

GLUTproc fgPlatformGetGLUTProcAddress( const char *procName )
{
    /* optimization: quick initial check */
    if ( strncmp( procName, "glut", 4 ) != 0 )
        return NULL;

#define CHECK_NAME( x )                \
    if ( strcmp( procName, #x ) == 0 ) \
        return (GLUTproc)x;
    CHECK_NAME( glutJoystickFunc );
    CHECK_NAME( glutForceJoystickFunc );
    CHECK_NAME( glutGameModeString );
    CHECK_NAME( glutEnterGameMode );
    CHECK_NAME( glutLeaveGameMode );
    CHECK_NAME( glutGameModeGet );
#undef CHECK_NAME

    return NULL;
}

SFG_Proc fgPlatformGetProcAddress( const char *procName )
{
    static void *glHandle = NULL;
    if ( glHandle == NULL ) {
        glHandle = dlopen( "/System/Library/Frameworks/OpenGL.framework/OpenGL", RTLD_LAZY | RTLD_GLOBAL );
        if ( glHandle == NULL ) {
            fgError( "Failed to dlopen OpenGL framework" );
        }
    }
    void *addr = dlsym( glHandle, procName );
    return addr;
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
 * Sleep for a specified time and process events
 */
void fgPlatformSleepForEvents( fg_time_t msec )
{
    // Let the RunLoop handle events for the specified time
    // This is similar to the X11 implementation but using Cocoa's run loop

    NSDate *limitDate;
    if ( msec < 0 ) {
        limitDate = [NSDate distantFuture];
    }
    else {
        limitDate = [NSDate dateWithTimeIntervalSinceNow:(NSTimeInterval)msec / 1000.0];
    }

    // Process events until the time expires or an event arrives
    NSEvent *event;

    do {
        event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                   untilDate:limitDate
                                      inMode:NSDefaultRunLoopMode
                                     dequeue:YES];

        if ( event ) {
            [NSApp sendEvent:event];
        }
    } while ( event );
}

/*
 * Process a single event - allows glutMainLoopStep mode
 */
void fgPlatformProcessSingleEvent( void )
{
    // Process just one event if available
    NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                        untilDate:[NSDate distantPast] // Don't wait for events
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];

    if ( event ) {
        [NSApp sendEvent:event];
    }
}

/*
 * Performs preliminary work for mainloop
 */
void fgPlatformMainLoopPreliminaryWork( void )
{
    // In Cocoa, we need to ensure NSApplication is properly set up
    [NSApplication sharedApplication];

    // We need a delegate to handle application lifecycle events
    // This would ideally be set up in the init phase, but we ensure it's done here
    if ( ![NSApp delegate] ) {
        // Create and set a delegate class (implementation would be elsewhere)
        // For now, this is a placeholder for where you'd set up the app delegate
        NSLog( @"Warning: NSApp has no delegate. Application lifecycle events will not be handled." );
    }
}

/*
 * Returns GLUT modifier mask for key modifiers
 */
int fgPlatformGetModifiers( int state )
{
    // Map Cocoa modifiers to GLUT modifiers
    // This would typically be called when processing key or mouse events

    NSUInteger modifiers     = [NSEvent modifierFlags];
    int        glutModifiers = 0;

    if ( modifiers & NSEventModifierFlagShift ) {
        glutModifiers |= GLUT_ACTIVE_SHIFT;
    }

    if ( modifiers & NSEventModifierFlagControl ) {
        glutModifiers |= GLUT_ACTIVE_CTRL;
    }

    if ( modifiers & NSEventModifierFlagOption ) {
        glutModifiers |= GLUT_ACTIVE_ALT;
    }

    return glutModifiers;
}

// NSApp category to handle display callbacks for CVDisplayLink
@interface NSApplication ( FreeGLUT )
- (void)displayFreeGLUTWindow:(NSValue *)windowPtr;
@end

@implementation NSApplication ( FreeGLUT )
- (void)displayFreeGLUTWindow:(NSValue *)windowPtr
{
    SFG_Window *window = (SFG_Window *)[windowPtr pointerValue];

    // Skip if window isn't valid anymore
    if ( !window || !window->Window.Context || !window->State.Visible )
        return;

    // Make the context current
    NSOpenGLContext *context = (NSOpenGLContext *)window->Window.Context;
    [context makeCurrentContext];

    // Call the display callback
    if ( FETCH_WCB( *window, Display ) ) {
        INVOKE_WCB( *window, Display, ( ) );
    }

    // Swap buffers if it's double-buffered
    if ( window->Window.DoubleBuffered ) {
        [context flushBuffer];
    }
    else {
        glFlush( );
    }
}
@end

/*
 * Handle window initialization work (called from fgPlatformOpenWindow)
 */
void fgPlatformInitWork( SFG_Window *window )
{
    if ( !window )
        return;

    // Update the current window's state
    window->State.WorkMask |= GLUT_DISPLAY_WORK;

    // Ensure the window is visible
    fgPlatformShowWindow( window );
    window->State.Visible = GL_TRUE;

    // Make sure the context is current
    NSOpenGLContext *context = (NSOpenGLContext *)window->Window.Context;
    if ( context ) {
        [context makeCurrentContext];

        // Ensure proper initial viewport setup
        glViewport( 0, 0, window->State.Width, window->State.Height );

        // Set initial clear color
        glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
        glClear( GL_COLOR_BUFFER_BIT );
        [context flushBuffer];
    }
}

/*
 * Handle position, resize, and Z-order work
 */
void fgPlatformPosResZordWork( SFG_Window *window, unsigned int workMask )
{
    if ( !window )
        return;

    NSWindow *nsWindow = (NSWindow *)window->Window.Handle;
    if ( !nsWindow )
        return;

    // Position work
    if ( workMask & GLUT_POSITION_WORK ) {
        fgPlatformPositionWindow( window, window->State.DesiredXpos, window->State.DesiredYpos );
    }

    // Size work
    if ( workMask & GLUT_SIZE_WORK ) {
        fgPlatformReshapeWindow( window, window->State.DesiredWidth, window->State.DesiredHeight );
    }

    // Z-order work
    if ( workMask & GLUT_ZORDER_WORK ) {
        if ( window->State.DesiredZOrder > 0 ) {
            [nsWindow orderFront:nil];
        }
        else {
            [nsWindow orderBack:nil];
        }
    }

    // Full screen work
    if ( workMask & GLUT_FULL_SCREEN_WORK ) {
        fgPlatformFullScreenToggle( window );
    }
}

/*
 * Handle visibility work
 */
void fgPlatformVisibilityWork( SFG_Window *window )
{
    if ( !window )
        return;

    NSWindow *nsWindow = (NSWindow *)window->Window.Handle;
    if ( !nsWindow )
        return;

    switch ( window->State.DesiredVisibility ) {
    case DesireHiddenState:
        [nsWindow orderOut:nil];
        window->State.Visible = GL_FALSE;
        break;

    case DesireIconicState:
        [nsWindow miniaturize:nil];
        window->State.Visible = GL_TRUE;
        break;

    case DesireNormalState:
        if ( [nsWindow isMiniaturized] ) {
            [nsWindow deminiaturize:nil];
        }
        [nsWindow makeKeyAndOrderFront:nil];
        window->State.Visible = GL_TRUE;
        break;
    }
}

/*
 * Helper function to update OpenGL context
 * Call this after window resizes or moves to ensure proper context updates
 */
void fgPlatformUpdateContext( SFG_Window *window )
{
    if ( !window )
        return;

    NSOpenGLContext *context = (NSOpenGLContext *)window->Window.Context;
    if ( context ) {
        [context update];

        // Make current and update viewport
        [context makeCurrentContext];
        glViewport( 0, 0, window->State.Width, window->State.Height );
    }
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

/*
 * Create the window structure
 */
void fgPlatformCreateWindow( SFG_Window *window )
{
    // This function mainly initializes additional window structure
    // before fgPlatformOpenWindow is called

    // Initialize the platform-specific window state
    window->State.pWState.OldWidth     = 0;
    window->State.pWState.OldHeight    = 0;
    window->State.pWState.InFullScreen = NO;
    window->State.pWState.WindowLevel  = 0;
    window->State.pWState.CursorHidden = NO;
}

///////////////////////////////////////////
// STATE
///////////////////////////////////////////

int fgPlatformGlutDeviceGet( GLenum eWhat )
{
    TODO_IMPL;
    return 0;
}

/*
 * Get platform-specific GLUT settings
 */
int fgPlatformGlutGet( GLenum eWhat )
{
    switch ( eWhat ) {
    case GLUT_WINDOW_X:
        if ( fgStructure.CurrentWindow )
            return fgStructure.CurrentWindow->State.Xpos;
        break;

    case GLUT_WINDOW_Y:
        if ( fgStructure.CurrentWindow )
            return fgStructure.CurrentWindow->State.Ypos;
        break;

    case GLUT_WINDOW_WIDTH:
        if ( fgStructure.CurrentWindow )
            return fgStructure.CurrentWindow->State.Width;
        break;

    case GLUT_WINDOW_HEIGHT:
        if ( fgStructure.CurrentWindow )
            return fgStructure.CurrentWindow->State.Height;
        break;

    case GLUT_WINDOW_BORDER_WIDTH:
        return 0; // Not directly available in Cocoa

    case GLUT_WINDOW_HEADER_HEIGHT:
        if ( fgStructure.CurrentWindow ) {
            NSWindow *nsWindow = (NSWindow *)fgStructure.CurrentWindow->Window.Handle;
            if ( nsWindow ) {
                NSRect frameRect   = [nsWindow frame];
                NSRect contentRect = [nsWindow contentRectForFrameRect:frameRect];
                return frameRect.size.height - contentRect.size.height;
            }
        }
        return 0;

    case GLUT_DISPLAY_MODE_POSSIBLE:
        return 1; // Most display modes are possible on macOS

    case GLUT_SCREEN_WIDTH:
        return fgDisplay.ScreenWidth;

    case GLUT_SCREEN_HEIGHT:
        return fgDisplay.ScreenHeight;

    case GLUT_SCREEN_WIDTH_MM:
        return fgDisplay.ScreenWidthMM;

    case GLUT_SCREEN_HEIGHT_MM:
        return fgDisplay.ScreenHeightMM;

    default:
        return -1;
    }

    return -1;
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
 * Creates and opens a Cocoa window
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
    // Prepare window position and size
    NSRect contentRect;

    if ( sizeUse ) {
        contentRect.size.width  = w;
        contentRect.size.height = h;
    }
    else {
        contentRect.size.width  = 300;
        contentRect.size.height = 300;
    }

    if ( positionUse ) {
        contentRect.origin.x = x;
        contentRect.origin.y = fgDisplay.ScreenHeight - y - contentRect.size.height;
    }
    else {
        contentRect.origin.x = 100;
        contentRect.origin.y = 100;
    }

    // Create the window style mask
    NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;

    if ( !gameMode ) {
        styleMask |= NSWindowStyleMaskResizable;
    }

    // Create the window
    NSWindow *nsWindow = [[NSWindow alloc] initWithContentRect:contentRect
                                                     styleMask:styleMask
                                                       backing:NSBackingStoreBuffered
                                                         defer:NO];

    if ( !nsWindow ) {
        fgError( "Failed to create window" );
        return;
    }

    // Set up window properties
    [nsWindow setTitle:[NSString stringWithUTF8String:( title ? title : "GLUT" )]];
    [nsWindow setReleasedWhenClosed:NO]; // We'll handle release explicitly
    [nsWindow setAcceptsMouseMovedEvents:YES];
    [nsWindow setRestorable:NO]; // Don't restore window state across app restarts

    // Adjust pixel format attributes for Core vs Legacy profile based on user request
    NSOpenGLPixelFormatAttribute pixelAttribs[20]; // More than enough room
    int                          i = 0;

    // Always use double buffer
    pixelAttribs[i++] = NSOpenGLPFADoubleBuffer;
    pixelAttribs[i++] = NSOpenGLPFADepthSize;
    pixelAttribs[i++] = 24;
    pixelAttribs[i++] = NSOpenGLPFAStencilSize;
    pixelAttribs[i++] = 8;
    pixelAttribs[i++] = NSOpenGLPFAAccelerated;

    // For legacy compatibility (allows fixed-function commands like glBegin/glEnd)
    pixelAttribs[i++] = NSOpenGLPFAOpenGLProfile;
    pixelAttribs[i++] = NSOpenGLProfileVersionLegacy;

    // NULL-terminate the attributes list
    pixelAttribs[i] = 0;

    // Create the pixel format
    NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelAttribs];

    if ( !pixelFormat ) {
        fgError( "Failed to create OpenGL pixel format" );
        [nsWindow release];
        return;
    }

    // Create the OpenGL context
    NSOpenGLContext *openGLContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];

    if ( !openGLContext ) {
        fgError( "Failed to create OpenGL context" );
        [pixelFormat release];
        [nsWindow release];
        return;
    }

    // Create the OpenGL view
    NSOpenGLView *openGLView = [[NSOpenGLView alloc] initWithFrame:contentRect pixelFormat:pixelFormat];

    [openGLView setOpenGLContext:openGLContext];
    [openGLView setWantsBestResolutionOpenGLSurface:YES]; // For Retina support

    // Set the window content view to our OpenGL view
    [nsWindow setContentView:openGLView];

    // Store our Cocoa objects in the window structure
    window->Window.Handle                = (SFG_WindowHandleType)nsWindow;
    window->Window.Context               = (SFG_WindowContextType)openGLContext;
    window->Window.pContext.CocoaContext = openGLContext;
    window->Window.pContext.PixelFormat  = pixelFormat;
    window->Window.pContext.View         = openGLView;
    window->Window.DoubleBuffered        = GL_TRUE; // We always use double buffering

    // Initialize platform-specific window state
    window->State.Xpos   = contentRect.origin.x;
    window->State.Ypos   = contentRect.origin.y;
    window->State.Width  = contentRect.size.width;
    window->State.Height = contentRect.size.height;

    // Make this window's context current
    [openGLContext makeCurrentContext];

    // Set initial viewport and clear
    glViewport( 0, 0, contentRect.size.width, contentRect.size.height );
    glClear( GL_COLOR_BUFFER_BIT );

    // Enable VSync
    GLint swapInt = 1;
    [openGLContext setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];

    // Make sure the window is visible (critical for GLUT)
    [nsWindow makeKeyAndOrderFront:nil];
    window->State.Visible = GL_TRUE;

    if ( isSubWindow ) {
        // Set up as a child window
        if ( window->Parent ) {
            NSWindow *parentWindow = (NSWindow *)window->Parent->Window.Handle;
            [parentWindow addChildWindow:nsWindow ordered:NSWindowAbove];
        }
    }
}

/*
 * Sets the current window
 */
void fgPlatformSetWindow( SFG_Window *window )
{
    if ( !window )
        return;

    // Make this window's OpenGL context current
    NSOpenGLContext *context = (NSOpenGLContext *)window->Window.Context;
    if ( context ) {
        [context makeCurrentContext];
    }
}

/*
 * Closes a window, destroying the frame and OpenGL context
 */
void fgPlatformCloseWindow( SFG_Window *window )
{
    if ( !window )
        return;

    // Stop and release the display link
    if ( window->Window.pContext.DisplayLink ) {
        fgPlatformCleanupDisplayLink( window->Window.pContext.DisplayLink );
        window->Window.pContext.DisplayLink = NULL;
    }

    // Get our NSWindow, NSOpenGLContext, and NSOpenGLView
    NSWindow            *nsWindow    = (NSWindow *)window->Window.Handle;
    NSOpenGLContext     *context     = (NSOpenGLContext *)window->Window.Context;
    NSOpenGLView        *view        = (NSOpenGLView *)window->Window.pContext.View;
    NSOpenGLPixelFormat *pixelFormat = (NSOpenGLPixelFormat *)window->Window.pContext.PixelFormat;

    // Close the window (this also removes it from screen)
    [nsWindow orderOut:nil];

    // If it's a child window, detach from parent
    if ( window->Parent ) {
        NSWindow *parentWindow = (NSWindow *)window->Parent->Window.Handle;
        [parentWindow removeChildWindow:nsWindow];
    }

    // Clear the OpenGL context's drawable
    [context clearDrawable];

    // Release Cocoa objects
    [context release];
    [pixelFormat release];
    [view release];
    [nsWindow release];

    // Clear the window structure references
    window->Window.Handle                = NULL;
    window->Window.Context               = NULL;
    window->Window.pContext.CocoaContext = NULL;
    window->Window.pContext.PixelFormat  = NULL;
    window->Window.pContext.View         = NULL;
}

/*
 * Reshapes a window
 */
void fgPlatformReshapeWindow( SFG_Window *window, int width, int height )
{
    if ( !window )
        return;

    NSWindow *nsWindow = (NSWindow *)window->Window.Handle;
    if ( !nsWindow )
        return;

    // Get current frame
    NSRect frame = [nsWindow frame];

    // Calculate the new frame size while maintaining the top-left position
    // (Cocoa origin is bottom-left, GLUT uses top-left)
    NSRect newFrame      = frame;
    newFrame.origin.y    = frame.origin.y + frame.size.height - height;
    newFrame.size.width  = width;
    newFrame.size.height = height;

    // Set the new frame
    [nsWindow setFrame:newFrame display:YES animate:NO];

    // Update the OpenGL context
    NSOpenGLContext *context = (NSOpenGLContext *)window->Window.Context;
    if ( context ) {
        [context update];
    }

    // Store the new size
    window->State.Width  = width;
    window->State.Height = height;
}

/*
 * Shows a window
 */
void fgPlatformShowWindow( SFG_Window *window )
{
    if ( !window )
        return;

    NSWindow *nsWindow = (NSWindow *)window->Window.Handle;
    if ( nsWindow ) {
        [nsWindow makeKeyAndOrderFront:nil];
        window->State.Visible = GL_TRUE;
    }
}

/*
 * Hides a window
 */
void fgPlatformHideWindow( SFG_Window *window )
{
    if ( !window )
        return;

    NSWindow *nsWindow = (NSWindow *)window->Window.Handle;
    if ( nsWindow ) {
        [nsWindow orderOut:nil];
        window->State.Visible = GL_FALSE;
    }
}

/*
 * Iconify the window
 */
void fgPlatformIconifyWindow( SFG_Window *window )
{
    if ( !window )
        return;

    NSWindow *nsWindow = (NSWindow *)window->Window.Handle;
    if ( nsWindow ) {
        [nsWindow miniaturize:nil];
    }
}

/*
 * Sets a window's position
 */
void fgPlatformPositionWindow( SFG_Window *window, int x, int y )
{
    if ( !window )
        return;

    NSWindow *nsWindow = (NSWindow *)window->Window.Handle;
    if ( !nsWindow )
        return;

    // Get current frame
    NSRect frame = [nsWindow frame];

    // Calculate new origin (remember to convert from GLUT's top-left to Cocoa's bottom-left)
    NSPoint newOrigin;
    newOrigin.x = x;
    newOrigin.y = fgDisplay.ScreenHeight - y - frame.size.height;

    // Set new position
    [nsWindow setFrameOrigin:newOrigin];

    // Store the new position
    window->State.Xpos = x;
    window->State.Ypos = y;
}

/*
 * Toggles fullscreen for a window
 */
void fgPlatformFullScreenToggle( SFG_Window *window )
{
    if ( !window )
        return;

    NSWindow *nsWindow = (NSWindow *)window->Window.Handle;
    if ( !nsWindow )
        return;

    if ( !window->State.pWState.InFullScreen ) {
        // Store current window rect before going fullscreen
        window->State.pWState.WindowedRect = [nsWindow frame];

        // Enter fullscreen
        [nsWindow setStyleMask:NSWindowStyleMaskBorderless];
        [nsWindow setFrame:[[NSScreen mainScreen] frame] display:YES];
        window->State.pWState.InFullScreen = YES;
        window->State.IsFullscreen         = GL_TRUE;
    }
    else {
        // Exit fullscreen
        [nsWindow setStyleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |
                               NSWindowStyleMaskResizable];

        // Restore windowed mode frame
        [nsWindow setFrame:window->State.pWState.WindowedRect display:YES];
        window->State.pWState.InFullScreen = NO;
        window->State.IsFullscreen         = GL_FALSE;
    }

    // Update context for the new size
    NSOpenGLContext *context = (NSOpenGLContext *)window->Window.Context;
    if ( context ) {
        [context update];
    }
}

/*
 * Sets the window title
 */
void fgPlatformGlutSetWindowTitle( const char *title )
{
    if ( !fgStructure.CurrentWindow )
        return;

    NSWindow *nsWindow = (NSWindow *)fgStructure.CurrentWindow->Window.Handle;
    if ( nsWindow ) {
        [nsWindow setTitle:[NSString stringWithUTF8String:( title ? title : "GLUT" )]];
    }
}

/*
 * Sets the window's iconified title
 */
void fgPlatformGlutSetIconTitle( const char *title )
{
    // In macOS, there's no separate title for iconified state
    // Delegate to the normal title function
    fgPlatformGlutSetWindowTitle( title );
}

/*
 * Push the window to the background (lower Z-order)
 */
void fgPlatformPushWindow( SFG_Window *window )
{
    if ( !window )
        return;

    NSWindow *nsWindow = (NSWindow *)window->Window.Handle;
    if ( nsWindow ) {
        [nsWindow orderBack:nil];
    }
}

/*
 * Raise the window to the foreground (higher Z-order)
 */
void fgPlatformPopWindow( SFG_Window *window )
{
    if ( !window )
        return;

    NSWindow *nsWindow = (NSWindow *)window->Window.Handle;
    if ( nsWindow ) {
        [nsWindow orderFront:nil];
    }
}
#endif