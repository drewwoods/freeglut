#define FREEGLUT_BUILDING_LIB
#include <GL/freeglut.h>
#include "../fg_internal.h"
#include "../fg_dstr.h"

#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#include <limits.h>

static int gWeightsLoaded = 0;
static int gMinStencil = INT_MAX, gMaxStencil = 0;
static int gMinDepth = INT_MAX, gMaxDepth = 0;
static int gMaxAuxBuffers = 0;
static int gMinSamples = INT_MAX, gMaxSamples = 0;
static int gMaxColor = 0, gMaxAlpha = 0;
static int gMaxAccumColor = 0, gMaxAccumAlpha = 0;

#define FG_COCOA_MAX_BITS 17
static const char gBitTable[FG_COCOA_MAX_BITS] = {
    0, 1, 2, 3, 4, 5, 6, 8, 10, 12, 16, 24, 32, 48, 64, 96, 128
};

#define FG_COCOA_MAX_COLOR_MODES 24
static const char gColorModeTable[FG_COCOA_MAX_COLOR_MODES][2] = {
    { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 4, 0 }, { 4, 4 },
    { 4, 8 }, { 5, 0 }, { 5, 1 }, { 5, 8 }, { 6, 0 }, { 6, 8 }, { 8, 0 }, { 8, 8 },
    { 8, 8 }, { 10, 0 }, { 10, 2 }, { 10, 8 }, { 12, 0 }, { 12, 12 }, { 16, 0 }, { 16, 16 }
};

static int fghMinInt( int a, int b )
{
    return a < b ? a : b;
}

static int fghMaxInt( int a, int b )
{
    return a > b ? a : b;
}

static int fghMaskToMinWeight( int mask )
{
    int i;
    int value = INT_MAX;

    for( i = 0; i < FG_COCOA_MAX_BITS; ++i )
        if( mask & ( 1L << i ) )
            value = fghMinInt( value, (int)gBitTable[i] );

    return value;
}

static int fghMaskToMaxWeight( int mask )
{
    int i;
    int value = 0;

    for( i = 0; i < FG_COCOA_MAX_BITS; ++i )
        if( mask & ( 1L << i ) )
            value = fghMaxInt( value, (int)gBitTable[i] );

    return value;
}

static void fghMaskToMaxColorWeight( int mask, int *colorBits, int *alphaBits )
{
    int i;
    int color = 0;
    int alpha = 0;

    for( i = 0; i < FG_COCOA_MAX_COLOR_MODES; ++i )
        if( mask & ( 1L << i ) )
            color = fghMaxInt( color, (int)gColorModeTable[i][0] );
    for( i = 0; i < FG_COCOA_MAX_COLOR_MODES; ++i )
        if( mask & ( 1L << i ) )
            alpha = fghMaxInt( alpha, (int)gColorModeTable[i][1] );

    *colorBits = color;
    *alphaBits = alpha;
}

