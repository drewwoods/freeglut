/*
 * Shared display-string parsing and matching.
 *
 * This file ports the original GLUT parsing and scoring logic while keeping
 * the platform-specific config enumeration in backend helpers.
 */

#define FREEGLUT_BUILDING_LIB
#include <GL/freeglut.h>
#include "fg_internal.h"
#include "fg_dstr.h"


static GLboolean fghWordBaseEquals( const char *word, size_t baseLength, const char *token )
{
    return ( strlen( token ) == baseLength ) && ( strncmp( word, token, baseLength ) == 0 );
}

static GLboolean fghIsVisualClassToken( const char *word, size_t baseLength )
{
    return fghWordBaseEquals( word, baseLength, "xvisual" ) ||
           fghWordBaseEquals( word, baseLength, "xstaticgray" ) ||
           fghWordBaseEquals( word, baseLength, "xgrayscale" ) ||
           fghWordBaseEquals( word, baseLength, "xstaticcolor" ) ||
           fghWordBaseEquals( word, baseLength, "xpseudocolor" ) ||
           fghWordBaseEquals( word, baseLength, "xtruecolor" ) ||
           fghWordBaseEquals( word, baseLength, "xdirectcolor" ) ||
           fghWordBaseEquals( word, baseLength, "xstaticgrey" ) ||
           fghWordBaseEquals( word, baseLength, "xgreyscale" ) ||
           fghWordBaseEquals( word, baseLength, "xstaticcolour" ) ||
           fghWordBaseEquals( word, baseLength, "xpseudocolour" ) ||
           fghWordBaseEquals( word, baseLength, "xtruecolour" ) ||
           fghWordBaseEquals( word, baseLength, "xdirectcolour" );
}

static GLboolean fghDisplayStringTokenSupported(
    const char *word,
    size_t baseLength,
    const SFG_DisplayStringParserOptions *options
)
{
    if ( fghWordBaseEquals( word, baseLength, "borderless" ) ) {
        return GL_TRUE;
    }

    if ( ( options->target != FGDSTR_TARGET_X11 ) &&
         fghIsVisualClassToken( word, baseLength ) ) {
        return GL_FALSE;
    }

    if ( ( options->target != FGDSTR_TARGET_WIN32 ) &&
         ( fghWordBaseEquals( word, baseLength, "win32pfd" ) ||
           fghWordBaseEquals( word, baseLength, "win32pdf" ) ) ) {
        return GL_FALSE;
    }

    if ( ( options->target == FGDSTR_TARGET_EGL ||
           options->target == FGDSTR_TARGET_COCOA ) &&
         ( fghWordBaseEquals( word, baseLength, "index" ) ||
           fghWordBaseEquals( word, baseLength, "luminance" ) ) ) {
        return GL_FALSE;
    }

    return GL_TRUE;
}

