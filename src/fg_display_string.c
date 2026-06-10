/*
 * fg_display_string.c
 *
 * Shared glutInitDisplayString criteria model: default resolution, hard
 * filtering, and left-to-right lexicographic ranking. Backends supply a
 * per-capability value array (indexed by FGCapability) and reuse this logic
 * so matching semantics stay identical across platforms.
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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define FREEGLUT_BUILDING_LIB
#include <GL/freeglut.h>
#include "fg_internal.h"

FGCriterion fghMakeCriterion( FGCriterionComparison comparison, int value )
{
    FGCriterion c;

    c.comparison = comparison;
    c.value      = value;
    return c;
}

GLboolean fghCriterionIsConcrete( FGCriterionComparison comparison )
{
    /* Concrete comparators are the contiguous range FG_EQ..FG_MIN; this also
     * rejects the (FGCriterionComparison)(-1) invalid sentinel. */
    return ( comparison >= FG_EQ && comparison <= FG_MIN ) ? GL_TRUE : GL_FALSE;
}

void fghAddCriterion( FGDisplayStringCriteria *c, FGCapability capability,
                      FGCriterion parsed, FGCriterion deflt )
{
    if ( c->count >= FG_MAX_DISPLAY_CRITERIA )
        return;

    c->entries[ c->count ].capability = capability;
    c->entries[ c->count ].criterion  =
        fghCriterionIsConcrete( parsed.comparison ) ? parsed : deflt;
    c->count++;
}

static GLboolean fghCriterionPasses( FGCriterion crit, int fbvalue )
{
    switch ( crit.comparison ) {
    case FG_EQ:   return ( fbvalue == crit.value ) ? GL_TRUE : GL_FALSE;
    case FG_NEQ:  return ( fbvalue != crit.value ) ? GL_TRUE : GL_FALSE;
    case FG_LT:   return ( fbvalue <  crit.value ) ? GL_TRUE : GL_FALSE;
    case FG_GT:   return ( fbvalue >  crit.value ) ? GL_TRUE : GL_FALSE;
    case FG_LTE:  return ( fbvalue <= crit.value ) ? GL_TRUE : GL_FALSE;
    case FG_GTE:  return ( fbvalue >= crit.value ) ? GL_TRUE : GL_FALSE;
    case FG_MIN:  return ( fbvalue >= crit.value ) ? GL_TRUE : GL_FALSE; /* ~ */
    default:      return GL_TRUE;  /* FG_NONE / unresolved: no constraint */
    }
}

GLboolean fghCriteriaPass( const FGDisplayStringCriteria *c, const int *values )
{
    int i;

    for ( i = 0; i < c->count; i++ ) {
        const FGCapabilityCriterion *e = &c->entries[ i ];

        if ( !fghCriterionPasses( e->criterion, values[ e->capability ] ) )
            return GL_FALSE;
    }
    return GL_TRUE;
}

static GLboolean fghHasCapability( const FGDisplayStringCriteria *c, FGCapability capability )
{
    int i;

    for ( i = 0; i < c->count; i++ )
        if ( c->entries[ i ].capability == capability )
            return GL_TRUE;
    return GL_FALSE;
}

static void fghPrependCriterion( FGDisplayStringCriteria *c, FGCapability capability,
                                 FGCriterion criterion )
{
    if ( c->count >= FG_MAX_DISPLAY_CRITERIA )
        return;

    memmove( &c->entries[ 1 ], &c->entries[ 0 ], (size_t)c->count * sizeof( c->entries[ 0 ] ) );
    c->entries[ 0 ].capability = capability;
    c->entries[ 0 ].criterion  = criterion;
    c->count++;
}

