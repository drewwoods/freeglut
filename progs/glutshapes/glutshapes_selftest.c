/*
 * glutshapes_selftest.c
 *
 * Functional test for the standalone "glutshapes" library: it drives the
 * freeglut geometry + font code with a raw OSMesa context and *no freeglut
 * windowing whatsoever* (this program links libglutshapes + libOSMesa only).
 *
 * It checks:
 *   1. glutSolidSphere under glRenderMode(GL_FEEDBACK) yields polygon tokens,
 *   2. glutStrokeString under feedback yields line tokens (the font path runs),
 *   3. rendering a sphere into the OSMesa buffer produces a non-blank image.
 *
 * Exits 0 on success, 1 on any failure. Compiled with -DFREEGLUT_OSMESA so
 * freeglut.h and osmesa.h agree on Mesa's GL headers.
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

#include <GL/freeglut.h>      /* glutSolidSphere / glutStrokeString decls + GL types */
#include <GL/glutshapes.h>    /* the standalone config API                          */
#include <GL/osmesa.h>
#include <stdio.h>
#include <stdlib.h>

#define W 96
#define H 96

static int count_tokens( const GLfloat *fb, GLint n, int wanted )
{
    int i = 0, hits = 0;
    while( i < n )
    {
        int tok = (int)fb[ i ];
        if( tok == GL_POLYGON_TOKEN )
        {
            int cnt = (int)fb[ i + 1 ];
            if( tok == wanted ) hits++;
            i += 2 + cnt * 3;                 /* GL_3D: 3 floats / vertex */
        }
        else if( tok == GL_POINT_TOKEN )
        {
            if( tok == wanted ) hits++;
            i += 1 + 3;
        }
        else if( tok == GL_LINE_TOKEN || tok == GL_LINE_RESET_TOKEN )
        {
            if( tok == wanted || ( wanted == GL_LINE_TOKEN &&
                                   tok == GL_LINE_RESET_TOKEN ) )
                hits++;
            i += 1 + 6;
        }
        else
            i += 1;
    }
    return hits;
}

int main( void )
{
    unsigned char *buf = (unsigned char *)malloc( W * H * 4 );
    OSMesaContext ctx;
    int failures = 0;
    long lit = 0, p;

    ctx = OSMesaCreateContextExt( OSMESA_RGBA, 24, 0, 0, NULL );
    if( !ctx || !buf || !OSMesaMakeCurrent( ctx, buf, GL_UNSIGNED_BYTE, W, H ) )
    {
        fprintf( stderr, "FAIL: could not create/bind OSMesa context\n" );
        return 1;
    }
    glViewport( 0, 0, W, H );
    /* OSMesa is single-buffered; pin the draw/read buffer like the freeglut
     * OSMesa backend does, so rasterized output lands in (and reads back from)
     * the one physical buffer. */
    glDrawBuffer( GL_FRONT );
    glReadBuffer( GL_FRONT );

    /* 1) glutSolidSphere feedback -> polygon tokens (GL1.1 fixed-function path). */
    {
        static GLfloat fb[ 1 << 16 ];
        GLint n;
        int polys;
        glFeedbackBuffer( (GLsizei)( sizeof( fb ) / sizeof( fb[ 0 ] ) ), GL_3D, fb );
        glRenderMode( GL_FEEDBACK );
        glutSolidSphere( 0.8, 24, 24 );
        n = glRenderMode( GL_RENDER );
        polys = count_tokens( fb, n, GL_POLYGON_TOKEN );
        printf( "glutSolidSphere feedback: %d values, %d polygon token(s)\n",
                (int)n, polys );
        if( !( n > 0 && polys > 0 ) ) { printf( "  FAIL\n" ); failures++; }
    }

    /* 2) glutStrokeString feedback -> line tokens (the stroke-font path runs).
     * Stroke coordinates are in font units (~0..100), so set an ortho projection
     * that keeps them inside the clip volume; otherwise the (fully clipped)
     * primitives are never fed back. */
    {
        static GLfloat fb[ 1 << 16 ];
        GLint n;
        int lines;
        glMatrixMode( GL_PROJECTION ); glPushMatrix(); glLoadIdentity();
        glOrtho( 0.0, 200.0, -40.0, 160.0, -1.0, 1.0 );
        glMatrixMode( GL_MODELVIEW );  glPushMatrix(); glLoadIdentity();

        glFeedbackBuffer( (GLsizei)( sizeof( fb ) / sizeof( fb[ 0 ] ) ), GL_3D, fb );
        glRenderMode( GL_FEEDBACK );
        glutStrokeString( GLUT_STROKE_ROMAN, (const unsigned char *)"GL" );
        n = glRenderMode( GL_RENDER );

        glMatrixMode( GL_PROJECTION ); glPopMatrix();
        glMatrixMode( GL_MODELVIEW );  glPopMatrix();

        lines = count_tokens( fb, n, GL_LINE_TOKEN );
        printf( "glutStrokeString feedback: %d values, %d line token(s)\n",
                (int)n, lines );
        if( !( n > 0 && lines > 0 ) ) { printf( "  FAIL\n" ); failures++; }
    }

    /* 3) Render a white sphere and confirm the framebuffer is non-blank. The
     * Gallium OSMesa backend renders into an internal resource, so read the
     * finished image back via OSMesaGetColorBuffer (not the raw user pointer). */
    {
        GLint cw = 0, ch = 0, cfmt = 0;
        void *cbuf = NULL;
        glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        glColor3f( 1.0f, 1.0f, 1.0f );
        glutSolidSphere( 0.8, 24, 24 );
        glFinish();
        if( OSMesaGetColorBuffer( ctx, &cw, &ch, &cfmt, &cbuf ) && cbuf )
        {
            unsigned char *cb = (unsigned char *)cbuf;
            for( p = 0; p < (long)cw * ch; ++p )
                if( cb[ p * 4 ] > 24 || cb[ p * 4 + 1 ] > 24 || cb[ p * 4 + 2 ] > 24 )
                    lit++;
        }
        printf( "rendered sphere: %ld non-black pixel(s) of %d\n", lit, (int)( cw * ch ) );
        if( lit < 64 ) { printf( "  FAIL: framebuffer essentially blank\n" ); failures++; }
    }

    OSMesaDestroyContext( ctx );
    free( buf );
    printf( "glutshapes standalone test: %s (%d failure(s))\n",
            failures ? "FAIL" : "PASS", failures );
    return failures ? 1 : 0;
}
