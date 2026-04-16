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

static FGCriterion fghMakeCriterion( FGCriterionComparison comparison, int value )
{
    FGCriterion criterion;

    criterion.comparison = comparison;
    criterion.value      = value;
    return criterion;
}

static FGCriterion fghResolveCriterion( FGCriterion criterion, FGCriterion defaultCriterion )
{
    return criterion.comparison == FG_UNSPECIFIED ? defaultCriterion : criterion;
}

static GLboolean fghCocoaUsesUnsupportedPixelMode( void )
{
    if ( fgState.DisplayMode & GLUT_INDEX )
        return GL_TRUE;

    if ( fgState.DisplayMode & GLUT_LUMINANCE )
        return GL_TRUE;

    /*
     * In the legacy glutInitDisplayMode bitmask, GLUT_SRGB aliases GLUT_AUX1.
     * GLUT_SRGB is always enabled in cocoa anyway, so we ignore it if it's set.
     * But the aliasing bug should still be fixed in the future to avoid confusion.
     */
#if 0
#ifdef GLUT_SRGB
    if ( fgState.DisplayMode & GLUT_SRGB )
        return GL_TRUE;
#endif
#endif

    return GL_FALSE;
}

static int fghWeightForCriterion( FGCriterion criterion, int minWeight, int maxWeight )
{
    switch ( criterion.comparison ) {
    case FG_EQ:
        return criterion.value;
    case FG_NEQ: {
        int candidate = maxWeight - criterion.value;

        if ( candidate != criterion.value && candidate > minWeight )
            return candidate;
        if ( maxWeight != criterion.value )
            return maxWeight;
        return minWeight;
    }
    case FG_LT:
    case FG_LTE:
        return minWeight;
    case FG_GT:
        return MIN( maxWeight, MAX( minWeight, criterion.value + 1 ) );
    case FG_GTE:
        return MIN( maxWeight, MAX( minWeight, criterion.value ) );
    case FG_MIN:
        return MIN( maxWeight, MAX( minWeight, criterion.value ) );
    case FG_UNSPECIFIED:
        return minWeight;
    case FG_NONE:
    default:
        return 0;
    }
}