void fghAppendUnspecifiedCriteriaDefaults( FGDisplayStringCriteria *c, GLboolean indexMode )
{
    /* Mirror original GLUT's parseModeString(): every capability not named in
     * the display string gets a low-priority criterion appended after the
     * user's entries, so terse strings behave sensibly and prefer minimal
     * configurations (no over-allocation of samples, accum, aux, depth or
     * stencil). Append order follows glut_dstr.c and matters: these rank
     * strictly below everything the user wrote. */
    FGCriterion exactlyZero  = fghMakeCriterion( FG_EQ, 0 );
    FGCriterion preferZero   = fghMakeCriterion( FG_MIN, 0 );
    FGCriterion atLeastOne   = fghMakeCriterion( FG_GTE, 1 );

    /* Unlike all the other defaults, GLUT gives the undesignated "slow"
     * preference HIGHER priority than the user's criteria (it is prepended):
     * slow-caveat formats are avoided whenever possible, but still allowed.
     * On platforms that cannot detect slowness every format reports 0 and
     * this is a no-op. (Notably, Mesa marks accumulation-capable GLX
     * configs with the slow caveat.) */
    if ( !fghHasCapability( c, FG_CAP_SLOW ) )
        fghPrependCriterion( c, FG_CAP_SLOW, preferZero );

    if ( !fghHasCapability( c, FG_CAP_SAMPLES ) )
        fghAddCriterion( c, FG_CAP_SAMPLES, exactlyZero, exactlyZero );

    if ( !fghHasCapability( c, FG_CAP_ACCUM_RED ) ) {
        fghAddCriterion( c, FG_CAP_ACCUM_RED,   preferZero, preferZero );
        fghAddCriterion( c, FG_CAP_ACCUM_GREEN, preferZero, preferZero );
        fghAddCriterion( c, FG_CAP_ACCUM_BLUE,  preferZero, preferZero );
        fghAddCriterion( c, FG_CAP_ACCUM_ALPHA, preferZero, preferZero );
    }

    if ( !fghHasCapability( c, FG_CAP_AUX ) )
        fghAddCriterion( c, FG_CAP_AUX, preferZero, preferZero );

    if ( !indexMode &&
         !fghHasCapability( c, FG_CAP_RED ) && !fghHasCapability( c, FG_CAP_GREEN ) &&
         !fghHasCapability( c, FG_CAP_BLUE ) && !fghHasCapability( c, FG_CAP_ALPHA ) ) {
        fghAddCriterion( c, FG_CAP_RED,   atLeastOne, atLeastOne );
        fghAddCriterion( c, FG_CAP_GREEN, atLeastOne, atLeastOne );
        fghAddCriterion( c, FG_CAP_BLUE,  atLeastOne, atLeastOne );
        fghAddCriterion( c, FG_CAP_ALPHA, preferZero, preferZero );
    }

    if ( !fghHasCapability( c, FG_CAP_DEPTH ) )
        fghAddCriterion( c, FG_CAP_DEPTH, preferZero, preferZero );

    if ( !fghHasCapability( c, FG_CAP_STENCIL ) )
        fghAddCriterion( c, FG_CAP_STENCIL, preferZero, preferZero );
}

/* Ranking direction per original GLUT findMatch() scoring: <, <=, > and >=
 * all maximise (fbvalue - cvalue), i.e. prefer MORE (for < and <= that means
 * the closest value under the bound -- this is what makes bare "samples",
 * which defaults to "<=4", actually pick 4 samples rather than 0). Only ~
 * (FG_MIN) prefers less; = and != are exact (no ranking contribution).
 * Note the man page's "preferring larger difference" wording for < and <=
 * contradicts the GLUT implementation; the implementation wins. */
static int fghPreferenceDir( FGCriterionComparison comparison )
{
    switch ( comparison ) {
    case FG_GT:
    case FG_GTE:
    case FG_LT:
    case FG_LTE:
        return 1;
    case FG_MIN:
        return -1;
    default:
        return 0;
    }
}

int fghCriteriaCompare( const FGDisplayStringCriteria *c,
                        const int *aValues, const int *bValues )
{
    int i;

    for ( i = 0; i < c->count; i++ ) {
        const FGCapabilityCriterion *e = &c->entries[ i ];
        int dir = fghPreferenceDir( e->criterion.comparison );
        int va, vb;

        if ( dir == 0 )
            continue;

        va = aValues[ e->capability ];
        vb = bValues[ e->capability ];
        if ( va == vb )
            continue;

        if ( dir > 0 )
            return ( va > vb ) ? -1 : 1;
        return ( va < vb ) ? -1 : 1;
    }
    return 0;
}
