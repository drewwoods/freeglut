/*
 * osmesa_selftest.c
 *
 * Self-test for the freeglut OSMesa (headless, off-screen) backend.
 *
 * It exercises, with no display server / window system:
 *   1. the first-frame callback synthesis (the display callback must fire),
 *   2. rasterization + read-back of a single-buffered window,
 *   3. the GLUT_DOUBLE -> single-buffer draw-buffer downgrade (read-back must
 *      still see the rendered image), and
 *   4. glRenderMode(GL_FEEDBACK) capture of glutSolidTeapot (the property the
 *      gl-repl consumer depends on): a non-zero GL_POLYGON_TOKEN count.
 *
 * Exits 0 on success, 1 on any failure.
 *
 * Copyright (c) 2026 freeglut contributors. All Rights Reserved.
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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <GL/freeglut.h>
#include <GL/osmesa.h>
#include <stdio.h>
#include <stdlib.h>

static int g_failures = 0;
static int g_displayed = 0;

/*
 * Read the centre pixel from the OSMesa colour buffer and compare it (loosely)
 * to the expected RGB. The triangle covers the centre regardless of the
 * OSMESA_Y_UP orientation, so the read-back is orientation-independent.
 */
static void check_centre( int er, int eg, int eb, const char *label )
{
    OSMesaContext ctx = OSMesaGetCurrentContext();
    GLint w = 0, h = 0, fmt = 0;
    void *buf = NULL;
    unsigned char *px;

    if( !ctx || !OSMesaGetColorBuffer( ctx, &w, &h, &fmt, &buf ) || !buf )
    {
        printf( "  [%s] FAIL: OSMesaGetColorBuffer failed\n", label );
        g_failures++;
        return;
    }

    px = (unsigned char *)buf + ( (size_t)( h / 2 ) * w + ( w / 2 ) ) * 4;
    printf( "  [%s] %dx%d centre RGBA = %u,%u,%u,%u\n",
            label, w, h, px[ 0 ], px[ 1 ], px[ 2 ], px[ 3 ] );

    if( abs( (int)px[ 0 ] - er ) > 48 ||
        abs( (int)px[ 1 ] - eg ) > 48 ||
        abs( (int)px[ 2 ] - eb ) > 48 )
    {
        printf( "  [%s] FAIL: expected ~%d,%d,%d\n", label, er, eg, eb );
        g_failures++;
    }
    else
        printf( "  [%s] PASS\n", label );
}

static void draw_triangle( void )
{
    glViewport( 0, 0, glutGet( GLUT_WINDOW_WIDTH ), glutGet( GLUT_WINDOW_HEIGHT ) );
    glClearColor( 0.0f, 0.0f, 1.0f, 1.0f );          /* blue background */
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glColor3f( 1.0f, 0.0f, 0.0f );                   /* red triangle over centre */
    glBegin( GL_TRIANGLES );
        glVertex2f( -0.9f, -0.9f );
        glVertex2f(  0.9f, -0.9f );
        glVertex2f(  0.0f,  0.9f );
    glEnd();
    glFinish();
}

static void disp_triangle( void )
{
    draw_triangle();
    check_centre( 255, 0, 0, "triangle" );
    g_displayed = 1;
}

static void disp_triangle_double( void )
{
    draw_triangle();
    glutSwapBuffers();                               /* must be a harmless no-op */
    check_centre( 255, 0, 0, "triangle/DOUBLE" );
    g_displayed = 1;
}

static void disp_teapot_feedback( void )
{
    static GLfloat fb[ 1 << 18 ];                    /* 1 MiB; plenty for a teapot */
    GLint n, i = 0, polys = 0;

    glViewport( 0, 0, glutGet( GLUT_WINDOW_WIDTH ), glutGet( GLUT_WINDOW_HEIGHT ) );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glFeedbackBuffer( (GLsizei)( sizeof( fb ) / sizeof( fb[ 0 ] ) ), GL_3D, fb );
    glRenderMode( GL_FEEDBACK );
    glutSolidTeapot( 1.0 );
    n = glRenderMode( GL_RENDER );

    while( i < n )
    {
        int tok = (int)fb[ i ];
        if( tok == GL_POLYGON_TOKEN )
        {
            int cnt = (int)fb[ i + 1 ];
            polys++;
            i += 2 + cnt * 3;                        /* GL_3D: 3 floats / vertex */
        }
        else if( tok == GL_POINT_TOKEN )
            i += 1 + 3;
        else if( tok == GL_LINE_TOKEN || tok == GL_LINE_RESET_TOKEN )
            i += 1 + 6;
        else
            i += 1;
    }

    printf( "  [teapot/FEEDBACK] %d feedback values, %d polygon token(s)\n",
            (int)n, polys );
    if( n > 0 && polys > 0 )
        printf( "  [teapot/FEEDBACK] PASS\n" );
    else
    {
        printf( "  [teapot/FEEDBACK] FAIL: no polygon tokens captured\n" );
        g_failures++;
    }
    g_displayed = 1;
}

static void run( unsigned int mode, void ( *dispfn )( void ), const char *name )
{
    int win, i;

    g_displayed = 0;
    glutInitDisplayMode( mode );
    glutInitWindowSize( 64, 64 );
    win = glutCreateWindow( name );
    glutDisplayFunc( dispfn );

    /* Pump the work list; the synthesized first frame must fire the display
     * callback. (No real glutMainLoop needed for a finite headless test.) */
    for( i = 0; i < 200 && !g_displayed; ++i )
        glutMainLoopEvent();

    if( !g_displayed )
    {
        printf( "  [%s] FAIL: display callback never fired\n", name );
        g_failures++;
    }

    glutDestroyWindow( win );
    glutMainLoopEvent();                             /* process the close */
}

int main( int argc, char **argv )
{
    glutInit( &argc, argv );
    printf( "freeglut OSMesa backend self-test\n" );

    run( GLUT_RGBA | GLUT_DEPTH,                disp_triangle,        "triangle" );
    run( GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE,  disp_triangle_double, "triangle/DOUBLE" );
    run( GLUT_RGBA | GLUT_DEPTH,                disp_teapot_feedback, "teapot/FEEDBACK" );

    printf( "OVERALL: %s (%d failure(s))\n",
            g_failures ? "FAIL" : "PASS", g_failures );
    return g_failures ? 1 : 0;
}
