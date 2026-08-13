/*
 * bitmap_bench.c -- how much does the bitmap-font pixel-store save/restore cost?
 *
 * glutBitmapCharacter() saves and restores the six GL_UNPACK_* pixel-store
 * values around every single glyph; glutBitmapString() does it once for the
 * whole string.  So the per-glyph price of whichever save/restore strategy
 * freeglut uses shows up as the gap between the two calls drawing identical
 * text:
 *
 *     per-glyph save/restore cost = ns/glyph(Character) - ns/glyph(String)
 *
 * src/fg_font.c picks the strategy from FREEGLUT_BITMAP_PIXEL_STORE:
 *
 *   clientattrib   glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT) / glPopClientAttrib
 *   getset         6x glGetIntegerv, then 6x glPixelStorei to restore
 *   (unset)        platform default: getset on Apple, clientattrib elsewhere
 *
 * It reads that once and caches it, because re-reading per glyph would cost
 * more than the difference being measured.  A process therefore exercises
 * exactly one strategy, so by default this program re-runs itself once per
 * strategy and prints the comparison.  --strategy NAME measures just one.
 *
 * Usage: bitmap_bench [--seconds N] [--warmup N] [--lines N] [--cols N]
 *                     [--repeats N] [--font NAME] [--strategy NAME]
 *                     [--swap] [--swap-interval N] [--hold] [--tsv]
 *
 * By default nothing is presented, so the timing is pure draw cost and nothing
 * is clamped to the display refresh rate.  --swap presents each timed pass as a
 * real frame and --swap-interval 1 puts that behind vsync, which answers the
 * separate question of whether the difference survives into a frame budget.
 * (The Cocoa backend paces frames through CVDisplayLink and does not honour the
 * swap interval, so on macOS --swap always behaves as if vsync were on.)
 *
 * --repeats multiplies the work per pass without needing a larger window, which
 * is how to push the workload past one refresh period.
 *
 * The window is painted with the workload before and after the timed section,
 * and --hold keeps it up so the text can be eyeballed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/freeglut.h>

#if defined( _WIN32 )
#  define popen  _popen
#  define pclose _pclose
#  define setenv( n, v, o ) _putenv_s( ( n ), ( v ) )
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

static const char *strategies[2] = { "clientattrib", "getset" };

static double  opt_seconds = 2.0;
static double  opt_warmup  = 0.3;
static int     opt_lines   = 40;
static int     opt_cols    = 80;
static int     opt_repeats = 1;         /* redraw the block N times per pass:
                                         * more work without a bigger window */
static int     opt_font    = 0;         /* 8x13: smallest glyphs, so the
                                         * save/restore is the largest share
                                         * of per-glyph cost */
static int     opt_tsv     = 0;
static int     opt_hold    = 0;
static int     opt_swap    = 0;         /* present each timed pass          */
static int     opt_interval = 0;        /* glutSwapInterval: 1 == wait vsync */
static int     opt_child   = 0;         /* internal: emit one machine-readable line */
static int     opt_micro   = 0;         /* in-process save/restore A/B only   */
static int     opt_sweep   = 0;         /* Character vs String by string length */
static const char *opt_strategy = NULL; /* NULL = run every strategy as a child */

static char *text_line;                 /* opt_cols characters, NUL terminated */
static int   win_h = 720;
static int   line_step;                 /* vertical advance between rows       */

/* ---------------------------------------------------------------- workload */

static void *the_font( void ) { return fonts[opt_font].font; }

static int glyphs_per_pass( void )
{
    return opt_lines * opt_cols * opt_repeats;
}

