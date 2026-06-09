#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#  include <io.h>
#  define isatty _isatty
#  define fileno _fileno
#else
#  include <unistd.h>
#endif

#ifdef USE_GLUT
#  include <GLUT/glut.h>
#else
#  include <GL/freeglut.h>
#endif

#ifndef GL_SAMPLES
#define GL_SAMPLES 0x80A9
#endif

#ifndef GLUT_AUX
#define GLUT_AUX 0
#endif

static int g_use_color = 0;

static const char *ansi_red(void)   { return g_use_color ? "\033[1;31m" : ""; }
static const char *ansi_green(void) { return g_use_color ? "\033[1;32m" : ""; }
static const char *ansi_reset(void) { return g_use_color ? "\033[0m"    : ""; }

static void print_help( const char *progname )
{
    printf(
        "Usage: %s [display-string]\n"
        "       %s --self-test\n"
        "       %s --self-test-summary\n"
        "       %s --display-mode-reset-test\n"
        "       %s --help\n"
        "\n"
        "Probe glutInitDisplayString() to determine whether a display mode is\n"
        "possible and report the resulting GL pixel format attributes.\n"
        "\n"
        "Arguments:\n"
        "  display-string             Display string to probe (e.g. \"rgb double depth>=16\").\n"
        "  NULL                       Probe with a NULL display string (falls back to the\n"
        "                             mode set by glutInitDisplayMode).\n"
        "\n"
        "Options:\n"
        "  --self-test                Run the built-in test suite (detail mode).\n"
        "  --self-test-summary        Run the built-in test suite (summary mode).\n"
        "  --display-mode-reset-test  Verify that glutInitDisplayMode resets the display\n"
        "                             string state set by a prior glutInitDisplayString call.\n"
        "  --help, -h                 Show this help text.\n"
        "\n"
        "Exit status:\n"
        "  0   Display mode is possible (or all self-test cases passed).\n"
        "  1   Display mode is not possible (or one or more test cases failed).\n",
        progname, progname, progname, progname, progname );
}

typedef enum ProbeCaseType {
    PROBE_CASE_TYPE_VALID,
    PROBE_CASE_TYPE_INVALID,
    PROBE_CASE_TYPE_PLATFORM_SPECIFIC,
} ProbeCaseType;

