/*
 * dstrprobe.c
 *
 * Probe glutInitDisplayString() semantics.
 *
 * This program is deliberately restricted to the classic GLUT 3.7 API plus
 * plain OpenGL state queries, so the very same source file compiles
 * unmodified against different GLUT implementations and the output can be
 * diffed directly between them:
 *
 *   freeglut:               cc dstrprobe.c -lglut
 *   Apple GLUT.framework:   cc -DUSE_GLUT dstrprobe.c -framework GLUT -framework OpenGL
 *
 * Do not add freeglut-specific calls, tokens, or glutGet() enums here; the
 * only permitted implementation switch is the header include below.
 */

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
#  include <GLUT/glut.h>     /* Apple GLUT.framework */
#else
#  include <GL/glut.h>       /* freeglut (classic GLUT header) */
#endif

/* Fallbacks for older GL headers; all are standard OpenGL enums. */
#ifndef GL_SAMPLES
#define GL_SAMPLES 0x80A9
#endif
#ifndef GL_SAMPLE_BUFFERS
#define GL_SAMPLE_BUFFERS 0x80A8
#endif
#ifndef GL_RGBA_MODE
#define GL_RGBA_MODE 0x0C31
#endif
#ifndef GL_INDEX_BITS
#define GL_INDEX_BITS 0x0D51
#endif

/* GLUT_AUX is a freeglut extension bit; harmless as 0 elsewhere. It is only
 * used by --display-mode-reset-test to build a "kitchen sink" mode mask. */
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
        "possible and report the resulting GL framebuffer attributes.\n"
        "\n"
        "Arguments:\n"
        "  display-string             Display string to probe (e.g. \"rgb double depth>=16\").\n"
        "  NULL                       Probe with no display string (falls back to the\n"
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

/* All framebuffer attributes are queried straight from OpenGL (ground
 * truth), never through implementation-specific glutGet() enums. */
typedef struct ProbeResult {
    int possible;
    int haveDetails;
    int rgbaMode;
    int doublebuffer;
    int stereo;
    int redBits;
    int greenBits;
    int blueBits;
    int alphaBits;
    int indexBits;
    int depthBits;
    int stencilBits;
    int accumRedBits;
    int accumGreenBits;
    int accumBlueBits;
    int accumAlphaBits;
    int auxBuffers;
    int samples;
    int sampleBuffers;
} ProbeResult;

static int query_gl_int( GLenum pname )
{
    GLint value = 0;

    glGetIntegerv( pname, &value );
    return value;
}

static int query_gl_bool( GLenum pname )
{
    GLboolean value = GL_FALSE;

    glGetBooleanv( pname, &value );
    return value ? 1 : 0;
}

static void init_probe_result( ProbeResult *result )
{
    memset( result, 0, sizeof( *result ) );
    result->possible    = 0;
    result->haveDetails = 0;
}

/* Probe the current init state: report whether the display mode is possible
 * and, when requested, create a window and read the resulting framebuffer
 * attributes back from OpenGL. */
static int probe_display_string( int collectWindowDetails, ProbeResult *result )
{
    int window;

    init_probe_result( result );

    result->possible = glutGet( GLUT_DISPLAY_MODE_POSSIBLE );

    if ( !result->possible || !collectWindowDetails )
        return result->possible;

    window = glutCreateWindow( "dstrprobe" );

    result->haveDetails    = 1;
    result->rgbaMode       = query_gl_bool( GL_RGBA_MODE );
    result->doublebuffer   = query_gl_bool( GL_DOUBLEBUFFER );
    result->stereo         = query_gl_bool( GL_STEREO );
    result->redBits        = query_gl_int( GL_RED_BITS );
    result->greenBits      = query_gl_int( GL_GREEN_BITS );
    result->blueBits       = query_gl_int( GL_BLUE_BITS );
    result->alphaBits      = query_gl_int( GL_ALPHA_BITS );
    result->indexBits      = query_gl_int( GL_INDEX_BITS );
    result->depthBits      = query_gl_int( GL_DEPTH_BITS );
    result->stencilBits    = query_gl_int( GL_STENCIL_BITS );
    result->accumRedBits   = query_gl_int( GL_ACCUM_RED_BITS );
    result->accumGreenBits = query_gl_int( GL_ACCUM_GREEN_BITS );
    result->accumBlueBits  = query_gl_int( GL_ACCUM_BLUE_BITS );
    result->accumAlphaBits = query_gl_int( GL_ACCUM_ALPHA_BITS );
    result->auxBuffers     = query_gl_int( GL_AUX_BUFFERS );
    result->samples        = query_gl_int( GL_SAMPLES );
    result->sampleBuffers  = query_gl_int( GL_SAMPLE_BUFFERS );

    glutDestroyWindow( window );

    return result->possible;
}

