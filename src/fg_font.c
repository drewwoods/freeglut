/*
 * Copyright (c) 1999-2000 Pawel W. Olszta. All Rights Reserved.
 * Written by Pawel W. Olszta, <olszta@sourceforge.net>
 * Creation date: Thu Dec 16 1999
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
 * PAWEL W. OLSZTA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/* Bitmap and stroke font drawing */

#include <GL/freeglut.h>
#include "fg_internal.h"

/* These are the font faces defined in fg_font_data.c file: */
extern SFG_Font fgFontFixed8x13;
extern SFG_Font fgFontFixed9x15;
extern SFG_Font fgFontHelvetica10;
extern SFG_Font fgFontHelvetica12;
extern SFG_Font fgFontHelvetica18;
extern SFG_Font fgFontTimesRoman10;
extern SFG_Font fgFontTimesRoman24;
extern SFG_StrokeFont fgStrokeRoman;
extern SFG_StrokeFont fgStrokeMonoRoman;


/*
 * Matches a font ID with a SFG_Font structure pointer.
 * This was changed to match the GLUT header style.
 */
SFG_Font* fghFontByID( void* font )
{
    if( font == GLUT_BITMAP_8_BY_13        )
        return &fgFontFixed8x13;
    if( font == GLUT_BITMAP_9_BY_15        )
        return &fgFontFixed9x15;
    if( font == GLUT_BITMAP_HELVETICA_10   )
        return &fgFontHelvetica10;
    if( font == GLUT_BITMAP_HELVETICA_12   )
        return &fgFontHelvetica12;
    if( font == GLUT_BITMAP_HELVETICA_18   )
        return &fgFontHelvetica18;
    if( font == GLUT_BITMAP_TIMES_ROMAN_10 )
        return &fgFontTimesRoman10;
    if( font == GLUT_BITMAP_TIMES_ROMAN_24 )
        return &fgFontTimesRoman24;

    return 0;
}

/*
 * Matches a font ID with a SFG_StrokeFont structure pointer.
 * This was changed to match the GLUT header style.
 */
static SFG_StrokeFont* fghStrokeByID( void* font )
{
    if( font == GLUT_STROKE_ROMAN      )
        return &fgStrokeRoman;
    if( font == GLUT_STROKE_MONO_ROMAN )
        return &fgStrokeMonoRoman;

    return 0;
}

/*
 * Bitmap fonts must leave the six GL_UNPACK_* pixel-store values they touch
 * exactly as they found them.  There are two ways to do that:
 *
 *   PIXEL_STORE_CLIENT_ATTRIB  glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT)
 *                              / glPopClientAttrib()   -- needs GL 1.1
 *   PIXEL_STORE_GET_SET        six glGetIntegerv() up front, six
 *                              glPixelStorei() to put them back
 *
 * Apple's OpenGL compatibility implementation services the explicit
 * queries/restores far more cheaply than its client-attribute stack, so
 * Cocoa defaults to GET_SET and everyone else defaults to CLIENT_ATTRIB.
 *
 * Setting FREEGLUT_BITMAP_PIXEL_STORE to "clientattrib" or "getset" in the
 * environment overrides the default at run time.  That exists so a single
 * build can benchmark both strategies on any platform -- see
 * progs/demos/bitmap_bench/.  An unrecognised value is ignored.
 */
#define FGH_PIXEL_STORE_CLIENT_ATTRIB 0
#define FGH_PIXEL_STORE_GET_SET       1

#if defined(GL_VERSION_1_1) && !TARGET_HOST_MACOS_COCOA
#  define FGH_PIXEL_STORE_DEFAULT FGH_PIXEL_STORE_CLIENT_ATTRIB
#else
#  define FGH_PIXEL_STORE_DEFAULT FGH_PIXEL_STORE_GET_SET
#endif

typedef struct
{
    int   strategy;
    GLint swbytes, lsbfirst, rowlen, skiprows, skippix, align;
} fghPixelStoreState;

static int fghPixelStoreStrategy( void )
{
    static int strategy = -1;

    if( strategy < 0 )
    {
        const char* env = getenv( "FREEGLUT_BITMAP_PIXEL_STORE" );

        strategy = FGH_PIXEL_STORE_DEFAULT;
#ifdef GL_VERSION_1_1
        if( env && !strcmp( env, "clientattrib" ) )
            strategy = FGH_PIXEL_STORE_CLIENT_ATTRIB;
#endif
        if( env && !strcmp( env, "getset" ) )
            strategy = FGH_PIXEL_STORE_GET_SET;
    }

    return strategy;
}

