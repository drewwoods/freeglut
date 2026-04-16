#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/freeglut.h>

#ifndef GL_SAMPLES
#define GL_SAMPLES 0x80A9
#endif

typedef struct ProbeCase {
    const char *name;
    const char *displayString;
    int         expectedPossible;
    const char *note;
} ProbeCase;

typedef struct ProbeResult {
    int possible;
    int window;
    int formatId;
    int doublebuffer;
    int rgba;
    int stereo;
    int numSamples;
    int redBits;
    int greenBits;
    int blueBits;
    int alphaBits;
    int depthBits;
    int stencilBits;
    int accumRedBits;
    int accumGreenBits;
    int accumBlueBits;
    int accumAlphaBits;
    int auxBuffers;
    int samples;
} ProbeResult;

static int query_gl_int( GLenum pname )
{
    GLint value = 0;

    glGetIntegerv( pname, &value );
    return value;
}

static void init_probe_result( ProbeResult *result )
{
    result->possible       = 0;
    result->window         = -1;
    result->formatId       = -1;
    result->doublebuffer   = -1;
    result->rgba           = -1;
    result->stereo         = -1;
    result->numSamples     = -1;
    result->redBits        = -1;
    result->greenBits      = -1;
    result->blueBits       = -1;
    result->alphaBits      = -1;
    result->depthBits      = -1;
    result->stencilBits    = -1;
    result->accumRedBits   = -1;
    result->accumGreenBits = -1;
    result->accumBlueBits  = -1;
    result->accumAlphaBits = -1;
    result->auxBuffers     = -1;
    result->samples        = -1;
}

static void print_probe_result_detail( const char *displayString, const ProbeResult *result )
{
    printf( "display_string=%s\n", displayString ? displayString : "(null)" );
    printf( "possible=%d\n", result->possible );

    if ( result->window < 0 )
        return;

    printf( "window=%d\n", result->window );
    printf( "format_id=%d\n", result->formatId );
    printf( "doublebuffer=%d\n", result->doublebuffer );
    printf( "rgba=%d\n", result->rgba );
    printf( "stereo=%d\n", result->stereo );
    printf( "num_samples=%d\n", result->numSamples );
    printf( "red_bits=%d\n", result->redBits );
    printf( "green_bits=%d\n", result->greenBits );
    printf( "blue_bits=%d\n", result->blueBits );
    printf( "alpha_bits=%d\n", result->alphaBits );
    printf( "depth_bits=%d\n", result->depthBits );
    printf( "stencil_bits=%d\n", result->stencilBits );
    printf( "accum_red_bits=%d\n", result->accumRedBits );
    printf( "accum_green_bits=%d\n", result->accumGreenBits );
    printf( "accum_blue_bits=%d\n", result->accumBlueBits );
    printf( "accum_alpha_bits=%d\n", result->accumAlphaBits );
    printf( "aux_buffers=%d\n", result->auxBuffers );
    printf( "samples=%d\n", result->samples );
}

static void print_summary_value( const char *label, int value )
{
    if ( value >= 0 )
        printf( " %s=%d", label, value );
    else
        printf( " %s=na", label );
}

static void print_probe_result_summary( const ProbeCase *probeCase, const ProbeResult *result )
{
    const char *status = "platform";

    if ( probeCase->expectedPossible >= 0 )
        status = probeCase->expectedPossible == result->possible ? "pass" : "fail";

    printf( "case=%s status=%s expected=", probeCase->name, status );
    if ( probeCase->expectedPossible >= 0 )
        printf( "%d", probeCase->expectedPossible );
    else
        printf( "platform" );

    printf( " possible=%d display_string=\"%s\"",
        result->possible,
        probeCase->displayString ? probeCase->displayString : "(null)" );
    print_summary_value( "format_id", result->formatId );
    print_summary_value( "doublebuffer", result->doublebuffer );
    print_summary_value( "rgba", result->rgba );
    print_summary_value( "stereo", result->stereo );
    print_summary_value( "num_samples", result->numSamples );
    print_summary_value( "red_bits", result->redBits );
    print_summary_value( "green_bits", result->greenBits );
    print_summary_value( "blue_bits", result->blueBits );
    print_summary_value( "alpha_bits", result->alphaBits );
    print_summary_value( "depth_bits", result->depthBits );
    print_summary_value( "stencil_bits", result->stencilBits );
    print_summary_value( "accum_red_bits", result->accumRedBits );
    print_summary_value( "accum_green_bits", result->accumGreenBits );
    print_summary_value( "accum_blue_bits", result->accumBlueBits );
    print_summary_value( "accum_alpha_bits", result->accumAlphaBits );
    print_summary_value( "aux_buffers", result->auxBuffers );
    print_summary_value( "samples", result->samples );
    printf( "\n" );
}