static void draw_with_character( void )
{
    int rep, row;

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

static void draw_with_string( void )
{
    int rep, row;

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

/* ------------------------------------------------- string-length sweep ----
 *
 * glutBitmapCharacter pays one save/restore per glyph; glutBitmapString pays
 * one per *call*.  So the two should cost the same for a one-character string
 * and diverge as the string grows.  This sweep measures that curve.
 *
 * The glyphs drawn are IDENTICAL for every string length: a fixed grid of
 * SWEEP_ROWS x SWEEP_COLS positions is filled every time, and the only thing
 * that varies is whether it is emitted in chunks of 1, 10, 100 or 1000
 * characters.  Holding only the glyph *count* constant is not enough -- with
 * the same glyph redrawn in one spot at L=1 and a full block covered at
 * L=1000, the pixel coverage and cache behaviour differ enormously, which on
 * Mesa swamped the effect being measured and made long strings look slower.
 *
 * The grid is sized to match the default library-level workload, because the
 * absolute per-glyph figures depend on how many glyphs are in a timed pass: the
 * two glFinish() calls bracketing a pass cost a fixed ~450 us on Apple's
 * Metal-backed GL, which is amortised over the pass.  Halving the glyphs per
 * pass therefore inflates every per-glyph number by ~10-30%.  The Character
 * minus String *gap* is unaffected, since both passes pay the same overhead and
 * it cancels -- so gaps are comparable across workloads and absolutes are not.
 *
 * Both variants issue the same number of glRasterPos2f calls, one per chunk.
 * Chunk sizes divide (or are multiples of) the row length, so a chunk that
 * spans rows always starts at column 0 -- which matters because
 * glutBitmapString's newline steps back to the string's own start x, and the
 * character path has to mirror that exactly to stay comparable.
 */

#define SWEEP_ROWS  40
#define SWEEP_COLS 100
#define SWEEP_GLYPHS ( SWEEP_ROWS * SWEEP_COLS )

static const int sweep_len[] = { 1, 10, 100, 1000 };
#define SWEEP_N ( (int)( sizeof( sweep_len ) / sizeof( sweep_len[0] ) ) )

static char  sweep_grid[SWEEP_GLYPHS];   /* the glyphs, row-major        */
static char *sweep_chunk[SWEEP_N];       /* chunk text, newlines inserted */
static int   glyph_w[256];               /* precomputed: the character path
                                          * must not pay for glutBitmapWidth */

static void build_sweep_text( void )
{
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 .,;:!?-+/*";
    const int alen = (int)( sizeof( alphabet ) - 1 );
    int i, n;

    for( i = 0; i < 256; i++ )
        glyph_w[i] = glutBitmapWidth( the_font( ), i );

    for( i = 0; i < SWEEP_GLYPHS; i++ )
        sweep_grid[i] = alphabet[i % alen];

    /* One chunk's worth of text, with a newline wherever it crosses a row. */
    for( n = 0; n < SWEEP_N; n++ )
    {
        int len = sweep_len[n];
        int nl  = ( len - 1 ) / SWEEP_COLS;
        char *t = (char *)malloc( (size_t)( len + nl + 1 ) );
        int k, dst = 0;

        if( !t )
        {
            fprintf( stderr, "bitmap_bench: out of memory\n" );
            exit( 1 );
        }
        for( k = 0; k < len; k++ )
        {
            if( k > 0 && k % SWEEP_COLS == 0 )
                t[dst++] = '\n';
            t[dst++] = sweep_grid[k % SWEEP_GLYPHS];
        }
        t[dst] = '\0';
        sweep_chunk[n] = t;
    }
}

/* Top-left of the glyph at grid index i. */
static void sweep_origin( int i, float *x, float *y )
{
    *x = 8.0f + (float)( ( i % SWEEP_COLS ) * glyph_w[(unsigned char)'A'] );
    *y = (float)( win_h - line_step - ( i / SWEEP_COLS ) * line_step );
}

static void sweep_draw_character( int n )
{
    const int len = sweep_len[n];
    int i;

    for( i = 0; i < SWEEP_GLYPHS; i += len )
    {
        const char *p = sweep_chunk[n];
        float x0, y0, x = 0.0f;
        unsigned char c;

        sweep_origin( i, &x0, &y0 );
        glRasterPos2f( x0, y0 );
        while( ( c = (unsigned char)*p++ ) != 0 )
        {
            if( c == '\n' )
            {
                /* The same reposition glutBitmapString performs internally. */
                glBitmap( 0, 0, 0, 0, -x, (float)-line_step, NULL );
                x = 0.0f;
            }
            else
            {
                glutBitmapCharacter( the_font( ), c );
                x += (float)glyph_w[c];
            }
        }
    }
}

static void sweep_draw_string( int n )
{
    const int len = sweep_len[n];
    int i;

    for( i = 0; i < SWEEP_GLYPHS; i += len )
    {
        float x0, y0;

        sweep_origin( i, &x0, &y0 );
        glRasterPos2f( x0, y0 );
        glutBitmapString( the_font( ), (const unsigned char *)sweep_chunk[n] );
    }
}

/* ----------------------------------------------------------------- timing */

#if defined( _WIN32 )
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

/*
 * One timed pass: draw the whole text block once, ending in glFinish() so the
 * elapsed time covers GL work that actually completed rather than commands
 * merely queued.  Returns nanoseconds per glyph.
 *
 * --swap presents each pass as a real frame.  Combined with --swap-interval 1
 * that puts the measurement behind vsync, which is what an actual application
 * sees: below one refresh period of work the difference between the two
 * strategies is absorbed by the wait and cannot matter, above it the frame
 * budget is genuinely at stake.
 */
static double timed_pass( void ( *draw )( void ) )
{
    double t0;

    glFinish( );
    t0 = now_seconds( );
    draw( );
    if( opt_swap )
        glutSwapBuffers( );
    glFinish( );
    return ( now_seconds( ) - t0 ) * 1.0e9 / (double)glyphs_per_pass( );
}

/*
 * The two calls are timed alternately, pass by pass, so a warming GPU or a
 * ramping CPU clock cannot systematically favour whichever ran first.  Each
 * result is the *minimum* over all passes: interference from the compositor,
 * the scheduler and background load can only ever add time, so the minimum is
 * the most stable estimate of what the call itself costs.
 */
static void measure( double *ns_char, double *ns_string, long long *passes )
{
    double t_end;

    *ns_char = *ns_string = 1.0e30;
    *passes  = 0;

    t_end = now_seconds( ) + opt_warmup;
    while( now_seconds( ) < t_end )
    {
        draw_with_character( );
        draw_with_string( );
        glFinish( );
    }

    t_end = now_seconds( ) + opt_seconds;
    do {
        double c = timed_pass( draw_with_character );
        double s = timed_pass( draw_with_string );

        if( c < *ns_char   ) *ns_char   = c;
        if( s < *ns_string ) *ns_string = s;
        ( *passes )++;
    } while( now_seconds( ) < t_end );
}

static const char *strategy_in_use( void );

/*
 * Time all 2*SWEEP_N series round-robin, pass by pass, taking the minimum for
 * each -- same discipline as everywhere else here, so a drifting clock cannot
 * favour one length over another.
 */
static void run_sweep( void )
{
    double ns_char[SWEEP_N], ns_string[SWEEP_N];
    double t_end;
    long long passes = 0;
    int n;

    for( n = 0; n < SWEEP_N; n++ )
        ns_char[n] = ns_string[n] = 1.0e30;

    t_end = now_seconds( ) + opt_warmup;
    while( now_seconds( ) < t_end )
    {
        for( n = 0; n < SWEEP_N; n++ )
        {
            sweep_draw_character( n );
            sweep_draw_string( n );
        }
        glFinish( );
    }

    t_end = now_seconds( ) + opt_seconds;
    do {
        for( n = 0; n < SWEEP_N; n++ )
        {
            double glyphs = (double)SWEEP_GLYPHS;
            double t0, t;

            glFinish( );
            t0 = now_seconds( );
            sweep_draw_character( n );
            glFinish( );
            t = ( now_seconds( ) - t0 ) * 1.0e9 / glyphs;
            if( t < ns_char[n] ) ns_char[n] = t;

            glFinish( );
            t0 = now_seconds( );
            sweep_draw_string( n );
            glFinish( );
            t = ( now_seconds( ) - t0 ) * 1.0e9 / glyphs;
            if( t < ns_string[n] ) ns_string[n] = t;
        }
        passes++;
    } while( now_seconds( ) < t_end );

    if( opt_tsv )
    {
        printf( "sweep\t%s\t%lld", strategy_in_use( ), passes );
        for( n = 0; n < SWEEP_N; n++ )
            printf( "\t%.1f\t%.1f", ns_char[n], ns_string[n] );
        printf( "\n" );
        fflush( stdout );
        return;
    }

    printf( "\n" );
    printf( "  cost per glyph, %s font, %s\n",
            fonts[opt_font].name, strategy_in_use( ) );
    printf( "  (%d glyphs per timed pass, %lld passes)\n\n",
            SWEEP_GLYPHS, passes );
    printf( "    %-12s %20s %20s\n",
            "string len", "glutBitmapCharacter", "glutBitmapString" );
    for( n = 0; n < SWEEP_N; n++ )
        printf( "    %-12d %17.1f ns %17.1f ns\n",
                sweep_len[n], ns_char[n], ns_string[n] );
    printf( "\n" );
    printf( "  Both columns fall with L because each unit's glRasterPos2f is\n"
            "  amortised over L glyphs.  What isolates the save/restore is the\n"
            "  gap between them: glutBitmapString pays it once per call, so the\n"
            "  gap starts at zero for L=1 and converges to the full per-glyph\n"
            "  cost as L grows:\n\n" );
    for( n = 0; n < SWEEP_N; n++ )
        printf( "    L=%-6d gap %+.1f ns/glyph\n",
                sweep_len[n], ns_char[n] - ns_string[n] );
    printf( "\n" );
    printf( "\n" );
    fflush( stdout );
}

/* ------------------------------------------------- in-process micro A/B ---
 *
 * The library-level comparison above has to fork a child per strategy, because
 * fg_font.c caches its choice.  On a machine that scales CPU/GPU frequency the
 * two children can therefore run at different clock states, and no amount of
 * averaging inside a child fixes a difference *between* them -- the noise check
 * detects that but cannot remove it.
 *
 * These two functions are the save/restore sequences from fg_font.c lifted out
 * verbatim, with no glBitmap and no library involvement, so both strategies can
 * be timed in one process, alternating pass by pass.  Same clocks, same cache
 * state, no cross-process confound.  This measures the patch itself rather than
 * the patch plus a glyph.
 */

static void micro_clientattrib( int iters )
{
    int i;

    for( i = 0; i < iters; i++ )
    {
        glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT );
        glPixelStorei( GL_UNPACK_SWAP_BYTES,  GL_FALSE );
        glPixelStorei( GL_UNPACK_LSB_FIRST,   GL_FALSE );
        glPixelStorei( GL_UNPACK_ROW_LENGTH,  0        );
        glPixelStorei( GL_UNPACK_SKIP_ROWS,   0        );
        glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0        );
        glPixelStorei( GL_UNPACK_ALIGNMENT,   1        );
        glPopClientAttrib( );
    }
}