/* Save the unpack state the bitmap font paths are about to clobber, then
 * set it to what glBitmap() needs.
 */
static void fghPixelStoreSave( fghPixelStoreState* state )
{
    state->strategy = fghPixelStoreStrategy( );

#ifdef GL_VERSION_1_1
    if( state->strategy == FGH_PIXEL_STORE_CLIENT_ATTRIB )
        glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT );
    else
#endif
    {
        glGetIntegerv( GL_UNPACK_SWAP_BYTES,  &state->swbytes  );
        glGetIntegerv( GL_UNPACK_LSB_FIRST,   &state->lsbfirst );
        glGetIntegerv( GL_UNPACK_ROW_LENGTH,  &state->rowlen   );
        glGetIntegerv( GL_UNPACK_SKIP_ROWS,   &state->skiprows );
        glGetIntegerv( GL_UNPACK_SKIP_PIXELS, &state->skippix  );
        glGetIntegerv( GL_UNPACK_ALIGNMENT,   &state->align    );
    }

    glPixelStorei( GL_UNPACK_SWAP_BYTES,  GL_FALSE );
    glPixelStorei( GL_UNPACK_LSB_FIRST,   GL_FALSE );
    glPixelStorei( GL_UNPACK_ROW_LENGTH,  0        );
    glPixelStorei( GL_UNPACK_SKIP_ROWS,   0        );
    glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0        );
    glPixelStorei( GL_UNPACK_ALIGNMENT,   1        );
}

static void fghPixelStoreRestore( const fghPixelStoreState* state )
{
#ifdef GL_VERSION_1_1
    if( state->strategy == FGH_PIXEL_STORE_CLIENT_ATTRIB )
    {
        glPopClientAttrib( );
        return;
    }
#endif
    glPixelStorei( GL_UNPACK_SWAP_BYTES,  state->swbytes  );
    glPixelStorei( GL_UNPACK_LSB_FIRST,   state->lsbfirst );
    glPixelStorei( GL_UNPACK_ROW_LENGTH,  state->rowlen   );
    glPixelStorei( GL_UNPACK_SKIP_ROWS,   state->skiprows );
    glPixelStorei( GL_UNPACK_SKIP_PIXELS, state->skippix  );
    glPixelStorei( GL_UNPACK_ALIGNMENT,   state->align    );
}

/* Draw a bitmap character */
void FGAPIENTRY glutBitmapCharacter( void* fontID, int character )
{
    fghPixelStoreState pixelStore;
    const GLubyte* face;
    SFG_Font* font;
    FREEGLUT_EXIT_IF_NOT_INITIALISED ( "glutBitmapCharacter" );
    font = fghFontByID( fontID );
    if (!font)
    {
        fgWarning("glutBitmapCharacter: bitmap font 0x%08x not found. Make sure you're not passing a stroke font.\n",fontID);
        return;
    }
    freeglut_return_if_fail( ( character >= 1 )&&( character < 256 ) );

    /* Find the character we want to draw */
    face = font->Characters[ character ];

    fghPixelStoreSave( &pixelStore );
    glBitmap(
        face[ 0 ], font->Height,      /* The bitmap's width and height  */
        font->xorig, font->yorig,     /* The origin in the font glyph   */
        ( float )( face[ 0 ] ), 0.0,  /* The raster advance -- inc. x,y */
        ( face + 1 )                  /* The packed bitmap data...      */
    );
    fghPixelStoreRestore( &pixelStore );
}

