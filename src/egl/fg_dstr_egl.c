#define FREEGLUT_BUILDING_LIB
#include <GL/freeglut.h>
#include "../fg_internal.h"
#include "../fg_dstr.h"
#include "egl/fg_window_egl.h"

#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x00000040
#endif

static EGLint fghEGLRenderableType( void )
{
#ifdef EGL_OPENGL_ES3_BIT
    if( fgDisplay.pDisplay.egl.MinorVersion >= 5 && fgState.MajorVersion >= 3 )
        return EGL_OPENGL_ES3_BIT;
#endif
    if( fgState.MajorVersion >= 3 )
        return EGL_OPENGL_ES3_BIT_KHR;
    if( fgState.MajorVersion >= 2 )
        return EGL_OPENGL_ES2_BIT;
    return EGL_OPENGL_ES_BIT;
}

static int fghGetConfigAttribOrDefault( EGLConfig config, EGLint attr, int defaultValue )
{
    EGLint value = defaultValue;
    if( !eglGetConfigAttrib( fgDisplay.pDisplay.egl.Display, config, attr, &value ) )
        return defaultValue;
    return value;
}

static void fghFillConfigCaps( EGLConfig config, SFG_DisplayStringMode *mode )
{
    int caveat;
    int renderableType;
    int surfaceType;
    int transparentType;
    int samples;
    int sampleBuffers;

    memset( mode->cap, 0, sizeof( mode->cap ) );
    mode->valid = 0;

    surfaceType = fghGetConfigAttribOrDefault( config, EGL_SURFACE_TYPE, 0 );
    renderableType = fghGetConfigAttribOrDefault( config, EGL_RENDERABLE_TYPE, 0 );
    if( !( surfaceType & EGL_WINDOW_BIT ) || !( renderableType & fghEGLRenderableType() ) )
        return;

    mode->valid = 1;
    mode->cap[ FGDSTR_CAP_NUM ] = 1;
    mode->cap[ FGDSTR_CAP_BUFFER_SIZE ] = fghGetConfigAttribOrDefault( config, EGL_BUFFER_SIZE, 0 );
    mode->cap[ FGDSTR_CAP_DOUBLEBUFFER ] = 1;
    mode->cap[ FGDSTR_CAP_RED_SIZE ] = fghGetConfigAttribOrDefault( config, EGL_RED_SIZE, 0 );
    mode->cap[ FGDSTR_CAP_GREEN_SIZE ] = fghGetConfigAttribOrDefault( config, EGL_GREEN_SIZE, 0 );
    mode->cap[ FGDSTR_CAP_BLUE_SIZE ] = fghGetConfigAttribOrDefault( config, EGL_BLUE_SIZE, 0 );
    mode->cap[ FGDSTR_CAP_ALPHA_SIZE ] = fghGetConfigAttribOrDefault( config, EGL_ALPHA_SIZE, 0 );
    mode->cap[ FGDSTR_CAP_DEPTH_SIZE ] = fghGetConfigAttribOrDefault( config, EGL_DEPTH_SIZE, 0 );
    mode->cap[ FGDSTR_CAP_STENCIL_SIZE ] = fghGetConfigAttribOrDefault( config, EGL_STENCIL_SIZE, 0 );
    mode->cap[ FGDSTR_CAP_LEVEL ] = fghGetConfigAttribOrDefault( config, EGL_LEVEL, 0 );
    mode->cap[ FGDSTR_CAP_RGBA_MODE ] = 1;
    mode->cap[ FGDSTR_CAP_CI_MODE ] = 0;
    mode->cap[ FGDSTR_CAP_LUMINANCE_MODE ] = 0;
    mode->cap[ FGDSTR_CAP_AUX_BUFFERS ] = 0;
    mode->cap[ FGDSTR_CAP_STEREO ] = 0;

    samples = fghGetConfigAttribOrDefault( config, EGL_SAMPLES, 0 );
    sampleBuffers = fghGetConfigAttribOrDefault( config, EGL_SAMPLE_BUFFERS, 0 );
    mode->cap[ FGDSTR_CAP_SAMPLES ] = sampleBuffers ? samples : 0;

    transparentType = fghGetConfigAttribOrDefault( config, EGL_TRANSPARENT_TYPE, EGL_NONE );
    mode->cap[ FGDSTR_CAP_TRANSPARENT ] = ( transparentType == EGL_TRANSPARENT_RGB );

    caveat = fghGetConfigAttribOrDefault( config, EGL_CONFIG_CAVEAT, EGL_NONE );
    mode->cap[ FGDSTR_CAP_SLOW ] = ( caveat == EGL_SLOW_CONFIG );
    mode->cap[ FGDSTR_CAP_CONFORMANT ] = ( caveat != EGL_NON_CONFORMANT_CONFIG );
}

int fghChooseConfigDisplayStringEGL( EGLConfig *config, int *doubleBuffered, int *treatAsSingle )
{
    EGLConfig *configs;
    EGLint configCount = 0;
    EGLint i;
    int result = 0;
    SFG_DisplayStringMode *modes = NULL;
    SFG_DisplayStringCriteria criteria;
    SFG_DisplayStringParserOptions options;
    SFG_DisplayStringSelection selection;

    if( !fghDisplayStringIsActive() )
        return 0;

    memset( &criteria, 0, sizeof( criteria ) );
    memset( &options, 0, sizeof( options ) );
    memset( &selection, 0, sizeof( selection ) );

    options.target = FGDSTR_TARGET_EGL;
    criteria = fghDisplayStringParseModeString( fgState.DisplayString, &options );
    if( !criteria.criteria )
        return 0;

    if( !eglGetConfigs( fgDisplay.pDisplay.egl.Display, NULL, 0, &configCount ) || configCount <= 0 )
        goto done;

    configs = malloc( sizeof( EGLConfig ) * configCount );
    if( !configs )
        goto done;

    if( !eglGetConfigs( fgDisplay.pDisplay.egl.Display, configs, configCount, &configCount ) )
        goto done_with_configs;

    modes = calloc( configCount, sizeof( *modes ) );
    if( !modes )
        goto done_with_configs;

    for( i = 0; i < configCount; ++i )
    {
        modes[ i ].data = &configs[ i ];
        fghFillConfigCaps( configs[ i ], &modes[ i ] );
        if( modes[ i ].valid )
            modes[ i ].cap[ FGDSTR_CAP_NUM ] = i + 1;
    }

    selection = fghDisplayStringChooseMode( modes, configCount, &criteria, GL_FALSE );
    if( !selection.mode )
        goto done_with_modes;

    *config = *(const EGLConfig*)selection.mode->data;
    if( doubleBuffered )
        *doubleBuffered = 1;
    if( treatAsSingle )
        *treatAsSingle = 0;
    result = 1;

done_with_modes:
    free( modes );
done_with_configs:
    free( configs );
done:
    fghDisplayStringFreeCriteria( &criteria );
    return result;
}