static void micro_getset( int iters )
{
    int i;

    for( i = 0; i < iters; i++ )
    {
        GLint swbytes, lsbfirst, rowlen, skiprows, skippix, align;

        glGetIntegerv( GL_UNPACK_SWAP_BYTES,  &swbytes  );
        glGetIntegerv( GL_UNPACK_LSB_FIRST,   &lsbfirst );
        glGetIntegerv( GL_UNPACK_ROW_LENGTH,  &rowlen   );
        glGetIntegerv( GL_UNPACK_SKIP_ROWS,   &skiprows );
        glGetIntegerv( GL_UNPACK_SKIP_PIXELS, &skippix  );
        glGetIntegerv( GL_UNPACK_ALIGNMENT,   &align    );

        glPixelStorei( GL_UNPACK_SWAP_BYTES,  GL_FALSE );
        glPixelStorei( GL_UNPACK_LSB_FIRST,   GL_FALSE );
        glPixelStorei( GL_UNPACK_ROW_LENGTH,  0        );
        glPixelStorei( GL_UNPACK_SKIP_ROWS,   0        );
        glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0        );
        glPixelStorei( GL_UNPACK_ALIGNMENT,   1        );

        glPixelStorei( GL_UNPACK_SWAP_BYTES,  swbytes  );
        glPixelStorei( GL_UNPACK_LSB_FIRST,   lsbfirst );
        glPixelStorei( GL_UNPACK_ROW_LENGTH,  rowlen   );
        glPixelStorei( GL_UNPACK_SKIP_ROWS,   skiprows );
        glPixelStorei( GL_UNPACK_SKIP_PIXELS, skippix  );
        glPixelStorei( GL_UNPACK_ALIGNMENT,   align    );
    }
}

