/*
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

#define FREEGLUT_BUILDING_LIB

#include <GL/freeglut.h>
#include "../fg_internal.h"
#include "../fg_init.h"

#import <Cocoa/Cocoa.h>

#define DEBUG

#ifdef DEBUG
#define DGB( ... ) printf( __VA_ARGS__ )
#endif

#if 0
/* FreeGLUT platform initialization for Cocoa */
void fgPlatformInitialize(const char* displayName)
{
    [NSAutoreleasePool new];

    /* Initialize NSApplication - required for Cocoa */
    [NSApplication sharedApplication];

    /* Set up our application as a GUI app */
    ProcessSerialNumber psn = {0, kCurrentProcess};
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);
    SetFrontProcess(&psn);

    /* Create an app delegate to handle basic app lifecycle */
    NSObject<NSApplicationDelegate>* delegate = [[NSObject alloc] init];
    [NSApp setDelegate:delegate];

    /* Create display connection */
    fgDisplay.pDisplay.Display = CGMainDisplayID();
    if (!fgDisplay.pDisplay.Display) {
        fgError("Failed to connect to display");
    }

    /* Get main screen dimensions */
    NSRect screenFrame = [[NSScreen mainScreen] frame];
    fgDisplay.ScreenWidth = screenFrame.size.width;
    fgDisplay.ScreenHeight = screenFrame.size.height;

    /* Initialize window deletion protocol */
    fgDisplay.pDisplay.DeleteWindow = kCGWindowIDInvalid;

    /* Set up state tracking */
    fgDisplay.pDisplay.State = 0;
    fgDisplay.pDisplay.StateFullScreen = 0;
    fgDisplay.pDisplay.NetWMSupported = 0;

    /* 
     * Create the window delete protocol name
     * In Cocoa we use the window delegate method windowShouldClose: instead
     * but we keep this for compatibility
     */
    NSString *protocolName = @"WM_DELETE_WINDOW";
    NSData *nameData = [protocolName dataUsingEncoding:NSUTF8StringEncoding];
    fgDisplay.pDisplay.DeleteWindow = (Atom)[nameData bytes];

    /* Record the display connection number */
    fgDisplay.pDisplay.Connection = kCGDirectMainDisplay;

    /* Initialize window system properties */
    fgDisplay.pDisplay.ClientMachine = 0;
    fgDisplay.pDisplay.NetWMPid = 0;
    fgDisplay.pDisplay.NetWMName = 0;
    fgDisplay.pDisplay.NetWMIconName = 0;

    /* 
     * For OpenGL initialization, we'll defer until a window is created
     * since Cocoa manages OpenGL contexts differently than X11
     */
}

void fgPlatformInitialize(const char* displayName)
{
    @autoreleasepool {
        /* Initialize NSApplication - required for Cocoa */
        [NSApplication sharedApplication];

        /* Set up our application as a GUI app */
        ProcessSerialNumber psn = {0, kCurrentProcess};

        /* Modern replacement for SetFrontProcess */
        [[NSRunningApplication currentApplication] activateWithOptions:NSApplicationActivateIgnoringOtherApps];

        /* Create an app delegate to handle basic app lifecycle */
        FGAppDelegate* delegate = [[FGAppDelegate alloc] init];
        [NSApp setDelegate:delegate];

        /* Create display connection */
        fgDisplay.pDisplay.CocoaDisplay = CGMainDisplayID();
        if (!fgDisplay.pDisplay.CocoaDisplay) {
            fgError("Failed to connect to display for Cocoa");
        }

        /* Get main screen dimensions */
        NSRect screenFrame     = [[NSScreen mainScreen] frame];
        fgDisplay.ScreenWidth  = screenFrame.size.width;
        fgDisplay.ScreenHeight = screenFrame.size.height;

        /* Initialize window deletion protocol */
        fgDisplay.pDisplay.DeleteWindow = 0; // Using 0 instead of kCGWindowIDInvalid

        /* Set up state tracking */
        fgDisplay.pDisplay.State           = 0;
        fgDisplay.pDisplay.StateFullScreen = 0;
        fgDisplay.pDisplay.NetWMSupported  = 0;

        /* Create the window delete protocol name */
        NSString* protocolName          = @"WM_DELETE_WINDOW";
        NSData*   nameData              = [protocolName dataUsingEncoding:NSUTF8StringEncoding];
        fgDisplay.pDisplay.DeleteWindow = nameData;

        /* Record the display connection number */
        fgDisplay.pDisplay.Connection = kCGDirectMainDisplay;

        /* Initialize window system properties */
        fgDisplay.pDisplay.ClientMachine = 0;
        fgDisplay.pDisplay.NetWMPid      = 0;
        fgDisplay.pDisplay.NetWMName     = 0;
        fgDisplay.pDisplay.NetWMIconName = 0;
    } /* end @autoreleasepool */

    DGB("FreeGLUT: Platform initialized\n");

#ifdef X11_IMPLEMENTATION
    fgX11PlatformInitialize(displayName);
#endif
}
#endif

