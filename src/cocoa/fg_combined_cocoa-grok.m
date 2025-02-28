
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
    PART_IMPL;
    // Release the OpenGL context
    NSOpenGLContext *context = (NSOpenGLContext *)CurrentWindow->Window.Context;
    [context makeCurrentContext];
    [context flushBuffer];
}

///////////////////////////////////////////
// EXT
///////////////////////////////////////////

#include <dlfcn.h>

GLUTproc fgPlatformGetGLUTProcAddress( const char* procName )
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

SFG_Proc fgPlatformGetProcAddress( const char* procName )
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
// INIT
///////////////////////////////////////////

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#import <CoreVideo/CVDisplayLink.h>
#include <sys/time.h>

#if 0
// Window delegate to handle close events
@interface AppDelegate : NSObject <NSWindowDelegate>
@end

@implementation AppDelegate
- (BOOL)windowShouldClose:(NSWindow *)sender
{
    return YES;
}
@end

// Custom NSOpenGLView subclass for OpenGL rendering
@interface MyOpenGLView : NSOpenGLView
@end

@implementation MyOpenGLView

@end
#endif

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
    struct timeval now;
    gettimeofday( &now, NULL );
    return (fg_time_t)( now.tv_sec * 1000LL + now.tv_usec / 1000 ); // Return time in milliseconds
}

/*
 * Does the magic required to relinquish the CPU until something interesting
 * happens.
 */

void fgPlatformSleepForEvents( fg_time_t msec )
{
    // Implement sleep functionality according to msec
    PART_IMPL;
    usleep( msec * 1000 ); // Sleep for the specified milliseconds
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
    PART_IMPL;
    if ( fgStructure.CurrentWindow ) {
        fgPlatformSetWindow( fgStructure.CurrentWindow );
        // display() would be called here or via a callback mechanism
    }
    NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                        untilDate:[NSDate distantPast]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:YES];
    if ( event ) {
        [NSApp sendEvent:event];
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
    // Example: [nsWindow setDelegate:someDelegate];
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

int* fgPlatformGlutGetModeValues( GLenum eWhat, int* size )
{
    TODO_IMPL;
    return NULL;
}

///////////////////////////////////////////
// WINDOW
///////////////////////////////////////////

#if 1
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
    // 1. Determine pixel format based on fgState.DisplayMode
    NSOpenGLPixelFormatAttribute attrs[32];
    int                          attrIndex = 0;
    attrs[attrIndex++]                     = NSOpenGLPFAOpenGLProfile;
    // attrs[attrIndex++]                     = NSOpenGLProfileVersion3_2Core; // Default to a modern profile
    attrs[attrIndex++] = NSOpenGLProfileVersionLegacy; // Use legacy profile for now, TODO: query glutInitContextVersion
    attrs[attrIndex++] = NSOpenGLPFAColorSize;
    attrs[attrIndex++] = 24; // RGB 8 bits each
    attrs[attrIndex++] = NSOpenGLPFAAlphaSize;
    attrs[attrIndex++] = 8;
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

    // 2. Create NSWindow
    NSRect frame = NSMakeRect( positionUse ? x : 0, positionUse ? y : 0, sizeUse ? w : 300, sizeUse ? h : 300 );
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |
                              NSWindowStyleMaskResizable;
    if ( window->IsMenu || gameMode ) {
        style = NSWindowStyleMaskBorderless;
    }
    NSWindow *nsWindow = [[NSWindow alloc] initWithContentRect:frame
                                                     styleMask:style
                                                       backing:NSBackingStoreBuffered
                                                         defer:NO];
    [nsWindow setTitle:[NSString stringWithUTF8String:title ? title : "freeglut"]];
    window->Window.Handle = nsWindow;

    // 3. Create NSOpenGLContext
    NSOpenGLContext *context;
    if ( window->IsMenu && fgStructure.MenuContext ) {
        NSLog( @"Menu context found" );
        context = (NSOpenGLContext *)fgStructure.MenuContext->MContext;
    }
    else {
        NSLog( @"Creating OpenGL context" );
        context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
    }
    if ( !context ) {
        fgError( "Failed to create OpenGL context" );
    }
    window->Window.Context = context;

    // 4. Attach context to window
    [context setView:[nsWindow contentView]];

    // 5. Make context current
    [context makeCurrentContext];

    const GLubyte *version = glGetString( GL_VERSION );
    NSLog( @"OpenGL Version: %s", version );

    // 6. Display the window (unless it’s a menu)
    if ( !window->IsMenu ) {
        [nsWindow makeKeyAndOrderFront:nil];
        window->State.Visible = GL_TRUE;
    }

    // 7. Initialize window state
    window->State.pWState.OldWidth  = frame.size.width;
    window->State.pWState.OldHeight = frame.size.height;
}
#else

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
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    NSOpenGLPixelFormatAttribute attrs[] = {
        // NSOpenGLPFATripleBuffer,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAColorSize,
        24,
        NSOpenGLPFAAlphaSize,
        8,
        NSOpenGLPFADepthSize,
        24,
        NSOpenGLPFAStencilSize,
        8,
        NSOpenGLPFAOpenGLProfile,
        NSOpenGLProfileVersionLegacy,
        0 // Null terminator
    };

    // clang-format on
    NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if ( !pixelFormat ) {
        NSLog( @"Failed to create pixel format" );
        return;
    }

    window->Window.pContext.PixelFormat = pixelFormat;

    // 2. Create NSWindow
    // Create the window
    NSRect    frame    = NSMakeRect( 0, 0, 800, 600 );
    NSWindow *nsWindow = [[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered
                      defer:NO];
    [nsWindow setTitle:@"OpenGL Test"];
    window->Window.Handle = nsWindow;

    // Create and set up the OpenGL view
    // NSOpenGLView *openglView = [[NSOpenGLView alloc] initWithFrame:frame pixelFormat:pixelFormat];
    // [nsWindow setContentView:openglView];

    // 3. Create NSOpenGLContext
    NSOpenGLContext *context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
    [context makeCurrentContext];
    const GLubyte *version = glGetString( GL_VERSION );
    NSLog( @"OpenGL Version: %s", version );
    window->Window.Context = context;

    // 4. Attach context to window
    [context setView:[nsWindow contentView]];

    // 6. Display the window (unless it’s a menu)
    if ( !window->IsMenu ) {
        [nsWindow makeKeyAndOrderFront:nil];
        window->State.Visible = GL_TRUE;
    }

    // 7. Initialize window state
    window->State.pWState.OldWidth  = frame.size.width;
    window->State.pWState.OldHeight = frame.size.height;

    // 5. Make context current
    [context makeCurrentContext];
}
#endif

/*
 * Request a window resize
 */
void fgPlatformReshapeWindow( SFG_Window *window, int width, int height )
{
    NSWindow *nsWindow = (NSWindow *)window->Window.Handle;
    [nsWindow setContentSize:NSMakeSize( width, height )];

    // Update OpenGL viewport
    [(NSOpenGLContext *)window->Window.Context makeCurrentContext];
    glViewport( 0, 0, width, height );

    // Update stored state
    window->State.pWState.OldWidth  = width;
    window->State.pWState.OldHeight = height;
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