void FGAPIENTRY glutBitmapString( void* fontID, const unsigned char *string )
{
    unsigned char c;
    float x = 0.0f ;
    SFG_Font* font;
    fghPixelStoreState pixelStore;
    FREEGLUT_EXIT_IF_NOT_INITIALISED ( "glutBitmapString" );
    font = fghFontByID( fontID );
    if (!font)
    {
        fgWarning("glutBitmapString: bitmap font 0x%08x not found. Make sure you're not passing a stroke font.\n",fontID);
        return;
    }
    if ( !string || ! *string )
        return;

    fghPixelStoreSave( &pixelStore );

    /*
     * Step through the string, drawing each character.
     * A newline will simply translate the next character's insertion
     * point back to the start of the line and down one line.
     */
    while( ( c = *string++) )
        if( c == '\n' )
        {
            glBitmap ( 0, 0, 0, 0, -x, (float) -font->Height, NULL );
            x = 0.0f;
        }
        else  /* Not an EOL, draw the bitmap character */
        {
            const GLubyte* face = font->Characters[ c ];

            glBitmap(
                face[ 0 ], font->Height,     /* Bitmap's width and height    */
                font->xorig, font->yorig,    /* The origin in the font glyph */
                ( float )( face[ 0 ] ), 0.0, /* The raster advance; inc. x,y */
                ( face + 1 )                 /* The packed bitmap data...    */
            );

            x += ( float )( face[ 0 ] );
        }

    fghPixelStoreRestore( &pixelStore );
}

/* Returns the width in pixels of a font's character */
int FGAPIENTRY glutBitmapWidth( void* fontID, int character )
{
    SFG_Font* font;
    FREEGLUT_EXIT_IF_NOT_INITIALISED ( "glutBitmapWidth" );
    freeglut_return_val_if_fail( character > 0 && character < 256, 0 );
    font = fghFontByID( fontID );
    if (!font)
    {
        fgWarning("glutBitmapWidth: bitmap font 0x%08x not found. Make sure you're not passing a stroke font.\n",fontID);
        return 0;
    }
    return *( font->Characters[ character ] );
}

/* Return the width of a string drawn using a bitmap font */
int FGAPIENTRY glutBitmapLength( void* fontID, const unsigned char* string )
{
    unsigned char c;
    int length = 0, this_line_length = 0;
    SFG_Font* font;
    FREEGLUT_EXIT_IF_NOT_INITIALISED ( "glutBitmapLength" );
    font = fghFontByID( fontID );
    if (!font)
    {
        fgWarning("glutBitmapLength: bitmap font 0x%08x not found. Make sure you're not passing a stroke font.\n",fontID);
        return 0;
    }
    if ( !string || ! *string )
        return 0;

    while( ( c = *string++) )
    {
        if( c != '\n' )/* Not an EOL, increment length of line */
            this_line_length += *( font->Characters[ c ]);
        else  /* EOL; reset the length of this line */
        {
            if( length < this_line_length )
                length = this_line_length;
            this_line_length = 0;
        }
    }
    if ( length < this_line_length )
        length = this_line_length;

    return length;
}

/* Returns the height of a bitmap font */
int FGAPIENTRY glutBitmapHeight( void* fontID )
{
    SFG_Font* font;
    FREEGLUT_EXIT_IF_NOT_INITIALISED ( "glutBitmapHeight" );
    font = fghFontByID( fontID );
    if (!font)
    {
        fgWarning("glutBitmapHeight: bitmap font 0x%08x not found. Make sure you're not passing a stroke font.\n",fontID);
        return 0;
    }
    return font->Height;
}

/* Draw a stroke character */
void FGAPIENTRY glutStrokeCharacter( void* fontID, int character )
{
    const SFG_StrokeChar *schar;
    const SFG_StrokeStrip *strip;
    int i, j;
    SFG_StrokeFont* font;
    FREEGLUT_EXIT_IF_NOT_INITIALISED ( "glutStrokeCharacter" );
    font = fghStrokeByID( fontID );
    if (!font)
    {
        fgWarning("glutStrokeCharacter: stroke font 0x%08x not found. Make sure you're not passing a bitmap font.\n",fontID);
        return;
    }
    freeglut_return_if_fail( character >= 0 );
    freeglut_return_if_fail( character < font->Quantity );

    schar = font->Characters[ character ];
    freeglut_return_if_fail( schar );
    strip = schar->Strips;

    for( i = 0; i < schar->Number; i++, strip++ )
    {
        glBegin( GL_LINE_STRIP );
        for( j = 0; j < strip->Number; j++ )
            glVertex2f( strip->Vertices[ j ].X, strip->Vertices[ j ].Y );
        glEnd( );

        if (fgState.StrokeFontDrawJoinDots)
        {
            glBegin( GL_POINTS );
            for( j = 0; j < strip->Number; j++ )
                glVertex2f( strip->Vertices[ j ].X, strip->Vertices[ j ].Y );
            glEnd( );
        }
    }
    glTranslatef( schar->Right, 0.0, 0.0 );
}

