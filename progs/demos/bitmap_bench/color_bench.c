/*
 * color_bench.c -- what does changing glColor between bitmap glyphs cost?
 *
 * Syntax-highlighted text, per-character colour effects and the like issue a
 * glColor*() between glyphs.  Some implementations revalidate a good deal of
 * state on the next raster operation after a colour change, so the same text
 * can get dramatically more expensive purely from how often the colour moves.
 * Mesa has historically been sensitive to this.
 *
 * Every variant below draws exactly the same glyphs with glutBitmapCharacter(),
 * so the bitmap-font pixel-store handling is identical throughout and the only
 * thing that varies is how often glColor3f() is called:
 *
 *   per pass    one glColor3f for the whole text block   (baseline)
 *   per line    one glColor3f before each line
 *   per glyph   one glColor3f before every character
 *
 *     per-glyph colour-change cost = ns/glyph(per glyph) - ns/glyph(per pass)
 *
 * For reference the same block is also drawn with a single colour and
 * glutBitmapString(), which is the cheapest way to put this text on screen.
 *
 * Usage: color_bench [--seconds N] [--warmup N] [--lines N] [--cols N]
 *                    [--repeats N] [--font NAME] [--strategy NAME]
 *                    [--hold] [--tsv]
 *
 * --strategy sets FREEGLUT_BITMAP_PIXEL_STORE for the run; the colour delta
 * should be independent of it, which is worth confirming.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/freeglut.h>

#if defined( _WIN32 )
#  define setenv( n, v, o ) _putenv_s( ( n ), ( v ) )
#  include <windows.h>
static double now_seconds( void )
{
    LARGE_INTEGER freq, ctr;
    QueryPerformanceFrequency( &freq );
    QueryPerformanceCounter( &ctr );
    return (double)ctr.QuadPart / (double)freq.QuadPart;
}
#else
#  include <time.h>
static double now_seconds( void )
{
    struct timespec ts;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1.0e-9;
}
#endif

static const struct { const char *name; void *font; } fonts[] = {
    { "8x13",  GLUT_BITMAP_8_BY_13        },
    { "9x15",  GLUT_BITMAP_9_BY_15        },
    { "tr10",  GLUT_BITMAP_TIMES_ROMAN_10 },
    { "tr24",  GLUT_BITMAP_TIMES_ROMAN_24 },
    { "hel10", GLUT_BITMAP_HELVETICA_10   },
    { "hel12", GLUT_BITMAP_HELVETICA_12   },
    { "hel18", GLUT_BITMAP_HELVETICA_18   }
};
static const int num_fonts = (int)( sizeof( fonts ) / sizeof( fonts[0] ) );

static double  opt_seconds = 2.0;
static double  opt_warmup  = 0.3;
static int     opt_lines   = 40;
static int     opt_cols    = 80;
static int     opt_repeats = 1;
static int     opt_font    = 0;         /* 8x13: smallest glyphs, so state
                                         * changes are the largest share */
static int     opt_tsv     = 0;
static int     opt_hold    = 0;
static const char *opt_strategy = NULL;

static char *text_line;
static int   win_h = 720;
static int   line_step;

/* A small palette, cycled so successive colours always actually differ --
 * setting the same colour twice may well be free. */
static const GLfloat palette[6][3] = {
    { 0.95f, 0.85f, 0.60f }, { 0.60f, 0.90f, 0.95f },
    { 0.90f, 0.60f, 0.85f }, { 0.70f, 0.95f, 0.65f },
    { 0.95f, 0.70f, 0.55f }, { 0.75f, 0.75f, 1.00f }
};

/* ---------------------------------------------------------------- workload */

static void *the_font( void ) { return fonts[opt_font].font; }

static int glyphs_per_pass( void )
{
    return opt_lines * opt_cols * opt_repeats;
}

static void set_color( int n )
{
    const GLfloat *c = palette[ n % 6 ];
    glColor3f( c[0], c[1], c[2] );
}

