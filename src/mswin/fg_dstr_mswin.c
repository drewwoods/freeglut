#define FREEGLUT_BUILDING_LIB
#include <GL/freeglut.h>
#include "../fg_internal.h"
#include "../fg_dstr.h"

#if !defined(_WIN32_WCE)

typedef const char * (WINAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC) (HDC hdc);
typedef BOOL (WINAPI * PFNWGLGETPIXELFORMATATTRIBIVARBPROC) (HDC hdc, int iPixelFormat, int iLayerPlane,
                                                             UINT nAttributes, const int *piAttributes, int *piValues);

#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_ACCELERATION_ARB              0x2003
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_SAMPLE_BUFFERS_ARB            0x2041
#define WGL_SAMPLES_ARB                   0x2042
#define WGL_FULL_ACCELERATION_ARB         0x2027
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB  0x20A9

typedef struct tagSFG_WGLPixelFormatQuery SFG_WGLPixelFormatQuery;
struct tagSFG_WGLPixelFormatQuery
{
    HWND hWnd;
    HDC hDC;
    HGLRC hRC;
    PFNWGLGETPIXELFORMATATTRIBIVARBPROC getPixelFormatAttribivARB;
    int hasMultisample;
    int hasSRGB;
};

static int fghWGLHasExtension( HDC hdc, const char *extension )
{
    const char *extString;
    PFNWGLGETEXTENSIONSSTRINGARBPROC getExtensionsStringARB =
        (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress( "wglGetExtensionsStringARB" );

    if( !getExtensionsStringARB )
        return 0;

    extString = getExtensionsStringARB( hdc );
    return extString && strstr( extString, extension );
}

static void fghDestroyWGLPixelFormatQuery( SFG_WGLPixelFormatQuery *query )
{
    if( query->hRC )
    {
        wglMakeCurrent( NULL, NULL );
        wglDeleteContext( query->hRC );
    }
    if( query->hDC && query->hWnd )
        ReleaseDC( query->hWnd, query->hDC );
    if( query->hWnd )
        DestroyWindow( query->hWnd );
    if( query->hWnd )
        UnregisterClass( _T("FREEGLUT_dstr_dummy"), fgDisplay.pDisplay.Instance );
}

static int fghInitWGLPixelFormatQuery( SFG_WGLPixelFormatQuery *query )
{
    PIXELFORMATDESCRIPTOR pfd;
    WNDCLASS wndCls;
    int pixelFormat;

    memset( query, 0, sizeof( *query ) );
    memset( &wndCls, 0, sizeof( wndCls ) );
    wndCls.lpfnWndProc = DefWindowProc;
    wndCls.hInstance = fgDisplay.pDisplay.Instance;
    wndCls.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wndCls.lpszClassName = _T("FREEGLUT_dstr_dummy");
    RegisterClass( &wndCls );

    query->hWnd = CreateWindow( _T("FREEGLUT_dstr_dummy"), _T(""),
                                WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW,
                                0, 0, 0, 0, 0, 0, fgDisplay.pDisplay.Instance, 0 );
    if( !query->hWnd )
        goto fail;

    query->hDC = GetDC( query->hWnd );
    if( !query->hDC )
        goto fail;

    memset( &pfd, 0, sizeof( pfd ) );
    pfd.nSize = sizeof( pfd );
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    pixelFormat = ChoosePixelFormat( query->hDC, &pfd );
    if( !pixelFormat || !SetPixelFormat( query->hDC, pixelFormat, &pfd ) )
        goto fail;

    query->hRC = wglCreateContext( query->hDC );
    if( !query->hRC || !wglMakeCurrent( query->hDC, query->hRC ) )
        goto fail;

    query->getPixelFormatAttribivARB =
        (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)wglGetProcAddress( "wglGetPixelFormatAttribivARB" );
    query->hasMultisample = fghWGLHasExtension( query->hDC, "WGL_ARB_multisample" );
    query->hasSRGB = fghWGLHasExtension( query->hDC, "WGL_ARB_framebuffer_sRGB" ) ||
                     fghWGLHasExtension( query->hDC, "WGL_EXT_framebuffer_sRGB" );
    return query->getPixelFormatAttribivARB != NULL;

fail:
    fghDestroyWGLPixelFormatQuery( query );
    memset( query, 0, sizeof( *query ) );
    return 0;
}

static int fghGetPixelFormatAttr( const SFG_WGLPixelFormatQuery *query, HDC hdc, int pixelFormat,
                                  int layerPlane, int attribute, int defaultValue )
{
    int value = defaultValue;
    if( query->getPixelFormatAttribivARB &&
        query->getPixelFormatAttribivARB( hdc, pixelFormat, layerPlane, 1, &attribute, &value ) )
        return value;
    return defaultValue;
}

static void fghFillPixelFormatCaps( SFG_DisplayStringMode *mode, HDC hdc,
                                    const PIXELFORMATDESCRIPTOR *pfd, int pixelFormat, unsigned char layer_type,
                                    const SFG_WGLPixelFormatQuery *query, int wantSRGB )
{
    int acceleration;
    int sampleBuffers;
    int samples;
    int srgbCapable;

    memset( mode->cap, 0, sizeof( mode->cap ) );
    mode->valid = 0;

    if( !( pfd->dwFlags & PFD_SUPPORT_OPENGL ) || !( pfd->dwFlags & PFD_DRAW_TO_WINDOW ) )
        return;
    if( pfd->iLayerType != layer_type )
        return;

    mode->valid = 1;
    mode->cap[ FGDSTR_CAP_NUM ] = pixelFormat;
    mode->cap[ FGDSTR_CAP_BUFFER_SIZE ] = pfd->cColorBits + ( pfd->iPixelType == PFD_TYPE_RGBA ? pfd->cAlphaBits : 0 );
    mode->cap[ FGDSTR_CAP_DOUBLEBUFFER ] = ( pfd->dwFlags & PFD_DOUBLEBUFFER ) != 0;
    mode->cap[ FGDSTR_CAP_STEREO ] = ( pfd->dwFlags & PFD_STEREO ) != 0;
    mode->cap[ FGDSTR_CAP_AUX_BUFFERS ] = pfd->cAuxBuffers;
    mode->cap[ FGDSTR_CAP_RED_SIZE ] = pfd->cRedBits;
    mode->cap[ FGDSTR_CAP_GREEN_SIZE ] = pfd->cGreenBits;
    mode->cap[ FGDSTR_CAP_BLUE_SIZE ] = pfd->cBlueBits;
    mode->cap[ FGDSTR_CAP_ALPHA_SIZE ] = pfd->cAlphaBits;
    mode->cap[ FGDSTR_CAP_DEPTH_SIZE ] = pfd->cDepthBits;
    mode->cap[ FGDSTR_CAP_STENCIL_SIZE ] = pfd->cStencilBits;
    mode->cap[ FGDSTR_CAP_ACCUM_RED_SIZE ] = pfd->cAccumRedBits;
    mode->cap[ FGDSTR_CAP_ACCUM_GREEN_SIZE ] = pfd->cAccumGreenBits;
    mode->cap[ FGDSTR_CAP_ACCUM_BLUE_SIZE ] = pfd->cAccumBlueBits;
    mode->cap[ FGDSTR_CAP_ACCUM_ALPHA_SIZE ] = pfd->cAccumAlphaBits;
    mode->cap[ FGDSTR_CAP_LEVEL ] = 0;
    mode->cap[ FGDSTR_CAP_XVISUAL ] = pixelFormat;
    mode->cap[ FGDSTR_CAP_TRANSPARENT ] = 0;
    mode->cap[ FGDSTR_CAP_RGBA_MODE ] = ( pfd->iPixelType == PFD_TYPE_RGBA );
    mode->cap[ FGDSTR_CAP_CI_MODE ] = ( pfd->iPixelType == PFD_TYPE_COLORINDEX );
    mode->cap[ FGDSTR_CAP_LUMINANCE_MODE ] = 0;

    acceleration = WGL_FULL_ACCELERATION_ARB;
    if( query->getPixelFormatAttribivARB )
        acceleration = fghGetPixelFormatAttr( query, hdc, pixelFormat, 0, WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB );
    else if( pfd->dwFlags & PFD_GENERIC_FORMAT )
        acceleration = 0;

    mode->cap[ FGDSTR_CAP_SLOW ] = ( acceleration != WGL_FULL_ACCELERATION_ARB );
    mode->cap[ FGDSTR_CAP_CONFORMANT ] = !( pfd->dwFlags & PFD_GENERIC_FORMAT );

    mode->cap[ FGDSTR_CAP_SAMPLES ] = 0;
    if( query->getPixelFormatAttribivARB && query->hasMultisample )
    {
        sampleBuffers = fghGetPixelFormatAttr( query, hdc, pixelFormat, 0, WGL_SAMPLE_BUFFERS_ARB, 0 );
        samples = fghGetPixelFormatAttr( query, hdc, pixelFormat, 0, WGL_SAMPLES_ARB, 0 );
        mode->cap[ FGDSTR_CAP_SAMPLES ] = sampleBuffers ? samples : 0;
    }

    if( wantSRGB )
    {
        if( !query->getPixelFormatAttribivARB || !query->hasSRGB )
        {
            mode->valid = 0;
            return;
        }

        srgbCapable = fghGetPixelFormatAttr( query, hdc, pixelFormat, 0, WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, 0 );
        if( !srgbCapable )
            mode->valid = 0;
    }
}

int fghChoosePixelFormatDisplayString( HDC hdc, unsigned char layer_type,
                                       int *pixelformat, PIXELFORMATDESCRIPTOR *ppfd,
                                       int *doubleBuffered, int *treatAsSingle )
{
    int maxPixelFormats;
    int i;
    int count = 0;
    int result = 0;
    int wantSRGB = ( fghDisplayStringWindowModeMask() & GLUT_SRGB ) != 0;
    int *pixelFormats = NULL;
    PIXELFORMATDESCRIPTOR *pfds = NULL;
    SFG_DisplayStringMode *modes = NULL;
    SFG_DisplayStringCriteria criteria;
    SFG_DisplayStringParserOptions options;
    SFG_DisplayStringSelection selection;
    SFG_WGLPixelFormatQuery query;

    if( !fghDisplayStringIsActive() )
        return 0;

    memset( &criteria, 0, sizeof( criteria ) );
    memset( &options, 0, sizeof( options ) );
    memset( &selection, 0, sizeof( selection ) );
    memset( &query, 0, sizeof( query ) );

    options.target = FGDSTR_TARGET_WIN32;
    criteria = fghDisplayStringParseModeString( fgState.DisplayString, &options );
    if( !criteria.criteria )
        return 0;

    maxPixelFormats = DescribePixelFormat( hdc, 1, 0, NULL );
    if( maxPixelFormats <= 0 )
        goto done;

    pixelFormats = malloc( sizeof( *pixelFormats ) * maxPixelFormats );
    pfds = malloc( sizeof( *pfds ) * maxPixelFormats );
    modes = calloc( maxPixelFormats, sizeof( *modes ) );
    if( !pixelFormats || !pfds || !modes )
        goto done;

    fghInitWGLPixelFormatQuery( &query );

    for( i = 1; i <= maxPixelFormats; ++i )
    {
        if( !DescribePixelFormat( hdc, i, sizeof( PIXELFORMATDESCRIPTOR ), &pfds[ count ] ) )
            continue;

        pixelFormats[ count ] = i;
        modes[ count ].data = &pixelFormats[ count ];
        fghFillPixelFormatCaps( &modes[ count ], hdc, &pfds[ count ], i, layer_type, &query, wantSRGB );
        if( modes[ count ].valid )
            ++count;
    }

    if( count <= 0 )
        goto done;

    selection = fghDisplayStringChooseMode( modes, count, &criteria, GL_TRUE );
    if( !selection.mode )
        goto done;

    for( i = 0; i < count; ++i )
    {
        if( &pixelFormats[ i ] == selection.mode->data )
        {
            *pixelformat = pixelFormats[ i ];
            if( ppfd )
                *ppfd = pfds[ i ];
            if( doubleBuffered )
                *doubleBuffered = modes[ i ].cap[ FGDSTR_CAP_DOUBLEBUFFER ];
            if( treatAsSingle )
                *treatAsSingle = selection.usedDoubleRetry ? 1 : 0;
            result = 1;
            break;
        }
    }

done:
    fghDestroyWGLPixelFormatQuery( &query );
    fghDisplayStringFreeCriteria( &criteria );
    free( modes );
    free( pfds );
    free( pixelFormats );
    return result;
}

#endif