/* glxinfo-style context banner. Uses a throwaway window with a portable
 * display mode so glGetString() has a current context. */
static void print_gl_info_banner( void )
{
    int window;

    glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
    window = glutCreateWindow( "dstrprobe" );

    printf( "OpenGL vendor string: %s\n",   (const char *)glGetString( GL_VENDOR ) );
    printf( "OpenGL renderer string: %s\n", (const char *)glGetString( GL_RENDERER ) );
    printf( "OpenGL version string: %s\n",  (const char *)glGetString( GL_VERSION ) );
    printf( "\n" );

    glutDestroyWindow( window );
}

static void print_probe_result_detail( const char *displayString, const ProbeResult *result )
{
    printf( "display_string=%s\n", displayString ? displayString : "(null)" );
    printf( "possible=%d\n", result->possible );

    if ( !result->haveDetails )
        return;

    printf( "rgba_mode=%d\n", result->rgbaMode );
    printf( "doublebuffer=%d\n", result->doublebuffer );
    printf( "stereo=%d\n", result->stereo );
    printf( "red_bits=%d\n", result->redBits );
    printf( "green_bits=%d\n", result->greenBits );
    printf( "blue_bits=%d\n", result->blueBits );
    printf( "alpha_bits=%d\n", result->alphaBits );
    printf( "index_bits=%d\n", result->indexBits );
    printf( "depth_bits=%d\n", result->depthBits );
    printf( "stencil_bits=%d\n", result->stencilBits );
    printf( "accum_red_bits=%d\n", result->accumRedBits );
    printf( "accum_green_bits=%d\n", result->accumGreenBits );
    printf( "accum_blue_bits=%d\n", result->accumBlueBits );
    printf( "accum_alpha_bits=%d\n", result->accumAlphaBits );
    printf( "aux_buffers=%d\n", result->auxBuffers );
    printf( "samples=%d\n", result->samples );
    printf( "sample_buffers=%d\n", result->sampleBuffers );
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

/* One glxinfo-style table row. "id" is the probe row number (GLUT has no
 * portable way to report the underlying visual/pixel-format id). */
static void print_probe_result_summary( const ProbeCase *probeCase, const char *displayString,
                                        const ProbeResult *result, int id )
{
    int valid = result->possible && result->haveDetails;
    int dep = -1;
    int sz = -1;

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
        dep = result->redBits + result->greenBits + result->blueBits + result->alphaBits;
        sz  = result->rgbaMode ? dep : result->indexBits;
        cl_str = result->rgbaMode ? "tc" : "ci";
        b_char = result->doublebuffer ? 'y' : '.';
        ro_char = result->stereo ? 'y' : '.';
    }

    format_col_hex( id_str, valid ? id : -1, 5 );
    format_col_int( dep_str, dep, 2 );
    format_col_int( sp_str, -1, 2 );
    format_col_int( sz_str, sz, 3 );
    format_col_int( l_str, valid ? 0 : -1, 2 );

    if ( valid ) {
        ci_str[0] = result->rgbaMode ? 'r' : ' ';
        ci_str[1] = result->rgbaMode ? ' ' : 'c';
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

    format_col_int( ns_str, valid ? result->samples : -1, 2 );
    format_col_int( ms_b_str, valid ? result->sampleBuffers : -1, 1 );

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

static const ProbeCase g_self_test_cases[] = {
    { "rgb_single", "rgb", PROBE_CASE_TYPE_VALID, "Baseline single-buffered RGB configuration" },
    { "rgb_double", "rgb double", PROBE_CASE_TYPE_VALID, "Baseline double-buffered RGB configuration" },
    { "single", "rgb single", PROBE_CASE_TYPE_VALID,
        "Explicit single token maps to doublebuffer=0 (double-buffered visuals may serve as single)" },
    { "basic_rgb_depth", "rgb double depth>=12", PROBE_CASE_TYPE_VALID, "Portable RGB double-buffered depth request" },
    { "rgba_alpha_depth", "rgba alpha>=1 depth>=12", PROBE_CASE_TYPE_VALID, "Explicit alpha plus depth request" },
    { "rgba_depth_stencil", "rgba double depth stencil", PROBE_CASE_TYPE_PLATFORM_SPECIFIC,
        "Bare stencil token defaults to ~1: stencil-capable but preferring the least stencil" },
    { "rgba8_depth24", "red>=8 green>=8 blue>=8 alpha>=8 depth>=24", PROBE_CASE_TYPE_VALID, "Typical modern RGBA8 plus depth configuration" },
    { "depth_eq_16", "depth=16", PROBE_CASE_TYPE_PLATFORM_SPECIFIC, "Platform-dependent exact 16-bit depth availability" },
    { "depth_neq_16", "depth!=16", PROBE_CASE_TYPE_PLATFORM_SPECIFIC, "Useful for checking NEQ comparator handling across implementations" },
    { "samples_default", "samples", PROBE_CASE_TYPE_PLATFORM_SPECIFIC,
        "Bare samples defaults to <=4 preferring more (i.e. requests multisampling if available)" },
    { "samples_eq_4", "samples=4", PROBE_CASE_TYPE_PLATFORM_SPECIFIC, "Platform-dependent exact multisample count request" },
    { "samples_lte_4", "samples<=4", PROBE_CASE_TYPE_PLATFORM_SPECIFIC, "Comparator test for multisample selection" },
    { "stencil_samples_combo", "stencil~2 rgb double depth>=16 samples", PROBE_CASE_TYPE_PLATFORM_SPECIFIC,
        "Man-page style mixed comparator request; sample count preference varies" },
    { "stereo", "stereo", PROBE_CASE_TYPE_PLATFORM_SPECIFIC, "Stereo support depends on hardware and driver stack" },
    { "acca", "acca", PROBE_CASE_TYPE_PLATFORM_SPECIFIC, "Accumulation buffers are platform-dependent and often unavailable on modern systems" },
    { "auxbufs_eq_1", "auxbufs=1", PROBE_CASE_TYPE_PLATFORM_SPECIFIC, "Aux buffer availability varies widely by backend" },
    { "slow_bare", "rgb double slow", PROBE_CASE_TYPE_VALID,
        "Bare slow means >=0: permit fast formats but accept slow ones in preference; always satisfiable" },
    { "acca_fast", "rgb acca slow=0", PROBE_CASE_TYPE_PLATFORM_SPECIFIC,
        "slow=0 requires a fast format; Mesa marks accum-capable GLX configs with the slow caveat, making this impossible there" },
    { "impossible_depth", "rgb depth>256", PROBE_CASE_TYPE_INVALID, "Strict impossible comparator case" },
    { "impossible_alpha", "rgba alpha>64", PROBE_CASE_TYPE_INVALID, "Strict impossible comparator case" },
    { "bare_value_tokens", "rgb double depth", PROBE_CASE_TYPE_VALID,
        "Bare value tokens must resolve to documented defaults (regression: X11 treated unspecified criteria as a hard fail)" },
    { "malformed_token", "rgb double depth=", PROBE_CASE_TYPE_VALID,
        "Malformed token (comparator without value) is warned about and ignored, per GLUT" },
    { "srgb_token", "rgb double depth>=16 srgb", PROBE_CASE_TYPE_PLATFORM_SPECIFIC,
        "freeglut sRGB extension token; unrecognized (warn+ignore) on classic GLUT implementations" },
    { "captionless_token", "rgba depth captionless", PROBE_CASE_TYPE_PLATFORM_SPECIFIC,
        "freeglut captionless extension token; unrecognized (warn+ignore) on classic GLUT implementations" },
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
#define NUM_SELF_TEST_CASES ( (int)( sizeof( g_self_test_cases ) / sizeof( g_self_test_cases[0] ) ) )

static int run_self_test( int summaryMode )
{
    ProbeResult results[NUM_SELF_TEST_CASES];
    int failures = 0;
    int i;

    print_gl_info_banner( );

    for ( i = 0; i < NUM_SELF_TEST_CASES; i++ ) {
        const ProbeCase *probeCase = &g_self_test_cases[i];

        glutInitDisplayString( (char *)probeCase->displayString );
        probe_display_string( 1, &results[i] );

        if ( !summaryMode ) {
            print_probe_result_detail( probeCase->displayString, &results[i] );

            if ( probeCase->type != PROBE_CASE_TYPE_PLATFORM_SPECIFIC ) {
                int expected = ( probeCase->type == PROBE_CASE_TYPE_VALID );
                int mismatch = results[i].possible != expected;
                printf( "case=%s expected=%d actual=%s%d%s\n",
                    probeCase->name,
                    expected,
                    mismatch ? ansi_red( ) : ansi_green( ),
                    results[i].possible,
                    ansi_reset( ) );
            } else {
                printf( "case=%s expected=platform actual=%d\n", probeCase->name, results[i].possible );
            }

            if ( probeCase->note )
                printf( "note=%s\n", probeCase->note );
            printf( "\n" );
        }

        if ( probeCase->type != PROBE_CASE_TYPE_PLATFORM_SPECIFIC ) {
            int expected = ( probeCase->type == PROBE_CASE_TYPE_VALID );
            if ( results[i].possible != expected )
                failures++;
        }
    }

    /* glxinfo-style table of every probed configuration. */
    print_probe_result_table_header( );
    for ( i = 0; i < NUM_SELF_TEST_CASES; i++ )
        print_probe_result_summary( &g_self_test_cases[i], g_self_test_cases[i].displayString,
                                    &results[i], i );

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
        print_probe_result_summary( NULL, displayString, &result, 0 );
        return result.possible ? 0 : 1;
    }
}