static int fghParseCriteria(
    char *word,
    SFG_DisplayStringCriterion *criterion,
    int *mask,
    GLboolean *allowDoubleAsSingle
)
{
    char *cstr, *vstr, *response;
    int comparator, value;
    int rgb, rgba, acc, acca, count, i;

    cstr = strpbrk( word, "=><!~" );
    if ( cstr ) {
        switch ( cstr[0] ) {
        case '=':
            comparator = FGDSTR_CMP_EQ;
            vstr = &cstr[1];
            break;
        case '~':
            comparator = FGDSTR_CMP_MIN;
            vstr = &cstr[1];
            break;
        case '>':
            if ( cstr[1] == '=' ) {
                comparator = FGDSTR_CMP_GTE;
                vstr = &cstr[2];
            } else {
                comparator = FGDSTR_CMP_GT;
                vstr = &cstr[1];
            }
            break;
        case '<':
            if ( cstr[1] == '=' ) {
                comparator = FGDSTR_CMP_LTE;
                vstr = &cstr[2];
            } else {
                comparator = FGDSTR_CMP_LT;
                vstr = &cstr[1];
            }
            break;
        case '!':
            if ( cstr[1] == '=' ) {
                comparator = FGDSTR_CMP_NEQ;
                vstr = &cstr[2];
            } else {
                return -1;
            }
            break;
        default:
            return -1;
        }
        value = (int)strtol( vstr, &response, 0 );
        if ( response == vstr ) {
            return -1;
        }
        *cstr = '\0';
    } else {
        comparator = FGDSTR_CMP_NONE;
        value = 0;
    }

    switch ( word[0] ) {
    case 'a':
        if ( !strcmp( word, "alpha" ) ) {
            criterion[0].capability = FGDSTR_CAP_ALPHA_SIZE;
            if ( comparator == FGDSTR_CMP_NONE ) {
                criterion[0].comparison = FGDSTR_CMP_GTE;
                criterion[0].value = 1;
            } else {
                criterion[0].comparison = comparator;
                criterion[0].value = value;
            }
            *mask |= ( 1 << FGDSTR_CAP_RGBA );
            *mask |= ( 1 << FGDSTR_CAP_ALPHA_SIZE );
            *mask |= ( 1 << FGDSTR_CAP_RGBA_MODE );
            return 1;
        }
        acca = !strcmp( word, "acca" );
        acc = !strcmp( word, "acc" );
        if ( acc || acca ) {
            criterion[0].capability = FGDSTR_CAP_ACCUM_RED_SIZE;
            criterion[1].capability = FGDSTR_CAP_ACCUM_GREEN_SIZE;
            criterion[2].capability = FGDSTR_CAP_ACCUM_BLUE_SIZE;
            criterion[3].capability = FGDSTR_CAP_ACCUM_ALPHA_SIZE;
            if ( acca ) {
                count = 4;
            } else {
                count = 3;
                criterion[3].comparison = FGDSTR_CMP_MIN;
                criterion[3].value = 0;
            }
            if ( comparator == FGDSTR_CMP_NONE ) {
                comparator = FGDSTR_CMP_GTE;
                value = 8;
            }
            for ( i = 0; i < count; i++ ) {
                criterion[i].comparison = comparator;
                criterion[i].value = value;
            }
            *mask |= ( 1 << FGDSTR_CAP_ACCUM_RED_SIZE );
            return 4;
        }
        if ( !strcmp( word, "auxbufs" ) || !strcmp( word, "aux" ) ) {
            criterion[0].capability = FGDSTR_CAP_AUX_BUFFERS;
            if ( comparator == FGDSTR_CMP_NONE ) {
                criterion[0].comparison = FGDSTR_CMP_MIN;
                criterion[0].value = 1;
            } else {
                criterion[0].comparison = comparator;
                criterion[0].value = value;
            }
            *mask |= ( 1 << FGDSTR_CAP_AUX_BUFFERS );
            return 1;
        }
        return -1;

    case 'b':
        if ( !strcmp( word, "blue" ) ) {
            criterion[0].capability = FGDSTR_CAP_BLUE_SIZE;
            if ( comparator == FGDSTR_CMP_NONE ) {
                criterion[0].comparison = FGDSTR_CMP_GTE;
                criterion[0].value = 1;
            } else {
                criterion[0].comparison = comparator;
                criterion[0].value = value;
            }
            *mask |= ( 1 << FGDSTR_CAP_RGBA );
            *mask |= ( 1 << FGDSTR_CAP_RGBA_MODE );
            return 1;
        }
        if ( !strcmp( word, "buffer" ) ) {
            criterion[0].capability = FGDSTR_CAP_BUFFER_SIZE;
            if ( comparator == FGDSTR_CMP_NONE ) {
                criterion[0].comparison = FGDSTR_CMP_GTE;
                criterion[0].value = 1;
            } else {
                criterion[0].comparison = comparator;
                criterion[0].value = value;
            }
            return 1;
        }
        return -1;

    case 'c':
        if ( !strcmp( word, "conformant" ) ) {
            criterion[0].capability = FGDSTR_CAP_CONFORMANT;
            if ( comparator == FGDSTR_CMP_NONE ) {
                criterion[0].comparison = FGDSTR_CMP_EQ;
                criterion[0].value = 1;
            } else {
                criterion[0].comparison = comparator;
                criterion[0].value = value;
            }
            *mask |= ( 1 << FGDSTR_CAP_CONFORMANT );
            return 1;
        }
        return -1;

    case 'd':
        if ( !strcmp( word, "depth" ) ) {
            criterion[0].capability = FGDSTR_CAP_DEPTH_SIZE;
            if ( comparator == FGDSTR_CMP_NONE ) {
                criterion[0].comparison = FGDSTR_CMP_GTE;
                criterion[0].value = 12;
            } else {
                criterion[0].comparison = comparator;
                criterion[0].value = value;
            }
            *mask |= ( 1 << FGDSTR_CAP_DEPTH_SIZE );
            return 1;
        }
        if ( !strcmp( word, "double" ) ) {
            criterion[0].capability = FGDSTR_CAP_DOUBLEBUFFER;
            if ( comparator == FGDSTR_CMP_NONE ) {
                criterion[0].comparison = FGDSTR_CMP_EQ;
                criterion[0].value = 1;
            } else {
                criterion[0].comparison = comparator;
                criterion[0].value = value;
            }
            *mask |= ( 1 << FGDSTR_CAP_DOUBLEBUFFER );
            return 1;
        }
        return -1;

    case 'g':
        if ( !strcmp( word, "green" ) ) {
            criterion[0].capability = FGDSTR_CAP_GREEN_SIZE;
            if ( comparator == FGDSTR_CMP_NONE ) {
                criterion[0].comparison = FGDSTR_CMP_GTE;
                criterion[0].value = 1;
            } else {
                criterion[0].comparison = comparator;
                criterion[0].value = value;
            }
            *mask |= ( 1 << FGDSTR_CAP_RGBA );
            *mask |= ( 1 << FGDSTR_CAP_RGBA_MODE );
            return 1;
        }
        return -1;

    case 'i':
        if ( !strcmp( word, "index" ) ) {
            criterion[0].capability = FGDSTR_CAP_RGBA;
            criterion[0].comparison = FGDSTR_CMP_EQ;
            criterion[0].value = 0;
            *mask |= ( 1 << FGDSTR_CAP_RGBA );
            *mask |= ( 1 << FGDSTR_CAP_CI_MODE );
            criterion[1].capability = FGDSTR_CAP_BUFFER_SIZE;
            if ( comparator == FGDSTR_CMP_NONE ) {
                criterion[1].comparison = FGDSTR_CMP_GTE;
                criterion[1].value = 1;
            } else {
                criterion[1].comparison = comparator;
                criterion[1].value = value;
            }
            return 2;
        }
        return -1;

    case 'l':
        if ( !strcmp( word, "luminance" ) ) {
            criterion[0].capability = FGDSTR_CAP_RGBA;
            criterion[0].comparison = FGDSTR_CMP_EQ;
            criterion[0].value = 1;

            criterion[1].capability = FGDSTR_CAP_RED_SIZE;
            if ( comparator == FGDSTR_CMP_NONE ) {
                criterion[1].comparison = FGDSTR_CMP_GTE;
                criterion[1].value = 1;
            } else {
                criterion[1].comparison = comparator;
                criterion[1].value = value;
            }

            criterion[2].capability = FGDSTR_CAP_GREEN_SIZE;
            criterion[2].comparison = FGDSTR_CMP_EQ;
            criterion[2].value = 0;

            criterion[3].capability = FGDSTR_CAP_BLUE_SIZE;
            criterion[3].comparison = FGDSTR_CMP_EQ;
            criterion[3].value = 0;

            *mask |= ( 1 << FGDSTR_CAP_RGBA );
            *mask |= ( 1 << FGDSTR_CAP_RGBA_MODE );
            *mask |= ( 1 << FGDSTR_CAP_LUMINANCE_MODE );
            return 4;
        }
        return -1;

    case 'n':
        if ( !strcmp( word, "num" ) ) {
            criterion[0].capability = FGDSTR_CAP_NUM;
            if ( comparator == FGDSTR_CMP_NONE ) {
                return -1;
            } else {
                criterion[0].comparison = comparator;
                criterion[0].value = value;
                return 1;
            }
        }
        return -1;

    case 'r':
        if ( !strcmp( word, "red" ) ) {
            criterion[0].capability = FGDSTR_CAP_RED_SIZE;
            if ( comparator == FGDSTR_CMP_NONE ) {
                criterion[0].comparison = FGDSTR_CMP_GTE;
                criterion[0].value = 1;
            } else {
                criterion[0].comparison = comparator;
                criterion[0].value = value;
            }
            *mask |= ( 1 << FGDSTR_CAP_RGBA );
            *mask |= ( 1 << FGDSTR_CAP_RGBA_MODE );
            return 1;
        }
        rgba = !strcmp( word, "rgba" );
        rgb = !strcmp( word, "rgb" );
        if ( rgb || rgba ) {
            criterion[0].capability = FGDSTR_CAP_RGBA;
            criterion[0].comparison = FGDSTR_CMP_EQ;
            criterion[0].value = 1;

            criterion[1].capability = FGDSTR_CAP_RED_SIZE;
            criterion[2].capability = FGDSTR_CAP_GREEN_SIZE;
            criterion[3].capability = FGDSTR_CAP_BLUE_SIZE;
            criterion[4].capability = FGDSTR_CAP_ALPHA_SIZE;
            if ( rgba ) {
                count = 5;
            } else {
                count = 4;
                criterion[4].comparison = FGDSTR_CMP_MIN;
                criterion[4].value = 0;
            }
            if ( comparator == FGDSTR_CMP_NONE ) {
                comparator = FGDSTR_CMP_GTE;
                value = 1;
            }
            for ( i = 1; i < count; i++ ) {
                criterion[i].comparison = comparator;
                criterion[i].value = value;
            }
            *mask |= ( 1 << FGDSTR_CAP_RGBA );
            *mask |= ( 1 << FGDSTR_CAP_RGBA_MODE );
            return 5;
        }
        return -1;

    case 's':
        if ( !strcmp( word, "stencil" ) ) {
            criterion[0].capability = FGDSTR_CAP_STENCIL_SIZE;
            if ( comparator == FGDSTR_CMP_NONE ) {
                criterion[0].comparison = FGDSTR_CMP_MIN;
                criterion[0].value = 1;
            } else {
                criterion[0].comparison = comparator;
                criterion[0].value = value;
            }
            *mask |= ( 1 << FGDSTR_CAP_STENCIL_SIZE );
            return 1;
        }
        if ( !strcmp( word, "single" ) ) {
            criterion[0].capability = FGDSTR_CAP_DOUBLEBUFFER;
            if ( comparator == FGDSTR_CMP_NONE ) {
                criterion[0].comparison = FGDSTR_CMP_EQ;
                criterion[0].value = 0;
                *allowDoubleAsSingle = GL_TRUE;
                *mask |= ( 1 << FGDSTR_CAP_DOUBLEBUFFER );
                return 1;
            } else {
                return -1;
            }
        }
        if ( !strcmp( word, "stereo" ) ) {
            criterion[0].capability = FGDSTR_CAP_STEREO;
            if ( comparator == FGDSTR_CMP_NONE ) {
                criterion[0].comparison = FGDSTR_CMP_EQ;
                criterion[0].value = 1;
            } else {
                criterion[0].comparison = comparator;
                criterion[0].value = value;
            }
            *mask |= ( 1 << FGDSTR_CAP_STEREO );
            return 1;
        }
        if ( !strcmp( word, "samples" ) ) {
            criterion[0].capability = FGDSTR_CAP_SAMPLES;
            if ( comparator == FGDSTR_CMP_NONE ) {
                criterion[0].comparison = FGDSTR_CMP_LTE;
                criterion[0].value = 4;
            } else {
                criterion[0].comparison = comparator;
                criterion[0].value = value;
            }
            *mask |= ( 1 << FGDSTR_CAP_SAMPLES );
            return 1;
        }
        if ( !strcmp( word, "slow" ) ) {
            criterion[0].capability = FGDSTR_CAP_SLOW;
            if ( comparator == FGDSTR_CMP_NONE ) {
                criterion[0].comparison = FGDSTR_CMP_GTE;
                criterion[0].value = 0;
            } else {
                criterion[0].comparison = comparator;
                criterion[0].value = value;
            }
            *mask |= ( 1 << FGDSTR_CAP_SLOW );
            return 1;
        }
        return -1;

    case 'w':
        if ( !strcmp( word, "win32pfd" ) || !strcmp( word, "win32pdf" ) ) {
            criterion[0].capability = FGDSTR_CAP_XVISUAL;
            if ( comparator == FGDSTR_CMP_NONE ) {
                return -1;
            } else {
                criterion[0].comparison = comparator;
                criterion[0].value = value;
                return 1;
            }
        }
        return -1;

    case 'x':
        if ( !strcmp( word, "xvisual" ) ) {
            if ( comparator == FGDSTR_CMP_NONE ) {
                return -1;
            } else {
                criterion[0].capability = FGDSTR_CAP_XVISUAL;
                criterion[0].comparison = comparator;
                criterion[0].value = value;
                *mask |= ~0;
                return 1;
            }
        }
        if ( comparator == FGDSTR_CMP_NONE ) {
            criterion[0].comparison = FGDSTR_CMP_EQ;
            criterion[0].value = 1;
        } else {
            criterion[0].comparison = comparator;
            criterion[0].value = value;
        }

        if ( !strcmp( word, "xstaticgray" ) || !strcmp( word, "xstaticgrey" ) ) {
            criterion[0].capability = FGDSTR_CAP_XSTATICGRAY;
            *mask |= ( 1 << FGDSTR_CAP_XSTATICGRAY );
            return 1;
        }
        if ( !strcmp( word, "xgrayscale" ) || !strcmp( word, "xgreyscale" ) ) {
            criterion[0].capability = FGDSTR_CAP_XGRAYSCALE;
            *mask |= ( 1 << FGDSTR_CAP_XSTATICGRAY );
            return 1;
        }
        if ( !strcmp( word, "xstaticcolor" ) || !strcmp( word, "xstaticcolour" ) ) {
            criterion[0].capability = FGDSTR_CAP_XSTATICCOLOR;
            *mask |= ( 1 << FGDSTR_CAP_XSTATICGRAY );
            return 1;
        }
        if ( !strcmp( word, "xpseudocolor" ) || !strcmp( word, "xpseudocolour" ) ) {
            criterion[0].capability = FGDSTR_CAP_XPSEUDOCOLOR;
            *mask |= ( 1 << FGDSTR_CAP_XSTATICGRAY );
            return 1;
        }
        if ( !strcmp( word, "xtruecolor" ) || !strcmp( word, "xtruecolour" ) ) {
            criterion[0].capability = FGDSTR_CAP_XTRUECOLOR;
            *mask |= ( 1 << FGDSTR_CAP_XSTATICGRAY );
            return 1;
        }
        if ( !strcmp( word, "xdirectcolor" ) || !strcmp( word, "xdirectcolour" ) ) {
            criterion[0].capability = FGDSTR_CAP_XDIRECTCOLOR;
            *mask |= ( 1 << FGDSTR_CAP_XSTATICGRAY );
            return 1;
        }
        return -1;

    default:
        return -1;
    }
}