/* One glColor3f for the entire block. */
static void draw_color_per_pass( void )
{
    int rep, row;

    set_color( 0 );
    for( rep = 0; rep < opt_repeats; rep++ )
    {
        float y = (float)( win_h - line_step );
        for( row = 0; row < opt_lines; row++ )
        {
            const char *p = text_line;
            glRasterPos2f( 8.0f, y );
            while( *p )
                glutBitmapCharacter( the_font( ), (unsigned char)*p++ );
            y -= (float)line_step;
        }
    }
}

/* One glColor3f per line of text. */
static void draw_color_per_line( void )
{
    int rep, row, n = 0;

    for( rep = 0; rep < opt_repeats; rep++ )
    {
        float y = (float)( win_h - line_step );
        for( row = 0; row < opt_lines; row++ )
        {
            const char *p = text_line;
            set_color( n++ );
            glRasterPos2f( 8.0f, y );
            while( *p )
                glutBitmapCharacter( the_font( ), (unsigned char)*p++ );
            y -= (float)line_step;
        }
    }
}

/* One glColor3f per glyph -- what syntax highlighting costs. */
static void draw_color_per_glyph( void )
{
    int rep, row, n = 0;

    for( rep = 0; rep < opt_repeats; rep++ )
    {
        float y = (float)( win_h - line_step );
        for( row = 0; row < opt_lines; row++ )
        {
            const char *p = text_line;
            glRasterPos2f( 8.0f, y );
            while( *p )
            {
                set_color( n++ );
                glutBitmapCharacter( the_font( ), (unsigned char)*p++ );
            }
            y -= (float)line_step;
        }
    }
}

/* Reference point: cheapest way to draw this text at all. */
static void draw_string_one_color( void )
{
    int rep, row;

    set_color( 0 );
    for( rep = 0; rep < opt_repeats; rep++ )
    {
        float y = (float)( win_h - line_step );
        for( row = 0; row < opt_lines; row++ )
        {
            glRasterPos2f( 8.0f, y );
            glutBitmapString( the_font( ), (const unsigned char *)text_line );
            y -= (float)line_step;
        }
    }
}

/* ------------------------------------------------------------------ timing */

enum { CASE_PER_PASS, CASE_PER_LINE, CASE_PER_GLYPH, CASE_STRING, NUM_CASES };

static const struct {
    const char *name;
    void      ( *draw )( void );
} cases[NUM_CASES] = {
    { "Character, colour/pass",  draw_color_per_pass  },
    { "Character, colour/line",  draw_color_per_line  },
    { "Character, colour/glyph", draw_color_per_glyph },
    { "String,    colour/pass",  draw_string_one_color }
};

static double timed_pass( void ( *draw )( void ) )
{
    double t0;

    glFinish( );
    t0 = now_seconds( );
    draw( );
    glFinish( );
    return ( now_seconds( ) - t0 ) * 1.0e9 / (double)glyphs_per_pass( );
}

/*
 * All cases are timed round-robin, pass by pass, so drift in clocks or thermals
 * cannot favour whichever ran first.  Each result is the minimum over its
 * passes: interference can only add time, so the minimum is the most stable
 * estimate of the call's own cost.
 */
static void measure( double ns[NUM_CASES], long long *passes )
{
    double t_end;
    int i;

    for( i = 0; i < NUM_CASES; i++ )
        ns[i] = 1.0e30;
    *passes = 0;

    t_end = now_seconds( ) + opt_warmup;
    while( now_seconds( ) < t_end )
    {
        for( i = 0; i < NUM_CASES; i++ )
            cases[i].draw( );
        glFinish( );
    }

    t_end = now_seconds( ) + opt_seconds;
    do {
        for( i = 0; i < NUM_CASES; i++ )
        {
            double t = timed_pass( cases[i].draw );
            if( t < ns[i] )
                ns[i] = t;
        }
        ( *passes )++;
    } while( now_seconds( ) < t_end );
}

/* --------------------------------------------------------------- reporting */