typedef struct ProbeCase {
    const char *name;
    const char *displayString;
    ProbeCaseType type;
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

static void format_col_int( char *buf, int val, int width )
{
    if ( val >= 0 ) {
        char fmt[16];
        sprintf( fmt, "%%%dd", width );
        sprintf( buf, fmt, val );
    } else {
        int i;
        for ( i = 0; i < width - 1; i++ ) {
            buf[i] = ' ';
        }
        buf[width - 1] = '.';
        buf[width] = '\0';
    }
}

static void format_col_hex( char *buf, int val, int width )
{
    if ( val >= 0 ) {
        char fmt[16];
        sprintf( fmt, "0x%%0%dx", width - 2 );
        sprintf( buf, fmt, val );
    } else {
        int i;
        for ( i = 0; i < width; i++ ) {
            buf[i] = ' ';
        }
        buf[width / 2] = '.';
        buf[width] = '\0';
    }
}

static void print_probe_result_table_header( void )
{
    printf( "    visual  x   bf lv rg d st  colorbuffer  sr ax dp st accumbuffer  ms  sw cav\n" );
    printf( "  id dep cl sp  sz l  ci b ro  r  g  b  a F gb bf th cl  r  g  b  a ns b ap eat\n" );
    printf( "----------------------------------------------------------------------------\n" );
}

static void print_probe_result_summary( const ProbeCase *probeCase, const char *displayString, const ProbeResult *result )
{
    int valid = result->possible;
    int dep = -1;

    char id_str[16];
    char dep_str[16];
    const char *cl_str = " .";
    char sp_str[16];
    char sz_str[16];
    char l_str[16];
    char ci_str[4];
    char b_char = '.';
    char ro_char = '.';
    char r_str[16];
    char g_str[16];
    char b_str[16];
    char a_str[16];
    char float_char = '.';
    char srgb_char = '.';
    char aux_str[16];
    char depth_str[16];
    char stencil_str[16];
    char ar_str[16];
    char ag_str[16];
    char ab_str[16];
    char aa_str[16];
    char ns_str[16];
    char ms_b_str[16];
    char swap_char = '.';
    const char *caveat = "None";

    if ( valid ) {
        dep = 0;
        if ( result->redBits >= 0 ) dep += result->redBits;
        if ( result->greenBits >= 0 ) dep += result->greenBits;
        if ( result->blueBits >= 0 ) dep += result->blueBits;
        if ( result->alphaBits >= 0 ) dep += result->alphaBits;
        cl_str = ( result->rgba == 1 ) ? "tc" : "ci";
        b_char = ( result->doublebuffer == 1 ) ? 'y' : '.';
        ro_char = ( result->stereo == 1 ) ? 'y' : '.';
    }

    format_col_hex( id_str, valid ? result->formatId : -1, 5 );
    format_col_int( dep_str, dep, 2 );
    format_col_int( sp_str, -1, 2 );
    format_col_int( sz_str, dep, 3 );
    format_col_int( l_str, valid ? 0 : -1, 2 );

    if ( valid ) {
        ci_str[0] = ( result->rgba == 1 ) ? 'r' : ' ';
        ci_str[1] = ( result->rgba == 0 ) ? 'c' : ' ';
        ci_str[2] = '\0';
    } else {
        strcpy( ci_str, "  " );
    }

    format_col_int( r_str, valid ? result->redBits : -1, 2 );
    format_col_int( g_str, valid ? result->greenBits : -1, 2 );
    format_col_int( b_str, valid ? result->blueBits : -1, 2 );
    format_col_int( a_str, valid ? result->alphaBits : -1, 2 );

    format_col_int( aux_str, valid ? result->auxBuffers : -1, 2 );
    format_col_int( depth_str, valid ? result->depthBits : -1, 2 );
    format_col_int( stencil_str, valid ? result->stencilBits : -1, 2 );

    format_col_int( ar_str, valid ? result->accumRedBits : -1, 2 );
    format_col_int( ag_str, valid ? result->accumGreenBits : -1, 2 );
    format_col_int( ab_str, valid ? result->accumBlueBits : -1, 2 );
    format_col_int( aa_str, valid ? result->accumAlphaBits : -1, 2 );

    int ns = -1;
    if ( valid ) {
        ns = result->samples >= 0 ? result->samples : ( result->numSamples >= 0 ? result->numSamples : -1 );
    }
    format_col_int( ns_str, ns, 2 );
    format_col_int( ms_b_str, ( ns > 0 ) ? 1 : ( ( ns == 0 ) ? 0 : -1 ), 1 );

    printf( "%s %s %s %s %s %s %s %c %c  %s %s %s %s %c  %c %s %s %s",
            id_str, dep_str, cl_str, sp_str, sz_str, l_str,
            ci_str, b_char, ro_char,
            r_str, g_str, b_str, a_str,
            float_char, srgb_char,
            aux_str, depth_str, stencil_str );

    printf( " %s %s %s %s %s %s %c  %s",
            ar_str, ag_str, ab_str, aa_str,
            ns_str, ms_b_str, swap_char, caveat );

    if ( probeCase ) {
        const char *status = "platform";
        const char *sc     = "";
        const char *sr     = "";

        if ( probeCase->type != PROBE_CASE_TYPE_PLATFORM_SPECIFIC ) {
            int expected = ( probeCase->type == PROBE_CASE_TYPE_VALID );
            if ( expected == result->possible ) {
                status = "pass";
                sc     = ansi_green( );
            } else {
                status = "fail";
                sc     = ansi_red( );
            }
            sr = ansi_reset( );
        }
        printf( "  %-22s (%s%s%s)  \"%s\"", probeCase->name, sc, status, sr, displayString ? displayString : "" );
    } else {
        if ( displayString ) {
            printf( "  \"%s\"", displayString );
        }
    }
    printf( "\n" );
}

static int probe_display_string( int collectWindowDetails, ProbeResult *result )
{
    init_probe_result( result );

    result->possible = glutGet( GLUT_DISPLAY_MODE_POSSIBLE );

    if ( !result->possible || !collectWindowDetails )
        return result->possible;

    result->window       = glutCreateWindow( "dstrprobe" );
    result->formatId     =
#ifdef USE_GLUT
                           -1;
#else
                           glutGet( GLUT_WINDOW_FORMAT_ID );
#endif
    result->doublebuffer = glutGet( GLUT_WINDOW_DOUBLEBUFFER );
    result->rgba         = glutGet( GLUT_WINDOW_RGBA );
    result->stereo       = glutGet( GLUT_WINDOW_STEREO );
    result->numSamples   =
#ifdef USE_GLUT
                           -1;
#else
                           glutGet( GLUT_WINDOW_NUM_SAMPLES );
#endif
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
#ifndef USE_GLUT
    glutMainLoopEvent( );
#endif

    return result->possible;
}

static int run_self_test( int summaryMode )
{
    static const ProbeCase cases[] = {
        { "rgb_single", "rgb", PROBE_CASE_TYPE_VALID, "Baseline single-buffered RGB configuration" },
        { "rgb_double", "rgb double", PROBE_CASE_TYPE_VALID, "Baseline double-buffered RGB configuration" },
        { "basic_rgb_depth", "rgb double depth>=12", PROBE_CASE_TYPE_VALID, "Portable RGB double-buffered depth request" },
        { "rgba_alpha_depth", "rgba alpha>=1 depth>=12", PROBE_CASE_TYPE_VALID, "Explicit alpha plus depth request" },
        { "rgba_depth_stencil", "rgba double depth stencil", PROBE_CASE_TYPE_PLATFORM_SPECIFIC,
            "Bare stencil token should request a stencil-capable format when the platform supports one" },
        { "rgba8_depth24", "red>=8 green>=8 blue>=8 alpha>=8 depth>=24", PROBE_CASE_TYPE_VALID, "Typical modern RGBA8 plus depth configuration" },
        { "depth_eq_16", "depth=16", PROBE_CASE_TYPE_PLATFORM_SPECIFIC, "Platform-dependent exact 16-bit depth availability" },
        { "depth_neq_16", "depth!=16", PROBE_CASE_TYPE_PLATFORM_SPECIFIC, "Useful for checking NEQ comparator handling across implementations" },
        { "samples_default", "samples", PROBE_CASE_TYPE_PLATFORM_SPECIFIC, "Platform-dependent multisample default request" },
        { "samples_eq_4", "samples=4", PROBE_CASE_TYPE_PLATFORM_SPECIFIC, "Platform-dependent exact multisample count request" },
        { "samples_lte_4", "samples<=4", PROBE_CASE_TYPE_PLATFORM_SPECIFIC, "Comparator test for multisample selection" },
        { "stencil_samples_combo", "stencil~2 rgb double depth>=16 samples", PROBE_CASE_TYPE_PLATFORM_SPECIFIC,
            "Man-page style mixed comparator request; sample count preference varies" },
        { "stereo", "stereo", PROBE_CASE_TYPE_PLATFORM_SPECIFIC, "Stereo support depends on hardware and driver stack" },
        { "acca", "acca", PROBE_CASE_TYPE_PLATFORM_SPECIFIC, "Accumulation buffers are platform-dependent and often unavailable on modern systems" },
        { "auxbufs_eq_1", "auxbufs=1", PROBE_CASE_TYPE_PLATFORM_SPECIFIC, "Aux buffer availability varies widely by backend" },
        { "impossible_depth", "rgb depth>256", PROBE_CASE_TYPE_INVALID, "Strict impossible comparator case" },
        { "impossible_alpha", "rgba alpha>64", PROBE_CASE_TYPE_INVALID, "Strict impossible comparator case" },
        { "bare_value_tokens", "rgb double depth", PROBE_CASE_TYPE_VALID,
            "Bare value tokens must resolve to documented defaults (regression: X11 treated unspecified criteria as a hard fail)" },
        { "srgb_token", "rgb double depth>=16 srgb", PROBE_CASE_TYPE_PLATFORM_SPECIFIC,
            "freeglut sRGB extension token; always available on macOS, GLX_ARB_framebuffer_sRGB dependent on X11 (regression: Cocoa once aborted on GLUT_SRGB)" },
        { "captionless_token", "rgba depth captionless", PROBE_CASE_TYPE_PLATFORM_SPECIFIC,
            "freeglut captionless extension token; window-decoration support is platform-dependent" },
        { "acca_eq_16", "acca=16", PROBE_CASE_TYPE_PLATFORM_SPECIFIC,
            "Exact accumulation precision: must match exactly, so impossible where the driver's accum size is fixed (e.g. 32-bit on macOS)" },
        { "acca_min_16", "acca~16", PROBE_CASE_TYPE_PLATFORM_SPECIFIC,
            "'~' means >=value preferring less: satisfied by any accum >= 16, including the fixed 32-bit accum on macOS" },
        { "complex_acca_eq_combo",
            "rgba double depth~24 samples=4 alpha acca=16 auxbufs~2 slow=0 buffer stencil", PROBE_CASE_TYPE_PLATFORM_SPECIFIC,
            "Mixed comparators; the exact acca=16 makes this impossible on macOS (driver-fixed 32-bit accum) -- prefer acca~16 for a portable request" },
        { "complex_acca_min_combo",
            "rgba double depth~24 samples=4 alpha acca~16 auxbufs~2 slow=0 buffer stencil", PROBE_CASE_TYPE_PLATFORM_SPECIFIC,
            "Portable form of the mixed-comparator combo using acca~16; satisfied wherever accum >= 16" },
    };
    int failures = 0;
    int i;

    if ( summaryMode ) {
        print_probe_result_table_header( );
    }

    for ( i = 0; i < (int)( sizeof( cases ) / sizeof( cases[0] ) ); i++ ) {
        ProbeResult result;

        glutInitDisplayString( cases[i].displayString );
        probe_display_string( summaryMode, &result );

        if ( summaryMode ) {
            print_probe_result_summary( &cases[i], cases[i].displayString, &result );
        }
        else {
            print_probe_result_detail( cases[i].displayString, &result );

            if ( cases[i].type != PROBE_CASE_TYPE_PLATFORM_SPECIFIC ) {
                int expected = ( cases[i].type == PROBE_CASE_TYPE_VALID );
                int mismatch = result.possible != expected;
                printf( "case=%s expected=%d actual=%s%d%s\n",
                    cases[i].name,
                    expected,
                    mismatch ? ansi_red( ) : ansi_green( ),
                    result.possible,
                    mismatch ? ansi_reset( ) : ansi_reset( ) );
            } else {
                printf( "case=%s expected=platform actual=%d\n", cases[i].name, result.possible );
            }

            if ( cases[i].note )
                printf( "note=%s\n", cases[i].note );

            if ( result.possible ) {
                probe_display_string( 1, &result );
                print_probe_result_detail( cases[i].displayString, &result );
            }
        }

        if ( cases[i].type != PROBE_CASE_TYPE_PLATFORM_SPECIFIC ) {
            int expected = ( cases[i].type == PROBE_CASE_TYPE_VALID );
            if ( result.possible != expected ) {
                failures++;
                continue;
            }
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

    g_use_color = isatty( fileno( stdout ) );

    if ( displayString &&
         ( strcmp( displayString, "--help" ) == 0 || strcmp( displayString, "-h" ) == 0 ) ) {
        print_help( argv[0] );
        return 0;
    }

    glutInit( &argc, argv );
#ifndef USE_GLUT
    glutSetOption( GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS );
#endif
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
        print_probe_result_table_header( );
        print_probe_result_summary( NULL, displayString, &result );
        return result.possible ? 0 : 1;
    }
}