unsigned int fghDisplayStringParseExtraMode( const char *mode )
{
    unsigned int extraMode = 0;
    char *copy, *word;

    if ( !mode ) {
        return 0;
    }

    copy = strdup( mode );
    if ( !copy ) {
        fgError( "out of memory" );
    }

    word = strtok( copy, " \t" );
    while ( word ) {
        size_t baseLength = strcspn( word, "=><!~" );
        if ( fghWordBaseEquals( word, baseLength, "borderless" ) ) {
            extraMode |= GLUT_BORDERLESS;
        }
        word = strtok( NULL, " \t" );
    }

    free( copy );
    return extraMode;
}

unsigned int fghDisplayStringWindowModeMask( void )
{
    unsigned int keepMask = GLUT_BORDERLESS | GLUT_CAPTIONLESS | GLUT_SRGB;

    if ( fgState.DisplayString ) {
        return ( fgState.DisplayMode & keepMask ) | fgState.DisplayStringExtraMode;
    }
    return fgState.DisplayMode;
}

GLboolean fghDisplayStringIsActive( void )
{
    return fgState.DisplayString != NULL;
}

SFG_DisplayStringCriteria fghDisplayStringParseModeString(
    const char *mode,
    const SFG_DisplayStringParserOptions *options
)
{
    SFG_DisplayStringCriteria parsed;
    int n, mask, parsedCount, i;
    char *copy, *word;

    parsed.criteria = NULL;
    parsed.ncriteria = 0;
    parsed.allowDoubleAsSingle = GL_FALSE;
    parsed.mask = 0;

    if ( !mode ) {
        return parsed;
    }

    copy = strdup( mode );
    if ( !copy ) {
        fgError( "out of memory" );
    }

    n = 0;
    word = strtok( copy, " \t" );
    while ( word ) {
        n++;
        word = strtok( NULL, " \t" );
    }

    parsed.criteria = (SFG_DisplayStringCriterion *)malloc(
        ( 4 * n + 30 + options->nRequired ) * sizeof( SFG_DisplayStringCriterion )
    );
    if ( !parsed.criteria ) {
        free( copy );
        fgError( "out of memory" );
    }

    strcpy( copy, mode );

    mask = options->requiredMask;
    for ( i = 0; i < options->nRequired; i++ ) {
        parsed.criteria[i] = options->requiredCriteria[i];
    }
    n = options->nRequired;

    if ( options->supportsSlow && !( mask & ( 1 << FGDSTR_CAP_SLOW ) ) ) {
        parsed.criteria[n].capability = FGDSTR_CAP_SLOW;
        parsed.criteria[n].comparison = FGDSTR_CMP_MIN;
        parsed.criteria[n].value = 0;
        n++;
    }

    if ( options->supportsTransparent && !( mask & ( 1 << FGDSTR_CAP_TRANSPARENT ) ) ) {
        parsed.criteria[n].capability = FGDSTR_CAP_TRANSPARENT;
        parsed.criteria[n].comparison = FGDSTR_CMP_EQ;
        parsed.criteria[n].value = 0;
        n++;
    }

    word = strtok( copy, " \t" );
    while ( word ) {
        size_t baseLength = strcspn( word, "=><!~" );

        if ( fghWordBaseEquals( word, baseLength, "borderless" ) ) {
            word = strtok( NULL, " \t" );
            continue;
        }

        if ( !fghDisplayStringTokenSupported( word, baseLength, options ) ) {
            fgWarning( "Display string token not supported on this backend: %.*s",
                       (int)baseLength, word );
            word = strtok( NULL, " \t" );
            continue;
        }

        parsedCount = fghParseCriteria( word, &parsed.criteria[n], &mask, &parsed.allowDoubleAsSingle );
        if ( parsedCount >= 0 ) {
            n += parsedCount;
        } else {
            fgWarning( "Unrecognized display string word: %s (ignoring)", word );
        }
        word = strtok( NULL, " \t" );
    }

    if ( options->supportsMultisample ) {
        if ( !( mask & ( 1 << FGDSTR_CAP_SAMPLES ) ) ) {
            parsed.criteria[n].capability = FGDSTR_CAP_SAMPLES;
            parsed.criteria[n].comparison = FGDSTR_CMP_EQ;
            parsed.criteria[n].value = 0;
            n++;

            if ( options->supportsConformant &&
                 !( mask & ( 1 << FGDSTR_CAP_CONFORMANT ) ) ) {
                parsed.criteria[n].capability = FGDSTR_CAP_CONFORMANT;
                parsed.criteria[n].comparison = FGDSTR_CMP_EQ;
                parsed.criteria[n].value = 1;
                n++;
            }
        } else if ( options->supportsConformant &&
                    !( mask & ( 1 << FGDSTR_CAP_CONFORMANT ) ) ) {
            parsed.criteria[n].capability = FGDSTR_CAP_CONFORMANT;
            parsed.criteria[n].comparison = FGDSTR_CMP_MIN;
            parsed.criteria[n].value = 0;
            n++;
            mask |= ( 1 << FGDSTR_CAP_CONFORMANT );
        }
    }

    if ( !( mask & ( 1 << FGDSTR_CAP_ACCUM_RED_SIZE ) ) ) {
        parsed.criteria[n].capability = FGDSTR_CAP_ACCUM_RED_SIZE;
        parsed.criteria[n].comparison = FGDSTR_CMP_MIN;
        parsed.criteria[n].value = 0;
        parsed.criteria[n + 1].capability = FGDSTR_CAP_ACCUM_GREEN_SIZE;
        parsed.criteria[n + 1].comparison = FGDSTR_CMP_MIN;
        parsed.criteria[n + 1].value = 0;
        parsed.criteria[n + 2].capability = FGDSTR_CAP_ACCUM_BLUE_SIZE;
        parsed.criteria[n + 2].comparison = FGDSTR_CMP_MIN;
        parsed.criteria[n + 2].value = 0;
        parsed.criteria[n + 3].capability = FGDSTR_CAP_ACCUM_ALPHA_SIZE;
        parsed.criteria[n + 3].comparison = FGDSTR_CMP_MIN;
        parsed.criteria[n + 3].value = 0;
        n += 4;
    }

    if ( !( mask & ( 1 << FGDSTR_CAP_AUX_BUFFERS ) ) ) {
        parsed.criteria[n].capability = FGDSTR_CAP_AUX_BUFFERS;
        parsed.criteria[n].comparison = FGDSTR_CMP_MIN;
        parsed.criteria[n].value = 0;
        n++;
    }

    if ( !( mask & ( 1 << FGDSTR_CAP_RGBA ) ) ) {
        parsed.criteria[n].capability = FGDSTR_CAP_RGBA;
        parsed.criteria[n].comparison = FGDSTR_CMP_EQ;
        parsed.criteria[n].value = 1;
        parsed.criteria[n + 1].capability = FGDSTR_CAP_RED_SIZE;
        parsed.criteria[n + 1].comparison = FGDSTR_CMP_GTE;
        parsed.criteria[n + 1].value = 1;
        parsed.criteria[n + 2].capability = FGDSTR_CAP_GREEN_SIZE;
        parsed.criteria[n + 2].comparison = FGDSTR_CMP_GTE;
        parsed.criteria[n + 2].value = 1;
        parsed.criteria[n + 3].capability = FGDSTR_CAP_BLUE_SIZE;
        parsed.criteria[n + 3].comparison = FGDSTR_CMP_GTE;
        parsed.criteria[n + 3].value = 1;
        parsed.criteria[n + 4].capability = FGDSTR_CAP_ALPHA_SIZE;
        parsed.criteria[n + 4].comparison = FGDSTR_CMP_MIN;
        parsed.criteria[n + 4].value = 0;
        n += 5;
        mask |= ( 1 << FGDSTR_CAP_RGBA_MODE );
    }

    if ( options->target == FGDSTR_TARGET_X11 &&
         !( mask & ( 1 << FGDSTR_CAP_XSTATICGRAY ) ) ) {
        if ( ( mask & ( 1 << FGDSTR_CAP_RGBA_MODE ) ) && !options->isMesa ) {
            if ( mask & ( 1 << FGDSTR_CAP_LUMINANCE_MODE ) ) {
                parsed.criteria[n].capability = FGDSTR_CAP_XSTATICGRAY;
            } else {
                parsed.criteria[n].capability = FGDSTR_CAP_XTRUECOLOR;
            }
            parsed.criteria[n].comparison = FGDSTR_CMP_EQ;
            parsed.criteria[n].value = 1;
            n++;
        }
        if ( mask & ( 1 << FGDSTR_CAP_CI_MODE ) ) {
            parsed.criteria[n].capability = FGDSTR_CAP_XPSEUDOCOLOR;
            parsed.criteria[n].comparison = FGDSTR_CMP_EQ;
            parsed.criteria[n].value = 1;
            n++;
        }
    }

    if ( !( mask & ( 1 << FGDSTR_CAP_STEREO ) ) ) {
        parsed.criteria[n].capability = FGDSTR_CAP_STEREO;
        parsed.criteria[n].comparison = FGDSTR_CMP_EQ;
        parsed.criteria[n].value = 0;
        n++;
    }

    if ( !( mask & ( 1 << FGDSTR_CAP_DOUBLEBUFFER ) ) ) {
        parsed.criteria[n].capability = FGDSTR_CAP_DOUBLEBUFFER;
        parsed.criteria[n].comparison = FGDSTR_CMP_EQ;
        parsed.criteria[n].value = 0;
        parsed.allowDoubleAsSingle = GL_TRUE;
        n++;
    }

    if ( !( mask & ( 1 << FGDSTR_CAP_DEPTH_SIZE ) ) ) {
        parsed.criteria[n].capability = FGDSTR_CAP_DEPTH_SIZE;
        parsed.criteria[n].comparison = FGDSTR_CMP_MIN;
        parsed.criteria[n].value = 0;
        n++;
    }

    if ( !( mask & ( 1 << FGDSTR_CAP_STENCIL_SIZE ) ) ) {
        parsed.criteria[n].capability = FGDSTR_CAP_STENCIL_SIZE;
        parsed.criteria[n].comparison = FGDSTR_CMP_MIN;
        parsed.criteria[n].value = 0;
        n++;
    }

    if ( !( mask & ( 1 << FGDSTR_CAP_LEVEL ) ) ) {
        parsed.criteria[n].capability = FGDSTR_CAP_LEVEL;
        parsed.criteria[n].comparison = FGDSTR_CMP_EQ;
        parsed.criteria[n].value = 0;
        n++;
    }

    if ( n > 0 ) {
        parsed.criteria = (SFG_DisplayStringCriterion *)realloc(
            parsed.criteria,
            n * sizeof( SFG_DisplayStringCriterion )
        );
        if ( !parsed.criteria ) {
            free( copy );
            fgError( "out of memory" );
        }
    } else {
        free( parsed.criteria );
        parsed.criteria = NULL;
    }

    free( copy );
    parsed.ncriteria = n;
    parsed.mask = mask;
    return parsed;
}

