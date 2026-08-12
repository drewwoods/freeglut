/*
 * pixel_store_check.c -- correctness companion to bitmap_bench.
 *
 * Whichever strategy src/fg_font.c uses to preserve the GL_UNPACK_* pixel-store
 * state, it must leave that state exactly as it found it.  Set all six values to
 * non-defaults, draw with each bitmap-font entry point, and read them back.
 *
 * Exits 0 if every value survived, 1 otherwise, so it can be run for both
 * settings of FREEGLUT_BITMAP_PIXEL_STORE:
 *
 *   for s in clientattrib getset; do
 *       FREEGLUT_BITMAP_PIXEL_STORE=$s ./bin/pixel_store_check || echo "$s FAILED"
 *   done
 */
#include <stdio.h>
#include <stdlib.h>
#include <GL/freeglut.h>

static const GLenum pnames[6] = {
    GL_UNPACK_SWAP_BYTES, GL_UNPACK_LSB_FIRST, GL_UNPACK_ROW_LENGTH,
    GL_UNPACK_SKIP_ROWS,  GL_UNPACK_SKIP_PIXELS, GL_UNPACK_ALIGNMENT
};
static const char *names[6] = {
    "SWAP_BYTES", "LSB_FIRST", "ROW_LENGTH", "SKIP_ROWS", "SKIP_PIXELS", "ALIGNMENT"
};
static const GLint want[6] = { GL_TRUE, GL_TRUE, 7, 3, 2, 4 };

static int check( const char *what )
{
    int i, bad = 0;
    for( i = 0; i < 6; i++ ) {
        GLint got = -1;
        glGetIntegerv( pnames[i], &got );
        if( got != want[i] ) {
            printf( "  FAIL %s: %s = %d, expected %d\n", what, names[i], got, want[i] );
            bad = 1;
        }
    }
    if( !bad ) printf( "  ok   %s\n", what );
    return bad;
}

static void display( void )
{
    int i, bad = 0;
    for( i = 0; i < 6; i++ ) glPixelStorei( pnames[i], want[i] );

    glRasterPos2f( 5.0f, 5.0f );
    glutBitmapCharacter( GLUT_BITMAP_8_BY_13, 'A' );
    bad |= check( "after glutBitmapCharacter" );

    glRasterPos2f( 5.0f, 5.0f );
    glutBitmapString( GLUT_BITMAP_8_BY_13, (const unsigned char *)"hello\nworld" );
    bad |= check( "after glutBitmapString" );

    exit( bad );
}

int main( int argc, char **argv )
{
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB );
    glutInitWindowSize( 200, 200 );
    glutCreateWindow( "freeglut pixel_store_check" );
    glutDisplayFunc( display );
    glutMainLoop( );
    return 0;
}
