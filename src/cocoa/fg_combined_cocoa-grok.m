
// From src/cocoa/fg_init_cocoa.m
// #define DEBUG

#ifdef DEBUG_LOG
#define DGB( ... ) printf( __VA_ARGS__ )
#else
#define DGB( ... ) /* No operations for non-debug mode */
#endif

// #define FREEGLUT_BUILDING_LIB

///////////////////////////////////////////
// INTERNAL
///////////////////////////////////////////

#ifndef FREEGLUT_INTERNAL_COCOA_H
#define FREEGLUT_INTERNAL_COCOA_H

#include <unistd.h>
#include <CoreVideo/CVDisplayLink.h> // Include for CVDisplayLinkRef

#define DEBUG_LOG

#ifdef DEBUG_LOG
#define TODO_IMPL fgWarning( "%s not implemented yet in Cocoa", __func__ )
#define PART_IMPL fgWarning( "%s partially implemented in Cocoa", __func__ )
#define NO_IMPL   fgWarning( "%s not implemented in Cocoa", __func__ )
#else
#define TODO_IMPL
#define PART_IMPL
#define NO_IMPL
#endif

/* Menu font and color definitions */
#define FREEGLUT_MENU_FONT GLUT_BITMAP_HELVETICA_18

#define FREEGLUT_MENU_PEN_FORE_COLORS  { 0.0f, 0.0f, 0.0f, 1.0f }
#define FREEGLUT_MENU_PEN_BACK_COLORS  { 0.70f, 0.70f, 0.70f, 1.0f }
#define FREEGLUT_MENU_PEN_HFORE_COLORS { 0.0f, 0.0f, 0.0f, 1.0f }
#define FREEGLUT_MENU_PEN_HBACK_COLORS { 1.0f, 1.0f, 1.0f, 1.0f }