static int fghBuildAttrsFromCriteria( NSOpenGLPixelFormatAttribute *attrs )
{
    FGDisplayStringCriteria *c = &fgState.DisplayStrCriteria;
    int                      n = 0;
    FGCriterion              red = fghResolveCriterion( c->red, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              green = fghResolveCriterion( c->green, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              blue = fghResolveCriterion( c->blue, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              alpha = fghResolveCriterion( c->alpha, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              accumRed = fghResolveCriterion( c->accumRed, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              accumGreen = fghResolveCriterion( c->accumGreen, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              accumBlue = fghResolveCriterion( c->accumBlue, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              accumAlpha = fghResolveCriterion( c->accumAlpha, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              depth = fghResolveCriterion( c->depth, fghMakeCriterion( FG_GTE, 12 ) );
    FGCriterion              stencil = c->stencil;
    FGCriterion              auxBuffers = fghResolveCriterion( c->auxBuffers, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              samples = fghResolveCriterion( c->samples, fghMakeCriterion( FG_LTE, 4 ) );

    attrs[n++] = NSOpenGLPFAAccelerated;
    attrs[n++] = NSOpenGLPFAClosestPolicy;

    {
        int colorBits = 0;

        colorBits = MAX( fghWeightForCriterion( red, 1, 16 ), colorBits );
        colorBits = MAX( fghWeightForCriterion( green, 1, 16 ), colorBits );
        colorBits = MAX( fghWeightForCriterion( blue, 1, 16 ), colorBits );
        colorBits *= 3;

        if ( colorBits > 0 ) {
            attrs[n++] = NSOpenGLPFAColorSize;
            attrs[n++] = colorBits;
        }
    }

    {
        int alphaBits = fghWeightForCriterion( alpha, 0, 16 );

        if ( alphaBits > 0 ) {
            attrs[n++] = NSOpenGLPFAAlphaSize;
            attrs[n++] = alphaBits;
        }
    }

    {
        int accumBits = 0;

        accumBits = MAX( fghWeightForCriterion( accumRed, 1, 32 ), accumBits );
        accumBits = MAX( fghWeightForCriterion( accumGreen, 1, 32 ), accumBits );
        accumBits = MAX( fghWeightForCriterion( accumBlue, 1, 32 ), accumBits );
        accumBits = MAX( fghWeightForCriterion( accumAlpha, 0, 32 ), accumBits );
        accumBits *= 4;

        if ( accumBits > 0 ) {
            attrs[n++] = NSOpenGLPFAAccumSize;
            attrs[n++] = accumBits;
        }
    }

    if ( depth.comparison != FG_NONE ) {
        attrs[n++] = NSOpenGLPFADepthSize;
        attrs[n++] = fghWeightForCriterion( depth, 12, 32 );
    }

    if ( stencil.comparison != FG_NONE ) {
        attrs[n++] = NSOpenGLPFAStencilSize;
        attrs[n++] = fghWeightForCriterion( stencil, 0, 8 );
    }

    if ( auxBuffers.comparison != FG_NONE ) {
        attrs[n++] = NSOpenGLPFAAuxBuffers;
        attrs[n++] = fghWeightForCriterion( auxBuffers, 1, 2 );
    }

    if ( samples.comparison != FG_NONE ) {
        int requestedSamples = fghWeightForCriterion( samples, 1, 16 );

        if ( requestedSamples > 0 ) {
            attrs[n++] = NSOpenGLPFAMultisample;
            attrs[n++] = NSOpenGLPFASampleBuffers;
            attrs[n++] = 1;
            attrs[n++] = NSOpenGLPFASamples;
            attrs[n++] = requestedSamples;
        }
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

static GLboolean fghEvaluateCriterion( FGCriterion criterion, int actualValue )
{
    switch ( criterion.comparison ) {
    case FG_NONE:
        return GL_TRUE;
    case FG_EQ:
        return actualValue == criterion.value;
    case FG_NEQ:
        return actualValue != criterion.value;
    case FG_LT:
        return actualValue < criterion.value;
    case FG_GT:
        return actualValue > criterion.value;
    case FG_LTE:
        return actualValue <= criterion.value;
    case FG_GTE:
        return actualValue >= criterion.value;
    case FG_MIN:
        return actualValue >= criterion.value;
    case FG_UNSPECIFIED:
        return actualValue > 0;
    default:
        return GL_FALSE;
    }
}

static GLboolean fghPixelFormatMatchesDisplayString( NSOpenGLPixelFormat *pixelFormat )
{
    FGDisplayStringCriteria *criteria = &fgState.DisplayStrCriteria;
    FGCriterion              red = fghResolveCriterion( criteria->red, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              green = fghResolveCriterion( criteria->green, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              blue = fghResolveCriterion( criteria->blue, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              alpha = fghResolveCriterion( criteria->alpha, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              accumRed = fghResolveCriterion( criteria->accumRed, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              accumGreen = fghResolveCriterion( criteria->accumGreen, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              accumBlue = fghResolveCriterion( criteria->accumBlue, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              accumAlpha = fghResolveCriterion( criteria->accumAlpha, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              depth = fghResolveCriterion( criteria->depth, fghMakeCriterion( FG_GTE, 12 ) );
    FGCriterion              auxBuffersCriterion = fghResolveCriterion( criteria->auxBuffers, fghMakeCriterion( FG_GTE, 1 ) );
    FGCriterion              samplesCriterion = fghResolveCriterion( criteria->samples, fghMakeCriterion( FG_LTE, 4 ) );
    int colorSize                     = fghGetPixelFormatValue( pixelFormat, NSOpenGLPFAColorSize );
    int alphaSize                     = fghGetPixelFormatValue( pixelFormat, NSOpenGLPFAAlphaSize );
    int accumSize                     = fghGetPixelFormatValue( pixelFormat, NSOpenGLPFAAccumSize );
    int depthSize                     = fghGetPixelFormatValue( pixelFormat, NSOpenGLPFADepthSize );
    int stencilSize                   = fghGetPixelFormatValue( pixelFormat, NSOpenGLPFAStencilSize );
    int auxBuffers                    = fghGetPixelFormatValue( pixelFormat, NSOpenGLPFAAuxBuffers );
    int samples                       = fghGetPixelFormatValue( pixelFormat, NSOpenGLPFASamples );
    int rgbColorSize                  = colorSize - alphaSize;

    if ( rgbColorSize < 0 )
        rgbColorSize = colorSize;

    if ( !fghEvaluateCriterion( red, rgbColorSize / 3 ) )
        return GL_FALSE;
    if ( !fghEvaluateCriterion( green, rgbColorSize / 3 ) )
        return GL_FALSE;
    if ( !fghEvaluateCriterion( blue, rgbColorSize / 3 ) )
        return GL_FALSE;
    if ( !fghEvaluateCriterion( alpha, alphaSize ) )
        return GL_FALSE;
    if ( !fghEvaluateCriterion( depth, depthSize ) )
        return GL_FALSE;
    if ( !fghEvaluateCriterion( criteria->stencil, stencilSize ) )
        return GL_FALSE;
    if ( !fghEvaluateCriterion( accumRed, accumSize / 4 ) )
        return GL_FALSE;
    if ( !fghEvaluateCriterion( accumGreen, accumSize / 4 ) )
        return GL_FALSE;
    if ( !fghEvaluateCriterion( accumBlue, accumSize / 4 ) )
        return GL_FALSE;
    if ( !fghEvaluateCriterion( accumAlpha, accumSize / 4 ) )
        return GL_FALSE;
    if ( !fghEvaluateCriterion( samplesCriterion, samples ) )
        return GL_FALSE;
    if ( !fghEvaluateCriterion( auxBuffersCriterion, auxBuffers ) )
        return GL_FALSE;
    if ( !fghEvaluateCriterion( criteria->buffer, colorSize + alphaSize ) )
        return GL_FALSE;

    return GL_TRUE;
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