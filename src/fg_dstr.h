/*
 * Shared display-string parsing and selection helpers.
 */

#ifndef FG_DSTR_H__
#define FG_DSTR_H__

#include <GL/freeglut.h>

typedef enum
{
    FGDSTR_TARGET_X11,
    FGDSTR_TARGET_WIN32,
    FGDSTR_TARGET_EGL,
    FGDSTR_TARGET_COCOA
} fgDisplayStringTarget;

typedef enum
{
    FGDSTR_CMP_NONE,
    FGDSTR_CMP_EQ,
    FGDSTR_CMP_NEQ,
    FGDSTR_CMP_LTE,
    FGDSTR_CMP_GTE,
    FGDSTR_CMP_GT,
    FGDSTR_CMP_LT,
    FGDSTR_CMP_MIN
} SFG_DisplayStringComparison;

enum
{
    FGDSTR_CAP_RGBA = 0,
    FGDSTR_CAP_BUFFER_SIZE,
    FGDSTR_CAP_DOUBLEBUFFER,
    FGDSTR_CAP_STEREO,
    FGDSTR_CAP_AUX_BUFFERS,
    FGDSTR_CAP_RED_SIZE,
    FGDSTR_CAP_GREEN_SIZE,
    FGDSTR_CAP_BLUE_SIZE,
    FGDSTR_CAP_ALPHA_SIZE,
    FGDSTR_CAP_DEPTH_SIZE,
    FGDSTR_CAP_STENCIL_SIZE,
    FGDSTR_CAP_ACCUM_RED_SIZE,
    FGDSTR_CAP_ACCUM_GREEN_SIZE,
    FGDSTR_CAP_ACCUM_BLUE_SIZE,
    FGDSTR_CAP_ACCUM_ALPHA_SIZE,
    FGDSTR_CAP_LEVEL,
    FGDSTR_NUM_GLXCAPS,
    FGDSTR_CAP_XVISUAL = FGDSTR_NUM_GLXCAPS,
    FGDSTR_CAP_TRANSPARENT,
    FGDSTR_CAP_SAMPLES,
    FGDSTR_CAP_XSTATICGRAY,
    FGDSTR_CAP_XGRAYSCALE,
    FGDSTR_CAP_XSTATICCOLOR,
    FGDSTR_CAP_XPSEUDOCOLOR,
    FGDSTR_CAP_XTRUECOLOR,
    FGDSTR_CAP_XDIRECTCOLOR,
    FGDSTR_CAP_SLOW,
    FGDSTR_CAP_CONFORMANT,
    FGDSTR_NUM_CAPS,
    FGDSTR_CAP_NUM = FGDSTR_NUM_CAPS,
    FGDSTR_CAP_RGBA_MODE,
    FGDSTR_CAP_CI_MODE,
    FGDSTR_CAP_LUMINANCE_MODE
};

typedef struct tagSFG_DisplayStringCriterion SFG_DisplayStringCriterion;
struct tagSFG_DisplayStringCriterion
{
    int capability;
    int comparison;
    int value;
};

typedef struct tagSFG_DisplayStringMode SFG_DisplayStringMode;
struct tagSFG_DisplayStringMode
{
    const void *data;
    int         valid;
    int         cap[FGDSTR_NUM_CAPS];
};

typedef struct tagSFG_DisplayStringParserOptions SFG_DisplayStringParserOptions;
struct tagSFG_DisplayStringParserOptions
{
    fgDisplayStringTarget             target;
    GLboolean                         supportsMultisample;
    GLboolean                         supportsSlow;
    GLboolean                         supportsConformant;
    GLboolean                         supportsTransparent;
    GLboolean                         isMesa;
    const SFG_DisplayStringCriterion *requiredCriteria;
    int                               nRequired;
    int                               requiredMask;
};

typedef struct tagSFG_DisplayStringCriteria SFG_DisplayStringCriteria;
struct tagSFG_DisplayStringCriteria
{
    SFG_DisplayStringCriterion *criteria;
    int                        ncriteria;
    GLboolean                  allowDoubleAsSingle;
    int                        mask;
};

typedef struct tagSFG_DisplayStringSelection SFG_DisplayStringSelection;
struct tagSFG_DisplayStringSelection
{
    const SFG_DisplayStringMode *mode;
    GLboolean                    usedDoubleRetry;
};

unsigned int fghDisplayStringParseExtraMode( const char *mode );
unsigned int fghDisplayStringWindowModeMask( void );
GLboolean    fghDisplayStringIsActive( void );

SFG_DisplayStringCriteria fghDisplayStringParseModeString(
    const char *mode,
    const SFG_DisplayStringParserOptions *options
);
void fghDisplayStringFreeCriteria( SFG_DisplayStringCriteria *criteria );
void fghDisplayStringRewriteSingleAsDouble( SFG_DisplayStringCriteria *criteria );

const SFG_DisplayStringMode *fghDisplayStringFindMatch(
    const SFG_DisplayStringMode *modes,
    int nmodes,
    const SFG_DisplayStringCriterion *criteria,
    int ncriteria
);

SFG_DisplayStringSelection fghDisplayStringChooseMode(
    const SFG_DisplayStringMode *modes,
    int nmodes,
    const SFG_DisplayStringCriteria *criteria,
    GLboolean allowRetry
);

#endif
