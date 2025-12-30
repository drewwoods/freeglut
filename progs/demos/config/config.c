/*
 * windows.c - multi-window demo for freeglut
 * Written by Andrew Woods <drew.woods at gmail.com>
 *
 * Demonstrates creating multiple freeglut windows and querying/manipulating their positions and sizes.
 *
 * This highlights some gotchas in window geometry handling
 *
 * See docs/api.md#conventions
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* disable precision conversion warnings */
#ifdef _MSC_VER
#pragma warning( disable : 4305 4244 )
#endif

/* backward compatibility with GLUT for testing */
#if 1 && !defined( USE_GLUT )
#include <GL/freeglut.h>
#else
#include <GLUT/glut.h>
#endif

#define MIN( a, b )    ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX( a, b )    ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define MID( a, b, c ) MIN( MAX( a, b ), c )

#define ARRAY_SIZE( a ) ( sizeof( a ) / sizeof( a[0] ) )
#define NUM_WINDOWS     ARRAY_SIZE( winGeom )
#define WIN_SZ          200 /* MSVC  */

void global_info( void )
{
    int size = 0, i = 0;
    int *array = NULL;
    
    #define PRINT_ARRAY( name, arr, sz  ) \
        printf( "  " name ": " ); \
        for ( i = 0; i < sz; i++ ) printf( "%d ", arr[i] ); \
        printf( "\n" ); \

    printf( "------------------- Global GLUT Info ------------------\n" );
    printf( "glutGet():\n");
    printf( "  GLUT_DISPLAY_MODE_POSSIBLE: %d\n", glutGet( GLUT_DISPLAY_MODE_POSSIBLE ) );
    printf( "  GLUT_INIT_DISPLAY_MODE: %d\n", glutGet( GLUT_INIT_DISPLAY_MODE ) );
    printf( "  GLUT_INIT_WINDOW_X: %d\n", glutGet( GLUT_INIT_WINDOW_X ) );
    printf( "  GLUT_INIT_WINDOW_Y: %d\n", glutGet( GLUT_INIT_WINDOW_Y ) );
    printf( "  GLUT_INIT_WINDOW_WIDTH: %d\n", glutGet( GLUT_INIT_WINDOW_WIDTH ) );
    printf( "  GLUT_INIT_WINDOW_HEIGHT: %d\n", glutGet( GLUT_INIT_WINDOW_HEIGHT ) );
    printf( "  GLUT_SCREEN_WIDTH: %d\n", glutGet( GLUT_SCREEN_WIDTH ) );
    printf( "  GLUT_SCREEN_HEIGHT: %d\n", glutGet( GLUT_SCREEN_HEIGHT ) );
    printf( "  GLUT_SCREEN_WIDTH_MM: %d\n", glutGet( GLUT_SCREEN_WIDTH_MM ) );
    printf( "  GLUT_SCREEN_HEIGHT_MM: %d\n", glutGet( GLUT_SCREEN_HEIGHT_MM ) );
    printf( "glutGetModeValues():\n");

    array = glutGetModeValues(GLUT_AUX, &size);
    PRINT_ARRAY("GLUT_AUX", array, size);
    free(array);

    array = glutGetModeValues(GLUT_MULTISAMPLE, &size);
    PRINT_ARRAY("GLUT_MULTISAMPLE", array, size);
    free(array);
}

void window_info( void )
{
    printf( "------------------- Window %d Info ------------------\n", glutGetWindow( ) );
    printf( "GLUT_ELAPSED_TIME: %d\n", glutGet( GLUT_ELAPSED_TIME ) );
    printf( "GLUT_WINDOW_X: %d\n", glutGet( GLUT_WINDOW_X ) );
    printf( "GLUT_WINDOW_Y: %d\n", glutGet( GLUT_WINDOW_Y ) );
#ifdef FREEGLUT
    printf( "GLUT_WINDOW_BORDER_WIDTH: %d\n", glutGet( GLUT_WINDOW_BORDER_WIDTH ) );
    printf( "GLUT_WINDOW_BORDER_HEIGHT: %d\n", glutGet( GLUT_WINDOW_BORDER_HEIGHT ) );
    assert( GLUT_WINDOW_BORDER_HEIGHT == GLUT_WINDOW_HEADER_HEIGHT ); /* both defined the same */
#endif
}

void display( void )
{
    int win = glutGetWindow( );

    glClearColor( 0.2f, 0.2f, 0.2f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT );

    glColor3f( 1.0f, 1.0f, 0.0f );
    glBegin( GL_LINES );
    glVertex2f( -0.8f, 0.0f );
    glVertex2f(  0.8f, 0.0f );
    glVertex2f( 0.0f, -0.8f );
    glVertex2f( 0.0f,  0.8f );
    glEnd( );

    glutSwapBuffers( );
}

int main( int argc, char **argv )
{
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGBA );
    glutInitWindowSize( WIN_SZ, WIN_SZ );

    global_info( );

    glutCreateWindow("Window");

    glutSetColor(1, 1, 1, 1);

    glutDisplayFunc(display);
    glutMainLoop( );

    return 0;
}
