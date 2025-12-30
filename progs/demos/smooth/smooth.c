/*
 * Original copyright notice from smooth.c:
 *
 * License Applicability. Except to the extent portions of this file are
 * made subject to an alternative license as permitted in the SGI Free
 * Software License B, Version 1.1 (the "License"), the contents of this
 * file are subject only to the provisions of the License. You may not use
 * this file except in compliance with the License. You may obtain a copy
 * of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
 * Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
 *
 * http://oss.sgi.com/projects/FreeB
 *
 * Note that, as provided in the License, the Software is distributed on an
 * "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
 * DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
 * CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
 * PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 *
 * Original Code. The Original Code is: OpenGL Sample Implementation,
 * Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
 * Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
 * Copyright in any portions created by third parties is as indicated
 * elsewhere herein. All Rights Reserved.
 *
 * Additional Notice Provisions: The application programming interfaces
 * established by SGI in conjunction with the Original Code are The
 * OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
 * April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
 * 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
 * Window System(R) (Version 1.3), released October 19, 1998. This software
 * was created using the OpenGL(R) version 1.2.1 Sample Implementation
 * published by SGI, but has not been independently verified as being
 * compliant with the OpenGL(R) version 1.2.1 Specification.
 *
 */

#include "GL/freeglut_std.h"
#include <GL/freeglut.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

#ifdef __APPLE__
#include "common.h"
#endif

/* report GL errors, if any, to stderr */
void checkError( const char *functionName )
{
    GLenum error;
    while ( ( error = glGetError( ) ) != GL_NO_ERROR ) {
        fprintf( stderr, "GL error 0x%X detected in %s\n", error, functionName );
    }
}

/* vertex array data for a colored 2D triangle, consisting of RGB color values
   and XY coordinates */
const GLfloat varray[] = {
    1.0f,
    0.0f,
    0.0f, /* red */
    5.0f,
    5.0f, /* lower left */

    0.0f,
    1.0f,
    0.0f, /* green */
    25.0f,
    5.0f, /* lower right */

    0.0f,
    0.0f,
    1.0f, /* blue */
    5.0f,
    25.0f /* upper left */
};

/* ISO C somehow enforces this silly use of 'enum' for compile-time constants */
enum {
    numColorComponents  = 3,
    numVertexComponents = 2,
    stride              = sizeof( GLfloat ) * ( numColorComponents + numVertexComponents ),
    numElements         = sizeof( varray ) / stride
};

void dumpInfo( void )
{
    printf( "Vendor: %s\n", glGetString( GL_VENDOR ) );
    printf( "Renderer: %s\n", glGetString( GL_RENDERER ) );
    printf( "Version: %s\n", glGetString( GL_VERSION ) );
    printf( "GLSL: %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION ) );
    checkError( "dumpInfo" );
}
float deg_per_frame = 0.0;

void displayCB( void )
{
    // Make the context current if it exists
    const char *fps = get_fps( 5e9 );
    if ( fps ) {
        printf( "%s\n", fps );
    }

    glClearColor( 0.1, 0.2, 0.3, 1.0 );
    glClear( GL_COLOR_BUFFER_BIT );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity( );
    glRotatef( deg_per_frame, 0, 0, 1 );
    deg_per_frame += 0.1;

    glBegin( GL_TRIANGLES );
    glColor3f( 1.0, 0.0, 0.0 );
    glVertex2d( 0.0, 0.5 );
    glColor3f( 0.0, 1.0, 0.0 );
    glVertex2d( 0.5, -0.5 );
    glColor3f( 0.0, 0.0, 1.0 );
    glVertex2d( -0.5, -0.5 );
    glEnd( );

    glutSwapBuffers( );
    //   glutPostRedisplay();
}

void reshape( int w, int h )
{
    printf( "reshape, width: %d, height: %d\n", w, h );
    glViewport( 0, 0, (GLsizei)w, (GLsizei)h );

    double aspect = (double)w / h;
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );
    glOrtho( -aspect, aspect, -1.0, 1.0, -1.0, 1.0 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity( );
    glutPostRedisplay( );
    checkError( "reshape" );
}

void keyboard( unsigned char key, int x, int y )
{
    printf( "Key  pressed: %d, '%c', at (%d, %d)\n", key, key, x, y );
    switch ( key ) {
    case 27:
        exit( 0 );
        break;
    }
}

void keyboardUp( unsigned char key, int x, int y )
{
    printf( "Key released: %d, '%c', at (%d, %d)\n", key, key, x, y );
}

void special( int key, int x, int y )
{
    printf( "Special key pressed: %d at (%d, %d)\n", key, x, y );
}

void specialUp( int key, int x, int y )
{
    printf( "Special key released: %d at (%d, %d)\n", key, x, y );
}

void passiveMotion( int x, int y )
{
    printf( "\rMouse moved to (%d, %d)", x, y );
    fflush( stdout );
}

static int mButton = -1;

const char *mButtonName( int button )
{
    return button == GLUT_LEFT_BUTTON     ? "left"
           : button == GLUT_MIDDLE_BUTTON ? "middle"
           : button == GLUT_RIGHT_BUTTON  ? "right"
                                          : "???";
}

void motion( int x, int y )
{
    printf( "%c%s mouse dragged to (%d, %d)\n", *mButtonName( mButton ) - 0x20, mButtonName( mButton ) + 1, x, y );
}

void mouse( int button, int state, int x, int y )
{
    printf(
        "Button %d (%s) is %s at (%d, %d)\n", button, mButtonName( button ), state == GLUT_DOWN ? "down" : "up", x, y );

    if ( state == GLUT_DOWN ) {
        mButton = button;
    }
    else {
        mButton = -1;
    }
}

void mouseWheel( int wheel, int direction, int x, int y )
{
    static int count[2];
    count[wheel] += direction > 0 ? 1 : -1;
    printf( "Wheel number %d is spun %s at (%d, %d), count: x:%d, y:%d\n",
        wheel,
        direction > 0 ? "up" : "down",
        x,
        y,
        count[1],
        count[0] );
}

void timer( int value )
{
    glutTimerFunc( 16, timer, value + 1 );
    glutPostRedisplay( );
}

int main( int argc, char **argv )
{
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB );
    glutInitWindowSize( 800, 600 );
    glutInitWindowPosition( 100, 100 );
    glutCreateWindow( argv[0] );
    dumpInfo( );
    // init ();
    glutDisplayFunc( displayCB );
    glutReshapeFunc( reshape );
    //  glutIdleFunc(displayCB);
    glutKeyboardFunc( keyboard );
    glutKeyboardUpFunc( keyboardUp );
    glutSpecialFunc( special );
    glutSpecialUpFunc( specialUp );
    glutPassiveMotionFunc( passiveMotion );
    glutMotionFunc( motion );
    glutMouseFunc( mouse );
    glutMouseWheelFunc( mouseWheel );
    glutTimerFunc( 16, timer, 0 );
    // glutPostRedisplay(); // Trigger an initial redraw

    // while(1);
    glutMainLoop( );
    return 0;
}