static int micro_iters = 20000;

static double timed_micro( void ( *fn )( int ) )
{
    double t0;

    glFinish( );
    t0 = now_seconds( );
    fn( micro_iters );
    glFinish( );
    return ( now_seconds( ) - t0 ) * 1.0e9 / (double)micro_iters;
}

static void run_micro( void )
{
    double ns_ca = 1.0e30, ns_gs = 1.0e30, t_end;
    long long passes = 0;

    t_end = now_seconds( ) + opt_warmup;
    while( now_seconds( ) < t_end )
    {
        micro_clientattrib( micro_iters );
        micro_getset( micro_iters );
        glFinish( );
    }

    t_end = now_seconds( ) + opt_seconds;
    do {
        double a = timed_micro( micro_clientattrib );
        double b = timed_micro( micro_getset );

        if( a < ns_ca ) ns_ca = a;
        if( b < ns_gs ) ns_gs = b;
        passes++;
    } while( now_seconds( ) < t_end );

    if( opt_tsv )
    {
        printf( "micro\t%d\t%lld\t%.1f\t%.1f\n",
                micro_iters, passes, ns_ca, ns_gs );
        fflush( stdout );
        return;
    }

    printf( "\n" );
    printf( "  pixel-store save/restore, measured in one process\n" );
    printf( "  (no glBitmap, both strategies interleaved -- immune to clock\n"
            "   drift between runs, unlike the library-level table)\n" );
    printf( "  %d iterations per timed pass, %lld passes\n\n",
            micro_iters, passes );
    printf( "    %-14s %18s\n", "strategy", "ns per save/restore" );
    printf( "    %-14s %18.1f\n", "clientattrib", ns_ca );
    printf( "    %-14s %18.1f\n", "getset", ns_gs );
    printf( "\n" );
    printf( "  %s is %.2fx cheaper\n",
            ns_gs < ns_ca ? "getset" : "clientattrib",
            ns_gs < ns_ca ? ns_ca / ns_gs : ns_gs / ns_ca );
    printf( "\n" );
    fflush( stdout );
}