static int probe_display_string( int collectWindowDetails, ProbeResult *result )
{
    init_probe_result( result );

    result->possible = glutGet( GLUT_DISPLAY_MODE_POSSIBLE );

    if ( !result->possible || !collectWindowDetails )
        return result->possible;

    result->window       = glutCreateWindow( "dstrprobe" );
    result->formatId     = glutGet( GLUT_WINDOW_FORMAT_ID );
    result->doublebuffer = glutGet( GLUT_WINDOW_DOUBLEBUFFER );
    result->rgba         = glutGet( GLUT_WINDOW_RGBA );
    result->stereo       = glutGet( GLUT_WINDOW_STEREO );
    result->numSamples   = glutGet( GLUT_WINDOW_NUM_SAMPLES );
    result->redBits      = query_gl_int( GL_RED_BITS );
    result->greenBits    = query_gl_int( GL_GREEN_BITS );
    result->blueBits     = query_gl_int( GL_BLUE_BITS );
    result->alphaBits    = query_gl_int( GL_ALPHA_BITS );
    result->depthBits    = query_gl_int( GL_DEPTH_BITS );
    result->stencilBits  = query_gl_int( GL_STENCIL_BITS );
    result->accumRedBits = query_gl_int( GL_ACCUM_RED_BITS );
    result->accumGreenBits = query_gl_int( GL_ACCUM_GREEN_BITS );
    result->accumBlueBits  = query_gl_int( GL_ACCUM_BLUE_BITS );
    result->accumAlphaBits = query_gl_int( GL_ACCUM_ALPHA_BITS );
    result->auxBuffers     = query_gl_int( GL_AUX_BUFFERS );
    result->samples        = query_gl_int( GL_SAMPLES );

    glutDestroyWindow( result->window );
    glutMainLoopEvent( );

    return result->possible;
}

