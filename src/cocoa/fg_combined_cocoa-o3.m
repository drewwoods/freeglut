
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
#include <CoreVideo/CVDisplayLink.h> // Added for CVDisplayLink support

#define TODO_IMPL fgWarning( "%s not implemented yet in Cocoa", __func__ )

/* Menu font and color definitions */
#define FREEGLUT_MENU_FONT GLUT_BITMAP_HELVETICA_18

#define FREEGLUT_MENU_PEN_FORE_COLORS  { 0.0f, 0.0f, 0.0f, 1.0f }
#define FREEGLUT_MENU_PEN_BACK_COLORS  { 0.70f, 0.70f, 0.70f, 1.0f }
#define FREEGLUT_MENU_PEN_HFORE_COLORS { 0.0f, 0.0f, 0.0f, 1.0f }
#define FREEGLUT_MENU_PEN_HBACK_COLORS { 1.0f, 1.0f, 1.0f, 1.0f }

/* Platform-specific display structure */
struct CocoaPlatformDisplay {
    uint32_t         CocoaDisplay; /* The display ID */
    CVDisplayLinkRef DisplayLink;  /* CVDisplayLink for synchronized rendering */
};

/* Platform-specific window context */
struct CocoaPlatformContext {
    void *CocoaContext; /* OpenGL context (NSOpenGLContext*) */
    void *PixelFormat;  /* Pixel format (NSOpenGLPixelFormat*) */
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
// INIT
///////////////////////////////////////////

#include <QuartzCore/CVDisplayLink.h>

// Global variable to hold the display link reference for rendering
static CVDisplayLinkRef cocoaDisplayLink = NULL;

// Forward declaration of the display link callback function
static CVReturn CocoaDisplayLinkCallback( CVDisplayLinkRef displayLink,
    const CVTimeStamp                                     *now,
    const CVTimeStamp                                     *outputTime,
    CVOptionFlags                                          flagsIn,
    CVOptionFlags                                         *flagsOut,
    void                                                  *displayLinkContext );

// fgPlatformInitialize -- initialize the Cocoa platform and create the CVDisplayLink
void fgPlatformInitialize( const char *displayName )
{
    // (Your existing Cocoa initialization code here; for example, create NSApplication, etc.)

    // Create and configure the CVDisplayLink for rendering at 60fps
    CVReturn err = CVDisplayLinkCreateWithActiveCGDisplays( &cocoaDisplayLink );
    if ( err != kCVReturnSuccess ) {
        fgError( "CVDisplayLink creation failed: %d", err );
    }

    // Set the output callback with the current window/context as the context
    // For freeglut, you might store the current window (SFG_Window) in a global variable or pass a pointer
    err = CVDisplayLinkSetOutputCallback( cocoaDisplayLink, CocoaDisplayLinkCallback, NULL );
    if ( err != kCVReturnSuccess ) {
        fgError( "CVDisplayLinkSetOutputCallback failed: %d", err );
    }

    // Start the display link
    err = CVDisplayLinkStart( cocoaDisplayLink );
    if ( err != kCVReturnSuccess ) {
        fgError( "CVDisplayLinkStart failed: %d", err );
    }

    // Continue with any other initialization required...
    // Set the initial time
    fgState.Time = fgPlatformSystemTime( );

    // Mark freeglut as initialized
    fgState.Initialised = GL_TRUE;

    // Register the deinitialization function
    atexit( fgDeinitialize );

    // Initialize input devices (joysticks, etc.)
    // fgInitialiseInputDevices( );
}

// Display link callback function, which will be called at roughly 60fps
static CVReturn CocoaDisplayLinkCallback( CVDisplayLinkRef displayLink,
    const CVTimeStamp                                     *now,
    const CVTimeStamp                                     *outputTime,
    CVOptionFlags                                          flagsIn,
    CVOptionFlags                                         *flagsOut,
    void                                                  *displayLinkContext )
{
    // In a Cocoa freeglut implementation, you might want to trigger the window redraw here.
    // For example, post an event or call a function that schedules a redraw of the current window.
    // If you have a pointer to the current SFG_Window or its associated NSOpenGLView, call its display/draw method.

    // Here's an example assuming you have a function fgRenderCurrentWindow():
    // fgRenderCurrentWindow( );

    return kCVReturnSuccess;
}

// Optionally, implement a shutdown/deinitialization function to stop the CVDisplayLink
void fgPlatformCloseDisplay( void )
{
    if ( cocoaDisplayLink ) {
        CVDisplayLinkStop( cocoaDisplayLink );
        CVDisplayLinkRelease( cocoaDisplayLink );
        cocoaDisplayLink = NULL;
    }

    // Continue with any additional cleanup (e.g. releasing Cocoa resources)
}


void fgPlatformDeinitialiseInputDevices( void )
{
    TODO_IMPL;
}

void fgPlatformDestroyContext( SFG_PlatformDisplay pDisplay, SFG_WindowContextType MContext )
{
    TODO_IMPL;
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

void fgPlatformCreateWindow( SFG_Window *window )
{
    // For Cocoa, initialize the platform-specific OpenGL context and pixel format
    window->Window.pContext.CocoaContext = NULL;
    window->Window.pContext.PixelFormat  = NULL;

    // Initialize the platform window state dimensions to -1 (unset)
    window->State.pWState.OldWidth  = -1;
    window->State.pWState.OldHeight = -1;
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

#import <Cocoa/Cocoa.h>

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
    @autoreleasepool {
        // Determine the frame: if position or size are not specified, use defaults.
        NSRect frame;
        if ( sizeUse )
            frame = NSMakeRect( x, y, w, h );
        else
            frame = NSMakeRect( 100, 100, 800, 600 );

        // Choose style mask – for game mode, use a borderless window.
        NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
        if ( gameMode )
            style = NSWindowStyleMaskBorderless;

        // Create the NSWindow.
        NSWindow *nsWindow = [[NSWindow alloc] initWithContentRect:frame
                                                         styleMask:style
                                                           backing:NSBackingStoreBuffered
                                                             defer:NO];
        // Set the window title.
        NSString *nsTitle = [NSString stringWithUTF8String:title];
        [nsWindow setTitle:nsTitle];

        // Create an NSOpenGLPixelFormat with desired attributes.
        NSOpenGLPixelFormatAttribute attrs[]     = { NSOpenGLPFAOpenGLProfile,
                NSOpenGLProfileVersionLegacy,
                NSOpenGLPFADoubleBuffer,
                NSOpenGLPFAColorSize,
                24,
                NSOpenGLPFAAlphaSize,
                8,
                NSOpenGLPFADepthSize,
                24,
                NSOpenGLPFAAccelerated,
                0 };
        NSOpenGLPixelFormat         *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];

        // Create an NSOpenGLView using the pixel format.
        NSOpenGLView *glView = [[NSOpenGLView alloc] initWithFrame:[[nsWindow contentView] bounds]
                                                       pixelFormat:pixelFormat];
        [glView setWantsBestResolutionOpenGLSurface:YES];
        // Allow the view to resize with the window.
        [glView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

        // Set the content view of the window.
        [nsWindow setContentView:glView];
        [nsWindow makeKeyAndOrderFront:nil];

        // Retrieve the OpenGL context from the view.
        NSOpenGLContext *glContext = [glView openGLContext];

        // Store the NSWindow and NSOpenGLContext in the freeglut SFG_Window structure.
        // (We use CFBridgingRetain to convert to a void* and take ownership.)
        window->Window.Handle  = (SFG_WindowHandleType)CFBridgingRetain( nsWindow );
        window->Window.Context = (SFG_WindowContextType)CFBridgingRetain( glContext );

        // Update the freeglut window state.
        window->State.Xpos    = (int)frame.origin.x;
        window->State.Ypos    = (int)frame.origin.y;
        window->State.Width   = (int)frame.size.width;
        window->State.Height  = (int)frame.size.height;
        window->State.Visible = GL_TRUE;
    }
}

/*
 * Request a window resize
 */
void fgPlatformReshapeWindow( SFG_Window *window, int width, int height )
{
    @autoreleasepool {
        NSWindow *nsWindow = (__bridge NSWindow *)window->Window.Handle;
        NSRect    frame    = [nsWindow frame];
        frame.size.width   = width;
        frame.size.height  = height;
        [nsWindow setFrame:frame display:YES animate:NO];

        // Update internal state.
        window->State.Width  = width;
        window->State.Height = height;
    }
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
    // Set the current window pointer in the freeglut structure.
    fgStructure.CurrentWindow = window;

    // Retrieve the NSOpenGLContext and make it current.
    NSOpenGLContext *glContext = (__bridge NSOpenGLContext *)window->Window.Context;
    [glContext makeCurrentContext];
}
