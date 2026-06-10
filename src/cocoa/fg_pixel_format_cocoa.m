/*
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

#define FREEGLUT_BUILDING_LIB
#include <GL/freeglut.h>
#include "../fg_internal.h"

#import <Cocoa/Cocoa.h>

#include "fg_pixel_format_cocoa.h"

enum { FG_COCOA_MAX_PIXEL_FORMAT_ATTRS = 128 };

static GLboolean fghCocoaUsesUnsupportedPixelMode( void )
{
    if ( fgState.DisplayMode & GLUT_INDEX )
        return GL_TRUE;

    if ( fgState.DisplayMode & GLUT_LUMINANCE )
        return GL_TRUE;

    /* GLUT_SRGB is always satisfiable on macOS: the framebuffer is
     * sRGB-capable and the app opts in with glEnable(GL_FRAMEBUFFER_SRGB). */

    return GL_FALSE;
}

/* Smallest value of a capability that can satisfy one criterion, clamped to
 * what the platform can plausibly provide. For < and <= this is the bound
 * itself (they prefer MORE under the bound, per GLUT's findMatch scoring);
 * NSOpenGLPixelFormat then rounds to the closest supported value. */
static int fghWeightForCriterion( FGCriterion criterion, int maxWeight )
{
    switch ( criterion.comparison ) {
    case FG_EQ:
        return criterion.value;
    case FG_NEQ:
        /* Anything but the value; ask for nothing unless 0 is forbidden. */
        return ( criterion.value == 0 ) ? 1 : 0;
    case FG_LT:
        return MAX( 0, MIN( maxWeight, criterion.value - 1 ) );
    case FG_LTE:
        return MAX( 0, MIN( maxWeight, criterion.value ) );
    case FG_GT:
        return MIN( maxWeight, criterion.value + 1 );
    case FG_GTE:
    case FG_MIN:
        return MIN( maxWeight, MAX( 0, criterion.value ) );
    case FG_NONE:
    case FG_UNSPECIFIED:
    default:
        return 0;
    }
}

/* NSOpenGLPixelFormat has no enumeration API; it returns one closest format,
 * so the request must be a single value per capability. Use the largest
 * weight over all entries for the capability (user tokens plus the appended
 * unspecified-capability defaults), which is the smallest request that can
 * satisfy every criterion at once. The authoritative accept/reject test
 * remains fghCriteriaPass() over the full ordered entry list afterwards. */
static int fghCapabilityWeight( FGCapability capability, int maxWeight )
{
    const FGDisplayStringCriteria *c = &fgState.DisplayStrCriteria;
    int weight = 0;
    int i;

    for ( i = 0; i < c->count; i++ ) {
        if ( c->entries[i].capability != capability )
            continue;
        weight = MAX( weight, fghWeightForCriterion( c->entries[i].criterion, maxWeight ) );
    }
    return weight;
}

static int fghBuildAttrsFromCriteria( NSOpenGLPixelFormatAttribute *attrs )
{
    int n = 0;
    int colorBits, alphaBits, accumBits, depthBits, stencilBits, auxBuffers, samples;

    colorBits = MAX( fghCapabilityWeight( FG_CAP_RED, 16 ),
                MAX( fghCapabilityWeight( FG_CAP_GREEN, 16 ),
                     fghCapabilityWeight( FG_CAP_BLUE, 16 ) ) ) * 3;
    alphaBits = fghCapabilityWeight( FG_CAP_ALPHA, 16 );

    accumBits = MAX( fghCapabilityWeight( FG_CAP_ACCUM_RED, 32 ),
                MAX( fghCapabilityWeight( FG_CAP_ACCUM_GREEN, 32 ),
                MAX( fghCapabilityWeight( FG_CAP_ACCUM_BLUE, 32 ),
                     fghCapabilityWeight( FG_CAP_ACCUM_ALPHA, 32 ) ) ) ) * 4;

    depthBits   = fghCapabilityWeight( FG_CAP_DEPTH, 32 );
    stencilBits = fghCapabilityWeight( FG_CAP_STENCIL, 8 );
    auxBuffers  = fghCapabilityWeight( FG_CAP_AUX, 4 );
    samples     = fghCapabilityWeight( FG_CAP_SAMPLES, 16 );

    attrs[n++] = NSOpenGLPFAAccelerated;
    attrs[n++] = NSOpenGLPFAClosestPolicy;

    if ( colorBits > 0 ) {
        attrs[n++] = NSOpenGLPFAColorSize;
        attrs[n++] = colorBits;
    }
    if ( alphaBits > 0 ) {
        attrs[n++] = NSOpenGLPFAAlphaSize;
        attrs[n++] = alphaBits;
    }
    if ( accumBits > 0 ) {
        attrs[n++] = NSOpenGLPFAAccumSize;
        attrs[n++] = accumBits;
    }
    if ( depthBits > 0 ) {
        attrs[n++] = NSOpenGLPFADepthSize;
        attrs[n++] = depthBits;
    }
    if ( stencilBits > 0 ) {
        attrs[n++] = NSOpenGLPFAStencilSize;
        attrs[n++] = stencilBits;
    }
    if ( auxBuffers > 0 ) {
        attrs[n++] = NSOpenGLPFAAuxBuffers;
        attrs[n++] = auxBuffers;
    }
    if ( samples > 0 ) {
        attrs[n++] = NSOpenGLPFAMultisample;
        attrs[n++] = NSOpenGLPFASampleBuffers;
        attrs[n++] = 1;
        attrs[n++] = NSOpenGLPFASamples;
        attrs[n++] = samples;
    }

    return n;
}