@interface FGAppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation FGAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    // Called after app finishes launching

    CGColorSpaceRef jcs = CGColorSpaceCreateWithName( kCGColorSpaceGenericRGB );

    // Create basic menu setup for proper macOS integration
    NSMenu     *menubar     = [[NSMenu new] autorelease];
    NSMenuItem *appMenuItem = [[NSMenuItem new] autorelease];
    [menubar addItem:appMenuItem];
    [NSApp setMainMenu:menubar];

    NSMenu     *appMenu      = [[NSMenu new] autorelease];
    NSString   *appName      = [[NSProcessInfo processInfo] processName];
    NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:[@"Quit " stringByAppendingString:appName]
                                                          action:@selector( terminate: )
                                                   keyEquivalent:@"q"];
    [appMenu addItem:quitMenuItem];
    [quitMenuItem release];
    [appMenuItem setSubmenu:appMenu];

    DGB( "FreeGLUT: Application did finish launching\n" );
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    // Called when user tries to quit the app
    // You might want to trigger GLUT close callbacks here
    return NSTerminateNow;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    // Return YES to automatically quit when all windows are closed
    return YES;
}

- (void)applicationWillBecomeActive:(NSNotification *)notification
{
    // Called when app is about to become active
}

- (void)applicationDidBecomeActive:(NSNotification *)notification
{
    // Called after app becomes active
}

- (void)applicationWillResignActive:(NSNotification *)notification
{
    // Called when app is about to become inactive
}

@end

void fgPlatformInitialize( const char *displayName )
{
    @autoreleasepool {
        /* Initialize NSApplication - required for Cocoa */
        [NSApplication sharedApplication];

        /* Set up our application as a GUI app */
        [[NSRunningApplication currentApplication] activateWithOptions:NSApplicationActivateIgnoringOtherApps];

        /* Create an app delegate to handle basic app lifecycle */
        FGAppDelegate *delegate = [[FGAppDelegate alloc] init];
        [NSApp setDelegate:delegate];

        /* Create display connection and get screen info */
        fgDisplay.pDisplay.CocoaDisplay = CGMainDisplayID( );
        if ( !fgDisplay.pDisplay.CocoaDisplay ) {
            fgError( "Failed to connect to display for Cocoa" );
        }

        /* Get main screen dimensions */
        NSRect screenFrame     = [[NSScreen mainScreen] frame];
        fgDisplay.ScreenWidth  = screenFrame.size.width;
        fgDisplay.ScreenHeight = screenFrame.size.height;
    }

    DGB( "FreeGLUT: Platform initialized\n" );
}

void fgPlatformDeinitialiseInputDevices( void )
{
    TODO_IMPL;
}

void fgPlatformCloseDisplay( void )
{
    TODO_IMPL;
}

void fgPlatformDestroyContext( SFG_PlatformDisplay pDisplay, SFG_WindowContextType MContext )
{
    TODO_IMPL;
}