/* Platform-specific display structure */
struct CocoaPlatformDisplay {
    CVDisplayLinkRef DisplayLink; /* Core Video Display Link for vsync */
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
// PROTOTYPES (delete me)
///////////////////////////////////////////

fg_time_t fgPlatformSystemTime( void );
void      fgPlatformSetWindow( SFG_Window *window );

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

#import <Cocoa/Cocoa.h>

void fgPlatformGlutSwapBuffers( SFG_PlatformDisplay *pDisplayPtr, SFG_Window *CurrentWindow )
{
    // Release the OpenGL context
    NSOpenGLContext *context = (NSOpenGLContext *)CurrentWindow->Window.Context;
    [context makeCurrentContext];
    [context flushBuffer];

    // TODO: Emulate VSync using CVDisplayLink
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
    TODO_IMPL;
    return NULL;
}
void fg_serial_close( SERIALPORT *port )
{
    TODO_IMPL;
}
int fg_serial_getchar( SERIALPORT *port )
{
    TODO_IMPL;
    return EOF;
}
int fg_serial_putchar( SERIALPORT *port, unsigned char ch )
{
    TODO_IMPL;
    return 0;
}
void fg_serial_flush( SERIALPORT *port )
{
    TODO_IMPL;
}

///////////////////////////////////////////
// INIT
///////////////////////////////////////////

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#import <CoreVideo/CVDisplayLink.h>
#include <sys/time.h>

void fgPlatformInitialize( const char *displayName )
{
    // Initialize the Cocoa application
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // Get the main screen properties
    NSScreen *mainScreen   = [NSScreen mainScreen];
    NSRect    screenFrame  = [mainScreen frame];
    fgDisplay.ScreenWidth  = screenFrame.size.width;
    fgDisplay.ScreenHeight = screenFrame.size.height;

    // Calculate screen size in millimeters (optional, for compatibility with X11)
    CGFloat dpi              = mainScreen.backingScaleFactor * 96.0;   // Assuming 96 DPI base for macOS
    fgDisplay.ScreenWidthMM  = ( fgDisplay.ScreenWidth / dpi ) * 25.4; // Convert to mm
    fgDisplay.ScreenHeightMM = ( fgDisplay.ScreenHeight / dpi ) * 25.4;

    // Prepare for CVDisplayLink (actual setup will occur per-window, but initialize global state here)
    fgDisplay.pDisplay.DisplayLink = NULL; // Will be set up later in window creation

    // Set the initial time
    fgState.Time = fgPlatformSystemTime( );

    // Mark freeglut as initialized
    fgState.Initialised = GL_TRUE;

    // Register the deinitialization function
    atexit( fgDeinitialize );

    // Initialize input devices (joysticks, etc.)
    fgInitialiseInputDevices( );
}

void fgPlatformDeinitialiseInputDevices( void )
{
    // Clean up any input device resources
    // For now, just reset the state flags as a placeholder
    fgState.JoysticksInitialised = GL_FALSE;
    fgState.InputDevsInitialised = GL_FALSE;
    PART_IMPL;
    // TODO: Add specific cleanup for Cocoa input devices if implemented later
}

void fgPlatformCloseDisplay( void )
{
    // Clean up display-related resources
    if ( fgDisplay.pDisplay.DisplayLink != NULL ) {
        CVDisplayLinkStop( fgDisplay.pDisplay.DisplayLink );
        CVDisplayLinkRelease( fgDisplay.pDisplay.DisplayLink );
        fgDisplay.pDisplay.DisplayLink = NULL;
    }
    // No explicit display connection to close in Cocoa, unlike X11
}

void fgPlatformDestroyContext( SFG_PlatformDisplay pDisplay, SFG_WindowContextType MContext )
{
    // Release the OpenGL context
    NSOpenGLContext *context = (NSOpenGLContext *)MContext;
    [context clearDrawable]; // Ensure the context is detached from any drawable
    [context release];
}

///////////////////////////////////////////
// INPUT DEVICES
///////////////////////////////////////////

/*
 * Try initializing the input device(s)
 */
void fgPlatformRegisterDialDevice( const char *dial_device )
{
    NO_IMPL;
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

fg_time_t fgPlatformSystemTime( void )
{
    uint64_t now_ns = clock_gettime_nsec_np( CLOCK_REALTIME );
    return (fg_time_t)( now_ns / 1000000LL ); // Return time in milliseconds
}

/*
 * Does the magic required to relinquish the CPU until something interesting
 * happens.
 */

void fgPlatformSleepForEvents( fg_time_t msec )
{
    // Implement sleep functionality according to msec
    @autoreleasepool {
        NSTimeInterval timeout_sec = ( msec == INT_MAX ) ? 1.0 : ( msec / 1000.0 );
        NSEvent       *event       = [NSApp nextEventMatchingMask:NSEventMaskAny
                                            untilDate:[NSDate dateWithTimeIntervalSinceNow:timeout_sec]
                                               inMode:NSDefaultRunLoopMode
                                              dequeue:YES];
        if ( event ) {
            [NSApp sendEvent:event];
        }
    }
}

void fgPlatformProcessSingleEvent( void )
{
    @autoreleasepool {
        // Process all pending Cocoa events
        while ( true ) {
            NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                untilDate:[NSDate distantPast] // Return immediately
                                                   inMode:NSDefaultRunLoopMode
                                                  dequeue:YES];
            if ( !event )
                break; // Exit when no more events are pending
            [NSApp sendEvent:event];

            // TODO: Do we need to also do: [NSApp updateWindows];
        }

        // Set the current window’s OpenGL context after event processing
        if ( fgStructure.CurrentWindow ) {
            fgPlatformSetWindow( fgStructure.CurrentWindow );
        }
    }
}

void fgPlatformMainLoopPreliminaryWork( void )
{
    [NSApp finishLaunching];               // Completes the app launch process
    [NSApp activateIgnoringOtherApps:YES]; // Bring app to the front
}

/* deal with work list items */
void fgPlatformInitWork( SFG_Window *window )
{
    PART_IMPL;
    NSWindow *nsWindow = (NSWindow *)window->Window.Handle;
    // Placeholder for initialization tasks
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

void fgPlatformCreateWindow( SFG_Window *window )
{
    window->Window.pContext.PixelFormat = NULL;
    window->State.pWState.OldWidth      = -1;
    window->State.pWState.OldHeight     = -1;
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

int *fgPlatformGlutGetModeValues( GLenum eWhat, int *size )
{
    TODO_IMPL;
    return NULL;
}

///////////////////////////////////////////
// WINDOW
///////////////////////////////////////////

// #define MOUSEWHEEL_X_AXIS // Define to enable horizontal mouse wheel events

extern void fghOnReshapeNotify( SFG_Window *window, int width, int height, GLboolean forceNotify );
extern void fghOnPositionNotify( SFG_Window *window, int x, int y, GLboolean forceNotify );

static const int    fgMouseYWheel    = 0;
static const int    fgMouseXWheel    = 1;
static const double fgWheelThreshold = 1.0; // Threshold for mouse wheel events. TODO: decide on a suitable value

@interface                       fgWindowDelegate : NSObject <NSWindowDelegate>
@property ( assign ) SFG_Window *fgWindow; // Freeglut’s window structure
@end

@implementation fgWindowDelegate

- (BOOL)windowShouldClose:(NSWindow *)sender
{
    glutDestroyWindow( self.fgWindow->ID ); // Freeglut’s window cleanup
    return YES;
}

- (void)windowDidChangeOcclusionState:(NSNotification *)notification
{
    BOOL isVisible               = [notification.object occlusionState] & NSWindowOcclusionStateVisible;
    self.fgWindow->State.Visible = isVisible;

    if ( isVisible ) {
        INVOKE_WCB( *self.fgWindow, WindowStatus, ( GLUT_FULLY_RETAINED ) );
    }
    else {
        INVOKE_WCB( *self.fgWindow, WindowStatus, ( GLUT_FULLY_COVERED ) );
    }
}
@end

@interface                       fgOpenGLView : NSOpenGLView
@property ( assign ) SFG_Window *fgWindow; // Freeglut’s window structure
@end

@implementation fgOpenGLView

+ (char)mapKeyToSpecial:(uint16_t)key
{
    switch ( key ) {
    case NSUpArrowFunctionKey:
        return GLUT_KEY_UP;
    case NSDownArrowFunctionKey:
        return GLUT_KEY_DOWN;
    case NSLeftArrowFunctionKey:
        return GLUT_KEY_LEFT;
    case NSRightArrowFunctionKey:

        return GLUT_KEY_RIGHT;
    case NSF1FunctionKey:
        return GLUT_KEY_F1;
    case NSF2FunctionKey:
        return GLUT_KEY_F2;
    case NSF3FunctionKey:
        return GLUT_KEY_F3;
    case NSF4FunctionKey:
        return GLUT_KEY_F4;
    case NSF5FunctionKey:
        return GLUT_KEY_F5;
    case NSF6FunctionKey:
        return GLUT_KEY_F6;
    case NSF7FunctionKey:
        return GLUT_KEY_F7;
    case NSF8FunctionKey:
        return GLUT_KEY_F8;
    case NSF9FunctionKey:
        return GLUT_KEY_F9;
    case NSF10FunctionKey:
        return GLUT_KEY_F10;
    case NSF11FunctionKey:
        return GLUT_KEY_F11;
    case NSF12FunctionKey:
        return GLUT_KEY_F12;

    case NSPageUpFunctionKey:
        return GLUT_KEY_PAGE_UP;
    case NSPageDownFunctionKey:
        return GLUT_KEY_PAGE_DOWN;
    case NSHomeFunctionKey:
        return GLUT_KEY_HOME;
    case NSEndFunctionKey:
        return GLUT_KEY_END;
    case NSInsertFunctionKey:
    case NSInsertCharFunctionKey:
        return GLUT_KEY_INSERT;

    case NSDeleteFunctionKey:
    case NSDeleteCharFunctionKey:
        return GLUT_KEY_DELETE;

    // undocumented key codes
    case 0x38: // Left Shift
        return GLUT_KEY_SHIFT_L;
    case 0x3C: // Right Shift
        return GLUT_KEY_SHIFT_R;
    case 0x3B: // Left Control
        return GLUT_KEY_CTRL_L;
    case 0x3E: // Right Control
        return GLUT_KEY_CTRL_R;
    case 0x3A: // Left Option (Alt)
        return GLUT_KEY_ALT_L;
    case 0x3D: // Right Option (Alt)
        return GLUT_KEY_ALT_R;
    case 0x37: // Left Command
        return GLUT_KEY_SUPER_L;
    case 0x36: // Right Command
        return GLUT_KEY_SUPER_R;
    }

    return (char)key;
}

- (BOOL)acceptsFirstResponder
{
    return YES; // Allow the view to receive keyboard events
}

- (void)updateModifiers:(NSEvent *)event
{
    // Update the modifier key state
    int modifiers = 0;
    if ( [event modifierFlags] & NSEventModifierFlagShift ) {
        modifiers |= GLUT_ACTIVE_SHIFT;
    }
    if ( [event modifierFlags] & NSEventModifierFlagControl ) {
        modifiers |= GLUT_ACTIVE_CTRL;
    }
    if ( [event modifierFlags] & NSEventModifierFlagOption ) {
        modifiers |= GLUT_ACTIVE_ALT;
    }
    if ( [event modifierFlags] & NSEventModifierFlagCommand ) {
        modifiers |= GLUT_ACTIVE_SUPER;
    }
    fgState.Modifiers = modifiers;
}

#pragma mark Mouse Section

// Enable mouse movement events
- (BOOL)acceptsMouseMovedEvents
{
    return YES;
}

// Left button pressed
- (void)mouseDown:(NSEvent *)event
{
    [self handleMouseEvent:event withButton:GLUT_LEFT_BUTTON state:GLUT_DOWN];
}

// Left button released
- (void)mouseUp:(NSEvent *)event
{
    [self handleMouseEvent:event withButton:GLUT_LEFT_BUTTON state:GLUT_UP];
}

// Right button pressed
- (void)rightMouseDown:(NSEvent *)event
{
    [self handleMouseEvent:event withButton:GLUT_RIGHT_BUTTON state:GLUT_DOWN];
}

// Right button released
- (void)rightMouseUp:(NSEvent *)event
{
    [self handleMouseEvent:event withButton:GLUT_RIGHT_BUTTON state:GLUT_UP];
}

// Middle or other buttons pressed
- (void)otherMouseDown:(NSEvent *)event
{
    int button = ( [event buttonNumber] == 2 ) ? GLUT_MIDDLE_BUTTON : [event buttonNumber];
    [self handleMouseEvent:event withButton:button state:GLUT_DOWN];
}

// Middle or other buttons released
- (void)otherMouseUp:(NSEvent *)event
{
    int button = ( [event buttonNumber] == 2 ) ? GLUT_MIDDLE_BUTTON : [event buttonNumber];
    [self handleMouseEvent:event withButton:button state:GLUT_UP];
}

// Centralized handler for mouse events
- (void)handleMouseEvent:(NSEvent *)event withButton:(int)button state:(int)state
{
    if ( !self.fgWindow ) {
        return;
    }

    [self updateModifiers:event];

    NSPoint mouseLoc = [self convertPoint:[event locationInWindow] fromView:nil];
    int     x        = (int)mouseLoc.x;
    int     y        = (int)( self.bounds.size.height - mouseLoc.y ); // Flip y for OpenGL
    INVOKE_WCB( *self.fgWindow, Mouse, ( button, state, x, y ) );
}

// Passive motion: mouse moves with no buttons pressed
- (void)mouseMoved:(NSEvent *)event
{
    if ( self.fgWindow ) {
        return;
    }

    [self updateModifiers:event];

    NSPoint mouseLoc = [self convertPoint:[event locationInWindow] fromView:nil];
    int     x        = (int)mouseLoc.x;
    int     y        = (int)( self.bounds.size.height - mouseLoc.y );
    INVOKE_WCB( *self.fgWindow, Passive, ( x, y ) );
}

// Active motion: mouse moves with a button pressed
- (void)mouseDragged:(NSEvent *)event
{
    if ( !self.fgWindow ) {
        return;
    }

    [self updateModifiers:event];

    NSPoint mouseLoc = [self convertPoint:[event locationInWindow] fromView:nil];
    int     x        = (int)mouseLoc.x;
    int     y        = (int)( self.bounds.size.height - mouseLoc.y );
    INVOKE_WCB( *self.fgWindow, Motion, ( x, y ) );
}

- (void)scrollWheel:(NSEvent *)event
{
    if ( !self.fgWindow ) {
        return;
    }

    [self updateModifiers:event];

    static double bufferedX = 0.0;
    static double bufferedY = 0.0;

    // Get mouse coordinates in the view
    NSPoint mouseLoc = [self convertPoint:[event locationInWindow] fromView:nil];
    int     x        = (int)mouseLoc.x;
    int     y        = (int)( self.bounds.size.height - mouseLoc.y ); // Flip y-axis for OpenGL

    double deltaX = [event scrollingDeltaX];
    double deltaY = [event scrollingDeltaY];

    if ( [event hasPreciseScrollingDeltas] ) {
        deltaX *= 0.1;
        deltaY *= 0.1;
    }

#ifdef MOUSEWHEEL_X_AXIS
    if ( fabs( bufferedX ) > fgWheelThreshold ) {
        int direction = ( bufferedX > 0 ) ? GLUT_UP : GLUT_DOWN;
        INVOKE_WCB( *self.fgWindow, MouseWheel, ( fgMouseXWheel, direction, x, y ) );
        bufferedX = 0.0;
    }
    else {
        bufferedX += deltaX;
    }
#endif

    // TODO: Decide on a suitable threshold for scrolling events
    // Macos sends a lot of small delta values, so we need to filter them if we want to match the behavior of other
    // platforms
    if ( fabs( bufferedY ) > fgWheelThreshold ) {
        int direction = ( bufferedY > 0 ) ? GLUT_UP : GLUT_DOWN;
        INVOKE_WCB( *self.fgWindow, MouseWheel, ( fgMouseYWheel, direction, x, y ) );
        bufferedY = 0.0;
    }
    else {
        bufferedY += deltaY;
    }
}

#pragma mark Key Section

- (void)keyDown:(NSEvent *)event
{
    if ( !self.fgWindow || ![[event characters] length] ) {
        return;
    }

    [self updateModifiers:event];

    NSPoint mouseLoc = [self convertPoint:[[self window] mouseLocationOutsideOfEventStream] fromView:nil];
    int     x        = (int)mouseLoc.x;
    int     y        = (int)( self.bounds.size.height - mouseLoc.y ); // Flip y-coordinate for OpenGL

    unichar key       = [[event charactersIgnoringModifiers] characterAtIndex:0];
    char    convKey   = [fgOpenGLView mapKeyToSpecial:key];
    BOOL    isSpecial = ( convKey != key );

    if ( isSpecial ) {
        INVOKE_WCB( *self.fgWindow, Special, ( convKey, x, y ) );
    }
    else {
        INVOKE_WCB( *self.fgWindow, Keyboard, ( convKey, x, y ) );
    }
}

- (void)keyUp:(NSEvent *)event
{
    if ( !self.fgWindow || ![[event characters] length] ) {
        return;
    }

    [self updateModifiers:event];

    NSPoint mouseLoc = [self convertPoint:[[self window] mouseLocationOutsideOfEventStream] fromView:nil];
    int     x        = (int)mouseLoc.x;
    int     y        = (int)( self.bounds.size.height - mouseLoc.y );

    uint16_t key       = [[event charactersIgnoringModifiers] characterAtIndex:0];
    char    convKey   = [fgOpenGLView mapKeyToSpecial:key];
    BOOL    isSpecial = ( convKey != key );

    if ( isSpecial ) {
        INVOKE_WCB( *self.fgWindow, SpecialUp, ( convKey, x, y ) );
    }
    else {
        INVOKE_WCB( *self.fgWindow, KeyboardUp, ( convKey, x, y ) );
    }
}

// Handles individual modifier key changes
- (void)flagsChanged:(NSEvent *)event
{
    if ( !self.fgWindow ) {
        return;
    }

    NSEventModifierFlags flags   = [event modifierFlags];
    uint16_t             keyCode = [event keyCode];

    int  state      = -1;
    char specialKey = [fgOpenGLView mapKeyToSpecial:keyCode];

    switch ( specialKey ) {
    case GLUT_KEY_SHIFT_L:
    case GLUT_KEY_SHIFT_R:
        state = ( flags & NSEventModifierFlagShift ) ? GLUT_DOWN : GLUT_UP;
        break;
    case GLUT_KEY_CTRL_L:
    case GLUT_KEY_CTRL_R:
        state = ( flags & NSEventModifierFlagControl ) ? GLUT_DOWN : GLUT_UP;
        break;
    case GLUT_KEY_ALT_L:
    case GLUT_KEY_ALT_R: // Right Option (Alt)
        state = ( flags & NSEventModifierFlagOption ) ? GLUT_DOWN : GLUT_UP;
        break;
    case GLUT_KEY_SUPER_L: // Note: FreeGLUT may not define this; use 0x75 if needed
    case GLUT_KEY_SUPER_R: // Note: FreeGLUT may not define this; use 0x75 if needed
        state = ( flags & NSEventModifierFlagCommand ) ? GLUT_DOWN : GLUT_UP;
        break;
    default:
        return; // Ignore unmapped keys
    }

    NSPoint mouseLoc = [self convertPoint:[[self window] mouseLocationOutsideOfEventStream] fromView:nil];
    int     x        = (int)mouseLoc.x;
    int     y        = (int)( self.bounds.size.height - mouseLoc.y ); // Flip y for OpenGL

    if ( state == GLUT_DOWN ) {
        INVOKE_WCB( *self.fgWindow, Special, ( specialKey, x, y ) );
    }
    else {
        INVOKE_WCB( *self.fgWindow, SpecialUp, ( specialKey, x, y ) );
    }
}

#pragma mark -

/*
 * Handle window resize events - notify freeglut
 */

- (void)reshape
{
    [super reshape];

    // TODO: move all these window guards to a separate method or delegate
    if ( !self.fgWindow ) {
        return;
    }

    NSWindow *window = self.fgWindow->Window.Handle;
    NSRect    frame  = [window contentRectForFrameRect:[window frame]];

    /* Update state and call callback, if there was a change */
    fghOnPositionNotify( self.fgWindow, frame.origin.x, frame.origin.y, GL_FALSE );
    fghOnReshapeNotify( self.fgWindow, frame.size.width, frame.size.height, GL_FALSE );
}

@end

/*
 * Freeglut request a window resize
 */
void fgPlatformReshapeWindow( SFG_Window *window, int width, int height )
{
    printf( "fgPlatformReshapeWindow, width: %d, height: %d\n", width, height );
    NSWindow *nsWindow = (NSWindow *)window->Window.Handle;
    [nsWindow setContentSize:NSMakeSize( width, height )];

    // Update OpenGL viewport
    [(NSOpenGLContext *)window->Window.Context makeCurrentContext];
    glViewport( 0, 0, width, height );

    // Todo: this is for fullscreen toggle
    // window->State.pWState.OldWidth  = width;
    // window->State.pWState.OldHeight = height;
}

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
    // 1. Define pixel format attributes based on fgState.DisplayMode
    NSOpenGLPixelFormatAttribute attrs[32];
    int                          attrIndex = 0;
    attrs[attrIndex++]                     = NSOpenGLPFAOpenGLProfile;
    attrs[attrIndex++]                     = NSOpenGLProfileVersionLegacy; // Legacy profile; update as needed
    attrs[attrIndex++]                     = NSOpenGLPFAColorSize;
    attrs[attrIndex++]                     = 24; // 8 bits per RGB channel
    attrs[attrIndex++]                     = NSOpenGLPFAAlphaSize;
    attrs[attrIndex++]                     = 8;
    if ( fgState.DisplayMode & GLUT_DOUBLE ) {
        attrs[attrIndex++] = NSOpenGLPFADoubleBuffer;
    }
    if ( fgState.DisplayMode & GLUT_DEPTH ) {
        attrs[attrIndex++] = NSOpenGLPFADepthSize;
        attrs[attrIndex++] = 24;
    }
    if ( fgState.DisplayMode & GLUT_STENCIL ) {
        attrs[attrIndex++] = NSOpenGLPFAStencilSize;
        attrs[attrIndex++] = 8;
    }
    if ( fgState.DisplayMode & GLUT_ACCUM ) {
        attrs[attrIndex++] = NSOpenGLPFAAccumSize;
        attrs[attrIndex++] = 32;
    }
    attrs[attrIndex++] = 0; // Null terminator

    NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if ( !pixelFormat ) {
        fgError( "Failed to create pixel format" );
    }
    window->Window.pContext.PixelFormat = pixelFormat;

    // 2. Create fgOpenGLView with the pixel format
    NSRect        frame = NSMakeRect( positionUse ? x : 0, positionUse ? y : 0, sizeUse ? w : 300, sizeUse ? h : 300 );
    fgOpenGLView *openGLView = [[fgOpenGLView alloc] initWithFrame:frame pixelFormat:pixelFormat];
    if ( !openGLView ) {
        fgError( "Failed to create fgOpenGLView" );
    }
    openGLView.fgWindow = window; // Link to the FreeGLUT window structure

    // 3. Create NSWindow and set fgOpenGLView as content view
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |
                              NSWindowStyleMaskResizable;
    if ( window->IsMenu || gameMode ) {
        style = NSWindowStyleMaskBorderless;
    }
    NSWindow *nsWindow = [[NSWindow alloc] initWithContentRect:frame
                                                     styleMask:style
                                                       backing:NSBackingStoreBuffered
                                                         defer:NO];
    [nsWindow setAcceptsMouseMovedEvents:YES];
    [nsWindow setTitle:[NSString stringWithUTF8String:title ? title : "freeglut"]];
    window->Window.Handle = nsWindow;

    // use the fgOpenGLView as the content view
    [nsWindow setContentView:openGLView];

    // 4. Set window delegate
    fgWindowDelegate *delegate = [[fgWindowDelegate alloc] init];
    delegate.fgWindow          = window;
    [nsWindow setDelegate:delegate];

    // 5. Retrieve the NSOpenGLContext from the fgOpenGLView
    NSOpenGLContext *context = [openGLView openGLContext];
    if ( !context ) {
        fgError( "Failed to retrieve NSOpenGLContext from fgOpenGLView" );
    }
    window->Window.Context = context;

    // 6. Make the context current for OpenGL rendering
    [context makeCurrentContext];

    // 7. Log OpenGL version (optional, for debugging)
    const GLubyte *version = glGetString( GL_VERSION );
    NSLog( @"OpenGL Version: %s", version );

    // 8. Show the window if not a menu and make it first responder
    if ( !window->IsMenu ) {
        [nsWindow makeKeyAndOrderFront:nil];
        [nsWindow makeFirstResponder:openGLView]; // Ensure view receives events
        window->State.Visible = GL_TRUE;
    }

    // 9. Store initial window size
    window->State.pWState.OldWidth  = frame.size.width;
    window->State.pWState.OldHeight = frame.size.height;
}

/*
 * Closes a window, destroying the frame and OpenGL context
 */
void fgPlatformCloseWindow( SFG_Window *window )
{
    NSWindow        *nsWindow = (NSWindow *)window->Window.Handle;
    NSOpenGLContext *context  = (NSOpenGLContext *)window->Window.Context;

    // Detach the OpenGL context from the view
    [context clearDrawable];

    // Close the window
    [nsWindow close];

    // Release resources
    [context release];
    [nsWindow release];
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
    if ( window && window->Window.Context ) {
        [(NSOpenGLContext *)window->Window.Context makeCurrentContext];
    }
    else {
        [NSOpenGLContext clearCurrentContext];
    }
}