/*
 * X11/GLX display-string selector.
 */

#define FREEGLUT_BUILDING_LIB
#include <GL/freeglut.h>
#include "../fg_internal.h"
#include "../fg_dstr.h"
#include "fg_window_x11_glx.h"

#ifndef GLX_VISUAL_CAVEAT_EXT
#define GLX_VISUAL_CAVEAT_EXT 0x20
#endif
#ifndef GLX_NON_CONFORMANT_VISUAL_EXT
#define GLX_NON_CONFORMANT_VISUAL_EXT 0x800D
#endif
#ifndef GLX_SLOW_VISUAL_EXT
#define GLX_SLOW_VISUAL_EXT 0x8001
#endif
#ifndef GLX_NONE_EXT
#define GLX_NONE_EXT 0x8000
#endif
#ifndef GLX_TRANSPARENT_TYPE_EXT
#define GLX_TRANSPARENT_TYPE_EXT 0x23
#endif
#ifndef GLX_TRANSPARENT_INDEX_EXT
#define GLX_TRANSPARENT_INDEX_EXT 0x8009
#endif
#ifndef GLX_SAMPLE_BUFFERS_ARB
#define GLX_SAMPLE_BUFFERS_ARB GLX_SAMPLE_BUFFERS
#endif
#ifndef GLX_SAMPLES_ARB
#define GLX_SAMPLES_ARB GLX_SAMPLES
#endif

static const SFG_DisplayStringCriterion fghWindowCriteria[] =
{
    { FGDSTR_CAP_LEVEL,       FGDSTR_CMP_EQ, 0 },
    { FGDSTR_CAP_TRANSPARENT, FGDSTR_CMP_EQ, 0 }
};

static const int fghWindowCriteriaMask =
    ( 1 << FGDSTR_CAP_LEVEL ) | ( 1 << FGDSTR_CAP_TRANSPARENT );

static GLboolean fghDetermineMesaGLX( void )
{
#ifdef GLX_VERSION_1_1
    const char *vendor = glXGetClientString( fgDisplay.pDisplay.Display, GLX_VENDOR );
    const char *version, *ch;

    if ( !vendor || strcmp( vendor, "Brian Paul" ) != 0 ) {
        return GL_FALSE;
    }

    version = glXGetClientString( fgDisplay.pDisplay.Display, GLX_VERSION );
    if ( !version ) {
        return GL_FALSE;
    }

    for ( ch = version; *ch != ' ' && *ch != '\0'; ch++ ) {
    }
    while ( *ch == ' ' ) {
        ch++;
    }

    return strncmp( ch, "Mesa ", 5 ) == 0;
#else
    return GL_FALSE;
#endif
}