/* ------------------------------------------------------------- one process */

/* What the parent collects from each child, and what --strategy reports. */
struct result {
    double    ns_char;
    double    ns_string;
    long long passes;
    char      raw[512];   /* the child's own --tsv line, for --tsv passthrough */
};

/*
 * Report what GL implementation actually answered.  Which libGL a build links
 * is not obvious from the platform -- an X11 build on macOS may get Mesa via
 * XQuartz rather than Apple's GL -- and the whole point of these numbers is
 * which implementation they describe.  Goes to stderr so it survives --tsv.
 */
static void report_renderer( void )
{
    const GLubyte *vendor   = glGetString( GL_VENDOR );
    const GLubyte *renderer = glGetString( GL_RENDERER );
    const GLubyte *version  = glGetString( GL_VERSION );

    fprintf( stderr, "  GL: %s | %s | %s\n",
             vendor   ? (const char *)vendor   : "?",
             renderer ? (const char *)renderer : "?",
             version  ? (const char *)version  : "?" );
    fflush( stderr );
}

static const char *strategy_in_use( void )
{
    const char *env = getenv( "FREEGLUT_BITMAP_PIXEL_STORE" );

    if( env && ( !strcmp( env, "clientattrib" ) || !strcmp( env, "getset" ) ) )
        return env;

    /* fg_font.c keys its default off TARGET_HOST_MACOS_COCOA, which is a
     * property of the freeglut build, not of this program -- an X11 build on
     * macOS defaults to clientattrib even though __APPLE__ is defined.  Do not
     * guess; name the strategy explicitly to pin it down. */
    return "platform default";
}

/* Paint the workload so the window shows what is being measured.  The timed
 * passes deliberately never swap -- that would clamp them to the refresh rate
 * -- so without this the window would stay blank for the whole run. */