static int fghBuildAttrsFromDisplayMode( NSOpenGLPixelFormatAttribute *attrs )
{
    int n = 0;

    attrs[n++] = NSOpenGLPFAAccelerated;
    attrs[n++] = NSOpenGLPFAColorSize;
    attrs[n++] = 24;
    attrs[n++] = NSOpenGLPFAAlphaSize;
    attrs[n++] = 8;

    if ( fgState.DisplayMode & GLUT_DEPTH ) {
        attrs[n++] = NSOpenGLPFADepthSize;
        attrs[n++] = 24;
    }
    if ( fgState.DisplayMode & GLUT_STENCIL ) {
        attrs[n++] = NSOpenGLPFAStencilSize;
        attrs[n++] = 8;
    }
    if ( fgState.DisplayMode & GLUT_ACCUM ) {
        attrs[n++] = NSOpenGLPFAAccumSize;
        attrs[n++] = 32;
    }
    if ( fgState.DisplayMode & GLUT_AUX ) {
        attrs[n++] = NSOpenGLPFAAuxBuffers;
        attrs[n++] = fghNumberOfAuxBuffersRequested( );
    }
    if ( fgState.DisplayMode & GLUT_MULTISAMPLE ) {
        attrs[n++] = NSOpenGLPFAMultisample;
        attrs[n++] = NSOpenGLPFASampleBuffers;
        attrs[n++] = 1;
        attrs[n++] = NSOpenGLPFASamples;
        attrs[n++] = fgState.SampleNumber;
    }

    return n;
}

static void fghBuildPixelFormatAttrs( NSOpenGLPixelFormatAttribute *attrs, GLboolean isMenu )
{
    int attrIndex = fghBuildAttrsFromDisplayMode( attrs );

    if ( fgState.DisplayStrCriteria.haveDisplayString )
        attrIndex = fghBuildAttrsFromCriteria( attrs );

    if ( fgState.DisplayMode & GLUT_DOUBLE )
        attrs[attrIndex++] = NSOpenGLPFADoubleBuffer;
    if ( fgState.DisplayMode & GLUT_STEREO )
        attrs[attrIndex++] = NSOpenGLPFAStereo;

    attrs[attrIndex++] = NSOpenGLPFAOpenGLProfile;
    if ( fgState.MajorVersion == 3 && !isMenu )
        attrs[attrIndex++] = NSOpenGLProfileVersion3_2Core;
    else if ( fgState.MajorVersion == 4 && !isMenu )
        attrs[attrIndex++] = NSOpenGLProfileVersion4_1Core;
    else
        attrs[attrIndex++] = NSOpenGLProfileVersionLegacy;

    attrs[attrIndex++] = 0;
}

static int fghGetPixelFormatValue( NSOpenGLPixelFormat *pixelFormat, NSOpenGLPixelFormatAttribute attribute )
{
    GLint value = 0;

    [pixelFormat getValues:&value forAttribute:attribute forVirtualScreen:0];
    return value;
}

