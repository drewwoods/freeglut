#pragma once

#include <GL/FreeGLUT.h>

#include "utils.h"

static struct Timer ftimer;
static uint32_t     frames;

static int   idx          = 0;
static float colours[][3] = {
    { 1.0f, 0.0f, 0.0f }, /* Red */
    { 0.0f, 0.0f, 1.0f }, /* Blue */
    { 0.0f, 1.0f, 1.0f }, /* Cyan */
    { 1.0f, 1.0f, 0.0f }, /* Yellow */
    { 0.5f, 0.5f, 0.5f }, /* Grey */
    { 1.0f, 0.0f, 1.0f }, /* Magenta */
};

enum { ROWS = 16, COLS = 16, BLOCKS_PER_SCREEN = ( ROWS * COLS ) };

#define ARR_LEN( arr ) ( sizeof( arr ) / sizeof( arr[0] ) )

static void draw_block( void )
{
    uint32_t row = ( frames / COLS ) % ROWS;
    uint32_t col = frames % COLS;

    if ( ( col + row ) % 2 )
        glColor3fv( &colours[idx][0] );
    else
        glColor3fv( &colours[idx ^ 1][0] );

    glRecti( col, row, col + 1, row + 1 );
}

static void drawBlocks( void )
{
    static uint32_t last_switch;
    if ( !last_switch ) {
        last_switch = clock_gettime_nsec_np( CLOCK_REALTIME );
    }

    draw_block( );

    frames++;

    /* Update the block colors when we reach the end of a screen */
    if ( frames % BLOCKS_PER_SCREEN == 0 ) {
        idx = ( idx + 2 ) % ARR_LEN( colours );

        uint32_t now = clock_gettime_nsec_np( CLOCK_REALTIME );
        float    dur = ( now - last_switch ) / 1e6;
        float    avg = dur / BLOCKS_PER_SCREEN;
        printf( "Switching colours -- avg frametime %.2fms (%.1fFPS)\n", avg, 1e3 / avg );
        last_switch = now;
    }
}

static void block_reshape( int width, int height )
{
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );
    glOrtho( 0, ROWS, 0, COLS, -1, 1 );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity( );
}

static void block_render( void ( ^flush )( void ) )
{
    start_scope( __FUNCTION__ );
    const char *fps = get_fps( 1e9 );

    if ( fps ) {
        printf( "%s\n", fps );
        EMIT_EVENT( "%s", fps );
    }

    drawBlocks( );

    // Present the frame
    uint32_t start = clock_gettime_nsec_np( CLOCK_REALTIME );
    start_scope( "FlushBuffer" );
    tstart( &ftimer );
    flush( );
    const char *msg = tend( &ftimer );
    if ( msg )
        printf( "%15s -- %s\n", "Flush Time", msg );
    EMIT_EVENT( "FlushBuffer: %.2f ms", ( clock_gettime_nsec_np( CLOCK_REALTIME ) - start ) / 1e6 );
    end_scope( "FlushBuffer" );
    end_scope( __FUNCTION__ );
}