static void show_workload( const char *caption )
{
    glClearColor( 0.07f, 0.07f, 0.09f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT );
    glColor3f( 0.85f, 0.87f, 0.80f );
    draw_with_string( );
    glColor3f( 0.45f, 0.75f, 1.0f );
    glRasterPos2f( 8.0f, 6.0f );
    glutBitmapString( GLUT_BITMAP_9_BY_15, (const unsigned char *)caption );
    glutSwapBuffers( );
}

static void report_one( const struct result *r )
{
    double overhead = r->ns_char - r->ns_string;

    if( opt_tsv || opt_child )
    {
        printf( "%s\t%s\t%d\t%d\t%d\t%lld\t%.2f\t%.2f\t%.2f\n",
                strategy_in_use( ), fonts[opt_font].name, opt_lines, opt_cols,
                opt_repeats, r->passes, r->ns_char, r->ns_string, overhead );
        fflush( stdout );
        return;
    }

    printf( "\n" );
    printf( "  pixel-store strategy : %s\n", strategy_in_use( ) );
    printf( "  workload             : %s, %d lines x %d cols = %d glyphs/pass,"
            " %lld passes\n",
            fonts[opt_font].name, opt_lines, opt_cols,
            glyphs_per_pass( ), r->passes );
    printf( "\n" );
    printf( "    %-22s %14s %16s\n", "call", "ns/glyph", "glyphs/sec" );
    printf( "    %-22s %14.1f %16.0f\n",
            "glutBitmapCharacter", r->ns_char, 1.0e9 / r->ns_char );
    printf( "    %-22s %14.1f %16.0f\n",
            "glutBitmapString", r->ns_string, 1.0e9 / r->ns_string );
    printf( "\n" );
    printf( "  per-glyph save/restore overhead : %.1f ns  (Character - String)\n",
            overhead );
    printf( "  Character is %.2fx the cost of String\n",
            r->ns_char / r->ns_string );
    printf( "\n" );
    fflush( stdout );
}

static void finished( void )
{
    if( !opt_hold )
        exit( 0 );
    if( !opt_child )
        printf( "  (window held open -- press any key to continue)\n\n" );
    fflush( stdout );
}

