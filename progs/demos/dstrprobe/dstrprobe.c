#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/freeglut.h>

#ifndef GL_SAMPLES
#define GL_SAMPLES 0x80A9
#endif

static void print_gl_int( const char *label, GLenum pname )
{
    GLint value = 0;
    glGetIntegerv( pname, &value );
    printf( "%s=%d\n", label, value );
}

int main( int argc, char **argv )
{
    const char *displayString = argc > 1 ? argv[1] : NULL;
    int possible;
    int window;

    glutInit( &argc, argv );
    glutSetOption( GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS );
    glutInitWindowSize( 160, 120 );
    glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );

    if( displayString && strcmp( displayString, "NULL" ) == 0 )
        displayString = NULL;

    if( displayString )
        glutInitDisplayString( (char *)displayString );
    else
        glutInitDisplayString( NULL );

    possible = glutGet( GLUT_DISPLAY_MODE_POSSIBLE );
    printf( "display_string=%s\n", displayString ? displayString : "(null)" );
    printf( "possible=%d\n", possible );
    if( !possible )
        return 1;

    window = glutCreateWindow( "dstrprobe" );
    printf( "window=%d\n", window );
    printf( "format_id=%d\n", glutGet( GLUT_WINDOW_FORMAT_ID ) );
    printf( "doublebuffer=%d\n", glutGet( GLUT_WINDOW_DOUBLEBUFFER ) );
    printf( "rgba=%d\n", glutGet( GLUT_WINDOW_RGBA ) );
    printf( "stereo=%d\n", glutGet( GLUT_WINDOW_STEREO ) );
    printf( "num_samples=%d\n", glutGet( GLUT_WINDOW_NUM_SAMPLES ) );
    print_gl_int( "red_bits", GL_RED_BITS );
    print_gl_int( "green_bits", GL_GREEN_BITS );
    print_gl_int( "blue_bits", GL_BLUE_BITS );
    print_gl_int( "alpha_bits", GL_ALPHA_BITS );
    print_gl_int( "depth_bits", GL_DEPTH_BITS );
    print_gl_int( "stencil_bits", GL_STENCIL_BITS );
    print_gl_int( "accum_red_bits", GL_ACCUM_RED_BITS );
    print_gl_int( "accum_green_bits", GL_ACCUM_GREEN_BITS );
    print_gl_int( "accum_blue_bits", GL_ACCUM_BLUE_BITS );
    print_gl_int( "accum_alpha_bits", GL_ACCUM_ALPHA_BITS );
    print_gl_int( "aux_buffers", GL_AUX_BUFFERS );
    print_gl_int( "samples", GL_SAMPLES );

    glutDestroyWindow( window );
    return 0;
}