static void fghLoadRendererWeights( void )
{
    CGLError err;
    CGLRendererInfoObj rendererInfo;
    GLint rendererCount;
    GLint i;
    GLuint displayMask = CGDisplayIDToOpenGLDisplayMask( kCGDirectMainDisplay );

    if( gWeightsLoaded )
        return;

    err = CGLQueryRendererInfo( displayMask, &rendererInfo, &rendererCount );
    if( err != kCGLNoError )
        fgError( "CGLQueryRendererInfo failed: %d", err );

    for( i = 0; i < rendererCount; ++i )
    {
        GLint value = 0;
        GLint colorBits = 0;
        GLint alphaBits = 0;

        CGLDescribeRenderer( rendererInfo, i, kCGLRPWindow, &value );
        if( !value )
            continue;

        CGLDescribeRenderer( rendererInfo, i, kCGLRPColorModes, &value );
        fghMaskToMaxColorWeight( value, &colorBits, &alphaBits );
        gMaxColor = fghMaxInt( gMaxColor, colorBits );
        gMaxAlpha = fghMaxInt( gMaxAlpha, alphaBits );

        CGLDescribeRenderer( rendererInfo, i, kCGLRPAccumModes, &value );
        fghMaskToMaxColorWeight( value, &colorBits, &alphaBits );
        gMaxAccumColor = fghMaxInt( gMaxAccumColor, colorBits );
        gMaxAccumAlpha = fghMaxInt( gMaxAccumAlpha, alphaBits );

        CGLDescribeRenderer( rendererInfo, i, kCGLRPDepthModes, &value );
        gMinDepth = fghMinInt( gMinDepth, fghMaskToMinWeight( value ) );
        gMaxDepth = fghMaxInt( gMaxDepth, fghMaskToMaxWeight( value ) );

        CGLDescribeRenderer( rendererInfo, i, kCGLRPStencilModes, &value );
        gMinStencil = fghMinInt( gMinStencil, fghMaskToMinWeight( value ) );
        gMaxStencil = fghMaxInt( gMaxStencil, fghMaskToMaxWeight( value ) );

        CGLDescribeRenderer( rendererInfo, i, kCGLRPMaxAuxBuffers, &value );
        gMaxAuxBuffers = fghMaxInt( gMaxAuxBuffers, value );

        CGLDescribeRenderer( rendererInfo, i, kCGLRPMaxSampleBuffers, &value );
        if( value > 0 )
        {
            CGLDescribeRenderer( rendererInfo, i, kCGLRPMaxSamples, &value );
            gMinSamples = fghMinInt( gMinSamples, value );
            gMaxSamples = fghMaxInt( gMaxSamples, value );
        }
    }

    CGLDestroyRendererInfo( rendererInfo );

    if( gMinDepth == INT_MAX )
        gMinDepth = 0;
    if( gMinStencil == INT_MAX )
        gMinStencil = 0;
    if( gMinSamples == INT_MAX )
        gMinSamples = 0;

    gWeightsLoaded = 1;
}

static int fghCocoaWeightForCriterion( const SFG_DisplayStringCriterion *criterion, int minWeight, int maxWeight )
{
    switch( criterion->comparison )
    {
    case FGDSTR_CMP_EQ:
        return criterion->value;
    case FGDSTR_CMP_NEQ:
        return maxWeight - criterion->value;
    case FGDSTR_CMP_LT:
    case FGDSTR_CMP_LTE:
        return minWeight;
    case FGDSTR_CMP_GT:
    case FGDSTR_CMP_GTE:
        return maxWeight;
    case FGDSTR_CMP_MIN:
        return criterion->value;
    default:
        return 0;
    }
}

static BOOL fghCocoaRequestsBestFormat( const SFG_DisplayStringCriterion *criterion )
{
    switch( criterion->comparison )
    {
    case FGDSTR_CMP_EQ:
    case FGDSTR_CMP_LTE:
        return criterion->value == 1;
    case FGDSTR_CMP_LT:
        return criterion->value == 2;
    default:
        return NO;
    }
}

static BOOL fghCocoaCriterionMatchesZero( const SFG_DisplayStringCriterion *criterion )
{
    switch( criterion->comparison )
    {
    case FGDSTR_CMP_EQ:
        return criterion->value == 0;
    case FGDSTR_CMP_NEQ:
        return criterion->value != 0;
    case FGDSTR_CMP_LTE:
        return 0 <= criterion->value;
    case FGDSTR_CMP_GTE:
        return 0 >= criterion->value;
    case FGDSTR_CMP_LT:
        return 0 < criterion->value;
    case FGDSTR_CMP_GT:
        return 0 > criterion->value;
    case FGDSTR_CMP_MIN:
        return criterion->value == 0;
    default:
        return YES;
    }
}

static void fghAppendProfileAttribute( NSOpenGLPixelFormatAttribute *attrs, int *index )
{
    attrs[(*index)++] = NSOpenGLPFAOpenGLProfile;
    if ( fgState.MajorVersion == 3 )
        attrs[(*index)++] = NSOpenGLProfileVersion3_2Core;
    else if ( fgState.MajorVersion == 4 )
        attrs[(*index)++] = NSOpenGLProfileVersion4_1Core;
    else
        attrs[(*index)++] = NSOpenGLProfileVersionLegacy;
}