static const char *strategy_in_use( void )
{
    const char *env = getenv( "FREEGLUT_BITMAP_PIXEL_STORE" );

    if( env && ( !strcmp( env, "clientattrib" ) || !strcmp( env, "getset" ) ) )
        return env;
#ifdef __APPLE__
    return "getset (default)";
#else
    return "clientattrib (default)";
#endif
}

static void report( const double ns[NUM_CASES], long long passes )
{
    double per_glyph = ns[CASE_PER_GLYPH] - ns[CASE_PER_PASS];
    double per_line  = ns[CASE_PER_LINE]  - ns[CASE_PER_PASS];
    int i;

    if( opt_tsv )
    {
        printf( "%s\t%s\t%d\t%d\t%d\t%lld", strategy_in_use( ),
                fonts[opt_font].name, opt_lines, opt_cols, opt_repeats, passes );
        for( i = 0; i < NUM_CASES; i++ )
            printf( "\t%.2f", ns[i] );
        printf( "\t%.2f\n", per_glyph );
        fflush( stdout );
        return;
    }

    printf( "\n" );
    printf( "  pixel-store strategy : %s\n", strategy_in_use( ) );
    printf( "  workload             : %s, %d lines x %d cols x %d = %d"
            " glyphs/pass, %lld passes\n",
            fonts[opt_font].name, opt_lines, opt_cols, opt_repeats,
            glyphs_per_pass( ), passes );
    printf( "\n" );
    printf( "    %-24s %14s %16s\n", "case", "ns/glyph", "glyphs/sec" );
    for( i = 0; i < NUM_CASES; i++ )
        printf( "    %-24s %14.1f %16.0f\n",
                cases[i].name, ns[i], 1.0e9 / ns[i] );
    printf( "\n" );
    printf( "  colour change per line  : %+.1f ns/glyph\n", per_line );
    printf( "  colour change per glyph : %+.1f ns/glyph"
            "   (%.2fx the one-colour cost)\n",
            per_glyph, ns[CASE_PER_GLYPH] / ns[CASE_PER_PASS] );
    printf( "\n" );
    fflush( stdout );
}

/* ------------------------------------------------------------------ window */

static void show_workload( const char *caption )
{
    glClearColor( 0.07f, 0.07f, 0.09f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT );
    draw_color_per_glyph( );
    glColor3f( 0.45f, 0.75f, 1.0f );
    glRasterPos2f( 8.0f, 6.0f );
    glutBitmapString( GLUT_BITMAP_9_BY_15, (const unsigned char *)caption );
    glutSwapBuffers( );
}

static void display( void )
{
    static int measured = 0;
    double ns[NUM_CASES];
    long long passes;

    if( measured )
    {
        show_workload( strategy_in_use( ) );
        return;
    }
    measured = 1;

    /* A raster position outside the window is invalid and glBitmap() then draws
     * nothing, which would look like an impossibly fast result. */
    while( opt_lines > 1 && (float)( win_h - opt_lines * line_step ) < 0.0f )
        opt_lines--;

    show_workload( "measuring..." );
    measure( ns, &passes );
    report( ns, passes );
    show_workload( strategy_in_use( ) );

    if( !opt_hold )
        exit( 0 );
    printf( "  (window held open -- press any key to quit)\n\n" );
    fflush( stdout );
}

static void reshape( int w, int h )
{
    win_h = h > 1 ? h : 1;
    glViewport( 0, 0, w, win_h );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );
    glOrtho( 0, w, 0, win_h, -1, 1 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity( );
}

static void keyboard( unsigned char key, int x, int y )
{
    (void)key; (void)x; (void)y;
    exit( 0 );
}

/* --------------------------------------------------------------------- CLI */

static void usage( const char *argv0 )
{
    int i;
    fprintf( stderr,
             "Usage: %s [--seconds N] [--warmup N] [--lines N] [--cols N]\n"
             "          [--repeats N] [--font NAME]"
             " [--strategy clientattrib|getset]\n"
             "          [--hold] [--tsv]\n"
             "Fonts:", argv0 );
    for( i = 0; i < num_fonts; i++ )
        fprintf( stderr, " %s", fonts[i].name );
    fprintf( stderr, "\n" );
}