static GLboolean fghPixelFormatMatchesDisplayString( NSOpenGLPixelFormat *pixelFormat )
{
    int values[FG_CAP_COUNT];
    int colorSize    = fghGetPixelFormatValue( pixelFormat, NSOpenGLPFAColorSize );
    int alphaSize    = fghGetPixelFormatValue( pixelFormat, NSOpenGLPFAAlphaSize );
    int accumSize    = fghGetPixelFormatValue( pixelFormat, NSOpenGLPFAAccumSize );
    int depthSize    = fghGetPixelFormatValue( pixelFormat, NSOpenGLPFADepthSize );
    int stencilSize  = fghGetPixelFormatValue( pixelFormat, NSOpenGLPFAStencilSize );
    int auxBuffers   = fghGetPixelFormatValue( pixelFormat, NSOpenGLPFAAuxBuffers );
    int samples      = fghGetPixelFormatValue( pixelFormat, NSOpenGLPFASamples );
    int rgbColorSize = colorSize - alphaSize;

    /* NSOpenGLPFAColorSize sometimes includes alpha; recover the per-channel
     * RGB size as best we can for the shared comparison. */
    if ( rgbColorSize < 0 )
        rgbColorSize = colorSize;

    values[FG_CAP_RED]         = rgbColorSize / 3;
    values[FG_CAP_GREEN]       = rgbColorSize / 3;
    values[FG_CAP_BLUE]        = rgbColorSize / 3;
    values[FG_CAP_ALPHA]       = alphaSize;
    values[FG_CAP_DEPTH]       = depthSize;
    values[FG_CAP_STENCIL]     = stencilSize;
    values[FG_CAP_ACCUM_RED]   = accumSize / 4;
    values[FG_CAP_ACCUM_GREEN] = accumSize / 4;
    values[FG_CAP_ACCUM_BLUE]  = accumSize / 4;
    values[FG_CAP_ACCUM_ALPHA] = accumSize / 4;
    values[FG_CAP_SAMPLES]     = samples;
    values[FG_CAP_AUX]         = auxBuffers;
    values[FG_CAP_BUFFER]      = colorSize + alphaSize;

    /* Exact comparators ("=") must match exactly per the man page. macOS
     * reports a fixed accumulation precision (32 bits/channel), so e.g.
     * "acca=16" is genuinely impossible here while "acca~16" / "acca>=16"
     * are satisfied -- this is the spec-correct behaviour and is enforced
     * uniformly by the shared filter. */
    return fghCriteriaPass( &fgState.DisplayStrCriteria, values );
}

static GLboolean fghPixelFormatMatchesRequestedFlags( NSOpenGLPixelFormat *pixelFormat )
{
    if ( fgState.DisplayMode & GLUT_DOUBLE ) {
        if ( !fghGetPixelFormatValue( pixelFormat, NSOpenGLPFADoubleBuffer ) )
            return GL_FALSE;
    }

    if ( fgState.DisplayMode & GLUT_STEREO ) {
        if ( !fghGetPixelFormatValue( pixelFormat, NSOpenGLPFAStereo ) )
            return GL_FALSE;
    }

    return GL_TRUE;
}

GLboolean fgCocoaIsValidContextRequest( int majorVersion, int minorVersion, int contextFlags, int contextProfile )
{
    (void)contextFlags;

    if ( majorVersion < 2 || ( majorVersion == 2 && minorVersion <= 1 ) ) {
        if ( contextProfile != 0 && contextProfile != GLUT_COMPATIBILITY_PROFILE )
            return GL_FALSE;
        return GL_TRUE;
    }

    if ( ( majorVersion == 3 && minorVersion >= 2 ) || ( majorVersion == 4 && minorVersion <= 1 ) )
        return contextProfile == GLUT_CORE_PROFILE;

    if ( majorVersion == 3 && minorVersion < 2 )
        return GL_FALSE;

    if ( majorVersion > 4 || ( majorVersion == 4 && minorVersion > 1 ) )
        return GL_FALSE;

    return GL_FALSE;
}

NSOpenGLPixelFormat *fgCocoaCreatePixelFormat( GLboolean isMenu )
{
    NSOpenGLPixelFormatAttribute attrs[FG_COCOA_MAX_PIXEL_FORMAT_ATTRS];
    NSOpenGLPixelFormat         *pixelFormat;

    if ( fghCocoaUsesUnsupportedPixelMode( ) )
        return nil;

    if ( !fgCocoaIsValidContextRequest(
             fgState.MajorVersion, fgState.MinorVersion, fgState.ContextFlags, fgState.ContextProfile ) ) {
        return nil;
    }

    fghBuildPixelFormatAttrs( attrs, isMenu );

    pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if ( !pixelFormat )
        return nil;

    if ( fgState.DisplayStrCriteria.haveDisplayString && !fghPixelFormatMatchesDisplayString( pixelFormat ) ) {
        [pixelFormat release];
        return nil;
    }

    if ( !fghPixelFormatMatchesRequestedFlags( pixelFormat ) ) {
        [pixelFormat release];
        return nil;
    }

    return pixelFormat;
}

GLboolean fgCocoaIsDisplayModePossible( GLboolean isMenu )
{
    NSOpenGLPixelFormat *pixelFormat = fgCocoaCreatePixelFormat( isMenu );

    if ( !pixelFormat )
        return GL_FALSE;

    [pixelFormat release];
    return GL_TRUE;
}