void FGAPIENTRY glutStrokeString( void* fontID, const unsigned char *string )
{
    unsigned char c;
    int i, j;
    float length = 0.0;
    SFG_StrokeFont* font;
    FREEGLUT_EXIT_IF_NOT_INITIALISED ( "glutStrokeString" );
    font = fghStrokeByID( fontID );
    if (!font)
    {
        fgWarning("glutStrokeString: stroke font 0x%08x not found. Make sure you're not passing a bitmap font.\n",fontID);
        return;
    }
    if ( !string || ! *string )
        return;

    /*
     * Step through the string, drawing each character.
     * A newline will simply translate the next character's insertion
     * point back to the start of the line and down one line.
     */
    while( ( c = *string++) )
        if( c < font->Quantity )
        {
            if( c == '\n' )
            {
                glTranslatef ( -length, -( float )( font->Height ), 0.0 );
                length = 0.0;
            }
            else  /* Not an EOL, draw the bitmap character */
            {
                const SFG_StrokeChar *schar = font->Characters[ c ];
                if( schar )
                {
                    const SFG_StrokeStrip *strip = schar->Strips;

                    for( i = 0; i < schar->Number; i++, strip++ )
                    {
                        glBegin( GL_LINE_STRIP );
                        for( j = 0; j < strip->Number; j++ )
                            glVertex2f( strip->Vertices[ j ].X,
                                        strip->Vertices[ j ].Y);

                        glEnd( );
                    }

                    length += schar->Right;
                    glTranslatef( schar->Right, 0.0, 0.0 );
                }
            }
        }
}

/* Return the width in pixels of a stroke character */
GLfloat FGAPIENTRY glutStrokeWidthf( void* fontID, int character )
{
    const SFG_StrokeChar *schar;
    SFG_StrokeFont* font;
    FREEGLUT_EXIT_IF_NOT_INITIALISED ( "glutStrokeWidth" );
    font = fghStrokeByID( fontID );
    if (!font)
    {
        fgWarning("glutStrokeWidth: stroke font 0x%08x not found. Make sure you're not passing a bitmap font.\n",fontID);
        return 0;
    }
    freeglut_return_val_if_fail( ( character >= 0 ) &&
                                 ( character < font->Quantity ),
                                 0
    );
    schar = font->Characters[ character ];
    freeglut_return_val_if_fail( schar, 0 );

    return schar->Right;
}
int FGAPIENTRY glutStrokeWidth(void* fontID, int character)
{
    return ( int )( glutStrokeWidthf(fontID,character) + 0.5f );
}

/* Return the width of a string drawn using a stroke font */
GLfloat FGAPIENTRY glutStrokeLengthf( void* fontID, const unsigned char* string )
{
    unsigned char c;
    GLfloat length = 0.0;
    GLfloat this_line_length = 0.0;
    SFG_StrokeFont* font;
    FREEGLUT_EXIT_IF_NOT_INITIALISED ( "glutStrokeLength" );
    font = fghStrokeByID( fontID );
    if (!font)
    {
        fgWarning("glutStrokeLength: stroke font 0x%08x not found. Make sure you're not passing a bitmap font.\n",fontID);
        return 0;
    }
    if ( !string || ! *string )
        return 0;

    while( ( c = *string++) )
        if( c < font->Quantity )
        {
            if( c == '\n' ) /* EOL; reset the length of this line */
            {
                if( length < this_line_length )
                    length = this_line_length;
                this_line_length = 0.0;
            }
            else  /* Not an EOL, increment the length of this line */
            {
                const SFG_StrokeChar *schar = font->Characters[ c ];
                if( schar )
                    this_line_length += schar->Right;
            }
        }
    if( length < this_line_length )
        length = this_line_length;
    return length;
}
int FGAPIENTRY glutStrokeLength( void* fontID, const unsigned char* string )
{
    return( int )( glutStrokeLengthf(fontID,string) + 0.5f );
}

/* Returns the height of a stroke font */
GLfloat FGAPIENTRY glutStrokeHeight( void* fontID )
{
    SFG_StrokeFont* font;
    FREEGLUT_EXIT_IF_NOT_INITIALISED ( "glutStrokeHeight" );
    font = fghStrokeByID( fontID );
    if (!font)
    {
        fgWarning("glutStrokeHeight: stroke font 0x%08x not found. Make sure you're not passing a bitmap font.\n",fontID);
        return 0.f;
    }
    return font->Height;
}