static void fghFillVisualClassCaps( XVisualInfo *visualInfo, int *caps )
{
    caps[FGDSTR_CAP_XSTATICGRAY]  = 0;
    caps[FGDSTR_CAP_XGRAYSCALE]   = 0;
    caps[FGDSTR_CAP_XSTATICCOLOR] = 0;
    caps[FGDSTR_CAP_XPSEUDOCOLOR] = 0;
    caps[FGDSTR_CAP_XTRUECOLOR]   = 0;
    caps[FGDSTR_CAP_XDIRECTCOLOR] = 0;

    if ( !visualInfo ) {
        return;
    }

#if defined(__cplusplus) || defined(c_plusplus)
    switch ( visualInfo->c_class ) {
#else
    switch ( visualInfo->class ) {
#endif
    case StaticGray:
        caps[FGDSTR_CAP_XSTATICGRAY] = 1;
        break;
    case GrayScale:
        caps[FGDSTR_CAP_XGRAYSCALE] = 1;
        break;
    case StaticColor:
        caps[FGDSTR_CAP_XSTATICCOLOR] = 1;
        break;
    case PseudoColor:
        caps[FGDSTR_CAP_XPSEUDOCOLOR] = 1;
        break;
    case TrueColor:
        caps[FGDSTR_CAP_XTRUECOLOR] = 1;
        break;
    case DirectColor:
        caps[FGDSTR_CAP_XDIRECTCOLOR] = 1;
        break;
    }
}

static void fghFillRatingCaps(
    int rating,
    GLboolean visualRating,
    int *caps
)
{
    if ( !visualRating ) {
        caps[FGDSTR_CAP_SLOW] = 0;
        caps[FGDSTR_CAP_CONFORMANT] = 1;
        return;
    }

    switch ( rating ) {
    case GLX_SLOW_VISUAL_EXT:
        caps[FGDSTR_CAP_SLOW] = 1;
        caps[FGDSTR_CAP_CONFORMANT] = 1;
        break;
    case GLX_NON_CONFORMANT_VISUAL_EXT:
        caps[FGDSTR_CAP_SLOW] = 0;
        caps[FGDSTR_CAP_CONFORMANT] = 0;
        break;
    case GLX_NONE_EXT:
    default:
        caps[FGDSTR_CAP_SLOW] = 0;
        caps[FGDSTR_CAP_CONFORMANT] = 1;
        break;
    }
}

#ifdef USE_FBCONFIG
static void fghFillConfigCaps(
    GLXFBConfig config,
    SFG_DisplayStringMode *mode,
    GLboolean visualInfo,
    GLboolean visualRating,
    GLboolean wantSRGB
)
{
    Display *dpy = fgDisplay.pDisplay.Display;
    int drawType, renderType, value;
    XVisualInfo *visualInfoStruct;

    memset( mode->cap, 0, sizeof( mode->cap ) );
    mode->valid = 1;

    if ( glXGetFBConfigAttrib( dpy, config, GLX_DRAWABLE_TYPE, &drawType ) != Success ||
         !( drawType & GLX_WINDOW_BIT ) ) {
        mode->valid = 0;
        return;
    }

    if ( glXGetFBConfigAttrib( dpy, config, GLX_RENDER_TYPE, &renderType ) != Success ) {
        mode->valid = 0;
        return;
    }

    mode->cap[FGDSTR_CAP_RGBA] = !!( renderType & GLX_RGBA_BIT );

    if ( glXGetFBConfigAttrib( dpy, config, GLX_BUFFER_SIZE, &mode->cap[FGDSTR_CAP_BUFFER_SIZE] ) != Success ||
         glXGetFBConfigAttrib( dpy, config, GLX_DOUBLEBUFFER, &mode->cap[FGDSTR_CAP_DOUBLEBUFFER] ) != Success ||
         glXGetFBConfigAttrib( dpy, config, GLX_STEREO, &mode->cap[FGDSTR_CAP_STEREO] ) != Success ||
         glXGetFBConfigAttrib( dpy, config, GLX_AUX_BUFFERS, &mode->cap[FGDSTR_CAP_AUX_BUFFERS] ) != Success ||
         glXGetFBConfigAttrib( dpy, config, GLX_RED_SIZE, &mode->cap[FGDSTR_CAP_RED_SIZE] ) != Success ||
         glXGetFBConfigAttrib( dpy, config, GLX_GREEN_SIZE, &mode->cap[FGDSTR_CAP_GREEN_SIZE] ) != Success ||
         glXGetFBConfigAttrib( dpy, config, GLX_BLUE_SIZE, &mode->cap[FGDSTR_CAP_BLUE_SIZE] ) != Success ||
         glXGetFBConfigAttrib( dpy, config, GLX_ALPHA_SIZE, &mode->cap[FGDSTR_CAP_ALPHA_SIZE] ) != Success ||
         glXGetFBConfigAttrib( dpy, config, GLX_DEPTH_SIZE, &mode->cap[FGDSTR_CAP_DEPTH_SIZE] ) != Success ||
         glXGetFBConfigAttrib( dpy, config, GLX_STENCIL_SIZE, &mode->cap[FGDSTR_CAP_STENCIL_SIZE] ) != Success ||
         glXGetFBConfigAttrib( dpy, config, GLX_ACCUM_RED_SIZE, &mode->cap[FGDSTR_CAP_ACCUM_RED_SIZE] ) != Success ||
         glXGetFBConfigAttrib( dpy, config, GLX_ACCUM_GREEN_SIZE, &mode->cap[FGDSTR_CAP_ACCUM_GREEN_SIZE] ) != Success ||
         glXGetFBConfigAttrib( dpy, config, GLX_ACCUM_BLUE_SIZE, &mode->cap[FGDSTR_CAP_ACCUM_BLUE_SIZE] ) != Success ||
         glXGetFBConfigAttrib( dpy, config, GLX_ACCUM_ALPHA_SIZE, &mode->cap[FGDSTR_CAP_ACCUM_ALPHA_SIZE] ) != Success ||
         glXGetFBConfigAttrib( dpy, config, GLX_LEVEL, &mode->cap[FGDSTR_CAP_LEVEL] ) != Success ) {
        mode->valid = 0;
        return;
    }

    if ( glXGetFBConfigAttrib( dpy, config, GLX_VISUAL_ID, &mode->cap[FGDSTR_CAP_XVISUAL] ) != Success ) {
        mode->cap[FGDSTR_CAP_XVISUAL] = 0;
    }

    visualInfoStruct = glXGetVisualFromFBConfig( dpy, config );
    fghFillVisualClassCaps( visualInfoStruct, mode->cap );
    if ( visualInfoStruct ) {
        XFree( visualInfoStruct );
    }

    if ( visualRating &&
         glXGetFBConfigAttrib( dpy, config, GLX_VISUAL_CAVEAT_EXT, &value ) == Success ) {
        fghFillRatingCaps( value, visualRating, mode->cap );
    } else {
        fghFillRatingCaps( GLX_NONE_EXT, GL_FALSE, mode->cap );
    }

    if ( visualInfo &&
         glXGetFBConfigAttrib( dpy, config, GLX_TRANSPARENT_TYPE_EXT, &value ) == Success ) {
        mode->cap[FGDSTR_CAP_TRANSPARENT] = ( value != GLX_NONE_EXT );
    } else {
        mode->cap[FGDSTR_CAP_TRANSPARENT] = 0;
    }

    if ( glXGetFBConfigAttrib( dpy, config, GLX_SAMPLES_ARB, &mode->cap[FGDSTR_CAP_SAMPLES] ) != Success ) {
        mode->cap[FGDSTR_CAP_SAMPLES] = 0;
    }

    if ( wantSRGB ) {
        if ( glXGetFBConfigAttrib( dpy, config, GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB, &value ) != Success ||
             !value ) {
            mode->valid = 0;
        }
    }
}
#else
static void fghFillVisualCaps(
    XVisualInfo *visualInfo,
    SFG_DisplayStringMode *mode,
    GLboolean visualInfoExt,
    GLboolean visualRating,
    GLboolean wantSRGB
)
{
    Display *dpy = fgDisplay.pDisplay.Display;
    int rating;

    memset( mode->cap, 0, sizeof( mode->cap ) );
    mode->valid = 1;

    if ( glXGetConfig( dpy, visualInfo, GLX_RGBA, &mode->cap[FGDSTR_CAP_RGBA] ) != 0 ||
         glXGetConfig( dpy, visualInfo, GLX_BUFFER_SIZE, &mode->cap[FGDSTR_CAP_BUFFER_SIZE] ) != 0 ||
         glXGetConfig( dpy, visualInfo, GLX_DOUBLEBUFFER, &mode->cap[FGDSTR_CAP_DOUBLEBUFFER] ) != 0 ||
         glXGetConfig( dpy, visualInfo, GLX_STEREO, &mode->cap[FGDSTR_CAP_STEREO] ) != 0 ||
         glXGetConfig( dpy, visualInfo, GLX_AUX_BUFFERS, &mode->cap[FGDSTR_CAP_AUX_BUFFERS] ) != 0 ||
         glXGetConfig( dpy, visualInfo, GLX_RED_SIZE, &mode->cap[FGDSTR_CAP_RED_SIZE] ) != 0 ||
         glXGetConfig( dpy, visualInfo, GLX_GREEN_SIZE, &mode->cap[FGDSTR_CAP_GREEN_SIZE] ) != 0 ||
         glXGetConfig( dpy, visualInfo, GLX_BLUE_SIZE, &mode->cap[FGDSTR_CAP_BLUE_SIZE] ) != 0 ||
         glXGetConfig( dpy, visualInfo, GLX_ALPHA_SIZE, &mode->cap[FGDSTR_CAP_ALPHA_SIZE] ) != 0 ||
         glXGetConfig( dpy, visualInfo, GLX_DEPTH_SIZE, &mode->cap[FGDSTR_CAP_DEPTH_SIZE] ) != 0 ||
         glXGetConfig( dpy, visualInfo, GLX_STENCIL_SIZE, &mode->cap[FGDSTR_CAP_STENCIL_SIZE] ) != 0 ||
         glXGetConfig( dpy, visualInfo, GLX_ACCUM_RED_SIZE, &mode->cap[FGDSTR_CAP_ACCUM_RED_SIZE] ) != 0 ||
         glXGetConfig( dpy, visualInfo, GLX_ACCUM_GREEN_SIZE, &mode->cap[FGDSTR_CAP_ACCUM_GREEN_SIZE] ) != 0 ||
         glXGetConfig( dpy, visualInfo, GLX_ACCUM_BLUE_SIZE, &mode->cap[FGDSTR_CAP_ACCUM_BLUE_SIZE] ) != 0 ||
         glXGetConfig( dpy, visualInfo, GLX_ACCUM_ALPHA_SIZE, &mode->cap[FGDSTR_CAP_ACCUM_ALPHA_SIZE] ) != 0 ||
         glXGetConfig( dpy, visualInfo, GLX_LEVEL, &mode->cap[FGDSTR_CAP_LEVEL] ) != 0 ) {
        mode->valid = 0;
        return;
    }

    mode->cap[FGDSTR_CAP_XVISUAL] = (int)visualInfo->visualid;
    fghFillVisualClassCaps( visualInfo, mode->cap );

    if ( visualRating &&
         glXGetConfig( dpy, visualInfo, GLX_VISUAL_CAVEAT_EXT, &rating ) == 0 ) {
        fghFillRatingCaps( rating, visualRating, mode->cap );
    } else {
        fghFillRatingCaps( GLX_NONE_EXT, GL_FALSE, mode->cap );
    }

    if ( visualInfoExt &&
         glXGetConfig( dpy, visualInfo, GLX_TRANSPARENT_TYPE_EXT, &rating ) == 0 ) {
        mode->cap[FGDSTR_CAP_TRANSPARENT] = ( rating != GLX_NONE_EXT );
    } else {
        mode->cap[FGDSTR_CAP_TRANSPARENT] = 0;
    }

    if ( glXGetConfig( dpy, visualInfo, GLX_SAMPLES_ARB, &mode->cap[FGDSTR_CAP_SAMPLES] ) != 0 ) {
        mode->cap[FGDSTR_CAP_SAMPLES] = 0;
    }

    if ( wantSRGB &&
         ( glXGetConfig( dpy, visualInfo, GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB, &rating ) != 0 ||
           !rating ) ) {
        mode->valid = 0;
    }
}
#endif

#ifdef USE_FBCONFIG
int fghChooseConfigDisplayString(
    GLXFBConfig *fbconfig,
    int *doubleBuffered,
    int *treatAsSingle
)
#else
int fghChooseConfigDisplayString(
    XVisualInfo **vinf_ret,
    int *doubleBuffered,
    int *treatAsSingle
)
#endif
{
    Display *dpy = fgDisplay.pDisplay.Display;
    int scr = fgDisplay.pDisplay.Screen;
    GLboolean visualInfo = fgPlatformExtSupported( "GLX_EXT_visual_info" );
    GLboolean visualRating = fgPlatformExtSupported( "GLX_EXT_visual_rating" );
    GLboolean multisample = fgPlatformExtSupported( "GLX_SGIS_multisample" ) ||
                            fgPlatformExtSupported( "GLX_ARB_multisample" );
    GLboolean wantSRGB = ( fghDisplayStringWindowModeMask() & GLUT_SRGB ) != 0;
    SFG_DisplayStringParserOptions options;
    SFG_DisplayStringCriteria parsed;
    SFG_DisplayStringSelection selection;
    SFG_DisplayStringMode *modes;
    int i, nitems = 0;
#ifdef USE_FBCONFIG
    GLXFBConfig *configs = NULL;
#else
    XVisualInfo visualTemplate;
    XVisualInfo *visuals = NULL;
#endif

    options.target = FGDSTR_TARGET_X11;
    options.supportsMultisample = multisample;
    options.supportsSlow = visualRating;
    options.supportsConformant = visualRating;
    options.supportsTransparent = visualInfo;
    options.isMesa = fghDetermineMesaGLX();
    options.requiredCriteria = fghWindowCriteria;
    options.nRequired = sizeof( fghWindowCriteria ) / sizeof( fghWindowCriteria[0] );
    options.requiredMask = fghWindowCriteriaMask;

    parsed = fghDisplayStringParseModeString( fgState.DisplayString, &options );
    if ( !parsed.criteria ) {
        return 0;
    }

#ifdef USE_FBCONFIG
    configs = glXGetFBConfigs( dpy, scr, &nitems );
    if ( !configs || nitems <= 0 ) {
        fghDisplayStringFreeCriteria( &parsed );
        if ( configs ) {
            XFree( configs );
        }
        return 0;
    }
#else
    visualTemplate.screen = scr;
    visuals = XGetVisualInfo( dpy, VisualScreenMask, &visualTemplate, &nitems );
    if ( !visuals || nitems <= 0 ) {
        fghDisplayStringFreeCriteria( &parsed );
        if ( visuals ) {
            XFree( visuals );
        }
        return 0;
    }
#endif

    modes = (SFG_DisplayStringMode *)calloc( nitems, sizeof( SFG_DisplayStringMode ) );
    if ( !modes ) {
        fghDisplayStringFreeCriteria( &parsed );
#ifdef USE_FBCONFIG
        XFree( configs );
#else
        XFree( visuals );
#endif
        fgError( "out of memory" );
    }

    for ( i = 0; i < nitems; i++ ) {
#ifdef USE_FBCONFIG
        modes[i].data = &configs[i];
        fghFillConfigCaps( configs[i], &modes[i], visualInfo, visualRating, wantSRGB );
#else
        int glCapable = 0;
        modes[i].data = NULL;
        if ( glXGetConfig( dpy, &visuals[i], GLX_USE_GL, &glCapable ) == 0 && glCapable ) {
            fghFillVisualCaps( &visuals[i], &modes[i], visualInfo, visualRating, wantSRGB );
        }
#endif
    }

    selection = fghDisplayStringChooseMode( modes, nitems, &parsed, GL_TRUE );
    fghDisplayStringFreeCriteria( &parsed );

    if ( selection.mode ) {
        if ( doubleBuffered ) {
            *doubleBuffered = selection.mode->cap[FGDSTR_CAP_DOUBLEBUFFER];
        }
        if ( treatAsSingle ) {
            *treatAsSingle = selection.usedDoubleRetry ? 1 : 0;
        }
#ifdef USE_FBCONFIG
        *fbconfig = *(GLXFBConfig *)selection.mode->data;
#else
        visualTemplate.visualid = selection.mode->cap[FGDSTR_CAP_XVISUAL];
        *vinf_ret = XGetVisualInfo( dpy, VisualIDMask, &visualTemplate, &nitems );
#endif
    }

    free( modes );
#ifdef USE_FBCONFIG
    XFree( configs );
#else
    XFree( visuals );
#endif

#ifdef USE_FBCONFIG
    return selection.mode != NULL;
#else
    return selection.mode != NULL && *vinf_ret != NULL;
#endif
}