static int parse_args( int argc, char **argv )
{
    int i;

    for( i = 1; i < argc; i++ )
    {
        const char *a = argv[i];

        if( !strcmp( a, "--tsv" ) )
            opt_tsv = 1;
        else if( !strcmp( a, "--hold" ) )
            opt_hold = 1;
        else if( !strcmp( a, "--seconds" ) && i + 1 < argc )
            opt_seconds = atof( argv[++i] );
        else if( !strcmp( a, "--warmup" ) && i + 1 < argc )
            opt_warmup = atof( argv[++i] );
        else if( !strcmp( a, "--lines" ) && i + 1 < argc )
            opt_lines = atoi( argv[++i] );
        else if( !strcmp( a, "--cols" ) && i + 1 < argc )
            opt_cols = atoi( argv[++i] );
        else if( !strcmp( a, "--repeats" ) && i + 1 < argc )
            opt_repeats = atoi( argv[++i] );
        else if( !strcmp( a, "--strategy" ) && i + 1 < argc )
        {
            opt_strategy = argv[++i];
            if( strcmp( opt_strategy, "clientattrib" ) &&
                strcmp( opt_strategy, "getset" ) )
            {
                fprintf( stderr, "color_bench: unknown strategy '%s'\n",
                         opt_strategy );
                usage( argv[0] );
                return 0;
            }
        }
        else if( !strcmp( a, "--font" ) && i + 1 < argc )
        {
            const char *name = argv[++i];
            int f;
            for( f = 0; f < num_fonts && strcmp( name, fonts[f].name ); f++ )
                ;
            if( f == num_fonts )
            {
                fprintf( stderr, "color_bench: unknown font '%s'\n", name );
                usage( argv[0] );
                return 0;
            }
            opt_font = f;
        }
        else
        {
            usage( argv[0] );
            return 0;
        }
    }

    if( opt_seconds <= 0.0 ) opt_seconds = 0.5;
    if( opt_warmup  <  0.0 ) opt_warmup  = 0.0;
    if( opt_lines   <  1   ) opt_lines   = 1;
    if( opt_cols    <  1   ) opt_cols    = 1;
    if( opt_repeats <  1   ) opt_repeats = 1;
    return 1;
}

static void build_text_line( void )
{
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 .,;:!?-+/*";
    const int alen = (int)( sizeof( alphabet ) - 1 );
    int i;

    text_line = (char *)malloc( (size_t)opt_cols + 1 );
    if( !text_line )
    {
        fprintf( stderr, "color_bench: out of memory\n" );
        exit( 1 );
    }
    for( i = 0; i < opt_cols; i++ )
        text_line[i] = alphabet[i % alen];
    text_line[opt_cols] = '\0';
}

int main( int argc, char **argv )
{
    int win_w;

    if( !parse_args( argc, argv ) )
        return 1;

    /* Must be set before the first bitmap-font call, which is where fg_font.c
     * resolves and caches it. */
    if( opt_strategy )
        setenv( "FREEGLUT_BITMAP_PIXEL_STORE", opt_strategy, 1 );

    build_text_line( );

    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB );
    glutInitWindowSize( 800, win_h );
    glutCreateWindow( "freeglut color_bench" );

    line_step = glutBitmapHeight( the_font( ) ) + 2;
    win_h = 24 + opt_lines * line_step;
    win_w = 16 + glutBitmapLength( the_font( ),
                                   (const unsigned char *)text_line );
    glutReshapeWindow( win_w, win_h );

    glutDisplayFunc( display );
    glutReshapeFunc( reshape );
    glutKeyboardFunc( keyboard );
    glutSwapInterval( 0 );
    glDisable( GL_DEPTH_TEST );

    glutMainLoop( );
    return 0;
}