static int run_self_test( int summaryMode )
{
    static const ProbeCase cases[] = {
        { "rgb_single", "rgb", 1, "Baseline single-buffered RGB configuration" },
        { "rgb_double", "rgb double", 1, "Baseline double-buffered RGB configuration" },
        { "basic_rgb_depth", "rgb double depth>=12", 1, "Portable RGB double-buffered depth request" },
        { "rgba_alpha_depth", "rgba alpha>=1 depth>=12", 1, "Explicit alpha plus depth request" },
        { "rgba8_depth24", "red>=8 green>=8 blue>=8 alpha>=8 depth>=24", 1, "Typical modern RGBA8 plus depth configuration" },
        { "depth_eq_16", "depth=16", -1, "Platform-dependent exact 16-bit depth availability" },
        { "depth_neq_16", "depth!=16", -1, "Useful for checking NEQ comparator handling across implementations" },
        { "samples_default", "samples", -1, "Platform-dependent multisample default request" },
        { "samples_eq_4", "samples=4", -1, "Platform-dependent exact multisample count request" },
        { "samples_lte_4", "samples<=4", -1, "Comparator test for multisample selection" },
        { "stencil_samples_combo", "stencil~2 rgb double depth>=16 samples", -1,
            "Man-page style mixed comparator request; sample count preference varies" },
        { "stereo", "stereo", -1, "Stereo support depends on hardware and driver stack" },
        { "acca", "acca", -1, "Accumulation buffers are platform-dependent and often unavailable on modern systems" },
        { "auxbufs_eq_1", "auxbufs=1", -1, "Aux buffer availability varies widely by backend" },
        { "impossible_depth", "rgb depth>256", 0, "Strict impossible comparator case" },
        { "impossible_alpha", "rgba alpha>64", 0, "Strict impossible comparator case" },
    };
    int failures = 0;
    int i;

    for ( i = 0; i < (int)( sizeof( cases ) / sizeof( cases[0] ) ); i++ ) {
        ProbeResult result;

        glutInitDisplayString( cases[i].displayString );
        probe_display_string( summaryMode, &result );

        if ( summaryMode ) {
            print_probe_result_summary( &cases[i], &result );
        }
        else {
            print_probe_result_detail( cases[i].displayString, &result );

            if ( cases[i].expectedPossible >= 0 )
                printf( "case=%s expected=%d actual=%d\n", cases[i].name, cases[i].expectedPossible, result.possible );
            else
                printf( "case=%s expected=platform actual=%d\n", cases[i].name, result.possible );

            if ( cases[i].note )
                printf( "note=%s\n", cases[i].note );

            if ( result.possible ) {
                probe_display_string( 1, &result );
                print_probe_result_detail( cases[i].displayString, &result );
            }
        }

        if ( cases[i].expectedPossible >= 0 && result.possible != cases[i].expectedPossible ) {
            failures++;
            continue;
        }
    }

    printf( "self_test_failures=%d\n", failures );
    return failures ? 1 : 0;
}

static int run_display_mode_reset_test( void )
{
    unsigned int portableDisplayMode = GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH;
    unsigned int requestedDisplayMode = GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL | GLUT_AUX | GLUT_MULTISAMPLE;
    ProbeResult  portableBaselineResult;
    ProbeResult  portableResetResult;
    ProbeResult  requestedBaselineResult;
    ProbeResult  requestedResetResult;

    glutInitDisplayMode( portableDisplayMode );
    probe_display_string( 0, &portableBaselineResult );

    glutInitDisplayString( "rgb depth>256" );
    glutInitDisplayMode( portableDisplayMode );
    probe_display_string( 0, &portableResetResult );

    glutInitDisplayMode( requestedDisplayMode );
    probe_display_string( 0, &requestedBaselineResult );

    glutInitDisplayString( "rgb depth>256" );
    glutInitDisplayMode( requestedDisplayMode );
    probe_display_string( 0, &requestedResetResult );

    printf(
        "display_mode_reset portable_baseline_possible=%d portable_reset_possible=%d portable_display_mode=0x%x requested_baseline_possible=%d requested_reset_possible=%d requested_display_mode=0x%x\n",
        portableBaselineResult.possible,
        portableResetResult.possible,
        portableDisplayMode,
        requestedBaselineResult.possible,
        requestedResetResult.possible,
        requestedDisplayMode );

    return portableBaselineResult.possible == 1 && portableResetResult.possible == 1 &&
           requestedBaselineResult.possible == requestedResetResult.possible ?
               0 :
               1;
}

int main( int argc, char **argv )
{
    const char *displayString = argc > 1 ? argv[1] : NULL;

    glutInit( &argc, argv );
    glutSetOption( GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS );
    glutInitWindowSize( 160, 120 );
    glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );

    if ( displayString && strcmp( displayString, "--self-test" ) == 0 )
        return run_self_test( 0 );

    if ( displayString && strcmp( displayString, "--self-test-summary" ) == 0 )
        return run_self_test( 1 );

    if ( displayString && strcmp( displayString, "--display-mode-reset-test" ) == 0 )
        return run_display_mode_reset_test( );

    if ( displayString && strcmp( displayString, "NULL" ) == 0 )
        displayString = NULL;

    if ( displayString )
        glutInitDisplayString( (char *)displayString );

    {
        ProbeResult result;

        probe_display_string( 1, &result );
        print_probe_result_detail( displayString, &result );
        return result.possible ? 0 : 1;
    }
}