static void display( void )
{
    static int measured = 0;
    struct result r;

    /* With --hold the window stays up and GLUT keeps calling us on expose;
     * only the first call is the benchmark. */
    if( measured )
    {
        show_workload( strategy_in_use( ) );
        return;
    }
    measured = 1;

    /* A raster position outside the window is invalid, and glBitmap() then
     * draws nothing -- which would look like an impossibly fast result.  Drop
     * rows that do not fit rather than report a number that measures nothing. */
    while( opt_lines > 1 && (float)( win_h - opt_lines * line_step ) < 0.0f )
        opt_lines--;

    report_renderer( );
    show_workload( "measuring..." );

    if( opt_sweep )
    {
        run_sweep( );
        show_workload( strategy_in_use( ) );
        finished( );
        return;
    }

    if( opt_micro )
    {
        run_micro( );
        show_workload( strategy_in_use( ) );
        finished( );
        return;
    }

    measure( &r.ns_char, &r.ns_string, &r.passes );
    report_one( &r );
    show_workload( strategy_in_use( ) );
    finished( );
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

/* -------------------------------------------------- parent: run every path */

static void quote_append( char *dst, size_t cap, const char *arg )
{
    size_t len = strlen( dst );

#if defined( _WIN32 )
    snprintf( dst + len, cap - len, " \"%s\"", arg );
#else
    snprintf( dst + len, cap - len, " '%s'", arg );
#endif
}

/*
 * Re-run this program with --strategy NAME and read back its one-line result.
 * A separate process is what makes measuring both strategies possible at all:
 * fg_font.c resolves FREEGLUT_BITMAP_PIXEL_STORE once and caches it.
 */
static int run_child( const char *self, int argc, char **argv,
                      const char *strategy, struct result *r )
{
    char cmd[4096];
    char line[512];
    char got_strategy[64];
    FILE *pipe;
    int i, ok;

    cmd[0] = '\0';
    quote_append( cmd, sizeof( cmd ), self );
    for( i = 1; i < argc; i++ )
        quote_append( cmd, sizeof( cmd ), argv[i] );
    quote_append( cmd, sizeof( cmd ), "--strategy" );
    quote_append( cmd, sizeof( cmd ), strategy );
    quote_append( cmd, sizeof( cmd ), "--child" );

    pipe = popen( cmd, "r" );
    if( !pipe )
    {
        fprintf( stderr, "bitmap_bench: cannot run '%s'\n", cmd );
        return 0;
    }
    ok = ( fgets( line, sizeof( line ), pipe ) != NULL ) &&
         ( sscanf( line, "%63s %*s %*d %*d %*d %lld %lf %lf",
                   got_strategy, &r->passes, &r->ns_char, &r->ns_string ) == 4 );
    if( ok )
    {
        line[ strcspn( line, "\n" ) ] = '\0';
        snprintf( r->raw, sizeof( r->raw ), "%s", line );
    }
    if( pclose( pipe ) != 0 || !ok )
    {
        fprintf( stderr, "bitmap_bench: '%s' run failed\n", strategy );
        return 0;
    }
    return 1;
}

static void report_both( const struct result res[2] )
{
    double over[2];
    int i;

    for( i = 0; i < 2; i++ )
        over[i] = res[i].ns_char - res[i].ns_string;

    if( opt_tsv )
    {
        for( i = 0; i < 2; i++ )
            printf( "%s\n", res[i].raw );
        fflush( stdout );
        return;
    }

    printf( "\n" );
    printf( "  workload : %s, %d lines x %d cols = %d glyphs/pass,"
            " %.1fs per strategy\n",
            fonts[opt_font].name, opt_lines, opt_cols,
            glyphs_per_pass( ), opt_seconds );
    printf( "  presenting: %s\n",
            !opt_swap ? "no buffer swap (pure draw cost)"
                      : opt_interval ? "glutSwapBuffers, vsync on"
                                     : "glutSwapBuffers, vsync off" );
    printf( "\n" );
    printf( "    %-14s %16s %16s %18s\n", "strategy",
            "Character", "String", "save/restore" );
    printf( "    %-14s %16s %16s %18s\n", "",
            "ns/glyph", "ns/glyph", "ns/glyph" );
    for( i = 0; i < 2; i++ )
        printf( "    %-14s %16.1f %16.1f %18.1f\n",
                strategies[i], res[i].ns_char, res[i].ns_string, over[i] );

    /* Under vsync the per-glyph figure is dominated by the wait, so show what
     * the frame actually cost -- that is the number an application feels. */
    if( opt_swap )
    {
        printf( "\n" );
        printf( "    %-14s %16s %16s\n", "", "Character", "String" );
        printf( "    %-14s %16s %16s\n", "", "ms/frame", "ms/frame" );
        for( i = 0; i < 2; i++ )
            printf( "    %-14s %16.2f %16.2f\n", strategies[i],
                    res[i].ns_char   * glyphs_per_pass( ) * 1.0e-6,
                    res[i].ns_string * glyphs_per_pass( ) * 1.0e-6 );
    }
    printf( "\n" );

    /* String pays for just one save/restore per line either way, so it is very
     * nearly the same work under both strategies.  Comparing it across the two
     * child processes therefore measures the noise floor, not the patch: a
     * large number here means the two children saw different machine
     * conditions and the Character comparison above is not trustworthy. */
    printf( "  noise check: glutBitmapString differs by %.1f%% between the"
            " clientattrib and getset runs\n"
            "               (near-identical work in both, so this is roughly"
            " the measurement noise floor)\n",
            100.0 * ( res[0].ns_string - res[1].ns_string ) /
                    ( 0.5 * ( res[0].ns_string + res[1].ns_string ) ) );

    if( over[0] > 0.0 && over[1] > 0.0 )
        printf( "  save/restore    : %s is %.2fx cheaper per glyph\n",
                over[1] < over[0] ? "getset" : "clientattrib",
                over[1] < over[0] ? over[0] / over[1] : over[1] / over[0] );
    printf( "  glutBitmapCharacter : %s is %.2fx faster overall\n",
            res[1].ns_char < res[0].ns_char ? "getset" : "clientattrib",
            res[1].ns_char < res[0].ns_char
                ? res[0].ns_char / res[1].ns_char
                : res[1].ns_char / res[0].ns_char );
    printf( "\n" );
    fflush( stdout );
}

static int run_parent( int argc, char **argv )
{
    struct result res[2];
    int i;

    for( i = 0; i < 2; i++ )
    {
        fprintf( stderr, "  measuring %s ...\n", strategies[i] );
        if( !run_child( argv[0], argc, argv, strategies[i], &res[i] ) )
            return 1;
    }
    report_both( res );
    return 0;
}

/* --------------------------------------------------------------------- CLI */

static void usage( const char *argv0 )
{
    int i;
    fprintf( stderr,
             "Usage: %s [--seconds N] [--warmup N] [--lines N] [--cols N]\n"
             "          [--repeats N] [--font NAME] [--strategy clientattrib|getset]\n"
             "          [--swap] [--swap-interval N] [--hold] [--tsv]\n"
             "          [--micro] [--micro-iters N] [--sweep]\n"
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
        else if( !strcmp( a, "--swap" ) )
            opt_swap = 1;
        else if( !strcmp( a, "--swap-interval" ) && i + 1 < argc )
        {
            opt_interval = atoi( argv[++i] );
            opt_swap = 1;               /* an interval only bites on a swap */
        }
        else if( !strcmp( a, "--child" ) )
            opt_child = 1;
        else if( !strcmp( a, "--micro" ) )
            opt_micro = 1;
        else if( !strcmp( a, "--sweep" ) )
            opt_sweep = 1;
        else if( !strcmp( a, "--micro-iters" ) && i + 1 < argc )
        {
            micro_iters = atoi( argv[++i] );
            if( micro_iters < 1 )
                micro_iters = 1;
            opt_micro = 1;
        }
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
                fprintf( stderr, "bitmap_bench: unknown strategy '%s'\n",
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
                fprintf( stderr, "bitmap_bench: unknown font '%s'\n", name );
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

    /* A held window in a child would block the parent waiting on its output.
     * --hold is for eyeballing a single --strategy run. */
    if( opt_child )
        opt_hold = 0;
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
        fprintf( stderr, "bitmap_bench: out of memory\n" );
        exit( 1 );
    }
    for( i = 0; i < opt_cols; i++ )
        text_line[i] = alphabet[i % alen];
    text_line[opt_cols] = '\0';
}

int main( int argc, char **argv )
{
    char title[128];
    int win_w;

    if( !parse_args( argc, argv ) )
        return 1;

    /* --micro times both strategies itself, in this process, so it neither
     * needs nor wants a child per strategy. */
    if( !opt_strategy && !opt_micro && !opt_sweep )
        return run_parent( argc, argv );

    /* Must be set before the first bitmap-font call, which is where
     * fg_font.c resolves and caches it. */
    if( opt_strategy )
        setenv( "FREEGLUT_BITMAP_PIXEL_STORE", opt_strategy, 1 );

    build_text_line( );

    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB );

    /* Size the window to the workload *before* creating it.  Resizing
     * afterwards races: the layout below would be in effect while the window
     * still had its old size, putting the text outside the visible area.
     * The font metrics only need glutInit, not a window. */
    line_step = glutBitmapHeight( the_font( ) ) + 2;
    if( opt_sweep )
    {
        /* Tall/wide enough that the wrapped 1000-glyph string stays entirely
         * inside the window -- see the sweep comment on invalid raster pos. */
        opt_lines = SWEEP_ROWS + 1;
        opt_cols  = SWEEP_COLS;
        build_text_line( );
    }
    win_h = 24 + opt_lines * line_step;
    win_w = 16 + glutBitmapLength( the_font( ),
                                   (const unsigned char *)text_line );
    glutInitWindowSize( win_w, win_h );

    snprintf( title, sizeof( title ), "freeglut bitmap_bench -- %s",
              opt_micro ? "micro" : opt_sweep ? "sweep" : opt_strategy );
    glutCreateWindow( title );

    if( opt_sweep )
        build_sweep_text( );

    glutDisplayFunc( display );
    glutReshapeFunc( reshape );
    glutKeyboardFunc( keyboard );
    glutSwapInterval( opt_interval );
    glDisable( GL_DEPTH_TEST );

    glutMainLoop( );
    return 0;
}