static int fghGetPixelFormatValue( NSOpenGLPixelFormat *pixelFormat, NSOpenGLPixelFormatAttribute attr )
{
    GLint value = 0;
    [pixelFormat getValues:&value forAttribute:attr forVirtualScreen:0];
    return value;
}

static NSOpenGLPixelFormat *fghCreatePixelFormatFromDisplayMode( GLboolean gameMode,
                                                                 int *doubleBuffered, int *treatAsSingle )
{
    NSOpenGLPixelFormatAttribute attrs[32];
    int attrIndex = 0;
    NSOpenGLPixelFormat *pixelFormat;

    attrs[attrIndex++] = NSOpenGLPFAAccelerated;
    attrs[attrIndex++] = NSOpenGLPFAColorSize;
    attrs[attrIndex++] = 24;
    attrs[attrIndex++] = NSOpenGLPFAAlphaSize;
    attrs[attrIndex++] = 8;
    if ( fgState.DisplayMode & GLUT_DOUBLE )
        attrs[attrIndex++] = NSOpenGLPFADoubleBuffer;
    if ( fgState.DisplayMode & GLUT_DEPTH ) {
        attrs[attrIndex++] = NSOpenGLPFADepthSize;
        attrs[attrIndex++] = 24;
    }
    if ( fgState.DisplayMode & GLUT_STENCIL ) {
        attrs[attrIndex++] = NSOpenGLPFAStencilSize;
        attrs[attrIndex++] = 8;
    }
    if ( fgState.DisplayMode & GLUT_ACCUM ) {
        attrs[attrIndex++] = NSOpenGLPFAAccumSize;
        attrs[attrIndex++] = 32;
    }
    if ( fgState.DisplayMode & GLUT_AUX ) {
        attrs[attrIndex++] = NSOpenGLPFAAuxBuffers;
        attrs[attrIndex++] = fghNumberOfAuxBuffersRequested();
    }
    if ( fgState.DisplayMode & GLUT_MULTISAMPLE ) {
        attrs[attrIndex++] = NSOpenGLPFAMultisample;
        attrs[attrIndex++] = NSOpenGLPFASampleBuffers;
        attrs[attrIndex++] = 1;
        attrs[attrIndex++] = NSOpenGLPFASamples;
        attrs[attrIndex++] = fgState.SampleNumber;
    }
    if( gameMode ) {
        attrs[attrIndex++] = NSOpenGLPFAScreenMask;
        attrs[attrIndex++] = CGDisplayIDToOpenGLDisplayMask( kCGDirectMainDisplay );
    }
    fghAppendProfileAttribute( attrs, &attrIndex );
    attrs[attrIndex++] = 0;

    pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if( pixelFormat ) {
        if( doubleBuffered )
            *doubleBuffered = fghGetPixelFormatValue( pixelFormat, NSOpenGLPFADoubleBuffer );
        if( treatAsSingle )
            *treatAsSingle = ( doubleBuffered && *doubleBuffered && !( fgState.DisplayMode & GLUT_DOUBLE ) ) ? 1 : 0;
    }
    return pixelFormat;
}