void fghDisplayStringFreeCriteria( SFG_DisplayStringCriteria *criteria )
{
    if ( criteria->criteria ) {
        free( criteria->criteria );
    }
    criteria->criteria = NULL;
    criteria->ncriteria = 0;
    criteria->allowDoubleAsSingle = GL_FALSE;
    criteria->mask = 0;
}

void fghDisplayStringRewriteSingleAsDouble( SFG_DisplayStringCriteria *criteria )
{
    int i;

    for ( i = 0; i < criteria->ncriteria; i++ ) {
        if ( criteria->criteria[i].capability == FGDSTR_CAP_DOUBLEBUFFER &&
             criteria->criteria[i].comparison == FGDSTR_CMP_EQ &&
             criteria->criteria[i].value == 0 ) {
            criteria->criteria[i].value = 1;
        }
    }
}

const SFG_DisplayStringMode *fghDisplayStringFindMatch(
    const SFG_DisplayStringMode *modes,
    int nmodes,
    const SFG_DisplayStringCriterion *criteria,
    int ncriteria
)
{
    const SFG_DisplayStringMode *found;
    int *bestScore, *thisScore;
    int i, j, numok, worse, better;

    found = NULL;
    numok = 1;

    bestScore = (int *)malloc( ncriteria * sizeof( int ) );
    thisScore = (int *)malloc( ncriteria * sizeof( int ) );
    if ( !bestScore || !thisScore ) {
        free( bestScore );
        free( thisScore );
        fgError( "out of memory" );
    }

    for ( j = 0; j < ncriteria; j++ ) {
        bestScore[j] = -32768;
    }

    for ( i = 0; i < nmodes; i++ ) {
        if ( modes[i].valid ) {
            worse = 0;
            better = 0;

            for ( j = 0; j < ncriteria; j++ ) {
                int cap, cvalue, fbvalue, result;

                cap = criteria[j].capability;
                cvalue = criteria[j].value;
                if ( cap == FGDSTR_CAP_NUM ) {
                    fbvalue = numok;
                } else {
                    fbvalue = modes[i].cap[cap];
                }

                result = 0;
                switch ( criteria[j].comparison ) {
                case FGDSTR_CMP_EQ:
                    result = cvalue == fbvalue;
                    thisScore[j] = 1;
                    break;
                case FGDSTR_CMP_NEQ:
                    result = cvalue != fbvalue;
                    thisScore[j] = 1;
                    break;
                case FGDSTR_CMP_LT:
                    result = fbvalue < cvalue;
                    thisScore[j] = fbvalue - cvalue;
                    break;
                case FGDSTR_CMP_GT:
                    result = fbvalue > cvalue;
                    thisScore[j] = fbvalue - cvalue;
                    break;
                case FGDSTR_CMP_LTE:
                    result = fbvalue <= cvalue;
                    thisScore[j] = fbvalue - cvalue;
                    break;
                case FGDSTR_CMP_GTE:
                    result = fbvalue >= cvalue;
                    thisScore[j] = fbvalue - cvalue;
                    break;
                case FGDSTR_CMP_MIN:
                    result = fbvalue >= cvalue;
                    thisScore[j] = cvalue - fbvalue;
                    break;
                default:
                    break;
                }

                if ( result ) {
                    if ( better || thisScore[j] > bestScore[j] ) {
                        better = 1;
                    } else if ( thisScore[j] < bestScore[j] ) {
                        goto next_mode;
                    }
                } else {
                    if ( cap == FGDSTR_CAP_NUM ) {
                        worse = 1;
                    } else {
                        goto next_mode;
                    }
                }
            }

            if ( better && !worse ) {
                found = &modes[i];
                for ( j = 0; j < ncriteria; j++ ) {
                    bestScore[j] = thisScore[j];
                }
            }
            numok++;
        }
next_mode:
        ;
    }

    free( bestScore );
    free( thisScore );
    return found;
}