static NSOpenGLPixelFormat *fghCreatePixelFormatFromCriteria( const SFG_DisplayStringCriteria *criteria,
                                                              GLboolean gameMode )
{
    NSOpenGLPixelFormatAttribute attrs[64];
    int attrIndex = 0;
    int redWeight = 0, greenWeight = 0, blueWeight = 0, alphaWeight = 0;
    int accumRedWeight = 0, accumGreenWeight = 0, accumBlueWeight = 0, accumAlphaWeight = 0;
    int bufferWeight = 0;
    int i;

    fghLoadRendererWeights();

    for( i = 0; i < criteria->ncriteria; ++i )
    {
        const SFG_DisplayStringCriterion *criterion = &criteria->criteria[i];
        switch( criterion->capability )
        {
        case FGDSTR_CAP_DOUBLEBUFFER:
            if( fghCocoaWeightForCriterion( criterion, 0, 1 ) > 0 )
                attrs[attrIndex++] = NSOpenGLPFADoubleBuffer;
            break;
        case FGDSTR_CAP_STEREO:
            if( fghCocoaWeightForCriterion( criterion, 0, 1 ) > 0 )
                attrs[attrIndex++] = NSOpenGLPFAStereo;
            break;
        case FGDSTR_CAP_AUX_BUFFERS:
            attrs[attrIndex++] = NSOpenGLPFAAuxBuffers;
            attrs[attrIndex++] = fghCocoaWeightForCriterion( criterion, 0, gMaxAuxBuffers );
            break;
        case FGDSTR_CAP_RED_SIZE:
            redWeight = fghCocoaWeightForCriterion( criterion, 0, gMaxColor );
            break;
        case FGDSTR_CAP_GREEN_SIZE:
            greenWeight = fghCocoaWeightForCriterion( criterion, 0, gMaxColor );
            break;
        case FGDSTR_CAP_BLUE_SIZE:
            blueWeight = fghCocoaWeightForCriterion( criterion, 0, gMaxColor );
            break;
        case FGDSTR_CAP_ALPHA_SIZE:
            alphaWeight = fghCocoaWeightForCriterion( criterion, 0, gMaxAlpha );
            break;
        case FGDSTR_CAP_BUFFER_SIZE:
            bufferWeight = fghCocoaWeightForCriterion( criterion, 16, 32 );
            break;
        case FGDSTR_CAP_DEPTH_SIZE:
            attrs[attrIndex++] = NSOpenGLPFADepthSize;
            attrs[attrIndex++] = fghCocoaWeightForCriterion( criterion, gMinDepth, gMaxDepth );
            break;
        case FGDSTR_CAP_STENCIL_SIZE:
            attrs[attrIndex++] = NSOpenGLPFAStencilSize;
            attrs[attrIndex++] = fghCocoaWeightForCriterion( criterion, gMinStencil, gMaxStencil );
            break;
        case FGDSTR_CAP_ACCUM_RED_SIZE:
            accumRedWeight = fghCocoaWeightForCriterion( criterion, 0, gMaxAccumColor );
            break;
        case FGDSTR_CAP_ACCUM_GREEN_SIZE:
            accumGreenWeight = fghCocoaWeightForCriterion( criterion, 0, gMaxAccumColor );
            break;
        case FGDSTR_CAP_ACCUM_BLUE_SIZE:
            accumBlueWeight = fghCocoaWeightForCriterion( criterion, 0, gMaxAccumColor );
            break;
        case FGDSTR_CAP_ACCUM_ALPHA_SIZE:
            accumAlphaWeight = fghCocoaWeightForCriterion( criterion, 0, gMaxAccumAlpha );
            break;
        case FGDSTR_CAP_SAMPLES:
            if( fghCocoaWeightForCriterion( criterion, gMinSamples, gMaxSamples ) > 0 ) {
                attrs[attrIndex++] = NSOpenGLPFAMultisample;
                attrs[attrIndex++] = NSOpenGLPFASampleBuffers;
                attrs[attrIndex++] = 1;
                attrs[attrIndex++] = NSOpenGLPFASamples;
                attrs[attrIndex++] = fghCocoaWeightForCriterion( criterion, gMinSamples, gMaxSamples );
            }
            break;
        case FGDSTR_CAP_SLOW:
            if( fghCocoaWeightForCriterion( criterion, 0, 1 ) == 0 )
                attrs[attrIndex++] = NSOpenGLPFAAccelerated;
            break;
        case FGDSTR_CAP_CONFORMANT:
            if( fghCocoaWeightForCriterion( criterion, 0, 1 ) > 0 )
                attrs[attrIndex++] = NSOpenGLPFACompliant;
            break;
        case FGDSTR_CAP_NUM:
            if( !fghCocoaRequestsBestFormat( criterion ) )
                return nil;
            break;
        case FGDSTR_CAP_LEVEL:
            if( !fghCocoaCriterionMatchesZero( criterion ) )
                return nil;
            break;
        case FGDSTR_CAP_CI_MODE:
        case FGDSTR_CAP_LUMINANCE_MODE:
        case FGDSTR_CAP_XVISUAL:
        case FGDSTR_CAP_XSTATICGRAY:
        case FGDSTR_CAP_XGRAYSCALE:
        case FGDSTR_CAP_XSTATICCOLOR:
        case FGDSTR_CAP_XPSEUDOCOLOR:
        case FGDSTR_CAP_XTRUECOLOR:
        case FGDSTR_CAP_XDIRECTCOLOR:
            return nil;
        default:
            break;
        }
    }

    if( bufferWeight || redWeight || greenWeight || blueWeight || alphaWeight ) {
        int colorSize = bufferWeight ? bufferWeight : ( ( redWeight > 5 || greenWeight > 5 || blueWeight > 5 || alphaWeight > 1 ) ? 32 : 16 );
        attrs[attrIndex++] = NSOpenGLPFAColorSize;
        attrs[attrIndex++] = colorSize;
        attrs[attrIndex++] = NSOpenGLPFAClosestPolicy;
        if( alphaWeight > 0 ) {
            attrs[attrIndex++] = NSOpenGLPFAAlphaSize;
            attrs[attrIndex++] = alphaWeight;
        }
    }

    if( accumRedWeight || accumGreenWeight || accumBlueWeight || accumAlphaWeight ) {
        attrs[attrIndex++] = NSOpenGLPFAAccumSize;
        attrs[attrIndex++] = ( accumRedWeight > 8 || accumGreenWeight > 8 || accumBlueWeight > 8 || accumAlphaWeight > 8 ) ? 64 : 32;
    }

    if( gameMode ) {
        int colorDepth = fgState.GameModeDepth > 0 ? fgState.GameModeDepth : (int)NSBitsPerPixelFromDepth( [[NSScreen mainScreen] depth] );
        attrs[attrIndex++] = NSOpenGLPFAScreenMask;
        attrs[attrIndex++] = CGDisplayIDToOpenGLDisplayMask( kCGDirectMainDisplay );
        attrs[attrIndex++] = NSOpenGLPFAColorSize;
        attrs[attrIndex++] = colorDepth;
        attrs[attrIndex++] = NSOpenGLPFAClosestPolicy;
    }

    fghAppendProfileAttribute( attrs, &attrIndex );
    attrs[attrIndex++] = 0;

    return [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
}

static NSOpenGLPixelFormat *fghCreatePixelFormatFromDisplayString( GLboolean gameMode,
                                                                   int *doubleBuffered, int *treatAsSingle )
{
    NSOpenGLPixelFormat *pixelFormat;
    SFG_DisplayStringParserOptions options;
    SFG_DisplayStringCriteria criteria;
    GLboolean usedRetry = GL_FALSE;

    memset( &options, 0, sizeof( options ) );
    options.target = FGDSTR_TARGET_COCOA;
    options.supportsMultisample = GL_TRUE;
    options.supportsSlow = GL_TRUE;
    options.supportsConformant = GL_TRUE;

    criteria = fghDisplayStringParseModeString( fgState.DisplayString, &options );
    if( !criteria.criteria )
        return nil;

    pixelFormat = fghCreatePixelFormatFromCriteria( &criteria, gameMode );
    if( !pixelFormat && criteria.allowDoubleAsSingle ) {
        fghDisplayStringRewriteSingleAsDouble( &criteria );
        pixelFormat = fghCreatePixelFormatFromCriteria( &criteria, gameMode );
        usedRetry = ( pixelFormat != nil );
    }

    if( pixelFormat ) {
        if( doubleBuffered )
            *doubleBuffered = fghGetPixelFormatValue( pixelFormat, NSOpenGLPFADoubleBuffer );
        if( treatAsSingle )
            *treatAsSingle = usedRetry ? 1 : 0;
    }

    fghDisplayStringFreeCriteria( &criteria );
    return pixelFormat;
}

void *fghCreatePixelFormatCocoa( GLboolean gameMode, GLboolean isMenu,
                                 int *doubleBuffered, int *treatAsSingle )
{
    if( doubleBuffered )
        *doubleBuffered = 0;
    if( treatAsSingle )
        *treatAsSingle = 0;

    if( !isMenu && fghDisplayStringIsActive() )
        return fghCreatePixelFormatFromDisplayString( gameMode, doubleBuffered, treatAsSingle );
    return fghCreatePixelFormatFromDisplayMode( gameMode, doubleBuffered, treatAsSingle );
}