SFG_DisplayStringSelection fghDisplayStringChooseMode(
    const SFG_DisplayStringMode *modes,
    int nmodes,
    const SFG_DisplayStringCriteria *criteria,
    GLboolean allowRetry
)
{
    SFG_DisplayStringSelection result;
    SFG_DisplayStringCriteria localCriteria;

    result.mode = NULL;
    result.usedDoubleRetry = GL_FALSE;

    if ( !criteria->criteria ) {
        return result;
    }

    result.mode = fghDisplayStringFindMatch(
        modes, nmodes, criteria->criteria, criteria->ncriteria
    );
    if ( result.mode || !allowRetry || !criteria->allowDoubleAsSingle ) {
        return result;
    }

    localCriteria = *criteria;
    localCriteria.criteria = (SFG_DisplayStringCriterion *)malloc(
        criteria->ncriteria * sizeof( SFG_DisplayStringCriterion )
    );
    if ( !localCriteria.criteria ) {
        fgError( "out of memory" );
    }
    memcpy( localCriteria.criteria, criteria->criteria,
            criteria->ncriteria * sizeof( SFG_DisplayStringCriterion ) );
    fghDisplayStringRewriteSingleAsDouble( &localCriteria );

    result.mode = fghDisplayStringFindMatch(
        modes, nmodes, localCriteria.criteria, localCriteria.ncriteria
    );
    result.usedDoubleRetry = ( result.mode != NULL );

    free( localCriteria.criteria );
    return result;
}
